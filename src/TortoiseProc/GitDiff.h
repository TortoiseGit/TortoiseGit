#pragma once
#include "TGitPath.h"
#include "GitStatus.h"
#include "Git.h"

class CGitDiff
{
public:
	CGitDiff(void);
	~CGitDiff(void);
	static int Parser(git_revnum_t &rev);

	// Use two path to handle rename cases
	static int Diff(CTGitPath * pPath1, CTGitPath *pPath2 ,git_revnum_t & rev1, git_revnum_t & rev2, bool blame=false, bool unified=false);
	static int SubmoduleDiff(CTGitPath * pPath1, CTGitPath *pPath2 ,git_revnum_t & rev1, git_revnum_t & rev2, bool blame=false, bool unified=false);
	static int DiffNull(CTGitPath *pPath, git_revnum_t &rev1,bool bIsAdd=true);
	static int DiffCommit(CTGitPath &path, GitRev *r1, GitRev *r2);
	static int DiffCommit(CTGitPath &path, CString &r1, CString &r2);
	static int SubmoduleDiffNull(CTGitPath *pPath1,git_revnum_t &rev1);
};
