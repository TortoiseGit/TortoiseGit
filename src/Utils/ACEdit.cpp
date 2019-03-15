// TortoiseGit - a Windows shell extension for easy version control

// Copyright (c) 2003 by Andreas Kapust <info@akinstaller.de>; <http://www.codeproject.com/Articles/2607/AutoComplete-without-IAutoComplete>
// Copyright (C) 2009, 2012-2013, 2015-2016, 2018-2019 - TortoiseGit

// Licensed under: The Code Project Open License (CPOL); <http://www.codeproject.com/info/cpol10.aspx>

// ACEdit.cpp: Implementierungsdatei
//

#include "stdafx.h"
#include "ACEdit.h"
#include  <io.h>
#include "StringUtils.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define _EDIT_ 1
#define _COMBOBOX_ 2

/////////////////////////////////////////////////////////////////////////////
// CACEdit

CACEdit::CACEdit()
{
	m_iMode = _MODE_STANDARD_;
	m_iType = -1;
	m_pEdit = nullptr;
	m_CursorMode = false;
	m_PrefixChar = L'\0';
	m_szDrive[0] = L'\0';
	m_szDir[0] = L'\0';
	m_szFname[0] = L'\0';
	m_szExt[0] = L'\0';
}

/*********************************************************************/

CACEdit::~CACEdit()
{
	DestroyWindow();
}

/*********************************************************************/

BEGIN_MESSAGE_MAP(CACEdit, CWnd)
	//{{AFX_MSG_MAP(CACEdit)
	ON_CONTROL_REFLECT(EN_KILLFOCUS, OnKillfocus)
	ON_CONTROL_REFLECT(CBN_KILLFOCUS, OnKillfocus)
	ON_WM_KEYDOWN()
	ON_CONTROL_REFLECT(EN_CHANGE, OnChange)
	ON_CONTROL_REFLECT(CBN_EDITCHANGE, OnChange)
	ON_CONTROL_REFLECT(CBN_DROPDOWN, OnCloseList)
	//}}AFX_MSG_MAP
	ON_MESSAGE(ENAC_UPDATE,OnUpdateFromList)
END_MESSAGE_MAP()

/*********************************************************************/

void CACEdit::SetMode(int iMode)
{
	if(m_iType == -1)
		Init();

	m_iMode = iMode;

	/*
	** Vers. 1.1
	** NEW: _MODE_CURSOR_O_LIST_
	*/
	if(iMode == _MODE_CURSOR_O_LIST_)
		m_iMode |= _MODE_STANDARD_;

	if(iMode & _MODE_FILESYSTEM_)
		m_SeparationStr = L'\\';

	// Vers. 1.2
	if(iMode & _MODE_FIND_ALL_)
	{
		m_Liste.m_lMode |= _MODE_FIND_ALL_;
	}
}

/*********************************************************************/

void CACEdit::Init()
{
	CString szClassName = AfxRegisterWndClass(CS_CLASSDC|CS_SAVEBITS|CS_HREDRAW|CS_VREDRAW,
		0, reinterpret_cast<HBRUSH>(COLOR_WINDOW), 0);
	CRect rcWnd,rcWnd1;
	GetWindowRect(rcWnd);

	VERIFY(m_Liste.CreateEx(WS_EX_TOOLWINDOW,
		szClassName, nullptr,
		WS_THICKFRAME | WS_CHILD | WS_BORDER |
		WS_CLIPSIBLINGS | WS_OVERLAPPED,
		CRect(rcWnd.left, rcWnd.top +20, rcWnd.left+ 200, rcWnd.top+200),
		GetDesktopWindow(),
		0x3E8, nullptr));

	CString m_ClassName;
	::GetClassName(GetSafeHwnd(), CStrBuf(m_ClassName, 32), 32);

	if (m_ClassName.Compare(L"Edit") == 0)
		m_iType = _EDIT_;
	else
	{
		if (m_ClassName.Compare(L"ComboBox") == 0)
		{
			m_iType = _COMBOBOX_;

			m_pEdit = static_cast<CEdit*>(GetWindow(GW_CHILD));
			VERIFY(m_pEdit);
			::GetClassName(m_pEdit->GetSafeHwnd(), CStrBuf(m_ClassName, 32), 32);
			VERIFY(m_ClassName.Compare(L"Edit") == 0);
		}
	}

	if(m_iType == -1)
	{
		ASSERT(0);
		return;
	}

	m_Liste.Init(this);
}

