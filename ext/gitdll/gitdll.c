// gitdll.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "git-compat-util.h"
#include "msvc.h"
#include "gitdll.h"
#include "cache.h"
#include "commit.h"
#include "diff.h"
#include "revision.h"
#include "diffcore.h"
#include "dir.h"

const char git_version_string[] = GIT_VERSION;


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

extern char g_last_error[];
void * g_prefix;

char * get_git_last_error()
{
	return g_last_error;
}

extern void die_dll(const char *err, va_list params);

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
	int ret;
	size_t homesize,size,httpsize;

	_fmode = _O_BINARY; 
	_setmode(_fileno(stdin), _O_BINARY); 
	_setmode(_fileno(stdout), _O_BINARY); 
	_setmode(_fileno(stderr), _O_BINARY); 

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
	g_prefix = prefix = setup_git_directory();
	ret = git_config(git_default_config, NULL);

	if (!homesize)
	{
		_putenv_s("HOME","");/* clear home evironment to avoid affact third part software*/
	}

	return ret;
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

int git_parse_commit(GIT_COMMIT *commit)
{
	int ret = 0;
	char *pbuf;
	char *end;
	struct commit *p;

	p= (struct commit *)commit->m_pGitCommit;

	memcpy(commit->m_hash,p->object.sha1,GIT_HASH_SIZE);

	commit->m_Encode = NULL;
	commit->m_EncodeSize = 0;

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

			if( strncmp(pbuf, "encoding",8) == 0 )
			{
				pbuf += 9;
				commit->m_Encode=pbuf;
				end = strchr(pbuf,'\n');
				commit->m_EncodeSize=end -pbuf;

				pbuf = end +1;
				while((*pbuf) && (*pbuf == '\n'))
					pbuf ++;
			}
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
	
	struct commit *p;
	
	memset(commit,0,sizeof(GIT_COMMIT));

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

int git_get_commit_first_parent(GIT_COMMIT *commit,GIT_COMMIT_LIST *list)
{
	struct commit *p = commit->m_pGitCommit;

	if(list == NULL)
		return -1;
	
	*list = (GIT_COMMIT_LIST*)p->parents;
	return 0;
}
int git_get_commit_next_parent(GIT_COMMIT_LIST *list, GIT_HASH hash)
{
	struct commit_list *l = *(struct commit_list **)list;
	if(list == NULL || l==NULL)
		return -1;

	if(hash)
		memcpy(hash, l->item->object.sha1, GIT_HASH_SIZE);

	*list = (GIT_COMMIT_LIST *)l->next;
	return 0;

}


int git_free_commit(GIT_COMMIT *commit)
{
	struct commit *p = commit->m_pGitCommit;

	if( p->parents)
		free_commit_list(p->parents);	

	if( p->buffer )
	{
		free(p->buffer);
		p->buffer=NULL;
		p->object.parsed=0;
		p->parents=0;
		p->tree=0;
	}
	memset(commit,0,sizeof(GIT_COMMIT));
	return 0;
}

char **strtoargv(char *arg, int *size)
{
	int count=0;
	char *p=arg;
	char **argv;
	
	int i=0;
	while(*p)
	{
		if(*p == '\\')
			*p='/';
		p++;
	}
	p=arg;

	while(*p)
	{
		if(*p == ' ')
			count ++;
		p++;
	}
	
	argv=malloc(strlen(arg)+1 + (count +2)*sizeof(void*));
	p=(char*)(argv+count+2);

	while(*arg)
	{
		if(*arg == '"')
		{
			argv[i] = p;
			arg++;
			*p=*arg;
			while(*arg && *arg!= '"')
				*p++=*arg++;
			*p++=0;
			arg++;
			i++;
			if(*arg == 0)
				break;
		}
		if(*arg != ' ')
		{
			argv[i]=p;
			while(*arg && *arg !=' ')
				*p++ = *arg++;
			i++;
			*p++=0;
		}
		arg++;
	}
	argv[i]=NULL;
	*size = i;
	return argv;
}
int git_open_log(GIT_LOG * handle, char * arg)
{
	struct rev_info *p_Rev;
	int size;
	char ** argv=0;
	int argc=0;
	int i=0;

	/* clear flags */
	unsigned int obj_size = get_max_object_index();
	for(i =0; i<obj_size; i++)
	{
		struct object *ob= get_indexed_object(i);
		if(ob)
			ob->flags=0;
	}

	if(arg != NULL)
		argv = strtoargv(arg,&argc);

	p_Rev = malloc(sizeof(struct rev_info));
	memset(p_Rev,0,sizeof(struct rev_info));

	if(p_Rev == NULL)
		return -1;

	init_revisions(p_Rev, g_prefix);
	p_Rev->diff = 1;
		
	cmd_log_init(argc, argv, g_prefix,p_Rev);

	p_Rev->pPrivate = argv;
	*handle = p_Rev;
	return 0;

}
int git_get_log_firstcommit(GIT_LOG handle)
{
	return prepare_revision_walk(handle);
}

int git_get_log_estimate_commit_count(GIT_LOG handle)
{
	struct rev_info *p_Rev;
	p_Rev=(struct rev_info *)handle;

	return estimate_commit_count(p_Rev, p_Rev->commits);
}

int git_get_log_nextcommit(GIT_LOG handle, GIT_COMMIT *commit)
{
	int ret =0;
	
	if(commit == NULL)
		return -1;

	memset(commit, 0, sizeof(GIT_COMMIT));

	commit->m_pGitCommit = get_revision(handle);
	if( commit->m_pGitCommit == NULL)
		return -2;
	
	ret=git_parse_commit(commit);
	if(ret)
		return ret;

	return 0;
}

int git_close_log(GIT_LOG handle)
{
	if(handle)
	{
		struct rev_info *p_Rev;
		p_Rev=(struct rev_info *)handle;
		if(p_Rev->pPrivate)
			free(p_Rev->pPrivate);
		free(handle);
	}
	
	return 0;
}

int git_open_diff(GIT_DIFF *diff, char * arg)
{
	struct rev_info *p_Rev;
	int size;
	char ** argv=0;
	int argc=0;
		
	if(arg != NULL)
		argv = strtoargv(arg,&argc);

	p_Rev = malloc(sizeof(struct rev_info));
	memset(p_Rev,0,sizeof(struct rev_info));

	p_Rev->pPrivate = argv;
	*diff = (GIT_DIFF)p_Rev;

	init_revisions(p_Rev, g_prefix);
	git_config(git_diff_basic_config, NULL); /* no "diff" UI options */
	p_Rev->abbrev = 0;
	p_Rev->diff = 1;
	argc = setup_revisions(argc, argv, p_Rev, NULL);

	return 0;
}
int git_close_diff(GIT_DIFF handle)
{
	git_diff_flush(handle);
	if(handle)
	{
		struct rev_info *p_Rev;
		p_Rev=(struct rev_info *)handle;
		if(p_Rev->pPrivate)
			free(p_Rev->pPrivate);
		free(handle);
	}
	return 0;
}
int git_diff_flush(GIT_DIFF diff)
{
	struct diff_queue_struct *q = &diff_queued_diff;
	struct rev_info *p_Rev;
	int i;
	p_Rev = (struct rev_info *)diff;
	
	if(q->nr == 0)
		return 0;

	for (i = 0; i < q->nr; i++)
		diff_free_filepair(q->queue[i]);

	if(q->queue)
	{
		free(q->queue);
		q->queue = NULL;
		q->nr = q->alloc = 0;
	}

	if (p_Rev->diffopt.close_file)
		fclose(p_Rev->diffopt.close_file);

	free_diffstat_info(&p_Rev->diffstat);
}

int git_root_diff(GIT_DIFF diff, GIT_HASH hash,GIT_FILE *file, int *count)
{
	int ret;
	struct rev_info *p_Rev;
	int i;
	struct diff_queue_struct *q = &diff_queued_diff;

	p_Rev = (struct rev_info *)diff;

	ret=diff_root_tree_sha1(hash, "", &p_Rev->diffopt);

	if(ret)
		return ret;

	diffcore_std(&p_Rev->diffopt);

	memset(&p_Rev->diffstat, 0, sizeof(struct diffstat_t));
	for (i = 0; i < q->nr; i++) {
		struct diff_filepair *p = q->queue[i];
		//if (check_pair_status(p))
		diff_flush_stat(p, &p_Rev->diffopt, &p_Rev->diffstat);
	}

	if(file)
		*file = q;
	if(count)
		*count = q->nr;

	return 0;
}

int git_diff(GIT_DIFF diff, GIT_HASH hash1, GIT_HASH hash2, GIT_FILE * file, int *count)
{
	struct rev_info *p_Rev;
	int ret;
	int i;
	struct diff_queue_struct *q = &diff_queued_diff;
	
	p_Rev = (struct rev_info *)diff;

	ret = diff_tree_sha1(hash1,hash2,"",&p_Rev->diffopt);
	if( ret )
		return ret;
	
	diffcore_std(&p_Rev->diffopt);

	memset(&p_Rev->diffstat, 0, sizeof(struct diffstat_t));
	for (i = 0; i < q->nr; i++) {
		struct diff_filepair *p = q->queue[i];
		//if (check_pair_status(p))
		diff_flush_stat(p, &p_Rev->diffopt, &p_Rev->diffstat);
	}

	if(file)
		*file = q;
	if(count)
		*count = q->nr;
	return 0;
}

int git_get_diff_file(GIT_DIFF diff,GIT_FILE file,int i, char **newname, char ** oldname,  int *status, int *IsBin, int *inc, int *dec)
{
	struct diff_queue_struct *q = &diff_queued_diff;
	struct rev_info *p_Rev;
	p_Rev = (struct rev_info *)diff;
	
	q = (struct diff_queue_struct *)file;
	if(file == 0)
		return -1;
	if(i>=q->nr)
		return -1;

	if(newname)
		*newname = q->queue[i]->two->path;

	if(oldname)
		*oldname = q->queue[i]->one->path;

	if(status)
		*status = q->queue[i]->status;

	if(p_Rev->diffstat.files)
	{
		int j;
		for(j=0;j<p_Rev->diffstat.nr;j++)
		{
			if(strcmp(*newname,p_Rev->diffstat.files[j]->name)==0)
				break;
		}
		if( j== p_Rev->diffstat.nr)
		{
			*IsBin=1;
			*inc=0;
			*dec=0;
			return 0;
		}
		if(IsBin)
			*IsBin = p_Rev->diffstat.files[j]->is_binary;
		if(inc)
			*inc = p_Rev->diffstat.files[j]->added;
		if(dec)
			*dec = p_Rev->diffstat.files[j]->deleted;
	}else
	{
		*IsBin=1;
		*inc=0;
		*dec=0;
	}

	return 0;
}

int git_read_tree(GIT_HASH hash,read_tree_fn_t fn, void *context)
{
	struct tree * root;
	int ret;
	reprepare_packed_git();
	root = parse_tree_indirect(hash);

	if (!root)
	{
		free_all_pack();
		return -1;
	}
	ret = read_tree_recursive(root,NULL,NULL,0,NULL,fn,context);
	free_all_pack();
	return ret;
}

int git_add_exclude(const char *string, const char *base,
					int baselen, struct exclude_list *which)
{
	add_exclude(string, base, baselen, which);
	return 0;
}

int git_create_exclude_list(EXCLUDE_LIST *which)
{
	*which = malloc(sizeof(struct exclude_list));
	memset(*which,0,sizeof(struct exclude_list));
	return 0;
}

int git_free_exclude_list(EXCLUDE_LIST which)
{
	int i=0;
	struct exclude_list *p = (struct exclude_list *) which;
	
	for(i=0; i<p->nr;i++)
	{
		free(p->excludes[i]);
	}
	free(p->excludes);
	free(p);
}

int git_check_excluded_1(const char *pathname,
							int pathlen, const char *basename, int *dtype,
							EXCLUDE_LIST el)
{
	return excluded_1(pathname, pathlen, basename,dtype,el);
}