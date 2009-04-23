// PathListCtrl.cpp : implementation file
//

#include "stdafx.h"
#include "TortoiseProc.h"
#include "PatchListCtrl.h"
#include "iconmenu.h"
#include "AppUtils.h"
#include "git.h"
#include "PathUtils.h"
#include "AppUtils.h"
// CPatchListCtrl

IMPLEMENT_DYNAMIC(CPatchListCtrl, CListCtrl)

CPatchListCtrl::CPatchListCtrl()
{
	m_ContextMenuMask=0xFFFFFFFF;
}

CPatchListCtrl::~CPatchListCtrl()
{
}


BEGIN_MESSAGE_MAP(CPatchListCtrl, CListCtrl)
	ON_NOTIFY_REFLECT(NM_DBLCLK, &CPatchListCtrl::OnNMDblclk)
	ON_WM_CONTEXTMENU()
END_MESSAGE_MAP()



// CPatchListCtrl message handlers



void CPatchListCtrl::OnNMDblclk(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	// TODO: Add your control notification handler code here
	CString path=GetItemText(pNMItemActivate->iItem,0);
	CTGitPath gitpath;
	gitpath.SetFromWin(path);
	
	CAppUtils::StartUnifiedDiffViewer(path,gitpath.GetFilename());

	*pResult = 0;

}

void CPatchListCtrl::OnContextMenu(CWnd* /*pWnd*/, CPoint point)
{
	// TODO: Add your message handler code here
	int selected=this->GetSelectedCount();
	int index=0;
	POSITION pos=this->GetFirstSelectedItemPosition();
	index=this->GetNextSelectedItem(pos);

	CIconMenu popup;
	if (popup.CreatePopupMenu())
	{
		if(selected == 1)
		{
			if( m_ContextMenuMask&GetMenuMask(MENU_VIEWPATCH))
				popup.AppendMenuIcon(MENU_VIEWPATCH, IDS_MENU_VIEWPATCH, 0);

			if( m_ContextMenuMask&GetMenuMask(MENU_VIEWWITHMERGE))
				popup.AppendMenuIcon(MENU_VIEWWITHMERGE, IDS_MENU_VIEWWITHMERGE, 0);

			popup.SetDefaultItem(MENU_VIEWPATCH, FALSE);
		}
		if(selected >= 1)
		{
			if( m_ContextMenuMask&GetMenuMask(MENU_SENDMAIL))
				popup.AppendMenuIcon(MENU_SENDMAIL, IDS_MENU_SENDMAIL, IDI_MENUSENDMAIL);

			if( m_ContextMenuMask&GetMenuMask(MENU_APPLY))
				popup.AppendMenuIcon(MENU_APPLY, IDS_MENU_APPLY, 0);
		}

		int cmd = popup.TrackPopupMenu(TPM_RETURNCMD | TPM_LEFTALIGN | TPM_NONOTIFY, point.x, point.y, this, 0);

		switch (cmd)
		{
		case MENU_VIEWPATCH:
			{

				CString path=GetItemText(index,0);
				CTGitPath gitpath;
				gitpath.SetFromWin(path);
	
				CAppUtils::StartUnifiedDiffViewer(path,gitpath.GetFilename());
				break;
			}
		case MENU_VIEWWITHMERGE:
			{
				CString path=GetItemText(index,0);
				CTGitPath gitpath;
				gitpath.SetFromWin(path);

				CTGitPath dir;
				dir.SetFromGit(g_Git.m_CurrentDir);

				CAppUtils::StartExtPatch(gitpath,dir);
				break;
			}
		case MENU_SENDMAIL:
			{
				LaunchProc(CString(_T("sendmail")));
				break;
			}
		case MENU_APPLY:
			{
				LaunchProc(CString(_T("importpatch")));

				break;
			}
		default:
			break;
		}
	}
}

int CPatchListCtrl::LaunchProc(CString& command)
{
	CString tempfile=GetTempFile();
	POSITION pos=this->GetFirstSelectedItemPosition();
	CFile file;
	file.Open(tempfile,CFile::modeWrite|CFile::modeCreate);

	while(pos)
	{
		int index = this->GetNextSelectedItem(pos);
		CString one=this->GetItemText(index,0);
		file.Write(one.GetBuffer(),sizeof(TCHAR)*one.GetLength());
		file.Write(_T("\n"),sizeof(TCHAR)*1);
	}

	file.Close();

	CString cmd;
	cmd = CPathUtils::GetAppDirectory();
	cmd += _T("TortoiseProc.exe /command:");
	cmd += command;
	cmd +=_T(" /pathfile:\"");
	cmd += tempfile;
	cmd += _T("\" /deletepathfile");
	CAppUtils::LaunchApplication(cmd, IDS_ERR_PROC,false);
	return 0;
}
