// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2012 - TortoiseGit

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
	if(parser.HasVal(_T("rev")) && m_bFirstStartup)
	{
		m_Rev=parser.GetVal(_T("rev"));
		m_bFirstStartup = false;
	}
	else
	{
		m_Rev.Empty();
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
		CCommonAppUtils::RunTortoiseProc(_T(" /command:settings"));
		return FALSE;
	}
	GitAdminDir admindir;
	CString topdir;
	if(!admindir.HasAdminDir(m_CurrentFileName, &topdir))
	{
		CString temp;
		temp.Format(IDS_CANNOTBLAMENOGIT, CString(m_CurrentFileName));
		CMessageBox::Show(NULL, temp, _T("TortoiseGitBlame"), MB_OK);
	}
	else
	{
		GitAdminDir lastAdminDir;
		CString oldTopDir;
		if (topdir != g_Git.m_CurrentDir && CTGitPath(g_Git.m_CurrentDir).HasAdminDir(&oldTopDir) && oldTopDir != topdir)
		{
			CString sMsg;
			sMsg.Format(IDS_ERR_DIFFENERTPREPO, oldTopDir, topdir);
			MessageBox(NULL, sMsg, _T("TortoiseGitBlame"), MB_OK | MB_ICONERROR);
			return FALSE;
		}

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

		m_GitPath = path;
		if (GetMainFrame()->m_wndOutput.LoadHistory(path.GetGitPathString(), m_Rev, (theApp.GetInt(_T("FollowRenames"), 0) == 1)))
			return FALSE;

		CString cmd;

		cmd.Format(_T("git.exe blame -s -l %s -- \"%s\""),Rev,path.GetGitPathString());
		m_BlameData.clear();
		BYTE_VECTOR err;
		if(g_Git.Run(cmd, &m_BlameData, &err))
		{
			CString str;
			if (m_BlameData.size() > 0)
				g_Git.StringAppend(&str, &m_BlameData[0], CP_UTF8);
			if (err.size() > 0)
				g_Git.StringAppend(&str, &err[0], CP_UTF8);
			MessageBox(NULL, CString(MAKEINTRESOURCE(IDS_BLAMEERROR)) + _T("\n\n") + str, _T("TortoiseGitBlame"), MB_OK);

			return FALSE;
		}

		if(!m_TempFileName.IsEmpty())
		{
			::DeleteFile(m_TempFileName);
			m_TempFileName.Empty();
		}

		m_TempFileName=GetTempFile();

		if(Rev.IsEmpty())
			Rev=_T("HEAD");

		cmd.Format(_T("git.exe cat-file blob %s:\"%s\""),Rev,path.GetGitPathString());

		if(g_Git.RunLogFile(cmd, m_TempFileName))
		{
			CString str;
			str.Format(IDS_CHECKOUTFAILED, path.GetGitPathString());
			MessageBox(NULL, CString(MAKEINTRESOURCE(IDS_BLAMEERROR)) + _T("\n\n") + str, _T("TortoiseGitBlame"), MB_OK);
		}

		CTortoiseGitBlameView *pView=DYNAMIC_DOWNCAST(CTortoiseGitBlameView,GetMainFrame()->GetActiveView());
		if(pView == NULL)
		{
			CWnd* pWnd = GetMainFrame()->GetDescendantWindow(AFX_IDW_PANE_FIRST, TRUE);
			if (pWnd != NULL && pWnd->IsKindOf(RUNTIME_CLASS(CTortoiseGitBlameView)))
			{
				pView = (CTortoiseGitBlameView*)pWnd;
			}
			else
			{
				return FALSE;
			}
		}
		pView->UpdateInfo();
	}

	return TRUE;
}

void CTortoiseGitBlameDoc::SetPathName(LPCTSTR lpszPathName, BOOL bAddToMRU)
{
	CDocument::SetPathName(lpszPathName,bAddToMRU);

	CString title;
	if(m_Rev.IsEmpty())
		title=CString(lpszPathName)+_T(":HEAD");
	else
		title=CString(lpszPathName)+_T(":")+m_Rev;

	this->SetTitle(title);
}

// CTortoiseGitBlameDoc serialization

void CTortoiseGitBlameDoc::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
		// TODO: add storing code here
	}
	else
	{
		// TODO: add loading code here
	}
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
