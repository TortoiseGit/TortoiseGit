// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2003-2008, 2014 - TortoiseSVN
// Copyright (C) 2008-2019 - TortoiseGit

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

#include "ShellExt.h"
#include "gitpropertypage.h"
#include "UnicodeUtils.h"
#include "PathUtils.h"
#include "UnicodeUtils.h"
#include "CreateProcessHelper.h"
#include "FormatMessageWrapper.h"
#include "StringUtils.h"
#include "Git.h"
#include "GitAdminDir.h"

#define MAX_STRING_LENGTH	4096	//should be big enough

// Nonmember function prototypes
BOOL CALLBACK PageProc (HWND, UINT, WPARAM, LPARAM);
UINT CALLBACK PropPageCallbackProc ( HWND hwnd, UINT uMsg, LPPROPSHEETPAGE ppsp );

/////////////////////////////////////////////////////////////////////////////
// Dialog procedures and other callback functions

BOOL CALLBACK PageProc (HWND hwnd, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
	CGitPropertyPage * sheetpage;

	if (uMessage == WM_INITDIALOG)
	{
		sheetpage = reinterpret_cast<CGitPropertyPage*>(reinterpret_cast<LPPROPSHEETPAGE>(lParam)->lParam);
		SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(sheetpage));
		sheetpage->SetHwnd(hwnd);
	}
	else
		sheetpage = reinterpret_cast<CGitPropertyPage*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

	if (sheetpage)
		return sheetpage->PageProc(hwnd, uMessage, wParam, lParam);
	else
		return FALSE;
}

UINT CALLBACK PropPageCallbackProc ( HWND /*hwnd*/, UINT uMsg, LPPROPSHEETPAGE ppsp )
{
	// Delete the page before closing.
	if (PSPCB_RELEASE == uMsg)
	{
		delete reinterpret_cast<CGitPropertyPage*>(ppsp->lParam);
	}
	return 1;
}

// *********************** CGitPropertyPage *************************
const UINT CGitPropertyPage::m_UpdateLastCommit = RegisterWindowMessage(L"TORTOISEGIT_PROP_UPDATELASTCOMMIT");

CGitPropertyPage::CGitPropertyPage(const std::vector<std::wstring>& newFilenames, CString projectTopDir, bool bIsSubmodule)
	: filenames(newFilenames)
	, m_ProjectTopDir(projectTopDir)
	,m_bChanged(false)
	, m_hwnd(nullptr)
	, m_bIsSubmodule(bIsSubmodule)
{
	m_iStripLength = m_ProjectTopDir.GetLength();
	if (m_ProjectTopDir[m_iStripLength - 1] != L'\\')
		++m_iStripLength;
}

CGitPropertyPage::~CGitPropertyPage(void)
{
}

void CGitPropertyPage::SetHwnd(HWND newHwnd)
{
	m_hwnd = newHwnd;
}

