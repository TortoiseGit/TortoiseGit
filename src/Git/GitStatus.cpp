// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2017 - TortoiseGit

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

#include "stdafx.h"
#include "registry.h"
#include "..\TortoiseShell\resource.h"
#include "GitStatus.h"
#include "UnicodeUtils.h"
#include "Git.h"
#include "gitindex.h"
#include "ShellCache.h"
#include "SysInfo.h"
#include "SmartHandle.h"

extern CGitAdminDirMap g_AdminDirMap;
CGitIndexFileMap g_IndexFileMap;
CGitHeadFileMap g_HeadFileMap;
CGitIgnoreList  g_IgnoreList;

GitStatus::GitStatus()
	: status(nullptr)
{
	m_status.assumeValid = m_status.skipWorktree = false;
	m_status.prop_status = m_status.text_status = git_wc_status_none;
}

// static method
#ifndef TGITCACHE
git_wc_status_kind GitStatus::GetAllStatus(const CTGitPath& path, git_depth_t depth, bool * assumeValid, bool * skipWorktree)
{
	git_wc_status_kind			statuskind;
	BOOL						err;
	BOOL						isDir;
	CString						sProjectRoot;

	isDir = path.IsDirectory();
	if (!path.HasAdminDir(&sProjectRoot))
		return git_wc_status_none;

//	rev.kind = git_opt_revision_unspecified;
	statuskind = git_wc_status_none;

	const BOOL bIsRecursive = (depth == git_depth_infinity || depth == git_depth_unknown); // taken from SVN source

	CString sSubPath;
	CString s = path.GetWinPathString();
	if (s.GetLength() > sProjectRoot.GetLength())
	{
		if (sProjectRoot.GetLength() == 3 && sProjectRoot[1] == L':')
			sSubPath = s.Right(s.GetLength() - sProjectRoot.GetLength());
		else
			sSubPath = s.Right(s.GetLength() - sProjectRoot.GetLength() - 1/*otherwise it gets initial slash*/);
	}

	bool isfull = ((DWORD)CRegStdDWORD(L"Software\\TortoiseGit\\CacheType",
				GetSystemMetrics(SM_REMOTESESSION) ? ShellCache::dll : ShellCache::exe) == ShellCache::dllFull);

	if(isDir)
	{
		err = GetDirStatus(sProjectRoot, sSubPath, &statuskind, isfull, bIsRecursive, isfull);
		// folders must not be displayed as added or deleted only as modified (this is for Shell Overlay-Modes)
		if (statuskind == git_wc_status_unversioned && sSubPath.IsEmpty())
			statuskind = git_wc_status_normal;
		else if (statuskind == git_wc_status_deleted || statuskind == git_wc_status_added)
			statuskind = git_wc_status_modified;
	}
	else
		err = GetFileStatus(sProjectRoot, sSubPath, &statuskind, isfull, false, isfull, nullptr, nullptr, assumeValid, skipWorktree);

	return statuskind;
}
#endif

// static method
git_wc_status_kind GitStatus::GetMoreImportant(git_wc_status_kind status1, git_wc_status_kind status2)
{
	if (GetStatusRanking(status1) >= GetStatusRanking(status2))
		return status1;
	return status2;
}
// static private method
int GitStatus::GetStatusRanking(git_wc_status_kind status)
{
	switch (status)
	{
		case git_wc_status_none:
			return 0;
		case git_wc_status_unversioned:
			return 1;
		case git_wc_status_ignored:
			return 2;
		case git_wc_status_incomplete:
			return 4;
		case git_wc_status_normal:
		case git_wc_status_external:
			return 5;
		case git_wc_status_added:
			return 6;
		case git_wc_status_missing:
			return 7;
		case git_wc_status_deleted:
			return 8;
		case git_wc_status_replaced:
			return 9;
		case git_wc_status_modified:
			return 10;
		case git_wc_status_merged:
			return 11;
		case git_wc_status_conflicted:
			return 12;
		case git_wc_status_obstructed:
			return 13;
	}
	return 0;
}

