﻿// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2020, 2023 - TortoiseGit
// Copyright (C) 2020-2021 - TortoiseSVN

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
#include "stdafx.h"
#include "Theme.h"
#include "resource.h"
#include "PathUtils.h"
#include "StringUtils.h"
#include "DarkModeHelper.h"
#include "DPIAware.h"
#include <Uxtheme.h>
#include <vssym32.h>
#include <richedit.h>
#pragma warning(push)
#pragma warning(disable : 4458) // declaration of 'xxx' hides class member
#include <gdiplus.h>
#pragma warning(pop)
#include "SmartHandle.h"

#define WM_MENUBAR_DRAWMENU 0x0091 // lParam is MENUBAR_MENU
#define WM_MENUBAR_DRAWMENUITEM 0x0092 // lParam is MENUBAR_DRAWMENUITEM

// describes the sizes of the menu bar or menu item
union MenubarMenuitemmetrics
{
	struct
	{
		DWORD cx;
		DWORD cy;
	} rgSizeBar[2];
	struct
	{
		DWORD cx;
		DWORD cy;
	} rgSizePopup[4];
};

struct MenubarMenupopupmetrics
{
	DWORD rgCx[4];
	DWORD fUpdateMaxWidths : 2;
};

struct MenubarMenu
{
	HMENU hMenu; // main window menu
	HDC hdc;
	DWORD dwFlags;
};

struct MenubarMenuitem
{
	int iPosition; // 0-based position
	MenubarMenuitemmetrics umIm;
	MenubarMenupopupmetrics umpm;
};

struct MenubarDrawmenuitem
{
	DRAWITEMSTRUCT dis; // itemID looks uninitialized
	MenubarMenu um;
	MenubarMenuitem umi;
};

struct MenubarMeasuremenuitem
{
	MEASUREITEMSTRUCT mis;
	MenubarMenu um;
	MenubarMenuitem umi;
};

constexpr auto SubclassID = 1234;

static int GetStateFromBtnState(LONG_PTR dwStyle, BOOL bHot, BOOL bFocus, LRESULT dwCheckState, int iPartId, BOOL bHasMouseCapture);
static void GetRoundRectPath(Gdiplus::GraphicsPath* pPath, const Gdiplus::Rect& r, int dia);
static void DrawRect(LPRECT prc, HDC hdcPaint, Gdiplus::DashStyle dashStyle, Gdiplus::Color clr, Gdiplus::REAL width);
static void DrawFocusRect(LPRECT prcFocus, HDC hdcPaint);
static void PaintControl(HWND hWnd, HDC hdc, RECT* prc, bool bDrawBorder);
static BOOL DetermineGlowSize(int* piSize, LPCWSTR pszClassIdList = nullptr);
static BOOL GetEditBorderColor(HWND hWnd, COLORREF* pClr);

HBRUSH CTheme::s_backBrush = nullptr;
HBRUSH CTheme::s_backHotBrush = nullptr;

CTheme::CTheme()
	: m_regDarkTheme(REGSTRING_DARKTHEME, 0) // define REGSTRING_DARKTHEME on app level as e.g. L"Software\\TortoiseMerge\\DarkTheme"
{
}

CTheme::~CTheme()
{
	if (s_backBrush)
		DeleteObject(s_backBrush);
	if (s_backHotBrush)
		DeleteObject(s_backHotBrush);
}

CTheme& CTheme::Instance()
{
	static CTheme instance;
	if (!instance.m_bLoaded)
		instance.Load();

	return instance;
}

void CTheme::Load()
{
	IsDarkModeAllowed();
	OnSysColorChanged();
	m_dark = !!m_regDarkTheme && IsDarkModeAllowed() && !IsHighContrastMode() && DarkModeHelper::Instance().ShouldAppsUseDarkMode();
	m_bLoaded = true;
}

bool CTheme::IsHighContrastMode() const
{
	return m_isHighContrastMode;
}

bool CTheme::IsHighContrastModeDark() const
{
	return m_isHighContrastModeDark;
}

COLORREF CTheme::GetThemeColor(COLORREF clr, bool fixed /*= false*/) const
{
	if (m_dark || (fixed && m_isHighContrastModeDark))
	{
		float h, s, l;
		CTheme::RGBtoHSL(clr, h, s, l);
		l = 100.0f - l;
		if (!m_isHighContrastModeDark)
		{
			// to avoid too much contrast, prevent
			// too dark and too bright colors.
			// this is because in dark mode, contrast is
			// much more visible.
			l = std::clamp(l, 5.0f, 90.0f);
		}
		return CTheme::HSLtoRGB(h, s, l);
	}

	return clr;
}

Gdiplus::Color CTheme::GetThemeColor(Gdiplus::Color clr, bool fixed /*= false*/) const
{
	if (m_dark || (fixed && m_isHighContrastModeDark))
	{
		Gdiplus::Color color;
		color.SetFromCOLORREF(GetThemeColor(clr.ToCOLORREF(), fixed));
		return color;
	}
	return clr;
}

int CTheme::RegisterThemeChangeCallback(ThemeChangeCallback&& cb)
{
	++m_lastThemeChangeCallbackId;
	int nextThemeCallBackId = m_lastThemeChangeCallbackId;
	m_themeChangeCallbacks.emplace(nextThemeCallBackId, std::move(cb));
	return nextThemeCallBackId;
}

bool CTheme::RemoveRegisteredCallback(int id)
{
	auto foundIt = m_themeChangeCallbacks.find(id);
	if (foundIt != m_themeChangeCallbacks.end())
	{
		m_themeChangeCallbacks.erase(foundIt);
		return true;
	}
	return false;
}

bool CTheme::SetThemeForDialog(HWND hWnd, bool bDark)
{
	ASSERT(hWnd);
	DarkModeHelper::Instance().AllowDarkModeForWindow(hWnd, bDark);
	if (bDark && !DarkModeHelper::Instance().CanHaveDarkMode())
		return false;
	if (bDark)
		SetWindowSubclass(hWnd, MainSubclassProc, SubclassID, reinterpret_cast<DWORD_PTR>(&s_backBrush));
	else
		RemoveWindowSubclass(hWnd, MainSubclassProc, SubclassID);
	AdjustThemeForChildrenProc(hWnd, bDark ? TRUE : FALSE);
	EnumChildWindows(hWnd, AdjustThemeForChildrenProc, bDark ? TRUE : FALSE);
	EnumThreadWindows(GetCurrentThreadId(), AdjustThemeForChildrenProc, bDark ? TRUE : FALSE);
	DarkModeHelper::Instance().RefreshTitleBarThemeColor(hWnd, bDark);
	::RedrawWindow(hWnd, nullptr, nullptr, RDW_FRAME | RDW_INVALIDATE | RDW_ERASE | RDW_INTERNALPAINT | RDW_ALLCHILDREN | RDW_UPDATENOW);
	DarkModeHelper::Instance().RefreshTitleBarThemeColor(hWnd, bDark);
	return true;
}

