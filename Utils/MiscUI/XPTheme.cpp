/////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2001-2002 by Pål Kristian Tønder
//
// Distribute and change freely, except: don't remove my name from the source 
//
// No warrantee of any kind, express or implied, is included with this
// software; use at your own risk, responsibility for damages (if any) to
// anyone resulting from the use of this software rests entirely with the
// user.
//
// Heavily based on the CVisualStyleXP class of David Yuheng Zhao. The difference
// is that the dll is global and the HTHEME handle is wrapped. The HTHEME handle is
// automatically released as the object is deleted. All API functions not directly
// related to a HTHEME instance are realized as static methods. To optimize the
// execution speed, static function pointers are used. During execution, either
// the actual function or the failure function is assigned to the static function
// pointer, which is used from there on.
//
// To explicitly Release the dll, call the static Release method. To check whether
// Theming is available, that is, if the platform is XP or newer, use the static
// method IsAvailable
//
// If you have any questions, I can be reached as follows:
//	pal.k.tonder@powel.no
//
//
// How to use:
// Instead of calling the API directly, 
//    HTHEME hTheme = OpenThemeData(...);
//	  if(hTheme == NULL) {
//        // draw old fashion way
//    }
//    else {
//       DrawThemeBackground(hTheme, ...);
//       .
//       [more method calls]
//       .
//       CloseThemeData(hTheme);
//	  }
//	
// use a variable of the CXPTheme class 
//    CXPTheme theme(...);
//	  if(!theme) {
//        // draw old fashion way
//    }
//    else {
//       theme.DrawBackground(...);
//       .
//       [more method calls]
//       .
//    }
//
// A couple of convenient operator overloads are provided::
// - operator HTHEME for accessing the HTHEME handle
// - operator ! for checking if HTHEME handle is valid
//
/////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "XPTheme.h"

// Initializing static members
BOOL CXPTheme::m_bLoaded = FALSE;
HMODULE CXPTheme::m_hThemeDll = NULL;

