// TortoiseGitMerge - a Diff/Patch program

// Copyright (C) 2013-2017, 2019 - TortoiseGit
// Copyright (C) 2006-2014, 2016 - TortoiseSVN

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
#include <dlgs.h>
#include "TortoiseMerge.h"
#include "MainFrm.h"
#include "AboutDlg.h"
#include "CmdLineParser.h"
#include "version.h"
#include "AppUtils.h"
#include "PathUtils.h"
#include "BrowseFolder.h"
#include "DirFileEnum.h"
#include "SelectFileFilter.h"
#include "FileDlgEventHandler.h"
#include "TempFile.h"
#include "TaskbarUUID.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#pragma comment(linker, "\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#pragma comment(lib, "Propsys.lib")

BEGIN_MESSAGE_MAP(CTortoiseMergeApp, CWinAppEx)
	ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
END_MESSAGE_MAP()

class PatchOpenDlgEventHandler : public CFileDlgEventHandler
{
public:
	PatchOpenDlgEventHandler() {}
	~PatchOpenDlgEventHandler() {}

	virtual STDMETHODIMP OnButtonClicked(IFileDialogCustomize* pfdc, DWORD dwIDCtl)
	{
		if (dwIDCtl == 101)
		{
			CComQIPtr<IFileOpenDialog> pDlg = pfdc;
			if (pDlg)
			{
				pDlg->Close(S_OK);
			}
		}
		return S_OK;
	}
};


CTortoiseMergeApp::CTortoiseMergeApp()
{
	EnableHtmlHelp();
	git_libgit2_init();
}

CTortoiseMergeApp::~CTortoiseMergeApp()
{
	git_libgit2_shutdown();
}

// The one and only CTortoiseMergeApp object
CTortoiseMergeApp theApp;
CString sOrigCWD;
#if ENABLE_CRASHHANLDER
CCrashReportTGit g_crasher(L"TortoiseGitMerge " _T(APP_X64_STRING), TGIT_VERMAJOR, TGIT_VERMINOR, TGIT_VERMICRO, TGIT_VERBUILD, TGIT_VERDATE);
#endif

CString g_sGroupingUUID;
CString g_sGroupingIcon;
bool g_bGroupingRemoveIcon = false;

