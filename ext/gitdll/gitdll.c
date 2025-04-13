// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2025 - TortoiseGit

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
#define USE_THE_REPOSITORY_VARIABLE
#include "../build/libgit-defines.h"
#pragma warning(push)
#pragma warning(disable: 4100 4267)
#include "git-compat-util.h"
#include "read-cache.h"
#include "object-name.h"
#include "entry.h"
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
#include "environment.h"
#include "setup.h"
#include "path.h"
#include "notes.h"
#include "gitdll.h"
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
extern void free_all_pack(struct repository* repo);
extern void reset_git_env(const LPWSTR*);
extern void drop_all_attr_stacks(void);
extern void git_atexit_dispatch(void);
extern void git_atexit_clear(void);
extern void ref_store_release_and_clear(struct repository* repo);
extern void clear_ref_decorations(void);
extern void cmd_log_init_tgit(int argc, const char** argv, const char* prefix, struct rev_info* rev, struct setup_revision_opt* opt);
extern int estimate_commit_count(struct commit_list* list);
extern int log_tree_commit(struct rev_info*, struct commit*);
extern int write_entry(struct cache_entry* ce, char* path, struct conv_attrs* ca, const struct checkout* state, int to_tempfile, int* nr_checkouts);
extern void diff_flush_stat(struct diff_filepair* p, struct diff_options* o, struct diffstat_t* diffstat);
extern void free_diffstat_info(struct diffstat_t* diffstat);
static_assert(sizeof(unsigned long long) == sizeof(timestamp_t), "Required for each_reflog_ent_fn definition in gitdll.h");
extern int for_each_reflog_ent(const char* refname, each_reflog_ent_fn fn, void* cb_data);
extern void reset_setup(void);

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
	int ret = repo_get_oid(the_repository, name, &oid);
	hashcpy(sha1, oid.hash, the_repository->hash_algo);
	return ret;
}

int git_init(const LPWSTR* env)
{
	_fmode = _O_BINARY;
	_setmode(_fileno(stdin), _O_BINARY);
	_setmode(_fileno(stdout), _O_BINARY);
	_setmode(_fileno(stderr), _O_BINARY);

	cleanup_chdir_notify();
	fscache_flush();
	reset_setup();
	reset_git_env(env);
	assert(getenv("HOME")); // make sure HOME is already set
	drop_all_attr_stacks();
	git_config_clear();
	g_prefix = setup_git_directory();
	git_config(git_default_config, NULL);
	ref_store_release_and_clear(the_repository);
	clear_ref_decorations();

	/* add a safeguard until we have full support in TortoiseGit */
	if (the_repository && the_repository->hash_algo && strcmp(the_repository->hash_algo->name, "sha1") != 0)
		die("Only SHA1 is supported right now.");

	return 0;
}

