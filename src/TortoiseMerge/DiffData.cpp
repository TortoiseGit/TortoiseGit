// TortoiseMerge - a Diff/Patch program

// Copyright (C) 2006-2009 - TortoiseSVN

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
#include "StdAfx.h"
#include "svn_version.h"
#include "svn_io.h"
#include "diff.h"
#include "TempFiles.h"
#include "registry.h"
#include "Resource.h"
#include "Diffdata.h"
#include "UnicodeUtils.h"
#include "GitAdminDir.h"
#include "svn_dso.h"

#pragma warning(push)
#pragma warning(disable: 4702) // unreachable code
int CDiffData::abort_on_pool_failure (int /*retcode*/)
{
	abort ();
	return -1;
}
#pragma warning(pop)

CDiffData::CDiffData(void)
	: m_bBlame(false)
{
	apr_initialize();
	svn_dso_initialize2();

	m_sPatchOriginal = _T(": original");
	m_sPatchPatched = _T(": patched");
}

CDiffData::~CDiffData(void)
{
	apr_terminate();
}

int CDiffData::GetLineCount()
{
	int count = 0;
	count = (int)m_arBaseFile.GetCount();
	count = (int)(count > m_arTheirFile.GetCount() ? count : m_arTheirFile.GetCount());
	count = (int)(count > m_arYourFile.GetCount() ? count : m_arYourFile.GetCount());
	return count;
}

int CDiffData::GetLineActualLength(int index)
{
	int count = 0;
	if (index < m_arBaseFile.GetCount())
		count = (count > m_arBaseFile.GetAt(index).GetLength() ? count : m_arBaseFile.GetAt(index).GetLength());
	if (index < m_arTheirFile.GetCount())
		count = (count > m_arTheirFile.GetAt(index).GetLength() ? count : m_arTheirFile.GetAt(index).GetLength());
	if (index < m_arYourFile.GetCount())
		count = (count > m_arYourFile.GetAt(index).GetLength() ? count : m_arYourFile.GetAt(index).GetLength());
	return count;
}

LPCTSTR CDiffData::GetLineChars(int index)
{
	if (index < m_arBaseFile.GetCount())
		return m_arBaseFile.GetAt(index);
	if (index < m_arTheirFile.GetCount())
		return m_arTheirFile.GetAt(index);
	if (index < m_arYourFile.GetCount())
		return m_arYourFile.GetAt(index);
	return NULL;
}

