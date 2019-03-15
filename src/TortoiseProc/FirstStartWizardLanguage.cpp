// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2016-2019 - TortoiseGit

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
#include "TortoiseProc.h"
#include "FirstStartWizard.h"
#include "FirstStartWizardStart.h"
#include "PathUtils.h"
#include "DirFileEnum.h"
#include "../version.h"
#include "FirstStartWizardLanguage.h"

#define DOWNLOAD_URL L"https://tortoisegit.org/download/"
#define WM_SETPAGEFOCUS WM_APP+2

IMPLEMENT_DYNAMIC(CFirstStartWizardLanguage, CFirstStartWizardBasePage)

CFirstStartWizardLanguage::CFirstStartWizardLanguage() : CFirstStartWizardBasePage(CFirstStartWizardLanguage::IDD)
, m_dwLanguage(1033)
, m_regLanguage(L"Software\\TortoiseGit\\LanguageID", 1033)
{
	m_psp.dwFlags |= PSP_DEFAULT | PSP_USEHEADERTITLE;
	m_psp.pszHeaderTitle = MAKEINTRESOURCE(IDS_FIRSTSTART_STARTTITLE);
}

CFirstStartWizardLanguage::~CFirstStartWizardLanguage()
{
}

void CFirstStartWizardLanguage::DoDataExchange(CDataExchange* pDX)
{
	CFirstStartWizardBasePage::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LANGUAGECOMBO, m_LanguageCombo);
	DDX_Control(pDX, IDC_LINK, m_link);
	if (m_LanguageCombo.GetCurSel() >= 0)
		m_dwLanguage = static_cast<DWORD>(m_LanguageCombo.GetItemData(m_LanguageCombo.GetCurSel()));
}

BEGIN_MESSAGE_MAP(CFirstStartWizardLanguage, CFirstStartWizardBasePage)
	ON_MESSAGE(WM_SETPAGEFOCUS, OnDialogDisplayed)
	ON_NOTIFY(NM_CLICK, IDC_FIRSTSTART_HINT, OnClickedLink)
	ON_BN_CLICKED(IDC_REFRESH, &CFirstStartWizardLanguage::OnBnClickedRefresh)
END_MESSAGE_MAP()

static void AppendStringResource(CString& text, UINT resouceID)
{
	CString temp;
	temp.LoadString(resouceID);
	text.AppendChar(L'\n');
	text.AppendChar(L'\n');
	text.Append(temp);
}

BOOL CFirstStartWizardLanguage::OnInitDialog()
{
	CFirstStartWizardBasePage::OnInitDialog();

	CString hinttext;
	hinttext.LoadString(IDS_FIRSTSTART_LANGUAGEHINT1);
	AppendStringResource(hinttext, IDS_FIRSTSTART_LANGUAGEHINT2);
	AppendStringResource(hinttext, IDS_FIRSTSTART_LANGUAGEHINT3);
	GetDlgItem(IDC_FIRSTSTART_HINT)->SetWindowText(hinttext);

	GetDlgItem(IDC_LINK)->SetWindowText(DOWNLOAD_URL);
	m_link.SetURL(DOWNLOAD_URL);
	AdjustControlSize(IDC_LINK, false);

	OnBnClickedRefresh();

	return TRUE;
}

BOOL CFirstStartWizardLanguage::OnSetActive()
{
	auto wiz = static_cast<CFirstStartWizard*>(GetParent());

	wiz->SetWizardButtons(PSWIZB_NEXT);

	PostMessage(WM_SETPAGEFOCUS, 0, 0);

	return CFirstStartWizardBasePage::OnSetActive();
}

LRESULT CFirstStartWizardLanguage::OnDialogDisplayed(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
	GetDlgItem(IDC_LANGUAGECOMBO)->SetFocus();

	return 0;
}

LRESULT CFirstStartWizardLanguage::OnWizardNext()
{
	UpdateData();

	if (m_dwLanguage != m_regLanguage)
	{
		m_regLanguage = m_dwLanguage;

		CAppUtils::RunTortoiseGitProc(L"/command:firststart /page:1", false, false);

		EndDialog(0);

		return -1;
	}

	return __super::OnWizardNext();
}


void CFirstStartWizardLanguage::OnClickedLink(NMHDR* pNMHDR, LRESULT* pResult)
{
	ATLASSERT(pNMHDR && pResult);
	auto pNMLink = reinterpret_cast<PNMLINK>(pNMHDR);
	if (wcscmp(pNMLink->item.szID, L"download") == 0)
		ShellExecute(GetSafeHwnd(), L"open", DOWNLOAD_URL, nullptr, nullptr, SW_SHOWNORMAL);
	*pResult = 0;
}


void CFirstStartWizardLanguage::OnBnClickedRefresh()
{
	UpdateData();
	m_LanguageCombo.ResetContent();

	// set up the language selecting combobox
	TCHAR buf[MAX_PATH] = { 0 };
	GetLocaleInfo(1033, LOCALE_SNATIVELANGNAME, buf, _countof(buf));
	m_LanguageCombo.AddString(buf);
	m_LanguageCombo.SetItemData(0, 1033);
	CString path = CPathUtils::GetAppParentDirectory();
	path = path + L"Languages\\";
	CSimpleFileFind finder(path, L"*.dll");
	int langcount = 0;
	while (finder.FindNextFileNoDirectories())
	{
		CString file = finder.GetFilePath();
		CString filename = finder.GetFileName();
		if (CStringUtils::StartsWithI(filename, L"TortoiseProc"))
		{
			CString sVer = _T(STRPRODUCTVER);
			sVer = sVer.Left(sVer.ReverseFind('.'));
			CString sFileVer = CPathUtils::GetVersionFromFile(file);
			sFileVer = sFileVer.Left(sFileVer.ReverseFind('.'));
			if (sFileVer.Compare(sVer) != 0)
				continue;
			CString sLoc = filename.Mid(static_cast<int>(wcslen(L"TortoiseProc")));
			sLoc = sLoc.Left(sLoc.GetLength() - static_cast<int>(wcslen(L".dll"))); // cut off ".dll"
			if (CStringUtils::StartsWith(sLoc, L"32") && (sLoc.GetLength() > 5))
				continue;
			DWORD loc = _wtoi(filename.Mid(static_cast<int>(wcslen(L"TortoiseProc"))));
			GetLocaleInfo(loc, LOCALE_SNATIVELANGNAME, buf, _countof(buf));
			CString sLang = buf;
			GetLocaleInfo(loc, LOCALE_SNATIVECTRYNAME, buf, _countof(buf));
			if (buf[0])
			{
				sLang += L" (";
				sLang += buf;
				sLang += L')';
			}
			m_LanguageCombo.AddString(sLang);
			m_LanguageCombo.SetItemData(++langcount, loc);
		}
	}

	m_regLanguage.read();
	m_dwLanguage = m_regLanguage;
	for (int i = 0; i < m_LanguageCombo.GetCount(); ++i)
	{
		if (m_LanguageCombo.GetItemData(i) == m_dwLanguage)
			m_LanguageCombo.SetCurSel(i);
	}
	UpdateData(FALSE);
}
