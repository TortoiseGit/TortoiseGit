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

#include "stdafx.h"
#include "registry.h"
#include "../TortoiseShell/resource.h"
#include "GitStatus.h"
#include "UnicodeUtils.h"
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
int GitStatus::GetAllStatus(const CTGitPath& path, bool bIsRecursive, git_wc_status2_t& status)
{
	BOOL						isDir;
	CString						sProjectRoot;

	isDir = path.IsDirectory();
	if (!path.HasAdminDir(&sProjectRoot))
		return git_wc_status_none;

	CString sSubPath;
	CString s = path.GetWinPathString();
	if (s.GetLength() > sProjectRoot.GetLength())
	{
		if (sProjectRoot.GetLength() == 3 && sProjectRoot[1] == L':')
			sSubPath = s.Right(s.GetLength() - sProjectRoot.GetLength());
		else
			sSubPath = s.Right(s.GetLength() - sProjectRoot.GetLength() - 1/*otherwise it gets initial slash*/);
	}

	bool isfull = (static_cast<DWORD>(CRegStdDWORD(L"Software\\TortoiseGit\\CacheType", GetSystemMetrics(SM_REMOTESESSION)) ? ShellCache::dll : ShellCache::exe) == ShellCache::dllFull);

	if(isDir)
	{
		auto err = GetDirStatus(sProjectRoot, sSubPath, &status.status, isfull, bIsRecursive, isfull);
		AdjustFolderStatus(status.status);
		return err;
	}

	return GetFileStatus(sProjectRoot, sSubPath, status, isfull, isfull);
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

	bool isfull = (static_cast<DWORD>(CRegStdDWORD(L"Software\\TortoiseGit\\CacheType", GetSystemMetrics(SM_REMOTESESSION)) ? ShellCache::dll : ShellCache::exe) == ShellCache::dllFull);

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
		err = GetFileStatus(sProjectRoot, lpszSubPath, m_status, isfull, !noignore);

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

typedef struct CGitRepoLists
{
	SHARED_INDEX_PTR pIndex;
	SHARED_TREE_PTR pTree;
} CGitRepoLists;

static int GetFileStatus_int(const CString& gitdir, CGitRepoLists& repolists, const CString& path, git_wc_status2_t& status, BOOL IsFull, BOOL IsIgnore, BOOL update)
{
	ATLASSERT(repolists.pIndex);
	ATLASSERT(!status.assumeValid && !status.skipWorktree);

	CGitHash hash;
	if (repolists.pIndex->GetFileStatus(gitdir, path, status, &hash))
	{
		// an error occurred in GetFileStatus
		status.status = git_wc_status_none;
		return -1;
	}

	if (status.status == git_wc_status_conflicted)
		return 0;

	if (status.status == git_wc_status_unversioned)
	{
		if (IsFull)
		{
			if (!repolists.pTree)
			{
				if (update)
					g_HeadFileMap.CheckHeadAndUpdate(gitdir, repolists.pIndex->IsIgnoreCase());

				// Check Head Tree Hash
				repolists.pTree = g_HeadFileMap.SafeGet(gitdir);
			}
			// broken HEAD
			if (!repolists.pTree)
			{
				status.status = git_wc_status_none;
				return -1;
			}

			// deleted only in index item?
			if (SearchInSortVector(*repolists.pTree, path, -1, repolists.pIndex->IsIgnoreCase()) != NPOS)
			{
				status.status = git_wc_status_deleted;
				return 0;
			}
		}

		if (!IsIgnore)
		{
			status.status = git_wc_status_unversioned;
			return 0;
		}

		g_IgnoreList.CheckAndUpdateIgnoreFiles(gitdir, path, false);
		if (g_IgnoreList.IsIgnore(path, gitdir, false, g_AdminDirMap.GetAdminDir(gitdir)))
			status.status = git_wc_status_ignored;

		return 0;
	}

	if ((status.status == git_wc_status_normal || status.status == git_wc_status_modified) && IsFull)
	{
		if (!repolists.pTree)
		{
			if (update)
				g_HeadFileMap.CheckHeadAndUpdate(gitdir, repolists.pIndex->IsIgnoreCase());

			// Check Head Tree Hash
			repolists.pTree = g_HeadFileMap.SafeGet(gitdir);
		}
		// broken HEAD
		if (!repolists.pTree)
		{
			status.status = git_wc_status_none;
			return -1;
		}

		//add item
		size_t start = SearchInSortVector(*repolists.pTree, path, -1, repolists.pIndex->IsIgnoreCase());
		if (start == NPOS)
		{
			status.status = git_wc_status_added;
			CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": File miss in head tree %s", static_cast<LPCTSTR>(path));
			return 0;
		}

		// staged and not commit
		if ((*repolists.pTree)[start].m_Hash != hash)
		{
			status = { git_wc_status_modified, false, false };
			return 0;
		}
	}

	return 0;
}

