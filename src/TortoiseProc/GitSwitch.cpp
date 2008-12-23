// GitSwitch.cpp : implementation file
//

#include "stdafx.h"
#include "TortoiseProc.h"
#include "GitSwitch.h"


// CGitSwitch dialog

IMPLEMENT_DYNAMIC(CGitSwitch, CDialog)

CGitSwitch::CGitSwitch(CWnd* pParent /*=NULL*/)
	: CDialog(CGitSwitch::IDD, pParent)
{

}

CGitSwitch::~CGitSwitch()
{
}

void CGitSwitch::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CGitSwitch, CDialog)
END_MESSAGE_MAP()


// CGitSwitch message handlers