static int git_parse_commit_author(struct GIT_COMMIT_AUTHOR* author, const char* pbuff)
{
	const char* end;

	end=strchr(pbuff,'<');
	if (!end || end - pbuff - 1 >= INT_MAX)
		return -1;
	author->Name = pbuff;
	author->NameSize = (int)(end - pbuff - 1);

	pbuff = end +1;
	end = strchr(pbuff, '>');
	if (!end || end[1] != ' ' || end - pbuff >= INT_MAX)
		return -1;
	author->Email = pbuff ;
	author->EmailSize = (int)(end - pbuff);

	pbuff = end + 2;

	author->Date = parse_timestamp(pbuff, &end, 10);
	if (end == pbuff || !end || *end != ' ' || end[1] != '+' && end[1] != '-' || !isdigit(end[2]) || !isdigit(end[3]) || !isdigit(end[4]) || !isdigit(end[5]))
		return -1;
	pbuff = end + 1;
	author->TimeZone = strtol(pbuff, NULL, 10);

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
		if (strncmp(pbuf, "author ", strlen("author ")) == 0)
		{
			ret = git_parse_commit_author(&commit->m_Author, pbuf + strlen("author "));
			if(ret)
				return -4;
		}
		else if (strncmp(pbuf, "committer ", strlen("committer ")) == 0)
		{
			ret = git_parse_commit_author(&commit->m_Committer, pbuf + strlen("committer "));
			if(ret)
				return -5;

			pbuf = strchr(pbuf,'\n');
			if(pbuf == NULL)
				return -6;
		}
		else if (strncmp(pbuf, "encoding ", strlen("encoding ")) == 0)
		{
			pbuf += strlen("encoding ");
			commit->m_Encode=pbuf;
			end = strchr(pbuf,'\n');
			if (!pbuf || end - pbuf >= INT_MAX)
				return -7;
			commit->m_EncodeSize= (int)(end -pbuf);
		}

		// the headers end after the first empty line
		else if (*pbuf == '\n')
		{
			++pbuf;

			commit->m_Subject=pbuf;
			end = strchr(pbuf,'\n');
			if (!end)
			{
				size_t subjLen = strlen(pbuf);
				if (subjLen >= INT_MAX)
					return -8;
				commit->m_SubjectSize = (int)subjLen;
			}
			else
			{
				size_t bodyLen;
				if (end - pbuf >= INT_MAX)
					return -9;
				commit->m_SubjectSize = (int)(end - pbuf);
				pbuf = end +1;
				commit->m_Body = pbuf;
				bodyLen = strlen(pbuf);
				if (bodyLen >= INT_MAX)
					return -10;
				commit->m_BodySize = (int)bodyLen;
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

	hashcpy(oid.hash, hash, the_repository->hash_algo);
	oid.algo = 0;

	commit->m_pGitCommit = p = lookup_commit(the_repository, &oid);

	if(p == NULL)
		return -1;

	ret = repo_parse_commit(the_repository, p);
	if( ret )
		return ret;

	return git_parse_commit(commit);
}

int git_get_commit_first_parent(const GIT_COMMIT* commit, GIT_COMMIT_LIST* list)
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

	free_commit_buffer(the_repository->parsed_objects, p);
	free((void*)commit->buffer);

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

	argv = malloc(strlen(arg) + 2 + (count + 3) * sizeof(char*)); // 1 char* for every parameter + 1 for argv[0] + 1 NULL as end; and some space for the actual parameters: strlen() + 1 for \0, + 1 for \0 for argv[0]
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

	ref_store_release_and_clear(the_repository);
	clear_ref_decorations();

	repo_init_revisions(the_repository, p_Rev, g_prefix);
	p_Rev->diff = 1;

	memset(&opt, 0, sizeof(opt));
	opt.def = "HEAD";

	cmd_log_init_tgit(argc, argv, g_prefix, p_Rev, &opt);

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

	return estimate_commit_count(p_Rev->commits);
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

	if (follow)
	{
		struct rev_info* p_Rev = (struct rev_info*)handle;
		p_Rev->diffopt.no_free = 1;
		if (!log_tree_commit(handle, commit->m_pGitCommit))
		{
			commit->m_ignore = 1;
			return 0;
		}
	}
	commit->m_ignore = 0;

	ret=git_parse_commit(commit);
	if(ret)
		return ret;

	return 0;
}

struct notes_tree **display_notes_trees;
int git_close_log(GIT_LOG handle, int releaseRevsisions)
{
	if(handle)
	{
		struct rev_info *p_Rev;
		p_Rev=(struct rev_info *)handle;
		p_Rev->diffopt.no_free = 0;
		if (releaseRevsisions)
			release_revisions(p_Rev);
		diff_free(&p_Rev->diffopt);
		free(p_Rev->pPrivate);
		free(handle);
	}
	free_all_pack(the_repository);

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
	if (!p_Rev)
		return -1;
	memset(p_Rev,0,sizeof(struct rev_info));

	p_Rev->pPrivate = argv;
	*diff = (GIT_DIFF)p_Rev;

	repo_init_revisions(the_repository, p_Rev, g_prefix);
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
	struct object_id oid = { 0 };
	struct rev_info *p_Rev;
	struct diff_queue_struct *q = &diff_queued_diff;

	p_Rev = (struct rev_info *)diff;

	hashcpy(oid.hash, hash, the_repository->hash_algo);

	diff_root_tree_oid(&oid, "", &p_Rev->diffopt);

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
	struct object_id oid1, oid2;
	struct diff_queue_struct *q = &diff_queued_diff;

	p_Rev = (struct rev_info *)diff;

	hashcpy(oid1.hash, hash1, the_repository->hash_algo);
	oid1.algo = 0;
	hashcpy(oid2.hash, hash2, the_repository->hash_algo);
	oid2.algo = 0;

	diff_tree_oid(&oid1, &oid2, "", &p_Rev->diffopt);

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
	free_all_pack(the_repository);
	if(file)
		*file = q;
	if(count)
		*count = q->nr;
	return 0;
}

int git_get_diff_file(GIT_DIFF diff, GIT_FILE file, int i, char** newname, char** oldname, int* IsDir, char* status, int* IsBin, int* inc, int* dec)
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
	add_pattern(string, base, baselen, which, lineno);
	return 0;
}

