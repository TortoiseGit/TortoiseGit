// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2003-2008 - TortoiseSVN
// Copyright (C) 2008-2012 - TortoiseGit

// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software Foundation,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//
#include "stdafx.h"
#include "TortoiseProc.h"
#include "ExportDlg.h"

#include "Messagebox.h"
#include "PathUtils.h"
#include "BrowseFolder.h"
#include "AppUtils.h"


IMPLEMENT_DYNAMIC(CExportDlg, CHorizontalResizableStandAloneDialog)
CExportDlg::CExportDlg(CWnd* pParent /*=NULL*/)
	: CHorizontalResizableStandAloneDialog(CExportDlg::IDD, pParent)
	, CChooseVersion(this)
	, m_Revision(_T("HEAD"))
	, m_strExportDirectory(_T(""))
	, m_sExportDirOrig(_T(""))
	, m_bNoExternals(FALSE)
	, m_pLogDlg(NULL)
{
}

CExportDlg::~CExportDlg()
{
	if (m_pLogDlg)
		delete m_pLogDlg;
}

void CExportDlg::DoDataExchange(CDataExchange* pDX)
{
	CHorizontalResizableStandAloneDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_CHECKOUTDIRECTORY, m_strExportDirectory);
	DDX_Control(pDX, IDC_CHECKOUTDIRECTORY, m_cCheckoutEdit);

	CHOOSE_VERSION_DDX;

}


BEGIN_MESSAGE_MAP(CExportDlg, CHorizontalResizableStandAloneDialog)
	ON_BN_CLICKED(IDC_CHECKOUTDIRECTORY_BROWSE, OnBnClickedCheckoutdirectoryBrowse)
	ON_EN_CHANGE(IDC_CHECKOUTDIRECTORY, OnEnChangeCheckoutdirectory)
	ON_BN_CLICKED(IDHELP, OnBnClickedHelp)

	CHOOSE_VERSION_EVENT
	ON_WM_DESTROY()
END_MESSAGE_MAP()

BOOL CExportDlg::OnInitDialog()
{
	CHorizontalResizableStandAloneDialog::OnInitDialog();
	CAppUtils::MarkWindowAsUnpinnable(m_hWnd);

	m_sExportDirOrig = m_strExportDirectory;
	m_bAutoCreateTargetName = !PathIsDirectoryEmpty(m_sExportDirOrig);

	AddAnchor(IDC_REPOGROUP, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_EXPORT_CHECKOUTDIR, TOP_LEFT);
	AddAnchor(IDC_CHECKOUTDIRECTORY_BROWSE, TOP_RIGHT);
	AddAnchor(IDC_CHECKOUTDIRECTORY, TOP_LEFT, TOP_RIGHT);

	AddAnchor(IDOK, BOTTOM_RIGHT);
	AddAnchor(IDCANCEL, BOTTOM_RIGHT);
	AddAnchor(IDHELP, BOTTOM_RIGHT);

	AdjustControlSize(IDC_RADIO_HEAD);
	AdjustControlSize(IDC_RADIO_BRANCH);
	AdjustControlSize(IDC_RADIO_TAGS);
	AdjustControlSize(IDC_RADIO_VERSION);

	CString sWindowTitle;
	GetWindowText(sWindowTitle);
	CAppUtils::SetWindowTitle(m_hWnd, g_Git.m_CurrentDir, sWindowTitle);

	CHOOSE_VERSION_ADDANCHOR;
	this->AddOthersToAnchor();
	Init();
	if(this->m_Revision.IsEmpty())
	{
		SetDefaultChoose(IDC_RADIO_HEAD);
	}
	else
	{
		SetDefaultChoose(IDC_RADIO_VERSION);
		this->GetDlgItem(IDC_COMBOBOXEX_VERSION)->SetWindowTextW(m_Revision);
	}

	m_tooltips.Create(this);
	m_tooltips.AddTool(IDC_CHECKOUTDIRECTORY, IDS_CHECKOUT_TT_DIR);

	SHAutoComplete(GetDlgItem(IDC_CHECKOUTDIRECTORY)->m_hWnd, SHACF_FILESYSTEM);

	if ((m_pParentWnd==NULL)&&(hWndExplorer))
		CenterWindow(CWnd::FromHandle(hWndExplorer));
	EnableSaveRestore(_T("ExportDlg"));
	return TRUE;
}

