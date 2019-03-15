// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2015-2016, 2019 - TortoiseGit
// Copyright (C) 2003-2015 - TortoiseSVN

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

template <typename BaseType> class CommonDialogFunctions
{
public:
	CommonDialogFunctions(BaseType* ctrl)
	: m_ctrl(ctrl)
	{}

private:
	BaseType* m_ctrl;

public:
	/**
	* Adjusts the size of a checkbox or radio button control.
	* Since we always make the size of those bigger than 'necessary'
	* for making sure that translated strings can fit in those too,
	* this method can reduce the size of those controls again to only
	* fit the text.
	*/
	RECT AdjustControlSize(UINT nID, bool ischeck = true)
	{
		CWnd* pwndDlgItem = m_ctrl->GetDlgItem(nID);
		if (!pwndDlgItem)
			return { 0 };
		// adjust the size of the control to fit its content
		CString sControlText;
		pwndDlgItem->GetWindowText(sControlText);
		// next step: find the rectangle the control text needs to
		// be displayed

		CDC* pDC = pwndDlgItem->GetWindowDC();
		RECT controlrect;
		RECT controlrectorig;
		pwndDlgItem->GetWindowRect(&controlrect);
		::MapWindowPoints(nullptr, m_ctrl->GetSafeHwnd(), reinterpret_cast<LPPOINT>(&controlrect), 2);
		controlrectorig = controlrect;
		if (pDC)
		{
			CFont* font = pwndDlgItem->GetFont();
			CFont* pOldFont = pDC->SelectObject(font);
			if (pDC->DrawText(sControlText, -1, &controlrect, DT_EDITCONTROL | DT_EXPANDTABS | DT_LEFT | DT_CALCRECT))
			{
				// now we have the rectangle the control really needs
				if ((controlrectorig.right - controlrectorig.left) > (controlrect.right - controlrect.left))
				{
					// we're dealing with radio buttons and check boxes,
					// which means we have to add a little space for the checkbox
					// the value of 3 pixels added here is necessary in case certain visual styles have
					// been disabled. Without this, the width is calculated too short.
					const int checkWidth = GetSystemMetrics(SM_CXMENUCHECK) + 2 * GetSystemMetrics(SM_CXEDGE) + 3;
					controlrectorig.right = controlrectorig.left + (controlrect.right - controlrect.left) + (ischeck ? checkWidth : 0);
					pwndDlgItem->MoveWindow(&controlrectorig);
				}
			}
			pDC->SelectObject(pOldFont);
			m_ctrl->ReleaseDC(pDC);
		}
		return controlrectorig;
	}

	/**
	* Adjusts the size of a static control.
	* \param nID control ID
	* \param rc the position of the control where this control shall
	*           be positioned next to on its right side.
	* \param spacing number of pixels to add to rc.right
	*/
	RECT AdjustStaticSize(UINT nID, RECT rc, long spacing)
	{
		CWnd* pwndDlgItem = m_ctrl->GetDlgItem(nID);
		// adjust the size of the control to fit its content
		CString sControlText;
		pwndDlgItem->GetWindowText(sControlText);
		// next step: find the rectangle the control text needs to
		// be displayed

		CDC* pDC = pwndDlgItem->GetWindowDC();
		RECT controlrect;
		RECT controlrectorig;
		pwndDlgItem->GetWindowRect(&controlrect);
		::MapWindowPoints(nullptr, m_ctrl->GetSafeHwnd(), reinterpret_cast<LPPOINT>(&controlrect), 2);
		controlrect.right += 200;   // in case the control needs to be bigger than it currently is (e.g., due to translations)
		controlrectorig = controlrect;

		long height = controlrectorig.bottom - controlrectorig.top;
		long width = controlrectorig.right - controlrectorig.left;
		controlrectorig.left = rc.right + spacing;
		controlrectorig.right = controlrectorig.left + width;
		controlrectorig.bottom = rc.bottom;
		controlrectorig.top = controlrectorig.bottom - height;

		if (pDC)
		{
			CFont* font = pwndDlgItem->GetFont();
			CFont* pOldFont = pDC->SelectObject(font);
			if (pDC->DrawText(sControlText, -1, &controlrect, DT_WORDBREAK | DT_EDITCONTROL | DT_EXPANDTABS | DT_LEFT | DT_CALCRECT))
			{
				// now we have the rectangle the control really needs
				controlrectorig.right = controlrectorig.left + (controlrect.right - controlrect.left);
				pwndDlgItem->MoveWindow(&controlrectorig);
			}
			pDC->SelectObject(pOldFont);
			m_ctrl->ReleaseDC(pDC);
		}
		return controlrectorig;
	}

	/**
	* Display a balloon with close button, anchored at a given edit control on this dialog.
	*/
	void ShowEditBalloon(UINT nIdControl, UINT nIdText, UINT nIdTitle, int nIcon = TTI_WARNING)
	{
		CString text(MAKEINTRESOURCE(nIdText));
		CString title(MAKEINTRESOURCE(nIdTitle));
		ShowEditBalloon(nIdControl, text, title, nIcon);
	}
	void ShowEditBalloon(UINT nIdControl, const CString& text, const CString& title, int nIcon = TTI_WARNING)
	{
		EDITBALLOONTIP bt;
		bt.cbStruct = sizeof(bt);
		bt.pszText = text;
		bt.pszTitle = title;
		bt.ttiIcon = nIcon;
		m_ctrl->SendDlgItemMessage(nIdControl, EM_SHOWBALLOONTIP, 0, reinterpret_cast<LPARAM>(&bt));
	}
};