BOOL CGitPropertyPage::PageProc (HWND /*hwnd*/, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
	switch (uMessage)
	{
	case WM_INITDIALOG:
		{
			InitWorkfileView();
			return TRUE;
		}
	case WM_NOTIFY:
		{
			auto point = reinterpret_cast<LPNMHDR>(lParam);
			int code = point->code;
			//
			// Respond to notifications.
			//
			if (code == PSN_APPLY && m_bChanged)
			{
				do
				{
					CAutoRepository repository(CUnicodeUtils::GetUTF8(m_ProjectTopDir));
					if (!repository)
						break;

					CAutoIndex index;
					if (git_repository_index(index.GetPointer(), repository))
						break;

					auto assumeValid = static_cast<BOOL>(SendMessage(GetDlgItem(m_hwnd, IDC_ASSUMEVALID), BM_GETCHECK, 0, 0));
					auto skipWorktree = static_cast<BOOL>(SendMessage(GetDlgItem(m_hwnd, IDC_SKIPWORKTREE), BM_GETCHECK, 0, 0));
					auto executable = static_cast<BOOL>(SendMessage(GetDlgItem(m_hwnd, IDC_EXECUTABLE), BM_GETCHECK, 0, 0));
					auto symlink = static_cast<BOOL>(SendMessage(GetDlgItem(m_hwnd, IDC_SYMLINK), BM_GETCHECK, 0, 0));
					if (m_fileStats.submodule != 0)
						executable = symlink = BST_INDETERMINATE; // don't update executable or symlink state if we have at least one submodule

					bool changed = false;

					for (const auto& filename : filenames)
					{
						CTGitPath file;
						file.SetFromWin(CString(filename.c_str()).Mid(m_iStripLength));
						CStringA pathA = CUnicodeUtils::GetMulti(file.GetGitPathString(), CP_UTF8);
						size_t idx;
						if (!git_index_find(&idx, index, pathA))
						{
							git_index_entry *e = const_cast<git_index_entry *>(git_index_get_byindex(index, idx)); // HACK

							if (assumeValid == BST_CHECKED)
							{
								if (!(e->flags & GIT_INDEX_ENTRY_VALID))
								{
									e->flags |= GIT_INDEX_ENTRY_VALID;
									changed = true;
								}
							}
							else if (assumeValid != BST_INDETERMINATE)
							{
								if (e->flags & GIT_INDEX_ENTRY_VALID)
								{
									e->flags &= ~GIT_INDEX_ENTRY_VALID;
									changed = true;
								}
							}
							if (skipWorktree == BST_CHECKED)
							{
								if (!(e->flags_extended & GIT_INDEX_ENTRY_SKIP_WORKTREE))
								{
									e->flags_extended |= GIT_INDEX_ENTRY_SKIP_WORKTREE;
									changed = true;
								}
							}
							else if (skipWorktree != BST_INDETERMINATE)
							{
								if (e->flags_extended & GIT_INDEX_ENTRY_SKIP_WORKTREE)
								{
									e->flags_extended &= ~GIT_INDEX_ENTRY_SKIP_WORKTREE;
									changed = true;
								}
							}
							if (executable == BST_CHECKED)
							{
								if (!(e->mode & 0111))
								{
									e->mode = GIT_FILEMODE_BLOB_EXECUTABLE;
									changed = true;
								}
							}
							else if (executable != BST_INDETERMINATE)
							{
								if (e->mode & 0111)
								{
									e->mode = GIT_FILEMODE_BLOB;
									changed = true;
								}
							}
							if (symlink == BST_CHECKED)
							{
								if ((e->mode & GIT_FILEMODE_LINK) != GIT_FILEMODE_LINK)
								{
									e->mode = GIT_FILEMODE_LINK;
									changed = true;
								}
							}
							else if (symlink != BST_INDETERMINATE)
							{
								if ((e->mode & GIT_FILEMODE_LINK) == GIT_FILEMODE_LINK)
								{
									e->mode = GIT_FILEMODE_BLOB;
									changed = true;
								}
							}
							if (changed)
								git_index_add(index, e);
						}
					}

					if (changed)
					{
						if (!git_index_write(index))
							m_bChanged = false;
					}
				} while (0);
			}
			SetWindowLongPtr (m_hwnd, DWLP_MSGRESULT, FALSE);
			return TRUE;
		}
		case WM_DESTROY:
			return TRUE;

		case WM_COMMAND:
		PageProcOnCommand(wParam);
		break;
	} // switch (uMessage)

	if (uMessage == m_UpdateLastCommit)
	{
		DisplayCommit(reinterpret_cast<git_commit*>(lParam), IDC_LAST_HASH, IDC_LAST_SUBJECT, IDC_LAST_AUTHOR, IDC_LAST_DATE);
		return TRUE;
	}

	return FALSE;
}
void CGitPropertyPage::PageProcOnCommand(WPARAM wParam)
{
	if(HIWORD(wParam) != BN_CLICKED)
		return;

	switch (LOWORD(wParam))
	{
	case IDC_SHOWLOG:
		{
			tstring gitCmd = L" /command:";
			gitCmd += L"log /path:\"";
			gitCmd += filenames.front().c_str();
			gitCmd += L'"';
			if (m_bIsSubmodule)
				gitCmd += L" /submodule";
			RunCommand(gitCmd);
		}
		break;
	case IDC_SHOWSETTINGS:
		{
			tstring gitCmd = L" /command:";
			gitCmd += L"settings /path:\"";
			gitCmd += m_ProjectTopDir;
			gitCmd += L'"';
			RunCommand(gitCmd);
		}
		break;
	case IDC_ASSUMEVALID:
	case IDC_SKIPWORKTREE:
	case IDC_EXECUTABLE:
	case IDC_SYMLINK:
		auto executable = static_cast<BOOL>(SendMessage(GetDlgItem(m_hwnd, IDC_EXECUTABLE), BM_GETCHECK, 0, 0));
		auto symlink = static_cast<BOOL>(SendMessage(GetDlgItem(m_hwnd, IDC_SYMLINK), BM_GETCHECK, 0, 0));
		if (executable == BST_CHECKED)
		{
			EnableWindow(GetDlgItem(m_hwnd, IDC_SYMLINK), FALSE);
			SendMessage(GetDlgItem(m_hwnd, IDC_SYMLINK), BM_SETCHECK, BST_UNCHECKED, 0);
		}
		else
			EnableWindow(GetDlgItem(m_hwnd, IDC_SYMLINK), TRUE);
		if (symlink == BST_CHECKED)
		{
			EnableWindow(GetDlgItem(m_hwnd, IDC_EXECUTABLE), FALSE);
			SendMessage(GetDlgItem(m_hwnd, IDC_EXECUTABLE), BM_SETCHECK, BST_UNCHECKED, 0);
		}
		else
			EnableWindow(GetDlgItem(m_hwnd, IDC_EXECUTABLE), TRUE);
		m_bChanged = true;
		SendMessage(GetParent(m_hwnd), PSM_CHANGED, reinterpret_cast<WPARAM>(m_hwnd), 0);
		break;
	}
}

