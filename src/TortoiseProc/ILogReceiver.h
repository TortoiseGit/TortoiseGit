// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2007-2007 - TortoiseSVN

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

///////////////////////////////////////////////////////////////
// temporarily used to disambiguate LogChangedPath definitions
///////////////////////////////////////////////////////////////

#ifndef __ILOGRECEIVER_H__
#define __ILOGRECEIVER_H__
#endif

///////////////////////////////////////////////////////////////
// required includes
///////////////////////////////////////////////////////////////

//#include "svn_types.h"


/**
 * data structure to accommodate the change list.
 */
struct LogChangedPath
{
	CString sPath;
//	CString sCopyFromPath;
	CString lCopyFromRev;
	DWORD action;

	/// returns the action as a string
	const CString& GetAction() const;

private:

	/// cached return value of GetAction()
	mutable CString actionAsString;
};



/// auto-deleting extension of MFC Arrays for pointer arrays

template<class T>
class CAutoArray : public CArray<T*,T*>
{
public:

	// default and copy construction

	CAutoArray() 
	{
	}

	CAutoArray (const CAutoArray& rhs)
	{
		Copy (rhs);
	}

	// destruction deletes members

	~CAutoArray()
	{
		for (INT_PTR i = 0, count = GetCount(); i < count; ++i)
			delete GetAt (i);
	}
};

typedef CAutoArray<LogChangedPath> LogChangedPathArray;

/**
 * standard revision properties
 */

struct StandardRevProps
{
	CString author;
//	apr_time_t timeStamp;
	CString message;
};

/**
 * data structure to accommodate the list of user-defined revision properties.
 */
struct UserRevProp
{
	CString name;
	CString value;
};

typedef CAutoArray<UserRevProp> UserRevPropArray;


/**
 * Interface for receiving log information. It will be used as a callback
 * in ILogQuery::Log().
 *
 * To cancel the log and/or indicate errors, throw an SVNError exception.
 */
class ILogReceiver
{
public:

	/// call-back for every revision found
	/// (called at most once per revision)
	///
	/// the implementation may modify but not delete()
	/// the data containers passed to it
	///
	/// any pointer may be NULL
	///
	/// may throw a SVNError to cancel the log

	virtual void ReceiveLog ( LogChangedPathArray* changes
							, CString rev
							, const StandardRevProps* stdRevProps
							, UserRevPropArray* userRevProps
							, bool mergesFollow) = 0;
};
