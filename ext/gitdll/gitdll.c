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
#include "builtin.h"
#include "exec_cmd.h"
#include "cache.h"
#include "quote.h"
#include "run-command.h"

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
		if(*arg == NULL)
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
	int size;
	char ** argv=0;
	int argc=0;
	int i=0;
	struct setup_revision_opt opt;

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

	free_notes(*display_notes_trees);
	display_notes_trees = 0;
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
	return excluded_from_list(pathname, pathlen, basename,dtype,el);
}

int git_get_notes(GIT_HASH hash, char **p_note)
{
	struct strbuf sb;
	size_t size;
	strbuf_init(&sb,0);
	format_display_notes(hash, &sb, "utf-8", 0);
	*p_note = strbuf_detach(&sb,&size);
		
	return 0;
}

struct cmd_struct {
	const char *cmd;
	int (*fn)(int, const char **, const char *);
	int option;
};

#define RUN_SETUP	(1<<0)
#define USE_PAGER	(1<<1)
/*
 * require working tree to be present -- anything uses this needs
 * RUN_SETUP for reading from the configuration file.
 */
#define NEED_WORK_TREE	(1<<2)

const char git_usage_string[] =
	"git [--version] [--exec-path[=GIT_EXEC_PATH]] [--html-path]\n"
	"           [-p|--paginate|--no-pager] [--no-replace-objects]\n"
	"           [--bare] [--git-dir=GIT_DIR] [--work-tree=GIT_WORK_TREE]\n"
	"           [-c name=value] [--help]\n"
	"           COMMAND [ARGS]";

const char git_more_info_string[] =
	"See 'git help COMMAND' for more information on a specific command.";

/* returns 0 for "no pager", 1 for "use pager", and -1 for "not specified" */
int check_pager_config(const char *cmd)
{
	return 0;
}


