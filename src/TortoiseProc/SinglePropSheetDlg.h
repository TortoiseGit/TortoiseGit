#pragma once

#include "Settings/SettingsPropPage.h"
#include "TreePropSheet/TreePropSheet.h"

class CSinglePropSheetDlg : public TreePropSheet::CTreePropSheet
{
	DECLARE_DYNAMIC(CSinglePropSheetDlg)

public:
	CSinglePropSheetDlg(const TCHAR* szCaption, ISettingsPropPage* pThePropPage, CWnd* pParent = NULL);   // standard constructor
	virtual ~CSinglePropSheetDlg();

	void AddPropPages();
	void RemovePropPages();

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()

private:
	ISettingsPropPage*	m_pThePropPage;

public:
	virtual BOOL OnInitDialog();
};
