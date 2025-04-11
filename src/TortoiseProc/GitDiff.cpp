﻿// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2003-2008 - TortoiseSVN
// Copyright (C) 2008-2020, 2023, 2025 - TortoiseGit

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
#include "GitDiff.h"
#include "TortoiseProc.h"
#include "AppUtils.h"
#include "gittype.h"
#include "resource.h"
#include "MessageBox.h"
#include "FileDiffDlg.h"
#include "SubmoduleDiffDlg.h"
#include "TempFile.h"

int CGitDiff::SubmoduleDiffNull(HWND hWnd, const CTGitPath* pPath, const CGitHash& hash)
{
	CString newsub;
	CGitHash newhash;

	CString cmd;
	if (!hash.IsEmpty())
		cmd.Format(L"git.exe ls-tree \"%s\" -- \"%s\"", static_cast<LPCWSTR>(hash.ToString()), static_cast<LPCWSTR>(pPath->GetGitPathString()));
	else
		cmd.Format(L"git.exe ls-files -s -- \"%s\"", static_cast<LPCWSTR>(pPath->GetGitPathString()));

	CString output, err;
	if (g_Git.Run(cmd, &output, &err, CP_UTF8))
	{
		CMessageBox::Show(hWnd, output + L'\n' + err, L"TortoiseGit", MB_OK | MB_ICONERROR);
		return -1;
	}

	int start = output.Find(L' ');
	if(start>0)
	{
		if (!hash.IsEmpty()) // in ls-files the hash is in the second column; in ls-tree it's in the third one
			start = output.Find(L' ', start + 1);
		if(start>0)
			newhash = CGitHash::FromHexStr(output.Mid(start + 1, CGitHash::HashLength(g_Git.GetCurrentRepoHashType()) * 2), g_Git.GetCurrentRepoHashType());

		CGit subgit;
		subgit.m_IsUseGitDLL = false;
		subgit.m_CurrentDir = g_Git.CombinePath(pPath);
		const int encode = CAppUtils::GetLogOutputEncode(&subgit);

		cmd.Format(L"git.exe log -n1 --pretty=format:\"%%s\" %s --", static_cast<LPCWSTR>(newhash.ToString()));
		const bool toOK = !subgit.Run(cmd, &newsub, encode);

		bool dirty = false;
		if (hash.IsEmpty() && !(pPath->m_Action & CTGitPath::LOGACTIONS_DELETED))
		{
			CString dirtyList;
			subgit.Run(L"git.exe status --porcelain", &dirtyList, encode);
			dirty = !dirtyList.IsEmpty();
		}

		CSubmoduleDiffDlg submoduleDiffDlg(GetExplorerHWND() == hWnd ? nullptr : CWnd::FromHandle(hWnd));
		if (pPath->m_Action & CTGitPath::LOGACTIONS_DELETED)
			submoduleDiffDlg.SetDiff(pPath->GetWinPath(), false, newhash, newsub, toOK, CGitHash(), L"", false, dirty, ChangeType::DeleteSubmodule);
		else
			submoduleDiffDlg.SetDiff(pPath->GetWinPath(), false, CGitHash(), L"", true, newhash, newsub, toOK, dirty, ChangeType::NewSubmodule);
		submoduleDiffDlg.DoModal();
		if (submoduleDiffDlg.IsRefresh())
			return 1;

		return 0;
	}

	if (!hash.IsEmpty())
		CMessageBox::Show(hWnd, L"ls-tree output format error", L"TortoiseGit", MB_OK | MB_ICONERROR);
	else
		CMessageBox::Show(hWnd, L"ls-files output format error", L"TortoiseGit", MB_OK | MB_ICONERROR);
	return -1;
}

