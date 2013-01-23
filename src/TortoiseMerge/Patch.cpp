// TortoiseGitMerge - a Diff/Patch program

// Copyright (C) 2009-2013 - TortoiseGit
// Copyright (C) 2012-2013 - Sven Strickroth <email@cs-ware.de>
// Copyright (C) 2004-2009,2011-2012 - TortoiseSVN

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
	for (int i=0; i<m_arFileDiffs.GetCount(); i++)
	{
		Chunks * chunks = m_arFileDiffs.GetAt(i);
		for (int j=0; j<chunks->chunks.GetCount(); j++)
		{
			delete chunks->chunks.GetAt(j);
		}
		chunks->chunks.RemoveAll();
		delete chunks;
	}
	m_arFileDiffs.RemoveAll();
}

BOOL CPatch::ParsePatchFile(CFileTextLines &PatchLines)
{
	CString sLine;
	EOL ending = EOL_NOENDING;

	int state = 0;
	int nIndex = 0;
	Chunks * chunks = NULL;
	Chunk * chunk = NULL;
	int nAddLineCount = 0;
	int nRemoveLineCount = 0;
	int nContextLineCount = 0;
	std::map<CString, int> filenamesToPatch;
	for ( ;nIndex<PatchLines.GetCount(); nIndex++)
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
				if (sLine.Find(_T("diff ")) == 0)
				{
					if (chunks)
					{
						//this is a new file diff, so add the last one to
						//our array.
						if (chunks->chunks.GetCount() > 0)
							m_arFileDiffs.Add(chunks);
						else
							delete chunks;
					}
					chunks = new Chunks();
					state = 1;
					break;
				}
			}
			// fallthrough!
			case 1:
			{
				//index
				if( sLine.Find(_T("index"))==0 )
				{
					int dotstart=sLine.Find(_T(".."));
					if(dotstart>=0)
					{
						chunks->sRevision = sLine.Mid(dotstart-7,7);
						chunks->sRevision2 = sLine.Mid(dotstart+2,7);
					}
					break;
				}

				//---
				if (sLine.Find(_T("--- ")) == 0)
				{
					if (state == 0)
					{
						if (chunks)
						{
							//this is a new file diff, so add the last one to
							//our array.
							if (chunks->chunks.GetCount() > 0)
								m_arFileDiffs.Add(chunks);
							else
								delete chunks;
						}
						chunks = new Chunks();
					}

					sLine = sLine.Mid(3);	//remove the "---"
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
					{
						chunks->sFilePath = chunks->sFilePath.Left(chunks->sFilePath.Find('\t'));
					}
					if (chunks->sFilePath.Find(_T('"')) == 0 && chunks->sFilePath.ReverseFind(_T('"')) == chunks->sFilePath.GetLength() - 1)
						chunks->sFilePath=chunks->sFilePath.Mid(1, chunks->sFilePath.GetLength() - 2);
					if( chunks->sFilePath.Find(_T("a/")) == 0 )
						chunks->sFilePath=chunks->sFilePath.Mid(2);

					if( chunks->sFilePath.Find(_T("b/")) == 0 )
						chunks->sFilePath=chunks->sFilePath.Mid(2);


					chunks->sFilePath.Replace(_T('/'),_T('\\'));

					if (chunks->sFilePath == _T("\\dev\\null") || chunks->sFilePath == _T("/dev/null"))
						chunks->sFilePath  = _T("NUL");

					state = 3;
				}
				if (state == 0)
				{
					if (sLine.Find(_T("@@")) == 0)
					{
						if (chunks != NULL)
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
				if (sLine.Left(3).Compare(_T("+++")) != 0)
				{
					// no starting "+++" found
					m_sErrorMessage.Format(IDS_ERR_PATCH_NOADDFILELINE, nIndex);
					goto errorcleanup;
				}
				sLine = sLine.Mid(3);	//remove the "---"
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
				{
					chunks->sFilePath2 = chunks->sFilePath2.Left(chunks->sFilePath2.Find('\t'));
				}
				if (chunks->sFilePath2.Find(_T('"')) == 0 && chunks->sFilePath2.ReverseFind(_T('"')) == chunks->sFilePath2.GetLength() - 1)
					chunks->sFilePath2=chunks->sFilePath2.Mid(1, chunks->sFilePath2.GetLength() - 2);
				if( chunks->sFilePath2.Find(_T("a/")) == 0 )
					chunks->sFilePath2=chunks->sFilePath2.Mid(2);

				if( chunks->sFilePath2.Find(_T("b/")) == 0 )
					chunks->sFilePath2=chunks->sFilePath2.Mid(2);

				chunks->sFilePath2.Replace(_T('/'),_T('\\'));

				if (chunks->sFilePath2 == _T("\\dev\\null") || chunks->sFilePath2 == _T("/dev/null"))
					chunks->sFilePath2  = _T("NUL");

				++state;
			}
			break;

			case 4:
			{
				//start of a new chunk
				if (sLine.Left(2).Compare(_T("@@")) != 0)
				{
					//chunk doesn't start with "@@"
					//so there's garbage in between two file diffs
					state = 0;
					if (chunk)
					{
						delete chunk;
						chunk = nullptr;
						if (chunks)
						{
							for (int i=0; i<chunks->chunks.GetCount(); i++)
							{
								delete chunks->chunks.GetAt(i);
							}
							chunks->chunks.RemoveAll();
							delete chunks;
							chunks = nullptr;
						}
					}
					break;		//skip the garbage
				}

				//@@ -xxx,xxx +xxx,xxx @@
				sLine = sLine.Mid(2);
				sLine = sLine.Trim();
				chunk = new Chunk();
				CString sRemove = sLine.Left(sLine.Find(' '));
				CString sAdd = sLine.Mid(sLine.Find(' '));
				chunk->lRemoveStart = (-_ttol(sRemove));
				if (sRemove.Find(',')>=0)
				{
					sRemove = sRemove.Mid(sRemove.Find(',')+1);
					chunk->lRemoveLength = _ttol(sRemove);
				}
				else
				{
					chunk->lRemoveStart = 0;
					chunk->lRemoveLength = (-_ttol(sRemove));
				}
				chunk->lAddStart = _ttol(sAdd);
				if (sAdd.Find(',')>=0)
				{
					sAdd = sAdd.Mid(sAdd.Find(',')+1);
					chunk->lAddLength = _ttol(sAdd);
				}
				else
				{
					chunk->lAddStart = 1;
					chunk->lAddLength = _ttol(sAdd);
				}
				state++;
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
					chunk->arLines.Add(RemoveUnicodeBOM(sLine.Mid(1)));
					chunk->arLinesStates.Add(PATCHSTATE_CONTEXT);
					chunk->arEOLs.push_back(ending);
					nContextLineCount++;
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
					chunk->arLines.Add(RemoveUnicodeBOM(sLine.Mid(1)));
					chunk->arLinesStates.Add(PATCHSTATE_REMOVED);
					chunk->arEOLs.push_back(ending);
					nRemoveLineCount++;
				}
				else if (type == '+')
				{
					//an added line
					chunk->arLines.Add(RemoveUnicodeBOM(sLine.Mid(1)));
					chunk->arLinesStates.Add(PATCHSTATE_ADDED);
					chunk->arEOLs.push_back(ending);
					nAddLineCount++;
				}
				else
				{
					//none of those lines! what the hell happened here?
					m_sErrorMessage.Format(IDS_ERR_PATCH_UNKOWNLINETYPE, nIndex);
					goto errorcleanup;
				}
				if ((chunk->lAddLength == (nAddLineCount + nContextLineCount)) &&
					chunk->lRemoveLength == (nRemoveLineCount + nContextLineCount))
				{
					//chunk is finished
					if (chunks)
						chunks->chunks.Add(chunk);
					else
						delete chunk;
					chunk = NULL;
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
		m_arFileDiffs.Add(chunks);

	for (int i = 0; i < m_arFileDiffs.GetCount(); ++i)
	{
		if (filenamesToPatch[m_arFileDiffs.GetAt(i)->sFilePath] > 1 && m_arFileDiffs.GetAt(i)->sFilePath != _T("NUL"))
		{
			m_sErrorMessage.Format(IDS_ERR_PATCH_FILENAMENOTUNIQUE, m_arFileDiffs.GetAt(i)->sFilePath);
			FreeMemory();
			return FALSE;
		}
		++filenamesToPatch[m_arFileDiffs.GetAt(i)->sFilePath];
		if (m_arFileDiffs.GetAt(i)->sFilePath != m_arFileDiffs.GetAt(i)->sFilePath2)
		{
			if (filenamesToPatch[m_arFileDiffs.GetAt(i)->sFilePath2] > 1 && m_arFileDiffs.GetAt(i)->sFilePath2 != _T("NUL"))
			{
				m_sErrorMessage.Format(IDS_ERR_PATCH_FILENAMENOTUNIQUE, m_arFileDiffs.GetAt(i)->sFilePath);
				FreeMemory();
				return FALSE;
			}
			++filenamesToPatch[m_arFileDiffs.GetAt(i)->sFilePath2];
		}
	}

	return TRUE;

errorcleanup:
	if (chunk)
		delete chunk;
	if (chunks)
	{
		for (int i=0; i<chunks->chunks.GetCount(); i++)
		{
			delete chunks->chunks.GetAt(i);
		}
		chunks->chunks.RemoveAll();
		delete chunks;
	}
	FreeMemory();
	return FALSE;
}

BOOL CPatch::OpenUnifiedDiffFile(const CString& filename)
{
	CCrashReport::Instance().AddFile2(filename, NULL, _T("unified diff file"), CR_AF_MAKE_FILE_COPY);

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
	if (nIndex < 0)
		return _T("");
	if (nIndex < m_arFileDiffs.GetCount())
	{
		Chunks * c = m_arFileDiffs.GetAt(nIndex);
		CString filepath = Strip(c->sFilePath);
		return filepath;
	}
	return _T("");
}

CString CPatch::GetRevision(int nIndex)
{
	if (nIndex < 0)
		return 0;
	if (nIndex < m_arFileDiffs.GetCount())
	{
		Chunks * c = m_arFileDiffs.GetAt(nIndex);
		return c->sRevision;
	}
	return 0;
}

CString CPatch::GetFilename2(int nIndex)
{
	if (nIndex < 0)
		return _T("");
	if (nIndex < m_arFileDiffs.GetCount())
	{
		Chunks * c = m_arFileDiffs.GetAt(nIndex);
		CString filepath = Strip(c->sFilePath2);
		return filepath;
	}
	return _T("");
}

CString CPatch::GetRevision2(int nIndex)
{
	if (nIndex < 0)
		return 0;
	if (nIndex < m_arFileDiffs.GetCount())
	{
		Chunks * c = m_arFileDiffs.GetAt(nIndex);
		return c->sRevision2;
	}
	return 0;
}

int CPatch::PatchFile(const int strip, int nIndex, const CString& sPatchPath, const CString& sSavePath, const CString& sBaseFile, const bool force)
{
	m_nStrip = strip;
	CString sPath = GetFullPath(sPatchPath, nIndex);
	if (PathIsDirectory(sPath))
	{
		m_sErrorMessage.Format(IDS_ERR_PATCH_INVALIDPATCHFILE, (LPCTSTR)sPath);
		return FALSE;
	}
	if (nIndex < 0)
	{
		m_sErrorMessage.Format(IDS_ERR_PATCH_FILENOTINPATCH, (LPCTSTR)sPath);
		return FALSE;
	}

	if (!force && sPath == _T("NUL") && PathFileExists(GetFullPath(sPatchPath, nIndex, 1)))
		return FALSE;

	if (GetFullPath(sPatchPath, nIndex, 1) == _T("NUL") && !PathFileExists(sPath))
		return 2;

	CString sLine;
	CString sPatchFile = sBaseFile.IsEmpty() ? sPath : sBaseFile;
	if (PathFileExists(sPatchFile))
	{
		CCrashReport::Instance().AddFile2(sPatchFile, NULL, _T("File to patch"), CR_AF_MAKE_FILE_COPY);
	}
	CFileTextLines PatchLines;
	CFileTextLines PatchLinesResult;
	PatchLines.Load(sPatchFile);
	PatchLinesResult = PatchLines;  //.Copy(PatchLines);
	PatchLines.CopySettings(&PatchLinesResult);

	Chunks * chunks = m_arFileDiffs.GetAt(nIndex);

	for (int i=0; i<chunks->chunks.GetCount(); i++)
	{
		Chunk * chunk = chunks->chunks.GetAt(i);
		LONG lRemoveLine = chunk->lRemoveStart;
		LONG lAddLine = chunk->lAddStart;
		for (int j=0; j<chunk->arLines.GetCount(); j++)
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
			int nPatchState = (int)chunk->arLinesStates.GetAt(j);
			switch (nPatchState)
			{
			case PATCHSTATE_REMOVED:
				{
					if ((lAddLine > PatchLines.GetCount())||(PatchLines.GetCount()==0))
					{
						m_sErrorMessage.Format(IDS_ERR_PATCH_DOESNOTMATCH, _T(""), (LPCTSTR)sPatchLine);
						return FALSE;
					}
					if (lAddLine == 0)
						lAddLine = 1;
					if ((sPatchLine.Compare(PatchLines.GetAt(lAddLine-1))!=0)&&(!HasExpandedKeyWords(sPatchLine)))
					{
						m_sErrorMessage.Format(IDS_ERR_PATCH_DOESNOTMATCH, (LPCTSTR)sPatchLine, (LPCTSTR)PatchLines.GetAt(lAddLine-1));
						return FALSE;
					}
					if (lAddLine > PatchLines.GetCount())
					{
						m_sErrorMessage.Format(IDS_ERR_PATCH_DOESNOTMATCH, (LPCTSTR)sPatchLine, _T(""));
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
						if ((int)chunk->arLinesStates.GetAt(k) == PATCHSTATE_ADDED)
							continue;
						if (chunk->arLines.GetAt(k).Compare(PatchLines.GetAt(lAddLine - 1)) == 0)
							insertOk = true;
						else
							break;
					}
					
					if (insertOk)
					{
						PatchLines.InsertAt(lAddLine-1, sPatchLine, ending);
						lAddLine++;
					}
					else
					{
						m_sErrorMessage.Format(IDS_ERR_PATCH_DOESNOTMATCH, (LPCTSTR)PatchLines.GetAt(lAddLine - 1), chunk->arLines.GetAt(k));
						return FALSE;
					}
				}
				break;
			case PATCHSTATE_CONTEXT:
				{
					if (lAddLine > PatchLines.GetCount())
					{
						m_sErrorMessage.Format(IDS_ERR_PATCH_DOESNOTMATCH, _T(""), (LPCTSTR)sPatchLine);
						return FALSE;
					}
					if (lAddLine == 0)
						lAddLine++;
					if (lRemoveLine == 0)
						lRemoveLine++;
					if ((sPatchLine.Compare(PatchLines.GetAt(lAddLine-1))!=0) &&
						(!HasExpandedKeyWords(sPatchLine)) &&
						(lRemoveLine <= PatchLines.GetCount()) &&
						(sPatchLine.Compare(PatchLines.GetAt(lRemoveLine-1))!=0))
					{
						if ((lAddLine < PatchLines.GetCount())&&(sPatchLine.Compare(PatchLines.GetAt(lAddLine))==0))
							lAddLine++;
						else if (((lAddLine + 1) < PatchLines.GetCount())&&(sPatchLine.Compare(PatchLines.GetAt(lAddLine+1))==0))
							lAddLine += 2;
						else if ((lRemoveLine < PatchLines.GetCount())&&(sPatchLine.Compare(PatchLines.GetAt(lRemoveLine))==0))
							lRemoveLine++;
						else
						{
							m_sErrorMessage.Format(IDS_ERR_PATCH_DOESNOTMATCH, (LPCTSTR)sPatchLine, (LPCTSTR)PatchLines.GetAt(lAddLine-1));
							return FALSE;
						}
					}
					lAddLine++;
					lRemoveLine++;
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

BOOL CPatch::HasExpandedKeyWords(const CString& line)
{
	if (line.Find(_T("$LastChangedDate"))>=0)
		return TRUE;
	if (line.Find(_T("$Date"))>=0)
		return TRUE;
	if (line.Find(_T("$LastChangedRevision"))>=0)
		return TRUE;
	if (line.Find(_T("$Rev"))>=0)
		return TRUE;
	if (line.Find(_T("$LastChangedBy"))>=0)
		return TRUE;
	if (line.Find(_T("$Author"))>=0)
		return TRUE;
	if (line.Find(_T("$HeadURL"))>=0)
		return TRUE;
	if (line.Find(_T("$URL"))>=0)
		return TRUE;
	if (line.Find(_T("$Id"))>=0)
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
		if (g_GitAdminDir.IsAdminDirPath(subpath))
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
			temp = path + _T("\\")+ temp;
		if (PathFileExists(temp))
			matches++;
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
			temp = path + _T("\\")+ temp;
		// remove the filename
		temp = temp.Left(temp.ReverseFind('\\'));
		if (PathFileExists(temp))
			matches++;
	}
	return matches;
}

BOOL CPatch::StripPrefixes(const CString& path)
{
	int nSlashesMax = 0;
	for (int i=0; i<GetNumberOfFiles(); i++)
	{
		CString filename = GetFilename(i);
		filename.Replace('/','\\');
		int nSlashes = filename.Replace('\\','/');
		nSlashesMax = max(nSlashesMax,nSlashes);
	}

	for (int nStrip=1;nStrip<nSlashesMax;nStrip++)
	{
		m_nStrip = nStrip;
		if ( CountMatches(path) > GetNumberOfFiles()/3 )
		{
			// Use current m_nStrip
			return TRUE;
		}
	}

	// Stripping doesn't help so reset it again
	m_nStrip = 0;
	return FALSE;
}

CString	CPatch::Strip(const CString& filename)
{
	CString s = filename;
	if ( m_nStrip>0 )
	{
		// Remove windows drive letter "c:"
		if ( s.GetLength()>2 && s[1]==':')
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
			s = s.Mid(s.FindOneOf(_T("/\\"))+1);
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
	if(temp == _T("NUL"))
		return temp;

	if (PathIsRelative(temp))
	{
		if (sPath.Right(1).Compare(_T("\\")) != 0)
			temp = sPath + _T("\\") + temp;
		else
			temp = sPath + temp;
	}

	return temp;
}

CString CPatch::RemoveUnicodeBOM(const CString& str)
{
	if (str.GetLength()==0)
		return str;
	if (str[0] == 0xFEFF)
		return str.Mid(1);
	return str;
}
