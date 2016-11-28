// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2016 - TortoiseGit

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

#pragma once
#include "StringUtils.h"
#include "BrowseFolder.h"
#include "PathUtils.h"
#include "Git.h"
#include "MessageBox.h"
#include "AppUtils.h"

#define GIT_FOR_WINDOWS_URL L"https://git-for-windows.github.io/"

class CConfigureGitExe
{
public:
	CConfigureGitExe()
		: m_regCygwinHack(L"Software\\TortoiseGit\\CygwinHack", FALSE)
		, m_regMsys2Hack(L"Software\\TortoiseGit\\Msys2Hack", FALSE)
	{};

protected:
	CRegDWORD		m_regCygwinHack;
	CRegDWORD		m_regMsys2Hack;
	CRegString		m_regMsysGitPath;
	CRegString		m_regMsysGitExtranPath;

	static void PerformCommonGitPathCleanup(CString& path)
	{
		path.Trim(L"\"'");

		if (path.Find(L'%') >= 0)
		{
			int neededSize = ExpandEnvironmentStrings(path, nullptr, 0);
			CString origPath(path);
			ExpandEnvironmentStrings(origPath, path.GetBufferSetLength(neededSize), neededSize);
			path.ReleaseBuffer();
		}

		path.Replace(L'/', L'\\');
		path.Replace(L"\\\\", L"\\");

		if (CStringUtils::EndsWith(path, L"git.exe"))
			path.Truncate(path.GetLength() - 7);

		path.TrimRight(L'\\');

		// prefer git.exe in cmd-directory for Git for Windows based on msys2
		if (path.GetLength() > 12 && (CStringUtils::EndsWith(path, L"\\mingw32\\bin") || CStringUtils::EndsWith(path, L"\\mingw64\\bin")) && PathFileExists(path.Left(path.GetLength() - 12) + L"\\cmd\\git.exe"))
			path = path.Left(path.GetLength() - 12) + L"\\cmd";

		// prefer git.exe in bin-directory, see https://github.com/msysgit/msysgit/issues/103
		if (path.GetLength() > 5 && CStringUtils::EndsWith(path, L"\\cmd") && PathFileExists(path.Left(path.GetLength() - 4) + L"\\bin\\git.exe"))
			path = path.Left(path.GetLength() - 4) + L"\\bin";
	}

	static bool GuessExtraPath(CString gitpath, CString& pathaddition)
	{
		if (gitpath.IsEmpty())
			return false;

		PerformCommonGitPathCleanup(gitpath);

		if (!CStringUtils::EndsWith(gitpath, L"bin"))
			return false;

		gitpath.Truncate(gitpath.GetLength() - 3);
		gitpath += L"mingw\\bin";
		if (!::PathFileExists(gitpath))
			return false;

		if (pathaddition.Find(gitpath + L';') >= 0 || CStringUtils::EndsWith(pathaddition, gitpath))
			return false;

		pathaddition = gitpath + pathaddition;
		return true;
	}

	static bool SelectFolder(HWND hwnd, CString& gitpath, CString& pathaddition)
	{
		CBrowseFolder browseFolder;
		browseFolder.m_style = BIF_EDITBOX | BIF_NEWDIALOGSTYLE | BIF_RETURNFSANCESTORS | BIF_RETURNONLYFSDIRS;
		CString dir;
		dir = gitpath;
		if (dir.IsEmpty())
			dir = CPathUtils::GetProgramsDirectory();
		if (browseFolder.Show(hwnd, dir) != CBrowseFolder::OK)
			return false;

		gitpath = dir;
		PerformCommonGitPathCleanup(gitpath);
		if (!PathFileExists(gitpath + L"\\git.exe"))
			MessageBox(hwnd, L"No git.exe found in the selected folder.", L"TortoiseGit", MB_ICONEXCLAMATION);
		GuessExtraPath(gitpath, pathaddition);
		return true;
	}