int git_create_exclude_list(EXCLUDE_LIST *which)
{
	*which = malloc(sizeof(struct pattern_list));
	if (!*which)
		return -1;
	memset(*which, 0, sizeof(struct pattern_list));
	return 0;
}

int git_free_exclude_list(EXCLUDE_LIST which)
{
	struct pattern_list* p = (struct pattern_list*)which;

	for (int i = 0; i < p->nr; ++i)
	{
		free(p->patterns[i]);
	}
	free(p->patterns);
	free(p);
	return 0;
}

int git_check_excluded_1(const char *pathname,
							int pathlen, const char *basename, int *dtype,
							EXCLUDE_LIST el, int ignorecase)
{
	ignore_case = ignorecase;
	return path_matches_pattern_list(pathname, pathlen, basename, dtype, el, NULL);
}

int git_get_notes(const GIT_HASH hash, char** p_note)
{
	struct object_id oid;
	struct strbuf sb;
	size_t size;
	strbuf_init(&sb,0);
	hashcpy(oid.hash, hash, the_repository->hash_algo);
	oid.algo = 0;
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

	ret = cmd_update_index(argc, argv, NULL, the_repository);
	free(argv);

	discard_index(the_repository->index);
	free_all_pack(the_repository);

	return ret;
}

void git_exit_cleanup(void)
{
	git_atexit_dispatch();
	// do not clear the atexit list, as lots of methods register it just once (and have a local static int flag)
}

int git_for_each_reflog_ent(const char *ref, each_reflog_ent_fn fn, void *cb_data)
{
	return refs_for_each_reflog_ent(get_main_ref_store(the_repository) , ref, fn, cb_data);
}

