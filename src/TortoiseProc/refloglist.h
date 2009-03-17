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
	
	std::map<CString,CLogDataVector> m_RefMap;
protected:
	virtual void OnLvnGetdispinfoLoglist(NMHDR *pNMHDR, LRESULT *pResult);
	virtual void OnNMCustomdrawLoglist(NMHDR *pNMHDR, LRESULT *pResult);
	
};