// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2008 - Stefan Kueng

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
#pragma once
#include "DragDropImpl.h"


class CItem;
class CTreeItem;
class CRepositoryBrowser;

/**
 * \ingroup TortoiseProc
 * Implements a drop target on the left tree control in the repository browser
 */
class CTreeDropTarget : public CIDropTarget
{
public:
	CTreeDropTarget(CRepositoryBrowser * pRepoBrowser);
	
	virtual bool OnDrop(FORMATETC* pFmtEtc, STGMEDIUM& medium, DWORD *pdwEffect, POINTL pt);
	virtual HRESULT STDMETHODCALLTYPE DragEnter(IDataObject __RPC_FAR *pDataObj, DWORD grfKeyState, POINTL pt, DWORD __RPC_FAR *pdwEffect);
	virtual HRESULT STDMETHODCALLTYPE DragOver(DWORD grfKeyState, POINTL pt, DWORD __RPC_FAR *pdwEffect);
	virtual HRESULT STDMETHODCALLTYPE DragLeave(void);

private:
	CRepositoryBrowser *	m_pRepoBrowser;
	bool					m_bFiles;
	DWORD					m_dwHoverStartTicks;
	HTREEITEM				hLastItem;
};

/**
 * \ingroup TortoiseProc
 * Implements a drop target on the right list control in the repository browser
 */
class CListDropTarget : public CIDropTarget
{
public:
	CListDropTarget(CRepositoryBrowser * pRepoBrowser);

	virtual bool OnDrop(FORMATETC* pFmtEtc, STGMEDIUM& medium, DWORD *pdwEffect, POINTL pt);
	virtual HRESULT STDMETHODCALLTYPE DragEnter(IDataObject __RPC_FAR *pDataObj, DWORD grfKeyState, POINTL pt, DWORD __RPC_FAR *pdwEffect);
	virtual HRESULT STDMETHODCALLTYPE DragOver(DWORD grfKeyState, POINTL pt, DWORD __RPC_FAR *pdwEffect);
	virtual HRESULT STDMETHODCALLTYPE DragLeave(void);

private:
	CRepositoryBrowser * m_pRepoBrowser;
	bool m_bFiles;
};
