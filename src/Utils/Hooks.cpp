// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2007-2008 - TortoiseSVN

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
#include "Hooks.h"
#include "registry.h"
#include "StringUtils.h"
#include "TempFile.h"
#include "SmartHandle.h"

CHooks* CHooks::m_pInstance;

CHooks::CHooks()
{
}

CHooks::~CHooks()
{
};

bool CHooks::Create()
{
	if (m_pInstance == NULL)
		m_pInstance = new CHooks();
	CRegString reghooks = CRegString(_T("Software\\TortoiseGit\\hooks"));
	CString strhooks = reghooks;
	// now fill the map with all the hooks defined in the string
	// the string consists of multiple lines, where one hook script is defined
	// as four lines:
	// line 1: the hook type
	// line 2: path to working copy where to apply the hook script
	// line 3: command line to execute
	// line 4: 'true' or 'false' for waiting for the script to finish
	// line 5: 'show' or 'hide' on how to start the hook script
	hookkey key;
	int pos = 0;
	hookcmd cmd;
	while ((pos = strhooks.Find('\n')) >= 0)
	{
		// line 1
		key.htype = GetHookType(strhooks.Mid(0, pos));
		if (pos+1 < strhooks.GetLength())
			strhooks = strhooks.Mid(pos+1);
		else
			strhooks.Empty();
		bool bComplete = false;
		if ((pos = strhooks.Find('\n')) >= 0)
		{
			// line 2
			key.path = CTGitPath(strhooks.Mid(0, pos));
			if (pos+1 < strhooks.GetLength())
				strhooks = strhooks.Mid(pos+1);
			else
				strhooks.Empty();
			if ((pos = strhooks.Find('\n')) >= 0)
			{
				// line 3
				cmd.commandline = strhooks.Mid(0, pos);
				if (pos+1 < strhooks.GetLength())
					strhooks = strhooks.Mid(pos+1);
				else
					strhooks.Empty();
				if ((pos = strhooks.Find('\n')) >= 0)
				{
					// line 4
					cmd.bWait = (strhooks.Mid(0, pos).CompareNoCase(_T("true"))==0);
					if (pos+1 < strhooks.GetLength())
						strhooks = strhooks.Mid(pos+1);
					else
						strhooks.Empty();
					if ((pos = strhooks.Find('\n')) >= 0)
					{
						// line 5
						cmd.bShow = (strhooks.Mid(0, pos).CompareNoCase(_T("show"))==0);
						if (pos+1 < strhooks.GetLength())
							strhooks = strhooks.Mid(pos+1);
						else
							strhooks.Empty();
						bComplete = true;
					}
				}
			}
		}
		if (bComplete)
		{
			m_pInstance->insert(std::pair<hookkey, hookcmd>(key, cmd));
		}
	} 
	return true;
}

CHooks& CHooks::Instance()
{
	return *m_pInstance;
}

void CHooks::Destroy()
{
	delete m_pInstance;
}

bool CHooks::Save()
{
	CString strhooks;
	for (hookiterator it = begin(); it != end(); ++it)
	{
		strhooks += GetHookTypeString(it->first.htype);
		strhooks += '\n';
		strhooks += it->first.path.GetWinPathString();
		strhooks += '\n';
		strhooks += it->second.commandline;
		strhooks += '\n';
		strhooks += (it->second.bWait ? _T("true") : _T("false"));
		strhooks += '\n';
		strhooks += (it->second.bShow ? _T("show") : _T("hide"));
		strhooks += '\n';
	}
	CRegString reghooks = CRegString(_T("Software\\TortoiseGit\\hooks"));
	reghooks = strhooks;
	if (reghooks.GetLastError())
		return false;
	return true;
}

bool CHooks::Remove(hookkey key)
{
	return (erase(key) > 0);
}

void CHooks::Add(hooktype ht, const CTGitPath& Path, LPCTSTR szCmd, bool bWait, bool bShow)
{
	hookkey key;
	key.htype = ht;
	key.path = Path;
	hookiterator it = find(key);
	if (it!=end())
		erase(it);

	hookcmd cmd;
	cmd.commandline = szCmd;
	cmd.bWait = bWait;
	cmd.bShow = bShow;
	insert(std::pair<hookkey, hookcmd>(key, cmd));
}

