// TGitInstaller.cpp : Definiert den Einstiegspunkt für die Anwendung.
//

#include "stdafx.h"
#include "TGitInstaller.h"
#include <propsys.h>
#include <PropKey.h>

#include "UpdateDownloader.h"
#include "Win7.h"
#include "DirFileEnum.h"
#include "PathUtils.h"
#include "UpdateCrypto.h"
#include "SimpleIni.h"

#include <commctrl.h>
#pragma comment(lib, "comctl32.lib")
#pragma comment(linker, "\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#define MAX_LOADSTRING 100

#define SIGNATURE_FILE_ENDING _T(".rsa.asc")

#define WM_USER_DISPLAYSTATUS	(WM_USER + 1)
#define WM_USER_ENDDOWNLOAD		(WM_USER + 2)
#define WM_USER_FILLCHANGELOG	(WM_USER + 3)

// Globale Variablen:
HINSTANCE hInst;								// Aktuelle Instanz
TCHAR szTitle[MAX_LOADSTRING];					// Titelleistentext
TCHAR szWindowClass[MAX_LOADSTRING];			// Klassenname des Hauptfensters
CUpdateDownloader* m_updateDownloader = nullptr;
HWND listview;

extern CString GetTempFile();

// Vorwärtsdeklarationen der in diesem Codemodul enthaltenen Funktionen:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY _tWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPTSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	SetDllDirectory(L"");

	InitCommonControls();

 	// TODO: Hier Code einfügen.
	MSG msg;
	HACCEL hAccelTable;

	// Globale Zeichenfolgen initialisieren
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_TGITINSTALLER, szWindowClass, MAX_LOADSTRING);
	//MyRegisterClass(hInstance);

	// Anwendungsinitialisierung ausführen:
	/*if (!InitInstance (hInstance, nCmdShow))
	{
		return FALSE;
	}*/
	DialogBoxParam(hInstance, MAKEINTRESOURCE(IDD_CHECKFORUPDATES), nullptr, About, 0);

	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_TGITINSTALLER));

	// Hauptnachrichtenschleife:
	/*while (GetMessage(&msg, NULL, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}*/

	return 0;
}

void MarkWindowAsUnpinnable(HWND hWnd)
{
	typedef HRESULT(WINAPI *SHGPSFW) (HWND hwnd, REFIID riid, void** ppv);

	HMODULE hShell = AtlLoadSystemLibraryUsingFullPath(_T("Shell32.dll"));

	if (hShell) {
		SHGPSFW pfnSHGPSFW = (SHGPSFW)::GetProcAddress(hShell, "SHGetPropertyStoreForWindow");
		if (pfnSHGPSFW) {
			IPropertyStore *pps;
			HRESULT hr = pfnSHGPSFW(hWnd, IID_PPV_ARGS(&pps));
			if (SUCCEEDED(hr)) {
				PROPVARIANT var;
				var.vt = VT_BOOL;
				var.boolVal = VARIANT_TRUE;
				pps->SetValue(PKEY_AppUserModel_PreventPinning, var);
				pps->Release();
			}
		}
		FreeLibrary(hShell);
	}
}

//
//  FUNKTION: MyRegisterClass()
//
//  ZWECK: Registriert die Fensterklasse.
//
/*ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex = { 0 };

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_TGIT));
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszClassName = szWindowClass;
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_TGIT));

	return RegisterClassEx(&wcex);
}*/

//
//   FUNKTION: InitInstance(HINSTANCE, int)
//
//   ZWECK: Speichert das Instanzenhandle und erstellt das Hauptfenster.
//
//   KOMMENTARE:
//
//        In dieser Funktion wird das Instanzenhandle in einer globalen Variablen gespeichert, und das
//        Hauptprogrammfenster wird erstellt und angezeigt.
//
/*BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   HWND hWnd;

   hInst = hInstance; // Instanzenhandle in der globalen Variablen speichern

   hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}*/

