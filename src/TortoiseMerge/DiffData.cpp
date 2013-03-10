// TortoiseGitMerge - a Diff/Patch program

// Copyright (C) 2006-2013 - TortoiseSVN

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
#include "stdafx.h"
#pragma warning(push)
#include "diff.h"
#pragma warning(pop)
#include "TempFile.h"
#include "registry.h"
#include "resource.h"
#include "Diffdata.h"
#include "UnicodeUtils.h"
#include "svn_dso.h"
#include "MovedBlocks.h"

#pragma warning(push)
#pragma warning(disable: 4702) // unreachable code
int CDiffData::abort_on_pool_failure (int /*retcode*/)
{
	abort ();
	return -1;
}
#pragma warning(pop)

CDiffData::CDiffData(void)
	: m_bViewMovedBlocks(false)
	, m_bPatchRequired(false)
{
	apr_initialize();
	svn_dso_initialize2();

	m_bBlame = false;

	m_sPatchOriginal = _T(": original");
	m_sPatchPatched = _T(": patched");
}

CDiffData::~CDiffData(void)
{
	apr_terminate();
}

void CDiffData::SetMovedBlocks(bool bViewMovedBlocks/* = true*/)
{
	m_bViewMovedBlocks = bViewMovedBlocks;
}

int CDiffData::GetLineCount() const
{
	int count = (int)m_arBaseFile.GetCount();
	if (count < m_arTheirFile.GetCount())
		count = m_arTheirFile.GetCount();
	if (count < m_arYourFile.GetCount())
		count = m_arYourFile.GetCount();
	return count;
}

svn_diff_file_ignore_space_t CDiffData::GetIgnoreSpaceMode(DWORD dwIgnoreWS)
{
	switch (dwIgnoreWS)
	{
	case 0:
		return svn_diff_file_ignore_space_none;
	case 1:
		return svn_diff_file_ignore_space_all;
	case 2:
		return svn_diff_file_ignore_space_change;
	default:
		return svn_diff_file_ignore_space_none;
	}
}

svn_diff_file_options_t * CDiffData::CreateDiffFileOptions(DWORD dwIgnoreWS, bool bIgnoreEOL, apr_pool_t * pool)
{
	svn_diff_file_options_t * options = svn_diff_file_options_create(pool);
	options->ignore_eol_style = bIgnoreEOL;
	options->ignore_space = GetIgnoreSpaceMode(dwIgnoreWS);
	return options;
}

bool CDiffData::HandleSvnError(svn_error_t * svnerr)
{
	TRACE(_T("diff-error in CDiffData::Load()\n"));
	TRACE(_T("diff-error in CDiffData::Load()\n"));
	CStringA sMsg = CStringA(svnerr->message);
	while (svnerr->child)
	{
		svnerr = svnerr->child;
		sMsg += _T("\n");
		sMsg += CStringA(svnerr->message);
	}
	CString readableMsg = CUnicodeUtils::GetUnicode(sMsg);
	m_sError.Format(IDS_ERR_DIFF_DIFF, (LPCTSTR)readableMsg);
	svn_error_clear(svnerr);
	return false;
}

void CDiffData::TieMovedBlocks(int from, int to, apr_off_t length)
{
	for(int i=0; i<length; i++, to++, from++)
	{
		int fromIndex = m_YourBaseLeft.FindLineNumber(from);
		int toIndex = m_YourBaseRight.FindLineNumber(to);
		m_YourBaseLeft.SetState(fromIndex, DIFFSTATE_MOVED_FROM);
		m_YourBaseLeft.SetMovedIndex(fromIndex, toIndex);
		m_YourBaseRight.SetState(toIndex, DIFFSTATE_MOVED_TO);
		m_YourBaseRight.SetMovedIndex(toIndex, fromIndex);

		toIndex = m_YourBaseBoth.FindLineNumber(to);
		while((toIndex < m_YourBaseBoth.GetCount())&&
			  ((m_YourBaseBoth.GetState(toIndex) != DIFFSTATE_ADDED)&&
			  (m_YourBaseBoth.GetState(toIndex) != DIFFSTATE_MOVED_TO)||
			  (m_YourBaseBoth.GetLineNumber(toIndex) != to)))
		{
			toIndex++;
		}

		fromIndex = m_YourBaseBoth.FindLineNumber(from);
		while((fromIndex < m_YourBaseBoth.GetCount())&&
			  ((m_YourBaseBoth.GetState(fromIndex) != DIFFSTATE_REMOVED)&&
			  (m_YourBaseBoth.GetState(fromIndex) != DIFFSTATE_MOVED_FROM)||
			  (m_YourBaseBoth.GetLineNumber(fromIndex) != from)))
		{
			fromIndex++;
		}
		if ((fromIndex < m_YourBaseBoth.GetCount())&&(toIndex < m_YourBaseBoth.GetCount()))
		{
			m_YourBaseBoth.SetState(fromIndex, DIFFSTATE_MOVED_FROM);
			m_YourBaseBoth.SetMovedIndex(fromIndex, toIndex);
			m_YourBaseBoth.SetState(toIndex, DIFFSTATE_MOVED_TO);
			m_YourBaseBoth.SetMovedIndex(toIndex, fromIndex);
		}
	}
}

bool CDiffData::CompareWithIgnoreWS(CString s1, CString s2, DWORD dwIgnoreWS)
{
	if (dwIgnoreWS == 2)
	{
		s1 = s1.TrimLeft(_T(" \t"));
		s2 = s2.TrimLeft(_T(" \t"));
	}
	else
	{
		s1 = s1.TrimRight(_T(" \t"));
		s2 = s2.TrimRight(_T(" \t"));
	}

	return s1 != s2;
}

