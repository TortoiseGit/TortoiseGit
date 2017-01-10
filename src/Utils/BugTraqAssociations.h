// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2009, 2014, 2016-2017 - TortoiseGit
// Copyright (C) 2008,2013 - TortoiseSVN

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
#include "TGitPath.h"

struct CBugTraqProvider
{
	CLSID clsid;
	CString name;
};

/* TODO: It's less of an association and more of a "token" or "memento".
 */
class CBugTraqAssociation
{
	CTGitPath m_path;
	CBugTraqProvider m_provider;
	CString m_parameters;
	bool m_enabled;

public:
	CBugTraqAssociation()
		: m_enabled(true)
	{
		 m_provider.clsid = GUID_NULL;
	}

	CBugTraqAssociation(LPCTSTR szWorkingCopy, const CLSID &provider_clsid, LPCTSTR szProviderName, LPCTSTR szParameters, bool enabled)
		: m_path(szWorkingCopy), m_parameters(szParameters), m_enabled(enabled)
	{
		m_provider.clsid = provider_clsid;
		m_provider.name = szProviderName;
	}

	const CTGitPath &GetPath() const { return m_path; }
	CString GetProviderName() const { return m_provider.name; }
	CLSID GetProviderClass() const { return m_provider.clsid; }
	CString GetProviderClassAsString() const;
	CString GetParameters() const { return m_parameters; }
	bool IsEnabled() const { return m_enabled; }
	bool SetEnabled(bool enabled) { if (m_enabled == enabled) { return false; } m_enabled = enabled; return true; }
};

class CBugTraqAssociations
{
	typedef std::vector< CBugTraqAssociation * > inner_t;
	inner_t m_inner;

public:
	CBugTraqAssociations();
	~CBugTraqAssociations();

	void Load(LPCTSTR uuid = nullptr, LPCTSTR params = nullptr);
	void Save() const;

	void Add(const CBugTraqAssociation &assoc);
	void Remove(CBugTraqAssociation* assoc);

	bool FindProvider(const CString &path, CBugTraqAssociation *assoc);

	typedef inner_t::const_iterator const_iterator;
	const_iterator begin() const { return m_inner.begin(); }
	const_iterator end() const { return m_inner.end(); }

	static std::vector<CBugTraqProvider> GetAvailableProviders();
	static CString LookupProviderName(const CLSID &provider_clsid);

private:
	bool FindProviderForPath(const CTGitPath& path, CBugTraqAssociation *assoc) const;

	struct FindByPathPred
	{
		const CTGitPath &m_path;

		FindByPathPred(const CTGitPath &path)
			: m_path(path) { }

		bool operator() (const CBugTraqAssociation *assoc) const
		{
			return (assoc->GetPath().IsEquivalentToWithoutCase(m_path));
		}
	};
	CString	providerUUID;
	CString	providerParams;
	CBugTraqAssociation * pProjectProvider;
};
