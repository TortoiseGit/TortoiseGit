/*
 * $Revision: 2565 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-07 17:14:54 +0200 (Sa, 07. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Implementation of GML parser (class GmlParser)
 * (used for parsing and reading GML files)
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


#include <ogdf/fileformats/GmlParser.h>
#include <ctype.h>
#include <string.h>

extern ofstream os;

namespace ogdf {

GmlParser::GmlParser(const char *fileName, bool doCheck)
{
	ifstream is(fileName, ios::in);  // open file
	doInit(is,doCheck);
}


GmlParser::GmlParser(istream &is, bool doCheck)
{
	doInit(is,doCheck);
}


void GmlParser::doInit(istream &is, bool doCheck)
{
	m_objectTree = 0;

	if (!is) {
		setError("Cannot open file.");
		return;
	}

	createObjectTree(is,doCheck);

	int minId, maxId;
	m_graphObject = getNodeIdRange(minId, maxId);
	m_mapToNode.init(minId,maxId,0);
}


void GmlParser::createObjectTree(istream &is, bool doCheck)
{
	initPredefinedKeys();
	m_error = false;

	m_is = &is;
	m_doCheck = doCheck; // indicates more extensive checking

	// initialize line buffer (note: GML specifies a maximal line length
	// of 254 characters!)
	m_rLineBuffer = new char[256];
	if (m_rLineBuffer == 0) OGDF_THROW(InsufficientMemoryException);

	*m_rLineBuffer = '\n';
	m_lineBuffer = m_rLineBuffer+1;

	m_pCurrent = m_pStore = m_lineBuffer;
	m_cStore = 0; // forces getNextSymbol() to read first line

	// create object tree
	m_objectTree = parseList(gmlEOF,gmlListEnd);

	delete[] m_rLineBuffer;
}

// we use predefined id constants for all relevant keys
// this allows us to use efficient switch() statemnts in read() methods
void GmlParser::initPredefinedKeys()
{
	m_hashTable.fastInsert("id",       idPredefKey);
	m_hashTable.fastInsert("label",    labelPredefKey);
	m_hashTable.fastInsert("Creator",  CreatorPredefKey);
	m_hashTable.fastInsert("name",     namePredefKey);
	m_hashTable.fastInsert("graph",    graphPredefKey);
	m_hashTable.fastInsert("version",  versionPredefKey);
	m_hashTable.fastInsert("directed", directedPredefKey);
	m_hashTable.fastInsert("node",     nodePredefKey);
	m_hashTable.fastInsert("edge",     edgePredefKey);
	m_hashTable.fastInsert("graphics", graphicsPredefKey);
	m_hashTable.fastInsert("x",        xPredefKey);
	m_hashTable.fastInsert("y",        yPredefKey);
	m_hashTable.fastInsert("w",        wPredefKey);
	m_hashTable.fastInsert("h",        hPredefKey);
	m_hashTable.fastInsert("type",     typePredefKey);
	m_hashTable.fastInsert("width",    widthPredefKey);
	m_hashTable.fastInsert("source",   sourcePredefKey);
	m_hashTable.fastInsert("target",   targetPredefKey);
	m_hashTable.fastInsert("arrow",    arrowPredefKey);
	m_hashTable.fastInsert("Line",     LinePredefKey);
	m_hashTable.fastInsert("line",     linePredefKey);
	m_hashTable.fastInsert("point",    pointPredefKey);
	m_hashTable.fastInsert("generalization", generalizationPredefKey);
	m_hashTable.fastInsert("subgraph", subGraphPredefKey);
	m_hashTable.fastInsert("fill",     fillPredefKey);
	m_hashTable.fastInsert("cluster",     clusterPredefKey);
	m_hashTable.fastInsert("rootcluster", rootClusterPredefKey);
	m_hashTable.fastInsert("vertex",    vertexPredefKey);
	m_hashTable.fastInsert("color",		colorPredefKey);
	m_hashTable.fastInsert("height",		heightPredefKey);
	m_hashTable.fastInsert("stipple",   stipplePredefKey);  //linestyle
	m_hashTable.fastInsert("pattern",    patternPredefKey); //brush pattern
	m_hashTable.fastInsert("lineWidth", lineWidthPredefKey);//line width
	m_hashTable.fastInsert("template", templatePredefKey);//line width
	m_hashTable.fastInsert("weight", edgeWeightPredefKey);

	// further keys get id's starting with NEXTPREDEFKEY
	m_num = NEXTPREDEFKEY;
}


GmlObject *GmlParser::parseList(GmlObjectType closingKey,
	GmlObjectType /* errorKey */)
{
	GmlObject *firstSon = 0;
	GmlObject **pPrev = &firstSon;

	for( ; ; ) {
		GmlObjectType symbol = getNextSymbol();

		if (symbol == closingKey || symbol == gmlError)
			return firstSon;

		if (symbol != gmlKey) {
			setError("key expected");
			return firstSon;
		}

		GmlKey key = m_keySymbol;

		symbol = getNextSymbol();
		GmlObject *object = 0;

		switch (symbol) {
		case gmlIntValue:
			object = OGDF_NEW GmlObject(key,m_intSymbol);
			break;

		case gmlDoubleValue:
			object = OGDF_NEW GmlObject(key,m_doubleSymbol);
			break;

		case gmlStringValue: {
			size_t len = strlen(m_stringSymbol)+1;
			char *pChar = new char[len];
			if (pChar == 0) OGDF_THROW(InsufficientMemoryException);

			ogdf::strcpy(pChar,len,m_stringSymbol);
			object = OGDF_NEW GmlObject(key,pChar); }
			break;

		case gmlListBegin:
			object = OGDF_NEW GmlObject(key);
			object->m_pFirstSon = parseList(gmlListEnd,gmlEOF);
			break;

		case gmlListEnd:
			setError("unexpected end of list");
			return firstSon;

		case gmlKey:
			setError("unexpected key");
			return firstSon;

		case gmlEOF:
			setError("missing value");
			return firstSon;

		case gmlError:
			return firstSon;

		OGDF_NODEFAULT // one of the cases above has to occur
		}

		*pPrev = object;
		pPrev = &object->m_pBrother;
	}

	return firstSon;
}


