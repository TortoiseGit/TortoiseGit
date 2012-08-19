// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2003-2008 - TortoiseSVN
// Copyright (C) 2008-2012 - TortoiseGit

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
#include "GitStatus.h"
#include "git2.h"
#include "UnicodeUtils.h"
#include "CreateProcessHelper.h"
#include "FormatMessageWrapper.h"

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
		sheetpage = (CGitPropertyPage*) ((LPPROPSHEETPAGE) lParam)->lParam;
		SetWindowLongPtr (hwnd, GWLP_USERDATA, (LONG_PTR) sheetpage);
		sheetpage->SetHwnd(hwnd);
	}
	else
	{
		sheetpage = (CGitPropertyPage*) GetWindowLongPtr (hwnd, GWLP_USERDATA);
	}

	if (sheetpage != 0L)
		return sheetpage->PageProc(hwnd, uMessage, wParam, lParam);
	else
		return FALSE;
}

UINT CALLBACK PropPageCallbackProc ( HWND /*hwnd*/, UINT uMsg, LPPROPSHEETPAGE ppsp )
{
	// Delete the page before closing.
	if (PSPCB_RELEASE == uMsg)
	{
		CGitPropertyPage* sheetpage = (CGitPropertyPage*) ppsp->lParam;
		if (sheetpage != NULL)
			delete sheetpage;
	}
	return 1;
}

// *********************** CGitPropertyPage *************************

CGitPropertyPage::CGitPropertyPage(const std::vector<stdstring> &newFilenames)
	:filenames(newFilenames)
	,m_bChanged(false)
{
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
			LPNMHDR point = (LPNMHDR)lParam;
			int code = point->code;
			//
			// Respond to notifications.
			//
			if (code == PSN_APPLY && filenames.size() == 1 && m_bChanged)
			{
				do
				{
					CTGitPath path(filenames.front().c_str());
					CString projectTopDir;
					if(!path.HasAdminDir(&projectTopDir) || path.IsDirectory())
						break;

					CTGitPath relatepath;
					CString strpath=path.GetWinPathString();

					if(projectTopDir[projectTopDir.GetLength() - 1] == _T('\\'))
						relatepath.SetFromWin(strpath.Right(strpath.GetLength() - projectTopDir.GetLength()));
					else
						relatepath.SetFromWin(strpath.Right(strpath.GetLength() - projectTopDir.GetLength() - 1));

					CStringA gitdir = CUnicodeUtils::GetMulti(projectTopDir, CP_UTF8);
					git_repository *repository = NULL;
					git_index *index = NULL;

					int ret = git_repository_open(&repository, gitdir.GetBuffer());
					gitdir.ReleaseBuffer();
					if (ret)
						break;

					if (git_repository_index(&index, repository))
					{
						git_repository_free(repository);
						break;
					}

					bool changed = false;

					CStringA pathA = CUnicodeUtils::GetMulti(relatepath.GetGitPathString(), CP_UTF8);
					int idx = git_index_find(index, pathA);
					if (idx >= 0) {
						git_index_entry *e = git_index_get(index, idx);

						if (SendMessage(GetDlgItem(m_hwnd, IDC_ASSUMEVALID), BM_GETCHECK, 0, 0) == BST_CHECKED)
						{
							if (!(e->flags & GIT_IDXENTRY_VALID))
							{
								e->flags |= GIT_IDXENTRY_VALID;
								changed = true;
							}
						}
						else
						{
							if (e->flags & GIT_IDXENTRY_VALID)
							{
								e->flags &= ~GIT_IDXENTRY_VALID;
								changed = true;
							}
						}
						if (SendMessage(GetDlgItem(m_hwnd, IDC_SKIPWORKTREE), BM_GETCHECK, 0, 0) == BST_CHECKED)
						{
							if (!(e->flags_extended & GIT_IDXENTRY_SKIP_WORKTREE))
							{
								e->flags_extended |= GIT_IDXENTRY_SKIP_WORKTREE;
								changed = true;
							}
						}
						else
						{
							if (e->flags_extended & GIT_IDXENTRY_SKIP_WORKTREE)
							{
								e->flags_extended &= ~GIT_IDXENTRY_SKIP_WORKTREE;
								changed = true;
							}
						}
						if (SendMessage(GetDlgItem(m_hwnd, IDC_EXECUTABLE), BM_GETCHECK, 0, 0) == BST_CHECKED)
						{
							if (!(e->mode & 0111))
							{
								e->mode |= 0111;
								changed = true;
							}
						}
						else
						{
							if (e->mode & 0111)
							{
								e->mode &= ~0111;
								changed = true;
							}
						}
						if (changed)
							git_index_add2(index, e);
					}

					if (changed)
					{
						if (!git_index_write(index))
							m_bChanged = false;
					}

					git_index_free(index);
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
			tstring gitCmd = _T(" /command:");
			gitCmd += _T("log /path:\"");
			gitCmd += filenames.front().c_str();
			gitCmd += _T("\"");
			RunCommand(gitCmd);
		}
		break;
	case IDC_SHOWSETTINGS:
		{
			CTGitPath path(filenames.front().c_str());
			CString projectTopDir;
			if(!path.HasAdminDir(&projectTopDir))
				return;

			tstring gitCmd = _T(" /command:");
			gitCmd += _T("settings /path:\"");
			gitCmd += projectTopDir;
			gitCmd += _T("\"");
			RunCommand(gitCmd);
		}
		break;
	case IDC_ASSUMEVALID:
	case IDC_SKIPWORKTREE:
	case IDC_EXECUTABLE:
		m_bChanged = true;
		SendMessage(GetParent(m_hwnd), PSM_CHANGED, (WPARAM)m_hwnd, 0);
		break;
	}
}

