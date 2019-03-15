// TortoiseGitMerge - a Diff/Patch program

// Copyright (C) 2018-2019 - TortoiseGit
// Copyright (C) 2006, 2009, 2015, 2018 - TortoiseSVN

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

class CSetMainPage;
class CSetColorPage;

/**
 * \ingroup TortoiseMerge
 * This is the container for all settings pages. A setting page is
 * a class derived from CPropertyPage with an additional method called
 * SaveData(). The SaveData() method is used by the dialog to save
 * the settings the user has made - if that method is not called then
 * it means that the changes are discarded! Each settings page has
 * to make sure that no changes are saved outside that method.
 *
 */
class CSettings : public CPropertySheet
{
	DECLARE_DYNAMIC(CSettings)
private:
	/**
	 * Adds all pages to this Settings-Dialog.
	 */
	void AddPropPages();
	/**
	 * Removes the pages and frees up memory.
	 */
	void RemovePropPages();

	void BuildPropPageArray() override
	{
		CPropertySheet::BuildPropPageArray();

		// create a copy of existing PROPSHEETPAGE array which can be modified
		int nPages = static_cast<int>(m_pages.GetSize());
		int nBytes = 0;
		for (decltype(nPages) i = 0; i < nPages; ++i)
		{
			auto pPage = GetPage(i);
			nBytes += pPage->m_psp.dwSize;
		}
		auto ppsp0 = static_cast<LPPROPSHEETPAGE>(malloc(nBytes));
		Checked::memcpy_s(ppsp0, nBytes, m_psh.ppsp, nBytes);
		auto ppsp = ppsp0;
		for (decltype(nPages) i = 0; i < nPages; ++i)
		{
			const DLGTEMPLATE* pResource = ppsp->pResource;
			CDialogTemplate dlgTemplate(pResource);
			dlgTemplate.SetFont(L"MS Shell Dlg 2", 9);
			HGLOBAL hNew = GlobalAlloc(GPTR, dlgTemplate.m_dwTemplateSize);
			ppsp->pResource = static_cast<DLGTEMPLATE*>(GlobalLock(hNew));
			Checked::memcpy_s(const_cast<void*>(static_cast<const void*>(ppsp->pResource)), dlgTemplate.m_dwTemplateSize, dlgTemplate.m_hTemplate, dlgTemplate.m_dwTemplateSize);
			GlobalUnlock(hNew);
			reinterpret_cast<BYTE*&>(ppsp) += ppsp->dwSize;
		}
		// free existing PROPSHEETPAGE array and assign the new one
		free((void*)m_psh.ppsp);
		m_psh.ppsp = ppsp0;
	}

	CSetMainPage *		m_pMainPage;
	CSetColorPage *		m_pColorPage;

public:
	CSettings(UINT nIDCaption, CWnd* pParentWnd = nullptr, UINT iSelectPage = 0);
	CSettings(LPCTSTR pszCaption, CWnd* pParentWnd = nullptr, UINT iSelectPage = 0);
	virtual ~CSettings();

	/**
	 * Calls the SaveData()-methods of each of the settings pages.
	 */
	void SaveData();

	BOOL IsReloadNeeded() const;
protected:
	DECLARE_MESSAGE_MAP()
	virtual BOOL OnInitDialog();
};