int CGitDiff::DiffNull(HWND hWnd, const CTGitPath* pPath, const CString& rev1, bool bIsAdd, int jumpToLine, bool bAlternative)
{
	CGitHash rev1Hash;
	if (rev1 != GitRev::GetWorkingCopyRef(g_Git.GetCurrentRepoHashType()))
	{
		if (g_Git.GetHash(rev1Hash, rev1 + L"^{}")) // make sure we have a HASH here, otherwise filenames might be invalid, also add ^{} in order to dereference signed tags
		{
			MessageBox(hWnd, g_Git.GetGitLastErr(L"Could not get hash of \"" + rev1 + L"^{}\"."), L"TortoiseGit", MB_ICONERROR);
			return -1;
		}
	}
	CString file1;
	CString nullfile;
	CString cmd;

	if(pPath->IsDirectory())
	{
		int result;
		// refresh if result = 1
		CTGitPath path = *pPath;
		while ((result = SubmoduleDiffNull(hWnd, &path, rev1Hash)) == 1)
			path.SetFromGit(pPath->GetGitPathString());
		return result;
	}

	if (!rev1Hash.IsEmpty())
	{
		file1 = CTempFiles::Instance().GetTempFilePath(false, *pPath, rev1Hash).GetWinPathString();
		if (g_Git.GetOneFile(rev1Hash.ToString(), *pPath, file1))
		{
			CString out;
			out.FormatMessage(IDS_STATUSLIST_CHECKOUTFILEFAILED, static_cast<LPCWSTR>(pPath->GetGitPathString()), static_cast<LPCWSTR>(rev1Hash.ToString()), static_cast<LPCWSTR>(file1));
			CMessageBox::Show(hWnd, g_Git.GetGitLastErr(out, CGit::GIT_CMD_GETONEFILE), L"TortoiseGit", MB_OK);
			return -1;
		}
		::SetFileAttributes(file1, FILE_ATTRIBUTE_READONLY);
	}
	else
		file1 = g_Git.CombinePath(pPath);

	CString tempfile = CTempFiles::Instance().GetTempFilePath(false, *pPath, rev1Hash).GetWinPathString();
	::SetFileAttributes(tempfile, FILE_ATTRIBUTE_READONLY);

	const auto flags = CAppUtils::DiffFlags().AlternativeTool(bAlternative);
	if(bIsAdd)
		CAppUtils::StartExtDiff(tempfile,file1,
							pPath->GetGitPathString(),
							pPath->GetGitPathString() + L':' + rev1Hash.ToString(g_Git.GetShortHASHLength()),
							g_Git.CombinePath(pPath), g_Git.CombinePath(pPath),
							CGitHash(), rev1Hash
							, flags, jumpToLine);
	else
		CAppUtils::StartExtDiff(file1,tempfile,
							pPath->GetGitPathString() + L':' + rev1Hash.ToString(g_Git.GetShortHASHLength()),
							pPath->GetGitPathString(),
							g_Git.CombinePath(pPath), g_Git.CombinePath(pPath),
							rev1Hash, CGitHash()
							, flags, jumpToLine);

	return 0;
}

