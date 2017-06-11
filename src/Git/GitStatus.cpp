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
	m_status.status = git_wc_status_none;
}

// static method
#ifndef TGITCACHE
git_wc_status_kind GitStatus::GetAllStatus(const CTGitPath& path, bool bIsRecursive, bool* assumeValid, bool* skipWorktree)
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
		AdjustFolderStatus(statuskind);
	}
	else
		err = GetFileStatus(sProjectRoot, sSubPath, &statuskind, isfull, isfull, assumeValid, skipWorktree);

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
		case git_wc_status_normal:
		case git_wc_status_added:
			return 6;
		case git_wc_status_deleted:
			return 8;
		case git_wc_status_modified:
			return 10;
		case git_wc_status_conflicted:
			return 12;
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

	m_status.status = git_wc_status_none;
	m_status.assumeValid = false;
	m_status.skipWorktree = false;

	if (path.IsDirectory())
	{
		err = GetDirStatus(sProjectRoot, lpszSubPath, &m_status.status, isfull, false, !noignore);
		AdjustFolderStatus(m_status.status);
	}
	else
		err = GetFileStatus(sProjectRoot, lpszSubPath, &m_status.status, isfull, !noignore, &m_status.assumeValid, &m_status.skipWorktree);

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

int GitStatus::GetFileStatus(const CString& gitdir, CString path, git_wc_status_kind* status, BOOL IsFull, BOOL IsIgnore, bool* assumeValid, bool* skipWorktree)
{
	if (!status)
		return 0;

	path.Replace(L'\\', L'/');

	git_wc_status_kind st = git_wc_status_none;
	CGitHash hash;

	g_IndexFileMap.GetFileStatus(gitdir, path, &st, &hash, assumeValid, skipWorktree);

	if (st == git_wc_status_conflicted)
	{
		*status = st;
		return 0;
	}

	if (st == git_wc_status_unversioned)
	{
		if (IsFull)
		{
			g_HeadFileMap.CheckHeadAndUpdate(gitdir);

			// Check Head Tree Hash
			SHARED_TREE_PTR treeptr = g_HeadFileMap.SafeGet(gitdir);

			// deleted only in index item?
			if (SearchInSortVector(*treeptr, path, -1) != NPOS)
			{
				*status = git_wc_status_deleted;
				return 0;
			}
		}

		if (!IsIgnore)
		{
			*status = git_wc_status_unversioned;
			return 0;
		}

		g_IgnoreList.CheckAndUpdateIgnoreFiles(gitdir, path, false);
		if (g_IgnoreList.IsIgnore(path, gitdir, false))
			st = git_wc_status_ignored;

		*status = st;
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
			return 0;
		}

		// staged and not commit
		if (!assumeValid && !skipWorktree &&(*treeptr)[start].m_Hash != hash)
		{
			*status = st = git_wc_status_modified;
			return 0;
		}
	}
	*status = st;
	return 0;
}

// checks whether indexPath is a direct submodule and not one in a subfolder
static bool IsDirectSubmodule(const CString& indexPath, int prefix)
{
	if (!CStringUtils::EndsWith(indexPath, L'/'))
		return false;

	auto ptr = indexPath.GetString() + prefix;
	int folderdepth = 0;
	while (*ptr)
	{
		if (*ptr == L'/')
			++folderdepth;
		++ptr;
	}

	return folderdepth == 1;
}

#ifdef TGITCACHE
bool GitStatus::CheckAndUpdateIgnoreFiles(const CString& gitdir, const CString& subpaths, bool isDir)
{
	return g_IgnoreList.CheckAndUpdateIgnoreFiles(gitdir, subpaths, isDir);
}

bool GitStatus::IsIgnored(const CString& gitdir, const CString& path, bool isDir)
{
	return g_IgnoreList.IsIgnore(path, gitdir, isDir);
}

