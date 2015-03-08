// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2013 - TortoiseGit
// Copyright (C) 2011-2013 Sven Strickroth <email@cs-ware.de>

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

#include "PropertiesWnd.h"
#include "Resource.h"
#include "MainFrm.h"
#include "TortoiseGitBlame.h"
#include "IconMenu.h"
#include "StringUtils.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

/////////////////////////////////////////////////////////////////////////////
// CResourceViewBar

CPropertiesWnd::CPropertiesWnd()
	: m_CommitHash(nullptr)
	, m_AuthorName(nullptr)
	, m_AuthorEmail(nullptr)
	, m_AuthorDate(nullptr)
	, m_CommitterName(nullptr)
	, m_CommitterEmail(nullptr)
	, m_CommitterDate(nullptr)
	, m_Subject(nullptr)
	, m_Body(nullptr)
	, m_ParentGroup(nullptr)
	, m_BaseInfoGroup(nullptr)
{
}

CPropertiesWnd::~CPropertiesWnd()
{
}

BEGIN_MESSAGE_MAP(CPropertiesWnd, CDockablePane)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_SETFOCUS()
	ON_WM_SETTINGCHANGE()
	ON_WM_CONTEXTMENU()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CResourceViewBar message handlers

void CPropertiesWnd::AdjustLayout()
{
	if (GetSafeHwnd() == NULL)
	{
		return;
	}

	CRect rectClient,rectCombo;
	GetClientRect(rectClient);

	m_wndPropList.SetWindowPos(NULL, rectClient.left, rectClient.top, rectClient.Width(), rectClient.Height(), SWP_NOACTIVATE | SWP_NOZORDER);
}

int CPropertiesWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CDockablePane::OnCreate(lpCreateStruct) == -1)
		return -1;

	CRect rectDummy;
	rectDummy.SetRectEmpty();

	if (!m_wndPropList.Create(WS_VISIBLE | WS_CHILD, rectDummy, this, 2))
	{
		TRACE0("Failed to create Properties Grid \n");
		return -1;      // fail to create
	}

	InitPropList();

	AdjustLayout();
	return 0;
}

void CPropertiesWnd::OnSize(UINT nType, int cx, int cy)
{
	CDockablePane::OnSize(nType, cx, cy);
	AdjustLayout();
}

