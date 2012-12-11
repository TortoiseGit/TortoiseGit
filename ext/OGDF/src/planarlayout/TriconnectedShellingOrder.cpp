/*
 * $Revision: 2599 $
 *
 * last checkin:
 *   $Author: chimani $
 *   $Date: 2012-07-15 22:39:24 +0200 (So, 15. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Implements class TriconnectedShellingOrder which computes
 * a shelling order for a triconnected planar graph.
 *
 * \author Bernd Zey
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


#include <ogdf/planarlayout/TriconnectedShellingOrder.h>
#include <ogdf/basic/CombinatorialEmbedding.h>
#include <ogdf/basic/FaceArray.h>
#include <ogdf/basic/SList.h>
#include <ogdf/basic/extended_graph_alg.h>
#include <ogdf/basic/simple_graph_alg.h>

// define for debugging
//#define OUTPUT_TSO

namespace ogdf {



//---------------------------------------------------------
// class ComputeTricOrder
//---------------------------------------------------------
class ComputeTricOrder
{
public:
	// constructor
	ComputeTricOrder (
			const Graph& G,                  	// biconnected planar graph
			ConstCombinatorialEmbedding& E, 	// combinatorial embedding of G
			face outerFace,              		// outer face
			double baseRatio,          			// size of base (baseRatio * size(extFace)
			bool preferNodes = false); 			// boolean value if nodes are prefered to faces



	// initialize the nodes of the external face
	void initOuterNodes(node v1, node v2);
	// initialize the edges of the external face
	void initOuterEdges();

	// compute the node n of face f that belongs to the outer face and has degree 2 or is node v2
	node getOuterNodeDeg2(face f, NodeArray<adjEntry>& adjPred, NodeArray<adjEntry>& adjSucc);

	// gets the next possible face or node
	// ordering depends on m_preferNodes
	void getNextPossible(node& v, face& f);

	// update all nodes/faces depending on m_updateNodes/Faces
	void doUpdate();


	// initialize the possible lists with v:=v_n
	void initPossible(node v){
		m_nodesLink[v] = m_possibleNodes.pushBack(v);
	}

	// returns true <=> there are possible nodes or faces
	bool isPossible(){
		return (!(m_possibleNodes.empty() && m_possibleFaces.empty()));
	}

	// returns true <=> the current selection is a node
	bool isNode(){
		return 	m_currentIsNode;
	}

	// test if face f has only one edge on outer face
	bool isOnlyEdge(face f){
		return ((m_outv[f] == 2) && (m_oute[f] == 1));
	}

	// add a node v of face f to the outer face
	void addOuterNode(node v, face f){
		incOutv(f);
		m_outerNodes[f].pushBack(v);
		if (m_isSeparationFace[f])
			m_sepf[v]++;
	}

	void incVisited(node v){
		m_visited[v]++;
		setUpdate(v);
	}

	void incSepf(node v){
		m_sepf[v]++;
		setUpdate(v);
	}

	void decSepf(node v){
		m_sepf[v]--;
		setUpdate(v);
	}

	void incOutv(face f){
		m_outv[f]++;
		setUpdate(f);
	}

	void incOute(face f){
		m_oute[f]++;
		setUpdate(f);
	}

	int getOutv (face f) {
		return m_outv[f];
	}

	void output(){
		cout << "ComputeTricOrder::output():" << endl;
		cout << "nodes: " << endl;
		node n;
		forall_nodes(n, (*m_pGraph)){
			cout << " node " << n << ": ";
			cout << "m_visited == " << m_visited[n] << ", ";
			cout << "m_sepf == " << m_sepf[n] << endl;
		}
		cout << "faces: " << endl;
		face f;
		forall_faces(f, (*m_pEmbedding)){
			cout << " face " << f->index() << ": ";
			cout << "  outv == " << m_outv[f] << ", ";
			cout << "  oute == " << m_oute[f] << ", ";
			cout << "  isSpearationFace == " << m_isSeparationFace[f] << endl;
			cout << "  nodes in outerNodes: ";
			ListIterator<node> it = m_outerNodes[f].begin();
			while (it.valid()){
				cout << " " << (*it) << ", ";
				it++;
			}
			cout << ". " << endl;
			cout << "  edges in outerNodes: ";
			ListIterator<edge> itE = m_outerEdges[f].begin();
			while (itE.valid()){
				cout << " " << (*itE) << ", ";
				itE++;
			}
			cout << ". " << endl;
		}
	}

private:

	void setUpdate(node n);

	void setUpdate(face f);


	// member variables

	const Graph             	*m_pGraph;     	// the graph
	ConstCombinatorialEmbedding *m_pEmbedding; 	// the embedding of the graph

	face m_outerFace; 						// the outer/external face

	node m_v1, m_v2;						// the two nodes on the "bottom edge"

	bool m_preferNodes;						// m_preferNodes = true <=> nodes are prefered to faces
											// in selection of getNextPossible(..)

	NodeArray<int> m_visited, m_sepf;		// number of visited neighbours and separation faces
	NodeArray< ListIterator<node> > m_link;	// item in m_possibleNodes containing v

	List<node> m_possibleNodes;				// the possible nodes for the next step
	List<face> m_possibleFaces;				// the possible faces for the next step

	NodeArray< ListIterator<node> > m_nodesLink;	// list-iterator in m_possibleNodes for each node, or NULL if it doesn't exist
	FaceArray< ListIterator<face> > m_facesLink;	// list-iterator in m_possibleNodes for each node, or NULL

	List<node> m_updateNodes;				// list of actual changed nodes
	List<face> m_updateFaces;				// list of actual changed faces

	NodeArray<bool> m_nodeUpdate;			// m_nodeUpdate[v] = true <=> v \in m_updateNodes
	FaceArray<bool> m_faceUpdate; 			// m_faceUpdate[f] = true <=> f \in m_updateFaces

	FaceArray<bool> m_isSeparationFace;

	bool m_currentIsNode;					// m_currentIsNode == true <=> in the current step a node was picked
											//  => V_k is a singleton. The Variable == false <=> V_k is a node set

	FaceArray<int> m_outv;					// number of nodes...
	FaceArray<int> m_oute;					//... and edges of a face that also belong to the outer face

	FaceArray< List<node> > m_outerNodes;	// nodes of each face that also belong to the outer face
	FaceArray< List<edge> > m_outerEdges;	// edges of each face that also belong to the outer face

};	// class ComputeTricOrder


// constructor
ComputeTricOrder::ComputeTricOrder(
	const Graph& G, 					// the graph
	ConstCombinatorialEmbedding &E, 	// embedding of the graph
	face outerFace,                   	// the outer face
	double baseRatio,          			// size of base (baseRatio * size(extFace))
	bool preferNodes) 					// boolean value if nodes are prefered to faces
{
	m_pGraph = &G;
	m_pEmbedding = &E;
	m_outerFace = outerFace;
	m_preferNodes = preferNodes;

	// initialize member variables
	m_visited			.init(G, 0);
	m_sepf				.init(G, 0);
	m_link				.init(G, 0);
	m_nodeUpdate		.init(G, false);
	m_faceUpdate		.init(E, false);
	m_isSeparationFace	.init(E, false);
	m_nodesLink			.init(G, 0);
	m_facesLink			.init(E, 0);
	m_outv				.init(E, 0);
	m_oute				.init(E, 0);
	m_outerNodes		.init(E);
	m_outerEdges		.init(E);
}


// gets the next possible face or node
// ordering depends on m_preferNodes
void ComputeTricOrder::getNextPossible(node& v, face& f){
	if (m_preferNodes){
		if (m_possibleNodes.empty()){
			m_currentIsNode = false;
			f = m_possibleFaces.popFrontRet();
		}
		else{
			m_currentIsNode = true;
			v = m_possibleNodes.popFrontRet();
		}
	}
	else{
		if (m_possibleFaces.empty()){
			m_currentIsNode = true;
			v = m_possibleNodes.popFrontRet();
		}
		else{
			m_currentIsNode = false;
			f = m_possibleFaces.popFrontRet();
		}
	}
}


// initialize the nodes of the outer face
// and the corresponding faces
void ComputeTricOrder::initOuterNodes(node v1, node v2){
	// set nodes v1 and v2 on the "base"
	m_v1 = v1;
	m_v2 = v2;

	node v;

	adjEntry firstAdj = m_outerFace->firstAdj();
	adjEntry adjV;
	// set firstAdj, so outerface is on the right
	if (m_pEmbedding->rightFace(firstAdj) == m_outerFace)
		firstAdj = firstAdj->cyclicSucc();

	 adjEntry adjRun = firstAdj;
	 // traverse all nodes of the outer face
	 do {
	 	 	v = adjRun->theNode();
			// now traverse the faces f of v
			// and increase outv[f] and add v to outerNodes[f]
			forall_adj(adjV, v){
				face f = m_pEmbedding->rightFace(adjV);
				if (f != m_outerFace){
					m_outv[f]++;
					m_outerNodes[f].pushBack(v);
				}
			}
			adjRun = adjRun->twin()->cyclicSucc();
	} while (adjRun != firstAdj);
}


// initialize the edges of the external face
// and the corresponding faces
void ComputeTricOrder::initOuterEdges(){
	edge e;
	face f;

	adjEntry firstAdj = m_outerFace->firstAdj();
	// set firstAdj, so outerface is on the right
	if (m_pEmbedding->rightFace(firstAdj) == m_outerFace)
	 	firstAdj = firstAdj->cyclicSucc();
	adjEntry adjRun = firstAdj;
	// traverse all edges of the outer face
	do {
	 	 e = adjRun->theEdge();
	 	 f = m_pEmbedding->rightFace(adjRun);
	 	 // verify that actual edge is not edge (v1,v2)
	 	 if (!((e->source() == m_v1 && e->target() == m_v2) || ((e->source() == m_v2 && e->target() == m_v1)))){
	 	 	m_oute[f]++;
	 	 	m_outerEdges[f].pushBack(e);
	 	 }
		 adjRun = adjRun->twin()->cyclicSucc();
	} while (adjRun != firstAdj);
}


// compute the node n of face f that belongs to the outer face and has actually degree 2 or is node v2
// adjPred is the adjElement in clockwise order on the outerface
// adjSucc is the adjElement in counterclockwise order on the outerface
node ComputeTricOrder::getOuterNodeDeg2(face f, NodeArray<adjEntry>& adjPred, NodeArray<adjEntry>& adjSucc){
	// need the boolean value if v2 is found before another node with degree 2
	bool foundV2;
	node v;
	ListIterator<node> it = m_outerNodes[f].begin();
	for (it = m_outerNodes[f].begin(); it.valid(); it++){
		v = *it;
		if (v == m_v2){
			foundV2 = true;
			continue;
		}
		if (v == m_v1)
			continue;
		// check if node v has degree 2
		//   not so easy, because we do not delete nodes from the graph, so
		//   node v has degree 2 iff
		//    adjSucc[v]->cyclicSucc() == adjPred[v]
		//    adjPred[v]->cyclicPred() == adjSucc[v]
		if ((adjSucc[v])->cyclicSucc() == adjPred[v]){
			return v;
		}
	}
	if (foundV2)
		return m_v2;
	// no node found
	return 0;
}


// set update-value of node v true and append v to the update-nodes
void ComputeTricOrder::setUpdate(node v){
	if (!m_nodeUpdate[v]){
		m_nodeUpdate[v] = true;
		m_updateNodes.pushBack(v);
	}
}

// set update-value of face f true and append f to the update-faces
void ComputeTricOrder::setUpdate(face f){
	if (!m_faceUpdate[f]){
		m_faceUpdate[f] = true;
		m_updateFaces.pushBack(f);
	}
}


// update if
//	- v or f need to be inserted in possible list
//	- v or f need to be deleted from possible list
//	- f is becoming a separation face in the current step
void ComputeTricOrder::doUpdate(){
	bool isPossible, isSepFace;
	node v;
	face f;

	// first update faces, because variables for nodes can change here
	while (!m_updateFaces.empty()){
		f = m_updateFaces.popFrontRet();

		m_faceUpdate[f] = false;
		// check if face f is a possible face
		isPossible = ((m_outv[f] == m_oute[f] + 1) && (m_oute[f] >= 2) && (f != m_outerFace));

		// insert face f if it is not in possible-faces-list
		//  and is a possible face
		if (!m_facesLink[f].valid()){
			if (isPossible){
				m_facesLink[f] = m_possibleFaces.pushBack(f);
			}
		}
		else
			// delete f in possible-faces-list if it's not possible anymore
			if (!isPossible){
				m_possibleFaces.del(m_facesLink[f]);
				m_facesLink[f] = 0;
			}

		// test if face f is a separation face
		isSepFace = ((m_outv[f] >= 3) || ((m_outv[f] == 2) && (m_oute[f] == 0)));

		if (!m_isSeparationFace[f]){
			// f wasn't a separation face...
			if (isSepFace){
				// ... and is now one
				m_isSeparationFace[f] = true;
				// increase seperation-faces (->sepf) for all outer-nodes v in f
				ListIterator<node> it;
				for (it = m_outerNodes[f].begin(); it.valid(); it++)
					incSepf(*it);
			}
		}
		else{
			// face f was a separation face
			if (!isSepFace)
				// and isn't a separation face anymore
				// need only to set array-index to false
				// decrease of sepf is done before in main function
				m_isSeparationFace[f] = false;
		}
	}// while (!m_updateFaces.empty())

	// now update nodes
	while (!m_updateNodes.empty()){
		v = m_updateNodes.popFrontRet();

		m_nodeUpdate[v] = false;
		// check if v is a possible node
		isPossible = ((m_visited[v] >= 1) && (m_sepf[v] == 0) && (v != m_v1) && (v != m_v2));

		if (!m_nodesLink[v].valid()){
			// v is not in possible list, but v is a possible node
			if (isPossible){
				m_nodesLink[v] = m_possibleNodes.pushBack(v);
			}
		}
		else{
			// v is in the possible list but actually not possible
			if (!isPossible){
				m_possibleNodes.del(m_nodesLink[v]);
				m_nodesLink[v] = 0;
			}
		}
	}// while (!m_updateNodes.empty())
};



//---------------------------------------------------------
// TriconnectedShellingOrder
//---------------------------------------------------------

void TriconnectedShellingOrder::doCall(
		const Graph& G,
		adjEntry adj,
		List<ShellingOrderSet>& partition)
{

	// prefer nodes to faces?
	bool preferNodes = false;

	#ifdef OUTPUT_TSO
		cout << "Graph G is planar         == " << isPlanar(G)  << endl;
		cout << "Graph G has no self loops == " << isLoopFree(G) 		<< endl;
		cout << "Graph G is connected      == " << isConnected(G) 		<< endl;
		cout << "Graph G is triconnected   == " << isTriconnected(G) 	<< endl;
	#endif

	OGDF_ASSERT(isPlanar(G) == true);
	OGDF_ASSERT(isLoopFree(G) 		== true);
	OGDF_ASSERT(isTriconnected(G) 	== true);

	// crate an embedding for G
	ConstCombinatorialEmbedding E(G);

	// set outerFace so adj is on it or to face with maximal size
	face outerFace = (adj != 0) ? E.rightFace(adj) : E.maximalFace();

	#ifdef OUTPUT_TSO
		cout << "faces:" << endl;
		face fh;
		forall_faces(fh,E) {
			if (fh == outerFace)
				cout << "  face *" << fh->index() << ":";
			else
				cout << "  face  " << fh->index() << ":";
			adjEntry adj;
			forall_face_adj(adj,fh)
				cout << " " << adj;
			cout << endl;
		}

		cout << "adjacency lists:" << endl;
		node vh;
		forall_nodes(vh,G) {
			cout << "  node " << vh << ":";
			adjEntry adj;
			forall_adj(adj,vh)
				cout << " " << adj;
			cout << endl;
		}
	#endif

	adjEntry firstAdj = outerFace->firstAdj();
	// set firstAdj that the outer face is on the left of firstAdj
	if (E.rightFace(firstAdj) == outerFace)
		firstAdj = firstAdj->cyclicSucc();

	// set "base" nodes v1, v2 on outer face with edge [v1,v2]
	node v1 = firstAdj->theNode();
	node v2 = firstAdj->cyclicPred()->twinNode();

	ComputeTricOrder cto(G, E, outerFace, m_baseRatio, preferNodes);

	// if outerFace == {v_1,...,v_q}
	// 		adjPred(v_i) == v_i -> v_{i-1}
	// 		adjSucc(v_i) == v_1 -> v_{i+1}
	// these arrays will be updated during the algo so they define the outer face
	NodeArray<adjEntry> adjPred(G),
						adjSucc(G);

	// init adjPred and adjSucc for the nodes of the outer face
	adjSucc[v1] = firstAdj;
	adjEntry adjRun = firstAdj->twin()->cyclicSucc();
	do {
		adjPred[adjRun->theNode()] = adjRun->cyclicPred();
		adjSucc[adjRun->theNode()] = adjRun;
		adjRun = adjRun->twin()->cyclicSucc();
	} while (adjRun != firstAdj);
	adjPred[v1] = adjSucc[v2] = 0;

	// init outer nodes and outer edges
	cto.initOuterNodes(v1, v2);
	cto.initOuterEdges();

	// init the first possible node as the node in the middle of v_1
	//   and v_2 on the outer face
	int l = (outerFace->size() -2)/2;
	if (l == 0)
		l = 1;
	adjRun = firstAdj;
	for (int i=1; i <= l; i++)
		adjRun = adjRun->twin()->cyclicSucc();

	cto.initPossible(adjRun->theNode());

	// node and face that are selected during the algorithm
	node vk;
	face Fk;
	// left and right node of current nodeset
	node cl, cr;
	// the actual nodeset V in the shelling order
	ShellingOrderSet V;
	// further auxiliary variables
	adjEntry adj1, adj2;
	node u;

	#ifdef OUTPUT_TSO
		cout << "finished initialization of cto, adjSucc, adjPred." << endl << flush;
		cout << "v1 = " << v1 << ", v2 = " << v2 << ", first possible node = " << adjRun->theNode() << endl;

		forall_face_adj(adj1, outerFace){
			cout << " node " << adj1->theNode() << ": adjPred=(" << adjPred[adj1->theNode()]
				 << "), adjSucc=" << adjSucc[adj1->theNode()] << endl;
		}
		cto.output();
		cout << "starting main loop" << endl;
	#endif

	// main loop
	while (cto.isPossible()){

		// get the next possible nodeset for the order
		cto.getNextPossible(vk, Fk);

		// check if the current selection is a node or a face
		if (cto.isNode()){

			#ifdef OUTPUT_TSO
				cout << " nextPossible is node " << vk << endl << flush;
			#endif

			// current item is a node
			V = ShellingOrderSet(1, adjPred[vk], adjSucc[vk]);
			V[1] = vk;
			cl = (adjPred[vk])->twinNode();
			cr = (adjSucc[vk])->twinNode();
			// insert actual nodeset to the front of the shelling order
			partition.pushFront(V);
		}
		else{

			#ifdef OUTPUT_TSO
				cout << " nextPossible is face " << Fk->index() << endl << flush;
			#endif

			// current item is a face
			// create set with chain {z_1,...,z_l}
			V = ShellingOrderSet(cto.getOutv(Fk)-2);

			// now find node v on Fk with degree 2
			cl = cto.getOuterNodeDeg2(Fk, adjPred, adjSucc);
			// find end of chain cl and cr
			// traverse to left while degree == 2
			while ((cl != v1) && (adjPred[cl] == adjSucc[cl]->cyclicSucc()))
				cl = (adjPred[cl])->twinNode();

			// traverse to the right while degree == 2
			//  and insert nodes into the ShellingOrderSet
			cr = adjSucc[cl]->twinNode();
			int i = 1;
			while ((cr != v2) && (adjPred[cr] == adjSucc[cr]->cyclicSucc())){
				V[i] = cr;
				cr = (adjSucc[cr])->twinNode();
				i++;
			}
			cto.decSepf(cl);
			cto.decSepf(cr);
			// set left and right node in the shelling order set
			V.left(cl);
			V.right(cr);
			// set left and right adjacency entry
			V.leftAdj((adjPred[cr])->twin());
			V.rightAdj((adjSucc[cl])->twin());
			// insert actual nodeset to the front of the shelling order
			partition.pushFront(V);
		}// current item is a face

		#ifdef OUTPUT_TSO
			cout << "  set cl = " << cl << endl;
			cout << "  set cr = " << cr << endl;
		#endif

		// update adjSucc[cl] and adjPred[cr]
		adjSucc[cl] = adjSucc[cl]->cyclicSucc();
		adjPred[cr] = adjPred[cr]->cyclicPred();
		// increase number of outer edges of face left of adjPred[cr]
		cto.incOute(E.leftFace(adjPred[cr]));
		cto.incVisited(cl);
		cto.incVisited(cr);

		// traverse from cl to cr on the new outer face
		//  and update adjSucc[] and adjPred[]
		adj1 = adjSucc[cl]->twin();

		for (u = adj1->theNode(); u != cr; u = adj1->theNode()){
			// increase oute for the right face of adj1
			cto.incOute(E.leftFace(adj1));

			// set new predecessor
			adjPred[u] = adj1;

			// go to next adj-entry
			adj1 = adj1->cyclicSucc();

			// if the actual node has an edge to the deleted node
			//  increase the visited value for the actual node...
			if (adj1->twinNode() == vk){
				cto.incVisited(u);
				// ... and skip the actual adjEntry
				adj1 = adj1->cyclicSucc();
			}
			adjSucc[u] = adj1;

			// add actual node to outerNodes[f]
			for (adj2 = adjPred[u]; adj2 != adjSucc[u]; adj2 = adj2->cyclicPred()){
				cto.addOuterNode(u, E.leftFace(adj2));
			}
			adj1 = adj1->twin();
		}

		if (!cto.isNode()){
			if ( ((adjSucc[cl])->twinNode() == cr)
						&& ( cto.isOnlyEdge(E.rightFace(adjSucc[cl])) ) ){
				cto.decSepf(cl);
				cto.decSepf(cr);
			}
		}

		// update cto
		cto.doUpdate();

		#ifdef OUTPUT_TSO
			cto.output();
		#endif
	}// while (cto.isPossible())

	// finally push the base (v1,v2) to the order
	V = ShellingOrderSet(2);
	V[1] = v1;
	V[2] = v2;
	partition.pushFront(V);

	#ifdef OUTPUT_TSO
		cout << "output of the computed partition:" << endl;
		ListIterator<ShellingOrderSet> it;
		int k = 1;
		for (it = partition.begin(); it.valid(); it++){
			int size = (*it).len();
			cout << "nodeset with nr " << k << ":" << endl;
			for (int j=1; j<=size; j++)
				cout << " node " << (*it)[j] <<", ";
			cout << "." << endl;
		}
	#endif
}// void TriconnectedShellingOrder::doCall


} // end namespace ogdf

