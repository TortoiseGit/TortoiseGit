#ifndef _THEME_H_
#define _THEME_H_

#pragma once

#include <uxtheme.h>
#include <tmschema.h>

#ifndef WM_THEMECHANGED
#define WM_THEMECHANGED                 0x031A
#endif

#pragma warning(push)
#pragma warning(disable : 4100)

class CXPTheme
{
private:
	HTHEME m_hTheme;

	static HMODULE m_hThemeDll;
	static BOOL m_bLoaded;

	static void* GetProc(LPCSTR szProc, void* pfnFail);

	typedef HTHEME(__stdcall *PFNOPENTHEMEDATA)(HWND hwnd, LPCWSTR pszClassList);
	static HTHEME __stdcall OpenThemeDataFail(HWND , LPCWSTR )
	{return NULL;}
	static PFNOPENTHEMEDATA m_pOpenThemeData;

	typedef HRESULT(__stdcall *PFNCLOSETHEMEDATA)(HTHEME hTheme);
	static HRESULT __stdcall CloseThemeDataFail(HTHEME)
	{return E_FAIL;}
	static PFNCLOSETHEMEDATA m_pCloseThemeData;

	typedef HRESULT(__stdcall *PFNDRAWTHEMEBACKGROUND)(HTHEME hTheme, HDC hdc, 
		int iPartId, int iStateId, const RECT *pRect,  const RECT *pClipRect);
	static HRESULT __stdcall DrawThemeBackgroundFail(HTHEME, HDC, int, int, const RECT *, const RECT *)
	{return E_FAIL;}
	static PFNDRAWTHEMEBACKGROUND m_pDrawThemeBackground;

	typedef HRESULT (__stdcall *PFNDRAWTHEMETEXT)(HTHEME hTheme, HDC hdc, int iPartId, 
		int iStateId, LPCWSTR pszText, int iCharCount, DWORD dwTextFlags, 
		DWORD dwTextFlags2, const RECT *pRect);
	static HRESULT __stdcall DrawThemeTextFail(HTHEME, HDC, int, int, LPCWSTR, int, DWORD, DWORD, const RECT*)
	{return E_FAIL;}
	static PFNDRAWTHEMETEXT m_pDrawThemeText;

	typedef HRESULT (__stdcall *PFNGETTHEMEBACKGROUNDCONTENTRECT)(HTHEME hTheme,  HDC hdc, 
		int iPartId, int iStateId,  const RECT *pBoundingRect, 
		RECT *pContentRect);
	static HRESULT __stdcall GetThemeBackgroundContentRectFail(HTHEME hTheme,  HDC hdc, 
		int iPartId, int iStateId,  const RECT *pBoundingRect, 
		RECT *pContentRect)
	{return E_FAIL;}
	static PFNGETTHEMEBACKGROUNDCONTENTRECT m_pGetThemeBackgroundContentRect;

	typedef HRESULT (__stdcall *PFNGETTHEMEBACKGROUNDEXTENT)(HTHEME hTheme,  HDC hdc,
		int iPartId, int iStateId, const RECT *pContentRect, 
		RECT *pExtentRect);
	static HRESULT __stdcall GetThemeBackgroundExtentFail(HTHEME hTheme,  HDC hdc,
		int iPartId, int iStateId, const RECT *pContentRect, 
		RECT *pExtentRect)
	{return E_FAIL;}
	static PFNGETTHEMEBACKGROUNDEXTENT m_pGetThemeBackgroundExtent;

	typedef HRESULT(__stdcall *PFNGETTHEMEPARTSIZE)(HTHEME hTheme, HDC hdc, 
		int iPartId, int iStateId, RECT * pRect, enum THEMESIZE eSize,  SIZE *psz);
	static HRESULT __stdcall GetThemePartSizeFail(HTHEME, HDC, int, int, RECT *, enum THEMESIZE, SIZE *)
	{return E_FAIL;}
	static PFNGETTHEMEPARTSIZE m_pGetThemePartSize;

