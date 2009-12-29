// gitdlltest.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "gitdll.h"

int output(int ret, char * name)
{
	if(ret)
		printf("Fail \t%s\r\n",name);
	else
		printf("Success\t%s\r\n",name);
	return 0;
}
int _tmain(int argc, _TCHAR* argv[])
{
	GIT_HASH hash;
	GIT_COMMIT commit;
	char *buf;
	int size;
	memset(&hash,0,sizeof(GIT_HASH));
	int ret;
	ret=git_init();
	output(ret,"git_init");
	ret=git_get_sha1("master",hash);
	output(ret,"git_get_sha1");
	ret=git_get_sha1("head",hash);
	output(ret,"git_get_sha1");
	ret=git_get_commit_from_hash(&commit, hash);
	output(ret,"git_get_commit_from_hash");

	return ret;
}

