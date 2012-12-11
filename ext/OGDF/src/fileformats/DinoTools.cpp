/*
 * $Revision: 2565 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-07 17:14:54 +0200 (Sa, 07. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Implementation of some tools
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


#include <ogdf/fileformats/DinoTools.h>
#include <ctype.h>

namespace ogdf {

	//
	// s t r i n g T o D o u b l e A r r a y
	//
	void DinoTools::stringToDoubleArray(const String &str, Array<double> &doubleArray)
	{
		size_t strIndex = 0;
		char tempString[20];
		int tempStringIndex = 0;

		for (int i = 0; i < 4; i++){

			tempStringIndex = 0;

			// Skip whitespace
			while (isspace(str[strIndex])){
				++strIndex;
			}

			// Copy characters of double value
			// values are separated by comma
			while (str[strIndex] != ','){

				tempString[tempStringIndex] = str[strIndex];
				++tempStringIndex;
				++strIndex;

			}

			// Skip over ','
			++strIndex;

			// Terminate string
			tempString[tempStringIndex] = '\0';

			// Put double value into array
			doubleArray[i] = atof(tempString);

		} // for

	} // stringToDoubleArray

	//
	// r e p o r t E r r o r
	//
	void DinoTools::reportError(
		const char *functionName,
		int sourceLine,
		const char *message,
		int inputFileLine,
		bool abort)
	{
		cerr << "Error reported!" << endl;
		cerr << "\tFunction: " << functionName << "(), Source line: " << sourceLine << endl;
		cerr << "\tMessage: " << message << endl;
		if (inputFileLine != -1) {
			cerr << "\tCurrent line of input file: " << inputFileLine;
		}

		cerr << endl;

		if (abort)
			exit(1);

	} // reportError

} // namespace ogdf
