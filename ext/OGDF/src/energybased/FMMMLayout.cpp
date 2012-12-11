/*
 * $Revision: 2565 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-07 17:14:54 +0200 (Sa, 07. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Implementation of Fast Multipole Multilevel Method (FM^3).
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


#include <ogdf/energybased/FMMMLayout.h>
#include <ogdf/basic/Math.h>
#include "numexcept.h"
#include "MAARPacking.h"
#include "Multilevel.h"
#include "Edge.h"
#include <ogdf/basic/simple_graph_alg.h>
#include <ogdf/basic/basic.h>

#include <ogdf/internal/energybased/NodeAttributes.h>
#include <ogdf/internal/energybased/EdgeAttributes.h>
#include "Rectangle.h"

namespace ogdf {


FMMMLayout::FMMMLayout()
{
	initialize_all_options();
}


//--------------------------- most important functions --------------------------------

void FMMMLayout::call(GraphAttributes &GA)
{
	const Graph &G = GA.constGraph();
	EdgeArray<double> edgelength(G,1.0);
	call(GA,edgelength);
}

void FMMMLayout::call(ClusterGraphAttributes &GA)
{
	const Graph &G = GA.constGraph();
	//compute depth of cluster tree, also sets cluster depth values
	const ClusterGraph &CG = GA.constClusterGraph();
	int cdepth = CG.treeDepth();
	EdgeArray<double> edgeLength(G);
	//compute lca of end vertices for each edge
	edge e;
	forall_edges(e, G)
	{
		edgeLength[e] = cdepth - CG.clusterDepth(CG.commonCluster(e->source(),e->target())) + 1;
		OGDF_ASSERT(edgeLength[e] > 0)
	}
	call(GA,edgeLength);
	GA.updateClusterPositions();
}


void FMMMLayout::call(GraphAttributes &GA, const EdgeArray<double> &edgeLength)
{
	const Graph &G = GA.constGraph();
	NodeArray<NodeAttributes> A(G);       //stores the attributes of the nodes (given by L)
	EdgeArray<EdgeAttributes> E(G);       //stores the edge attributes of G
	Graph G_reduced;                      //stores a undirected simple and loopfree copy
										//of G
	EdgeArray<EdgeAttributes> E_reduced;  //stores the edge attributes of G_reduced
	NodeArray<NodeAttributes> A_reduced;  //stores the node attributes of G_reduced

	if(G.numberOfNodes() > 1)
	{
		GA.clearAllBends();//all edges are straight-line
		if(useHighLevelOptions())
			update_low_level_options_due_to_high_level_options_settings();
		import_NodeAttributes(G,GA,A);
		import_EdgeAttributes(G,edgeLength,E);

		double t_total;
		usedTime(t_total);
		max_integer_position = pow(2.0,maxIntPosExponent());
		init_ind_ideal_edgelength(G,A,E);
		make_simple_loopfree(G,A,E,G_reduced,A_reduced,E_reduced);
		call_DIVIDE_ET_IMPERA_step(G_reduced,A_reduced,E_reduced);
		if(allowedPositions() != apAll)
			make_positions_integer(G_reduced,A_reduced);
		time_total = usedTime(t_total);

		export_NodeAttributes(G_reduced,A_reduced,GA);
	}
	else //trivial cases
	{
		if(G.numberOfNodes() == 1 )
		{
			node v = G.firstNode();
			GA.x(v) = 0;
			GA.y(v) = 0;
		}
	}
}


void FMMMLayout::call(GraphAttributes &AG, char* ps_file)
{
	call(AG);
	create_postscript_drawing(AG,ps_file);
}


void FMMMLayout::call(
	GraphAttributes &AG,
	const EdgeArray<double> &edgeLength,
	char* ps_file)
{
	call(AG,edgeLength);
	create_postscript_drawing(AG,ps_file);
}


void FMMMLayout::call_DIVIDE_ET_IMPERA_step(
	Graph& G,
	NodeArray<NodeAttributes>& A,
	EdgeArray<EdgeAttributes>& E)
{
	NodeArray<int> component(G); //holds for each node the index of its component
	number_of_components = connectedComponents(G,component);//calculate components of G
	Graph* G_sub = new Graph[number_of_components];
	NodeArray<NodeAttributes>* A_sub = new NodeArray<NodeAttributes>[number_of_components];
	EdgeArray<EdgeAttributes>* E_sub = new EdgeArray<EdgeAttributes>[number_of_components];
	create_maximum_connected_subGraphs(G,A,E,G_sub,A_sub,E_sub,component);

	if(number_of_components == 1)
		call_MULTILEVEL_step_for_subGraph(G_sub[0],A_sub[0],E_sub[0],-1);
	else
		for(int i = 0; i < number_of_components;i++)
			call_MULTILEVEL_step_for_subGraph(G_sub[i],A_sub[i],E_sub[i],i);

	pack_subGraph_drawings (A,G_sub,A_sub);
	delete_all_subGraphs(G_sub,A_sub,E_sub);
}


void FMMMLayout::call_MULTILEVEL_step_for_subGraph(
	Graph& G,
	NodeArray<NodeAttributes>& A,
	EdgeArray<EdgeAttributes>& E,
	int comp_index)
{
	Multilevel Mult;

	int max_level = 30;//sufficient for all graphs with upto pow(2,30) nodes!
	//adapt mingraphsize such that no levels are created beyond input graph.
	if (m_singleLevel) m_minGraphSize = G.numberOfNodes();
	Array<Graph*> G_mult_ptr(max_level+1);
	Array<NodeArray<NodeAttributes>*> A_mult_ptr (max_level+1);
	Array<EdgeArray<EdgeAttributes>*> E_mult_ptr (max_level+1);

	Mult.create_multilevel_representations(G,A,E,randSeed(),
				galaxyChoice(),minGraphSize(),
				randomTries(),G_mult_ptr,A_mult_ptr,
				E_mult_ptr,max_level);

	for(int i = max_level;i >= 0;i--)
	{
		if(i == max_level)
			create_initial_placement(*G_mult_ptr[i],*A_mult_ptr[i]);
		else
		{
			Mult.find_initial_placement_for_level(i,initialPlacementMult(),G_mult_ptr, A_mult_ptr,E_mult_ptr);
			update_boxlength_and_cornercoordinate(*G_mult_ptr[i],*A_mult_ptr[i]);
		}
		call_FORCE_CALCULATION_step(*G_mult_ptr[i],*A_mult_ptr[i],*E_mult_ptr[i],
				 i,max_level);
	}
	Mult.delete_multilevel_representations(G_mult_ptr,A_mult_ptr,E_mult_ptr,max_level);
}


void FMMMLayout::call_FORCE_CALCULATION_step(
	Graph& G,
	NodeArray<NodeAttributes>&A,
	EdgeArray<EdgeAttributes>& E,
	int act_level,
	int max_level)
{
	const int ITERBOUND = 10000;//needed to guarantee termination if
							 //stopCriterion() == scThreshold
	if(G.numberOfNodes() > 1)
	{
		int iter = 1;
		int max_mult_iter = get_max_mult_iter(act_level,max_level,G.numberOfNodes());
		double actforcevectorlength = threshold() + 1;

		NodeArray<DPoint> F_rep(G); //stores rep. forces
		NodeArray<DPoint> F_attr(G); //stores attr. forces
		NodeArray<DPoint> F (G); //stores resulting forces
		NodeArray<DPoint> last_node_movement(G);//stores the force vectors F of the last
												//iterations (needed to avoid oscillations)

		set_average_ideal_edgelength(G,E);//needed for easy scaling of the forces
		make_initialisations_for_rep_calc_classes(G);

		while( ((stopCriterion() == scFixedIterations)&&(iter <= max_mult_iter)) ||
			((stopCriterion() == scThreshold)&&(actforcevectorlength >= threshold())&&
			(iter <= ITERBOUND)) ||
			((stopCriterion() == scFixedIterationsOrThreshold)&&(iter <= max_mult_iter) &&
			(actforcevectorlength >= threshold())) )
		{//while
			calculate_forces(G,A,E,F,F_attr,F_rep,last_node_movement,iter,0);
			if(stopCriterion() != scFixedIterations)
				actforcevectorlength = get_average_forcevector_length(G,F);
			iter++;
		}//while

		if(act_level == 0)
			call_POSTPROCESSING_step(G,A,E,F,F_attr,F_rep,last_node_movement);

		deallocate_memory_for_rep_calc_classes();
	}
}


void FMMMLayout::call_POSTPROCESSING_step(
	Graph& G,
	NodeArray<NodeAttributes>& A,
	EdgeArray<EdgeAttributes>& E,
	NodeArray<DPoint>& F,
	NodeArray<DPoint>& F_attr,
	NodeArray<DPoint>& F_rep,
	NodeArray<DPoint>& last_node_movement)
{
	for(int i = 1; i<= 10; i++)
		calculate_forces(G,A,E,F,F_attr,F_rep,last_node_movement,i,1);

	if((resizeDrawing() == true))
	{
		adapt_drawing_to_ideal_average_edgelength(G,A,E);
		update_boxlength_and_cornercoordinate(G,A);
	}

	for(int i = 1; i<= fineTuningIterations(); i++)
		calculate_forces(G,A,E,F,F_attr,F_rep,last_node_movement,i,2);

	if((resizeDrawing() == true))
		adapt_drawing_to_ideal_average_edgelength(G,A,E);
}


//------------------------- functions for pre/post-processing -------------------------

void FMMMLayout::initialize_all_options()
{
	//setting high level options
	useHighLevelOptions(false); pageFormat(pfSquare); unitEdgeLength(100);
	newInitialPlacement(false); qualityVersusSpeed(qvsBeautifulAndFast);

	//setting low level options
	//setting general options
	randSeed(100);edgeLengthMeasurement(elmBoundingCircle);
	allowedPositions(apInteger);maxIntPosExponent(40);

	//setting options for the divide et impera step
	pageRatio(1.0);stepsForRotatingComponents(10);
	tipOverCCs(toNoGrowingRow);minDistCC(100);
	presortCCs(psDecreasingHeight);

	//setting options for the multilevel step
	minGraphSize(50);galaxyChoice(gcNonUniformProbLowerMass);randomTries(20);
	maxIterChange(micLinearlyDecreasing);maxIterFactor(10);
	initialPlacementMult(ipmAdvanced);
	m_singleLevel = false;

	//setting options for the force calculation step
	forceModel(fmNew);springStrength(1);repForcesStrength(1);
	repulsiveForcesCalculation(rfcNMM);stopCriterion(scFixedIterationsOrThreshold);
	threshold(0.01);fixedIterations(30);forceScalingFactor(0.05);
	coolTemperature(false);coolValue(0.99);initialPlacementForces(ipfRandomRandIterNr);

	//setting options for postprocessing
	resizeDrawing(true);resizingScalar(1);fineTuningIterations(20);
	fineTuneScalar(0.2);adjustPostRepStrengthDynamically(true);
	postSpringStrength(2.0);postStrengthOfRepForces(0.01);

	//setting options for different repulsive force calculation methods
	frGridQuotient(2);
	nmTreeConstruction(rtcSubtreeBySubtree);nmSmallCell(scfIteratively);
	nmParticlesInLeaves(25); nmPrecision(4);
}


void FMMMLayout::update_low_level_options_due_to_high_level_options_settings()
{
	PageFormatType pf = pageFormat();
	double uel = unitEdgeLength();
	bool nip = newInitialPlacement();
	QualityVsSpeed qvs = qualityVersusSpeed();

	//update
	initialize_all_options();
	useHighLevelOptions(true);
	pageFormat(pf);
	unitEdgeLength(uel);
	newInitialPlacement(nip);
	qualityVersusSpeed(qvs);

	if(pageFormat() == pfSquare)
		pageRatio(1.0);
	else if(pageFormat() ==pfLandscape)
		pageRatio(1.4142);
	else //pageFormat() == pfPortrait
		pageRatio(0.7071);

	if(newInitialPlacement())
		initialPlacementForces(ipfRandomTime);
	else
		initialPlacementForces(ipfRandomRandIterNr);

	if(qualityVersusSpeed() == qvsGorgeousAndEfficient)
	{
		fixedIterations(60);
		fineTuningIterations(40);
		nmPrecision(6);
	}
	else if(qualityVersusSpeed() == qvsBeautifulAndFast)
	{
		fixedIterations(30);
		fineTuningIterations(20);
		nmPrecision(4);
	}
	else //qualityVersusSpeed() == qvsNiceAndIncredibleSpeed
	{
		fixedIterations(15);
		fineTuningIterations(10);
		nmPrecision(2);
	}
}


void FMMMLayout::import_NodeAttributes(
	const Graph& G,
	GraphAttributes& GA,
	NodeArray<NodeAttributes>& A)
{
	node v;
	DPoint position;

	forall_nodes(v,G)
	{
		position.m_x = GA.x(v);
		position.m_y = GA.y(v);
		A[v].set_NodeAttributes(GA.width(v),GA.height(v),position,NULL,NULL);
	}
}


void FMMMLayout::import_EdgeAttributes(
	const Graph& G,
	const EdgeArray<double>& edgeLength,
	EdgeArray<EdgeAttributes>& E)
{
	edge e;
	double length;

	forall_edges(e,G)
	{
		if(edgeLength[e] > 0) //no negative edgelength allowed
			length = edgeLength[e];
		else
			length = 1;

		E[e].set_EdgeAttributes(length,NULL,NULL);
	}
}


void FMMMLayout::init_ind_ideal_edgelength(
	const Graph& G,
	NodeArray<NodeAttributes>& A,
	EdgeArray<EdgeAttributes>& E)
{
	edge e;

	if (edgeLengthMeasurement() == elmMidpoint)
		forall_edges(e,G)
			E[e].set_length(E[e].get_length() * unitEdgeLength());

	else //(edgeLengthMeasurement() == elmBoundingCircle)
	{
		set_radii(G,A);
		forall_edges(e,G)
			E[e].set_length(E[e].get_length() * unitEdgeLength() + radius[e->source()]
				+ radius[e->target()]);
	}
}


void FMMMLayout::set_radii(const Graph& G, NodeArray<NodeAttributes>& A)
{
	node v;
	radius.init(G);
	double w,h;
	forall_nodes(v,G)
	{
		w = A[v].get_width()/2;
		h = A[v].get_height()/2;
		radius[v] = sqrt(w*w+ h*h);
	}
}


void FMMMLayout::export_NodeAttributes(
	Graph& G_reduced,
	NodeArray<NodeAttributes>& A_reduced,
	GraphAttributes& GA)
{
	node v_copy;
	forall_nodes(v_copy,G_reduced)
	{
		GA.x(A_reduced[v_copy].get_original_node()) =  A_reduced[v_copy].get_position().m_x;
		GA.y(A_reduced[v_copy].get_original_node()) =  A_reduced[v_copy].get_position().m_y;
	}
}


void FMMMLayout::make_simple_loopfree(
	const Graph& G,
	NodeArray<NodeAttributes>& A,
	EdgeArray<EdgeAttributes>E,
	Graph& G_reduced,
	NodeArray<NodeAttributes>& A_reduced,
	EdgeArray<EdgeAttributes>& E_reduced)
{
	node u_orig,v_orig,v_reduced;
	edge e_reduced,e_orig;

	//create the reduced Graph G_reduced and save in A/E links to node/edges of G_reduced
	//create G_reduced as a copy of G without selfloops!

	G_reduced.clear();
	forall_nodes(v_orig,G)
		A[v_orig].set_copy_node(G_reduced.newNode());
	forall_edges(e_orig,G)
	{
		u_orig = e_orig->source();
		v_orig = e_orig->target();
		if(u_orig != v_orig)
			E[e_orig].set_copy_edge(G_reduced.newEdge (A[u_orig].get_copy_node(),
			A[v_orig].get_copy_node()));
		else
			E[e_orig].set_copy_edge(NULL);//mark this edge as deleted
	}

	//remove parallel (and reversed) edges from G_reduced
	EdgeArray<double> new_edgelength(G_reduced);
	List<edge> S;
	S.clear();
	delete_parallel_edges(G,E,G_reduced,S,new_edgelength);

	//make A_reduced, E_reduced valid for G_reduced
	A_reduced.init(G_reduced);
	E_reduced.init(G_reduced);

	//import information for A_reduced, E_reduced and links to the original nodes/edges
	//of the copy nodes/edges
	forall_nodes(v_orig,G)
	{
		v_reduced = A[v_orig].get_copy_node();
		A_reduced[v_reduced].set_NodeAttributes(A[v_orig].get_width(), A[v_orig].
			get_height(),A[v_orig].get_position(),
			v_orig,NULL);
	}
	forall_edges(e_orig,G)
	{
		e_reduced = E[e_orig].get_copy_edge();
		if(e_reduced != NULL)
			E_reduced[e_reduced].set_EdgeAttributes(E[e_orig].get_length(),e_orig,NULL);
	}

	//update edgelength of copy edges in G_reduced associated with a set of parallel
	//edges in G
	update_edgelength(S,new_edgelength,E_reduced);
}


void FMMMLayout::delete_parallel_edges(
	const Graph& G,
	EdgeArray<EdgeAttributes>& E,
	Graph& G_reduced,
	List<edge>& S,
	EdgeArray<double>& new_edgelength)
{
	EdgeMaxBucketFunc MaxSort;
	EdgeMinBucketFunc MinSort;
	ListIterator<Edge> EdgeIterator;
	edge e_act,e_save;
	Edge f_act;
	List<Edge> sorted_edges;
	EdgeArray<edge> original_edge (G_reduced); //helping array
	int save_s_index,save_t_index,act_s_index,act_t_index;
	int counter = 1;
	Graph* Graph_ptr = &G_reduced;

	//save the original edges for each edge in G_reduced
	forall_edges(e_act,G)
		if(E[e_act].get_copy_edge() != NULL) //e_act is no self_loops
			original_edge[E[e_act].get_copy_edge()] = e_act;

	forall_edges(e_act,G_reduced)
	{
		f_act.set_Edge(e_act,Graph_ptr);
		sorted_edges.pushBack(f_act);
	}

	sorted_edges.bucketSort(0,G_reduced.numberOfNodes()-1,MaxSort);
	sorted_edges.bucketSort(0,G_reduced.numberOfNodes()-1,MinSort);

	//now parallel edges are consecutive in sorted_edges
	for(EdgeIterator = sorted_edges.begin();EdgeIterator.valid();++EdgeIterator)
	{//for
		e_act = (*EdgeIterator).get_edge();
		act_s_index = e_act->source()->index();
		act_t_index = e_act->target()->index();

		if(EdgeIterator != sorted_edges.begin())
		{//if
			if( (act_s_index == save_s_index && act_t_index == save_t_index) ||
				(act_s_index == save_t_index && act_t_index == save_s_index) )
			{
				if(counter == 1) //first parallel edge
				{
					S.pushBack(e_save);
					new_edgelength[e_save] = E[original_edge[e_save]].get_length() +
						E[original_edge[e_act]].get_length();
				}
				else //more then two parallel edges
					new_edgelength[e_save] +=E[original_edge[e_act]].get_length();

				E[original_edge[e_act]].set_copy_edge(NULL); //mark copy of edge as deleted
				G_reduced.delEdge(e_act);                    //delete copy edge in G_reduced
				counter++;
			}
			else
			{
				if (counter > 1)
				{
					new_edgelength[e_save]/=counter;
					counter = 1;
				}
				save_s_index = act_s_index;
				save_t_index = act_t_index;
				e_save = e_act;
			}
		}//if
		else //first edge
		{
			save_s_index = act_s_index;
			save_t_index = act_t_index;
			e_save = e_act;
		}
	}//for

	//treat special case (last edges were multiple edges)
	if(counter >1)
		new_edgelength[e_save]/=counter;
}


void FMMMLayout::update_edgelength(
	List<edge>& S,
	EdgeArray<double>& new_edgelength,
	EdgeArray<EdgeAttributes>& E_reduced)
{
	edge e;
	while (!S.empty())
	{
		e = S.popFrontRet();
		E_reduced[e].set_length(new_edgelength[e]);
	}
}


//inline double FMMMLayout::get_post_rep_force_strength(int n)
//{
//	return min(0.2,400.0/double(n));
//}


void FMMMLayout::make_positions_integer(Graph& G, NodeArray<NodeAttributes>& A)
{
	node v;
	double new_x,new_y;

	if(allowedPositions() == apInteger)
	{//if
		//calculate value of max_integer_position
		max_integer_position = 100 * average_ideal_edgelength * G.numberOfNodes() *
			G.numberOfNodes();
	}//if

	//restrict positions to lie in [-max_integer_position,max_integer_position]
	//X [-max_integer_position,max_integer_position]
	forall_nodes(v,G)
		if( (A[v].get_x() > max_integer_position) ||
			(A[v].get_y() > max_integer_position) ||
			(A[v].get_x() < max_integer_position * (-1.0)) ||
			(A[v].get_y() < max_integer_position * (-1.0)) )
		{
			DPoint cross_point;
			DPoint nullpoint (0,0);
			DPoint old_pos (A[v].get_x(),A[v].get_y());
			DPoint lt ( max_integer_position * (-1.0),max_integer_position);
			DPoint rt ( max_integer_position,max_integer_position);
			DPoint lb ( max_integer_position * (-1.0),max_integer_position * (-1.0));
			DPoint rb ( max_integer_position,max_integer_position * (-1.0));
			DLine s (nullpoint,old_pos);
			DLine left_bound (lb,lt);
			DLine right_bound (rb,rt);
			DLine top_bound (lt,rt);
			DLine bottom_bound (lb,rb);

			if(s.intersection(left_bound,cross_point))
			{
				A[v].set_x(cross_point.m_x);
				A[v].set_y(cross_point.m_y);
			}
			else if(s.intersection(right_bound,cross_point))
			{
				A[v].set_x(cross_point.m_x);
				A[v].set_y(cross_point.m_y);
			}
			else if(s.intersection(top_bound,cross_point))
			{
				A[v].set_x(cross_point.m_x);
				A[v].set_y(cross_point.m_y);
			}
			else if(s.intersection(bottom_bound,cross_point))
			{
				A[v].set_x(cross_point.m_x);
				A[v].set_y(cross_point.m_y);
			}
			else cout<<"Error FMMMLayout:: make_positions_integer()"<<endl;
		}

		//make positions integer
		forall_nodes(v,G)
		{
			new_x = floor(A[v].get_x());
			new_y = floor(A[v].get_y());
			if(new_x < down_left_corner.m_x)
			{
				boxlength += 2;
				down_left_corner.m_x = down_left_corner.m_x-2;
			}
			if(new_y < down_left_corner.m_y)
			{
				boxlength += 2;
				down_left_corner.m_y = down_left_corner.m_y-2;
			}
			A[v].set_x(new_x);
			A[v].set_y(new_y);
		}
}


void FMMMLayout::create_postscript_drawing(GraphAttributes& AG, char* ps_file)
{
	ofstream out_fmmm (ps_file,ios::out);
	if (!ps_file) cout<<ps_file<<" could not be opened !"<<endl;
	const Graph& G = AG.constGraph();
	node v;
	edge e;
	double x_min = AG.x(G.firstNode());
	double x_max = x_min;
	double y_min = AG.y(G.firstNode());
	double y_max = y_min;
	double max_dist;
	double scale_factor;

	forall_nodes(v,G)
	{
		if(AG.x(v) < x_min)
			x_min = AG.x(v);
		else if(AG.x(v) > x_max)
			x_max = AG.x(v);
		if(AG.y(v) < y_min)
			y_min = AG.y(v);
		else if(AG.y(v) > y_max)
			y_max = AG.y(v);
	}
	max_dist = max(x_max -x_min,y_max-y_min);
	scale_factor = 500.0/max_dist;

	out_fmmm<<"%!PS-Adobe-2.0 "<<endl;
	out_fmmm<<"%%Pages:  1 "<<endl;
	out_fmmm<<"% %BoundingBox: "<<x_min<<" "<<x_max<<" "<<y_min<<" "<<y_max<<endl;
	out_fmmm<<"%%EndComments "<<endl;
	out_fmmm<<"%%"<<endl;
	out_fmmm<<"%% Circle"<<endl;
	out_fmmm<<"/ellipse_dict 4 dict def"<<endl;
	out_fmmm<<"/ellipse {"<<endl;
	out_fmmm<<"  ellipse_dict"<<endl;
	out_fmmm<<"  begin"<<endl;
	out_fmmm<<"   newpath"<<endl;
	out_fmmm<<"   /yrad exch def /xrad exch def /ypos exch def /xpos exch def"<<endl;
	out_fmmm<<"   matrix currentmatrix"<<endl;
	out_fmmm<<"   xpos ypos translate"<<endl;
	out_fmmm<<"   xrad yrad scale"<<endl;
	out_fmmm<<"  0 0 1 0 360 arc"<<endl;
	out_fmmm<<"  setmatrix"<<endl;
	out_fmmm<<"  closepath"<<endl;
	out_fmmm<<" end"<<endl;
	out_fmmm<<"} def"<<endl;
	out_fmmm<<"%% Nodes"<<endl;
	out_fmmm<<"/v { "<<endl;
	out_fmmm<<" /y exch def"<<endl;
	out_fmmm<<" /x exch def"<<endl;
	out_fmmm<<"1.000 1.000 0.894 setrgbcolor"<<endl;
	out_fmmm<<"x y 10.0 10.0 ellipse fill"<<endl;
	out_fmmm<<"0.000 0.000 0.000 setrgbcolor"<<endl;
	out_fmmm<<"x y 10.0 10.0 ellipse stroke"<<endl;
	out_fmmm<<"} def"<<endl;
	out_fmmm<<"%% Edges"<<endl;
	out_fmmm<<"/e { "<<endl;
	out_fmmm<<" /b exch def"<<endl;
	out_fmmm<<" /a exch def"<<endl;
	out_fmmm<<" /y exch def"<<endl;
	out_fmmm<<" /x exch def"<<endl;
	out_fmmm<<"x y moveto a b lineto stroke"<<endl;
	out_fmmm<<"} def"<<endl;
	out_fmmm<<"%% "<<endl;
	out_fmmm<<"%% INIT "<<endl;
	out_fmmm<<"20  200 translate"<<endl;
	out_fmmm<<scale_factor<<"  "<<scale_factor<<"  scale "<<endl;
	out_fmmm<<"1 setlinewidth "<<endl;
	out_fmmm<<"%%BeginProgram "<<endl;
	forall_edges(e,G)
		out_fmmm<<AG.x(e->source())<<" "<<AG.y(e->source())<<" "
		<<AG.x(e->target())<<" "<<AG.y(e->target())<<" e"<<endl;
	forall_nodes(v,G)
		out_fmmm<<AG.x(v)<<" "<<AG.y(v) <<" v"<<endl;
	out_fmmm<<"%%EndProgram "<<endl;
	out_fmmm<<"showpage "<<endl;
	out_fmmm<<"%%EOF "<<endl;
}


//------------------------- functions for divide et impera step -----------------------

void FMMMLayout::create_maximum_connected_subGraphs(
	Graph& G,
	NodeArray<NodeAttributes>& A,
	EdgeArray<EdgeAttributes>&E,
	Graph G_sub[],
	NodeArray<NodeAttributes> A_sub[],
	EdgeArray<EdgeAttributes> E_sub[],
	NodeArray<int>& component)
{
	node u_orig,v_orig,v_sub;
	edge e_sub,e_orig;
	int i;

	//create the subgraphs and save links to subgraph nodes/edges in A
	forall_nodes(v_orig,G)
		A[v_orig].set_subgraph_node(G_sub[component[v_orig]].newNode());
	forall_edges(e_orig,G)
	{
		u_orig = e_orig->source();
		v_orig = e_orig->target();
		E[e_orig].set_subgraph_edge( G_sub[component[u_orig]].newEdge
			(A[u_orig].get_subgraph_node(),A[v_orig].get_subgraph_node()));
	}

	//make A_sub,E_sub valid for the subgraphs
	for(i = 0; i< number_of_components;i++)
	{
		A_sub[i].init(G_sub[i]);
		E_sub[i].init(G_sub[i]);
	}

	//import information for A_sub,E_sub and links to the original nodes/edges
	//of the subGraph nodes/edges

	forall_nodes(v_orig,G)
	{
		v_sub = A[v_orig].get_subgraph_node();
		A_sub[component[v_orig]][v_sub].set_NodeAttributes(A[v_orig].get_width(),
			A[v_orig].get_height(),A[v_orig].get_position(),
			v_orig,NULL);
	}
	forall_edges(e_orig,G)
	{
		e_sub = E[e_orig].get_subgraph_edge();
		v_orig = e_orig->source();
		E_sub[component[v_orig]][e_sub].set_EdgeAttributes(E[e_orig].get_length(),
			e_orig,NULL);
	}
}


void FMMMLayout::pack_subGraph_drawings(
	NodeArray<NodeAttributes>& A,
	Graph G_sub[],
	NodeArray<NodeAttributes> A_sub[])
{
	double aspect_ratio_area, bounding_rectangles_area;
	MAARPacking P;
	List<Rectangle> R;

	if(stepsForRotatingComponents() == 0) //no rotation
		calculate_bounding_rectangles_of_components(R,G_sub,A_sub);
	else
		rotate_components_and_calculate_bounding_rectangles(R,G_sub,A_sub);

	P.pack_rectangles_using_Best_Fit_strategy(R,pageRatio(),presortCCs(),
		tipOverCCs(),aspect_ratio_area,
		bounding_rectangles_area);
	export_node_positions(A,R,G_sub,A_sub);
}


void FMMMLayout::calculate_bounding_rectangles_of_components(
	List<Rectangle>& R,
	Graph G_sub[],
	NodeArray<NodeAttributes> A_sub[])
{
	int i;
	Rectangle r;
	R.clear();

	for(i=0;i<number_of_components;i++)
	{
		r = calculate_bounding_rectangle(G_sub[i],A_sub[i],i);
		R.pushBack(r);
	}
}


Rectangle FMMMLayout::calculate_bounding_rectangle(
	Graph& G,
	NodeArray<NodeAttributes>& A,
	int componenet_index)
{
	Rectangle r;
	node v;
	double x_min,x_max,y_min,y_max,act_x_min,act_x_max,act_y_min,act_y_max;
	double max_boundary;//the maximum of half of the width and half of the height of
	//each node; (needed to be able to tipp rectangles over without
	//having access to the height and width of each node)

	forall_nodes(v,G)
	{
		max_boundary = max(A[v].get_width()/2, A[v].get_height()/2);
		if(v == G.firstNode())
		{
			x_min = A[v].get_x() - max_boundary;
			x_max = A[v].get_x() + max_boundary;
			y_min = A[v].get_y() - max_boundary;
			y_max = A[v].get_y() + max_boundary;
		}
		else
		{
			act_x_min = A[v].get_x() - max_boundary;
			act_x_max = A[v].get_x() + max_boundary;
			act_y_min = A[v].get_y() - max_boundary;
			act_y_max = A[v].get_y() + max_boundary;
			if(act_x_min < x_min) x_min = act_x_min;
			if(act_x_max > x_max) x_max = act_x_max;
			if(act_y_min < y_min) y_min = act_y_min;
			if(act_y_max > y_max) y_max = act_y_max;
		}
	}

	//add offset
	x_min -= minDistCC()/2;
	x_max += minDistCC()/2;
	y_min -= minDistCC()/2;
	y_max += minDistCC()/2;

	r.set_rectangle(x_max-x_min,y_max-y_min,x_min,y_min,componenet_index);
	return r;
}


void FMMMLayout::rotate_components_and_calculate_bounding_rectangles(
	List<Rectangle>&R,
	Graph G_sub[],
	NodeArray<NodeAttributes> A_sub[])
{
	int i,j;
	double sin_j,cos_j;
	double angle,act_area,act_area_PI_half_rotated,best_area;
	double ratio,new_width,new_height;
	Array<NodeArray<DPoint> > best_coords(number_of_components);
	Array<NodeArray<DPoint> > old_coords(number_of_components);
	node v_sub;
	Rectangle r_act,r_best;
	DPoint new_pos,new_dlc;

	R.clear(); //make R empty

	for(i=0;i<number_of_components;i++)
	{//allcomponents

		//init r_best, best_area and best_(old)coords
		r_best = calculate_bounding_rectangle(G_sub[i],A_sub[i],i);
		best_area =  calculate_area(r_best.get_width(),r_best.get_height(),
			number_of_components);
		best_coords[i].init(G_sub[i]);
		old_coords[i].init(G_sub[i]);

		forall_nodes(v_sub,G_sub[i])
			old_coords[i][v_sub] = best_coords[i][v_sub] = A_sub[i][v_sub].get_position();

		//rotate the components
		for(j=1;j<=stepsForRotatingComponents();j++)
		{
			//calculate new positions for the nodes, the new rectangle and area
			angle = Math::pi_2 * (double(j)/double(stepsForRotatingComponents()+1));
			sin_j = sin(angle);
			cos_j = cos(angle);
			forall_nodes(v_sub,G_sub[i])
			{
				new_pos.m_x =  cos_j * old_coords[i][v_sub].m_x
					- sin_j * old_coords[i][v_sub].m_y;
				new_pos.m_y =   sin_j * old_coords[i][v_sub].m_x
					+ cos_j * old_coords[i][v_sub].m_y;
				A_sub[i][v_sub].set_position(new_pos);
			}

			r_act = calculate_bounding_rectangle(G_sub[i],A_sub[i],i);
			act_area =  calculate_area(r_act.get_width(),r_act.get_height(),
				number_of_components);
			if(number_of_components == 1)
				act_area_PI_half_rotated =calculate_area(r_act.get_height(),
				r_act.get_width(),
				number_of_components);

			//store placement of the nodes with minimal area (in case that
			//number_of_components >1) else store placement with minimal aspect ratio area
			if(act_area < best_area)
			{
				r_best = r_act;
				best_area = act_area;
				forall_nodes(v_sub,G_sub[i])
					best_coords[i][v_sub] = A_sub[i][v_sub].get_position();
			}
			else if ((number_of_components == 1) && (act_area_PI_half_rotated < best_area))
			{ //test if rotating further with PI_half would be an improvement
				r_best = r_act;
				best_area = act_area_PI_half_rotated;
				forall_nodes(v_sub,G_sub[i])
					best_coords[i][v_sub] = A_sub[i][v_sub].get_position();
				//the needed rotation step follows in the next if statement
			}
		}

		//tipp the smallest rectangle over by angle PI/2 around the origin if it makes the
		//aspect_ratio of r_best more similar to the desired aspect_ratio
		ratio = r_best.get_width()/r_best.get_height();

		if( (pageRatio() <  1 && ratio > 1) ||  (pageRatio() >= 1 && ratio < 1) )
		{
			forall_nodes(v_sub,G_sub[i])
			{
				new_pos.m_x = best_coords[i][v_sub].m_y*(-1);
				new_pos.m_y = best_coords[i][v_sub].m_x;
				best_coords[i][v_sub] = new_pos;
			}

			//calculate new rectangle
			new_dlc.m_x = r_best.get_old_dlc_position().m_y*(-1)-r_best.get_height();
			new_dlc.m_y = r_best.get_old_dlc_position().m_x;
			new_width = r_best.get_height();
			new_height = r_best.get_width();
			r_best.set_width(new_width);
			r_best.set_height(new_height);
			r_best.set_old_dlc_position(new_dlc);
		}

		//save the computed information in A_sub and R
		forall_nodes(v_sub,G_sub[i])
			A_sub[i][v_sub].set_position(best_coords[i][v_sub]);
		R.pushBack(r_best);

	}//allcomponents
}


void FMMMLayout::export_node_positions(
	NodeArray<NodeAttributes>& A,
	List<Rectangle>&  R,
	Graph G_sub[],
	NodeArray<NodeAttributes> A_sub[])
{
	ListIterator<Rectangle> RectIterator;
	Rectangle r;
	int i;
	node v_sub;
	DPoint newpos,tipped_pos,tipped_dlc;

	for(RectIterator = R.begin();RectIterator.valid();++RectIterator)
	{//for
		r = *RectIterator;
		i = r.get_component_index();
		if(r.is_tipped_over())
		{//if
			//calculate tipped coordinates of the nodes
			forall_nodes(v_sub,G_sub[i])
			{
				tipped_pos.m_x = A_sub[i][v_sub].get_y()*(-1);
				tipped_pos.m_y = A_sub[i][v_sub].get_x();
				A_sub[i][v_sub].set_position(tipped_pos);
			}
		}//if

		forall_nodes(v_sub,G_sub[i])
		{
			newpos = A_sub[i][v_sub].get_position() + r.get_new_dlc_position()
				- r.get_old_dlc_position();
			A[A_sub[i][v_sub].get_original_node()].set_position(newpos);
		}
	}//for
}


//----------------------- functions for multilevel step -----------------------------

inline int FMMMLayout::get_max_mult_iter(int act_level, int max_level, int node_nr)
{
	int iter;
	if(maxIterChange() == micConstant) //nothing to do
		iter =  fixedIterations();
	else if (maxIterChange() == micLinearlyDecreasing) //linearly decreasing values
	{
		if(max_level == 0)
			iter = fixedIterations() +  ((maxIterFactor()-1) * fixedIterations());
		else
			iter = fixedIterations() + int((double(act_level)/double(max_level) ) *
			((maxIterFactor()-1)) * fixedIterations());
	}
	else //maxIterChange == micRapidlyDecreasing (rapidly decreasing values)
	{
		if(act_level == max_level)
			iter = fixedIterations() + int( (maxIterFactor()-1) * fixedIterations());
		else if(act_level == max_level - 1)
			iter = fixedIterations() + int(0.5 * (maxIterFactor()-1) * fixedIterations());
		else if(act_level == max_level - 2)
			iter = fixedIterations() + int(0.25 * (maxIterFactor()-1) * fixedIterations());
		else //act_level >= max_level - 3
			iter = fixedIterations();
	}

	//helps to get good drawings for small graphs and graphs with few multilevels
	if((node_nr <= 500) && (iter < 100))
		return 100;
	else
		return iter;
}


//-------------------------- functions for force calculation ---------------------------

inline void FMMMLayout::calculate_forces(
	Graph& G,
	NodeArray<NodeAttributes>& A,
	EdgeArray<EdgeAttributes>& E,
	NodeArray<DPoint>& F,
	NodeArray<DPoint>& F_attr,
	NodeArray<DPoint>& F_rep,
	NodeArray<DPoint>& last_node_movement,
	int iter,
	int fine_tuning_step)
{
	if(allowedPositions() != apAll)
		make_positions_integer(G,A);
	calculate_attractive_forces(G,A,E,F_attr);
	calculate_repulsive_forces(G,A,F_rep);
	add_attr_rep_forces(G,F_attr,F_rep,F,iter,fine_tuning_step);
	prevent_oscilations(G,F,last_node_movement,iter);
	move_nodes(G,A,F);
	update_boxlength_and_cornercoordinate(G,A);
}


void FMMMLayout::init_boxlength_and_cornercoordinate (
	Graph& G,
	NodeArray<NodeAttributes>& A)
{
	//boxlength is set

	const double MIN_NODE_SIZE = 10;
	const double BOX_SCALING_FACTOR = 1.1;
	double w=0,h=0;       //helping variables

	node v;
	forall_nodes(v,G)
	{
		w  += max(A[v].get_width(),MIN_NODE_SIZE);
		h  += max(A[v].get_height(),MIN_NODE_SIZE);
	}

	boxlength = ceil(max(w,h) * BOX_SCALING_FACTOR);

	//down left corner of comp. box is the origin
	down_left_corner.m_x = 0;
	down_left_corner.m_y = 0;
}


void FMMMLayout::create_initial_placement (Graph& G, NodeArray<NodeAttributes>& A)
{
	const int BILLION = 1000000000;
	int i,j,k;
	node v;

	if (initialPlacementForces() == ipfKeepPositions) // don't change anything
	{
		init_boxlength_and_cornercoordinate(G,A);
	}
	else if (initialPlacementForces() == ipfUniformGrid) //set nodes to the midpoints of a  grid
	{//(uniform on a grid)
		init_boxlength_and_cornercoordinate(G,A);
		int level = static_cast<int>( ceil(Math::log4(G.numberOfNodes())));
		int m     = static_cast<int>(pow(2.0,level))-1;
		bool finished = false;
		double blall = boxlength/(m+1); //boxlength for boxes at the lowest level (depth)
		Array<node> all_nodes(G.numberOfNodes());

		k = 0;
		forall_nodes(v,G)
		{
			all_nodes[k] = v;
			k++;
		}
		v = all_nodes[0];
		k = 0;
		i = 0;
		while ((!finished) && (i <= m))
		{//while1
			j = 0;
			while((!finished) && (j <= m))
			{//while2
				A[v].set_x(boxlength*i/(m+1) + blall/2);
				A[v].set_y(boxlength*j/(m+1) + blall/2);
				if(k == G.numberOfNodes()-1)
					finished = true;
				else
				{
					k++;
					v = all_nodes[k];
				}
				j++;
			}//while2
			i++;
		}//while1
	}//(uniform on a grid)
	else //randomised distribution of the nodes;
	{//(random)
		init_boxlength_and_cornercoordinate(G,A);
		if(initialPlacementForces() == ipfRandomTime)//(RANDOM based on actual CPU-time)
			srand((unsigned int)time(0));
		else if(initialPlacementForces() == ipfRandomRandIterNr)//(RANDOM based on seed)
			srand(randSeed());

		forall_nodes(v,G)
		{
			DPoint rndp;
			rndp.m_x = double(randomNumber(0,BILLION))/BILLION;//rand_x in [0,1]
			rndp.m_y = double(randomNumber(0,BILLION))/BILLION;//rand_y in [0,1]
			A[v].set_x(rndp.m_x*(boxlength-2)+ 1);
			A[v].set_y(rndp.m_y*(boxlength-2)+ 1);
		}
	}//(random)
	update_boxlength_and_cornercoordinate(G,A);
}


inline void FMMMLayout::init_F(Graph& G, NodeArray<DPoint>& F)
{
	DPoint nullpoint (0,0);
	node v;
	forall_nodes(v,G)
		F[v] = nullpoint;
}


inline void FMMMLayout::make_initialisations_for_rep_calc_classes(Graph& G)
{
	if(repulsiveForcesCalculation() == rfcExact)
		FR.make_initialisations(boxlength,down_left_corner,frGridQuotient());
	else if(repulsiveForcesCalculation() == rfcGridApproximation)
		FR.make_initialisations(boxlength,down_left_corner,frGridQuotient());
	else //(repulsiveForcesCalculation() == rfcNMM
		NM.make_initialisations(G,boxlength,down_left_corner,
		nmParticlesInLeaves(),nmPrecision(),
		nmTreeConstruction(),nmSmallCell());
}


void FMMMLayout::calculate_attractive_forces(
	Graph& G,
	NodeArray<NodeAttributes> & A,
	EdgeArray<EdgeAttributes> & E,
	NodeArray<DPoint>& F_attr)
{
	numexcept N;
	edge e;
	node u,v;
	double norm_v_minus_u,scalar;
	DPoint vector_v_minus_u,f_u;
	DPoint nullpoint (0,0);

	//initialisation
	init_F(G,F_attr);

	//calculation
	forall_edges (e,G)
	{//for
		u = e->source();
		v = e->target();
		vector_v_minus_u  = A[v].get_position() - A[u].get_position();
		norm_v_minus_u = vector_v_minus_u.norm();
		if(vector_v_minus_u == nullpoint)
			f_u = nullpoint;
		else if(!N.f_near_machine_precision(norm_v_minus_u,f_u))
		{
			scalar = f_attr_scalar(norm_v_minus_u,E[e].get_length())/norm_v_minus_u;
			f_u.m_x = scalar * vector_v_minus_u.m_x;
			f_u.m_y = scalar * vector_v_minus_u.m_y;
		}

		F_attr[v] = F_attr[v] - f_u;
		F_attr[u] = F_attr[u] + f_u;
	}//for
}


double FMMMLayout::f_attr_scalar(double d, double ind_ideal_edge_length)
{
	double s;

	if(forceModel() == fmFruchtermanReingold)
		s =  d*d/(ind_ideal_edge_length*ind_ideal_edge_length*ind_ideal_edge_length);
	else if (forceModel() == fmEades)
	{
		double c = 10;
		if (d == 0)
			s = -1e10;
		else
			s =  c * Math::log2(d/ind_ideal_edge_length) /(ind_ideal_edge_length);
	}
	else if (forceModel() == fmNew)
	{
		double c =  Math::log2(d/ind_ideal_edge_length);
		if (d > 0)
			s =  c * d * d /
			(ind_ideal_edge_length * ind_ideal_edge_length * ind_ideal_edge_length);
		else
			s = -1e10;
	}
	else cout <<" Error FMMMLayout:: f_attr_scalar"<<endl;

	return s;
}


void FMMMLayout::add_attr_rep_forces(
	Graph& G,
	NodeArray<DPoint>& F_attr,
	NodeArray<DPoint>& F_rep,
	NodeArray<DPoint>& F,
	int iter,
	int fine_tuning_step)
{
	numexcept N;
	node v;
	DPoint f,force;
	DPoint nullpoint (0,0);
	double norm_f,scalar;
	double act_spring_strength,act_rep_force_strength;

	//set cool_factor
	if(coolTemperature() == false)
		cool_factor = 1.0;
	else if((coolTemperature() == true) && (fine_tuning_step == 0))
	{
		if(iter == 1)
			cool_factor = coolValue();
		else
			cool_factor *= coolValue();
	}

	if(fine_tuning_step == 1)
		cool_factor /= 10.0; //decrease the temperature rapidly
	else if (fine_tuning_step == 2)
	{
		if(iter <= fineTuningIterations() -5)
			cool_factor = fineTuneScalar(); //decrease the temperature rapidly
		else
			cool_factor = (fineTuneScalar()/10.0);
	}

	//set the values for the spring strength and strength of the rep. force field
	if(fine_tuning_step <= 1)//usual case
	{
		act_spring_strength = springStrength();
		act_rep_force_strength = repForcesStrength();
	}
	else if(!adjustPostRepStrengthDynamically())
	{
		act_spring_strength = postSpringStrength();
		act_rep_force_strength = postStrengthOfRepForces();
	}
	else //adjustPostRepStrengthDynamically())
	{
		act_spring_strength = postSpringStrength();
		act_rep_force_strength = get_post_rep_force_strength(G.numberOfNodes());
	}

	forall_nodes(v,G)
	{
		f.m_x = act_spring_strength * F_attr[v].m_x + act_rep_force_strength * F_rep[v].m_x;
		f.m_y = act_spring_strength * F_attr[v].m_y + act_rep_force_strength * F_rep[v].m_y;
		f.m_x = average_ideal_edgelength * average_ideal_edgelength * f.m_x;
		f.m_y = average_ideal_edgelength * average_ideal_edgelength * f.m_y;

		norm_f = f.norm();
		if(f == nullpoint)
			force = nullpoint;
		else if(N.f_near_machine_precision(norm_f,force))
			restrict_force_to_comp_box(force);
		else
		{
			scalar = min (norm_f * cool_factor * forceScalingFactor(),
				max_radius(iter))/norm_f;
			force.m_x = scalar * f.m_x;
			force.m_y = scalar * f.m_y;
		}
		F[v] = force;
	}
}


void FMMMLayout::move_nodes(
	Graph& G,
	NodeArray<NodeAttributes>& A,
	NodeArray<DPoint>& F)
{
	node v;

	forall_nodes(v,G)
		A[v].set_position(A[v].get_position() + F[v]);
}


void FMMMLayout::update_boxlength_and_cornercoordinate(
	Graph& G,
	NodeArray<NodeAttributes>&A)
{
	node v;
	double xmin,xmax,ymin,ymax;
	DPoint midpoint;


	v = G.firstNode();
	midpoint = A[v].get_position();
	xmin = xmax = midpoint.m_x;
	ymin = ymax = midpoint.m_y;

	forall_nodes(v,G)
	{
		midpoint = A[v].get_position();
		if (midpoint.m_x < xmin )
			xmin = midpoint.m_x;
		if (midpoint.m_x > xmax )
			xmax = midpoint.m_x;
		if (midpoint.m_y < ymin )
			ymin = midpoint.m_y;
		if (midpoint.m_y > ymax )
			ymax = midpoint.m_y;
	}

	//set down_left_corner and boxlength

	down_left_corner.m_x = floor(xmin - 1);
	down_left_corner.m_y = floor(ymin - 1);
	boxlength = ceil(max(ymax-ymin, xmax-xmin) *1.01 + 2);

	//exception handling: all nodes have same x and y coordinate
	if(boxlength <= 2 )
	{
		boxlength = G.numberOfNodes()* 20;
		down_left_corner.m_x = floor(xmin) - (boxlength/2);
		down_left_corner.m_y = floor(ymin) - (boxlength/2);
	}

	//export the boxlength and down_left_corner values to the rep. calc. classes

	if(repulsiveForcesCalculation() == rfcExact ||
		repulsiveForcesCalculation() == rfcGridApproximation)
		FR.update_boxlength_and_cornercoordinate(boxlength,down_left_corner);
	else //repulsiveForcesCalculation() == rfcNMM
		NM.update_boxlength_and_cornercoordinate(boxlength,down_left_corner);
}


void FMMMLayout::set_average_ideal_edgelength(
	Graph& G,
	EdgeArray<EdgeAttributes>& E)
{
	double averagelength = 0;
	edge e;

	if(G.numberOfEdges() > 0)
	{
		forall_edges(e,G)
			averagelength += E[e].get_length();
		average_ideal_edgelength = averagelength/G.numberOfEdges();
	}
	else
		average_ideal_edgelength = 50;
}


double FMMMLayout::get_average_forcevector_length (Graph& G, NodeArray<DPoint>& F)
{
	double lengthsum = 0;
	node v;
	forall_nodes(v,G)
		lengthsum += F[v].norm();
	lengthsum /=G.numberOfNodes();
	return lengthsum;
}


void FMMMLayout::prevent_oscilations(
	Graph& G,
	NodeArray<DPoint>& F,
	NodeArray<DPoint>& last_node_movement,
	int iter)
{

	const double pi_times_1_over_6 = 0.52359878;
	const double pi_times_2_over_6 = 2 * pi_times_1_over_6;
	const double pi_times_3_over_6 = 3 * pi_times_1_over_6;
	const double pi_times_4_over_6 = 4 * pi_times_1_over_6;
	const double pi_times_5_over_6 = 5 * pi_times_1_over_6;
	const double pi_times_7_over_6 = 7 * pi_times_1_over_6;
	const double pi_times_8_over_6 = 8 * pi_times_1_over_6;
	const double pi_times_9_over_6 = 9 * pi_times_1_over_6;
	const double pi_times_10_over_6 = 10 * pi_times_1_over_6;
	const double pi_times_11_over_6 = 11 * pi_times_1_over_6;

	DPoint nullpoint (0,0);
	double fi; //angle in [0,2pi) measured counterclockwise
	double norm_old,norm_new,quot_old_new;

	if (iter > 1) //usual case
	{//if1
		node v;
		forall_nodes(v,G)
		{
			DPoint force_new (F[v].m_x,F[v].m_y);
			DPoint force_old (last_node_movement[v].m_x,last_node_movement[v].m_y);
			norm_new = F[v].norm();
			norm_old  = last_node_movement[v].norm();
			if ((norm_new > 0) && (norm_old > 0))
			{//if2
				quot_old_new =  norm_old / norm_new;

				//prevent oszilations
				fi = angle(nullpoint,force_old,force_new);
				if(((fi <= pi_times_1_over_6)||(fi >= pi_times_11_over_6))&&
					((norm_new > (norm_old*2.0))) )
				{
					F[v].m_x = quot_old_new * 2.0 * F[v].m_x;
					F[v].m_y = quot_old_new * 2.0 * F[v].m_y;
				}
				else if ((fi >= pi_times_1_over_6)&&(fi <= pi_times_2_over_6)&&
					(norm_new > (norm_old*1.5) ) )
				{
					F[v].m_x = quot_old_new * 1.5 * F[v].m_x;
					F[v].m_y = quot_old_new * 1.5 * F[v].m_y;
				}
				else if ((fi >= pi_times_2_over_6)&&(fi <= pi_times_3_over_6)&&
					(norm_new > (norm_old)) )
				{
					F[v].m_x = quot_old_new * F[v].m_x;
					F[v].m_y = quot_old_new * F[v].m_y;
				}
				else if ((fi >= pi_times_3_over_6)&&(fi <= pi_times_4_over_6)&&
					(norm_new > (norm_old*0.66666666)) )
				{
					F[v].m_x = quot_old_new * 0.66666666 * F[v].m_x;
					F[v].m_y = quot_old_new * 0.66666666 * F[v].m_y;
				}
				else if ((fi >= pi_times_4_over_6)&&(fi <= pi_times_5_over_6)&&
					(norm_new > (norm_old*0.5)) )
				{
					F[v].m_x = quot_old_new * 0.5 * F[v].m_x;
					F[v].m_y = quot_old_new * 0.5 * F[v].m_y;
				}
				else if ((fi >= pi_times_5_over_6)&&(fi <= pi_times_7_over_6)&&
					(norm_new > (norm_old*0.33333333)) )
				{
					F[v].m_x = quot_old_new * 0.33333333 * F[v].m_x;
					F[v].m_y = quot_old_new * 0.33333333 * F[v].m_y;
				}
				else if ((fi >= pi_times_7_over_6)&&(fi <= pi_times_8_over_6)&&
					(norm_new > (norm_old*0.5)) )
				{
					F[v].m_x = quot_old_new * 0.5 * F[v].m_x;
					F[v].m_y = quot_old_new * 0.5 * F[v].m_y;
				}
				else if ((fi >= pi_times_8_over_6)&&(fi <= pi_times_9_over_6)&&
					(norm_new > (norm_old*0.66666666)) )
				{
					F[v].m_x = quot_old_new * 0.66666666 * F[v].m_x;
					F[v].m_y = quot_old_new * 0.66666666 * F[v].m_y;
				}
				else if ((fi >= pi_times_9_over_6)&&(fi <= pi_times_10_over_6)&&
					(norm_new > (norm_old)) )
				{
					F[v].m_x = quot_old_new * F[v].m_x;
					F[v].m_y = quot_old_new * F[v].m_y;
				}
				else if ((fi >= pi_times_10_over_6)&&(fi <= pi_times_11_over_6)&&
					(norm_new > (norm_old*1.5) ) )
				{
					F[v].m_x = quot_old_new * 1.5 * F[v].m_x;
					F[v].m_y = quot_old_new * 1.5 * F[v].m_y;
				}
			}//if2
			last_node_movement[v]= F[v];
		}
	}//if1
	else if (iter == 1)
		init_last_node_movement(G,F,last_node_movement);
}


double FMMMLayout::angle(DPoint& P, DPoint& Q, DPoint& R)
{
	double dx1 = Q.m_x - P.m_x;
	double dy1 = Q.m_y - P.m_y;
	double dx2 = R.m_x - P.m_x;
	double dy2 = R.m_y - P.m_y;
	double fi;//the angle

	if ((dx1 == 0 && dy1 == 0) || (dx2 == 0 && dy2 == 0))
		cout<<"Multilevel::angle()"<<endl;

	double norm  = (dx1*dx1+dy1*dy1)*(dx2*dx2+dy2*dy2);
	double cosfi = (dx1*dx2+dy1*dy2) / sqrt(norm);

	if (cosfi >=  1.0 ) fi = 0;
	if (cosfi <= -1.0 ) fi = Math::pi;
	else
	{
		fi = acos(cosfi);
		if (dx1*dy2 < dy1*dx2) fi = -fi;
		if (fi < 0) fi += 2*Math::pi;
	}
	return fi;
}


void FMMMLayout::init_last_node_movement(
	Graph& G,
	NodeArray<DPoint>& F,
	NodeArray<DPoint>& last_node_movement)
{
	node v;
	forall_nodes(v,G)
		last_node_movement[v]= F[v];
}


void FMMMLayout::adapt_drawing_to_ideal_average_edgelength(
	Graph& G,
	NodeArray<NodeAttributes>& A,
	EdgeArray<EdgeAttributes>& E)
{
	edge e;
	node v;
	double sum_real_edgelength = 0;
	double sum_ideal_edgelength = 0;
	double area_scaling_factor;
	DPoint new_pos;

	forall_edges(e,G)
	{
		sum_ideal_edgelength += E[e].get_length();
		sum_real_edgelength += (A[e->source()].get_position() - A[e->target()].get_position()).norm();
	}

	if(sum_real_edgelength == 0) //very very unlike case
		area_scaling_factor = 1;
	else
		area_scaling_factor = sum_ideal_edgelength/sum_real_edgelength;

	forall_nodes(v,G)
	{
		new_pos.m_x = resizingScalar() * area_scaling_factor * A[v].get_position().m_x;
		new_pos.m_y = resizingScalar() * area_scaling_factor * A[v].get_position().m_y;
		A[v].set_position(new_pos);
	}
}


} //end namespace ogdf
