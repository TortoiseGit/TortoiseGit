// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2007 - TortoiseSVN

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
#include "registry.h"
#include "RegHistory.h"


CRegHistory::CRegHistory() : m_nMaxHistoryItems(25)
{
}

CRegHistory::~CRegHistory()
{
}

bool CRegHistory::AddEntry(LPCTSTR szText)
{
	if (_tcslen(szText) == 0)
		return false;

	if ((!m_sSection.empty())&&(!m_sKeyPrefix.empty()))
	{
		// refresh the history from the registry
		Load(m_sSection.c_str(), m_sKeyPrefix.c_str());
	}

	for (size_t i=0; i<m_arEntries.size(); ++i)
	{
		if (_tcscmp(szText, m_arEntries[i].c_str())==0)
		{
			m_arEntries.erase(m_arEntries.begin() + i);
			m_arEntries.insert(m_arEntries.begin(), szText);
			return false;
		}
	}
	m_arEntries.insert(m_arEntries.begin(), szText);
	return true;
}

void CRegHistory::RemoveEntry(int pos)
{
	m_arEntries.erase(m_arEntries.begin() + pos);
}

size_t CRegHistory::Load(LPCTSTR lpszSection, LPCTSTR lpszKeyPrefix)
{
	if (lpszSection == NULL || lpszKeyPrefix == NULL || *lpszSection == '\0')
		return (size_t)(-1);

	m_arEntries.clear();

	m_sSection = lpszSection;
	m_sKeyPrefix = lpszKeyPrefix;

	int n = 0;
	std::wstring sText;
	do
	{
		//keys are of form <lpszKeyPrefix><entrynumber>
		TCHAR sKey[4096] = {0};
		_stprintf_s(sKey, 4096, _T("%s\\%s%d"), lpszSection, lpszKeyPrefix, n++);
		sText = CRegStdString(sKey);
		if (!sText.empty())
		{
			m_arEntries.push_back(sText);
		}
	} while (!sText.empty() && n < m_nMaxHistoryItems);

	return m_arEntries.size();
}

bool CRegHistory::Save() const
{
	if (m_sSection.empty())
		return false;

	// save history to registry
	int nMax = min((int)m_arEntries.size(), m_nMaxHistoryItems + 1);
	for (int n = 0; n < (int)m_arEntries.size(); ++n)
	{
		TCHAR sKey[4096] = {0};
		_stprintf_s(sKey, 4096, _T("%s\\%s%d"), m_sSection.c_str(), m_sKeyPrefix.c_str(), n);
		CRegStdString regkey = CRegStdString(sKey);
		regkey = m_arEntries[n];
	}
	// remove items exceeding the max number of history items
	for (int n = nMax; ; ++n)
	{
		TCHAR sKey[4096] = {0};
		_stprintf_s(sKey, 4096, _T("%s\\%s%d"), m_sSection.c_str(), m_sKeyPrefix.c_str(), n);
		CRegStdString regkey = CRegStdString(sKey);
		if (((tstring)regkey).empty())
			break;
		regkey.removeValue(); // remove entry
	}
	return true;
}

