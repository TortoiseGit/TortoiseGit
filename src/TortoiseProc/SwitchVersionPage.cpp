// SwitchVersionPage.cpp : implementation file
//

#include "stdafx.h"
#include "TortoiseProc.h"
#include "SwitchVersionPage.h"


// CSwitchVersionPage dialog

IMPLEMENT_DYNAMIC(CSwitchVersionPage, CPropertyPage)

CSwitchVersionPage::CSwitchVersionPage()
	: CPropertyPage(CSwitchVersionPage::IDD)
{

}

CSwitchVersionPage::~CSwitchVersionPage()
{
}

void CSwitchVersionPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CSwitchVersionPage, CPropertyPage)
END_MESSAGE_MAP()


// CSwitchVersionPage message handlers
