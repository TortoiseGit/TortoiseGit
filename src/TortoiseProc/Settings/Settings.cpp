// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2003-2008 - TortoiseSVN

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
#include "Settings.h"
#include "MessageBox.h"
#include "..\..\TGitCache\CacheInterface.h"
#include "CommonResource.h"
#include "GitAdminDir.h"
#include "AppUtils.h"

IMPLEMENT_DYNAMIC(CSettings, CTreePropSheet)
CSettings::CSettings(UINT nIDCaption,CTGitPath *cmdPath, CWnd* pParentWnd, UINT iSelectPage)
	:CTreePropSheet(nIDCaption, pParentWnd, iSelectPage)
{
	if(cmdPath)
		this->m_CmdPath=*cmdPath;

	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	AddPropPages();
}

CSettings::~CSettings()
{
	RemovePropPages();
}

void CSettings::AddPropPages()
{
	m_pMainPage = new CSetMainPage();
	m_pOverlayPage = new CSetOverlayPage();
	m_pOverlaysPage = new CSetOverlayIcons();
	m_pOverlayHandlersPage = new CSetOverlayHandlers();
	m_pProxyPage = new CSetProxyPage();
	m_pProgsDiffPage = new CSettingsProgsDiff();
	m_pProgsMergePage = new CSettingsProgsMerge();
	m_pProgsAlternativeEditor = new CSettingsProgsAlternativeEditor();
	m_pLookAndFeelPage = new CSetLookAndFeelPage();

	m_pExtMenu	= new CSetExtMenu();

	m_pDialogsPage = new CSetDialogs();
	m_pColorsPage = new CSettingsColors();
    m_pColorsPage2 = new CSettingsColors2();
    m_pColorsPage3 = new CSettingsColors3();
	m_pSavedPage = new CSetSavedDataPage();
	m_pHooksPage = new CSetHooks();
	m_pBugTraqPage = new CSetBugTraq();
	m_pTBlamePage = new CSettingsTBlame();
	m_pGitConfig = new CSettingGitConfig();
	m_pGitRemote = new CSettingGitRemote(m_CmdPath.GetWinPath());
	m_pBugtraqConfig = new CSettingsBugtraqConfig(m_CmdPath.GetWinPath());


	SetPageIcon(m_pExtMenu,m_pExtMenu->GetIconID());

	SetPageIcon(m_pMainPage, m_pMainPage->GetIconID());
	SetPageIcon(m_pOverlayPage, m_pOverlayPage->GetIconID());
	SetPageIcon(m_pOverlaysPage, m_pOverlaysPage->GetIconID());
	SetPageIcon(m_pOverlayHandlersPage, m_pOverlayHandlersPage->GetIconID());
	SetPageIcon(m_pProxyPage, m_pProxyPage->GetIconID());
	SetPageIcon(m_pProgsDiffPage, m_pProgsDiffPage->GetIconID());
	SetPageIcon(m_pProgsMergePage, m_pProgsMergePage->GetIconID());
	SetPageIcon(m_pProgsAlternativeEditor, m_pProgsAlternativeEditor->GetIconID());
	SetPageIcon(m_pLookAndFeelPage, m_pLookAndFeelPage->GetIconID());
	SetPageIcon(m_pDialogsPage, m_pDialogsPage->GetIconID());
	SetPageIcon(m_pColorsPage, m_pColorsPage->GetIconID());
    SetPageIcon(m_pColorsPage2, m_pColorsPage2->GetIconID());
    SetPageIcon(m_pColorsPage3, m_pColorsPage3->GetIconID());

	SetPageIcon(m_pSavedPage, m_pSavedPage->GetIconID());
	SetPageIcon(m_pHooksPage, m_pHooksPage->GetIconID());

	SetPageIcon(m_pGitConfig, m_pGitConfig->GetIconID());
	SetPageIcon(m_pGitRemote, m_pGitRemote->GetIconID());
	SetPageIcon(m_pBugTraqPage, m_pBugTraqPage->GetIconID());
	SetPageIcon(m_pBugtraqConfig, m_pBugtraqConfig->GetIconID());
	SetPageIcon(m_pTBlamePage, m_pTBlamePage->GetIconID());

	AddPage(m_pMainPage);
	AddPage(m_pOverlayPage);
	AddPage(m_pOverlaysPage);
	AddPage(m_pOverlayHandlersPage);
	AddPage(m_pProxyPage);
	AddPage(m_pProgsDiffPage);
	AddPage(m_pProgsMergePage);
	AddPage(m_pLookAndFeelPage);
	AddPage(m_pExtMenu);
	AddPage(m_pDialogsPage);
	AddPage(m_pColorsPage);
	AddPage(m_pColorsPage2);
	AddPage(m_pColorsPage3);
	AddPage(m_pProgsAlternativeEditor);
	AddPage(m_pSavedPage);

	AddPage(m_pGitConfig);

	if (g_GitAdminDir.HasAdminDir(this->m_CmdPath.GetWinPath()) || g_GitAdminDir.IsBareRepo(this->m_CmdPath.GetWinPath()))
	{
		AddPage(m_pGitRemote);
	}
	AddPage(m_pHooksPage);
	AddPage(m_pBugTraqPage);
	if (g_GitAdminDir.HasAdminDir(this->m_CmdPath.GetWinPath()))
	{
		AddPage(m_pBugtraqConfig);
	}
	AddPage(m_pTBlamePage);
}

