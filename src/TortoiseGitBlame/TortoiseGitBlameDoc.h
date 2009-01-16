
// TortoiseGitBlameDoc.h : interface of the CTortoiseGitBlameDoc class
//


#pragma once

class CMainFrame ;

class CTortoiseGitBlameDoc : public CDocument
{
protected: // create from serialization only
	CTortoiseGitBlameDoc();
	DECLARE_DYNCREATE(CTortoiseGitBlameDoc)

// Attributes
public:
	CString m_BlameData;
	CString m_CurrentFileName;
// Operations
public:
	BOOL m_IsGitFile;
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


