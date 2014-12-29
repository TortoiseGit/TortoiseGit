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

#include "stdafx.h"
#include "CheckCertificateDlg.h"
#include "AppUtils.h"
#include "TempFile.h"
#include <WinCrypt.h>

IMPLEMENT_DYNAMIC(CCheckCertificateDlg, CStandAloneDialog)
CCheckCertificateDlg::CCheckCertificateDlg(CWnd* pParent /*=NULL*/)
: CStandAloneDialog(CCheckCertificateDlg::IDD, pParent)
, cert(nullptr)
{
}

void CCheckCertificateDlg::DoDataExchange(CDataExchange* pDX)
{
	CStandAloneDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_ERROR, m_sError);
	DDX_Text(pDX, IDC_COMMONNAME, m_sCertificateCN);
	DDX_Text(pDX, IDC_ISSUER, m_sCertificateIssuer);
	DDX_Text(pDX, IDC_SHA1, m_sSHA1);
	DDX_Text(pDX, IDC_SHA256, m_sSHA256);
}

BEGIN_MESSAGE_MAP(CCheckCertificateDlg, CStandAloneDialog)
	ON_BN_CLICKED(IDOK, OnBnClickedOk)
	ON_BN_CLICKED(IDC_OPENCERT, &CCheckCertificateDlg::OnBnClickedOpencert)
END_MESSAGE_MAP()

void CCheckCertificateDlg::OnBnClickedOk()
{
	OnOK();
}

static CString getCertificateHash(HCRYPTPROV hCryptProv, ALG_ID algId, BYTE* certificate, size_t len)
{
	CString readable = _T("unknown");
	std::unique_ptr<BYTE[]> pHash(nullptr);
	HCRYPTHASH hHash = NULL;

	if (!hCryptProv)
		goto finish;

	if (!CryptCreateHash(hCryptProv, algId, 0, 0, &hHash))
		goto finish;

	if (!CryptHashData(hHash, certificate, (DWORD)len, 0))
		goto finish;

	DWORD hashLen;
	DWORD hashLenLen = sizeof(DWORD);
	if (!CryptGetHashParam(hHash, HP_HASHSIZE, (BYTE *)&hashLen, &hashLenLen, 0))
		goto finish;

	pHash.reset(new BYTE[hashLen]);
	if (!CryptGetHashParam(hHash, HP_HASHVAL, pHash.get(), &hashLen, 0))
		goto finish;

	readable.Empty();
	for (const BYTE* it = pHash.get(); it < pHash.get() + hashLen; ++it)
	{
		CString tmp;
		tmp.Format(L"%02X", *it);
		if (!readable.IsEmpty())
			readable += L":";
		readable += tmp;
	}

finish:
	if (hHash)
		CryptDestroyHash(hHash);

	return readable;
}

BOOL CCheckCertificateDlg::OnInitDialog()
{
	CStandAloneDialog::OnInitDialog();
	CAppUtils::MarkWindowAsUnpinnable(m_hWnd);

	HCRYPTPROV hCryptProv = 0;
	CryptAcquireContext(&hCryptProv, nullptr, nullptr, PROV_RSA_AES, CRYPT_VERIFYCONTEXT);
	
	m_sSHA1 = getCertificateHash(hCryptProv, CALG_SHA1, (BYTE*)cert->data, cert->len);
	m_sSHA256 = getCertificateHash(hCryptProv, CALG_SHA_256, (BYTE*)cert->data, cert->len);
	if (m_sSHA256.GetLength() > 57)
		m_sSHA256 = m_sSHA256.Left(57) + L"\r\n" + m_sSHA256.Mid(57);

	CryptReleaseContext(hCryptProv, 0);

	CString error;
	error.Format(IDS_ERR_SSL_VALIDATE, m_sHostname);
	SetDlgItemText(IDC_ERRORDESC, error);

	UpdateData(FALSE);

	GetDlgItem(IDCANCEL)->SetFocus();

	return FALSE;
}

void CCheckCertificateDlg::OnBnClickedOpencert()
{
	CTGitPath tempFile = CTempFiles::Instance().GetTempFilePath(true, CTGitPath(_T("certificate.der")));
	
	try
	{
		CFile file(tempFile.GetWinPathString(), CFile::modeReadWrite);
		file.Write(cert->data, (UINT)cert->len);
		file.Close();
	}
	catch (CFileException* e)
	{
		MessageBox(_T("Could not write to file."), _T("TortoiseGit"), MB_ICONERROR);
		e->Delete();
		return;
	}

	if ((int)ShellExecute(this->m_hWnd, NULL, tempFile.GetWinPathString(), NULL, NULL, SW_SHOW) > HINSTANCE_ERROR)
		return;

	CAppUtils::ShowOpenWithDialog(tempFile.GetWinPathString(), GetSafeHwnd());
}
