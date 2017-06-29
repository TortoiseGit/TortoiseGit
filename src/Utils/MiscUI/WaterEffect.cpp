// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2003-2006, 2009-2010, 2012-2013 - TortoiseSVN

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
#include "WaterEffect.h"

#include <math.h>

CWaterEffect::CWaterEffect()
	: m_iWidth(0)
	, m_iHeight(0)

	, m_iLightModifier(10)
	, m_iHpage(0)
	, m_iDensity(2)
{
}

CWaterEffect::~CWaterEffect()
{
}

void CWaterEffect::Create(int iWidth, int iHeight)
{
	m_iBuffer1.reset (new int[iWidth*iHeight]);
	m_iBuffer2.reset (new int[iWidth*iHeight]);

	m_iWidth = iWidth;
	m_iHeight = iHeight;

	ClearWater();

	m_iHpage = 0;
}

void CWaterEffect::Blob(int x, int y, int radius, int height, int page)
{
	int *pNew;

	if (page == 0)
		pNew = m_iBuffer1.get();
	else
		pNew = m_iBuffer2.get();

	if (x<0)
		x = 1 + radius + rand() % (m_iWidth - 2 * radius - 1);
	if (y<0)
		y = 1 + radius + rand() % (m_iHeight - 2 * radius - 1);

	int left = -radius;
	int right = radius;
	int top = -radius;
	int bottom = radius;

	// clip edges
	if (x - radius < 1)
		left -= (x-radius-1);
	if (y - radius < 1)
		top  -= (y-radius-1);
	if (x + radius > m_iWidth-1)
		right -= (x+radius-m_iWidth+1);
	if (y + radius > m_iHeight-1)
		bottom-= (y+radius-m_iHeight+1);

	const int rquad = radius * radius;
	for(int cy = top; cy < bottom; cy++)
	{
		const int cyq = cy*cy;
		for(int cx = left; cx < right; cx++)
		{
			if (cx*cx + cyq < rquad)
				pNew[m_iWidth*(cy+y) + (cx+x)] += height;
		}
	}
}

void CWaterEffect::ClearWater()
{
	// clear height fields
	SecureZeroMemory(m_iBuffer1.get(), (m_iWidth*m_iHeight)*sizeof(int));
	SecureZeroMemory(m_iBuffer2.get(), (m_iWidth*m_iHeight)*sizeof(int));
}

void CWaterEffect::Render(DWORD* pSrcImage, DWORD* pTargetImage)
{
	DrawWater(m_iHpage, m_iLightModifier, pSrcImage, pTargetImage);

	CalcWater(m_iHpage, m_iDensity);

	//change the field from 0 to 1 and vice versa
	m_iHpage ^= 1;
}

void CWaterEffect::CalcWater(int npage, int density)
{
	int *pNew;
	int *pOld;

	if (npage == 0)
	{
		pNew = m_iBuffer1.get();
		pOld = m_iBuffer2.get();
	}
	else
	{
		pNew = m_iBuffer2.get();
		pOld = m_iBuffer1.get();
	}

	// a description of the algorithm and an implementation
	// in 'pseudocode' can be found here:
	// http://freespace.virgin.net/hugo.elias/graphics/x_water.htm
	int count = m_iWidth + 1;
	for (int y = (m_iHeight-1)*m_iWidth; count < y; count += 2)
	{
		for (int x = count+m_iWidth-2; count < x; count++)
		{
			// use eight pixels
			const int newh = ((pOld[count + m_iWidth]
								+ pOld[count - m_iWidth]
								+ pOld[count + 1]
								+ pOld[count - 1]
								+ pOld[count - m_iWidth - 1]
								+ pOld[count - m_iWidth + 1]
								+ pOld[count + m_iWidth - 1]
								+ pOld[count + m_iWidth + 1]
								) >> 2 )
								- pNew[count];

			pNew[count] = newh - (newh >> density);
		}
	}
}

void CWaterEffect::SmoothWater(int npage)
{
	//flatten and spread the waves
	int *pNew;
	int *pOld;

	if (npage == 0)
	{
		pNew = m_iBuffer1.get();
		pOld = m_iBuffer2.get();
	}
	else
	{
		pNew = m_iBuffer2.get();
		pOld = m_iBuffer1.get();
	}

	// a description of the algorithm and an implementation
	// in 'pseudocode' can be found here:
	// http://freespace.virgin.net/hugo.elias/graphics/x_water.htm
	int count = m_iWidth + 1;
	for(int y=1; y<m_iHeight-1; y++)
	{
		for(int x=1; x<m_iWidth-1; x++)
		{
			const int newh = ((pOld[count + m_iWidth]
							+ pOld[count - m_iWidth]
							+ pOld[count + 1]
							+ pOld[count - 1]
							+ pOld[count - m_iWidth - 1]
							+ pOld[count - m_iWidth + 1]
							+ pOld[count + m_iWidth - 1]
							+ pOld[count + m_iWidth + 1]
							) >> 3 )
							+ pNew[count];

			pNew[count] = newh>>1;
			count++;
		}
		count += 2;
	}
}

void CWaterEffect::DrawWater(int /*page*/, int /*LightModifier*/, DWORD* pSrcImage, DWORD* pTargetImage)
{
	int dx, dy;
	int x, y;
	DWORD c;

	int offset = m_iWidth + 1;
	long lIndex;
	long lBreak = m_iWidth*m_iHeight;

	int *ptr = m_iBuffer1.get();


	for (y = (m_iHeight-1)*m_iWidth; offset < y; offset += 2)
	{
		for (x = offset + m_iWidth - 2; offset < x; offset++)
		{
			dx = ptr[offset] - ptr[offset+1];
			dy = ptr[offset] - ptr[offset+m_iWidth];

			lIndex = offset + m_iWidth*(dy>>3) + (dx>>3);
			if (lIndex < lBreak && lIndex > 0)
			{
				c = pSrcImage[lIndex];
				c = GetShiftedColor(c,dx);
				pTargetImage[offset] = c;
			}

			offset++;
			dx = ptr[offset] - ptr[offset+1];
			dy = ptr[offset] - ptr[offset+m_iWidth];

			lIndex = offset + m_iWidth*(dy>>3) + (dx>>3);
			if (lIndex < lBreak && lIndex > 0)
			{
				c = pSrcImage[lIndex];
				c = GetShiftedColor(c,dx);
				pTargetImage[offset] = c;
			}
		}
	}
}

COLORREF CWaterEffect::GetShiftedColor(COLORREF color, int shift)
{
	const long R = GetRValue(color)-shift;
	const long G = GetGValue(color)-shift;
	const long B = GetBValue(color)-shift;

	const int ir = NormalizeColor(R);
	const int ig = NormalizeColor(G);
	const int ib = NormalizeColor(B);

	return RGB(ir,ig,ib);
}

int CWaterEffect::NormalizeColor(long color) const
{
	if(color < 0)
		return 0;
	if(color > 255)
		return 255;
	return color;
}
