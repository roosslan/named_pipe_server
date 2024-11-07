#pragma once

#include "StdAfx.h"

#ifndef HELPER_FUNCS_H
#define HELPER_FUNCS_H

static constexpr unsigned char PermanentConfig = 0;
static constexpr unsigned char TemporaryConfig = 1;

enum class rus1Action
{
	Print,
	Start,
	Nothing,
	Save,
	SaveByDate
};

struct actEvent {
	std::string Device_Name;
	std::string Device_GUID;
	std::string btnEvent_Name;
	std::string Event_GUID;
	int indexOfDevice;
	rus1Action  Action;
	std::string Param;
};

void pipeMessageHandler(
	void* context,
	w32::CHandle& handle,
	CIOBuffer& input,
	CIOBuffer& output);

std::string bstr_to_u32(BSTR source);
bool launchDebugger();
std::string GetConfigFilePath(int ConfigFileType);
bool fill_cb_Events(CComboBox* comboBoxDevices, CComboBox* comboBoxEvents, std::vector<actEvent>* actEvents);
bool SaveActionToFile(std::string actionToSave);
std::string BstrToStdString(BSTR bstr);

#endif