	typedef HRESULT (__stdcall *PFNGETTHEMETEXTEXTENT)(HTHEME hTheme, HDC hdc, 
		int iPartId, int iStateId, LPCWSTR pszText, int iCharCount, 
		DWORD dwTextFlags,  const RECT *pBoundingRect, 
		RECT *pExtentRect);
	static HRESULT __stdcall GetThemeTextExtentFail(HTHEME hTheme, HDC hdc, 
		int iPartId, int iStateId, LPCWSTR pszText, int iCharCount, 
		DWORD dwTextFlags,  const RECT *pBoundingRect, 
		RECT *pExtentRect)
	{return E_FAIL;}
	static PFNGETTHEMETEXTEXTENT m_pGetThemeTextExtent;

	typedef HRESULT (__stdcall *PFNGETTHEMETEXTMETRICS)(HTHEME hTheme,  HDC hdc, 
		int iPartId, int iStateId,  TEXTMETRIC* ptm);
	static HRESULT __stdcall GetThemeTextMetricsFail(HTHEME hTheme,  HDC hdc, 
		int iPartId, int iStateId,  TEXTMETRIC* ptm)
	{return E_FAIL;}
	static PFNGETTHEMETEXTMETRICS m_pGetThemeTextMetrics;

	typedef HRESULT (__stdcall *PFNGETTHEMEBACKGROUNDREGION)(HTHEME hTheme,  HDC hdc,  
		int iPartId, int iStateId, const RECT *pRect,  HRGN *pRegion);
	static HRESULT __stdcall GetThemeBackgroundRegionFail(HTHEME hTheme,  HDC hdc,  
		int iPartId, int iStateId, const RECT *pRect,  HRGN *pRegion)
	{return E_FAIL;}
	static PFNGETTHEMEBACKGROUNDREGION m_pGetThemeBackgroundRegion;

	typedef HRESULT (__stdcall *PFNHITTESTTHEMEBACKGROUND)(HTHEME hTheme,  HDC hdc, int iPartId, 
		int iStateId, DWORD dwOptions, const RECT *pRect,  HRGN hrgn, 
		POINT ptTest,  WORD *pwHitTestCode);
	static HRESULT __stdcall HitTestThemeBackgroundFail(HTHEME hTheme,  HDC hdc, int iPartId, 
		int iStateId, DWORD dwOptions, const RECT *pRect,  HRGN hrgn, 
		POINT ptTest,  WORD *pwHitTestCode)
	{return E_FAIL;}
	static PFNHITTESTTHEMEBACKGROUND m_pHitTestThemeBackground;

	typedef HRESULT (__stdcall *PFNDRAWTHEMEEDGE)(HTHEME hTheme, HDC hdc, int iPartId, int iStateId, 
		const RECT *pDestRect, UINT uEdge, UINT uFlags,   RECT *pContentRect);
	static HRESULT __stdcall DrawThemeEdgeFail(HTHEME hTheme, HDC hdc, int iPartId, int iStateId, 
		const RECT *pDestRect, UINT uEdge, UINT uFlags,   RECT *pContentRect)
	{return E_FAIL;}
	static PFNDRAWTHEMEEDGE m_pDrawThemeEdge;

	typedef HRESULT (__stdcall *PFNDRAWTHEMEICON)(HTHEME hTheme, HDC hdc, int iPartId, 
		int iStateId, const RECT *pRect, HIMAGELIST himl, int iImageIndex);
	static HRESULT __stdcall DrawThemeIconFail(HTHEME hTheme, HDC hdc, int iPartId, 
		int iStateId, const RECT *pRect, HIMAGELIST himl, int iImageIndex)
	{return E_FAIL;}
	static PFNDRAWTHEMEICON m_pDrawThemeIcon;

	typedef BOOL (__stdcall *PFNISTHEMEPARTDEFINED)(HTHEME hTheme, int iPartId, 
		int iStateId);
	static BOOL __stdcall IsThemePartDefinedFail(HTHEME hTheme, int iPartId, 
		int iStateId)
	{return FALSE;}
	static PFNISTHEMEPARTDEFINED m_pIsThemePartDefined;

