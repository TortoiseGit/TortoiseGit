// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2007 - TortoiseSVN

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
#include ".\colors.h"

CColors::CColors(void) : m_regAdded(_T("Software\\TortoiseGit\\Colors\\Added"), RGB(100, 0, 100))
	, m_regCmd(_T("Software\\TortoiseGit\\Colors\\Cmd"), ::GetSysColor(COLOR_GRAYTEXT))
	, m_regConflict(_T("Software\\TortoiseGit\\Colors\\Conflict"), RGB(255, 0, 0))
	, m_regModified(_T("Software\\TortoiseGit\\Colors\\Modified"), RGB(0, 50, 160))
	, m_regMerged(_T("Software\\TortoiseGit\\Colors\\Merged"), RGB(0, 100, 0))
	, m_regDeleted(_T("Software\\TortoiseGit\\Colors\\Deleted"), RGB(100, 0, 0))
	, m_regLastCommit(_T("Software\\TortoiseGit\\Colors\\LastCommit"), RGB(100, 100, 100))
	, m_regDeletedNode(_T("Software\\TortoiseGit\\Colors\\DeletedNode"), RGB(255, 0, 0))
	, m_regAddedNode(_T("Software\\TortoiseGit\\Colors\\AddedNode"), RGB(0, 255, 0))
	, m_regReplacedNode(_T("Software\\TortoiseGit\\Colors\\ReplacedNode"), RGB(0, 255, 0))
	, m_regRenamedNode(_T("Software\\TortoiseGit\\Colors\\RenamedNode"), RGB(0, 0, 255))
	, m_regLastCommitNode(_T("Software\\TortoiseGit\\Colors\\LastCommitNode"), RGB(200, 200, 200))
	, m_regPropertyChanged(_T("Software\\TortoiseGit\\Colors\\PropertyChanged"), RGB(0, 50, 160))
{
}

CColors::~CColors(void)
{
}

COLORREF CColors::GetColor(Colors col, bool bDefault /*=true*/)
{
	switch (col)
	{
	case Cmd:
		if (bDefault)
			return ::GetSysColor(COLOR_GRAYTEXT);
		return (COLORREF)(DWORD)m_regCmd;
	case Conflict:
		if (bDefault)
			return RGB(255, 0, 0);
		return (COLORREF)(DWORD)m_regConflict;
	case Modified:
		if (bDefault)
			return RGB(0, 50, 160);
		return (COLORREF)(DWORD)m_regModified;
	case Merged:
		if (bDefault)
			return RGB(0, 100, 0);
		return (COLORREF)(DWORD)m_regMerged;
	case Deleted:
		if (bDefault)
			return RGB(100, 0, 0);
		return (COLORREF)(DWORD)m_regDeleted;
	case Added:
		if (bDefault)
			return RGB(100, 0, 100);
		return (COLORREF)(DWORD)m_regAdded;
	case LastCommit:
		if (bDefault)
			return RGB(100, 100, 100);
		return (COLORREF)(DWORD)m_regAdded;
	case DeletedNode:
		if (bDefault)
			return RGB(255, 0, 0);
		return (COLORREF)(DWORD)m_regDeletedNode;
	case AddedNode:
		if (bDefault)
			return RGB(0, 255, 0);
		return (COLORREF)(DWORD)m_regAddedNode;
	case ReplacedNode:
		if (bDefault)
			return RGB(0, 255, 0);
		return (COLORREF)(DWORD)m_regReplacedNode;
	case RenamedNode:
		if (bDefault)
			return RGB(0, 0, 255);
		return (COLORREF)(DWORD)m_regRenamedNode;
	case LastCommitNode:
		if (bDefault)
			return RGB(200, 200, 200);
		return (COLORREF)(DWORD)m_regLastCommitNode;
	case PropertyChanged:
		if (bDefault)
			return RGB(0, 50, 160);
		return (COLORREF)(DWORD)m_regPropertyChanged;
	}
	return RGB(0,0,0);
}

void CColors::SetColor(Colors col, COLORREF cr)
{
	switch (col)
	{
	case Cmd:
		m_regCmd = cr;
		break;
	case Conflict:
		m_regConflict = cr;
		break;
	case Modified:
		m_regModified = cr;
		break;
	case Merged:
		m_regMerged = cr;
		break;
	case Deleted:
		m_regDeleted = cr;
		break;
	case Added:
		m_regAdded = cr;
		break;
	case DeletedNode:
		m_regDeletedNode = cr;
		break;
	case AddedNode:
		m_regAddedNode = cr;
		break;
	case ReplacedNode:
		m_regReplacedNode = cr;
		break;
	case RenamedNode:
		m_regRenamedNode = cr;
		break;
	case LastCommit:
		m_regLastCommit = cr;
		break;
	case PropertyChanged:
		m_regPropertyChanged = cr;
		break;
	default:
		ATLASSERT(false);
	}
}