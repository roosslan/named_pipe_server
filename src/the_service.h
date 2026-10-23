#include <winsock2.h>
#include <ws2tcpip.h>

#if !defined(AFX_BG_H__115F4224_5CD5_11D1_ABBA_00A0243D1382__INCLUDED_)
#define AFX_BG_H__115F4224_5CD5_11D1_ABBA_00A0243D1382__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "StdAfx.h"

#include <atomic>
#include <mutex>
#include <string_view>

#ifndef __AFXWIN_H__
#error include 'stdafx.h' before including this file for PCH
#endif

class CBgHelperSrv final : public CWinApp {
	std::string inf_config_file_path_;
	SOCKADDR_IN addr_{};
	int tcp_port_ = 6777;
	bool wsa_started_ = false;

	/* TCP-сокет до ifc_exporter (QML). Используется из нескольких потоков, поэтому
	 * любое обращение к нему - только под socket_mu_ */
	SOCKET socket_ = INVALID_SOCKET;
	std::mutex socket_mu_;

	HANDLE instance_mutex_ = nullptr;	/* защита от второго экземпляра */
	HANDLE stop_event_ = nullptr;		/* сигнал завершения работы */
	HANDLE start_event_ = nullptr;		/* запрос немедленного запуска экспорта (START_IMMEDIATELY) */

	void close_socket_locked();
	void run_pipe_server();
public:
	std::atomic<bool> m_connected_to_qml{ false };
	std::mutex mu;		/* сериализует запуск Revit */

	CBgHelperSrv();
	bool socket_connect();
	void socket_send(std::string_view data);
	void close_socket();

	/* Запросить немедленный запуск экспорта, обработка - в основном цикле */
	void request_start_immediately() const;
	/* Запросить завершение работы */
	void request_stop() const;

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CBgHelperSrv)
	BOOL InitInstance() override;
	int ExitInstance() override;
	//}}AFX_VIRTUAL
	// Implementation
	//{{AFX_MSG(CBgHelperSrv)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

extern CBgHelperSrv bg_service;

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_BG_H__115F4224_5CD5_11D1_ABBA_00A0243D1382__INCLUDED_)
