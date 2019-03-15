// TortoiseGitMerge - a Diff/Patch program

// Copyright (C) 2006-2009, 2011 - TortoiseSVN

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
#include "SVNLineDiff.h"

const svn_diff_fns2_t SVNLineDiff::SVNLineDiff_vtable =
{
	SVNLineDiff::datasources_open,
	SVNLineDiff::datasource_close,
	SVNLineDiff::next_token,
	SVNLineDiff::compare_token,
	SVNLineDiff::discard_token,
	SVNLineDiff::discard_all_token
};

#define SVNLINEDIFF_CHARTYPE_NONE			0
#define SVNLINEDIFF_CHARTYPE_ALPHANUMERIC	1
#define SVNLINEDIFF_CHARTYPE_SPACE			2
#define SVNLINEDIFF_CHARTYPE_OTHER			3

typedef void (*LineParser)(LPCTSTR line, apr_size_t lineLength, std::vector<std::wstring> &tokens);

void SVNLineDiff::ParseLineWords(
	LPCTSTR line, apr_size_t lineLength, std::vector<std::wstring> &tokens)
{
	std::wstring token;
	int prevCharType = SVNLINEDIFF_CHARTYPE_NONE;
	tokens.reserve(lineLength/2);
	for (apr_size_t i = 0; i < lineLength; ++i)
	{
		int charType =
			IsCharAlphaNumeric(line[i]) ? SVNLINEDIFF_CHARTYPE_ALPHANUMERIC :
			IsCharWhiteSpace(line[i]) ? SVNLINEDIFF_CHARTYPE_SPACE :
			SVNLINEDIFF_CHARTYPE_OTHER;

		// Token is a sequence of either alphanumeric or whitespace characters.
		// Treat all other characters as a separate tokens.
		if (charType == prevCharType && charType != SVNLINEDIFF_CHARTYPE_OTHER)
			token += line[i];
		else
		{
			if (!token.empty())
				tokens.push_back(token);
			token = line[i];
		}
		prevCharType = charType;
	}
	if (!token.empty())
		tokens.push_back(token);
}

void SVNLineDiff::ParseLineChars(
	LPCTSTR line, apr_size_t lineLength, std::vector<std::wstring> &tokens)
{
	std::wstring token;
	for (apr_size_t i = 0; i < lineLength; ++i)
	{
		token = line[i];
		tokens.push_back(token);
	}
}

svn_error_t * SVNLineDiff::datasources_open(void *baton, apr_off_t *prefix_lines, apr_off_t * /*suffix_lines*/, const svn_diff_datasource_e *datasources, apr_size_t datasource_len)
{
	auto linediff = static_cast<SVNLineDiff*>(baton);
	LineParser parser = linediff->m_bWordDiff ? ParseLineWords : ParseLineChars;
	for (apr_size_t i = 0; i < datasource_len; i++)
	{
		switch (datasources[i])
		{
		case svn_diff_datasource_original:
			parser(linediff->m_line1, linediff->m_line1length, linediff->m_line1tokens);
			break;
		case svn_diff_datasource_modified:
			parser(linediff->m_line2, linediff->m_line2length, linediff->m_line2tokens);
			break;
		}
	}
	// Don't claim any prefix matches.
	*prefix_lines = 0;

	return SVN_NO_ERROR;
}

svn_error_t * SVNLineDiff::datasource_close(void * /*baton*/, svn_diff_datasource_e /*datasource*/)
{
	return SVN_NO_ERROR;
}

void SVNLineDiff::NextTokenWords(
	apr_uint32_t* hash, void** token, apr_size_t& linePos, const std::vector<std::wstring>& tokens)
{
	if (linePos < tokens.size())
	{
		*token = (void*)tokens[linePos].c_str();
		*hash = SVNLineDiff::Adler32(0, tokens[linePos].c_str(), tokens[linePos].size());
		linePos++;
	}
}

void SVNLineDiff::NextTokenChars(
	apr_uint32_t* hash, void** token, apr_size_t& linePos, LPCTSTR line, apr_size_t lineLength)
{
	if (linePos < lineLength)
	{
		*token = (void*)&line[linePos];
		*hash = line[linePos];
		linePos++;
	}
}

svn_error_t * SVNLineDiff::next_token(
	apr_uint32_t * hash, void ** token, void * baton, svn_diff_datasource_e datasource)
{
	auto linediff = static_cast<SVNLineDiff*>(baton);
	*token = NULL;
	switch (datasource)
	{
	case svn_diff_datasource_original:
		if (linediff->m_bWordDiff)
			NextTokenWords(hash, token, linediff->m_line1pos, linediff->m_line1tokens);
		else
			NextTokenChars(hash, token, linediff->m_line1pos, linediff->m_line1, linediff->m_line1length);
		break;
	case svn_diff_datasource_modified:
		if (linediff->m_bWordDiff)
			NextTokenWords(hash, token, linediff->m_line2pos, linediff->m_line2tokens);
		else
			NextTokenChars(hash, token, linediff->m_line2pos, linediff->m_line2, linediff->m_line2length);
		break;
	}
	return SVN_NO_ERROR;
}