int GitStatus::GetFileList(CString path, std::vector<CGitFileName>& list, bool& isRepoRoot)
{
	path += L"\\*.*";
	WIN32_FIND_DATA data;
	CAutoFindFile handle = ::FindFirstFileEx(path, FindExInfoBasic, &data, FindExSearchNameMatch, nullptr, FIND_FIRST_EX_LARGE_FETCH);
	if (!handle)
		return -1;
	do
	{
		if (wcscmp(data.cFileName, L".git") == 0)
		{
			isRepoRoot = true;
			continue;
		}

		if (wcscmp(data.cFileName, L".") == 0)
			continue;

		if (wcscmp(data.cFileName, L"..") == 0)
			continue;

		CGitFileName filename(data.cFileName, ((__int64)data.nFileSizeHigh << 32) + data.nFileSizeLow, ((__int64)data.ftLastWriteTime.dwHighDateTime << 32) + data.ftLastWriteTime.dwLowDateTime);
		if(data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			filename.m_FileName += L'/';

		list.push_back(filename);

	}while(::FindNextFile(handle, &data));

	handle.CloseHandle(); // manually close handle here in order to keep handles open as short as possible

	std::sort(list.begin(), list.end(), SortCGitFileName);
	return 0;
}

int GitStatus::EnumDirStatus(const CString& gitdir, const CString& subpath, git_wc_status_kind* dirstatus, FILL_STATUS_CALLBACK callback, void* pData)
{
	CString path = subpath;

	path.Replace(L'\\', L'/');
	if (!path.IsEmpty() && path[path.GetLength() - 1] != L'/')
		path += L'/'; // Add trail / to show it is directory, not file name.

	std::vector<CGitFileName> filelist;
	bool isRepoRoot = false;
	GetFileList(CombinePath(gitdir, subpath), filelist, isRepoRoot);
	*dirstatus = git_wc_status_unknown;
	if (isRepoRoot)
		*dirstatus = git_wc_status_normal;

	g_IndexFileMap.CheckAndUpdate(gitdir);

	g_HeadFileMap.CheckHeadAndUpdate(gitdir);

	SHARED_INDEX_PTR indexptr = g_IndexFileMap.SafeGet(gitdir);
	SHARED_TREE_PTR treeptr = g_HeadFileMap.SafeGet(gitdir);

	// working tree has broken index file
	if (!indexptr.get())
	{
		for (auto it = filelist.cbegin(); it != filelist.cend(); ++it)
		{
			CString casepath = path;
			casepath += it->m_FileName;

			bool bIsDir = false;
			if (!it->m_FileName.IsEmpty() && it->m_FileName[it->m_FileName.GetLength() - 1] == L'/')
				bIsDir = true;

			g_IgnoreList.CheckAndUpdateIgnoreFiles(gitdir, casepath, bIsDir);
			git_wc_status_kind filestatus = git_wc_status_unversioned;
			if (g_IgnoreList.IsIgnore(casepath, gitdir, bIsDir))
				filestatus = git_wc_status_ignored;
			else if (bIsDir)
				continue;

			callback(CombinePath(gitdir, casepath), filestatus, bIsDir, it->m_LastModified, pData, false, false);
		}
		return 0;
	}

	CAutoRepository repository;
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
			git_wc_status_kind filestatus = git_wc_status_unversioned;

			g_IgnoreList.CheckAndUpdateIgnoreFiles(gitdir, onepath, bIsDir);
			if (g_IgnoreList.IsIgnore(onepath, gitdir, bIsDir))
				filestatus = git_wc_status_ignored;

			callback(CombinePath(gitdir, onepath), filestatus, bIsDir, it->m_LastModified, pData, false, false);
		}
		else if (pos == NPOS && posintree != NPOS) /* check if file delete in index */
			callback(CombinePath(gitdir, onepath), git_wc_status_deleted, bIsDir, it->m_LastModified, pData, false, false);
		else if (pos != NPOS && posintree == NPOS) /* Check if file added */
		{
			git_wc_status_kind filestatus = git_wc_status_added;
			if ((*indexptr)[pos].m_Flags & GIT_IDXENTRY_STAGEMASK)
				filestatus = git_wc_status_conflicted;
			callback(CombinePath(gitdir, onepath), filestatus, bIsDir, it->m_LastModified, pData, false, false);
		}
		else
		{
			if (bIsDir)
				callback(CombinePath(gitdir, onepath), git_wc_status_normal, bIsDir, it->m_LastModified, pData, false, false);
			else
			{
				if ((*indexptr)[pos].m_Flags & GIT_IDXENTRY_STAGEMASK)
				{
					callback(CombinePath(gitdir, onepath), git_wc_status_conflicted, false, it->m_LastModified, pData, false, false);
					continue;
				}
				bool assumeValid = false;
				bool skipWorktree = false;
				git_wc_status_kind filestatus;
				(*indexptr).GetFileStatus(repository, gitdir, (*indexptr)[pos], &filestatus, CGit::filetime_to_time_t((*it).m_LastModified), (*it).m_Size, &assumeValid, &skipWorktree);
				if (filestatus == git_wc_status_normal && !assumeValid && !skipWorktree && (*treeptr)[posintree].m_Hash != (*indexptr)[pos].m_IndexHash)
					filestatus = git_wc_status_modified;
				callback(CombinePath(gitdir, onepath), filestatus, false, it->m_LastModified, pData, assumeValid, skipWorktree);
			}
		}
	}/*End of For*/
	repository.Free(); // explicitly free the handle here in order to keep an open repository as short as possible

	/* Check deleted file in system */
	size_t start = 0, end = 0;
	size_t pos = SearchInSortVector(*indexptr, path, path.GetLength()); // match path prefix, (sub)folders end with slash
	std::set<CString> alreadyReported;

	if (GetRangeInSortVector(*indexptr, path, path.GetLength(), &start, &end, pos) == 0)
	{
		*dirstatus = git_wc_status_normal; // here we know that this folder has versioned entries
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
				bool isDir = filename[length - 1] == L'/';
				if (SearchInSortVector(filelist, filename, isDir ? length : -1) == NPOS) // do full match for filenames and only prefix-match ending with "/" for folders
				{
					bool skipWorktree = false;
					git_wc_status_kind filestatus = (!isDir || IsDirectSubmodule(entry.m_FileName, commonPrefixLength)) ? git_wc_status_deleted : git_wc_status_modified; // only report deleted submodules and files as deletedy
					if ((entry.m_FlagsExtended & GIT_IDXENTRY_SKIP_WORKTREE) != 0)
					{
						skipWorktree = true;
						filestatus = git_wc_status_normal;
						oldstring.Empty(); // without this a deleted folder which has two versioned files and only the first is skipwoktree flagged gets reported as normal
						if (alreadyReported.find(filename) != alreadyReported.cend())
							continue;
					}
					alreadyReported.insert(filename);
					callback(CombinePath(gitdir, subpath, filename), filestatus, isDir, 0, pData, false, skipWorktree);
				}
			}
		}
	}

	start = end = 0;
	pos = SearchInSortVector(*treeptr, path, path.GetLength()); // match path prefix, (sub)folders end with slash
	if (GetRangeInSortVector(*treeptr, path, path.GetLength(), &start, &end, pos) == 0)
	{
		*dirstatus = git_wc_status_normal; // here we know that this folder has versioned entries
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
			if (oldstring != filename && alreadyReported.find(filename) == alreadyReported.cend())
			{
				oldstring = filename;
				int length = filename.GetLength();
				bool isDir = filename[length - 1] == L'/';
				if (SearchInSortVector(filelist, filename, isDir ? length : -1) == NPOS) // do full match for filenames and only prefix-match ending with "/" for folders
					callback(CombinePath(gitdir, subpath, filename), (!isDir || IsDirectSubmodule(entry.m_FileName, commonPrefixLength)) ? git_wc_status_deleted : git_wc_status_modified, isDir, 0, pData, false, false);
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
			// WC root is at least normal if there are no files added/deleted
			if (subpath.IsEmpty())
			{
				*status = git_wc_status_normal;
				return 0;
			}
			*status = git_wc_status_unversioned;
			return 0;
		}

		g_HeadFileMap.CheckHeadAndUpdate(gitdir);

		SHARED_TREE_PTR treeptr = g_HeadFileMap.SafeGet(gitdir);

		// check whether there files in head with are not in index
		pos = SearchInSortVector(*treeptr, path, path.GetLength());
		if (pos != NPOS)
		{
			*status = git_wc_status_deleted;
			return 0;
		}

		// WC root is at least normal if there are no files added/deleted
		if (path.IsEmpty())
		{
			*status = git_wc_status_normal;
			return 0;
		}

		// Check ignore
		g_IgnoreList.CheckAndUpdateIgnoreFiles(gitdir, path, true);
		if (g_IgnoreList.IsIgnore(path, gitdir, true))
			*status = git_wc_status_ignored;
		else
			*status = git_wc_status_unversioned;

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
			// When status == git_wc_status_conflicted, we don't need to check each file status
			// because git_wc_status_conflicted is the highest.
			return 0;
		}
	}

	if (IsFul)
	{
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
						*status = GetMoreImportant(git_wc_status_added, *status); // added file found
						AdjustFolderStatus(*status);
						if (GetMoreImportant(*status, git_wc_status_modified) == *status) // the only potential higher status which me might get in this loop
							break;
						continue;
					}

					if (((*it).m_Flags & GIT_IDXENTRY_VALID) == 0 && ((*it).m_FlagsExtended & GIT_IDXENTRY_SKIP_WORKTREE) == 0 && (*treeptr)[pos].m_Hash != (*it).m_IndexHash)
					{
						*status = GetMoreImportant(git_wc_status_modified, *status); // modified file found
						break;
					}
				}

				// Check Delete
				if (*status == git_wc_status_normal)
				{
					pos = SearchInSortVector(*treeptr, path, path.GetLength());
					if (pos == NPOS)
						*status = GetMoreImportant(git_wc_status_added, *status); // added file found
					else
					{
						size_t hstart, hend;
						// we know that pos exists in treeptr
						GetRangeInSortVector(*treeptr, path, path.GetLength(), &hstart, &hend, pos);
						for (auto hit = treeptr->cbegin() + hstart, lastElement = treeptr->cbegin() + hend; hit <= lastElement; ++hit)
						{
							if (SearchInSortVector(*indexptr, (*hit).m_FileName, -1) == NPOS)
							{
								*status = GetMoreImportant(git_wc_status_deleted, *status); // deleted file found
								break;
							}
						}
					}
				}
			}
		} /* End lock*/
	}

	auto mostImportantPossibleFolderStatus = GetMoreImportant(git_wc_status_added, GetMoreImportant(git_wc_status_modified, git_wc_status_deleted));
	AdjustFolderStatus(mostImportantPossibleFolderStatus);
	// we can skip here when we already have the highest possible status
	if (mostImportantPossibleFolderStatus == *status)
		return 0;

	for (auto it = indexptr->cbegin() + start, itlast = indexptr->cbegin() + end; it <= itlast; ++it)
	{
		// skip child directory, but handle submodules
		if (!IsRecursive && (*it).m_FileName.Find(L'/', path.GetLength()) > 0 && !IsDirectSubmodule((*it).m_FileName, path.GetLength()))
			continue;

		git_wc_status_kind filestatus = git_wc_status_none;
		bool assumeValid = false;
		bool skipWorktree = false;
		GetFileStatus(gitdir, (*it).m_FileName, &filestatus, IsFul, IsIgnore, &assumeValid, &skipWorktree);
		switch (filestatus)
		{
		case git_wc_status_added:
		case git_wc_status_modified:
		case git_wc_status_deleted:
		//case git_wc_status_conflicted: cannot happen, we exit as soon we found a conflict in subpath
			*status = GetMoreImportant(filestatus, *status);
			AdjustFolderStatus(*status);
			if (mostImportantPossibleFolderStatus == *status)
				return 0;
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

void GitStatus::AdjustFolderStatus(git_wc_status_kind& status)
{
	if (status == git_wc_status_deleted || status == git_wc_status_added)
		status = git_wc_status_modified;
}
