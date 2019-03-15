// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2013-2017, 2019 - TortoiseGit

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
#pragma comment(lib, "Crypt32.lib")

static DWORD VerifyServerCertificate(PCCERT_CONTEXT pServerCert, PTSTR pwszServerName, DWORD dwCertFlags)
{
	if (pServerCert == nullptr || pwszServerName == nullptr)
		return static_cast<DWORD>(SEC_E_WRONG_PRINCIPAL);

	LPSTR rgszUsages[] = { szOID_PKIX_KP_SERVER_AUTH, szOID_SERVER_GATED_CRYPTO, szOID_SGC_NETSCAPE };
	DWORD cUsages = sizeof(rgszUsages) / sizeof(LPSTR);

	// Build certificate chain.
	CERT_CHAIN_PARA ChainPara = { 0 };
	ChainPara.cbSize = sizeof(ChainPara);
	ChainPara.RequestedUsage.dwType = USAGE_MATCH_TYPE_OR;
	ChainPara.RequestedUsage.Usage.cUsageIdentifier = cUsages;
	ChainPara.RequestedUsage.Usage.rgpszUsageIdentifier = rgszUsages;

	PCCERT_CHAIN_CONTEXT pChainContext = nullptr;
	if (!CertGetCertificateChain(nullptr, pServerCert, nullptr, pServerCert->hCertStore, &ChainPara, CERT_CHAIN_REVOCATION_CHECK_CHAIN_EXCLUDE_ROOT, nullptr, &pChainContext))
		return GetLastError();
	SCOPE_EXIT { CertFreeCertificateChain(pChainContext); };

	// Validate certificate chain.
	HTTPSPolicyCallbackData polHttps = { 0 };
	polHttps.cbStruct = sizeof(HTTPSPolicyCallbackData);
	polHttps.dwAuthType = AUTHTYPE_SERVER;
	polHttps.fdwChecks = dwCertFlags;
	polHttps.pwszServerName = pwszServerName;

	CERT_CHAIN_POLICY_PARA PolicyPara = { 0 };
	PolicyPara.cbSize = sizeof(PolicyPara);
	PolicyPara.pvExtraPolicyPara = &polHttps;

	CERT_CHAIN_POLICY_STATUS PolicyStatus = { 0 };
	PolicyStatus.cbSize = sizeof(PolicyStatus);

	if (!CertVerifyCertificateChainPolicy(CERT_CHAIN_POLICY_SSL, pChainContext, &PolicyPara, &PolicyStatus))
		return GetLastError();

	if (PolicyStatus.dwError)
		return PolicyStatus.dwError;

	return SEC_E_OK;
}
