// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2003-2006 - Stefan Kueng

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
#include "registry.h"
#include "..\TortoiseProc\resource.h"
#include "..\TortoiseProc\AppUtils.h"
#include "PathUtils.h"
#include ".\soundutils.h"
#include "mmsystem.h"

#pragma comment(lib, "Winmm")

CSoundUtils::CSoundUtils(void)
{
}

CSoundUtils::~CSoundUtils(void)
{
}

void CSoundUtils::RegisterTGitSounds()
{
	// create the event labels
	CRegString eventlabelerr = CRegString(_T("AppEvents\\EventLabels\\TGit_Error\\"));
	eventlabelerr = CString(MAKEINTRESOURCE(IDS_ERR_ERROR));
	CRegString eventlabelwarn = CRegString(_T("AppEvents\\EventLabels\\TGit_Warning\\"));
	eventlabelwarn = CString(MAKEINTRESOURCE(IDS_WARN_WARNING));
	CRegString eventlabelnote = CRegString(_T("AppEvents\\EventLabels\\TGit_Notification\\"));
	eventlabelnote = CString(MAKEINTRESOURCE(IDS_WARN_NOTE));
	
	CRegString appscheme = CRegString(_T("AppEvents\\Schemes\\Apps\\TortoiseProc\\"));
	appscheme = _T("TortoiseGit");

	CString apppath = CPathUtils::GetAppDirectory();
	
	CRegistryKey schemenamekey = CRegistryKey(_T("AppEvents\\Schemes\\Names"));
	CStringList schemenames;
	schemenamekey.getSubKeys(schemenames);
	// if the sound scheme has been modified but not save under a different name,
	// the name of the sound scheme is ".current" and not under the names list.
	// so add the .current scheme to the list too
	schemenames.AddHead(_T(".current"));
	POSITION pos;
	for (pos = schemenames.GetHeadPosition(); pos != NULL;)
	{
		CString name = schemenames.GetNext(pos);
		if ((name.CompareNoCase(_T(".none"))!=0)&&(name.CompareNoCase(_T(".nosound"))!=0))
		{
			CString errorkey = _T("AppEvents\\Schemes\\Apps\\TortoiseProc\\TGit_Error\\") + name + _T("\\");
			CRegString errorkeyval = CRegString(errorkey);
			if (((CString)(errorkeyval)).IsEmpty())
			{
				errorkeyval = apppath + _T("TortoiseGit_Error.wav");
			}
			CString warnkey = _T("AppEvents\\Schemes\\Apps\\TortoiseProc\\TGit_Warning\\") + name + _T("\\");
			CRegString warnkeyval = CRegString(warnkey);
			if (((CString)(warnkeyval)).IsEmpty())
			{
				warnkeyval = apppath + _T("TortoiseGit_Warning.wav");
			}
			CString notificationkey = _T("AppEvents\\Schemes\\Apps\\TortoiseProc\\TGit_Notification\\") + name + _T("\\");
			CRegString notificationkeyval = CRegString(notificationkey);
			if (((CString)(notificationkeyval)).IsEmpty())
			{
				notificationkeyval = apppath + _T("TortoiseGit_Notification.wav");
			}
		}		
	}
}

void CSoundUtils::PlayTGitWarning()
{
	PlaySound(_T("TGit_Warning"), NULL, SND_APPLICATION | SND_ASYNC | SND_NODEFAULT);
}

void CSoundUtils::PlayTGitError()
{
	PlaySound(_T("TGit_Error"), NULL, SND_APPLICATION | SND_ASYNC | SND_NODEFAULT);
}

void CSoundUtils::PlayTGitNotification()
{
	PlaySound(_T("TGit_Notification"), NULL, SND_APPLICATION | SND_ASYNC | SND_NODEFAULT);
}