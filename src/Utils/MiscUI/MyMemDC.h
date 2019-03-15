#pragma once

//////////////////////////////////////////////////
// CMyMemDC - memory DC
//
// Author: Keith Rule
// Email:  keithr@europa.com
// Copyright 1996-1997, Keith Rule
//
// You may freely use or modify this code provided this
// Copyright is included in all derived versions.
//
// History - 10/3/97 Fixed scrolling bug.
//                   Added print support.
//           25 feb 98 - fixed minor assertion bug
//
// This class implements a memory Device Context

#ifdef __AFXWIN_H__
class CMyMemDC : public CDC
{
public:
	// constructor sets up the memory DC
	CMyMemDC(CDC* pDC, bool bTempOnly = false, int nOffset = 0) : CDC()
	{
		ASSERT(pDC);

		m_pDC = pDC;
		m_pOldBitmap = nullptr;
		m_bMyMemDC = ((!pDC->IsPrinting()) && (!GetSystemMetrics(SM_REMOTESESSION)));
		m_bTempOnly = bTempOnly;

		if (m_bMyMemDC)	// Create a Memory DC
		{
			pDC->GetClipBox(&m_rect);
			CreateCompatibleDC(pDC);
			m_bitmap.CreateCompatibleBitmap(pDC, m_rect.Width() - nOffset, m_rect.Height());
			m_pOldBitmap = SelectObject(&m_bitmap);
			SetWindowOrg(m_rect.left, m_rect.top);
		}
		else		// Make a copy of the relevant parts of the current DC for printing
		{
			m_bPrinting = pDC->m_bPrinting;
			m_hDC		= pDC->m_hDC;
			m_hAttribDC = pDC->m_hAttribDC;
		}

		FillSolidRect(m_rect, pDC->GetBkColor());
	}

	CMyMemDC(CDC* pDC, const CRect* pRect) : CDC()
	{
		ASSERT(pDC);

		// Some initialization
		m_pDC = pDC;
		m_pOldBitmap = nullptr;
		m_bMyMemDC = !pDC->IsPrinting();
		m_bTempOnly = false;

		// Get the rectangle to draw
		if (!pRect) {
			pDC->GetClipBox(&m_rect);
		} else {
			m_rect = *pRect;
		}

		if (m_bMyMemDC) {
			// Create a Memory DC
			CreateCompatibleDC(pDC);
			pDC->LPtoDP(&m_rect);

			m_bitmap.CreateCompatibleBitmap(pDC, m_rect.Width(), m_rect.Height());
			m_pOldBitmap = SelectObject(&m_bitmap);

			SetMapMode(pDC->GetMapMode());

			SetWindowExt(pDC->GetWindowExt());
			SetViewportExt(pDC->GetViewportExt());

			pDC->DPtoLP(&m_rect);
			SetWindowOrg(m_rect.left, m_rect.top);
		} else {
			// Make a copy of the relevant parts of the current DC for printing
			m_bPrinting = pDC->m_bPrinting;
			m_hDC       = pDC->m_hDC;
			m_hAttribDC = pDC->m_hAttribDC;
		}

		// Fill background
		FillSolidRect(m_rect, pDC->GetBkColor());
	}

	// Destructor copies the contents of the mem DC to the original DC
	~CMyMemDC()
	{
		if (m_bMyMemDC) {
			// Copy the off screen bitmap onto the screen.
			if (!m_bTempOnly)
				m_pDC->BitBlt(m_rect.left, m_rect.top, m_rect.Width(), m_rect.Height(),
								this, m_rect.left, m_rect.top, SRCCOPY);

			//Swap back the original bitmap.
			SelectObject(m_pOldBitmap);
		} else {
			// All we need to do is replace the DC with an illegal value,
			// this keeps us from accidentally deleting the handles associated with
			// the CDC that was passed to the constructor.
			m_hDC = m_hAttribDC = nullptr;
		}
	}

	// Allow usage as a pointer
	CMyMemDC* operator->() {return this;}

	// Allow usage as a pointer
	operator CMyMemDC*() {return this;}

private:
	CBitmap  m_bitmap;		// Off screen bitmap
	CBitmap* m_pOldBitmap;	// bitmap originally found in CMyMemDC
	CDC*     m_pDC;			// Saves CDC passed in constructor
	CRect    m_rect;		// Rectangle of drawing area.
	BOOL     m_bMyMemDC;		// TRUE if CDC really is a Memory DC.
	BOOL	 m_bTempOnly;	// Whether to copy the contents on the real DC on destroy
};
#else
class CMyMemDC
{
public:

	// constructor sets up the memory DC
	CMyMemDC(HDC hDC, bool bTempOnly = false)
	{
		m_hDC = hDC;
		m_hOldBitmap = nullptr;
		m_bTempOnly = bTempOnly;

		GetClipBox(m_hDC, &m_rect);
		m_hMyMemDC = ::CreateCompatibleDC(m_hDC);
		m_hBitmap = CreateCompatibleBitmap(m_hDC, m_rect.right - m_rect.left, m_rect.bottom - m_rect.top);
		m_hOldBitmap = static_cast<HBITMAP>(SelectObject(m_hMyMemDC, m_hBitmap));
		SetWindowOrgEx(m_hMyMemDC, m_rect.left, m_rect.top, nullptr);
	}

	// Destructor copies the contents of the mem DC to the original DC
	~CMyMemDC()
	{
		if (m_hMyMemDC) {
			// Copy the off screen bitmap onto the screen.
			if (!m_bTempOnly)
				BitBlt(m_hDC, m_rect.left, m_rect.top, m_rect.right-m_rect.left, m_rect.bottom-m_rect.top, m_hMyMemDC, m_rect.left, m_rect.top, SRCCOPY);

			//Swap back the original bitmap.
			SelectObject(m_hMyMemDC, m_hOldBitmap);
			DeleteObject(m_hBitmap);
			DeleteDC(m_hMyMemDC);
		} else {
			// All we need to do is replace the DC with an illegal value,
			// this keeps us from accidentally deleting the handles associated with
			// the CDC that was passed to the constructor.
			DeleteObject(m_hBitmap);
			DeleteDC(m_hMyMemDC);
			m_hMyMemDC = nullptr;
		}
	}

	// Allow usage as a pointer
	operator HDC() {return m_hMyMemDC;}
private:
	HBITMAP  m_hBitmap;		// Off screen bitmap
	HBITMAP	 m_hOldBitmap;	// bitmap originally found in CMyMemDC
	HDC      m_hDC;			// Saves CDC passed in constructor
	HDC		 m_hMyMemDC;		// our own memory DC
	RECT	 m_rect;		// Rectangle of drawing area.
	bool	 m_bTempOnly;	// Whether to copy the contents on the real DC on destroy
};

#endif
