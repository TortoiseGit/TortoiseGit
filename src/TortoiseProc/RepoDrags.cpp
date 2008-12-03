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
#include "StdAfx.h"
#include "RepoDrags.h"
#include "RepositoryBrowser.h"
#include "SVNDataObject.h"

CTreeDropTarget::CTreeDropTarget(CRepositoryBrowser * pRepoBrowser) : CIDropTarget(pRepoBrowser->m_RepoTree.GetSafeHwnd())
	, m_pRepoBrowser(pRepoBrowser)
	, m_bFiles(false)
{
}

bool CTreeDropTarget::OnDrop(FORMATETC* pFmtEtc, STGMEDIUM& medium, DWORD *pdwEffect, POINTL pt)
{
	// find the target
	CString targetUrl;
	HTREEITEM hDropTarget = m_pRepoBrowser->m_RepoTree.GetNextItem(TVI_ROOT, TVGN_DROPHILITE);
	if (hDropTarget)
	{
		CTreeItem * pItem = (CTreeItem*)m_pRepoBrowser->m_RepoTree.GetItemData(hDropTarget);
		if (pItem == NULL)
			return false;
		targetUrl = pItem->url;
	}
	if (pFmtEtc->cfFormat == CF_UNICODETEXT && medium.tymed == TYMED_HGLOBAL)
	{
		TCHAR* pStr = (TCHAR*)GlobalLock(medium.hGlobal);
		CString urls;
		if(pStr != NULL)
		{
			urls = pStr;
		}
		GlobalUnlock(medium.hGlobal);
		urls.Replace(_T("\r\n"), _T("*"));
		CTSVNPathList urlList;
		urlList.LoadFromAsteriskSeparatedString(urls);

		m_pRepoBrowser->OnDrop(CTSVNPath(targetUrl), urlList, m_pRepoBrowser->GetRevision(), *pdwEffect, pt);
	}

	if (pFmtEtc->cfFormat == CF_SVNURL && medium.tymed == TYMED_HGLOBAL)
	{
		TCHAR* pStr = (TCHAR*)GlobalLock(medium.hGlobal);
		CString urls;
		if(pStr != NULL)
		{
			urls = pStr;
		}
		GlobalUnlock(medium.hGlobal);
		urls.Replace(_T("\r\n"), _T("*"));
		CTSVNPathList urlListRevs;
		urlListRevs.LoadFromAsteriskSeparatedString(urls);
		CTSVNPathList urlList;
		SVNRev srcRev;
		for (int i=0; i<urlListRevs.GetCount(); ++i)
		{
			int pos = urlListRevs[i].GetSVNPathString().Find('?');
			if (pos > 0)
			{
				if (!srcRev.IsValid())
					srcRev = SVNRev(urlListRevs[i].GetSVNPathString().Mid(pos+1));
				urlList.AddPath(CTSVNPath(urlListRevs[i].GetSVNPathString().Left(pos)));
			}
			else
				urlList.AddPath(urlListRevs[i]);
		}

		m_pRepoBrowser->OnDrop(CTSVNPath(targetUrl), urlList, srcRev, *pdwEffect, pt);
	}

	if(pFmtEtc->cfFormat == CF_HDROP && medium.tymed == TYMED_HGLOBAL)
	{
		HDROP hDrop = (HDROP)GlobalLock(medium.hGlobal);
		if(hDrop != NULL)
		{
			CTSVNPathList urlList;
			TCHAR szFileName[MAX_PATH];

			UINT cFiles = DragQueryFile(hDrop, 0xFFFFFFFF, NULL, 0); 
			for(UINT i = 0; i < cFiles; ++i)
			{
				DragQueryFile(hDrop, i, szFileName, sizeof(szFileName));
				urlList.AddPath(CTSVNPath(szFileName));
			}
			m_pRepoBrowser->OnDrop(CTSVNPath(targetUrl), urlList, m_pRepoBrowser->GetRevision(), *pdwEffect, pt);
		}
		GlobalUnlock(medium.hGlobal);
	}
	TreeView_SelectDropTarget(m_hTargetWnd, NULL);
	return true;
}

HRESULT CTreeDropTarget::DragEnter(IDataObject __RPC_FAR *pDataObj, DWORD grfKeyState, POINTL pt, DWORD __RPC_FAR *pdwEffect)
{
	FORMATETC ftetc={0}; 
	ftetc.dwAspect = DVASPECT_CONTENT; 
	ftetc.lindex = -1; 
	ftetc.tymed = TYMED_HGLOBAL; 
	ftetc.cfFormat=CF_HDROP; 
	if (pDataObj->QueryGetData(&ftetc) == S_OK)
		m_bFiles = true;
	else
		m_bFiles = false;
	m_dwHoverStartTicks = 0;
	hLastItem = NULL;
	return CIDropTarget::DragEnter(pDataObj, grfKeyState, pt, pdwEffect);
}

