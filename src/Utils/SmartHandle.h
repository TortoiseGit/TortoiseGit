﻿// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2011, 2015 - TortoiseSVN
// Copyright (C) 2015-2016, 2019 - TortoiseGit

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

template <typename type>
struct CDefaultHandleNull
{
	static constexpr type DefaultHandle()
	{
		return nullptr;
	}
};

struct CDefaultHandleInvalid
{
	static constexpr HANDLE DefaultHandle()
	{
		return INVALID_HANDLE_VALUE;
	}
};

/**
 * \ingroup Utils
 * Helper classes for handles.
 */
template <typename HandleType,
	template <class> class CloseFunction,
	typename NullType = CDefaultHandleNull<HandleType>>
class CSmartHandle
{
public:
	CSmartHandle()
		: m_Handle(NullType::DefaultHandle())
	{
	}

	CSmartHandle(HandleType&& h)
		: m_Handle(h)
	{
	}

	CSmartHandle(CSmartHandle&& h)
		: m_Handle(h.Detach())
	{
	}

	CSmartHandle& operator=(CSmartHandle&& h)
	{
		*this = h.Detach();
		return *this;
	}

	HandleType& operator=(HandleType&& h)
	{
		if (m_Handle != h)
		{
			CleanUp();
			m_Handle = h;
		}

		return m_Handle;
	}

	bool CloseHandle()
	{
		return CleanUp();
	}

	HandleType Detach()
	{
		HandleType p = m_Handle;
		m_Handle = NullType::DefaultHandle();

		return p;
	}

	operator HandleType() const
	{
		return m_Handle;
	}

	HandleType * GetPointer()
	{
		return &m_Handle;
	}

	operator bool() const
	{
		return IsValid();
	}

	bool IsValid() const
	{
		return m_Handle != NullType::DefaultHandle();
	}


	~CSmartHandle()
	{
		CleanUp();
	}

private:
	CSmartHandle(const HandleType&) = delete;
	CSmartHandle(const CSmartHandle&) = delete;
	HandleType& operator=(const HandleType&) = delete;
	CSmartHandle& operator=(const CSmartHandle&) = delete;

protected:
	bool CleanUp()
	{
		if (m_Handle != NullType::DefaultHandle())
		{
			const bool b = CloseFunction<HandleType>::Close(m_Handle);
			m_Handle = NullType::DefaultHandle();
			return b;
		}
		return false;
	}


	HandleType m_Handle;
};

template <typename T>
struct CCloseHandle
{
	static bool Close(T handle)
	{
		return !!::CloseHandle(handle);
	}
};



template <typename T>
struct CCloseRegKey
{
	static bool Close(T handle)
	{
		return RegCloseKey(handle) == ERROR_SUCCESS;
	}
};


template <typename T>
struct CCloseLibrary
{
	static bool Close(T handle)
	{
		return !!::FreeLibrary(handle);
	}
};


template <typename T>
struct CCloseViewOfFile
{
	static bool Close(T handle)
	{
		return !!::UnmapViewOfFile(handle);
	}
};

template <typename T>
struct CCloseFindFile
{
	static bool Close(T handle)
	{
		return !!::FindClose(handle);
	}
};

template <typename T>
struct CCloseFILE
{
	static bool Close(T handle)
	{
		return !!fclose(handle);
	}
};


// Client code (definitions of standard Windows handles).
typedef CSmartHandle<HANDLE,	CCloseHandle>											CAutoGeneralHandle;
typedef CSmartHandle<HKEY,		CCloseRegKey>											CAutoRegKey;
typedef CSmartHandle<PVOID,		CCloseViewOfFile>										CAutoViewOfFile;
typedef CSmartHandle<HMODULE,	CCloseLibrary>											CAutoLibrary;
typedef CSmartHandle<HANDLE,	CCloseHandle, CDefaultHandleInvalid>					CAutoFile;
typedef CSmartHandle<HANDLE,	CCloseFindFile, CDefaultHandleInvalid>					CAutoFindFile;
typedef CSmartHandle<FILE*,		CCloseFILE>												CAutoFILE;

/*
void CompilerTests()
{
	// compiler tests
	{
		HANDLE h = reinterpret_cast<HANDLE>(1);
		CAutoFile hFile = h;                    // C2280
		CAutoFile hFile2 = std::move(h);        // OK
		// OK, uses move semantics
		CAutoFile hFile3 = CreateFile(L"c:\\test.txt", GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
		CAutoFile hFile4 = hFile3;              // C2280
		CAutoFile hFile5 = std::move(hFile3);   // OK
	}
}
*/