void GmlParser::destroyObjectList(GmlObject *object)
{
	GmlObject *nextObject;
	for(; object; object = nextObject) {
		nextObject = object->m_pBrother;

		if (object->m_valueType == gmlStringValue)
			delete[] const_cast<char *>(object->m_stringValue);

		else if (object->m_valueType == gmlListBegin)
			destroyObjectList(object->m_pFirstSon);

		delete object;
	}
}


GmlParser::~GmlParser()
{
	// we have to delete all objects and allocated char arrays in string values
	destroyObjectList(m_objectTree);
}


bool GmlParser::getLine()
{
	do {
		if (m_is->eof()) return false;
		(*m_is) >> std::ws;  // skip whitespace like spaces for indentation
		m_is->getline(m_lineBuffer,255);
		if (m_is->fail())
			return false;
		for(m_pCurrent = m_lineBuffer;
			*m_pCurrent && isspace(*m_pCurrent); ++m_pCurrent) ;
	} while (*m_pCurrent == '#' || *m_pCurrent == 0);

	return true;
}


GmlObjectType GmlParser::getNextSymbol()
{
	*m_pStore = m_cStore;

	// eat whitespace
	for(; *m_pCurrent && isspace(*m_pCurrent); ++m_pCurrent) ;

	// get new line if required
	if (*m_pCurrent == 0) {
		if (!getLine()) return gmlEOF;
	}

	// identify start of current symbol
	char *pStart = m_pCurrent;

	// we currently do not support strings with line breaks!
	if (*pStart == '\"')
	{ // string
		m_stringSymbol = ++m_pCurrent;
		char *pWrite = m_pCurrent;
		while(*m_pCurrent != 0 && *m_pCurrent != '\"')
		{
			if (*m_pCurrent == '\\')
			{
				// note: this block is repeated below
				switch(*(m_pCurrent+1)) {
				case 0:
					*m_pCurrent = 0;
					break;
				case '\\':
					*pWrite++ = '\\';
					m_pCurrent += 2;
					break;
				case '\"':
					*pWrite++ = '\"';
					m_pCurrent += 2;
					break;
				default:
					// just copy the escape sequence as is
					*pWrite++ = *m_pCurrent++;
					*pWrite++ = *m_pCurrent++;
				}

			} else
				*pWrite++ = *m_pCurrent++;
		}

		if (*m_pCurrent == 0)
		{
			*pWrite = 0;
			m_longString = (pStart+1);
			while(getLine())
			{
				m_pCurrent = pWrite = m_lineBuffer;
				while(*m_pCurrent != 0 && *m_pCurrent != '\"')
				{
					if (*m_pCurrent == '\\')
					{
						// (block repeated from above)
						switch(*(m_pCurrent+1)) {
						case 0:
							*m_pCurrent = 0;
							break;
						case '\\':
							*pWrite++ = '\\';
							m_pCurrent += 2;
							break;
						case '\"':
							*pWrite++ = '\"';
							m_pCurrent += 2;
							break;
						default:
							// just copy the escape sequence as is
							*pWrite++ = *m_pCurrent++;
							*pWrite++ = *m_pCurrent++;
						}

					} else
						*pWrite++ = *m_pCurrent++;
				}

				if (*m_pCurrent == 0) {
					*pWrite = 0;
					m_longString += m_lineBuffer;

				} else {
					m_cStore = *(m_pStore = m_pCurrent);
					++m_pCurrent;
					*pWrite = 0;
					m_longString += m_lineBuffer;
					break;
				}
			}
			m_stringSymbol = m_longString.cstr();

		} else {
			m_cStore = *(m_pStore = m_pCurrent);
			++m_pCurrent;
			*pWrite = 0;
		}

		return gmlStringValue;
	}

	// identify end of current symbol
	while(*m_pCurrent != 0 && !isspace(*m_pCurrent)) ++m_pCurrent;

	m_cStore = *(m_pStore = m_pCurrent);
	*m_pCurrent = 0;

	if(isalpha(*pStart)) { // key

		// check if really a correct key (error if not)
		if (m_doCheck) {
			for (char *p = pStart+1; *p; ++p)
				if (!(isalpha(*p) || isdigit(*p))) {
					setError("malformed key");
					return gmlError;
				}
		}

		m_keySymbol = hashString(pStart);
		return gmlKey;

	} else if (*pStart == '[') {
		return gmlListBegin;

	} else if (*pStart == ']') {
		return gmlListEnd;

	} else if (*pStart == '-' || isdigit(*pStart)) { // int or double
		char *p = pStart+1;
		while(isdigit(*p)) ++p;

		if (*p == '.') { // double
			// check to be done

			sscanf(pStart,"%lf",&m_doubleSymbol);
			return gmlDoubleValue;

		} else { // int
			if (*p != 0) {
				setError("malformed number");
				return gmlError;
			}

			sscanf(pStart,"%d",&m_intSymbol);
			return gmlIntValue;
		}
	}

	setError("unknown symbol");

	return gmlError;
}


