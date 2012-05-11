// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2012 - TortoiseGit
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
#include "UnicodeUtils.h"
#include "MessageBox.h"
#include "AppUtils.h"
#include "TempFile.h"
#include "SysImageList.h"
#include "IconMenu.h"
//#include "GitProperties.h"
#include "StringUtils.h"
#include "PathUtils.h"
#include "BrowseFolder.h"
#include ".\filediffdlg.h"
#include "gitdiff.h"
#include "LoglistCommonResource.h"
#include "LoglistUtils.h"
#include "BrowseRefsDlg.h"
#include "LogDlg.h"
#include "RefLogDlg.h"
#include "GitStatusListCtrl.h"
#include "FormatMessageWrapper.h"

#define ID_COMPARE 1
#define ID_BLAME 2
#define ID_SAVEAS 3
#define ID_EXPORT 4
#define ID_CLIPBOARD_PATH 5
#define ID_CLIPBOARD_ALL 6
#define ID_LOG 7

BOOL	CFileDiffDlg::m_bAscending = FALSE;
int		CFileDiffDlg::m_nSortedColumn = -1;


IMPLEMENT_DYNAMIC(CFileDiffDlg, CResizableStandAloneDialog)
CFileDiffDlg::CFileDiffDlg(CWnd* pParent /*=NULL*/)
	: CResizableStandAloneDialog(CFileDiffDlg::IDD, pParent),
	m_bBlame(false),
	m_pProgDlg(NULL),
	m_bCancelled(false)
{
	m_bLoadingRef=FALSE;
}

CFileDiffDlg::~CFileDiffDlg()
{
}

void CFileDiffDlg::DoDataExchange(CDataExchange* pDX)
{
	CResizableStandAloneDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_FILELIST, m_cFileList);
	DDX_Control(pDX, IDC_SWITCHLEFTRIGHT, m_SwitchButton);
	DDX_Control(pDX, IDC_REV1BTN, m_cRev1Btn);
	DDX_Control(pDX, IDC_REV2BTN, m_cRev2Btn);
	DDX_Control(pDX, IDC_FILTER, m_cFilter);
	DDX_Control(pDX, IDC_REV1EDIT, m_ctrRev1Edit);
	DDX_Control(pDX, IDC_REV2EDIT, m_ctrRev2Edit);
}


BEGIN_MESSAGE_MAP(CFileDiffDlg, CResizableStandAloneDialog)
	ON_NOTIFY(NM_DBLCLK, IDC_FILELIST, OnNMDblclkFilelist)
	ON_NOTIFY(LVN_GETINFOTIP, IDC_FILELIST, OnLvnGetInfoTipFilelist)
	ON_NOTIFY(NM_CUSTOMDRAW, IDC_FILELIST, OnNMCustomdrawFilelist)
	ON_WM_CONTEXTMENU()
	ON_WM_SETCURSOR()
	ON_EN_SETFOCUS(IDC_SECONDURL, &CFileDiffDlg::OnEnSetfocusSecondurl)
	ON_EN_SETFOCUS(IDC_FIRSTURL, &CFileDiffDlg::OnEnSetfocusFirsturl)
	ON_BN_CLICKED(IDC_SWITCHLEFTRIGHT, &CFileDiffDlg::OnBnClickedSwitchleftright)
	ON_NOTIFY(HDN_ITEMCLICK, 0, &CFileDiffDlg::OnHdnItemclickFilelist)
	ON_BN_CLICKED(IDC_REV1BTN, &CFileDiffDlg::OnBnClickedRev1btn)
	ON_BN_CLICKED(IDC_REV2BTN, &CFileDiffDlg::OnBnClickedRev2btn)
	ON_MESSAGE(WM_FILTEREDIT_CANCELCLICKED, OnClickedCancelFilter)
	ON_EN_CHANGE(IDC_FILTER, &CFileDiffDlg::OnEnChangeFilter)
	ON_WM_TIMER()
	ON_MESSAGE(ENAC_UPDATE, &CFileDiffDlg::OnEnUpdate)
	ON_MESSAGE(MSG_REF_LOADED, OnRefLoad)
END_MESSAGE_MAP()


void CFileDiffDlg::SetDiff(CTGitPath * path, GitRev rev1, GitRev rev2)
{
	if(path!=NULL)
	{
		m_path1 = *path;
		m_path2 = *path;
		m_sFilter = path->GetGitPathString();
	}
	m_rev1 = rev1;
	m_rev2 = rev2;
}

void CFileDiffDlg::SetDiff(CTGitPath * path, CString &hash1, CString &hash2)
{
	if(path!=NULL)
	{
		m_path1 = *path;
		m_path2 = *path;
		m_sFilter = path->GetGitPathString();
	}

	BYTE_VECTOR logout;

	if(hash1 == GIT_REV_ZERO)
	{
		m_rev1.m_CommitHash.Empty();
		m_rev1.GetSubject() = CString(MAKEINTRESOURCE(IDS_git_DEPTH_WORKING));
	}
	else
	{
		m_rev1.GetCommit(hash1);
	}

	logout.clear();

	if(hash2 == GIT_REV_ZERO)
	{
		m_rev2.m_CommitHash.Empty();
		m_rev2.GetSubject() = CString(MAKEINTRESOURCE(IDS_git_DEPTH_WORKING));
	}
	else
	{
		m_rev2.GetCommit(hash2);
	}
}

