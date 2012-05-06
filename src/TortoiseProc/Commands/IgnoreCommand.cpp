// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2009-2012 - TortoiseGit
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
#include "IgnoreCommand.h"

#include "MessageBox.h"
#include "PathUtils.h"
#include "AppUtils.h"
#include "ShellUpdater.h"
//#include "SVNProperties.h"

bool IgnoreCommand::Execute()
{
	bool bmask=false;

	if(parser.HasKey(_T("onlymask")))
	{
		bmask=true;
	}

	bool ret = CAppUtils::IgnoreFile(pathList,bmask);

	if (parser.HasKey(_T("delete")))
	{
		int key;

		CString format;

		if(CMessageBox::Show(hwndExplorer, IDS_PROC_KEEPFILELOCAL, IDS_APPNAME, MB_ICONERROR|MB_YESNO) == IDYES)
		{
			format= _T("git.exe update-index --force-remove -- \"%s\"");
		}
		else
		{
			format=_T("git.exe rm -r -f -- \"%s\"");
		}

		CString output;
		CString cmd;
		int nPath;
		for(nPath = 0; nPath < pathList.GetCount(); nPath++)
		{

			cmd.Format(format,pathList[nPath].GetGitPathString());
			if (g_Git.Run(cmd, &output, CP_UTF8))
			{
				key = MessageBox(hwndExplorer, output, _T("TortoiseGit"), MB_ICONERROR | MB_OKCANCEL);
				if(key == IDCANCEL)
					return FALSE;

			}
		}

		output.Format(IDS_PROC_FILESREMOVED, nPath);

		CShellUpdater::Instance().AddPathsForUpdate(pathList);

		MessageBox(hwndExplorer, output, _T("TortoiseGit"), MB_ICONINFORMATION | MB_OK);
	}

	CShellUpdater::Instance().AddPathsForUpdate(orgPathList);
	CShellUpdater::Instance().Flush();

	return ret;
#if 0
	CString filelist;
	BOOL err = FALSE;
	for(int nPath = 0; nPath < pathList.GetCount(); nPath++)
	{
		CString name = CPathUtils::PathPatternEscape(pathList[nPath].GetFileOrDirectoryName());
		if (parser.HasKey(_T("onlymask")))
		{
			name = _T("*")+pathList[nPath].GetFileExtension();
		}
		filelist += name + _T("\n");
		CTSVNPath parentfolder = pathList[nPath].GetContainingDirectory();
		SVNProperties props(parentfolder, SVNRev::REV_WC, false);
		CStringA value;
		for (int i=0; i<props.GetCount(); i++)
		{
			CString propname(props.GetItemName(i).c_str());
			if (propname.CompareNoCase(_T("svn:ignore"))==0)
			{
				//treat values as normal text even if they're not
				value = (char *)props.GetItemValue(i).c_str();
			}
		}
		if (value.IsEmpty())
			value = name;
		else
		{
			// make sure we don't have duplicate entries
			std::set<CStringA> ignoreItems;
			ignoreItems.insert(CUnicodeUtils::GetUTF8(name));
			CStringA token;
			int curPos = 0;
			token= value.Tokenize("\n",curPos);
			while (token != _T(""))
			{
				token.Trim();
				ignoreItems.insert(token);
				token = value.Tokenize("\n", curPos);
			};
			value.Empty();
			for (std::set<CStringA>::iterator it = ignoreItems.begin(); it != ignoreItems.end(); ++it)
			{
				value += *it;
				value += "\n";
			}
		}
		if (!props.Add(_T("svn:ignore"), (LPCSTR)value))
		{
			CString temp;
			temp.Format(IDS_ERR_FAILEDIGNOREPROPERTY, (LPCTSTR)name);
			temp += _T("\n");
			temp += props.GetLastErrorMsg().c_str();
			CMessageBox::Show(hwndExplorer, temp, _T("TortoiseGit"), MB_ICONERROR);
			err = TRUE;
			break;
		}
	}
	if (err == FALSE)
	{
		CString temp;
		temp.Format(IDS_PROC_IGNORESUCCESS, (LPCTSTR)filelist);
		CMessageBox::Show(hwndExplorer, temp, _T("TortoiseGit"), MB_ICONINFORMATION);
		return true;
	}
#endif
}