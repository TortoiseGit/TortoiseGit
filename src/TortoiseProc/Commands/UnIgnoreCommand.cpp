// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2016, 2019 - TortoiseGit
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
#include "stdafx.h"
#include "UnIgnoreCommand.h"

#include "MessageBox.h"
#include "PathUtils.h"
#include "SVNProperties.h"

bool UnIgnoreCommand::Execute()
{
	CString filelist;
	BOOL err = FALSE;
	for (int nPath = 0; nPath < pathList.GetCount(); ++nPath)
	{
		CString name = CPathUtils::PathPatternEscape(pathList[nPath].GetFileOrDirectoryName());
		if (parser.HasKey(L"onlymask"))
			name = L'*' + pathList[nPath].GetFileExtension();
		filelist += name + L'\n';
		CTSVNPath parentfolder = pathList[nPath].GetContainingDirectory();
		SVNProperties props(parentfolder, SVNRev::REV_WC, false);
		CStringA value;
		for (int i = 0; i < props.GetCount(); ++i)
		{
			CString propname(props.GetItemName(i).c_str());
			if (propname.CompareNoCase(L"svn:ignore") == 0)
			{
				//treat values as normal text even if they're not
				value = props.GetItemValue(i).c_str();
				break;
			}
		}
		value = value.Trim("\n\r");
		value += '\n';
		value.Remove('\r');
		value.Replace("\n\n", "\n");

		// Delete all occurrences of 'name'
		// "\n" is temporarily prepended to make the algorithm easier
		value = "\n" + value;
		value.Replace("\n" + CUnicodeUtils::GetUTF8(name) + "\n", "\n");
		value = value.Mid(1);

		CStringA sTrimmedvalue = value;
		sTrimmedvalue.Trim();
		if (sTrimmedvalue.IsEmpty())
		{
			if (!props.Remove(L"svn:ignore"))
			{
				CString temp;
				temp.Format(IDS_ERR_FAILEDUNIGNOREPROPERTY, static_cast<LPCTSTR>(name));
				CMessageBox::Show(hwndExplorer, temp, L"TortoiseGit", MB_ICONERROR);
				err = TRUE;
				break;
			}
		}
		else
		{
			if (!props.Add(L"svn:ignore", static_cast<LPCSTR>(value)))
			{
				CString temp;
				temp.Format(IDS_ERR_FAILEDUNIGNOREPROPERTY, static_cast<LPCTSTR>(name));
				CMessageBox::Show(hwndExplorer, temp, L"TortoiseGit", MB_ICONERROR);
				err = TRUE;
				break;
			}
		}
	}
	if (err == FALSE)
	{
		CString temp;
		temp.Format(IDS_PROC_UNIGNORESUCCESS, static_cast<LPCTSTR>(filelist));
		CMessageBox::Show(hwndExplorer, temp, L"TortoiseGit", MB_ICONINFORMATION);
		return true;
	}
	return false;
}
