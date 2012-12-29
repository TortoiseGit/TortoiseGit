// TortoiseGitMerge - a Diff/Patch program

// Copyright (C) 2012 - TortoiseGit
// Copyright (C) 2010-2012 - TortoiseSVN

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
#include "resource.h"
#include "GitPatch.h"
#include "UnicodeUtils.h"
#include "SysProgressDlg.h"
#include "DirFileEnum.h"
#include "GitAdminDir.h"
#include "StringUtils.h"

#define STRIP_LIMIT 10

GitPatch::GitPatch()
	: m_nStrip(0)
	, m_bSuccessfullyPatched(false)
	, m_nRejected(0)
	, m_pProgDlg(NULL)
{
}

GitPatch::~GitPatch()
{
}

int GitPatch::Init(const CString& patchfile, const CString& targetpath, CSysProgressDlg *pPprogDlg)
{
	CTGitPath target = CTGitPath(targetpath);
	if (patchfile.IsEmpty() || targetpath.IsEmpty())
	{
		m_errorStr.LoadString(IDS_ERR_PATCHPATHS);
		return 0;
	}

	m_errorStr.Empty();
	m_patchfile = patchfile;
	m_targetpath = targetpath;
	m_testPath.Empty();

	m_patchfile.Replace('\\', '/');
	m_targetpath.Replace('\\', '/');

	if (pPprogDlg)
	{
		pPprogDlg->SetTitle(IDS_APPNAME);
		pPprogDlg->FormatNonPathLine(1, IDS_PATCH_PROGTITLE);
		pPprogDlg->SetShowProgressBar(false);
		pPprogDlg->ShowModeless(AfxGetMainWnd());
		m_pProgDlg = pPprogDlg;
	}

	m_filePaths.clear();
	m_nRejected = 0;
	m_nStrip = 0;

	// TODO: Read and try to apply patch

	m_pProgDlg = NULL;

	if ((m_nRejected > ((int)m_filePaths.size() / 3)) && !m_testPath.IsEmpty())
	{
		m_nStrip++;
		bool found = false;
		for (m_nStrip = 0; m_nStrip < STRIP_LIMIT; ++m_nStrip)
		{
			for (std::vector<PathRejects>::iterator it = m_filePaths.begin(); it != m_filePaths.end(); ++it)
			{
				if (Strip(it->path).IsEmpty())
				{
					found = true;
					m_nStrip--;
					break;
				}
			}
			if (found)
				break;
		}
	}

	if (m_nStrip == STRIP_LIMIT)
		m_filePaths.clear();
	else if (m_nStrip > 0)
	{
		m_filePaths.clear();
		m_nRejected = 0;

		// apply patch
#if 0
		if (err)
		{
			m_filePaths.clear();
		}
#endif
	}
	return (int)m_filePaths.size();
}
bool GitPatch::PatchPath(const CString& path)
{
	m_errorStr.Empty();

	m_patchfile.Replace('\\', '/');
	m_targetpath.Replace('\\', '/');

	m_filetopatch = path.Mid(m_targetpath.GetLength()+1);
	m_filetopatch.Replace('\\', '/');

	m_nRejected = 0;

	m_errorStr = _T("NOT IMPLEMENTED");
	return false;
}


int GitPatch::GetPatchResult(const CString& sPath, CString& sSavePath, CString& sRejectPath) const
{
	for (std::vector<PathRejects>::const_iterator it = m_filePaths.begin(); it != m_filePaths.end(); ++it)
	{
		if (Strip(it->path).CompareNoCase(sPath)==0)
		{
			sSavePath = it->resultPath;
			if (it->rejects > 0)
				sRejectPath = it->rejectsPath;
			else
				sRejectPath.Empty();
			return it->rejects;
		}
	}
	return -1;
}

