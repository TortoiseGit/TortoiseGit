/*
 * $Revision: 2566 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-07 23:10:08 +0200 (Sa, 07. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Edge routing and node placement implementation.
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


//TODO: handle multiedges in a way that forbids assignment of different sides


#include <ogdf/orthogonal/EdgeRouter.h>
#include <string.h>
#include <limits.h>


namespace ogdf {

#define SETMULTIMINDELTA //set multidelta,


const double machineeps = 1.0e-10;
const int m_init = -1234567; //just to check initialization


//************************************************************************************************
//edgerouter places original node boxes in preassigned cages, computes a number of
//bendfree edges minimizing placement and routes edges, thereby introducing bends, to
//achieve a correct layout
//************************************************************************************************

// routing channel and number of adjacent edges / generalization is supplied by previous
// compaction step in class routingchannel
// class NodeInfo holds the specific information for a single replaced node ( adjEntry != 0)


//constructor
EdgeRouter::EdgeRouter(
	PlanRep& pru,
	OrthoRep& H,
	GridLayoutMapped& L,
	CombinatorialEmbedding& E,
	RoutingChannel<int>& rou,
	MinimumEdgeDistances<int>& mid,
	NodeArray<int>& nodewidth,
	NodeArray<int>& nodeheight)
:
	m_prup(&pru),
	m_layoutp(&L),
	m_orp(&H),
	m_comb(&E),
	m_rc(&rou),
	m_med(&mid),
	m_nodewidth(&nodewidth),
	m_nodeheight(&nodeheight)
{
	init(pru, rou);
}//EdgeRouter


//initializes the members
void EdgeRouter::init(
	PlanRep& pru,
	RoutingChannel<int>& rou,
	bool align)
{
	//saves cage position left lower
	m_newx.init(pru, m_init);
	m_newy.init(pru, m_init);
	//saves glue and connection point positions
	m_agp_x.init(pru, m_init);
	m_agp_y.init(pru, m_init);
	m_acp_x.init(pru, m_init);
	m_acp_y.init(pru, m_init);
	m_abends.init(pru, bend_free);
	m_oppositeBendType.init(pru, bend_free);

	m_minDelta = false;
#ifdef SETMULTIMINDELTA
	m_minDelta = true;
#endif

	m_mergerSon.init(pru, false);
	m_mergeDir.init(pru, odNorth);
	m_align = align;

	m_fixed.init(pru, false);
	m_processStatus.init(pru, unprocessed);
	m_cage_point.init(pru);

	m_sep = rou.separation();
	m_overh = rou.overhang();
	Cconst = double(m_overh)/double(m_sep);
}//init


//call - function: placing nodes and routing edges
void EdgeRouter::call()
{
	//if no graph is given, you should stop or return 0,0
	OGDF_ASSERT( (m_prup != 0) && (m_layoutp != 0) && (m_orp != 0) && (m_comb != 0) && (m_nodewidth != 0));
	call(*m_prup, *m_orp, *m_layoutp, *m_comb, *m_rc, *m_med, *m_nodewidth, *m_nodeheight);
}//call


void EdgeRouter::call(
	PlanRep& pru,
	OrthoRep& H,
	GridLayoutMapped& L,
	CombinatorialEmbedding& E,
	RoutingChannel<int>& rou,
	MinimumEdgeDistances<int>& mid,
	NodeArray<int>& nodewidth,
	NodeArray<int>& nodeheight,
	bool align)
{
	String msg;
	OGDF_ASSERT(H.check(msg));

	init(pru, rou, align);
	m_prup = &pru;
	m_layoutp = &L;
	m_orp = &H;
	m_comb = &E;
	m_rc = &rou;
	m_med = &mid;
	m_nodewidth = &nodewidth;
	m_nodeheight = &nodeheight;
	//just input some stuff
	infos.init(pru);

	node v;
	int mysep = m_sep;
	//set specific delta values automatically for all nodes
	//preliminary: set to minimum value perimeter / degree of all nodes
	//seems to cause problems with compaction
	if (m_minDelta)
		forall_nodes(v, pru)
		{
			if ( (pru.expandAdj(v) != 0) &&
				(pru.typeOf(v) != Graph::generalizationMerger))
			{
			int perimeter = 2*nodewidth[v] + 2*nodeheight[v];
			OrthoDir debod = odNorth;
			int vdeg = 0;
			do {
				OrthoRep::SideInfoUML sinfo = m_orp->cageInfo(v)->m_side[debod];
				if (sinfo.m_adjGen)
					vdeg += (sinfo.m_nAttached[0] + 1 + sinfo.m_nAttached[1]);
				else
					vdeg += sinfo.m_nAttached[0];
				debod = OrthoRep::nextDir(debod);
				} while (debod != odNorth);
				if (vdeg != 0) mysep = min(mysep, int(floor((double)perimeter / vdeg)));
			}//if expanded
		}//forallnodes

	forall_nodes(v, pru)
	{
		if ( (pru.expandAdj(v) != 0) &&
			 (pru.typeOf(v) != Graph::generalizationMerger)) //==Expander) )
		//adjEntry != nil, this cage node is a copy(original node)
		{
			OGDF_ASSERT(pru.widthOrig(pru.original(v)) > 0.0)
			initialize_node_info(v, mysep); //delta, epsilon, cagesize, boxsize
		}
	}//forallnodes

	//the rerouting **********************************************************
	//simple rerouting version: maximize the number of bend free edges in the
	//placement step, then try to minimize bends by changing attachment sides
	//in the rerouting step

	//work on all expanded nodes  / positioning info is kept in class NodeInfo
	//and in layout previous compaction step guarantees routing channel

	//for every hor. edge e, lowe(e) denotes the biggest yvalue possible for
	//the lower border of target(e)'s cage if e is routed bendfree (depending on pred edges)
	//uppe(e) denotes the minimum yvalue for the upper border of v's cage if ...
	lowe.init(*m_prup, m_init);
	uppe.init(*m_prup, m_init);
	alowe.init(*m_prup, m_init);
	auppe.init(*m_prup, m_init);
	//for each and every vert. edge e, lefte(e) denotes the biggest xvalue possible
	//for the left border of v's cage if e is routed bendfree (depending on pred edges)
	//righte(e) denotes the minimum xvalue for the right border of v's cage ...
	lefte.init(*m_prup, m_init);
	righte.init(*m_prup, m_init);
	alefte.init(*m_prup, m_init);
	arighte.init(*m_prup, m_init);

	//compute the lowe / uppe / lefte / righte values for every adjacent edge
	//if generalization exists the node position is already fixed
	node l_v;

	//*********************************************************************
	//compute LOWER / UPPER / LEFTER / RIGHTER border values
	//*********************************************************************

	//forall expanded nodes
	forall_nodes(l_v, pru)
	{
		if ((pru.expandAdj(l_v) != 0) && (pru.typeOf(l_v) != Graph::generalizationMerger) )//check if replaced
		{
			NodeInfo& inf = infos[l_v];

			//edges to the left side, pointing towards cage
			const List<edge>& left_in_edges = inf.inList(odNorth);
			ListConstIterator<edge> itE;
			//LEFT EDGES
			int pos_e = 1;
			//for all edges incident to cage, we compute lower, upper, lefter, righter values from the paper
			for (itE = left_in_edges.begin(); itE.valid(); ++itE)
			{
				edge inedge = *itE;

				int l_seps; //adjust multiples of delta for lower values
				int u_seps; // -"- upper
				int remaining_num = left_in_edges.size() - pos_e; //incident edges lying above ine

				l_seps = inf.delta(odNorth, odWest)*(pos_e-1);
				u_seps = inf.delta(odNorth, odEast)*remaining_num;

				lowe[inedge] = L.y(inedge->target()) - l_seps - inf.eps(odNorth, odWest);
				alowe[outEntry(inf, odNorth,pos_e-1)] = L.y(inedge->target()) - l_seps - inf.eps(odNorth, odWest);
				uppe[inedge] = L.y(inedge->target()) + u_seps + inf.eps(odNorth, odEast);
				auppe[outEntry(inf, odNorth, pos_e -1)] = L.y(inedge->target()) + u_seps + inf.eps(odNorth, odEast);

				arighte[outEntry(inf, odNorth, pos_e -1)] = righte[inedge] = 0; //unused for horizontal edges in twostep simple rerouting
				alefte[outEntry(inf, odNorth, pos_e -1)] = lefte[inedge] = 0;  //maybe initialize to -1
				pos_e++;
			}//for
			//RIGHT EDGES
			//edges to the right side, pointing towards cage , check bottom -> top!!!!!!!!!!
			const List<edge>& right_in_edges = inf.inList(odSouth);
			pos_e = 1;
			for (itE = right_in_edges.begin(); itE.valid(); ++itE)
			{
				edge inedge = *itE;

				int l_seps; //adjust multiples of delta to generalization/ no generalization for lower
				int u_seps; // -"- upper
				int remaining_num = right_in_edges.size() - pos_e; //incident edges lying above ine

				l_seps = inf.delta(odSouth, odWest)*(pos_e-1);
				u_seps = inf.delta(odSouth, odEast)*remaining_num;

				lowe[inedge] = L.y(inedge->target()) - l_seps - inf.eps(odSouth, odWest);
				alowe[outEntry(inf, odSouth, pos_e-1)] = L.y(inedge->target()) - l_seps - inf.eps(odSouth, odWest);
				uppe[inedge] = L.y(inedge->target()) + u_seps + inf.eps(odSouth, odEast);
				auppe[outEntry(inf, odSouth, pos_e-1)] = L.y(inedge->target()) + u_seps + inf.eps(odSouth, odEast);
				//unused for horizontal edges in twostep simple rerouting
				arighte[outEntry(inf, odSouth, pos_e-1)] = righte[inedge] = 0;
				alefte[outEntry(inf, odSouth, pos_e-1)] = lefte[inedge] = 0;  //maybe initialize to -1
				pos_e++;
			}//for

			//TOP EDGES
			//edges at the top side, pointing towards cage , check left -> right!!!!!!!!!!
			const List<edge>& top_in_edges = inf.inList(odEast); //debug!!!!!!!!!!!!
			pos_e = 1;
			for (itE = top_in_edges.begin(); itE.valid(); ++itE)
			{
				edge inedge = *itE;

				int l_seps; //adjust multiples of delta for left
				int r_seps; // -"- right
				int remaining_num = top_in_edges.size() - pos_e; //incident edges lying above ine

				l_seps = inf.delta(odEast, odNorth)*(pos_e-1);
				r_seps = inf.delta(odEast, odSouth)*remaining_num;

				lefte[inedge] = L.x(inedge->target()) - l_seps - inf.eps(odEast, odNorth); //eps are the same w/o gene
				alefte[outEntry(inf, odEast, pos_e - 1)] = L.x(inedge->target()) - l_seps - inf.eps(odEast, odNorth);
				righte[inedge] = L.x(inedge->target()) + r_seps + inf.eps(odEast, odSouth);
				arighte[outEntry(inf,odEast, pos_e - 1)] = L.x(inedge->target()) + r_seps + inf.eps(odEast, odSouth);

				alowe[outEntry(inf,odEast, pos_e - 1)] = lowe[inedge] = 0; //unused for vertical edges in twostep simple rerouting
				auppe[outEntry(inf,odEast, pos_e - 1)] = uppe[inedge] = 0;  //maybe initialize to -1
				pos_e++;
			}//for
			//edges at the bottom side, pointing towards cage , check left -> right!!!!!!!!!!
			const List<edge>& bottom_in_edges = inf.inList(odWest); //debug!!!!!!!!!!!!
			pos_e = 1;
			for (itE = bottom_in_edges.begin(); itE.valid(); ++itE)
			{
				edge inedge = *itE;

				int l_seps; //adjust multiples of delta for left
				int r_seps; // -"- right
				int remaining_num = bottom_in_edges.size() - pos_e; //incident edges lying above ine

				l_seps = inf.delta(odWest, odNorth)*(pos_e-1);
				r_seps = inf.delta(odWest, odSouth)*remaining_num;

				lefte[inedge] = L.x(inedge->target()) - l_seps - inf.eps(odWest, odNorth); //eps are the same w/o gene
				alefte[outEntry(inf, odWest, pos_e-1)] = L.x(inedge->target()) - l_seps - inf.eps(odWest, odNorth);
				righte[inedge] = L.x(inedge->target()) + r_seps + inf.eps(odWest, odSouth);
				arighte[outEntry(inf, odWest, pos_e-1)] = L.x(inedge->target()) + r_seps + inf.eps(odWest, odSouth);

				alowe[outEntry(inf, odWest, pos_e-1)] = lowe[inedge] = 0; //unused for vertical edges in twostep simple rerouting
				auppe[outEntry(inf, odWest, pos_e-1)] = uppe[inedge] = 0;  //maybe initialize to -1
				pos_e++;
			}//for
		}//if replaced
	}//forallnodes

	//***********************************************************************************
	//now for all edges pointing towards cages representing nodes without generalization,
	//we defined lowe/uppe values for horizontal and lefte/righte values for vertical edges
	//***********************************************************************************

	forall_nodes(v, pru)
	{
		if ( (pru.expandAdj(v) != 0) && (pru.typeOf(v) != Graph::generalizationMerger) )//== Graph::highDegreeExpander) ) //expanded high degree
		{
			compute_place(v, infos[v]/*, m_sep, m_overh*/);
			//***************************************************************************
			//forall expanded nodes we computed a box placement and a preliminary routing
			//now we can reroute some edges to avoid unnecessary bends (won't be optimal)
			//simple approach: implement only local decision at corner between two
			//neighboured nodebox sides
			//***************************************************************************
			//classify_edges(v, infos[v]); //E sets from paper, reroutable
			compute_routing(v); //maybe store result in structure and apply later
		}//if expanded
	}//forall_nodes

	////try to place deg1 nodes
	//forall_nodes(v, pru)
	//   {
	//	//there is an omission of placement here leading to errors
	//	if (false) //preliminary until umlex4 error checked
	//       {
	//		if ( (pru.expandAdj(v) != 0) && (pru.typeOf(v) != Graph::generalizationMerger) )
	//		{
	//			//****************************************
	//			//Re - Place degree one nodes if possible, to neighbour cage
	//			//mag sein, dass firstadj ein zeiger sein muss
	//			adjEntry fa = infos[v].firstAdj();
	//			//step over possibly inserted bends
	//			while (pru.typeOf(fa->twinNode()) == Graph::dummy)
	//				fa = fa->faceCycleSucc();
	//			if ((pru.typeOf(fa->twinNode()) != Graph::highDegreeExpander) &&
	//				(pru.typeOf(fa->twinNode()) != Graph::lowDegreeExpander) )
	//			{
	//				//place(v, m_sep, m_overh);
	//				continue;
	//			}//if expander
	//			//get neighbour node
	//			node v1 = fa->twinNode();
	//			//get attachment side
	//			OrthoDir od = OrthoRep::prevDir(H.direction(fa));
	//
	//			node expandNode = pru.expandedNode(v1);
	//			//v is neighbour of expanded node and may fit in its cage
	//			//check if deg 1 can be placed near to node
	//			//problem: wenn nicht geflippt wird, ist meistens auch kein Platz auf der Seite
	//			if ( (infos[v].vDegree() == 1) && //can be placed freely
	//				//no edges to cross on neighbours side
	//				(infos[expandNode].num_edges(od) + infos[expandNode].flips(OrthoRep::prevDir(od), od) +
	//					infos[expandNode].flips(OrthoRep::nextDir(od), od) == 1) &&
	//				//enough space to host trabant
	//				(infos[expandNode].coordDistance(od) > infos[v].nodeSize(OrthoRep::prevDir(od)) + m_sep) &&
	//				(infos[expandNode].cageSize(od) > infos[v].nodeSize(od)) &&
	//				//dumm gelaufen: der Knoten muss auch kleiner als der Nachbar sein, damit er nicht in
	//				//abgeknickte Kanten der Seite laeuft, deshalb hier spaeter deren Position testen, Bed. weg
	//				//das ist etwas doppelt, damit man spaeter nur diese entfernen muss, die oben bleibt
	//				(infos[expandNode].nodeSize(od) >= infos[v].nodeSize(od))
	//			)
	//			{
	//				//find new place, v must be cage node with out edge attached
	//				int npos, spos, epos, wpos, vxpos, vypos;
	//				switch (od)
	//	            {
	//					case odNorth:
	//						npos = infos[expandNode].coord(odNorth) - m_sep - infos[v].node_xsize();
	//						spos = infos[expandNode].coord(odNorth) - m_sep;
	//						wpos = infos[expandNode].cageCoord(odWest)
	//                                        + int(floor(infos[expandNode].cageSize(odNorth)/2.0)
	//                                        - floor(infos[v].nodeSize(odNorth)/2.0));
	//						epos = wpos + infos[v].nodeSize(odNorth);
	//						vxpos = spos;
	//						vypos = wpos + int(floor(infos[v].nodeSize(odNorth)/2.0));
	//						break;
	//					case odSouth:
	//						npos = infos[expandNode].coord(odSouth) + m_sep;
	//						spos = infos[expandNode].coord(odSouth) + m_sep + infos[v].node_xsize();
	//						wpos = infos[expandNode].cageCoord(odWest)
	//                                        + int(floor(infos[expandNode].cageSize(odNorth)/2.0)
	//                                        - floor(infos[v].nodeSize(odNorth)/2.0));
	//						epos = wpos + infos[v].nodeSize(odNorth);
	//						vxpos = npos;
	//						vypos = wpos + int(floor(infos[v].nodeSize(odNorth)/2.0));
	//						break;
	//	                case odEast:
	//	                    npos = infos[expandNode].cageCoord(odNorth)
	//                              + int(floor(infos[expandNode].cageSize(odEast)/2.0)
	//                              - floor(infos[v].nodeSize(odEast)/2.0));
	//						spos = npos + infos[v].cageSize(odEast);
	//						wpos = infos[expandNode].coord(odEast) + m_sep;
	//						epos = wpos + infos[v].nodeSize(odEast);
	//						vypos = wpos;
	//						vxpos = npos + int(floor(infos[v].nodeSize(odEast)/2.0));
	//						break;
	//	                case odWest:
	//			            npos = infos[expandNode].cageCoord(odNorth)
	//                              + int(floor(infos[expandNode].cageSize(odEast)/2.0)
	//                              - floor(infos[v].nodeSize(odEast)/2.0));
	//						spos = npos + infos[v].cageSize(odEast);
	//						epos = infos[expandNode].coord(odWest) - m_sep;
	//						wpos = epos - infos[v].nodeSize(odNorth);
	//						vypos = epos;
	//						vxpos = npos + int(floor(infos[v].nodeSize(odEast)/2.0));
	//						break;
	//                }//switch
	//				infos[v].set_coord(odNorth, npos);
	//				infos[v].set_coord(odSouth, spos);
	//				infos[v].set_coord(odWest, wpos);
	//				infos[v].set_coord(odEast, epos);
	//				infos[v].setCageCoord(odNorth, npos);
	//				infos[v].setCageCoord(odSouth, spos);
	//				infos[v].setCageCoord(odWest, wpos);
	//				infos[v].setCageCoord(odEast, epos);
	//				//set v coordinates
	//				m_layoutp->x(v) = vxpos;
	//				m_layoutp->y(v) = vypos;
	//				m_layoutp->x(v1) = vxpos;
	//				m_layoutp->y(v1) = vypos;
	//				//set corner coordinates after placing
	//				set_corners(v);
	//				m_processStatus[v] = processed;
	//				m_processStatus[v1] = used;
	//				//hier muss man auch die Kantenendpunkte setzen, sonst gibt es einen Fehler
	//			}//if
	//		}//if degreeexpander
	//       }//debug stop
	//}//forall_nodes

	forall_nodes(v, pru)
	{
		if ( (pru.expandAdj(v) != 0) && (pru.typeOf(v) != Graph::generalizationMerger) )
		{
			if (!(m_processStatus[v] == processed))
				place(v);
		}//if expanded
	}//forallnodes

	setDistances();

	//  PathFinder pf(pru, H, L, *m_comb);
	//  int routable = pf.analyse();
	//if (routable > 0) pf.route(false);

	OGDF_ASSERT(H.check(msg));
}//call


//simple local placement decision based on incident edges attachment positions
//size of input original (replaced) node and original box

