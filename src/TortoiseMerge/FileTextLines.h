// TortoiseMerge - a Diff/Patch program

// Copyright (C) 2006-2007 - TortoiseSVN

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
#pragma once
#include "EOL.h"

// A template class to make an array which looks like a CStringArray or CDWORDArray but
// is in fact based on a STL array, which is much faster at large sizes
template <typename T> class CStdArray
{
public:
	int GetCount() const { return (int)m_vec.size(); }
	const T& GetAt(int index) const { return m_vec[index]; }
	void RemoveAt(int index)	{ m_vec.erase(m_vec.begin()+index); }
	void InsertAt(int index, const T& strVal)	{ m_vec.insert(m_vec.begin()+index, strVal); }
	void InsertAt(int index, const T& strVal, int nCopies)	{ m_vec.insert(m_vec.begin()+index, nCopies, strVal); }
	void SetAt(int index, const T& strVal)	{ m_vec[index] = strVal; }
	void Add(const T& strVal)	 { m_vec.push_back(strVal); }
	void RemoveAll()				{ m_vec.clear(); }
	void Reserve(int lengthHint)	{ m_vec.reserve(lengthHint); }

private:
	std::vector<T> m_vec;
};

typedef CStdArray<CString> CStdCStringArray;
typedef CStdArray<DWORD> CStdDWORDArray;

/**
 * \ingroup TortoiseMerge
 *
 * Represents an array of text lines which are read from a file.
 * This class is also responsible for determining the encoding of
 * the file (e.g. UNICODE, UTF8, ASCII, ...).
 */
class CFileTextLines  : public CStdCStringArray
{
public:
	CFileTextLines(void);
	~CFileTextLines(void);

	enum UnicodeType
	{
		AUTOTYPE,
		BINARY,
		ASCII,
		UNICODE_LE,
		UTF8,
		UTF8BOM,
	};

	/**
	 * Loads the text file and adds each line to the array
	 * \param sFilePath the path to the file
	 */
	BOOL		Load(const CString& sFilePath, int lengthHint = 0);
	/**
	 * Saves the whole array of text lines to a file, preserving
	 * the line endings detected at Load()
	 * \param sFilePath the path to save the file to
	 */
	BOOL		Save(const CString& sFilePath, bool bSaveAsUTF8, DWORD dwIgnoreWhitespaces=0, BOOL bIgnoreCase = FALSE, bool bBlame = false);
	/**
	 * Returns an error string of the last failed operation
	 */
	CString		GetErrorString() const  {return m_sErrorString;}
	/**
	 * Copies the settings of a file like the line ending styles
	 * to another CFileTextLines object.
	 */
	void		CopySettings(CFileTextLines * pFileToCopySettingsTo);

	CFileTextLines::UnicodeType GetUnicodeType() const  {return m_UnicodeType;}
	EOL GetLineEndings() const {return m_LineEndings;}

	void		Add(const CString& sLine, EOL ending) {CStdCStringArray::Add(sLine); m_endings.push_back(ending);}
	void		RemoveAt(int index)	{CStdCStringArray::RemoveAt(index); m_endings.erase(m_endings.begin()+index);}
	void		InsertAt(int index, const CString& strVal, EOL ending) {CStdCStringArray::InsertAt(index, strVal); m_endings.insert(m_endings.begin()+index, ending);}

	EOL			GetLineEnding(int index) {return m_endings[index];}
	void		SetLineEnding(int index, EOL ending) {m_endings[index] = ending;}

	void		RemoveAll() {CStdCStringArray::RemoveAll(); m_endings.clear();}

	/**
	 * Checks the Unicode type in a text buffer
	 * \param pBuffer pointer to the buffer containing text
	 * \param cd size of the text buffer in bytes
	 */
	CFileTextLines::UnicodeType CheckUnicodeType(LPVOID pBuffer, int cb);

private:
	/**
	 * Checks the line endings in a text buffer
	 * \param pBuffer pointer to the buffer containing text
	 * \param cd size of the text buffer in bytes
	 */
	EOL CheckLineEndings(LPVOID pBuffer, int cb);

	void		SetErrorString();

	void StripAsciiWhiteSpace(CStringA& sLine);

	void StripWhiteSpace(CString& sLine,DWORD dwIgnoreWhitespaces, bool blame);
	void StripAsciiWhiteSpace(CStringA& sLine,DWORD dwIgnoreWhitespaces, bool blame);


private:
	std::vector<EOL>							m_endings;
	CString										m_sErrorString;
	CFileTextLines::UnicodeType					m_UnicodeType;
	EOL											m_LineEndings;
	bool										m_bReturnAtEnd;
};
