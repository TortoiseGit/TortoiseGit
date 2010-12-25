// ImportPatchDlg.cpp : implementation file
//

#include "stdafx.h"
#include "TortoiseProc.h"
#include "ImportPatchDlg.h"
#include "git.h"
#include "MessageBox.h"

// CImportPatchDlg dialog

IMPLEMENT_DYNAMIC(CImportPatchDlg, CResizableStandAloneDialog)

CImportPatchDlg::CImportPatchDlg(CWnd* pParent /*=NULL*/)
	: CResizableStandAloneDialog(CImportPatchDlg::IDD, pParent)
{
	m_cList.m_ContextMenuMask &=~ m_cList.GetMenuMask(CPatchListCtrl::MENU_APPLY);

	m_CurrentItem =0;

	m_bExitThread = false;
	m_bThreadRunning =false;
}

CImportPatchDlg::~CImportPatchDlg()
{

}

void CImportPatchDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_PATCH,m_cList);
	DDX_Control(pDX, IDC_AM_SPLIT, m_wndSplitter);
}
void CImportPatchDlg::AddAmAnchor()
{
	AddAnchor(IDC_AM_TAB,TOP_LEFT,BOTTOM_RIGHT);
	AddAnchor(IDC_LIST_PATCH, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_AM_SPLIT, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_BUTTON_ADD, TOP_RIGHT);
	AddAnchor(IDC_BUTTON_UP, TOP_RIGHT);
	AddAnchor(IDC_BUTTON_DOWN, TOP_RIGHT);
	AddAnchor(IDC_BUTTON_REMOVE, TOP_RIGHT);
	
	AddAnchor(IDOK,BOTTOM_RIGHT);
	AddAnchor(IDCANCEL,BOTTOM_RIGHT);
	AddAnchor(IDHELP, BOTTOM_RIGHT);

	AddAnchor(IDC_AM_DUMY_TAB, TOP_LEFT, BOTTOM_RIGHT);

	this->AddOthersToAnchor();
}

void CImportPatchDlg::SetSplitterRange()
{
	if ((m_cList)&&(m_ctrlTabCtrl))
	{
		CRect rcTop;
		m_cList.GetWindowRect(rcTop);
		ScreenToClient(rcTop);
		CRect rcMiddle;
		m_ctrlTabCtrl.GetWindowRect(rcMiddle);
		ScreenToClient(rcMiddle);
		if (rcMiddle.Height() && rcMiddle.Width())
			m_wndSplitter.SetRange(rcTop.top+160, rcMiddle.bottom-160);
	}
}

BOOL CImportPatchDlg::OnInitDialog()
{
	CResizableStandAloneDialog::OnInitDialog();
	
	CRect rectDummy;

	GetClientRect(m_DlgOrigRect);
	m_cList.GetClientRect(m_PatchListOrigRect);

	CWnd *pwnd=this->GetDlgItem(IDC_AM_DUMY_TAB);
	pwnd->GetWindowRect(&rectDummy);
	this->ScreenToClient(rectDummy);

	if (!m_ctrlTabCtrl.Create(CMFCTabCtrl::STYLE_FLAT, rectDummy, this, IDC_AM_TAB))
	{
		TRACE0("Failed to create output tab window\n");
		return FALSE;      // fail to create
	}
	m_ctrlTabCtrl.SetResizeMode(CMFCTabCtrl::RESIZE_NO);
	// Create output panes:
	//const DWORD dwStyle = LBS_NOINTEGRALHEIGHT | WS_CHILD | WS_VISIBLE | WS_HSCROLL | WS_VSCROLL;
	DWORD dwStyle =LVS_REPORT | LVS_SHOWSELALWAYS | LVS_ALIGNLEFT | WS_BORDER | WS_TABSTOP |LVS_SINGLESEL |WS_CHILD | WS_VISIBLE;

	if( ! this->m_PatchCtrl.Create(_T("Scintilla"),_T("source"),0,rectDummy,&m_ctrlTabCtrl,0,0) )
	{
		TRACE0("Failed to create log message control");
		return FALSE;
	}
	m_PatchCtrl.Init(0);
	m_PatchCtrl.Call(SCI_SETREADONLY, TRUE);

	dwStyle = LBS_NOINTEGRALHEIGHT | WS_CHILD | WS_VISIBLE | WS_HSCROLL | WS_VSCROLL;

	if (!m_wndOutput.Create(_T("Scintilla"),_T("source"),0,rectDummy, &m_ctrlTabCtrl, 0,0) )
	{
		TRACE0("Failed to create output windows\n");
		return -1;      // fail to create
	}
	m_wndOutput.Init(0);
	m_wndOutput.Call(SCI_SETREADONLY, TRUE);
	
	m_tooltips.Create(this);
	
	//m_tooltips.AddTool(IDC_REBASE_CHECK_FORCE,IDS_REBASE_FORCE_TT);
	//m_tooltips.AddTool(IDC_REBASE_ABORT,IDS_REBASE_ABORT_TT);
	
	m_ctrlTabCtrl.AddTab(&m_PatchCtrl,_T("Patch"),1);
	m_ctrlTabCtrl.AddTab(&m_wndOutput,_T("Log"),2);

	AddAmAnchor();

	m_PathList.SortByPathname(true);
	m_cList.SetExtendedStyle( m_cList.GetExtendedStyle()| LVS_EX_CHECKBOXES );

	for(int i=0;i<m_PathList.GetCount();i++)
	{
		m_cList.InsertItem(0,m_PathList[i].GetWinPath());
		m_cList.SetCheck(0,true);
	}

	DWORD yPos = CRegDWORD(_T("Software\\TortoiseGit\\TortoiseProc\\ResizableState\\AMDlgSizer"));
	RECT rcDlg, rcLogMsg, rcFileList;
	GetClientRect(&rcDlg);
	m_cList.GetWindowRect(&rcLogMsg);
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

	//CAppUtils::SetListCtrlBackgroundImage(m_cList.GetSafeHwnd(), nID);

	CString title;
	this->GetWindowText(title);
	this->SetWindowText(title+_T(" - ")+g_Git.m_CurrentDir);
	EnableSaveRestore(_T("ImportDlg"));

	SetSplitterRange();

	return TRUE;
}