HRESULT CTreeDropTarget::DragOver(DWORD grfKeyState, POINTL pt, DWORD __RPC_FAR *pdwEffect)
{
	TVHITTESTINFO hit;
	hit.pt = (POINT&)pt;
	ScreenToClient(m_hTargetWnd,&hit.pt);
	hit.flags = TVHT_ONITEM;
	HTREEITEM hItem = TreeView_HitTest(m_hTargetWnd,&hit);
	if (hItem != hLastItem)
		m_dwHoverStartTicks = 0;
	hLastItem = hItem;

	*pdwEffect = DROPEFFECT_MOVE;
	if (grfKeyState & MK_CONTROL)
		*pdwEffect = DROPEFFECT_COPY;

	if (hItem != NULL)
	{
		if (m_bFiles)
			*pdwEffect = DROPEFFECT_COPY;
		TreeView_SelectDropTarget(m_hTargetWnd, hItem);
		if ((m_pRepoBrowser->m_RepoTree.GetItemState(hItem, TVIS_EXPANDED)&TVIS_EXPANDED) != TVIS_EXPANDED)
		{
			if (m_dwHoverStartTicks == 0)
				m_dwHoverStartTicks = GetTickCount();
			UINT timeout = 0;
			//SystemParametersInfo(SPI_GETMOUSEHOVERTIME, 0, &timeout, 0);
			timeout = 2000;
			if ((GetTickCount() - m_dwHoverStartTicks) > timeout)
			{
				// expand the item
				m_pRepoBrowser->m_RepoTree.Expand(hItem, TVE_EXPAND);
				m_dwHoverStartTicks = 0;
			}
		}
		else
			m_dwHoverStartTicks = 0;
	}
	else
	{
		TreeView_SelectDropTarget(m_hTargetWnd, NULL);
		*pdwEffect = DROPEFFECT_NONE;
		m_dwHoverStartTicks = 0;
	}
	CRect rect;
	m_pRepoBrowser->m_RepoTree.GetWindowRect(&rect);
	if (rect.PtInRect((POINT&)pt))
	{
		if (pt.y > (rect.bottom-20))
		{
			m_pRepoBrowser->m_RepoTree.SendMessage(WM_VSCROLL, MAKEWPARAM (SB_LINEDOWN, 0), NULL);
			m_dwHoverStartTicks = 0;
		}
		if (pt.y < (rect.top+20))
		{
			m_pRepoBrowser->m_RepoTree.SendMessage(WM_VSCROLL, MAKEWPARAM (SB_LINEUP, 0), NULL);
			m_dwHoverStartTicks = 0;
		}
	}

	return CIDropTarget::DragOver(grfKeyState, pt, pdwEffect);
}

HRESULT CTreeDropTarget::DragLeave(void)
{
	TreeView_SelectDropTarget(m_hTargetWnd, NULL);
	m_dwHoverStartTicks = 0;
	return CIDropTarget::DragLeave();
}

CListDropTarget::CListDropTarget(CRepositoryBrowser * pRepoBrowser):CIDropTarget(pRepoBrowser->m_RepoList.GetSafeHwnd())
	, m_pRepoBrowser(pRepoBrowser)
	, m_bFiles(false)
{
}	

bool CListDropTarget::OnDrop(FORMATETC* pFmtEtc, STGMEDIUM& medium, DWORD *pdwEffect, POINTL pt)
{
	// find the target url
	CString targetUrl;
	int targetIndex = m_pRepoBrowser->m_RepoList.GetNextItem(-1, LVNI_DROPHILITED);
	if (targetIndex >= 0)
	{
		targetUrl = ((CItem*)m_pRepoBrowser->m_RepoList.GetItemData(targetIndex))->absolutepath;
	}
	else
	{
		HTREEITEM hDropTarget = m_pRepoBrowser->m_RepoTree.GetSelectedItem();
		if (hDropTarget)
		{
			targetUrl = ((CTreeItem*)m_pRepoBrowser->m_RepoTree.GetItemData(hDropTarget))->url;
		}
	}
	if(pFmtEtc->cfFormat == CF_UNICODETEXT && medium.tymed == TYMED_HGLOBAL)
	{
		TCHAR* pStr = (TCHAR*)GlobalLock(medium.hGlobal);
		CString urls;
		if(pStr != NULL)
		{
			urls = pStr;
		}
		GlobalUnlock(medium.hGlobal);
		urls.Replace(_T("\r\n"), _T("*"));
		CTSVNPathList urlList;
		urlList.LoadFromAsteriskSeparatedString(urls);
		m_pRepoBrowser->OnDrop(CTSVNPath(targetUrl), urlList, m_pRepoBrowser->GetRevision(), *pdwEffect, pt);
	}
	if(pFmtEtc->cfFormat == CF_SVNURL && medium.tymed == TYMED_HGLOBAL)
	{
		TCHAR* pStr = (TCHAR*)GlobalLock(medium.hGlobal);
		CString urls;
		if(pStr != NULL)
		{
			urls = pStr;
		}
		GlobalUnlock(medium.hGlobal);
		urls.Replace(_T("\r\n"), _T("*"));
		CTSVNPathList urlListRevs;
		urlListRevs.LoadFromAsteriskSeparatedString(urls);
		CTSVNPathList urlList;
		SVNRev srcRev;
		for (int i=0; i<urlListRevs.GetCount(); ++i)
		{
			int pos = urlListRevs[i].GetSVNPathString().Find('?');
			if (pos > 0)
			{
				if (!srcRev.IsValid())
					srcRev = SVNRev(urlListRevs[i].GetSVNPathString().Mid(pos+1));
				urlList.AddPath(CTSVNPath(urlListRevs[i].GetSVNPathString().Left(pos)));
			}
			else
				urlList.AddPath(urlListRevs[i]);
		}

		m_pRepoBrowser->OnDrop(CTSVNPath(targetUrl), urlList, srcRev, *pdwEffect, pt);
	}

	if(pFmtEtc->cfFormat == CF_HDROP && medium.tymed == TYMED_HGLOBAL)
	{
		HDROP hDrop = (HDROP)GlobalLock(medium.hGlobal);
		if(hDrop != NULL)
		{
			CTSVNPathList urlList;
			TCHAR szFileName[MAX_PATH];

			UINT cFiles = DragQueryFile(hDrop, 0xFFFFFFFF, NULL, 0); 
			for(UINT i = 0; i < cFiles; ++i)
			{
				DragQueryFile(hDrop, i, szFileName, sizeof(szFileName));
				urlList.AddPath(CTSVNPath(szFileName));
			}
			m_pRepoBrowser->OnDrop(CTSVNPath(targetUrl), urlList, m_pRepoBrowser->GetRevision(), *pdwEffect, pt);
		}
		GlobalUnlock(medium.hGlobal);
	}
	ListView_SetItemState(m_hTargetWnd, -1, 0, LVIS_DROPHILITED);
	return true; //let base free the medium
}

