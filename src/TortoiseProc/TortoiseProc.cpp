// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2012 - TortoiseGit
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
#include "SysImageList.h"
#include "..\Utils\CrashReport.h"
#include "CmdLineParser.h"
#include "Hooks.h"
#include "AppUtils.h"
#include "PathUtils.h"
#include "UnicodeUtils.h"
#include "MessageBox.h"
//#include "libintl.h"
#include "DirFileEnum.h"
//#include "SoundUtils.h"
#include "GitAdminDir.h"
#include "Git.h"
#include "SmartHandle.h"
#include "Commands\Command.h"
#include "..\version.h"
#include "JumpListHelpers.h"
#include "SinglePropSheetDlg.h"
#include "Settings\setmainpage.h"
#include "..\Settings\Settings.h"
#include "gitindex.h"
#include "Libraries.h"

#define STRUCT_IOVEC_DEFINED

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#pragma comment(linker, "\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#define APPID (_T("TGIT.TGIT.1") _T(TGIT_PLATFORM))

BEGIN_MESSAGE_MAP(CTortoiseProcApp, CWinAppEx)
	ON_COMMAND(ID_HELP, CWinAppEx::OnHelp)
END_MESSAGE_MAP()

//CString g_version;
//CString CGit::m_MsysGitPath;
//////////////////////////////////////////////////////////////////////////

CTortoiseProcApp::CTortoiseProcApp()
{
	SetDllDirectory(L"");
	CCrashReport::Instance().AddUserInfoToReport(L"CommandLine", GetCommandLine());
	EnableHtmlHelp();
//	int argc = 0;
//	const char* const * argv = NULL;
	SYS_IMAGE_LIST();
	CHooks::Create();
	m_bLoadUserToolbars = FALSE;
	m_bSaveState = FALSE;
	retSuccess = false;

}

CTortoiseProcApp::~CTortoiseProcApp()
{
	CHooks::Destroy();
	SYS_IMAGE_LIST().Cleanup();
}

// The one and only CTortoiseProcApp object
CTortoiseProcApp theApp;
CString sOrigCWD;
HWND hWndExplorer;

BOOL CTortoiseProcApp::CheckMsysGitDir()
{
	//CGitIndexFileMap map;
	//int status;
	//CTGitPath path;
	//path.SetFromGit(_T("src/gpl.txt"));
	//map.GetFileStatus(_T("D:\\TortoiseGit"),&path, &status);
	return g_Git.CheckMsysGitDir();
}
CCrashReportTGit crasher(L"TortoiseGit " _T(APP_X64_STRING));

// CTortoiseProcApp initialization

BOOL CTortoiseProcApp::InitInstance()
{
	CheckUpgrade();
	CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerWindows));
	CMFCButton::EnableWindowsTheming();

	Gdiplus::GdiplusStartupInput gdiplusStartupInput;
	Gdiplus::GdiplusStartup(&m_gdiplusToken,&gdiplusStartupInput,NULL);

	//set the resource dll for the required language
	CRegDWORD loc = CRegDWORD(_T("Software\\TortoiseGit\\LanguageID"), 1033);
	long langId = loc;
	CString langDll;
	CStringA langpath = CStringA(CPathUtils::GetAppParentDirectory());
	langpath += "Languages";
