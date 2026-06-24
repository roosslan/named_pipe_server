/* last change 27.5.2026, removed 60-chars dividing */

#include "stdafx.h"

#include "helper_funcs.h"
#include "theService.h"
#include "sensitive_data.h"

#include <comutil.h>

#pragma comment(lib,"comsuppw.lib")

CBgHelperSrv bg_service;
HWND launched_revit_hwnd;
DWORD launched_revit_process_id;

void pipe_message_handler(void* context, w32::CHandle& handle, CIOBuffer& input, CIOBuffer& output) {
    auto pmutex = (std::mutex*)context;
    DWORD thread_id = GetCurrentThreadId();

    const auto buff = reinterpret_cast<char*>(input.Ptr());

    std::string log_text_from_revit_addin(buff);
    LOG_SAVE << "pipeMessageHandler: " << log_text_from_revit_addin;

    /* Сообщение от плагина */
    if (log_text_from_revit_addin.starts_with("Begin of export")) {
        LOG_SAVE << "m_started_status is set to false";
/*
        const std::string s_debug("m_started_status is set to false\n");
        const auto cs_debug = new char[s_debug.size() + 1];
        std::strcpy(cs_debug, s_debug.c_str());
        send(bg_service.m_server_socket, cs_debug, (int)strlen(cs_debug), 0);
        delete[] cs_debug;
*/
        bg_service.m_started_status = false;

    }

    /* Сообщение от QML Exporter */
    if (log_text_from_revit_addin == "START_IMMEDIATELY") {

        /* первый параметр start_immediately = true: */
        ini_timer_check(true, &bg_service.m_started_status);
        bg_service.m_started_status = false;
    }
    /* Дублируем из пайпа в сокет Qt для отладки */
    else if (bg_service.m_connected_to_qml)
    {
        send(bg_service.m_server_socket, buff, (int)strlen(buff), 0);

	     /* split 60 chars */
/*      constexpr int char_count = 60;
 *      const std::string s_buff(buff);
 *      for (size_t i = 0; i < strlen(buff); i += char_count)
 *      {
 *          std::string str = s_buff.substr(i, char_count);
 *          const auto cstr = new char[str.size() + 1];
 *          std::strcpy(cstr, str.c_str());
 *          send(bg_service.m_server_socket, cstr, (int)strlen(cstr), 0);
 *          delete[] cstr;
 *      }
 */

    }

    if (log_text_from_revit_addin.rfind("End of export", 0) == 0)  /* Сообщение от плагина begins with, что экспорт завершен */
    {
        LOG_SAVE << "Kиляем Rевит";
        if (const HANDLE h_process = OpenProcess(PROCESS_TERMINATE, FALSE, launched_revit_process_id); h_process == nullptr) {
            LOG_SAVE << "Failed to open the Revit's process with termination rights. Error: " << GetLastError();
        }
        else
        {
	        if (TerminateProcess(h_process, 0)) {
                LOG_SAVE << "Revit terminated successfully";
            }
            else {
                LOG_SAVE << "Failed to terminate process. Error: " << GetLastError();
            }

            CloseHandle(h_process);
        }
    }
}

std::string read_inf_flag(const LPCWSTR key_name)
{
    const CA2W inf_config_path(get_config_file_path().c_str());

    wchar_t ws_value[_MAX_FNAME] = L"";
    GetPrivateProfileStringW(L"ControlFlags", key_name, nullptr, ws_value, std::size(ws_value), inf_config_path);
    const CW2A o_value(ws_value);
    return std::string(CW2A(ws_value));
}

std::wstring expand_environment_variables(const std::wstring& input) {
    const DWORD size = ExpandEnvironmentStringsW(input.c_str(), nullptr, 0);
    if (size == 0) return input;

    std::vector<wchar_t> buffer(size);
    ExpandEnvironmentStringsW(input.c_str(), buffer.data(), size);
    return std::wstring(buffer.data());
}