BOOL CTheme::AdjustThemeForChildrenProc(HWND hwnd, LPARAM lParam)
{
	DarkModeHelper::Instance().AllowDarkModeForWindow(hwnd, static_cast<BOOL>(lParam));
	wchar_t szWndClassName[MAX_PATH] = { 0 };
	GetClassName(hwnd, szWndClassName, _countof(szWndClassName));
	if (lParam)
	{
		if ((wcscmp(szWndClassName, WC_LISTVIEW) == 0) || (wcscmp(szWndClassName, WC_LISTBOX) == 0))
		{
			// theme "Explorer" also gets the scrollbars with dark mode, but the hover color
			// is the blueish from the bright mode.
			// theme "ItemsView" has the hover color the same as the windows explorer (grayish),
			// but then the scrollbars are not drawn for dark mode.
			// theme "DarkMode_Explorer" doesn't paint a hover color at all.
			//
			// Also, the group headers are not affected in dark mode and therefore the group texts are
			// hardly visible.
			//
			// so use "Explorer" for now. The downside of the bluish hover color isn't that bad,
			// except in situations where both a treeview and a listview are on the same dialog
			// at the same time (e.g. repobrowser) - then the difference is unfortunately very
			// noticeable...
			SetWindowTheme(hwnd, L"Explorer", nullptr);
			auto header = ListView_GetHeader(hwnd);
			DarkModeHelper::Instance().AllowDarkModeForWindow(header, static_cast<BOOL>(lParam));
			SetWindowTheme(header, L"Explorer", nullptr);
			ListView_SetTextColor(hwnd, darkTextColor);
			ListView_SetTextBkColor(hwnd, darkBkColor);
			ListView_SetBkColor(hwnd, darkBkColor);
			if (auto hTT = ListView_GetToolTips(hwnd))
			{
				DarkModeHelper::Instance().AllowDarkModeForWindow(hTT, static_cast<BOOL>(lParam));
				SetWindowTheme(hTT, L"Explorer", nullptr);
			}
			SetWindowSubclass(hwnd, ListViewSubclassProc, SubclassID, reinterpret_cast<DWORD_PTR>(&s_backBrush));
		}
		else if (wcscmp(szWndClassName, WC_HEADER) == 0)
			SetWindowTheme(hwnd, L"ItemsView", nullptr);
		else if (wcscmp(szWndClassName, WC_BUTTON) == 0)
		{
			SetWindowTheme(hwnd, L"Explorer", nullptr);
			auto style = GetWindowLongPtr(hwnd, GWL_STYLE) & 0x0F;
			if ((style & BS_GROUPBOX) == BS_GROUPBOX)
				SetWindowSubclass(hwnd, ButtonSubclassProc, SubclassID, reinterpret_cast<DWORD_PTR>(&s_backBrush));
			else if (style == BS_CHECKBOX || style == BS_AUTOCHECKBOX || style == BS_3STATE || style == BS_AUTO3STATE || style == BS_RADIOBUTTON || style == BS_AUTORADIOBUTTON)
				SetWindowSubclass(hwnd, ButtonSubclassProc, SubclassID, reinterpret_cast<DWORD_PTR>(&s_backBrush));
		}
		else if (wcscmp(szWndClassName, WC_STATIC) == 0)
			SetWindowTheme(hwnd, L"", L"");
		else if (wcscmp(szWndClassName, L"SysDateTimePick32") == 0)
		{
			SetWindowTheme(hwnd, L"Explorer", nullptr);
		}
		else if ((wcscmp(szWndClassName, WC_COMBOBOXEX) == 0) ||
				 (wcscmp(szWndClassName, L"ComboLBox") == 0) ||
				 (wcscmp(szWndClassName, WC_COMBOBOX) == 0))
		{
			SetWindowTheme(hwnd, L"Explorer", nullptr);
			HWND hCombo = hwnd;
			if (wcscmp(szWndClassName, WC_COMBOBOXEX) == 0)
			{
				SendMessage(hwnd, CBEM_SETWINDOWTHEME, 0, reinterpret_cast<LPARAM>(L"Explorer"));
				hCombo = reinterpret_cast<HWND>(SendMessage(hwnd, CBEM_GETCOMBOCONTROL, 0, 0));
			}
			if (hCombo)
			{
				SetWindowSubclass(hCombo, ComboBoxSubclassProc, SubclassID, reinterpret_cast<DWORD_PTR>(&s_backBrush));
				COMBOBOXINFO info = { 0 };
				info.cbSize = sizeof(COMBOBOXINFO);
				if (SendMessage(hCombo, CB_GETCOMBOBOXINFO, 0, reinterpret_cast<LPARAM>(&info)))
				{
					DarkModeHelper::Instance().AllowDarkModeForWindow(info.hwndList, static_cast<BOOL>(lParam));
					DarkModeHelper::Instance().AllowDarkModeForWindow(info.hwndItem, static_cast<BOOL>(lParam));
					DarkModeHelper::Instance().AllowDarkModeForWindow(info.hwndCombo, static_cast<BOOL>(lParam));

					SetWindowTheme(info.hwndList, L"Explorer", nullptr);
					SetWindowTheme(info.hwndItem, L"Explorer", nullptr);
					SetWindowTheme(info.hwndCombo, L"CFD", nullptr);
				}
			}
		}
		else if (wcscmp(szWndClassName, WC_TREEVIEW) == 0)
		{
			SetWindowTheme(hwnd, L"Explorer", nullptr);
			TreeView_SetTextColor(hwnd, darkTextColor);
			TreeView_SetBkColor(hwnd, darkBkColor);
			if (auto hTT = TreeView_GetToolTips(hwnd))
			{
				DarkModeHelper::Instance().AllowDarkModeForWindow(hTT, static_cast<BOOL>(lParam));
				SetWindowTheme(hTT, L"Explorer", nullptr);
			}
		}
		else if (wcsncmp(szWndClassName, L"RICHEDIT", 8) == 0)
		{
			SetWindowTheme(hwnd, L"Explorer", nullptr);
			CHARFORMAT2 format = { 0 };
			format.cbSize = sizeof(CHARFORMAT2);
			format.dwMask = CFM_COLOR | CFM_BACKCOLOR;
			format.crTextColor = darkTextColor;
			format.crBackColor = darkBkColor;
			SendMessage(hwnd, EM_SETCHARFORMAT, SCF_ALL, reinterpret_cast<LPARAM>(&format));
			SendMessage(hwnd, EM_SETBKGNDCOLOR, 0, static_cast<LPARAM>(format.crBackColor));
		}
		else if (wcscmp(szWndClassName, PROGRESS_CLASS) == 0)
		{
			SetWindowTheme(hwnd, L"", L"");
			SendMessage(hwnd, PBM_SETBKCOLOR, 0, static_cast<LPARAM>(darkBkColor));
			SendMessage(hwnd, PBM_SETBARCOLOR, 0, static_cast<LPARAM>(RGB(50, 50, 180)));
		}
		else if (wcscmp(szWndClassName, REBARCLASSNAME) == 0)
			SetWindowTheme(hwnd, L"DarkModeNavBar", nullptr);
		else if (wcscmp(szWndClassName, L"Auto-Suggest Dropdown") == 0)
		{
			SetWindowTheme(hwnd, L"Explorer", nullptr);
			SetWindowSubclass(hwnd, AutoSuggestSubclassProc, SubclassID, reinterpret_cast<DWORD_PTR>(&s_backBrush));
			EnumChildWindows(hwnd, AdjustThemeForChildrenProc, lParam);
		}
		else if (wcscmp(szWndClassName, TOOLTIPS_CLASSW) == 0)
			SetWindowTheme(hwnd, L"Explorer", nullptr);
		else if (FAILED(SetWindowTheme(hwnd, L"DarkMode_Explorer", nullptr)))
			SetWindowTheme(hwnd, L"Explorer", nullptr);
	}
	else
	{
		if (wcscmp(szWndClassName, WC_LISTVIEW) == 0)
		{
			SetWindowTheme(hwnd, L"Explorer", nullptr);
			if (CAutoThemeData hTheme = OpenThemeData(nullptr, L"ItemsView"))
			{
				COLORREF color;
				if (SUCCEEDED(::GetThemeColor(hTheme, 0, 0, TMT_TEXTCOLOR, &color)))
					ListView_SetTextColor(hwnd, color);
			}
			COLORREF color = GetSysColor(COLOR_WINDOW);
			ListView_SetTextBkColor(hwnd, color);
			ListView_SetBkColor(hwnd, color);
			if (auto hTT = ListView_GetToolTips(hwnd))
			{
				DarkModeHelper::Instance().AllowDarkModeForWindow(hTT, static_cast<BOOL>(lParam));
				SetWindowTheme(hTT, L"Explorer", nullptr);
			}
			RemoveWindowSubclass(hwnd, ListViewSubclassProc, SubclassID);
		}
		else if (wcscmp(szWndClassName, WC_BUTTON) == 0)
		{
			SetWindowTheme(hwnd, L"Explorer", nullptr);
			RemoveWindowSubclass(hwnd, ButtonSubclassProc, SubclassID);
		}
		else if ((wcscmp(szWndClassName, WC_COMBOBOXEX) == 0) ||
				 (wcscmp(szWndClassName, WC_COMBOBOX) == 0))
		{
			SetWindowTheme(hwnd, L"DarkMode_Explorer", nullptr);
			HWND hCombo = hwnd;
			if (wcscmp(szWndClassName, WC_COMBOBOXEX) == 0)
			{
				SendMessage(hwnd, CBEM_SETWINDOWTHEME, 0, reinterpret_cast<LPARAM>(L"DarkMode_Explorer"));
				hCombo = reinterpret_cast<HWND>(SendMessage(hwnd, CBEM_GETCOMBOCONTROL, 0, 0));
			}
			if (hCombo)
			{
				COMBOBOXINFO info = { 0 };
				info.cbSize = sizeof(COMBOBOXINFO);
				if (SendMessage(hCombo, CB_GETCOMBOBOXINFO, 0, reinterpret_cast<LPARAM>(&info)))
				{
					DarkModeHelper::Instance().AllowDarkModeForWindow(info.hwndList, static_cast<BOOL>(lParam));
					DarkModeHelper::Instance().AllowDarkModeForWindow(info.hwndItem, static_cast<BOOL>(lParam));
					DarkModeHelper::Instance().AllowDarkModeForWindow(info.hwndCombo, static_cast<BOOL>(lParam));

					SetWindowTheme(info.hwndList, L"Explorer", nullptr);
					SetWindowTheme(info.hwndItem, L"Explorer", nullptr);
					SetWindowTheme(info.hwndCombo, L"Explorer", nullptr);

					if (CAutoThemeData hTheme = OpenThemeData(nullptr, L"ItemsView"))
					{
						COLORREF color;
						if (SUCCEEDED(::GetThemeColor(hTheme, 0, 0, TMT_TEXTCOLOR, &color)))
							ListView_SetTextColor(info.hwndList, color);
						if (SUCCEEDED(::GetThemeColor(hTheme, 0, 0, TMT_FILLCOLOR, &color)))
						{
							ListView_SetTextBkColor(info.hwndList, color);
							ListView_SetBkColor(info.hwndList, color);
						}
					}

					RemoveWindowSubclass(info.hwndList, ListViewSubclassProc, SubclassID);
				}
				RemoveWindowSubclass(hCombo, ComboBoxSubclassProc, SubclassID);
			}
		}
		else if (wcscmp(szWndClassName, WC_TREEVIEW) == 0)
		{
			SetWindowTheme(hwnd, L"Explorer", nullptr);
			if (CAutoThemeData hTheme = OpenThemeData(nullptr, L"ItemsView"))
			{
				COLORREF color;
				if (SUCCEEDED(::GetThemeColor(hTheme, 0, 0, TMT_TEXTCOLOR, &color)))
					TreeView_SetTextColor(hwnd, color);
				if (SUCCEEDED(::GetThemeColor(hTheme, 0, 0, TMT_FILLCOLOR, &color)))
					TreeView_SetBkColor(hwnd, color);
			}
			if (auto hTT = TreeView_GetToolTips(hwnd))
			{
				DarkModeHelper::Instance().AllowDarkModeForWindow(hTT, static_cast<BOOL>(lParam));
				SetWindowTheme(hTT, L"Explorer", nullptr);
			}
		}
		else if (wcsncmp(szWndClassName, L"RICHEDIT", 8) == 0)
		{
			SetWindowTheme(hwnd, L"Explorer", nullptr);
			CHARFORMAT2 format = { 0 };
			format.cbSize = sizeof(CHARFORMAT2);
			format.dwMask = CFM_COLOR | CFM_BACKCOLOR;
			format.crTextColor = CTheme::Instance().GetThemeColor(GetSysColor(COLOR_WINDOWTEXT));
			format.crBackColor = CTheme::Instance().GetThemeColor(GetSysColor(COLOR_WINDOW));
			SendMessage(hwnd, EM_SETCHARFORMAT, SCF_ALL, reinterpret_cast<LPARAM>(&format));
			SendMessage(hwnd, EM_SETBKGNDCOLOR, 0, static_cast<LPARAM>(format.crBackColor));
		}
		else if (wcscmp(szWndClassName, PROGRESS_CLASS) == 0)
			SetWindowTheme(hwnd, nullptr, nullptr);
		else if (wcscmp(szWndClassName, REBARCLASSNAME) == 0)
			SetWindowTheme(hwnd, L"Explorer", nullptr);
		else if (wcscmp(szWndClassName, L"Auto-Suggest Dropdown") == 0)
		{
			SetWindowTheme(hwnd, L"Explorer", nullptr);
			RemoveWindowSubclass(hwnd, AutoSuggestSubclassProc, SubclassID);
			EnumChildWindows(hwnd, AdjustThemeForChildrenProc, lParam);
		}
		else
			SetWindowTheme(hwnd, L"Explorer", nullptr);
	}
	return TRUE;
}

