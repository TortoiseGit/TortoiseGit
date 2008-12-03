// TortoiseSVN - a Windows shell extension for easy version control

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
#include "SVNProperties.h"
#include "MessageBox.h"
#include "TortoiseProc.h"
#include "EditPropertiesDlg.h"
#include "EditPropertyValueDlg.h"
#include "UnicodeUtils.h"
#include "PathUtils.h"
#include "AppUtils.h"
#include "StringUtils.h"
#include "ProgressDlg.h"
#include "InputLogDlg.h"
#include "XPTheme.h"


IMPLEMENT_DYNAMIC(CEditPropertiesDlg, CResizableStandAloneDialog)

CEditPropertiesDlg::CEditPropertiesDlg(CWnd* pParent /*=NULL*/)
	: CResizableStandAloneDialog(CEditPropertiesDlg::IDD, pParent)
	, m_bRecursive(FALSE)
	, m_bChanged(false)
	, m_revision(SVNRev::REV_WC)
	, m_bRevProps(false)
{
}

CEditPropertiesDlg::~CEditPropertiesDlg()
{
}

void CEditPropertiesDlg::DoDataExchange(CDataExchange* pDX)
{
	CResizableStandAloneDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDITPROPLIST, m_propList);
}


BEGIN_MESSAGE_MAP(CEditPropertiesDlg, CResizableStandAloneDialog)
	ON_BN_CLICKED(IDHELP, &CEditPropertiesDlg::OnBnClickedHelp)
	ON_NOTIFY(NM_CUSTOMDRAW, IDC_EDITPROPLIST, &CEditPropertiesDlg::OnNMCustomdrawEditproplist)
	ON_BN_CLICKED(IDC_REMOVEPROPS, &CEditPropertiesDlg::OnBnClickedRemoveProps)
	ON_BN_CLICKED(IDC_EDITPROPS, &CEditPropertiesDlg::OnBnClickedEditprops)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_EDITPROPLIST, &CEditPropertiesDlg::OnLvnItemchangedEditproplist)
	ON_NOTIFY(NM_DBLCLK, IDC_EDITPROPLIST, &CEditPropertiesDlg::OnNMDblclkEditproplist)
	ON_BN_CLICKED(IDC_SAVEPROP, &CEditPropertiesDlg::OnBnClickedSaveprop)
	ON_BN_CLICKED(IDC_ADDPROPS, &CEditPropertiesDlg::OnBnClickedAddprops)
	ON_BN_CLICKED(IDC_EXPORT, &CEditPropertiesDlg::OnBnClickedExport)
	ON_BN_CLICKED(IDC_IMPORT, &CEditPropertiesDlg::OnBnClickedImport)
END_MESSAGE_MAP()

void CEditPropertiesDlg::OnBnClickedHelp()
{
	OnHelp();
}

