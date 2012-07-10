// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2012 - TortoiseGit
// Copyright (C) 2003-2011 - TortoiseSVN

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
#include "StdAfx.h"
#include "resource.h"
#include "TortoiseProc.h"
#include "PathUtils.h"
#include "AppUtils.h"
//#include "GitProperties.h"
#include "StringUtils.h"
#include "MessageBox.h"
#include "Registry.h"
#include "TGitPath.h"
#include "Git.h"
//#include "RepositoryBrowser.h"
//#include "BrowseFolder.h"
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
#include "GITProgressDlg.h"
#include "PushDlg.h"
#include "CommitDlg.h"
#include "MergeDlg.h"
#include "hooks.h"
#include "..\Settings\Settings.h"
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

CAppUtils::CAppUtils(void)
{
}

CAppUtils::~CAppUtils(void)
{
}

bool CAppUtils::StashSave()
{
	CStashSaveDlg dlg;

	if (dlg.DoModal() == IDOK)
	{
		CString cmd, out;
		cmd = _T("git.exe stash save");

		if (dlg.m_bIncludeUntracked && CAppUtils::GetMsysgitVersion() >= 0x01070700)
			cmd += _T(" --include-untracked");

		if (!dlg.m_sMessage.IsEmpty())
		{
			CString message = dlg.m_sMessage;
			message.Replace(_T("\""), _T("\"\""));
			cmd += _T(" \"") + message + _T("\"");
		}

		if (g_Git.Run(cmd, &out, CP_UTF8))
		{
			CMessageBox::Show(NULL, CString(MAKEINTRESOURCE(IDS_PROC_STASHFAILED)) + _T("\n") + out, _T("TortoiseGit"), MB_OK | MB_ICONERROR);
		}
		else
		{
 			CMessageBox::Show(NULL, CString(MAKEINTRESOURCE(IDS_PROC_STASHSUCCESS)) + _T("\n") + out, _T("TortoiseGit"), MB_OK | MB_ICONINFORMATION);
			return true;
		}
	}
	return false;
}

int	 CAppUtils::StashApply(CString ref, bool showChanges /* true */)
{
	CString cmd,out;
	cmd = _T("git.exe stash apply ");
	if (ref.Find(_T("refs/")) == 0)
		ref = ref.Mid(5);
	if (ref.Find(_T("stash{")) == 0)
		ref = _T("stash@") + ref.Mid(5);
	cmd += ref;

	int ret = g_Git.Run(cmd, &out, CP_UTF8);
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
			return 0;
		}
		else
		{
			CMessageBox::Show(NULL, message ,_T("TortoiseGit"), MB_OK | MB_ICONINFORMATION);
			return 0;
		}
	}
	return -1;
}

int	 CAppUtils::StashPop(bool showChanges /* true */)
{
	CString cmd,out;
	cmd=_T("git.exe stash pop ");

	int ret = g_Git.Run(cmd, &out, CP_UTF8);
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
			return 0;
		}
		else
		{
			CMessageBox::Show(NULL, message ,_T("TortoiseGit"), MB_OK | MB_ICONINFORMATION);
			return 0;
		}
	}
	return -1;
}

bool CAppUtils::GetMimeType(const CTGitPath& /*file*/, CString& /*mimetype*/)
{
#if 0
	GitProperties props(file, GitRev::REV_WC, false);
	for (int i = 0; i < props.GetCount(); ++i)
	{
		if (props.GetItemName(i).compare(_T("svn:mime-type"))==0)
		{
			mimetype = props.GetItemValue(i).c_str();
			return true;
		}
	}
#endif
	return false;
}

