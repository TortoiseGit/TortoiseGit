// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2018 - TortoiseGit
// Copyright (C) 2003-2011, 2013-2014 - TortoiseSVN

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
#include "TortoiseProc.h"
#include "PathUtils.h"
#include "AppUtils.h"
#include "MessageBox.h"
#include "registry.h"
#include "TGitPath.h"
#include "Git.h"
#include "UnicodeUtils.h"
#include "ExportDlg.h"
#include "ProgressDlg.h"
#include "GitAdminDir.h"
#include "ProgressDlg.h"
#include "BrowseFolder.h"
#include "DirFileEnum.h"
#include "CreateBranchTagDlg.h"
#include "GitSwitchDlg.h"
#include "ResetDlg.h"
#include "DeleteConflictDlg.h"
#include "SendMailDlg.h"
#include "GitProgressDlg.h"
#include "PushDlg.h"
#include "CommitDlg.h"
#include "MergeDlg.h"
#include "MergeAbortDlg.h"
#include "Hooks.h"
#include "../Settings/Settings.h"
#include "InputDlg.h"
#include "SVNDCommitDlg.h"
#include "requestpulldlg.h"
#include "PullFetchDlg.h"
#include "RebaseDlg.h"
#include "PropKey.h"
#include "StashSave.h"
#include "IgnoreDlg.h"
#include "FormatMessageWrapper.h"
#include "SmartHandle.h"
#include "BisectStartDlg.h"
#include "SysProgressDlg.h"
#include "UserPassword.h"
#include "SendmailPatch.h"
#include "Globals.h"
#include "ProgressCommands/ResetProgressCommand.h"
#include "ProgressCommands/FetchProgressCommand.h"
#include "ProgressCommands/SendMailProgressCommand.h"
#include "CertificateValidationHelper.h"
#include "CheckCertificateDlg.h"
#include "SubmoduleResolveConflictDlg.h"
#include "GitDiff.h"
#include "../TGitCache/CacheInterface.h"

static struct last_accepted_cert {
	BYTE*		data;
	size_t		len;

	last_accepted_cert()
		: data(nullptr)
		, len(0)
	{
	}
	~last_accepted_cert()
	{
		free(data);
	};
	boolean cmp(git_cert_x509* cert)
	{
		return len > 0 && len == cert->len && memcmp(data, cert->data, len) == 0;
	}
	void set(git_cert_x509* cert)
	{
		free(data);
		len = cert->len;
		if (len == 0)
		{
			data = nullptr;
			return;
		}
		data = new BYTE[len];
		memcpy(data, cert->data, len);
	}
} last_accepted_cert;

static bool DoFetch(const CString& url, const bool fetchAllRemotes, const bool loadPuttyAgent, const int prune, const bool bDepth, const int nDepth, const int fetchTags, const CString& remoteBranch, int runRebase, const bool rebasePreserveMerges);

bool CAppUtils::StashSave(const CString& msg, bool showPull, bool pullShowPush, bool showMerge, const CString& mergeRev)
{
	if (!CheckUserData())
		return false;

	CStashSaveDlg dlg;
	dlg.m_sMessage = msg;
	if (dlg.DoModal() == IDOK)
	{
		CString cmd = L"git.exe stash push";
		if (!CAppUtils::IsGitVersionNewerOrEqual(2, 14))
			cmd = L"git.exe stash save";

		if (dlg.m_bIncludeUntracked)
			cmd += L" --include-untracked";
		else if (dlg.m_bAll)
			cmd += L" --all";

		if (!dlg.m_sMessage.IsEmpty())
		{
			CString message = dlg.m_sMessage;
			message.Replace(L"\"", L"\"\"");
			cmd += L" -- \"" + message + L'"';
		}

		CProgressDlg progress;
		progress.m_GitCmd = cmd;
		progress.m_PostCmdCallback = [&](DWORD status, PostCmdList& postCmdList)
		{
			if (status)
				return;

			if (showPull)
				postCmdList.emplace_back(IDI_PULL, IDS_MENUPULL, [&]{ CAppUtils::Pull(pullShowPush, true); });
			if (showMerge)
				postCmdList.emplace_back(IDI_MERGE, IDS_MENUMERGE, [&]{ CAppUtils::Merge(&mergeRev, true); });
			postCmdList.emplace_back(IDI_RELOCATE, IDS_MENUSTASHPOP, [] { CAppUtils::StashPop(); });
		};
		return (progress.DoModal() == IDOK);
	}
	return false;
}

bool CAppUtils::StashApply(CString ref, bool showChanges /* true */)
{
	CString cmd = L"git.exe stash apply ";
	if (CStringUtils::StartsWith(ref, L"refs/"))
		ref = ref.Mid(5);
	if (CStringUtils::StartsWith(ref, L"stash{"))
		ref = L"stash@" + ref.Mid(5);
	cmd += ref;

	CSysProgressDlg sysProgressDlg;
	sysProgressDlg.SetTitle(CString(MAKEINTRESOURCE(IDS_APPNAME)));
	sysProgressDlg.SetLine(1, CString(MAKEINTRESOURCE(IDS_PROC_STASHRUNNING)));
	sysProgressDlg.SetLine(2, CString(MAKEINTRESOURCE(IDS_PROGRESSWAIT)));
	sysProgressDlg.SetShowProgressBar(false);
	sysProgressDlg.SetCancelMsg(IDS_PROGRS_INFOFAILED);
	sysProgressDlg.ShowModeless((HWND)nullptr, true);

	CString out;
	int ret = g_Git.Run(cmd, &out, CP_UTF8);

	sysProgressDlg.Stop();

	bool hasConflicts = (out.Find(L"CONFLICT") >= 0);
	if (ret && !(ret == 1 && hasConflicts))
		CMessageBox::Show(nullptr, CString(MAKEINTRESOURCE(IDS_PROC_STASHAPPLYFAILED)) + L'\n' + out, L"TortoiseGit", MB_OK | MB_ICONERROR);
	else
	{
		CString message;
		message.LoadString(IDS_PROC_STASHAPPLYSUCCESS);
		if (hasConflicts)
			message.LoadString(IDS_PROC_STASHAPPLYFAILEDCONFLICTS);
		if (showChanges)
		{
			if (CMessageBox::Show(nullptr, message + L'\n' + CString(MAKEINTRESOURCE(IDS_SEECHANGES)), L"TortoiseGit", MB_YESNO | MB_ICONINFORMATION) == IDYES)
			{
				cmd.Format(L"/command:repostatus /path:\"%s\"", (LPCTSTR)g_Git.m_CurrentDir);
				CAppUtils::RunTortoiseGitProc(cmd);
			}
			return true;
		}
		else
		{
			MessageBox(nullptr, message ,L"TortoiseGit", MB_OK | MB_ICONINFORMATION);
			return true;
		}
	}
	return false;
}

bool CAppUtils::StashPop(int showChanges /* = 1 */)
{
	CString cmd = L"git.exe stash pop";

	CSysProgressDlg sysProgressDlg;
	sysProgressDlg.SetTitle(CString(MAKEINTRESOURCE(IDS_APPNAME)));
	sysProgressDlg.SetLine(1, CString(MAKEINTRESOURCE(IDS_PROC_STASHRUNNING)));
	sysProgressDlg.SetLine(2, CString(MAKEINTRESOURCE(IDS_PROGRESSWAIT)));
	sysProgressDlg.SetShowProgressBar(false);
	sysProgressDlg.SetCancelMsg(IDS_PROGRS_INFOFAILED);
	sysProgressDlg.ShowModeless((HWND)nullptr, true);

	CString out;
	int ret = g_Git.Run(cmd, &out, CP_UTF8);

	sysProgressDlg.Stop();

	bool hasConflicts = (out.Find(L"CONFLICT") >= 0);
	if (ret && !(ret == 1 && hasConflicts))
		CMessageBox::Show(nullptr, CString(MAKEINTRESOURCE(IDS_PROC_STASHPOPFAILED)) + L'\n' + out, L"TortoiseGit", MB_OK | MB_ICONERROR);
	else
	{
		CString message;
		message.LoadString(IDS_PROC_STASHPOPSUCCESS);
		if (hasConflicts)
			message.LoadString(IDS_PROC_STASHPOPFAILEDCONFLICTS);
		if (showChanges == 1 || (showChanges == 0 && hasConflicts))
		{
			if (CMessageBox::ShowCheck(nullptr, message + L'\n' + CString(MAKEINTRESOURCE(IDS_SEECHANGES)), L"TortoiseGit", MB_YESNO | (hasConflicts ? MB_ICONEXCLAMATION : MB_ICONINFORMATION), hasConflicts ? L"StashPopShowConflictChanges" : L"StashPopShowChanges") == IDYES)
			{
				cmd.Format(L"/command:repostatus /path:\"%s\"", (LPCTSTR)g_Git.m_CurrentDir);
				CAppUtils::RunTortoiseGitProc(cmd);
			}
			return true;
		}
		else if (showChanges > 1)
		{
			MessageBox(nullptr, message, L"TortoiseGit", MB_OK | MB_ICONINFORMATION);
			return true;
		}
		else if (showChanges == 0)
			return true;
	}
	return false;
}

BOOL CAppUtils::StartExtMerge(bool bAlternative,
	const CTGitPath& basefile, const CTGitPath& theirfile, const CTGitPath& yourfile, const CTGitPath& mergedfile,
	const CString& basename, const CString& theirname, const CString& yourname, const CString& mergedname, bool bReadOnly,
	HWND resolveMsgHwnd, bool bDeleteBaseTheirsMineOnClose)
{
	CRegString regCom = CRegString(L"Software\\TortoiseGit\\Merge");
	CString ext = mergedfile.GetFileExtension();
	CString com = regCom;
	bool bInternal = false;

	if (!ext.IsEmpty())
	{
		// is there an extension specific merge tool?
		CRegString mergetool(L"Software\\TortoiseGit\\MergeTools\\" + ext.MakeLower());
		if (!CString(mergetool).IsEmpty())
			com = mergetool;
	}
	// is there a filename specific merge tool?
	CRegString mergetool(L"Software\\TortoiseGit\\MergeTools\\." + mergedfile.GetFilename().MakeLower());
	if (!CString(mergetool).IsEmpty())
		com = mergetool;

	if (bAlternative && !com.IsEmpty())
	{
		if (CStringUtils::StartsWith(com, L"#"))
			com.Delete(0);
		else
			com.Empty();
	}

	if (com.IsEmpty() || CStringUtils::StartsWith(com, L"#"))
	{
		// Maybe we should use TortoiseIDiff?
		if ((ext == L".jpg") || (ext == L".jpeg") ||
			(ext == L".bmp") || (ext == L".gif")  ||
			(ext == L".png") || (ext == L".ico")  ||
			(ext == L".tif") || (ext == L".tiff") ||
			(ext == L".dib") || (ext == L".emf")  ||
			(ext == L".cur"))
		{
			com = CPathUtils::GetAppDirectory() + L"TortoiseGitIDiff.exe";
			com = L'"' + com + L'"';
			com = com + L" /base:%base /theirs:%theirs /mine:%mine /result:%merged";
			com = com + L" /basetitle:%bname /theirstitle:%tname /minetitle:%yname";
			if (resolveMsgHwnd)
				com.AppendFormat(L" /resolvemsghwnd:%I64d", (__int64)resolveMsgHwnd);
		}
		else
		{
			// use TortoiseGitMerge
			bInternal = true;
			com = CPathUtils::GetAppDirectory() + L"TortoiseGitMerge.exe";
			com = L'"' + com + L'"';
			com = com + L" /base:%base /theirs:%theirs /mine:%mine /merged:%merged";
			com = com + L" /basename:%bname /theirsname:%tname /minename:%yname /mergedname:%mname";
			com += L" /saverequired";
			if (resolveMsgHwnd)
				com.AppendFormat(L" /resolvemsghwnd:%I64d", (__int64)resolveMsgHwnd);
			if (bDeleteBaseTheirsMineOnClose)
				com += L" /deletebasetheirsmineonclose";
		}
		if (!g_sGroupingUUID.IsEmpty())
		{
			com += L" /groupuuid:\"";
			com += g_sGroupingUUID;
			com += L'"';
		}
	}
	// check if the params are set. If not, just add the files to the command line
	if ((com.Find(L"%merged") < 0) && (com.Find(L"%base") < 0) && (com.Find(L"%theirs") < 0) && (com.Find(L"%mine") < 0))
	{
		com += L" \"" + basefile.GetWinPathString() + L'"';
		com += L" \"" + theirfile.GetWinPathString() + L'"';
		com += L" \"" + yourfile.GetWinPathString() + L'"';
		com += L" \"" + mergedfile.GetWinPathString() + L'"';
	}
	if (basefile.IsEmpty())
	{
		com.Replace(L"/base:%base", L"");
		com.Replace(L"%base", L"");
	}
	else
		com.Replace(L"%base", L'"' + basefile.GetWinPathString() + L'"');
	if (theirfile.IsEmpty())
	{
		com.Replace(L"/theirs:%theirs", L"");
		com.Replace(L"%theirs", L"");
	}
	else
		com.Replace(L"%theirs", L'"' + theirfile.GetWinPathString() + L'"');
	if (yourfile.IsEmpty())
	{
		com.Replace(L"/mine:%mine", L"");
		com.Replace(L"%mine", L"");
	}
	else
		com.Replace(L"%mine", L'"' + yourfile.GetWinPathString() + L'"');
	if (mergedfile.IsEmpty())
	{
		com.Replace(L"/merged:%merged", L"");
		com.Replace(L"%merged", L"");
	}
	else
		com.Replace(L"%merged", L'"' + mergedfile.GetWinPathString() + L'"');
	if (basename.IsEmpty())
	{
		if (basefile.IsEmpty())
		{
			com.Replace(L"/basename:%bname", L"");
			com.Replace(L"%bname", L"");
		}
		else
			com.Replace(L"%bname", L'"' + basefile.GetUIFileOrDirectoryName() + L'"');
	}
	else
		com.Replace(L"%bname", L'"' + basename + L'"');
	if (theirname.IsEmpty())
	{
		if (theirfile.IsEmpty())
		{
			com.Replace(L"/theirsname:%tname", L"");
			com.Replace(L"%tname", L"");
		}
		else
			com.Replace(L"%tname", L'"' + theirfile.GetUIFileOrDirectoryName() + L'"');
	}
	else
		com.Replace(L"%tname", L'"' + theirname + L'"');
	if (yourname.IsEmpty())
	{
		if (yourfile.IsEmpty())
		{
			com.Replace(L"/minename:%yname", L"");
			com.Replace(L"%yname", L"");
		}
		else
			com.Replace(L"%yname", L'"' + yourfile.GetUIFileOrDirectoryName() + L'"');
	}
	else
		com.Replace(L"%yname", L'"' + yourname + L'"');
	if (mergedname.IsEmpty())
	{
		if (mergedfile.IsEmpty())
		{
			com.Replace(L"/mergedname:%mname", L"");
			com.Replace(L"%mname", L"");
		}
		else
			com.Replace(L"%mname", L'"' + mergedfile.GetUIFileOrDirectoryName() + L'"');
	}
	else
		com.Replace(L"%mname", L'"' + mergedname + L'"');

	com.Replace(L"%wtroot", L'"' + g_Git.m_CurrentDir + L'"');

	if ((bReadOnly)&&(bInternal))
		com += L" /readonly";

	if(!LaunchApplication(com, IDS_ERR_EXTMERGESTART, false))
	{
		return FALSE;
	}

	return TRUE;
}

BOOL CAppUtils::StartExtPatch(const CTGitPath& patchfile, const CTGitPath& dir, const CString& sOriginalDescription, const CString& sPatchedDescription, BOOL bReversed, BOOL bWait)
{
	CString viewer;
	// use TortoiseGitMerge
	viewer = CPathUtils::GetAppDirectory();
	viewer += L"TortoiseGitMerge.exe";

	viewer = L'"' + viewer + L'"';
	viewer = viewer + L" /diff:\"" + patchfile.GetWinPathString() + L'"';
	viewer = viewer + L" /patchpath:\"" + dir.GetWinPathString() + L'"';
	if (bReversed)
		viewer += L" /reversedpatch";
	if (!sOriginalDescription.IsEmpty())
		viewer = viewer + L" /patchoriginal:\"" + sOriginalDescription + L'"';
	if (!sPatchedDescription.IsEmpty())
		viewer = viewer + L" /patchpatched:\"" + sPatchedDescription + L'"';
	if (!g_sGroupingUUID.IsEmpty())
	{
		viewer += L" /groupuuid:\"";
		viewer += g_sGroupingUUID;
		viewer += L'"';
	}
	if(!LaunchApplication(viewer, IDS_ERR_DIFFVIEWSTART, !!bWait))
		return FALSE;
	return TRUE;
}

CString CAppUtils::PickDiffTool(const CTGitPath& file1, const CTGitPath& file2)
{
	CString difftool = CRegString(L"Software\\TortoiseGit\\DiffTools\\" + file2.GetFilename().MakeLower());
	if (!difftool.IsEmpty())
		return difftool;
	difftool = CRegString(L"Software\\TortoiseGit\\DiffTools\\" + file1.GetFilename().MakeLower());
	if (!difftool.IsEmpty())
		return difftool;

	// Is there an extension specific diff tool?
	CString ext = file2.GetFileExtension().MakeLower();
	if (!ext.IsEmpty())
	{
		difftool = CRegString(L"Software\\TortoiseGit\\DiffTools\\" + ext);
		if (!difftool.IsEmpty())
			return difftool;
		// Maybe we should use TortoiseIDiff?
		if ((ext == L".jpg") || (ext == L".jpeg") ||
			(ext == L".bmp") || (ext == L".gif")  ||
			(ext == L".png") || (ext == L".ico")  ||
			(ext == L".tif") || (ext == L".tiff") ||
			(ext == L".dib") || (ext == L".emf")  ||
			(ext == L".cur"))
		{
			return
				L'"' + CPathUtils::GetAppDirectory() + L"TortoiseGitIDiff.exe" + L'"' +
				L" /left:%base /right:%mine /lefttitle:%bname /righttitle:%yname" +
				L" /groupuuid:\"" + g_sGroupingUUID + L'"';
		}
	}

	// Finally, pick a generic external diff tool
	difftool = CRegString(L"Software\\TortoiseGit\\Diff");
	return difftool;
}

