/*
 * $Revision: 2599 $
 *
 * last checkin:
 *   $Author: chimani $
 *   $Date: 2012-07-15 22:39:24 +0200 (So, 15. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief implementation of the wrapper class of the Boyer-Myrvold planarity test
 *
 * \author Jens Schmidt
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


#include <ogdf/planarity/BoyerMyrvold.h>
#include <ogdf/planarity/ExtractKuratowskis.h>


namespace ogdf {


// returns true, if g is planar, false otherwise. this is the
// routine, which avoids the overhead of copying the input graph.
// it is therefore not suitable, if your graph must not be changed.
bool BoyerMyrvold::isPlanarDestructive(Graph& g)
{
	clear();
	nOfStructures = 0;

	// less than 9 edges are always planar
	if (g.numberOfEdges() < 9) return true;

	SListPure<KuratowskiStructure> dummy;
	pBMP = new BoyerMyrvoldPlanar(g,false,BoyerMyrvoldPlanar::doNotEmbed,false,
									dummy,false,true);
	return pBMP->start();
}


// returns true, if g is planar, false otherwise.
// use this slower routine, if your graph must not be changed.
bool BoyerMyrvold::isPlanar(const Graph& g)
{
	clear();
	nOfStructures = 0;

	// less than 9 edges are always planar
	if (g.numberOfEdges() < 9) return true;

	Graph h(g);
	SListPure<KuratowskiStructure> dummy;
	pBMP = new BoyerMyrvoldPlanar(h,false,BoyerMyrvoldPlanar::doNotEmbed,false,
									dummy,false,true);
	return pBMP->start();
}


// Transforms KuratowskiWrapper in KuratowskiSubdivision
void BoyerMyrvold::transform(
	const KuratowskiWrapper& source,
	KuratowskiSubdivision& target,
	NodeArray<int>& count,
	EdgeArray<int>& countEdge)
{
	// init linear counting structure
	node kn[6];
	int k = 0;
	SListConstIterator<edge> itE;
	for (itE = source.edgeList.begin(); itE.valid(); ++itE) {
		const edge& e(*itE);
		OGDF_ASSERT(!countEdge[e]);
		countEdge[e] = 1;
		if (++count[e->source()] == 3) kn[k++] = e->source();
		if (++count[e->target()] == 3) kn[k++] = e->target();
	}

	// transform edgelist of KuratowskiSubdivision to KuratowskiWrapper
	OGDF_ASSERT(k==5 || k==6);
	node n;
	edge e,f,h;
	List<edge> L;
	if (k==5) { // K5
		kn[5] = 0;
		target.init(10);
		for (int k = 0; k<5; k++) {
			forall_adj_edges(e,kn[k]) {
				if (!countEdge[e]) continue;
				n = kn[k];
				f = e;
				// traverse degree-2-path
				while (count[n = f->opposite(n)] == 2) {
					L.pushBack(f);
					forall_adj_edges(h,n) {
						if (countEdge[h] && h != f) {
							f = h;
							break;
						}
					}
				}
				L.pushBack(f);
				int i = 0;
				while (kn[i] != n) i++;
				if (i > k) {
					if (k==0) i--;
					else if (k==1) i+=2;
					else i += k+2;
					target[i].conc(L);
				} else L.clear();
			}
		}
	} else { // k33
		target.init(9);
		int touched[6] = { -1, -1, -1, -1, -1, -1}, t=0, i=0;
		for (int k = 0; k<6; k++) {
			if (touched[k] != -1) continue;
			forall_adj_edges(e,kn[k]) {
				if (!countEdge[e]) continue;
				n = kn[k];
				f = e;
				while(count[n = f->opposite(n)] == 2) {
					L.pushBack(f);
					forall_adj_edges(h,n) {
						if (countEdge[h] && h != f) {
							f = h;
							break;
						}
					}
				}
				L.pushBack(f);
				int j = 0;
				while (kn[j] != n) j++;
				if (touched[j] == -1)
					touched[j] = t++;
				target[i*3 + touched[j]].conc(L);
			}
			i++;
		}
	}

	// destruct linear counting structure
	for (itE = source.edgeList.begin(); itE.valid(); ++itE) {
		const edge& e(*itE);
		countEdge[e] = 0;
		count[e->source()] = 0;
		count[e->target()] = 0;
	}
}

// Transforms KuratowskiWrapper-List in KuratowskiSubdivision-List with respect to sieving constraints
void BoyerMyrvold::transform(
	const SList<KuratowskiWrapper>& sourceList,
	SList<KuratowskiSubdivision>& targetList,
	const Graph& g,
	const bool onlyDifferent)
{
	if (sourceList.empty()) return;
	targetList.clear();
	NodeArray<int> count(g,0);
	EdgeArray<int> countEdge(g,0);
	SListConstIterator<KuratowskiWrapper> it;
	node lastEmbeddedVertex = NULL;

	// transform each KuratowskiWrapper into KuratowskiSubdivision
	for (it = sourceList.begin(); it.valid(); ++it) {
		if (!onlyDifferent || (*it).V != lastEmbeddedVertex) {
			lastEmbeddedVertex = (*it).V;
			KuratowskiSubdivision s;
			transform(*it,s,count,countEdge);

			targetList.pushBack(s);
		}
	}
}

// returns true, if g is planar, false otherwise. in addition,
// g contains a planar embedding, if planar. if not planar,
// kuratowski subdivisions are added to output.
// use this function, if g may be changed.
// use embeddingGrade to bound the overall number of extracted kuratowski subdivisions;
// use the value 0 to extract no kuratowski subdivision and the value -1 to find as much
// as possible. value -2 doesn't even invoke the FIND-procedure.
bool BoyerMyrvold::planarEmbedDestructive(
	Graph& g,
	SList<KuratowskiWrapper>& output,
	int embeddingGrade,
	bool bundles,
	bool limitStructures,
	bool randomDFSTree,
	bool avoidE2Minors)
{
	OGDF_ASSERT(embeddingGrade != BoyerMyrvoldPlanar::doNotEmbed);

	clear();
	SListPure<KuratowskiStructure> dummy;
	pBMP = new BoyerMyrvoldPlanar(g,bundles,embeddingGrade,limitStructures,dummy,
									randomDFSTree,avoidE2Minors);
	bool planar = pBMP->start();
	OGDF_ASSERT(!planar || g.genus()==0);

	nOfStructures = dummy.size();

	// Kuratowski extraction
	if (embeddingGrade > BoyerMyrvoldPlanar::doFindZero ||
				embeddingGrade == BoyerMyrvoldPlanar::doFindUnlimited) {
		ExtractKuratowskis extract(*pBMP);
		if (bundles) {
			extract.extractBundles(dummy,output);
		} else {
			extract.extract(dummy,output);
		}
		OGDF_ASSERT(planar || !output.empty());
	}
	return planar;
}

// returns true, if g is planar, false otherwise. in addition,
// h contains a planar embedding, if planar. if not planar, list
// contains a kuratowski subdivision.
// use this slower function, if g must not be changed.
// use embeddingGrade to bound the overall number of extracted kuratowski subdivisions;
// use the value 0 to extract no kuratowski subdivision and the value -1 to find as much
// as possible. value -2 doesn't even invoke the FIND-procedure.
bool BoyerMyrvold::planarEmbed(
	Graph& g,
	SList<KuratowskiWrapper>& output,
	int embeddingGrade,
	bool bundles,
	bool limitStructures,
	bool randomDFSTree,
	bool avoidE2Minors)
{
	OGDF_ASSERT(embeddingGrade != BoyerMyrvoldPlanar::doNotEmbed);

	clear();
	GraphCopySimple h(g);
	SListPure<KuratowskiStructure> dummy;
	pBMP = new BoyerMyrvoldPlanar(h,bundles,embeddingGrade,limitStructures,dummy,
									randomDFSTree,avoidE2Minors);
	bool planar = pBMP->start();
	OGDF_ASSERT(!planar || h.genus()==0);

	nOfStructures = dummy.size();

	// Kuratowski extraction
	if (embeddingGrade > BoyerMyrvoldPlanar::doFindZero ||
				embeddingGrade == BoyerMyrvoldPlanar::doFindUnlimited) {
		ExtractKuratowskis extract(*pBMP);
		if (bundles) {
			extract.extractBundles(dummy,output);
		} else {
			extract.extract(dummy,output);
		}
		OGDF_ASSERT(planar || !output.empty());

		// convert kuratowski edges in original graph edges
		if (!output.empty()) {
			SListIterator<KuratowskiWrapper> it;
			SListIterator<edge> itE;
			for (it = output.begin(); it.valid(); ++it) {
				for (itE = (*it).edgeList.begin(); itE.valid(); ++itE)
					(*itE) = h.original(*itE);
			}
		}
	}

	// copy adjacency lists, if planar
	if (planar) {
		node v;
		adjEntry adj;
		SListPure<adjEntry> entries;
		forall_nodes(v,g) {
			entries.clear();
			forall_adj(adj,h.copy(v)) {
				OGDF_ASSERT(adj->theNode() == h.copy(v));
				edge e = h.original(adj->theEdge());
				OGDF_ASSERT(e->graphOf() == &g);
				//if (e->source() == v) {
				if(adj == adj->theEdge()->adjSource()) {
					entries.pushBack(e->adjSource());
					OGDF_ASSERT(e->adjSource()->theNode() == v);
				} else {
					entries.pushBack(e->adjTarget());
					OGDF_ASSERT(e->adjTarget()->theNode() == v);
				}
			}
			g.sort(v,entries);
		}
	}

	return planar;
}

// returns true, if graph copy h is planar, false otherwise. in addition,
// h contains a planar embedding, if planar. if not planar, list
// contains a kuratowski subdivision.
// use this slower function, if g must not be changed.
// use embeddingGrade to bound the overall number of extracted kuratowski subdivisions;
// use the value 0 to extract no kuratowski subdivision and the value -1 to find as much
// as possible. value -2 doesn't even invoke the FIND-procedure.
bool BoyerMyrvold::planarEmbed(
	//const Graph& g,
	GraphCopySimple& h,
	SList<KuratowskiWrapper>& output,
	int embeddingGrade,
	bool bundles,
	bool limitStructures,
	bool randomDFSTree,
	bool avoidE2Minors)
{
	OGDF_ASSERT(embeddingGrade != BoyerMyrvoldPlanar::doNotEmbed);

	clear();
	//OGDF_ASSERT(&h.original() == &g);
	SListPure<KuratowskiStructure> dummy;
	pBMP = new BoyerMyrvoldPlanar(h,bundles,embeddingGrade,limitStructures,dummy,
									randomDFSTree,avoidE2Minors);
	bool planar = pBMP->start();
	OGDF_ASSERT(!planar || h.genus()==0);

	nOfStructures = dummy.size();

	// Kuratowski extraction
	if (embeddingGrade > BoyerMyrvoldPlanar::doFindZero ||
				embeddingGrade == BoyerMyrvoldPlanar::doFindUnlimited) {
		ExtractKuratowskis extract(*pBMP);
		if (bundles) {
			extract.extractBundles(dummy,output);
		} else {
			extract.extract(dummy,output);
		}
		OGDF_ASSERT(planar || !output.empty());

		// convert kuratowski edges in original graph edges
		if (!output.empty()) {
			SListIterator<KuratowskiWrapper> it;
			SListIterator<edge> itE;
			for (it = output.begin(); it.valid(); ++it) {
				for (itE = (*it).edgeList.begin(); itE.valid(); ++itE)
					(*itE) = h.original(*itE);
			}
		}
	}

	return planar;
}

}