void EdgeRouter::compute_gen_glue_points_y(node v)
//compute preliminary glue point positions based on placement
//and generalizations for horizontal edges and set bend type accordingly
{
	OGDF_ASSERT(infos[v].has_gen(odNorth) || infos[v].has_gen(odSouth));
	int ybase = 0;
	int gen_y = infos[v].coord(odWest) + int(floor((double)(infos[v].node_ysize())/2)); //in the middle
	//y coordinates
	//check for left/right generalization

	//NORTH SIDE *************************************************
	ListIterator<edge> l_it = infos[v].inList(odNorth).begin();
	//NORTH GENERATOR ******************************************
	if (infos[v].has_gen(odNorth))//gen at left side
	{
		int pos = infos[v].gen_pos(odNorth)-1; //compare edge position to generalization position
		if (pos > -1) l_it = infos[v].inList(odNorth).get(pos);//muss unter gen sein, -2???
		else {l_it = 0; pos = 0;}
		bool firstcheck = true;
		bool lastcheck = true;
		//classify edges
		//***************************************************
		//assign gp value for edges underneath generalization
		ybase = gen_y - infos[v].delta(odNorth, odWest);
		//bendfree edges underneath
		while (l_it.valid() &&
			(pos*infos[v].delta(odNorth, odWest) + infos[v].eps(odNorth,odWest) <=
			(cp_y(outEntry(infos[v], odNorth, pos)) - infos[v].coord(odWest)) )) //bendfree edges cage_coord??!!!
		{
			m_agp_y[outEntry(infos[v], odNorth, pos)] = cp_y(outEntry(infos[v], odNorth, pos));

			if (firstcheck) {firstcheck = false; infos[v].set_l_upper(cp_y(outEntry(infos[v], odNorth, pos)));}
			lastcheck = false;
			infos[v].set_l_lower(cp_y(outEntry(infos[v], odNorth, pos)));

			m_abends[outEntry(infos[v], odNorth, pos)] = bend_free;
			ybase = cp_y(outEntry(infos[v], odNorth, pos)) - infos[v].delta(odNorth, odWest);
			l_it--;
			pos--;
			infos[v].nbf(odNorth)++;
		}//while

		//still some lower edges to bend
		while (l_it.valid())
		{
			m_agp_y[outEntry(infos[v], odNorth, pos)] = infos[v].coord(odWest)
				+ infos[v].eps(odNorth, odWest)
				+ pos*infos[v].delta(odNorth, odWest);
			if (cp_y(outEntry(infos[v], odNorth, pos)) < infos[v].coord(odWest) - m_sep) //paper E^
			{  m_abends[outEntry(infos[v], odNorth, pos)] = prob_b1l; infos[v].inc_E_hook(odNorth, odWest);}//numr??
			else {m_abends[outEntry(infos[v], odNorth, pos)] = prob_b2l; infos[v].inc_E(odNorth, odWest);}

			ybase = ybase - infos[v].delta(odNorth, odWest);
			pos--;
			l_it--;
		}

		//*******************************************************
		//assign gp value for generalization
		ybase = gen_y; //check == y(current edge)???
		l_it = infos[v].inList(odNorth).get(infos[v].gen_pos(odNorth));

		infos[v].nbf(odNorth)++;
		m_agp_y[outEntry(infos[v], odNorth, infos[v].gen_pos(odNorth))] = ybase;
		m_abends[outEntry(infos[v], odNorth, infos[v].gen_pos(odNorth))] = bend_free;

		if (lastcheck) infos[v].set_l_lower(ybase);
		infos[v].set_l_upper(ybase);

		//*******************************************************
		//assign gp value for bendfree edges above generalization
		l_it++;
		pos = infos[v].gen_pos(odNorth) + 1;
		while (l_it.valid() &&
			((infos[v].inList(odNorth).size() - 1 - pos)*infos[v].delta(odNorth, odEast)
			+ infos[v].eps(odNorth,odEast) <=
			(infos[v].coord(odEast) - cp_y(outEntry(infos[v], odNorth, pos))) ))
		{
			m_abends[outEntry(infos[v], odNorth, pos)] = bend_free;
			ybase = cp_y(outEntry(infos[v], odNorth, pos));//+= infos[v].delta(odNorth, odEast);

			m_agp_y[outEntry(infos[v], odNorth, pos)] = ybase;

			infos[v].set_l_upper(ybase);
			l_it++; pos++;
			infos[v].nbf(odNorth)++;
		}

		//*******************************************************
		//assign gp value for bend edges on top of generalization
		int bendnum = infos[v].inList(odNorth).size() - pos;
		while (l_it.valid())
		{
			//check for single/2 bend
			//ybase += infos[v].delta(odNorth, odEast); //there is a generalization
			ybase = infos[v].l_upper_unbend() +
				(pos  + 1 + bendnum - infos[v].inList(odNorth).size())*infos[v].delta(odNorth, odEast);

			if (m_acp_y[outEntry(infos[v], odNorth, pos)] < infos[v].coord(odEast) + m_sep)
			{
				m_abends[outEntry(infos[v],odNorth, pos)] = prob_b2r; infos[v].inc_E(odNorth, odEast);
			}
			else {
				m_abends[outEntry(infos[v], odNorth, pos)] = prob_b1r; infos[v].inc_E_hook(odNorth, odEast);
			}
			m_agp_y[outEntry(infos[v], odNorth, pos)] = ybase;

			l_it++;
			pos++;
		}//while valid
	}//if left gen

	//NO LEFT GENERATOR ****************************************************
	else
	{
		int pos = 0;
		OGDF_ASSERT(infos[v].has_gen(odSouth));//obs
		//classify edges
		//******************
		//edges bending down
		//******************
		while (l_it.valid() && ( infos[v].coord(odWest)  >
				 (cp_y(outEntry(infos[v], odNorth, pos)) - pos*infos[v].delta(odNorth, odWest) - infos[v].eps(odNorth,odWest)) ))
		{
			if (cp_y(outEntry(infos[v], odNorth, pos)) > infos[v].coord(odWest) - m_sep)//must be doublebend
			{
				m_abends[outEntry(infos[v], odNorth, pos)] = bend_2left;
				infos[v].inc_E(odNorth, odWest);
			}
			else //may be singlebend
			{
				m_abends[outEntry(infos[v], odNorth, pos)] = prob_b1l;
				infos[v].inc_E_hook(odNorth, odWest);
			}

			m_agp_y[outEntry(infos[v], odNorth, pos)] = infos[v].coord(odWest)
				+ infos[v].eps(odNorth, odWest)
				+ pos*infos[v].delta(odNorth, odWest);
			l_it++;
			pos++;
		}//while
		//*********************
		//bendfree edges
		//*********************
		bool check = true;
		while (l_it.valid() && ( infos[v].coord(odEast)  >= (cp_y(outEntry(infos[v], odNorth, pos))
							 + (infos[v].inList(odNorth).size() - 1 - pos)*infos[v].delta(odNorth, odWest)
							 + infos[v].eps(odNorth,odWest)) ))
		{
			if (check) infos[v].set_l_lower(cp_y(outEntry(infos[v], odNorth, pos)));
			infos[v].set_l_upper(cp_y(outEntry(infos[v], odNorth, pos)));
			check = false;
			m_abends[outEntry(infos[v], odNorth, pos)] = bend_free;
			infos[v].nbf(odNorth)++;
			m_agp_y[outEntry(infos[v], odNorth, pos)] =  cp_y(outEntry(infos[v], odNorth, pos));//m_acp_y[outEntry(infos[v], odNorth, pos)];
			l_it++;
			pos++;
		}
		//*********************
		//edges bending upwards
		//*********************
		while (l_it.valid()) //&&???!!!
		{
			if (cp_y(outEntry(infos[v], odNorth, pos)) <= infos[v].coord(odEast) + m_sep)
			{
				m_abends[outEntry(infos[v], odNorth, pos)] = bend_2right;
				infos[v].inc_E(odNorth, odEast);
			}
			else
			{
				m_abends[outEntry(infos[v], odNorth, pos)] = prob_b1r; infos[v].inc_E_hook(odNorth, odEast);
			}//else

			m_agp_y[outEntry(infos[v], odNorth, pos)] = infos[v].coord(odEast)
				- infos[v].eps(odNorth, odEast)
				- (infos[v].inList(odNorth).size() - 1 - pos)*infos[v].delta(odNorth, odEast);
			l_it++;
			pos++;
		}//while

	}//else left gen

	//RIGHT SIDE **************************************************************
	//RIGHT GENERATOR ***********************************************
	if (infos[v].has_gen(odSouth))
	{
		//left copy
		int pos = infos[v].gen_pos(odSouth)-1; //compare edge position to generalization position
		if (pos > -1)
			l_it = infos[v].inList(odSouth).get(pos);
		else l_it = 0;
		//classify edges
		//***************************************************
		//assign gp value for edges underneath generalization
		ybase = gen_y - infos[v].delta(odSouth, odWest);

		//bendfree edges underneath
		bool check = false;
		bool lastcheck = true;
		while (l_it.valid() &&
			(pos*infos[v].delta(odSouth, odWest) + infos[v].eps(odSouth,odWest) <=
			(cp_y(outEntry(infos[v], odSouth, pos)) - infos[v].coord(odWest)) ))
		{
			m_agp_y[outEntry(infos[v], odSouth, pos)] = cp_y(outEntry(infos[v], odSouth, pos));

			lastcheck = false;
			infos[v].set_r_lower(m_agp_y[outEntry(infos[v], odSouth, pos)]);
			if (!check) {infos[v].set_r_upper(m_agp_y[outEntry(infos[v], odSouth, pos)]); check = true;}
			m_abends[outEntry(infos[v], odSouth, pos)] = bend_free;
			ybase = cp_y(outEntry(infos[v], odSouth, pos)) - infos[v].delta(odSouth, odWest);
			l_it--;
			pos--;
			infos[v].nbf(odSouth)++;
		}//while
		while (l_it.valid()) //still some lower edges to bend, ycoord+eps+delta
		{
			m_agp_y[outEntry(infos[v], odSouth, pos)] = ybase;
			if (cp_y(outEntry(infos[v], odSouth, pos)) < infos[v].coord(odWest) - m_sep)
			{
				m_abends[outEntry(infos[v], odSouth, pos)] = prob_b1r;
				infos[v].inc_E_hook(odSouth, odWest);
			}
			else {
				m_abends[outEntry(infos[v], odSouth, pos)] = prob_b2r;
				infos[v].inc_E(odSouth, odWest);
			}
			ybase = ybase - infos[v].delta(odSouth, odWest);
			l_it--;
			pos--;
		}
		//*******************************************************
		//assign gp value for generalization
		ybase = gen_y; //check == y(current edge)???
		l_it = infos[v].inList(odSouth).get(infos[v].gen_pos(odSouth));
		infos[v].nbf(odSouth)++;
		m_agp_y[outEntry(infos[v], odSouth, infos[v].gen_pos(odSouth))] = ybase;
		m_abends[outEntry(infos[v], odSouth, infos[v].gen_pos(odSouth))] = bend_free;
		if (lastcheck)
		{
			infos[v].set_r_lower(m_agp_y[outEntry(infos[v], odSouth, infos[v].gen_pos(odSouth))]);
			lastcheck = false;
		}
		infos[v].set_r_upper(ybase);
		//*******************************************************
		//assign gp value for bendfree edges above generalization
		l_it++;
		pos = infos[v].gen_pos(odSouth) + 1;
		while (l_it.valid() &&
			((infos[v].inList(odSouth).size() - 1 - pos)*infos[v].delta(odSouth, odEast) + infos[v].eps(odSouth,odEast) <=
			(infos[v].coord(odEast) - cp_y(outEntry(infos[v], odSouth, pos))) ))
		{
			m_abends[outEntry(infos[v], odSouth, pos)] = bend_free;
			ybase = cp_y(outEntry(infos[v], odSouth, pos));//+= infos[v].delta(odNorth, odEast);
			m_agp_y[outEntry(infos[v], odSouth, pos)] = ybase;
			infos[v].set_r_upper(ybase);
			infos[v].nbf(odSouth)++;
			l_it++; pos++;
		}
		//*******************************************************
		//assign gp value for bend edges on top of generalization
		while (l_it.valid())
		{
			//check for single/2 bend
			if (cp_y(outEntry(infos[v], odSouth, pos)) > infos[v].coord(odEast) + m_sep)
			{
				m_abends[outEntry(infos[v], odSouth, pos)] = prob_b1l;
				infos[v].inc_E_hook(odSouth, odEast);
			}
			else {
				m_abends[outEntry(infos[v], odSouth, pos)] = prob_b2l;
				infos[v].inc_E(odSouth, odEast);
			}

			ybase += infos[v].delta(odSouth, odEast); //there is a generalization
			m_agp_y[outEntry(infos[v], odSouth, pos)] = ybase;
			l_it++;
			pos++;
		}//while valid
	}//if rightgen
	//NO RIGHT GENERATOR ****************************************************
	else
	{
		int pos = 0;
		l_it = infos[v].inList(odSouth).begin();
		//classify edges
		//******************
		//edges bending down
		//******************
		while (l_it.valid() && ( infos[v].coord(odWest)  >
			 (cp_y(outEntry(infos[v], odSouth, pos)) - pos*infos[v].delta(odSouth, odWest) - infos[v].eps(odSouth,odWest)) ))
		{
			if (cp_y(outEntry(infos[v], odSouth, pos)) > infos[v].coord(odWest) - m_sep)//must be intbend
			{
				m_abends[outEntry(infos[v], odSouth, pos)] = bend_2right;
				infos[v].inc_E(odSouth, odWest); }
			else //may be singlebend
			{
				m_abends[outEntry(infos[v], odSouth, pos)] = prob_b1r;
				infos[v].inc_E_hook(odSouth, odWest);
			}
			//ab unterem Rand, oder ab gen, teile Abstand Anzahl Kanten??!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
			m_agp_y[outEntry(infos[v], odSouth, pos)] =
				infos[v].coord(odWest)
				+ infos[v].eps(odSouth, odWest)
				+ pos*infos[v].delta(odSouth, odWest);
			l_it++;
			pos++;
		}//while
		//*********************
		//bendfree edges
		//*********************
		bool check = false;
		while (l_it.valid() && ( infos[v].coord(odEast)  >=
			(cp_y(outEntry(infos[v], odSouth, pos))
			+ (infos[v].inList(odSouth).size() - 1 - pos)*infos[v].delta(odSouth, odWest)
			+ infos[v].eps(odSouth,odWest)) ))
		{
			if (!check) {
				infos[v].set_r_lower(cp_y(outEntry(infos[v], odSouth, pos)));
				check = true;
			}
			infos[v].set_r_upper(cp_y(outEntry(infos[v], odSouth, pos)));
			m_abends[outEntry(infos[v], odSouth,pos)] = bend_free;
			infos[v].nbf(odSouth)++;
			m_agp_y[outEntry(infos[v], odSouth, pos)] = cp_y(outEntry(infos[v], odSouth, pos));
			l_it++;
			pos++;
		}//while
		//*********************
		//edges bending upwards
		//*********************
		while (l_it.valid())
		{
			if (cp_y(outEntry(infos[v], odSouth, pos)) <= infos[v].coord(odEast) + m_sep)
			{
				m_abends[outEntry(infos[v], odSouth, pos)] = bend_2left;
				infos[v].inc_E(odSouth, odEast);
			}//N/O?????!!!!!
			else
			{
				m_abends[outEntry(infos[v], odSouth, pos)] = prob_b1l;
				infos[v].inc_E_hook(odSouth, odEast);
			}//else
			m_agp_y[outEntry(infos[v], odSouth, pos)] = infos[v].coord(odEast)
				- infos[v].eps(odSouth, odEast)
				- (infos[v].inList(odSouth).size() - 1 - pos)*infos[v].delta(odSouth, odEast);
			l_it++;
			pos++;
		}//while
	}//else rightgen
	//end set m_gy

	//x coordinates, just on the cage boundary
	int l_pos = 0;
	l_it = infos[v].inList(odNorth).begin();
	while (l_it.valid())
	{
		m_agp_x[outEntry(infos[v], odNorth, l_pos)] = infos[v].coord(odNorth);
		l_it++;
		l_pos++;
	}
	l_it = infos[v].inList(odSouth).begin();
	l_pos = 0;
	while (l_it.valid())
	{
		m_agp_x[outEntry(infos[v], odSouth, l_pos)] = infos[v].coord(odSouth);
		l_it++;
		l_pos++;
	}
}//gengluey


//todo: parameterize the different functions and delete the obsolete copies

