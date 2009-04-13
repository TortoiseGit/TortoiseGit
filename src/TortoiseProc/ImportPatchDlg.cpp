// ImportPatchDlg.cpp : implementation file
//

#include "stdafx.h"
#include "TortoiseProc.h"
#include "ImportPatchDlg.h"


// CImportPatchDlg dialog

IMPLEMENT_DYNAMIC(CImportPatchDlg, CResizableStandAloneDialog)

CImportPatchDlg::CImportPatchDlg(CWnd* pParent /*=NULL*/)
	: CResizableStandAloneDialog(CImportPatchDlg::IDD, pParent)
{

}

CImportPatchDlg::~CImportPatchDlg()
{

}

void CImportPatchDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_PATCH,m_cList);
}

BOOL CImportPatchDlg::OnInitDialog()
{
	CResizableStandAloneDialog::OnInitDialog();

	AddAnchor(IDC_LIST_PATCH, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_BUTTON_ADD, TOP_RIGHT);
	AddAnchor(IDC_BUTTON_UP, TOP_RIGHT);
	AddAnchor(IDC_BUTTON_DOWN, TOP_RIGHT);
	AddAnchor(IDC_BUTTON_REMOVE, TOP_RIGHT);

	AddAnchor(IDOK,BOTTOM_RIGHT);
	AddAnchor(IDCANCEL,BOTTOM_RIGHT);

	this->AddOthersToAnchor();

	m_PathList.SortByPathname(true);

	for(int i=0;i<m_PathList.GetCount();i++)
	{
		m_cList.InsertItem(0,m_PathList[i].GetWinPath());
	}

	//CAppUtils::SetListCtrlBackgroundImage(m_cList.GetSafeHwnd(), nID);

	EnableSaveRestore(_T("ImportDlg"));

	return TRUE;
}

BEGIN_MESSAGE_MAP(CImportPatchDlg, CResizableStandAloneDialog)
	ON_LBN_SELCHANGE(IDC_LIST_PATCH, &CImportPatchDlg::OnLbnSelchangeListPatch)
	ON_BN_CLICKED(IDC_BUTTON_ADD, &CImportPatchDlg::OnBnClickedButtonAdd)
	ON_BN_CLICKED(IDC_BUTTON_UP, &CImportPatchDlg::OnBnClickedButtonUp)
	ON_BN_CLICKED(IDC_BUTTON_DOWN, &CImportPatchDlg::OnBnClickedButtonDown)
	ON_BN_CLICKED(IDC_BUTTON_REMOVE, &CImportPatchDlg::OnBnClickedButtonRemove)
	ON_BN_CLICKED(IDOK, &CImportPatchDlg::OnBnClickedOk)
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
			m_cList.InsertItem(0,dlg.GetNextPathName(pos));
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

void CImportPatchDlg::OnBnClickedOk()
{
	m_PathList.Clear();

	for(int i=0;i<m_cList.GetItemCount();i++)
	{
		CTGitPath path;
		path.SetFromWin(m_cList.GetItemText(i,0));
		m_PathList.AddPath(path);
	}
	OnOK();
}
