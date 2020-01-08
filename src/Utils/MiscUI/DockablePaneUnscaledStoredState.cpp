// TortoiseGit - a Windows shell extension for easy version control

// heavily based on the code of afxpane.cpp of the Microsoft MFC Framework

#pragma once

#include "stdafx.h"
#include "DockablePaneUnscaledStoredState.h"
#include "afxregpath.h"
#include "afxsettingsstore.h"
#include "DPIAware.h"

#define AFX_REG_SECTION_FMT _T("%TsPane-%d")
#define AFX_REG_SECTION_FMT_EX _T("%TsPane-%d%x")

BOOL CDockablePaneUnscaledStoredState::LoadState(LPCTSTR lpszProfileName, int nIndex, UINT uiID)
{
	CString strProfileName = ::AFXGetRegPath(AFX_CONTROL_BAR_PROFILE, lpszProfileName);

	if (nIndex == -1)
		nIndex = GetDlgCtrlID();

	CString strSection;
	if (uiID == (UINT)-1)
		strSection.Format(AFX_REG_SECTION_FMT, (LPCTSTR)strProfileName, nIndex);
	else
		strSection.Format(AFX_REG_SECTION_FMT_EX, (LPCTSTR)strProfileName, nIndex, uiID);

	CSettingsStoreSP regSP;
	CSettingsStore& reg = regSP.Create(FALSE, TRUE);

	if (!reg.Open(strSection))
		return FALSE;

	reg.Read(_T("ID"), (int&)m_nID);

	reg.Read(_T("RectRecentFloat"), m_recentDockInfo.m_rectRecentFloatingRect);
	reg.Read(_T("RectRecentDocked"), m_rectSavedDockedRect);

	CDPIAware::Instance().ScaleRect(&m_recentDockInfo.m_rectRecentFloatingRect);
	CDPIAware::Instance().ScaleRect(&m_rectSavedDockedRect);

	m_recentDockInfo.m_recentSliderInfo.m_rectDockedRect = m_rectSavedDockedRect;

	reg.Read(_T("RecentFrameAlignment"), m_recentDockInfo.m_dwRecentAlignmentToFrame);
	reg.Read(_T("RecentRowIndex"), m_recentDockInfo.m_nRecentRowIndex);
	reg.Read(_T("IsFloating"), m_bRecentFloatingState);
	reg.Read(_T("MRUWidth"), m_nMRUWidth);
	reg.Read(_T("PinState"), m_bPinState);

	return CBasePane::LoadState(lpszProfileName, nIndex, uiID); // skip CDockablePane!
}

BOOL CDockablePaneUnscaledStoredState::SaveState(LPCTSTR lpszProfileName, int nIndex, UINT uiID)
{
	CString strProfileName = ::AFXGetRegPath(AFX_CONTROL_BAR_PROFILE, lpszProfileName);

	if (nIndex == -1)
		nIndex = GetDlgCtrlID();

	CString strSection;
	if (uiID == (UINT)-1)
		strSection.Format(AFX_REG_SECTION_FMT, (LPCTSTR)strProfileName, nIndex);
	else
		strSection.Format(AFX_REG_SECTION_FMT_EX, (LPCTSTR)strProfileName, nIndex, uiID);

	CSettingsStoreSP regSP;
	CSettingsStore& reg = regSP.Create(FALSE, FALSE);

	if (reg.CreateKey(strSection))
	{
		BOOL bFloating = IsFloating();

		if (bFloating)
		{
			CPaneFrameWnd* pMiniFrame = GetParentMiniFrame();
			if (pMiniFrame)
				pMiniFrame->GetWindowRect(m_recentDockInfo.m_rectRecentFloatingRect);
		}
		else
		{
			CalcRecentDockedRect();
			if (m_pParentDockBar)
			{
				m_recentDockInfo.m_dwRecentAlignmentToFrame = m_pParentDockBar->GetCurrentAlignment();
				m_recentDockInfo.m_nRecentRowIndex = m_pParentDockBar->FindRowIndex(m_pDockBarRow);
			}
		}

		reg.Write(_T("ID"), (int&)m_nID);

		CRect floatingRect = m_recentDockInfo.m_rectRecentFloatingRect;
		CRect dockedRect = m_recentDockInfo.m_recentSliderInfo.m_rectDockedRect;
		CDPIAware::Instance().UnscaleRect(&floatingRect);
		CDPIAware::Instance().UnscaleRect(&dockedRect);

		reg.Write(_T("RectRecentFloat"), floatingRect);
		reg.Write(_T("RectRecentDocked"), dockedRect);

		reg.Write(_T("RecentFrameAlignment"), m_recentDockInfo.m_dwRecentAlignmentToFrame);
		reg.Write(_T("RecentRowIndex"), m_recentDockInfo.m_nRecentRowIndex);
		reg.Write(_T("IsFloating"), bFloating);
		reg.Write(_T("MRUWidth"), m_nMRUWidth);
		reg.Write(_T("PinState"), m_bPinState);
	}
	return CBasePane::SaveState(lpszProfileName, nIndex, uiID); // skip CDockablePane!
}
