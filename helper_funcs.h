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

static std::string get_host_name();
bool is_network_file_exists();
void ini_timer_check(bool start_immediately, bool* started_status);

void pipe_message_handler(
	void* context,
	w32::CHandle& handle,
	w32::CIOBuffer& input,
	w32::CIOBuffer& output);

void start_revit_process(LPCTSTR lp_application_name);
std::string get_config_file_path();
std::string read_inf_flag(LPCWSTR key_name);
bool is_process_running(const wchar_t* process_name);

/* Windows API(Win32) */
std::wstring expand_environment_variables(const std::wstring& input);

/* _dupenv_s wrapper, C Runtime Library (CRT) */
std::string get_env(const std::string& env_var);

void receiveData(int socket, void (*callback)(int, const char*, SSIZE_T));
void onDataReceived(int socket, const char* buffer, SSIZE_T bytesReceived);
