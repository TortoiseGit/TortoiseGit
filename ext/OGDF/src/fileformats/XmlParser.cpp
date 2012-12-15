/*
 * $Revision: 2565 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-07 17:14:54 +0200 (Sa, 07. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Implementation of XML parser (class XmlParser)
 * (used for parsing and reading XML files)
 *
 * \author Sebastian Leipert
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


#include <ogdf/fileformats/XmlParser.h>
#include <ctype.h>
#include <string.h>

extern ofstream os;

namespace ogdf {


#define BUFFERLENGTH 8192

XmlParser::XmlParser(const char *fileName, bool doCheck)
{
	// open file
	ifstream is(fileName, ios::in);// | ios::nocreate); // not accepted by gnu 3.02
	if (!is) {
		setError("Cannot open file."); return;
	}

	createObjectTree(is, doCheck);
}


XmlParser::XmlParser(istream &is, bool doCheck)
{
	createObjectTree(is,doCheck);
}


void XmlParser::createObjectTree(istream &is, bool doCheck)
{
	initPredefinedKeys();
	m_error = false;
	m_objectTree = 0;

	m_is = &is;
	m_doCheck = doCheck; // indicates more extensive checking

	// initialize line buffer (note: XML specifies a maximal line length
	// of 254 characters!)
	// See als the  workaround for Get2Chip in function getLine()

	m_rLineBuffer = new char[BUFFERLENGTH]; // get2Chip special:
											// XML Standard: char[256];
	*m_rLineBuffer = '\n';
	m_lineBuffer = m_rLineBuffer+1;

	m_pCurrent = m_pStore = m_lineBuffer;
	m_cStore = 0; // forces getNextSymbol() to read first line
	m_keyName = 0;

	// create object tree
	m_objectTree = parseList(xmlEOF,xmlListEnd,"");

	delete[] m_rLineBuffer;
}


// we use predefined id constants for all relevant keys
// this allows us to use efficient switch() statemnts in read() methods
void XmlParser::initPredefinedKeys()
{

	m_hashTable.fastInsert("NAME",      namePredefKey);
	m_hashTable.fastInsert("GRAPH",     graphPredefKey);
	m_hashTable.fastInsert("NODE",      nodePredefKey);
	m_hashTable.fastInsert("TRANSITION",edgePredefKey);
	m_hashTable.fastInsert("EDGE",		edgePredefKey);

	m_hashTable.fastInsert("POSITION",	positionPredefKey);
	m_hashTable.fastInsert("X",         xPredefKey);
	m_hashTable.fastInsert("Y",         yPredefKey);

	m_hashTable.fastInsert("SIZE",		sizePredefKey);
	m_hashTable.fastInsert("W",         wPredefKey);
	m_hashTable.fastInsert("H",         hPredefKey);
	m_hashTable.fastInsert("WIDTH",     widthPredefKey);
	m_hashTable.fastInsert("HEIGHT",    heightPredefKey);

	m_hashTable.fastInsert("NODETYPE",  nodetypePredefKey);
	m_hashTable.fastInsert("EDGETYPE",  edgetypePredefKey);
	m_hashTable.fastInsert("TYPE",      typePredefKey);

	m_hashTable.fastInsert("FROM",      sourcePredefKey);
	m_hashTable.fastInsert("SOURCE",	sourcePredefKey);
	m_hashTable.fastInsert("TO",        targetPredefKey);
	m_hashTable.fastInsert("TARGET",	targetPredefKey);
	m_hashTable.fastInsert("SENSE",     sensePredefKey);
	m_hashTable.fastInsert("PATH",      pathPredefKey);


	// further keys get id's starting with NEXTPREDEFKEY
	m_num = NEXTPREDEFKEY;
}


XmlObject *XmlParser::parseList(XmlObjectType closingKey,
								XmlObjectType /* errorKey */,
								const char *objectBodyName)
{
	XmlObject *firstSon = 0;
	XmlObject **pPrev = &firstSon;

	for( ; ; ) {
		XmlObjectType symbol = getNextSymbol();

		if (symbol == closingKey || symbol == xmlError)
			return firstSon;

		XmlObject *object = 0;

		if (symbol == xmlListBegin) {
			symbol = getNextSymbol();
			if (symbol != xmlKey) {
				setError("key expected");
				return firstSon;
			}
			XmlKey key = m_keySymbol;
			object = OGDF_NEW XmlObject(key);

			size_t len = strlen(m_keyName)+1;
			char* newObjectBodyName = new char[len];
			m_objectBody.pushBack(newObjectBodyName);
			ogdf::strcpy(newObjectBodyName,len,m_keyName);

			// Recursive call for building the tree.
			object->m_pFirstSon = parseList(xmlListEnd,xmlEOF,newObjectBodyName);
		}

		else if (m_eoTag)
		{ // must be the body of an element
			if (symbol != xmlStringValue) {
				setError("String expected");
				return firstSon;
			}
			size_t len = strlen(m_stringSymbol)+1;
			char *pChar = new char[len];
			ogdf::strcpy(pChar,len,m_stringSymbol);

			object = OGDF_NEW XmlObject(hashString(objectBodyName),pChar);
		}
		else
		{ // must be a symbol
			if (symbol != xmlKey) {
				setError("key expected");
				return firstSon;
			}

			XmlKey key = m_keySymbol;

			symbol = getNextSymbol();
			switch (symbol) {
			case xmlIntValue:
				object = OGDF_NEW XmlObject(key,m_intSymbol);
				break;

			case xmlDoubleValue:
				object = OGDF_NEW XmlObject(key,m_doubleSymbol);
				break;

			case xmlStringValue: {
				size_t len = strlen(m_stringSymbol)+1;
				char *pChar = new char[len];
				ogdf::strcpy(pChar,len,m_stringSymbol);
				object = OGDF_NEW XmlObject(key,pChar); }
				break;

			case xmlListBegin:
				setError("unexpected begin of list");
				break;

			case xmlListEnd:
				setError("unexpected end of list");
				return firstSon;

			case xmlKey:
				setError("unexpected key");
				return firstSon;

			case xmlEOF:
				setError("missing value");
				return firstSon;

			case xmlError:
				return firstSon;
			}
		}

		*pPrev = object;
		pPrev = &object->m_pBrother;

	}
	return firstSon;
}


