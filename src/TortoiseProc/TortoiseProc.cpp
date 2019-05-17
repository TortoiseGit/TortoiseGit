// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2019 - TortoiseGit
// Copyright (C) 2003-2008, 2012-2014 - TortoiseSVN

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
#include "SysImageList.h"
#include "../Utils/CrashReport.h"
#include "CmdLineParser.h"
#include "Hooks.h"
#include "AppUtils.h"
#include "PathUtils.h"
#include "StringUtils.h"
#include "UnicodeUtils.h"
#include "MessageBox.h"
#include "DirFileEnum.h"
#include "GitAdminDir.h"
#include "Git.h"
#include "SmartHandle.h"
#include "Commands\Command.h"
#include "../version.h"
#include "JumpListHelpers.h"
#include "ConfigureGitExe.h"
#include "Libraries.h"
#include "TaskbarUUID.h"
#include "ProjectProperties.h"
#include "HistoryCombo.h"
#include <math.h>
#include <random>
#include "SendMail.h"
#include "WindowsCredentialsStore.h"
#include "FirstStartWizard.h"
#include "AnimationManager.h"

#define STRUCT_IOVEC_DEFINED

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#pragma comment(linker, "\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

BEGIN_MESSAGE_MAP(CTortoiseProcApp, CWinAppEx)
	ON_COMMAND(ID_HELP, CWinAppEx::OnHelp)
END_MESSAGE_MAP()

//////////////////////////////////////////////////////////////////////////

CTortoiseProcApp::CTortoiseProcApp()
	: hWndExplorer(nullptr)
{
	CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": Constructor\n");
	SetDllDirectory(L"");
	CCrashReport::Instance().AddUserInfoToReport(L"CommandLine", GetCommandLine());
	EnableHtmlHelp();
	CHooks::Create();
	git_libgit2_init();
	CGit::SetGit2CredentialCallback(CAppUtils::Git2GetUserPassword);
	CGit::SetGit2CertificateCheckCertificate(CAppUtils::Git2CertificateCheck);
	m_bLoadUserToolbars = FALSE;
	m_bSaveState = FALSE;
	retSuccess = false;
	m_gdiplusToken = NULL;
#if defined (_WIN64) && _MSC_VER == 1800
	_set_FMA3_enable(0);
#endif
}

CTortoiseProcApp::~CTortoiseProcApp()
{
	CHooks::Destroy();
	git_libgit2_shutdown();
}

// The one and only CTortoiseProcApp object
CTortoiseProcApp theApp;
CString sOrigCWD;
CString g_sGroupingUUID;
CString g_sGroupingIcon;
bool g_bGroupingRemoveIcon = false;
HWND GetExplorerHWND()
{
	return theApp.GetExplorerHWND();
}

#if ENABLE_CRASHHANLDER
CCrashReportTGit crasher(L"TortoiseGit " _T(APP_X64_STRING), TGIT_VERMAJOR, TGIT_VERMINOR, TGIT_VERMICRO, TGIT_VERBUILD, TGIT_VERDATE);
#endif

// CTortoiseProcApp initialization