	bool CheckGitExe(HWND hwnd, CString& gitpath, CString& pathaddition, int versionLabelId, std::function<void(UINT)> callHelp, bool* needWorkarounds = nullptr)
	{
		SetWindowText(GetDlgItem(hwnd, versionLabelId), L"");

		PerformCommonGitPathCleanup(gitpath);

		GuessExtraPath(gitpath, pathaddition);

		CString oldpath = m_regMsysGitPath;
		CString oldextranpath = m_regMsysGitExtranPath;

		StoreSetting(hwnd, gitpath, m_regMsysGitPath);
		StoreSetting(hwnd, pathaddition, m_regMsysGitExtranPath);
		SCOPE_EXIT{
			StoreSetting(hwnd, oldpath, m_regMsysGitPath);
			StoreSetting(hwnd, oldextranpath, m_regMsysGitExtranPath);
		};

		g_Git.m_bInitialized = false;

		if (g_Git.CheckMsysGitDir(FALSE))
		{
			CString out;
			int ret = g_Git.Run(L"git.exe --version", &out, CP_UTF8);
			SetWindowText(GetDlgItem(hwnd, versionLabelId), out);
			if (out.IsEmpty())
			{
				if (ret == 0xC0000135 && CMessageBox::Show(hwnd, L"Could not start git.exe. A dynamic library (dll) is missing.\nYou might need to specify an extra PATH.\nCheck help file for \"Extra PATH\".", L"TortoiseGit", 1, IDI_ERROR, CString(MAKEINTRESOURCE(IDS_MSGBOX_OK)), CString(MAKEINTRESOURCE(IDS_MSGBOX_HELP))) == 2)
					callHelp(IDD_SETTINGSMAIN);
				else if (CMessageBox::Show(hwnd, L"Could not get read version information from git.exe.\nCheck help file for \"Git.exe Path\".", L"TortoiseGit", 1, IDI_ERROR, CString(MAKEINTRESOURCE(IDS_MSGBOX_OK)), CString(MAKEINTRESOURCE(IDS_MSGBOX_HELP))) == 2)
					callHelp(IDD_SETTINGSMAIN);
				return false;
			}
			else if (!CStringUtils::StartsWith(out.Trim(), L"git version "))
			{
				if (CMessageBox::Show(hwnd, L"Could not get read version information from git.exe.\nGot: \"" + out.Trim() + L"\"\n\nCheck help file for \"Git.exe Path\".", L"TortoiseGit", 1, IDI_ERROR, CString(MAKEINTRESOURCE(IDS_MSGBOX_OK)), CString(MAKEINTRESOURCE(IDS_MSGBOX_HELP))) == 2)
					callHelp(IDD_SETTINGSMAIN);
				return false;
			}
			else if (!(CGit::ms_bCygwinGit || CGit::ms_bMsys2Git) && out.Find(L"msysgit") == -1 && out.Find(L"windows") == -1)
			{
				bool wasAlreadyWarned = !needWorkarounds || *needWorkarounds;
				if (CMessageBox::Show(hwnd, L"Could not find \"msysgit\" or \"windows\" in versionstring of git.exe.\nIf you are using git of the cygwin or msys2 environment please read the help file for the keyword \"cygwin git\" or \"msys2 git\".", L"TortoiseGit", 1, IDI_INFORMATION, CString(MAKEINTRESOURCE(IDS_MSGBOX_OK)), CString(MAKEINTRESOURCE(IDS_MSGBOX_HELP))) == 2)
					callHelp(IDD_SETTINGSMAIN);
				if (needWorkarounds)
					*needWorkarounds = true;
				if (!wasAlreadyWarned)
					return false;
			}
			else if ((CGit::ms_bCygwinGit || CGit::ms_bMsys2Git) && out.Find(L"msysgit") > 0 && out.Find(L"windows") > 0)
			{
				if (CMessageBox::Show(hwnd, L"Found \"msysgit\" or \"windows\" in versionstring of git.exe, however, you have git.exe quirks enabled. These hacks must be disabled for proper operation with Git for Windows!\nYou can find more information in the help file for the keyword \"cygwin git\" or \"msys2 git\".", L"TortoiseGit", 1, IDI_INFORMATION, CString(MAKEINTRESOURCE(IDS_MSGBOX_OK)), CString(MAKEINTRESOURCE(IDS_MSGBOX_HELP))) == 2)
					callHelp(IDD_SETTINGSMAIN);
				if (needWorkarounds)
					*needWorkarounds = true;
				return false;
			}
		}
		else
		{
			if (CMessageBox::Show(hwnd, L"Invalid git.exe path.\nCheck help file for \"Git.exe Path\".", L"TortoiseGit", 1, IDI_ERROR, CString(MAKEINTRESOURCE(IDS_MSGBOX_OK)), CString(MAKEINTRESOURCE(IDS_MSGBOX_HELP))) == 2)
				callHelp(IDD_SETTINGSMAIN);
			return false;
		}

		return true;
	}

	template<class T, class Reg>
	void StoreSetting(HWND hwnd, const T& value, Reg& registryKey)
	{
		registryKey = value;
		if (registryKey.GetLastError() != ERROR_SUCCESS)
			CMessageBox::Show(hwnd, registryKey.getErrorString(), L"TortoiseGit", MB_ICONERROR);
	}
};
