/*
 * $Revision: 2565 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-07 17:14:54 +0200 (Sa, 07. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Implementation of class MinCostFlowReinelt.
 *
 * \author Gerhard Reinelt, various adaptions by Carsten Gutwenger
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

#include <ogdf/graphalg/MinCostFlowReinelt.h>


namespace ogdf {


void MinCostFlowReinelt::start(Array<int> &supply)
{
	/*----------------------------------------------------------------------*/
	/*     determine intial basis tree and initialize data structure        */
	/*----------------------------------------------------------------------*/

	/* initialize artificial root node */
	root->father = root;
	root->successor = &nodes[1];
	root->arc_id = NULL;
	root->orientation = false;
	root->dual = 0;
	root->flow = 0;
	root->nr_of_nodes = nn + 1;
	root->last = &nodes[nn];
	root->name = nn + 1;
	// artificials = nn; moved to mcf() [CG]
	int highCost = 1 + (nn+1) * m_maxCost;

	for (int i = 1; i <= nn; i++)
	{   /* for every node an artificial arc is created */
		arctype *ep = OGDF_NEW arctype;
		if (supply[i - 1] >= 0) {
			ep->tail = &nodes[i];
			ep->head = root;
		} else {
			ep->tail = root;
			ep->head = &nodes[i];
		}
		ep->cost = highCost;
		ep->upper_bound = infinity();
		ep->arcnum = mm + i - 1;
		ep->next_arc = start_b;
		start_b = ep;
		nodes[i].father = root;
		if (i < nn)
			nodes[i].successor = &nodes[i+1];
		else
			nodes[i].successor = root;
		if (supply[i - 1] < 0) {
			nodes[i].orientation = false;
			nodes[i].dual = -highCost;
		} else {
			nodes[i].orientation = true;
			nodes[i].dual = highCost;
		}
		nodes[i].flow = abs(supply[i - 1]);
		nodes[i].nr_of_nodes = 1;
		nodes[i].last = &nodes[i];
		nodes[i].arc_id = ep;
	}  /* for i */
	start_n1 = start_arc;
}  /*start*/



/***************************************************************************/
/*             circle variant for determine basis entering arc             */
/***************************************************************************/

void MinCostFlowReinelt::beacircle(
	arctype **eplus,
	arctype **pre,
	bool *from_ub)
{
	//the first arc with negative reduced costs is taken, but the search is
	//started at the successor of the successor of eplus in the last iteration

	bool found = false;   /* true<=>entering arc found */

	*pre = startsearch;
	if (*pre != NULL)
		*eplus = (*pre)->next_arc;
	else
		*eplus = NULL;
	searchend = *eplus;

	if (!*from_ub) {

		while (*eplus != NULL && !found)
		{  /* search in n' for an arc with negative reduced costs */
			if ((*eplus)->cost + (*eplus)->head->dual
				- (*eplus)->tail->dual < 0)
			{
				found = true;
			} else {
				*pre = *eplus;   /* save predecessor */
				*eplus = (*eplus)->next_arc;   /* go to next arc */
			}
		}  /* while */

		if (!found)   /* entering arc still not found */
		{  /* search in n'' */
			*from_ub = true;
			*eplus = start_n2;
			*pre = NULL;

			while (*eplus != NULL && !found) {
				if ((*eplus)->tail->dual -
					(*eplus)->head->dual - (*eplus)->cost < 0)
				{
					found = true;
				} else {
					*pre = *eplus;   /* save predecessor */
					*eplus = (*eplus)->next_arc;   /* go to next arc */
				}

			}  /* while */


			if (!found) {   /* search again in n' */
				*from_ub = false;
				*eplus = start_n1;
				*pre = NULL;

				while (*eplus != searchend && !found) {
					if ((*eplus)->cost +
						(*eplus)->head->dual - (*eplus)->tail->dual < 0)
					{
						found = true;
					} else {
						*pre = *eplus;   /* save predecessor */
						*eplus = (*eplus)->next_arc;   /* go to next arc */
					}
				}  /* while */

			}  /* search in n'' */
		}  /* serch again in n' */
	}  /* if from_ub */
	else {  /* startsearch in n'' */

		while (*eplus != NULL && !found) {
			if ((*eplus)->tail->dual -
				(*eplus)->head->dual - (*eplus)->cost < 0)
			{
				found = true;
			} else {
				*pre = *eplus;   /* save predecessor */
				*eplus = (*eplus)->next_arc;   /* go to next arc */
			}
		}  /* while */

		if (!found) {   /* search now in n' */
			*from_ub = false;
			*eplus = start_n1;
			*pre = NULL;

			while (*eplus != NULL && !found) {
				if ((*eplus)->cost +
					(*eplus)->head->dual - (*eplus)->tail->dual < 0)
				{
					found = true;
				} else {
					*pre = *eplus;   /* save predecessor */
					*eplus = (*eplus)->next_arc;   /* go to next arc */
				}
			}  /* while */


			if (!found) {   /* search again in n'' */
				*from_ub = true;
				*eplus = start_n2;
				*pre = NULL;

				while (*eplus != searchend && !found) {
					if ((*eplus)->tail->dual -
						(*eplus)->head->dual - (*eplus)->cost < 0)
					{
						found = true;
					} else {
						*pre = *eplus;   /* save predecessor */
						*eplus = (*eplus)->next_arc;   /* go to next arc */
					}
				}  /* while */

			}  /* search in n' */
		}  /* search in n'' */
	}  /* from_ub = true */



	if (!found) {
		*pre = NULL;
		*eplus = NULL;
	} else
		startsearch = (*eplus)->next_arc;

}  /* beacircle */