/*********************************************************************/

void CACEdit::AddSearchStrings(LPCTSTR Strings[])
{
	int i = 0;
	LPCTSTR str;
	if(m_iType == -1) {ASSERT(0); return;}

	m_Liste.RemoveAll();

	do
	{
		str = Strings[i];
		if(str)
		{
			m_Liste.AddSearchString(str);
		}

		i++;
	}
	while(str);

	m_Liste.SortSearchList();
}

/*********************************************************************/

void CACEdit::AddSearchString(LPCTSTR lpszString)
{
	if(m_iType == -1) {ASSERT(0); return;}

	m_Liste.AddSearchString(lpszString);
}

/*********************************************************************/

void CACEdit::RemoveSearchAll()
{
	if(m_iType == -1) {ASSERT(0); return;}

	m_Liste.RemoveAll();
}

/*********************************************************************/

void CACEdit::OnKillfocus()
{
	if(m_Liste.GetSafeHwnd()) // fix Vers 1.1
		m_Liste.ShowWindow(false);
}

/*********************************************************************/

void CACEdit::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	if(!HandleKey(nChar,false))
		CWnd::OnKeyDown(nChar, nRepCnt, nFlags);
}

/*********************************************************************/

bool CACEdit::HandleKey(UINT nChar, bool m_bFromChild)
{
	if (nChar == VK_ESCAPE || nChar == VK_RETURN && !m_Liste.IsWindowVisible())
	{
		m_Liste.ShowWindow(false);
		return true;
	}

	if (nChar == VK_DOWN || nChar == VK_UP
		|| nChar == VK_PRIOR || nChar == VK_NEXT
		|| nChar == VK_HOME || nChar == VK_END || nChar == VK_RETURN)
	{
		/*
		** Vers. 1.1
		** NEW: _MODE_CURSOR_O_LIST_
		*/
		if(!m_Liste.IsWindowVisible() && (m_iMode & _MODE_CURSOR_O_LIST_))
		{
			GetWindowText(m_EditText);
			if(m_EditText.IsEmpty())
			{
				m_Liste.CopyList();
				return true;
			}
		}

		if(m_Liste.IsWindowVisible())
		{
			int pos;


			if(m_iMode & _MODE_STANDARD_
				|| m_iMode & _MODE_FILESYSTEM_
				|| m_iMode & _MODE_FS_START_DIR_)
			{
				m_CursorMode = true;

				if(!m_bFromChild)
					m_EditText = m_Liste.GetNextString(nChar);
				else
					m_EditText = m_Liste.GetString();

				if(m_iMode & _MODE_FILESYSTEM_)
				{
					if (CStringUtils::EndsWith(m_EditText, L'\\'))
						m_EditText = m_EditText.Left(m_EditText.GetLength() - 1);
				}

				if (nChar != VK_RETURN)
					m_Liste.SelectItem(m_Liste.GetSelectedItem());
				else
				{
					m_Liste.SelectItem(-1);
					SetWindowText(m_EditText);
					pos = m_EditText.GetLength();

					if (m_iType == _COMBOBOX_)
					{
						m_pEdit->SetSel(pos, pos, true);
						m_pEdit->SetModify(true);
					}

					if (m_iType == _EDIT_)
					{
						reinterpret_cast<CEdit*>(this)->SetSel(pos, pos, true);
						reinterpret_cast<CEdit*>(this)->SetModify(true);
					}
				}

				GetParent()->SendMessage(ENAC_UPDATE, WM_KEYDOWN, GetDlgCtrlID());
				m_CursorMode = false;
				if (nChar == VK_RETURN)
					m_Liste.ShowWindow(false);
				return true;
			}

			if(m_iMode & _MODE_SEPARATION_)
			{
				CString m_Text,m_Left,m_Right;
				int left,right,pos2=0,len;

				m_CursorMode = true;

				GetWindowText(m_EditText);

				if(m_iType == _EDIT_)
					pos2 = LOWORD(reinterpret_cast<CEdit*>(this)->CharFromPos(GetCaretPos()));

				if(m_iType == _COMBOBOX_)
					pos2 = m_pEdit->CharFromPos(m_pEdit->GetCaretPos());

				left  = FindSepLeftPos(pos2-1,true);
				right = FindSepRightPos(pos2);

				m_Text = m_EditText.Left(left);

				if(!m_bFromChild)
					m_Text += m_Liste.GetNextString(nChar);
				else
					m_Text += m_Liste.GetString();

				if (nChar != VK_RETURN)
					m_Liste.SelectItem(m_Liste.GetSelectedItem());
				else
				{
					m_Liste.SelectItem(-1);
					m_Text += m_EditText.Mid(right);
					len = m_Liste.GetString().GetLength();

					m_Text += this->m_SeparationStr;

					SetWindowText(m_Text);
					GetParent()->SendMessage(ENAC_UPDATE, WM_KEYDOWN, GetDlgCtrlID());

					right = FindSepLeftPos2(pos2 - 1);
					left -= right;
					len += right;

					left += m_SeparationStr.GetLength();

					if (m_iType == _EDIT_)
					{
						reinterpret_cast<CEdit*>(this)->SetModify(true);
						reinterpret_cast<CEdit*>(this)->SetSel(left + len, left + len, false);
					}

					if (m_iType == _COMBOBOX_)
					{
						m_pEdit->SetModify(true);
						m_pEdit->SetSel(left, left + len, true);
					}
				}
				m_CursorMode = false;
				if (nChar == VK_RETURN)
					m_Liste.ShowWindow(false);
				return true;
			}
		}
	}
	return false;
}