GmlKey GmlParser::hashString(const String &str)
{
	GmlKey key = m_hashTable.insertByNeed(str,-1);
	if(key->info() == -1) key->info() = m_num++;

	return key;
}


GmlObject *GmlParser::getNodeIdRange(int &minId,int &maxId)
{
	minId = maxId = 0;

	GmlObject *graphObject = m_objectTree;
	for(; graphObject; graphObject = graphObject->m_pBrother)
		if (id(graphObject) == graphPredefKey) break;

	if (!graphObject || graphObject->m_valueType != gmlListBegin) return 0;

	bool first = true;
	GmlObject *son = graphObject->m_pFirstSon;
	for(; son; son = son->m_pBrother) {
		if (id(son) == nodePredefKey && son->m_valueType == gmlListBegin) {

			GmlObject *nodeSon = son->m_pFirstSon;
			for(; nodeSon; nodeSon = nodeSon->m_pBrother) {
				if (id(nodeSon) == idPredefKey ||
					nodeSon->m_valueType == gmlIntValue)
				{
					int nodeSonId = nodeSon->m_intValue;
					if (first) {
						minId = maxId = nodeSonId;
						first = false;
					} else {
						if (nodeSonId < minId) minId = nodeSonId;
						if (nodeSonId > maxId) maxId = nodeSonId;
					}
				}
			}
		}
	}

	return graphObject;
}


bool GmlParser::read(Graph &G)
{
	G.clear();

	int minId = m_mapToNode.low();
	int maxId = m_mapToNode.high();
	int notDefined = minId-1; //indicates not defined id key

	GmlObject *son = m_graphObject->m_pFirstSon;
	for(; son; son = son->m_pBrother)
	{
		switch(id(son))
		{
		case nodePredefKey: {
			if (son->m_valueType != gmlListBegin) break;

			// set attributes to default values
			int vId = notDefined;

			// read all relevant attributes
			GmlObject *nodeSon = son->m_pFirstSon;
			for(; nodeSon; nodeSon = nodeSon->m_pBrother) {
				if (id(nodeSon) == idPredefKey &&
					nodeSon->m_valueType == gmlIntValue)
				{
					vId = nodeSon->m_intValue;
				}
			}

			// check if everything required is defined correctly
			if (vId == notDefined) {
				setError("node id not defined");
				return false;
			}

			// create new node if necessary
			if (m_mapToNode[vId] == 0) m_mapToNode[vId] = G.newNode(); }
			break;

		case edgePredefKey: {
			if (son->m_valueType != gmlListBegin) break;

			// set attributes to default values
			int sourceId = notDefined, targetId = notDefined;

			// read all relevant attributes
			GmlObject *edgeSon = son->m_pFirstSon;
			for(; edgeSon; edgeSon = edgeSon->m_pBrother) {

				switch(id(edgeSon)) {
				case sourcePredefKey:
					if (edgeSon->m_valueType != gmlIntValue) break;
					sourceId = edgeSon->m_intValue;
					break;

				case targetPredefKey:
					if (edgeSon->m_valueType != gmlIntValue) break;
					targetId = edgeSon->m_intValue;
					break;
				}
			}

			// check if everything required is defined correctly
			if (sourceId == notDefined || targetId == notDefined) {
				setError("source or target id not defined");
				return false;

			} else if (sourceId < minId || maxId < sourceId ||
				targetId < minId || maxId < targetId) {
				setError("source or target id out of range");
				return false;
			}

			// create adjacent nodes if necessary and new edge
			if (m_mapToNode[sourceId] == 0) m_mapToNode[sourceId] = G.newNode();
			if (m_mapToNode[targetId] == 0) m_mapToNode[targetId] = G.newNode();

			G.newEdge(m_mapToNode[sourceId],m_mapToNode[targetId]);
			}//case edge
			break;
		}//switch
	}//for sons

	return true;
}