	typedef BOOL (__stdcall *PFNISTHEMEBACKGROUNDPARTIALLYTRANSPARENT)(HTHEME hTheme, 
		int iPartId, int iStateId);
	static BOOL __stdcall IsThemeBackgroundPartiallyTransparentFail(HTHEME hTheme, 
		int iPartId, int iStateId)
	{return FALSE;}
	static PFNISTHEMEBACKGROUNDPARTIALLYTRANSPARENT m_pIsThemeBackgroundPartiallyTransparent;

	typedef HRESULT (__stdcall *PFNGETTHEMECOLOR)(HTHEME hTheme, int iPartId, 
		int iStateId, int iPropId,  COLORREF *pColor);
	static HRESULT __stdcall GetThemeColorFail(HTHEME hTheme, int iPartId, 
		int iStateId, int iPropId,  COLORREF *pColor)
	{return E_FAIL;}
	static PFNGETTHEMECOLOR m_pGetThemeColor;

	typedef HRESULT (__stdcall *PFNGETTHEMEMETRIC)(HTHEME hTheme,  HDC hdc, int iPartId, 
		int iStateId, int iPropId,  int *piVal);
	static HRESULT __stdcall GetThemeMetricFail(HTHEME hTheme,  HDC hdc, int iPartId, 
		int iStateId, int iPropId,  int *piVal)
	{return E_FAIL;}
	static PFNGETTHEMEMETRIC m_pGetThemeMetric;

	typedef HRESULT (__stdcall *PFNGETTHEMESTRING)(HTHEME hTheme, int iPartId, 
		int iStateId, int iPropId,  LPWSTR pszBuff, int cchMaxBuffChars);
	static HRESULT __stdcall GetThemeStringFail(HTHEME hTheme, int iPartId, 
		int iStateId, int iPropId,  LPWSTR pszBuff, int cchMaxBuffChars)
	{return E_FAIL;}
	static PFNGETTHEMESTRING m_pGetThemeString;

	typedef HRESULT (__stdcall *PFNGETTHEMEBOOL)(HTHEME hTheme, int iPartId, 
		int iStateId, int iPropId,  BOOL *pfVal);
	static HRESULT __stdcall GetThemeBoolFail(HTHEME hTheme, int iPartId, 
		int iStateId, int iPropId,  BOOL *pfVal)
	{return E_FAIL;}
	static PFNGETTHEMEBOOL m_pGetThemeBool;

	typedef HRESULT (__stdcall *PFNGETTHEMEINT)(HTHEME hTheme, int iPartId, 
		int iStateId, int iPropId,  int *piVal);
	static HRESULT __stdcall GetThemeIntFail(HTHEME hTheme, int iPartId, 
		int iStateId, int iPropId,  int *piVal)
	{return E_FAIL;}
	static PFNGETTHEMEINT m_pGetThemeInt;

	typedef HRESULT (__stdcall *PFNGETTHEMEENUMVALUE)(HTHEME hTheme, int iPartId, 
		int iStateId, int iPropId,  int *piVal);
	static HRESULT __stdcall GetThemeEnumValueFail(HTHEME hTheme, int iPartId, 
		int iStateId, int iPropId,  int *piVal)
	{return E_FAIL;}
	static PFNGETTHEMEENUMVALUE m_pGetThemeEnumValue;

	typedef HRESULT (__stdcall *PFNGETTHEMEPOSITION)(HTHEME hTheme, int iPartId, 
		int iStateId, int iPropId,  POINT *pPoint);
	static HRESULT __stdcall GetThemePositionFail(HTHEME hTheme, int iPartId, 
		int iStateId, int iPropId,  POINT *pPoint)
	{return E_FAIL;}
	static PFNGETTHEMEPOSITION m_pGetThemePosition;