/***************************************************************************/
/*       doublecircle variant for determine basis entering arc             */
/***************************************************************************/

void MinCostFlowReinelt::beadouble(
	arctype **eplus,
	arctype **pre,
	bool *from_ub)
{

	/* search as in procedure beacircle, but in each list the search started is
	at the last movement
	*/
	bool found = false;   /* true<=>entering arc found */

	if (!*from_ub) {
		*pre = last_n1;
		if (*pre != NULL)
			*eplus = (*pre)->next_arc;
		else
			*eplus = NULL;
		searchend_n1 = *eplus;

		while (*eplus != NULL && !found)
		{  /* search in n' for an arc with negative reduced costs */
			if ((*eplus)->cost +
				(*eplus)->head->dual - (*eplus)->tail->dual < 0)
			{
				found = true;
			} else {
				*pre = *eplus;   /* save predecessor */
				*eplus = (*eplus)->next_arc;   /* go to next arc */
			}
		}  /* while */

		if (!found)   /* entering arc still not found */
		{  /* search in n'' beginning at the last movement */
			*from_ub = true;
			*pre = last_n2;
			if (*pre != NULL)
				*eplus = (*pre)->next_arc;
			else
				*eplus = NULL;
			searchend_n2 = *eplus;

			while (*eplus != NULL && !found) {
				if ((*eplus)->tail->dual -
					(*eplus)->head->dual - (*eplus)->cost < 0)
				{
					found = true;
				} else {
					*pre = *eplus;   /* save predecessor */
					*eplus = (*eplus)->next_arc;   /* go to next arc */
				}

			}  /* while */

			if (!found)   /* entering arc still not found */
			{  /* search in n'' in the first part of the list */
				*eplus = start_n2;
				*pre = NULL;

				while (*eplus != searchend_n2 && !found) {
					if ((*eplus)->tail->dual -
						(*eplus)->head->dual - (*eplus)->cost < 0)
					{
						found = true;
					} else {
						*pre = *eplus;   /* save predecessor */
						*eplus = (*eplus)->next_arc;   /* go to next arc */
					}

				}  /* while */


				if (!found) {
					/* search again in n' in the first part of the list*/
					*from_ub = false;
					*eplus = start_n1;
					*pre = NULL;

					while (*eplus != searchend_n1 && !found) {
						if ((*eplus)->cost +
							(*eplus)->head->dual - (*eplus)->tail->dual < 0)
						{
							found = true;
						} else {
							*pre = *eplus;   /* save predecessor */
							*eplus = (*eplus)->next_arc;  /* go to next arc */
						}
					}  /* while */
				}  /* first part n' */
			}  /* first part n'' */
		}  /* second part n'' */
	}  /* if from_ub */
	else {  /* startsearch in n'' */
		*pre = last_n2;
		if (*pre != NULL)
			*eplus = (*pre)->next_arc;
		else
			*eplus = NULL;
		searchend_n2 = *eplus;

		while (*eplus != NULL && !found) {
			if ((*eplus)->tail->dual -
				(*eplus)->head->dual - (*eplus)->cost < 0)
			{
				found = true;
			} else {
				*pre = *eplus;   /* save predecessor */
				*eplus = (*eplus)->next_arc;   /* go to next arc */
			}
		}  /* while */

		if (!found) {   /* search now in n' beginning at the last movement */
			*from_ub = false;
			*pre = last_n1;
			if (*pre != NULL)
				*eplus = (*pre)->next_arc;
			else
				*eplus = NULL;
			searchend_n1 = *eplus;

			while (*eplus != NULL && !found) {
				if ((*eplus)->cost +
					(*eplus)->head->dual - (*eplus)->tail->dual < 0)
				{
					found = true;
				} else {
					*pre = *eplus;   /* save predecessor */
					*eplus = (*eplus)->next_arc;   /* go to next arc */
				}
			}  /* while */


			if (!found) {   /* search now in n' in the first part */
				*eplus = start_n1;
				*pre = NULL;

				while (*eplus != searchend_n1 && !found) {
					if ((*eplus)->cost +
						(*eplus)->head->dual - (*eplus)->tail->dual < 0)
					{
						found = true;
					} else {
						*pre = *eplus;   /* save predecessor */
						*eplus = (*eplus)->next_arc;   /* go to next arc */
					}
				}  /* while */


				if (!found) {   /* search again in n'' in the first part */
					*from_ub = true;
					*eplus = start_n2;
					*pre = NULL;

					while (*eplus != searchend_n2 && !found) {
						if ((*eplus)->tail->dual -
							(*eplus)->head->dual - (*eplus)->cost < 0)
						{
							found = true;
						} else {
							*pre = *eplus;   /* save predecessor */
							*eplus = (*eplus)->next_arc;   /* go to next arc */
						}
					}  /* while */
				}  /* first part of n'' */
			}  /* first part of n' */
		}  /* second part of n' */
	}  /* from_ub = true */



	if (!found) {
		*pre = NULL;
		*eplus = NULL;
		return;
	}

	if (*from_ub)
		last_n2 = (*eplus)->next_arc;
	else
		last_n1 = (*eplus)->next_arc;

}  /* beadouble */



