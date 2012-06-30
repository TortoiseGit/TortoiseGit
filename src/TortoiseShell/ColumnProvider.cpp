// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2003-2008,2012 - TortoiseSVN
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
#include "ShellExt.h"
#include "guids.h"
#include "PreserveChdir.h"
//#include "SVNProperties.h"
#include "UnicodeUtils.h"
#include "GitStatus.h"
#include "PathUtils.h"
#include "..\TGitCache\CacheInterface.h"


const static int ColumnFlags = SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT;

// Defines that revision numbers occupy at most MAX_REV_STRING_LEN characters.
// There are Perforce repositories out there that have several 100,000 revs.
// So, don't be too restrictive by limiting this to 6 digits + 1 separator,
// for instance.
//
// Because shorter strings will be extended to have exactly MAX_REV_STRING_LEN
// characters, large numbers will produce large strings. These, in turn, will
// affect column auto sizing. This setting is a reasonable compromise.

STDMETHODIMP CShellExt::GetColumnInfo(DWORD dwIndex, SHCOLUMNINFO *psci)
{
	__try
	{
		return GetColumnInfo_Wrap(dwIndex, psci);
	}
	__except(CCrashReport::Instance().SendReport(GetExceptionInformation()))
	{
	}
	return E_FAIL;
}

// IColumnProvider members
STDMETHODIMP CShellExt::GetColumnInfo_Wrap(DWORD dwIndex, SHCOLUMNINFO *psci)
{
	if (psci == 0)
		return E_POINTER;

	PreserveChdir preserveChdir;
	if (dwIndex > 0) // TODO: keep for now to be able to hide unimplemented columns
		return S_FALSE;

	ShellCache::CacheType cachetype = g_ShellCache.GetCacheType();
	if (cachetype == ShellCache::none)
		return S_FALSE;

	LoadLangDll();
	switch (dwIndex)
	{
		case 0:	// Git Status
			GetColumnInfo(dwIndex, psci, 15, IDS_COLTITLESTATUS, IDS_COLDESCSTATUS);
			break;
		case 1:	// Git Revision
			GetColumnInfo(dwIndex, psci, 40, IDS_COLTITLEREV, IDS_COLDESCREV);
			break;
		case 2:	// Git Url
			GetColumnInfo(dwIndex, psci, 30, IDS_COLTITLEURL, IDS_COLDESCURL);
			break;
		case 3:	// Git Short Url
			GetColumnInfo(dwIndex, psci, 30, IDS_COLTITLESHORTURL, IDS_COLDESCSHORTURL);
			break;
		case 4:	// Author and Git Author
			psci->scid.fmtid = FMTID_SummaryInformation;	// predefined FMTID
			psci->scid.pid   = PIDSI_AUTHOR;				// Predefined - author
			psci->vt         = VT_LPSTR;					// We'll return the data as a string
			psci->fmt        = LVCFMT_LEFT;					// Text will be left-aligned in the column
			psci->csFlags    = SHCOLSTATE_TYPE_STR;			// Data should be sorted as strings
			psci->cChars     = 32;							// Default col width in chars
			MAKESTRING(IDS_COLTITLEAUTHOR);
			lstrcpynW(psci->wszTitle, stringtablebuffer, MAX_COLUMN_NAME_LEN);
			MAKESTRING(IDS_COLDESCAUTHOR);
			lstrcpynW(psci->wszDescription, stringtablebuffer, MAX_COLUMN_DESC_LEN);
			break;
		default:
			return S_FALSE;
	}

	return S_OK;
}

void CShellExt::GetColumnInfo(DWORD dwIndex, SHCOLUMNINFO *psci, UINT characterCount, UINT title, UINT description)
{
			psci->scid.fmtid = CLSID_Tortoisegit_UPTODATE;
			psci->scid.pid = dwIndex;
			psci->vt = VT_BSTR;
			psci->fmt = LVCFMT_LEFT;
			psci->cChars = characterCount;
			psci->csFlags = ColumnFlags;

			MAKESTRING(title);
			lstrcpynW(psci->wszTitle, stringtablebuffer, MAX_COLUMN_NAME_LEN);
			MAKESTRING(description);
			lstrcpynW(psci->wszDescription, stringtablebuffer, MAX_COLUMN_DESC_LEN);
}

STDMETHODIMP CShellExt::GetItemData(LPCSHCOLUMNID pscid, LPCSHCOLUMNDATA pscd, VARIANT *pvarData)
{
	__try
	{
		return GetItemData_Wrap(pscid, pscd, pvarData);
	}
	__except(CCrashReport::Instance().SendReport(GetExceptionInformation()))
	{
	}
	return E_FAIL;
}

