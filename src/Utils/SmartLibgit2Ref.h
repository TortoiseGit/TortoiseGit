// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2014-2016 - TortoiseGit
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
template <typename HandleType, class FreeFunction>
class CSmartBuffer
{
public:
	CSmartBuffer()
	{
		m_Ref = { 0 };
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
		FreeFunction::Free(&m_Ref);
	}

private:
	CSmartBuffer(const CSmartBuffer&) = delete;
	CSmartBuffer& operator=(const CSmartBuffer&) = delete;

protected:
	HandleType m_Ref;
};

struct CFreeBuf
{
	static void Free(git_buf* ref)
	{
		git_buf_free(ref);
	}
};

struct CFreeStrArray
{
	static void Free(git_strarray* ref)
	{
		git_strarray_free(ref);
	}
};

typedef CSmartBuffer<git_buf, CFreeBuf>	CAutoBuf;
typedef CSmartBuffer<git_strarray, CFreeStrArray>	CAutoStrArray;

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
		ReferenceType* p = m_Ref;
		m_Ref = nullptr;

		return p;
	}

	operator ReferenceType*() const
	{
		return m_Ref;
	}

	/**
	 * Free the wrapped object and return a pointer to the wrapped object ReferenceType*
	 */
	ReferenceType** GetPointer()
	{
		CleanUp();
		return &m_Ref;
	}

	operator bool() const
	{
		return IsValid();
	}

	bool IsValid() const
	{
		return m_Ref != nullptr;
	}

private:
	CSmartLibgit2Ref(const CSmartLibgit2Ref&) = delete;
	CSmartLibgit2Ref& operator=(const CSmartLibgit2Ref&) = delete;

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

	CAutoRepository(CAutoRepository&& that)
	{
		m_Ref = that.Detach();
	}

	int Open(const CString& gitDir)
	{
		return Open(CUnicodeUtils::GetUTF8(gitDir));
	}

	~CAutoRepository()
	{
		CleanUp();
	}

private:
	CAutoRepository(const CAutoRepository&) = delete;
	CAutoRepository& operator=(const CAutoRepository&) = delete;

protected:
	int Open(const CStringA& gitDirA)
	{
		return git_repository_open(GetPointer(), gitDirA);
	}

	virtual void FreeRef()
	{
		git_repository_free(m_Ref);
	}
};

class CAutoSubmodule : public CSmartLibgit2Ref<git_submodule>
{
public:
	CAutoSubmodule() {};

	~CAutoSubmodule()
	{
		CleanUp();
	}

private:
	CAutoSubmodule(const CAutoSubmodule&) = delete;
	CAutoSubmodule& operator=(const CAutoSubmodule&) = delete;

protected:
	virtual void FreeRef()
	{
		git_submodule_free(m_Ref);
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

private:
	CAutoCommit(const CAutoCommit&) = delete;
	CAutoCommit& operator=(const CAutoCommit&) = delete;

protected:
	virtual void FreeRef()
	{
		git_commit_free(m_Ref);
	}
};

class CAutoTree : public CSmartLibgit2Ref<git_tree>
{
public:
	CAutoTree() {};

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

private:
	CAutoTree(const CAutoTree&) = delete;
	CAutoTree& operator=(const CAutoTree&) = delete;

protected:
	virtual void FreeRef()
	{
		git_tree_free(m_Ref);
	}
};

class CAutoObject : public CSmartLibgit2Ref<git_object>
{
public:
	CAutoObject() {};

	~CAutoObject()
	{
		CleanUp();
	}

private:
	CAutoObject(const CAutoObject&) = delete;
	CAutoObject& operator=(const CAutoObject&) = delete;

protected:
	virtual void FreeRef()
	{
		git_object_free(m_Ref);
	}
};

class CAutoBlob : public CSmartLibgit2Ref<git_blob>
{
public:
	CAutoBlob() {};

	~CAutoBlob()
	{
		CleanUp();
	}

private:
	CAutoBlob(const CAutoBlob&) = delete;
	CAutoBlob& operator=(const CAutoBlob&) = delete;

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

		CAutoBuf buf;
		int ret = 0;
		if ((ret = git_config_get_string_buf(buf, m_Ref, CUnicodeUtils::GetUTF8(key))))
		{
			value.Empty();
			return ret;
		}

		value = CUnicodeUtils::GetUnicode((CStringA)buf->ptr);

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
		if ((ret = GetBOOL(key, value)) < 0)
			return ret;

		b = (value == TRUE);

		return ret;
	}

private:
	CAutoConfig(const CAutoConfig&) = delete;
	CAutoConfig& operator=(const CAutoConfig&) = delete;

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

private:
	CAutoReference(const CAutoReference&) = delete;
	CAutoReference& operator=(const CAutoReference&) = delete;

protected:
	virtual void FreeRef()
	{
		git_reference_free(m_Ref);
	}
};

class CAutoTag : public CSmartLibgit2Ref<git_tag>
{
public:
	CAutoTag() {}

	CAutoTag(git_tag* h)
	{
		m_Ref = h;
	}

	~CAutoTag()
	{
		CleanUp();
	}

private:
	CAutoTag(const CAutoTag&) = delete;
	CAutoTag& operator=(const CAutoTag&) = delete;

protected:
	virtual void FreeRef()
	{
		git_tag_free(m_Ref);
	}
};

