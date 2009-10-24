#include "StdAfx.h"
#include "ATLComTime.h"
#include "GitRev.h"
#include "Git.h"

class CException; //Just in case afx.h is not included (cannot be included in every project which uses this file)

// provide an ASSERT macro for when compiled without MFC
#if !defined ASSERT
	// Don't use _asm here, it isn't supported by x64 version of compiler. In fact, MFC's ASSERT() is the same with _ASSERTE().
	#define ASSERT(x) _ASSERTE(x)
#endif


GitRev::GitRev(void)
{
	m_Action=0;
	m_IsFull = 0;
	m_IsUpdateing = 0;
	// fetch local machine timezone info
	if ( GetTimeZoneInformation( &m_TimeZone ) == TIME_ZONE_ID_INVALID )
	{
		ASSERT(false);
	}
}

GitRev::~GitRev(void)
{
}

#if 0
GitRev::GitRev(GitRev & rev)
{
}
GitRev& GitRev::operator=(GitRev &rev)
{
	return *this;
}
#endif
void GitRev::Clear()
{
	this->m_Action=0;
	this->m_Files.Clear();
	this->m_Action=0;
	this->m_ParentHash.clear();
	m_CommitterName.Empty();
	m_CommitterEmail.Empty();
	m_Body.Empty();
	m_Subject.Empty();
	m_CommitHash.Empty();
	m_Ref.Empty();
	m_RefAction.Empty();
	m_Mark=0;

}
int GitRev::CopyFrom(GitRev &rev,bool OmitParentAndMark)
{
	m_AuthorName	=rev.m_AuthorName	;
	m_AuthorEmail	=rev.m_AuthorEmail	;
	m_AuthorDate	=rev.m_AuthorDate	;
	m_CommitterName	=rev.m_CommitterName	;
	m_CommitterEmail=rev.m_CommitterEmail;
	m_CommitterDate	=rev.m_CommitterDate	;
	m_Subject		=rev.m_Subject		;
	m_Body			=rev.m_Body			;
	m_CommitHash	=rev.m_CommitHash	;
	m_Files			=rev.m_Files			;	
	m_Action		=rev.m_Action		;

	if(!OmitParentAndMark)
	{
		m_ParentHash	=rev.m_ParentHash	;
		m_Mark			=rev.m_Mark;
	}
	return 0;
}
int GitRev::ParserFromLog(BYTE_VECTOR &log,int start)
{
	int pos=start;
	CString one;
	CString key;
	CString text;
	BYTE_VECTOR filelist;
	BYTE mode=0;
	CTGitPath  path;
	this->m_Files.Clear();
    m_Action=0;
	int begintime=0;
	int filebegin=-1;

	while( pos < log.size() && pos>=0)
	{
		
		//one=log.Tokenize(_T("\n"),pos);
		if(log[pos]==_T('#') && log[pos+1] == _T('<') && log[pos+3] == _T('>'))
		{
			//text = one.Right(one.GetLength()-4);
			text.Empty();
			g_Git.StringAppend(&text,&log[pos+4],CGit::m_LogEncode);
			mode = log[pos+2];
			
			switch(mode)
			{
			case LOG_REV_ITEM_BEGIN:
				begintime++;
				if(begintime>1)
					break;
				else
					this->Clear();
				break;
			case LOG_REV_AUTHOR_NAME:
				this->m_AuthorName = text;
				break;
			case LOG_REV_AUTHOR_EMAIL:
				this->m_AuthorEmail = text;
				break;
			case LOG_REV_AUTHOR_DATE:
				this->m_AuthorDate =ConverFromString(text);
				break;
			case LOG_REV_COMMIT_NAME:
				this->m_CommitterName = text;
				break;
			case LOG_REV_COMMIT_EMAIL:
				this->m_CommitterEmail = text;
				break;
			case LOG_REV_COMMIT_DATE:
				this->m_CommitterDate =ConverFromString(text);
				break;
			case LOG_REV_COMMIT_SUBJECT:
				this->m_Subject = text;
				break;
			case LOG_REV_COMMIT_BODY:
				this->m_Body = text +_T("\n");
				break;
			case LOG_REV_COMMIT_HASH:
				this->m_CommitHash = text.Right(40);
				if(text.GetLength()>40)
				{
					this->m_Mark=text[0];
				}
				break;
			case LOG_REV_COMMIT_PARENT:
				while(text.GetLength()>0)
				{
					this->m_ParentHash.insert(this->m_ParentHash.end(),text.Left(40));
					if(text.GetLength()>40)
						text=text.Right(text.GetLength()-41);
					else
						break;
				}
				if(m_ParentHash.size()>1)
				{
					int a=1;
				}
				break;
			case LOG_REV_COMMIT_FILE:
				break;
			}
		}else
		{
			switch(mode)
			{
//			case LOG_REV_COMMIT_BODY:
//				this->m_Body += one+_T("\n");
//				break;
			case LOG_REV_COMMIT_FILE:
				//filelist += one +_T("\n");
				//filelist.append(log,pos,log.find(0,pos));
				if(filebegin<0)
					filebegin=pos;
				break;
			}
		}
		
		if(begintime>1)
		{
			break;
		}

		//find next string start 
		pos=log.findNextString(pos);
	}
	
	if(filebegin>=0)
	{
		
		filelist.append(log,filebegin,pos);	
		this->m_Files.ParserFromLog(filelist);
		this->m_Action=this->m_Files.GetAction();
	}
	return pos;
}

