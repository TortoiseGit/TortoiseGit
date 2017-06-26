// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2003-2007, 2009, 2012-2015, 2017 - TortoiseSVN

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

#pragma once
#include "tstring.h"
#include <string>
#include <ocidl.h>
#pragma warning(push)
#pragma warning(disable: 4458) // declaration of 'xxx' hides class member
#include <GdiPlus.h>
#pragma warning(pop)

using namespace Gdiplus;

/**
 * \ingroup Utils
 * Class for showing picture files.
 * Use this class to show pictures of different file formats: BMP, DIB, EMF, GIF, ICO, JPG, WMF
 * If Gdi+ is installed (default on XP and later, optional on Win2k), other image formats can
 * be shown too: png, tiff.
 * The class uses the IPicture interface, the same way as internet explorer does.
 *
 * Example of usage:
 * \code
 * CPicture m_picture;
 * //load picture data into the IPicture interface
 * m_picture.Load("Test.jpg");	//load from a file
 * m_picture.Load(IDR_TEST, "JPG");	//load from a resource
 *
 * //when using in a dialog based application (CPaintDC dc(this);)
 * m_picture.UpdateSizeOnDC(&dc);	//get picture dimensions in pixels
 * m_picture.Show(&dc, CPoint(0,0), CPoint(m_picture.m_Width, m_picture.m_Height), 0, 0);
 * m_picture.Show(&dc, CRect(0,0,100,100)); //change original dimensions
 * m_picture.ShowBitmapResource(&dc, IDB_TEST, CPoint(0,0));	//show bitmap resource
 *
 * //when using in a regular mfc application (CDC* pDC)
 * m_picture.UpdateSizeOnDC(pDC);	//get picture dimensions in pixels
 * m_picture.Show(pDC, CPoint(0,0), CPoint(m_picture.m_Width, m_picture.m_Height), 0, 0);
 * m_picture.Show(pDC, CRect(0,0,100,100)); //change original dimensions
 * m_picture.ShowBitmapResource(pDC, IDB_TEST, CPoint(0,0));	//show bitmap resource
 *
 * //to show picture information
 * std::string s;
 * s.Format("Size = %4d\nWidth = %4d\nHeight = %4d\nWeight = %4d\n",
 *              m_picture.m_Weigth, m_picture.m_Width, m_picture.m_Height, m_picture.m_Weight);
 * AfxMessageBox(s);
 * \endcode
 * \remark GDI+ is only used if it is installed. If you link with gdiplus.lib and mark the gdiplus.dll
 * as a delay-loaded dll, then your application won't throw an error if it isn't installed, but of course
 * png and tiff images can't be shown.
 */
class CPicture
{
public:
	/**
	 * open a picture file and load it (BMP, DIB, EMF, GIF, ICO, JPG, WMF).
	 *
	 * \param sFilePathName the path of the picture file
	 * \return TRUE if succeeded.
	 */
	bool Load(tstring sFilePathName);
	/**
	 * draws the loaded picture directly to the given device context.
	 * \note
	 * if the given size is not the actual picture size, then the picture will
	 * be drawn stretched to the given dimensions.
	 * \param hDC the device context to draw on
	 * \param DrawRect the dimensions to draw the picture on
	 * \return TRUE if succeeded
	 */
	bool Show(HDC hDC, RECT DrawRect);
	/**
	 * get the original picture pixel size. A pointer to a device context is needed
	 * for the pixel calculation (DPI). Also updates the classes height and width
	 * members.
	 * \param hDC the device context to perform the calculations on
	 * \return TRUE if succeeded
	 */
	bool UpdateSizeOnDC(HDC hDC);