#ifndef TGITCACHE
void GitStatus::GetStatus(const CTGitPath& path, bool /*update*/ /* = false */, bool noignore /* = false */, bool /*noexternals*/ /* = false */)
{
	// NOTE: unlike the SVN version this one does not cache the enumerated files, because in practice no code in all of
	//       Tortoise uses this, all places that call GetStatus create a temp GitStatus object which gets destroyed right
	//       after the call again

	CString sProjectRoot;
	if ( !path.HasAdminDir(&sProjectRoot) )
		return;

	bool isfull = ((DWORD)CRegStdDWORD(L"Software\\TortoiseGit\\CacheType",
				GetSystemMetrics(SM_REMOTESESSION) ? ShellCache::dll : ShellCache::exe) == ShellCache::dllFull);

	int err = 0;

	LPCTSTR lpszSubPath = nullptr;
	CString sSubPath;
	CString s = path.GetWinPathString();
	if (s.GetLength() > sProjectRoot.GetLength())
	{
		sSubPath = s.Right(s.GetLength() - sProjectRoot.GetLength());
		lpszSubPath = sSubPath;
		// skip initial slash if necessary
		if (*lpszSubPath == L'\\')
			++lpszSubPath;
	}

	m_status.prop_status = m_status.text_status = git_wc_status_none;
	m_status.assumeValid = false;
	m_status.skipWorktree = false;

	if (path.IsDirectory())
	{
		err = GetDirStatus(sProjectRoot, lpszSubPath, &m_status.text_status, isfull, false, !noignore);
		if (m_status.text_status == git_wc_status_added || m_status.text_status == git_wc_status_deleted) // fix for issue #1769; a folder is either modified, conflicted or normal
			m_status.text_status = git_wc_status_modified;
	}
	else
		err = GetFileStatus(sProjectRoot, lpszSubPath, &m_status.text_status, isfull, false, !noignore, nullptr, nullptr, &m_status.assumeValid, &m_status.skipWorktree);

	// Error present if function is not under version control
	if (err)
	{
		status = nullptr;
		return;
	}

	status = &m_status;
}
#endif

typedef CComCritSecLock<CComCriticalSection> CAutoLocker;

int GitStatus::GetFileStatus(const CString& gitdir, CString path, git_wc_status_kind* status, BOOL IsFull, BOOL IsRecursive, BOOL IsIgnore, FILL_STATUS_CALLBACK callback, void* pData, bool* assumeValid, bool* skipWorktree)
{
	if (!status)
		return 0;

	path.Replace(L'\\', L'/');

	git_wc_status_kind st = git_wc_status_none;
	CGitHash hash;

	g_IndexFileMap.GetFileStatus(gitdir, path, &st, IsFull, false, IsRecursive ? nullptr : callback, pData, &hash, assumeValid, skipWorktree);

	if (st == git_wc_status_conflicted)
	{
		*status = st;
		if (callback && assumeValid && skipWorktree)
			callback(CombinePath(gitdir, path), st, false, pData, *assumeValid, *skipWorktree);
		return 0;
	}

	if (st == git_wc_status_unversioned)
	{
		if (!IsIgnore)
		{
			*status = git_wc_status_unversioned;
			if (callback && assumeValid && skipWorktree)
				callback(CombinePath(gitdir, path), *status, false, pData, *assumeValid, *skipWorktree);
			return 0;
		}

		g_IgnoreList.CheckAndUpdateIgnoreFiles(gitdir, path, false);
		if (g_IgnoreList.IsIgnore(path, gitdir, false))
			st = git_wc_status_ignored;

		*status = st;
		if (callback && assumeValid && skipWorktree)
			callback(CombinePath(gitdir, path), st, false, pData, *assumeValid, *skipWorktree);

		return 0;
	}

	if ((st == git_wc_status_normal || st == git_wc_status_modified) && IsFull)
	{
		g_HeadFileMap.CheckHeadAndUpdate(gitdir);

		// Check Head Tree Hash
		SHARED_TREE_PTR treeptr = g_HeadFileMap.SafeGet(gitdir);

		//add item
		size_t start = SearchInSortVector(*treeptr, path, -1);
		if (start == NPOS)
		{
			*status = st = git_wc_status_added;
			CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": File miss in head tree %s", (LPCTSTR)path);
			if (callback && assumeValid && skipWorktree)
				callback(CombinePath(gitdir, path), st, false, pData, *assumeValid, *skipWorktree);
			return 0;
		}

		// staged and not commit
		if ((*treeptr)[start].m_Hash != hash)
		{
			*status = st = git_wc_status_modified;
			if (callback && assumeValid && skipWorktree)
				callback(CombinePath(gitdir, path), st, false, pData, *assumeValid, *skipWorktree);
			return 0;
		}
	}
	*status = st;
	if (callback && assumeValid && skipWorktree)
		callback(CombinePath(gitdir, path), st, false, pData, *assumeValid, *skipWorktree);
	return 0;
}

