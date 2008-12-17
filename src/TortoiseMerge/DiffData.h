// TortoiseMerge - a Diff/Patch program

// Copyright (C) 2006-2008 - TortoiseSVN

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

#include "svn_diff.h"
#include "apr_pools.h"
#include "FileTextLines.h"
#include "Registry.h"
#include "WorkingFile.h"
#include "ViewData.h"



#define DIFF_EMPTYLINENUMBER						((DWORD)-1)
/**
 * \ingroup TortoiseMerge
 * Main class for handling diffs.
 */
class CDiffData
{
public:
	CDiffData(void);
	virtual ~CDiffData(void);


	BOOL						Load();
	void						SetBlame(bool bBlame = true) {m_bBlame = bBlame;}
	int							GetLineCount();
	int							GetLineActualLength(int index);
	LPCTSTR						GetLineChars(int index);
	CString						GetError() const  {return m_sError;}

	bool	IsBaseFileInUse() const		{ return m_baseFile.InUse(); }
	bool	IsTheirFileInUse() const	{ return m_theirFile.InUse(); }
	bool	IsYourFileInUse() const		{ return m_yourFile.InUse(); }

private:
	bool DoTwoWayDiff(const CString& sBaseFilename, const CString& sYourFilename, DWORD dwIgnoreWS, bool bIgnoreEOL, apr_pool_t * pool);
	bool DoThreeWayDiff(const CString& sBaseFilename, const CString& sYourFilename, const CString& sTheirFilename, DWORD dwIgnoreWS, bool bIgnoreEOL, bool bIgnoreCase,apr_pool_t * pool);


public:
	CWorkingFile				m_baseFile;
	CWorkingFile				m_theirFile;
	CWorkingFile				m_yourFile;
	CWorkingFile				m_mergedFile;

	CString						m_sDiffFile;
	CString						m_sPatchPath;
	CString						m_sPatchOriginal;
	CString						m_sPatchPatched;

public:
	CFileTextLines				m_arBaseFile;
	CFileTextLines				m_arTheirFile;
	CFileTextLines				m_arYourFile;

	CViewData					m_YourBaseBoth;				///< one-pane view, diff between 'yours' and 'base' (in three-pane view: right view)
	CViewData					m_YourBaseLeft;				///< two-pane view, diff between 'yours' and 'base', left view
	CViewData					m_YourBaseRight;			///< two-pane view, diff between 'yours' and 'base', right view

	CViewData					m_TheirBaseBoth;			///< one-pane view, diff between 'theirs' and 'base' (in three-pane view: left view)
	CViewData					m_TheirBaseLeft;			///< two-pane view, diff between 'theirs' and 'base', left view
	CViewData					m_TheirBaseRight;			///< two-pane view, diff between 'theirs' and 'base', right view

	CViewData					m_Diff3;					///< thee-pane view, bottom pane

	// the following three arrays are used to check for conflicts even in case the
	// user has ignored spaces/eols.
	CStdDWORDArray				m_arDiff3LinesBase;
	CStdDWORDArray				m_arDiff3LinesYour;
	CStdDWORDArray				m_arDiff3LinesTheir;

	CString						m_sError;

	static int					abort_on_pool_failure (int retcode);
protected:
	bool						m_bBlame;
};