//compute preliminary glue point positions based on placement
//and generalizations for horizontal edges
void EdgeRouter::compute_gen_glue_points_x(node v)
{
	OGDF_ASSERT(infos[v].has_gen(odEast) || infos[v].has_gen(odWest));
	int xbase = 0;

	//position generalization in the middle of the node
	int gen_x = infos[v].coord(odNorth) + infos[v].node_xsize()/2; //in the middle
	//x coordinates in m_gp_x, set bend types for all edges

	//**********************************************************
	//TOP SIDE *************************************************
	ListIterator<edge> l_it = infos[v].inList(odEast).begin();

	//TOP GENERATOR ******************************************
	if (infos[v].has_gen(odEast))//gen at top side
	{
		int pos = infos[v].gen_pos(odEast)-1; //compare edge position to generalization position
		if (pos > -1)
			l_it = infos[v].inList(odEast).get(pos);
		else {
			l_it = 0; pos = 0;
		}
		//classify edges
		//***************************************************
		//assign gp value for edges underneath generalization
		xbase = gen_x - infos[v].delta(odEast, odNorth);

		//bendfree edges underneath
		bool check = false;
		while (l_it.valid() &&
			(pos*infos[v].delta(odEast, odNorth) + infos[v].eps(odEast,odNorth) <=
			(cp_x(outEntry(infos[v], odEast, pos)) - infos[v].coord(odNorth)) ))
		{
			m_agp_x[outEntry(infos[v], odEast, pos)] = cp_x(outEntry(infos[v], odEast, pos));
			infos[v].set_t_right(m_agp_x[outEntry(infos[v], odEast, pos)]);
			if (!check) {
				check = true;
				infos[v].set_t_left(m_agp_x[outEntry(infos[v], odEast, pos)]);
			}
			m_abends[outEntry(infos[v], odEast, pos)] = bend_free;
			xbase = cp_x(outEntry(infos[v], odEast, pos)) - infos[v].delta(odEast, odNorth);
			l_it--;
			pos--;
			infos[v].nbf(odEast)++;
		}//while

		while (l_it.valid()) //still some lower edges to bend
		{
			m_agp_x[outEntry(infos[v], odEast, pos)] = xbase;
			if (cp_x(outEntry(infos[v], odEast, pos)) < infos[v].coord(odNorth) - m_sep)
			{
				m_abends[outEntry(infos[v], odEast, pos)] = prob_b1l;
				infos[v].inc_E_hook(odEast, odNorth);
			}
			else {
				m_abends[outEntry(infos[v], odEast, pos)] = prob_b2l;
				infos[v].inc_E(odEast, odNorth);
			}
			xbase = xbase - infos[v].delta(odEast, odNorth);
			l_it--;
			pos--;
		}

		//*******************************************************
		//assign gp value for generalization
		xbase = gen_x; //check == x(current edge)???
		l_it = infos[v].inList(odEast).get(infos[v].gen_pos(odEast));
		m_agp_x[outEntry(infos[v], odEast, infos[v].gen_pos(odEast))] = xbase;
		m_abends[outEntry(infos[v], odEast, infos[v].gen_pos(odEast))] = bend_free;
		infos[v].nbf(odEast)++;
		if (!check)
			infos[v].set_t_left(m_agp_x[outEntry(infos[v], odEast, infos[v].gen_pos(odEast))]);
		infos[v].set_t_right(m_agp_x[outEntry(infos[v], odEast, infos[v].gen_pos(odEast))]);

		//*******************************************************
		//assign gp value for bendfree edges above generalization
		l_it++;
		pos = infos[v].gen_pos(odEast) + 1;
		while (l_it.valid() &&
			((infos[v].inList(odEast).size() - 1 - pos)*infos[v].delta(odEast, odSouth) + infos[v].eps(odEast,odSouth) <=
			(infos[v].coord(odSouth) - cp_x(outEntry(infos[v], odEast, pos))) ))
		{
			m_abends[outEntry(infos[v], odEast, pos)] = bend_free;
			xbase = cp_x(outEntry(infos[v], odEast, pos));//+= infos[v].delta(odEast, odSouth);
			m_agp_x[outEntry(infos[v], odEast, pos)] = xbase;
			infos[v].set_t_right(m_agp_x[outEntry(infos[v], odEast, pos)]);
			l_it++; pos++;
			infos[v].nbf(odEast)++;
		}
		//*******************************************************
		//assign gp value for bend edges on top of generalization
		while (l_it.valid())
		{
			//check for single/2 bend
			if (m_acp_x[outEntry(infos[v], odEast, pos)] < infos[v].coord(odSouth) + m_sep)
			{
				m_abends[outEntry(infos[v], odEast, pos)] = prob_b2r;
				infos[v].inc_E(odEast, odSouth);
			}
			else {
				m_abends[outEntry(infos[v], odEast, pos)] = prob_b1r;
				infos[v].inc_E_hook(odEast, odSouth);
			}
			xbase += infos[v].delta(odEast, odSouth); //there is a generalization
			m_agp_x[outEntry(infos[v], odEast, pos)] = xbase;
			l_it++;
			pos++;
		}//while valid
	}//if top gen
	//NO TOPGENERATOR ****************************************************
	else
	{
		int pos = 0;
		int numbends = 0; //number of bend edges, used to correct position after assignment
		OGDF_ASSERT(infos[v].has_gen(odWest));//obs

		//******************
		//edges bending down
		//******************
		while (l_it.valid() &&
			( infos[v].coord(odNorth)  >
			(cp_x(outEntry(infos[v], odEast, pos)) - pos*infos[v].delta(odEast, odNorth)
			- infos[v].eps(odEast,odNorth)) ))
		{
			if (cp_x(outEntry(infos[v], odEast, pos)) > infos[v].coord(odNorth) - m_sep)//must be doublebend
			{
				m_abends[outEntry(infos[v], odEast, pos)] = bend_2left;
				infos[v].inc_E(odEast, odNorth);
			}
			else //may be singlebend
			{
				m_abends[outEntry(infos[v], odEast, pos)] = prob_b1l;
				infos[v].inc_E_hook(odEast, odNorth);
			}

			m_agp_x[outEntry(infos[v], odEast, pos)] = infos[v].coord(odNorth)
				+ infos[v].eps(odEast, odNorth)
				+ pos*infos[v].delta(odEast, odNorth);
			l_it++;
			pos++;
		}//while
		numbends = pos;
		//*********************
		//bendfree edges
		//*********************
		bool check =  false;
		int lastunbend = m_init;
		int firstunbend = m_init;
		while (l_it.valid() && ( infos[v].coord(odSouth)  >=
			(cp_x(outEntry(infos[v], odEast, pos))
			+ (infos[v].inList(odEast).size() - 1 - pos)*infos[v].delta(odEast, odNorth)
			+ infos[v].eps(odEast,odNorth)) ))
		{
			m_abends[outEntry(infos[v], odEast, pos)] = bend_free;
			infos[v].nbf(odEast)++;
			lastunbend = m_agp_x[outEntry(infos[v], odEast, pos)] = cp_x(outEntry(infos[v], odEast, pos));
			if (firstunbend == m_init) firstunbend = lastunbend;
			if (!check) {
				check = true;
				infos[v].set_t_left(m_agp_x[outEntry(infos[v], odEast, pos)] );
			}
			infos[v].set_t_right(m_agp_x[outEntry(infos[v], odEast, pos)]);
			l_it++;
			pos++;
		}
		//*********************************************************************
		//now we set all bending edges (left) as close as possible to the unbend edges
		//to allow possible bend saving by edge flipping at the corner

		if (firstunbend != m_init)
		{
			ListIterator<edge> ll_it = infos[v].inList(odEast).begin();
			int llpos = 0;
			while (ll_it.valid() && ( infos[v].coord(odNorth)  >
				(cp_x(outEntry(infos[v], odEast, llpos))
				- llpos*infos[v].delta(odEast, odNorth)
				- infos[v].eps(odEast,odNorth)) ))
			{
				m_agp_x[outEntry(infos[v], odEast, llpos)] = firstunbend -
					(numbends - llpos)*infos[v].delta(odEast, odNorth);
				ll_it++;
				llpos++;
			}//while
		}//if unbend edges
		//*********************************************************************
		//*********************
		//edges bending upwards
		//*********************
		while (l_it.valid()) //&&???!!!
		{
			if (cp_x(outEntry(infos[v], odEast, pos)) <= infos[v].coord(odSouth) + m_sep)
			{
				m_abends[outEntry(infos[v], odEast, pos)] = bend_2right;
				infos[v].inc_E(odEast, odSouth);
			}
			else {
				m_abends[outEntry(infos[v], odEast, pos)] = prob_b1r;
				infos[v].inc_E_hook(odEast, odSouth);
			}//else
			m_agp_x[outEntry(infos[v], odEast, pos)] = infos[v].coord(odSouth)
				- infos[v].eps(odEast, odSouth)
				- (infos[v].inList(odEast).size() - 1 - pos)*infos[v].delta(odEast, odSouth);
			l_it++;
			pos++;
		}

	}//else top gen

	//BOTTOM SIDE **************************************************************
	//BOTTOM GENERALIZATION ***********************************************
	if (infos[v].has_gen(odWest))
	{
		//left copy
		int pos = infos[v].gen_pos(odWest)-1; //compare edge position to generalization position
		if (pos > -1)
			l_it = infos[v].inList(odWest).get(pos);
		else { l_it = 0; pos = 0; }
		//classify edges
		//***************************************************
		//assign gp value for edges underneath generalization
		xbase = gen_x - infos[v].delta(odWest, odNorth);

		//bendfree edges underneath
		bool firstcheck = true;
		while (l_it.valid() &&
			(pos*infos[v].delta(odWest, odNorth) + infos[v].eps(odWest,odNorth) <=
			(cp_x(outEntry(infos[v], odWest, pos)) - infos[v].coord(odNorth)) ))
		{
			m_agp_x[outEntry(infos[v], odWest, pos)] = cp_x(outEntry(infos[v], odWest, pos));
			m_abends[outEntry(infos[v], odWest, pos)] = bend_free;
			xbase = cp_x(outEntry(infos[v], odWest, pos)) - infos[v].delta(odWest, odNorth);
			if (firstcheck) {
				firstcheck = false;
				infos[v].set_b_left(m_agp_x[outEntry(infos[v], odWest, pos)]);
			}
			infos[v].set_b_right(m_agp_x[outEntry(infos[v], odWest, pos)]);
			l_it--;
			pos--;
			infos[v].nbf(odWest)++;
		}//while
		while (l_it.valid()) //still some lower edges to bend, xcoord+eps+delta
		{
			m_agp_x[outEntry(infos[v], odWest, pos)] = xbase;
			if ( cp_x(outEntry(infos[v], odWest, pos)) < infos[v].coord(odNorth) - m_sep) //paper E^
			{
				m_abends[outEntry(infos[v], odWest, pos)] = prob_b1r;
				infos[v].inc_E_hook(odWest, odNorth);
			}
			else {
				m_abends[outEntry(infos[v], odWest, pos)] = bend_2right;
				infos[v].inc_E(odWest, odNorth);
			}
			xbase = xbase - infos[v].delta(odWest, odNorth);
			l_it--;
			pos--;
		}
		//*******************************************************
		//assign gp value for generalization
		xbase = gen_x; //check == x(current edge)???cout
		l_it = infos[v].inList(odWest).get(infos[v].gen_pos(odWest));
		m_agp_x[outEntry(infos[v], odWest, infos[v].gen_pos(odWest))] = xbase;
		m_abends[outEntry(infos[v], odWest, infos[v].gen_pos(odWest))] = bend_free;
		infos[v].nbf(odWest)++;
		if (firstcheck) {
			firstcheck = false;
			infos[v].set_b_right(m_agp_x[outEntry(infos[v], odWest, infos[v].gen_pos(odWest))]);
		}
		infos[v].set_b_left(m_agp_x[outEntry(infos[v], odWest, infos[v].gen_pos(odWest))]);
		//*******************************************************
		//assign gp value for bendfree edges above generalization
		l_it++;
		pos = infos[v].gen_pos(odWest) + 1;
		while (l_it.valid() &&
			((infos[v].inList(odWest).size() - 1 - pos)*infos[v].delta(odWest, odSouth) + infos[v].eps(odWest,odSouth) <=
			(infos[v].coord(odSouth) - cp_x(outEntry(infos[v], odWest, pos))) ))
		{
			m_abends[outEntry(infos[v], odWest, pos)] = bend_free;
			xbase = cp_x(outEntry(infos[v], odWest, pos));//+= infos[v].delta(odNorth, odEast);
			m_agp_x[outEntry(infos[v], odWest, pos)] = xbase;
			infos[v].nbf(odWest)++;
			infos[v].set_b_left(m_agp_x[outEntry(infos[v], odWest, pos)]);
			if (firstcheck)
			{
				infos[v].set_b_right(m_agp_x[outEntry(infos[v], odWest, pos)]);
				firstcheck = false;
			}
			l_it++; pos++;
		}//while
		//*******************************************************
		//assign gp value for bend edges on top of generalization
		while (l_it.valid())
		{
			//check for single/2 bend
			if (m_acp_x[outEntry(infos[v], odWest, pos)] > infos[v].coord(odSouth) + m_sep)
			{
				m_abends[outEntry(infos[v], odWest, pos)] = prob_b1l;
				infos[v].inc_E_hook(odWest, odSouth);
			}
			else {
				m_abends[outEntry(infos[v], odWest, pos)] = prob_b2l;
				infos[v].inc_E(odWest, odSouth);
			}
			xbase += infos[v].delta(odWest, odSouth); //there is a generalization
			m_agp_x[outEntry(infos[v], odWest, pos)] = xbase;
			l_it++;
			pos++;
		}//while valid
	}//if bottomgen
	//************************************************************************
	//NO BOTTOM GENERATOR ****************************************************
	else
	{
		int pos = 0;
		int rightbend = 0; //save number of actually bend edges to correct their position later
		l_it = infos[v].inList(odWest).begin();
		//classify edges
		//******************
		//edges bending down
		//******************
		while (l_it.valid() && ( infos[v].coord(odNorth)  >
			 (cp_x(outEntry(infos[v], odWest, pos)) - pos*infos[v].delta(odWest, odNorth) - infos[v].eps(odWest,odNorth)) ))
		{
			if (cp_x(outEntry(infos[v], odWest, pos)) > infos[v].coord(odNorth) - m_sep)//must be doublebend
			{
				m_abends[outEntry(infos[v], odWest, pos)] = bend_2right;
				infos[v].inc_E(odWest, odNorth);
			}
			else //may be singlebend
			{
				m_abends[outEntry(infos[v], odWest, pos)] = prob_b1r;
				infos[v].inc_E_hook(odWest, odNorth);
			}

			m_agp_x[outEntry(infos[v], odWest, pos)] = infos[v].coord(odNorth)
				+ infos[v].eps(odWest, odNorth)
				+ pos*infos[v].delta(odWest, odNorth);
			l_it++;
			pos++;
		}//while
		rightbend = pos;
		//*********************
		//bendfree edges
		//*********************
		bool firstcheck = true;
		int lastunbend = m_init;
		int firstunbend = m_init;
		while (l_it.valid() && ( infos[v].coord(odSouth)  >=
			(cp_x(outEntry(infos[v], odWest, pos))
			+ (infos[v].inList(odWest).size() - 1 - pos)*infos[v].delta(odWest, odNorth)
			+ infos[v].eps(odWest,odNorth)) ))
		{
			m_abends[outEntry(infos[v], odWest, pos)] = bend_free;
			infos[v].nbf(odWest)++;
			lastunbend = m_agp_x[outEntry(infos[v], odWest, pos)] = cp_x(outEntry(infos[v], odWest, pos));

			if (firstunbend == m_init) firstunbend = lastunbend;

			if (firstcheck)
			{
				infos[v].set_b_right(lastunbend);
				firstcheck = false;
			}
			infos[v].set_b_left(lastunbend);
			l_it++;
			pos++;
		}
		//*********************************************************************
		//no assign bend edges as close as possible

		if (firstunbend != m_init)
		{
			ListIterator<edge> ll_it = infos[v].inList(odWest).begin();
			int llpos = 0;
			while (ll_it.valid() && ( infos[v].coord(odNorth)  >
				(cp_x(outEntry(infos[v], odWest, llpos)) - llpos*infos[v].delta(odWest, odNorth) - infos[v].eps(odWest,odNorth)) ))
			{
				m_agp_x[outEntry(infos[v], odWest, llpos)] = firstunbend -
					(rightbend - llpos)*infos[v].delta(odWest, odNorth);
				ll_it++;
				llpos++;
			}//while
		}//if
		//*********************************************************************
		//*********************
		//edges bending upwards
		//*********************

		while (l_it.valid()) //&&???!!!
		{
			if (cp_x(outEntry(infos[v], odWest, pos)) <= infos[v].coord(odSouth) + m_sep)
			{
				m_abends[outEntry(infos[v], odWest, pos)] = bend_2left;
				infos[v].inc_E(odWest, odSouth);
			}//if
			else
			{
				m_abends[outEntry(infos[v], odWest, pos)] = prob_b1l;
				infos[v].inc_E_hook(odWest, odSouth);
			}//else

			if (lastunbend != m_init)
			{
				m_agp_x[outEntry(infos[v], odWest, pos)] = lastunbend + infos[v].delta(odWest, odSouth);
				lastunbend = lastunbend + infos[v].delta(odWest, odSouth);
			}//if
			else
				m_agp_x[outEntry(infos[v], odWest, pos)] = infos[v].coord(odSouth)
							- infos[v].eps(odWest, odSouth)
							- (infos[v].inList(odWest).size() - 1 - pos)*infos[v].delta(odWest, odSouth);
			l_it++;
			pos++;
		}//while

	}//else leftgen
	//end set m_gx

	//y coordinates, just on the cage boundary
	l_it = infos[v].inList(odEast).begin();
	int l_pos = 0;
	while (l_it.valid())
	{
		m_agp_y[outEntry(infos[v], odEast, l_pos)] = infos[v].coord(odEast);
		l_it++;
		l_pos++;
	}
	l_it = infos[v].inList(odWest).begin();
	l_pos = 0;
	while (l_it.valid())
	{
		m_agp_y[outEntry(infos[v], odWest, l_pos)] = infos[v].coord(odWest);
		l_it++;
		l_pos++;
	}
}//gengluex


//compute preliminary glue point positions based on placement
//maybe: use earlier classification of edges: bendfree?
void EdgeRouter::compute_glue_points_y(node v)
{
	//forall edges in horizontal lists, we set the glue point y coordinate
	ListIterator<edge> l_it = infos[v].inList(odNorth).begin();
	int pos = 0;
	int bendDownCounter = 0;
	//left edges
	//classify edges
	//******************
	//edges bending down
	//******************
	while (l_it.valid() && ( infos[v].coord(odWest)  >
		(cp_y(outEntry(infos[v], odNorth, pos)) - pos*infos[v].delta(odNorth, odWest) - infos[v].eps(odNorth,odWest)) ))
	{
		if (cp_y(outEntry(infos[v], odNorth, pos)) > infos[v].coord(odWest) - m_sep)//must be doublebend
		{
			m_abends[outEntry(infos[v], odNorth, pos)] = bend_2left;
			infos[v].inc_E(odNorth, odWest);
		}
		else //may be singlebend
		{
			m_abends[outEntry(infos[v], odNorth, pos)] = prob_b1l;
			infos[v].inc_E_hook(odNorth, odWest);
		}
		m_agp_y[outEntry(infos[v], odNorth, pos)] = infos[v].coord(odWest)
			+ infos[v].eps(odNorth, odWest)
			+ pos*infos[v].delta(odNorth, odWest);
		bendDownCounter++;
		l_it++;
		pos++;
	}//while
	//*********************
	//bendfree edges
	//*********************
	int lastunbend = m_init;
	int firstunbend = m_init;
	bool firstcheck = true;
	while (l_it.valid() && ( infos[v].coord(odEast)  >=
		(cp_y(outEntry(infos[v], odNorth, pos))
		+ (infos[v].inList(odNorth).size() - 1 - pos)*infos[v].delta(odNorth, odWest)
		+ infos[v].eps(odNorth,odWest)) ))
	{
		m_abends[outEntry(infos[v], odNorth, pos)] = bend_free;
		infos[v].nbf(odNorth)++;
		lastunbend = m_agp_y[outEntry(infos[v], odNorth, pos)]  = cp_y(outEntry(infos[v], odNorth, pos));
		if (firstcheck)
		{
			infos[v].set_l_lower(m_agp_y[outEntry(infos[v], odNorth, pos)]);
			firstunbend = lastunbend;
			firstcheck = false;
		}
		infos[v].set_l_upper(m_agp_y[outEntry(infos[v], odNorth, pos)]);
		l_it++;
		pos++;
	}
	//*********************************************************************
	//correct left edges
	if (firstunbend != m_init)
	{
		ListIterator<edge> ll_it = infos[v].inList(odNorth).begin();
		int llpos = 0;
		while (ll_it.valid() && ( infos[v].coord(odWest)  >
			(cp_y(outEntry(infos[v], odNorth, llpos)) - llpos*infos[v].delta(odNorth, odWest) - infos[v].eps(odNorth, odWest)) ))
		{
			m_agp_y[outEntry(infos[v], odNorth, llpos)] = firstunbend -
				(bendDownCounter - llpos)*infos[v].delta(odNorth, odWest);
			ll_it++;
			llpos++;
		}//while
	}//if
	//*********************************************************************
	//*********************
	//edges bending upwards
	//*********************
	while (l_it.valid()) //&&???!!!
	{
		if (cp_y(outEntry(infos[v], odNorth, pos)) <= infos[v].coord(odEast) + m_sep)
		{
			m_abends[outEntry(infos[v], odNorth, pos)] = bend_2right;
			infos[v].inc_E(odNorth, odEast);
		}
		else
		{
			m_abends[outEntry(infos[v], odNorth, pos)] = prob_b1r;
			infos[v].inc_E_hook(odNorth, odEast);
		}
		//leave space to reroute
		if (lastunbend != m_init)
		{
			m_agp_y[outEntry(infos[v], odNorth, pos)] = lastunbend + infos[v].delta(odNorth, odEast);
			lastunbend = lastunbend + infos[v].delta(odNorth, odEast);
		}//if
		else
			m_agp_y[outEntry(infos[v], odNorth, pos)] = infos[v].coord(odEast)
			 - infos[v].eps(odNorth, odEast)
			 - (infos[v].inList(odNorth).size() - 1 - pos)*infos[v].delta(odNorth, odEast);
		l_it++;
		pos++;
	}//while
	//South edges
	pos = 0;
	bendDownCounter = 0;
	l_it = infos[v].inList(odSouth).begin();

	//classify edges
	//******************
	//edges bending down
	//******************
	while (l_it.valid() && ( infos[v].coord(odWest)  >
		(cp_y(outEntry(infos[v], odSouth, pos)) - pos*infos[v].delta(odSouth, odWest) - infos[v].eps(odSouth,odWest)) ))
	{
		if (cp_y(outEntry(infos[v], odSouth, pos)) > infos[v].coord(odWest) - m_sep)//must be doublebend
		{
			m_abends[outEntry(infos[v], odSouth, pos)] = bend_2right; //was left
			infos[v].inc_E(odSouth, odWest);
		}
		else //may be singlebend
		{
			m_abends[outEntry(infos[v], odSouth, pos)] = prob_b1r;
			infos[v].inc_E_hook(odSouth, odWest);
		}
		m_agp_y[outEntry(infos[v], odSouth, pos)] = infos[v].coord(odWest)
			+ infos[v].eps(odSouth, odWest)
			+ pos*infos[v].delta(odSouth, odWest);
		l_it++;
		bendDownCounter++;
		pos++;
	}//while
	//*********************
	//bendfree edges
	//*********************
	firstcheck = true;
	lastunbend = m_init;
	firstunbend = m_init;
	while (l_it.valid() && ( infos[v].coord(odEast)  >=
		(cp_y(outEntry(infos[v], odSouth, pos))
		+ (infos[v].inList(odSouth).size() - 1 - pos)*infos[v].delta(odSouth, odWest)
		+ infos[v].eps(odSouth,odWest)) ))
	{
		m_abends[outEntry(infos[v], odSouth, pos)] = bend_free;
		infos[v].nbf(odSouth)++;
		lastunbend = m_agp_y[outEntry(infos[v], odSouth, pos)] = cp_y(outEntry(infos[v], odSouth, pos));
		if (firstcheck)
		{
			firstcheck = false;
			infos[v].set_r_lower(m_agp_y[outEntry(infos[v], odSouth, pos)]);
			firstunbend = lastunbend;
		}
		infos[v].set_r_upper(m_agp_y[outEntry(infos[v], odSouth, pos)]);
		l_it++;
		pos++;
	}//while

	//***************************
	//correct right bending edges
	if (firstunbend != m_init)
	{
		ListIterator<edge> ll_it = infos[v].inList(odSouth).begin();
		int llpos = 0;
		while (ll_it.valid() && ( infos[v].coord(odWest)  >
			(cp_y(outEntry(infos[v], odSouth, llpos)) - llpos*infos[v].delta(odSouth, odWest)
			- infos[v].eps(odSouth, odWest)) ))
		{
			m_agp_y[outEntry(infos[v], odSouth, llpos)] = firstunbend -
				(bendDownCounter - llpos)*infos[v].delta(odSouth, odWest);
			ll_it++;
			llpos++;
		}//while
	}//if
	//*********************************************************************
	//*********************
	//edges bending upwards
	//*********************
	while (l_it.valid()) //&&???!!!
	{
		if (cp_y(outEntry(infos[v], odSouth, pos)) <= infos[v].coord(odEast) + m_sep)
		{
			m_abends[outEntry(infos[v], odSouth, pos)] = bend_2left;
			infos[v].inc_E(odSouth, odEast);
		}//was right
		else
		{
			m_abends[outEntry(infos[v], odSouth, pos)] = prob_b1l;
			infos[v].inc_E_hook(odSouth, odEast);
		}//else
		//leave as much space as possible for rerouters
		if (lastunbend != m_init)
		{
			m_agp_y[outEntry(infos[v], odSouth, pos)] = lastunbend + infos[v].delta(odSouth, odEast);
			lastunbend = lastunbend + infos[v].delta(odSouth, odEast);
		}//if
		else
			m_agp_y[outEntry(infos[v], odSouth, pos)] =
				infos[v].coord(odEast) - infos[v].eps(odSouth, odEast) - (infos[v].inList(odSouth).size() - 1 - pos)*infos[v].delta(odSouth, odEast);
		l_it++;
		pos++;
	}//while

	//x coordinates, just on the cage boundary
	l_it = infos[v].inList(odNorth).begin();
	int l_pos = 0;
	while (l_it.valid())
	{
		m_agp_x[outEntry(infos[v], odNorth, l_pos)] = infos[v].coord(odNorth);
		l_it++;
		l_pos++;
	}
	l_it = infos[v].inList(odSouth).begin();
	l_pos = 0;
	while (l_it.valid())
	{
		m_agp_x[outEntry(infos[v], odSouth, l_pos)] = infos[v].coord(odSouth);
		l_it++;
		l_pos++;
	}
}


