// TortoiseGitMerge - a Diff/Patch program

// Copyright (C) 2012, 2019 - TortoiseGit
// Copyright (C) 2006-2010, 2012-2013 - TortoiseSVN

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
#include "TortoiseMerge.h"
#include "BrowseFolder.h"
#include "OpenDlg.h"
#include "CommonAppUtils.h"
#include "registry.h"

// COpenDlg dialog

IMPLEMENT_DYNAMIC(COpenDlg, CStandAloneDialog)
COpenDlg::COpenDlg(CWnd* pParent /*=nullptr*/)
	: CStandAloneDialog(COpenDlg::IDD, pParent)
	, m_bFromClipboard(FALSE)
	, m_cFormat(0)
	, m_nextViewer(nullptr)
{
}

COpenDlg::~COpenDlg()
{
}

void COpenDlg::DoDataExchange(CDataExchange* pDX)
{
	CStandAloneDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_BASEFILEEDIT, m_sBaseFile);
	DDX_Text(pDX, IDC_THEIRFILEEDIT, m_sTheirFile);
	DDX_Text(pDX, IDC_YOURFILEEDIT, m_sYourFile);
	DDX_Text(pDX, IDC_DIFFFILEEDIT, m_sUnifiedDiffFile);
	DDX_Text(pDX, IDC_DIRECTORYEDIT, m_sPatchDirectory);
	DDX_Control(pDX, IDC_BASEFILEEDIT, m_cBaseFileEdit);
	DDX_Control(pDX, IDC_THEIRFILEEDIT, m_cTheirFileEdit);
	DDX_Control(pDX, IDC_YOURFILEEDIT, m_cYourFileEdit);
	DDX_Control(pDX, IDC_DIFFFILEEDIT, m_cDiffFileEdit);
	DDX_Control(pDX, IDC_DIRECTORYEDIT, m_cDirEdit);
	DDX_Check(pDX, IDC_PATCHFROMCLIPBOARD, m_bFromClipboard);
}

BEGIN_MESSAGE_MAP(COpenDlg, CStandAloneDialog)
	ON_BN_CLICKED(IDC_BASEFILEBROWSE, OnBnClickedBasefilebrowse)
	ON_BN_CLICKED(IDC_THEIRFILEBROWSE, OnBnClickedTheirfilebrowse)
	ON_BN_CLICKED(IDC_YOURFILEBROWSE, OnBnClickedYourfilebrowse)
	ON_BN_CLICKED(IDHELP, OnBnClickedHelp)
	ON_BN_CLICKED(IDC_DIFFFILEBROWSE, OnBnClickedDifffilebrowse)
	ON_BN_CLICKED(IDC_DIRECTORYBROWSE, OnBnClickedDirectorybrowse)
	ON_BN_CLICKED(IDC_MERGERADIO, OnBnClickedMergeradio)
	ON_BN_CLICKED(IDC_APPLYRADIO, OnBnClickedApplyradio)
	ON_WM_CHANGECBCHAIN()
	ON_WM_DRAWCLIPBOARD()
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_PATCHFROMCLIPBOARD, &COpenDlg::OnBnClickedPatchfromclipboard)
END_MESSAGE_MAP()

