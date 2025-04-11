﻿// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2017-2025 - TortoiseGit
// Copyright (C) 2003-2016 - TortoiseSVN

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

#include "stdafx.h"
#include "GitWCRev.h"
#include "status.h"
#include "registry.h"
#include "StringUtils.h"
#include "UnicodeUtils.h"
#include <fstream>

void LoadIgnorePatterns(const char* wc, GitWCRev_t* GitStat)
{
	std::string path = wc;
	std::string ignorepath = path + "/.GitWCRevignore";

	std::ifstream infile;
	infile.open(ignorepath);
	if (!infile.good())
		return;

	std::string line;
	while (std::getline(infile, line))
	{
		if (line.empty())
			continue;

		line.insert(line.begin(), '!');
		GitStat->ignorepatterns.emplace(line);
	}
}

static std::wstring GetHomePath()
{
	// also see Git.cpp!
	wchar_t* tmp;
	if ((tmp = _wgetenv(L"HOME")) != nullptr && *tmp)
		return tmp;

	if ((tmp = _wgetenv(L"HOMEDRIVE")) != nullptr)
	{
		std::wstring home(tmp);
		if ((tmp = _wgetenv(L"HOMEPATH")) != nullptr)
		{
			home.append(tmp);
			if (PathIsDirectory(home.c_str()))
				return home;
		}
	}

	if ((tmp = _wgetenv(L"USERPROFILE")) != nullptr && *tmp)
		return tmp;

	return {};
}

static int is_cygwin_msys2_hack_active()
{
	HKEY hKey;
	DWORD dwType = REG_DWORD;
	DWORD dwValue = 0;
	DWORD dwSize = sizeof(dwValue);
	if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\TortoiseGit", 0, KEY_ALL_ACCESS, &hKey) == ERROR_SUCCESS)
	{
		RegQueryValueExW(hKey, L"CygwinHack", nullptr, &dwType, reinterpret_cast<LPBYTE>(&dwValue), &dwSize);
		if (dwValue != 1)
			RegQueryValueExW(hKey, L"Msys2Hack", nullptr, &dwType, reinterpret_cast<LPBYTE>(&dwValue), &dwSize);
		RegCloseKey(hKey);
	}
	return dwValue == 1;
}

static int is_new_git_with_appdata()
{
	HKEY hKey;
	DWORD dwValue = 0;
	if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\TortoiseGit", 0, KEY_ALL_ACCESS, &hKey) == ERROR_SUCCESS)
	{
		DWORD dwType = REG_DWORD;
		DWORD dwSize = sizeof(dwValue);
		RegQueryValueExW(hKey, L"git_cached_version", NULL, &dwType, (LPBYTE)&dwValue, &dwSize);
		RegCloseKey(hKey);
	}
	return dwValue >= (2 << 24 | 46 << 16);
}

static std::wstring GetXDGConfigPath()
{
	// also see Git.cpp!
	wchar_t* tmp;
	if ((tmp = _wgetenv(L"XDG_CONFIG_HOME")) != nullptr && *tmp)
		return std::wstring(tmp) + L"\\git";

	if ((tmp = _wgetenv(L"APPDATA")) != nullptr && *tmp && !is_cygwin_msys2_hack_active() && is_new_git_with_appdata())
	{
		std::wstring xdgPath = std::wstring(tmp) + L"\\Git";
		if (PathFileExists((xdgPath + L"\\config").c_str()))
			return xdgPath;
	}

	if (std::wstring home = GetHomePath(); !home.empty())
		return home + L"\\.config\\git";

	return {};
}

static std::wstring GetSystemGitConfig()
{
	HKEY hKey;
	DWORD dwType = REG_SZ;
	wchar_t path[MAX_PATH] = { 0 };
	DWORD dwSize = _countof(path) - 1;
	if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\TortoiseGit", 0, KEY_ALL_ACCESS, &hKey) == ERROR_SUCCESS)
	{
		RegQueryValueExW(hKey, L"SystemConfig", nullptr, &dwType, reinterpret_cast<LPBYTE>(&path), &dwSize);
		RegCloseKey(hKey);
		std::wstring readPath{ path };
		if (readPath.size() > wcslen(L"\\gitconfig"))
			return readPath.substr(0, readPath.size() - wcslen(L"\\gitconfig"));
	}
	return path;
}

