// TortoiseGitMerge - a Diff/Patch program

// Copyright (C) 2006, 2011 - TortoiseSVN

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
 * \ingroup TortoiseMerge
 * Extends the MFC CSplitterWnd with the functionality to
 * Show/Hide specific columns and rows, allows to lock
 * the splitter bars so the user can't move them
 * and also allows dynamic replacing of views with
 * other views.
 *
 * \par requirements
 * win98 or later\n
 * win2k or later\n
 * MFC\n
 *
 */
class CXSplitter : public CSplitterWnd
{
public:
	CXSplitter();
	virtual ~CXSplitter();

public:
	/**
	 * Checks if the splitter has its bars locked.
	 */
	BOOL		IsBarLocked() const  {return m_bBarLocked;}
	/**
	 * Locks/Unlocks the bar so the user can't move it.
	 * \param bState TRUE to lock, FALSE to unlock
	 */
	void		LockBar(BOOL bState=TRUE) {m_bBarLocked=bState;}
	/**
	 * Shows a splitter column which was previously hidden. Don't call
	 * this method if the column is already visible! Check it first
	 * with IsColumnHidden()
	 */
	void		ShowColumn();
	/**
	 * Hides the given splitter column. Don't call this method on already hidden columns!
	 * Check it first with IsColumnHidden()
	 * \param nColHide The column to hide
	 */
	void		HideColumn(int nColHide);
	/**
	 * Checks if a given column is hidden.
	 */
	BOOL		IsColumnHidden(int nCol) const {return (m_nHiddenCol == nCol);}
	/**
	 * Shows a splitter row which was previously hidden. Don't call
	 * this method if the row is already visible! Check it first
	 * with IsRowHidden()
	 */
	void		ShowRow();
	/**
	 * Hides the given splitter row. Don't call this method on already hidden rows!
	 * Check it first with IsRowHidden()
	 * \param nRowHide The row to hide
	 */
	void		HideRow(int nRowHide);
	/**
	 * Checks if a given row is hidden.
	 */
	BOOL		IsRowHidden(int nRow) const  {return (m_nHiddenRow == nRow);}

	/**
	 * Centers the splitter in the middle of the views
	 */
	void		CenterSplitter();

	int			GetOldRowCount() { return m_nOldRows; }
	int			GetOldColCount() { return m_nOldCols; }
	bool		HasOldRowSize() { return m_pRowOldSize != nullptr; }
	bool		HasOldColSize() { return m_pColOldSize != nullptr; }
	int			GetOldRowSize(int index) { return m_pRowOldSize[index]; }
	int			GetOldColSize(int index) { return m_pColOldSize[index]; }

protected:
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	DECLARE_MESSAGE_MAP()

	void		CopyRowAndColInfo();
private:
	BOOL		m_bBarLocked;	///< is the splitter bar locked?
	int			m_nHiddenCol;	///< Index of the hidden column.
	int			m_nHiddenRow;	///< Index of the hidden row
	int *		m_pColOldSize;	///< the current size of the last splitter positioning
	int *		m_pRowOldSize;	///< the current size of the last splitter positioning
	int			m_nOldCols;
	int			m_nOldRows;
};