STDMETHODIMP CShellExt::GetItemData_Wrap(LPCSHCOLUMNID pscid, LPCSHCOLUMNDATA pscd, VARIANT *pvarData)
{
	if ((pscid == 0) || (pscd == 0))
		return E_INVALIDARG;
	if (pvarData == 0)
		return E_POINTER;

	PreserveChdir preserveChdir;
	if (!g_ShellCache.IsPathAllowed((TCHAR *)pscd->wszFile))
	{
		return S_FALSE;
	}
	LoadLangDll();
	ShellCache::CacheType cachetype = g_ShellCache.GetCacheType();
	if (pscid->fmtid == CLSID_Tortoisegit_UPTODATE)
	{
		stdstring szInfo;
		const TCHAR * path = (TCHAR *)pscd->wszFile;

		// reserve for the path + trailing \0

		TCHAR buf[MAX_STATUS_STRING_LENGTH+1];
		SecureZeroMemory(buf, MAX_STATUS_STRING_LENGTH);
		switch (pscid->pid)
		{
			case 0:	// Git Status
				GetColumnStatus(path, pscd->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
				GitStatus::GetStatusString(g_hResInst, filestatus, buf, _countof(buf), (WORD)CRegStdDWORD(_T("Software\\TortoiseGit\\LanguageID"), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT)));
				szInfo = buf;
				break;
			case 1:	// Git Revision
#if 0
				GetColumnStatus(path, pscd->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
				if (columnrev >= 0)
				{
					V_VT(pvarData) = VT_I4;
					V_I4(pvarData) = columnrev;
				}
#endif
				return S_OK;
				break;
			case 2:	// Git Url
				GetColumnStatus(path, pscd->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
				szInfo = itemurl;
				break;
			case 3:	// Git Short Url
				GetColumnStatus(path, pscd->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
				szInfo = itemshorturl;
				break;
			default:
				return S_FALSE;
		}
		const WCHAR * wsInfo = szInfo.c_str();
		V_VT(pvarData) = VT_BSTR;
		V_BSTR(pvarData) = SysAllocString(wsInfo);
		return S_OK;
	}
	if (pscid->fmtid == FMTID_SummaryInformation)
	{
		stdstring szInfo;
		const TCHAR * path = pscd->wszFile;

		if (cachetype == ShellCache::none)
			return S_FALSE;
		switch (pscid->pid)
		{
		case PIDSI_AUTHOR:			// Author and Git Author
			GetColumnStatus(path, pscd->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
			szInfo = columnauthor;
			break;
		default:
			return S_FALSE;
		}
		wide_string wsInfo = szInfo;
		V_VT(pvarData) = VT_BSTR;
		V_BSTR(pvarData) = SysAllocString(wsInfo.c_str());
		return S_OK;
	}

	return S_FALSE;
}

STDMETHODIMP CShellExt::Initialize(LPCSHCOLUMNINIT psci)
{
	__try
	{
		return Initialize_Wrap(psci);
	}
	__except(CCrashReport::Instance().SendReport(GetExceptionInformation()))
	{
	}
	return E_FAIL;
}

STDMETHODIMP CShellExt::Initialize_Wrap(LPCSHCOLUMNINIT psci)
{
	if (psci == 0)
		return E_INVALIDARG;
	// psci->wszFolder (WCHAR) holds the path to the folder to be displayed
	// Should check to see if its a "SVN" folder and if not return E_FAIL

	PreserveChdir preserveChdir;
	if (g_ShellCache.IsColumnsEveryWhere())
		return S_OK;
	std::wstring path = psci->wszFolder;
	if (!path.empty())
	{
		if (! g_ShellCache.HasGITAdminDir(path.c_str(), TRUE))
			return E_FAIL;
	}
	columnfilepath = _T("");
	return S_OK;
}

void CShellExt::GetColumnStatus(const TCHAR * path, BOOL bIsDir)
{
	PreserveChdir preserveChdir;
	if (_tcscmp(path, columnfilepath.c_str())==0)
		return;
	LoadLangDll();
	columnfilepath = path;
	const FileStatusCacheEntry * status = NULL;
	TGITCacheResponse itemStatus;
	ShellCache::CacheType t = ShellCache::exe;
	AutoLocker lock(g_csGlobalCOMGuard);
	t = g_ShellCache.GetCacheType();

	switch (t)
	{
	case ShellCache::exe:
		{
			SecureZeroMemory(&itemStatus, sizeof(itemStatus));
			if(m_remoteCacheLink.GetStatusFromRemoteCache(CTGitPath(path), &itemStatus, true))
			{
				filestatus = GitStatus::GetMoreImportant(itemStatus.m_status.text_status, itemStatus.m_status.prop_status);
			}
			else
			{
				filestatus = git_wc_status_none;
				columnauthor.clear();
				columnrev = GIT_INVALID_REVNUM;
				itemurl.clear();
				itemshorturl.clear();
				return;
			}
		}
		break;
	case ShellCache::dll:
	case ShellCache::dllFull:
		{
			status = m_CachedStatus.GetFullStatus(CTGitPath(path), bIsDir, TRUE);
			filestatus = status->status;
		}
		break;
	default:
	case ShellCache::none:
		{
			if (g_ShellCache.HasGITAdminDir(path, bIsDir))
				filestatus = git_wc_status_normal;
			else
				filestatus = git_wc_status_none;
			columnauthor.clear();
			columnrev = GIT_INVALID_REVNUM;
			itemurl.clear();
			itemshorturl.clear();
			return;
		}
		break;
	}

	if (t == ShellCache::exe)
	{
		columnrev = itemStatus.m_entry.cmt_rev;
	}
	else
	{
		if (status)
		{
			columnrev = status->rev;
		}
	}
}

