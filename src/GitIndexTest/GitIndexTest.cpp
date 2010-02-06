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
	filelist.ReadTree();

	return 0;
}