BOOL COpenDlg::OnInitDialog()
{
	CStandAloneDialog::OnInitDialog();

	CRegDWORD lastRadioButton(L"Software\\TortoiseGitMerge\\OpenRadio", IDC_MERGERADIO);
	if (static_cast<DWORD>(lastRadioButton) != IDC_MERGERADIO && static_cast<DWORD>(lastRadioButton) != IDC_APPLYRADIO)
		lastRadioButton = IDC_MERGERADIO;
	GroupRadio(static_cast<DWORD>(lastRadioButton));
	CheckRadioButton(IDC_MERGERADIO, IDC_APPLYRADIO, static_cast<DWORD>(lastRadioButton));

	// turn on auto completion for the edit controls
	AutoCompleteOn(IDC_BASEFILEEDIT);
	AutoCompleteOn(IDC_THEIRFILEEDIT);
	AutoCompleteOn(IDC_YOURFILEEDIT);
	AutoCompleteOn(IDC_DIFFFILEEDIT);
	AutoCompleteOn(IDC_DIRECTORYEDIT);

	m_cFormat = RegisterClipboardFormat(L"TGIT_UNIFIEDDIFF");
	m_nextViewer = SetClipboardViewer();

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

// COpenDlg message handlers

void COpenDlg::OnBnClickedBasefilebrowse()
{
	OnBrowseForFile(m_sBaseFile);
}

void COpenDlg::OnBnClickedTheirfilebrowse()
{
	OnBrowseForFile(m_sTheirFile);
}

void COpenDlg::OnBnClickedYourfilebrowse()
{
	OnBrowseForFile(m_sYourFile);
}

void COpenDlg::OnBnClickedHelp()
{
	this->OnHelp();
}

void COpenDlg::OnBrowseForFile(CString& filepath, UINT nFileFilter)
{
	UpdateData();
	CCommonAppUtils::FileOpenSave(filepath, nullptr, IDS_SELECTFILE, nFileFilter, true, m_hWnd);
	UpdateData(FALSE);
}

void COpenDlg::OnBnClickedDifffilebrowse()
{
	OnBrowseForFile(m_sUnifiedDiffFile, IDS_PATCHFILEFILTER);
}

void COpenDlg::OnBnClickedDirectorybrowse()
{
	CBrowseFolder folderBrowser;
	UpdateData();
	folderBrowser.m_style = BIF_EDITBOX | BIF_NEWDIALOGSTYLE | BIF_RETURNFSANCESTORS | BIF_RETURNONLYFSDIRS;
	folderBrowser.Show(GetSafeHwnd(), m_sPatchDirectory, m_sPatchDirectory);
	UpdateData(FALSE);
}

void COpenDlg::OnBnClickedMergeradio()
{
	GroupRadio(IDC_MERGERADIO);
}

void COpenDlg::OnBnClickedApplyradio()
{
	GroupRadio(IDC_APPLYRADIO);
}

void COpenDlg::GroupRadio(UINT nID)
{
	BOOL bMerge = FALSE;
	BOOL bUnified = FALSE;
	if (nID == IDC_MERGERADIO)
		bMerge = TRUE;
	if (nID == IDC_APPLYRADIO)
		bUnified = TRUE;

	GetDlgItem(IDC_BASEFILEEDIT)->EnableWindow(bMerge);
	GetDlgItem(IDC_BASEFILEBROWSE)->EnableWindow(bMerge);
	GetDlgItem(IDC_THEIRFILEEDIT)->EnableWindow(bMerge);
	GetDlgItem(IDC_THEIRFILEBROWSE)->EnableWindow(bMerge);
	GetDlgItem(IDC_YOURFILEEDIT)->EnableWindow(bMerge);
	GetDlgItem(IDC_YOURFILEBROWSE)->EnableWindow(bMerge);

	GetDlgItem(IDC_DIFFFILEEDIT)->EnableWindow(bUnified);
	GetDlgItem(IDC_DIFFFILEBROWSE)->EnableWindow(bUnified);
	GetDlgItem(IDC_DIRECTORYEDIT)->EnableWindow(bUnified);
	GetDlgItem(IDC_DIRECTORYBROWSE)->EnableWindow(bUnified);

	CheckAndEnableClipboardChecker();
}

void COpenDlg::OnOK()
{
	UpdateData(TRUE);

	bool bUDiffOnClipboard = false;
	if (OpenClipboard())
	{
		UINT enumFormat = 0;
		do
		{
			if (enumFormat == m_cFormat)
			{
				bUDiffOnClipboard = true;
			}
		} while((enumFormat = EnumClipboardFormats(enumFormat))!=0);
		CloseClipboard();
	}

	if (GetDlgItem(IDC_BASEFILEEDIT)->IsWindowEnabled())
	{
		m_sUnifiedDiffFile.Empty();
		m_sPatchDirectory.Empty();
	}
	else
	{
		m_sBaseFile.Empty();
		m_sYourFile.Empty();
		m_sTheirFile.Empty();
	}
	UpdateData(FALSE);
	CString sFile;
	if (!m_sUnifiedDiffFile.IsEmpty())
		if (!PathFileExists(m_sUnifiedDiffFile))
			sFile = m_sUnifiedDiffFile;
	if (!m_sPatchDirectory.IsEmpty())
		if (!PathFileExists(m_sPatchDirectory))
			sFile = m_sPatchDirectory;
	if (!m_sBaseFile.IsEmpty())
		if (!PathFileExists(m_sBaseFile))
			sFile = m_sBaseFile;
	if (!m_sYourFile.IsEmpty())
		if (!PathFileExists(m_sYourFile))
			sFile = m_sYourFile;
	if (!m_sTheirFile.IsEmpty())
		if (!PathFileExists(m_sTheirFile))
			sFile = m_sTheirFile;

	if (bUDiffOnClipboard && m_bFromClipboard)
	{
		if (OpenClipboard())
		{
			HGLOBAL hglb = GetClipboardData(m_cFormat);
			auto lpstr = static_cast<LPCSTR>(GlobalLock(hglb));

			DWORD len = GetTempPath(0, nullptr);
			auto path = std::make_unique<TCHAR[]>(len + 1);
			auto tempF = std::make_unique<TCHAR[]>(len + 100);
			GetTempPath (len+1, path.get());
			GetTempFileName(path.get(), L"tsm", 0, tempF.get());
			CString sTempFile = CString(tempF.get());

			FILE * outFile;
			size_t patchlen = strlen(lpstr);
			_wfopen_s(&outFile, sTempFile, L"wb");
			if(outFile)
			{
				size_t size = fwrite(lpstr, sizeof(char), patchlen, outFile);
				if (size < patchlen)
					bUDiffOnClipboard = false;
				else
				{
					m_sUnifiedDiffFile = sTempFile;
					UpdateData(FALSE);
					sFile.Empty();
				}
				fclose(outFile);
			}
			GlobalUnlock(hglb);
			CloseClipboard();
		}

	}

	if (!sFile.IsEmpty())
	{
		CString sErr;
		sErr.Format(IDS_ERR_PATCH_INVALIDPATCHFILE, static_cast<LPCTSTR>(sFile));
		MessageBox(sErr, nullptr, MB_ICONERROR);
		return;
	}
	CRegDWORD lastRadioButton(L"Software\\TortoiseGitMerge\\OpenRadio", IDC_MERGERADIO);
	lastRadioButton = GetCheckedRadioButton(IDC_MERGERADIO, IDC_APPLYRADIO);
	CStandAloneDialog::OnOK();
}

void COpenDlg::OnChangeCbChain(HWND hWndRemove, HWND hWndAfter)
{
	CStandAloneDialog::OnChangeCbChain(hWndRemove, hWndAfter);
}

bool COpenDlg::CheckAndEnableClipboardChecker()
{
	int radio = GetCheckedRadioButton(IDC_MERGERADIO, IDC_APPLYRADIO);
	bool bUDiffOnClipboard = false;
	if (radio == IDC_APPLYRADIO)
	{
		if (OpenClipboard())
		{
			UINT enumFormat = 0;
			do
			{
				if (enumFormat == m_cFormat)
				{
					bUDiffOnClipboard = true;
				}
			} while((enumFormat = EnumClipboardFormats(enumFormat))!=0);
			CloseClipboard();
		}
	}

	DialogEnableWindow(IDC_PATCHFROMCLIPBOARD, bUDiffOnClipboard);
	return bUDiffOnClipboard;
}

void COpenDlg::OnDrawClipboard()
{
	CheckAndEnableClipboardChecker();
	CStandAloneDialog::OnDrawClipboard();
}

void COpenDlg::OnDestroy()
{
	ChangeClipboardChain(m_nextViewer);
	CStandAloneDialog::OnDestroy();
}

void COpenDlg::OnBnClickedPatchfromclipboard()
{
	UpdateData();
	DialogEnableWindow(IDC_DIFFFILEEDIT, !m_bFromClipboard);
	DialogEnableWindow(IDC_DIFFFILEBROWSE, !m_bFromClipboard);
}

void COpenDlg::AutoCompleteOn(int controlId)
{
	HWND hwnd;
	GetDlgItem(controlId, &hwnd);
	if (hwnd)
		SHAutoComplete(hwnd, SHACF_AUTOSUGGEST_FORCE_ON | SHACF_AUTOAPPEND_FORCE_ON | SHACF_FILESYSTEM);
}
