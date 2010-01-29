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

#if 0
// This class is exported from the gitdll.dll
class GITDLL_API Cgitdll {
public:
	Cgitdll(void);
	// TODO: add your methods here.
};
#endif

#define GIT_HASH_SIZE 20

typedef unsigned char GIT_HASH[GIT_HASH_SIZE];

typedef void *  GIT_HANDLE;
typedef void *  GIT_LOG;

typedef void * GIT_DIFF;
typedef void * GIT_FILE;
typedef void * GIT_COMMIT_LIST;

struct GIT_COMMIT_AUTHOR
{
	char *Name;
	int	  NameSize;
	char *Email;
	int	  EmailSize;
	int	  Date;
	int   TimeZone;
	
};
typedef struct GIT_COMMIT_DATA
{
	GIT_HASH m_hash;
	struct GIT_COMMIT_AUTHOR m_Author;
	struct GIT_COMMIT_AUTHOR m_Committer;
	char *	 m_Subject;
	int		 m_SubjectSize;
	char *	 m_Body;
	int		 m_BodySize;
	void *   m_pGitCommit; /** internal used */
	char *   m_Encode;
	int		 m_EncodeSize;

} GIT_COMMIT;


GITDLL_API int ngitdll;

GITDLL_API int fngitdll(void);
/**
 *	Get Git Last Error string. 
 */
GITDLL_API char * get_git_last_error();
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
GITDLL_API int git_init();

GITDLL_API int git_open_log(GIT_LOG * handle, char * arg);
GITDLL_API int git_get_log_firstcommit(GIT_LOG handle);
GITDLL_API int git_get_log_estimate_commit_count(GIT_LOG handle);

/**
 *	Get Next Commit
 *  @param handle	[IN]handle  Get handle from git_open_log
 *	@param commit	[OUT]commit	Caller need prepare buffer for this call
 *  @return			0	success
 *	@remark			Caller need call git_free_commit to free internal buffer after use it;
 */
GITDLL_API int git_get_log_nextcommit(GIT_LOG handle, GIT_COMMIT *commit);

GITDLL_API int git_close_log(GIT_LOG handle);

/**
 *	Get Commit information from commit hash
 *	@param  commit	[OUT] output commit information
 *  @param	hash	[in] hash 
 *	@return		0	success
 */
GITDLL_API int git_get_commit_from_hash(GIT_COMMIT *commit, GIT_HASH hash);
GITDLL_API int git_parse_commit(GIT_COMMIT *commit);

GITDLL_API int git_get_commit_first_parent(GIT_COMMIT *commit,GIT_COMMIT_LIST *list);
GITDLL_API int git_get_commit_next_parent(GIT_COMMIT_LIST *list, GIT_HASH hash);

GITDLL_API int git_free_commit(GIT_COMMIT *commit);

GITDLL_API int git_open_diff(GIT_DIFF *diff, char * arg);
GITDLL_API int git_diff(GIT_DIFF diff, GIT_HASH hash1,GIT_HASH hash2, GIT_FILE * file, int *count);
GITDLL_API int git_root_diff(GIT_DIFF diff, GIT_HASH hash,GIT_FILE *file, int *count);
GITDLL_API int git_diff_flush(GIT_DIFF diff);
GITDLL_API int git_close_diff(GIT_DIFF diff);


GITDLL_API int git_get_diff_file(GIT_DIFF diff,GIT_FILE file, int i,char **newname, char **oldname,  int *mode, int *IsBin, int *inc, int *dec);

#define READ_TREE_RECURSIVE 1
typedef int (*read_tree_fn_t)(const unsigned char *, const char *, int, const char *, unsigned int, int, void *);

GITDLL_API int git_read_tree(GIT_HASH hash,read_tree_fn_t fn, void *context);

#endif