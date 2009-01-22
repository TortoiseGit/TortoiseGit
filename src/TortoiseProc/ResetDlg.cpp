// ResetDlg.cpp : implementation file
//

#include "stdafx.h"
#include "TortoiseProc.h"
#include "ResetDlg.h"


// CResetDlg dialog

IMPLEMENT_DYNAMIC(CResetDlg, CDialog)

CResetDlg::CResetDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CResetDlg::IDD, pParent)
    , m_ResetType(0)
{

}

CResetDlg::~CResetDlg()
{
}

void CResetDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CResetDlg, CDialog)
END_MESSAGE_MAP()


// CResetDlg message handlers
