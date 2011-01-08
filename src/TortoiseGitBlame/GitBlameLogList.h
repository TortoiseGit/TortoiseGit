#pragma once
#include "GitLoglistBase.h"
class CGitBlameLogList : public CGitLogListBase
{
	DECLARE_DYNAMIC(CGitBlameLogList)
public:
	void hideUnimplementedCommands();
	void ContextMenuAction(int cmd,int FirstSelect, int LastSelect,CMenu * menu);
};