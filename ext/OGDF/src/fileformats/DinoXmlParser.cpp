/*
 * $Revision: 2565 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-07 17:14:54 +0200 (Sa, 07. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Implementation of XML parser (class DinoXmlParser)
 * (used for parsing and reading XML files)
 *
 * \author Dino Ahr
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


#include <ogdf/fileformats/DinoXmlParser.h>
#include <ogdf/fileformats/DinoTools.h>

#include <ctype.h>
#include <string.h>


namespace ogdf {

	//-----------------------------------------------------
	//Methods for handling XML objects for OGML file format
	//-----------------------------------------------------
	bool XmlTagObject::isLeaf() const {
		if(this->m_pFirstSon)	return false;
		else return true;
	}

	bool XmlTagObject::findSonXmlTagObjectByName(
		const String sonsName,
		XmlTagObject *&son) const
	{
		XmlTagObject *currentSon = this->m_pFirstSon;
		while(currentSon && currentSon->m_pTagName->key() != sonsName)
		{
			currentSon = currentSon->m_pBrother;
		}

		if(currentSon) {
			son = currentSon;
			return true;
		}

		son = 0;
		return false;
	}

	bool XmlTagObject::findSonXmlTagObjectByName(
		const String sonsName,
		List<XmlTagObject*> &sons) const
	{
		bool found;
		XmlTagObject *currentSon = this->m_pFirstSon;
		while(currentSon)
		{
			if(currentSon->m_pTagName->key() == sonsName) {
				found = true;
				sons.pushBack(currentSon);
			}
			currentSon = currentSon->m_pBrother;
		}

		return found;
	}

	bool XmlTagObject::hasMoreSonXmlTagObject(const List<String> &sonNamesToIgnore) const {
		const XmlTagObject *currentSon = this->m_pFirstSon;
		while(currentSon)
		{

			//Proof if name of currentSon is inequal to all in sonsName
			ListConstIterator<String> it;
			bool found = false;
			for(it = sonNamesToIgnore.begin(); it.valid() && !found; it++) {
					if(*it == currentSon->m_pTagName->key()) found = true;
			}
			if(!found) return true;
			currentSon = currentSon->m_pBrother;
		}

		return false;
	}

	bool XmlTagObject::findXmlAttributeObjectByName(
		const String attName,
		XmlAttributeObject*& attribute) const
	{
		XmlAttributeObject *currentAttribute = this->m_pFirstAttribute;
		while ((currentAttribute != 0) &&
			(currentAttribute->m_pAttributeName->key() != attName))
		{
			currentAttribute = currentAttribute->m_pNextAttribute;
		}

		// Attribute found
		if (currentAttribute != 0){
			attribute = currentAttribute;
			return true;
		}

		// Not found
		attribute = 0;
		return false;
	}

	bool XmlTagObject::isAttributeLess() const {
		if(this->m_pFirstAttribute) return false;
		else return true;
	}


	//
	// ---------- D i n o X m l P a r s e r ------------------------
	//

	//
	// C o n s t r u c t o r
	//
	DinoXmlParser::DinoXmlParser(const char *fileName) :
		m_pRootTag(0),
		m_hashTableInfoIndex(0),
		m_recursionDepth(0)
	{
		// Create scanner
		m_pScanner = new DinoXmlScanner(fileName);

	} // DinoXmlParser::DinoXmlParser

	//
	// D e s t r u c t o r
	//
	DinoXmlParser::~DinoXmlParser()
	{
		// Delete parse tree
		if (m_pRootTag)
			destroyParseTree(m_pRootTag);

		// Delete scanner
		delete m_pScanner;

	} // DinoXmlParser::~DinoXmlParser


	//
	//  c r e a t e P a r s e T r e e
	//
	void DinoXmlParser::createParseTree()
	{

		// Info
		//cout << "Parsing..." << endl;

		// create parse tree
		m_pRootTag = parse();

		// recursion depth not correct
		if (m_recursionDepth != 0) {
			DinoTools::reportError("DinoXmlParser::createParseTree", __LINE__, "Recursion depth not equal to zero after parsing!");
		}

	} // createParseTree

	//
	// d e s t r o y P a r s e T r e e
	//
	void DinoXmlParser::destroyParseTree(XmlTagObject *root)
	{
		// Destroy all attributes of root
		XmlAttributeObject *currentAttribute = root->m_pFirstAttribute;
		while (currentAttribute != 0){
			XmlAttributeObject *nextAttribute = currentAttribute->m_pNextAttribute;
			delete currentAttribute;
			currentAttribute = nextAttribute;
		}

		// Traverse children of root and destroy them
		XmlTagObject *currentChild = root->m_pFirstSon;
		while (currentChild != 0){
			XmlTagObject *nextChild = currentChild->m_pBrother;
			destroyParseTree(currentChild);
			currentChild = nextChild;
		}

		// Destroy root itself
		delete root;

	} // destroyParseTree


	//
	// p a r s e
	//
	// Take a look at the state machine of parse() to understand
	// what is going on here.
	//
	// TODO: It seems to be useful that this function throws an exception
	//       if something goes wrong.
	XmlTagObject *DinoXmlParser::parse()
	{
		// Increment recursion depth
		++m_recursionDepth;

		// currentTagObject is the tag object we want to create
		// in this invocation of parse()
		XmlTagObject *currentTagObject = 0;

		// Now we are in the start state of the state machine
		for( ; ; )
		{
			XmlToken token = m_pScanner->getNextToken();

			// Expect "<", otherwise failure
			if (token != openingBracket){
				DinoTools::reportError("DinoXmlParser::parse",
							__LINE__,
							"Opening Bracket expected!",
							getInputFileLineCounter());
			}

			// Let's look what comes after "<"
			token = m_pScanner->getNextToken();

			// Read "?", i.e. we have the XML header line <? ... ?>
			if (token == questionMark){

				// Skip until we reach the matching question mark
				if (!m_pScanner->skipUntil('?')){
					DinoTools::reportError("DinoXmlParser::parse",
								__LINE__,
								"Could not found the matching '?'",
								getInputFileLineCounter());
				}

				// Consume ">", otherwise failure
				token = m_pScanner->getNextToken();
				if (token != closingBracket){
					DinoTools::reportError("DinoXmlParser::parse",
								__LINE__,
								"Closing Bracket expected!",
								getInputFileLineCounter());
				}

				// Go to start state of the state machine
				continue;

			} // end of Read "?"

			// Read "!", i.e. we have a XML comment <!-- bla -->
			if (token == exclamationMark){

				// A preambel comment <!lala > which could be also nested
				if ((m_pScanner->getNextToken() != minus) ||
					(m_pScanner->getNextToken() != minus))
				{
					if (!m_pScanner->skipUntilMatchingClosingBracket()){

						DinoTools::reportError("DinoXmlParser::parse",
							__LINE__,
							"Could not find closing comment bracket!",
							getInputFileLineCounter());
					}

					continue;
				}

				// Find end of comment
				bool endOfCommentFound = false;
				while (!endOfCommentFound){

					// Skip until we find a - (and skip over it)
					if (!m_pScanner->skipUntil('-', true)){
						DinoTools::reportError("DinoXmlParser::parse",
							__LINE__,
							"Closing --> of comment not found!",
							getInputFileLineCounter());
					}

					// The next characters must be -> (note that one minus is already consumed)
					if ((m_pScanner->getNextToken() == minus) &&
						(m_pScanner->getNextToken() == closingBracket))
					{
						endOfCommentFound = true;
					}

				} // while

				// Go to start state of the state machine
				continue;

			} // end of Read "!"

			// We have found an identifier, i.e. a tag name
			if (token == identifier){

				// Get hash element of token string
				HashedString *tagName =
					hashString(m_pScanner->getCurrentTokenString());

				// Create new tag object
				currentTagObject = new XmlTagObject(tagName);
				if (currentTagObject == 0){
					OGDF_THROW(InsufficientMemoryException);
				}
				//push (opening) tagName to stack
				m_tagObserver.push(tagName->key());
				// set depth of current tag object
				currentTagObject->setDepth(m_recursionDepth);

				// set line of the tag object in the parsed xml document
				currentTagObject->setLine(getInputFileLineCounter());

				// Next token
				token = m_pScanner->getNextToken();

				// Again we found an identifier, so it must be an attribute
				if (token == identifier){

					// Read list of attributes
					do {
						// Save the attribute name
						HashedString *attributeName =
							hashString(m_pScanner->getCurrentTokenString());

						// Consume "=", otherwise failure
						token = m_pScanner->getNextToken();
						if (token != equalSign)
						{
							DinoTools::reportError("DinoXmlParser::parse",
										__LINE__,
										"Equal Sign expected!",
										getInputFileLineCounter());
						}

						// Read value
						token = m_pScanner->getNextToken();
						if ((token != quotedValue) &&
							(token != identifier) &&
							(token != attributeValue))
						{
							DinoTools::reportError("DinoXmlParser::parse",
										__LINE__,
										"No valid attribute value!",
										getInputFileLineCounter());
						}

						// Create a new XmlAttributeObject
						XmlAttributeObject *currentAttributeObject =
							new XmlAttributeObject(attributeName, hashString(m_pScanner->getCurrentTokenString()));
						if (currentAttributeObject == 0){
							OGDF_THROW(InsufficientMemoryException);
						}

						// Append attribute to attribute list of the current tag object
						appendAttributeObject(currentTagObject, currentAttributeObject);

						// Get next token
						token = m_pScanner->getNextToken();

					}
					while (token == identifier);

				} // Found an identifier of an attribute

				// Read "/", i.e. the tag is ended immeadiately, e.g.
				// <A ... /> without a closing tag </A>
				if (token == slash){

					// Consume ">", otherwise failure
					token = m_pScanner->getNextToken();
					if (token != closingBracket)
					{
						DinoTools::reportError("DinoXmlParser::parse",
									__LINE__,
									"Closing Bracket expected!",
									getInputFileLineCounter());
					}

					// The tag is closed and ended so we return
					String s = m_tagObserver.pop();
					--m_recursionDepth;
					return currentTagObject;

				} // end of Read "/"

				// Read ">", i.e. the tag is closed and we
				// expect some content
				if (token == closingBracket){

					// We read something different from "<", so we have to
					// deal with a tag value now, i.e. a string inbetween the
					// opening and the closing tag, e.g. <A ...> lalala </A>
					if (m_pScanner->testNextToken() != openingBracket){

						// Read the characters until "<" is reached and put them into
						// currentTagObject
						m_pScanner->readStringUntil('<');
						currentTagObject->m_pTagValue = hashString(m_pScanner->getCurrentTokenString());

						// We expect a closing tag now, i.e. </id>
						token = m_pScanner->getNextToken();
						if (token != openingBracket)
						{
							DinoTools::reportError("DinoXmlParser::parse",
								__LINE__,
								"Opening Bracket expected!",
								getInputFileLineCounter());
						}

						token = m_pScanner->getNextToken();
						if (token != slash)
						{
							DinoTools::reportError("DinoXmlParser::parse",
										__LINE__,
										"Slash expected!",
										getInputFileLineCounter());
						}

						token = m_pScanner->getNextToken();
						if (token != identifier)
						{
							DinoTools::reportError("DinoXmlParser::parse",
										__LINE__,
										"Identifier expected!",
										getInputFileLineCounter());
						}

						// next token is the closing tag
						String nextTag(m_pScanner->getCurrentTokenString());
						// pop corresponding tag from stack
						String s = m_tagObserver.pop();
						// compare the two tags
						if (s != nextTag)
						{
							// the closing tag doesn't correspond to the opening tag:
							DinoTools::reportError("DinoXmlParser::parse",
										__LINE__,
										"wrong closing tag!",
										getInputFileLineCounter());
						}

						token = m_pScanner->getNextToken();
						if (token != closingBracket)
						{
							DinoTools::reportError("DinoXmlParser::parse",
										__LINE__,
										"Closing Bracket expected!",
										getInputFileLineCounter());
						}

						// The tag is closed so we return
						--m_recursionDepth;
						return currentTagObject;

					} // end of read something different from "<"

					// Found "<", so a (series of) new tag begins and we have to perform
					// recursive invocation of parse()
					//
					// There are two exceptions:
					// - a slash follows afer <, i.e. we have a closing tag
					// - an exclamation mark follows after <, i.e. we have a comment
					while (m_pScanner->testNextToken() == openingBracket){

						// Leave the while loop if a closing tag occurs
						if (m_pScanner->testNextNextToken() == slash){
							break;
						}

						// Ignore comments
						if (m_pScanner->testNextNextToken() == exclamationMark){

							// Comment must start with <!--
							if ((m_pScanner->getNextToken() != openingBracket) ||
								(m_pScanner->getNextToken() != exclamationMark) ||
								(m_pScanner->getNextToken() != minus) ||
								(m_pScanner->getNextToken() != minus))
							{
								DinoTools::reportError("DinoXmlParser::parse",
									__LINE__,
									"Comment must start with <!--",
									getInputFileLineCounter());
							}

							// Find end of comment
							bool endOfCommentFound = false;
							while (!endOfCommentFound){

								// Skip until we find a - (and skip over it)
								if (!m_pScanner->skipUntil('-', true)){
									DinoTools::reportError("DinoXmlParser::parse",
										__LINE__,
										"Closing --> of comment not found!",
										getInputFileLineCounter());
								}

								// The next characters must be -> (note that one minus is already consumed)
								if ((m_pScanner->getNextToken() == minus) &&
									(m_pScanner->getNextToken() == closingBracket))
								{
									endOfCommentFound = true;
								}

							} // while

							// Proceed with outer while loop
							continue;

						} // Ignore comments

						// The new tag object is a son of the current tag object
						XmlTagObject *sonTagObject = parse();
						appendSonTagObject(currentTagObject, sonTagObject);

					} // while

					// Now we have found all tags.
					// We expect a closing tag now, i.e. </id>
					token = m_pScanner->getNextToken();
					if (token != openingBracket)
					{
						DinoTools::reportError("DinoXmlParser::parse",
									__LINE__,
									"Opening Bracket expected!",
									getInputFileLineCounter());
					}

					token = m_pScanner->getNextToken();
					if (token != slash)
					{
						DinoTools::reportError("DinoXmlParser::parse",
									__LINE__,
									"Slash expected!",
									getInputFileLineCounter());
					}

					token = m_pScanner->getNextToken();
					if (token != identifier)
					{
						DinoTools::reportError("DinoXmlParser::parse",
									__LINE__,
									"Identifier expected!",
									getInputFileLineCounter());
					}

 					// next token is the closing tag
					String nextTag(m_pScanner->getCurrentTokenString());
					// pop corresponding tag from stack
					String s = m_tagObserver.pop();
					// compare the two tags
					if (s != nextTag)
					{
						// the closing tag doesn't correspond to the opening tag:
						DinoTools::reportError("DinoXmlParser::parse",
									__LINE__,
									"wrong closing tag!",
									getInputFileLineCounter());
					}

					token = m_pScanner->getNextToken();
					if (token != closingBracket)
					{
						DinoTools::reportError("DinoXmlParser::parse",
									__LINE__,
									"Closing Bracket expected!",
									getInputFileLineCounter());
					}

					--m_recursionDepth;

					// check if Document contains code after the last closing bracket
					if (m_recursionDepth == 0){
						token = m_pScanner->getNextToken();
						if (token != endOfFile){
							DinoTools::reportError("DinoXmlParser::parse",
									__LINE__,
									"Document contains code after the last closing bracket!",
									getInputFileLineCounter());
						}
					}

					return currentTagObject;

				} // end of Read ">"

				OGDF_ASSERT(false)
				//continue;

			} // end of found identifier

			OGDF_ASSERT(false)

		} // end of while (true)

	} // parse

	//
	// a p p e n d A t t r i b u t e O b j e c t
	//
	void DinoXmlParser::appendAttributeObject(
		XmlTagObject *tagObject,
		XmlAttributeObject *attributeObject)
	{

		// No attribute exists yet
		if (tagObject->m_pFirstAttribute == 0) {
			tagObject->m_pFirstAttribute = attributeObject;
		}
		// At least one attribute exists
		else{

			XmlAttributeObject *currentAttribute = tagObject->m_pFirstAttribute;

			// Find the last attribute
			while (currentAttribute->m_pNextAttribute != 0){
				currentAttribute = currentAttribute->m_pNextAttribute;
			}

			// Append given attribute
			currentAttribute->m_pNextAttribute = attributeObject;

		}

	} // appendAttributeObject

	//
	// a p p e n d S o n T a g O b j e c t
	//
	void DinoXmlParser::appendSonTagObject(
		XmlTagObject *currentTagObject,
		XmlTagObject *sonTagObject)
	{
		// No Son exists yet
		if (currentTagObject->m_pFirstSon == 0) {
			currentTagObject->m_pFirstSon = sonTagObject;
		}
		// At least one son exists
		else{

			XmlTagObject *currentSon = currentTagObject->m_pFirstSon;

			// Find the last son
			while (currentSon->m_pBrother != 0){
				currentSon = currentSon->m_pBrother;
			}

			// Append given son
			currentSon->m_pBrother = sonTagObject;
		}

	} // appendSonTagObject

	//
	// h a s h S t r i n g
	//
	HashedString *DinoXmlParser::hashString(const String &str)
	{
		// insertByNeed inserts a new element (str, -1) into the
		// table if no element with key str exists;
		// otherwise nothing is done
		HashedString *key = m_hashTable.insertByNeed(str,-1);

		// String str was not contained in the table
		// --> assign a new info index to the new string
		if(key->info() == -1){
			key->info() = m_hashTableInfoIndex++;
		}

		return key;

	} // hashString

	//
	// t r a v e r s e P a t h
	//
	bool DinoXmlParser::traversePath(
		const XmlTagObject &startTag,
		const Array<int> &infoIndexPath,
		const XmlTagObject *&targetTag) const
	{
		// Traverse array
		const XmlTagObject *currentTag = &startTag;
		for (int i = 0; i < infoIndexPath.size(); i++){

			const XmlTagObject *sonTag;

			// Not found
			if (!findSonXmlTagObject(*currentTag, infoIndexPath[i], sonTag)){
				return false;
			}

			// Found
			currentTag = sonTag;

		} // for

		targetTag = currentTag;
		return true;

	} // traversePath

	//
	// f i n d S o n X m l T a g O b j e c t
	//
	bool DinoXmlParser::findSonXmlTagObject(const XmlTagObject &father,
			 								int sonInfoIndex,
											const XmlTagObject *&son) const
	{
		// Traverse sons
		const XmlTagObject *currentSon = father.m_pFirstSon;
		while ((currentSon != 0) &&
			(currentSon->m_pTagName->info() != sonInfoIndex))
		{
			currentSon = currentSon->m_pBrother;
		}

		// Son found
		if (currentSon != 0){
			son = currentSon;
			return true;
		}

		// Not found
		son = 0;
		return false;

	} // findSonXmlTagObject

	//
	// f i n d B r o t h e r X m l T a g O b j e c t
	//
	bool DinoXmlParser::findBrotherXmlTagObject(const XmlTagObject &currentTag,
												int brotherInfoIndex,
												const XmlTagObject *&brother) const
	{

		const XmlTagObject *currentBrother = currentTag.m_pBrother;
		while ((currentBrother != 0) &&
			(currentBrother->m_pTagName->info() != brotherInfoIndex))
		{
			currentBrother = currentBrother->m_pBrother;
		}

		// brother found
		if (currentBrother != 0){
			brother = currentBrother;
			return true;
		}

		// Not found
		brother = 0;
		return false;

	} // findBrotherXmlTagObject

	//
	// f i n d X m l A t t r i b u t e O b j e c t
	//
	bool DinoXmlParser::findXmlAttributeObject(
		const XmlTagObject &currentTag,
		int attributeInfoIndex,
		const XmlAttributeObject *&attribute) const
	{
		const XmlAttributeObject *currentAttribute = currentTag.m_pFirstAttribute;
		while ((currentAttribute != 0) &&
			(currentAttribute->m_pAttributeName->info() != attributeInfoIndex))
		{
			currentAttribute = currentAttribute->m_pNextAttribute;
		}

		// Attribute found
		if (currentAttribute != 0){
			attribute = currentAttribute;
			return true;
		}

		// Not found
		attribute = 0;
		return false;

	} // findXmlAttributeObject

	//
	// p r i n t H a s h T a b l e
	//
	void DinoXmlParser::printHashTable(ostream &os)
	{
		// Header
		os << "\n--- Content of Hash table: m_hashTable ---\n" << endl;

		// Get iterator
		HashConstIterator<String, int> it;

		// Traverse table
		for( it = m_hashTable.begin(); it.valid(); ++it){
			os << "\"" << it.key() << "\" has index " << it.info() << endl;
		}

	} // printHashTable

	//
	// p r i n t X m l T a g O b j e c t T r e e
	//
	void DinoXmlParser::printXmlTagObjectTree(
		ostream &outs,
		const XmlTagObject &rootObject,
		int indent) const
	{
		printSpaces(outs, indent);

		// Opening tag (bracket and Tag name)
		outs << "<" << rootObject.m_pTagName->key();

		// Attributes
		XmlAttributeObject *currentAttribute = rootObject.m_pFirstAttribute;
		while (currentAttribute != 0){

			outs << " "
				 << currentAttribute->m_pAttributeName->key()
				 << " = \""
				 << currentAttribute->m_pAttributeValue->key()
				 << "\"";

			// Next attribute
			currentAttribute = currentAttribute->m_pNextAttribute;

		} // while

		// Closing bracket
		outs << ">" << endl;

		// Children
		const XmlTagObject *currentChild = rootObject.m_pFirstSon;
		while (currentChild != 0){

			// Proceed recursively
			printXmlTagObjectTree(outs, *currentChild, indent + 2);

			// Next child
			currentChild = currentChild->m_pBrother;

		} // while

		// Content
		if (rootObject.m_pTagValue != 0){

			printSpaces(outs, indent + 2);

			outs << rootObject.m_pTagValue->key() << endl;

		}

		// Closing tag
		printSpaces(outs, indent);
		outs << "</" << rootObject.m_pTagName->key() << ">" << endl;

	} // printXmlTagObjectTree

	//
	// p r i n t S p a c e s
	//
	void DinoXmlParser::printSpaces(ostream &outs, int nOfSpaces) const
	{
		for (int i = 0; i < nOfSpaces; i++){
			outs << " ";
		}

	} // printSpaces


	//
	// o u t p u t O p e r a t o r  for DinoXmlParser
	//
	ostream &operator<<(ostream &os, const DinoXmlParser &parser)
	{
		parser.printXmlTagObjectTree(os, parser.getRootTag(), 0);
		return os;
	}


} // namespace ogdf