BOOL CDiffData::Load()
{
	CString sConvertedBaseFilename, sConvertedTheirFilename, sConvertedYourFilename;
	apr_pool_t * pool;

	apr_pool_create_ex (&pool, NULL, abort_on_pool_failure, NULL);

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

	m_arDiff3LinesBase.RemoveAll();
	m_arDiff3LinesYour.RemoveAll();
	m_arDiff3LinesTheir.RemoveAll();

	CTempFiles tempfiles;
	CRegDWORD regIgnoreWS = CRegDWORD(_T("Software\\TortoiseMerge\\IgnoreWS"));
	CRegDWORD regIgnoreEOL = CRegDWORD(_T("Software\\TortoiseMerge\\IgnoreEOL"), TRUE);
	CRegDWORD regIgnoreCase = CRegDWORD(_T("Software\\TortoiseMerge\\CaseInsensitive"), FALSE);
	DWORD dwIgnoreWS = regIgnoreWS;
	bool bIgnoreEOL = ((DWORD)regIgnoreEOL)!=0;
	BOOL bIgnoreCase = ((DWORD)regIgnoreCase)!=0;

	// The Subversion diff API only can ignore whitespaces and eol styles.
	// It also can only handle one-byte charsets.
	// To ignore case changes or to handle UTF-16 files, we have to
	// save the original file in UTF-8 and/or the letters changed to lowercase
	// so the Subversion diff can handle those.
	sConvertedBaseFilename = m_baseFile.GetFilename();
	sConvertedYourFilename = m_yourFile.GetFilename();
	sConvertedTheirFilename = m_theirFile.GetFilename();
	if (IsBaseFileInUse())
	{
		if (!m_arBaseFile.Load(m_baseFile.GetFilename()))
		{
			m_sError = m_arBaseFile.GetErrorString();
			return FALSE;
		}
		if ((bIgnoreCase)||(m_arBaseFile.GetUnicodeType() == CFileTextLines::UNICODE_LE)||(m_arBaseFile.GetUnicodeType() == CFileTextLines::UNICODE_BE))
		{
			CFileTextLines converted(m_arBaseFile);
			sConvertedBaseFilename = tempfiles.GetTempFilePath();
			converted.Save(sConvertedBaseFilename, m_arBaseFile.GetUnicodeType() == CFileTextLines::UNICODE_LE || m_arBaseFile.GetUnicodeType() == CFileTextLines::UNICODE_BE, dwIgnoreWS, bIgnoreCase, m_bBlame);
		}
	}

	if (IsTheirFileInUse())
	{
		// m_arBaseFile.GetCount() is passed as a hint for the number of lines in this file
		// It's a fair guess that the files will be roughly the same size
		if (!m_arTheirFile.Load(m_theirFile.GetFilename(),m_arBaseFile.GetCount()))
		{
			m_sError = m_arTheirFile.GetErrorString();
			return FALSE;
		}
		if ((bIgnoreCase)||(m_arTheirFile.GetUnicodeType() == CFileTextLines::UNICODE_LE)||(m_arTheirFile.GetUnicodeType() == CFileTextLines::UNICODE_BE))
		{
			CFileTextLines converted(m_arTheirFile);
			sConvertedTheirFilename = tempfiles.GetTempFilePath();
			converted.Save(sConvertedTheirFilename, m_arTheirFile.GetUnicodeType() == CFileTextLines::UNICODE_LE || m_arTheirFile.GetUnicodeType() == CFileTextLines::UNICODE_BE, dwIgnoreWS, bIgnoreCase, m_bBlame);
		}
	}

	if (IsYourFileInUse())
	{
		// m_arBaseFile.GetCount() is passed as a hint for the number of lines in this file
		// It's a fair guess that the files will be roughly the same size
		if (!m_arYourFile.Load(m_yourFile.GetFilename(),m_arBaseFile.GetCount()))
		{
			m_sError = m_arYourFile.GetErrorString();
			return FALSE;
		}
		if ((bIgnoreCase)||(m_arYourFile.GetUnicodeType() == CFileTextLines::UNICODE_LE)||(m_arYourFile.GetUnicodeType() == CFileTextLines::UNICODE_BE))
		{
			CFileTextLines converted(m_arYourFile);
			sConvertedYourFilename = tempfiles.GetTempFilePath();
			converted.Save(sConvertedYourFilename, m_arYourFile.GetUnicodeType() == CFileTextLines::UNICODE_LE || m_arYourFile.GetUnicodeType() == CFileTextLines::UNICODE_BE, dwIgnoreWS, bIgnoreCase, m_bBlame);
		}
	}

	// Calculate the number of lines in the largest of the three files
	int lengthHint = max(m_arBaseFile.GetCount(), m_arTheirFile.GetCount());
	lengthHint = max(lengthHint, m_arYourFile.GetCount());

	try
	{
		m_YourBaseBoth.Reserve(lengthHint);
		m_YourBaseLeft.Reserve(lengthHint);
		m_YourBaseRight.Reserve(lengthHint);

		m_TheirBaseBoth.Reserve(lengthHint);
		m_TheirBaseLeft.Reserve(lengthHint);
		m_TheirBaseRight.Reserve(lengthHint);

		m_arDiff3LinesBase.Reserve(lengthHint);
		m_arDiff3LinesYour.Reserve(lengthHint);
		m_arDiff3LinesTheir.Reserve(lengthHint);
	}
	catch (CMemoryException* e)
	{
		e->GetErrorMessage(m_sError.GetBuffer(255), 255);
		m_sError.ReleaseBuffer();
		return FALSE;
	}

	// Is this a two-way diff?
	if (IsBaseFileInUse() && IsYourFileInUse() && !IsTheirFileInUse())
	{
		if (!DoTwoWayDiff(sConvertedBaseFilename, sConvertedYourFilename, dwIgnoreWS, bIgnoreEOL, pool))
		{
			apr_pool_destroy (pool);					// free the allocated memory
			return FALSE;
		}
	}

	if (IsBaseFileInUse() && IsTheirFileInUse() && !IsYourFileInUse())
	{
		ASSERT(FALSE);
	}

	if (IsBaseFileInUse() && IsTheirFileInUse() && IsYourFileInUse())
	{
		m_Diff3.Reserve(lengthHint);

		if (!DoThreeWayDiff(sConvertedBaseFilename, sConvertedYourFilename, sConvertedTheirFilename, dwIgnoreWS, bIgnoreEOL, !!bIgnoreCase, pool))
		{
			apr_pool_destroy (pool);					// free the allocated memory
			return FALSE;
		}
	}

	apr_pool_destroy (pool);					// free the allocated memory
	return TRUE;
}