static int RepoStatus(const wchar_t* path, std::string pathA, git_repository* repo, GitWCRev_t& GitStat)
{
	git_status_options git_status_options = GIT_STATUS_OPTIONS_INIT;
	git_status_options.flags = GIT_STATUS_OPT_INCLUDE_UNTRACKED | GIT_STATUS_OPT_RECURSE_UNTRACKED_DIRS;
	if (GitStat.bNoSubmodules)
		git_status_options.flags |= GIT_STATUS_OPT_EXCLUDE_SUBMODULES;

	std::string workdir(git_repository_workdir(repo));
	std::transform(pathA.begin(), pathA.end(), pathA.begin(), [](char c) { return (c == '\\') ? '/' : c; });
	pathA.erase(pathA.begin(), pathA.begin() + min(workdir.length(), pathA.length())); // workdir always ends with a slash, however, wcA is not guaranteed to
	LoadIgnorePatterns(workdir.c_str(), &GitStat);

	std::vector<const char*> pathspec;
	if (!pathA.empty())
	{
		pathspec.emplace_back(pathA.c_str());
		git_status_options.pathspec.count = 1;
	}
	if (!GitStat.ignorepatterns.empty())
	{
		std::transform(GitStat.ignorepatterns.cbegin(), GitStat.ignorepatterns.cend(), std::back_inserter(pathspec), [](auto& pattern) { return pattern.c_str(); });
		git_status_options.pathspec.count += GitStat.ignorepatterns.size();
		if (pathA.empty())
		{
			pathspec.push_back("*");
			git_status_options.pathspec.count += 1;
		}
	}
	if (git_status_options.pathspec.count > 0)
		git_status_options.pathspec.strings = const_cast<char**>(pathspec.data());

	CAutoStatusList status;
	if (git_status_list_new(status.GetPointer(), repo, &git_status_options) < 0)
		return ERR_GIT_ERR;

	for (size_t i = 0, maxi = git_status_list_entrycount(status); i < maxi; ++i)
	{
		const git_status_entry* s = git_status_byindex(status, i);
		if (s->index_to_workdir && s->index_to_workdir->new_file.mode == GIT_FILEMODE_COMMIT)
		{
			GitStat.bHasSubmodule = TRUE;
			unsigned int smstatus = 0;
			if (!git_submodule_status(&smstatus, repo, s->index_to_workdir->new_file.path, GIT_SUBMODULE_IGNORE_UNSPECIFIED))
			{
				if (smstatus & GIT_SUBMODULE_STATUS_WD_MODIFIED) // HEAD of submodule not matching
					GitStat.bHasSubmoduleNewCommits = TRUE;
				else if ((smstatus & GIT_SUBMODULE_STATUS_WD_INDEX_MODIFIED) || (smstatus & GIT_SUBMODULE_STATUS_WD_WD_MODIFIED))
					GitStat.bHasSubmoduleMods = TRUE;
				else if (smstatus & GIT_SUBMODULE_STATUS_WD_UNTRACKED)
					GitStat.bHasSubmoduleUnversioned = TRUE;
			}
			continue;
		}
		if (s->status == GIT_STATUS_CURRENT)
			continue;
		if (s->status == GIT_STATUS_WT_NEW || s->status == GIT_STATUS_INDEX_NEW)
			GitStat.HasUnversioned = TRUE;
		else
			GitStat.HasMods = TRUE;
	}

	if (pathA.empty()) // working tree root is always versioned
	{
		GitStat.bIsGitItem = TRUE;
		return 0;
	}
	else if (PathIsDirectory(path)) // directories are unversioned in Git
	{
		GitStat.bIsGitItem = FALSE;
		return 0;
	}
	unsigned int status_flags = 0;
	int ret = git_status_file(&status_flags, repo, pathA.c_str());
	GitStat.bIsGitItem = (ret == GIT_OK && !(status_flags & (GIT_STATUS_WT_NEW | GIT_STATUS_IGNORED | GIT_STATUS_INDEX_NEW)));
	return 0;
}

int GetStatusUnCleanPath(const wchar_t* wcPath, GitWCRev_t& GitStat)
{
	DWORD reqLen = GetFullPathName(wcPath, 0, nullptr, nullptr);
	auto wcfullPath = std::make_unique<wchar_t[]>(reqLen + 1);
	GetFullPathName(wcPath, reqLen, wcfullPath.get(), nullptr);
	// GetFullPathName() sometimes returns the full path with the wrong
	// case. This is not a problem on Windows since its filesystem is
	// case-insensitive. But for Git that's a problem if the wrong case
	// is inside a working copy: the git index is case sensitive.
	// To fix the casing of the path, we use a trick:
	// convert the path to its short form, then back to its long form.
	// That will fix the wrong casing of the path.
	int shortlen = GetShortPathName(wcfullPath.get(), nullptr, 0);
	if (shortlen)
	{
		auto shortPath = std::make_unique<wchar_t[]>(shortlen + 1);
		if (GetShortPathName(wcfullPath.get(), shortPath.get(), shortlen + 1))
		{
			reqLen = GetLongPathName(shortPath.get(), nullptr, 0);
			wcfullPath = std::make_unique<wchar_t[]>(reqLen + 1);
			GetLongPathName(shortPath.get(), wcfullPath.get(), reqLen);
		}
	}
	return GetStatus(wcfullPath.get(), GitStat);
}