void XmlParser::destroyObjectList(XmlObject *object)
{
	XmlObject *nextObject;
	for(; object; object = nextObject) {
		nextObject = object->m_pBrother;

		if (object->m_valueType == xmlStringValue)
			delete[] const_cast<char *>(object->m_stringValue);

		else if (object->m_valueType == xmlListBegin)
			destroyObjectList(object->m_pFirstSon);

		delete object;
	}
}


XmlParser::~XmlParser()
{
	// we have to delete all objects and allocated char arrays in string values
	destroyObjectList(m_objectTree);
	while (!m_objectBody.empty())
		delete[] m_objectBody.popFrontRet();
	delete[] m_keyName;
}


bool XmlParser::getLine()
{
	do {
//              Standard XML needs only this
//    	        if (m_is->eof()) return false;
//		m_is->getline(m_lineBuffer,255);

// Workaround for Get2Chip. XML-Information may exceed 254 signs per line.
// Moreover, XML signs may be longer than 254 signs. Cut information at '>'.
// Workaround starts here.
		char c;
		int count = 0;
		while ( ((c = m_is->get() ) != '>') && (count <= (BUFFERLENGTH - 2))){
			if (m_is->eof()) return false;
			m_lineBuffer[count++] = c;
		}
		if ( (c == '>') && (count <= (BUFFERLENGTH - 2)))
			m_lineBuffer[count++] = c;
		m_lineBuffer[count] = '\0';
// Workaround stops here.

		// Eat Whitespaces.
		for(m_pCurrent = m_lineBuffer; *m_pCurrent && isspace(*m_pCurrent); ++m_pCurrent) ;
	} while (*m_pCurrent == '#' || *m_pCurrent == 0);

	return true;
}

/*****************************************************************************
								getNextSymbol
******************************************************************************/