/***************************************************************************/
/*                  Min Cost Flow Function                                 */
/***************************************************************************/


int MinCostFlowReinelt::mcf(
	int mcfNrNodes,
	int mcfNrArcs,
	Array<int> &supply,
	Array<int> &mcfTail,
	Array<int> &mcfHead,
	Array<int> &mcfLb,
	Array<int> &mcfUb,
	Array<int> &mcfCost,
	Array<int> &mcfFlow,
	Array<int> &mcfDual,
	int *mcfObj)
{
	int i;
	int low,up;

	/************************************************/
	/* 1: Allocations (malloc's no longer required) */
	/************************************************/

	root = &rootStruct;


	/**********************/
	/* 2: Initializations */
	/**********************/

	/* Number of nodes/arcs */
	nn = mcfNrNodes;
	OGDF_ASSERT(nn >= 2);
	mm = mcfNrArcs;
	OGDF_ASSERT(mm >= 2);

	// number of artificial basis arcs
	int artificials = nn;


	/* Node space and pointers to nodes */
	nodes.init(nn+1);
	nodes[0].name = -1; // for debuggin, should not occur(?)
	for(i = 1; i <= nn; ++i)
		nodes[i].name = i;

	/* Arc space and arc data */
	arcs.init(mm+1);

	int lb_cost = 0; // cost of lower bound
	m_maxCost = 0;
	int from = mcfTail[0]; // name of tail (input)
	int toh = mcfHead[0];  // name of head (input)
	low = mcfLb[0];
	up = mcfUb[0];
	int c = mcfCost[0]; // cost (input)
	if (from<=0 || from>nn || toh<=0 || toh>nn || up<0 || low>up || low<0) {
		return 4;
	}
	if(abs(c) > m_maxCost) m_maxCost = abs(c);

	start_arc = &arcs[1];
	start_arc->tail = &nodes[from];
	start_arc->head = &nodes[toh];
	start_arc->cost = c;
	start_arc->upper_bound = up - low;
	start_arc->arcnum = 0;
	supply[from - 1] -= low;
	supply[toh - 1] += low;
	lb_cost += start_arc->cost * low;

	arctype *e = start_arc;

	int l; // lower bound (input)
	for (l=2;l<=mm;l++) {
		from = mcfTail[l-1];
		toh = mcfHead[l-1];
		low = mcfLb[l-1];
		up = mcfUb[l-1];
		c = mcfCost[l-1];
		if (from<=0 || from>nn || toh<=0 || toh>nn ||
			up<0 || low>up || low<0)
		{
			return 4;
		}
		if(abs(c) > m_maxCost) m_maxCost = abs(c);

		arctype *ep = &arcs[l];
		e->next_arc = ep;
		ep->tail = &nodes[from];
		ep->head = &nodes[toh];
		ep->cost = c;
		ep->upper_bound = up - low;
		ep->arcnum = l-1;
		supply[from-1] -= low;
		supply[toh-1] += low;
		lb_cost += ep->cost * low;
		e = ep;
	}

	e->next_arc = NULL;
	// feasible = true <=> feasible solution exists
	bool feasible = true;


	/************************/
	/* 3: Starting solution */
	/************************/

	start_n1 = NULL;
	start_n2 = NULL;
	start_b = NULL;

	start(supply);

	int step = 1;   /* initialize iteration counter */

	/*********************/
	/* 4: Iteration loop */
	/*********************/

	/*************************************/
	/* 4.1: Determine basis entering arc */
	/*************************************/

	// finished = true <=> iteration finished
	bool finished = false;
	// from_ub = true <=> entering arc at upper bound
	bool from_ub = false;
	startsearch = start_n1;
	//startsearchpre = NULL;
	last_n1 = NULL;
	last_n2 = NULL;
	nodetype *np; // general nodeptr

	do {
		arctype *eplus; // ->basis entering arc
		arctype *pre;   // ->predecessor of eplus in list
		beacircle(&eplus, &pre, &from_ub);

		if (eplus == NULL) {
			finished = true;
		} else {

			nodetype *iplus = eplus->tail; // -> tail of basis entering arc
			nodetype *jplus = eplus->head; // -> head of basis entering arc

			/******************************************************/
			/* 4.2: Determine leaving arc and maximal flow change */
			/******************************************************/

			int delta = eplus->upper_bound; // maximal flow change
			nodetype *iminus = NULL; // -> tail of basis leaving arc
			nodetype *p1 = iplus;
			nodetype *p2 = jplus;

			bool to_ub;   // to_ub = true <=> leaving arc goes to upperbound
			bool xchange; // xchange = true <=> exchange iplus and jplus
			while (p1 != p2) {
				if (p1->nr_of_nodes<=p2->nr_of_nodes) {
					np = p1;
					if (from_ub==np->orientation) {
						if (delta > np->arc_id->upper_bound - np->flow) {
							iminus = np;
							delta = np->arc_id->upper_bound - np->flow;
							xchange = false;
							to_ub = true;
						}
					}
					else if (delta>np->flow) {
						iminus = np;
						delta = np->flow;
						xchange = false;
						to_ub = false;
					}
					p1 = np->father;
					continue;
				}
				np = p2;
				if (from_ub != np->orientation) {
					if (delta > np->arc_id->upper_bound - np->flow) {
						iminus = np;
						delta = np->arc_id->upper_bound - np->flow;
						xchange = true;
						to_ub = true;
					}
				}
				else if (delta > np->flow) {
					iminus = np;
					delta = np->flow;
					xchange = true;
					to_ub = false;
				}
				p2 = np->father;
			}
			// paths from iplus and jplus to root meet at w
			nodetype *w = p1;
			nodetype *iw;
			nodetype *jminus;  // -> head of basis leaving arc

			arctype *eminus; /// ->basis leaving arc
			if (iminus == NULL) {
				to_ub = !from_ub;
				eminus = eplus;
				iminus = iplus;
				jminus = jplus;
			}
			else {
				if (xchange) {
					iw = jplus;
					jplus = iplus;
					iplus = iw;
				}
				jminus = iminus->father;
				eminus = iminus->arc_id;
			}

			// artif_to_lb = true <=> artif. arc goes to lower bound
			bool artif_to_lb = false;
			if (artificials>1) {
				if (iminus==root || jminus==root) {
					if (jplus!=root && iplus!=root) {
						artificials--;
						artif_to_lb = true;
					}
					else if (eminus==eplus) {
						if (from_ub) {
							artificials--;
							artif_to_lb = true;
						} else
							artificials++;
					}
				}
				else {
					if (iplus == root || jplus == root)
						artificials++;
				}
			}

			/*********************************/
			/* 4.3: Update of data structure */
			/*********************************/

			int sigma; // change of dual variables

			if (eminus==eplus) {
				if (from_ub) delta = -delta;

				bool s_orientation;
				if (eminus->tail==iplus) s_orientation = true;
				else s_orientation = false;

				np = iplus;
				while (np!=w) {
					if (np->orientation==s_orientation) {
						np->flow -= delta;
					}
					else {
						np->flow += delta;
					}
					np = np->father;
				}

				np = jplus;
				while (np!=w) {
					if (np->orientation==s_orientation) {
						np->flow += delta;
					}
					else {
						np->flow -= delta;
					}
					np = np->father;
				}

			} else {
				/* 4.3.2.1 : initialize sigma */

				if (eplus->tail==iplus)
					sigma = eplus->cost + jplus->dual - iplus->dual;
				else
					sigma = jplus->dual - iplus->dual - eplus->cost;

				// 4.3.2.2 : find new succ. of jminus if current succ. is iminus

				nodetype *newsuc = jminus->successor; // -> new successor
				if (newsuc==iminus) {
					for (i=1; i<=iminus->nr_of_nodes; i++)
						newsuc = newsuc->successor;
				}

				/* 4.3.2.3 : initialize data for iplus */

				nodetype *s_father = jplus; // save area
				bool s_orientation;
				if (eplus->tail==jplus) s_orientation = false;
				else s_orientation = true;

				// eplus_ori = true <=> eplus=(iplus,jplus)
				bool eplus_ori = s_orientation;

				int s_flow;
				if (from_ub) {
					s_flow = eplus->upper_bound - delta;
					delta = -delta;
				}
				else
					s_flow = delta;

				arctype *s_arc_id = eminus;
				int oldnumber = 0;
				nodetype *nd = iplus;     // -> current node
				nodetype *f = nd->father; // ->father of nd

				/* 4.3.2.4 : traverse subtree under iminus */

				while (nd!=jminus) {
					nodetype *pred = f; // ->predecessor of current node
					while (pred->successor != nd) pred = pred->successor;
					nodetype *lastnode = nd; // -> last node of subtree
					i = 1;
					int non = nd->nr_of_nodes - oldnumber;
					while (i<non) {
						lastnode = lastnode->successor;
						lastnode->dual += sigma;
						i++;
					}
					nd->dual += sigma;
					pred->successor = lastnode->successor;

					if (nd!=iminus) lastnode->successor = f;
					else lastnode->successor = jplus->successor;

					nodetype *w_father = nd; // save area
					arctype *w_arc_id = nd->arc_id; // save area

					bool w_orientation;
					if (nd->arc_id->tail==nd) w_orientation = false;
					else w_orientation = true;

					int w_flow;
					if (w_orientation==eplus_ori) {
						w_flow = nd->flow + delta;
					}
					else {
						w_flow = nd->flow - delta;
					}

					nd->father = s_father;
					nd->orientation = s_orientation;
					nd->arc_id = s_arc_id;
					nd->flow = s_flow;
					s_father = w_father;
					s_orientation = w_orientation;
					s_arc_id = w_arc_id;
					s_flow = w_flow;

					oldnumber = nd->nr_of_nodes;
					nd = f;
					f = f->father;

				}

				jminus->successor = newsuc;
				jplus->successor = iplus;

				// 4.3.2.5: assign new nr_of_nodes in path from iminus to iplus

				oldnumber = iminus->nr_of_nodes;
				np = iminus;
				while (np!=iplus) {
					np->nr_of_nodes = oldnumber - np->father->nr_of_nodes;
					np = np->father;
				}

				iplus->nr_of_nodes = oldnumber;

				// 4.3.2.6: update flows and nr_of_nodes in path from jminus to w

				np = jminus;
				while (np!=w) {
					np->nr_of_nodes -= oldnumber;
					if (np->orientation!=eplus_ori) {
						np->flow += delta;
					}
					else {
						np->flow -= delta;
					}
					np = np->father;
				}

				// 4.3.2.7 update flows and nr_of_nodes in path from jplus to w

				np = jplus;
				while (np!=w) {
					np->nr_of_nodes += oldnumber;
					if (np->orientation == eplus_ori) {
						np->flow += delta;
					}
					else {
						np->flow -= delta;
					}
					np = np->father;
				}

			}

			/***********************************/
			/* 4.4: Update lists B, N' and N'' */
			/***********************************/

			if (eminus==eplus) {
				if (!from_ub) {
					if (pre==NULL)
						start_n1 = eminus->next_arc;
					else
						pre->next_arc = eminus->next_arc;

					eminus->next_arc = start_n2;
					start_n2 = eminus;
				} else {
					if (pre==NULL)
						start_n2 = eminus->next_arc;
					else
						pre->next_arc = eminus->next_arc;
					eminus->next_arc = start_n1;
					start_n1 = eminus;
				}
			} else {
				int wcost = eminus->cost;
				int wub = eminus->upper_bound;
				int wnum = eminus->arcnum;
				nodetype *w_head = eminus->head;
				nodetype *w_tail = eminus->tail;
				eminus->tail = eplus->tail;
				eminus->head = eplus->head;
				eminus->upper_bound = eplus->upper_bound;
				eminus->arcnum = eplus->arcnum;
				eminus->cost = eplus->cost;
				eplus->tail = w_tail;
				eplus->head = w_head;
				eplus->upper_bound = wub;
				eplus->cost = wcost;
				eplus->arcnum = wnum;
				arctype *ep = eplus;

				if (pre!=NULL)
					pre->next_arc = ep->next_arc;
				else {
					if (from_ub) start_n2 = ep->next_arc;
					else start_n1 = ep->next_arc;
				}

				if (to_ub) {
					ep->next_arc = start_n2;
					start_n2 = ep;
				} else {
					if (!artif_to_lb) {
						ep->next_arc = start_n1;
						start_n1 = ep;
					}
				}
			}

			step++;

			/***********************************************************/
			/* 4.5: Eliminate artificial arcs and artificial root node */
			/***********************************************************/

			if (artificials==1) {
				artificials = 0;
				nodetype *nd = root->successor;
				arctype *e1 = nd->arc_id;

				if (nd->flow>0) {
					feasible = false;
					finished = true;
				} else {
					feasible = true;
					if (e1==start_b) {
						start_b = e1->next_arc;
					} else {
						e = start_b;
						while (e->next_arc!=e1)
							e = e->next_arc;
						e->next_arc = e1->next_arc;
					}

					iw = root;
					root = root->successor;
					root->father = root;
					sigma = root->dual;

					np = root;
					while (np->successor!=iw) {
						np->dual -= sigma;
						np = np->successor;
					}

					np->dual -= sigma;
					np->successor = root;

				}

			}

		}

	} while (!finished);

	/*********************/
	/* 5: Return results */
	/*********************/

	/* Feasible solution? */
	if (artificials!=0 && feasible) {
		np = root->successor;
		do {
			if (np->father==root && np->flow>0) {
				feasible = false;
				np = root;
			}
			else
				np = np->successor;
		} while (np != root);

		arctype *ep = start_n2;
		while (ep!=NULL && feasible) {
			if (ep==NULL)
				break;
			if (ep->tail==root && ep->head==root)
				feasible = false;
			ep = ep->next_arc;
		}
	}

	int retValue;

	if (feasible) {

		/* Objective function value */
		int zfw = 0; // current total cost
		np = root->successor;
		while (np!=root) {
			if (np->flow!=0)
				zfw += np->flow * np->arc_id->cost;
			np = np->successor;
		}
		arctype *ep = start_n2;
		while (ep!=NULL) {
			zfw += ep->cost * ep->upper_bound;
			ep = ep->next_arc;
		}
		*mcfObj = zfw + lb_cost;

		/* Dual variables */
		// CG: removed computation of duals
		np = root->successor;
		while (np != root) {
			mcfDual[np->name-1] = np->dual;
			np = np->successor;
		}
		mcfDual[root->name-1] = root->dual;

		/* Arc flows */
		for (i=0;i<mm;i++)
			mcfFlow[i] = mcfLb[i];

		np = root->successor;
		while (np != root) {
			// flow on artificial arcs has to be 0 to be ignored! [CG]
			OGDF_ASSERT(np->arc_id->arcnum < mm || np->flow == 0);

			if (np->arc_id->arcnum < mm)
				mcfFlow[np->arc_id->arcnum] += np->flow;

			np = np->successor;
		}

		ep = start_n2;
		while (ep!=NULL) {
			mcfFlow[ep->arcnum] += ep->upper_bound;
			ep = ep->next_arc;
		}

		retValue = 0;

	} else {
		retValue = 10;
	}

	// deallocate artificial arcs
	for(i = 1; i <= nn; ++i)
		//delete p[i]->arc_id;
		delete nodes[i].arc_id;

	return retValue;
}


} // end namespace ogdf
