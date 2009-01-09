// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2008 - TortoiseSVN

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
#include "TortoiseProc.h"
#include "Blame.h"
#include "ProgressDlg.h"
#include "TGitPath.h"
#include "Registry.h"
#include "UnicodeUtils.h"
#include "TempFile.h"

CBlame::CBlame()
{
	m_bCancelled = FALSE;
	m_lowestrev = -1;
	m_highestrev = -1;
	m_nCounter = 0;
	m_nHeadRev = -1;
	m_bNoLineNo = false;
}
CBlame::~CBlame()
{
//	m_progressDlg.Stop();
}

BOOL CBlame::BlameCallback(LONG linenumber, git_revnum_t revision, const CString& author, const CString& date,
						   git_revnum_t merged_revision, const CString& merged_author, const CString& merged_date, const CString& merged_path,
						   const CStringA& line)
{

#if 0
	CStringA infolineA;
	CStringA fulllineA;
	git_revnum_t origrev = revision;

	if (((m_lowestrev < 0)||(m_lowestrev > revision))&&(revision >= 0))
		m_lowestrev = revision;
	if (m_highestrev < revision)
		m_highestrev = revision;

	CStringA dateA(date);
	CStringA authorA(author);
	CStringA pathA(merged_path);
	TCHAR c = ' ';
	if (!merged_author.IsEmpty() && (merged_revision > 0))
	{
		dateA = CStringA(merged_date);
		authorA = CStringA(merged_author);
		revision = merged_revision;
		c = 'G';
		m_bHasMerges = true;
	}

	if (pathA.Find(' ') >= 60)
	{
		// the merge path has spaces in it:
		// TortoiseBlame can't deal with such paths if the space is after
		// the 60 char which is reserved for the path length in the blame file
		// To avoid these problems, we escape the space
		// (not the best solution, but it works)
		pathA.Replace(" ", "%20");
	}
	if (authorA.GetLength() > 30 )
		authorA = authorA.Left(30);
	if (m_bNoLineNo)
		infolineA.Format("%c %6ld %6ld %-30s %-60s %-30s ", c, revision, origrev, (LPCSTR)dateA, (LPCSTR)pathA, (LPCSTR)authorA);
	else
		infolineA.Format("%c %6ld %6ld %6ld %-30s %-60s %-30s ", c, linenumber, revision, origrev, (LPCSTR)dateA, (LPCSTR)pathA, (LPCSTR)authorA);
	fulllineA = line;
	fulllineA.TrimRight("\r\n");
	fulllineA += "\n";
	if (m_saveFile.m_hFile != INVALID_HANDLE_VALUE)
	{
		m_saveFile.WriteString(infolineA);
		m_saveFile.WriteString(fulllineA);
	}
	else
		return FALSE;
#endif
	return TRUE;
}

#if 0
BOOL CBlame::Log(git_revnum_t revision, const CString& /*author*/, const CString& /*date*/, const CString& message, LogChangedPathArray * /*cpaths*/, apr_time_t /*time*/, int /*filechanges*/, BOOL /*copies*/, DWORD /*actions*/, BOOL /*children*/)
{
	m_progressDlg.SetProgress(m_highestrev - revision, m_highestrev);
	if (m_saveLog.m_hFile != INVALID_HANDLE_VALUE)
	{
		CStringA msgutf8 = CUnicodeUtils::GetUTF8(message);
		int length = msgutf8.GetLength();
		m_saveLog.Write(&revision, sizeof(LONG));
		m_saveLog.Write(&length, sizeof(int));
		m_saveLog.Write((LPCSTR)msgutf8, length);
	}
	return TRUE;
}
#endif

BOOL CBlame::Cancel()
{
//	if (m_progressDlg.HasUserCancelled())
//		m_bCancelled = TRUE;
	return m_bCancelled;
}

