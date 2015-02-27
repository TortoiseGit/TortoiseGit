// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2015 - TortoiseGit
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
#include "StringUtils.h"
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
#include "MessageBox.h"
#include "GitStatus.h"
#include "CreateBranchTagDlg.h"
#include "GitSwitchDlg.h"
#include "ResetDlg.h"
#include "DeleteConflictDlg.h"
#include "ChangedDlg.h"
#include "SendMailDlg.h"
#include "GitProgressDlg.h"
#include "PushDlg.h"
#include "CommitDlg.h"
#include "MergeDlg.h"
#include "MergeAbortDlg.h"
#include "Hooks.h"
#include "..\Settings\Settings.h"
#include "InputDlg.h"
#include "SVNDCommitDlg.h"
#include "requestpulldlg.h"
#include "PullFetchDlg.h"
#include "FileDiffDlg.h"
#include "RebaseDlg.h"
#include "PropKey.h"
#include "StashSave.h"
#include "IgnoreDlg.h"
#include "FormatMessageWrapper.h"
#include "SmartHandle.h"
#include "BisectStartDlg.h"
#include "SysProgressDlg.h"
#include "UserPassword.h"
#include "Patch.h"
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

static bool DoFetch(const CString& url, const bool fetchAllRemotes, const bool loadPuttyAgent, const int prune, const bool bDepth, const int nDepth, const int fetchTags, const CString& remoteBranch, boolean runRebase);

CAppUtils::CAppUtils(void)
{
}

CAppUtils::~CAppUtils(void)
{
}

bool CAppUtils::StashSave(const CString& msg, bool showPull, bool pullShowPush, bool showMerge, const CString& mergeRev)
{
	CStashSaveDlg dlg;
	dlg.m_sMessage = msg;
	if (dlg.DoModal() == IDOK)
	{
		CString cmd;
		cmd = _T("git.exe stash save");

		if (CAppUtils::GetMsysgitVersion() >= 0x01070700)
		{
			if (dlg.m_bIncludeUntracked)
				cmd += _T(" --include-untracked");
			else if (dlg.m_bAll)
				cmd += _T(" --all");
		}

		if (!dlg.m_sMessage.IsEmpty())
		{
			CString message = dlg.m_sMessage;
			message.Replace(_T("\""), _T("\"\""));
			cmd += _T(" -- \"") + message + _T("\"");
		}

		CProgressDlg progress;
		progress.m_GitCmd = cmd;
		progress.m_PostCmdCallback = [&](DWORD status, PostCmdList& postCmdList)
		{
			if (status)
				return;

			if (showPull)
				postCmdList.push_back(PostCmd(IDI_PULL, IDS_MENUPULL, [&]{ CAppUtils::Pull(pullShowPush, true); }));
			if (showMerge)
				postCmdList.push_back(PostCmd(IDI_MERGE, IDS_MENUMERGE, [&]{ CAppUtils::Merge(&mergeRev, true); }));
		};
		return (progress.DoModal() == IDOK);
	}
	return false;
}

bool CAppUtils::StashApply(CString ref, bool showChanges /* true */)
{
	CString cmd,out;
	cmd = _T("git.exe stash apply ");
	if (ref.Find(_T("refs/")) == 0)
		ref = ref.Mid(5);
	if (ref.Find(_T("stash{")) == 0)
		ref = _T("stash@") + ref.Mid(5);
	cmd += ref;

	CSysProgressDlg sysProgressDlg;
	sysProgressDlg.SetTitle(CString(MAKEINTRESOURCE(IDS_APPNAME)));
	sysProgressDlg.SetLine(1, CString(MAKEINTRESOURCE(IDS_PROC_STASHRUNNING)));
	sysProgressDlg.SetLine(2, CString(MAKEINTRESOURCE(IDS_PROGRESSWAIT)));
	sysProgressDlg.SetShowProgressBar(false);
	sysProgressDlg.SetCancelMsg(IDS_PROGRS_INFOFAILED);
	sysProgressDlg.ShowModeless((HWND)NULL, true);

	int ret = g_Git.Run(cmd, &out, CP_UTF8);

	sysProgressDlg.Stop();

	bool hasConflicts = (out.Find(_T("CONFLICT")) >= 0);
	if (ret && !(ret == 1 && hasConflicts))
	{
		CMessageBox::Show(NULL, CString(MAKEINTRESOURCE(IDS_PROC_STASHAPPLYFAILED)) + _T("\n") + out, _T("TortoiseGit"), MB_OK | MB_ICONERROR);
	}
	else
	{
		CString message;
		message.LoadString(IDS_PROC_STASHAPPLYSUCCESS);
		if (hasConflicts)
			message.LoadString(IDS_PROC_STASHAPPLYFAILEDCONFLICTS);
		if (showChanges)
		{
			if(CMessageBox::Show(NULL,message + _T("\n") + CString(MAKEINTRESOURCE(IDS_SEECHANGES))
				,_T("TortoiseGit"),MB_YESNO|MB_ICONINFORMATION) == IDYES)
			{
				CChangedDlg dlg;
				dlg.m_pathList.AddPath(CTGitPath());
				dlg.DoModal();
			}
			return true;
		}
		else
		{
			CMessageBox::Show(NULL, message ,_T("TortoiseGit"), MB_OK | MB_ICONINFORMATION);
			return true;
		}
	}
	return false;
}

bool CAppUtils::StashPop(bool showChanges /* true */)
{
	CString cmd,out;
	cmd=_T("git.exe stash pop ");

	CSysProgressDlg sysProgressDlg;
	sysProgressDlg.SetTitle(CString(MAKEINTRESOURCE(IDS_APPNAME)));
	sysProgressDlg.SetLine(1, CString(MAKEINTRESOURCE(IDS_PROC_STASHRUNNING)));
	sysProgressDlg.SetLine(2, CString(MAKEINTRESOURCE(IDS_PROGRESSWAIT)));
	sysProgressDlg.SetShowProgressBar(false);
	sysProgressDlg.SetCancelMsg(IDS_PROGRS_INFOFAILED);
	sysProgressDlg.ShowModeless((HWND)NULL, true);

	int ret = g_Git.Run(cmd, &out, CP_UTF8);

	sysProgressDlg.Stop();

	bool hasConflicts = (out.Find(_T("CONFLICT")) >= 0);
	if (ret && !(ret == 1 && hasConflicts))
	{
		CMessageBox::Show(NULL,CString(MAKEINTRESOURCE(IDS_PROC_STASHPOPFAILED)) + _T("\n") + out, _T("TortoiseGit"), MB_OK | MB_ICONERROR);
	}
	else
	{
		CString message;
		message.LoadString(IDS_PROC_STASHPOPSUCCESS);
		if (hasConflicts)
			message.LoadString(IDS_PROC_STASHPOPFAILEDCONFLICTS);
		if (showChanges)
		{
 			if(CMessageBox::Show(NULL,CString(message + _T("\n") + CString(MAKEINTRESOURCE(IDS_SEECHANGES)))
				,_T("TortoiseGit"),MB_YESNO|MB_ICONINFORMATION) == IDYES)
			{
				CChangedDlg dlg;
				dlg.m_pathList.AddPath(CTGitPath());
				dlg.DoModal();
			}
			return true;
		}
		else
		{
			CMessageBox::Show(NULL, message ,_T("TortoiseGit"), MB_OK | MB_ICONINFORMATION);
			return true;
		}
	}
	return false;
}

BOOL CAppUtils::StartExtMerge(
	const CTGitPath& basefile, const CTGitPath& theirfile, const CTGitPath& yourfile, const CTGitPath& mergedfile,
	const CString& basename, const CString& theirname, const CString& yourname, const CString& mergedname, bool bReadOnly,
	HWND resolveMsgHwnd, bool bDeleteBaseTheirsMineOnClose)
{

	CRegString regCom = CRegString(_T("Software\\TortoiseGit\\Merge"));
	CString ext = mergedfile.GetFileExtension();
	CString com = regCom;
	bool bInternal = false;

	if (!ext.IsEmpty())
	{
		// is there an extension specific merge tool?
		CRegString mergetool(_T("Software\\TortoiseGit\\MergeTools\\") + ext.MakeLower());
		if (!CString(mergetool).IsEmpty())
		{
			com = mergetool;
		}
	}
	// is there a filename specific merge tool?
	CRegString mergetool(_T("Software\\TortoiseGit\\MergeTools\\.") + mergedfile.GetFilename().MakeLower());
	if (!CString(mergetool).IsEmpty())
	{
		com = mergetool;
	}

	if (com.IsEmpty()||(com.Left(1).Compare(_T("#"))==0))
	{
		// Maybe we should use TortoiseIDiff?
		if ((ext == _T(".jpg")) || (ext == _T(".jpeg")) ||
			(ext == _T(".bmp")) || (ext == _T(".gif"))  ||
			(ext == _T(".png")) || (ext == _T(".ico"))  ||
			(ext == _T(".tif")) || (ext == _T(".tiff"))  ||
			(ext == _T(".dib")) || (ext == _T(".emf"))  ||
			(ext == _T(".cur")))
		{
			com = CPathUtils::GetAppDirectory() + _T("TortoiseGitIDiff.exe");
			com = _T("\"") + com + _T("\"");
			com = com + _T(" /base:%base /theirs:%theirs /mine:%mine /result:%merged");
			com = com + _T(" /basetitle:%bname /theirstitle:%tname /minetitle:%yname");
		}
		else
		{
			// use TortoiseGitMerge
			bInternal = true;
			com = CPathUtils::GetAppDirectory() + _T("TortoiseGitMerge.exe");
			com = _T("\"") + com + _T("\"");
			com = com + _T(" /base:%base /theirs:%theirs /mine:%mine /merged:%merged");
			com = com + _T(" /basename:%bname /theirsname:%tname /minename:%yname /mergedname:%mname");
			com += _T(" /saverequired");
			if (resolveMsgHwnd)
			{
				CString s;
				s.Format(L" /resolvemsghwnd:%I64d", (__int64)resolveMsgHwnd);
				com += s;
			}
			if (bDeleteBaseTheirsMineOnClose)
				com += _T(" /deletebasetheirsmineonclose");
		}
		if (!g_sGroupingUUID.IsEmpty())
		{
			com += L" /groupuuid:\"";
			com += g_sGroupingUUID;
			com += L"\"";
		}
	}
	// check if the params are set. If not, just add the files to the command line
	if ((com.Find(_T("%merged"))<0)&&(com.Find(_T("%base"))<0)&&(com.Find(_T("%theirs"))<0)&&(com.Find(_T("%mine"))<0))
	{
		com += _T(" \"")+basefile.GetWinPathString()+_T("\"");
		com += _T(" \"")+theirfile.GetWinPathString()+_T("\"");
		com += _T(" \"")+yourfile.GetWinPathString()+_T("\"");
		com += _T(" \"")+mergedfile.GetWinPathString()+_T("\"");
	}
	if (basefile.IsEmpty())
	{
		com.Replace(_T("/base:%base"), _T(""));
		com.Replace(_T("%base"), _T(""));
	}
	else
		com.Replace(_T("%base"), _T("\"") + basefile.GetWinPathString() + _T("\""));
	if (theirfile.IsEmpty())
	{
		com.Replace(_T("/theirs:%theirs"), _T(""));
		com.Replace(_T("%theirs"), _T(""));
	}
	else
		com.Replace(_T("%theirs"), _T("\"") + theirfile.GetWinPathString() + _T("\""));
	if (yourfile.IsEmpty())
	{
		com.Replace(_T("/mine:%mine"), _T(""));
		com.Replace(_T("%mine"), _T(""));
	}
	else
		com.Replace(_T("%mine"), _T("\"") + yourfile.GetWinPathString() + _T("\""));
	if (mergedfile.IsEmpty())
	{
		com.Replace(_T("/merged:%merged"), _T(""));
		com.Replace(_T("%merged"), _T(""));
	}
	else
		com.Replace(_T("%merged"), _T("\"") + mergedfile.GetWinPathString() + _T("\""));
	if (basename.IsEmpty())
	{
		if (basefile.IsEmpty())
		{
			com.Replace(_T("/basename:%bname"), _T(""));
			com.Replace(_T("%bname"), _T(""));
		}
		else
		{
			com.Replace(_T("%bname"), _T("\"") + basefile.GetUIFileOrDirectoryName() + _T("\""));
		}
	}
	else
		com.Replace(_T("%bname"), _T("\"") + basename + _T("\""));
	if (theirname.IsEmpty())
	{
		if (theirfile.IsEmpty())
		{
			com.Replace(_T("/theirsname:%tname"), _T(""));
			com.Replace(_T("%tname"), _T(""));
		}
		else
		{
			com.Replace(_T("%tname"), _T("\"") + theirfile.GetUIFileOrDirectoryName() + _T("\""));
		}
	}
	else
		com.Replace(_T("%tname"), _T("\"") + theirname + _T("\""));
	if (yourname.IsEmpty())
	{
		if (yourfile.IsEmpty())
		{
			com.Replace(_T("/minename:%yname"), _T(""));
			com.Replace(_T("%yname"), _T(""));
		}
		else
		{
			com.Replace(_T("%yname"), _T("\"") + yourfile.GetUIFileOrDirectoryName() + _T("\""));
		}
	}
	else
		com.Replace(_T("%yname"), _T("\"") + yourname + _T("\""));
	if (mergedname.IsEmpty())
	{
		if (mergedfile.IsEmpty())
		{
			com.Replace(_T("/mergedname:%mname"), _T(""));
			com.Replace(_T("%mname"), _T(""));
		}
		else
		{
			com.Replace(_T("%mname"), _T("\"") + mergedfile.GetUIFileOrDirectoryName() + _T("\""));
		}
	}
	else
		com.Replace(_T("%mname"), _T("\"") + mergedname + _T("\""));

	if ((bReadOnly)&&(bInternal))
		com += _T(" /readonly");

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
	viewer += _T("TortoiseGitMerge.exe");

	viewer = _T("\"") + viewer + _T("\"");
	viewer = viewer + _T(" /diff:\"") + patchfile.GetWinPathString() + _T("\"");
	viewer = viewer + _T(" /patchpath:\"") + dir.GetWinPathString() + _T("\"");
	if (bReversed)
		viewer += _T(" /reversedpatch");
	if (!sOriginalDescription.IsEmpty())
		viewer = viewer + _T(" /patchoriginal:\"") + sOriginalDescription + _T("\"");
	if (!sPatchedDescription.IsEmpty())
		viewer = viewer + _T(" /patchpatched:\"") + sPatchedDescription + _T("\"");
	if (!g_sGroupingUUID.IsEmpty())
	{
		viewer += L" /groupuuid:\"";
		viewer += g_sGroupingUUID;
		viewer += L"\"";
	}
	if(!LaunchApplication(viewer, IDS_ERR_DIFFVIEWSTART, !!bWait))
	{
		return FALSE;
	}
	return TRUE;
}

