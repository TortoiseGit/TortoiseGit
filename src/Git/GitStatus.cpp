// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2014 - TortoiseGit

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

extern CGitAdminDirMap g_AdminDirMap;
extern CGitIndexFileMap g_IndexFileMap;
CGitHeadFileMap g_HeadFileMap;
CGitIgnoreList  g_IgnoreList;

GitStatus::GitStatus()
	: status(NULL)
	, m_allstatus(git_wc_status_none)
{
	m_status.assumeValid = m_status.skipWorktree = false;
	m_status.prop_status = m_status.text_status = git_wc_status_none;
}

GitStatus::~GitStatus(void)
{
}

// static method
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
		if (sProjectRoot.GetLength() == 3 && sProjectRoot[1] == _T(':'))
			sSubPath = s.Right(s.GetLength() - sProjectRoot.GetLength());
		else
			sSubPath = s.Right(s.GetLength() - sProjectRoot.GetLength() - 1/*otherwise it gets initial slash*/);
	}

	bool isfull = ((DWORD)CRegStdDWORD(_T("Software\\TortoiseGit\\CacheType"),
				GetSystemMetrics(SM_REMOTESESSION) ? ShellCache::dll : ShellCache::exe) == ShellCache::dllFull);

	if(isDir)
	{
		err = GetDirStatus(sProjectRoot,sSubPath,&statuskind, isfull,bIsRecursive,isfull,NULL, NULL);
		// folders must not be displayed as added or deleted only as modified (this is for Shell Overlay-Modes)
		if (statuskind == git_wc_status_unversioned && sSubPath.IsEmpty())
			statuskind = git_wc_status_normal;
		else if (statuskind == git_wc_status_deleted || statuskind == git_wc_status_added)
			statuskind = git_wc_status_modified;
	}
	else
	{
		err = GetFileStatus(sProjectRoot, sSubPath, &statuskind, isfull, false, isfull, NULL, NULL, assumeValid, skipWorktree);
	}

	return statuskind;
}

// static method
git_wc_status_kind GitStatus::GetAllStatusRecursive(const CTGitPath& path)
{
	return GetAllStatus(path, git_depth_infinity);
}

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

void GitStatus::GetStatus(const CTGitPath& path, bool /*update*/ /* = false */, bool noignore /* = false */, bool /*noexternals*/ /* = false */)
{
	// NOTE: unlike the SVN version this one does not cache the enumerated files, because in practice no code in all of
	//       Tortoise uses this, all places that call GetStatus create a temp GitStatus object which gets destroyed right
	//       after the call again

	CString sProjectRoot;
	if ( !path.HasAdminDir(&sProjectRoot) )
		return;

	bool isfull = ((DWORD)CRegStdDWORD(_T("Software\\TortoiseGit\\CacheType"),
				GetSystemMetrics(SM_REMOTESESSION) ? ShellCache::dll : ShellCache::exe) == ShellCache::dllFull);

	int err = 0;

	{
		LPCTSTR lpszSubPath = NULL;
		CString sSubPath;
		CString s = path.GetWinPathString();
		if (s.GetLength() > sProjectRoot.GetLength())
		{
			sSubPath = s.Right(s.GetLength() - sProjectRoot.GetLength());
			lpszSubPath = sSubPath;
			// skip initial slash if necessary
			if (*lpszSubPath == _T('\\'))
				++lpszSubPath;
		}

		m_status.prop_status = m_status.text_status = git_wc_status_none;
		m_status.assumeValid = false;
		m_status.skipWorktree = false;

		if(path.IsDirectory())
		{
			err = GetDirStatus(sProjectRoot,lpszSubPath,&m_status.text_status , isfull, false,!noignore, NULL, NULL);
			if (m_status.text_status == git_wc_status_added || m_status.text_status == git_wc_status_deleted) // fix for issue #1769; a folder is either modified, conflicted or normal
				m_status.text_status = git_wc_status_modified;
		}
		else
		{
			err = GetFileStatus(sProjectRoot, lpszSubPath, &m_status.text_status ,isfull, false,!noignore, NULL,NULL, &m_status.assumeValid, &m_status.skipWorktree);
		}
	}

	// Error present if function is not under version control
	if (err)
	{
		status = NULL;
		return;
	}

	status = &m_status;
}

