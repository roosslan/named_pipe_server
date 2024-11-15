#include "StdAfx.h"

#include <mutex>
#include <chrono>

#include <winsock2.h>
#include <ws2tcpip.h>


// Need to link with Ws2_32.lib, Mswsock.lib, and Advapi32.lib
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")


std::mutex mu;

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



/////////////////////////////////////////////////////////////////////////////
// CUpdaterService construction

CUpdaterService::CUpdaterService() { }

CUpdaterService extBIMALDEsvc;

bool CUpdaterService::SocketConnect()
{
	string buffer = "bgHelper";
	auto wsaRes = WSAStartup(MAKEWORD(2, 0), &wsa_data);
	const auto server = socket(AF_INET, SOCK_STREAM, 0);

	InetPton(AF_INET, "127.1", &addr.sin_addr.s_addr);
	int iResult;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(6667);
	iResult = connect(server, reinterpret_cast<SOCKADDR*>(&addr), sizeof(addr));

	if (iResult == SOCKET_ERROR) {
		closesocket(server);
		LOG_SAVE << "Socketerr:" << WSAGetLastError();
		return false;
	}

	send(server, buffer.c_str(), buffer.length(), 0);
	closesocket(server);
	WSACleanup();

	return true;
}

void iniTimer_check()
{
	std::string appData = getenv("appdata");
	std::string iniFile = appData + "\\alabuga_dev\\bimalde.inf";

	CA2W infConfigPath(iniFile.c_str());
	wchar_t wsExportEnabled[_MAX_FNAME] = L"";	
	
	int ret = GetPrivateProfileStringW(L"ControlFlags", L"runNow", nullptr, wsExportEnabled, std::size(wsExportEnabled), infConfigPath);
	CW2A o_ExportEnabled(wsExportEnabled);
	std::string isExportEnabled = o_ExportEnabled;

	if (isExportEnabled == "true")
	{
		wchar_t wsTime[_MAX_FNAME] = L"";
		ret = GetPrivateProfileStringW(L"ControlFlags", L"Time", nullptr, wsTime, std::size(wsTime), infConfigPath);		
		CW2A o_Time(wsTime);
		std::string isTime = o_Time;

		auto now = std::chrono::system_clock::now();
		auto in_time_t = std::chrono::system_clock::to_time_t(now);
		std::stringstream ss;
		ss << std::put_time(std::localtime(&in_time_t), "%H:%M");
		auto s_hh_mm = ss.str();

		if (s_hh_mm == isTime)
		{
			std::unique_lock<mutex> mu_lock(mu);				
				ret = WritePrivateProfileStringW(L"ControlFlags", L"runNow", L"false", infConfigPath);
				LPCTSTR revitEXE = "C:\\Program Files\\Autodesk\\Revit 2023\\Revit.exe";		
				startRevitProccess(revitEXE);
				LOG_SAVE << "Started Revit process: " << revitEXE;
			mu_lock.unlock();
		}
	};
		
	std::this_thread::sleep_for(std::chrono::seconds(25));
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
						std::cout << "Starting server connections" << endl;
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

	LOG_SAVE << "InitInstance: Exit";

	// Since the dialog has been closed, return FALSE so that we exit the
	//  application, rather than start the application's message pump.
	return FALSE;
}
