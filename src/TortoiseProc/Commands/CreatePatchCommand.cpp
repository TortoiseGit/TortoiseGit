// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007-2008 - TortoiseSVN

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
#include "CreatePatchCommand.h"

#include "PathUtils.h"
#include "StringUtils.h"
#include "AppUtils.h"
#include "CreatePatchDlg.h"
#include "SVN.h"
#include "TempFile.h"
#include "ProgressDlg.h"

#define PATCH_TO_CLIPBOARD_PSEUDO_FILENAME		_T(".TSVNPatchToClipboard")


bool CreatePatchCommand::Execute()
{
	bool bRet = false;
	CString savepath = CPathUtils::GetLongPathname(parser.GetVal(_T("savepath")));
	CCreatePatch dlg;
	dlg.m_pathList = pathList;
	if (dlg.DoModal()==IDOK)
	{
		if (cmdLinePath.IsEmpty())
		{
			cmdLinePath = pathList.GetCommonRoot();
		}
		bRet = CreatePatch(cmdLinePath.GetDirectory(), dlg.m_pathList, CTSVNPath(savepath));
		SVN svn;
		svn.Revert(dlg.m_filesToRevert, CStringArray(), false);
	}
	return bRet;
}

UINT_PTR CALLBACK CreatePatchCommand::CreatePatchFileOpenHook(HWND hDlg, UINT uiMsg, WPARAM wParam, LPARAM /*lParam*/)
{
	if(uiMsg ==	WM_COMMAND && LOWORD(wParam) == IDC_PATCH_TO_CLIPBOARD)
	{
		HWND hFileDialog = GetParent(hDlg);

		CString strFilename = CTempFiles::Instance().GetTempFilePath(false).GetWinPathString() + PATCH_TO_CLIPBOARD_PSEUDO_FILENAME;

		CommDlg_OpenSave_SetControlText(hFileDialog, edt1, (LPCTSTR)strFilename);   

		PostMessage(hFileDialog, WM_COMMAND, MAKEWPARAM(IDOK, BM_CLICK), (LPARAM)(GetDlgItem(hDlg, IDOK)));
	}
	return 0;
}

bool CreatePatchCommand::CreatePatch(const CTSVNPath& root, const CTSVNPathList& path, const CTSVNPath& cmdLineSavePath)
{
	OPENFILENAME ofn = {0};				// common dialog box structure
	CString temp;
	CTSVNPath savePath;

	if (cmdLineSavePath.IsEmpty())
	{
		TCHAR szFile[MAX_PATH] = {0};  // buffer for file name
		// Initialize OPENFILENAME
		ofn.lStructSize = sizeof(OPENFILENAME);
		ofn.hwndOwner = hwndExplorer;
		ofn.lpstrFile = szFile;
		ofn.nMaxFile = sizeof(szFile)/sizeof(TCHAR);
		ofn.lpstrInitialDir = root.GetWinPath();

		temp.LoadString(IDS_REPOBROWSE_SAVEAS);
		CStringUtils::RemoveAccelerators(temp);
		if (temp.IsEmpty())
			ofn.lpstrTitle = NULL;
		else
			ofn.lpstrTitle = temp;
		ofn.Flags = OFN_OVERWRITEPROMPT | OFN_ENABLETEMPLATE | OFN_EXPLORER | OFN_ENABLEHOOK;

		ofn.hInstance = AfxGetResourceHandle();
		ofn.lpTemplateName = MAKEINTRESOURCE(IDD_PATCH_FILE_OPEN_CUSTOM);
		ofn.lpfnHook = CreatePatchFileOpenHook;

		CString sFilter;
		sFilter.LoadString(IDS_PATCHFILEFILTER);
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
		// Display the Open dialog box. 
		if (GetSaveFileName(&ofn)==FALSE)
		{
			delete [] pszFilters;
			return FALSE;
		}
		delete [] pszFilters;
		savePath = CTSVNPath(ofn.lpstrFile);
		if (ofn.nFilterIndex == 1)
		{
			if (savePath.GetFileExtension().IsEmpty())
				savePath.AppendRawString(_T(".patch"));
		}
	}
	else
	{
		savePath = cmdLineSavePath;
	}

	// This is horrible and I should be ashamed of myself, but basically, the 
	// the file-open dialog writes ".TSVNPatchToClipboard" to its file field if the user clicks
	// the "Save To Clipboard" button.
	bool bToClipboard = _tcsstr(savePath.GetWinPath(), PATCH_TO_CLIPBOARD_PSEUDO_FILENAME) != NULL;

	CProgressDlg progDlg;
	progDlg.SetTitle(IDS_PROC_PATCHTITLE);
	progDlg.SetShowProgressBar(false);
	progDlg.ShowModeless(CWnd::FromHandle(hwndExplorer));
	progDlg.FormatNonPathLine(1, IDS_PROC_SAVEPATCHTO);
	if(bToClipboard)
	{
		progDlg.FormatNonPathLine(2, IDS_CLIPBOARD_PROGRESS_DEST);
	}
	else
	{
		progDlg.SetLine(2, savePath.GetUIPathString(), true);
	}
	//progDlg.SetAnimation(IDR_ANIMATION);

	CTSVNPath tempPatchFilePath;
	if (bToClipboard)
		tempPatchFilePath = CTempFiles::Instance().GetTempFilePath(true);
	else
		tempPatchFilePath = savePath;

	::DeleteFile(tempPatchFilePath.GetWinPath());

	CTSVNPath sDir = root;
	if (sDir.IsEmpty())
		sDir = path.GetCommonRoot();

	SVN svn;
	for (int fileindex = 0; fileindex < path.GetCount(); ++fileindex)
	{
		svn_depth_t depth = path[fileindex].IsDirectory() ? svn_depth_empty : svn_depth_files;
		if (!svn.CreatePatch(path[fileindex], SVNRev::REV_BASE, path[fileindex], SVNRev::REV_WC, sDir.GetDirectory(), depth, FALSE, FALSE, FALSE, _T(""), true, tempPatchFilePath))
		{
			progDlg.Stop();
			::MessageBox(hwndExplorer, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
			return FALSE;
		}
	}

	if(bToClipboard)
	{
		// The user actually asked for the patch to be written to the clipboard
		CStringA sClipdata;
		FILE * inFile;
		_tfopen_s(&inFile, tempPatchFilePath.GetWinPath(), _T("rb"));
		if(inFile)
		{
			char chunkBuffer[16384];
			while(!feof(inFile))
			{
				size_t readLength = fread(chunkBuffer, 1, sizeof(chunkBuffer), inFile);
				sClipdata.Append(chunkBuffer, (int)readLength);
			}
			fclose(inFile);

			CStringUtils::WriteDiffToClipboard(sClipdata);
		}
	}
	else
		CAppUtils::StartUnifiedDiffViewer(tempPatchFilePath, tempPatchFilePath.GetFilename());

	progDlg.Stop();
	return TRUE;
}
