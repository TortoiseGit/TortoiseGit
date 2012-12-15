/*
 * $Revision: 2571 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-10 17:25:20 +0200 (Di, 10. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Implementation of class NodeInfo.
 *
 * \author Karsten Klein
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

#include <ogdf/internal/orthogonal/NodeInfo.h>


namespace ogdf {

void NodeInfo::get_data(
	OrthoRep& O,
	GridLayout& L,
	node v,
	RoutingChannel<int>& rc,
	NodeArray<int>& nw,
	NodeArray<int>& nh)
//initializes basic node data
//nodeboxsize, numsedges, mgenpos
//ACHTUNG: odNorth ist 0, soll aber links sein
{
	edge e;
	//first, initialize the node and cage size
	box_x_size = nw[v]; //nw[P.original(v)];//P.widthOrig(P.original(v));
	box_y_size = nh[v]; //nh[P.original(v)];//P.heightOrig(P.original(v));
	//{ fright, fleft, ftop, fbottom}
	m_vdegree = 0;
	//get the generalization edge position on all four sides if existant
	OrthoDir od = odNorth;
	do
	{
		OrthoRep::SideInfoUML sinfo = O.cageInfo(v)->m_side[od];
		if (sinfo.m_adjGen)
		{
			if ((od == odNorth) || (od == odEast)) set_gen_pos(od, sinfo.m_nAttached[0]);
			else set_gen_pos(od, sinfo.m_nAttached[1]);
			set_num_edges(od, sinfo.m_nAttached[0] + 1 + sinfo.m_nAttached[1]);
			m_vdegree += num_s_edges[od];
		}
		else
		{
			set_gen_pos(od, -1);
			set_num_edges(od, sinfo.m_nAttached[0]);
			m_vdegree += num_s_edges[od];
		}
		m_rc[od] = rc(v, od);//sinfo.m_routingChannel;

		od = OrthoRep::nextDir(od);
	} while (od != odNorth);

	//cout<<"input nodedeg: "<<m_vdegree<<"\n"<<flush;

	//compute cage coordinates, use cage corners vertexinfoUML::m_corner
	const OrthoRep::VertexInfoUML* vinfo = O.cageInfo(v);
	adjEntry ae = vinfo->m_corner[0]; e = *ae; //pointing towards north, on left side
	m_ccoord[0] =  L.x(e->source()); //already odDir
	ae = vinfo->m_corner[1]; e = *ae;
	m_ccoord[1] = L.y(e->source()); //already odDir
	ae = vinfo->m_corner[2]; e = *ae;
	m_ccoord[2] = L.x(e->source()); //already odDir
	ae = vinfo->m_corner[3]; e = *ae;
	m_ccoord[3] = L.y(e->source()); //already odDir
	compute_cage_size();
	//fill the in_edges lists for all box_sides
}


int NodeInfo::free_coord(OrthoDir s_main, OrthoDir s_to)
{
	int result = coord(s_main);
	int offset;
	switch (s_main)
	{
	case odNorth: offset = flips(odNorth, s_to)*delta(s_to, odNorth);
	case odSouth: offset = flips(odSouth, s_to)*delta(s_to, odSouth);
	case odWest: offset = flips(odWest, s_to)*delta(s_to, odWest);
	case odEast: offset = -flips(odEast, s_to)*delta(s_to, odEast);
		OGDF_NODEFAULT
	}//switch

	result = result + offset;
	return result;
}//freecoord


ostream& operator<<(ostream& O, const NodeInfo& inf)
{
	O.precision(5);//O.setf(ios::fixed);???????
	O
		<< "\n********************************************\nnodeinfo: \n********************************************\n"
		<< "box left/top/right/bottom: " << inf.coord(OrthoDir(0)) << "/" << inf.coord(OrthoDir(1)) << "/"
		<< inf.coord(OrthoDir(2)) << "/" << inf.coord(OrthoDir(3)) << "\n"
		<< "boxsize:                   " << inf.box_x_size << ":" << inf.box_y_size << "\n"
		<< "cage l/t/r/b:              " << inf.cage_coord(OrthoDir(0)) << "/" << inf.cage_coord(OrthoDir(1)) << "/"
		<< inf.cage_coord(OrthoDir(2)) << "/" << inf.cage_coord(OrthoDir(3)) << "\n"
		<< "gen. pos.:                 " << inf.gen_pos(OrthoDir(0)) << "/"
		<< inf.gen_pos(OrthoDir(1)) << "/"
		<< inf.gen_pos(OrthoDir(2)) << "/" << inf.gen_pos(OrthoDir(3)) << "\n"
		<< "delta l/t/r/b (left/right):" << inf.delta(odNorth, odWest) << ":" << inf.delta(odNorth, odEast) << " / \n"
		<< "                          " << inf.delta(odEast, odNorth) << ":" << inf.delta(odEast, odSouth) << " / \n"
		<< "                          " << inf.delta(odSouth, odEast) << ":" << inf.delta(odSouth, odWest) << " / "
		<< inf.delta(odWest, odSouth) << ":" << inf.delta(odWest, odNorth) << "\n"
		<< "eps l/t/r/b (left/right):  " << inf.eps(odNorth, odWest) << ":" << inf.eps(odNorth, odEast) << " / \n"
		<< "                          " << inf.eps(odEast, odNorth) << ":" << inf.eps(odEast, odSouth) << " / \n"
		<< "                          " << inf.eps(odSouth, odEast) << ":" << inf.eps(odSouth, odWest) << " / "
		<< inf.eps(odWest, odSouth) << ":" << inf.eps(odWest, odNorth) << "\n"
		<< "rc:                         " << inf.rc(OrthoDir(0)) << "/" << inf.rc(OrthoDir(1)) << "/" << inf.rc(OrthoDir(2)) << "/" << inf.rc(OrthoDir(3)) << "\n"
		<< "num edges:                  " << inf.num_edges(OrthoDir(0)) << "/" << inf.num_edges(OrthoDir(1)) << "/" << inf.num_edges(OrthoDir(2))
		<< "/" << inf.num_edges(OrthoDir(3)) << "\n"
		<< "num bendfree edges:         " << inf.num_bend_free(OrthoDir(0)) << "/" << inf.num_bend_free(OrthoDir(1)) << "/" << inf.num_bend_free(OrthoDir(2))
		<< "/" << inf.num_bend_free(OrthoDir(3)) << endl;

	return O;
}

} //end namespace