int CGitDiff::SubmoduleDiff(HWND hWnd, const CTGitPath* pPath, const CTGitPath* /*pPath2*/, const CGitHash& rev1, const CGitHash& rev2, bool /*blame*/, bool /*unified*/)
{
	CGitHash oldhash;
	CGitHash newhash;
	bool dirty = false;
	CString cmd;
	bool isWorkingCopy = false;
	if (rev2.IsEmpty() || rev1.IsEmpty())
	{
		CGitHash rev;
		if (!rev2.IsEmpty())
			rev = rev2;
		if (!rev1.IsEmpty())
			rev = rev1;

		isWorkingCopy = true;

		cmd.Format(L"git.exe diff --submodule=short %s -- \"%s\"",
		static_cast<LPCWSTR>(rev.ToString()), static_cast<LPCWSTR>(pPath->GetGitPathString()));

		CString output, err;
		if (g_Git.Run(cmd, &output, &err, CP_UTF8))
		{
			CMessageBox::Show(hWnd, output + L'\n' + err, L"TortoiseGit", MB_OK | MB_ICONERROR);
			return -1;
		}

		if (output.IsEmpty())
		{
			output.Empty();
			err.Empty();
			// also compare against index
			cmd.Format(L"git.exe diff --submodule=short -- \"%s\"", static_cast<LPCWSTR>(pPath->GetGitPathString()));
			if (g_Git.Run(cmd, &output, &err, CP_UTF8))
			{
				CMessageBox::Show(hWnd, output + L'\n' + err, L"TortoiseGit", MB_OK | MB_ICONERROR);
				return -1;
			}

			if (output.IsEmpty())
			{
				CMessageBox::Show(hWnd, IDS_ERR_EMPTYDIFF, IDS_APPNAME, MB_OK | MB_ICONERROR);
				return -1;
			}
			else if (CMessageBox::Show(hWnd, IDS_SUBMODULE_EMPTYDIFF, IDS_APPNAME, 1, IDI_QUESTION, IDS_MSGBOX_YES, IDS_MSGBOX_NO) == 1)
			{
				CString sCmd;
				sCmd.Format(L"/command:subupdate /bkpath:\"%s\"", static_cast<LPCWSTR>(g_Git.m_CurrentDir));
				CAppUtils::RunTortoiseGitProc(sCmd);
			}
			return -1;
		}

		int start =0;
		const int oldstart = output.Find(L"-Subproject commit", start);
		if(oldstart<0)
		{
			CMessageBox::Show(hWnd, L"Subproject Diff Format error", L"TortoiseGit", MB_OK | MB_ICONERROR);
			return -1;
		}
		oldhash = CGitHash::FromHexStr(output.Mid(oldstart + static_cast<int>(wcslen(L"-Subproject commit")) + 1, CGitHash::HashLength(g_Git.GetCurrentRepoHashType()) * 2), g_Git.GetCurrentRepoHashType());
		start = 0;
		const int newstart = output.Find(L"+Subproject commit", start);
		if (newstart < 0)
		{
			CMessageBox::Show(hWnd, L"Subproject Diff Format error", L"TortoiseGit", MB_OK | MB_ICONERROR);
			return -1;
		}
		newhash = CGitHash::FromHexStr(output.Mid(newstart + static_cast<int>(wcslen(L"+Subproject commit")) + 1, CGitHash::HashLength(g_Git.GetCurrentRepoHashType()) * 2), g_Git.GetCurrentRepoHashType());
		dirty = output.Mid(newstart + static_cast<int>(wcslen(L"+Subproject commit")) + CGitHash::HashLength(g_Git.GetCurrentRepoHashType()) * 2 + 1) == L"-dirty\n";
	}
	else
	{
		cmd.Format(L"git.exe diff-tree -r -z %s %s -- \"%s\"",
		static_cast<LPCWSTR>(rev2.ToString()), static_cast<LPCWSTR>(rev1.ToString()), static_cast<LPCWSTR>(pPath->GetGitPathString()));

		BYTE_VECTOR bytes, errBytes;
		if(g_Git.Run(cmd, &bytes, &errBytes))
		{
			CString err;
			CGit::StringAppend(err, errBytes.data(), CP_UTF8);
			CMessageBox::Show(hWnd, err, L"TortoiseGit", MB_OK | MB_ICONERROR);
			return -1;
		}

		const auto hashLength = 2 * CGitHash::HashLength(g_Git.GetCurrentRepoHashType());
		if (bytes.size() < strlen(":000000 000000 ") + static_cast<size_t>(hashLength) + 1 + static_cast<size_t>(hashLength))
		{
			CMessageBox::Show(hWnd, L"git diff-tree gives invalid output", L"TortoiseGit", MB_OK | MB_ICONERROR);
			return -1;
		}
		oldhash = CGitHash::FromHexStr(std::string_view(&bytes[strlen(":000000 000000 ")], hashLength), g_Git.GetCurrentRepoHashType());
		newhash = CGitHash::FromHexStr(std::string_view(&bytes[strlen(":000000 000000 ") + hashLength + 1], hashLength), g_Git.GetCurrentRepoHashType());
	}

	CString oldsub;
	CString newsub;
	bool oldOK = false, newOK = false;

	CGit subgit;
	subgit.m_IsUseGitDLL = false;
	subgit.m_CurrentDir = g_Git.CombinePath(pPath);
	ChangeType changeType = ChangeType::Unknown;

	if (CTGitPath(subgit.m_CurrentDir).HasAdminDir())
		GetSubmoduleChangeType(subgit, oldhash, newhash, oldOK, newOK, changeType, oldsub, newsub);

	CSubmoduleDiffDlg submoduleDiffDlg(GetExplorerHWND() == hWnd ? nullptr : CWnd::FromHandle(hWnd));
	submoduleDiffDlg.SetDiff(pPath->GetWinPath(), isWorkingCopy, oldhash, oldsub, oldOK, newhash, newsub, newOK, dirty, changeType);
	submoduleDiffDlg.DoModal();
	if (submoduleDiffDlg.IsRefresh())
		return 1;

	return 0;
}

