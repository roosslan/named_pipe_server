#pragma once

#include "StdAfx.h"
#include <winsock2.h>
#include <ws2tcpip.h>

#ifndef HELPER_FUNCS_H
#define HELPER_FUNCS_H

static constexpr unsigned char PermanentConfig = 0;
static constexpr unsigned char TemporaryConfig = 1;

void iniTimer_check();

void pipeMessageHandler(
	void* context,
	w32::CHandle& handle,
	CIOBuffer& input,
	CIOBuffer& output);

VOID startRevitProccess(LPCTSTR lpApplicationName);
std::string GetConfigFilePath(int ConfigFileType);

#endif