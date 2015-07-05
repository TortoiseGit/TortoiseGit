// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2003-2008 - TortoiseSVN
// Copyright (C) 2008-2015 - TortoiseGit

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
#include "AppUtils.h"
#include "gittype.h"
#include "resource.h"
#include "MessageBox.h"
#include "FileDiffDlg.h"
#include "SubmoduleDiffDlg.h"

CGitDiff::CGitDiff(void)
{
}

CGitDiff::~CGitDiff(void)
{
}
int CGitDiff::SubmoduleDiffNull(const CTGitPath * pPath, const git_revnum_t &rev1)
{
	CString oldhash = GIT_REV_ZERO;
	CString oldsub ;
	CString newsub;
	CString newhash;

	CString cmd;
	if (rev1 != GIT_REV_ZERO)
		cmd.Format(_T("git.exe ls-tree \"%s\" -- \"%s\""), (LPCTSTR)rev1, (LPCTSTR)pPath->GetGitPathString());
	else
		cmd.Format(_T("git.exe ls-files -s -- \"%s\""), (LPCTSTR)pPath->GetGitPathString());

	CString output, err;
	if (g_Git.Run(cmd, &output, &err, CP_UTF8))
	{
		CMessageBox::Show(NULL, output + L"\n" + err, _T("TortoiseGit"), MB_OK|MB_ICONERROR);
		return -1;
	}

	int start=0;
	start=output.Find(_T(' '),start);
	if(start>0)
	{
		if (rev1 != GIT_REV_ZERO) // in ls-files the hash is in the second column; in ls-tree it's in the third one
			start = output.Find(_T(' '), start + 1);
		if(start>0)
			newhash=output.Mid(start+1, 40);

		CGit subgit;
		subgit.m_CurrentDir = g_Git.CombinePath(pPath);
		int encode=CAppUtils::GetLogOutputEncode(&subgit);

		cmd.Format(_T("git.exe log -n1 --pretty=format:\"%%s\" %s --"), (LPCTSTR)newhash);
		bool toOK = !subgit.Run(cmd,&newsub,encode);

		bool dirty = false;
		if (rev1 == GIT_REV_ZERO)
		{
			CString dirtyList;
			subgit.Run(_T("git.exe status --porcelain"), &dirtyList, encode);
			dirty = !dirtyList.IsEmpty();
		}

		CSubmoduleDiffDlg submoduleDiffDlg;
		submoduleDiffDlg.SetDiff(pPath->GetWinPath(), false, oldhash, oldsub, true, newhash, newsub, toOK, dirty, NewSubmodule);
		submoduleDiffDlg.DoModal();
		if (submoduleDiffDlg.IsRefresh())
			return 1;

		return 0;
	}

	if (rev1 != GIT_REV_ZERO)
		CMessageBox::Show(NULL, _T("ls-tree output format error"), _T("TortoiseGit"), MB_OK | MB_ICONERROR);
	else
		CMessageBox::Show(NULL, _T("ls-files output format error"), _T("TortoiseGit"), MB_OK | MB_ICONERROR);
	return -1;
}

