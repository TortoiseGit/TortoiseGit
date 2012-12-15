/*
 * $Revision: 2626 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-17 12:10:52 +0200 (Di, 17. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Implementation of basic functionality (incl. file and
 * directory handling)
 *
 * \author Carsten Gutwenger
 *
 * \par License:
 * This file is part of the Open Graph Drawing Framework (OGDF).
 *
 * \par
 * Copyright (C)<br>
 * See README.txt in the root directory of the OGDF installation for details.
 *
 * \par
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * Version 2 or 3 as published by the Free Software Foundation;
 * see the file LICENSE.txt included in the packaging of this file
 * for details.
 *
 * \par
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * \par
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the Free
 * Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 * \see  http://www.gnu.org/copyleft/gpl.html
 ***************************************************************/


#include <ogdf/basic/basic.h>
#include <ogdf/basic/List.h>
#include <ogdf/basic/String.h>
#include <time.h>

// Windows includes
#ifdef OGDF_SYSTEM_WINDOWS
#include <direct.h>
#if defined(_MSC_VER) && defined(UNICODE)
#undef GetFileAttributes
#undef FindFirstFile
#undef FindNextFile
#define GetFileAttributes  GetFileAttributesA
#define FindFirstFile  FindFirstFileA
#define WIN32_FIND_DATA WIN32_FIND_DATAA
#define FindNextFile  FindNextFileA
#endif
#endif

#ifdef __BORLANDC__
#define _chdir chdir
#endif

// Unix includes
#ifdef OGDF_SYSTEM_UNIX
#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/times.h>
#include <sys/stat.h>
#include <fnmatch.h>

double OGDF_clk_tck = sysconf(_SC_CLK_TCK); //is long. but definig it here avoids casts...
#endif


#ifdef OGDF_DLL

#ifdef OGDF_SYSTEM_WINDOWS

#ifdef __MINGW32__
extern "C"
#endif
BOOL APIENTRY DllMain(HANDLE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		ogdf::PoolMemoryAllocator::init();
		ogdf::System::init();
		break;

	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
		break;

	case DLL_PROCESS_DETACH:
		ogdf::PoolMemoryAllocator::cleanup();
		break;
	}
	return TRUE;
}

#else

void __attribute__ ((constructor)) my_load(void)
{
	ogdf::PoolMemoryAllocator::init();
	ogdf::System::init();
}

void __attribute__ ((destructor)) my_unload(void)
{
	ogdf::PoolMemoryAllocator::cleanup();
}

#endif

#else

namespace ogdf {

//static int variables are automatically initialized with 0
int Initialization::s_count;

Initialization::Initialization()
{
	if (s_count++ == 0) {
		ogdf::PoolMemoryAllocator::init();
		ogdf::System::init();
	}
}

Initialization::~Initialization()
{
	if (--s_count == 0) {
		ogdf::PoolMemoryAllocator::cleanup();
	}
}

} // namespace ogdf

#endif


namespace ogdf {

	// debug level (in debug build only)
#ifdef OGDF_DEBUG
	DebugLevel debugLevel;
#endif


double usedTime(double& T)
{
	double t = T;
#ifdef OGDF_SYSTEM_WINDOWS
	T = double(clock())/CLOCKS_PER_SEC;
#endif
#ifdef OGDF_SYSTEM_UNIX
	struct tms now;
	times (&now);
	T = now.tms_utime/OGDF_clk_tck;
#endif
	return  T-t;
}


#ifdef OGDF_SYSTEM_WINDOWS

bool isFile(const char *fileName)
{
	DWORD att = GetFileAttributes(fileName);

	if (att == 0xffffffff) return false;
	return (att & FILE_ATTRIBUTE_DIRECTORY) == 0;
}


bool isDirectory(const char *fileName)
{
	DWORD att = GetFileAttributes(fileName);

	if (att == 0xffffffff) return false;
	return (att & FILE_ATTRIBUTE_DIRECTORY) != 0;
}


bool changeDir(const char *dirName)
{
	return (_chdir(dirName) == 0);
}


void getEntriesAppend(const char *dirName,
		FileType t,
		List<String> &entries,
		const char *pattern)
{
	OGDF_ASSERT(isDirectory(dirName));

	String filePattern;
	filePattern.sprintf("%s\\%s", dirName, pattern);

	WIN32_FIND_DATA findData;
	HANDLE handle = FindFirstFile(filePattern.cstr(), &findData);

	if (handle != INVALID_HANDLE_VALUE)
	{
		do {
			DWORD isDir = (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
			if(isDir && (
				strcmp(findData.cFileName,".") == 0 ||
				strcmp(findData.cFileName,"..") == 0)
			)
				continue;

			if (t == ftEntry || (t == ftFile && !isDir) ||
				(t == ftDirectory && isDir))
			{
				entries.pushBack(findData.cFileName);
			}
		} while(FindNextFile(handle, &findData));

		FindClose(handle);
	}
}
#endif

#ifdef OGDF_SYSTEM_UNIX

bool isDirectory(const char *fname)
{
	struct stat stat_buf;

	if (stat(fname,&stat_buf) != 0)
		return false;
	return (stat_buf.st_mode & S_IFMT) == S_IFDIR;
}

bool isFile(const char *fname)
{
	struct stat stat_buf;

	if (stat(fname,&stat_buf) != 0)
		return false;
	return (stat_buf.st_mode & S_IFMT) == S_IFREG;
}

bool changeDir(const char *dirName)
{
	return (chdir(dirName) == 0);
}

void getEntriesAppend(const char *dirName,
	FileType t,
	List<String> &entries,
	const char *pattern)
{
	OGDF_ASSERT(isDirectory(dirName));

 	DIR* dir_p = opendir(dirName);

	dirent* dir_e;
	while ( (dir_e = readdir(dir_p)) != NULL )
	{
		const char *fname = dir_e->d_name;
		if (pattern != 0 && fnmatch(pattern,fname,0)) continue;

		String fullName;
		fullName.sprintf("%s/%s", dirName, fname);

		bool isDir = isDirectory(fullName.cstr());
		if(isDir && (
			strcmp(fname,".") == 0 ||
			strcmp(fname,"..") == 0)
			)
			continue;

		if (t == ftEntry || (t == ftFile && !isDir) ||
			(t == ftDirectory && isDir))
		{
			entries.pushBack(fname);
		}
	}

 	closedir(dir_p);
}
#endif


void getEntries(const char *dirName,
		FileType t,
		List<String> &entries,
		const char *pattern)
{
	entries.clear();
	getEntriesAppend(dirName, t, entries, pattern);
}


void getFiles(const char *dirName,
	List<String> &files,
	const char *pattern)
{
	getEntries(dirName, ftFile, files, pattern);
}


void getSubdirs(const char *dirName,
	List<String> &subdirs,
	const char *pattern)
{
	getEntries(dirName, ftDirectory, subdirs, pattern);
}


void getEntries(const char *dirName,
	List<String> &entries,
	const char *pattern)
{
	getEntries(dirName, ftEntry, entries, pattern);
}


void getFilesAppend(const char *dirName,
	List<String> &files,
	const char *pattern)
{
	getEntriesAppend(dirName, ftFile, files, pattern);
}


void getSubdirsAppend(const char *dirName,
	List<String> &subdirs,
	const char *pattern)
{
	getEntriesAppend(dirName, ftDirectory, subdirs, pattern);
}


void getEntriesAppend(const char *dirName,
	List<String> &entries,
	const char *pattern)
{
	getEntriesAppend(dirName, ftEntry, entries, pattern);
}





} // end namespace ogdf
