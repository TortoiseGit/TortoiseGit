#pragma once

#ifdef _WIN32_WCE
#error "CDHtmlDialog is not supported for Windows CE."
#endif 

// CCloneDlg dialog

class CCloneDlg : public CDHtmlDialog
{
	DECLARE_DYNCREATE(CCloneDlg)

public:
	CCloneDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CCloneDlg();
// Overrides
	HRESULT OnButtonOK(IHTMLElement *pElement);
	HRESULT OnButtonCancel(IHTMLElement *pElement);

// Dialog Data
	enum { IDD = IDD_CLONE, IDH = IDR_HTML_CLONEDLG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()
	DECLARE_DHTML_EVENT_MAP()
public:
	afx_msg void OnBnClickedCloneBrowseUrl();
	afx_msg void OnBnClickedCloneDirBrowse();
};
