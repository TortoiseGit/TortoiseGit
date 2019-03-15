// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2011-2016, 2018-2019 - TortoiseGit
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
#include "Git.h"
#include "resource.h"
#include <afxtaskdialog.h>
#include <WinCrypt.h>

CHooks* CHooks::m_pInstance;
CTGitPath CHooks::m_RootPath;

static CString CalcSHA256(const CString& text)
{
	HCRYPTPROV hProv = 0;
	if (!CryptAcquireContext(&hProv, nullptr, nullptr, PROV_RSA_AES, CRYPT_VERIFYCONTEXT))
		return L"";
	SCOPE_EXIT{ CryptReleaseContext(hProv, 0); };

	HCRYPTHASH hHash = 0;
	if (!CryptCreateHash(hProv, CALG_SHA_256, 0, 0, &hHash))
		return L"";
	SCOPE_EXIT{ CryptDestroyHash(hHash); };

	CStringA textA = CUnicodeUtils::GetUTF8(text);
	if (!CryptHashData(hHash, reinterpret_cast<const BYTE*>(static_cast<LPCSTR>(textA)), textA.GetLength(), 0))
		return L"";

	CString hash;
	BYTE rgbHash[32];
	DWORD cbHash = _countof(rgbHash);
	if (!CryptGetHashParam(hHash, HP_HASHVAL, rgbHash, &cbHash, 0))
		return L"";

	for (DWORD i = 0; i < cbHash; i++)
	{
		BYTE hi = rgbHash[i] >> 4;
		BYTE lo = rgbHash[i] & 0xf;
		hash.AppendChar(hi + (hi > 9 ? 87 : 48));
		hash.AppendChar(lo + (lo > 9 ? 87 : 48));
	}

	return hash;
}

CHooks::CHooks()
{
}

CHooks::~CHooks()
{
};

bool CHooks::Create()
{
	if (!m_pInstance)
		m_pInstance = new CHooks();
	CRegString reghooks(L"Software\\TortoiseGit\\hooks");
	CString strhooks = reghooks;
	ParseHookString(strhooks, false);
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
		if (it->second.bLocal)
			continue;
		strhooks += GetHookTypeString(it->first.htype);
		strhooks += '\n';
		if (!it->second.bEnabled)
			strhooks += '!';
		strhooks += it->first.path.GetWinPathString();
		strhooks += '\n';
		strhooks += it->second.commandline;
		strhooks += '\n';
		strhooks += (it->second.bWait ? L"true" : L"false");
		strhooks += '\n';
		strhooks += (it->second.bShow ? L"show" : L"hide");
		strhooks += '\n';
	}
	CRegString reghooks(L"Software\\TortoiseGit\\hooks");
	reghooks = strhooks;
	if (reghooks.GetLastError())
		return false;

	if (g_Git.m_CurrentDir.IsEmpty() || GitAdminDir::IsBareRepo(g_Git.m_CurrentDir))
		return true;

	// load the .tgitconfig file
	CAutoConfig gitconfig(true);
	git_config_add_file_ondisk(gitconfig, CGit::GetGitPathStringA(g_Git.CombinePath(L".tgitconfig")), GIT_CONFIG_LEVEL_LOCAL, nullptr, FALSE); // this needs to have the second highest priority

	// delete all existing local hooks
	for (const auto& hook : { PROJECTPROPNAME_STARTCOMMITHOOK, PROJECTPROPNAME_PRECOMMITHOOK, PROJECTPROPNAME_POSTCOMMITHOOK, PROJECTPROPNAME_PREPUSHHOOK, PROJECTPROPNAME_POSTPUSHHOOK, PROJECTPROPNAME_PREREBASEHOOK })
	{
		CStringA hookA = CUnicodeUtils::GetUTF8(hook);
		for (const auto& val : { "cmdline", "wait", "show" })
		{
			git_config_delete_entry(gitconfig, hookA + val);
		}
	}

	// now save the local hooks to .tgitconfig
	for (auto& hook : CHooks::Instance())
	{
		if (!hook.second.bLocal)
			continue;

		CStringA sHookPropName;
		switch (hook.first.htype)
		{
		case start_commit_hook:
			sHookPropName = CUnicodeUtils::GetUTF8(PROJECTPROPNAME_STARTCOMMITHOOK);
			break;
		case pre_commit_hook:
			sHookPropName = CUnicodeUtils::GetUTF8(PROJECTPROPNAME_PRECOMMITHOOK);
			break;
		case post_commit_hook:
			sHookPropName = CUnicodeUtils::GetUTF8(PROJECTPROPNAME_POSTCOMMITHOOK);
			break;
		case pre_push_hook:
			sHookPropName = CUnicodeUtils::GetUTF8(PROJECTPROPNAME_PREPUSHHOOK);
			break;
		case post_push_hook:
			sHookPropName = CUnicodeUtils::GetUTF8(PROJECTPROPNAME_POSTPUSHHOOK);
			break;
		case pre_rebase_hook:
			sHookPropName = CUnicodeUtils::GetUTF8(PROJECTPROPNAME_PREREBASEHOOK);
			break;
		}
		git_config_set_string(gitconfig, sHookPropName + "cmdline", CUnicodeUtils::GetUTF8(hook.second.commandline));
		git_config_set_bool(gitconfig, sHookPropName + "wait", hook.second.bWait);
		git_config_set_bool(gitconfig, sHookPropName + "show", hook.second.bShow);

		CRegDWORD reg(hook.second.sRegKey);
		if (!hook.second.bEnabled)
		{
			reg = 0;
			hook.second.bApproved = false;
			hook.second.bStored = true;
		}
		else if (reg.exists() && reg == 0)
		{
			reg.removeValue();
			hook.second.bApproved = false;
			hook.second.bStored = false;
		}
	}

	return true;
}

