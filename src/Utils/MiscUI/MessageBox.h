// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2013-2017 - TortoiseGit
// Copyright (C) 2003-2008 - TortoiseSVN

// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software Foundation,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//
#pragma once

#include "dlgtemplate.h"
#include "cursor.h"

#define IDC_MESSAGEBOX_BUTTON1 101
#define IDC_MESSAGEBOX_BUTTON2 102
#define IDC_MESSAGEBOX_BUTTON3 103
#define IDC_MESSAGEBOX_CHECKBOX 104

#define MESSAGEBOX_BUTTONMARGIN 5
#define MESSAGEBOX_ICONMARGIN 15
#define MESSAGEBOX_BORDERMARGINX 10
#define MESSAGEBOX_BORDERMARGINY 10
#define MESSAGEBOX_TEXTBUTTONMARGIN 15
#define MESSAGEBOX_BUTTONCHECKMARGIN 5
#define MESSAGEBOX_BUTTONX 20
#define MESSAGEBOX_BUTTONY 5

/**
 * \ingroup Utils
 * Implements an enhanced MessageBox().\n
 *
 * You can use CMessageBox::Show() as a replacement for the Platform SDK version of
 * MessageBox(). Most of the flags are supported (param uType):\n
 * To indicate the buttons displayed in the message box, specify one of the following values:
 * <TABLE>
 * <TR VALIGN="top">
 * <TH align=left width=39%>Value</TH>
 * <TH align=left width=61%>Meaning</TH>
 * </TR>
 *
 * <TR VALIGN="top">
 * <TD width=39%>MB_ABORTRETRYIGNORE</TD>
 * <TD width=61%>The message box contains three push buttons: <B>Abort</B>, <B>Retry</B>, and <B>Ignore</B>.</TD>
 * </TR>
 *
 * <TR VALIGN="top">
 * <TD width=39%>MB_CANCELTRYCONTINUE</TD>
 * <TD width=61%>The message box contains three push buttons: <B>Cancel</B>, <B>Try Again</B>, <B>Continue</B>. Use this message box type instead of MB_ABORTRETRYIGNORE.</TD>
 * </TR>
 *
 * <TR VALIGN="top">
 * <TD width=39%>MB_OK</TD>
 * <TD width=61%>The message box contains one push button: <B>OK</B>. This is the default.</TD>
 * </TR>
 *
 * <TR VALIGN="top">
 * <TD width=39%>MB_OKCANCEL</TD>
 * <TD width=61%>The message box contains two push buttons: <B>OK</B> and <B>Cancel</B>.</TD>
 * </TR>
 *
 * <TR VALIGN="top">
 * <TD width=39%>MB_RETRYCANCEL</TD>
 * <TD width=61%>The message box contains two push buttons: <B>Retry</B> and <B>Cancel</B>.</TD>
 * </TR>
 *
 * <TR VALIGN="top">
 * <TD width=39%>MB_YESNO</TD>
 * <TD width=61%>The message box contains two push buttons: <B>Yes</B> and <B>No</B>.</TD>
 * </TR>
 *
 * <TR VALIGN="top">
 * <TD width=39%>MB_YESNOCANCEL</TD>
 * <TD width=61%>The message box contains three push buttons: <B>Yes</B>, <B>No</B>, and <B>Cancel</B>.</TD>
 * </TR>
 * </TABLE>
 * To display an icon in the message box, specify one of the following values
 * <TABLE>
 *
 * <TR VALIGN="top">
 * <TH align=left width=38%>Value</TH>
 * <TH align=left width=62%>Meaning</TH>
 * </TR>
 *
 * <TR VALIGN="top">
 * <TD width=38%>MB_ICONEXCLAMATION, <BR>
 * MB_ICONWARNING</TD>
 * <TD width=62%>An exclamation-point icon appears in the message box.</TD>
 * </TR>
 *
 * <TR VALIGN="top">
 * <TD width=38%>MB_ICONINFORMATION, MB_ICONASTERISK</TD>
 * <TD width=62%>An icon consisting of a lowercase letter <I>i</I> in a circle appears in the message box.</TD>
 * </TR>
 *
 * <TR VALIGN="top">
 * <TD width=38%>MB_ICONQUESTION</TD>
 * <TD width=62%>A question-mark icon appears in the message box.</TD>
 * </TR>
 *
 * <TR VALIGN="top">
 * <TD width=38%>MB_ICONSTOP, <BR>
 * MB_ICONERROR, <BR>
 * MB_ICONHAND</TD>
 * <TD width=62%>A stop-sign icon appears in the message box.</TD>
 * </TR>
 * </TABLE>
 * To indicate the default button, specify one of the following values
 * <TABLE>
 *
 * <TR VALIGN="top">
 * <TH align=left width=39%>Value</TH>
 * <TH align=left width=61%>Meaning</TH>
 * </TR>
 *
 * <TR VALIGN="top">
 * <TD width=39%>MB_DEFBUTTON1</TD>
 * <TD width=61%>The first button is the default button.
 * <P>MB_DEFBUTTON1 is the default unless MB_DEFBUTTON2, MB_DEFBUTTON3, or MB_DEFBUTTON4 is specified.</P>
 * </TD>
 * </TR>
 *
 * <TR VALIGN="top">
 * <TD width=39%>MB_DEFBUTTON2</TD>
 * <TD width=61%>The second button is the default button.</TD>
 * </TR>
 *
 * <TR VALIGN="top">
 * <TD width=39%>MB_DEFBUTTON3</TD>
 * <TD width=61%>The third button is the default button.</TD>
 * </TR>
 *
 * <TR VALIGN="top">
 * <TD width=39%>MB_DEFBUTTON4</TD>
 * <TD width=61%>The fourth button is the default button.</TD>
 * </TR>
 * </TABLE>
 *
 * <b>return values:</b>\n
 * <TABLE>
 *
 * <TR VALIGN="top">
 * <TH align=left width=22%>Value</TH>
 * <TH align=left width=78%>Meaning</TH>
 * </TR>
 *
 * <TR VALIGN="top">
 * <TD width=22%>IDABORT</TD>
 * <TD width=78%><B>Abort</B> button was selected.</TD>
 * </TR>
 *
 * <TR VALIGN="top">
 * <TD width=22%>IDCANCEL</TD>
 * <TD width=78%><B>Cancel</B> button was selected.</TD>
 * </TR>
 *
 * <TR VALIGN="top">
 * <TD width=22%>IDCONTINUE</TD>
 * <TD width=78%><B>Continue</B> button was selected.</TD>
 * </TR>
 *
 * <TR VALIGN="top">
 * <TD width=22%>IDIGNORE</TD>
 * <TD width=78%><B>Ignore</B> button was selected.</TD>
 * </TR>
 *
 * <TR VALIGN="top">
 * <TD width=22%>IDNO</TD>
 * <TD width=78%><B>No</B> button was selected.</TD>
 * </TR>
 *
 * <TR VALIGN="top">
 * <TD width=22%>IDOK</TD>
 * <TD width=78%><B>OK</B> button was selected.</TD>
 * </TR>
 *
 * <TR VALIGN="top">
 * <TD width=22%>IDRETRY</TD>
 * <TD width=78%><B>Retry</B> button was selected.</TD>
 * </TR>
 *
 * <TR VALIGN="top">
 * <TD width=22%>IDTRYAGAIN</TD>
 * <TD width=78%><B>Try Again</B> button was selected.</TD>
 * </TR>
 *
 * <TR VALIGN="top">
 * <TD width=22%>IDYES</TD>
 * <TD width=78%><B>Yes</B> button was selected.</TD>
 * </TR>
 * </TABLE>
 * If a message box has a <B>Cancel</B> button, the function returns the IDCANCEL value
 * if either the ESC key is pressed or the <B>Cancel</B> button is selected. If the
 * message box has no <B>Cancel</B> button, pressing ESC has no effect.
 *
 * To get I18L you can define the following strings in your resource string table:\n
 * - IDS_MSGBOX_ABORT
 * - IDS_MSGBOX_RETRY
 * - IDS_MSGBOX_IGNORE
 * - IDS_MSGBOX_CANCEL
 * - IDS_MSGBOX_TRYAGAIN
 * - IDS_MSGBOX_CONTINUE
 * - IDS_MSGBOX_OK
 * - IDS_MSGBOX_YES
 * - IDS_MSGBOX_NO
 * - IDS_MSGBOX_DONOTSHOWAGAIN
 *
 * It is also possible to set the button texts for each one of the three buttons.
 * Use the corresponding method provided for this.
 * \image html "CMessageBox_3.jpg"
 *
 * For the "Do not show again" functionality you can define the registry base path
 * for storing the return values with\n
 * \code
 * #define XMESSAGEBOX_APPREGPATH "Software\\MyApplication\\MsgDialogs\\"
 * \endcode
 * if you don't do that define then CMessageBox will use the default path "Software\\YourApplicationName\\"
 */