void CGitPropertyPage::RunCommand(const tstring& command)
{
	tstring tortoiseProcPath = CPathUtils::GetAppDirectory(g_hmodThisDll) + _T("TortoiseProc.exe");
	if (CCreateProcessHelper::CreateProcessDetached(tortoiseProcPath.c_str(), const_cast<TCHAR*>(command.c_str())))
	{
		// process started - exit
		return;
	}

	MessageBox(NULL, CFormatMessageWrapper(), _T("TortoiseProc launch failed"), MB_OK | MB_ICONINFORMATION);
}

void CGitPropertyPage::Time64ToTimeString(__time64_t time, TCHAR * buf, size_t buflen)
{
	struct tm newtime;
	SYSTEMTIME systime;
	TCHAR timebuf[MAX_STRING_LENGTH];
	TCHAR datebuf[MAX_STRING_LENGTH];

	LCID locale = (WORD)CRegStdDWORD(_T("Software\\TortoiseGit\\LanguageID"), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT));
	locale = MAKELCID(locale, SORT_DEFAULT);

	*buf = '\0';
	if (time)
	{
		_localtime64_s(&newtime, &time);

		systime.wDay = (WORD)newtime.tm_mday;
		systime.wDayOfWeek = (WORD)newtime.tm_wday;
		systime.wHour = (WORD)newtime.tm_hour;
		systime.wMilliseconds = 0;
		systime.wMinute = (WORD)newtime.tm_min;
		systime.wMonth = (WORD)newtime.tm_mon+1;
		systime.wSecond = (WORD)newtime.tm_sec;
		systime.wYear = (WORD)newtime.tm_year+1900;
		if (CRegStdDWORD(_T("Software\\TortoiseGit\\LogDateFormat")) == 1)
			GetDateFormat(locale, DATE_SHORTDATE, &systime, NULL, datebuf, MAX_STRING_LENGTH);
		else
			GetDateFormat(locale, DATE_LONGDATE, &systime, NULL, datebuf, MAX_STRING_LENGTH);
		GetTimeFormat(locale, 0, &systime, NULL, timebuf, MAX_STRING_LENGTH);
		*buf = '\0';
		_tcsncat_s(buf, buflen, timebuf, MAX_STRING_LENGTH-1);
		_tcsncat_s(buf, buflen, _T(", "), MAX_STRING_LENGTH-1);
		_tcsncat_s(buf, buflen, datebuf, MAX_STRING_LENGTH-1);
	}
}