// CTortoiseMergeApp initialization
BOOL CTortoiseMergeApp::InitInstance()
{
	SetDllDirectory(L"");
	SetTaskIDPerUUID();
	CCrashReport::Instance().AddUserInfoToReport(L"CommandLine", GetCommandLine());

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
	}

	//set the resource dll for the required language
	CRegDWORD loc = CRegDWORD(L"Software\\TortoiseGit\\LanguageID", 1033);
	long langId = loc;
	CString langDll;
	HINSTANCE hInst = nullptr;
	do
	{
		langDll.Format(L"%sLanguages\\TortoiseMerge%ld.dll", static_cast<LPCTSTR>(CPathUtils::GetAppParentDirectory()), langId);

		hInst = LoadLibrary(langDll);
		CString sVer = _T(STRPRODUCTVER);
		CString sFileVer = CPathUtils::GetVersionFromFile(langDll);
		if (sFileVer.Compare(sVer)!=0)
		{
			FreeLibrary(hInst);
			hInst = nullptr;
		}
		if (hInst)
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
	} while ((!hInst) && (langId != 0));
	{
		CString langStr;
		langStr.Format(L"%ld", langId);
		CCrashReport::Instance().AddUserInfoToReport(L"LanguageID", langStr);
	}
	TCHAR buf[6] = { 0 };
	wcscpy_s(buf, L"en");
	langId = loc;
	CString sHelppath = CPathUtils::GetAppDirectory() + L"TortoiseMerge_en.chm";
	free((void*)m_pszHelpFilePath);
	m_pszHelpFilePath=_wcsdup(sHelppath);
	sHelppath = CPathUtils::GetAppParentDirectory() + L"Languages\\TortoiseMerge_en.chm";
	do
	{
		GetLocaleInfo(MAKELCID(langId, SORT_DEFAULT), LOCALE_SISO639LANGNAME, buf, _countof(buf));
		CString sLang = L"_";
		sLang += buf;
		sHelppath.Replace(L"_en", sLang);
		if (PathFileExists(sHelppath))
		{
			free((void*)m_pszHelpFilePath);
			m_pszHelpFilePath=_wcsdup(sHelppath);
			break;
		}
		sHelppath.Replace(sLang, L"_en");
		GetLocaleInfo(MAKELCID(langId, SORT_DEFAULT), LOCALE_SISO3166CTRYNAME, buf, _countof(buf));
		sLang += L'_';
		sLang += buf;
		sHelppath.Replace(L"_en", sLang);
		if (PathFileExists(sHelppath))
		{
			free((void*)m_pszHelpFilePath);
			m_pszHelpFilePath=_wcsdup(sHelppath);
			break;
		}
		sHelppath.Replace(sLang, L"_en");

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

	// InitCommonControls() is required on Windows XP if an application
	// manifest specifies use of ComCtl32.dll version 6 or later to enable
	// visual styles.  Otherwise, any window creation will fail.
	InitCommonControls();

	CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerWindows7));
	CMFCButton::EnableWindowsTheming();
	EnableTaskbarInteraction(FALSE);

	// Initialize all Managers for usage. They are automatically constructed
	// if not yet present
	InitContextMenuManager();
	InitKeyboardManager();
	InitTooltipManager ();
	CMFCToolTipInfo params;
	params.m_bVislManagerTheme = TRUE;

	GetTooltipManager ()->SetTooltipParams (
		AFX_TOOLTIP_TYPE_ALL,
		RUNTIME_CLASS (CMFCToolTipCtrl),
		&params);

	CCmdLineParser parser(m_lpCmdLine);

	g_sGroupingUUID = parser.GetVal(L"groupuuid");

	if (parser.HasKey(L"?") || parser.HasKey(L"help"))
	{
		CString sHelpText;
		sHelpText.LoadString(IDS_COMMANDLINEHELP);
		MessageBox(nullptr, sHelpText, L"TortoiseGitMerge", MB_ICONINFORMATION);
		return FALSE;
	}

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
	SetRegistryKey(L"TortoiseGitMerge");

	if (CRegDWORD(L"Software\\TortoiseGitMerge\\Debug", FALSE)==TRUE)
		AfxMessageBox(AfxGetApp()->m_lpCmdLine, MB_OK | MB_ICONINFORMATION);

	// To create the main window, this code creates a new frame window
	// object and then sets it as the application's main window object
	CMainFrame* pFrame = new CMainFrame;
	if (!pFrame)
		return FALSE;
	m_pMainWnd = pFrame;

	// create and load the frame with its resources
	if (!pFrame->LoadFrame(IDR_MAINFRAME, WS_OVERLAPPEDWINDOW | FWS_ADDTOTITLE, nullptr, nullptr))
		return FALSE;

	// Fill in the command line options
	pFrame->m_Data.m_baseFile.SetFileName(parser.GetVal(L"base"));
	pFrame->m_Data.m_baseFile.SetDescriptiveName(parser.GetVal(L"basename"));
	pFrame->m_Data.m_baseFile.SetReflectedName(parser.GetVal(L"basereflectedname"));
	pFrame->m_Data.m_theirFile.SetFileName(parser.GetVal(L"theirs"));
	pFrame->m_Data.m_theirFile.SetDescriptiveName(parser.GetVal(L"theirsname"));
	pFrame->m_Data.m_theirFile.SetReflectedName(parser.GetVal(L"theirsreflectedname"));
	pFrame->m_Data.m_yourFile.SetFileName(parser.GetVal(L"mine"));
	pFrame->m_Data.m_yourFile.SetDescriptiveName(parser.GetVal(L"minename"));
	pFrame->m_Data.m_yourFile.SetReflectedName(parser.GetVal(L"minereflectedname"));
	pFrame->m_Data.m_mergedFile.SetFileName(parser.GetVal(L"merged"));
	pFrame->m_Data.m_mergedFile.SetDescriptiveName(parser.GetVal(L"mergedname"));
	pFrame->m_Data.m_mergedFile.SetReflectedName(parser.GetVal(L"mergedreflectedname"));
	pFrame->m_Data.m_sPatchPath = parser.HasVal(L"patchpath") ? parser.GetVal(L"patchpath") : L"";
	pFrame->m_Data.m_sPatchPath.Replace('/', '\\');
	if (parser.HasKey(L"patchoriginal"))
		pFrame->m_Data.m_sPatchOriginal = parser.GetVal(L"patchoriginal");
	if (parser.HasKey(L"patchpatched"))
		pFrame->m_Data.m_sPatchPatched = parser.GetVal(L"patchpatched");
	pFrame->m_Data.m_sDiffFile = parser.GetVal(L"diff");
	pFrame->m_Data.m_sDiffFile.Replace('/', '\\');
	if (parser.HasKey(L"oneway"))
		pFrame->m_bOneWay = TRUE;
	if (parser.HasKey(L"diff"))
		pFrame->m_bOneWay = FALSE;
	if (parser.HasKey(L"reversedpatch"))
		pFrame->m_bReversedPatch = TRUE;
	if (parser.HasKey(L"saverequired"))
		pFrame->m_bSaveRequired = true;
	if (parser.HasKey(L"saverequiredonconflicts"))
		pFrame->m_bSaveRequiredOnConflicts = true;
	if (parser.HasKey(L"deletebasetheirsmineonclose"))
		pFrame->m_bDeleteBaseTheirsMineOnClose = true;
	if (pFrame->m_Data.IsBaseFileInUse() && !pFrame->m_Data.IsYourFileInUse() && pFrame->m_Data.IsTheirFileInUse())
	{
		pFrame->m_Data.m_yourFile.TransferDetailsFrom(pFrame->m_Data.m_theirFile);
	}

	if ((!parser.HasKey(L"patchpath")) && (parser.HasVal(L"diff")))
	{
		// a patchfile was given, but not folder path to apply the patch to
		// If the patchfile is located inside a working copy, then use the parent directory
		// of the patchfile as the target directory, otherwise ask the user for a path.
		if (parser.HasKey(L"wc"))
			pFrame->m_Data.m_sPatchPath = pFrame->m_Data.m_sDiffFile.Left(pFrame->m_Data.m_sDiffFile.ReverseFind('\\'));
		else
		{
			CBrowseFolder fbrowser;
			fbrowser.m_style = BIF_EDITBOX | BIF_NEWDIALOGSTYLE | BIF_RETURNFSANCESTORS | BIF_RETURNONLYFSDIRS;
			if (fbrowser.Show(nullptr, pFrame->m_Data.m_sPatchPath) == CBrowseFolder::CANCEL)
				return FALSE;
		}
	}

	if ((parser.HasKey(L"patchpath")) && (!parser.HasVal(L"diff")))
	{
		// A path was given for applying a patchfile, but
		// the patchfile itself was not.
		// So ask the user for that patchfile

		HRESULT hr;
		// Create a new common save file dialog
		CComPtr<IFileOpenDialog> pfd;
		hr = pfd.CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER);
		if (SUCCEEDED(hr))
		{
			// Set the dialog options
			DWORD dwOptions;
			if (SUCCEEDED(hr = pfd->GetOptions(&dwOptions)))
			{
				hr = pfd->SetOptions(dwOptions | FOS_FILEMUSTEXIST | FOS_FORCEFILESYSTEM | FOS_PATHMUSTEXIST);
			}

			// Set a title
			if (SUCCEEDED(hr))
			{
				CString temp;
				temp.LoadString(IDS_OPENDIFFFILETITLE);
				pfd->SetTitle(temp);
			}
			CSelectFileFilter fileFilter(IDS_PATCHFILEFILTER);
			hr = pfd->SetFileTypes(fileFilter.GetCount(), fileFilter);
			bool bAdvised = false;
			DWORD dwCookie = 0;
			CComObjectStackEx<PatchOpenDlgEventHandler> cbk;
			CComQIPtr<IFileDialogEvents> pEvents = cbk.GetUnknown();

			{
				CComPtr<IFileDialogCustomize> pfdCustomize;
				hr = pfd.QueryInterface(&pfdCustomize);
				if (SUCCEEDED(hr))
				{
					// check if there's a unified diff on the clipboard and
					// add a button to the fileopen dialog if there is.
					UINT cFormat = RegisterClipboardFormat(L"TGIT_UNIFIEDDIFF");
					if ((cFormat) && (OpenClipboard(nullptr)))
					{
						HGLOBAL hglb = GetClipboardData(cFormat);
						if (hglb)
						{
							pfdCustomize->AddPushButton(101, CString(MAKEINTRESOURCE(IDS_PATCH_COPYFROMCLIPBOARD)));
							hr = pfd->Advise(pEvents, &dwCookie);
							bAdvised = SUCCEEDED(hr);
						}
						CloseClipboard();
					}
				}
			}

			// Show the save file dialog
			if (SUCCEEDED(hr) && SUCCEEDED(hr = pfd->Show(pFrame->m_hWnd)))
			{
				// Get the selection from the user
				CComPtr<IShellItem> psiResult;
				hr = pfd->GetResult(&psiResult);
				if (bAdvised)
					pfd->Unadvise(dwCookie);
				if (SUCCEEDED(hr))
				{
					CComHeapPtr<WCHAR> pszPath;
					hr = psiResult->GetDisplayName(SIGDN_FILESYSPATH, &pszPath);
					if (SUCCEEDED(hr))
						pFrame->m_Data.m_sDiffFile = pszPath;
				}
				else
				{
					// no result, which means we closed the dialog in our button handler
					std::wstring sTempFile;
					if (TrySavePatchFromClipboard(sTempFile))
						pFrame->m_Data.m_sDiffFile = sTempFile.c_str();
				}
			}
			else
			{
				if (bAdvised)
					pfd->Unadvise(dwCookie);
				return FALSE;
			}
		}
	}

	if ( pFrame->m_Data.m_baseFile.GetFilename().IsEmpty() && pFrame->m_Data.m_yourFile.GetFilename().IsEmpty() )
	{
		int nArgs;
		LPWSTR *szArglist = CommandLineToArgvW(GetCommandLineW(), &nArgs);
		if (!szArglist)
			TRACE("CommandLineToArgvW failed\n");
		else
		{
			if ( nArgs==3 || nArgs==4 )
			{
				// Four parameters:
				// [0]: Program name
				// [1]: BASE file
				// [2]: my file
				// [3]: THEIR file (optional)
				// This is the same format CAppUtils::StartExtDiff
				// uses if %base and %mine are not set and most
				// other diff tools use it too.
				if ( PathFileExists(szArglist[1]) && PathFileExists(szArglist[2]) )
				{
					pFrame->m_Data.m_baseFile.SetFileName(szArglist[1]);
					pFrame->m_Data.m_yourFile.SetFileName(szArglist[2]);
					if ( nArgs == 4 && PathFileExists(szArglist[3]) )
					{
						pFrame->m_Data.m_theirFile.SetFileName(szArglist[3]);
					}
				}
			}
			else if (nArgs == 2)
			{
				// only one path specified: use it to fill the "open" dialog
				if (PathFileExists(szArglist[1]))
				{
					pFrame->m_Data.m_yourFile.SetFileName(szArglist[1]);
					pFrame->m_Data.m_yourFile.StoreFileAttributes();
				}
			}
		}

		// Free memory allocated for CommandLineToArgvW arguments.
		LocalFree(szArglist);
	}

	pFrame->m_bReadOnly = !!parser.HasKey(L"readonly");
	if (GetFileAttributes(pFrame->m_Data.m_yourFile.GetFilename()) & FILE_ATTRIBUTE_READONLY)
		pFrame->m_bReadOnly = true;
	pFrame->m_bBlame = !!parser.HasKey(L"blame");
	// diffing a blame means no editing!
	if (pFrame->m_bBlame)
		pFrame->m_bReadOnly = true;

	pFrame->SetWindowTitle();

	if (parser.HasKey(L"createunifieddiff"))
	{
		// user requested to create a unified diff file
		CString origFile = parser.GetVal(L"origfile");
		CString modifiedFile = parser.GetVal(L"modifiedfile");
		if (!origFile.IsEmpty() && !modifiedFile.IsEmpty())
		{
			CString outfile = parser.GetVal(L"outfile");
			if (outfile.IsEmpty())
			{
				CCommonAppUtils::FileOpenSave(outfile, nullptr, IDS_SAVEASTITLE, IDS_COMMONFILEFILTER, false);
			}
			if (!outfile.IsEmpty())
			{
				CRegStdDWORD regContextLines(L"Software\\TortoiseGitMerge\\ContextLines", static_cast<DWORD>(-1));
				CAppUtils::CreateUnifiedDiff(origFile, modifiedFile, outfile, regContextLines, false);
				return FALSE;
			}
		}
	}

	pFrame->resolveMsgWnd    = parser.HasVal(L"resolvemsghwnd")   ? reinterpret_cast<HWND>(parser.GetLongLongVal(L"resolvemsghwnd")) : 0;
	pFrame->resolveMsgWParam = parser.HasVal(L"resolvemsgwparam") ? static_cast<WPARAM>(parser.GetLongLongVal(L"resolvemsgwparam"))  : 0;
	pFrame->resolveMsgLParam = parser.HasVal(L"resolvemsglparam") ? static_cast<LPARAM>(parser.GetLongLongVal(L"resolvemsglparam"))  : 0;

	// The one and only window has been initialized, so show and update it
	pFrame->ActivateFrame();
	pFrame->ShowWindow(SW_SHOW);
	pFrame->UpdateWindow();
	pFrame->ShowDiffBar(!pFrame->m_bOneWay);
	if (!pFrame->m_Data.IsBaseFileInUse() && pFrame->m_Data.m_sPatchPath.IsEmpty() && pFrame->m_Data.m_sDiffFile.IsEmpty())
	{
		pFrame->OnFileOpen(pFrame->m_Data.m_yourFile.InUse());
		return TRUE;
	}

	int line = -2;
	if (parser.HasVal(L"line"))
	{
		line = parser.GetLongVal(L"line");
		line--; // we need the index
	}

	return pFrame->LoadViews(line);
}

