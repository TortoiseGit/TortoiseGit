// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2003-2006,2008 - TortoiseSVN

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
#include "StdAfx.h"
#include "htmlformatter.h"

CHTMLFormatter::CHTMLFormatter(void)
{
}

CHTMLFormatter::~CHTMLFormatter(void)
{
}

CSize CHTMLFormatter::DrawHTML(CDC * pDC, CRect rect, CString str, LOGFONT font, BOOL bCalculate /* = FALSE */)
{
	CUIntArray nLengthLines;
	int nLine = 0;
	int nCmd = NONE;
	STATEMACHINE nState = BEGIN_TAG;
	int nAlign = ALIGN_LEFT;
	BOOL bCloseTag = FALSE;

	m_arLinkRects.RemoveAll();
	m_arLinkURLs.RemoveAll();

	CSize sz(0, 0);

	if (str.IsEmpty())
		return sz;

	CPoint	pt = rect.TopLeft();
	CPoint  ptCur = pt;
	

	COLORREF crText = pDC->GetTextColor();
	COLORREF crBg = pDC->GetBkColor();

	LOGFONT lf;
    memcpy(&lf, &font, sizeof(LOGFONT));

	CFont tempFont;
	tempFont.CreateFontIndirect(&lf);

	CFont * pOldFont = pDC->SelectObject(&tempFont);

	TEXTMETRIC textMetric;
	pDC->GetTextMetrics(&textMetric);
	int nHeight = textMetric.tmHeight;
	int nWidth = textMetric.tmAveCharWidth;

	CString strTag = _T("");
	CString strText = _T("");
	UINT nParam = 0;
	
	CRect linkRect;
	linkRect.SetRectEmpty();

	CUIntArray percent;
	percent.Add(0);
	
	int nTemp = 0; //the temporary variable
	BOOL bFirstOutput = TRUE;

	//iterate through all characters of the string
	for (int i = 0; i <= str.GetLength(); i++)
	{
		if (i < str.GetLength())
		{
			//Searches the command and parameters in the string
			switch (nState)
			{
			case BEGIN_TAG:
				//waiting for the begin of a tag (<tag>), newline ('\n' or '\r') or tab ('\t')
				switch (str.GetAt(i))
				{
				case _T('<'):
					nState = TEXT_TAG;	//statemachine to 'waiting for the tag'
					bCloseTag = FALSE;		//opening bracket
					strTag = _T("");
					break;
				case _T('\n'):
					nCmd = NEW_LINE;
					nParam = 1;
					break;
				case _T('\t'):
					nCmd = TABULATION;
					nParam = 1;
					break;
				case _T('\r'):
					break;
				default: 
					strText += str.GetAt(i);
					break;
				}
				break;
			case TEXT_TAG:
				//get the tag itself (until the closing bracket ('>'))
				switch (str.GetAt(i))
				{
				case _T('/'):
					if (strTag.IsEmpty())
						bCloseTag = TRUE; //found the char's cancel tag
					break;
				case _T('<'):
					if (strTag.IsEmpty())
					{
						nState = BEGIN_TAG;
						strText += str.GetAt(i);
					}
					else strTag += str.GetAt(i);
					break;
				case _T('='):
				case _T('>'):
					i--;
				//case _T(' '):
					//Analyses tags
					if (strTag.CompareNoCase(_T("b"))==0)
					{
						//Bold text
						nCmd = BOLD;
						nState = END_TAG;
					}
					else if (strTag.CompareNoCase(_T("i")) == 0)
					{
						//Italic text
						nCmd = ITALIC;
						nState = END_TAG;
					}
					else if (strTag.CompareNoCase(_T("s")) == 0)
					{
						//Strikeout text
						nCmd = STRIKE;
						nState = END_TAG;
					}
					else if (strTag.CompareNoCase(_T("u")) == 0)
					{
						//Underline text
						nCmd = UNDERLINE;
						nState = END_TAG;
					}
					else if (strTag.CompareNoCase(_T("t")) == 0)
					{
						//Tabulation
						nCmd = TABULATION;
						nParam = 1;
						nState = BEGIN_NUMBER;
					}
					else if (strTag.CompareNoCase(_T("ct")) == 0)
					{
						//Color of the text
						nCmd = COLOR_TEXT;
						nParam = crText;
						nState = BEGIN_NUMBER;
					}
					else if (strTag.CompareNoCase(_T("cb")) == 0)
					{
						//Color of the background
						nCmd = COLOR_BK;
						nParam = crBg;
						nState = BEGIN_NUMBER;
					}
					else if (strTag.CompareNoCase(_T("al")) == 0)
					{
						//left align
						nAlign = ALIGN_LEFT;
						nState = END_TAG;
					}
					else if (strTag.CompareNoCase(_T("ac")) == 0)
					{
						//center align
						if (!bCalculate)
							nAlign = bCloseTag ? ALIGN_LEFT : ALIGN_CENTER;
						nState = END_TAG;
					}
					else if (strTag.CompareNoCase(_T("ar")) == 0)
					{
						//right align
						if (!bCalculate)
							nAlign = bCloseTag ? ALIGN_LEFT : ALIGN_RIGHT;
						nState = END_TAG;
					}
					else if (strTag.CompareNoCase(_T("hr")) == 0)
					{
						//horizontal line
						nCmd = HORZ_LINE_PERCENT;
						nParam = 100;
						nState = BEGIN_NUMBER;
					}
					else if (strTag.CompareNoCase(_T("a")) == 0)
					{
						//link
						nCmd = LINK;
						nState = BEGIN_URL;	//wait for '='
					}
					else nState = END_TAG; //Unknown tag
					break;
				default:
					strTag += str.GetAt(i);
					break;
				}
				break;
			case END_TAG:
				//waiting for the end of the tag
				if (str.GetAt(i) == _T('>'))
					nState = BEGIN_TAG;
				break;
			case BEGIN_NUMBER:
				//waiting for the start of a number
				if (str.GetAt(i) == _T('='))
				{
					strTag = _T("");
					nState = TEXT_NUMBER;
				}
				else if (str.GetAt(i) == _T('>'))
					nState = BEGIN_TAG; //not a number
				break;
			case BEGIN_URL:
				//waiting for the start of a number
				if (str.GetAt(i) == _T('='))
				{
					strTag = _T("");
					nState = TEXT_URL;
				}
				else if (str.GetAt(i) == _T('>'))
					nState = BEGIN_TAG; //not a url
				break;
			case TEXT_NUMBER:
				//waiting for a number string
				switch (str.GetAt(i))
				{
				case _T('>'):
					i --;
					//intended fall through!
				case _T('%'):
					//Gets the real number from the string
					if (!strTag.IsEmpty())
						nParam = _tcstoul(strTag, 0, 0);
					nState = END_TAG;
					break;
				default:
					strTag += str.GetAt(i);
					break;
				}
				break;
			case TEXT_URL:
				//waiting for a url
				switch (str.GetAt(i))
				{
				case _T('>'):
					i--;
					if (!strTag.IsEmpty())
						m_arLinkURLs.Add(strTag);
					nState = END_TAG;
					break;
				default:
					strTag += str.GetAt(i);
					break;
				}
			} // switch (nState)
		}
		else
		{
			//Immitates new line at the end of the string
			nState = BEGIN_TAG;
			nCmd = NEW_LINE;
			nParam = 1;
		}

		if ((nState == BEGIN_TAG) && (nCmd != NONE))
		{
			//New Command with full parameters
			if (!strText.IsEmpty())
			{
				if (bFirstOutput)
				{
					switch (nAlign)
					{
					case ALIGN_CENTER:
						ptCur.x = pt.x + (rect.Width() - nLengthLines.GetAt(nLine)) / 2;
						break;
					case ALIGN_RIGHT:
						ptCur.x = pt.x + rect.Width() - nLengthLines.GetAt(nLine);
						break;
					}
				}
				if (!bCalculate)
					pDC->TextOut(ptCur.x, ptCur.y, strText);
				CSize s = pDC->GetTextExtent(strText);
				linkRect.left = ptCur.x;
				linkRect.top = ptCur.y;
				linkRect.right = linkRect.left + s.cx;
				linkRect.bottom = linkRect.top + s.cy;
				ptCur.x += s.cx;
				strText = _T("");
				bFirstOutput = FALSE;
			}
			
			//Executes command
			switch (nCmd)
			{
			case LINK:
				if (bCloseTag)
				{
					//closing the link
					m_arLinkRects.Add(linkRect);
					linkRect.SetRectEmpty();
				}
				break;
			case BOLD:
				//Bold text
				pDC->SelectObject(pOldFont);
				tempFont.DeleteObject();
				lf.lfWeight = font.lfWeight;
				if (!bCloseTag)
				{
					lf.lfWeight *= 2;
					if (lf.lfWeight > FW_BLACK)
						lf.lfWeight = FW_BLACK;
				}
				tempFont.CreateFontIndirect(&lf);
				pDC->SelectObject(&tempFont);
				break;
			case ITALIC:
				//Italic text
				pDC->SelectObject(pOldFont);
				tempFont.DeleteObject();
				lf.lfItalic = bCloseTag ? FALSE : TRUE;
				tempFont.CreateFontIndirect(&lf);
				pDC->SelectObject(&tempFont);
				break;
			case STRIKE:
				//Strikeout text
				pDC->SelectObject(pOldFont);
				tempFont.DeleteObject();
				lf.lfStrikeOut = bCloseTag ? FALSE : TRUE;
				tempFont.CreateFontIndirect(&lf);
				pDC->SelectObject(&tempFont);
				break;
			case UNDERLINE:
				//Underline text
				pDC->SelectObject(pOldFont);
				tempFont.DeleteObject();
				lf.lfUnderline = bCloseTag ? FALSE : TRUE;
				tempFont.CreateFontIndirect(&lf);
				pDC->SelectObject(&tempFont);
				break;
			case COLOR_TEXT:
				//Color of the text
				pDC->SetTextColor((COLORREF)nParam);
				break;
			case COLOR_BK:
				//Color of the background
				pDC->SetBkColor((COLORREF)nParam);
				pDC->SetBkMode(bCloseTag ? TRANSPARENT : OPAQUE);
				break;
			case HORZ_LINE_PERCENT:
				//Horizontal line with percent length
				if (bCalculate)
				{
					percent.SetAt(nLine, percent.GetAt(nLine) + nParam);
					nParam = 0;
				}
				else nParam = ::MulDiv(rect.Width(), nParam, 100);
			case HORZ_LINE:
				//Horizontal line with absolute length
				//If text to output is exist
				if (bFirstOutput)
				{
					switch (nAlign)
					{
					case ALIGN_CENTER:
						ptCur.x = pt.x + (rect.Width() - nLengthLines.GetAt(nLine)) / 2;
						break;
					case ALIGN_RIGHT:
						ptCur.x = pt.x + rect.Width() - nLengthLines.GetAt(nLine);
						break;
					}
				}
				DrawHorzLine(pDC, ptCur.x, ptCur.x + nParam, ptCur.y + nHeight / 2);
				ptCur.x += nParam;
				bFirstOutput = FALSE;
				break;
			case NEW_LINE:
				//New line
				if (!nParam)
					nParam = 1;
				sz.cx = max(sz.cx, ptCur.x - pt.x);
				nLengthLines.Add(ptCur.x - pt.x); //Adds the real length of the lines
				ptCur.y += nHeight * nParam;
				nLine ++;
				percent.Add(0);
				bFirstOutput = TRUE;
				ptCur.x = pt.x;
				break;
			case TABULATION:
				//Tabulation
				if (!nParam)
					nParam = 1;
				nTemp = (ptCur.x - pt.x) % (nWidth * 4);
				if (nTemp)
				{
					//aligns with tab
					ptCur.x += (nWidth * 4) - nTemp;
					nParam --;
				}
				ptCur.x += (nParam * nWidth * 4);
				break;
			}
			//Resets the last command
			nCmd = NONE;
			bCloseTag = FALSE;
		}
	}

	//Gets real height of the tooltip
	sz.cy = ptCur.y - pt.y;

	pDC->SelectObject(pOldFont);
	tempFont.DeleteObject();

	//Adds the percent's length to the line's length
	for (int i = 0; i < percent.GetSize(); i++)
	{
		if (percent.GetAt(i))
			nLengthLines.SetAt(i, nLengthLines.GetAt(i) + ::MulDiv(percent.GetAt(i), sz.cx, 100));
	}

	return sz;
}

void CHTMLFormatter::DrawHorzLine(CDC * pDC, int xStart, int xEnd, int y)
{
	CPen pen(PS_SOLID, 1, pDC->GetTextColor());
	CPen * penOld = pDC->SelectObject(&pen);
	pDC->MoveTo(xStart, y);
	pDC->LineTo(xEnd, y);
	pDC->SelectObject(penOld);
	pen.DeleteObject();
}

BOOL CHTMLFormatter::IsPointOverALink(CPoint pt)
{
	CRect rect;
	for (int i=0; i<m_arLinkRects.GetCount(); i++)
	{
		rect = m_arLinkRects.GetAt(i);
		if (rect.PtInRect(pt))
			return TRUE;
	}
	return FALSE;
}

CString CHTMLFormatter::GetLinkForPoint(CPoint pt)
{
	CRect rect;
	for (int i=0; i<m_arLinkRects.GetCount(); i++)
	{
		rect = m_arLinkRects.GetAt(i);
		if (rect.PtInRect(pt))
		{
			return m_arLinkURLs.GetAt(i);
		}
	}
	return _T("");
}
