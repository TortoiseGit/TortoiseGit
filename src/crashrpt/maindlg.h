#pragma once

#include "BaseDialog.h"
#include "Utility.h"
#include "CrashHandler.h"


////////////////////////////// Class Definitions /////////////////////////////

// ===========================================================================
// CMainDlg
// 
// See the module comment at top of file.
//
class CMainDlg : public CDialog
{
public:
	CMainDlg(void);
	~CMainDlg(void);

protected:
	LRESULT CALLBACK		DlgFunc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT					DoCommand(int id);

public:

   string     m_sEmail;         // Email: From
   string     m_sDescription;   // Email: Body
   TStrStrVector  *m_pUDFiles;      // Files <name,desc>
   BOOL        m_sendButton;     // Display 'Send' or 'Save' button
};

/////////////////////////////////////////////////////////////////////////////