LRESULT CTheme::ListViewSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR /*uIdSubclass*/, DWORD_PTR /*dwRefData*/)
{
	switch (uMsg)
	{
	case WM_NOTIFY:
		{
			if (reinterpret_cast<LPNMHDR>(lParam)->code == NM_CUSTOMDRAW)
			{
				auto nmcd = reinterpret_cast<LPNMCUSTOMDRAW>(lParam);
				switch (nmcd->dwDrawStage)
				{
				case CDDS_PREPAINT:
					return CDRF_NOTIFYITEMDRAW;
				case CDDS_ITEMPREPAINT:
					{
						SetTextColor(nmcd->hdc, darkTextColor);
						return CDRF_DODEFAULT;
					}
				}
			}
		}
		break;
	case WM_DESTROY:
	case WM_NCDESTROY:
		RemoveWindowSubclass(hWnd, ListViewSubclassProc, SubclassID);
		break;
	}
	return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

LRESULT CTheme::ComboBoxSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR /*uIdSubclass*/, DWORD_PTR dwRefData)
{
	switch (uMsg)
	{
	case WM_CTLCOLORDLG:
	case WM_CTLCOLORSTATIC:
	case WM_CTLCOLOREDIT:
	case WM_CTLCOLORLISTBOX:
	case WM_CTLCOLORBTN:
	case WM_CTLCOLORSCROLLBAR:
		{
			auto hbrBkgnd = reinterpret_cast<HBRUSH*>(dwRefData);
			HDC hdc = reinterpret_cast<HDC>(wParam);
			SetBkMode(hdc, TRANSPARENT);
			SetTextColor(hdc, darkTextColor);
			SetBkColor(hdc, darkBkColor);
			if (!*hbrBkgnd)
				*hbrBkgnd = CreateSolidBrush(darkBkColor);
			return reinterpret_cast<LRESULT>(*hbrBkgnd);
		}
	case WM_DRAWITEM:
		{
			auto pDIS = reinterpret_cast<LPDRAWITEMSTRUCT>(lParam);
			HDC hDC = pDIS->hDC;
			RECT rc = pDIS->rcItem;
			wchar_t itemText[1024] = { 0 };

			COMBOBOXEXITEM cbi = { 0 };
			cbi.mask = CBEIF_TEXT | CBEIF_IMAGE | CBEIF_SELECTEDIMAGE | CBEIF_OVERLAY | CBEIF_INDENT;
			cbi.iItem = pDIS->itemID;
			cbi.cchTextMax = _countof(itemText);
			cbi.pszText = itemText;

			auto cwnd = GetParent(hWnd);
			if (SendMessage(cwnd, CBEM_GETITEM, 0, reinterpret_cast<LPARAM>(&cbi)))
			{
				rc.left += (cbi.iIndent * 10);
				auto img = (pDIS->itemState & LVIS_SELECTED) ? cbi.iSelectedImage : cbi.iImage;
				if (pDIS->itemState & LVIS_FOCUSED)
					::SetBkColor(hDC, darkDisabledTextColor);
				else
					::SetBkColor(hDC, darkBkColor);
				::ExtTextOut(hDC, 0, 0, ETO_OPAQUE, &rc, nullptr, 0, nullptr);

				if (img)
				{
					auto imglist = reinterpret_cast<HIMAGELIST>(SendMessage(cwnd, CBEM_GETIMAGELIST, 0, 0));
					if (imglist)
					{
						int iconX(0), iconY(0);
						ImageList_GetIconSize(imglist, &iconX, &iconY);
						ImageList_Draw(imglist, img, hDC, rc.left, rc.top, ILD_TRANSPARENT | INDEXTOOVERLAYMASK(cbi.iOverlay));
						rc.left += (iconX + 2);
					}
				}

				SetTextColor(pDIS->hDC, darkTextColor);
				SetBkMode(hDC, TRANSPARENT);
				DrawText(hDC, cbi.pszText, -1, &rc, DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX | DT_END_ELLIPSIS);
				return TRUE;
			}
		}
		break;
	case WM_DESTROY:
	case WM_NCDESTROY:
		RemoveWindowSubclass(hWnd, ComboBoxSubclassProc, SubclassID);
		break;
	}
	return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

LRESULT CTheme::MainSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR /*uIdSubclass*/, DWORD_PTR dwRefData)
{
	switch (uMsg)
	{
	case WM_CTLCOLORDLG:
	case WM_CTLCOLORSTATIC:
	case WM_CTLCOLOREDIT:
	case WM_CTLCOLORLISTBOX:
	case WM_CTLCOLORBTN:
	case WM_CTLCOLORSCROLLBAR:
		{
			auto hbrBkgnd = reinterpret_cast<HBRUSH*>(dwRefData);
			HDC hdc = reinterpret_cast<HDC>(wParam);
			SetBkMode(hdc, TRANSPARENT);
			SetTextColor(hdc, darkTextColor);
			SetBkColor(hdc, darkBkColor);
			if (!*hbrBkgnd)
				*hbrBkgnd = CreateSolidBrush(darkBkColor);
			return reinterpret_cast<LRESULT>(*hbrBkgnd);
		}
	case WM_DESTROY:
	case WM_NCDESTROY:
		RemoveWindowSubclass(hWnd, MainSubclassProc, SubclassID);
		break;
	default:
		break;
	}
	return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

LRESULT CTheme::AutoSuggestSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR /*uIdSubclass*/, DWORD_PTR /*dwRefData*/)
{
	switch (uMsg)
	{
	case WM_DRAWITEM:
		{
			auto pDIS = reinterpret_cast<LPDRAWITEMSTRUCT>(lParam);
			HDC hDC = pDIS->hDC;
			RECT rc = pDIS->rcItem;
			wchar_t itemText[256];
			// get the text from sub-items
			ListView_GetItemText(pDIS->hwndItem, pDIS->itemID, 0, itemText, _countof(itemText));

			if (pDIS->itemState & LVIS_FOCUSED)
				::SetBkColor(hDC, darkDisabledTextColor);
			else
				::SetBkColor(hDC, darkBkColor);
			::ExtTextOut(hDC, 0, 0, ETO_OPAQUE, &rc, nullptr, 0, nullptr);

			SetTextColor(pDIS->hDC, darkTextColor);
			SetBkMode(hDC, TRANSPARENT);
			DrawText(hDC, itemText, -1, &rc, DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX | DT_END_ELLIPSIS);
		}
		return TRUE;
	case WM_DESTROY:
	case WM_NCDESTROY:
		RemoveWindowSubclass(hWnd, AutoSuggestSubclassProc, SubclassID);
		break;
	}
	return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

#ifndef RECTWIDTH
#define RECTWIDTH(rc) ((rc).right - (rc).left)
#endif

#ifndef RECTHEIGHT
#define RECTHEIGHT(rc) ((rc).bottom - (rc).top)
#endif

LRESULT CTheme::ButtonSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR /*uIdSubclass*/, DWORD_PTR /*dwRefData*/)
{
	switch (uMsg)
	{
	case WM_SETTEXT:
	case WM_ENABLE:
	case WM_STYLECHANGED:
		{
			LRESULT res = DefSubclassProc(hWnd, uMsg, wParam, lParam);
			InvalidateRgn(hWnd, nullptr, FALSE);
			return res;
		}
	case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(hWnd, &ps);
			if (hdc)
			{
				LONG_PTR dwStyle = GetWindowLongPtr(hWnd, GWL_STYLE);
				LONG_PTR dwButtonStyle = LOWORD(dwStyle);
				LONG_PTR dwButtonType = dwButtonStyle & 0xF;
				RECT rcClient;
				GetClientRect(hWnd, &rcClient);

				if ((dwButtonType & BS_GROUPBOX) == BS_GROUPBOX)
				{
					if (CAutoThemeData hTheme = OpenThemeData(hWnd, L"Button"))
					{
						BP_PAINTPARAMS params = { sizeof(BP_PAINTPARAMS) };
						params.dwFlags = BPPF_ERASE;

						RECT rcExclusion = rcClient;
						params.prcExclude = &rcExclusion;

						// We have to calculate the exclusion rect and therefore
						// calculate the font height. We select the control's font
						// into the DC and fake a drawing operation:
						auto hFontOld = reinterpret_cast<HFONT>(SendMessage(hWnd, WM_GETFONT, 0L, 0));
						if (hFontOld)
							hFontOld = static_cast<HFONT>(SelectObject(hdc, hFontOld));

						RECT rcDraw = rcClient;
						DWORD dwFlags = DT_SINGLELINE;

						// we use uppercase A to determine the height of text, so we
						// can draw the upper line of the groupbox:
						DrawTextW(hdc, L"A", -1, &rcDraw, dwFlags | DT_CALCRECT);

						if (hFontOld)
						{
							SelectObject(hdc, hFontOld);
							hFontOld = nullptr;
						}

						rcExclusion.left += 2;
						rcExclusion.top += RECTHEIGHT(rcDraw);
						rcExclusion.right -= 2;
						rcExclusion.bottom -= 2;

						HDC hdcPaint = nullptr;
						HPAINTBUFFER hBufferedPaint = BeginBufferedPaint(hdc, &rcClient, BPBF_TOPDOWNDIB, &params, &hdcPaint);
						if (hdcPaint)
						{
							// now we again retrieve the font, but this time we select it into
							// the buffered DC:
							hFontOld = reinterpret_cast<HFONT>(SendMessage(hWnd, WM_GETFONT, 0L, 0));
							if (hFontOld)
								hFontOld = static_cast<HFONT>(SelectObject(hdcPaint, hFontOld));

							::SetBkColor(hdcPaint, darkBkColor);
							::ExtTextOut(hdcPaint, 0, 0, ETO_OPAQUE, &rcClient, nullptr, 0, nullptr);

							BufferedPaintSetAlpha(hBufferedPaint, &ps.rcPaint, 0x00);

							DTTOPTS DttOpts = { sizeof(DTTOPTS) };
							DttOpts.dwFlags = DTT_COMPOSITED | DTT_GLOWSIZE;
							DttOpts.crText = darkTextColor;
							DttOpts.iGlowSize = 12; // Default value

							DetermineGlowSize(&DttOpts.iGlowSize);

							COLORREF cr = darkBkColor;
							GetEditBorderColor(hWnd, &cr);
							cr |= 0xff000000;

							auto myPen = std::make_unique<Gdiplus::Pen>(Gdiplus::Color(cr));
							auto myGraphics = std::make_unique<Gdiplus::Graphics>(hdcPaint);
							int iY = RECTHEIGHT(rcDraw) / 2;
							auto rr = Gdiplus::Rect(rcClient.left, rcClient.top + iY, RECTWIDTH(rcClient), RECTHEIGHT(rcClient) - iY - 1);
							Gdiplus::GraphicsPath path;
							GetRoundRectPath(&path, rr, 5);
							myGraphics->DrawPath(myPen.get(), &path);
							myGraphics.reset();
							myPen.reset();

							int iLen = GetWindowTextLength(hWnd);

							if (iLen)
							{
								iLen += 5; // 1 for terminating zero, 4 for DT_MODIFYSTRING
								auto szText = static_cast<LPWSTR>(LocalAlloc(LPTR, sizeof(WCHAR) * iLen));
								if (szText)
								{
									iLen = GetWindowTextW(hWnd, szText, iLen);
									if (iLen)
									{
										int iX = RECTWIDTH(rcDraw);
										rcDraw = rcClient;
										rcDraw.left += iX;
										DrawTextW(hdcPaint, szText, -1, &rcDraw, dwFlags | DT_CALCRECT);
										::SetBkColor(hdcPaint, darkBkColor);
										::ExtTextOut(hdcPaint, 0, 0, ETO_OPAQUE, &rcDraw, nullptr, 0, nullptr);
										++rcDraw.left;
										++rcDraw.right;

										SetBkMode(hdcPaint, TRANSPARENT);
										SetTextColor(hdcPaint, darkTextColor);
										DrawText(hdcPaint, szText, -1, &rcDraw, dwFlags);
									}
									LocalFree(szText);
								}
							}

							if (hFontOld)
							{
								SelectObject(hdcPaint, hFontOld);
								hFontOld = nullptr;
							}

							EndBufferedPaint(hBufferedPaint, TRUE);
						}
					}
				}

				else if (dwButtonType == BS_CHECKBOX || dwButtonType == BS_AUTOCHECKBOX ||
						 dwButtonType == BS_3STATE || dwButtonType == BS_AUTO3STATE || dwButtonType == BS_RADIOBUTTON || dwButtonType == BS_AUTORADIOBUTTON)
				{
					if (CAutoThemeData hTheme = OpenThemeData(hWnd, L"Button"))
					{
						HDC hdcPaint = nullptr;
						BP_PAINTPARAMS params = { sizeof(BP_PAINTPARAMS) };
						params.dwFlags = BPPF_ERASE;
						HPAINTBUFFER hBufferedPaint = BeginBufferedPaint(hdc, &rcClient, BPBF_TOPDOWNDIB, &params, &hdcPaint);
						if (hdcPaint && hBufferedPaint)
						{
							::SetBkColor(hdcPaint, darkBkColor);
							::ExtTextOut(hdcPaint, 0, 0, ETO_OPAQUE, &rcClient, nullptr, 0, nullptr);

							BufferedPaintSetAlpha(hBufferedPaint, &ps.rcPaint, 0x00);

							LRESULT dwCheckState = SendMessage(hWnd, BM_GETCHECK, 0, 0);
							POINT pt;
							RECT rc;
							GetWindowRect(hWnd, &rc);
							GetCursorPos(&pt);
							BOOL bHot = PtInRect(&rc, pt);
							BOOL bFocus = GetFocus() == hWnd;

							int iPartId = BP_CHECKBOX;
							if (dwButtonType == BS_RADIOBUTTON || dwButtonType == BS_AUTORADIOBUTTON)
								iPartId = BP_RADIOBUTTON;

							int iState = GetStateFromBtnState(dwStyle, bHot, bFocus, dwCheckState, iPartId, FALSE);

							int bmWidth = int(ceil(13.0 * CDPIAware::Instance().GetDPIX(hWnd) / 96.0));

							UINT uiHalfWidth = (RECTWIDTH(rcClient) - bmWidth) / 2;

							// we have to use the whole client area, otherwise we get only partially
							// drawn areas:
							RECT rcPaint = rcClient;

							if (dwButtonStyle & BS_LEFTTEXT)
							{
								rcPaint.left += uiHalfWidth;
								rcPaint.right += uiHalfWidth;
							}
							else
							{
								rcPaint.left -= uiHalfWidth;
								rcPaint.right -= uiHalfWidth;
							}

							// we assume that bmWidth is both the horizontal and the vertical
							// dimension of the control bitmap and that it is square. bm.bmHeight
							// seems to be the height of a striped bitmap because it is an absurdly
							// high dimension value
							if ((dwButtonStyle & BS_VCENTER) == BS_VCENTER) /// BS_VCENTER is BS_TOP|BS_BOTTOM
							{
								int h = RECTHEIGHT(rcPaint);
								rcPaint.top = (h - bmWidth) / 2;
								rcPaint.bottom = rcPaint.top + bmWidth;
							}
							else if (dwButtonStyle & BS_TOP)
								rcPaint.bottom = rcPaint.top + bmWidth;
							else if (dwButtonStyle & BS_BOTTOM)
								rcPaint.top = rcPaint.bottom - bmWidth;
							else // default: center the checkbox/radiobutton vertically
							{
								int h = RECTHEIGHT(rcPaint);
								rcPaint.top = (h - bmWidth) / 2;
								rcPaint.bottom = rcPaint.top + bmWidth;
							}

							DrawThemeBackground(hTheme, hdcPaint, iPartId, iState, &rcPaint, nullptr);
							rcPaint = rcClient;

							GetThemeBackgroundContentRect(hTheme, hdcPaint, iPartId, iState, &rcPaint, &rc);

							if (dwButtonStyle & BS_LEFTTEXT)
								rc.right -= bmWidth + 2 * GetSystemMetrics(SM_CXEDGE);
							else
								rc.left += bmWidth + 2 * GetSystemMetrics(SM_CXEDGE);

							DTTOPTS DttOpts = { sizeof(DTTOPTS) };
							DttOpts.dwFlags = DTT_COMPOSITED | DTT_GLOWSIZE;
							DttOpts.crText = darkTextColor;
							DttOpts.iGlowSize = 12; // Default value

							DetermineGlowSize(&DttOpts.iGlowSize);

							HFONT hFontOld = reinterpret_cast<HFONT>(SendMessage(hWnd, WM_GETFONT, 0L, 0));
							if (hFontOld)
								hFontOld = static_cast<HFONT>(SelectObject(hdcPaint, hFontOld));
							int iLen = GetWindowTextLength(hWnd);

							if (iLen)
							{
								iLen += 5; // 1 for terminating zero, 4 for DT_MODIFYSTRING
								LPWSTR szText = static_cast<LPWSTR>(LocalAlloc(LPTR, sizeof(WCHAR) * iLen));
								if (szText)
								{
									iLen = GetWindowTextW(hWnd, szText, iLen);
									if (iLen)
									{
										DWORD dwFlags = DT_SINGLELINE /*|DT_VCENTER*/;
										if (dwButtonStyle & BS_MULTILINE)
										{
											dwFlags |= DT_WORDBREAK;
											dwFlags &= ~(DT_SINGLELINE | DT_VCENTER);
										}

										if ((dwButtonStyle & BS_CENTER) == BS_CENTER) /// BS_CENTER is BS_LEFT|BS_RIGHT
											dwFlags |= DT_CENTER;
										else if (dwButtonStyle & BS_LEFT)
											dwFlags |= DT_LEFT;
										else if (dwButtonStyle & BS_RIGHT)
											dwFlags |= DT_RIGHT;

										if ((dwButtonStyle & BS_VCENTER) == BS_VCENTER) /// BS_VCENTER is BS_TOP|BS_BOTTOM
											dwFlags |= DT_VCENTER;
										else if (dwButtonStyle & BS_TOP)
											dwFlags |= DT_TOP;
										else if (dwButtonStyle & BS_BOTTOM)
											dwFlags |= DT_BOTTOM;
										else
											dwFlags |= DT_VCENTER;

										if ((dwButtonStyle & BS_MULTILINE) && (dwFlags & DT_VCENTER))
										{
											// the DT_VCENTER flag only works for DT_SINGLELINE, so
											// we have to center the text ourselves here
											RECT rcdummy = rc;
											int height = DrawText(hdcPaint, szText, -1, &rcdummy, dwFlags | DT_WORDBREAK | DT_CALCRECT);
											int center_y = rc.top + (RECTHEIGHT(rc) / 2);
											rc.top = center_y - height / 2;
											rc.bottom = center_y + height / 2;
										}
										SetBkMode(hdcPaint, TRANSPARENT);
										if (dwStyle & WS_DISABLED)
											SetTextColor(hdcPaint, darkDisabledTextColor);
										else
											SetTextColor(hdcPaint, darkTextColor);
										DrawText(hdcPaint, szText, -1, &rc, dwFlags);

										// draw the focus rectangle if neccessary:
										if (bFocus)
										{
											RECT rcDraw = rc;

											DrawTextW(hdcPaint, szText, -1, &rcDraw, dwFlags | DT_CALCRECT);
											if (dwFlags & DT_SINGLELINE)
											{
												dwFlags &= ~DT_VCENTER;
												RECT rcDrawTop;
												DrawTextW(hdcPaint, szText, -1, &rcDrawTop, dwFlags | DT_CALCRECT);
												rcDraw.top = rcDraw.bottom - RECTHEIGHT(rcDrawTop);
											}

											if (dwFlags & DT_RIGHT)
											{
												int iWidth = RECTWIDTH(rcDraw);
												rcDraw.right = rc.right;
												rcDraw.left = rcDraw.right - iWidth;
											}

											RECT rcFocus;
											IntersectRect(&rcFocus, &rc, &rcDraw);

											DrawFocusRect(hdcPaint, &rcFocus);
										}
									}
									LocalFree(szText);
								}
							}

							if (hFontOld)
							{
								SelectObject(hdcPaint, hFontOld);
								hFontOld = nullptr;
							}

							EndBufferedPaint(hBufferedPaint, TRUE);
						}
					}
				}
				else if (BS_PUSHBUTTON == dwButtonType || BS_DEFPUSHBUTTON == dwButtonType)
				{
					// push buttons are drawn properly in dark mode without us doing anything
					return DefSubclassProc(hWnd, uMsg, wParam, lParam);
				}
				else
					PaintControl(hWnd, hdc, &ps.rcPaint, false);
			}

			EndPaint(hWnd, &ps);
			return 0;
		}
	case WM_DESTROY:
	case WM_NCDESTROY:
		RemoveWindowSubclass(hWnd, ButtonSubclassProc, SubclassID);
		break;
	}
	return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

