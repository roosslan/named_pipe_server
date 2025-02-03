#include "StdAfx.h"

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

BEGIN_MESSAGE_MAP(CUpdaterService, CWinApp)
	//{{AFX_MSG_MAP(CUpdaterService)
	//}}AFX_MSG
	// ON_COMMAND(ID_HELP, CWinApp::OnHelp)	
END_MESSAGE_MAP()

#define SERVICE_NAME  _T("bgExtBIMALDE")


// Launched as a /SUBSYSTEM:CONSOLE - 
CUpdaterService::CUpdaterService()
{
	rLogger::InitLogging();

	const char szUniqueNamedMutex[] = "bghelpermutex";
	HANDLE hHandle = CreateMutex(NULL, TRUE, szUniqueNamedMutex);
	rLogger::LAST_ERROR = GetLastError();
	if (ERROR_ALREADY_EXISTS == rLogger::LAST_ERROR)
	{
		LOG_SAVE << "Program already running - exiting.";
		return;
	}

	std::string appData = getenv("appdata");
	std::string iniFile = appData + "\\alabuga_dev\\bimalde.inf";

	CA2W infConfigPath(iniFile.c_str());
	wchar_t wsWindowEnabled[_MAX_FNAME] = L"";

	int ret = GetPrivateProfileStringW(L"Version", L"ClassVer", nullptr, wsWindowEnabled, std::size(wsWindowEnabled), infConfigPath);
	CW2A o_WindowEnabled(wsWindowEnabled);
	std::string isWindowEnabled = o_WindowEnabled;

	/* Hide console window: */
	if (isWindowEnabled != "0")
		::ShowWindow(::GetConsoleWindow(), SW_HIDE);

	InitInstance();

	ReleaseMutex(hHandle); // Explicitly release mutex
	CloseHandle(hHandle); // close handle before terminating
}

bool CUpdaterService::SocketConnect()
{	
	auto wsaRes = WSAStartup(MAKEWORD(2, 0), &wsa_data);
	
	int iResult;
	if(!server_socket)
	{
		server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		InetPton(AF_INET, "127.0.0.1", &addr.sin_addr.s_addr);
		addr.sin_family = AF_INET;
		addr.sin_port = htons(6667);
	}

	bool running = IsProcessRunning(L"exportToWindow.exe");
	if(!running)
	{
		int wtrue = 1;
		setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, (char*)&wtrue, sizeof(int));
		shutdown(server_socket, 2);
		closesocket(server_socket);
		server_socket = 0;
		connectedToQML = false;
		return false;
	}

	if(!connectedToQML)
	{
		iResult = connect(server_socket, reinterpret_cast<SOCKADDR*>(&addr), sizeof(addr));
		if (iResult == SOCKET_ERROR)
		{
			int wtrue = 1;
			setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, (char*)&wtrue, sizeof(int));
			shutdown(server_socket, 2);
			closesocket(server_socket);
			server_socket = 0;
			connectedToQML = false;
			return false;
		}
	}
	connectedToQML = true;
	
	return true;
}

void CUpdaterService::CloseSocket()
{
	closesocket(server_socket);
	WSACleanup();
}



/////////////////////////////////////////////////////////////////////////////
// CUpdaterService initialization

BOOL CUpdaterService::InitInstance()
{
	// Standard initialization
#ifdef _AFXDLL
	// CWinApp::Enable3dControls is no longer needed.You should remove this call
	// Enable3dControls();			// Call this when using MFC in a shared DLL
#else
	Enable3dControlsStatic();	// Call this when linking to MFC statically
#endif
	infConfigFilePath = GetConfigFilePath(PermanentConfig);

	LOG_SAVE << "SocketConnect()...";
	SocketConnect();	
	
	HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
	LOG_SAVE << "CoInitializeEx HRESULT: " << hr;

	/* Бесконечный цикл */
	active_object obj([this] { SocketConnect(); std::this_thread::sleep_for(200ms); });

	// Запущено как приложение
	LPTSTR argvc = AfxGetApp()->m_lpCmdLine;
	if (__argc == 1)
	{
		try
		{
			std::mutex g_mutex;
			wstring name(L"\\\\.\\Pipe\\bghelperpipe");

			/* Иногда выдает ошибку 'CreateNamedPipeW error code 5 ("отказано в доступе?")  */
			WIN32_FIND_DATAW fd;
			HANDLE hwndPipe = FindFirstFileW(L"\\\\.\\Pipe\\bghelperpipe", &fd);
			if(hwndPipe)
				DisconnectNamedPipe(hwndPipe);				

			LOG_SAVE << "Starting pipeServer... ";
			CNamedPipeServer pipeServer(name,
				pipeMessageHandler,
				&g_mutex,
				256,
				256,
				10);

			char choice = 's';
			do
			{						
				std::thread t(iniTimer_check);
					
				if (choice == 'q') {
					LOG_SAVE << "Initiating shutdown";
					pipeServer.Shutdown();
					LOG_SAVE << "Waiting until everything is shutdown";
					pipeServer.WaitUntilFinished(INFINITE);
					LOG_SAVE << "Shutdown finished";
				}
				else if (choice == 's')
				{
					LOG_SAVE << "Starting pipeServer connections";
					pipeServer.StartServing();
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

	CloseSocket();

	LOG_SAVE << "InitInstance: Exit";

	// Since the dialog has been closed, return FALSE so that we exit the
	//  application, rather than start the application's message pump.
	return FALSE;
}
