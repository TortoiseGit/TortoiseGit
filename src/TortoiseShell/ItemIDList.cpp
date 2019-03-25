// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2016, 2019 - TortoiseGit
// Copyright (C) 2003-2006, 2009, 2011-2013, 2015-2016 - TortoiseSVN

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
#include "ItemIDList.h"
#include "StringUtils.h"

ItemIDList::ItemIDList(PCUITEMID_CHILD item, PCUIDLIST_RELATIVE parent)
	: item_ (item)
	, parent_ (parent)
	, count_ (-1)
{
}

ItemIDList::ItemIDList(PCIDLIST_ABSOLUTE item)
	: item_(static_cast<PCUITEMID_CHILD>(item))
	, parent_(0)
	, count_(-1)
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
			while (ptr && ptr->cb != 0)
			{
				++count_;
				LPCBYTE byte = reinterpret_cast<LPCBYTE>(ptr);
				byte += ptr->cb;
				ptr = reinterpret_cast<LPCSHITEMID>(byte);
			}
		}
	}
	return count_;
}

LPCSHITEMID ItemIDList::get(int index) const
{
	int count = 0;

	if (!item_)
		return nullptr;
	LPCSHITEMID ptr = &item_->mkid;
	if (!ptr)
		return nullptr;
	while (ptr->cb != 0)
	{
		if (count == index)
			break;

		++count;
		LPCBYTE byte = reinterpret_cast<LPCBYTE>(ptr);
		byte += ptr->cb;
		ptr = reinterpret_cast<LPCSHITEMID>(byte);
	}
	return ptr;
}

tstring ItemIDList::toString(bool resolveLibraries /*= true*/)
{
	CComPtr<IShellFolder> shellFolder;
	CComPtr<IShellFolder> parentFolder;
	tstring ret;

	if (FAILED(::SHGetDesktopFolder(&shellFolder)))
		return ret;
	if (!parent_ || FAILED(shellFolder->BindToObject(parent_, 0, IID_IShellFolder, reinterpret_cast<void**>(&parentFolder))))
		parentFolder = shellFolder;

	if (parentFolder && item_ != 0)
	{
		STRRET name;
		if (FAILED(parentFolder->GetDisplayNameOf(item_, SHGDN_NORMAL | SHGDN_FORPARSING, &name)))
			return ret;
		CComHeapPtr<TCHAR> szDisplayName;
		if (FAILED(StrRetToStr(&name, item_, &szDisplayName)) || !szDisplayName)
			return ret;
		ret = szDisplayName;
	}
	else
		return ret;

	if (!((resolveLibraries) && (CStringUtils::StartsWith(ret.c_str(), L"::{"))))
		return ret;

	CComPtr<IShellLibrary> plib;
	if (FAILED(plib.CoCreateInstance(CLSID_ShellLibrary, nullptr, CLSCTX_INPROC_SERVER)))
		return ret;

	CComPtr<IShellItem> psiLibrary;
	if (FAILED(SHCreateItemFromParsingName(ret.c_str(), nullptr, IID_PPV_ARGS(&psiLibrary))))
		return ret;

	if (FAILED(plib->LoadLibraryFromItem(psiLibrary, STGM_READ | STGM_SHARE_DENY_NONE)))
		return ret;

	CComPtr<IShellItem> psiSaveLocation;
	if (FAILED(plib->GetDefaultSaveFolder(DSFT_DETECT, IID_PPV_ARGS(&psiSaveLocation))))
		return ret;

	if (CComHeapPtr<WCHAR> pszName; SUCCEEDED(psiSaveLocation->GetDisplayName(SIGDN_FILESYSPATH, &pszName)))
		return tstring(pszName);

	return ret;
}

PCUITEMID_CHILD ItemIDList::operator& ()
{
	return item_;
}
