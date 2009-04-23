#pragma once


// CPatchListCtrl

class CPatchListCtrl : public CListCtrl
{
	DECLARE_DYNAMIC(CPatchListCtrl)

public:
	CPatchListCtrl();
	virtual ~CPatchListCtrl();
	DWORD m_ContextMenuMask;
	enum
	{
		MENU_SENDMAIL=1,
		MENU_VIEWPATCH,
		MENU_VIEWWITHMERGE,
		MENU_APPLY
	};
	DWORD GetMenuMask(int x){return 1<<x;}

protected:
	DECLARE_MESSAGE_MAP()
	
public:
	afx_msg void OnNMDblclk(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnContextMenu(CWnd* /*pWnd*/, CPoint /*point*/);
	int LaunchProc(CString& cmd);
};


