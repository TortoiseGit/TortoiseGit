// TortoiseGitMerge - a Diff/Patch program

// Copyright (C) 2012-2013, 2015-2019 - TortoiseGit
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

#include "AppUtils.h"

#define STRIP_LIMIT 10

GitPatch::GitPatch()
	: m_nStrip(0)
	, m_bSuccessfullyPatched(false)
	, m_nRejected(0)
	, m_pProgDlg(nullptr)
	, m_patch()
{
}

GitPatch::~GitPatch()
{
}

int GitPatch::Init(const CString& patchfile, const CString& targetpath, CSysProgressDlg *pPprogDlg)
{
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

	// Read and try to apply patch
	if (m_patch.OpenUnifiedDiffFile(m_patchfile) == FALSE)
	{
		m_errorStr = m_patch.GetErrorMessage();
		m_filePaths.clear();
		return 0;
	}
	if (!ApplyPatches())
	{
		m_filePaths.clear();
		return 0;
	}

	m_pProgDlg = nullptr;

	if ((m_nRejected > (static_cast<int>(m_filePaths.size()) / 3)) && !m_testPath.IsEmpty())
	{
		++m_nStrip;
		for (m_nStrip = 0; m_nStrip < STRIP_LIMIT; ++m_nStrip)
		{
			if (std::any_of(m_filePaths.cbegin(), m_filePaths.cend(), [this](auto& filepath) { return Strip(filepath.path).IsEmpty(); }))
			{
				--m_nStrip;
				break;
			}
		}
	}

	if (m_nStrip == STRIP_LIMIT)
		m_filePaths.clear();
	else if (m_nStrip > 0)
	{
		m_filePaths.clear();
		m_nRejected = 0;

		if (!ApplyPatches())
			m_filePaths.clear();
	}
	return static_cast<int>(m_filePaths.size());
}

bool GitPatch::ApplyPatches()
{
	for (int i = 0; i < m_patch.GetNumberOfFiles(); ++i)
	{
		if (!PatchFile(i, m_targetpath))
			return false;
	}

	return true;
}

bool GitPatch::PatchFile(int nIndex, CString &datapath)
{
	CString sFilePath = m_patch.GetFullPath(datapath, nIndex);
	CString sTempFile = CTempFiles::Instance().GetTempFilePathString();

	PathRejects pr;
	m_testPath = m_patch.GetFilename2(nIndex);
	pr.path = m_patch.GetFilename2(nIndex);
	if (pr.path == L"NUL")
		pr.path = m_patch.GetFilename(nIndex);

	if (m_pProgDlg)
		m_pProgDlg->FormatPathLine(2, IDS_PATCH_PATHINGFILE, static_cast<LPCTSTR>(pr.path));

	//first, do a "dry run" of patching against the file in place...
	if (!m_patch.PatchFile(m_nStrip, nIndex, datapath, sTempFile))
	{
		//patching not successful, so retrieve the
		//base file from version control and try
		//again...
		CString sVersion = m_patch.GetRevision(nIndex);

		CString sBaseFile;
		if ((sVersion.GetLength() >= 7 && wcsncmp(sVersion, GIT_REV_ZERO, sVersion.GetLength()) == 0) || sFilePath == L"NUL")
			sBaseFile = CTempFiles::Instance().GetTempFilePathString();
		else
		{
			if (sVersion.IsEmpty())
			{
				m_errorStr.Format(IDS_ERR_MAINFRAME_FILECONFLICTNOVERSION, static_cast<LPCTSTR>(sFilePath));
				return false; // cannot apply patch which does not apply cleanly w/o git information in patch file.
			}
			sBaseFile = CTempFiles::Instance().GetTempFilePathString();
			if (!CAppUtils::GetVersionedFile(sFilePath, sVersion, sBaseFile, m_pProgDlg))
			{
				m_errorStr.FormatMessage(IDS_ERR_MAINFRAME_FILEVERSIONNOTFOUND, static_cast<LPCTSTR>(sVersion), static_cast<LPCTSTR>(sFilePath));

				return false;
			}
		}

		if (m_pProgDlg)
			m_pProgDlg->FormatPathLine(2, IDS_PATCH_PATHINGFILE, static_cast<LPCTSTR>(pr.path));

		int patchtry = m_patch.PatchFile(m_nStrip, nIndex, datapath, sTempFile, sBaseFile, true);

		if (patchtry == TRUE)
		{
			pr.rejects = 0;
			pr.basePath = sBaseFile;
		}
		else
		{
			pr.rejects = 1;
			// rejectsPath should hold the absolute path to the reject files, but we do not support reject files ATM; also see changes FilePatchesDlg
			pr.rejectsPath = m_patch.GetErrorMessage();
		}

		TRACE(L"comparing %s and %s\nagainst the base file %s\n", static_cast<LPCTSTR>(sTempFile), static_cast<LPCTSTR>(sFilePath), static_cast<LPCTSTR>(sBaseFile));
	}
	else
	{
		//"dry run" was successful, so save the patched file somewhere...
		pr.rejects = 0;
		TRACE(L"comparing %s\nwith the patched result %s\n", static_cast<LPCTSTR>(sFilePath), static_cast<LPCTSTR>(sTempFile));
	}

	pr.resultPath = sTempFile;
	pr.content = true;
	pr.props = false;
	// only add this entry if it hasn't been added already
	if (std::none_of(m_filePaths.crbegin(), m_filePaths.crend(), [&pr](auto& filepath) { return filepath.path.Compare(pr.path) == 0; }))
		m_filePaths.push_back(pr);
	m_nRejected += pr.rejects;

	return true;
}

