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

bool launchDebugger()
{
    // Get System directory, typically c:\windows\system32
    std::wstring systemDir(MAX_PATH + 1, '\0');
    UINT nChars = GetSystemDirectoryW(&systemDir[0], systemDir.length());
    if (nChars == 0) return false; // failed to get system directory
    systemDir.resize(nChars);

    // Get process ID and create the command line
    DWORD pid = GetCurrentProcessId();
    std::wostringstream s;
    s << systemDir << L"\\vsjitdebugger.exe -p " << pid;
    std::wstring cmdLine = s.str();

    // Start debugger process
    STARTUPINFOW si;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);

    PROCESS_INFORMATION pi;
    ZeroMemory(&pi, sizeof(pi));

    if (!CreateProcessW(NULL, &cmdLine[0], NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) return false;

    // Close debugger process handles to eliminate resource leak
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);

    // Wait for the debugger to attach
    while (!IsDebuggerPresent()) Sleep(100);

    // Stop execution so the debugger can take over
    DebugBreak();
    return true;
}

std::string bstr_to_u32(BSTR source) {
    _bstr_t wrapped_bstr = _bstr_t(source);
    int length = wrapped_bstr.length();
    char* char_array = new char[length];
    strcpy_s(char_array, length + 1, wrapped_bstr);
    return char_array;
}

std::string GetConfigFilePath(int ConfigFileType)
{
    char* appdata = getenv("APPDATA");
    std::string roamingDirectory;
    /* Convert the Windows path type to a C++ path */
    roamingDirectory = appdata;

    std:string fn_ini = "";
    if (ConfigFileType == PermanentConfig)
        fn_ini = "\\exports.inf";
    else if (ConfigFileType == TemporaryConfig)
        fn_ini = "\\exports.ini";

    return roamingDirectory + "\\alabuga_dev" + fn_ini;
}

bool SaveActionToFile(std::string actionToSave)
{
    std::string actionToLaunch;
    std::string actionForIni;

    actionToLaunch = actionToSave;
    actionForIni = actionToLaunch;
    actionForIni = std::regex_replace(actionForIni, std::regex("^ +| +$"), "$1"); // removing leading and trailing spaces

    std::ofstream of;
    of.open(GetConfigFilePath(TemporaryConfig), std::ofstream::out | std::ofstream::app);

    of << actionForIni << "\n";

    of.flush();
    of.close();

    try {
        std::filesystem::remove(GetConfigFilePath(PermanentConfig));
        std::filesystem::rename(GetConfigFilePath(TemporaryConfig),
            GetConfigFilePath(PermanentConfig));
    }
    catch (std::filesystem::filesystem_error& e) {
        std::cout << e.what() << '\n';
    }
    return true;
}

std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> conv_utf8_utf32;
// convert a BSTR to a std::string. 
std::string& BstrToStdString(const BSTR bstr, std::string& dst)
{
    if (!bstr)
    {
        // define NULL functionality. I just clear the target.
        dst.clear();
        return dst;
    }

    // request content length in single-chars through a terminating
    //  nullchar in the BSTR. note: BSTR's support imbedded nullchars,
    //  so this will only convert through the first nullchar.
    int res = WideCharToMultiByte(CP_UTF8, 0, bstr, -1, NULL, 0, NULL, NULL);
    if (res > 0)
    {
        dst.resize(res);
        WideCharToMultiByte(CP_UTF8, 0, bstr, -1, &dst[0], res, NULL, NULL);
    }
    else
    {    // no content. clear target
        dst.clear();
    }

    return dst;
}

// conversion with temp.
std::string BstrToStdString(BSTR bstr)
{
    std::string str;
    BstrToStdString(bstr, str);
    return str;
}
