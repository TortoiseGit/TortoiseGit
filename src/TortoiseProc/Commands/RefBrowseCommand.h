#pragma once
#include "Command.h"
#include "BrowseRefsDlg.h"

class RefBrowseCommand : public Command
{
	virtual bool Execute()
	{
		CBrowseRefsDlg(orgCmdLinePath.GetWinPath()).DoModal();
		return true;
	}
};