bool CHooks::Remove(const hookkey &key)
{
	return (erase(key) > 0);
}

void CHooks::Add(hooktype ht, const CTGitPath& Path, LPCTSTR szCmd, bool bWait, bool bShow, bool bEnabled, bool bLocal)
{
	hookkey key;
	key.htype = ht;
	key.local = bLocal;
	key.path = Path;
	hookiterator it = find(key);
	if (it!=end())
		erase(it);

	hookcmd cmd;
	cmd.commandline = szCmd;
	cmd.bWait = bWait;
	cmd.bShow = bShow;
	cmd.bEnabled = bEnabled;
	cmd.bLocal = bLocal;
	insert(std::pair<hookkey, hookcmd>(key, cmd));
}

bool CHooks::SetEnabled(const hookkey& k, bool bEnabled)
{
	auto it = find(k);
	if (it == end())
		return false;
	if (it->second.bEnabled == bEnabled)
		return false;
	it->second.bEnabled = bEnabled;
	return true;
}

CString CHooks::GetHookTypeString(hooktype t)
{
	switch (t)
	{
	case start_commit_hook:
		return L"start_commit_hook";
	case pre_commit_hook:
		return L"pre_commit_hook";
	case post_commit_hook:
		return L"post_commit_hook";
	case pre_push_hook:
		return L"pre_push_hook";
	case post_push_hook:
		return L"post_push_hook";
	case pre_rebase_hook:
		return L"pre_rebase_hook";
	}
	return L"";
}

hooktype CHooks::GetHookType(const CString& s)
{
	if (s.Compare(L"start_commit_hook") == 0)
		return start_commit_hook;
	if (s.Compare(L"pre_commit_hook") == 0)
		return pre_commit_hook;
	if (s.Compare(L"post_commit_hook") == 0)
		return post_commit_hook;
	if (s.Compare(L"pre_push_hook") == 0)
		return pre_push_hook;
	if (s.Compare(L"post_push_hook") == 0)
		return post_push_hook;
	if (s.Compare(L"pre_rebase_hook") == 0)
		return pre_rebase_hook;

	return unknown_hook;
}

void CHooks::SetProjectProperties(const CTGitPath& Path, const ProjectProperties& pp)
{
	m_RootPath = Path;

	CString sHookString;
	if (!pp.sPreCommitHook.IsEmpty())
		sHookString += GetHookTypeString(pre_commit_hook) + L"\n?\n" + pp.sPreCommitHook + L"\n";
	if (!pp.sStartCommitHook.IsEmpty())
		sHookString += GetHookTypeString(start_commit_hook) + L"\n?\n" + pp.sStartCommitHook + L"\n";
	if (!pp.sPostCommitHook.IsEmpty())
		sHookString += GetHookTypeString(post_commit_hook) + L"\n?\n" + pp.sPostCommitHook + L"\n";
	if (!pp.sPrePushHook.IsEmpty())
		sHookString += GetHookTypeString(pre_push_hook) + L"\n?\n" + pp.sPrePushHook + L"\n";
	if (!pp.sPostPushHook.IsEmpty())
		sHookString += GetHookTypeString(post_push_hook) + L"\n?\n" + pp.sPostPushHook + L"\n";
	if (!pp.sPreRebaseHook.IsEmpty())
		sHookString += GetHookTypeString(pre_rebase_hook) + L"\n?\n" + pp.sPreRebaseHook + L"\n";
	ParseHookString(sHookString, true);
}

