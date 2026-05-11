// stdafx.h: include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
#define _WTL_NO_CSTRING
#define _WTL_NO_WTYPES
#define _WTL_NO_AUTOMATIC_NAMESPACE

#define WIN32_LEAN_AND_MEAN // MFC must use Winsock2.h. So we need include MFC headers first (before windows.h).

#ifndef __STDAFX__
#define __STDAFX__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define VC_EXTRALEAN				// Exclude rarely-used stuff from Windows headers

#include <afxwin.h>					// MFC core and standard components
#include <afxext.h>					// MFC extensions
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>					// MFC support for Windows Common Controls
#endif // _AFX_NO_AFXCMN_SUPPORT

#include <atlcomcli.h>
#include <winsvc.h>
#include <tchar.h>
#include <stdio.h>
#include <iostream>
#include <string>
#include <vector>
#include <set>
#include <shlobj.h>
#include <codecvt> // for std::codecvt_utf8
#include <locale>  // for std::wstring_convert
#include <commctrl.h>
#include <shellapi.h>
#include <objbase.h>
#include <atlbase.h>
#include <future>
#include <filesystem>
#include <regex>
#include <sti.h>
#include <gdiplus.h>
#include <windows.h>
#include <fstream>
#include <sstream>
#include <conio.h>
#include <thread>
#include <string_view>

#include "boost_logger.h"
#include "w32.h"

using namespace w32;

#define COUNTOF(x) ( sizeof(x) / sizeof(*x) )

#define DEFAULT_STRING_SIZE 256

extern HINSTANCE g_hInstance;

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual Studio will insert additional declarations immediately before the previous line.
#endif //__STDAFX__