class CMessageBox : public CDialog
{
public:
	CMessageBox(void);
	~CMessageBox(void);
	/**
	 * Shows a message box. Use this as a replacement for the usual ::MessageBox() calls.
	 * Most of the flags of the Platform SDK version are supported. See the class descriptions
	 * for details.
	 * \param hWnd handle to the parent window or nullptr
	 * \param lpMessage the message string to show on the message box
	 * \param lpCaption the dialog title
	 * \param uType see class description for details
	 * \param sHelpPath if uType has MB_HELP, the path for the help file
	 * \return see class descriptions for details
	 */
	static UINT Show(HWND hWnd, LPCTSTR lpMessage, LPCTSTR lpCaption, UINT uType, LPCTSTR sHelpPath = nullptr);
	/**
	 * Shows a message box.
	 * \param hWnd handle to the parent window or nullptr
	 * \param nMessage resource ID of the message string
	 * \param nCaption resource ID of the title string
	 * \param uType see class description for details
	 * \param sHelpPath if uType has MB_HELP, this is the path of the help file to use
	 * \return see class description for details
	 */
	static UINT Show(HWND hWnd, UINT nMessage, UINT nCaption, UINT uType, LPCTSTR sHelpPath = nullptr);
	/**
	* Shows a message box.
	* \param hWnd handle to the parent window or nullptr
	* \param nMessage resource ID of the message string
	* \param nCaption resource ID of the title string
	* \param uType see class description for details
	* \param nHelpID if uType has MB_HELP, this is the help ID to use
	* \return see class description for details
	*/
	static UINT Show(HWND hWnd, UINT nMessage, UINT nCaption, UINT uType, UINT nHelpID);
	/**
	 * Shows a message box with a checkbox. If the user checks it then the next time
	 * the message box isn't shown anymore - the method simply returns the same value as
	 * the last time. Use this to give the user the possibility to ignore specific message
	 * boxes ("Do not show again"-checkbox).
	 * \param hWnd handle to the parent window or nullptr
	 * \param lpMessage the message string to show on the message box
	 * \param lpCaption the dialog title
	 * \param uType see class description for details
	 * \param lpRegistry a value string to store the return value of this specific message box. Put nullptr or empty if you do not use registry.
	 * Each one of your message boxes must have it's own value string! Examples for such values
	 * might be "WarnOverwrite", "InformAboutMissingMailSubject", ...
	 * \param lpCheckMessage the message to show on the checkbox label. If this parameter is omitted
	 * then it defaults to "do not show again" or the string with resource ID IDS_MSGBOX_DONOTSHOWAGAIN.
	 * \param bChecked get and set checkbox check state. This is optional.
	 * \return see class description for details
	 */
	static UINT ShowCheck(HWND hWnd, LPCTSTR lpMessage, LPCTSTR lpCaption, UINT uType, LPCTSTR lpRegistry, LPCTSTR lpCheckMessage = nullptr, BOOL *bChecked = nullptr);
	/**
	 * Shows a message box with a checkbox. If the user checks it then the next time
	 * the message box isn't shown anymore - the method simply returns the same value as
	 * the last time. Use this to give the user the possibility to ignore specific message
	 * boxes ("Do not show again"-checkbox).
	 * \param hWnd handle to the parent window or nullptr
	 * \param nMessage resource ID of the message string
	 * \param nCaption resource ID of the title string
	 * \param uType see class description for details
	 * \param lpRegistry a value string to store the return value of this specific message box. Put nullptr or empty if you do not use registry.
	 * Each one of your message boxes must have it's own value string! Examples for such values
	 * might be "WarnOverwrite", "InformAboutMissingMailSubject", ...
	 * \param nCheckMessage resource ID of the checkbox string
	 * \param bChecked get and set checkbox check state. This is optional.
	 * \return see class description for details
	 */
	static UINT ShowCheck(HWND hWnd, UINT nMessage, UINT nCaption, UINT uType, LPCTSTR lpRegistry, UINT nCheckMessage, BOOL* bChecked = nullptr);

