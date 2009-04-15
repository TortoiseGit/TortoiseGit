#include "StdAfx.h"
#include "Patch.h"

CPatch::CPatch()
{
	
}

CPatch::~CPatch()
{
	

}

int CPatch::Parser(CString &pathfile)
{
	CString str;

	CStdioFile PatchFile;

	m_PathFile=pathfile;
	if( ! PatchFile.Open(pathfile,CFile::modeRead) )
		return -1;
	
	int i=0;
	while(PatchFile.ReadString(str))
	{
		if(i==1)
			this->m_Author=str.Right( str.GetLength() - 6 );
		if(i==2)
			this->m_Date = str.Right( str.GetLength() - 6 );
		if(i==3)
			this->m_Subject = str.Right( str.GetLength() - 8 );
		if(i==4)
			break;
		i++;		
	}


	PatchFile.Close();

}