CString GitPatch::CheckPatchPath(const CString& path)
{
	// first check if the path already matches
	if (CountMatches(path) > (GetNumberOfFiles() / 3))
		return path;

	CSysProgressDlg progress;
	CString tmp;
	progress.SetTitle(IDS_PATCH_SEARCHPATHTITLE);
	progress.SetShowProgressBar(false);
	tmp.LoadString(IDS_PATCH_SEARCHPATHLINE1);
	progress.SetLine(1, tmp);
	progress.ShowModeless(AfxGetMainWnd());

	// now go up the tree and try again
	CString upperpath = path;
	while (upperpath.ReverseFind('\\')>0)
	{
		upperpath = upperpath.Left(upperpath.ReverseFind('\\'));
		progress.SetLine(2, upperpath, true);
		if (progress.HasUserCancelled())
			return path;
		if (CountMatches(upperpath) > (GetNumberOfFiles()/3))
			return upperpath;
	}
	// still no match found. So try sub folders
	bool isDir = false;
	CString subpath;
	CDirFileEnum filefinder(path);
	while (filefinder.NextFile(subpath, &isDir))
	{
		if (progress.HasUserCancelled())
			return path;
		if (!isDir)
			continue;
		if (g_GitAdminDir.IsAdminDirPath(subpath))
			continue;
		progress.SetLine(2, subpath, true);
		if (CountMatches(subpath) > (GetNumberOfFiles()/3))
			return subpath;
	}

	// if a patch file only contains newly added files
	// we can't really find the correct path.
	// But: we can compare paths strings without the filenames
	// and check if at least those match
	upperpath = path;
	while (upperpath.ReverseFind('\\')>0)
	{
		upperpath = upperpath.Left(upperpath.ReverseFind('\\'));
		progress.SetLine(2, upperpath, true);
		if (progress.HasUserCancelled())
			return path;
		if (CountDirMatches(upperpath) > (GetNumberOfFiles()/3))
			return upperpath;
	}

	return path;
}

int GitPatch::CountMatches(const CString& path) const
{
	int matches = 0;
	for (int i=0; i<GetNumberOfFiles(); ++i)
	{
		CString temp = GetStrippedPath(i);
		temp.Replace('/', '\\');
		if ((PathIsRelative(temp)) ||
			((temp.GetLength() > 1) && (temp[0]=='\\') && (temp[1]!='\\')) )
			temp = path + _T("\\")+ temp;
		if (PathFileExists(temp))
			matches++;
	}
	return matches;
}

int GitPatch::CountDirMatches(const CString& path) const
{
	int matches = 0;
	for (int i=0; i<GetNumberOfFiles(); ++i)
	{
		CString temp = GetStrippedPath(i);
		temp.Replace('/', '\\');
		if (PathIsRelative(temp))
			temp = path + _T("\\")+ temp;
		// remove the filename
		temp = temp.Left(temp.ReverseFind('\\'));
		if (PathFileExists(temp))
			matches++;
	}
	return matches;
}

CString GitPatch::GetStrippedPath(int nIndex) const
{
	if (nIndex < 0)
		return _T("");
	if (nIndex < (int)m_filePaths.size())
	{
		CString filepath = Strip(GetFilePath(nIndex));
		return filepath;
	}

	return _T("");
}

CString GitPatch::Strip(const CString& filename) const
{
	CString s = filename;
	if ( m_nStrip>0 )
	{
		// Remove windows drive letter "c:"
		if (s.GetLength()>2 && s[1]==':')
		{
			s = s.Mid(2);
		}

		for (int nStrip=1;nStrip<=m_nStrip;nStrip++)
		{
			// "/home/ts/my-working-copy/dir/file.txt"
			//  "home/ts/my-working-copy/dir/file.txt"
			//       "ts/my-working-copy/dir/file.txt"
			//          "my-working-copy/dir/file.txt"
			//                          "dir/file.txt"
			int p = s.FindOneOf(_T("/\\"));
			if (p < 0)
			{
				s.Empty();
				break;
			}
			s = s.Mid(p+1);
		}
	}
	return s;
}

bool GitPatch::RemoveFile(const CString& /*path*/)
{
	// Delete file in Git
	// not necessary now, because TGit doesn't support the "missing file" status
	return true;
}
