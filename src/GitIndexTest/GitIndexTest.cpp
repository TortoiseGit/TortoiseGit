// GitIndexTest.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "gitstatus.h"
#include "gitindex.h"

int _tmain(int argc, _TCHAR* argv[])
{
	if(argc != 2)
	{
		_tprintf(_T("argument error \n"));
		return 0;
	}

	CGitIndexList list;
	list.ReadIndex(argv[1]);
	for(int i=0;i<list.size();i++)
		list[i].Print();

	CGitHeadFileList filelist;
	CString str;
	
	filelist.ReadHeadHash(_T("d:\\gitest"));
	_tprintf(_T("update %d\n"), filelist.CheckHeadUpdate());
	return 0;
}