#ifdef TGITCACHE
bool GitStatus::CheckAndUpdateIgnoreFiles(const CString& gitdir, const CString& subpaths, bool isDir)
{
	return g_IgnoreList.CheckAndUpdateIgnoreFiles(gitdir, subpaths, isDir);
}
int GitStatus::IsUnderVersionControl(const CString &gitdir, const CString &path, bool isDir,bool *isVersion)
{
	if (g_IndexFileMap.IsUnderVersionControl(gitdir, path, isDir, isVersion))
		return 1;
	if (!*isVersion)
		return g_HeadFileMap.IsUnderVersionControl(gitdir, path, isDir, isVersion);
	return 0;
}

bool GitStatus::IsIgnored(const CString& gitdir, const CString& path, bool isDir)
{
	return g_IgnoreList.IsIgnore(path, gitdir, isDir);
}

int GitStatus::GetFileList(CString path, std::vector<CGitFileName> &list)
{
	path += L"\\*.*";
	WIN32_FIND_DATA data;
	CAutoFindFile handle = ::FindFirstFileEx(path, SysInfo::Instance().IsWin7OrLater() ? FindExInfoBasic : FindExInfoStandard, &data, FindExSearchNameMatch, nullptr, SysInfo::Instance().IsWin7OrLater() ? FIND_FIRST_EX_LARGE_FETCH : 0);
	if (!handle)
		return -1;
	do
	{
		if (wcscmp(data.cFileName, L".git") == 0)
			continue;

		if (wcscmp(data.cFileName, L".") == 0)
			continue;

		if (wcscmp(data.cFileName, L"..") == 0)
			continue;

		CGitFileName filename(data.cFileName);
		if(data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			filename.m_FileName += L'/';

		list.push_back(filename);

	}while(::FindNextFile(handle, &data));

	handle.CloseHandle(); // manually close handle here in order to keep handles open as short as possible

	std::sort(list.begin(), list.end(), SortCGitFileName);
	return 0;
}

int GitStatus::EnumDirStatus(const CString& gitdir, const CString& subpath, git_wc_status_kind* status, BOOL IsFul, BOOL /*IsRecursive*/, BOOL IsIgnore, FILL_STATUS_CALLBACK callback, void* pData)
{
	if (!status)
		return 0;

	CString path = subpath;

	path.Replace(L'\\', L'/');
	if (!path.IsEmpty() && path[path.GetLength() - 1] != L'/')
		path += L'/'; // Add trail / to show it is directory, not file name.

	std::vector<CGitFileName> filelist;
	GetFileList(CombinePath(gitdir, subpath), filelist);

	g_IndexFileMap.CheckAndUpdate(gitdir);

	g_HeadFileMap.CheckHeadAndUpdate(gitdir);

	SHARED_INDEX_PTR indexptr = g_IndexFileMap.SafeGet(gitdir);
	SHARED_TREE_PTR treeptr = g_HeadFileMap.SafeGet(gitdir);

	// new git working tree has no index file
	if (!indexptr.get())
	{
		for (auto it = filelist.cbegin(); it != filelist.cend(); ++it)
		{
			CString casepath = path;
			casepath += it->m_FileName;

			bool bIsDir = false;
			if (!it->m_FileName.IsEmpty() && it->m_FileName[it->m_FileName.GetLength() - 1] == L'/')
				bIsDir = true;

			if (IsIgnore)
			{
				g_IgnoreList.CheckAndUpdateIgnoreFiles(gitdir, casepath, bIsDir);
				if (g_IgnoreList.IsIgnore(casepath, gitdir, bIsDir))
					*status = git_wc_status_ignored;
				else if (bIsDir)
					continue;
				else
					*status = git_wc_status_unversioned;
			}
			else if (bIsDir)
				continue;
			else
				*status = git_wc_status_unversioned;

			if (callback)
				callback(CombinePath(gitdir, casepath), *status, bIsDir, pData, false, false);
		}
		return 0;
	}

	for (auto it = filelist.cbegin(), itend = filelist.cend(); it != itend; ++it)
	{
		CString onepath(path);
		onepath += it->m_FileName;

		bool bIsDir = false;
		if (!onepath.IsEmpty() && onepath[onepath.GetLength() - 1] == L'/')
			bIsDir = true;

		int matchLength = -1;
		if (bIsDir)
			matchLength = onepath.GetLength();
		size_t pos = SearchInSortVector(*indexptr, onepath, matchLength);
		size_t posintree = SearchInSortVector(*treeptr, onepath, matchLength);

		if (pos == NPOS && posintree == NPOS)
		{
			if (onepath.IsEmpty())
				continue;

			if (!IsIgnore)
			{
				*status = git_wc_status_unversioned;
				if (callback)
					callback(CombinePath(gitdir, onepath), *status, bIsDir, pData, false, false);
				continue;
			}

			g_IgnoreList.CheckAndUpdateIgnoreFiles(gitdir, onepath, bIsDir);
			if (g_IgnoreList.IsIgnore(onepath, gitdir, bIsDir))
				*status = git_wc_status_ignored;
			else
				*status = git_wc_status_unversioned;

			if (callback)
				callback(CombinePath(gitdir, onepath), *status, bIsDir, pData, false, false);
		}
		else if (pos == NPOS && posintree != NPOS) /* check if file delete in index */
		{
			*status = git_wc_status_deleted;
			if (callback)
				callback(CombinePath(gitdir, onepath), *status, bIsDir, pData, false, false);
		}
		else if (pos != NPOS && posintree == NPOS) /* Check if file added */
		{
			*status = git_wc_status_added;
			if ((*indexptr)[pos].m_Flags & GIT_IDXENTRY_STAGEMASK)
				*status = git_wc_status_conflicted;
			if (callback)
				callback(CombinePath(gitdir, onepath), *status, bIsDir, pData, false, false);
		}
		else
		{
			if (onepath.IsEmpty())
				continue;

			if (bIsDir)
			{
				*status = git_wc_status_normal;
				if (callback)
					callback(CombinePath(gitdir, onepath), *status, bIsDir, pData, false, false);
			}
			else
			{
				bool assumeValid = false;
				bool skipWorktree = false;
				git_wc_status_kind filestatus;
				GetFileStatus(gitdir, onepath, &filestatus, IsFul, true, IsIgnore, callback, pData, &assumeValid, &skipWorktree);
			}
		}
	}/*End of For*/

	/* Check deleted file in system */
	size_t start = 0, end = 0;
	size_t pos = SearchInSortVector(*indexptr, path, path.GetLength()); // match path prefix, (sub)folders end with slash
	std::set<CString> skipWorktreeSet;

	if (GetRangeInSortVector(*indexptr, path, path.GetLength(), &start, &end, pos) == 0)
	{
		CString oldstring;
		for (auto it = indexptr->cbegin() + start, itlast = indexptr->cbegin() + end; it <= itlast; ++it)
		{
			auto& entry = *it;
			int commonPrefixLength = path.GetLength();
			int index = entry.m_FileName.Find(L'/', commonPrefixLength);
			if (index < 0)
				index = entry.m_FileName.GetLength();
			else
				++index; // include slash at the end for subfolders, so that we do not match files by mistake

			CString filename = entry.m_FileName.Mid(commonPrefixLength, index - commonPrefixLength);
			if (oldstring != filename)
			{
				oldstring = filename;
				int length = filename.GetLength();
				if (SearchInSortVector(filelist, filename, filename[length - 1] == L'/' ? length : -1) == NPOS) // do full match for filenames and only prefix-match ending with "/" for folders
				{
					bool skipWorktree = false;
					*status = git_wc_status_deleted;
					if ((entry.m_FlagsExtended & GIT_IDXENTRY_SKIP_WORKTREE) != 0)
					{
						skipWorktreeSet.insert(filename);
						skipWorktree = true;
						*status = git_wc_status_normal;
					}
					if (callback)
						callback(CombinePath(gitdir, entry.m_FileName), *status, false, pData, false, skipWorktree);
				}
			}
		}
	}

	start = end = 0;
	pos = SearchInSortVector(*treeptr, path, path.GetLength()); // match path prefix, (sub)folders end with slash
	if (GetRangeInSortVector(*treeptr, path, path.GetLength(), &start, &end, pos) == 0)
	{
		CString oldstring;
		for (auto it = treeptr->cbegin() + start, itlast = treeptr->cbegin() + end; it <= itlast; ++it)
		{
			auto& entry = *it;
			int commonPrefixLength = path.GetLength();
			int index = entry.m_FileName.Find(L'/', commonPrefixLength);
			if (index < 0)
				index = entry.m_FileName.GetLength();
			else
				++index; // include slash at the end for subfolders, so that we do not match files by mistake

			CString filename = entry.m_FileName.Mid(commonPrefixLength, index - commonPrefixLength);
			if (oldstring != filename && skipWorktreeSet.find(filename) == skipWorktreeSet.cend())
			{
				oldstring = filename;
				int length = filename.GetLength();
				if (SearchInSortVector(filelist, filename, filename[length - 1] == L'/' ? length : -1) == NPOS) // do full match for filenames and only prefix-match ending with "/" for folders
				{
					*status = git_wc_status_deleted;
					if (callback)
						callback(CombinePath(gitdir, entry.m_FileName), *status, false, pData, false, false);
				}
			}
		}
	}
	return 0;
}
#endif

#ifndef TGITCACHE
int GitStatus::GetDirStatus(const CString& gitdir, const CString& subpath, git_wc_status_kind* status, BOOL IsFul, BOOL IsRecursive, BOOL IsIgnore)
{
	if (!status)
		return 0;

	CString path = subpath;

	path.Replace(L'\\', L'/');
	if (!path.IsEmpty() && path[path.GetLength() - 1] != L'/')
		path += L'/'; //Add trail / to show it is directory, not file name.

	g_IndexFileMap.CheckAndUpdate(gitdir);

	SHARED_INDEX_PTR indexptr = g_IndexFileMap.SafeGet(gitdir);

	if (!indexptr)
	{
		*status = git_wc_status_unversioned;
		return 0;
	}

	size_t pos = SearchInSortVector(*indexptr, path, path.GetLength());

	// Not In Version Contorl
	if (pos == NPOS)
	{
		if (!IsIgnore)
		{
			*status = git_wc_status_unversioned;
			return 0;
		}

		// Check ignore always.
		g_IgnoreList.CheckAndUpdateIgnoreFiles(gitdir, path, true);
		if (g_IgnoreList.IsIgnore(path, gitdir, true))
			*status = git_wc_status_ignored;
		else
			*status = git_wc_status_unversioned;

		g_HeadFileMap.CheckHeadAndUpdate(gitdir);

		SHARED_TREE_PTR treeptr = g_HeadFileMap.SafeGet(gitdir);
		// Check init repository
		if (treeptr->HeadIsEmpty() && path.IsEmpty())
			*status = git_wc_status_normal;
		// check if only one file in repository is deleted in index
		else if (path.IsEmpty() && !treeptr->empty())
			*status = git_wc_status_deleted;

		return 0;
	}

	// In version control
	*status = git_wc_status_normal;

	size_t start = 0;
	size_t end = 0;

	GetRangeInSortVector(*indexptr, path, path.GetLength(), &start, &end, pos);

	// Check Conflict;
	for (auto it = indexptr->cbegin() + start, itlast = indexptr->cbegin() + end; indexptr->m_bHasConflicts && it <= itlast; ++it)
	{
		if (((*it).m_Flags & GIT_IDXENTRY_STAGEMASK) != 0)
		{
			*status = git_wc_status_conflicted;
			break;
		}
	}

	if (IsFul && (*status != git_wc_status_conflicted))
	{
		*status = git_wc_status_normal;

		g_HeadFileMap.CheckHeadAndUpdate(gitdir);

		// Check Add
		{
			// Check if new init repository
			SHARED_TREE_PTR treeptr = g_HeadFileMap.SafeGet(gitdir);

			if (!treeptr->empty() || treeptr->HeadIsEmpty())
			{
				for (auto it = indexptr->cbegin() + start, itlast = indexptr->cbegin() + end; it <= itlast; ++it)
				{
					pos = SearchInSortVector(*treeptr, (*it).m_FileName, -1);

					if (pos == NPOS)
					{
						*status = max(git_wc_status_added, *status); // added file found
						break;
					}

					if ((*treeptr)[pos].m_Hash != (*it).m_IndexHash)
					{
						*status = max(git_wc_status_modified, *status); // modified file found
						break;
					}
				}

				// Check Delete
				if (*status == git_wc_status_normal)
				{
					pos = SearchInSortVector(*treeptr, path, path.GetLength());
					if (pos == NPOS)
						*status = max(git_wc_status_added, *status); // added file found
					else
					{
						size_t hstart, hend;
						// we know that pos exists in treeptr
						GetRangeInSortVector(*treeptr, path, path.GetLength(), &hstart, &hend, pos);
						for (auto hit = treeptr->cbegin() + hstart, lastElement = treeptr->cbegin() + hend; hit <= lastElement; ++hit)
						{
							if (SearchInSortVector(*indexptr, (*hit).m_FileName, -1) == NPOS)
							{
								*status = max(git_wc_status_deleted, *status); // deleted file found
								break;
							}
						}
					}
				}
			}
		} /* End lock*/
	}

	// When status == git_wc_status_conflicted, needn't check each file status
	// because git_wc_status_conflicted is highest.s
	if (*status == git_wc_status_conflicted)
		return 0;

	for (auto it = indexptr->cbegin() + start, itlast = indexptr->cbegin() + end; it <= itlast; ++it)
	{
		//skip child directory
		if (!IsRecursive && (*it).m_FileName.Find(L'/', path.GetLength()) > 0)
			continue;

		git_wc_status_kind filestatus = git_wc_status_none;
		bool assumeValid = false;
		bool skipWorktree = false;
		GetFileStatus(gitdir, (*it).m_FileName, &filestatus, IsFul, IsRecursive, IsIgnore, nullptr, nullptr, &assumeValid, &skipWorktree);
		switch (filestatus)
		{
		case git_wc_status_added:
		case git_wc_status_modified:
		case git_wc_status_deleted:
		case git_wc_status_conflicted:
			*status = GetMoreImportant(filestatus, *status);
		}
	}

	return 0;
}
#endif

#ifdef TGITCACHE
bool GitStatus::IsExistIndexLockFile(CString sDirName)
{
	if (!PathIsDirectory(sDirName))
	{
		int x = sDirName.ReverseFind(L'\\');
		if (x < 2)
			return false;

		sDirName.Truncate(x);
	}

	for (;;)
	{
		if (PathFileExists(CombinePath(sDirName, L".git")))
		{
			if (PathFileExists(g_AdminDirMap.GetWorktreeAdminDirConcat(sDirName, L"index.lock")))
				return true;

			return false;
		}

		int x = sDirName.ReverseFind(L'\\');
		if (x < 2)
			return false;

		sDirName.Truncate(x);
	}
}
#endif

bool GitStatus::ReleasePath(const CString &gitdir)
{
	g_IndexFileMap.SafeClear(gitdir);
	g_HeadFileMap.SafeClear(gitdir);
	return true;
}

bool GitStatus::ReleasePathsRecursively(const CString &rootpath)
{
	g_IndexFileMap.SafeClearRecursively(rootpath);
	g_HeadFileMap.SafeClearRecursively(rootpath);
	return true;
}

