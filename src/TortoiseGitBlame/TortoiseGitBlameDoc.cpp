// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2017, 2019 - TortoiseGit

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


// TortoiseGitBlameDoc.cpp : implementation of the CTortoiseGitBlameDoc class
//

#include "stdafx.h"
#include "TortoiseGitBlame.h"

#include "TortoiseGitBlameDoc.h"
#include "GitAdminDir.h"
#include "Git.h"
#include "MainFrm.h"
#include "TGitPath.h"
#include "TortoiseGitBlameView.h"
#include "CmdLineParser.h"
#include "CommonAppUtils.h"
#include "BlameDetectMovedOrCopiedLines.h"
#include "TempFile.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CTortoiseGitBlameDoc

IMPLEMENT_DYNCREATE(CTortoiseGitBlameDoc, CDocument)

BEGIN_MESSAGE_MAP(CTortoiseGitBlameDoc, CDocument)
END_MESSAGE_MAP()

typedef CComCritSecLock<CComCriticalSection> CAutoLocker;

// CTortoiseGitBlameDoc construction/destruction

CTortoiseGitBlameDoc::CTortoiseGitBlameDoc()
: m_bFirstStartup(true)
, m_IsGitFile(FALSE)
, m_lLine(1)
{
}

CTortoiseGitBlameDoc::~CTortoiseGitBlameDoc()
{
}

BOOL CTortoiseGitBlameDoc::OnNewDocument()
{
	return TRUE;
}
BOOL CTortoiseGitBlameDoc::OnOpenDocument(LPCTSTR lpszPathName)
{
	CCmdLineParser parser(AfxGetApp()->m_lpCmdLine);
	if (m_bFirstStartup)
	{
		m_Rev = parser.GetVal(L"rev");
		m_lLine = static_cast<int>(parser.GetLongVal(L"line"));
		m_bFirstStartup = false;
	}
	else
	{
		m_Rev.Empty();
		m_lLine = 1;
	}

	return OnOpenDocument(lpszPathName,m_Rev);
}

