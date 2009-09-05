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

GITDLL_API int ngitdll;

GITDLL_API int fngitdll(void);

GITDLL_API char * get_git_last_error();
GITDLL_API int git_get_sha1(const char *name, unsigned char *sha1);