bool GmlParser::read(Graph &G, GraphAttributes &AG)
{
	OGDF_ASSERT(&G == &(AG.constGraph()))

		G.clear();

	int minId = m_mapToNode.low();
	int maxId = m_mapToNode.high();
	int notDefined = minId-1; //indicates not defined id key

	DPolyline bends;

	GmlObject *son = m_graphObject->m_pFirstSon;
	for(; son; son = son->m_pBrother) {

		switch(id(son)) {
		case nodePredefKey: {
			if (son->m_valueType != gmlListBegin) break;

			// set attributes to default values
			int vId = notDefined;
			double x = 0, y = 0, w = 0, h = 0;
			String label;
			String templ;
			String fill;  // the fill color attribute
			String line;  // the line color attribute
			String shape; //the shape type
			double lineWidth = 1.0; //node line width
			int    pattern = 1; //node brush pattern
			int    stipple = 1; //line style pattern

			// read all relevant attributes
			GmlObject *nodeSon = son->m_pFirstSon;
			for(; nodeSon; nodeSon = nodeSon->m_pBrother) {
				switch(id(nodeSon)) {
				case idPredefKey:
					if(nodeSon->m_valueType != gmlIntValue) break;
					vId = nodeSon->m_intValue;
					break;

				case graphicsPredefKey: {
					if (nodeSon->m_valueType != gmlListBegin) break;

					GmlObject *graphicsObject = nodeSon->m_pFirstSon;
					for(; graphicsObject;
						graphicsObject = graphicsObject->m_pBrother)
					{
						switch(id(graphicsObject)) {
						case xPredefKey:
							if(graphicsObject->m_valueType != gmlDoubleValue) break;
							x = graphicsObject->m_doubleValue;
							break;

						case yPredefKey:
							if(graphicsObject->m_valueType != gmlDoubleValue) break;
							y = graphicsObject->m_doubleValue;
							break;

						case wPredefKey:
							if(graphicsObject->m_valueType != gmlDoubleValue) break;
							w = graphicsObject->m_doubleValue;
							break;

						case hPredefKey:
							if(graphicsObject->m_valueType != gmlDoubleValue) break;
							h = graphicsObject->m_doubleValue;
							break;

						case fillPredefKey:
							if(graphicsObject->m_valueType != gmlStringValue) break;
							fill = graphicsObject->m_stringValue;
							break;

						case linePredefKey:
							if(graphicsObject->m_valueType != gmlStringValue) break;
							line = graphicsObject->m_stringValue;
							break;

						case lineWidthPredefKey:
							if(graphicsObject->m_valueType != gmlDoubleValue) break;
							lineWidth = graphicsObject->m_doubleValue;
							break;

						case typePredefKey:
							if(graphicsObject->m_valueType != gmlStringValue) break;
							shape = graphicsObject->m_stringValue;
							break;
						case patternPredefKey: //fill style
							if(graphicsObject->m_valueType != gmlIntValue) break;
							pattern = graphicsObject->m_intValue;
						case stipplePredefKey: //line style
							if(graphicsObject->m_valueType != gmlIntValue) break;
							stipple = graphicsObject->m_intValue;
						}
					}
					break; }

				case templatePredefKey:
					if (nodeSon->m_valueType != gmlStringValue) break;

					templ = nodeSon->m_stringValue;
					break;

				case labelPredefKey:
					if (nodeSon->m_valueType != gmlStringValue) break;

					label = nodeSon->m_stringValue;
					break;
				}
			}

			// check if everything required is defined correctly
			if (vId == notDefined) {
				setError("node id not defined");
				return false;
			}

			// create new node if necessary and assign attributes
			if (m_mapToNode[vId] == 0) m_mapToNode[vId] = G.newNode();
			if (AG.attributes() & GraphAttributes::nodeGraphics)
			{
				AG.x(m_mapToNode[vId]) = x;
				AG.y(m_mapToNode[vId]) = y;
				AG.width (m_mapToNode[vId]) = w;
				AG.height(m_mapToNode[vId]) = h;
				if (shape == "oval")
					AG.shapeNode(m_mapToNode[vId]) = GraphAttributes::oval;
				else AG.shapeNode(m_mapToNode[vId]) = GraphAttributes::rectangle;
			}
			if ( (AG.attributes() & GraphAttributes::nodeColor) &&
				(AG.attributes() & GraphAttributes::nodeGraphics) )
			{
				AG.colorNode(m_mapToNode[vId]) = fill;
				AG.nodeLine(m_mapToNode[vId]) = line;
			}
			if (AG.attributes() & GraphAttributes::nodeLabel)
				AG.labelNode(m_mapToNode[vId]) = label;
			if (AG.attributes() & GraphAttributes::nodeTemplate)
				AG.templateNode(m_mapToNode[vId]) = templ;
			if (AG.attributes() & GraphAttributes::nodeId)
				AG.idNode(m_mapToNode[vId]) = vId;
			if (AG.attributes() & GraphAttributes::nodeStyle)
			{
				AG.nodePattern(m_mapToNode[vId]) =
					GraphAttributes::intToPattern(pattern);
				AG.styleNode(m_mapToNode[vId]) =
					GraphAttributes::intToStyle(stipple);
				AG.lineWidthNode(m_mapToNode[vId]) =
					lineWidth;
			}
							}//node
							//Todo: line style set stipple value
							break;

		case edgePredefKey: {
			String arrow; // the arrow type attribute
			String fill;  //the color fill attribute
			int stipple = 1;  //the line style
			double lineWidth = 1.0;
			double edgeWeight = 1.0;
			int subGraph = 0; //edgeSubGraph attribute
			String label; // label attribute

			if (son->m_valueType != gmlListBegin) break;

			// set attributes to default values
			int sourceId = notDefined, targetId = notDefined;
			Graph::EdgeType umlType = Graph::association;

			// read all relevant attributes
			GmlObject *edgeSon = son->m_pFirstSon;
			for(; edgeSon; edgeSon = edgeSon->m_pBrother) {

				switch(id(edgeSon)) {
				case sourcePredefKey:
					if (edgeSon->m_valueType != gmlIntValue) break;
					sourceId = edgeSon->m_intValue;
					break;

				case targetPredefKey:
					if (edgeSon->m_valueType != gmlIntValue) break;
					targetId = edgeSon->m_intValue;
					break;

				case subGraphPredefKey:
					if (edgeSon->m_valueType != gmlIntValue) break;
					subGraph = edgeSon->m_intValue;
					break;

				case labelPredefKey:
					if (edgeSon->m_valueType != gmlStringValue) break;
					label = edgeSon->m_stringValue;
					break;

				case graphicsPredefKey: {
					if (edgeSon->m_valueType != gmlListBegin) break;

					GmlObject *graphicsObject = edgeSon->m_pFirstSon;
					for(; graphicsObject;
						graphicsObject = graphicsObject->m_pBrother)
					{
						if(id(graphicsObject) == LinePredefKey &&
							graphicsObject->m_valueType == gmlListBegin)
						{
							readLineAttribute(graphicsObject->m_pFirstSon,bends);
						}
						if(id(graphicsObject) == arrowPredefKey &&
							graphicsObject->m_valueType == gmlStringValue)
							arrow = graphicsObject->m_stringValue;
						if(id(graphicsObject) == fillPredefKey &&
							graphicsObject->m_valueType == gmlStringValue)
							fill = graphicsObject->m_stringValue;
						if (id(graphicsObject) == stipplePredefKey && //line style
							graphicsObject->m_valueType == gmlIntValue)
							stipple = graphicsObject->m_intValue;
						if (id(graphicsObject) == lineWidthPredefKey && //line width
							graphicsObject->m_valueType == gmlDoubleValue)
							lineWidth = graphicsObject->m_doubleValue;
						if (id(graphicsObject) == edgeWeightPredefKey &&
							graphicsObject->m_valueType == gmlDoubleValue)
							edgeWeight = graphicsObject->m_doubleValue;
					}//for graphics
										}

				case generalizationPredefKey:
					if (edgeSon->m_valueType != gmlIntValue) break;
					umlType = (edgeSon->m_intValue == 0) ?
						Graph::association : Graph::generalization;
					break;

				}
			}

			// check if everything required is defined correctly
			if (sourceId == notDefined || targetId == notDefined) {
				setError("source or target id not defined");
				return false;

			} else if (sourceId < minId || maxId < sourceId ||
				targetId < minId || maxId < targetId) {
					setError("source or target id out of range");
					return false;
			}

			// create adjacent nodes if necessary and new edge
			if (m_mapToNode[sourceId] == 0) m_mapToNode[sourceId] = G.newNode();
			if (m_mapToNode[targetId] == 0) m_mapToNode[targetId] = G.newNode();

			edge e = G.newEdge(m_mapToNode[sourceId],m_mapToNode[targetId]);
			if (AG.attributes() & GraphAttributes::edgeGraphics)
				AG.bends(e).conc(bends);
			if (AG.attributes() & GraphAttributes::edgeType)
				AG.type(e) = umlType;
			if(AG.attributes() & GraphAttributes::edgeSubGraph)
				AG.subGraphBits(e) = subGraph;
			if (AG.attributes() & GraphAttributes::edgeLabel)
				AG.labelEdge(e) = label;

			if (AG.attributes() & GraphAttributes::edgeArrow) {
				if (arrow == "none")
					AG.arrowEdge(e) = GraphAttributes::none;
				else if (arrow == "last")
					AG.arrowEdge(e) = GraphAttributes::last;
				else if (arrow == "first")
					AG.arrowEdge(e) = GraphAttributes::first;
				else if (arrow == "both")
					AG.arrowEdge(e) = GraphAttributes::both;
				else
					AG.arrowEdge(e) = GraphAttributes::undefined;
			}

			if (AG.attributes() & GraphAttributes::edgeColor)
				AG.colorEdge(e) = fill;
			if (AG.attributes() & GraphAttributes::edgeStyle)
			{
				AG.styleEdge(e) = AG.intToStyle(stipple);
				AG.edgeWidth(e) = lineWidth;
			}

			if (AG.attributes() & GraphAttributes::edgeDoubleWeight)
				AG.doubleWeight(e) = edgeWeight;


			break; }
		case directedPredefKey: {
			if(son->m_valueType != gmlIntValue) break;
			AG.directed(son->m_intValue > 0);
			break; }
		}
	}

	return true;
}//read