void CHooks::AddParam(CString& sCmd, const CString& param)
{
	sCmd += L" \"";
	sCmd += param;
	sCmd += L'"';
}

void CHooks::AddPathParam(CString& sCmd, const CTGitPathList& pathList)
{
	CTGitPath temppath = CTempFiles::Instance().GetTempFilePath(true);
	pathList.WriteToFile(temppath.GetWinPathString(), true);
	AddParam(sCmd, temppath.GetWinPathString());
}

void CHooks::AddCWDParam(CString& sCmd, const CString& workingTree)
{
	AddParam(sCmd, workingTree);
}

void CHooks::AddErrorParam(CString& sCmd, const CString& error)
{
	CTGitPath tempPath;
	tempPath = CTempFiles::Instance().GetTempFilePath(true);
	CStringUtils::WriteStringToTextFile(tempPath.GetWinPath(), error);
	AddParam(sCmd, tempPath.GetWinPathString());
}

CTGitPath CHooks::AddMessageFileParam(CString& sCmd, const CString& message)
{
	CTGitPath tempPath;
	tempPath = CTempFiles::Instance().GetTempFilePath(true);
	CStringUtils::WriteStringToTextFile(tempPath.GetWinPath(), message);
	AddParam(sCmd, tempPath.GetWinPathString());
	return tempPath;
}

bool CHooks::StartCommit(HWND hWnd, const CString& workingTree, const CTGitPathList& pathList, CString& message, DWORD& exitcode, CString& error)
{
	auto it = FindItem(start_commit_hook, workingTree);
	if (it == end())
		return false;
	if (!ApproveHook(hWnd, it, exitcode))
	{
		error.LoadString(IDS_USERCANCELLED);
		return true;
	}
	CString sCmd = it->second.commandline;
	sCmd.Replace(L"%root%", m_RootPath.GetWinPathString());
	AddPathParam(sCmd, pathList);
	CTGitPath temppath = AddMessageFileParam(sCmd, message);
	AddCWDParam(sCmd, workingTree);
	exitcode = RunScript(sCmd, workingTree, error, it->second.bWait, it->second.bShow);
	if (!exitcode && !temppath.IsEmpty())
	{
		CStringUtils::ReadStringFromTextFile(temppath.GetWinPathString(), message);
	}
	return true;
}

bool CHooks::PreCommit(HWND hWnd, const CString& workingTree, const CTGitPathList& pathList, CString& message, DWORD& exitcode, CString& error)
{
	auto it = FindItem(pre_commit_hook, workingTree);
	if (it == end())
		return false;
	if (!ApproveHook(hWnd, it, exitcode))
	{
		error.LoadString(IDS_USERCANCELLED);
		return true;
	}
	CString sCmd = it->second.commandline;
	sCmd.Replace(L"%root%", m_RootPath.GetWinPathString());
	AddPathParam(sCmd, pathList);
	CTGitPath temppath = AddMessageFileParam(sCmd, message);
	AddCWDParam(sCmd, workingTree);
	exitcode = RunScript(sCmd, workingTree, error, it->second.bWait, it->second.bShow);
	if (!exitcode && !temppath.IsEmpty())
		CStringUtils::ReadStringFromTextFile(temppath.GetWinPathString(), message);
	return true;
}

bool CHooks::PostCommit(HWND hWnd, const CString& workingTree, bool amend, DWORD& exitcode, CString& error)
{
	auto it = FindItem(post_commit_hook, workingTree);
	if (it == end())
		return false;
	if (!ApproveHook(hWnd, it, exitcode))
	{
		error.LoadString(IDS_USERCANCELLED);
		return true;
	}
	CString sCmd = it->second.commandline;
	sCmd.Replace(L"%root%", m_RootPath.GetWinPathString());
	AddCWDParam(sCmd, workingTree);
	if (amend)
		AddParam(sCmd, L"true");
	else
		AddParam(sCmd, L"false");
	exitcode = RunScript(sCmd, workingTree, error, it->second.bWait, it->second.bShow);
	return true;
}

