// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008 - TortoiseSVN

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

#include <initguid.h>

// {3494FA92-B139-4730-9591-01135D5E7831}
DEFINE_GUID(CATID_BugTraqProvider, 
			0x3494fa92, 0xb139, 0x4730, 0x95, 0x91, 0x1, 0x13, 0x5d, 0x5e, 0x78, 0x31);

#define BUGTRAQ_ASSOCIATIONS_REGPATH _T("Software\\TortoiseGit\\BugTraq Associations")

CBugTraqAssociations::~CBugTraqAssociations()
{
	for (inner_t::iterator it = m_inner.begin(); it != m_inner.end(); ++it)
		delete *it;
}

void CBugTraqAssociations::Load()
{
	HKEY hk;
	if (RegOpenKeyEx(HKEY_CURRENT_USER, BUGTRAQ_ASSOCIATIONS_REGPATH, 0, KEY_READ, &hk) != ERROR_SUCCESS)
		return;

	for (DWORD dwIndex = 0; /* nothing */; ++dwIndex)
	{
		TCHAR szSubKey[MAX_PATH];
		DWORD cchSubKey = MAX_PATH;
		LSTATUS status = RegEnumKeyEx(hk, dwIndex, szSubKey, &cchSubKey, NULL, NULL, NULL, NULL);
		if (status != ERROR_SUCCESS)
			break;

		HKEY hk2;
		if (RegOpenKeyEx(hk, szSubKey, 0, KEY_READ, &hk2) == ERROR_SUCCESS)
		{
			TCHAR szWorkingCopy[MAX_PATH];
			DWORD cbWorkingCopy = sizeof(szWorkingCopy);
			RegQueryValueEx(hk2, _T("WorkingCopy"), NULL, NULL, (LPBYTE)szWorkingCopy, &cbWorkingCopy);

			TCHAR szClsid[MAX_PATH];
			DWORD cbClsid = sizeof(szClsid);
			RegQueryValueEx(hk2, _T("Provider"), NULL, NULL, (LPBYTE)szClsid, &cbClsid);

			CLSID provider_clsid;
			CLSIDFromString(szClsid, &provider_clsid);

			DWORD cbParameters = 0;
			RegQueryValueEx(hk2, _T("Parameters"), NULL, NULL, (LPBYTE)NULL, &cbParameters);
			TCHAR * szParameters = new TCHAR[cbParameters+1];
			RegQueryValueEx(hk2, _T("Parameters"), NULL, NULL, (LPBYTE)szParameters, &cbParameters);
			szParameters[cbParameters] = 0;
			m_inner.push_back(new CBugTraqAssociation(szWorkingCopy, provider_clsid, LookupProviderName(provider_clsid), szParameters));
			delete [] szParameters;

			RegCloseKey(hk2);
		}
	}

	RegCloseKey(hk);
}

void CBugTraqAssociations::Add(const CBugTraqAssociation &assoc)
{
	m_inner.push_back(new CBugTraqAssociation(assoc));
}

bool CBugTraqAssociations::FindProvider(const CTGitPathList &pathList, CBugTraqAssociation *assoc) const
{
	return FindProviderForPathList(pathList, assoc);
}

bool CBugTraqAssociations::FindProvider(const CString &path, CBugTraqAssociation *assoc) const
{

	CTGitPath gitpath;
	gitpath.SetFromUnknown(path);
	return FindProviderForPath(gitpath,assoc);

}
bool CBugTraqAssociations::FindProviderForPathList(const CTGitPathList &pathList, CBugTraqAssociation *assoc) const
{
	for (int i = 0; i < pathList.GetCount(); ++i)
	{
		CTGitPath path = pathList[i];
		if (FindProviderForPath(path, assoc))
			return true;
	}

	return false;
}

bool CBugTraqAssociations::FindProviderForPath(CTGitPath path, CBugTraqAssociation *assoc) const
{
	do
	{
		inner_t::const_iterator it = std::find_if(m_inner.begin(), m_inner.end(), FindByPathPred(path));
		if (it != m_inner.end())
		{
			*assoc = *(*it);
			return true;
		}
		
		path = path.GetContainingDirectory();
	} while(!path.IsEmpty());

	return false;
}