BOOL CTortoiseProcApp::InitInstance()
{
	CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": InitInstance\n");
	CheckUpgrade();
	CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerWindows));
	CMFCButton::EnableWindowsTheming();

	Gdiplus::GdiplusStartupInput gdiplusStartupInput;
	Gdiplus::GdiplusStartup(&m_gdiplusToken, &gdiplusStartupInput, nullptr);

	//set the resource dll for the required language
	CRegDWORD loc = CRegDWORD(L"Software\\TortoiseGit\\LanguageID", 1033);
	long langId = loc;
	CString langDll;
	CStringA langpath = CStringA(CPathUtils::GetAppParentDirectory());
	langpath += "Languages";
	do
	{
		langDll.Format(L"%sLanguages\\TortoiseProc%ld.dll", static_cast<LPCTSTR>(CPathUtils::GetAppParentDirectory()), langId);

		CString sVer = _T(STRPRODUCTVER);
		CString sFileVer = CPathUtils::GetVersionFromFile(langDll);
		if (sFileVer == sVer)
		{
			HINSTANCE hInst = LoadLibrary(langDll);
			if (hInst)
			{
				CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": Load Language DLL %s\n", static_cast<LPCTSTR>(langDll));
				AfxSetResourceHandle(hInst);
				break;
			}
		}
		{
			DWORD lid = SUBLANGID(langId);
			lid--;
			if (lid > 0)
				langId = MAKELANGID(PRIMARYLANGID(langId), lid);
			else
				langId = 0;
		}
	} while (langId != 0);
	{
		CString langStr;
		langStr.Format(L"%ld", langId);
		CCrashReport::Instance().AddUserInfoToReport(L"LanguageID", langStr);
	}
	TCHAR buf[6] = { 0 };
	wcscpy_s(buf, L"en");
	langId = loc;
	// MFC uses a help file with the same name as the application by default,
	// which means we have to change that default to our language specific help files
	CString sHelppath = CPathUtils::GetAppDirectory() + L"TortoiseGit_en.chm";
	free((void*)m_pszHelpFilePath);
	m_pszHelpFilePath=_wcsdup(sHelppath);
	sHelppath = CPathUtils::GetAppParentDirectory() + L"Languages\\TortoiseGit_en.chm";
	do
	{
		CString sLang = L"_";
		if (GetLocaleInfo(MAKELCID(langId, SORT_DEFAULT), LOCALE_SISO639LANGNAME, buf, _countof(buf)))
		{
			sLang += buf;
			sHelppath.Replace(L"_en", sLang);
			if (PathFileExists(sHelppath))
			{
				free((void*)m_pszHelpFilePath);
				m_pszHelpFilePath=_wcsdup(sHelppath);
				break;
			}
		}
		sHelppath.Replace(sLang, L"_en");
		if (GetLocaleInfo(MAKELCID(langId, SORT_DEFAULT), LOCALE_SISO3166CTRYNAME, buf, _countof(buf)))
		{
			sLang += L'_';
			sLang += buf;
			sHelppath.Replace(L"_en", sLang);
			if (PathFileExists(sHelppath))
			{
				free((void*)m_pszHelpFilePath);
				m_pszHelpFilePath=_wcsdup(sHelppath);
				break;
			}
		}
		sHelppath.Replace(sLang, L"_en");

		DWORD lid = SUBLANGID(langId);
		lid--;
		if (lid > 0)
			langId = MAKELANGID(PRIMARYLANGID(langId), lid);
		else
			langId = 0;
	} while (langId);
	CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": Set Help Filename %s\n", m_pszHelpFilePath);
	setlocale(LC_ALL, "");

	CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": Initializing UI components ...\n");
	// InitCommonControls() is required on Windows XP if an application
	// manifest specifies use of ComCtl32.dll version 6 or later to enable
	// visual styles.  Otherwise, any window creation will fail.

	INITCOMMONCONTROLSEX used = {
		sizeof(INITCOMMONCONTROLSEX),
			ICC_ANIMATE_CLASS | ICC_BAR_CLASSES | ICC_COOL_CLASSES | ICC_DATE_CLASSES |
			ICC_HOTKEY_CLASS | ICC_INTERNET_CLASSES | ICC_LISTVIEW_CLASSES |
			ICC_NATIVEFNTCTL_CLASS | ICC_PAGESCROLLER_CLASS | ICC_PROGRESS_CLASS |
			ICC_TAB_CLASSES | ICC_TREEVIEW_CLASSES | ICC_UPDOWN_CLASS |
			ICC_USEREX_CLASSES | ICC_WIN95_CLASSES
	};
	InitCommonControlsEx(&used);
	AfxOleInit();
	AfxEnableControlContainer();
	AfxInitRichEdit5();
	CWinAppEx::InitInstance();
	SetRegistryKey(L"TortoiseGit");
	SYS_IMAGE_LIST();
	CHistoryCombo::m_nGitIconIndex = SYS_IMAGE_LIST().AddIcon(CCommonAppUtils::LoadIconEx(IDI_GITCONFIG, 0, 0));
	AfxGetApp()->m_pszProfileName = _wcsdup(L"TortoiseProc"); // w/o this ResizableLib will store data under TortoiseGitProc which is not compatible with older versions

	CCmdLineParser parser(AfxGetApp()->m_lpCmdLine);

	hWndExplorer = nullptr;
	CString sVal = parser.GetVal(L"hwnd");
	if (!sVal.IsEmpty())
		hWndExplorer = reinterpret_cast<HWND>(_wcstoui64(sVal, nullptr, 16));

	while (GetParent(hWndExplorer))
		hWndExplorer = GetParent(hWndExplorer);
	if (!IsWindow(hWndExplorer))
		hWndExplorer = nullptr;

	// if HKCU\Software\TortoiseGit\Debug is not 0, show our command line
	// in a message box
	if (CRegDWORD(L"Software\\TortoiseGit\\Debug", FALSE) == TRUE)
		AfxMessageBox(AfxGetApp()->m_lpCmdLine, MB_OK | MB_ICONINFORMATION);

	if (parser.HasKey(L"command") && wcscmp(parser.GetVal(L"command"), L"firststart") == 0)
	{
		// CFirstStartWizard requires sOrigCWD to be set
		DWORD len = GetCurrentDirectory(0, nullptr);
		if (len)
		{
			auto originalCurrentDirectory = std::make_unique<TCHAR[]>(len);
			if (GetCurrentDirectory(len, originalCurrentDirectory.get()))
			{
				sOrigCWD = originalCurrentDirectory.get();
				sOrigCWD = CPathUtils::GetLongPathname(sOrigCWD);
			}
		}
		CFirstStartWizard wizard(IDS_APPNAME, CWnd::FromHandle(hWndExplorer), parser.GetLongVal(L"page"));
		theApp.m_pMainWnd = &wizard;
		return (wizard.DoModal() == ID_WIZFINISH);
	}

	if (!g_Git.CheckMsysGitDir())
	{
		UINT ret = CMessageBox::Show(hWndExplorer, IDS_PROC_NOMSYSGIT, IDS_APPNAME, 3, IDI_HAND, IDS_PROC_SETMSYSGITPATH, IDS_PROC_GOTOMSYSGITWEBSITE, IDS_ABORTBUTTON);
		if (ret == 2)
			ShellExecute(hWndExplorer, L"open", GIT_FOR_WINDOWS_URL, nullptr, nullptr, SW_SHOW);
		else if (ret == 1)
		{
			CFirstStartWizard wizard(IDS_APPNAME, CWnd::FromHandle(hWndExplorer), 2);
			theApp.m_pMainWnd = &wizard;
			wizard.DoModal();
		}
		return FALSE;
	}
	if (!CConfigureGitExe::CheckGitVersion(hWndExplorer))
		return FALSE;

	{
		CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": Registering Crash Report ...\n");
		CCrashReport::Instance().AddUserInfoToReport(L"msysGitDir", CGit::ms_LastMsysGitDir);
		CString versionString;
		versionString.Format(L"%X", CGit::ms_LastMsysGitVersion);
		CCrashReport::Instance().AddUserInfoToReport(L"msysGitVersion", versionString);
		if (CGit::ms_bCygwinGit)
			CCrashReport::Instance().AddUserInfoToReport(L"CygwinHack", L"true");
		if (CGit::ms_bMsys2Git)
			CCrashReport::Instance().AddUserInfoToReport(L"Msys2Hack", L"true");
	}

	if (parser.HasKey(L"urlhandler"))
	{
		CString url = parser.GetVal(L"urlhandler");
		if (CStringUtils::StartsWith(url, L"tgit://clone/"))
			url = url.Mid(static_cast<int>(wcslen(L"tgit://clone/")));
		else if (CStringUtils::StartsWith(url, L"github-windows://openRepo/"))
		{
			url = url.Mid(static_cast<int>(wcslen(L"github-windows://openRepo/")));
			int questioMark = url.Find('?');
			if (questioMark > 0)
				url = url.Left(questioMark);
		}
		else if (CStringUtils::StartsWith(url, L"x-github-client://openRepo/")) {
			url = url.Mid(static_cast<int>(wcslen(L"x-github-client://openRepo/")));
			int questioMark = url.Find('?');
			if (questioMark > 0)
				url = url.Left(questioMark);
		}
		else if (CStringUtils::StartsWith(url, L"smartgit://cloneRepo/"))
			url = url.Mid(static_cast<int>(wcslen(L"smartgit://cloneRepo/")));
		else if (!CStringUtils::StartsWith(url, L"git://"))
		{
			CMessageBox::Show(nullptr, IDS_ERR_INVALIDPATH, IDS_APPNAME, MB_ICONERROR);
			return FALSE;
		}
		CString newCmd;
		newCmd.Format(L"/command:clone /url:\"%s\" /hasurlhandler", static_cast<LPCTSTR>(url));
		parser = CCmdLineParser(newCmd);
	}

	if (parser.HasKey(L"path") && parser.HasKey(L"pathfile"))
	{
		CMessageBox::Show(nullptr, IDS_ERR_INVALIDPATH, IDS_APPNAME, MB_ICONERROR);
		return FALSE;
	}

	CTGitPath cmdLinePath;
	CTGitPathList pathList;
	if (g_sGroupingUUID.IsEmpty())
		g_sGroupingUUID = parser.GetVal(L"groupuuid");
	if (parser.HasKey(L"pathfile"))
	{
		CString sPathfileArgument = CPathUtils::GetLongPathname(parser.GetVal(L"pathfile"));

		cmdLinePath.SetFromUnknown(sPathfileArgument);
		if (pathList.LoadFromFile(cmdLinePath)==false)
			return FALSE;		// no path specified!
		if (parser.HasKey(L"deletepathfile"))
		{
			// We can delete the temporary path file, now that we've loaded it
			::DeleteFile(cmdLinePath.GetWinPath());
		}
		// This was a path to a temporary file - it's got no meaning now, and
		// anybody who uses it again is in for a problem...
		cmdLinePath.Reset();

	}
	else
	{
		CString sPathArgument = CPathUtils::GetLongPathname(parser.GetVal(L"path"));
		if (parser.HasKey(L"expaths"))
		{
			// an /expaths param means we're started via the buttons in our Win7 library
			// and that means the value of /expaths is the current directory, and
			// the selected paths are then added as additional parameters but without a key, only a value

			// because of the "strange treatment of quotation marks and backslashes by CommandLineToArgvW"
			// we have to escape the backslashes first. Since we're only dealing with paths here, that's
			// a save bet.
			// Without this, a command line like:
			// /command:commit /expaths:"D:\" "D:\Utils"
			// would fail because the "D:\" is treated as the backslash being the escape char for the quotation
			// mark and we'd end up with:
			// argv[1] = /command:commit
			// argv[2] = /expaths:D:" D:\Utils
			// See here for more details: http://blogs.msdn.com/b/oldnewthing/archive/2010/09/17/10063629.aspx
			CString cmdLine = GetCommandLineW();
			cmdLine.Replace(L"\\", L"\\\\");
			int nArgs = 0;
			LPWSTR *szArglist = CommandLineToArgvW(cmdLine, &nArgs);
			if (szArglist)
			{
				// argument 0 is the process path, so start with 1
				for (int i = 1; i < nArgs; ++i)
				{
					if (szArglist[i][0] != '/')
					{
						if (!sPathArgument.IsEmpty())
							sPathArgument += '*';
						sPathArgument += szArglist[i];
					}
				}
				sPathArgument.Replace(L"\\\\", L"\\");
			}
			LocalFree(szArglist);
		}
		if (sPathArgument.IsEmpty() && parser.HasKey(L"path"))
		{
			CMessageBox::Show(hWndExplorer, IDS_ERR_INVALIDPATH, IDS_APPNAME, MB_ICONERROR);
			return FALSE;
		}
		int asterisk = sPathArgument.Find('*');
		cmdLinePath.SetFromUnknown(asterisk >= 0 ? sPathArgument.Left(asterisk) : sPathArgument);
		pathList.LoadFromAsteriskSeparatedString(sPathArgument);
	}

	if (pathList.IsEmpty()) {
		pathList.AddPath(CTGitPath::CTGitPath(g_Git.m_CurrentDir));
	}

	// Set CWD to temporary dir, and restore it later
	{
		DWORD len = GetCurrentDirectory(0, nullptr);
		if (len)
		{
			auto originalCurrentDirectory = std::make_unique<TCHAR[]>(len);
			if (GetCurrentDirectory(len, originalCurrentDirectory.get()))
			{
				sOrigCWD = originalCurrentDirectory.get();
				sOrigCWD = CPathUtils::GetLongPathname(sOrigCWD);
			}
		}
		TCHAR pathbuf[MAX_PATH] = {0};
		GetTortoiseGitTempPath(_countof(pathbuf), pathbuf);
		SetCurrentDirectory(pathbuf);
	}

	CheckForNewerVersion();

	CAutoGeneralHandle TGitMutex = ::CreateMutex(nullptr, FALSE, L"TortoiseGitProc.exe");
	if (!g_Git.SetCurrentDir(cmdLinePath.GetWinPathString(), parser.HasKey(L"submodule") == TRUE))
	{
		for (int i = 0; i < pathList.GetCount(); ++i)
			if(g_Git.SetCurrentDir(pathList[i].GetWinPath()))
				break;
	}
	if (parser.HasKey(L"pathfile") && parser.HasKey(L"submodule"))
		g_Git.SetCurrentDir(pathList[0].GetWinPathString(), true);

	if(!g_Git.m_CurrentDir.IsEmpty())
	{
		sOrigCWD = g_Git.m_CurrentDir;
		SetCurrentDirectory(g_Git.m_CurrentDir);
	}

	if (g_sGroupingUUID.IsEmpty())
	{
		CRegStdDWORD groupSetting = CRegStdDWORD(L"Software\\TortoiseGit\\GroupTaskbarIconsPerRepo", 3);
		switch (DWORD(groupSetting))
		{
		case 1:
		case 2:
			// implemented differently to TortoiseSVN atm
			break;
		case 3:
		case 4:
			{
				CString wcroot;
				if (GitAdminDir::HasAdminDir(g_Git.m_CurrentDir, true, &wcroot))
				{
					git_oid oid;
					CStringA wcRootA(wcroot + CPathUtils::GetAppDirectory());
					if (!git_odb_hash(&oid, wcRootA.MakeLower(), wcRootA.GetLength(), GIT_OBJECT_BLOB))
					{
						CStringA hash;
						git_oid_tostr(CStrBufA(hash, GIT_OID_HEXSZ, CStrBufA::SET_LENGTH), GIT_OID_HEXSZ + 1, &oid);
						g_sGroupingUUID = hash;
					}
					ProjectProperties pp;
					pp.ReadProps();
					CString icon = pp.sIcon;
					icon.Replace('/', '\\');
					if (icon.IsEmpty())
						g_bGroupingRemoveIcon = true;
					g_sGroupingIcon = icon;
				}
			}
		}
	}

	CString sAppID = GetTaskIDPerUUID(g_sGroupingUUID).c_str();
	InitializeJumpList(sAppID);
	EnsureGitLibrary(false);

	{
		CString err;
		try
		{
			// requires CWD to be set
			CGit::m_LogEncode = CAppUtils::GetLogOutputEncode();

			// make sure all config files are read in order to check that none contains an error
			g_Git.GetConfigValue(L"doesnot.exist");
		}
		catch (char* msg)
		{
			err = CString(msg);
		}

		if (!err.IsEmpty())
		{
			UINT choice = CMessageBox::Show(hWndExplorer, err, L"TortoiseGit", 1, IDI_ERROR, CString(MAKEINTRESOURCE(IDS_PROC_EDITLOCALGITCONFIG)), CString(MAKEINTRESOURCE(IDS_PROC_EDITGLOBALGITCONFIG)), CString(MAKEINTRESOURCE(IDS_ABORTBUTTON)));
			if (choice == 1)
			{
				// open the config file with alternative editor
				CAppUtils::LaunchAlternativeEditor(g_Git.GetGitLocalConfig());
			}
			else if (choice == 2)
			{
				// open the global config file with alternative editor
				CAppUtils::LaunchAlternativeEditor(g_Git.GetGitGlobalConfig());
			}
			return FALSE;
		}
	}

	// execute the requested command
	CommandServer server;
	Command* cmd = server.GetCommand(parser.GetVal(L"command"));
	if (cmd)
	{
		cmd->SetExplorerHwnd(hWndExplorer);

		cmd->SetParser(parser);
		cmd->SetPaths(pathList, cmdLinePath);

		retSuccess = cmd->Execute();
		delete cmd;
	}

	// Look for temporary files left around by TortoiseSVN and
	// remove them. But only delete 'old' files because some
	// apps might still be needing the recent ones.
	{
		DWORD len = GetTortoiseGitTempPath(0, nullptr);
		auto path = std::make_unique<TCHAR[]>(len + 100);
		len = GetTortoiseGitTempPath (len + 100, path.get());
		if (len != 0)
		{
			CDirFileEnum finder(path.get());
			FILETIME systime_;
			::GetSystemTimeAsFileTime(&systime_);
			__int64 systime = static_cast<__int64>(systime_.dwHighDateTime) << 32 | systime_.dwLowDateTime;
			bool isDir;
			CString filepath;
			while (finder.NextFile(filepath, &isDir))
			{
				HANDLE hFile = ::CreateFile(filepath, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, isDir ? FILE_FLAG_BACKUP_SEMANTICS : 0, nullptr);
				if (hFile != INVALID_HANDLE_VALUE)
				{
					FILETIME createtime_;
					if (::GetFileTime(hFile, &createtime_, nullptr, nullptr))
					{
						::CloseHandle(hFile);
						__int64 createtime = static_cast<__int64>(createtime_.dwHighDateTime) << 32 | createtime_.dwLowDateTime;
						if ((createtime + 864000000000) < systime)		//only delete files older than a day
						{
							::SetFileAttributes(filepath, FILE_ATTRIBUTE_NORMAL);
							if (isDir)
								::RemoveDirectory(filepath);
							else
								::DeleteFile(filepath);
						}
					}
					else
						::CloseHandle(hFile);
				}
			}
		}
	}

	Animator::Instance().ShutDown();

	// Since the dialog has been closed, return FALSE so that we exit the
	// application, rather than start the application's message pump.
	return FALSE;
}

