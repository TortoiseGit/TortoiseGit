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

CAppUtils::CAppUtils(void)
{
}

CAppUtils::~CAppUtils(void)
{
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

	if (CreateProcess(NULL, const_cast<TCHAR*>((LPCTSTR)cleanCommandLine), NULL, NULL, FALSE, 0, 0, sOrigCWD, &startup, &process)==0)
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

void CAppUtils::ResizeAllListCtrlCols(CListCtrl * pListCtrl)
{
	int maxcol = ((CHeaderCtrl*)(pListCtrl->GetDlgItem(0)))->GetItemCount()-1;
	int nItemCount = pListCtrl->GetItemCount();
	TCHAR textbuf[MAX_PATH];
	CHeaderCtrl * pHdrCtrl = (CHeaderCtrl*)(pListCtrl->GetDlgItem(0));
	if (pHdrCtrl)
	{
		for (int col = 0; col <= maxcol; col++)
		{
			HDITEM hdi = {0};
			hdi.mask = HDI_TEXT;
			hdi.pszText = textbuf;
			hdi.cchTextMax = sizeof(textbuf);
			pHdrCtrl->GetItem(col, &hdi);
			int cx = pListCtrl->GetStringWidth(hdi.pszText)+20; // 20 pixels for col separator and margin
			for (int index = 0; index<nItemCount; ++index)
			{
				// get the width of the string and add 14 pixels for the column separator and margins
				int linewidth = pListCtrl->GetStringWidth(pListCtrl->GetItemText(index, col)) + 14;
				if (index == 0)
				{
					// add the image size
					CImageList * pImgList = pListCtrl->GetImageList(LVSIL_SMALL);
					if ((pImgList)&&(pImgList->GetImageCount()))
					{
						IMAGEINFO imginfo;
						pImgList->GetImageInfo(0, &imginfo);
						linewidth += (imginfo.rcImage.right - imginfo.rcImage.left);
						linewidth += 3;	// 3 pixels between icon and text
					}
				}
				if (cx < linewidth)
					cx = linewidth;
			}
			pListCtrl->SetColumnWidth(col, cx);

		}
	}
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
		cmd.Format(_T("git.exe diff --stat -p %s"),rev2);
	}else
	{	
		cmd.Format(_T("git.exe diff-tree -r -p --stat %s %s"),rev1,rev2);
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
			track=_T("--track");

		if(dlg.m_bForce)
			force=_T("-f");

		if(IsTag)
		{
			cmd.Format(_T("git.exe tag %s %s %s %s"),
				track,
				force,
				dlg.m_BranchTagName,
				dlg.m_VersionName
				);

	
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
		return TRUE;
		
	}
	return FALSE;
}

bool CAppUtils::Switch(CString *CommitHash)
{
	CGitSwitchDlg dlg;
	if(CommitHash)
		dlg.m_Base=*CommitHash;
	
	if (dlg.DoModal() == IDOK)
	{
		CString cmd;
		CString track;
		CString base;
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

bool CAppUtils::IgnoreFile(CTGitPath &path,bool IsMask)
{
	CString ignorefile;
	ignorefile=g_Git.m_CurrentDir;
	ignorefile+=path.GetDirectory().GetWinPathString()+_T("\\.gitignore");

	CStdioFile file;
	if(!file.Open(ignorefile,CFile::modeCreate|CFile::modeWrite))
	{
		CMessageBox::Show(NULL,ignorefile+_T(" Open Failure"),_T("TortoiseGit"),MB_OK);
		return FALSE;
	}

	CString ignorelist;
	file.ReadString(ignorelist);

	if(IsMask)
	{
		ignorelist+=_T("\n*.")+path.GetFileExtension();
	}else
	{
		ignorelist+=_T("\n")+path.GetBaseFilename();
	}
	file.WriteString(ignorelist);

	file.Close();
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

bool CAppUtils::ConflictEdit(CTGitPath &path,bool bAlternativeTool)
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

	CString format;
	format=g_Git.m_CurrentDir+_T("\\")+directory.GetWinPathString()+merge.GetFilename()+CString(_T(".%s."))+temp+merge.GetFileExtension();

	CString file;
	file.Format(format,_T("LOCAL"));
	mine.SetFromGit(file);
	file.Format(format,_T("REMOTE"));
	theirs.SetFromGit(file);
	file.Format(format,_T("BASE"));
	base.SetFromGit(file);

	
	format=_T("git.exe cat-file blob \":%d:%s\"");
	for(int i=0;i<list.GetCount();i++)
	{
		CString cmd;
		CString outfile;
		cmd.Format(format,list[i].m_Stage,list[i].GetGitPathString());

		if( list[i].m_Stage == 1)
		{
			outfile=base.GetWinPathString();
		}
		if( list[i].m_Stage == 2 )
		{
			outfile=mine.GetWinPathString();
		}
		if( list[i].m_Stage == 3 )
		{
			outfile=theirs.GetWinPathString();
		}
		g_Git.RunLogFile(cmd,outfile);
	}

	merge.SetFromWin(g_Git.m_CurrentDir+_T("\\")+merge.GetWinPathString());
	bRet = !!CAppUtils::StartExtMerge(base, theirs, mine, merge,_T("BASE"),_T("REMOTE"),_T("LOCAL"));

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