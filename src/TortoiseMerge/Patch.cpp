// TortoiseGitMerge - a Diff/Patch program

// Copyright (C) 2009-2013, 2015-2019 - TortoiseGit
// Copyright (C) 2012-2013 - Sven Strickroth <email@cs-ware.de>
// Copyright (C) 2004-2009,2011-2014 - TortoiseSVN

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
#include "UnicodeUtils.h"
#include "DirFileEnum.h"
#include "TortoiseMerge.h"
#include "GitAdminDir.h"
#include "Patch.h"
#include "StringUtils.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CPatch::CPatch(void)
	: m_nStrip(0)
	, m_UnicodeType(CFileTextLines::AUTOTYPE)
{
}

CPatch::~CPatch(void)
{
	FreeMemory();
}

void CPatch::FreeMemory()
{
	m_arFileDiffs.clear();
}

BOOL CPatch::ParsePatchFile(CFileTextLines &PatchLines)
{
	CString sLine;
	EOL ending = EOL_NOENDING;

	int state = 0;
	int nIndex = 0;
	std::unique_ptr<Chunks> chunks;
	std::unique_ptr<Chunk> chunk;
	int nAddLineCount = 0;
	int nRemoveLineCount = 0;
	int nContextLineCount = 0;
	std::map<CString, int> filenamesToPatch;
	for ( ; nIndex < PatchLines.GetCount(); ++nIndex)
	{
		sLine = PatchLines.GetAt(nIndex);
		ending = PatchLines.GetLineEnding(nIndex);
		if (ending != EOL_NOENDING)
			ending = EOL_AUTOLINE;

		switch (state)
		{
			case 0:
			{
				// diff
				if (CStringUtils::StartsWith(sLine, L"diff "))
				{
					if (chunks)
					{
						//this is a new file diff, so add the last one to
						//our array.
						if (!chunks->chunks.empty())
							m_arFileDiffs.emplace_back(std::move(chunks));
					}
					chunks = std::make_unique<Chunks>();
					state = 1;
					break;
				}
			}
			// fallthrough!
			case 1:
			{
				//index
				if (CStringUtils::StartsWith(sLine, L"index "))
				{
					int dotstart = sLine.Find(L"..", static_cast<int>(wcslen(L"index ")));
					if (dotstart > 0 && chunks)
					{
						chunks->sRevision = sLine.Mid(static_cast<int>(wcslen(L"index ")), dotstart - static_cast<int>(wcslen(L"index ")));
						int end = sLine.Find(L' ', dotstart + 2);
						if (end > 0)
							chunks->sRevision2 = sLine.Mid(dotstart + 2, end - (dotstart + 2));
					}
					break;
				}

				//---
				if (CStringUtils::StartsWith(sLine, L"--- "))
				{
					if (state == 0)
					{
						if (chunks)
						{
							//this is a new file diff, so add the last one to
							//our array.
							if (!chunks->chunks.empty())
								m_arFileDiffs.emplace_back(std::move(chunks));
						}
						chunks = std::make_unique<Chunks>();
					}

					sLine = sLine.Mid(static_cast<int>(wcslen(L"---"))); //remove the "---"
					sLine =sLine.Trim();
					//at the end of the filepath there's a revision number...
					int bracket = sLine.ReverseFind('(');
					if (bracket < 0)
					// some patch files can have another '(' char, especially ones created in Chinese OS
						bracket = sLine.ReverseFind(0xff08);

					if (bracket < 0)
					{
						if (chunks->sFilePath.IsEmpty())
							chunks->sFilePath = sLine.Trim();
					}
					else
						chunks->sFilePath = sLine.Left(bracket-1).Trim();

					if (chunks->sFilePath.Find('\t')>=0)
						chunks->sFilePath = chunks->sFilePath.Left(chunks->sFilePath.Find('\t'));
					if (CStringUtils::StartsWith(chunks->sFilePath, L"\"") && CStringUtils::EndsWith(chunks->sFilePath, L'"'))
						chunks->sFilePath=chunks->sFilePath.Mid(1, chunks->sFilePath.GetLength() - 2);
					if (CStringUtils::StartsWith(chunks->sFilePath, L"a/"))
						chunks->sFilePath=chunks->sFilePath.Mid(static_cast<int>(wcslen(L"a/")));

					if (CStringUtils::StartsWith(chunks->sFilePath, L"b/"))
						chunks->sFilePath=chunks->sFilePath.Mid(static_cast<int>(wcslen(L"b/")));


					chunks->sFilePath.Replace(L'/', L'\\');

					if (chunks->sFilePath == L"\\dev\\null" || chunks->sFilePath == L"/dev/null")
						chunks->sFilePath  = L"NUL";

					state = 3;
				}
				if (state == 0)
				{
					if (CStringUtils::StartsWith(sLine, L"@@"))
					{
						if (chunks)
						{
							nIndex--;
							state = 4;
						}
						else
							break;
					}
				}
			}
			break;

			case 3:
			{
				// +++
				if (!CStringUtils::StartsWith(sLine, L"+++"))
				{
					// no starting "+++" found
					m_sErrorMessage.Format(IDS_ERR_PATCH_NOADDFILELINE, nIndex);
					goto errorcleanup;
				}
				sLine = sLine.Mid(static_cast<int>(wcslen(L"---"))); //remove the "---"
				sLine =sLine.Trim();

				//at the end of the filepath there's a revision number...
				int bracket = sLine.ReverseFind('(');
				if (bracket < 0)
				// some patch files can have another '(' char, especially ones created in Chinese OS
					bracket = sLine.ReverseFind(0xff08);

				if (bracket < 0)
					chunks->sFilePath2 = sLine.Trim();
				else
					chunks->sFilePath2 = sLine.Left(bracket-1).Trim();
				if (chunks->sFilePath2.Find('\t')>=0)
					chunks->sFilePath2 = chunks->sFilePath2.Left(chunks->sFilePath2.Find('\t'));
				if (CStringUtils::StartsWith(chunks->sFilePath2, L"\"") && chunks->sFilePath2.ReverseFind(L'"') == chunks->sFilePath2.GetLength() - 1)
					chunks->sFilePath2=chunks->sFilePath2.Mid(1, chunks->sFilePath2.GetLength() - 2);
				if (CStringUtils::StartsWith(chunks->sFilePath2, L"a/"))
					chunks->sFilePath2=chunks->sFilePath2.Mid(static_cast<int>(wcslen(L"a/")));

				if (CStringUtils::StartsWith(chunks->sFilePath2, L"b/"))
					chunks->sFilePath2=chunks->sFilePath2.Mid(static_cast<int>(wcslen(L"b/")));

				chunks->sFilePath2.Replace(L'/', L'\\');
				chunks->sFilePath2.Replace(L'/', L'\\');

				if (chunks->sFilePath2 == L"\\dev\\null" || chunks->sFilePath2 == L"/dev/null")
					chunks->sFilePath2  = L"NUL";

				++state;
			}
			break;

			case 4:
			{
				//start of a new chunk
				if (!CStringUtils::StartsWith(sLine, L"@@"))
				{
					//chunk doesn't start with "@@"
					//so there's garbage in between two file diffs
					state = 0;
					chunk.reset();
					chunks.reset();
					break;		//skip the garbage
				}

				//@@ -xxx,xxx +xxx,xxx @@
				sLine = sLine.Mid(static_cast<int>(wcslen(L"@@")));
				sLine = sLine.Trim();
				chunk = std::make_unique<Chunk>();
				CString sRemove = sLine.Left(sLine.Find(' '));
				CString sAdd = sLine.Mid(sLine.Find(' '));
				chunk->lRemoveStart = abs(_wtol(sRemove));
				if (sRemove.Find(',')>=0)
				{
					sRemove = sRemove.Mid(sRemove.Find(',')+1);
					chunk->lRemoveLength = _wtol(sRemove);
				}
				else
				{
					chunk->lRemoveStart = 0;
					chunk->lRemoveLength = (_wtol(sRemove));
				}
				chunk->lAddStart = _wtol(sAdd);
				if (sAdd.Find(',')>=0)
				{
					sAdd = sAdd.Mid(sAdd.Find(',')+1);
					chunk->lAddLength = _wtol(sAdd);
				}
				else
				{
					chunk->lAddStart = 1;
					chunk->lAddLength = _wtol(sAdd);
				}
				++state;
			}
			break;

			case 5: //[ |+|-] <sourceline>
			{
				//this line is either a context line (with a ' ' in front)
				//a line added (with a '+' in front)
				//or a removed line (with a '-' in front)
				TCHAR type;
				if (sLine.IsEmpty())
					type = ' ';
				else
					type = sLine.GetAt(0);
				if (type == ' ')
				{
					//it's a context line - we don't use them here right now
					//but maybe in the future the patch algorithm can be
					//extended to use those in case the file to patch has
					//already changed and no base file is around...
					chunk->arLines.Add(RemoveUnicodeBOM(sLine.Mid(static_cast<int>(wcslen(L" ")))));
					chunk->arLinesStates.Add(PATCHSTATE_CONTEXT);
					chunk->arEOLs.push_back(ending);
					++nContextLineCount;
				}
				else if (type == '\\')
				{
					//it's a context line (sort of):
					//warnings start with a '\' char (e.g. "\ No newline at end of file")
					//so just ignore this...
				}
				else if (type == '-')
				{
					//a removed line
					chunk->arLines.Add(RemoveUnicodeBOM(sLine.Mid(static_cast<int>(wcslen(L"-")))));
					chunk->arLinesStates.Add(PATCHSTATE_REMOVED);
					chunk->arEOLs.push_back(ending);
					++nRemoveLineCount;
				}
				else if (type == '+')
				{
					//an added line
					chunk->arLines.Add(RemoveUnicodeBOM(sLine.Mid(static_cast<int>(wcslen(L"+")))));
					chunk->arLinesStates.Add(PATCHSTATE_ADDED);
					chunk->arEOLs.push_back(ending);
					++nAddLineCount;
				}
				else
				{
					//none of those lines! what the hell happened here?
					m_sErrorMessage.Format(IDS_ERR_PATCH_UNKNOWNLINETYPE, nIndex);
					goto errorcleanup;
				}
				if ((chunk->lAddLength == (nAddLineCount + nContextLineCount)) &&
					chunk->lRemoveLength == (nRemoveLineCount + nContextLineCount))
				{
					//chunk is finished
					if (chunks)
						chunks->chunks.emplace_back(std::move(chunk));
					chunk.reset();
					nAddLineCount = 0;
					nContextLineCount = 0;
					nRemoveLineCount = 0;
					state = 0;
				}
			}
		break;
		default:
			ASSERT(FALSE);
		} // switch (state)
	} // for ( ;nIndex<m_PatchLines.GetCount(); nIndex++)
	if (chunk)
	{
		m_sErrorMessage.LoadString(IDS_ERR_PATCH_CHUNKMISMATCH);
		goto errorcleanup;
	}
	if (chunks)
		m_arFileDiffs.emplace_back(chunks.release());

	for (size_t i = 0; i < m_arFileDiffs.size(); ++i)
	{
		if (filenamesToPatch[m_arFileDiffs[i]->sFilePath] > 1 && m_arFileDiffs[i]->sFilePath != L"NUL")
		{
			m_sErrorMessage.Format(IDS_ERR_PATCH_FILENAMENOTUNIQUE, static_cast<LPCTSTR>(m_arFileDiffs[i]->sFilePath));
			FreeMemory();
			return FALSE;
		}
		++filenamesToPatch[m_arFileDiffs[i]->sFilePath];
		if (m_arFileDiffs[i]->sFilePath != m_arFileDiffs[i]->sFilePath2)
		{
			if (filenamesToPatch[m_arFileDiffs[i]->sFilePath2] > 1 && m_arFileDiffs[i]->sFilePath2 != L"NUL")
			{
				m_sErrorMessage.Format(IDS_ERR_PATCH_FILENAMENOTUNIQUE, static_cast<LPCTSTR>(m_arFileDiffs[i]->sFilePath));
				FreeMemory();
				return FALSE;
			}
			++filenamesToPatch[m_arFileDiffs[i]->sFilePath2];
		}
	}

	return TRUE;

errorcleanup:
	FreeMemory();
	return FALSE;
}

