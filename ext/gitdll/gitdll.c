// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2015 - TortoiseGit

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
// gitdll.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "../build/libgit-defines.h"
#pragma warning(push)
#pragma warning(disable: 4100 4018 4127 4244 4267)
#include "git-compat-util.h"
#include "gitdll.h"
#include "cache.h"
#include "commit.h"
#include "diff.h"
#include "revision.h"
#include "diffcore.h"
#include "dir.h"
#include "builtin.h"
#include "exec_cmd.h"
#include "cache.h"
#include "quote.h"
#include "run-command.h"
#include "mailmap.h"
#pragma warning(pop)

extern char g_last_error[];
const char * g_prefix;

extern void die_dll(const char *err, va_list params);
extern int die_is_recursing_dll(void);

extern void free_all_pack();
extern void reset_git_env();
extern void drop_attr_stack();
extern void git_atexit_dispatch();
extern void git_atexit_clear();
extern void invalidate_ref_cache(const char* submodule);
extern void cmd_log_init(int argc, const char** argv, const char* prefix, struct rev_info* rev, struct setup_revision_opt* opt);
extern int estimate_commit_count(struct rev_info* rev, struct commit_list* list);
extern int log_tree_commit(struct rev_info*, struct commit*);
extern int write_entry(struct cache_entry* ce, char* path, const struct checkout* state, int to_tempfile);
extern struct object* deref_tag(struct object* o, const char* warn, int warnlen);
extern void diff_flush_stat(struct diff_filepair* p, struct diff_options* o, struct diffstat_t* diffstat);
extern void free_diffstat_info(struct diffstat_t* diffstat);
extern int for_each_reflog_ent(const char* refname, each_reflog_ent_fn fn, void* cb_data);
extern int for_each_ref_in(const char* prefix, each_ref_fn fn, void* cb_data);

void dll_entry()
{
	set_die_routine(die_dll);
	set_die_is_recursing_routine(die_is_recursing_dll);
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
	return 0;
}

int git_init()
{
	char path[MAX_PATH+1];
	size_t homesize;

	_fmode = _O_BINARY;
	_setmode(_fileno(stdin), _O_BINARY);
	_setmode(_fileno(stdout), _O_BINARY);
	_setmode(_fileno(stderr), _O_BINARY);

	// set HOME if not set already
	getenv_s(&homesize, NULL, 0, "HOME");
	if (!homesize)
	{
		_wputenv_s(L"HOME", wget_windows_home_directory());
	}
	GetModuleFileName(NULL, path, MAX_PATH);
	convert_slash(path);

	git_extract_argv0_path(path);
	reset_git_env();
	drop_attr_stack();
	g_prefix = setup_git_directory();
	git_config(git_default_config, NULL);

	if (!homesize)
	{
		_putenv_s("HOME","");/* clear home evironment to avoid affact third part software*/
	}

	return 0;
}

static int git_parse_commit_author(struct GIT_COMMIT_AUTHOR* author, const char* pbuff)
{
	const char* end;

	author->Name=pbuff;
	end=strchr(pbuff,'<');
	if( end == 0)
	{
		return -1;
	}
	author->NameSize = (int)(end - pbuff - 1);

	pbuff = end +1;
	end = strchr(pbuff, '>');
	if( end == 0)
		return -1;

	author->Email = pbuff ;
	author->EmailSize = (int)(end - pbuff);

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
	const char* pbuf;
	const char* end;
	struct commit *p;

	p= (struct commit *)commit->m_pGitCommit;

	memcpy(commit->m_hash,p->object.sha1,GIT_HASH_SIZE);

	commit->m_Encode = NULL;
	commit->m_EncodeSize = 0;

	commit->buffer = detach_commit_buffer(commit->m_pGitCommit, NULL);

	pbuf = commit->buffer;
	while(pbuf)
	{
		if (strncmp(pbuf, "author", 6) == 0)
		{
			ret = git_parse_commit_author(&commit->m_Author,pbuf + 7);
			if(ret)
				return -4;
		}
		else if (strncmp(pbuf, "committer", 9) == 0)
		{
			ret =  git_parse_commit_author(&commit->m_Committer,pbuf + 10);
			if(ret)
				return -5;

			pbuf = strchr(pbuf,'\n');
			if(pbuf == NULL)
				return -6;
		}
		else if (strncmp(pbuf, "encoding", 8) == 0)
		{
			pbuf += 9;
			commit->m_Encode=pbuf;
			end = strchr(pbuf,'\n');
			commit->m_EncodeSize= (int)(end -pbuf);
		}

		// the headers end after the first empty line
		else if (*pbuf == '\n')
		{
			pbuf++;

			commit->m_Subject=pbuf;
			end = strchr(pbuf,'\n');
			if( end == 0)
				commit->m_SubjectSize = (int)strlen(pbuf);
			else
			{
				commit->m_SubjectSize = (int)(end - pbuf);
				pbuf = end +1;
				commit->m_Body = pbuf;
				commit->m_BodySize = (int)strlen(pbuf);
				return 0;
			}
		}

		pbuf = strchr(pbuf,'\n');
		if(pbuf)
			pbuf ++;
	}
	return 0;
}

