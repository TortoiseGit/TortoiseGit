// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007-2008 - TortoiseSVN

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
 * base class for the merge wizard property pages
 */
class CMergeWizardBasePage : public CPropertyPage
{
public:
	CMergeWizardBasePage() : CPropertyPage() {;}
	explicit CMergeWizardBasePage(UINT nIDTemplate, UINT nIDCaption = 0, DWORD dwSize = sizeof(PROPSHEETPAGE)) 
		: CPropertyPage(nIDTemplate, nIDCaption, dwSize) {;}
	explicit CMergeWizardBasePage(LPCTSTR lpszTemplateName, UINT nIDCaption = 0, DWORD dwSize = sizeof(PROPSHEETPAGE))
		: CPropertyPage(lpszTemplateName, nIDCaption, dwSize) {;}

	virtual ~CMergeWizardBasePage() {;}



protected:
	virtual void			SetButtonTexts()
	{
		CPropertySheet* psheet = (CPropertySheet*) GetParent();
		if (psheet)
		{
			psheet->GetDlgItem(ID_WIZFINISH)->SetWindowText(CString(MAKEINTRESOURCE(IDS_MERGE_MERGE)));
			psheet->GetDlgItem(ID_WIZBACK)->SetWindowText(CString(MAKEINTRESOURCE(IDS_PROPPAGE_BACK)));
			psheet->GetDlgItem(ID_WIZNEXT)->SetWindowText(CString(MAKEINTRESOURCE(IDS_PROPPAGE_NEXT)));
			psheet->GetDlgItem(IDHELP)->SetWindowText(CString(MAKEINTRESOURCE(IDS_PROPPAGE_HELP)));
			psheet->GetDlgItem(IDCANCEL)->SetWindowText(CString(MAKEINTRESOURCE(IDS_PROPPAGE_CANCEL)));
		}
	}

	void AdjustControlSize(UINT nID)
	{
		CWnd * pwndDlgItem = GetDlgItem(nID);
		// adjust the size of the control to fit its content
		CString sControlText;
		pwndDlgItem->GetWindowText(sControlText);
		// next step: find the rectangle the control text needs to
		// be displayed

		CDC * pDC = pwndDlgItem->GetWindowDC();
		RECT controlrect;
		RECT controlrectorig;
		pwndDlgItem->GetWindowRect(&controlrect);
		::MapWindowPoints(NULL, GetSafeHwnd(), (LPPOINT)&controlrect, 2);
		controlrectorig = controlrect;
		if (pDC)
		{
			CFont * font = pwndDlgItem->GetFont();
			CFont * pOldFont = pDC->SelectObject(font);
			if (pDC->DrawText(sControlText, -1, &controlrect, DT_WORDBREAK | DT_EDITCONTROL | DT_EXPANDTABS | DT_LEFT | DT_CALCRECT))
			{
				// now we have the rectangle the control really needs
				if ((controlrectorig.right - controlrectorig.left) > (controlrect.right - controlrect.left))
				{
					// we're dealing with radio buttons and check boxes,
					// which means we have to add a little space for the checkbox
					controlrectorig.right = controlrectorig.left + (controlrect.right - controlrect.left) + 20;
					pwndDlgItem->MoveWindow(&controlrectorig);
				}
			}
			pDC->SelectObject(pOldFont);
			ReleaseDC(pDC);
		}
	}
};
