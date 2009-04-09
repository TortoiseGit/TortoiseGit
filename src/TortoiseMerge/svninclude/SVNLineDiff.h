// TortoiseMerge - a Diff/Patch program

// Copyright (C) 2006-2007 - TortoiseSVN

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
#include "diff.h"
#include "svn_pools.h"

/**
 * \ingroup TortoiseMerge
 * Handles diffs of single lines. Used for inline diffs.
 */
class SVNLineDiff
{
public:
	SVNLineDiff();
	~SVNLineDiff();

	bool Diff(svn_diff_t** diff, LPCTSTR line1, int len1, LPCTSTR line2, int len2, bool bWordDiff);
	/** Checks if we really should show inline diffs.
	 * Inline diffs are only useful if the two lines are not
	 * completely different but at least a little bit similar.
	 */
	static bool ShowInlineDiff(svn_diff_t* diff);

	std::vector<std::wstring>	m_line1tokens;
	std::vector<std::wstring>	m_line2tokens;
private:

	apr_pool_t *		m_pool;
	apr_pool_t *		m_subpool;
	LPCTSTR				m_line1;
	unsigned long		m_line1length;
	LPCTSTR				m_line2;
	unsigned long		m_line2length;
	unsigned long		m_line1pos;
	unsigned long		m_line2pos;

	bool				m_bWordDiff;

	static svn_error_t * datasource_open(void *baton, svn_diff_datasource_e datasource);
	static svn_error_t * datasource_close(void *baton, svn_diff_datasource_e datasource);
	static svn_error_t * next_token(apr_uint32_t * hash, void ** token, void * baton, svn_diff_datasource_e datasource);
	static svn_error_t * compare_token(void * baton, void * token1, void * token2, int * compare);
	static void discard_token(void * baton, void * token);
	static void discard_all_token(void *baton);
	static bool IsCharWhiteSpace(TCHAR c);

	static apr_uint32_t Adler32(apr_uint32_t checksum, const WCHAR *data, apr_size_t len);
	static void ParseLineWords(
		LPCTSTR line, unsigned long lineLength, std::vector<std::wstring>& tokens);
	static void ParseLineChars(
		LPCTSTR line, unsigned long lineLength, std::vector<std::wstring>& tokens);
	static void NextTokenWords(
		apr_uint32_t* hash, void** token, unsigned long& linePos, const std::vector<std::wstring>& tokens);
	static void NextTokenChars(
		apr_uint32_t* hash, void** token, unsigned long& linePos, LPCTSTR line, unsigned long lineLength);
	static const svn_diff_fns_t SVNLineDiff_vtable;
};