void CGitDiff::GetSubmoduleChangeType(CGit& subgit, const CGitHash& oldhash, const CGitHash& newhash, bool& oldOK, bool& newOK, ChangeType& changeType, CString& oldsub, CString& newsub)
{
	CString cmd;
	const int encode = CAppUtils::GetLogOutputEncode(&subgit);
	int oldTime = 0, newTime = 0;

	if (!oldhash.IsEmpty())
	{
		CString cmdout, cmderr;
		cmd.Format(L"git.exe log -n1 --pretty=format:\"%%ct %%s\" %s --", static_cast<LPCWSTR>(oldhash.ToString()));
		oldOK = !subgit.Run(cmd, &cmdout, &cmderr, encode);
		if (oldOK)
		{
			int pos = cmdout.Find(L' ');
			oldTime = _wtoi(cmdout.Left(pos));
			oldsub = cmdout.Mid(pos + 1);
		}
		else
			oldsub = cmderr;
	}
	if (!newhash.IsEmpty())
	{
		CString cmdout, cmderr;
		cmd.Format(L"git.exe log -n1 --pretty=format:\"%%ct %%s\" %s --", static_cast<LPCWSTR>(newhash.ToString()));
		newOK = !subgit.Run(cmd, &cmdout, &cmderr, encode);
		if (newOK)
		{
			int pos = cmdout.Find(L' ');
			newTime = _wtoi(cmdout.Left(pos));
			newsub = cmdout.Mid(pos + 1);
		}
		else
			newsub = cmderr;
	}

	if (oldhash.IsEmpty())
	{
		oldOK = true;
		changeType = ChangeType::NewSubmodule;
	}
	else if (newhash.IsEmpty())
	{
		newOK = true;
		changeType = ChangeType::DeleteSubmodule;
	}
	else if (oldhash != newhash)
	{
		bool ffNewer = subgit.IsFastForward(oldhash.ToString(), newhash.ToString());
		if (!ffNewer)
		{
			bool ffOlder = subgit.IsFastForward(newhash.ToString(), oldhash.ToString());
			if (!ffOlder)
			{
				if (newTime > oldTime)
					changeType = ChangeType::NewerTime;
				else if (newTime < oldTime)
					changeType = ChangeType::OlderTime;
				else
					changeType = ChangeType::SameTime;
			}
			else
				changeType = ChangeType::Rewind;
		}
		else
			changeType = ChangeType::FastForward;
	}
	else if (oldhash == newhash)
		changeType = ChangeType::Identical;

	if (!oldOK || !newOK)
		changeType = ChangeType::Unknown;
}

