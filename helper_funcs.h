#pragma once

#include <cstdio>
#include <windows.h>
#include "StdAfx.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <tlhelp32.h>

#include "CHandle.h"
#include "CIOBuffer.h"

static constexpr unsigned char permanent_config = 0;
static constexpr unsigned char temporary_config = 1;

struct active_object
{
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
std::string get_config_file_path(int config_file_type);
std::string read_inf_flag(LPCWSTR key_name);
bool is_process_running(const wchar_t* process_name);
void receiveData(int socket, void (*callback)(int, const char*, SSIZE_T));
void onDataReceived(int socket, const char* buffer, SSIZE_T bytesReceived);
std::string get_env(const std::string& env_var);
