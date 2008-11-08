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
#include "MyGraph.h"
#include "XPImageButton.h"
#include "TSVNPath.h"
#include "UnicodeUtils.h"

#include <map>
#include <list>

/**
 * \ingroup TortoiseProc
 * Helper class for drawing and then saving the drawing to a meta file (wmf)
 */
class CMyMetaFileDC : public CMetaFileDC
{
public:
	HGDIOBJ CMyMetaFileDC::SelectObject(HGDIOBJ hObject) 
	{
		return (hObject != NULL) ? ::SelectObject(m_hDC, hObject) : NULL; 
	}
};

/**
 * \ingroup TortoiseProc
 * Helper dialog showing statistics gathered from the log messages shown in the
 * log dialog.
 *
 * The function GatherData() collects statistical information and stores it
 * in the corresponding member variables. You can access the data as shown in
 * the following examples:
 * @code
 *    commits = m_commitsPerWeekAndAuthor[week_nr][author_name];
 *    filechanges = m_filechangesPerWeekAndAuthor[week_nr][author_name];
 *    commits = m_commitsPerAuthor[author_name];
 * @endcode

 */
class CStatGraphDlg : public CResizableStandAloneDialog//CResizableStandAloneDialog
{
	DECLARE_DYNAMIC(CStatGraphDlg)

public:
	CStatGraphDlg(CWnd* pParent = NULL);
	virtual ~CStatGraphDlg();

	enum { IDD = IDD_STATGRAPH };

	// Data passed from the caller of the dialog.
	CDWordArray	*	m_parDates;
	CDWordArray	*	m_parFileChanges;
	CStringArray *	m_parAuthors;
	CTSVNPath		m_path;

protected:

	// ** Data types **

	/// The types of units used in the various graphs.
	enum UnitType
	{
		Weeks,
		Months,
		Quarters,
		Years
	};

	/// The mapping type used to store data per interval/week and author.
	typedef std::map<int, std::map<stdstring, LONG> >	IntervalDataMap;
	/// The mapping type used to store data per author.
	typedef std::map<stdstring, LONG>					AuthorDataMap;

	// *** Re-implemented member functions from CDialog
	virtual void OnOK();
	virtual void OnCancel();

	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();
	void ShowLabels(BOOL bShow);
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg void OnCbnSelchangeGraphcombo();
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnBnClickedStacked();
	afx_msg void OnNeedText(NMHDR *pnmh, LRESULT *pResult);
	afx_msg void OnBnClickedGraphbarbutton();
	afx_msg void OnBnClickedGraphbarstackedbutton();
	afx_msg void OnBnClickedGraphlinebutton();
	afx_msg void OnBnClickedGraphlinestackedbutton();
	afx_msg void OnBnClickedGraphpiebutton();
	afx_msg void OnFileSavestatgraphas();
	DECLARE_MESSAGE_MAP()

	// ** Member functions **

	/// Updates the variables m_weekCount and m_minDate and returns the number 
	/// of weeks in the revision interval.
	void UpdateWeekCount();
	/// Returns the week-of-the-year for the given time.
	int	GetCalendarWeek(const CTime& time);
	/// Parses the data given to the dialog and generates mappings with statistical data. 
	void GatherData();
	/// Populates the lists passed as arguments based on the commit threshold set with the skipper.
	void FilterSkippedAuthors(std::list<stdstring>& included_authors, std::list<stdstring>& skipped_authors);
	/// Shows the graph with commit counts per author.
	void ShowCommitsByAuthor();
	/// Shows the graph with commit counts per author and date.
	void ShowCommitsByDate();
	/// Shows the initial statistics page.
	void ShowStats();


	/// Called when user checks/unchecks the "Authors case sensitive" checkbox.
	/// Recalculates statistical data because the number and names of authors 
	/// can have changed. Also calls RedrawGraph().
	void AuthorsCaseSensitiveChanged();
	/// Called when user checks/unchecks the "Sort by commit count" checkbox.
	/// Calls RedrawGraph().
	void SortModeChanged();
	/// Clears the current graph and frees all data series.
	void ClearGraph();
	/// Updates the currently shown statistics page.
	void RedrawGraph();

	int						GetUnitCount();
	int						GetUnit(const CTime& time);
	CStatGraphDlg::UnitType	GetUnitType();
	CString					GetUnitString();
	CString					GetUnitLabel(int unit, CTime &lasttime);

	void EnableDisableMenu();

	void SaveGraph(CString sFilename);
	int	 GetEncoderClsid(const WCHAR* format, CLSID* pClsid);

	void StoreCurrentGraphType();

	CPtrArray		m_graphDataArray;
	MyGraph			m_graph;
	CComboBox		m_cGraphType;
	CSliderCtrl		m_Skipper;
	BOOL			m_bAuthorsCaseSensitive;
	BOOL			m_bSortByCommitCount;

	CXPImageButton	m_btnGraphBar;
	CXPImageButton	m_btnGraphBarStacked;
	CXPImageButton	m_btnGraphLine;
	CXPImageButton	m_btnGraphLineStacked;
	CXPImageButton	m_btnGraphPie;

	HICON			m_hGraphBarIcon;
	HICON			m_hGraphBarStackedIcon;
	HICON			m_hGraphLineIcon;
	HICON			m_hGraphLineStackedIcon;
	HICON			m_hGraphPieIcon;

	MyGraph::GraphType	m_GraphType;
	bool				m_bStacked;

	CToolTipCtrl*	m_pToolTip;

	int				m_langOrder;

	// ** Member variables holding the statistical data **

	/// Number of weeks in the revision interval.
	int						m_nWeeks;		
	/// The starting date/time for the revision interval.
	__time64_t				m_minDate;
	/// The ending date/time for the revision interval.
	__time64_t				m_maxDate;		
	/// The total number of commits (equals size of the m_parXXX arrays).
	INT_PTR					m_nTotalCommits;
	/// The total number of file changes.
	LONG					m_nTotalFileChanges;
	/// Holds the number of commits per unit and author.
	IntervalDataMap			m_commitsPerUnitAndAuthor;		
	/// Holds the number of file changes per unit and author.
	IntervalDataMap			m_filechangesPerUnitAndAuthor;
	/// First interval number (key) in the mappings.
	int						m_firstInterval;
	/// Last interval number (key) in the mappings.
	int						m_lastInterval;
	/// Mapping of total commits per author, access data via
	AuthorDataMap			m_commitsPerAuthor;
	/// The list of author names sorted based on commit count 
	/// (author with most commits is first in list).
	std::list<stdstring>	m_authorNames;
	/// unit names by week/month/quarter
	std::map<LONG, stdstring> m_unitNames;

};