void CGitPropertyPage::RunCommand(const tstring& command)
{
	tstring tortoiseProcPath = CPathUtils::GetAppDirectory(g_hmodThisDll) + L"TortoiseGitProc.exe";
	if (CCreateProcessHelper::CreateProcessDetached(tortoiseProcPath.c_str(), command.c_str()))
	{
		// process started - exit
		return;
	}

	MessageBox(nullptr, CFormatMessageWrapper(), L"TortoiseGitProc launch failed", MB_OK | MB_ICONERROR);
}

void CGitPropertyPage::Time64ToTimeString(__time64_t time, TCHAR * buf, size_t buflen) const
{
	struct tm newtime;
	SYSTEMTIME systime;

	LCID locale = LOCALE_USER_DEFAULT;
	if (!CRegDWORD(L"Software\\TortoiseGit\\UseSystemLocaleForDates", TRUE, false, HKEY_CURRENT_USER, KEY_WOW64_64KEY))
		locale = MAKELCID(static_cast<WORD>(CRegStdDWORD(L"Software\\TortoiseGit\\LanguageID", MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), false, HKEY_CURRENT_USER, KEY_WOW64_64KEY)), SORT_DEFAULT);

	*buf = '\0';
	if (time)
	{
		TCHAR timebuf[MAX_STRING_LENGTH] = { 0 };
		TCHAR datebuf[MAX_STRING_LENGTH] = { 0 };
		_localtime64_s(&newtime, &time);

		systime.wDay = static_cast<WORD>(newtime.tm_mday);
		systime.wDayOfWeek = static_cast<WORD>(newtime.tm_wday);
		systime.wHour = static_cast<WORD>(newtime.tm_hour);
		systime.wMilliseconds = 0;
		systime.wMinute = static_cast<WORD>(newtime.tm_min);
		systime.wMonth = static_cast<WORD>(newtime.tm_mon) + 1;
		systime.wSecond = static_cast<WORD>(newtime.tm_sec);
		systime.wYear = static_cast<WORD>(newtime.tm_year) + 1900;
		if (CRegStdDWORD(L"Software\\TortoiseGit\\LogDateFormat", 0, false, HKEY_CURRENT_USER, KEY_WOW64_64KEY) == 1)
			GetDateFormat(locale, DATE_SHORTDATE, &systime, nullptr, datebuf, MAX_STRING_LENGTH);
		else
			GetDateFormat(locale, DATE_LONGDATE, &systime, nullptr, datebuf, MAX_STRING_LENGTH);
		GetTimeFormat(locale, 0, &systime, nullptr, timebuf, MAX_STRING_LENGTH);
		*buf = '\0';
		wcsncat_s(buf, buflen, datebuf, MAX_STRING_LENGTH - 1);
		wcsncat_s(buf, buflen, L" ", MAX_STRING_LENGTH - 1);
		wcsncat_s(buf, buflen, timebuf, MAX_STRING_LENGTH - 1);
	}
}

