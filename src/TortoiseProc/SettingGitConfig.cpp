// SettingGitConfig.cpp : implementation file
//

#include "stdafx.h"
#include "TortoiseProc.h"
#include "SettingGitConfig.h"


// CSettingGitConfig dialog

IMPLEMENT_DYNAMIC(CSettingGitConfig, CPropertyPage)

CSettingGitConfig::CSettingGitConfig()
	: CPropertyPage(CSettingGitConfig::IDD)
    , m_UserName(_T(""))
    , m_UserEmail(_T(""))
    , m_bGlobal(FALSE)
{

}

CSettingGitConfig::~CSettingGitConfig()
{
}

void CSettingGitConfig::DoDataExchange(CDataExchange* pDX)
{
    CPropertyPage::DoDataExchange(pDX);
    DDX_Text(pDX, IDC_GIT_USERNAME, m_UserName);
    DDX_Text(pDX, IDC_GIT_USEREMAIL, m_UserEmail);
    DDX_Check(pDX, IDC_CHECK_GLOBAL, m_bGlobal);
}


BEGIN_MESSAGE_MAP(CSettingGitConfig, CPropertyPage)
END_MESSAGE_MAP()


// CSettingGitConfig message handlers