void CFileDiffDlg::SetDiff(CTGitPath * path, GitRev rev1)
{
	if(path!=NULL)
	{
		m_path1 = *path;
		m_path2 = *path;
		m_sFilter = path->GetGitPathString();
	}
	m_rev1 = rev1;
	m_rev2.m_CommitHash.Empty();
	m_rev2.GetSubject() = CString(MAKEINTRESOURCE(IDS_PROC_PREVIOUSVERSION));

	//this->GetDlgItem()->EnableWindow(FALSE);
}

BOOL CFileDiffDlg::OnInitDialog()
{
	CResizableStandAloneDialog::OnInitDialog();
	CString temp;

	CString sWindowTitle;
	GetWindowText(sWindowTitle);
	CAppUtils::SetWindowTitle(m_hWnd, g_Git.m_CurrentDir, sWindowTitle);

	this->m_ctrRev1Edit.Init();
	this->m_ctrRev2Edit.Init();

	m_tooltips.Create(this);
	m_tooltips.AddTool(IDC_SWITCHLEFTRIGHT, IDS_FILEDIFF_SWITCHLEFTRIGHT_TT);

	m_cFileList.SetRedraw(false);
	m_cFileList.DeleteAllItems();
	DWORD exStyle = LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_INFOTIP;
	m_cFileList.SetExtendedStyle(exStyle);

	m_nIconFolder = SYS_IMAGE_LIST().GetDirIconIndex();
	m_cFileList.SetImageList(&SYS_IMAGE_LIST(), LVSIL_SMALL);

	m_SwitchButton.SetImage((HICON)LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_SWITCHLEFTRIGHT), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR));
	m_SwitchButton.Invalidate();

	m_cFilter.SetCancelBitmaps(IDI_CANCELNORMAL, IDI_CANCELPRESSED);
	m_cFilter.SetInfoIcon(IDI_FILTEREDIT);
	temp.LoadString(IDS_FILEDIFF_FILTERCUE);
	temp = _T("   ")+temp;
	m_cFilter.SetCueBanner(temp);
	if (!m_sFilter.IsEmpty())
		m_cFilter.SetWindowText(m_sFilter);

	int c = ((CHeaderCtrl*)(m_cFileList.GetDlgItem(0)))->GetItemCount()-1;
	while (c>=0)
		m_cFileList.DeleteColumn(c--);

	temp.LoadString(IDS_FILEDIFF_FILE);
	m_cFileList.InsertColumn(0, temp);
	temp.LoadString(IDS_FILEDIFF_ACTION);
	m_cFileList.InsertColumn(1, temp);

	temp.LoadString(IDS_FILEDIFF_STATADD);
	m_cFileList.InsertColumn(2, temp);
	temp.LoadString(IDS_FILEDIFF_STATDEL);
	m_cFileList.InsertColumn(3, temp);

	int mincol = 0;
	int maxcol = ((CHeaderCtrl*)(m_cFileList.GetDlgItem(0)))->GetItemCount()-1;
	int col;
	for (col = mincol; col <= maxcol; col++)
	{
		m_cFileList.SetColumnWidth(col,LVSCW_AUTOSIZE_USEHEADER);
	}

	m_cFileList.SetRedraw(true);

	AddAnchor(IDC_DIFFSTATIC1, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_SWITCHLEFTRIGHT, TOP_RIGHT);
	AddAnchor(IDC_FIRSTURL, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_REV1BTN, TOP_RIGHT);
	//AddAnchor(IDC_DIFFSTATIC2, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_SECONDURL, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_REV2BTN, TOP_RIGHT);
	AddAnchor(IDC_FILTER, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_FILELIST, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_REV1GROUP,TOP_LEFT,TOP_RIGHT);
	AddAnchor(IDC_REV2GROUP,TOP_LEFT,TOP_RIGHT);
	AddAnchor(IDC_REV1EDIT,TOP_LEFT);
	AddAnchor(IDC_REV2EDIT,TOP_LEFT);

	EnableSaveRestore(_T("FileDiffDlg"));

	if(this->m_strRev1.IsEmpty())
		this->m_ctrRev1Edit.SetWindowText(this->m_rev1.m_CommitHash.ToString());
	else
	{
		if(m_rev1.GetCommit(this->m_strRev1))
		{
			CString msg;
			msg.Format(IDS_PROC_REFINVALID, this->m_strRev1);
			this->m_FileListText += msg;
		}

		this->m_ctrRev1Edit.SetWindowText(m_strRev1);
	}

	if(this->m_strRev2.IsEmpty())
		this->m_ctrRev2Edit.SetWindowText(this->m_rev2.m_CommitHash.ToString());
	else
	{
		if(m_rev2.GetCommit(this->m_strRev2))
		{
			CString msg;
			msg.Format(IDS_PROC_REFINVALID, this->m_strRev2);
			this->m_FileListText += msg;
		}

		this->m_ctrRev2Edit.SetWindowText(m_strRev2);
	}

	SetURLLabels();

	InterlockedExchange(&m_bThreadRunning, TRUE);
	if (AfxBeginThread(DiffThreadEntry, this)==NULL)
	{
		InterlockedExchange(&m_bThreadRunning, FALSE);
		CMessageBox::Show(NULL, IDS_ERR_THREADSTARTFAILED, IDS_APPNAME, MB_OK | MB_ICONERROR);
	}

	InterlockedExchange(&m_bLoadingRef, TRUE);
	if (AfxBeginThread(LoadRefThreadEntry, this)==NULL)
	{
		InterlockedExchange(&m_bLoadingRef, FALSE);
		CMessageBox::Show(NULL, IDS_ERR_THREADSTARTFAILED, IDS_APPNAME, MB_OK | MB_ICONERROR);
	}

	this->m_cRev1Btn.AddEntry(CString(MAKEINTRESOURCE(IDS_REFBROWSE)));
	this->m_cRev1Btn.AddEntry(CString(MAKEINTRESOURCE(IDS_LOG)));
	this->m_cRev1Btn.AddEntry(CString(MAKEINTRESOURCE(IDS_REFLOG)));

	this->m_cRev2Btn.AddEntry(CString(MAKEINTRESOURCE(IDS_REFBROWSE)));
	this->m_cRev2Btn.AddEntry(CString(MAKEINTRESOURCE(IDS_LOG)));
	this->m_cRev2Btn.AddEntry(CString(MAKEINTRESOURCE(IDS_REFLOG)));

	// Start with focus on file list
	GetDlgItem(IDC_FILELIST)->SetFocus();

	if(m_rev2.m_CommitHash.IsEmpty())
		m_SwitchButton.EnableWindow(FALSE);

	KillTimer(IDT_INPUT);
	return FALSE;
}