CXPTheme::PFNOPENTHEMEDATA CXPTheme::m_pOpenThemeData = NULL;
CXPTheme::PFNCLOSETHEMEDATA CXPTheme::m_pCloseThemeData = NULL;
CXPTheme::PFNDRAWTHEMEBACKGROUND CXPTheme::m_pDrawThemeBackground = NULL;
CXPTheme::PFNDRAWTHEMETEXT CXPTheme::m_pDrawThemeText = NULL;
CXPTheme::PFNGETTHEMEBACKGROUNDCONTENTRECT CXPTheme::m_pGetThemeBackgroundContentRect = NULL;
CXPTheme::PFNGETTHEMEBACKGROUNDEXTENT CXPTheme::m_pGetThemeBackgroundExtent = NULL;
CXPTheme::PFNGETTHEMEPARTSIZE CXPTheme::m_pGetThemePartSize = NULL;
CXPTheme::PFNGETTHEMETEXTEXTENT CXPTheme::m_pGetThemeTextExtent = NULL;
CXPTheme::PFNGETTHEMETEXTMETRICS CXPTheme::m_pGetThemeTextMetrics = NULL;
CXPTheme::PFNGETTHEMEBACKGROUNDREGION CXPTheme::m_pGetThemeBackgroundRegion = NULL;
CXPTheme::PFNHITTESTTHEMEBACKGROUND CXPTheme::m_pHitTestThemeBackground = NULL;
CXPTheme::PFNDRAWTHEMEEDGE CXPTheme::m_pDrawThemeEdge = NULL;
CXPTheme::PFNDRAWTHEMEICON CXPTheme::m_pDrawThemeIcon = NULL;
CXPTheme::PFNISTHEMEPARTDEFINED CXPTheme::m_pIsThemePartDefined = NULL;
CXPTheme::PFNISTHEMEBACKGROUNDPARTIALLYTRANSPARENT CXPTheme::m_pIsThemeBackgroundPartiallyTransparent = NULL;
CXPTheme::PFNGETTHEMECOLOR CXPTheme::m_pGetThemeColor = NULL;
CXPTheme::PFNGETTHEMEMETRIC CXPTheme::m_pGetThemeMetric = NULL;
CXPTheme::PFNGETTHEMESTRING CXPTheme::m_pGetThemeString = NULL;
CXPTheme::PFNGETTHEMEBOOL CXPTheme::m_pGetThemeBool = NULL;
CXPTheme::PFNGETTHEMEINT CXPTheme::m_pGetThemeInt = NULL;
CXPTheme::PFNGETTHEMEENUMVALUE CXPTheme::m_pGetThemeEnumValue = NULL;
CXPTheme::PFNGETTHEMEPOSITION CXPTheme::m_pGetThemePosition = NULL;
CXPTheme::PFNGETTHEMEFONT CXPTheme::m_pGetThemeFont = NULL;
CXPTheme::PFNGETTHEMERECT CXPTheme::m_pGetThemeRect = NULL;
CXPTheme::PFNGETTHEMEMARGINS CXPTheme::m_pGetThemeMargins = NULL;
CXPTheme::PFNGETTHEMEINTLIST CXPTheme::m_pGetThemeIntList = NULL;
CXPTheme::PFNGETTHEMEPROPERTYORIGIN CXPTheme::m_pGetThemePropertyOrigin = NULL;
CXPTheme::PFNSETWINDOWTHEME CXPTheme::m_pSetWindowTheme = NULL;
CXPTheme::PFNGETTHEMEFILENAME CXPTheme::m_pGetThemeFilename = NULL;
CXPTheme::PFNGETTHEMESYSCOLOR CXPTheme::m_pGetThemeSysColor = NULL;
CXPTheme::PFNGETTHEMESYSCOLORBRUSH CXPTheme::m_pGetThemeSysColorBrush = NULL;
CXPTheme::PFNGETTHEMESYSBOOL CXPTheme::m_pGetThemeSysBool = NULL;
CXPTheme::PFNGETTHEMESYSSIZE CXPTheme::m_pGetThemeSysSize = NULL;
CXPTheme::PFNGETTHEMESYSFONT CXPTheme::m_pGetThemeSysFont = NULL;
CXPTheme::PFNGETTHEMESYSSTRING CXPTheme::m_pGetThemeSysString = NULL;
CXPTheme::PFNGETTHEMESYSINT CXPTheme::m_pGetThemeSysInt = NULL;
CXPTheme::PFNISTHEMEACTIVE CXPTheme::m_pIsThemeActive = NULL;
CXPTheme::PFNISAPPTHEMED CXPTheme::m_pIsAppThemed = NULL;
CXPTheme::PFNGETWINDOWTHEME CXPTheme::m_pGetWindowTheme = NULL;
CXPTheme::PFNENABLETHEMEDIALOGTEXTURE CXPTheme::m_pEnableThemeDialogTexture = NULL;
CXPTheme::PFNISTHEMEDIALOGTEXTUREENABLED CXPTheme::m_pIsThemeDialogTextureEnabled = NULL;
CXPTheme::PFNGETTHEMEAPPPROPERTIES CXPTheme::m_pGetThemeAppProperties = NULL;
CXPTheme::PFNSETTHEMEAPPPROPERTIES CXPTheme::m_pSetThemeAppProperties = NULL;
CXPTheme::PFNGETCURRENTTHEMENAME CXPTheme::m_pGetCurrentThemeName = NULL;
CXPTheme::PFNGETTHEMEDOCUMENTATIONPROPERTY CXPTheme::m_pGetThemeDocumentationProperty = NULL;
CXPTheme::PFNDRAWTHEMEPARENTBACKGROUND CXPTheme::m_pDrawThemeParentBackground = NULL;
CXPTheme::PFNENABLETHEMING CXPTheme::m_pEnableTheming = NULL;

// Constructors
CXPTheme::CXPTheme(void) : m_hTheme(NULL)
{
}

CXPTheme::CXPTheme(HTHEME hTheme)
{
	m_hTheme = hTheme;
}

CXPTheme::CXPTheme(HWND hwnd, LPCWSTR pszClassList)
{
	m_hTheme = NULL;
	Open(hwnd, pszClassList);
}

// Destructor
CXPTheme::~CXPTheme(void)
{
	Close();
}

void CXPTheme::Attach(HTHEME hTheme)
{
	Close();
	m_hTheme = hTheme;
}

HTHEME CXPTheme::Detach()
{
	HTHEME hTheme = m_hTheme;
	m_hTheme = NULL;
	return hTheme;
}

CXPTheme::operator HTHEME() const
{
	return m_hTheme;
}

bool CXPTheme::operator !() const
{
	return (m_hTheme == NULL);
}

