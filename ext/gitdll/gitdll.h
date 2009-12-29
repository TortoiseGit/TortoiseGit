// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the GITDLL_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// GITDLL_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#ifdef __cplusplus
#define EXTERN extern "C"
#else
#define EXTERN
#endif

#ifdef GITDLL_EXPORTS
#define GITDLL_API __declspec(dllexport) EXTERN
#else
#define GITDLL_API __declspec(dllimport) EXTERN
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
typedef unsigned int  GIT_HANDLE;
typedef unsigned int  GIT_LOG;

typedef unsigned int  GIT_DIFF;
typedef unsigned int  GIT_FILE;

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
GITDLL_API int git_free_commit(GIT_COMMIT *commit);

GITDLL_API int git_get_diff(GIT_COMMIT *commit, GIT_DIFF *diff);

GITDLL_API int git_get_diff_firstfile(GIT_DIFF diff, GIT_FILE * file);
GITDLL_API int git_get_diff_nextfile(GIT_DIFF diff, GIT_FILE *file);
GITDLL_API int git_get_diff_status(GIT_DIFF diff, int * status);
GITDLL_API int git_get_diff_stat(GIT_FILE file, int *inc, int *dec, int *mode);
GITDLL_API int git_get_diff_file(GIT_FILE file, char *newname, int newsize,  char *oldname, int oldsize, int *mode);