class CAutoTreeEntry : public CSmartLibgit2Ref<git_tree_entry>
{
public:
	CAutoTreeEntry() {};

	~CAutoTreeEntry()
	{
		CleanUp();
	}

private:
	CAutoTreeEntry(const CAutoTreeEntry&) = delete;
	CAutoTreeEntry& operator=(const CAutoTreeEntry&) = delete;

protected:
	virtual void FreeRef()
	{
		git_tree_entry_free(m_Ref);
	}
};

class CAutoDiff : public CSmartLibgit2Ref<git_diff>
{
public:
	CAutoDiff() {};

	~CAutoDiff()
	{
		CleanUp();
	}

private:
	CAutoDiff(const CAutoDiff&) = delete;
	CAutoDiff& operator=(const CAutoDiff&) = delete;

protected:
	virtual void FreeRef()
	{
		git_diff_free(m_Ref);
	}
};

class CAutoPatch : public CSmartLibgit2Ref<git_patch>
{
public:
	CAutoPatch() {};

	~CAutoPatch()
	{
		CleanUp();
	}

private:
	CAutoPatch(const CAutoPatch&) = delete;
	CAutoPatch& operator=(const CAutoPatch&) = delete;

protected:
	virtual void FreeRef()
	{
		git_patch_free(m_Ref);
	}
};

class CAutoDiffStats : public CSmartLibgit2Ref<git_diff_stats>
{
public:
	CAutoDiffStats() {};

	~CAutoDiffStats()
	{
		CleanUp();
	}

private:
	CAutoDiffStats(const CAutoDiffStats&) = delete;
	CAutoDiffStats& operator=(const CAutoDiffStats&) = delete;

protected:
	virtual void FreeRef()
	{
		git_diff_stats_free(m_Ref);
	}
};

class CAutoIndex : public CSmartLibgit2Ref<git_index>
{
public:
	CAutoIndex() {};

	~CAutoIndex()
	{
		CleanUp();
	}

private:
	CAutoIndex(const CAutoIndex& that);
	CAutoIndex& operator=(const CAutoIndex& that);

protected:
	virtual void FreeRef()
	{
		git_index_free(m_Ref);
	}
};

class CAutoRemote : public CSmartLibgit2Ref<git_remote>
{
public:
	CAutoRemote() {};

	~CAutoRemote()
	{
		CleanUp();
	}

private:
	CAutoRemote(const CAutoRemote&) = delete;
	CAutoRemote& operator=(const CAutoRemote&) = delete;

protected:
	virtual void FreeRef()
	{
		git_remote_free(m_Ref);
	}
};

class CAutoReflog : public CSmartLibgit2Ref<git_reflog>
{
public:
	CAutoReflog() {};

	~CAutoReflog()
	{
		CleanUp();
	}

private:
	CAutoReflog(const CAutoReflog&) = delete;
	CAutoReflog& operator=(const CAutoReflog&) = delete;

protected:
	virtual void FreeRef()
	{
		git_reflog_free(m_Ref);
	}
};

class CAutoRevwalk : public CSmartLibgit2Ref<git_revwalk>
{
public:
	CAutoRevwalk() {};

	~CAutoRevwalk()
	{
		CleanUp();
	}

private:
	CAutoRevwalk(const CAutoRevwalk&) = delete;
	CAutoRevwalk& operator=(const CAutoRevwalk&) = delete;

protected:
	virtual void FreeRef()
	{
		git_revwalk_free(m_Ref);
	}
};

class CAutoBranchIterator : public CSmartLibgit2Ref<git_branch_iterator>
{
public:
	CAutoBranchIterator() {};

	~CAutoBranchIterator()
	{
		CleanUp();
	}

private:
	CAutoBranchIterator(const CAutoBranchIterator&) = delete;
	CAutoBranchIterator& operator=(const CAutoBranchIterator&) = delete;

protected:
	virtual void FreeRef()
	{
		git_branch_iterator_free(m_Ref);
	}
};

class CAutoReferenceIterator : public CSmartLibgit2Ref<git_reference_iterator>
{
public:
	CAutoReferenceIterator() {};

	~CAutoReferenceIterator()
	{
		CleanUp();
	}

private:
	CAutoReferenceIterator(const CAutoReferenceIterator&) = delete;
	CAutoReferenceIterator& operator=(const CAutoReferenceIterator&) = delete;

protected:
	virtual void FreeRef()
	{
		git_reference_iterator_free(m_Ref);
	}
};

class CAutoDescribeResult : public CSmartLibgit2Ref<git_describe_result>
{
public:
	CAutoDescribeResult() {};

	~CAutoDescribeResult()
	{
		CleanUp();
	}

private:
	CAutoDescribeResult(const CAutoDescribeResult&) = delete;
	CAutoDescribeResult& operator=(const CAutoDescribeResult&) = delete;

protected:
	virtual void FreeRef()
	{
		git_describe_result_free(m_Ref);
	}
};

class CAutoStatusList : public CSmartLibgit2Ref<git_status_list>
{
public:
	CAutoStatusList() {};

	~CAutoStatusList()
	{
		CleanUp();
	}

private:
	CAutoStatusList(const CAutoStatusList&) = delete;
	CAutoStatusList& operator=(const CAutoStatusList&) = delete;

protected:
	virtual void FreeRef()
	{
		git_status_list_free(m_Ref);
	}
};