BOOL CEditPropertiesDlg::OnInitDialog()
{
	CResizableStandAloneDialog::OnInitDialog();

	// fill in the path edit control
	if (m_pathlist.GetCount() == 1)
	{
		if (m_pathlist[0].IsUrl())
			SetDlgItemText(IDC_PROPPATH, m_pathlist[0].GetSVNPathString());
		else
			SetDlgItemText(IDC_PROPPATH, m_pathlist[0].GetWinPathString());
	}
	else
	{
		CString sTemp;
		sTemp.Format(IDS_EDITPROPS_NUMPATHS, m_pathlist.GetCount());
		SetDlgItemText(IDC_PROPPATH, sTemp);
	}

	CXPTheme theme;
	theme.SetWindowTheme(m_propList.GetSafeHwnd(), L"Explorer", NULL);

	// initialize the property list control
	m_propList.DeleteAllItems();
	DWORD exStyle = LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER;
	m_propList.SetExtendedStyle(exStyle);
	int c = ((CHeaderCtrl*)(m_propList.GetDlgItem(0)))->GetItemCount()-1;
	while (c>=0)
		m_propList.DeleteColumn(c--);
	CString temp;
	temp.LoadString(IDS_PROPPROPERTY);
	m_propList.InsertColumn(0, temp);
	temp.LoadString(IDS_PROPVALUE);
	m_propList.InsertColumn(1, temp);
	m_propList.SetRedraw(false);

	if (m_bRevProps)
	{
		GetDlgItem(IDC_IMPORT)->ShowWindow(SW_HIDE);
		GetDlgItem(IDC_EXPORT)->ShowWindow(SW_HIDE);
	}

	m_tooltips.Create(this);
	m_tooltips.AddTool(IDC_IMPORT, IDS_PROP_TT_IMPORT);
	m_tooltips.AddTool(IDC_EXPORT,  IDS_PROP_TT_EXPORT);
	m_tooltips.AddTool(IDC_SAVEPROP,  IDS_PROP_TT_SAVE);
	m_tooltips.AddTool(IDC_REMOVEPROPS,  IDS_PROP_TT_REMOVE);
	m_tooltips.AddTool(IDC_EDITPROPS,  IDS_PROP_TT_EDIT);
	m_tooltips.AddTool(IDC_ADDPROPS,  IDS_PROP_TT_ADD);

	AddAnchor(IDC_GROUP, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_PROPPATH, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_EDITPROPLIST, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_IMPORT, BOTTOM_RIGHT);
	AddAnchor(IDC_EXPORT, BOTTOM_RIGHT);
	AddAnchor(IDC_SAVEPROP, BOTTOM_RIGHT);
	AddAnchor(IDC_REMOVEPROPS, BOTTOM_RIGHT);
	AddAnchor(IDC_EDITPROPS, BOTTOM_RIGHT);
	AddAnchor(IDC_ADDPROPS, BOTTOM_RIGHT);
	AddAnchor(IDOK, BOTTOM_RIGHT);
	AddAnchor(IDHELP, BOTTOM_RIGHT);
	if (hWndExplorer)
		CenterWindow(CWnd::FromHandle(hWndExplorer));
	EnableSaveRestore(_T("EditPropertiesDlg"));

	InterlockedExchange(&m_bThreadRunning, TRUE);
	if (AfxBeginThread(PropsThreadEntry, this)==NULL)
	{
		InterlockedExchange(&m_bThreadRunning, FALSE);
		CMessageBox::Show(this->m_hWnd, IDS_ERR_THREADSTARTFAILED, IDS_APPNAME, MB_OK | MB_ICONERROR);
	}
	GetDlgItem(IDC_EDITPROPLIST)->SetFocus();

	return FALSE;
}

void CEditPropertiesDlg::Refresh()
{
	if (m_bThreadRunning)
		return;
	m_propList.DeleteAllItems();
	InterlockedExchange(&m_bThreadRunning, TRUE);
	if (AfxBeginThread(PropsThreadEntry, this)==NULL)
	{
		InterlockedExchange(&m_bThreadRunning, FALSE);
		CMessageBox::Show(this->m_hWnd, IDS_ERR_THREADSTARTFAILED, IDS_APPNAME, MB_OK | MB_ICONERROR);
	}
}

UINT CEditPropertiesDlg::PropsThreadEntry(LPVOID pVoid)
{
	return ((CEditPropertiesDlg*)pVoid)->PropsThread();
}

