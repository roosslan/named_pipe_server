
#include "StdAfx.h"

#include "theService.h"
#include "srvWindow.h"
#include "helper_funcs.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// CSrvWindow dialog

CSrvWindow::CSrvWindow(CWnd* pParent /*=NULL*/)
	: CDialog(CSrvWindow::IDD, pParent)
{
	//{{AFX_DATA_INIT(CSrvWindow)
	
	//}}AFX_DATA_INIT
}

void CSrvWindow::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSrvWindow)
	DDX_Control(pDX, IDC_STATIC_DESCRIPTION, m_DescrText);
	DDX_Control(pDX, IDC_EDIT_FOLDER, m_editSaveDirectory);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CSrvWindow, CDialog)
	//{{AFX_MSG_MAP(CSrvWindow)
	ON_BN_CLICKED(IDC_BUTTON_BROWSE, &CSrvWindow::OnBnClickedButtonBrowse)
	ON_WM_SYSCOMMAND(WM_SYSCOMMAND, CSrvWindow::OnSysCommand)
	// ON_COMMAND(IDM_ABOUT, CSrvWindow::OnAbout)	
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSrvWindow message handlers
afx_msg void CSrvWindow::OnSysCommand(UINT nID, LPARAM lParam) {

	if ((nID & 0xFFF0) == IDM_ABOUT) AfxMessageBox("(c) 2024 alabuga dev");
	else {
		CDialog::OnSysCommand(nID, lParam);
	}
}

void CSrvWindow::OnComboChanged()
{
	int indexSelected = m_ComboBoxDevices.GetCurSel();

	m_ComboBoxEvents.ResetContent();
/*	for (std::vector<actEvent>::iterator it = addinUpdater->actEvents.begin(); it != addinUpdater->actEvents.end(); ++it)
	{
		if (indexSelected == it->indexOfDevice)
			m_ComboBoxEvents.AddString(it->btnEvent_Name.c_str());
	}
*/
}

void CSrvWindow::OnComboEdited()
{
	UpdateData();
}

std::string CSrvWindow::GetConfigFilePath() {
	char current_path[MAX_PATH];
	GetModuleFileName(NULL, current_path, MAX_PATH);
	PathRemoveFileSpec(current_path);		// Removes filename from the path	
	strcat(current_path, "\\rus1scan.conf");
	return std::string(current_path);
}

BOOL CSrvWindow::PreTranslateMessage(MSG* pMsg)
{
	m_ToolTip.RelayEvent(pMsg);
	return CDialog::PreTranslateMessage(pMsg);
}

BOOL CSrvWindow::OnInitDialog()
{
	CDialog::OnInitDialog();
	HICON hIcon = LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_ICON_MAIN));
	SetIcon(hIcon, FALSE);

	CMenu* hSysMenu = GetSystemMenu(FALSE);
	AppendMenuW(*hSysMenu, MF_STRING, IDM_ABOUT, L"&About...");

	//Create the ToolTip control
	if (!m_ToolTip.Create(this))
	{
		TRACE0("Unable to create the ToolTip!");
	}
	else
	{
		// Add tool tips to the controls
		m_ToolTip.AddTool(&m_editSaveDirectory, IDC_EDIT_FOLDER);
		m_ToolTip.Activate(TRUE);
	}

	m_editSaveDirectory.SetWindowText("C:\\Scans\\%d.%m.%Y\\"); // Edit_SetCueBannerText(m_editDatFormat.GetSafeHwnd(), "DDMMYYYY");
	
//	fill_cb_Events(&m_ComboBoxDevices, &m_ComboBoxEvents, &addinUpdater->actEvents);


	return FALSE;	// return TRUE  unless you set the focus to a control
}

void CSrvWindow::OnOK()
{
	// Since SHORT is signed, high - order bit equals sign bit.
	// Therefore to check if CTRL key is pressed, we check if the value returned by GetKeyState() is negative
	if (GetKeyState(VK_LCONTROL) < 0 || GetKeyState(VK_RCONTROL) < 0)
		SaveActionToFile("true");
	else
		SaveActionToFile("false");
	EndDialog(IDCANCEL); // Quit the programm
}

void CSrvWindow::OnBnClickedButtonBrowse() // "Browse..." for directory button
{
	CFolderPickerDialog m_dlg;
	std::string m_Folder;
	if (m_dlg.DoModal() == IDOK) {
		m_Folder = m_dlg.GetPathName();
	}
}