int git_get_commit_from_hash(GIT_COMMIT* commit, const GIT_HASH hash)
{
	int ret = 0;

	struct commit *p;

	if (commit == NULL)
		return -1;

	memset(commit,0,sizeof(GIT_COMMIT));

	commit->m_pGitCommit = p = lookup_commit(hash);

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
	struct commit_list *l;
	if (list == NULL)
		return -1;

	l = *(struct commit_list **)list;
	if (l == NULL)
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

	if (p->tree)
		free_tree_buffer(p->tree);

#pragma warning(push)
#pragma warning(disable: 4090)
	if (commit->buffer)
		free(commit->buffer);
#pragma warning(pop)

	p->object.parsed = 0;
	p->parents = 0;
	p->tree = 0;

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
		if(*arg != ' ')
		{
			char space=' ';
			argv[i]=p;

			while(*arg)
			{
				if(*arg == '"')
				{
					arg++;
					if(space == ' ')
						space = '"';
					else
						space = ' ';
				}
				if((*arg == space) || (*arg == 0))
					break;

				*p++ = *arg++;
			}
			i++;
			*p++=0;
		}
		if(*arg == 0)
			break;
		arg++;
	}
	argv[i]=NULL;
	*size = i;
	return argv;
}
int git_open_log(GIT_LOG * handle, char * arg)
{
	struct rev_info *p_Rev;
	char ** argv=0;
	int argc=0;
	unsigned int i=0;
	struct setup_revision_opt opt;

	/* clear flags */
	unsigned int obj_size = get_max_object_index();
	for(i =0; i<obj_size; i++)
	{
		struct object *ob= get_indexed_object(i);
		if(ob)
		{
			ob->flags=0;
			if (ob->parsed && ob->type == OBJ_COMMIT)
			{
				struct commit* commit = (struct commit*)ob;
				free_commit_list(commit->parents);
				commit->parents = NULL;
				if (commit->tree)
					free_tree_buffer(commit->tree);
				commit->tree = NULL;
				ob->parsed = 0;
			}
		}
	}

	if(arg != NULL)
		argv = strtoargv(arg,&argc);

	if (!argv)
		return -1;

	p_Rev = malloc(sizeof(struct rev_info));
	if (p_Rev == NULL)
	{
		free(argv);
		return -1;
	}

	memset(p_Rev,0,sizeof(struct rev_info));

	invalidate_ref_cache(NULL);

	init_revisions(p_Rev, g_prefix);
	p_Rev->diff = 1;

	memset(&opt, 0, sizeof(opt));
	opt.def = "HEAD";

	cmd_log_init(argc, argv, g_prefix,p_Rev,&opt);

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

int git_get_log_nextcommit(GIT_LOG handle, GIT_COMMIT *commit, int follow)
{
	int ret =0;

	if(commit == NULL)
		return -1;

	memset(commit, 0, sizeof(GIT_COMMIT));

	commit->m_pGitCommit = get_revision(handle);
	if( commit->m_pGitCommit == NULL)
		return -2;

	if (follow && !log_tree_commit(handle, commit->m_pGitCommit))
	{
		commit->m_ignore = 1;
		return 0;
	}
	commit->m_ignore = 0;

	ret=git_parse_commit(commit);
	if(ret)
		return ret;

	return 0;
}

struct notes_tree **display_notes_trees;
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
	free_all_pack();

	if (display_notes_trees)
		free_notes(*display_notes_trees);
	display_notes_trees = 0;
	return 0;
}

