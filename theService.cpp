/* last changed 16.10.26 */
#include "stdafx.h"

#include "theService.h"
#include "helper_funcs.h"
#include "sensitive_data.h"
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

BEGIN_MESSAGE_MAP(CBgHelperSrv, CWinApp)
	//{{AFX_MSG_MAP(CBgHelperSrv)
	//}}AFX_MSG
	// ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()

/* Единственный объект приложения. Вся работа - в InitInstance, конструктор ничего не делает */
CBgHelperSrv bg_service;

namespace
{
	constexpr char sz_unique_named_mutex[] = "bghelpermutex";
	/* Именованное событие: его установка любым процессом пользователя завершает bgHelper */
	constexpr wchar_t sz_stop_event_name[] = L"Local\\bghelper_stop_event";
	constexpr wchar_t sz_pipe_name[] = L"\\\\.\\Pipe\\bghelperpipe";
	/* Период проверки расписания в INI-файле */
	constexpr DWORD timer_period_ms = 20 * 1000;

	BOOL WINAPI on_console_ctrl(DWORD /*ctrl_type*/) {
		bg_service.request_stop();
		return TRUE;
	}
}

CBgHelperSrv::CBgHelperSrv() = default;

bool CBgHelperSrv::socket_connect() {
	if (!wsa_started_)
		return false;

	/* Проверка процесса медленная, делаем её до захвата мьютекса */
	if (!is_process_running(L"ifc_exporter.exe")) {
		std::lock_guard lock(socket_mu_);
		close_socket_locked();
		return false;
	}

	if (m_connected_to_qml)
		return true;

	/* connect к закрытому порту localhost на Windows длится до ~2 с, поэтому
	 * соединяемся без мьютекса и публикуем сокет только после успеха */
	SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (s == INVALID_SOCKET)
		return false;

	if (connect(s, reinterpret_cast<const SOCKADDR*>(&addr_), sizeof(addr_)) == SOCKET_ERROR) {
		closesocket(s);
		return false;
	}

	std::lock_guard lock(socket_mu_);
	if (socket_ != INVALID_SOCKET) {
		/* Другой поток успел подключиться раньше */
		closesocket(s);
		return true;
	}
	socket_ = s;
	m_connected_to_qml = true;
	return true;
}

void CBgHelperSrv::socket_send(std::string_view data) {
	if (!m_connected_to_qml)
		return;

	std::lock_guard lock(socket_mu_);
	if (socket_ == INVALID_SOCKET)
		return;

	while (!data.empty()) {
		const int sent = send(socket_, data.data(), static_cast<int>(data.size()), 0);
		if (sent == SOCKET_ERROR) {
			LOG_SAVE << "send() failed, WSA error " << WSAGetLastError() << ", closing socket";
			close_socket_locked();
			return;
		}
		data.remove_prefix(static_cast<size_t>(sent));
	}
}

void CBgHelperSrv::close_socket_locked() {
	if (socket_ != INVALID_SOCKET) {
		shutdown(socket_, SD_BOTH);
		closesocket(socket_);
		socket_ = INVALID_SOCKET;
	}
	m_connected_to_qml = false;
}

void CBgHelperSrv::close_socket() {
	std::lock_guard lock(socket_mu_);
	close_socket_locked();
}

void CBgHelperSrv::request_start_immediately() const {
	if (start_event_)
		SetEvent(start_event_);
}

void CBgHelperSrv::request_stop() const {
	if (stop_event_)
		SetEvent(stop_event_);
}

/////////////////////////////////////////////////////////////////////////////
// CBgHelperSrv initialization

