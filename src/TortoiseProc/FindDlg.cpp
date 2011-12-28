// TortoiseGit - a Windows shell extension for easy version control

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
#include "resource.h"
#include "FindDlg.h"


// CFindDlg dialog

IMPLEMENT_DYNAMIC(CFindDlg, CResizableStandAloneDialog)

CFindDlg::CFindDlg(CWnd* pParent /*=NULL*/)
	: CResizableStandAloneDialog(CFindDlg::IDD, pParent)
	, m_bTerminating(false)
	, m_bFindNext(false)
	, m_bMatchCase(FALSE)
	, m_bLimitToDiffs(FALSE)
	, m_bWholeWord(FALSE)
	, m_bIsRef(false)
{
	m_pParent = pParent;
}

CFindDlg::~CFindDlg()
{
}

void CFindDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Check(pDX, IDC_MATCHCASE, m_bMatchCase);
	DDX_Check(pDX, IDC_WHOLEWORD, m_bWholeWord);
	DDX_Control(pDX, IDC_FINDCOMBO, m_FindCombo);
	DDX_Control(pDX, IDC_LIST_REF, m_ctrlRefList);
	DDX_Control(pDX, IDC_EDIT_FILTER, m_ctrlFilter);
}


BEGIN_MESSAGE_MAP(CFindDlg, CResizableStandAloneDialog)
	ON_CBN_EDITCHANGE(IDC_FINDCOMBO, &CFindDlg::OnCbnEditchangeFindcombo)
	ON_NOTIFY(NM_CLICK, IDC_LIST_REF, &CFindDlg::OnNMClickListRef)
	ON_EN_CHANGE(IDC_EDIT_FILTER, &CFindDlg::OnEnChangeEditFilter)
	ON_WM_TIMER()
END_MESSAGE_MAP()


// CFindDlg message handlers

void CFindDlg::OnCancel()
{
	m_bTerminating = true;

	CWnd *parent = m_pParent;
	if(parent == NULL)
		parent = GetParent();

	if (parent)
		parent->SendMessage(m_FindMsg);

	DestroyWindow();
}

void CFindDlg::PostNcDestroy()
{
	delete this;
}

void CFindDlg::OnOK()
{
	UpdateData();
	m_FindCombo.SaveHistory();

	if (m_FindCombo.GetString().IsEmpty())
		return;
	m_bFindNext = true;
	m_FindString = m_FindCombo.GetString();

	CWnd *parent = m_pParent;
	if(parent == NULL)
		parent = GetParent();

	if (parent)
		parent->SendMessage(m_FindMsg);
	m_bFindNext = false;
}

BOOL CFindDlg::OnInitDialog()
{
	CDialog::OnInitDialog();
	m_FindMsg = RegisterWindowMessage(FINDMSGSTRING);

	m_FindCombo.LoadHistory(_T("Software\\TortoiseGit\\History\\Find"), _T("Search"));

	m_FindCombo.SetFocus();

	this->AddAnchor(IDC_STATIC_FIND, TOP_LEFT, TOP_RIGHT);
	this->AddAnchor(IDC_FINDCOMBO, TOP_LEFT, TOP_RIGHT);
	this->AddAnchor(IDOK, TOP_RIGHT);
	this->AddAnchor(IDCANCEL, TOP_RIGHT);
	this->AddAnchor(IDC_STATIC_GROUP_REF, TOP_LEFT, BOTTOM_RIGHT);
	this->AddAnchor(IDC_STATIC_FILTER, BOTTOM_LEFT);
	this->AddAnchor(IDC_EDIT_FILTER, BOTTOM_LEFT, BOTTOM_RIGHT);
	this->AddAnchor(IDC_LIST_REF, TOP_LEFT, BOTTOM_RIGHT);
	this->AddOthersToAnchor();

	EnableSaveRestore(_T("FindDlg"));

	CImageList *imagelist = new CImageList();
	imagelist->Create(IDB_BITMAP_REFTYPE,16,3,RGB(255,255,255));
	this->m_ctrlRefList.SetImageList(imagelist,LVSIL_SMALL);

	CRect rect;
	m_ctrlRefList.GetClientRect(&rect);

	this->m_ctrlRefList.InsertColumn(0,_T("Ref"),0, rect.Width()-50);
	g_Git.GetRefList(m_RefList);
	AddToList();
	return FALSE;
}

void CFindDlg::OnCbnEditchangeFindcombo()
{
	UpdateData();
	GetDlgItem(IDOK)->EnableWindow(!m_FindCombo.GetString().IsEmpty());
}

void CFindDlg::AddToList()
{
	this->m_ctrlRefList.DeleteAllItems();
	CString filter;
	this->m_ctrlFilter.GetWindowText(filter);

	int item =0;
	for(int i=0;i< m_RefList.size();i++)
	{
		int nImage = -1;
		CString ref = m_RefList[i];
		if(ref.Find(_T("refs/tags")) == 0)
			nImage = 0;
		else if(ref.Find(_T("refs/remotes"))==0)
			nImage = 2;
		else if(ref.Find(_T("refs/heads"))== 0)
			nImage = 1;

		if(ref.Find(filter)>=0)
			m_ctrlRefList.InsertItem(item++,ref,nImage);
	}
}

void CFindDlg::OnNMClickListRef(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);

	this->m_FindString = this->m_ctrlRefList.GetItemText(pNMItemActivate->iItem,0);
	this->m_bIsRef =true;

	CWnd *parent = m_pParent;
	if(parent == NULL)
		parent = GetParent();

	if (parent)
		parent->SendMessage(m_FindMsg);

	this->m_bIsRef =false;

	*pResult = 0;
}

void CFindDlg::OnEnChangeEditFilter()
{
	SetTimer(IDT_FILTER, 1000, NULL);
}

void CFindDlg::OnTimer(UINT_PTR nIDEvent)
{
	if( nIDEvent == IDT_FILTER)
	{
		KillTimer(IDT_FILTER);
		this->AddToList();
	}

	CResizableStandAloneDialog::OnTimer(nIDEvent);
}