XmlObjectType XmlParser::getNextSymbol()
{
	*m_pStore = m_cStore;
	m_eoTag = false;
	bool digit = false;

	// eat whitespace
	for(; *m_pCurrent && isspace(*m_pCurrent); ++m_pCurrent) ;
	if (*m_pCurrent == '>')
	{
		m_pCurrent++;
		m_eoTag = true; // end of a tag reached.
	}
	for(; *m_pCurrent && isspace(*m_pCurrent); ++m_pCurrent) ;
	// get new line if required
	if (*m_pCurrent == 0) {
		if (!getLine()) return xmlEOF;
	}


	// identify start of current symbol
	char *pStart = m_pCurrent;

	// we currently do not support strings with line breaks!

	if (*pStart == '=')
	{	// attribute value
		// string or int or double expected

		// again: eat whitespace
		pStart++;m_pCurrent++;
		for(; *m_pCurrent && isspace(*m_pCurrent); ++m_pCurrent) ;
		// again: get new line if required
		if (*m_pCurrent == 0) {
			if (!getLine()) return xmlEOF;
		}
		// again: identify start of current symbol
		char *pStart = m_pCurrent;

		bool quotation = (*pStart == '\"' ? 1 : 0);
		if (quotation)
		{
			pStart++;
			m_pCurrent++;
		}
		if (*pStart == '-' || isdigit(*pStart)) // Check if int or double
		{
			digit = true;
			char *pCheck = m_pCurrent;
			pCheck++;
			while(isdigit(*pCheck))  ++pCheck;	// int or double
			if (*pCheck == '.')
				pCheck++;						// only double
			else if (quotation && *pCheck != '\"')
				digit = false;                  // must be string
			else if (!quotation && !isspace(*pCheck) && *pCheck != '>')
				digit = false;                  // must be string
			if (digit)
			{
				while(isdigit(*pCheck))  ++pCheck;
				if (quotation && *pCheck != '\"')
					digit = false;                  // must be string
				else if (!quotation && !isspace(*pCheck) && *pCheck != '>')
					digit = false;                  // must be string
			}
		}

//		if (!isdigit(*pStart) &&  (*pStart != '-')) { // string
		if (!digit) // string
		{
			m_stringSymbol = m_pCurrent;
			if (quotation){
				for(; *m_pCurrent != 0 && *m_pCurrent != '\"'; ++m_pCurrent)
					if (*m_pCurrent == '\\')
						++m_pCurrent; // No quotation mark found yet. Drop the line.
			}
			else {
				for(; *m_pCurrent != 0 && !isspace(*m_pCurrent) && *m_pCurrent != '>'; ++m_pCurrent)
					if (*m_pCurrent == '\\') ++m_pCurrent;
			}
			if (quotation && *m_pCurrent == 0) {
				m_longString = (pStart);
				while(getLine()) {
					for(m_pCurrent = m_lineBuffer; *m_pCurrent != 0 && *m_pCurrent != '\"'; ++m_pCurrent)
						if (*m_pCurrent == '\\') ++m_pCurrent;
					if (*m_pCurrent == 0)
						m_longString += m_lineBuffer;
					else {
						m_cStore = *(m_pStore = m_pCurrent);
						*m_pCurrent++ = 0;  // Drop quotation mark.
						m_longString += m_lineBuffer;
						break;
					}
				}
				m_stringSymbol = m_longString.cstr();

			}
			else {
				m_cStore = *(m_pStore = m_pCurrent);
				if (quotation)
					*m_pCurrent++ = 0;  // Drop quotation mark.
				else
					*m_pCurrent = 0;
			}

			return xmlStringValue;
		}
//		else if (*pStart == '-' || isdigit(*pStart))
		else  // int or double
		{
			m_pCurrent++;
			while(isdigit(*m_pCurrent)) ++m_pCurrent;

			if (*m_pCurrent == '.')  // double
			{
				// check to be done

				sscanf(pStart,"%lf",&m_doubleSymbol);
				m_pCurrent++;
				while (isdigit(*m_pCurrent)) ++m_pCurrent;
				if (quotation){
					for(; *m_pCurrent != 0 && *m_pCurrent != '\"' && isdigit(*m_pCurrent); ++m_pCurrent) ;

					if (*m_pCurrent == '\"')
						m_pCurrent++;
					else
					{
						setError("malformed number");
						return xmlError;
					}
				}
				return xmlDoubleValue;

			}

			else // int
			{
				if (isalpha(*m_pCurrent)) {
					setError("malformed number");
					return xmlError;
				}
				if (quotation){
					for(; *m_pCurrent != 0 && *m_pCurrent != '\"'; ++m_pCurrent);
					if (*m_pCurrent == '\"')
						m_pCurrent++;
					else{
						setError("malformed number");
						return xmlError;
					}
				}
				sscanf(pStart,"%d",&m_intSymbol);
				return xmlIntValue;
			}

		}
	}

	if (*pStart == '<') {
		//check if end of list
		m_pCurrent++;
		for (; *m_pCurrent && isspace(*m_pCurrent); ++m_pCurrent);
		if (*m_pCurrent  == '/'){
			for (; *m_pCurrent && *m_pCurrent != '>'; ++m_pCurrent);
			m_cStore = *(m_pStore = m_pCurrent);
			return xmlListEnd;
		}
		else {
			m_cStore = *(m_pStore = m_pCurrent);
			return xmlListBegin;
		}
	}
	else if (*pStart == '/') {
		m_pCurrent++;
		for (; *m_pCurrent && *m_pCurrent == '>'; ++m_pCurrent);
		m_cStore = *(m_pStore = m_pCurrent);
		return xmlListEnd;

	}

	else	// Invalid clause: if(isalpha(*pStart)). May contain numbers
	{
		// Tag name, Attribute Name (both are said to be keys)
		// or body name (element)

		// check if really a correct key (error if not)
		if (m_doCheck) {
			m_pCurrent++;
			for (;*m_pCurrent; ++m_pCurrent)
				if (!(isalpha(*m_pCurrent) || isdigit(*m_pCurrent))) {
					setError("malformed key");
					return xmlError;
				}
		}
		if (m_eoTag)  // its the body of an element
		{
			// Do not ignore whitespace, quotation marks etc.
			// They belong to the element.
			// Only search for the '<' of the next tag.
			// The element is considered as string.

			// Get new line if required
			if (*m_pCurrent == 0) {
				if (!getLine()) return xmlEOF;
			}
			// again: identify start of current symbol
			//char *pStart = m_pCurrent;


			m_stringSymbol = m_pCurrent;

			while(*m_pCurrent != 0 && *m_pCurrent != '<' )
				++m_pCurrent;
			m_cStore = *(m_pStore = m_pCurrent);
			*m_pCurrent = 0;

			return xmlStringValue;
		}
		else  // it is a key
		{
			while(*m_pCurrent != 0 &&
				*m_pCurrent != '=' &&
				*m_pCurrent != '>' &&
				*m_pCurrent != '/' &&
				*m_pCurrent != '<' &&
				!isspace(*m_pCurrent)
			)
				++m_pCurrent;
			m_cStore = *(m_pStore = m_pCurrent);
			*m_pCurrent = 0;

			if (m_keyName != 0)
				delete[] m_keyName;
			size_t len = strlen(pStart)+6;
			m_keyName = new char[len];
			ogdf::strcpy(m_keyName,len,pStart);

			m_keySymbol = hashString(pStart);
			return xmlKey;
		}
	}

	//
	//setError("unknown symbol");

	//return xmlError;
}