int CGitDiff::DiffNull(const CTGitPath *pPath, git_revnum_t rev1, bool bIsAdd, int jumpToLine)
{
	CString temppath;
	GetTempPath(temppath);
	if (rev1 != GIT_REV_ZERO)
	{
		CGitHash rev1Hash;
		if (g_Git.GetHash(rev1Hash, rev1)) // make sure we have a HASH here, otherwise filenames might be invalid
		{
			MessageBox(NULL, g_Git.GetGitLastErr(_T("Could not get hash of \"") + rev1 + _T("\".")), _T("TortoiseGit"), MB_ICONERROR);
			return -1;
		}
		rev1 = rev1Hash.ToString();
	}
	CString file1;
	CString nullfile;
	CString cmd;

	if(pPath->IsDirectory())
	{
		int result;
		// refresh if result = 1
		CTGitPath path = *pPath;
		while ((result = SubmoduleDiffNull(&path, rev1)) == 1)
		{
			path.SetFromGit(pPath->GetGitPathString());
		}
		return result;
	}

	if(rev1 != GIT_REV_ZERO )
	{
		TCHAR szTempName[MAX_PATH] = {0};
		GetTempFileName(temppath, pPath->GetBaseFilename(), 0, szTempName);
		CString temp(szTempName);
		DeleteFile(szTempName);
		CreateDirectory(szTempName, NULL);
		file1.Format(_T("%s\\%s-%s%s"),
				(LPCTSTR)temp,
				(LPCTSTR)pPath->GetBaseFilename(),
				(LPCTSTR)rev1.Left(g_Git.GetShortHASHLength()),
				(LPCTSTR)pPath->GetFileExtension());

		if (g_Git.GetOneFile(rev1, *pPath, file1))
		{
			CString out;
			out.Format(IDS_STATUSLIST_CHECKOUTFILEFAILED, (LPCTSTR)pPath->GetGitPathString(), (LPCTSTR)rev1, (LPCTSTR)file1);
			CMessageBox::Show(nullptr, g_Git.GetGitLastErr(out, CGit::GIT_CMD_GETONEFILE), _T("TortoiseGit"), MB_OK);
			return -1;
		}
	}
	else
	{
		file1 = g_Git.CombinePath(pPath);
	}

	// preserve FileExtension, needed especially for diffing deleted images (detection on new filename extension)
	CString tempfile=::GetTempFile() + pPath->GetFileExtension();
	CStdioFile file(tempfile,CFile::modeReadWrite|CFile::modeCreate );
	//file.WriteString();
	file.Close();
	::SetFileAttributes(tempfile, FILE_ATTRIBUTE_READONLY);

	CAppUtils::DiffFlags flags;

	if(bIsAdd)
		CAppUtils::StartExtDiff(tempfile,file1,
							pPath->GetGitPathString(),
							pPath->GetGitPathString() + _T(":") + rev1.Left(g_Git.GetShortHASHLength()),
							g_Git.CombinePath(pPath), g_Git.CombinePath(pPath),
							git_revnum_t(GIT_REV_ZERO), rev1
							, flags, jumpToLine);
	else
		CAppUtils::StartExtDiff(file1,tempfile,
							pPath->GetGitPathString() + _T(":") + rev1.Left(g_Git.GetShortHASHLength()),
							pPath->GetGitPathString(),
							g_Git.CombinePath(pPath), g_Git.CombinePath(pPath),
							rev1, git_revnum_t(GIT_REV_ZERO)
							, flags, jumpToLine);

	return 0;
}

