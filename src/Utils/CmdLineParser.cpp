// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2013, 2016-2019 - TortoiseGit
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

const TCHAR CCmdLineParser::m_sDelims[] = L"-/";
const TCHAR CCmdLineParser::m_sQuotes[] = L"\"";
const TCHAR CCmdLineParser::m_sValueSep[] = L" :"; // don't forget space!!


CCmdLineParser::CCmdLineParser(LPCTSTR sCmdLine)
{
	if(sCmdLine)
		Parse(sCmdLine);
}

CCmdLineParser& CCmdLineParser::operator=(CCmdLineParser&& other)
{
	m_sCmdLine = std::move(other.m_sCmdLine);
	m_valueMap = std::move(other.m_valueMap);
	return *this;
}

BOOL CCmdLineParser::Parse(LPCTSTR sCmdLine)
{
	const std::wstring sEmpty;			//use this as a value if no actual value is given in commandline

	if(!sCmdLine)
		return false;

	m_valueMap.clear();
	m_sCmdLine = sCmdLine;

	tstring working = sCmdLine;
	auto sCurrent = working.data();

	for(;;)
	{
		//format is  -Key:"arg"

		if (!sCurrent[0])
			break;		// no more data, leave loop

		LPCTSTR sArg = wcspbrk(sCurrent, m_sDelims);
		if(!sArg)
			break; // no (more) delimiters found
		sArg = _wcsinc(sArg);

		if (!sArg[0])
			break; // ends with delim

		LPCTSTR sVal = wcspbrk(sArg, m_sValueSep);
		if (!sVal)
		{
			std::wstring Key(sArg);
			std::transform(Key.begin(), Key.end(), Key.begin(), ::towlower);
			m_valueMap.insert(CValsMap::value_type(Key, sEmpty));
			break;
		}
		else
		{
			std::wstring Key(sArg, static_cast<int>(sVal - sArg));
			std::transform(Key.begin(), Key.end(), Key.begin(), ::towlower);

			LPCTSTR sQuote(nullptr), sEndQuote(nullptr);
			if (sVal[0])
			{
				if (sVal[0] != L' ')
					sVal = _wcsinc(sVal);
				else
				{
					while (sVal[0] == L' ')
						sVal = _wcsinc(sVal);
				}

				LPCTSTR nextArg = wcspbrk(sVal, m_sDelims);

				sQuote = wcspbrk(sVal, m_sQuotes);

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
					sEndQuote = wcschr(sQuote, L' ');
				}
				else
				{
					if(sQuote == sVal)
					{
						// string with quotes (defined in m_sQuotes, e.g. '")
						sQuote = _wcsinc(sVal);
						sEndQuote = wcspbrk(sQuote, m_sQuotes);

						// search for double quotes
						while (sEndQuote)
						{
							auto nextQuote = _wcsinc(sEndQuote);
							if (nextQuote[0] == L'"')
							{
								working.erase(working.begin() + (sEndQuote - working.data()));
								sEndQuote = wcspbrk(nextQuote, m_sQuotes);
								continue;
							}
							break;
						}
					}
					else
					{
						sQuote = sVal;
						sEndQuote = wcschr(sQuote, L' ');
					}
				}
			}

			if (!sEndQuote)
			{
				// no end quotes or terminating space, take the rest of the string to its end
				if (!Key.empty() && sQuote)
				{
					std::wstring csVal(sQuote);
					m_valueMap.insert(CValsMap::value_type(Key, csVal));
				}
				break;
			}
			else
			{
				// end quote
				if(!Key.empty())
				{
					std::wstring csVal(sQuote, static_cast<int>(sEndQuote - sQuote));
					m_valueMap.insert(CValsMap::value_type(Key, csVal));
				}
				sCurrent = _wcsinc(sEndQuote);
				continue;
			}
		}
	}

	return !m_valueMap.empty();		//TRUE if arguments were found
}

CCmdLineParser::CValsMap::const_iterator CCmdLineParser::findKey(LPCTSTR sKey) const
{
	std::wstring s(sKey);
	std::transform(s.begin(), s.end(), s.begin(), ::towlower);
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
	return _wtol(it->second.c_str());
}

__int64 CCmdLineParser::GetLongLongVal(LPCTSTR sKey) const
{
	CValsMap::const_iterator it = findKey(sKey);
	if (it == m_valueMap.cend())
		return 0;
	return _wtoi64(it->second.c_str());
}

CCmdLineParser::ITERPOS CCmdLineParser::begin() const
{
	return m_valueMap.cbegin();
}

CCmdLineParser::ITERPOS CCmdLineParser::getNext(ITERPOS& pos, std::wstring& sKey, std::wstring& sValue) const
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
