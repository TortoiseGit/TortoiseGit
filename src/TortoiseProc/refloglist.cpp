#include "stdafx.h"
#include "resource.h"
#include "RefLogDlg.h"
#include "git.h"
#include "RefLogList.h"

IMPLEMENT_DYNAMIC(CRefLogList, CGitLogList)

CRefLogList::CRefLogList()
{

}

void CRefLogList::InsertRefLogColumn()
{
	CString temp;

	int c = ((CHeaderCtrl*)(GetDlgItem(0)))->GetItemCount()-1;
	
	while (c>=0)
		DeleteColumn(c--);
	
	temp=_T("Hash");
	InsertColumn(REFLOG_HASH, temp);
	
	temp=_T("Ref");
	InsertColumn(REFLOG_REF, temp);
	
	temp=_T("Action");
	InsertColumn(REFLOG_ACTION, temp);
	
	temp=_T("Message");
	InsertColumn(REFLOG_MESSAGE, temp);


	SetRedraw(false);
	ResizeAllListCtrlCols();
	SetRedraw(true);
}