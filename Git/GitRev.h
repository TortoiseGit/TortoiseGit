#pragma once

class GitRev
{
public:
	GitRev(void);
	GitRev(GitRev &rev);
	~GitRev(void);
	enum
	{
		REV_HEAD = -1,			///< head revision
		REV_BASE = -2,			///< base revision
		REV_WC = -3,			///< revision of the working copy
		REV_UNSPECIFIED = -4,	///< unspecified revision
	};

};