CString CHooks::GetHookTypeString(hooktype t)
{
	switch (t)
	{
	case start_commit_hook:
		return _T("start_commit_hook");
	case pre_commit_hook:
		return _T("pre_commit_hook");
	case post_commit_hook:
		return _T("post_commit_hook");
	case start_update_hook:
		return _T("start_update_hook");
	case pre_update_hook:
		return _T("pre_update_hook");
	case post_update_hook:
		return _T("post_update_hook");
	case pre_push_hook:
		return _T("pre_push_hook");
	case post_push_hook:
		return _T("post_push_hook");
	}
	return _T("");
}

hooktype CHooks::GetHookType(const CString& s)
{
	if (s.Compare(_T("start_commit_hook"))==0)
		return start_commit_hook;
	if (s.Compare(_T("pre_commit_hook"))==0)
		return pre_commit_hook;
	if (s.Compare(_T("post_commit_hook"))==0)
		return post_commit_hook;
	if (s.Compare(_T("start_update_hook"))==0)
		return start_update_hook;
	if (s.Compare(_T("pre_update_hook"))==0)
		return pre_update_hook;
	if (s.Compare(_T("post_update_hook"))==0)
		return post_update_hook;
	if (s.Compare(_T("pre_push_hook"))==0)
		return pre_push_hook;
	if (s.Compare(_T("post_push_hook"))==0)
		return post_push_hook;

	return unknown_hook;
}

void CHooks::AddParam(CString& sCmd, const CString& param)
{
	sCmd += _T(" \"");
	sCmd += param;
	sCmd += _T("\"");
}

void CHooks::AddPathParam(CString& sCmd, const CTGitPathList& pathList)
{
	CTGitPath temppath = CTempFiles::Instance().GetTempFilePath(true);
	pathList.WriteToFile(temppath.GetWinPathString(), true);
	AddParam(sCmd, temppath.GetWinPathString());
}

void CHooks::AddCWDParam(CString& sCmd, const CTGitPathList& pathList)
{
	AddParam(sCmd, pathList.GetCommonRoot().GetDirectory().GetWinPathString());
}

void CHooks::AddDepthParam(CString& sCmd, git_depth_t depth)
{
	CString sTemp;
	sTemp.Format(_T("%d"), depth);
	AddParam(sCmd, sTemp);
}

void CHooks::AddErrorParam(CString& sCmd, const CString& error)
{
	CTGitPath tempPath;
	tempPath = CTempFiles::Instance().GetTempFilePath(true);
	CStringUtils::WriteStringToTextFile(tempPath.GetWinPath(), (LPCTSTR)error);
	AddParam(sCmd, tempPath.GetWinPathString());
}

CTGitPath CHooks::AddMessageFileParam(CString& sCmd, const CString& message)
{
	CTGitPath tempPath;
	tempPath = CTempFiles::Instance().GetTempFilePath(true);
	CStringUtils::WriteStringToTextFile(tempPath.GetWinPath(), (LPCTSTR)message);
	AddParam(sCmd, tempPath.GetWinPathString());
	return tempPath;
}

bool CHooks::StartCommit(const CTGitPathList& pathList, CString& message, DWORD& exitcode, CString& error)
{
	hookiterator it = FindItem(start_commit_hook, pathList);
	if (it == end())
		return false;
	CString sCmd = it->second.commandline;
	AddPathParam(sCmd, pathList);
	CTGitPath temppath = AddMessageFileParam(sCmd, message);
	AddCWDParam(sCmd, pathList);
	exitcode = RunScript(sCmd, pathList.GetCommonRoot().GetDirectory().GetWinPath(), error, it->second.bWait, it->second.bShow);
	if (!exitcode && !temppath.IsEmpty())
	{
		CStringUtils::ReadStringFromTextFile(temppath.GetWinPathString(), message);
	}
	return true;
}

bool CHooks::PreCommit(const CTGitPathList& pathList, git_depth_t depth, const CString& message, DWORD& exitcode, CString& error)
{
	hookiterator it = FindItem(pre_commit_hook, pathList);
	if (it == end())
		return false;
	CString sCmd = it->second.commandline;
	AddPathParam(sCmd, pathList);
	AddDepthParam(sCmd, depth);
	AddMessageFileParam(sCmd, message);
	AddCWDParam(sCmd, pathList);
	exitcode = RunScript(sCmd, pathList.GetCommonRoot().GetDirectory().GetWinPath(), error, it->second.bWait, it->second.bShow);
	return true;
}

