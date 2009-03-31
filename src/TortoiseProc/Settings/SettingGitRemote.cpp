// SettingGitRemote.cpp : implementation file
//

#include "stdafx.h"
#include "TortoiseProc.h"
#include "SettingGitRemote.h"


// CSettingGitRemote dialog

IMPLEMENT_DYNAMIC(CSettingGitRemote, ISettingsPropPage)

CSettingGitRemote::CSettingGitRemote()
	: ISettingsPropPage(CSettingGitRemote::IDD)
    , m_strRemote(_T(""))
    , m_strUrl(_T(""))
    , m_strPuttyKeyfile(_T(""))
{

}

CSettingGitRemote::~CSettingGitRemote()
{
}

void CSettingGitRemote::DoDataExchange(CDataExchange* pDX)
{
    CPropertyPage::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_LIST_REMOTE, m_ctrlRemoteList);
    DDX_Text(pDX, IDC_EDIT_REMOTE, m_strRemote);
    DDX_Text(pDX, IDC_EDIT_URL, m_strUrl);
    DDX_Control(pDX, IDC_CHECK_ISAUTOLOADPUTTYKEY, m_bAutoLoad);
    DDX_Text(pDX, IDC_EDIT_PUTTY_KEY, m_strPuttyKeyfile);
}


BEGIN_MESSAGE_MAP(CSettingGitRemote, CPropertyPage)
    ON_BN_CLICKED(IDC_BUTTON_BROWSE, &CSettingGitRemote::OnBnClickedButtonBrowse)
    ON_BN_CLICKED(IDC_BUTTON_ADD, &CSettingGitRemote::OnBnClickedButtonAdd)
    ON_LBN_SELCHANGE(IDC_LIST_REMOTE, &CSettingGitRemote::OnLbnSelchangeListRemote)
    ON_EN_CHANGE(IDC_EDIT_REMOTE, &CSettingGitRemote::OnEnChangeEditRemote)
    ON_EN_CHANGE(IDC_EDIT_URL, &CSettingGitRemote::OnEnChangeEditUrl)
    ON_BN_CLICKED(IDC_CHECK_ISAUTOLOADPUTTYKEY, &CSettingGitRemote::OnBnClickedCheckIsautoloadputtykey)
    ON_EN_CHANGE(IDC_EDIT_PUTTY_KEY, &CSettingGitRemote::OnEnChangeEditPuttyKey)
END_MESSAGE_MAP()


// CSettingGitRemote message handlers

void CSettingGitRemote::OnBnClickedButtonBrowse()
{
    // TODO: Add your control notification handler code here
}

void CSettingGitRemote::OnBnClickedButtonAdd()
{
    // TODO: Add your control notification handler code here
}

void CSettingGitRemote::OnLbnSelchangeListRemote()
{
    // TODO: Add your control notification handler code here
}

void CSettingGitRemote::OnEnChangeEditRemote()
{
    // TODO:  If this is a RICHEDIT control, the control will not
    // send this notification unless you override the ISettingsPropPage::OnInitDialog()
    // function and call CRichEditCtrl().SetEventMask()
    // with the ENM_CHANGE flag ORed into the mask.

    // TODO:  Add your control notification handler code here
}

void CSettingGitRemote::OnEnChangeEditUrl()
{
    // TODO:  If this is a RICHEDIT control, the control will not
    // send this notification unless you override the ISettingsPropPage::OnInitDialog()
    // function and call CRichEditCtrl().SetEventMask()
    // with the ENM_CHANGE flag ORed into the mask.

    // TODO:  Add your control notification handler code here
}

void CSettingGitRemote::OnBnClickedCheckIsautoloadputtykey()
{
    // TODO: Add your control notification handler code here
}

void CSettingGitRemote::OnEnChangeEditPuttyKey()
{
    // TODO:  If this is a RICHEDIT control, the control will not
    // send this notification unless you override the ISettingsPropPage::OnInitDialog()
    // function and call CRichEditCtrl().SetEventMask()
    // with the ENM_CHANGE flag ORed into the mask.

    // TODO:  Add your control notification handler code here
}