std::string get_env(const std::string& env_var) {
    std::string rret;
    char* buf = nullptr;
    size_t sz = 0;
    if (_dupenv_s(&buf, &sz, env_var.c_str()) == 0 && buf != nullptr)
    {
        rret = buf;
        free(buf);
    }
    return rret;
}
std::string get_host_name() {
    char hostname[256];

    WSADATA wsa_data;
    if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) return "Error: WSAStartup failed";

    if (gethostname(hostname, sizeof(hostname)) == 0) {
        std::string name(hostname);
        WSACleanup();

        return name;
    }
}

bool is_network_file_exists() {
    /* для проверки доступности сетевого файла -
     * имя, например L:\99_IT\00_ifc_export\pc-c421-173.sav */
    try {
        bool debug_sav_exists = std::filesystem::exists("L:\\99_IT\\00_ifc_export\\" + get_host_name() + ".sav");
        return debug_sav_exists;
    }
    catch (...) {
        /* Если сеть отвалилась в момент проверки, fs::exists может бросить исключение */
        return false;
    }
}

void ini_timer_check(bool start_immediately, bool* started_status) {
    /* Проверяем сетевой диск с файлом .sav - доступен (+для перемещения) или нет,
     * если да, перемещаем его в views_sites.sav и стартуем экспорт   */
    std::future<bool> file_check_future = std::async(std::launch::async, is_network_file_exists);
    bool async_result_ready = false;
    bool sav_file_exists = false;

    /* Неблокирующий цикл */
    while (!async_result_ready) {
        /* timeout 0 - вернуть сразу */
        auto status = file_check_future.wait_for(std::chrono::milliseconds(0));

        if (status == std::future_status::ready) {
            sav_file_exists = file_check_future.get();

            /* Начинаем удаленный экспорт */
            async_result_ready = true;
        }
        else {
            /* Работаем, в обычном режиме, с локальным .sav-файлом */

            std::this_thread::sleep_for(std::chrono::milliseconds(100));

            CA2W inf_config_path(get_config_file_path().c_str());
            wchar_t ws_export_enabled[_MAX_FNAME] = L"";

            GetPrivateProfileStringW(L"ControlFlags", L"Enabled", nullptr, ws_export_enabled, std::size(ws_export_enabled), inf_config_path);
            std::string is_export_enabled{ CW2A(ws_export_enabled) };

            if (is_export_enabled == "true")
            {
                wchar_t ws_time[_MAX_FNAME] = L"";
                GetPrivateProfileStringW(L"ControlFlags", L"Time", nullptr, ws_time, std::size(ws_time), inf_config_path);
                std::string is_time{ CW2A(ws_time) };

                auto now = std::chrono::system_clock::now();
                auto in_time_t = std::chrono::system_clock::to_time_t(now);
                std::stringstream ss;
                ss << std::put_time(std::localtime(&in_time_t), "%H:%M");
                auto s_hh_mm = ss.str();
                ss.clear();

                wchar_t ws_date[_MAX_FNAME] = L"";
                GetPrivateProfileStringW(L"ControlFlags", L"Date", nullptr, ws_date, std::size(ws_date), inf_config_path);
                std::string is_date{ CW2A(ws_date) };

                std::stringstream str_stream;
                str_stream << std::put_time(std::localtime(&in_time_t), "%d.%m.%Y");
                const std::string s_dd_mm_yyyy = str_stream.str();
                str_stream.clear();

                if ((s_hh_mm == is_time && s_dd_mm_yyyy == is_date) || start_immediately)
                {
                    if (!bg_service.m_connected_to_qml)
                        bg_service.socket_connect();

                    std::unique_lock<mutex> mu_lock(bg_service.mu);
                    /*	Это должен писать плагин после завершения работы.  ret = WritePrivateProfileStringW(L"ControlFlags", L"Enabled", L"false", infConfigPath);		*/

                    const char* send_buf = "Starting Revit process";
                    if (bg_service.m_connected_to_qml)
                        send(bg_service.m_server_socket, send_buf, (int)strlen(send_buf), 0);

                    /* Какую версию Revit запускать - берём из ComboBox'a ifc_exporter'a (из INF-файла) */
                    const std::string s_revit_version = read_inf_flag(L"RevitVersion");
                    const std::string revit_version = "C:\\Program Files\\Autodesk\\Revit " + s_revit_version + "\\Revit.exe";

                    if (!bg_service.m_started_status)
                    {
                        start_revit_process(revit_version.c_str());
                        bg_service.m_started_status = true;
                        std::string started_record = "Started Revit " + s_revit_version + ", pid " + std::to_string(launched_revit_process_id);
                        LOG_SAVE << started_record;

                        send_buf = started_record.c_str();
                        if (bg_service.m_connected_to_qml)
                            send(bg_service.m_server_socket, send_buf, (int)strlen(send_buf), 0);
                    }
                    mu_lock.unlock();
                }
            }
            std::this_thread::sleep_for(std::chrono::seconds(20));
        }
    }
    LOG_SAVE << "debug: " << (sav_file_exists ? "Found" : "Not found\n");
}