void CDiffData::StickAndSkip(svn_diff_t * &tempdiff, apr_off_t &original_length_sticked, apr_off_t &modified_length_sticked)
{
	if((m_bViewMovedBlocks)&&(tempdiff->type == svn_diff__type_diff_modified))
	{
		svn_diff_t * nextdiff = tempdiff->next;
		while((nextdiff)&&(nextdiff->type == svn_diff__type_diff_modified))
		{
			original_length_sticked += nextdiff->original_length;
			modified_length_sticked += nextdiff->modified_length;
			tempdiff = nextdiff;
			nextdiff = tempdiff->next;
		}
	}
}

BOOL CDiffData::Load()
{
	m_arBaseFile.RemoveAll();
	m_arYourFile.RemoveAll();
	m_arTheirFile.RemoveAll();

	m_YourBaseBoth.Clear();
	m_YourBaseLeft.Clear();
	m_YourBaseRight.Clear();

	m_TheirBaseBoth.Clear();
	m_TheirBaseLeft.Clear();
	m_TheirBaseRight.Clear();

	m_Diff3.Clear();

	CRegDWORD regIgnoreWS = CRegDWORD(_T("Software\\TortoiseGitMerge\\IgnoreWS"));
	CRegDWORD regIgnoreEOL = CRegDWORD(_T("Software\\TortoiseGitMerge\\IgnoreEOL"), TRUE);
	CRegDWORD regIgnoreCase = CRegDWORD(_T("Software\\TortoiseGitMerge\\CaseInsensitive"), FALSE);
	DWORD dwIgnoreWS = regIgnoreWS;
	bool bIgnoreEOL = ((DWORD)regIgnoreEOL)!=0;
	BOOL bIgnoreCase = ((DWORD)regIgnoreCase)!=0;

	// The Subversion diff API only can ignore whitespaces and eol styles.
	// It also can only handle one-byte charsets.
	// To ignore case changes or to handle UTF-16 files, we have to
	// save the original file in UTF-8 and/or the letters changed to lowercase
	// so the Subversion diff can handle those.
	CString sConvertedBaseFilename = m_baseFile.GetFilename();
	CString sConvertedYourFilename = m_yourFile.GetFilename();
	CString sConvertedTheirFilename = m_theirFile.GetFilename();

	m_baseFile.StoreFileAttributes();
	m_theirFile.StoreFileAttributes();
	m_yourFile.StoreFileAttributes();
	//m_mergedFile.StoreFileAttributes();

	bool bBaseNeedConvert  = false;
	bool bTheirNeedConvert = false;
	bool bYourNeedConvert  = false;
	bool bBaseIsUtf8  = false;
	bool bTheirIsUtf8 = false;
	bool bYourIsUtf8  = false;

	// in case at least one of the files got converted or is UTF8
	// we have to convert all non UTF8 (ASCII) files
	// otherwise one file might be in ANSI and the other in UTF8 and we'll end up
	// with lines marked as different throughout the files even though the lines
	// would show no change at all in the viewer.
	bool bIsNotUtf8 = false; // Any non UTF8 file ?

	if (IsBaseFileInUse())
	{
		if (!m_arBaseFile.Load(m_baseFile.GetFilename()))
		{
			m_sError = m_arBaseFile.GetErrorString();
			return FALSE;
		}
		bBaseNeedConvert = bIgnoreCase || (m_arBaseFile.NeedsConversion());
		bBaseIsUtf8 = (m_arBaseFile.GetUnicodeType()!=CFileTextLines::ASCII) || bBaseNeedConvert;
		bIsNotUtf8 |= !bBaseIsUtf8;
	}

	if (IsTheirFileInUse())
	{
		// m_arBaseFile.GetCount() is passed as a hint for the number of lines in this file
		// It's a fair guess that the files will be roughly the same size
		if (!m_arTheirFile.Load(m_theirFile.GetFilename(), m_arBaseFile.GetCount()))
		{
			m_sError = m_arTheirFile.GetErrorString();
			return FALSE;
		}
		bTheirNeedConvert = bIgnoreCase || (m_arTheirFile.NeedsConversion());
		bTheirIsUtf8 = (m_arTheirFile.GetUnicodeType()!=CFileTextLines::ASCII) || bTheirNeedConvert;
		bIsNotUtf8 |= !bTheirIsUtf8;
	}

	if (IsYourFileInUse())
	{
		// m_arBaseFile.GetCount() is passed as a hint for the number of lines in this file
		// It's a fair guess that the files will be roughly the same size
		if (!m_arYourFile.Load(m_yourFile.GetFilename(), m_arBaseFile.GetCount()))
		{
			m_sError = m_arYourFile.GetErrorString();
			return FALSE;
		}
		bYourNeedConvert = bIgnoreCase || (m_arYourFile.NeedsConversion());
		bYourIsUtf8 = (m_arYourFile.GetUnicodeType()!=CFileTextLines::ASCII) || bYourNeedConvert;
		bIsNotUtf8 |= !bYourIsUtf8;
	}

	// convert all files we need to
	bool bIsUtf8 = bBaseIsUtf8 || bTheirIsUtf8 || bYourIsUtf8; // any file end as UTF8
	bBaseNeedConvert |= (IsBaseFileInUse() && !bBaseIsUtf8 && bIsUtf8);
	if (bBaseNeedConvert)
	{
		sConvertedBaseFilename = CTempFiles::Instance().GetTempFilePathString();
		m_baseFile.SetConvertedFileName(sConvertedBaseFilename);
		m_arBaseFile.Save(sConvertedBaseFilename, true, true, 0, bIgnoreCase, m_bBlame);
	}
	bYourNeedConvert |= (IsYourFileInUse() && !bYourIsUtf8 && bIsUtf8);
	if (bYourNeedConvert)
	{
		sConvertedYourFilename = CTempFiles::Instance().GetTempFilePathString();
		m_yourFile.SetConvertedFileName(sConvertedYourFilename);
		m_arYourFile.Save(sConvertedYourFilename, true, true, 0, bIgnoreCase, m_bBlame);
	}
	bTheirNeedConvert |= (IsTheirFileInUse() && !bTheirIsUtf8 && bIsUtf8);
	if (bTheirNeedConvert)
	{
		sConvertedTheirFilename = CTempFiles::Instance().GetTempFilePathString();
		m_theirFile.SetConvertedFileName(sConvertedTheirFilename);
		m_arTheirFile.Save(sConvertedTheirFilename, true, true, 0, bIgnoreCase, m_bBlame);
	}

	// Calculate the number of lines in the largest of the three files
	int lengthHint = GetLineCount();

	try
	{
		m_YourBaseBoth.Reserve(lengthHint);
		m_YourBaseLeft.Reserve(lengthHint);
		m_YourBaseRight.Reserve(lengthHint);

		m_TheirBaseBoth.Reserve(lengthHint);
		m_TheirBaseLeft.Reserve(lengthHint);
		m_TheirBaseRight.Reserve(lengthHint);
	}
	catch (CMemoryException* e)
	{
		e->GetErrorMessage(m_sError.GetBuffer(255), 255);
		m_sError.ReleaseBuffer();
		e->Delete();
		return FALSE;
	}

	apr_pool_t * pool = NULL;
	apr_pool_create_ex (&pool, NULL, abort_on_pool_failure, NULL);

	// Is this a two-way diff?
	if (IsBaseFileInUse() && IsYourFileInUse() && !IsTheirFileInUse())
	{
		if (!DoTwoWayDiff(sConvertedBaseFilename, sConvertedYourFilename, dwIgnoreWS, bIgnoreEOL, pool))
		{
			apr_pool_destroy (pool);                    // free the allocated memory
			return FALSE;
		}
	}

	if (IsBaseFileInUse() && IsTheirFileInUse() && !IsYourFileInUse())
	{
		ASSERT(FALSE);
	}

	// Is this a three-way diff?
	if (IsBaseFileInUse() && IsTheirFileInUse() && IsYourFileInUse())
	{
		m_Diff3.Reserve(lengthHint);

		if (!DoThreeWayDiff(sConvertedBaseFilename, sConvertedYourFilename, sConvertedTheirFilename, dwIgnoreWS, bIgnoreEOL, !!bIgnoreCase, pool))
		{
			apr_pool_destroy (pool);                    // free the allocated memory
			return FALSE;
		}
	}

	// free the allocated memory
	apr_pool_destroy (pool);

	m_arBaseFile.RemoveAll();
	m_arYourFile.RemoveAll();
	m_arTheirFile.RemoveAll();

	return TRUE;
}

