// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2011, 2015 - TortoiseSVN
// Copyright (C) 2015 - TortoiseGit

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
 * Helper classes for handles.
 */
template <typename HandleType,
	template <class> class CloseFunction,
	HandleType NULL_VALUE = NULL>
class CSmartHandle
{
public:
	CSmartHandle()
	{
		m_Handle = NULL_VALUE;
	}

	CSmartHandle(HandleType h)
	{
		m_Handle = h;
	}

	HandleType operator=(HandleType h)
	{
		if (m_Handle != h)
		{
			CleanUp();
			m_Handle = h;
		}

		return(*this);
	}

	bool CloseHandle()
	{
		return CleanUp();
	}

	HandleType Detach()
	{
		HandleType p = m_Handle;
		m_Handle = NULL_VALUE;

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
		return m_Handle != NULL_VALUE;
	}


	~CSmartHandle()
	{
		CleanUp();
	}

private:
	CSmartHandle(const CSmartHandle&) = delete;
	CSmartHandle& operator=(const CSmartHandle&) = delete;

protected:
	bool CleanUp()
	{
		if ( m_Handle != NULL_VALUE )
		{
			const bool b = CloseFunction<HandleType>::Close(m_Handle);
			m_Handle = NULL_VALUE;
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
typedef CSmartHandle<HANDLE,	CCloseHandle, INVALID_HANDLE_VALUE>						CAutoFile;
typedef CSmartHandle<HANDLE,	CCloseFindFile, INVALID_HANDLE_VALUE>					CAutoFindFile;
typedef CSmartHandle<FILE*,		CCloseFILE>												CAutoFILE;