int CGitDiff::SubmoduleDiff(const CTGitPath * pPath, const CTGitPath * /*pPath2*/, const git_revnum_t &rev1, const git_revnum_t &rev2, bool /*blame*/, bool /*unified*/)
{
	CString oldhash;
	CString newhash;
	bool dirty = false;
	CString cmd;
	bool isWorkingCopy = false;
	if( rev2 == GIT_REV_ZERO || rev1 == GIT_REV_ZERO )
	{
		oldhash = GIT_REV_ZERO;
		newhash = GIT_REV_ZERO;

		CString rev;
		if( rev2 != GIT_REV_ZERO )
			rev = rev2;
		if( rev1 != GIT_REV_ZERO )
			rev = rev1;

		isWorkingCopy = true;

		cmd.Format(_T("git.exe diff %s -- \"%s\""),
		(LPCTSTR)rev, (LPCTSTR)pPath->GetGitPathString());

		CString output, err;
		if (g_Git.Run(cmd, &output, &err, CP_UTF8))
		{
			CMessageBox::Show(NULL, output + L"\n" + err, _T("TortoiseGit"), MB_OK|MB_ICONERROR);
			return -1;
		}

		if (output.IsEmpty())
		{
			output.Empty();
			err.Empty();
			// also compare against index
			cmd.Format(_T("git.exe diff -- \"%s\""), (LPCTSTR)pPath->GetGitPathString());
			if (g_Git.Run(cmd, &output, &err, CP_UTF8))
			{
				CMessageBox::Show(NULL, output + _T("\n") + err, _T("TortoiseGit"), MB_OK | MB_ICONERROR);
				return -1;
			}

			if (output.IsEmpty())
			{
				CMessageBox::Show(NULL, CString(MAKEINTRESOURCE(IDS_ERR_EMPTYDIFF)), _T("TortoiseGit"), MB_OK | MB_ICONERROR);
				return -1;
			}
			else if (CMessageBox::Show(NULL, CString(MAKEINTRESOURCE(IDS_SUBMODULE_EMPTYDIFF)), _T("TortoiseGit"), 1, IDI_QUESTION, CString(MAKEINTRESOURCE(IDS_MSGBOX_YES)), CString(MAKEINTRESOURCE(IDS_MSGBOX_NO))) == 1)
			{
				CString sCmd;
				sCmd.Format(_T("/command:subupdate /bkpath:\"%s\""), (LPCTSTR)g_Git.m_CurrentDir);
				CAppUtils::RunTortoiseGitProc(sCmd);
			}
			return -1;
		}

		int start =0;
		int oldstart = output.Find(_T("-Subproject commit"),start);
		if(oldstart<0)
		{
			CMessageBox::Show(NULL,_T("Subproject Diff Format error") ,_T("TortoiseGit"),MB_OK|MB_ICONERROR);
			return -1;
		}
		oldhash = output.Mid(oldstart+ CString(_T("-Subproject commit")).GetLength()+1,40);
		start = 0;
		int newstart = output.Find(_T("+Subproject commit"),start);
		if (newstart < 0)
		{
			CMessageBox::Show(NULL,_T("Subproject Diff Format error") ,_T("TortoiseGit"),MB_OK|MB_ICONERROR);
			return -1;
		}
		newhash = output.Mid(newstart+ CString(_T("+Subproject commit")).GetLength()+1,40);
		dirty = output.Mid(newstart + CString(_T("+Subproject commit")).GetLength() + 41) == _T("-dirty\n");
	}
	else
	{
		cmd.Format(_T("git.exe diff-tree -r -z %s %s -- \"%s\""),
		(LPCTSTR)rev2, (LPCTSTR)rev1, (LPCTSTR)pPath->GetGitPathString());

		BYTE_VECTOR bytes, errBytes;
		if(g_Git.Run(cmd, &bytes, &errBytes))
		{
			CString err;
			CGit::StringAppend(&err, &errBytes[0], CP_UTF8);
			CMessageBox::Show(NULL,err,_T("TortoiseGit"),MB_OK|MB_ICONERROR);
			return -1;
		}

		if (bytes.size() < 15 + 2 * GIT_HASH_SIZE + 1 + 2 * GIT_HASH_SIZE)
		{
			CMessageBox::Show(NULL, _T("git diff-tree gives invalid output"), _T("TortoiseGit"), MB_OK | MB_ICONERROR);
			return -1;
		}
		CGit::StringAppend(&oldhash, &bytes[15], CP_UTF8, 2 * GIT_HASH_SIZE);
		CGit::StringAppend(&newhash, &bytes[15 + 2 * GIT_HASH_SIZE + 1], CP_UTF8, 2 * GIT_HASH_SIZE);

	}

	CString oldsub;
	CString newsub;
	bool oldOK = false, newOK = false;

	CGit subgit;
	subgit.m_CurrentDir = g_Git.CombinePath(pPath);
	ChangeType changeType = Unknown;

	if (pPath->HasAdminDir())
		GetSubmoduleChangeType(subgit, oldhash, newhash, oldOK, newOK, changeType, oldsub, newsub);

	CSubmoduleDiffDlg submoduleDiffDlg;
	submoduleDiffDlg.SetDiff(pPath->GetWinPath(), isWorkingCopy, oldhash, oldsub, oldOK, newhash, newsub, newOK, dirty, changeType);
	submoduleDiffDlg.DoModal();
	if (submoduleDiffDlg.IsRefresh())
		return 1;

	return 0;
}

void CGitDiff::GetSubmoduleChangeType(CGit& subgit, const CString& oldhash, const CString& newhash, bool& oldOK, bool& newOK, ChangeType& changeType, CString& oldsub, CString& newsub)
{
	CString cmd;
	int encode = CAppUtils::GetLogOutputEncode(&subgit);
	int oldTime = 0, newTime = 0;

	if (oldhash != GIT_REV_ZERO)
	{
		CString cmdout, cmderr;
		cmd.Format(_T("git.exe log -n1 --pretty=format:\"%%ct %%s\" %s --"), (LPCTSTR)oldhash);
		oldOK = !subgit.Run(cmd, &cmdout, &cmderr, encode);
		if (oldOK)
		{
			int pos = cmdout.Find(_T(" "));
			oldTime = _ttoi(cmdout.Left(pos));
			oldsub = cmdout.Mid(pos + 1);
		}
		else
			oldsub = cmderr;
	}
	if (newhash != GIT_REV_ZERO)
	{
		CString cmdout, cmderr;
		cmd.Format(_T("git.exe log -n1 --pretty=format:\"%%ct %%s\" %s --"), (LPCTSTR)newhash);
		newOK = !subgit.Run(cmd, &cmdout, &cmderr, encode);
		if (newOK)
		{
			int pos = cmdout.Find(_T(" "));
			newTime = _ttoi(cmdout.Left(pos));
			newsub = cmdout.Mid(pos + 1);
		}
		else
			newsub = cmderr;
	}

	if (oldhash == GIT_REV_ZERO)
	{
		oldOK = true;
		changeType = NewSubmodule;
	}
	else if (newhash == GIT_REV_ZERO)
	{
		newOK = true;
		changeType = DeleteSubmodule;
	}
	else if (oldhash != newhash)
	{
		bool ffNewer = false, ffOlder = false;
		ffNewer = subgit.IsFastForward(oldhash, newhash);
		if (!ffNewer)
		{
			ffOlder = subgit.IsFastForward(newhash, oldhash);
			if (!ffOlder)
			{
				if (newTime > oldTime)
					changeType = NewerTime;
				else if (newTime < oldTime)
					changeType = OlderTime;
				else
					changeType = SameTime;
			}
			else
				changeType = Rewind;
		}
		else
			changeType = FastForward;
	}
	else if (oldhash == newhash)
		changeType = Identical;

	if (!oldOK || !newOK)
		changeType = Unknown;
}