#if 0
svn_error_t* CFileDiffDlg::DiffSummarizeCallback(const CTGitPath& path,
												 svn_client_diff_summarize_kind_t kind,
												 bool propchanged, svn_node_kind_t node)
{
	CTGitPath* fd;
	fd.path = path;
	fd.kind = kind;
	fd.node = node;
	fd.propchanged = propchanged;
	m_arFileList.push_back(fd);
	return Git_NO_ERROR;
}
#endif

UINT CFileDiffDlg::DiffThreadEntry(LPVOID pVoid)
{
	return ((CFileDiffDlg*)pVoid)->DiffThread();
}

UINT CFileDiffDlg::DiffThread()
{
	RefreshCursor();
	m_cFileList.ShowText(CString(MAKEINTRESOURCE(IDS_FILEDIFF_WAIT)));
	m_cFileList.DeleteAllItems();
	m_arFileList.Clear();
	EnableInputControl(false);
#if 0
	bool bSuccess = true;
	if (m_bDoPegDiff)
	{
//		bSuccess = DiffSummarizePeg(m_path1, m_peg, m_rev1, m_rev2, m_depth, m_bIgnoreancestry);
	}
	else
	{
//		bSuccess = DiffSummarize(m_path1, m_rev1, m_path2, m_rev2, m_depth, m_bIgnoreancestry);
	}
//	if (!bSuccess)
//	{
//		m_cFileList.ShowText(GetLastErrorMessage());
//		InterlockedExchange(&m_bThreadRunning, FALSE);
//		return 0;
//	}
#endif

	if( m_rev1.m_CommitHash.IsEmpty() || m_rev2.m_CommitHash.IsEmpty())
		g_Git.RefreshGitIndex();

	g_Git.GetCommitDiffList(m_rev1.m_CommitHash.ToString(),m_rev2.m_CommitHash.ToString(),m_arFileList);

	CString sFilterText;
	m_cFilter.GetWindowText(sFilterText);
	m_cFileList.SetRedraw(false);
	Filter(sFilterText);
	if (m_arFileList.GetCount()>0)
	{
		// Highlight first entry in file list
		m_cFileList.SetSelectionMark(0);
		m_cFileList.SetItemState(0, LVIS_SELECTED, LVIS_SELECTED);
	}

	int mincol = 0;
	int maxcol = ((CHeaderCtrl*)(m_cFileList.GetDlgItem(0)))->GetItemCount()-1;
	int col;
	for (col = mincol; col <= maxcol; col++)
	{
		m_cFileList.SetColumnWidth(col,LVSCW_AUTOSIZE_USEHEADER);
	}

	m_cFileList.ClearText();
	m_cFileList.SetRedraw(true);

	InterlockedExchange(&m_bThreadRunning, FALSE);
	InvalidateRect(NULL);
	RefreshCursor();
	EnableInputControl(true);
	return 0;
}

int CFileDiffDlg::AddEntry(const CTGitPath * fd)
{
	int ret = -1;
	if (fd)
	{
		int index = m_cFileList.GetItemCount();

		int icon_idx = 0;
//		if (fd->node == svn_node_dir)
//				icon_idx = m_nIconFolder;
//		else
		{
			icon_idx = SYS_IMAGE_LIST().GetPathIconIndex(fd->GetGitPathString());
		}

		ret = m_cFileList.InsertItem(index, fd->GetGitPathString(), icon_idx);
		m_cFileList.SetItemText(index, 1, ((CTGitPath*)fd)->GetActionName());
		m_cFileList.SetItemText(index, 2, ((CTGitPath*)fd)->m_StatAdd);
		m_cFileList.SetItemText(index, 3, ((CTGitPath*)fd)->m_StatDel);
	}
	return ret;
}

void CFileDiffDlg::EnableInputControl(bool b)
{
	this->m_ctrRev1Edit.EnableWindow(b);
	this->m_ctrRev2Edit.EnableWindow(b);
	this->m_cRev1Btn.EnableWindow(b);
	this->m_cRev2Btn.EnableWindow(b);
	m_cFilter.EnableWindow(b);
	m_SwitchButton.EnableWindow(b);
}