void CPropertiesWnd::InitPropList()
{
	SetPropListFont();

	m_wndPropList.EnableHeaderCtrl(FALSE);
	m_wndPropList.EnableDescriptionArea();
	m_wndPropList.SetVSDotNetLook();
	m_wndPropList.MarkModifiedProperties();

	CMFCPropertyGridProperty* pGroup1 = new CMFCPropertyGridProperty(CString(MAKEINTRESOURCE(IDS_PROPERTIES_BASICINFO)));


	m_CommitHash = new CMFCPropertyGridProperty(
				CString(MAKEINTRESOURCE(IDS_LOG_HASH)),
				_T(""),
				CString(MAKEINTRESOURCE(IDS_LOG_HASH))
				);
	pGroup1->AddSubItem(m_CommitHash);

	m_AuthorName = new CMFCPropertyGridProperty(
				CString(MAKEINTRESOURCE(IDS_LOG_AUTHOR)),
				_T(""),
				CString(MAKEINTRESOURCE(IDS_LOG_AUTHOR))
				);
	pGroup1->AddSubItem(m_AuthorName);

	m_AuthorDate = new CMFCPropertyGridProperty(
				CString(MAKEINTRESOURCE(IDS_LOG_DATE)),
				_T(""),
				CString(MAKEINTRESOURCE(IDS_LOG_DATE))
				);
	pGroup1->AddSubItem(m_AuthorDate);

	m_AuthorEmail= new CMFCPropertyGridProperty(
				CString(MAKEINTRESOURCE(IDS_LOG_EMAIL)),
				_T(""),
				CString(MAKEINTRESOURCE(IDS_LOG_EMAIL))
				);
	pGroup1->AddSubItem(m_AuthorEmail);

	m_CommitterName = new CMFCPropertyGridProperty(
				CString(MAKEINTRESOURCE(IDS_LOG_COMMIT_NAME)),
				_T(""),
				CString(MAKEINTRESOURCE(IDS_LOG_COMMIT_NAME))
				);
	pGroup1->AddSubItem(m_CommitterName);

	m_CommitterEmail =new CMFCPropertyGridProperty(
				CString(MAKEINTRESOURCE(IDS_LOG_COMMIT_EMAIL)),
				_T(""),
				CString(MAKEINTRESOURCE(IDS_LOG_COMMIT_EMAIL))
				);
	pGroup1->AddSubItem(m_CommitterEmail);

	m_CommitterDate = new CMFCPropertyGridProperty(
				CString(MAKEINTRESOURCE(IDS_LOG_COMMIT_DATE)),
				_T(""),
				CString(MAKEINTRESOURCE(IDS_LOG_COMMIT_DATE))
				);;
	pGroup1->AddSubItem(m_CommitterDate);

	m_Subject = new CMFCPropertyGridProperty(
				CString(MAKEINTRESOURCE(IDS_SUBJECT)),
				_T(""),
				CString(MAKEINTRESOURCE(IDS_SUBJECT))
				);;;
	pGroup1->AddSubItem(m_Subject);

	m_Body = new CMFCPropertyGridProperty(
				CString(MAKEINTRESOURCE(IDS_BODY)),
				_T(""),
				CString(MAKEINTRESOURCE(IDS_BODY))
				);;;;
	pGroup1->AddSubItem(m_Body);

	for (int i = 0; i < pGroup1->GetSubItemsCount(); ++i)
	{
		pGroup1->GetSubItem(i)->AllowEdit(FALSE);
	}

	m_wndPropList.AddProperty(pGroup1);
	m_BaseInfoGroup=pGroup1;

	m_ParentGroup=new CMFCPropertyGridProperty(CString(MAKEINTRESOURCE(IDS_PARENTS)));

	m_wndPropList.AddProperty(m_ParentGroup);
}

void CPropertiesWnd::OnSetFocus(CWnd* pOldWnd)
{
	CDockablePane::OnSetFocus(pOldWnd);
	m_wndPropList.SetFocus();
}

void CPropertiesWnd::OnSettingChange(UINT uFlags, LPCTSTR lpszSection)
{
	CDockablePane::OnSettingChange(uFlags, lpszSection);
	SetPropListFont();
}

void CPropertiesWnd::SetPropListFont()
{
	::DeleteObject(m_fntPropList.Detach());

	LOGFONT lf;
	afxGlobalData.fontRegular.GetLogFont(&lf);

	NONCLIENTMETRICS info;
	info.cbSize = sizeof(info);

	afxGlobalData.GetNonClientMetrics(info);

	lf.lfHeight = info.lfMenuFont.lfHeight;
	lf.lfWeight = info.lfMenuFont.lfWeight;
	lf.lfItalic = info.lfMenuFont.lfItalic;

	m_fntPropList.CreateFontIndirect(&lf);

	m_wndPropList.SetFont(&m_fntPropList);
}

void CPropertiesWnd::RemoveParent()
{
	m_ParentGroup->Expand(FALSE);
	while (m_ParentGroup->GetSubItemsCount())
	{
		CMFCPropertyGridProperty * p=m_ParentGroup->GetSubItem(0);
		m_ParentGroup->RemoveSubItem(p);
	}
}

