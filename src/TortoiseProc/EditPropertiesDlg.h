// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2008 - TortoiseSVN

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
#include "StandAloneDlg.h"
#include "SVNRev.h"
#include "ProjectProperties.h"
#include "Tooltip.h"

/**
 * \ingroup TortoiseProc
 * dialog showing a list of properties of the files/folders specified with SetPathList().
 */
class CEditPropertiesDlg : public CResizableStandAloneDialog
{
	DECLARE_DYNAMIC(CEditPropertiesDlg)

public:
	CEditPropertiesDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CEditPropertiesDlg();

	void	SetPathList(const CTSVNPathList& pathlist) {m_pathlist = pathlist;}
	void	SetRevision(const SVNRev& rev) {m_revision = rev;}
	void	Refresh();
	bool	HasChanged() {return m_bChanged;}

	void	SetProjectProperties(ProjectProperties * pProps) {m_pProjectProperties = pProps;}
	void	SetUUID(const CString& sUUID) {m_sUUID = sUUID;}
	void	RevProps(bool bRevProps = false) {m_bRevProps = bRevProps;}

// Dialog Data
	enum { IDD = IDD_EDITPROPERTIES };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual void OnCancel();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg void OnBnClickedHelp();
	afx_msg void OnNMCustomdrawEditproplist(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnBnClickedRemoveProps();
	afx_msg void OnBnClickedEditprops();
	afx_msg void OnLvnItemchangedEditproplist(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNMDblclkEditproplist(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnBnClickedSaveprop();
	afx_msg void OnBnClickedAddprops();
	afx_msg void OnBnClickedExport();
	afx_msg void OnBnClickedImport();

	DECLARE_MESSAGE_MAP()
private:
	static UINT PropsThreadEntry(LPVOID pVoid);
	UINT PropsThread();
	void EditProps(bool bAdd = false);

protected:
	class PropValue
	{
	public:
		PropValue(void) : count(0), allthesamevalue(true), isbinary(false) {};

		std::string	value;
		stdstring	value_without_newlines;
		int			count;
		bool		allthesamevalue;
		bool		isbinary;
	};
	CTSVNPathList	m_pathlist;
	CListCtrl		m_propList;
	BOOL			m_bRecursive;
	bool			m_bChanged;
	bool			m_bRevProps;
	volatile LONG	m_bThreadRunning;
	std::map<stdstring, PropValue>	m_properties;
	SVNRev			m_revision;
	CToolTips		m_tooltips;

	CString			m_sUUID;
	ProjectProperties *	m_pProjectProperties;
};