	/**
	 * Shows a message box with user defined button texts.
	 * \param hWnd handle to the parent window or nullptr
	 * \param lpMessage the message string
	 * \param lpCaption the title string
	 * \param nDef number of the default button (1,2 or 3)
	 * \param icon an icon loaded with MAKEINTRESOURCE() or one of
	 * the system default icons.
	 * \param lpButton1 text for the first button
	 * \param lpButton2 text for the second button
	 * \param lpButton3 text for the third button
	 * \return the number of the button pressed (1,2 or 3)
	 */
	static UINT Show(HWND hWnd, LPCTSTR lpMessage, LPCTSTR lpCaption, int nDef, LPCTSTR icon, LPCTSTR lpButton1, LPCTSTR lpButton2 = nullptr, LPCTSTR lpButton3 = nullptr);
	/**
	 * Shows a message box with user defined button texts.
	 * \param hWnd handle to the parent window or nullptr
	 * \param nMessage resource ID of the message string
	 * \param nCaption resource ID of the title string
	 * \param nDef number of the default button (1,2 or 3)
	 * \param icon an icon loaded with MAKEINTRESOURCE() or one of
	 * the system default icons.
	 * \param nButton1 resource ID of the text for the first button
	 * \param nButton2 resource ID of the text for the second button
	 * \param nButton3 resource ID of the text for the third button
	 * \return the number of the button pressed (1,2 or 3)
	 */
	static UINT Show(HWND hWnd, UINT nMessage, UINT nCaption, int nDef, LPCTSTR icon, UINT nButton1, UINT nButton2 = NULL, UINT nButton3 = NULL);
	/**
	 * Shows a message box with user defined button texts and a checkbox.
	 * \param hWnd handle to the parent window or nullptr
	 * \param lpMessage the message string
	 * \param lpCaption the title string
	 * \param nDef number of the default button (1,2 or 3)
	 * \param icon an icon loaded with MAKEINTRESOURCE() or one
	 * of the system default icons
	 * \param lpButton1 string for the first button
	 * \param lpButton2 string for the second button
	 * \param lpButton3 string for the third button
	 * \param lpRegistry  a value string to store the return value of this specific message box. Put nullptr or empty if you do not use registry.
	 * Each one of your message boxes must have it's own value string! Examples for such values
	 * might be "WarnOverwrite", "InformAboutMissingMailSubject", ...
	 * \param lpCheckMessage the message to show on the checkbox label. If this parameter is omitted
	 * then it defaults to "do not show again" or the string with resource ID IDS_MSGBOX_DONOTSHOWAGAIN.
	 * \param bChecked get and set checkbox check state. This is optional.
	 * \return the number of the button pressed (1,2 or 3)
	 */
	static UINT ShowCheck(HWND hWnd, LPCTSTR lpMessage, LPCTSTR lpCaption, int nDef, LPCTSTR icon, LPCTSTR lpButton1, LPCTSTR lpButton2, LPCTSTR lpButton3, LPCTSTR lpRegistry, LPCTSTR lpCheckMessage = nullptr, BOOL* bChecked = nullptr);
	/**
	 * Shows a message box with user defined button texts and a checkbox.
	 * \param hWnd handle to the parent window or nullptr
	 * \param nMessage resource ID of the message string
	 * \param nCaption resource ID of the title string
	 * \param nDef number of the default button (1,2 or 3)
	 * \param icon an icon loaded with MAKEINTRESOURCE() or one
	 * of the system default icons
	 * \param nButton1 resource ID of string for the first button
	 * \param nButton2 resource ID of string for the second button
	 * \param nButton3 resource ID of string for the third button
	 * \param lpRegistry  a value string to store the return value of this specific message box. Put nullptr or empty if you do not use registry.
	 * Each one of your message boxes must have it's own value string! Examples for such values
	 * might be "WarnOverwrite", "InformAboutMissingMailSubject", ...
	 * \param nCheckMessage resource ID of the checkbox string
	 * \param bChecked get and set checkbox check state. This is optional.
	 * \return the number of the button pressed (1,2 or 3)
	 */
	static UINT ShowCheck(HWND hWnd, UINT nMessage, UINT nCaption, int nDef, LPCTSTR icon, UINT nButton1, UINT nButton2, UINT nButton3, LPCTSTR lpRegistry, UINT nCheckMessage = NULL, BOOL* bChecked = nullptr);

