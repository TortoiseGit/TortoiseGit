// CloneDlg.cpp : implementation file
//

#include "stdafx.h"
#include "TortoiseProc.h"
#include "CloneDlg.h"


// CCloneDlg dialog

IMPLEMENT_DYNCREATE(CCloneDlg, CDHtmlDialog)

CCloneDlg::CCloneDlg(CWnd* pParent /*=NULL*/)
	: CDHtmlDialog(CCloneDlg::IDD, CCloneDlg::IDH, pParent)
{

}

CCloneDlg::~CCloneDlg()
{
}

void CCloneDlg::DoDataExchange(CDataExchange* pDX)
{
	CDHtmlDialog::DoDataExchange(pDX);
}

BOOL CCloneDlg::OnInitDialog()
{
	CDHtmlDialog::OnInitDialog();
	return TRUE;  // return TRUE  unless you set the focus to a control
}

BEGIN_MESSAGE_MAP(CCloneDlg, CDHtmlDialog)
	ON_BN_CLICKED(IDC_CLONE_BROWSE_URL, &CCloneDlg::OnBnClickedCloneBrowseUrl)
	ON_BN_CLICKED(IDC_CLONE_DIR_BROWSE, &CCloneDlg::OnBnClickedCloneDirBrowse)
END_MESSAGE_MAP()

BEGIN_DHTML_EVENT_MAP(CCloneDlg)
	DHTML_EVENT_ONCLICK(_T("ButtonOK"), OnButtonOK)
	DHTML_EVENT_ONCLICK(_T("ButtonCancel"), OnButtonCancel)
END_DHTML_EVENT_MAP()



// CCloneDlg message handlers

HRESULT CCloneDlg::OnButtonOK(IHTMLElement* /*pElement*/)
{
	OnOK();
	return S_OK;
}

HRESULT CCloneDlg::OnButtonCancel(IHTMLElement* /*pElement*/)
{
	OnCancel();
	return S_OK;
}

void CCloneDlg::OnBnClickedCloneBrowseUrl()
{
	// TODO: Add your control notification handler code here
}

void CCloneDlg::OnBnClickedCloneDirBrowse()
{
	// TODO: Add your control notification handler code here
}
