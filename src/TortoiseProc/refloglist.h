#pragma once
#include "GitLoglist.h"

class CGitLogListBase;

class CRefLogList : public CGitLogList
{
	DECLARE_DYNAMIC(CRefLogList)
public:
	CRefLogList();
	void InsertRefLogColumn();
	enum
	{
		REFLOG_HASH,
		REFLOG_REF,
		REFLOG_ACTION,
		REFLOG_MESSAGE
	};
protected:
	
};