BOOL CTortoiseGitBlameDoc::OnOpenDocument(LPCTSTR lpszPathName,CString Rev)
{
	if(Rev.IsEmpty())
		Rev = L"HEAD";

	// enable blame for files which do not exist in current working tree
	if (!PathFileExists(lpszPathName) && Rev != L"HEAD")
	{
		if (!CDocument::OnOpenDocument(CTempFiles::Instance().GetTempFilePath(true).GetWinPathString()))
			return FALSE;
	}
	else
	{
		if (!CDocument::OnOpenDocument(lpszPathName))
			return FALSE;
	}

	m_CurrentFileName = lpszPathName;

	m_Rev=Rev;

	// (SDI documents will reuse this document)
	if(!g_Git.CheckMsysGitDir())
	{
		CCommonAppUtils::RunTortoiseGitProc(L" /command:settings");
		return FALSE;
	}
	CString topdir;
	if (!GitAdminDir::HasAdminDir(m_CurrentFileName, &topdir))
	{
		CString temp;
		temp.Format(IDS_CANNOTBLAMENOGIT, static_cast<LPCTSTR>(m_CurrentFileName));
		MessageBox(nullptr, temp, L"TortoiseGitBlame", MB_OK | MB_ICONERROR);
		return FALSE;
	}
	else
	{
		m_IsGitFile=TRUE;
		sOrigCWD = g_Git.m_CurrentDir = topdir;

		CString PathName = m_CurrentFileName;
		if (topdir[topdir.GetLength() - 1] == L'\\' || topdir[topdir.GetLength() - 1] == L'/')
			PathName=PathName.Right(PathName.GetLength()-g_Git.m_CurrentDir.GetLength());
		else
			PathName=PathName.Right(PathName.GetLength()-g_Git.m_CurrentDir.GetLength()-1);

		CTGitPath path;
		path.SetFromWin(PathName);

		if(!g_Git.m_CurrentDir.IsEmpty())
			SetCurrentDirectory(g_Git.m_CurrentDir);

		try
		{
			// make sure all config files are read in order to check that none contains an error
			g_Git.GetConfigValue(L"doesnot.exist");

			// make sure git_init() works and that .git-dir is ok
			CAutoLocker lock(g_Git.m_critGitDllSec);
			g_Git.CheckAndInitDll();
		}
		catch (char * libgiterr)
		{
			MessageBox(nullptr, CString(libgiterr), L"TortoiseGitBlame", MB_ICONERROR);
			return FALSE;
		}

		CString cmd, option;
		int dwDetectMovedOrCopiedLines = theApp.GetInt(L"DetectMovedOrCopiedLines", BLAME_DETECT_MOVED_OR_COPIED_LINES_DISABLED);
		int dwDetectMovedOrCopiedLinesNumCharactersWithinFile = theApp.GetInt(L"DetectMovedOrCopiedLinesNumCharactersWithinFile", BLAME_DETECT_MOVED_OR_COPIED_LINES_NUM_CHARACTERS_WITHIN_FILE_DEFAULT);
		int dwDetectMovedOrCopiedLinesNumCharactersFromFiles = theApp.GetInt(L"DetectMovedOrCopiedLinesNumCharactersFromFiles", BLAME_DETECT_MOVED_OR_COPIED_LINES_NUM_CHARACTERS_FROM_FILES_DEFAULT);
		switch(dwDetectMovedOrCopiedLines)
		{
		default:
		case BLAME_DETECT_MOVED_OR_COPIED_LINES_DISABLED:
			option.Empty();
			break;
		case BLAME_DETECT_MOVED_OR_COPIED_LINES_WITHIN_FILE:
			option.Format(L"-M%d", dwDetectMovedOrCopiedLinesNumCharactersWithinFile);
			break;
		case BLAME_DETECT_MOVED_OR_COPIED_LINES_FROM_MODIFIED_FILES:
			option.Format(L"-C%d", dwDetectMovedOrCopiedLinesNumCharactersFromFiles);
			break;
		case BLAME_DETECT_MOVED_OR_COPIED_LINES_FROM_EXISTING_FILES_AT_FILE_CREATION:
			option.Format(L"-C -C%d", dwDetectMovedOrCopiedLinesNumCharactersFromFiles);
			break;
		case BLAME_DETECT_MOVED_OR_COPIED_LINES_FROM_EXISTING_FILES:
			option.Format(L"-C -C -C%d", dwDetectMovedOrCopiedLinesNumCharactersFromFiles);
			break;
		}

		if (theApp.GetInt(L"IgnoreWhitespace", 0) == 1)
			option += L" -w";

		bool onlyFirstParent = theApp.GetInt(L"OnlyFirstParent", 0) == 1;
		if (onlyFirstParent)
		{
			CString tmpfile = CTempFiles::Instance().GetTempFilePath(true).GetWinPathString();
			cmd.Format(L"git rev-list --first-parent %s", static_cast<LPCTSTR>(Rev));
			CString err;
			CAutoFILE file = _wfsopen(tmpfile, L"wb", SH_DENYWR);
			if (!file)
			{
				MessageBox(nullptr, CString(MAKEINTRESOURCE(IDS_BLAMEERROR)) + L"\n\nCould not create temp file!", L"TortoiseGitBlame", MB_OK | MB_ICONERROR);
				return FALSE;
			}

			CStringA lastline;
			if (g_Git.Run(cmd, [&](const CStringA& line)
			{
				fwrite(lastline + ' ' + line + '\n', sizeof(char), lastline.GetLength() + 1 + line.GetLength() + 1, file);
				lastline = line;
			}
			, &err))
			{
				MessageBox(nullptr, CString(MAKEINTRESOURCE(IDS_BLAMEERROR)) + L"\n\n" + err, L"TortoiseGitBlame", MB_OK | MB_ICONERROR);
				return FALSE;
			}
			option.AppendFormat(L" -S \"%s\"", static_cast<LPCTSTR>(tmpfile));
		}

		cmd.Format(L"git.exe blame -p %s %s -- \"%s\"", static_cast<LPCTSTR>(option), static_cast<LPCTSTR>(Rev), static_cast<LPCTSTR>(path.GetGitPathString()));
		m_BlameData.clear();
		BYTE_VECTOR err;
		if(g_Git.Run(cmd, &m_BlameData, &err))
		{
			CString str;
			if (!m_BlameData.empty())
				CGit::StringAppend(&str, m_BlameData.data(), CP_UTF8, static_cast<int>(m_BlameData.size()));
			if (!err.empty())
				CGit::StringAppend(&str, err.data(), CP_UTF8, static_cast<int>(err.size()));
			MessageBox(nullptr, CString(MAKEINTRESOURCE(IDS_BLAMEERROR)) + L"\n\n" + str, L"TortoiseGitBlame", MB_OK | MB_ICONERROR);

			return FALSE;
		}

#ifdef USE_TEMPFILENAME
		m_TempFileName = CTempFiles::Instance().GetTempFilePath(true).GetWinPathString();

		cmd.Format(L"git.exe cat-file blob %s:\"%s\"", static_cast<LPCTSTR>(Rev), static_cast<LPCTSTR>(path.GetGitPathString()));

		if(g_Git.RunLogFile(cmd, m_TempFileName))
		{
			CString str;
			str.Format(IDS_CHECKOUTFAILED, static_cast<LPCTSTR>(path.GetGitPathString()));
			MessageBox(nullptr, CString(MAKEINTRESOURCE(IDS_BLAMEERROR)) + L"\n\n" + str, L"TortoiseGitBlame", MB_OK | MB_ICONERROR);
			return FALSE;
		}
#endif
		m_GitPath = path;

		CTortoiseGitBlameView *pView=DYNAMIC_DOWNCAST(CTortoiseGitBlameView,GetMainFrame()->GetActiveView());
		if (!pView)
		{
			CWnd* pWnd = GetMainFrame()->GetDescendantWindow(AFX_IDW_PANE_FIRST, TRUE);
			if (pWnd && pWnd->IsKindOf(RUNTIME_CLASS(CTortoiseGitBlameView)))
				pView = static_cast<CTortoiseGitBlameView*>(pWnd);
			else
				return FALSE;
		}
		pView->ParseBlame();

		BOOL bShowCompleteLog = (theApp.GetInt(L"ShowCompleteLog", 1) == 1);
		if (bShowCompleteLog && BlameIsLimitedToOneFilename(dwDetectMovedOrCopiedLines) && !onlyFirstParent)
		{
			if (GetMainFrame()->m_wndOutput.LoadHistory(path.GetGitPathString(), m_Rev, (theApp.GetInt(L"FollowRenames", 0) == 1)))
				return FALSE;
		}
		else
		{
			std::unordered_set<CGitHash> hashes;
			pView->m_data.GetHashes(hashes);
			if (GetMainFrame()->m_wndOutput.LoadHistory(hashes))
				return FALSE;
		}

		pView->MapLineToLogIndex();
		pView->UpdateInfo();
		if (m_lLine > 0)
			pView->GotoLine(m_lLine);

		SetPathName(m_CurrentFileName, FALSE);
	}

	return TRUE;
}

void CTortoiseGitBlameDoc::SetPathName(LPCTSTR lpszPathName, BOOL bAddToMRU)
{
	CDocument::SetPathName(lpszPathName, bAddToMRU && (m_Rev == L"HEAD"));

	this->SetTitle(CString(lpszPathName) + L':' + m_Rev);
}

// CTortoiseGitBlameDoc diagnostics

#ifdef _DEBUG
void CTortoiseGitBlameDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CTortoiseGitBlameDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG


// CTortoiseGitBlameDoc commands
