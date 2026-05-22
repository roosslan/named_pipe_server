#include <Windows.h> /* Самый первый, потому что C1189: MFC requires use of Winsock2.h */
#include <Shlwapi.h>

#include "boost_logger.h"
#include "sensitive_data.h"

#include <boost/locale.hpp>
#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>

#pragma comment(lib, "version.lib")

namespace logging = boost::log;
namespace expr = boost::log::expressions;
namespace keywords = boost::log::keywords;

std::string rLogger::path_to_filename(const std::string& path) {
    return path.substr(path.find_last_of("/\\") + 1);
}

void rLogger::log_formatter(logging::record_view const& rec, logging::formatting_ostream& strm) {
    // strm << logging::extract< int >("Line", rec) << ":";
    strm << rec[expr::smessage];
}

void rLogger::init_logging() {
    const std::wstring roaming_directory = expand_environment_variables(L"%appdata%");
    
    boost::log::add_common_attributes();

    const auto console_sink = boost::log::add_console_log(std::clog);
    console_sink->set_formatter(&log_formatter);
    logging::core::get()->add_sink(console_sink);

    const auto fs_sink = boost::log::add_file_log(
        boost::log::keywords::file_name = roaming_directory + log_file_path,
        keywords::format = "%TimeStamp% % Message % ",
        boost::log::keywords::rotation_size = 10 * 1024 * 1024,
        boost::log::keywords::min_free_space = 30 * 1024 * 1024,
        boost::log::keywords::open_mode = std::ios_base::app);
    std::locale loc = boost::locale::generator()("ru_RU.UTF-8");
    fs_sink->locked_backend()->auto_flush(true);

    LOG_SAVE << "logger initialized. " << "bgHelper version is " << bg_helper_version;
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