void EdgeRouter::compute_glue_points_x(node& v)
//compute preliminary glue point positions based on placement
//maybe: use earlier classification of edges: bendfree?
{
	//forall edges in vertical lists, we set the glue point x coordinate
	//TOP SIDE *************************************************
	ListIterator<edge> l_it = infos[v].inList(odEast).begin();
	int pos = 0;
	int numbends = 0;
	//classify edges
	//******************
	//edges bending down
	//******************
	while (l_it.valid() && ( infos[v].coord(odNorth)  >
		 (cp_x(outEntry(infos[v], odEast, pos)) - pos*infos[v].delta(odEast, odNorth) - infos[v].eps(odEast,odNorth)) ))
	{
		if (cp_x(outEntry(infos[v], odEast, pos)) > infos[v].coord(odNorth) - m_sep)//must be doublebend
		{
			m_abends[outEntry(infos[v], odEast, pos)] = bend_2left;
			infos[v].inc_E(odEast, odNorth);
		}
		else //may be singlebend
		{
			m_abends[outEntry(infos[v], odEast, pos)] = prob_b1l;
			infos[v].inc_E_hook(odEast, odNorth);
		}
		m_agp_x[outEntry(infos[v], odEast, pos)] = infos[v].coord(odNorth)
				+ infos[v].eps(odEast, odNorth) + pos*infos[v].delta(odEast, odNorth);
		l_it++;
		pos++;
	}//while
	numbends = pos;
	int lastunbend = m_init;
	int firstunbend = m_init;
	//*********************
	//bendfree edges
	//*********************
	bool firstcheck = true;
	while (l_it.valid() && ( infos[v].coord(odSouth)  >=
		(cp_x(outEntry(infos[v], odEast, pos))
		+ (infos[v].inList(odEast).size() - 1 - pos)*infos[v].delta(odEast, odNorth)
		+ infos[v].eps(odEast,odNorth)) ))
	{
		m_abends[outEntry(infos[v], odEast, pos)] = bend_free;
		infos[v].nbf(odEast)++;
		lastunbend = m_agp_x[outEntry(infos[v], odEast, pos)] = cp_x(outEntry(infos[v], odEast, pos));
		if (firstunbend == m_init) firstunbend = lastunbend;
		if (firstcheck)
		{
			firstcheck = false;
			infos[v].set_t_left(lastunbend);
		}//if
		infos[v].set_t_right(lastunbend);
		l_it++;
		pos++;
	}
	//*********************************************************************
	//temporary sol, correct left edges

	if (firstunbend != m_init)
	{
		ListIterator<edge> ll_it = infos[v].inList(odEast).begin();
		int llpos = 0;
		while (ll_it.valid() && ( infos[v].coord(odNorth)  >
			(cp_x(outEntry(infos[v], odEast, llpos))
			- llpos*infos[v].delta(odEast, odNorth)
			- infos[v].eps(odEast,odNorth)) ))
		{
			m_agp_x[outEntry(infos[v], odEast, llpos)] = firstunbend -
				(numbends - llpos)*infos[v].delta(odEast, odNorth);
			ll_it++;
			llpos++;
		}//while
	}//if
	//*********************************************************************
	//*********************
	//edges bending to the right side
	//*********************
	while (l_it.valid()) //&&???!!!
	{
		if (cp_x(outEntry(infos[v], odEast, pos)) <= infos[v].coord(odSouth) + m_sep)
		{
			if (cp_x(outEntry(infos[v], odEast, pos)) > infos[v].coord(odSouth) - infos[v].eps(odEast, odSouth))
			{
				m_abends[outEntry(infos[v], odEast, pos)] = prob_b2r;
				infos[v].inc_E(odEast, odSouth);
			}//if
			else
			{
				m_abends[outEntry(infos[v], odEast, pos)] = prob_b2r;//use prob_bf;
				infos[v].inc_E(odEast, odSouth);
			}//else
		}//if
		else
		{
			m_abends[outEntry(infos[v], odEast, pos)] = prob_b1r;
			infos[v].inc_E_hook(odEast, odSouth);
		}//else
		if (lastunbend != m_init)
		{
			m_agp_x[outEntry(infos[v], odEast, pos)] = lastunbend + infos[v].delta(odEast, odSouth);
			lastunbend = lastunbend + infos[v].delta(odEast, odSouth);
		}//if
		else m_agp_x[outEntry(infos[v], odEast, pos)] = infos[v].coord(odSouth)
			- infos[v].eps(odEast, odSouth)
			- (infos[v].inList(odEast).size() - 1 - pos)*infos[v].delta(odEast, odSouth);
		l_it++;
		pos++;
	}//while

	//bottom**********************************************************
	pos = 0;
	l_it = infos[v].inList(odWest).begin();
	int rightbend = 0; //save number of atually bend edge to correct their position later
	//classify edges
	//******************
	//edges bending to north dir / westright
	//******************
	while (l_it.valid() && ( infos[v].coord(odNorth)  >
		(cp_x(outEntry(infos[v], odWest, pos)) - pos*infos[v].delta(odWest, odNorth) - infos[v].eps(odWest,odNorth)) ))
	{
		if (cp_x(outEntry(infos[v], odWest, pos)) > infos[v].coord(odNorth) - m_sep)//must be doublebend
		{
			m_abends[outEntry(infos[v], odWest, pos)] = bend_2right;
			infos[v].inc_E(odWest, odNorth);
		}
		else //may be singlebend
		{
			m_abends[outEntry(infos[v], odWest, pos)] = prob_b1r;
			infos[v].inc_E_hook(odWest, odNorth);
		}
		m_agp_x[outEntry(infos[v], odWest, pos)] = infos[v].coord(odNorth)
			+ infos[v].eps(odWest, odNorth)
			+ pos*infos[v].delta(odWest, odNorth);
		l_it++;
		pos++;
	}//while
	rightbend = pos; //NUMBER of bend edges
	//*********************
	//bendfree edges
	//*********************
	firstunbend = true;
	firstcheck = true;
	lastunbend = m_init;
	firstunbend = m_init;
	while (l_it.valid() && ( infos[v].coord(odSouth)  >=
		(cp_x(outEntry(infos[v], odWest, pos))
		+ (infos[v].inList(odWest).size() - 1 - pos)*infos[v].delta(odWest, odNorth)
		+ infos[v].eps(odWest,odNorth)) ))
	{
		m_abends[outEntry(infos[v], odWest, pos)] = bend_free;
		infos[v].nbf(odWest)++;
		lastunbend = m_agp_x[outEntry(infos[v], odWest, pos)] = cp_x(outEntry(infos[v], odWest, pos));
		if (firstunbend == m_init) firstunbend = lastunbend;
		if (firstcheck)
		{
			firstcheck = false;
			infos[v].set_b_right(lastunbend);
		}
		infos[v].set_b_left(lastunbend);
		l_it++;
		pos++;
	}
	//*********************************************************************
	//temporary sol, correct left edges

	if (firstunbend != m_init)
	{
		ListIterator<edge> ll_it = infos[v].inList(odWest).begin();
		int llpos = 0;
		while (ll_it.valid() && ( infos[v].coord(odNorth)  >
			(cp_x(outEntry(infos[v], odWest, llpos)) - llpos*infos[v].delta(odWest, odNorth) - infos[v].eps(odWest,odNorth)) ))
		{
			//ab unterem Rand, oder ab gen, teile Abstand Anzahl Kanten??!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
			m_agp_x[outEntry(infos[v], odWest, llpos)] = firstunbend -
				(rightbend - llpos)*infos[v].delta(odWest, odNorth);
			ll_it++;
			llpos++;
		}//while
		OGDF_ASSERT(rightbend == llpos);
	}//if
	//*********************************************************************
	//*********************
	//edges bending upwards
	//*********************
	while (l_it.valid()) //&&???!!!
	{
		if (cp_x(outEntry(infos[v], odWest, pos)) <= infos[v].coord(odSouth) + m_sep)
		{
			m_abends[outEntry(infos[v], odWest, pos)] = bend_2left;
			infos[v].inc_E(odWest, odSouth);
		}
		else
		{
			m_abends[outEntry(infos[v], odWest, pos)] = prob_b1l;
			infos[v].inc_E_hook(odWest, odSouth);
		}//else

		if (lastunbend != m_init)
		{
			m_agp_x[outEntry(infos[v], odWest, pos)] = lastunbend + infos[v].delta(odWest, odSouth);
			lastunbend = lastunbend + infos[v].delta(odWest, odSouth);
		}
		else
			m_agp_x[outEntry(infos[v], odWest, pos)] = infos[v].coord(odSouth)
			- infos[v].eps(odWest, odSouth)
			- (infos[v].inList(odWest).size() - 1 - pos)*infos[v].delta(odWest, odSouth);
		l_it++;
		pos++;
	}//while
	//y values
	//y coordinates, just on the cage boundary
	l_it = infos[v].inList(odEast).begin();
	int l_pos = 0;
	while (l_it.valid())
	{
		m_agp_y[outEntry(infos[v], odEast, l_pos)] = infos[v].coord(odEast);
		l_it++;
		l_pos++;
	}
	l_it = infos[v].inList(odWest).begin();
	l_pos = 0;
	while (l_it.valid())
	{
		m_agp_y[outEntry(infos[v], odWest, l_pos)] = infos[v].coord(odWest);
		l_it++;
		l_pos++;
	}
}//compute gpx


void EdgeRouter::set_corners(node v)
{
	//set the layout position
	//set the expandedNode entries for the corners, should be where they are inserted
	edge e;
	node w;
	const OrthoRep::VertexInfoUML* vinfo = m_orp->cageInfo(v);
	adjEntry ae = vinfo->m_corner[0];
	e = *ae;
	w = e->source();//pointing towards north, on left side
	m_prup->setExpandedNode(w, v);

	m_layoutp->x(w) = infos[v].coord(odNorth);
	m_layoutp->y(w) = infos[v].coord(odWest);
	ae = vinfo->m_corner[1];
	e = *ae;
	w = e->source();
	m_prup->setExpandedNode(w, v);

	m_layoutp->x(w) = infos[v].coord(odNorth);
	m_layoutp->y(w) = infos[v].coord(odEast);
	ae = vinfo->m_corner[2];
	e = *ae;
	w = e->source();
	m_prup->setExpandedNode(w, v);

	m_layoutp->x(w) = infos[v].coord(odSouth);
	m_layoutp->y(w) = infos[v].coord(odEast);
	ae = vinfo->m_corner[3];
	e = *ae;
	w = e->source();
	m_prup->setExpandedNode(w, v);

	m_layoutp->x(w) = infos[v].coord(odSouth);
	m_layoutp->y(w) = infos[v].coord(odWest);
}//setcorners



//*****************************************************************************
//locally decide where to place the node in the computed cage area
//allow individual separation and overhang distance, input original node v
//classify edges with preliminary bend_types prob_xxx
//choose bendfree edges, bend edges may be rerouted to save bends
//*****************************************************************************

void EdgeRouter::compute_place(node v, NodeInfo& inf/*, int l_sep, int l_overh*/)
{
	int l_directvalue = 10; //value of edges connecting two cages bendfree, with bends: value = 1

	//gen at left or right cage side
	bool horizontal_merger = (inf.has_gen(odNorth) || inf.has_gen(odSouth));
	//gen at top or bottom side
	bool vertical_merger = (inf.has_gen(odWest) || inf.has_gen(odEast));

	List<edge> l_horz; //contains horizontal incoming edges at v's cage,  sorted increasing uppe, paper L
	List<edge> l_horzl; //by increasing lowe

	List<int> edgevalue; //saves value for direct / bend edges in lhorz

	//for every element of list l_horzl, we store its iterator in l_horz, ist Wahnsinn, spaeter global
	EdgeArray< ListIterator<edge> > horz_entry(*m_prup);
	EdgeArray< ListIterator<edge> > vert_entry(*m_prup);
	EdgeArray< ListIterator<int> > value_entry(*m_prup);
	EdgeArray<bool> valueCounted(*m_prup, false); //did we consider edge in numunbend sum?
	List<edge> l_vert; //         vertical
	List<edge> l_vertl; //by increasing lefte
	//attachment side, maybe check the direction instead
	EdgeArray<bool> at_left(*m_prup, false);
	EdgeArray<bool> at_top(*m_prup, false);

	//Fill edge lists ******************************************************
	int lhorz_size = inf.inList(odNorth).size() + inf.inList(odSouth).size();
	if (lhorz_size && (!horizontal_merger))
	{
		edge e;
		//fill l_horz sorting by uppe, remember entry for edges sorted by lowe
		//all inf.inList[odNorth] and [odSouth], sorted by lower

		//check: each side is already sorted by lowe/uppe definition!!!!!!!!!!!!!!!!!!!
		ListIterator<edge> li_l = inf.inList(odNorth).begin();
		ListIterator<edge> li_r = inf.inList(odSouth).begin();
		//iterators for increasing lower value
		ListIterator<edge> li_ll = inf.inList(odNorth).begin();
		ListIterator<edge> li_lr = inf.inList(odSouth).begin();

		int uppe_l, uppe_r, lowe_l, lowe_r;

		uppe_l = (li_l.valid() ? auppe[outEntry(inf, odNorth, 0)] : INT_MAX);//only both 10000.0 if both empty!!!
		uppe_r = (li_r.valid() ? auppe[outEntry(inf, odSouth, 0)] : INT_MAX);//never in for - loop
		lowe_l = (li_ll.valid() ? alowe[outEntry(inf, odNorth, 0)] : INT_MAX);//only both 10000.0 if both empty!!!
		lowe_r = (li_lr.valid() ? alowe[outEntry(inf, odSouth, 0)] : INT_MAX);//never in for - loop

		int lcount, rcount, llcount, rlcount;//zaehle fuer outEntry in Listen
		lcount = rcount = llcount = rlcount = 0;
		//forall horizontal edges, sort
		for (int k = 0; k < lhorz_size; k++)
		{
			node nextNeighbour;
			//run in parallel over opposite side lists
			//upper value
			if (uppe_l <= uppe_r) //favour left edges, maybe we should prefer them
			{
				e = *li_l;
				at_left[e] = true;
				nextNeighbour = (inf.is_in_edge(odNorth, lcount) ? e->source() : e->target());
				li_l++;
				lcount++;
				uppe_l =  (lcount <  inf.inList(odNorth).size() ? auppe[outEntry(inf, odNorth, lcount)] : INT_MAX);
			}
			else
			{
				e = *li_r;
				nextNeighbour = (inf.is_in_edge(odSouth, rcount) ? e->source() : e->target());
				li_r++;
				rcount++;
				uppe_r = (rcount < inf.inList(odSouth).size() ? auppe[outEntry(inf, odSouth, rcount)] : INT_MAX);
			}
			horz_entry[e] = l_horz.pushBack(e);//fill edge in L

			value_entry[e] = edgevalue.pushBack((m_prup->expandedNode(nextNeighbour) ? l_directvalue : 1));

			//lower value
			if (lowe_l <= lowe_r) //favour left edges, maybe we should prefer them
			{
				e = *li_ll;
				//at_left[e] = true; //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
				li_ll++;
				llcount++;
				lowe_l = (llcount < inf.inList(odNorth).size() ? alowe[outEntry(inf, odNorth, llcount)] : INT_MAX);
			}
			else
			{
				e = *li_lr;
				li_lr++;
				rlcount++;
				//lowe_r = (li_lr.valid() ? lowe[*li_lr] : 10000.0);
				lowe_r = (rlcount < inf.inList(odSouth).size() ? alowe[outEntry(inf, odSouth, rlcount)] : INT_MAX);
			}
			l_horzl.pushBack(e); //fill edge in e_0,...e_k

		}//for
	}//if horz
	//vertical edges *******************************************************
	int lvert_size = inf.inList(odEast).size() + inf.inList(odWest).size();
	if (lvert_size && !vertical_merger)
	{
		//fill l_vert
		edge e;
		//fill l_vert sorting by righte, remember entry for edges sorted by lefte
		//all inf.inList[odEast] and [odWest], sorted by lefte

		//check: each side is already sorted by lefte/righte definition!!!!!!!!!!!!!!!!!!!
		ListIterator<edge> li_t = inf.inList(odEast).begin();
		ListIterator<edge> li_b = inf.inList(odWest).begin();
		//iterators for increasing lefte value
		ListIterator<edge> li_lt = inf.inList(odEast).begin();
		ListIterator<edge> li_lb = inf.inList(odWest).begin();

		int righte_t, righte_b, lefte_t, lefte_b;
		righte_t = (li_t.valid() ? arighte[outEntry(inf, odEast, 0)] : INT_MAX);//only both 10000.0 if both empty!!!
		righte_b = (li_b.valid() ? arighte[outEntry(inf, odWest, 0)] : INT_MAX);//never in for - loop
		lefte_t = (li_lt.valid() ? alefte[outEntry(inf, odEast, 0)] : INT_MAX);//only both 10000.0 if both empty!!!
		lefte_b = (li_lb.valid() ? alefte[outEntry(inf, odWest, 0)] : INT_MAX);//never in for - loop

		//forall horizontal edges, sort
		int tcount, bcount, tlcount, blcount;//zaehle fuer outEntry in Listen
		tcount = bcount = tlcount = blcount = 0;
		for (int k = 0; k < lvert_size; k++)
		{
			if (li_t.valid() || li_b.valid())
			{
				//righter value
				if (righte_t <= righte_b) //favour top edges, maybe we should prefer them
				{
					if (li_t.valid())
					{
						e = *li_t;
						at_top[e] = true; //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
						li_t++;
						tcount++;
					}//if
					righte_t = (tcount < inf.inList(odEast).size() ? arighte[outEntry(inf, odEast, tcount)] : INT_MAX);
				}
				else
				{
					if (li_b.valid())
					{
						e = *li_b;
						li_b++;
						bcount++;
					}//if
					righte_b = (bcount < inf.inList(odWest).size()  ? arighte[outEntry(inf, odWest, bcount)] : INT_MAX);
				}
				vert_entry[e] = l_vert.pushBack(e);//fill edge in L
			}
			//lefter value
			if (lefte_t <= lefte_b) //favour top edges, maybe we should prefer them
			{
				e = *li_lt;
				//at_left[e] = true; //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
				li_lt++;
				tlcount++;
				//lefte_t = (li_lt.valid() ? lefte[*li_lt] : 100000.0);
				lefte_t = (tlcount < inf.inList(odEast).size() ? alefte[outEntry(inf, odEast, tlcount)] : INT_MAX);
				//if (lefte_t != (tlcount < inf.inList(odEast).size() ? alefte[outEntry(inf, odEast, tlcount)] : 100000.0)) exit;
			}
			else
			{
				e = *li_lb;
				li_lb++;
				blcount++;
				//lefte_b = (li_lb.valid() ? lefte[*li_lb] : 100000.0);
				lefte_b = (blcount < inf.inList(odWest).size() ? alefte[outEntry(inf, odWest, blcount)] : INT_MAX);
			}
			l_vertl.pushBack(e);
		}//for
	}//if vert

	//we inserted the edges incident to v int the list l_horz (horizontal edges sorted
	//by upper) / l_horzl (horizontal edges sorted by lower) and l_vert (vertical edges
	//sorted by righter / l_vertl (vertical edges sorted by lefter)
	int boxx = inf.node_xsize();
	int boxy = inf.node_ysize();

	//now two iterations are executed to compute the best starting index
	//in the list of horizontal/vertical edges to maximise unbend edges

	//some variables for the placement iteration
	int num_unbend = 0; //current number of unbend edges(paper: size)
	int best_unbend = -1;//best number of unbend edges so far (paper: best)
	int i;

	//vertical position paper ALGORITHM1******************************************************************
	//check e_i sorted by lowe, l_horz sorted by uppe !!!!!!!!!!!!!!
	//we need to have an listentry for all edges sorted by lowe to
	//delete them in l_horz

	if (!l_horz.empty())
	{
		//starting from the lowest horizontal edge, we move the virtual node box up and count the number of potentially
		//unbend edges to find the best starting unbend edge additionally, we set the values for the connection/glue point position
		int stop = l_horz.size();
		int bestvalue;

		if (l_horz.size() == 1)
		{
			best_unbend = 1;
			if (at_left[*(l_horz.begin())])
				bestvalue = alowe[outEntry(inf, odNorth, 0)];
			else bestvalue = alowe[outEntry(inf, odSouth, 0)];
		}
		else
		{
			ListIterator<edge> p = l_horz.begin();
			ListIterator<int> valp = edgevalue.begin();// run over edgevalue in parallel to p

			int leftcount = 0, rightcount = 0;
			for (i = 1; i < stop+1; i++) //in der Regel laenger als noetig??
			{
				if (!valueCounted[l_horzl.front()])
				{
					num_unbend += *value_entry[l_horzl.front()];
					valueCounted[l_horzl.front()] = true;
				}
				while (p.valid())
				{
					//the edge indicated by p fits in the box started with l_horzl.front lower value
					if (uppe[(*p)]<= (lowe[l_horzl.front()]+boxy)) //+machineeps)) //||(p == horz_entry[l_horzl.front()]))
					//assert first edge int horzl is edge i?
					//p's entry needs no bend => increase num_unbend (edges)
					{
						//num_unbend++;
						num_unbend += *valp;
						valueCounted[*p] = true;
						//only to be set if new best_unbend
						p++;
						valp++;
					}//if
					else
					{
						break;
					}
				}//while unbend

				if (num_unbend > best_unbend)
				{
					best_unbend =  num_unbend;
					bestvalue = (at_left[l_horzl.front()] ? alowe[outEntry(inf,odNorth, leftcount)] : alowe[outEntry(inf, odSouth, rightcount)]);
				}//if new best

				if (at_left[l_horzl.front()]) leftcount++;
				else rightcount++;

				if ( p == horz_entry[l_horzl.front()]) p++;
				if ( valp == value_entry[l_horzl.front()]) valp++;

				l_horz.del(horz_entry[l_horzl.front()]);

				if (num_unbend) num_unbend -= *value_entry[l_horzl.front()];

				OGDF_ASSERT(num_unbend >= 0);

				edgevalue.del(value_entry[l_horzl.front()]);

				valueCounted[l_horzl.front()] = false;

				l_horzl.popFront();
			}//for, l_horz list entries

		}//else bugfix size 1

		m_newy[v] = min((inf.cage_coord(odEast) - inf.node_ysize() - inf.rc(odEast)), bestvalue);
		inf.set_coord(odWest, m_newy[v]);
		inf.set_coord(odEast, m_newy[v] + inf.node_ysize());
	}//if horz
	else
	{
		if (horizontal_merger)
		{
			//position is fixed //odNorth was odNorth
			edge e;
			//note that get starts indexing with zero, whereas gen_pos starts with one
			if (inf.has_gen(odNorth)) e = *(inf.inList(odNorth).get(inf.gen_pos(odNorth)));
			else e = *(inf.inList(odSouth).get(inf.gen_pos(odSouth))); //check e !!!!!!!!!!!!!
			int gen_y = m_layoutp->y( e->target()); //koennte man auch in inf schreiben !!!!!!!!!
			m_newy[v] =  gen_y - int(floor((double)(inf.node_ysize())/2));
			inf.set_coord(odWest, m_newy[v]);
			inf.set_coord(odEast, m_newy[v] + inf.node_ysize());
		}//if
		else
		{
			//new heuristics: look for vertical generalization and shift towards it
			if (vertical_merger)
			{
				//find out direction of generalization
				bool wg = inf.has_gen(odWest);
				bool eg = inf.has_gen(odEast);
				int mynewy = 0;
				if (wg)
				{
					if (eg)
					{
						if (inf.is_in_edge(odWest, inf.gen_pos(odWest))) //position to odEast
						{
							mynewy = inf.cage_coord(odEast) - inf.rc(odEast) - inf.node_ysize();
						}
						else
						{
							mynewy = inf.cage_coord(odWest) + inf.rc(odWest);
						}
					}//both
					else
					{
						mynewy = inf.cage_coord(odWest) + inf.rc(odWest);
					}//only west
				}
				else
				{
					mynewy = inf.cage_coord(odEast) - inf.rc(odEast) - inf.node_ysize();
				}//else no west
				m_newy[v] = mynewy;
				inf.set_coord(odWest, m_newy[v]);
				inf.set_coord(odEast, m_newy[v] + inf.node_ysize());
			}//if verticalmerger
			else
			{
				//we place the node at the position cage_lower_border(v)+routing_channel_bottom(v)
				m_newy[v] = inf.cage_coord(odEast) - inf.rc(odEast) - inf.node_ysize();
				inf.set_coord(odWest, m_newy[v]);
				inf.set_coord(odEast, m_newy[v] + inf.node_ysize());
				//forall horizontal edges set their glue point y coordinate
			}//else verticalmerger
		}//else merger
	}//else horz


	//forall horizontal edges we computed the y-coordinate of their glue point in m_gp_y
	//and we computed the y-coordinate of the lower box segment in m_newy
	//horizontal position****************************
	if (!l_vert.empty())
	{
		//starting from the leftmost vertical edge, we move the virtual
		//node box rightwards and count the number of potentially unbend edges
		//to find the best starting unbend edge
		num_unbend = 0;
		best_unbend = -1;
		//edge bestedge;
		int bestvalue;
		int stop = l_vert.size();
		//bugfix
		if (l_vert.size() == 1)
		{
			best_unbend = 1;
			//bestedge = l_vert.front();
			if (at_top[*(l_vert.begin())])
				bestvalue = alefte[outEntry(inf, odEast, 0)];
			else bestvalue = alefte[outEntry(inf, odWest, 0)];
		}
		else
		{
			//ALGORITHM 1
			int topcount = 0, lowcount = 0;
			ListIterator<edge> p = l_vert.begin(); //pointer on paper list L
			for (i = 1; i < stop+1; i++)
			{
				while (p.valid())
				{
					if  (righte[(*p)] <= lefte[l_vertl.front()]+boxx+machineeps) //assert first edge is edge i?
					//(p == vert_entry[l_vertl.front()]))
					//p's entry needs no bend => increase num_unbend (edges)
					{
						num_unbend++;
						++p;
					}//while unbend
					else break;
				}
				if (num_unbend > best_unbend)
				{
					best_unbend =  num_unbend;
					//bestedge = l_vertl.front();
					bestvalue = (at_top[l_vertl.front()] ? alefte[outEntry(inf,odEast, topcount)] : alefte[outEntry(inf, odWest, lowcount)]);
				}//if new best
				if (at_top[l_vertl.front()]) topcount++;
				else lowcount++;

				if (p == vert_entry[l_vertl.front()]) p++; //may be -- if valid

				OGDF_ASSERT(p != vert_entry[l_vertl.front()])

				l_vert.del(vert_entry[l_vertl.front()]);
				l_vertl.popFront();

				if (num_unbend) num_unbend--;  //the next index will bend current start edge
			}//for, l_vert list entries
		}//else bugfix

		//assign computed value
		m_newx[v] = min( (inf.cage_coord(odSouth) - inf.node_xsize() - inf.rc(odSouth)),
			(bestvalue));
		// (lefte[bestedge]));
		inf.set_coord(odNorth, m_newx[v]);
		inf.set_coord(odSouth, m_newx[v] + inf.node_xsize());
		//check: do we need this here !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
		//- (at_left[bestedge] ?
		// inf.eps(odNorth, odWest) : inf.eps(odSouth, odWest))));//maybe top / down epsilon
	}//if vert
	else
	{
		if (vertical_merger)
		{
			//position is fixed!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!111
			//position is fixed //odNorth was odNorth
			edge e;
			//note that get starts indexing with zero, whereas gen_pos starts with one
			if (inf.has_gen(odEast)) e = *(inf.inList(odEast).get(inf.gen_pos(odEast)));
			else e = *(inf.inList(odWest).get(inf.gen_pos(odWest))); //check e !!!!!!!!!!!!!
			int gen_x = m_layoutp->x( e->target());
			m_newx[v] =  gen_x - int(inf.node_xsize()/2.0);//abziehen => aufrunden !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
			inf.set_coord(odNorth, m_newx[v]);
			inf.set_coord(odSouth, m_newx[v] + inf.node_xsize());
		}//if
		else
		{
			if (horizontal_merger)
			{
				//find out direction of generalization
				bool sg = inf.has_gen(odSouth);
				bool ng = inf.has_gen(odNorth);

				int mynewx = 0;
				if (sg)
				{
					if (ng)
					{
						if (inf.is_in_edge(odSouth, inf.gen_pos(odSouth))) //position to odNorth
						{
							mynewx = inf.cage_coord(odNorth) + inf.rc(odNorth);
						}
						else
						{
							mynewx = inf.cage_coord(odSouth) - inf.rc(odSouth) - inf.node_xsize();
						}
					}//both
					else
					{
						mynewx = inf.cage_coord(odSouth) - inf.rc(odSouth) - inf.node_xsize();
					}//only south
				}
				else
				{
					mynewx = inf.cage_coord(odNorth) + inf.rc(odNorth);
				}//else no south
				m_newx[v] = mynewx;
				inf.set_coord(odNorth, mynewx);
				inf.set_coord(odSouth, mynewx + inf.node_xsize());
			}//if horizontalmerger
			//we place the node at the position cage_left_border(v)+routing_channel_left(v)
			//should be handled by value assign
			else
			{
				m_newx[v] = inf.cage_coord(odSouth) - inf.rc(odSouth) - inf.node_xsize();
				inf.set_coord(odNorth, m_newx[v]);
				inf.set_coord(odSouth, m_newx[v] + inf.node_xsize());
			}
		}//else merger
	}//else vert

	if (m_mergerSon[v])
	{
		if (vertical_merger) //change vertical position
		{
			//test incons.
			OGDF_ASSERT(!horizontal_merger);
			//we have to know the exact direction of the edge to the merger
			OGDF_ASSERT( (m_mergeDir[v] == odNorth) || (m_mergeDir[v] == odSouth));
			if (m_mergeDir[v] == odNorth)
				m_newy[v] = (inf.cage_coord(odEast) - inf.node_ysize() - inf.rc(odEast));
			else
				m_newy[v] = (inf.cage_coord(odWest) + inf.rc(odWest));
			inf.set_coord(odWest, m_newy[v]);
			inf.set_coord(odEast, m_newy[v] + inf.node_ysize());
		}
		if (horizontal_merger)
		{
			//test inconsistency
			OGDF_ASSERT(!vertical_merger);
			//we have to know the exact direction of the edge to the merger
			OGDF_ASSERT( (m_mergeDir[v] == odEast) || (m_mergeDir[v] == odWest));
			if (m_mergeDir[v] == odWest)
				m_newx[v] = inf.cage_coord(odNorth) + inf.rc(odNorth);
			else
				m_newx[v] = inf.cage_coord(odSouth) - inf.rc(odSouth) - inf.node_xsize();
			inf.set_coord(odNorth, m_newx[v]);
			inf.set_coord(odSouth, m_newx[v] + inf.node_xsize());
		}
	}//if genmergeson
	//ende test

	//now we have vertical as well as horizontal position and can assign both values
	if (horizontal_merger)
		compute_gen_glue_points_y(v);
	else compute_glue_points_y(v);
	if (vertical_merger)
		compute_gen_glue_points_x(v);
	else compute_glue_points_x(v);
		set_corners(v);
	//we computed the new placement position for original node box v and stored it in m_new as well as in inf (debug)
	//assert some assigment
	//{
	//	if ( (m_prup->expandAdj(v) != 0) && (m_prup->typeOf(v) != Graph::generalizationMerger) )
	//	{
	//		OGDF_ASSERT( (m_newx[v] >= inf.cage_coord(odNorth)) && (m_newy[v] >= inf.cage_coord(odWest)) );
	//		OGDF_ASSERT( (m_newx[v] + inf.node_xsize()<= inf.cage_coord(odSouth)) &&
 //                   (m_newy[v] + inf.node_ysize()<= inf.cage_coord(odEast)));
	//	}
	//}

	//now assign boolean values for reroutability: is edge to box distance large enough to allow rerouting
	//{} is done in call function in classify_edges()

	//we computed the new placement position for original node box v
	//and stored it in m_newxy as well as in inf (debug)
}//compute_place


