// TortoiseGit - a Windows shell extension for easy version control

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

#pragma once
#include "GitHash.h"

enum
{
	TGIT_GIT_SUCCESS=0,
	TGIT_GIT_ERROR_OPEN_PIP,
	TGIT_GIT_ERROR_CREATE_PROCESS,
	TGIT_GIT_ERROR_GET_EXIT_CODE
};

extern BOOL g_IsWingitDllload;

class CGitByteArray:public std::vector<BYTE>
{
public:
	CGitByteArray(){ m_critSec.Init(); }
	CComCriticalSection			m_critSec;

	int find(BYTE data,int start=0)
	{
		for(unsigned int i=start;i<size();i++)
			if( at(i) == data )
				return i;
		return -1;
	}
	int RevertFind(BYTE data, int start=-1)
	{
		if(start == -1)
			start = (int)size() - 1;

		if(start<0)
			return -1;

		for(int i=start; i>=0;i--)
			if( at(i) == data )
				return i;
		return -1;
	}
	int findNextString(int start=0)
	{
		int pos=start;
		do
		{
			pos=find(0,pos);
			if(pos >= 0)
				pos++;
			else
				break;

			if (pos >= (int)size())
				return -1;

		}while(at(pos)==0);

		return pos;
	}
	int findData(const BYTE* dataToFind, size_t dataSize, int start=0)
	{
		//Pre checks
		if(empty())
			return -1;
		if(dataSize==0)
			return 0;
		if(dataSize>size()-start)
			return -1;//Data to find is greater then data to search in. No match

		//Initialize
		const BYTE* pos=&*(begin()+start);
		const BYTE* dataEnd=&*(begin()+(size()-dataSize) );++dataEnd;//Set end one step after last place to search
		if(pos>=dataEnd)
			return -1;//Started over end. Return not found
		if(dataSize==0)
			return start;//No search data. Return current position
		BYTE firstByte=dataToFind[0];
		while(pos<dataEnd)
		{
			//memchr for first character
			const BYTE* found=(const BYTE*)memchr(pos,firstByte,dataEnd-pos);
			if(found==NULL)
				return -1;//Not found
			//check rest of characters
			if(memcmp(found,dataToFind,dataSize)==0)
				return (int)(found-&*begin());//Match. Return position.
			//No match. Set position on next byte and continue search
			pos=found+1;
		}
		return -1;
	}
	int append( std::vector<BYTE> &v,int start=0,int end=-1)
	{
		if(end<0)
			end = (int)v.size();
		for(int i=start;i<end;i++)
			this->push_back(v[i]);
		return 0;
	}
	int append(const BYTE* data, size_t dataSize)
	{
		size_t oldsize=size();
		resize(oldsize+dataSize);
		memcpy(&*(begin()+oldsize),data,dataSize);
		return 0;
	}
};
typedef std::vector<CString> STRING_VECTOR;
typedef std::map<CGitHash, STRING_VECTOR> MAP_HASH_NAME;
typedef CGitByteArray BYTE_VECTOR;