void CFileDiffDlg::DoDiff(int selIndex, bool blame)
{
	CGitDiff diff;
	CTGitPath* fd = m_arFilteredList[selIndex];
	diff.Diff(fd, fd,this->m_rev1.m_CommitHash.ToString(), this->m_rev2.m_CommitHash.ToString(), blame, FALSE);
}


void CFileDiffDlg::OnNMDblclkFilelist(NMHDR *pNMHDR, LRESULT *pResult)
{
	*pResult = 0;
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	int selIndex = pNMLV->iItem;
	if (selIndex < 0)
		return;
	if (selIndex >= (int)m_arFilteredList.size())
		return;

	DoDiff(selIndex, m_bBlame);
}

void CFileDiffDlg::OnLvnGetInfoTipFilelist(NMHDR *pNMHDR, LRESULT *pResult)
{

	LPNMLVGETINFOTIP pGetInfoTip = reinterpret_cast<LPNMLVGETINFOTIP>(pNMHDR);
	if (pGetInfoTip->iItem >= (int)m_arFilteredList.size())
		return;

	CString path = m_path1.GetGitPathString() + _T("/") + m_arFilteredList[pGetInfoTip->iItem]->GetGitPathString();
	if (pGetInfoTip->cchTextMax > path.GetLength())
			_tcsncpy_s(pGetInfoTip->pszText, pGetInfoTip->cchTextMax, path, pGetInfoTip->cchTextMax);

	*pResult = 0;
}

void CFileDiffDlg::OnNMCustomdrawFilelist(NMHDR *pNMHDR, LRESULT *pResult)
{
	NMLVCUSTOMDRAW* pLVCD = reinterpret_cast<NMLVCUSTOMDRAW*>( pNMHDR );
	// Take the default processing unless we set this to something else below.
	*pResult = CDRF_DODEFAULT;

	// First thing - check the draw stage. If it's the control's prepaint
	// stage, then tell Windows we want messages for every item.

	if ( CDDS_PREPAINT == pLVCD->nmcd.dwDrawStage )
	{
		*pResult = CDRF_NOTIFYITEMDRAW;
	}
	else if ( CDDS_ITEMPREPAINT == pLVCD->nmcd.dwDrawStage )
	{
		// This is the prepaint stage for an item. Here's where we set the
		// item's text color. Our return value will tell Windows to draw the
		// item itself, but it will use the new color we set here.

		// Tell Windows to paint the control itself.
		*pResult = CDRF_DODEFAULT;

		COLORREF crText = GetSysColor(COLOR_WINDOWTEXT);

		if (m_arFilteredList.size() > pLVCD->nmcd.dwItemSpec)
		{
			CTGitPath * fd = m_arFilteredList[pLVCD->nmcd.dwItemSpec];
			switch (fd->m_Action)
			{
			case CTGitPath::LOGACTIONS_ADDED:
				crText = m_colors.GetColor(CColors::Added);
				break;
			case CTGitPath::LOGACTIONS_DELETED:
				crText = m_colors.GetColor(CColors::Deleted);
				break;
			case CTGitPath::LOGACTIONS_MODIFIED:
				crText = m_colors.GetColor(CColors::Modified);
				break;
			//case svn_client_diff_summarize_kind_normal:
			default:
			//if (fd.propchanged)
				crText = m_colors.GetColor(CColors::PropertyChanged);
				break;
			}
		}
		// Store the color back in the NMLVCUSTOMDRAW struct.
		pLVCD->clrText = crText;
	}
}

UINT CFileDiffDlg::LoadRefThread()
{
	g_Git.GetBranchList(m_Reflist,NULL,CGit::BRANCH_ALL_F);
	g_Git.GetTagList(m_Reflist);

	this->PostMessage(MSG_REF_LOADED);
	InterlockedExchange(&m_bLoadingRef, FALSE);
	return 0;
}

