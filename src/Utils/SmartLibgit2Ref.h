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

/**
 * \ingroup Utils
 * Helper classes for libgit2 references.
 */
template <typename ReferenceType, template <class> class FreeFunction>
class CSmartLibgit2Ref : public FreeFunction<ReferenceType>
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

		return(*this);
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


	~CSmartLibgit2Ref()
	{
		CleanUp();
	}


protected:
	void CleanUp()
	{
		if (m_Ref != nullptr)
		{
			FreeRef(m_Ref);
			m_Ref = nullptr;
		}
	}


	ReferenceType* m_Ref;
};

template <typename T>
struct CFreeRepository
{
	void FreeRef(T* ref)
	{
		git_repository_free(ref);
	}

protected:
	~CFreeRepository()
	{
	}
};

template <typename T>
struct CFreeCommit
{
	void FreeRef(T* ref)
	{
		git_commit_free(ref);
	}

protected:
	~CFreeCommit()
	{
	}
};

template <typename T>
struct CFreeTree
{
	void FreeRef(T* ref)
	{
		git_tree_free(ref);
	}

protected:
	~CFreeTree()
	{
	}
};

template <typename T>
struct CFreeConfig
{
	void FreeRef(T* ref)
	{
		git_config_free(ref);
	}

protected:
	~CFreeConfig()
	{
	}
};

template <typename T>
struct CFreeObject
{
	void FreeRef(T* ref)
	{
		git_object_free(ref);
	}

protected:
	~CFreeObject()
	{
	}
};

template <typename T>
struct CFreeBlob
{
	void FreeRef(T* ref)
	{
		git_blob_free(ref);
	}

protected:
	~CFreeBlob()
	{
	}
};


// Client code (definitions of standard Windows handles).
typedef CSmartLibgit2Ref<git_repository,		CFreeRepository>						CAutoRepository;
typedef CSmartLibgit2Ref<git_commit,			CFreeCommit>							CAutoCommit;
typedef CSmartLibgit2Ref<git_tree,				CFreeTree>								CAutoTree;
typedef CSmartLibgit2Ref<git_object,			CFreeObject>							CAutoObject;
typedef CSmartLibgit2Ref<git_blob,				CFreeBlob>								CAutoBlob;
typedef CSmartLibgit2Ref<git_config,			CFreeConfig>							CAutoConfig;
