// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2014 - TortoiseGit

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
#pragma once
#include "StandAloneDlg.h"
#include "TortoiseProc.h"

class CCheckCertificateDlg : public CStandAloneDialog
{
	DECLARE_DYNAMIC(CCheckCertificateDlg)
public:
	CCheckCertificateDlg(CWnd* pParent = NULL);   // standard constructor

	// Dialog Data
	enum { IDD = IDD_CERTCHECK };

	CString m_sHostname;
	CString m_sError;

	CString m_sCertificateCN;
	CString m_sCertificateIssuer;

	git_cert_x509* cert;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedOpencert();

	DECLARE_MESSAGE_MAP()

	CString m_sSHA1;
	CString m_sSHA256;
};