void CSettings::RemovePropPages()
{
	delete m_pMainPage;
	delete m_pOverlayPage;
	delete m_pOverlaysPage;
	delete m_pOverlayHandlersPage;
	delete m_pProxyPage;
	delete m_pProgsDiffPage;
	delete m_pProgsMergePage;
	delete m_pProgsAlternativeEditor;
	delete m_pLookAndFeelPage;
	delete m_pDialogsPage;
	delete m_pColorsPage;
	delete m_pColorsPage2;
	delete m_pColorsPage3;
	delete m_pSavedPage;
	delete m_pHooksPage;
	delete m_pBugTraqPage;
	delete m_pTBlamePage;

	delete m_pGitConfig;
	delete m_pGitRemote;
	delete m_pBugtraqConfig;
	delete m_pExtMenu;

}

void CSettings::HandleRestart()
{
	int restart = ISettingsPropPage::Restart_None;
	restart |= m_pMainPage->GetRestart();
	restart |= m_pOverlayPage->GetRestart();
	restart |= m_pOverlaysPage->GetRestart();
	restart |= m_pOverlayHandlersPage->GetRestart();
	restart |= m_pProxyPage->GetRestart();
	restart |= m_pProgsDiffPage->GetRestart();
	restart |= m_pProgsMergePage->GetRestart();
	restart |= m_pProgsAlternativeEditor->GetRestart();
	restart |= m_pLookAndFeelPage->GetRestart();
	restart |= m_pDialogsPage->GetRestart();
	restart |= m_pColorsPage->GetRestart();
	restart |= m_pColorsPage2->GetRestart();
	restart |= m_pColorsPage3->GetRestart();
	restart |= m_pSavedPage->GetRestart();
	restart |= m_pHooksPage->GetRestart();
	restart |= m_pBugTraqPage->GetRestart();
	restart |= m_pTBlamePage->GetRestart();

	restart |= m_pGitConfig->GetRestart();
	restart |= m_pGitRemote->GetRestart();
	restart |= m_pBugTraqPage->GetRestart();
	restart |= m_pExtMenu->GetRestart();

	if (restart & ISettingsPropPage::Restart_System)
	{
		DWORD_PTR res = 0;
		::SendMessageTimeout(HWND_BROADCAST, WM_SETTINGCHANGE, 0, 0, SMTO_ABORTIFHUNG, 20, &res);
		CMessageBox::Show(NULL, IDS_SETTINGS_RESTARTSYSTEM, IDS_APPNAME, MB_ICONINFORMATION);
	}
	if (restart & ISettingsPropPage::Restart_Cache)
	{
		DWORD_PTR res = 0;
		::SendMessageTimeout(HWND_BROADCAST, WM_SETTINGCHANGE, 0, 0, SMTO_ABORTIFHUNG, 20, &res);
		// tell the cache to refresh everything
		HANDLE hPipe = CreateFile(
			GetCacheCommandPipeName(),		// pipe name
			GENERIC_READ |					// read and write access
			GENERIC_WRITE,
			0,								// no sharing
			NULL,							// default security attributes
			OPEN_EXISTING,					// opens existing pipe
			FILE_FLAG_OVERLAPPED,			// default attributes
			NULL);							// no template file


		if (hPipe != INVALID_HANDLE_VALUE)
		{
			// The pipe connected; change to message-read mode.
			DWORD dwMode;

			dwMode = PIPE_READMODE_MESSAGE;
			if (SetNamedPipeHandleState(
				hPipe,    // pipe handle
				&dwMode,  // new pipe mode
				NULL,     // don't set maximum bytes
				NULL))    // don't set maximum time
			{
				DWORD cbWritten;
				TGITCacheCommand cmd;
				SecureZeroMemory(&cmd, sizeof(TGITCacheCommand));
				cmd.command = TGITCACHECOMMAND_REFRESHALL;
				BOOL fSuccess = WriteFile(
					hPipe,			// handle to pipe
					&cmd,			// buffer to write from
					sizeof(cmd),	// number of bytes to write
					&cbWritten,		// number of bytes written
					NULL);			// not overlapped I/O

				if (! fSuccess || sizeof(cmd) != cbWritten)
				{
					DisconnectNamedPipe(hPipe);
					CloseHandle(hPipe);
					hPipe = INVALID_HANDLE_VALUE;
				}
				if (hPipe != INVALID_HANDLE_VALUE)
				{
					// now tell the cache we don't need it's command thread anymore
					DWORD cbWritten;
					TGITCacheCommand cmd;
					SecureZeroMemory(&cmd, sizeof(TGITCacheCommand));
					cmd.command = TGITCACHECOMMAND_END;
					WriteFile(
						hPipe,			// handle to pipe
						&cmd,			// buffer to write from
						sizeof(cmd),	// number of bytes to write
						&cbWritten,		// number of bytes written
						NULL);			// not overlapped I/O
					DisconnectNamedPipe(hPipe);
					CloseHandle(hPipe);
					hPipe = INVALID_HANDLE_VALUE;
				}
			}
			else
			{
				ATLTRACE("SetNamedPipeHandleState failed");
				CloseHandle(hPipe);
			}
		}
	}
}