//************************************************************************
//REAL PLACEMENT Change graph based on placement/rerouting
void EdgeRouter::place(node l_v)
{
	//two steps: first, introduce the bends on the incoming edges,
	// normalise, then adjust the layout information for the cage nodes and bend nodes
	String m;
	String msg;
	OrthoRep::VertexInfoUML* vinfo = m_orp->cageInfo(l_v);
	edge e;
	bool inedge;
	bool corn, acorn; //test on last and first corner transition after rerouting
	//forall four sides check the edges

	//NORTH SIDE ***************************************************
	//integrate offset for double bend edges without bendfree edges because of rerouting
	int leftofs = (infos[l_v].num_bend_free(odNorth) ? 0 :
		infos[l_v].delta(odNorth, odWest)*infos[l_v].flips(odWest, odNorth));
	int rightofs = (infos[l_v].num_bend_free(odNorth) ? 0 :
		infos[l_v].delta(odNorth, odEast)*infos[l_v].flips(odEast, odNorth));

	List<edge>& inlist = infos[l_v].inList(odNorth); //left side
	ListIterator<edge> it = inlist.begin();
	int ipos = 0;
	corn = false; //check if end corner changed after necessary rerouting
	acorn = false; //check if start corner changed after necessary re

	while (it.valid())
	{
		e = *it;
		node v;
		adjEntry ae; //"outgoing" from cage
		inedge = infos[l_v].is_in_edge(odNorth, ipos);
		if (inedge)
		{
			ae = e->adjTarget();
		}
		else
		{
			ae = e->adjSource();
		}
		v = m_cage_point[ae];

		if (m_processStatus[v] == used)
		{
			it++;//should be enough to break
			continue;
		}//if degree1 preprocessing on opposite side
		adjEntry cornersucc = ae->cyclicSucc();
		adjEntry saveadj = ae;
		//correct possible shift by bends from other cage
		if (!((inedge && (v == e->target()))
			|| ((v == e->source()) && !inedge) ))
		{
			edge run = e;
			if (inedge)
			{
				adjEntry runadj = run->adjSource();
				while (v != runadj->theEdge()->target()) {
					runadj = runadj->faceCycleSucc();
				}
				e = runadj->theEdge();
				cornersucc = runadj->twin()->cyclicSucc();
				OGDF_ASSERT(v == cornersucc->theNode());
				saveadj = runadj->twin();
			}
			OGDF_ASSERT(((v == e->target()) && (inedge)) ||(v == e->source()));
		}//if buggy
		//position
		if ((m_agp_x[ae] != m_init) &&  (m_agp_y[ae] != m_init))
			set_position(v, m_agp_x[ae], m_agp_y[ae]);
		OGDF_ASSERT((m_agp_y[ae] != m_init) && (m_agp_x[ae] != m_init));

		//bends
		if (abendType(ae) != bend_free)
		{
			edge newe;
			node newbend, newglue;
			int xtacy;
			switch (abendType(ae))
			{
				//case prob_b1l:
				case bend_1left: //rerouted single bend
					//delete old corner
					if (ipos == 0)
					{
						OGDF_ASSERT(infos[l_v].flips(odNorth, odWest) > 0);
						adjEntry ae2 = saveadj->cyclicPred();//ae->cyclicPred(); //aussen an cage
						adjEntry ae3 = ae2->faceCycleSucc();
						//as long as unsplit does not work, transition
						//node oldcorner;
						if (ae2 == (ae2->theEdge()->adjSource()))
						{
							//oldcorner = ae2->theEdge()->target();
							unsplit(ae2->theEdge(), ae3->theEdge());
						}
						else
						{
							unsplit(ae3->theEdge(), ae2->theEdge());
							//oldcorner = ae2->theEdge()->source();
						}
					}//if pos==0
					//delete corner after last rerouting
					if ((!acorn) && (ipos == infos[l_v].flips(odNorth, odWest) - 1))
					{
						acorn = true;
						edge newe2;
						adjEntry ae2 = saveadj->cyclicSucc();//cornersucc;//ae->cyclicSucc();//next to the right in cage
						edge savedge = ae2->theEdge();
						if (ae2 == (ae2->theEdge()->adjSource()))
						{
							newe2 = addRightBend(ae2->theEdge());
							vinfo->m_corner[0] = newe2->adjSource();
						}
						else {
							newe2 = addLeftBend(ae2->theEdge());
							vinfo->m_corner[0] = savedge->adjTarget();
						}
						fix_position(newe2->source(), infos[l_v].coord(odNorth), infos[l_v].coord(odWest));
						m_prup->setExpandedNode(newe2->source(), l_v);
					}

					xtacy = infos[l_v].coord(odNorth)
						+ infos[l_v].delta(odWest, odNorth)
						*(infos[l_v].flips(odNorth, odWest) - 1 - ipos)
						+ infos[l_v].eps(odWest, odNorth);
					if (inedge) newe = addLeftBend(e);
					else newe = addRightBend(e);
					newbend = newe->source();

					if (inedge) newglue = newe->target();
					else newglue = e->source();
					fix_position(newglue, xtacy, infos[l_v].coord(odWest));
					fix_position(newbend, xtacy, cp_y(ae));
					break;
				case prob_b1l:
				case prob_b2l:
				case bend_2left:
					//adjust glue point to rerouted edges
					fix_position(v, m_agp_x[ae], m_agp_y[ae]+leftofs);
					xtacy = infos[l_v].cage_coord(odNorth)
						+ (infos[l_v].num_bend_edges(odNorth, odWest) -ipos)*m_sep;
					newe = addLeftBend(e);
					fix_position(newe->source(), xtacy, (inedge ? cp_y(ae) : (gp_y(ae)+leftofs)));
					newe = addRightBend(newe);
					fix_position(newe->source(), xtacy, (inedge ? (gp_y(ae)+leftofs) : cp_y(ae)));
					break;//int bend downwards
				case bend_1right:
					if (!corn)
					{
						corn = true;
						edge newe2;
						adjEntry ae2 = saveadj->cyclicPred();//ae->cyclicPred();
						edge savedge = ae2->theEdge();
						if (ae2 == (ae2->theEdge()->adjSource()))
						{
							newe2 = addLeftBend(ae2->theEdge());
							vinfo->m_corner[1] = savedge->adjTarget();
						}
						else {
							newe2 = addRightBend(ae2->theEdge());
							vinfo->m_corner[1] = newe2->adjSource();
						}
						fix_position(newe2->source(), infos[l_v].coord(odNorth), infos[l_v].coord(odEast));
						m_prup->setExpandedNode(newe2->source(), l_v);
					}
					//delete corner after last rerouting
					if (corn && (ipos == infos[l_v].inList(odNorth).size()-1))
					{
						adjEntry ae2 = saveadj->cyclicSucc();//ae->cyclicSucc();
						adjEntry ae3 = ae2->faceCycleSucc();
						if (ae2 == (ae2->theEdge()->adjSource()))
							unsplit(ae2->theEdge(), ae3->theEdge());
						else
							unsplit(ae3->theEdge(), ae2->theEdge());
					}//if last
					xtacy = infos[l_v].coord(odNorth)
						+ (ipos + infos[l_v].flips(odNorth, odEast) - infos[l_v].inList(odNorth).size())
						* infos[l_v].delta(odEast, odNorth) + infos[l_v].eps(odEast, odNorth);
					if (inedge) newe = addRightBend(e);
					else newe = addLeftBend(e);
					newbend = newe->source();

					if (inedge) newglue = newe->target();
					else newglue = e->source();
					fix_position(newbend, xtacy, cp_y(ae));
					fix_position(newglue, xtacy, infos[l_v].coord(odEast));
					break;//rerouted single bend
				case prob_b1r:
				case prob_b2r:
				case bend_2right:
					//adjust glue point to rerouted edges
					fix_position(v, m_agp_x[ae], m_agp_y[ae]-rightofs);
					xtacy = infos[l_v].cage_coord(odNorth)
						+ (1 + ipos + infos[l_v].num_bend_edges(odNorth, odEast) - infos[l_v].inList(odNorth).size())
						* m_sep;
					//* infos[l_v].delta(odNorth, odEast);
					newe = addRightBend(e);
					fix_position(newe->source(), xtacy, (inedge ? cp_y(ae) : (gp_y(ae)-rightofs)));
					newe = addLeftBend(newe);
					fix_position(newe->source(), xtacy, (inedge ? (gp_y(ae)-rightofs) : cp_y(ae)));
					break; //double bend upwards
				default: break;
			}//switch
			m_orp->normalize();
		}//if not bendfree
		ipos++;
		it++;
	}//while

	//*********************************************
	//EAST SIDE (bottom)************************************
	leftofs = (infos[l_v].num_bend_free(odEast) ? 0 :
		infos[l_v].delta(odEast, odNorth)*infos[l_v].flips(odNorth, odEast));
		//temp
		//leftofs = infos[l_v].delta(odEast, odNorth)*infos[l_v].flips(odNorth, odEast);
		//temp end
	rightofs = (infos[l_v].num_bend_free(odEast) ? 0 :
		infos[l_v].delta(odEast, odSouth)*infos[l_v].flips(odSouth, odEast));
		//temp
		//rightofs = infos[l_v].delta(odEast, odSouth)*infos[l_v].flips(odSouth, odEast);
		//tempend

	it = infos[l_v].inList(odEast).begin();
	ipos = 0;
	corn = false; //check if end corner changed after necessary rerouting
	acorn = false; //check if start corner changed after necessary re
	while (it.valid())
	{
		e = *it;
		node v;
		adjEntry ae; //"outgoing" from cage
		inedge = infos[l_v].is_in_edge(odEast, ipos);
		if (inedge)
		{
			ae = e->adjTarget();
		}
		else
		{
			ae = e->adjSource();
		}
		v = m_cage_point[ae];
		if (m_processStatus[v] == used)
		{
			it++;//should be enough to break
			continue;
		}//if degree1 preprocessing on opposite side
		adjEntry cornersucc = ae->cyclicSucc();
		adjEntry adjsave = ae;
		//correct possible shift by bends from other cage
		if (!((inedge && (v == e->target()))
			|| ((v == e->source()) && !inedge) ))
		{
			edge run = e;
			if (inedge)
			{
				adjEntry runadj = run->adjSource();
				while (v != runadj->theEdge()->target()) {runadj = runadj->faceCycleSucc(); }
				e = runadj->theEdge();
				cornersucc = runadj->twin()->cyclicSucc();
				OGDF_ASSERT(v == cornersucc->theNode());
				adjsave = runadj->twin();
			}
			OGDF_ASSERT((v == e->target()) ||(v == e->source()));
		}//if buggy
		OGDF_ASSERT((v == e->target()) ||(v == e->source()));
		//position
		if ((m_agp_x[ae] != m_init) && (m_agp_y[ae] != m_init)) set_position(v, m_agp_x[ae], m_agp_y[ae]);
		OGDF_ASSERT((m_agp_y[ae] != m_init) && (m_agp_x[ae] != m_init))
		//bends
		if (abendType(ae) != bend_free)
		{
			switch (abendType(ae))
			{
				edge newe;
				node newbend, newglue;
				int ypsiqueen;
				//case prob_b1l:
				case bend_1left:
					//set new corner before first rerouting
					//delete corner before starting
					if (ipos == 0)
					{
						adjEntry ae2 = adjsave->cyclicPred();  //ae->cyclicPred(); //aussen an cage
						adjEntry ae3 = ae2->faceCycleSucc();

						if (ae2 == (ae2->theEdge()->adjSource()))
							unsplit(ae2->theEdge(), ae3->theEdge());
						else
							unsplit(ae3->theEdge(), ae2->theEdge());
						}
						if ((!acorn) && (ipos == infos[l_v].flips(odEast, odNorth) - 1))
						{

						acorn = true;
						edge newe2;
						adjEntry ae2 = adjsave->cyclicSucc();//next to the right in cage
						edge savedge = ae2->theEdge();
						if (ae2 == (ae2->theEdge()->adjSource()))
						{
							newe2 = addRightBend(ae2->theEdge());
							vinfo->m_corner[1] = newe2->adjSource();
						}
						else {
							newe2 = addLeftBend(ae2->theEdge());
							vinfo->m_corner[1] = savedge->adjTarget();
						}
						fix_position(newe2->source(), infos[l_v].coord(odNorth), infos[l_v].coord(odEast));
						m_prup->setExpandedNode(newe2->source(), l_v);
					}
					if (inedge) newe = addLeftBend(e); //abhaengig von inedge, ??
					else newe = addRightBend(e);
					newbend = newe->source();

					if (inedge) newglue = newe->target();
					else newglue = e->source();
					ypsiqueen = infos[l_v].coord(odEast) - (infos[l_v].flips(odEast, odNorth) - ipos - 1)
						* infos[l_v].delta(odNorth, odEast) - infos[l_v].eps(odNorth, odEast);
					fix_position(newbend, cp_x(ae), ypsiqueen);
					fix_position(newglue, infos[l_v].coord(odNorth), ypsiqueen);
					//target oder source von newe coord
					break; //rerouted single bend
				case prob_b1l:
				case prob_b2l:
				case bend_2left:
					//adjust glue point to rerouted edges
					fix_position(v, m_agp_x[ae]+leftofs, m_agp_y[ae]);
					ypsiqueen = infos[l_v].cage_coord(odEast)
							 - (infos[l_v].num_bend_edges(odEast, odNorth) - ipos)*m_sep;
					newe = addLeftBend(e);
					fix_position(newe->source(),(inedge ? cp_x(ae) : m_agp_x[ae]+leftofs) ,ypsiqueen);
					newe = addRightBend(newe);
					fix_position(newe->source(),(inedge ? m_agp_x[ae] + leftofs : cp_x(ae)) ,ypsiqueen);
					break;//double bend downwards
				//case prob_b1r:
				case bend_1right:
					//set new corner before first rerouting
					if (!corn)
					{
						corn = true;
						edge newe2;
						adjEntry ae2 = adjsave->cyclicPred();
						edge le = ae2->theEdge();
						if (ae2 == (ae2->theEdge()->adjSource()))
						{
							newe2 = addLeftBend(ae2->theEdge());
							vinfo->m_corner[2] = le->adjTarget();
						}
						else {
							newe2 = addRightBend(ae2->theEdge());
							vinfo->m_corner[2] = newe2->adjSource();
						}
						fix_position(newe2->source(),infos[l_v].coord(odSouth), infos[l_v].coord(odEast));
						m_prup->setExpandedNode(newe2->source(), l_v);
					}
					ypsiqueen = infos[l_v].coord(odEast) - (ipos + infos[l_v].flips(odEast, odSouth) - infos[l_v].inList(odEast).size())
												* infos[l_v].delta(odSouth, odEast) - infos[l_v].eps(odSouth, odEast);
					//delete corner after last rerouting
					if (corn && (ipos == infos[l_v].inList(odEast).size() - 1))
					{
						adjEntry ae2 = adjsave->cyclicSucc();//ae->cyclicSucc();
						adjEntry ae3 = ae2->faceCycleSucc();
						//as long as unsplit does not work, transition
						//node oldcorner;
						if (ae2 == (ae2->theEdge()->adjSource()))
						{
							//oldcorner = ae2->theEdge()->target();
							unsplit(ae2->theEdge(), ae3->theEdge());
						}
						else
						{
							unsplit(ae3->theEdge(), ae2->theEdge());
							//oldcorner = ae2->theEdge()->source();
						}
					}
					if (inedge) newe = addRightBend(e);
					else newe = addLeftBend(e);
					newbend = newe->source();

					if (inedge) newglue = newe->target();
					else newglue = e->source();

					fix_position(newbend, cp_x(ae), ypsiqueen);
					fix_position(newglue, infos[l_v].coord(odSouth), ypsiqueen);
					break;//rerouted single bend
				case prob_b1r:
				case prob_b2r:
				case bend_2right:
					//adjust glue point to rerouted edges
					fix_position(v, m_agp_x[ae]-rightofs, m_agp_y[ae]);
					ypsiqueen = infos[l_v].cage_coord(odEast) -
							 (ipos - infos[l_v].inList(odEast).size() + infos[l_v].num_bend_edges(odEast, odSouth))*m_sep;
							 //(infos[l_v].inList(odEast).size() + infos[l_v].flips(odEast, odSouth) - ipos)*m_sep;
					newe = addRightBend(e);
					fix_position(newe->source(), (inedge ? cp_x(ae) : m_agp_x[ae]-rightofs), ypsiqueen);
					newe = addLeftBend(newe);
					fix_position(newe->source(), (inedge ? m_agp_x[ae]-rightofs : cp_x(ae)), ypsiqueen);
					break; //double bend upwards
				default: break;
			}//switch
			m_orp->normalize();
		}//if
		ipos++;
		it++;
	}//while

	//*****************************************************
	//SOUTH SIDE*******************************************
	leftofs = (infos[l_v].num_bend_free(odSouth) ? 0 :
		infos[l_v].delta(odSouth, odEast)*infos[l_v].flips(odEast, odSouth));
	rightofs = (infos[l_v].num_bend_free(odSouth) ? 0 :
		infos[l_v].delta(odSouth, odWest)*infos[l_v].flips(odWest, odSouth));
	it = infos[l_v].inList(odSouth).begin();
	ipos = 0;
	corn = false; //check if end corner changed after necessary rerouting
	acorn = false; //check if start corner changed after necessary re
	while (it.valid() && (infos[l_v].inList(odSouth).size() > 0))
	{
		e = *it;
		node v;//test
		adjEntry ae; //"outgoing" from cage
		inedge = infos[l_v].is_in_edge(odSouth, ipos);
		if (inedge)
		{
			ae = e->adjTarget();
		}
		else
		{
			ae = e->adjSource();
		}
		v = m_cage_point[ae];
		if (m_processStatus[v] == used)
		{
			it++;//should be enough to break
			continue;
		}//if degree1 preprocessing on opposite side
		adjEntry cornersucc = ae->cyclicSucc();
		adjEntry saveadj = ae;
		//correct possible shift by bends from other cage
		if (!((inedge && (v == e->target()))
			|| ((v == e->source()) && !inedge) ))
		{
			edge run = e;
			if (inedge)
			{
				adjEntry runadj = run->adjSource();
				while (v != runadj->theEdge()->target()) {runadj = runadj->faceCycleSucc();}
				e = runadj->theEdge();
				cornersucc = runadj->twin()->cyclicSucc();
				OGDF_ASSERT(v == cornersucc->theNode());
				saveadj = runadj->twin();
			}
			OGDF_ASSERT((v == e->target()) ||(v == e->source()));
		}//if buggy
		//position
		if ((m_agp_x[ae] != m_init) && (m_agp_y[ae] != m_init)) set_position(v, m_agp_x[ae], m_agp_y[ae]);
		OGDF_ASSERT((m_agp_x[ae] != m_init) && (m_agp_y[ae] != m_init))
		//bends
		if (abendType(ae) != bend_free)
		{
			edge newe;
			node newbend, newglue;
			int xtacy;
			switch (abendType(ae))
			{
				case bend_1left:
					if ((!corn))
					{
						corn = true;
						edge newe2;
						adjEntry ae2 = saveadj->cyclicSucc();//next to the right in cage
						edge savedge = ae2->theEdge();
						if (ae2 == (ae2->theEdge()->adjSource()))
						{
							newe2 = addRightBend(ae2->theEdge());
							vinfo->m_corner[2] = newe2->adjSource();
						}
						else {
							newe2 = addLeftBend(ae2->theEdge());
							vinfo->m_corner[2] = savedge->adjTarget();
						}
						fix_position(newe2->source(), infos[l_v].coord(odSouth), infos[l_v].coord(odEast));
						m_prup->setExpandedNode(newe2->source(), l_v);
					}
					//delete corner after last rerouting
					if (ipos == infos[l_v].inList(odSouth).size()-1)
					{
						adjEntry ae2 = saveadj->cyclicPred();//ae->cyclicPred();
						adjEntry ae3 = ae2->faceCycleSucc();
						if (ae2 == (ae2->theEdge()->adjSource()))
							unsplit(ae2->theEdge(), ae3->theEdge());
						else
							unsplit(ae3->theEdge(), ae2->theEdge());
					}//if last
					xtacy = infos[l_v].coord(odSouth)
						- infos[l_v].delta(odEast, odSouth)
						*(infos[l_v].flips(odSouth, odEast) + ipos - infos[l_v].inList(odSouth).size())
						-infos[l_v].eps(odEast, odSouth);
					if (inedge) newe = addLeftBend(e);
					else newe = addRightBend(e);
					newbend = newe->source();

					if (inedge) newglue = newe->target();
					else newglue = e->source();
					fix_position(newglue, xtacy, infos[l_v].coord(odEast));
					fix_position(newbend, xtacy, cp_y(ae));
					break; //rerouted single bend
				case prob_b1l:
				case prob_b2l:
				case bend_2left:
					//adjust glue point to rerouted edges
					fix_position(v, m_agp_x[ae], m_agp_y[ae]-leftofs);
					xtacy = infos[l_v].cage_coord(odSouth)
						- (ipos + 1 + infos[l_v].num_bend_edges(odSouth, odEast)
						- infos[l_v].inList(odSouth).size())*m_sep;
					newe = addLeftBend(e);

					//gy = gp_y(ae) - infos[l_v].flips(odEast, odSouth)*infos[l_v].delta(odSouth, odEast);
					//if (inedge) fix_position(e->target(), m_agp_x[ae], gy);
					//else fix_position(e->source(), m_agp_x[ae], gy);
					fix_position(newe->source(), xtacy, (inedge ? cp_y(ae) : gp_y(ae)-leftofs));
					newe = addRightBend(newe);
					fix_position(newe->source(), xtacy, (inedge ? gp_y(ae)-leftofs : cp_y(ae)));
					break;//double bend downwards
				//case prob_b1r:
				case bend_1right:
					//delete corner before rerouting
					if (ipos == 0)
					{
						adjEntry ae2 = saveadj->cyclicSucc();//cornersucc;
						adjEntry ae3 = ae2->faceCycleSucc();

						//node oldcorner;
						if (ae2 == (ae2->theEdge()->adjSource()))
						{
							//oldcorner = ae2->theEdge()->target();
							unsplit(ae2->theEdge(), ae3->theEdge());
						}
						else
						{
							unsplit(ae3->theEdge(), ae2->theEdge());
							//oldcorner = ae2->theEdge()->source();
						}
					}//if
					//last flipped edge, insert sw - corner node
					if ((!acorn) && (ipos == infos[l_v].flips(odSouth, odWest) - 1))
					{
						acorn = true;
						edge newe2;
						adjEntry ae2 = saveadj->cyclicPred();
						edge le = ae2->theEdge();
						if (ae2 == (ae2->theEdge()->adjSource()))
						{
							newe2 = addLeftBend(ae2->theEdge());
							vinfo->m_corner[3] = le->adjTarget();
						}
						else {
							newe2 = addRightBend(ae2->theEdge());
							vinfo->m_corner[3] = newe2->adjSource();
						}
						fix_position(newe2->source(), infos[l_v].coord(odSouth), infos[l_v].coord(odWest));
						m_prup->setExpandedNode(newe2->source(), l_v);
					}

					if (inedge) newe = addRightBend(e);
					else newe = addLeftBend(e);
					newbend = newe->source();
					//source ist immer neuer Knick, aber target haengt von der Richtung ab...
					if (inedge) newglue = newe->target();
					else newglue = e->source();
					xtacy = infos[l_v].coord(odSouth) - (infos[l_v].flips(odSouth, odWest) - ipos - 1)
						* infos[l_v].delta(odWest, odSouth) - infos[l_v].eps(odWest, odSouth);
					fix_position(newbend, xtacy, cp_y(ae));
					fix_position(newglue, xtacy, infos[l_v].coord(odWest));
					break;//rerouted single bend
				case prob_b1r:
				case prob_b2r:
				case bend_2right:
					//adjust glue point to rerouted edges
					fix_position(v, m_agp_x[ae], m_agp_y[ae]+rightofs);
					xtacy = infos[l_v].cage_coord(odSouth) -
						(infos[l_v].num_bend_edges(odSouth, odWest) - ipos) * m_sep;
					newe = addRightBend(e);
					fix_position(newe->source(), xtacy,(inedge ? cp_y(ae) : m_agp_y[ae]+rightofs ));
					newe = addLeftBend(newe);
					fix_position(newe->source(), xtacy, (inedge ? m_agp_y[ae]+rightofs : cp_y(ae)));
					break; //double bend downwards
				default: break;
			}//switch
			m_orp->normalize();
		}//if
		ipos++;
		it++;
	}//while

	//*****************************************************
	//WEST SIDE******************************************
	leftofs = (infos[l_v].num_bend_free(odWest) ? 0 :
		infos[l_v].delta(odWest, odSouth)*infos[l_v].flips(odSouth, odWest));
	rightofs = (infos[l_v].num_bend_free(odWest) ? 0 :
		infos[l_v].delta(odWest, odNorth)*infos[l_v].flips(odNorth, odWest));
	it = infos[l_v].inList(odWest).begin();
	ipos = 0;
	corn = acorn = false;
	while (it.valid())
	{
		e = *it;
		node v;
		adjEntry ae; //"outgoing" from cage
		inedge = infos[l_v].is_in_edge(odWest, ipos);
		if (inedge)
		{
			ae = e->adjTarget();
		}
		else
		{
			ae = e->adjSource();
		}
		v = m_cage_point[ae];
		if (m_processStatus[v] == used)
		{
			it++;//should be enough to break
			continue;
		}//if degree1 preprocessing on opposite side
		//position
		adjEntry cornersucc = ae->cyclicSucc();
		adjEntry saveadj = ae;
		//correct possible shift by bends from other cage
		if (!((inedge && (v == e->target()))
		|| ((v == e->source()) && !inedge) ))
		{
			edge run = e;
			if (inedge)
			{
				adjEntry runadj = run->adjSource();
				while (v != runadj->theEdge()->target()) {runadj = runadj->faceCycleSucc(); }
				e = runadj->theEdge();
				cornersucc = runadj->twin()->cyclicSucc();
				OGDF_ASSERT(v == cornersucc->theNode());
				saveadj = runadj->twin();
			}
			OGDF_ASSERT((v == e->target()) ||(v == e->source()));
		}//if buggy
		//position
		if ((m_agp_x[ae] != m_init) && (m_agp_y[ae] != m_init)) set_position(v, m_agp_x[ae], m_agp_y[ae]);
		OGDF_ASSERT((m_agp_x[ae] != m_init) && (m_agp_y[ae] != m_init));
		//bends
		if (abendType(ae) != bend_free)
		{
			edge newe;
			node newbend, newglue;
			int ypsiqueen;
			switch (abendType(ae))
			{
				case bend_1left:
					//node right side
					//set new corner before first rerouting
					if (!acorn)
					{
						acorn = true;
						edge newe2;
						adjEntry ae2 = saveadj->cyclicSucc();     // cornersucc;//next to the right in cage
						edge savedge = ae2->theEdge();
						if (ae2 == (ae2->theEdge()->adjSource()))
						{
							newe2 = addRightBend(ae2->theEdge());
							vinfo->m_corner[3] = newe2->adjSource();
						}
						else {
							newe2 = addLeftBend(ae2->theEdge());
							vinfo->m_corner[3] = savedge->adjTarget();
						}
						fix_position(newe2->source(), infos[l_v].coord(odSouth), infos[l_v].coord(odWest));
						m_prup->setExpandedNode(newe2->source(), l_v);
					}
					//delete corner after last rerouting
					if (acorn && (ipos == infos[l_v].inList(odWest).size()-1))
					{
						adjEntry ae2 = saveadj->cyclicPred(); //ae->cyclicPred(); //aussen an cage
						adjEntry ae3 = ae2->faceCycleSucc();
						if (ae2 == (ae2->theEdge()->adjSource()))
							unsplit(ae2->theEdge(), ae3->theEdge());
						else
							unsplit(ae3->theEdge(), ae2->theEdge());
					}//if
					//
					if (inedge) newe = addLeftBend(e);
					else newe = addRightBend(e);
					newbend = newe->source();

					if (inedge) newglue = newe->target();
					else newglue = e->source();
					ypsiqueen = infos[l_v].coord(odWest)
										 + (infos[l_v].flips(odWest, odSouth) + ipos - infos[l_v].inList(odWest).size())
										 *infos[l_v].delta(odSouth, odWest)
										 +infos[l_v].eps(odSouth, odWest);
					fix_position(newbend, cp_x(ae), ypsiqueen);
					fix_position(newglue, infos[l_v].coord(odSouth), ypsiqueen);
					break; //rerouted single bend
				case prob_b1l:
				case prob_b2l:
				case bend_2left:
					//adjust glue point to rerouted edges
					fix_position(v, m_agp_x[ae]-leftofs, m_agp_y[ae]);
					ypsiqueen = infos[l_v].cage_coord(odWest)
						+ (ipos + 1 + infos[l_v].num_bend_edges(odWest, odSouth)
						- infos[l_v].inList(odWest).size())*m_sep;
					//coord - (-infos[l_v].flips(odWest, odSouth) - ipos  + infos[l_v].inList(odWest).size())*m_sep;
					newe = addLeftBend(e);
					fix_position(newe->source(), (inedge ? cp_x(ae) : m_agp_x[ae]-leftofs), ypsiqueen);
					newe = addRightBend(newe);
					fix_position(newe->source(), (inedge ? m_agp_x[ae]-leftofs : cp_x(ae)), ypsiqueen);
					break;//double bend downwards
				//case prob_b1r:
				case bend_1right:
					//set new corner before first rerouting

					//delete corner after last rerouting
					if (ipos == 0)//(corn && (ipos == infos[l_v].inList(odWest).size()-1))
					{
						adjEntry ae2 = saveadj->cyclicSucc();   //cornersucc;//ae->cyclicSucc();
						adjEntry ae3 = ae2->faceCycleSucc();
						//as long as unsplit does not work, transition
						//node oldcorner;
						if (ae2 == (ae2->theEdge()->adjSource()))
						{
							//oldcorner = ae2->theEdge()->target();
							unsplit(ae2->theEdge(), ae3->theEdge());
						}
						else  {
							unsplit(ae3->theEdge(), ae2->theEdge());
							//oldcorner = ae2->theEdge()->source();
						}
					}
					if ((!corn) && (ipos == infos[l_v].flips(odWest, odNorth) - 1))
					{
						corn = true;
						edge newe2;
						adjEntry ae2 = saveadj->cyclicPred();//ae->cyclicPred();
						edge savedge = ae2->theEdge();
						if (ae2 == (ae2->theEdge()->adjSource()))
						{
							newe2 = addLeftBend(ae2->theEdge());
							vinfo->m_corner[0] = savedge->adjTarget();
						}
						else {
							newe2 = addRightBend(ae2->theEdge());
							vinfo->m_corner[0] = newe2->adjSource();
						}
						fix_position(newe2->source(), infos[l_v].coord(odNorth), infos[l_v].coord(odWest));
						m_prup->setExpandedNode(newe2->source(), l_v);
					}
					if (inedge) newe = addRightBend(e);
					else newe = addLeftBend(e);
					newbend = newe->source();

					if (inedge) newglue = newe->target();
					else newglue = e->source();
					ypsiqueen = infos[l_v].coord(odWest) + (infos[l_v].flips(odWest, odNorth) - ipos - 1)
						* infos[l_v].delta(odNorth, odWest) + infos[l_v].eps(odNorth, odWest);
					fix_position(newbend, cp_x(ae), ypsiqueen);
					fix_position(newglue, infos[l_v].coord(odNorth), ypsiqueen);
					break;//rerouted single bend
				case prob_b1r:
				case prob_b2r:
				case bend_2right:
					//adjust glue point to rerouted edges
					fix_position(v, m_agp_x[ae]+rightofs, m_agp_y[ae]);
					ypsiqueen = infos[l_v].cage_coord(odWest)
						 + (infos[l_v].num_bend_edges(odWest, odNorth) - ipos)*m_sep;
					//infos[l_v].coord(odWest) - ((ipos + 1) - infos[l_v].flips(odNorth, odWest)) * m_sep;
					newe = addRightBend(e);
					fix_position(newe->source(), (inedge ? cp_x(ae) : m_agp_x[ae]+rightofs), ypsiqueen);
					newe = addLeftBend(newe);

					fix_position(newe->source(), (inedge ? m_agp_x[ae]+rightofs : cp_x(ae)), ypsiqueen);
					break; //double bend upwards
				default: break;
			}//switch
			m_orp->normalize();
		}//if

		ipos++;
		it++;

	}//while

	//m_prup->writeGML(String("placeInput%d.gml",l_v->index()), *m_orp, *m_layoutp);
	//OGDF_ASSERT(m_orp->check(msg));
	//***********************************
}//place


