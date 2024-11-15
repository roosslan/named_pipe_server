#include "StdAfx.h"
#include <mutex>
#include "helper_funcs.h"
#include <comutil.h>
#pragma comment(lib,"comsuppw.lib")

void pipeMessageHandler(
    void* context,
    w32::CHandle& handle,
    CIOBuffer& input,
    CIOBuffer& output)
{
    std::mutex* pmutex = (std::mutex*)context;
    DWORD threadId = GetCurrentThreadId();
    
    std::string logTextFrom_extBIMALDE_addin(reinterpret_cast<char*>(input.Ptr()));
    LOG_SAVE << logTextFrom_extBIMALDE_addin;
    /*
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
