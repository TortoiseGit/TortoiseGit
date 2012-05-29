// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2012 - TortoiseGit

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

// TortoiseGitBlame.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "afxwinappex.h"
#include "TortoiseGitBlame.h"
#include "MainFrm.h"
#include "../version.h"

#include "TortoiseGitBlameDoc.h"
#include "TortoiseGitBlameView.h"
#include "CmdLineParser.h"
#include "PathUtils.h"
#include "CommonAppUtils.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CTortoiseGitBlameApp

BEGIN_MESSAGE_MAP(CTortoiseGitBlameApp, CWinAppEx)
	ON_COMMAND(ID_APP_ABOUT, &CTortoiseGitBlameApp::OnAppAbout)
	// Standard file based document commands
	ON_COMMAND(ID_FILE_OPEN, &CWinAppEx::OnFileOpen)
	ON_COMMAND(ID_FILE_SETTINGS, &CTortoiseGitBlameApp::OnFileSettings)
	// Standard print setup command
	ON_COMMAND(ID_FILE_PRINT_SETUP, &CWinAppEx::OnFilePrintSetup)
END_MESSAGE_MAP()


// CTortoiseGitBlameApp construction

CTortoiseGitBlameApp::CTortoiseGitBlameApp()
{
	SetDllDirectory(L"");
	EnableHtmlHelp();

	m_bHiColorIcons = TRUE;
}

// The one and only CTortoiseGitBlameApp object
CTortoiseGitBlameApp theApp;
CString sOrigCWD;

// CTortoiseGitBlameApp initialization

BOOL CTortoiseGitBlameApp::InitInstance()
{
	{
		DWORD len = GetCurrentDirectory(0, NULL);
		if (len)
		{
			auto_buffer<TCHAR> originalCurrentDirectory(len);
			if (GetCurrentDirectory(len, originalCurrentDirectory))
			{
				sOrigCWD = originalCurrentDirectory;
				sOrigCWD = CPathUtils::GetLongPathname(sOrigCWD);
			}
		}
	}

	//set the resource dll for the required language
	CRegDWORD loc = CRegDWORD(_T("Software\\TortoiseGit\\LanguageID"), 1033);
	long langId = loc;
	CString langDll;
	HINSTANCE hInst = NULL;
	do
	{
		langDll.Format(_T("%sLanguages\\TortoiseGitBlame%d.dll"), (LPCTSTR)CPathUtils::GetAppParentDirectory(), langId);

		hInst = LoadLibrary(langDll);
		CString sVer = _T(STRPRODUCTVER);
		CString sFileVer = CPathUtils::GetVersionFromFile(langDll);
		if (sFileVer.Compare(sVer)!=0)
		{
			FreeLibrary(hInst);
			hInst = NULL;
		}
		if (hInst != NULL)
			AfxSetResourceHandle(hInst);
		else
		{
			DWORD lid = SUBLANGID(langId);
			lid--;
			if (lid > 0)
			{
				langId = MAKELANGID(PRIMARYLANGID(langId), lid);
			}
			else
				langId = 0;
		}
	} while ((hInst == NULL) && (langId != 0));
	TCHAR buf[6];
	_tcscpy_s(buf, _T("en"));
	langId = loc;
	CString sHelppath;
	sHelppath = this->m_pszHelpFilePath;
	sHelppath = sHelppath.MakeLower();
	sHelppath.Replace(_T(".chm"), _T("_en.chm"));
	free((void*)m_pszHelpFilePath);
	m_pszHelpFilePath=_tcsdup(sHelppath);
	sHelppath = CPathUtils::GetAppParentDirectory() + _T("Languages\\TortoiseGitBlame_en.chm");
	do
	{
		GetLocaleInfo(MAKELCID(langId, SORT_DEFAULT), LOCALE_SISO639LANGNAME, buf, _countof(buf));
		CString sLang = _T("_");
		sLang += buf;
		sHelppath.Replace(_T("_en"), sLang);
		if (PathFileExists(sHelppath))
		{
			free((void*)m_pszHelpFilePath);
			m_pszHelpFilePath=_tcsdup(sHelppath);
			break;
		}
		sHelppath.Replace(sLang, _T("_en"));
		GetLocaleInfo(MAKELCID(langId, SORT_DEFAULT), LOCALE_SISO3166CTRYNAME, buf, _countof(buf));
		sLang += _T("_");
		sLang += buf;
		sHelppath.Replace(_T("_en"), sLang);
		if (PathFileExists(sHelppath))
		{
			free((void*)m_pszHelpFilePath);
			m_pszHelpFilePath=_tcsdup(sHelppath);
			break;
		}
		sHelppath.Replace(sLang, _T("_en"));

		DWORD lid = SUBLANGID(langId);
		lid--;
		if (lid > 0)
		{
			langId = MAKELANGID(PRIMARYLANGID(langId), lid);
		}
		else
			langId = 0;
	} while (langId);
	setlocale(LC_ALL, ""); 
	// We need to explicitly set the thread locale to the system default one to avoid possible problems with saving files in its original codepage
	// The problems occures when the language of OS differs from the regional settings
	// See the details here: http://connect.microsoft.com/VisualStudio/feedback/ViewFeedback.aspx?FeedbackID=100887
	SetThreadLocale(LOCALE_SYSTEM_DEFAULT);

	// InitCommonControlsEx() is required on Windows XP if an application
	// manifest specifies use of ComCtl32.dll version 6 or later to enable
	// visual styles.  Otherwise, any window creation will fail.
	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);
	// Set this to include all the common control classes you want to use
	// in your application.
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);

	CWinAppEx::InitInstance();

	Gdiplus::GdiplusStartupInput gdiplusStartupInput;
	Gdiplus::GdiplusStartup(&m_gdiplusToken,&gdiplusStartupInput,NULL);

	// Initialize OLE libraries
	if (!AfxOleInit())
	{
		AfxMessageBox(IDP_OLE_INIT_FAILED);
		return FALSE;
	}
	AfxEnableControlContainer();
	// Standard initialization
	// If you are not using these features and wish to reduce the size
	// of your final executable, you should remove from the following
	// the specific initialization routines you do not need
	// Change the registry key under which our settings are stored

	SetRegistryKey(_T("TortoiseGit"));
	LoadStdProfileSettings(4);  // Load standard INI file options (including MRU)

	InitContextMenuManager();

	InitKeyboardManager();

	InitTooltipManager();
	CMFCToolTipInfo ttParams;
	ttParams.m_bVislManagerTheme = TRUE;
	theApp.GetTooltipManager()->SetTooltipParams(AFX_TOOLTIP_TYPE_ALL,
		RUNTIME_CLASS(CMFCToolTipCtrl), &ttParams);

	// Register the application's document templates.  Document templates
	//  serve as the connection between documents, frame windows and views
	CSingleDocTemplate* pDocTemplate;
	pDocTemplate = new CSingleDocTemplate(
		IDR_TORTOISE_GIT_BLAME_MAINFRAME,
		RUNTIME_CLASS(CTortoiseGitBlameDoc),
		RUNTIME_CLASS(CMainFrame),       // main SDI frame window
		RUNTIME_CLASS(CTortoiseGitBlameView));
	if (!pDocTemplate)
		return FALSE;
	AddDocTemplate(pDocTemplate);



	// Parse command line for standard shell commands, DDE, file open
	CCommandLineInfo cmdInfo;
	ParseCommandLine(cmdInfo);

	// Dispatch commands specified on the command line.  Will return FALSE if
	// app was launched with /RegServer, /Register, /Unregserver or /Unregister.
	if (!ProcessShellCommand(cmdInfo))
		return FALSE;

	// The one and only window has been initialized, so show and update it
	m_pMainWnd->ShowWindow(SW_SHOW);
	m_pMainWnd->UpdateWindow();
	// call DragAcceptFiles only if there's a suffix
	//  In an SDI app, this should occur after ProcessShellCommand
	return TRUE;
}

// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	enum { IDD = IDD_ABOUTBOX };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	BOOL OnInitDialog();

// Implementation
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
END_MESSAGE_MAP()

BOOL CAboutDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	TCHAR verbuf[1024] = {0};
	TCHAR maskbuf[1024] = {0};
	if (!::LoadString(GetModuleHandle(NULL), IDS_VERSION, maskbuf, _countof(maskbuf)))
	{
		SecureZeroMemory(maskbuf, sizeof(maskbuf));
	}
	_stprintf_s(verbuf, maskbuf, TGIT_VERMAJOR, TGIT_VERMINOR, TGIT_VERMICRO, TGIT_VERBUILD);
	SetDlgItemText(IDC_VERSION, verbuf);

	return FALSE;
}

// App command to run the dialog
void CTortoiseGitBlameApp::OnAppAbout()
{
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}

void CTortoiseGitBlameApp::OnFileSettings()
{
	CCommonAppUtils::RunTortoiseProc(_T(" /command:settings /page:blame"));
}

// CTortoiseGitBlameApp customization load/save methods

void CTortoiseGitBlameApp::PreLoadState()
{
	GetContextMenuManager()->AddMenu(IDR_BLAME_POPUP, IDR_BLAME_POPUP);
}

void CTortoiseGitBlameApp::LoadCustomState()
{
}

void CTortoiseGitBlameApp::SaveCustomState()
{
}

// CTortoiseGitBlameApp message handlers

int CTortoiseGitBlameApp::ExitInstance()
{
	Gdiplus::GdiplusShutdown(m_gdiplusToken);
	return CWinAppEx::ExitInstance();
}