CString CBlame::BlameToTempFile(const CTGitPath& path, GitRev startrev, GitRev endrev, GitRev pegrev, 
								CString& logfile, const CString& options, BOOL includemerge, 
								BOOL showprogress, BOOL ignoremimetype)
{
#if 0
	// if the user specified to use another tool to show the blames, there's no
	// need to fetch the log later: only TortoiseBlame uses those logs to give 
	// the user additional information for the blame.
	BOOL extBlame = CRegDWORD(_T("Software\\TortoiseGit\\TextBlame"), FALSE);

	CString temp;
	m_sSavePath = CTempFiles::Instance().GetTempFilePath(false).GetWinPathString();
	if (m_sSavePath.IsEmpty())
		return _T("");
	temp = path.GetFileExtension();
	if (!temp.IsEmpty() && !extBlame)
		m_sSavePath += temp;
	if (!m_saveFile.Open(m_sSavePath, CFile::typeText | CFile::modeReadWrite | CFile::modeCreate))
		return _T("");
	CString headline;
	m_bNoLineNo = false;
	headline.Format(_T("%c %-6s %-6s %-6s %-30s %-60s %-30s %-s \n"), ' ', _T("line"), _T("rev"), _T("rev"), _T("date"), _T("path"), _T("author"), _T("content"));
	m_saveFile.WriteString(headline);
	m_saveFile.WriteString(_T("\n"));
	m_progressDlg.SetTitle(IDS_BLAME_PROGRESSTITLE);
	m_progressDlg.SetAnimation(IDR_DOWNLOAD);
	m_progressDlg.SetShowProgressBar(TRUE);
	if (showprogress)
	{
		m_progressDlg.ShowModeless(CWnd::FromHandle(hWndExplorer));
	}
	m_progressDlg.FormatNonPathLine(1, IDS_BLAME_PROGRESSINFO);
	m_progressDlg.FormatNonPathLine(2, IDS_BLAME_PROGRESSINFOSTART);
	m_progressDlg.SetCancelMsg(IDS_BLAME_PROGRESSCANCEL);
	m_progressDlg.SetTime(FALSE);
	m_nHeadRev = endrev;
	if (m_nHeadRev < 0)
		m_nHeadRev = GetHEADRevision(path);
	m_progressDlg.SetProgress(0, m_nHeadRev);

	m_bHasMerges = false;
	BOOL bBlameSuccesful = this->Blame(path, startrev, endrev, pegrev, options, !!ignoremimetype, !!includemerge);
	if ( !bBlameSuccesful && !pegrev.IsValid() )
	{
		// retry with the end rev as peg rev
		if (this->Blame(path, startrev, endrev, endrev, options, !!ignoremimetype, !!includemerge))
		{
			bBlameSuccesful = TRUE;
			pegrev = endrev;
		}
	}
	if (!bBlameSuccesful)
	{
		m_saveFile.Close();
		DeleteFile(m_sSavePath);
		m_sSavePath.Empty();
	}
	else if (!extBlame)
	{
		m_progressDlg.FormatNonPathLine(2, IDS_BLAME_PROGRESSLOGSTART);
		m_progressDlg.SetProgress(0, m_highestrev);
		logfile = CTempFiles::Instance().GetTempFilePath(false).GetWinPathString();
		if (!m_saveLog.Open(logfile, CFile::typeBinary | CFile::modeReadWrite | CFile::modeCreate))
		{
			logfile.Empty();
			return m_sSavePath;
		}
		BOOL bRet = ReceiveLog(CTGitPathList(path), pegrev, m_nHeadRev, m_lowestrev, 0, FALSE, m_bHasMerges, false);
		if (!bRet)
		{
			m_saveLog.Close();
			DeleteFile(logfile);
			logfile.Empty();
		}
		else
		{
			m_saveLog.Close();
		}
	}
	m_progressDlg.Stop();
	if (m_saveFile.m_hFile != INVALID_HANDLE_VALUE)
		m_saveFile.Close();
#endif;
	return m_sSavePath;
}
#if 0
BOOL CBlame::Notify(const CTGitPath& /*path*/, svn_wc_notify_action_t /*action*/, 
					svn_node_kind_t /*kind*/, const CString& /*mime_type*/, 
					svn_wc_notify_state_t /*content_state*/, 
					svn_wc_notify_state_t /*prop_state*/, LONG rev,
					const svn_lock_t * /*lock*/, svn_wc_notify_lock_state_t /*lock_state*/,
					svn_error_t * /*err*/, apr_pool_t * /*pool*/)
{
	m_progressDlg.FormatNonPathLine(2, IDS_BLAME_PROGRESSINFO2, rev, m_nHeadRev);
	m_progressDlg.SetProgress(rev, m_nHeadRev);
	return TRUE;
}
#endif
bool CBlame::BlameToFile(const CTGitPath& path, GitRev startrev, GitRev endrev, GitRev peg, 
						 const CTGitPath& tofile, const CString& options, BOOL ignoremimetype, BOOL includemerge)
{
	CString temp;
	if (!m_saveFile.Open(tofile.GetWinPathString(), CFile::typeText | CFile::modeReadWrite | CFile::modeCreate))
		return false;
	m_bNoLineNo = true;
	m_nHeadRev = endrev;
	if (m_nHeadRev < 0)
		m_nHeadRev = GetHEADRevision(path);

	BOOL bBlameSuccesful = this->Blame(path, startrev, endrev, peg, options, !!ignoremimetype, !!includemerge);
	if ( !bBlameSuccesful && !peg.IsValid() )
	{
		// retry with the end rev as peg rev
		if (this->Blame(path, startrev, endrev, endrev, options, !!ignoremimetype, !!includemerge))
		{
			bBlameSuccesful = TRUE;
			peg = endrev;
		}
	}

	if (!bBlameSuccesful)
	{
		m_saveFile.Close();
		return false;
	}

	if (m_saveFile.m_hFile != INVALID_HANDLE_VALUE)
		m_saveFile.Close();

	return true;
}