const wchar_t* get_wc(const char* c) {
    const size_t c_size = strlen(c) + 1;
    const auto wc = new wchar_t[c_size];
    mbstowcs(wc, c, c_size);

    return wc;
}

bool is_process_running(const wchar_t* process_name) {
    bool exists = false;
    PROCESSENTRY32 entry;
    entry.dwSize = sizeof(PROCESSENTRY32);

    const HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);

    if (Process32First(snapshot, &entry))
        while (Process32Next(snapshot, &entry))
        {
            const auto wc_exe = get_wc(entry.szExeFile);
            if (!wcsicmp(wc_exe, process_name))
                exists = true;
            delete wc_exe;
        }
    CloseHandle(snapshot);
    return exists;
}

std::string get_config_file_path()
{
    std::string roaming_directory = get_env("APPDATA");

    return roaming_directory + inf_file_path;
}

struct process_window_finder
{
    DWORD m_target_process_id;
    HWND m_found_window;
};
static BOOL CALLBACK enum_windows_proc(const HWND hwnd, const LPARAM lParam) {
    DWORD process_id;
    GetWindowThreadProcessId(hwnd, &process_id);

    auto* finder = reinterpret_cast<process_window_finder*>(lParam);

    // Check if this window belongs to our target process
    if (process_id == finder->m_target_process_id) {
        // Additional checks to ensure it's a main window
        if (IsWindowVisible(hwnd) && GetParent(hwnd) == NULL) {
            finder->m_found_window = hwnd;
            return FALSE; // Stop enumeration
        }
    }
    return TRUE; // Continue enumeration
}

HWND find_main_window(const DWORD process_id, const int timeout_ms = 10000) {
    process_window_finder finder;
    finder.m_target_process_id = process_id;
    finder.m_found_window = nullptr;

    const auto start_time = std::chrono::steady_clock::now();

    while (finder.m_found_window == nullptr) {
        EnumWindows(enum_windows_proc, reinterpret_cast<LPARAM>(&finder));

        if (finder.m_found_window != nullptr) {
            break;
        }

        // Check timeout
        auto current_time = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            current_time - start_time);

        if (elapsed.count() > timeout_ms) {
            std::cout << "Timeout: Could not find window for process " << process_id << std::endl;
            break;
        }

        // Wait before trying again
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    return finder.m_found_window;
}

VOID start_revit_process(const LPCTSTR fpath) {
    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    // set the size of the structures
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    CreateProcess(fpath,             // the path
                  nullptr,           // Command line
                  nullptr,           // Process handle not inheritable
                  nullptr,           // Thread handle not inheritable
				  FALSE,             // Set handle inheritance to FALSE
				  0,                 // No creation flags
                  nullptr,           // Use parent's environment block
                  nullptr,           // Use parent's starting directory 
				  &si,               // Pointer to STARTUPINFO structure
				  &pi                // Pointer to PROCESS_INFORMATION structure (removed extra parentheses)
    );

    CloseHandle(pi.hThread);

    constexpr int timeout_ms = 10000;
    launched_revit_process_id = pi.dwProcessId;
    launched_revit_hwnd = find_main_window(pi.dwProcessId, timeout_ms);

    CloseHandle(pi.hProcess);
}