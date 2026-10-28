/* last change 16.10.2026 */

#include "stdafx.h"

#include "helper_funcs.h"
#include "the_service.h"
#include "sensitive_data.h"

#include <algorithm>
#include <cctype>
#include <iomanip>
#include <comutil.h>

#pragma comment(lib,"comsuppw.lib")

namespace
{
    /* Каталог по умолчанию для .sav-файлов удалённого экспорта, переопределяется
     * ключом [ControlFlags] RemoteSavDir в INI-файле */
    constexpr char default_remote_sav_dir[] = "L:\\99_IT\\00_ifc_export\\";

    /* Состояние запущенного Revit, защищено revit_mu */
    std::mutex revit_mu;
    HANDLE revit_process = nullptr;
    DWORD revit_process_id = 0;

    std::string get_host_name() {
        char hostname[256] = "";
        DWORD size = static_cast<DWORD>(std::size(hostname));
        if (!GetComputerNameExA(ComputerNameDnsHostname, hostname, &size))
            return {};
        return hostname;
    }

    std::string format_now(const char* format) {
        const auto in_time_t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        std::tm local_tm{};
        localtime_s(&local_tm, &in_time_t);
        std::ostringstream ss;
        ss << std::put_time(&local_tm, format);
        return ss.str();
    }

    /* Версия Revit из INI подставляется в путь к исполняемому файлу,
     * поэтому допускаются только 4 цифры (2024, 2025, ...) */
    bool is_valid_revit_version(const std::string& version) {
        return version.size() == 4 && std::all_of(version.begin(), version.end(),
            [](const unsigned char c) { return std::isdigit(c) != 0; });
    }

    std::string get_program_files_dir() {
        for (const char* var : { "ProgramW6432", "ProgramFiles" }) {
            if (std::string dir = get_env(var); !dir.empty())
                return dir;
        }
        return "C:\\Program Files";
    }

    std::string read_inf_string(const LPCWSTR section, const LPCWSTR key_name, const LPCWSTR default_value) {
        const CA2W inf_config_path(get_config_file_path().c_str());

        wchar_t ws_value[_MAX_PATH] = L"";
        GetPrivateProfileStringW(section, key_name, default_value, ws_value, static_cast<DWORD>(std::size(ws_value)), inf_config_path);
        return std::string(CW2A(ws_value));
    }
}

void pipe_message_handler(void* /*context*/, w32::CHandle& /*handle*/, CIOBuffer& input, CIOBuffer& /*output*/) {
    /* Сообщение не обязано заканчиваться '\0': длину берём из буфера,
     * а всё после первого '\0' (если клиент его передал) отбрасываем */
    std::string message(static_cast<const char*>(input.Ptr()), input.Offset());
    if (const auto end = message.find('\0'); end != std::string::npos)
        message.resize(end);

    LOG_SAVE("pipeMessageHandler: {}", message);

    /* Сообщение от плагина */
    if (message.starts_with("Begin of export")) {
        LOG_SAVE("Export has begun");
    }

    /* Сообщение от QML Exporter. Обрабатывается в основном цикле, чтобы не занимать
     * рабочий поток канала на время запуска Revit */
    if (message == "START_IMMEDIATELY") {
        bg_service.request_start_immediately();
    }
    /* Дублируем из пайпа в сокет Qt для отладки */
    else {
        bg_service.socket_send(message);
    }

    if (message.starts_with("End of export"))  /* Сообщение от плагина, что экспорт завершен */
    {
        LOG_SAVE("Kиляем Rевит");
        terminate_revit_process();
    }
}

