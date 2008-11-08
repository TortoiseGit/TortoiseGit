// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2008 - TortoiseSVN

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
#include "CheckoutDlg.h"
#include "RepositoryBrowser.h"
#include "Messagebox.h"
#include "PathUtils.h"
#include "BrowseFolder.h"
#include "AppUtils.h"


IMPLEMENT_DYNAMIC(CCheckoutDlg, CResizableStandAloneDialog)
CCheckoutDlg::CCheckoutDlg(CWnd* pParent /*=NULL*/)
	: CResizableStandAloneDialog(CCheckoutDlg::IDD, pParent)
	, Revision(_T("HEAD"))
	, m_strCheckoutDirectory(_T(""))
	, m_sCheckoutDirOrig(_T(""))
	, m_bNoExternals(FALSE)
	, m_pLogDlg(NULL)
{
}

CCheckoutDlg::~CCheckoutDlg()
{
	if (m_pLogDlg)
		delete m_pLogDlg;
}

void CCheckoutDlg::DoDataExchange(CDataExchange* pDX)
{
	CResizableStandAloneDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_URLCOMBO, m_URLCombo);
	DDX_Control(pDX, IDC_REVISION_NUM, m_editRevision);
	DDX_Control(pDX, IDC_BROWSE, m_butBrowse);
	DDX_Text(pDX, IDC_REVISION_NUM, m_sRevision);
	DDX_Text(pDX, IDC_CHECKOUTDIRECTORY, m_strCheckoutDirectory);
	DDX_Check(pDX, IDC_NOEXTERNALS, m_bNoExternals);
	DDX_Control(pDX, IDC_CHECKOUTDIRECTORY, m_cCheckoutEdit);
	DDX_Control(pDX, IDC_DEPTH, m_depthCombo);
}


BEGIN_MESSAGE_MAP(CCheckoutDlg, CResizableStandAloneDialog)
	ON_REGISTERED_MESSAGE(WM_REVSELECTED, OnRevSelected)
	ON_BN_CLICKED(IDC_BROWSE, OnBnClickedBrowse)
	ON_BN_CLICKED(IDC_CHECKOUTDIRECTORY_BROWSE, OnBnClickedCheckoutdirectoryBrowse)
	ON_EN_CHANGE(IDC_CHECKOUTDIRECTORY, OnEnChangeCheckoutdirectory)
	ON_BN_CLICKED(IDHELP, OnBnClickedHelp)
	ON_BN_CLICKED(IDC_SHOW_LOG, OnBnClickedShowlog)
	ON_EN_CHANGE(IDC_REVISION_NUM, &CCheckoutDlg::OnEnChangeRevisionNum)
	ON_CBN_EDITCHANGE(IDC_URLCOMBO, &CCheckoutDlg::OnCbnEditchangeUrlcombo)
END_MESSAGE_MAP()

