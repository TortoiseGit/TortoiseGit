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
	static int DiffNull(CTGitPath *pPath, git_revnum_t &rev1);
};
