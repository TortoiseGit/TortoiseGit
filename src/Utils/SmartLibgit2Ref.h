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

	void Swap(CSmartLibgit2Ref& tmp)
	{
		ReferenceType* p;

		p = m_Ref;
		m_Ref = tmp.m_Ref;
		tmp.m_Ref = p;
	}

	void Free()
	{
		CleanUp();
	}

	ReferenceType* Detach()
	{
		ReferenceType* p;

		p = m_Ref;
		m_Ref = nullptr;

		return p;
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

	bool IsValid() const
	{
		return m_Ref != nullptr;
	}

protected:
	virtual void FreeRef() = 0;

	void CleanUp()
	{
		if (IsValid())
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

	CAutoRepository(git_repository* h)
	{
		m_Ref = h;
	}

	CAutoRepository(const CString& gitDir)
	{
		Open(gitDir);
	}

	CAutoRepository(const CStringA& gitDirA)
	{
		Open(gitDirA);
	}

	int Open(const CString& gitDir)
	{
		return Open(CUnicodeUtils::GetUTF8(gitDir));
	}

	int Open(const CStringA& gitDirA)
	{
		CleanUp();
		return git_repository_open(GetPointer(), gitDirA);
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
	CAutoCommit() {}

	CAutoCommit(git_commit* h)
	{
		m_Ref = h;
	}

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
	git_tree* operator=(git_tree* h)
	{
		if (m_Ref != h)
		{
			CleanUp();
			m_Ref = h;
		}
		return (*this);
	}

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
	CAutoConfig() {}

	CAutoConfig(CAutoRepository &repo)
	{
		git_repository_config(&m_Ref, repo);
	}

	CAutoConfig(bool init)
	{
		if (init)
			New();
	}

	void New()
	{
		CleanUp();
		git_config_new(&m_Ref);
	}

	~CAutoConfig()
	{
		CleanUp();
	}

	int GetString(const CString &key, CString &value) const
	{
		if (!IsValid())
			return -1;

		const char* out = nullptr;
		int ret = 0;
		if ((ret = git_config_get_string(&out, m_Ref, CUnicodeUtils::GetUTF8(key))))
		{
			value.Empty();
			return ret;
		}

		value = CUnicodeUtils::GetUnicode((CStringA)out);

		return ret;
	}

	int GetBOOL(const CString &key, BOOL &b) const
	{
		if (!IsValid())
			return -1;

		return git_config_get_bool(&b, m_Ref, CUnicodeUtils::GetUTF8(key));
	}

	int GetBool(const CString &key, bool &b) const
	{
		if (!IsValid())
			return -1;

		int value = FALSE;
		int ret = 0;
		if ((ret = GetBOOL(key, value)))
			return ret;

		b = (value == TRUE);

		return ret;
	}

protected:
	virtual void FreeRef()
	{
		git_config_free(m_Ref);
	}
};

class CAutoReference : public CSmartLibgit2Ref<git_reference>
{
public:
	CAutoReference() {}

	CAutoReference(git_reference* h)
	{
		m_Ref = h;
	}

	~CAutoReference()
	{
		CleanUp();
	}

protected:
	virtual void FreeRef()
	{
		git_reference_free(m_Ref);
	}
};

class CAutoTreeEntry : public CSmartLibgit2Ref<git_tree_entry>
{
public:
	~CAutoTreeEntry()
	{
		CleanUp();
	}

protected:
	virtual void FreeRef()
	{
		git_tree_entry_free(m_Ref);
	}
};

class CAutoDiff : public CSmartLibgit2Ref<git_diff>
{
public:
	~CAutoDiff()
	{
		CleanUp();
	}

protected:
	virtual void FreeRef()
	{
		git_diff_free(m_Ref);
	}
};

class CAutoPatch : public CSmartLibgit2Ref<git_patch>
{
public:
	~CAutoPatch()
	{
		CleanUp();
	}

protected:
	virtual void FreeRef()
	{
		git_patch_free(m_Ref);
	}
};

class CAutoIndex : public CSmartLibgit2Ref<git_index>
{
public:
	~CAutoIndex()
	{
		CleanUp();
	}

protected:
	virtual void FreeRef()
	{
		git_index_free(m_Ref);
	}
};

class CAutoRemote : public CSmartLibgit2Ref<git_remote>
{
public:
	~CAutoRemote()
	{
		CleanUp();
	}

protected:
	virtual void FreeRef()
	{
		if (git_remote_connected(m_Ref) == 1)
			git_remote_disconnect(m_Ref);
		git_remote_free(m_Ref);
	}
};

class CAutoReflog : public CSmartLibgit2Ref<git_reflog>
{
public:
	~CAutoReflog()
	{
		CleanUp();
	}

protected:
	virtual void FreeRef()
	{
		git_reflog_free(m_Ref);
	}
};

class CAutoRevwalk : public CSmartLibgit2Ref<git_revwalk>
{
public:
	~CAutoRevwalk()
	{
		CleanUp();
	}

protected:
	virtual void FreeRef()
	{
		git_revwalk_free(m_Ref);
	}
};

class CAutoBranchIterator : public CSmartLibgit2Ref<git_branch_iterator>
{
public:
	~CAutoBranchIterator()
	{
		CleanUp();
	}

protected:
	virtual void FreeRef()
	{
		git_branch_iterator_free(m_Ref);
	}
};

template <typename HandleType, class FreeFunction>
class CSmartBuffer : public FreeFunction
{
public:
	CSmartBuffer()
	{
		HandleType tmp = { 0 };
		m_Ref = tmp;
	}

	operator HandleType*()
	{
		return &m_Ref;
	}

	HandleType* operator->()
	{
		return &m_Ref;
	}

	~CSmartBuffer()
	{
		Free(&m_Ref);
	}

protected:
	HandleType m_Ref;
};

struct CFreeBuf
{
protected:
	void Free(git_buf* ref)
	{
		git_buf_free(ref);
	}

	~CFreeBuf()
	{
	}
};

struct CFreeStrArray
{
protected:
	void Free(git_strarray* ref)
	{
		git_strarray_free(ref);
	}

	~CFreeStrArray()
	{
	}
};

typedef CSmartBuffer<git_buf, CFreeBuf>	CAutoBuf;
typedef CSmartBuffer<git_strarray, CFreeStrArray>	CAutoStrArray;