	/**
	 * Return the horizontal resolutions in dpi of the loaded picture.
	 * \remark this only works if gdi+ is installed.
	 */
	float GetHorizontalResolution() {return pBitmap ? pBitmap->GetHorizontalResolution() : 0.0f;}
	/**
	 * Return the vertical resolution in dpi of the loaded picture.
	 * \remark this only works if gdi+ is installed.
	 */
	float GetVerticalResolution() {return pBitmap ? pBitmap->GetVerticalResolution() : 0.0f;}
	/**
	 * Returns the picture height in pixels.
	 * \remark this only works if gdi+ is installed.
	 */
	UINT GetHeight() const;
	/**
	 * Returns the picture width in pixels.
	 * \remark this only works if gdi+ is installed.
	 */
	UINT GetWidth() const;
	/**
	 * Returns the pixel format of the loaded picture.
	 * \remark this only works if gdi+ is installed.
	 */
	PixelFormat GetPixelFormat() const;
	/**
	 * Returns the color depth in bits.
	 * \remark this only works if gdi+ is installed.
	 */
	UINT GetColorDepth() const;

	/**
	 * Sets the interpolation used for drawing the image.
	 * The interpolation mode is one of the following:
	 * InterpolationModeInvalid
	 * InterpolationModeDefault
	 * InterpolationModeLowQuality
	 * InterpolationModeHighQuality
	 * InterpolationModeBilinear
	 * InterpolationModeBicubic
	 * InterpolationModeNearestNeighbor
	 * InterpolationModeHighQualityBilinear
	 * InterpolationModeHighQualityBicubic
	 */
	void SetInterpolationMode(InterpolationMode ip) {m_ip = ip;}

	/**
	 * Returns the number of frames in the specified dimension of the image.
	 */
	UINT GetNumberOfFrames(int dimension);
	/**
	 * Returns the number of dimensions in the image.
	 * For example, icons can have multiple dimensions (sizes).
	 */
	UINT GetNumberOfDimensions();

	/**
	 * Sets the active frame which is used when drawing the image.
	 * \return the delay value for this frame, i.e. how long this frame
	 * should be shown.
	 */
	long SetActiveFrame(UINT frame);

	/**
	 * frees the allocated memory that holds the IPicture interface data and
	 * clear picture information
	 */
	void FreePictureData();

	DWORD GetFileSize() const {return m_nSize;}
	tstring GetFileSizeAsText(bool bAbbrev = true);
	CPicture();
	virtual ~CPicture();


	IPicture* m_IPicture;	///< Same As LPPICTURE (typedef IPicture __RPC_FAR *LPPICTURE)

	LONG		m_Height;	///< Height (in pixels)
	LONG		m_Width;	///< Width (in pixels)
	UINT		m_ColorDepth;///< the color depth
	LONG		m_Weight;	///< Size Of The Image Object In Bytes (File OR Resource)
	tstring m_Name;			///< The FileName of the Picture as used in Load()

protected:
	/**
	 * reads the picture data from a source and loads it into the current IPicture object in use.
	 * \param pBuffer buffer of data source
	 * \param nSize the size of the buffer
	 * \return TRUE if succeeded
	 */
	bool LoadPictureData(BYTE* pBuffer, int nSize);

private:
	GdiplusStartupInput gdiplusStartupInput;
	ULONG_PTR			gdiplusToken;
	Bitmap *			pBitmap;
	BYTE*				pBitmapBuffer;
	InterpolationMode	m_ip;
	bool				bIsIcon;
	bool				bIsTiff;
	UINT				nCurrentIcon;
	BYTE *				lpIcons;
	HICON *				hIcons;
	DWORD				m_nSize;

	#pragma pack(push, r1, 2)   // n = 16, pushed to stack

	typedef struct
	{
		BYTE	bWidth;               // Width of the image
		BYTE	bHeight;              // Height of the image (times 2)
		BYTE	bColorCount;          // Number of colors in image (0 if >=8bpp)
		BYTE	bReserved;            // Reserved
		WORD	wPlanes;              // Color Planes
		WORD	wBitCount;            // Bits per pixel
		DWORD	dwBytesInRes;         // how many bytes in this resource?
		DWORD	dwImageOffset;        // where in the file is this image
	} ICONDIRENTRY, *LPICONDIRENTRY;
	typedef struct
	{
		WORD			idReserved;   // Reserved
		WORD			idType;       // resource type (1 for icons)
		WORD			idCount;      // how many images?
		ICONDIRENTRY	idEntries[1]; // the entries for each image
	} ICONDIR, *LPICONDIR;
	#pragma pack(pop, r1)
};

