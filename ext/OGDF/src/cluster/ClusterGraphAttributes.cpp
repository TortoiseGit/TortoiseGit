/*
 * $Revision: 2616 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-16 15:34:43 +0200 (Mo, 16. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Implement class ClusterGraphAttributes
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


#include <ogdf/cluster/ClusterGraphAttributes.h>
#include <ogdf/cluster/ClusterArray.h>
#include <ogdf/fileformats/GmlParser.h>
#include <ogdf/fileformats/OgmlParser.h>
#include <sstream>


namespace ogdf {


ClusterGraphAttributes::ClusterGraphAttributes(
	ClusterGraph& cg,
	long initAttributes)
: GraphAttributes(cg.getGraph(), initAttributes | edgeType | nodeType |
		nodeGraphics | edgeGraphics), m_clusterTemplate(cg), m_pClusterGraph(&cg)
//we should initialize m__clusterinfo here
{
	//should we always fill the cluster infos here?
}//constructor


//reinitialize graph
void ClusterGraphAttributes::init(ClusterGraph &cg, long initAttributes)
{
	m_pClusterGraph = &cg;
	m_clusterInfo.clear();

	//need to initialize GraphAttributes with getGraph()
	//we only use parameter initAttributes here in constrast
	//to the initialization in the constructor
	GraphAttributes::init(cg.getGraph(), initAttributes );
}


//
// calculates the bounding box of the graph including clusters
const DRect ClusterGraphAttributes::boundingBox() const
{
	DRect bb = GraphAttributes::boundingBox();
	double minx = bb.p1().m_x;
	double miny = bb.p1().m_y;
	double maxx = bb.p2().m_x;
	double maxy = bb.p2().m_y;

	cluster c;
	forall_clusters(c,*m_pClusterGraph)
	{
		if(c == m_pClusterGraph->rootCluster())
			continue;

		double x1 = clusterXPos(c);
		double y1 = clusterYPos(c);
		double x2 = x1 + clusterWidth(c);
		double y2 = y1 + clusterHeight(c);

		if (x1 < minx) minx = x1;
		if (x2 > maxx) maxx = x2;
		if (y1 < miny) miny = y1;
		if (y2 > maxy) maxy = y2;
	}

	return DRect(minx, miny, maxx, maxy);
}


void ClusterGraphAttributes::updateClusterPositions(double boundaryDist)
{
	cluster c;
	//run through children and nodes and update size accordingly
	//we use width, height temporarily to store max values
	forall_postOrderClusters(c,*m_pClusterGraph)
	{
		ListIterator<node> nit = c->nBegin();
		ListConstIterator<ClusterElement*> cit = c->cBegin();
		//Initialize with first element
		if (nit.valid())
		{
			clusterXPos(c->index()) = m_x[*nit] - m_width[*nit]/2;
			clusterYPos(c->index()) = m_y[*nit] - m_height[*nit]/2;
			clusterWidth(c->index()) = m_x[*nit] + m_width[*nit]/2;
			clusterHeight(c->index()) = m_y[*nit] + m_height[*nit]/2;
			nit++;
		}
		else
		{
			if (cit.valid())
			{
				clusterXPos(c->index()) = clusterXPos(*cit);
				clusterYPos(c->index()) = clusterYPos(*cit);
				clusterWidth(c->index()) = clusterXPos(*cit) + clusterWidth(*cit);
				clusterHeight(c->index()) = clusterYPos(*cit) + clusterHeight(*cit);
				cit++;
			}
			else
			{
				clusterXPos(c->index()) = 0.0;
				clusterYPos(c->index()) = 0.0;
				clusterWidth(c->index()) = 1.0;
				clusterHeight(c->index()) = 1.0;
			}
		}
		//run through elements and update
		while (nit.valid())
		{
			if (clusterXPos(c->index()) > m_x[*nit] - m_width[*nit]/2)
				clusterXPos(c->index()) = m_x[*nit] - m_width[*nit]/2;
			if (clusterYPos(c->index()) > m_y[*nit] - m_height[*nit]/2)
				clusterYPos(c->index()) = m_y[*nit] - m_height[*nit]/2;
			if (clusterWidth(c->index()) < m_x[*nit] + m_width[*nit]/2)
				clusterWidth(c->index()) = m_x[*nit] + m_width[*nit]/2;
			if (clusterHeight(c->index()) < m_y[*nit] + m_height[*nit]/2)
				clusterHeight(c->index()) = m_y[*nit] + m_height[*nit]/2;
			nit++;
		}
		while (cit.valid())
		{
			if (clusterXPos(c->index()) > clusterXPos((*cit)->index()))
				clusterXPos(c->index()) = clusterXPos((*cit)->index());
			if (clusterYPos(c->index()) > clusterYPos((*cit)->index()))
				clusterYPos(c->index()) = clusterYPos((*cit)->index());
			if (clusterWidth(c->index()) < clusterXPos((*cit)->index()) + clusterWidth((*cit)->index()))
				clusterWidth(c->index()) = clusterXPos((*cit)->index()) + clusterWidth((*cit)->index());
			if (clusterHeight(c->index()) < clusterYPos((*cit)->index()) + clusterHeight((*cit)->index()))
				clusterHeight(c->index()) = clusterYPos((*cit)->index()) + clusterHeight((*cit)->index());
			cit++;
		}
		clusterXPos(c->index()) -= boundaryDist;
		clusterYPos(c->index()) -= boundaryDist;
		clusterWidth(c->index()) = clusterWidth(c->index()) - clusterXPos(c->index()) + boundaryDist;
		clusterHeight(c->index()) = clusterHeight(c->index()) - clusterYPos(c->index()) + boundaryDist;
	}
}


void ClusterGraphAttributes::writeGML(const char *fileName)
{
	ofstream os(fileName);
	writeGML(os);
}


void ClusterGraphAttributes::writeGML(ostream &os)
{
	NodeArray<int>    nId(*m_pGraph);

	int nextId = 0;

	os.setf(ios::showpoint);

	GraphAttributes::writeGML(os);

	// set index string for cluster entries
	node v;
	forall_nodes(v,*m_pGraph)
	{
		nId[v] = nextId++;
	}

	// output the cluster information
	String indent = "\0";
	nextId = 1;
	writeGraphWinCluster(os, nId, nextId,
		m_pClusterGraph->rootCluster(), indent);
}


// recursively write the cluster structure in GML
void ClusterGraphAttributes::writeCluster(
	ostream &os,
	NodeArray<int> &nId,
	ClusterArray<int> & cId,
	int &nextId,
	cluster c,
	String indent)
{
	String newindent = indent;
	newindent += "  ";
	os << indent << "cluster [\n";
	os << indent << "  id " << (cId[c] = nextId++) << "\n";
	ListConstIterator<cluster> it;
	for (it = c->cBegin(); it.valid(); it++)
		writeCluster(os,nId,cId,nextId,*it,newindent);
	ListConstIterator<node> itn;
	for (itn = c->nBegin(); itn.valid(); itn++)
		os << indent << "  node " << nId[*itn] << "\n";
	os << indent << "]\n"; // cluster
}


// recursively write the cluster structure in GraphWin GML
void ClusterGraphAttributes::writeGraphWinCluster(
	ostream        &os,
	NodeArray<int> &nId,
	int            &nextId,
	cluster        c,
	String         indent
	)
{
	String newindent = indent;
	newindent += "  ";

	if (c == m_pClusterGraph->rootCluster())
		os << indent << "rootcluster [\n";
	else
	{
		os << indent << "cluster [\n";
		os << indent << "  id " << c->index() << "\n";

		const String &templStr = m_clusterTemplate[c];
		if(templStr.length() > 0) {
			// GDE extension: Write cluster template and custom attribute
			os << "  template ";
			writeLongString(os, templStr);
			os << "\n";

			os << "  label ";
			writeLongString(os, clusterLabel(c));
			os << "\n";

		} else {
			os << indent << "  label \"" << clusterLabel(c) << "\"\n";
		}

		os << indent << "  graphics [\n";

		double shiftPos;
		shiftPos = clusterYPos(c->index());

		os << indent << "    x " << clusterXPos(c->index()) << "\n";
		os << indent << "    y " << shiftPos/*clusterYPos(c->index())*/ << "\n";

		os << indent << "    width " << clusterWidth(c->index()) << "\n";
		os << indent << "    height " << clusterHeight(c->index()) << "\n";
		os << indent << "    fill \""  << clusterFillColor(c->index()) << "\"\n";
		os << indent << "    pattern "  << clusterFillPattern(c->index()) << "\n";

		//border line styles
		os << indent << "    color \"" << clusterColor(c) << "\"\n";
		os << indent << "    lineWidth " << clusterLineWidth(c) << "\n";
		//save space by defaulting
		if (clusterLineStyle(c) != esSolid)
			os << indent << "    stipple " << clusterLineStyle(c) << "\n";

		os << indent << "    style \"rectangle\"\n";

		os << indent << "  ]\n"; //graphics
	}

	// write contained clusters
	ListConstIterator<cluster> it;
	for (it = c->cBegin(); it.valid(); it++)
		writeGraphWinCluster(os, nId, nextId, *it, newindent);

	// write contained nodes
	ListConstIterator<node> itn;
	for (itn = c->nBegin(); itn.valid(); itn++)
		os << indent << "vertex \"" << nId[*itn] << "\"\n";

	os << indent << "]\n"; // cluster
}


