/*
 * $Revision: 2597 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-15 19:26:11 +0200 (So, 15. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Contains diverse enumerations and string constants.
 *
 * \author Christian Wolf, Carsten Gutwenger
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


#include <ogdf/fileformats/Ogml.h>


namespace ogdf
{

	/// This vector contains the real names of all OGML tags
	const String Ogml::s_tagNames[TAG_NUM] =
	{
		//"none"
		"bool",
		"composed",
		"constraint",
		"constraints",
		"content",
		"data",
		"default",
		"edge",
		"edgeRef",
		"edgeStyle",
		"edgeStyleTemplate",
		"template",
		"endpoint",
		"fill",
		"font",
		"graph",
		"graphStyle",
		"int",
		"label",
		"labelRef",
		"labelStyle",
		"labelStyleTemplate",
		"template",
		"layout",
		"line",
		"location",
		"node",
		"nodeRef",
		"nodeStyle",
		"nodeStyleTemplate",
		"template",
		"num",
		"ogml",
		"point",
		"port",
		"segment",
		"shape",
		"source",
		"sourceStyle",
		"string",
		"structure",
		"styles",
		"styleTemplates",
		"target",
		"targetStyle",
		"text",
		"image"
	};


	// This vector contains the real names of all OGML attributes.
	const String Ogml::s_attributeNames[ATT_NUM] =
	{
		"alignment",
		"angle",
		"color",
		"decoration",
		"defaultEdgeTemplate",
		"defaultLabelTemplate",
		"defaultNodeTemplate",
		"family",
		"height",
		"id",			// id attribute
		"idRef",		// attribute idRef of elements source, target, nodeRef, nodeStyle
		"idRef",		// attribute idRef of elements edgeRef, edgeStyle
		"idRef",		// attribute idRef of elements edgeRef, edgeStyle
		"idRef",		// attribute idRef of element endpoint
		"idRef",		// attribute idRef of element endpoint
		"idRef",		// attribute idRef of subelement template of element nodeStyle
		"idRef",		// attribute idRef of subelement template of element edgeStyle
		"idRef",		// attribute idRef of subelement template of element labelStyle
		"idRef",		// attribute idRef of subelement endpoint of element segment
		"name",
		"type",			// attribute type of subelement line of tag nodeStyleTemplate
		"type",			// attribute type of subelement shape of tag nodeStyleTemplate
		"pattern",
		"patternColor",
		"rotation",
		"size",
		"stretch",
		"style",
		"transform",
		"type",			// attribute type of subelements source-/targetStyle of tag edgeStyleTemplate
		"uri",
		"value",
		"value",
		"value",
		"variant",
		"weight",
		"width",
		"x",
		"y",
		"z",
		"uri",
		"style",
		"alignment",
		"drawLine",
		"width",
		"height",
		"type",
		"disabled"
	};


	// This vector contains the real names of all OGML values of attributes.
	const String Ogml::s_attributeValueNames[ATT_VAL_NUM] = {
		"any",					// for any attributeValue
		"blink",
		"bold",
		"bolder",
		"bool",
		"box",
		"capitalize",
		"center",
		"checked",
		"circle",
		"condensed",
		"cursive",
		"dashed",
		"esNoPen",				// values for line style
		"esSolid",
		"esDash",
		"esDot",
		"esDashdot",
		"esDashdotdot",
		"diamond",
		"dotted",
		"double",
		"doubleSlash",
		"ellipse",
		"expanded",
		"extraCondensed",
		"extraExpanded",
		"fantasy",
		"filledBox",
		"filledCircle",
		"filledDiamond",
		"filledHalfBox",
		"filledHalfCircle",
		"filledHalfDiamond",
		"filledHalfRhomb",
		"filledRhomb",
		"smurf",
		"arrow",
		"groove",
		"halfBox",
		"halfCircle",
		"halfDiamond",
		"halfRhomb",
		"hexagon",
		"hex",
		"id",
		"nodeId",				// attribute idRef of elements source, target, nodeRef, nodeStyle
		"edgeId",				// attribute idRef of elements edgeRef, edgeStyle
		"labelId",				// attribute idRef of elements edgeRef, edgeStyle
		"sourceId",				// attribute idRef of element endpoint
		"targetId",				// attribute idRef of element endpoint
		"nodeStyleTemplateId",	// attribute idRef of subelement template of element nodeStyle
		"edgeStyleTemplateId",	// attribute idRef of subelement template of element edgeStyle
		"labelStyleTemplateId",	// attribute idRef of subelement template of element labelStyle
		"pointId",				// attribute idRef of subelement endpoint of element segment
		"image",
		"inset",
		"int",
		"italic",
		"justify",
		"left",
		"lighter",
		"line",
		"lineThrough",
		"lowercase",
		"lParallelogram",
		"monospace",
		"narrower",
		"none",
		"normal",
		"num",
		"oblique",
		"oct",
		"octagon",
		"outset",
		"overline",
		"pentagon",
		"rect",
		"rectSimple",
		"rhomb",
		"ridge",
		"right",
		"rParallelogram",
		"sansSerif",
		"semiCondensed",
		"semiExpanded",
		"serif",
		"slash",
		"smallCaps",
		"solid",
		"bpNone",				// values for node patterns
		"bpSolid",
		"bpDense1",
		"bpDense2",
		"bpDense3",
		"bpDense4",
		"bpDense5",
		"bpDense6",
		"bpDense7",
		"bpHorizontal",
		"bpVertical",
		"bpCross",
		"bpBackwardDiagonal",
		"bpForwardDiagonal",
		"bpDiagonalCross",
		"string",
		"striped",
		"trapeze",
		"triangle",
		"triple",
		"ultraCondensed",
		"ultraExpanded",
		"umlClass",
		"underline",
		"uppercase",
		"upTrapeze",
		"uri",
		"wider",
		"freeScale",   			// image-style
		"fixScale",				// image-style
		"topLeft",				// image-alignemnt
		"topCenter",			// image-alignemnt
		"topRight",				// image-alignemnt
		"centerLeft",			// image-alignemnt
		//	"center",			// just defined	// image-alignemnt
		"centerRight",			// image-alignemnt
		"bottomLeft",			// image-alignemnt
		"bottomCenter",			// image-alignemnt
		"bottomRight",			// image-alignemnt
		"Alignment",
		"Anchor",
		"Sequence"
	};


	static const String s_graphTypeS[] =
	{
		"graph",
		"clusterGraph",
		"compoundGraph",
		"corruptCompoundGraph"
	};

}; //namspace ogdf
