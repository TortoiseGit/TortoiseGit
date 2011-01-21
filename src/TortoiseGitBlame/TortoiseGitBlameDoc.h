
// TortoiseGitBlameDoc.h : interface of the CTortoiseGitBlameDoc class
//


#pragma once

#include "Git.h"
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
	CString m_TempFileName;
// Operations
public:
	BOOL m_IsGitFile;
	CTGitPath m_GitPath;
// Overrides
public:
	virtual BOOL OnNewDocument();
	virtual void Serialize(CArchive& ar);
	virtual BOOL OnOpenDocument(LPCTSTR lpszPathName);
	virtual BOOL OnOpenDocument(LPCTSTR lpszPathName,CString Rev);
	virtual void SetPathName(LPCTSTR lpszPathName, BOOL bAddToMRU = TRUE);
	
	
// Implementation
public:
	virtual ~CTortoiseGitBlameDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif
	CMainFrame *GetMainFrame()
	{
		return (CMainFrame*)AfxGetApp()->GetMainWnd();
	}

protected:
	
	CString m_Rev;
// Generated message map functions
protected:
	DECLARE_MESSAGE_MAP()
};