void CTortoiseProcApp::CheckUpgrade()
{
	CRegString regVersion = CRegString(L"Software\\TortoiseGit\\CurrentVersion");
	CString sVersion = regVersion;
	CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": Current TGit Version %s\n", static_cast<LPCTSTR>(sVersion));
	if (sVersion.Compare(_T(STRPRODUCTVER))==0)
		return;
	// we're starting the first time with a new version!

	LONG lVersion = 0;
	int pos = sVersion.Find('.');
	if (pos > 0)
	{
		lVersion = (_wtol(sVersion.Left(pos)) << 24);
		lVersion |= (_wtol(sVersion.Mid(pos + 1)) << 16);
		pos = sVersion.Find('.', pos+1);
		lVersion |= (_wtol(sVersion.Mid(pos + 1)) << 8);
	}
	else
	{
		pos = sVersion.Find(',');
		if (pos > 0)
		{
			lVersion = (_wtol(sVersion.Left(pos)) << 24);
			lVersion |= (_wtol(sVersion.Mid(pos + 1)) << 16);
			pos = sVersion.Find(',', pos+1);
			lVersion |= (_wtol(sVersion.Mid(pos + 1)) << 8);
		}
	}

	// generic cleanup
	if (CRegStdDWORD(L"Software\\TortoiseGit\\UseLibgit2", TRUE) != TRUE)
	{
		if (CMessageBox::Show(nullptr, L"You have disabled the usage of libgit2 in TortoiseGit.\n\nThis might be the case in order to resolve an issue in an older TortoiseGit version.\n\nDo you want to restore the default value (i.e., enable it)?", L"TortoiseGit", MB_ICONQUESTION | MB_YESNO) == IDYES)
			CRegStdDWORD(L"Software\\TortoiseGit\\UseLibgit2").removeValue();
	}

	if (CRegStdDWORD(L"Software\\TortoiseGit\\UseLibgit2_mask").exists())
	{
		if (CMessageBox::Show(nullptr, L"You have a non-default setting of UseLibgit2_mask in your registry.\n\nThis might be the case in order to resolve an issue in an older TortoiseGit version.\n\nDo you want to restore the default value (i.e., remove custom setting from registry)?", L"TortoiseGit", MB_ICONQUESTION | MB_YESNO) == IDYES)
			CRegStdDWORD(L"Software\\TortoiseGit\\UseLibgit2_mask").removeValue();
	}

	CMessageBox::RemoveRegistryKey(L"OldMsysgitVersionWarning");

	CRegDWORD checkNewerWeekDay = CRegDWORD(L"Software\\TortoiseGit\\CheckNewerWeekDay", 0);
	if (!checkNewerWeekDay.exists() || lVersion <= ConvertVersionToInt(1, 8, 16))
	{
		std::random_device rd;
		std::mt19937 mt(rd());
		std::uniform_int_distribution<int> dist(0, 6);
		checkNewerWeekDay = dist(mt);
	}

	// version specific updates
	if (lVersion <= ConvertVersionToInt(2, 4, 1))
	{
		CRegStdDWORD(L"Software\\TortoiseGit\\CommitAskBeforeCancel").removeValue();
	}

	if (lVersion <= ConvertVersionToInt(2, 4, 0))
	{
		CRegStdDWORD commmitAskBeforeCancel(L"Software\\TortoiseGit\\CommitAskBeforeCancel");
		if (commmitAskBeforeCancel.exists() && commmitAskBeforeCancel != IDYES)
			commmitAskBeforeCancel = IDYES;
	}

	if (lVersion <= ConvertVersionToInt(2, 2, 1))
	{
		CString username = CRegString(L"Software\\TortoiseGit\\TortoiseProc\\SendMail\\Username", L"");
		CString password = CRegString(L"Software\\TortoiseGit\\TortoiseProc\\SendMail\\Password", L"");
		if (!username.IsEmpty() && !password.IsEmpty())
		{
			if (CWindowsCredentialsStore::SaveCredential(L"TortoiseGit:SMTP-Credentials", username, password) == 0)
			{
				CRegString(L"Software\\TortoiseGit\\TortoiseProc\\SendMail\\Username").removeValue();
				CRegString(L"Software\\TortoiseGit\\TortoiseProc\\SendMail\\Password").removeValue();
			}
		}
		for (const CString& setting : { L"SyncIn", L"SyncOut" })
		{
			CRegDWORD reg(L"Software\\TortoiseGit\\StatusColumns\\" + setting + L"loglistVersion", 0xff);
			if (reg == 6)
				reg.removeValue();
		}
	}

	if (lVersion <= ConvertVersionToInt(2, 1, 5))
	{
		// We updated GITSLC_COL_VERSION, but only significant changes were made for GitStatusList
		// so, smoothly migrate GitLoglistBase settings
		for (const CString& setting : { L"log", L"Blame", L"Rebase", L"reflog", L"SyncIn", L"SyncOut" })
		{
			CRegDWORD reg(L"Software\\TortoiseGit\\StatusColumns\\" + setting + L"loglistVersion", 0xff);
			if (reg == 5)
				reg = 6;
		}
	}

	if (lVersion <= ConvertVersionToInt(1, 9, 0))
	{
		if (CRegDWORD(L"Software\\TortoiseGit\\TGitCacheCheckContent", TRUE) == FALSE)
		{
			CRegDWORD(L"Software\\TortoiseGit\\TGitCacheCheckContentMaxSize") = 0;
			CRegDWORD(L"Software\\TortoiseGit\\TGitCacheCheckContent").removeValue();
		}
	}

	if (lVersion <= ConvertVersionToInt(1, 8, 8, 1))
	{
		CRegStdDWORD(L"Software\\TortoiseGit\\StatusColumns\\BrowseRefs").removeValue();
		CRegStdString(L"Software\\TortoiseGit\\StatusColumns\\BrowseRefs_Order").removeValue();
		CRegStdString(L"Software\\TortoiseGit\\StatusColumns\\BrowseRefs_Width").removeValue();
	}

	if (lVersion <= ConvertVersionToInt(1, 8, 4, 1))
	{
		if (CRegStdDWORD(L"Software\\TortoiseGit\\TortoiseProc\\SendMail\\UseMAPI", FALSE) == TRUE)
			CRegStdDWORD(L"Software\\TortoiseGit\\TortoiseProc\\SendMail\\DeliveryType") = SEND_MAIL_MAPI;
		CRegStdDWORD(L"Software\\TortoiseGit\\TortoiseProc\\SendMail\\UseMAPI").removeValue();
	}

	if (lVersion <= ConvertVersionToInt(1, 8, 2, 2))
	{
		// upgrade to 1.8.3: force recreation of all diff scripts.
		CAppUtils::SetupDiffScripts(true, CString());
	}

	if (lVersion <= ConvertVersionToInt(1, 8, 1))
	{
		if (CRegStdDWORD(L"Software\\TortoiseGit\\LogTopoOrder", TRUE) == FALSE)
			CRegStdDWORD(L"Software\\TortoiseGit\\LogOrderBy") = 0;

		// smoothly migrate broken msysgit path settings
		CString oldmsysGitSetting = CRegString(REG_MSYSGIT_PATH);
		oldmsysGitSetting.TrimRight(L'\\');
		if (oldmsysGitSetting.GetLength() > 4 && CStringUtils::EndsWith(oldmsysGitSetting, L"\\cmd"))
		{
			CString newPath = oldmsysGitSetting.Left(oldmsysGitSetting.GetLength() - 3) + L"bin";
			if (PathFileExists(newPath + L"\\git.exe"))
			{
				CRegString(REG_MSYSGIT_PATH) = newPath;
				g_Git.m_bInitialized = FALSE;
				g_Git.CheckMsysGitDir();
			}
		}
	}

	if (lVersion <= ConvertVersionToInt(1, 4, 0))
	{
		CRegStdDWORD(L"Software\\TortoiseGit\\OwnerdrawnMenus").removeValue();
	}

	if (lVersion <= ConvertVersionToInt(1, 7, 6))
	{
		CoInitialize(nullptr);
		EnsureGitLibrary();
		CoUninitialize();
		CRegStdDWORD(L"Software\\TortoiseGit\\ConvertBase").removeValue();
		CRegStdDWORD(L"Software\\TortoiseGit\\DiffProps").removeValue();
		if (CRegStdDWORD(L"Software\\TortoiseGit\\CheckNewer", TRUE) == FALSE)
			CRegStdDWORD(L"Software\\TortoiseGit\\VersionCheck") = FALSE;
		CRegStdDWORD(L"Software\\TortoiseGit\\CheckNewer").removeValue();
	}

	CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": Setting up diff scripts ...\n");
	CAppUtils::SetupDiffScripts(false, CString());

	// set the current version so we don't come here again until the next update!
	regVersion = _T(STRPRODUCTVER);
}

