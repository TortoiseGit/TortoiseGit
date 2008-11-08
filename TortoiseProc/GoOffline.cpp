// GoOffline.cpp : implementation file
//

#include "stdafx.h"
#include "TortoiseProc.h"
#include "GoOffline.h"


// CGoOffline dialog

IMPLEMENT_DYNAMIC(CGoOffline, CDialog)

CGoOffline::CGoOffline(CWnd* pParent /*=NULL*/)
	: CDialog(CGoOffline::IDD, pParent)
    , asDefault(false)
    , selection(LogCache::CRepositoryInfo::online)
{

}

CGoOffline::~CGoOffline()
{
}

void CGoOffline::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);

	DDX_Check(pDX, IDC_ASDEFAULTOFFLINE, asDefault);
}


BEGIN_MESSAGE_MAP(CGoOffline, CDialog)
    ON_BN_CLICKED(IDOK, &CGoOffline::OnBnClickedOk)
    ON_BN_CLICKED(IDC_PERMANENTLYOFFLINE, &CGoOffline::OnBnClickedPermanentlyOffline)
    ON_BN_CLICKED(IDCANCEL, &CGoOffline::OnBnClickedCancel)
END_MESSAGE_MAP()


// CGoOffline message handlers

void CGoOffline::OnBnClickedOk()
{
    selection = LogCache::CRepositoryInfo::tempOffline;

    OnOK();
}

void CGoOffline::OnBnClickedPermanentlyOffline()
{
    selection = LogCache::CRepositoryInfo::offline;

    OnOK();
}

void CGoOffline::OnBnClickedCancel()
{
    selection = LogCache::CRepositoryInfo::online;

    OnCancel();
}
