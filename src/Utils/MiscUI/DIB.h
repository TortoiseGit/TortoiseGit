// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2003-2008 - TortoiseSVN

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
#pragma once

/**
 * \ingroup Utils
 *
 * A wrapper class for DIB's. It provides only a very small
 * amount of methods (just the ones I need). Especially for
 * creating 32bit 'image fields' which can be used for
 * implementing image filters.
 */
class CDib : public CObject
{
public:
	CDib();
	virtual ~CDib();

	/**
	 * Clears all member variables and frees allocated memory.
	 */
	void		DeleteObject();
	/**
	 * Gets the number of bytes per horizontal line in the image.
	 * \param nWidth the width of the image
	 * \param nBitsPerPixel number of bits per pixel (color depth)
	 */
	static int	BytesPerLine(int nWidth, int nBitsPerPixel);
	/**
	 * Returns the height of the image in pixels
	 */
	int			GetHeight() const { return m_BMinfo.bmiHeader.biHeight; }
	/**
	 * Returns the width of the image in pixels
	 */
	int			GetWidth() const { return m_BMinfo.bmiHeader.biWidth; }
	/**
	 * Returns the size of the image in pixels
	 */
	CSize		GetSize() const { return CSize(GetWidth(), GetHeight()); }
	/**
	 * Returns the image byte field which can be used to work on.
	 */
	LPVOID		GetDIBits() { return m_pBits; }
	/**
	 * Creates a DIB from a CPictureHolder object with the specified width and height.
	 * \param pPicture the CPictureHolder object
	 * \param iWidth the width of the resulting picture
	 * \param iHeight the height of the resulting picture
	 */
	void		Create32BitFromPicture (CPictureHolder* pPicture, int iWidth, int iHeight);

	/**
	 * Returns a 32-bit RGB color
	 */
	static COLORREF	FixColorRef		(COLORREF clr);
	/**
	 * Sets the created Bitmap-image (from Create32BitFromPicture) to the internal
	 * member variables and fills in all required values for this class.
	 * \param lpBitmapInfo a pointer to a BITMAPINFO structure
	 * \param lpBits pointer to the image byte field
	 */
	BOOL		SetBitmap(const LPBITMAPINFO lpBitmapInfo, const LPVOID lpBits);

public:
	/**
	 * Draws the image on the specified device context at the specified point.
	 * No stretching is done!
	 * \param pDC the device context to draw on
	 * \param ptDest the upper left corner to where the picture should be drawn to
	 */
	BOOL		Draw(CDC* pDC, CPoint ptDest);

protected:
	HBITMAP		m_hBitmap;
	BITMAPINFO  m_BMinfo;
	VOID		*m_pBits;
};

