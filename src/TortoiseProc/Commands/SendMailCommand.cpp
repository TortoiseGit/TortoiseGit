#include "StdAfx.h"
#include "SendMailCommand.h"
#include "SendMailDlg.h"
#include "GITProgressDlg.h"
#include "AppUtils.h"

bool SendMailCommand::Execute()
{
	bool autoclose=false;
	if (parser.HasVal(_T("closeonend")))
		autoclose=!!parser.GetLongVal(_T("closeonend"));

	return CAppUtils::SendPatchMail(this->orgPathList,autoclose);
}