BOOL CPatch::OpenUnifiedDiffFile(const CString& filename)
{
#ifndef GTEST_INCLUDE_GTEST_GTEST_H_
	CCrashReport::Instance().AddFile2(filename, nullptr, L"unified diff file", CR_AF_MAKE_FILE_COPY);
#endif

	CFileTextLines PatchLines;
	if (!PatchLines.Load(filename))
	{
		m_sErrorMessage = PatchLines.GetErrorString();
		return FALSE;
	}
	FreeMemory();

	//now we got all the lines of the patch file
	//in our array - parsing can start...
	return ParsePatchFile(PatchLines);
}

CString CPatch::GetFilename(int nIndex)
{
	if (nIndex < 0 || nIndex >= static_cast<int>(m_arFileDiffs.size()))
		return L"";

	return Strip(m_arFileDiffs[nIndex]->sFilePath);
}

CString CPatch::GetRevision(int nIndex)
{
	if (nIndex < 0 || nIndex >= static_cast<int>(m_arFileDiffs.size()))
		return L"";

	return m_arFileDiffs[nIndex]->sRevision;
}

CString CPatch::GetFilename2(int nIndex)
{
	if (nIndex < 0 || nIndex >= static_cast<int>(m_arFileDiffs.size()))
		return L"";

	return Strip(m_arFileDiffs[nIndex]->sFilePath2);
}