bool CHooks::PostCommit(const CTGitPathList& pathList, git_depth_t depth, GitRev rev, const CString& message, DWORD& exitcode, CString& error)
{
	hookiterator it = FindItem(post_commit_hook, pathList);
	if (it == end())
		return false;
	CString sCmd = it->second.commandline;
	AddPathParam(sCmd, pathList);
	AddDepthParam(sCmd, depth);
	AddMessageFileParam(sCmd, message);
	AddParam(sCmd, rev.m_CommitHash.ToString());
	AddErrorParam(sCmd, error);
	AddCWDParam(sCmd, pathList);
	exitcode = RunScript(sCmd, pathList.GetCommonRoot().GetDirectory().GetWinPath(), error, it->second.bWait, it->second.bShow);
	return true;
}

bool CHooks::StartUpdate(const CTGitPathList& pathList, DWORD& exitcode, CString& error)
{
	hookiterator it = FindItem(start_update_hook, pathList);
	if (it == end())
		return false;
	CString sCmd = it->second.commandline;
	AddPathParam(sCmd, pathList);
	AddCWDParam(sCmd, pathList);
	exitcode = RunScript(sCmd, pathList.GetCommonRoot().GetDirectory().GetWinPath(), error, it->second.bWait, it->second.bShow);
	return true;
}

bool CHooks::PreUpdate(const CTGitPathList& pathList, git_depth_t depth, GitRev rev, DWORD& exitcode, CString& error)
{
	hookiterator it = FindItem(pre_update_hook, pathList);
	if (it == end())
		return false;
	CString sCmd = it->second.commandline;
	AddPathParam(sCmd, pathList);
	AddDepthParam(sCmd, depth);
	AddParam(sCmd, rev.m_CommitHash.ToString());
	AddCWDParam(sCmd, pathList);
	exitcode = RunScript(sCmd, pathList.GetCommonRoot().GetDirectory().GetWinPath(), error, it->second.bWait, it->second.bShow);
	return true;
}

bool CHooks::PostUpdate(const CTGitPathList& pathList, git_depth_t depth, GitRev rev, DWORD& exitcode, CString& error)
{
	hookiterator it = FindItem(post_update_hook, pathList);
	if (it == end())
		return false;
	CString sCmd = it->second.commandline;
	AddPathParam(sCmd, pathList);
	AddDepthParam(sCmd, depth);
	AddParam(sCmd, rev.m_CommitHash.ToString());
	AddErrorParam(sCmd, error);
	AddCWDParam(sCmd, pathList);
	exitcode = RunScript(sCmd, pathList.GetCommonRoot().GetDirectory().GetWinPath(), error, it->second.bWait, it->second.bShow);
	return true;
}

bool CHooks::PrePush(const CTGitPathList& pathList,DWORD& exitcode, CString& error)
{
	hookiterator it = FindItem(pre_push_hook, pathList);
	if (it == end())
		return false;
	CString sCmd = it->second.commandline;
	AddPathParam(sCmd, pathList);
	AddErrorParam(sCmd, error);
	AddCWDParam(sCmd, pathList);
	exitcode = RunScript(sCmd, pathList.GetCommonRoot().GetDirectory().GetWinPath(), error, it->second.bWait, it->second.bShow);
	return true;
}

bool CHooks::PostPush(const CTGitPathList& pathList,DWORD& exitcode, CString& error)
{
	hookiterator it = FindItem(post_push_hook, pathList);
	if (it == end())
		return false;
	CString sCmd = it->second.commandline;
	AddPathParam(sCmd, pathList);
	AddErrorParam(sCmd, error);
	AddCWDParam(sCmd, pathList);
	exitcode = RunScript(sCmd, pathList.GetCommonRoot().GetDirectory().GetWinPath(), error, it->second.bWait, it->second.bShow);
	return true;

}
hookiterator CHooks::FindItem(hooktype t, const CTGitPathList& pathList)
{
	hookkey key;
	for (int i=0; i<pathList.GetCount(); ++i)
	{
		CTGitPath path = pathList[i];
		do 
		{
			key.htype = t;
			key.path = path;
			hookiterator it = find(key);
			if (it != end())
			{
				return it;
			}
			path = path.GetContainingDirectory();
		} while(!path.IsEmpty());
	}
	// look for a script with a path as '*'
	key.htype = t;
	key.path = CTGitPath(_T("*"));
	hookiterator it = find(key);
	if (it != end())
	{
		return it;
	}

	return end();
}