BOOL CCheckoutDlg::OnInitDialog()
{
	CResizableStandAloneDialog::OnInitDialog();

	AdjustControlSize(IDC_NOEXTERNALS);
	AdjustControlSize(IDC_REVISION_HEAD);
	AdjustControlSize(IDC_REVISION_N);

	m_sCheckoutDirOrig = m_strCheckoutDirectory;

	CString sUrlSave = m_URL;
	m_URLCombo.SetURLHistory(TRUE);
	m_bAutoCreateTargetName = FALSE;
	m_URLCombo.LoadHistory(_T("Software\\TortoiseSVN\\History\\repoURLS"), _T("url"));
	m_bAutoCreateTargetName = !PathIsDirectoryEmpty(m_sCheckoutDirOrig);
	m_URLCombo.SetCurSel(0);

	m_depthCombo.AddString(CString(MAKEINTRESOURCE(IDS_SVN_DEPTH_INFINITE)));
	m_depthCombo.AddString(CString(MAKEINTRESOURCE(IDS_SVN_DEPTH_IMMEDIATE)));
	m_depthCombo.AddString(CString(MAKEINTRESOURCE(IDS_SVN_DEPTH_FILES)));
	m_depthCombo.AddString(CString(MAKEINTRESOURCE(IDS_SVN_DEPTH_EMPTY)));
	m_depthCombo.SetCurSel(0);

	// set radio buttons according to the revision
	SetRevision(Revision);

	m_editRevision.SetWindowText(_T(""));

	if (!sUrlSave.IsEmpty())
	{
		SetDlgItemText(IDC_CHECKOUTDIRECTORY, m_sCheckoutDirOrig);
		m_URLCombo.SetWindowText(sUrlSave);
	}
	m_tooltips.Create(this);
	m_tooltips.AddTool(IDC_CHECKOUTDIRECTORY, IDS_CHECKOUT_TT_DIR);

	SHAutoComplete(GetDlgItem(IDC_CHECKOUTDIRECTORY)->m_hWnd, SHACF_FILESYSTEM);

	if (!Revision.IsHead())
	{
		CString temp;
		temp.Format(_T("%ld"), (LONG)Revision);
		m_editRevision.SetWindowText(temp);
		CheckRadioButton(IDC_REVISION_HEAD, IDC_REVISION_N, IDC_REVISION_N);
	}
	if (m_strCheckoutDirectory.IsEmpty())
	{
		CRegString lastCheckoutPath = CRegString(_T("Software\\TortoiseSVN\\History\\lastCheckoutPath"));
		m_strCheckoutDirectory = lastCheckoutPath;
		if (m_strCheckoutDirectory.GetLength() <= 2)
			m_strCheckoutDirectory += _T("\\");
	}
	UpdateData(FALSE);

	AddAnchor(IDC_GROUPTOP, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_URLOFREPO, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_URLCOMBO, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_BROWSE, TOP_RIGHT);
	AddAnchor(IDC_EXPORT_CHECKOUTDIR, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_CHECKOUTDIRECTORY, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_CHECKOUTDIRECTORY_BROWSE, TOP_RIGHT);
	AddAnchor(IDC_GROUPMIDDLE, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_DEPTH, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_NOEXTERNALS, TOP_LEFT);
	AddAnchor(IDC_GROUPBOTTOM, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_REVISION_HEAD, TOP_LEFT);
	AddAnchor(IDC_REVISION_N, TOP_LEFT);
	AddAnchor(IDC_REVISION_NUM, TOP_LEFT);
	AddAnchor(IDC_SHOW_LOG, TOP_LEFT);
	AddAnchor(IDOK, TOP_RIGHT);
	AddAnchor(IDCANCEL, TOP_RIGHT);
	AddAnchor(IDHELP, TOP_RIGHT);

	// prevent resizing vertically
	CRect rect;
	GetWindowRect(&rect);
	CSize size;
	size.cx = MAXLONG;
	size.cy = rect.Height();
	SetMaxTrackSize(size);

	if ((m_pParentWnd==NULL)&&(hWndExplorer))
		CenterWindow(CWnd::FromHandle(hWndExplorer));
	EnableSaveRestore(_T("CheckoutDlg"));
	return TRUE;
}

void CCheckoutDlg::OnOK()
{
	if (!UpdateData(TRUE))
		return; // don't dismiss dialog (error message already shown by MFC framework)

	CTSVNPath checkoutDirectory;
	if (::PathIsRelative(m_strCheckoutDirectory))
	{
		checkoutDirectory = CTSVNPath(sOrigCWD);
		checkoutDirectory.AppendPathString(_T("\\") + m_strCheckoutDirectory);
		m_strCheckoutDirectory = checkoutDirectory.GetWinPathString();
	}
	else
		checkoutDirectory = CTSVNPath(m_strCheckoutDirectory);
	if (!checkoutDirectory.IsValidOnWindows())
	{
		ShowBalloon(IDC_CHECKOUTDIRECTORY, IDS_ERR_NOVALIDPATH);
		return;
	}

	if (GetCheckedRadioButton(IDC_REVISION_HEAD, IDC_REVISION_N) == IDC_REVISION_HEAD)
	{
		Revision = SVNRev(_T("HEAD"));
	}
	else
		Revision = SVNRev(m_sRevision);
	if (!Revision.IsValid())
	{
		ShowBalloon(IDC_REVISION_NUM, IDS_ERR_INVALIDREV);
		return;
	}

	bool bAutoCreateTargetName = m_bAutoCreateTargetName;
	m_bAutoCreateTargetName = false;
	m_URLCombo.SaveHistory();
	m_URL = m_URLCombo.GetString();

	if (!SVN::PathIsURL(CTSVNPath(m_URL)))
	{
		ShowBalloon(IDC_URLCOMBO, IDS_ERR_MUSTBEURL, IDI_ERROR);
		m_bAutoCreateTargetName = bAutoCreateTargetName;
		return;
	}

	if (m_strCheckoutDirectory.IsEmpty())
	{
		return;			//don't dismiss the dialog
	}
	if (!PathFileExists(m_strCheckoutDirectory))
	{
		CString temp;
		temp.Format(IDS_WARN_FOLDERNOTEXIST, (LPCTSTR)m_strCheckoutDirectory);
		if (CMessageBox::Show(this->m_hWnd, temp, _T("TortoiseSVN"), MB_YESNO | MB_ICONQUESTION) == IDYES)
		{
			CPathUtils::MakeSureDirectoryPathExists(m_strCheckoutDirectory);
		}
		else
		{
			m_bAutoCreateTargetName = bAutoCreateTargetName;
			return;		//don't dismiss the dialog
		}
	}
	if (!PathIsDirectoryEmpty(m_strCheckoutDirectory))
	{
		CString message;
		message.Format(CString(MAKEINTRESOURCE(IDS_WARN_FOLDERNOTEMPTY)),(LPCTSTR)m_strCheckoutDirectory);
		if (CMessageBox::Show(this->m_hWnd, message, _T("TortoiseSVN"), MB_YESNO | MB_ICONQUESTION) != IDYES)
		{
			m_bAutoCreateTargetName = bAutoCreateTargetName;
			return;		//don't dismiss the dialog
		}
	}
	switch (m_depthCombo.GetCurSel())
	{
	case 0:
		m_depth = svn_depth_infinity;
		break;
	case 1:
		m_depth = svn_depth_immediates;
		break;
	case 2:
		m_depth = svn_depth_files;
		break;
	case 3:
		m_depth = svn_depth_empty;
		break;
	default:
		m_depth = svn_depth_empty;
		break;
	}
	UpdateData(FALSE);
	CRegString lastCheckoutPath = CRegString(_T("Software\\TortoiseSVN\\History\\lastCheckoutPath"));
	lastCheckoutPath = m_strCheckoutDirectory.Left(m_strCheckoutDirectory.ReverseFind('\\'));
	CResizableStandAloneDialog::OnOK();
}

