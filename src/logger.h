#ifndef LOGGER_H
#define LOGGER_H

#include <string>

#include <spdlog/spdlog.h>

/* Запись в лог: LOG_SAVE("Started Revit {}, pid {}", version, pid).
 * Имя файла, строка и функция добавляются паттерном логгера (см. init_logging) */
#define LOG_SAVE(...) SPDLOG_LOGGER_CALL(spdlog::default_logger_raw(), spdlog::level::info, __VA_ARGS__)

namespace rLogger
{
	std::string get_self_version();
	inline const std::string bg_helper_version = get_self_version();
	void init_logging();
	std::string get_log_folder_path();
	std::wstring expand_environment_variables(const std::wstring& input);
};

#endif