DWORD CHooks::RunScript(CString cmd, LPCTSTR currentDir, CString& error, bool bWait, bool bShow)
{
	DWORD exitcode = 0;
	SECURITY_ATTRIBUTES sa;
	SecureZeroMemory(&sa, sizeof(sa));
	sa.nLength = sizeof(sa);
	sa.bInheritHandle = TRUE;

	CAutoFile hOut ;
	CAutoFile hRedir;
	CAutoFile hErr;

	// clear the error string
	error.Empty();

	// Create Temp File for redirection
	TCHAR szTempPath[MAX_PATH];
	TCHAR szOutput[MAX_PATH];
	TCHAR szErr[MAX_PATH];
	GetTempPath(_countof(szTempPath), szTempPath);
	GetTempFileName(szTempPath, _T("git"), 0, szErr);

	// setup redirection handles
	// output handle must be WRITE mode, share READ
	// redirect handle must be READ mode, share WRITE
	hErr   = CreateFile(szErr, GENERIC_WRITE, FILE_SHARE_READ, &sa, CREATE_ALWAYS, FILE_ATTRIBUTE_TEMPORARY,	0);

	if (!hErr)
		return (DWORD)-1;

	hRedir = CreateFile(szErr, GENERIC_READ, FILE_SHARE_WRITE, 0, OPEN_EXISTING, 0, 0);

	if (!hRedir)
		return (DWORD)-1;

	GetTempFileName(szTempPath, _T("git"), 0, szOutput);
	hOut   = CreateFile(szOutput, GENERIC_WRITE, FILE_SHARE_READ, &sa, CREATE_ALWAYS, FILE_ATTRIBUTE_TEMPORARY,	0);

	if (!hOut)
		return (DWORD)-1;

	// setup startup info, set std out/err handles
	// hide window
	STARTUPINFO si;
	SecureZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	if (hOut  != INVALID_HANDLE_VALUE) 
	{
		si.dwFlags     = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
		si.hStdOutput  = hOut;
		si.hStdError   = hErr;
		si.wShowWindow = bShow ? SW_SHOW : SW_HIDE;
	}

	PROCESS_INFORMATION pi;
	SecureZeroMemory(&pi, sizeof(pi));

	DWORD dwFlags = 0;

	if (!CreateProcess(NULL, cmd.GetBuffer(), NULL, NULL, TRUE, dwFlags, NULL, currentDir, &si, &pi)) 
	{
			int err = GetLastError();  // preserve the CreateProcess error
			error = CFormatMessageWrapper(err);
			SetLastError(err);
			cmd.ReleaseBuffer();
			return (DWORD)-1;
	}
	cmd.ReleaseBuffer();

	CloseHandle(pi.hThread);

	// wait for process to finish, capture redirection and
	// send it to the parent window/console
	if (bWait)
	{
		DWORD dw;
		char buf[256];
		do 
		{
			SecureZeroMemory(&buf,sizeof(buf));
			while (ReadFile(hRedir, &buf, sizeof(buf)-1, &dw, NULL)) 
			{
				if (dw == 0) 
					break;
				error += CString(CStringA(buf,dw));
				SecureZeroMemory(&buf,sizeof(buf));
			}
		} while (WaitForSingleObject(pi.hProcess, 0) != WAIT_OBJECT_0);

		// perform any final flushing
		while (ReadFile(hRedir, &buf, sizeof(buf)-1, &dw, NULL)) 
		{
			if (dw == 0) 
				break;

			error += CString(CStringA(buf, dw));
			SecureZeroMemory(&buf,sizeof(buf));
		}
		WaitForSingleObject(pi.hProcess, INFINITE);
		GetExitCodeProcess(pi.hProcess, &exitcode);
	}
	CloseHandle(pi.hProcess);
	DeleteFile(szOutput);
	DeleteFile(szErr);

	return exitcode;
}