void CCheckoutDlg::OnBnClickedBrowse()
{
	m_tooltips.Pop();	// hide the tooltips
	SVNRev rev;
	UpdateData();
	if (GetCheckedRadioButton(IDC_REVISION_HEAD, IDC_REVISION_N) == IDC_REVISION_HEAD)
	{
		rev = SVNRev(_T("HEAD"));
	}
	else
		rev = SVNRev(m_sRevision);

	if (!rev.IsValid())
		rev = SVNRev::REV_HEAD;
	if (CAppUtils::BrowseRepository(m_URLCombo, this, rev))
	{
		SetRevision(rev);

		CRegString regDefCheckoutUrl(_T("Software\\TortoiseSVN\\DefaultCheckoutUrl"));
		CRegString regDefCheckoutPath(_T("Software\\TortoiseSVN\\DefaultCheckoutPath"));
		if (!CString(regDefCheckoutUrl).IsEmpty())
		{
			m_URL = m_URLCombo.GetString();
		}
		else
		{
			m_URLCombo.GetWindowText(m_URL);
			if (m_URL.IsEmpty())
				return;
		}
		CString name = CAppUtils::GetProjectNameFromURL(m_URL);
		if (CPathUtils::GetFileNameFromPath(m_strCheckoutDirectory).CompareNoCase(name))
			m_strCheckoutDirectory = m_sCheckoutDirOrig.TrimRight('\\')+_T('\\')+name;
		if (m_strCheckoutDirectory.IsEmpty())
		{
			CRegString lastCheckoutPath = CRegString(_T("Software\\TortoiseSVN\\History\\lastCheckoutPath"));
			m_strCheckoutDirectory = lastCheckoutPath;
			if (m_strCheckoutDirectory.GetLength() <= 2)
				m_strCheckoutDirectory += _T("\\");
		}
		UpdateData(FALSE);
	}
}

void CCheckoutDlg::OnBnClickedCheckoutdirectoryBrowse()
{
	m_tooltips.Pop();	// hide the tooltips
	//
	// Create a folder browser dialog. If the user selects OK, we should update
	// the local data members with values from the controls, copy the checkout
	// directory from the browse folder, then restore the local values into the
	// dialog controls.
	//
	CBrowseFolder browseFolder;
	browseFolder.m_style = BIF_EDITBOX | BIF_NEWDIALOGSTYLE | BIF_RETURNFSANCESTORS | BIF_RETURNONLYFSDIRS;
	CString strCheckoutDirectory = m_strCheckoutDirectory;
	if (browseFolder.Show(GetSafeHwnd(), strCheckoutDirectory) == CBrowseFolder::OK) 
	{
		UpdateData(TRUE);
		m_strCheckoutDirectory = strCheckoutDirectory;
		m_sCheckoutDirOrig = m_strCheckoutDirectory;
		m_bAutoCreateTargetName = !PathIsDirectoryEmpty(m_sCheckoutDirOrig);
		UpdateData(FALSE);
	}
}