void CTheme::OnSysColorChanged()
{
	m_isHighContrastModeDark = false;
	m_isHighContrastMode = false;
	HIGHCONTRAST hc = { sizeof(HIGHCONTRAST) };
	SystemParametersInfo(SPI_GETHIGHCONTRAST, sizeof(HIGHCONTRAST), &hc, FALSE);
	if ((hc.dwFlags & HCF_HIGHCONTRASTON) != 0)
	{
		m_isHighContrastMode = true;
		// check if the high contrast mode is dark
		float h1, h2, s1, s2, l1, l2;
		RGBtoHSL(::GetSysColor(COLOR_WINDOWTEXT), h1, s1, l1);
		RGBtoHSL(::GetSysColor(COLOR_WINDOW), h2, s2, l2);
		m_isHighContrastModeDark = l2 < l1;
	}
	m_dark = !!m_regDarkTheme && IsDarkModeAllowed() && !IsHighContrastMode() && DarkModeHelper::Instance().ShouldAppsUseDarkMode();
}

bool CTheme::IsDarkModeAllowed()
{
	if (IsHighContrastMode())
		return false;

	if (m_bLoaded)
		return m_bDarkModeIsAllowed;
	// we only allow the dark mode for Win10 1809 and later,
	// because on earlier versions it would look really, really ugly!
	m_bDarkModeIsAllowed = false;
	auto version = CPathUtils::GetVersionFromFile(L"uiribbon.dll");
	std::vector<long> tokens;
	stringtok(tokens, version, false, L".");
	if (tokens.size() == 4)
	{
		auto major = tokens[0];
		auto minor = tokens[1];
		auto micro = tokens[2];

		// the windows 10 update 1809 has the version
		// number as 10.0.17763.1
		if (major > 10)
			m_bDarkModeIsAllowed = true;
		else if (major == 10)
		{
			if (minor > 0)
				m_bDarkModeIsAllowed = true;
			else if (micro > 17762)
				m_bDarkModeIsAllowed = true;
		}
	}
	m_bDarkModeIsAllowed = m_bDarkModeIsAllowed && DarkModeHelper::Instance().ShouldAppsUseDarkMode();
	return m_bDarkModeIsAllowed;
}