//given a replacement cage (defining routing channels??)
//and a box placement, compute a bend-minimising routing
//box placement is in m_newx[v], m_newy[v] for left lower corner
//now it is in NodeInfo, m_new may be obs
//check rounding !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
void EdgeRouter::compute_routing(node v) //reroute(face f)
{
	//compute for each box corner the possible reroutings

	//first, define some values
	//find the uppermost/lowest unbend edge on the left and right side
	//and the rightmost/leftmost unbend edge on the top and bottom side
	//use values stored in inf
	//******************************************************************
	//alpha used in move - functions, variable al_12 means:
	//max. number of edges to be moved from side 2 to side 1
	//eight cases, two for ev corner
	//1space left for edges moved from top to left side
	int al_lt = alpha_move(odNorth, odEast, v);
	//2space left for edges moved from left to top side
	int al_tl = alpha_move(odEast, odNorth, v);
	//3space left for edges moved from left to bottom side
	//int al_bl = alpha_move(odWest, odNorth, v);
	//4Space left for edges moved from bottom side to left side
	//int al_lb = alpha_move(odNorth, odWest, v);
	//5space left for edges moved from right to top side
	//int al_tr = alpha_move(odEast, odSouth, v);
	//6space left for edges moved from top to right side
	int al_rt = alpha_move(odSouth, odEast, v);
	//7space left for edges moved from right to bottom side
	int al_br = alpha_move(odWest, odSouth, v);
	//8space left for edges moved from bottom to right side
	//int al_rb = alpha_move(odSouth, odWest, v);

	//*******************************************************************

	//*******************************************************************
	//beta used in move functions
	//be_12k defines the number of edges with preliminary bends
	//but connection point in the glue point area, so that
	//they could be routed bendfree if k (enough) edges are moved from
	//side 1 to side 2
	//k is computed as the minimum of al_21 and card(E^12) (rerouteable edges)
	//beta is only used internally to compute bend save
	//save_12 holds the number of saved bends by moving edges from 1 to 2

	//flip_12 contains the number of edges flipped from
	//side 1 to 2 and flip_21 ~
	//from side 2 to 1 (only one case is possible)

	//algorithm 3 in the paper

	//This means that for two sides with equal value, our algorithm always
	//prefers one (the same) above the other

	//top-left
	int flip_tl, flip_lt;
	if (compute_move(odEast, odNorth, flip_tl, v) < compute_move(odNorth, odEast, flip_lt, v))
		flip_tl = 0;
	else flip_lt = 0;
	//left-bottom
	int flip_lb, flip_bl;
	if (compute_move(odNorth, odWest, flip_lb, v) < compute_move(odWest, odNorth, flip_bl, v))
		flip_lb = 0;
	else flip_bl = 0;
	//top-right
	int flip_tr, flip_rt;
	if (compute_move(odEast, odSouth, flip_tr, v) < compute_move(odSouth, odEast, flip_rt, v))
		flip_tr = 0;
	else flip_rt = 0;
	//bottom-right
	int flip_br, flip_rb;
	if (compute_move(odWest, odSouth, flip_br, v) < compute_move(odSouth, odWest, flip_rb, v))
		flip_br = 0;
	else flip_rb = 0;
	//cout<<"resulting flip numbers: "<<flip_tl<<"/"<<flip_tr<<"/"<<flip_bl<<"/"<<flip_br<<"/"
	//	  <<flip_rt<<"/"<<flip_rb<<"/"<<flip_lt<<"/"<<flip_lb<<"\n";
	//if there are no bendfree edges on side s, we have
	//to assure that the edges moved in from both sides
	//dont take up too much space, simple approach: decrease
	{
		int surplus; //flippable edges without enough space to place
		if (infos[v].num_bend_free(odEast) == 0)//top, contributing neigbours are left/right
		{
			surplus = flip_lt + flip_rt - al_tl;
			if (surplus > 0)
			{
				flip_lt -= int(floor(surplus/2.0));
				flip_rt -= int(ceil(surplus/2.0));
			}//if surplus
		}//if
		if (infos[v].num_bend_free(odWest) == 0)//bottom
		{
			surplus = flip_rb + flip_lb - al_br;
			if (surplus > 0)
			{
				flip_lb -= int(floor(surplus/2.0));
				flip_rb -= int(ceil(surplus/2.0));
			}
		}//if
		if (infos[v].num_bend_free(odSouth) == 0)
		{
			surplus = flip_br + flip_tr - al_rt;
			if (surplus > 0)
			{
				flip_br -= int(floor(surplus/2.0));
				flip_tr -= int(ceil(surplus/2.0));
			}

		}//if
		if (infos[v].num_bend_free(odNorth) == 0)
		{
			surplus = flip_tl + flip_bl - al_lt;
			if (surplus > 0)
			{
				flip_tl -= int(floor(surplus/2.0));
				flip_bl -= int(ceil(surplus/2.0));
			}
		}//if

	}
	//cout<<"resulting flip numbers: "<<flip_tl<<"/"<<flip_tr<<"/"<<flip_bl<<"/"<<flip_br<<"/"
	//	  <<flip_rt<<"/"<<flip_rb<<"/"<<flip_lt<<"/"<<flip_lb<<"\n";
	//now we have the exact number of edges to be rerouted at every corner
	//we change the bendtype for rerouted edges and change their glue point
	//correction: we set the glue point at bend introduction

	int flipedges;
	ListIterator<edge> l_it;

	OrthoDir od = odNorth;
/*TO BE REMOVED IF OBSOLETE
	do
	{
		l_it = infos[v].inList(od).begin();
		while (l_it.valid())
		{
			l_it++; //TODO: what happens here?07.2004
		}
		od =OrthoRep::nextDir(od);
	} while (od != odNorth);
*/
	//start flipping
	l_it = infos[v].inList(odNorth).rbegin();
	for (flipedges = 0; flipedges < flip_lt; flipedges++)
	{
		m_abends[outEntry(infos[v], odNorth, infos[v].inList(odNorth).size() - 1 - flipedges)] = bend_1right;
		infos[v].flips(odNorth, odEast)++;
		l_it--;
	}
	//*********************************
	int newbendfree;
	int newbf;
	if (flip_lt) //we flipped and may save bends, if may be not necessary cause fliplt parameter in betamove
	{
		newbendfree = beta_move(odNorth, odEast, flip_lt, v);
		for (newbf = 0; newbf < newbendfree; newbf++)
		{
			m_abends[outEntry(infos[v], odNorth, infos[v].inList(odNorth).size() - 1 - flip_lt - newbf)] = bend_free;
			m_agp_y[outEntry(infos[v], odNorth, infos[v].inList(odNorth).size() - 1 - flip_lt - newbf)] =
				cp_y(outEntry(infos[v], odNorth, infos[v].inList(odNorth).size() - 1 - flip_lt - newbf));
		}//for
	}//if
	//*********************************
	l_it = infos[v].inList(odNorth).begin();
	for (flipedges = 0; flipedges < flip_lb; flipedges++)
	{
		infos[v].flips(odNorth, odWest)++;
		m_abends[outEntry(infos[v], odNorth, flipedges)] = bend_1left;
		l_it++;
	}
	//**********************************
	newbendfree = beta_move(odNorth, odWest, flip_lb, v);
	for (newbf = 0; newbf < newbendfree; newbf++)
	{
		m_abends[outEntry(infos[v], odNorth, flip_lb + newbf)] = bend_free;
		m_agp_y[outEntry(infos[v], odNorth, flip_lb + newbf)] =
			cp_y(outEntry(infos[v], odNorth, flip_lb + newbf));
	}//for
	//**********************************
	l_it = infos[v].inList(odSouth).rbegin();
	for (flipedges = 0; flipedges < flip_rt; flipedges++)
	{
		infos[v].flips(odSouth, odEast)++;
		m_abends[outEntry(infos[v], odSouth, infos[v].inList(odSouth).size() - 1 - flipedges)] = bend_1left;
		l_it--;
	}
	//***********************************
	newbendfree = beta_move(odSouth, odEast, flip_rt, v);
	for (newbf = 0; newbf < newbendfree; newbf++)
	{
		m_abends[outEntry(infos[v], odSouth, infos[v].inList(odSouth).size() - 1 - flip_rt - newbf)] = bend_free;
		m_agp_y[outEntry(infos[v], odSouth, infos[v].inList(odSouth).size() - 1 - flip_rt - newbf)] =
			cp_y(outEntry(infos[v], odSouth, infos[v].inList(odSouth).size() - 1 - flip_rt - newbf));
	}//for
	//***********************************
	l_it = infos[v].inList(odSouth).begin();
	for (flipedges = 0; flipedges < flip_rb; flipedges++)
	{
		m_abends[outEntry(infos[v], odSouth, flipedges)] = bend_1right;
		infos[v].flips(odSouth, odWest)++;
		l_it++;
	}
	//**********************************
	newbendfree = beta_move(odSouth, odWest, flip_rb, v);
	for (newbf = 0; newbf < newbendfree; newbf++)
	{
		m_abends[outEntry(infos[v], odSouth, flip_rb + newbf)] = bend_free;
		m_agp_y[outEntry(infos[v], odSouth, flip_rb + newbf)] =
			cp_y(outEntry(infos[v], odSouth, flip_rb + newbf));
	}//for
	//**********************************
	//only one of the quadruples will we executed
	l_it = infos[v].inList(odEast).begin();
	for (flipedges = 0; flipedges < flip_tl; flipedges++)
	{
		m_abends[outEntry(infos[v], odEast, flipedges)] = bend_1left;
		infos[v].flips(odEast, odNorth)++;
		l_it++;
	}
	//**********************************
	newbendfree = beta_move(odEast, odNorth, flip_tl, v);
	for (newbf = 0; newbf < newbendfree; newbf++)
	{
		m_abends[outEntry(infos[v], odEast, flip_tl + newbf)] = bend_free;
		m_agp_x[outEntry(infos[v], odEast, flip_tl + newbf)] =
			cp_x(outEntry(infos[v], odEast, flip_tl + newbf));
	}//for
	//**********************************
	l_it = infos[v].inList(odWest).begin();
	for (flipedges = 0; flipedges < flip_bl; flipedges++)
	{
		m_abends[outEntry(infos[v], odWest, flipedges)] = bend_1right;
		infos[v].flips(odWest, odNorth)++;
		l_it++;
	}
	//**********************************
	newbendfree = beta_move(odWest, odNorth, flip_bl, v);
	for (newbf = 0; newbf < newbendfree; newbf++)
	{
		m_abends[outEntry(infos[v], odWest, flip_bl + newbf)] = bend_free;
		m_agp_x[outEntry(infos[v], odWest, flip_bl + newbf)] =
		cp_x(outEntry(infos[v], odWest, flip_bl + newbf));
	}//for
	//**********************************
	l_it = infos[v].inList(odEast).rbegin();
	for (flipedges = 0; flipedges < flip_tr; flipedges++)
	{
		if (l_it.valid()) //temporary check
		{
			m_abends[outEntry(infos[v], odEast, infos[v].inList(odEast).size() - 1 - flipedges)] = bend_1right;
			infos[v].flips(odEast, odSouth)++;
			l_it--;
		}
	}
	//***************************************************
	newbendfree = beta_move(odEast, odSouth, flip_tr, v);
	for (newbf = 0; newbf < newbendfree; newbf++)
	{
		m_abends[outEntry(infos[v], odEast, infos[v].inList(odEast).size() - 1 - flip_tr - newbf)] = bend_free;
		m_agp_x[outEntry(infos[v], odEast, infos[v].inList(odEast).size() - 1 - flip_tr - newbf)] =
			cp_x(outEntry(infos[v], odEast, infos[v].inList(odEast).size() - 1 - flip_tr - newbf));
	}//for
	//***************************************************
	l_it = infos[v].inList(odWest).rbegin();
	for (flipedges = 0; flipedges < flip_br; flipedges++)
	{
		m_abends[outEntry(infos[v], odWest, infos[v].inList(odWest).size() - 1 - flipedges)] = bend_1left;
		infos[v].flips(odWest, odSouth)++;
		l_it--;
	}
	//***************************************************
	newbendfree = beta_move(odWest, odSouth, flip_br, v);
	for (newbf = 0; newbf < newbendfree; newbf++)
	{
		m_abends[outEntry(infos[v], odWest, infos[v].inList(odWest).size() - 1 - flip_br - newbf)] = bend_free;
		m_agp_x[outEntry(infos[v], odWest, infos[v].inList(odWest).size() - 1 - flip_br - newbf)] =
			cp_x(outEntry(infos[v], odWest, infos[v].inList(odWest).size() - 1 - flip_br - newbf));
	}//for
	//***************************************************
	//rest loeschen ?????
	od = odNorth;
	do
	{
		l_it = infos[v].inList(od).begin();
		while (l_it.valid())
		{
			l_it++;
		}
		od =OrthoRep::nextDir(od);
	} while (od != odNorth);

}//reroute