bool
CDiffData::DoTwoWayDiff(const CString& sBaseFilename, const CString& sYourFilename, DWORD dwIgnoreWS, bool bIgnoreEOL, apr_pool_t * pool)
{
	svn_diff_file_options_t * options = CreateDiffFileOptions(dwIgnoreWS, bIgnoreEOL, pool);
	// convert CString filenames (UTF-16 or ANSI) to UTF-8
	CStringA sBaseFilenameUtf8 = CUnicodeUtils::GetUTF8(sBaseFilename);
	CStringA sYourFilenameUtf8 = CUnicodeUtils::GetUTF8(sYourFilename);

	svn_diff_t * diffYourBase = NULL;
	svn_error_t * svnerr = svn_diff_file_diff_2(&diffYourBase, sBaseFilenameUtf8, sYourFilenameUtf8, options, pool);

	if (svnerr)
		return HandleSvnError(svnerr);

	tsvn_svn_diff_t_extension * movedBlocks = NULL;
	if(m_bViewMovedBlocks)
		movedBlocks = MovedBlocksDetect(diffYourBase, dwIgnoreWS, pool); // Side effect is that diffs are now splitted

	svn_diff_t * tempdiff = diffYourBase;
	LONG baseline = 0;
	LONG yourline = 0;
	while (tempdiff)
	{
		svn_diff__type_e diffType = tempdiff->type;
		// Side effect described above overcoming - sticking together
		apr_off_t original_length_sticked = tempdiff->original_length;
		apr_off_t modified_length_sticked = tempdiff->modified_length;
		StickAndSkip(tempdiff, original_length_sticked, modified_length_sticked);

		for (int i=0; i<original_length_sticked; i++)
		{
			if (baseline >= m_arBaseFile.GetCount())
			{
				m_sError.LoadString(IDS_ERR_DIFF_NEWLINES);
				return false;
			}
			const CString& sCurrentBaseLine = m_arBaseFile.GetAt(baseline);
			EOL endingBase = m_arBaseFile.GetLineEnding(baseline);
			if (diffType == svn_diff__type_common)
			{
				if (yourline >= m_arYourFile.GetCount())
				{
					m_sError.LoadString(IDS_ERR_DIFF_NEWLINES);
					return false;
				}
				const CString& sCurrentYourLine = m_arYourFile.GetAt(yourline);
				EOL endingYours = m_arYourFile.GetLineEnding(yourline);
				if (sCurrentBaseLine != sCurrentYourLine)
				{
					bool changedWS = false;
					if (dwIgnoreWS == 2 || dwIgnoreWS == 3)
						changedWS = CompareWithIgnoreWS(sCurrentBaseLine, sCurrentYourLine, dwIgnoreWS);
					if (changedWS || dwIgnoreWS == 0)
					{
						// one-pane view: two lines, one 'removed' and one 'added'
						m_YourBaseBoth.AddData(sCurrentBaseLine, DIFFSTATE_REMOVEDWHITESPACE, yourline, endingBase, HIDESTATE_SHOWN, -1);
						m_YourBaseBoth.AddData(sCurrentYourLine, DIFFSTATE_ADDEDWHITESPACE, yourline, endingYours, HIDESTATE_SHOWN, -1);
					}
					else
					{
						m_YourBaseBoth.AddData(sCurrentYourLine, DIFFSTATE_NORMAL, yourline, endingBase, HIDESTATE_HIDDEN, -1);
					}
				}
				else
				{
					m_YourBaseBoth.AddData(sCurrentYourLine, DIFFSTATE_NORMAL, yourline, endingBase, HIDESTATE_HIDDEN, -1);
				}
				yourline++;     //in both files
			}
			else
			{ // small trick - we need here a baseline, but we fix it back to yourline at the end of routine
				m_YourBaseBoth.AddData(sCurrentBaseLine, DIFFSTATE_REMOVED, baseline, endingBase, HIDESTATE_SHOWN, -1);
			}
			baseline++;
		}
		if (diffType == svn_diff__type_diff_modified)
		{
			for (int i=0; i<modified_length_sticked; i++)
			{
				if (m_arYourFile.GetCount() > yourline)
				{
					m_YourBaseBoth.AddData(m_arYourFile.GetAt(yourline), DIFFSTATE_ADDED, yourline, m_arYourFile.GetLineEnding(yourline), HIDESTATE_SHOWN, -1);
				}
				yourline++;
			}
		}
		tempdiff = tempdiff->next;
	}

	HideUnchangedSections(&m_YourBaseBoth, NULL, NULL);

	tempdiff = diffYourBase;
	baseline = 0;
	yourline = 0;
	while (tempdiff)
	{
		if (tempdiff->type == svn_diff__type_common)
		{
			for (int i=0; i<tempdiff->original_length; i++)
			{
				const CString& sCurrentYourLine = m_arYourFile.GetAt(yourline);
				EOL endingYours = m_arYourFile.GetLineEnding(yourline);
				const CString& sCurrentBaseLine = m_arBaseFile.GetAt(baseline);
				EOL endingBase = m_arBaseFile.GetLineEnding(baseline);
				if (sCurrentBaseLine != sCurrentYourLine)
				{
					bool changedWS = false;
					if (dwIgnoreWS == 2 || dwIgnoreWS == 3)
						changedWS = CompareWithIgnoreWS(sCurrentBaseLine, sCurrentYourLine, dwIgnoreWS);
					if (changedWS || dwIgnoreWS == 0)
					{
						m_YourBaseLeft.AddData(sCurrentBaseLine, DIFFSTATE_WHITESPACE, baseline, endingBase, HIDESTATE_SHOWN, -1);
						m_YourBaseRight.AddData(sCurrentYourLine, DIFFSTATE_WHITESPACE, yourline, endingYours, HIDESTATE_SHOWN, -1);
					}
					else
					{
						m_YourBaseLeft.AddData(sCurrentBaseLine, DIFFSTATE_NORMAL, baseline, endingBase, HIDESTATE_HIDDEN, -1);
						m_YourBaseRight.AddData(sCurrentYourLine, DIFFSTATE_NORMAL, yourline, endingYours, HIDESTATE_HIDDEN, -1);
					}
				}
				else
				{
					m_YourBaseLeft.AddData(sCurrentBaseLine, DIFFSTATE_NORMAL, baseline, endingBase, HIDESTATE_HIDDEN, -1);
					m_YourBaseRight.AddData(sCurrentYourLine, DIFFSTATE_NORMAL, yourline, endingYours, HIDESTATE_HIDDEN, -1);
				}
				baseline++;
				yourline++;
			}
		}
		if (tempdiff->type == svn_diff__type_diff_modified)
		{
			// now we trying to stick together parts, that were splitted by MovedBlocks
			apr_off_t original_length_sticked = tempdiff->original_length;
			apr_off_t modified_length_sticked = tempdiff->modified_length;
			StickAndSkip(tempdiff, original_length_sticked, modified_length_sticked);

			apr_off_t original_length = original_length_sticked;
			for (int i=0; i<modified_length_sticked; i++)
			{
				if (m_arYourFile.GetCount() > yourline)
				{
					EOL endingYours = m_arYourFile.GetLineEnding(yourline);
					m_YourBaseRight.AddData(m_arYourFile.GetAt(yourline), DIFFSTATE_ADDED, yourline, endingYours, HIDESTATE_SHOWN, -1);
					if (original_length-- <= 0)
					{
						m_YourBaseLeft.AddEmpty();
					}
					else
					{
						EOL endingBase = m_arBaseFile.GetLineEnding(baseline);
						m_YourBaseLeft.AddData(m_arBaseFile.GetAt(baseline), DIFFSTATE_REMOVED, baseline, endingBase, HIDESTATE_SHOWN, -1);
						baseline++;
					}
					yourline++;
				}
			}
			apr_off_t modified_length = modified_length_sticked;
			for (int i=0; i<original_length_sticked; i++)
			{
				if ((modified_length-- <= 0)&&(m_arBaseFile.GetCount() > baseline))
				{
					EOL endingBase = m_arBaseFile.GetLineEnding(baseline);
					m_YourBaseLeft.AddData(m_arBaseFile.GetAt(baseline), DIFFSTATE_REMOVED, baseline, endingBase, HIDESTATE_SHOWN, -1);
					m_YourBaseRight.AddEmpty();
					baseline++;
				}
			}
		}
		tempdiff = tempdiff->next;
	}
	// add last (empty) lines if needed - diff don't report those
	if (m_arBaseFile.GetCount() > baseline)
	{
		if (m_arYourFile.GetCount() > yourline)
		{
			// last line is missing in both files add them to end and mark as no diff
			m_YourBaseLeft.AddData(m_arBaseFile.GetAt(baseline), DIFFSTATE_NORMAL, baseline, m_arBaseFile.GetLineEnding(baseline), HIDESTATE_SHOWN, -1);
			m_YourBaseRight.AddData(m_arYourFile.GetAt(yourline), DIFFSTATE_NORMAL, yourline, m_arYourFile.GetLineEnding(yourline), HIDESTATE_SHOWN, -1);
			yourline++;
			baseline++;
		}
		else
		{
			viewdata oViewData(m_arBaseFile.GetAt(baseline), DIFFSTATE_REMOVED, baseline, m_arBaseFile.GetLineEnding(baseline), HIDESTATE_SHOWN, -1);
			baseline++;

			// find first EMPTY line in last blok
			int nPos = m_YourBaseLeft.GetCount();
			while (--nPos>=0 && m_YourBaseLeft.GetState(nPos)==DIFFSTATE_EMPTY) ;
			if (++nPos<m_YourBaseLeft.GetCount())
			{
				m_YourBaseLeft.SetData(nPos, oViewData);
			}
			else
			{
				m_YourBaseLeft.AddData(oViewData);
				m_YourBaseRight.AddEmpty();
			}
		}
	}
	else if (m_arYourFile.GetCount() > yourline)
	{
		viewdata oViewData(m_arYourFile.GetAt(yourline), DIFFSTATE_ADDED, yourline, m_arYourFile.GetLineEnding(yourline), HIDESTATE_SHOWN, -1);
		yourline++;

		// try to move last line higher
		int nPos = m_YourBaseRight.GetCount();
		while (--nPos>=0 && m_YourBaseRight.GetState(nPos)==DIFFSTATE_EMPTY) ;
		if (++nPos<m_YourBaseRight.GetCount())
		{
			m_YourBaseRight.SetData(nPos, oViewData);
		}
		else
		{
			m_YourBaseLeft.AddEmpty();
			m_YourBaseRight.AddData(oViewData);
		}
	}


	// Fixing results for conforming moved blocks

	while(movedBlocks)
	{
		tempdiff = movedBlocks->base;
		if(movedBlocks->moved_to != -1)
		{
			// set states in a block original:length -> moved_to:length
			TieMovedBlocks((int)tempdiff->original_start, movedBlocks->moved_to, tempdiff->original_length);
		}
		if(movedBlocks->moved_from != -1)
		{
			// set states in a block modified:length -> moved_from:length
			TieMovedBlocks(movedBlocks->moved_from, (int)tempdiff->modified_start, tempdiff->modified_length);
		}
		movedBlocks = movedBlocks->next;
	}

	// replace baseline with the yourline in m_YourBaseBoth
	yourline = 0;
	for(int i=0; i<m_YourBaseBoth.GetCount(); i++)
	{
		DiffStates state = m_YourBaseBoth.GetState(i);
		if((state == DIFFSTATE_REMOVED)||(state == DIFFSTATE_MOVED_FROM))
		{
			m_YourBaseBoth.SetLineNumber(i, yourline);
		}
		else
		{
			yourline++;
		}
	}

	TRACE(_T("done with 2-way diff\n"));

	HideUnchangedSections(&m_YourBaseLeft, &m_YourBaseRight, NULL);

	return true;
}