XmlKey XmlParser::hashString(const String &str)
{
	XmlKey key = m_hashTable.insertByNeed(str,-1);
	if(key->info() == -1) key->info() = m_num++;

	return key;
}

/*****************************************************************************
								getNodeIdRange
******************************************************************************/


XmlObject *XmlParser::getNodeIdRange(int &minId,int &maxId,
										 int &nodetypeCount,
										 XmlObject *graphObject)

{
	nodetypeCount = minId = maxId = -1;

	if (graphObject == 0)
		graphObject = m_objectTree;
	XmlObject *scanObject = graphObject;

	for(; scanObject; scanObject = scanObject->m_pBrother)
		if (id(scanObject) == graphPredefKey) break;

	if (!scanObject || id(scanObject) != graphPredefKey)
	{
		scanObject = graphObject;
		for(; scanObject; scanObject = scanObject->m_pBrother)
		{
			graphObject = getNodeIdRange(minId,maxId,nodetypeCount,scanObject->m_pFirstSon);
			if (graphObject && id(graphObject) == graphPredefKey)
				return graphObject;
		}
	}

	if (!scanObject || scanObject->m_valueType != xmlListBegin) return 0;

	XmlObject *son = scanObject->m_pFirstSon;
	for(; son; son = son->m_pBrother) {
		if (id(son) == nodePredefKey && son->m_valueType == xmlListBegin)
			maxId++;
		else if (id(son) == nodetypePredefKey && son->m_valueType == xmlListBegin)
			nodetypeCount++;
	}

	if (maxId >= 0)
		minId = 0;

	return scanObject;
}




/*****************************************************************************
									makeIdMap
******************************************************************************/


