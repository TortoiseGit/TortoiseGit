// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2019 - TortoiseGit

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
#pragma warning(disable: 4100 4267)
#include "git-compat-util.h"
#include "gitdll.h"
#include "cache.h"
#include "commit.h"
#include "diff.h"
#include "packfile.h"
#include "revision.h"
#include "diffcore.h"
#include "dir.h"
#include "builtin.h"
#include "config.h"
#include "mailmap.h"
#include "tree.h"
#pragma warning(pop)

extern char g_last_error[];
static const char* g_prefix;

/* also see GitHash.h */
static_assert(GIT_SHA1_RAWSZ <= LIBGIT_GIT_HASH_SIZE && GIT_SHA256_RAWSZ <= LIBGIT_GIT_HASH_SIZE && GIT_MAX_RAWSZ == LIBGIT_GIT_HASH_SIZE, "Required to be equal in gitdll.h");
static_assert(sizeof(struct object_id) == sizeof(struct GIT_OBJECT_OID), "Required to be equal in gitdll.h");

extern NORETURN void die_dll(const char* err, va_list params);
extern void handle_error(const char* err, va_list params);
extern void handle_warning(const char* warn, va_list params);
extern int die_is_recursing_dll(void);

extern void libgit_initialize(void);
extern void cleanup_chdir_notify(void);
extern void free_all_pack(void);
extern void reset_git_env(void);
extern void drop_all_attr_stacks(void);
extern void git_atexit_dispatch(void);
extern void git_atexit_clear(void);
extern void invalidate_ref_cache(void);
extern void cmd_log_init(int argc, const char** argv, const char* prefix, struct rev_info* rev, struct setup_revision_opt* opt);
extern int estimate_commit_count(struct rev_info* rev, struct commit_list* list);
extern int log_tree_commit(struct rev_info*, struct commit*);
extern int write_entry(struct cache_entry* ce, char* path, const struct checkout* state, int to_tempfile);
extern void diff_flush_stat(struct diff_filepair* p, struct diff_options* o, struct diffstat_t* diffstat);
extern void free_diffstat_info(struct diffstat_t* diffstat);
static_assert(sizeof(unsigned long long) == sizeof(timestamp_t), "Required for each_reflog_ent_fn definition in gitdll.h");
extern int for_each_reflog_ent(const char* refname, each_reflog_ent_fn fn, void* cb_data);

void dll_entry(void)
{
	set_die_routine(die_dll);
	set_error_routine(handle_error);
	set_warn_routine(handle_warning);
	set_die_is_recursing_routine(die_is_recursing_dll);
	libgit_initialize();
}

int git_get_sha1(const char *name, GIT_HASH sha1)
{
	struct object_id oid = { 0 };
	int ret = get_oid(name, &oid);
	hashcpy(sha1, oid.hash);
	return ret;
}

int git_init(void)
{
	_fmode = _O_BINARY;
	_setmode(_fileno(stdin), _O_BINARY);
	_setmode(_fileno(stdout), _O_BINARY);
	_setmode(_fileno(stderr), _O_BINARY);

	cleanup_chdir_notify();
	fscache_flush();
	reset_git_env();
	// set HOME if not set already
	gitsetenv("HOME", get_windows_home_directory(), 0);
	drop_all_attr_stacks();
	git_config_clear();
	g_prefix = setup_git_directory();
	git_config(git_default_config, NULL);
	invalidate_ref_cache();

	/* add a safeguard until we have full support in TortoiseGit */
	if (the_repository && the_repository->hash_algo && strcmp(the_repository->hash_algo->name, "sha1") != 0)
		die("Only SHA1 is supported right now.");

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

	memcpy(commit->m_hash, p->object.oid.hash, GIT_SHA1_RAWSZ);

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
			++pbuf;

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
			++pbuf;
	}
	return 0;
}