//
//  FUNKTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  ZWECK:  Verarbeitet Meldungen vom Hauptfenster.
//
//  WM_COMMAND	- Verarbeiten des Anwendungsmenüs
//  WM_PAINT	- Zeichnen des Hauptfensters
//  WM_DESTROY	- Beenden-Meldung anzeigen und zurückgeben
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;

	switch (message)
	{
	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		// Menüauswahl bearbeiten:
		switch (wmId)
		{
		case IDM_ABOUT:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			break;
		/*case IDM_EXIT:
			DestroyWindow(hWnd);
			break;*/
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		// TODO: Hier den Zeichnungscode hinzufügen.
		EndPaint(hWnd, &ps);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

void FillDownloads(CSimpleIni& versioncheck, const CString version)
{
	BOOL bIsWow64 = FALSE;
	CString x86x64 = _T("32");
	IsWow64Process(GetCurrentProcess(), &bIsWow64);
	if (bIsWow64)
		x86x64 = _T("64");

	CString m_sFilesURL = versioncheck.GetValue(L"tortoisegit", L"baseurl");
	if (m_sFilesURL.IsEmpty())
		m_sFilesURL.Format(_T("http://updater.download.tortoisegit.org/tgit/%s/"), (LPCTSTR)version);

	CString filenamePattern;
	{
		LVITEM listItem = { 0 };
		listItem.mask = LVIF_TEXT | LVIF_PARAM;
		listItem.iItem = 0;
		listItem.iSubItem = 0;
		listItem.pszText = L"TortoiseGit";
		CString filenameMain = versioncheck.GetValue(L"tortoisegit", L"mainfilename");
		if (filenamePattern.IsEmpty())
			filenamePattern = _T("TortoiseGit-%1!s!-%2!s!bit.msi");
		filenameMain.FormatMessage(filenamePattern, version, x86x64);
		//listItem.lParam = new CUpdateListCtrl::Entry(filenameMain, CUpdateListCtrl::STATUS_NONE);
		ListView_InsertItem(listview, &listItem);
		ListView_SetCheckState(listview, 0, TRUE);
	}

	struct LangPack
	{
		CString m_PackName;
		CString m_LangName;
		DWORD m_LocaleID;
		CString m_LangCode;
		bool m_Installed;
	};
	struct LanguagePacks
	{
		std::vector<LangPack> availableLangs;
		std::vector<DWORD> installedLangs;
	} languagePacks;
	{
		// set up the language selecting combobox
		/*CString path = CPathUtils::GetAppParentDirectory();
		path = path + _T("Languages\\");
		CSimpleFileFind finder(path, _T("*.dll"));
		while (finder.FindNextFileNoDirectories())
		{
			CString file = finder.GetFilePath();
			CString filename = finder.GetFileName();
			if (filename.Left(12).CompareNoCase(_T("TortoiseProc")) == 0)
			{
				CString sVer = _T(STRPRODUCTVER);
				sVer = sVer.Left(sVer.ReverseFind('.'));
				CString sFileVer = CPathUtils::GetVersionFromFile(file);
				sFileVer = sFileVer.Left(sFileVer.ReverseFind('.'));
				CString sLoc = filename.Mid(12);
				sLoc = sLoc.Left(sLoc.GetLength() - 4); // cut off ".dll"
				if ((sLoc.Left(2) == L"32") && (sLoc.GetLength() > 5))
					continue;
				DWORD loc = _tstoi(filename.Mid(12));
				languagePacks.installedLangs.push_back(loc);
			}
		}*/
	}

	CSimpleIni::TNamesDepend values;
	versioncheck.GetAllValues(L"tortoisegit", L"langs", values);
	for (const auto& value : values)
	{
		CString langs(value);
		langs.Trim(L'"');

		int nextTokenPos = langs.Find(_T(";"), 5); // be extensible for the future
		if (nextTokenPos > 0)
			langs = langs.Left(nextTokenPos);
		CString sLang = _T("TortoiseGit Language Pack ") + langs.Mid(5);

		DWORD loc = _tstoi(langs.Mid(0, 4));
		TCHAR buf[MAX_PATH] = { 0 };
		GetLocaleInfo(loc, LOCALE_SNATIVELANGNAME, buf, _countof(buf));
		CString sLang2(buf);
		GetLocaleInfo(loc, LOCALE_SNATIVECTRYNAME, buf, _countof(buf));
		if (buf[0])
		{
			sLang2 += _T(" (");
			sLang2 += buf;
			sLang2 += _T(")");
		}

		bool installed = std::find(languagePacks.installedLangs.cbegin(), languagePacks.installedLangs.cend(), loc) != languagePacks.installedLangs.cend();
		LangPack pack = { sLang, sLang2, loc, langs.Mid(5), installed };
		languagePacks.availableLangs.push_back(pack);
	}
	std::stable_sort(languagePacks.availableLangs.begin(), languagePacks.availableLangs.end(), [&](const LangPack& a, const LangPack& b) -> int
	{
		return (a.m_Installed && !b.m_Installed) ? 1 : (!a.m_Installed && b.m_Installed) ? 0 : (a.m_PackName.Compare(b.m_PackName) < 0);
	});
	filenamePattern = versioncheck.GetValue(L"tortoisegit", L"languagepackfilename");
	if (filenamePattern.IsEmpty())
		filenamePattern = _T("TortoiseGit-LanguagePack-%1!s!-%2!s!bit-%3!s!.msi");
	int cnt = 0;
	for (auto& langs : languagePacks.availableLangs)
	{
		//int pos = m_ctrlFiles.InsertItem(m_ctrlFiles.GetItemCount(), langs.m_PackName);
		//m_ctrlFiles.SetItemText(pos, 1, langs.m_LangName);

		LVITEM listItem = { 0 };
		listItem.mask = LVIF_TEXT;
		listItem.iItem = ++cnt;
		listItem.iSubItem = 0;
		listItem.pszText = langs.m_PackName.GetBuffer();
		ListView_InsertItem(listview, &listItem);
		langs.m_PackName.ReleaseBuffer();
		/*listItem.mask |= LVCF_SUBITEM;
		listItem.iSubItem = 1;
		listItem.pszText = L"de";
		ListView_SetItem(listview, &listItem);*/

		CString filename;
		filename.FormatMessage(filenamePattern, version, x86x64, langs.m_LangCode, langs.m_LocaleID);
		//m_ctrlFiles.SetItemData(pos, (DWORD_PTR)(new CUpdateListCtrl::Entry(filename, CUpdateListCtrl::STATUS_NONE)));

		/*if (langs.m_Installed)
			m_ctrlFiles.SetCheck(pos, TRUE);*/
	}
	//DialogEnableWindow(IDC_BUTTON_UPDATE, TRUE);
}

CString GetWinINetError(DWORD err)
{
	CString readableError = CFormatMessageWrapper(err);
	if (readableError.IsEmpty())
	{
		for (const CString& module : { _T("wininet.dll"), _T("urlmon.dll") })
		{
			LPTSTR buffer;
			FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_FROM_HMODULE, GetModuleHandle(module), err, 0, (LPTSTR)&buffer, 0, NULL);
			readableError = buffer;
			LocalFree(buffer);
			if (!readableError.IsEmpty())
				break;
		}
	}
	return readableError.Trim();
}

DWORD WINAPI CheckThread(LPVOID lpParameter)
{
	HWND hDlg = (HWND)lpParameter;

	CString sCheckURL = _T("https://versioncheck.tortoisegit.org/version.txt");

	CString tempfile = GetTempFile();
	CString errorText, temp;
	CString ver;
	CSimpleIni versioncheck(true, true);
	DWORD ret = m_updateDownloader->DownloadFile(sCheckURL, tempfile, false);
	if (!ret)
	{
		CString signatureTempfile = GetTempFile();
		ret = m_updateDownloader->DownloadFile(sCheckURL + SIGNATURE_FILE_ENDING, signatureTempfile, false);
		if (ret || VerifyIntegrity(tempfile, signatureTempfile, m_updateDownloader))
		{
			CString error = _T("Could not verify digital signature.");
			if (ret)
				error += _T("\r\nError: ") + GetWinINetError(ret) + _T(" (on ") + sCheckURL + SIGNATURE_FILE_ENDING + _T(")");
			//SetDlgItemText(IDC_CHECKRESULT, error);
			DeleteUrlCacheEntry(sCheckURL);
			DeleteUrlCacheEntry(sCheckURL + SIGNATURE_FILE_ENDING);
			goto finish;
		}
	}
	else if (ret)
	{
		DeleteUrlCacheEntry(sCheckURL);
		/*if (CRegDWORD(_T("Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings\\GlobalUserOffline"), 0))
			errorText.LoadString(IDS_OFFLINEMODE); // offline mode enabled
		else
			errorText.Format(IDS_CHECKNEWER_NETERROR_FORMAT, (LPCTSTR)(GetWinINetError(ret) + _T(" URL: ") + sCheckURL), ret);
		SetDlgItemText(IDC_CHECKRESULT, errorText);*/
		goto finish;
	}

	versioncheck.LoadFile(tempfile);

	unsigned int major, minor, micro, build;
	major = minor = micro = build = 0;
	unsigned __int64 version = 0;

	ver = versioncheck.GetValue(L"tortoisegit", L"version");
	if (!ver.IsEmpty())
	{
		CString vertemp = ver;
		major = _ttoi(vertemp);
		vertemp = vertemp.Mid(vertemp.Find('.') + 1);
		minor = _ttoi(vertemp);
		vertemp = vertemp.Mid(vertemp.Find('.') + 1);
		micro = _ttoi(vertemp);
		vertemp = vertemp.Mid(vertemp.Find('.') + 1);
		build = _ttoi(vertemp);
		version = major;
		version <<= 16;
		version += minor;
		version <<= 16;
		version += micro;
		version <<= 16;
		version += build;

		if (version == 0)
		{
			/*temp.LoadString(IDS_CHECKNEWER_NETERROR);
			SetDlgItemText(IDC_CHECKRESULT, temp);*/
			goto finish;
		}

		// another versionstring for the filename can be provided
		// this is needed for preview releases
		vertemp.Empty();
		vertemp = versioncheck.GetValue(L"tortoisegit", L"versionstring");
		if (!vertemp.IsEmpty())
			ver = vertemp;
	}
	else
	{
		/*errorText = _T("Could not parse version check file: ") + g_Git.GetLibGit2LastErr();
		SetDlgItemText(IDC_CHECKRESULT, errorText);*/
		DeleteUrlCacheEntry(sCheckURL);
		goto finish;
	}

	{
		CString versionstr;
		versionstr.Format(_T("%u.%u.%u.%u"), major, minor, micro, build);
		if (versionstr != ver)
			versionstr += _T(" (") + ver + _T(")");
		/*temp.Format(IDS_CHECKNEWER_CURRENTVERSION, (LPCTSTR)versionstr);
		SetDlgItemText(IDC_CURRENTVERSION, temp);*/

			temp = versioncheck.GetValue(L"tortoisegit", L"infotext");
			if (!temp.IsEmpty())
			{
				CString tempLink = versioncheck.GetValue(L"tortoisegit", L"infotexturl");
				//if (!tempLink.IsEmpty()) // find out the download link-URL, if any
					//m_sUpdateDownloadLink = tempLink;
			}
			//else
				//temp.LoadString(IDS_CHECKNEWER_NEWERVERSIONAVAILABLE);
			//SetDlgItemText(IDC_CHECKRESULT, temp);

			FillDownloads(versioncheck, ver);

			// Show download controls
			/*RECT rectWindow, rectProgress, rectGroupDownloads, rectOKButton;
			GetWindowRect(&rectWindow);
			m_progress.GetWindowRect(&rectProgress);
			GetDlgItem(IDC_GROUP_DOWNLOADS)->GetWindowRect(&rectGroupDownloads);
			GetDlgItem(IDOK)->GetWindowRect(&rectOKButton);
			LONG bottomDistance = rectWindow.bottom - rectOKButton.bottom;
			OffsetRect(&rectOKButton, 0, (rectGroupDownloads.bottom + (rectGroupDownloads.bottom - rectProgress.bottom)) - rectOKButton.top);
			rectWindow.bottom = rectOKButton.bottom + bottomDistance;
			MoveWindow(&rectWindow);
			::MapWindowPoints(NULL, GetSafeHwnd(), (LPPOINT)&rectOKButton, 2);
			if (CRegDWORD(_T("Software\\TortoiseGit\\VersionCheck"), TRUE) != FALSE && !m_bForce && !m_bShowInfo)
			{
				GetDlgItem(IDC_DONOTASKAGAIN)->ShowWindow(SW_SHOW);
				rectOKButton.left += 60;
				temp.LoadString(IDS_REMINDMELATER);
				GetDlgItem(IDOK)->SetWindowText(temp);
				rectOKButton.right += 160;
			}
			GetDlgItem(IDOK)->MoveWindow(&rectOKButton);
			m_ctrlFiles.ShowWindow(SW_SHOW);
			GetDlgItem(IDC_GROUP_DOWNLOADS)->ShowWindow(SW_SHOW);
			CenterWindow();
			m_bShowInfo = TRUE;*/
	}

finish:
	/*if (!m_sUpdateDownloadLink.IsEmpty())
	{
		m_link.ShowWindow(SW_SHOW);
		m_link.SetURL(m_sUpdateDownloadLink);
	}*/
	//m_bThreadRunning = FALSE;
	//DialogEnableWindow(IDOK, TRUE);
	/*Sleep(2500);*/
	HWND listview = GetDlgItem(hDlg, IDC_LIST_DOWNLOADS);
	ShowWindow(listview, SW_SHOW);
	return 0;
}

// Meldungshandler für Infofeld.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
	{
		MarkWindowAsUnpinnable(hDlg);
		listview = GetDlgItem(hDlg, IDC_LIST_DOWNLOADS);
		ListView_SetExtendedListViewStyle(listview, LVS_EX_DOUBLEBUFFER | LVS_EX_CHECKBOXES);
		LVCOLUMN column = { 0 };
		column.cx = 350;
		column.mask = LVCF_WIDTH;
		column.iOrder = 0;
		ListView_InsertColumn(listview, 0, &column);
		column.cx = 200;
		column.iOrder = 1;
		ListView_InsertColumn(listview, 1, &column);
		ShowWindow(listview, SW_HIDE);

		// hide download controls
		/*m_ctrlFiles.ShowWindow(SW_HIDE);
		GetDlgItem(IDC_GROUP_DOWNLOADS)->ShowWindow(SW_HIDE);
		RECT rectWindow, rectGroupDownloads, rectOKButton;
		GetWindowRect(&rectWindow);
		GetDlgItem(IDC_GROUP_DOWNLOADS)->GetWindowRect(&rectGroupDownloads);
		GetDlgItem(IDOK)->GetWindowRect(&rectOKButton);
		LONG bottomDistance = rectWindow.bottom - rectOKButton.bottom;
		OffsetRect(&rectOKButton, 0, rectGroupDownloads.top - rectOKButton.top);
		rectWindow.bottom = rectOKButton.bottom + bottomDistance;
		MoveWindow(&rectWindow);
		::MapWindowPoints(NULL, GetSafeHwnd(), (LPPOINT)&rectOKButton, 2);
		GetDlgItem(IDOK)->MoveWindow(&rectOKButton);*/

		CAutoGeneralHandle m_eventStop = CreateEvent(nullptr, TRUE, FALSE, nullptr);
		m_updateDownloader = new CUpdateDownloader(hDlg, FALSE, WM_USER_DISPLAYSTATUS, m_eventStop);

		//https://msdn.microsoft.com/en-us/library/windows/desktop/ms682516%28v=vs.85%29.aspx
		CAutoGeneralHandle thread = CreateThread(nullptr, 0, CheckThread, hDlg, 0, nullptr);

	}
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	case WM_USER_DISPLAYSTATUS:
		break;
	}

	return (INT_PTR)FALSE;
}

// https://stackoverflow.com/questions/10282964/how-can-i-update-a-progress-bar
//InsertColumn