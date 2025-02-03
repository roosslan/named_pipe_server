#pragma once

#include <cstdio>
#include <windows.h>
#include "StdAfx.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <tlhelp32.h>

#ifndef HELPER_FUNCS_H
#define HELPER_FUNCS_H

static constexpr unsigned char PermanentConfig = 0;
static constexpr unsigned char TemporaryConfig = 1;

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

void iniTimer_check();

void pipeMessageHandler(
	void* context,
	w32::CHandle& handle,
	CIOBuffer& input,
	CIOBuffer& output);

VOID startRevitProccess(LPCTSTR lpApplicationName);
std::string GetConfigFilePath(int ConfigFileType);
std::string ReadINF_Flag(LPCWSTR keyName);
bool IsProcessRunning(const wchar_t* processName);
void receiveData(int socket, void (*callback)(int, const char*, SSIZE_T));
void onDataReceived(int socket, const char* buffer, SSIZE_T bytesReceived);

#endif