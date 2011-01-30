// EditGoto.cpp : implementation file
//

#include "stdafx.h"
#include "TortoiseGitBlame.h"
#include "EditGotoDlg.h"


// CEditGotoDlg dialog

IMPLEMENT_DYNAMIC(CEditGotoDlg, CDialog)

CEditGotoDlg::CEditGotoDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CEditGotoDlg::IDD, pParent)
	, m_LineNumber(0)
{

}

CEditGotoDlg::~CEditGotoDlg()
{
}

void CEditGotoDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_LINENUMBER, m_LineNumber);
	DDV_MinMaxUInt(pDX, m_LineNumber, 0, 40000000);
}


BEGIN_MESSAGE_MAP(CEditGotoDlg, CDialog)

END_MESSAGE_MAP()


// CEditGotoDlg message handlers
