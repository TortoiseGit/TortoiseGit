/*
 * $Revision: 2565 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-07 17:14:54 +0200 (Sa, 07. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Implementation of a line buffer serving the class DinoXmlScanner
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


#include <ogdf/fileformats/DinoLineBuffer.h>
#include <ogdf/basic/String.h>
#include <ogdf/fileformats/DinoTools.h>

#include <ctype.h>
#include <string.h>

extern ofstream os;

namespace ogdf {

	// Initialize static variables
	const int DinoLineBuffer::c_maxStringLength = OGDF_STRING_BUFFER_SIZE;
	const int DinoLineBuffer::c_maxLineLength = 200;
	const int DinoLineBuffer::c_maxNoOfLines = 20;

	//
	// ---------- D i n o L i n e B u f f e r P o s i t i o n ----------
	//

	//
	// C o n s t r u c t o r
	//
	DinoLineBufferPosition::DinoLineBufferPosition(
		int lineNumber,
		int lineUpdateCount,
		int linePosition)
	{
		set(lineNumber, lineUpdateCount, linePosition);
	}

	//
	// C o p y C o n s t r u c t o r
	//
	DinoLineBufferPosition::DinoLineBufferPosition(const DinoLineBufferPosition &position)
	{
		m_lineNumber = position.m_lineNumber;
		m_lineUpdateCount = position.m_lineUpdateCount;
		m_linePosition = position.m_linePosition;
	}

	//
	// s e t
	//
	void DinoLineBufferPosition::set(int lineNumber, int lineUpdateCount, int linePosition)
	{
		OGDF_ASSERT((lineNumber >= 0) && (lineNumber < DinoLineBuffer::c_maxNoOfLines))
		OGDF_ASSERT(lineUpdateCount >= 0)
		OGDF_ASSERT((linePosition >= 0) && (linePosition < DinoLineBuffer::c_maxLineLength))

		m_lineNumber = lineNumber;
		m_lineUpdateCount = lineUpdateCount;
		m_linePosition = linePosition;

	} // set

	//
	// i n c r e m e n t P o s i t i o n
	//
	void DinoLineBufferPosition::incrementPosition()
	{
		++m_linePosition;

		OGDF_ASSERT((m_linePosition >= 0) && (m_linePosition < DinoLineBuffer::c_maxLineLength))

	} // increasePosition

	//
	// o p e r a t o r ! =
	//
	bool DinoLineBufferPosition::operator!=(const DinoLineBufferPosition &position) const
	{
		if ((m_lineNumber != position.m_lineNumber) ||
			(m_lineUpdateCount != position.m_lineUpdateCount) ||
			(m_linePosition != position.m_linePosition))
		{
			return true;
		}

		return false;

	} // operator!=

	//
	// o p e r a t o r =
	//
	const DinoLineBufferPosition &
	DinoLineBufferPosition::operator=(const DinoLineBufferPosition &position)
	{
		if (&position != this){

			m_lineNumber = position.getLineNumber();
			m_lineUpdateCount = position.getLineUpdateCount();
			m_linePosition = position.getLinePosition();

		}

		return *this;

	} // operator=

	//
	// ---------- D i n o L i n e B u f f e r ----------
	//

	//
	// C o n s t r u c t o r
	//
	DinoLineBuffer::DinoLineBuffer(const char *fileName) :
		m_pIs(0),
		m_pLinBuf(0),
		m_numberOfMostRecentlyReadLine(0),
		m_inputFileLineCounter(0)
	{
		// Open file
		m_pIs = new ifstream(fileName, ios::in);
		if (!(*m_pIs)) {
			DinoTools::reportError("DinoLineBuffer::DinoLineBuffer", __LINE__, "Error opening file!");
		}

		// Create and initialize lineUpdateCountArray
		m_lineUpdateCountArray = new int[DinoLineBuffer::c_maxNoOfLines];
		int i;
		for (i = 0; i < DinoLineBuffer::c_maxNoOfLines; i++){
			m_lineUpdateCountArray[i] = 0;
		}

		// Create and initialize line buffer
		m_pLinBuf = new char[(DinoLineBuffer::c_maxNoOfLines * DinoLineBuffer::c_maxLineLength)];
		if (m_pLinBuf == 0)
			OGDF_THROW(InsufficientMemoryException);
		for (i = 0; i < DinoLineBuffer::c_maxNoOfLines * DinoLineBuffer::c_maxLineLength; i++){
			m_pLinBuf[i] = '0';
		}

		// Read first line
		if (!m_pIs->eof()){

			// Read first line
			m_pIs->getline(m_pLinBuf, DinoLineBuffer::c_maxLineLength);

			// Increase inputFileLineCounter
			++m_inputFileLineCounter;

			// Increase updateCount
			++(m_lineUpdateCountArray[0]);

		}
		// End of file is reached immeadiately
		else{

			// Set eof marker
			*m_pLinBuf = EOF;

		}

		// Set position
		m_currentPosition.set(0, m_lineUpdateCountArray[0], 0);

	} // DinoLineBuffer::DinoLineBuffer

	//
	// D e s t r u c t o r
	//
	DinoLineBuffer::~DinoLineBuffer()
	{
		// destroy line buffer
		delete [] m_pLinBuf;

		// destroy lineUpdateCountArray
		delete [] m_lineUpdateCountArray;

		// Close file
		delete m_pIs;

	} // DinoLineBuffer::~DinoLineBuffer

	//
	// m o v e T o N e x t C h a r a c t e r
	//
	char DinoLineBuffer::moveToNextCharacter(){

		// Return if end of file is reached
		if (getCurrentCharacter() == EOF){
			return EOF;
		}

		// Increment position
		m_currentPosition.incrementPosition();

		// End of line is reached, there can be some consecutive lines
		// with only \0 in it; hence we use a while loop
		while (getCurrentCharacter() == '\0'){

			// Current line is equal to most recently read line,
			// i.e. we have to read a new line from the file
			if (m_currentPosition.getLineNumber() == m_numberOfMostRecentlyReadLine){

				// Increment line pointer (modulo c_maxNoOfLines - 1)
				if (m_numberOfMostRecentlyReadLine == (DinoLineBuffer::c_maxNoOfLines - 1)){
					m_numberOfMostRecentlyReadLine = 0;
				}
				else {
					++m_numberOfMostRecentlyReadLine;
				}

				// Increment update count
				++(m_lineUpdateCountArray[m_numberOfMostRecentlyReadLine]);

				// Increment inputFileLineCounter
				++m_inputFileLineCounter;

				// Set current position
				m_currentPosition.set(
					m_numberOfMostRecentlyReadLine,
					m_lineUpdateCountArray[m_numberOfMostRecentlyReadLine],
					0);

				// End of file is reached
				if (m_pIs->eof()){

					// Set eof marker
					setCurrentCharacter(EOF);

				}
				// Read next line and put it to the new position
				else{

					m_pIs->getline(getCurrentCharacterPointer(), DinoLineBuffer::c_maxLineLength);
				}

			} // Current line is equal to most recently read line

			// Current line is NOT equal to most recently read line, i.e.
			// it is not necessary to read a new line from the file but to
			// set the currentPosition to the next line which is already in
			// the line buffer.
			else{

				int newLine;

				// Increment current line pointer (modulo c_maxNoOfLines - 1)
				if (m_currentPosition.getLineNumber() == (DinoLineBuffer::c_maxNoOfLines - 1)){
					newLine = 0;
				}
				else {
					newLine = m_currentPosition.getLineNumber() + 1;
				}

				// Set current position
				m_currentPosition.set(newLine, m_lineUpdateCountArray[newLine], 0);

			} // Current line is NOT equal to most recently read line

		} // End of line is reached

		return getCurrentCharacter();

	} // moveToNextCharacter

	//
	// s e t C u r r e n t P o s i t i o n
	//
	bool DinoLineBuffer::setCurrentPosition(const DinoLineBufferPosition &newPosition){

		// Given positon is not valid
		if (!isValidPosition(newPosition))
		{
			return false;
		}

		m_currentPosition = newPosition;

		return true;

	} // setCurrentPosition

	//
	// s k i p W h i t e s p a c e
	//
	void DinoLineBuffer::skipWhitespace()
	{

		if (getCurrentCharacter() == EOF) {
			return;
		}

		while ((isspace(getCurrentCharacter())) && (!(getCurrentCharacter() == EOF)))
		{
			moveToNextCharacter();
		}

	} // skipWhitespace

	//
	// e x t r a c t S t r i n g
	//
	bool DinoLineBuffer::extractString(
		const DinoLineBufferPosition &startPosition,
		const DinoLineBufferPosition &endPosition,
		char *targetString)
	{

		// StartPosition invalid, probably because the line of the startPosition
		// has already been overwritten, i.e. the string is too long
		if (!isValidPosition(startPosition))
		{
			ogdf::strcpy(targetString, DinoLineBuffer::c_maxStringLength, "String too long!");
			return false;
		}

		// EndPosition must be valid
		OGDF_ASSERT(isValidPosition(endPosition))

		// Remember original currentPosition
		DinoLineBufferPosition originalCurrentPosition = getCurrentPosition();

		// Begin at startPosition
		setCurrentPosition(startPosition);

		// Copy characters to tempString
		int targetStringIndex = 0;
		while (getCurrentPosition() != endPosition)
		{

			// Check if eof
			OGDF_ASSERT(getCurrentCharacter() != EOF)

			// Put character into targetString
			targetString[targetStringIndex] = getCurrentCharacter();
			++targetStringIndex;

			// String too long
			if (targetStringIndex >= DinoLineBuffer::c_maxStringLength - 1){

				ogdf::strcpy(targetString, DinoLineBuffer::c_maxStringLength, "String too long!");

				// Set back the original current position
				setCurrentPosition(originalCurrentPosition);

				return false;

			}

			// Move to next character
			moveToNextCharacter();

		} // Copy characters to tempString

		// Set back the original current position
		setCurrentPosition(originalCurrentPosition);

		// Terminate string
		targetString[targetStringIndex] = '\0';

		return true;

	} // extractString

	//
	// i s V a l i d P o s i t i o n
	//
	bool DinoLineBuffer::isValidPosition(const DinoLineBufferPosition &position) const
	{

		// We can assume that the position is valid according to
		// array ranges since these things are checked in constructor and set of
		// class DinoLineBufferPosition

		// The line of the given position has already been overwritten
		if (position.getLineUpdateCount() !=
			m_lineUpdateCountArray[position.getLineNumber()])
		{
			return false;
		}

		return true;

	} // isValidPosition

} // namespace ogdf