BOOL CXPTheme::IsAvailable()
{
	if (m_hThemeDll == NULL && !m_bLoaded) {
		m_hThemeDll = LoadLibrary(_T("UxTheme.dll"));
		m_bLoaded = TRUE;
	}
	return m_hThemeDll != NULL;
}

void CXPTheme::Release(void)
{
	if (m_hThemeDll != NULL) {
		FreeLibrary(m_hThemeDll);
		m_hThemeDll = NULL;
		m_bLoaded = FALSE;

		// Set all function pointers to NULL
		m_pOpenThemeData = NULL;
		m_pCloseThemeData = NULL;
		m_pDrawThemeBackground = NULL;
		m_pDrawThemeText = NULL;
		m_pGetThemeBackgroundContentRect = NULL;
		m_pGetThemeBackgroundExtent = NULL;
		m_pGetThemePartSize = NULL;
		m_pGetThemeTextExtent = NULL;
		m_pGetThemeTextMetrics = NULL;
		m_pGetThemeBackgroundRegion = NULL;
		m_pHitTestThemeBackground = NULL;
		m_pDrawThemeEdge = NULL;
		m_pDrawThemeIcon = NULL;
		m_pIsThemePartDefined = NULL;
		m_pIsThemeBackgroundPartiallyTransparent = NULL;
		m_pGetThemeColor = NULL;
		m_pGetThemeMetric = NULL;
		m_pGetThemeString = NULL;
		m_pGetThemeBool = NULL;
		m_pGetThemeInt = NULL;
		m_pGetThemeEnumValue = NULL;
		m_pGetThemePosition = NULL;
		m_pGetThemeFont = NULL;
		m_pGetThemeRect = NULL;
		m_pGetThemeMargins = NULL;
		m_pGetThemeIntList = NULL;
		m_pGetThemePropertyOrigin = NULL;
		m_pSetWindowTheme = NULL;
		m_pGetThemeFilename = NULL;
		m_pGetThemeSysColor = NULL;
		m_pGetThemeSysColorBrush = NULL;
		m_pGetThemeSysBool = NULL;
		m_pGetThemeSysSize = NULL;
		m_pGetThemeSysFont = NULL;
		m_pGetThemeSysString = NULL;
		m_pGetThemeSysInt = NULL;
		m_pIsThemeActive = NULL;
		m_pIsAppThemed = NULL;
		m_pGetWindowTheme = NULL;
		m_pEnableThemeDialogTexture = NULL;
		m_pIsThemeDialogTextureEnabled = NULL;
		m_pGetThemeAppProperties = NULL;
		m_pSetThemeAppProperties = NULL;
		m_pGetCurrentThemeName = NULL;
		m_pGetThemeDocumentationProperty = NULL;
		m_pDrawThemeParentBackground = NULL;
		m_pEnableTheming = NULL;
	}
}

void* CXPTheme::GetProc(LPCSTR szProc, void* pfnFail)
{
	void* pRet = pfnFail;
	if (m_hThemeDll == NULL && !m_bLoaded) {
		m_hThemeDll = LoadLibrary(_T("UxTheme.dll"));
		m_bLoaded = TRUE;
	}
	if (m_hThemeDll != NULL)
		pRet = GetProcAddress(m_hThemeDll, szProc);
	return pRet;
}

BOOL CXPTheme::Open(HWND hwnd, LPCWSTR pszClassList)
{
	// Close before opening
	Close();
	if(m_pOpenThemeData == NULL)
		m_pOpenThemeData = (CXPTheme::PFNOPENTHEMEDATA)GetProc("OpenThemeData", (void*)OpenThemeDataFail);
	m_hTheme = (*m_pOpenThemeData)(hwnd, pszClassList);
	return m_hTheme != NULL;
}

void CXPTheme::Close()
{
	if(m_hTheme != NULL) {
		if(m_pCloseThemeData == NULL)
			m_pCloseThemeData = (CXPTheme::PFNCLOSETHEMEDATA)GetProc("CloseThemeData", (void*)CloseThemeDataFail);
		(*m_pCloseThemeData)(m_hTheme);
		m_hTheme = NULL;
	}
}

HRESULT CXPTheme::DrawBackground(HDC hdc, int iPartId, int iStateId, const RECT *pRect, const RECT *pClipRect)
{
	if(m_pDrawThemeBackground == NULL)
		m_pDrawThemeBackground = 
		(CXPTheme::PFNDRAWTHEMEBACKGROUND)GetProc("DrawThemeBackground", (void*)DrawThemeBackgroundFail);
	return (*m_pDrawThemeBackground)(m_hTheme, hdc, iPartId, iStateId, pRect, pClipRect);
}