bool
CDiffData::DoThreeWayDiff(const CString& sBaseFilename, const CString& sYourFilename, const CString& sTheirFilename, DWORD dwIgnoreWS, bool bIgnoreEOL, bool bIgnoreCase, apr_pool_t * pool)
{
	// the following three arrays are used to check for conflicts even in case the
	// user has ignored spaces/eols.
	CStdDWORDArray              m_arDiff3LinesBase;
	CStdDWORDArray              m_arDiff3LinesYour;
	CStdDWORDArray              m_arDiff3LinesTheir;
#define AddLines(baseline, yourline, theirline) m_arDiff3LinesBase.Add(baseline), m_arDiff3LinesYour.Add(yourline), m_arDiff3LinesTheir.Add(theirline)
	int lengthHint = GetLineCount();

	m_arDiff3LinesBase.Reserve(lengthHint);
	m_arDiff3LinesYour.Reserve(lengthHint);
	m_arDiff3LinesTheir.Reserve(lengthHint);

	CRegDWORD contextLines = CRegDWORD(_T("Software\\TortoiseGitMerge\\ContextLines"), 3);
	svn_diff_file_options_t * options = CreateDiffFileOptions(dwIgnoreWS, bIgnoreEOL, pool);

	// convert CString filenames (UTF-16 or ANSI) to UTF-8
	CStringA sBaseFilenameUtf8  = CUnicodeUtils::GetUTF8(sBaseFilename);
	CStringA sYourFilenameUtf8  = CUnicodeUtils::GetUTF8(sYourFilename);
	CStringA sTheirFilenameUtf8 = CUnicodeUtils::GetUTF8(sTheirFilename);

	svn_diff_t * diffTheirYourBase = NULL;
	svn_error_t * svnerr = svn_diff_file_diff3_2(&diffTheirYourBase, sBaseFilenameUtf8, sTheirFilenameUtf8, sYourFilenameUtf8, options, pool);
	if (svnerr)
		return HandleSvnError(svnerr);

	svn_diff_t * tempdiff = diffTheirYourBase;
	LONG baseline = 0;
	LONG yourline = 0;
	LONG theirline = 0;
	LONG resline = 0;
	// common viewdata
	const viewdata emptyConflictEmpty(_T(""), DIFFSTATE_CONFLICTEMPTY, DIFF_EMPTYLINENUMBER, EOL_NOENDING, HIDESTATE_SHOWN, -1);
	const viewdata emptyIdenticalRemoved(_T(""), DIFFSTATE_IDENTICALREMOVED, DIFF_EMPTYLINENUMBER, EOL_NOENDING, HIDESTATE_SHOWN, -1);
	while (tempdiff)
	{
		if (tempdiff->type == svn_diff__type_common)
		{
			ASSERT((tempdiff->latest_length == tempdiff->modified_length) && (tempdiff->modified_length == tempdiff->original_length));
			for (int i=0; i<tempdiff->original_length; i++)
			{
				if ((m_arYourFile.GetCount() > yourline)&&(m_arTheirFile.GetCount() > theirline))
				{
					AddLines(baseline, yourline, theirline);

					m_Diff3.AddData(m_arYourFile.GetAt(yourline), DIFFSTATE_NORMAL, resline, m_arYourFile.GetLineEnding(yourline), HIDESTATE_HIDDEN, -1);
					m_YourBaseBoth.AddData(m_arYourFile.GetAt(yourline), DIFFSTATE_NORMAL, yourline, m_arYourFile.GetLineEnding(yourline), HIDESTATE_HIDDEN, -1);
					m_TheirBaseBoth.AddData(m_arTheirFile.GetAt(theirline), DIFFSTATE_NORMAL, theirline, m_arTheirFile.GetLineEnding(theirline), HIDESTATE_HIDDEN, -1);

					baseline++;
					yourline++;
					theirline++;
					resline++;
				}
			}
		}
		else if (tempdiff->type == svn_diff__type_diff_common)
		{
			ASSERT(tempdiff->latest_length == tempdiff->modified_length);
			//both theirs and yours have lines replaced
			for (int i=0; i<tempdiff->original_length; i++)
			{
				if (m_arBaseFile.GetCount() > baseline)
				{
					AddLines(baseline, yourline, theirline);

					m_Diff3.AddData(m_arBaseFile.GetAt(baseline), DIFFSTATE_IDENTICALREMOVED, DIFF_EMPTYLINENUMBER, m_arBaseFile.GetLineEnding(baseline), HIDESTATE_SHOWN, -1);
					m_YourBaseBoth.AddData(m_arBaseFile.GetAt(baseline), DIFFSTATE_IDENTICALREMOVED, DIFF_EMPTYLINENUMBER, EOL_NOENDING, HIDESTATE_SHOWN, -1);
					m_TheirBaseBoth.AddData(m_arBaseFile.GetAt(baseline), DIFFSTATE_IDENTICALREMOVED, DIFF_EMPTYLINENUMBER, EOL_NOENDING, HIDESTATE_SHOWN, -1);

					baseline++;
				}
			}
			for (int i=0; i<tempdiff->modified_length; i++)
			{
				if ((m_arYourFile.GetCount() > yourline)&&(m_arTheirFile.GetCount() > theirline))
				{
					AddLines(baseline, yourline, theirline);

					m_Diff3.AddData(m_arYourFile.GetAt(yourline), DIFFSTATE_IDENTICALADDED, resline, m_arYourFile.GetLineEnding(yourline), HIDESTATE_SHOWN, -1);
					m_YourBaseBoth.AddData(m_arYourFile.GetAt(yourline), DIFFSTATE_IDENTICALADDED, yourline, m_arYourFile.GetLineEnding(yourline), HIDESTATE_SHOWN, -1);
					m_TheirBaseBoth.AddData(m_arTheirFile.GetAt(theirline), DIFFSTATE_IDENTICALADDED, resline, m_arTheirFile.GetLineEnding(theirline), HIDESTATE_SHOWN, -1);

					yourline++;
					theirline++;
					resline++;
				}
			}
		}
		else if (tempdiff->type == svn_diff__type_conflict)
		{
			apr_off_t length = max(tempdiff->original_length, tempdiff->modified_length);
			length = max(tempdiff->latest_length, length);
			apr_off_t original = tempdiff->original_length;
			apr_off_t modified = tempdiff->modified_length;
			apr_off_t latest = tempdiff->latest_length;

			apr_off_t originalresolved = 0;
			apr_off_t modifiedresolved = 0;
			apr_off_t latestresolved = 0;

			LONG base = baseline;
			LONG your = yourline;
			LONG their = theirline;
			if (tempdiff->resolved_diff)
			{
				originalresolved = tempdiff->resolved_diff->original_length;
				modifiedresolved = tempdiff->resolved_diff->modified_length;
				latestresolved = tempdiff->resolved_diff->latest_length;
			}
			for (int i=0; i<length; i++)
			{
				EOL endingBase = m_arBaseFile.GetCount() > baseline ? m_arBaseFile.GetLineEnding(baseline) : EOL_AUTOLINE;
				if (original)
				{
					if (m_arBaseFile.GetCount() > baseline)
					{
						AddLines(baseline, yourline, theirline);

						m_Diff3.AddData(m_arBaseFile.GetAt(baseline), DIFFSTATE_IDENTICALREMOVED, DIFF_EMPTYLINENUMBER, endingBase , HIDESTATE_SHOWN, -1);
						m_YourBaseBoth.AddData(m_arBaseFile.GetAt(baseline), DIFFSTATE_IDENTICALREMOVED, DIFF_EMPTYLINENUMBER, endingBase , HIDESTATE_SHOWN, -1);
						m_TheirBaseBoth.AddData(m_arBaseFile.GetAt(baseline), DIFFSTATE_IDENTICALREMOVED, DIFF_EMPTYLINENUMBER, endingBase , HIDESTATE_SHOWN, -1);
					}
				}
				else if ((originalresolved)||((modifiedresolved)&&(latestresolved)))
				{
					AddLines(baseline, yourline, theirline);

					m_Diff3.AddData(emptyIdenticalRemoved);
					if ((latestresolved)&&(modifiedresolved))
					{
						m_YourBaseBoth.AddData(emptyIdenticalRemoved);
						m_TheirBaseBoth.AddData(emptyIdenticalRemoved);
					}
				}
				if (original)
				{
					original--;
					baseline++;
				}
				if (originalresolved)
					originalresolved--;

				if (modified)
				{
					modified--;
					theirline++;
				}
				if (modifiedresolved)
					modifiedresolved--;
				if (latest)
				{
					latest--;
					yourline++;
				}
				if (latestresolved)
					latestresolved--;
			}
			original = tempdiff->original_length;
			modified = tempdiff->modified_length;
			latest = tempdiff->latest_length;
			baseline = base;
			yourline = your;
			theirline = their;
			if (tempdiff->resolved_diff)
			{
				originalresolved = tempdiff->resolved_diff->original_length;
				modifiedresolved = tempdiff->resolved_diff->modified_length;
				latestresolved = tempdiff->resolved_diff->latest_length;
			}
			for (int i=0; i<length; i++)
			{
				if ((latest)||(modified))
				{
					AddLines(baseline, yourline, theirline);

					m_Diff3.AddData(_T(""), DIFFSTATE_CONFLICTED, resline, EOL_NOENDING, HIDESTATE_SHOWN, -1);

					resline++;
				}

				if (latest)
				{
					if (m_arYourFile.GetCount() > yourline)
					{
						m_YourBaseBoth.AddData(m_arYourFile.GetAt(yourline), DIFFSTATE_CONFLICTADDED, yourline, m_arYourFile.GetLineEnding(yourline), HIDESTATE_SHOWN, -1);
					}
				}
				else if ((latestresolved)||(modified)||(modifiedresolved))
				{
					m_YourBaseBoth.AddData(emptyConflictEmpty);
				}
				if (modified)
				{
					if (m_arTheirFile.GetCount() > theirline)
					{
						m_TheirBaseBoth.AddData(m_arTheirFile.GetAt(theirline), DIFFSTATE_CONFLICTADDED, theirline, m_arTheirFile.GetLineEnding(theirline), HIDESTATE_SHOWN, -1);
					}
				}
				else if ((modifiedresolved)||(latest)||(latestresolved))
				{
					m_TheirBaseBoth.AddData(emptyConflictEmpty);
				}
				if (original)
				{
					original--;
					baseline++;
				}
				if (originalresolved)
					originalresolved--;
				if (modified)
				{
					modified--;
					theirline++;
				}
				if (modifiedresolved)
					modifiedresolved--;
				if (latest)
				{
					latest--;
					yourline++;
				}
				if (latestresolved)
					latestresolved--;
			}
		}
		else if (tempdiff->type == svn_diff__type_diff_modified)
		{
			//deleted!
			for (int i=0; i<tempdiff->original_length; i++)
			{
				if ((m_arBaseFile.GetCount() > baseline)&&(m_arYourFile.GetCount() > yourline))
				{
					AddLines(baseline, yourline, theirline);

					m_Diff3.AddData(m_arBaseFile.GetAt(baseline), DIFFSTATE_THEIRSREMOVED, DIFF_EMPTYLINENUMBER, m_arBaseFile.GetLineEnding(baseline), HIDESTATE_SHOWN, -1);
					m_YourBaseBoth.AddData(m_arYourFile.GetAt(yourline), DIFFSTATE_NORMAL, yourline, m_arYourFile.GetLineEnding(yourline), HIDESTATE_SHOWN, -1);
					m_TheirBaseBoth.AddData(m_arBaseFile.GetAt(baseline), DIFFSTATE_THEIRSREMOVED, DIFF_EMPTYLINENUMBER, EOL_NOENDING, HIDESTATE_SHOWN, -1);

					baseline++;
					yourline++;
				}
			}
			//added
			for (int i=0; i<tempdiff->modified_length; i++)
			{
				if (m_arTheirFile.GetCount() > theirline)
				{
					AddLines(baseline, yourline, theirline);

					m_Diff3.AddData(m_arTheirFile.GetAt(theirline), DIFFSTATE_THEIRSADDED, resline, m_arTheirFile.GetLineEnding(theirline), HIDESTATE_SHOWN, -1);
					m_YourBaseBoth.AddEmpty();
					m_TheirBaseBoth.AddData(m_arTheirFile.GetAt(theirline), DIFFSTATE_THEIRSADDED, theirline, m_arTheirFile.GetLineEnding(theirline), HIDESTATE_SHOWN, -1);

					theirline++;
					resline++;
				}
			}
		}
		else if (tempdiff->type == svn_diff__type_diff_latest)
		{
			//YOURS differs from base

			for (int i=0; i<tempdiff->original_length; i++)
			{
				if ((m_arBaseFile.GetCount() > baseline)&&(m_arTheirFile.GetCount() > theirline))
				{
					AddLines(baseline, yourline, theirline);

					m_Diff3.AddData(m_arBaseFile.GetAt(baseline), DIFFSTATE_YOURSREMOVED, DIFF_EMPTYLINENUMBER, m_arBaseFile.GetLineEnding(baseline), HIDESTATE_SHOWN, -1);
					m_YourBaseBoth.AddData(m_arBaseFile.GetAt(baseline), DIFFSTATE_YOURSREMOVED, DIFF_EMPTYLINENUMBER, m_arBaseFile.GetLineEnding(baseline), HIDESTATE_SHOWN, -1);
					m_TheirBaseBoth.AddData(m_arTheirFile.GetAt(theirline), DIFFSTATE_NORMAL, theirline, m_arTheirFile.GetLineEnding(theirline), HIDESTATE_HIDDEN, -1);

					baseline++;
					theirline++;
				}
			}
			for (int i=0; i<tempdiff->latest_length; i++)
			{
				if (m_arYourFile.GetCount() > yourline)
				{
					AddLines(baseline, yourline, theirline);

					m_Diff3.AddData(m_arYourFile.GetAt(yourline), DIFFSTATE_YOURSADDED, resline, m_arYourFile.GetLineEnding(yourline), HIDESTATE_SHOWN, -1);
					m_YourBaseBoth.AddData(m_arYourFile.GetAt(yourline), DIFFSTATE_IDENTICALADDED, yourline, m_arYourFile.GetLineEnding(yourline), HIDESTATE_SHOWN, -1);
					m_TheirBaseBoth.AddEmpty();

					yourline++;
					resline++;
				}
			}
		}
		else
		{
			TRACE(_T("something bad happened during diff!\n"));
		}
		tempdiff = tempdiff->next;

	} // while (tempdiff)

	if ((options->ignore_space != svn_diff_file_ignore_space_none)||(bIgnoreCase)||(bIgnoreEOL))
	{
		// If whitespaces are ignored, a conflict could have been missed
		// We now go through all lines again and check if they're identical.
		// If they're not, then that means it is a conflict, and we
		// mark the conflict with the proper colors.
		for (long i=0; i<m_Diff3.GetCount(); ++i)
		{
			DiffStates state1 = m_YourBaseBoth.GetState(i);
			DiffStates state2 = m_TheirBaseBoth.GetState(i);

			if (((state1 == DIFFSTATE_IDENTICALADDED)||(state1 == DIFFSTATE_NORMAL))&&
				((state2 == DIFFSTATE_IDENTICALADDED)||(state2 == DIFFSTATE_NORMAL)))
			{
				LONG lineyour = m_arDiff3LinesYour.GetAt(i);
				LONG linetheir = m_arDiff3LinesTheir.GetAt(i);
				LONG linebase = m_arDiff3LinesBase.GetAt(i);
				if ((lineyour < m_arYourFile.GetCount()) &&
					(linetheir < m_arTheirFile.GetCount()) &&
					(linebase < m_arBaseFile.GetCount()))
				{
					if (((m_arYourFile.GetLineEnding(lineyour)!=m_arBaseFile.GetLineEnding(linebase))&&
						(m_arTheirFile.GetLineEnding(linetheir)!=m_arBaseFile.GetLineEnding(linebase))&&
						(m_arYourFile.GetLineEnding(lineyour)!=m_arTheirFile.GetLineEnding(linetheir))) ||
						((m_arYourFile.GetAt(lineyour).Compare(m_arBaseFile.GetAt(linebase))!=0)&&
						(m_arTheirFile.GetAt(linetheir).Compare(m_arBaseFile.GetAt(linebase))!=0)&&
						(m_arYourFile.GetAt(lineyour).Compare(m_arTheirFile.GetAt(linetheir))!=0)))
					{
						m_Diff3.SetState(i, DIFFSTATE_CONFLICTED_IGNORED);
						m_YourBaseBoth.SetState(i, DIFFSTATE_CONFLICTADDED);
						m_TheirBaseBoth.SetState(i, DIFFSTATE_CONFLICTADDED);
					}
				}
			}
		}
	}
	ASSERT(m_Diff3.GetCount() == m_YourBaseBoth.GetCount());
	ASSERT(m_TheirBaseBoth.GetCount() == m_YourBaseBoth.GetCount());

	TRACE(_T("done with 3-way diff\n"));

	HideUnchangedSections(&m_Diff3, &m_YourBaseBoth, &m_TheirBaseBoth);

	return true;
}