int git_get_commit_from_hash(GIT_COMMIT* commit, const GIT_HASH hash)
{
	int ret = 0;

	struct commit *p;
	struct object_id oid;

	if (commit == NULL)
		return -1;

	memset(commit,0,sizeof(GIT_COMMIT));

	hashcpy(oid.hash, hash);

	commit->m_pGitCommit = p = lookup_commit(the_repository, &oid);

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

int git_commit_is_root(const GIT_COMMIT* commit)
{
	struct commit* p = commit->m_pGitCommit;
	return (struct commit_list**)p->parents ? 1 : 0;
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
		memcpy(hash, l->item->object.oid.hash, GIT_SHA1_RAWSZ);

	*list = (GIT_COMMIT_LIST *)l->next;
	return 0;

}


int git_free_commit(GIT_COMMIT *commit)
{
	struct commit *p = commit->m_pGitCommit;

	if( p->parents)
		free_commit_list(p->parents);

	if (p->maybe_tree)
		free_tree_buffer(p->maybe_tree);

#pragma warning(push)
#pragma warning(disable: 4090)
	free(commit->buffer);
#pragma warning(pop)

	p->object.parsed = 0;
	p->parents = 0;
	p->maybe_tree = NULL;

	memset(commit,0,sizeof(GIT_COMMIT));
	return 0;
}

static char** strtoargv(const char* arg, int* size)
{
	int count=0;
	const char* parg = arg;
	char **argv;
	char* p;
	int i=0;

	assert(arg && parg);

	while (*parg)
	{
		if (*parg == ' ')
			++count;
		assert(*parg != '\\' && "no backslashes allowed, use a Git path (with slashes) - no escaping of chars possible");
		++parg;
	}

	argv = malloc(strlen(arg) + 2 + (count + 3) * sizeof(char*)); // 1 char* for every parameter + 1 for argv[0] + 1 NULL as end end; and some space for the actual parameters: strlen() + 1 for \0, + 1 for \0 for argv[0]
	if (!argv)
		return NULL;
	p = (char*)(argv + count + 3);

	argv[i++] = p;
	*p++ = '\0';

	parg = arg;
	while (*parg)
	{
		if (*parg != ' ')
		{
			char space=' ';
			argv[i]=p;

			while (*parg)
			{
				if (*parg == '"')
				{
					++parg;
					if(space == ' ')
						space = '"';
					else
						space = ' ';
				}
				if (*parg == space || !*parg)
					break;

				*p++ = *parg++;
			}
			++i;
			*p++=0;
		}
		if (!*parg)
			break;
		++parg;
	}
	argv[i]=NULL;
	*size = i;
	return argv;
}
int git_open_log(GIT_LOG* handle, const char* arg)
{
	struct rev_info *p_Rev;
	char ** argv=0;
	int argc=0;
	struct setup_revision_opt opt;

	/* clear flags */
	unsigned int obj_size = get_max_object_index();
	for (unsigned int i = 0; i < obj_size; ++i)
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
				if (commit->maybe_tree)
					free_tree_buffer(commit->maybe_tree);
				commit->maybe_tree = NULL;
				ob->parsed = 0;
			}
		}
	}

	argv = strtoargv(arg, &argc);
	if (!argv)
		return -1;

	p_Rev = malloc(sizeof(struct rev_info));
	if (p_Rev == NULL)
	{
		free(argv);
		return -1;
	}

	memset(p_Rev,0,sizeof(struct rev_info));

	invalidate_ref_cache();

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
		free(p_Rev->pPrivate);
		free(handle);
	}
	free_all_pack();

	if (display_notes_trees)
		free_notes(*display_notes_trees);
	display_notes_trees = 0;
	return 0;
}

