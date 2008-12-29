#include "StdAfx.h"
#include "GitDiff.h"
#include "AppUtils.h"
#include "git.h"

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
		if(!g_Git.Run(cmd,&output))
		{
			//int start=output.Find(_T('\n'));
			rev=output.Left(40);
		}
	}
	return 0;
}

int CGitDiff::Diff(CTGitPath * pPath, git_revnum_t & rev1, git_revnum_t & rev2, bool blame, bool unified)
{
	CString temppath;
	GetTempPath(temppath);
	Parser(rev1);
	Parser(rev2);
	CString file1;
	CString cmd;
	if(rev1 != GIT_REV_ZERO )
	{
		file1.Format(_T("%s%s_%s%s"),
				temppath,						
				pPath->GetBaseFilename(),
				rev1.Left(6),
				pPath->GetFileExtension());
		cmd.Format(_T("git.exe cat-file -p %s:%s"),rev1,pPath->GetGitPathString());
				g_Git.RunLogFile(cmd,file1);
	}else
	{
		file1=g_Git.m_CurrentDir+_T("\\")+pPath->GetWinPathString();
	}

	CString file2;
	if(rev2 != GIT_REV_ZERO)
	{
		
		file2.Format(_T("%s\\%s_%s%s"),
				temppath,						
				pPath->GetBaseFilename(),
				rev2.Left(6),
				pPath->GetFileExtension());
		cmd.Format(_T("git.exe cat-file -p %s:%s"),rev2,pPath->GetGitPathString());
		g_Git.RunLogFile(cmd,file2);
	}else
	{
		file2=g_Git.m_CurrentDir+_T("\\")+pPath->GetWinPathString();
	}
	
	CAppUtils::DiffFlags flags;
	CAppUtils::StartExtDiff(file1,file2,
							pPath->GetGitPathString()+_T(":")+rev2.Left(6),
							pPath->GetGitPathString()+_T(":")+rev1.Left(6)
							,flags);

	return 0;
}
