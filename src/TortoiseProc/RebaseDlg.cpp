// RebaseDlg.cpp : implementation file
//

#include "stdafx.h"
#include "TortoiseProc.h"
#include "RebaseDlg.h"


// CRebaseDlg dialog

IMPLEMENT_DYNAMIC(CRebaseDlg, CResizableStandAloneDialog)

CRebaseDlg::CRebaseDlg(CWnd* pParent /*=NULL*/)
	: CResizableStandAloneDialog(CRebaseDlg::IDD, pParent)
    , m_bPickAll(FALSE)
    , m_bSquashAll(FALSE)
    , m_bEditAll(FALSE)
{

}

CRebaseDlg::~CRebaseDlg()
{
}

void CRebaseDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_REBASE_PROGRESS, m_ProgressBar);
    DDX_Control(pDX, IDC_STATUS_STATIC, m_CtrlStatusText);
    DDX_Check(pDX, IDC_PICK_ALL, m_bPickAll);
    DDX_Check(pDX, IDC_SQUASH_ALL, m_bSquashAll);
    DDX_Check(pDX, IDC_EDIT_ALL, m_bEditAll);
	DDX_Control(pDX, IDC_REBASE_SPLIT, m_wndSplitter);
	DDX_Control(pDX,IDC_COMMIT_LIST,m_CommitList);
}


BEGIN_MESSAGE_MAP(CRebaseDlg, CResizableStandAloneDialog)
    ON_BN_CLICKED(IDC_PICK_ALL, &CRebaseDlg::OnBnClickedPickAll)
    ON_BN_CLICKED(IDC_SQUASH_ALL, &CRebaseDlg::OnBnClickedSquashAll)
    ON_BN_CLICKED(IDC_EDIT_ALL, &CRebaseDlg::OnBnClickedEditAll)
    ON_BN_CLICKED(IDC_REBASE_SPLIT, &CRebaseDlg::OnBnClickedRebaseSplit)
	ON_WM_SIZE()
END_MESSAGE_MAP()

void CRebaseDlg::AddRebaseAnchor()
{
	AddAnchor(IDC_REBASE_TAB,TOP_LEFT,BOTTOM_RIGHT);
	AddAnchor(IDC_COMMIT_LIST,TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_REBASE_SPLIT,TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_STATUS_STATIC, BOTTOM_LEFT,BOTTOM_RIGHT);
	AddAnchor(IDC_REBASE_CONTINUE,BOTTOM_RIGHT);
	AddAnchor(IDC_REBASE_ABORT, BOTTOM_RIGHT);
	AddAnchor(IDC_REBASE_PROGRESS,BOTTOM_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_PICK_ALL,TOP_LEFT);
	AddAnchor(IDC_SQUASH_ALL,TOP_LEFT);
	AddAnchor(IDC_EDIT_ALL,TOP_LEFT);
	
}

BOOL CRebaseDlg::OnInitDialog()
{
	CResizableStandAloneDialog::OnInitDialog();

	CRect rectDummy;
	//IDC_REBASE_DUMY_TAB
	
	GetClientRect(m_DlgOrigRect);
	m_CommitList.GetClientRect(m_CommitListOrigRect);

	CWnd *pwnd=this->GetDlgItem(IDC_REBASE_DUMY_TAB);
	pwnd->GetWindowRect(&rectDummy);

	rectDummy.top-=20;
	rectDummy.bottom-=20;

	rectDummy.left-=5;
	rectDummy.right-=5;
	
	if (!m_ctrlTabCtrl.Create(CMFCTabCtrl::STYLE_FLAT, rectDummy, this, IDC_REBASE_TAB))
	{
		TRACE0("Failed to create output tab window\n");
		return FALSE;      // fail to create
	}
	m_ctrlTabCtrl.SetResizeMode(CMFCTabCtrl::RESIZE_NO);
	// Create output panes:
	//const DWORD dwStyle = LBS_NOINTEGRALHEIGHT | WS_CHILD | WS_VISIBLE | WS_HSCROLL | WS_VSCROLL;
	const DWORD dwStyle =LVS_REPORT | LVS_SHOWSELALWAYS | LVS_ALIGNLEFT | LVS_OWNERDATA | WS_BORDER | WS_TABSTOP |LVS_SINGLESEL |WS_CHILD | WS_VISIBLE;

	if (! this->m_FileListCtrl.Create(dwStyle,rectDummy,&this->m_ctrlTabCtrl,0) )
	{
		TRACE0("Failed to create output windows\n");
		return FALSE;      // fail to create
	}

	if( ! this->m_LogMessageCtrl.Create(_T("Scintilla"),_T("source"),0,rectDummy,&m_ctrlTabCtrl,0,0) )
	{
		TRACE0("Failed to create log message control");
		return FALSE;
	}

	m_FileListCtrl.Init(SVNSLC_COLEXT | SVNSLC_COLSTATUS , _T("RebaseDlg"));

	m_ctrlTabCtrl.AddTab(&m_FileListCtrl,_T("Modified File"));
	m_ctrlTabCtrl.AddTab(&m_LogMessageCtrl,_T("Log Message"),1);
	AddRebaseAnchor();


	EnableSaveRestore(_T("RebaseDlg"));

	DWORD yPos = CRegDWORD(_T("Software\\TortoiseGit\\TortoiseProc\\ResizableState\\RebaseDlgSizer"));
	RECT rcDlg, rcLogMsg, rcFileList;
	GetClientRect(&rcDlg);
	m_CommitList.GetWindowRect(&rcLogMsg);
	ScreenToClient(&rcLogMsg);
	this->m_ctrlTabCtrl.GetWindowRect(&rcFileList);
	ScreenToClient(&rcFileList);
	if (yPos)
	{
		RECT rectSplitter;
		m_wndSplitter.GetWindowRect(&rectSplitter);
		ScreenToClient(&rectSplitter);
		int delta = yPos - rectSplitter.top;
		if ((rcLogMsg.bottom + delta > rcLogMsg.top)&&(rcLogMsg.bottom + delta < rcFileList.bottom - 30))
		{
			m_wndSplitter.SetWindowPos(NULL, 0, yPos, 0, 0, SWP_NOSIZE);
			DoSize(delta);
		}
	}


	return TRUE;
}
// CRebaseDlg message handlers

