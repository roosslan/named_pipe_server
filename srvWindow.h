#if !defined(AFX_RUS1SCANWINDOW_H__115F4226_5CD5_11D1_00A0243D1382__INCLUDED_)
#define AFX_RUS1SCANWINDOW_H__115F4226_5CD5_11D1_00A0243D1382__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "StdAfx.h"
#include "resource.h"

#include "helper_funcs.h"

// Imaging Device
// Class = Image
// ClassGuid = { 6bdd1fc6 - 810f - 11d0 - bec7 - 08002be2092f }
// This class includes still - image capture devices, digital cameras, and scanners.

/////////////////////////////////////////////////////////////////////////////
// CSrvWindow (dialog window)

class CSrvWindow : public CDialog
{
	BOOL  m_bDisplayWaitCursor;
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	//	void SaveCmdToFile(bool asAdministrator);
	//	void LoadCmdFromFile();
	std::string GetConfigFilePath();
public:	
//	CAddinUpdater* addinUpdater;
	CSrvWindow(CWnd* pParent = NULL);	// standard constructor
// Dialog Data
	//{{AFX_DATA(CSrvWindow)
	enum { IDD = IDD_RUS1SCAN_DIALOG };
	CComboBox m_ComboBoxEvents;
	CComboBox m_ComboBoxDevices;
	CComboBox m_ComboBoxPrinters;
	CStatic m_DescrText;
	//}}AFX_DATA
	CEdit m_editSaveDirectory;
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSrvWindow)
protected:
	CToolTipCtrl m_ToolTip;
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL
	afx_msg void OnBnClickedButtonBrowse();
	// Generated message map functions
	//{{AFX_MSG(CSrvWindow)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	void OnComboChanged();
	void OnComboEdited();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_RUS1SCANWINDOW_H__115F4226_5CD5_11D1_00A0243D1382__INCLUDED_)
