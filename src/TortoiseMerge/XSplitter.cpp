// TortoiseGitMerge - a Diff/Patch program

// Copyright (C) 2006, 2011, 2016 - TortoiseSVN

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
#include "stdafx.h"
#include "XSplitter.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CXSplitter::CXSplitter()
	: m_bBarLocked(FALSE)
	, m_nHiddenCol(-1)
	, m_nHiddenRow(-1)
	, m_pColOldSize(nullptr)
	, m_pRowOldSize(nullptr)
	, m_nOldCols(0)
	, m_nOldRows(0)
{
}

CXSplitter::~CXSplitter()
{
	delete [] m_pRowOldSize;
	delete [] m_pColOldSize;
}


BEGIN_MESSAGE_MAP(CXSplitter, CSplitterWnd)
	ON_WM_LBUTTONDBLCLK()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_WM_SETCURSOR()
END_MESSAGE_MAP()

void CXSplitter::OnLButtonDown(UINT nFlags, CPoint point)
{
	if (!m_bBarLocked)
		CSplitterWnd::OnLButtonDown(nFlags, point);
}

void CXSplitter::OnMouseMove(UINT nFlags, CPoint point)
{
	if (!m_bBarLocked)
		CSplitterWnd::OnMouseMove(nFlags, point);
	else
		CWnd::OnMouseMove(nFlags, point);
}

BOOL CXSplitter::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	if (!m_bBarLocked)
		return CWnd::OnSetCursor(pWnd, nHitTest, message);

	return CSplitterWnd::OnSetCursor(pWnd, nHitTest, message);
}

void CXSplitter::HideRow(int nRowHide)
{
	ASSERT_VALID( this );
	ASSERT( m_nRows > 1 );
	ASSERT( nRowHide < m_nRows );
	ASSERT( m_nHiddenRow == -1 );
	m_nHiddenRow = nRowHide;

	int nActiveRow, nActiveCol;

	// if the nRow has an active window -- change it
	if (GetActivePane(&nActiveRow, &nActiveCol) != nullptr)
	{
		if( nActiveRow == nRowHide )
		{
			if( ++nActiveRow >= m_nRows )
				nActiveRow = 0;
			SetActivePane( nActiveRow, nActiveCol );
		}
	}

	// hide all nRow panes.
	for( int nCol = 0; nCol < m_nCols; ++nCol )
	{
		CWnd* pPaneHide = GetPane( nRowHide, nCol );
		ASSERT(pPaneHide != nullptr);

		pPaneHide->ShowWindow( SW_HIDE );
		pPaneHide->SetDlgCtrlID( AFX_IDW_PANE_FIRST+nCol * 16+m_nRows );

		for( int nRow = nRowHide+1; nRow < m_nRows; ++nRow )
		{
			CWnd* pPane = GetPane( nRow, nCol );
			ASSERT(pPane != nullptr);

			pPane->SetDlgCtrlID( IdFromRowCol( nRow-1, nCol ));
		}
	}

	m_nRows--;
	m_pRowInfo[m_nRows].nCurSize = m_pRowInfo[nRowHide].nCurSize;
	RecalcLayout();
}

void CXSplitter::ShowRow()
{
	ASSERT_VALID( this );
	ASSERT( m_nRows < m_nMaxRows );
	ASSERT( m_nHiddenRow != -1 );

	int nShowRow = m_nHiddenRow;
	m_nHiddenRow = -1;

	int cyNew = m_pRowInfo[m_nRows].nCurSize;
	m_nRows++;  // add a nRow

	ASSERT( m_nRows == m_nMaxRows );

	int nRow;

	// Show the hidden nRow
	for( int nCol = 0; nCol < m_nCols; ++nCol )
	{
		CWnd* pPaneShow = GetDlgItem( AFX_IDW_PANE_FIRST+nCol * 16+m_nRows );
		ASSERT(pPaneShow != nullptr);
		pPaneShow->ShowWindow( SW_SHOWNA );

		for( nRow = m_nRows - 2; nRow >= nShowRow; --nRow )
		{
			CWnd* pPane = GetPane( nRow, nCol );
			ASSERT(pPane != nullptr);
			pPane->SetDlgCtrlID( IdFromRowCol( nRow + 1, nCol ));
		}

		pPaneShow->SetDlgCtrlID( IdFromRowCol( nShowRow, nCol ));
	}

	// new panes have been created -- recalculate layout
	for( nRow = nShowRow+1; nRow < m_nRows; nRow++ )
		m_pRowInfo[nRow].nIdealSize = m_pRowInfo[nRow - 1].nCurSize;

	m_pRowInfo[nShowRow].nIdealSize = cyNew;
	RecalcLayout();
}