HRESULT CListDropTarget::DragEnter(IDataObject __RPC_FAR *pDataObj, DWORD grfKeyState, POINTL pt, DWORD __RPC_FAR *pdwEffect)
{
	FORMATETC ftetc={0}; 
	ftetc.dwAspect = DVASPECT_CONTENT; 
	ftetc.lindex = -1; 
	ftetc.tymed = TYMED_HGLOBAL; 
	ftetc.cfFormat=CF_HDROP; 
	if (pDataObj->QueryGetData(&ftetc) == S_OK)
	{
		m_bFiles = true;
	}
	else
	{
		m_bFiles = false;
	}
	return CIDropTarget::DragEnter(pDataObj, grfKeyState, pt, pdwEffect);
}


HRESULT CListDropTarget::DragOver(DWORD grfKeyState, POINTL pt, DWORD __RPC_FAR *pdwEffect)
{
	LVHITTESTINFO hit;
	hit.pt = (POINT&)pt;
	ScreenToClient(m_hTargetWnd,&hit.pt);
	hit.flags = LVHT_ONITEM;
	int iItem = ListView_HitTest(m_hTargetWnd,&hit);

	*pdwEffect = DROPEFFECT_MOVE;
	if (grfKeyState & MK_CONTROL)
		*pdwEffect = DROPEFFECT_COPY;

	if (iItem >= 0)
	{
		CItem * pItem = (CItem*)m_pRepoBrowser->m_RepoList.GetItemData(iItem);
		if (pItem)
		{
			if (pItem->kind != svn_node_dir)
			{
				*pdwEffect = DROPEFFECT_NONE;
				ListView_SetItemState(m_hTargetWnd, -1, 0, LVIS_DROPHILITED);
			}
			else
			{
				if (m_bFiles)
					*pdwEffect = DROPEFFECT_COPY;
				ListView_SetItemState(m_hTargetWnd, -1, 0, LVIS_DROPHILITED);
				ListView_SetItemState(m_hTargetWnd, iItem, LVIS_DROPHILITED, LVIS_DROPHILITED);
			}
		}
	}
	else
	{
		ListView_SetItemState(m_hTargetWnd, -1, 0, LVIS_DROPHILITED);
	}

	CRect rect;
	m_pRepoBrowser->m_RepoList.GetWindowRect(&rect);
	if (rect.PtInRect((POINT&)pt))
	{
		if (pt.y > (rect.bottom-20))
		{
			m_pRepoBrowser->m_RepoList.SendMessage(WM_VSCROLL, MAKEWPARAM (SB_LINEDOWN, 0), NULL);
		}
		if (pt.y < (rect.top+20))
		{
			m_pRepoBrowser->m_RepoList.SendMessage(WM_VSCROLL, MAKEWPARAM (SB_LINEUP, 0), NULL);
		}
	}

	return CIDropTarget::DragOver(grfKeyState, pt, pdwEffect);
}

HRESULT CListDropTarget::DragLeave(void)
{
	ListView_SetItemState(m_hTargetWnd, -1, 0, LVIS_DROPHILITED);
	return CIDropTarget::DragLeave();
}