HRESULT CXPTheme::DrawText(HDC hdc, int iPartId, 
									   int iStateId, LPCWSTR pszText, int iCharCount, DWORD dwTextFlags, 
									   DWORD dwTextFlags2, const RECT *pRect)
{
	if(m_pDrawThemeText == NULL)
		m_pDrawThemeText = (CXPTheme::PFNDRAWTHEMETEXT)GetProc("DrawThemeText", (void*)DrawThemeTextFail);
	return (*m_pDrawThemeText)(m_hTheme, hdc, iPartId, iStateId, pszText, iCharCount, dwTextFlags, dwTextFlags2, pRect);
}
HRESULT CXPTheme::GetBackgroundContentRect( HDC hdc, 
													   int iPartId, int iStateId,  const RECT *pBoundingRect, 
													   RECT *pContentRect)
{
	if(m_pGetThemeBackgroundContentRect == NULL)
		m_pGetThemeBackgroundContentRect = (CXPTheme::PFNGETTHEMEBACKGROUNDCONTENTRECT)GetProc("GetThemeBackgroundContentRect", (void*)GetThemeBackgroundContentRectFail);
	return (*m_pGetThemeBackgroundContentRect)(m_hTheme,  hdc, iPartId, iStateId,  pBoundingRect, pContentRect);
}
HRESULT CXPTheme::GetBackgroundExtent( HDC hdc,
												  int iPartId, int iStateId, const RECT *pContentRect, 
												  RECT *pExtentRect)
{
	if(m_pGetThemeBackgroundExtent == NULL)
		m_pGetThemeBackgroundExtent = (CXPTheme::PFNGETTHEMEBACKGROUNDEXTENT)GetProc("GetThemeBackgroundExtent", (void*)GetThemeBackgroundExtentFail);
	return (*m_pGetThemeBackgroundExtent)(m_hTheme, hdc, iPartId, iStateId, pContentRect, pExtentRect);
}
HRESULT CXPTheme::GetPartSize(HDC hdc, int iPartId, int iStateId, RECT * pRect, enum THEMESIZE eSize, SIZE *psz)
{
	if(m_pGetThemePartSize == NULL)
		m_pGetThemePartSize = 
		(CXPTheme::PFNGETTHEMEPARTSIZE)GetProc("GetThemePartSize", (void*)GetThemePartSizeFail);
	return (*m_pGetThemePartSize)(m_hTheme, hdc, iPartId, iStateId, pRect, eSize, psz);
}

HRESULT CXPTheme::GetTextExtent(HDC hdc, int iPartId, int iStateId, LPCWSTR pszText, int iCharCount, 
							  DWORD dwTextFlags,  const RECT *pBoundingRect, 
							  RECT *pExtentRect)
{
	if(m_pGetThemeTextExtent == NULL)
		m_pGetThemeTextExtent = (CXPTheme::PFNGETTHEMETEXTEXTENT)GetProc("GetThemeTextExtent", (void*)GetThemeTextExtentFail);
	return (*m_pGetThemeTextExtent)(m_hTheme, hdc, iPartId, iStateId, pszText, iCharCount, dwTextFlags,  pBoundingRect, pExtentRect);
}

HRESULT CXPTheme::GetTextMetrics(HDC hdc, int iPartId, int iStateId,  TEXTMETRIC* ptm)
{
	if(m_pGetThemeTextMetrics == NULL)
		m_pGetThemeTextMetrics = (CXPTheme::PFNGETTHEMETEXTMETRICS)GetProc("GetThemeTextMetrics", (void*)GetThemeTextMetricsFail);
	return (*m_pGetThemeTextMetrics)(m_hTheme, hdc, iPartId, iStateId,  ptm);
}

HRESULT CXPTheme::GetBackgroundRegion(HDC hdc, int iPartId, int iStateId, const RECT *pRect,  HRGN *pRegion)
{
	if(m_pGetThemeBackgroundRegion == NULL)
		m_pGetThemeBackgroundRegion = (CXPTheme::PFNGETTHEMEBACKGROUNDREGION)GetProc("GetThemeBackgroundRegion", (void*)GetThemeBackgroundRegionFail);
	return (*m_pGetThemeBackgroundRegion)(m_hTheme, hdc, iPartId, iStateId, pRect, pRegion);
}