UINT CEditPropertiesDlg::PropsThread()
{
	// get all properties
	m_properties.clear();
	for (int i=0; i<m_pathlist.GetCount(); ++i)
	{
		SVNProperties props(m_pathlist[i], m_revision, m_bRevProps);
		for (int p=0; p<props.GetCount(); ++p)
		{
			wide_string prop_str = props.GetItemName(p);
			std::string prop_value = props.GetItemValue(p);
			std::map<stdstring,PropValue>::iterator it = m_properties.lower_bound(prop_str);
			if (it != m_properties.end() && it->first == prop_str)
			{
				it->second.count++;
				if (it->second.value.compare(prop_value)!=0)
					it->second.allthesamevalue = false;
			}
			else
			{
				it = m_properties.insert(it, std::make_pair(prop_str, PropValue()));
				stdstring value = MultibyteToWide((char *)prop_value.c_str());
				it->second.value = prop_value;
				CString stemp = value.c_str();
				stemp.Replace('\n', ' ');
				stemp.Replace(_T("\r"), _T(""));
				it->second.value_without_newlines = stdstring((LPCTSTR)stemp);
				it->second.count = 1;
				it->second.allthesamevalue = true;
				if (SVNProperties::IsBinary(prop_value))
					it->second.isbinary = true;
			}
		}
	}
	
	// fill the property list control with the gathered information
	int index=0;
	m_propList.SetRedraw(FALSE);
	for (std::map<stdstring,PropValue>::iterator it = m_properties.begin(); it != m_properties.end(); ++it)
	{
		m_propList.InsertItem(index, it->first.c_str());
		if (it->second.isbinary)
		{
			m_propList.SetItemText(index, 1, CString(MAKEINTRESOURCE(IDS_EDITPROPS_BINVALUE)));
			m_propList.SetItemData(index, FALSE);
		}
		else if (it->second.count != m_pathlist.GetCount())
		{
			// if the property values are the same for all paths they're set
			// but they're not set for *all* paths, then we show the entry grayed out
			m_propList.SetItemText(index, 1, it->second.value_without_newlines.c_str());
			m_propList.SetItemData(index, FALSE);
		}
		else if (it->second.allthesamevalue)
		{
			// if the property values are the same for all paths,
			// we show the value
			m_propList.SetItemText(index, 1, it->second.value_without_newlines.c_str());
			m_propList.SetItemData(index, TRUE);
		}
		else
		{
			// if the property values aren't the same for all paths
			// then we show "values are different" instead of the value
			CString sTemp(MAKEINTRESOURCE(IDS_EDITPROPS_DIFFERENTPROPVALUES));
			m_propList.SetItemText(index, 1, sTemp);
			m_propList.SetItemData(index, FALSE);
		}
		if (index == 0)
		{
			// select the first entry
			m_propList.SetItemState(index, LVIS_SELECTED, LVIS_SELECTED);
			m_propList.SetSelectionMark(index);
		}
		index++;
	}
	int maxcol = ((CHeaderCtrl*)(m_propList.GetDlgItem(0)))->GetItemCount()-1;
	for (int col = 0; col <= maxcol; col++)
	{
		m_propList.SetColumnWidth(col, LVSCW_AUTOSIZE_USEHEADER);
	}

	InterlockedExchange(&m_bThreadRunning, FALSE);
	m_propList.SetRedraw(TRUE);
	return 0;
}

void CEditPropertiesDlg::OnNMCustomdrawEditproplist(NMHDR *pNMHDR, LRESULT *pResult)
{
	NMLVCUSTOMDRAW* pLVCD = reinterpret_cast<NMLVCUSTOMDRAW*>( pNMHDR );
	// Take the default processing unless we set this to something else below.
	*pResult = CDRF_DODEFAULT;

	if (m_bThreadRunning)
		return;

	// First thing - check the draw stage. If it's the control's prepaint
	// stage, then tell Windows we want messages for every item.

	if ( CDDS_PREPAINT == pLVCD->nmcd.dwDrawStage )
	{
		*pResult = CDRF_NOTIFYITEMDRAW;
	}
	else if ( CDDS_ITEMPREPAINT == pLVCD->nmcd.dwDrawStage )
	{
		// This is the prepaint stage for an item. Here's where we set the
		// item's text color. Our return value will tell Windows to draw the
		// item itself, but it will use the new color we set here.

		// Tell Windows to paint the control itself.
		*pResult = CDRF_DODEFAULT;

		if ((int)pLVCD->nmcd.dwItemSpec > m_propList.GetItemCount())
			return;

		COLORREF crText = GetSysColor(COLOR_WINDOWTEXT);
		if (m_propList.GetItemData (static_cast<int>(pLVCD->nmcd.dwItemSpec))==FALSE)
			crText = GetSysColor(COLOR_GRAYTEXT);
		// Store the color back in the NMLVCUSTOMDRAW struct.
		pLVCD->clrText = crText;
	}

}

void CEditPropertiesDlg::OnLvnItemchangedEditproplist(NMHDR * /*pNMHDR*/, LRESULT *pResult)
{
	*pResult = 0;
	// disable the "remove" button if nothing
	// is selected, enable it otherwise
	int selCount = m_propList.GetSelectedCount();
	DialogEnableWindow(IDC_EXPORT, selCount >= 1);
	DialogEnableWindow(IDC_REMOVEPROPS, selCount >= 1);
	DialogEnableWindow(IDC_SAVEPROP, selCount == 1);
	DialogEnableWindow(IDC_EDITPROPS, selCount == 1);

	*pResult = 0;
}