int git_open_diff(GIT_DIFF* diff, const char* arg)
{
	struct rev_info *p_Rev;
	char ** argv=0;
	int argc=0;

	argv = strtoargv(arg, &argc);
	if (!argv)
		return -1;

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
		free(p_Rev->pPrivate);
		free(handle);
	}
	return 0;
}
int git_diff_flush(GIT_DIFF diff)
{
	struct diff_queue_struct *q = &diff_queued_diff;
	struct rev_info *p_Rev;
	p_Rev = (struct rev_info *)diff;

	if(q->nr == 0)
		return 0;

	for (int i = 0; i < q->nr; ++i)
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

int git_root_diff(GIT_DIFF diff, const GIT_HASH hash, GIT_FILE* file, int* count, int isstat)
{
	int ret;
	struct object_id oid;
	struct rev_info *p_Rev;
	struct diff_queue_struct *q = &diff_queued_diff;

	p_Rev = (struct rev_info *)diff;

	hashcpy(oid.hash, hash);

	ret = diff_root_tree_oid(&oid, "", &p_Rev->diffopt);

	if(ret)
		return ret;

	if(isstat)
	{
		diffcore_std(&p_Rev->diffopt);

		memset(&p_Rev->diffstat, 0, sizeof(struct diffstat_t));
		for (int i = 0; i < q->nr; ++i) {
			struct diff_filepair *p = q->queue[i];
			//if (check_pair_status(p))
			diff_flush_stat(p, &p_Rev->diffopt, &p_Rev->diffstat);
		}
	}

	if (file)
		*file = q;
	if (count)
		*count = q->nr;

	return 0;
}

int git_do_diff(GIT_DIFF diff, const GIT_HASH hash1, const GIT_HASH hash2, GIT_FILE* file, int* count, int isstat)
{
	struct rev_info *p_Rev;
	int ret;
	struct object_id oid1, oid2;
	struct diff_queue_struct *q = &diff_queued_diff;

	p_Rev = (struct rev_info *)diff;

	hashcpy(oid1.hash, hash1);
	hashcpy(oid2.hash, hash2);

	ret = diff_tree_oid(&oid1, &oid2, "", &p_Rev->diffopt);
	if( ret )
	{
		free_all_pack();
		return ret;
	}

	if(isstat)
	{
		diffcore_std(&p_Rev->diffopt);
		memset(&p_Rev->diffstat, 0, sizeof(struct diffstat_t));
		for (int i = 0; i < q->nr; ++i) {
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

int git_get_diff_file(GIT_DIFF diff, GIT_FILE file, int i, char** newname, char** oldname, int* IsDir, int* status, int* IsBin, int* inc, int* dec)
{
	struct diff_queue_struct *q = &diff_queued_diff;
	struct rev_info *p_Rev;
	p_Rev = (struct rev_info *)diff;

	q = (struct diff_queue_struct *)file;
	if(file == 0)
		return -1;
	if(i>=q->nr)
		return -1;

	assert(newname && oldname && status && IsDir);

	*newname = q->queue[i]->two->path;
	*oldname = q->queue[i]->one->path;
	*status = q->queue[i]->status;
	if (*status == 'D')
		*IsDir = (q->queue[i]->one->mode & S_IFDIR) == S_IFDIR;
	else
		*IsDir = (q->queue[i]->two->mode & S_IFDIR) == S_IFDIR;

	if (q->queue[i]->one->mode && q->queue[i]->two->mode && DIFF_PAIR_TYPE_CHANGED(q->queue[i]))
		*IsDir = 0;

	if(p_Rev->diffstat.files)
	{
		int j;
		for (j = 0; j < p_Rev->diffstat.nr; ++j)
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

int git_add_exclude(const char *string, const char *base,
					int baselen, EXCLUDE_LIST which, int lineno)
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
	struct exclude_list *p = (struct exclude_list *) which;

	for (int i = 0; i < p->nr; ++i)
	{
		free(p->excludes[i]);
	}
	free(p->excludes);
	free(p);
	return 0;
}

int git_check_excluded_1(const char *pathname,
							int pathlen, const char *basename, int *dtype,
							EXCLUDE_LIST el, int ignorecase)
{
	ignore_case = ignorecase;
	return is_excluded_from_list(pathname, pathlen, basename, dtype, el, NULL);
}

int git_get_notes(const GIT_HASH hash, char** p_note)
{
	struct object_id oid;
	struct strbuf sb;
	size_t size;
	strbuf_init(&sb,0);
	hashcpy(oid.hash, hash);
	format_display_notes(&oid, &sb, "utf-8", 1);
	*p_note = strbuf_detach(&sb,&size);

	return 0;
}

int git_update_index(void)
{
	char** argv = NULL;
	int argc = 0;
	int ret;

	argv = strtoargv("-q --refresh", &argc);
	if (!argv)
		return -1;

	cleanup_chdir_notify();
	drop_all_attr_stacks();

	ret = cmd_update_index(argc, argv, NULL);
	free(argv);

	discard_index(the_repository->index);
	free_all_pack();

	return ret;
}

void git_exit_cleanup(void)
{
	git_atexit_dispatch();
	// do not clear the atexit list, as lots of methods register it just once (and have a local static int flag)
}

int git_for_each_reflog_ent(const char *ref, each_reflog_ent_fn fn, void *cb_data)
{
	return for_each_reflog_ent(ref,fn,cb_data);
}

static int update_some(const struct object_id* sha1, struct strbuf* base, const char* pathname, unsigned mode, int stage, void* context)
{
	struct cache_entry *ce;
	UNREFERENCED_PARAMETER(stage);

	ce = (struct cache_entry *)context;

	if (S_ISDIR(mode))
		return READ_TREE_RECURSIVE;

	oidcpy(&ce->oid, sha1);
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
	struct object_id oid;
	struct tree * root;
	struct checkout state;
	struct pathspec pathspec;
	const char *matchbuf[1];
	ret = get_oid(ref, &oid);
	if(ret)
		return ret;

	reprepare_packed_git(the_repository);
	root = parse_tree_indirect(&oid);

	if(!root)
	{
		free_all_pack();
		return -1;
	}

	ce = xcalloc(1, cache_entry_size(strlen(path)));

	matchbuf[0] = NULL;
	parse_pathspec(&pathspec, PATHSPEC_ALL_MAGIC, PATHSPEC_PREFER_CWD, path, matchbuf);
	pathspec.items[0].nowildcard_len = pathspec.items[0].len;
	ret = read_tree_recursive(the_repository, root, "", 0, 0, &pathspec, update_some, ce);
	clear_pathspec(&pathspec);

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
		strlcpy(buf->buf, value_, buf->size);
	else
	{
		assert(buf->size >= 5);
		buf->buf[0] = 't';
		buf->buf[1] = 'r';
		buf->buf[2] = 'u';
		buf->buf[3] = 'e';
		buf->buf[4] = 0;
	}
	buf->seen = 1;
	return 0;

}

// wchar_t wrapper for program_data_config()
const wchar_t* wget_program_data_config(void)
{
	static const wchar_t *programdata_git_config = NULL;
	wchar_t wpointer[MAX_PATH];

	if (programdata_git_config)
		return programdata_git_config;

	if (xutftowcs_path(wpointer, program_data_config()) < 0)
		return NULL;

	programdata_git_config = _wcsdup(wpointer);

	return programdata_git_config;
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
	const char *home, *system, *programdata;
	struct config_buf buf;
	struct git_config_source config_source = { 0 };

	struct config_options opts = { 0 };
	opts.respect_includes = 1;

	buf.buf=buffer;
	buf.size=size;
	buf.seen = 0;
	buf.key = key;

	if (have_git_dir())
	{
		opts.git_dir = get_git_dir();
		char* local = git_pathdup("config");
		config_source.file = local;
		config_with_options(get_config, &buf, &config_source, &opts);
		free(local);
		if (buf.seen)
			return !buf.seen;
	}

	home = get_windows_home_directory();
	if (home)
	{
		char* global = mkpathdup("%s/.gitconfig", home);
		if (global)
		{
			config_source.file = global;
			config_with_options(get_config, &buf, &config_source, &opts);
			free(global);
			if (buf.seen)
				return !buf.seen;
		}
		char* globalxdg = mkpathdup("%s/.config/git/config", home);
		if (globalxdg)
		{
			config_source.file = globalxdg;
			config_with_options(get_config, &buf, &config_source, &opts);
			free(globalxdg);
			if (buf.seen)
				return !buf.seen;
		}
	}

	system = git_etc_gitconfig();
	if (system)
	{
		config_source.file = system;
		config_with_options(get_config, &buf, &config_source, &opts);
		if (buf.seen)
			return !buf.seen;
	}

	programdata = git_program_data_config();
	if (programdata)
	{
		config_source.file = programdata;
		config_with_options(get_config, &buf, &config_source, &opts);
	}

	return !buf.seen;
}

// taken from msysgit: compat/mingw.c
const char *get_windows_home_directory(void)
{
	static const char *home_directory = NULL;
	const char* tmp;

	if (home_directory)
		return home_directory;

	if ((tmp = getenv("HOME")) != NULL && *tmp)
	{
		home_directory = _strdup(tmp);
		return home_directory;
	}

	if ((tmp = getenv("HOMEDRIVE")) != NULL)
	{
		struct strbuf buf = STRBUF_INIT;
		strbuf_addstr(&buf, tmp);
		if ((tmp = getenv("HOMEPATH")) != NULL)
		{
			strbuf_addstr(&buf, tmp);
			if (is_directory(buf.buf))
			{
				home_directory = strbuf_detach(&buf, NULL);
				return home_directory;
			}
		}
		strbuf_release(&buf);
	}

	if ((tmp = getenv("USERPROFILE")) != NULL && *tmp)
		home_directory = _strdup(tmp);

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
		if (!the_repository || !the_repository->gitdir)
			die("repository not correctly initialized.");
		config_exclusive_filename  = git_pathdup("config");
		break;
	case CONFIG_GLOBAL:
	case CONFIG_XDGGLOBAL:
		{
			const char *home = get_windows_home_directory();
			if (home)
			{
				if (type == CONFIG_GLOBAL)
					config_exclusive_filename = mkpathdup("%s/.gitconfig", home);
				else
					config_exclusive_filename = mkpathdup("%s/.config/git/config", home);
			}
		}
		break;
	}

	if(!config_exclusive_filename)
		return -1;

	ret = git_config_set_multivar_in_file_gently(config_exclusive_filename, key, value, NULL, 0);
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
	{
		clear_mailmap(map);
		free(map);
		return result;
	}

	if (!map->items)
	{
		clear_mailmap(map);
		free(map);
		 
		return -1;
	}

	*mailmap = map;
	return 0;
}

int git_lookup_mailmap(GIT_MAILMAP mailmap, const char** email1, const char** name1, const char* email2, void* payload, const char *(*author2_cb)(void*))
{
	struct string_list *map;
	int imax, imin = 0;

	if (!mailmap)
		return -1;

	map = (struct string_list *)mailmap;
	imax = map->nr - 1;
	while (imax >= imin)
	{
		int i = imin + ((imax - imin) / 2);
		struct string_list_item *si = (struct string_list_item *)&map->items[i];
		struct mailmap_entry *me = (struct mailmap_entry *)si->util;
		int comp = map->cmp(si->string, email2);

		if (!comp)
		{
			if (me->namemap.nr)
			{
				const char *author2 = author2_cb(payload);
				for (unsigned int j = 0; j < me->namemap.nr; ++j)
				{
					struct string_list_item *sj = (struct string_list_item *)&me->namemap.items[j];
					struct mailmap_info *mi = (struct mailmap_info *)sj->util;

					if (!map->cmp(sj->string, author2))
					{
						if (email1)
							*email1 = mi->email;
						if (name1)
							*name1 = mi->name;
						return 0;
					}
				}
			}

			if (email1)
				*email1 = me->email;
			if (name1)
				*name1 = me->name;
			return 0;
		}
		else if (comp < 0)
			imin = i + 1;
		else
			imax = i - 1;
	}

	return -1;
}

void git_free_mailmap(GIT_MAILMAP mailmap)
{
	if (!mailmap)
		return;

	clear_mailmap((struct string_list *)mailmap);
	free(mailmap);
}

// just for regression tests
int git_mkdir(const char* path)
{
	return mkdir(path, 0);
}
