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
	int count;
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

	GIT_COMMIT_LIST list;
	GIT_HASH outhash;
	int i=0;
	ret = git_get_commit_first_parent(&commit,&list);
	output(ret, "git_get_commit_first_parent");
	while(list)
	{
		i++;
		ret = git_get_commit_next_parent(&list, outhash);
	}
	printf("parent count %d\r\n",i);

	GIT_DIFF diff;
	ret = git_open_diff(&diff, "-M -C --stat");
	output(ret, "git_open_diff");
	
	GIT_FILE file;

	ret = git_diff(diff, outhash, commit.m_hash,&file, &count);
	output(ret, "git_diff");

	for(i =0;i<count;i++)
	{	
		char * newname;
		char * oldname;
		int status;
		int IsBin;
		int inc, dec;
		ret = git_get_diff_file(diff,file,i,&newname, &oldname, &status,
			&IsBin, &inc, &dec);
	}
	ret = git_diff_flush(diff);
	output(ret, "git_diff_flush");
//	ret = git_close_diff(diff);
//	output(ret, "git_close_diff");


	git_free_commit(&commit);


	GIT_HANDLE handle;
	ret=git_open_log(&handle,"--stat -c -- \"build.txt\"");
	output(ret,"git_open_log");
	ret=git_get_log_firstcommit(handle);
	output(ret,"git_get_log_firstcommit");
	count = 0;
	while( git_get_log_nextcommit(handle,&commit) == 0)
	{
		//printf("%s\r\n",commit.m_Subject);
		count ++;
		git_free_commit(&commit);
	}
	printf("commit number %d\r\n",count);
	ret=git_close_log(handle);


	output(ret,"git_close_log");
	return ret;
}

