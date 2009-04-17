#include "StdAfx.h"
#include "SendMailCommand.h"
#include "SendMailDlg.h"
#include "SVNProgressDlg.h"

bool SendMailCommand::Execute()
{
	CSendMailDlg dlg;

	dlg.m_PathList  = orgPathList;
	
	if(dlg.DoModal()==IDOK)
	{
		if(dlg.m_PathList.GetCount() == 0)
			return FALSE;
	
		CGitProgressDlg progDlg;
		
		theApp.m_pMainWnd = &progDlg;
		progDlg.SetCommand(CGitProgressDlg::GitProgress_SendMail);
				
		if (parser.HasVal(_T("closeonend")))
				progDlg.SetAutoClose(parser.GetLongVal(_T("closeonend")));
		
		progDlg.SetPathList(dlg.m_PathList);
				//ProjectProperties props;
				//props.ReadPropsPathList(dlg.m_pathList);
				//progDlg.SetProjectProperties(props);
		progDlg.SetItemCount(dlg.m_PathList.GetCount());

		DWORD flags =0;
		if(dlg.m_bAttachment)
			flags |= SENDMAIL_ATTACHMENT;
		if(dlg.m_bCombine)
			flags |= SENDMAIL_COMBINED;

		progDlg.SetSendMailOption(dlg.m_To,dlg.m_CC,dlg.m_Subject,flags);
		
		progDlg.DoModal();		

		return true;
	}
	return false;
}