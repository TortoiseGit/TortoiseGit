
#pragma once

#include "GitBlameLogList.h"
#include "ProjectProperties.h"
/////////////////////////////////////////////////////////////////////////////
// COutputList window

class COutputList : public CListBox
{
// Construction
public:
	COutputList();

// Implementation
public:
	virtual ~COutputList();

protected:
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnEditCopy();
	afx_msg void OnEditClear();
	afx_msg void OnViewOutput();

	DECLARE_MESSAGE_MAP()
};

class COutputWnd;

class CGitMFCTabCtrl: public CMFCTabCtrl
{
protected:
	DECLARE_MESSAGE_MAP()
	afx_msg void OnLvnItemchangedLoglist(NMHDR *pNMHDR, LRESULT *pResult);

	DECLARE_DYNCREATE(CGitMFCTabCtrl);
};

class COutputWnd : public CDockablePane
{
	DECLARE_DYNAMIC(COutputWnd)
// Construction
public:
	COutputWnd();

// Attributes
public:
	CFont m_Font;

	CGitMFCTabCtrl	m_wndTabs;

	CGitBlameLogList m_LogList;
//	COutputList m_wndOutputBuild;
//	COutputList m_wndOutputDebug;
//	COutputList m_wndOutputFind;

protected:
	void FillBuildWindow();
	void FillDebugWindow();
	void FillFindWindow();

	void AdjustHorzScroll(CListBox& wndListBox);

private:
	ProjectProperties	m_ProjectProperties;

// Implementation
public:
	virtual ~COutputWnd();
	afx_msg void OnLvnItemchangedLoglist(NMHDR *pNMHDR, LRESULT *pResult);
	int	LoadHistory(CString filename);

protected:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);

	DECLARE_MESSAGE_MAP()
};

