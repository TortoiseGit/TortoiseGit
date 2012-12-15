/*
 * $Revision: 2554 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-06 11:39:38 +0200 (Fr, 06. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief implementation of class TileToRowsCCPacker
 *
 * \author Carsten Gutwenger
 *
 * \par License:
 * This file is part of the Open Graph Drawing Framework (OGDF).
 *
 * \par
 * Copyright (C)<br>
 * See README.txt in the root directory of the OGDF installation for details.
 *
 * \par
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * Version 2 or 3 as published by the Free Software Foundation;
 * see the file LICENSE.txt included in the packaging of this file
 * for details.
 *
 * \par
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * \par
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the Free
 * Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 * \see  http://www.gnu.org/copyleft/gpl.html
 ***************************************************************/

#include <ogdf/packing/TileToRowsCCPacker.h>
#include <ogdf/basic/SList.h>


namespace ogdf {


template<class POINT>
struct TileToRowsCCPacker::RowInfo {
	SListPure<int> m_boxes;
	typename POINT::numberType m_maxHeight, m_width;

	RowInfo() {
		m_maxHeight = m_width = 0;
	}
};


template<class POINT>
class DecrIndexComparer
{
	const Array<POINT> &m_box;

public:
	DecrIndexComparer(const Array<POINT> &box) : m_box(box) { }

	int compare(const int &i, const int &j) const
	{
		typename POINT::numberType y1 = m_box[i].m_y, y2 = m_box[j].m_y;

		if (y1 > y2) return -1;
		else if (y1 < y2) return 1;
		else return 0;
	}

	// avoid automatic creation of assignment operator
	DecrIndexComparer<POINT> &operator=(const DecrIndexComparer<POINT> &);

	OGDF_AUGMENT_COMPARER(int)
};


void TileToRowsCCPacker::call(Array<DPoint> &box,
	Array<DPoint> &offset,
	double pageRatio)
{
	callGeneric(box,offset,pageRatio);
}


void TileToRowsCCPacker::call(Array<IPoint> &box,
	Array<IPoint> &offset,
	double pageRatio)
{
	callGeneric(box,offset,pageRatio);
}


//
// finds out to which row box rect has to be added in order to minimize the
// covered area taking page ratio into account (the area is the area of the
// smallest rectangle covering all boxes and having the desired width/height
// ratio)
template<class POINT>
int TileToRowsCCPacker::findBestRow(
	Array<RowInfo<POINT> > &row,  // current rows
	int nRows,            // number of rows currently used
	double pageRatio,     // desired page ratio (width / height)
	const POINT &rect)   // box to be added
{
	// Compute the width and height of the current arrangement of boxes
	typename POINT::numberType totalWidth = 0;
	typename POINT::numberType totalHeight = 0;

	int i;
	for(i = 0; i < nRows; ++i)
	{
		const RowInfo<POINT> &r = row[i];
		if (r.m_width > totalWidth)
			totalWidth = r.m_width;

		totalHeight += r.m_maxHeight;
	}

	// For each row, we compute the area we need if rect is added to this row;
	// We store the index of the row minimizing the area in bestRow and return
	// it.
	int bestRow = -1;  // we start with the case of a new row
	totalWidth = max(totalWidth,rect.m_x);
	totalHeight += rect.m_y;

	// note: the area has to take into account the desired page ratio!
	double bestArea = max(pageRatio*totalHeight*totalHeight,
		totalWidth*totalWidth/pageRatio);

	for(i = 0; i < nRows; ++i)
	{
		const RowInfo<POINT> &r = row[i];

		typename POINT::numberType w = r.m_width + rect.m_x;
		typename POINT::numberType h = max(r.m_maxHeight,rect.m_y);

		double area = max(pageRatio*h*h, w*w/pageRatio);

		if (area < bestArea) {
			bestArea = area;
			bestRow = i;
		}
	}

	return bestRow;
}


template<class POINT>
void TileToRowsCCPacker::callGeneric(Array<POINT> &box,
	Array<POINT> &offset,
	double pageRatio)
{
	OGDF_ASSERT(box.size() == offset.size());
	// negative pageRatio makes no sense,
	// pageRatio = 0 will cause division by zero
	OGDF_ASSERT(pageRatio > 0);

	const int n = box.size();
	int nRows = 0;
	Array<RowInfo<POINT> > row(n);

	// sort the box indices according to decreasing height of the
	// corresponding boxes
	Array<int> sortedIndices(n);

	int i;
	for(i = 0; i < n; ++i)
		sortedIndices[i] = i;

	DecrIndexComparer<POINT> comp(box);
	sortedIndices.quicksort(comp);

	// i iterates over all box indices according to decreasing height of
	// the boxes
	for(int iSI = 0; iSI < n; ++iSI)
	{
		int i = sortedIndices[iSI];

		// Find the row which increases the covered area as few as possible.
		// The area measured is the area of the smallest rectangle that covers
		// all boxes and whose width / height ratio is pageRatio
		int bestRow = findBestRow(row,nRows,pageRatio,box[i]);

		// bestRow = -1 indictes that a new row is added
		if (bestRow < 0) {
			struct RowInfo<POINT> &r = row[nRows++];
			r.m_boxes.pushBack(i);
			r.m_maxHeight = box[i].m_y;
			r.m_width = box[i].m_x;

		} else {
			struct RowInfo<POINT> &r = row[bestRow];
			r.m_boxes.pushBack(i);
			r.m_maxHeight = max(r.m_maxHeight,box[i].m_y);
			r.m_width += box[i].m_x;
		}
	}

	// At this moment, we know which box is contained in which row.
	// The following loop sets the required offset of each box
	typename POINT::numberType y = 0;  // sum of the heights of boxes 0,...,i-1
	for(i = 0; i < nRows; ++i)
	{
		const RowInfo<POINT> &r = row[i];

		typename POINT::numberType x = 0;  // sum of the widths of the boxes to the left of box *it

		SListConstIterator<int> it;
		for(it = r.m_boxes.begin(); it.valid(); ++it)
		{
			offset[*it] = POINT(x,y);
			x += box[*it].m_x;
		}

		y += r.m_maxHeight;
	}

	OGDF_ASSERT_IF(dlConsistencyChecks, checkOffsets(box,offset));
}


} // end namespace ogdf