int CGitDiff::Diff(const CTGitPath * pPath, const CTGitPath * pPath2, git_revnum_t rev1, git_revnum_t rev2, bool /*blame*/, bool /*unified*/, int jumpToLine)
{
	CString temppath;
	GetTempPath(temppath);

	// make sure we have HASHes here, otherwise filenames might be invalid
	if (rev1 != GIT_REV_ZERO)
	{
		CGitHash rev1Hash;
		if (g_Git.GetHash(rev1Hash, rev1))
		{
			MessageBox(NULL, g_Git.GetGitLastErr(_T("Could not get hash of \"") + rev1 + _T("\".")), _T("TortoiseGit"), MB_ICONERROR);
			return -1;
		}
		rev1 = rev1Hash.ToString();
	}
	if (rev2 != GIT_REV_ZERO)
	{
		CGitHash rev2Hash;
		if (g_Git.GetHash(rev2Hash, rev2))
		{
			MessageBox(NULL, g_Git.GetGitLastErr(_T("Could not get hash of \"") + rev2 + _T("\".")), _T("TortoiseGit"), MB_ICONERROR);
			return -1;
		}
		rev2 = rev2Hash.ToString();
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
		while ((result = SubmoduleDiff(&path, &path2, rev1, rev2)) == 1)
		{
			path.SetFromGit(pPath->GetGitPathString());
			path2.SetFromGit(pPath2->GetGitPathString());
		}
		return result;
	}

	if(rev1 != GIT_REV_ZERO )
	{
		TCHAR szTempName[MAX_PATH] = {0};
		GetTempFileName(temppath, pPath->GetBaseFilename(), 0, szTempName);
		CString temp(szTempName);
		DeleteFile(szTempName);
		CreateDirectory(szTempName, NULL);
		// use original file extension, an external diff tool might need it
		file1.Format(_T("%s\\%s-%s-right%s"),
				(LPCTSTR)temp,
				(LPCTSTR)pPath->GetBaseFilename(),
				(LPCTSTR)rev1.Left(g_Git.GetShortHASHLength()),
				(LPCTSTR)pPath->GetFileExtension());
		title1 = pPath->GetFileOrDirectoryName() + _T(":") + rev1.Left(g_Git.GetShortHASHLength());
		if (g_Git.GetOneFile(rev1, *pPath, file1))
		{
			CString out;
			out.Format(IDS_STATUSLIST_CHECKOUTFILEFAILED, (LPCTSTR)pPath->GetGitPathString(), (LPCTSTR)rev1, (LPCTSTR)file1);
			CMessageBox::Show(nullptr, g_Git.GetGitLastErr(out, CGit::GIT_CMD_GETONEFILE), _T("TortoiseGit"), MB_OK);
			return -1;
		}
		::SetFileAttributes(file1, FILE_ATTRIBUTE_READONLY);
	}
	else
	{
		file1 = g_Git.CombinePath(pPath);
		title1.Format(IDS_DIFF_WCNAME, (LPCTSTR)pPath->GetFileOrDirectoryName());
		if (!PathFileExists(file1))
		{
			CString sMsg;
			sMsg.Format(IDS_PROC_DIFFERROR_FILENOTINWORKINGTREE, (LPCTSTR)file1);
			if (MessageBox(NULL, sMsg, _T("TortoiseGit"), MB_ICONEXCLAMATION | MB_YESNO) != IDYES)
				return 1;
			if (!CCommonAppUtils::FileOpenSave(file1, NULL, IDS_SELECTFILE, IDS_COMMONFILEFILTER, true))
				return 1;
			title1.Format(IDS_DIFF_WCNAME, (LPCTSTR)CTGitPath(file1).GetUIFileOrDirectoryName());
		}
	}

	CString file2;
	CString title2;
	if(rev2 != GIT_REV_ZERO)
	{
		TCHAR szTempName[MAX_PATH] = {0};
		GetTempFileName(temppath, pPath2->GetBaseFilename(), 0, szTempName);
		CString temp(szTempName);
		DeleteFile(szTempName);
		CreateDirectory(szTempName, NULL);
		CTGitPath fileName = *pPath2;
		if (pPath2->m_Action & CTGitPath::LOGACTIONS_REPLACED)
			fileName = CTGitPath(pPath2->GetGitOldPathString());

		// use original file extension, an external diff tool might need it
		file2.Format(_T("%s\\%s-%s-left%s"),
				(LPCTSTR)temp,
				(LPCTSTR)fileName.GetBaseFilename(),
				(LPCTSTR)rev2.Left(g_Git.GetShortHASHLength()),
				(LPCTSTR)fileName.GetFileExtension());
		title2 = fileName.GetFileOrDirectoryName() + _T(":") + rev2.Left(g_Git.GetShortHASHLength());
		if (g_Git.GetOneFile(rev2, fileName, file2))
		{
			CString out;
			out.Format(IDS_STATUSLIST_CHECKOUTFILEFAILED, (LPCTSTR)pPath2->GetGitPathString(), (LPCTSTR)rev2, (LPCTSTR)file2);
			CMessageBox::Show(nullptr, g_Git.GetGitLastErr(out, CGit::GIT_CMD_GETONEFILE), _T("TortoiseGit"), MB_OK);
			return -1;
		}
		::SetFileAttributes(file2, FILE_ATTRIBUTE_READONLY);
	}
	else
	{
		file2 = g_Git.CombinePath(pPath2);
		title2.Format( IDS_DIFF_WCNAME, pPath2->GetFileOrDirectoryName() );
	}

	if (pPath->m_Action == pPath->LOGACTIONS_ADDED)
	{
		CGitDiff::DiffNull(pPath, rev1, true, jumpToLine);
	}
	else if (pPath->m_Action == pPath->LOGACTIONS_DELETED)
	{
		CGitDiff::DiffNull(pPath, rev2, false, jumpToLine);
	}
	else
	{
		CAppUtils::DiffFlags flags;
		CAppUtils::StartExtDiff(file2,file1,
								title2,
								title1,
								g_Git.CombinePath(pPath2),
								g_Git.CombinePath(pPath),
								rev2,
								rev1,
								flags, jumpToLine);
	}
	return 0;
}