bool CAppUtils::StartExtDiff(
	const CString& file1,  const CString& file2,
	const CString& sName1, const CString& sName2,
	const CString& originalFile1, const CString& originalFile2,
	const CString& hash1, const CString& hash2,
	const DiffFlags& flags, int jumpToLine)
{
	CString viewer;

	CRegDWORD blamediff(L"Software\\TortoiseGit\\DiffBlamesWithTortoiseMerge", FALSE);
	if (!flags.bBlame || !(DWORD)blamediff)
	{
		viewer = PickDiffTool(file1, file2);
		// If registry entry for a diff program is commented out, use TortoiseGitMerge.
		bool bCommentedOut = CStringUtils::StartsWith(viewer, L"#");
		if (flags.bAlternativeTool)
		{
			// Invert external vs. internal diff tool selection.
			if (bCommentedOut)
				viewer.Delete(0); // uncomment
			else
				viewer.Empty();
		}
		else if (bCommentedOut)
			viewer.Empty();
	}

	bool bInternal = viewer.IsEmpty();
	if (bInternal)
	{
		viewer =
			L'"' + CPathUtils::GetAppDirectory() + L"TortoiseGitMerge.exe" + L'"' +
			L" /base:%base /mine:%mine /basename:%bname /minename:%yname" +
			L" /basereflectedname:%bpath /minereflectedname:%ypath";
		if (!g_sGroupingUUID.IsEmpty())
		{
			viewer += L" /groupuuid:\"";
			viewer += g_sGroupingUUID;
			viewer += L'"';
		}
		if (flags.bBlame)
			viewer += L" /blame";
	}
	// check if the params are set. If not, just add the files to the command line
	if ((viewer.Find(L"%base") < 0) && (viewer.Find(L"%mine") < 0))
	{
		viewer += L" \"" + file1 + L'"';
		viewer += L" \"" + file2 + L'"';
	}
	if (viewer.Find(L"%base") >= 0)
		viewer.Replace(L"%base", L'"' + file1 + L'"');
	if (viewer.Find(L"%mine") >= 0)
		viewer.Replace(L"%mine", L'"' + file2 + L'"');

	if (sName1.IsEmpty())
		viewer.Replace(L"%bname", L'"' + file1 + L'"');
	else
		viewer.Replace(L"%bname", L'"' + sName1 + L'"');

	if (sName2.IsEmpty())
		viewer.Replace(L"%yname", L'"' + file2 + L'"');
	else
		viewer.Replace(L"%yname", L'"' + sName2 + L'"');

	viewer.Replace(L"%bpath", L'"' + originalFile1 + L'"');
	viewer.Replace(L"%ypath", L'"' + originalFile2 + L'"');

	viewer.Replace(L"%brev", L'"' + hash1 + L'"');
	viewer.Replace(L"%yrev", L'"' + hash2 + L'"');

	viewer.Replace(L"%wtroot", L'"' + g_Git.m_CurrentDir + L'"');

	if (flags.bReadOnly && bInternal)
		viewer += L" /readonly";

	if (jumpToLine > 0)
		viewer.AppendFormat(L" /line:%d", jumpToLine);

	return LaunchApplication(viewer, IDS_ERR_EXTDIFFSTART, flags.bWait);
}

BOOL CAppUtils::StartUnifiedDiffViewer(const CString& patchfile, const CString& title, BOOL bWait, bool bAlternativeTool)
{
	CString viewer;
	CRegString v = CRegString(L"Software\\TortoiseGit\\DiffViewer");
	viewer = v;

	// If registry entry for a diff program is commented out, use TortoiseGitMerge.
	bool bCommentedOut = CStringUtils::StartsWith(viewer, L"#");
	if (bAlternativeTool)
	{
		// Invert external vs. internal diff tool selection.
		if (bCommentedOut)
			viewer.Delete(0); // uncomment
		else
			viewer.Empty();
	}
	else if (bCommentedOut)
		viewer.Empty();

	if (viewer.IsEmpty())
	{
		// use TortoiseGitUDiff
		viewer = CPathUtils::GetAppDirectory();
		viewer += L"TortoiseGitUDiff.exe";
		// enquote the path to TortoiseGitUDiff
		viewer = L'"' + viewer + L'"';
		// add the params
		viewer = viewer + L" /patchfile:%1 /title:\"%title\"";
		if (!g_sGroupingUUID.IsEmpty())
		{
			viewer += L" /groupuuid:\"";
			viewer += g_sGroupingUUID;
			viewer += L'"';
		}
	}
	if (viewer.Find(L"%1") >= 0)
	{
		if (viewer.Find(L"\"%1\"") >= 0)
			viewer.Replace(L"%1", patchfile);
		else
			viewer.Replace(L"%1", L'"' + patchfile + L'"');
	}
	else
		viewer += L" \"" + patchfile + L'"';
	if (viewer.Find(L"%title") >= 0)
		viewer.Replace(L"%title", title);

	if(!LaunchApplication(viewer, IDS_ERR_DIFFVIEWSTART, !!bWait))
		return FALSE;
	return TRUE;
}

BOOL CAppUtils::StartTextViewer(CString file)
{
	CString viewer;
	CRegString txt = CRegString(L".txt\\", L"", FALSE, HKEY_CLASSES_ROOT);
	viewer = txt;
	viewer = viewer + L"\\Shell\\Open\\Command\\";
	CRegString txtexe = CRegString(viewer, L"", FALSE, HKEY_CLASSES_ROOT);
	viewer = txtexe;

	DWORD len = ExpandEnvironmentStrings(viewer, nullptr, 0);
	auto buf = std::make_unique<TCHAR[]>(len + 1);
	ExpandEnvironmentStrings(viewer, buf.get(), len);
	viewer = buf.get();
	len = ExpandEnvironmentStrings(file, nullptr, 0);
	auto buf2 = std::make_unique<TCHAR[]>(len + 1);
	ExpandEnvironmentStrings(file, buf2.get(), len);
	file = buf2.get();
	file = L'"' + file + L'"';
	if (viewer.IsEmpty())
		return CAppUtils::ShowOpenWithDialog(file) ? TRUE : FALSE;
	if (viewer.Find(L"\"%1\"") >= 0)
		viewer.Replace(L"\"%1\"", file);
	else if (viewer.Find(L"%1") >= 0)
		viewer.Replace(L"%1", file);
	else
		viewer += L' ';
		viewer += file;

	if(!LaunchApplication(viewer, IDS_ERR_TEXTVIEWSTART, false))
		return FALSE;
	return TRUE;
}