const char NEWLINE('\n');		// newline character
const char INDENTCHAR(' ');		// indent character
const int  INDENTSIZE(2); 		// indent size


class omani
{
	int m_n;
	ostream& (*m_f)(ostream&, int);

public:
	omani(ostream& (*f)(ostream&, int), int n) : m_n(n), m_f(f) { }

	friend ostream& operator<<(ostream& os, omani man) {
		return man.m_f(os, man.m_n);
	}
};

ostream& padN(ostream& os, int depth)
{
	int n = INDENTSIZE * depth;
	for( ; n > 0; --n)
		os.put(INDENTCHAR);

	return os;
}

omani ind(int depth)
{
	return omani(&padN, depth);
}


ostream &ind(ostream &os, int depth)
{
	int n = INDENTSIZE * depth;
	for( ; n > 0; --n)
		os.put(INDENTCHAR);

	return os;
}

void ClusterGraphAttributes::writeOGML(const char *fileName)//, GraphConstraints &GC)
{
	ofstream os(fileName);
	writeOGML(os);//, GC);
}


void ClusterGraphAttributes::writeOGML(ostream & os) //, GraphConstraints & GC)
{
	int labelId = 0;						// new ID of current label
	int pointId = 0;						// new ID of current point

	int indentDepth = 0; 					// main indent depth
	int indentDepthS = 4; 					// indent depth for styles block
	std::ostringstream osS; 				// string output stream for buffering the styles of the handled elements
	std::ostringstream osC; 				// string output stream for buffering constraints

	// CONFIGURING OUTPUT STREAMS
	os.setf(ios::showpoint);
	os.precision(10);
	osS.setf(ios::showpoint);
	osS.precision(10);
	osC.setf(ios::showpoint);
	osC.precision(10);

	// XML DECLARATION AND OGML TAG
	os << "<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?>" << NEWLINE; // Latin-1
	// Simple version
	os << "<ogml>" << NEWLINE;
	// Strict version
	// os << "<ogml xmlns=\"http://www.ogdf.org\"";
	// os << " xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"";
	// os << " xsi:schemaLocation=\"http://www.ogdf.org http://www.ogdf.org/ogml.xsd\">" << NEWLINE;

	// WRITING GRAPH BLOCK
	os << ind(++indentDepth) << "<graph>" << NEWLINE;

	// WRITING STRUCTURE BLOCK
	os << ind(++indentDepth) << "<structure>" << NEWLINE;

	// recursive handling of clusters
	writeClusterOGML(os, osS, labelId, m_pClusterGraph->rootCluster(), ++indentDepth, indentDepthS);

	// handling of edges
	edge e;
	forall_edges(e, *m_pGraph)
	{
		// EDGE STRUCTURE
		os << ind(indentDepth) << "<edge id=\"e" << e->index() << "\">" << NEWLINE;
		++indentDepth;

		// handling of label (if exists)
		if (attributes() & edgeLabel) {
			os << ind(indentDepth)   << "<label id=\"l" << labelId++ << "\">" << NEWLINE;
			os << ind(indentDepth+1) << "<content>" << formatLabel(labelEdge(e)) << "</content>" << NEWLINE;
			os << ind(indentDepth)   << "</label>" << NEWLINE;
		}

		os << ind(indentDepth) << "<source idRef=\"n" << e->source()->index() << "\"/>" << NEWLINE;
		os << ind(indentDepth) << "<target idRef=\"n" << e->target()->index() << "\"/>" << NEWLINE;

		/*
		* TODO find solution for handling the edge types (app specific data?)
		*
		* os << "GENERALIZATION=\"" << (m_eType[e]==Graph::generalization?1:0) << "\">" << delimiter;
		*/

		os << ind(--indentDepth) << "</edge>" << NEWLINE; // edge

		// EDGE LAYOUT
		if (attributes() & edgeGraphics || attributes() & edgeColor || attributes() & edgeStyle)
		{
			osS << ind(indentDepthS) << "<edgeStyle idRef=\"e" << e->index() << "\">" << NEWLINE;

			// handling of style information
			if(attributes() & edgeStyle || attributes() & edgeColor)
			{
				osS << ind(indentDepthS+1) << "<line ";
				if (attributes() & edgeStyle) {
					osS << "type=\"" << edgeStyleToOGML(styleEdge(e)) << "\" width=\"" << m_edgeWidth[e] << "\" ";
				}
				if (attributes() & edgeColor) {
					osS << "color=\"" << colorEdge(e) << "\" />" << NEWLINE;
				} else 	{
					osS << "/>" << NEWLINE;
				}
			}

			// TODO review the handling of edge arrows
			if(attributes() & edgeArrow)
			{
				++indentDepthS;
				switch(arrowEdge(e)) {
				case GraphAttributes::none:
					osS << ind(indentDepthS) << "<sourceStyle type=\"none\" color=\"#000000\" size=\"1\" />" << NEWLINE;
					osS << ind(indentDepthS) << "<targetStyle type=\"none\" color=\"#000000\" size=\"1\" />" << NEWLINE;
					break;
				case GraphAttributes::last:
					osS << ind(indentDepthS) << "<sourceStyle type=\"none\" color=\"#000000\" size=\"1\" />" << NEWLINE;
					osS << ind(indentDepthS) << "<targetStyle type=\"arrow\" color=\"#000000\" size=\"1\" />" << NEWLINE;
					break;
				case GraphAttributes::first:
					osS << ind(indentDepthS) << "<sourceStyle type=\"arrow\" color=\"#000000\" size=\"1\" />" << NEWLINE;
					osS << ind(indentDepthS) << "<targetStyle type=\"none\" color=\"#000000\" size=\"1\" />" << NEWLINE;
					break;
				case GraphAttributes::both:
					osS << ind(indentDepthS) << "<sourceStyle type=\"arrow\" color=\"#000000\" size=\"1\" />" << NEWLINE;
					osS << ind(indentDepthS) << "<targetStyle type=\"arrow\" color=\"#000000\" size=\"1\" />" << NEWLINE;
					break;
				case GraphAttributes::undefined:
					// do nothing
					break;
				default:
					// do nothing
					break;
				}
				--indentDepthS;
			}

			// handling of points
			const DPolyline &dpl = m_bends[e];
			if (!dpl.empty()) {
				++indentDepthS;
				// handle source
				node v = e->source();
				if(dpl.front().m_x < m_x[v] - m_width[v]/2 ||
					dpl.front().m_x > m_x[v] + m_width[v]/2 ||
					dpl.front().m_y < m_y[v] - m_height[v]/2 ||
					dpl.front().m_y > m_y[v] + m_height[v]/2)	{
						osS << ind(indentDepthS) << "<point id=\"p" << pointId++ << "\" x=\"" << m_x[e->source()] << "\" y=\"" << m_y[e->source()] << "\"/>" << NEWLINE;
				}
				// handle points
				ListConstIterator<DPoint> it;
				for(it = dpl.begin(); it.valid(); ++it) {
					osS << ind(indentDepthS) << "<point id=\"p" << pointId++ << "\" x=\"" << (*it).m_x << "\" y=\"" << (*it).m_y << "\"/>" << NEWLINE;
				}
				// handle target
				v = e->target();
				if(dpl.back().m_x < m_x[v] - m_width[v]/2 ||
					dpl.back().m_x > m_x[v] + m_width[v]/2 ||
					dpl.back().m_y < m_y[v] - m_height[v]/2 ||
					dpl.back().m_y > m_y[v] + m_height[v]/2) {
						osS << ind(indentDepthS) << "<point id=\"p" << pointId++ << "\" x=\"" << m_x[e->target()] << "\" y=\"" << m_y[e->target()] << "\"/>" << NEWLINE;
				}
				--indentDepthS;
			}

			osS << ind(indentDepthS) << "</edgeStyle>" << NEWLINE;
		}
	}

	--indentDepth;
	os << ind(indentDepth) << "</structure>" << NEWLINE;

	// WRITING LAYOUT BLOCK
	os << ind(indentDepth) << "<layout>" << NEWLINE;

	// WRITING STYLES
	++indentDepth;
	os << ind(indentDepth) << "<styles>" << NEWLINE;
	os << osS.str();
	os << ind(indentDepth) << "</styles>" << NEWLINE;

	// 2.2.2) WRITING CONSTRAINTS
	// No constraint handling so far in OGDF
	/*
	List<Constraint *> * csList = GC.getConstraints();
	ListConstIterator<Constraint *> it;
	int constID = 0;
	if (csList->size() > 0) {
	os << indent << "<constraints>" << NEWLINE;
	for (it = csList->begin(); it.valid(); ++it) {
	(*it)->storeToOgml(constID++, os, indentDepth + indentDepthS + 1);
	}
	os << indent << "</constraints>" << NEWLINE;
	cout << "Constraints written...\n" << flush;
	}
	*/

	--indentDepth;
	os << ind(indentDepth) << "</layout>" << NEWLINE;

	--indentDepth;
	os << ind(indentDepth) << "</graph>" << NEWLINE;

	os << "</ogml>";
}


