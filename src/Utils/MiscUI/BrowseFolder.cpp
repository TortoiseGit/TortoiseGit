// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2016 - TortoiseGit
// Copyright (C) 2003-2014, 2016 - TortoiseSVN

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
#include <windowsx.h>
#include "BrowseFolder.h"
#include "SmartHandle.h"
#include <strsafe.h>

BOOL CBrowseFolder::m_bCheck = FALSE;
BOOL CBrowseFolder::m_bCheck2 = FALSE;
WNDPROC CBrowseFolder::CBProc = NULL;
HWND CBrowseFolder::checkbox = NULL;
HWND CBrowseFolder::checkbox2 = NULL;
HWND CBrowseFolder::ListView = NULL;
TCHAR CBrowseFolder::m_CheckText[200];
TCHAR CBrowseFolder::m_CheckText2[200];
CString CBrowseFolder::m_sDefaultPath;
bool CBrowseFolder::m_DisableCheckbox2WhenCheckbox1IsChecked = false;


CBrowseFolder::CBrowseFolder(void)
:	m_style(0),
	m_root(NULL)
{
	SecureZeroMemory(m_displayName, sizeof(m_displayName));
	SecureZeroMemory(m_title, sizeof(m_title));
	SecureZeroMemory(m_CheckText, sizeof(m_CheckText));
}

CBrowseFolder::~CBrowseFolder(void)
{
}

//show the dialog
CBrowseFolder::retVal CBrowseFolder::Show(HWND parent, LPTSTR path, size_t pathlen, LPCTSTR szDefaultPath /* = NULL */)
{
	CString temp;
	temp = path;
	CString sDefault;
	if (szDefaultPath)
		sDefault = szDefaultPath;
	CBrowseFolder::retVal ret = Show(parent, temp, sDefault);
	_tcscpy_s(path, pathlen, temp);
	return ret;
}
CBrowseFolder::retVal CBrowseFolder::Show(HWND parent, CString& path, const CString& sDefaultPath /* = CString() */)
{
	m_sDefaultPath = sDefaultPath;
	if (m_sDefaultPath.IsEmpty() && !path.IsEmpty())
	{
		while (!PathFileExists(path) && !path.IsEmpty())
		{
			CString p = path.Left(path.ReverseFind(L'\\'));
			if ((p.GetLength() == 2) && (p[1] == L':'))
			{
				p += L"\\";
				if (p.Compare(path) == 0)
					p.Empty();
			}
			if (p.GetLength() < 2)
				p.Empty();
			path = p;
		}
		// if the result path already contains a path, use that as the default path
		m_sDefaultPath = path;
	}

	// Create a new common open file dialog
	CComPtr<IFileOpenDialog> pfd;
	if (FAILED(pfd.CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER)))
	{
		BROWSEINFO browseInfo = {};
		browseInfo.hwndOwner = parent;
		browseInfo.pidlRoot = m_root;
		browseInfo.pszDisplayName = m_displayName;
		browseInfo.lpszTitle = m_title;
		browseInfo.ulFlags = m_style;
		browseInfo.lParam = (LPARAM)this;
		browseInfo.lpfn = BrowseCallBackProc;

		PCIDLIST_ABSOLUTE itemIDList = SHBrowseForFolder(&browseInfo);
		//is the dialog canceled?
		if (!itemIDList)
			return CANCEL;
		SCOPE_EXIT{ CoTaskMemFree((LPVOID)itemIDList); };

		if (!SHGetPathFromIDList(itemIDList, CStrBuf(path, MAX_PATH)))		// MAX_PATH ok. Explorer can't handle paths longer than MAX_PATH.
			return NOPATH;

		return OK;
	}

	// Set the dialog as a folder picker
	DWORD dwOptions;
	if (FAILED(pfd->GetOptions(&dwOptions)))
		return CANCEL;
	if (FAILED(pfd->SetOptions(dwOptions | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM | FOS_PATHMUSTEXIST)))
		return CANCEL;

	// Set a title
	TCHAR* nl = _tcschr(m_title, '\n');
	if (nl)
		*nl = 0;
	pfd->SetTitle(m_title);

	// set the default folder
	CComPtr<IShellItem> psiDefault;
	if (FAILED(SHCreateItemFromParsingName(m_sDefaultPath, nullptr, IID_PPV_ARGS(&psiDefault))))
		return CANCEL;
	if (FAILED(pfd->SetFolder(psiDefault)))
		return CANCEL;

	if (m_CheckText[0] != 0)
	{
		CComPtr<IFileDialogCustomize> pfdCustomize;
		if (FAILED(pfd.QueryInterface(&pfdCustomize)))
			return CANCEL;

		pfdCustomize->StartVisualGroup(100, L"");
		pfdCustomize->AddCheckButton(101, m_CheckText, FALSE);
		if (m_CheckText2[0] != 0)
			pfdCustomize->AddCheckButton(102, m_CheckText2, FALSE);
		pfdCustomize->EndVisualGroup();
	}

	// Show the open file dialog
	if (FAILED(pfd->Show(parent)))
		return CANCEL;

	// Get the selection from the user
	CComPtr<IShellItem> psiResult;
	if (FAILED(pfd->GetResult(&psiResult)))
		return CANCEL;

	PWSTR pszPath = nullptr;
	if (SUCCEEDED(psiResult->GetDisplayName(SIGDN_FILESYSPATH, &pszPath)))
	{
		path = pszPath;
		CoTaskMemFree(pszPath);
	}

	CComPtr<IFileDialogCustomize> pfdCustomize;
	if (SUCCEEDED(pfd.QueryInterface(&pfdCustomize)))
	{
		pfdCustomize->GetCheckButtonState(101, &m_bCheck);
		pfdCustomize->GetCheckButtonState(102, &m_bCheck2);
	}

	return OK;
}