int GitStatus::GetFileStatus(const CString& gitdir, CString path, git_wc_status2_t& status, BOOL IsFull, BOOL IsIgnore, bool update)
{
	ATLASSERT(!status.assumeValid && !status.skipWorktree);

	path.Replace(L'\\', L'/');

	CGitRepoLists sharedRepoLists;
	if (update)
		g_IndexFileMap.CheckAndUpdate(gitdir);
	sharedRepoLists.pIndex = g_IndexFileMap.SafeGet(gitdir);
	if (!sharedRepoLists.pIndex)
	{
		// git working tree has broken index
		status.status = git_wc_status_none;
		return -1;
	}

	return GetFileStatus_int(gitdir, sharedRepoLists, path, status, IsFull, IsIgnore, update);
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
int GitStatus::GetFileList(const CString& path, std::vector<CGitFileName>& list, bool& isRepoRoot, bool ignoreCase)
{
	WIN32_FIND_DATA data;
	CAutoFindFile handle = ::FindFirstFileEx(CombinePath(path, L"*.*"), FindExInfoBasic, &data, FindExSearchNameMatch, nullptr, FIND_FIRST_EX_LARGE_FETCH);
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

		CGitFileName filename(data.cFileName, static_cast<__int64>(data.nFileSizeHigh) << 32 | data.nFileSizeLow, static_cast<__int64>(data.ftLastWriteTime.dwHighDateTime) << 32 | data.ftLastWriteTime.dwLowDateTime);
		if ((data.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) && !CPathUtils::ReadLink(CombinePath(path, filename.m_FileName)))
			filename.m_bSymlink = true;
		else if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			filename.m_FileName += L'/';

		list.push_back(filename);

	}while(::FindNextFile(handle, &data));

	handle.CloseHandle(); // manually close handle here in order to keep handles open as short as possible

	DoSortFilenametSortVector(list, ignoreCase);

	return 0;
}

