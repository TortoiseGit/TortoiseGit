// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2014 - TortoiseGit
// Copyright (C) 2003-2008 - TortoioseSVN

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
 * Extends the CComboBoxEx class with a history of entered
 * values. An example of such a combobox is the Start/Run
 * dialog which lists the programs you used last in a combobox.
 * To use this class do the following:
 * -# add both files HistoryCombo.h and HistoryCombo.cpp to your project.
 * -# add a ComboBoxEx to your dialog
 * -# create a variable for the ComboBox of type control
 * -# change the type of the created variable from CComboBoxEx to
 *    CHistoryCombo
 * -# in your OnInitDialog() call SetURLHistory(TRUE) if your ComboBox
 *    contains URLs
 * -# in your OnInitDialog() call the LoadHistory() method
 * -# in your OnOK() or somewhere similar call the SaveHistory() method
 *
 * thats it.
 */
#include "Git.h"
class CHistoryCombo : public CComboBoxEx
{
// Construction
public:
	CHistoryCombo(BOOL bAllowSortStyle = FALSE);
	virtual ~CHistoryCombo();

	bool m_bWantReturn;
// Operations
public:
	/**
	 * Adds the string \a str to both the combobox and the history.
	 * If \a pos is specified, insert the string at the specified
	 * position, otherwise add it to the end of the list.
	 */
	int AddString(const CString& str, INT_PTR pos = -1, BOOL isSel = true);

	void DisableTooltip(){m_bDyn = FALSE;} //because rebase need disable combox tooltip to show version info
protected:
	DECLARE_MESSAGE_MAP()
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs) override;
	virtual BOOL PreTranslateMessage(MSG* pMsg) override;
	virtual void PreSubclassWindow() override;

	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);

	void CreateToolTip();

private:
	/**
	 * Inserts an entry into the combobox
	 */
	int InsertEntry(const CString& combostring, INT_PTR pos);

// Implementation
public:
	/**
	 * Clears the history in the registry/inifile and the ComboBox.
	 * \param bDeleteRegistryEntries if this value is true then the registry key
	 * itself is deleted.
	 */
	void ClearHistory(BOOL bDeleteRegistryEntries = TRUE);

	void Reset(){	ResetContent(); m_arEntries.RemoveAll(); };
	/**
	 * When \a bURLHistory is TRUE, treat the combo box entries
	 * as URLs. This activates Shell URL auto completion and
	 * the display of special icons in front of the combobox
	 * entries. Default is FALSE.
	 */
	void SetURLHistory(BOOL bURLHistory);
	/**
	 * When \a bPathHistory is TRUE, treat the combo box entries
	 * as Paths. This activates Shell Path auto completion and
	 * the display of special icons in front of the combobox
	 * entries. Default is FALSE.
	 */
	void SetPathHistory(BOOL bPathHistory);
	void SetCustomAutoSuggest(BOOL listEntries, BOOL bPathHistory, BOOL bURLHistory);
	/**
	 * Sets the maximum numbers of entries in the history list.
	 * If the history is larger as \em nMaxItems then the last
	 * items in the history are deleted.
	 */
	void SetMaxHistoryItems(int nMaxItems);
	/**
	 * Saves the history to the registry/inifile.
	 * \remark if you haven't called LoadHistory() before this method
	 * does nothing!
	 */
	void SaveHistory();
	/**
	 * Loads the history from the registry/inifile and fills in the
	 * ComboBox.
	 * \param lpszSection a section name where to put the entries, e.g. "lastloadedfiles"
	 * \param lpszKeyPrefix a prefix to use for the history entries in registry/inifiles. E.g. "file" or "entry"
	 */
	CString LoadHistory(LPCTSTR lpszSection, LPCTSTR lpszKeyPrefix);

	/**
	 * Goes through the stored history in registry and removes a specific entry
	 */
	static void RemoveEntryFromHistory(LPCTSTR lpszSection, LPCTSTR lpszKeyPrefix, const CString& entryToRemove);

	/**
	 * Returns the string in the combobox which is either selected or the user has entered.
	 */
	CString GetString() const;

	/**
	 * Populates the combobox with the items provided in the list, existing
	 * items will be removed.
	 * No checks for duplicates are performed!
	 */
	void SetList(const STRING_VECTOR& list);

	/**
	 * Removes the selected item from the combo box and updates
	 * the registry settings. Returns TRUE if successful.
	 */
	BOOL RemoveSelectedItem();

	/**
	 * Disables trimming of strings in the combobox. Useful if the combo box is
	 * used e.g. for searching.
	 */
	void DisableTrimming() { m_bTrim = false; }

	void SetCaseSensitive(BOOL bCaseSensitive) { m_bCaseSensitive = bCaseSensitive; }

	int FindStringExactCaseSensitive(int nIndexStart, LPCTSTR lpszFind);

	static int		m_nGitIconIndex;

protected:
	/**
	 * Will be called whenever the return key is pressed while the
	 * history combo has the input focus. A derived class may implement
	 * a special behavior for the return key by overriding this method.
	 * It must return true to prevent the default processing for the
	 * return key. The default implementation returns false.
	 */
	virtual bool OnReturnKeyPressed() { return m_bWantReturn; }

protected:
	CStringArray	m_arEntries;
	CString			m_sSection;
	CString			m_sKeyPrefix;
	int				m_nMaxHistoryItems;
	BOOL			m_bAllowSortStyle;
	BOOL			m_bURLHistory;
	BOOL			m_bPathHistory;
	HWND			m_hWndToolTip;
	TOOLINFO		m_ToolInfo;
	CString			m_ToolText;
	BOOL			m_ttShown;
	BOOL			m_bDyn;
	BOOL			m_bTrim;
	BOOL			m_bCaseSensitive;
};

class CCustomAutoCompleteSource : public IEnumString
{
public:
	CCustomAutoCompleteSource(const CStringArray& pData);

	//IUnknown
	virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject) override;
	virtual ULONG STDMETHODCALLTYPE AddRef() override;
	virtual ULONG STDMETHODCALLTYPE Release() override;

	//IEnumString
	virtual HRESULT STDMETHODCALLTYPE Clone(IEnumString** ppenum) override;
	virtual HRESULT STDMETHODCALLTYPE Next(ULONG celt, LPOLESTR* rgelt, ULONG* pceltFetched) override;
	virtual HRESULT STDMETHODCALLTYPE Reset() override;
	virtual HRESULT STDMETHODCALLTYPE Skip(ULONG celt) override;

private:
	volatile ULONG		m_cRefCount;
	int					m_index;
	const CStringArray&	m_pData;
};