int GetStatus(const wchar_t* path, GitWCRev_t& GitStat)
{
	// Configure libgit2 search paths
	std::wstring systemConfig = GetSystemGitConfig();
	git_libgit2_opts(GIT_OPT_SET_SEARCH_PATH, GIT_CONFIG_LEVEL_SYSTEM, CUnicodeUtils::StdGetUTF8(systemConfig).c_str());
	git_libgit2_opts(GIT_OPT_SET_SEARCH_PATH, GIT_CONFIG_LEVEL_GLOBAL, CUnicodeUtils::StdGetUTF8(GetHomePath()).c_str());
	git_libgit2_opts(GIT_OPT_SET_SEARCH_PATH, GIT_CONFIG_LEVEL_XDG, CUnicodeUtils::StdGetUTF8(GetXDGConfigPath()).c_str());
	git_libgit2_opts(GIT_OPT_SET_SEARCH_PATH, GIT_CONFIG_LEVEL_PROGRAMDATA, L"");

	std::string pathA = CUnicodeUtils::StdGetUTF8(path);
	CAutoBuf dotgitdir;
	if (git_repository_discover(dotgitdir, pathA.c_str(), 0, nullptr) < 0)
		return ERR_NOWC;

	CAutoRepository repo;
	if (git_repository_open(repo.GetPointer(), dotgitdir->ptr))
		return ERR_NOWC;

	if (git_repository_is_bare(repo))
		return ERR_NOWC;

	if (int ret = git_repository_head_unborn(repo); ret < 0)
		return ERR_GIT_ERR;
	else if (ret == 1)
	{
		git_oid emptyOid = { static_cast<unsigned char>(git_repository_oid_type(repo)) };
		git_oid_tostr(GitStat.HeadHashReadable, sizeof(GitStat.HeadHashReadable), &emptyOid);
		GitStat.bIsUnborn = TRUE;

		CAutoReference symbolicHead;
		if (git_reference_lookup(symbolicHead.GetPointer(), repo, "HEAD"))
			return ERR_GIT_ERR;
		auto branchName = git_reference_symbolic_target(symbolicHead);
		if (CStringUtils::StartsWith(branchName, "refs/heads/"))
			branchName += strlen("refs/heads/");
		else if (CStringUtils::StartsWith(branchName, "refs/"))
			branchName += strlen("refs/");
		GitStat.CurrentBranch = branchName;

		return RepoStatus(path, pathA, repo, GitStat);
	}

	CAutoReference head;
	if (git_repository_head(head.GetPointer(), repo) < 0)
		return ERR_GIT_ERR;
	GitStat.CurrentBranch = git_reference_shorthand(head);

	CAutoObject object;
	if (git_reference_peel(object.GetPointer(), head, GIT_OBJECT_COMMIT) < 0)
		return ERR_GIT_ERR;

	const git_oid* oid = git_object_id(object);
	git_oid_tostr(GitStat.HeadHashReadable, sizeof(GitStat.HeadHashReadable), oid);

	CAutoCommit commit;
	if (git_commit_lookup(commit.GetPointer(), repo, oid) < 0)
		return ERR_GIT_ERR;

	const git_signature* sig = git_commit_author(commit);
	GitStat.HeadTime = sig->when.time;
	if (CRegStdDWORD(L"Software\\TortoiseGit\\UseMailmap", TRUE) == TRUE)
	{
		CAutoMailmap mailmap;
		if (git_mailmap_from_repository(mailmap.GetPointer(), repo))
			return ERR_GIT_ERR;
		CAutoSignature resolvedSignature;
		if (git_mailmap_resolve_signature(resolvedSignature.GetPointer(), mailmap, sig))
			return ERR_GIT_ERR;
		GitStat.HeadAuthor = (*resolvedSignature).name;
		GitStat.HeadEmail = (*resolvedSignature).email;
	}
	else
	{
		GitStat.HeadAuthor = sig->name;
		GitStat.HeadEmail = sig->email;
	}

	struct TagPayload { git_repository* repo; GitWCRev_t& GitStat; const git_oid* headOid; } tagpayload = { repo, GitStat, oid };

	if (git_tag_foreach(repo, [](const char*, git_oid* tagoid, void* payload)
	{
		auto pl = reinterpret_cast<struct TagPayload*>(payload);
		if (git_oid_cmp(tagoid, pl->headOid) == 0)
		{
			pl->GitStat.bIsTagged = TRUE;
			return 0;
		}

		CAutoTag tag;
		if (git_tag_lookup(tag.GetPointer(), pl->repo, tagoid))
			return 0; // not an annotated tag
		CAutoObject tagObject;
		if (git_tag_peel(tagObject.GetPointer(), tag))
			return -1;
		if (git_oid_cmp(git_object_id(tagObject), pl->headOid) == 0)
			pl->GitStat.bIsTagged = TRUE;

		return 0;
	}, &tagpayload))
		return ERR_GIT_ERR;

	// count the first-parent revisions from HEAD to the first commit
	CAutoRevwalk walker;
	if (git_revwalk_new(walker.GetPointer(), repo) < 0)
		return ERR_GIT_ERR;
	git_revwalk_simplify_first_parent(walker);
	if (git_revwalk_push_head(walker) < 0)
		return ERR_GIT_ERR;
	git_oid oidlog;
	while (!git_revwalk_next(&oidlog, walker))
		++GitStat.NumCommits;

	return RepoStatus(path, pathA, repo, GitStat);
}