/*********************************************************************/

void CACEdit::OnChange()
{
	CString m_Text;
	int pos=0,len;

	if(m_iType == -1)
	{ASSERT(0); return;}

	GetWindowText(m_EditText);
	len = m_EditText.GetLength();
	//----------------------------------------------
	if(m_iMode & _MODE_FILESYSTEM_ || m_iMode & _MODE_FS_START_DIR_)
	{
		if(!m_CursorMode)
		{
			if(m_iType == _EDIT_)
				pos = LOWORD(reinterpret_cast<CEdit*>(this)->CharFromPos(GetCaretPos()));

			if(m_iType == _COMBOBOX_)
				pos = m_pEdit->CharFromPos(m_pEdit->GetCaretPos());

			if(m_iMode & _MODE_FS_START_DIR_)
			{
				if(len)
					m_Liste.FindString(-1,m_EditText);
				else
					m_Liste.ShowWindow(false);
			}
			else
			{
				if(len > 2 && pos == len)
				{
					if(_taccess(m_EditText,0) == 0)
					{
						ReadDirectory(m_EditText);
					}
					m_Liste.FindString(-1,m_EditText);
				}
				else
					m_Liste.ShowWindow(false);
			}
		} // m_CursorMode
	}
	//----------------------------------------------
	if(m_iMode & _MODE_SEPARATION_)
	{
		if(!m_CursorMode)
		{
			if(m_iType == _EDIT_)
				pos = LOWORD(reinterpret_cast<CEdit*>(this)->CharFromPos(GetCaretPos()));

			if(m_iType == _COMBOBOX_)
				pos = m_pEdit->CharFromPos(m_pEdit->GetCaretPos());

			int left,right;
			left  = FindSepLeftPos(pos-1);
			right = FindSepRightPos(pos);
			m_Text = m_EditText.Mid(left,right-left);
			m_Liste.FindString(-1,m_Text);
		}
	}
	//----------------------------------------------
	if(m_iMode & _MODE_STANDARD_)
	{
		if(!m_CursorMode)
			m_Liste.FindString(-1,m_EditText);
	}
	//----------------------------------------------
	GetParent()->SendMessage(ENAC_UPDATE, EN_UPDATE, GetDlgCtrlID());
}

