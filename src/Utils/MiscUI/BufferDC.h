#pragma once
#include "afxwin.h"

class CBufferDC :
	public CPaintDC
{
	DECLARE_DYNAMIC(CBufferDC)

private:
	HDC m_hOutputDC = nullptr;
	HDC m_hAttributeDC = nullptr;
	HDC m_hMemoryDC = nullptr;

	HBITMAP m_hPaintBitmap = nullptr;
	HBITMAP m_hOldBitmap = nullptr;

	RECT m_ClientRect{};

	BOOL m_bBoundsUpdated = FALSE;

public:
	CBufferDC(CWnd* pWnd);
	~CBufferDC();

private:
	void Flush();

public:
	UINT SetBoundsRect(LPCRECT lpRectBounds, UINT flags);
	BOOL RestoreDC(int nSavedDC) override;
};