void CDiffData::HideUnchangedSections(CViewData * data1, CViewData * data2, CViewData * data3)
{
	if (data1 == NULL)
		return;

	CRegDWORD contextLines = CRegDWORD(_T("Software\\TortoiseGitMerge\\ContextLines"), 1);

	if (data1->GetCount() > 1)
	{
		HIDESTATE lastHideState = data1->GetHideState(0);
		if (lastHideState == HIDESTATE_HIDDEN)
		{
			data1->SetLineHideState(0, HIDESTATE_MARKER);
			if (data2)
				data2->SetLineHideState(0, HIDESTATE_MARKER);
			if (data3)
				data3->SetLineHideState(0, HIDESTATE_MARKER);
		}
		for (int i = 1; i < data1->GetCount(); ++i)
		{
			HIDESTATE hideState = data1->GetHideState(i);
			if (hideState != lastHideState)
			{
				if (hideState == HIDESTATE_SHOWN)
				{
					// go back and show the last 'contextLines' lines to "SHOWN"
					int lineback = i - 1;
					int stopline = lineback - (int)(DWORD)contextLines;
					while ((lineback >= 0)&&(lineback > stopline))
					{
						data1->SetLineHideState(lineback, HIDESTATE_SHOWN);
						if (data2)
							data2->SetLineHideState(lineback, HIDESTATE_SHOWN);
						if (data3)
							data3->SetLineHideState(lineback, HIDESTATE_SHOWN);
						lineback--;
					}
				}
				else if ((hideState == HIDESTATE_HIDDEN)&&(lastHideState != HIDESTATE_MARKER))
				{
					// go forward and show the next 'contextLines' lines to "SHOWN"
					int lineforward = i + 1;
					int stopline = lineforward + (int)(DWORD)contextLines;
					while ((lineforward < data1->GetCount())&&(lineforward < stopline))
					{
						data1->SetLineHideState(lineforward, HIDESTATE_SHOWN);
						if (data2)
							data2->SetLineHideState(lineforward, HIDESTATE_SHOWN);
						if (data3)
							data3->SetLineHideState(lineforward, HIDESTATE_SHOWN);
						lineforward++;
					}
					if ((lineforward < data1->GetCount())&&(data1->GetHideState(lineforward) == HIDESTATE_HIDDEN))
					{
						data1->SetLineHideState(lineforward, HIDESTATE_MARKER);
						if (data2)
							data2->SetLineHideState(lineforward, HIDESTATE_MARKER);
						if (data3)
							data3->SetLineHideState(lineforward, HIDESTATE_MARKER);
					}
				}
			}
			lastHideState = hideState;
		}
	}
}