	/**
	 * Removes the registry key
	 */
	static bool RemoveRegistryKey(LPCTSTR lpRegistry);

	/**
	 * Stores the value in the registry
	 * \param sValue the value name
	 * \param value DWORD to store
	 */
	static void SetRegistryValue(const CString& sValue, DWORD value);

protected:
	/**
	 * Shows the modal dialog
	 * \param pWnd handle to the parent window or nullptr
	 * \param title message box title
	 * \param msg message to show
	 * \param nDefaultButton number of the default button
	 * \return the value stored in the member variables for the buttons (m_uButtonXRet)
	 * of the button pressed
	 */
	UINT GoModal(CWnd * pWnd, const CString& title, const CString& msg, int nDefaultButton);

	/**
	 * Fills in the member variables according to the uType parameter
	 */
	int FillBoxStandard(UINT uType);

	/**
	 * Calculates the size of the string in pixels
	 * \param str the string to check the size
	 */
	CSize GetTextSize(const CString& str);
	/**
	 * Returns the size of the icon
	 */
	CSize GetIconSize(HICON hIcon);
	/**
	 * Returns the size of all four buttons (three pushbuttons and the checkbox).
	 * Also resizes the buttons accordingly and fills in m_szAllButtons and m_szButtons.
	 */
	CSize GetButtonSize();

