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

// The following ifdef block is the standard way of creating macros which make exporting
// from a DLL simpler. All files within this DLL are compiled with the GITDLL_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see
// GITDLL_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.

#ifndef __GITDLL__
#define __GITDLL__

#ifdef __cplusplus
#define EXTERN extern "C"
#else
#define EXTERN
#endif

#ifdef GITDLL_EXPORTS
#define GITDLL_API EXTERN __declspec(dllexport)
#else
#define GITDLL_API EXTERN __declspec(dllimport)
#endif

#define LIBGIT_GIT_HASH_SIZE 32

typedef unsigned char GIT_HASH[LIBGIT_GIT_HASH_SIZE];

struct GIT_OBJECT_OID {
	unsigned char hash[LIBGIT_GIT_HASH_SIZE];
};

typedef void *  GIT_HANDLE;
typedef void *  GIT_LOG;

typedef void * GIT_DIFF;
typedef void * GIT_FILE;
typedef void * GIT_COMMIT_LIST;

struct GIT_COMMIT_AUTHOR
{
	const char* Name;
	int	  NameSize;
	const char* Email;
	int	  EmailSize;
	int	  Date;
	int   TimeZone;

};
typedef struct GIT_COMMIT_DATA
{
	GIT_HASH m_hash;
	struct GIT_COMMIT_AUTHOR m_Author;
	struct GIT_COMMIT_AUTHOR m_Committer;
	const char* m_Subject;
	int		 m_SubjectSize;
	const char* m_Body;
	int		 m_BodySize;
	void *   m_pGitCommit; /** internal used */
	const char* m_Encode;
	int		 m_EncodeSize;
	int		 m_ignore;
	const void* buffer;
} GIT_COMMIT;

/**
 *	Get hash value.
 *	@param	name	[IN] Reference name, such as HEAD, master, ...
 *	@param	sha1	[OUT] char[20] hash value. Caller prepare char[20] buffer.
 *	@return			0	success.
 */
GITDLL_API int git_get_sha1(const char *name, GIT_HASH sha1);
/**
 *	Init git dll
 *  @remark, this function must be call before other function.
 *	@return			0	success
 */
GITDLL_API int git_init(void);

GITDLL_API int git_open_log(GIT_LOG* handle, const char* arg);
GITDLL_API int git_get_log_firstcommit(GIT_LOG handle);
GITDLL_API int git_get_log_estimate_commit_count(GIT_LOG handle);

/**
 *	Get Next Commit
 *  @param handle	[IN]handle  Get handle from git_open_log
 *	@param commit	[OUT]commit	Caller need prepare buffer for this call
 *	@param follow	[IN]follow	Follow history beyond renames (see --follow)
 *  @return			0	success
 *	@remark			Caller need call git_free_commit to free internal buffer after use it;
 */
GITDLL_API int git_get_log_nextcommit(GIT_LOG handle, GIT_COMMIT *commit, int follow);

GITDLL_API int git_close_log(GIT_LOG handle);

/**
 *	Get Commit information from commit hash
 *	@param  commit	[OUT] output commit information
 *  @param	hash	[in] hash
 *	@return		0	success
 */
GITDLL_API int git_get_commit_from_hash(GIT_COMMIT* commit, const GIT_HASH hash);
GITDLL_API int git_parse_commit(GIT_COMMIT *commit);
GITDLL_API int git_commit_is_root(const GIT_COMMIT* commit);
GITDLL_API int git_get_commit_first_parent(GIT_COMMIT *commit,GIT_COMMIT_LIST *list);
GITDLL_API int git_get_commit_next_parent(GIT_COMMIT_LIST *list, GIT_HASH hash);

GITDLL_API int git_free_commit(GIT_COMMIT *commit);

GITDLL_API int git_open_diff(GIT_DIFF* diff, const char* arg);
GITDLL_API int git_do_diff(GIT_DIFF diff, const GIT_HASH hash1, const GIT_HASH hash2, GIT_FILE* file, int* count, int isstat);
GITDLL_API int git_root_diff(GIT_DIFF diff, const GIT_HASH hash, GIT_FILE* file, int* count, int isstat);
GITDLL_API int git_diff_flush(GIT_DIFF diff);
GITDLL_API int git_close_diff(GIT_DIFF diff);


GITDLL_API int git_get_diff_file(GIT_DIFF diff, GIT_FILE file, int i, char** newname, char** oldname, int* IsDir, int* status, int* IsBin, int* inc, int* dec);


typedef void * EXCLUDE_LIST;

GITDLL_API int git_create_exclude_list(EXCLUDE_LIST *which);

GITDLL_API int git_add_exclude(const char *string, const char *base,
					int baselen, EXCLUDE_LIST which, int lineno);

GITDLL_API int git_check_excluded_1(const char *pathname,
							int pathlen, const char *basename, int *dtype,
							EXCLUDE_LIST el, int ignorecase);

#define DT_UNKNOWN	0
#define DT_DIR		1
#define DT_REG		2
#define DT_LNK		3

GITDLL_API int git_free_exclude_list(EXCLUDE_LIST which);

//caller need free p_note
GITDLL_API int git_get_notes(const GIT_HASH hash, char** p_note);

GITDLL_API int git_update_index(void);

GITDLL_API void git_exit_cleanup(void);

#define REF_ISSYMREF 01
#define REF_ISPACKED 02

typedef int each_ref_fn(const char* refname, const struct GIT_OBJECT_OID* oid, int flags, void* cb_data);

typedef int each_reflog_ent_fn(struct GIT_OBJECT_OID* old_oid, struct GIT_OBJECT_OID* new_oid, const char* committer, unsigned long long timestamp, int tz, const char* msg, void* cb_data);
GITDLL_API int git_for_each_reflog_ent(const char *ref, each_reflog_ent_fn fn, void *cb_data);

GITDLL_API int git_checkout_file(const char* ref, const char* path, char* outputpath);

GITDLL_API int git_get_config(const char *key, char *buffer, int size);

typedef enum
{
	CONFIG_LOCAL,
	CONFIG_GLOBAL,
	CONFIG_XDGGLOBAL,
	CONFIG_SYSTEM,

}CONFIG_TYPE;

GITDLL_API int get_set_config(const char *key, const char *value, CONFIG_TYPE type);

const char *get_windows_home_directory(void);

GITDLL_API const wchar_t *wget_windows_home_directory(void);
GITDLL_API const wchar_t *wget_msysgit_etc(void);
GITDLL_API const wchar_t *wget_program_data_config(void);

typedef void *GIT_MAILMAP;

GITDLL_API int git_read_mailmap(GIT_MAILMAP *mailmap);
GITDLL_API int git_lookup_mailmap(GIT_MAILMAP mailmap, const char** email1, const char** name1, const char* email2, void* payload, const char* (*author2_cb)(void*));
GITDLL_API void git_free_mailmap(GIT_MAILMAP mailmap);

GITDLL_API int git_mkdir(const char* path);

#endif