int GitStatus::EnumDirStatus(const CString& gitdir, const CString& subpath, git_wc_status_kind* dirstatus, FILL_STATUS_CALLBACK callback, void* pData)
{
	CString path = subpath;

	path.Replace(L'\\', L'/');
	if (!path.IsEmpty() && path[path.GetLength() - 1] != L'/')
		path += L'/'; // Add trail / to show it is directory, not file name.

	g_IndexFileMap.CheckAndUpdate(gitdir);

	SHARED_INDEX_PTR indexptr = g_IndexFileMap.SafeGet(gitdir);
	// there was an error loading the index
	if (!indexptr)
		return -1;

	g_HeadFileMap.CheckHeadAndUpdate(gitdir, indexptr->IsIgnoreCase());

	SHARED_TREE_PTR treeptr = g_HeadFileMap.SafeGet(gitdir);
	// there was an error loading the HEAD commit/tree
	if (!treeptr)
		return -1;

	size_t indexpos = SearchInSortVector(*indexptr, path, path.GetLength(), indexptr->IsIgnoreCase()); // match path prefix, (sub)folders end with slash
	size_t treepos = SearchInSortVector(*treeptr, path, path.GetLength(), indexptr->IsIgnoreCase()); // match path prefix, (sub)folders end with slash

	std::set<CString> localLastCheckCache;
	CString adminDir = g_AdminDirMap.GetAdminDir(gitdir);

	std::vector<CGitFileName> filelist;
	int folderignoredchecked = false;
	bool isRepoRoot = false;
	GetFileList(CombinePath(gitdir, subpath), filelist, isRepoRoot, indexptr->IsIgnoreCase());
	*dirstatus = git_wc_status_unknown;
	if (isRepoRoot)
		*dirstatus = git_wc_status_normal;
	else if (indexpos == NPOS && treepos == NPOS)
	{
		// if folder does not contain any versioned items, it might be ignored
		g_IgnoreList.CheckAndUpdateIgnoreFiles(gitdir, subpath, true, &localLastCheckCache);
		if (g_IgnoreList.IsIgnore(subpath, gitdir, true, adminDir))
			*dirstatus = git_wc_status_ignored;
		folderignoredchecked = true;
	}

	CAutoRepository repository;
	for (auto it = filelist.cbegin(), itend = filelist.cend(); it != itend; ++it)
	{
		auto& fileentry = *it;

		CString onepath(path);
		onepath += fileentry.m_FileName;

		bool bIsDir = false;
		if (!onepath.IsEmpty() && onepath[onepath.GetLength() - 1] == L'/')
			bIsDir = true;

		int matchLength = -1;
		if (bIsDir)
			matchLength = onepath.GetLength();
		size_t pos = SearchInSortVector(*indexptr, onepath, matchLength, indexptr->IsIgnoreCase());
		size_t posintree = SearchInSortVector(*treeptr, onepath, matchLength, indexptr->IsIgnoreCase());

		git_wc_status2_t status = { git_wc_status_none, false, false };

		if (pos == NPOS && posintree == NPOS)
		{
			if (*dirstatus == git_wc_status_ignored)
				status.status = git_wc_status_ignored;
			else
			{
				status.status = git_wc_status_unversioned;

				g_IgnoreList.CheckAndUpdateIgnoreFiles(gitdir, onepath, bIsDir, &localLastCheckCache);
				// whole folder might be ignored, check this once if we are not the root folder in order to speed up all following ignored files
				if (!folderignoredchecked && *dirstatus != git_wc_status_normal)
				{
					if (g_IgnoreList.IsIgnore(subpath, gitdir, true, adminDir))
					{
						*dirstatus = git_wc_status_ignored;
						status.status = git_wc_status_ignored;
					}
					folderignoredchecked = true;
				}
				if (status.status != git_wc_status_ignored && g_IgnoreList.IsIgnore(onepath, gitdir, bIsDir, adminDir))
					status.status = git_wc_status_ignored;
			}
			callback(CombinePath(gitdir, onepath), &status, bIsDir, fileentry.m_LastModified, pData);
		}
		else if (pos == NPOS && posintree != NPOS) /* check if file delete in index */
		{
			status.status = git_wc_status_deleted;
			callback(CombinePath(gitdir, onepath), &status, bIsDir, fileentry.m_LastModified, pData);
		}
		else if (pos != NPOS && posintree == NPOS) /* Check if file added */
		{
			status.status = git_wc_status_added;
			if ((*indexptr)[pos].m_Flags & GIT_INDEX_ENTRY_STAGEMASK)
				status.status = git_wc_status_conflicted;
			callback(CombinePath(gitdir, onepath), &status, bIsDir, fileentry.m_LastModified, pData);
		}
		else
		{
			if (bIsDir)
			{
				status.status = git_wc_status_normal;
				callback(CombinePath(gitdir, onepath), &status, bIsDir, fileentry.m_LastModified, pData);
			}
			else
			{
				auto& indexentry = (*indexptr)[pos];
				if (indexentry.m_Flags & GIT_INDEX_ENTRY_STAGEMASK)
				{
					status.status = git_wc_status_conflicted;
					callback(CombinePath(gitdir, onepath), &status, false, fileentry.m_LastModified, pData);
					continue;
				}
				if ((*indexptr).GetFileStatus(repository, gitdir, indexentry, status, fileentry.m_LastModified, fileentry.m_Size, fileentry.m_bSymlink))
					return -1;
				if (status.status == git_wc_status_normal && (*treeptr)[posintree].m_Hash != indexentry.m_IndexHash)
					status = { git_wc_status_modified, false, false };
				callback(CombinePath(gitdir, onepath), &status, false, fileentry.m_LastModified, pData);
			}
		}
	}/*End of For*/
	repository.Free(); // explicitly free the handle here in order to keep an open repository as short as possible

	/* Check deleted file in system */
	size_t start = 0, end = 0;
	std::set<CString> alreadyReported;
	if (GetRangeInSortVector(*indexptr, path, path.GetLength(), indexptr->IsIgnoreCase(), &start, &end, indexpos) == 0)
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
				if (SearchInSortVector(filelist, filename, isDir ? length : -1, indexptr->IsIgnoreCase()) == NPOS) // do full match for filenames and only prefix-match ending with "/" for folders
				{
					git_wc_status2_t status = { (!isDir || IsDirectSubmodule(entry.m_FileName, commonPrefixLength)) ? git_wc_status_deleted : git_wc_status_modified, false, false }; // only report deleted submodules and files as deletedy
					if ((entry.m_FlagsExtended & GIT_INDEX_ENTRY_SKIP_WORKTREE) != 0)
					{
						status.skipWorktree = true;
						status.status = git_wc_status_normal;
						oldstring.Empty(); // without this a deleted folder which has two versioned files and only the first is skipwoktree flagged gets reported as normal
						if (alreadyReported.find(filename) != alreadyReported.cend())
							continue;
					}
					alreadyReported.insert(filename);
					callback(CombinePath(gitdir, subpath, filename), &status, isDir, 0, pData);
					if (isDir)
					{
						// folder might be replaced by symlink
						filename.TrimRight(L'/');
						auto filepos = SearchInSortVector(filelist, filename, -1, indexptr->IsIgnoreCase());
						if (filepos == NPOS || !filelist[filepos].m_bSymlink)
							continue;
						status.status = git_wc_status_deleted;
						callback(CombinePath(gitdir, subpath, filename), &status, false, 0, pData);
					}
				}
			}
		}
	}

	start = end = 0;
	if (GetRangeInSortVector(*treeptr, path, path.GetLength(), indexptr->IsIgnoreCase(), &start, &end, treepos) == 0)
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
				if (SearchInSortVector(filelist, filename, isDir ? length : -1, indexptr->IsIgnoreCase()) == NPOS) // do full match for filenames and only prefix-match ending with "/" for folders
				{
					git_wc_status2_t status = { (!isDir || IsDirectSubmodule(entry.m_FileName, commonPrefixLength)) ? git_wc_status_deleted : git_wc_status_modified, false, false };
					callback(CombinePath(gitdir, subpath, filename), &status, isDir, 0, pData);
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
	ATLASSERT(status);

	CString path = subpath;

	path.Replace(L'\\', L'/');
	if (!path.IsEmpty() && path[path.GetLength() - 1] != L'/')
		path += L'/'; //Add trail / to show it is directory, not file name.

	g_IndexFileMap.CheckAndUpdate(gitdir);

	CGitRepoLists sharedRepoLists;
	sharedRepoLists.pIndex = g_IndexFileMap.SafeGet(gitdir);

	// broken index
	if (!sharedRepoLists.pIndex)
	{
		*status = git_wc_status_none;
		return -1;
	}

	size_t pos = SearchInSortVector(*sharedRepoLists.pIndex, path, path.GetLength(), sharedRepoLists.pIndex->IsIgnoreCase());

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

		g_HeadFileMap.CheckHeadAndUpdate(gitdir, sharedRepoLists.pIndex->IsIgnoreCase());

		sharedRepoLists.pTree = g_HeadFileMap.SafeGet(gitdir);
		// broken HEAD
		if (!sharedRepoLists.pTree)
		{
			*status = git_wc_status_none;
			return -1;
		}

		// check whether there files in head with are not in index
		pos = SearchInSortVector(*sharedRepoLists.pTree, path, path.GetLength(), sharedRepoLists.pIndex->IsIgnoreCase());
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
		if (g_IgnoreList.IsIgnore(path, gitdir, true, g_AdminDirMap.GetAdminDir(gitdir)))
			*status = git_wc_status_ignored;
		else
			*status = git_wc_status_unversioned;

		return 0;
	}

	// In version control
	*status = git_wc_status_normal;

	size_t start = 0;
	size_t end = 0;

	GetRangeInSortVector(*sharedRepoLists.pIndex, path,path.GetLength(), sharedRepoLists.pIndex->IsIgnoreCase(), &start, &end, pos);

	// Check Conflict;
	for (auto it = sharedRepoLists.pIndex->cbegin() + start, itlast = sharedRepoLists.pIndex->cbegin() + end; sharedRepoLists.pIndex->m_bHasConflicts && it <= itlast; ++it)
	{
		if (((*it).m_Flags & GIT_INDEX_ENTRY_STAGEMASK) != 0)
		{
			*status = git_wc_status_conflicted;
			// When status == git_wc_status_conflicted, we don't need to check each file status
			// because git_wc_status_conflicted is the highest.
			return 0;
		}
	}

	if (IsFul)
	{
		g_HeadFileMap.CheckHeadAndUpdate(gitdir, sharedRepoLists.pIndex->IsIgnoreCase());

		// Check Add
		{
			// Check if new init repository
			sharedRepoLists.pTree = g_HeadFileMap.SafeGet(gitdir);
			// broken HEAD
			if (!sharedRepoLists.pTree)
			{
				*status = git_wc_status_none;
				return -1;
			}

			{
				for (auto it = sharedRepoLists.pIndex->cbegin() + start, itlast = sharedRepoLists.pIndex->cbegin() + end; it <= itlast; ++it)
				{
					auto& indexentry = *it;
					pos = SearchInSortVector(*sharedRepoLists.pTree, indexentry.m_FileName, -1, sharedRepoLists.pIndex->IsIgnoreCase());

					if (pos == NPOS)
					{
						*status = GetMoreImportant(git_wc_status_added, *status); // added file found
						AdjustFolderStatus(*status);
						if (GetMoreImportant(*status, git_wc_status_modified) == *status) // the only potential higher status which me might get in this loop
							break;
						continue;
					}

					if ((*sharedRepoLists.pTree)[pos].m_Hash != indexentry.m_IndexHash)
					{
						*status = GetMoreImportant(git_wc_status_modified, *status); // modified file found
						break;
					}
				}

				// Check Delete
				if (*status == git_wc_status_normal)
				{
					pos = SearchInSortVector(*sharedRepoLists.pTree, path, path.GetLength(), sharedRepoLists.pIndex->IsIgnoreCase());
					if (pos == NPOS)
						*status = GetMoreImportant(git_wc_status_added, *status); // added file found
					else
					{
						size_t hstart, hend;
						// we know that pos exists in treeptr
						GetRangeInSortVector(*sharedRepoLists.pTree, path, path.GetLength(), sharedRepoLists.pIndex->IsIgnoreCase(), &hstart, &hend, pos);
						for (auto hit = sharedRepoLists.pTree->cbegin() + hstart, lastElement = sharedRepoLists.pTree->cbegin() + hend; hit <= lastElement; ++hit)
						{
							if (SearchInSortVector(*sharedRepoLists.pIndex, (*hit).m_FileName, -1, sharedRepoLists.pIndex->IsIgnoreCase()) == NPOS)
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

	for (auto it = sharedRepoLists.pIndex->cbegin() + start, itlast = sharedRepoLists.pIndex->cbegin() + end; it <= itlast; ++it)
	{
		auto& indexentry = *it;
		// skip child directory, but handle submodules
		if (!IsRecursive && indexentry.m_FileName.Find(L'/', path.GetLength()) > 0 && !IsDirectSubmodule(indexentry.m_FileName, path.GetLength()))
			continue;

		git_wc_status2_t filestatus = { git_wc_status_none, false, false };
		GetFileStatus_int(gitdir, sharedRepoLists, indexentry.m_FileName, filestatus, IsFul, IsIgnore, false);
		switch (filestatus.status)
		{
		case git_wc_status_added:
		case git_wc_status_modified:
		case git_wc_status_deleted:
		//case git_wc_status_conflicted: cannot happen, we exit as soon we found a conflict in subpath
			*status = GetMoreImportant(filestatus.status, *status);
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