CString CAppUtils::PickDiffTool(const CTGitPath& file1, const CTGitPath& file2)
{
	CString difftool = CRegString(_T("Software\\TortoiseGit\\DiffTools\\.") + file2.GetFilename().MakeLower());
	if (!difftool.IsEmpty())
		return difftool;
	difftool = CRegString(_T("Software\\TortoiseGit\\DiffTools\\.") + file1.GetFilename().MakeLower());
	if (!difftool.IsEmpty())
		return difftool;

	// Is there an extension specific diff tool?
	CString ext = file2.GetFileExtension().MakeLower();
	if (!ext.IsEmpty())
	{
		difftool = CRegString(_T("Software\\TortoiseGit\\DiffTools\\") + ext);
		if (!difftool.IsEmpty())
			return difftool;
		// Maybe we should use TortoiseIDiff?
		if ((ext == _T(".jpg")) || (ext == _T(".jpeg")) ||
			(ext == _T(".bmp")) || (ext == _T(".gif"))  ||
			(ext == _T(".png")) || (ext == _T(".ico"))  ||
			(ext == _T(".tif")) || (ext == _T(".tiff")) ||
			(ext == _T(".dib")) || (ext == _T(".emf"))  ||
			(ext == _T(".cur")))
		{
			return
				_T("\"") + CPathUtils::GetAppDirectory() + _T("TortoiseGitIDiff.exe") + _T("\"") +
				_T(" /left:%base /right:%mine /lefttitle:%bname /righttitle:%yname") +
				L" /groupuuid:\"" + g_sGroupingUUID + L"\"";
		}
	}

	// Finally, pick a generic external diff tool
	difftool = CRegString(_T("Software\\TortoiseGit\\Diff"));
	return difftool;
}

bool CAppUtils::StartExtDiff(
	const CString& file1,  const CString& file2,
	const CString& sName1, const CString& sName2,
	const CString& originalFile1, const CString& originalFile2,
	const git_revnum_t& hash1, const git_revnum_t& hash2,
	const DiffFlags& flags, int jumpToLine)
{
	CString viewer;

	CRegDWORD blamediff(_T("Software\\TortoiseGit\\DiffBlamesWithTortoiseMerge"), FALSE);
	if (!flags.bBlame || !(DWORD)blamediff)
	{
		viewer = PickDiffTool(file1, file2);
		// If registry entry for a diff program is commented out, use TortoiseGitMerge.
		bool bCommentedOut = viewer.Left(1) == _T("#");
		if (flags.bAlternativeTool)
		{
			// Invert external vs. internal diff tool selection.
			if (bCommentedOut)
				viewer.Delete(0); // uncomment
			else
				viewer = "";
		}
		else if (bCommentedOut)
			viewer = "";
	}

	bool bInternal = viewer.IsEmpty();
	if (bInternal)
	{
		viewer =
			_T("\"") + CPathUtils::GetAppDirectory() + _T("TortoiseGitMerge.exe") + _T("\"") +
			_T(" /base:%base /mine:%mine /basename:%bname /minename:%yname") +
			_T(" /basereflectedname:%bpath /minereflectedname:%ypath");
		if (!g_sGroupingUUID.IsEmpty())
		{
			viewer += L" /groupuuid:\"";
			viewer += g_sGroupingUUID;
			viewer += L"\"";
		}
		if (flags.bBlame)
			viewer += _T(" /blame");
	}
	// check if the params are set. If not, just add the files to the command line
	if ((viewer.Find(_T("%base"))<0)&&(viewer.Find(_T("%mine"))<0))
	{
		viewer += _T(" \"")+file1+_T("\"");
		viewer += _T(" \"")+file2+_T("\"");
	}
	if (viewer.Find(_T("%base")) >= 0)
	{
		viewer.Replace(_T("%base"),  _T("\"")+file1+_T("\""));
	}
	if (viewer.Find(_T("%mine")) >= 0)
	{
		viewer.Replace(_T("%mine"),  _T("\"")+file2+_T("\""));
	}

	if (sName1.IsEmpty())
		viewer.Replace(_T("%bname"), _T("\"") + file1 + _T("\""));
	else
		viewer.Replace(_T("%bname"), _T("\"") + sName1 + _T("\""));

	if (sName2.IsEmpty())
		viewer.Replace(_T("%yname"), _T("\"") + file2 + _T("\""));
	else
		viewer.Replace(_T("%yname"), _T("\"") + sName2 + _T("\""));

	viewer.Replace(_T("%bpath"), _T("\"") + originalFile1 + _T("\""));
	viewer.Replace(_T("%ypath"), _T("\"") + originalFile2 + _T("\""));

	viewer.Replace(_T("%brev"), _T("\"") + hash1 + _T("\""));
	viewer.Replace(_T("%yrev"), _T("\"") + hash2 + _T("\""));

	if (flags.bReadOnly && bInternal)
		viewer += _T(" /readonly");

	if (jumpToLine > 0)
	{
		CString temp;
		temp.Format(_T(" /line:%d"), jumpToLine);
		viewer += temp;
	}

	return LaunchApplication(viewer, IDS_ERR_EXTDIFFSTART, flags.bWait);
}

BOOL CAppUtils::StartUnifiedDiffViewer(const CString& patchfile, const CString& title, BOOL bWait)
{
	CString viewer;
	CRegString v = CRegString(_T("Software\\TortoiseGit\\DiffViewer"));
	viewer = v;
	if (viewer.IsEmpty() || (viewer.Left(1).Compare(_T("#"))==0))
	{
		// use TortoiseGitUDiff
		viewer = CPathUtils::GetAppDirectory();
		viewer += _T("TortoiseGitUDiff.exe");
		// enquote the path to TortoiseGitUDiff
		viewer = _T("\"") + viewer + _T("\"");
		// add the params
		viewer = viewer + _T(" /patchfile:%1 /title:\"%title\"");
		if (!g_sGroupingUUID.IsEmpty())
		{
			viewer += L" /groupuuid:\"";
			viewer += g_sGroupingUUID;
			viewer += L"\"";
		}
	}
	if (viewer.Find(_T("%1"))>=0)
	{
		if (viewer.Find(_T("\"%1\"")) >= 0)
			viewer.Replace(_T("%1"), patchfile);
		else
			viewer.Replace(_T("%1"), _T("\"") + patchfile + _T("\""));
	}
	else
		viewer += _T(" \"") + patchfile + _T("\"");
	if (viewer.Find(_T("%title")) >= 0)
	{
		viewer.Replace(_T("%title"), title);
	}

	if(!LaunchApplication(viewer, IDS_ERR_DIFFVIEWSTART, !!bWait))
	{
		return FALSE;
	}
	return TRUE;
}

BOOL CAppUtils::StartTextViewer(CString file)
{
	CString viewer;
	CRegString txt = CRegString(_T(".txt\\"), _T(""), FALSE, HKEY_CLASSES_ROOT);
	viewer = txt;
	viewer = viewer + _T("\\Shell\\Open\\Command\\");
	CRegString txtexe = CRegString(viewer, _T(""), FALSE, HKEY_CLASSES_ROOT);
	viewer = txtexe;

	DWORD len = ExpandEnvironmentStrings(viewer, NULL, 0);
	std::unique_ptr<TCHAR[]> buf(new TCHAR[len + 1]);
	ExpandEnvironmentStrings(viewer, buf.get(), len);
	viewer = buf.get();
	len = ExpandEnvironmentStrings(file, NULL, 0);
	std::unique_ptr<TCHAR[]> buf2(new TCHAR[len + 1]);
	ExpandEnvironmentStrings(file, buf2.get(), len);
	file = buf2.get();
	file = _T("\"")+file+_T("\"");
	if (viewer.IsEmpty())
	{
		return CAppUtils::ShowOpenWithDialog(file) ? TRUE : FALSE;
	}
	if (viewer.Find(_T("\"%1\"")) >= 0)
	{
		viewer.Replace(_T("\"%1\""), file);
	}
	else if (viewer.Find(_T("%1")) >= 0)
	{
		viewer.Replace(_T("%1"),  file);
	}
	else
	{
		viewer += _T(" ");
		viewer += file;
	}

	if(!LaunchApplication(viewer, IDS_ERR_TEXTVIEWSTART, false))
	{
		return FALSE;
	}
	return TRUE;
}

BOOL CAppUtils::CheckForEmptyDiff(const CTGitPath& sDiffPath)
{
	DWORD length = 0;
	CAutoFile hFile = ::CreateFile(sDiffPath.GetWinPath(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, NULL, NULL);
	if (!hFile)
		return TRUE;
	length = ::GetFileSize(hFile, NULL);
	if (length < 4)
		return TRUE;
	return FALSE;

}

void CAppUtils::CreateFontForLogs(CFont& fontToCreate)
{
	LOGFONT logFont;
	HDC hScreenDC = ::GetDC(NULL);
	logFont.lfHeight			= -MulDiv((DWORD)CRegDWORD(_T("Software\\TortoiseGit\\LogFontSize"), 8), GetDeviceCaps(hScreenDC, LOGPIXELSY), 72);
	::ReleaseDC(NULL, hScreenDC);
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
	_tcscpy_s(logFont.lfFaceName, 32, (LPCTSTR)(CString)CRegString(_T("Software\\TortoiseGit\\LogFontName"), _T("Courier New")));
	VERIFY(fontToCreate.CreateFontIndirect(&logFont));
}

bool CAppUtils::LaunchPAgent(const CString* keyfile, const CString* pRemote)
{
	CString key,remote;
	CString cmd,out;
	if( pRemote == NULL)
	{
		remote=_T("origin");
	}
	else
	{
		remote=*pRemote;
	}
	if(keyfile == NULL)
	{
		cmd.Format(_T("remote.%s.puttykeyfile"),remote);
		key = g_Git.GetConfigValue(cmd);
	}
	else
		key=*keyfile;

	if(key.IsEmpty())
		return false;

	CString proc=CPathUtils::GetAppDirectory();
	proc += _T("pageant.exe \"");
	proc += key;
	proc += _T("\"");

	CString tempfile = GetTempFile();
	::DeleteFile(tempfile);

	proc += _T(" -c \"");
	proc += CPathUtils::GetAppDirectory();
	proc += _T("tgittouch.exe\"");
	proc += _T(" \"");
	proc += tempfile;
	proc += _T("\"");

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
	{
		CMessageBox::Show(NULL, IDS_ERR_PAEGENTTIMEOUT, IDS_APPNAME, MB_OK | MB_ICONERROR);
	}
	::DeleteFile(tempfile);
	return true;
}
bool CAppUtils::LaunchAlternativeEditor(const CString& filename, bool uac)
{
	CString editTool = CRegString(_T("Software\\TortoiseGit\\AlternativeEditor"));
	if (editTool.IsEmpty() || (editTool.Left(1).Compare(_T("#"))==0)) {
		editTool = CPathUtils::GetAppDirectory() + _T("notepad2.exe");
	}

	CString sCmd;
	sCmd.Format(_T("\"%s\" \"%s\""), editTool, filename);

	LaunchApplication(sCmd, NULL, false, NULL, uac);
	return true;
}
bool CAppUtils::LaunchRemoteSetting()
{
	CTGitPath path(g_Git.m_CurrentDir);
	CSettings dlg(IDS_PROC_SETTINGS_TITLE, &path);
	dlg.SetTreeViewMode(TRUE, TRUE, TRUE);
	dlg.SetTreeWidth(220);
	dlg.m_DefaultPage = _T("gitremote");

	dlg.DoModal();
	dlg.HandleRestart();
	return true;
}
/**
* Launch the external blame viewer
*/
bool CAppUtils::LaunchTortoiseBlame(const CString& sBlameFile, const CString& Rev, const CString& sParams)
{
	CString viewer = _T("\"") + CPathUtils::GetAppDirectory();
	viewer += _T("TortoiseGitBlame.exe");
	viewer += _T("\" \"") + sBlameFile + _T("\"");
	//viewer += _T(" \"") + sLogFile + _T("\"");
	//viewer += _T(" \"") + sOriginalFile + _T("\"");
	if(!Rev.IsEmpty() && Rev != GIT_REV_ZERO)
		viewer += CString(_T(" /rev:"))+Rev;
	if (!g_sGroupingUUID.IsEmpty())
	{
		viewer += L" /groupuuid:\"";
		viewer += g_sGroupingUUID;
		viewer += L"\"";
	}
	viewer += _T(" ")+sParams;

	return LaunchApplication(viewer, IDS_ERR_TGITBLAME, false);
}

bool CAppUtils::FormatTextInRichEditControl(CWnd * pWnd)
{
	CString sText;
	if (pWnd == NULL)
		return false;
	bool bStyled = false;
	pWnd->GetWindowText(sText);
	// the rich edit control doesn't count the CR char!
	// to be exact: CRLF is treated as one char.
	sText.Remove(_T('\r'));

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
	TCHAR c = i == 0 ? _T('\0') : sText[i - 1];
	TCHAR nextChar = i >= last ? _T('\0') : sText[i + 1];

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
			ch == L'|' || ch == L'>' || ch == L'<';
	}

	bool IsUrl(const CString& sText)
	{
		if (!PathIsURLW(sText))
			return false;
		CString prefixes[] = { L"http://", L"https://", L"git://", L"ftp://", L"file://", L"mailto:" };
		for (const CString& prefix : prefixes)
		{
			if (sText.Find(prefix) == 0 && sText.GetLength() != prefix.GetLength())
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
				if (!IsUrl(msg.Mid(starturl, i - starturl)))
				{
					starturl = -1;
					continue;
				}

				int skipTrailing = 0;
				while (strip && i - skipTrailing - 1 > starturl && (msg[i - skipTrailing - 1] == '.' || msg[i - skipTrailing - 1] == '-' || msg[i - skipTrailing - 1] == '?' || msg[i - skipTrailing - 1] == ';' || msg[i - skipTrailing - 1] == ':' || msg[i - skipTrailing - 1] == '>' || msg[i - skipTrailing - 1] == '<'))
					++skipTrailing;
				
				CHARRANGE range = { starturl, i - skipTrailing };
				result.push_back(range);
			}
			starturl = -1;
		}
	}

	return result;
}