HRESULT CXPTheme::HitTestBackground(HDC hdc, int iPartId, int iStateId, DWORD dwOptions, const RECT *pRect,  HRGN hrgn, 
								  POINT ptTest,  WORD *pwHitTestCode)
{
	if(m_pHitTestThemeBackground == NULL)
		m_pHitTestThemeBackground = (CXPTheme::PFNHITTESTTHEMEBACKGROUND)GetProc("HitTestThemeBackground", (void*)HitTestThemeBackgroundFail);
	return (*m_pHitTestThemeBackground)(m_hTheme, hdc, iPartId, iStateId, dwOptions, pRect, hrgn, ptTest, pwHitTestCode);
}

HRESULT CXPTheme::DrawEdge(HDC hdc, int iPartId, int iStateId, 
						 const RECT *pDestRect, UINT uEdge, UINT uFlags,   RECT *pContentRect)
{
	if(m_pDrawThemeEdge == NULL)
		m_pDrawThemeEdge = (CXPTheme::PFNDRAWTHEMEEDGE)GetProc("DrawThemeEdge", (void*)DrawThemeEdgeFail);
	return (*m_pDrawThemeEdge)(m_hTheme, hdc, iPartId, iStateId, pDestRect, uEdge, uFlags, pContentRect);
}

HRESULT CXPTheme::DrawIcon(HDC hdc, int iPartId, 
						 int iStateId, const RECT *pRect, HIMAGELIST himl, int iImageIndex)
{
	if(m_pDrawThemeIcon == NULL)
		m_pDrawThemeIcon = (CXPTheme::PFNDRAWTHEMEICON)GetProc("DrawThemeIcon", (void*)DrawThemeIconFail);
	return (*m_pDrawThemeIcon)(m_hTheme, hdc, iPartId, iStateId, pRect, himl, iImageIndex);
}

BOOL CXPTheme::IsPartDefined(int iPartId, int iStateId)
{
	if(m_pIsThemePartDefined == NULL)
		m_pIsThemePartDefined = (CXPTheme::PFNISTHEMEPARTDEFINED)GetProc("IsThemePartDefined", (void*)IsThemePartDefinedFail);
	return (*m_pIsThemePartDefined)(m_hTheme, iPartId, iStateId);
}

BOOL CXPTheme::IsBackgroundPartiallyTransparent(int iPartId, int iStateId)
{
	if(m_pIsThemeBackgroundPartiallyTransparent == NULL)
		m_pIsThemeBackgroundPartiallyTransparent = (CXPTheme::PFNISTHEMEBACKGROUNDPARTIALLYTRANSPARENT)GetProc("IsThemeBackgroundPartiallyTransparent", (void*)IsThemeBackgroundPartiallyTransparentFail);
	return (*m_pIsThemeBackgroundPartiallyTransparent)(m_hTheme, iPartId, iStateId);
}

HRESULT CXPTheme::GetColor(int iPartId, int iStateId, int iPropId,  COLORREF *pColor)
{
	if(m_pGetThemeColor == NULL)
		m_pGetThemeColor = (CXPTheme::PFNGETTHEMECOLOR)GetProc("GetThemeColor", (void*)GetThemeColorFail);
	return (*m_pGetThemeColor)(m_hTheme, iPartId, iStateId, iPropId, pColor);
}

HRESULT CXPTheme::GetMetric(HDC hdc, int iPartId, int iStateId, int iPropId,  int *piVal)
{
	if(m_pGetThemeMetric == NULL)
		m_pGetThemeMetric = (CXPTheme::PFNGETTHEMEMETRIC)GetProc("GetThemeMetric", (void*)GetThemeMetricFail);
	return (*m_pGetThemeMetric)(m_hTheme, hdc, iPartId, iStateId, iPropId, piVal);
}

HRESULT CXPTheme::GetString(int iPartId, int iStateId, int iPropId,  LPWSTR pszBuff, int cchMaxBuffChars)
{
	if(m_pGetThemeString == NULL)
		m_pGetThemeString = (CXPTheme::PFNGETTHEMESTRING)GetProc("GetThemeString", (void*)GetThemeStringFail);
	return (*m_pGetThemeString)(m_hTheme, iPartId, iStateId, iPropId, pszBuff, cchMaxBuffChars);
}