BEGIN_MESSAGE_MAP(CImportPatchDlg, CResizableStandAloneDialog)
	ON_LBN_SELCHANGE(IDC_LIST_PATCH, &CImportPatchDlg::OnLbnSelchangeListPatch)
	ON_BN_CLICKED(IDC_BUTTON_ADD, &CImportPatchDlg::OnBnClickedButtonAdd)
	ON_BN_CLICKED(IDC_BUTTON_UP, &CImportPatchDlg::OnBnClickedButtonUp)
	ON_BN_CLICKED(IDC_BUTTON_DOWN, &CImportPatchDlg::OnBnClickedButtonDown)
	ON_BN_CLICKED(IDC_BUTTON_REMOVE, &CImportPatchDlg::OnBnClickedButtonRemove)
	ON_BN_CLICKED(IDOK, &CImportPatchDlg::OnBnClickedOk)
	ON_STN_CLICKED(IDC_AM_SPLIT, &CImportPatchDlg::OnStnClickedAmSplit)
	ON_WM_SIZE()
END_MESSAGE_MAP()


// CImportPatchDlg message handlers

void CImportPatchDlg::OnLbnSelchangeListPatch()
{
	// TODO: Add your control notification handler code here
	if(m_cList.GetSelectedCount() == 0)
	{
		this->GetDlgItem(IDC_BUTTON_UP)->EnableWindow(FALSE);
		this->GetDlgItem(IDC_BUTTON_DOWN)->EnableWindow(FALSE);
		this->GetDlgItem(IDC_BUTTON_REMOVE)->EnableWindow(FALSE);
	}else
	{
		this->GetDlgItem(IDC_BUTTON_UP)->EnableWindow(TRUE);
		this->GetDlgItem(IDC_BUTTON_DOWN)->EnableWindow(TRUE);
		this->GetDlgItem(IDC_BUTTON_REMOVE)->EnableWindow(TRUE);

	}

}

void CImportPatchDlg::OnBnClickedButtonAdd()
{
	
	CFileDialog dlg(TRUE,NULL,
					NULL,
					OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT|OFN_ALLOWMULTISELECT,
					_T("Patch Files(*.patch)|*.patch|Diff Files(*.diff)|*.diff|All Files(*.*)|*.*||"));
	if(dlg.DoModal()==IDOK)
	{
		POSITION pos;
		pos=dlg.GetStartPosition();
		while(pos)
		{
			CString file=dlg.GetNextPathName(pos);
			file.Trim();
			if(!file.IsEmpty())
			{
				m_cList.InsertItem(0,file);
				m_cList.SetCheck(0,true);
			}
		}
	}

	// TODO: Add your control notification handler code here
}

void CImportPatchDlg::OnBnClickedButtonUp()
{
	// TODO: Add your control notification handler code here
	POSITION pos;
	pos=m_cList.GetFirstSelectedItemPosition();
	while(pos)
	{
		int index=m_cList.GetNextSelectedItem(pos);
		if(index>1)
		{
			CString old=m_cList.GetItemText(index,0);
			m_cList.DeleteItem(index);

			m_cList.InsertItem(index-1,old);
		}
	}

}

void CImportPatchDlg::OnBnClickedButtonDown()
{
	// TODO: Add your control notification handler code here
	POSITION pos;
	pos=m_cList.GetFirstSelectedItemPosition();
	while(pos)
	{
		int index=m_cList.GetNextSelectedItem(pos);
		
		CString old=m_cList.GetItemText(index,0);
		m_cList.DeleteItem(index);

		m_cList.InsertItem(index+1,old);
		
	}
}

void CImportPatchDlg::OnBnClickedButtonRemove()
{
	// TODO: Add your control notification handler code here
	POSITION pos;
	pos=m_cList.GetFirstSelectedItemPosition();
	while(pos)
	{
		int index=m_cList.GetNextSelectedItem(pos);
		m_cList.DeleteItem(index);
		pos=m_cList.GetFirstSelectedItemPosition();
	}
}