/*********************************************************************/

int CACEdit::FindSepLeftPos(int pos,bool m_bIncludePrefix)
{
	int len = m_EditText.GetLength();
	TCHAR ch;
	int i;

	if(pos >= len && len != 1)
		pos =  len -1;

	for(i = pos; i >= 0 ; i--)
	{
		ch = m_EditText.GetAt(i);
		if(m_PrefixChar == ch)
			return i + (m_bIncludePrefix ? 1 : 0);
		if(m_SeparationStr.Find(ch) != -1)
			break;
	}

	return  i + 1;
}

/*********************************************************************/

int CACEdit::FindSepLeftPos2(int pos)
{
	int len = m_EditText.GetLength();
	TCHAR ch;

	if(pos >= len && len != 1)
		pos =  len -1;

	if(len == 1)
		return 0;

	for(int i = pos; i >= 0 ; i--)
	{
		ch = m_EditText.GetAt(i);
		if(m_PrefixChar == ch)
			return 1;
	}

	return  0;
}

/*********************************************************************/

int CACEdit::FindSepRightPos(int pos)
{
	int len = m_EditText.GetLength();
	TCHAR ch;
	int i;

	for(i = pos; i < len ; i++)
	{
		ch = m_EditText.GetAt(i);
		if(m_SeparationStr.Find(ch) != -1)
			break;
	}

	return i;
}

/*********************************************************************/
LRESULT CACEdit::OnUpdateFromList(WPARAM lParam, LPARAM /*wParam*/)
{
	UpdateData(true);

	if(lParam == WM_KEYDOWN)
		HandleKey(VK_RETURN, true);
	return 0;
}

/*********************************************************************/

void CACEdit::OnCloseList()
{
	m_Liste.ShowWindow(false);
}

/*********************************************************************/

BOOL CACEdit::PreTranslateMessage(MSG* pMsg)
{
	if(pMsg->message == WM_KEYDOWN)
	{
		if(m_Liste.IsWindowVisible())
		{
			if(m_iType == _COMBOBOX_)
			{
				if(pMsg->wParam == VK_DOWN || pMsg->wParam == VK_UP)
					if (HandleKey(static_cast<UINT>(pMsg->wParam), false))
						return true;
			}

			if(pMsg->wParam == VK_ESCAPE || pMsg->wParam == VK_RETURN)
				if (HandleKey(static_cast<UINT>(pMsg->wParam), false))
					return true;
		}
	}
	return CWnd::PreTranslateMessage(pMsg);
}

/*********************************************************************/

