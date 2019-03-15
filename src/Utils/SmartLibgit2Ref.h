// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2014-2019 - TortoiseGit
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
template <typename HandleType, void FreeFunction(HandleType*)>
class CSmartBuffer
{
public:
	CSmartBuffer()
		: m_Ref({0})
	{
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
		FreeFunction(&m_Ref);
	}

private:
	CSmartBuffer(const CSmartBuffer&) = delete;
	CSmartBuffer& operator=(const CSmartBuffer&) = delete;

	HandleType m_Ref;
};

typedef CSmartBuffer<git_buf,			git_buf_dispose>					CAutoBuf;
typedef CSmartBuffer<git_strarray,		git_strarray_free>					CAutoStrArray;

template <typename ReferenceType, void FreeFunction(ReferenceType*)>
class CSmartLibgit2Ref
{
public:
	CSmartLibgit2Ref()
		: m_Ref(nullptr)
	{
	}

	~CSmartLibgit2Ref()
	{
		Free();
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
		FreeFunction(m_Ref);
		m_Ref = nullptr;
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
		Free();
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
	ReferenceType* m_Ref;
};

typedef CSmartLibgit2Ref<git_object,				git_object_free>				CAutoObject;
typedef CSmartLibgit2Ref<git_submodule,				git_submodule_free>				CAutoSubmodule;
typedef CSmartLibgit2Ref<git_blob,					git_blob_free>					CAutoBlob;
typedef CSmartLibgit2Ref<git_reference,				git_reference_free>				CAutoReference;
typedef CSmartLibgit2Ref<git_tag,					git_tag_free>					CAutoTag;
typedef CSmartLibgit2Ref<git_tree_entry,			git_tree_entry_free>			CAutoTreeEntry;
typedef CSmartLibgit2Ref<git_diff,					git_diff_free>					CAutoDiff;
typedef CSmartLibgit2Ref<git_patch,					git_patch_free>					CAutoPatch;
typedef CSmartLibgit2Ref<git_diff_stats,			git_diff_stats_free>			CAutoDiffStats;
typedef CSmartLibgit2Ref<git_index,					git_index_free>					CAutoIndex;
typedef CSmartLibgit2Ref<git_remote,				git_remote_free>				CAutoRemote;
typedef CSmartLibgit2Ref<git_reflog,				git_reflog_free>				CAutoReflog;
typedef CSmartLibgit2Ref<git_revwalk,				git_revwalk_free>				CAutoRevwalk;
typedef CSmartLibgit2Ref<git_branch_iterator,		git_branch_iterator_free>		CAutoBranchIterator;
typedef CSmartLibgit2Ref<git_reference_iterator,	git_reference_iterator_free>	CAutoReferenceIterator;
typedef CSmartLibgit2Ref<git_describe_result,		git_describe_result_free>		CAutoDescribeResult;
typedef CSmartLibgit2Ref<git_status_list,			git_status_list_free>			CAutoStatusList;
typedef CSmartLibgit2Ref<git_note,					git_note_free>					CAutoNote;
typedef CSmartLibgit2Ref<git_signature,				git_signature_free>				CAutoSignature;

class CAutoRepository : public CSmartLibgit2Ref<git_repository, git_repository_free>
{
public:
	CAutoRepository() {};

	explicit CAutoRepository(git_repository*&& h)
	{
		m_Ref = h;
	}

	CAutoRepository(CAutoRepository&& that)
	{
		m_Ref = that.Detach();
	}

#if defined(_MFC_VER) || defined(CSTRING_AVAILABLE)
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
#endif

private:
	CAutoRepository(const CAutoRepository&) = delete;
	CAutoRepository& operator=(const CAutoRepository&) = delete;

protected:
	int Open(const char* gitDirA)
	{
		return git_repository_open(GetPointer(), gitDirA);
	}
};

class CAutoCommit : public CSmartLibgit2Ref<git_commit, git_commit_free>
{
public:
	CAutoCommit() {}

	explicit CAutoCommit(git_commit*&& h)
	{
		m_Ref = h;
	}

private:
	CAutoCommit(const CAutoCommit&) = delete;
	CAutoCommit& operator=(const CAutoCommit&) = delete;
};

class CAutoTree : public CSmartLibgit2Ref<git_tree, git_tree_free>
{
public:
	CAutoTree() {};

	void ConvertFrom(CAutoObject&& h)
	{
		if (m_Ref != reinterpret_cast<git_tree*>(static_cast<git_object*>(h)))
		{
			Free();
			m_Ref = reinterpret_cast<git_tree*>(h.Detach());
		}
	}

private:
	CAutoTree(const CAutoTree&) = delete;
	CAutoTree& operator=(const CAutoTree&) = delete;
};

class CAutoConfig : public CSmartLibgit2Ref<git_config, git_config_free>
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
		Free();
		git_config_new(&m_Ref);
	}

#if defined(_MFC_VER) || defined(CSTRING_AVAILABLE)
	int GetString(const CString &key, CString &value) const
	{
		if (!IsValid())
			return -1;

		CAutoBuf buf;
		if (auto ret = git_config_get_string_buf(buf, m_Ref, CUnicodeUtils::GetUTF8(key)); ret)
		{
			value.Empty();
			return ret;
		}

		value = CUnicodeUtils::GetUnicode(CStringA(buf->ptr));

		return 0;
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
		if (auto ret = GetBOOL(key, value); ret < 0)
			return ret;

		b = (value == TRUE);

		return 0;
	}
#endif

private:
	CAutoConfig(const CAutoConfig&) = delete;
	CAutoConfig& operator=(const CAutoConfig&) = delete;
};