BEGIN_MESSAGE_MAP(CSettings, CTreePropSheet)
	ON_WM_QUERYDRAGICON()
	ON_WM_PAINT()
END_MESSAGE_MAP()

BOOL CSettings::OnInitDialog()
{
	BOOL bResult = CTreePropSheet::OnInitDialog();
	CAppUtils::MarkWindowAsUnpinnable(m_hWnd);

	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	CenterWindow(CWnd::FromHandle(hWndExplorer));

	if(this->m_DefaultPage == _T("gitremote"))
	{
		this->SetActivePage(this->m_pGitRemote);
	}
	if(this->m_DefaultPage == _T("gitconfig"))
	{
		this->SetActivePage(this->m_pGitConfig);
	}
	if(this->m_DefaultPage == _T("main"))
	{
		this->SetActivePage(this->m_pMainPage);
	}
	if(this->m_DefaultPage == _T("overlay"))
	{
		this->SetActivePage(this->m_pOverlayPage);
	}
	if(this->m_DefaultPage == _T("overlays"))
	{
		this->SetActivePage(this->m_pOverlaysPage);
	}
	if(this->m_DefaultPage == _T("overlayshandlers"))
	{
		this->SetActivePage(this->m_pOverlayHandlersPage);
	}
	if(this->m_DefaultPage == _T("proxy"))
	{
		this->SetActivePage(this->m_pProxyPage);
	}

	if(this->m_DefaultPage == _T("diff"))
	{
		this->SetActivePage(this->m_pProgsDiffPage);
	}
	if(this->m_DefaultPage == _T("merge"))
	{
		this->SetActivePage(this->m_pProgsMergePage);
	}
	if(this->m_DefaultPage == _T("alternativeeditor"))
	{
		this->SetActivePage(this->m_pProgsAlternativeEditor);
	}
	if(this->m_DefaultPage == _T("look"))
	{
		this->SetActivePage(this->m_pLookAndFeelPage);
	}
	if(this->m_DefaultPage == _T("dialog"))
	{
		this->SetActivePage(this->m_pDialogsPage);
	}
	if(this->m_DefaultPage == _T("color1"))
	{
		this->SetActivePage(this->m_pColorsPage);
	}
	if(this->m_DefaultPage == _T("color2"))
	{
		this->SetActivePage(this->m_pColorsPage2);
	}
	if(this->m_DefaultPage == _T("color3"))
	{
		this->SetActivePage(this->m_pColorsPage3);
	}
	if(this->m_DefaultPage == _T("save"))
	{
		this->SetActivePage(this->m_pSavedPage);
	}

	return bResult;
}

void CSettings::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CTreePropSheet::OnPaint();
	}
}

HCURSOR CSettings::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}