bool XmlParser::makeIdMap(
	int maxId,
	Array<char*> & idMap,
	int nodetypeCount,
	Array<char*> & typeName,
	Array<double> & typeWidth,
	Array<double> & typeHeight,
	XmlObject *graphObject)
{
	int idCount = 0;
	int typeCount = 0;
	for(; graphObject; graphObject = graphObject->m_pBrother)
		if (id(graphObject) == graphPredefKey) break;

	if (!graphObject || graphObject->m_valueType != xmlListBegin) return 0;

	XmlObject *son = graphObject->m_pFirstSon;
	for(; son; son = son->m_pBrother) {
		if (id(son) == nodePredefKey && son->m_valueType == xmlListBegin) {
			XmlObject *nodeSon = son->m_pFirstSon;
			for(; nodeSon; nodeSon = nodeSon->m_pBrother)
				if (id(nodeSon) == namePredefKey && nodeSon->m_valueType == xmlStringValue){
					if (idCount >= maxId+1)
						return 0;
					size_t len = strlen(nodeSon->m_stringValue)+1;
					idMap[idCount] = new char[len];
					ogdf::strcpy(idMap[idCount++],len,nodeSon->m_stringValue);
				}
		}
		else if (id(son) == nodetypePredefKey && son->m_valueType == xmlListBegin){
			XmlObject *nodeSon = son->m_pFirstSon;
			if (typeCount <= nodetypeCount){
				for(; nodeSon; nodeSon = nodeSon->m_pBrother) {
					if (id(nodeSon) == namePredefKey && nodeSon->m_valueType == xmlStringValue){
						size_t len = strlen(nodeSon->m_stringValue)+1;
						typeName[typeCount] = new char[len];
						ogdf::strcpy(typeName[typeCount],len,nodeSon->m_stringValue);
					}
					else if (id(nodeSon) == widthPredefKey ){
						if (nodeSon->m_valueType == xmlIntValue)
							typeWidth[typeCount] = (int) nodeSon->m_intValue;
						else if	(nodeSon->m_valueType == xmlDoubleValue)
							typeWidth[typeCount] = nodeSon->m_doubleValue;
					}
					else if (id(nodeSon) == heightPredefKey ){
						if (nodeSon->m_valueType == xmlIntValue)
							typeHeight[typeCount] = (int) nodeSon->m_intValue;
						else if	(nodeSon->m_valueType == xmlDoubleValue)
							typeHeight[typeCount] = nodeSon->m_doubleValue;
					}
				}
				typeCount++;
			}
		}

	}
	if (idCount != maxId+1)
		return 0;
	else
		return 1;
}


/*****************************************************************************
										read
******************************************************************************/


bool XmlParser::read(Graph &G)
{
	G.clear();

	int minId, maxId, nodetypeCount;
	XmlObject *graphObject = getNodeIdRange(minId, maxId, nodetypeCount,0);

	//cout << endl << minId << " " << maxId << endl;
	if (!graphObject) {
		setError("missing graph key");
		return false;
	}

	Array<double> typeWidth(0,nodetypeCount,0);
	Array<double> typeHeight(0,nodetypeCount,0);
	Array<char*>  typeName(nodetypeCount+1);
	Array<char*>  idMap(maxId+1);
	if (!makeIdMap(maxId,idMap,nodetypeCount,typeName,typeWidth,typeHeight,graphObject)) {
		setError("wrong name identifier");
		return false;
	}

	Array<node> mapToNode(minId,maxId,0);
	int notDefined = minId-1; //indicates not defined id key
	int idCount = minId;

	XmlObject *son = graphObject->m_pFirstSon;
	for(; son; son = son->m_pBrother) {

		switch(id(son)) {
		case nodePredefKey: {
			if (son->m_valueType != xmlListBegin) break;

			// set attributes to default values
			int vId = idCount++;

			// create new node if necessary and assign attributes
			if (mapToNode[vId] == 0)
				mapToNode[vId] = G.newNode();
			break;
		}
		case edgePredefKey:
			if (son->m_valueType != xmlListBegin) break;

			// set attributes to default values
			int sourceId = notDefined, targetId = notDefined;
			// read all relevant attributes
			XmlObject *edgeSon = son->m_pFirstSon;
			for(; edgeSon; edgeSon = edgeSon->m_pBrother) {
				int i = 0;
				switch(id(edgeSon)) {
				case sourcePredefKey:
					if (edgeSon->m_valueType != xmlStringValue) break;
					for (i = 0; i<= maxId;i++)
						if (!strcmp(idMap[i],edgeSon->m_stringValue))
							sourceId = i;
					break;

				case targetPredefKey:
					if (edgeSon->m_valueType != xmlStringValue) break;
					for (i = 0; i<= maxId;i++)
						if (!strcmp(idMap[i],edgeSon->m_stringValue))
							targetId = i;
					break;

				}
			}

			// check if everything required is defined correctly
			if (sourceId == notDefined || targetId == notDefined) {
				setError("source or target id not defined");
				//cout << "source or target id not defined" << endl;
				return false;

			} else if (sourceId < minId || maxId < sourceId ||
				targetId < minId || maxId < targetId) {
				setError("source or target id out of range");
				//cout <<  "source or target id out of range" << endl;
				return false;
			}

			// create adjacent nodes if necessary and new edge
			if (mapToNode[sourceId] == 0) mapToNode[sourceId] = G.newNode();
			if (mapToNode[targetId] == 0) mapToNode[targetId] = G.newNode();


			G.newEdge(mapToNode[sourceId],mapToNode[targetId]);
			break;
		}
	}

	return true;
}


/*****************************************************************************
									read
******************************************************************************/


