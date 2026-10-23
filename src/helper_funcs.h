#pragma once

#include "stdafx.h"

#include <cstdio>
#include <windows.h>

#include <winsock2.h>
#include <ws2tcpip.h>
#include <tlhelp32.h>

#include "cnamedps\CHandle.h"
#include "cnamedps\CIOBuffer.h"

struct active_object {
    template < typename FN > active_object(FN fn) : thread([this, fn] { while (alive) fn(); }) {}

    ~active_object() { alive = false; thread.join(); }

    active_object(const active_object&) = delete;
    active_object(active_object&&) = delete;
    active_object& operator= (active_object) = delete;

    std::atomic<bool> alive{ true };
    std::thread thread;
};

/* Неблокирующая проверка .sav-файла на сетевом диске (задел под удалённый экспорт).
 * poll() вызывается периодически из одного потока; результат пишется в лог при изменении */
class remote_sav_checker {
    std::future<bool> check_;
    int last_state_ = -1;   /* -1 - ещё не проверялось */
public:
    void poll();
};

bool is_network_file_exists(const std::string& remote_sav_dir);
void ini_timer_check(bool start_immediately);

void pipe_message_handler(
	void* context,
	w32::CHandle& handle,
	w32::CIOBuffer& input,
	w32::CIOBuffer& output);

/* Запущенный bgHelper'ом процесс Revit. Хендл процесса хранится до завершения,
 * чтобы TerminateProcess не попал в чужой процесс с переиспользованным pid */
/* Возвращает pid запущенного процесса, 0 - при ошибке */
DWORD start_revit_process(const std::string& revit_exe_path);
/* pid запущенного ранее Revit, если он ещё работает, иначе 0 */
DWORD running_revit_pid();
void terminate_revit_process();

std::string get_config_file_path();
std::string read_inf_flag(LPCWSTR key_name);
bool is_process_running(const wchar_t* process_name);

/* Windows API(Win32) */
std::wstring expand_environment_variables(const std::wstring& input);

/* _dupenv_s wrapper, C Runtime Library (CRT) */
std::string get_env(const std::string& env_var);