HRESULT CXPTheme::GetBool(int iPartId, int iStateId, int iPropId,  BOOL *pfVal)
{
	if(m_pGetThemeBool == NULL)
		m_pGetThemeBool = (CXPTheme::PFNGETTHEMEBOOL)GetProc("GetThemeBool", (void*)GetThemeBoolFail);
	return (*m_pGetThemeBool)(m_hTheme, iPartId, iStateId, iPropId, pfVal);
}

HRESULT CXPTheme::GetInt(int iPartId, int iStateId, int iPropId,  int *piVal)
{
	if(m_pGetThemeInt == NULL)
		m_pGetThemeInt = (CXPTheme::PFNGETTHEMEINT)GetProc("GetThemeInt", (void*)GetThemeIntFail);
	return (*m_pGetThemeInt)(m_hTheme, iPartId, iStateId, iPropId, piVal);
}

HRESULT CXPTheme::GetEnumValue(int iPartId, int iStateId, int iPropId,  int *piVal)
{
	if(m_pGetThemeEnumValue == NULL)
		m_pGetThemeEnumValue = (CXPTheme::PFNGETTHEMEENUMVALUE)GetProc("GetThemeEnumValue", (void*)GetThemeEnumValueFail);
	return (*m_pGetThemeEnumValue)(m_hTheme, iPartId, iStateId, iPropId, piVal);
}

HRESULT CXPTheme::GetPosition(int iPartId, int iStateId, int iPropId,  POINT *pPoint)
{
	if(m_pGetThemePosition == NULL)
		m_pGetThemePosition = (CXPTheme::PFNGETTHEMEPOSITION)GetProc("GetThemePosition", (void*)GetThemePositionFail);
	return (*m_pGetThemePosition)(m_hTheme, iPartId, iStateId, iPropId, pPoint);
}

HRESULT CXPTheme::GetFont( HDC hdc, int iPartId, 
									  int iStateId, int iPropId,  LOGFONT *pFont)
{
	if(m_pGetThemeFont == NULL)
		m_pGetThemeFont = (CXPTheme::PFNGETTHEMEFONT)GetProc("GetThemeFont", (void*)GetThemeFontFail);
	return (*m_pGetThemeFont)(m_hTheme, hdc, iPartId, iStateId, iPropId, pFont);
}

HRESULT CXPTheme::GetRect(int iPartId, int iStateId, int iPropId,  RECT *pRect)
{
	if(m_pGetThemeRect == NULL)
		m_pGetThemeRect = (CXPTheme::PFNGETTHEMERECT)GetProc("GetThemeRect", (void*)GetThemeRectFail);
	return (*m_pGetThemeRect)(m_hTheme, iPartId, iStateId, iPropId, pRect);
}

HRESULT CXPTheme::GetMargins( HDC hdc, int iPartId, 
										 int iStateId, int iPropId,  RECT *prc,  MARGINS *pMargins)
{
	if(m_pGetThemeMargins == NULL)
		m_pGetThemeMargins = (CXPTheme::PFNGETTHEMEMARGINS)GetProc("GetThemeMargins", (void*)GetThemeMarginsFail);
	return (*m_pGetThemeMargins)(m_hTheme, hdc, iPartId, iStateId, iPropId, prc, pMargins);
}

HRESULT CXPTheme::GetIntList(int iPartId, int iStateId, int iPropId,  INTLIST *pIntList)
{
	if(m_pGetThemeIntList == NULL)
		m_pGetThemeIntList = (CXPTheme::PFNGETTHEMEINTLIST)GetProc("GetThemeIntList", (void*)GetThemeIntListFail);
	return (*m_pGetThemeIntList)(m_hTheme, iPartId, iStateId, iPropId, pIntList);
}

HRESULT CXPTheme::GetPropertyOrigin(int iPartId, int iStateId, int iPropId,  enum PROPERTYORIGIN *pOrigin)
{
	if(m_pGetThemePropertyOrigin == NULL)
		m_pGetThemePropertyOrigin = (CXPTheme::PFNGETTHEMEPROPERTYORIGIN)GetProc("GetThemePropertyOrigin", (void*)GetThemePropertyOriginFail);
	return (*m_pGetThemePropertyOrigin)(m_hTheme, iPartId, iStateId, iPropId, pOrigin);
}