BOOL CAppUtils::StartExtMerge(
	const CTGitPath& basefile, const CTGitPath& theirfile, const CTGitPath& yourfile, const CTGitPath& mergedfile,
	const CString& basename, const CString& theirname, const CString& yourname, const CString& mergedname, bool bReadOnly)
{

	CRegString regCom = CRegString(_T("Software\\TortoiseGit\\Merge"));
	CString ext = mergedfile.GetFileExtension();
	CString com = regCom;
	bool bInternal = false;

	CString mimetype;
	if (ext != "")
	{
		// is there an extension specific merge tool?
		CRegString mergetool(_T("Software\\TortoiseGit\\MergeTools\\") + ext.MakeLower());
		if (CString(mergetool) != "")
		{
			com = mergetool;
		}
	}
	if (GetMimeType(yourfile, mimetype) || GetMimeType(theirfile, mimetype) || GetMimeType(basefile, mimetype))
	{
		// is there a mime type specific merge tool?
		CRegString mergetool(_T("Software\\TortoiseGit\\MergeTools\\") + mimetype);
		if (CString(mergetool) != "")
		{
			com = mergetool;
		}
	}

	if (com.IsEmpty()||(com.Left(1).Compare(_T("#"))==0))
	{
		// use TortoiseMerge
		bInternal = true;
		CRegString tortoiseMergePath(_T("Software\\TortoiseGit\\TMergePath"), _T(""), false, HKEY_LOCAL_MACHINE);
		com = tortoiseMergePath;
		if (com.IsEmpty())
		{
			com = CPathUtils::GetAppDirectory();
			com += _T("TortoiseMerge.exe");
		}
		com = _T("\"") + com + _T("\"");
		com = com + _T(" /base:%base /theirs:%theirs /mine:%mine /merged:%merged");
		com = com + _T(" /basename:%bname /theirsname:%tname /minename:%yname /mergedname:%mname");
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
	// use TortoiseMerge
	viewer = CPathUtils::GetAppDirectory();
	viewer += _T("TortoiseMerge.exe");

	viewer = _T("\"") + viewer + _T("\"");
	viewer = viewer + _T(" /diff:\"") + patchfile.GetWinPathString() + _T("\"");
	viewer = viewer + _T(" /patchpath:\"") + dir.GetWinPathString() + _T("\"");
	if (bReversed)
		viewer += _T(" /reversedpatch");
	if (!sOriginalDescription.IsEmpty())
		viewer = viewer + _T(" /patchoriginal:\"") + sOriginalDescription + _T("\"");
	if (!sPatchedDescription.IsEmpty())
		viewer = viewer + _T(" /patchpatched:\"") + sPatchedDescription + _T("\"");
	if(!LaunchApplication(viewer, IDS_ERR_DIFFVIEWSTART, !!bWait))
	{
		return FALSE;
	}
	return TRUE;
}

CString CAppUtils::PickDiffTool(const CTGitPath& file1, const CTGitPath& file2)
{
	// Is there a mime type specific diff tool?
	CString mimetype;
	if (GetMimeType(file1, mimetype) ||  GetMimeType(file2, mimetype))
	{
		CString difftool = CRegString(_T("Software\\TortoiseGit\\DiffTools\\") + mimetype);
		if (!difftool.IsEmpty())
			return difftool;
	}

	// Is there an extension specific diff tool?
	CString ext = file2.GetFileExtension().MakeLower();
	if (!ext.IsEmpty())
	{
		CString difftool = CRegString(_T("Software\\TortoiseGit\\DiffTools\\") + ext);
		if (!difftool.IsEmpty())
			return difftool;
		// Maybe we should use TortoiseIDiff?
		if ((ext == _T(".jpg")) || (ext == _T(".jpeg")) ||
			(ext == _T(".bmp")) || (ext == _T(".gif"))  ||
			(ext == _T(".png")) || (ext == _T(".ico"))  ||
			(ext == _T(".dib")) || (ext == _T(".emf")))
		{
			return
				_T("\"") + CPathUtils::GetAppDirectory() + _T("TortoiseIDiff.exe") + _T("\"") +
				_T(" /left:%base /right:%mine /lefttitle:%bname /righttitle:%yname");
		}
	}

	// Finally, pick a generic external diff tool
	CString difftool = CRegString(_T("Software\\TortoiseGit\\Diff"));
	return difftool;
}

bool CAppUtils::StartExtDiff(
	const CString& file1,  const CString& file2,
	const CString& sName1, const CString& sName2,
	const DiffFlags& flags)
{
	CString viewer;

	CRegDWORD blamediff(_T("Software\\TortoiseGit\\DiffBlamesWithTortoiseMerge"), FALSE);
	if (!flags.bBlame || !(DWORD)blamediff)
	{
		viewer = PickDiffTool(file1, file2);
		// If registry entry for a diff program is commented out, use TortoiseMerge.
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
			_T("\"") + CPathUtils::GetAppDirectory() + _T("TortoiseMerge.exe") + _T("\"") +
			_T(" /base:%base /mine:%mine /basename:%bname /minename:%yname");
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

	if (flags.bReadOnly && bInternal)
		viewer += _T(" /readonly");

	return LaunchApplication(viewer, IDS_ERR_EXTDIFFSTART, flags.bWait);
}

BOOL CAppUtils::StartUnifiedDiffViewer(const CString& patchfile, const CString& title, BOOL bWait)
{
	CString viewer;
	CRegString v = CRegString(_T("Software\\TortoiseGit\\DiffViewer"));
	viewer = v;
	if (viewer.IsEmpty() || (viewer.Left(1).Compare(_T("#"))==0))
	{
		// use TortoiseUDiff
		viewer = CPathUtils::GetAppDirectory();
		viewer += _T("TortoiseUDiff.exe");
		// enquote the path to TortoiseUDiff
		viewer = _T("\"") + viewer + _T("\"");
		// add the params
		viewer = viewer + _T(" /patchfile:%1 /title:\"%title\"");

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
	TCHAR * buf = new TCHAR[len+1];
	ExpandEnvironmentStrings(viewer, buf, len);
	viewer = buf;
	delete [] buf;
	len = ExpandEnvironmentStrings(file, NULL, 0);
	buf = new TCHAR[len+1];
	ExpandEnvironmentStrings(file, buf, len);
	file = buf;
	delete [] buf;
	file = _T("\"")+file+_T("\"");
	if (viewer.IsEmpty())
	{
		viewer = _T("RUNDLL32 Shell32,OpenAs_RunDLL");
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

bool CAppUtils::LaunchPAgent(CString *keyfile,CString * pRemote)
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
		int start=0;
		key = key.Tokenize(_T("\n"),start);
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
	proc += _T("touch.exe\"");
	proc += _T(" \"");
	proc += tempfile;
	proc += _T("\"");

	bool b = LaunchApplication(proc, IDS_ERR_PAGEANT, true, &CPathUtils::GetAppDirectory());
	if(!b)
		return b;

	int i=0;
	while(!::PathFileExists(tempfile))
	{
		Sleep(100);
		i++;
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
bool CAppUtils::LaunchAlternativeEditor(const CString& filename)
{
	CString editTool = CRegString(_T("Software\\TortoiseGit\\AlternativeEditor"));
	if (editTool.IsEmpty() || (editTool.Left(1).Compare(_T("#"))==0)) {
		editTool = CPathUtils::GetAppDirectory() + _T("notepad2.exe");
	}

	CString sCmd;
	sCmd.Format(_T("\"%s\" \"%s\""), editTool, filename);

	LaunchApplication(sCmd, NULL, false);
	return true;
}
bool CAppUtils::LaunchRemoteSetting()
{
	CTGitPath path(g_Git.m_CurrentDir);
	CSettings dlg(IDS_PROC_SETTINGS_TITLE, &path);
	dlg.SetTreeViewMode(TRUE, TRUE, TRUE);
	//dlg.SetTreeWidth(220);
	dlg.m_DefaultPage = _T("gitremote");

	dlg.DoModal();
	dlg.HandleRestart();
	return true;
}
/**
* Launch the external blame viewer
*/
bool CAppUtils::LaunchTortoiseBlame(const CString& sBlameFile,CString Rev,const CString& sParams)
{
	CString viewer = _T("\"") + CPathUtils::GetAppDirectory();
	viewer += _T("TortoiseGitBlame.exe");
	viewer += _T("\" \"") + sBlameFile + _T("\"");
	//viewer += _T(" \"") + sLogFile + _T("\"");
	//viewer += _T(" \"") + sOriginalFile + _T("\"");
	if(!Rev.IsEmpty() && Rev != GIT_REV_ZERO)
		viewer += CString(_T(" /rev:"))+Rev;
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
	sText.Remove('\r');

	// style each line separately
	int offset = 0;
	int nNewlinePos;
	do
	{
		nNewlinePos = sText.Find('\n', offset);
		CString sLine = sText.Mid(offset);
		if (nNewlinePos>=0)
			sLine = sLine.Left(nNewlinePos-offset);
		int start = 0;
		int end = 0;
		while (FindStyleChars(sLine, '*', start, end))
		{
			CHARRANGE range = {(LONG)start+offset, (LONG)end+offset};
			pWnd->SendMessage(EM_EXSETSEL, NULL, (LPARAM)&range);
			CHARFORMAT2 format;
			SecureZeroMemory(&format, sizeof(CHARFORMAT2));
			format.cbSize = sizeof(CHARFORMAT2);
			format.dwMask = CFM_BOLD;
			format.dwEffects = CFE_BOLD;
			pWnd->SendMessage(EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&format);
			bStyled = true;
			start = end;
		}
		start = 0;
		end = 0;
		while (FindStyleChars(sLine, '^', start, end))
		{
			CHARRANGE range = {(LONG)start+offset, (LONG)end+offset};
			pWnd->SendMessage(EM_EXSETSEL, NULL, (LPARAM)&range);
			CHARFORMAT2 format;
			SecureZeroMemory(&format, sizeof(CHARFORMAT2));
			format.cbSize = sizeof(CHARFORMAT2);
			format.dwMask = CFM_ITALIC;
			format.dwEffects = CFE_ITALIC;
			pWnd->SendMessage(EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&format);
			bStyled = true;
			start = end;
		}
		start = 0;
		end = 0;
		while (FindStyleChars(sLine, '_', start, end))
		{
			CHARRANGE range = {(LONG)start+offset, (LONG)end+offset};
			pWnd->SendMessage(EM_EXSETSEL, NULL, (LPARAM)&range);
			CHARFORMAT2 format;
			SecureZeroMemory(&format, sizeof(CHARFORMAT2));
			format.cbSize = sizeof(CHARFORMAT2);
			format.dwMask = CFM_UNDERLINE;
			format.dwEffects = CFE_UNDERLINE;
			pWnd->SendMessage(EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&format);
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
	bool bFoundMarker = false;
	// find a starting marker
	while (sText[i] != 0)
	{
		if (sText[i] == stylechar)
		{
			if (((i+1)<sText.GetLength())&&(IsCharAlphaNumeric(sText[i+1])) &&
				(((i>0)&&(!IsCharAlphaNumeric(sText[i-1])))||(i==0)))
			{
				start = i+1;
				i++;
				bFoundMarker = true;
				break;
			}
		}
		i++;
	}
	if (!bFoundMarker)
		return false;
	// find ending marker
	bFoundMarker = false;
	while (sText[i] != 0)
	{
		if (sText[i] == stylechar)
		{
			if ((IsCharAlphaNumeric(sText[i-1])) &&
				((((i+1)<sText.GetLength())&&(!IsCharAlphaNumeric(sText[i+1])))||(i+1)==sText.GetLength()))
			{
				end = i;
				i++;
				bFoundMarker = true;
				break;
			}
		}
		i++;
	}
	return bFoundMarker;
}

CString CAppUtils::GetProjectNameFromURL(CString url)
{
	CString name;
	while (name.IsEmpty() || (name.CompareNoCase(_T("branches"))==0) ||
		(name.CompareNoCase(_T("tags"))==0) ||
		(name.CompareNoCase(_T("trunk"))==0))
	{
		name = url.Mid(url.ReverseFind('/')+1);
		url = url.Left(url.ReverseFind('/'));
	}
	if ((name.Compare(_T("svn")) == 0)||(name.Compare(_T("svnroot")) == 0))
	{
		// a name of svn or svnroot indicates that it's not really the project name. In that
		// case, we try the first part of the URL
		// of course, this won't work in all cases (but it works for Google project hosting)
		url.Replace(_T("http://"), _T(""));
		url.Replace(_T("https://"), _T(""));
		url.Replace(_T("svn://"), _T(""));
		url.Replace(_T("svn+ssh://"), _T(""));
		url.TrimLeft(_T("/"));
		name = url.Left(url.Find('.'));
	}
	return name;
}

bool CAppUtils::StartShowUnifiedDiff(HWND /*hWnd*/, const CTGitPath& url1, const git_revnum_t& rev1,
												const CTGitPath& /*url2*/, const git_revnum_t& rev2,
												//const GitRev& peg /* = GitRev */, const GitRev& headpeg /* = GitRev */,
												bool /*bAlternateDiff*/ /* = false */, bool /*bIgnoreAncestry*/ /* = false */, bool /* blame = false */, bool bMerge)
{

	CString tempfile=GetTempFile();
	CString cmd;
	if(rev2 == GitRev::GetWorkingCopy())
	{
		cmd.Format(_T("git.exe diff --stat -p %s "), rev1);
	}
	else
	{
		CString merge;
		if(bMerge)
				merge = _T("-c");

		cmd.Format(_T("git.exe diff-tree -r -p %s --stat %s %s"),merge, rev1,rev2);
	}

	if( !url1.IsEmpty() )
	{
		cmd += _T(" -- \"");
		cmd += url1.GetGitPathString();
		cmd += _T("\" ");
	}
	g_Git.RunLogFile(cmd,tempfile);
	CAppUtils::StartUnifiedDiffViewer(tempfile, rev1 + _T(":") + rev2);


#if 0
	CString sCmd;
	sCmd.Format(_T("%s /command:showcompare /unified"),
		(LPCTSTR)(CPathUtils::GetAppDirectory()+_T("TortoiseProc.exe")));
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
		_stprintf_s(buf, 30, _T("%d"), hWnd);
		sCmd += buf;
	}

	return CAppUtils::LaunchApplication(sCmd, NULL, false);
#endif
	return TRUE;
}


bool CAppUtils::Export(CString *BashHash)
{
	bool bRet = false;

		// ask from where the export has to be done
	CExportDlg dlg;
	if(BashHash)
		dlg.m_Revision=*BashHash;

	if (dlg.DoModal() == IDOK)
	{
		CString cmd;
		cmd.Format(_T("git.exe archive --output=\"%s\" --format=zip --verbose %s"),
					dlg.m_strExportDirectory, g_Git.FixBranchName(dlg.m_VersionName));

		CProgressDlg pro;
		pro.m_GitCmd=cmd;
		pro.DoModal();
		return TRUE;
	}
	return bRet;
}

bool CAppUtils::CreateBranchTag(bool IsTag,CString *CommitHash, bool switch_new_brach)
{
	CCreateBranchTagDlg dlg;
	dlg.m_bIsTag=IsTag;
	dlg.m_bSwitch=switch_new_brach;

	if(CommitHash)
		dlg.m_Base = *CommitHash;

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

			CString tempfile=::GetTempFile();
			if(!dlg.m_Message.Trim().IsEmpty())
			{
				CAppUtils::SaveCommitUnicodeFile(tempfile,dlg.m_Message);
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
		}
		if( !IsTag  &&  dlg.m_bSwitch )
		{
			// it is a new branch and the user has requested to switch to it
			cmd.Format(_T("git.exe checkout %s"), dlg.m_BranchTagName);
			g_Git.Run(cmd,&out,CP_UTF8);
			CMessageBox::Show(NULL,out,_T("TortoiseGit"),MB_OK);
		}

		return TRUE;
	}
	return FALSE;
}

bool CAppUtils::Switch(CString *CommitHash, CString initialRefName, bool autoclose)
{
	CGitSwitchDlg dlg;
	if(CommitHash)
		dlg.m_Base=*CommitHash;
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

		return PerformSwitch(dlg.m_VersionName, dlg.m_bForce == TRUE , branch, dlg.m_bBranchOverride == TRUE, dlg.m_bTrack == TRUE, autoclose);
	}
	return FALSE;
}

bool CAppUtils::PerformSwitch(CString ref, bool bForce /* false */, CString sNewBranch /* CString() */, bool bBranchOverride /* false */, bool bTrack /* false */, bool autoClose /* false */)
{
	CString cmd;
	CString track;
	CString force;
	CString branch;

	if(!sNewBranch.IsEmpty()){
		if (bBranchOverride)
		{
			branch.Format(_T("-B %s"), sNewBranch);
		}
		else
		{
			branch.Format(_T("-b %s"), sNewBranch);
		}
		if (bTrack)
			track = _T("--track");
	}
	if (bForce)
		force = _T("-f");

	cmd.Format(_T("git.exe checkout %s %s %s %s"),
		 force,
		 track,
		 branch,
		 g_Git.FixBranchName(ref));

	CProgressDlg progress;
	progress.m_bAutoCloseOnSuccess = autoClose;
	progress.m_GitCmd = cmd;

	CTGitPath gitPath = g_Git.m_CurrentDir;
	if (gitPath.HasSubmodules())
		progress.m_PostCmdList.Add(CString(MAKEINTRESOURCE(IDS_PROC_SUBMODULESUPDATE)));

	int ret = progress.DoModal();
	if (gitPath.HasSubmodules() && ret == IDC_PROGRESS_BUTTON1)
	{
		CString sCmd;
		sCmd.Format(_T("/command:subupdate /bkpath:\"%s\""), g_Git.m_CurrentDir);

		RunTortoiseProc(sCmd);
		return TRUE;
	}
	else if (ret == IDOK)
		return TRUE;

	return FALSE;
}

bool CAppUtils::OpenIgnoreFile(CStdioFile &file, const CString& filename)
{
	if (!file.Open(filename, CFile::modeCreate | CFile::modeReadWrite | CFile::modeNoTruncate))
	{
		CMessageBox::Show(NULL, filename + _T(" Open Failure"), _T("TortoiseGit"), MB_OK | MB_ICONERROR);
		return false;
	}

	if (file.GetLength() > 0)
	{
		file.Seek(file.GetLength() - 1, 0);
		auto_buffer<TCHAR> buf(1);
		file.Read(buf, 1);
		file.SeekToEnd();
		if (buf[0] != _T('\n'))
			file.WriteString(_T("\n"));
	}
	else
		file.SeekToEnd();

	return true;
}

bool CAppUtils::IgnoreFile(CTGitPathList &path,bool IsMask)
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
				ignorefile += _T(".git/info/exclude");
				break;
		}

		CStdioFile file;
		try
		{
			if (ignoreDlg.m_IgnoreFile != 1 && !OpenIgnoreFile(file, ignorefile))
				return false;

			for (int i = 0; i < path.GetCount(); i++)
			{
				if (ignoreDlg.m_IgnoreFile == 1)
				{
					ignorefile = g_Git.m_CurrentDir + _T("\\") + path[i].GetContainingDirectory().GetWinPathString() + _T("\\.gitignore");
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
				file.WriteString(ignorePattern + _T("\n"));

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


bool CAppUtils::GitReset(CString *CommitHash,int type)
{
	CResetDlg dlg;
	dlg.m_ResetType=type;
	dlg.m_ResetToVersion=*CommitHash;
	if (dlg.DoModal() == IDOK)
	{
		CString cmd;
		CString type;
		switch(dlg.m_ResetType)
		{
		case 0:
			type=_T("--soft");
			break;
		case 1:
			type=_T("--mixed");
			break;
		case 2:
			type=_T("--hard");
			break;
		default:
			type=_T("--mixed");
			break;
		}
		cmd.Format(_T("git.exe reset %s %s"),type, *CommitHash);

		CProgressDlg progress;
		progress.m_GitCmd=cmd;

		CTGitPath gitPath = g_Git.m_CurrentDir;
		if (gitPath.HasSubmodules() && dlg.m_ResetType == 2)
			progress.m_PostCmdList.Add(CString(MAKEINTRESOURCE(IDS_PROC_SUBMODULESUPDATE)));

		int ret = progress.DoModal();
		if (gitPath.HasSubmodules() && dlg.m_ResetType == 2 && ret == IDC_PROGRESS_BUTTON1)
		{
			CString sCmd;
			sCmd.Format(_T("/command:subupdate /bkpath:\"%s\""), g_Git.m_CurrentDir);

			RunTortoiseProc(sCmd);
			return TRUE;
		}
		else if (ret == IDOK)
			return TRUE;

	}
	return FALSE;
}

void CAppUtils::DescribeFile(bool mode, bool base,CString &descript)
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

void CAppUtils::RemoveTempMergeFile(CTGitPath &path)
{
		CString tempmergefile;
		try
		{
			tempmergefile = CAppUtils::GetMergeTempFile(_T("LOCAL"),path);
			CFile::Remove(tempmergefile);
		}catch(...)
		{
		}

		try
		{
			tempmergefile = CAppUtils::GetMergeTempFile(_T("REMOTE"),path);
			CFile::Remove(tempmergefile);
		}catch(...)
		{
		}

		try
		{
			tempmergefile = CAppUtils::GetMergeTempFile(_T("BASE"),path);
			CFile::Remove(tempmergefile);
		}catch(...)
		{
		}
}
CString CAppUtils::GetMergeTempFile(CString type,CTGitPath &merge)
{
	CString file;
	file=g_Git.m_CurrentDir+_T("\\") + merge.GetWinPathString()+_T(".")+type+merge.GetFileExtension();

	return file;
}

bool CAppUtils::ConflictEdit(CTGitPath &path,bool /*bAlternativeTool*/,bool revertTheirMy)
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

	CTGitPathList list;
	list.ParserFromLsFile(vector);

	if(list.GetCount() == 0)
		return FALSE;

	CTGitPath theirs;
	CTGitPath mine;
	CTGitPath base;

	mine.SetFromGit(GetMergeTempFile(_T("LOCAL"),merge));
	theirs.SetFromGit(GetMergeTempFile(_T("REMOTE"),merge));
	base.SetFromGit(GetMergeTempFile(_T("BASE"),merge));

	CString format;

	//format=_T("git.exe cat-file blob \":%d:%s\"");
	format = _T("git checkout-index --temp --stage=%d -- \"%s\"");
	CFile tempfile;
	//create a empty file, incase stage is not three
	tempfile.Open(mine.GetWinPathString(),CFile::modeCreate|CFile::modeReadWrite);
	tempfile.Close();
	tempfile.Open(theirs.GetWinPathString(),CFile::modeCreate|CFile::modeReadWrite);
	tempfile.Close();
	tempfile.Open(base.GetWinPathString(),CFile::modeCreate|CFile::modeReadWrite);
	tempfile.Close();

	bool b_base=false, b_local=false, b_remote=false;

	for(int i=0;i<list.GetCount();i++)
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
		merge.SetFromWin(g_Git.m_CurrentDir+_T("\\")+merge.GetWinPathString());
		if( revertTheirMy )
			bRet = !!CAppUtils::StartExtMerge(base,mine, theirs,  merge,_T("BASE"),_T("LOCAL"),_T("REMOTE"));
		else
			bRet = !!CAppUtils::StartExtMerge(base, theirs, mine, merge,_T("BASE"),_T("REMOTE"),_T("LOCAL"));

	}
	else
	{
		CFile::Remove(mine.GetWinPathString());
		CFile::Remove(theirs.GetWinPathString());
		CFile::Remove(base.GetWinPathString());

		CDeleteConflictDlg dlg;
		DescribeFile(b_local, b_base,dlg.m_LocalStatus);
		DescribeFile(b_remote,b_base,dlg.m_RemoteStatus);
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

CString CAppUtils::GetClipboardLink()
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

		if(sClipboardText.GetLength()>=2)
			if( sClipboardText[1] == _T(':') )
				if( (sClipboardText[0] >= 'A' &&  sClipboardText[0] <= 'Z')
					|| (sClipboardText[0] >= 'a' &&  sClipboardText[0] <= 'z') )
					return sClipboardText;
	}

	return CString(_T(""));
}

CString CAppUtils::ChooseRepository(CString *path)
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

bool CAppUtils::SendPatchMail(CTGitPathList &list,bool autoclose)
{
	CSendMailDlg dlg;

	dlg.m_PathList  = list;

	if(dlg.DoModal()==IDOK)
	{
		if(dlg.m_PathList.GetCount() == 0)
			return FALSE;

		CGitProgressDlg progDlg;

		theApp.m_pMainWnd = &progDlg;
		progDlg.SetCommand(CGitProgressDlg::GitProgress_SendMail);

		progDlg.SetAutoClose(autoclose);

		progDlg.SetPathList(dlg.m_PathList);
				//ProjectProperties props;
				//props.ReadPropsPathList(dlg.m_pathList);
				//progDlg.SetProjectProperties(props);
		progDlg.SetItemCount(dlg.m_PathList.GetCount());

		DWORD flags =0;
		if(dlg.m_bAttachment)
			flags |= SENDMAIL_ATTACHMENT;
		if(dlg.m_bCombine)
			flags |= SENDMAIL_COMBINED;
		if(dlg.m_bUseMAPI)
			flags |= SENDMAIL_MAPI;

		progDlg.SetSendMailOption(dlg.m_To,dlg.m_CC,dlg.m_Subject,flags);

		progDlg.DoModal();

		return true;
	}
	return false;
}

bool CAppUtils::SendPatchMail(CString &cmd,CString &formatpatchoutput,bool autoclose)
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
		if(one.IsEmpty() || one == _T("Success"))
			continue;
		one.Replace(_T('/'),_T('\\'));
		CTGitPath path;
		path.SetFromWin(one);
		list.AddPath(path);
	}
	if (list.GetCount() > 0)
	{
		return SendPatchMail(list, autoclose);
	}
	else
	{
		CMessageBox::Show(NULL, IDS_ERR_NOPATCHES, IDS_APPNAME, MB_ICONINFORMATION);
		return true;
	}
}


int CAppUtils::GetLogOutputEncode(CGit *pGit)
{
	CString cmd,output;
	int start=0;

	output = pGit->GetConfigValue(_T("i18n.logOutputEncoding"));
	if(output.IsEmpty())
	{
		output =  pGit->GetConfigValue(_T("i18n.commitencoding"));
		if(output.IsEmpty())
			return CP_UTF8;

		int start=0;
		output=output.Tokenize(_T("\n"),start);
		return CUnicodeUtils::GetCPCode(output);

	}
	else
	{
		output=output.Tokenize(_T("\n"),start);
		return CUnicodeUtils::GetCPCode(output);
	}
}
int CAppUtils::GetCommitTemplate(CString &temp)
{
	CString cmd,output;

	output = g_Git.GetConfigValue(_T("commit.template"), CP_UTF8);
	if( output.IsEmpty() )
		return -1;

	if( output.GetLength()<1)
		return -1;

	if( output[0] == _T('/'))
	{
		if(output.GetLength()>=3)
			if(output[2] == _T('/'))
			{
				output.GetBuffer()[0] = output[1];
				output.GetBuffer()[1] = _T(':');
			}
	}

	int start=0;
	output=output.Tokenize(_T("\n"),start);

	output.Replace(_T('/'),_T('\\'));

	try
	{
		CStdioFile file(output,CFile::modeRead|CFile::typeText);
		CString str;
		while(file.ReadString(str))
		{
			temp+=str+_T("\n");
		}

	}catch(...)
	{
		return -1;
	}
	return 0;
}
int CAppUtils::SaveCommitUnicodeFile(CString &filename, CString &message)
{
	CFile file(filename,CFile::modeReadWrite|CFile::modeCreate );
	CString cmd,output;
	int cp=CP_UTF8;

	output= g_Git.GetConfigValue(_T("i18n.commitencoding"));
	if(output.IsEmpty())
		cp=CP_UTF8;

	int start=0;
	output=output.Tokenize(_T("\n"),start);
	cp=CUnicodeUtils::GetCPCode(output);

	int len=message.GetLength();

	char * buf;
	buf = new char[len*4 + 4];
	SecureZeroMemory(buf, (len*4 + 4));

	int lengthIncTerminator = WideCharToMultiByte(cp, 0, message, -1, buf, len*4, NULL, NULL);

	file.Write(buf,lengthIncTerminator-1);
	file.Close();
	delete buf;
	return 0;
}

bool CAppUtils::Fetch(CString remoteName, bool allowRebase, bool autoClose)
{
	CPullFetchDlg dlg;
	dlg.m_PreSelectRemote = remoteName;
	dlg.m_bAllowRebase = allowRebase;
	dlg.m_IsPull=FALSE;

	if(dlg.DoModal()==IDOK)
	{
		if(dlg.m_bAutoLoad)
		{
			CAppUtils::LaunchPAgent(NULL,&dlg.m_RemoteURL);
		}

		CString url;
		url=dlg.m_RemoteURL;
		CString cmd;
		CString arg;

		int ver = CAppUtils::GetMsysgitVersion();

		if(ver >= 0x01070203) //above 1.7.0.2
			arg = _T("--progress ");

		if (dlg.m_bPrune) {
			arg += _T("--prune ");
		}

		if (dlg.m_bFetchTags == 1)
		{
			arg += _T("--tags ");
		}
		else if (dlg.m_bFetchTags == 0)
		{
			arg += _T("--no-tags ");
		}

		if (dlg.m_bAllRemotes)
			cmd.Format(_T("git.exe fetch --all -v %s"), arg);
		else
			cmd.Format(_T("git.exe fetch -v %s \"%s\" %s"), arg, url, dlg.m_RemoteBranchName);

		CProgressDlg progress;

		progress.m_bAutoCloseOnSuccess = autoClose;

		progress.m_PostCmdList.Add(CString(MAKEINTRESOURCE(IDS_MENULOG)));

		if(!dlg.m_bRebase && !g_GitAdminDir.IsBareRepo(g_Git.m_CurrentDir))
		{
			progress.m_PostCmdList.Add(CString(MAKEINTRESOURCE(IDS_MENUREBASE)));
		}

		progress.m_GitCmd=cmd;
		int userResponse=progress.DoModal();

		if (userResponse == IDC_PROGRESS_BUTTON1)
		{
			CString cmd = _T("/command:log");
			cmd += _T(" /path:\"") + g_Git.m_CurrentDir + _T("\"");
			RunTortoiseProc(cmd);
			return TRUE;
		}
		else if ((userResponse == IDC_PROGRESS_BUTTON1 + 1) || (progress.m_GitStatus == 0 && dlg.m_bRebase))
		{
			while(1)
			{
				CRebaseDlg dlg;
				dlg.m_PostButtonTexts.Add(CString(MAKEINTRESOURCE(IDS_MENUDESSENDMAIL)));
				dlg.m_PostButtonTexts.Add(CString(MAKEINTRESOURCE(IDS_MENUREBASE)));
				int response = dlg.DoModal();
				if(response == IDOK)
				{
					return TRUE;
				}
				if(response == IDC_REBASE_POST_BUTTON )
				{
					CString cmd, out, err;
					cmd.Format(_T("git.exe  format-patch -o \"%s\" %s..%s"),
						g_Git.m_CurrentDir,
						g_Git.FixBranchName(dlg.m_Upstream),
						g_Git.FixBranchName(dlg.m_Branch));
					if (g_Git.Run(cmd, &out, &err, CP_UTF8))
					{
						CMessageBox::Show(NULL, out + L"\n" + err, _T("TortoiseGit"), MB_OK|MB_ICONERROR);
						return FALSE;
					}

					CAppUtils::SendPatchMail(cmd,out);
					return TRUE;
				}

				if(response == IDC_REBASE_POST_BUTTON +1 )
					continue;

				if(response == IDCANCEL)
					return FALSE;
			}
			return TRUE;
		}
		else if (userResponse != IDCANCEL)
			return TRUE;
	}
	return FALSE;
}

bool CAppUtils::Push(CString selectLocalBranch, bool autoClose)
{
	CPushDlg dlg;
	dlg.m_BranchSourceName = selectLocalBranch;
	CString error;
	DWORD exitcode = 0xFFFFFFFF;
	CTGitPathList list;
	list.AddPath(CTGitPath(g_Git.m_CurrentDir));
	if (CHooks::Instance().PrePush(list,exitcode, error))
	{
		if (exitcode)
		{
			CString temp;
			temp.Format(IDS_ERR_HOOKFAILED, (LPCTSTR)error);
			//ReportError(temp);
			CMessageBox::Show(NULL,temp,_T("TortoiseGit"),MB_OK|MB_ICONERROR);
			return false;
		}
	}

	if(dlg.DoModal()==IDOK)
	{
		CString arg;

		if(dlg.m_bAutoLoad)
		{
			CAppUtils::LaunchPAgent(NULL,&dlg.m_URL);
		}

		if(dlg.m_bPack)
			arg += _T("--thin ");
		if(dlg.m_bTags && !dlg.m_bPushAllBranches)
			arg += _T("--tags ");
		if(dlg.m_bForce)
			arg += _T("--force ");

		int ver = CAppUtils::GetMsysgitVersion();

		if(ver >= 0x01070203) //above 1.7.0.2
			arg += _T("--progress ");

		CProgressDlg progress;
		progress.m_bAutoCloseOnSuccess=autoClose;

		STRING_VECTOR remotesList;
		if (dlg.m_bPushAllRemotes)
			g_Git.GetRemoteList(remotesList);
		else
			remotesList.push_back(dlg.m_URL);

		for (unsigned int i = 0; i < remotesList.size(); i++)
		{
			CString cmd;
			if (dlg.m_bPushAllBranches)
			{
				cmd.Format(_T("git.exe push --all %s \"%s\""),
						arg,
						remotesList[i]);
			}
			else
			{
				cmd.Format(_T("git.exe push %s \"%s\" %s"),
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

		progress.m_PostCmdList.Add(CString(MAKEINTRESOURCE(IDS_PROC_REQUESTPULL)));
		progress.m_PostCmdList.Add(CString(MAKEINTRESOURCE(IDS_MENUPUSH)));
		int ret = progress.DoModal();

		if(!progress.m_GitStatus)
		{
			if (CHooks::Instance().PostPush(list,exitcode, error))
			{
				if (exitcode)
				{
					CString temp;
					temp.Format(IDS_ERR_HOOKFAILED, (LPCTSTR)error);
					//ReportError(temp);
					CMessageBox::Show(NULL,temp,_T("TortoiseGit"),MB_OK|MB_ICONERROR);
					return false;
				}
			}
			if(ret == IDC_PROGRESS_BUTTON1)
			{
				RequestPull(dlg.m_BranchRemoteName);
			}
			else if(ret == IDC_PROGRESS_BUTTON1 + 1)
			{
				Push();
			}
			return TRUE;
		}

	}
	return FALSE;
}

bool CAppUtils::RequestPull(CString endrevision, CString repositoryUrl)
{
	CRequestPullDlg dlg;
	dlg.m_RepositoryURL = repositoryUrl;
	dlg.m_EndRevision = endrevision;
	if (dlg.DoModal()==IDOK)
	{
		CString cmd;
		cmd.Format(_T("git.exe request-pull %s \"%s\" %s"), dlg.m_StartRevision, dlg.m_RepositoryURL, dlg.m_EndRevision);

		CString tempFileName = GetTempFile();
		if (g_Git.RunLogFile(cmd, tempFileName))
		{
			CMessageBox::Show(NULL, IDS_ERR_PULLREUQESTFAILED, IDS_APPNAME, MB_OK);
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

	std::vector<CString>::const_iterator vIter;
	for (vIter = vPath.begin(); vIter != vPath.end(); vIter++)
	{
		bSuccess = CreateDirectory(*vIter, NULL) ? true : false;
	}

	return bSuccess;
}

void CAppUtils::RemoveTrailSlash(CString &path)
{
	if(path.IsEmpty())
		return ;

	while(path[path.GetLength()-1] == _T('\\') || path[path.GetLength()-1] == _T('/' ) )
	{
		path=path.Left(path.GetLength()-1);
		if(path.IsEmpty())
			return;
	}
}

BOOL CAppUtils::Commit(CString bugid,BOOL bWholeProject,CString &sLogMsg,
					CTGitPathList &pathList,
					CTGitPathList &selectedList,
					bool bSelectFilesForCommit,
					bool autoClose)
{
	bool bFailed = true;

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
		dlg.m_bAutoClose = autoClose;
		if (dlg.DoModal() == IDOK)
		{
			if (dlg.m_pathList.GetCount()==0)
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

			if( dlg.m_bPushAfterCommit )
			{
				switch(dlg.m_PostCmd)
				{
				case GIT_POST_CMD_DCOMMIT:
					CAppUtils::SVNDCommit();
					break;
				default:
					CAppUtils::Push();
				}
			}
			else if (dlg.m_bCreateTagAfterCommit)
			{
				CAppUtils::CreateBranchTag(TRUE);
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
		if(CMessageBox::Show(NULL,	IDS_ERROR_NOCLEAN_STASH,IDS_APPNAME,MB_YESNO|MB_ICONINFORMATION)==IDYES)
		{
			CString cmd,out;
			cmd=_T("git.exe stash");
			if (g_Git.Run(cmd, &out, CP_UTF8))
			{
				CMessageBox::Show(NULL,out,_T("TortoiseGit"),MB_OK);
				return false;
			}
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
		if( IsStash)
		{
			if(CMessageBox::Show(NULL,IDS_DCOMMIT_STASH_POP,IDS_APPNAME,MB_YESNO|MB_ICONINFORMATION)==IDYES)
			{
				CString cmd,out;
				cmd=_T("git.exe stash pop");
				if (g_Git.Run(cmd, &out, CP_UTF8))
				{
					CMessageBox::Show(NULL,out,_T("TortoiseGit"),MB_OK);
					return false;
				}

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

BOOL CAppUtils::Merge(CString *commit)
{
	CMergeDlg dlg;
	if(commit)
		dlg.m_initialRefName = *commit;

	if(dlg.DoModal()==IDOK)
	{
		CString cmd;
		CString noff;
		CString squash;
		CString nocommit;
		CString msg;

		if(dlg.m_bNoFF)
			noff=_T("--no-ff");

		if(dlg.m_bSquash)
			squash=_T("--squash");

		if(dlg.m_bNoCommit)
			nocommit=_T("--no-commit");

		if(!dlg.m_strLogMesage.IsEmpty())
		{
			msg += _T("-m \"")+dlg.m_strLogMesage+_T("\"");
		}
		cmd.Format(_T("git.exe merge %s %s %s %s %s"),
			msg,
			noff,
			squash,
			nocommit,
			g_Git.FixBranchName(dlg.m_VersionName));

		CProgressDlg Prodlg;
		Prodlg.m_GitCmd = cmd;

		if (dlg.m_bNoCommit)
			Prodlg.m_PostCmdList.Add(CString(MAKEINTRESOURCE(IDS_MENUCOMMIT)));
		else if (dlg.m_bIsBranch)
			Prodlg.m_PostCmdList.Add(CString(MAKEINTRESOURCE(IDS_PROC_REMOVEBRANCH)));

		int ret = Prodlg.DoModal();

		if (ret == IDC_PROGRESS_BUTTON1)
			if (dlg.m_bNoCommit)
				return Commit(_T(""), TRUE, CString(), CTGitPathList(), CTGitPathList(), true);
			else if (dlg.m_bIsBranch)
			{
				CString msg;
				msg.Format(IDS_PROC_DELETEBRANCHTAG, dlg.m_VersionName);
				if (CMessageBox::Show(NULL, msg, _T("TortoiseGit"), 2, IDI_QUESTION, CString(MAKEINTRESOURCE(IDS_DELETEBUTTON)), CString(MAKEINTRESOURCE(IDS_ABORTBUTTON))) == 1)
				{
					CString cmd, out;
					cmd.Format(_T("git.exe branch -D -- %s"), dlg.m_VersionName);
					if (g_Git.Run(cmd, &out, CP_UTF8))
						MessageBox(NULL, out, _T("TortoiseGit"), MB_OK);
				}
			}

		return !Prodlg.m_GitStatus;
	}
	return false;
}

void CAppUtils::EditNote(GitRev *rev)
{
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
		CAppUtils::SaveCommitUnicodeFile(tempfile,dlg.m_sInputText);
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
		CFile::Remove(tempfile);

	}
}

int CAppUtils::GetMsysgitVersion(CString *versionstr)
{
	CString cmd;
	CString progressarg;
	CString version;

	CRegDWORD regTime		= CRegDWORD(_T("Software\\TortoiseGit\\git_file_time"));
	CRegDWORD regVersion	= CRegDWORD(_T("Software\\TortoiseGit\\git_cached_version"));

	CString gitpath = CGit::ms_LastMsysGitDir+_T("\\git.exe");

	__int64 time=0;
	if(!g_Git.GetFileModifyTime(gitpath, &time) && !versionstr)
	{
		if((DWORD)time == regTime)
		{
			return regVersion;
		}
	}

	if(versionstr)
		version = *versionstr;
	else
	{
		CString err;
		cmd = _T("git.exe --version");
		if (g_Git.Run(cmd, &version, &err, CP_UTF8))
		{
			CMessageBox::Show(NULL, _T("git have not installed (") + err + _T(")"), _T("TortoiseGit"), MB_OK|MB_ICONERROR);
			return false;
		}
	}

	int start=0;
	int ver = 0;

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
			CMessageBox::Show(NULL, _T("Could not parse git.exe version number."), _T("TortoiseGit"), MB_OK|MB_ICONERROR);
			return false;
		}
	}

	regTime = time&0xFFFFFFFF;
	regVersion = ver;

	return ver;
}

void CAppUtils::MarkWindowAsUnpinnable(HWND hWnd)
{
	typedef HRESULT (WINAPI *SHGPSFW) (HWND hwnd,REFIID riid,void** ppv);

	CAutoLibrary hShell = LoadLibrary(_T("Shell32.dll"));

	if (hShell.IsValid()) {
		SHGPSFW pfnSHGPSFW = (SHGPSFW)::GetProcAddress(hShell, "SHGetPropertyStoreForWindow");
		if (pfnSHGPSFW) {
			IPropertyStore *pps;
			HRESULT hr = pfnSHGPSFW(hWnd, IID_PPV_ARGS(&pps));
			if (SUCCEEDED(hr)) {
				PROPVARIANT var;
				var.vt = VT_BOOL;
				var.boolVal = VARIANT_TRUE;
				hr = pps->SetValue(PKEY_AppUserModel_PreventPinning, var);
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
