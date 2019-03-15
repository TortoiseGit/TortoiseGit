// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2009, 2012-2016, 2019 - TortoiseGit
// Copyright (C) 2008,2014 - TortoiseSVN

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
#include "BugTraqAssociations.h"
#include <memory>
#include <initguid.h>

// {3494FA92-B139-4730-9591-01135D5E7831}
DEFINE_GUID(CATID_BugTraqProvider,
			0x3494fa92, 0xb139, 0x4730, 0x95, 0x91, 0x1, 0x13, 0x5d, 0x5e, 0x78, 0x31);

#define BUGTRAQ_ASSOCIATIONS_REGPATH L"Software\\TortoiseGit\\BugTraq Associations"

CBugTraqAssociations::CBugTraqAssociations()
: pProjectProvider(nullptr)
{
}

CBugTraqAssociations::~CBugTraqAssociations()
{
	for (inner_t::iterator it = m_inner.begin(); it != m_inner.end(); ++it)
		delete *it;
	if (pProjectProvider)
		delete pProjectProvider;
}

void CBugTraqAssociations::Load(LPCTSTR uuid /* = nullptr */, LPCTSTR params /* = nullptr */)
{
	HKEY hk;
	if (RegOpenKeyEx(HKEY_CURRENT_USER, BUGTRAQ_ASSOCIATIONS_REGPATH, 0, KEY_READ, &hk) != ERROR_SUCCESS)
	{
		if (uuid)
			providerUUID = uuid;
		if (params)
			providerParams = params;
		return;
	}

	for (DWORD dwIndex = 0; /* nothing */; ++dwIndex)
	{
		TCHAR szSubKey[MAX_PATH] = {0};
		DWORD cchSubKey = MAX_PATH;
		LSTATUS status = RegEnumKeyEx(hk, dwIndex, szSubKey, &cchSubKey, nullptr, nullptr, nullptr, nullptr);
		if (status != ERROR_SUCCESS)
			break;

		HKEY hk2;
		if (RegOpenKeyEx(hk, szSubKey, 0, KEY_READ, &hk2) == ERROR_SUCCESS)
		{
			TCHAR szWorkingCopy[MAX_PATH] = {0};
			DWORD cbWorkingCopy = sizeof(szWorkingCopy);
			RegQueryValueEx(hk2, L"WorkingCopy", nullptr, nullptr, reinterpret_cast<LPBYTE>(szWorkingCopy), &cbWorkingCopy);

			TCHAR szClsid[MAX_PATH] = {0};
			DWORD cbClsid = sizeof(szClsid);
			RegQueryValueEx(hk2, L"Provider", nullptr, nullptr, reinterpret_cast<LPBYTE>(szClsid), &cbClsid);

			CLSID provider_clsid;
			CLSIDFromString(szClsid, &provider_clsid);

			DWORD cbParameters = 0;
			RegQueryValueEx(hk2, L"Parameters", nullptr, nullptr, nullptr, &cbParameters);
			auto szParameters = std::make_unique<TCHAR[]>(cbParameters + 1);
			RegQueryValueEx(hk2, L"Parameters", nullptr, nullptr, reinterpret_cast<LPBYTE>(szParameters.get()), &cbParameters);
			szParameters[cbParameters] = 0;

			DWORD enabled = TRUE;
			DWORD size = sizeof(enabled);
			RegQueryValueEx(hk2, L"Enabled", nullptr, nullptr, reinterpret_cast<BYTE*>(&enabled), &size);

			m_inner.push_back(new CBugTraqAssociation(szWorkingCopy, provider_clsid, LookupProviderName(provider_clsid), szParameters.get(), enabled != FALSE));

			RegCloseKey(hk2);
		}
	}

	RegCloseKey(hk);

	if (uuid)
		providerUUID = uuid;
	if (params)
		providerParams = params;
}

void CBugTraqAssociations::Add(const CBugTraqAssociation &assoc)
{
	m_inner.push_back(new CBugTraqAssociation(assoc));
}

bool CBugTraqAssociations::FindProvider(const CString &path, CBugTraqAssociation *assoc)
{
	CTGitPath gitpath;
	gitpath.SetFromUnknown(path);

	if (FindProviderForPath(gitpath, assoc))
		return true;

	if (pProjectProvider)
	{
		if (assoc)
			*assoc = *pProjectProvider;
		return true;
	}
	if (!providerUUID.IsEmpty())
	{
		CLSID provider_clsid;
		CLSIDFromString(static_cast<LPCOLESTR>(static_cast<LPCWSTR>(providerUUID)), &provider_clsid);
		pProjectProvider = new CBugTraqAssociation(L"", provider_clsid, L"bugtraq:provider", static_cast<LPCWSTR>(providerParams), true);
		if (pProjectProvider)
		{
			if (assoc)
				*assoc = *pProjectProvider;
			return true;
		}
	}
	return false;
}

bool CBugTraqAssociations::FindProviderForPath(const CTGitPath& path, CBugTraqAssociation *assoc) const
{
	const auto it = std::find_if(m_inner.cbegin(), m_inner.cend(), [&path](const CBugTraqAssociation* assoc) { return assoc->IsEnabled() && assoc->GetPath().IsEquivalentToWithoutCase(path); });
		if (it != m_inner.end())
		{
			*assoc = *(*it);
			return true;
		}

	return false;
}