	void SetHelpPath(LPCTSTR sHelpPath) {m_sHelpPath = sHelpPath;}

	LOGFONT	m_LogFont;
	CCursor m_Cursor;
	CString m_sMessage;
	HICON m_hIcon;
	BOOL m_bDestroyIcon;
	int m_nDefButton;
	CString m_sButton1;
	CString m_sButton2;
	CString m_sButton3;
	CString m_sCheckbox;
	CString m_sHelpPath;
	UINT m_uButton1Ret;
	UINT m_uButton2Ret;
	UINT m_uButton3Ret;
	UINT m_uCancelRet;
	UINT m_uType;
	CSize m_szIcon;
	CSize m_szAllButtons;
	CSize m_szButtons;
	CSize m_szText;

	BOOL m_bShowCheck;
	CString m_sRegistryValue;

	CString m_i18l;			//only used if some strings are defined for internationalization

public:
	DECLARE_MESSAGE_MAP()
	afx_msg void OnPaint();
	afx_msg void OnButton1();
	afx_msg void OnButton2();
	afx_msg void OnButton3();
	virtual BOOL OnInitDialog() override;

	BOOL m_bChecked;
protected:
	virtual void OnCancel() override;
	virtual BOOL PreTranslateMessage(MSG* pMsg) override;
};
