// TortoiseMerge - a Diff/Patch program

// Copyright (C) 2006 - Stefan Kueng

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
{
	m_bBarLocked=FALSE;
    m_nHiddenCol = -1;
    m_nHiddenRow = -1;
}

CXSplitter::~CXSplitter()
{
}


BEGIN_MESSAGE_MAP(CXSplitter, CSplitterWnd)
	ON_WM_LBUTTONDOWN()
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

BOOL CXSplitter::ReplaceView(int row, int col,CRuntimeClass * pViewClass,SIZE size)
{
	CCreateContext context;
	BOOL bSetActive;

	if ((GetPane(row,col)->IsKindOf(pViewClass))==TRUE)
		return FALSE;

	// Get pointer to CDocument object so that it can be used in the creation 
	// process of the new view
	CDocument * pDoc= ((CView *)GetPane(row,col))->GetDocument();
	CView * pActiveView=GetParentFrame()->GetActiveView();
	if (pActiveView==NULL || pActiveView==GetPane(row,col))
		bSetActive=TRUE;
	else
		bSetActive=FALSE;

	// set flag so that document will not be deleted when view is destroyed
	pDoc->m_bAutoDelete=FALSE;    
	// Delete existing view 
	((CView *) GetPane(row,col))->DestroyWindow();
	// set flag back to default 
	pDoc->m_bAutoDelete=TRUE;

	// Create new view                      
	context.m_pNewViewClass=pViewClass;
	context.m_pCurrentDoc=pDoc;
	context.m_pNewDocTemplate=NULL;
	context.m_pLastView=NULL;
	context.m_pCurrentFrame=NULL;

	CreateView(row,col,pViewClass,size, &context);

	CView * pNewView= (CView *)GetPane(row,col);

	if (bSetActive==TRUE)
		GetParentFrame()->SetActiveView(pNewView);

	RecalcLayout(); 
	GetPane(row,col)->SendMessage(WM_PAINT);

	return TRUE;
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
	if( GetActivePane( &nActiveRow, &nActiveCol ) != NULL )
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
		ASSERT( pPaneHide != NULL );

		pPaneHide->ShowWindow( SW_HIDE );
		pPaneHide->SetDlgCtrlID( AFX_IDW_PANE_FIRST+nCol * 16+m_nRows );

		for( int nRow = nRowHide+1; nRow < m_nRows; ++nRow )
		{
			CWnd* pPane = GetPane( nRow, nCol );
			ASSERT( pPane != NULL );

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
		ASSERT( pPaneShow != NULL );
		pPaneShow->ShowWindow( SW_SHOWNA );

		for( nRow = m_nRows - 2; nRow >= nShowRow; --nRow )
		{
			CWnd* pPane = GetPane( nRow, nCol );
			ASSERT( pPane != NULL );
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
	if( GetActivePane( &nActiveRow, &nActiveCol ) != NULL )
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
		ASSERT( pPaneHide != NULL );

		pPaneHide->ShowWindow(SW_HIDE);
		pPaneHide->SetDlgCtrlID( AFX_IDW_PANE_FIRST+nRow * 16+m_nCols );

		for( int nCol = nColHide + 1; nCol < m_nCols; nCol++ )
		{
			CWnd* pPane = GetPane( nRow, nCol );
			ASSERT( pPane != NULL );

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
		ASSERT( pPaneShow != NULL );
		pPaneShow->ShowWindow( SW_SHOWNA );

		for( nCol = m_nCols - 2; nCol >= nShowCol; --nCol )
		{
			CWnd* pPane = GetPane( nRow, nCol );
			ASSERT( pPane != NULL );
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