void CFileDiffDlg::OnContextMenu(CWnd* pWnd, CPoint point)
{
	if ((pWnd==0)||(pWnd != &m_cFileList))
		return;
	if (m_cFileList.GetSelectedCount() == 0)
		return;
	// if the context menu is invoked through the keyboard, we have to use
	// a calculated position on where to anchor the menu on
	if ((point.x == -1) && (point.y == -1))
	{
		CRect rect;
		m_cFileList.GetItemRect(m_cFileList.GetSelectionMark(), &rect, LVIR_LABEL);
		m_cFileList.ClientToScreen(&rect);
		point = rect.CenterPoint();
	}
	CIconMenu popup;
	if (popup.CreatePopupMenu())
	{
		popup.AppendMenuIcon(ID_COMPARE, IDS_LOG_POPUP_COMPARETWO, IDI_DIFF);
		popup.AppendMenuIcon(ID_BLAME, IDS_FILEDIFF_POPBLAME, IDI_BLAME);
		popup.AppendMenuIcon(ID_LOG, IDS_FILEDIFF_LOG, IDI_LOG);
		popup.AppendMenu(MF_SEPARATOR, NULL);
		popup.AppendMenuIcon(ID_EXPORT, IDS_FILEDIFF_POPEXPORT, IDI_EXPORT);
		popup.AppendMenu(MF_SEPARATOR, NULL);
		popup.AppendMenuIcon(ID_SAVEAS, IDS_FILEDIFF_POPSAVELIST, IDI_SAVEAS);
		popup.AppendMenuIcon(ID_CLIPBOARD_PATH, IDS_STATUSLIST_CONTEXT_COPY, IDI_COPYCLIP);
		popup.AppendMenuIcon(ID_CLIPBOARD_ALL, IDS_STATUSLIST_CONTEXT_COPYEXT, IDI_COPYCLIP);

		int cmd = popup.TrackPopupMenu(TPM_RETURNCMD | TPM_LEFTALIGN | TPM_NONOTIFY, point.x, point.y, this, 0);
		m_bCancelled = false;
		switch (cmd)
		{
		case ID_COMPARE:
			{
				POSITION pos = m_cFileList.GetFirstSelectedItemPosition();
				while (pos)
				{
					int index = m_cFileList.GetNextSelectedItem(pos);
					DoDiff(index, false);
				}
			}
			break;
		case ID_BLAME:
			{
				POSITION pos = m_cFileList.GetFirstSelectedItemPosition();
				while (pos)
				{
					int index = m_cFileList.GetNextSelectedItem(pos);
					CAppUtils::LaunchTortoiseBlame(m_arFilteredList[index]->GetWinPathString(), m_rev1.m_CommitHash.ToString());
				}
			}
			break;
		case ID_LOG:
			{
				POSITION pos = m_cFileList.GetFirstSelectedItemPosition();
				while (pos)
				{
					int index = m_cFileList.GetNextSelectedItem(pos);
					CString cmd = _T("/command:log");
					cmd += _T(" /path:\"")+m_arFilteredList[index]->GetWinPathString()+_T("\" ");
					cmd += _T(" /endrev:")+m_rev1.m_CommitHash.ToString();
					CAppUtils::RunTortoiseProc(cmd);
				}
			}
			break;
		case ID_SAVEAS:
			{
				if (m_cFileList.GetSelectedCount() > 0)
				{
					CString temp;
					CTGitPath savePath;
					CString pathSave;
					if (!CAppUtils::FileOpenSave(pathSave, NULL, IDS_REPOBROWSE_SAVEAS, IDS_COMMONFILEFILTER, false, m_hWnd))
					{
						break;
					}
					savePath = CTGitPath(pathSave);

					// now open the selected file for writing
					try
					{
						CStdioFile file(savePath.GetWinPathString(), CFile::typeBinary | CFile::modeReadWrite | CFile::modeCreate);
//						temp.Format(IDS_FILEDIFF_CHANGEDLISTINTRO, (LPCTSTR)m_path1.GetGitPathString(), (LPCTSTR)m_rev1.ToString(), (LPCTSTR)m_path2.GetGitPathString(), (LPCTSTR)m_rev2.ToString());
						file.WriteString(temp + _T("\n"));
						POSITION pos = m_cFileList.GetFirstSelectedItemPosition();
						while (pos)
						{
							int index = m_cFileList.GetNextSelectedItem(pos);
							CTGitPath* fd = m_arFilteredList[index];
							file.WriteString(fd->GetGitPathString());
							file.WriteString(_T("\n"));
						}
						file.Close();
					}
					catch (CFileException* pE)
					{
						pE->ReportError();
					}
				}
			}
			break;
		case ID_CLIPBOARD_PATH:
			{
				CopySelectionToClipboard();
			}
			break;

		case ID_CLIPBOARD_ALL:
			{
				CopySelectionToClipboard(TRUE);
			}
			break;
		case ID_EXPORT:
			{
				// export all changed files to a folder
				CBrowseFolder browseFolder;
				browseFolder.m_style = BIF_EDITBOX | BIF_NEWDIALOGSTYLE | BIF_RETURNFSANCESTORS | BIF_RETURNONLYFSDIRS;
				if (browseFolder.Show(GetSafeHwnd(), m_strExportDir) == CBrowseFolder::OK)
				{
					POSITION pos = m_cFileList.GetFirstSelectedItemPosition();
					while (pos)
					{
						int index = m_cFileList.GetNextSelectedItem(pos);
						CTGitPath* fd = m_arFilteredList[index];
						// we cannot export directories or folders
						if (fd->m_Action == CTGitPath::LOGACTIONS_DELETED || fd->IsDirectory())
							continue;
						CAppUtils::CreateMultipleDirectory(m_strExportDir + _T("\\") + fd->GetDirectory().GetWinPathString());
						CString filename = m_strExportDir + _T("\\") + fd->GetWinPathString();
						if(m_rev1.m_CommitHash.ToString() == GIT_REV_ZERO)
						{
							if(!CopyFile(g_Git.m_CurrentDir + _T("\\") + fd->GetWinPath(), filename, false))
							{
								MessageBox(CFormatMessageWrapper(), _T("TortoiseGit"), MB_OK | MB_ICONERROR);
								return;
							}
						}
						else
						{
							if(g_Git.GetOneFile(m_rev1.m_CommitHash, *fd, filename))
							{
								CString out;
								out.Format(IDS_STATUSLIST_CHECKOUTFILEFAILED, fd->GetGitPathString(), m_rev1.m_CommitHash.ToString(), filename);
								if (CMessageBox::Show(NULL, out, _T("TortoiseGit"), 2, IDI_WARNING, CString(MAKEINTRESOURCE(IDS_IGNOREBUTTON)), CString(MAKEINTRESOURCE(IDS_ABORTBUTTON))) == 2)
									return;
							}
						}
					}
				}
			}

			break;

		}
	}
}