typedef CComCritSecLock<CComCriticalSection> CAutoLocker;

int GitStatus::GetFileStatus(const CString &gitdir, const CString &pathParam, git_wc_status_kind * status,BOOL IsFull, BOOL /*IsRecursive*/,BOOL IsIgnore, FILL_STATUS_CALLBACK callback, void *pData, bool * assumeValid, bool * skipWorktree)
{
	try
	{
		CString path = pathParam;

		path.Replace(_T('\\'),_T('/'));

		CString lowcasepath =path;
		lowcasepath.MakeLower();

		if(status)
		{
			git_wc_status_kind st = git_wc_status_none;
			CGitHash hash;

			g_IndexFileMap.GetFileStatus(gitdir, path, &st, IsFull, false, callback, pData, &hash, true, assumeValid, skipWorktree);

			if( st == git_wc_status_conflicted )
			{
				*status =st;
				if (callback && assumeValid && skipWorktree)
					callback(gitdir + _T("/") + path, st, false, pData, *assumeValid, *skipWorktree);
				return 0;
			}

			if( st == git_wc_status_unversioned )
			{
				if(!IsIgnore)
				{
					*status = git_wc_status_unversioned;
					if (callback && assumeValid && skipWorktree)
						callback(gitdir + _T("/") + path, *status, false, pData, *assumeValid, *skipWorktree);
					return 0;
				}

				if (g_IgnoreList.CheckIgnoreChanged(gitdir, path, false))
				{
					g_IgnoreList.LoadAllIgnoreFile(gitdir, path, false);
				}
				if (g_IgnoreList.IsIgnore(path, gitdir, false))
				{
					st = git_wc_status_ignored;
				}
				*status = st;
				if (callback && assumeValid && skipWorktree)
					callback(gitdir + _T("/") + path, st, false, pData, *assumeValid, *skipWorktree);

				return 0;
			}

			if ((st == git_wc_status_normal || st == git_wc_status_modified) && IsFull)
			{
				{
					g_HeadFileMap.CheckHeadAndUpdate(gitdir);

					SHARED_TREE_PTR treeptr = g_HeadFileMap.SafeGet(gitdir);

					// Check Head Tree Hash;
					{
						//add item

						int start = SearchInSortVector(*treeptr, lowcasepath, -1);

						if(start<0)
						{
							*status =st=git_wc_status_added;
							CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) _T(": File miss in head tree %s"), path);
							if (callback && assumeValid && skipWorktree)
								callback(gitdir + _T("/") + path, st, false, pData, *assumeValid, *skipWorktree);
							return 0;
						}

						//staged and not commit
						if( treeptr->at(start).m_Hash != hash )
						{
							*status =st=git_wc_status_modified;
							if (callback && assumeValid && skipWorktree)
								callback(gitdir + _T("/") + path, st, false, pData, *assumeValid, *skipWorktree);
							return 0;
						}
					}
				}
			}
			*status =st;
			if (callback && assumeValid && skipWorktree)
				callback(gitdir + _T("/") + path, st, false, pData, *assumeValid, *skipWorktree);
			return 0;
		}
	}
	catch(...)
	{
		if(status)
			*status = git_wc_status_none;
		return -1;
	}

	return 0;

}

bool GitStatus::HasIgnoreFilesChanged(const CString &gitdir, const CString &subpaths, bool isDir)
{
	return g_IgnoreList.CheckIgnoreChanged(gitdir, subpaths, isDir);
}

int GitStatus::LoadIgnoreFile(const CString &gitdir, const CString &subpaths, bool isDir)
{
	return g_IgnoreList.LoadAllIgnoreFile(gitdir, subpaths, isDir);
}
int GitStatus::IsUnderVersionControl(const CString &gitdir, const CString &path, bool isDir,bool *isVersion)
{
	if (g_IndexFileMap.IsUnderVersionControl(gitdir, path, isDir, isVersion))
		return 1;
	if (!*isVersion)
		return g_HeadFileMap.IsUnderVersionControl(gitdir, path, isDir, isVersion);
	return 0;
}

__int64 GitStatus::GetIndexFileTime(const CString &gitdir)
{
	SHARED_INDEX_PTR ptr=g_IndexFileMap.SafeGet(gitdir);
	if(ptr.get() == NULL)
		return 0;

	return ptr->m_LastModifyTime;
}