// CTortoiseMergeApp message handlers

void CTortoiseMergeApp::OnAppAbout()
{
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}

int CTortoiseMergeApp::ExitInstance()
{
	// Look for temporary files left around by TortoiseMerge and
	// remove them. But only delete 'old' files
	CTempFiles::DeleteOldTempFiles(L"*tsm*.*");

	return CWinAppEx::ExitInstance();
}

bool CTortoiseMergeApp::HasClipboardPatch()
{
	// check if there's a patchfile in the clipboard
	const UINT cFormat = RegisterClipboardFormat(L"TGIT_UNIFIEDDIFF");
	if (cFormat == 0)
		return false;

	if (OpenClipboard(nullptr) == 0)
		return false;

	bool containsPatch = false;
	UINT enumFormat = 0;
	do
	{
		if (enumFormat == cFormat)
		{
			containsPatch = true;   // yes, there's a patchfile in the clipboard
		}
	} while((enumFormat = EnumClipboardFormats(enumFormat))!=0);
	CloseClipboard();

	return containsPatch;
}

bool CTortoiseMergeApp::TrySavePatchFromClipboard(std::wstring& resultFile)
{
	resultFile.clear();

	UINT cFormat = RegisterClipboardFormat(L"TGIT_UNIFIEDDIFF");
	if (cFormat == 0)
		return false;
	if (OpenClipboard(nullptr) == 0)
		return false;

	HGLOBAL hglb = GetClipboardData(cFormat);
	auto lpstr = static_cast<LPCSTR>(GlobalLock(hglb));

	DWORD len = GetTempPath(0, nullptr);
	auto path = std::make_unique<TCHAR[]>(len + 1);
	auto tempF = std::make_unique<TCHAR[]>(len + 100);
	GetTempPath (len+1, path.get());
	GetTempFileName (path.get(), L"tsm", 0, tempF.get());
	std::wstring sTempFile = std::wstring(tempF.get());

	FILE* outFile = 0;
	_wfopen_s(&outFile, sTempFile.c_str(), L"wb");
	if (outFile != 0)
	{
		size_t patchlen = strlen(lpstr);
		size_t size = fwrite(lpstr, sizeof(char), patchlen, outFile);
		if (size == patchlen)
			 resultFile = sTempFile;

		fclose(outFile);
	}
	GlobalUnlock(hglb);
	CloseClipboard();

	return !resultFile.empty();
}
