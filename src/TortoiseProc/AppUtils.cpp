// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2003-2008 - TortoiseGit

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
#include "SVNProgressDlg.h"
#include "PushDlg.h"
#include "CommitDlg.h"
#include "MergeDlg.h"

CAppUtils::CAppUtils(void)
{
}

CAppUtils::~CAppUtils(void)
{
}

int	 CAppUtils::StashApply(CString ref)
{
	CString cmd,out;
	cmd=_T("git.exe stash apply ");
	cmd+=ref;
	
	if(g_Git.Run(cmd,&out,CP_ACP))
	{
		CMessageBox::Show(NULL,CString(_T("<ct=0x0000FF>Stash Apply Fail!!!</ct>\n"))+out,_T("TortoiseGit"),MB_OK|MB_ICONERROR);

	}else
	{
 		if(CMessageBox::Show(NULL,CString(_T("<ct=0xff0000>Stash Apply Success</ct>\nDo you want to show change?"))
			,_T("TortoiseGit"),MB_YESNO|MB_ICONINFORMATION) == IDYES)
		{
			CChangedDlg dlg;
			dlg.m_pathList.AddPath(CTGitPath());
			dlg.DoModal();			
		}
		return 0;
	}
	return -1;
}

int	 CAppUtils::StashPop()
{
	CString cmd,out;
	cmd=_T("git.exe stash pop ");
		
	if(g_Git.Run(cmd,&out,CP_ACP))
	{
		CMessageBox::Show(NULL,CString(_T("<ct=0x0000FF>Stash POP Fail!!!</ct>\n"))+out,_T("TortoiseGit"),MB_OK|MB_ICONERROR);

	}else
	{
 		if(CMessageBox::Show(NULL,CString(_T("<ct=0xff0000>Stash POP Success</ct>\nDo you want to show change?"))
			,_T("TortoiseGit"),MB_YESNO|MB_ICONINFORMATION) == IDYES)
		{
			CChangedDlg dlg;
			dlg.m_pathList.AddPath(CTGitPath());
			dlg.DoModal();			
		}
		return 0;
	}
	return -1;
}

bool CAppUtils::GetMimeType(const CTGitPath& file, CString& mimetype)
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