//to be called AFTER calling read(G, AG)
bool GmlParser::readAttributedCluster(
	Graph &G,
	ClusterGraph& CG,
	ClusterGraphAttributes& ACG)
{
	OGDF_ASSERT(&CG.getGraph() == &G)


	//now we need the cluster object
	GmlObject *rootObject = m_objectTree;
	for(; rootObject; rootObject = rootObject->m_pBrother)
		if (id(rootObject) == rootClusterPredefKey) break;

	if(rootObject == 0)
		return true;

	if (id(rootObject) != rootClusterPredefKey)
	{
		setError("missing rootcluster key");
		return false;
	}

	if (rootObject->m_valueType != gmlListBegin) return false;

	attributedClusterRead(rootObject, CG, ACG);

	return true;
}//readAttributedCluster


//the clustergraph has to be initialized on G!!,
//no clusters other then root cluster may exist, which holds all nodes
bool GmlParser::readCluster(Graph &G, ClusterGraph& CG)
{
	OGDF_ASSERT(&CG.getGraph() == &G)

	//now we need the cluster object
	GmlObject *rootObject = m_objectTree;
	for(; rootObject; rootObject = rootObject->m_pBrother)
		if (id(rootObject) == rootClusterPredefKey) break;

	//we have to check if the file does really contain clusters
	//otherwise, rootcluster will suffice
	if (rootObject == 0) return true;
	if (id(rootObject) != rootClusterPredefKey)
	{
		setError("missing rootcluster key");
		return false;
	}

	if (rootObject->m_valueType != gmlListBegin) return false;

	clusterRead(rootObject, CG);

	return true;
}//read clustergraph


