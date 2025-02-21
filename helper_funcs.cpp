#include "StdAfx.h"
#include <mutex>
#include "helper_funcs.h"
#include <comutil.h>
#include "theService.h"

#pragma comment(lib,"comsuppw.lib")

CUpdaterService extBIMALDEsvc;

void pipeMessageHandler(void* context, w32::CHandle& handle, CIOBuffer& input, CIOBuffer& output)
{
    std::mutex* pmutex = (std::mutex*)context;
    DWORD threadId = GetCurrentThreadId();
    
    char* buff = reinterpret_cast<char*>(input.Ptr());

    std::string logTextFrom_extBIMALDE_addin(buff);
    LOG_SAVE << "pipeMessageHandler: " << logTextFrom_extBIMALDE_addin;

    if (logTextFrom_extBIMALDE_addin == "START_IMMEDIATELY") /* Сообщение от QML Exporter */
        iniTimer_check();
    /* Дублируем из пайпа в сокет Qt для отладки */
    else if (extBIMALDEsvc.connectedToQML)
    {
        int charCount = 60;         /* split 60 chars */
        std::string s_buff(buff);
        for (size_t i = 0; i < strlen(buff); i += charCount)
        {
            std::string str = s_buff.substr(i, charCount);
            char* cstr = new char[str.size() + 1];
            std::strcpy(cstr, str.c_str());
            send(extBIMALDEsvc.server_socket, cstr, (int)strlen(cstr), 0);
            delete[] cstr;
        }            
    }
    /*
    send(extBIMALDEsvc.server_socket, buff, (int)strlen(buff), 0);
    LPWSTR text = (LPWSTR)input.Ptr();
    {
        lock_guard< std::mutex> lock(*pmutex);
        // std::wcout << L"Thread " << threadId << L" recieved: " << text << std::endl;
    }

    wstringstream stream;
    stream << L"Message answered by thread " << threadId;
    wstring message = stream.str();
    DWORD bytes = (message.size() + 1) * sizeof(wstring::value_type);
    memcpy(
        output.Ptr(),
        (void*)message.c_str(),
        bytes);
    output.SetOffset(bytes);        
    */
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
            if (!extBIMALDEsvc.connectedToQML)
                extBIMALDEsvc.SocketConnect();

            std::unique_lock<mutex> mu_lock(extBIMALDEsvc.mu);
            /*				ret = WritePrivateProfileStringW(L"ControlFlags", L"runNow", L"false", infConfigPath);		*/

            char* sendbuf = "Starting Revit process";
            if (extBIMALDEsvc.connectedToQML)
                send(extBIMALDEsvc.server_socket, sendbuf, (int)strlen(sendbuf), 0);

            /* Какую версию Revit запускать - берём из ComboBox'a ExportTo (из INF-файла) */
            std::string sRevitVersion = ReadINF_Flag(L"RevitVersion");
            std::string revitVersion = "C:\\Program Files\\Autodesk\\Revit " + sRevitVersion + "\\Revit.exe";

            startRevitProccess(revitVersion.c_str());
            LOG_SAVE << "Started Revit process: " << revitVersion;
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
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
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
