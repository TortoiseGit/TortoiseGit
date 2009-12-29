// gitdll.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "gitdll.h"
#include "cache.h"
#include "commit.h"
#if 0

// This is an example of an exported variable
GITDLL_API int ngitdll=0;

// This is an example of an exported function.
GITDLL_API int fngitdll(void)
{
	return 42;
}

// This is the constructor of a class that has been exported.
// see gitdll.h for the class definition
Cgitdll::Cgitdll()
{
	return;
}
#endif

#define MAX_ERROR_STR_SIZE 512
char g_last_error[MAX_ERROR_STR_SIZE]={0};

char * get_git_last_error()
{
	return g_last_error;
}

static void die_dll(const char *err, va_list params)
{
	memset(g_last_error,0,MAX_ERROR_STR_SIZE);
	vsnprintf(g_last_error, MAX_ERROR_STR_SIZE-1, err, params);	
}

void dll_entry()
{
	set_die_routine(die_dll);
}

int git_get_sha1(const char *name, GIT_HASH sha1)
{
	return get_sha1(name,sha1);
}

static int convert_slash(char * path)
{
	while(*path)
	{
		if(*path == '\\' )
			*path = '/';
		path++;
	}
}

int git_init()
{
	char *home;
	char path[MAX_PATH+1];
	char *prefix;
	size_t homesize,size,httpsize;

	// set HOME if not set already
	getenv_s(&homesize, NULL, 0, "HOME");
	if (!homesize)
	{
		_dupenv_s(&home,&size,"USERPROFILE"); 
		_putenv_s("HOME",home);
		free(home);
	}
	GetModuleFileName(NULL, path, MAX_PATH);
	convert_slash(path);

	git_extract_argv0_path(path);
	prefix = setup_git_directory();
	return git_config(git_default_config, NULL);
}

static int git_parse_commit_author(struct GIT_COMMIT_AUTHOR *author, char *pbuff)
{
	char *end;

	author->Name=pbuff;
	end=strchr(pbuff,'<');
	if( end == 0)
	{
		return -1;
	}
	author->NameSize = end - pbuff - 1;

	pbuff = end +1;
	end = strchr(pbuff, '>');
	if( end == 0)
		return -1;

	author->Email = pbuff ;
	author->EmailSize = end - pbuff;

	pbuff = end + 2;

	author->Date = atol(pbuff);
	end =  strchr(pbuff, ' ');
	if( end == 0 )
		return -1;

	pbuff=end;
	author->TimeZone = atol(pbuff);

	return 0;
}

static int git_parse_commit(GIT_COMMIT *commit)
{
	int ret = 0;
	char *pbuf;
	char *end;
	struct commit *p;

	p= (struct commit *)commit->m_pGitCommit;

	memcpy(commit->m_hash,p->object.sha1,GIT_HASH_SIZE);

	if(p->buffer == NULL)
		return -1;

	pbuf = p->buffer;
	while(pbuf)
	{
		if( strncmp(pbuf,"author",6) == 0)
		{
			ret = git_parse_commit_author(&commit->m_Author,pbuf + 7);
			if(ret)
				return ret;
		}
		if( strncmp(pbuf, "committer",9) == 0)
		{
			ret =  git_parse_commit_author(&commit->m_Committer,pbuf + 10);
			if(ret)
				return ret;

			pbuf = strchr(pbuf,'\n');
			if(pbuf == NULL)
				return -1;

			while((*pbuf) && (*pbuf == '\n'))
				pbuf ++;

			commit->m_Subject=pbuf;
			end = strchr(pbuf,'\n');
			if( end == 0)
				commit->m_SubjectSize = strlen(pbuf);
			else
			{
				commit->m_SubjectSize = end - pbuf;
				pbuf = end +1;
				commit->m_Body = pbuf;
				commit->m_BodySize = strlen(pbuf);
				return 0;
			}

		}

		pbuf = strchr(pbuf,'\n');
		if(pbuf)
			pbuf ++;
	}

}

int git_get_commit_from_hash(GIT_COMMIT *commit, GIT_HASH hash)
{
	int ret = 0;
	
	struct commit *p = (struct commit*)commit;
	commit->m_pGitCommit = p = lookup_commit(hash);

	if(commit == NULL)
		return -1;
	
	if(p == NULL)
		return -1;
	
	ret = parse_commit(p);
	if( ret )
		return ret;

	return git_parse_commit(commit);
}