void CEditPropertiesDlg::OnBnClickedRemoveProps()
{
	CString sLogMsg;
	POSITION pos = m_propList.GetFirstSelectedItemPosition();
	while ( pos )
	{
		int selIndex = m_propList.GetNextSelectedItem(pos);

		bool bRecurse = false;
		CString sName = m_propList.GetItemText(selIndex, 0);
		if (m_pathlist[0].IsUrl())
		{
			CInputLogDlg input(this);
			input.SetUUID(m_sUUID);
			input.SetProjectProperties(m_pProjectProperties);
			CString sHint;
			sHint.Format(IDS_INPUT_REMOVEPROP, (LPCTSTR)sName, (LPCTSTR)(m_pathlist[0].GetSVNPathString()));
			input.SetActionText(sHint);
			if (input.DoModal() != IDOK)
				return;
			sLogMsg = input.GetLogMessage();
		}
		CString sQuestion;
		sQuestion.Format(IDS_EDITPROPS_RECURSIVEREMOVEQUESTION, (LPCTSTR)sName);
		CString sRecursive(MAKEINTRESOURCE(IDS_EDITPROPS_RECURSIVE));
		CString sNotRecursive(MAKEINTRESOURCE(IDS_EDITPROPS_NOTRECURSIVE));
		CString sCancel(MAKEINTRESOURCE(IDS_EDITPROPS_CANCEL));

		if ((m_pathlist.GetCount()>1)||((m_pathlist.GetCount()==1)&&(PathIsDirectory(m_pathlist[0].GetWinPath()))))
		{
			int ret = CMessageBox::Show(m_hWnd, sQuestion, _T("TortoiseSVN"), MB_DEFBUTTON1, IDI_QUESTION, sRecursive, sNotRecursive, sCancel);
			if (ret == 1)
				bRecurse = true;
			else if (ret == 2)
				bRecurse = false;
			else
				break;
		}

		CProgressDlg prog;
		CString sTemp;
		sTemp.LoadString(IDS_SETPROPTITLE);
		prog.SetTitle(sTemp);
		sTemp.LoadString(IDS_PROPWAITCANCEL);
		prog.SetCancelMsg(sTemp);
		prog.SetTime(TRUE);
		prog.SetShowProgressBar(TRUE);
		prog.ShowModeless(m_hWnd);
		for (int i=0; i<m_pathlist.GetCount(); ++i)
		{
			prog.SetLine(1, m_pathlist[i].GetWinPath(), true);
			SVNProperties props(m_pathlist[i], m_revision, m_bRevProps);
			if (!props.Remove(sName, bRecurse ? svn_depth_infinity : svn_depth_empty, (LPCTSTR)sLogMsg))
			{
				CMessageBox::Show(m_hWnd, props.GetLastErrorMsg().c_str(), _T("TortoiseSVN"), MB_ICONERROR);
			}
			else
			{
				m_bChanged = true;
				if (m_revision.IsNumber())
					m_revision = LONG(m_revision)+1;
			}
		}
		prog.Stop();
	}
	DialogEnableWindow(IDC_REMOVEPROPS, FALSE);
	DialogEnableWindow(IDC_SAVEPROP, FALSE);
	Refresh();
}

void CEditPropertiesDlg::OnNMDblclkEditproplist(NMHDR * /*pNMHDR*/, LRESULT *pResult)
{
	if (m_propList.GetSelectedCount() == 1)
		EditProps();

	*pResult = 0;
}

void CEditPropertiesDlg::OnBnClickedEditprops()
{
	EditProps();
}

void CEditPropertiesDlg::OnBnClickedAddprops()
{
	EditProps(true);
}

