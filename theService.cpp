
#include "StdAfx.h"
#include <mutex>
#include <chrono>
#include "theService.h"
#include "srvWindow.h"
#include "helper_funcs.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

SERVICE_STATUS_HANDLE g_StatusHandle = NULL;
SERVICE_STATUS        g_ServiceStatus = { 0 };
HANDLE                g_ServiceStopEvent = INVALID_HANDLE_VALUE;

DWORD m_argc;
LPTSTR* m_argv;

VOID WINAPI ServiceMain(DWORD argc, LPTSTR* argv);
VOID WINAPI ServiceCtrlHandler(DWORD);
DWORD WINAPI ServiceWorkerThread(LPVOID lpParam);

#define SERVICE_NAME  _T("extBIMALDEsvc")

/////////////////////////////////////////////////////////////////////////////
// CUpdaterService

BEGIN_MESSAGE_MAP(CUpdaterService, CWinApp)
	//{{AFX_MSG_MAP(CUpdaterService)
	//}}AFX_MSG
	// ON_COMMAND(ID_HELP, CWinApp::OnHelp)	
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CUpdaterService construction

CUpdaterService::CUpdaterService() { }

/////////////////////////////////////////////////////////////////////////////
// The one and only CUpdaterService object

CUpdaterService extBIMALDEsvc;


void iniTimer_check() {
	std::this_thread::sleep_for(std::chrono::seconds(15));
//	MessageBoxW(0, L"hello from thread", L"std::endl", MB_OK);
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
	std::string infConfigFilePath = GetConfigFilePath(PermanentConfig);
	rLogger::InitLogging();
	LOG_SAVE << "InitInstance: Entry";

	HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
	//if (SUCCEEDED(hr))
	{
		// Create the main window and enter the message loop
		// RunGetImage();
		// Close the COM library 
		// CoUninitialize();
	}

	//ruWIADev = new CAddinUpdater();

	SERVICE_TABLE_ENTRY ServiceTable[] =
	{
		{SERVICE_NAME, (LPSERVICE_MAIN_FUNCTION)ServiceMain},
		{NULL, NULL}

	};

	if (StartServiceCtrlDispatcher(ServiceTable) == TRUE)
    {
		LOG_SAVE << "InitInstance: StartServiceCtrlDispatcher: Launched as service";

		{
			LOG_SAVE << "InitInstance: Register pEventCallback";

		}

		LOG_SAVE << GetLastError();
		// return FALSE;		
	}
	else
 {
		// «апущено как приложение
		LPTSTR argvc = AfxGetApp()->m_lpCmdLine;
		if (__argc == 3)
		{
			if ((strcmp(__argv[1], "set") == 0) && (strcmp(__argv[2], "up") == 0))
			{
				CSrvWindow rus1scanWindow;
				m_pMainWnd = &rus1scanWindow;
				//		rus1scanWindow.addinUpdater = ruWIADev;

				int nResponse = rus1scanWindow.DoModal();
				if (nResponse == IDOK)
				{
				}
				else if (nResponse == IDCANCEL)
				{
				}
			}

		}
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
					//AfxOutputDebugString("worken");
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
	}
	// Close the COM library 
	CoUninitialize();

	LOG_SAVE << "InitInstance: Exit";

	// Since the dialog has been closed, return FALSE so that we exit the
	//  application, rather than start the application's message pump.
	return FALSE;
}