void CTheme::SetDarkTheme(bool b /*= true*/, bool force /*= false*/)
{
	if (!m_bDarkModeIsAllowed && !force)
		return;
	if (force || m_dark != b)
	{
		bool highContrast = IsHighContrastMode();
		m_dark = b && !highContrast && DarkModeHelper::Instance().ShouldAppsUseDarkMode();
		if (!highContrast && DarkModeHelper::Instance().ShouldAppsUseDarkMode())
			m_regDarkTheme = b ? 1 : 0;
		for (auto& cb : m_themeChangeCallbacks)
			cb.second();
	}
}

/// returns true if dark theme is enabled. If false, then the normal theme is active.

bool CTheme::IsDarkTheme() const
{
	return m_dark;
}

void CTheme::RGBToHSB(COLORREF rgb, BYTE& hue, BYTE& saturation, BYTE& brightness)
{
	BYTE r = GetRValue(rgb);
	BYTE g = GetGValue(rgb);
	BYTE b = GetBValue(rgb);
	BYTE minRGB = min(min(r, g), b);
	BYTE maxRGB = max(max(r, g), b);
	BYTE delta = maxRGB - minRGB;
	double l = maxRGB;
	double s = 0.0;
	double h = 0.0;
	if (maxRGB == 0)
	{
		hue = 0;
		saturation = 0;
		brightness = 0;
		return;
	}
	if (maxRGB)
		s = (255.0 * delta) / maxRGB;

	if (static_cast<BYTE>(s) != 0)
	{
		if (r == maxRGB)
			h = 0 + 43 * static_cast<double>(g - b) / delta;
		else if (g == maxRGB)
			h = 85 + 43 * static_cast<double>(b - r) / delta;
		else if (b == maxRGB)
			h = 171 + 43 * static_cast<double>(r - g) / delta;
	}
	else
		h = 0.0;

	hue = static_cast<BYTE>(h);
	saturation = static_cast<BYTE>(s);
	brightness = static_cast<BYTE>(l);
}