svn_error_t * SVNLineDiff::compare_token(void * baton, void * token1, void * token2, int * compare)
{
	auto linediff = static_cast<SVNLineDiff*>(baton);
	if (linediff->m_bWordDiff)
	{
		auto s1 = static_cast<LPCTSTR>(token1);
		auto s2 = static_cast<LPCTSTR>(token2);
		if (s1 && s2)
		{
			*compare = _tcscmp(s1, s2);
		}
	}
	else
	{
		TCHAR * c1 = (TCHAR *)token1;
		TCHAR * c2 = (TCHAR *)token2;
		if (c1 && c2)
		{
			if (*c1 == *c2)
				*compare = 0;
			else if (*c1 < *c2)
				*compare = -1;
			else
				*compare = 1;
		}
	}
	return SVN_NO_ERROR;
}

void SVNLineDiff::discard_token(void * /*baton*/, void * /*token*/)
{
}

void SVNLineDiff::discard_all_token(void * /*baton*/)
{
}

SVNLineDiff::SVNLineDiff()
	: m_pool(NULL)
	, m_subpool(NULL)
	, m_line1(NULL)
	, m_line1length(0)
	, m_line2(NULL)
	, m_line2length(0)
	, m_line1pos(0)
	, m_line2pos(0)
	, m_bWordDiff(false)
{
	m_pool = svn_pool_create(NULL);
}

SVNLineDiff::~SVNLineDiff()
{
	svn_pool_destroy(m_pool);
};

bool SVNLineDiff::Diff(svn_diff_t **diff, LPCTSTR line1, apr_size_t len1, LPCTSTR line2, apr_size_t len2, bool bWordDiff)
{
	if (m_subpool)
		svn_pool_clear(m_subpool);
	else
		m_subpool = svn_pool_create(m_pool);

	if (m_subpool == NULL)
		return false;

	m_bWordDiff = bWordDiff;
	m_line1 = line1;
	m_line2 = line2;
	m_line1length = len1 ? len1 : _tcslen(m_line1);
	m_line2length = len2 ? len2 : _tcslen(m_line2);

	m_line1pos = 0;
	m_line2pos = 0;
	m_line1tokens.clear();
	m_line2tokens.clear();
	svn_error_t * err = svn_diff_diff_2(diff, this, &SVNLineDiff_vtable, m_subpool);
	if (err)
	{
		svn_error_clear(err);
		svn_pool_clear(m_subpool);
		return false;
	}
	return true;
}

#define ADLER_MOD_BASE 65521
#define ADLER_MOD_BLOCK_SIZE 5552

apr_uint32_t SVNLineDiff::Adler32(apr_uint32_t checksum, const WCHAR *data, apr_size_t len)
{
	const unsigned char * input = (const unsigned char *)data;
	apr_uint32_t s1 = checksum & 0xFFFF;
	apr_uint32_t s2 = checksum >> 16;
	apr_uint32_t b;
	len *= 2;
	apr_size_t blocks = len / ADLER_MOD_BLOCK_SIZE;

	len %= ADLER_MOD_BLOCK_SIZE;

	while (blocks--)
	{
		int count = ADLER_MOD_BLOCK_SIZE;
		while (count--)
		{
			b = *input++;
			s1 += b;
			s2 += s1;
		}

		s1 %= ADLER_MOD_BASE;
		s2 %= ADLER_MOD_BASE;
	}

	while (len--)
	{
		b = *input++;
		s1 += b;
		s2 += s1;
	}

	return ((s2 % ADLER_MOD_BASE) << 16) | (s1 % ADLER_MOD_BASE);
}

bool SVNLineDiff::IsCharWhiteSpace(TCHAR c)
{
	return (c == ' ') || (c == '\t');
}

bool SVNLineDiff::ShowInlineDiff(svn_diff_t* diff)
{
	svn_diff_t* tempdiff = diff;
	apr_size_t diffcounts = 0;
	apr_off_t origsize = 0;
	apr_off_t diffsize = 0;
	while (tempdiff)
	{
		if (tempdiff->type == svn_diff__type_common)
		{
			origsize += tempdiff->original_length;
		}
		else
		{
			diffcounts++;
			diffsize += tempdiff->original_length;
			diffsize += tempdiff->modified_length;
		}
		tempdiff = tempdiff->next;
	}
	return (origsize > 0.5 * diffsize + 1.0 * diffcounts); // Multiplier values may be subject to individual preferences
}