// recursively write the cluster structure in OGML
void ClusterGraphAttributes::writeClusterOGML(
	ostream & os,
	std::ostringstream & osS,
	int & nextLabelId,
	cluster clust,
	int & indentDepth,
	int indentDepthS)
{
	// we handle all cluster except the root cluster
	if (clust != m_pClusterGraph->rootCluster()) {
		/* cluster structure infos */
		os << ind(indentDepth) << "<node id=\"c" << clust->index() << "\">" << NEWLINE;

		/*
		* TODO What are cluster templates and how can/should they be handled?
		*
		const String &templStr = m_clusterTemplate[cluster];
		if(templStr.length() > 0) {
		// GDE extension: Write cluster template and custom attribute
		os << "template ";
		writeLongString(os, templStr);
		os << "\n";
		os << "label ";
		writeLongString(os, clusterLabel(cluster));
		os << "\n";
		} else {
		*/

		++indentDepth;
		os << ind(indentDepth)   << "<label id=\"l" << nextLabelId++ << "\">" << NEWLINE;
		os << ind(indentDepth+1) << "<content>" << formatLabel(clusterLabel(clust)) << "</content>" << NEWLINE;
		os << ind(indentDepth)   << "</label>" << NEWLINE;
	}

	// we handle the contained nodes first
	ListConstIterator<node> itn;
	for (itn = clust->nBegin(); itn.valid(); ++itn)
	{
		node v = *itn;

		// node structure infos
		os << ind(indentDepth) << "<node id=\"n" << v->index() << "\">" << NEWLINE;

		// handling of label (if exists)
		if (attributes() & nodeLabel) {
			os << ind(indentDepth+1) << "<label id=\"l" << nextLabelId++ << "\">" << NEWLINE;
			os << ind(indentDepth+2) << "<content>" << formatLabel(labelNode(v)) << "</content>" << NEWLINE;
			os << ind(indentDepth+1) << "</label>" << NEWLINE;
		}

		os << ind(indentDepth) << "</node>" << NEWLINE;

		// node layout infos
		osS << ind(indentDepthS) << "<nodeStyle idRef=\"n" << v->index() << "\">" << NEWLINE;
		++indentDepthS;
		osS << ind(indentDepthS) << "<location x=\"" << x(v)-0.5*width(v) << "\" y=\""<< y(v)-0.5*height(v) << "\"/>" << NEWLINE;
		osS << ind(indentDepthS) << "<shape type=\"";
		switch (m_nodeShape[v]) {
		case rectangle:
			osS << "rect";
			break;
		case oval:
			osS << "ellipse";
			break;
		}
		osS << "\" width=\"" << m_width[v] << "\" height=\"" << m_height[v] << "\"/>" << NEWLINE;
		if(attributes() & nodeColor || attributes() & nodeStyle) {
			// fill-tag
			osS << ind(indentDepthS) << "<fill";
			// color-attribute of fill-tag
			if (attributes() & nodeColor) {
				if (m_nodeColor[v].length() > 0) {
					osS << " color=\"" << m_nodeColor[v] << "\"";
				}
			}
			if (attributes() & nodeStyle) {
				// pattern- and patternColor-attribute of fill-tag (closing)
				osS << " pattern=\"" << brushPatternToOGML(m_nodePattern[v]) << "\" patternColor=\"#000000\"/>" << NEWLINE;
				// line-tag
				osS << ind(indentDepthS) << "<line type=\"" << edgeStyleToOGML(m_nodeStyle[v]) <<  "\" width=\"" << m_nodeLineWidth[v] << "\" color=\"" << m_nodeLine[v] << "\"/>" << NEWLINE;
			} else 	{
				// closing fill-tag
				osS << "/>" << NEWLINE;
			}
		}

		--indentDepthS;
		osS << ind(indentDepthS) << "</nodeStyle>" << NEWLINE;
	}//for clusters

	// now we recursively handle the contained clusters
	ListConstIterator<cluster> it;
	for (it = clust->cBegin(); it.valid(); ++it) {
		writeClusterOGML(os, osS, nextLabelId, *it, indentDepth, indentDepthS);
	}

	// we handle all clusters except the root cluster
	if (clust != m_pClusterGraph->rootCluster()) {
		--indentDepth;
		os << ind(indentDepth) << "</node>" << NEWLINE;

		// cluster layout infos
		osS << ind(indentDepthS) << "<nodeStyle idRef=\"c" << clust->index() << "\">" << NEWLINE;

		++indentDepthS;
		osS << ind(indentDepthS) << "<location x=\"" << clusterXPos(clust) << "\" y=\"" << clusterYPos(clust) << "\"/>" << NEWLINE;
		osS << ind(indentDepthS) << "<shape type=\"rect\" width=\"" << clusterWidth(clust) << "\" height=\"" << clusterHeight(clust) << "\"/>" << NEWLINE;
		if(clusterFillColor(clust).length() > 0) {
			osS << ind(indentDepthS) << "<fill color=\"" << clusterFillColor(clust) << "\"";
			osS << " pattern=\"" << brushPatternToOGML(clusterFillPattern(clust)) << "\" patternColor=\"#000000\"/>" << NEWLINE;
		}
		osS << ind(indentDepthS) << "<line type=\"" << edgeStyleToOGML(clusterLineStyle(clust)) << "\" width=\"" << clusterLineWidth(clust) << "\" color=\"" << clusterColor(clust) << "\"/>" << NEWLINE;
		--indentDepthS;

		osS << ind(indentDepthS) << "</nodeStyle>" << NEWLINE;
	}
}


