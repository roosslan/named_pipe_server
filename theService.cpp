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
}

bool CUpdaterService::SocketConnect()
{	
	auto wsaRes = WSAStartup(MAKEWORD(2, 0), &wsa_data);
	//const auto server = socket(AF_INET, SOCK_STREAM, 0);
	server_socket = socket(AF_INET, SOCK_STREAM, 0);
	InetPton(AF_INET, "127.0.0.1", &addr.sin_addr.s_addr);
	int iResult;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(6667);
	iResult = connect(server_socket, reinterpret_cast<SOCKADDR*>(&addr), sizeof(addr));

	if (iResult == SOCKET_ERROR) {
		closesocket(server_socket);
		//LOG_SAVE << "Socketerr: " << WSAGetLastError();
		return false;
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
	rLogger::InitLogging();

	SocketConnect();
	

	HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
	
		// Запущено как приложение
		LPTSTR argvc = AfxGetApp()->m_lpCmdLine;
		if (__argc == 1)
		{
			try
			{
				std::mutex g_mutex;
				wstring name(L"\\\\.\\Pipe\\ifcexporter");

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
						std::cout << "Initiating shutdown" << endl;
						pipeServer.Shutdown();
						std::cout << "Waiting until everything is shutdown" << endl;
						pipeServer.WaitUntilFinished(INFINITE);
						std::cout << "Shutdown finished" << endl;
					}
					else if (choice == 's')
					{
						std::cout << "Starting pipeServer connections" << endl;
						pipeServer.StartServing();
						choice = 'i';				/* infinite */
					}

					t.join();
				} while (choice != 'q');			/* while (SIGABORT != "1"); */
			}
			catch (exception& ex)
			{
				std::cout << ex.what() << endl;
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
