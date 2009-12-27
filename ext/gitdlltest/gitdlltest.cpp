// gitdlltest.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "gitdll.h"

int _tmain(int argc, _TCHAR* argv[])
{
	unsigned char hash[20];
	int ret;
	ret=git_init();
	ret=git_get_sha1("master",hash);
	return ret;
}

