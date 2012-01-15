// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2003-2006 - Stefan Kueng
// Copyright (C) 2008-2011 - TortoiseGit

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
#include "ShellUpdater.h"

#define ListView_GetItemTextEx(hwndLV, i, iSubItem_, __buf) \
{ \
  int nLen = 1024;\
  int nRes;\
  LV_ITEM _ms_lvi;\
  _ms_lvi.iSubItem = iSubItem_;\
  do\
  {\
	nLen += 2;\
	_ms_lvi.cchTextMax = nLen;\
    if (__buf)\
		delete[] __buf;\
	__buf = new TCHAR[nLen];\
	_ms_lvi.pszText = __buf;\
    nRes  = (int)::SendMessage((hwndLV), LVM_GETITEMTEXT, (WPARAM)(i), (LPARAM)(LV_ITEM *)&_ms_lvi);\
  } while (nRes == nLen-1);\
}
#define GetDlgItemTextEx(hwndDlg, _id, __buf) \
{\
	int nLen = 1024;\
	int nRes;\
	do\
	{\
		nLen *= 2;\
		if (__buf)\
			delete [] __buf;\
		__buf = new TCHAR[nLen];\
		nRes = GetDlgItemText(hwndDlg, _id, __buf, nLen);\
	} while (nRes == nLen-1);\
}

/**
 * \ingroup TortoiseShell
 * Displays and updates all controls on the property page. The property
 * page itself is shown by explorer.
 */
class CGitPropertyPage
{
public:
	CGitPropertyPage(const std::vector<stdstring> &filenames);
	virtual ~CGitPropertyPage();

	/**
	 * Sets the window handle.
	 * \param hwnd the handle.
	 */
	virtual void SetHwnd(HWND hwnd);
	/**
	 * Callback function which receives the window messages of the
	 * property page. See the Win32 API for PropertySheets for details.
	 */
	virtual BOOL PageProc(HWND hwnd, UINT uMessage, WPARAM wParam, LPARAM lParam);

protected:
	/**
	 * Initializes the property page.
	 */
	virtual void InitWorkfileView();
	void Time64ToTimeString(__time64_t time, TCHAR * buf, size_t buflen);
	void PageProcOnCommand(WPARAM wParam);

	struct listproperty
	{
		stdstring name;
		std::string value;
		int		  count;
	};
	HWND m_hwnd;
	std::vector<stdstring> filenames;
	std::map<stdstring, std::string> propmap;
	TCHAR stringtablebuffer[255];
};


