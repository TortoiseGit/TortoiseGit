
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
	// TODO: add one-time construction code here

}

CTortoiseGitBlameDoc::~CTortoiseGitBlameDoc()
{
}

BOOL CTortoiseGitBlameDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

	// TODO: add reinitialization code here
	// (SDI documents will reuse this document)

	return TRUE;
}

BOOL CTortoiseGitBlameDoc::OnOpenDocument(LPCTSTR lpszPathName)
{
	if (!CDocument::OnOpenDocument(lpszPathName))
		return FALSE;

	m_CurrentFileName=lpszPathName;

	// TODO: add reinitialization code here
	// (SDI documents will reuse this document)
	if(!CGit::CheckMsysGitDir())
	{
		CMessageBox::Show(NULL,_T("MsysGit have not install or config fail"),_T("TortoiseGitBlame"),MB_OK);
		return FALSE;
	}

	GitAdminDir admindir;
	CString topdir;
	if(!admindir.HasAdminDir(lpszPathName,&topdir))
	{
		CMessageBox::Show(NULL,CString(lpszPathName)+_T(" is not controled by git\nJust Show File Context"),_T("TortoiseGitBlame"),MB_OK);
	}else
	{
		m_IsGitFile=TRUE;
		g_Git.m_CurrentDir=topdir;
		GetMainFrame()->m_wndOutput.LoadHistory(lpszPathName);
		
		CString cmd;
		CTGitPath path;
		path.SetFromWin(lpszPathName);
		cmd.Format(_T("git.exe blame -s -l -- \"%s\""),path.GetGitPathString());
		m_BlameData.Empty();
		if(g_Git.Run(cmd,&m_BlameData))
		{
			CMessageBox::Show(NULL,CString(_T("Blame Error"))+m_BlameData,_T("TortoiseGitBlame"),MB_OK);

		}
		CTortoiseGitBlameView *pView=DYNAMIC_DOWNCAST(CTortoiseGitBlameView,GetMainFrame()->GetActiveView());
		pView->UpdateInfo();
		
	}

	return TRUE;
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