VOID WINAPI ServiceMain(DWORD argc, LPTSTR* argv)
{
	m_argc = argc;
	m_argv = argv;
	DWORD Status = E_FAIL;

	LOG_SAVE << "ServiceMain: Entry";
	//CComPtr<WiaWrap::CEventCallback> srv_pEventCallback = new WiaWrap::CEventCallback;

	//HRESULT hr = CoInitialize(NULL);
	//

	//if (srv_pEventCallback != NULL)
	//{
	//	LOG_SAVE << "ServiceMain: Register pEventCallback";
	//	srv_pEventCallback->Register();
	//}
	//else LOG_SAVE << "ServiceMain: srv_pEventCallback is NULL";
	//IWiaDevMgr* RooWiaMgr;	
	//CAddinUpdater* RooWiaDev = new CAddinUpdater(NULL, &RooWiaMgr);

	g_StatusHandle = RegisterServiceCtrlHandler(SERVICE_NAME, ServiceCtrlHandler);

	if (g_StatusHandle == NULL)
	{
		LOG_SAVE << "ServiceMain: RegisterServiceCtrlHandler returned error";
		goto EXIT;
	}

	// Tell the service controller we are starting
	ZeroMemory(&g_ServiceStatus, sizeof(g_ServiceStatus));
	g_ServiceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
	g_ServiceStatus.dwControlsAccepted = 0;
	g_ServiceStatus.dwCurrentState = SERVICE_START_PENDING;
	g_ServiceStatus.dwWin32ExitCode = 0;
	g_ServiceStatus.dwServiceSpecificExitCode = 0;
	g_ServiceStatus.dwCheckPoint = 0;

	if (SetServiceStatus(g_StatusHandle, &g_ServiceStatus) == FALSE)
	{
		LOG_SAVE << "ServiceMain: SetServiceStatus returned error";
	}

	LOG_SAVE << "ServiceMain: Performing Service Start Operations";
	IID rGuid;
	LOG_SAVE << "ServiceMain:IIDFromString";
	//const CLSID = IID("{73856D9A - 2720 - 487A - A584 - 21D5774E9D0F}");
	HRESULT hR = IIDFromString(L"0000001B00000000C000000000000046", &rGuid);
	LOG_SAVE << "ServiceMain: QueryInterface";
	//HRESULT hhR = srv_pEventCallback->QueryInterface(rGuid, NULL);
	//LOG_SAVE << hhR;
	//::CoTaskMemFree(&rGuid);


	//HRESULT hr = CoInitialize(NULL);
	//if (m_pEventCallback != NULL)
	//{
	//    LOG_SAVE << "ServiceMain: Register pEventCallback";
	//    m_pEventCallback->Register();
	//}

	// Create stop event to wait on later.
	g_ServiceStopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (g_ServiceStopEvent == NULL)
	{
		LOG_SAVE << "ServiceMain: CreateEvent(g_ServiceStopEvent) returned error";

		g_ServiceStatus.dwControlsAccepted = 0;
		g_ServiceStatus.dwCurrentState = SERVICE_STOPPED;
		g_ServiceStatus.dwWin32ExitCode = GetLastError();
		g_ServiceStatus.dwCheckPoint = 1;

		if (SetServiceStatus(g_StatusHandle, &g_ServiceStatus) == FALSE)
		{
			LOG_SAVE << "ServiceMain: SetServiceStatus returned error";
		}
		goto EXIT;
	}

	// Tell the service controller we are started
	g_ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;
	g_ServiceStatus.dwCurrentState = SERVICE_RUNNING;
	g_ServiceStatus.dwWin32ExitCode = 0;
	g_ServiceStatus.dwCheckPoint = 0;

	if (SetServiceStatus(g_StatusHandle, &g_ServiceStatus) == FALSE)
	{
		LOG_SAVE << "ServiceMain: SetServiceStatus returned error";
	}

	// Start the thread that will perform the main task of the service
	HANDLE hThread = CreateThread(NULL, 0, ServiceWorkerThread, NULL, 0, NULL);

	LOG_SAVE << "ServiceMain: Waiting for Worker Thread to complete";

	// Wait until our worker thread exits effectively signaling that the service needs to stop
	WaitForSingleObject(hThread, INFINITE);

	LOG_SAVE << "ServiceMain: Worker Thread Stop Event signaled";

	
	// Perform any cleanup tasks
	 
	LOG_SAVE << "ServiceMain: Performing Cleanup Operations";

	CloseHandle(g_ServiceStopEvent);

	g_ServiceStatus.dwControlsAccepted = 0;
	g_ServiceStatus.dwCurrentState = SERVICE_STOPPED;
	g_ServiceStatus.dwWin32ExitCode = 0;
	g_ServiceStatus.dwCheckPoint = 3;

	if (SetServiceStatus(g_StatusHandle, &g_ServiceStatus) == FALSE)
	{
		LOG_SAVE << "ServiceMain: SetServiceStatus returned error";
	}

EXIT:
	LOG_SAVE << "ServiceMain: Exit";

	return;
}	



VOID WINAPI ServiceCtrlHandler(DWORD CtrlCode)
{
	LOG_SAVE << "ServiceCtrlHandler: Entry";

	switch (CtrlCode)
	{
	case SERVICE_CONTROL_STOP:

		LOG_SAVE << "ServiceCtrlHandler: SERVICE_CONTROL_STOP Request";

		if (g_ServiceStatus.dwCurrentState != SERVICE_RUNNING)
			break;

		/*
		 * Perform tasks neccesary to stop the service here
		 */

		g_ServiceStatus.dwControlsAccepted = 0;
		g_ServiceStatus.dwCurrentState = SERVICE_STOP_PENDING;
		g_ServiceStatus.dwWin32ExitCode = 0;
		g_ServiceStatus.dwCheckPoint = 4;

		if (SetServiceStatus(g_StatusHandle, &g_ServiceStatus) == FALSE)
		{
			LOG_SAVE << "ServiceCtrlHandler: SetServiceStatus returned error";
		}

		// This will signal the worker thread to start shutting down
		SetEvent(g_ServiceStopEvent);

		break;

	default:
		break;
	}

	LOG_SAVE << "ServiceCtrlHandler: Exit";
}


DWORD WINAPI ServiceWorkerThread(LPVOID lpParam)
{
	launchDebugger();
	LOG_SAVE << "ServiceWorkerThread: Entry";

	//HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
	//CComPtr<WiaWrap::CEventCallback>  srv_pEventCallback;
	//srv_pEventCallback = new WiaWrap::CEventCallback;

	//if (srv_pEventCallback != NULL)
	//{
	//	LOG_SAVE << "ServiceWorkerThread: Register pEventCallback";
	//	srv_pEventCallback->Register();
	//}
	//else LOG_SAVE << "ServiceWorkerThread: srv_pEventCallback is NULL";
	//LOG_SAVE << GetLastError();


	//  Periodically check if the service has been requested to stop
	while (WaitForSingleObject(g_ServiceStopEvent, 0) != WAIT_OBJECT_0)
	{
		//LOG_SAVE << "ServiceWorkerThread: WaitForSingleObject";
		Sleep(3000); // ... каждые 3 секунды
	}

	LOG_SAVE << "ServiceWorkerThread: Exit";
	// Close the COM library 
	CoUninitialize();
	return ERROR_SUCCESS;
}