/* static */
CString CBugTraqAssociations::LookupProviderName(const CLSID &provider_clsid)
{
	OLECHAR szClsid[40];
	StringFromGUID2(provider_clsid, szClsid, ARRAYSIZE(szClsid));

	TCHAR szSubKey[MAX_PATH];
	_stprintf_s(szSubKey, _T("CLSID\\%ls"), szClsid);
	
	CString provider_name = CString(szClsid);

	HKEY hk;
	if (RegOpenKeyEx(HKEY_CLASSES_ROOT, szSubKey, 0, KEY_READ, &hk) == ERROR_SUCCESS)
	{
		TCHAR szClassName[MAX_PATH];
		DWORD cbClassName = sizeof(szClassName);

		if (RegQueryValueEx(hk, NULL, NULL, NULL, (LPBYTE)szClassName, &cbClassName) == ERROR_SUCCESS)
			provider_name = CString(szClassName);

		RegCloseKey(hk);
	}

	return provider_name;
}

LSTATUS RegSetValueFromCString(HKEY hKey, LPCTSTR lpValueName, CString str)
{
	LPCTSTR lpsz = str;
	DWORD cb = (str.GetLength() + 1) * sizeof(TCHAR);
	return RegSetValueEx(hKey, lpValueName, 0, REG_SZ, (const BYTE *)lpsz, cb);
}

void CBugTraqAssociations::Save() const
{
	SHDeleteKey(HKEY_CURRENT_USER, BUGTRAQ_ASSOCIATIONS_REGPATH);

	HKEY hk;
	if (RegCreateKeyEx(HKEY_CURRENT_USER, BUGTRAQ_ASSOCIATIONS_REGPATH, 0, NULL, 0, KEY_READ | KEY_WRITE, NULL, &hk, NULL) != ERROR_SUCCESS)
		return;

	DWORD dwIndex = 0;
	for (const_iterator it = begin(); it != end(); ++it)
	{
		TCHAR szSubKey[MAX_PATH];
		_stprintf_s(szSubKey, _T("%d"), dwIndex);

		HKEY hk2;
		if (RegCreateKeyEx(hk, szSubKey, 0, NULL, 0, KEY_WRITE, NULL, &hk2, NULL) == ERROR_SUCCESS)
		{
			RegSetValueFromCString(hk2, _T("Provider"), (*it)->GetProviderClassAsString());
			RegSetValueFromCString(hk2, _T("WorkingCopy"), (*it)->GetPath().GetWinPath());
			RegSetValueFromCString(hk2, _T("Parameters"), (*it)->GetParameters());
			
			RegCloseKey(hk2);
		}

		++dwIndex;
	}

	RegCloseKey(hk);
}

void CBugTraqAssociations::RemoveByPath(const CTGitPath &path)
{
	inner_t::iterator it = std::find_if(m_inner.begin(), m_inner.end(), FindByPathPred(path));
	if (it != m_inner.end())
	{
		delete *it;
		m_inner.erase(it);
	}
}

CString CBugTraqAssociation::GetProviderClassAsString() const
{
	OLECHAR szTemp[40];
	StringFromGUID2(m_provider.clsid, szTemp, ARRAYSIZE(szTemp));

	return CString(szTemp);
}

/* static */
std::vector<CBugTraqProvider> CBugTraqAssociations::GetAvailableProviders()
{
	std::vector<CBugTraqProvider> results;

	ICatInformation *pCatInformation = NULL;

	HRESULT hr;
	if (SUCCEEDED(hr = CoCreateInstance(CLSID_StdComponentCategoriesMgr, NULL, CLSCTX_ALL, IID_ICatInformation, (void **)&pCatInformation)))
	{
		IEnumGUID *pEnum = NULL;
		if (SUCCEEDED(hr = pCatInformation->EnumClassesOfCategories(1, &CATID_BugTraqProvider, 0, NULL, &pEnum)))
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
		pEnum = NULL;
	}

	if (pCatInformation)
		pCatInformation->Release();
	pCatInformation = NULL;

	return results;
}
