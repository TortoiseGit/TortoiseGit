// Copyright 2014 Idol Software, Inc.
//
// This file is part of Doctor Dump SDK.
//
// Doctor Dump SDK is free software: you can redistribute it and/or modify
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

#include "StdAfx.h"
#include "SendReportDlg.h"

LRESULT CStaticEx::OnPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    PAINTSTRUCT ps;
    CDCHandle dc(BeginPaint(&ps));

    dc.SetTextColor(m_TextColor);

    CFontHandle font(GetFont());
    dc.SelectFont(font);

    RECT rect;
    GetClientRect(&rect);

    CString text;
    GetWindowText(text);
    dc.DrawText(text, -1, &rect, DT_END_ELLIPSIS | DT_WORDBREAK);

    EndPaint(&ps);

    bHandled = TRUE;

    return 0;
}

LRESULT CSolutionDlg::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    LRESULT res = Base::OnInitDialog(uMsg, wParam, lParam, bHandled);
    GetDlgItem(IDC_QUESTION).SetWindowText(m_question);

    m_Quest.m_TextColor = CDC(GetDlgItem(IDC_QUESTION).GetDC()).GetTextColor();
    m_Quest.SubclassWindow(GetDlgItem(IDC_QUESTION));

    return res;
}

LRESULT CSendFullDumpDlg::OnSetProgress(UINT uMsg, WPARAM wTotal, LPARAM lSent, BOOL& bHandled)
{
    if (m_Progress.IsWindow())
    {
        if (!m_progressBegan)
        {
            m_Text.LockWindowUpdate(); // without that CStaticEx::OnPaint doesn't called ???
            m_Text.SetWindowText(m_translator.GetString(L"SendingData"));
            m_Text.LockWindowUpdate(FALSE);
            m_Progress.ModifyStyle(PBS_MARQUEE, 0);
            m_Progress.SetRange(0, 1000); // 1000 to make moving smooth (not less than pixels in progress bar)
            m_progressBegan = true;
        }
        m_Progress.SetPos(int((1000.0 * lSent) / wTotal));
    }
    return 0;
}

LRESULT CAskSendFullDumpDlg::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    Base::OnInitDialog(uMsg, wParam, lParam, bHandled);
    RECT rect_client, rect_main, rect_details;
    GetClientRect(&rect_client);
    ClientToScreen(&rect_client);
    GetWindowRect(&rect_main);
    GetDlgItem(IDC_DETAILS).GetWindowRect(&rect_details);
    m_nWidth = rect_client.right - rect_client.left;
    m_nFullHeight = rect_client.bottom - rect_client.top;
    int nNewHeight = rect_details.bottom - rect_client.top + 8;

    ResizeClient(m_nWidth, nNewHeight);

    m_Details = GetDlgItem(IDC_DETAILS_TEXT);
    m_Details.SetEventMask(ENM_LINK);

    SetDetailsText(m_translator.GetString(L"PrivateInfoText"));

    return 0;
}

void CAskSendFullDumpDlg::SetDetailsText(const CString &text)
{
    CString clearText = text;
    std::vector<std::pair<DWORD, DWORD>> links;
    int pos;
    while ((pos = clearText.Find(_T("<a>"))) != -1)
    {
        DWORD beg = pos;
        clearText.Delete(pos, 3);
        pos = clearText.Find(_T("</a>"));
        if (pos != -1)
        {
            links.push_back(std::make_pair(beg, DWORD(pos)));
            clearText.Delete(pos, 4);
        }
    }

    m_Details.SetWindowTextW(clearText);

    CHARRANGE sel;
    m_Details.GetSel(sel);

    CHARFORMAT2 cf;
    cf.cbSize = sizeof(cf);
    cf.dwMask = CFM_LINK | CFM_WEIGHT;
    cf.dwEffects = CFE_LINK;
    cf.wWeight = FW_BOLD;

    for (size_t i = 0; i < links.size(); ++i)
    {
        m_Details.SetSel(links[i].first, links[i].second);
        m_Details.SetWordCharFormat(cf);
    }

    m_Details.SetSel(sel);
}

LRESULT CAskSendFullDumpDlg::OnClickedDetails(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    ResizeClient(m_nWidth, m_nFullHeight);
    GetDlgItem(IDC_DETAILS).EnableWindow(FALSE);
    return 0;
}

LRESULT CAskSendFullDumpDlg::OnLinkClicked(int windowId, LPNMHDR wParam, BOOL& bHandled)
{
    ENLINK* enLink = (ENLINK*) wParam;
    if (enLink->msg != WM_LBUTTONUP)
        return 0;

    if (!m_url.IsEmpty())
        ShellExecute(NULL, _T("open"), CW2CT(m_url), NULL, NULL, SW_SHOWNORMAL);

    bHandled = TRUE;

    return 0;
}

LRESULT CAskSendFullDumpDlg::OnClickedOKCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    if (wID == IDCANCEL)
    {
        if (IDNO == MessageBox(m_translator.GetString(L"MotivateToSendFull"), m_translator.GetAppName(), MB_ICONWARNING | MB_YESNO | MB_DEFBUTTON2))
            return 0;
    }
    EndDialog(wID);
    return 0;
}
