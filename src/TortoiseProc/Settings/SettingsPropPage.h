// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2007-2010 - TortoiseSVN

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
#include "MessageBox.h"

/**
 * \ingroup TortoiseProc
 * Base class for all the settings property pages
 */
class ISettingsPropPage : public CPropertyPage
{
	DECLARE_DYNAMIC(ISettingsPropPage)
public:
	// simple construction
	ISettingsPropPage();
	explicit ISettingsPropPage(UINT nIDTemplate, UINT nIDCaption = 0, DWORD dwSize = sizeof(PROPSHEETPAGE));
	explicit ISettingsPropPage(LPCTSTR lpszTemplateName, UINT nIDCaption = 0, DWORD dwSize = sizeof(PROPSHEETPAGE));

	// extended construction
	ISettingsPropPage(UINT nIDTemplate, UINT nIDCaption,
		UINT nIDHeaderTitle, UINT nIDHeaderSubTitle = 0, DWORD dwSize = sizeof(PROPSHEETPAGE));
	ISettingsPropPage(LPCTSTR lpszTemplateName, UINT nIDCaption,
		UINT nIDHeaderTitle, UINT nIDHeaderSubTitle = 0, DWORD dwSize = sizeof(PROPSHEETPAGE));

	virtual ~ISettingsPropPage();

	enum SettingsRestart
	{
		Restart_None = 0,
		Restart_System = 1,
		Restart_Cache = 2
	};

	/**
	 * Returns the icon ID
	 */
	virtual UINT GetIconID() = 0;

	/**
	 * Returns the restart code
	 */
	virtual SettingsRestart GetRestart() {return m_restart;}

protected:

	SettingsRestart m_restart;

	/**
	* Adjusts the size of a checkbox or radio button control.
	* Since we always make the size of those bigger than 'necessary'
	* for making sure that translated strings can fit in those too,
	* this method can reduce the size of those controls again to only
	* fit the text.
	*/
	void AdjustControlSize(UINT nID)
	{
		CWnd* pwndDlgItem = GetDlgItem(nID);
		if (!pwndDlgItem)
			return;
		CDC* pDC = pwndDlgItem->GetWindowDC();
		if (!pDC)
			return;

		CString sControlText;
		pwndDlgItem->GetWindowText(sControlText);

		CRect ctrlRect;
		CRect ctrlRectOrig;
		pwndDlgItem->GetWindowRect(&ctrlRect);
		::MapWindowPoints(nullptr, GetSafeHwnd(), (LPPOINT)&ctrlRect, 2);
		ctrlRectOrig = ctrlRect;

		// find the rectangle the control text needs to be displayed
		CFont* font = pwndDlgItem->GetFont();
		CFont* pOldFont = pDC->SelectObject(font);
		if (pDC->DrawText(sControlText, -1, &ctrlRect, DT_EDITCONTROL | DT_EXPANDTABS | DT_LEFT | DT_CALCRECT))
		{
			// now we have the rectangle the control really needs
			if (ctrlRectOrig.Width() < ctrlRect.Width())
			{
				// we're dealing with radio buttons and check boxes,
				// which means we have to add a little space for the checkbox
				// the value of 3 pixels added here is necessary in case certain visual styles have
				// been disabled. Without this, the width is calculated too short.
				const int checkWidth = GetSystemMetrics(SM_CXMENUCHECK) + 2 * GetSystemMetrics(SM_CXEDGE) + 3;
				ctrlRectOrig.right = ctrlRectOrig.left + ctrlRect.Width() + checkWidth;
				pwndDlgItem->MoveWindow(&ctrlRectOrig);
			}
		}
		pDC->SelectObject(pOldFont);
		ReleaseDC(pDC);
	}

    /**
     * Utility method:
     * Store the current value of a BOOL, DWORD or CString into the
     * respective CRegDWORD etc. and check for success.
     */

    template<class T, class Reg>
    void Store (const T& value, Reg& registryKey)
    {
    	registryKey = value;
	    if (registryKey.GetLastError() != ERROR_SUCCESS)
		    CMessageBox::Show (m_hWnd, registryKey.getErrorString(), _T("TortoiseGit"), MB_ICONERROR);
    }
};
