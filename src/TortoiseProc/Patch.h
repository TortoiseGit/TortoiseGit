// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2011,2013 - TortoiseGit

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

#include "hwsmtp.h"
#include "TGitPath.h"

class CPatch
{
public:
	CPatch();
	~CPatch(void);
	int Parser(CString &pathfile);
	int Send(CString &pathfile,CString &To, CString &CC,bool bAttachment, bool useMAPI);

	static int SendPatchesCombined(CTGitPathList &list,CString &To,CString &CC, CString &subject,bool bAttachment, bool useMAPI,CString *errortext);

	CString		m_LastError;
	CString		m_Author;
	CString		m_Date;
	CString		m_Subject;
	CString		m_PathFile;
	CStringA	m_Body;
	CString		m_strBody;

private:
	static int SendMail(CString &To, CString &CC, CString &subject, CString &body, CStringArray &attachments, bool useMAPI, CString *errortext);
};