void CXSplitter::HideColumn(int nColHide)
{
	ASSERT_VALID( this );
	ASSERT( m_nCols > 1 );
	ASSERT( nColHide < m_nCols );
	ASSERT( m_nHiddenCol == -1 );
	m_nHiddenCol = nColHide;

	// if the column has an active window -- change it
	int nActiveRow, nActiveCol;
	if (GetActivePane(&nActiveRow, &nActiveCol) != nullptr)
	{
		if( nActiveCol == nColHide )
		{
			if( ++nActiveCol >= m_nCols )
				nActiveCol = 0;
			SetActivePane( nActiveRow, nActiveCol );
		}
	}

	// hide all column panes
	for( int nRow = 0; nRow < m_nRows; nRow++)
	{
		CWnd* pPaneHide = GetPane(nRow, nColHide);
		ASSERT(pPaneHide != nullptr);

		pPaneHide->ShowWindow(SW_HIDE);
		pPaneHide->SetDlgCtrlID( AFX_IDW_PANE_FIRST+nRow * 16+m_nCols );

		for( int nCol = nColHide + 1; nCol < m_nCols; nCol++ )
		{
			CWnd* pPane = GetPane( nRow, nCol );
			ASSERT(pPane != nullptr);

			pPane->SetDlgCtrlID( IdFromRowCol( nRow, nCol - 1 ));
		}
	}

	m_nCols--;
	m_pColInfo[m_nCols].nCurSize = m_pColInfo[nColHide].nCurSize;
	RecalcLayout();
}

void CXSplitter::ShowColumn()
{
	ASSERT_VALID( this );
	ASSERT( m_nCols < m_nMaxCols );
	ASSERT( m_nHiddenCol != -1 );

	int nShowCol = m_nHiddenCol;
	m_nHiddenCol = -1;

	int cxNew = m_pColInfo[m_nCols].nCurSize;
	m_nCols++;  // add a column

	ASSERT( m_nCols == m_nMaxCols );

	int nCol;

	// Show the hidden column
	for( int nRow = 0; nRow < m_nRows; ++nRow )
	{
		CWnd* pPaneShow = GetDlgItem( AFX_IDW_PANE_FIRST+nRow * 16+m_nCols );
		ASSERT(pPaneShow != nullptr);
		pPaneShow->ShowWindow( SW_SHOWNA );

		for( nCol = m_nCols - 2; nCol >= nShowCol; --nCol )
		{
			CWnd* pPane = GetPane( nRow, nCol );
			ASSERT(pPane != nullptr);
			pPane->SetDlgCtrlID( IdFromRowCol( nRow, nCol + 1 ));
		}

		pPaneShow->SetDlgCtrlID( IdFromRowCol( nRow, nShowCol ));
	}

	// new panes have been created -- recalculate layout
	for( nCol = nShowCol+1; nCol < m_nCols; nCol++ )
		m_pColInfo[nCol].nIdealSize = m_pColInfo[nCol - 1].nCurSize;

	m_pColInfo[nShowCol].nIdealSize = cxNew;
	RecalcLayout();
}

void CXSplitter::CenterSplitter()
{
	// get the size of all views
	int width = 0;
	int height = 0;
	for( int nRow = 0; nRow < m_nRows; ++nRow )
	{
		height += m_pRowInfo[nRow].nCurSize;
	}
	for( int nCol = 0; nCol < m_nCols; ++nCol )
	{
		width += m_pColInfo[nCol].nCurSize;
	}

	// now set the sizes of the views
	for( int nRow = 0; nRow < m_nRows; ++nRow )
	{
		m_pRowInfo[nRow].nIdealSize = height / m_nRows;
	}
	for( int nCol = 0; nCol < m_nCols; ++nCol )
	{
		m_pColInfo[nCol].nIdealSize = width / m_nCols;
	}
	RecalcLayout();
	CopyRowAndColInfo();
}

void CXSplitter::OnLButtonDblClk( UINT /*nFlags*/, CPoint /*point*/ )
{
	CenterSplitter();
}

void CXSplitter::OnLButtonUp(UINT nFlags, CPoint point)
{
	CSplitterWnd::OnLButtonUp(nFlags, point);
	CopyRowAndColInfo();
}

void CXSplitter::CopyRowAndColInfo()
{
	delete [] m_pColOldSize; m_pColOldSize = nullptr;
	delete [] m_pRowOldSize; m_pRowOldSize = nullptr;

	m_nOldCols = m_nCols;
	m_nOldRows = m_nRows;
	if (m_nCols)
	{
		m_pColOldSize = new int[m_nCols];
		for (int i = 0; i < m_nCols; ++i)
			m_pColOldSize[i] = m_pColInfo[i].nCurSize;
	}
	if (m_nRows)
	{
		m_pRowOldSize = new int[m_nRows];
		for (int i = 0; i < m_nRows; ++i)
			m_pRowOldSize[i] = m_pRowInfo[i].nCurSize;
	}
}