void CPropertiesWnd::UpdateProperties(GitRev *pRev)
{
	if (pRev)
	{
		if (pRev->m_ParentHash.empty())
		{
			try
			{
				pRev->GetParentFromHash(pRev->m_CommitHash);//TODO
			}
			catch (const char* msg)
			{
				MessageBox(_T("Could not get parent.\nlibgit reports:\n") + CString(msg), _T("TortoiseGit"), MB_ICONERROR);
			}
		}
		CString hash = pRev->m_CommitHash.ToString();
		m_CommitHash->SetValue(hash);
		m_AuthorName->SetValue(pRev->GetAuthorName());
		CString authorDate = pRev->GetAuthorDate().Format(_T("%Y-%m-%d %H:%M"));
		m_AuthorDate->SetValue(authorDate);
		m_AuthorEmail->SetValue(pRev->GetAuthorEmail());

		m_CommitterName->SetValue(pRev->GetAuthorName());
		m_CommitterEmail->SetValue(pRev->GetCommitterEmail());
		CString committerDate = pRev->GetCommitterDate().Format(_T("%Y-%m-%d %H:%M"));
		m_CommitterDate->SetValue(committerDate);

		m_Subject->SetValue(pRev->GetSubject());
		m_Body->SetValue(pRev->GetBody().Trim());

		RemoveParent();

		CLogDataVector *pLogEntry = &((CMainFrame*)AfxGetApp()->GetMainWnd())->m_wndOutput.m_LogList.m_logEntries;

		CGitHashMap & hashMap = pLogEntry->m_pLogCache->m_HashMap;
		for (size_t i = 0; i < pRev->m_ParentHash.size(); ++i)
		{
			CString str;
			CString parentsubject;

			auto it = hashMap.find(pRev->m_ParentHash[i]);
			if (it != hashMap.end())
				parentsubject = it->second.GetSubject();

			str.Format(_T("%u - %s\n%s"), i, pRev->m_ParentHash[i].ToString(), parentsubject);

			CMFCPropertyGridProperty *pProperty = new CMFCPropertyGridProperty(pRev->m_ParentHash[i].ToString().Left(8), parentsubject, str);
			pProperty->AllowEdit(FALSE);
			m_ParentGroup->AddSubItem(pProperty);
		}
		m_ParentGroup->Expand();
		for (int i = 0; i < m_BaseInfoGroup->GetSubItemsCount(); ++i)
			m_BaseInfoGroup->GetSubItem(i)->SetDescription(m_BaseInfoGroup->GetSubItem(i)->GetValue());
	}
	else
	{
		m_CommitHash->SetValue(_T(""));
		m_AuthorName->SetValue(_T(""));
		m_AuthorDate->SetValue(_T(""));
		m_AuthorEmail->SetValue(_T(""));

		m_CommitterName->SetValue(_T(""));
		m_CommitterEmail->SetValue(_T(""));
		m_CommitterDate->SetValue(_T(""));

		m_Subject->SetValue(_T(""));
		m_Body->SetValue(_T(""));

		RemoveParent();

		for (int i = 0; i < m_BaseInfoGroup->GetSubItemsCount(); ++i)
			m_BaseInfoGroup->GetSubItem(i)->SetDescription(_T(""));
	}
	m_wndPropList.AdjustLayout();
}

void CPropertiesWnd::OnContextMenu(CWnd* /*pWnd*/, CPoint point)
{
	CMFCPropertyGridProperty * pProperty = m_wndPropList.GetCurSel();

	CString sMenuItemText;
	CIconMenu popup;
	if (pProperty && !pProperty->IsGroup() && popup.CreatePopupMenu())
	{
		sMenuItemText.LoadString(IDS_SCIEDIT_COPY);
		popup.AppendMenu(MF_STRING | MF_ENABLED, WM_COPY, sMenuItemText);

		int cmd = popup.TrackPopupMenu(TPM_RETURNCMD | TPM_LEFTALIGN | TPM_NONOTIFY, point.x, point.y, this, 0);
		switch (cmd)
		{
		case 0:
			break;	// no command selected
		case WM_COPY:
			CStringUtils::WriteAsciiStringToClipboard(pProperty->GetValue(), GetSafeHwnd());
			break;
		}
	}
}
