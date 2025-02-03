#include <winsock2.h>
#include <ws2tcpip.h>

#if !defined(AFX_BG_H__115F4224_5CD5_11D1_ABBA_00A0243D1382__INCLUDED_)
#define AFX_BG_H__115F4224_5CD5_11D1_ABBA_00A0243D1382__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "StdAfx.h"

#ifndef __AFXWIN_H__
#error include 'stdafx.h' before including this file for PCH
#endif

class CUpdaterService : public CWinApp
{
	std::string infConfigFilePath;
	WSADATA wsa_data;
	SOCKADDR_IN addr;
public:
	std::mutex mu;
	bool connectedToQML = false;
	CUpdaterService();
	bool SocketConnect();
	void CloseSocket();
	SOCKET server_socket;
	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CUpdaterService)	
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL
	// Implementation
	//{{AFX_MSG(CUpdaterService)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()	
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_BG_H__115F4224_5CD5_11D1_ABBA_00A0243D1382__INCLUDED_)