BOOL CAppUtils::CheckForEmptyDiff(const CTGitPath& sDiffPath)
{
	DWORD length = 0;
	CAutoFile hFile = ::CreateFile(sDiffPath.GetWinPath(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (!hFile)
		return TRUE;
	length = ::GetFileSize(hFile, nullptr);
	if (length < 4)
		return TRUE;
	return FALSE;
}

CString CAppUtils::GetLogFontName()
{
	return (CString)CRegString(L"Software\\TortoiseGit\\LogFontName", L"Consolas");
}

DWORD CAppUtils::GetLogFontSize()
{
	return (DWORD)CRegDWORD(L"Software\\TortoiseGit\\LogFontSize", 9);
}

void CAppUtils::CreateFontForLogs(CFont& fontToCreate)
{
	LOGFONT logFont;
	HDC hScreenDC = ::GetDC(nullptr);
	logFont.lfHeight = -MulDiv(GetLogFontSize(), GetDeviceCaps(hScreenDC, LOGPIXELSY), 72);
	::ReleaseDC(nullptr, hScreenDC);
	logFont.lfWidth				= 0;
	logFont.lfEscapement		= 0;
	logFont.lfOrientation		= 0;
	logFont.lfWeight			= FW_NORMAL;
	logFont.lfItalic			= 0;
	logFont.lfUnderline			= 0;
	logFont.lfStrikeOut			= 0;
	logFont.lfCharSet			= DEFAULT_CHARSET;
	logFont.lfOutPrecision		= OUT_DEFAULT_PRECIS;
	logFont.lfClipPrecision		= CLIP_DEFAULT_PRECIS;
	logFont.lfQuality			= DRAFT_QUALITY;
	logFont.lfPitchAndFamily	= FF_DONTCARE | FIXED_PITCH;
	wcscpy_s(logFont.lfFaceName, 32, (LPCTSTR)GetLogFontName());
	VERIFY(fontToCreate.CreateFontIndirect(&logFont));
}

bool CAppUtils::LaunchPAgent(const CString* keyfile, const CString* pRemote)
{
	CString key,remote;
	CString cmd,out;
	if (!pRemote)
		remote = L"origin";
	else
		remote=*pRemote;

	if (!keyfile)
	{
		cmd.Format(L"remote.%s.puttykeyfile", (LPCTSTR)remote);
		key = g_Git.GetConfigValue(cmd);
	}
	else
		key=*keyfile;

	if(key.IsEmpty())
		return false;

	CString proc=CPathUtils::GetAppDirectory();
	proc += L"pageant.exe \"";
	proc += key;
	proc += L'"';

	CString tempfile = GetTempFile();
	::DeleteFile(tempfile);

	proc += L" -c \"";
	proc += CPathUtils::GetAppDirectory();
	proc += L"tgittouch.exe\"";
	proc += L" \"";
	proc += tempfile;
	proc += L'"';

	CString appDir = CPathUtils::GetAppDirectory();
	bool b = LaunchApplication(proc, IDS_ERR_PAGEANT, true, &appDir);
	if(!b)
		return b;

	int i=0;
	while(!::PathFileExists(tempfile))
	{
		Sleep(100);
		++i;
		if(i>10*60*5)
			break; //timeout 5 minutes
	}

	if( i== 10*60*5)
		CMessageBox::Show(nullptr, IDS_ERR_PAEGENTTIMEOUT, IDS_APPNAME, MB_OK | MB_ICONERROR);
	::DeleteFile(tempfile);
	return true;
}

bool CAppUtils::LaunchAlternativeEditor(const CString& filename, bool uac)
{
	CString editTool = CRegString(L"Software\\TortoiseGit\\AlternativeEditor");
	if (editTool.IsEmpty() || CStringUtils::StartsWith(editTool, L"#"))
		editTool = CPathUtils::GetAppDirectory() + L"notepad2.exe";

	CString sCmd;
	sCmd.Format(L"\"%s\" \"%s\"", (LPCTSTR)editTool, (LPCTSTR)filename);

	LaunchApplication(sCmd, 0, false, nullptr, uac);
	return true;
}

bool CAppUtils::LaunchRemoteSetting()
{
	CTGitPath path(g_Git.m_CurrentDir);
	CSettings dlg(IDS_PROC_SETTINGS_TITLE, &path);
	dlg.SetTreeViewMode(TRUE, TRUE, TRUE);
	dlg.SetTreeWidth(220);
	dlg.m_DefaultPage = L"gitremote";

	dlg.DoModal();
	dlg.HandleRestart();
	return true;
}

/**
* Launch the external blame viewer
*/
bool CAppUtils::LaunchTortoiseBlame(const CString& sBlameFile, const CString& Rev, const CString& sParams)
{
	CString viewer = L'"' + CPathUtils::GetAppDirectory();
	viewer += L"TortoiseGitBlame.exe";
	viewer += L"\" \"" + sBlameFile + L'"';
	//viewer += L" \"" + sLogFile + L'"';
	//viewer += L" \"" + sOriginalFile + L'"';
	if(!Rev.IsEmpty() && Rev != GIT_REV_ZERO)
		viewer += L" /rev:" + Rev;
	if (!g_sGroupingUUID.IsEmpty())
	{
		viewer += L" /groupuuid:\"";
		viewer += g_sGroupingUUID;
		viewer += L'"';
	}
	viewer += L' ' + sParams;

	return LaunchApplication(viewer, IDS_ERR_TGITBLAME, false);
}

bool CAppUtils::FormatTextInRichEditControl(CWnd * pWnd)
{
	CString sText;
	if (!pWnd)
		return false;
	bool bStyled = false;
	pWnd->GetWindowText(sText);
	// the rich edit control doesn't count the CR char!
	// to be exact: CRLF is treated as one char.
	sText.Remove(L'\r');

	// style each line separately
	int offset = 0;
	int nNewlinePos;
	do
	{
		nNewlinePos = sText.Find('\n', offset);
		CString sLine = nNewlinePos >= 0 ? sText.Mid(offset, nNewlinePos - offset) : sText.Mid(offset);

		int start = 0;
		int end = 0;
		while (FindStyleChars(sLine, '*', start, end))
		{
			CHARRANGE range = {(LONG)start+offset, (LONG)end+offset};
			pWnd->SendMessage(EM_EXSETSEL, NULL, (LPARAM)&range);
			SetCharFormat(pWnd, CFM_BOLD, CFE_BOLD);
			bStyled = true;
			start = end;
		}
		start = 0;
		end = 0;
		while (FindStyleChars(sLine, '^', start, end))
		{
			CHARRANGE range = {(LONG)start+offset, (LONG)end+offset};
			pWnd->SendMessage(EM_EXSETSEL, NULL, (LPARAM)&range);
			SetCharFormat(pWnd, CFM_ITALIC, CFE_ITALIC);
			bStyled = true;
			start = end;
		}
		start = 0;
		end = 0;
		while (FindStyleChars(sLine, '_', start, end))
		{
			CHARRANGE range = {(LONG)start+offset, (LONG)end+offset};
			pWnd->SendMessage(EM_EXSETSEL, NULL, (LPARAM)&range);
			SetCharFormat(pWnd, CFM_UNDERLINE, CFE_UNDERLINE);
			bStyled = true;
			start = end;
		}
		offset = nNewlinePos+1;
	} while(nNewlinePos>=0);
	return bStyled;
}

bool CAppUtils::FindStyleChars(const CString& sText, TCHAR stylechar, int& start, int& end)
{
	int i=start;
	int last = sText.GetLength() - 1;
	bool bFoundMarker = false;
	TCHAR c = i == 0 ? L'\0' : sText[i - 1];
	TCHAR nextChar = i >= last ? L'\0' : sText[i + 1];

	// find a starting marker
	while (i < last)
	{
		TCHAR prevChar = c;
		c = nextChar;
		nextChar = sText[i + 1];

		// IsCharAlphaNumeric can be somewhat expensive.
		// Long lines of "*****" or "----" will be pre-empted efficiently
		// by the (c != nextChar) condition.

		if ((c == stylechar) && (c != nextChar))
		{
			if (IsCharAlphaNumeric(nextChar) && !IsCharAlphaNumeric(prevChar))
			{
				start = ++i;
				bFoundMarker = true;
				break;
			}
		}
		++i;
	}
	if (!bFoundMarker)
		return false;

	// find ending marker
	// c == sText[i - 1]

	bFoundMarker = false;
	while (i <= last)
	{
		TCHAR prevChar = c;
		c = sText[i];
		if (c == stylechar)
		{
			if ((i == last) || (!IsCharAlphaNumeric(sText[i + 1]) && IsCharAlphaNumeric(prevChar)))
			{
				end = i;
				++i;
				bFoundMarker = true;
				break;
			}
		}
		++i;
	}
	return bFoundMarker;
}

// from CSciEdit
namespace {
	bool IsValidURLChar(wchar_t ch)
	{
		return iswalnum(ch) ||
			ch == L'_' || ch == L'/' || ch == L';' || ch == L'?' || ch == L'&' || ch == L'=' ||
			ch == L'%' || ch == L':' || ch == L'.' || ch == L'#' || ch == L'-' || ch == L'+' ||
			ch == L'|' || ch == L'>' || ch == L'<' || ch == L'!' || ch == L'@' || ch == L'~';
	}

	bool IsUrlOrEmail(const CString& sText)
	{
		if (!PathIsURLW(sText))
		{
			auto atpos = sText.Find(L'@');
			if (atpos <= 0)
				return false;
			if (sText.Find(L'.', atpos) <= atpos + 1) // a dot must follow after the @, but not directly after it
				return false;
			if (sText.Find(L':', atpos) < 0) // do not detect git@example.com:something as an email address
				return true;
			return false;
		}
		for (const CString& prefix : { L"http://", L"https://", L"git://", L"ftp://", L"file://", L"mailto:" })
		{
			if (CStringUtils::StartsWith(sText, prefix) && sText.GetLength() != prefix.GetLength())
				return true;
		}
		return false;
	}
}

BOOL CAppUtils::StyleURLs(const CString& msg, CWnd* pWnd)
{
	std::vector<CHARRANGE> positions = FindURLMatches(msg);
	CAppUtils::SetCharFormat(pWnd, CFM_LINK, CFE_LINK, positions);

	return positions.empty() ? FALSE : TRUE;
}

/**
* implements URL searching with the same logic as CSciEdit::StyleURLs
*/
std::vector<CHARRANGE> CAppUtils::FindURLMatches(const CString& msg)
{
	std::vector<CHARRANGE> result;

	int len = msg.GetLength();
	int starturl = -1;

	for (int i = 0; i <= msg.GetLength(); ++i)
	{
		if ((i < len) && IsValidURLChar(msg[i]))
		{
			if (starturl < 0)
				starturl = i;
		}
		else
		{
			if (starturl >= 0)
			{
				bool strip = true;
				if (msg[starturl] == '<' && i < len) // try to detect and do not strip URLs put within <>
				{
					while (starturl <= i && msg[starturl] == '<') // strip leading '<'
						++starturl;
					strip = false;
					i = starturl;
					while (i < len && msg[i] != '\r' && msg[i] != '\n' && msg[i] != '>') // find first '>' or new line after resetting i to start position
						++i;
				}

				int skipTrailing = 0;
				while (strip && i - skipTrailing - 1 > starturl && (msg[i - skipTrailing - 1] == '.' || msg[i - skipTrailing - 1] == '-' || msg[i - skipTrailing - 1] == '?' || msg[i - skipTrailing - 1] == ';' || msg[i - skipTrailing - 1] == ':' || msg[i - skipTrailing - 1] == '>' || msg[i - skipTrailing - 1] == '<' || msg[i - skipTrailing - 1] == '!'))
					++skipTrailing;

				if (!IsUrlOrEmail(msg.Mid(starturl, i - starturl - skipTrailing)))
				{
					starturl = -1;
					continue;
				}

				CHARRANGE range = { starturl, i - skipTrailing };
				result.push_back(range);
			}
			starturl = -1;
		}
	}

	return result;
}

bool CAppUtils::StartShowUnifiedDiff(HWND hWnd, const CTGitPath& url1, const CString& rev1,
												const CTGitPath& /*url2*/, const CString& rev2,
												//const GitRev& peg /* = GitRev */, const GitRev& headpeg /* = GitRev */,
												bool bAlternateDiff /* = false */, bool /*bIgnoreAncestry*/ /* = false */,
												bool /* blame = false */, 
												bool bMerge,
												bool bCombine,
												bool bNoPrefix)
{
	int diffContext = g_Git.GetConfigValueInt32(L"diff.context", -1);
	CString tempfile=GetTempFile();
	if (g_Git.GetUnifiedDiff(url1, rev1, rev2, tempfile, bMerge, bCombine, diffContext, bNoPrefix))
	{
		MessageBox(hWnd, g_Git.GetGitLastErr(L"Could not get unified diff.", CGit::GIT_CMD_DIFF), L"TortoiseGit", MB_OK);
		return false;
	}
	CAppUtils::StartUnifiedDiffViewer(tempfile, rev1.IsEmpty() ? rev2 : rev1 + L':' + rev2, FALSE, bAlternateDiff);

#if 0
	CString sCmd;
	sCmd.Format(L"%s /command:showcompare /unified",
		(LPCTSTR)(CPathUtils::GetAppDirectory()+L"TortoiseGitProc.exe"));
	sCmd += L" /url1:\"" + url1.GetGitPathString() + L'"';
	if (rev1.IsValid())
		sCmd += L" /revision1:" + rev1.ToString();
	sCmd += L" /url2:\"" + url2.GetGitPathString() + L'"';
	if (rev2.IsValid())
		sCmd += L" /revision2:" + rev2.ToString();
	if (peg.IsValid())
		sCmd += L" /pegrevision:" + peg.ToString();
	if (headpeg.IsValid())
		sCmd += L" /headpegrevision:" + headpeg.ToString();

	if (bAlternateDiff)
		sCmd += L" /alternatediff";

	if (bIgnoreAncestry)
		sCmd += L" /ignoreancestry";

	if (hWnd)
	{
		sCmd += L" /hwnd:";
		TCHAR buf[30];
		swprintf_s(buf, 30, L"%p", (void*)hWnd);
		sCmd += buf;
	}

	return CAppUtils::LaunchApplication(sCmd, 0, false);
#endif
	return TRUE;
}

bool CAppUtils::SetupDiffScripts(bool force, const CString& type)
{
	CString scriptsdir = CPathUtils::GetAppParentDirectory();
	scriptsdir += L"Diff-Scripts";
	CSimpleFileFind files(scriptsdir);
	while (files.FindNextFileNoDirectories())
	{
		CString file = files.GetFilePath();
		CString filename = files.GetFileName();
		CString ext = file.Mid(file.ReverseFind('-') + 1);
		ext = L"." + ext.Left(ext.ReverseFind(L'.'));
		std::set<CString> extensions;
		extensions.insert(ext);
		CString kind;
		if (CStringUtils::EndsWithI(file, L"vbs"))
			kind = L" //E:vbscript";
		if (CStringUtils::EndsWithI(file, L"js"))
			kind = L" //E:javascript";
		// open the file, read the first line and find possible extensions
		// this script can handle
		try
		{
			CStdioFile f(file, CFile::modeRead | CFile::shareDenyNone);
			CString extline;
			if (f.ReadString(extline))
			{
				if ((extline.GetLength() > 15 ) &&
					(CStringUtils::StartsWith(extline, L"// extensions: ") ||
					CStringUtils::StartsWith(extline, L"' extensions: ")))
				{
					if (extline[0] == '/')
						extline = extline.Mid(15);
					else
						extline = extline.Mid(14);
					CString sToken;
					int curPos = 0;
					sToken = extline.Tokenize(L";", curPos);
					while (!sToken.IsEmpty())
					{
						if (!sToken.IsEmpty())
						{
							if (sToken[0] != '.')
								sToken = L"." + sToken;
							extensions.insert(sToken);
						}
						sToken = extline.Tokenize(L";", curPos);
					}
				}
			}
			f.Close();
		}
		catch (CFileException* e)
		{
			e->Delete();
		}

		for (const auto& extension : extensions)
		{
			if (type.IsEmpty() || (type.Compare(L"Diff") == 0))
			{
				if (CStringUtils::StartsWithI(filename, L"diff-"))
				{
					CRegString diffreg = CRegString(L"Software\\TortoiseGit\\DiffTools\\" + extension);
					CString diffregstring = diffreg;
					if (force || (diffregstring.IsEmpty()) || (diffregstring.Find(filename) >= 0))
						diffreg = L"wscript.exe \"" + file + L"\" %base %mine" + kind;
				}
			}
			if (type.IsEmpty() || (type.Compare(L"Merge") == 0))
			{
				if (CStringUtils::StartsWithI(filename, L"merge-"))
				{
					CRegString diffreg = CRegString(L"Software\\TortoiseGit\\MergeTools\\" + extension);
					CString diffregstring = diffreg;
					if (force || (diffregstring.IsEmpty()) || (diffregstring.Find(filename) >= 0))
						diffreg = L"wscript.exe \"" + file + L"\" %merged %theirs %mine %base" + kind;
				}
			}
		}
	}

	return true;
}

bool CAppUtils::Export(const CString* BashHash, const CTGitPath* orgPath)
{
		// ask from where the export has to be done
	CExportDlg dlg;
	if(BashHash)
		dlg.m_initialRefName=*BashHash;
	if (orgPath)
	{
		if (PathIsRelative(orgPath->GetWinPath()))
			dlg.m_orgPath = g_Git.CombinePath(orgPath);
		else
			dlg.m_orgPath = *orgPath;
	}

	if (dlg.DoModal() == IDOK)
	{
		CString cmd;
		cmd.Format(L"git.exe archive --output=\"%s\" --format=zip --verbose %s --",
					(LPCTSTR)dlg.m_strFile, (LPCTSTR)g_Git.FixBranchName(dlg.m_VersionName));

		CProgressDlg pro;
		pro.m_GitCmd=cmd;
		pro.m_PostCmdCallback = [&](DWORD status, PostCmdList& postCmdList)
		{
			if (status)
				return;
			postCmdList.emplace_back(IDI_EXPLORER, IDS_STATUSLIST_CONTEXT_EXPLORE, [&]{ CAppUtils::ExploreTo(hWndExplorer, dlg.m_strFile); });
		};

		CGit git;
		if (!dlg.m_bWholeProject && !dlg.m_orgPath.IsEmpty() && PathIsDirectory(dlg.m_orgPath.GetWinPathString()))
		{
			git.m_CurrentDir = dlg.m_orgPath.GetWinPathString();
			pro.m_Git = &git;
		}
		return (pro.DoModal() == IDOK);
	}
	return false;
}

bool CAppUtils::UpdateBranchDescription(const CString& branch, CString description)
{
	if (branch.IsEmpty())
		return false;

	CString key;
	key.Format(L"branch.%s.description", (LPCTSTR)branch);
	description.Remove(L'\r');
	description.Trim();
	if (description.IsEmpty())
		g_Git.UnsetConfigValue(key);
	else
		g_Git.SetConfigValue(key, description);

	return true;
}

bool CAppUtils::CreateBranchTag(bool isTag /*true*/, const CString* commitHash /*nullptr*/, bool switchNewBranch /*false*/, LPCTSTR name /*nullptr*/)
{
	CCreateBranchTagDlg dlg;
	dlg.m_bIsTag = isTag;
	dlg.m_bSwitch = switchNewBranch;

	if (commitHash)
		dlg.m_initialRefName = *commitHash;

	if (name)
		dlg.m_BranchTagName = name;

	if(dlg.DoModal()==IDOK)
	{
		CString cmd;
		CString force;
		CString track;
		if(dlg.m_bTrack == TRUE)
			track = L"--track";
		else if(dlg.m_bTrack == FALSE)
			track = L"--no-track";

		if(dlg.m_bForce)
			force = L"-f";

		if (isTag)
		{
			CString sign;
			if(dlg.m_bSign)
				sign = L"-s";

			cmd.Format(L"git.exe tag %s %s %s %s",
				(LPCTSTR)force,
				(LPCTSTR)sign,
				(LPCTSTR)dlg.m_BranchTagName,
				(LPCTSTR)g_Git.FixBranchName(dlg.m_VersionName)
				);

			if(!dlg.m_Message.Trim().IsEmpty())
			{
				CString tempfile = ::GetTempFile();
				if (CAppUtils::SaveCommitUnicodeFile(tempfile, dlg.m_Message))
				{
					MessageBox(nullptr, L"Could not save tag message", L"TortoiseGit", MB_OK | MB_ICONERROR);
					return FALSE;
				}
				cmd += L" -F " + tempfile;
			}
		}
		else
		{
			cmd.Format(L"git.exe branch %s %s %s %s",
				(LPCTSTR)track,
				(LPCTSTR)force,
				(LPCTSTR)dlg.m_BranchTagName,
				(LPCTSTR)g_Git.FixBranchName(dlg.m_VersionName)
				);
		}
		CString out;
		if(g_Git.Run(cmd,&out,CP_UTF8))
		{
			MessageBox(nullptr, out, L"TortoiseGit", MB_OK | MB_ICONERROR);
			return FALSE;
		}
		if (!isTag && dlg.m_bSwitch)
		{
			// it is a new branch and the user has requested to switch to it
			PerformSwitch(dlg.m_BranchTagName);
		}
		if (!isTag && !dlg.m_Message.IsEmpty())
			UpdateBranchDescription(dlg.m_BranchTagName, dlg.m_Message);

		return TRUE;
	}
	return FALSE;
}

bool CAppUtils::Switch(const CString& initialRefName)
{
	CGitSwitchDlg dlg;
	if(!initialRefName.IsEmpty())
		dlg.m_initialRefName = initialRefName;

	if (dlg.DoModal() == IDOK)
	{
		CString branch;
		if (dlg.m_bBranch)
			branch = dlg.m_NewBranch;

		// if refs/heads/ is not stripped, checkout will detach HEAD
		// checkout prefers branches on name clashes (with tags)
		if (CStringUtils::StartsWith(dlg.m_VersionName, L"refs/heads/") && dlg.m_bBranchOverride != TRUE)
			dlg.m_VersionName = dlg.m_VersionName.Mid(11);

		return PerformSwitch(dlg.m_VersionName, dlg.m_bForce == TRUE , branch, dlg.m_bBranchOverride == TRUE, dlg.m_bTrack, dlg.m_bMerge == TRUE);
	}
	return FALSE;
}

bool CAppUtils::PerformSwitch(const CString& ref, bool bForce /* false */, const CString& sNewBranch /* CString() */, bool bBranchOverride /* false */, BOOL bTrack /* 2 */, bool bMerge /* false */)
{
	CString cmd;
	CString track;
	CString force;
	CString branch;
	CString merge;

	if(!sNewBranch.IsEmpty()){
		if (bBranchOverride)
			branch.Format(L"-B %s ", (LPCTSTR)sNewBranch);
		else
			branch.Format(L"-b %s ", (LPCTSTR)sNewBranch);
		if (bTrack == TRUE)
			track = L"--track ";
		else if (bTrack == FALSE)
			track = L"--no-track ";
	}
	if (bForce)
		force = L"-f ";
	if (bMerge)
		merge = L"--merge ";

	cmd.Format(L"git.exe checkout %s%s%s%s%s --",
		 (LPCTSTR)force,
		 (LPCTSTR)track,
		 (LPCTSTR)merge,
		 (LPCTSTR)branch,
		 (LPCTSTR)g_Git.FixBranchName(ref));

	CProgressDlg progress;
	progress.m_GitCmd = cmd;

	CString currentBranch;
	bool hasBranch = CGit::GetCurrentBranchFromFile(g_Git.m_CurrentDir, currentBranch) == 0;
	progress.m_PostCmdCallback = [&](DWORD status, PostCmdList& postCmdList)
	{
		if (!status)
		{
			CTGitPath gitPath = g_Git.m_CurrentDir;
			if (gitPath.HasSubmodules())
			{
				postCmdList.emplace_back(IDI_UPDATE, IDS_PROC_SUBMODULESUPDATE, [&]
				{
					CString sCmd;
					sCmd.Format(L"/command:subupdate /bkpath:\"%s\"", (LPCTSTR)g_Git.m_CurrentDir);
					RunTortoiseGitProc(sCmd);
				});
			}
			if (hasBranch)
				postCmdList.emplace_back(IDI_MERGE, IDS_MENUMERGE, [&]{ Merge(&currentBranch); });


			CString newBranch;
			if (!CGit::GetCurrentBranchFromFile(g_Git.m_CurrentDir, newBranch))
				postCmdList.emplace_back(IDI_PULL, IDS_MENUPULL, [&]{ Pull(); });

			postCmdList.emplace_back(IDI_COMMIT, IDS_MENUCOMMIT, []{
				CTGitPathList pathlist;
				CTGitPathList selectedlist;
				pathlist.AddPath(CTGitPath());
				bool bSelectFilesForCommit = !!DWORD(CRegStdDWORD(L"Software\\TortoiseGit\\SelectFilesForCommit", TRUE));
				CString str;
				Commit(CString(), false, str, pathlist, selectedlist, bSelectFilesForCommit);
			});
		}
		else
		{
			if (bMerge && g_Git.HasWorkingTreeConflicts() > 0)
			{
				postCmdList.emplace_back(IDI_RESOLVE, IDS_PROGRS_CMD_RESOLVE, []
				{
					CString sCmd;
					sCmd.Format(L"/command:commit /path:\"%s\"", (LPCTSTR)g_Git.m_CurrentDir);
					CAppUtils::RunTortoiseGitProc(sCmd);
				});
			}
			postCmdList.emplace_back(IDI_REFRESH, IDS_MSGBOX_RETRY, [&]{ PerformSwitch(ref, bForce, sNewBranch, bBranchOverride, bTrack, bMerge); });
			if (!bMerge)
				postCmdList.emplace_back(IDI_SWITCH, IDS_SWITCH_WITH_MERGE, [&]{ PerformSwitch(ref, bForce, sNewBranch, bBranchOverride, bTrack, true); });
		}
	};
	progress.m_PostExecCallback = [&](DWORD& exitCode, CString& extraMsg)
	{
		if (bMerge && !exitCode && g_Git.HasWorkingTreeConflicts() > 0)
		{
			exitCode = 1; // Treat it as failure
			extraMsg = L"Has merge conflict";
		}
	};

	INT_PTR ret = progress.DoModal();

	return ret == IDOK;
}

class CIgnoreFile : public CStdioFile
{
public:
	STRING_VECTOR m_Items;
	CString m_eol;

	virtual BOOL ReadString(CString& rString)
	{
		if (GetPosition() == 0)
		{
			unsigned char utf8bom[] = { 0xEF, 0xBB, 0xBF };
			char buf[3] = { 0, 0, 0 };
			Read(buf, 3);
			if (memcpy(buf, utf8bom, sizeof(utf8bom)))
				SeekToBegin();
		}

		CStringA strA;
		char lastChar = '\0';
		for (char c = '\0'; Read(&c, 1) == 1; lastChar = c)
		{
			if (c == '\r')
				continue;
			if (c == '\n')
			{
				m_eol = lastChar == L'\r' ? L"\r\n" : L"\n";
				break;
			}
			strA.AppendChar(c);
		}
		if (strA.IsEmpty())
			return FALSE;

		rString = CUnicodeUtils::GetUnicode(strA);
		return TRUE;
	}

	void ResetState()
	{
		m_Items.clear();
		m_eol.Empty();
	}
};

bool CAppUtils::OpenIgnoreFile(CIgnoreFile &file, const CString& filename)
{
	file.ResetState();
	if (!file.Open(filename, CFile::modeCreate | CFile::modeReadWrite | CFile::modeNoTruncate | CFile::typeBinary))
	{
		MessageBox(nullptr, filename + L" Open Failure", L"TortoiseGit", MB_OK | MB_ICONERROR);
		return false;
	}

	if (file.GetLength() > 0)
	{
		CString fileText;
		while (file.ReadString(fileText))
			file.m_Items.push_back(fileText);
		file.Seek(file.GetLength() - 1, 0);
		char lastchar[1] = { 0 };
		file.Read(lastchar, 1);
		file.SeekToEnd();
		if (lastchar[0] != '\n')
		{
			CStringA eol = CStringA(file.m_eol.IsEmpty() ? L"\n" : file.m_eol);
			file.Write(eol, eol.GetLength());
		}
	}
	else
		file.SeekToEnd();

	return true;
}

bool CAppUtils::IgnoreFile(const CTGitPathList& path,bool IsMask)
{
	CIgnoreDlg ignoreDlg;
	if (ignoreDlg.DoModal() == IDOK)
	{
		CString ignorefile;
		ignorefile = g_Git.m_CurrentDir + L'\\';

		switch (ignoreDlg.m_IgnoreFile)
		{
			case 0:
				ignorefile += L".gitignore";
				break;
			case 2:
				GitAdminDir::GetAdminDirPath(g_Git.m_CurrentDir, ignorefile);
				ignorefile += L"info";
				if (!PathFileExists(ignorefile))
					CreateDirectory(ignorefile, nullptr);
				ignorefile += L"\\exclude";
				break;
		}

		CIgnoreFile file;
		try
		{
			if (ignoreDlg.m_IgnoreFile != 1 && !OpenIgnoreFile(file, ignorefile))
				return false;

			for (int i = 0; i < path.GetCount(); ++i)
			{
				if (ignoreDlg.m_IgnoreFile == 1)
				{
					ignorefile = g_Git.CombinePath(path[i].GetContainingDirectory()) + L"\\.gitignore";
					if (!OpenIgnoreFile(file, ignorefile))
						return false;
				}

				CString ignorePattern;
				if (ignoreDlg.m_IgnoreType == 0)
				{
					if (ignoreDlg.m_IgnoreFile != 1 && !path[i].GetContainingDirectory().GetGitPathString().IsEmpty())
						ignorePattern += L'/' + path[i].GetContainingDirectory().GetGitPathString();

					ignorePattern += L'/';
				}
				if (IsMask)
					ignorePattern += L'*' + path[i].GetFileExtension();
				else
					ignorePattern += path[i].GetFileOrDirectoryName();

				// escape [ and ] so that files get ignored correctly
				ignorePattern.Replace(L"[", L"\\[");
				ignorePattern.Replace(L"]", L"\\]");

				bool found = false;
				for (size_t j = 0; j < file.m_Items.size(); ++j)
				{
					if (file.m_Items[j] == ignorePattern)
					{
						found = true;
						break;
					}
				}
				if (!found)
				{
					file.m_Items.push_back(ignorePattern);
					ignorePattern += file.m_eol.IsEmpty() ? L"\n" : file.m_eol;
					CStringA ignorePatternA = CUnicodeUtils::GetUTF8(ignorePattern);
					file.Write(ignorePatternA, ignorePatternA.GetLength());
				}

				if (ignoreDlg.m_IgnoreFile == 1)
					file.Close();
			}

			if (ignoreDlg.m_IgnoreFile != 1)
				file.Close();
		}
		catch(...)
		{
			file.Abort();
			return false;
		}

		return true;
	}
	return false;
}

static bool Reset(const CString& resetTo, int resetType)
{
	CString cmd;
	CString type;
	switch (resetType)
	{
	case 0:
		type = L"--soft";
		break;
	case 1:
		type = L"--mixed";
		break;
	case 2:
		type = L"--hard";
		break;
	case 3:
	{
		CProgressDlg progress;
		progress.m_GitCmd = L"git.exe reset --merge";
		progress.m_PostCmdCallback = [&](DWORD status, PostCmdList& postCmdList)
		{
			if (status)
			{
				postCmdList.emplace_back(IDI_REFRESH, IDS_MSGBOX_RETRY, [] { CAppUtils::MergeAbort(); });
				return;
			}

			CTGitPath gitPath = g_Git.m_CurrentDir;
			if (gitPath.HasSubmodules() && resetType == 2)
			{
				postCmdList.emplace_back(IDI_UPDATE, IDS_PROC_SUBMODULESUPDATE, [&]
				{
					CString sCmd;
					sCmd.Format(L"/command:subupdate /bkpath:\"%s\"", (LPCTSTR)g_Git.m_CurrentDir);
					CAppUtils::RunTortoiseGitProc(sCmd);
				});
			}
		};
		return progress.DoModal() == IDOK;
	}
	default:
		ATLASSERT(false);
		resetType = 1;
		type = L"--mixed";
		break;
	}
	cmd.Format(L"git.exe reset %s %s --", (LPCTSTR)type, (LPCTSTR)resetTo);

	CProgressDlg progress;
	progress.m_GitCmd = cmd;

	progress.m_PostCmdCallback = [&](DWORD status, PostCmdList& postCmdList)
	{
		if (status)
		{
			postCmdList.emplace_back(IDI_REFRESH, IDS_MSGBOX_RETRY, [&]{ Reset(resetTo, resetType); });
			return;
		}

		CTGitPath gitPath = g_Git.m_CurrentDir;
		if (gitPath.HasSubmodules() && resetType == 2)
		{
			postCmdList.emplace_back(IDI_UPDATE, IDS_PROC_SUBMODULESUPDATE, [&]
			{
				CString sCmd;
				sCmd.Format(L"/command:subupdate /bkpath:\"%s\"", (LPCTSTR)g_Git.m_CurrentDir);
				CAppUtils::RunTortoiseGitProc(sCmd);
			});
		}
	};

	INT_PTR ret;
	if (g_Git.UsingLibGit2(CGit::GIT_CMD_RESET))
	{
		CGitProgressDlg gitdlg;
		ResetProgressCommand resetProgressCommand;
		gitdlg.SetCommand(&resetProgressCommand);
		resetProgressCommand.m_PostCmdCallback = progress.m_PostCmdCallback;
		resetProgressCommand.SetRevision(resetTo);
		resetProgressCommand.SetResetType(resetType);
		ret = gitdlg.DoModal();
	}
	else
		ret = progress.DoModal();

	return ret == IDOK;
}

bool CAppUtils::GitReset(const CString* CommitHash, int type)
{
	CResetDlg dlg;
	dlg.m_ResetType=type;
	dlg.m_ResetToVersion=*CommitHash;
	dlg.m_initialRefName = *CommitHash;
	if (dlg.DoModal() == IDOK)
		return Reset(dlg.m_ResetToVersion, dlg.m_ResetType);

	return false;
}

void CAppUtils::DescribeConflictFile(bool mode, bool base,CString &descript)
{
	if(mode == FALSE)
	{
		descript.LoadString(IDS_SVNACTION_DELETE);
		return;
	}
	if(base)
	{
		descript.LoadString(IDS_SVNACTION_MODIFIED);
		return;
	}
	descript.LoadString(IDS_PROC_CREATED);
}

void CAppUtils::RemoveTempMergeFile(const CTGitPath& path)
{
	::DeleteFile(CAppUtils::GetMergeTempFile(L"LOCAL", path));
	::DeleteFile(CAppUtils::GetMergeTempFile(L"REMOTE", path));
	::DeleteFile(CAppUtils::GetMergeTempFile(L"BASE", path));
}
CString CAppUtils::GetMergeTempFile(const CString& type, const CTGitPath &merge)
{
	return g_Git.CombinePath(merge.GetWinPathString() + L'.' + type + merge.GetFileExtension());;
}

static bool ParseHashesFromLsFile(const BYTE_VECTOR& out, CString& hash1, bool& isFile1, CString& hash2, bool& isFile2, CString& hash3, bool& isFile3)
{
	size_t pos = 0;
	CString one;
	CString part;

	while (pos < out.size())
	{
		one.Empty();

		CGit::StringAppend(&one, &out[pos], CP_UTF8);
		int tabstart = 0;
		one.Tokenize(L"\t", tabstart);

		tabstart = 0;
		part = one.Tokenize(L" ", tabstart); //Tag
		CString mode = one.Tokenize(L" ", tabstart); //Mode
		part = one.Tokenize(L" ", tabstart); //Hash
		CString hash = part;
		part = one.Tokenize(L"\t", tabstart); //Stage
		int stage = _wtol(part);
		if (stage == 1)
		{
			hash1 = hash;
			isFile1 = _wtol(mode) != 160000;
		}
		else if (stage == 2)
		{
			hash2 = hash;
			isFile2 = _wtol(mode) != 160000;
		}
		else if (stage == 3)
		{
			hash3 = hash;
			isFile3 = _wtol(mode) != 160000;
			return true;
		}

		pos = out.findNextString(pos);
	}

	return false;
}

void CAppUtils::GetConflictTitles(CString* baseText, CString& mineText, CString& theirsText, bool rebaseActive)
{
	if (baseText)
		baseText->LoadString(IDS_PROC_DIFF_BASE);
	if (rebaseActive)
	{
		CString adminDir;
		GitAdminDir::GetAdminDirPath(g_Git.m_CurrentDir, adminDir);
		mineText = L"Branch being rebased onto";
		if (!CStringUtils::ReadStringFromTextFile(adminDir + L"tgitrebase.active\\onto", mineText))
		{
			CGitHash hash;
			if (!g_Git.GetHash(hash, L"rebase-apply/onto"))
				g_Git.GuessRefForHash(mineText, hash);
		}
		theirsText = L"Branch being rebased";
		if (!CStringUtils::ReadStringFromTextFile(adminDir + L"tgitrebase.active\\head-name", theirsText))
		{
			if (CStringUtils::ReadStringFromTextFile(adminDir + L"rebase-apply/head-name", theirsText))
				theirsText = CGit::StripRefName(theirsText);
		}
		return;
	}

	static const struct {
		const wchar_t*	headref;
		bool			guessRef;
		UINT			theirstext;
	} infotexts[] = { { L"MERGE_HEAD", true, IDS_CONFLICT_INFOTEXT }, { L"CHERRY_PICK_HEAD", false, IDS_CONFLICT_INFOTEXT }, { L"REVERT_HEAD", false, IDS_CONFLICT_REVERT } };
	mineText = L"HEAD";
	theirsText.LoadString(IDS_CONFLICT_REFTOBEMERGED);
	for (const auto& infotext : infotexts)
	{
		CGitHash hash;
		if (!g_Git.GetHash(hash, infotext.headref))
		{
			CString guessedRef;
			if (!infotext.guessRef)
				guessedRef = hash.ToString();
			else
				g_Git.GuessRefForHash(guessedRef, hash);
			theirsText.FormatMessage(infotext.theirstext, infotext.headref, (LPCTSTR)guessedRef);
			break;
		}
	}
}

bool CAppUtils::ConflictEdit(CTGitPath& path, bool bAlternativeTool /*= false*/, bool isRebase /*= false*/, HWND resolveMsgHwnd /*= nullptr*/)
{
	CTGitPath merge=path;
	CTGitPath directory = merge.GetDirectory();

	BYTE_VECTOR vector;

	CString cmd;
	cmd.Format(L"git.exe ls-files -u -t -z -- \"%s\"", (LPCTSTR)merge.GetGitPathString());

	if (g_Git.Run(cmd, &vector))
		return FALSE;

	CString baseTitle, mineTitle, theirsTitle;
	GetConflictTitles(&baseTitle, mineTitle, theirsTitle, isRebase);

	CString baseHash, realBaseHash(GIT_REV_ZERO), localHash(GIT_REV_ZERO), remoteHash(GIT_REV_ZERO);
	bool baseIsFile = true, localIsFile = true, remoteIsFile = true;
	if (ParseHashesFromLsFile(vector, realBaseHash, baseIsFile, localHash, localIsFile, remoteHash, remoteIsFile))
		baseHash = realBaseHash;

	if (!baseIsFile || !localIsFile || !remoteIsFile)
	{
		if (merge.HasAdminDir())
		{
			CGit subgit;
			subgit.m_CurrentDir = g_Git.CombinePath(merge);
			CGitHash hash;
			subgit.GetHash(hash, L"HEAD");
			baseHash = hash;
		}

		CGitDiff::ChangeType changeTypeMine = CGitDiff::Unknown;
		CGitDiff::ChangeType changeTypeTheirs = CGitDiff::Unknown;

		bool baseOK = false, mineOK = false, theirsOK = false;
		CString baseSubject, mineSubject, theirsSubject;
		if (merge.HasAdminDir())
		{
			CGit subgit;
			subgit.m_CurrentDir = g_Git.CombinePath(merge);
			CGitDiff::GetSubmoduleChangeType(subgit, baseHash, localHash, baseOK, mineOK, changeTypeMine, baseSubject, mineSubject);
			CGitDiff::GetSubmoduleChangeType(subgit, baseHash, remoteHash, baseOK, theirsOK, changeTypeTheirs, baseSubject, theirsSubject);
		}
		else if (baseHash == GIT_REV_ZERO && localHash == GIT_REV_ZERO && remoteHash != GIT_REV_ZERO) // merge conflict with no submodule, but submodule in merged revision (not initialized) 
		{
			changeTypeMine = CGitDiff::Identical;
			changeTypeTheirs = CGitDiff::NewSubmodule;
			baseSubject.LoadString(IDS_CONFLICT_NOSUBMODULE);
			mineSubject = baseSubject;
			theirsSubject.LoadString(IDS_CONFLICT_SUBMODULENOTINITIALIZED);
		}
		else if (baseHash.IsEmpty() && localHash != GIT_REV_ZERO && remoteHash == GIT_REV_ZERO) // merge conflict with no submodule initialized, but submodule exists in base and folder with no submodule is merged
		{
			baseHash = localHash;
			baseSubject.LoadString(IDS_CONFLICT_SUBMODULENOTINITIALIZED);
			mineSubject = baseSubject;
			theirsSubject.LoadString(IDS_CONFLICT_SUBMODULENOTINITIALIZED);
			changeTypeMine = CGitDiff::Identical;
			changeTypeTheirs = CGitDiff::DeleteSubmodule;
		}
		else if (baseHash != GIT_REV_ZERO && localHash != GIT_REV_ZERO && remoteHash != GIT_REV_ZERO) // base has submodule, mine has submodule and theirs also, but not initialized
		{
			baseSubject.LoadString(IDS_CONFLICT_SUBMODULENOTINITIALIZED);
			mineSubject = baseSubject;
			theirsSubject = baseSubject;
			if (baseHash == localHash)
				changeTypeMine = CGitDiff::Identical;
		}
		else if (baseHash == GIT_REV_ZERO && localHash != GIT_REV_ZERO && remoteHash != GIT_REV_ZERO)
		{
			baseOK = true;
			mineSubject = baseSubject;
			if (remoteIsFile)
			{
				theirsSubject.LoadString(IDS_CONFLICT_NOTASUBMODULE);
				changeTypeMine = CGitDiff::NewSubmodule;
			}
			else
				theirsSubject.LoadString(IDS_CONFLICT_SUBMODULENOTINITIALIZED);
			if (localIsFile)
			{
				mineSubject.LoadString(IDS_CONFLICT_NOTASUBMODULE);
				changeTypeTheirs = CGitDiff::NewSubmodule;
			}
			else
				mineSubject.LoadString(IDS_CONFLICT_SUBMODULENOTINITIALIZED);
		}
		else if (baseHash != GIT_REV_ZERO && (localHash == GIT_REV_ZERO || remoteHash == GIT_REV_ZERO))
		{
			baseSubject.LoadString(IDS_CONFLICT_SUBMODULENOTINITIALIZED);
			if (localHash == GIT_REV_ZERO)
			{
				mineSubject.LoadString(IDS_CONFLICT_SUBMODULENOTINITIALIZED);
				changeTypeMine = CGitDiff::DeleteSubmodule;
			}
			else
			{
				mineSubject.LoadString(IDS_CONFLICT_SUBMODULENOTINITIALIZED);
				if (localHash == baseHash)
					changeTypeMine = CGitDiff::Identical;
			}
			if (remoteHash == GIT_REV_ZERO)
			{
				theirsSubject.LoadString(IDS_CONFLICT_SUBMODULENOTINITIALIZED);
				changeTypeTheirs = CGitDiff::DeleteSubmodule;
			}
			else
			{
				theirsSubject.LoadString(IDS_CONFLICT_SUBMODULENOTINITIALIZED);
				if (remoteHash == baseHash)
					changeTypeTheirs = CGitDiff::Identical;
			}
		}
		else
			return FALSE;

		CSubmoduleResolveConflictDlg resolveSubmoduleConflictDialog;
		resolveSubmoduleConflictDialog.SetDiff(merge.GetGitPathString(), isRebase, baseTitle, mineTitle, theirsTitle, baseHash, baseSubject, baseOK, localHash, mineSubject, mineOK, changeTypeMine, remoteHash, theirsSubject, theirsOK, changeTypeTheirs);
		resolveSubmoduleConflictDialog.DoModal();
		if (resolveSubmoduleConflictDialog.m_bResolved && resolveMsgHwnd)
		{
			static UINT WM_REVERTMSG = RegisterWindowMessage(L"GITSLNM_NEEDSREFRESH");
			::PostMessage(resolveMsgHwnd, WM_REVERTMSG, NULL, NULL);
		}

		return TRUE;
	}

	CTGitPathList list;
	if (list.ParserFromLsFile(vector))
	{
		MessageBox(nullptr, L"Parse ls-files failed!", L"TortoiseGit", MB_OK | MB_ICONERROR);
		return FALSE;
	}

	if (list.IsEmpty())
		return FALSE;

	CTGitPath theirs;
	CTGitPath mine;
	CTGitPath base;

	if (isRebase)
	{
		mine.SetFromGit(GetMergeTempFile(L"REMOTE", merge));
		theirs.SetFromGit(GetMergeTempFile(L"LOCAL", merge));
	}
	else
	{
		mine.SetFromGit(GetMergeTempFile(L"LOCAL", merge));
		theirs.SetFromGit(GetMergeTempFile(L"REMOTE", merge));
	}
	base.SetFromGit(GetMergeTempFile(L"BASE",merge));

	CString format = L"git.exe checkout-index --temp --stage=%d -- \"%s\"";
	CFile tempfile;
	//create a empty file, incase stage is not three
	tempfile.Open(mine.GetWinPathString(),CFile::modeCreate|CFile::modeReadWrite);
	tempfile.Close();
	tempfile.Open(theirs.GetWinPathString(),CFile::modeCreate|CFile::modeReadWrite);
	tempfile.Close();
	tempfile.Open(base.GetWinPathString(),CFile::modeCreate|CFile::modeReadWrite);
	tempfile.Close();

	bool b_base=false, b_local=false, b_remote=false;

	for (int i = 0; i< list.GetCount(); ++i)
	{
		CString outfile;
		cmd.Empty();
		outfile.Empty();

		if( list[i].m_Stage == 1)
		{
			cmd.Format(format, list[i].m_Stage, (LPCTSTR)list[i].GetGitPathString());
			b_base = true;
			outfile = base.GetWinPathString();

		}
		if( list[i].m_Stage == 2 )
		{
			cmd.Format(format, list[i].m_Stage, (LPCTSTR)list[i].GetGitPathString());
			b_local = true;
			outfile = mine.GetWinPathString();

		}
		if( list[i].m_Stage == 3 )
		{
			cmd.Format(format, list[i].m_Stage, (LPCTSTR)list[i].GetGitPathString());
			b_remote = true;
			outfile = theirs.GetWinPathString();
		}
		CString output, err;
		if(!outfile.IsEmpty())
			if (!g_Git.Run(cmd, &output, &err, CP_UTF8))
			{
				CString file;
				int start =0 ;
				file = output.Tokenize(L"\t", start);
				::MoveFileEx(file,outfile,MOVEFILE_REPLACE_EXISTING|MOVEFILE_COPY_ALLOWED);
			}
			else
				CMessageBox::Show(nullptr, output + L'\n' + err, L"TortoiseGit", MB_OK | MB_ICONERROR);
	}

	if(b_local && b_remote )
	{
		merge.SetFromWin(g_Git.CombinePath(merge));
		if (isRebase)
			return !!CAppUtils::StartExtMerge(bAlternativeTool, base, mine, theirs, merge, baseTitle, mineTitle, theirsTitle, CString(), false, resolveMsgHwnd, true);

		return !!CAppUtils::StartExtMerge(bAlternativeTool, base, theirs, mine, merge, baseTitle, theirsTitle, mineTitle, CString(), false, resolveMsgHwnd, true);
	}
	else
	{
		::DeleteFile(mine.GetWinPathString());
		::DeleteFile(theirs.GetWinPathString());
		if (!b_base)
			::DeleteFile(base.GetWinPathString());

		SCOPE_EXIT{
			if (b_base)
				::DeleteFile(base.GetWinPathString());
		};

		CDeleteConflictDlg dlg;
		if (!isRebase)
		{
			DescribeConflictFile(b_local, b_base, dlg.m_LocalStatus);
			DescribeConflictFile(b_remote, b_base, dlg.m_RemoteStatus);
			dlg.m_LocalHash = mineTitle;
			dlg.m_RemoteHash = theirsTitle;
			dlg.m_bDiffMine = b_local;
		}
		else
		{
			DescribeConflictFile(b_local, b_base, dlg.m_RemoteStatus);
			DescribeConflictFile(b_remote, b_base, dlg.m_LocalStatus);
			dlg.m_LocalHash = theirsTitle;
			dlg.m_RemoteHash = mineTitle;
			dlg.m_bDiffMine = !b_local;
		}
		dlg.m_bShowModifiedButton = b_base;
		dlg.m_File = merge;
		dlg.m_FileBaseVersion = base;
		if(dlg.DoModal() == IDOK)
		{
			CString out;
			if(dlg.m_bIsDelete)
				cmd.Format(L"git.exe rm -- \"%s\"", (LPCTSTR)merge.GetGitPathString());
			else
				cmd.Format(L"git.exe add -- \"%s\"", (LPCTSTR)merge.GetGitPathString());

			if (g_Git.Run(cmd, &out, CP_UTF8))
			{
				MessageBox(nullptr, out, L"TortoiseGit", MB_OK | MB_ICONERROR);
				return FALSE;
			}
			if (!dlg.m_bIsDelete)
			{
				path.m_Action |= CTGitPath::LOGACTIONS_ADDED;
				path.m_Action &= ~CTGitPath::LOGACTIONS_UNMERGED;
			}
			return TRUE;
		}
		return FALSE;
	}
}

bool CAppUtils::IsSSHPutty()
{
	CString sshclient = g_Git.m_Environment.GetEnv(L"GIT_SSH");
	sshclient=sshclient.MakeLower();
	return sshclient.Find(L"plink", 0) >= 0;
}

CString CAppUtils::GetClipboardLink(const CString &skipGitPrefix, int paramsCount)
{
	if (!OpenClipboard(nullptr))
		return CString();

	CString sClipboardText;
	HGLOBAL hglb = GetClipboardData(CF_TEXT);
	if (hglb)
	{
		LPCSTR lpstr = (LPCSTR)GlobalLock(hglb);
		sClipboardText = CString(lpstr);
		GlobalUnlock(hglb);
	}
	hglb = GetClipboardData(CF_UNICODETEXT);
	if (hglb)
	{
		LPCTSTR lpstr = (LPCTSTR)GlobalLock(hglb);
		sClipboardText = lpstr;
		GlobalUnlock(hglb);
	}
	CloseClipboard();

	if(!sClipboardText.IsEmpty())
	{
		if (sClipboardText[0] == L'"' && sClipboardText[sClipboardText.GetLength() - 1] == L'"')
			sClipboardText=sClipboardText.Mid(1,sClipboardText.GetLength()-2);

		for (const CString& prefix : { L"http://", L"https://", L"git://", L"ssh://", L"git@" })
		{
			if (CStringUtils::StartsWith(sClipboardText, prefix) && sClipboardText.GetLength() != prefix.GetLength())
				return sClipboardText;
		}

		if(sClipboardText.GetLength()>=2)
			if (sClipboardText[1] == L':')
				if( (sClipboardText[0] >= 'A' &&  sClipboardText[0] <= 'Z')
					|| (sClipboardText[0] >= 'a' &&  sClipboardText[0] <= 'z') )
					return sClipboardText;

		// trim prefixes like "git clone "
		if (!skipGitPrefix.IsEmpty() && CStringUtils::StartsWith(sClipboardText, skipGitPrefix))
		{
			sClipboardText = sClipboardText.Mid(skipGitPrefix.GetLength()).Trim();
			int spacePos = -1;
			while (paramsCount >= 0)
			{
				--paramsCount;
				spacePos = sClipboardText.Find(L' ', spacePos + 1);
				if (spacePos == -1)
					break;
			}
			if (spacePos > 0 && paramsCount < 0)
				sClipboardText.Truncate(spacePos);
			return sClipboardText;
		}
	}

	return CString();
}

CString CAppUtils::ChooseRepository(const CString* path)
{
	CBrowseFolder browseFolder;
	CRegString regLastResopitory = CRegString(L"Software\\TortoiseGit\\TortoiseProc\\LastRepo", L"");

	browseFolder.m_style = BIF_EDITBOX | BIF_NEWDIALOGSTYLE | BIF_RETURNFSANCESTORS | BIF_RETURNONLYFSDIRS;
	CString strCloneDirectory;
	if(path)
		strCloneDirectory=*path;
	else
		strCloneDirectory = regLastResopitory;

	CString title;
	title.LoadString(IDS_CHOOSE_REPOSITORY);

	browseFolder.SetInfo(title);

	if (browseFolder.Show(nullptr, strCloneDirectory) == CBrowseFolder::OK)
	{
		regLastResopitory = strCloneDirectory;
		return strCloneDirectory;
	}
	else
		return CString();
}

bool CAppUtils::SendPatchMail(CTGitPathList& list, bool bIsMainWnd)
{
	CSendMailDlg dlg;

	dlg.m_PathList  = list;

	if(dlg.DoModal()==IDOK)
	{
		if (dlg.m_PathList.IsEmpty())
			return FALSE;

		CGitProgressDlg progDlg;
		if (bIsMainWnd)
			theApp.m_pMainWnd = &progDlg;
		SendMailProgressCommand sendMailProgressCommand;
		progDlg.SetCommand(&sendMailProgressCommand);

		sendMailProgressCommand.SetPathList(dlg.m_PathList);
		progDlg.SetItemCount(dlg.m_PathList.GetCount());

		CSendMailPatch sendMailPatch(dlg.m_To, dlg.m_CC, dlg.m_Subject, !!dlg.m_bAttachment, !!dlg.m_bCombine);
		sendMailProgressCommand.SetSendMailOption(&sendMailPatch);

		progDlg.DoModal();

		return true;
	}
	return false;
}

bool CAppUtils::SendPatchMail(const CString& cmd, const CString& formatpatchoutput, bool bIsMainWnd)
{
	CTGitPathList list;
	CString log=formatpatchoutput;
	int start=log.Find(cmd);
	if(start >=0)
		log.Tokenize(L"\n", start);
	else
		start = 0;

	while(start>=0)
	{
		CString one = log.Tokenize(L"\n", start);
		one=one.Trim();
		if (one.IsEmpty() || CStringUtils::StartsWith(one, CString(MAKEINTRESOURCE(IDS_SUCCESS))))
			continue;
		one.Replace(L'/', L'\\');
		CTGitPath path;
		path.SetFromWin(one);
		list.AddPath(path);
	}
	if (!list.IsEmpty())
		return SendPatchMail(list, bIsMainWnd);
	else
	{
		CMessageBox::Show(nullptr, IDS_ERR_NOPATCHES, IDS_APPNAME, MB_ICONINFORMATION);
		return true;
	}
}


int CAppUtils::GetLogOutputEncode(CGit *pGit)
{
	CString output;
	output = pGit->GetConfigValue(L"i18n.logOutputEncoding");
	if(output.IsEmpty())
		return CUnicodeUtils::GetCPCode(pGit->GetConfigValue(L"i18n.commitencoding"));
	else
		return CUnicodeUtils::GetCPCode(output);
}
int CAppUtils::SaveCommitUnicodeFile(const CString& filename, CString &message)
{
	try
	{
		CFile file(filename, CFile::modeReadWrite | CFile::modeCreate);
		int cp = CUnicodeUtils::GetCPCode(g_Git.GetConfigValue(L"i18n.commitencoding"));

		bool stripComments = (CRegDWORD(L"Software\\TortoiseGit\\StripCommentedLines", FALSE) == TRUE);
		TCHAR commentChar = L'#';
		if (stripComments)
		{
			CString commentCharValue = g_Git.GetConfigValue(L"core.commentchar");
			if (!commentCharValue.IsEmpty())
				commentChar = commentCharValue[0];
		}

		bool sanitize = (CRegDWORD(L"Software\\TortoiseGit\\SanitizeCommitMsg", TRUE) == TRUE);
		if (sanitize)
			message.Trim(L" \r\n");

		int len = message.GetLength();
		int start = 0;
		int emptyLineCnt = 0;
		while (start >= 0 && start < len)
		{
			int oldStart = start;
			start = message.Find(L'\n', oldStart);
			CString line = message.Mid(oldStart);
			if (start != -1)
			{
				line.Truncate(start - oldStart);
				++start; // move forward so we don't find the same char again
			}
			if (stripComments && (!line.IsEmpty() && line.GetAt(0) == commentChar) || (start < 0 && line.IsEmpty()))
				continue;
			line.TrimRight(L" \r");
			if (sanitize)
			{
				if (line.IsEmpty())
				{
					++emptyLineCnt;
					continue;
				}
				if (emptyLineCnt) // squash multiple newlines
					file.Write("\n", 1);
				emptyLineCnt = 0;
			}
			CStringA lineA = CUnicodeUtils::GetMulti(line + L'\n', cp);
			file.Write((LPCSTR)lineA, lineA.GetLength());
		}
		file.Close();
		return 0;
	}
	catch (CFileException *e)
	{
		e->Delete();
		return -1;
	}
}

bool DoPull(const CString& url, bool bAutoLoad, BOOL bFetchTags, bool bNoFF, bool bFFonly, bool bSquash, bool bNoCommit, int* nDepth, BOOL bPrune, const CString& remoteBranchName, bool showPush, bool showStashPop, bool bUnrelated)
{
	if (bAutoLoad)
		CAppUtils::LaunchPAgent(nullptr, &url);

	CGitHash hashOld;
	if (g_Git.GetHash(hashOld, L"HEAD"))
	{
		MessageBox(nullptr, g_Git.GetGitLastErr(L"Could not get HEAD hash."), L"TortoiseGit", MB_ICONERROR);
		return false;
	}

	CString args;
	if (CRegDWORD(L"Software\\TortoiseGit\\PullRebaseBehaviorLike1816", FALSE) == FALSE)
		args += L" --no-rebase";

	if (bFetchTags == BST_UNCHECKED)
		args += L" --no-tags";
	else if (bFetchTags == BST_CHECKED)
		args += L" --tags";

	if (bNoFF)
		args += L" --no-ff";

	if (bFFonly)
		args += L" --ff-only";

	if (bSquash)
		args += L" --squash";

	if (bNoCommit)
		args += L" --no-commit";

	if (nDepth)
		args.AppendFormat(L" --depth %d", *nDepth);

	if (bPrune == BST_CHECKED)
		args += L" --prune";
	else if (bPrune == BST_UNCHECKED)
		args += L" --no-prune";

	if (bUnrelated)
		args += L" --allow-unrelated-histories";

	CString cmd;
	cmd.Format(L"git.exe pull --progress -v%s \"%s\" %s", (LPCTSTR)args, (LPCTSTR)url, (LPCTSTR)remoteBranchName);
	CProgressDlg progress;
	progress.m_GitCmd = cmd;

	CGitHash hashNew; // declare outside lambda, because it is captured by reference
	progress.m_PostCmdCallback = [&](DWORD status, PostCmdList& postCmdList)
	{
		if (status)
		{
			if (CAppUtils::IsGitVersionNewerOrEqual(2, 9))
			{
				STRING_VECTOR remotes;
				g_Git.GetRemoteList(remotes);
				if (std::find(remotes.begin(), remotes.end(), url) != remotes.end())
				{
					CString currentBranch;
					if (g_Git.GetCurrentBranchFromFile(g_Git.m_CurrentDir, currentBranch))
						currentBranch.Empty();
					CString remoteRef = L"remotes/" + url + L"/" + remoteBranchName;
					if (!currentBranch.IsEmpty() && remoteBranchName.IsEmpty())
					{
						CString pullRemote, pullBranch;
						g_Git.GetRemoteTrackedBranch(currentBranch, pullRemote, pullBranch);
						if (!pullRemote.IsEmpty() && !pullBranch.IsEmpty())
							remoteRef = L"remotes/" + pullRemote + L"/" + pullBranch;
					}
					CGitHash common;
					g_Git.IsFastForward(L"HEAD", remoteRef, &common);
					if (common.IsEmpty())
						postCmdList.emplace_back(IDI_MERGE, IDS_MERGE_UNRELATED, [=] { DoPull(url, bAutoLoad, bFetchTags, bNoFF, bFFonly, bSquash, bNoCommit, nDepth, bPrune, remoteBranchName, showPush, showStashPop, true); });
				}
			}

			postCmdList.emplace_back(IDI_PULL, IDS_MENUPULL, [&]{ CAppUtils::Pull(); });
			postCmdList.emplace_back(IDI_COMMIT, IDS_MENUSTASHSAVE, [&]{ CAppUtils::StashSave(L"", true); });
			return;
		}

		if (showStashPop)
			postCmdList.emplace_back(IDI_RELOCATE, IDS_MENUSTASHPOP, []{ CAppUtils::StashPop(); });

		if (g_Git.GetHash(hashNew, L"HEAD"))
			MessageBox(nullptr, g_Git.GetGitLastErr(L"Could not get HEAD hash after pulling."), L"TortoiseGit", MB_ICONERROR);
		else
		{
			postCmdList.emplace_back(IDI_DIFF, IDS_PROC_PULL_DIFFS, [&]
			{
				CString sCmd;
				sCmd.Format(L"/command:showcompare /path:\"%s\" /revision1:%s /revision2:%s", (LPCTSTR)g_Git.m_CurrentDir, (LPCTSTR)hashOld.ToString(), (LPCTSTR)hashNew.ToString());
				CAppUtils::RunTortoiseGitProc(sCmd);
			});
			postCmdList.emplace_back(IDI_LOG, IDS_PROC_PULL_LOG, [&]
			{
				CString sCmd;
				sCmd.Format(L"/command:log /path:\"%s\" /range:%s", (LPCTSTR)g_Git.m_CurrentDir, (LPCTSTR)(hashOld.ToString() + L".." + hashNew.ToString()));
				CAppUtils::RunTortoiseGitProc(sCmd);
			});
		}

		if (showPush)
			postCmdList.emplace_back(IDI_PUSH, IDS_MENUPUSH, []{ CAppUtils::Push(); });

		CTGitPath gitPath = g_Git.m_CurrentDir;
		if (gitPath.HasSubmodules())
		{
			postCmdList.emplace_back(IDI_UPDATE, IDS_PROC_SUBMODULESUPDATE, []
			{
				CString sCmd;
				sCmd.Format(L"/command:subupdate /bkpath:\"%s\"", (LPCTSTR)g_Git.m_CurrentDir);
				CAppUtils::RunTortoiseGitProc(sCmd);
			});
		}
	};

	INT_PTR ret = progress.DoModal();

	if (ret == IDOK && progress.m_GitStatus == 1 && progress.m_LogText.Find(L"CONFLICT") >= 0 && CMessageBox::Show(nullptr, IDS_SEECHANGES, IDS_APPNAME, MB_YESNO | MB_ICONINFORMATION) == IDYES)
	{
		cmd.Format(L"/command:repostatus /path:\"%s\"", (LPCTSTR)g_Git.m_CurrentDir);
		CAppUtils::RunTortoiseGitProc(cmd);

		return true;
	}

	return ret == IDOK;
}

bool CAppUtils::Pull(bool showPush, bool showStashPop)
{
	if (IsTGitRebaseActive())
		return false;

	CPullFetchDlg dlg;
	dlg.m_IsPull = TRUE;
	if (dlg.DoModal() == IDOK)
	{
		// "git.exe pull --rebase" is not supported, never and ever. So, adapting it to Fetch & Rebase.
		if (dlg.m_bRebase)
			return DoFetch(dlg.m_RemoteURL,
							FALSE, // Fetch all remotes
							dlg.m_bAutoLoad == BST_CHECKED,
							dlg.m_bPrune,
							dlg.m_bDepth == BST_CHECKED,
							dlg.m_nDepth,
							dlg.m_bFetchTags,
							dlg.m_RemoteBranchName,
							dlg.m_bRebaseActivatedInConfigForPull ? 2 : 1, // Rebase after fetching
							dlg.m_bRebasePreserveMerges == TRUE); // Preserve merges on rebase

		return DoPull(dlg.m_RemoteURL, dlg.m_bAutoLoad == BST_CHECKED, dlg.m_bFetchTags, dlg.m_bNoFF == BST_CHECKED, dlg.m_bFFonly == BST_CHECKED, dlg.m_bSquash == BST_CHECKED, dlg.m_bNoCommit == BST_CHECKED, dlg.m_bDepth ? &dlg.m_nDepth : nullptr, dlg.m_bPrune, dlg.m_RemoteBranchName, showPush, showStashPop, false);
	}

	return false;
}

bool CAppUtils::RebaseAfterFetch(const CString& upstream, int rebase, bool preserveMerges)
{
	while (true)
	{
		CRebaseDlg dlg;
		if (!upstream.IsEmpty())
			dlg.m_Upstream = upstream;
		dlg.m_PostButtonTexts.Add(CString(MAKEINTRESOURCE(IDS_MENULOG)));
		dlg.m_PostButtonTexts.Add(CString(MAKEINTRESOURCE(IDS_MENUPUSH)));
		dlg.m_PostButtonTexts.Add(CString(MAKEINTRESOURCE(IDS_MENUDESSENDMAIL)));
		dlg.m_PostButtonTexts.Add(CString(MAKEINTRESOURCE(IDS_MENUREBASE)));
		dlg.m_bRebaseAutoStart = (rebase == 2);
		dlg.m_bPreserveMerges = preserveMerges;
		INT_PTR response = dlg.DoModal();
		if (response == IDOK)
			return true;
		else if (response == IDC_REBASE_POST_BUTTON)
		{
			CString cmd = L"/command:log";
			cmd += L" /path:\"" + g_Git.m_CurrentDir + L'"';
			CAppUtils::RunTortoiseGitProc(cmd);
			return true;
		}
		else if (response == IDC_REBASE_POST_BUTTON + 1)
			return Push();
		else if (response == IDC_REBASE_POST_BUTTON + 2)
		{
			CString cmd, out, err;
			cmd.Format(L"git.exe format-patch -o \"%s\" %s..%s",
				(LPCTSTR)g_Git.m_CurrentDir,
				(LPCTSTR)g_Git.FixBranchName(dlg.m_Upstream),
				(LPCTSTR)g_Git.FixBranchName(dlg.m_Branch));
			if (g_Git.Run(cmd, &out, &err, CP_UTF8))
			{
				CMessageBox::Show(nullptr, out + L'\n' + err, L"TortoiseGit", MB_OK | MB_ICONERROR);
				return false;
			}
			CAppUtils::SendPatchMail(cmd, out);
			return true;
		}
		else if (response == IDC_REBASE_POST_BUTTON + 3)
			continue;
		else if (response == IDCANCEL)
			return false;
		return false;
	}
}

static bool DoFetch(const CString& url, const bool fetchAllRemotes, const bool loadPuttyAgent, const int prune, const bool bDepth, const int nDepth, const int fetchTags, const CString& remoteBranch, int runRebase, const bool rebasePreserveMerges)
{
	if (loadPuttyAgent)
	{
		if (fetchAllRemotes)
		{
			STRING_VECTOR list;
			g_Git.GetRemoteList(list);

			for (const auto& remote : list)
				CAppUtils::LaunchPAgent(nullptr, &remote);
		}
		else
			CAppUtils::LaunchPAgent(nullptr, &url);
	}

	CString upstream = L"FETCH_HEAD";
	CGitHash oldUpstreamHash;
	if (runRebase)
	{
		STRING_VECTOR list;
		g_Git.GetRemoteList(list);
		for (auto it = list.cbegin(); it != list.cend(); ++it)
		{
			if (url == *it)
			{
				upstream.Empty();
				if (remoteBranch.IsEmpty()) // pulldlg might clear remote branch if its the default tracked branch
				{
					CString currentBranch;
					if (g_Git.GetCurrentBranchFromFile(g_Git.m_CurrentDir, currentBranch))
						currentBranch.Empty();
					if (!currentBranch.IsEmpty() && remoteBranch.IsEmpty())
					{
						CString pullRemote, pullBranch;
						g_Git.GetRemoteTrackedBranch(currentBranch, pullRemote, pullBranch);
						if (!pullRemote.IsEmpty() && !pullBranch.IsEmpty() && pullRemote == url) // pullRemote == url is just another safety-check and should not be needed
							upstream = L"remotes/" + *it + L'/' + pullBranch;
					}
				}
				else
					upstream = L"remotes/" + *it + L'/' + remoteBranch;

				g_Git.GetHash(oldUpstreamHash, upstream);
				break;
			}
		}
	}

	CString cmd, arg;
	arg = L" --progress";

	if (bDepth)
		arg.AppendFormat(L" --depth %d", nDepth);

	if (prune == TRUE)
		arg += L" --prune";
	else if (prune == FALSE)
		arg += L" --no-prune";

	if (fetchTags == 1)
		arg += L" --tags";
	else if (fetchTags == 0)
		arg += L" --no-tags";

	if (fetchAllRemotes)
		cmd.Format(L"git.exe fetch --all -v%s", (LPCTSTR)arg);
	else
		cmd.Format(L"git.exe fetch -v%s \"%s\" %s", (LPCTSTR)arg, (LPCTSTR)url, (LPCTSTR)remoteBranch);

	CProgressDlg progress;
	progress.m_PostCmdCallback = [&](DWORD status, PostCmdList& postCmdList)
	{
		if (status)
		{
			postCmdList.emplace_back(IDI_REFRESH, IDS_MSGBOX_RETRY, [&]{ DoFetch(url, fetchAllRemotes, loadPuttyAgent, prune, bDepth, nDepth, fetchTags, remoteBranch, runRebase, rebasePreserveMerges); });
			if (fetchAllRemotes)
				postCmdList.emplace_back(IDI_LOG, IDS_MENULOG, []
				{
					CString cmd = L"/command:log";
					cmd += L" /path:\"" + g_Git.m_CurrentDir + L'"';
					CAppUtils::RunTortoiseGitProc(cmd);
				});
			return;
		}

		postCmdList.emplace_back(IDI_LOG, IDS_MENULOG, []
		{
			CString cmd = L"/command:log";
			cmd += L" /path:\"" + g_Git.m_CurrentDir + L'"';
			CAppUtils::RunTortoiseGitProc(cmd);
		});

		postCmdList.emplace_back(IDI_REVERT, IDS_PROC_RESET, []
		{
			CString pullRemote, pullBranch;
			g_Git.GetRemoteTrackedBranchForHEAD(pullRemote, pullBranch);
			CString defaultUpstream;
			if (!pullRemote.IsEmpty() && !pullBranch.IsEmpty())
				defaultUpstream.Format(L"remotes/%s/%s", (LPCTSTR)pullRemote, (LPCTSTR)pullBranch);
			CAppUtils::GitReset(&defaultUpstream, 2);
		});

		postCmdList.emplace_back(IDI_PULL, IDS_MENUFETCH, []{ CAppUtils::Fetch(); });

		if (!runRebase && !GitAdminDir::IsBareRepo(g_Git.m_CurrentDir))
			postCmdList.emplace_back(IDI_REBASE, IDS_MENUREBASE, [&]{ runRebase = false; CAppUtils::RebaseAfterFetch(); });
	};

	progress.m_GitCmd = cmd;

	if (g_Git.UsingLibGit2(CGit::GIT_CMD_FETCH))
	{
		CGitProgressDlg gitdlg;
		FetchProgressCommand fetchProgressCommand;
		if (!fetchAllRemotes)
			fetchProgressCommand.SetUrl(url);
		gitdlg.SetCommand(&fetchProgressCommand);
		fetchProgressCommand.m_PostCmdCallback = progress.m_PostCmdCallback;
		fetchProgressCommand.SetAutoTag(fetchTags == 1 ? GIT_REMOTE_DOWNLOAD_TAGS_ALL : fetchTags == 2 ? GIT_REMOTE_DOWNLOAD_TAGS_AUTO : GIT_REMOTE_DOWNLOAD_TAGS_NONE);
		fetchProgressCommand.SetPrune(prune == BST_CHECKED ? GIT_FETCH_PRUNE : prune == BST_INDETERMINATE ? GIT_FETCH_PRUNE_UNSPECIFIED : GIT_FETCH_NO_PRUNE);
		if (!fetchAllRemotes)
			fetchProgressCommand.SetRefSpec(remoteBranch);
		return gitdlg.DoModal() == IDOK;
	}

	progress.m_PostExecCallback = [&](DWORD& exitCode, CString&)
	{
		if (exitCode || !runRebase)
			return;

		CGitHash remoteBranchHash;
		g_Git.GetHash(remoteBranchHash, upstream);

		if (runRebase == 1)
		{
			CGitHash headHash, commonAcestor;
			if (!g_Git.GetHash(headHash, L"HEAD") && (remoteBranchHash == headHash || (g_Git.IsFastForward(upstream, L"HEAD", &commonAcestor) && commonAcestor == remoteBranchHash)) && CMessageBox::ShowCheck(nullptr, IDS_REBASE_CURRENTBRANCHUPTODATE, IDS_APPNAME, MB_ICONQUESTION | MB_YESNO | MB_DEFBUTTON2, L"OpenRebaseRemoteBranchEqualsHEAD", IDS_MSGBOX_DONOTSHOWAGAIN) == IDNO)
				return;

			if (remoteBranchHash == oldUpstreamHash && !oldUpstreamHash.IsEmpty() && CMessageBox::ShowCheck(nullptr, IDS_REBASE_BRANCH_UNCHANGED, IDS_APPNAME, MB_ICONQUESTION | MB_YESNO | MB_DEFBUTTON2, L"OpenRebaseRemoteBranchUnchanged", IDS_MSGBOX_DONOTSHOWAGAIN) == IDNO)
				return;
		}

		if (runRebase == 1 && g_Git.IsFastForward(L"HEAD", upstream))
		{
			UINT ret = CMessageBox::ShowCheck(nullptr, IDS_REBASE_BRANCH_FF, IDS_APPNAME, 2, IDI_QUESTION, IDS_MERGEBUTTON, IDS_REBASEBUTTON, IDS_ABORTBUTTON, L"OpenRebaseRemoteBranchFastForwards", IDS_MSGBOX_DONOTSHOWAGAIN);
			if (ret == 3)
				return;
			if (ret == 1)
			{
				CProgressDlg mergeProgress;
				mergeProgress.m_GitCmd = L"git.exe merge --ff-only " + upstream;
				mergeProgress.m_AutoClose = AUTOCLOSE_IF_NO_ERRORS;
				mergeProgress.m_PostCmdCallback = [](DWORD status, PostCmdList& postCmdList)
				{
					if (status && g_Git.HasWorkingTreeConflicts())
					{
						// there are conflict files
						postCmdList.emplace_back(IDI_RESOLVE, IDS_PROGRS_CMD_RESOLVE, []
						{
							CString sCmd;
							sCmd.Format(L"/command:commit /path:\"%s\"", (LPCTSTR)g_Git.m_CurrentDir);
							CAppUtils::RunTortoiseGitProc(sCmd);
						});
					}
				};
				mergeProgress.DoModal();
				return;
			}
		}

		CAppUtils::RebaseAfterFetch(upstream, runRebase, rebasePreserveMerges);
	};

	return progress.DoModal() == IDOK;
}

bool CAppUtils::Fetch(const CString& remoteName, bool allRemotes)
{
	CPullFetchDlg dlg;
	dlg.m_PreSelectRemote = remoteName;
	dlg.m_IsPull=FALSE;
	dlg.m_bAllRemotes = allRemotes;

	if(dlg.DoModal()==IDOK)
		return DoFetch(dlg.m_RemoteURL, dlg.m_bAllRemotes == BST_CHECKED, dlg.m_bAutoLoad == BST_CHECKED, dlg.m_bPrune, dlg.m_bDepth == BST_CHECKED, dlg.m_nDepth, dlg.m_bFetchTags, dlg.m_RemoteBranchName, dlg.m_bRebase == BST_CHECKED ? 1 : 0, FALSE);

	return false;
}

bool CAppUtils::DoPush(bool autoloadKey, bool pack, bool tags, bool allRemotes, bool allBranches, bool force, bool forceWithLease, const CString& localBranch, const CString& remote, const CString& remoteBranch, bool setUpstream, int recurseSubmodules)
{
	CString error;
	DWORD exitcode = 0xFFFFFFFF;
	if (CHooks::Instance().PrePush(g_Git.m_CurrentDir, exitcode, error))
	{
		if (exitcode)
		{
			CString temp;
			temp.Format(IDS_ERR_HOOKFAILED, (LPCTSTR)error);
			MessageBox(nullptr, temp, L"TortoiseGit", MB_OK | MB_ICONERROR);
			return false;
		}
	}

	int iRecurseSubmodules = 0;
	if (IsGitVersionNewerOrEqual(2, 7))
	{
		CString sRecurseSubmodules = g_Git.GetConfigValue(L"push.recurseSubmodules");
		if (sRecurseSubmodules == L"check")
			iRecurseSubmodules = 1;
		else if (sRecurseSubmodules == L"on-demand")
			iRecurseSubmodules = 2;
	}

	CString arg;
	if (pack)
		arg += L"--thin ";
	if (tags && !allBranches)
		arg += L"--tags ";
	if (force)
		arg += L"--force ";
	if (forceWithLease)
		arg += L"--force-with-lease ";
	if (setUpstream)
		arg += L"--set-upstream ";
	if (recurseSubmodules == 0 && recurseSubmodules != iRecurseSubmodules)
		arg += L"--recurse-submodules=no ";
	if (recurseSubmodules == 1 && recurseSubmodules != iRecurseSubmodules)
		arg += L"--recurse-submodules=check ";
	if (recurseSubmodules == 2 && recurseSubmodules != iRecurseSubmodules)
		arg += L"--recurse-submodules=on-demand ";

	arg += L"--progress ";

	CProgressDlg progress;

	STRING_VECTOR remotesList;
	if (allRemotes)
		g_Git.GetRemoteList(remotesList);
	else
		remotesList.push_back(remote);

	for (unsigned int i = 0; i < remotesList.size(); ++i)
	{
		if (autoloadKey)
			CAppUtils::LaunchPAgent(nullptr, &remotesList[i]);

		CString cmd;
		if (allBranches)
		{
			cmd.Format(L"git.exe push --all %s\"%s\"",
				(LPCTSTR)arg,
				(LPCTSTR)remotesList[i]);

			if (tags)
			{
				progress.m_GitCmdList.push_back(cmd);
				cmd.Format(L"git.exe push --tags %s\"%s\"", (LPCTSTR)arg, (LPCTSTR)remotesList[i]);
			}
		}
		else
		{
			cmd.Format(L"git.exe push %s\"%s\" %s",
				(LPCTSTR)arg,
				(LPCTSTR)remotesList[i],
				(LPCTSTR)localBranch);
			if (!remoteBranch.IsEmpty())
			{
				cmd += L":";
				cmd += remoteBranch;
			}
		}
		progress.m_GitCmdList.push_back(cmd);

		if (!allBranches && !!CRegDWORD(L"Software\\TortoiseGit\\ShowBranchRevisionNumber", FALSE))
		{
			cmd.Format(L"git.exe rev-list --count --first-parent %s", (LPCTSTR)localBranch);
			progress.m_GitCmdList.push_back(cmd);
		}
	}

	CString superprojectRoot;
	GitAdminDir::HasAdminDir(g_Git.m_CurrentDir, false, &superprojectRoot);
	progress.m_PostCmdCallback = [&](DWORD status, PostCmdList& postCmdList)
	{
		// need to execute hooks as those might be needed by post action commands
		DWORD exitcode = 0xFFFFFFFF;
		CString error;
		if (CHooks::Instance().PostPush(g_Git.m_CurrentDir, exitcode, error))
		{
			if (exitcode)
			{
				CString temp;
				temp.Format(IDS_ERR_HOOKFAILED, (LPCTSTR)error);
				MessageBox(nullptr, temp, L"TortoiseGit", MB_OK | MB_ICONERROR);
			}
		}

		if (status)
		{
			bool rejected = progress.GetLogText().Find(L"! [rejected]") > 0;
			if (rejected)
			{
				postCmdList.emplace_back(IDI_PULL, IDS_MENUPULL, []{ Pull(true); });
				postCmdList.emplace_back(IDI_PULL, IDS_MENUFETCH, [&]{ Fetch(allRemotes ? L"" : remote, allRemotes); });
			}
			postCmdList.emplace_back(IDI_PUSH, IDS_MENUPUSH, [&]{ Push(localBranch); });
			return;
		}

		postCmdList.emplace_back(IDS_PROC_REQUESTPULL, [&]{ RequestPull(remoteBranch); });
		postCmdList.emplace_back(IDI_PUSH, IDS_MENUPUSH, [&]{ Push(localBranch); });
		postCmdList.emplace_back(IDI_SWITCH, IDS_MENUSWITCH, [&]{ Switch(); });
		if (!superprojectRoot.IsEmpty())
		{
			postCmdList.emplace_back(IDI_COMMIT, IDS_PROC_COMMIT_SUPERPROJECT, [&]
			{
				CString sCmd;
				sCmd.Format(L"/command:commit /path:\"%s\"", (LPCTSTR)superprojectRoot);
				RunTortoiseGitProc(sCmd);
			});
		}
	};

	INT_PTR ret = progress.DoModal();
	return ret == IDOK;
}

bool CAppUtils::Push(const CString& selectLocalBranch)
{
	CPushDlg dlg;
	dlg.m_BranchSourceName = selectLocalBranch;

	if (dlg.DoModal() == IDOK)
		return DoPush(!!dlg.m_bAutoLoad, !!dlg.m_bPack, !!dlg.m_bTags, !!dlg.m_bPushAllRemotes, !!dlg.m_bPushAllBranches, !!dlg.m_bForce, !!dlg.m_bForceWithLease, dlg.m_BranchSourceName, dlg.m_URL, dlg.m_BranchRemoteName, !!dlg.m_bSetUpstream, dlg.m_RecurseSubmodules);

	return FALSE;
}

bool CAppUtils::RequestPull(const CString& endrevision, const CString& repositoryUrl, bool bIsMainWnd)
{
	CRequestPullDlg dlg;
	dlg.m_RepositoryURL = repositoryUrl;
	dlg.m_EndRevision = endrevision;
	if (dlg.DoModal()==IDOK)
	{
		CString cmd;
		cmd.Format(L"git.exe request-pull %s \"%s\" %s", (LPCTSTR)dlg.m_StartRevision, (LPCTSTR)dlg.m_RepositoryURL, (LPCTSTR)dlg.m_EndRevision);

		CSysProgressDlg sysProgressDlg;
		sysProgressDlg.SetTitle(CString(MAKEINTRESOURCE(IDS_APPNAME)));
		sysProgressDlg.SetLine(1, CString(MAKEINTRESOURCE(IDS_PROC_CREATINGPULLREUQEST)));
		sysProgressDlg.SetLine(2, CString(MAKEINTRESOURCE(IDS_PROGRESSWAIT)));
		sysProgressDlg.SetShowProgressBar(false);
		sysProgressDlg.ShowModeless((HWND)nullptr, true);

		CString tempFileName = GetTempFile();
		CString err;
		DeleteFile(tempFileName);
		CreateDirectory(tempFileName, nullptr);
		tempFileName += L"\\pullrequest.txt";
		if (g_Git.RunLogFile(cmd, tempFileName, &err))
		{
			CString msg;
			msg.LoadString(IDS_ERR_PULLREUQESTFAILED);
			MessageBox(nullptr, msg + L'\n' + err, L"TortoiseGit", MB_OK | MB_ICONERROR);
			return false;
		}

		if (sysProgressDlg.HasUserCancelled())
		{
			CMessageBox::Show(nullptr, IDS_USERCANCELLED, IDS_APPNAME, MB_OK);
			::DeleteFile(tempFileName);
			return false;
		}

		sysProgressDlg.Stop();

		if (dlg.m_bSendMail)
		{
			CSendMailDlg sendmaildlg;
			sendmaildlg.m_PathList = CTGitPathList(CTGitPath(tempFileName));
			sendmaildlg.m_bCustomSubject = true;

			if (sendmaildlg.DoModal() == IDOK)
			{
				if (sendmaildlg.m_PathList.IsEmpty())
					return FALSE;

				CGitProgressDlg progDlg;
				if (bIsMainWnd)
					theApp.m_pMainWnd = &progDlg;
				SendMailProgressCommand sendMailProgressCommand;
				progDlg.SetCommand(&sendMailProgressCommand);

				sendMailProgressCommand.SetPathList(sendmaildlg.m_PathList);
				progDlg.SetItemCount(sendmaildlg.m_PathList.GetCount());

				CSendMailCombineable sendMailCombineable(sendmaildlg.m_To, sendmaildlg.m_CC, sendmaildlg.m_Subject, !!sendmaildlg.m_bAttachment, !!sendmaildlg.m_bCombine);
				sendMailProgressCommand.SetSendMailOption(&sendMailCombineable);

				progDlg.DoModal();

				return true;
			}
			return false;
		}

		CAppUtils::LaunchAlternativeEditor(tempFileName);
	}
	return true;
}

void CAppUtils::RemoveTrailSlash(CString &path)
{
	if(path.IsEmpty())
		return ;

	// For URL, do not trim the slash just after the host name component.
	int index = path.Find(L"://");
	if (index >= 0)
	{
		index += 4;
		index = path.Find(L'/', index);
		if (index == path.GetLength() - 1)
			return;
	}

	while (path[path.GetLength() - 1] == L'\\' || path[path.GetLength() - 1] == L'/')
	{
		path.Truncate(path.GetLength() - 1);
		if(path.IsEmpty())
			return;
	}
}

bool CAppUtils::CheckUserData()
{
	while(g_Git.GetUserName().IsEmpty() || g_Git.GetUserEmail().IsEmpty())
	{
		if (CMessageBox::Show(nullptr, IDS_PROC_NOUSERDATA, IDS_APPNAME, MB_YESNO | MB_ICONERROR) == IDYES)
		{
			CTGitPath path(g_Git.m_CurrentDir);
			CSettings dlg(IDS_PROC_SETTINGS_TITLE,&path);
			dlg.SetTreeViewMode(TRUE, TRUE, TRUE);
			dlg.SetTreeWidth(220);
			dlg.m_DefaultPage = L"gitconfig";

			dlg.DoModal();
			dlg.HandleRestart();

		}
		else
			return false;
	}

	return true;
}

BOOL CAppUtils::Commit(const CString& bugid, BOOL bWholeProject, CString &sLogMsg,
					CTGitPathList &pathList,
					CTGitPathList &selectedList,
					bool bSelectFilesForCommit)
{
	bool bFailed = true;

	if (!CheckUserData())
		return false;

	while (bFailed)
	{
		bFailed = false;
		CCommitDlg dlg;
		dlg.m_sBugID = bugid;

		dlg.m_bWholeProject = bWholeProject;

		dlg.m_sLogMessage = sLogMsg;
		dlg.m_pathList = pathList;
		dlg.m_checkedPathList = selectedList;
		dlg.m_bSelectFilesForCommit = bSelectFilesForCommit;
		if (dlg.DoModal() == IDOK)
		{
			if (dlg.m_pathList.IsEmpty())
				return false;
			// if the user hasn't changed the list of selected items
			// we don't use that list. Because if we would use the list
			// of pre-checked items, the dialog would show different
			// checked items on the next startup: it would only try
			// to check the parent folder (which might not even show)
			// instead, we simply use an empty list and let the
			// default checking do its job.
			if (!dlg.m_pathList.IsEqual(pathList))
				selectedList = dlg.m_pathList;
			pathList = dlg.m_updatedPathList;
			sLogMsg = dlg.m_sLogMessage;
			bSelectFilesForCommit = true;

			switch (dlg.m_PostCmd)
			{
			case GIT_POSTCOMMIT_CMD_DCOMMIT:
				CAppUtils::SVNDCommit();
				break;
			case GIT_POSTCOMMIT_CMD_PUSH:
				CAppUtils::Push();
				break;
			case GIT_POSTCOMMIT_CMD_CREATETAG:
				CAppUtils::CreateBranchTag(TRUE);
				break;
			case GIT_POSTCOMMIT_CMD_PULL:
				CAppUtils::Pull(true);
				break;
			default:
				break;
			}

//			CGitProgressDlg progDlg;
//			progDlg.SetChangeList(dlg.m_sChangeList, !!dlg.m_bKeepChangeList);
//			if (parser.HasVal(L"closeonend"))
//				progDlg.SetAutoClose(parser.GetLongVal(L"closeonend"));
//			progDlg.SetCommand(CGitProgressDlg::GitProgress_Commit);
//			progDlg.SetPathList(dlg.m_pathList);
//			progDlg.SetCommitMessage(dlg.m_sLogMessage);
//			progDlg.SetDepth(dlg.m_bRecursive ? Git_depth_infinity : svn_depth_empty);
//			progDlg.SetSelectedList(dlg.m_selectedPathList);
//			progDlg.SetItemCount(dlg.m_itemsCount);
//			progDlg.SetBugTraqProvider(dlg.m_BugTraqProvider);
//			progDlg.DoModal();
//			CRegDWORD err = CRegDWORD(L"Software\\TortoiseGit\\ErrorOccurred", FALSE);
//			err = (DWORD)progDlg.DidErrorsOccur();
//			bFailed = progDlg.DidErrorsOccur();
//			bRet = progDlg.DidErrorsOccur();
//			CRegDWORD bFailRepeat = CRegDWORD(L"Software\\TortoiseGit\\CommitReopen", FALSE);
//			if (DWORD(bFailRepeat)==0)
//				bFailed = false;		// do not repeat if the user chose not to in the settings.
		}
	}
	return true;
}

BOOL CAppUtils::SVNDCommit()
{
	CSVNDCommitDlg dcommitdlg;
	CString gitSetting = g_Git.GetConfigValue(L"svn.rmdir");
	if (gitSetting.IsEmpty()) {
		if (dcommitdlg.DoModal() != IDOK)
			return false;
		else
		{
			if (dcommitdlg.m_remember)
			{
				if (dcommitdlg.m_rmdir)
					gitSetting = L"true";
				else
					gitSetting = L"false";
				if (g_Git.SetConfigValue(L"svn.rmdir", gitSetting))
				{
					CString msg;
					msg.Format(IDS_PROC_SAVECONFIGFAILED, L"svn.rmdir", (LPCTSTR)gitSetting);
					MessageBox(nullptr, msg, L"TortoiseGit", MB_OK | MB_ICONERROR);
				}
			}
		}
	}

	BOOL IsStash = false;
	if(!g_Git.CheckCleanWorkTree())
	{
		if (CMessageBox::Show(nullptr, IDS_ERROR_NOCLEAN_STASH, IDS_APPNAME, 1, IDI_QUESTION, IDS_STASHBUTTON, IDS_ABORTBUTTON) == 1)
		{
			CSysProgressDlg sysProgressDlg;
			sysProgressDlg.SetTitle(CString(MAKEINTRESOURCE(IDS_APPNAME)));
			sysProgressDlg.SetLine(1, CString(MAKEINTRESOURCE(IDS_PROC_STASHRUNNING)));
			sysProgressDlg.SetLine(2, CString(MAKEINTRESOURCE(IDS_PROGRESSWAIT)));
			sysProgressDlg.SetShowProgressBar(false);
			sysProgressDlg.SetCancelMsg(IDS_PROGRS_INFOFAILED);
			sysProgressDlg.ShowModeless((HWND)nullptr, true);

			CString out;
			if (g_Git.Run(L"git.exe stash", &out, CP_UTF8))
			{
				sysProgressDlg.Stop();
				MessageBox(nullptr, out, L"TortoiseGit", MB_OK | MB_ICONERROR);
				return false;
			}
			sysProgressDlg.Stop();

			IsStash =true;
		}
		else
			return false;
	}

	CProgressDlg progress;
	if (dcommitdlg.m_rmdir)
		progress.m_GitCmd = L"git.exe svn dcommit --rmdir";
	else
		progress.m_GitCmd = L"git.exe svn dcommit";
	if(progress.DoModal()==IDOK && progress.m_GitStatus == 0)
	{
		if( IsStash)
		{
			if (CMessageBox::Show(nullptr, IDS_DCOMMIT_STASH_POP, IDS_APPNAME, MB_YESNO | MB_ICONINFORMATION) == IDYES)
			{
				CSysProgressDlg sysProgressDlg;
				sysProgressDlg.SetTitle(CString(MAKEINTRESOURCE(IDS_APPNAME)));
				sysProgressDlg.SetLine(1, CString(MAKEINTRESOURCE(IDS_PROC_STASHRUNNING)));
				sysProgressDlg.SetLine(2, CString(MAKEINTRESOURCE(IDS_PROGRESSWAIT)));
				sysProgressDlg.SetShowProgressBar(false);
				sysProgressDlg.SetCancelMsg(IDS_PROGRS_INFOFAILED);
				sysProgressDlg.ShowModeless((HWND)nullptr, true);

				CString out;
				if (g_Git.Run(L"git.exe stash pop", &out, CP_UTF8))
				{
					sysProgressDlg.Stop();
					MessageBox(nullptr, out, L"TortoiseGit", MB_OK | MB_ICONERROR);
					return false;
				}
				sysProgressDlg.Stop();
			}
			else
				return false;
		}
		return TRUE;
	}
	return FALSE;
}

static bool DoMerge(bool noFF, bool ffOnly, bool squash, bool noCommit, const int* log, bool unrelated, const CString& mergeStrategy, const CString& strategyOption, const CString& strategyParam, const CString& logMessage, const CString& version, bool isBranch, bool showStashPop)
{
	CString args;
	if (noFF)
		args += L" --no-ff";
	else if (ffOnly)
		args += L" --ff-only";

	if (squash)
		args += L" --squash";

	if (noCommit)
		args += L" --no-commit";

	if (unrelated)
		args += L" --allow-unrelated-histories";

	if (log)
		args.AppendFormat(L" --log=%d", *log);

	if (!mergeStrategy.IsEmpty())
	{
		args += L" --strategy=" + mergeStrategy;
		if (!strategyOption.IsEmpty())
		{
			args += L" --strategy-option=" + strategyOption;
			if (!strategyParam.IsEmpty())
				args += L'=' + strategyParam;
		}
	}

	if (!logMessage.IsEmpty())
	{
		CString logmsg = logMessage;
		logmsg.Replace(L"\\\"", L"\\\\\"");
		logmsg.Replace(L"\"", L"\\\"");
		args += L" -m \"" + logmsg + L"\"";
	}

	CString mergeVersion = g_Git.FixBranchName(version);
	CString cmd;
	cmd.Format(L"git.exe merge%s %s", (LPCTSTR)args, (LPCTSTR)mergeVersion);

	CProgressDlg Prodlg;
	Prodlg.m_GitCmd = cmd;

	Prodlg.m_PostCmdCallback = [&](DWORD status, PostCmdList& postCmdList)
	{
		if (status)
		{
			int hasConflicts = g_Git.HasWorkingTreeConflicts();
			if (hasConflicts < 0)
				CMessageBox::Show(nullptr, g_Git.GetGitLastErr(L"Checking for conflicts failed.", CGit::GIT_CMD_CHECKCONFLICTS), L"TortoiseGit", MB_ICONEXCLAMATION);
			else if (hasConflicts)
			{
				// there are conflict files

				postCmdList.emplace_back(IDI_RESOLVE, IDS_PROGRS_CMD_RESOLVE, []
				{
					CString sCmd;
					sCmd.Format(L"/command:resolve /path:\"%s\"", (LPCTSTR)g_Git.m_CurrentDir);
					CAppUtils::RunTortoiseGitProc(sCmd);
				});

				postCmdList.emplace_back(IDI_COMMIT, IDS_MENUCOMMIT, []
				{
					CString sCmd;
					sCmd.Format(L"/command:commit /path:\"%s\"", (LPCTSTR)g_Git.m_CurrentDir);
					CAppUtils::RunTortoiseGitProc(sCmd);
				});
			}

			if (CAppUtils::IsGitVersionNewerOrEqual(2, 9))
			{
				CGitHash common;
				g_Git.IsFastForward(L"HEAD", mergeVersion, &common);
				if (common.IsEmpty())
					postCmdList.emplace_back(IDI_MERGE, IDS_MERGE_UNRELATED, [=] { DoMerge(noFF, ffOnly, squash, noCommit, log, true, mergeStrategy, strategyOption, strategyParam, logMessage, version, isBranch, showStashPop); });
			}

			postCmdList.emplace_back(IDI_COMMIT, IDS_MENUSTASHSAVE, [=]{ CAppUtils::StashSave(L"", false, false, true, mergeVersion); });

			return;
		}

		if (showStashPop)
			postCmdList.emplace_back(IDI_RELOCATE, IDS_MENUSTASHPOP, []{ CAppUtils::StashPop(); });

		if (noCommit || squash)
		{
			postCmdList.emplace_back(IDI_COMMIT, IDS_MENUCOMMIT, []
			{
				CString sCmd;
				sCmd.Format(L"/command:commit /path:\"%s\"", (LPCTSTR)g_Git.m_CurrentDir);
				CAppUtils::RunTortoiseGitProc(sCmd);
			});
			return;
		}

		if (isBranch && !CStringUtils::StartsWith(version, L"remotes/")) // do not ask to remove remote branches
		{
			postCmdList.emplace_back(IDI_DELETE, IDS_PROC_REMOVEBRANCH, [&]
			{
				CString msg;
				msg.Format(IDS_PROC_DELETEBRANCHTAG, (LPCTSTR)version);
				if (CMessageBox::Show(nullptr, msg, L"TortoiseGit", 2, IDI_QUESTION, CString(MAKEINTRESOURCE(IDS_DELETEBUTTON)), CString(MAKEINTRESOURCE(IDS_ABORTBUTTON))) == 1)
				{
					CString cmd, out;
					cmd.Format(L"git.exe branch -D -- %s", (LPCTSTR)version);
					if (g_Git.Run(cmd, &out, CP_UTF8))
						MessageBox(nullptr, out, L"TortoiseGit", MB_OK);
				}
			});
		}
		if (isBranch)
			postCmdList.emplace_back(IDI_PUSH, IDS_MENUPUSH, []{ CAppUtils::Push(); });

		BOOL hasGitSVN = CTGitPath(g_Git.m_CurrentDir).GetAdminDirMask() & ITEMIS_GITSVN;
		if (hasGitSVN)
			postCmdList.emplace_back(IDI_COMMIT, IDS_MENUSVNDCOMMIT, []{ CAppUtils::SVNDCommit(); });
	};

	Prodlg.DoModal();
	return !Prodlg.m_GitStatus;
}

BOOL CAppUtils::Merge(const CString* commit, bool showStashPop)
{
	if (!CheckUserData())
		return FALSE;

	if (IsTGitRebaseActive())
		return FALSE;

	CMergeDlg dlg;
	if (commit)
		dlg.m_initialRefName = *commit;

	if (dlg.DoModal() == IDOK)
		return DoMerge(dlg.m_bNoFF == BST_CHECKED, dlg.m_bFFonly == BST_CHECKED, dlg.m_bSquash == BST_CHECKED, dlg.m_bNoCommit == BST_CHECKED, dlg.m_bLog ? &dlg.m_nLog : nullptr, false, dlg.m_MergeStrategy, dlg.m_StrategyOption, dlg.m_StrategyParam, dlg.m_strLogMesage, dlg.m_VersionName, dlg.m_bIsBranch, showStashPop);

	return FALSE;
}

BOOL CAppUtils::MergeAbort()
{
	CMergeAbortDlg dlg;
	if (dlg.DoModal() == IDOK)
		return Reset(L"HEAD", (dlg.m_ResetType == 0) ? 3 : dlg.m_ResetType);

	return FALSE;
}

void CAppUtils::EditNote(GitRevLoglist* rev, ProjectProperties* projectProperties)
{
	if (!CheckUserData())
		return;

	CInputDlg dlg;
	dlg.m_sHintText = CString(MAKEINTRESOURCE(IDS_PROGS_TITLE_EDITNOTES));
	dlg.m_sInputText = rev->m_Notes;
	dlg.m_sTitle = CString(MAKEINTRESOURCE(IDS_PROGS_TITLE_EDITNOTES));
	dlg.m_pProjectProperties = projectProperties;
	dlg.m_bUseLogWidth = true;
	if(dlg.DoModal() == IDOK)
	{
		if (g_Git.SetGitNotes(rev->m_CommitHash, dlg.m_sInputText))
		{
			CString err;
			err.LoadString(IDS_PROC_FAILEDSAVINGNOTES);
			MessageBox(nullptr, g_Git.GetLibGit2LastErr(err), L"TortoiseGit", MB_OK | MB_ICONERROR);
			return;
		}

		if (g_Git.GetGitNotes(rev->m_CommitHash, rev->m_Notes))
			MessageBox(nullptr, g_Git.GetLibGit2LastErr(L"Could not load notes for commit " + rev->m_CommitHash.ToString() + L'.'), L"TortoiseGit", MB_OK | MB_ICONERROR);
	}
}

inline static int ConvertToVersionInt(int major, int minor, int patchlevel, int build)
{
	return (major << 24) + (minor << 16) + (patchlevel << 8) + build;
}

inline bool CAppUtils::IsGitVersionNewerOrEqual(int major, int minor, int patchlevel, int build)
{
	auto ver = GetMsysgitVersion();
	return ver >= ConvertToVersionInt(major, minor, patchlevel, build);
}

int CAppUtils::GetMsysgitVersion()
{
	if (g_Git.ms_LastMsysGitVersion)
		return g_Git.ms_LastMsysGitVersion;

	CString cmd;
	CString versiondebug;
	CString version;

	CRegDWORD regTime		= CRegDWORD(L"Software\\TortoiseGit\\git_file_time");
	CRegDWORD regVersion	= CRegDWORD(L"Software\\TortoiseGit\\git_cached_version");

	CString gitpath = CGit::ms_LastMsysGitDir + L"\\git.exe";

	__int64 time=0;
	if (!CGit::GetFileModifyTime(gitpath, &time))
	{
		if ((DWORD)CGit::filetime_to_time_t(time) == regTime)
		{
			g_Git.ms_LastMsysGitVersion = regVersion;
			return regVersion;
		}
	}

	CString err;
	int ver = g_Git.GetGitVersion(&versiondebug, &err);
	if (ver < 0)
	{
		MessageBox(nullptr, L"git.exe not correctly set up (" + err + L")\nCheck TortoiseGit settings and consult help file for \"Git.exe Path\".", L"TortoiseGit", MB_OK | MB_ICONERROR);
		return -1;
	}

	{
		if (!ver)
		{
			MessageBox(nullptr, L"Could not parse git.exe version number: \"" + versiondebug + L'"', L"TortoiseGit", MB_OK | MB_ICONERROR);
			return -1;
		}
	}

	regTime = time&0xFFFFFFFF;
	regVersion = ver;
	g_Git.ms_LastMsysGitVersion = ver;

	return ver;
}

void CAppUtils::MarkWindowAsUnpinnable(HWND hWnd)
{
	typedef HRESULT (WINAPI *SHGPSFW) (HWND hwnd,REFIID riid,void** ppv);

	CAutoLibrary hShell = AtlLoadSystemLibraryUsingFullPath(L"Shell32.dll");

	if (hShell.IsValid()) {
		SHGPSFW pfnSHGPSFW = (SHGPSFW)::GetProcAddress(hShell, "SHGetPropertyStoreForWindow");
		if (pfnSHGPSFW) {
			IPropertyStore *pps;
			HRESULT hr = pfnSHGPSFW(hWnd, IID_PPV_ARGS(&pps));
			if (SUCCEEDED(hr)) {
				PROPVARIANT var;
				var.vt = VT_BOOL;
				var.boolVal = VARIANT_TRUE;
				pps->SetValue(PKEY_AppUserModel_PreventPinning, var);
				pps->Release();
			}
		}
	}
}

void CAppUtils::SetWindowTitle(HWND hWnd, const CString& urlorpath, const CString& dialogname)
{
	ASSERT(dialogname.GetLength() < 70);
	ASSERT(urlorpath.GetLength() < MAX_PATH);
	WCHAR pathbuf[MAX_PATH] = {0};

	PathCompactPathEx(pathbuf, urlorpath, 70 - dialogname.GetLength(), 0);

	wcscat_s(pathbuf, L" - ");
	wcscat_s(pathbuf, dialogname);
	wcscat_s(pathbuf, L" - ");
	wcscat_s(pathbuf, CString(MAKEINTRESOURCE(IDS_APPNAME)));
	SetWindowText(hWnd, pathbuf);
}

bool CAppUtils::BisectStart(const CString& lastGood, const CString& firstBad, bool bIsMainWnd)
{
	if (!g_Git.CheckCleanWorkTree())
	{
		if (CMessageBox::Show(nullptr, IDS_ERROR_NOCLEAN_STASH, IDS_APPNAME, 1, IDI_QUESTION, IDS_STASHBUTTON, IDS_ABORTBUTTON) == 1)
		{
			CSysProgressDlg sysProgressDlg;
			sysProgressDlg.SetTitle(CString(MAKEINTRESOURCE(IDS_APPNAME)));
			sysProgressDlg.SetLine(1, CString(MAKEINTRESOURCE(IDS_PROC_STASHRUNNING)));
			sysProgressDlg.SetLine(2, CString(MAKEINTRESOURCE(IDS_PROGRESSWAIT)));
			sysProgressDlg.SetShowProgressBar(false);
			sysProgressDlg.SetCancelMsg(IDS_PROGRS_INFOFAILED);
			sysProgressDlg.ShowModeless((HWND)nullptr, true);

			CString out;
			if (g_Git.Run(L"git.exe stash", &out, CP_UTF8))
			{
				sysProgressDlg.Stop();
				MessageBox(nullptr, out, L"TortoiseGit", MB_OK | MB_ICONERROR);
				return false;
			}
			sysProgressDlg.Stop();
		}
		else
			return false;
	}

	CBisectStartDlg bisectStartDlg;

	if (!lastGood.IsEmpty())
		bisectStartDlg.m_sLastGood = lastGood;
	if (!firstBad.IsEmpty())
		bisectStartDlg.m_sFirstBad = firstBad;

	if (bisectStartDlg.DoModal() == IDOK)
	{
		CProgressDlg progress;
		if (bIsMainWnd)
			theApp.m_pMainWnd = &progress;
		progress.m_GitCmdList.push_back(L"git.exe bisect start");
		progress.m_GitCmdList.push_back(L"git.exe bisect good " + bisectStartDlg.m_LastGoodRevision);
		progress.m_GitCmdList.push_back(L"git.exe bisect bad " + bisectStartDlg.m_FirstBadRevision);

		progress.m_PostCmdCallback = [&](DWORD status, PostCmdList& postCmdList)
		{
			if (status)
				return;

			CTGitPath path(g_Git.m_CurrentDir);
			if (path.HasSubmodules())
			{
				postCmdList.emplace_back(IDI_UPDATE, IDS_PROC_SUBMODULESUPDATE, []
				{
					CString sCmd;
					sCmd.Format(L"/command:subupdate /bkpath:\"%s\"", (LPCTSTR)g_Git.m_CurrentDir);
					CAppUtils::RunTortoiseGitProc(sCmd);
				});
			}
		};

		INT_PTR ret = progress.DoModal();
		return ret == IDOK;
	}

	return false;
}

bool CAppUtils::BisectOperation(const CString& op, const CString& ref, bool bIsMainWnd)
{
	CString cmd = L"git.exe bisect " + op;

	if (!ref.IsEmpty())
	{
		cmd += L' ';
		cmd += ref;
	}

	CProgressDlg progress;
	if (bIsMainWnd)
		theApp.m_pMainWnd = &progress;
	progress.m_GitCmd = cmd;

	progress.m_PostCmdCallback = [&](DWORD status, PostCmdList& postCmdList)
	{
		if (status)
			return;

		CTGitPath path = g_Git.m_CurrentDir;
		if (path.HasSubmodules())
		{
			postCmdList.emplace_back(IDI_UPDATE, IDS_PROC_SUBMODULESUPDATE, []
			{
				CString sCmd;
				sCmd.Format(L"/command:subupdate /bkpath:\"%s\"", (LPCTSTR)g_Git.m_CurrentDir);
				CAppUtils::RunTortoiseGitProc(sCmd);
			});
		}

		if (op != L"reset")
			postCmdList.emplace_back(IDS_MENUBISECTRESET, []{ CAppUtils::RunTortoiseGitProc(L"/command:bisect /reset"); });
	};

	INT_PTR ret = progress.DoModal();
	return ret == IDOK;
}

int CAppUtils::Git2GetUserPassword(git_cred **out, const char *url, const char *username_from_url, unsigned int /*allowed_types*/, void * /*payload*/)
{
	CUserPassword dlg;
	dlg.m_URL = CUnicodeUtils::GetUnicode(url, CP_UTF8);
	if (username_from_url)
		dlg.m_UserName = CUnicodeUtils::GetUnicode(username_from_url, CP_UTF8);

	CStringA username, password;
	if (dlg.DoModal() == IDOK)
	{
		username = CUnicodeUtils::GetMulti(dlg.m_UserName, CP_UTF8);
		password = CUnicodeUtils::GetMulti(dlg.m_Password, CP_UTF8);
		return git_cred_userpass_plaintext_new(out, username, password);
	}
	giterr_set_str(GITERR_NONE, "User cancelled.");
	return GIT_EUSER;
}

int CAppUtils::Git2CertificateCheck(git_cert* base_cert, int /*valid*/, const char* host, void* /*payload*/)
{
	if (base_cert->cert_type == GIT_CERT_X509)
	{
		git_cert_x509* cert = (git_cert_x509*)base_cert;

		if (last_accepted_cert.cmp(cert))
			return 0;

		PCCERT_CONTEXT pServerCert = CertCreateCertificateContext(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, (BYTE*)cert->data, (DWORD)cert->len);
		SCOPE_EXIT { CertFreeCertificateContext(pServerCert); };

		DWORD verificationError = VerifyServerCertificate(pServerCert, CUnicodeUtils::GetUnicode(host).GetBuffer(), 0);
		if (!verificationError)
		{
			last_accepted_cert.set(cert);
			return 0;
		}

		CString servernameInCert;
		CertGetNameString(pServerCert, CERT_NAME_SIMPLE_DISPLAY_TYPE, 0, nullptr, CStrBuf(servernameInCert, 128), 128);

		CString issuer;
		CertGetNameString(pServerCert, CERT_NAME_SIMPLE_DISPLAY_TYPE, CERT_NAME_ISSUER_FLAG, nullptr, CStrBuf(issuer, 128), 128);

		CCheckCertificateDlg dlg;
		dlg.cert = cert;
		dlg.m_sCertificateCN = servernameInCert;
		dlg.m_sCertificateIssuer = issuer;
		dlg.m_sHostname = CUnicodeUtils::GetUnicode(host);
		dlg.m_sError = CFormatMessageWrapper(verificationError);
		if (dlg.DoModal() == IDOK)
		{
			last_accepted_cert.set(cert);
			return 0;
		}
	}
	return GIT_ECERTIFICATE;
}

int CAppUtils::ExploreTo(HWND hwnd, CString path)
{
	if (PathFileExists(path))
	{
		HRESULT ret = E_FAIL;
		ITEMIDLIST __unaligned * pidl = ILCreateFromPath(path);
		if (pidl)
		{
			ret = SHOpenFolderAndSelectItems(pidl, 0, 0, 0);
			ILFree(pidl);
		}
		return SUCCEEDED(ret) ? 0 : -1;
	}
	// if filepath does not exist any more, navigate to closest matching folder
	do
	{
		int pos = path.ReverseFind(L'\\');
		if (pos <= 3)
			break;
		path.Truncate(pos);
	} while (!PathFileExists(path));
	return (INT_PTR)ShellExecute(hwnd, L"explore", path, nullptr, nullptr, SW_SHOW) > 32 ? 0 : -1;
}

int CAppUtils::ResolveConflict(CTGitPath& path, resolve_with resolveWith)
{
	bool b_local = false, b_remote = false;
	BYTE_VECTOR vector;
	{
		CString cmd;
		cmd.Format(L"git.exe ls-files -u -t -z -- \"%s\"", (LPCTSTR)path.GetGitPathString());
		if (g_Git.Run(cmd, &vector))
		{
			MessageBox(nullptr, L"git ls-files failed!", L"TortoiseGit", MB_OK);
			return -1;
		}

		CTGitPathList list;
		if (list.ParserFromLsFile(vector))
		{
			MessageBox(nullptr, L"Parse ls-files failed!", L"TortoiseGit", MB_OK);
			return -1;
		}

		if (list.IsEmpty())
			return 0;
		for (int i = 0; i < list.GetCount(); ++i)
		{
			if (list[i].m_Stage == 2)
				b_local = true;
			if (list[i].m_Stage == 3)
				b_remote = true;
		}
	}

	bool baseIsFile = true, localIsFile = true, remoteIsFile = true;
	CString baseHash, localHash, remoteHash;
	ParseHashesFromLsFile(vector, baseHash, baseIsFile, localHash, localIsFile, remoteHash, remoteIsFile);

	CBlockCacheForPath block(g_Git.m_CurrentDir);
	if ((resolveWith == RESOLVE_WITH_THEIRS && !b_remote) || (resolveWith == RESOLVE_WITH_MINE && !b_local))
	{
		CString gitcmd, output; //retest with registered submodule!
		gitcmd.Format(L"git.exe rm -f -- \"%s\"", (LPCTSTR)path.GetGitPathString());
		if (g_Git.Run(gitcmd, &output, CP_UTF8))
		{
			// a .git folder in a submodule which is not in .gitmodules cannot be deleted using "git rm"
			if (PathIsDirectory(path.GetGitPathString()) && !PathIsDirectoryEmpty(path.GetGitPathString()))
			{
				CString message(output);
				output += L"\n\n";
				output.AppendFormat(IDS_PROC_DELETEBRANCHTAG, path.GetWinPath());
				CString deleteButton;
				deleteButton.LoadString(IDS_DELETEBUTTON);
				CString abortButton;
				abortButton.LoadString(IDS_ABORTBUTTON);
				if (CMessageBox::Show(nullptr, output, L"TortoiseGit", 2, IDI_QUESTION, deleteButton, abortButton) == 2)
					return -1;
				path.Delete(true, true);
				output.Empty();
				if (!g_Git.Run(gitcmd, &output, CP_UTF8))
				{
					RemoveTempMergeFile(path);
					return 0;
				}
			}
			MessageBox(nullptr, output, L"TortoiseGit", MB_ICONERROR);
			return -1;
		}
		RemoveTempMergeFile(path);
		return 0;
	}

	if (resolveWith == RESOLVE_WITH_THEIRS || resolveWith == RESOLVE_WITH_MINE)
	{
		auto resolve = [&b_local, &b_remote](const CTGitPath& path, int stage, bool willBeFile, const CString& hash) -> int
		{
			if (!willBeFile)
			{
				if (!path.HasAdminDir()) // check if submodule is initialized
				{
					CString gitcmd, output;
					if (!path.IsDirectory())
					{
						gitcmd.Format(L"git.exe checkout-index -f --stage=%d -- \"%s\"", stage, (LPCTSTR)path.GetGitPathString());
						if (g_Git.Run(gitcmd, &output, CP_UTF8))
						{
							MessageBox(nullptr, output, L"TortoiseGit", MB_ICONERROR);
							return -1;
						}
					}
					gitcmd.Format(L"git.exe update-index --replace --cacheinfo 0160000,%s,\"%s\"", (LPCTSTR)hash, (LPCTSTR)path.GetGitPathString());
					if (g_Git.Run(gitcmd, &output, CP_UTF8))
					{
						MessageBox(nullptr, output, L"TortoiseGit", MB_ICONERROR);
						return -1;
					}
					return 0;
				}

				CGit subgit;
				subgit.m_CurrentDir = g_Git.CombinePath(path);
				CGitHash submoduleHead;
				if (subgit.GetHash(submoduleHead, L"HEAD"))
				{
					MessageBox(nullptr, subgit.GetGitLastErr(L"Could not get HEAD hash of submodule, this should not happen!"), L"TortoiseGit", MB_ICONERROR);
					return -1;
				}
				if (submoduleHead.ToString() != hash)
				{
					CString origPath = g_Git.m_CurrentDir;
					g_Git.m_CurrentDir = g_Git.CombinePath(path);
					if (!GitReset(&hash))
					{
						g_Git.m_CurrentDir = origPath;
						return -1;
					}
					g_Git.m_CurrentDir = origPath;
				}
			}
			else
			{
				CString gitcmd, output;
				if (b_local && b_remote)
					gitcmd.Format(L"git.exe checkout-index -f --stage=%d -- \"%s\"", stage, (LPCTSTR)path.GetGitPathString());
				else
					gitcmd.Format(L"git.exe add -f -- \"%s\"", (LPCTSTR)path.GetGitPathString());
				if (g_Git.Run(gitcmd, &output, CP_UTF8))
				{
					MessageBox(nullptr, output, L"TortoiseGit", MB_ICONERROR);
					return -1;
				}
			}
			return 0;
		};
		int ret = -1;
		if (resolveWith == RESOLVE_WITH_THEIRS)
			ret = resolve(path, 3, remoteIsFile, remoteHash);
		else
			ret = resolve(path, 2, localIsFile, localHash);
		if (ret)
			return ret;
	}

	if (PathFileExists(g_Git.CombinePath(path)) && (path.m_Action & CTGitPath::LOGACTIONS_UNMERGED))
	{
		CString gitcmd, output;
		gitcmd.Format(L"git.exe add -f -- \"%s\"", (LPCTSTR)path.GetGitPathString());
		if (g_Git.Run(gitcmd, &output, CP_UTF8))
		{
			MessageBox(nullptr, output, L"TortoiseGit", MB_ICONERROR);
			return -1;
		}

		path.m_Action |= CTGitPath::LOGACTIONS_MODIFIED;
		path.m_Action &= ~CTGitPath::LOGACTIONS_UNMERGED;
	}

	RemoveTempMergeFile(path);
	return 0;
}

bool CAppUtils::ShellOpen(const CString& file, HWND hwnd /*= nullptr */)
{
	if ((INT_PTR)ShellExecute(hwnd, nullptr, file, nullptr, nullptr, SW_SHOW) > HINSTANCE_ERROR)
		return true;

	return ShowOpenWithDialog(file, hwnd);
}

bool CAppUtils::ShowOpenWithDialog(const CString& file, HWND hwnd /*= nullptr */)
{
	OPENASINFO oi = { 0 };
	oi.pcszFile = file;
	oi.oaifInFlags = OAIF_EXEC;
	return SUCCEEDED(SHOpenWithDialog(hwnd, &oi));
}

bool CAppUtils::IsTGitRebaseActive()
{
	CString adminDir;
	if (!GitAdminDir::GetAdminDirPath(g_Git.m_CurrentDir, adminDir))
		return false;

	if (!PathIsDirectory(adminDir + L"tgitrebase.active"))
		return false;

	if (CMessageBox::Show(nullptr, IDS_REBASELOCKFILEFOUND, IDS_APPNAME, 2, IDI_EXCLAMATION, IDS_REMOVESTALEBUTTON, IDS_ABORTBUTTON) == 2)
		return true;

	RemoveDirectory(adminDir + L"tgitrebase.active");

	return false;
}

bool CAppUtils::DeleteRef(CWnd* parent, const CString& ref)
{
	CString shortname;
	if (CGit::GetShortName(ref, shortname, L"refs/remotes/"))
	{
		CString msg;
		msg.Format(IDS_PROC_DELETEREMOTEBRANCH, (LPCTSTR)ref);
		int result = CMessageBox::Show(parent->GetSafeOwner()->GetSafeHwnd(), msg, L"TortoiseGit", 3, IDI_QUESTION, CString(MAKEINTRESOURCE(IDS_PROC_DELETEREMOTEBRANCH_LOCALREMOTE)), CString(MAKEINTRESOURCE(IDS_PROC_DELETEREMOTEBRANCH_LOCAL)), CString(MAKEINTRESOURCE(IDS_ABORTBUTTON)));
		if (result == 1)
		{
			CString remoteName = shortname.Left(shortname.Find(L'/'));
			shortname = shortname.Mid(shortname.Find(L'/') + 1);
			if (CAppUtils::IsSSHPutty())
				CAppUtils::LaunchPAgent(nullptr, &remoteName);

			CSysProgressDlg sysProgressDlg;
			sysProgressDlg.SetTitle(CString(MAKEINTRESOURCE(IDS_APPNAME)));
			sysProgressDlg.SetLine(1, CString(MAKEINTRESOURCE(IDS_DELETING_REMOTE_REFS)));
			sysProgressDlg.SetLine(2, CString(MAKEINTRESOURCE(IDS_PROGRESSWAIT)));
			sysProgressDlg.SetShowProgressBar(false);
			sysProgressDlg.ShowModal(parent, true);
			STRING_VECTOR list;
			list.push_back(L"refs/heads/" + shortname);
			if (g_Git.DeleteRemoteRefs(remoteName, list))
				CMessageBox::Show(parent->GetSafeOwner()->GetSafeHwnd(), g_Git.GetGitLastErr(L"Could not delete remote ref.", CGit::GIT_CMD_PUSH), L"TortoiseGit", MB_OK | MB_ICONERROR);
			sysProgressDlg.Stop();
			return true;
		}
		else if (result == 2)
		{
			if (g_Git.DeleteRef(ref))
			{
				CMessageBox::Show(parent->GetSafeOwner()->GetSafeHwnd(), g_Git.GetGitLastErr(L"Could not delete reference.", CGit::GIT_CMD_DELETETAGBRANCH), L"TortoiseGit", MB_OK | MB_ICONERROR);
				return false;
			}
			return true;
		}
		return false;
	}
	else if (CGit::GetShortName(ref, shortname, L"refs/stash"))
	{
		CString err;
		std::vector<GitRevLoglist> stashList;
		size_t count = !GitRevLoglist::GetRefLog(ref, stashList, err) ? stashList.size() : 0;
		CString msg;
		msg.Format(IDS_PROC_DELETEALLSTASH, count);
		int choose = CMessageBox::Show(parent->GetSafeOwner()->GetSafeHwnd(), msg, L"TortoiseGit", 3, IDI_QUESTION, CString(MAKEINTRESOURCE(IDS_DELETEBUTTON)), CString(MAKEINTRESOURCE(IDS_DROPONESTASH)), CString(MAKEINTRESOURCE(IDS_ABORTBUTTON)));
		if (choose == 1)
		{
			CString out;
			if (g_Git.Run(L"git.exe stash clear", &out, CP_UTF8))
				CMessageBox::Show(parent->GetSafeOwner()->GetSafeHwnd(), out, L"TortoiseGit", MB_OK | MB_ICONERROR);
			return true;
		}
		else if (choose == 2)
		{
			CString out;
			if (g_Git.Run(L"git.exe stash drop refs/stash@{0}", &out, CP_UTF8))
				CMessageBox::Show(parent->GetSafeOwner()->GetSafeHwnd(), out, L"TortoiseGit", MB_OK | MB_ICONERROR);
			return true;
		}
		return false;
	}

	CString msg;
	msg.Format(IDS_PROC_DELETEBRANCHTAG, (LPCTSTR)ref);
	// Check if branch is fully merged in HEAD
	if (CGit::GetShortName(ref, shortname, L"refs/heads/") && !g_Git.IsFastForward(ref, L"HEAD"))
	{
		msg += L"\n\n";
		msg += CString(MAKEINTRESOURCE(IDS_PROC_BROWSEREFS_WARNINGUNMERGED));
	}
	if (CMessageBox::Show(parent->GetSafeOwner()->GetSafeHwnd(), msg, L"TortoiseGit", 2, IDI_QUESTION, CString(MAKEINTRESOURCE(IDS_DELETEBUTTON)), CString(MAKEINTRESOURCE(IDS_ABORTBUTTON))) == 1)
	{
		if (g_Git.DeleteRef(ref))
		{
			CMessageBox::Show(parent->GetSafeOwner()->GetSafeHwnd(), g_Git.GetGitLastErr(L"Could not delete reference.", CGit::GIT_CMD_DELETETAGBRANCH), L"TortoiseGit", MB_OK | MB_ICONERROR);
			return false;
		}
		return true;
	}
	return false;
}
