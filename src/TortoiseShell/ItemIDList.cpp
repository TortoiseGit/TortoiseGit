// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2003-2006,2009,2011 - TortoiseSVN

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
#include "ShellExt.h"
#include "ItemIDList.h"


ItemIDList::ItemIDList(LPCITEMIDLIST item, LPCITEMIDLIST parent) :
	  item_ (item)
	, parent_ (parent)
	, count_ (-1)
{
}

ItemIDList::~ItemIDList()
{
}

int ItemIDList::size() const
{
	if (count_ == -1)
	{
		count_ = 0;
		if (item_)
		{
			LPCSHITEMID ptr = &item_->mkid;
			while (ptr != 0 && ptr->cb != 0)
			{
				++count_;
				LPBYTE byte = (LPBYTE) ptr;
				byte += ptr->cb;
				ptr = (LPCSHITEMID) byte;
			}
		}
	}
	return count_;
}

LPCSHITEMID ItemIDList::get(int index) const
{
	int count = 0;

	if (item_ == NULL)
		return NULL;
	LPCSHITEMID ptr = &item_->mkid;
	if (ptr == NULL)
		return NULL;
	while (ptr->cb != 0)
	{
		if (count == index)
			break;

		++count;
		LPBYTE byte = (LPBYTE) ptr;
		byte += ptr->cb;
		ptr = (LPCSHITEMID) byte;
	}
	return ptr;

}
stdstring ItemIDList::toString(bool resolveLibraries /*= true*/)
{
	IShellFolder *shellFolder = NULL;
	IShellFolder *parentFolder = NULL;
	STRRET name;
	TCHAR * szDisplayName = NULL;
	stdstring ret;
	HRESULT hr;

	hr = ::SHGetDesktopFolder(&shellFolder);
	if (!SUCCEEDED(hr))
		return ret;
	if (parent_)
	{
		hr = shellFolder->BindToObject(parent_, 0, IID_IShellFolder, (void**) &parentFolder);
		if (!SUCCEEDED(hr))
			parentFolder = shellFolder;
	}
	else
	{
		parentFolder = shellFolder;
	}

	if ((parentFolder != 0)&&(item_ != 0))
	{
		hr = parentFolder->GetDisplayNameOf(item_, SHGDN_NORMAL | SHGDN_FORPARSING, &name);
		if (!SUCCEEDED(hr))
		{
			parentFolder->Release();
			return ret;
		}
		hr = StrRetToStr (&name, item_, &szDisplayName);
		if (!SUCCEEDED(hr))
			return ret;
	}
	parentFolder->Release();
	if (szDisplayName == NULL)
	{
		CoTaskMemFree(szDisplayName);
		return ret;
	}
	ret = szDisplayName;
	CoTaskMemFree(szDisplayName);
	if ((resolveLibraries) &&
		(_tcsncmp(ret.c_str(), _T("::{"), 3)==0))
	{
		CComPtr<IShellLibrary> plib;
		HRESULT hr = CoCreateInstance(CLSID_ShellLibrary, 
									  NULL, 
									  CLSCTX_INPROC_SERVER, 
									  IID_PPV_ARGS(&plib));
		if (SUCCEEDED(hr))
		{
			typedef HRESULT STDAPICALLTYPE SHCreateItemFromParsingNameFN(__in PCWSTR pszPath, __in_opt IBindCtx *pbc, __in REFIID riid, __deref_out void **ppv);
			CAutoLibrary hShell = ::LoadLibrary(_T("shell32.dll"));
			if (hShell)
			{
				SHCreateItemFromParsingNameFN *pfnSHCreateItemFromParsingName = (SHCreateItemFromParsingNameFN*)GetProcAddress(hShell, "SHCreateItemFromParsingName");
				if (pfnSHCreateItemFromParsingName)
				{
					CComPtr<IShellItem> psiLibrary;
					hr = pfnSHCreateItemFromParsingName(ret.c_str(), NULL, IID_PPV_ARGS(&psiLibrary));
					if (SUCCEEDED(hr))
					{
						hr = plib->LoadLibraryFromItem(psiLibrary, STGM_READ|STGM_SHARE_DENY_NONE);
						if (SUCCEEDED(hr))
						{
							CComPtr<IShellItem> psiSaveLocation;
							hr = plib->GetDefaultSaveFolder(DSFT_DETECT, IID_PPV_ARGS(&psiSaveLocation));
							if (SUCCEEDED(hr))
							{
								PWSTR pszName = NULL;
								hr = psiSaveLocation->GetDisplayName(SIGDN_FILESYSPATH, &pszName);
								if (SUCCEEDED(hr))
								{
									ret = pszName;
									CoTaskMemFree(pszName);
								}
							}
						}
					}
				}
			}
		}
	}
	return ret;
}

LPCITEMIDLIST ItemIDList::operator& ()
{
	return item_;
}
