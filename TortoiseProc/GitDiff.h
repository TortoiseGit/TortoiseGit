#pragma once
#include "TGitPath.h"
#include "GitStatus.h"
#include "Git.h"

class CGitDiff
{
public:
	CGitDiff(void);
	~CGitDiff(void);
	

	static int Diff(CTGitPath * pPath, git_revnum_t & rev1, git_revnum_t & rev2, bool blame=false, bool unified=false);
};
