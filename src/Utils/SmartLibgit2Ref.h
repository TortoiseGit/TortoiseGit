// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2014 - TortoiseGit
// based on SmartHandle of TortoiseSVN

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
#include "UnicodeUtils.h"

/**
* \ingroup Utils
* Helper classes for libgit2 references.
*/
template <typename ReferenceType>
class CSmartLibgit2Ref
{
public:
	CSmartLibgit2Ref()
	{
		m_Ref = nullptr;
	}

	CSmartLibgit2Ref(ReferenceType* h)
	{
		m_Ref = h;
	}

	ReferenceType* operator=(ReferenceType* h)
	{
		if (m_Ref != h)
		{
			CleanUp();
			m_Ref = h;
		}

		return (*this);
	}

	void Free()
	{
		CleanUp();
	}

	void Detach()
	{
		m_Handle = nullptr;
	}

	operator ReferenceType*()
	{
		return m_Ref;
	}

	ReferenceType** GetPointer()
	{
		return &m_Ref;
	}

	operator bool()
	{
		return IsValid();
	}

	bool IsValid()
	{
		return m_Ref != nullptr;
	}

protected:
	virtual void FreeRef() = 0;

	void CleanUp()
	{
		if (m_Ref != nullptr)
		{
			FreeRef();
			m_Ref = nullptr;
		}
	}

	ReferenceType* m_Ref;
};

class CAutoRepository : public CSmartLibgit2Ref<git_repository>
{
public:
	CAutoRepository() {};

	CAutoRepository(const CString& gitDir)
	{
		Open(gitDir);
	}

	int Open(const CString& gitDir)
	{
		return git_repository_open(GetPointer(), CUnicodeUtils::GetUTF8(gitDir));
	}

	~CAutoRepository()
	{
		CleanUp();
	}

protected:
	virtual void FreeRef()
	{
		git_repository_free(m_Ref);
	}
};

class CAutoCommit : public CSmartLibgit2Ref<git_commit>
{
public:
	~CAutoCommit()
	{
		CleanUp();
	}
protected:
	virtual void FreeRef()
	{
		git_commit_free(m_Ref);
	}
};

class CAutoTree : public CSmartLibgit2Ref<git_tree>
{
public:
	~CAutoTree()
	{
		CleanUp();
	}

protected:
	virtual void FreeRef()
	{
		git_tree_free(m_Ref);
	}
};

class CAutoObject : public CSmartLibgit2Ref<git_object>
{
public:
	~CAutoObject()
	{
		CleanUp();
	}

protected:
	virtual void FreeRef()
	{
		git_object_free(m_Ref);
	}
};

class CAutoBlob : public CSmartLibgit2Ref<git_blob>
{
public:
	~CAutoBlob()
	{
		CleanUp();
	}

protected:
	virtual void FreeRef()
	{
		git_blob_free(m_Ref);
	}
};

class CAutoConfig : public CSmartLibgit2Ref<git_config>
{
public:
	~CAutoConfig()
	{
		CleanUp();
	}

protected:
	virtual void FreeRef()
	{
		git_config_free(m_Ref);
	}
};