//	bindtextdomain("subversion", (LPCSTR)langpath);
//	bind_textdomain_codeset("subversion", "UTF-8");
	HINSTANCE hInst = NULL;
	do
	{
		langDll.Format(_T("%sLanguages\\TortoiseProc%d.dll"), (LPCTSTR)CPathUtils::GetAppParentDirectory(), langId);

		hInst = LoadLibrary(langDll);

		CString sVer = _T(STRPRODUCTVER);
		CString sFileVer = CPathUtils::GetVersionFromFile(langDll);
		if (sFileVer.Compare(sVer)!=0)
		{
			FreeLibrary(hInst);
			hInst = NULL;
		}
		if (hInst != NULL)
		{
			AfxSetResourceHandle(hInst);
		}
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
	// MFC uses a help file with the same name as the application by default,
	// which means we have to change that default to our language specific help files
	sHelppath.Replace(_T("tortoiseproc.chm"), _T("TortoiseGit_en.chm"));
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
	setlocale(LC_ALL, "");

	if(!CheckMsysGitDir())
	{
		UINT ret = CMessageBox::Show(NULL, IDS_PROC_NOMSYSGIT, IDS_APPNAME, 3, IDI_HAND, IDS_PROC_SETMSYSGITPATH, IDS_PROC_GOTOMSYSGITWEBSITE, IDS_ABORTBUTTON);
		if(ret == 2)
		{
			ShellExecute(NULL, NULL, _T("http://code.google.com/p/msysgit/"), NULL, NULL, SW_SHOW);
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
			CRegStdDWORD(_T("Software\\TortoiseGit\\TortoiseProc\\OldMsysgitVersionWarning")).removeValue(); // only store answer if it is "Ignore"
			ShellExecute(NULL, NULL, _T("http://code.google.com/p/msysgit/"), NULL, NULL, SW_SHOW);
			return FALSE;
		}
		else if (ret == 2)
		{
			CRegStdDWORD(_T("Software\\TortoiseGit\\TortoiseProc\\OldMsysgitVersionWarning")).removeValue(); // only store answer if it is "Ignore"
			return FALSE;
		}
	}

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
	AfxInitRichEdit2();
	CWinAppEx::InitInstance();
	SetRegistryKey(_T("TortoiseGit"));

	CCmdLineParser parser(AfxGetApp()->m_lpCmdLine);

	hWndExplorer = NULL;
	CString sVal = parser.GetVal(_T("hwnd"));
	if (!sVal.IsEmpty())
		hWndExplorer = (HWND)_ttoi64(sVal);

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

	if ( parser.HasKey(_T("path")) && parser.HasKey(_T("pathfile")))
	{
		CMessageBox::Show(NULL, IDS_ERR_INVALIDPATH, IDS_APPNAME, MB_ICONERROR);
		return FALSE;
	}

	CTGitPath cmdLinePath;
	CTGitPathList pathList;
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
				for (int i=1; i<nArgs; i++)
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

	if (pathList.GetCount() == 0) {
		pathList.AddPath(CTGitPath::CTGitPath(g_Git.m_CurrentDir));
	}

	InitializeJumpList();
	EnsureGitLibrary(false);

	// Subversion sometimes writes temp files to the current directory!
	// Since TSVN doesn't need a specific CWD anyway, we just set it
	// to the users temp folder: that way, Subversion is guaranteed to
	// have write access to the CWD
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
		TCHAR pathbuf[MAX_PATH];
		GetTortoiseGitTempPath(MAX_PATH, pathbuf);
		SetCurrentDirectory(pathbuf);
	}

	CheckForNewerVersion();

	if (parser.HasVal(_T("configdir")))
	{
		// the user can override the location of the Subversion config directory here
		CString sConfigDir = parser.GetVal(_T("configdir"));
//		g_GitGlobal.SetConfigDir(sConfigDir);
	}

	CAutoGeneralHandle TGitMutex = ::CreateMutex(NULL, FALSE, _T("TortoiseGitProc.exe"));
	if (!g_Git.SetCurrentDir(cmdLinePath.GetWinPathString(), parser.HasKey(_T("submodule")) == TRUE))
	{
		int i=0;
		for(i=0;i<pathList.GetCount();i++)
			if(g_Git.SetCurrentDir(pathList[i].GetWinPath()))
				break;
	}

	if(!g_Git.m_CurrentDir.IsEmpty())
	{
		sOrigCWD = g_Git.m_CurrentDir;
		SetCurrentDirectory(g_Git.m_CurrentDir);
	}

	{
		CString err;
		try
		{
			// requires CWD to be set
			CGit::m_LogEncode = CAppUtils::GetLogOutputEncode();
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
		TCHAR * path = new TCHAR[len + 100];
		len = GetTortoiseGitTempPath (len+100, path);
		if (len != 0)
		{
			CDirFileEnum finder(path);
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
		delete[] path;
	}

	// Since the dialog has been closed, return FALSE so that we exit the
	// application, rather than start the application's message pump.
	return FALSE;
}

void CTortoiseProcApp::CheckUpgrade()
{
	CRegString regVersion = CRegString(_T("Software\\TortoiseGit\\CurrentVersion"));
	CString sVersion = regVersion;
	if (sVersion.Compare(_T(STRPRODUCTVER))==0)
		return;
	// we're starting the first time with a new version!

	LONG lVersion = 0;
	int pos = sVersion.Find(',');
	if (pos > 0)
	{
		lVersion = (_ttol(sVersion.Left(pos))<<24);
		lVersion |= (_ttol(sVersion.Mid(pos+1))<<16);
		pos = sVersion.Find(',', pos+1);
		lVersion |= (_ttol(sVersion.Mid(pos+1))<<8);
	}

#if 0
	if (lVersion <= 0x01010300)
	{
		CSoundUtils::RegisterTSVNSounds();
	}
#endif

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
	}

	if (lVersion <= 0x01070E00)
	{
		CRegStdDWORD(_T("Software\\TortoiseGit\\CheckNewer")).removeValue();
		// upgrade to 1.7.15: force recreation of all diff scripts.
		CAppUtils::SetupDiffScripts(true, CString());
	}
	CAppUtils::SetupDiffScripts(false, CString());

	// set the current version so we don't come here again until the next update!
	regVersion = _T(STRPRODUCTVER);
}

void CTortoiseProcApp::InitializeJumpList()
{
	// for Win7 : use a custom jump list
	CoInitialize(NULL);
	SetAppID(APPID);
	DeleteJumpList(APPID);
	DoInitializeJumpList();
	CoUninitialize();
}

void CTortoiseProcApp::DoInitializeJumpList()
{
	ATL::CComPtr<ICustomDestinationList> pcdl;
	HRESULT hr = pcdl.CoCreateInstance(CLSID_DestinationList, NULL, CLSCTX_INPROC_SERVER);
	if (FAILED(hr))
		return;

	hr = pcdl->SetAppID(APPID);
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
	sTemp.Remove('&');

	ATL::CComPtr<IShellLink> psl;
	hr = CreateShellLink(_T("/command:settings"), (LPCTSTR)sTemp, 20, &psl);
	if (SUCCEEDED(hr)) {
		poc->AddObject(psl);
	}
	sTemp = CString(MAKEINTRESOURCE(IDS_MENUHELP));
	sTemp.Remove('&');
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
					TCHAR com[MAX_PATH+100];
					GetModuleFileName(NULL, com, MAX_PATH);
					_tcscat_s(com, MAX_PATH+100, _T(" /command:updatecheck"));

					CAppUtils::LaunchApplication(com, 0, false);
				}
			}
		}
	}
}
