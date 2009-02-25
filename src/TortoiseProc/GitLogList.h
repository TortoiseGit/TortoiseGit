#pragma once
#include "GitLoglistBase.h"

class CGitLogListBase;

class CGitLogList : public CGitLogListBase
{
	DECLARE_DYNAMIC(CGitLogList)
protected:
	
	void SetSelectedAction(int action);
	int	 CherryPickFrom(CString from, CString to);

public:
	void ContextMenuAction(int cmd,int FirstSelect, int LastSelect);
};