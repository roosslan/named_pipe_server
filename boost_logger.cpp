#include <Windows.h>
#include <Shlwapi.h>

#include <boost/locale.hpp>
#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>

#include "boost_logger.h"

namespace logging = boost::log;
namespace src = boost::log::sources;
namespace expr = boost::log::expressions;
namespace sinks = boost::log::sinks;
namespace attrs = boost::log::attributes;
namespace keywords = boost::log::keywords;

std::string rLogger::PathToFilename(std::string path) {
    return path.substr(path.find_last_of("/\\") + 1);
}

void rLogger::LogFormatter(logging::record_view const& rec, logging::formatting_ostream& strm)
{
    // strm << logging::extract< int >("Line", rec) << ":";
    strm << rec[expr::smessage];
}

void rLogger::InitLogging()
{
    char* appdata = getenv("APPDATA");
    std::string roamingDirectory;
    /* Convert the Windows path type to a C++ path */
    roamingDirectory = appdata;


    boost::log::add_common_attributes();

    auto consoleSink = boost::log::add_console_log(std::clog);
    consoleSink->set_formatter(&LogFormatter);
    logging::core::get()->add_sink(consoleSink);

    auto fsSink = boost::log::add_file_log(
        boost::log::keywords::file_name = roamingDirectory + "\\alabuga_dev\\alabuga.bg.log", /* "\\alabuga_dev\\alabuga_dev%d.%m.%Y-%H_%M_%S.log", */
        keywords::format = "%TimeStamp% % Message % ",
        boost::log::keywords::rotation_size = 10 * 1024 * 1024,
        boost::log::keywords::min_free_space = 30 * 1024 * 1024,
        boost::log::keywords::open_mode = std::ios_base::app);
    std::locale loc = boost::locale::generator()("ru_RU.UTF-8");
    fsSink->locked_backend()->auto_flush(true);

    LOG_SAVE << "extBIMALDE bgHelper v" << bgHelperVersion << "'s logger initialized";
}

std::string rLogger::GetLogFolderPath() {
    char current_path[MAX_PATH];
    GetModuleFileName(NULL, current_path, MAX_PATH);
    PathRemoveFileSpec(current_path);		            /* Removes filename from the path */
    return std::string(current_path);
}