BOOL CCheckoutDlg::PreTranslateMessage(MSG* pMsg)
{
	m_tooltips.RelayEvent(pMsg);
	return CResizableStandAloneDialog::PreTranslateMessage(pMsg);
}

void CCheckoutDlg::OnEnChangeCheckoutdirectory()
{
	UpdateData(TRUE);		
	DialogEnableWindow(IDOK, !m_strCheckoutDirectory.IsEmpty());
}

void CCheckoutDlg::OnBnClickedHelp()
{
	OnHelp();
}

void CCheckoutDlg::OnBnClickedShowlog()
{
	m_tooltips.Pop();	// hide the tooltips
	UpdateData(TRUE);
	m_URL = m_URLCombo.GetString();
	if ((m_pLogDlg)&&(m_pLogDlg->IsWindowVisible()))
		return;
	AfxGetApp()->DoWaitCursor(1);
	//now show the log dialog for working copy
	if (!m_URL.IsEmpty())
	{
		delete m_pLogDlg;
		m_pLogDlg = new CLogDlg();
		m_pLogDlg->SetParams(CTSVNPath(m_URL), SVNRev::REV_HEAD, SVNRev::REV_HEAD, 1, (int)(DWORD)CRegDWORD(_T("Software\\TortoiseSVN\\NumberOfLogs"), 100));
		m_pLogDlg->m_wParam = 1;
		m_pLogDlg->SetSelect(true);
		m_pLogDlg->m_pNotifyWindow = this;
		m_pLogDlg->Create(IDD_LOGMESSAGE, this);
		m_pLogDlg->ShowWindow(SW_SHOW);
	}
	AfxGetApp()->DoWaitCursor(-1);
}

LPARAM CCheckoutDlg::OnRevSelected(WPARAM /*wParam*/, LPARAM lParam)
{
	CString temp;
	temp.Format(_T("%ld"), lParam);
	SetDlgItemText(IDC_REVISION_NUM, temp);
	CheckRadioButton(IDC_REVISION_HEAD, IDC_REVISION_N, IDC_REVISION_N);
	return 0;
}

void CCheckoutDlg::OnEnChangeRevisionNum()
{
	UpdateData();
	if (m_sRevision.IsEmpty())
		CheckRadioButton(IDC_REVISION_HEAD, IDC_REVISION_N, IDC_REVISION_HEAD);
	else
		CheckRadioButton(IDC_REVISION_HEAD, IDC_REVISION_N, IDC_REVISION_N);
}

void CCheckoutDlg::SetRevision(const SVNRev& rev)
{
	if (rev.IsHead() || !rev.IsValid())
		CheckRadioButton(IDC_REVISION_HEAD, IDC_REVISION_N, IDC_REVISION_HEAD);
	else
	{
		CheckRadioButton(IDC_REVISION_HEAD, IDC_REVISION_N, IDC_REVISION_N);
		CString sRev;
		sRev.Format(_T("%ld"), (LONG)rev);
		SetDlgItemText(IDC_REVISION_NUM, sRev);
	}
}

void CCheckoutDlg::OnCbnEditchangeUrlcombo()
{
	if (!m_bAutoCreateTargetName)
		return;
	if (m_sCheckoutDirOrig.IsEmpty())
		return;
	// find out what to use as the checkout directory name
	UpdateData();
	m_URLCombo.GetWindowText(m_URL);
	if (m_URL.IsEmpty())
		return;
	CString tempURL = m_URL;
	CString name = CAppUtils::GetProjectNameFromURL(m_URL);
	if (CPathUtils::GetFileNameFromPath(m_strCheckoutDirectory).CompareNoCase(name))
		m_strCheckoutDirectory = m_sCheckoutDirOrig.TrimRight('\\')+_T('\\')+name;
	if (m_strCheckoutDirectory.IsEmpty())
	{
		CRegString lastCheckoutPath = CRegString(_T("Software\\TortoiseSVN\\History\\lastCheckoutPath"));
		m_strCheckoutDirectory = lastCheckoutPath;
		if (m_strCheckoutDirectory.GetLength() <= 2)
			m_strCheckoutDirectory += _T("\\");
	}
	UpdateData(FALSE);
}