BOOL CAppUtils::StartExtDiffProps(const CTGitPath& file1, const CTGitPath& file2, const CString& sName1, const CString& sName2, BOOL bWait, BOOL bReadOnly)
{
	CRegString diffpropsexe(_T("Software\\TortoiseGit\\DiffProps"));
	CString viewer = diffpropsexe;
	bool bInternal = false;
	if (viewer.IsEmpty()||(viewer.Left(1).Compare(_T("#"))==0))
	{
		//no registry entry (or commented out) for a diff program
		//use TortoiseMerge
		bInternal = true;
		viewer = CPathUtils::GetAppDirectory();
		viewer += _T("TortoiseMerge.exe");
		viewer = _T("\"") + viewer + _T("\"");
		viewer = viewer + _T(" /base:%base /mine:%mine /basename:%bname /minename:%yname");
	}
	// check if the params are set. If not, just add the files to the command line
	if ((viewer.Find(_T("%base"))<0)&&(viewer.Find(_T("%mine"))<0))
	{
		viewer += _T(" \"")+file1.GetWinPathString()+_T("\"");
		viewer += _T(" \"")+file2.GetWinPathString()+_T("\"");
	}
	if (viewer.Find(_T("%base")) >= 0)
	{
		viewer.Replace(_T("%base"),  _T("\"")+file1.GetWinPathString()+_T("\""));
	}
	if (viewer.Find(_T("%mine")) >= 0)
	{
		viewer.Replace(_T("%mine"),  _T("\"")+file2.GetWinPathString()+_T("\""));
	}

	if (sName1.IsEmpty())
		viewer.Replace(_T("%bname"), _T("\"") + file1.GetUIFileOrDirectoryName() + _T("\""));
	else
		viewer.Replace(_T("%bname"), _T("\"") + sName1 + _T("\""));

	if (sName2.IsEmpty())
		viewer.Replace(_T("%yname"), _T("\"") + file2.GetUIFileOrDirectoryName() + _T("\""));
	else
		viewer.Replace(_T("%yname"), _T("\"") + sName2 + _T("\""));

	if ((bReadOnly)&&(bInternal))
		viewer += _T(" /readonly");

	if(!LaunchApplication(viewer, IDS_ERR_EXTDIFFSTART, !!bWait))
	{
		return FALSE;
	}
	return TRUE;
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
		OPENFILENAME ofn = {0};				// common dialog box structure
		TCHAR szFile[MAX_PATH] = {0};		// buffer for file name. Explorer can't handle paths longer than MAX_PATH.
		// Initialize OPENFILENAME
		ofn.lStructSize = sizeof(OPENFILENAME);
		ofn.hwndOwner = NULL;
		ofn.lpstrFile = szFile;
		ofn.nMaxFile = sizeof(szFile)/sizeof(TCHAR);
		CString sFilter;
		sFilter.LoadString(IDS_PROGRAMSFILEFILTER);
		TCHAR * pszFilters = new TCHAR[sFilter.GetLength()+4];
		_tcscpy_s (pszFilters, sFilter.GetLength()+4, sFilter);
		// Replace '|' delimiters with '\0's
		TCHAR *ptr = pszFilters + _tcslen(pszFilters);  //set ptr at the NULL
		while (ptr != pszFilters)
		{
			if (*ptr == '|')
				*ptr = '\0';
			ptr--;
		}
		ofn.lpstrFilter = pszFilters;
		ofn.nFilterIndex = 1;
		ofn.lpstrFileTitle = NULL;
		ofn.nMaxFileTitle = 0;
		ofn.lpstrInitialDir = NULL;
		CString temp;
		temp.LoadString(IDS_UTILS_SELECTTEXTVIEWER);
		CStringUtils::RemoveAccelerators(temp);
		ofn.lpstrTitle = temp;
		ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;

		// Display the Open dialog box. 

		if (GetOpenFileName(&ofn)==TRUE)
		{
			delete [] pszFilters;
			viewer = CString(ofn.lpstrFile);
		}
		else
		{
			delete [] pszFilters;
			return FALSE;
		}
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
	HANDLE hFile = ::CreateFile(sDiffPath.GetWinPath(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, NULL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
		return TRUE;
	length = ::GetFileSize(hFile, NULL);
	::CloseHandle(hFile);
	if (length < 4)
		return TRUE;
	return FALSE;

}

void CAppUtils::CreateFontForLogs(CFont& fontToCreate)
{
	LOGFONT logFont;
	HDC hScreenDC = ::GetDC(NULL);
	logFont.lfHeight         = -MulDiv((DWORD)CRegDWORD(_T("Software\\TortoiseGit\\LogFontSize"), 8), GetDeviceCaps(hScreenDC, LOGPIXELSY), 72);
	::ReleaseDC(NULL, hScreenDC);
	logFont.lfWidth          = 0;
	logFont.lfEscapement     = 0;
	logFont.lfOrientation    = 0;
	logFont.lfWeight         = FW_NORMAL;
	logFont.lfItalic         = 0;
	logFont.lfUnderline      = 0;
	logFont.lfStrikeOut      = 0;
	logFont.lfCharSet        = DEFAULT_CHARSET;
	logFont.lfOutPrecision   = OUT_DEFAULT_PRECIS;
	logFont.lfClipPrecision  = CLIP_DEFAULT_PRECIS;
	logFont.lfQuality        = DRAFT_QUALITY;
	logFont.lfPitchAndFamily = FF_DONTCARE | FIXED_PITCH;
	_tcscpy_s(logFont.lfFaceName, 32, (LPCTSTR)(CString)CRegString(_T("Software\\TortoiseGit\\LogFontName"), _T("Courier New")));
	VERIFY(fontToCreate.CreateFontIndirect(&logFont));
}

bool CAppUtils::LaunchApplication(const CString& sCommandLine, UINT idErrMessageFormat, bool bWaitForStartup)
{
	STARTUPINFO startup;
	PROCESS_INFORMATION process;
	memset(&startup, 0, sizeof(startup));
	startup.cb = sizeof(startup);
	memset(&process, 0, sizeof(process));

	CString cleanCommandLine(sCommandLine);

	if (CreateProcess(NULL, const_cast<TCHAR*>((LPCTSTR)cleanCommandLine), NULL, NULL, FALSE, 0, 0, g_Git.m_CurrentDir, &startup, &process)==0)
	{
		if(idErrMessageFormat != 0)
		{
			LPVOID lpMsgBuf;
			FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | 
				FORMAT_MESSAGE_FROM_SYSTEM | 
				FORMAT_MESSAGE_IGNORE_INSERTS,
				NULL,
				GetLastError(),
				MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
				(LPTSTR) &lpMsgBuf,
				0,
				NULL 
				);
			CString temp;
			temp.Format(idErrMessageFormat, lpMsgBuf);
			CMessageBox::Show(NULL, temp, _T("TortoiseGit"), MB_OK | MB_ICONINFORMATION);
			LocalFree( lpMsgBuf );
		}
		return false;
	}

	if (bWaitForStartup)
	{
		WaitForInputIdle(process.hProcess, 10000);
	}

	CloseHandle(process.hThread);
	CloseHandle(process.hProcess);
	return true;
}
bool CAppUtils::LaunchPAgent(CString *keyfile,CString * pRemote)
{
	CString key,remote;
	CString cmd,out;
	if( pRemote == NULL)
	{
		remote=_T("origin");
	}else
	{
		remote=*pRemote;
	}
	if(keyfile == NULL)
	{
		cmd.Format(_T("git.exe config remote.%s.puttykeyfile"),remote);
		g_Git.Run(cmd,&key,CP_ACP);
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

    return LaunchApplication(proc, IDS_ERR_PAGEANT, false);
}
bool CAppUtils::LaunchRemoteSetting()
{
    CString proc=CPathUtils::GetAppDirectory();
    proc += _T("TortoiseProc.exe /command:settings");
    proc += _T(" /path:\"");
    proc += g_Git.m_CurrentDir;
    proc += _T("\" /page:gitremote");
    return LaunchApplication(proc, IDS_ERR_EXTDIFFSTART, false);
}
/**
* Launch the external blame viewer
*/
bool CAppUtils::LaunchTortoiseBlame(const CString& sBlameFile,CString Rev,const CString& sParams)
{
	CString viewer = CPathUtils::GetAppDirectory();
	viewer += _T("TortoiseGitBlame.exe");
	viewer += _T(" \"") + sBlameFile + _T("\"");
	//viewer += _T(" \"") + sLogFile + _T("\"");
	//viewer += _T(" \"") + sOriginalFile + _T("\"");
	if(!Rev.IsEmpty())
		viewer += CString(_T(" /rev:"))+Rev;
	viewer += _T(" ")+sParams;
	
	return LaunchApplication(viewer, IDS_ERR_EXTDIFFSTART, false);
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
	sText.Replace(_T("\r"), _T(""));

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

bool CAppUtils::BrowseRepository(CHistoryCombo& combo, CWnd * pParent, GitRev& rev)
{
#if 0
	CString strUrl;
	combo.GetWindowText(strUrl);
	strUrl.Replace('\\', '/');
	strUrl.Replace(_T("%"), _T("%25"));
	strUrl = CUnicodeUtils::GetUnicode(CPathUtils::PathEscape(CUnicodeUtils::GetUTF8(strUrl)));
	if (strUrl.Left(7) == _T("file://"))
	{
		CString strFile(strUrl);
		Git::UrlToPath(strFile);

		Git svn;
		if (svn.IsRepository(CTGitPath(strFile)))
		{
			// browse repository - show repository browser
			Git::preparePath(strUrl);
			CRepositoryBrowser browser(strUrl, rev, pParent);
			if (browser.DoModal() == IDOK)
			{
				combo.SetCurSel(-1);
				combo.SetWindowText(browser.GetPath());
				rev = browser.GetRevision();
				return true;
			}
		}
		else
		{
			// browse local directories
			CBrowseFolder folderBrowser;
			folderBrowser.m_style = BIF_EDITBOX | BIF_NEWDIALOGSTYLE | BIF_RETURNFSANCESTORS | BIF_RETURNONLYFSDIRS;
			// remove the 'file:///' so the shell can recognize the local path
			Git::UrlToPath(strUrl);
			if (folderBrowser.Show(pParent->GetSafeHwnd(), strUrl) == CBrowseFolder::OK)
			{
				Git::PathToUrl(strUrl);

				combo.SetCurSel(-1);
				combo.SetWindowText(strUrl);
				return true;
			}
		}
	}
	else if ((strUrl.Left(7) == _T("http://")
		||(strUrl.Left(8) == _T("https://"))
		||(strUrl.Left(6) == _T("svn://"))
		||(strUrl.Left(4) == _T("svn+"))) && strUrl.GetLength() > 6)
	{
		// browse repository - show repository browser
		CRepositoryBrowser browser(strUrl, rev, pParent);
		if (browser.DoModal() == IDOK)
		{
			combo.SetCurSel(-1);
			combo.SetWindowText(browser.GetPath());
			rev = browser.GetRevision();
			return true;
		}
	}
	else
	{
		// browse local directories
		CBrowseFolder folderBrowser;
		folderBrowser.m_style = BIF_EDITBOX | BIF_NEWDIALOGSTYLE | BIF_RETURNFSANCESTORS | BIF_RETURNONLYFSDIRS;
		if (folderBrowser.Show(pParent->GetSafeHwnd(), strUrl) == CBrowseFolder::OK)
		{
			Git::PathToUrl(strUrl);

			combo.SetCurSel(-1);
			combo.SetWindowText(strUrl);
			return true;
		}
	}
#endif
	return false;
}

bool CAppUtils::FileOpenSave(CString& path, int * filterindex, UINT title, UINT filter, bool bOpen, HWND hwndOwner)
{
	OPENFILENAME ofn = {0};				// common dialog box structure
	TCHAR szFile[MAX_PATH] = {0};		// buffer for file name. Explorer can't handle paths longer than MAX_PATH.
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = hwndOwner;
	_tcscpy_s(szFile, MAX_PATH, (LPCTSTR)path);
	ofn.lpstrFile = szFile;
	ofn.nMaxFile = sizeof(szFile)/sizeof(TCHAR);
	CString sFilter;
	TCHAR * pszFilters = NULL;
	if (filter)
	{
		sFilter.LoadString(filter);
		pszFilters = new TCHAR[sFilter.GetLength()+4];
		_tcscpy_s (pszFilters, sFilter.GetLength()+4, sFilter);
		// Replace '|' delimiters with '\0's
		TCHAR *ptr = pszFilters + _tcslen(pszFilters);  //set ptr at the NULL
		while (ptr != pszFilters)
		{
			if (*ptr == '|')
				*ptr = '\0';
			ptr--;
		}
		ofn.lpstrFilter = pszFilters;
	}
	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = NULL;
	CString temp;
	if (title)
	{
		temp.LoadString(title);
		CStringUtils::RemoveAccelerators(temp);
	}
	ofn.lpstrTitle = temp;
	if (bOpen)
		ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_EXPLORER;
	else
		ofn.Flags = OFN_OVERWRITEPROMPT | OFN_EXPLORER;


	// Display the Open dialog box. 
	bool bRet = false;
	if (bOpen)
	{
		bRet = !!GetOpenFileName(&ofn);
	}
	else
	{
		bRet = !!GetSaveFileName(&ofn);
	}
	if (bRet)
	{
		if (pszFilters)
			delete [] pszFilters;
		path = CString(ofn.lpstrFile);
		if (filterindex)
			*filterindex = ofn.nFilterIndex;
		return true;
	}
	if (pszFilters)
		delete [] pszFilters;
	return false;
}

bool CAppUtils::SetListCtrlBackgroundImage(HWND hListCtrl, UINT nID, int width /* = 128 */, int height /* = 128 */)
{
	ListView_SetTextBkColor(hListCtrl, CLR_NONE);
	COLORREF bkColor = ListView_GetBkColor(hListCtrl);
	// create a bitmap from the icon
	HICON hIcon = (HICON)LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(nID), IMAGE_ICON, width, height, LR_DEFAULTCOLOR);
	if (!hIcon)
		return false;

	RECT rect = {0};
	rect.right = width;
	rect.bottom = height;
	HBITMAP bmp = NULL;

	HWND desktop = ::GetDesktopWindow();
	if (desktop)
	{
		HDC screen_dev = ::GetDC(desktop);
		if (screen_dev)
		{
			// Create a compatible DC
			HDC dst_hdc = ::CreateCompatibleDC(screen_dev);
			if (dst_hdc)
			{
				// Create a new bitmap of icon size
				bmp = ::CreateCompatibleBitmap(screen_dev, rect.right, rect.bottom);
				if (bmp)
				{
					// Select it into the compatible DC
					HBITMAP old_dst_bmp = (HBITMAP)::SelectObject(dst_hdc, bmp);
					// Fill the background of the compatible DC with the given color
					::SetBkColor(dst_hdc, bkColor);
					::ExtTextOut(dst_hdc, 0, 0, ETO_OPAQUE, &rect, NULL, 0, NULL);

					// Draw the icon into the compatible DC
					::DrawIconEx(dst_hdc, 0, 0, hIcon, rect.right, rect.bottom, 0, NULL, DI_NORMAL);
					::SelectObject(dst_hdc, old_dst_bmp);
				}
				::DeleteDC(dst_hdc);
			}
		}
		::ReleaseDC(desktop, screen_dev); 
	}

	// Restore settings
	DestroyIcon(hIcon);

	if (bmp == NULL)
		return false;

	LVBKIMAGE lv;
	lv.ulFlags = LVBKIF_TYPE_WATERMARK;
	lv.hbm = bmp;
	lv.xOffsetPercent = 100;
	lv.yOffsetPercent = 100;
	ListView_SetBkImage(hListCtrl, &lv);
	return true;
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

bool CAppUtils::StartShowUnifiedDiff(HWND hWnd, const CTGitPath& url1, const git_revnum_t& rev1, 
												const CTGitPath& url2, const git_revnum_t& rev2, 
									 //const GitRev& peg /* = GitRev */, const GitRev& headpeg /* = GitRev */,  
												bool bAlternateDiff /* = false */, bool bIgnoreAncestry /* = false */, bool /* blame = false */)
{

	CString tempfile=GetTempFile();
	CString cmd;
	if(rev1 == GitRev::GetWorkingCopy())
	{
		cmd.Format(_T("git.exe diff --stat -p %s "),rev2);
	}else
	{	
		cmd.Format(_T("git.exe diff-tree -r -p --stat %s %s"),rev1,rev2);
	}

	if( !url1.IsEmpty() )
	{
		cmd+=_T(" \"");
		cmd+=url1.GetGitPathString();
		cmd+=_T("\" ");
	}
	g_Git.RunLogFile(cmd,tempfile);
	CAppUtils::StartUnifiedDiffViewer(tempfile,rev1.Left(6)+_T(":")+rev2.Left(6));


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

bool CAppUtils::StartShowCompare(HWND hWnd, const CTGitPath& url1, const GitRev& rev1, 
								 const CTGitPath& url2, const GitRev& rev2, 
								 const GitRev& peg /* = GitRev */, const GitRev& headpeg /* = GitRev */, 
								 bool bAlternateDiff /* = false */, bool bIgnoreAncestry /* = false */, bool blame /* = false */)
{
#if 0
	CString sCmd;
	sCmd.Format(_T("%s /command:showcompare"),
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
	if (blame)
		sCmd += _T(" /blame");

	if (hWnd)
	{
		sCmd += _T(" /hwnd:");
		TCHAR buf[30];
		_stprintf_s(buf, 30, _T("%d"), hWnd);
		sCmd += buf;
	}

	return CAppUtils::LaunchApplication(sCmd, NULL, false);
#endif
	return true;
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
		cmd.Format(_T("git.exe archive --format=zip --verbose %s"),
					dlg.m_VersionName);

		//g_Git.RunLogFile(cmd,dlg.m_strExportDirectory);
		CProgressDlg pro;
		pro.m_GitCmd=cmd;
		pro.m_LogFile=dlg.m_strExportDirectory;
		pro.DoModal();
		return TRUE;
	}
	return bRet;
}

bool CAppUtils::CreateBranchTag(bool IsTag,CString *CommitHash)
{
	CCreateBranchTagDlg dlg;
	dlg.m_bIsTag=IsTag;
	if(CommitHash)
		dlg.m_Base = *CommitHash;

	if(dlg.DoModal()==IDOK)
	{
		CString cmd;
		CString force;
		CString track;
		if(dlg.m_bTrack)
			track=_T(" --track ");

		if(dlg.m_bForce)
			force=_T(" -f ");

		if(IsTag)
		{
			cmd.Format(_T("git.exe tag %s %s %s %s"),
				track,
				force,
				dlg.m_BranchTagName,
				dlg.m_VersionName
				);
			
			CString tempfile=::GetTempFile();
			if(!dlg.m_Message.Trim().IsEmpty())
			{
				CAppUtils::SaveCommitUnicodeFile(tempfile,dlg.m_Message);
				cmd += _T(" -F ")+tempfile;
			}
	
		}else
		{
			cmd.Format(_T("git.exe branch %s %s %s %s"),
				track,
				force,
				dlg.m_BranchTagName,
				dlg.m_VersionName
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

bool CAppUtils::Switch(CString *CommitHash, CString initialRefName)
{
	CGitSwitchDlg dlg;
	if(CommitHash)
		dlg.m_Base=*CommitHash;
	if(!initialRefName.IsEmpty())
		dlg.m_initialRefName = initialRefName;
	
	if (dlg.DoModal() == IDOK)
	{
		CString cmd;
		CString track;
//		CString base;
		CString force;
		CString branch;

		if(dlg.m_bBranch)
			branch.Format(_T("-b %s"),dlg.m_NewBranch);
		if(dlg.m_bForce)
			force=_T("-f");
		if(dlg.m_bTrack)
			track=_T("--track");

		cmd.Format(_T("git.exe checkout %s %s %s %s"),
			 force,
			 track,
			 branch,
			 dlg.m_VersionName);

		CProgressDlg progress;
		progress.m_GitCmd=cmd;
		if(progress.DoModal()==IDOK)
			return TRUE;

	}
	return FALSE;
}

bool CAppUtils::IgnoreFile(CTGitPathList &path,bool IsMask)
{
	CString ignorefile;
	ignorefile=g_Git.m_CurrentDir+_T("\\");

	if(IsMask)
	{
		ignorefile+=path.GetCommonRoot().GetDirectory().GetWinPathString()+_T("\\.gitignore");

	}else
	{
		ignorefile+=_T("\\.gitignore");
	}

	CStdioFile file;
	if(!file.Open(ignorefile,CFile::modeCreate|CFile::modeReadWrite|CFile::modeNoTruncate))
	{
		CMessageBox::Show(NULL,ignorefile+_T(" Open Failure"),_T("TortoiseGit"),MB_OK);
		return FALSE;
	}

	CString ignorelist;
	CString mask;
	try
	{
		//file.ReadString(ignorelist);
		file.SeekToEnd();
		for(int i=0;i<path.GetCount();i++)
		{
			if(IsMask)
			{
				mask=_T("*")+path[i].GetFileExtension();
				if(ignorelist.Find(mask)<0)
					ignorelist+=_T("\n")+mask;
				
			}else
			{
				ignorelist+=_T("\n/")+path[i].GetGitPathString();
			}
		}
		file.WriteString(ignorelist);

		file.Close();

	}catch(...)
	{
		file.Close();
		return FALSE;
	}
	
	return TRUE;
}


bool CAppUtils::GitReset(CString *CommitHash,int type)
{
	CResetDlg dlg;
	dlg.m_ResetType=type;
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
		if(progress.DoModal()==IDOK)
			return TRUE;

	}
	return FALSE;
}

void CAppUtils::DescribeFile(bool mode, bool base,CString &descript)
{
	if(mode == FALSE)
	{
		descript=_T("Deleted");
		return;
	}
	if(base)
	{
		descript=_T("Modified");
		return;
	}
	descript=_T("Created");
	return;
}

CString CAppUtils::GetMergeTempFile(CString type,CTGitPath &merge)
{
	CString file;
	file=g_Git.m_CurrentDir+_T("\\")+merge.GetDirectory().GetWinPathString()+_T("\\")+merge.GetFilename()+_T(".")+type+merge.GetFileExtension();

	return file;
}

bool CAppUtils::ConflictEdit(CTGitPath &path,bool bAlternativeTool,bool revertTheirMy)
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

	if(g_Git.Run(cmd,&vector))
	{
		return FALSE;
	}

	CTGitPathList list;
	list.ParserFromLsFile(vector);

	if(list.GetCount() == 0)
		return FALSE;

	TCHAR szTempName[512];  
	GetTempFileName(_T(""),_T(""),0,szTempName);
	CString temp(szTempName);
	temp=temp.Mid(1,temp.GetLength()-5);

	CTGitPath theirs;
	CTGitPath mine;
	CTGitPath base;

	
	mine.SetFromGit(GetMergeTempFile(_T("LOCAL"),merge));
	theirs.SetFromGit(GetMergeTempFile(_T("REMOTE"),merge));
	base.SetFromGit(GetMergeTempFile(_T("BASE"),merge));

	CString format;

	format=_T("git.exe cat-file blob \":%d:%s\"");
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
		cmd.Format(format,list[i].m_Stage,list[i].GetGitPathString());
		
		if( list[i].m_Stage == 1)
		{
			b_base = true;
			outfile=base.GetWinPathString();
		}
		if( list[i].m_Stage == 2 )
		{
			b_local = true;
			outfile=mine.GetWinPathString();
		}
		if( list[i].m_Stage == 3 )
		{
			b_remote = true;
			outfile=theirs.GetWinPathString();
		}	
		g_Git.RunLogFile(cmd,outfile);
	}

	if(b_local && b_remote )
	{
		merge.SetFromWin(g_Git.m_CurrentDir+_T("\\")+merge.GetWinPathString());
		if( revertTheirMy )
			bRet = !!CAppUtils::StartExtMerge(base,mine, theirs,  merge,_T("BASE"),_T("LOCAL"),_T("REMOTE"));
		else
			bRet = !!CAppUtils::StartExtMerge(base, theirs, mine, merge,_T("BASE"),_T("REMOTE"),_T("LOCAL"));
	
	}else
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
			}else
				cmd.Format(_T("git.exe add -- \"%s\""),merge.GetGitPathString());

			if(g_Git.Run(cmd,&out,CP_ACP))
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

/**
 * FUNCTION    :   FormatDateAndTime
 * DESCRIPTION :   Generates a displayable string from a CTime object in
 *                 system short or long format  or as a relative value
 *				   cTime - the time
 *				   option - DATE_SHORTDATE or DATE_LONGDATE
 *				   bIncluedeTime - whether to show time as well as date
 *				   bRelative - if true then relative time is shown if reasonable 
 *				   If HKCU\Software\TortoiseGit\UseSystemLocaleForDates is 0 then use fixed format
 *				   rather than locale
 * RETURN      :   CString containing date/time
 */
CString CAppUtils::FormatDateAndTime( const CTime& cTime, DWORD option, bool bIncludeTime /*=true*/,
	bool bRelative /*=false*/)
{
	CString datetime;
	if ( bRelative )
	{
		datetime = ToRelativeTimeString( cTime );
	}
	else
	{
		// should we use the locale settings for formatting the date/time?
		if (CRegDWORD(_T("Software\\TortoiseGit\\UseSystemLocaleForDates"), TRUE))
		{
			// yes
			SYSTEMTIME sysTime;
			cTime.GetAsSystemTime( sysTime );
			
			TCHAR buf[100];
			
			GetDateFormat(LOCALE_USER_DEFAULT, option, &sysTime, NULL, buf, 
				sizeof(buf)/sizeof(TCHAR)-1);
			datetime = buf;
			if ( bIncludeTime )
			{
				datetime += _T(" ");
				GetTimeFormat(LOCALE_USER_DEFAULT, 0, &sysTime, NULL, buf, sizeof(buf)/sizeof(TCHAR)-1);
				datetime += buf;
			}
		}
		else
		{
			// no, so fixed format
			if ( bIncludeTime )
			{
				datetime = cTime.Format(_T("%Y-%m-%d %H:%M:%S"));
			}
			else
			{
				datetime = cTime.Format(_T("%Y-%m-%d"));
			}
		}
	}
	return datetime;
}

/**
 *	Converts a given time to a relative display string (relative to current time)
 *	Given time must be in local timezone
 */
CString CAppUtils::ToRelativeTimeString(CTime time)
{
    CString answer;
	// convert to COleDateTime
	SYSTEMTIME sysTime;
	time.GetAsSystemTime( sysTime );
	COleDateTime oleTime( sysTime );
	answer = ToRelativeTimeString(oleTime, COleDateTime::GetCurrentTime());
	return answer;
}

/**
 *	Generates a display string showing the relative time between the two given times as COleDateTimes
 */
CString CAppUtils::ToRelativeTimeString(COleDateTime time,COleDateTime RelativeTo)
{
    CString answer;
	COleDateTimeSpan ts = RelativeTo - time;
    //years
	if(fabs(ts.GetTotalDays()) >= 3*365)
    {
		answer = ExpandRelativeTime( (int)ts.GetTotalDays()/365, IDS_YEAR_AGO, IDS_YEARS_AGO );
	}
	//Months
	if(fabs(ts.GetTotalDays()) >= 60)
	{
		answer = ExpandRelativeTime( (int)ts.GetTotalDays()/30, IDS_MONTH_AGO, IDS_MONTHS_AGO );
		return answer;
	}
	//Weeks
	if(fabs(ts.GetTotalDays()) >= 14)
	{
		answer = ExpandRelativeTime( (int)ts.GetTotalDays()/7, IDS_WEEK_AGO, IDS_WEEKS_AGO );
		return answer;
	}
	//Days
	if(fabs(ts.GetTotalDays()) >= 2)
	{
		answer = ExpandRelativeTime( (int)ts.GetTotalDays(), IDS_DAY_AGO, IDS_DAYS_AGO );
		return answer;
	}
	//hours
	if(fabs(ts.GetTotalHours()) >= 2)
	{
		answer = ExpandRelativeTime( (int)ts.GetTotalHours(), IDS_HOUR_AGO, IDS_HOURS_AGO );
		return answer;
	}
	//minutes
	if(fabs(ts.GetTotalMinutes()) >= 2)
	{
		answer = ExpandRelativeTime( (int)ts.GetTotalMinutes(), IDS_MINUTE_AGO, IDS_MINUTES_AGO );
		return answer;
	}
	//seconds
		answer = ExpandRelativeTime( (int)ts.GetTotalSeconds(), IDS_SECOND_AGO, IDS_SECONDS_AGO );
    return answer;
}

/** 
 * Passed a value and two resource string ids
 * if count is 1 then FormatString is called with format_1 and the value
 * otherwise format_2 is used
 * the formatted string is returned
*/
CString CAppUtils::ExpandRelativeTime( int count, UINT format_1, UINT format_n )
{
	CString answer;
	if ( count == 1 )
	{
		answer.FormatMessage( format_1, count );
	}
	else
	{
		answer.FormatMessage( format_n, count );
	}
	return answer;
}

bool CAppUtils::IsSSHPutty()
{
    CString sshclient=CRegString(_T("Software\\TortoiseGit\\SSH"));
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
	browseFolder.m_style = BIF_EDITBOX | BIF_NEWDIALOGSTYLE | BIF_RETURNFSANCESTORS | BIF_RETURNONLYFSDIRS;
	CString strCloneDirectory;
	if(path)
		strCloneDirectory=*path;

	CString title;
	title.LoadString(IDS_CHOOSE_REPOSITORY);

	browseFolder.SetInfo(title);

	if (browseFolder.Show(NULL, strCloneDirectory) == CBrowseFolder::OK) 
	{
		return strCloneDirectory;
		
	}else
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
		if(one.IsEmpty())
			continue;
		one.Replace(_T('/'),_T('\\'));
		CTGitPath path;
		path.SetFromWin(one);
		list.AddPath(path);
	}
	return SendPatchMail(list,autoclose);
}


int CAppUtils::GetLogOutputEncode(CGit *pGit)
{
	CString cmd,output;
	int start=0;
	cmd=_T("git.exe config i18n.logOutputEncoding");
	if(pGit->Run(cmd,&output,CP_ACP))
	{
		cmd=_T("git.exe config i18n.commitencoding");
		if(pGit->Run(cmd,&output,CP_ACP))
			return CP_UTF8;
	
		int start=0;
		output=output.Tokenize(_T("\n"),start);
		return CUnicodeUtils::GetCPCode(output);	

	}else
	{
		output=output.Tokenize(_T("\n"),start);
		return CUnicodeUtils::GetCPCode(output);
	}
}
int CAppUtils::GetCommitTemplate(CString &temp)
{
	CString cmd,output;
	cmd = _T("git.exe config commit.template");
	if( g_Git.Run(cmd,&output,CP_ACP) )
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

	cmd=_T("git.exe config i18n.commitencoding");
	if(g_Git.Run(cmd,&output,CP_ACP))
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

bool CAppUtils::Push()
{
	CPushDlg dlg;
//	dlg.m_Directory=this->orgCmdLinePath.GetWinPathString();
	if(dlg.DoModal()==IDOK)
	{
//		CString dir=dlg.m_Directory;
//		CString url=dlg.m_URL;
		CString cmd;
		CString force;
		CString tags;
		CString thin;

		if(dlg.m_bAutoLoad)
		{
			CAppUtils::LaunchPAgent(NULL,&dlg.m_URL);
		}

		if(dlg.m_bPack)
			thin=_T("--thin");
		if(dlg.m_bTags)
			tags=_T("--tags");
		if(dlg.m_bForce)
			force=_T("--force");
		
		cmd.Format(_T("git.exe push %s %s %s \"%s\" %s"),
				thin,tags,force,
				dlg.m_URL,
				dlg.m_BranchSourceName);
		if (!dlg.m_BranchRemoteName.IsEmpty())
		{
			cmd += _T(":") + dlg.m_BranchRemoteName;
		}

		CProgressDlg progress;
		progress.m_GitCmd=cmd;
		if(progress.DoModal()==IDOK)
			return TRUE;
		
	}
	return FALSE;
}

bool CAppUtils::CreateMultipleDirectory(CString& szPath)
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
					BOOL bSelectFilesForCommit)
{
	bool bFailed = true;
	
	g_Git.RefreshGitIndex();

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
	if(!g_Git.CheckCleanWorkTree())
	{
		if(CMessageBox::Show(NULL,	IDS_ERROR_NOCLEAN_STASH,IDS_APPNAME,MB_YESNO|MB_ICONINFORMATION)==IDYES)
		{
			CString cmd,out;
			cmd=_T("git.exe stash");
			if(g_Git.Run(cmd,&out,CP_ACP))
			{
				CMessageBox::Show(NULL,out,_T("TortoiseGit"),MB_OK);
				return false;
			}

		}else
		{
			return false;
		}
	}

	CProgressDlg progress;
	progress.m_GitCmd=_T("git.exe svn dcommit");
	if(progress.DoModal()==IDOK)
		return TRUE;

}

BOOL CAppUtils::Merge(CString *commit, int mode)
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
			msg+=_T("-m \"")+dlg.m_strLogMesage+_T("\"");
		}
		cmd.Format(_T("git.exe merge %s %s %s %s %s"),
			msg,
			noff,
			squash,
			nocommit,
			dlg.m_VersionName);

		CProgressDlg Prodlg;
		Prodlg.m_GitCmd = cmd;

		Prodlg.DoModal();

		return !Prodlg.m_GitStatus;
	}
	return false;
}