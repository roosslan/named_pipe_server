#include "StdAfx.h"
#include <mutex>
#include "helper_funcs.h"
#include <comutil.h>
#include "theService.h"

#pragma comment(lib,"comsuppw.lib")

CUpdaterService extBIMALDEsvc;
HWND launchedRevitHWND;
DWORD launchedRevitProcessId;

void pipeMessageHandler(void* context, w32::CHandle& handle, CIOBuffer& input, CIOBuffer& output)
{
    std::mutex* pmutex = (std::mutex*)context;
    DWORD threadId = GetCurrentThreadId();
    
    char* buff = reinterpret_cast<char*>(input.Ptr());

    std::string logTextFrom_extBIMALDE_addin(buff);
    LOG_SAVE << "pipeMessageHandler: " << logTextFrom_extBIMALDE_addin;

    if (logTextFrom_extBIMALDE_addin == "Begin of export\n")    /* Сообщение от плагина */
    {
        LOG_SAVE << "m_startedStatus is set to false";
        extBIMALDEsvc.m_startedStatus = false;
    }
    if (logTextFrom_extBIMALDE_addin == "START_IMMEDIATELY") /* Сообщение от QML Exporter */
    {
        iniTimer_check(true, &extBIMALDEsvc.m_startedStatus);
        extBIMALDEsvc.m_startedStatus = false;
    }
    /* Дублируем из пайпа в сокет Qt для отладки */
    else if (extBIMALDEsvc.m_connectedToQML)
    {
        int charCount = 60;         /* split 60 chars */
        std::string s_buff(buff);
        for (size_t i = 0; i < strlen(buff); i += charCount)
        {
            std::string str = s_buff.substr(i, charCount);
            char* cstr = new char[str.size() + 1];
            std::strcpy(cstr, str.c_str());
            send(extBIMALDEsvc.m_server_socket, cstr, (int)strlen(cstr), 0);
            delete[] cstr;
        }            
    }

    if (logTextFrom_extBIMALDE_addin.rfind("End of export", 0) == 0)   /* Сообщение от плагина begins with, что экспорт завершен */
    {
        LOG_SAVE << "Kиляем Rевит";
        HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, launchedRevitProcessId);
        if (hProcess == NULL) {
            LOG_SAVE << "Failed to open the Revit's process with termination rights. Error: " << GetLastError();
        }
        else
        {
            BOOL result = TerminateProcess(hProcess, 0);

            if (result) {
                LOG_SAVE << "Revit terminated successfully";
            }
            else {
                LOG_SAVE << "Failed to terminate process. Error: " << GetLastError();
            }

            CloseHandle(hProcess);
        }
    }
}

std::string ReadINF_Flag(LPCWSTR keyName)
{
    std::string appData = getenv("appdata");
    std::string iniFile = appData + "\\alabuga_dev\\bimalde.inf";

    CA2W infConfigPath(iniFile.c_str());

    wchar_t wsValue[_MAX_FNAME] = L"";
    int rret = GetPrivateProfileStringW(L"ControlFlags", keyName, nullptr, wsValue, std::size(wsValue), infConfigPath);
    CW2A o_Value(wsValue);
    std::string retValue = o_Value;
    return retValue;
}

void iniTimer_check(bool startImmediately, bool* startedStatus)
{
    std::string appData = getenv("appdata");
    std::string iniFile = appData + "\\alabuga_dev\\bimalde.inf";

    CA2W infConfigPath(iniFile.c_str());
    wchar_t wsExportEnabled[_MAX_FNAME] = L"";

    int ret = GetPrivateProfileStringW(L"ControlFlags", L"Enabled", nullptr, wsExportEnabled, std::size(wsExportEnabled), infConfigPath);
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
        ss.clear();

        wchar_t wsDate[_MAX_FNAME] = L"";
        ret = GetPrivateProfileStringW(L"ControlFlags", L"Date", nullptr, wsDate, std::size(wsDate), infConfigPath);
        CW2A o_Date(wsDate);
        std::string isDate = o_Date;

        std::stringstream strStream;
        strStream << std::put_time(std::localtime(&in_time_t), "%d.%m.%Y");
        std::string s_dd_mm_yyyy = strStream.str();
        strStream.clear();

        if ( (s_hh_mm == isTime && s_dd_mm_yyyy == isDate) || startImmediately) 
        {
            if (!extBIMALDEsvc.m_connectedToQML)
                extBIMALDEsvc.SocketConnect();

            std::unique_lock<mutex> mu_lock(extBIMALDEsvc.mu);
            /*	Это должен писать плагин после завершения работы.  ret = WritePrivateProfileStringW(L"ControlFlags", L"Enabled", L"false", infConfigPath);		*/

            char* sendbuf = "Starting Revit process";
            if (extBIMALDEsvc.m_connectedToQML)
                send(extBIMALDEsvc.m_server_socket, sendbuf, (int)strlen(sendbuf), 0);

            /* Какую версию Revit запускать - берём из ComboBox'a ExportTo (из INF-файла) */
            std::string sRevitVersion = ReadINF_Flag(L"RevitVersion");
            std::string revitVersion = "C:\\Program Files\\Autodesk\\Revit " + sRevitVersion + "\\Revit.exe";

            if (!extBIMALDEsvc.m_startedStatus)
            {
                startRevitProccess(revitVersion.c_str());
                extBIMALDEsvc.m_startedStatus = true;
                LOG_SAVE << "Started Revit process: " << revitVersion;
            }
            mu_lock.unlock();
        }
    };

    std::this_thread::sleep_for(std::chrono::seconds(20));
}