void CTheme::RGBtoHSL(COLORREF color, float& h, float& s, float& l)
{
	const float rPercent = static_cast<float>(GetRValue(color)) / 255;
	const float gPercent = static_cast<float>(GetGValue(color)) / 255;
	const float bPercent = static_cast<float>(GetBValue(color)) / 255;

	float maxColor = 0;
	if ((rPercent >= gPercent) && (rPercent >= bPercent))
		maxColor = rPercent;
	else if ((gPercent >= rPercent) && (gPercent >= bPercent))
		maxColor = gPercent;
	else if ((bPercent >= rPercent) && (bPercent >= gPercent))
		maxColor = bPercent;

	float minColor = 0;
	if ((rPercent <= gPercent) && (rPercent <= bPercent))
		minColor = rPercent;
	else if ((gPercent <= rPercent) && (gPercent <= bPercent))
		minColor = gPercent;
	else if ((bPercent <= rPercent) && (bPercent <= gPercent))
		minColor = bPercent;

	float L = 0, S = 0, H = 0;

	L = (maxColor + minColor) / 2;

	if (maxColor == minColor)
	{
		S = 0;
		H = 0;
	}
	else
	{
		auto d = maxColor - minColor;
		if (L < .50)
			S = d / (maxColor + minColor);
		else
			S = d / ((2.0f - maxColor) - minColor);

		if (maxColor == rPercent)
			H = (gPercent - bPercent) / d;

		else if (maxColor == gPercent)
			H = 2.0f + (bPercent - rPercent) / d;

		else if (maxColor == bPercent)
			H = 4.0f + (rPercent - gPercent) / d;
	}
	H = H * 60;
	if (H < 0)
		H += 360;
	s = S * 100;
	l = L * 100;
	h = H;
}

