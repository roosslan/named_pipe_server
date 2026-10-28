#include <Windows.h> /* Самый первый, потому что C1189: MFC requires use of Winsock2.h */
#include <Shlwapi.h>

#include <memory>
#include <vector>

#include "logger.h"
#include "sensitive_data.h"

#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_sinks.h>

#pragma comment(lib, "version.lib")

namespace
{
    constexpr size_t log_rotation_size = 10 * 1024 * 1024;
    constexpr size_t log_max_files = 5;
}

void rLogger::init_logging() {
    std::wstring log_path = expand_environment_variables(L"%appdata%") + log_file_path;

    /* Совместимость со старыми sensitive_data.h: шаблон номера файла "%N" был нужен Boost.Log,
     * spdlog нумерует файлы сам (bg_helper.log, bg_helper.1.log, ...) */
    for (const std::wstring placeholder : { L"_%N", L"%N" }) {
        if (const auto pos = log_path.find(placeholder); pos != std::wstring::npos)
            log_path.erase(pos, placeholder.size());
    }

    auto console_sink = std::make_shared<spdlog::sinks::stderr_sink_mt>();
    console_sink->set_pattern("%v");

    std::vector<spdlog::sink_ptr> sinks{ console_sink };
    std::string file_error;
    try {
        auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(log_path, log_rotation_size, log_max_files);
        file_sink->set_pattern("%Y-%m-%d %H:%M:%S.%f bg <%s:%#> %! | %v");
        sinks.push_back(file_sink);
    }
    catch (const spdlog::spdlog_ex& ex) {
        /* Без файла продолжаем работать с выводом в консоль */
        file_error = ex.what();
    }

    auto logger = std::make_shared<spdlog::logger>("bg", sinks.begin(), sinks.end());
    logger->set_level(spdlog::level::trace);
    logger->flush_on(spdlog::level::trace);
    spdlog::set_default_logger(logger);

    if (!file_error.empty())
        LOG_SAVE("Failed to open log file: {}", file_error);
    LOG_SAVE("logger initialized. bgHelper version is {}", bg_helper_version);
}

std::string rLogger::get_log_folder_path() {
    char current_path[MAX_PATH];
    GetModuleFileName(nullptr, current_path, MAX_PATH);
    PathRemoveFileSpec(current_path);		            /* Removes filename from the path */
    return std::string(current_path);
}

std::string rLogger::get_self_version() {
    char sz_path[MAX_PATH];
    GetModuleFileNameA(nullptr, sz_path, MAX_PATH);

    DWORD dw_handle = 0;
    const DWORD dw_size = GetFileVersionInfoSizeA(sz_path, &dw_handle);

    if (dw_size == 0) return "0.0.0.1";

    std::vector<BYTE> pVersionInfo(dw_size);
    if (!GetFileVersionInfoA(sz_path, dw_handle, dw_size, pVersionInfo.data()))
        return "0.0.0.2";

    VS_FIXEDFILEINFO* ptr_file_info = nullptr;
    UINT ui_size = 0;
    if (!VerQueryValueA(pVersionInfo.data(), "\\", (LPVOID*)&ptr_file_info, &ui_size))
        return "0.0.0.3";

    return std::to_string(HIWORD(ptr_file_info->dwFileVersionMS)) + "." +
        std::to_string(LOWORD(ptr_file_info->dwFileVersionMS)) + "." +
        std::to_string(HIWORD(ptr_file_info->dwFileVersionLS)) + "." +
        std::to_string(LOWORD(ptr_file_info->dwFileVersionLS));
}

std::wstring rLogger::expand_environment_variables(const std::wstring& input) {
    DWORD size = ExpandEnvironmentStringsW(input.c_str(), nullptr, 0);
    if (size == 0) return input;

    std::vector<wchar_t> buffer(size);
    ExpandEnvironmentStringsW(input.c_str(), buffer.data(), size);
    return std::wstring(buffer.data());
}
