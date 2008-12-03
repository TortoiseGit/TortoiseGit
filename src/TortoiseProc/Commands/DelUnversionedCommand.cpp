// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007-2008 - TortoiseSVN

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
#include "DelUnversionedCommand.h"

#include "DeleteUnversionedDlg.h"

bool DelUnversionedCommand::Execute()
{
	bool bRet = false;
	CDeleteUnversionedDlg dlg;
	dlg.m_pathList = pathList;
	if (dlg.DoModal() == IDOK)
	{
		if (dlg.m_pathList.GetCount() == 0)
			return FALSE;
		// now remove all items by moving them to the trash bin
		dlg.m_pathList.RemoveChildren();
		CString filelist;
		for (INT_PTR i=0; i<dlg.m_pathList.GetCount(); ++i)
		{
			filelist += dlg.m_pathList[i].GetWinPathString();
			filelist += _T("|");
		}
		filelist += _T("|");
		int len = filelist.GetLength();
		TCHAR * buf = new TCHAR[len+2];
		_tcscpy_s(buf, len+2, filelist);
		for (int i=0; i<len; ++i)
			if (buf[i] == '|')
				buf[i] = 0;
		SHFILEOPSTRUCT fileop;
		fileop.hwnd = hwndExplorer;
		fileop.wFunc = FO_DELETE;
		fileop.pFrom = buf;
		fileop.pTo = NULL;
		fileop.fFlags = FOF_NO_CONNECTED_ELEMENTS | FOF_ALLOWUNDO;
		fileop.lpszProgressTitle = _T("deleting file");
		bRet = (SHFileOperation(&fileop) == 0);
		delete [] buf;
	}
	return true;
}
