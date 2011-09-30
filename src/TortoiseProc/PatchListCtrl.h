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
		STATUS_APPLYING = 0x10000,
		STATUS_APPLY_RETRY = 0x1,
		STATUS_APPLY_FAIL = 0x2,
		STATUS_APPLY_SUCCESS =0x4,
		STATUS_APPLY_SKIP=0x8,
		STATUS_MASK = 0xFFFF,
	};

	DWORD GetMenuMask(int x){return 1<<x;}

	HFONT				m_boldFont;

protected:
	DECLARE_MESSAGE_MAP()

public:
	afx_msg void OnNMDblclk(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnContextMenu(CWnd* /*pWnd*/, CPoint /*point*/);
	int LaunchProc(const CString& cmd);
	afx_msg void OnNMCustomdraw(NMHDR *pNMHDR, LRESULT *pResult);
};


