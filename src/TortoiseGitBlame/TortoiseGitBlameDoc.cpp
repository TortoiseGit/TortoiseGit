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


// TortoiseGitBlameDoc.cpp : implementation of the CTortoiseGitBlameDoc class
//

#include "stdafx.h"
#include "TortoiseGitBlame.h"

#include "TortoiseGitBlameDoc.h"
#include "GitAdminDir.h"
#include "MessageBox.h"
#include "Git.h"
#include "MainFrm.h"
#include "TGitPath.h"
#include "TortoiseGitBlameView.h"
#include "CmdLineParser.h"
#include "CommonAppUtils.h"
#include "BlameDetectMovedOrCopiedLines.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CTortoiseGitBlameDoc

IMPLEMENT_DYNCREATE(CTortoiseGitBlameDoc, CDocument)

BEGIN_MESSAGE_MAP(CTortoiseGitBlameDoc, CDocument)
END_MESSAGE_MAP()


// CTortoiseGitBlameDoc construction/destruction

CTortoiseGitBlameDoc::CTortoiseGitBlameDoc()
{
	m_bFirstStartup = true;
	m_IsGitFile = FALSE;
	m_lLine = 1;
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
		m_Rev=parser.GetVal(_T("rev"));
		m_lLine = (int)parser.GetLongVal(_T("line"));
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
		Rev = _T("HEAD");

	// enable blame for files which do not exist in current working tree
	if (!PathFileExists(lpszPathName) && Rev != _T("HEAD"))
	{
		if (!CDocument::OnOpenDocument(GetTempFile()))
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
		CCommonAppUtils::RunTortoiseGitProc(_T(" /command:settings"));
		return FALSE;
	}
	CString topdir;
	if (!GitAdminDir::HasAdminDir(m_CurrentFileName, &topdir))
	{
		CString temp;
		temp.Format(IDS_CANNOTBLAMENOGIT, (LPCTSTR)m_CurrentFileName);
		MessageBox(nullptr, temp, _T("TortoiseGitBlame"), MB_OK | MB_ICONERROR);
		return FALSE;
	}
	else
	{
		m_IsGitFile=TRUE;
		sOrigCWD = g_Git.m_CurrentDir = topdir;

		CString PathName = m_CurrentFileName;
		if(topdir[topdir.GetLength()-1] == _T('\\') ||
			topdir[topdir.GetLength()-1] == _T('/'))
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
			g_Git.GetConfigValue(_T("doesnot.exist"));
		}
		catch (char * libgiterr)
		{
			MessageBox(nullptr, CString(libgiterr), _T("TortoiseGitBlame"), MB_ICONERROR);
			return FALSE;
		}

		CString cmd, option;
		int dwDetectMovedOrCopiedLines = theApp.GetInt(_T("DetectMovedOrCopiedLines"), BLAME_DETECT_MOVED_OR_COPIED_LINES_DISABLED);
		int dwDetectMovedOrCopiedLinesNumCharactersWithinFile = theApp.GetInt(_T("DetectMovedOrCopiedLinesNumCharactersWithinFile"), BLAME_DETECT_MOVED_OR_COPIED_LINES_NUM_CHARACTERS_WITHIN_FILE_DEFAULT);
		int dwDetectMovedOrCopiedLinesNumCharactersFromFiles = theApp.GetInt(_T("DetectMovedOrCopiedLinesNumCharactersFromFiles"), BLAME_DETECT_MOVED_OR_COPIED_LINES_NUM_CHARACTERS_FROM_FILES_DEFAULT);
		switch(dwDetectMovedOrCopiedLines)
		{
		default:
		case BLAME_DETECT_MOVED_OR_COPIED_LINES_DISABLED:
			option.Empty();
			break;
		case BLAME_DETECT_MOVED_OR_COPIED_LINES_WITHIN_FILE:
			option.Format(_T("-M%d"), dwDetectMovedOrCopiedLinesNumCharactersWithinFile);
			break;
		case BLAME_DETECT_MOVED_OR_COPIED_LINES_FROM_MODIFIED_FILES:
			option.Format(_T("-C%d"), dwDetectMovedOrCopiedLinesNumCharactersFromFiles);
			break;
		case BLAME_DETECT_MOVED_OR_COPIED_LINES_FROM_EXISTING_FILES_AT_FILE_CREATION:
			option.Format(_T("-C -C%d"), dwDetectMovedOrCopiedLinesNumCharactersFromFiles);
			break;
		case BLAME_DETECT_MOVED_OR_COPIED_LINES_FROM_EXISTING_FILES:
			option.Format(_T("-C -C -C%d"), dwDetectMovedOrCopiedLinesNumCharactersFromFiles);
			break;
		}

		if (theApp.GetInt(_T("IgnoreWhitespace"), 0) == 1)
			option += _T(" -w");

		cmd.Format(_T("git.exe blame -p %s %s -- \"%s\""), (LPCTSTR)option, (LPCTSTR)Rev, (LPCTSTR)path.GetGitPathString());
		m_BlameData.clear();
		BYTE_VECTOR err;
		if(g_Git.Run(cmd, &m_BlameData, &err))
		{
			CString str;
			if (!m_BlameData.empty())
				CGit::StringAppend(&str, &m_BlameData[0], CP_UTF8);
			if (!err.empty())
				CGit::StringAppend(&str, &err[0], CP_UTF8);
			MessageBox(nullptr, CString(MAKEINTRESOURCE(IDS_BLAMEERROR)) + _T("\n\n") + str, _T("TortoiseGitBlame"), MB_OK | MB_ICONERROR);

			return FALSE;
		}

#ifdef USE_TEMPFILENAME
		if(!m_TempFileName.IsEmpty())
		{
			::DeleteFile(m_TempFileName);
			m_TempFileName.Empty();
		}

		m_TempFileName=GetTempFile();

		cmd.Format(_T("git.exe cat-file blob %s:\"%s\""), (LPCTSTR)Rev, (LPCTSTR)path.GetGitPathString());

		if(g_Git.RunLogFile(cmd, m_TempFileName))
		{
			CString str;
			str.Format(IDS_CHECKOUTFAILED, (LPCTSTR)path.GetGitPathString());
			MessageBox(nullptr, CString(MAKEINTRESOURCE(IDS_BLAMEERROR)) + _T("\n\n") + str, _T("TortoiseGitBlame"), MB_OK | MB_ICONERROR);
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

		BOOL bShowCompleteLog = (theApp.GetInt(_T("ShowCompleteLog"), 1) == 1);
		if (bShowCompleteLog && BlameIsLimitedToOneFilename(dwDetectMovedOrCopiedLines))
		{
			if (GetMainFrame()->m_wndOutput.LoadHistory(path.GetGitPathString(), m_Rev, (theApp.GetInt(_T("FollowRenames"), 0) == 1)))
				return FALSE;
		}
		else
		{
			std::set<CGitHash> hashes;
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
	CDocument::SetPathName(lpszPathName, bAddToMRU && (m_Rev == _T("HEAD")));

	this->SetTitle(CString(lpszPathName) + _T(":") + m_Rev);
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