void CEditPropertiesDlg::EditProps(bool bAdd /* = false*/)
{
	m_tooltips.Pop();	// hide the tooltips
	int selIndex = m_propList.GetSelectionMark();

	CEditPropertyValueDlg dlg;
	CString sName;

	if ((!bAdd)&&(selIndex >= 0)&&(m_propList.GetSelectedCount()))
	{
		sName = m_propList.GetItemText(selIndex, 0);
		PropValue& prop = m_properties[stdstring(sName)];
		dlg.SetPropertyName(sName);
		if (prop.allthesamevalue)
			dlg.SetPropertyValue(prop.value);
		dlg.SetDialogTitle(CString(MAKEINTRESOURCE(IDS_EDITPROPS_EDITTITLE)));
	}
	else
	{
		dlg.SetPathList(m_pathlist);  // this is the problem
		dlg.SetDialogTitle(CString(MAKEINTRESOURCE(IDS_EDITPROPS_ADDTITLE)));
	}

	if (m_pathlist.GetCount() > 1)
		dlg.SetMultiple();
	else if (m_pathlist.GetCount() == 1)
	{
		if (PathIsDirectory(m_pathlist[0].GetWinPath()))
			dlg.SetFolder();
	}

	dlg.RevProps(m_bRevProps);
	if ( dlg.DoModal()==IDOK )
	{
		sName = dlg.GetPropertyName();
		if (!sName.IsEmpty())
		{
			CString sMsg;
			bool bDoIt = true;
			if (!m_bRevProps&&(m_pathlist.GetCount())&&(m_pathlist[0].IsUrl()))
			{
				CInputLogDlg input(this);
				input.SetUUID(m_sUUID);
				input.SetProjectProperties(m_pProjectProperties);
				CString sHint;
				sHint.Format(IDS_INPUT_EDITPROP, (LPCTSTR)sName, (LPCTSTR)(m_pathlist[0].GetSVNPathString()));
				input.SetActionText(sHint);
				if (input.DoModal() == IDOK)
				{
					sMsg = input.GetLogMessage();
				}
				else
					bDoIt = false;
			}
			if (bDoIt)
			{
				CProgressDlg prog;
				CString sTemp;
				sTemp.LoadString(IDS_SETPROPTITLE);
				prog.SetTitle(sTemp);
				sTemp.LoadString(IDS_PROPWAITCANCEL);
				prog.SetCancelMsg(sTemp);
				prog.SetTime(TRUE);
				prog.SetShowProgressBar(TRUE);
				prog.ShowModeless(m_hWnd);
				for (int i=0; i<m_pathlist.GetCount(); ++i)
				{
					prog.SetLine(1, m_pathlist[i].GetWinPath(), true);
					SVNProperties props(m_pathlist[i], m_revision, m_bRevProps);
					if (!props.Add(sName, dlg.IsBinary() ? dlg.GetPropertyValue() : dlg.GetPropertyValue().c_str(), 
						dlg.GetRecursive() ? svn_depth_infinity : svn_depth_empty, sMsg))
					{
						CMessageBox::Show(m_hWnd, props.GetLastErrorMsg().c_str(), _T("TortoiseSVN"), MB_ICONERROR);
					}
					else
					{
						m_bChanged = true;
						// bump the revision number since we just did a commit
						if (!m_bRevProps && m_revision.IsNumber())
							m_revision = LONG(m_revision)+1;
					}
				}
				prog.Stop();
				Refresh();
			}
		}
	}
}

void CEditPropertiesDlg::OnOK()
{
	if (m_bThreadRunning)
		return;
	CStandAloneDialogTmpl<CResizableDialog>::OnOK();
}

void CEditPropertiesDlg::OnCancel()
{
	if (m_bThreadRunning)
		return;
	CStandAloneDialogTmpl<CResizableDialog>::OnCancel();
}

BOOL CEditPropertiesDlg::PreTranslateMessage(MSG* pMsg)
{
	m_tooltips.RelayEvent(pMsg);

	if (pMsg->message == WM_KEYDOWN)
	{
		switch (pMsg->wParam)
		{
		case VK_F5:
			{
				if (m_bThreadRunning)
					return __super::PreTranslateMessage(pMsg);
				Refresh();
			}
			break;
		case VK_RETURN:
			{
				if (GetAsyncKeyState(VK_CONTROL)&0x8000)
				{
					if ( GetDlgItem(IDOK)->IsWindowEnabled() )
					{
						PostMessage(WM_COMMAND, IDOK);
					}
				}
			}
			break;
		default:
			break;
		}
	}

	return __super::PreTranslateMessage(pMsg);
}

