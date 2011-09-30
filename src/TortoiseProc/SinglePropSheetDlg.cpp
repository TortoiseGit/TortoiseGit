// SinglePropSheetDlg.cpp : implementation file
//

#include "stdafx.h"
#include "TortoiseProc.h"
#include "SinglePropSheetDlg.h"


// CSinglePropSheetDlg dialog
using namespace TreePropSheet;

IMPLEMENT_DYNAMIC(CSinglePropSheetDlg, CTreePropSheet)

CSinglePropSheetDlg::CSinglePropSheetDlg(const TCHAR* szCaption, ISettingsPropPage* pThePropPage, CWnd* pParent /*=NULL*/)
:	CTreePropSheet(szCaption,pParent),// CSinglePropSheetDlg::IDD, pParent),
	m_pThePropPage(pThePropPage)
{
	AddPropPages();
}

CSinglePropSheetDlg::~CSinglePropSheetDlg()
{
	RemovePropPages();
}

void CSinglePropSheetDlg::AddPropPages()
{
	SetPageIcon(m_pThePropPage, m_pThePropPage->GetIconID());
	AddPage(m_pThePropPage);
}

void CSinglePropSheetDlg::RemovePropPages()
{
	delete m_pThePropPage;
}

void CSinglePropSheetDlg::DoDataExchange(CDataExchange* pDX)
{
	CTreePropSheet::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CSinglePropSheetDlg, CTreePropSheet)
END_MESSAGE_MAP()


// CSinglePropSheetDlg message handlers

BOOL CSinglePropSheetDlg::OnInitDialog()
{
	BOOL bReturn = CTreePropSheet::OnInitDialog();

//	CRect clientRect;
//	GetClientRect(&clientRect);
//	clientRect.DeflateRect(10,10,10,10);
//	m_pThePropPage->Create(m_pThePropPage->m_lpszTemplateName,this);
//	m_pThePropPage->MoveWindow(clientRect);


	CenterWindow(CWnd::FromHandle(hWndExplorer));

	return bReturn;
}
