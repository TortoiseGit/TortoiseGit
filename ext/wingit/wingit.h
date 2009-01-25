// wingit.h
//
// Windows GIT wrapper (under msysgit)
//
// This is NOT a full git API, it only exposes a few helper functions that
// third party windows apps might find useful, to avoid having to parse
// console output from the command-line tools.
//
// Make sure wingit.dll is placed in the bin directory of the msysgit installation.
//
// Created by Georg Fischer

#ifndef _WINGIT_H_
#define _WINGIT_H_

#define WG_VERSION "0.1.3"


#define DLLIMPORT __declspec(dllimport) __stdcall
#define DLLEXPORT __declspec(dllexport) __stdcall

#ifdef WINGIT_EXPORTS
#define WINGIT_API DLLEXPORT
#else // GAME_EXPORTS
#define WINGIT_API DLLIMPORT
#endif // GAME_EXPORTS

#ifdef __cplusplus
extern "C" {
#endif


// Flags for wgEnumFiles
enum WGENUMFILEFLAGS
{
	WGEFF_NoRecurse		= (1<<0),	// only enumerate files directly in the specified path
	WGEFF_FullPath		= (1<<1),	// enumerated filenames are specified with full path (instead of relative to proj root)
	WGEFF_DirStatusDelta= (1<<2),	// include directories, in enumeration, that have a recursive status != WGFS_Normal
	WGEFF_DirStatusAll	= (1<<3)	// include directories, in enumeration, with recursive status
};


// File status
enum WGFILESTATUS
{
	WGFS_Normal,
	WGFS_Modified,
	WGFS_Deleted,

	WGFS_Unknown = -1
};


// File flags
enum WGFILEFLAGS
{
	WGFF_Directory		= (1<<0)	// enumerated file is a directory
};


struct wgFile_s
{
	const char *sFileName;			// filename or directory relative to project root (using forward slashes)
	int nStatus;					// the WGFILESTATUS of the file
	int nStage;						// the stage number of the file (0 if unstaged)
	int nFlags;						// a combination of WGFILEFLAGS
};


// Application-defined callback function for wgEnumFiles, returns TRUE to abort enumeration
typedef BOOL (__cdecl WGENUMFILECB)(const struct wgFile_s *pFile, void *pUserData);


// Init git framework
BOOL WINGIT_API wgInit(void);

// Get the lib version
LPCSTR WINGIT_API wgGetVersion(void);

// Get the git version that is used in this lib
LPCSTR WINGIT_API wgGetGitVersion(void);

// Enumerate files in a git project
// Ex: wgEnumFiles("C:\\Projects\\MyProject", "src/core", WGEFF_NoRecurse, MyEnumFunc, NULL)
BOOL WINGIT_API wgEnumFiles(const char *pszProjectPath, const char *pszSubPath, unsigned int nFlags, WGENUMFILECB *pEnumCb, void *pUserData);


#ifdef __cplusplus
}
#endif
#endif