bool CHooks::PrePush(HWND hWnd, const CString& workingTree, DWORD& exitcode, CString& error)
{
	auto it = FindItem(pre_push_hook, workingTree);
	if (it == end())
		return false;
	if (!ApproveHook(hWnd, it, exitcode))
	{
		error.LoadString(IDS_USERCANCELLED);
		return true;
	}
	CString sCmd = it->second.commandline;
	sCmd.Replace(L"%root%", m_RootPath.GetWinPathString());
	AddErrorParam(sCmd, error);
	AddCWDParam(sCmd, workingTree);
	exitcode = RunScript(sCmd, workingTree, error, it->second.bWait, it->second.bShow);
	return true;
}

bool CHooks::PostPush(HWND hWnd, const CString& workingTree, DWORD& exitcode, CString& error)
{
	auto it = FindItem(post_push_hook, workingTree);
	if (it == end())
		return false;
	if (!ApproveHook(hWnd, it, exitcode))
	{
		error.LoadString(IDS_USERCANCELLED);
		return true;
	}
	CString sCmd = it->second.commandline;
	sCmd.Replace(L"%root%", m_RootPath.GetWinPathString());
	AddErrorParam(sCmd, error);
	AddCWDParam(sCmd, workingTree);
	exitcode = RunScript(sCmd, workingTree, error, it->second.bWait, it->second.bShow);
	return true;
}

bool CHooks::PreRebase(HWND hWnd, const CString& workingTree, const CString& upstream, const CString& rebasedBranch, DWORD& exitcode, CString& error)
{
	auto it = FindItem(pre_rebase_hook, workingTree);
	if (it == end())
		return false;
	if (!ApproveHook(hWnd, it, exitcode))
	{
		error.LoadString(IDS_USERCANCELLED);
		return true;
	}
	CString sCmd = it->second.commandline;
	sCmd.Replace(L"%root%", m_RootPath.GetWinPathString());
	AddParam(sCmd, upstream);
	AddParam(sCmd, rebasedBranch);
	AddErrorParam(sCmd, error);
	AddCWDParam(sCmd, workingTree);
	exitcode = RunScript(sCmd, workingTree, error, it->second.bWait, it->second.bShow);
	return true;
}

bool CHooks::IsHookPresent(hooktype t, const CString& workingTree)
{
	auto it = FindItem(t, workingTree);
	return it != end();
}

hookiterator CHooks::FindItem(hooktype t, const CString& workingTree)
{
	hookkey key;
	CTGitPath path = workingTree;
	bool local = false;
	do
	{
		key.htype = t;
		key.path = path;
		key.local = local;
		auto it = find(key);
		if (it != end() && (it->second.bEnabled || it->second.bLocal))
			return it;
		if (!local)
		{
			local = true;
			continue;
		}
		else
			local = false;
		path = path.GetContainingDirectory();
	} while(!path.IsEmpty());

	/* if this ever gets called with something different than the workingTree root,
	 * recheck whether it is necessary to add a check for "key.path = m_RootPath"
	 * (e.g., for sparse checkouts or other hook types).
	 */
	ATLASSERT(CTGitPath(workingTree).IsWCRoot());

	// look for a script with a path as '*'
	key.htype = t;
	key.path = CTGitPath(L"*");
	key.local = false;
	auto it = find(key);
	if (it != end() && it->second.bEnabled)
	{
		return it;
	}

	return end();
}