CString CPatch::GetRevision2(int nIndex)
{
	if (nIndex < 0 || nIndex >= static_cast<int>(m_arFileDiffs.size()))
		return L"";

	return m_arFileDiffs[nIndex]->sRevision2;
}

int CPatch::PatchFile(const int strip, int nIndex, const CString& sPatchPath, const CString& sSavePath, const CString& sBaseFile, const bool force)
{
	m_nStrip = strip;
	CString sPath = GetFullPath(sPatchPath, nIndex);
	if (PathIsDirectory(sPath))
	{
		m_sErrorMessage.Format(IDS_ERR_PATCH_INVALIDPATCHFILE, static_cast<LPCTSTR>(sPath));
		return FALSE;
	}
	if (nIndex < 0)
	{
		m_sErrorMessage.Format(IDS_ERR_PATCH_FILENOTINPATCH, static_cast<LPCTSTR>(sPath));
		return FALSE;
	}

	if (!force && sPath == L"NUL" && PathFileExists(GetFullPath(sPatchPath, nIndex, 1)))
		return FALSE;

	if (GetFullPath(sPatchPath, nIndex, 1) == L"NUL" && !PathFileExists(sPath))
		return 2;

	CString sLine;
	CString sPatchFile = sBaseFile.IsEmpty() ? sPath : sBaseFile;
#ifndef GTEST_INCLUDE_GTEST_GTEST_H_
	if (PathFileExists(sPatchFile))
	{
		CCrashReport::Instance().AddFile2(sPatchFile, nullptr, L"File to patch", CR_AF_MAKE_FILE_COPY);
	}
#endif
	CFileTextLines PatchLines;
	CFileTextLines PatchLinesResult;
	PatchLines.Load(sPatchFile);
	PatchLinesResult = PatchLines;  //.Copy(PatchLines);
	PatchLines.CopySettings(&PatchLinesResult);

	auto chunks = m_arFileDiffs[nIndex].get();

	for (size_t i = 0; i < chunks->chunks.size(); ++i)
	{
		auto chunk = chunks->chunks[i].get();
		LONG lRemoveLine = chunk->lRemoveStart;
		LONG lAddLine = chunk->lAddStart;
		for (int j = 0; j < chunk->arLines.GetCount(); ++j)
		{
			CString sPatchLine = chunk->arLines.GetAt(j);
			EOL ending = chunk->arEOLs[j];
			if ((m_UnicodeType != CFileTextLines::UTF8)&&(m_UnicodeType != CFileTextLines::UTF8BOM))
			{
				if ((PatchLines.GetUnicodeType()==CFileTextLines::UTF8)||(m_UnicodeType == CFileTextLines::UTF8BOM))
				{
					// convert the UTF-8 contents in CString sPatchLine into a CStringA
					sPatchLine = CUnicodeUtils::GetUnicode(CStringA(sPatchLine));
				}
			}
			int nPatchState = static_cast<int>(chunk->arLinesStates.GetAt(j));
			switch (nPatchState)
			{
			case PATCHSTATE_REMOVED:
				{
					if ((lAddLine > PatchLines.GetCount())||(PatchLines.GetCount()==0))
					{
						m_sErrorMessage.FormatMessage(IDS_ERR_PATCH_DOESNOTMATCH, L"", static_cast<LPCTSTR>(sPatchLine));
						return FALSE;
					}
					if (lAddLine == 0)
						lAddLine = 1;
					if ((sPatchLine.Compare(PatchLines.GetAt(lAddLine-1))!=0)&&(!HasExpandedKeyWords(sPatchLine)))
					{
						m_sErrorMessage.FormatMessage(IDS_ERR_PATCH_DOESNOTMATCH, static_cast<LPCTSTR>(sPatchLine), static_cast<LPCTSTR>(PatchLines.GetAt(lAddLine-1)));
						return FALSE;
					}
					if (lAddLine > PatchLines.GetCount())
					{
						m_sErrorMessage.FormatMessage(IDS_ERR_PATCH_DOESNOTMATCH, static_cast<LPCTSTR>(sPatchLine), L"");
						return FALSE;
					}
					PatchLines.RemoveAt(lAddLine-1);
				}
				break;
			case PATCHSTATE_ADDED:
				{
					if (lAddLine == 0)
						lAddLine = 1;
					// check context after insertions in order to avoid double insertions
					bool insertOk = !(lAddLine < PatchLines.GetCount());
					int k = j;
					for (; k < chunk->arLines.GetCount(); ++k)
					{
						if (static_cast<int>(chunk->arLinesStates.GetAt(k)) == PATCHSTATE_ADDED)
							continue;
						if (PatchLines.GetCount() >= lAddLine && chunk->arLines.GetAt(k).Compare(PatchLines.GetAt(lAddLine - 1)) == 0)
							insertOk = true;
						else
							break;
					}

					if (insertOk)
					{
						PatchLines.InsertAt(lAddLine-1, sPatchLine, ending);
						++lAddLine;
					}
					else
					{
						if (k >= chunk->arLines.GetCount())
							k = j;
						m_sErrorMessage.FormatMessage(IDS_ERR_PATCH_DOESNOTMATCH, static_cast<LPCTSTR>(PatchLines.GetAt(lAddLine) - 1), static_cast<LPCTSTR>(chunk->arLines.GetAt(k)));
						return FALSE;
					}
				}
				break;
			case PATCHSTATE_CONTEXT:
				{
					if (lAddLine > PatchLines.GetCount())
					{
						m_sErrorMessage.FormatMessage(IDS_ERR_PATCH_DOESNOTMATCH, L"", static_cast<LPCTSTR>(sPatchLine));
						return FALSE;
					}
					if (lAddLine == 0)
						++lAddLine;
					if (lRemoveLine == 0)
						++lRemoveLine;
					if ((sPatchLine.Compare(PatchLines.GetAt(lAddLine-1))!=0) &&
						(!HasExpandedKeyWords(sPatchLine)) &&
						(lRemoveLine <= PatchLines.GetCount()) &&
						(sPatchLine.Compare(PatchLines.GetAt(lRemoveLine-1))!=0))
					{
						if ((lAddLine < PatchLines.GetCount())&&(sPatchLine.Compare(PatchLines.GetAt(lAddLine))==0))
							++lAddLine;
						else if (((lAddLine + 1) < PatchLines.GetCount())&&(sPatchLine.Compare(PatchLines.GetAt(lAddLine+1))==0))
							lAddLine += 2;
						else if ((lRemoveLine < PatchLines.GetCount())&&(sPatchLine.Compare(PatchLines.GetAt(lRemoveLine))==0))
							++lRemoveLine;
						else
						{
							m_sErrorMessage.FormatMessage(IDS_ERR_PATCH_DOESNOTMATCH, static_cast<LPCTSTR>(sPatchLine), static_cast<LPCTSTR>(PatchLines.GetAt(lAddLine-1)));
							return FALSE;
						}
					}
					++lAddLine;
					++lRemoveLine;
				}
				break;
			default:
				ASSERT(FALSE);
				break;
			} // switch (nPatchState)
		} // for (j=0; j<chunk->arLines.GetCount(); j++)
	} // for (int i=0; i<chunks->chunks.GetCount(); i++)
	if (!sSavePath.IsEmpty())
	{
		PatchLines.Save(sSavePath, false);
	}
	return TRUE;
}