	typedef HRESULT (__stdcall *PFNGETTHEMEFONT)(HTHEME hTheme,  HDC hdc, int iPartId, 
		int iStateId, int iPropId,  LOGFONT *pFont);
	static HRESULT __stdcall GetThemeFontFail(HTHEME hTheme,  HDC hdc, int iPartId, 
		int iStateId, int iPropId,  LOGFONT *pFont)
	{return E_FAIL;}
	static PFNGETTHEMEFONT m_pGetThemeFont;

	typedef HRESULT (__stdcall *PFNGETTHEMERECT)(HTHEME hTheme, int iPartId, 
		int iStateId, int iPropId,  RECT *pRect);
	static HRESULT __stdcall GetThemeRectFail(HTHEME hTheme, int iPartId, 
		int iStateId, int iPropId,  RECT *pRect)
	{return E_FAIL;}
	static PFNGETTHEMERECT m_pGetThemeRect;

	typedef HRESULT (__stdcall *PFNGETTHEMEMARGINS)(HTHEME hTheme,  HDC hdc, int iPartId, 
		int iStateId, int iPropId,  RECT *prc,  MARGINS *pMargins);
	static HRESULT __stdcall GetThemeMarginsFail(HTHEME hTheme,  HDC hdc, int iPartId, 
		int iStateId, int iPropId,  RECT *prc,  MARGINS *pMargins)
	{return E_FAIL;}
	static PFNGETTHEMEMARGINS m_pGetThemeMargins;

	typedef HRESULT (__stdcall *PFNGETTHEMEINTLIST)(HTHEME hTheme, int iPartId, 
		int iStateId, int iPropId,  INTLIST *pIntList);
	static HRESULT __stdcall GetThemeIntListFail(HTHEME hTheme, int iPartId, 
		int iStateId, int iPropId,  INTLIST *pIntList)
	{return E_FAIL;}
	static PFNGETTHEMEINTLIST m_pGetThemeIntList;

	typedef HRESULT (__stdcall *PFNGETTHEMEPROPERTYORIGIN)(HTHEME hTheme, int iPartId, 
		int iStateId, int iPropId,  enum PROPERTYORIGIN *pOrigin);
	static HRESULT __stdcall GetThemePropertyOriginFail(HTHEME hTheme, int iPartId, 
		int iStateId, int iPropId,  enum PROPERTYORIGIN *pOrigin)
	{return E_FAIL;}
	static PFNGETTHEMEPROPERTYORIGIN m_pGetThemePropertyOrigin;

	typedef HRESULT (__stdcall *PFNSETWINDOWTHEME)(HWND hwnd, LPCWSTR pszSubAppName, 
		LPCWSTR pszSubIdList);
	static HRESULT __stdcall SetWindowThemeFail(HWND hwnd, LPCWSTR pszSubAppName, 
		LPCWSTR pszSubIdList)
	{return E_FAIL;}
	static PFNSETWINDOWTHEME m_pSetWindowTheme;

	typedef HRESULT (__stdcall *PFNGETTHEMEFILENAME)(HTHEME hTheme, int iPartId, 
		int iStateId, int iPropId,  LPWSTR pszThemeFileName, int cchMaxBuffChars);
	static HRESULT __stdcall GetThemeFilenameFail(HTHEME hTheme, int iPartId, 
		int iStateId, int iPropId,  LPWSTR pszThemeFileName, int cchMaxBuffChars)
	{return E_FAIL;}
	static PFNGETTHEMEFILENAME m_pGetThemeFilename;

	typedef COLORREF (__stdcall *PFNGETTHEMESYSCOLOR)(HTHEME hTheme, int iColorId);
	static COLORREF __stdcall GetThemeSysColorFail(HTHEME hTheme, int iColorId)
	{return RGB(255,255,255);}
	static PFNGETTHEMESYSCOLOR m_pGetThemeSysColor;

	typedef HBRUSH (__stdcall *PFNGETTHEMESYSCOLORBRUSH)(HTHEME hTheme, int iColorId);
	static HBRUSH __stdcall GetThemeSysColorBrushFail(HTHEME hTheme, int iColorId)
	{return NULL;}
	static PFNGETTHEMESYSCOLORBRUSH m_pGetThemeSysColorBrush;

