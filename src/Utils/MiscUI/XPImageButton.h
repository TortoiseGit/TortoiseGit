#pragma once

#include "XPTheme.h"
#include "OddButton.h"

/**
 * \ingroup TortoiseProc
 * A CButton which can show icons and images with
 * the XP style. Normal CButtons only show the icons/images
 * themselves, without the XP style background.
 */
class CXPImageButton : public CButton
{
	DECLARE_DYNAMIC(CXPImageButton)

public:
	CXPImageButton();
	virtual ~CXPImageButton();

protected:
	DECLARE_MESSAGE_MAP()

	afx_msg void OnNotifyCustomDraw ( NMHDR * pNotifyStruct, LRESULT* result );

	void DrawBitmap (HDC hDC, const CRect& Rect, DWORD style);
	void DrawIcon (HDC hDC, const CRect& Rect, DWORD style);
	int ImageLeft(int cx, const CRect& Rect, DWORD style) const;
	int ImageTop(int cy, const CRect& Rect, DWORD style) const;
	CXPTheme m_xpButton;
};

