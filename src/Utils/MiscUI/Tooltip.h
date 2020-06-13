// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008, 2010-2011, 2020 - TortoiseSVN

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
#pragma once




/**
 * \ingroup Utils
 * Extends the MFC CToolTipCtrl with convenience methods for dialogs and
 * provides mechanism to use tooltips longer than 80 chars whithout having
 * to implement the TTN_NEEDTEXT handler in every dialog.
 */
class CToolTips : public CToolTipCtrl
{
// Construction
public:
	virtual BOOL Create(CWnd* pParentWnd, DWORD dwStyle = 0);
	CToolTips() : CToolTipCtrl(), m_pParentWnd(nullptr), m_themeCallbackId(0) {}
	virtual ~CToolTips() {}

	BOOL AddTool(CWnd* pWnd, UINT nIDText, LPCRECT lpRectTool = nullptr, UINT_PTR nIDTool = 0);
	BOOL AddTool(CWnd* pWnd, LPCTSTR lpszText = LPSTR_TEXTCALLBACK, LPCRECT lpRectTool = nullptr, UINT_PTR nIDTool = 0);
	void AddTool(int nIdWnd, UINT nIdText, LPCRECT lpRectTool = nullptr, UINT_PTR nIDTool = 0);
	void AddTool(int nIdWnd, CString sBalloonTipText, LPCRECT lpRectTool = nullptr, UINT_PTR nIDTool = 0);
	void DelTool(CWnd* pWnd, UINT_PTR nIDTool = 0);
	void DelTool(int nIdWnd, UINT_PTR nIDTool = 0);

	static BOOL ShowBalloon(CWnd* pWnd, UINT nIDText, UINT nIDTitle, UINT icon = 0);
	void ShowBalloon(int nIdWnd, UINT nIdText, UINT nIDTitle, UINT icon = 0);
	void RelayEvent(LPMSG lpMsg, CWnd* dlgWnd = nullptr);

protected:
	DECLARE_MESSAGE_MAP()
	afx_msg BOOL OnTtnNeedText(NMHDR *pNMHDR, LRESULT *pResult);

private:
	void SetTheme(bool bDark);

	CWnd *	m_pParentWnd;
	std::map<UINT, CString>		toolTextMap;
	int m_themeCallbackId;

	static CString LoadTooltip( UINT nIDText );
};