bool
CDiffData::DoTwoWayDiff(const CString& sBaseFilename, const CString& sYourFilename, DWORD dwIgnoreWS, bool bIgnoreEOL, apr_pool_t * pool)
{
	// convert CString filenames (UTF-16 or ANSI) to UTF-8
	CStringA sBaseFilenameUtf8 = CUnicodeUtils::GetUTF8(sBaseFilename);
	CStringA sYourFilenameUtf8 = CUnicodeUtils::GetUTF8(sYourFilename);

	svn_diff_t * diffYourBase = NULL;
	svn_error_t * svnerr = NULL;
	svn_diff_file_options_t * options = svn_diff_file_options_create(pool);
	options->ignore_eol_style = bIgnoreEOL;
	options->ignore_space = svn_diff_file_ignore_space_none;
	switch (dwIgnoreWS)
	{
	case 0:
		options->ignore_space = svn_diff_file_ignore_space_none;
		break;
	case 1:
		options->ignore_space = svn_diff_file_ignore_space_all;
		break;
	case 2:
		options->ignore_space = svn_diff_file_ignore_space_change;
		break;
	}

	svnerr = svn_diff_file_diff_2(&diffYourBase, sBaseFilenameUtf8, sYourFilenameUtf8, options, pool);
	if (svnerr)
	{
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
	svn_diff_t * tempdiff = diffYourBase;
	LONG baseline = 0;
	LONG yourline = 0;

	while (tempdiff)
	{
		for (int i=0; i<tempdiff->original_length; i++)
		{
			if (baseline >= m_arBaseFile.GetCount())
			{
				m_sError.LoadString(IDS_ERR_DIFF_NEWLINES);
				return false;
			}
			const CString& sCurrentBaseLine = m_arBaseFile.GetAt(baseline);
			EOL endingBase = m_arBaseFile.GetLineEnding(baseline);
			if (tempdiff->type == svn_diff__type_common)
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
					if (dwIgnoreWS == 2 || dwIgnoreWS == 3)
					{
						CString s1 = m_arBaseFile.GetAt(baseline);
						CString s2 = sCurrentYourLine;
						
						if ( dwIgnoreWS == 2 )
						{
							s1.TrimLeft(_T(" \t"));
							s2.TrimLeft(_T(" \t"));
						}
						else
						{
							s1.TrimRight(_T(" \t"));
							s2.TrimRight(_T(" \t"));
						}

						if (s1 != s2)
						{
							// one-pane view: two lines, one 'removed' and one 'added'
							m_YourBaseBoth.AddData(sCurrentBaseLine, DIFFSTATE_REMOVEDWHITESPACE, yourline, endingBase);
							m_YourBaseBoth.AddData(sCurrentYourLine, DIFFSTATE_ADDEDWHITESPACE, yourline, endingYours);
						}
						else
						{
							m_YourBaseBoth.AddData(sCurrentBaseLine, DIFFSTATE_NORMAL, yourline, endingBase);
						}
					}
					else if (dwIgnoreWS == 0)
					{
						m_YourBaseBoth.AddData(sCurrentBaseLine, DIFFSTATE_REMOVEDWHITESPACE, yourline, endingBase);
						m_YourBaseBoth.AddData(sCurrentYourLine, DIFFSTATE_ADDEDWHITESPACE, yourline, endingYours);
					}
					else
					{
						m_YourBaseBoth.AddData(sCurrentBaseLine, DIFFSTATE_NORMAL, yourline, endingBase);
					}
				}
				else
				{
					m_YourBaseBoth.AddData(sCurrentBaseLine, DIFFSTATE_NORMAL, yourline, endingBase);
				}
				yourline++;		//in both files
			}
			else
			{
				m_YourBaseBoth.AddData(sCurrentBaseLine, DIFFSTATE_REMOVED, yourline, endingBase);
			}
			baseline++;
		}
		if (tempdiff->type == svn_diff__type_diff_modified)
		{
			for (int i=0; i<tempdiff->modified_length; i++)
			{
				if (m_arYourFile.GetCount() > yourline)
				{
					m_YourBaseBoth.AddData(m_arYourFile.GetAt(yourline), DIFFSTATE_ADDED, yourline, m_arYourFile.GetLineEnding(yourline));
				}
				yourline++;
			}
		}
		tempdiff = tempdiff->next;
	}

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
					if (dwIgnoreWS == 2 || dwIgnoreWS == 3)
					{
						CString s1 = sCurrentBaseLine;
						CString s2 = sCurrentYourLine;
						if ( dwIgnoreWS == 2 )
						{
							s1 = s1.TrimLeft(_T(" \t"));
							s2 = s2.TrimLeft(_T(" \t"));
						}
						else
						{
							s1 = s1.TrimRight(_T(" \t"));
							s2 = s2.TrimRight(_T(" \t"));
						}
						
						if (s1 != s2)
						{
							m_YourBaseLeft.AddData(sCurrentBaseLine, DIFFSTATE_WHITESPACE, baseline, endingBase);
							m_YourBaseRight.AddData(sCurrentYourLine, DIFFSTATE_WHITESPACE, yourline, endingYours);
						}
						else
						{
							m_YourBaseLeft.AddData(sCurrentBaseLine, DIFFSTATE_NORMAL, baseline, endingBase);
							m_YourBaseRight.AddData(sCurrentYourLine, DIFFSTATE_NORMAL, yourline, endingYours);
						}
					}
					else if (dwIgnoreWS == 0)
					{
						m_YourBaseLeft.AddData(sCurrentBaseLine, DIFFSTATE_WHITESPACE, baseline, endingBase);
						m_YourBaseRight.AddData(sCurrentYourLine, DIFFSTATE_WHITESPACE, yourline, endingYours);
					}
					else
					{
						m_YourBaseLeft.AddData(sCurrentBaseLine, DIFFSTATE_NORMAL, baseline, endingBase);
						m_YourBaseRight.AddData(sCurrentYourLine, DIFFSTATE_NORMAL, yourline, endingYours);
					}
				}
				else
				{
					m_YourBaseLeft.AddData(sCurrentBaseLine, DIFFSTATE_NORMAL, baseline, endingBase);
					m_YourBaseRight.AddData(sCurrentYourLine, DIFFSTATE_NORMAL, yourline, endingYours);
				}
				baseline++;
				yourline++;
			}
		}
		if (tempdiff->type == svn_diff__type_diff_modified)
		{
			apr_off_t original_length = tempdiff->original_length;
			for (int i=0; i<tempdiff->modified_length; i++)
			{
				if (m_arYourFile.GetCount() > yourline)
				{
					EOL endingYours = m_arYourFile.GetLineEnding(yourline);
					if (original_length-- <= 0)
					{
						m_YourBaseLeft.AddData(_T(""), DIFFSTATE_EMPTY, DIFF_EMPTYLINENUMBER, EOL_NOENDING);
						m_YourBaseRight.AddData(m_arYourFile.GetAt(yourline), DIFFSTATE_ADDED, yourline, endingYours);
					}
					else
					{
						EOL endingBase = m_arBaseFile.GetLineEnding(baseline);
						m_YourBaseLeft.AddData(m_arBaseFile.GetAt(baseline), DIFFSTATE_REMOVED, baseline, endingBase);
						m_YourBaseRight.AddData(m_arYourFile.GetAt(yourline), DIFFSTATE_ADDED, yourline, endingYours);
						baseline++;
					}
					yourline++;
				}
			}
			apr_off_t modified_length = tempdiff->modified_length;
			for (int i=0; i<tempdiff->original_length; i++)
			{
				if (modified_length-- <= 0)
				{
					if (m_arBaseFile.GetCount() > baseline)
					{
						EOL endingBase = m_arBaseFile.GetLineEnding(baseline);
						m_YourBaseLeft.AddData(m_arBaseFile.GetAt(baseline), DIFFSTATE_REMOVED, baseline, endingBase);
						m_YourBaseRight.AddData(_T(""), DIFFSTATE_EMPTY, DIFF_EMPTYLINENUMBER, EOL_NOENDING);
						baseline++;
					}
				}
			}
		}
		tempdiff = tempdiff->next;
	}
	TRACE(_T("done with 2-way diff\n"));

	return true;
}

