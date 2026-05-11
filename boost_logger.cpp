#include <Windows.h>
#include <Shlwapi.h>

#include <boost/locale.hpp>
#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>

#include "boost_logger.h"

#pragma comment(lib, "version.lib")

namespace logging = boost::log;
namespace src = boost::log::sources;
namespace expr = boost::log::expressions;
namespace sinks = boost::log::sinks;
namespace attrs = boost::log::attributes;
namespace keywords = boost::log::keywords;

std::string rLogger::path_to_filename(std::string path) {
    return path.substr(path.find_last_of("/\\") + 1);
}

void rLogger::log_formatter(logging::record_view const& rec, logging::formatting_ostream& strm)
{
    // strm << logging::extract< int >("Line", rec) << ":";
    strm << rec[expr::smessage];
}

void rLogger::init_logging()
{
    char* appdata = getenv("APPDATA");
    std::string roamingDirectory;
    /* Convert the Windows path type to a C++ path */
    roamingDirectory = appdata;

    boost::log::add_common_attributes();

    auto consoleSink = boost::log::add_console_log(std::clog);
    consoleSink->set_formatter(&log_formatter);
    logging::core::get()->add_sink(consoleSink);

    auto fsSink = boost::log::add_file_log(
        boost::log::keywords::file_name = roamingDirectory + "\\alabuga_dev\\alabuga.bg.log", /* "\\alabuga_dev\\alabuga_dev%d.%m.%Y-%H_%M_%S.log", */
        keywords::format = "%TimeStamp% % Message % ",
        boost::log::keywords::rotation_size = 10 * 1024 * 1024,
        boost::log::keywords::min_free_space = 30 * 1024 * 1024,
        boost::log::keywords::open_mode = std::ios_base::app);
    std::locale loc = boost::locale::generator()("ru_RU.UTF-8");
    fsSink->locked_backend()->auto_flush(true);

    LOG_SAVE << "logger initialized. " << "bgHelper version is " << bg_helper_version;
}

std::string rLogger::get_log_folder_path() {
    char current_path[MAX_PATH];
    GetModuleFileName(NULL, current_path, MAX_PATH);
    PathRemoveFileSpec(current_path);		            /* Removes filename from the path */
    return std::string(current_path);
}

std::string rLogger::get_self_version() {
    char szPath[MAX_PATH];
    GetModuleFileNameA(NULL, szPath, MAX_PATH);

    DWORD dwHandle = 0;
    DWORD dwSize = GetFileVersionInfoSizeA(szPath, &dwHandle);

    if (dwSize == 0) return "0.0.0.1";

    std::vector<BYTE> pVersionInfo(dwSize);
    if (!GetFileVersionInfoA(szPath, dwHandle, dwSize, pVersionInfo.data()))
        return "0.0.0.2";

    VS_FIXEDFILEINFO* pFileInfo = nullptr;
    UINT uiSize = 0;
    if (!VerQueryValueA(pVersionInfo.data(), "\\", (LPVOID*)&pFileInfo, &uiSize))
        return "0.0.0.3";

    return std::to_string(HIWORD(pFileInfo->dwFileVersionMS)) + "." +
        std::to_string(LOWORD(pFileInfo->dwFileVersionMS)) + "." +
        std::to_string(HIWORD(pFileInfo->dwFileVersionLS)) + "." +
        std::to_string(LOWORD(pFileInfo->dwFileVersionLS));
}