HRESULT CXPTheme::SetWindowTheme(HWND hwnd, LPCWSTR pszSubAppName, 
										LPCWSTR pszSubIdList)
{
	if(m_pSetWindowTheme == NULL)
		m_pSetWindowTheme = (CXPTheme::PFNSETWINDOWTHEME)GetProc("SetWindowTheme", (void*)SetWindowThemeFail);
	return (*m_pSetWindowTheme)(hwnd, pszSubAppName, pszSubIdList);
}

HRESULT CXPTheme::GetFilename(int iPartId, 
										  int iStateId, int iPropId,  LPWSTR pszThemeFileName, int cchMaxBuffChars)
{
	if(m_pGetThemeFilename == NULL)
		m_pGetThemeFilename = (CXPTheme::PFNGETTHEMEFILENAME)GetProc("GetThemeFilename", (void*)GetThemeFilenameFail);
	return (*m_pGetThemeFilename)(m_hTheme, iPartId, iStateId, iPropId,  pszThemeFileName, cchMaxBuffChars);
}

COLORREF CXPTheme::GetSysColor(int iColorId)
{
	if(m_pGetThemeSysColor == NULL)
		m_pGetThemeSysColor = (CXPTheme::PFNGETTHEMESYSCOLOR)GetProc("GetThemeSysColor", (void*)GetThemeSysColorFail);
	return (*m_pGetThemeSysColor)(m_hTheme, iColorId);
}

HBRUSH CXPTheme::GetSysColorBrush(int iColorId)
{
	if(m_pGetThemeSysColorBrush == NULL)
		m_pGetThemeSysColorBrush = (CXPTheme::PFNGETTHEMESYSCOLORBRUSH)GetProc("GetThemeSysColorBrush", (void*)GetThemeSysColorBrushFail);
	return (*m_pGetThemeSysColorBrush)(m_hTheme, iColorId);
}

BOOL CXPTheme::GetSysBool(int iBoolId)
{
	if(m_pGetThemeSysBool == NULL)
		m_pGetThemeSysBool = (CXPTheme::PFNGETTHEMESYSBOOL)GetProc("GetThemeSysBool", (void*)GetThemeSysBoolFail);
	return (*m_pGetThemeSysBool)(m_hTheme, iBoolId);
}

int CXPTheme::GetSysSize(int iSizeId)
{
	if(m_pGetThemeSysSize == NULL)
		m_pGetThemeSysSize = (CXPTheme::PFNGETTHEMESYSSIZE)GetProc("GetThemeSysSize", (void*)GetThemeSysSizeFail);
	return (*m_pGetThemeSysSize)(m_hTheme, iSizeId);
}

HRESULT CXPTheme::GetSysFont(int iFontId,  LOGFONT *plf)
{
	if(m_pGetThemeSysFont == NULL)
		m_pGetThemeSysFont = (CXPTheme::PFNGETTHEMESYSFONT)GetProc("GetThemeSysFont", (void*)GetThemeSysFontFail);
	return (*m_pGetThemeSysFont)(m_hTheme, iFontId, plf);
}

HRESULT CXPTheme::GetSysString(int iStringId, 
										   LPWSTR pszStringBuff, int cchMaxStringChars)
{
	if(m_pGetThemeSysString == NULL)
		m_pGetThemeSysString = (CXPTheme::PFNGETTHEMESYSSTRING)GetProc("GetThemeSysString", (void*)GetThemeSysStringFail);
	return (*m_pGetThemeSysString)(m_hTheme, iStringId, pszStringBuff, cchMaxStringChars);
}

HRESULT CXPTheme::GetSysInt(int iIntId, int *piValue)
{
	if(m_pGetThemeSysInt == NULL)
		m_pGetThemeSysInt = (CXPTheme::PFNGETTHEMESYSINT)GetProc("GetThemeSysInt", (void*)GetThemeSysIntFail);
	return (*m_pGetThemeSysInt)(m_hTheme, iIntId, piValue);
}

BOOL CXPTheme::IsActive()
{
	if(m_pIsThemeActive == NULL)
		m_pIsThemeActive = (CXPTheme::PFNISTHEMEACTIVE)GetProc("IsThemeActive", (void*)IsThemeActiveFail);
	return (*m_pIsThemeActive)();
}

BOOL CXPTheme::IsAppThemed()
{
	if(m_pIsAppThemed == NULL)
		m_pIsAppThemed = (CXPTheme::PFNISAPPTHEMED)GetProc("IsAppThemed", (void*)IsAppThemedFail);
	return (*m_pIsAppThemed)();
}