//read all cluster tree information
bool GmlParser::clusterRead(
	GmlObject* rootCluster,
	ClusterGraph& CG)
{

	//the root cluster is only allowed to hold child clusters and
	//nodes in a list

	if (rootCluster->m_valueType != gmlListBegin) return false;

	// read all clusters and nodes
	GmlObject *rootClusterSon = rootCluster->m_pFirstSon;

	for(; rootClusterSon; rootClusterSon = rootClusterSon->m_pBrother)
	{
		switch(id(rootClusterSon))
		{
		case clusterPredefKey:
			{
				//we could delete this, but we aviod the call
				if (rootClusterSon->m_valueType != gmlListBegin) return false;
				// set attributes to default values
				//we currently do not set any values
				cluster c = CG.newCluster(CG.rootCluster());

				//recursively read cluster
				recursiveClusterRead(rootClusterSon, CG, c);

			} //case cluster
			break;
		case vertexPredefKey: //direct root vertices
			{
				if (rootClusterSon->m_valueType != gmlStringValue) return false;
				String vIDString = rootClusterSon->m_stringValue;

				//we only allow a vertex id as string identification
				if ((vIDString[0] != 'v') &&
					(!isdigit(vIDString[0])))return false; //do not allow labels
				//if old style entry "v"i
				if (!isdigit(vIDString[0])) //should check prefix?
					vIDString[0] = '0'; //leading zero to allow conversion
				int vID = atoi(vIDString.cstr());

				OGDF_ASSERT(m_mapToNode[vID] != 0)

					//we assume that no node is already assigned ! Changed:
					//all new nodes are assigned to root
					//CG.reassignNode(mapToNode[vID], CG.rootCluster());
					//it seems that this may be unnessecary, TODO check
					CG.reassignNode(m_mapToNode[vID], CG.rootCluster());
				//char* vIDChar = new char[vIDString.length()+1];
				//for (int ind = 1; ind < vIDString.length(); ind++)
				//	vIDChar

			}//case vertex
		}//switch
	}//for all rootcluster sons

	return true;

}//clusterread


