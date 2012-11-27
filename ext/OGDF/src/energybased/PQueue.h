/*
 * $Revision: 2559 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-06 15:04:28 +0200 (Fr, 06. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Declaration of class PQueue (priority queue).
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

#ifndef OGDF_PQUEUE_H
#define OGDF_PQUEUE_H

#include <ogdf/basic/List.h>

namespace ogdf {

//Needed for storing entries of the heap.
class HelpRecord
{
public:
	HelpRecord() { }  //constructor
	~HelpRecord(){ } //destructor

	void set_ListIterator(ListIterator<PackingRowInfo>& it) { iterator = it; }
	void set_value (double v) { value = v; }
	ListIterator<PackingRowInfo> get_ListIterator() const { return iterator; }
	double get_value() const { return value; }

private:
	double value;
	ListIterator<PackingRowInfo> iterator;
};

class PQueue
{
	//Helping data structure that is a priority queue (heap) and holds  double values
	//(as comparison values) and iterators of type ListIterator<PackingRowInfo>
	//as contents. It is needed in class MAARPacking for the Best_Fit insert strategy.

public:

	PQueue() { P.clear(); } //constructor
	~PQueue(){ }            //destructor

	//Inserts content with value value into the priority queue and restores the heap.
	void insert(double value, ListIterator<PackingRowInfo> iterator)
	{
		HelpRecord h;
		h.set_value(value);
		h.set_ListIterator(iterator);
		P.pushBack(h);
		//reheap bottom up
		reheap_bottom_up(P.size()-1);
	}

	//Deletes the element with the minimum value from the queue and restores
	//the heap.
	void del_min()
	{
		if(P.size() < 1)
			cout<<"Error PQueue:: del_min() ; Heap is empty"<<endl;
		else
		{
			//last element becomes first element
			P.popFront();
			if(!P.empty())
			{
				P.pushFront(P.back());
				P.popBack();
				//reheap top down
				reheap_top_down(0);
			}
		}
	}

	//Returns the content with the minimum value.
	ListIterator<PackingRowInfo> find_min()
	{
		OGDF_ASSERT(P.size() >= 1);
		//if(P.size() < 1)
		//  cout<<"Error PQueue:: find_min() ; Heap is empty"<<endl;
		//else
		return P.front().get_ListIterator();
	}

private:
	List<HelpRecord> P;//the priority queue;

	//Restores the heap property in P starting from position i bottom up.
	void reheap_bottom_up(int i)
	{
		int parent = (i-1)/2;

		if((i != 0) && ((*P.get(parent)).get_value() > (*P.get(i)).get_value()))
		{
			exchange(i,parent);
			reheap_bottom_up(parent);
		}
	}

	//Restores the heap property in P starting from position i top down.
	void reheap_top_down(int i)
	{
		int smallest = i;
		int l = 2*i+1;
		int r = 2*i+2;

		if((l <= P.size()-1) && ((*P.get(l)).get_value() < (*P.get(i)).get_value()))
			smallest = l;
		else
			smallest = i;
		if((r <= P.size()-1) && ((*P.get(r)).get_value() < (*P.get(smallest)).get_value()))
			smallest = r;
		if(smallest != i)//exchange and recursion
		{
			exchange(i,smallest);
			reheap_top_down(smallest);
		}
	}

	//Exchanges heap entries at positions i and j.
	void exchange(int i, int j)
	{
		HelpRecord h = *P.get(i);
		*P.get(i) = *P.get(j);
		*P.get(j) = h;
	}
};

}//namespace ogdf
#endif