void CHooks::ParseHookString(CString strhooks, bool bLocal)
{
	// now fill the map with all the hooks defined in the string
	// the string consists of multiple lines, where one hook script is defined
	// as four lines:
	// line 1: the hook type
	// line 2: path to working copy where to apply the hook script,
	//         if it starts with "!" this hook is disabled (this should provide backward and forward compatibility)
	//         if it starts with "?" this hook is for the current project folder
	// line 3: command line to execute
	// line 4: 'true' or 'false' for waiting for the script to finish
	// line 5: 'show' or 'hide' on how to start the hook script
	hookkey key;
	int pos = 0;
	hookcmd cmd;
	cmd.bLocal = bLocal;
	cmd.bApproved = !bLocal; // user configured scripts are pre-approved
	cmd.bStored = false;
	while ((pos = strhooks.Find(L'\n')) >= 0)
	{
		// line 1
		key.htype = GetHookType(strhooks.Left(pos));
		if (pos + 1 < strhooks.GetLength())
			strhooks = strhooks.Mid(pos + 1);
		else
			strhooks.Empty();
		bool bComplete = false;
		if ((pos = strhooks.Find(L'\n')) >= 0)
		{
			// line 2
			cmd.bEnabled = true;
			if (strhooks[0] == L'!' && pos > 1)
			{
				cmd.bEnabled = false;
				strhooks = strhooks.Mid(static_cast<int>(wcslen(L"!")));
				--pos;
			}
			if (strhooks[0] == L'?' && pos > 0)
				key.path = CTGitPath(m_RootPath);
			else
				key.path = CTGitPath(strhooks.Left(pos));
			if (pos + 1 < strhooks.GetLength())
				strhooks = strhooks.Mid(pos + 1);
			else
				strhooks.Empty();
			if ((pos = strhooks.Find(L'\n')) >= 0)
			{
				// line 3
				cmd.commandline = strhooks.Left(pos);
				if (pos + 1 < strhooks.GetLength())
					strhooks = strhooks.Mid(pos + 1);
				else
					strhooks.Empty();
				if ((pos = strhooks.Find(L'\n')) >= 0)
				{
					// line 4
					cmd.bWait = (strhooks.Left(pos).CompareNoCase(L"true") == 0);
					if (pos + 1 < strhooks.GetLength())
						strhooks = strhooks.Mid(pos + 1);
					else
						strhooks.Empty();
					if ((pos = strhooks.Find(L'\n')) >= 0)
					{
						// line 5
						cmd.bShow = (strhooks.Left(pos).CompareNoCase(L"show") == 0);
						if (pos + 1 < strhooks.GetLength())
							strhooks = strhooks.Mid(pos + 1);
						else
							strhooks.Empty();
						key.local = cmd.bLocal;
						if (cmd.bLocal)
						{
							CString temp;
							temp.Format(L"%s|%d%s", m_RootPath.GetWinPath(), static_cast<int>(key.htype), static_cast<LPCTSTR>(cmd.commandline));

							cmd.sRegKey = L"Software\\TortoiseGit\\approvedhooks\\" + CalcSHA256(temp);
							CRegDWORD reg(cmd.sRegKey, 0);
							cmd.bApproved = (DWORD(reg) != 0);
							cmd.bStored = reg.exists();
							cmd.bEnabled = cmd.bStored ? cmd.bApproved : true;
						}

						bComplete = true;
					}
				}
			}
		}
		if (bComplete)
			m_pInstance->insert(std::pair<hookkey, hookcmd>(key, cmd));
	}
}