int CGitDiff::Diff(HWND hWnd, const CTGitPath* pPath, const CTGitPath* pPath2, const CString& rev1, const CString& rev2, bool /*blame*/, bool /*unified*/, int jumpToLine, bool bAlternativeTool, bool mustExist)
{
	// make sure we have HASHes here, otherwise filenames might be invalid
	CGitHash rev1Hash;
	CGitHash rev2Hash;
	if (rev1 != GitRev::GetWorkingCopyRef(g_Git.GetCurrentRepoHashType()))
	{
		if (g_Git.GetHash(rev1Hash, rev1 + L"^{}")) // add ^{} in order to dereference signed tags
		{
			MessageBox(hWnd, g_Git.GetGitLastErr(L"Could not get hash of \"" + rev1 + L"^{}\"."), L"TortoiseGit", MB_ICONERROR);
			return -1;
		}
	}
	if (rev2 != GitRev::GetWorkingCopyRef(g_Git.GetCurrentRepoHashType()))
	{
		if (g_Git.GetHash(rev2Hash, rev2 + L"^{}")) // add ^{} in order to dereference signed tags
		{
			MessageBox(hWnd, g_Git.GetGitLastErr(L"Could not get hash of \"" + rev2 + L"^{}\"."), L"TortoiseGit", MB_ICONERROR);
			return -1;
		}
	}

	CString file1;
	CString title1;
	CString cmd;

	if(pPath->IsDirectory() || pPath2->IsDirectory())
	{
		int result;
		// refresh if result = 1
		CTGitPath path = *pPath;
		CTGitPath path2 = *pPath2;
		while ((result = SubmoduleDiff(hWnd, &path, &path2, rev1Hash, rev2Hash)) == 1)
		{
			path.SetFromGit(pPath->GetGitPathString());
			path2.SetFromGit(pPath2->GetGitPathString());
		}
		return result;
	}

	if (!rev1Hash.IsEmpty())
	{
		// use original file extension, an external diff tool might need it
		file1 = CTempFiles::Instance().GetTempFilePath(false, *pPath, rev1Hash).GetWinPathString();
		title1 = pPath->GetGitPathString() + L": " + rev1Hash.ToString(g_Git.GetShortHASHLength());
		auto ret = g_Git.GetOneFile(rev1Hash.ToString(), *pPath, file1);
		if (ret && !(!mustExist && ret == GIT_ENOTFOUND))
		{
			CString out;
			out.FormatMessage(IDS_STATUSLIST_CHECKOUTFILEFAILED, static_cast<LPCWSTR>(pPath->GetGitPathString()), static_cast<LPCWSTR>(rev1Hash.ToString()), static_cast<LPCWSTR>(file1));
			CMessageBox::Show(hWnd, g_Git.GetGitLastErr(out, CGit::GIT_CMD_GETONEFILE), L"TortoiseGit", MB_OK);
			return -1;
		}
		::SetFileAttributes(file1, FILE_ATTRIBUTE_READONLY);
	}
	else
	{
		if (PathIsRelative(pPath->GetWinPath()))
		{
			file1 = g_Git.CombinePath(pPath);
			title1.Format(IDS_DIFF_WCNAME, static_cast<LPCWSTR>(pPath->GetGitPathString()));
		}
		else
		{
			file1 = pPath->GetWinPath();
			title1 = pPath->GetWinPathString();
		}
		if (!PathFileExists(file1))
		{
			CString sMsg;
			sMsg.Format(IDS_PROC_DIFFERROR_FILENOTINWORKINGTREE, static_cast<LPCWSTR>(file1));
			if (MessageBox(hWnd, sMsg, L"TortoiseGit", MB_ICONEXCLAMATION | MB_YESNO) != IDYES)
				return 1;
			if (!CCommonAppUtils::FileOpenSave(file1, nullptr, IDS_SELECTFILE, IDS_COMMONFILEFILTER, true))
				return 1;
			title1 = file1;
		}
	}

	CString file2;
	CString title2;
	if (!rev2Hash.IsEmpty())
	{
		CTGitPath fileName = *pPath2;
		if (pPath2->m_Action & CTGitPath::LOGACTIONS_REPLACED)
			fileName = CTGitPath(pPath2->GetGitOldPathString());

		file2 = CTempFiles::Instance().GetTempFilePath(false, fileName, rev2Hash).GetWinPathString();
		title2 = fileName.GetGitPathString() + L": " + rev2Hash.ToString(g_Git.GetShortHASHLength());
		const auto ret = g_Git.GetOneFile(rev2Hash.ToString(), fileName, file2);
		if (ret && !(!mustExist && ret == GIT_ENOTFOUND))
		{
			CString out;
			out.FormatMessage(IDS_STATUSLIST_CHECKOUTFILEFAILED, static_cast<LPCWSTR>(pPath2->GetGitPathString()), static_cast<LPCWSTR>(rev2Hash.ToString()), static_cast<LPCWSTR>(file2));
			CMessageBox::Show(hWnd, g_Git.GetGitLastErr(out, CGit::GIT_CMD_GETONEFILE), L"TortoiseGit", MB_OK);
			return -1;
		}
		::SetFileAttributes(file2, FILE_ATTRIBUTE_READONLY);
	}
	else
	{
		if (PathIsRelative(pPath2->GetWinPath()))
		{
			file2 = g_Git.CombinePath(pPath2);
			title2.Format(IDS_DIFF_WCNAME, static_cast<LPCWSTR>(pPath2->GetGitPathString()));
		}
		else
		{
			file2 = pPath2->GetWinPath();
			title2 = pPath2->GetWinPathString();
		}
	}

	CAppUtils::StartExtDiff(file2,file1,
							title2,
							title1,
							g_Git.CombinePath(pPath2),
							g_Git.CombinePath(pPath),
							rev2Hash,
							rev1Hash,
							CAppUtils::DiffFlags().AlternativeTool(bAlternativeTool), jumpToLine);

	return 0;
}