bool XmlParser::read(Graph &G, GraphAttributes &AG)
{
	OGDF_ASSERT(&G == &(const Graph &)AG)

	G.clear();

	int minId, maxId, nodetypeCount;
	XmlObject *graphObject = getNodeIdRange(minId, maxId, nodetypeCount,0);

	if (!graphObject) {
		setError("missing graph key");
		return false;
	}

	Array<double> typeWidth(0,nodetypeCount,0);
	Array<double> typeHeight(0,nodetypeCount,0);
	Array<char*>  typeName(nodetypeCount+1);
	Array<char*>  idMap(maxId+1);
	if (!makeIdMap(maxId,idMap,nodetypeCount,typeName,typeWidth,typeHeight,graphObject)) {
		setError("wrong name identifier");
		closeLabels(idMap,typeName);
		return false;
	}
	Array<node> mapToNode(minId,maxId,0);
	int notDefined = minId-1; //indicates not defined id key
	int idCount = minId;

	DPolyline bends;
	String label;

	XmlObject *son = graphObject->m_pFirstSon;
	for(; son; son = son->m_pBrother) {

		switch(id(son)) {
		case nodePredefKey:
		{
			if (son->m_valueType != xmlListBegin) break;

			// set attributes to default values
			int vId = idCount++;;
			double x = 0, y = 0, w = 0, h = 0;
			bool typeDefined = 0;
			// read all relevant attributes

			XmlObject *graphicsObject = son->m_pFirstSon;
			for(; graphicsObject; graphicsObject = graphicsObject->m_pBrother)
			{
				switch(id(graphicsObject))
				{
					case namePredefKey:
						if (graphicsObject->m_valueType == xmlStringValue)
							label = graphicsObject->m_stringValue;
						break;
					case xPredefKey:
						if(graphicsObject->m_valueType == xmlDoubleValue)
							x = graphicsObject->m_doubleValue;
						else if (graphicsObject->m_valueType == xmlIntValue)
							x = (int) graphicsObject->m_intValue;
						break;
					case yPredefKey:
						if(graphicsObject->m_valueType == xmlDoubleValue)
							y = graphicsObject->m_doubleValue;
						else if (graphicsObject->m_valueType == xmlIntValue)
							y = (int) graphicsObject->m_intValue;
						break;
					case wPredefKey:
						if (!typeDefined)
						{
							if(graphicsObject->m_valueType == xmlDoubleValue)
								w = graphicsObject->m_doubleValue;
							else if (graphicsObject->m_valueType == xmlIntValue)
								w = (int) graphicsObject->m_intValue;
						}
						break;
					case hPredefKey:
						if (!typeDefined)
						{
							if(graphicsObject->m_valueType == xmlDoubleValue)
								h = graphicsObject->m_doubleValue;
							else if (graphicsObject->m_valueType == xmlIntValue)
								h = (int) graphicsObject->m_intValue;
						}
						break;
					case widthPredefKey:
						if (!typeDefined)
						{
							if(graphicsObject->m_valueType == xmlDoubleValue)
								w = graphicsObject->m_doubleValue;
							else if (graphicsObject->m_valueType == xmlIntValue)
								w = (int) graphicsObject->m_intValue;
						}
						break;
					case heightPredefKey:
						if (!typeDefined)
						{
							if(graphicsObject->m_valueType == xmlDoubleValue)
								h = graphicsObject->m_doubleValue;
							else if (graphicsObject->m_valueType == xmlIntValue)
								h = (int) graphicsObject->m_intValue;
						}
						break;
					case typePredefKey:
						if(graphicsObject->m_valueType == xmlStringValue)
						{
							int i = 0;
							for (;i <= nodetypeCount && strcmp(typeName[i],graphicsObject->m_stringValue);i++);
							if (i <= nodetypeCount)
							{
								w = typeWidth[i];
								h = typeHeight[i];
								typeDefined = 1;
							}
						}
						break;


					case positionPredefKey:
					{
						if (graphicsObject->m_valueType != xmlListBegin) break;


						XmlObject *graphicsObjectElement = graphicsObject->m_pFirstSon;
						for(; graphicsObjectElement; graphicsObjectElement = graphicsObjectElement->m_pBrother)
						{
							switch(id(graphicsObjectElement))
							{
								case xPredefKey:
									if(graphicsObjectElement->m_valueType == xmlDoubleValue)
										x = graphicsObjectElement->m_doubleValue;
									else if (graphicsObjectElement->m_valueType == xmlIntValue)
										x = (int) graphicsObjectElement->m_intValue;
									break;
								case yPredefKey:
									if(graphicsObjectElement->m_valueType == xmlDoubleValue)
										y = graphicsObjectElement->m_doubleValue;
									else if (graphicsObjectElement->m_valueType == xmlIntValue)
										y = (int) graphicsObjectElement->m_intValue;
									break;
							}// switch(id(graphicsObjectElement))
						}// for(; graphicsObjectElement; graphicsObjectElement = graphicsObjectElement->m_pBrother)
						break;
					}// case positionPredefKey:
					case sizePredefKey:
					{
						if (graphicsObject->m_valueType != xmlListBegin) break;


						XmlObject *graphicsObjectElement = graphicsObject->m_pFirstSon;
						for(; graphicsObjectElement; graphicsObjectElement = graphicsObjectElement->m_pBrother)
						{
							switch(id(graphicsObjectElement))
							{
								case wPredefKey:
									if (!typeDefined)
									{
										if(graphicsObjectElement->m_valueType == xmlDoubleValue)
											w = graphicsObjectElement->m_doubleValue;
										else if (graphicsObjectElement->m_valueType == xmlIntValue)
											w = (int) graphicsObjectElement->m_intValue;
									}
									break;
								case hPredefKey:
									if (!typeDefined)
									{
										if(graphicsObjectElement->m_valueType == xmlDoubleValue)
											h = graphicsObjectElement->m_doubleValue;
										else if (graphicsObjectElement->m_valueType == xmlIntValue)
											h = (int) graphicsObjectElement->m_intValue;
									}
									break;
								case widthPredefKey:
									if (!typeDefined)
									{
										if(graphicsObjectElement->m_valueType == xmlDoubleValue)
											w = graphicsObjectElement->m_doubleValue;
										else if (graphicsObjectElement->m_valueType == xmlIntValue)
											w = (int) graphicsObjectElement->m_intValue;
									}
									break;
								case heightPredefKey:
									if (!typeDefined)
									{
										if(graphicsObjectElement->m_valueType == xmlDoubleValue)
											h = graphicsObjectElement->m_doubleValue;
										else if (graphicsObjectElement->m_valueType == xmlIntValue)
											h = (int) graphicsObjectElement->m_intValue;
									}
									break;
							}// switch(id(graphicsObjectElement))
						}// for(; graphicsObjectElement; graphicsObjectElement = graphicsObjectElement->m_pBrother)

						break;
					}// case sizePredefKey:




				}// switch(id(graphicsObject))
			}// for(; graphicsObject; graphicsObject = graphicsObject->m_pBrother)


			// create new node if necessary and assign attributes
			if (mapToNode[vId] == 0) mapToNode[vId] = G.newNode();
			AG.x(mapToNode[vId]) = x;
			AG.y(mapToNode[vId]) = y;
			if (w > 0) //skip negative width
				AG.width (mapToNode[vId]) = w;
			if (h > 0) // skip negative height
				AG.height(mapToNode[vId]) = h;
			AG.labelNode(mapToNode[vId]) = label;
			break;

		}// case nodePredefKey:





		case edgePredefKey:
		{
			if (son->m_valueType != xmlListBegin) break;

			// set attributes to default values
			int sourceId = notDefined, targetId = notDefined;
			bool backward = false;
			// read all relevant attributes
			XmlObject *graphicsObject = son->m_pFirstSon;
			for(; graphicsObject; graphicsObject = graphicsObject->m_pBrother)
			{
				int i = 0;
				switch(id(graphicsObject))
				{
					case namePredefKey:
						if (graphicsObject->m_valueType == xmlStringValue)
							label = graphicsObject->m_stringValue;
						break;
					case sourcePredefKey:
						if (graphicsObject->m_valueType != xmlStringValue) break;
						for (i = 0; i<= maxId;i++)
							if (!strcmp(idMap[i],graphicsObject->m_stringValue))
								sourceId = i;
						break;

					case targetPredefKey:
						if (graphicsObject->m_valueType != xmlStringValue) break;
						for (i = 0; i<= maxId;i++)
							if (!strcmp(idMap[i],graphicsObject->m_stringValue))
								targetId = i;
						break;
					case sensePredefKey:
						if (graphicsObject->m_valueType != xmlStringValue) break;
							if (!strcmp("BACKWARD",graphicsObject->m_stringValue))
								backward = true;
					case pathPredefKey:
					{
						if (graphicsObject->m_valueType != xmlListBegin) break;
						DPoint dp;

						XmlObject *graphicsObjectElement = graphicsObject->m_pFirstSon;
						for(; graphicsObjectElement; graphicsObjectElement = graphicsObjectElement->m_pBrother)
						{
							switch(id(graphicsObjectElement))
							{
								case positionPredefKey:
								{
									if (graphicsObjectElement->m_valueType != xmlListBegin) break;
									DPoint dp;

									XmlObject *element = graphicsObjectElement->m_pFirstSon;
									for(; element; element = element->m_pBrother)
									{
										switch(id(element))
										{
											case xPredefKey:
												if(element->m_valueType == xmlDoubleValue)
													dp.m_x = element->m_doubleValue;
												else if (element->m_valueType == xmlIntValue)
													dp.m_x = (int) element->m_intValue;
												break;
											case yPredefKey:
												if(element->m_valueType == xmlDoubleValue)
													dp.m_y = element->m_doubleValue;
												else if (element->m_valueType == xmlIntValue)
													dp.m_y = (int) element->m_intValue;
												break;
										}// switch(id(element))
									}// for(; element; element = element->m_pBrother)
									bends.pushBack(dp);
									break;
								}// case positionPredefKey:
							}//switch(id(graphicsObjectElement))
						}//for(; graphicsObjectElement; graphicsObjectElement = graphicsObjectElement->m_pBrother)
						break;
					}// case positionPredefKey:
/*
					case graphicsPredefKey:
						if (graphicsObject->m_valueType != xmlListBegin) break;

						XmlObject *graphicsObject = graphicsObject->m_pFirstSon;
						for(; graphicsObject; graphicsObject = graphicsObject->m_pBrother) {
							if(id(graphicsObject) == LinePredefKey &&
								graphicsObject->m_valueType == xmlListBegin)
								readLineAttribute(graphicsObject->m_pFirstSon,bends);
						}
*/
				}// switch(id(graphicsObject))
			}// for(; graphicsObject; graphicsObject = graphicsObject->m_pBrother)

			// check if everything required is defined correctly
			if (sourceId == notDefined || targetId == notDefined)
			{
				setError("source or target id not defined");
				closeLabels(idMap,typeName);
				return false;

			}
			else if (sourceId < minId || maxId < sourceId ||
				targetId < minId || maxId < targetId)
			{
				setError("source or target id out of range");
				closeLabels(idMap,typeName);
				return false;
			}

			// create adjacent nodes if necessary and new edge
			if (mapToNode[sourceId] == 0) mapToNode[sourceId] = G.newNode();
			if (mapToNode[targetId] == 0) mapToNode[targetId] = G.newNode();

			edge e;
			if (backward)
				e = G.newEdge(mapToNode[targetId],mapToNode[sourceId]);
			else
				e = G.newEdge(mapToNode[sourceId],mapToNode[targetId]);
			AG.labelEdge(e) = label;
			AG.bends(e).conc(bends);
			break;

		}// case edgePredefKey:

		}// switch(id(son)) {
	}
	closeLabels(idMap,typeName);
	return true;
}