static float HSLtoRGB_Subfunction(float temp1, float temp2, float temp3)
{
	if ((temp3 * 6) < 1)
		return (temp2 + (temp1 - temp2) * 6 * temp3) * 100;
	else if ((temp3 * 2) < 1)
		return temp1 * 100;
	else if ((temp3 * 3) < 2)
		return (temp2 + (temp1 - temp2) * (.66666f - temp3) * 6) * 100;
	else
		return temp2 * 100;
}

COLORREF CTheme::HSLtoRGB(float h, float s, float l)
{
	if (s == 0)
	{
		BYTE t = static_cast<BYTE>(l / 100 * 255);
		return RGB(t, t, t);
	}
	const float L = l / 100;
	const float S = s / 100;
	const float H = h / 360;
	const float temp1 = (L < .50) ? L * (1 + S) : L + S - (L * S);
	const float temp2 = 2 * L - temp1;
	float temp3 = 0;
	temp3 = H + .33333f;
	if (temp3 > 1)
		temp3 -= 1;
	const float pcr = HSLtoRGB_Subfunction(temp1, temp2, temp3);
	temp3 = H;
	const float pcg = HSLtoRGB_Subfunction(temp1, temp2, temp3);
	temp3 = H - .33333f;
	if (temp3 < 0)
		temp3 += 1;
	const float pcb = HSLtoRGB_Subfunction(temp1, temp2, temp3);
	BYTE r = static_cast<BYTE>(pcr / 100 * 255);
	BYTE g = static_cast<BYTE>(pcg / 100 * 255);
	BYTE b = static_cast<BYTE>(pcb / 100 * 255);
	return RGB(r, g, b);
}

std::optional<LRESULT> CTheme::HandleMenuBar(HWND hWnd, UINT msg, WPARAM /*wParam*/, LPARAM lParam)
{
	static CAutoThemeData sMenuTheme = OpenThemeData(hWnd, L"Menu");

	if (!CTheme::Instance().IsDarkTheme())
		return {};
	switch (msg)
	{
		case WM_MENUBAR_DRAWMENU:
		{
			auto* pMbm = reinterpret_cast<MenubarMenu*>(lParam);
			RECT rc = { 0 };

			// get the menubar rect
			MENUBARINFO mbi = { sizeof(mbi) };
			GetMenuBarInfo(hWnd, OBJID_MENU, 0, &mbi);

			RECT rcWindow;
			GetWindowRect(hWnd, &rcWindow);

			// the rcBar is offset by the window rect
			rc = mbi.rcBar;
			OffsetRect(&rc, -rcWindow.left, -rcWindow.top);
			if (!s_backBrush)
				s_backBrush = CreateSolidBrush(darkBkColor);

			FillRect(pMbm->hdc, &rc, s_backBrush);

			return FALSE;
		}
		case WM_MENUBAR_DRAWMENUITEM:
		{
			if (!s_backBrush)
				s_backBrush = CreateSolidBrush(darkBkColor);
			if (!s_backHotBrush)
				s_backHotBrush = CreateSolidBrush(darkBkHotColor);

			auto* pMdi = reinterpret_cast<MenubarDrawmenuitem*>(lParam);

			// get the menu item string
			wchar_t menuString[256] = { 0 };
			MENUITEMINFO mii = { sizeof(mii), MIIM_STRING };
			{
				mii.dwTypeData = menuString;
				mii.cch = (sizeof(menuString) / 2) - 1;

				GetMenuItemInfo(pMdi->um.hMenu, pMdi->umi.iPosition, TRUE, &mii);
			}

			// get the item state for drawing

			DWORD dwFlags = DT_CENTER | DT_SINGLELINE | DT_VCENTER;

			int iTextStateID = MPI_NORMAL;
			int iBackgroundStateID = MPI_NORMAL;
			{
				if ((pMdi->dis.itemState & ODS_INACTIVE) | (pMdi->dis.itemState & ODS_DEFAULT))
				{
					// normal display
					iTextStateID = MPI_NORMAL;
					iBackgroundStateID = MPI_NORMAL;
				}
				if (pMdi->dis.itemState & ODS_HOTLIGHT)
				{
					// hot tracking
					iTextStateID = MPI_HOT;
					iBackgroundStateID = MPI_HOT;
				}
				if (pMdi->dis.itemState & ODS_SELECTED)
				{
					// clicked
					iTextStateID = MPI_HOT;
					iBackgroundStateID = MPI_HOT;
				}
				if ((pMdi->dis.itemState & ODS_GRAYED) || (pMdi->dis.itemState & ODS_DISABLED))
				{
					// disabled / grey text
					iTextStateID = MPI_DISABLED;
					iBackgroundStateID = MPI_DISABLED;
				}
				if (pMdi->dis.itemState & ODS_NOACCEL)
					dwFlags |= DT_HIDEPREFIX;
			}

			if (!sMenuTheme)
				sMenuTheme = OpenThemeData(hWnd, L"Menu");

			if (iBackgroundStateID == MPI_NORMAL || iBackgroundStateID == MPI_DISABLED)
				FillRect(pMdi->um.hdc, &pMdi->dis.rcItem, s_backBrush);
			else if (iBackgroundStateID == MPI_HOT || iBackgroundStateID == MPI_DISABLEDHOT)
				FillRect(pMdi->um.hdc, &pMdi->dis.rcItem, s_backHotBrush);
			else
				DrawThemeBackground(sMenuTheme, pMdi->um.hdc, MENU_POPUPITEM, iBackgroundStateID, &pMdi->dis.rcItem, nullptr);

			DTTOPTS dttopts = { sizeof(dttopts) };
			if (iTextStateID == MPI_NORMAL || iTextStateID == MPI_HOT)
			{
				dttopts.dwFlags |= DTT_TEXTCOLOR;
				dttopts.crText = CTheme::Instance().GetThemeColor(GetSysColor(COLOR_WINDOWTEXT));
			}
			DrawThemeTextEx(sMenuTheme, pMdi->um.hdc, MENU_POPUPITEM, iTextStateID, menuString, mii.cch, dwFlags, &pMdi->dis.rcItem, &dttopts);

			return TRUE;
		}
		case WM_THEMECHANGED:
			sMenuTheme = OpenThemeData(hWnd, L"Menu");
			break;
	}
	return {};
}

