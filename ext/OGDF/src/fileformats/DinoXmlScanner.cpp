/*
 * $Revision: 2565 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-07 17:14:54 +0200 (Sa, 07. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Implementation of the class DinoXmlScanner serving the
 *        class DinoXmlParser
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


#include <ogdf/fileformats/DinoXmlScanner.h>

#include <ctype.h>
#include <string.h>

extern ofstream os;

namespace ogdf {

	//
	// C o n s t r u c t o r
	//
	DinoXmlScanner::DinoXmlScanner(const char *fileName)
	{
		// Create line buffer
		m_pLineBuffer = new DinoLineBuffer(fileName);

		// Create current token string
		m_pCurrentTokenString = new char[DinoLineBuffer::c_maxStringLength];
		if (m_pCurrentTokenString == 0)
			OGDF_THROW(InsufficientMemoryException);
		for (int i = 0; i < DinoLineBuffer::c_maxStringLength; i++){
			m_pCurrentTokenString[i] = '0';
		}

	} // DinoXmlScanner::DinoXmlScanner

	//
	// D e s t r u c t o r
	//
	DinoXmlScanner::~DinoXmlScanner()
	{
		// Destroy current token string
		delete [] m_pCurrentTokenString;

		// Destroy line buffer
		delete m_pLineBuffer;

	} // DinoXmlScanner::~DinoXmlScanner

	//
	// g e t N e x t T o k e n
	//
	// Take a look at the state machine of getNextToken() to understand
	// what is going on here.
	//
	// TODO: It seems to be useful that this function throws an exception
	//       if something goes wrong.
	XmlToken DinoXmlScanner::getNextToken(){

		// First skip whitespaces
		m_pLineBuffer->skipWhitespace();

		// Let's have a look at the current character
		char currentCharacter = m_pLineBuffer->getCurrentCharacter();

		// End of file reached
		if (currentCharacter == EOF){
			return endOfFile;
		}

		// First we handle single characters with a switch statement
		switch (currentCharacter){

		// Opening Bracket
		case '<':
			{
				m_pLineBuffer->moveToNextCharacter();
				return openingBracket;
			}
			break;

		// Closing Bracket
		case '>':
			{
				m_pLineBuffer->moveToNextCharacter();
				return closingBracket;
			}
			break;

		// Question Mark
		case '?':
			{
				m_pLineBuffer->moveToNextCharacter();
				return questionMark;
			}
			break;

		// Exclamation Mark
		case '!':
			{
				m_pLineBuffer->moveToNextCharacter();
				return exclamationMark;
			}
			break;

		// Minus
		case '-':
			{
				m_pLineBuffer->moveToNextCharacter();
				return minus;
			}
			break;

		// Slash
		case '/':
			{
				m_pLineBuffer->moveToNextCharacter();
				return slash;
			}
			break;

		// Equal Sign
		case '=':
			{
				m_pLineBuffer->moveToNextCharacter();
				return equalSign;
			}
			break;

		} // end of switch

		// Now we handle more complex token

		// Identifier
		if (isalpha(currentCharacter)){

			// Put a pointer to the beginning of the identifier
			DinoLineBufferPosition startPosition = m_pLineBuffer->getCurrentPosition();

			currentCharacter = m_pLineBuffer->moveToNextCharacter();

			// Read valid identifier characters
			while ((isalnum(currentCharacter)) ||  // a..z|A..Z|0..9
				(currentCharacter == '.') ||
				(currentCharacter == ':') ||
				(currentCharacter == '_'))
			{
				currentCharacter = m_pLineBuffer->moveToNextCharacter();
			}

			// Copy identifier to currentTokenString
			m_pLineBuffer->extractString(startPosition,
										 m_pLineBuffer->getCurrentPosition(),
										 m_pCurrentTokenString);

			// Return identifier token
			return identifier;

		} // end of identifier

		// Quoted characters " ... " or ' ... '
		if ((currentCharacter == '\"') ||
			(currentCharacter == '\''))
		{
			// Distinguish what kind of quote sign we have
			bool doubleQuote;
			if (currentCharacter == '\"')
				doubleQuote = true;
			else
				doubleQuote = false;

			// Skip quote sign
			currentCharacter = m_pLineBuffer->moveToNextCharacter();

			// Read until the closing quotation sign is found
			// String is copied to m_pCurrentTokenString by readStringUntil()
			if (doubleQuote){
				readStringUntil('\"', false);
			}
			else{
				readStringUntil('\'', false);
			}

			// Skip over the end quote character
			m_pLineBuffer->moveToNextCharacter();

			// Return token for quoted value
			return quotedValue;

		} // end of quoted characters

		// An atributeValue, i.e. a sequence of characters, digits, minus - or dot .
		if ((isalnum(currentCharacter)) ||
			(currentCharacter == '-') ||
			(currentCharacter == '.'))
		{
			// Put a pointer to the beginning of the quoted text
			DinoLineBufferPosition startPosition = m_pLineBuffer->getCurrentPosition();;

			// Read until until an invalid character occurs
			currentCharacter = m_pLineBuffer->moveToNextCharacter();
			while ((isalnum(currentCharacter)) ||
				(currentCharacter == '-') ||
				(currentCharacter == '.'))
			{
				currentCharacter = m_pLineBuffer->moveToNextCharacter();
			}

			// Copy attributeValue to currentTokenString
			m_pLineBuffer->extractString(startPosition,
										 m_pLineBuffer->getCurrentPosition(),
										 m_pCurrentTokenString);

			// Return token for attribute value
			return attributeValue;

		} // end of an attributeValue

		// No valid token
		m_pLineBuffer->moveToNextCharacter();
		return invalidToken;

	} // getNextToken

	//
	// t e s t N e x t T o k e n
	//
	XmlToken DinoXmlScanner::testNextToken(){

		// Save pointer to the current position
		DinoLineBufferPosition originalPosition = m_pLineBuffer->getCurrentPosition();

		// Call getNextToken()
		XmlToken returnToken = getNextToken();

		// Set pointer back to the original position
		m_pLineBuffer->setCurrentPosition(originalPosition);

		// Return token
		return returnToken;

	} // testNextToken

	//
	// t e s t N e x t N e x t T o k e n
	//
	XmlToken DinoXmlScanner::testNextNextToken(){

		// Save pointer to the current position
		DinoLineBufferPosition originalPosition = m_pLineBuffer->getCurrentPosition();

		// Call getNextToken()
		getNextToken();

		// Again Call getNextToken()
		XmlToken returnToken = getNextToken();

		// Set pointer back to the original position
		m_pLineBuffer->setCurrentPosition(originalPosition);

		// Return token
		return returnToken;

	} // testNextNextToken

	//
	// s k i p U n t i l
	//
	bool DinoXmlScanner::skipUntil(char searchCharacter, bool skipOverSearchCharacter){

		while (m_pLineBuffer->getCurrentCharacter() != EOF){

			// Search character has been found!
			if (m_pLineBuffer->getCurrentCharacter() == searchCharacter){

				// Move to the position behind the search character if desired
				if (skipOverSearchCharacter){
					m_pLineBuffer->moveToNextCharacter();
				}

				return true;

			} // Search character has been found!

			// Move to next character and proceed
			m_pLineBuffer->moveToNextCharacter();

		} // while (!EOF)

		return false;

	} // skipUntil

	//
	// s k i p U n t i l M a t c h i n g C l o s i n g B r a c k e t
	//
	bool DinoXmlScanner::skipUntilMatchingClosingBracket(){

		// We assume that the opening bracket has already been read
		int bracketParity = 1;

		while ((m_pLineBuffer->getCurrentCharacter() != EOF) &&
			(bracketParity != 0))
		{
			// Opening bracket has been found!
			if (m_pLineBuffer->getCurrentCharacter() == '<'){

				++bracketParity;
			}

			// Closing bracket has been found!
			if (m_pLineBuffer->getCurrentCharacter() == '>'){

				--bracketParity;
			}

			// Move to next character and proceed
			m_pLineBuffer->moveToNextCharacter();

		} // while

		if (bracketParity != 0 )
			return false;
		else
			return true;

	} // skipUntilMatchingClosingBracket

	//
	// r e a d S t r i n g U n t i l
	//
	bool DinoXmlScanner::readStringUntil(char searchCharacter,
										 bool includeSearchCharacter){

		// Remember start position
		DinoLineBufferPosition startPosition = m_pLineBuffer->getCurrentPosition();

		// Use skipUntil()
		if (skipUntil(searchCharacter, includeSearchCharacter)){

			// Copy found string to m_pCurrentTokenString
			m_pLineBuffer->extractString(startPosition,
										 m_pLineBuffer->getCurrentPosition(),
										 m_pCurrentTokenString);

			return true;

		}
		// An error occurred
		else{
			return false;
		}

	} // getStringUntil

	//
	//  t e s t
	//
	void DinoXmlScanner::test(){

		bool terminate = false;
		XmlToken currentToken;

		while (!terminate){

			cout << "Line " << getInputFileLineCounter() << ": ";
			currentToken = getNextToken();

			switch (currentToken){
			case openingBracket:
				cout << "<" << endl;
				break;
			case closingBracket:
				cout << ">" << endl;
				break;
			case questionMark:
				cout << "?" << endl;
				break;
			case exclamationMark:
				cout << "!" << endl;
				break;
			case minus:
				cout << "-" << endl;
				break;
			case slash:
				cout << "/" << endl;
				break;
			case equalSign:
				cout << "<" << endl;
				break;
			case identifier:
				cout << "Identifier: " << m_pCurrentTokenString << endl;
				break;
			case attributeValue:
				cout << "Attribute value: " << m_pCurrentTokenString << endl;
				break;
			case quotedValue:
				cout << "Quoted value: \"" << m_pCurrentTokenString << "\"" << endl;
				break;
			case endOfFile:
				cout << "EOF" << endl;
				terminate = true;
				break;
			default:
				cout << "Invalid token!" << endl;

			} // switch

		} // while

	} // testScanner

} // namespace ogdf
