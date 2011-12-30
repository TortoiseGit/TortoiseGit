// TortoiseMerge - a Diff/Patch program

// Copyright (C) 2006-2009 - TortoiseSVN

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

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#pragma comment(linker, "\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

BEGIN_MESSAGE_MAP(CTortoiseMergeApp, CWinAppEx)
	ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
END_MESSAGE_MAP()


CTortoiseMergeApp::CTortoiseMergeApp()
{
	EnableHtmlHelp();
	m_bLoadUserToolbars = FALSE;
	m_bSaveState = FALSE;
}

// The one and only CTortoiseMergeApp object
CTortoiseMergeApp theApp;
CCrashReport g_crasher("tortoisesvn@gmail.com", "Crash Report for TortoiseMerge " APP_X64_STRING " : " STRPRODUCTVER, TRUE);

// CTortoiseMergeApp initialization
BOOL CTortoiseMergeApp::InitInstance()
{
	SetDllDirectory(L"");

	CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerWindows));
	CMFCButton::EnableWindowsTheming();
	//set the resource dll for the required language
	CRegDWORD loc = CRegDWORD(_T("Software\\TortoiseGit\\LanguageID"), 1033);
	long langId = loc;
	CString langDll;
	HINSTANCE hInst = NULL;
	do
	{
		langDll.Format(_T("..\\Languages\\TortoiseMerge%d.dll"), langId);
		
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

	// Initialize all Managers for usage. They are automatically constructed
	// if not yet present
	InitContextMenuManager();
	InitKeyboardManager();

	CCmdLineParser parser = CCmdLineParser(this->m_lpCmdLine);

	if (parser.HasKey(_T("?")) || parser.HasKey(_T("help")))
	{
		CString sHelpText;
		sHelpText.LoadString(IDS_COMMANDLINEHELP);
		MessageBox(NULL, sHelpText, _T("TortoiseMerge"), MB_ICONINFORMATION);
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
	SetRegistryKey(_T("TortoiseMerge"));

	if (CRegDWORD(_T("Software\\TortoiseMerge\\Debug"), FALSE)==TRUE)
		AfxMessageBox(AfxGetApp()->m_lpCmdLine, MB_OK | MB_ICONINFORMATION);

	// To create the main window, this code creates a new frame window
	// object and then sets it as the application's main window object
	CMainFrame* pFrame = new CMainFrame;
	if (pFrame == NULL)
		return FALSE;
	m_pMainWnd = pFrame;

	// create and load the frame with its resources
	pFrame->LoadFrame(IDR_MAINFRAME,
		WS_OVERLAPPEDWINDOW | FWS_ADDTOTITLE, NULL,
		NULL);

	// Fill in the command line options
	pFrame->m_Data.m_baseFile.SetFileName(parser.GetVal(_T("base")));
	pFrame->m_Data.m_baseFile.SetDescriptiveName(parser.GetVal(_T("basename")));
	pFrame->m_Data.m_theirFile.SetFileName(parser.GetVal(_T("theirs")));
	pFrame->m_Data.m_theirFile.SetDescriptiveName(parser.GetVal(_T("theirsname")));
	pFrame->m_Data.m_yourFile.SetFileName(parser.GetVal(_T("mine")));
	pFrame->m_Data.m_yourFile.SetDescriptiveName(parser.GetVal(_T("minename")));
	pFrame->m_Data.m_mergedFile.SetFileName(parser.GetVal(_T("merged")));
	pFrame->m_Data.m_mergedFile.SetDescriptiveName(parser.GetVal(_T("mergedname")));
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

		OPENFILENAME ofn = {0};			// common dialog box structure
		TCHAR szFile[MAX_PATH] = {0};	// buffer for file name
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
		// check if there's a patchfile in the clipboard
		UINT cFormat = RegisterClipboardFormat(_T("TSVN_UNIFIEDDIFF"));
		if (cFormat)
		{
			if (OpenClipboard(NULL))
			{
				UINT enumFormat = 0;
				do 
				{
					if (enumFormat == cFormat)
					{
						// yes, there's a patchfile in the clipboard
						ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | OFN_ENABLETEMPLATE | OFN_ENABLEHOOK | OFN_EXPLORER;

						ofn.hInstance = AfxGetResourceHandle();
						ofn.lpTemplateName = MAKEINTRESOURCE(IDD_PATCH_FILE_OPEN_CUSTOM);
						ofn.lpfnHook = CreatePatchFileOpenHook;
					}
				} while((enumFormat = EnumClipboardFormats(enumFormat))!=0);
				CloseClipboard();
			}
		}

		CString sFilter;
		sFilter.LoadString(IDS_PATCHFILEFILTER);
		TCHAR * pszFilters = new TCHAR[sFilter.GetLength()+4];
		_tcscpy_s (pszFilters, sFilter.GetLength()+4, sFilter);
		// Replace '|' delimiters with '\0's
		TCHAR *ptr = pszFilters + _tcslen(pszFilters);  //set ptr at the NULL
		while (ptr != pszFilters)
		{
			if (*ptr == '|')
				*ptr = '\0';
			ptr--;
		}
		ofn.lpstrFilter = pszFilters;
		ofn.nFilterIndex = 1;

		// Display the Open dialog box. 
		CString tempfile;
		if (GetOpenFileName(&ofn)==FALSE)
		{
			delete [] pszFilters;
			return FALSE;
		}
		delete [] pszFilters;
		pFrame->m_Data.m_sDiffFile = ofn.lpstrFile;
	}

	if ( pFrame->m_Data.m_baseFile.GetFilename().IsEmpty() && pFrame->m_Data.m_yourFile.GetFilename().IsEmpty() )
	{
		LPWSTR *szArglist;
		int nArgs;

		szArglist = CommandLineToArgvW(GetCommandLineW(), &nArgs);
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

	// try to find a suitable window title
	CString sYour = pFrame->m_Data.m_yourFile.GetDescriptiveName();
	if (sYour.Find(_T(" - "))>=0)
		sYour = sYour.Left(sYour.Find(_T(" - ")));
	if (sYour.Find(_T(" : "))>=0)
		sYour = sYour.Left(sYour.Find(_T(" : ")));
	CString sTheir = pFrame->m_Data.m_theirFile.GetDescriptiveName();
	if (sTheir.Find(_T(" - "))>=0)
		sTheir = sTheir.Left(sTheir.Find(_T(" - ")));
	if (sTheir.Find(_T(" : "))>=0)
		sTheir = sTheir.Left(sTheir.Find(_T(" : ")));

	if (!sYour.IsEmpty() && !sTheir.IsEmpty())
	{
		if (sYour.CompareNoCase(sTheir)==0)
			pFrame->SetWindowText(sYour + _T(" - TortoiseMerge"));
		else if ((sYour.GetLength() < 10) &&
				(sTheir.GetLength() < 10))
			pFrame->SetWindowText(sYour + _T(" - ") + sTheir + _T(" - TortoiseMerge"));
		else
		{
			// we have two very long descriptive texts here, which
			// means we have to find a way to use them as a window 
			// title in a shorter way.
			// for simplicity, we just use the one from "yourfile"
			pFrame->SetWindowText(sYour + _T(" - TortoiseMerge"));
		}
	}
	else if (!sYour.IsEmpty())
		pFrame->SetWindowText(sYour + _T(" - TortoiseMerge"));
	else if (!sTheir.IsEmpty())
		pFrame->SetWindowText(sTheir + _T(" - TortoiseMerge"));

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
				OPENFILENAME ofn = {0};			// common dialog box structure
				TCHAR szFile[MAX_PATH] = {0};	// buffer for file name
				ofn.lStructSize = sizeof(OPENFILENAME);
				ofn.lpstrFile = szFile;
				ofn.nMaxFile = _countof(szFile);
				CString temp;
				temp.LoadString(IDS_SAVEASTITLE);
				if (!temp.IsEmpty())
					ofn.lpstrTitle = temp;
				ofn.Flags = OFN_OVERWRITEPROMPT;
				CString sFilter;
				sFilter.LoadString(IDS_COMMONFILEFILTER);
				TCHAR * pszFilters = new TCHAR[sFilter.GetLength()+4];
				_tcscpy_s (pszFilters, sFilter.GetLength()+4, sFilter);
				// Replace '|' delimiters with '\0's
				TCHAR *ptr = pszFilters + _tcslen(pszFilters);  //set ptr at the NULL
				while (ptr != pszFilters)
				{
					if (*ptr == '|')
						*ptr = '\0';
					ptr--;
				}
				ofn.lpstrFilter = pszFilters;
				ofn.nFilterIndex = 1;

				// Display the Save dialog box. 
				CString sFile;
				if (GetSaveFileName(&ofn)==TRUE)
				{
					outfile = CString(ofn.lpstrFile);
				}
				delete [] pszFilters;
			}
			if (!outfile.IsEmpty())
			{
				CAppUtils::CreateUnifiedDiff(origFile, modifiedFile, outfile, false);
				return FALSE;
			}
		}
	}
	// The one and only window has been initialized, so show and update it
	pFrame->ActivateFrame();
	pFrame->ShowWindow(SW_SHOW);
	pFrame->UpdateWindow();
	pFrame->ShowDiffBar(!pFrame->m_bOneWay);
	if (!pFrame->m_Data.IsBaseFileInUse() && pFrame->m_Data.m_sPatchPath.IsEmpty() && pFrame->m_Data.m_sDiffFile.IsEmpty())
	{
		pFrame->OnFileOpen();
		return TRUE;
	}

	return pFrame->LoadViews();
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
	if(uiMsg ==	WM_COMMAND && LOWORD(wParam) == IDC_PATCH_TO_CLIPBOARD)
	{
		HWND hFileDialog = GetParent(hDlg);

		// if there's a patchfile in the clipboard, we save it
		// to a temporary file and tell TortoiseMerge to use that one
		UINT cFormat = RegisterClipboardFormat(_T("TSVN_UNIFIEDDIFF"));
		if ((cFormat)&&(OpenClipboard(NULL)))
		{ 
			HGLOBAL hglb = GetClipboardData(cFormat); 
			LPCSTR lpstr = (LPCSTR)GlobalLock(hglb); 

			DWORD len = GetTempPath(0, NULL);
			TCHAR * path = new TCHAR[len+1];
			TCHAR * tempF = new TCHAR[len+100];
			GetTempPath (len+1, path);
			GetTempFileName (path, TEXT("tsm"), 0, tempF);
			std::wstring sTempFile = std::wstring(tempF);
			delete [] path;
			delete [] tempF;

			FILE * outFile;
			size_t patchlen = strlen(lpstr);
			_tfopen_s(&outFile, sTempFile.c_str(), _T("wb"));
			if(outFile)
			{
				size_t size = fwrite(lpstr, sizeof(char), patchlen, outFile);
				if (size == patchlen)
				{
					CommDlg_OpenSave_SetControlText(hFileDialog, edt1, sTempFile.c_str());
					PostMessage(hFileDialog, WM_COMMAND, MAKEWPARAM(IDOK, BM_CLICK), (LPARAM)(GetDlgItem(hDlg, IDOK)));
				}
				fclose(outFile);
			}
			GlobalUnlock(hglb); 
			CloseClipboard(); 
		} 
	}
	return 0;
}

int CTortoiseMergeApp::ExitInstance()
{
	// Look for temporary files left around by TortoiseMerge and
	// remove them. But only delete 'old' files 
	DWORD len = ::GetTempPath(0, NULL);
	TCHAR * path = new TCHAR[len + 100];
	len = ::GetTempPath (len+100, path);
	if (len != 0)
	{
		CSimpleFileFind finder = CSimpleFileFind(path, _T("*tsm*.*"));
		FILETIME systime_;
		::GetSystemTimeAsFileTime(&systime_);
		__int64 systime = (((_int64)systime_.dwHighDateTime)<<32) | ((__int64)systime_.dwLowDateTime);
		while (finder.FindNextFileNoDirectories())
		{
			CString filepath = finder.GetFilePath();
			HANDLE hFile = ::CreateFile(filepath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, NULL, NULL);
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
						::DeleteFile(filepath);
					}
				}
				else
					::CloseHandle(hFile);
			}
		}
	}	
	delete[] path;		

	return CWinAppEx::ExitInstance();
}