BOOL CFileDiffDlg::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	if (pWnd != &m_cFileList)
		return CResizableStandAloneDialog::OnSetCursor(pWnd, nHitTest, message);
	if (m_bThreadRunning == 0)
	{
		HCURSOR hCur = LoadCursor(NULL, MAKEINTRESOURCE(IDC_ARROW));
		SetCursor(hCur);
		return CResizableStandAloneDialog::OnSetCursor(pWnd, nHitTest, message);
	}
	HCURSOR hCur = LoadCursor(NULL, MAKEINTRESOURCE(IDC_WAIT));
	SetCursor(hCur);
	return TRUE;
}

void CFileDiffDlg::OnEnSetfocusFirsturl()
{
	GetDlgItem(IDC_FIRSTURL)->HideCaret();
}

void CFileDiffDlg::OnEnSetfocusSecondurl()
{
	GetDlgItem(IDC_SECONDURL)->HideCaret();
}


void CFileDiffDlg::OnBnClickedSwitchleftright()
{

	if (m_bThreadRunning)
		return;

#if 0
	CString sFilterString;
	m_cFilter.GetWindowText(sFilterString);

	m_cFileList.SetRedraw(false);
	m_cFileList.DeleteAllItems();
	for (int i=0; i<(int)m_arFileList.GetCount(); ++i)
	{
		CTGitPath fd = m_arFileList[i];
		if (fd.m_Action == CTGitPath::LOGACTIONS_ADDED)
			fd.m_Action = CTGitPath::LOGACTIONS_DELETED;
		else if (fd.m_Action == CTGitPath::LOGACTIONS_DELETED)
			fd.m_Action = CTGitPath::LOGACTIONS_ADDED;
		std::swap(fd.m_StatAdd, fd.m_StatDel);
		(CTGitPath&)m_arFileList[i] = fd;
	}
	Filter(sFilterString);
#endif

	m_cFileList.SetRedraw(true);
	CTGitPath path = m_path1;
	m_path1 = m_path2;
	m_path2 = path;
	GitRev rev = m_rev1;
	m_rev1 = m_rev2;
	m_rev2 = rev;

	CString str1,str2;
	this->m_ctrRev1Edit.GetWindowText(str1);
	this->m_ctrRev2Edit.GetWindowText(str2);

	this->m_ctrRev1Edit.SetWindowText(str2);
	this->m_ctrRev2Edit.SetWindowText(str1);

	SetURLLabels();
	//KillTimer(IDT_INPUT);
}

void CFileDiffDlg::SetURLLabels(int mask)
{

//	m_cRev1Btn.SetWindowText(m_rev1.m_CommitHash.ToString().Left(6));
//	m_cRev2Btn.SetWindowText(m_rev2.m_CommitHash.ToString().Left(6));

	if(mask &0x1)
	{
		SetDlgItemText(IDC_FIRSTURL, m_rev1.m_CommitHash.ToString().Left(8)+_T(": ")+m_rev1.GetSubject());
		m_tooltips.AddTool(IDC_FIRSTURL,
			CLoglistUtils::FormatDateAndTime(m_rev1.GetAuthorDate(), DATE_SHORTDATE, false) + _T("  ") + m_rev1.GetAuthorName());

	}

	if(mask &0x2)
	{
		SetDlgItemText(IDC_SECONDURL,m_rev2.m_CommitHash.ToString().Left(8)+_T(": ")+m_rev2.GetSubject());

		m_tooltips.AddTool(IDC_SECONDURL,
			CLoglistUtils::FormatDateAndTime(m_rev2.GetAuthorDate(), DATE_SHORTDATE, false) + _T("  ") + m_rev2.GetAuthorName());
	}

	this->GetDlgItem(IDC_REV2GROUP)->SetWindowText(CString(MAKEINTRESOURCE(IDS_PROC_FILEDIFF_VERSION2BASE)));
	this->GetDlgItem(IDC_REV1GROUP)->SetWindowText(CString(MAKEINTRESOURCE(IDS_PROC_FILEDIFF_VERSION1)));

	if( (mask&0x3) == 0x3)
		if(m_rev2.GetCommitterDate() > m_rev1.GetCommitterDate())
		{
			this->GetDlgItem(IDC_REV2GROUP)->SetWindowText(CString(MAKEINTRESOURCE(IDS_PROC_FILEDIFF_VERSION2BASENEWER)));
		}
		else
		{
			this->GetDlgItem(IDC_REV1GROUP)->SetWindowText(CString(MAKEINTRESOURCE(IDS_PROC_FILEDIFF_VERSION1NEWER)));
		}
}