struct TreewalkStruct
{
	const char *folder;
	const char *name;
	git_oid oid;
};

static int TreewalkCB_FindFileRecentCommit(const char *root, const git_tree_entry *entry, void *payload)
{
	auto treewalkstruct = reinterpret_cast<TreewalkStruct*>(payload);
	char folder[MAX_PATH] = {0};
	strcpy_s(folder, root);
	strcat_s(folder, git_tree_entry_name(entry));
	strcat_s(folder, "/");
	if (strstr(treewalkstruct->folder, folder))
		return 0;

	if (!strcmp(treewalkstruct->folder, root))
	{
		if (!strcmp(git_tree_entry_name(entry), treewalkstruct->name))
		{
			git_oid_cpy(&treewalkstruct->oid, git_tree_entry_id(entry));
			return GIT_EUSER;
		}

		return 1;
	}

	return 1;
}

static git_commit* FindFileRecentCommit(git_repository* repository, const CString& path)
{
	CAutoRevwalk walk;
	if (git_revwalk_new(walk.GetPointer(), repository))
		return nullptr;

	CStringA pathA = CUnicodeUtils::GetUTF8(path);
	if (pathA.GetLength() >= MAX_PATH)
		return nullptr;
	const char *pathC = pathA;
	char folder[MAX_PATH] = {0}, file[MAX_PATH] = {0};
	const char *slash = strrchr(pathC, '/');
	if (slash)
	{
		strncpy(folder, pathC, slash - pathC + 1);
		folder[slash - pathC + 1] = '\0';
		strcpy(file, slash + 1);
	}
	else
	{
		folder[0] = '\0';
		strcpy(file, pathC);
	}

	TreewalkStruct treewalkstruct = { folder, file };

	if (git_revwalk_push_head(walk))
		return nullptr;

	git_oid oid;
	CAutoCommit commit;
	while (!git_revwalk_next(&oid, walk))
	{
		if (git_commit_lookup(commit.GetPointer(), repository, &oid))
			return nullptr;

		CAutoTree tree;
		if (git_commit_tree(tree.GetPointer(), commit))
			return nullptr;

		memset(&treewalkstruct.oid.id, 0, sizeof(treewalkstruct.oid.id));
		if (auto ret = git_tree_walk(tree, GIT_TREEWALK_PRE, TreewalkCB_FindFileRecentCommit, &treewalkstruct);  ret < 0 && ret != GIT_EUSER)
			return nullptr;

		// check if file not found
		if (git_oid_iszero(&treewalkstruct.oid))
			return nullptr;

		bool diff = true;
		// for merge point, check if it is different to all parents, if yes then there are real change in the merge point.
		// if no parent then of course it is different
		for (unsigned int i = 0; i < git_commit_parentcount(commit); ++i)
		{
			CAutoCommit commit2;
			if (git_commit_parent(commit2.GetPointer(), commit, i))
				return nullptr;

			CAutoTree tree2;
			if (git_commit_tree(tree2.GetPointer(), commit2))
				return nullptr;

			TreewalkStruct treewalkstruct2 = { folder, file };
			memset(&treewalkstruct2.oid.id, 0, sizeof(treewalkstruct2.oid.id));
			if (auto ret = git_tree_walk(tree2, GIT_TREEWALK_PRE, TreewalkCB_FindFileRecentCommit, &treewalkstruct2); ret < 0 && ret != GIT_EUSER)
				return nullptr;

			if (!git_oid_cmp(&treewalkstruct.oid, &treewalkstruct2.oid))
				diff = false;
			else if (git_revwalk_hide(walk, git_commit_parent_id(commit, i)))
				return nullptr;
		}

		if (diff)
			break;
	}

	return commit.Detach();
}