static int update_some(const struct object_id* sha1, struct strbuf* base, const char* pathname, unsigned mode, void* context)
{
	struct cache_entry* ce = (struct cache_entry*)context;

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
	struct checkout state = CHECKOUT_INIT;
	struct pathspec pathspec;
	struct conv_attrs ca;
	const char *matchbuf[1];
	ret = repo_get_oid(the_repository, ref, &oid);
	if(ret)
		return ret;

	reprepare_packed_git(the_repository);
	root = parse_tree_indirect(&oid);

	if(!root)
	{
		free_all_pack(the_repository);
		return -1;
	}

	ce = xcalloc(1, cache_entry_size(strlen(path)));

	matchbuf[0] = NULL;
	parse_pathspec(&pathspec, PATHSPEC_ALL_MAGIC, PATHSPEC_PREFER_CWD, path, matchbuf);
	pathspec.items[0].nowildcard_len = pathspec.items[0].len;
	ret = read_tree(the_repository, root, &pathspec, update_some, ce);
	clear_pathspec(&pathspec);

	if(ret)
	{
		free_all_pack(the_repository);
		free(ce);
		return ret;
	}
	state.force = 1;
	state.refresh_cache = 0;
	state.istate = the_repository->index;

	reset_parsed_attributes();
	convert_attrs(state.istate, &ca, path);

	ret = write_entry(ce, outputpath, &ca, &state, 0, NULL);
	free_all_pack(the_repository);
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

static int get_config(const char* key_, const char* value_, const struct config_context* ctx, void* cb)
{
	UNREFERENCED_PARAMETER(ctx);
	struct config_buf *buf = (struct config_buf*)cb;
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

// wchar_t wrapper for git_etc_gitconfig()
const wchar_t *wget_msysgit_etc(const LPWSTR* env)
{
	static const wchar_t *etc_gitconfig = NULL;
	wchar_t wpointer[MAX_PATH];

	if (etc_gitconfig)
		return etc_gitconfig;

	build_libgit_environment(env);
	char* systemconfig = git_system_config();
	if (xutftowcs_path(wpointer, systemconfig) < 0)
	{
		free(systemconfig);
		return NULL;
	}
	free(systemconfig);

	etc_gitconfig = _wcsdup(wpointer);

	return etc_gitconfig;
}

int git_get_config(const char *key, char *buffer, int size)
{
	char* system;
	char* global = NULL;
	char* globalxdg = NULL;
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
		opts.git_dir = repo_get_git_dir(the_repository);
		char* local = repo_git_path(the_repository, "config");
		config_source.file = local;
		config_with_options(get_config, &buf, &config_source, the_repository, &opts);
		free(local);
		if (buf.seen)
			return !buf.seen;
	}

	git_global_config_paths(&global, &globalxdg);
	if (globalxdg)
	{
		config_source.file = globalxdg;
		config_with_options(get_config, &buf, &config_source, the_repository, &opts);
		free(globalxdg);
		globalxdg = NULL;
		if (buf.seen)
		{
			free(global);
			return !buf.seen;
		}
	}
	if (global)
	{
		config_source.file = global;
		config_with_options(get_config, &buf, &config_source, the_repository, &opts);
		free(global);
		global = NULL;
		if (buf.seen)
			return !buf.seen;
	}

	system = git_system_config();
	if (system)
	{
		config_source.file = system;
		config_with_options(get_config, &buf, &config_source, the_repository, &opts);
		free(system);
		system = NULL;
		if (buf.seen)
			return !buf.seen;
	}

	return !buf.seen;
}

const char* git_default_notes_ref(void)
{
	return default_notes_ref(the_repository);
}

int git_set_config(const char* key, const char* value, CONFIG_TYPE type)
{
	char * config_exclusive_filename = NULL;
	int ret;

	switch(type)
	{
	case CONFIG_LOCAL:
		if (!the_repository || !the_repository->gitdir)
			die("repository not correctly initialized.");
		config_exclusive_filename = repo_git_path(the_repository, "config");
		break;
	case CONFIG_GLOBAL:
	case CONFIG_XDGGLOBAL:
		{
			char* global = NULL;
			char* globalxdg = NULL;
			git_global_config_paths(&global, &globalxdg);
			if (type == CONFIG_GLOBAL)
			{
				config_exclusive_filename = global;
				global = NULL;
			}
			else
			{
				config_exclusive_filename = globalxdg;
				globalxdg = NULL;
			}
			free(global);
			free(globalxdg);
		}
		break;
	}

	if(!config_exclusive_filename)
		return -1;

	ret = git_config_set_multivar_in_file_gently(config_exclusive_filename, key, value, NULL, NULL, 0);
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

	if ((result = read_mailmap(map)) != 0)
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
	struct string_list_item* si;
	struct mailmap_entry* me;

	if (!mailmap)
		return -1;

	map = (struct string_list *)mailmap;
	si = string_list_lookup(map, email2);
	if (!si)
		return -1;

	me = (struct mailmap_entry*)si->util;
	if (me->namemap.nr)
	{
		/* The item has multiple items */
		const char* author2 = author2_cb(payload);
		struct string_list_item* subitem = string_list_lookup(&me->namemap, author2);
		if (subitem)
			me = (struct mailmap_entry*)subitem->util;
	}

	if (email1)
		*email1 = me->email;
	if (name1)
		*name1 = me->name;
	return 0;
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