void CTortoiseProcApp::InitializeJumpList(const CString& appid)
{
	// for Win7 : use a custom jump list
	CoInitialize(nullptr);
	SetAppID(appid);
	DeleteJumpList(appid);
	DoInitializeJumpList(appid);
	CoUninitialize();
}

void CTortoiseProcApp::DoInitializeJumpList(const CString& appid)
{
	ATL::CComPtr<ICustomDestinationList> pcdl;
	HRESULT hr = pcdl.CoCreateInstance(CLSID_DestinationList, nullptr, CLSCTX_INPROC_SERVER);
	if (FAILED(hr))
		return;

	hr = pcdl->SetAppID(appid);
	if (FAILED(hr))
		return;

	UINT uMaxSlots;
	ATL::CComPtr<IObjectArray> poaRemoved;
	hr = pcdl->BeginList(&uMaxSlots, IID_PPV_ARGS(&poaRemoved));
	if (FAILED(hr))
		return;

	ATL::CComPtr<IObjectCollection> poc;
	hr = poc.CoCreateInstance(CLSID_EnumerableObjectCollection, nullptr, CLSCTX_INPROC_SERVER);
	if (FAILED(hr))
		return;

	CString sTemp = CString(MAKEINTRESOURCE(IDS_MENUSETTINGS));
	CStringUtils::RemoveAccelerators(sTemp);

	ATL::CComPtr<IShellLink> psl;
	hr = CreateShellLink(L"/command:settings", static_cast<LPCTSTR>(sTemp), 18, &psl);
	if (SUCCEEDED(hr)) {
		poc->AddObject(psl);
	}
	sTemp = CString(MAKEINTRESOURCE(IDS_MENUHELP));
	CStringUtils::RemoveAccelerators(sTemp);
	psl.Release(); // Need to release the object before calling operator&()
	hr = CreateShellLink(L"/command:help", static_cast<LPCTSTR>(sTemp), 17, &psl);
	if (SUCCEEDED(hr)) {
		poc->AddObject(psl);
	}

	ATL::CComPtr<IObjectArray> poa;
	hr = poc.QueryInterface(&poa);
	if (SUCCEEDED(hr)) {
		pcdl->AppendCategory(static_cast<LPCTSTR>(CString(MAKEINTRESOURCE(IDS_PROC_TASKS))), poa);
		pcdl->CommitList();
	}
}

