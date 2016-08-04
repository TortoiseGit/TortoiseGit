// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2003-2008, 2010, 2013, 2016 - TortoiseSVN

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
 * On Vista and later, the IFileDialog is used with the FOS_PICKFOLDERS flag.
 */
class CBrowseFolder
{
public:
	enum retVal
	{
		CANCEL = 0,		///< the user has pressed cancel
		NOPATH,			///< no folder was selected
		OK				///< everything went just fine
	};
public:
	//constructor / deconstructor
	CBrowseFolder(void);
	~CBrowseFolder(void);
public:
	DWORD m_style;		///< styles of the dialog.
	/**
	 * Sets the info text of the dialog. Call this method before calling Show().
	 */
	void SetInfo(LPCTSTR title);
	/*
	 * Sets the text to show for the checkbox. If this method is not called,
	 * then no checkbox is added.
	 */
	void SetCheckBoxText(LPCTSTR checktext);
	void SetCheckBoxText2(LPCTSTR checktext);
	/**
	 * Shows the Dialog.
	 * \param parent [in] window handle of the parent window.
	 * \param path [out] the path to the folder which the user has selected
	 * \param sDefaultPath [in]
	 * \return one of CANCEL, NOPATH or OK
	 */
	CBrowseFolder::retVal Show(HWND parent, CString& path, const CString& sDefaultPath = CString());
	CBrowseFolder::retVal Show(HWND parent, LPTSTR path, size_t pathlen, LPCTSTR szDefaultPath = nullptr);

	/**
	 * If this is set to true, then the second checkbox gets disabled as soon as the first
	 * checkbox is checked. If the first checkbox is unchecked, then the second checkbox is enabled
	 * again.
	 */
	void DisableCheckBox2WhenCheckbox1IsEnabled(bool bSet = true) {m_DisableCheckbox2WhenCheckbox1IsChecked = bSet;}

	static BOOL m_bCheck;		///< state of the checkbox on closing the dialog
	static BOOL m_bCheck2;
protected:
	static CString m_sDefaultPath;
	TCHAR m_title[200];
	CString m_CheckText;
	CString m_CheckText2;
	bool m_DisableCheckbox2WhenCheckbox1IsChecked;
};