void XmlParser::closeLabels(Array<char*> idMap,	Array<char*>  typeName)
{
	int i;
	for (i = idMap.low(); i <= idMap.high();i++)
		if (idMap[i])
			delete[] idMap[i];
	for (i = typeName.low(); i <= typeName.high();i++)
		if (typeName[i])
			delete[] typeName[i];
}


void XmlParser::readLineAttribute(XmlObject *object, DPolyline &dpl)
{
	dpl.clear();
	for(; object; object = object->m_pBrother) {
		if (id(object) == pointPredefKey && object->m_valueType == xmlListBegin) {
			DPoint dp;

			XmlObject *pointObject = object->m_pFirstSon;
			for (; pointObject; pointObject = pointObject->m_pBrother) {
				if (pointObject->m_valueType != xmlDoubleValue) continue;
				if (id(pointObject) == xPredefKey)
					dp.m_x = pointObject->m_doubleValue;
				else if (id(pointObject) == yPredefKey)
					dp.m_y = pointObject->m_doubleValue;
			}

			dpl.pushBack(dp);
		}
	}
}


void XmlParser::setError(const char *errorString)
{
	m_error = true;
	m_errorString = errorString;
}



void XmlParser::indent(ostream &os, int d)
{
	for(int i = 1; i <= d; ++i)
		os << " ";
}

/*
void XmlParser::output(ostream &os, XmlObject *object, int d)
{
	for(; object; object = object->m_pBrother) {
		indent(os,d); os << object->m_key->key();

		switch(object->m_valueType) {
		case xmlIntValue:
			os << " " << object->m_intValue << "\n";
			break;

		case xmlDoubleValue:
			os << " " << object->m_doubleValue << "\n";
			break;

		case xmlStringValue:
			os << " \"" << object->m_stringValue << "\"\n";
			break;

		case xmlListBegin:
			os << "\n";
			output(os, object->m_pFirstSon, d+2);
			break;
		}
	}
}
*/

} // end namespace ogdf