BOOL CBgHelperSrv::InitInstance() {
	CWinApp::InitInstance();

	rLogger::init_logging();

	/* Единственный экземпляр: мьютекс удерживается до ExitInstance */
	instance_mutex_ = CreateMutex(nullptr, FALSE, sz_unique_named_mutex);
	if (instance_mutex_ == nullptr || GetLastError() == ERROR_ALREADY_EXISTS) {
		LOG_SAVE << "The program is already running - exiting.";
		return FALSE;
	}

	inf_config_file_path_ = get_config_file_path();

	const CA2W inf_config_path(inf_config_file_path_.c_str());
	tcp_port_ = GetPrivateProfileIntW(L"Manufacturer", L"tcp_port", 7777, inf_config_path);

	/* Hide console window: */
	::ShowWindow(::GetConsoleWindow(), SW_HIDE);

	stop_event_ = CreateEventW(nullptr, TRUE, FALSE, sz_stop_event_name);
	start_event_ = CreateEventW(nullptr, FALSE, FALSE, nullptr);
	if (stop_event_ == nullptr || start_event_ == nullptr) {
		LOG_SAVE << "CreateEvent failed, error " << GetLastError();
		return FALSE;
	}
	SetConsoleCtrlHandler(on_console_ctrl, TRUE);

	WSADATA wsa_data;
	wsa_started_ = WSAStartup(MAKEWORD(2, 2), &wsa_data) == 0;
	if (!wsa_started_)
		LOG_SAVE << "WSAStartup failed";

	addr_.sin_family = AF_INET;
	addr_.sin_port = htons(static_cast<u_short>(tcp_port_));
	InetPtonA(AF_INET, "127.0.0.1", &addr_.sin_addr);

	LOG_SAVE << "SocketConnect()...";
	socket_connect();

	const HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
	LOG_SAVE << "CoInitializeEx HRESULT: " << hr;

	{
		/* Фоновое (пере)подключение к ifc_exporter, останавливается при выходе из блока */
		active_object connector([this] { socket_connect(); std::this_thread::sleep_for(200ms); });

		/* Запущено как приложение */
		if (__argc == 1)
			run_pipe_server();
	}

	// Close the COM library
	if (SUCCEEDED(hr))
		CoUninitialize();

	close_socket();

	LOG_SAVE << "InitInstance: Exit";

	// Since the dialog has been closed, return FALSE so that we exit the
	//  application, rather than start the application's message pump.
	return FALSE;
}

/* Сервер канала и основной цикл: проверка расписания раз в timer_period_ms,
 * немедленный запуск по start_event_, выход по stop_event_ */
void CBgHelperSrv::run_pipe_server() {
	try
	{
		LOG_SAVE << "Starting pipeServer... ";
		CNamedPipeServer pipe_server(sz_pipe_name,
			pipe_message_handler,
			nullptr,
			256,
			256,
			10);

		LOG_SAVE << "Starting pipeServer connections";
		pipe_server.StartServing();

		remote_sav_checker sav_checker;
		const HANDLE wait_objects[] = { stop_event_, start_event_ };
		bool start_immediately = false;

		for (;;) {
			try
			{
				sav_checker.poll();
				ini_timer_check(start_immediately);
			}
			catch (exception& ex)
			{
				LOG_SAVE << "Timer check failed: " << ex.what();
			}

			const DWORD wait_result = WaitForMultipleObjects(2, wait_objects, FALSE, timer_period_ms);
			if (wait_result == WAIT_OBJECT_0) {
				LOG_SAVE << "Stop requested";
				break;
			}
			if (wait_result == WAIT_FAILED) {
				LOG_SAVE << "WaitForMultipleObjects failed, error " << GetLastError();
				break;
			}
			start_immediately = (wait_result == WAIT_OBJECT_0 + 1);
		}

		LOG_SAVE << "Initiating shutdown";
		pipe_server.Shutdown();
		LOG_SAVE << "Waiting until everything is shutdown";
		pipe_server.WaitUntilFinished(INFINITE);
		LOG_SAVE << "Shutdown finished";
	}
	catch (exception& ex)
	{
		LOG_SAVE << ex.what();
	}
}

int CBgHelperSrv::ExitInstance() {
	SetConsoleCtrlHandler(on_console_ctrl, FALSE);

	if (wsa_started_) {
		WSACleanup();
		wsa_started_ = false;
	}
	for (HANDLE* h : { &start_event_, &stop_event_, &instance_mutex_ }) {
		if (*h) {
			CloseHandle(*h);
			*h = nullptr;
		}
	}
	return CWinApp::ExitInstance();
}