void EdgeRouter::initialize_node_info(node v, int sep)
{
	//derive simple data
	const OrthoRep::VertexInfoUML* vinfo = m_orp->cageInfo(v);
	//inf.get_data(*m_prup, *m_orp, v);
	//zuerst die kantenlisten und generalizations finden und pos festlegen
	{//construct the edge lists for the incoming edges on ev side
		//struct VertexInfoUML {
		// side information (odNorth, odEast, odSouth, odWest corresponds to left, top, right, bottom)
		//SideInfoUML m_side[4];
		// m_corner[dir] is adjacency entry in direction dir starting at
		// a corner
		//adjEntry m_corner[4];
		infos[v].firstAdj() = 0;

		adjEntry adj = m_prup->expandAdj(v);

		if (adj != 0)
		{
			//preliminary: reset PlanRep expandedNode values if necessary
			adjEntry adjRun = adj;

			do
			{
				if (!m_prup->expandedNode(adjRun->theNode()))
					m_prup->setExpandedNode(adjRun->theNode(), v);
				adjRun = adjRun->faceCycleSucc();
			} while (adjRun != adj);

			OGDF_ASSERT(m_prup->typeOf(v) != Graph::generalizationMerger);
			OrthoDir od = odNorth; //start with edges to the left
			do {
				adjEntry sadj = vinfo->m_corner[od];
				adjEntry adjSucc = sadj->faceCycleSucc();

				List<edge>& inedges = infos[v].inList(od);
				List<bool>& inpoint = infos[v].inPoint(od);

				//parse the side and insert incoming edges
				while (m_orp->direction(sadj) == m_orp->direction(adjSucc)) //edges may never be attached at corners
				{
					adjEntry in_edge_adj = adjSucc->cyclicPred();
					edge in_edge = in_edge_adj->theEdge();
					//clockwise cyclic search  ERROR: in/out edges: I always use target later for cage nodes!!
					bool is_in = (in_edge->adjTarget() == in_edge_adj);
					if (infos[v].firstAdj() == 0) infos[v].firstAdj() = in_edge_adj;

					OGDF_ASSERT(m_orp->direction(in_edge_adj) == OrthoRep::nextDir(od) ||
						m_orp->direction(in_edge_adj) == OrthoRep::prevDir(od));
					if ((od == odNorth) || (od == odEast)) //if left or top
					{
						inedges.pushBack(in_edge);
						inpoint.pushBack(is_in);
					}
					else
					{
						inedges.pushFront(in_edge);
						inpoint.pushFront(is_in);
					}
					//setting connection point coordinates
					if (is_in)
					{
						m_acp_x[in_edge_adj] = m_layoutp->x(in_edge->target());
						m_acp_y[in_edge_adj] = m_layoutp->y(in_edge->target());
						m_cage_point[in_edge_adj] = in_edge->target();
						//align test
						if (m_prup->typeOf(in_edge->source()) == Graph::generalizationExpander)
						{
							if (m_align) m_mergerSon[v] = true;
							m_mergeDir[v] = OrthoRep::oppDir(m_orp->direction(in_edge->adjSource()));
						}//if align
					}
					else
					{
						m_acp_x[in_edge_adj] = m_layoutp->x(in_edge->source());
						m_acp_y[in_edge_adj] = m_layoutp->y(in_edge->source());
						m_cage_point[in_edge_adj] = in_edge->source();
						//align test
						if (m_prup->typeOf(in_edge->target()) == Graph::generalizationExpander)
						{
							if (m_align) m_mergerSon[v] = true;
							m_mergeDir[v] = m_orp->direction(in_edge->adjSource());
						}//if align
					}
					sadj = adjSucc;
					adjSucc = sadj->faceCycleSucc();

				}//while
				od =  OrthoRep::nextDir(od);
			} while (od != odNorth);

			infos[v].get_data(*m_orp, *m_layoutp, v, *m_rc, *m_nodewidth, *m_nodeheight);
		}//if no adj, this should never happen
	}//construct edge lists

	//derive the maximum separation between edges on the node sides
	//left side
	int dval, dsep;

	if (infos[v].has_gen(odNorth))
	{
		int le = vinfo->m_side[0].m_nAttached[0]; //to bottom side
		int re = vinfo->m_side[0].m_nAttached[1]; //to top
		dsep =  ( (le+Cconst == 0) ? sep : int(floor(infos[v].node_ysize() / (2*(le+Cconst)))) );
		dval = min(dsep,sep);
		OGDF_ASSERT( dval > 0);
		infos[v].set_delta(OrthoDir(0),OrthoDir(3),dval); infos[v].set_eps(OrthoDir(0),OrthoDir(3),int(floor(Cconst*dval)));
		//top side
		dsep = ( (re+Cconst == 0) ? sep : int(floor(infos[v].node_ysize() / (2*(re+Cconst)))) );
		dval = min(dsep, sep);
		OGDF_ASSERT( dval > 0);
		infos[v].set_delta(OrthoDir(0),OrthoDir(1),dval); infos[v].set_eps(OrthoDir(0),OrthoDir(1),int(floor(Cconst*dval)));
	}//if left
	else
	{
		int ae = vinfo->m_side[0].m_nAttached[0];
		if (ae > 0)
			dsep = ( (ae+Cconst == 1) ? sep : int(floor(infos[v].node_ysize() / (ae - 1 + 2*Cconst))) ); //may be < 0 !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
		else dsep = sep;
		dval = min(dsep, sep);
		OGDF_ASSERT( dval > 0);
		if (dval >= infos[v].node_ysize()) dval = int(floor((double)(infos[v].node_ysize()) / 2)); //allow 1 flip
		OGDF_ASSERT( dval < infos[v].node_ysize() );
		infos[v].set_delta(OrthoDir(0),OrthoDir(1),dval); infos[v].set_eps(OrthoDir(0),OrthoDir(1),int(floor(Cconst*dval)));
		infos[v].set_delta(OrthoDir(0),OrthoDir(3),dval); infos[v].set_eps(OrthoDir(0),OrthoDir(3),int(floor(Cconst*dval)));
	}//else left
	if (infos[v].has_gen(odEast))
	{
		int le = vinfo->m_side[1].m_nAttached[0]; //to left side
		int re = vinfo->m_side[1].m_nAttached[1]; //to right
		dsep = ( (le+Cconst == 0) ? sep : int(floor(infos[v].node_xsize() / (2*(le+Cconst)))) );
		dval = min(dsep,sep);
		OGDF_ASSERT(dval > 0);
		infos[v].set_delta(odEast,odNorth,dval); infos[v].set_eps(odEast,odNorth,int(floor(Cconst*dval)));
		//top side
		dsep = ( (re+Cconst == 0) ? sep : int(floor((double)(infos[v].node_xsize()) / (2*(re+Cconst)))) );
		dval = min(dsep, sep);
		OGDF_ASSERT( dval > 0);
		infos[v].set_delta(odEast,odSouth,dval); infos[v].set_eps(odEast,odSouth,int(floor(Cconst*dval)));
	}//if top
	else
	{
		int ae = vinfo->m_side[1].m_nAttached[0];
		if (ae > 0)
			dsep = ( (ae+Cconst == 1) ? sep : int(floor((double)(infos[v].node_xsize()) / (ae - 1 + 2*Cconst))) ); //may be <= 0
		else dsep = (sep < int(floor((double)(infos[v].node_xsize())/2)) ? sep : int(floor((double)(infos[v].node_xsize())/2)));
		dval = min(dsep, sep);
		OGDF_ASSERT( dval > 0);

		infos[v].set_delta(odEast,odNorth,dval); infos[v].set_eps(odEast,odNorth,int(floor(Cconst*dval)));
		infos[v].set_delta(odEast,odSouth,dval); infos[v].set_eps(odEast,odSouth,int(floor(Cconst*dval)));
	}//else top
	if (infos[v].has_gen(odSouth))
	{
		int le = vinfo->m_side[2].m_nAttached[0]; //to top
		int re = vinfo->m_side[2].m_nAttached[1]; //to bottom
		dsep = ( (le+Cconst == 0) ? sep : int(floor((double)(infos[v].node_ysize()) / (2*(le+Cconst)))) );
		dval = min(dsep,sep);
		OGDF_ASSERT( dval > 0);
		infos[v].set_delta(odSouth,odEast,dval); infos[v].set_eps(odSouth,odEast,int(floor(Cconst*dval)));
		//top side
		dsep = ( (re+Cconst == 0) ? sep : int(floor((double)(infos[v].node_ysize()) / (2*(re+Cconst)))) );
		dval = min(dsep, sep);
		infos[v].set_delta(odSouth,odWest,dval); infos[v].set_eps(odSouth,odWest,int(floor(Cconst*dval)));
	}//if right
	else
	{
		int ae = vinfo->m_side[2].m_nAttached[0];
		if (ae > 0)
			dsep = ( (ae+Cconst == 1) ? sep : int(floor(infos[v].node_ysize() / (ae - 1 + 2*Cconst))) ); //may be <= 0
		else dsep = sep;
		dval = min(dsep, sep);
		infos[v].set_delta(odSouth,odEast,dval); infos[v].set_eps(odSouth,odEast,int(floor(Cconst*dval)));
		infos[v].set_delta(odSouth,odWest,dval); infos[v].set_eps(odSouth,odWest,int(floor(Cconst*dval)));
	}//else right
	if (infos[v].has_gen(odWest))
	{
		int le = vinfo->m_side[3].m_nAttached[0]; //to left side
		int re = vinfo->m_side[3].m_nAttached[1]; //to right
		dsep = ( (le+Cconst == 0) ? sep : int(floor((double)(infos[v].node_xsize()) / (2*(le+Cconst)))) );
		dval = min(dsep,sep);
		OGDF_ASSERT( dval > 0);
		infos[v].set_delta(odWest,odSouth,dval); infos[v].set_eps(odWest,odSouth,int(floor(Cconst*dval)));
		//top side
		dsep = ( (re+Cconst == 0) ? sep : int(floor((double)(infos[v].node_xsize()) / (2*(re+Cconst)))) );
		dval = min(dsep, sep);
		infos[v].set_delta(odWest,odNorth,dval); infos[v].set_eps(odWest,odNorth,int(floor(Cconst*dval)));
	}//if bottom
	else
	{
		int ae = vinfo->m_side[3].m_nAttached[0];
		if (ae > 0)
			dsep = ( (ae+Cconst == 1) ? sep : int(floor((double)(infos[v].node_xsize()) / (ae - 1 + 2*Cconst))) ); //may be <= 0
		else dsep = sep;
		dval = min(dsep, sep);
		OGDF_ASSERT( dval > 0);
		infos[v].set_delta(odWest,odSouth,dval); infos[v].set_eps(odWest,odSouth,int(floor(Cconst*dval)));
		infos[v].set_delta(odWest,odNorth,dval); infos[v].set_eps(odWest,odNorth,int(floor(Cconst*dval)));
	}//else bottom
	//{cagesize, boxsize, delta, epsilon, gen_pos  ...}
}//initialize_node_info


