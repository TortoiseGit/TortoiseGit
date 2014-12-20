// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2014 - TortoiseGit
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
#include "..\Utils\CrashReport.h"
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
#include "..\version.h"
#include "JumpListHelpers.h"
#include "SinglePropSheetDlg.h"
#include "Settings\setmainpage.h"
#include "Libraries.h"
#include "TaskbarUUID.h"
#include "ProjectProperties.h"
#include "HistoryCombo.h"
#include "gitindex.h"
#include <math.h>

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
{
	CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) _T(": Constructor\n"));
	SetDllDirectory(L"");
	// prevent from inheriting %GIT_DIR% from parent process by resetting it,
	// use MSVC function instead of Windows API because MSVC runtime caches environment variables
	_tputenv(_T("GIT_DIR="));
	CCrashReport::Instance().AddUserInfoToReport(L"CommandLine", GetCommandLine());
	EnableHtmlHelp();
	SYS_IMAGE_LIST();
	CHooks::Create();
	git_libgit2_init();
	CGit::SetGit2CredentialCallback(CAppUtils::Git2GetUserPassword);
	CGit::SetGit2CertificateCheckCertificate(CAppUtils::Git2CertificateCheck);
	m_bLoadUserToolbars = FALSE;
	m_bSaveState = FALSE;
	retSuccess = false;
	m_gdiplusToken = NULL;
#if defined (_WIN64) && _MSC_VER >= 1800
	_set_FMA3_enable(0);
#endif
}

CTortoiseProcApp::~CTortoiseProcApp()
{
	CHooks::Destroy();
	SYS_IMAGE_LIST().Cleanup();
	git_libgit2_shutdown();
}

// The one and only CTortoiseProcApp object
CTortoiseProcApp theApp;
CString sOrigCWD;
CString g_sGroupingUUID;
CString g_sGroupingIcon;
bool g_bGroupingRemoveIcon = false;
HWND hWndExplorer;
CGitIndexFileMap g_IndexFileMap;

#if ENABLE_CRASHHANLDER
CCrashReportTGit crasher(L"TortoiseGit " _T(APP_X64_STRING), TGIT_VERMAJOR, TGIT_VERMINOR, TGIT_VERMICRO, TGIT_VERBUILD, TGIT_VERDATE);
#endif

// CTortoiseProcApp initialization