HTHEME CXPTheme::GetWindowTheme(HWND hwnd)
{
	if(m_pGetWindowTheme == NULL)
		m_pGetWindowTheme = (CXPTheme::PFNGETWINDOWTHEME)GetProc("GetWindowTheme", (void*)GetWindowThemeFail);
	return (*m_pGetWindowTheme)(hwnd);
}

HRESULT CXPTheme::EnableDialogTexture(HWND hwnd, DWORD dwFlags)
{
	if(m_pEnableThemeDialogTexture == NULL)
		m_pEnableThemeDialogTexture = (CXPTheme::PFNENABLETHEMEDIALOGTEXTURE)GetProc("EnableThemeDialogTexture", (void*)EnableThemeDialogTextureFail);
	return (*m_pEnableThemeDialogTexture)(hwnd, dwFlags);
}

BOOL CXPTheme::IsDialogTextureEnabled(HWND hwnd)
{
	if(m_pIsThemeDialogTextureEnabled == NULL)
		m_pIsThemeDialogTextureEnabled = (CXPTheme::PFNISTHEMEDIALOGTEXTUREENABLED)GetProc("IsThemeDialogTextureEnabled", (void*)IsThemeDialogTextureEnabledFail);
	return (*m_pIsThemeDialogTextureEnabled)(hwnd);
}

DWORD CXPTheme::GetAppProperties()
{
	if(m_pGetThemeAppProperties == NULL)
		m_pGetThemeAppProperties = (CXPTheme::PFNGETTHEMEAPPPROPERTIES)GetProc("GetThemeAppProperties", (void*)GetThemeAppPropertiesFail);
	return (*m_pGetThemeAppProperties)();
}

void CXPTheme::SetAppProperties(DWORD dwFlags)
{
	if(m_pSetThemeAppProperties == NULL)
		m_pSetThemeAppProperties = (CXPTheme::PFNSETTHEMEAPPPROPERTIES)GetProc("SetThemeAppProperties", (void*)SetThemeAppPropertiesFail);
	(*m_pSetThemeAppProperties)(dwFlags);
}

HRESULT CXPTheme::GetCurrentName(
	LPWSTR pszThemeFileName, int cchMaxNameChars, 
	LPWSTR pszColorBuff, int cchMaxColorChars,
	LPWSTR pszSizeBuff, int cchMaxSizeChars)
{
	if(m_pGetCurrentThemeName == NULL)
		m_pGetCurrentThemeName = (CXPTheme::PFNGETCURRENTTHEMENAME)GetProc("GetCurrentThemeName", (void*)GetCurrentThemeNameFail);
	return (*m_pGetCurrentThemeName)(pszThemeFileName, cchMaxNameChars, pszColorBuff, cchMaxColorChars, pszSizeBuff, cchMaxSizeChars);
}

HRESULT CXPTheme::GetDocumentationProperty(LPCWSTR pszThemeName,
													   LPCWSTR pszPropertyName,  LPWSTR pszValueBuff, int cchMaxValChars)
{
	if(m_pGetThemeDocumentationProperty == NULL)
		m_pGetThemeDocumentationProperty = (CXPTheme::PFNGETTHEMEDOCUMENTATIONPROPERTY)GetProc("GetThemeDocumentationProperty", (void*)GetThemeDocumentationPropertyFail);
	return (*m_pGetThemeDocumentationProperty)(pszThemeName, pszPropertyName, pszValueBuff, cchMaxValChars);
}


HRESULT CXPTheme::DrawParentBackground(HWND hwnd, HDC hdc,  RECT* prc)
{
	if(m_pDrawThemeParentBackground == NULL)
		m_pDrawThemeParentBackground = (CXPTheme::PFNDRAWTHEMEPARENTBACKGROUND)GetProc("DrawThemeParentBackground", (void*)DrawThemeParentBackgroundFail);
	return (*m_pDrawThemeParentBackground)(hwnd, hdc, prc);
}

HRESULT CXPTheme::EnableTheming(BOOL fEnable)
{
	if(m_pEnableTheming == NULL)
		m_pEnableTheming = (CXPTheme::PFNENABLETHEMING)GetProc("EnableTheming", (void*)EnableThemingFail);
	return (*m_pEnableTheming)(fEnable);
}