CTime GitRev::ConverFromString(CString input)
{
	// pick up date from string
	try
	{
		COleDateTime tm(_wtoi(input.Mid(0,4)),
				 _wtoi(input.Mid(5,2)),
				 _wtoi(input.Mid(8,2)),
				 _wtoi(input.Mid(11,2)),
				 _wtoi(input.Mid(14,2)),
				 _wtoi(input.Mid(17,2)));
		if( tm.GetStatus() != COleDateTime::valid )
			return CTime();//Error parsing time-string

		// pick up utc offset
		CString sign = input.Mid(20,1);		// + or -
		int hoursOffset =  _wtoi(input.Mid(21,2));
		int minsOffset = _wtoi(input.Mid(23,2));
		// convert to a fraction of a day
		double offset = (hoursOffset*60 + minsOffset) / 1440.0;  	// 1440 mins = 1 day
		if ( sign == "-" )
		{
			offset = -offset;
		}
		// we have to subtract this from the time given to get UTC
		tm -= offset;
		// get utc time as a SYSTEMTIME
		SYSTEMTIME sysTime;
		tm.GetAsSystemTime( sysTime );
		// and convert to users local time
		SYSTEMTIME local;
		if ( SystemTimeToTzSpecificLocalTime( &m_TimeZone, &sysTime, &local ) )
		{
			sysTime = local;
		}
		else
		{
			ASSERT(false);	// this should not happen but leave time in utc if it does
		}
		// convert to CTime and return
		return CTime( sysTime, -1 );;
	}
	catch(CException* e)
	{
		//Probably the date was something like 1970-01-01 00:00:00. _mktime64() doesnt like this.
		//Dont let the application crash on this exception

#ifdef _AFX //CException classes are only defined when afx.h is included.
			//When afx.h is not included, the exception is leaked.
			//This will probably never happen because when CException is not defined, it cannot be thrown.
		e->Delete();
#endif //ifdef _AFX
	}
	return CTime(); //Return an invalid time
}

int GitRev::SafeFetchFullInfo(CGit *git)
{
	if(InterlockedExchange(&m_IsUpdateing,TRUE) == FALSE)
	{
		//GitRev rev;
		BYTE_VECTOR onelog;
		TCHAR oldmark=this->m_Mark;
	
		git->GetLog(onelog,m_CommitHash,NULL,1,CGit::LOG_INFO_FULL_DIFF|CGit::LOG_INFO_STAT|CGit::LOG_INFO_FILESTATE|CGit::LOG_INFO_DETECT_COPYRENAME|CGit::LOG_INFO_SHOW_MERGEDFILE);
		CString oldhash=m_CommitHash;
		GIT_REV_LIST oldlist=this->m_ParentHash;
		ParserFromLog(onelog);
		
		//ASSERT(oldhash==m_CommitHash);
		if(oldmark!=0)
			this->m_Mark=oldmark;  //parser full log will cause old mark overwrited. 
							       //So we need keep old bound mark.
		this->m_ParentHash=oldlist;
		InterlockedExchange(&m_IsUpdateing,FALSE);
		InterlockedExchange(&m_IsFull,TRUE);
		return 0;
	}
	return -1;
}