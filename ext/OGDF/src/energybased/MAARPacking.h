/*
 * $Revision: 2559 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-06 15:04:28 +0200 (Fr, 06. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Implementation of class MAARPacking (used by FMMMLayout).
 *
 * \author Stefan Hachul
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

#ifdef _MSC_VER
#pragma once
#endif

#ifndef OGDF_MAAR_PACKING_H
#define OGDF_MAAR_PACKING_H

#include "PackingRowInfo.h"
#include "Rectangle.h"
#include "Set.h"
#include <ogdf/basic/List.h>
#include "PQueue.h"

namespace ogdf {

	class MAARPacking
	{
		//data structure for packing rectangles within an area of a desired aspect ratio
		//without overlappings; optimization goal: to minimize the used aspect ratio area

	public:

		MAARPacking();     //constructor
		~MAARPacking();    //destructor

		//The rectangles in R are packed using the First Fit tiling stratey (precisely the
		//new down left corner coordinate of each rectangle is calculated and stored in R).
		//The aspect ratio area and the area of the bounding rectangles are calculated,
		//too.
		void pack_rectangles_using_Best_Fit_strategy(List<Rectangle>& R,double
			aspect_ratio, int presort, int
			allow_tipping_over,double&
			aspect_ratio_area,double&
			bounding_rectangles_area);

	private:


		double area_height; //total height of the packing area
		double area_width;  //total width of the packing area


		//Sorts elemets of R with momotonously dedreasing height.
		void presort_rectangles_by_height(List<Rectangle>& R);

		//Sorts elemets of R with momotonously decreasing width.
		void presort_rectangles_by_width(List<Rectangle>& R);

		//Creates a new empty row in P and inserts r into this row (by updating P,
		//row_of_rectangle and total_width_of_row).
		void  B_F_insert_rectangle_in_new_row(Rectangle r,List<PackingRowInfo>& P, List
			<ListIterator<PackingRowInfo> >&
			row_of_rectangle, PQueue&
			total_width_of_row);


		//Finds the Best Fit insert positions of *rect_item and returns the
		//corresp. ListIterator in P or NULL (indicating that a new row has
		//to be created in P); aspect_ratio_area stores the used aspect ratio area of
		//the drawing.
		ListIterator<PackingRowInfo> find_Best_Fit_insert_position(
			ListIterator<Rectangle> rect_item,
			int allow_tipping_over,
			double aspect_ratio,
			double& aspect_ratio_area,
			PQueue& total_width_of_row);


		//Inserts r into the row with corresponding ListIterator B_F_item and updates
		//total_width_of_row.
		void B_F_insert_rectangle(Rectangle r,List<PackingRowInfo>& P,List
			<ListIterator
			<PackingRowInfo> >& row_of_rectangle,ListIterator
			<PackingRowInfo> B_F_item, PQueue& total_width_of_row);


		//The information in P and row_of_rectangle are used to generate the new down left
		//coordinates of the rectangles in R (rectangle_order holds the order in which
		//the rectangles of R have to be processed.
		void  export_new_rectangle_positions(List<PackingRowInfo>& P,List<ListIterator
			<PackingRowInfo> >&
			row_of_rectangle,List<ListIterator
			<Rectangle> >& rectangle_order);

		//Returns the area of the bounding rectangles in R.
		double calculate_bounding_rectangles_area(List<Rectangle>&R);

		//Calculate the aspect ratio area of a rectangle with width w and height h and the
		//given aspect ratio r.
		double calculate_aspect_ratio_area(double w,double h,double r);

		//Returns true if the aspect_ratio_area of the acual packing becomes better, when
		//tipping r over bevore inserting it into the new row. best_area holds the aspect
		//ratio area of the best of the two insertion alternatives.
		bool better_tipp_rectangle_in_new_row(Rectangle r,double aspect_ratio, int
			allow_tipping_over,double& best_area);

		//Returns true if the aspect_ratio_area of the acual packing becomes better, when
		//tipping r over bevore inserting it into the existing row B_F_row. best_area holds
		//the aspect ratio area of the best of the two insertion alternatives.
		bool better_tipp_rectangle_in_this_row(Rectangle r,double aspect_ratio,int
			allow_tipping_over,PackingRowInfo B_F_row,
			double& best_area);

		//Tipps *rect_item over, by newly calculatting its width, height, and old_dlc
		//values (Coordinates of the underlying connected subgraph are not recaculated
		//here!!!). The new values are saved in R[rect_item] and are returned.
		Rectangle tipp_over(ListIterator<Rectangle> rect_item);

	};

}//namespace ogdf
#endif


