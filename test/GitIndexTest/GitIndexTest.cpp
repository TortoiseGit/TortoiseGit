// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2010 - TortoiseGit

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

// GitIndexTest.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "gitstatus.h"
#include "gitindex.h"
#include "gitdll.h"
#include "TGitPath.h"

int _tmain(int argc, _TCHAR* argv[])
{
	CTGitPath path;
	CString root;
	TCHAR buff[256];
	
	GetCurrentDirectory(256,buff);
	path.SetFromWin(buff);

	if(!path.HasAdminDir(&root))
	{
		printf("not in git repository\n");
		return -1;
	}

	CGitIndexList list;
	list.ReadIndex(root+_T("\\.git\\index"));

	CGitHeadFileList filelist;
	filelist.ReadHeadHash(buff);
	_tprintf(_T("update %d\n"), filelist.CheckHeadUpdate());

	git_init();
//	filelist.ReadTree();

	WIN32_FIND_DATA data;
	CString str(buff);
	str+=_T("\\*.*");
	GitStatus status;

	HANDLE handle = FindFirstFile(str,&data);
	while(FindNextFile(handle,&data))
	{
		if( _tcsnccmp(data.cFileName, _T(".."),2) ==0) 
			continue;
		if( _tcsnccmp(data.cFileName, _T("."),1) ==0 ) 
			continue;

		CString spath(buff);
		spath += _T("\\");
		spath += data.cFileName;
		CTGitPath path(spath);

		TCHAR name[100];
		int t1,t2;
		t1 = ::GetCurrentTime();
		status.GetStatusString(status.GetAllStatus(path), 100,name);
		t2 = ::GetCurrentTime();

		_tprintf(_T("%s - %s - %d\n"),data.cFileName, name, t2-t1);
		
	}

	return 0;
}

