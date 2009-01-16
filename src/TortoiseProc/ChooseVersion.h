#pragma once
#include "afxwin.h"

class CChooseVersion
{
public:
	
private:
	CWnd *	m_pWin;
protected:
	CHistoryCombo m_ChooseVersioinBranch;  
	CHistoryCombo m_ChooseVersioinTags;    
	CHistoryCombo m_ChooseVersioinVersion; 

	afx_msg void OnBnClickedChooseRadio() 
	{
		this->m_ChooseVersioinTags.EnableWindow(FALSE);													
		this->m_ChooseVersioinBranch.EnableWindow(FALSE);					
		this->m_ChooseVersioinVersion.EnableWindow(FALSE);				
		int radio=m_pWin->GetCheckedRadioButton(IDC_RADIO_HEAD,IDC_RADIO_VERSION);
		switch (radio)											
		{															
		case IDC_RADIO_HEAD:										
			break;													
		case IDC_RADIO_BRANCH:										
			this->m_ChooseVersioinBranch.EnableWindow(TRUE);						
			break;													
		case IDC_RADIO_TAGS:										
			this->m_ChooseVersioinTags.EnableWindow(TRUE);						
			break;													
		case IDC_RADIO_VERSION:										
			this->m_ChooseVersioinVersion.EnableWindow(TRUE);						
		break;		
		}
	}
	void UpdateRevsionName()
	{																			
		int radio=m_pWin->GetCheckedRadioButton(IDC_RADIO_HEAD,IDC_RADIO_VERSION);		
		switch (radio)															
		{																		
		case IDC_RADIO_HEAD:													
			this->m_VersionName=_T("HEAD");											
			break;																
		case IDC_RADIO_BRANCH:													
			this->m_VersionName=m_ChooseVersioinBranch.GetString();									
			break;																
		case IDC_RADIO_TAGS:													
			this->m_VersionName=m_ChooseVersioinTags.GetString();									
			break;																
		case IDC_RADIO_VERSION:													
			this->m_VersionName=m_ChooseVersioinVersion.GetString();									
			break;	
		}
	}			
	void SetDefaultChoose(int id)
	{
		m_pWin->CheckRadioButton(IDC_RADIO_HEAD,IDC_RADIO_VERSION,id);
		OnBnClickedChooseRadio();
	}

	void Init()
	{	
		m_ChooseVersioinBranch.SetMaxHistoryItems(0x7FFFFFFF);
		m_ChooseVersioinTags.SetMaxHistoryItems(0x7FFFFFFF);

		STRING_VECTOR list;
		g_Git.GetTagList(list);
		m_ChooseVersioinTags.AddString(list);

		list.clear();
		int current;
		g_Git.GetBranchList(list,&current,CGit::BRANCH_ALL);
		m_ChooseVersioinBranch.AddString(list);
		m_ChooseVersioinBranch.SetCurSel(current);


	}
public:					
	CString m_VersionName;
	CChooseVersion(CWnd *win)
	{
		m_pWin=win;
	};

};


#define CHOOSE_VERSION_DDX \
	DDX_Control(pDX, IDC_COMBOBOXEX_BRANCH,		m_ChooseVersioinBranch); \
	DDX_Control(pDX, IDC_COMBOBOXEX_TAGS,		m_ChooseVersioinTags);     \
	DDX_Control(pDX, IDC_COMBOBOXEX_VERSION,	m_ChooseVersioinVersion);

#define CHOOSE_VERSION_EVENT\
	ON_BN_CLICKED(IDC_RADIO_HEAD,		OnBnClickedChooseRadioHost)\
	ON_BN_CLICKED(IDC_RADIO_BRANCH,		OnBnClickedChooseRadioHost)\
	ON_BN_CLICKED(IDC_RADIO_TAGS,		OnBnClickedChooseRadioHost)\
	ON_BN_CLICKED(IDC_RADIO_VERSION,	OnBnClickedChooseRadioHost)

#define CHOOSE_VERSION_ADDANCHOR								\
	{															\
		AddAnchor(IDC_COMBOBOXEX_BRANCH, TOP_LEFT, TOP_RIGHT);	\
		AddAnchor(IDC_COMBOBOXEX_TAGS, TOP_LEFT, TOP_RIGHT);	\
		AddAnchor(IDC_COMBOBOXEX_VERSION, TOP_LEFT, TOP_RIGHT);	\
		AddAnchor(IDC_GROUP_BASEON, TOP_LEFT, TOP_RIGHT);		\
		AddAnchor(IDC_BUTTON_SHOW,TOP_RIGHT);		\
	}	

#define CHOOSE_EVENT_RADIO() \
	afx_msg void OnBnClickedChooseRadioHost(){OnBnClickedChooseRadio();}