void CGitPropertyPage::InitWorkfileView()
{
	CString username;
	CGit git;
	if (filenames.empty())
		return;

	CTGitPath path(filenames.front().c_str());
	CString ProjectTopDir;

	if(!path.HasAdminDir(&ProjectTopDir))
		return;

	git.SetCurrentDir(ProjectTopDir);
	//can't git.exe when create process
	git.Run(_T("git.exe config user.name"), &username, CP_UTF8);
	CString useremail;
	git.Run(_T("git.exe config user.email"), &useremail, CP_UTF8);
	CString autocrlf;
	git.Run(_T("git.exe config core.autocrlf"), &autocrlf, CP_UTF8);
	CString safecrlf;
	git.Run(_T("git.exe config core.safecrlf"), &safecrlf, CP_UTF8);

	CString branch;
	CString headhash;
	CString remotebranch;

	git.Run(_T("git.exe symbolic-ref HEAD"), &branch, CP_UTF8);
	CString cmd,log;

	if(!branch.IsEmpty())
	{
		if( branch.Find(_T("refs/heads/"),0) >=0 )
		{
			CString remote;
			branch=branch.Mid(11);
			int start=0;
			branch=branch.Tokenize(_T("\n"),start);
			start=0;
			branch=branch.Tokenize(_T("\r"),start);
			cmd.Format(_T("git.exe config branch.%s.merge"),branch);
			git.Run(cmd, &remotebranch, CP_UTF8);
			cmd.Format(_T("git.exe config branch.%s.remote"),branch);
			git.Run(cmd, &remote, CP_UTF8);
			if((!remote.IsEmpty()) && (!remotebranch.IsEmpty()))
			{
				remotebranch=remotebranch.Mid(11);
				remotebranch=remote+_T("/")+remotebranch;
			}
		}
	}

	TCHAR oldpath[MAX_PATH+1];

	::GetCurrentDirectory(MAX_PATH, oldpath);

	::SetCurrentDirectory(ProjectTopDir);

	if (autocrlf.Trim().IsEmpty())
		autocrlf = _T("false");
	if (safecrlf.Trim().IsEmpty())
		safecrlf = _T("false");

	SetDlgItemText(m_hwnd,IDC_CONFIG_USERNAME,username.Trim());
	SetDlgItemText(m_hwnd,IDC_CONFIG_USEREMAIL,useremail.Trim());
	SetDlgItemText(m_hwnd,IDC_CONFIG_AUTOCRLF,autocrlf.Trim());
	SetDlgItemText(m_hwnd,IDC_CONFIG_SAFECRLF,safecrlf.Trim());

	SetDlgItemText(m_hwnd,IDC_SHELL_CURRENT_BRANCH,branch.Trim());
	remotebranch.Trim().Replace(_T("\n"), _T("; "));
	SetDlgItemText(m_hwnd,IDC_SHELL_REMOTE_BRANCH, remotebranch);

	try
	{
		AutoLocker lock(g_Git.m_critGitDllSec);
		g_Git.CheckAndInitDll();
		GitRev revHEAD;
		revHEAD.GetCommit(CString(_T("HEAD")));

		SetDlgItemText(m_hwnd, IDC_HEAD_HASH, revHEAD.m_CommitHash.ToString());
		SetDlgItemText(m_hwnd, IDC_HEAD_SUBJECT, revHEAD.GetSubject());
		SetDlgItemText(m_hwnd, IDC_HEAD_AUTHOR, revHEAD.GetAuthorName());
		SetDlgItemText(m_hwnd, IDC_HEAD_DATE, revHEAD.GetAuthorDate().Format(_T("%Y-%m-%d %H:%M:%S")));

		if (filenames.size() == 1)
		{
			CTGitPath relatepath;
			CString strpath=path.GetWinPathString();

			if(ProjectTopDir[ProjectTopDir.GetLength()-1] == _T('\\'))
			{
				relatepath.SetFromWin( strpath.Right(strpath.GetLength()-ProjectTopDir.GetLength()));
			}
			else
			{
				relatepath.SetFromWin( strpath.Right(strpath.GetLength()-ProjectTopDir.GetLength()-1));
			}

			GitRev revLast;
			if(! relatepath.GetGitPathString().IsEmpty())
			{
				cmd=_T("-z --topo-order -n1 --parents -- \"");
				cmd+=relatepath.GetGitPathString();
				cmd+=_T("\"");

				GIT_LOG handle;
				do
				{
					if(git_open_log(&handle, CUnicodeUtils::GetUTF8(cmd).GetBuffer()))
						break;
					if(git_get_log_firstcommit(handle))
						break;

					GIT_COMMIT commit;
					if (git_get_log_nextcommit(handle, &commit, 0))
						break;

					git_close_log(handle);
					handle = NULL;
					revLast.ParserFromCommit(&commit);
					git_free_commit(&commit);

				}while(0);
				if (handle != NULL) {
					git_close_log(handle);
				}
			}

			SetDlgItemText(m_hwnd, IDC_LAST_HASH, revLast.m_CommitHash.ToString());
			SetDlgItemText(m_hwnd, IDC_LAST_SUBJECT, revLast.GetSubject());
			SetDlgItemText(m_hwnd, IDC_LAST_AUTHOR, revLast.GetAuthorName());
			if (revLast.m_CommitHash.IsEmpty())
				SetDlgItemText(m_hwnd, IDC_LAST_DATE, _T(""));
			else
				SetDlgItemText(m_hwnd, IDC_LAST_DATE, revLast.GetAuthorDate().Format(_T("%Y-%m-%d %H:%M:%S")));

			if (!path.IsDirectory())
			{
				// get assume valid flag
				bool assumevalid = false;
				bool skipworktree = false;
				bool executable = false;
				do
				{
					CStringA gitdir = CUnicodeUtils::GetMulti(ProjectTopDir, CP_UTF8);
					git_repository *repository = NULL;
					git_index *index = NULL;

					int ret = git_repository_open(&repository, gitdir.GetBuffer());
					gitdir.ReleaseBuffer();
					if (ret)
						break;

					if (git_repository_index(&index, repository))
					{
						git_repository_free(repository);
						break;
					}

					CStringA pathA = CUnicodeUtils::GetMulti(relatepath.GetGitPathString(), CP_UTF8);
					int idx = git_index_find(index, pathA);
					if (idx >= 0) {
						git_index_entry *e = git_index_get(index, idx);

						if (e->flags & GIT_IDXENTRY_VALID)
							assumevalid = true;

						if (e->flags_extended & GIT_IDXENTRY_SKIP_WORKTREE)
							skipworktree = true;

						if (e->mode & 0111)
							executable = true;
					}
					else
					{
						// do not show checkboxes for unversioned files
						ShowWindow(GetDlgItem(m_hwnd, IDC_ASSUMEVALID), SW_HIDE);
						ShowWindow(GetDlgItem(m_hwnd, IDC_SKIPWORKTREE), SW_HIDE);
						ShowWindow(GetDlgItem(m_hwnd, IDC_EXECUTABLE), SW_HIDE);
					}

					git_index_free(index);
				} while (0);
				SendMessage(GetDlgItem(m_hwnd, IDC_ASSUMEVALID), BM_SETCHECK, assumevalid ? BST_CHECKED : BST_UNCHECKED, 0);
				SendMessage(GetDlgItem(m_hwnd, IDC_SKIPWORKTREE), BM_SETCHECK, skipworktree ? BST_CHECKED : BST_UNCHECKED, 0);
				SendMessage(GetDlgItem(m_hwnd, IDC_EXECUTABLE), BM_SETCHECK, executable ? BST_CHECKED : BST_UNCHECKED, 0);
			}
			else
			{
				ShowWindow(GetDlgItem(m_hwnd, IDC_ASSUMEVALID), SW_HIDE);
				ShowWindow(GetDlgItem(m_hwnd, IDC_SKIPWORKTREE), SW_HIDE);
				ShowWindow(GetDlgItem(m_hwnd, IDC_EXECUTABLE), SW_HIDE);
			}
		}
		else
		{
			ShowWindow(GetDlgItem(m_hwnd, IDC_ASSUMEVALID), SW_HIDE);
			ShowWindow(GetDlgItem(m_hwnd, IDC_SKIPWORKTREE), SW_HIDE);
			ShowWindow(GetDlgItem(m_hwnd, IDC_EXECUTABLE), SW_HIDE);
		}

	}catch(...)
	{
	}
	::SetCurrentDirectory(oldpath);
}