/* static */
CString CBugTraqAssociations::LookupProviderName(const CLSID &provider_clsid)
{
	OLECHAR szClsid[40] = { 0 };
	StringFromGUID2(provider_clsid, szClsid, ARRAYSIZE(szClsid));

	TCHAR szSubKey[MAX_PATH] = {0};
	swprintf_s(szSubKey, L"CLSID\\%ls", szClsid);

	CString provider_name = CString(szClsid);

	HKEY hk;
	if (RegOpenKeyEx(HKEY_CLASSES_ROOT, szSubKey, 0, KEY_READ, &hk) == ERROR_SUCCESS)
	{
		TCHAR szClassName[MAX_PATH] = {0};
		DWORD cbClassName = sizeof(szClassName);

		if (RegQueryValueEx(hk, nullptr, nullptr, nullptr, reinterpret_cast<LPBYTE>(szClassName), &cbClassName) == ERROR_SUCCESS)
			provider_name = CString(szClassName);

		RegCloseKey(hk);
	}

	return provider_name;
}

LSTATUS RegSetValueFromCString(HKEY hKey, LPCTSTR lpValueName, CString str)
{
	LPCTSTR lpsz = str;
	DWORD cb = (str.GetLength() + 1) * sizeof(TCHAR);
	return RegSetValueEx(hKey, lpValueName, 0, REG_SZ, reinterpret_cast<const BYTE*>(lpsz), cb);
}

void CBugTraqAssociations::Save() const
{
	SHDeleteKey(HKEY_CURRENT_USER, BUGTRAQ_ASSOCIATIONS_REGPATH);

	HKEY hk;
	if (RegCreateKeyEx(HKEY_CURRENT_USER, BUGTRAQ_ASSOCIATIONS_REGPATH, 0, nullptr, 0, KEY_READ | KEY_WRITE, nullptr, &hk, nullptr) != ERROR_SUCCESS)
		return;

	DWORD dwIndex = 0;
	for (const_iterator it = begin(); it != end(); ++it)
	{
		TCHAR szSubKey[MAX_PATH] = {0};
		swprintf_s(szSubKey, L"%lu", dwIndex);

		HKEY hk2;
		if (RegCreateKeyEx(hk, szSubKey, 0, nullptr, 0, KEY_WRITE, nullptr, &hk2, nullptr) == ERROR_SUCCESS)
		{
			RegSetValueFromCString(hk2, L"Provider", (*it)->GetProviderClassAsString());
			RegSetValueFromCString(hk2, L"WorkingCopy", (*it)->GetPath().GetWinPath());
			RegSetValueFromCString(hk2, L"Parameters", (*it)->GetParameters());
			DWORD enabled = (*it)->IsEnabled() ? 1 : 0;
			RegSetValueEx(hk2, L"Enabled", 0, REG_DWORD, reinterpret_cast<BYTE*>(&enabled), sizeof(enabled));

			RegCloseKey(hk2);
		}

		++dwIndex;
	}

	RegCloseKey(hk);
}

void CBugTraqAssociations::Remove(CBugTraqAssociation* assoc)
{
	inner_t::iterator it = std::find(m_inner.begin(), m_inner.end(), assoc);
	if (it != m_inner.end())
	{
		delete *it;
		m_inner.erase(it);
	}
}

CString CBugTraqAssociation::GetProviderClassAsString() const
{
	OLECHAR szTemp[40] = { 0 };
	StringFromGUID2(m_provider.clsid, szTemp, ARRAYSIZE(szTemp));

	return CString(szTemp);
}

/* static */
std::vector<CBugTraqProvider> CBugTraqAssociations::GetAvailableProviders()
{
	std::vector<CBugTraqProvider> results;

	ICatInformation* pCatInformation = nullptr;

	HRESULT hr;
	if (SUCCEEDED(hr = CoCreateInstance(CLSID_StdComponentCategoriesMgr, nullptr, CLSCTX_ALL, IID_ICatInformation, reinterpret_cast<void**>(&pCatInformation))))
	{
		IEnumGUID* pEnum = nullptr;
		if (SUCCEEDED(hr = pCatInformation->EnumClassesOfCategories(1, &CATID_BugTraqProvider, 0, nullptr, &pEnum)))
		{
			HRESULT hrEnum;
			do
			{
				CLSID clsids[5];
				ULONG cClsids;

				hrEnum = pEnum->Next(ARRAYSIZE(clsids), clsids, &cClsids);
				if (SUCCEEDED(hrEnum))
				{
					for (ULONG i = 0; i < cClsids; ++i)
					{
						CBugTraqProvider provider;
						provider.clsid = clsids[i];
						provider.name = LookupProviderName(clsids[i]);
						results.push_back(provider);
					}
				}
			} while (hrEnum == S_OK);
		}

		if (pEnum)
			pEnum->Release();
		pEnum = nullptr;
	}

	if (pCatInformation)
		pCatInformation->Release();
	pCatInformation = nullptr;

	return results;
}
