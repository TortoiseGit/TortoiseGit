// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2013, 2016 - TortoiseGit
// Copyright (C) 2003-2006,2012 - Stefan Kueng

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
#include "CmdLineParser.h"
#include <locale>
#include <algorithm>

const TCHAR CCmdLineParser::m_sDelims[] = _T("-/");
const TCHAR CCmdLineParser::m_sQuotes[] = _T("\"");
const TCHAR CCmdLineParser::m_sValueSep[] = _T(" :"); // don't forget space!!


CCmdLineParser::CCmdLineParser(LPCTSTR sCmdLine)
{
	if(sCmdLine)
		Parse(sCmdLine);
}

CCmdLineParser::~CCmdLineParser()
{
	m_valueMap.clear();
}

BOOL CCmdLineParser::Parse(LPCTSTR sCmdLine)
{
	const tstring sEmpty = _T("");			//use this as a value if no actual value is given in commandline
	int nArgs = 0;

	if(!sCmdLine)
		return false;

	m_valueMap.clear();
	m_sCmdLine = sCmdLine;

	LPCTSTR sCurrent = sCmdLine;

	for(;;)
	{
		//format is  -Key:"arg"

		if (_tcslen(sCurrent) == 0)
			break;		// no more data, leave loop

		LPCTSTR sArg = _tcspbrk(sCurrent, m_sDelims);
		if(!sArg)
			break; // no (more) delimiters found
		sArg =  _tcsinc(sArg);

		if(_tcslen(sArg) == 0)
			break; // ends with delim

		LPCTSTR sVal = _tcspbrk(sArg, m_sValueSep);
		if (!sVal)
		{
			tstring Key(sArg);
			std::transform(Key.begin(), Key.end(), Key.begin(), ::tolower);
			m_valueMap.insert(CValsMap::value_type(Key, sEmpty));
			break;
		}
		else
		{
			tstring Key(sArg, (int)(sVal - sArg));
			std::transform(Key.begin(), Key.end(), Key.begin(), ::tolower);

			LPCTSTR sQuote(nullptr), sEndQuote(nullptr);
			if (_tcslen(sVal) > 0)
			{
				if (sVal[0] != _T(' '))
					sVal = _tcsinc(sVal);
				else
				{
					while (_tcslen(sVal) > 0 && sVal[0] == _T(' '))
						sVal = _tcsinc(sVal);
				}
				
				LPCTSTR nextArg = _tcspbrk(sVal, m_sDelims);

				sQuote = _tcspbrk(sVal, m_sQuotes);

				if (nextArg == sVal)
				{
					// current key has no value, but a next key exist - so don't use next key as value of current one
					--sVal;
					sQuote = sVal;
					sEndQuote = sVal;
				}
				else if (nextArg && nextArg < sQuote)
				{
					// current key has a value w/o quotes, but next key one has value in quotes
					sQuote = sVal;
					sEndQuote = _tcschr(sQuote, _T(' '));
				}
				else
				{
					if(sQuote == sVal)
					{
						// string with quotes (defined in m_sQuotes, e.g. '")
						sQuote = _tcsinc(sVal);
						sEndQuote = _tcspbrk(sQuote, m_sQuotes);
					}
					else
					{
						sQuote = sVal;
						sEndQuote = _tcschr(sQuote, _T(' '));
					}
				}
			}

			if (!sEndQuote)
			{
				// no end quotes or terminating space, take the rest of the string to its end
				if (!Key.empty() && sQuote)
				{
					tstring csVal(sQuote);
					m_valueMap.insert(CValsMap::value_type(Key, csVal));
				}
				break;
			}
			else
			{
				// end quote
				if(!Key.empty())
				{
					tstring csVal(sQuote, (int)(sEndQuote - sQuote));
					m_valueMap.insert(CValsMap::value_type(Key, csVal));
				}
				sCurrent = _tcsinc(sEndQuote);
				continue;
			}
		}
	}

	return (nArgs > 0);		//TRUE if arguments were found
}

CCmdLineParser::CValsMap::const_iterator CCmdLineParser::findKey(LPCTSTR sKey) const
{
	tstring s(sKey);
	std::transform(s.begin(), s.end(), s.begin(), ::tolower);
	return m_valueMap.find(s);
}

BOOL CCmdLineParser::HasKey(LPCTSTR sKey) const
{
	CValsMap::const_iterator it = findKey(sKey);
	if (it == m_valueMap.cend())
		return false;
	return true;
}


BOOL CCmdLineParser::HasVal(LPCTSTR sKey) const
{
	CValsMap::const_iterator it = findKey(sKey);
	if (it == m_valueMap.cend())
		return false;
	if(it->second.empty())
		return false;
	return true;
}

LPCTSTR CCmdLineParser::GetVal(LPCTSTR sKey) const
{
	CValsMap::const_iterator it = findKey(sKey);
	if (it == m_valueMap.cend())
		return 0;
	return it->second.c_str();
}

LONG CCmdLineParser::GetLongVal(LPCTSTR sKey) const
{
	CValsMap::const_iterator it = findKey(sKey);
	if (it == m_valueMap.cend())
		return 0;
	return _tstol(it->second.c_str());
}

__int64 CCmdLineParser::GetLongLongVal(LPCTSTR sKey) const
{
	CValsMap::const_iterator it = findKey(sKey);
	if (it == m_valueMap.cend())
		return 0;
	return _ttoi64(it->second.c_str());
}

CCmdLineParser::ITERPOS CCmdLineParser::begin() const
{
	return m_valueMap.cbegin();
}

CCmdLineParser::ITERPOS CCmdLineParser::getNext(ITERPOS& pos, tstring& sKey, tstring& sValue) const
{
	if (m_valueMap.cend() == pos)
	{
		sKey.clear();
		return pos;
	}
	else
	{
		sKey = pos->first;
		sValue = pos->second;
		return ++pos;
	}
}

BOOL CCmdLineParser::isLast(const ITERPOS& pos) const
{
	return (pos == m_valueMap.cend());
}
