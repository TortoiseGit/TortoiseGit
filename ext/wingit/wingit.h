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

#define WG_VERSION "0.1.6"


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
	WGEFF_DirStatusDelta= (1<<2),	// include directories, in enumeration, that have a recursive status != WGFS_Normal (may have a slightly better performance than WGEFF_DirStatusAll)
	WGEFF_DirStatusAll	= (1<<3),	// include directories, in enumeration, with recursive status
	WGEFF_EmptyAsNormal	= (1<<4),	// report sub-directories, with no versioned files, as WGFS_Normal instead of WGFS_Empty
	WGEFF_SingleFile	= (1<<5)	// indicates that the status of a single file or dir, specified by pszSubPath, is wanted
};

// NOTE: Special behavior for directories when specifying WGEFF_SingleFile:
//
//       * when combined with WGEFF_SingleFile the returned status will only reflect the immediate files in the dir,
//         NOT the recusrive status of immediate sub-dirs
//       * unlike a normal enumeration where the project root dir always is returned as WGFS_Normal regardless
//         of WGEFF_EmptyAsNormal, the project root will return WGFS_Empty if no immediate versioned files
//         unless WGEFF_EmptyAsNormal is specified
//       * WGEFF_DirStatusDelta and WGEFF_DirStatusAll are ignored and can be omitted even for dirs


// File status
enum WGFILESTATUS
{
	WGFS_Normal,
	WGFS_Modified,
	WGFS_Deleted,

	WGFS_Unknown = -1,
	WGFS_Empty = -2
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

	const BYTE* sha1;				// points to the BYTE[20] sha1 (NULL for directories, WGFF_Directory)
};


// Application-defined callback function for wgEnumFiles, returns TRUE to abort enumeration
// NOTE: do NOT store the pFile pointer or any pointers in wgFile_s for later use, the data is only valid for a single callback call
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

// Get the SHA1 of pszName (NULL is same as "HEAD", "HEAD^" etc. expression allowed), returns NULL on failure
// NOTE: do not store returned pointer for later used, data must be used/copied right away before further wg calls
LPBYTE WINGIT_API wgGetRevisionID(const char *pszProjectPath, const char *pszName);


#ifdef __cplusplus
}
#endif
#endif