//++++++++++++++++++++++++++++++++++++++++++++
//reading graph, attributes, cluster structure
bool ClusterGraphAttributes::readClusterGML(
	const char* fileName,
	ClusterGraph& CG,
	Graph& G)
{

	ifstream is(fileName);
	if (!is)
		return false; // couldn't open file

	return readClusterGML(is, CG, G);
}


bool ClusterGraphAttributes::readClusterGML(
	istream& is,
	ClusterGraph& CG,
	Graph& G)
{

	bool result;
	GmlParser gml(is);
	if (gml.error())
		return false;

	result = gml.read(G,*this);

	if (!result) return false;

	return readClusterGraphGML(CG, G, gml);
}


//read Cluster Graph with Attributes, base graph G, from fileName
bool ClusterGraphAttributes::readClusterGraphGML(
	const char* fileName,
	ClusterGraph& CG,
	Graph& G,
	GmlParser& gml)
{
	ifstream is(fileName);
	if (!is)
		return false; // couldn't open file

	return readClusterGraphGML(CG, G, gml);
}


//read from input stream
bool ClusterGraphAttributes::readClusterGraphGML(
	ClusterGraph& CG,
	Graph& G,
	GmlParser& gml)
{
	return gml.readAttributedCluster(G, CG, *this);
}


// read Cluster Graph from OGML file
bool ClusterGraphAttributes::readClusterGraphOGML(
	const char* fileName,
	ClusterGraph& CG,
	Graph& G)
{
	ifstream is(fileName);
	if (!is)
		return false;

	OgmlParser op;
	return op.read(fileName, G, CG, *this);
};


ostream &operator<<(ostream &os, ogdf::cluster c)
{
	if (c) os << c->index(); else os << "nil";
	return os;
}


} // end namespace ogdf

