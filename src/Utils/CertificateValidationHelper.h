// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2013-2014 - TortoiseGit

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

#include <wincrypt.h>

static DWORD VerifyServerCertificate(PCCERT_CONTEXT pServerCert, PTSTR pwszServerName, DWORD dwCertFlags)
{
	HTTPSPolicyCallbackData polHttps;
	CERT_CHAIN_POLICY_PARA PolicyPara;
	CERT_CHAIN_POLICY_STATUS PolicyStatus;
	CERT_CHAIN_PARA ChainPara;
	PCCERT_CHAIN_CONTEXT pChainContext = nullptr;
	DWORD Status;
	LPSTR rgszUsages[] = { szOID_PKIX_KP_SERVER_AUTH, szOID_SERVER_GATED_CRYPTO, szOID_SGC_NETSCAPE };

	DWORD cUsages = sizeof(rgszUsages) / sizeof(LPSTR);

	if (pServerCert == nullptr || pwszServerName == nullptr)
	{
		Status = (DWORD)SEC_E_WRONG_PRINCIPAL;
		goto cleanup;
	}

	// Build certificate chain.
	SecureZeroMemory(&ChainPara, sizeof(ChainPara));
	ChainPara.cbSize = sizeof(ChainPara);
	ChainPara.RequestedUsage.dwType = USAGE_MATCH_TYPE_OR;
	ChainPara.RequestedUsage.Usage.cUsageIdentifier = cUsages;
	ChainPara.RequestedUsage.Usage.rgpszUsageIdentifier = rgszUsages;

	if (!CertGetCertificateChain(nullptr, pServerCert, nullptr, pServerCert->hCertStore, &ChainPara, 0, nullptr, &pChainContext))
	{
		Status = GetLastError();
		goto cleanup;
	}

	// Validate certificate chain.
	SecureZeroMemory(&polHttps, sizeof(HTTPSPolicyCallbackData));
	polHttps.cbStruct = sizeof(HTTPSPolicyCallbackData);
	polHttps.dwAuthType = AUTHTYPE_SERVER;
	polHttps.fdwChecks = dwCertFlags;
	polHttps.pwszServerName = pwszServerName;

	SecureZeroMemory(&PolicyPara, sizeof(PolicyPara));
	PolicyPara.cbSize = sizeof(PolicyPara);
	PolicyPara.pvExtraPolicyPara = &polHttps;

	SecureZeroMemory(&PolicyStatus, sizeof(PolicyStatus));
	PolicyStatus.cbSize = sizeof(PolicyStatus);

	if (!CertVerifyCertificateChainPolicy(CERT_CHAIN_POLICY_SSL, pChainContext, &PolicyPara, &PolicyStatus))
	{
		Status = GetLastError();
		goto cleanup;
	}

	if (PolicyStatus.dwError)
	{
		Status = PolicyStatus.dwError;
		goto cleanup;
	}

	Status = SEC_E_OK;

cleanup:
	if (pChainContext)
		CertFreeCertificateChain(pChainContext);

	return Status;
}
