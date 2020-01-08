// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2009-2020 - TortoiseGit

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
#include "stdafx.h"
#include "gittype.h"
#include "Git.h"
#include "GitRevRefBrowser.h"
#include "StringUtils.h"
#include "GitMailmap.h"

GitRevRefBrowser::GitRevRefBrowser() : GitRev()
{
}

void GitRevRefBrowser::Clear()
{
	GitRev::Clear();
	m_Description.Empty();
	m_UpstreamRef.Empty();
}

int GitRevRefBrowser::GetGitRevRefMap(MAP_REF_GITREVREFBROWSER& map, int mergefilter, CString& err, std::function<bool(const CString& refName)> filterCallback)
{
	err.Empty();
	MAP_STRING_STRING descriptions;
	g_Git.GetBranchDescriptions(descriptions);

	CString args;
	switch (mergefilter)
	{
	case 1:
		args = L" --merged HEAD";
		break;
	case 2:
		args = L" --no-merged HEAD";
		break;
	}

	CString cmd;
	cmd.Format(L"git.exe for-each-ref%s --format=\"%%(refname)%%04 %%(objectname)%%04 %%(upstream)%%04 %%(subject)%%04 %%(authorname)%%04 %%(authoremail)%%04 %%(authordate:raw)%%04 %%(creator)%%04 %%(creatordate:raw)%%03\"", static_cast<LPCTSTR>(args));
	CString allRefs;
	if (g_Git.Run(cmd, &allRefs, &err, CP_UTF8))
		return -1;

	int linePos = 0;
	CString singleRef;
	CGitMailmap mailmap;
	while (!(singleRef = allRefs.Tokenize(L"\03", linePos)).IsEmpty())
	{
		singleRef.TrimLeft(L"\r\n");
		int valuePos = 0;
		CString refName = singleRef.Tokenize(L"\04", valuePos);

		if (refName.IsEmpty() || (filterCallback && !filterCallback(refName)))
			continue;

		GitRevRefBrowser ref;
		ref.m_CommitHash = CGitHash::FromHexStrTry(singleRef.Tokenize(L"\04", valuePos).Trim()); if (valuePos < 0) continue;
		ref.m_UpstreamRef = singleRef.Tokenize(L"\04", valuePos).Trim(); if (valuePos < 0) continue;
		ref.m_Subject = singleRef.Tokenize(L"\04", valuePos).Trim(); if (valuePos < 0) continue;
		ref.m_AuthorName = singleRef.Tokenize(L"\04", valuePos).Trim(); if (valuePos < 0) continue;
		CString email = singleRef.Tokenize(L"\04", valuePos).Trim().Trim(L"<>"); if (valuePos < 0) continue;
		CString date = singleRef.Tokenize(L"\04", valuePos).Trim();
		ref.m_AuthorDate = StrToInt(date);
		if (ref.m_AuthorName.IsEmpty())
		{
			ref.m_AuthorName = singleRef.Tokenize(L"\04", valuePos).Trim(); if (valuePos < 0) continue;
			email = ref.m_AuthorName.Mid(ref.m_AuthorName.Find(L" <") + static_cast<int>(wcslen(L" <")));
			email.Truncate(max(0, email.Find(L'>')));
			ref.m_AuthorName.Truncate(max(0, ref.m_AuthorName.Find(L" <")));
			date = singleRef.Tokenize(L"\04", valuePos).Trim();
			ref.m_AuthorDate = StrToInt(date);
		}
		if (mailmap)
			ref.m_AuthorName = mailmap.TranslateAuthor(ref.m_AuthorName, email);

		if (CStringUtils::StartsWith(refName, L"refs/heads/"))
			ref.m_Description = descriptions[refName.Mid(static_cast<int>(wcslen(L"refs/heads/")))];

		map.emplace(refName, ref);
	}

	return 0;
}
