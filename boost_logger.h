#pragma once

#ifndef BOOST_LOGGER_H
#define BOOST_LOGGER_H

#include <shellapi.h>
#include <boost/current_function.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/utility/manipulators/add_value.hpp>

namespace src = boost::log::sources;
namespace logging = boost::log;

#define LOG_SAVE BOOST_LOG_SEV(boost::log::trivial::logger::get(), boost::log::trivial::severity_level::trace)	\
	<< "bg <" << rLogger::path_to_filename(__FILE__) << ":" << __LINE__ << "> " BOOST_CURRENT_FUNCTION << " | " 	\
	<< boost::log::add_value("Line", __LINE__)

namespace rLogger
{
	std::string get_self_version();
	inline const std::string bg_helper_version = get_self_version();
	static 
		_Check_return_
		_Post_equals_last_error_
		DWORD
		WINAPI LAST_ERROR;
	void init_logging();
	void log_formatter(logging::record_view const& rec, logging::formatting_ostream& strm);
	std::string path_to_filename(std::string path);
	std::string get_log_folder_path();	
};

#endif