int CGitDiff::DiffCommit(HWND hWnd, const CTGitPath& path, const GitRev* r1, const GitRev* r2, bool bAlternative)
{
	return DiffCommit(hWnd, path, path, r1, r2, bAlternative);
}

int CGitDiff::DiffCommit(HWND hWnd, const CTGitPath& path1, const CTGitPath& path2, const GitRev* r1, const GitRev* r2, bool bAlternative)
{
	if (path1.GetWinPathString().IsEmpty())
	{
		CFileDiffDlg dlg(GetExplorerHWND() == hWnd ? nullptr : CWnd::FromHandle(hWnd));
		dlg.SetDiff(nullptr, *r2, *r1);
		dlg.DoModal();
	}
	else if (path1.IsDirectory())
	{
		CFileDiffDlg dlg(GetExplorerHWND() == hWnd ? nullptr : CWnd::FromHandle(hWnd));
		dlg.SetDiff(&path1, *r2, *r1);
		dlg.DoModal();
	}
	else
		Diff(hWnd, &path1, &path2, r1->m_CommitHash.ToString(), r2->m_CommitHash.ToString(), false, false, 0, bAlternative);
	return 0;
}

int CGitDiff::DiffCommit(HWND hWnd, const CTGitPath& path, const CString& r1, const CString& r2, bool bAlternative)
{
	return DiffCommit(hWnd, path, path, r1, r2, bAlternative);
}

int CGitDiff::DiffCommit(HWND hWnd, const CTGitPath& path1, const CTGitPath& path2, const CString& r1, const CString& r2, bool bAlternative)
{
	if (path1.GetWinPathString().IsEmpty())
	{
		CFileDiffDlg dlg(GetExplorerHWND() == hWnd ? nullptr : CWnd::FromHandle(hWnd));
		dlg.SetDiff(nullptr, r2, r1);
		dlg.DoModal();
	}
	else if (path1.IsDirectory())
	{
		CFileDiffDlg dlg(GetExplorerHWND() == hWnd ? nullptr : CWnd::FromHandle(hWnd));
		dlg.SetDiff(&path1, r2, r1);
		dlg.DoModal();
	}
	else
		Diff(hWnd, &path1, &path2, r1, r2, false, false, 0, bAlternative);
	return 0;
}