void CEditPropertiesDlg::OnBnClickedSaveprop()
{
	m_tooltips.Pop();	// hide the tooltips
	int selIndex = m_propList.GetSelectionMark();

	CString sName;
	CString sValue;
	if ((selIndex >= 0)&&(m_propList.GetSelectedCount()))
	{
		sName = m_propList.GetItemText(selIndex, 0);
		PropValue& prop = m_properties[stdstring(sName)];
		sValue = prop.value.c_str();
		if (prop.allthesamevalue)
		{
			CString savePath;
			if (!CAppUtils::FileOpenSave(savePath, NULL, IDS_REPOBROWSE_SAVEAS, 0, false, m_hWnd))
				return;

			FILE * stream;
			errno_t err = 0;
			if ((err = _tfopen_s(&stream, (LPCTSTR)savePath, _T("wbS")))==0)
			{
				fwrite(prop.value.c_str(), sizeof(char), prop.value.size(), stream);
				fclose(stream);
			}
			else
			{
				TCHAR strErr[4096] = {0};
				_tcserror_s(strErr, 4096, err);
				CMessageBox::Show(m_hWnd, strErr, _T("TortoiseSVN"), MB_ICONERROR);
			}
		}
	}
}

void CEditPropertiesDlg::OnBnClickedExport()
{
	m_tooltips.Pop();	// hide the tooltips
	if (m_propList.GetSelectedCount() == 0)
		return;

	CString savePath;
	if (!CAppUtils::FileOpenSave(savePath, NULL, IDS_REPOBROWSE_SAVEAS, IDS_PROPSFILEFILTER, false, m_hWnd))
		return;

	if (CPathUtils::GetFileExtFromPath(savePath).Compare(_T(".svnprops")))
	{
		// append the default ".svnprops" extension since the user did not specify it himself
		savePath += _T(".svnprops");
	}
	// we save the list of selected properties not in a text file but in our own
	// binary format. That's because properties can be binary themselves, a text
	// or xml file just won't do it properly.
	CString sName;
	CString sValue;
	FILE * stream;
	errno_t err = 0;

	if ((err = _tfopen_s(&stream, (LPCTSTR)savePath, _T("wbS")))==0)
	{
		POSITION pos = m_propList.GetFirstSelectedItemPosition();
		int len = m_propList.GetSelectedCount();
		fwrite(&len, sizeof(int), 1, stream);										// number of properties
		while (pos)
		{
			int index = m_propList.GetNextSelectedItem(pos);
			sName = m_propList.GetItemText(index, 0);
			PropValue& prop = m_properties[stdstring(sName)];
			sValue = prop.value.c_str();
			int len = sName.GetLength()*sizeof(TCHAR);
			fwrite(&len, sizeof(int), 1, stream);									// length of property name in bytes
			fwrite(sName, sizeof(TCHAR), sName.GetLength(), stream);				// property name
			len = static_cast<int>(prop.value.size());
			fwrite(&len, sizeof(int), 1, stream);									// length of property value in bytes
			fwrite(prop.value.c_str(), sizeof(char), prop.value.size(), stream);	// property value
		}
		fclose(stream);
	}
	else
	{
		TCHAR strErr[4096] = {0};
		_tcserror_s(strErr, 4096, err);
		CMessageBox::Show(m_hWnd, strErr, _T("TortoiseSVN"), MB_ICONERROR);
	}
}