void CGitPropertyPage::DisplayCommit(const git_commit* commit, UINT hashLabel, UINT subjectLabel, UINT authorLabel, UINT dateLabel)
{
	if (!commit)
	{
		SetDlgItemText(m_hwnd, hashLabel, L"");
		SetDlgItemText(m_hwnd, subjectLabel, L"");
		SetDlgItemText(m_hwnd, authorLabel, L"");
		SetDlgItemText(m_hwnd, dateLabel, L"");
		return;
	}

	int encode = CP_UTF8;
	const char * encodingString = git_commit_message_encoding(commit);
	if (encodingString)
		encode = CUnicodeUtils::GetCPCode(CUnicodeUtils::GetUnicode(encodingString));

	const git_signature * author = git_commit_author(commit);
	CString authorName = CUnicodeUtils::GetUnicode(author->name, encode);

	CString message = CUnicodeUtils::GetUnicode(git_commit_message(commit), encode);

	int start = 0;
	message = message.Tokenize(L"\n", start);

	SetDlgItemText(m_hwnd, hashLabel, CGitHash(git_commit_id(commit)).ToString());
	SetDlgItemText(m_hwnd, subjectLabel, message);
	SetDlgItemText(m_hwnd, authorLabel, authorName);

	CString authorDate;
	Time64ToTimeString(author->when.time, authorDate.GetBufferSetLength(200), 200);
	SetDlgItemText(m_hwnd, dateLabel, authorDate);
}

int CGitPropertyPage::LogThread()
{
	CTGitPath path(filenames.front().c_str());

	CAutoRepository repository(CUnicodeUtils::GetUTF8(m_ProjectTopDir));
	if (!repository)
		return 0;

	CTGitPath relatepath;
	relatepath.SetFromWin(path.GetWinPathString().Mid(m_iStripLength));

	CAutoCommit commit(FindFileRecentCommit(repository, relatepath.GetGitPathString()));
	if (commit)
	{
		SendMessage(m_hwnd, m_UpdateLastCommit, NULL, reinterpret_cast<LPARAM>(static_cast<git_commit*>(commit)));
	}
	else
	{
		SendMessage(m_hwnd, m_UpdateLastCommit, NULL, NULL);
	}

	return 0;
}

void CGitPropertyPage::LogThreadEntry(void *param)
{
	reinterpret_cast<CGitPropertyPage*>(param)->LogThread();
}