void CExportDlg::OnOK()
{
	if (!UpdateData(TRUE))
		return; // don't dismiss dialog (error message already shown by MFC framework)

	// check it the export path is a valid windows path
	UpdateRevsionName();

	if (m_VersionName.IsEmpty())
	{
		m_tooltips.ShowBalloon(IDC_COMBOBOXEX_VERSION, IDS_ERR_INVALIDREV, IDS_ERR_ERROR, TTI_ERROR);
		return;
	}

	m_bAutoCreateTargetName = false;

//	m_URLCombo.SaveHistory();
//	m_URL = m_URLCombo.GetString();


	if(::PathFileExists(this->m_strExportDirectory))
	{
		if(::PathIsDirectory(m_strExportDirectory))
		{
			CMessageBox::Show(NULL, IDS_PROCEXPORTERRFOLDER, IDS_APPNAME, MB_OK | MB_ICONERROR);
			return;
		}
		CString sMessage;
		sMessage.Format(IDS_PROC_OVERWRITE_CONFIRM, m_strExportDirectory);
		if (CMessageBox::Show(NULL, sMessage, _T("TortoiseGit"), MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) != IDYES)
		{
			return ;
		}
	}
	else if (m_strExportDirectory.IsEmpty())
	{
		CMessageBox::Show(NULL, IDS_PROC_NOZIPFILE, IDS_APPNAME, MB_OK | MB_ICONERROR);
		return;
	}

	UpdateData(FALSE);
	CHorizontalResizableStandAloneDialog::OnOK();
}

void CExportDlg::OnBnClickedBrowse()
{
	m_tooltips.Pop();	// hide the tooltips

}

void CExportDlg::OnBnClickedCheckoutdirectoryBrowse()
{
	m_tooltips.Pop();	// hide the tooltips
	//
	// Create a folder browser dialog. If the user selects OK, we should update
	// the local data members with values from the controls, copy the checkout
	// directory from the browse folder, then restore the local values into the
	// dialog controls.
	//
	this->UpdateRevsionName();
	CFileDialog dlg(FALSE, _T("zip"), this->m_VersionName, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, _T("*.zip"));

	if(dlg.DoModal()==IDOK)
	{
		UpdateData(TRUE);
		m_strExportDirectory = dlg.GetPathName();
		UpdateData(FALSE);
	}
}

BOOL CExportDlg::PreTranslateMessage(MSG* pMsg)
{
	m_tooltips.RelayEvent(pMsg);
	return CHorizontalResizableStandAloneDialog::PreTranslateMessage(pMsg);
}

void CExportDlg::OnEnChangeCheckoutdirectory()
{
	UpdateData(TRUE);
	DialogEnableWindow(IDOK, !m_strExportDirectory.IsEmpty());
}

void CExportDlg::OnBnClickedHelp()
{
	OnHelp();
}

void CExportDlg::OnBnClickedShowlog()
{
	m_tooltips.Pop();	// hide the tooltips

}

void CExportDlg::OnCbnSelchangeEolcombo()
{
}

void CExportDlg::OnCbnEditchangeUrlcombo()
{
	if (!m_bAutoCreateTargetName)
		return;
	if (m_sExportDirOrig.IsEmpty())
		return;
	// find out what to use as the checkout directory name
	UpdateData();
	m_URLCombo.GetWindowText(m_URL);
	if (m_URL.IsEmpty())
		return;
	CString name = CAppUtils::GetProjectNameFromURL(m_URL);
	m_strExportDirectory = m_sExportDirOrig+_T('\\')+name;
	UpdateData(FALSE);
}

void CExportDlg::OnDestroy()
{
	WaitForFinishLoading();
	__super::OnDestroy();
}
