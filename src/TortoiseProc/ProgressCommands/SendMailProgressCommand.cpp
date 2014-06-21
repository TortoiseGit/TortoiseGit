// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2013-2014 - TortoiseGit

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

#include "stdafx.h"
#include "SendMailProgressCommand.h"
#include "ShellUpdater.h"

bool SendMailProgressCommand::Run(CGitProgressList* list, CString& sWindowTitle, int& /*m_itemCountTotal*/, int& /*m_itemCount*/)
{
	ASSERT(m_SendMail);
	list->SetWindowTitle(IDS_PROGRS_TITLE_SENDMAIL, g_Git.CombinePath(m_targetPathList.GetCommonRoot().GetUIPathString()), sWindowTitle);
	//SetBackgroundImage(IDI_ADD_BKG);
	list->ReportCmd(CString(MAKEINTRESOURCE(IDS_PROGRS_CMD_SENDMAIL)));

	return m_SendMail->Send(m_targetPathList, list) == 0;
}
