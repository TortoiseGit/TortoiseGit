// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2011, 2013, 2016-2017 - TortoiseGit

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

// TortoiseGitBlameDoc.h : interface of the CTortoiseGitBlameDoc class
//


#pragma once
#include "TGitPath.h"

class CMainFrame ;

class CTortoiseGitBlameDoc : public CDocument
{
protected: // create from serialization only
	CTortoiseGitBlameDoc();
	DECLARE_DYNCREATE(CTortoiseGitBlameDoc)
	bool m_bFirstStartup;

// Attributes
public:
	BYTE_VECTOR m_BlameData;
	CString m_CurrentFileName;
#ifdef USE_TEMPFILENAME
	CString m_TempFileName;
#endif
	CString m_Rev;
	int		m_lLine;

// Operations
	BOOL m_IsGitFile;
	CTGitPath m_GitPath;

// Overrides
	virtual BOOL OnNewDocument() override;
	virtual BOOL OnOpenDocument(LPCTSTR lpszPathName) override;
	BOOL OnOpenDocument(LPCTSTR lpszPathName, CString Rev);
	virtual void SetPathName(LPCTSTR lpszPathName, BOOL bAddToMRU = TRUE) override;

// Implementation
	virtual ~CTortoiseGitBlameDoc();
#ifdef _DEBUG
	virtual void AssertValid() const override;
	virtual void Dump(CDumpContext& dc) const override;
#endif
	CMainFrame *GetMainFrame()
	{
		return reinterpret_cast<CMainFrame*>(AfxGetApp()->GetMainWnd());
	}

protected:
// Generated message map functions
	DECLARE_MESSAGE_MAP()
};