void CFileDiffDlg::ClearURLabels(int mask)
{
	if(mask&0x1)
	{
		SetDlgItemText(IDC_FIRSTURL, _T(""));
		m_tooltips.AddTool(IDC_FIRSTURL,  _T(""));
	}

	if(mask&0x2)
	{
		SetDlgItemText(IDC_SECONDURL, _T(""));
		m_tooltips.AddTool(IDC_SECONDURL,  _T(""));
	}
}
BOOL CFileDiffDlg::PreTranslateMessage(MSG* pMsg)
{
	m_tooltips.RelayEvent(pMsg);
	if (pMsg->message == WM_KEYDOWN)
	{
		switch (pMsg->wParam)
		{
		case 'A':
			{
				if (GetAsyncKeyState(VK_CONTROL)&0x8000)
				{
					// select all entries
					for (int i=0; i<m_cFileList.GetItemCount(); ++i)
					{
						m_cFileList.SetItemState(i, LVIS_SELECTED, LVIS_SELECTED);
					}
					return TRUE;
				}
			}
			break;
		case 'C':
		case VK_INSERT:
			{
				if (GetAsyncKeyState(VK_CONTROL)&0x8000)
				{
					CopySelectionToClipboard();
					return TRUE;
				}
			}
			break;
		case '\r':
			{
				if (GetFocus() == GetDlgItem(IDC_FILELIST))
				{
					// Return pressed in file list. Show diff, as for double click
					int selIndex = m_cFileList.GetSelectionMark();
					if ((selIndex >= 0) && (selIndex < (int)m_arFileList.GetCount()))
						DoDiff(selIndex, m_bBlame);
					return TRUE;
				}
			}
			break;
		}
	}
	return __super::PreTranslateMessage(pMsg);
}

void CFileDiffDlg::OnCancel()
{
	if (m_bThreadRunning)
	{
		m_bCancelled = true;
		return;
	}
	__super::OnCancel();
}

void CFileDiffDlg::OnHdnItemclickFilelist(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMHEADER phdr = reinterpret_cast<LPNMHEADER>(pNMHDR);
	if (m_bThreadRunning)
		return;

	if (m_nSortedColumn == phdr->iItem)
		m_bAscending = !m_bAscending;
	else
		m_bAscending = TRUE;
	m_nSortedColumn = phdr->iItem;
	Sort();

	CString temp;
	m_cFileList.SetRedraw(FALSE);
	m_cFileList.DeleteAllItems();
	m_cFilter.GetWindowText(temp);
	Filter(temp);

	CHeaderCtrl * pHeader = m_cFileList.GetHeaderCtrl();
	HDITEM HeaderItem = {0};
	HeaderItem.mask = HDI_FORMAT;
	for (int i=0; i<pHeader->GetItemCount(); ++i)
	{
		pHeader->GetItem(i, &HeaderItem);
		HeaderItem.fmt &= ~(HDF_SORTDOWN | HDF_SORTUP);
		pHeader->SetItem(i, &HeaderItem);
	}
	pHeader->GetItem(m_nSortedColumn, &HeaderItem);
	HeaderItem.fmt |= (m_bAscending ? HDF_SORTUP : HDF_SORTDOWN);
	pHeader->SetItem(m_nSortedColumn, &HeaderItem);

	m_cFileList.SetRedraw(TRUE);

	*pResult = 0;
}

void CFileDiffDlg::Sort()
{
	if(m_arFileList.GetCount() < 2)
	{
		return;
	}

	std::sort(m_arFileList.m_paths.begin(), m_arFileList.m_paths.end(), &CFileDiffDlg::SortCompare);
}

bool CFileDiffDlg::SortCompare(const CTGitPath& Data1, const CTGitPath& Data2)
{
	int result = 0;
	int d1, d2;
	switch (m_nSortedColumn)
	{
	case 0:		//path column
		result = Data1.GetWinPathString().Compare(Data2.GetWinPathString());
		break;
	case 1:		//action column
		result = Data1.m_Action - Data2.m_Action;
		break;
	case 2:
		d1 = CSorter::A2L(Data1.m_StatAdd);
		d2 = CSorter::A2L(Data2.m_StatAdd);
		result = d1 - d2;
		break;
	case 3:
		d1 = CSorter::A2L(Data1.m_StatDel);;
		d2 = CSorter::A2L(Data2.m_StatDel);
		result = d1 - d2;
		break;
	default:
		break;
	}

	if (!m_bAscending)
		result = -result;
	return result < 0;
}


void CFileDiffDlg::OnBnClickedRev1btn()
{
	ClickRevButton(&this->m_cRev1Btn,&this->m_rev1, &this->m_ctrRev1Edit);
}

void CFileDiffDlg::ClickRevButton(CMenuButton *button, GitRev *rev, CACEdit *edit)
{
	int entry=button->GetCurrentEntry();
	if(entry == 0) /* Browse Refence*/
	{
		{
			CString str = CBrowseRefsDlg::PickRef();
			if(str.IsEmpty())
				return;

			if(FillRevFromString(rev,str))
				return;

			edit->SetWindowText(str);
		}
	}

	if(entry == 1) /*Log*/
	{
		CLogDlg dlg;
		dlg.SetSelect(true);
		if(dlg.DoModal() == IDOK)
		{
			if( dlg.GetSelectedHash().IsEmpty() )
				return;

			if(FillRevFromString(rev,dlg.GetSelectedHash()))
				return;

			edit->SetWindowText(dlg.GetSelectedHash());

		}
		else
			return;
	}

	if(entry == 2) /*RefLog*/
	{
		CRefLogDlg dlg;
		if(dlg.DoModal() == IDOK)
		{
			if(FillRevFromString(rev,dlg.m_SelectedHash))
				return;

			edit->SetWindowText(dlg.m_SelectedHash);

		}
		else
			return;
	}

	SetURLLabels();

	InterlockedExchange(&m_bThreadRunning, TRUE);
	if (AfxBeginThread(DiffThreadEntry, this)==NULL)
	{
		InterlockedExchange(&m_bThreadRunning, FALSE);
		CMessageBox::Show(NULL, IDS_ERR_THREADSTARTFAILED, IDS_APPNAME, MB_OK | MB_ICONERROR);
	}
	KillTimer(IDT_INPUT);
}