void CEditPropertiesDlg::OnBnClickedImport()
{
	m_tooltips.Pop();	// hide the tooltips
	CString openPath;
	if (!CAppUtils::FileOpenSave(openPath, NULL, IDS_REPOBROWSE_OPEN, IDS_PROPSFILEFILTER, true, m_hWnd))
	{
		return;
	}
	// first check the size of the file
	FILE * stream;
	_tfopen_s(&stream, openPath, _T("rbS"));
	int nProperties = 0;
	if (fread(&nProperties, sizeof(int), 1, stream) == 1)
	{
		bool bFailed = false;
		if ((nProperties < 0)||(nProperties > 4096))
		{
			CMessageBox::Show(m_hWnd, IDS_EDITPROPS_ERRIMPORTFORMAT, IDS_APPNAME, MB_ICONERROR);
			bFailed = true;
		}
		CProgressDlg prog;
		CString sTemp;
		sTemp.LoadString(IDS_SETPROPTITLE);
		prog.SetTitle(sTemp);
		sTemp.LoadString(IDS_PROPWAITCANCEL);
		prog.SetCancelMsg(sTemp);
		prog.SetTime(TRUE);
		prog.SetShowProgressBar(TRUE);
		prog.ShowModeless(m_hWnd);
		while ((nProperties-- > 0)&&(!bFailed))
		{
			int nNameBytes = 0;
			if ((nNameBytes < 0)||(nNameBytes > 4096))
			{
				prog.Stop();
				CMessageBox::Show(m_hWnd, IDS_EDITPROPS_ERRIMPORTFORMAT, IDS_APPNAME, MB_ICONERROR);
				bFailed = true;
			}
			if (fread(&nNameBytes, sizeof(int), 1, stream) == 1)
			{
				TCHAR * pNameBuf = new TCHAR[nNameBytes/sizeof(TCHAR)];
				if (fread(pNameBuf, 1, nNameBytes, stream) == (size_t)nNameBytes)
				{
					CString sName = CString(pNameBuf, nNameBytes/sizeof(TCHAR));
					int nValueBytes = 0;
					if (fread(&nValueBytes, sizeof(int), 1, stream) == 1)
					{
						BYTE * pValueBuf = new BYTE[nValueBytes];
						if (fread(pValueBuf, sizeof(char), nValueBytes, stream) == (size_t)nValueBytes)
						{
							std::string propertyvalue;
							propertyvalue.assign((const char*)pValueBuf, nValueBytes);
							CString sMsg;
							if (m_pathlist[0].IsUrl())
							{
								CInputLogDlg input(this);
								input.SetUUID(m_sUUID);
								input.SetProjectProperties(m_pProjectProperties);
								CString sHint;
								sHint.Format(IDS_INPUT_SETPROP, (LPCTSTR)sName, (LPCTSTR)(m_pathlist[0].GetSVNPathString()));
								input.SetActionText(sHint);
								if (input.DoModal() == IDOK)
								{
									sMsg = input.GetLogMessage();
								}
								else
									bFailed = true;
							}

							for (int i=0; i<m_pathlist.GetCount() && !bFailed; ++i)
							{
								prog.SetLine(1, m_pathlist[i].GetWinPath(), true);
								SVNProperties props(m_pathlist[i], m_revision, m_bRevProps);
								if (!props.Add(sName, propertyvalue, svn_depth_empty, (LPCTSTR)sMsg))
								{
									prog.Stop();
									CMessageBox::Show(m_hWnd, props.GetLastErrorMsg().c_str(), _T("TortoiseSVN"), MB_ICONERROR);
									bFailed = true;
								}
								else
								{
									if (m_revision.IsNumber())
										m_revision = (LONG)m_revision + 1;
								}
							}
						}
						else
						{
							prog.Stop();
							CMessageBox::Show(m_hWnd, IDS_EDITPROPS_ERRIMPORTFORMAT, IDS_APPNAME, MB_ICONERROR);
							bFailed = true;
						}
						delete [] pValueBuf;
					}
					else
					{
						prog.Stop();
						CMessageBox::Show(m_hWnd, IDS_EDITPROPS_ERRIMPORTFORMAT, IDS_APPNAME, MB_ICONERROR);
						bFailed = true;
					}
				}
				else
				{
					prog.Stop();
					CMessageBox::Show(m_hWnd, IDS_EDITPROPS_ERRIMPORTFORMAT, IDS_APPNAME, MB_ICONERROR);
					bFailed = true;
				}
				delete [] pNameBuf;
			}
			else
			{
				prog.Stop();
				CMessageBox::Show(m_hWnd, IDS_EDITPROPS_ERRIMPORTFORMAT, IDS_APPNAME, MB_ICONERROR);
				bFailed = true;
			}
		}
		prog.Stop();
		fclose(stream);
	}

	Refresh();
}
