// TortoiseGitMerge - a Diff/Patch program

// Copyright (C) 2013-2014 - TortoiseGit
// Copyright (C) 2006-2014 - TortoiseSVN

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
#include "git2/threads.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#pragma comment(linker, "\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

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
	: m_nAppLook(0)
{
	EnableHtmlHelp();
	m_bHiColorIcons = TRUE;
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
	}

	//set the resource dll for the required language
	CRegDWORD loc = CRegDWORD(_T("Software\\TortoiseGit\\LanguageID"), 1033);
	long langId = loc;
	CString langDll;
	HINSTANCE hInst = NULL;
	do
	{
		langDll.Format(_T("%sLanguages\\TortoiseMerge%ld.dll"), (LPCTSTR)CPathUtils::GetAppParentDirectory(), langId);

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
	TCHAR buf[6] = { 0 };
	_tcscpy_s(buf, _T("en"));
	langId = loc;
	CString sHelppath = CPathUtils::GetAppDirectory() + _T("TortoiseMerge_en.chm");
	free((void*)m_pszHelpFilePath);
	m_pszHelpFilePath=_tcsdup(sHelppath);
	sHelppath = CPathUtils::GetAppParentDirectory() + _T("Languages\\TortoiseMerge_en.chm");
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

	// InitCommonControls() is required on Windows XP if an application
	// manifest specifies use of ComCtl32.dll version 6 or later to enable
	// visual styles.  Otherwise, any window creation will fail.
	InitCommonControls();

	CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerWindows));
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

	CCmdLineParser parser = CCmdLineParser(this->m_lpCmdLine);

	g_sGroupingUUID = parser.GetVal(L"groupuuid");

	if (parser.HasKey(_T("?")) || parser.HasKey(_T("help")))
	{
		CString sHelpText;
		sHelpText.LoadString(IDS_COMMANDLINEHELP);
		MessageBox(NULL, sHelpText, _T("TortoiseGitMerge"), MB_ICONINFORMATION);
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
	SetRegistryKey(_T("TortoiseGitMerge"));

	if (CRegDWORD(_T("Software\\TortoiseGitMerge\\Debug"), FALSE)==TRUE)
		AfxMessageBox(AfxGetApp()->m_lpCmdLine, MB_OK | MB_ICONINFORMATION);

	// To create the main window, this code creates a new frame window
	// object and then sets it as the application's main window object
	CMainFrame* pFrame = new CMainFrame;
	if (pFrame == NULL)
		return FALSE;
	m_pMainWnd = pFrame;

	// create and load the frame with its resources
	if (!pFrame->LoadFrame(IDR_MAINFRAME, WS_OVERLAPPEDWINDOW | FWS_ADDTOTITLE, NULL, NULL))
		return FALSE;

	// Fill in the command line options
	pFrame->m_Data.m_baseFile.SetFileName(parser.GetVal(_T("base")));
	pFrame->m_Data.m_baseFile.SetDescriptiveName(parser.GetVal(_T("basename")));
	pFrame->m_Data.m_baseFile.SetReflectedName(parser.GetVal(_T("basereflectedname")));
	pFrame->m_Data.m_theirFile.SetFileName(parser.GetVal(_T("theirs")));
	pFrame->m_Data.m_theirFile.SetDescriptiveName(parser.GetVal(_T("theirsname")));
	pFrame->m_Data.m_theirFile.SetReflectedName(parser.GetVal(_T("theirsreflectedname")));
	pFrame->m_Data.m_yourFile.SetFileName(parser.GetVal(_T("mine")));
	pFrame->m_Data.m_yourFile.SetDescriptiveName(parser.GetVal(_T("minename")));
	pFrame->m_Data.m_yourFile.SetReflectedName(parser.GetVal(_T("minereflectedname")));
	pFrame->m_Data.m_mergedFile.SetFileName(parser.GetVal(_T("merged")));
	pFrame->m_Data.m_mergedFile.SetDescriptiveName(parser.GetVal(_T("mergedname")));
	pFrame->m_Data.m_mergedFile.SetReflectedName(parser.GetVal(_T("mergedreflectedname")));
	pFrame->m_Data.m_sPatchPath = parser.HasVal(_T("patchpath")) ? parser.GetVal(_T("patchpath")) : _T("");
	pFrame->m_Data.m_sPatchPath.Replace('/', '\\');
	if (parser.HasKey(_T("patchoriginal")))
		pFrame->m_Data.m_sPatchOriginal = parser.GetVal(_T("patchoriginal"));
	if (parser.HasKey(_T("patchpatched")))
		pFrame->m_Data.m_sPatchPatched = parser.GetVal(_T("patchpatched"));
	pFrame->m_Data.m_sDiffFile = parser.GetVal(_T("diff"));
	pFrame->m_Data.m_sDiffFile.Replace('/', '\\');
	if (parser.HasKey(_T("oneway")))
		pFrame->m_bOneWay = TRUE;
	if (parser.HasKey(_T("diff")))
		pFrame->m_bOneWay = FALSE;
	if (parser.HasKey(_T("reversedpatch")))
		pFrame->m_bReversedPatch = TRUE;
	if (parser.HasKey(_T("saverequired")))
		pFrame->m_bSaveRequired = true;
	if (parser.HasKey(_T("saverequiredonconflicts")))
		pFrame->m_bSaveRequiredOnConflicts = true;
	if (parser.HasKey(_T("deletebasetheirsmineonclose")))
		pFrame->m_bDeleteBaseTheirsMineOnClose = true;
	if (pFrame->m_Data.IsBaseFileInUse() && !pFrame->m_Data.IsYourFileInUse() && pFrame->m_Data.IsTheirFileInUse())
	{
		pFrame->m_Data.m_yourFile.TransferDetailsFrom(pFrame->m_Data.m_theirFile);
	}

	if ((!parser.HasKey(_T("patchpath")))&&(parser.HasVal(_T("diff"))))
	{
		// a patchfile was given, but not folder path to apply the patch to
		// If the patchfile is located inside a working copy, then use the parent directory
		// of the patchfile as the target directory, otherwise ask the user for a path.
		if (parser.HasKey(_T("wc")))
			pFrame->m_Data.m_sPatchPath = pFrame->m_Data.m_sDiffFile.Left(pFrame->m_Data.m_sDiffFile.ReverseFind('\\'));
		else
		{
			CBrowseFolder fbrowser;
			fbrowser.m_style = BIF_EDITBOX | BIF_NEWDIALOGSTYLE | BIF_RETURNFSANCESTORS | BIF_RETURNONLYFSDIRS;
			if (fbrowser.Show(NULL, pFrame->m_Data.m_sPatchPath)==CBrowseFolder::CANCEL)
				return FALSE;
		}
	}

	if ((parser.HasKey(_T("patchpath")))&&(!parser.HasVal(_T("diff"))))
	{
		// A path was given for applying a patchfile, but
		// the patchfile itself was not.
		// So ask the user for that patchfile

		HRESULT hr;
		// Create a new common save file dialog
		CComPtr<IFileOpenDialog> pfd = NULL;
		hr = pfd.CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER);
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
				hr = pfd->QueryInterface(IID_PPV_ARGS(&pfdCustomize));
				if (SUCCEEDED(hr))
				{
					// check if there's a unified diff on the clipboard and
					// add a button to the fileopen dialog if there is.
					UINT cFormat = RegisterClipboardFormat(_T("TSVN_UNIFIEDDIFF"));
					if ((cFormat)&&(OpenClipboard(NULL)))
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
				CComPtr<IShellItem> psiResult = NULL;
				hr = pfd->GetResult(&psiResult);
				if (bAdvised)
					pfd->Unadvise(dwCookie);
				if (SUCCEEDED(hr))
				{
					PWSTR pszPath = NULL;
					hr = psiResult->GetDisplayName(SIGDN_FILESYSPATH, &pszPath);
					if (SUCCEEDED(hr))
					{
						pFrame->m_Data.m_sDiffFile = pszPath;
						CoTaskMemFree(pszPath);
					}
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
		else
		{
			OPENFILENAME ofn = {0};         // common dialog box structure
			TCHAR szFile[MAX_PATH] = {0};   // buffer for file name
			// Initialize OPENFILENAME
			ofn.lStructSize = sizeof(OPENFILENAME);
			ofn.hwndOwner = pFrame->m_hWnd;
			ofn.lpstrFile = szFile;
			ofn.nMaxFile = _countof(szFile);
			CString temp;
			temp.LoadString(IDS_OPENDIFFFILETITLE);
			if (temp.IsEmpty())
				ofn.lpstrTitle = NULL;
			else
				ofn.lpstrTitle = temp;

			ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | OFN_EXPLORER;
			if( HasClipboardPatch() ) {
				ofn.Flags |= ( OFN_ENABLETEMPLATE | OFN_ENABLEHOOK );
				ofn.hInstance = AfxGetResourceHandle();
				ofn.lpTemplateName = MAKEINTRESOURCE(IDD_PATCH_FILE_OPEN_CUSTOM);
				ofn.lpfnHook = CreatePatchFileOpenHook;
			}

			CSelectFileFilter fileFilter(IDS_PATCHFILEFILTER);
			ofn.lpstrFilter = fileFilter;
			ofn.nFilterIndex = 1;

			// Display the Open dialog box.
			if (GetOpenFileName(&ofn)==FALSE)
			{
				return FALSE;
			}
			pFrame->m_Data.m_sDiffFile = ofn.lpstrFile;
		}
	}

	if ( pFrame->m_Data.m_baseFile.GetFilename().IsEmpty() && pFrame->m_Data.m_yourFile.GetFilename().IsEmpty() )
	{
		int nArgs;
		LPWSTR *szArglist = CommandLineToArgvW(GetCommandLineW(), &nArgs);
		if( NULL == szArglist )
		{
			TRACE("CommandLineToArgvW failed\n");
		}
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

	pFrame->m_bReadOnly = !!parser.HasKey(_T("readonly"));
	if (GetFileAttributes(pFrame->m_Data.m_yourFile.GetFilename()) & FILE_ATTRIBUTE_READONLY)
		pFrame->m_bReadOnly = true;
	pFrame->m_bBlame = !!parser.HasKey(_T("blame"));
	// diffing a blame means no editing!
	if (pFrame->m_bBlame)
		pFrame->m_bReadOnly = true;

	pFrame->SetWindowTitle();

	if (parser.HasKey(_T("createunifieddiff")))
	{
		// user requested to create a unified diff file
		CString origFile = parser.GetVal(_T("origfile"));
		CString modifiedFile = parser.GetVal(_T("modifiedfile"));
		if (!origFile.IsEmpty() && !modifiedFile.IsEmpty())
		{
			CString outfile = parser.GetVal(_T("outfile"));
			if (outfile.IsEmpty())
			{
				CCommonAppUtils::FileOpenSave(outfile, NULL, IDS_SAVEASTITLE, IDS_COMMONFILEFILTER, false, NULL);
			}
			if (!outfile.IsEmpty())
			{
				CRegStdDWORD regContextLines(L"Software\\TortoiseGitMerge\\ContextLines", (DWORD)-1);
				CAppUtils::CreateUnifiedDiff(origFile, modifiedFile, outfile, regContextLines, false);
				return FALSE;
			}
		}
	}

	pFrame->resolveMsgWnd    = parser.HasVal(L"resolvemsghwnd")   ? (HWND)parser.GetLongLongVal(L"resolvemsghwnd")     : 0;
	pFrame->resolveMsgWParam = parser.HasVal(L"resolvemsgwparam") ? (WPARAM)parser.GetLongLongVal(L"resolvemsgwparam") : 0;
	pFrame->resolveMsgLParam = parser.HasVal(L"resolvemsglparam") ? (LPARAM)parser.GetLongLongVal(L"resolvemsglparam") : 0;

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
	if (parser.HasVal(_T("line")))
	{
		line = parser.GetLongVal(_T("line"));
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

UINT_PTR CALLBACK
CTortoiseMergeApp::CreatePatchFileOpenHook(HWND hDlg, UINT uiMsg, WPARAM wParam, LPARAM /*lParam*/)
{
	if(uiMsg == WM_COMMAND && LOWORD(wParam) == IDC_PATCH_TO_CLIPBOARD)
	{
		HWND hFileDialog = GetParent(hDlg);

		// if there's a patchfile in the clipboard, we save it
		// to a temporary file and tell TortoiseMerge to use that one
		std::wstring sTempFile;
		if (TrySavePatchFromClipboard(sTempFile))
		{
			CommDlg_OpenSave_SetControlText(hFileDialog, edt1, sTempFile.c_str());
			PostMessage(hFileDialog, WM_COMMAND, MAKEWPARAM(IDOK, BM_CLICK), (LPARAM)(GetDlgItem(hDlg, IDOK)));
		}
	}
	return 0;
}

int CTortoiseMergeApp::ExitInstance()
{
	// Look for temporary files left around by TortoiseMerge and
	// remove them. But only delete 'old' files
	CTempFiles::DeleteOldTempFiles(_T("*tsm*.*"));

	return CWinAppEx::ExitInstance();
}

bool CTortoiseMergeApp::HasClipboardPatch()
{
	// check if there's a patchfile in the clipboard
	const UINT cFormat = RegisterClipboardFormat(_T("TSVN_UNIFIEDDIFF"));
	if (cFormat == 0)
		return false;

	if (OpenClipboard(NULL) == 0)
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

	UINT cFormat = RegisterClipboardFormat(_T("TSVN_UNIFIEDDIFF"));
	if (cFormat == 0)
		return false;
	if (OpenClipboard(NULL) == 0)
		return false;

	HGLOBAL hglb = GetClipboardData(cFormat);
	LPCSTR lpstr = (LPCSTR)GlobalLock(hglb);

	DWORD len = GetTempPath(0, NULL);
	std::unique_ptr<TCHAR[]> path(new TCHAR[len+1]);
	std::unique_ptr<TCHAR[]> tempF(new TCHAR[len+100]);
	GetTempPath (len+1, path.get());
	GetTempFileName (path.get(), _T("tsm"), 0, tempF.get());
	std::wstring sTempFile = std::wstring(tempF.get());

	FILE* outFile = 0;
	_tfopen_s(&outFile, sTempFile.c_str(), _T("wb"));
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