BOOL CPatch::HasExpandedKeyWords(const CString& line) const
{
	if (line.Find(L"$LastChangedDate") >= 0)
		return TRUE;
	if (line.Find(L"$Date") >= 0)
		return TRUE;
	if (line.Find(L"$LastChangedRevision") >= 0)
		return TRUE;
	if (line.Find(L"$Rev") >= 0)
		return TRUE;
	if (line.Find(L"$LastChangedBy") >= 0)
		return TRUE;
	if (line.Find(L"$Author") >= 0)
		return TRUE;
	if (line.Find(L"$HeadURL") >= 0)
		return TRUE;
	if (line.Find(L"$URL") >= 0)
		return TRUE;
	if (line.Find(L"$Id") >= 0)
		return TRUE;
	return FALSE;
}

CString	CPatch::CheckPatchPath(const CString& path)
{
	//first check if the path already matches
	if (CountMatches(path) > (GetNumberOfFiles()/3))
		return path;
	//now go up the tree and try again
	CString upperpath = path;
	while (upperpath.ReverseFind('\\')>0)
	{
		upperpath = upperpath.Left(upperpath.ReverseFind('\\'));
		if (CountMatches(upperpath) > (GetNumberOfFiles()/3))
			return upperpath;
	}
	//still no match found. So try sub folders
	bool isDir = false;
	CString subpath;
	CDirFileEnum filefinder(path);
	while (filefinder.NextFile(subpath, &isDir))
	{
		if (!isDir)
			continue;
		if (GitAdminDir::IsAdminDirPath(subpath))
			continue;
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
		if (CountDirMatches(upperpath) > (GetNumberOfFiles()/3))
			return upperpath;
	}

	return path;
}

