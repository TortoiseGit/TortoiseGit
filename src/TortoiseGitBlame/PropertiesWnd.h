
#pragma once

#include "GitRev.h"

class CPropertiesToolBar : public CMFCToolBar
{
public:
	virtual void OnUpdateCmdUI(CFrameWnd* /*pTarget*/, BOOL bDisableIfNoHndler)
	{
		CMFCToolBar::OnUpdateCmdUI((CFrameWnd*) GetOwner(), bDisableIfNoHndler);
	}

	virtual BOOL AllowShowOnList() const { return FALSE; }
};

class CPropertiesWnd : public CDockablePane
{
// Construction
public:
	CPropertiesWnd();

	void AdjustLayout();

// Attributes
public:
	void SetVSDotNetLook(BOOL bSet)
	{
		m_wndPropList.SetVSDotNetLook(bSet);
		m_wndPropList.SetGroupNameFullWidth(bSet);
	}

	// rev=NULL, means clear properties info;
	void UpdateProperties(GitRev *rev=NULL);

protected:
	CFont m_fntPropList;
	CComboBox m_wndObjectCombo;
	CPropertiesToolBar m_wndToolBar;
	CMFCPropertyGridCtrl m_wndPropList;

	void RemoveParent();
// Implementation
public:
	virtual ~CPropertiesWnd();

protected:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnExpandAllProperties();
	afx_msg void OnUpdateExpandAllProperties(CCmdUI* pCmdUI);
	afx_msg void OnSortProperties();
	afx_msg void OnUpdateSortProperties(CCmdUI* pCmdUI);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnSettingChange(UINT uFlags, LPCTSTR lpszSection);

	DECLARE_MESSAGE_MAP()

	void InitPropList();
	void SetPropListFont();

	CMFCPropertyGridProperty* m_CommitHash;
	CMFCPropertyGridProperty* m_AuthorName;
	CMFCPropertyGridProperty* m_AuthorDate;
	CMFCPropertyGridProperty* m_AuthorEmail;

	CMFCPropertyGridProperty* m_CommitterName;
	CMFCPropertyGridProperty* m_CommitterEmail;
	CMFCPropertyGridProperty* m_CommitterDate;

	CMFCPropertyGridProperty* m_Subject;
	CMFCPropertyGridProperty* m_Body;

	CMFCPropertyGridProperty* m_ParentGroup;
	CMFCPropertyGridProperty* m_BaseInfoGroup;

	std::vector<CMFCPropertyGridProperty*> m_ParentHash;
	std::vector<CMFCPropertyGridProperty*> m_ParentSubject;
};
