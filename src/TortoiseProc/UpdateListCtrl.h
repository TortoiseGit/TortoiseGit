// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2012, 2016, 2018 - TortoiseGit

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

// CUpdateListCtrl

class CUpdateListCtrl : public CListCtrl
{
	DECLARE_DYNAMIC(CUpdateListCtrl)

public:
	CUpdateListCtrl();
	virtual ~CUpdateListCtrl();
	enum
	{
		STATUS_NONE,
		STATUS_DOWNLOADING = 0x10000,
		STATUS_FAIL = 0x1,
		STATUS_SUCCESS = 0x2,
		STATUS_IGNORE = 0x4,
		STATUS_MASK   = 0xffff
	};

	class Entry
	{
	public:
		CString m_filename;
		int m_status;

		Entry(CString filename, int status)
		: m_filename(filename)
		, m_status(status)
		{
		}
	};

	CFont				m_boldFont;

protected:
	DECLARE_MESSAGE_MAP()
	afx_msg void OnNMCustomdraw(NMHDR *pNMHDR, LRESULT *pResult);
	void PreSubclassWindow() override;
};