int GetStateFromBtnState(LONG_PTR dwStyle, BOOL bHot, BOOL bFocus, LRESULT dwCheckState, int iPartId, BOOL bHasMouseCapture)
{
	int iState = 0;
	switch (iPartId)
	{
	case BP_PUSHBUTTON:
		iState = PBS_NORMAL;
		if (dwStyle & WS_DISABLED)
			iState = PBS_DISABLED;
		else
		{
			if (dwStyle & BS_DEFPUSHBUTTON)
				iState = PBS_DEFAULTED;

			if (bHasMouseCapture && bHot)
				iState = PBS_PRESSED;
			else if (bHasMouseCapture || bHot)
				iState = PBS_HOT;
		}
		break;
	case BP_GROUPBOX:
		iState = (dwStyle & WS_DISABLED) ? GBS_DISABLED : GBS_NORMAL;
		break;

	case BP_RADIOBUTTON:
		iState = RBS_UNCHECKEDNORMAL;
		switch (dwCheckState)
		{
		case BST_CHECKED:
			if (dwStyle & WS_DISABLED)
				iState = RBS_CHECKEDDISABLED;
			else if (bFocus)
				iState = RBS_CHECKEDPRESSED;
			else if (bHot)
				iState = RBS_CHECKEDHOT;
			else
				iState = RBS_CHECKEDNORMAL;
			break;
		case BST_UNCHECKED:
			if (dwStyle & WS_DISABLED)
				iState = RBS_UNCHECKEDDISABLED;
			else if (bFocus)
				iState = RBS_UNCHECKEDPRESSED;
			else if (bHot)
				iState = RBS_UNCHECKEDHOT;
			else
				iState = RBS_UNCHECKEDNORMAL;
			break;
		}
		break;

	case BP_CHECKBOX:
		switch (dwCheckState)
		{
		case BST_CHECKED:
			if (dwStyle & WS_DISABLED)
				iState = CBS_CHECKEDDISABLED;
			else if (bFocus)
				iState = CBS_CHECKEDPRESSED;
			else if (bHot)
				iState = CBS_CHECKEDHOT;
			else
				iState = CBS_CHECKEDNORMAL;
			break;
		case BST_INDETERMINATE:
			if (dwStyle & WS_DISABLED)
				iState = CBS_MIXEDDISABLED;
			else if (bFocus)
				iState = CBS_MIXEDPRESSED;
			else if (bHot)
				iState = CBS_MIXEDHOT;
			else
				iState = CBS_MIXEDNORMAL;
			break;
		case BST_UNCHECKED:
			if (dwStyle & WS_DISABLED)
				iState = CBS_UNCHECKEDDISABLED;
			else if (bFocus)
				iState = CBS_UNCHECKEDPRESSED;
			else if (bHot)
				iState = CBS_UNCHECKEDHOT;
			else
				iState = CBS_UNCHECKEDNORMAL;
			break;
		}
		break;
	default:
		ASSERT(0);
		break;
	}

	return iState;
}

void GetRoundRectPath(Gdiplus::GraphicsPath* pPath, const Gdiplus::Rect& r, int dia)
{
	// diameter can't exceed width or height
	if (dia > r.Width)
		dia = r.Width;
	if (dia > r.Height)
		dia = r.Height;

	// define a corner
	Gdiplus::Rect Corner(r.X, r.Y, dia, dia);

	// begin path
	pPath->Reset();
	pPath->StartFigure();

	// top left
	pPath->AddArc(Corner, 180, 90);

	// top right
	Corner.X += (r.Width - dia - 1);
	pPath->AddArc(Corner, 270, 90);

	// bottom right
	Corner.Y += (r.Height - dia - 1);
	pPath->AddArc(Corner, 0, 90);

	// bottom left
	Corner.X -= (r.Width - dia - 1);
	pPath->AddArc(Corner, 90, 90);

	// end path
	pPath->CloseFigure();
}

void DrawRect(LPRECT prc, HDC hdcPaint, Gdiplus::DashStyle dashStyle, Gdiplus::Color clr, Gdiplus::REAL width)
{
	auto myPen = std::make_unique<Gdiplus::Pen>(clr, width);
	myPen->SetDashStyle(dashStyle);
	auto myGraphics = std::make_unique<Gdiplus::Graphics>(hdcPaint);

	myGraphics->DrawRectangle(myPen.get(), static_cast<INT>(prc->left), static_cast<INT>(prc->top), static_cast<INT>(prc->right - 1 - prc->left), static_cast<INT>(prc->bottom - 1 - prc->top));
}

void DrawFocusRect(LPRECT prcFocus, HDC hdcPaint)
{
	DrawRect(prcFocus, hdcPaint, Gdiplus::DashStyleDot, Gdiplus::Color(0xFF, 0, 0, 0), 1.0);
}

void PaintControl(HWND hWnd, HDC hdc, RECT* prc, bool bDrawBorder)
{
	HDC hdcPaint = nullptr;

	if (bDrawBorder)
		InflateRect(prc, 1, 1);
	HPAINTBUFFER hBufferedPaint = BeginBufferedPaint(hdc, prc, BPBF_TOPDOWNDIB, nullptr, &hdcPaint);
	if (hdcPaint && hBufferedPaint)
	{
		RECT rc;
		GetWindowRect(hWnd, &rc);

		PatBlt(hdcPaint, 0, 0, RECTWIDTH(rc), RECTHEIGHT(rc), BLACKNESS);
		BufferedPaintSetAlpha(hBufferedPaint, &rc, 0x00);

		///
		/// first blit white so list ctrls don't look ugly:
		///
		PatBlt(hdcPaint, 0, 0, RECTWIDTH(rc), RECTHEIGHT(rc), WHITENESS);

		if (bDrawBorder)
			InflateRect(prc, -1, -1);
		// Tell the control to paint itself in our memory buffer
		SendMessage(hWnd, WM_PRINTCLIENT, reinterpret_cast<WPARAM>(hdcPaint), PRF_CLIENT | PRF_ERASEBKGND | PRF_NONCLIENT | PRF_CHECKVISIBLE);

		if (bDrawBorder)
		{
			InflateRect(prc, 1, 1);
			FrameRect(hdcPaint, prc, static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)));
		}

		// don't make a possible border opaque, only the inner part of the control
		InflateRect(prc, -2, -2);
		// Make every pixel opaque
		BufferedPaintSetAlpha(hBufferedPaint, prc, 255);
		EndBufferedPaint(hBufferedPaint, TRUE);
	}
}

BOOL DetermineGlowSize(int* piSize, LPCWSTR pszClassIdList /*= nullptr*/)
{
	if (!piSize)
	{
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}

	if (!pszClassIdList)
		pszClassIdList = L"CompositedWindow::Window";

	if (CAutoThemeData hThemeWindow = OpenThemeData(nullptr, pszClassIdList))
	{
		SUCCEEDED(GetThemeInt(hThemeWindow, 0, 0, TMT_TEXTGLOWSIZE, piSize));
		return TRUE;
	}

	SetLastError(ERROR_FILE_NOT_FOUND);
	return FALSE;
}

BOOL GetEditBorderColor(HWND hWnd, COLORREF* pClr)
{
	ASSERT(pClr);

	if (CAutoThemeData hTheme = OpenThemeData(hWnd, L"Edit"))
	{
		::GetThemeColor(hTheme, EP_BACKGROUNDWITHBORDER, EBWBS_NORMAL, TMT_BORDERCOLOR, pClr);
		return TRUE;
	}

	return FALSE;
}