int git_open_diff(GIT_DIFF *diff, char * arg)
{
	struct rev_info *p_Rev;
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
		fclose(p_Rev->diffopt.file);

	free_diffstat_info(&p_Rev->diffstat);
	return 0;
}

int git_root_diff(GIT_DIFF diff, GIT_HASH hash,GIT_FILE *file, int *count, int isstat)
{
	int ret;
	struct rev_info *p_Rev;
	int i;
	struct diff_queue_struct *q = &diff_queued_diff;

	p_Rev = (struct rev_info *)diff;

	ret=diff_root_tree_sha1(hash, "", &p_Rev->diffopt);

	if(ret)
		return ret;

	if(isstat)
	{
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
	}
	return 0;
}

int git_do_diff(GIT_DIFF diff, GIT_HASH hash1, GIT_HASH hash2, GIT_FILE * file, int *count,int isstat)
{
	struct rev_info *p_Rev;
	int ret;
	int i;
	struct diff_queue_struct *q = &diff_queued_diff;

	p_Rev = (struct rev_info *)diff;

	ret = diff_tree_sha1(hash1,hash2,"",&p_Rev->diffopt);
	if( ret )
	{
		free_all_pack();
		return ret;
	}

	if(isstat)
	{
		diffcore_std(&p_Rev->diffopt);
		memset(&p_Rev->diffstat, 0, sizeof(struct diffstat_t));
		for (i = 0; i < q->nr; i++) {
			struct diff_filepair *p = q->queue[i];
			//if (check_pair_status(p))
			diff_flush_stat(p, &p_Rev->diffopt, &p_Rev->diffstat);
		}
	}
	free_all_pack();
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

	assert(newname && oldname && status);

	*newname = q->queue[i]->two->path;
	*oldname = q->queue[i]->one->path;
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
			*inc = (int)p_Rev->diffstat.files[j]->added;
		if(dec)
			*dec = (int)p_Rev->diffstat.files[j]->deleted;
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
	ret = read_tree_recursive(root, NULL, 0, 0, NULL, fn, context);
	free_all_pack();
	return ret;
}

int git_add_exclude(const char *string, const char *base,
					int baselen, struct exclude_list *which, int lineno)
{
	add_exclude(string, base, baselen, which, lineno);
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
	return 0;
}

int git_check_excluded_1(const char *pathname,
							int pathlen, const char *basename, int *dtype,
							EXCLUDE_LIST el)
{
	return is_excluded_from_list(pathname, pathlen, basename, dtype, el);
}

int git_get_notes(GIT_HASH hash, char **p_note)
{
	struct strbuf sb;
	size_t size;
	strbuf_init(&sb,0);
	format_display_notes(hash, &sb, "utf-8", 1);
	*p_note = strbuf_detach(&sb,&size);

	return 0;
}

struct cmd_struct {
	const char *cmd;
	int (*fn)(int, const char **, const char *);
	int option;
};

#define RUN_SETUP	(1<<0)

static struct cmd_struct commands[] = {
		{ "notes", cmd_notes, RUN_SETUP },
		{ "update-index", cmd_update_index, RUN_SETUP },
	};

int git_run_cmd(char *cmd, char *arg)
{

	int i=0;
	char ** argv=0;
	int argc=0;

	git_init();

	for(i=0;i<	sizeof(commands) / sizeof(struct cmd_struct);i++)
	{
		if(strcmp(cmd,commands[i].cmd)==0)
		{
			int ret;
			if(arg != NULL)
				argv = strtoargv(arg,&argc);

			ret = commands[i].fn(argc, argv, NULL);

			if(argv)
				free(argv);

			discard_cache();
			free_all_pack();

			return ret;


		}
	}
	return -1;
}

void git_exit_cleanup(void)
{
	git_atexit_dispatch();
	git_atexit_clear();
}

int git_for_each_reflog_ent(const char *ref, each_reflog_ent_fn fn, void *cb_data)
{
	return for_each_reflog_ent(ref,fn,cb_data);
}

