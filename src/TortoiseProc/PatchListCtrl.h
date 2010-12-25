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

	enum
	{
		STATUS_NONE,
		STATUS_APPLYING,
		STATUS_APPLY_RETRY,
		STATUS_APPLY_FAIL,
		STATUS_APPLY_SUCCESS,
		STATUS_APPLY_SKIP,
	};

	DWORD GetMenuMask(int x){return 1<<x;}

	HFONT				m_boldFont;

protected:
	DECLARE_MESSAGE_MAP()
	
public:
	afx_msg void OnNMDblclk(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnContextMenu(CWnd* /*pWnd*/, CPoint /*point*/);
	int LaunchProc(CString& cmd);
	afx_msg void OnNMCustomdraw(NMHDR *pNMHDR, LRESULT *pResult);
};