BOOL CTortoiseProcApp::InitInstance()
{
	CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) _T(": InitInstance\n"));
	CheckUpgrade();
	CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerWindows));
	CMFCButton::EnableWindowsTheming();
	CHistoryCombo::m_nGitIconIndex = SYS_IMAGE_LIST().AddIcon((HICON)LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_GITCONFIG), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE));

	Gdiplus::GdiplusStartupInput gdiplusStartupInput;
	Gdiplus::GdiplusStartup(&m_gdiplusToken,&gdiplusStartupInput,NULL);

	//set the resource dll for the required language
	CRegDWORD loc = CRegDWORD(_T("Software\\TortoiseGit\\LanguageID"), 1033);
	long langId = loc;
	{
		CString langStr;
		langStr.Format(_T("%ld"), langId);
		CCrashReport::Instance().AddUserInfoToReport(L"LanguageID", langStr);
	}
	CString langDll;
	CStringA langpath = CStringA(CPathUtils::GetAppParentDirectory());
	langpath += "Languages";
	do
	{
		langDll.Format(_T("%sLanguages\\TortoiseProc%ld.dll"), (LPCTSTR)CPathUtils::GetAppParentDirectory(), langId);

		CString sVer = _T(STRPRODUCTVER);
		CString sFileVer = CPathUtils::GetVersionFromFile(langDll);
		if (sFileVer == sVer)
		{
			HINSTANCE hInst = LoadLibrary(langDll);
			if (hInst != NULL)
			{
				CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) _T(": Load Language DLL %s\n"), langDll);
				AfxSetResourceHandle(hInst);
				break;
			}
		}
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
	} while (langId != 0);
	TCHAR buf[6] = { 0 };
	_tcscpy_s(buf, _T("en"));
	langId = loc;
	// MFC uses a help file with the same name as the application by default,
	// which means we have to change that default to our language specific help files
	CString sHelppath = CPathUtils::GetAppDirectory() + _T("TortoiseGit_en.chm");
	free((void*)m_pszHelpFilePath);
	m_pszHelpFilePath=_tcsdup(sHelppath);
	sHelppath = CPathUtils::GetAppParentDirectory() + _T("Languages\\TortoiseGit_en.chm");
	do
	{
		CString sLang = _T("_");
		if (GetLocaleInfo(MAKELCID(langId, SORT_DEFAULT), LOCALE_SISO639LANGNAME, buf, _countof(buf)))
		{
			sLang += buf;
			sHelppath.Replace(_T("_en"), sLang);
			if (PathFileExists(sHelppath))
			{
				free((void*)m_pszHelpFilePath);
				m_pszHelpFilePath=_tcsdup(sHelppath);
				break;
			}
		}
		sHelppath.Replace(sLang, _T("_en"));
		if (GetLocaleInfo(MAKELCID(langId, SORT_DEFAULT), LOCALE_SISO3166CTRYNAME, buf, _countof(buf)))
		{
			sLang += _T("_");
			sLang += buf;
			sHelppath.Replace(_T("_en"), sLang);
			if (PathFileExists(sHelppath))
			{
				free((void*)m_pszHelpFilePath);
				m_pszHelpFilePath=_tcsdup(sHelppath);
				break;
			}
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
	CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) _T(": Set Help Filename %s\n"), m_pszHelpFilePath);
	setlocale(LC_ALL, "");

	if (!g_Git.CheckMsysGitDir())
	{
		UINT ret = CMessageBox::Show(NULL, IDS_PROC_NOMSYSGIT, IDS_APPNAME, 3, IDI_HAND, IDS_PROC_SETMSYSGITPATH, IDS_PROC_GOTOMSYSGITWEBSITE, IDS_ABORTBUTTON);
		if(ret == 2)
		{
			ShellExecute(NULL, NULL, _T("http://msysgit.github.io/"), NULL, NULL, SW_SHOW);
		}
		else if(ret == 1)
		{
			// open settings dialog
			CSinglePropSheetDlg(CString(MAKEINTRESOURCE(IDS_PROC_SETTINGS_TITLE)), new CSetMainPage(), this->GetMainWnd()).DoModal();
		}
		return FALSE;
	}
	if (CAppUtils::GetMsysgitVersion() < 0x01070a00)
	{
		int ret = CMessageBox::ShowCheck(NULL, IDS_PROC_OLDMSYSGIT, IDS_APPNAME, 1, IDI_EXCLAMATION, IDS_PROC_GOTOMSYSGITWEBSITE, IDS_ABORTBUTTON, IDS_IGNOREBUTTON, _T("OldMsysgitVersionWarning"), IDS_PROC_NOTSHOWAGAINIGNORE);
		if (ret == 1)
		{
			CMessageBox::RemoveRegistryKey(_T("OldMsysgitVersionWarning")); // only store answer if it is "Ignore"
			ShellExecute(NULL, NULL, _T("http://msysgit.github.io/"), NULL, NULL, SW_SHOW);
			return FALSE;
		}
		else if (ret == 2)
		{
			CMessageBox::RemoveRegistryKey(_T("OldMsysgitVersionWarning")); // only store answer if it is "Ignore"
			return FALSE;
		}
	}

	{
		CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) _T(": Registering Crash Report ...\n"));
		CCrashReport::Instance().AddUserInfoToReport(L"msysGitDir", CGit::ms_LastMsysGitDir);
		CString versionString;
		versionString.Format(_T("%d"), CGit::ms_LastMsysGitVersion);
		CCrashReport::Instance().AddUserInfoToReport(L"msysGitVersion", versionString);
	}

	CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) _T(": Initializing UI components ...\n"));
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
	SetRegistryKey(_T("TortoiseGit"));
	AfxGetApp()->m_pszProfileName = _tcsdup(_T("TortoiseProc")); // w/o this ResizableLib will store data under TortoiseGitProc which is not compatible with older versions

	CCmdLineParser parser(AfxGetApp()->m_lpCmdLine);

	hWndExplorer = NULL;
	CString sVal = parser.GetVal(_T("hwnd"));
	if (!sVal.IsEmpty())
		hWndExplorer = (HWND)_wcstoui64(sVal, nullptr, 16);

	while (GetParent(hWndExplorer)!=NULL)
		hWndExplorer = GetParent(hWndExplorer);
	if (!IsWindow(hWndExplorer))
	{
		hWndExplorer = NULL;
	}

	// if HKCU\Software\TortoiseGit\Debug is not 0, show our command line
	// in a message box
	if (CRegDWORD(_T("Software\\TortoiseGit\\Debug"), FALSE)==TRUE)
		AfxMessageBox(AfxGetApp()->m_lpCmdLine, MB_OK | MB_ICONINFORMATION);

	if (parser.HasKey(_T("urlhandler")))
	{
		CString url = parser.GetVal(_T("urlhandler"));
		if (url.Find(_T("tgit://clone/")) == 0)
		{
			url = url.Mid(13); // 21 = "tgit://clone/".GetLength()
		}
		else if (url.Find(_T("github-windows://openRepo/")) == 0)
		{
			url = url.Mid(26); // 26 = "github-windows://openRepo/".GetLength()
			int questioMark = url.Find('?');
			if (questioMark > 0)
				url = url.Left(questioMark);
		}
		else if (url.Find(_T("smartgit://cloneRepo/")) == 0)
		{
			url = url.Mid(21); // 21 = "smartgit://cloneRepo/".GetLength()
		}
		else
		{
			CMessageBox::Show(NULL, IDS_ERR_INVALIDPATH, IDS_APPNAME, MB_ICONERROR);
			return FALSE;
		}
		CString newCmd;
		newCmd.Format(_T("/command:clone /url:\"%s\" /hasurlhandler"), url);
		parser = CCmdLineParser(newCmd);
	}

	if ( parser.HasKey(_T("path")) && parser.HasKey(_T("pathfile")))
	{
		CMessageBox::Show(NULL, IDS_ERR_INVALIDPATH, IDS_APPNAME, MB_ICONERROR);
		return FALSE;
	}

	CTGitPath cmdLinePath;
	CTGitPathList pathList;
	if (g_sGroupingUUID.IsEmpty())
		g_sGroupingUUID = parser.GetVal(L"groupuuid");
	if ( parser.HasKey(_T("pathfile")) )
	{

		CString sPathfileArgument = CPathUtils::GetLongPathname(parser.GetVal(_T("pathfile")));

		cmdLinePath.SetFromUnknown(sPathfileArgument);
		if (pathList.LoadFromFile(cmdLinePath)==false)
			return FALSE;		// no path specified!
		if ( parser.HasKey(_T("deletepathfile")) )
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

		CString sPathArgument = CPathUtils::GetLongPathname(parser.GetVal(_T("path")));
		if (parser.HasKey(_T("expaths")))
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
		DWORD len = GetCurrentDirectory(0, NULL);
		if (len)
		{
			std::unique_ptr<TCHAR[]> originalCurrentDirectory(new TCHAR[len]);
			if (GetCurrentDirectory(len, originalCurrentDirectory.get()))
			{
				sOrigCWD = originalCurrentDirectory.get();
				sOrigCWD = CPathUtils::GetLongPathname(sOrigCWD);
			}
		}
		TCHAR pathbuf[MAX_PATH] = {0};
		GetTortoiseGitTempPath(MAX_PATH, pathbuf);
		SetCurrentDirectory(pathbuf);
	}

	CheckForNewerVersion();

	CAutoGeneralHandle TGitMutex = ::CreateMutex(NULL, FALSE, _T("TortoiseGitProc.exe"));
	if (!g_Git.SetCurrentDir(cmdLinePath.GetWinPathString(), parser.HasKey(_T("submodule")) == TRUE))
	{
		for (int i = 0; i < pathList.GetCount(); ++i)
			if(g_Git.SetCurrentDir(pathList[i].GetWinPath()))
				break;
	}

	if(!g_Git.m_CurrentDir.IsEmpty())
	{
		sOrigCWD = g_Git.m_CurrentDir;
		SetCurrentDirectory(g_Git.m_CurrentDir);
	}

	if (g_sGroupingUUID.IsEmpty())
	{
		CRegStdDWORD groupSetting = CRegStdDWORD(_T("Software\\TortoiseGit\\GroupTaskbarIconsPerRepo"), 3);
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
				if (g_GitAdminDir.HasAdminDir(g_Git.m_CurrentDir, true, &wcroot))
				{
					git_oid oid;
					CStringA wcRootA(wcroot + CPathUtils::GetAppDirectory());
					if (!git_odb_hash(&oid, wcRootA, wcRootA.GetLength(), GIT_OBJ_BLOB))
					{
						CStringA hash;
						git_oid_tostr(hash.GetBufferSetLength(GIT_OID_HEXSZ + 1), GIT_OID_HEXSZ + 1, &oid);
						hash.ReleaseBuffer();
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
			g_Git.GetConfigValue(_T("doesnot.exist"));
		}
		catch (char* msg)
		{
			err = CString(msg);
		}

		if (!err.IsEmpty())
		{
			UINT choice = CMessageBox::Show(hWndExplorer, err, _T("TortoiseGit"), 1, IDI_ERROR, CString(MAKEINTRESOURCE(IDS_PROC_EDITLOCALGITCONFIG)), CString(MAKEINTRESOURCE(IDS_PROC_EDITGLOBALGITCONFIG)), CString(MAKEINTRESOURCE(IDS_ABORTBUTTON)));
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
	Command * cmd = server.GetCommand(parser.GetVal(_T("command")));
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
		DWORD len = GetTortoiseGitTempPath(0, NULL);
		std::unique_ptr<TCHAR[]> path(new TCHAR[len + 100]);
		len = GetTortoiseGitTempPath (len + 100, path.get());
		if (len != 0)
		{
			CDirFileEnum finder(path.get());
			FILETIME systime_;
			::GetSystemTimeAsFileTime(&systime_);
			__int64 systime = (((_int64)systime_.dwHighDateTime)<<32) | ((__int64)systime_.dwLowDateTime);
			bool isDir;
			CString filepath;
			while (finder.NextFile(filepath, &isDir))
			{
				HANDLE hFile = ::CreateFile(filepath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, isDir ? FILE_FLAG_BACKUP_SEMANTICS : NULL, NULL);
				if (hFile != INVALID_HANDLE_VALUE)
				{
					FILETIME createtime_;
					if (::GetFileTime(hFile, &createtime_, NULL, NULL))
					{
						::CloseHandle(hFile);
						__int64 createtime = (((_int64)createtime_.dwHighDateTime)<<32) | ((__int64)createtime_.dwLowDateTime);
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

	// Since the dialog has been closed, return FALSE so that we exit the
	// application, rather than start the application's message pump.
	return FALSE;
}

void CTortoiseProcApp::CheckUpgrade()
{
	CRegString regVersion = CRegString(_T("Software\\TortoiseGit\\CurrentVersion"));
	CString sVersion = regVersion;
	CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) _T(": Current TGit Version %s\n"), sVersion);
	if (sVersion.Compare(_T(STRPRODUCTVER))==0)
		return;
	// we're starting the first time with a new version!

	LONG lVersion = 0;
	int pos = sVersion.Find('.');
	if (pos > 0)
	{
		lVersion = (_ttol(sVersion.Left(pos))<<24);
		lVersion |= (_ttol(sVersion.Mid(pos+1))<<16);
		pos = sVersion.Find('.', pos+1);
		lVersion |= (_ttol(sVersion.Mid(pos+1))<<8);
	}
	else
	{
		pos = sVersion.Find(',');
		if (pos > 0)
		{
			lVersion = (_ttol(sVersion.Left(pos))<<24);
			lVersion |= (_ttol(sVersion.Mid(pos+1))<<16);
			pos = sVersion.Find(',', pos+1);
			lVersion |= (_ttol(sVersion.Mid(pos+1))<<8);
		}
	}

	// generic cleanup
	if (CRegStdDWORD(_T("Software\\TortoiseGit\\UseLibgit2"), TRUE) != TRUE)
	{
		if (CMessageBox::Show(nullptr, _T("You have disabled the usage of libgit2 in TortoiseGit.\n\nThis might be the case in order to resolve an issue in an older TortoiseGit version.\n\nDo you want to restore the default value (i.e., enable it)?"), _T("TortoiseGit"), MB_ICONQUESTION | MB_YESNO) == IDYES)
			CRegStdDWORD(_T("Software\\TortoiseGit\\UseLibgit2")).removeValue();
	}

	if (CRegStdDWORD(_T("Software\\TortoiseGit\\UseLibgit2_mask")).exists())
	{
		if (CMessageBox::Show(nullptr, _T("You have a non-default setting of UseLibgit2_mask in your registry.\n\nThis might be the case in order to resolve an issue in an older TortoiseGit version.\n\nDo you want to restore the default value (i.e., remove custom setting from registry)?"), _T("TortoiseGit"), MB_ICONQUESTION | MB_YESNO) == IDYES)
			CRegStdDWORD(_T("Software\\TortoiseGit\\UseLibgit2_mask")).removeValue();
	}

	// version specific updates
	if (lVersion <= 0x01080802)
	{
		CRegStdDWORD(_T("Software\\TortoiseGit\\TortoiseProc\\ResizableState\\CleanTypeDlgWindowPlacement")).removeValue();
	}

	if (lVersion <= 0x01080801)
	{
		CRegStdDWORD(_T("Software\\TortoiseGit\\StatusColumns\\BrowseRefs")).removeValue();
		CRegStdString(_T("Software\\TortoiseGit\\StatusColumns\\BrowseRefs_Order")).removeValue();
		CRegStdString(_T("Software\\TortoiseGit\\StatusColumns\\BrowseRefs_Width")).removeValue();
	}

	if (lVersion <= 0x01080401)
	{
		if (CRegStdDWORD(_T("Software\\TortoiseGit\\TortoiseProc\\SendMail\\UseMAPI"), FALSE) == TRUE)
			CRegStdDWORD(_T("Software\\TortoiseGit\\TortoiseProc\\SendMail\\DeliveryType")) = 1;
		CRegStdDWORD(_T("Software\\TortoiseGit\\TortoiseProc\\SendMail\\UseMAPI")).removeValue();
	}

	if (lVersion <= 0x01080202)
	{
		// upgrade to 1.8.3: force recreation of all diff scripts.
		CAppUtils::SetupDiffScripts(true, CString());
	}

	if (lVersion <= 0x01080100)
	{
		if (CRegStdDWORD(_T("Software\\TortoiseGit\\LogTopoOrder"), TRUE) == FALSE)
			CRegStdDWORD(_T("Software\\TortoiseGit\\LogOrderBy")) = 0;

		// smoothly migrate broken msysgit path settings
		CString oldmsysGitSetting = CRegString(REG_MSYSGIT_PATH);
		oldmsysGitSetting.TrimRight(_T("\\"));
		CString right = oldmsysGitSetting.Right(4);
		if (oldmsysGitSetting.GetLength() > 4 && oldmsysGitSetting.Right(4) == _T("\\cmd"))
		{
			CString newPath = oldmsysGitSetting.Mid(0, oldmsysGitSetting.GetLength() - 3) + _T("bin");
			if (PathFileExists(newPath + _T("\\git.exe")))
			{
				CRegString(REG_MSYSGIT_PATH) = newPath;
				g_Git.m_bInitialized = FALSE;
				g_Git.CheckMsysGitDir();
			}
		}
	}

	if (lVersion <= 0x01040000)
	{
		CRegStdDWORD(_T("Software\\TortoiseGit\\OwnerdrawnMenus")).removeValue();
	}

	if (lVersion <= 0x01070600)
	{
		CoInitialize(NULL);
		EnsureGitLibrary();
		CoUninitialize();
		CRegStdDWORD(_T("Software\\TortoiseGit\\ConvertBase")).removeValue();
		CRegStdDWORD(_T("Software\\TortoiseGit\\DiffProps")).removeValue();
		if (CRegStdDWORD(_T("Software\\TortoiseGit\\CheckNewer"), TRUE) == FALSE)
			CRegStdDWORD(_T("Software\\TortoiseGit\\VersionCheck")) = FALSE;
		CRegStdDWORD(_T("Software\\TortoiseGit\\CheckNewer")).removeValue();
	}

	CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) _T(": Setting up diff scripts ...\n"));
	CAppUtils::SetupDiffScripts(false, CString());

	// set the current version so we don't come here again until the next update!
	regVersion = _T(STRPRODUCTVER);
}

void CTortoiseProcApp::InitializeJumpList(const CString& appid)
{
	// for Win7 : use a custom jump list
	CoInitialize(NULL);
	SetAppID(appid);
	DeleteJumpList(appid);
	DoInitializeJumpList(appid);
	CoUninitialize();
}

void CTortoiseProcApp::DoInitializeJumpList(const CString& appid)
{
	ATL::CComPtr<ICustomDestinationList> pcdl;
	HRESULT hr = pcdl.CoCreateInstance(CLSID_DestinationList, NULL, CLSCTX_INPROC_SERVER);
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
	hr = poc.CoCreateInstance(CLSID_EnumerableObjectCollection, NULL, CLSCTX_INPROC_SERVER);
	if (FAILED(hr))
		return;

	CString sTemp = CString(MAKEINTRESOURCE(IDS_MENUSETTINGS));
	CStringUtils::RemoveAccelerators(sTemp);

	ATL::CComPtr<IShellLink> psl;
	hr = CreateShellLink(_T("/command:settings"), (LPCTSTR)sTemp, 20, &psl);
	if (SUCCEEDED(hr)) {
		poc->AddObject(psl);
	}
	sTemp = CString(MAKEINTRESOURCE(IDS_MENUHELP));
	CStringUtils::RemoveAccelerators(sTemp);
	psl.Release(); // Need to release the object before calling operator&()
	hr = CreateShellLink(_T("/command:help"), (LPCTSTR)sTemp, 19, &psl);
	if (SUCCEEDED(hr)) {
		poc->AddObject(psl);
	}

	ATL::CComPtr<IObjectArray> poa;
	hr = poc.QueryInterface(&poa);
	if (SUCCEEDED(hr)) {
		pcdl->AppendCategory((LPCTSTR)CString(MAKEINTRESOURCE(IDS_PROC_TASKS)), poa);
		pcdl->CommitList();
	}
}

int CTortoiseProcApp::ExitInstance()
{
	Gdiplus::GdiplusShutdown(m_gdiplusToken);

	CWinAppEx::ExitInstance();
	if (retSuccess)
		return 0;
	return -1;
}

void CTortoiseProcApp::CheckForNewerVersion()
{
	// check for newer versions
	if (CRegDWORD(_T("Software\\TortoiseGit\\VersionCheck"), TRUE) != FALSE)
	{
		time_t now;
		struct tm ptm;

		time(&now);
		if ((now != 0) && (localtime_s(&ptm, &now)==0))
		{
#if PREVIEW
			// Check daily for new preview releases
			CRegDWORD oldday = CRegDWORD(_T("Software\\TortoiseGit\\CheckNewerDay"), (DWORD)-1);
			if (((DWORD)oldday) == -1)
				oldday = ptm.tm_yday;
			else
			{
				if ((DWORD)oldday != (DWORD)ptm.tm_yday)
				{
					oldday = ptm.tm_yday;
#else
			int week = 0;
			// we don't calculate the real 'week of the year' here
			// because just to decide if we should check for an update
			// that's not needed.
			week = ptm.tm_yday / 7;

			CRegDWORD oldweek = CRegDWORD(_T("Software\\TortoiseGit\\CheckNewerWeek"), (DWORD)-1);
			if (((DWORD)oldweek) == -1)
				oldweek = week;		// first start of TortoiseProc, no update check needed
			else
			{
				if ((DWORD)week != oldweek)
				{
					oldweek = week;
#endif
					CAppUtils::RunTortoiseGitProc(_T("/command:updatecheck"), false, false);
				}
			}
		}
	}
}