	typedef BOOL (__stdcall *PFNGETTHEMESYSBOOL)(HTHEME hTheme, int iBoolId);
	static BOOL __stdcall GetThemeSysBoolFail(HTHEME hTheme, int iBoolId)
	{return FALSE;}
	static PFNGETTHEMESYSBOOL m_pGetThemeSysBool;

	typedef int (__stdcall *PFNGETTHEMESYSSIZE)(HTHEME hTheme, int iSizeId);
	static int __stdcall GetThemeSysSizeFail(HTHEME hTheme, int iSizeId)
	{return 0;}
	static PFNGETTHEMESYSSIZE m_pGetThemeSysSize;

	typedef HRESULT (__stdcall *PFNGETTHEMESYSFONT)(HTHEME hTheme, int iFontId,  LOGFONT *plf);
	static HRESULT __stdcall GetThemeSysFontFail(HTHEME hTheme, int iFontId,  LOGFONT *plf)
	{return E_FAIL;}
	static PFNGETTHEMESYSFONT m_pGetThemeSysFont;

	typedef HRESULT (__stdcall *PFNGETTHEMESYSSTRING)(HTHEME hTheme, int iStringId, 
		LPWSTR pszStringBuff, int cchMaxStringChars);
	static HRESULT __stdcall GetThemeSysStringFail(HTHEME hTheme, int iStringId, 
		LPWSTR pszStringBuff, int cchMaxStringChars)
	{return E_FAIL;}
	static PFNGETTHEMESYSSTRING m_pGetThemeSysString;

	typedef HRESULT (__stdcall *PFNGETTHEMESYSINT)(HTHEME hTheme, int iIntId, int *piValue);
	static HRESULT __stdcall GetThemeSysIntFail(HTHEME hTheme, int iIntId, int *piValue)
	{return E_FAIL;}
	static PFNGETTHEMESYSINT m_pGetThemeSysInt;

	typedef BOOL (__stdcall *PFNISTHEMEACTIVE)();
	static BOOL __stdcall IsThemeActiveFail()
	{return FALSE;}
	static PFNISTHEMEACTIVE m_pIsThemeActive;

	typedef BOOL(__stdcall *PFNISAPPTHEMED)();
	static BOOL __stdcall IsAppThemedFail()
	{return FALSE;}
	static PFNISAPPTHEMED m_pIsAppThemed;

	typedef HTHEME (__stdcall *PFNGETWINDOWTHEME)(HWND hwnd);
	static HTHEME __stdcall GetWindowThemeFail(HWND hwnd)
	{return NULL;}
	static PFNGETWINDOWTHEME m_pGetWindowTheme;

	typedef HRESULT (__stdcall *PFNENABLETHEMEDIALOGTEXTURE)(HWND hwnd, DWORD dwFlags);
	static HRESULT __stdcall EnableThemeDialogTextureFail(HWND hwnd, DWORD dwFlags)
	{return E_FAIL;}
	static PFNENABLETHEMEDIALOGTEXTURE m_pEnableThemeDialogTexture;

	typedef BOOL (__stdcall *PFNISTHEMEDIALOGTEXTUREENABLED)(HWND hwnd);
	static BOOL __stdcall IsThemeDialogTextureEnabledFail(HWND hwnd)
	{return FALSE;}
	static PFNISTHEMEDIALOGTEXTUREENABLED m_pIsThemeDialogTextureEnabled;

	typedef DWORD (__stdcall *PFNGETTHEMEAPPPROPERTIES)();
	static DWORD __stdcall GetThemeAppPropertiesFail()
	{return 0;}
	static PFNGETTHEMEAPPPROPERTIES m_pGetThemeAppProperties;

	typedef void (__stdcall *PFNSETTHEMEAPPPROPERTIES)(DWORD dwFlags);
	static void __stdcall SetThemeAppPropertiesFail(DWORD dwFlags)
	{return;}
	static PFNSETTHEMEAPPPROPERTIES m_pSetThemeAppProperties;