// CShellExt member functions (needed for IShellPropSheetExt)
STDMETHODIMP CShellExt::AddPages(LPFNADDPROPSHEETPAGE lpfnAddPage, LPARAM lParam)
{
	__try
	{
		return AddPages_Wrap(lpfnAddPage, lParam);
	}
	__except(CCrashReport::Instance().SendReport(GetExceptionInformation()))
	{
	}
	return E_FAIL;
}

STDMETHODIMP CShellExt::AddPages_Wrap(LPFNADDPROPSHEETPAGE lpfnAddPage, LPARAM lParam)
{
	CString ProjectTopDir;

	for (std::vector<stdstring>::iterator I = files_.begin(); I != files_.end(); ++I)
	{
		/*
		GitStatus svn = GitStatus();
		if (svn.GetStatus(CTGitPath(I->c_str())) == (-2))
			return S_OK;			// file/directory not under version control

		if (svn.status->entry == NULL)
			return S_OK;
		*/
		if( CTGitPath(I->c_str()).HasAdminDir(&ProjectTopDir))
			break;
		else
			return S_OK;
	}

	if (files_.empty())
		return S_OK;

	LoadLangDll();
	PROPSHEETPAGE psp;
	SecureZeroMemory(&psp, sizeof(PROPSHEETPAGE));
	HPROPSHEETPAGE hPage;
	CGitPropertyPage *sheetpage = new (std::nothrow) CGitPropertyPage(files_);

	psp.dwSize = sizeof (psp);
	psp.dwFlags = PSP_USEREFPARENT | PSP_USETITLE | PSP_USEICONID | PSP_USECALLBACK;
	psp.hInstance = g_hResInst;
	psp.pszTemplate = MAKEINTRESOURCE(IDD_PROPPAGE);
	psp.pszIcon = MAKEINTRESOURCE(IDI_APPSMALL);
	psp.pszTitle = _T("Git");
	psp.pfnDlgProc = (DLGPROC) PageProc;
	psp.lParam = (LPARAM) sheetpage;
	psp.pfnCallback = PropPageCallbackProc;
	psp.pcRefParent = (UINT*)&g_cRefThisDll;

	hPage = CreatePropertySheetPage (&psp);

	if (hPage != NULL)
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