static int update_some(const unsigned char* sha1, struct strbuf* base,
		const char *pathname, unsigned mode, int stage, void *context)
{
	struct cache_entry *ce;
	UNREFERENCED_PARAMETER(stage);

	ce = (struct cache_entry *)context;

	if (S_ISDIR(mode))
		return READ_TREE_RECURSIVE;

	hashcpy(ce->sha1, sha1);
	memcpy(ce->name, base->buf, base->len);
	memcpy(ce->name + base->len, pathname, strlen(pathname));
	ce->ce_flags = create_ce_flags((unsigned int)(strlen(pathname) + base->len));
	ce->ce_mode = create_ce_mode(mode);

	return 0;
}

int git_checkout_file(const char* ref, const char* path, char* outputpath)
{
	struct cache_entry *ce;
	int ret;
	GIT_HASH sha1;
	struct tree * root;
	struct checkout state;
	struct pathspec pathspec;
	const char *matchbuf[1];
	ret = get_sha1(ref, sha1);
	if(ret)
		return ret;

	reprepare_packed_git();
	root = parse_tree_indirect(sha1);

	if(!root)
	{
		free_all_pack();
		return -1;
	}

	ce = xcalloc(1, cache_entry_size(strlen(path)));

	matchbuf[0] = NULL;
	parse_pathspec(&pathspec, PATHSPEC_ALL_MAGIC, PATHSPEC_PREFER_CWD, path, matchbuf);
	pathspec.items[0].nowildcard_len = pathspec.items[0].len;
	ret = read_tree_recursive(root, "", 0, 0, &pathspec, update_some, ce);
	free_pathspec(&pathspec);

	if(ret)
	{
		free_all_pack();
		free(ce);
		return ret;
	}
	memset(&state, 0, sizeof(state));
	state.force = 1;
	state.refresh_cache = 0;

	ret = write_entry(ce, outputpath, &state, 0);
	free_all_pack();
	free(ce);
	return ret;
}
struct config_buf
{
	char *buf;
	const char *key;
	size_t size;
	int seen;
};

static int get_config(const char *key_, const char *value_, void *cb)
{
	struct config_buf *buf;
	buf=(struct config_buf*)cb;
	if(strcmp(key_, buf->key))
		return 0;

	if (value_)
		strncpy(buf->buf,value_,buf->size);
	else
	{
		buf->buf[0] = 't';
		buf->buf[1] = 'r';
		buf->buf[2] = 'u';
		buf->buf[3] = 'e';
		buf->buf[4] = 0;
	}
	buf->seen = 1;
	return 0;

}

// wchar_t wrapper for git_etc_gitconfig()
const wchar_t *wget_msysgit_etc(void)
{
	static const wchar_t *etc_gitconfig = NULL;
	wchar_t wpointer[MAX_PATH];

	if (etc_gitconfig)
		return etc_gitconfig;

	if (xutftowcs_path(wpointer, git_etc_gitconfig()) < 0)
		return NULL;

	etc_gitconfig = _wcsdup(wpointer);

	return etc_gitconfig;
}

int git_get_config(const char *key, char *buffer, int size)
{
	char *local, *global, *globalxdg;
	const char *home, *system;
	struct config_buf buf;
	struct git_config_source config_source = { 0 };

	buf.buf=buffer;
	buf.size=size;
	buf.seen = 0;
	buf.key = key;

	home = get_windows_home_directory();
	if (home)
	{
		global = xstrdup(mkpath("%s/.gitconfig", home));
		globalxdg = xstrdup(mkpath("%s/.config/git/config", home));
	}
	else
	{
		global = NULL;
		globalxdg = NULL;
	}

	system = git_etc_gitconfig();

	local = git_pathdup("config");

	if (!buf.seen)
	{
		config_source.file = local;
		git_config_with_options(get_config, &buf, &config_source, 1);
	}
	if (!buf.seen && global)
	{
		config_source.file = global;
		git_config_with_options(get_config, &buf, &config_source, 1);
	}
	if (!buf.seen && globalxdg)
	{
		config_source.file = globalxdg;
		git_config_with_options(get_config, &buf, &config_source, 1);
	}
	if (!buf.seen && system)
	{
		config_source.file = system;
		git_config_with_options(get_config, &buf, &config_source, 1);
	}

	if(local)
		free(local);
	if(global)
		free(global);
	if (globalxdg)
		free(globalxdg);

	return !buf.seen;
}