bool CAppUtils::StartShowUnifiedDiff(HWND hWnd, const CTGitPath& url1, const git_revnum_t& rev1,
												const CTGitPath& /*url2*/, const git_revnum_t& rev2,
												//const GitRev& peg /* = GitRev */, const GitRev& headpeg /* = GitRev */,
												bool /*bAlternateDiff*/ /* = false */, bool /*bIgnoreAncestry*/ /* = false */,
												bool /* blame = false */, 
												bool bMerge,
												bool bCombine)
{
	int diffContext = 0;
	if (GetMsysgitVersion() > 0x01080100)
		diffContext = g_Git.GetConfigValueInt32(_T("diff.context"), -1);
	CString tempfile=GetTempFile();
	if (g_Git.GetUnifiedDiff(url1, rev1, rev2, tempfile, bMerge, bCombine, diffContext))
	{
		CMessageBox::Show(hWnd, g_Git.GetGitLastErr(_T("Could not get unified diff."), CGit::GIT_CMD_DIFF), _T("TortoiseGit"), MB_OK);
		return false;
	}
	CAppUtils::StartUnifiedDiffViewer(tempfile, rev1 + _T(":") + rev2);

#if 0
	CString sCmd;
	sCmd.Format(_T("%s /command:showcompare /unified"),
		(LPCTSTR)(CPathUtils::GetAppDirectory()+_T("TortoiseGitProc.exe")));
	sCmd += _T(" /url1:\"") + url1.GetGitPathString() + _T("\"");
	if (rev1.IsValid())
		sCmd += _T(" /revision1:") + rev1.ToString();
	sCmd += _T(" /url2:\"") + url2.GetGitPathString() + _T("\"");
	if (rev2.IsValid())
		sCmd += _T(" /revision2:") + rev2.ToString();
	if (peg.IsValid())
		sCmd += _T(" /pegrevision:") + peg.ToString();
	if (headpeg.IsValid())
		sCmd += _T(" /headpegrevision:") + headpeg.ToString();

	if (bAlternateDiff)
		sCmd += _T(" /alternatediff");

	if (bIgnoreAncestry)
		sCmd += _T(" /ignoreancestry");

	if (hWnd)
	{
		sCmd += _T(" /hwnd:");
		TCHAR buf[30];
		_stprintf_s(buf, 30, _T("%p"), (void*)hWnd);
		sCmd += buf;
	}

	return CAppUtils::LaunchApplication(sCmd, NULL, false);
#endif
	return TRUE;
}