DWORD CHooks::RunScript(CString cmd, LPCTSTR currentDir, CString& error, bool bWait, bool bShow)
{
	DWORD exitcode = 0;
	SECURITY_ATTRIBUTES sa = { 0 };
	sa.nLength = sizeof(sa);
	sa.bInheritHandle = TRUE;

	CAutoFile hOut ;
	CAutoFile hRedir;
	CAutoFile hErr;

	// clear the error string
	error.Empty();

	// Create Temp File for redirection
	TCHAR szTempPath[MAX_PATH] = {0};
	TCHAR szOutput[MAX_PATH] = {0};
	TCHAR szErr[MAX_PATH] = {0};
	GetTortoiseGitTempPath(_countof(szTempPath), szTempPath);
	GetTempFileName(szTempPath, L"git", 0, szErr);

	// setup redirection handles
	// output handle must be WRITE mode, share READ
	// redirect handle must be READ mode, share WRITE
	hErr   = CreateFile(szErr, GENERIC_WRITE, FILE_SHARE_READ, &sa, CREATE_ALWAYS, FILE_ATTRIBUTE_TEMPORARY, nullptr);

	if (!hErr)
	{
		error = CFormatMessageWrapper();
		return DWORD(-1);
	}

	hRedir = CreateFile(szErr, GENERIC_READ, FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);

	if (!hRedir)
	{
		error = CFormatMessageWrapper();
		return DWORD(-1);
	}

	GetTempFileName(szTempPath, L"git", 0, szOutput);
	hOut   = CreateFile(szOutput, GENERIC_WRITE, FILE_SHARE_READ, &sa, CREATE_ALWAYS, FILE_ATTRIBUTE_TEMPORARY, nullptr);

	if (!hOut)
	{
		error = CFormatMessageWrapper();
		return DWORD(-1);
	}

	// setup startup info, set std out/err handles
	// hide window
	STARTUPINFO si = { 0 };
	si.cb = sizeof(si);
	si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
	si.hStdOutput = hOut;
	si.hStdError = hErr;
	si.wShowWindow = bShow ? SW_SHOW : SW_HIDE;

	PROCESS_INFORMATION pi = { 0 };
	if (!CreateProcess(nullptr, cmd.GetBuffer(), nullptr, nullptr, TRUE, CREATE_UNICODE_ENVIRONMENT, nullptr, currentDir, &si, &pi))
	{
		const DWORD err = GetLastError();  // preserve the CreateProcess error
		error = CFormatMessageWrapper(err);
		SetLastError(err);
		cmd.ReleaseBuffer();
		return DWORD(-1);
	}
	cmd.ReleaseBuffer();

	CloseHandle(pi.hThread);

	// wait for process to finish, capture redirection and
	// send it to the parent window/console
	if (bWait)
	{
		DWORD dw;
		char buf[256] = { 0 };
		do
		{
			while (ReadFile(hRedir, &buf, sizeof(buf) - 1, &dw, nullptr))
			{
				if (dw == 0)
					break;
				error += CString(CStringA(buf,dw));
			}
			Sleep(150);
		} while (WaitForSingleObject(pi.hProcess, 0) != WAIT_OBJECT_0);

		// perform any final flushing
		while (ReadFile(hRedir, &buf, sizeof(buf) - 1, &dw, nullptr))
		{
			if (dw == 0)
				break;

			error += CString(CStringA(buf, dw));
		}
		WaitForSingleObject(pi.hProcess, INFINITE);
		GetExitCodeProcess(pi.hProcess, &exitcode);
	}
	CloseHandle(pi.hProcess);
	DeleteFile(szOutput);
	DeleteFile(szErr);

	return exitcode;
}

bool CHooks::ApproveHook(HWND hWnd, hookiterator it, DWORD& exitcode)
{
	if (it->second.bApproved || it->second.bStored)
	{
		exitcode = 0;
		return it->second.bApproved;
	}

	CString sQuestion;
	sQuestion.Format(IDS_HOOKS_APPROVE_TASK1, static_cast<LPCWSTR>(it->second.commandline));
	CTaskDialog taskdlg(sQuestion, CString(MAKEINTRESOURCE(IDS_HOOKS_APPROVE_TASK2)), L"TortoiseGit", 0, TDF_USE_COMMAND_LINKS | TDF_ALLOW_DIALOG_CANCELLATION | TDF_POSITION_RELATIVE_TO_WINDOW | TDF_SIZE_TO_CONTENT);
	taskdlg.AddCommandControl(101, CString(MAKEINTRESOURCE(IDS_HOOKS_APPROVE_TASK3)));
	taskdlg.AddCommandControl(102, CString(MAKEINTRESOURCE(IDS_HOOKS_APPROVE_TASK4)));
	taskdlg.SetCommonButtons(TDCBF_CANCEL_BUTTON);
	taskdlg.SetVerificationCheckboxText(CString(MAKEINTRESOURCE(IDS_HOOKS_APPROVE_TASK5)));
	taskdlg.SetVerificationCheckbox(false);
	taskdlg.SetDefaultCommandControl(102);
	taskdlg.SetMainIcon(TD_WARNING_ICON);
	taskdlg.SetFooterText(CString(MAKEINTRESOURCE(IDS_HOOKS_APPROVE_SECURITYHINT)));
	auto ret = taskdlg.DoModal(hWnd);
	if (ret == IDCANCEL)
	{
		exitcode = 1;
		return false;
	}
	bool bApproved = (ret == 101);
	bool bDoNotAskAgain = !!taskdlg.GetVerificationCheckboxState();

	if (bDoNotAskAgain)
	{
		CRegDWORD reg(it->second.sRegKey, 0);
		reg = bApproved ? 1 : 0;
		it->second.bStored = true;
		it->second.bApproved = bApproved;
	}
	exitcode = 0;
	return bApproved;
}