bool
CDiffData::DoThreeWayDiff(const CString& sBaseFilename, const CString& sYourFilename, const CString& sTheirFilename, DWORD dwIgnoreWS, bool bIgnoreEOL, bool bIgnoreCase, apr_pool_t * pool)
{
	// convert CString filenames (UTF-16 or ANSI) to UTF-8
	CStringA sBaseFilenameUtf8  = CUnicodeUtils::GetUTF8(sBaseFilename);
	CStringA sYourFilenameUtf8  = CUnicodeUtils::GetUTF8(sYourFilename);
	CStringA sTheirFilenameUtf8 = CUnicodeUtils::GetUTF8(sTheirFilename);
	svn_diff_t * diffTheirYourBase = NULL;
	svn_diff_file_options_t * options = svn_diff_file_options_create(pool);
	options->ignore_eol_style = bIgnoreEOL;
	options->ignore_space = svn_diff_file_ignore_space_none;
	switch (dwIgnoreWS)
	{
	case 0:
		options->ignore_space = svn_diff_file_ignore_space_none;
		break;
	case 1:
		options->ignore_space = svn_diff_file_ignore_space_all;
		break;
	case 2:
		options->ignore_space = svn_diff_file_ignore_space_change;
		break;
	}
	svn_error_t * svnerr = svn_diff_file_diff3_2(&diffTheirYourBase, sBaseFilenameUtf8, sTheirFilenameUtf8, sYourFilenameUtf8, options, pool);
	if (svnerr)
	{
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

	svn_diff_t * tempdiff = diffTheirYourBase;
	LONG baseline = 0;
	LONG yourline = 0;
	LONG theirline = 0;
	LONG resline = 0;
	while (tempdiff)
	{
		if (tempdiff->type == svn_diff__type_common)
		{
			ASSERT((tempdiff->latest_length == tempdiff->modified_length) && (tempdiff->modified_length == tempdiff->original_length));
			for (int i=0; i<tempdiff->original_length; i++)
			{
				if ((m_arYourFile.GetCount() > yourline)&&(m_arTheirFile.GetCount() > theirline))
				{
					m_Diff3.AddData(m_arYourFile.GetAt(yourline), DIFFSTATE_NORMAL, resline, m_arYourFile.GetLineEnding(yourline));

					m_arDiff3LinesBase.Add(baseline);
					m_arDiff3LinesYour.Add(yourline);
					m_arDiff3LinesTheir.Add(theirline);

					m_YourBaseBoth.AddData(m_arYourFile.GetAt(yourline), DIFFSTATE_NORMAL, yourline, m_arYourFile.GetLineEnding(yourline));
					m_TheirBaseBoth.AddData(m_arTheirFile.GetAt(theirline), DIFFSTATE_NORMAL, theirline, m_arTheirFile.GetLineEnding(theirline));

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
					m_Diff3.AddData(m_arBaseFile.GetAt(baseline), DIFFSTATE_IDENTICALREMOVED, DIFF_EMPTYLINENUMBER, m_arBaseFile.GetLineEnding(baseline));

					m_arDiff3LinesBase.Add(baseline);
					m_arDiff3LinesYour.Add(yourline);
					m_arDiff3LinesTheir.Add(theirline);

					m_YourBaseBoth.AddData(m_arBaseFile.GetAt(baseline), DIFFSTATE_IDENTICALREMOVED, DIFF_EMPTYLINENUMBER, EOL_NOENDING);
					m_TheirBaseBoth.AddData(m_arBaseFile.GetAt(baseline), DIFFSTATE_IDENTICALREMOVED, DIFF_EMPTYLINENUMBER, EOL_NOENDING);

					baseline++;
				}
			}
			for (int i=0; i<tempdiff->modified_length; i++)
			{
				if ((m_arYourFile.GetCount() > yourline)&&(m_arTheirFile.GetCount() > theirline))
				{
					m_Diff3.AddData(m_arYourFile.GetAt(yourline), DIFFSTATE_IDENTICALADDED, resline, m_arYourFile.GetLineEnding(yourline));

					m_arDiff3LinesBase.Add(baseline);
					m_arDiff3LinesYour.Add(yourline);
					m_arDiff3LinesTheir.Add(theirline);

					m_YourBaseBoth.AddData(m_arYourFile.GetAt(yourline), DIFFSTATE_IDENTICALADDED, yourline, m_arYourFile.GetLineEnding(yourline));
					m_TheirBaseBoth.AddData(m_arTheirFile.GetAt(theirline), DIFFSTATE_IDENTICALADDED, resline, m_arTheirFile.GetLineEnding(theirline));

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
				if (original)
				{
					if (m_arBaseFile.GetCount() > baseline)
					{
						m_Diff3.AddData(m_arBaseFile.GetAt(baseline), DIFFSTATE_IDENTICALREMOVED, DIFF_EMPTYLINENUMBER, m_arBaseFile.GetLineEnding(baseline));

						m_arDiff3LinesBase.Add(baseline);
						m_arDiff3LinesYour.Add(yourline);
						m_arDiff3LinesTheir.Add(theirline);
					}
				}
				else if ((originalresolved)||((modifiedresolved)&&(latestresolved)))
				{
					m_Diff3.AddData(_T(""), DIFFSTATE_IDENTICALREMOVED, DIFF_EMPTYLINENUMBER, EOL_NOENDING);

					m_arDiff3LinesBase.Add(baseline);
					m_arDiff3LinesYour.Add(yourline);
					m_arDiff3LinesTheir.Add(theirline);

				}
				if ((latest)&&(original))
				{
					if (m_arBaseFile.GetCount() > baseline)
					{
						m_YourBaseBoth.AddData(m_arBaseFile.GetAt(baseline), DIFFSTATE_IDENTICALREMOVED, DIFF_EMPTYLINENUMBER, m_arBaseFile.GetLineEnding(baseline));
					}
				}
				else
				{
					if (original)
					{
						if (m_arBaseFile.GetCount() > baseline)
						{
							m_YourBaseBoth.AddData(m_arBaseFile.GetAt(baseline), DIFFSTATE_IDENTICALREMOVED, DIFF_EMPTYLINENUMBER, m_arBaseFile.GetLineEnding(baseline));
						}
					}
					else if ((latestresolved)&&(modifiedresolved))
					{
						m_YourBaseBoth.AddData(_T(""), DIFFSTATE_IDENTICALREMOVED, DIFF_EMPTYLINENUMBER, EOL_NOENDING);
					}
				}
				if ((modified)&&(original))
				{
					if (m_arBaseFile.GetCount() > baseline)
					{
						m_TheirBaseBoth.AddData(m_arBaseFile.GetAt(baseline), DIFFSTATE_IDENTICALREMOVED, DIFF_EMPTYLINENUMBER, m_arBaseFile.GetLineEnding(baseline));
					}
				}
				else
				{
					if (original)
					{
						if (m_arBaseFile.GetCount() > baseline)
						{
							m_TheirBaseBoth.AddData(m_arBaseFile.GetAt(baseline), DIFFSTATE_IDENTICALREMOVED, DIFF_EMPTYLINENUMBER, m_arBaseFile.GetLineEnding(baseline));
						}
					}
					else if ((modifiedresolved)&&(latestresolved))
					{
						m_TheirBaseBoth.AddData(_T(""), DIFFSTATE_IDENTICALREMOVED, DIFF_EMPTYLINENUMBER, EOL_NOENDING);
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
					m_Diff3.AddData(_T(""), DIFFSTATE_CONFLICTED, resline, EOL_NOENDING);

					m_arDiff3LinesBase.Add(baseline);
					m_arDiff3LinesYour.Add(yourline);
					m_arDiff3LinesTheir.Add(theirline);

					resline++;
				}

				if (latest)
				{
					if (m_arYourFile.GetCount() > yourline)
					{
						m_YourBaseBoth.AddData(m_arYourFile.GetAt(yourline), DIFFSTATE_CONFLICTADDED, yourline, m_arYourFile.GetLineEnding(yourline));
					}
				}
				else if ((latestresolved)||(modified)||(modifiedresolved))
				{
					m_YourBaseBoth.AddData(_T(""), DIFFSTATE_CONFLICTEMPTY, DIFF_EMPTYLINENUMBER, EOL_NOENDING);
				}
				if (modified)
				{
					if (m_arTheirFile.GetCount() > theirline)
					{
						m_TheirBaseBoth.AddData(m_arTheirFile.GetAt(theirline), DIFFSTATE_CONFLICTADDED, theirline, m_arTheirFile.GetLineEnding(theirline));
					}
				}
				else if ((modifiedresolved)||(latest)||(latestresolved))
				{
					m_TheirBaseBoth.AddData(_T(""), DIFFSTATE_CONFLICTEMPTY, DIFF_EMPTYLINENUMBER, EOL_NOENDING);
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
					m_Diff3.AddData(m_arBaseFile.GetAt(baseline), DIFFSTATE_THEIRSREMOVED, DIFF_EMPTYLINENUMBER, m_arBaseFile.GetLineEnding(baseline));

					m_arDiff3LinesBase.Add(baseline);
					m_arDiff3LinesYour.Add(yourline);
					m_arDiff3LinesTheir.Add(theirline);

					m_YourBaseBoth.AddData(m_arYourFile.GetAt(yourline), DIFFSTATE_NORMAL, yourline, m_arYourFile.GetLineEnding(yourline));
					m_TheirBaseBoth.AddData(m_arBaseFile.GetAt(baseline), DIFFSTATE_THEIRSREMOVED, DIFF_EMPTYLINENUMBER, EOL_NOENDING);
					baseline++;
					yourline++;
				}
			}
			//added
			for (int i=0; i<tempdiff->modified_length; i++)
			{
				if (m_arTheirFile.GetCount() > theirline)
				{
					m_Diff3.AddData(m_arTheirFile.GetAt(theirline), DIFFSTATE_THEIRSADDED, resline, m_arTheirFile.GetLineEnding(theirline));

					m_arDiff3LinesBase.Add(baseline);
					m_arDiff3LinesYour.Add(yourline);
					m_arDiff3LinesTheir.Add(theirline);

					m_YourBaseBoth.AddData(_T(""), DIFFSTATE_EMPTY, DIFF_EMPTYLINENUMBER, EOL_NOENDING);
					m_TheirBaseBoth.AddData(m_arTheirFile.GetAt(theirline), DIFFSTATE_THEIRSADDED, theirline, m_arTheirFile.GetLineEnding(theirline));

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
					m_Diff3.AddData(m_arBaseFile.GetAt(baseline), DIFFSTATE_YOURSREMOVED, DIFF_EMPTYLINENUMBER, m_arBaseFile.GetLineEnding(baseline));

					m_arDiff3LinesBase.Add(baseline);
					m_arDiff3LinesYour.Add(yourline);
					m_arDiff3LinesTheir.Add(theirline);

					m_YourBaseBoth.AddData(m_arBaseFile.GetAt(baseline), DIFFSTATE_YOURSREMOVED, DIFF_EMPTYLINENUMBER, m_arBaseFile.GetLineEnding(baseline));
					m_TheirBaseBoth.AddData(m_arTheirFile.GetAt(theirline), DIFFSTATE_NORMAL, theirline, m_arTheirFile.GetLineEnding(theirline));

					baseline++;
					theirline++;
				}
			}
			for (int i=0; i<tempdiff->latest_length; i++)
			{
				if (m_arYourFile.GetCount() > yourline)
				{
					m_Diff3.AddData(m_arYourFile.GetAt(yourline), DIFFSTATE_YOURSADDED, resline, m_arYourFile.GetLineEnding(yourline));

					m_arDiff3LinesBase.Add(baseline);
					m_arDiff3LinesYour.Add(yourline);
					m_arDiff3LinesTheir.Add(theirline);

					m_YourBaseBoth.AddData(m_arYourFile.GetAt(yourline), DIFFSTATE_IDENTICALADDED, yourline, m_arYourFile.GetLineEnding(yourline));
					m_TheirBaseBoth.AddData(_T(""), DIFFSTATE_EMPTY, DIFF_EMPTYLINENUMBER, EOL_NOENDING);

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
	return true;
}
