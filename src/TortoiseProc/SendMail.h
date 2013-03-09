// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2012-2013 - TortoiseGit

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

#include "GitProgressList.h"
#include "TGitPath.h"

class CSendMail
{
protected:
	static int SendMail(CString &FromName, CString &FromMail, CString &To, CString &CC, CString &subject, CString &body, CStringArray &attachments, bool useMAPI, CString *errortext);
	static int SendMail(const CTGitPath &item, CGitProgressList * instance, CString &FromName, CString &FromMail, CString &To, CString &CC, CString &subject, CString &body, CStringArray &attachments, bool useMAPI);
	CString	m_sSenderName;
	CString	m_sSenderMail;
	CString	m_sTo;
	CString	m_sCC;
	bool	m_bUseMAPI;
	bool	m_bAttachment;

public:
	CSendMail(CString &To, CString &CC, bool m_bAttachment, bool useMAPI);
	~CSendMail(void);
	virtual int Send(CTGitPathList &list, CGitProgressList * instance) = 0;
};

class CSendMailCombineable : public CSendMail
{
public:
	CSendMailCombineable(CString &To, CString &CC, CString &subject, bool bAttachment, bool bCombine, bool useMAPI);
	~CSendMailCombineable(void);

	virtual int Send(CTGitPathList &list, CGitProgressList * instance);

protected:
	virtual int SendAsSingleMail(CTGitPath &path, CGitProgressList * instance);
	virtual int SendAsCombinedMail(CTGitPathList &list, CGitProgressList * instance);

	CString	m_sSubject;
	bool	m_bCombine;
};