CString GitPatch::GetPatchRejects(int nIndex) const
{
	if (nIndex < 0)
		return L"";
	if (nIndex < static_cast<int>(m_filePaths.size()))
		return m_filePaths[nIndex].rejectsPath;

	return L"";
}

bool GitPatch::PatchPath(const CString& path)
{
	m_errorStr.Empty();

	m_patchfile.Replace(L'\\', L'/');
	m_targetpath.Replace(L'\\', L'/');

	m_filetopatch = path.Mid(m_targetpath.GetLength()+1);
	m_filetopatch.Replace(L'\\', L'/');

	m_nRejected = 0;

	m_errorStr = L"NOT IMPLEMENTED";
	return false;
}


int GitPatch::GetPatchResult(const CString& sPath, CString& sSavePath, CString& sRejectPath, CString &sBasePath) const
{
	auto it = std::find_if(m_filePaths.cbegin(), m_filePaths.cend(), [this, &sPath](auto& filePath) { return Strip(filePath.path).CompareNoCase(sPath) == 0; });
	if (it != m_filePaths.cend())
	{
		sSavePath = (*it).resultPath;
		sBasePath = (*it).basePath;
		if ((*it).rejects > 0)
			sRejectPath = (*it).rejectsPath;
		else
			sRejectPath.Empty();
		return (*it).rejects;
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
	int trimPos;
	while ((trimPos = upperpath.ReverseFind(L'\\')) > 0)
	{
		upperpath.Truncate(trimPos);
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
		if (GitAdminDir::IsAdminDirPath(subpath))
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
	while ((trimPos = upperpath.ReverseFind(L'\\')) > 0)
	{
		upperpath.Truncate(trimPos);
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
		if (PathIsRelative(temp) || ((temp.GetLength() > 1) && (temp[0] == L'\\') && (temp[1] != L'\\')))
			temp = path + L'\\' + temp;
		if (PathFileExists(temp))
			++matches;
	}
	return matches;
}

int GitPatch::CountDirMatches(const CString& path) const
{
	int matches = 0;
	for (int i=0; i<GetNumberOfFiles(); ++i)
	{
		CString temp = GetStrippedPath(i);
		temp.Replace(L'/', L'\\');
		if (PathIsRelative(temp))
			temp = path + L'\\' + temp;
		// remove the filename
		temp.Truncate(max(0, temp.ReverseFind(L'\\')));
		if (PathFileExists(temp))
			++matches;
	}
	return matches;
}

CString GitPatch::GetStrippedPath(int nIndex) const
{
	if (nIndex < 0)
		return L"";
	if (nIndex < static_cast<int>(m_filePaths.size()))
	{
		CString filepath = Strip(GetFilePath(nIndex));
		return filepath;
	}

	return L"";
}

CString GitPatch::Strip(const CString& filename) const
{
	CString s = filename;
	if ( m_nStrip>0 )
	{
		// Remove windows drive letter "c:"
		if (s.GetLength() > 2 && s[1] == L':')
			s = s.Mid(2);

		for (int nStrip = 1; nStrip <= m_nStrip; ++nStrip)
		{
			// "/home/ts/my-working-copy/dir/file.txt"
			//  "home/ts/my-working-copy/dir/file.txt"
			//       "ts/my-working-copy/dir/file.txt"
			//          "my-working-copy/dir/file.txt"
			//                          "dir/file.txt"
			int p = s.FindOneOf(L"/\\");
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
