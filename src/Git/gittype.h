#pragma once

enum
{
	GIT_SUCCESS=0,
	GIT_ERROR_OPEN_PIP,
	GIT_ERROR_CREATE_PROCESS,
	GIT_ERROR_GET_EXIT_CODE
};

class CGitByteArray:public std::vector<BYTE>
{
public:
	int find(BYTE data,int start=0)
	{
		for(int i=start;i<size();i++)
			if( at(i) == data )
				return i;
		return -1;
	}
	int append( std::vector<BYTE> &v,int start=0,int end=-1)
	{
		if(end<0)
			end=v.size();
		for(int i=start;i<end;i++)
			this->push_back(v[i]);
		return 0;
	}
};
typedef std::vector<CString> STRING_VECTOR;
typedef std::map<CString, STRING_VECTOR> MAP_HASH_NAME;
typedef CGitByteArray BYTE_VECTOR;