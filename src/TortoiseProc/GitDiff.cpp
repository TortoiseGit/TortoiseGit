#include "StdAfx.h"
#include "GitDiff.h"
#include "AppUtils.h"
#include "git.h"
#include "gittype.h"
#include "resource.h"
#include "MessageBox.h"

CGitDiff::CGitDiff(void)
{
}

CGitDiff::~CGitDiff(void)
{
}
int CGitDiff::Parser(git_revnum_t &rev)
{
	if(rev == GIT_REV_ZERO)
		return 0;
	if(rev.GetLength() > 40)
	{
		CString cmd;
		cmd.Format(_T("git.exe rev-parse %s"),rev);
		CString output;
		if(!g_Git.Run(cmd,&output,CP_UTF8))
		{
			//int start=output.Find(_T('\n'));
			rev=output.Left(40);
		}
	}
	return 0;
}
int CGitDiff::DiffNull(CTGitPath *pPath, git_revnum_t &rev1,bool bIsAdd)
{
	CString temppath;
	GetTempPath(temppath);
	Parser(rev1);
	CString file1;
	CString nullfile;
	CString cmd;
	if(rev1 != GIT_REV_ZERO )
	{
		file1.Format(_T("%s%s_%s%s"),
				temppath,						
				pPath->GetBaseFilename(),
				rev1.Left(6),
				pPath->GetFileExtension());
		cmd.Format(_T("git.exe cat-file -p %s:\"%s\""),rev1,pPath->GetGitPathString());
				g_Git.RunLogFile(cmd,file1);
	}else
	{
		file1=g_Git.m_CurrentDir+_T("\\")+pPath->GetWinPathString();
	}

	CString tempfile=::GetTempFile();
	CStdioFile file(tempfile,CFile::modeReadWrite|CFile::modeCreate );
	//file.WriteString();
	file.Close();
	
	CAppUtils::DiffFlags flags;

	if(bIsAdd)
		CAppUtils::StartExtDiff(tempfile,file1,
							_T("NULL"),
							pPath->GetGitPathString()+_T(":")+rev1.Left(6)
							,flags);
	else
		CAppUtils::StartExtDiff(file1,tempfile,
							pPath->GetGitPathString()+_T(":")+rev1.Left(6)
							,_T("NULL"),flags);

	return 0;
}

int CGitDiff::Diff(CTGitPath * pPath,CTGitPath * pPath2, git_revnum_t & rev1, git_revnum_t & rev2, bool /*blame*/, bool /*unified*/)
{
	CString temppath;
	GetTempPath(temppath);
	Parser(rev1);
	Parser(rev2);
	CString file1;
	CString title1;
	CString cmd;

	if(pPath->IsDirectory() || pPath2->IsDirectory())
	{
		cmd.Format(_T("git.exe diff-tree -r -z %s %s -- \"%s\""),
			rev2,rev1,pPath->GetGitPathString());
		
		BYTE_VECTOR bytes;
		if(g_Git.Run(cmd,&bytes))
		{
			CString err;
			g_Git.StringAppend(&err,&bytes[0],CP_ACP);
			CMessageBox::Show(NULL,err,_T("TortoiseGit"),MB_OK|MB_ICONERROR);
		}
		
		CString oldhash;
		g_Git.StringAppend(&oldhash,&bytes[15],CP_ACP,40);
		CString newhash;
		g_Git.StringAppend(&newhash,&bytes[15+41],CP_ACP,40);

		CString oldsub;
		CString newsub;

		CGit subgit; 
		subgit.m_CurrentDir=g_Git.m_CurrentDir+_T("\\")+pPath->GetWinPathString();

		CString cmd;
		if(pPath->HasAdminDir())
		{
			int encode=CAppUtils::GetLogOutputEncode(&subgit);

			if(oldhash != GIT_REV_ZERO)
			{
				cmd.Format(_T("git log -n1 HEAD --pretty=format:\"%%s\" %s"),oldhash);
				subgit.Run(cmd,&oldsub,encode);
			}

			if(newsub != GIT_REV_ZERO)
			{
				cmd.Format(_T("git log -n1 HEAD --pretty=format:\"%%s\" %s"),newhash);
				subgit.Run(cmd,&newsub,encode);
			}
		}
		CString msg;
		msg.Format(_T("Submodule <b>%s</b> Change\r\n\r\n<b>From:</b> %s\r\n\t%s\r\n\r\n<b>To:</b>     %s\r\n\t\t%s"),
				   pPath->GetWinPath(),
				   oldhash,
				   oldsub ,
				   newhash,
				   newsub);
		CMessageBox::Show(NULL,msg,_T("TortoiseGit"),MB_OK);

		return 0;
	}

	if(rev1 != GIT_REV_ZERO )
	{
		file1.Format(_T("%s%s_%s%s"),
				temppath,						
				pPath->GetBaseFilename(),
				rev1.Left(6),
				pPath->GetFileExtension());
		title1 = pPath->GetFileOrDirectoryName()+_T(":")+rev1.Left(6);
		cmd.Format(_T("git.exe cat-file -p %s:\"%s\""),rev1,pPath->GetGitPathString());
				g_Git.RunLogFile(cmd,file1);
	}else
	{
		file1=g_Git.m_CurrentDir+_T("\\")+pPath->GetWinPathString();
		title1.Format( IDS_DIFF_WCNAME, pPath->GetFileOrDirectoryName() );
	}

	CString file2;
	CString title2;
	if(rev2 != GIT_REV_ZERO)
	{
		
		file2.Format(_T("%s%s_%s%s"),
				temppath,						
				pPath2->GetBaseFilename(),
				rev2.Left(6),
				pPath2->GetFileExtension());
		title2 = pPath2->GetFileOrDirectoryName()+_T(":")+rev2.Left(6);
		cmd.Format(_T("git.exe cat-file -p %s:\"%s\""),rev2,pPath2->GetGitPathString());
		g_Git.RunLogFile(cmd,file2);
	}else
	{
		file2=g_Git.m_CurrentDir+_T("\\")+pPath2->GetWinPathString();
		title2.Format( IDS_DIFF_WCNAME, pPath2->GetFileOrDirectoryName() );
	}
	
	CAppUtils::DiffFlags flags;
	CAppUtils::StartExtDiff(file2,file1,
							title2,
							title1
							,flags);

	return 0;
}

