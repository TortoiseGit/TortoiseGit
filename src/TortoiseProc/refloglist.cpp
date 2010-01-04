#include "stdafx.h"
#include "resource.h"
#include "RefLogDlg.h"
#include "git.h"
#include "RefLogList.h"

IMPLEMENT_DYNAMIC(CRefLogList, CGitLogList)

CRefLogList::CRefLogList()
{
	m_ColumnRegKey=_T("reflog");

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

void CRefLogList::OnLvnGetdispinfoLoglist(NMHDR *pNMHDR, LRESULT *pResult)
{
	NMLVDISPINFO *pDispInfo = reinterpret_cast<NMLVDISPINFO*>(pNMHDR);

	// Create a pointer to the item
	LV_ITEM* pItem = &(pDispInfo)->item;

	// Do the list need text information?
	if (!(pItem->mask & LVIF_TEXT))
		return;

	// By default, clear text buffer.
	lstrcpyn(pItem->pszText, _T(""), pItem->cchTextMax);

	bool bOutOfRange = pItem->iItem >= ShownCountWithStopped();
	
	*pResult = 0;
	if (m_bNoDispUpdates || bOutOfRange)
		return;

	// Which item number?
	int itemid = pItem->iItem;
	GitRev * pLogEntry = NULL;
	if (itemid < m_arShownList.GetCount())
		pLogEntry = reinterpret_cast<GitRev*>(m_arShownList.GetAt(pItem->iItem));

	CString temp;
	    
	// Which column?
	switch (pItem->iSubItem)
	{
	case this->REFLOG_HASH:	//Graphic
		if (pLogEntry)
		{
			lstrcpyn(pItem->pszText,pLogEntry->m_CommitHash.ToString(), pItem->cchTextMax);
		}
		break;
	case REFLOG_REF: //action -- no text in the column
		if(pLogEntry)
			lstrcpyn(pItem->pszText, pLogEntry->m_Ref, pItem->cchTextMax);
		break;
	case REFLOG_ACTION: //Message
		if (pLogEntry)
			lstrcpyn(pItem->pszText, (LPCTSTR)pLogEntry->m_RefAction, pItem->cchTextMax);
		break;
	case REFLOG_MESSAGE: //Author
		if (pLogEntry)
			lstrcpyn(pItem->pszText, (LPCTSTR)pLogEntry->m_Subject, pItem->cchTextMax);
		break;
		
	default:
		ASSERT(false);
	}
}

void CRefLogList::OnNMCustomdrawLoglist(NMHDR *pNMHDR, LRESULT *pResult)
{

	NMLVCUSTOMDRAW* pLVCD = reinterpret_cast<NMLVCUSTOMDRAW*>( pNMHDR );
	// Take the default processing unless we set this to something else below.
	*pResult = CDRF_DODEFAULT;
}