int git_run_cmd(char *cmd, char *arg)
{

	int i=0;
	char ** argv=0;
	int argc=0;

static struct cmd_struct commands[] = {
		{ "add", cmd_add, RUN_SETUP | NEED_WORK_TREE },
		{ "stage", cmd_add, RUN_SETUP | NEED_WORK_TREE },
		{ "annotate", cmd_annotate, RUN_SETUP },
		{ "apply", cmd_apply },
		{ "archive", cmd_archive },
		{ "bisect--helper", cmd_bisect__helper, RUN_SETUP | NEED_WORK_TREE },
		{ "blame", cmd_blame, RUN_SETUP },
		{ "branch", cmd_branch, RUN_SETUP },
		{ "bundle", cmd_bundle },
		{ "cat-file", cmd_cat_file, RUN_SETUP },
		{ "checkout", cmd_checkout, RUN_SETUP | NEED_WORK_TREE },
		{ "checkout-index", cmd_checkout_index,
			RUN_SETUP | NEED_WORK_TREE},
		{ "check-ref-format", cmd_check_ref_format },
		{ "check-attr", cmd_check_attr, RUN_SETUP },
		{ "cherry", cmd_cherry, RUN_SETUP },
		{ "cherry-pick", cmd_cherry_pick, RUN_SETUP | NEED_WORK_TREE },
		{ "clone", cmd_clone },
		{ "clean", cmd_clean, RUN_SETUP | NEED_WORK_TREE },
		{ "commit", cmd_commit, RUN_SETUP | NEED_WORK_TREE },
		{ "commit-tree", cmd_commit_tree, RUN_SETUP },
		{ "config", cmd_config },
		{ "count-objects", cmd_count_objects, RUN_SETUP },
		{ "describe", cmd_describe, RUN_SETUP },
		{ "diff", cmd_diff },
		{ "diff-files", cmd_diff_files, RUN_SETUP | NEED_WORK_TREE },
		{ "diff-index", cmd_diff_index, RUN_SETUP },
		{ "diff-tree", cmd_diff_tree, RUN_SETUP },
		{ "fast-export", cmd_fast_export, RUN_SETUP },
		{ "fetch", cmd_fetch, RUN_SETUP },
		{ "fetch-pack", cmd_fetch_pack, RUN_SETUP },
		{ "fmt-merge-msg", cmd_fmt_merge_msg, RUN_SETUP },
		{ "for-each-ref", cmd_for_each_ref, RUN_SETUP },
		{ "format-patch", cmd_format_patch, RUN_SETUP },
		{ "fsck", cmd_fsck, RUN_SETUP },
		{ "fsck-objects", cmd_fsck, RUN_SETUP },
		{ "gc", cmd_gc, RUN_SETUP },
		{ "get-tar-commit-id", cmd_get_tar_commit_id },
		{ "grep", cmd_grep },
		{ "hash-object", cmd_hash_object },
		{ "help", cmd_help },
		{ "index-pack", cmd_index_pack },
		{ "init", cmd_init_db },
		{ "init-db", cmd_init_db },
		{ "log", cmd_log, RUN_SETUP },
		{ "ls-files", cmd_ls_files, RUN_SETUP },
		{ "ls-tree", cmd_ls_tree, RUN_SETUP },
		{ "ls-remote", cmd_ls_remote },
		{ "mailinfo", cmd_mailinfo },
		{ "mailsplit", cmd_mailsplit },
		{ "merge", cmd_merge, RUN_SETUP | NEED_WORK_TREE },
		{ "merge-base", cmd_merge_base, RUN_SETUP },
		{ "merge-file", cmd_merge_file },
		{ "merge-index", cmd_merge_index, RUN_SETUP },
		{ "merge-ours", cmd_merge_ours, RUN_SETUP },
		{ "merge-recursive", cmd_merge_recursive, RUN_SETUP | NEED_WORK_TREE },
		{ "merge-recursive-ours", cmd_merge_recursive, RUN_SETUP | NEED_WORK_TREE },
		{ "merge-recursive-theirs", cmd_merge_recursive, RUN_SETUP | NEED_WORK_TREE },
		{ "merge-subtree", cmd_merge_recursive, RUN_SETUP | NEED_WORK_TREE },
		{ "merge-tree", cmd_merge_tree, RUN_SETUP },
		{ "mktag", cmd_mktag, RUN_SETUP },
		{ "mktree", cmd_mktree, RUN_SETUP },
		{ "mv", cmd_mv, RUN_SETUP | NEED_WORK_TREE },
		{ "name-rev", cmd_name_rev, RUN_SETUP },
		{ "notes", cmd_notes, RUN_SETUP },
		{ "pack-objects", cmd_pack_objects, RUN_SETUP },
		{ "pack-redundant", cmd_pack_redundant, RUN_SETUP },
		{ "patch-id", cmd_patch_id },
		{ "peek-remote", cmd_ls_remote },
		{ "pickaxe", cmd_blame, RUN_SETUP },
		{ "prune", cmd_prune, RUN_SETUP },
		{ "prune-packed", cmd_prune_packed, RUN_SETUP },
		{ "push", cmd_push, RUN_SETUP },
		{ "read-tree", cmd_read_tree, RUN_SETUP },
		{ "receive-pack", cmd_receive_pack },
		{ "reflog", cmd_reflog, RUN_SETUP },
		{ "remote", cmd_remote, RUN_SETUP },
		{ "replace", cmd_replace, RUN_SETUP },
		{ "repo-config", cmd_config },
		{ "rerere", cmd_rerere, RUN_SETUP },
		{ "reset", cmd_reset, RUN_SETUP },
		{ "rev-list", cmd_rev_list, RUN_SETUP },
		{ "rev-parse", cmd_rev_parse },
		{ "revert", cmd_revert, RUN_SETUP | NEED_WORK_TREE },
		{ "rm", cmd_rm, RUN_SETUP },
		{ "send-pack", cmd_send_pack, RUN_SETUP },
		{ "shortlog", cmd_shortlog, USE_PAGER },
		{ "show-branch", cmd_show_branch, RUN_SETUP },
		{ "show", cmd_show, RUN_SETUP },
		{ "status", cmd_status, RUN_SETUP | NEED_WORK_TREE },
		{ "stripspace", cmd_stripspace },
		{ "symbolic-ref", cmd_symbolic_ref, RUN_SETUP },
		{ "tag", cmd_tag, RUN_SETUP },
		{ "tar-tree", cmd_tar_tree },
		{ "unpack-file", cmd_unpack_file, RUN_SETUP },
		{ "unpack-objects", cmd_unpack_objects, RUN_SETUP },
		{ "update-index", cmd_update_index, RUN_SETUP },
		{ "update-ref", cmd_update_ref, RUN_SETUP },
		{ "update-server-info", cmd_update_server_info, RUN_SETUP },
		{ "upload-archive", cmd_upload_archive },
		{ "var", cmd_var },
		{ "verify-tag", cmd_verify_tag, RUN_SETUP },
		{ "version", cmd_version },
		{ "whatchanged", cmd_whatchanged, RUN_SETUP },
		{ "write-tree", cmd_write_tree, RUN_SETUP },
		{ "verify-pack", cmd_verify_pack },
		{ "show-ref", cmd_show_ref, RUN_SETUP },
		{ "pack-refs", cmd_pack_refs, RUN_SETUP },
	};
	
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

			return ret;
			

		}
	}
	return -1;
}

int git_for_each_ref_in(const char * refname, each_ref_fn fn, void * data)
{
	invalidate_cached_refs();
	return for_each_ref_in(refname, fn, data);
}

const char *git_resolve_ref(const char *ref, unsigned char *sha1, int reading, int *flag)
{
	invalidate_cached_refs();
	return resolve_ref(ref,sha1,reading, flag);
}
int git_for_each_reflog_ent(const char *ref, each_reflog_ent_fn fn, void *cb_data)
{
	return for_each_reflog_ent(ref,fn,cb_data);
}

int git_deref_tag(unsigned char *tagsha1, GIT_HASH refhash)
{
	struct object *obj = NULL;
	obj = parse_object(tagsha1);
	if (!obj)
		return -1;

	if (obj->type == OBJ_TAG) 
	{
			obj = deref_tag(obj, "", 0);
			if (!obj)
				return -1;

			memcpy(refhash, obj->sha1, sizeof(GIT_HASH));
			return 0;
	}

	memcpy(refhash, tagsha1, sizeof(GIT_HASH));
	return 0;
}