#include "StdAfx.h"
#include "GitRev.h"


GitRev::GitRev(void)
{
	m_Action=0;
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
int GitRev::ParserFromLog(CString &log)
{
	int pos=0;
	CString one;
	CString key;
	CString text;
	TCHAR mode;
	CTGitPath  path;
	this->m_Files.Clear();
    m_Action=0;

	while( pos>=0 )
	{
		one=log.Tokenize(_T("\n"),pos);
		if(one[0]==_T('#') && one[1] == _T('<') && one[3] == _T('>'))
		{
			text = one.Right(one.GetLength()-4);
			mode = one[2];
			switch(mode)
			{
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
				this->m_CommitHash = text;
				break;
			case LOG_REV_COMMIT_PARENT:
				this->m_ParentHash.insert(this->m_ParentHash.end(),text);
				break;
			case LOG_REV_COMMIT_FILE:
				break;
			}
		}else
		{
			switch(mode)
			{
			case LOG_REV_COMMIT_BODY:
				this->m_Body += one+_T("\n");
				break;
			case LOG_REV_COMMIT_FILE:
				if(one[0]==_T(':'))
				{
					int tabstart=0;
					int actionstart=0;
					CString pathname;
					CString action;
					one.Tokenize(_T("\t"),tabstart);
					if(tabstart >0)
					{
						action=one.Left(tabstart);
						actionstart=action.ReverseFind(_T(' '));
						if(actionstart>0)
						{
							action=action.Right(action.GetLength()-actionstart);
						}
						pathname=one.Right(one.GetLength()-tabstart);
						
						CTGitPath *GitPath=m_Files.LookForGitPath(pathname);
						
						if(GitPath)
						{
							this->m_Action|=GitPath->ParserAction(action);	
							
						}else
						{	
							path.SetFromGit(pathname);
							//action must be set after setfromgit. SetFromGit will clear all status. 
							this->m_Action|=path.ParserAction(action);
							this->m_Files.AddPath(path);
						}
					}
					
				}else
				{
					int tabstart=0;
					path.Reset();
					CString StatAdd=(one.Tokenize(_T("\t"),tabstart));
					if( tabstart< 0)
						break;
					CString StatDel=(one.Tokenize(_T("\t"),tabstart));

					//SetFromGit will reset all context of GitRev
					path.SetFromGit(one.Right(one.GetLength()-tabstart));
				
					CTGitPath *GitPath=m_Files.LookForGitPath(path.GetGitPathString());
					if(GitPath)
					{
						GitPath->m_StatAdd=StatAdd;
						GitPath->m_StatDel=StatDel;

					}else
					{
						//path.SetFromGit(pathname);
						path.m_StatAdd=StatAdd;
						path.m_StatDel=StatDel;
						this->m_Files.AddPath(path);
					}
				}
				break;
			}
		}
	}
	return 0;
}

CTime GitRev::ConverFromString(CString input)
{
	CTime tm(_wtoi(input.Mid(0,4)),
			 _wtoi(input.Mid(5,2)),
			 _wtoi(input.Mid(8,2)),
			 _wtoi(input.Mid(11,2)),
			 _wtoi(input.Mid(14,2)),
			 _wtoi(input.Mid(17,2)),
			 _wtoi(input.Mid(20,4)));
	return tm;
}