void CBrowseFolder::SetInfo(LPCTSTR title)
{
	ASSERT(title);

	if (title)
		_tcscpy_s(m_title, title);
}

void CBrowseFolder::SetCheckBoxText(LPCTSTR checktext)
{
	ASSERT(checktext);

	if (checktext)
		_tcscpy_s(m_CheckText, checktext);
}

void CBrowseFolder::SetCheckBoxText2(LPCTSTR checktext)
{
	ASSERT(checktext);

	if (checktext)
		_tcscpy_s(m_CheckText2, checktext);
}

void CBrowseFolder::SetFont(HWND hwnd,LPTSTR FontName,int FontSize)
{

	HFONT hf;
	LOGFONT lf={0};
	HDC hdc=GetDC(hwnd);

	GetObject(GetWindowFont(hwnd),sizeof(lf),&lf);
	lf.lfWeight = FW_REGULAR;
	lf.lfHeight = (LONG)FontSize;
	StringCchCopy( lf.lfFaceName, _countof(lf.lfFaceName), FontName );
	hf=CreateFontIndirect(&lf);
	SetBkMode(hdc,OPAQUE);
	SendMessage(hwnd,WM_SETFONT,(WPARAM)hf,TRUE);
	ReleaseDC(hwnd,hdc);

}