//COMPUTING THE CONSTANTS
//maybe faster: alpha as parameter, recompute if -1, use otherwise
//paper algorithm 2
int EdgeRouter::compute_move(OrthoDir s_from, OrthoDir s_to, int& kflip, node v)
//compute the maximal number of moveable edges from s_from to s_to
//(and thereby the moveable edges, counted from the box corner)
//and return the number of saved bends
{
	//debug: first, we compute (min(al_21, E12)
	kflip = min( alpha_move(s_to, s_from, v), infos[v].num_routable(s_from, s_to) );
	OGDF_ASSERT(kflip > -1);

	return kflip + 2*beta_move(s_from, s_to, kflip, v);
}//compute_move


//helper functions computing the intermediate values
//number of edges that can additionally be routed bendfree at s_from if move_num edges are
//moved from s_from to s_to
int EdgeRouter::beta_move(OrthoDir s_from, OrthoDir s_to, int move_num, node v)
{
	//check the edges in E if their connection point
	//could be routed bendfree to an glue point if move_num edges are flipped to s_to
	if (move_num < 1) return 0;
	int ic = 0;
	bool down = (s_to == odNorth) || (s_to == odWest);

	//try to find out which bend direction is opposite
	//these edges can not be routed bendfree
	bend_type bt1, bt2, bt3, bt4;
	switch (s_from)
	{
	case odEast:
		switch (s_to)
		{
		case odSouth:
			bt1 = prob_b1l;
			bt2 = prob_b2l;
			bt3 = bend_1left;
			bt4 = bend_2left;
			break;
		case odNorth: bt1 = prob_b1r;
			bt2 = prob_b2r;
			bt3 = bend_1right;
			bt4 = bend_2right;
			break;
			OGDF_NODEFAULT
		}//switch s_to
		break;
	case odWest:
		switch (s_to)
		{
		case odSouth:
			bt1 = prob_b1r;
			bt2 = prob_b2r;
			bt3 = bend_1right;
			bt4 = bend_2right;
			break;
		case odNorth:
			bt1 = prob_b1l;
			bt2 = prob_b2l;
			bt3 = bend_1left;
			bt4 = bend_2left;
			break;
			OGDF_NODEFAULT
		}//switch s_to
		break;
	case odNorth:
		switch (s_to)
		{
		case odWest:
			bt1 = prob_b1r;
			bt2 = prob_b2r;
			bt3 = bend_1right;
			bt4 = bend_2right;
			break;
		case odEast:
			bt1 = prob_b1l;
			bt2 = prob_b2l;
			bt3 = bend_1left;
			bt4 = bend_2left;
			break;
			OGDF_NODEFAULT
		}//switch s_to
		break;
	case odSouth:
		switch (s_to)
		{
		case odEast:
			bt1 = prob_b1r;
			bt2 = prob_b2r;
			bt3 = bend_1right;
			bt4 = bend_2right;
			break;
		case odWest:
			bt1 = prob_b1l;
			bt2 = prob_b2l;
			bt3 = bend_1left;
			bt4 = bend_2left;
			break;
			OGDF_NODEFAULT
		}//switch s_to
		break;

		OGDF_NODEFAULT
	}//switch s_from


	{//debug top side to
		//first list all bend edges at corner s_from->s_to by increasing distance to corner in list E
		ListIterator<edge> ep;
		adjEntry ae; //used to find real position
		int adjcount;

		if (down)
		{
			ep = infos[v].inList(s_from).begin(); //first entry iterator
			if (ep.valid()) ae = outEntry(infos[v], s_from, 0);
			adjcount = 0;
		}
		else
		{
			adjcount = infos[v].inList(s_from).size()-1;
			ep = infos[v].inList(s_from).rbegin(); //last entry iterator
			if (ep.valid()) ae = outEntry(infos[v], s_from, adjcount);
		}
		ic = 0;
		while (ep.valid() && (ic < move_num))
		{
			ic++;
			if (down) {ep++; adjcount++;}
			else { ep--; adjcount--; }
		}

		if (ep.valid()) ae = outEntry(infos[v], s_from, adjcount);

		//now ep should point to first usable edge, if list was not empty
		if (!ep.valid()) return 0;

		//if this edge is already unbend, there is nothing to save
		if ((m_abends[ae] == bend_free) ||
			(m_abends[ae] == bt1) ||
			(m_abends[ae] == bt2) ||
			(m_abends[ae] == bt3) ||
			(m_abends[ae] == bt4) )
			return 0;

		bool bend_saveable;
		bool in_E_sfrom_sto; //models set E_s_from_s_to from paper

		ic = 0; //will hold the number of new unbend edges

		//four cases:
		switch (s_to)
		{
		case odEast: //from left and right
			bend_saveable = (cp_y(ae) <= (infos[v].coord(odEast)
				- infos[v].delta(s_from, s_to)*ic
				- infos[v].eps(s_from, s_to)));
			in_E_sfrom_sto =  (cp_y(ae) > gp_y(ae));
			break;
		case odNorth:  //from top and bottom
			bend_saveable = (cp_x(ae) >= (infos[v].coord(odNorth)
				+ infos[v].delta(s_from, s_to)*ic
				+ infos[v].eps(s_from, s_to)));
			in_E_sfrom_sto =  (cp_x(ae) < gp_x(ae));
			break;
		case odSouth:
			bend_saveable = (cp_x(ae) <= (infos[v].coord(odSouth)
				- infos[v].delta(s_from, s_to)*ic
				- infos[v].eps(s_from, s_to)));
			in_E_sfrom_sto =  (cp_x(ae) > gp_x(ae));
			break;
		case odWest:
			bend_saveable = (cp_y(ae) >= (infos[v].coord(odWest)
				+ infos[v].delta(s_from, s_to)*ic
				+ infos[v].eps(s_from, s_to)));
			in_E_sfrom_sto =  (cp_y(ae) < gp_y(ae));
			break;
			OGDF_NODEFAULT
		}//switch

		//compare edges connection point with available space
		while (ep.valid() &&
			bend_saveable &&
			in_E_sfrom_sto &&//models E_from_to set in paper
			(down ? (adjcount < infos[v].inList(s_from).size()-1) : (adjcount > 0))
			) //valid, connection point ok and edge was preliminarily bend (far enough from corner)
		{
			if (down)
			{
				ep++;
				ae = outEntry(infos[v], s_from, ++adjcount);
			}
			else {
				ep--;
				ae = outEntry(infos[v], s_from, --adjcount);
			}
			ic++;

			//four cases:
			if (ep.valid())
			{

				if ((m_abends[ae] == bend_free) ||
					(m_abends[ae] == bt1) ||
					(m_abends[ae] == bt2) ||
					(m_abends[ae] == bt3) ||
					(m_abends[ae] == bt4) )
				break; //no further saving possible
				//hier noch: falls Knick in andere Richtung: auch nicht bendfree

				switch (s_to)
				{
				case odEast: //from left and right , ersetzte cp(*ep) durch cp(ae)
					bend_saveable = (cp_y(ae) <= (infos[v].coord(odEast)
						- infos[v].delta(s_from, s_to)*ic
						- infos[v].eps(s_from, s_to)));
					in_E_sfrom_sto =  (cp_y(ae) > gp_y(ae));
					break;
				case odNorth:  //from top and bottom
					bend_saveable = (cp_x(ae) >= (infos[v].coord(odNorth)
						+ infos[v].delta(s_from, s_to)*ic
						+ infos[v].eps(s_from, s_to)));
					in_E_sfrom_sto =  (cp_x(ae) < gp_x(ae));
					break;
				case odSouth:
					bend_saveable = (cp_x(ae) <= (infos[v].coord(odSouth)
						- infos[v].delta(s_from, s_to)*ic
						- infos[v].eps(s_from, s_to)));
					in_E_sfrom_sto =  (cp_x(ae) > gp_x(ae));
					break;
				case odWest:
					bend_saveable = (cp_y(ae) >= (infos[v].coord(odWest)
						+ infos[v].delta(s_from, s_to)*ic
						+ infos[v].eps(s_from, s_to)));
					in_E_sfrom_sto =  (cp_y(ae) < gp_y(ae));
					break;
					OGDF_NODEFAULT
				}//switch
			}//if
		}//while
	}//debug

	return ic;
}//beta_move


//compute the maximum number of edges to be moved from s_from to s_to
//attention: order of sides reversed: to - from
//optimisation: check for the minimum distance (separaration on "to" side,
//separation on from side) to allow flipping improvement even if the separation
//cannot be guaranted, but the "to" separation will be improved (but assure
//that the postprocessing knows the changed values
//maybe reassign all glue points at the two sides
int EdgeRouter::alpha_move(OrthoDir s_to, OrthoDir s_from, node v)
{
	//Test fuer alignment: Falls der Knoten aligned wird, Kanten nicht an Seiten verlegen
	//zunaechst: garnicht
	if ((m_align) && m_mergerSon[m_prup->expandedNode(v)])
	{
		return 0;
	}

	int result = -1;
	//we can implode the cases, but...
	switch (s_to)
	{
	case odNorth: //two from cases: top or bottom
		switch (s_from)
		{
		case odEast:
			if (infos[v].num_bend_free(odNorth) != 0)
				result = int(floor( (double)((infos[v].coord(odEast) - infos[v].l_upper_unbend() - //oder doch ftop
				infos[v].delta(odNorth, odEast)*infos[v].num_bend_edges(odNorth, odEast) - infos[v].eps(odNorth, odEast)
				) )/  infos[v].delta(odNorth, odEast)
				));
			else //there cant be a generalization: delta = delta_t = delta_l
				result = int(floor( (double)((infos[v].node_ysize() -     //box size
				(infos[v].num_bend_edges(odNorth, odEast) + infos[v].num_bend_edges(odNorth, odWest) - 1)*
				infos[v].delta(odNorth, odEast) - //adjacent edges
				2*infos[v].eps(odNorth, odEast)) / infos[v].delta(odNorth, odEast)) )
				); //left side epsilon without generalization
			break;
		case odWest:
			if (infos[v].num_bend_free(odNorth) != 0)
				result = int(floor( (double)((infos[v].l_lower_unbend() - infos[v].coord(odWest) -
				infos[v].delta(odNorth, odWest)*infos[v].num_bend_edges(odNorth, odWest) -
				infos[v].eps(odNorth, odWest)
				) / infos[v].delta(odNorth, odWest)
				)));
			else  //al_lb = al_lt, just copied, check!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
				result = int(floor((double)((infos[v].node_ysize()     //box size
				- (infos[v].num_bend_edges(odNorth, odEast) + infos[v].num_bend_edges(odNorth, odWest) - 1)
				* infos[v].delta(odNorth, odEast) //adjacent edges
				- 2*infos[v].eps(odNorth, odEast)) / infos[v].delta(odNorth, odEast)
				))); //left side epsilon without generalization
			break;
		default:
			//unmatched s_to in EdgeRouter::alpha_move
			OGDF_THROW(AlgorithmFailureException);
		}//switch

		break;
	case odSouth:
		switch (s_from)
		{
		case odEast:
			if (infos[v].num_bend_free(odSouth) != 0)
			{
				result = int(floor( (double)((infos[v].coord(odEast) - infos[v].r_upper_unbend() -
					infos[v].num_bend_edges(odSouth, odEast)*infos[v].delta(odSouth, odEast) -
					infos[v].eps(odSouth, odEast)
					) / infos[v].delta(odSouth, odEast)
					)));
			}
			else
				result = int(floor((double)( (infos[v].node_ysize()
				- (infos[v].num_bend_edges(odSouth, odEast) + infos[v].num_bend_edges(odSouth, odWest) - 1)
				* infos[v].delta(odSouth, odEast)
				- 2*infos[v].eps(odSouth, odEast)
				) / infos[v].delta(odSouth, odEast)
				)));
			break;
		case odWest:
			if (infos[v].num_bend_free(odSouth) != 0)
				result = int(floor( (double)((infos[v].r_lower_unbend()
				- infos[v].coord(odWest)
				- (infos[v].num_bend_edges(odSouth, odWest))*infos[v].delta(odSouth, odWest)
				- infos[v].eps(odSouth, odWest)
				) / infos[v].delta(odSouth, odWest)
				)));
			else  //same as top, check !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
				result = int(floor( (double)((infos[v].node_ysize()
				- (infos[v].num_bend_edges(odSouth, odEast) + infos[v].num_bend_edges(odSouth, odWest) - 1)
				* infos[v].delta(odSouth, odEast)
				- 2*infos[v].eps(odSouth, odEast)
				) / infos[v].delta(odSouth, odEast)
				)));
			break;
		default:
			// unmatched s_to in EdgeRouter::alpha_move
			OGDF_THROW(AlgorithmFailureException);
		}//switch

		break;
	case odEast:
		switch (s_from)
		{
		case odNorth :
			if (infos[v].num_bend_free(odEast) != 0)
			{
				result = int(floor( (double)(((infos[v].t_left_unbend() - infos[v].coord(odNorth) -  //linkeste bf pos. - linken boxrand
					infos[v].eps(odEast, odNorth) -
					infos[v].delta(odEast, odNorth)*infos[v].num_bend_edges(odEast, odNorth)) / //linkes epsilon und linke Kanten
					infos[v].delta(odEast, odNorth))
					)));
			}
			else
				result = int(floor( (double)((infos[v].node_xsize() -
				(infos[v].num_bend_edges(odEast, odNorth) + infos[v].num_bend_edges(odEast, odSouth) - 1)*infos[v].delta(odEast, odNorth) -
				2*infos[v].eps(odEast, odNorth) ) / infos[v].delta(odEast, odNorth)
				)));
			break;
		case odSouth:
			if (infos[v].num_bend_free(odEast) != 0)
				result = int(floor( (double)((infos[v].coord(odSouth) - infos[v].t_right_unbend()
				- infos[v].num_bend_edges(odEast, odSouth)*infos[v].delta(odEast, odSouth)
				- infos[v].eps(odEast, odSouth)) / infos[v].delta(odEast, odSouth)
				)));
			else //same as left, check !!!
				result = int(floor( (double)((infos[v].node_xsize() -
				(infos[v].num_bend_edges(odEast, odNorth) + infos[v].num_bend_edges(odEast, odSouth) - 1)*infos[v].delta(odEast, odNorth) -
				2*infos[v].eps(odEast, odNorth)
				) / infos[v].delta(odEast, odNorth)
				)));
			break;
		default:
			// unmatched s_to in EdgeRouter::alpha_move
			OGDF_THROW(AlgorithmFailureException);
		}//switch
		break;
	case odWest:
		switch (s_from)
		{
		case odNorth:
			if (infos[v].num_bend_free(odWest) != 0)
				result = int(floor( (double)((infos[v].b_right_unbend() - infos[v].coord(odNorth)
				- infos[v].num_bend_edges(odWest, odNorth)*infos[v].delta(odWest, odNorth)
				- infos[v].eps(odWest, odNorth)
				) / infos[v].delta(odWest, odNorth)
				)));
			else
				result = int(floor( (double)((infos[v].node_xsize()
				- (infos[v].num_bend_edges(odWest, odNorth) + infos[v].num_bend_edges(odWest, odSouth) - 1)*infos[v].delta(odWest, odNorth)
				- 2*infos[v].eps(odWest, odNorth)
				) / infos[v].delta(odWest, odNorth)
				)));
			break;
		case odSouth:
			if (infos[v].num_bend_free(odWest) != 0)
				result = int(floor ( (double)((infos[v].coord(odSouth) - infos[v].b_left_unbend()
				- infos[v].num_bend_edges(odWest, odSouth)*infos[v].delta(odWest, odSouth)
				- infos[v].eps(odWest, odSouth)
				) / infos[v].delta(odWest, odSouth)
				)));
			else
				result = int(floor( (double)((infos[v].node_xsize()
				- (infos[v].num_bend_edges(odWest, odNorth) + infos[v].num_bend_edges(odWest, odSouth) - 1)*infos[v].delta(odWest, odNorth)
				- 2*infos[v].eps(odWest, odNorth)
				) / infos[v].delta(odWest, odNorth)
				)));

			break;
		default:
			// unmatched s_to in EdgeRouter::alpha_move
			OGDF_THROW(AlgorithmFailureException);
		}//switch
		break;

	default:
		// unrecognized side in EdgeRouter::alpha_move
		OGDF_THROW(AlgorithmFailureException);
	}//switch

	if (result < 0) result = 0;
	return result;

}//alpha_move


void EdgeRouter::addbends(BendString& bs, const char* s2)
{
	const char* s1 = bs.toString();
	size_t len = strlen(s1) + strlen(s2) + 1;
	char* resi = new char[len];
	bs.set(resi);
	delete[] resi;
}//addbends


//add a left bend to edge e
edge EdgeRouter::addLeftBend(edge e)
{
	int a1 = m_orp->angle(e->adjSource());
	int a2 = m_orp->angle(e->adjTarget());

	edge ePrime = m_comb->split(e);
	m_orp->angle(ePrime->adjSource()) = 3;
	m_orp->angle(ePrime->adjTarget()) = a2;
	m_orp->angle(e->adjSource()) = a1;
	m_orp->angle(e->adjTarget()) = 1;

	return ePrime;
}//addLeftBend


//add a right bend to edge e
edge EdgeRouter::addRightBend(edge e)
{
	String msg;

	int a1 = m_orp->angle(e->adjSource());
	int a2 = m_orp->angle(e->adjTarget());

	edge ePrime = m_comb->split(e);

	m_orp->angle(ePrime->adjSource()) = 1;
	m_orp->angle(ePrime->adjTarget()) = a2;
	m_orp->angle(e->adjSource()) = a1;
	m_orp->angle(e->adjTarget()) = 3;

	return ePrime;
}


//set the computed values in the m_med structure
void EdgeRouter::setDistances()
{
	node v;
	forall_nodes(v, *m_prup)
	{
		if ((m_prup->expandAdj(v) != 0) && (m_prup->typeOf(v) != Graph::generalizationMerger))
		{
			OrthoDir od = odNorth;
			do
			{
				m_med->delta(v, od, 0) = infos[v].delta(od, OrthoRep::prevDir(od));
				m_med->delta(v, od, 1) = infos[v].delta(od, OrthoRep::nextDir(od));
				m_med->epsilon(v, od, 0) = infos[v].eps(od, OrthoRep::prevDir(od));
				m_med->epsilon(v, od, 1) = infos[v].eps(od, OrthoRep::nextDir(od));

				od = OrthoRep::nextDir(od);
			} while (od != odNorth);
		}//if
	}//forallnodes
}


void EdgeRouter::unsplit(edge e1, edge e2)
{
	//precondition: ae1 is adjsource/sits on original edge
	int a1 = m_orp->angle(e1->adjSource()); //angle at source
	int a2 = m_orp->angle(e2->adjTarget()); //angle at target
	m_prup->unsplit(e1, e2);
	m_orp->angle(e1->adjSource()) = a1;
	m_orp->angle(e1->adjTarget()) = a2;
}//unsplit


void EdgeRouter::set_position(node v, int x, int y)
{
	if (!m_fixed[v])
	{
		m_layoutp->x(v) = x;
		m_layoutp->y(v) = y;
	}
}//set_position


void EdgeRouter::fix_position(node v, int x, int y)
{
	m_layoutp->x(v) = x;
	m_layoutp->y(v) = y;
	m_fixed[v] = true;
}

} //end namespace

