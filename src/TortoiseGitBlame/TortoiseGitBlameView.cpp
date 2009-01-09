
// TortoiseGitBlameView.cpp : implementation of the CTortoiseGitBlameView class
//

#include "stdafx.h"
#include "TortoiseGitBlame.h"

#include "TortoiseGitBlameDoc.h"
#include "TortoiseGitBlameView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CTortoiseGitBlameView

IMPLEMENT_DYNCREATE(CTortoiseGitBlameView, CView)

BEGIN_MESSAGE_MAP(CTortoiseGitBlameView, CView)
	// Standard printing commands
	ON_COMMAND(ID_FILE_PRINT, &CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_DIRECT, &CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, &CTortoiseGitBlameView::OnFilePrintPreview)
END_MESSAGE_MAP()

// CTortoiseGitBlameView construction/destruction

CTortoiseGitBlameView::CTortoiseGitBlameView()
{
	// TODO: add construction code here

}

CTortoiseGitBlameView::~CTortoiseGitBlameView()
{
}

BOOL CTortoiseGitBlameView::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	return CView::PreCreateWindow(cs);
}

// CTortoiseGitBlameView drawing

void CTortoiseGitBlameView::OnDraw(CDC* /*pDC*/)
{
	CTortoiseGitBlameDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	if (!pDoc)
		return;

	// TODO: add draw code for native data here
}


// CTortoiseGitBlameView printing


void CTortoiseGitBlameView::OnFilePrintPreview()
{
	AFXPrintPreview(this);
}

BOOL CTortoiseGitBlameView::OnPreparePrinting(CPrintInfo* pInfo)
{
	// default preparation
	return DoPreparePrinting(pInfo);
}

void CTortoiseGitBlameView::OnBeginPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add extra initialization before printing
}

void CTortoiseGitBlameView::OnEndPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add cleanup after printing
}

void CTortoiseGitBlameView::OnRButtonUp(UINT nFlags, CPoint point)
{
	ClientToScreen(&point);
	OnContextMenu(this, point);
}

void CTortoiseGitBlameView::OnContextMenu(CWnd* pWnd, CPoint point)
{
	theApp.GetContextMenuManager()->ShowPopupMenu(IDR_POPUP_EDIT, point.x, point.y, this, TRUE);
}


// CTortoiseGitBlameView diagnostics

#ifdef _DEBUG
void CTortoiseGitBlameView::AssertValid() const
{
	CView::AssertValid();
}

void CTortoiseGitBlameView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

CTortoiseGitBlameDoc* CTortoiseGitBlameView::GetDocument() const // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CTortoiseGitBlameDoc)));
	return (CTortoiseGitBlameDoc*)m_pDocument;
}
#endif //_DEBUG


// CTortoiseGitBlameView message handlers
