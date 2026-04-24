#pragma once

#ifndef BOOST_LOGGER_H
#define BOOST_LOGGER_H

#include <shellapi.h>
#include <boost/current_function.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/utility/manipulators/add_value.hpp>


namespace src = boost::log::sources;
namespace logging = boost::log;

const std::string bgHelperVersion = "3.12.10.1";

#define LOG_SAVE BOOST_LOG_SEV(boost::log::trivial::logger::get(), boost::log::trivial::severity_level::trace)	\
	<< "bg <" << rLogger::PathToFilename(__FILE__) << ":" << __LINE__ << "> " BOOST_CURRENT_FUNCTION << " | " 	\
	<< boost::log::add_value("Line", __LINE__)

namespace rLogger
{
	static 
		_Check_return_
		_Post_equals_last_error_
		DWORD
		WINAPI LAST_ERROR;
	void InitLogging();
	void LogFormatter(logging::record_view const& rec, logging::formatting_ostream& strm);
	std::string PathToFilename(std::string path);
	std::string GetLogFolderPath();
};

#endif