//the same for attributed graphs
//read all cluster tree information
//make changes to this as well as the recursive function
bool GmlParser::attributedClusterRead(
	GmlObject* rootCluster,
	ClusterGraph& CG,
	ClusterGraphAttributes& ACG)
{

	//the root cluster is only allowed to hold child clusters and
	//nodes in a list

	if (rootCluster->m_valueType != gmlListBegin) return false;

	// read all clusters and nodes
	GmlObject *rootClusterSon = rootCluster->m_pFirstSon;

	for(; rootClusterSon; rootClusterSon = rootClusterSon->m_pBrother)
	{
		switch(id(rootClusterSon))
		{
		case clusterPredefKey:
			{
				//we could delete this, but we avoid the call
				if (rootClusterSon->m_valueType != gmlListBegin) return false;
				// set attributes to default values
				//we currently do not set any values
				cluster c = CG.newCluster(CG.rootCluster());

				//recursively read cluster
				recursiveAttributedClusterRead(rootClusterSon, CG, ACG, c);

			} //case cluster
			break;

		case vertexPredefKey: //direct root vertices
			{
				if (rootClusterSon->m_valueType != gmlStringValue) return false;
				String vIDString = rootClusterSon->m_stringValue;

				//we only allow a vertex id as string identification
				if ((vIDString[0] != 'v') &&
					(!isdigit(vIDString[0])))return false; //do not allow labels
				//if old style entry "v"i
				if (!isdigit(vIDString[0])) //should check prefix?
					vIDString[0] = '0'; //leading zero to allow conversion
				int vID = atoi(vIDString.cstr());

				OGDF_ASSERT(m_mapToNode[vID] != 0)

					//we assume that no node is already assigned
					//CG.reassignNode(mapToNode[vID], CG.rootCluster());
					//changed: all nodes are already assigned to root
					//this code seems to be obsolete, todo: check
					CG.reassignNode(m_mapToNode[vID], CG.rootCluster());
				//char* vIDChar = new char[vIDString.length()+1];
				//for (int ind = 1; ind < vIDString.length(); ind++)
				//	vIDChar

			}//case vertex
		}//switch
	}//for all rootcluster sons

	return true;

}//attributedclusterread


bool GmlParser::readClusterAttributes(
	GmlObject* cGraphics,
	cluster c,
	ClusterGraphAttributes& ACG)
{
	String label;
	String fill;  // the fill color attribute
	String line;  // the line color attribute
	double lineWidth = 1.0; //node line width
	int    pattern = 1; //node brush pattern
	int    stipple = 1; //line style pattern

	// read all relevant attributes
	GmlObject *graphicsObject = cGraphics->m_pFirstSon;
	for(; graphicsObject; graphicsObject = graphicsObject->m_pBrother)
	{
		switch(id(graphicsObject))
		{
		case xPredefKey:
			if(graphicsObject->m_valueType != gmlDoubleValue) return false;
			ACG.clusterXPos(c) = graphicsObject->m_doubleValue;
			break;

		case yPredefKey:
			if(graphicsObject->m_valueType != gmlDoubleValue) return false;
			ACG.clusterYPos(c) = graphicsObject->m_doubleValue;
			break;

		case widthPredefKey:
			if(graphicsObject->m_valueType != gmlDoubleValue) return false;
			ACG.clusterWidth(c) = graphicsObject->m_doubleValue;
			break;

		case heightPredefKey:
			if(graphicsObject->m_valueType != gmlDoubleValue) return false;
			ACG.clusterHeight(c) = graphicsObject->m_doubleValue;
			break;
		case fillPredefKey:
			if(graphicsObject->m_valueType != gmlStringValue) return false;
			ACG.clusterFillColor(c) = graphicsObject->m_stringValue;
			break;
		case patternPredefKey:
			if(graphicsObject->m_valueType != gmlIntValue) return false;
			pattern = graphicsObject->m_intValue;
			break;
			//line style
		case colorPredefKey: // line color
			if(graphicsObject->m_valueType != gmlStringValue) return false;
			ACG.clusterColor(c) = graphicsObject->m_stringValue;
			break;

		case stipplePredefKey:
			if(graphicsObject->m_valueType != gmlIntValue) return false;
			stipple = graphicsObject->m_intValue;
			break;
		case lineWidthPredefKey:
			if(graphicsObject->m_valueType != gmlDoubleValue) return false;
			lineWidth =
				graphicsObject->m_doubleValue;
			break;
			//TODO: backgroundcolor
			//case stylePredefKey:
			//case boderwidthPredefKey:
		}//switch
	}//for

	//Hier eigentlich erst abfragen, ob clusterattributes setzbar in ACG,
	//dann setzen
	ACG.clusterLineStyle(c) = GraphAttributes::intToStyle(stipple); //defaulting 1
	ACG.clusterLineWidth(c) = lineWidth;
	ACG.clusterFillPattern(c) = GraphAttributes::intToPattern(pattern);

	return true;
}//readclusterattributes

