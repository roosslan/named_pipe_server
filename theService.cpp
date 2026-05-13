/* last changed 12.5.26 */
#include "stdafx.h"

#include <mutex>
#include <chrono>

#include <winsock2.h>
#include <ws2tcpip.h>


// Need to link with Ws2_32.lib, Mswsock.lib, and Advapi32.lib
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

#include "theService.h"
#include "helper_funcs.h"


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

#define SERVICE_NAME  _T("bgExtBIMALDE")

CBgHelperSrv::CBgHelperSrv()
{
	rLogger::init_logging();

	constexpr char sz_unique_named_mutex[] = "bghelpermutex";
	const HANDLE h_handle = CreateMutex(nullptr, TRUE, sz_unique_named_mutex);
	rLogger::LAST_ERROR = GetLastError();
	if (ERROR_ALREADY_EXISTS == rLogger::LAST_ERROR)
	{
		LOG_SAVE << "Program already running - exiting.";
		return;
	}

	const std::string app_data = get_env("appdata");
	const std::string ini_file = app_data + "\\alabuga_dev\\ifcexprt.inf";

	const CA2W inf_config_path(ini_file.c_str());
	tcp_port_ = GetPrivateProfileIntW(L"Manufacturer", L"tcp_port", 7777, inf_config_path);	

	/* Hide console window: */
	::ShowWindow(::GetConsoleWindow(), SW_HIDE);

	CBgHelperSrv::InitInstance();

	ReleaseMutex(h_handle); // Explicitly release mutex
	CloseHandle(h_handle);  // close handle before terminating
}

bool CBgHelperSrv::socket_connect()
{	
	auto wsa_res = WSAStartup(MAKEWORD(2, 0), &wsa_data_);
	
	int i_result;
	if(!m_server_socket)
	{
		m_server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		InetPton(AF_INET, "127.0.0.1", &addr_.sin_addr.s_addr);
		addr_.sin_family = AF_INET;
		addr_.sin_port = htons(tcp_port_);
	}

	const bool running = is_process_running(L"ifc_exporter.exe");
	if(!running)
	{
		int wtrue = 1;
		setsockopt(m_server_socket, SOL_SOCKET, SO_REUSEADDR, (char*)&wtrue, sizeof(int));
		shutdown(m_server_socket, 2);
		closesocket(m_server_socket);
		m_server_socket = 0;
		m_connected_to_qml = false;
		return false;
	}

	if(!m_connected_to_qml)
	{
		i_result = connect(m_server_socket, reinterpret_cast<SOCKADDR*>(&addr_), sizeof(addr_));
		if (i_result == SOCKET_ERROR)
		{
			int wtrue = 1;
			setsockopt(m_server_socket, SOL_SOCKET, SO_REUSEADDR, (char*)&wtrue, sizeof(int));
			shutdown(m_server_socket, 2);
			closesocket(m_server_socket);
			m_server_socket = 0;
			m_connected_to_qml = false;
			return false;
		}
	}
	m_connected_to_qml = true;
	
	return true;
}

void CBgHelperSrv::close_socket() const
{
	closesocket(m_server_socket);
	WSACleanup();
}

/////////////////////////////////////////////////////////////////////////////
// CBgHelperSrv initialization

BOOL CBgHelperSrv::InitInstance()
{
	// Standard initialization
#ifdef _AFXDLL
	// CWinApp::Enable3dControls is no longer needed.You should remove this call
	// Enable3dControls();			// Call this when using MFC in a shared DLL
#else
	Enable3dControlsStatic();	// Call this when linking to MFC statically
#endif
	inf_config_file_path_ = get_config_file_path(permanent_config);

	LOG_SAVE << "SocketConnect()...";
	socket_connect();	
	
	HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
	LOG_SAVE << "CoInitializeEx HRESULT: " << hr;

	/* Бесконечный цикл */
	active_object obj([this] { socket_connect(); std::this_thread::sleep_for(200ms); });

	/* Запущено как приложение */
	LPTSTR argvc = AfxGetApp()->m_lpCmdLine;
	if (__argc == 1)
	{
		try
		{
			std::mutex g_mutex;
			wstring name(L"\\\\.\\Pipe\\bghelperpipe");

			/* Иногда выдает ошибку 'CreateNamedPipeW error code 5 ("отказано в доступе?")  */
			WIN32_FIND_DATAW fd;
			HANDLE hwnd_pipe = FindFirstFileW(L"\\\\.\\Pipe\\bghelperpipe", &fd);
			if(hwnd_pipe)
				DisconnectNamedPipe(hwnd_pipe);				

			LOG_SAVE << "Starting pipeServer... ";
			CNamedPipeServer pipe_server(name,
				pipe_message_handler,
				&g_mutex,
				256,
				256,
				10);

			char choice = 's';
			do
			{						
				std::thread t(ini_timer_check, false, &m_started_status);
/*				active_object obj([] { iniTimer_check(); std::this_thread::sleep_for(200ms); });	*/
					
				if (choice == 'q') {
					LOG_SAVE << "Initiating shutdown";
					pipe_server.Shutdown();
					LOG_SAVE << "Waiting until everything is shutdown";
					pipe_server.WaitUntilFinished(INFINITE);
					LOG_SAVE << "Shutdown finished";
				}
				else if (choice == 's')
				{
					LOG_SAVE << "Starting pipeServer connections";
					pipe_server.StartServing();
					choice = 'i';				/* infinite */
				}

				t.join();
			} while (choice != 'q');			/* while (SIGABORT != "1"); */
		}
		catch (exception& ex)
		{
			LOG_SAVE << ex.what();
		}			
	}
	
	// Close the COM library 
	CoUninitialize();

	close_socket();

	LOG_SAVE << "InitInstance: Exit";

	// Since the dialog has been closed, return FALSE so that we exit the
	//  application, rather than start the application's message pump.
	return FALSE;
}