	typedef HRESULT (__stdcall *PFNGETCURRENTTHEMENAME)(
		LPWSTR pszThemeFileName, int cchMaxNameChars, 
		LPWSTR pszColorBuff, int cchMaxColorChars,
		LPWSTR pszSizeBuff, int cchMaxSizeChars);
	static HRESULT __stdcall GetCurrentThemeNameFail(
		LPWSTR pszThemeFileName, int cchMaxNameChars, 
		LPWSTR pszColorBuff, int cchMaxColorChars,
		LPWSTR pszSizeBuff, int cchMaxSizeChars)
	{return E_FAIL;}
	static PFNGETCURRENTTHEMENAME m_pGetCurrentThemeName;

	typedef HRESULT (__stdcall *PFNGETTHEMEDOCUMENTATIONPROPERTY)(LPCWSTR pszThemeName,
		LPCWSTR pszPropertyName,  LPWSTR pszValueBuff, int cchMaxValChars);
	static HRESULT __stdcall GetThemeDocumentationPropertyFail(LPCWSTR pszThemeName,
		LPCWSTR pszPropertyName,  LPWSTR pszValueBuff, int cchMaxValChars)
	{return E_FAIL;}
	static PFNGETTHEMEDOCUMENTATIONPROPERTY m_pGetThemeDocumentationProperty;

	typedef HRESULT (__stdcall *PFNDRAWTHEMEPARENTBACKGROUND)(HWND hwnd, HDC hdc,  RECT* prc);
	static HRESULT __stdcall DrawThemeParentBackgroundFail(HWND hwnd, HDC hdc,  RECT* prc)
	{return E_FAIL;}
	static PFNDRAWTHEMEPARENTBACKGROUND m_pDrawThemeParentBackground;