int CTortoiseProcApp::ExitInstance()
{
	SYS_IMAGE_LIST().Cleanup();
	Gdiplus::GdiplusShutdown(m_gdiplusToken);

	CWinAppEx::ExitInstance();
	if (retSuccess)
		return 0;
	return -1;
}

void CTortoiseProcApp::CheckForNewerVersion()
{
	// check for newer versions
	if (CRegDWORD(L"Software\\TortoiseGit\\VersionCheck", TRUE) != FALSE)
	{
		time_t now;
		struct tm ptm;

		time(&now);
		if ((now != 0) && (localtime_s(&ptm, &now)==0))
		{
#if PREVIEW
			// Check daily for new preview releases
			CRegDWORD oldday = CRegDWORD(L"Software\\TortoiseGit\\CheckNewerDay", DWORD(-1));
			if (oldday == -1)
				oldday = ptm.tm_yday;
			else
			{
				if (static_cast<DWORD>(oldday) != static_cast<DWORD>(ptm.tm_yday))
				{
					oldday = ptm.tm_yday;
#else
			int week = 0;
			// we don't calculate the real 'week of the year' here
			// because just to decide if we should check for an update
			// that's not needed.
			week = (ptm.tm_yday + CRegDWORD(L"Software\\TortoiseGit\\CheckNewerWeekDay", 0)) / 7;

			CRegDWORD oldweek = CRegDWORD(L"Software\\TortoiseGit\\CheckNewerWeek", DWORD(-1));
			if (oldweek == -1)
				oldweek = week;		// first start of TortoiseProc, no update check needed
			else
			{
				if (static_cast<DWORD>(week) != oldweek)
				{
					oldweek = week;
#endif
					CAppUtils::RunTortoiseGitProc(L"/command:updatecheck", false, false);
				}
			}
		}
	}
}