void CGitPropertyPage::InitWorkfileView()
{
	if (filenames.empty())
		return;

	CTGitPath path(filenames.front().c_str());

	CAutoRepository repository(CUnicodeUtils::GetUTF8(m_ProjectTopDir));
	if (!repository)
		return;

	CString username;
	CString useremail;
	CString autocrlf;
	CString safecrlf;

	CAutoConfig config(repository);
	if (config)
	{
		config.GetString(L"user.name", username);
		config.GetString(L"user.email", useremail);
		config.GetString(L"core.autocrlf", autocrlf);
		config.GetString(L"core.safecrlf", safecrlf);
	}

	CString branch;
	CString remotebranch;
	CString remoteUrl;
	if (!git_repository_head_detached(repository))
	{
		CAutoReference head;
		if (git_repository_head_unborn(repository))
		{
			git_reference_lookup(head.GetPointer(), repository, "HEAD");
			branch = CUnicodeUtils::GetUnicode(git_reference_symbolic_target(head));
			if (CStringUtils::StartsWith(branch, L"refs/heads/"))
				branch = branch.Mid(static_cast<int>(wcslen(L"refs/heads/")));
		}
		else if (!git_repository_head(head.GetPointer(), repository))
		{
			const char * branchChar = git_reference_shorthand(head);
			branch = CUnicodeUtils::GetUnicode(branchChar);

			const char * branchFullChar = git_reference_name(head);
			CAutoBuf upstreambranchname;
			if (!git_branch_upstream_name(upstreambranchname, repository, branchFullChar))
			{
				remotebranch = CUnicodeUtils::GetUnicode(CStringA(upstreambranchname->ptr, static_cast<int>(upstreambranchname->size)));
				remotebranch = remotebranch.Mid(static_cast<int>(wcslen(L"refs/remotes/")));
				int pos = remotebranch.Find(L'/');
				if (pos > 0)
				{
					CString remoteName;
					remoteName.Format(L"remote.%s.url", static_cast<LPCTSTR>(remotebranch.Left(pos)));
					config.GetString(remoteName, remoteUrl);
				}
			}
		}
	}
	else
		branch = L"detached HEAD";

	if (autocrlf.Trim().IsEmpty())
		autocrlf = L"false";
	if (safecrlf.Trim().IsEmpty())
		safecrlf = L"false";

	SetDlgItemText(m_hwnd,IDC_CONFIG_USERNAME,username.Trim());
	SetDlgItemText(m_hwnd,IDC_CONFIG_USEREMAIL,useremail.Trim());
	SetDlgItemText(m_hwnd,IDC_CONFIG_AUTOCRLF,autocrlf.Trim());
	SetDlgItemText(m_hwnd,IDC_CONFIG_SAFECRLF,safecrlf.Trim());

	SetDlgItemText(m_hwnd,IDC_SHELL_CURRENT_BRANCH,branch.Trim());
	SetDlgItemText(m_hwnd,IDC_SHELL_REMOTE_BRANCH, remotebranch);
	SetDlgItemText(m_hwnd, IDC_SHELL_REMOTE_URL, remoteUrl);

	git_oid oid;
	CAutoCommit HEADcommit;
	if (!git_reference_name_to_id(&oid, repository, "HEAD") && !git_commit_lookup(HEADcommit.GetPointer(), repository, &oid) && HEADcommit)
		DisplayCommit(HEADcommit, IDC_HEAD_HASH, IDC_HEAD_SUBJECT, IDC_HEAD_AUTHOR, IDC_HEAD_DATE);

	m_fileStats.allAreVersionedItems = false;
	do
	{
		CAutoIndex index;
		if (git_repository_index(index.GetPointer(), repository))
			break;

		m_fileStats.allAreVersionedItems = true;
		for (const auto& filename : filenames)
		{
			CTGitPath file;
			file.SetFromWin(CString(filename.c_str()).Mid(m_iStripLength));
			CStringA pathA = CUnicodeUtils::GetMulti(file.GetGitPathString(), CP_UTF8);
			size_t idx;
			if (!git_index_find(&idx, index, pathA))
			{
				const git_index_entry *e = git_index_get_byindex(index, idx);

				if (e->flags & GIT_INDEX_ENTRY_VALID)
					++m_fileStats.assumevalid;

				if (e->flags_extended & GIT_INDEX_ENTRY_SKIP_WORKTREE)
					++m_fileStats.skipworktree;

				if (e->mode & 0111)
					++m_fileStats.executable;

				if ((e->mode & GIT_FILEMODE_COMMIT) == GIT_FILEMODE_COMMIT)
					++m_fileStats.submodule;

				if ((e->mode & GIT_FILEMODE_LINK) == GIT_FILEMODE_LINK)
					++m_fileStats.symlink;
			}
			else
			{
				m_fileStats.allAreVersionedItems = false;
				break;
			}
		}
	} while (0);

	if (m_fileStats.allAreVersionedItems)
	{
		if (m_fileStats.assumevalid != 0 && m_fileStats.assumevalid != filenames.size())
		{
			SendMessage(GetDlgItem(m_hwnd, IDC_ASSUMEVALID), BM_SETSTYLE, static_cast<DWORD>(GetWindowLong(GetDlgItem(m_hwnd, IDC_ASSUMEVALID), GWL_STYLE)) & ~BS_AUTOCHECKBOX | BS_AUTO3STATE, 0);
			SendMessage(GetDlgItem(m_hwnd, IDC_ASSUMEVALID), BM_SETCHECK, BST_INDETERMINATE, 0);
		}
		else
			SendMessage(GetDlgItem(m_hwnd, IDC_ASSUMEVALID), BM_SETCHECK, (m_fileStats.assumevalid == 0) ? BST_UNCHECKED : BST_CHECKED, 0);

		if (m_fileStats.skipworktree != 0 && m_fileStats.skipworktree != filenames.size())
		{
			SendMessage(GetDlgItem(m_hwnd, IDC_SKIPWORKTREE), BM_SETSTYLE, static_cast<DWORD>(GetWindowLong(GetDlgItem(m_hwnd, IDC_SKIPWORKTREE), GWL_STYLE)) & ~BS_AUTOCHECKBOX | BS_AUTO3STATE, 0);
			SendMessage(GetDlgItem(m_hwnd, IDC_SKIPWORKTREE), BM_SETCHECK, BST_INDETERMINATE, 0);
		}
		else
			SendMessage(GetDlgItem(m_hwnd, IDC_SKIPWORKTREE), BM_SETCHECK, (m_fileStats.skipworktree == 0) ? BST_UNCHECKED : BST_CHECKED, 0);

		if (m_fileStats.executable != 0 && m_fileStats.executable != filenames.size())
		{
			SendMessage(GetDlgItem(m_hwnd, IDC_EXECUTABLE), BM_SETSTYLE, static_cast<DWORD>(GetWindowLong(GetDlgItem(m_hwnd, IDC_EXECUTABLE), GWL_STYLE)) & ~BS_AUTOCHECKBOX | BS_AUTO3STATE, 0);
			SendMessage(GetDlgItem(m_hwnd, IDC_EXECUTABLE), BM_SETCHECK, BST_INDETERMINATE, 0);
			EnableWindow(GetDlgItem(m_hwnd, IDC_SYMLINK), TRUE);
		}
		else
		{
			SendMessage(GetDlgItem(m_hwnd, IDC_EXECUTABLE), BM_SETCHECK, (m_fileStats.executable == 0) ? BST_UNCHECKED : BST_CHECKED, 0);
			EnableWindow(GetDlgItem(m_hwnd, IDC_SYMLINK), (m_fileStats.executable == 0) ? TRUE : FALSE);
		}

		if (m_fileStats.symlink != 0 && m_fileStats.symlink != filenames.size())
		{
			SendMessage(GetDlgItem(m_hwnd, IDC_SYMLINK), BM_SETSTYLE, static_cast<DWORD>(GetWindowLong(GetDlgItem(m_hwnd, IDC_SYMLINK), GWL_STYLE)) & ~BS_AUTOCHECKBOX | BS_AUTO3STATE, 0);
			SendMessage(GetDlgItem(m_hwnd, IDC_SYMLINK), BM_SETCHECK, BST_INDETERMINATE, 0);
			EnableWindow(GetDlgItem(m_hwnd, IDC_EXECUTABLE), TRUE);
		}
		else
		{
			SendMessage(GetDlgItem(m_hwnd, IDC_SYMLINK), BM_SETCHECK, (m_fileStats.symlink == 0) ? BST_UNCHECKED : BST_CHECKED, 0);
			EnableWindow(GetDlgItem(m_hwnd, IDC_EXECUTABLE), (m_fileStats.symlink == 0) ? TRUE : FALSE);
		}

		// check this last, so that we hide executable and symlink checkboxes in any case if we have at least one submodule
		if (m_fileStats.submodule != 0)
		{
			ShowWindow(GetDlgItem(m_hwnd, IDC_EXECUTABLE), SW_HIDE);
			ShowWindow(GetDlgItem(m_hwnd, IDC_SYMLINK), SW_HIDE);
		}
	}
	else
	{
		// do not show checkboxes for unversioned files
		ShowWindow(GetDlgItem(m_hwnd, IDC_ASSUMEVALID), SW_HIDE);
		ShowWindow(GetDlgItem(m_hwnd, IDC_SKIPWORKTREE), SW_HIDE);
		ShowWindow(GetDlgItem(m_hwnd, IDC_EXECUTABLE), SW_HIDE);
		ShowWindow(GetDlgItem(m_hwnd, IDC_SYMLINK), SW_HIDE);
	}

	if (filenames.size() == 1 && m_fileStats.allAreVersionedItems)
	{
		SetDlgItemText(m_hwnd, IDC_LAST_SUBJECT, CString(MAKEINTRESOURCE(IDS_LOADING)));
		_beginthread(LogThreadEntry, 0, this);
	}
	else
		ShowWindow(GetDlgItem(m_hwnd, IDC_STATIC_LASTMODIFIED), SW_HIDE);
}


