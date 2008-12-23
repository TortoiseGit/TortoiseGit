// SwitchTagPage.cpp : implementation file
//

#include "stdafx.h"
#include "TortoiseProc.h"
#include "SwitchTagPage.h"


// CSwitchTagPage dialog

IMPLEMENT_DYNAMIC(CSwitchTagPage, CPropertyPage)

CSwitchTagPage::CSwitchTagPage()
	: CPropertyPage(CSwitchTagPage::IDD)
{

}

CSwitchTagPage::~CSwitchTagPage()
{
}

void CSwitchTagPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CSwitchTagPage, CPropertyPage)
END_MESSAGE_MAP()


// CSwitchTagPage message handlers
