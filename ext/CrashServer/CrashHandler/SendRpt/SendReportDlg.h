// Copyright 2012 Idol Software, Inc.
//
// This file is part of CrashHandler library.
//
// CrashHandler library is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#pragma once

#include "resource.h"       // main symbols

#include <atlhost.h>
#include <atlapp.h>
#include <atlctrls.h>
#include <atlctrlx.h>


class CStaticEx: public CWindowImpl<CStatic>
{
public:
    BEGIN_MSG_MAP(CStaticEx)
        MESSAGE_HANDLER(WM_PAINT, OnPaint)
    END_MSG_MAP()

    COLORREF m_TextColor;

    LRESULT OnPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
};

// CBaseDlgT

template <DWORD T>
class CBaseDlgT :
    public CAxDialogImpl<CBaseDlgT<T> >
{
    CString m_AppName, m_Company;
    HFONT m_Big;
    CStaticEx m_Header;
protected:
    typedef CBaseDlgT<T> Base;
    CStaticEx m_Text;
    CProgressBarCtrl m_Progress;
public:
    CBaseDlgT(const wchar_t* pszAppName, const wchar_t* pszCompany)
        : m_AppName(pszAppName), m_Company(pszCompany)
    {
    }

    enum { IDD = T };

    BEGIN_MSG_MAP(CBaseDlgT<T>)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        COMMAND_HANDLER(IDOK, BN_CLICKED, OnClickedOKCancel)
        COMMAND_HANDLER(IDCANCEL, BN_CLICKED, OnClickedOKCancel)
        CHAIN_MSG_MAP(CAxDialogImpl<CBaseDlgT<T> >)
    END_MSG_MAP()

    // Handler prototypes:
    //  LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    //  LRESULT CommandHandler(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    //  LRESULT NotifyHandler(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);

    CString SubstituteText(const CString& text)
    {
        CString res = text;
        res.Replace(_T("[AppName]"), m_AppName);
        res.Replace(_T("[Company]"), m_Company);
        return res;
    }

    void SubstituteText(CWindow& window)
    {
        CString text;
        window.GetWindowText(text);
        window.SetWindowText(SubstituteText(text));
    }

    virtual LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        CAxDialogImpl<CBaseDlgT<T> >::OnInitDialog(uMsg, wParam, lParam, bHandled);
        bHandled = TRUE;
        SetIcon(::LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_SENDRPT)));

        HFONT hFont = GetFont();
        LOGFONT lf;
        GetObject(hFont, sizeof(lf), &lf);
        lf.lfWeight = FW_THIN;
        lf.lfHeight = 20;
        m_Big = CreateFontIndirect(&lf);

        SubstituteText(*this);
        SubstituteText(GetDlgItem(IDC_HEADER_TEXT));
        SubstituteText(GetDlgItem(IDC_TEXT));

        GetDlgItem(IDC_HEADER_TEXT).SetFont(m_Big);
        m_Header.SubclassWindow(GetDlgItem(IDC_HEADER_TEXT));
        m_Header.m_TextColor = RGB(0, 0x33, 0x99);

        m_Text.m_TextColor = CDC(GetDlgItem(IDC_TEXT).GetDC()).GetTextColor();
        m_Text.SubclassWindow(GetDlgItem(IDC_TEXT));

        m_Progress = GetDlgItem(IDC_PROGRESS);
        if (m_Progress.IsWindow())
            m_Progress.SetMarquee(TRUE);

        return 1;  // Let the system set the focus
    }

    virtual LRESULT OnClickedOKCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
    {
        EndDialog(wID);
        return 0;
    }
};


// CSendReportDlg

class CSendReportDlg : public CBaseDlgT<IDD_SENDREPORTDLG>
{
public:
    CSendReportDlg(const wchar_t* pszAppName, const wchar_t* pszCompany)
        : CBaseDlgT(pszAppName, pszCompany)
    {
    }
};

// CSendAssertReportDlg

class CSendAssertReportDlg : public CBaseDlgT<IDD_SENDASSERTREPORTDLG>
{
public:
    CSendAssertReportDlg(const wchar_t* pszAppName, const wchar_t* pszCompany)
        : CBaseDlgT(pszAppName, pszCompany)
    {
    }
};

// CSendFullDumpDlg

class CSendFullDumpDlg : public CBaseDlgT<IDD_SENDFULLDUMPDLG>
{
    bool m_progressBegan;
public:
    CSendFullDumpDlg(const wchar_t* pszAppName, const wchar_t* pszCompany)
        : CBaseDlgT(pszAppName, pszCompany), m_progressBegan(false)
    {
    }

    BEGIN_MSG_MAP(CSendFullDumpDlg)
        MESSAGE_HANDLER(WM_USER, OnSetProgress)
        CHAIN_MSG_MAP(Base)
    END_MSG_MAP()

    LRESULT OnSetProgress(UINT uMsg, WPARAM wTotal, LPARAM lSent, BOOL& bHandled);
};

// CSolutionDlg

class CSolutionDlg : public CBaseDlgT<IDD_SOLUTIONDLG>
{
    CStaticEx m_Quest;
    CString m_question;
public:
    CSolutionDlg(const wchar_t* pszAppName, const wchar_t* pszCompany, DWORD question)
        : CBaseDlgT(pszAppName, pszCompany), m_question(MAKEINTRESOURCE(question))
    {
    }

    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) override;
};

// CAskSendFullDumpDlg

class CAskSendFullDumpDlg : public CBaseDlgT<IDD_ASKSENDFULLDUMPDLG>
{
    int m_nWidth, m_nFullHeight;
    CRichEditCtrl m_Details;
    CString m_url;
public:
    CAskSendFullDumpDlg(const wchar_t* pszAppName, const wchar_t* pszCompany, const wchar_t* pszUrl)
        : CBaseDlgT(pszAppName, pszCompany), m_url(pszUrl)
    {
    }

    BEGIN_MSG_MAP(CAskSendFullDumpDlg)
        COMMAND_HANDLER(IDC_DETAILS, BN_CLICKED, OnClickedDetails)
        NOTIFY_HANDLER(IDC_DETAILS_TEXT, EN_LINK, OnLinkClicked)
        CHAIN_MSG_MAP(Base)
    END_MSG_MAP()

    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) override;
    void SetDetailsText(const CString &text);
    LRESULT OnClickedDetails(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnLinkClicked(int windowId, LPNMHDR wParam, BOOL& bHandled);
    LRESULT OnClickedOKCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled) override;
};

