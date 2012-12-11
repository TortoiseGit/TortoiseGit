/*
 * $Revision: 2566 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-07 23:10:08 +0200 (Sa, 07. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Implementation of classes BendString and OrthoRep.
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


#include <ogdf/orthogonal/OrthoRep.h>
#include <ogdf/planarity/PlanRepUML.h>
#include <ogdf/basic/String.h>
#include <string.h>


//TODO: Verknuepfe Uebergabe von PlanRep in dissect mit option align
//in undissect, denn nur so koennen Hierarchien korrekt erkannt werden


namespace ogdf {


//---------------------------------------------------------
// BendString
// represents the bends on an edge e consisting of vertical
// and horizontal segments
//---------------------------------------------------------

// initializes bends string to a given C-string
void BendString::init(const char *str)
{
#ifdef OGDF_DEBUG
	const char *q;
	for(q  = str; *q; ++q)
		OGDF_ASSERT(*q == '0' || *q == '1');
#endif

	m_len = strlen(str);
	if (m_len > 0)
	{
		m_pBend = new char[m_len+1];
		char *p = m_pBend;
		while ((*p++ = *str++) != 0) ;

	} else
		m_pBend = 0;
}


// initializes bends string to the string consoisting of n c's
void BendString::init(char c, size_t n)
{
	OGDF_ASSERT(c == '0' || c == '1');

	m_len = n;
	if (n > 0)
	{
		m_pBend = new char[n+1];
		m_pBend[n] = 0;
		do
			m_pBend[--n] = c;
		while(n > 0);
	}
	else
	{
		m_pBend = 0;
	}
}


// initializes bends string to a copy of bs
void BendString::init(const BendString &bs)
{
	m_len = bs.m_len;

	if (m_len == 0) {
		m_len = 0;
		m_pBend = 0;

	} else {
		m_pBend = new char[m_len+1];
		char *p = m_pBend;
		const char *str = bs.m_pBend;
		while ((*p++ = *str++) != 0) ;
	}
}



//---------------------------------------------------------
// OrthoRep
// orthogonal representation of an embedded graph
//---------------------------------------------------------

// constructor
OrthoRep::OrthoRep(CombinatorialEmbedding &E) : m_pE(&E), m_angle(E,0),
	m_bends(E)
{
	m_preprocess = true;
	m_pattern2 = true;
}


// initialization function; performs actual construction
void OrthoRep::init(CombinatorialEmbedding &E)
{
	m_pE = &E;
	m_angle.init(E,0);
	m_bends.init(E);
	m_preprocess = true;
	m_pattern2 = true;
}


// The check function below tests if the current OrthoRep instance really
// represents a correct orthogonal representation, i.e., it tests if
//    * the associated graph is embedded.
//    * the external face of the embedding is set
//    * the sum of the angles at each vertex is 4
//    * if corresponding bend strings are consistent, that is, if e has
//      adj. entries adjSrc and adjTgt, then the bend string of adjTgt
//      is the string obtained from bend string of adjSrc by reversing the
//      sequence and flipping the bits
//    * the shape of each face is rectagonal, i.e., if
//        #zeros(f) - #ones(f) - 2|f| + sum of angles at vertices in f
//      is 4 if f is an internal face or -4 if f is the external face.
bool OrthoRep::check(String &error)
{
	const Graph &G = (Graph&) *m_pE;

	// is the associated graph embedded ?
	if (G.representsCombEmbedding() == false) {
		error = "Graph is not embedded!";
		return false;
	}

	// sum of angles at each vertex equals 4 ?
	node v;
	forall_nodes(v,G)
	{
		int sumAngles = 0;
		adjEntry adj;
		forall_adj(adj,v)
			sumAngles += angle(adj);
		if(sumAngles != 4) {
			error.sprintf("Angle sum at vertex %d is %d.",
				v->index(), sumAngles);
			return false;
		}
	}

	// corresponding bend strings are consistent ?
	edge e;
	forall_edges(e,G)
	{
		const BendString &bs1 = bend(e->adjSource());
		const BendString &bs2 = bend(e->adjTarget());

		if (bs1.size() != bs2.size()) {
			error.sprintf(
				"Size of corresponding bend strings at edge %d differ!",
				e->index());
			return false;
		}

		size_t i = 0, j = bs2.size()-1;
		while(i < bs1.size()) {
			if (bs1[i] != flip(bs2[j])) {
				error.sprintf(
					"Corresponding bend strings at edge %d not consistent!",
					e->index());
				return false;
			}
			++i; --j;
		}
	}


	// external face set ?
	if (m_pE->externalFace() == 0) {
		error = "External face is not set!";
		return false;
	}

	// is shape of each face rectagonal ?
	face f;
	forall_faces(f,*m_pE)
	{
		int rho = 0;

		adjEntry adj;
		forall_face_adj(adj,f) {
			const BendString &bs = bend(adj);
			int zeroes = 0, ones = 0;
			for(size_t i = 0; i < bs.size(); ++i) {
				switch (bs[i])
				{
				case '0':
					zeroes++; break;
				case '1':
					ones++; break;
				default:
					error.sprintf("bend string of adjacency entry %d contains \
illegal character!", adj->index());
					return false;
				}
			}

			rho += zeroes - ones + 2 - angle(adj);
		}

		if (rho != ((f == m_pE->externalFace()) ? -4 : 4)) {
			error.sprintf("Shape of face %d not rectagonal!", f->index());
			return false;
		}
	}


	return true;
}


// normalizes an orthogonal representation, i.e., replaces each bend
// by a dummy vertex and updates the embedding as well as the orthogonal
// representation
void OrthoRep::normalize()
{
	const Graph &G = (Graph &) *m_pE;

	edge e;
	forall_edges(e,G)
	{
		// store current bend string in bs
		BendString bs(m_bends[e->adjSource()]);
		const char *str = bs.toString();
		if (str == 0) continue;

		m_bends[e->adjSource()].set();
		m_bends[e->adjTarget()].set();

		// for each bend in bs, introduce a new vertex by splitting
		for(; *str; ++str)
		{
			edge ePrime = m_pE->split(e);
			m_angle[ePrime->adjTarget()] = m_angle[e->adjTarget()];

			if(*str == '0') {
				m_angle[ePrime->adjSource()] = 1;
				m_angle[e     ->adjTarget()] = 3;

			} else {
				m_angle[ePrime->adjSource()] = 3;
				m_angle[e     ->adjTarget()] = 1;
			}
		}

	}
}


// checks if each bends string is empty
bool OrthoRep::isNormalized() const
{
	const Graph &G = (Graph &) *m_pE;

	edge e;
	forall_edges(e,G)
	{
		if (m_bends[e->adjSource()].size() != 0)
			return false;
		if (m_bends[e->adjTarget()].size() != 0)
			return false;
	}

	return true;
}



// Procedure dissect() modifies the orthogonal representation by splitting
// edges and faces until no more rectangular ears are contained. A rectangular
// ear consists of two 90 degree angles with only 180 degree angles inbetween.
//
// More exactly, each internal face has rectangular shape afterwards. For the
// external face, we guarantee only the absence of rectangular ears.
//
// Precondition: The orthogonal representation is normalized and contains
//               no 0 degree angles
void OrthoRep::dissect()
{
	// dissect() requires a normalized orthogonal representation
	OGDF_ASSERT(isNormalized());

	CombinatorialEmbedding &E = *m_pE;
	Graph &G = E;

	// assert that dissect hasn't been called before
	OGDF_ASSERT(m_splitNodes.empty());
	m_dissectionEdge.init(G,false);

	adjEntry saveExt = E.externalFace()->firstAdj(); //should check supersink
	m_adjExternal = saveExt;

	face f;
	forall_faces(f, E)
	{
		// dissect face f

		// We build the list faceCycle consisting of all adjacency entries
		// that do not form a 180 degree angle with their successors
		// (180 degree angles do not contribute to the shape of the face)
		List<adjEntry> faceCycle;

		adjEntry adj;
		forall_face_adj(adj,f) {
			// dissection does not work for graphs with 0 degree angles!
			OGDF_ASSERT(m_angle[adj] != 0);
			if (m_angle[adj] != 2)
				faceCycle.pushBack(adj);
		}

		// We iterate over faceCycle and look for occurrences of two
		// consecutive 90 degree angles
		ListIterator<adjEntry> it;;
		for(it = faceCycle.begin(); faceCycle.size() > 4 && it.valid(); ++it)
		{
			if (m_angle[*it] == 1 && m_angle[*faceCycle.cyclicPred(it)] == 1)
			{
				// now we run backwards and look for angles >= 270 degree. We
				// can eliminate such angles as long as the following two
				// angles (faceCycle is seen as cyclic list) are 90 degree.
				ListIterator<adjEntry> itBack =
					faceCycle.cyclicPred(faceCycle.cyclicPred(it));

				// Look for the next angle >= 270 degree.
				// We will find one since faceCylce has at least two elements
				// Note: For each 90 degree angle we have to skip we can
				// eliminate one more angle >= 270 degree.

				// if we did not find a >= 270 degree angle until it, the
				// face must be a rectangle and faceCycle consists of four 90
				// degree angles. Hence, we terminate also the for-loop above!
				while(it != itBack)
				{
					// If we see a 90 degree angle, we move further backwards
					if(m_angle[*itBack] < 3) {
						itBack = faceCycle.cyclicPred(itBack);
						continue;
					}

					ListIterator<adjEntry> itBackSucc =
						faceCycle.cyclicSucc(itBack);

					// If we see a >= 270 degree angle whose successor is it,
					// we do not have a rectangular ear anymore, so we break.
					// We then have processed all >= 270 degree angles behind
					// it
					if(itBackSucc == it)
						break;

					adjEntry &adjSplit = *faceCycle.cyclicSucc(itBackSucc);
					// We now have the following situation:
					// The rectangular ear (pattern 100) consists of
					// *itBack -> *itBackSucc -> adjSplit

					// Since a split operation can also change the id's of the
					// adjacency entries at the edge, we have to backup two
					// angles a1 and a2.
					int a1 = m_angle[adjSplit];
					adjEntry adj2 = adjSplit->twin();
					int a2 = m_angle[adj2];

					// We split *itBackSuccSucc ...
					node u = E.split(adjSplit->theEdge())->source();
					if (m_dissectionEdge[adjSplit] == false)
						m_splitNodes.push(u);
					adjEntry adjSplitSucc = adjSplit->faceCycleSucc();
					// and close a rectangular face
					edge eDissect = E.splitFace(*itBack, adjSplitSucc);
					m_dissectionEdge[eDissect] = true;

					// restore backup angles
					m_angle[adjSplit] = a1;
					m_angle[adj2] = a2;

					// set angles at the split node
					m_angle[adjSplitSucc] = 1;
					m_angle[adjSplitSucc->cyclicSucc()] = 1;
					m_angle[adjSplitSucc->cyclicPred()] = 2;

					adjEntry adjSucc = (*itBack)->cyclicSucc();
					if (m_angle[*itBack] == 4) {
						// If the former >= 270 degree angle was 360 degree,
						// we keep it as 270 degree angle ...
						m_angle[*itBack] = 1;
						m_angle[adjSucc] = 3;
						*itBack = adjSucc;

					} else {
						// ... otherwise, it is now a 180 degree angle and
						// we remove it
						m_angle[*itBack] = 1;
						m_angle[adjSucc] = 2;

						ListIterator<adjEntry> itDel = itBack;
						itBack = faceCycle.cyclicPred(itBack);
						faceCycle.del(itDel);
					}

					// The other edge (not adjSplit) which resulted from the
					// split edge operation is contained in the face we
					// consider
					adjSplit = adjSplitSucc;

					// This 90 degree angle vanishes from our face
					faceCycle.del(itBackSucc);
				}
			}
		}
	}
}


//artificial node saving test******************************************************
//gunnars vorschlag: segment saving
void OrthoRep::dissect2(PlanRepUML* PG)
{
	String msg;
	m_adjAlign = 0;
	// dissect() requires a normalized orthogonal representation
	OGDF_ASSERT(isNormalized());

	CombinatorialEmbedding &E = *m_pE;
	Graph &G = E;

	// assert that dissect hasn't been called before
	OGDF_ASSERT(m_splitNodes.empty());
	m_dissectionEdge.init(G,false);
	m_alignmentEdge.init(G, false);

	m_adjExternal = E.externalFace()->firstAdj();

	face f;
	forall_faces(f, E)
	{
		// dissect face f

		// We build the list faceCycle consisting of all adjacency entries
		// that do not form a 180 degree angle with their successors
		// (180 degree angles do not contribute to the shape of the face)
		List<adjEntry> faceCycle;

		adjEntry adj;
		forall_face_adj(adj,f) {
			// dissection does not work for graphs with 0 degree angles!
			OGDF_ASSERT(m_angle[adj] != 0);
			if (m_angle[adj] != 2)
				faceCycle.pushBack(adj);
		}
		//PREPROCESSING
		// some of the preprocessing steps could be mixed or used iteratively
		//is it better to mix? or to iterate? pattern1 and pattern2 don't mess with
		//each other
		bool change = true;

		while (change)
		{
			change = false;
			//preprocessing: we look for 311113 angle patterns and replace them by inserting an edge
			//between the two angle 3 adjacencies, thought for generalization merger / son nodes
			//if parameter PG is set, we use the type info to set some dissection edges as
			//alignment edges
			//in the case of m_align we have to take care of the situation that the adjExternal
			//lies in the cut ear, which is an error in the improvement compaction steps
			if (m_preprocess)
			{
				ListIterator<adjEntry> prit;

				//only run until no more patterns left
				//and dont iterate over the face, only check this one time
				//int prerun = 0;
				//prit = faceCycle.begin();
				//while ( prit.valid() && (prerun < faceCycle.size() + 1))
				//check if possible
				ListIterator<adjEntry> itEnd, itStart, it1one, it1two, it1three, it1four; //pattern defining edges
				//take care of prit, it will be deleted
				for (prit = faceCycle.begin(); prit.valid() && (faceCycle.size()>7); prit++) //go clockwise around face!?
				{
					itEnd = prit; //search pattern backwards
					if (m_angle[*itEnd]    != 3) continue;
					it1four = faceCycle.cyclicPred(itEnd);
					if (m_angle[*it1four]  != 1) continue;
					it1three = faceCycle.cyclicPred(it1four);
					if (m_angle[*it1three] != 1) continue;
					it1two = faceCycle.cyclicPred(it1three);
					if (m_angle[*it1two]   != 1) continue;
					it1one = faceCycle.cyclicPred(it1two);
					if (m_angle[*it1one]   != 1) continue;
					itStart = faceCycle.cyclicPred(it1one);
					if (m_angle[*itStart]  != 3) continue;

					//PATTERN FOUND
					// we proudly present the pattern we searched for
					//now we insert the new dissection edge, delete the entries in faceCycle,
					//and set the angles accordingly

					//take care of the special alignment situation where alignment edges
					//are kept during compaction
					if ( ( (m_adjExternal == (*itEnd)) ||
						(m_adjExternal == (*it1four)) ||
						(m_adjExternal == (*it1three)) ||
						(m_adjExternal == (*it1two)) ||
						(m_adjExternal == (*it1one)) ||
						(m_adjExternal == (*itStart))
						)
						||
						( (m_adjAlign == (*itEnd)) ||
						(m_adjAlign == (*it1four)) ||
						(m_adjAlign == (*it1three)) ||
						(m_adjAlign == (*it1two)) ||
						(m_adjAlign == (*it1one)) ||
						(m_adjAlign == (*itStart))
						)
						)
						//check: is itEnd appropriate?
						m_adjAlign = (*itEnd);//->faceCycleSucc();

					adjEntry& adEnd = *itEnd;
					adjEntry& adStart = *itStart;
					//split the face between two corners
					edge eDissect = E.splitFace(adStart, adEnd);

					//alignment part
					if (PG != 0)
					{
						if ( (PG->typeOf((*it1two)->theEdge()->source()) == Graph::generalizationExpander) &&
							(PG->typeOf((*it1two)->theEdge()->target()) == Graph::generalizationExpander))
							m_alignmentEdge[eDissect] = true;
					}//if  Pg
					m_dissectionEdge[eDissect] = true;

					change = true;
					//if (PG) PG->typeOf(e) = Graph::dissect;

					//set the angles, old values 3 are divided
					m_angle[adEnd] = 1;
					m_angle[adStart] = 2;
					m_angle[adStart->cyclicSucc()] = 1; //new edge entry from start to end
					m_angle[adEnd->cyclicSucc()] = 2; //new edge entry from end to start
					//we dont have any angles > 2 left, so delete some participants
					//Start and all itone are out of the face in the new rectangle
					faceCycle.del(it1four);
					faceCycle.del(it1three);
					faceCycle.del(it1two);
					faceCycle.del(it1one);
					//do not delete itStart, copy the new edges entry to this position
					//faceCycle.del(itStart);
					adStart = adStart->cyclicSucc();//use reference
					//itEnd stays with angle value 1

					OGDF_ASSERT_IF(dlConsistencyChecks,check(msg));
				}//for
			}//preprocessing pattern1
			//if (m_people)
			//we search for a 3111 pattern
			if (m_pattern2)
			{
				ListIterator<adjEntry> prit, savenext;

				//only run until no more patterns left
				//and dont iterate over the face, only check this one time
				//check if possible
				ListIterator<adjEntry> it1Top, itTopSucc, it1Back, it1Base, it3Start; //pattern defining edges
				//take care of prit, it will be deleted
				savenext = faceCycle.begin();
				for (prit = faceCycle.begin(); (prit.valid() && savenext.valid()) && (faceCycle.size()>6); (prit = savenext)) //go clockwise around face!?
				{
					savenext++;
					itTopSucc = prit; //search pattern backwards
					it1Top = faceCycle.cyclicPred(itTopSucc);
					if (m_angle[*it1Top]  != 1) continue;
					it1Back = faceCycle.cyclicPred(it1Top);
					if (m_angle[*it1Back] != 1) continue;
					it1Base = faceCycle.cyclicPred(it1Back);
					if (m_angle[*it1Base]   != 1) continue;
					it3Start = faceCycle.cyclicPred(it1Base);
					if (m_angle[*it3Start]  != 3) continue;

					//first version
					if (m_angle[*itTopSucc] < 2) continue;

					// we proudly present the pattern we searched for
					//now we insert the new dissection edge, delete the entries in faceCycle,
					//and set the angles accordingly

					adjEntry& adEnd = *itTopSucc;
					adjEntry& adStart = *it3Start;
					//split the face between two corners
					edge eDissect = E.splitFace(adStart, adEnd);

					m_dissectionEdge[eDissect] = true;

					change = true;
					//if (PG) PG->typeOf(e) = Graph::dissect;

					//set the angles, old values 3 are divided
					m_angle[adEnd] = m_angle[adEnd] - 1;
					m_angle[adStart] = 2;
					m_angle[adStart->cyclicSucc()] = 1; //new edge entry from start to end
					m_angle[adEnd->cyclicSucc()] = 1; //new edge entry from end to start
					//we dont have any angles > 2 left, so delete some participants
					//Start and all itone are out of the face in the new rectangle
					faceCycle.del(it1Top);
					faceCycle.del(it1Back);
					faceCycle.del(it1Base);
					//do not delete itStart, copy the new edges entry to this position
					//faceCycle.del(itStart);
					adStart = adStart->cyclicSucc();//use reference
					//check if itEnd stays
					if (m_angle[adEnd] == 2)
					{
						faceCycle.del(itTopSucc);
					}//if

					//OGDF_ASSERT(check(msg));
				}//for
			}//pattern2


		}//iterate pattern search
		//search for ears in connection between two cages and fill them in a preprocessing
		//step to avoid the separation, works only in combination with segment-saving
		//and if PlanRep-ifnormation is known, maybe use degree1-simplification
		if (PG != 0)
		{
			ListIterator<adjEntry> prit, savenext;
			//check if possible
			ListIterator<adjEntry> itEnd, itEar, itHead, itToe; //pattern defining edges
			//take care of prit, it will be deleted
			savenext = faceCycle.begin();

			//if (PG->original((*savenext)->theNode()) != 0)
			//cout << "Knotengraeder: " << PG->original((*savenext)->theNode())->degree() <<  "\n" << flush;

			for (prit = faceCycle.begin(); (prit.valid() && savenext.valid()) && (faceCycle.size()>5); (prit = savenext)) //go clockwise around face!?
			{
				savenext++;
				//es bleibt herauszufinden, in welcher reihenfolge die kanten ohnehin korrekt
				//durchlaufen werden, dann die andere nehmen
				itEnd = prit; //search pattern backwards
				//if (m_angle[*itEnd]    != 3) continue; //nur in einer Richtung
				itHead = faceCycle.cyclicPred(itEnd);
				if (m_angle[*itHead]  != 1) continue;
				itEar = faceCycle.cyclicPred(itHead);
				if (m_angle[*itEar] != 1) continue;
				itToe = faceCycle.cyclicPred(itEar);
				if (m_angle[*itToe]   != 3) continue; //andersherum anders


				node ov = PG->expandedNode( (*itEar)->theNode() );
				if (ov == 0) continue;
				ov = PG->original( ov );
				if (ov == 0) continue;
				//cout<<"Grad ist "<<PG->original( PG->expandedNode( (*itEar)->theNode() ) )->degree()<<"\n"<<flush;
				//zweiter Knoten reicht auch, andere Seite
				node ov2;
				ov2 = PG->expandedNode( (*itHead)->theNode() );
				if (ov2 == 0) continue;
				ov2 = PG->original( ov2 );
				if (ov2 == 0) continue;
				//cout<<"Grad ist "<<PG->original( PG->expandedNode( (*itEar)->theNode() ) )->degree()<<"\n"<<flush;
				if ( (ov2->degree() != 1) && (ov->degree() != 1) ) continue;
				///if ( PG->typeOf((*itEar)->theEdge()) == -1) continue;
				//I dropped this classification due to dirty programmed edge type decision
				//if not ass it is gen....
				// we proudly present the pattern we searched for
				//now we insert the new dissection edge, delete the entries in faceCycle,
				//and set the angles accordingly
				//cout<<"FOUND PATTERN \n\n";
				//adjEntry& adEnd = *itEnd;
				adjEntry& adHead = *itHead;
				adjEntry& adToe = *itToe;
				adjEntry adjHeadSucc;
				adjEntry adj2;

				adjHeadSucc = adHead->faceCycleSucc();

				int a1 = m_angle[adjHeadSucc];
				int a2;
				//bool splitted = false;
				edge eDissect;
				node u;

				//cout<<"PRE2splitting: head "<<adHead<<"/"<<adHead->twin()<<"\n";
				//cout<<"Winkel: "<<m_angle[adHead]<<"/"<<m_angle[adHead->twin()]<<"\n";
				//Hier noch einfuegen: natuerlich muss head.fcsucc statt faceCycle.succ getestet werden, da auch 180 Grad zulaessig

				//if (m_angle[*itEnd] != 1)
				if (m_angle[adjHeadSucc] != 1)
				{
					//split the face between two corners
					eDissect = E.splitFace(adToe, adjHeadSucc);
					//if (PG) PG->typeOf(e) = Graph::dissect;
					//hier koennte man highprio type gen setzen
					m_angle[adjHeadSucc] = a1 - 1;
					m_angle[adjHeadSucc->cyclicSucc()] = 1; //new edge entry from end to start
					if (m_angle[adjHeadSucc] == 1) adHead = adjHeadSucc;
					else faceCycle.del(itHead);
					if (m_angle[adjHeadSucc] == 2) faceCycle.del(itEnd); //it must have been in list
				}
				else
				{

					//split edge
					a1 = m_angle[adHead];
					adj2 = adHead->twin();
					a2 = m_angle[adj2];

					edge savee = adHead->theEdge();
					bool wasDissected =  m_dissectionEdge[savee];
					bool wasAlign = m_alignmentEdge[savee];
					Graph::EdgeType savetype;
					if (PG) savetype = PG->typeOf(adHead->theEdge());
					edge se = E.split(adHead->theEdge());
					if (PG) PG->typeOf(se) = savetype;
					adjHeadSucc = adHead->faceCycleSucc();
					//cout<<"headsucc: "<<adjHeadSucc<<"\n";

					u = se->source();
					//cout<<"Neuer Knoten: "<<u<<"\n";
					if (m_dissectionEdge[adHead] == false)
						m_splitNodes.push(u);
					if (wasDissected) m_dissectionEdge[se] = true;
					if (wasAlign) m_alignmentEdge[se] = true;

					//splitted = true;


					eDissect = E.splitFace(adToe, adjHeadSucc);
					//cout<<"restore: head"<<adHead<<" "<<a1<<"\n";
					//cout<<"restore: adj2"<<adj2<<" "<<a2<<"\n";
					//cout<<"restore: headsucc"<<adjHeadSucc<<" "<<1<<"\n";
					//cout<<"restore: headsucccyclicpred"<<adjHeadSucc->cyclicPred()<<" "<<2<<"\n";
					//cout<<"restore: headsucccyclicSucc"<<adjHeadSucc->cyclicSucc()<<" "<<1<<"\n";
					// restore backup angles
					m_angle[adHead] = a1; //alter Winkel unten
					m_angle[adj2] = a2;   //alter winkel oben
					m_angle[adjHeadSucc] = 1; //Winkel oben zu neuer Kante
					m_angle[adjHeadSucc->cyclicPred()] = 2;
					m_angle[adjHeadSucc->cyclicSucc()] = 1;
					adHead = adjHeadSucc;

				}//else
				//cout<<"inserted edge "<<eDissect<<"\n"<<flush;
				m_dissectionEdge[eDissect] = true;
				//set the angles, old values 3 are divided
				m_angle[adToe->cyclicSucc()] = 2; //new edge entry from start to end
				m_angle[adToe] = 1;

				//we dont have any angles > 2 left, so delete some participants
				faceCycle.del(itEar);
				faceCycle.del(itToe);
				//do not delete itStart, copy the new edges entry to this position
				//if (splitted)

				//OGDF_ASSERT(check(msg));

			}//for
		}//preprocessing 2


		// We iterate over faceCycle and look for occurrences of two
		// consecutive 90 degree angles
		ListIterator<adjEntry> it;
		int runcount = 0; //check progress
		it = faceCycle.begin();
		while ((faceCycle.size() > 4) && it.valid() && (runcount <= 2*faceCycle.size()))
		{

			if (m_angle[*it] == 1 && m_angle[*faceCycle.cyclicPred(it)] == 1)
			{
				runcount = 0; //start it over again
				// now we run backwards and look for angles >= 270 degree. We
				// can eliminate such angles as long as the following two
				// angles (faceCycle is seen as cyclic list) are 90 degree.
				ListIterator<adjEntry> itBack =
					faceCycle.cyclicPred(faceCycle.cyclicPred(it));

				// Look for the next angle >= 270 degree.
				// We will find one since faceCylce has at least two elements
				// Note: For each 90 degree angle we have to skip we can
				// eliminate one more angle >= 270 degree.

				// if we did not find a >= 270 degree angle until it, the
				// face must be a rectangle and faceCycle consists of four 90
				// degree angles. Hence, we terminate also the while-loop above!
				while((it != itBack) && (faceCycle.size() > 4))
				{
					if ((m_angle[*it] != 1) || (m_angle[*faceCycle.cyclicPred(it)] != 1)) break;

					// If we see a 90 degree angle, we move further backwards
					if( m_angle[*itBack] < 3) {
						itBack = faceCycle.cyclicPred(itBack);
						continue;
					}

					ListIterator<adjEntry> itBackSucc =
						faceCycle.cyclicSucc(itBack);

					// If we see a >= 270 degree angle whose successor is it,
					// we do not have a rectangular ear anymore, so we break.
					// We then have processed all >= 270 degree angles behind
					// it
					//if (itBackSucc == it)
					//	break;

					adjEntry &adjSplit = *faceCycle.cyclicSucc(itBackSucc);
					ListIterator<adjEntry> itsplit = faceCycle.cyclicSucc(itBackSucc);
					// We now have the following situation:
					// The rectangular ear (pattern 100) consists of
					// *itBack -> *itBackSucc -> adjSplit


					// Since a split operation can also change the id's of the
					// adjacency entries at the edge, we have to backup two
					// angles a1 and a2.
					int a1 = m_angle[adjSplit];
					adjEntry adj2 = adjSplit->twin();
					int a2 = m_angle[adj2];

					//hier nur splitten, falls noetig, aber testen, ob es in der Reihenfolge
					//Probleme macht, wenn man auch fuer 180 Grad, die nicht im cycle sind, nicht splittet
					//save the angle
					int earSlope = m_angle[adjSplit->faceCycleSucc()];
					//the target node of the split operation
					node u;
					bool savevertex = (earSlope >= 2); //should be >= 2, but 2 not in cycle
					ListIterator<adjEntry> itsucc;
					//check if we cant save a bend
					if (!savevertex)
					{
						// We split *itBackSuccSucc ...

						edge savee = adjSplit->theEdge();
						bool wasDissected =  m_dissectionEdge[savee];
						bool wasAlign = m_alignmentEdge[savee];

						edge se = E.split(adjSplit->theEdge());

						u = se->source();
						//die neue Kante erhaelt adj, ist aber adjSplit, also immer false?Nein, aber nicht korrekt
						//order problem: first delete disection edges, then unsplit, but what if split(dissection)
						if (m_dissectionEdge[adjSplit] == false)
							m_splitNodes.push(u);
						if (wasDissected) m_dissectionEdge[se] = true;
						if (wasAlign) m_alignmentEdge[se] = true;
					}
					else
					{
						u = (adjSplit->faceCycleSucc())->theNode(); //use this node instead
						itsucc = faceCycle.cyclicSucc(faceCycle.cyclicSucc(itBackSucc));
						//cout<<"we will save a vertex\n"<<flush;
					}
					adjEntry adjSplitSucc = adjSplit->faceCycleSucc();
					//if (savevertex) OGDF_ASSERT(adjSplitSucc == *itsucc);//doesnt work with 180 degree
					// and close a rectangular face
					edge eDissect = E.splitFace(*itBack, adjSplitSucc);
					m_dissectionEdge[eDissect] = true;

					//cout<<"inserted new edge "<<eDissect<<"\n"<<flush;
					//cout<<"setting angles split/adj2 : "<<adjSplit<<":"<<a1<<" / "<<adj2<<":"<<a2<<"\n"<<flush;
					// restore backup angles
					m_angle[adjSplit] = a1;
					m_angle[adj2] = a2;
					//but this should be splitsucccyclicpred???? why reset value if not savevertex

					// set angles at the split node
					bool shiftedit = false;
					if (savevertex)
					{
						m_angle[adjSplitSucc] = earSlope - 1;
						//cout<<"setting splitsucc angle to    : "<<adjSplitSucc<<": "<<earSlope - 1<<"\n"<<flush;
						//if the former split_to angle was a 270 degree angle, then it
						//is now 180 and we delete it
						if (m_angle[adjSplitSucc] == 2)
						{
							if (itsucc == it)
							{
								//cout<<"shifted it from to          "<<*it<<*faceCycle.cyclicSucc(it)<<"\n"<<flush;
								shiftedit = true;
								it = faceCycle.cyclicSucc(it);
							}
							//cout<<"deleted itsucc              "<<*itsucc<<"\n"<<flush;
							faceCycle.del(itsucc);
						}

					}
					else {
						m_angle[adjSplitSucc] = 1;
						//cout << "setting adjsplitsucc angle to : " <<adjSplitSucc<<" : "<<m_angle[adjSplitSucc]<<"\n"<<flush;
					}
					//cout << "setting asscyclicsucc angle to: " <<adjSplitSucc->cyclicSucc()<<" : "<<1<<"\n"<<flush;
					m_angle[adjSplitSucc->cyclicSucc()] = 1;
					//m_angle[adjSplitSucc->cyclicPred()] = 2;
					//Achtung, geht nur, wenn nicht vorher 180, sonst abhaengig von u.U. v. dritter Kante abziehen
					//spaeter zusammenlegen
					if (!savevertex)
						if (earSlope != 4) //already has new value
							m_angle[adjSplitSucc->cyclicPred()] = 4 - 1 - m_angle[adjSplitSucc];
					//if (savevertex && (earSlope == 2)) m_angle[adjSplitSucc->cyclicPred()] = ???

					adjEntry adjSucc = (*itBack)->cyclicSucc();
					if (m_angle[*itBack] == 4) {
						// If the former >= 270 degree angle was 360 degree,
						// we keep it as 270 degree angle ...
						m_angle[*itBack] = 1;
						m_angle[adjSucc] = 3;
						*itBack = adjSucc;

					} else {
						// ... otherwise, it is now a 180 degree angle and
						// we remove it
						m_angle[*itBack] = 1;
						m_angle[adjSucc] = 2;

						ListIterator<adjEntry> itDel = itBack;
						itBack = faceCycle.cyclicPred(itBack);
						if (it == itDel)
						{
							it = faceCycle.cyclicSucc(it); shiftedit = true;
							//cout<<"shifted it to "<<*it<<"\n"<<flush;
						}
						faceCycle.del(itDel);
					}

					// The other edge (not adjSplit) which resulted from the
					// split edge operation is contained in the face we
					// consider instead of adjsplit if its not 180 degree
					if ((!savevertex) || (earSlope == 2))
						adjSplit = adjSplitSucc;
					else
					{
						if (itsplit == it)
						{
							//cout<<"deleting itsplit, shifting it "<<*itsplit<<", "<<*it<<" - "<<*faceCycle.cyclicSucc(it)<<"\n"<<flush;
							it = faceCycle.cyclicSucc(it);
							shiftedit = true;
						}
						//else cout<<"deleting itsplit             "<<*itsplit<<"\n"<<flush;
						faceCycle.del(itsplit);
					}

					// This 90 degree angle vanishes from our face
					faceCycle.del(itBackSucc);
					if (shiftedit) break;
				}//while run backwards
			}//if double 1
			it = faceCycle.cyclicSucc(it);
			runcount++;
		}//while
		//OGDF_ASSERT(check(msg));
	}
}

//test end ************************************************************************
// undoes a previous dissect()
// important: recomputes list of faces, so previous faces are no
//            longer valid
void OrthoRep::undissect(bool align) //default false
{
	// assert that dissect() has been called before
	OGDF_ASSERT(m_dissectionEdge.valid() == true);

	Graph &G = *m_pE;

	// remove all dissection edges
	edge e, eSucc;
	for(e = G.firstEdge(); e != 0; e = eSucc)
	{
		eSucc = e->succ();
		if (m_dissectionEdge[e] == true) {

			if (!(align && m_alignmentEdge[e]))
			{
				// angles at source and target node are joined ...
				adjEntry adjSrc = e->adjSource();
				m_angle[adjSrc->cyclicPred()] += m_angle[adjSrc];

				adjEntry adjTgt = e->adjTarget();
				m_angle[adjTgt->cyclicPred()] += m_angle[adjTgt];

				// ... when dissection edge is removed
				node sv = adjSrc->theNode();
				node tv = adjTgt->theNode();

				G.delEdge(e);
				//remember that sv and tv are not allowed to be in splitNodes, see dissect
				if (sv->degree() == 0) G.delNode(sv);
				if (tv->degree() == 0) G.delNode(tv);
			}//if
		}//if
	}//for
	// free allocated memory
	if (!align) m_dissectionEdge.init();

	//alignment edges never split
	// unsplit remaining split nodes
	while(!m_splitNodes.empty())
	{
		G.unsplit(m_splitNodes.pop());
	}

	// recompute list of faces and restore external face
	m_pE->computeFaces();

	//may be the external face is still in the alignment part
	if (align && (m_adjAlign != 0))
	{
		m_pE->setExternalFace(m_pE->rightFace(m_adjAlign));
		//m_adjAlign = 0;
	}
	else m_pE->setExternalFace(m_pE->rightFace(m_adjExternal));

	//ofstream out("c:\\outerface.txt");
	//out<<"Aeusseres Face: \n";
	//adjEntry eo = m_pE->externalFace()->firstAdj();
	//adjEntry stop = eo;
	//do
	//{
	//  out<<eo<<"\n";
	//  eo = eo->faceCycleSucc();
	//} while (eo!=stop);
}//undissect



// assigns consistent directions (vertical or horizontal) to adj. entries
void OrthoRep::orientate()
{
	orientate(m_pE->getGraph().firstEdge()->adjSource(), odWest);
}


// assigns consistent directions to adj. entries such that most
// generalizations are directed in preferedDir
void OrthoRep::orientate(const PlanRep &PG, OrthoDir preferedDir)
{
	// assign an arbitrary orientation
	orientate();

	// count how many adjacency entries are orientated in a direction
	Array<int> num(0,3,0);
	edge e;
	forall_edges(e,PG) {
		if (PG.typeOf(e) == Graph::generalization)
			++num[m_dir[e->adjSource()]];
	}

	// find direction with maximum number
	int maxDir = 0;
	for(int i = 1; i < 4; ++i)
		if (num[i] > num[maxDir])
			maxDir = i;

	// rotate directions by (preferedDir - maxDir)
	rotate(preferedDir - maxDir);
	//orientate(PG.firstEdge()->adjSource(),m_dir[PG.firstEdge()->adjSource()]);
}


// assigns consistent directions (vertical or horizontal) to adj. entries,
// assigning dir to adj (this fixes all others!)
void OrthoRep::orientate(adjEntry adj, OrthoDir dir)
{
	OGDF_ASSERT(isNormalized());
	OGDF_ASSERT(adj != 0);
	OGDF_ASSERT(
		dir == odEast || dir == odWest || dir == odNorth || dir == odSouth
	);

	const Graph &G = (Graph &) *m_pE;

	m_dir.init(G, odUndefined);

	orientateFace(adj, dir);
}


void OrthoRep::orientateFace(adjEntry adj, OrthoDir dir)
{
	// We run only till the next already processed adj. entry, potentially
	// not around the whole face. This is important for linear runtime
	while (m_dir[adj] == odUndefined)
	{
		m_dir[adj] = dir;

		adj = adj->twin();

		dir = oppDir(dir);
		if (m_dir[adj] == odUndefined)
			orientateFace(adj, dir);

		// orientation changes at 90 and 270 degree angles
		dir = OrthoDir((dir + m_angle[adj]) & 3);

		// next adjacency entry in the face
		adj = adj->cyclicSucc();
	}
}


// rotate directions of adjacency entries by r
void OrthoRep::rotate(int r)
{
	const Graph &G = (Graph &) *m_pE;

	if (r < 0) {
		r = r + (-r / 4 + 1) * 4;
	}

	edge e;
	forall_edges(e,G) {
		m_dir[e->adjSource()] =
			OrthoDir((m_dir[e->adjSource()] + r) & 3);

		m_dir[e->adjTarget()] =
			OrthoDir((m_dir[e->adjTarget()] + r) & 3);
	}
}


// computes further information about cages which are collected in class
// VertexInfoUML
void OrthoRep::computeCageInfoUML(
	const PlanRep &PG)
{
	OGDF_ASSERT(&(const Graph &)PG == &m_pE->getGraph());

	if (m_umlCageInfo.valid())
		freeCageInfoUML();

	m_umlCageInfo.init(PG,0);

	node v;
	forall_nodes(v,PG)
	{
		adjEntry adj = PG.expandAdj(v);

		if (adj == 0) continue;

		m_umlCageInfo[v] = OGDF_NEW VertexInfoUML;
		VertexInfoUML &vi = *m_umlCageInfo[v];

		adjEntry adjSucc = adj->faceCycleSucc();

		// look for a corner such that the while-loop below starts by
		// considering adjacency entry adj at the beginning of a side
		while(m_dir[adj] == m_dir[adjSucc]) {
			adj = adjSucc;
			adjSucc = adj->faceCycleSucc();
		}

		int nCorners = 0;
		int attSide = 0;
		while(nCorners < 4) {
			adj = adjSucc;
			adjSucc = adj->faceCycleSucc();

			// reached a corner ?
			if(m_dir[adj] != m_dir[adjSucc]) {
				++nCorners;
				attSide = 0;
				vi.m_corner[m_dir[adjSucc]] = adjSucc;

			} else {
				adjEntry adjAttached = adjSucc->cyclicPred();
				edge eAttached = adjAttached->theEdge();

				if (PG.typeOf(eAttached) == Graph::generalization) {
					vi.m_side[m_dir[adj]].m_adjGen = adjAttached;
					++attSide;
				} else if (PG.original(eAttached) != 0) {
					vi.m_side[m_dir[adj]].m_nAttached[attSide]++;
				}
			}
		}
	}
}


void OrthoRep::freeCageInfoUML()
{
	if( !m_umlCageInfo.valid() ) return;

	const Graph &G = (Graph &) *m_pE;

	node v;
	forall_nodes(v,G) {
		delete m_umlCageInfo[v];
	}
}


} // end namespace ogdf