std::string read_inf_flag(const LPCWSTR key_name)
{
    return read_inf_string(L"ControlFlags", key_name, nullptr);
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

bool is_network_file_exists(const std::string& remote_sav_dir) {
    /* для проверки доступности сетевого файла -
     * имя, например L:\99_IT\00_ifc_export\pc-c421-173.sav */
    const std::string host_name = get_host_name();
    if (host_name.empty())
        return false;

    std::error_code ec;
    /* Если сеть отвалилась в момент проверки, вернётся false с кодом ошибки в ec */
    return std::filesystem::exists(remote_sav_dir + host_name + ".sav", ec);
}

void remote_sav_checker::poll() {
    if (check_.valid()) {
        /* Сетевой диск может отвечать долго - не ждём, проверим на следующем вызове */
        if (check_.wait_for(std::chrono::milliseconds(0)) != std::future_status::ready)
            return;

        const int state = check_.get() ? 1 : 0;
        if (state != last_state_) {
            LOG_SAVE("Remote .sav file: {}", state ? "Found" : "Not found");
            last_state_ = state;
        }
    }

    const std::string remote_sav_dir = read_inf_string(L"ControlFlags", L"RemoteSavDir", CA2W(default_remote_sav_dir));
    check_ = std::async(std::launch::async, is_network_file_exists, remote_sav_dir);
}

void ini_timer_check(const bool start_immediately) {
    /* Последний обработанный слот расписания "дата время": в течение минуты
     * проверка срабатывает несколько раз, а запускать Revit нужно один раз.
     * Функция вызывается только из основного цикла, поэтому static без синхронизации */
    static std::string last_scheduled_slot;

    if (read_inf_flag(L"Enabled") != "true") {
        if (start_immediately)
            LOG_SAVE("START_IMMEDIATELY ignored: export is disabled in INI");
        return;
    }

    const std::string inf_time = read_inf_flag(L"Time");
    const std::string inf_date = read_inf_flag(L"Date");
    const std::string slot = inf_date + " " + inf_time;

    const bool scheduled = format_now("%H:%M") == inf_time
        && format_now("%d.%m.%Y") == inf_date
        && slot != last_scheduled_slot;

    if (!scheduled && !start_immediately)
        return;

    if (scheduled)
        last_scheduled_slot = slot;

    if (!bg_service.m_connected_to_qml)
        bg_service.socket_connect();

    std::lock_guard lock(bg_service.mu);
    /*	Это должен писать плагин после завершения работы.  ret = WritePrivateProfileStringW(L"ControlFlags", L"Enabled", L"false", infConfigPath);		*/

    if (const DWORD running_pid = running_revit_pid(); running_pid != 0) {
        LOG_SAVE("Revit launched earlier is still running (pid {}) - not starting another one", running_pid);
        return;
    }

    bg_service.socket_send("Starting Revit process");

    /* Какую версию Revit запускать - берём из ComboBox'a ifc_exporter'a (из INF-файла) */
    const std::string s_revit_version = read_inf_flag(L"RevitVersion");
    if (!is_valid_revit_version(s_revit_version)) {
        LOG_SAVE("Invalid RevitVersion in INI: '{}'", s_revit_version);
        return;
    }
    const std::string revit_path = get_program_files_dir() + "\\Autodesk\\Revit " + s_revit_version + "\\Revit.exe";

    const DWORD started_pid = start_revit_process(revit_path);
    if (started_pid == 0) {
        const std::string failed_record = "Failed to start " + revit_path;
        bg_service.socket_send(failed_record);
        return;
    }

    const std::string started_record = "Started Revit " + s_revit_version + ", pid " + std::to_string(started_pid);
    LOG_SAVE("{}", started_record);
    bg_service.socket_send(started_record);
}

bool is_process_running(const wchar_t* process_name) {
    const HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE)
        return false;

    bool exists = false;
    PROCESSENTRY32W entry{};
    entry.dwSize = sizeof(entry);

    for (BOOL ok = Process32FirstW(snapshot, &entry); ok && !exists; ok = Process32NextW(snapshot, &entry))
        exists = _wcsicmp(entry.szExeFile, process_name) == 0;

    CloseHandle(snapshot);
    return exists;
}

std::string get_config_file_path()
{
    std::string roaming_directory = get_env("APPDATA");

    return roaming_directory + inf_file_path;
}

DWORD start_revit_process(const std::string& revit_exe_path) {
    STARTUPINFOA si{};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi{};

    if (!CreateProcessA(revit_exe_path.c_str(), // the path
                        nullptr,                // Command line
                        nullptr,                // Process handle not inheritable
                        nullptr,                // Thread handle not inheritable
                        FALSE,                  // Set handle inheritance to FALSE
                        0,                      // No creation flags
                        nullptr,                // Use parent's environment block
                        nullptr,                // Use parent's starting directory
                        &si,                    // Pointer to STARTUPINFO structure
                        &pi))                   // Pointer to PROCESS_INFORMATION structure
    {
        LOG_SAVE("CreateProcess failed for {}, error {}", revit_exe_path, GetLastError());
        return 0;
    }

    CloseHandle(pi.hThread);

    std::lock_guard lock(revit_mu);
    if (revit_process)
        CloseHandle(revit_process);
    revit_process = pi.hProcess;
    revit_process_id = pi.dwProcessId;
    return revit_process_id;
}

DWORD running_revit_pid() {
    std::lock_guard lock(revit_mu);
    if (revit_process != nullptr && WaitForSingleObject(revit_process, 0) == WAIT_TIMEOUT)
        return revit_process_id;
    return 0;
}

void terminate_revit_process() {
    std::lock_guard lock(revit_mu);
    if (revit_process == nullptr) {
        LOG_SAVE("No Revit process was launched by bgHelper - nothing to terminate");
        return;
    }

    if (WaitForSingleObject(revit_process, 0) != WAIT_TIMEOUT) {
        LOG_SAVE("Revit (pid {}) has already exited", revit_process_id);
    }
    else if (TerminateProcess(revit_process, 0)) {
        LOG_SAVE("Revit terminated successfully");
    }
    else {
        LOG_SAVE("Failed to terminate process. Error: {}", GetLastError());
    }

    CloseHandle(revit_process);
    revit_process = nullptr;
    revit_process_id = 0;
}