int CPatch::CountMatches(const CString& path)
{
	int matches = 0;
	for (int i=0; i<GetNumberOfFiles(); ++i)
	{
		CString temp = GetFilename(i);
		temp.Replace('/', '\\');
		if (PathIsRelative(temp))
			temp = path + L'\\' + temp;
		if (PathFileExists(temp))
			++matches;
	}
	return matches;
}

int CPatch::CountDirMatches(const CString& path)
{
	int matches = 0;
	for (int i=0; i<GetNumberOfFiles(); ++i)
	{
		CString temp = GetFilename(i);
		temp.Replace('/', '\\');
		if (PathIsRelative(temp))
			temp = path + L'\\' + temp;
		// remove the filename
		temp = temp.Left(temp.ReverseFind('\\'));
		if (PathFileExists(temp))
			++matches;
	}
	return matches;
}

CString	CPatch::Strip(const CString& filename) const
{
	CString s = filename;
	if ( m_nStrip>0 )
	{
		// Remove windows drive letter "c:"
		if ( s.GetLength()>2 && s[1]==':')
		{
			s = s.Mid(2);
		}

		for (int nStrip = 1; nStrip <= m_nStrip; ++nStrip)
		{
			// "/home/ts/my-working-copy/dir/file.txt"
			//  "home/ts/my-working-copy/dir/file.txt"
			//       "ts/my-working-copy/dir/file.txt"
			//          "my-working-copy/dir/file.txt"
			//                          "dir/file.txt"
			s = s.Mid(s.FindOneOf(L"/\\") + 1);
		}
	}
	return s;
}

CString CPatch::GetFullPath(const CString& sPath, int nIndex, int fileno /* = 0*/)
{
	CString temp;
	if (fileno == 0)
		temp = GetFilename(nIndex);
	else
		temp = GetFilename2(nIndex);

	temp.Replace('/', '\\');
	if(temp == L"NUL")
		return temp;

	if (PathIsRelative(temp))
	{
		if (sPath.Right(1) != L"\\")
			temp = sPath + L'\\' + temp;
		else
			temp = sPath + temp;
	}

	return temp;
}

CString CPatch::RemoveUnicodeBOM(const CString& str) const
{
	if (str.IsEmpty())
		return str;
	if (str[0] == 0xFEFF)
		return str.Mid(1);
	return str;
}