//recursively read cluster subtree information
bool GmlParser::recursiveClusterRead(GmlObject* clusterObject,
								ClusterGraph& CG,
								cluster c)
{

	//for direct root cluster sons, this is checked twice...
	if (clusterObject->m_valueType != gmlListBegin) return false;

	GmlObject *clusterSon = clusterObject->m_pFirstSon;

	for(; clusterSon; clusterSon = clusterSon->m_pBrother)
	{
		//we dont read the attributes, therefore look only for
		//id and sons
		switch(id(clusterSon))
		{
			case clusterPredefKey:
				{
					if (clusterSon->m_valueType != gmlListBegin) return false;

					cluster cson = CG.newCluster(c);
					//recursively read child cluster
					recursiveClusterRead(clusterSon, CG, cson);
				}
				break;
			case vertexPredefKey: //direct cluster vertex entries
				{
					if (clusterSon->m_valueType != gmlStringValue) return false;
					String vIDString = clusterSon->m_stringValue;

					//if old style entry "v"i
					if ((vIDString[0] != 'v') &&
						(!isdigit(vIDString[0])))return false; //do not allow labels
					//if old style entry "v"i
					if (!isdigit(vIDString[0])) //should check prefix?
						vIDString[0] = '0'; //leading zero to allow conversion
					int vID = atoi(vIDString.cstr());

					OGDF_ASSERT(m_mapToNode[vID] != 0)

					//we assume that no node is already assigned
					//CG.reassignNode(mapToNode[vID], c);
					//changed: all nodes are already assigned to root
					CG.reassignNode(m_mapToNode[vID], c);
					//char* vIDChar = new char[vIDString.length()+1];
					//for (int ind = 1; ind < vIDString.length(); ind++)
					//	vIDChar

				}//case vertex
		}//switch
	}//for clustersons

	return true;

}//recursiveclusterread

//recursively read cluster subtree information
bool GmlParser::recursiveAttributedClusterRead(GmlObject* clusterObject,
								ClusterGraph& CG,
								ClusterGraphAttributes& ACG,
								cluster c)
{

	//for direct root cluster sons, this is checked twice...
	if (clusterObject->m_valueType != gmlListBegin) return false;

	GmlObject *clusterSon = clusterObject->m_pFirstSon;

	for(; clusterSon; clusterSon = clusterSon->m_pBrother)
	{
		//we dont read the attributes, therefore look only for
		//id and sons
		switch(id(clusterSon))
		{
			case clusterPredefKey:
				{
					if (clusterSon->m_valueType != gmlListBegin) return false;

					cluster cson = CG.newCluster(c);
					//recursively read child cluster
					recursiveAttributedClusterRead(clusterSon, CG, ACG, cson);
				}
				break;
			case labelPredefKey:
				{
					if (clusterSon->m_valueType != gmlStringValue) return false;
					ACG.clusterLabel(c) = clusterSon->m_stringValue;
				}
				break;
			case templatePredefKey:
				{
					if (clusterSon->m_valueType != gmlStringValue) return false;
					ACG.templateCluster(c) = clusterSon->m_stringValue;
					break;
				}
			case graphicsPredefKey: //read the info for cluster c
				{
					if (clusterSon->m_valueType != gmlListBegin) return false;

					readClusterAttributes(clusterSon, c , ACG);
				}//graphics
				break;
			case vertexPredefKey: //direct cluster vertex entries
				{
					if (clusterSon->m_valueType != gmlStringValue) return false;
					String vIDString = clusterSon->m_stringValue;

					if ((vIDString[0] != 'v') &&
						(!isdigit(vIDString[0])))return false; //do not allow labels
					//if old style entry "v"i
					if (!isdigit(vIDString[0])) //should check prefix?
						vIDString[0] = '0'; //leading zero to allow conversion
					int vID = atoi(vIDString.cstr());

					OGDF_ASSERT(m_mapToNode[vID] != 0)

					//we assume that no node is already assigned
					//changed: all nodes are already assigned to root
					CG.reassignNode(m_mapToNode[vID], c);

				}//case vertex
		}//switch
	}//for clustersons

	return true;
}//recursiveAttributedClusterRead

void GmlParser::readLineAttribute(GmlObject *object, DPolyline &dpl)
{
	dpl.clear();
	for(; object; object = object->m_pBrother) {
		if (id(object) == pointPredefKey &&
			object->m_valueType == gmlListBegin)
		{
			DPoint dp;

			GmlObject *pointObject = object->m_pFirstSon;
			for (; pointObject; pointObject = pointObject->m_pBrother) {
				if (pointObject->m_valueType != gmlDoubleValue) continue;
				if (id(pointObject) == xPredefKey)
					dp.m_x = pointObject->m_doubleValue;
				else if (id(pointObject) == yPredefKey)
					dp.m_y = pointObject->m_doubleValue;
			}

			dpl.pushBack(dp);
		}
	}
}


void GmlParser::setError(const char *errorString)
{
	m_error = true;
	m_errorString = errorString;
}


void GmlParser::indent(ostream &os, int d)
{
	for(int i = 1; i <= d; ++i)
		os << " ";
}

void GmlParser::output(ostream &os, GmlObject *object, int d)
{
	for(; object; object = object->m_pBrother) {
		indent(os,d); os << object->m_key->key();

		switch(object->m_valueType) {
		case gmlIntValue:
			os << " " << object->m_intValue << "\n";
			break;

		case gmlDoubleValue:
			os << " " << object->m_doubleValue << "\n";
			break;

		case gmlStringValue:
			os << " \"" << object->m_stringValue << "\"\n";
			break;

		case gmlListBegin:
			os << "\n";
			output(os, object->m_pFirstSon, d+2);
			break;
		case gmlListEnd:
			break;
		case gmlKey:
			break;
		case gmlEOF:
			break;
		case gmlError:
			break;
		}
	}
}


} // end namespace ogdf