// CShellExt member functions (needed for IShellPropSheetExt)
STDMETHODIMP CShellExt::AddPages(LPFNADDPROPSHEETPAGE lpfnAddPage, LPARAM lParam)
{
	if (files_.empty())
		return S_OK;

	CTGitPath firstFile(files_[0].c_str());

	CString projectTopDir;
	if (!firstFile.HasAdminDir(&projectTopDir))
		return S_OK;

	if (files_.size() == 1 && firstFile.IsWCRoot()) // might be a submodule
	{
		CString parentRepo;
		if (firstFile.IsRegisteredSubmoduleOfParentProject(&parentRepo))
		{
			LoadLangDll();
			PROPSHEETPAGE psp = { 0 };
			HPROPSHEETPAGE hPage;
			CGitPropertyPage* sheetpage = new (std::nothrow) CGitPropertyPage(files_, parentRepo, true);

			if (!sheetpage)
				return E_OUTOFMEMORY;

			psp.dwSize = sizeof(psp);
			psp.dwFlags = PSP_USEREFPARENT | PSP_USETITLE | PSP_USEICONID | PSP_USECALLBACK;
			psp.hInstance = g_hResInst;
			psp.pszTemplate = MAKEINTRESOURCE(IDD_PROPPAGE);
			psp.pszIcon = MAKEINTRESOURCE(IDI_APPSMALL);
			psp.pszTitle = L"Git Submodule";
			psp.pfnDlgProc = reinterpret_cast<DLGPROC>(PageProc);
			psp.lParam = reinterpret_cast<LPARAM>(sheetpage);
			psp.pfnCallback = PropPageCallbackProc;
			psp.pcRefParent = (UINT*)&g_cRefThisDll;

			hPage = CreatePropertySheetPage(&psp);

			if (hPage && !lpfnAddPage(hPage, lParam))
			{
				delete sheetpage;
				DestroyPropertySheetPage(hPage);
			}
		}
	}

	for (const auto& file_ : files_)
	{
		CString currentProjectTopDir;
		if (!CTGitPath(file_.c_str()).HasAdminDir(&currentProjectTopDir) || !CPathUtils::ArePathStringsEqual(projectTopDir, currentProjectTopDir))
			return S_OK;
	}

	LoadLangDll();
	PROPSHEETPAGE psp = { 0 };
	HPROPSHEETPAGE hPage;
	CGitPropertyPage* sheetpage = new (std::nothrow) CGitPropertyPage(files_, projectTopDir, false);

	if (!sheetpage)
		return E_OUTOFMEMORY;

	psp.dwSize = sizeof (psp);
	psp.dwFlags = PSP_USEREFPARENT | PSP_USETITLE | PSP_USEICONID | PSP_USECALLBACK;
	psp.hInstance = g_hResInst;
	psp.pszTemplate = MAKEINTRESOURCE(IDD_PROPPAGE);
	psp.pszIcon = MAKEINTRESOURCE(IDI_APPSMALL);
	psp.pszTitle = L"Git";
	psp.pfnDlgProc = reinterpret_cast<DLGPROC>(PageProc);
	psp.lParam = reinterpret_cast<LPARAM>(sheetpage);
	psp.pfnCallback = PropPageCallbackProc;
	psp.pcRefParent = (UINT*)&g_cRefThisDll;

	hPage = CreatePropertySheetPage (&psp);

	if (hPage)
	{
		if (!lpfnAddPage (hPage, lParam))
		{
			delete sheetpage;
			DestroyPropertySheetPage (hPage);
		}
	}

	return S_OK;
}

STDMETHODIMP CShellExt::ReplacePage (UINT /*uPageID*/, LPFNADDPROPSHEETPAGE /*lpfnReplaceWith*/, LPARAM /*lParam*/)
{
	return E_FAIL;
}