void CACEdit::ReadDirectory(CString m_Dir)
{
	CFileFind FoundFiles;
	TCHAR ch;
	CWaitCursor hg;

	// Wenn mittem im Pfad,
	// vorheriges Verzeichnis einlesen.
	if (!CStringUtils::EndsWith(m_Dir, L'\\'))
	{
		_wsplitpath_s(m_Dir, m_szDrive, m_szDir, m_szFname, m_szExt);
		m_Dir.Format(L"%s%s",m_szDrive, m_szDir);
	}

	//ist hübscher
	ch = static_cast<TCHAR>(towupper(m_Dir.GetAt(0)));
	m_Dir.SetAt(0,ch);

	CString m_Name,m_File,m_Dir1 = m_Dir;
	if (!CStringUtils::EndsWith(m_Dir, L'\\'))
		m_Dir += L'\\';

	if(m_LastDirectory.CompareNoCase(m_Dir) == 0 && m_Liste.m_SearchList.GetSize())
		return;

	m_LastDirectory = m_Dir;
	m_Dir += L"*.*";

	BOOL bContinue = FoundFiles.FindFile(m_Dir);
	if(bContinue)
		RemoveSearchAll();

	while (bContinue == TRUE)
	{
		bContinue = FoundFiles.FindNextFile();
		m_File = FoundFiles.GetFileName();

		if(FoundFiles.IsHidden() || FoundFiles.IsSystem())
			continue;
		if(FoundFiles.IsDirectory())
		{
			if(m_iMode & _MODE_ONLY_FILES)
				continue;
			if(FoundFiles.IsDots())
				continue;

			if (!CStringUtils::EndsWith(m_File, L'\\'))
				m_File += L'\\';
		}

		if(!FoundFiles.IsDirectory())
			if(m_iMode & _MODE_ONLY_DIRS)
				continue;

		if(m_iMode & _MODE_FS_START_DIR_)
		{
			m_Name = m_File;
		}
		else
		{
			m_Name = m_Dir1;
			if (!CStringUtils::EndsWith(m_Name, L'\\'))
				m_Name += L'\\';

			m_Name += m_File;
		}

		AddSearchString(m_Name);
	}
	FoundFiles.Close();
	return;
}

/*********************************************************************/

void CACEdit::SetStartDirectory(LPCTSTR lpszString)
{
	if(m_iType == -1) {ASSERT(0); return;}

	if(m_iMode & _MODE_FS_START_DIR_)
		ReadDirectory(lpszString);
}

/*********************************************************************
** CComboBox
** NEW:V1.1
*********************************************************************/

int CACEdit::AddString( LPCTSTR lpszString )
{
	if(m_iType == _COMBOBOX_)
	{
		return reinterpret_cast<CComboBox*>(this)->AddString(lpszString);
	}
	return CB_ERR;
}

/*********************************************************************/

int CACEdit::SetDroppedWidth( UINT nWidth )
{
	if(m_iType == _COMBOBOX_)
	{
		return reinterpret_cast<CComboBox*>(this)->SetDroppedWidth(nWidth);
	}
	return CB_ERR;
}

/*********************************************************************/

int CACEdit::FindString( int nStartAfter, LPCTSTR lpszString )
{
	if(m_iType == _COMBOBOX_)
	{
		return reinterpret_cast<CComboBox*>(this)->FindString(nStartAfter, lpszString);
	}
	return CB_ERR;
}

/*********************************************************************/

int CACEdit::SelectString( int nStartAfter, LPCTSTR lpszString )
{
	if(m_iType == _COMBOBOX_)
	{
		return reinterpret_cast<CComboBox*>(this)->SelectString(nStartAfter, lpszString);
	}
	return CB_ERR;
}

/*********************************************************************/

void CACEdit::ShowDropDown(BOOL bShowIt)
{
	if(m_iType == _COMBOBOX_)
	{
		reinterpret_cast<CComboBox*>(this)->ShowDropDown(bShowIt);
	}
}

/*********************************************************************/

void CACEdit::ResetContent()
{
	if(m_iType == _COMBOBOX_)
	{
		reinterpret_cast<CComboBox*>(this)->ResetContent();
	}
}

/*********************************************************************/

int CACEdit::GetCurSel()
{
	if(m_iType == _COMBOBOX_)
	{
		reinterpret_cast<CComboBox*>(this)->GetCurSel();
	}
	return CB_ERR;
}

/*********************************************************************/

int CACEdit::GetLBText( int nIndex, LPTSTR lpszText )
{
	if(m_iType == _COMBOBOX_)
	{
		reinterpret_cast<CComboBox*>(this)->GetLBText(nIndex, lpszText);
	}
	return CB_ERR;
}

/*********************************************************************/

void CACEdit::GetLBText( int nIndex, CString& rString )
{
	if(m_iType == _COMBOBOX_)
	{
		reinterpret_cast<CComboBox*>(this)->GetLBText(nIndex, rString);
	}
}

/*********************************************************************/