	typedef HRESULT (__stdcall *PFNENABLETHEMING)(BOOL fEnable);
	static HRESULT __stdcall EnableThemingFail(BOOL fEnable)
	{return E_FAIL;}
	static PFNENABLETHEMING m_pEnableTheming;
public:
	BOOL Open(HWND hwnd, LPCWSTR pszClassList);
	void Close();
	HRESULT DrawBackground(HDC hdc, 
		int iPartId, int iStateId, const RECT *pRect, const RECT *pClipRect);
	HRESULT DrawText(HDC hdc, int iPartId, 
		int iStateId, LPCWSTR pszText, int iCharCount, DWORD dwTextFlags, 
		DWORD dwTextFlags2, const RECT *pRect);
	HRESULT GetBackgroundContentRect(HDC hdc, 
		int iPartId, int iStateId,  const RECT *pBoundingRect, 
		RECT *pContentRect);
	HRESULT GetBackgroundExtent(HDC hdc,
		int iPartId, int iStateId, const RECT *pContentRect, 
		RECT *pExtentRect);
	HRESULT GetPartSize(HDC hdc, 
		int iPartId, int iStateId, RECT * pRect, enum THEMESIZE eSize, SIZE *psz);
	HRESULT GetTextExtent(HDC hdc, 
		int iPartId, int iStateId, LPCWSTR pszText, int iCharCount, 
		DWORD dwTextFlags,  const RECT *pBoundingRect, 
		RECT *pExtentRect);
	HRESULT GetTextMetrics(HDC hdc, 
		int iPartId, int iStateId,  TEXTMETRIC* ptm);
	HRESULT GetBackgroundRegion(HDC hdc,  
		int iPartId, int iStateId, const RECT *pRect,  HRGN *pRegion);
	HRESULT HitTestBackground(HDC hdc, int iPartId, 
		int iStateId, DWORD dwOptions, const RECT *pRect,  HRGN hrgn, 
		POINT ptTest,  WORD *pwHitTestCode);
	HRESULT DrawEdge(HDC hdc, int iPartId, int iStateId, 
		const RECT *pDestRect, UINT uEdge, UINT uFlags,   RECT *pContentRect);
	HRESULT DrawIcon(HDC hdc, int iPartId, 
		int iStateId, const RECT *pRect, HIMAGELIST himl, int iImageIndex);
	BOOL IsPartDefined(int iPartId, 
		int iStateId);
	BOOL IsBackgroundPartiallyTransparent( 
		int iPartId, int iStateId);
	HRESULT GetColor(int iPartId, 
		int iStateId, int iPropId,  COLORREF *pColor);
	HRESULT GetMetric(HDC hdc, int iPartId, 
		int iStateId, int iPropId,  int *piVal);
	HRESULT GetString(int iPartId, 
		int iStateId, int iPropId,  LPWSTR pszBuff, int cchMaxBuffChars);
	HRESULT GetBool(int iPartId, 
		int iStateId, int iPropId,  BOOL *pfVal);
	HRESULT GetInt(int iPartId, 
		int iStateId, int iPropId,  int *piVal);
	HRESULT GetEnumValue(int iPartId, 
		int iStateId, int iPropId,  int *piVal);
	HRESULT GetPosition(int iPartId, 
		int iStateId, int iPropId,  POINT *pPoint);
	HRESULT GetFont(HDC hdc, int iPartId, 
		int iStateId, int iPropId,  LOGFONT *pFont);
	HRESULT GetRect(int iPartId, 
		int iStateId, int iPropId,  RECT *pRect);
	HRESULT GetMargins(HDC hdc, int iPartId, 
		int iStateId, int iPropId,  RECT *prc,  MARGINS *pMargins);
	HRESULT GetIntList(int iPartId, 
		int iStateId, int iPropId,  INTLIST *pIntList);
	HRESULT GetPropertyOrigin(int iPartId, 
		int iStateId, int iPropId,  enum PROPERTYORIGIN *pOrigin);
	static HRESULT SetWindowTheme(HWND hwnd, LPCWSTR pszSubAppName, 
		LPCWSTR pszSubIdList);
	HRESULT GetFilename(int iPartId, 
		int iStateId, int iPropId,  LPWSTR pszThemeFileName, int cchMaxBuffChars);
	COLORREF GetSysColor(int iColorId);
	HBRUSH GetSysColorBrush(int iColorId);
	BOOL GetSysBool(int iBoolId);
	int GetSysSize(int iSizeId);
	HRESULT GetSysFont(int iFontId,  LOGFONT *plf);
	HRESULT GetSysString(int iStringId, 
		LPWSTR pszStringBuff, int cchMaxStringChars);
	HRESULT GetSysInt(int iIntId, int *piValue);
	static BOOL IsActive();
	static BOOL IsAppThemed();	
	static HTHEME GetWindowTheme(HWND hwnd);
	static HRESULT EnableDialogTexture(HWND hwnd, DWORD dwFlags);
	static BOOL IsDialogTextureEnabled(HWND hwnd);
	static DWORD GetAppProperties();
	static void SetAppProperties(DWORD dwFlags);
	static HRESULT GetCurrentName(
		LPWSTR pszThemeFileName, int cchMaxNameChars, 
		LPWSTR pszColorBuff, int cchMaxColorChars,
		LPWSTR pszSizeBuff, int cchMaxSizeChars);
	static HRESULT GetDocumentationProperty(LPCWSTR pszThemeName,
		LPCWSTR pszPropertyName,  LPWSTR pszValueBuff, int cchMaxValChars);
	static HRESULT DrawParentBackground(HWND hwnd, HDC hdc,  RECT* prc);
	static HRESULT EnableTheming(BOOL fEnable);
public:
	CXPTheme(void);
	CXPTheme(HTHEME hTheme);
	CXPTheme(HWND hwnd, LPCWSTR pszClassList);
	~CXPTheme(void);
	void Attach(HTHEME hTheme);
	HTHEME Detach();
	operator HTHEME() const;
	bool operator!() const;

	static void Release(void);
	static BOOL IsAvailable();
};
#pragma warning(pop)
#endif