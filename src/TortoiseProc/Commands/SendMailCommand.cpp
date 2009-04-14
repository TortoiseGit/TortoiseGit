#include "StdAfx.h"
#include "SendMailCommand.h"
#include "SendMailDlg.h"

bool SendMailCommand::Execute()
{
	CSendMailDlg dlg;

	dlg.m_PathList  = orgPathList;
	
	if(dlg.DoModal()==IDOK)
	{
		return true;
	}
	return false;
}