// taken from msysgit: compat/mingw.c
const char *get_windows_home_directory(void)
{
	static const char *home_directory = NULL;
	struct strbuf buf = STRBUF_INIT;

	if (home_directory)
		return home_directory;

	home_directory = getenv("HOME");
	if (home_directory && *home_directory)
		return home_directory;

	strbuf_addf(&buf, "%s%s", getenv("HOMEDRIVE"), getenv("HOMEPATH"));
	home_directory = strbuf_detach(&buf, NULL);

	return home_directory;
}

// wchar_t wrapper for get_windows_home_directory()
const wchar_t *wget_windows_home_directory(void)
{
	static const wchar_t *home_directory = NULL;
	wchar_t wpointer[MAX_PATH];

	if (home_directory)
		return home_directory;

	if (xutftowcs_path(wpointer, get_windows_home_directory()) < 0)
		return NULL;

	home_directory = _wcsdup(wpointer);

	return home_directory;
}

int get_set_config(const char *key, const char *value, CONFIG_TYPE type)
{
	char * config_exclusive_filename = NULL;
	int ret;

	switch(type)
	{
	case CONFIG_LOCAL:
		config_exclusive_filename  = git_pathdup("config");
		break;
	case CONFIG_GLOBAL:
	case CONFIG_XDGGLOBAL:
		{
			const char *home = get_windows_home_directory();
			if (home)
			{
				if (type == CONFIG_GLOBAL)
					config_exclusive_filename = xstrdup(mkpath("%s/.gitconfig", home));
				else
					config_exclusive_filename = xstrdup(mkpath("%s/.config/git/config", home));
			}
		}
		break;
	}

	if(!config_exclusive_filename)
		return -1;

	ret = git_config_set_multivar_in_file(config_exclusive_filename, key, value, NULL, 0);
	free(config_exclusive_filename);
	return ret;
}

struct mailmap_info {
	char *name;
	char *email;
};

struct mailmap_entry {
	/* name and email for the simple mail-only case */
	char *name;
	char *email;

	/* name and email for the complex mail and name matching case */
	struct string_list namemap;
};

int git_read_mailmap(GIT_MAILMAP *mailmap)
{
	struct string_list *map;
	int result;

	if (!mailmap)
		return -1;

	*mailmap = NULL;
	if ((map = (struct string_list *)calloc(1, sizeof(struct string_list))) == NULL)
		return -1;

	if ((result = read_mailmap(map, NULL)) != 0)
		return result;

	*mailmap = map;
	return 0;
}

const char * git_get_mailmap_author(GIT_MAILMAP mailmap, const char *email2, void *payload, const char *(*author2_cb)(void *))
{
	struct string_list *map;
	int imax, imin = 0;

	if (!mailmap)
		return NULL;

	map = (struct string_list *)mailmap;
	imax = map->nr - 1;
	while (imax >= imin)
	{
		int i = imin + ((imax - imin) / 2);
		struct string_list_item *si = (struct string_list_item *)&map->items[i];
		struct mailmap_entry *me = (struct mailmap_entry *)si->util;
		int comp = strcmp(si->string, email2);

		if (!comp)
		{
			if (me->namemap.nr)
			{
				const char *author2 = author2_cb(payload);
				unsigned int j;
				for (j = 0; j < me->namemap.nr; ++j)
				{
					struct string_list_item *sj = (struct string_list_item *)&me->namemap.items[j];
					struct mailmap_info *mi = (struct mailmap_info *)sj->util;
					
					if (!strcmp(sj->string, author2))
						return mi->name;
				}
			}

			return me->name;
		}
		else if (comp < 0)
			imin = i + 1;
		else
			imax = i - 1;
	}

	return NULL;
}

void git_free_mailmap(GIT_MAILMAP mailmap)
{
	if (!mailmap)
		return;

	clear_mailmap((struct string_list *)mailmap);
	free(mailmap);
}