void CFileDiffDlg::OnBnClickedRev2btn()
{
	ClickRevButton(&this->m_cRev2Btn,&this->m_rev2, &this->m_ctrRev2Edit);
}

LRESULT CFileDiffDlg::OnClickedCancelFilter(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
	if (m_bThreadRunning)
	{
		SetTimer(IDT_FILTER, 1000, NULL);
		return 0L;
	}

	KillTimer(IDT_FILTER);

	m_cFileList.SetRedraw(FALSE);
	m_arFilteredList.clear();
	m_cFileList.DeleteAllItems();

	Filter(_T(""));

	m_cFileList.SetRedraw(TRUE);
	return 0L;
}

void CFileDiffDlg::OnEnChangeFilter()
{
	SetTimer(IDT_FILTER, 1000, NULL);
}

void CFileDiffDlg::OnTimer(UINT_PTR nIDEvent)
{
	if (m_bThreadRunning)
		return;

	if( nIDEvent == IDT_FILTER)
	{
		CString sFilterText;
		KillTimer(IDT_FILTER);
		m_cFilter.GetWindowText(sFilterText);

		m_cFileList.SetRedraw(FALSE);
		m_cFileList.DeleteAllItems();

		Filter(sFilterText);

		m_cFileList.SetRedraw(TRUE);

		__super::OnTimer(nIDEvent);
	}

	if( nIDEvent == IDT_INPUT)
	{
		KillTimer(IDT_INPUT);
		TRACE(_T("Input Timer\r\n"));

		GitRev gitrev;
		CString str;
		int mask = 0;
		this->m_ctrRev1Edit.GetWindowText(str);
		if( !gitrev.GetCommit(str) )
		{
			this->m_rev1=gitrev;
			mask |= 0x1;
		}

		this->m_ctrRev2Edit.GetWindowText(str);

		if( !gitrev.GetCommit(str) )
		{
			this->m_rev2=gitrev;
			mask |= 0x2;
		}

		this->SetURLLabels(mask);

		if(mask == 0x3)
		{

			InterlockedExchange(&m_bThreadRunning, TRUE);
			if (AfxBeginThread(DiffThreadEntry, this)==NULL)
			{
				InterlockedExchange(&m_bThreadRunning, FALSE);
				CMessageBox::Show(NULL, IDS_ERR_THREADSTARTFAILED, IDS_APPNAME, MB_OK | MB_ICONERROR);
			}
		}
	}
}

void CFileDiffDlg::Filter(CString sFilterText)
{
	sFilterText.MakeLower();

	m_arFilteredList.clear();

	for (int i=0;i<m_arFileList.GetCount();i++)
	{
		CString sPath = m_arFileList[i].GetGitPathString();
		sPath.MakeLower();
		if (sPath.Find(sFilterText) >= 0)
		{
			m_arFilteredList.push_back((CTGitPath*)&(m_arFileList[i]));
		}
	}
	for (std::vector<CTGitPath*>::const_iterator it = m_arFilteredList.begin(); it != m_arFilteredList.end(); ++it)
	{
		AddEntry(*it);
	}
}

void CFileDiffDlg::CopySelectionToClipboard(BOOL isFull)
{
	// copy all selected paths to the clipboard
	POSITION pos = m_cFileList.GetFirstSelectedItemPosition();
	int index;
	CString sTextForClipboard;
	while ((index = m_cFileList.GetNextSelectedItem(pos)) >= 0)
	{
		sTextForClipboard += m_cFileList.GetItemText(index, 0);
		sTextForClipboard += _T("\t");

		if(!isFull)
		{
			sTextForClipboard += _T("\r\n");

		}
		else
		{
			sTextForClipboard += m_cFileList.GetItemText(index, 1);
			sTextForClipboard += _T("\t");
			sTextForClipboard += m_cFileList.GetItemText(index, 2);
			sTextForClipboard += _T("\t");
			sTextForClipboard += m_cFileList.GetItemText(index, 3);
			sTextForClipboard += _T("\r\n");
		}
	}
	CStringUtils::WriteAsciiStringToClipboard(sTextForClipboard);
}


LRESULT CFileDiffDlg::OnRefLoad(WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(wParam);
	UNREFERENCED_PARAMETER(lParam);
	for(int i=0;i<m_Reflist.size();i++)
	{
		CString str=m_Reflist[i];

		if(str.Find(_T("remotes/")) == 0)
			str=str.Mid(8);

		m_ctrRev1Edit.AddSearchString(str);
		m_ctrRev2Edit.AddSearchString(str);
	}
	return 0;
}

BOOL CFileDiffDlg::DestroyWindow()
{
	return CResizableStandAloneDialog::DestroyWindow();
}

LRESULT CFileDiffDlg::OnEnUpdate(WPARAM /*wParam*/, LPARAM lParam)
{
	if(lParam == IDC_REV1EDIT)
	{
		OnTextUpdate(&this->m_ctrRev1Edit);
		ClearURLabels(1);
	}
	if(lParam == IDC_REV2EDIT)
	{
		OnTextUpdate(&this->m_ctrRev2Edit);
		ClearURLabels(1<<1);
	}
	return 0;
}

void CFileDiffDlg::OnTextUpdate(CACEdit * /*pEdit*/)
{
	SetTimer(IDT_INPUT, 1000, NULL);
	this->m_cFileList.ShowText(_T("Wait For input validate version"));
}
