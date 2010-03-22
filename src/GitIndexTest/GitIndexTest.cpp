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

