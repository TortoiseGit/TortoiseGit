// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2006 - Stefan Kueng

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
 * Class for drawing formatted text on a device context.
 */
class CHTMLFormatter
{
public:
	CHTMLFormatter(void);
	~CHTMLFormatter(void);
	/**
	 * Draws formatted text to the given device context with the given font. The following formatting
	 * parameters are available:\n
	 * - \c \<b> \e text \c \</b> \t draws the text in bold
	 * - \c \<u> \e text \c \</u> \t draws the text underlined
	 * - \c \<i> \e text \c \</u> \t draws the text in italic
	 * - \c \<s> \e text \c \</u> \t draws the text with strikeout
	 * 
	 * - \c \<ct=0x123456> \e text \c \</ct> \t draws the text in the given color. The value is in RGB format.
	 * - \c \<cb=0x123456> \e text \c \</cb> \t draws the background of the text in the given color. The value is in RGB format.
	 *
	 * - \c \<al> \e text \c \</al> \t aligns the text to the left
	 * - \c \<ac> \e text \c \</ac> \t aligns the text to the center
	 * - \c \<ar> \e text \c \</ar> \t aligns the text to the right
	 *
	 * - \c \<hr=50%> \t draws a horizontal line with 50% of the whole width
	 * - \c \<hr=100> \t draws a horizontal line with 100 pixels length
	 * 
	 * - \c \<a=http://something.com> \e text \c \</a> \t the text is marked as a link in the internal link list.
	 *
	 * also the common control codes \\n, \\t, \\r are recognized.
	 * 
	 * An example of usage:
	 * \code
	 * CHTMLFormatter formatter;
	 *
	 * CString strInfo = _T("<ct=0x0000FF><<b></ct>text<ct=0x0000FF><</b></ct><t=8> - <b>Bold text</b>\n");
	 * strInfo += _T("<ct=0x0000FF><<i></ct>text<ct=0x0000FF><</i></ct><t=8> - <i>Italic text</i>\n");
	 * strInfo += _T("<ct=0x0000FF><<u></ct>text<ct=0x0000FF><</u></ct><t=8> - <u>Underline text</u>\n");
	 * strInfo += _T("<ct=0x0000FF><<s></ct>text<ct=0x0000FF><</s></ct><t=8> - <s>Strikeout text</s>\n");
	 * strInfo += _T("<ct=0x0000FF><<ct=0x0000FF></ct>text<ct=0x0000FF><</ct></ct><t=5> - <ct=0x0000FF>Red text</ct>\n");
	 * strInfo += _T("<ct=0x0000FF><<cb=0xFFFF00></ct>text<ct=0x0000FF><</cb></ct><t=5> - <cb=0xFFFF00>Cyan background</cb>\n");
	 * strInfo += _T("<ct=0x0000FF><<t></ct><t=10> - Tabulation\n");
	 * strInfo += _T("<ct=0x0000FF><<hr=80%></ct><t=9> - Horizontal line\n");
	 * strInfo += _T("<hr=80%>\n");
	 * strInfo += _T("<ct=0x0000FF><<al></ct><t=10> - Left align\n");
	 * strInfo += _T("<ct=0x0000FF><<ac></ct><t=10> - Center align\n");
	 * strInfo += _T("<ct=0x0000FF><<ar></ct><t=10> - Right align\n");
	 * strInfo += _T("<ct=0x0000FF><<a=http://somelink.com><<u></ct>link<ct=0x0000FF><</u><</a></ct><t> - Link\n");
	 *
	 * formatter.DrawHTML(pDC, rect, strInfo, font);
	 * \endcode
	 * this example produces the following picture:
	 * \image html "htmlformatter.png"
	 * 
	 * \remarks please be aware that this is a lightweight class and not a real HTML printer. Only very basic
	 * tags are available and folding tags is also very limited.\n
	 * The link tag also has restrictions: 
	 * - a link must not span more than one line of text
	 * - inner tags are allowed, but only for the whole link text. I.e. "<a=http://something.com><b>this</b> is my link</a>" is not allowed!
	 * 
	 * \param pDC the device context to draw the text on
	 * \param rect the rectangle to draw the text within
	 * \param str the string to draw
	 * \param font the font to draw the text with
	 * \param bCalculate if set to TRUE, then no drawing is done but only the size of the required rectangle to fit
	 * the text is calculated.
	 * \return The required size of the rectangle to fit the text in.
	 */
	CSize DrawHTML(CDC * pDC, CRect rect, CString str, LOGFONT font, BOOL bCalculate = FALSE);

	/**
	 * Checks if a given point is over a hyperlink text
	 */
	BOOL IsPointOverALink(CPoint pt);

	/**
	 * Returns the URL of the link or an empty string if the point is not over a hyperlink text.
	 */
	CString GetLinkForPoint(CPoint pt);

protected:
	typedef enum{	NONE = 0,
		BOLD,
		ITALIC,
		STRIKE,
		UNDERLINE,
		COLOR_TEXT,
		COLOR_BK,
		NEW_LINE,
		TABULATION,
		HORZ_LINE,
		HORZ_LINE_PERCENT,
		LINK
	} COMMAND;

	typedef enum{   BEGIN_TAG = 0,
		END_TAG,
		TEXT_TAG,
		BEGIN_NUMBER,
		TEXT_NUMBER,
		BEGIN_TEXT,
		TEXT,
		PERCENT,
		BEGIN_URL,
		TEXT_URL
	} STATEMACHINE;

	enum{	ALIGN_LEFT = 0,
		ALIGN_CENTER,
		ALIGN_RIGHT
	};
	/**
	 * Draws a horizontal line to the device context
	 * \param pDC the device context to draw to
	 * \param xStart the starting x coordinate of the line
	 * \param xEnd the ending x coordinate of the line
	 * \param y the y coordinate of the line
	 */
	static void DrawHorzLine(CDC * pDC, int xStart, int xEnd, int y);


	CArray<CRect, CRect&>	m_arLinkRects;
	CStringArray m_arLinkURLs;
};