UINT CImportPatchDlg::PatchThread()
{
	for(int i=m_CurrentItem;i<m_cList.GetItemCount();i++)
	{
		m_cList.SetItemData(i, CPatchListCtrl::STATUS_APPLYING);

		{
			Sleep(1000);
		}

		if(m_cList.GetCheck(i))
		{
			m_cList.SetItemData(i, i&1? CPatchListCtrl::STATUS_APPLY_SUCCESS:CPatchListCtrl::STATUS_APPLY_FAIL);

		}else
		{
			m_cList.SetItemData(i, CPatchListCtrl::STATUS_APPLY_SKIP);
		}

		m_CurrentItem++;

		CRect rect;
		this->m_cList.GetItemRect(i,&rect,LVIR_BOUNDS);
		this->m_cList.InvalidateRect(rect);
	}
	
	EnableInputCtrl(true);

	return 0;
}

void CImportPatchDlg::OnBnClickedOk()
{
	m_PathList.Clear();

	SaveSplitterPos();

	EnableInputCtrl(false);
	if ( (m_LoadingThread=AfxBeginThread(ThreadEntry, this)) ==NULL)
	{
		InterlockedExchange(&m_bThreadRunning, FALSE);
		CMessageBox::Show(NULL, IDS_ERR_THREADSTARTFAILED, IDS_APPNAME, MB_OK | MB_ICONERROR);
	}
}

void CImportPatchDlg::OnStnClickedAmSplit()
{
	// TODO: Add your control notification handler code here
}

void CImportPatchDlg::DoSize(int delta)
{
	this->RemoveAllAnchors();

	CSplitterControl::ChangeHeight(GetDlgItem(IDC_LIST_PATCH), delta, CW_TOPALIGN);
	//CSplitterControl::ChangeHeight(GetDlgItem(), delta, CW_TOPALIGN);
	CSplitterControl::ChangeHeight(GetDlgItem(IDC_AM_TAB), -delta, CW_BOTTOMALIGN);
	//CSplitterControl::ChangeHeight(GetDlgItem(), -delta, CW_BOTTOMALIGN);
	CSplitterControl::ChangePos(GetDlgItem(IDC_CHECK_3WAY),0,delta);
	CSplitterControl::ChangePos(GetDlgItem(IDC_CHECK_IGNORE_SPACE),0,delta);
	CSplitterControl::ChangePos(GetDlgItem(IDC_CHECK_IGNORE_WHITE_SPACE),0,delta);
	CSplitterControl::ChangePos(GetDlgItem(IDC_CHECK_AUTORETRY),0,delta);
	
	this->AddAmAnchor();
	// adjust the minimum size of the dialog to prevent the resizing from
	// moving the list control too far down.
	CRect rcLogMsg;
	m_cList.GetClientRect(rcLogMsg);
	SetMinTrackSize(CSize(m_DlgOrigRect.Width(), m_DlgOrigRect.Height()-m_PatchListOrigRect.Height()+rcLogMsg.Height()));

	SetSplitterRange();
//	m_CommitList.Invalidate();

//	GetDlgItem(IDC_LOGMESSAGE)->Invalidate();

	this->m_ctrlTabCtrl.Invalidate();
	
}

void CImportPatchDlg::OnSize(UINT nType, int cx, int cy)
{
	CResizableStandAloneDialog::OnSize(nType, cx, cy);

	// TODO: Add your message handler code here
	SetSplitterRange();
}

LRESULT CImportPatchDlg::DefWindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	// TODO: Add your specialized code here and/or call the base class
	switch (message) {
	case WM_NOTIFY:
		if (wParam == IDC_AM_SPLIT)
		{ 
			SPC_NMHDR* pHdr = (SPC_NMHDR*) lParam;
			DoSize(pHdr->delta);
		}
		break;
	}

	return CResizableStandAloneDialog::DefWindowProc(message, wParam, lParam);
}

void CImportPatchDlg::SaveSplitterPos()
{
	if (!IsIconic())
	{
		CRegDWORD regPos = CRegDWORD(_T("Software\\TortoiseGit\\TortoiseProc\\ResizableState\\AMDlgSizer"));
		RECT rectSplitter;
		m_wndSplitter.GetWindowRect(&rectSplitter);
		ScreenToClient(&rectSplitter);
		regPos = rectSplitter.top;
	}
	
}


void CImportPatchDlg::EnableInputCtrl(BOOL b)
{
	this->GetDlgItem(IDC_BUTTON_UP)->EnableWindow(b);
	this->GetDlgItem(IDC_BUTTON_DOWN)->EnableWindow(b);
	this->GetDlgItem(IDC_BUTTON_REMOVE)->EnableWindow(b);
	this->GetDlgItem(IDC_BUTTON_ADD)->EnableWindow(b);
	this->GetDlgItem(IDOK)->EnableWindow(b);

}