int CGitDiff::DiffCommit(const CTGitPath &path, const GitRev *r1, const GitRev *r2)
{
	return DiffCommit(path, path, r1, r2);
}

int CGitDiff::DiffCommit(const CTGitPath &path1, const CTGitPath &path2, const GitRev *r1, const GitRev *r2)
{
	if (path1.GetWinPathString().IsEmpty())
	{
		CFileDiffDlg dlg;
		dlg.SetDiff(NULL, *r1, *r2);
		dlg.DoModal();
	}
	else if (path1.IsDirectory())
	{
		CFileDiffDlg dlg;
		dlg.SetDiff(&path1, *r1, *r2);
		dlg.DoModal();
	}
	else
	{
		Diff(&path1, &path2, r1->m_CommitHash.ToString(), r2->m_CommitHash.ToString());
	}
	return 0;
}

int CGitDiff::DiffCommit(const CTGitPath &path, const CString &r1, const CString &r2)
{
	return DiffCommit(path, path, r1, r2);
}


int CGitDiff::DiffCommit(const CTGitPath &path1, const CTGitPath &path2, const CString &r1, const CString &r2)
{
	if (path1.GetWinPathString().IsEmpty())
	{
		CFileDiffDlg dlg;
		dlg.SetDiff(NULL, r1, r2);
		dlg.DoModal();
	}
	else if (path1.IsDirectory())
	{
		CFileDiffDlg dlg;
		dlg.SetDiff(&path1, r1, r2);
		dlg.DoModal();
	}
	else
	{
		Diff(&path1, &path2, r1, r2);
	}
	return 0;
}