const wchar_t* GetWC(const char* c)
{
    const size_t cSize = strlen(c) + 1;
    wchar_t* wc = new wchar_t[cSize];
    mbstowcs(wc, c, cSize);

    return wc;
}

bool IsProcessRunning(const wchar_t* processName)
{
    bool exists = false;
    PROCESSENTRY32 entry;
    entry.dwSize = sizeof(PROCESSENTRY32);

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);

    if (Process32First(snapshot, &entry))
        while (Process32Next(snapshot, &entry))
        {
            auto wcExe = GetWC(entry.szExeFile);
            if (!wcsicmp(wcExe, processName))
                exists = true;
            delete wcExe;
        }
    CloseHandle(snapshot);
    return exists;
}

std::string GetConfigFilePath(int ConfigFileType)
{
    char* appdata = getenv("APPDATA");
    std::string roamingDirectory;
    /* Convert the Windows path type to a C++ path */
    roamingDirectory = appdata;

    std:string fn_ini = "";
    if (ConfigFileType == PermanentConfig)
        fn_ini = "\\bimalde.inf";
    else if (ConfigFileType == TemporaryConfig)
        fn_ini = "\\bimalde.ini";

    return roamingDirectory + "\\alabuga_dev" + fn_ini;
}

struct ProcessWindowFinder
{
    DWORD m_targetProcessId;
    HWND m_foundWindow;
};

static BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam) {
    DWORD processId;
    GetWindowThreadProcessId(hwnd, &processId);

    auto* finder = reinterpret_cast<ProcessWindowFinder*>(lParam);

    // Check if this window belongs to our target process
    if (processId == finder->m_targetProcessId) {
        // Additional checks to ensure it's a main window
        if (IsWindowVisible(hwnd) && GetParent(hwnd) == NULL) {
            finder->m_foundWindow = hwnd;
            return FALSE; // Stop enumeration
        }
    }
    return TRUE; // Continue enumeration
}

HWND FindMainWindow(DWORD processId, int timeoutMs = 10000)
{
    ProcessWindowFinder finder;
    finder.m_targetProcessId = processId;
    finder.m_foundWindow = nullptr;

    auto startTime = std::chrono::steady_clock::now();

    while (finder.m_foundWindow == nullptr) {
        EnumWindows(EnumWindowsProc, reinterpret_cast<LPARAM>(&finder));

        if (finder.m_foundWindow != nullptr) {
            break;
        }

        // Check timeout
        auto currentTime = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            currentTime - startTime);

        if (elapsed.count() > timeoutMs) {
            std::cout << "Timeout: Could not find window for process " << processId << std::endl;
            break;
        }

        // Wait before trying again
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    return finder.m_foundWindow;
}

VOID startRevitProccess(LPCTSTR lpApplicationName)
{
    // additional information
    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    // set the size of the structures
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    // start the program up
    CreateProcess(lpApplicationName,   // the path
        NULL,           // Command line
        NULL,           // Process handle not inheritable
        NULL,           // Thread handle not inheritable
        FALSE,          // Set handle inheritance to FALSE
        0,              // No creation flags
        NULL,           // Use parent's environment block
        NULL,           // Use parent's starting directory 
        &si,            // Pointer to STARTUPINFO structure
        &pi             // Pointer to PROCESS_INFORMATION structure (removed extra parentheses)
    );
    // Close process and thread handles. 

    CloseHandle(pi.hThread);

    int timeoutMs = 10000;
    launchedRevitProcessId = pi.dwProcessId;
    launchedRevitHWND = FindMainWindow(pi.dwProcessId, timeoutMs);

    CloseHandle(pi.hProcess);
}