int GitStatus::IsIgnore(const CString &gitdir, const CString &path, bool *isIgnore, bool isDir)
{
	if (g_IgnoreList.CheckIgnoreChanged(gitdir, path, isDir))
		g_IgnoreList.LoadAllIgnoreFile(gitdir, path, isDir);

	*isIgnore = g_IgnoreList.IsIgnore(path, gitdir, isDir);

	return 0;
}

static bool SortFileName(CGitFileName &Item1, CGitFileName &Item2)
{
	return Item1.m_FileName.Compare(Item2.m_FileName)<0;
}

int GitStatus::GetFileList(const CString &gitdir, const CString &subpath, std::vector<CGitFileName> &list)
{
	WIN32_FIND_DATA data;
	HANDLE handle=::FindFirstFile(gitdir+_T("\\")+subpath+_T("\\*.*"), &data);
	do
	{
		if(_tcscmp(data.cFileName, _T(".git")) == 0)
			continue;

		if(_tcscmp(data.cFileName, _T(".")) == 0)
			continue;

		if(_tcscmp(data.cFileName, _T("..")) == 0)
			continue;

		CGitFileName filename;

		filename.m_CaseFileName = filename.m_FileName = data.cFileName;
		filename.m_FileName.MakeLower();

		if(data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			filename.m_FileName += _T('/');
		}

		list.push_back(filename);

	}while(::FindNextFile(handle, &data));

	FindClose(handle);

	std::sort(list.begin(), list.end(), SortFileName);
	return 0;
}

