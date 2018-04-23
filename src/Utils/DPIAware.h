// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2018 - TortoiseSVN

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
class CDPIAware
{
private:
	CDPIAware() : m_fInitialized(false), m_dpiX(96), m_dpiY(96) {}
	~CDPIAware() {}

public:
	static CDPIAware&   Instance()
	{
		static CDPIAware instance;
		return instance;
	}

	// Get screen DPI.
	int GetDPIX() { _Init(); return m_dpiX; }
	int GetDPIY() { _Init(); return m_dpiY; }

	// Convert between raw pixels and relative pixels.
	int ScaleX(int x) { _Init(); return MulDiv(x, m_dpiX, 96); }
	int ScaleY(int y) { _Init(); return MulDiv(y, m_dpiY, 96); }
	float ScaleFactorX() { _Init(); return m_dpiX / 96.0f; }
	float ScaleFactorY() { _Init(); return m_dpiY / 96.0f; }
	int UnscaleX(int x) { _Init(); return MulDiv(x, 96, m_dpiX); }
	int UnscaleY(int y) { _Init(); return MulDiv(y, 96, m_dpiY); }

	// Determine the screen dimensions in relative pixels.
	int ScaledScreenWidth() { return _ScaledSystemMetricX(SM_CXSCREEN); }
	int ScaledScreenHeight() { return _ScaledSystemMetricY(SM_CYSCREEN); }

	// Scale rectangle from raw pixels to relative pixels.
	void ScaleRect(__inout RECT *pRect)
	{
		pRect->left = ScaleX(pRect->left);
		pRect->right = ScaleX(pRect->right);
		pRect->top = ScaleY(pRect->top);
		pRect->bottom = ScaleY(pRect->bottom);
	}

	// Scale Point from raw pixels to relative pixels.
	void ScalePoint(__inout POINT *pPoint)
	{
		pPoint->x = ScaleX(pPoint->x);
		pPoint->y = ScaleY(pPoint->y);
	}

	// Scale Size from raw pixels to relative pixels.
	void ScaleSize(__inout SIZE *pSize)
	{
		pSize->cx = ScaleX(pSize->cx);
		pSize->cy = ScaleY(pSize->cy);
	}

	// Determine if screen resolution meets minimum requirements in relative pixels.
	bool IsResolutionAtLeast(int cxMin, int cyMin)
	{
		return (ScaledScreenWidth() >= cxMin) && (ScaledScreenHeight() >= cyMin);
	}

	// Convert a point size (1/72 of an inch) to raw pixels.
	int PointsToPixels(int pt) { return MulDiv(pt, m_dpiY, 72); }

	// Invalidate any cached metrics.
	void Invalidate() { m_fInitialized = false; }

private:

	// This function initializes the CDPIAware Class
	void _Init()
	{
		if (!m_fInitialized)
		{
			HDC hdc = GetDC(nullptr);
			if (hdc)
			{
				// Initialize the DPI member variable
				// This will correspond to the DPI setting
				// With all Windows OS's to date the X and Y DPI will be identical
				m_dpiX = GetDeviceCaps(hdc, LOGPIXELSX);
				m_dpiY = GetDeviceCaps(hdc, LOGPIXELSY);
				ReleaseDC(nullptr, hdc);
			}
			m_fInitialized = true;
		}
	}

	// This returns a 96-DPI scaled-down equivalent value for nIndex
	// For example, the value 120 at 120 DPI setting gets scaled down to 96
	// X and Y versions are provided, though to date all Windows OS releases
	// have equal X and Y scale values
	int _ScaledSystemMetricX(int nIndex)
	{
		_Init();
		return MulDiv(GetSystemMetrics(nIndex), 96, m_dpiX);
	}

	// This returns a 96-DPI scaled-down equivalent value for nIndex
	// For example, the value 120 at 120 DPI setting gets scaled down to 96
	// X and Y versions are provided, though to date all Windows OS releases
	// have equal X and Y scale values
	int _ScaledSystemMetricY(int nIndex)
	{
		_Init();
		return MulDiv(GetSystemMetrics(nIndex), 96, m_dpiY);
	}

private:

	// Member variable indicating whether the class has been initialized
	bool m_fInitialized;

	// X and Y DPI values are provided, though to date all
	// Windows OS releases have equal X and Y scale values
	int m_dpiX;
	int m_dpiY;
};