int CBrowseFolder::BrowseCallBackProc(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM /*lpData*/)
{
	//Initialization callback message
	if (uMsg == BFFM_INITIALIZED)
	{
		if (m_CheckText[0] != 0)
		{
			bool bSecondCheckbox = (m_CheckText2[0] != 0);
			//Rectangles for getting the positions
			checkbox = CreateWindowEx(	0,
				_T("BUTTON"),
				m_CheckText,
				WS_CHILD|WS_VISIBLE|WS_CLIPCHILDREN|BS_AUTOCHECKBOX,
				0,100,100,50,
				hwnd,
				0,
				NULL,
				NULL);
			if (checkbox == NULL)
				return 0;

			if (bSecondCheckbox)
			{
				//Rectangles for getting the positions
				checkbox2 = CreateWindowEx(	0,
					_T("BUTTON"),
					m_CheckText2,
					WS_CHILD|WS_VISIBLE|WS_CLIPCHILDREN|BS_AUTOCHECKBOX,
					0,100,100,50,
					hwnd,
					0,
					NULL,
					NULL);
				if (checkbox2 == NULL)
					return 0;
			}

			ListView = FindWindowEx(hwnd,NULL,_T("SysTreeView32"),NULL);
			if (ListView == NULL)
				ListView = FindWindowEx(hwnd,NULL,_T("SHBrowseForFolder ShellNameSpace Control"),NULL);

			if (ListView == NULL)
				return 0;

			//Gets the dimensions of the windows
			const int controlHeight = ::GetSystemMetrics(SM_CYMENUCHECK) + 4;
			RECT listViewRect;
			GetWindowRect(ListView, &listViewRect);
			POINT pt;
			pt.x = listViewRect.left;
			pt.y = listViewRect.top;
			ScreenToClient(hwnd, &pt);
			listViewRect.top = pt.y;
			listViewRect.left = pt.x;
			pt.x = listViewRect.right;
			pt.y = listViewRect.bottom;
			ScreenToClient(hwnd, &pt);
			listViewRect.bottom = pt.y;
			listViewRect.right = pt.x;
			//Sets the list view controls dimensions
			SetWindowPos(ListView, 0, listViewRect.left,
				bSecondCheckbox ? listViewRect.top + (2 * controlHeight) : listViewRect.top + controlHeight,
				(listViewRect.right - listViewRect.left),
				bSecondCheckbox ? (listViewRect.bottom - listViewRect.top) - (2 * controlHeight) : (listViewRect.bottom - listViewRect.top) - controlHeight,
				SWP_NOZORDER);
			//Sets the window positions of checkbox and dialog controls
			SetWindowPos(checkbox, HWND_BOTTOM, listViewRect.left,
				listViewRect.top,
				(listViewRect.right - listViewRect.left),
				controlHeight,
				SWP_NOZORDER);
			if (bSecondCheckbox)
			{
				SetWindowPos(checkbox2, HWND_BOTTOM, listViewRect.left,
					listViewRect.top + controlHeight,
					(listViewRect.right - listViewRect.left),
					controlHeight,
					SWP_NOZORDER);
			}
			HWND label = FindWindowEx(hwnd, NULL, _T("STATIC"), NULL);
			if (label)
			{
				HFONT hFont = (HFONT)::SendMessage(label, WM_GETFONT, 0, 0);
				LOGFONT lf = {0};
				GetObject(hFont, sizeof(lf), &lf);
				HFONT hf2 = CreateFontIndirect(&lf);
				::SendMessage(checkbox, WM_SETFONT, (WPARAM)hf2, TRUE);
				if (bSecondCheckbox)
					::SendMessage(checkbox2, WM_SETFONT, (WPARAM)hf2, TRUE);
			}
			else
			{
				//Sets the fonts of static controls
				SetFont(checkbox,_T("MS Sans Serif"),12);
				if (bSecondCheckbox)
					SetFont(checkbox2,_T("MS Sans Serif"),12);
			}

			// Subclass the checkbox control.
			CBProc = (WNDPROC) SetWindowLongPtr(checkbox,GWLP_WNDPROC, (LONG_PTR) CheckBoxSubclassProc);
			//Sets the checkbox to checked position
			SendMessage(checkbox,BM_SETCHECK,(WPARAM)m_bCheck,0);
			if (bSecondCheckbox)
			{
				CBProc = (WNDPROC) SetWindowLongPtr(checkbox2,GWLP_WNDPROC, (LONG_PTR) CheckBoxSubclassProc2);
				SendMessage(checkbox2,BM_SETCHECK,(WPARAM)m_bCheck,0);
			}
			// send a resize message to the resized list view control. Otherwise it won't show
			// up properly until the user resizes the window!
			SendMessage(ListView, WM_SIZE, SIZE_RESTORED, MAKELONG(listViewRect.right - listViewRect.left, bSecondCheckbox ? (listViewRect.bottom - listViewRect.top) - 40 : (listViewRect.bottom - listViewRect.top) - 20));
		}

		// now set the default directory
		SendMessage(hwnd, BFFM_SETSELECTION, TRUE, (LPARAM)(LPCTSTR)m_sDefaultPath);
	}
	if (uMsg == BFFM_SELCHANGED)
	{
		// Set the status window to the currently selected path.
		TCHAR szDir[MAX_PATH] = {0};
		if (SHGetPathFromIDList((PCIDLIST_ABSOLUTE)lParam, szDir))
		{
			SendMessage(hwnd,BFFM_SETSTATUSTEXT, 0, (LPARAM)szDir);
		}
		else
			return BFFM_VALIDATEFAILED;
	}
	if (uMsg == BFFM_VALIDATEFAILED)
		return 1; // DONT_DISMISS

	return 0;
}

LRESULT CBrowseFolder::CheckBoxSubclassProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	if (uMsg == WM_LBUTTONUP)
	{
		m_bCheck = (SendMessage(hwnd,BM_GETCHECK,0,0)==BST_UNCHECKED);
		if (m_bCheck && m_DisableCheckbox2WhenCheckbox1IsChecked)
		{
			::EnableWindow(checkbox2, !m_bCheck);
		}
		else
			::EnableWindow(checkbox2, true);
	}

	return CallWindowProc(CBProc, hwnd, uMsg,
		wParam, lParam);
}

LRESULT CBrowseFolder::CheckBoxSubclassProc2(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	if (uMsg == WM_LBUTTONUP)
	{
		m_bCheck2 = (SendMessage(hwnd,BM_GETCHECK,0,0)==BST_UNCHECKED);
	}

	return CallWindowProc(CBProc, hwnd, uMsg,
		wParam, lParam);
}
