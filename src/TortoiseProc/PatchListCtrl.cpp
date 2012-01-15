// PathListCtrl.cpp : implementation file
//

#include "stdafx.h"
#include "TortoiseProc.h"
#include "PatchListCtrl.h"
#include "iconmenu.h"
#include "AppUtils.h"
#include "git.h"
#include "AppUtils.h"
// CPatchListCtrl

IMPLEMENT_DYNAMIC(CPatchListCtrl, CListCtrl)

CPatchListCtrl::CPatchListCtrl()
{
	m_ContextMenuMask=0xFFFFFFFF;

	HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
	LOGFONT lf = {0};
	GetObject(hFont, sizeof(LOGFONT), &lf);
	lf.lfWeight = FW_BOLD;
	m_boldFont = CreateFontIndirect(&lf);

}

CPatchListCtrl::~CPatchListCtrl()
{
	if (m_boldFont)
		DeleteObject(m_boldFont);
}


BEGIN_MESSAGE_MAP(CPatchListCtrl, CListCtrl)
	ON_NOTIFY_REFLECT(NM_DBLCLK, &CPatchListCtrl::OnNMDblclk)
	ON_WM_CONTEXTMENU()
	ON_NOTIFY_REFLECT(NM_CUSTOMDRAW, &CPatchListCtrl::OnNMCustomdraw)
END_MESSAGE_MAP()



// CPatchListCtrl message handlers



void CPatchListCtrl::OnNMDblclk(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);

	CString path=GetItemText(pNMItemActivate->iItem,0);
	CTGitPath gitpath;
	gitpath.SetFromWin(path);

	CAppUtils::StartUnifiedDiffViewer(path,gitpath.GetFilename());

	*pResult = 0;
}

void CPatchListCtrl::OnContextMenu(CWnd* /*pWnd*/, CPoint point)
{
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
				LaunchProc(_T("sendmail"));
				break;
			}
		case MENU_APPLY:
			{
				LaunchProc(_T("importpatch"));

				break;
			}
		default:
			break;
		}
	}
}

int CPatchListCtrl::LaunchProc(const CString& command)
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

	CString cmd = command;
	cmd +=_T(" /pathfile:\"");
	cmd += tempfile;
	cmd += _T("\" /deletepathfile");
	CAppUtils::RunTortoiseProc(cmd);
	return 0;
}

void CPatchListCtrl::OnNMCustomdraw(NMHDR *pNMHDR, LRESULT *pResult)
{
	NMLVCUSTOMDRAW *pNMCD = reinterpret_cast<NMLVCUSTOMDRAW*>(pNMHDR);

	*pResult = 0;


	switch (pNMCD->nmcd.dwDrawStage)
	{
	case CDDS_PREPAINT:
		{
			*pResult = CDRF_NOTIFYITEMDRAW;
			return;
		}
		break;
	case CDDS_ITEMPREPAINT:
		{
			// This is the prepaint stage for an item. Here's where we set the
			// item's text color.

			// Tell Windows to send draw notifications for each subitem.
			*pResult = CDRF_NOTIFYSUBITEMDRAW;

			DWORD data = this->GetItemData(pNMCD->nmcd.dwItemSpec);
			if(data & (STATUS_APPLY_FAIL | STATUS_APPLY_SUCCESS | STATUS_APPLY_SKIP))
			{
				pNMCD->clrTextBk = RGB(200,200,200);
			}

			switch(data & STATUS_MASK)
			{
			case STATUS_APPLY_SUCCESS:
				pNMCD->clrText = RGB(0,128,0);
				break;
			case STATUS_APPLY_FAIL:
				pNMCD->clrText = RGB(255,0,0);
				break;
			case STATUS_APPLY_SKIP:
				pNMCD->clrText = RGB(128,64,0);
				break;
			}

			if(data & STATUS_APPLYING)
			{
				SelectObject(pNMCD->nmcd.hdc, m_boldFont);
				*pResult = CDRF_NOTIFYSUBITEMDRAW | CDRF_NEWFONT;
			}

		}
		break;
	}
	*pResult = CDRF_DODEFAULT;
}