int GitStatus::EnumDirStatus(const CString &gitdir, const CString &subpath, git_wc_status_kind * status,BOOL IsFul, BOOL IsRecursive, BOOL IsIgnore, FILL_STATUS_CALLBACK callback, void *pData)
{
	try
	{
		CString path =subpath;

		path.Replace(_T('\\'),_T('/'));
		if(!path.IsEmpty())
			if(path[path.GetLength()-1] !=  _T('/'))
				path += _T('/'); //Add trail / to show it is directory, not file name.

		CString lowcasepath = path;
		lowcasepath.MakeLower();

		std::vector<CGitFileName> filelist;
		GetFileList(gitdir, subpath, filelist);

		if(status)
		{
			g_IndexFileMap.CheckAndUpdate(gitdir,true);

			g_HeadFileMap.CheckHeadAndUpdate(gitdir);

			SHARED_INDEX_PTR indexptr = g_IndexFileMap.SafeGet(gitdir);
			SHARED_TREE_PTR treeptr = g_HeadFileMap.SafeGet(gitdir);

			std::vector<CGitFileName>::iterator it;

			// new git working tree has no index file
			if (indexptr.get() == NULL)
			{
				for (it = filelist.begin(); it < filelist.end(); ++it)
				{
					CString casepath = path + it->m_CaseFileName;

					bool bIsDir = false;
					if (it->m_FileName.GetLength() > 0 && it->m_FileName[it->m_FileName.GetLength() - 1] == _T('/'))
						bIsDir = true;

					if (IsIgnore)
					{
						if (g_IgnoreList.CheckIgnoreChanged(gitdir, casepath, bIsDir))
							g_IgnoreList.LoadAllIgnoreFile(gitdir, casepath, bIsDir);

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

					if(callback)
						callback(gitdir + _T("/") + casepath, *status, bIsDir, pData, false, false);
				}
				return 0;
			}

			CString onepath;
			CString casepath;
			for (it = filelist.begin(); it < filelist.end(); ++it)
			{
				casepath=onepath = path;
				onepath.MakeLower();
				onepath += it->m_FileName;
				casepath += it->m_CaseFileName;

				bool bIsDir = false;
				if (!onepath.IsEmpty() && onepath[onepath.GetLength() - 1] == _T('/'))
					bIsDir = true;

				int matchLength = -1;
				if (bIsDir)
					matchLength = onepath.GetLength();
				int pos = SearchInSortVector(*indexptr, onepath, matchLength);
				int posintree = SearchInSortVector(*treeptr, onepath, matchLength);

				if(pos <0 && posintree<0)
				{
					if (onepath.IsEmpty())
						continue;

					if(!IsIgnore)
					{
						*status = git_wc_status_unversioned;
						if(callback)
							callback(gitdir + _T("/") + casepath, *status, bIsDir, pData, false, false);
						continue;
					}

					if (g_IgnoreList.CheckIgnoreChanged(gitdir, casepath, bIsDir))
						g_IgnoreList.LoadAllIgnoreFile(gitdir, casepath, bIsDir);

					if (g_IgnoreList.IsIgnore(casepath, gitdir, bIsDir))
						*status = git_wc_status_ignored;
					else
						*status = git_wc_status_unversioned;

					if(callback)
						callback(gitdir + _T("/") + casepath, *status, bIsDir, pData, false, false);

				}
				else if(pos <0 && posintree>=0) /* check if file delete in index */
				{
					*status = git_wc_status_deleted;
					if(callback)
						callback(gitdir + _T("/") + casepath, *status, bIsDir, pData, false, false);

				}
				else if(pos >=0 && posintree <0) /* Check if file added */
				{
					*status = git_wc_status_added;
					if(callback)
						callback(gitdir + _T("/") + casepath, *status, bIsDir, pData, false, false);
				}
				else
				{
					if (onepath.IsEmpty())
						continue;

					if (bIsDir)
					{
						*status = git_wc_status_normal;
						if(callback)
							callback(gitdir + _T("/") + casepath, *status, bIsDir, pData, false, false);
					}
					else
					{
						bool assumeValid = false;
						bool skipWorktree = false;
						git_wc_status_kind filestatus;
						GetFileStatus(gitdir, casepath, &filestatus, IsFul, IsRecursive, IsIgnore, callback, pData, &assumeValid, &skipWorktree);
					}
				}

			}/*End of For*/

			/* Check deleted file in system */
			int start=0, end=0;
			int pos = SearchInSortVector(*indexptr, lowcasepath, lowcasepath.GetLength()); // match path prefix, (sub)folders end with slash
			std::map<CString, bool> skipWorktreeMap;

			if (GetRangeInSortVector(*indexptr, lowcasepath, lowcasepath.GetLength(), &start, &end, pos) == 0)
			{
				CGitIndexList::iterator it;
				CString oldstring;

				for (it = indexptr->begin() + start; it <= indexptr->begin() + end; ++it)
				{
					int commonPrefixLength = lowcasepath.GetLength();
					int index = (*it).m_FileName.Find(_T('/'), commonPrefixLength);
					if(index<0)
						index = (*it).m_FileName.GetLength();
					else
						++index; // include slash at the end for subfolders, so that we do not match files by mistake

					CString filename = (*it).m_FileName.Mid(commonPrefixLength, index - commonPrefixLength);
					if(oldstring != filename)
					{
						oldstring = filename;
						if (SearchInSortVector(filelist, filename, filename.GetLength()) < 0)
						{
							bool skipWorktree = false;
							*status = git_wc_status_deleted;
							if (((*it).m_Flags & GIT_IDXENTRY_SKIP_WORKTREE) != 0)
							{
								skipWorktreeMap[filename] = true;
								skipWorktree = true;
								*status = git_wc_status_normal;
							}
							if(callback)
								callback(gitdir + _T("/") + (*it).m_FileName, *status, false, pData, false, skipWorktree);
						}
					}
				}
			}

			start = end =0;
			pos = SearchInSortVector(*treeptr, lowcasepath, lowcasepath.GetLength()); // match path prefix, (sub)folders end with slash
			if (GetRangeInSortVector(*treeptr, lowcasepath, lowcasepath.GetLength(), &start, &end, pos) == 0)
			{
				CGitHeadFileList::iterator it;
				CString oldstring;

				for (it = treeptr->begin() + start; it <= treeptr->begin() + end; ++it)
				{
					int commonPrefixLength = lowcasepath.GetLength();
					int index = (*it).m_FileName.Find(_T('/'), commonPrefixLength);
					if(index<0)
						index = (*it).m_FileName.GetLength();
					else
						++index; // include slash at the end for subfolders, so that we do not match files by mistake

					CString filename = (*it).m_FileName.Mid(commonPrefixLength, index - commonPrefixLength);
					if (oldstring != filename && skipWorktreeMap[filename] != true)
					{
						oldstring = filename;
						if (SearchInSortVector(filelist, filename, filename.GetLength()) < 0)
						{
							*status = git_wc_status_deleted;
							if(callback)
								callback(gitdir + _T("/") + (*it).m_FileName, *status, false, pData, false, false);
						}
					}
				}
			}

		}/*End of if status*/
	}catch(...)
	{
		return -1;
	}
	return 0;

}
int GitStatus::GetDirStatus(const CString &gitdir, const CString &subpath, git_wc_status_kind * status, BOOL IsFul, BOOL IsRecursive, BOOL IsIgnore, FILL_STATUS_CALLBACK callback, void *pData)
{
	try
	{
		CString path =subpath;

		path.Replace(_T('\\'),_T('/'));
		if(!path.IsEmpty())
			if(path[path.GetLength()-1] !=  _T('/'))
				path += _T('/'); //Add trail / to show it is directory, not file name.

		CString lowcasepath = path;
		lowcasepath.MakeLower();

		if(status)
		{
			g_IndexFileMap.CheckAndUpdate(gitdir, true);

			SHARED_INDEX_PTR indexptr = g_IndexFileMap.SafeGet(gitdir);

			if (indexptr == NULL)
			{
				*status = git_wc_status_unversioned;
				return 0;
			}

			int pos = SearchInSortVector(*indexptr, lowcasepath, lowcasepath.GetLength());

			//Not In Version Contorl
			if(pos<0)
			{
				if(!IsIgnore)
				{
					*status = git_wc_status_unversioned;
					if(callback)
						callback(gitdir + _T("/") + path, *status, false, pData, false, false);
					return 0;
				}
				//Check ignore always.
				{
					if (g_IgnoreList.CheckIgnoreChanged(gitdir, path, true))
						g_IgnoreList.LoadAllIgnoreFile(gitdir, path, true);

					if (g_IgnoreList.IsIgnore(path, gitdir, true))
						*status = git_wc_status_ignored;
					else
						*status = git_wc_status_unversioned;

					g_HeadFileMap.CheckHeadAndUpdate(gitdir, false);

					SHARED_TREE_PTR treeptr = g_HeadFileMap.SafeGet(gitdir);
					//Check init repository
					if (treeptr->HeadIsEmpty() && path.IsEmpty())
						*status = git_wc_status_normal;
				}

			}
			else  // In version control
			{
				*status = git_wc_status_normal;

				int start=0;
				int end=0;
				if(path.IsEmpty())
				{
					start=0;
					end = (int)indexptr->size() - 1;
				}
				GetRangeInSortVector(*indexptr, lowcasepath, lowcasepath.GetLength(), &start, &end, pos);
				CGitIndexList::iterator it;

				it = indexptr->begin()+start;

				// Check Conflict;
				for (int i = start; i <= end; ++i)
				{
					if (((*it).m_Flags & GIT_IDXENTRY_STAGEMASK) !=0)
					{
						*status = git_wc_status_conflicted;
						if(callback)
						{
							int dirpos = (*it).m_FileName.Find(_T('/'), path.GetLength());
							if(dirpos<0 || IsRecursive)
								callback(gitdir + _T("\\") + it->m_FileName, git_wc_status_conflicted, false, pData, false, false);
						}
						else
							break;
					}
					++it;
				}

				if( IsFul && (*status != git_wc_status_conflicted))
				{
					*status = git_wc_status_normal;

					g_HeadFileMap.CheckHeadAndUpdate(gitdir);

					//Check Add
					it = indexptr->begin()+start;


					{
						//Check if new init repository
						SHARED_TREE_PTR treeptr = g_HeadFileMap.SafeGet(gitdir);

						if (!treeptr->empty() || treeptr->HeadIsEmpty())
						{
							for (int i = start; i<= end; ++i)
							{
								pos = SearchInSortVector(*treeptr, (*it).m_FileName, -1);

								if(pos < 0)
								{
									*status = max(git_wc_status_added, *status); // added file found
									if(callback)
									{
										int dirpos = (*it).m_FileName.Find(_T('/'), path.GetLength());
										if(dirpos<0 || IsRecursive)
											callback(gitdir + _T("\\") + it->m_FileName, git_wc_status_added, false, pData, false, false);

									}
									else
										break;
								}

								if( pos>=0 && treeptr->at(pos).m_Hash != (*it).m_IndexHash)
								{
									*status = max(git_wc_status_modified, *status); // modified file found
									if(callback)
									{
										int dirpos = (*it).m_FileName.Find(_T('/'), path.GetLength());
										if(dirpos<0 || IsRecursive)
											callback(gitdir + _T("\\") + it->m_FileName, git_wc_status_modified, false, pData, ((*it).m_Flags & GIT_IDXENTRY_VALID) && !((*it).m_Flags & GIT_IDXENTRY_SKIP_WORKTREE), ((*it).m_Flags & GIT_IDXENTRY_SKIP_WORKTREE) != 0);

									}
									else
										break;
								}

								++it;
							}

							//Check Delete
							if( *status == git_wc_status_normal )
							{
								pos = SearchInSortVector(*treeptr, lowcasepath, lowcasepath.GetLength());
								if(pos <0)
								{
									*status = max(git_wc_status_added, *status); // added file found

								}
								else
								{
									int hstart,hend;
									GetRangeInSortVector(*treeptr, lowcasepath, lowcasepath.GetLength(), &hstart, &hend, pos);
									CGitHeadFileList::iterator hit;
									hit = treeptr->begin() + hstart;
									CGitHeadFileList::iterator lastElement = treeptr->end();
									for (int i = hstart; i <= hend && hit != lastElement; ++i)
									{
										if (SearchInSortVector(*indexptr, (*hit).m_FileName, -1) < 0)
										{
											*status = max(git_wc_status_deleted, *status); // deleted file found
											break;
										}
										++hit;
									}
								}
							}
						}
					}/* End lock*/
				}
				// If define callback, it need update each file status.
				// If not define callback, status == git_wc_status_conflicted, needn't check each file status
				// because git_wc_status_conflicted is highest.s
				if(callback || (*status != git_wc_status_conflicted))
				{
					//Check File Time;
					//if(IsRecursive)
					{
						CString sub, currentPath;
						it = indexptr->begin()+start;
						for (int i = start; i <= end; ++i, ++it)
						{
							if( !IsRecursive )
							{
								//skip child directory
								int pos = (*it).m_FileName.Find(_T('/'), path.GetLength());

								if( pos > 0)
								{
									currentPath = (*it).m_FileName.Left(pos);
									if( callback && (sub != currentPath) )
									{
										sub = currentPath;
										CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) _T(": index subdir %s\n"),sub);
										if(callback) callback(gitdir + _T("\\") + sub,
											git_wc_status_normal, true, pData, false, false);
									}
									continue;
								}
							}

							git_wc_status_kind filestatus = git_wc_status_none;
							bool assumeValid = false;
							bool skipWorktree = false;
							GetFileStatus(gitdir, (*it).m_FileName, &filestatus, IsFul, IsRecursive, IsIgnore, callback, pData, &assumeValid, &skipWorktree);
						}
					}
				}
			}

			if(callback) callback(gitdir + _T("/") + subpath, *status, true, pData, false, false);
		}

	}catch(...)
	{
		if(status)
			*status = git_wc_status_none;
		return -1;
	}

	return 0;
}

bool GitStatus::IsExistIndexLockFile(const CString &gitdir)
{
	CString sDirName= gitdir;

	if (!PathIsDirectory(sDirName))
	{
		int x = sDirName.ReverseFind(_T('\\'));
		if (x < 2)
			return false;

		sDirName = sDirName.Left(x);
	}

	for (;;)
	{
		if(PathFileExists(sDirName + _T("\\.git")))
		{
			if(PathFileExists(g_AdminDirMap.GetAdminDir(sDirName) + _T("index.lock")))
				return true;
			else
				return false;
		}

		int x = sDirName.ReverseFind(_T('\\'));
		if (x < 2)
			return false;

		sDirName = sDirName.Left(x);
	}
}

bool GitStatus::ReleasePath(const CString &gitdir)
{
	g_IndexFileMap.SafeClear(gitdir);
	return true;
}

bool GitStatus::ReleasePathsRecursively(const CString &rootpath)
{
	g_IndexFileMap.SafeClearRecursively(rootpath);
	return true;
}

