#include "stdafx.h"
#include "BufferDC.h"

IMPLEMENT_DYNAMIC(CBufferDC, CPaintDC)

CBufferDC::CBufferDC(CWnd* pWnd) : CPaintDC(pWnd)
{
	if (pWnd && CPaintDC::m_hDC)
	{
		m_hOutputDC    = CPaintDC::m_hDC;
		m_hAttributeDC = CPaintDC::m_hAttribDC;

		pWnd->GetClientRect(&m_ClientRect);

		m_hMemoryDC = ::CreateCompatibleDC(m_hOutputDC);

		m_hPaintBitmap =
			::CreateCompatibleBitmap(
					m_hOutputDC,
					m_ClientRect.right  - m_ClientRect.left,
					m_ClientRect.bottom - m_ClientRect.top);

		m_hOldBitmap = static_cast<HBITMAP>(::SelectObject(m_hMemoryDC, m_hPaintBitmap));

		CPaintDC::m_hDC       = m_hMemoryDC;
		CPaintDC::m_hAttribDC = m_hMemoryDC;
	}
	else
	{
		ATLASSERT(false);
		m_hAttributeDC = nullptr;
		m_hOutputDC = nullptr;
		m_hMemoryDC = nullptr;
		m_hPaintBitmap = nullptr;
		m_hOldBitmap = nullptr;
		m_ClientRect.right = 0;
		m_ClientRect.left = 0;
		m_ClientRect.top = 0;
		m_ClientRect.bottom = 0;
	}

	m_bBoundsUpdated = FALSE;
}

CBufferDC::~CBufferDC(void)
{
	Flush();

	::SelectObject(m_hMemoryDC, m_hOldBitmap);
	::DeleteObject(m_hPaintBitmap);

	CPaintDC::m_hDC		  = m_hOutputDC;
	CPaintDC::m_hAttribDC = m_hAttributeDC;

	::DeleteDC(m_hMemoryDC);
}

void CBufferDC::Flush()
{
	::BitBlt(
		m_hOutputDC,
		m_ClientRect.left, m_ClientRect.top,
		m_ClientRect.right  - m_ClientRect.left,
		m_ClientRect.bottom - m_ClientRect.top,
		m_hMemoryDC,
		0, 0,
		SRCCOPY);
}

UINT CBufferDC::SetBoundsRect( LPCRECT lpRectBounds, UINT flags )
{
	if (lpRectBounds)
	{
		if (m_ClientRect.right  - m_ClientRect.left > lpRectBounds->right  - lpRectBounds->left ||
			m_ClientRect.bottom - m_ClientRect.top  > lpRectBounds->bottom - lpRectBounds->top)
		{
			lpRectBounds = &m_ClientRect;
		}

		HBITMAP bmp =
			::CreateCompatibleBitmap(
					m_hOutputDC,
					lpRectBounds->right - lpRectBounds->left,
					lpRectBounds->bottom - lpRectBounds->top);

		HDC tmpDC  = ::CreateCompatibleDC(m_hOutputDC);

		auto oldBmp = static_cast<HBITMAP>(::SelectObject(tmpDC, bmp));

		::BitBlt(
			tmpDC,
			m_ClientRect.left, m_ClientRect.top,
			m_ClientRect.right  - m_ClientRect.left,
			m_ClientRect.bottom - m_ClientRect.top,
			m_hMemoryDC,
			0, 0,
			SRCCOPY);

		::SelectObject(tmpDC, oldBmp);
		::DeleteDC(tmpDC);

		auto old = static_cast<HBITMAP>(::SelectObject(m_hMemoryDC, bmp));

		if (old && old != m_hPaintBitmap)
		{
			::DeleteObject(old);
		}

		if (m_hPaintBitmap)
		{
			::DeleteObject(m_hPaintBitmap);
		}

		m_hPaintBitmap = bmp;

		m_ClientRect = *lpRectBounds;
		m_bBoundsUpdated = TRUE;
	}

	return CPaintDC::SetBoundsRect(lpRectBounds, flags);
}

BOOL CBufferDC::RestoreDC( int nSavedDC )
{
	BOOL ret = CPaintDC::RestoreDC(nSavedDC);

	if (m_bBoundsUpdated)
	{
		SelectObject(m_hPaintBitmap);
	}

	return ret;
}