bool CAppUtils::SetupDiffScripts(bool force, const CString& type)
{
	CString scriptsdir = CPathUtils::GetAppParentDirectory();
	scriptsdir += _T("Diff-Scripts");
	CSimpleFileFind files(scriptsdir);
	while (files.FindNextFileNoDirectories())
	{
		CString file = files.GetFilePath();
		CString filename = files.GetFileName();
		CString ext = file.Mid(file.ReverseFind('-') + 1);
		ext = _T(".") + ext.Left(ext.ReverseFind('.'));
		std::set<CString> extensions;
		extensions.insert(ext);
		CString kind;
		if (file.Right(3).CompareNoCase(_T("vbs"))==0)
		{
			kind = _T(" //E:vbscript");
		}
		if (file.Right(2).CompareNoCase(_T("js"))==0)
		{
			kind = _T(" //E:javascript");
		}
		// open the file, read the first line and find possible extensions
		// this script can handle
		try
		{
			CStdioFile f(file, CFile::modeRead | CFile::shareDenyNone);
			CString extline;
			if (f.ReadString(extline))
			{
				if ((extline.GetLength() > 15 ) &&
					((extline.Left(15).Compare(_T("// extensions: ")) == 0) ||
					(extline.Left(14).Compare(_T("' extensions: ")) == 0)))
				{
					if (extline[0] == '/')
						extline = extline.Mid(15);
					else
						extline = extline.Mid(14);
					CString sToken;
					int curPos = 0;
					sToken = extline.Tokenize(_T(";"), curPos);
					while (!sToken.IsEmpty())
					{
						if (!sToken.IsEmpty())
						{
							if (sToken[0] != '.')
								sToken = _T(".") + sToken;
							extensions.insert(sToken);
						}
						sToken = extline.Tokenize(_T(";"), curPos);
					}
				}
			}
			f.Close();
		}
		catch (CFileException* e)
		{
			e->Delete();
		}

		for (std::set<CString>::const_iterator it = extensions.begin(); it != extensions.end(); ++it)
		{
			if (type.IsEmpty() || (type.Compare(_T("Diff")) == 0))
			{
				if (filename.Left(5).CompareNoCase(_T("diff-")) == 0)
				{
					CRegString diffreg = CRegString(_T("Software\\TortoiseGit\\DiffTools\\") + *it);
					CString diffregstring = diffreg;
					if (force || (diffregstring.IsEmpty()) || (diffregstring.Find(filename) >= 0))
						diffreg = _T("wscript.exe \"") + file + _T("\" %base %mine") + kind;
				}
			}
			if (type.IsEmpty() || (type.Compare(_T("Merge"))==0))
			{
				if (filename.Left(6).CompareNoCase(_T("merge-"))==0)
				{
					CRegString diffreg = CRegString(_T("Software\\TortoiseGit\\MergeTools\\") + *it);
					CString diffregstring = diffreg;
					if (force || (diffregstring.IsEmpty()) || (diffregstring.Find(filename) >= 0))
						diffreg = _T("wscript.exe \"") + file + _T("\" %merged %theirs %mine %base") + kind;
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
		cmd.Format(_T("git.exe archive --output=\"%s\" --format=zip --verbose %s --"),
					dlg.m_strFile, g_Git.FixBranchName(dlg.m_VersionName));

		CProgressDlg pro;
		pro.m_GitCmd=cmd;
		pro.m_PostCmdCallback = [&](DWORD status, PostCmdList& postCmdList)
		{
			if (status)
				return;
			postCmdList.push_back(PostCmd(IDI_EXPLORER, IDS_STATUSLIST_CONTEXT_EXPLORE, [&]{ CAppUtils::ExploreTo(hWndExplorer, dlg.m_strFile); }));
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

bool CAppUtils::CreateBranchTag(bool IsTag, const CString* CommitHash, bool switch_new_brach)
{
	CCreateBranchTagDlg dlg;
	dlg.m_bIsTag=IsTag;
	dlg.m_bSwitch=switch_new_brach;

	if(CommitHash)
		dlg.m_initialRefName = *CommitHash;

	if(dlg.DoModal()==IDOK)
	{
		CString cmd;
		CString force;
		CString track;
		if(dlg.m_bTrack == TRUE)
			track=_T(" --track ");
		else if(dlg.m_bTrack == FALSE)
			track=_T(" --no-track");

		if(dlg.m_bForce)
			force=_T(" -f ");

		if(IsTag)
		{
			CString sign;
			if(dlg.m_bSign)
				sign=_T("-s");

			cmd.Format(_T("git.exe tag %s %s %s %s"),
				force,
				sign,
				dlg.m_BranchTagName,
				g_Git.FixBranchName(dlg.m_VersionName)
				);

			if(!dlg.m_Message.Trim().IsEmpty())
			{
				CString tempfile = ::GetTempFile();
				if (CAppUtils::SaveCommitUnicodeFile(tempfile, dlg.m_Message))
				{
					CMessageBox::Show(nullptr, _T("Could not save tag message"), _T("TortoiseGit"), MB_OK | MB_ICONERROR);
					return FALSE;
				}
				cmd += _T(" -F ")+tempfile;
			}
		}
		else
		{
			cmd.Format(_T("git.exe branch %s %s %s %s"),
				track,
				force,
				dlg.m_BranchTagName,
				g_Git.FixBranchName(dlg.m_VersionName)
				);
		}
		CString out;
		if(g_Git.Run(cmd,&out,CP_UTF8))
		{
			CMessageBox::Show(NULL,out,_T("TortoiseGit"),MB_OK);
			return FALSE;
		}
		if( !IsTag  &&  dlg.m_bSwitch )
		{
			// it is a new branch and the user has requested to switch to it
			PerformSwitch(dlg.m_BranchTagName);
		}

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
		if (dlg.m_VersionName.Left(11) ==_T("refs/heads/") && dlg.m_bBranchOverride != TRUE)
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
		{
			branch.Format(_T("-B %s"), sNewBranch);
		}
		else
		{
			branch.Format(_T("-b %s"), sNewBranch);
		}
		if (bTrack == TRUE)
			track = _T("--track");
		else if (bTrack == FALSE)
			track = _T("--no-track");
	}
	if (bForce)
		force = _T("-f");
	if (bMerge)
		merge = _T("--merge");

	cmd.Format(_T("git.exe checkout %s %s %s %s %s --"),
		 force,
		 track,
		 merge,
		 branch,
		 g_Git.FixBranchName(ref));

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
				postCmdList.push_back(PostCmd(IDI_UPDATE, IDS_PROC_SUBMODULESUPDATE, [&]
				{
					CString sCmd;
					sCmd.Format(_T("/command:subupdate /bkpath:\"%s\""), g_Git.m_CurrentDir);
					RunTortoiseGitProc(sCmd);
				}));
			}
			if (hasBranch)
				postCmdList.push_back(PostCmd(IDI_MERGE, IDS_MENUMERGE, [&]{ Merge(&currentBranch); }));


			CString newBranch;
			if (!CGit::GetCurrentBranchFromFile(g_Git.m_CurrentDir, newBranch))
				postCmdList.push_back(PostCmd(IDI_PULL, IDS_MENUPULL, [&]{ Pull(); }));

			postCmdList.push_back(PostCmd(IDI_COMMIT, IDS_MENUCOMMIT, []{
				CTGitPathList pathlist;
				CTGitPathList selectedlist;
				bool bSelectFilesForCommit = !!DWORD(CRegStdDWORD(_T("Software\\TortoiseGit\\SelectFilesForCommit"), TRUE));
				CString str;
				Commit(CString(), false, str, pathlist, selectedlist, bSelectFilesForCommit);
			}));
		}
		else
		{
			postCmdList.push_back(PostCmd(IDI_REFRESH, IDS_MSGBOX_RETRY, [&]{ PerformSwitch(ref, bForce, sNewBranch, bBranchOverride, bTrack, bMerge); }));
			if (!bMerge)
				postCmdList.push_back(PostCmd(IDI_SWITCH, IDS_SWITCH_WITH_MERGE, [&]{ PerformSwitch(ref, bForce, sNewBranch, bBranchOverride, bTrack, true); }));
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
			{
				SeekToBegin();
			}
		}

		CStringA strA;
		char lastChar = '\0';
		for (char c = '\0'; Read(&c, 1) == 1; lastChar = c)
		{
			if (c == '\r')
				continue;
			if (c == '\n')
			{
				m_eol = lastChar == '\r' ? _T("\r\n") : _T("\n");
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
		m_eol = _T("");
	}
};

bool CAppUtils::OpenIgnoreFile(CIgnoreFile &file, const CString& filename)
{
	file.ResetState();
	if (!file.Open(filename, CFile::modeCreate | CFile::modeReadWrite | CFile::modeNoTruncate | CFile::typeBinary))
	{
		CMessageBox::Show(NULL, filename + _T(" Open Failure"), _T("TortoiseGit"), MB_OK | MB_ICONERROR);
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
			CStringA eol = CStringA(file.m_eol.IsEmpty() ? _T("\n") : file.m_eol);
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
		ignorefile = g_Git.m_CurrentDir + _T("\\");

		switch (ignoreDlg.m_IgnoreFile)
		{
			case 0:
				ignorefile += _T(".gitignore");
				break;
			case 2:
				GitAdminDir::GetAdminDirPath(g_Git.m_CurrentDir, ignorefile);
				ignorefile += _T("info/exclude");
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
					ignorefile = g_Git.CombinePath(path[i].GetContainingDirectory()) + _T("\\.gitignore");
					if (!OpenIgnoreFile(file, ignorefile))
						return false;
				}

				CString ignorePattern;
				if (ignoreDlg.m_IgnoreType == 0)
				{
					if (ignoreDlg.m_IgnoreFile != 1 && !path[i].GetContainingDirectory().GetGitPathString().IsEmpty())
						ignorePattern += _T("/") + path[i].GetContainingDirectory().GetGitPathString();

					ignorePattern += _T("/");
				}
				if (IsMask)
				{
					ignorePattern += _T("*") + path[i].GetFileExtension();
				}
				else
				{
					ignorePattern += path[i].GetFileOrDirectoryName();
				}

				// escape [ and ] so that files get ignored correctly
				ignorePattern.Replace(_T("["), _T("\\["));
				ignorePattern.Replace(_T("]"), _T("\\]"));

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
					ignorePattern += file.m_eol.IsEmpty() ? _T("\n") : file.m_eol;
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
		type = _T("--soft");
		break;
	case 1:
		type = _T("--mixed");
		break;
	case 2:
		type = _T("--hard");
		break;
	default:
		resetType = 1;
		type = _T("--mixed");
		break;
	}
	cmd.Format(_T("git.exe reset %s %s --"), type, resetTo);

	CProgressDlg progress;
	progress.m_GitCmd = cmd;

	progress.m_PostCmdCallback = [&](DWORD status, PostCmdList& postCmdList)
	{
		if (status)
		{
			postCmdList.push_back(PostCmd(IDI_REFRESH, IDS_MSGBOX_RETRY, [&]{ Reset(resetTo, resetType); }));
			return;
		}

		CTGitPath gitPath = g_Git.m_CurrentDir;
		if (gitPath.HasSubmodules() && resetType == 2)
		{
			postCmdList.push_back(PostCmd(IDI_UPDATE, IDS_PROC_SUBMODULESUPDATE, [&]
			{
				CString sCmd;
				sCmd.Format(_T("/command:subupdate /bkpath:\"%s\""), g_Git.m_CurrentDir);
				CAppUtils::RunTortoiseGitProc(sCmd);
			}));
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
		descript = CString(MAKEINTRESOURCE(IDS_SVNACTION_DELETE));
		return;
	}
	if(base)
	{
		descript = CString(MAKEINTRESOURCE(IDS_SVNACTION_MODIFIED));
		return;
	}
	descript = CString(MAKEINTRESOURCE(IDS_PROC_CREATED));
	return;
}

void CAppUtils::RemoveTempMergeFile(const CTGitPath& path)
{
	::DeleteFile(CAppUtils::GetMergeTempFile(_T("LOCAL"), path));
	::DeleteFile(CAppUtils::GetMergeTempFile(_T("REMOTE"), path));
	::DeleteFile(CAppUtils::GetMergeTempFile(_T("BASE"), path));
}
CString CAppUtils::GetMergeTempFile(const CString& type, const CTGitPath &merge)
{
	CString file;
	file = g_Git.CombinePath(merge.GetWinPathString() + _T(".") + type + merge.GetFileExtension());

	return file;
}

bool ParseHashesFromLsFile(const BYTE_VECTOR& out, CString& hash1, CString& hash2, CString& hash3) // wtf?
{
	int pos = 0;
	CString one;
	CString part;

	while (pos >= 0 && pos < (int)out.size())
	{
		one.Empty();

		CGit::StringAppend(&one, &out[pos], CP_UTF8);
		int tabstart = 0;
		one.Tokenize(_T("\t"), tabstart);

		tabstart = 0;
		part = one.Tokenize(_T(" "), tabstart); //Tag
		part = one.Tokenize(_T(" "), tabstart); //Mode
		part = one.Tokenize(_T(" "), tabstart); //Hash
		CString hash = part;
		part = one.Tokenize(_T("\t"), tabstart); //Stage
		int stage = _ttol(part);
		if (stage == 1)
			hash1 = hash;
		else if (stage == 2)
			hash2 = hash;
		else if (stage == 3)
		{
			hash3 = hash;
			return true;
		}

		pos = out.findNextString(pos);
	}

	return false;
}

bool CAppUtils::ConflictEdit(const CTGitPath& path, bool /*bAlternativeTool = false*/, bool revertTheirMy /*= false*/, HWND resolveMsgHwnd /*= nullptr*/)
{
	bool bRet = false;

	CTGitPath merge=path;
	CTGitPath directory = merge.GetDirectory();

	// we have the conflicted file (%merged)
	// now look for the other required files
	//GitStatus stat;
	//stat.GetStatus(merge);
	//if (stat.status == NULL)
	//	return false;

	BYTE_VECTOR vector;

	CString cmd;
	cmd.Format(_T("git.exe ls-files -u -t -z -- \"%s\""),merge.GetGitPathString());

	if (g_Git.Run(cmd, &vector))
	{
		return FALSE;
	}

	if (merge.IsDirectory())
	{
		CString baseHash, realBaseHash(GIT_REV_ZERO), localHash(GIT_REV_ZERO), remoteHash(GIT_REV_ZERO);
		if (merge.HasAdminDir()) {
			CGit subgit;
			subgit.m_CurrentDir = g_Git.CombinePath(merge);
			CGitHash hash;
			subgit.GetHash(hash, _T("HEAD"));
			baseHash = hash;
		}
		if (ParseHashesFromLsFile(vector, realBaseHash, localHash, remoteHash)) // in base no submodule, but in remote submodule
			baseHash = realBaseHash;

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
			baseSubject = _T("no submodule");
			mineSubject = baseSubject;
			theirsSubject = _T("not initialized");
		}
		else if (baseHash.IsEmpty() && localHash != GIT_REV_ZERO && remoteHash == GIT_REV_ZERO) // merge conflict with no submodule initialized, but submodule exists in base and folder with no submodule is merged
		{
			baseHash = localHash;
			baseSubject = _T("not initialized");
			mineSubject = baseSubject;
			theirsSubject = _T("not initialized");
			changeTypeMine = CGitDiff::Identical;
			changeTypeTheirs = CGitDiff::DeleteSubmodule;
		}
		else if (baseHash != GIT_REV_ZERO && localHash != GIT_REV_ZERO && remoteHash != GIT_REV_ZERO) // base has submodule, mine has submodule and theirs also, but not initialized
		{
			baseSubject = _T("not initialized");
			mineSubject = baseSubject;
			theirsSubject = baseSubject;
			if (baseHash == localHash)
				changeTypeMine = CGitDiff::Identical;
		}
		else
			return FALSE;

		CSubmoduleResolveConflictDlg resolveSubmoduleConflictDialog;
		resolveSubmoduleConflictDialog.SetDiff(merge.GetGitPathString(), revertTheirMy, baseHash, baseSubject, baseOK, localHash, mineSubject, mineOK, changeTypeMine, remoteHash, theirsSubject, theirsOK, changeTypeTheirs);
		resolveSubmoduleConflictDialog.DoModal();
		if (resolveSubmoduleConflictDialog.m_bResolved && resolveMsgHwnd)
		{
			static UINT WM_REVERTMSG = RegisterWindowMessage(_T("GITSLNM_NEEDSREFRESH"));
			::PostMessage(resolveMsgHwnd, WM_REVERTMSG, NULL, NULL);
		}

		return TRUE;
	}

	CTGitPathList list;
	if (list.ParserFromLsFile(vector))
	{
		CMessageBox::Show(NULL, _T("Parse ls-files failed!"), _T("TortoiseGit"), MB_OK);
		return FALSE;
	}

	if (list.IsEmpty())
		return FALSE;

	CTGitPath theirs;
	CTGitPath mine;
	CTGitPath base;

	if (revertTheirMy)
	{
		mine.SetFromGit(GetMergeTempFile(_T("REMOTE"), merge));
		theirs.SetFromGit(GetMergeTempFile(_T("LOCAL"), merge));
	}
	else
	{
		mine.SetFromGit(GetMergeTempFile(_T("LOCAL"), merge));
		theirs.SetFromGit(GetMergeTempFile(_T("REMOTE"), merge));
	}
	base.SetFromGit(GetMergeTempFile(_T("BASE"),merge));

	CString format;

	//format=_T("git.exe cat-file blob \":%d:%s\"");
	format = _T("git.exe checkout-index --temp --stage=%d -- \"%s\"");
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
		CString cmd;
		CString outfile;
		cmd.Empty();
		outfile.Empty();

		if( list[i].m_Stage == 1)
		{
			cmd.Format(format, list[i].m_Stage, list[i].GetGitPathString());
			b_base = true;
			outfile = base.GetWinPathString();

		}
		if( list[i].m_Stage == 2 )
		{
			cmd.Format(format, list[i].m_Stage, list[i].GetGitPathString());
			b_local = true;
			outfile = mine.GetWinPathString();

		}
		if( list[i].m_Stage == 3 )
		{
			cmd.Format(format, list[i].m_Stage, list[i].GetGitPathString());
			b_remote = true;
			outfile = theirs.GetWinPathString();
		}
		CString output, err;
		if(!outfile.IsEmpty())
			if (!g_Git.Run(cmd, &output, &err, CP_UTF8))
			{
				CString file;
				int start =0 ;
				file = output.Tokenize(_T("\t"), start);
				::MoveFileEx(file,outfile,MOVEFILE_REPLACE_EXISTING|MOVEFILE_COPY_ALLOWED);
			}
			else
			{
				CMessageBox::Show(NULL, output + L"\n" + err, _T("TortoiseGit"), MB_OK|MB_ICONERROR);
			}
	}

	if(b_local && b_remote )
	{
		merge.SetFromWin(g_Git.CombinePath(merge));
		if( revertTheirMy )
			bRet = !!CAppUtils::StartExtMerge(base, mine, theirs, merge, _T("BASE"), _T("REMOTE"), _T("LOCAL"), CString(), false, resolveMsgHwnd, true);
		else
			bRet = !!CAppUtils::StartExtMerge(base, theirs, mine, merge, _T("BASE"), _T("REMOTE"), _T("LOCAL"), CString(), false, resolveMsgHwnd, true);

	}
	else
	{
		::DeleteFile(mine.GetWinPathString());
		::DeleteFile(theirs.GetWinPathString());
		::DeleteFile(base.GetWinPathString());

		CDeleteConflictDlg dlg;
		DescribeConflictFile(b_local, b_base,dlg.m_LocalStatus);
		DescribeConflictFile(b_remote,b_base,dlg.m_RemoteStatus);
		CGitHash localHash, remoteHash;
		if (!g_Git.GetHash(localHash, _T("HEAD")))
			dlg.m_LocalHash = localHash.ToString();
		if (!g_Git.GetHash(remoteHash, _T("MERGE_HEAD")))
			dlg.m_RemoteHash = remoteHash.ToString();
		else if (!g_Git.GetHash(remoteHash, _T("rebase-apply/original-commit")))
			dlg.m_RemoteHash = remoteHash.ToString();
		else if (!g_Git.GetHash(remoteHash, _T("CHERRY_PICK_HEAD")))
			dlg.m_RemoteHash = remoteHash.ToString();
		else if (!g_Git.GetHash(remoteHash, _T("REVERT_HEAD")))
			dlg.m_RemoteHash = remoteHash.ToString();
		dlg.m_bShowModifiedButton=b_base;
		dlg.m_File=merge.GetGitPathString();
		if(dlg.DoModal() == IDOK)
		{
			CString cmd,out;
			if(dlg.m_bIsDelete)
			{
				cmd.Format(_T("git.exe rm -- \"%s\""),merge.GetGitPathString());
			}
			else
				cmd.Format(_T("git.exe add -- \"%s\""),merge.GetGitPathString());

			if (g_Git.Run(cmd, &out, CP_UTF8))
			{
				CMessageBox::Show(NULL,out,_T("TortoiseGit"),MB_OK);
				return FALSE;
			}
			return TRUE;
		}
		else
			return FALSE;
	}

#if 0
	CAppUtils::StartExtMerge(CAppUtils::MergeFlags().AlternativeTool(bAlternativeTool),
			base, theirs, mine, merge);
#endif
#if 0
	if (stat.status->text_status == svn_wc_status_conflicted)
	{
		// we have a text conflict, use our merge tool to resolve the conflict

		CTSVNPath theirs(directory);
		CTSVNPath mine(directory);
		CTSVNPath base(directory);
		bool bConflictData = false;

		if ((stat.status->entry)&&(stat.status->entry->conflict_new))
		{
			theirs.AppendPathString(CUnicodeUtils::GetUnicode(stat.status->entry->conflict_new));
			bConflictData = true;
		}
		if ((stat.status->entry)&&(stat.status->entry->conflict_old))
		{
			base.AppendPathString(CUnicodeUtils::GetUnicode(stat.status->entry->conflict_old));
			bConflictData = true;
		}
		if ((stat.status->entry)&&(stat.status->entry->conflict_wrk))
		{
			mine.AppendPathString(CUnicodeUtils::GetUnicode(stat.status->entry->conflict_wrk));
			bConflictData = true;
		}
		else
		{
			mine = merge;
		}
		if (bConflictData)
			bRet = !!CAppUtils::StartExtMerge(CAppUtils::MergeFlags().AlternativeTool(bAlternativeTool),
												base, theirs, mine, merge);
	}

	if (stat.status->prop_status == svn_wc_status_conflicted)
	{
		// we have a property conflict
		CTSVNPath prej(directory);
		if ((stat.status->entry)&&(stat.status->entry->prejfile))
		{
			prej.AppendPathString(CUnicodeUtils::GetUnicode(stat.status->entry->prejfile));
			// there's a problem: the prej file contains a _description_ of the conflict, and
			// that description string might be translated. That means we have no way of parsing
			// the file to find out the conflicting values.
			// The only thing we can do: show a dialog with the conflict description, then
			// let the user either accept the existing property or open the property edit dialog
			// to manually change the properties and values. And a button to mark the conflict as
			// resolved.
			CEditPropConflictDlg dlg;
			dlg.SetPrejFile(prej);
			dlg.SetConflictedItem(merge);
			bRet = (dlg.DoModal() != IDCANCEL);
		}
	}

	if (stat.status->tree_conflict)
	{
		// we have a tree conflict
		SVNInfo info;
		const SVNInfoData * pInfoData = info.GetFirstFileInfo(merge, SVNRev(), SVNRev());
		if (pInfoData)
		{
			if (pInfoData->treeconflict_kind == svn_wc_conflict_kind_text)
			{
				CTSVNPath theirs(directory);
				CTSVNPath mine(directory);
				CTSVNPath base(directory);
				bool bConflictData = false;

				if (pInfoData->treeconflict_theirfile)
				{
					theirs.AppendPathString(pInfoData->treeconflict_theirfile);
					bConflictData = true;
				}
				if (pInfoData->treeconflict_basefile)
				{
					base.AppendPathString(pInfoData->treeconflict_basefile);
					bConflictData = true;
				}
				if (pInfoData->treeconflict_myfile)
				{
					mine.AppendPathString(pInfoData->treeconflict_myfile);
					bConflictData = true;
				}
				else
				{
					mine = merge;
				}
				if (bConflictData)
					bRet = !!CAppUtils::StartExtMerge(CAppUtils::MergeFlags().AlternativeTool(bAlternativeTool),
														base, theirs, mine, merge);
			}
			else if (pInfoData->treeconflict_kind == svn_wc_conflict_kind_tree)
			{
				CString sConflictAction;
				CString sConflictReason;
				CString sResolveTheirs;
				CString sResolveMine;
				CTSVNPath treeConflictPath = CTSVNPath(pInfoData->treeconflict_path);
				CString sItemName = treeConflictPath.GetUIFileOrDirectoryName();

				if (pInfoData->treeconflict_nodekind == svn_node_file)
				{
					switch (pInfoData->treeconflict_operation)
					{
					case svn_wc_operation_update:
						switch (pInfoData->treeconflict_action)
						{
						case svn_wc_conflict_action_edit:
							sConflictAction.Format(IDS_TREECONFLICT_FILEUPDATEEDIT, (LPCTSTR)sItemName);
							sResolveTheirs.LoadString(IDS_TREECONFLICT_RESOLVE_KEEPREPOSITORYFILE);
							break;
						case svn_wc_conflict_action_add:
							sConflictAction.Format(IDS_TREECONFLICT_FILEUPDATEADD, (LPCTSTR)sItemName);
							sResolveTheirs.LoadString(IDS_TREECONFLICT_RESOLVE_KEEPREPOSITORYFILE);
							break;
						case svn_wc_conflict_action_delete:
							sConflictAction.Format(IDS_TREECONFLICT_FILEUPDATEDELETE, (LPCTSTR)sItemName);
							sResolveTheirs.LoadString(IDS_TREECONFLICT_RESOLVE_REMOVEFILE);
							break;
						}
						break;
					case svn_wc_operation_switch:
						switch (pInfoData->treeconflict_action)
						{
						case svn_wc_conflict_action_edit:
							sConflictAction.Format(IDS_TREECONFLICT_FILESWITCHEDIT, (LPCTSTR)sItemName);
							sResolveTheirs.LoadString(IDS_TREECONFLICT_RESOLVE_KEEPREPOSITORYFILE);
							break;
						case svn_wc_conflict_action_add:
							sConflictAction.Format(IDS_TREECONFLICT_FILESWITCHADD, (LPCTSTR)sItemName);
							sResolveTheirs.LoadString(IDS_TREECONFLICT_RESOLVE_KEEPREPOSITORYFILE);
							break;
						case svn_wc_conflict_action_delete:
							sConflictAction.Format(IDS_TREECONFLICT_FILESWITCHDELETE, (LPCTSTR)sItemName);
							sResolveTheirs.LoadString(IDS_TREECONFLICT_RESOLVE_REMOVEFILE);
							break;
						}
						break;
					case svn_wc_operation_merge:
						switch (pInfoData->treeconflict_action)
						{
						case svn_wc_conflict_action_edit:
							sConflictAction.Format(IDS_TREECONFLICT_FILEMERGEEDIT, (LPCTSTR)sItemName);
							sResolveTheirs.LoadString(IDS_TREECONFLICT_RESOLVE_KEEPREPOSITORYFILE);
							break;
						case svn_wc_conflict_action_add:
							sResolveTheirs.Format(IDS_TREECONFLICT_FILEMERGEADD, (LPCTSTR)sItemName);
							sResolveTheirs.LoadString(IDS_TREECONFLICT_RESOLVE_KEEPREPOSITORYFILE);
							break;
						case svn_wc_conflict_action_delete:
							sConflictAction.Format(IDS_TREECONFLICT_FILEMERGEDELETE, (LPCTSTR)sItemName);
							sResolveTheirs.LoadString(IDS_TREECONFLICT_RESOLVE_REMOVEFILE);
							break;
						}
						break;
					}
				}
				else if (pInfoData->treeconflict_nodekind == svn_node_dir)
				{
					switch (pInfoData->treeconflict_operation)
					{
					case svn_wc_operation_update:
						switch (pInfoData->treeconflict_action)
						{
						case svn_wc_conflict_action_edit:
							sConflictAction.Format(IDS_TREECONFLICT_DIRUPDATEEDIT, (LPCTSTR)sItemName);
							sResolveTheirs.LoadString(IDS_TREECONFLICT_RESOLVE_KEEPREPOSITORYDIR);
							break;
						case svn_wc_conflict_action_add:
							sConflictAction.Format(IDS_TREECONFLICT_DIRUPDATEADD, (LPCTSTR)sItemName);
							sResolveTheirs.LoadString(IDS_TREECONFLICT_RESOLVE_KEEPREPOSITORYDIR);
							break;
						case svn_wc_conflict_action_delete:
							sConflictAction.Format(IDS_TREECONFLICT_DIRUPDATEDELETE, (LPCTSTR)sItemName);
							sResolveTheirs.LoadString(IDS_TREECONFLICT_RESOLVE_REMOVEDIR);
							break;
						}
						break;
					case svn_wc_operation_switch:
						switch (pInfoData->treeconflict_action)
						{
						case svn_wc_conflict_action_edit:
							sConflictAction.Format(IDS_TREECONFLICT_DIRSWITCHEDIT, (LPCTSTR)sItemName);
							sResolveTheirs.LoadString(IDS_TREECONFLICT_RESOLVE_KEEPREPOSITORYDIR);
							break;
						case svn_wc_conflict_action_add:
							sConflictAction.Format(IDS_TREECONFLICT_DIRSWITCHADD, (LPCTSTR)sItemName);
							sResolveTheirs.LoadString(IDS_TREECONFLICT_RESOLVE_KEEPREPOSITORYDIR);
							break;
						case svn_wc_conflict_action_delete:
							sConflictAction.Format(IDS_TREECONFLICT_DIRSWITCHDELETE, (LPCTSTR)sItemName);
							sResolveTheirs.LoadString(IDS_TREECONFLICT_RESOLVE_REMOVEDIR);
							break;
						}
						break;
					case svn_wc_operation_merge:
						switch (pInfoData->treeconflict_action)
						{
						case svn_wc_conflict_action_edit:
							sConflictAction.Format(IDS_TREECONFLICT_DIRMERGEEDIT, (LPCTSTR)sItemName);
							sResolveTheirs.LoadString(IDS_TREECONFLICT_RESOLVE_KEEPREPOSITORYDIR);
							break;
						case svn_wc_conflict_action_add:
							sConflictAction.Format(IDS_TREECONFLICT_DIRMERGEADD, (LPCTSTR)sItemName);
							sResolveTheirs.LoadString(IDS_TREECONFLICT_RESOLVE_KEEPREPOSITORYDIR);
							break;
						case svn_wc_conflict_action_delete:
							sConflictAction.Format(IDS_TREECONFLICT_DIRMERGEDELETE, (LPCTSTR)sItemName);
							sResolveTheirs.LoadString(IDS_TREECONFLICT_RESOLVE_REMOVEDIR);
							break;
						}
						break;
					}
				}

				UINT uReasonID = 0;
				switch (pInfoData->treeconflict_reason)
				{
				case svn_wc_conflict_reason_edited:
					uReasonID = IDS_TREECONFLICT_REASON_EDITED;
					sResolveMine.LoadString(pInfoData->treeconflict_nodekind == svn_node_dir ? IDS_TREECONFLICT_RESOLVE_KEEPLOCALDIR : IDS_TREECONFLICT_RESOLVE_KEEPLOCALFILE);
					break;
				case svn_wc_conflict_reason_obstructed:
					uReasonID = IDS_TREECONFLICT_REASON_OBSTRUCTED;
					sResolveMine.LoadString(pInfoData->treeconflict_nodekind == svn_node_dir ? IDS_TREECONFLICT_RESOLVE_KEEPLOCALDIR : IDS_TREECONFLICT_RESOLVE_KEEPLOCALFILE);
					break;
				case svn_wc_conflict_reason_deleted:
					uReasonID = IDS_TREECONFLICT_REASON_DELETED;
					sResolveMine.LoadString(pInfoData->treeconflict_nodekind == svn_node_dir ? IDS_TREECONFLICT_RESOLVE_REMOVEDIR : IDS_TREECONFLICT_RESOLVE_REMOVEFILE);
					break;
				case svn_wc_conflict_reason_added:
					uReasonID = IDS_TREECONFLICT_REASON_ADDED;
					sResolveMine.LoadString(pInfoData->treeconflict_nodekind == svn_node_dir ? IDS_TREECONFLICT_RESOLVE_KEEPLOCALDIR : IDS_TREECONFLICT_RESOLVE_KEEPLOCALFILE);
					break;
				case svn_wc_conflict_reason_missing:
					uReasonID = IDS_TREECONFLICT_REASON_MISSING;
					sResolveMine.LoadString(pInfoData->treeconflict_nodekind == svn_node_dir ? IDS_TREECONFLICT_RESOLVE_REMOVEDIR : IDS_TREECONFLICT_RESOLVE_REMOVEFILE);
					break;
				case svn_wc_conflict_reason_unversioned:
					uReasonID = IDS_TREECONFLICT_REASON_UNVERSIONED;
					sResolveMine.LoadString(pInfoData->treeconflict_nodekind == svn_node_dir ? IDS_TREECONFLICT_RESOLVE_KEEPLOCALDIR : IDS_TREECONFLICT_RESOLVE_KEEPLOCALFILE);
					break;
				}
				sConflictReason.Format(uReasonID, (LPCTSTR)sConflictAction);

				CTreeConflictEditorDlg dlg;
				dlg.SetConflictInfoText(sConflictReason);
				dlg.SetResolveTexts(sResolveTheirs, sResolveMine);
				dlg.SetPath(treeConflictPath);
				INT_PTR dlgRet = dlg.DoModal();
				bRet = (dlgRet != IDCANCEL);
			}
		}
	}
#endif
	return bRet;
}

bool CAppUtils::IsSSHPutty()
{
	CString sshclient=g_Git.m_Environment.GetEnv(_T("GIT_SSH"));
	sshclient=sshclient.MakeLower();
	if(sshclient.Find(_T("plink.exe"),0)>=0)
	{
		return true;
	}
	return false;
}

CString CAppUtils::GetClipboardLink(const CString &skipGitPrefix, int paramsCount)
{
	if (!OpenClipboard(NULL))
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
		if(sClipboardText[0] == _T('\"') && sClipboardText[sClipboardText.GetLength()-1] == _T('\"'))
			sClipboardText=sClipboardText.Mid(1,sClipboardText.GetLength()-2);

		if(sClipboardText.Find( _T("http://")) == 0)
			return sClipboardText;

		if(sClipboardText.Find( _T("https://")) == 0)
			return sClipboardText;

		if(sClipboardText.Find( _T("git://")) == 0)
			return sClipboardText;

		if(sClipboardText.Find( _T("ssh://")) == 0)
			return sClipboardText;

		if (sClipboardText.Find(_T("git@")) == 0)
			return sClipboardText;

		if(sClipboardText.GetLength()>=2)
			if( sClipboardText[1] == _T(':') )
				if( (sClipboardText[0] >= 'A' &&  sClipboardText[0] <= 'Z')
					|| (sClipboardText[0] >= 'a' &&  sClipboardText[0] <= 'z') )
					return sClipboardText;

		// trim prefixes like "git clone "
		if (!skipGitPrefix.IsEmpty() && sClipboardText.Find(skipGitPrefix) == 0)
		{
			sClipboardText = sClipboardText.Mid(skipGitPrefix.GetLength()).Trim();
			int spacePos = -1;
			while (paramsCount >= 0)
			{
				--paramsCount;
				spacePos = sClipboardText.Find(_T(' '), spacePos + 1);
				if (spacePos == -1)
					break;
			}
			if (spacePos > 0 && paramsCount < 0)
				sClipboardText = sClipboardText.Left(spacePos);
			return sClipboardText;
		}
	}

	return CString(_T(""));
}

CString CAppUtils::ChooseRepository(const CString* path)
{
	CBrowseFolder browseFolder;
	CRegString  regLastResopitory = CRegString(_T("Software\\TortoiseGit\\TortoiseProc\\LastRepo"),_T(""));

	browseFolder.m_style = BIF_EDITBOX | BIF_NEWDIALOGSTYLE | BIF_RETURNFSANCESTORS | BIF_RETURNONLYFSDIRS;
	CString strCloneDirectory;
	if(path)
		strCloneDirectory=*path;
	else
	{
		strCloneDirectory = regLastResopitory;
	}

	CString title;
	title.LoadString(IDS_CHOOSE_REPOSITORY);

	browseFolder.SetInfo(title);

	if (browseFolder.Show(NULL, strCloneDirectory) == CBrowseFolder::OK)
	{
		regLastResopitory = strCloneDirectory;
		return strCloneDirectory;
	}
	else
	{
		return CString();
	}
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
		CString one=log.Tokenize(_T("\n"),start);
	else
		start = 0;

	while(start>=0)
	{
		CString one=log.Tokenize(_T("\n"),start);
		one=one.Trim();
		if(one.IsEmpty() || one.Find(_T("Success")) == 0)
			continue;
		one.Replace(_T('/'),_T('\\'));
		CTGitPath path;
		path.SetFromWin(one);
		list.AddPath(path);
	}
	if (!list.IsEmpty())
	{
		return SendPatchMail(list, bIsMainWnd);
	}
	else
	{
		CMessageBox::Show(NULL, IDS_ERR_NOPATCHES, IDS_APPNAME, MB_ICONINFORMATION);
		return true;
	}
}


int CAppUtils::GetLogOutputEncode(CGit *pGit)
{
	CString output;
	output = pGit->GetConfigValue(_T("i18n.logOutputEncoding"));
	if(output.IsEmpty())
		return CUnicodeUtils::GetCPCode(pGit->GetConfigValue(_T("i18n.commitencoding")));
	else
	{
		return CUnicodeUtils::GetCPCode(output);
	}
}
int CAppUtils::SaveCommitUnicodeFile(const CString& filename, CString &message)
{
	try
	{
		CFile file(filename, CFile::modeReadWrite | CFile::modeCreate);
		int cp = CUnicodeUtils::GetCPCode(g_Git.GetConfigValue(_T("i18n.commitencoding")));

		bool stripComments = (CRegDWORD(_T("Software\\TortoiseGit\\StripCommentedLines"), FALSE) == TRUE);

		if (CRegDWORD(_T("Software\\TortoiseGit\\SanitizeCommitMsg"), TRUE) == TRUE)
			message.TrimRight(L" \r\n");

		int len = message.GetLength();
		int start = 0;
		while (start >= 0 && start < len)
		{
			int oldStart = start;
			start = message.Find(L"\n", oldStart);
			CString line = message.Mid(oldStart);
			if (start != -1)
			{
				line = line.Left(start - oldStart);
				++start; // move forward so we don't find the same char again
			}
			if (stripComments && (line.GetLength() >= 1 && line.GetAt(0) == '#') || (start < 0 && line.IsEmpty()))
				continue;
			line.TrimRight(L" \r");
			CStringA lineA = CUnicodeUtils::GetMulti(line + L"\n", cp);
			file.Write(lineA.GetBuffer(), lineA.GetLength());
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

bool CAppUtils::Pull(bool showPush, bool showStashPop)
{
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
							TRUE); // Rebase after fetching

		CString url = dlg.m_RemoteURL;

		if (dlg.m_bAutoLoad)
		{
			CAppUtils::LaunchPAgent(NULL, &dlg.m_RemoteURL);
		}

		CString cmd;
		CGitHash hashOld;
		if (g_Git.GetHash(hashOld, _T("HEAD")))
		{
			MessageBox(NULL, g_Git.GetGitLastErr(_T("Could not get HEAD hash.")), _T("TortoiseGit"), MB_ICONERROR);
			return false;
		}

		CString cmdRebase;
		CString noff;
		CString ffonly;
		CString squash;
		CString nocommit;
		CString depth;
		CString notags;
		CString prune;

		if (!dlg.m_bFetchTags)
			notags = _T("--no-tags ");

		if (dlg.m_bFetchTags == TRUE)
			notags = _T("--tags ");

		if (dlg.m_bNoFF)
			noff=_T("--no-ff ");

		if (dlg.m_bFFonly)
			ffonly = _T("--ff-only ");

		if (dlg.m_bSquash)
			squash = _T("--squash ");

		if (dlg.m_bNoCommit)
			nocommit = _T("--no-commit ");

		if (dlg.m_bDepth)
			depth.Format(_T("--depth %d "), dlg.m_nDepth);

		int ver = CAppUtils::GetMsysgitVersion();

		if (dlg.m_bPrune == TRUE)
			prune = _T("--prune ");
		else if (dlg.m_bPrune == FALSE && ver >= 0x01080500)
			prune = _T("--no-prune ");

		if(ver >= 0x01070203) //above 1.7.0.2
			cmdRebase += _T("--progress ");

		cmd.Format(_T("git.exe pull -v --no-rebase %s%s%s%s%s%s%s%s\"%s\" %s"), cmdRebase, noff, ffonly, squash, nocommit, depth, notags, prune, url, dlg.m_RemoteBranchName);
		CProgressDlg progress;
		progress.m_GitCmd = cmd;

		CGitHash hashNew; // declare outside lambda, because it is captured by reference
		progress.m_PostCmdCallback = [&](DWORD status, PostCmdList& postCmdList)
		{
			if (status)
			{
				postCmdList.push_back(PostCmd(IDI_PULL, IDS_MENUPULL, [&]{ Pull(); }));
				postCmdList.push_back(PostCmd(IDI_COMMIT, IDS_MENUSTASHSAVE, [&]{ StashSave(_T(""), true); }));
				return;
			}

			if (showStashPop)
				postCmdList.push_back(PostCmd(IDI_RELOCATE, IDS_MENUSTASHPOP, []{ StashPop(); }));

			if (g_Git.GetHash(hashNew, _T("HEAD")))
				MessageBox(nullptr, g_Git.GetGitLastErr(_T("Could not get HEAD hash after pulling.")), _T("TortoiseGit"), MB_ICONERROR);
			else
			{
				postCmdList.push_back(PostCmd(IDI_DIFF, IDS_PROC_PULL_DIFFS, [&]
				{
					CFileDiffDlg dlg;
					dlg.SetDiff(NULL, hashNew.ToString(), hashOld.ToString());
					dlg.DoModal();
				}));
				postCmdList.push_back(PostCmd(IDI_LOG, IDS_PROC_PULL_LOG, [&]
				{
					CLogDlg dlg;
					dlg.SetParams(CTGitPath(_T("")), CTGitPath(_T("")), _T(""), hashOld.ToString() + _T("..") + hashNew.ToString(), 0);
					dlg.DoModal();
				}));
			}

			if (showPush)
				postCmdList.push_back(PostCmd(IDI_PUSH, IDS_MENUPUSH, []{ Push(_T("")); }));

			CTGitPath gitPath = g_Git.m_CurrentDir;
			if (gitPath.HasSubmodules())
			{
				postCmdList.push_back(PostCmd(IDI_UPDATE, IDS_PROC_SUBMODULESUPDATE, []
				{
					CString sCmd;
					sCmd.Format(_T("/command:subupdate /bkpath:\"%s\""), g_Git.m_CurrentDir);
					CAppUtils::RunTortoiseGitProc(sCmd);
				}));
			}
		};

		INT_PTR ret = progress.DoModal();

		if (ret == IDOK && progress.m_GitStatus == 1 && progress.m_LogText.Find(_T("CONFLICT")) >= 0 && CMessageBox::Show(NULL, IDS_SEECHANGES, IDS_APPNAME, MB_YESNO | MB_ICONINFORMATION) == IDYES)
		{
			CChangedDlg dlg;
			dlg.m_pathList.AddPath(CTGitPath());
			dlg.DoModal();

			return true;
		}

		return ret == IDOK;
	}

	return false;
}

bool CAppUtils::RebaseAfterFetch(const CString& upstream)
{
	while (true)
	{
		CRebaseDlg dlg;
		if (!upstream.IsEmpty())
			dlg.m_Upstream = upstream;
		dlg.m_PostButtonTexts.Add(CString(MAKEINTRESOURCE(IDS_MENULOG)));
		dlg.m_PostButtonTexts.Add(CString(MAKEINTRESOURCE(IDS_MENUDESSENDMAIL)));
		dlg.m_PostButtonTexts.Add(CString(MAKEINTRESOURCE(IDS_MENUREBASE)));
		INT_PTR response = dlg.DoModal();
		if (response == IDOK)
		{
			return true;
		}
		else if (response == IDC_REBASE_POST_BUTTON)
		{
			CString cmd = _T("/command:log");
			cmd += _T(" /path:\"") + g_Git.m_CurrentDir + _T("\"");
			CAppUtils::RunTortoiseGitProc(cmd);
			return true;
		}
		else if (response == IDC_REBASE_POST_BUTTON + 1)
		{
			CString cmd, out, err;
			cmd.Format(_T("git.exe format-patch -o \"%s\" %s..%s"),
				g_Git.m_CurrentDir,
				g_Git.FixBranchName(dlg.m_Upstream),
				g_Git.FixBranchName(dlg.m_Branch));
			if (g_Git.Run(cmd, &out, &err, CP_UTF8))
			{
				CMessageBox::Show(NULL, out + L"\n" + err, _T("TortoiseGit"), MB_OK | MB_ICONERROR);
				return false;
			}
			CAppUtils::SendPatchMail(cmd, out);
			return true;
		}
		else if (response == IDC_REBASE_POST_BUTTON + 2)
			continue;
		else if (response == IDCANCEL)
			return false;
		return false;
	}
}

static bool DoFetch(const CString& url, const bool fetchAllRemotes, const bool loadPuttyAgent, const int prune, const bool bDepth, const int nDepth, const int fetchTags, const CString& remoteBranch, boolean runRebase)
{
	if (loadPuttyAgent)
	{
		if (fetchAllRemotes)
		{
			STRING_VECTOR list;
			g_Git.GetRemoteList(list);

			STRING_VECTOR::const_iterator it = list.begin();
			while (it != list.end())
			{
				CString remote(*it);
				CAppUtils::LaunchPAgent(NULL, &remote);
				++it;
			}
		}
		else
			CAppUtils::LaunchPAgent(NULL, &url);
	}

	CString cmd, arg;
	int ver = CAppUtils::GetMsysgitVersion();
	if (ver >= 0x01070203) //above 1.7.0.2
		arg += _T(" --progress");

	if (bDepth)
		arg.AppendFormat(_T(" --depth %d"), nDepth);

	if (prune == TRUE)
		arg += _T(" --prune");
	else if (prune == FALSE && ver >= 0x01080500)
		arg += _T(" --no-prune");

	if (fetchTags == 1)
		arg += _T(" --tags");
	else if (fetchTags == 0)
		arg += _T(" --no-tags");

	if (fetchAllRemotes)
		cmd.Format(_T("git.exe fetch --all -v%s"), arg);
	else
		cmd.Format(_T("git.exe fetch -v%s \"%s\" %s"), arg, url, remoteBranch);

	CProgressDlg progress;
	progress.m_PostCmdCallback = [&](DWORD status, PostCmdList& postCmdList)
	{
		if (status)
		{
			postCmdList.push_back(PostCmd(IDI_REFRESH, IDS_MSGBOX_RETRY, [&]{ DoFetch(url, fetchAllRemotes, loadPuttyAgent, prune, bDepth, nDepth, fetchTags, remoteBranch, runRebase); }));
			return;
		}

		postCmdList.push_back(PostCmd(IDI_LOG, IDS_MENULOG, []
		{
			CString cmd = _T("/command:log");
			cmd += _T(" /path:\"") + g_Git.m_CurrentDir + _T("\"");
			CAppUtils::RunTortoiseGitProc(cmd);
		}));

		postCmdList.push_back(PostCmd(IDI_REVERT, IDS_PROC_RESET, []
		{
			CString pullRemote, pullBranch;
			g_Git.GetRemoteTrackedBranchForHEAD(pullRemote, pullBranch);
			CString defaultUpstream;
			if (!pullRemote.IsEmpty() && !pullBranch.IsEmpty())
				defaultUpstream.Format(_T("remotes/%s/%s"), pullRemote, pullBranch);
			CAppUtils::GitReset(&defaultUpstream, 2);
		}));

		postCmdList.push_back(PostCmd(IDI_PULL, IDS_MENUFETCH, []{ CAppUtils::Fetch(); }));

		if (!runRebase && !GitAdminDir::IsBareRepo(g_Git.m_CurrentDir))
			postCmdList.push_back(PostCmd(IDI_REBASE, IDS_MENUREBASE, [&]{ runRebase = false; CAppUtils::RebaseAfterFetch(); }));
	};

	progress.m_GitCmd = cmd;
	INT_PTR userResponse;

	if (g_Git.UsingLibGit2(CGit::GIT_CMD_FETCH))
	{
		CGitProgressDlg gitdlg;
		FetchProgressCommand fetchProgressCommand;
		if (!fetchAllRemotes)
			fetchProgressCommand.SetUrl(url);
		gitdlg.SetCommand(&fetchProgressCommand);
		fetchProgressCommand.m_PostCmdCallback = progress.m_PostCmdCallback;
		fetchProgressCommand.SetAutoTag(fetchTags == 1 ? GIT_REMOTE_DOWNLOAD_TAGS_ALL : fetchTags == 2 ? GIT_REMOTE_DOWNLOAD_TAGS_AUTO : GIT_REMOTE_DOWNLOAD_TAGS_NONE);
		if (!fetchAllRemotes)
			fetchProgressCommand.SetRefSpec(remoteBranch);
		userResponse = gitdlg.DoModal();
		return userResponse == IDOK;
	}

	userResponse = progress.DoModal();
	if (!progress.m_GitStatus)
	{
		if (runRebase)
			return CAppUtils::RebaseAfterFetch();
	}

	return userResponse == IDOK;
}

bool CAppUtils::Fetch(const CString& remoteName, bool allRemotes)
{
	CPullFetchDlg dlg;
	dlg.m_PreSelectRemote = remoteName;
	dlg.m_IsPull=FALSE;
	dlg.m_bAllRemotes = allRemotes;

	if(dlg.DoModal()==IDOK)
		return DoFetch(dlg.m_RemoteURL, dlg.m_bAllRemotes == BST_CHECKED, dlg.m_bAutoLoad == BST_CHECKED, dlg.m_bPrune, dlg.m_bDepth == BST_CHECKED, dlg.m_nDepth, dlg.m_bFetchTags, dlg.m_RemoteBranchName, dlg.m_bRebase == BST_CHECKED);

	return false;
}

bool CAppUtils::Push(const CString& selectLocalBranch)
{
	CPushDlg dlg;
	dlg.m_BranchSourceName = selectLocalBranch;

	if (dlg.DoModal() == IDOK)
	{
		CString error;
		DWORD exitcode = 0xFFFFFFFF;
		if (CHooks::Instance().PrePush(g_Git.m_CurrentDir, exitcode, error))
		{
			if (exitcode)
			{
				CString temp;
				temp.Format(IDS_ERR_HOOKFAILED, (LPCTSTR)error);
				CMessageBox::Show(nullptr, temp, _T("TortoiseGit"), MB_OK | MB_ICONERROR);
				return false;
			}
		}

		CString arg;

		if(dlg.m_bPack)
			arg += _T("--thin ");
		if(dlg.m_bTags && !dlg.m_bPushAllBranches)
			arg += _T("--tags ");
		if(dlg.m_bForce)
			arg += _T("--force ");
		if (dlg.m_bForceWithLease)
			arg += _T("--force-with-lease ");
		if (dlg.m_bSetUpstream)
			arg += _T("--set-upstream ");
		if (dlg.m_RecurseSubmodules == 1)
			arg += _T("--recurse-submodules=check ");
		if (dlg.m_RecurseSubmodules == 2)
			arg += _T("--recurse-submodules=on-demand ");

		int ver = CAppUtils::GetMsysgitVersion();

		if(ver >= 0x01070203) //above 1.7.0.2
			arg += _T("--progress ");

		CProgressDlg progress;

		STRING_VECTOR remotesList;
		if (dlg.m_bPushAllRemotes)
			g_Git.GetRemoteList(remotesList);
		else
			remotesList.push_back(dlg.m_URL);

		for (unsigned int i = 0; i < remotesList.size(); ++i)
		{
			if (dlg.m_bAutoLoad)
				CAppUtils::LaunchPAgent(NULL, &remotesList[i]);

			CString cmd;
			if (dlg.m_bPushAllBranches)
			{
				cmd.Format(_T("git.exe push --all %s\"%s\""),
						arg,
						remotesList[i]);

				if (dlg.m_bTags)
				{
					progress.m_GitCmdList.push_back(cmd);
					cmd.Format(_T("git.exe push --tags %s\"%s\""), arg, remotesList[i]);
				}
			}
			else
			{
				cmd.Format(_T("git.exe push %s\"%s\" %s"),
						arg,
						remotesList[i],
						dlg.m_BranchSourceName);
				if (!dlg.m_BranchRemoteName.IsEmpty())
				{
					cmd += _T(":") + dlg.m_BranchRemoteName;
				}
			}
			progress.m_GitCmdList.push_back(cmd);
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
					MessageBox(nullptr, temp, _T("TortoiseGit"), MB_OK | MB_ICONERROR);
				}
			}

			if (status)
			{
				bool rejected = progress.GetLogText().Find(_T("! [rejected]")) > 0;
				if (rejected)
				{
					postCmdList.push_back(PostCmd(IDI_PULL, IDS_MENUPULL, []{ Pull(true); }));
					postCmdList.push_back(PostCmd(IDI_PULL, IDS_MENUFETCH, [&]{ Fetch(dlg.m_bPushAllRemotes ? _T("") : dlg.m_URL, !!dlg.m_bPushAllRemotes); }));
				}
				postCmdList.push_back(PostCmd(IDI_PUSH, IDS_MENUPUSH, [&]{ Push(selectLocalBranch); }));
				return;
			}

			postCmdList.push_back(PostCmd(IDS_PROC_REQUESTPULL, [&]{ RequestPull(dlg.m_BranchRemoteName); }));
			postCmdList.push_back(PostCmd(IDI_PUSH, IDS_MENUPUSH, [&]{ Push(selectLocalBranch); }));
			postCmdList.push_back(PostCmd(IDI_SWITCH, IDS_MENUSWITCH, [&]{ Switch(); }));
			if (!superprojectRoot.IsEmpty())
			{
				postCmdList.push_back(PostCmd(IDI_COMMIT, IDS_PROC_COMMIT_SUPERPROJECT, [&]
				{
					CString sCmd;
					sCmd.Format(_T("/command:commit /path:\"%s\""), superprojectRoot);
					RunTortoiseGitProc(sCmd);
				}));
			}
		};

		INT_PTR ret = progress.DoModal();
		return ret == IDOK;
	}
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
		cmd.Format(_T("git.exe request-pull %s \"%s\" %s"), dlg.m_StartRevision, dlg.m_RepositoryURL, dlg.m_EndRevision);

		CSysProgressDlg sysProgressDlg;
		sysProgressDlg.SetTitle(CString(MAKEINTRESOURCE(IDS_APPNAME)));
		sysProgressDlg.SetLine(1, CString(MAKEINTRESOURCE(IDS_PROC_CREATINGPULLREUQEST)));
		sysProgressDlg.SetLine(2, CString(MAKEINTRESOURCE(IDS_PROGRESSWAIT)));
		sysProgressDlg.SetShowProgressBar(false);
		sysProgressDlg.ShowModeless((HWND)NULL, true);

		CString tempFileName = GetTempFile();
		CString err;
		DeleteFile(tempFileName);
		CreateDirectory(tempFileName, NULL);
		tempFileName += _T("\\pullrequest.txt");
		if (g_Git.RunLogFile(cmd, tempFileName, &err))
		{
			CString msg;
			msg.LoadString(IDS_ERR_PULLREUQESTFAILED);
			CMessageBox::Show(NULL, msg + _T("\n") + err, _T("TortoiseGit"), MB_OK);
			return false;
		}

		if (sysProgressDlg.HasUserCancelled())
		{
			CMessageBox::Show(NULL, IDS_SVN_USERCANCELLED, IDS_APPNAME, MB_OK);
			::DeleteFile(tempFileName);
			return false;
		}

		sysProgressDlg.Stop();

		if (dlg.m_bSendMail)
		{
			CSendMailDlg dlg;
			dlg.m_PathList = CTGitPathList(CTGitPath(tempFileName));
			dlg.m_bCustomSubject = true;

			if (dlg.DoModal() == IDOK)
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

				CSendMailCombineable sendMailCombineable(dlg.m_To, dlg.m_CC, dlg.m_Subject, !!dlg.m_bAttachment, !!dlg.m_bCombine);
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

bool CAppUtils::CreateMultipleDirectory(const CString& szPath)
{
	CString strDir(szPath);
	if (strDir.GetAt(strDir.GetLength()-1)!=_T('\\'))
	{
		strDir.AppendChar(_T('\\'));
	}
	std::vector<CString> vPath;
	CString strTemp;
	bool bSuccess = false;

	for (int i=0;i<strDir.GetLength();++i)
	{
		if (strDir.GetAt(i) != _T('\\'))
		{
			strTemp.AppendChar(strDir.GetAt(i));
		}
		else
		{
			vPath.push_back(strTemp);
			strTemp.AppendChar(_T('\\'));
		}
	}

	for (auto vIter = vPath.begin(); vIter != vPath.end(); ++vIter)
	{
		bSuccess = CreateDirectory(*vIter, NULL) ? true : false;
	}

	return bSuccess;
}

void CAppUtils::RemoveTrailSlash(CString &path)
{
	if(path.IsEmpty())
		return ;

	// For URL, do not trim the slash just after the host name component.
	int index = path.Find(_T("://"));
	if (index >= 0)
	{
		index += 4;
		index = path.Find(_T('/'), index);
		if (index == path.GetLength() - 1)
			return;
	}

	while(path[path.GetLength()-1] == _T('\\') || path[path.GetLength()-1] == _T('/' ) )
	{
		path=path.Left(path.GetLength()-1);
		if(path.IsEmpty())
			return;
	}
}

bool CAppUtils::CheckUserData()
{
	while(g_Git.GetUserName().IsEmpty() || g_Git.GetUserEmail().IsEmpty())
	{
		if(CMessageBox::Show(NULL, IDS_PROC_NOUSERDATA, IDS_APPNAME, MB_YESNO| MB_ICONERROR) == IDYES)
		{
			CTGitPath path(g_Git.m_CurrentDir);
			CSettings dlg(IDS_PROC_SETTINGS_TITLE,&path);
			dlg.SetTreeViewMode(TRUE, TRUE, TRUE);
			dlg.SetTreeWidth(220);
			dlg.m_DefaultPage = _T("gitconfig");

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
//			if (parser.HasVal(_T("closeonend")))
//				progDlg.SetAutoClose(parser.GetLongVal(_T("closeonend")));
//			progDlg.SetCommand(CGitProgressDlg::GitProgress_Commit);
//			progDlg.SetOptions(dlg.m_bKeepLocks ? ProgOptKeeplocks : ProgOptNone);
//			progDlg.SetPathList(dlg.m_pathList);
//			progDlg.SetCommitMessage(dlg.m_sLogMessage);
//			progDlg.SetDepth(dlg.m_bRecursive ? Git_depth_infinity : svn_depth_empty);
//			progDlg.SetSelectedList(dlg.m_selectedPathList);
//			progDlg.SetItemCount(dlg.m_itemsCount);
//			progDlg.SetBugTraqProvider(dlg.m_BugTraqProvider);
//			progDlg.DoModal();
//			CRegDWORD err = CRegDWORD(_T("Software\\TortoiseGit\\ErrorOccurred"), FALSE);
//			err = (DWORD)progDlg.DidErrorsOccur();
//			bFailed = progDlg.DidErrorsOccur();
//			bRet = progDlg.DidErrorsOccur();
//			CRegDWORD bFailRepeat = CRegDWORD(_T("Software\\TortoiseGit\\CommitReopen"), FALSE);
//			if (DWORD(bFailRepeat)==0)
//				bFailed = false;		// do not repeat if the user chose not to in the settings.
		}
	}
	return true;
}


BOOL CAppUtils::SVNDCommit()
{
	CSVNDCommitDlg dcommitdlg;
	CString gitSetting = g_Git.GetConfigValue(_T("svn.rmdir"));
	if (gitSetting == _T("")) {
		if (dcommitdlg.DoModal() != IDOK)
		{
			return false;
		}
		else
		{
			if (dcommitdlg.m_remember)
			{
				if (dcommitdlg.m_rmdir)
				{
					gitSetting = _T("true");
				}
				else
				{
					gitSetting = _T("false");
				}
				if(g_Git.SetConfigValue(_T("svn.rmdir"),gitSetting))
				{
					CString msg;
					msg.Format(IDS_PROC_SAVECONFIGFAILED, _T("svn.rmdir"), gitSetting);
					CMessageBox::Show(NULL, msg, _T("TortoiseGit"), MB_OK | MB_ICONERROR);
				}
			}
		}
	}

	BOOL IsStash = false;
	if(!g_Git.CheckCleanWorkTree())
	{
		if (CMessageBox::Show(NULL, IDS_ERROR_NOCLEAN_STASH, IDS_APPNAME, 1, IDI_QUESTION, IDS_STASHBUTTON, IDS_ABORTBUTTON) == 1)
		{
			CSysProgressDlg sysProgressDlg;
			sysProgressDlg.SetTitle(CString(MAKEINTRESOURCE(IDS_APPNAME)));
			sysProgressDlg.SetLine(1, CString(MAKEINTRESOURCE(IDS_PROC_STASHRUNNING)));
			sysProgressDlg.SetLine(2, CString(MAKEINTRESOURCE(IDS_PROGRESSWAIT)));
			sysProgressDlg.SetShowProgressBar(false);
			sysProgressDlg.SetCancelMsg(IDS_PROGRS_INFOFAILED);
			sysProgressDlg.ShowModeless((HWND)NULL, true);

			CString cmd,out;
			cmd=_T("git.exe stash");
			if (g_Git.Run(cmd, &out, CP_UTF8))
			{
				sysProgressDlg.Stop();
				CMessageBox::Show(NULL,out,_T("TortoiseGit"),MB_OK);
				return false;
			}
			sysProgressDlg.Stop();

			IsStash =true;
		}
		else
		{
			return false;
		}
	}

	CProgressDlg progress;
	if (dcommitdlg.m_rmdir)
	{
		progress.m_GitCmd=_T("git.exe svn dcommit --rmdir");
	}
	else
	{
		progress.m_GitCmd=_T("git.exe svn dcommit");
	}
	if(progress.DoModal()==IDOK && progress.m_GitStatus == 0)
	{
		::DeleteFile(g_Git.m_CurrentDir + _T("\\sys$command"));
		if( IsStash)
		{
			if(CMessageBox::Show(NULL,IDS_DCOMMIT_STASH_POP,IDS_APPNAME,MB_YESNO|MB_ICONINFORMATION)==IDYES)
			{
				CSysProgressDlg sysProgressDlg;
				sysProgressDlg.SetTitle(CString(MAKEINTRESOURCE(IDS_APPNAME)));
				sysProgressDlg.SetLine(1, CString(MAKEINTRESOURCE(IDS_PROC_STASHRUNNING)));
				sysProgressDlg.SetLine(2, CString(MAKEINTRESOURCE(IDS_PROGRESSWAIT)));
				sysProgressDlg.SetShowProgressBar(false);
				sysProgressDlg.SetCancelMsg(IDS_PROGRS_INFOFAILED);
				sysProgressDlg.ShowModeless((HWND)NULL, true);

				CString cmd,out;
				cmd=_T("git.exe stash pop");
				if (g_Git.Run(cmd, &out, CP_UTF8))
				{
					sysProgressDlg.Stop();
					CMessageBox::Show(NULL,out,_T("TortoiseGit"),MB_OK);
					return false;
				}
				sysProgressDlg.Stop();
			}
			else
			{
				return false;
			}
		}
		return TRUE;
	}
	return FALSE;
}

BOOL CAppUtils::Merge(const CString* commit, bool showStashPop)
{
	if (!CheckUserData())
		return FALSE;

	CMergeDlg dlg;
	if(commit)
		dlg.m_initialRefName = *commit;

	if(dlg.DoModal()==IDOK)
	{
		CString cmd;
		CString args;

		if(dlg.m_bNoFF)
			args += _T(" --no-ff");

		if(dlg.m_bSquash)
			args += _T(" --squash");

		if(dlg.m_bNoCommit)
			args += _T(" --no-commit");

		if (dlg.m_bLog)
		{
			CString fmt;
			fmt.Format(_T(" --log=%d"), dlg.m_nLog);
			args += fmt;
		}

		if (!dlg.m_MergeStrategy.IsEmpty())
		{
			args += _T(" --strategy=") + dlg.m_MergeStrategy;
			if (!dlg.m_StrategyOption.IsEmpty())
			{
				args += _T(" --strategy-option=") + dlg.m_StrategyOption;
				if (!dlg.m_StrategyParam.IsEmpty())
					args += _T("=") + dlg.m_StrategyParam;
			}
		}

		if(!dlg.m_strLogMesage.IsEmpty())
		{
			CString logmsg = dlg.m_strLogMesage;
			logmsg.Replace(_T("\""), _T("\\\""));
			args += _T(" -m \"") + logmsg + _T("\"");
		}
		cmd.Format(_T("git.exe merge %s %s"), args, g_Git.FixBranchName(dlg.m_VersionName));

		CProgressDlg Prodlg;
		Prodlg.m_GitCmd = cmd;

		Prodlg.m_PostCmdCallback = [&](DWORD status, PostCmdList& postCmdList)
		{
			if (status)
			{
				int hasConflicts = g_Git.HasWorkingTreeConflicts();
				if (hasConflicts < 0)
					CMessageBox::Show(nullptr, g_Git.GetGitLastErr(L"Checking for conflicts failed.", CGit::GIT_CMD_CHECKCONFLICTS), _T("TortoiseGit"), MB_ICONEXCLAMATION);
				else if (hasConflicts)
				{
					// there are conflict files

					postCmdList.push_back(PostCmd(IDI_RESOLVE, IDS_PROGRS_CMD_RESOLVE, []
					{
						CString sCmd;
						sCmd.Format(_T("/command:commit /path:\"%s\""), g_Git.m_CurrentDir);
						CAppUtils::RunTortoiseGitProc(sCmd);
					}));
				}

				postCmdList.push_back(PostCmd(IDI_COMMIT, IDS_MENUSTASHSAVE, [&]{ CAppUtils::StashSave(_T(""), false, false, true, g_Git.FixBranchName(dlg.m_VersionName)); }));
				return;
			}

			if (showStashPop)
				postCmdList.push_back(PostCmd(IDI_RELOCATE, IDS_MENUSTASHPOP, []{ StashPop(); }));

			if (dlg.m_bNoCommit)
			{
				postCmdList.push_back(PostCmd(IDI_COMMIT, IDS_MENUCOMMIT, []
				{
					CString sCmd;
					sCmd.Format(_T("/command:commit /path:\"%s\""), g_Git.m_CurrentDir);
					CAppUtils::RunTortoiseGitProc(sCmd);
				}));
				return;
			}

			if (dlg.m_bIsBranch && dlg.m_VersionName.Find(L"remotes/") == -1) // do not ask to remove remote branches
			{
				postCmdList.push_back(PostCmd(IDI_DELETE, IDS_PROC_REMOVEBRANCH, [&]
				{
					CString msg;
					msg.Format(IDS_PROC_DELETEBRANCHTAG, dlg.m_VersionName);
					if (CMessageBox::Show(nullptr, msg, _T("TortoiseGit"), 2, IDI_QUESTION, CString(MAKEINTRESOURCE(IDS_DELETEBUTTON)), CString(MAKEINTRESOURCE(IDS_ABORTBUTTON))) == 1)
					{
						CString cmd, out;
						cmd.Format(_T("git.exe branch -D -- %s"), dlg.m_VersionName);
						if (g_Git.Run(cmd, &out, CP_UTF8))
							MessageBox(nullptr, out, _T("TortoiseGit"), MB_OK);
					}
				}));
			}
			if (dlg.m_bIsBranch)
				postCmdList.push_back(PostCmd(IDI_PUSH, IDS_MENUPUSH, []{ Push(); }));

			BOOL hasGitSVN = CTGitPath(g_Git.m_CurrentDir).GetAdminDirMask() & ITEMIS_GITSVN;
			if (hasGitSVN)
				postCmdList.push_back(PostCmd(IDI_COMMIT, IDS_MENUSVNDCOMMIT, []{ SVNDCommit(); }));
		};

		Prodlg.DoModal();
		return !Prodlg.m_GitStatus;
	}
	return false;
}

BOOL CAppUtils::MergeAbort()
{
	CMergeAbortDlg dlg;
	if (dlg.DoModal() == IDOK)
		return Reset(_T("HEAD"), dlg.m_ResetType + 1);

	return FALSE;
}

void CAppUtils::EditNote(GitRev *rev)
{
	if (!CheckUserData())
		return;

	CInputDlg dlg;
	dlg.m_sHintText = CString(MAKEINTRESOURCE(IDS_PROGS_TITLE_EDITNOTES));
	dlg.m_sInputText = rev->m_Notes;
	dlg.m_sTitle = CString(MAKEINTRESOURCE(IDS_PROGS_TITLE_EDITNOTES));
	//dlg.m_pProjectProperties = &m_ProjectProperties;
	dlg.m_bUseLogWidth = true;
	if(dlg.DoModal() == IDOK)
	{
		CString cmd,output;
		cmd=_T("notes add -f -F \"");

		CString tempfile=::GetTempFile();
		if (CAppUtils::SaveCommitUnicodeFile(tempfile, dlg.m_sInputText))
		{
			CMessageBox::Show(nullptr, IDS_PROC_FAILEDSAVINGNOTES, IDS_APPNAME, MB_OK | MB_ICONERROR);
			return;
		}
		cmd += tempfile;
		cmd += _T("\" ");
		cmd += rev->m_CommitHash.ToString();

		try
		{
			if (git_run_cmd("notes", CUnicodeUtils::GetMulti(cmd, CP_UTF8).GetBuffer()))
			{
				CMessageBox::Show(NULL, IDS_PROC_FAILEDSAVINGNOTES, IDS_APPNAME, MB_OK | MB_ICONERROR);

			}
			else
			{
				rev->m_Notes = dlg.m_sInputText;
			}
		}catch(...)
		{
			CMessageBox::Show(NULL, IDS_PROC_FAILEDSAVINGNOTES, IDS_APPNAME, MB_OK | MB_ICONERROR);
		}
		::DeleteFile(tempfile);

	}
}

int CAppUtils::GetMsysgitVersion()
{
	if (g_Git.ms_LastMsysGitVersion)
		return g_Git.ms_LastMsysGitVersion;

	CString cmd;
	CString versiondebug;
	CString version;

	CRegDWORD regTime		= CRegDWORD(_T("Software\\TortoiseGit\\git_file_time"));
	CRegDWORD regVersion	= CRegDWORD(_T("Software\\TortoiseGit\\git_cached_version"));

	CString gitpath = CGit::ms_LastMsysGitDir+_T("\\git.exe");

	__int64 time=0;
	if (!g_Git.GetFileModifyTime(gitpath, &time))
	{
		if((DWORD)time == regTime)
		{
			g_Git.ms_LastMsysGitVersion = regVersion;
			return regVersion;
		}
	}

	CString err;
	cmd = _T("git.exe --version");
	if (g_Git.Run(cmd, &version, &err, CP_UTF8))
	{
		CMessageBox::Show(NULL, _T("git.exe not correctly set up (") + err + _T(")\nCheck TortoiseGit settings and consult help file for \"Git.exe Path\"."), _T("TortoiseGit"), MB_OK|MB_ICONERROR);
		return -1;
	}

	int start=0;
	int ver = 0;

	versiondebug = version;

	try
	{
		CString str=version.Tokenize(_T("."), start);
		int space = str.ReverseFind(_T(' '));
		str = str.Mid(space+1,start);
		ver = _ttol(str);
		ver <<=24;

		version = version.Mid(start);
		start = 0;

		str = version.Tokenize(_T("."), start);

		ver |= (_ttol(str) & 0xFF) << 16;

		str = version.Tokenize(_T("."), start);
		ver |= (_ttol(str) & 0xFF) << 8;

		str = version.Tokenize(_T("."), start);
		ver |= (_ttol(str) & 0xFF);
	}
	catch(...)
	{
		if (!ver)
		{
			CMessageBox::Show(NULL, _T("Could not parse git.exe version number: \"") + versiondebug + _T("\""), _T("TortoiseGit"), MB_OK | MB_ICONERROR);
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

	CAutoLibrary hShell = AtlLoadSystemLibraryUsingFullPath(_T("Shell32.dll"));

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
		if (CMessageBox::Show(NULL, IDS_ERROR_NOCLEAN_STASH, IDS_APPNAME, 1, IDI_QUESTION, IDS_STASHBUTTON, IDS_ABORTBUTTON) == 1)
		{
			CSysProgressDlg sysProgressDlg;
			sysProgressDlg.SetTitle(CString(MAKEINTRESOURCE(IDS_APPNAME)));
			sysProgressDlg.SetLine(1, CString(MAKEINTRESOURCE(IDS_PROC_STASHRUNNING)));
			sysProgressDlg.SetLine(2, CString(MAKEINTRESOURCE(IDS_PROGRESSWAIT)));
			sysProgressDlg.SetShowProgressBar(false);
			sysProgressDlg.SetCancelMsg(IDS_PROGRS_INFOFAILED);
			sysProgressDlg.ShowModeless((HWND)NULL, true);

			CString cmd, out;
			cmd = _T("git.exe stash");
			if (g_Git.Run(cmd, &out, CP_UTF8))
			{
				sysProgressDlg.Stop();
				CMessageBox::Show(NULL, out, _T("TortoiseGit"), MB_OK);
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
		progress.m_GitCmdList.push_back(_T("git.exe bisect start"));
		progress.m_GitCmdList.push_back(_T("git.exe bisect good ") + bisectStartDlg.m_LastGoodRevision);
		progress.m_GitCmdList.push_back(_T("git.exe bisect bad ") + bisectStartDlg.m_FirstBadRevision);

		progress.m_PostCmdCallback = [&](DWORD status, PostCmdList& postCmdList)
		{
			if (status)
				return;

			CTGitPath path(g_Git.m_CurrentDir);
			if (path.HasSubmodules())
			{
				postCmdList.push_back(PostCmd(IDI_UPDATE, IDS_PROC_SUBMODULESUPDATE, []
				{
					CString sCmd;
					sCmd.Format(_T("/command:subupdate /bkpath:\"%s\""), g_Git.m_CurrentDir);
					CAppUtils::RunTortoiseGitProc(sCmd);
				}));
			}
		};

		

		INT_PTR ret = progress.DoModal();
		return ret == IDOK;
	}

	return false;
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

		DWORD verificationError = VerifyServerCertificate(pServerCert, CUnicodeUtils::GetUnicode(host).GetBuffer(), 0);
		if (!verificationError)
		{
			last_accepted_cert.set(cert);
			CertFreeCertificateContext(pServerCert);
			return 0;
		}

		CString servernameInCert;
		CertGetNameString(pServerCert, CERT_NAME_SIMPLE_DISPLAY_TYPE, 0, nullptr, servernameInCert.GetBuffer(128), 128);
		servernameInCert.ReleaseBuffer();

		CString issuer;
		CertGetNameString(pServerCert, CERT_NAME_SIMPLE_DISPLAY_TYPE, CERT_NAME_ISSUER_FLAG, nullptr, issuer.GetBuffer(128), 128);
		issuer.ReleaseBuffer();

		CertFreeCertificateContext(pServerCert);

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

void CAppUtils::ExploreTo(HWND hwnd, CString path)
{
	if (PathFileExists(path))
	{
		ITEMIDLIST __unaligned * pidl = ILCreateFromPath(path);
		if (pidl)
		{
			SHOpenFolderAndSelectItems(pidl, 0, 0, 0);
			ILFree(pidl);
		}
		return;
	}
	// if filepath does not exist any more, navigate to closest matching folder
	do
	{
		int pos = path.ReverseFind(_T('\\'));
		if (pos <= 3)
			break;
		path = path.Left(pos);
	} while (!PathFileExists(path));
	ShellExecute(hwnd, _T("explore"), path, nullptr, nullptr, SW_SHOW);
}

int CAppUtils::ResolveConflict(CTGitPath& path, resolve_with resolveWith)
{
	bool b_local = false, b_remote = false;
	BYTE_VECTOR vector;
	{
		CString cmd;
		cmd.Format(_T("git.exe ls-files -u -t -z -- \"%s\""), path.GetGitPathString());
		if (g_Git.Run(cmd, &vector))
		{
			CMessageBox::Show(nullptr, _T("git ls-files failed!"), _T("TortoiseGit"), MB_OK);
			return -1;
		}

		CTGitPathList list;
		if (list.ParserFromLsFile(vector))
		{
			CMessageBox::Show(nullptr, _T("Parse ls-files failed!"), _T("TortoiseGit"), MB_OK);
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

	CBlockCacheForPath block(g_Git.m_CurrentDir);
	if (path.IsDirectory()) // is submodule conflict
	{
		CString err = _T("We're sorry, but you hit a very rare conflict condition with a submodule which cannot be resolved by TortoiseGit. You have to use the command line git for this.");
		if (b_local && b_remote)
		{
			if (!path.HasAdminDir()) // check if submodule is initialized
			{
				err += _T("\n\nYou have to checkout the submodule manually into \"") + path.GetGitPathString() + _T("\" and then reset HEAD to the right commit (see resolve submodule conflict dialog for this).");
				MessageBox(nullptr, err, _T("TortoiseGit"), MB_ICONERROR);
				return -1;
			}
			CGit subgit;
			subgit.m_CurrentDir = g_Git.CombinePath(path);
			CGitHash submoduleHead;
			if (subgit.GetHash(submoduleHead, _T("HEAD")))
			{
				MessageBox(nullptr, err, _T("TortoiseGit"), MB_ICONERROR);
				return -1;
			}
			CString baseHash, localHash, remoteHash;
			ParseHashesFromLsFile(vector, baseHash, localHash, remoteHash);
			if (resolveWith == RESOLVE_WITH_THEIRS && submoduleHead.ToString() != remoteHash)
			{
				CString origPath = g_Git.m_CurrentDir;
				g_Git.m_CurrentDir = g_Git.CombinePath(path);
				if (!GitReset(&remoteHash))
				{
					g_Git.m_CurrentDir = origPath;
					return -1;
				}
				g_Git.m_CurrentDir = origPath;
			}
			else if (resolveWith == RESOLVE_WITH_MINE && submoduleHead.ToString() != localHash)
			{
				CString origPath = g_Git.m_CurrentDir;
				g_Git.m_CurrentDir = g_Git.CombinePath(path);
				if (!GitReset(&localHash))
				{
					g_Git.m_CurrentDir = origPath;
					return -1;
				}
				g_Git.m_CurrentDir = origPath;
			}
		}
		else
		{
			MessageBox(nullptr, err, _T("TortoiseGit"), MB_ICONERROR);
			return -1;
		}
	}

	if (resolveWith == RESOLVE_WITH_THEIRS)
	{
		CString gitcmd, output;
		if (b_local && b_remote)
			gitcmd.Format(_T("git.exe checkout-index -f --stage=3 -- \"%s\""), path.GetGitPathString());
		else if (b_remote)
			gitcmd.Format(_T("git.exe add -f -- \"%s\""), path.GetGitPathString());
		else if (b_local)
			gitcmd.Format(_T("git.exe rm -f -- \"%s\""), path.GetGitPathString());
		if (g_Git.Run(gitcmd, &output, CP_UTF8))
		{
			CMessageBox::Show(nullptr, output, _T("TortoiseGit"), MB_ICONERROR);
			return -1;
		}
	}
	else if (resolveWith == RESOLVE_WITH_MINE)
	{
		CString gitcmd, output;
		if (b_local && b_remote)
			gitcmd.Format(_T("git.exe checkout-index -f --stage=2 -- \"%s\""), path.GetGitPathString());
		else if (b_local)
			gitcmd.Format(_T("git.exe add -f -- \"%s\""), path.GetGitPathString());
		else if (b_remote)
			gitcmd.Format(_T("git.exe rm -f -- \"%s\""), path.GetGitPathString());
		if (g_Git.Run(gitcmd, &output, CP_UTF8))
		{
			CMessageBox::Show(nullptr, output, _T("TortoiseGit"), MB_ICONERROR);
			return -1;
		}
	}

	if (b_local && b_remote && path.m_Action & CTGitPath::LOGACTIONS_UNMERGED)
	{
		CString gitcmd, output;
		gitcmd.Format(_T("git.exe add -f -- \"%s\""), path.GetGitPathString());
		if (g_Git.Run(gitcmd, &output, CP_UTF8))
			CMessageBox::Show(nullptr, output, _T("TortoiseGit"), MB_ICONERROR);
		else
		{
			path.m_Action |= CTGitPath::LOGACTIONS_MODIFIED;
			path.m_Action &= ~CTGitPath::LOGACTIONS_UNMERGED;
		}
	}

	RemoveTempMergeFile(path);
	return 0;
}

bool CAppUtils::ShellOpen(const CString& file, HWND hwnd /*= nullptr */)
{
	if ((int)ShellExecute(hwnd, NULL, file, NULL, NULL, SW_SHOW) > HINSTANCE_ERROR)
		return true;

	return ShowOpenWithDialog(file, hwnd);
}

bool CAppUtils::ShowOpenWithDialog(const CString& file, HWND hwnd /*= nullptr */)
{
	CAutoLibrary hShell = AtlLoadSystemLibraryUsingFullPath(_T("shell32.dll"));
	if (hShell)
	{
		typedef HRESULT STDAPICALLTYPE SHOpenWithDialoFN(_In_opt_ HWND hwndParent, _In_ const OPENASINFO *poainfo);
		SHOpenWithDialoFN *pfnSHOpenWithDialog = (SHOpenWithDialoFN*)GetProcAddress(hShell, "SHOpenWithDialog");
		if (pfnSHOpenWithDialog)
		{
			OPENASINFO oi = { 0 };
			oi.pcszFile = file;
			oi.oaifInFlags = OAIF_EXEC;
			return SUCCEEDED(pfnSHOpenWithDialog(hwnd, &oi));
		}
	}
	CString cmd = _T("RUNDLL32 Shell32,OpenAs_RunDLL ");
	cmd += file;
	return CAppUtils::LaunchApplication(cmd, NULL, false);
}