void CRebaseDlg::OnBnClickedPickAll()
{
    // TODO: Add your control notification handler code here
}

void CRebaseDlg::OnBnClickedSquashAll()
{
    // TODO: Add your control notification handler code here
}

void CRebaseDlg::OnBnClickedEditAll()
{
    // TODO: Add your control notification handler code here
}

void CRebaseDlg::OnBnClickedRebaseSplit()
{
    // TODO: Add your control notification handler code here
}

LRESULT CRebaseDlg::DefWindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message) {
	case WM_NOTIFY:
		if (wParam == IDC_REBASE_SPLIT)
		{ 
			SPC_NMHDR* pHdr = (SPC_NMHDR*) lParam;
			DoSize(pHdr->delta);
		}
		break;
	}

	return __super::DefWindowProc(message, wParam, lParam);
}

void CRebaseDlg::DoSize(int delta)
{
	
	this->RemoveAllAnchors();

	CSplitterControl::ChangeHeight(GetDlgItem(IDC_COMMIT_LIST), delta, CW_TOPALIGN);
	//CSplitterControl::ChangeHeight(GetDlgItem(), delta, CW_TOPALIGN);
	CSplitterControl::ChangeHeight(GetDlgItem(IDC_REBASE_TAB), -delta, CW_BOTTOMALIGN);
	//CSplitterControl::ChangeHeight(GetDlgItem(), -delta, CW_BOTTOMALIGN);
	CSplitterControl::ChangePos(GetDlgItem(IDC_SQUASH_ALL),0,delta);
	CSplitterControl::ChangePos(GetDlgItem(IDC_PICK_ALL),0,delta);
	CSplitterControl::ChangePos(GetDlgItem(IDC_EDIT_ALL),0,delta);
	
	this->AddRebaseAnchor();
	// adjust the minimum size of the dialog to prevent the resizing from
	// moving the list control too far down.
	CRect rcLogMsg;
	m_CommitList.GetClientRect(rcLogMsg);
	SetMinTrackSize(CSize(m_DlgOrigRect.Width(), m_DlgOrigRect.Height()-m_CommitListOrigRect.Height()+rcLogMsg.Height()));

	SetSplitterRange();
//	m_CommitList.Invalidate();

//	GetDlgItem(IDC_LOGMESSAGE)->Invalidate();

	this->m_ctrlTabCtrl.Invalidate();
	this->m_CommitList.Invalidate();
	this->m_FileListCtrl.Invalidate();
	this->m_LogMessageCtrl.Invalidate();

}

void CRebaseDlg::SetSplitterRange()
{
	if ((m_CommitList)&&(m_ctrlTabCtrl))
	{
		CRect rcTop;
		m_CommitList.GetWindowRect(rcTop);
		ScreenToClient(rcTop);
		CRect rcMiddle;
		m_ctrlTabCtrl.GetWindowRect(rcMiddle);
		ScreenToClient(rcMiddle);
		if (rcMiddle.Height() && rcMiddle.Width())
			m_wndSplitter.SetRange(rcTop.top+60, rcMiddle.bottom-80);
	}
}

void CRebaseDlg::OnSize(UINT nType,int cx, int cy)
{
	 // first, let the resizing take place
    __super::OnSize(nType, cx, cy);

    //set range
    SetSplitterRange();
}

void CRebaseDlg::SaveSplitterPos()
{
	if (!IsIconic())
	{
		CRegDWORD regPos = CRegDWORD(_T("Software\\TortoiseGit\\TortoiseProc\\ResizableState\\RebaseDlgSizer"));
		RECT rectSplitter;
		m_wndSplitter.GetWindowRect(&rectSplitter);
		ScreenToClient(&rectSplitter);
		regPos = rectSplitter.top;
	}
}