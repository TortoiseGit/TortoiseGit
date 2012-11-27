/*
 * $Revision: 2565 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-07 17:14:54 +0200 (Sa, 07. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Implementation of OGML parser.
 *
 * \author Christian Wolf and Bernd Zey
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

#include <ogdf/fileformats/OgmlParser.h>
#include <ogdf/fileformats/Ogml.h>


namespace ogdf {


//---------------------------------------------------------
// OgmlParser::OgmlNodeTemplate
//---------------------------------------------------------

// struct definitions for mapping of templates
struct OgmlParser::OgmlNodeTemplate
{
	String  m_id;
	int     m_shapeType;
	double  m_width;
	double  m_height;
	String  m_color;
	GraphAttributes::BrushPattern m_pattern;
	String  m_patternColor;
	GraphAttributes::EdgeStyle m_lineType;
	double  m_lineWidth;
	String  m_lineColor;
	// nodeTemplate stores the graphical type
	// e.g. rectangle, ellipse, hexagon, ...
	String  m_nodeTemplate;

	//Constructor:
	OgmlNodeTemplate(const String &id): m_id(id) { }
};


//---------------------------------------------------------
// OgmlParser::OgmlEdgeTemplate
//---------------------------------------------------------

struct OgmlParser::OgmlEdgeTemplate
{
	String m_id;
	GraphAttributes::EdgeStyle m_lineType;
	double m_lineWidth;
	String m_color;
	int m_sourceType; // actually this is only a boolean value 0 or 1
	// ogdf doesn't support source-arrow-color and size
	//		String m_sourceColor;
	//		double m_sourceSize;
	int m_targetType; // actually this is only a boolean value 0 or 1
	// ogdf doesn't support target-arrow-color and size
	//		String m_targetColor;
	//		double m_targetSize;

	//Constructor:
	OgmlEdgeTemplate(const String &id): m_id(id) { }
};


//	struct  OgmlParser::OgmlLabelTemplate{
//		String m_id;
//	};


//---------------------------------------------------------
// OgmlParser::OgmlSegment
//---------------------------------------------------------

struct OgmlParser::OgmlSegment
{
	DPoint point1, point2;
};


//---------------------------------------------------------
// OgmlParser::OgmlAttributeValue
//---------------------------------------------------------

//! Objects of this class represent a value set of an attribute in Ogml.
class OgmlParser::OgmlAttributeValue
{
	int id; //!< Id of the attribute value; for possible ones see Ogml.h.

public:
	// Construction
	OgmlAttributeValue() : id(Ogml::av_any) { }

	OgmlAttributeValue(int id) {
		if(id >= 0 && id < Ogml::ATT_VAL_NUM) this->id = id;
		else id = Ogml::av_any;
	}

	// Destruction
	~OgmlAttributeValue() { }

	// Getter
	const int& getId() const { return id; }
	const String& getValue() const { return Ogml::s_attributeValueNames[id]; }

	// Setter
	void setId(int id) {
		if(id >= 0 && id < Ogml::ATT_VAL_NUM) this->id = id;
		else id = Ogml::av_any;
	}


	/**
	 * Checks the type of the input given in string
	 * and returns an OgmlAttributeValueId defined in Ogml.h
	 */
	Ogml::AttributeValueId getTypeOfString(const String& input) const
	{
		// |--------------------|
		// | char | ascii-value |
		// |--------------------|
		// | '.'  |     46      |
		// | '-'  |     45      |
		// | '+'  |     43      |
		// | '#'  |		35		|

		// bool values
		bool isInt = true;
		bool isNum = true;
		bool isHex = true;

		// value for point seperator
		bool numPoint = false;

		// input is a boolean value
		if (input == "true" || input == "false" /*|| input == "0" || input == "1"*/)
			return Ogml::av_bool;

		if (input.length() > 0){
			char actChar = input[0];
			int actCharInt = static_cast<int>(actChar);
			//check the first char
			if (!isalnum(actChar)){

				if (actCharInt == 35){
					// support hex values with starting "#"
					isInt = false;
					isNum = false;
				}
				else
				{

					// (actChar != '-') and (actChar != '+')
					if (!(actCharInt == 45) && !(actChar == 43)){
						isInt = isNum = false;
					}
					else
					{
						// input[0] == '-' or '+'
						if (input.length() > 1){
							// 2nd char have to be a digit or xdigit
							actChar = input[1];
							actCharInt = static_cast<int>(actChar);
							if (!isdigit(actChar)){
								isInt = false;
								isNum = false;
								if (!isxdigit(actChar))
									return Ogml::av_string;
							}
						}
						else
							return Ogml::av_string;
					} // else... (input[0] == '-')
				}
			}
			else{
				if (!isdigit(actChar)){
					isInt = false;
					isNum = false;
				}
				if (!isxdigit(actChar)){
					isHex = false;
				}
			}

			// check every input char
			// and set bool value to false if char-type is wrong
			for(size_t it=1; ( (it<input.length()) && ((isInt) || (isNum) || (isHex)) ); it++)
			{
				actChar = input[it];
				actCharInt = static_cast<int>(actChar);

				// actChar == '.'
				if (actChar == 46){
					isInt = false;
					isHex = false;
					if (!numPoint){
						numPoint = true;
					}
					else
						isNum = false;
				}// if (actChar == '.')
				else {
					if (!(isdigit(actChar))){
						isInt = false;
						isNum = false;
					}
					if (!(isxdigit(actChar)))
						isHex = false;
				}//else... (actChar != '.')
			}//for
		}//if (input.length() > 0)
		else{
			// input.length() == 0
			return Ogml::av_none;
		}
		// return correct value
		if (isInt) return Ogml::av_int;
		if (isNum) return Ogml::av_num;
		if (isHex) return Ogml::av_hex;
		// if all bool values are false return av_string
		return Ogml::av_string;

	}//getTypeOfString


	/**
	 * According to id this method proofs whether s is a valid value
	 * of the value set.
	 * E.g. if id=av_int s should contain an integer value.
	 * It returns the following validity states:
	 * 		vs_idNotUnique    =-10, //id already exhausted
	 * 		vs_idRefErr       = -9, //referenced id wasn't found or wrong type of referenced tag
	 * 		vs_idRefErr       = -8, //referenced id wrong
	 *		vs_attValueErr    = -3, //attribute-value error
	 * 		Ogml::vs_valid		  =  1  //attribute-value is valid
	 *
	 * TODO: Completion of the switch-case statement.
	 */
	int validValue(
		const String &attributeValue,
		const XmlTagObject* xmlTag,		        //owns an attribute with attributeValue
		Hashing<String,
		const XmlTagObject*>& ids) const //hashtable with id-tagName pairs
	{
		//get attribute value type of string
		Ogml::AttributeValueId stringType = getTypeOfString(attributeValue);

		HashElement<String, const XmlTagObject*>* he;

		int valid = Ogml::vs_attValueErr;

		switch(id) {
		case Ogml::av_any:
			valid = Ogml::vs_valid;
			break;

		case Ogml::av_int:
			if (stringType == Ogml::av_int) valid = Ogml::vs_valid;
			break;

		case Ogml::av_num:
			if (stringType == Ogml::av_num) valid = Ogml::vs_valid;
			if (stringType == Ogml::av_int) valid = Ogml::vs_valid;
			break;

		case Ogml::av_bool:
			if (stringType == Ogml::av_bool) valid = Ogml::vs_valid;
			break;

		case Ogml::av_string:
			valid = Ogml::vs_valid;
			break;

		case Ogml::av_hex:
			if (stringType == Ogml::av_hex) valid = Ogml::vs_valid;
			if (stringType == Ogml::av_int) valid = Ogml::vs_valid;
			break;

		case Ogml::av_oct:
			valid = Ogml::vs_attValueErr;
			break;

		case Ogml::av_id:
			// id mustn't exist
			if( !(he = ids.lookup(attributeValue)) ) {
				ids.fastInsert(attributeValue, xmlTag);
				valid = Ogml::vs_valid;
			}
			else valid = Ogml::vs_idNotUnique;
			break;

		// attribute idRef of elements source, target, nodeRef, nodeStyle
		case Ogml::av_nodeIdRef:
			// element exists && is tagname expected
			if( (he = ids.lookup(attributeValue)) && (he->info()->getName() == Ogml::s_tagNames[Ogml::t_node]) ) valid = Ogml::vs_valid;
			else valid = Ogml::vs_idRefErr;
			break;

		// attribute idRef of elements edgeRef, edgeStyle
		case Ogml::av_edgeIdRef:
			// element exists && is tagname expected
			if( (he = ids.lookup(attributeValue)) && (he->info()->getName() == Ogml::s_tagNames[Ogml::t_edge]) ) valid = Ogml::vs_valid;
			else valid = Ogml::vs_idRefErr;
			break;

		// attribute idRef of elements labelRef, labelStyle
		case Ogml::av_labelIdRef:
			// element exists && is tagname expected
			if( (he = ids.lookup(attributeValue)) && (he->info()->getName() == Ogml::s_tagNames[Ogml::t_label]) ) valid = Ogml::vs_valid;
			else valid = Ogml::vs_idRefErr;
			break;

		// attribute idRef of element endpoint
		case Ogml::av_sourceIdRef:
			// element exists && is tagname expected
			if( (he = ids.lookup(attributeValue)) && (he->info()->getName() == Ogml::s_tagNames[Ogml::t_source]) ) valid = Ogml::vs_valid;
			else valid = Ogml::vs_idRefErr;
			break;

		// attribute idRef of element endpoint
		case Ogml::av_targetIdRef:
			// element exists && is tagname expected
			if( (he = ids.lookup(attributeValue)) && (he->info()->getName() == Ogml::s_tagNames[Ogml::t_target]) ) valid = Ogml::vs_valid;
			else valid = Ogml::vs_idRefErr;
			break;

		// attribute idRef of subelement template of element nodeStyle
		case Ogml::av_nodeStyleTemplateIdRef:
			// element exists && is tagname expected
			if( (he = ids.lookup(attributeValue)) && (he->info()->getName() == Ogml::s_tagNames[Ogml::t_nodeStyleTemplate]) ) valid = Ogml::vs_valid;
			else valid = Ogml::vs_idRefErr;
			break;

		// attribute idRef of subelement template of element edgeStyle
		case Ogml::av_edgeStyleTemplateIdRef:
			// element exists && is tagname expected
			if( (he = ids.lookup(attributeValue)) && (he->info()->getName() == Ogml::s_tagNames[Ogml::t_edgeStyleTemplate]) ) valid = Ogml::vs_valid;
			else valid = Ogml::vs_idRefErr;
			break;

		// attribute idRef of subelement template of element labelStyle
		case Ogml::av_labelStyleTemplateIdRef:
			// element exists && is tagname expected
			if( (he = ids.lookup(attributeValue)) && (he->info()->getName() == Ogml::s_tagNames[Ogml::t_labelStyleTemplate]) ) valid = Ogml::vs_valid;
			else valid = Ogml::vs_idRefErr;
			break;

		case Ogml::av_pointIdRef:
			// element exists && is tagname expected
			if( (he = ids.lookup(attributeValue)) && (he->info()->getName() == Ogml::s_tagNames[Ogml::t_point]) ) valid = Ogml::vs_valid;
			else valid = Ogml::vs_idRefErr;
			break;

		default:
			// Proof string for equality
			if(getValue() == attributeValue) valid = Ogml::vs_valid;
			break;
		}

		return valid;
	}

};//class OgmlAttributeValue


//---------------------------------------------------------
// OgmlParser::OgmlAttribute
//---------------------------------------------------------

/** Objects of this class represent an attribute and its value set in Ogml.
*/
class OgmlParser::OgmlAttribute
{
	/**
	*/
	int id;  //!< Integer identifier of object; for possible ids see Ogml.h.
	List<OgmlAttributeValue*> values; //!< Represents the value set of this attribute.

public:

	// Construction
	OgmlAttribute() : id(Ogml::a_none), values() { }

	OgmlAttribute(int id) : values() {
		if(id >= 0 && id < Ogml::ATT_NUM) this->id = id;
		else this->id = Ogml::a_none;
	}

	// Destruction
	~OgmlAttribute() { }

	// Getter
	const int& getId() const { return id; }
	const String& getName() const { return Ogml::s_attributeNames[id]; }
	const List<OgmlAttributeValue*>& getValueList() const { return values; }

	// Setter
	void setId(int id) {
		if(id >= 0 && id < Ogml::ATT_NUM) this->id = id;
		else this->id = Ogml::a_none;
	}

	/**
	 * Pushes pointers to OgmlAttributeValue objects back to list values.
	 * These value objects are looked up in hashtable values.
	 *
	 * NOTE: This method uses a variable parameter list. The last parameter
	 *       need to be -1!
	 */
	void pushValues(Hashing<int, OgmlAttributeValue> *val, int key, ...) {
		va_list argp;
		int arg = key;
		HashElement<int, OgmlAttributeValue>* he;
		va_start(argp, key);
		while(arg!=-1) {
			if((he = val->lookup(arg))) values.pushBack( &(he->info()) );
			arg = va_arg(argp,int);
		}
		va_end(argp);
	}

	// Prints the value set of the attribute.
	void print(ostream &os) const {
		ListConstIterator<OgmlAttributeValue*> it;
		os << "\"" << getName() << "\"={ ";
		for(it = values.begin(); it.valid(); it++) {
			os << (**it).getValue() << " ";
		}
		os << "}\n";
	}

	/**This method proofs whether o is a valid attribute in comparison
	* to this object.
	* That means if the name of o and this object are equal and if
	* o has a valid value.
	* It returns a validity state code (see Ogml.h).
	**/
	int validAttribute(const XmlAttributeObject &xmlAttribute,
		const XmlTagObject* xmlTag,
		Hashing<String, const XmlTagObject*>& ids) const
	{
		int valid = Ogml::vs_expAttNotFound;

		if( xmlAttribute.getName() == getName() ) {
			ListConstIterator<OgmlAttributeValue*> it;
			for(it = values.begin(); it.valid(); it++) {
				if ( (valid = (**it).validValue( xmlAttribute.getValue(), xmlTag, ids )) == Ogml::vs_valid ) break;
			}
		}

		return valid;
	}
};//class OgmlAttribute



//---------------------------------------------------------
// OgmlParser::OgmlTag
//---------------------------------------------------------

/**Objects of this class represent a tag in Ogml with attributes.
*/
class OgmlParser::OgmlTag
{
	int id; //!< Integer identifier of object; for possible ids see Ogml.h.

	int minOccurs, maxOccurs; // Min. occurs and max. occurs of this tag.

	/**
	 * Flag denotes whether tag content can be ignored.
	 * It is possible to exchange this flag by a list of contents for more
	 * complex purposes ;-)
	 */
	bool ignoreContent;

	List<OgmlParser::OgmlAttribute*> compulsiveAttributes; //!< Represents the compulsive attributes of this object.
	List<OgmlAttribute*> choiceAttributes; //!< Represents the attributes of this object of which at least one needs to exist.
	List<OgmlAttribute*> optionalAttributes; //!< Represents the optional attributes of this object.

	List<OgmlTag*> compulsiveTags;
	List<OgmlTag*> choiceTags;
	List<OgmlTag*> optionalTags;


	void printOwnedTags(ostream &os, int mode) const
	{
		String s;
		const List<OgmlTag*> *list;

		switch(mode) {
		case 0:
			list = &compulsiveTags;
			s += "compulsive";
			break;

		case 1:
			list = &choiceTags;
			s += "selectable";
			break;

		case 2:
			list = &optionalTags;
			s += "optional";
			break;

		}

		if(list->empty())
			os << "Tag \"<" << getName() <<">\" doesn't include " << s << " tag(s).\n";
		else {
			os << "Tag \"<" << getName() <<">\" includes the following " << s << " tag(s): \n";
			ListConstIterator<OgmlTag*> currTag;
			for(currTag = list->begin(); currTag.valid(); currTag++)
				os << "\t<" << (**currTag).getName() << ">\n";
		}
	}

	void printOwnedAttributes(ostream &os, int mode) const
	{
		String s;
		const List<OgmlAttribute*> *list;

		switch(mode)
		case 0: {
			list = &compulsiveAttributes;
			s += "compulsive";
			break;

		case 1:
			list = &choiceAttributes;
			s += "selectable";
			break;

		case 2:
			list = &optionalAttributes;
			s += "optional";
			break;

		}

		if(list->empty())
			os << "Tag \"<" << getName() <<">\" doesn't include " << s << " attribute(s).\n";
		else {
			cout << "Tag \"<" << getName() <<">\" includes the following " << s << " attribute(s): \n";
			ListConstIterator<OgmlAttribute*> currAtt;
			for(currAtt = list->begin(); currAtt.valid(); currAtt++)
				os << "\t"  << (**currAtt);
		}
	}


public:

	bool ownsCompulsiveTags() {
		return !compulsiveTags.empty();
	}

	bool ownsChoiceTags() {
		return !choiceTags.empty();
	}

	bool ownsOptionalTags() {
		return !optionalTags.empty();
	}

	const List<OgmlTag*>& getCompulsiveTags() const { return compulsiveTags; }

	const List<OgmlTag*>& getChoiceTags() const { return choiceTags; }

	const List<OgmlTag*>& getOptionalTags() const { return optionalTags; }


	const int& getMinOccurs() const { return minOccurs; }

	const int& getMaxOccurs() const { return maxOccurs; }

	const bool& ignoresContent() const { return ignoreContent; }

	void setMinOccurs(int occurs) { minOccurs = occurs; }

	void setMaxOccurs(int occurs) { maxOccurs = occurs; }

	void setIgnoreContent(bool ignore) { ignoreContent = ignore; }

	//Construction
	OgmlTag() : id(Ogml::t_none), ignoreContent(0) { }

	OgmlTag(int id) : id(Ogml::t_none), ignoreContent(0) {
		if(id >= 0 && id < Ogml::TAG_NUM) this->id = id;
		else id = Ogml::a_none;
	}

	//Destruction
	~OgmlTag() {}

	//Getter
	const int& getId() const { return id; }
	const String& getName() const { return Ogml::s_tagNames[id]; }

	//Setter
	void setId(int id){
		if(id >= 0 && id < Ogml::TAG_NUM) this->id = id;
		else id = Ogml::a_none;
	}


	void printOwnedTags(ostream& os) const {
		printOwnedTags(os, 0);
		printOwnedTags(os, 1);
		printOwnedTags(os, 2);
	}

	void printOwnedAttributes(ostream& os) const {
		printOwnedAttributes(os, 0);
		printOwnedAttributes(os, 1);
		printOwnedAttributes(os, 2);
	}

	/**Pushes pointers to OgmlAttribute objects back to list reqAttributes.
	* These value objects are looked up in hashtable attrib.
	*
	* NOTE: This method uses a variable parameter list. The last parameter
	*       need to be -1!
	*/
	void pushAttributes(int mode, Hashing<int, OgmlAttribute> *attrib, int key, ...)
	{
		List<OgmlAttribute*>* list;

		if(mode==0) list = &compulsiveAttributes;
		else if(mode==1) list = &choiceAttributes;
		else list = &optionalAttributes;

		va_list argp;
		int arg = key;
		HashElement<int, OgmlAttribute>* he;
		va_start(argp, key);
		while(arg!=-1) {
			if((he = attrib->lookup(arg)))
				(*list).pushBack( &(he->info()) );
			arg = va_arg(argp,int);
		}
		va_end(argp);
	}

	/**Pushes pointers to OgmlAttribute objects back to list reqAttributes.
	* These value objects are looked up in hashtable tag.
	*
	* NOTE: This method uses a variable parameter list. The last parameter
	*       need to be -1!
	*/
	void pushTags(int mode, Hashing<int, OgmlTag> *tag, int key, ...)
	{
		List<OgmlTag*>* list;

		if(mode==0) list = &compulsiveTags;
		else if(mode==1) list = &choiceTags;
		else list = &optionalTags;

		va_list argp;
		int arg = key;
		HashElement<int, OgmlTag>* he;
		va_start(argp, key);

		while(arg!=-1) {
			if((he = tag->lookup(arg)))
				(*list).pushBack( &(he->info()) );
			arg = va_arg(argp,int);
		}
		va_end(argp);
	}

	/**This method proofs whether o is a valid tag in comparison
	* to this object.
	* That means if the name of o and this object are equal and if
	* the attribute list of o is valid (see also validAttribute(...)
	* in OgmlAttribute.h). Otherwise false.
	*/
	int validTag(const XmlTagObject &o,
		Hashing<String, const XmlTagObject*>& ids) const
	{
		int valid = Ogml::vs_unexpTag;

		if( o.getName() == getName() ) {

			ListConstIterator<OgmlAttribute*> it;
			XmlAttributeObject* att;

			//Tag requires attributes
			if(!compulsiveAttributes.empty()) {

				for(it = compulsiveAttributes.begin(); it.valid(); it++) {
					//Att not found or invalid
					if(!o.findXmlAttributeObjectByName((**it).getName(), att) )
						return valid = Ogml::vs_expAttNotFound;
					if( (valid = (**it).validAttribute(*att, &o, ids) ) <0 )
						return valid;
					//Att is valid
					att->setValid();
				}
			}

			//Choice attributes
			if(!choiceAttributes.empty()) {

				bool tookChoice = false;

				for(it = choiceAttributes.begin(); it.valid(); it++) {
					//Choice att found
					if( o.findXmlAttributeObjectByName((**it).getName(), att) ) {
						//Proof if valid
						if( (valid = (**it).validAttribute(*att, &o, ids)) <0 )
							return valid;
						tookChoice = true;
						att->setValid();
					}
				}

				if(!tookChoice)
					return valid = Ogml::vs_expAttNotFound;

			}

			if(!optionalAttributes.empty() && !o.isAttributeLess()) {

				//Check optional attributes
				for(it = optionalAttributes.begin(); it.valid(); it++) {
					if( o.findXmlAttributeObjectByName((**it).getName(), att) ) {
						if( (valid = (**it).validAttribute(*att, &o, ids)) <0 )
							return valid;
						att->setValid();
					}
				}
			}

			//Are there still invalid attributes?
			att = o.m_pFirstAttribute;
			while(att) {
				if(!att->valid())
					return valid = Ogml::vs_unexpAtt;
				att = att->m_pNextAttribute;
			}

			valid = Ogml::vs_valid;
		}

		return valid;
	}

};//class OgmlTag



//---------------------------------------------------------
// OgmlParser
//---------------------------------------------------------

// Definition of Hashtables
Hashing < int, OgmlParser::OgmlTag >            *OgmlParser::s_tags = 0;
Hashing < int, OgmlParser::OgmlAttribute >      *OgmlParser::s_attributes = 0;
Hashing < int, OgmlParser::OgmlAttributeValue > *OgmlParser::s_attValues = 0;


// ***********************************************************
//
// b u i l d H a s h T a b l e s
//
// ***********************************************************
void OgmlParser::buildHashTables()
{
	if(s_tags != 0)  // hash tables already built?
		return;

	s_tags       = new Hashing < int, OgmlParser::OgmlTag >;
	s_attributes = new Hashing < int, OgmlParser::OgmlAttribute >;
	s_attValues  = new Hashing < int, OgmlParser::OgmlAttributeValue >;

	// Create OgmlAttributeValue objects and fill hashtable s_attValues.

	for (int i = 0; i < Ogml::ATT_VAL_NUM; i++)
		s_attValues->fastInsert(i, OgmlAttributeValue(i));

	for (int i = 0; i < Ogml::ATT_NUM; i++)
		s_attributes->fastInsert(i, OgmlAttribute(i));


	// Create OgmlAttribute objects and fill hashtable attributes.

	for (int i = 0; i < Ogml::ATT_NUM; i++) {

		OgmlAttribute &att = s_attributes->lookup(i)->info();

		switch (i) {

		case Ogml::a_alignment:
			att.pushValues(s_attValues,
				Ogml::av_left,
				Ogml::av_center,
				Ogml::av_right,
				Ogml::av_justify,
				-1);
			break;

		case Ogml::a_angle:
			att.pushValues(s_attValues,
				Ogml::av_int,
				-1);
			break;

		case Ogml::a_color:
			att.pushValues(s_attValues,
				Ogml::av_hex,
				-1);
			break;

		case Ogml::a_decoration:
			att.pushValues(s_attValues,
				Ogml::av_underline,
				Ogml::av_overline,
				Ogml::av_lineThrough,
				Ogml::av_blink,
				Ogml::av_none,
				-1);
			break;

		case Ogml::a_defaultEdgeTemplate:
			att.pushValues(s_attValues, Ogml::av_any, -1);
			break;

		case Ogml::a_defaultLabelTemplate:
			att.pushValues(s_attValues, Ogml::av_any, -1);
			break;

		case Ogml::a_defaultNodeTemplate:
			att.pushValues(s_attValues, Ogml::av_any, -1);
			break;

		case Ogml::a_family:
			att.pushValues(s_attValues,
				Ogml::av_serif,
				Ogml::av_sansSerif,
				Ogml::av_cursive,
				Ogml::av_fantasy,
				Ogml::av_monospace,
				-1);
			break;

		case Ogml::a_height:
			att.pushValues(s_attValues, Ogml::av_num, -1);
			break;

		case Ogml::a_id:
			att.pushValues(s_attValues, Ogml::av_id, -1);
			break;

		case Ogml::a_nodeIdRef:
			att.pushValues(s_attValues, Ogml::av_nodeIdRef, -1);
			break;

		case Ogml::a_edgeIdRef:
			att.pushValues(s_attValues, Ogml::av_edgeIdRef, -1);
			break;

		case Ogml::a_labelIdRef:
			att.pushValues(s_attValues, Ogml::av_labelIdRef, -1);
			break;

		case Ogml::a_sourceIdRef:
			att.pushValues(s_attValues, Ogml::av_nodeIdRef, Ogml::av_edgeIdRef, -1);
			break;

		case Ogml::a_targetIdRef:
			att.pushValues(s_attValues, Ogml::av_nodeIdRef, Ogml::av_edgeIdRef, -1);
			break;

		case Ogml::a_nodeStyleTemplateIdRef:
			att.pushValues(s_attValues, Ogml::av_nodeStyleTemplateIdRef, -1);
			break;

		case Ogml::a_edgeStyleTemplateIdRef:
			att.pushValues(s_attValues, Ogml::av_edgeStyleTemplateIdRef, -1);
			break;

		case Ogml::a_labelStyleTemplateIdRef:
			att.pushValues(s_attValues, Ogml::av_labelStyleTemplateIdRef, -1);
			break;

		case Ogml::a_endpointIdRef:
			att.pushValues(s_attValues, Ogml::av_pointIdRef, Ogml::av_sourceIdRef, Ogml::av_targetIdRef, -1);
			break;

		case Ogml::a_name:
			att.pushValues(s_attValues, Ogml::av_any, -1);
			break;

		// attribute type of subelement line of tag nodeStyleTemplate
		case Ogml::a_nLineType:
			att.pushValues(s_attValues,
				Ogml::av_solid,
				Ogml::av_dotted,
				Ogml::av_dashed,
				Ogml::av_double,
				Ogml::av_triple,
				Ogml::av_groove,
				Ogml::av_ridge,
				Ogml::av_inset,
				Ogml::av_outset,
				Ogml::av_none,
				Ogml::av_esNoPen,
				Ogml::av_esSolid,
				Ogml::av_esDash,
				Ogml::av_esDot,
				Ogml::av_esDashdot,
				Ogml::av_esDashdotdot,
				-1);
			break;

		// attribute type of subelement shape of tag nodeStyleTemplate
		case Ogml::a_nShapeType:
			att.pushValues(s_attValues,
				Ogml::av_rect,
				Ogml::av_rectSimple,
				Ogml::av_triangle,
				Ogml::av_circle,
				Ogml::av_ellipse,
				Ogml::av_hexagon,
				Ogml::av_rhomb,
				Ogml::av_trapeze,
				Ogml::av_upTrapeze,
				Ogml::av_lParallelogram,
				Ogml::av_rParallelogram,
				Ogml::av_pentagon,
				Ogml::av_octagon,
				Ogml::av_umlClass,
				Ogml::av_image,
				-1);
			break;

		case Ogml::a_pattern:
			att.pushValues(s_attValues,
				Ogml::av_solid,
				Ogml::av_striped,
				Ogml::av_checked,
				Ogml::av_dotted,
				Ogml::av_none,
				Ogml::av_bpNone,
				Ogml::av_bpSolid,
				Ogml::av_bpDense1,
				Ogml::av_bpDense2,
				Ogml::av_bpDense3,
				Ogml::av_bpDense4,
				Ogml::av_bpDense5,
				Ogml::av_bpDense6,
				Ogml::av_bpDense7,
				Ogml::av_bpHorizontal,
				Ogml::av_bpVertical,
				Ogml::av_bpCross,
				Ogml::av_bpBackwardDiagonal,
				Ogml::av_bpForwardDiagonal,
				Ogml::av_bpDiagonalCross, -1);
			break;

		case Ogml::a_patternColor:
			att.pushValues(s_attValues, Ogml::av_hex, -1);
			break;

		case Ogml::a_rotation:
			att.pushValues(s_attValues, Ogml::av_int, -1);
			break;

		case Ogml::a_size:
			att.pushValues(s_attValues, Ogml::av_int, -1);
			break;

		case Ogml::a_stretch:
			att.pushValues(s_attValues,
				Ogml::av_normal,
				Ogml::av_wider,
				Ogml::av_narrower,
				Ogml::av_ultraCondensed,
				Ogml::av_extraCondensed,
				Ogml::av_condensed,
				Ogml::av_semiCondensed,
				Ogml::av_semiExpanded,
				Ogml::av_expanded,
				Ogml::av_extraExpanded,
				Ogml::av_ultraExpanded, -1);
			break;

		case Ogml::a_style:
			att.pushValues(s_attValues, Ogml::av_normal, Ogml::av_italic, Ogml::av_oblique, -1);
			break;

		case Ogml::a_transform:
			att.pushValues(s_attValues, Ogml::av_capitalize, Ogml::av_uppercase, Ogml::av_lowercase, Ogml::av_none, -1);
			break;

		// attribute type of subelements source-/targetStyle of tag edgeStyleTemplate
		case Ogml::a_type:
			att.pushValues(s_attValues,
				Ogml::av_circle,
				Ogml::av_halfCircle,
				Ogml::av_filledCircle,
				Ogml::av_filledHalfCircle,
				Ogml::av_box,
				Ogml::av_halfBox,
				Ogml::av_filledBox,
				Ogml::av_filledHalfBox,
				Ogml::av_rhomb,
				Ogml::av_halfRhomb,
				Ogml::av_filledRhomb,
				Ogml::av_filledHalfRhomb,
				Ogml::av_diamond,
				Ogml::av_halfDiamond,
				Ogml::av_filledDiamond,
				Ogml::av_filledHalfDiamond,
				Ogml::av_smurf,
				Ogml::av_arrow,
				Ogml::av_slash,
				Ogml::av_doubleSlash,
				Ogml::av_solid,
				Ogml::av_line,
				Ogml::av_none, -1);
			break;

		case Ogml::a_uri:
			att.pushValues(s_attValues, Ogml::av_uri, -1);
			break;

		case Ogml::a_intValue:
			att.pushValues(s_attValues, Ogml::av_int, -1);
			break;

		case Ogml::a_numValue:
			att.pushValues(s_attValues, Ogml::av_num, -1);
			break;

		case Ogml::a_boolValue:
			att.pushValues(s_attValues, Ogml::av_bool, -1);
			break;

		case Ogml::a_variant:
			att.pushValues(s_attValues, Ogml::av_normal, Ogml::av_smallCaps, -1);
			break;

		case Ogml::a_weight:
			att.pushValues(s_attValues,
				Ogml::av_normal,
				Ogml::av_bold,
				Ogml::av_bolder,
				Ogml::av_lighter,
				Ogml::av_int,
				-1);
			break;

		case Ogml::a_width:
			att.pushValues(s_attValues, Ogml::av_num, -1);
			break;

		case Ogml::a_x:
			att.pushValues(s_attValues, Ogml::av_num, -1);
			break;

		case Ogml::a_y:
			att.pushValues(s_attValues, Ogml::av_num, -1);
			break;

		case Ogml::a_z:
			att.pushValues(s_attValues, Ogml::av_num, -1);

		case Ogml::a_imageUri:
			att.pushValues(s_attValues, Ogml::av_string, -1);

		case Ogml::a_imageStyle:
			att.pushValues(s_attValues, Ogml::av_freeScale, Ogml::av_fixScale, -1);

		case Ogml::a_imageAlignment:
			att.pushValues(s_attValues, Ogml::av_topLeft,
				Ogml::av_topCenter,
				Ogml::av_topRight,
				Ogml::av_centerLeft,
				Ogml::av_center,
				Ogml::av_centerRight,
				Ogml::av_bottomLeft,
				Ogml::av_bottomCenter,
				Ogml::av_bottomRight, -1);

		case Ogml::a_imageDrawLine:
			att.pushValues(s_attValues, Ogml::av_bool, -1);

		case Ogml::a_imageWidth:
			att.pushValues(s_attValues, Ogml::av_num, -1);

		case Ogml::a_imageHeight:
			att.pushValues(s_attValues, Ogml::av_num, -1);

		case Ogml::a_constraintType:
			att.pushValues(s_attValues, Ogml::av_constraintAlignment, Ogml::av_constraintAnchor, Ogml::av_constraintSequence, -1);

		case Ogml::a_disabled:
			att.pushValues(s_attValues, Ogml::av_bool, -1);

		}
	}


	// Create OgmlTag objects and fill hashtable tags.

	for (int i = 0; i < Ogml::TAG_NUM; i++)
		s_tags->fastInsert(i, OgmlTag(i));


	enum Mode { compMode = 0, choiceMode, optMode };

	// Create tag relations.

	for (int i = 0; i < Ogml::TAG_NUM; i++)
	{
		OgmlTag &tag = s_tags->lookup(i)->info();

		switch (i) {
		case Ogml::t_bool:
			tag.setMinOccurs(0);
			tag.setMaxOccurs(Ogml::MAX_TAG_COUNT);
			tag.pushAttributes(compMode, s_attributes, Ogml::a_boolValue, -1);
			tag.pushAttributes(optMode, s_attributes, Ogml::a_name, -1);
			break;

		case Ogml::t_composed:
			tag.setMinOccurs(0);
			tag.setMaxOccurs(Ogml::MAX_TAG_COUNT);
			tag.pushAttributes(optMode, s_attributes, Ogml::a_name, -1);
			tag.pushTags(choiceMode, s_tags, Ogml::t_num, Ogml::t_int, Ogml::t_bool,
				Ogml::t_string, Ogml::t_nodeRef, Ogml::t_edgeRef, Ogml::t_labelRef, Ogml::t_composed, -1);
			break;

		case Ogml::t_constraint:
			tag.setMinOccurs(0);
			tag.setMaxOccurs(Ogml::MAX_TAG_COUNT);
			tag.pushAttributes(compMode, s_attributes, Ogml::a_constraintType, -1);
			tag.pushAttributes(choiceMode, s_attributes, Ogml::a_id, Ogml::a_name, Ogml::a_disabled, -1);
			tag.pushTags(choiceMode, s_tags, Ogml::t_num, Ogml::t_int, Ogml::t_bool,
				Ogml::t_string, Ogml::t_nodeRef, Ogml::t_edgeRef, Ogml::t_labelRef, Ogml::t_composed, Ogml::t_constraint, -1);
			break;

		case Ogml::t_constraints:
			tag.setMinOccurs(0);
			tag.setMaxOccurs(1);
			tag.pushTags(compMode, s_tags, Ogml::t_constraint, -1);
			break;

		case Ogml::t_content:
			tag.setMinOccurs(0);
			tag.setMaxOccurs(1);
			tag.setIgnoreContent(true);
			break;

		case Ogml::t_data:
			tag.setMinOccurs(0);
			tag.setMaxOccurs(Ogml::MAX_TAG_COUNT);
			tag.pushAttributes(optMode, s_attributes, Ogml::a_name, -1);
			tag.pushTags(choiceMode, s_tags, Ogml::t_int, Ogml::t_bool, Ogml::t_num, Ogml::t_string, Ogml::t_data, -1);
			break;

		case Ogml::t_default:
			tag.setMinOccurs(0);
			tag.setMaxOccurs(1);
			break;

		case Ogml::t_edge:
			tag.setMinOccurs(0);
			tag.setMaxOccurs(Ogml::MAX_TAG_COUNT);
			tag.pushAttributes(compMode, s_attributes, Ogml::a_id, -1);
			tag.pushTags(choiceMode, s_tags, Ogml::t_source, Ogml::t_target, -1);
			tag.pushTags(optMode, s_tags, Ogml::t_data, Ogml::t_label, -1);
			break;

		case Ogml::t_edgeRef:
			tag.setMinOccurs(0);
			tag.setMaxOccurs(Ogml::MAX_TAG_COUNT);
			tag.pushAttributes(compMode, s_attributes, Ogml::a_edgeIdRef, -1);
			tag.pushAttributes(optMode, s_attributes, Ogml::a_name, -1);
			break;

		case Ogml::t_edgeStyle:
			tag.setMinOccurs(0);
			tag.setMaxOccurs(Ogml::MAX_TAG_COUNT);
			tag.pushAttributes(compMode, s_attributes, Ogml::a_edgeIdRef, -1);
			tag.pushTags(choiceMode, s_tags, Ogml::t_edgeStyleTemplateRef,
				Ogml::t_line, Ogml::t_sourceStyle, Ogml::t_targetStyle, Ogml::t_point, Ogml::t_segment, -1);
			tag.pushTags(optMode, s_tags, Ogml::t_data, -1);
			break;

		case Ogml::t_edgeStyleTemplate:
			tag.setMinOccurs(0);
			tag.setMaxOccurs(Ogml::MAX_TAG_COUNT);
			tag.pushAttributes(compMode, s_attributes, Ogml::a_id, -1);
			tag.pushTags(choiceMode, s_tags, Ogml::t_line, Ogml::t_sourceStyle, Ogml::t_targetStyle, -1);
			tag.pushTags(optMode, s_tags, Ogml::t_data, Ogml::t_edgeStyleTemplateRef, -1);
			break;

		case Ogml::t_endpoint:
			tag.setMinOccurs(2);
			tag.setMaxOccurs(2);
			tag.pushAttributes(compMode, s_attributes, Ogml::a_endpointIdRef, -1);
			tag.pushAttributes(optMode, s_attributes, Ogml::a_type, Ogml::a_color, Ogml::a_size, -1);
			break;

		case Ogml::t_fill:
			tag.setMinOccurs(0);
			tag.setMaxOccurs(1);
			tag.pushAttributes(compMode, s_attributes, Ogml::a_color, Ogml::a_pattern, Ogml::a_patternColor, -1);
			break;

		case Ogml::t_font:
			tag.setMinOccurs(0);
			tag.setMaxOccurs(1);
			tag.pushAttributes(compMode, s_attributes, Ogml::a_family, -1);
			tag.pushAttributes(optMode, s_attributes, Ogml::a_style,
				Ogml::a_variant, Ogml::a_weight, Ogml::a_stretch, Ogml::a_size, Ogml::a_color, -1);
			break;

		case Ogml::t_graph:
			tag.setMinOccurs(1);
			tag.setMaxOccurs(1);
			tag.pushTags(compMode, s_tags, Ogml::t_structure, -1);
			tag.pushTags(optMode, s_tags, Ogml::t_layout, Ogml::t_data, -1);
			break;

		case Ogml::t_graphStyle:
			tag.setMinOccurs(0);
			tag.setMaxOccurs(1);
			tag.pushAttributes(choiceMode, s_attributes,
				Ogml::a_defaultNodeTemplate,
				Ogml::a_defaultEdgeTemplate, Ogml::a_defaultLabelTemplate, -1);
			break;

		case Ogml::t_int:
			tag.setMinOccurs(0);
			tag.setMaxOccurs(Ogml::MAX_TAG_COUNT);
			tag.pushAttributes(compMode, s_attributes, Ogml::a_intValue, -1);
			tag.pushAttributes(optMode, s_attributes, Ogml::a_name, -1);
			break;

		case Ogml::t_label:
			tag.setMinOccurs(0);
			tag.setMaxOccurs(Ogml::MAX_TAG_COUNT);
			tag.pushAttributes(compMode, s_attributes, Ogml::a_id, -1);
			tag.pushTags(compMode, s_tags, Ogml::t_content, -1);
			tag.pushTags(optMode, s_tags, Ogml::t_data, -1);
			break;

		case Ogml::t_labelRef:
			tag.setMinOccurs(0);
			tag.setMaxOccurs(Ogml::MAX_TAG_COUNT);
			tag.pushAttributes(compMode, s_attributes, Ogml::a_labelIdRef, -1);
			tag.pushAttributes(optMode, s_attributes, Ogml::a_name, -1);
			break;

		case Ogml::t_labelStyle:
			tag.setMinOccurs(0);
			tag.setMaxOccurs(Ogml::MAX_TAG_COUNT);
			tag.pushAttributes(compMode, s_attributes, Ogml::a_labelIdRef, -1);
			tag.pushTags(choiceMode, s_tags, Ogml::t_labelStyleTemplateRef,
				Ogml::t_data, Ogml::t_text, Ogml::t_font, Ogml::t_location, -1);
			break;

		case Ogml::t_labelStyleTemplate:
			tag.setMinOccurs(0);
			tag.setMaxOccurs(Ogml::MAX_TAG_COUNT);
			tag.pushAttributes(compMode, s_attributes, Ogml::a_id, -1);
			tag.pushTags(compMode, s_tags, Ogml::t_text, Ogml::t_font, -1);
			tag.pushTags(optMode, s_tags, Ogml::t_data, Ogml::t_labelStyleTemplateRef, -1);
			break;

		case Ogml::t_layout:
			tag.setMinOccurs(0);
			tag.setMaxOccurs(1);
			tag.pushTags(optMode, s_tags, Ogml::t_data, Ogml::t_styleTemplates, Ogml::t_styles, Ogml::t_constraints, -1);
			break;

		case Ogml::t_line:
			tag.setMinOccurs(0);
			tag.setMaxOccurs(1);
			tag.pushAttributes(choiceMode, s_attributes, Ogml::a_nLineType, Ogml::a_width, Ogml::a_color, -1);
			break;

		case Ogml::t_location:
			tag.setMinOccurs(0);
			tag.setMaxOccurs(1);
			tag.pushAttributes(compMode, s_attributes, Ogml::a_x, Ogml::a_y, -1);
			tag.pushAttributes(optMode, s_attributes, Ogml::a_z, -1);
			break;

		case Ogml::t_node:
			tag.setMinOccurs(1);
			tag.setMaxOccurs(Ogml::MAX_TAG_COUNT);
			tag.pushAttributes(compMode, s_attributes, Ogml::a_id, -1);
			tag.pushTags(optMode, s_tags, Ogml::t_data, Ogml::t_label, Ogml::t_node, -1);
			break;

		case Ogml::t_nodeRef:
			tag.setMinOccurs(0);
			tag.setMaxOccurs(Ogml::MAX_TAG_COUNT);
			tag.pushAttributes(compMode, s_attributes, Ogml::a_nodeIdRef, -1);
			tag.pushAttributes(optMode, s_attributes, Ogml::a_name, -1);
			break;

		case Ogml::t_nodeStyle:
			tag.setMinOccurs(0);
			tag.setMaxOccurs(Ogml::MAX_TAG_COUNT);
			tag.pushAttributes(compMode, s_attributes, Ogml::a_nodeIdRef, -1);
			tag.pushTags(choiceMode, s_tags, Ogml::t_location, Ogml::t_shape, Ogml::t_fill, Ogml::t_line, Ogml::t_image, -1);
			tag.pushTags(optMode, s_tags, Ogml::t_data, Ogml::t_nodeStyleTemplateRef, -1);
			break;


		case Ogml::t_nodeStyleTemplate:
			tag.setMinOccurs(0);
			tag.setMaxOccurs(Ogml::MAX_TAG_COUNT);
			tag.pushAttributes(compMode, s_attributes, Ogml::a_id, -1);
			tag.pushTags(choiceMode, s_tags, Ogml::t_shape, Ogml::t_fill, Ogml::t_line, -1);
			tag.pushTags(optMode, s_tags, Ogml::t_data, Ogml::t_nodeStyleTemplateRef, -1);
			break;

		case Ogml::t_num:
			tag.setMinOccurs(0);
			tag.setMaxOccurs(Ogml::MAX_TAG_COUNT);
			tag.pushAttributes(compMode, s_attributes, Ogml::a_numValue, -1);
			tag.pushAttributes(optMode, s_attributes, Ogml::a_name, -1);
			break;

		case Ogml::t_ogml:
			tag.setMinOccurs(1);
			tag.setMaxOccurs(1);
			tag.pushTags(compMode, s_tags, Ogml::t_graph, -1);
			break;

		case Ogml::t_point:
			tag.setMinOccurs(0);
			tag.setMaxOccurs(Ogml::MAX_TAG_COUNT);
			tag.pushAttributes(compMode, s_attributes, Ogml::a_id, Ogml::a_x, Ogml::a_y, -1);
			tag.pushAttributes(optMode, s_attributes, Ogml::a_z, -1);
			tag.pushTags(optMode, s_tags, Ogml::t_data, -1);
			break;

		case Ogml::t_port:
			tag.setMinOccurs(0);
			tag.setMaxOccurs(Ogml::MAX_TAG_COUNT);
			tag.pushAttributes(compMode, s_attributes, Ogml::a_id, Ogml::a_x, Ogml::a_y, -1);
			break;

		case Ogml::t_segment:
			tag.setMinOccurs(0);
			tag.setMaxOccurs(Ogml::MAX_TAG_COUNT);
			tag.pushTags(compMode, s_tags, Ogml::t_endpoint, -1);
			tag.pushTags(optMode, s_tags, Ogml::t_data, Ogml::t_line, -1);
			break;

		case Ogml::t_shape:
			tag.setMinOccurs(0);
			tag.setMaxOccurs(1);
			tag.pushAttributes(choiceMode, s_attributes, Ogml::a_nShapeType, Ogml::a_width, Ogml::a_height, /*a_uri,*/ -1);
			// comment (BZ): uri is obsolete, images got an own tag
			break;

		case Ogml::t_source:
			tag.setMinOccurs(0);
			tag.setMaxOccurs(Ogml::MAX_TAG_COUNT);
			tag.pushAttributes(compMode, s_attributes, Ogml::a_sourceIdRef, -1);
			tag.pushAttributes(optMode, s_attributes, Ogml::a_id, -1);
			tag.pushTags(optMode, s_tags, Ogml::t_data, Ogml::t_label, -1);
			break;

		case Ogml::t_sourceStyle:
			tag.setMinOccurs(0);
			tag.setMaxOccurs(1);
			tag.pushAttributes(choiceMode, s_attributes, Ogml::a_type, Ogml::a_color, Ogml::a_size, -1);
			break;

		case Ogml::t_string:
			tag.setMinOccurs(0);
			tag.setMaxOccurs(Ogml::MAX_TAG_COUNT);
			tag.pushAttributes(optMode, s_attributes, Ogml::a_name, -1);
			tag.setIgnoreContent(true);
			break;

		case Ogml::t_structure:
			tag.setMinOccurs(1);
			tag.setMaxOccurs(1);
			tag.pushTags(compMode, s_tags, Ogml::t_node, -1);
			tag.pushTags(optMode, s_tags, Ogml::t_edge, Ogml::t_label, Ogml::t_data, -1);
			break;

		case Ogml::t_styles:
			tag.setMinOccurs(0);
			tag.setMaxOccurs(1);
			tag.pushTags(choiceMode, s_tags, Ogml::t_nodeStyle, Ogml::t_edgeStyle, Ogml::t_labelStyle, -1);
			tag.pushTags(optMode, s_tags, Ogml::t_graphStyle, Ogml::t_data, -1);
			break;

		case Ogml::t_styleTemplates:
			tag.setMinOccurs(0);
			tag.setMaxOccurs(1);
			tag.pushTags(choiceMode, s_tags, Ogml::t_nodeStyleTemplate,
				Ogml::t_edgeStyleTemplate, Ogml::t_labelStyleTemplate, -1);
			tag.pushTags(optMode, s_tags, Ogml::t_data, -1);
			break;

		case Ogml::t_target:
			tag.setMinOccurs(0);
			tag.setMaxOccurs(Ogml::MAX_TAG_COUNT);
			tag.pushAttributes(compMode, s_attributes, Ogml::a_targetIdRef, -1);
			tag.pushAttributes(optMode, s_attributes, Ogml::a_id, -1);
			tag.pushTags(optMode, s_tags, Ogml::t_data, Ogml::t_label, -1);
			break;

		case Ogml::t_targetStyle:
			tag.setMinOccurs(0);
			tag.setMaxOccurs(1);
			tag.pushAttributes(choiceMode, s_attributes, Ogml::a_type, Ogml::a_color, Ogml::a_size, -1);
			break;

		case Ogml::t_labelStyleTemplateRef:
			tag.setMinOccurs(0);
			tag.setMaxOccurs(1);
			tag.pushAttributes(compMode, s_attributes, Ogml::a_labelStyleTemplateIdRef, -1);
			break;

		case Ogml::t_nodeStyleTemplateRef:
			tag.setMinOccurs(0);
			tag.setMaxOccurs(1);
			tag.pushAttributes(compMode, s_attributes, Ogml::a_nodeStyleTemplateIdRef, -1);
			break;

		case Ogml::t_edgeStyleTemplateRef:
			tag.setMinOccurs(0);
			tag.setMaxOccurs(1);
			tag.pushAttributes(compMode, s_attributes, Ogml::a_edgeStyleTemplateIdRef, -1);
			break;

		case Ogml::t_text:
			tag.setMinOccurs(0);
			tag.setMaxOccurs(1);
			tag.pushAttributes(choiceMode, s_attributes, Ogml::a_alignment,
				Ogml::a_decoration, Ogml::a_transform, Ogml::a_rotation, -1);
			break;

		case Ogml::t_image:
			tag.setMinOccurs(0);
			tag.setMaxOccurs(1);
			tag.pushAttributes(compMode, s_attributes, Ogml::a_imageUri, -1);
			tag.pushAttributes(optMode, s_attributes,
				Ogml::a_imageStyle,
				Ogml::a_imageAlignment,
				Ogml::a_imageDrawLine,
				Ogml::a_imageWidth,
				Ogml::a_imageHeight, -1);
			break;
		}
	}
}


// ********************************************************
//
// v a l i d a t e
//
// ********************************************************
int OgmlParser::validate(const XmlTagObject * xmlTag, int ogmlTagId)
{

	OgmlTag *ogmlTag = &s_tags->lookup(ogmlTagId)->info();
	ListConstIterator < OgmlTag * > it;
	XmlTagObject *sonTag;
	int valid;

	// Perhaps xmlTag is already valid
	if(xmlTag->valid())
		return valid = Ogml::vs_valid;

	if(!ogmlTag) {
		cerr << "Didn't found tag with id \"" << ogmlTagId << "\" in hashtable in OgmlParser::validate! Aborting.\n";
		return false;
	}

	if((valid = ogmlTag->validTag(*xmlTag, m_ids)) < 0) {
		this->printValidityInfo(*ogmlTag, *xmlTag, valid, __LINE__);
		return valid;
	}

	// if tag ignores its content simply return
	if(ogmlTag->ignoresContent()) {
		xmlTag->setValid();
#ifdef OGDF_DEBUG
		this->printValidityInfo(*ogmlTag, *xmlTag, valid = Ogml::vs_valid, __LINE__);
#endif
		return valid = Ogml::vs_valid;
	}

	// Check if all required son tags exist
	if(ogmlTag->ownsCompulsiveTags())
	{
		// find all obligatoric sons: all obligatoric sons
		for (it = ogmlTag->getCompulsiveTags().begin(); it.valid(); it++)
		{
			int cnt = 0;

			// search for untested sons
			sonTag = xmlTag->m_pFirstSon;
			while(sonTag) {
				if(sonTag->getName() == (**it).getName()) {
					cnt++;
					if((valid = validate(sonTag, (**it).getId())) < 0)
						return valid;
				}
				sonTag = sonTag->m_pBrother;
			}

			// Exp. son not found
			if(cnt == 0) {
				this->printValidityInfo(*ogmlTag, *xmlTag, valid = Ogml::vs_expTagNotFound, __LINE__);
				return valid;
			}

			// Check cardinality
			if(cnt < (**it).getMinOccurs() || cnt > (**it).getMaxOccurs()) {
				this->printValidityInfo((**it), *xmlTag, valid = Ogml::vs_cardErr, __LINE__);
				return valid;
			}
		}
	}

	// Check if choice son tags exist
	if(ogmlTag->ownsChoiceTags())
	{
		bool tookChoice = false;

		// find all obligatoric sons: all obligatoric sons
		for (it = ogmlTag->getChoiceTags().begin(); it.valid(); it++)
		{
			int cnt = 0;

			// search for untested sons
			sonTag = xmlTag->m_pFirstSon;
			while(sonTag) {
				if(sonTag->getName() == (**it).getName()) {
					if((valid = validate(sonTag, (**it).getId())) < 0)
						return valid;
					tookChoice = true;
					cnt++;
				}
				sonTag = sonTag->m_pBrother;
			}

			// Check cardinality
			if(cnt > 0 && (cnt < (**it).getMinOccurs()
				|| cnt > (**it).getMaxOccurs())) {
					this->printValidityInfo((**it), *xmlTag, valid = Ogml::vs_cardErr, __LINE__);
					return valid;
			}

		}
		if ((!tookChoice) && (xmlTag->m_pFirstSon)) {
			this->printValidityInfo((**it), *xmlTag, valid = Ogml::vs_tagEmptIncl, __LINE__);
			return valid;
		}

	}	//Check choice son tags

	// Check if opt son tags exist
	if(ogmlTag->ownsOptionalTags())
	{
		// find all obligatoric sons: all obligatoric sons
		for (it = ogmlTag->getOptionalTags().begin(); it.valid(); ++it)
		{
			int cnt = 0;

			// search for untested sons
			sonTag = xmlTag->m_pFirstSon;
			while(sonTag) {

				if(sonTag->getName() == (**it).getName()) {
					if((valid = validate(sonTag, (**it).getId())) < 0)
						return valid;
					cnt++;
				}
				sonTag = sonTag->m_pBrother;
			}

			// Check cardinality
			// if( (cnt<(**it).getMinOccurs() || cnt>(**it).getMaxOccurs()) ) {
			if(cnt > (**it).getMaxOccurs()) {
				this->printValidityInfo((**it), *xmlTag, valid = Ogml::vs_cardErr, __LINE__);
				return valid;
			}
		}
	}

	// Are there invalid son tags left?
	sonTag = xmlTag->m_pFirstSon;
	while(sonTag) {
		// tag already valid
		if(!sonTag->valid()) {
			this->printValidityInfo(*ogmlTag, *xmlTag, valid = Ogml::vs_unexpTag, __LINE__);
			return valid;
		}
		sonTag = sonTag->m_pBrother;
	}

	// Finally xmlTag is valid :-)
	xmlTag->setValid();

#ifdef OGDF_DEBUG
	this->printValidityInfo(*ogmlTag, *xmlTag, valid = Ogml::vs_valid, __LINE__);
#endif

	return Ogml::vs_valid;
}


//
// v a l i d a t e
//
void OgmlParser::validate(const char *fileName)
{
	DinoXmlParser p(fileName);
	p.createParseTree();

	const XmlTagObject *root = &p.getRootTag();
	buildHashTables();
	validate(root, Ogml::t_ogml);
}


//
// o p e r a t o r < <
//
ostream& operator<<(ostream& os, const OgmlParser::OgmlAttribute& oa)
{
	oa.print(os);
	return os;
}

//
// o p e r a t o r < <
//
ostream& operator<<(ostream& os, const OgmlParser::OgmlTag& ot)
{
	ot.printOwnedTags(os);
	ot.printOwnedAttributes(os);
	return os;
}


// ***********************************************************
//
// p r i n t V a l i d i t y I n f o
//
// ***********************************************************
void OgmlParser::printValidityInfo(const OgmlTag & ot, const XmlTagObject & xto, int valStatus, int line)
{
	const String &ogmlTagName = ot.getName();

	switch (valStatus) {

	case Ogml::vs_tagEmptIncl:
		cerr << "ERROR: tag \"<" << ogmlTagName << ">\" expects tag(s) to include! ";
		cerr << "(Input source line: " << xto.
			getLine() << ", recursion depth: " << xto.getDepth() << ")\n";
		ot.printOwnedTags(cerr);
		break;

	case Ogml::vs_idNotUnique:
		cerr << "ERROR: tag \"<" << ogmlTagName << ">\" owns already assigned id! ";
		cerr << "(Input source line: " << xto.
			getLine() << ", recursion depth: " << xto.getDepth() << ")\n";
		break;

	case Ogml::vs_idRefErr:
		cerr << "ERROR: tag \"<" << ogmlTagName << ">\" references unknown or wrong id! ";
		cerr << "(Input source line: " << xto.
			getLine() << ", recursion depth: " << xto.getDepth() << ")\n";
		break;

	case Ogml::vs_unexpTag:
		cerr << "ERROR: tag \"<" << ogmlTagName << ">\" owns unexpected tag! ";
		cerr << "(Input source line: " << xto.
			getLine() << ", recursion depth: " << xto.getDepth() << ")\n";
		ot.printOwnedTags(cerr);
		break;

	case Ogml::vs_unexpAtt:
		cerr << "ERROR: tag \"<" << ogmlTagName << ">\" owns unexpected attribute(s)! ";
		cerr << "(Input source line: " << xto.
			getLine() << ", recursion depth: " << xto.getDepth() << ")\n";
		ot.printOwnedAttributes(cerr);
		break;

	case Ogml::vs_expTagNotFound:
		cerr << "ERROR: tag \"<" << ogmlTagName << ">\" doesn't own compulsive tag(s)! ";
		cerr << "(Input source line: " << xto.
			getLine() << ", recursion depth: " << xto.getDepth() << ")\n";
		ot.printOwnedTags(cerr);
		break;

	case Ogml::vs_expAttNotFound:
		cerr << "ERROR: tag \"<" << ogmlTagName << ">\" doesn't own compulsive attribute(s)! ";
		cerr << "(Input source line: " << xto.
			getLine() << ", recursion depth: " << xto.getDepth() << ")\n";
		ot.printOwnedAttributes(cerr);
		break;

	case Ogml::vs_attValueErr:
		cerr << "ERROR: tag \"<" << ogmlTagName << ">\" owns attribute with wrong value! ";
		cerr << "(Input source line: " << xto.
			getLine() << ", recursion depth: " << xto.getDepth() << ")\n";
		ot.printOwnedAttributes(cerr);
		break;

	case Ogml::vs_cardErr:
		cerr << "ERROR: tag \"<" << ogmlTagName <<
			">\" occurence exceeds the number of min. (" << ot.
			getMinOccurs() << ") or max. (" << ot.getMaxOccurs() << ") occurences in its context! ";
		cerr << "(Input source line: " << xto.
			getLine() << ", recursion depth: " << xto.getDepth() << ")\n";
		break;

	case Ogml::vs_invalid:
		cerr << "ERROR: tag \"<" << ogmlTagName << ">\" is invalid! No further information available. ";
		cerr << "(Input source line: " << xto.
			getLine() << ", recursion depth: " << xto.getDepth() << ")\n";
		cerr << ot;
		break;

	case Ogml::vs_valid:
		//cout << "INFO: tag \"<" << ogmlTagName << ">\" is valid :-) ";
		//cout << "(Input source line: " << xto.
		//	getLine() << ", recursion depth: " << xto.getDepth() << ")\n";
		break;
	}

#ifdef OGDF_DEBUG
	if(valStatus != Ogml::vs_valid)
		cout << "(Line OgmlParser::validate: " << line << ")\n";
#endif
}



// ***********************************************************
//
// i s G r a p h H i e r a r c h i c a l
//
// ***********************************************************
bool OgmlParser::isGraphHierarchical(const XmlTagObject *xmlTag) const
{
	if(xmlTag->getName() == Ogml::s_tagNames[Ogml::t_node] && isNodeHierarchical(xmlTag))
		return true;

	// Depth-Search only if ret!=true
	if(xmlTag->m_pFirstSon && isGraphHierarchical(xmlTag->m_pFirstSon))
		return true;

	// Breadth-Search only if ret!=true
	if(xmlTag->m_pBrother && isGraphHierarchical(xmlTag->m_pBrother))
		return true;

	return false;
}



// ***********************************************************
//
// i s N o d e H i e r a r c h i c a l
//
// ***********************************************************
bool OgmlParser::isNodeHierarchical(const XmlTagObject *xmlTag) const
{
	bool ret = false;
	if(xmlTag->getName() == Ogml::s_tagNames[Ogml::t_node]) {

		XmlTagObject* dum;
		// check if an ancestor is a node
		ret = xmlTag->findSonXmlTagObjectByName(Ogml::s_tagNames[Ogml::t_node], dum);
	}

	return ret;
}



// ***********************************************************
//
// c h e c k G r a p h T y p e
//
// ***********************************************************
bool OgmlParser::checkGraphType(const XmlTagObject *xmlTag) const
{
	if(xmlTag->getName() != Ogml::s_tagNames[Ogml::t_ogml]) {
		cerr << "ERROR: Expecting root tag \"" << Ogml::s_tagNames[Ogml::t_ogml]	<< "\" in OgmlParser::checkGraphType!\n";
		return false;
	}

	// Normal graph present
	if(!isGraphHierarchical(xmlTag)) {
		m_graphType = Ogml::graph;
		return true;
	}

	// Cluster-/Compound graph present
	m_graphType = Ogml::clusterGraph;

	// Traverse the parse tree and collect all edge tags
	List<const XmlTagObject*> edges;
	if(xmlTag->getName() == Ogml::s_tagNames[Ogml::t_edge]) edges.pushBack(xmlTag);
	XmlTagObject* son = xmlTag->m_pFirstSon;
	while(son) {
		if(son->getName() == Ogml::s_tagNames[Ogml::t_edge]) edges.pushBack(son);
		son = son->m_pBrother;
	}

	// Cluster graph already present
	if(edges.empty()) return true;

	// Traverse edges
	ListConstIterator<const XmlTagObject*> edgeIt;
	for(edgeIt = edges.begin(); edgeIt.valid() && m_graphType != Ogml::compoundGraph; edgeIt++)
	{
		// Traverse the sources/targets
		son = (*edgeIt)->m_pFirstSon;

		// Parse tree is valid so one edge contains at least one source/target
		// with idRef attribute
		while(son) {
			XmlAttributeObject* att;
			if(son->findXmlAttributeObjectByName(Ogml::s_attributeNames[Ogml::a_nodeIdRef], att)) {
				const XmlTagObject *refTag = m_ids.lookup(att->getValue())->info();
				if(isNodeHierarchical(refTag)) {
					m_graphType = Ogml::compoundGraph;
					break;
				}
			}
			son = son->m_pBrother;
		}
	}

	return true;
}



// ***********************************************************
//
// a u x i l i a r y    m e t h o d s
//
// ***********************************************************
//   => Mapping of OGML to OGDF <=


// Mapping Brush Pattern
int OgmlParser::getBrushPatternAsInt(String s)
{
	if (s == Ogml::s_attributeValueNames[Ogml::av_bpNone])
		return 0;
	if (s == Ogml::s_attributeValueNames[Ogml::av_bpSolid])
		return 1;
	if (s == Ogml::s_attributeValueNames[Ogml::av_bpDense1])
		return 2;
	if (s == Ogml::s_attributeValueNames[Ogml::av_bpDense2])
		return 3;
	if (s == Ogml::s_attributeValueNames[Ogml::av_bpDense3])
		return 4;
	if (s == Ogml::s_attributeValueNames[Ogml::av_bpDense4])
		return 5;
	if (s == Ogml::s_attributeValueNames[Ogml::av_bpDense5])
		return 6;
	if (s == Ogml::s_attributeValueNames[Ogml::av_bpDense6])
		return 7;
	if (s == Ogml::s_attributeValueNames[Ogml::av_bpDense7])
		return 8;
	if (s == Ogml::s_attributeValueNames[Ogml::av_bpHorizontal])
		return 9;
	if (s == Ogml::s_attributeValueNames[Ogml::av_bpVertical])
		return 10;
	if (s == Ogml::s_attributeValueNames[Ogml::av_bpCross])
		return 11;
	if (s == Ogml::s_attributeValueNames[Ogml::av_bpBackwardDiagonal])
		return 12;
	if (s == Ogml::s_attributeValueNames[Ogml::av_bpForwardDiagonal])
		return 13;
	if (s == Ogml::s_attributeValueNames[Ogml::av_bpDiagonalCross])
		return 14;
	// default return bpSolid
	return 1;
}


// Mapping Shape to Integer
int OgmlParser::getShapeAsInt(String s)
{
	//TODO: has to be completed if other shape types are implemented!!!
	// SMALL PROBLEM: OGML DOESN'T SUPPORT SHAPES
	// -> change XSD, if necessary

	if (s=="rect" || s=="rectangle")
		return GraphAttributes::rectangle;
	// default return rectangle

	return GraphAttributes::rectangle;
}


// Mapping OgmlNodeShape to OGDF::NodeTemplate
String OgmlParser::getNodeTemplateFromOgmlValue(String s)
{
	// Mapping OGML-Values to ogdf
	// rect | triangle | circle | ellipse | hexagon | rhomb
	//	    | trapeze | upTrapeze | lParallelogram | rParallelogram | pentagon
	//		| octagon | umlClass | image
	if (s == Ogml::s_attributeValueNames[Ogml::av_rect])
		return "ogdf:std:rect";
	if (s == Ogml::s_attributeValueNames[Ogml::av_rectSimple])
		return "ogdf:std:rect simple";
	if (s == Ogml::s_attributeValueNames[Ogml::av_triangle])
		s = "ogdf:std:rect";
	if (s == Ogml::s_attributeValueNames[Ogml::av_circle])
		return "ogdf:std:ellipse";
	if (s == Ogml::s_attributeValueNames[Ogml::av_ellipse])
		return "ogdf:std:ellipse";
	if (s == Ogml::s_attributeValueNames[Ogml::av_hexagon])
		return "ogdf:std:hexagon";
	if (s == Ogml::s_attributeValueNames[Ogml::av_rhomb])
		return "ogdf:std:rect";
	if (s == Ogml::s_attributeValueNames[Ogml::av_trapeze])
		return "ogdf:std:rect";
	if (s == Ogml::s_attributeValueNames[Ogml::av_upTrapeze])
		return "ogdf:std:rect";
	if (s == Ogml::s_attributeValueNames[Ogml::av_lParallelogram])
		return "ogdf:std:rect";
	if (s == Ogml::s_attributeValueNames[Ogml::av_rParallelogram])
		return "ogdf:std:rect";
	if (s == Ogml::s_attributeValueNames[Ogml::av_pentagon])
		return "ogdf:std:rect";
	if (s == Ogml::s_attributeValueNames[Ogml::av_octagon])
		return"ogdf:std:rect";
	if (s == Ogml::s_attributeValueNames[Ogml::av_umlClass])
		return "ogdf:std:UML class";
	if (s == Ogml::s_attributeValueNames[Ogml::av_image])
		return "ogdf:std:rect";
	// default
	return "ogdf:std:rect";
}


// Mapping Line type to Integer
int OgmlParser::getLineTypeAsInt(String s)
{
	if (s == Ogml::s_attributeValueNames[Ogml::av_esNoPen])
		return 0;
	if (s == Ogml::s_attributeValueNames[Ogml::av_esSolid])
		return 1;
	if (s == Ogml::s_attributeValueNames[Ogml::av_esDash])
		return 2;
	if (s == Ogml::s_attributeValueNames[Ogml::av_esDot])
		return 3;
	if (s == Ogml::s_attributeValueNames[Ogml::av_esDashdot])
		return 4;
	if (s == Ogml::s_attributeValueNames[Ogml::av_esDashdotdot])
		return 5;
	// Mapping OGML-Values to ogdf
	// solid | dotted | dashed | double | triple
	//		 | groove | ridge | inset | outset | none
	if (s == Ogml::s_attributeValueNames[Ogml::av_solid])
		return 1;
	if (s == Ogml::s_attributeValueNames[Ogml::av_dotted])
		return 3;
	if (s == Ogml::s_attributeValueNames[Ogml::av_dashed])
		return 2;
	if (s == Ogml::s_attributeValueNames[Ogml::av_double])
		return 4;
	if (s == Ogml::s_attributeValueNames[Ogml::av_triple])
		return 5;
	if (s == Ogml::s_attributeValueNames[Ogml::av_groove])
		return 5;
	if (s == Ogml::s_attributeValueNames[Ogml::av_ridge])
		return 1;
	if (s == Ogml::s_attributeValueNames[Ogml::av_inset])
		return 1;
	if (s == Ogml::s_attributeValueNames[Ogml::av_outset])
		return 1;
	if (s == Ogml::s_attributeValueNames[Ogml::av_none])
		return 0;
	//default return bpSolid
	return 1;
}


// Mapping ArrowStyles to Integer
int OgmlParser::getArrowStyleAsInt(String s, String sot)
{
	// sot = "source" or "target", actually not necessary
	// TODO: Complete, if new arrow styles are implemented in ogdf
	if (s == "none")
		return 0;
	else
		return 1;
	// default return 0
	return 0;
}


// Mapping ArrowStyles to EdgeArrow
GraphAttributes::EdgeArrow OgmlParser::getArrowStyle(int i)
{
	switch (i){
	case 0:
		return GraphAttributes::none;
		break;
	case 1:
		return GraphAttributes::last;
		break;
	case 2:
		return GraphAttributes::first;
		break;
	case 3:
		return GraphAttributes::both;
		break;
	default:
		return GraphAttributes::last;
	}
}


// Mapping Image Style to Integer
int OgmlParser::getImageStyleAsInt(String s)
{
	if (s == Ogml::s_attributeValueNames[Ogml::av_freeScale])
		return 0;
	if (s == Ogml::s_attributeValueNames[Ogml::av_fixScale])
		return 1;
	//default return freeScale
	return 0;
}


// Mapping Image Alignment to Integer
int OgmlParser::getImageAlignmentAsInt(String s)
{
	if (s == Ogml::s_attributeValueNames[Ogml::av_topLeft])
		return 0;
	if (s == Ogml::s_attributeValueNames[Ogml::av_topCenter])
		return 1;
	if (s == Ogml::s_attributeValueNames[Ogml::av_topRight])
		return 2;
	if (s == Ogml::s_attributeValueNames[Ogml::av_centerLeft])
		return 3;
	if (s == Ogml::s_attributeValueNames[Ogml::av_center])
		return 4;
	if (s == Ogml::s_attributeValueNames[Ogml::av_centerRight])
		return 5;
	if (s == Ogml::s_attributeValueNames[Ogml::av_bottomLeft])
		return 6;
	if (s == Ogml::s_attributeValueNames[Ogml::av_bottomCenter])
		return 7;
	if (s == Ogml::s_attributeValueNames[Ogml::av_bottomRight])
		return 8;
	//default return center
	return 4;
}


// returns the string with "<" substituted for "&lt;"
//  and ">" substituted for "&gt;"
String OgmlParser::getLabelCaptionFromString(String str)
{
	String output;
	size_t i=0;
	while (i<str.length())
	{
		if (str[i] == '&')
		{
			if (i+3 < str.length())
			{
				if ((str[i+1] == 'l') && (str[i+2] == 't') && (str[i+3] == ';')){
					// found char sequence "&lt;"
					output += "<";
				} else {
					if ((str[i+1] == 'g') && (str[i+2] == 't') && (str[i+3] == ';')){
						// found char sequence "&gt;"
						// \n newline is required!!!
						output += ">\n";
					}
				}
				i = i + 4;
			}
		} else {
			char c = str[i];
			output += c;
			i++;
		}
	}
	str += "\n";
	return output;
}


// returns the integer value of the id at the end of the string - if existent
// the return value is 'id', the boolean return value is for checking existance of an integer value
//
// why do we need such a function?
// in OGML every id is globally unique, so we write a char-prefix
// to the ogdf-id's ('n' for node, 'e' for edge, ...)
bool OgmlParser::getIdFromString(String str, int &id)
{
	if (str.length() == 0)
		return false;

	String strId;
	size_t i=0;
	while (i<str.length()) {
		// if act char is a digit append it to the strId
		if (isdigit(str[i]))
			strId += str[i];
		i++;
	}

	if (strId.length() == 0)
		return false;

	// transform str to int
	id = atoi(strId.cstr());
	return true;
}


// ***********************************************************
//
// B U I L D    A T T R I B U T E D    C L U S T E R -- G R A P H
//
//
// ***********************************************************
bool OgmlParser::buildAttributedClusterGraph(
	Graph &G,
	ClusterGraphAttributes &CGA,
	const XmlTagObject *root)
{
	HashConstIterator<String, const XmlTagObject*> it;

	if(!root) {
		cout << "WARNING: can't determine layout information, no parse tree available!\n";

	} else {
		// root tag isn't a NULL pointer... let's start...
		XmlTagObject* son = root->m_pFirstSon;
		// first traverse to the structure- and the layout block
		if (son->getName() != Ogml::s_tagNames[Ogml::t_graph]){
			while (son->getName() != Ogml::s_tagNames[Ogml::t_graph]){
				son = son->m_pFirstSon;
				if (!son){
					// wrong rootTag given or graph tag wasn't found
					return false;
				}
			} //while
		} //if

		// now son is the graph tag which first child is structure
		XmlTagObject* structure = son->m_pFirstSon;
		if (structure->getName() != Ogml::s_tagNames[Ogml::t_structure]){
			return false;
		}
		// now structure is what it is meant to be
		// traverse the children of structure
		// and set the labels
		son = structure->m_pFirstSon;
		while(son)
		{
			//Set labels of nodes
			if ((son->getName() == Ogml::s_tagNames[Ogml::t_node]) && (CGA.attributes() & GraphAttributes::nodeLabel))
			{
				if (!isNodeHierarchical(son))
				{
					// get the id of the actual node
					XmlAttributeObject *att;
					if(son->findXmlAttributeObjectByName(Ogml::s_attributeNames[Ogml::a_id], att))
					{
						// lookup for node
						node actNode = (m_nodes.lookup(att->getValue()))->info();
						// find label tag
						XmlTagObject* label;
						if (son->findSonXmlTagObjectByName(Ogml::s_tagNames[Ogml::t_label], label))
						{
							// get content tag
							XmlTagObject* content = label->m_pFirstSon;
							// get the content as string
							if (content->m_pTagValue) {
								String str = content->getValue();
								String labelStr = getLabelCaptionFromString(str);
								// now set the label of the node
								CGA.labelNode(actNode) = labelStr;
							}
						}
					}
				}// "normal" nodes
				else
				{
					// get the id of the actual cluster
					XmlAttributeObject *att;
					if(son->findXmlAttributeObjectByName(Ogml::s_attributeNames[Ogml::a_id], att))
					{
						// lookup for cluster
						cluster actCluster = (m_clusters.lookup(att->getValue()))->info();
						// find label tag
						XmlTagObject* label;
						if (son->findSonXmlTagObjectByName(Ogml::s_tagNames[Ogml::t_label], label))
						{
							// get content tag
							XmlTagObject* content = label->m_pFirstSon;
							// get the content as string
							if (content->m_pTagValue) {
								String str = content->getValue();
								String labelStr = getLabelCaptionFromString(str);
								// now set the label of the node
								CGA.clusterLabel(actCluster) = labelStr;
							}
						}
					}
					// hierSon = hierarchical Son
					XmlTagObject *hierSon;
					if (son->m_pFirstSon)
					{
						hierSon = son->m_pFirstSon;
						while(hierSon) {
							// recursive call for setting labels of child nodes
							if (!setLabelsRecursive(G, CGA, hierSon))
								return false;
							hierSon = hierSon->m_pBrother;
						}
					}
				}//cluster nodes
			}// node labels

			//Set labels of edges
			if ((son->getName() == Ogml::s_tagNames[Ogml::t_edge]) && (CGA.attributes() & GraphAttributes::edgeLabel))
			{
				// get the id of the actual edge
				XmlAttributeObject *att;
				if (son->findXmlAttributeObjectByName(Ogml::s_attributeNames[Ogml::a_id], att))
				{
					// lookup for edge
					//  0, if (hyper)edge not read from file
					if(m_edges.lookup(att->getValue())){
						edge actEdge = (m_edges.lookup(att->getValue()))->info();
						// find label tag
						XmlTagObject* label;
						if(son->findSonXmlTagObjectByName(Ogml::s_tagNames[Ogml::t_label], label))
						{
							// get content tag
							XmlTagObject* content = label->m_pFirstSon;
							// get the content as string
							if (content->m_pTagValue) {
								String str = content->getValue();
								String labelStr = getLabelCaptionFromString(str);
								// now set the label of the node
								CGA.labelEdge(actEdge) = labelStr;
							}
						}
					}
				}
			}// edge labels

			// Labels
			// ACTUALLY NOT IMPLEMENTED IN OGDF
			//if (son->getName() == Ogml::s_tagNames[t_label]) {
			// get the id of the actual edge
			//XmlAttributeObject *att;
			//if (son->findXmlAttributeObjectByName(Ogml::s_attributeNames[Ogml::a_id], att)){
			// lookup for label
			//label actLabel = (labels.lookup(att->getValue()))->info();
			// get content tag
			//XmlTagObject* content = son->m_pFirstSon;
			// get the content as string
			//if (content->m_pTagValue){
			//String str = content->getValue();
			//String labelStr = getLabelCaptionFromString(str);
			//now set the label of the node
			//	CGA.labelLabel(actLabel) = labelStr;
			//}
			//}
			//}// Labels

			// go to the next brother
			son = son->m_pBrother;
		}// while(son) // son <=> children of structure

		// get the layout tag
		XmlTagObject* layout;
		if (structure->m_pBrother != NULL) {
			layout = structure->m_pBrother;
		}

		if ((layout) && (layout->getName() == Ogml::s_tagNames[Ogml::t_layout]))
		{
			// layout exists

			// first get the styleTemplates
			XmlTagObject *layoutSon;
			if (layout->m_pFirstSon)
			{
				// layout has at least one child-tag
				layoutSon = layout->m_pFirstSon;
				// ->loop through all of them
				while (layoutSon)
				{
					// style templates
					if (layoutSon->getName() == Ogml::s_tagNames[Ogml::t_styleTemplates])
					{
						// has children data, nodeStyleTemplate, edgeStyleTemplate, labelStyleTemplate
						XmlTagObject *styleTemplatesSon;
						if (layoutSon->m_pFirstSon)
						{
							styleTemplatesSon = layoutSon->m_pFirstSon;

							while (styleTemplatesSon)
							{
								// nodeStyleTemplate
								if (styleTemplatesSon->getName() == Ogml::s_tagNames[Ogml::t_nodeStyleTemplate])
								{
									XmlAttributeObject *actAtt;
									if (styleTemplatesSon->findXmlAttributeObjectByName(Ogml::s_attributeNames[Ogml::a_id], actAtt))
									{
										const String &actKey = actAtt->getValue();
										OgmlNodeTemplate *actTemplate = new OgmlNodeTemplate(actKey); // when will this be deleted?

										XmlTagObject *actTag;

										// template inheritance
										if (styleTemplatesSon->findSonXmlTagObjectByName(Ogml::s_tagNames[Ogml::t_nodeStyleTemplateRef], actTag))
										{
											if (actTag->findXmlAttributeObjectByName(Ogml::s_attributeNames[Ogml::a_nodeStyleTemplateIdRef], actAtt)) {
												// actual template references another
												// get it from the hash table
												OgmlNodeTemplate *refTemplate = m_ogmlNodeTemplates.lookup(actAtt->getValue())->info();
												if (refTemplate) {
													// the referenced template was inserted into the hash table
													// so copy the values
													String actId = actTemplate->m_id;
													*actTemplate = *refTemplate;
													actTemplate->m_id = actId;
												}
											}
										}// template inheritance

										//				// data
										//				if (styleTemplatesSon->findSonXmlTagObjectByName(Ogml::s_tagNames[t_data], actTag)){
										//					// found data for nodeStyleTemplate
										//					// no implementation required for ogdf
										//				}// data

										// shape tag
										if (styleTemplatesSon->findSonXmlTagObjectByName(Ogml::s_tagNames[Ogml::t_shape], actTag))
										{
											// type
											if (actTag->findXmlAttributeObjectByName(Ogml::s_attributeNames[Ogml::a_nShapeType], actAtt)) {
												// TODO: change, if shapes are expanded
												// actually shape and template are calculated from the same value!!!
												actTemplate->m_nodeTemplate = getNodeTemplateFromOgmlValue(actAtt->getValue());
												actTemplate->m_shapeType = getShapeAsInt(actAtt->getValue());
											}
											// width
											if (actTag->findXmlAttributeObjectByName(Ogml::s_attributeNames[Ogml::a_width], actAtt))
												actTemplate->m_width = atof(actAtt->getValue().cstr());
											// height
											if (actTag->findXmlAttributeObjectByName(Ogml::s_attributeNames[Ogml::a_height], actAtt))
												actTemplate->m_height = atof(actAtt->getValue().cstr());
											// uri
											//ACTUALLY NOT SUPPORTED
											//if (actTag->findXmlAttributeObjectByName(Ogml::s_attributeNames[a_uri], actAtt))
											//	CGA.uri(actNode) = actAtt->getValue();
										}// shape

										// fill tag
										if (styleTemplatesSon->findSonXmlTagObjectByName(Ogml::s_tagNames[Ogml::t_fill], actTag))
										{
											// fill color
											if (actTag->findXmlAttributeObjectByName(Ogml::s_attributeNames[Ogml::a_color], actAtt))
												actTemplate->m_color = actAtt->getValue();
											// fill pattern
											if (actTag->findXmlAttributeObjectByName(Ogml::s_attributeNames[Ogml::a_pattern], actAtt))
												actTemplate->m_pattern = GraphAttributes::intToPattern(getBrushPatternAsInt(actAtt->getValue()));
											// fill patternColor
											//TODO: check if pattern color exists
											//if (actTag->findXmlAttributeObjectByName(Ogml::s_attributeNames[a_patternColor], actAtt));
											//	actTemplate->m_patternColor = actAtt->getValue());
										}// fill

										// line tag
										if (styleTemplatesSon->findSonXmlTagObjectByName(Ogml::s_tagNames[Ogml::t_line], actTag))
										{
											// type
											if (actTag->findXmlAttributeObjectByName(Ogml::s_attributeNames[Ogml::a_nLineType], actAtt))
												actTemplate->m_lineType = GraphAttributes::intToStyle(getLineTypeAsInt(actAtt->getValue()));
											// width
											if (actTag->findXmlAttributeObjectByName(Ogml::s_attributeNames[Ogml::a_width], actAtt))
												actTemplate->m_lineWidth = atof(actAtt->getValue().cstr());
											// color
											if (actTag->findXmlAttributeObjectByName(Ogml::s_attributeNames[Ogml::a_color], actAtt))
												actTemplate->m_lineColor = actAtt->getValue();
										}// line

										//insert actual template into hash table
										m_ogmlNodeTemplates.fastInsert(actKey, actTemplate);
									}
								}//nodeStyleTemplate

								// edgeStyleTemplate
								if (styleTemplatesSon->getName() == Ogml::s_tagNames[Ogml::t_edgeStyleTemplate])
								{
									XmlAttributeObject *actAtt;
									if (styleTemplatesSon->findXmlAttributeObjectByName(Ogml::s_attributeNames[Ogml::a_id], actAtt))
									{
										const String &actKey = actAtt->getValue();
										OgmlEdgeTemplate *actTemplate = new OgmlEdgeTemplate(actKey); // when will this be deleted?

										XmlTagObject *actTag;

										// template inheritance
										if (styleTemplatesSon->findSonXmlTagObjectByName(Ogml::s_tagNames[Ogml::t_edgeStyleTemplateRef], actTag)){
											if (actTag->findXmlAttributeObjectByName(Ogml::s_attributeNames[Ogml::a_edgeStyleTemplateIdRef], actAtt)){
												// actual template references another
												// get it from the hash table
												OgmlEdgeTemplate *refTemplate = m_ogmlEdgeTemplates.lookup(actAtt->getValue())->info();
												if (refTemplate){
													// the referenced template was inserted into the hash table
													// so copy the values
													String actId = actTemplate->m_id;
													*actTemplate = *refTemplate;
													actTemplate->m_id = actId;
												}
											}
										}// template inheritance

										//	// data
										//	if (styleTemplatesSon->findSonXmlTagObjectByName(Ogml::s_tagNames[t_data], actTag)){
										//		// found data for edgeStyleTemplate
										//		// no implementation required for ogdf
										//	}// data

										// line tag
										if (styleTemplatesSon->findSonXmlTagObjectByName(Ogml::s_tagNames[Ogml::t_line], actTag))
										{
											// type
											if (actTag->findXmlAttributeObjectByName(Ogml::s_attributeNames[Ogml::a_type], actAtt))
												actTemplate->m_lineType = GraphAttributes::intToStyle(getLineTypeAsInt(actAtt->getValue()));
											// width
											if (actTag->findXmlAttributeObjectByName(Ogml::s_attributeNames[Ogml::a_width], actAtt))
												actTemplate->m_lineWidth = atof(actAtt->getValue().cstr());
											// color
											if (actTag->findXmlAttributeObjectByName(Ogml::s_attributeNames[Ogml::a_color], actAtt))
												actTemplate->m_color = actAtt->getValue();
										}// line

										// sourceStyle tag
										if (styleTemplatesSon->findSonXmlTagObjectByName(Ogml::s_tagNames[Ogml::t_sourceStyle], actTag))
										{
											// type
											if (actTag->findXmlAttributeObjectByName(Ogml::s_attributeNames[Ogml::a_type], actAtt))
												actTemplate->m_sourceType = getArrowStyleAsInt(actAtt->getValue(), Ogml::s_tagNames[Ogml::t_source]);
											// color
											//if (actTag->findXmlAttributeObjectByName(Ogml::s_attributeNames[a_color], actAtt))
											//	actTemplate->m_sourceColor = actAtt->getValue();
											// size
											//if (actTag->findXmlAttributeObjectByName(Ogml::s_attributeNames[a_size], actAtt))
											//	actTemplate->m_sourceSize = atof(actAtt->getValue());
										}

										// targetStyle tag
										if (styleTemplatesSon->findSonXmlTagObjectByName(Ogml::s_tagNames[Ogml::t_targetStyle], actTag)){
											// type
											if (actTag->findXmlAttributeObjectByName(Ogml::s_attributeNames[Ogml::a_type], actAtt))
												actTemplate->m_targetType = getArrowStyleAsInt(actAtt->getValue(), Ogml::s_tagNames[Ogml::t_target]);
											// color
											//if (actTag->findXmlAttributeObjectByName(Ogml::s_attributeNames[a_color], actAtt))
											//	actTemplate->m_targetColor = actAtt->getValue();
											// size
											//if (actTag->findXmlAttributeObjectByName(Ogml::s_attributeNames[a_size], actAtt))
											//	actTemplate->m_targetSize = atof(actAtt->getValue());
										}

										//insert actual template into hash table
										m_ogmlEdgeTemplates.fastInsert(actKey, actTemplate);
									}

								}//edgeStyleTemplate

								// labelStyleTemplate
								if (styleTemplatesSon->getName() == Ogml::s_tagNames[Ogml::t_labelStyleTemplate]){
									// ACTUALLY NOT SUPPORTED
								}//labelStyleTemplate

								styleTemplatesSon = styleTemplatesSon->m_pBrother;
							}
						}
					}// styleTemplates

					//STYLES
					if (layoutSon->getName() == Ogml::s_tagNames[Ogml::t_styles])
					{
						// has children graphStyle, nodeStyle, edgeStyle, labelStyle
						XmlTagObject *stylesSon;
						if (layoutSon->m_pFirstSon)
						{
							stylesSon = layoutSon->m_pFirstSon;

							while (stylesSon)
							{
								// GRAPHSTYLE
								if (stylesSon->getName() == Ogml::s_tagNames[Ogml::t_graphStyle])
								{
									XmlAttributeObject *actAtt;
									// defaultNodeTemplate
									if (stylesSon->findXmlAttributeObjectByName(Ogml::s_attributeNames[Ogml::a_defaultNodeTemplate], actAtt))
									{
										OgmlNodeTemplate* actTemplate = m_ogmlNodeTemplates.lookup(actAtt->getValue())->info();

										//	XmlTagObject *actTag;
										//	// data
										//	if (stylesSon->findSonXmlTagObjectByName(Ogml::s_tagNames[t_data], actTag)){
										//		// found data for graphStyle
										//		// no implementation required for ogdf
										//	}// data

										// set values for ALL nodes
										node v;
										forall_nodes(v, G){

											if (CGA.attributes() & GraphAttributes::nodeType){
												CGA.templateNode(v) = actTemplate->m_nodeTemplate;
												CGA.shapeNode(v) = actTemplate->m_shapeType;
											}
											if (CGA.attributes() & GraphAttributes::nodeGraphics){
												CGA.width(v) = actTemplate->m_width;
												CGA.height(v) = actTemplate->m_height;
											}
											if (CGA.attributes() & GraphAttributes::nodeColor)
												CGA.colorNode(v) = actTemplate->m_color;
											if (CGA.attributes() & GraphAttributes::nodeStyle){
												CGA.nodePattern(v) = actTemplate->m_pattern;
												//CGA.nodePatternColor(v) = actTemplate->m_patternColor;
												CGA.styleNode(v) = actTemplate->m_lineType;
												CGA.lineWidthNode(v) = actTemplate->m_lineWidth;
												CGA.nodeLine(v) = actTemplate->m_lineColor;
											}
										}// forall_nodes
									}// defaultNodeTemplate

									//		// defaultClusterTemplate
									//		if (stylesSon->findXmlAttributeObjectByName(Ogml::s_attributeNames[a_defaultCompoundTemplate], actAtt)){
									//			//										OgmlNodeTemplate* actTemplate = m_ogmlNodeTemplates.lookup(actAtt->getValue())->info();
									//			//										// set values for ALL Cluster
									//			cluster c;
									//			forall_clusters(c, G){
									//
									//				if (CGA.attributes() & GraphAttributes::nodeType){
									//					CGA.templateCluster(c) = actTemplate->m_nodeTemplate;
									//					// no shape definition for clusters
									//					//CGA.shapeNode(c) = actTemplate->m_shapeType;
									//				}
									//				if (CGA.attributes() & GraphAttributes::nodeGraphics){
									//						CGA.clusterWidth(c) = actTemplate->m_width;
									//						CGA.clusterHeight(c) = actTemplate->m_height;
									//				}
									//				if (CGA.attributes() & GraphAttributes::nodeColor)
									//					CGA.clusterFillColor(c) = actTemplate->m_color;
									//				if (CGA.attributes() & GraphAttributes::nodeStyle){
									//					CGA.clusterFillPattern(c) = actTemplate->m_pattern;
									//					CGA.clusterBackColor(c) = actTemplate->m_patternColor;
									//					CGA.clusterLineStyle(c) = actTemplate->m_lineType;
									//					CGA.clusterLineWidth(c) = actTemplate->m_lineWidth;
									//					CGA.clusterColor(c) = actTemplate->m_lineColor;
									//				}
									//			}// forall_clusters
									//		}// defaultClusterTemplate


									// defaultEdgeTemplate
									if (stylesSon->findXmlAttributeObjectByName(Ogml::s_attributeNames[Ogml::a_defaultEdgeTemplate], actAtt))
									{
										OgmlEdgeTemplate* actTemplate = m_ogmlEdgeTemplates.lookup(actAtt->getValue())->info();

										// set values for ALL edges
										edge e;
										forall_edges(e, G)
										{
											if (CGA.attributes() & GraphAttributes::edgeStyle) {
												CGA.styleEdge(e) = actTemplate->m_lineType;
												CGA.edgeWidth(e) = actTemplate->m_lineWidth;
											}
											if (CGA.attributes() & GraphAttributes::edgeColor) {
												CGA.colorEdge(e) = actTemplate->m_color;
											}

											//edgeArrow
											if ((CGA.attributes()) & (GraphAttributes::edgeArrow))
											{
												if (actTemplate->m_sourceType == 0) {
													if (actTemplate->m_targetType == 0) {
														// source = no_arrow, target = no_arrow // =>none
														CGA.arrowEdge(e) = GraphAttributes::none;
													}
													else {
														// source = no_arrow, target = arrow // =>last
														CGA.arrowEdge(e) = GraphAttributes::last;
													}
												}
												else {
													if (actTemplate->m_targetType == 0){
														// source = arrow, target = no_arrow // =>first
														CGA.arrowEdge(e) = GraphAttributes::first;
													}
													else {
														// source = arrow, target = arrow // =>both
														CGA.arrowEdge(e) = GraphAttributes::both;
													}
												}
											}//edgeArrow
										}//forall_edges
									}//defaultEdgeTemplate

									// defaultLabelTemplate
									//if (stylesSon->findXmlAttributeObjectByName(Ogml::s_attributeNames[a_defaultLabelTemplate], actAtt)){
									//	// set values for ALL labels
									//	// ACTUALLY NOT IMPLEMENTED
									//  label l;
									//  forall_labels(l, G){
									//
									//	}
									//}//defaultLabelTemplate
								}// graphStyle

								// NODESTYLE
								if (stylesSon->getName() == Ogml::s_tagNames[Ogml::t_nodeStyle])
								{
									// get the id of the actual node
									XmlAttributeObject *att;
									if(stylesSon->findXmlAttributeObjectByName(Ogml::s_attributeNames[Ogml::a_nodeIdRef], att))
									{
										// check if referenced id is a node or a cluster/compound
										if (m_nodes.lookup(att->getValue()))
										{
											// lookup for node
											node actNode = (m_nodes.lookup(att->getValue()))->info();

											// actTag is the actual tag that is considered
											XmlTagObject* actTag;
											XmlAttributeObject *actAtt;

											//			// data
											//			if (stylesSon->findSonXmlTagObjectByName(Ogml::s_tagNames[t_data], actTag)){
											//				// found data for nodeStyle
											//				// no implementation required for ogdf
											//			}// data

											// check if actual nodeStyle references a template
											if (stylesSon->findSonXmlTagObjectByName(Ogml::s_tagNames[Ogml::t_nodeStyleTemplateRef], actTag))
											{
												// get referenced template id
												if (actTag->findXmlAttributeObjectByName(Ogml::s_attributeNames[Ogml::a_nodeStyleTemplateIdRef], actAtt))
												{
													// actual nodeStyle references a template
													OgmlNodeTemplate* actTemplate = m_ogmlNodeTemplates.lookup(actAtt->getValue())->info();
													if (CGA.attributes() & GraphAttributes::nodeType) {
														CGA.templateNode(actNode) = actTemplate->m_nodeTemplate;
														CGA.shapeNode(actNode) = actTemplate->m_shapeType;
													}
													if (CGA.attributes() & GraphAttributes::nodeGraphics) {
														CGA.width(actNode) = actTemplate->m_width;
														CGA.height(actNode) = actTemplate->m_height;
													}
													if (CGA.attributes() & GraphAttributes::nodeColor)
														CGA.colorNode(actNode) = actTemplate->m_color;
													if (CGA.attributes() & GraphAttributes::nodeStyle) {
														CGA.nodePattern(actNode) = actTemplate->m_pattern;
														//CGA.nodePatternColor(actNode) = actTemplate->m_patternColor;
														CGA.styleNode(actNode) = actTemplate->m_lineType;
														CGA.lineWidthNode(actNode) = actTemplate->m_lineWidth;
														CGA.nodeLine(actNode) = actTemplate->m_lineColor;
													}
												}
											}//template

											// Graph::nodeType
											//TODO: COMPLETE, IF NECESSARY
											CGA.type(actNode) = Graph::vertex;

											// location tag
											if ((stylesSon->findSonXmlTagObjectByName(Ogml::s_tagNames[Ogml::t_location], actTag))
												&& (CGA.attributes() & GraphAttributes::nodeGraphics))
											{
												// set location of node
												// x
												if (actTag->findXmlAttributeObjectByName(Ogml::s_attributeNames[Ogml::a_x], actAtt))
													CGA.x(actNode) = atof(actAtt->getValue().cstr());
												// y
												if (actTag->findXmlAttributeObjectByName(Ogml::s_attributeNames[Ogml::a_y], actAtt))
													CGA.y(actNode) = atof(actAtt->getValue().cstr());
												// z
												//if (actTag->findXmlAttributeObjectByName(Ogml::s_attributeNames[a_x], actAtt))
												//CGA.z(actNode) = atof(actAtt->getValue());
											}// location

											// shape tag
											if ((stylesSon->findSonXmlTagObjectByName(Ogml::s_tagNames[Ogml::t_shape], actTag))
												&& (CGA.attributes() & GraphAttributes::nodeType))
											{
												// set shape of node
												// type
												if (actTag->findXmlAttributeObjectByName(Ogml::s_attributeNames[Ogml::a_nShapeType], actAtt)) {
													CGA.templateNode(actNode) = getNodeTemplateFromOgmlValue(actAtt->getValue());
													// TODO: change, if shapes are expanded
													// actually shape and template are calculated from the same value!!!
													CGA.shapeNode(actNode) = getShapeAsInt(actAtt->getValue());
												}
												// width
												if (actTag->findXmlAttributeObjectByName(Ogml::s_attributeNames[Ogml::a_width], actAtt))
													CGA.width(actNode) = atof(actAtt->getValue().cstr());
												// height
												if (actTag->findXmlAttributeObjectByName(Ogml::s_attributeNames[Ogml::a_height], actAtt))
													CGA.height(actNode) = atof(actAtt->getValue().cstr());
												// uri
												//ACTUALLY NOT SUPPORTED
												//if (actTag->findXmlAttributeObjectByName(Ogml::s_attributeNames[a_uri], actAtt))
												//	CGA.uri(actNode) = actAtt->getValue();
											}// shape

											// fill tag
											if ((stylesSon->findSonXmlTagObjectByName(Ogml::s_tagNames[Ogml::t_fill], actTag))
												&& (CGA.attributes() & GraphAttributes::nodeStyle))
											{
												// fill color
												if (actTag->findXmlAttributeObjectByName(Ogml::s_attributeNames[Ogml::a_color], actAtt))
													CGA.colorNode(actNode) = actAtt->getValue();
												// fill pattern
												if (actTag->findXmlAttributeObjectByName(Ogml::s_attributeNames[Ogml::a_pattern], actAtt))
													CGA.nodePattern(actNode) = GraphAttributes::intToPattern(getBrushPatternAsInt(actAtt->getValue()));
												// fill patternColor
												//TODO: check if pattern color exists
												//if (actTag->findXmlAttributeObjectByName(Ogml::s_attributeNames[a_patternColor], actAtt))
												//	CGA.nodePatternColor(actNode) = actAtt->getValue());
											}// fill

											// line tag
											if ((stylesSon->findSonXmlTagObjectByName(Ogml::s_tagNames[Ogml::t_line], actTag))
												&& (CGA.attributes() & GraphAttributes::nodeStyle))
											{
												// type
												if (actTag->findXmlAttributeObjectByName(Ogml::s_attributeNames[Ogml::a_nLineType], actAtt))
													CGA.styleNode(actNode) = GraphAttributes::intToStyle(getLineTypeAsInt(actAtt->getValue()));
												// width
												if (actTag->findXmlAttributeObjectByName(Ogml::s_attributeNames[Ogml::a_width], actAtt))
													CGA.lineWidthNode(actNode) = atof(actAtt->getValue().cstr());
												// color
												if (actTag->findXmlAttributeObjectByName(Ogml::s_attributeNames[Ogml::a_color], actAtt))
													CGA.nodeLine(actNode) = actAtt->getValue().cstr();
											}// line

											//			// ports
											//			// go through all ports with dummy tagObject port
											//			XmlTagObject* port = stylesSon->m_pFirstSon;
											//			while(port){
											//				if (port->getName() == ogmlTagObjects[t_port]){
											//					// TODO: COMPLETE
											//					// ACTUALLY NOT IMPLEMENTED IN OGDF
											//				}
											//
											//				// go to next tag
											//				port = port->m_pBrother;
											//			}

										}
										else

											// CLUSTER NODE STYLE
										{
											// get the id of the cluster/compound
											XmlAttributeObject *att;
											if(stylesSon->findXmlAttributeObjectByName(Ogml::s_attributeNames[Ogml::a_nodeIdRef], att))
											{
												// lookup for node
												cluster actCluster = (m_clusters.lookup(att->getValue()))->info();
												// actTag is the actual tag that is considered
												XmlTagObject* actTag;
												XmlAttributeObject *actAtt;

												//				// data
												//				if (stylesSon->findSonXmlTagObjectByName(Ogml::s_tagNames[t_data], actTag)){
												//					// found data for nodeStyle (CLuster/Compound)
												//					// no implementation required for ogdf
												//				}// data

												// check if actual nodeStyle (equal to cluster) references a template
												if (stylesSon->findSonXmlTagObjectByName(Ogml::s_tagNames[Ogml::t_nodeStyleTemplateRef], actTag))
												{
													// get referenced template id
													if (actTag->findXmlAttributeObjectByName(Ogml::s_attributeNames[Ogml::a_nodeStyleTemplateIdRef], actAtt))
													{
														// actual nodeStyle references a template
														OgmlNodeTemplate* actTemplate = m_ogmlNodeTemplates.lookup(actAtt->getValue())->info();
														if (CGA.attributes() & GraphAttributes::nodeType) {
															CGA.templateCluster(actCluster) = actTemplate->m_nodeTemplate;
															// no shape definition for clusters
															//CGA.shapeNode(actCluster) = actTemplate->m_shapeType;
														}
														if (CGA.attributes() & GraphAttributes::nodeGraphics) {
															CGA.clusterWidth(actCluster) = actTemplate->m_width;
															CGA.clusterHeight(actCluster) = actTemplate->m_height;
														}
														if (CGA.attributes() & GraphAttributes::nodeColor)
															CGA.clusterFillColor(actCluster) = actTemplate->m_color;
														if (CGA.attributes() & GraphAttributes::nodeStyle) {
															CGA.clusterFillPattern(actCluster) = actTemplate->m_pattern;
															CGA.clusterBackColor(actCluster) = actTemplate->m_patternColor;
															CGA.clusterLineStyle(actCluster) = actTemplate->m_lineType;
															CGA.clusterLineWidth(actCluster) = actTemplate->m_lineWidth;
															CGA.clusterColor(actCluster) = actTemplate->m_lineColor;
														}
													}
												}//template

												// Graph::nodeType
												//TODO: COMPLETE, IF NECESSARY
												// not supported for clusters!!!
												//CGA.type(actCluster) = Graph::vertex;

												// location tag
												if ((stylesSon->findSonXmlTagObjectByName(Ogml::s_tagNames[Ogml::t_location], actTag))
													&& (CGA.attributes() & GraphAttributes::nodeGraphics))
												{
													// set location of node
													// x
													if (actTag->findXmlAttributeObjectByName(Ogml::s_attributeNames[Ogml::a_x], actAtt))
														CGA.clusterXPos(actCluster) = atof(actAtt->getValue().cstr());
													// y
													if (actTag->findXmlAttributeObjectByName(Ogml::s_attributeNames[Ogml::a_y], actAtt))
														CGA.clusterYPos(actCluster) = atof(actAtt->getValue().cstr());
													// z
													//if (actTag->findXmlAttributeObjectByName(Ogml::s_attributeNames[a_x], actAtt))
													//CGA.clusterZPos(actCluster) = atof(actAtt->getValue());
												}// location

												// shape tag
												if ((stylesSon->findSonXmlTagObjectByName(Ogml::s_tagNames[Ogml::t_shape], actTag))
													&& (CGA.attributes() & GraphAttributes::nodeType))
												{
													// set shape of node
													// type
													if (actTag->findXmlAttributeObjectByName(Ogml::s_attributeNames[Ogml::a_nShapeType], actAtt)) {
														CGA.templateCluster(actCluster) = getNodeTemplateFromOgmlValue(actAtt->getValue().cstr());
														// no shape definition for clusters
														//CGA.shapeNode(actCluster) = getShapeAsInt(actAtt->getValue());
													}
													// width
													if (actTag->findXmlAttributeObjectByName(Ogml::s_attributeNames[Ogml::a_width], actAtt))
														CGA.clusterWidth(actCluster) = atof(actAtt->getValue().cstr());
													// height
													if (actTag->findXmlAttributeObjectByName(Ogml::s_attributeNames[Ogml::a_height], actAtt))
														CGA.clusterHeight(actCluster) = atof(actAtt->getValue().cstr());
													// uri
													//ACTUALLY NOT SUPPORTED
													//if (actTag->findXmlAttributeObjectByName(Ogml::s_attributeNames[a_uri], actAtt))
													//	CGA.uriCluster(actCluster) = actAtt->getValue();
												}// shape

												// fill tag
												if ((stylesSon->findSonXmlTagObjectByName(Ogml::s_tagNames[Ogml::t_fill], actTag))
													&& (CGA.attributes() & GraphAttributes::nodeStyle))
												{
													// fill color
													if (actTag->findXmlAttributeObjectByName(Ogml::s_attributeNames[Ogml::a_color], actAtt))
														CGA.clusterFillColor(actCluster) = actAtt->getValue().cstr();
													// fill pattern
													if (actTag->findXmlAttributeObjectByName(Ogml::s_attributeNames[Ogml::a_pattern], actAtt))
														CGA.clusterFillPattern(actCluster) = GraphAttributes::intToPattern(getBrushPatternAsInt(actAtt->getValue()));
													// fill patternColor
													if (actTag->findXmlAttributeObjectByName(Ogml::s_attributeNames[Ogml::a_patternColor], actAtt))
														CGA.clusterBackColor(actCluster) = actAtt->getValue().cstr();
												}// fill

												// line tag
												if ((stylesSon->findSonXmlTagObjectByName(Ogml::s_tagNames[Ogml::t_line], actTag))
													&& (CGA.attributes() & GraphAttributes::nodeStyle))
												{
													// type
													if (actTag->findXmlAttributeObjectByName(Ogml::s_attributeNames[Ogml::a_nLineType], actAtt))
														CGA.clusterLineStyle(actCluster) = GraphAttributes::intToStyle(getLineTypeAsInt(actAtt->getValue()));
													// width
													if (actTag->findXmlAttributeObjectByName(Ogml::s_attributeNames[Ogml::a_width], actAtt))
														CGA.clusterLineWidth(actCluster) = atof(actAtt->getValue().cstr());
													// color
													if (actTag->findXmlAttributeObjectByName(Ogml::s_attributeNames[Ogml::a_color], actAtt))
														CGA.clusterColor(actCluster) = actAtt->getValue();
												}// line


												//				// ports
												//				// go through all ports with dummy tagObject port
												//				XmlTagObject* port = stylesSon->m_pFirstSon;
												//				while(port){
												//					if (port->getName() == ogmlTagObjects[t_port]){
												//						// TODO: COMPLETE
												//						// no implementation required for ogdf
												//					}
												//
												//					// go to next tag
												//					port = port->m_pBrother;
												//				}

											}//nodeIdRef (with cluster)

										}// nodeStyle for cluster
									}//nodeIdRef

								}//nodeStyle

								// EDGESTYLE
								if (stylesSon->getName() == Ogml::s_tagNames[Ogml::t_edgeStyle])
								{
									// get the id of the actual edge
									XmlAttributeObject *att;
									if(stylesSon->findXmlAttributeObjectByName(Ogml::s_attributeNames[Ogml::a_edgeIdRef], att))
									{
										// lookup for edge
										edge actEdge = (m_edges.lookup(att->getValue()))->info();

										// actTag is the actual tag that is considered
										XmlTagObject* actTag;
										XmlAttributeObject *actAtt;

										//		// data
										//		if (stylesSon->findSonXmlTagObjectByName(Ogml::s_tagNames[t_data], actTag)){
										//			// found data for edgeStyle
										//			// no implementation required for ogdf
										//		}// data

										// check if actual edgeStyle references a template
										if (stylesSon->findSonXmlTagObjectByName(Ogml::s_tagNames[Ogml::t_edgeStyleTemplateRef], actTag))
										{
											// get referenced template id
											if (actTag->findXmlAttributeObjectByName(Ogml::s_attributeNames[Ogml::a_edgeStyleTemplateIdRef], actAtt))
											{
												// actual edgeStyle references a template
												OgmlEdgeTemplate* actTemplate = m_ogmlEdgeTemplates.lookup(actAtt->getValue())->info();
												if (CGA.attributes() & GraphAttributes::edgeStyle) {
													CGA.styleEdge(actEdge) = actTemplate->m_lineType;
													CGA.edgeWidth(actEdge) = actTemplate->m_lineWidth;
												}
												if (CGA.attributes() & GraphAttributes::edgeColor) {
													CGA.colorEdge(actEdge) = actTemplate->m_color;
												}

												//edgeArrow
												if ((CGA.attributes()) & (GraphAttributes::edgeArrow))
												{
													if (actTemplate->m_sourceType == 0) {
														if (actTemplate->m_targetType == 0) {
															// source = no_arrow, target = no_arrow // =>none
															CGA.arrowEdge(actEdge) = GraphAttributes::none;
														}
														else {
															// source = no_arrow, target = arrow // =>last
															CGA.arrowEdge(actEdge) = GraphAttributes::last;
														}
													}
													else {
														if (actTemplate->m_targetType == 0) {
															// source = arrow, target = no_arrow // =>first
															CGA.arrowEdge(actEdge) = GraphAttributes::first;
														}
														else {
															// source = arrow, target = arrow // =>both
															CGA.arrowEdge(actEdge) = GraphAttributes::both;
														}
													}
												}//edgeArrow

											}
										}//template

										// Graph::edgeType
										//TODO: COMPLETE, IF NECESSARY
										CGA.type(actEdge) = Graph::association;

										// line tag
										if ((stylesSon->findSonXmlTagObjectByName(Ogml::s_tagNames[Ogml::t_line], actTag))
											&& (CGA.attributes() & GraphAttributes::edgeType))
										{
											// type
											if (actTag->findXmlAttributeObjectByName(Ogml::s_attributeNames[Ogml::a_nLineType], actAtt))
												CGA.styleEdge(actEdge) = GraphAttributes::intToStyle(getLineTypeAsInt(actAtt->getValue()));
											// width
											if (actTag->findXmlAttributeObjectByName(Ogml::s_attributeNames[Ogml::a_width], actAtt))
												CGA.edgeWidth(actEdge) = atof(actAtt->getValue().cstr());
											// color
											if ((actTag->findXmlAttributeObjectByName(Ogml::s_attributeNames[Ogml::a_color], actAtt))
												&& (CGA.attributes() & GraphAttributes::edgeType))
												CGA.colorEdge(actEdge) = actAtt->getValue();
										}// line

										// mapping of arrows
										if (CGA.attributes() & GraphAttributes::edgeArrow)
										{
											// values for mapping edge arrows to GDE
											// init to -1 for a simple check
											int sourceInt = -1;
											int targetInt = -1;

											// sourceStyle tag
											if (stylesSon->findSonXmlTagObjectByName(Ogml::s_tagNames[Ogml::t_sourceStyle], actTag))
											{
												// type
												if (actTag->findXmlAttributeObjectByName(Ogml::s_attributeNames[Ogml::a_type], actAtt))
													sourceInt = getArrowStyleAsInt((actAtt->getValue()), Ogml::s_attributeNames[Ogml::t_source]);
												// color
												//if (actTag->findXmlAttributeObjectByName(Ogml::s_attributeNames[a_color], actAtt))
												//	;
												// size
												//if (actTag->findXmlAttributeObjectByName(Ogml::s_attributeNames[a_size], actAtt))
												//	;
											}// sourceStyle

											// targetStyle tag
											if (stylesSon->findSonXmlTagObjectByName(Ogml::s_tagNames[Ogml::t_targetStyle], actTag))
											{
												// type
												if (actTag->findXmlAttributeObjectByName(Ogml::s_attributeNames[Ogml::a_type], actAtt))
													targetInt = getArrowStyleAsInt((actAtt->getValue()), Ogml::s_attributeNames[Ogml::t_target]);
												// color
												//if (actTag->findXmlAttributeObjectByName(Ogml::s_attributeNames[a_color], actAtt))
												//	;
												// size
												//if (actTag->findXmlAttributeObjectByName(Ogml::s_attributeNames[a_size], actAtt))
												//	;
											}// targetStyle

											// map edge arrows
											if ((sourceInt != -1) || (targetInt != -1))
											{
												if (sourceInt <= 0) {
													if (targetInt <= 0) {
														//source=no arrow, target=no arrow // => none
														CGA.arrowEdge(actEdge) = GraphAttributes::none;
													}
													else {
														// source=no arrow, target=arrow // => last
														CGA.arrowEdge(actEdge) = GraphAttributes::last;
													}
												}
												else {
													if (targetInt <= 0) {
														//source=arrow, target=no arrow // => first
														CGA.arrowEdge(actEdge) = GraphAttributes::first;
													}
													else {
														//source=target=arrow // => both
														CGA.arrowEdge(actEdge) = GraphAttributes::both;
													}
												}
											}
										}//arrow

										// points & segments
										// bool value for checking if segments exist
										bool segmentsExist = stylesSon->findSonXmlTagObjectByName(Ogml::s_tagNames[Ogml::t_segment], actTag);
										if ((stylesSon->findSonXmlTagObjectByName(Ogml::s_tagNames[Ogml::t_point], actTag))
											&& (CGA.attributes() & GraphAttributes::edgeGraphics))
										{
											// at least one point exists
											XmlTagObject *pointTag = stylesSon->m_pFirstSon;
											DPolyline dpl;
											dpl.clear();
											// traverse all points in the order given in the ogml file
											while (pointTag)
											{
												if (pointTag->getName() == Ogml::s_tagNames[Ogml::t_point])
												{

													if (pointTag->findXmlAttributeObjectByName(Ogml::s_attributeNames[Ogml::a_id], actAtt)) {
														DPoint dp;
														// here we have a point
														if (pointTag->findXmlAttributeObjectByName(Ogml::s_attributeNames[Ogml::a_x], actAtt)) {
															dp.m_x = atof(actAtt->getValue().cstr());
														}
														if (pointTag->findXmlAttributeObjectByName(Ogml::s_attributeNames[Ogml::a_y], actAtt)) {
															dp.m_y = atof(actAtt->getValue().cstr());
														}
														//if (actTag->findXmlAttributeObjectByName(Ogml::s_attributeNames[a_z], actAtt))
														//	dp.m_z = atof(actAtt->getValue());
														// insert point into hash table
														pointTag->findXmlAttributeObjectByName(Ogml::s_attributeNames[Ogml::a_id], actAtt);
														m_points.fastInsert(actAtt->getValue(), dp);
														//insert point into polyline
														if (!segmentsExist)
															dpl.pushBack(dp);
													}
												}
												// go to next tag
												pointTag = pointTag->m_pBrother;
											}// while (pointTag)
											//concatenate polyline
											if (!segmentsExist) {
												CGA.bends(actEdge).conc(dpl);
											}
											else{
												// work with segments
												// one error can occur:
												// if a segments is going to be inserted,
												// which doesn't match with any other,
												// the order can be not correct at the end
												// then the edge is relly corrupted!!

												// TODO: this implementation doesn't work with hyperedges
												//       cause hyperedges have more than one source/target

												// segmentsUnsorted stores all found segments
												List<OgmlSegment> segmentsUnsorted;
												XmlTagObject *segmentTag = stylesSon->m_pFirstSon;
												while (segmentTag)
												{
													if (segmentTag->getName() == Ogml::s_tagNames[Ogml::t_segment])
													{
														XmlTagObject *endpointTag = segmentTag->m_pFirstSon;
														OgmlSegment actSeg;
														int endpointsSet = 0;
														while ((endpointTag) && (endpointsSet <2)) {
															if (endpointTag->getName() == Ogml::s_tagNames[Ogml::t_endpoint]) {
																// get the referenced point
																endpointTag->findXmlAttributeObjectByName(Ogml::s_attributeNames[Ogml::a_endpointIdRef], actAtt);
																DPoint dp = (m_points.lookup(actAtt->getValue()))->info();

																if (endpointsSet == 0)
																	actSeg.point1 = dp;
																else
																	actSeg.point2 = dp;
																endpointsSet++;
															}
															endpointTag = endpointTag->m_pBrother;
														}// while
														// now we created a segment
														// we can insert this easily into in segmentsUnsorted
														if (actSeg.point1 != actSeg.point2) {
															segmentsUnsorted.pushBack(actSeg);
														} // point1 != point2
													}// if (segment)
													// go to next tag
													segmentTag = segmentTag->m_pBrother;
												}// while (segmentTag)
												// now are the segments stored in the segmentsUnsorted list
												//  but we have to sort it in segments list while inserting
												List<OgmlSegment> segments;
												ListIterator<OgmlSegment> segIt;
												// check the number of re-insertions
												int checkNumOfSegReInserts = segmentsUnsorted.size()+2;
												while ((segmentsUnsorted.size() > 0) && (checkNumOfSegReInserts > 0))
												{
													OgmlSegment actSeg = segmentsUnsorted.front();
													segmentsUnsorted.popFront();
													// actSeg has to be inserted in correct order
													//  and then being deleted
													//  OR waiting in list until it can be inserted
													// size == 0 => insert
													if (segments.size() == 0) {
														segments.pushFront(actSeg);
													}
													else {
														// segments contains >1 segment
														segIt = segments.begin();
														bool inserted = false;
														while (segIt.valid() && !inserted)
														{
															if ((actSeg.point1 == (*segIt).point1) ||
																(actSeg.point1 == (*segIt).point2) ||
																(actSeg.point2 == (*segIt).point1) ||
																(actSeg.point2 == (*segIt).point2))
															{
																// found two matching segments
																// now we can insert
																// there are some cases to check
																if (actSeg.point1 == (*segIt).point1) {
																	DPoint dumP = actSeg.point1;
																	actSeg.point1 = actSeg.point2;
																	actSeg.point2 = dumP;
																	segments.insertBefore(actSeg, segIt);
																}
																else
																	if (actSeg.point2 == (*segIt).point1) {
																		segments.insertBefore(actSeg, segIt);
																	}
																	else
																		if ((actSeg.point2 == (*segIt).point2)) {
																			DPoint dumP = actSeg.point1;
																			actSeg.point1 = actSeg.point2;
																			actSeg.point2 = dumP;
																			segments.insertAfter(actSeg, segIt);
																		}
																		else {
																			segments.insertAfter(actSeg, segIt);
																		}
																		inserted = true;
															} // first if
															segIt++;
														} //while
														if (!inserted) {
															// segment doesn't found matching segment,
															//  so insert it again into unsorted segments list
															//  so it will be inserted later
															segmentsUnsorted.pushBack(actSeg);
															checkNumOfSegReInserts--;
														}
													}//else
												}//while segmentsUnsorted.size() > 0


												if (checkNumOfSegReInserts==0){
													cout << "WARNING! Segment definition is not correct" << endl << flush;
													cout << "Not able to work with #"<< segmentsUnsorted.size() << " segments" << endl << flush;
													cout << "Please check connection and sorting of segments!" << endl << flush;
													//				// inserting the bends although there might be an error
													//				// I commented this, because in this case in ogdf the edge will
													//				//   be a straight edge and there will not be any artefacts
													//				// TODO: uncomment if desired
													// 				for (segIt = segments.begin(); segIt.valid(); segIt++){
													//					dpl.pushBack((*segIt).point1);
													//					dpl.pushBack((*segIt).point2);
												}
												else {
													// the segments are now ordered (perhaps in wrong way)...
													// so we have to check if the first and last point
													//  are graphically laying in the source- and target- node
													bool invertSegments = false;
													segIt = segments.begin();
													node target = actEdge->target();
													node source = actEdge->source();
													// check if source is a normal node or a cluster
													//if (...){

													//}
													//else{
													// big if-check: if (first point is in target
													//                   and not in source)
													//                   AND
													//                   (last point is in source
													//                   and not in target)
													if (( ( (CGA.x(target) + CGA.width(target))>= (*segIt).point1.m_x )
														&&   (CGA.x(target)                      <= (*segIt).point1.m_x )
														&& ( (CGA.y(target) + CGA.height(target))>= (*segIt).point1.m_y )
														&&   (CGA.y(target)                      <= (*segIt).point1.m_y ) )
														&&
														(!( ( (CGA.x(source) + CGA.width(source))>= (*segIt).point1.m_x )
														&&   (CGA.x(source)                      <= (*segIt).point1.m_x )
														&& ( (CGA.y(source) + CGA.height(source))>= (*segIt).point1.m_y )
														&&   (CGA.y(source)                      <= (*segIt).point1.m_y ) )))
													{
														segIt = segments.rbegin();
														if (( ( (CGA.x(source) + CGA.width(source))>= (*segIt).point2.m_x )
															&&   (CGA.x(source)                      <= (*segIt).point2.m_x )
															&& ( (CGA.y(source) + CGA.height(source))>= (*segIt).point2.m_y )
															&&   (CGA.y(source)                      <= (*segIt).point2.m_y ) )
															&&
															(!( ( (CGA.x(target) + CGA.width(source))>= (*segIt).point2.m_x )
															&&   (CGA.x(target)                      <= (*segIt).point2.m_x )
															&& ( (CGA.y(target) + CGA.height(source))>= (*segIt).point2.m_y )
															&&   (CGA.y(target)                      <= (*segIt).point2.m_y ) ))) {
																// invert the segment-line
																invertSegments = true;
														}
													}
													//}
													if (!invertSegments){
														for (segIt = segments.begin(); segIt.valid(); segIt++) {
															dpl.pushBack((*segIt).point1);
															dpl.pushBack((*segIt).point2);
														}
													}
													else {
														for (segIt = segments.rbegin(); segIt.valid(); segIt--) {
															dpl.pushBack((*segIt).point2);
															dpl.pushBack((*segIt).point1);
														}
													}
													// unify bends = delete superfluous points
													dpl.unify();
													// finally concatenate/set the bends
													CGA.bends(actEdge).conc(dpl);
												}// else (checkNumOfSegReInserts==0)
											}// else (segments exist)
										}// points & segments

									}//edgeIdRef

								}// edgeStyle

								//			// LABELSTYLE
								//			if (stylesSon->getName() == Ogml::s_tagNames[t_labelStyle]){
								//				// labelStyle
								//				// ACTUALLY NOT SUPPORTED
								//			}// labelStyle

								stylesSon = stylesSon->m_pBrother;
							} // while

						}
					} //styles

					// CONSTRAINTS
					if (layoutSon->getName() == Ogml::s_tagNames[Ogml::t_constraints]) {

						// this code is encapsulated in the method
						// OgmlParser::buildConstraints
						// has to be called by read methods after building

						// here we only set the pointer,
						//  so we don't have to traverse the parse tree
						//  to the constraints tag later
						m_constraintsTag = layoutSon;

					}// constraints


					// go to next brother
					layoutSon = layoutSon->m_pBrother;
				}// while(layoutSon)
			}//if (layout->m_pFirstSon)
		}// if ((layout) && (layout->getName() == Ogml::s_tagNames[t_layout]))


	}// else





	//	cout << "buildAttributedClusterGraph COMPLETE. Check... " << endl << flush;
	//	edge e;
	//	forall_edges(e, G){
	//		//cout << "CGA.labelEdge" << e << " = " << CGA.labelEdge(e) << endl << flush;
	//		cout << "CGA.arrowEdge" << e << " = " << CGA.arrowEdge(e) << endl << flush;
	//		cout << "CGA.styleEdge" << e << " = " << CGA.styleEdge(e) << endl << flush;
	//		cout << "CGA.edgeWidth" << e << " = " << CGA.edgeWidth(e) << endl << flush;
	//		cout << "CGA.colorEdge" << e << " = " << CGA.colorEdge(e) << endl << flush;
	//		cout << "CGA.type     " << e << " = " << CGA.type(e) << endl << flush;
	//		ListConstIterator<DPoint> it;
	//		for(it = CGA.bends(e).begin(); it!=CGA.bends(e).end(); ++it) {
	//			cout << "point " << " x=" << (*it).m_x << " y=" << (*it).m_y << endl << flush;
	//		}
	//
	//	}
	//
	//	node n;
	//	forall_nodes(n, G){
	//		cout << "CGA.labelNode(" << n << ")     = " << CGA.labelNode(n) << endl << flush;
	//		cout << "CGA.templateNode(" << n << ")  = " << CGA.templateNode(n) << endl << flush;
	//		cout << "CGA.shapeNode(" << n << ")     = " << CGA.shapeNode(n) << endl << flush;
	//		cout << "CGA.width(" << n << ")         = " << CGA.width(n) << endl << flush;
	//		cout << "CGA.height(" << n << ")        = " << CGA.height(n) << endl << flush;
	//		cout << "CGA.colorNode(" << n << ")     = " << CGA.colorNode(n) << endl << flush;
	//		cout << "CGA.nodePattern(" << n << ")   = " << CGA.nodePattern(n) << endl << flush;
	//		cout << "CGA.styleNode(" << n << ")     = " << CGA.styleNode(n) << endl << flush;
	//		cout << "CGA.lineWidthNode(" << n << ") = " << CGA.lineWidthNode(n) << endl << flush;
	//		cout << "CGA.nodeLine(" << n << ")      = " << CGA.nodeLine(n) << endl << flush;
	//		cout << "CGA.x(" << n << ")             = " << CGA.x(n) << endl << flush;
	//		cout << "CGA.y(" << n << ")             = " << CGA.y(n) << endl << flush;
	//		cout << "CGA.type(" << n << ")          = " << CGA.type(n) << endl << flush;
	//	}
	//
	//	cluster c;
	//	forall_clusters(c, CGA.constClusterGraph()){
	//		cout << "CGA.templateCluster(" << c << ")    = " << CGA.templateCluster(c) << endl << flush;
	//		cout << "CGA.clusterWidth(" << c << ")       = " << CGA.clusterWidth(c) << endl << flush;
	//		cout << "CGA.clusterHeight(" << c << ")      = " << CGA.clusterHeight(c) << endl << flush;
	//		cout << "CGA.clusterFillColor(" << c << ")   = " << CGA.clusterFillColor(c) << endl << flush;
	//		cout << "CGA.clusterFillPattern(" << c << ") = " << CGA.clusterFillPattern(c) << endl << flush;
	//		cout << "CGA.clusterBackColor(" << c << ")   = " << CGA.clusterBackColor(c) << endl << flush;
	//		cout << "CGA.clusterLineStyle(" << c << ")   = " << CGA.clusterLineStyle(c) << endl << flush;
	//		cout << "CGA.clusterLineWidth(" << c << ")   = " << CGA.clusterLineWidth(c) << endl << flush;
	//		cout << "CGA.clusterColor(" << c << ")       = " << CGA.clusterColor(c) << endl << flush;
	//		cout << "CGA.clusterXPos(" << c << ")        = " << CGA.clusterXPos(c) << endl << flush;
	//		cout << "CGA.clusterYPos(" << c << ")        = " << CGA.clusterYPos(c) << endl << flush;
	//	}

	//	cout << "buildAttributedClusterGraph COMPLETE... Check COMPLETE... Let's have fun in GDE ;) " << endl << flush;

	// building terminated, so return true
	return true;

}//buildAttributedClusterGraph



// ***********************************************************
//
// s e t    l a b e l s    r e c u r s i v e     f o r     c l u s t e r s
//
// ***********************************************************
// sets the labels of hierarchical nodes => cluster
bool OgmlParser::setLabelsRecursive(Graph &G, ClusterGraphAttributes &CGA, XmlTagObject *root)
{
	if ((root->getName() == Ogml::s_tagNames[Ogml::t_node]) && (CGA.attributes() & GraphAttributes::nodeLabel))
	{
		if (!isNodeHierarchical(root))
		{
			// get the id of the actual node
			XmlAttributeObject *att;
			if(root->findXmlAttributeObjectByName(Ogml::s_attributeNames[Ogml::a_id], att))
			{
				// lookup for node
				node actNode = (m_nodes.lookup(att->getValue()))->info();
				// find label tag
				XmlTagObject* label;
				if (root->findSonXmlTagObjectByName(Ogml::s_tagNames[Ogml::t_label], label)) {
					// get content tag
					XmlTagObject* content = label->m_pFirstSon;
					// get the content as string
					if (content->m_pTagValue){
						String str = content->getValue();
						String labelStr = getLabelCaptionFromString(str);
						// now set the label of the node
						CGA.labelNode(actNode) = labelStr;
					}
				}
			}
		}// "normal" nodes
		else
		{
			// get the id of the actual cluster
			XmlAttributeObject *att;
			if(root->findXmlAttributeObjectByName(Ogml::s_attributeNames[Ogml::a_id], att))
			{
				// lookup for cluster
				cluster actCluster = (m_clusters.lookup(att->getValue()))->info();
				// find label tag
				XmlTagObject* label;
				if (root->findSonXmlTagObjectByName(Ogml::s_tagNames[Ogml::t_label], label)) {
					// get content tag
					XmlTagObject* content = label->m_pFirstSon;
					// get the content as string
					if (content->m_pTagValue) {
						String str = content->getValue();
						String labelStr = getLabelCaptionFromString(str);
						// now set the label of the node
						CGA.clusterLabel(actCluster) = labelStr;
					}
				}
			}
			// hierSon = hierarchical Son
			XmlTagObject *hierSon;
			if (root->m_pFirstSon)
			{
				hierSon = root->m_pFirstSon;
				while(hierSon) {
					// recursive call for setting labels of child nodes
					if (!setLabelsRecursive(G, CGA, hierSon))
						return false;
					hierSon = hierSon->m_pBrother;
				}
			}

		}//cluster nodes
	}
	return true;
}// setLabelsRecursive



// ***********************************************************
//
// b u i l d     g r a p h
//
// ***********************************************************
bool OgmlParser::buildGraph(Graph &G)
{
	G.clear();

	int id = 0;

	//Build nodes first
	HashConstIterator<String, const XmlTagObject*> it;

	for(it = m_ids.begin(); it.valid(); ++it)
	{
		if( it.info()->getName() == Ogml::s_tagNames[Ogml::t_node] && !isNodeHierarchical(it.info()))
		{
			// get id string from xmlTag
			XmlAttributeObject *idAtt;
			if ( (it.info())->findXmlAttributeObjectByName(Ogml::s_attributeNames[Ogml::a_id], idAtt)
				&& (getIdFromString(idAtt->getValue(), id)) )
			{
				// now we got an id from the id-string
				// we have to check, if this id was assigned
				if (m_nodeIds.lookup(id)) {
					// new id was assigned to another node
					id = G.maxNodeIndex() + 1;
				}
			}
			else {
				// default id setting
				id = G.maxNodeIndex() + 1;
			}
			m_nodes.fastInsert(it.key(), G.newNode(id));
			m_nodeIds.fastInsert(id, idAtt->getValue());
		}
	}//for nodes

	id = 0;

	//Build edges second
	for(it = m_ids.begin(); it.valid(); ++it)
	{
		if( it.info()->getName() == Ogml::s_tagNames[Ogml::t_edge] )
		{
			//Check sources/targets
			Stack<node> srcTgt;
			const XmlTagObject* son = it.info()->m_pFirstSon;
			while(son) {
				if( son->getName() == Ogml::s_tagNames[Ogml::t_source] ||
					son->getName() == Ogml::s_tagNames[Ogml::t_target] )
				{
					XmlAttributeObject *att;
					son->findXmlAttributeObjectByName(Ogml::s_attributeNames[Ogml::a_nodeIdRef], att);
					//Validate if source/target is really a node
					if(m_ids.lookup(att->getValue())->info()->getName() != Ogml::s_tagNames[Ogml::t_node]) {
						cout << "WARNING: edge relation between graph elements of none type node " <<
							"are temporarily not supported!\n";
					}
					else {
						srcTgt.push(m_nodes.lookup(att->getValue())->info());
					}
				}
				son = son->m_pBrother;
			}
			if(srcTgt.size() != 2) {
				cout << "WARNING: hyperedges are temporarily not supported! Discarding edge.\n";
			}
			else {
				// create edge

				// get id string from xmlTag
				XmlAttributeObject *idAtt;
				if ( (it.info())->findXmlAttributeObjectByName(Ogml::s_attributeNames[Ogml::a_id], idAtt)
					&& (getIdFromString(idAtt->getValue(), id)) )
				{
					if (m_edgeIds.lookup(id)) {
						// new id was assigned to another edge
						id = G.maxEdgeIndex() + 1;
					}
				}
				else {
					// default id setting
					id = G.maxEdgeIndex() + 1;
				}
				m_edges.fastInsert(it.key(), G.newEdge(srcTgt.pop(), srcTgt.pop(), id));
				m_edgeIds.fastInsert(id, idAtt->getValue());
			}
		}
	}//for edges

	//Structure data determined, so building the graph was successfull.
	return true;
}//buildGraph



// ***********************************************************
//
// b u i l d    c l u s t e r -- g r a p h
//
// ***********************************************************
bool OgmlParser::buildClusterRecursive(
	const XmlTagObject *xmlTag,
	cluster parent,
	Graph &G,
	ClusterGraph &CG)
{
	// create new cluster

	// first get the id
	int id = -1;

	XmlAttributeObject *idAtt;
	if (  (xmlTag->findXmlAttributeObjectByName(Ogml::s_attributeNames[Ogml::a_id], idAtt))
		&& (getIdFromString(idAtt->getValue(), id)) )
	{
		if (m_clusterIds.lookup(id)) {
			// id was assigned to another cluster
			id = CG.maxClusterIndex() + 1;
		}
	}
	else {
		// default id setting
		id = CG.maxClusterIndex() + 1;
	}
	// create cluster and insert into hash tables
	cluster actCluster = CG.newCluster(parent, id);
	m_clusters.fastInsert(idAtt->getValue(), actCluster);
	m_clusterIds.fastInsert(id, idAtt->getValue());

	// check children of cluster tag
	XmlTagObject *son = xmlTag->m_pFirstSon;

	while(son)
	{
		if (son->getName() == Ogml::s_tagNames[Ogml::t_node]) {
			if (isNodeHierarchical(son))
				// recursive call
				buildClusterRecursive(son, actCluster, G, CG);
			else {
				// the actual node tag is a child of the cluster
				XmlAttributeObject *att;
				//parse tree is valid so tag owns id attribute
				son->findXmlAttributeObjectByName(Ogml::s_attributeNames[Ogml::a_id], att);
				// get node from lookup table with the id in att
				node v = m_nodes.lookup(att->getValue())->info();
				// assign node to actual cluster
				CG.reassignNode(v, actCluster);
			}
		}

		son = son->m_pBrother;
	}//while

	return true;
}//buildClusterRecursive



bool OgmlParser::buildCluster(
	const XmlTagObject *rootTag,
	Graph &G,
	ClusterGraph &CG)
{
	CG.clear();
	CG.init(G);

	if(rootTag->getName() != Ogml::s_tagNames[Ogml::t_ogml]) {
		cerr << "ERROR: Expecting root tag \"" << Ogml::s_tagNames[Ogml::t_ogml]	<< "\" in OgmlParser::buildCluster!\n";
		return false;
	}

	//Search for first node tag
	XmlTagObject *nodeTag;
	rootTag->findSonXmlTagObjectByName(Ogml::s_tagNames[Ogml::t_graph], nodeTag);
	nodeTag->findSonXmlTagObjectByName(Ogml::s_tagNames[Ogml::t_structure], nodeTag);
	nodeTag->findSonXmlTagObjectByName(Ogml::s_tagNames[Ogml::t_node], nodeTag);

	while (nodeTag)
	{
		if(nodeTag->getName() == Ogml::s_tagNames[Ogml::t_node] && isNodeHierarchical(nodeTag)) {
			if (!buildClusterRecursive(nodeTag, CG.rootCluster(), G, CG))
				return false;
		}

		nodeTag = nodeTag->m_pBrother;
	}

	return true;
}//buildCluster




// ***********************************************************
//
// b u i l d     c o n s t r a i n t s
//
// ***********************************************************
//Commented out due to missing graphconstraints in OGDF
/*
bool OgmlParser::buildConstraints(Graph& G, GraphConstraints &GC) {

	// constraints-tag was already set
	// if not, then return... job's done
	if (!m_constraintsTag)
		return true;

	if (m_constraintsTag->getName() != Ogml::s_tagNames[t_constraints]){
		cerr << "Error: constraints tag is not the required tag!" << endl;
		return false;
	}

	XmlTagObject* constraintTag;
	if(! m_constraintsTag->findSonXmlTagObjectByName(Ogml::s_tagNames[t_constraint], constraintTag) ) {
		cerr << "Error: no constraint block in constraints block of valid parse tree found!" << endl;
		return false;
	}


	while(constraintTag) {

//		// found data
//		if (constraintTag->getName() == Ogml::s_tagNames[t_data]){
//			// found data for constraints in general
//			// no implementation required for ogdf
//		}//data

		if(constraintTag->getName() == Ogml::s_tagNames[t_constraint]) {

			XmlAttributeObject* actAtt;
			String cId;
			String cType;

			if (constraintTag->findXmlAttributeObjectByName(Ogml::s_attributeNames[Ogml::a_id], actAtt))
				// set id of the constraint
				cId = actAtt->getValue();

			if (constraintTag->findXmlAttributeObjectByName(Ogml::s_attributeNames[a_type], actAtt))
				cType = actAtt->getValue();
			else {
			 	cerr << "Error: constraint doesn't own compulsive attribute \'type\' in valid parse tree!" << endl;
				return false;
			}
			// now we need a constraint manager to create a constraint
			//  with the type of the name stored in cType
			// create the constraint
			Constraint* c = ConstraintManager::createConstraintByName(G, &cType);
			// check if the constraintManager doesn't return a null pointer
			//  that occurs if cM doesn't know the constraint name
			if (c) {
				// let the constraint load itself
				if (c->buildFromOgml(constraintTag, &m_nodes)){
					// add constraint if true is returned
					GC.addConstraint(c);
				}
				else
					cerr << "Error while building constraint with name \""<<cType<<"\"!" << endl;
			}
			else
				cerr << "Error: constraint type \""<<cType<<"\" is unknown!" << endl;

		}//constraint

		// go to next constraint tag
		constraintTag = constraintTag->m_pBrother;
	}//while

	// terminated, so return true
	return true;

}
*/



// ***********************************************************
//
// p u b l i c    r e a d     m e t h o d s
//
// ***********************************************************
bool OgmlParser::read(
	const char* fileName,
	Graph &G,
	ClusterGraph &CG)
{
	try {
		// DinoXmlParser for parsing the ogml file
		DinoXmlParser p(fileName);
		p.createParseTree();

		// get root object of the parse tree
		const XmlTagObject *root = &p.getRootTag();

		// build the required hash tables
		buildHashTables();

		// valide the document
		if ( validate(root, Ogml::t_ogml) == Ogml::vs_valid )
		{
			checkGraphType(root);
			// build graph
			if (buildGraph(G))
			{
				Ogml::GraphType gt = getGraphType();
				// switch GraphType
				switch (gt){
				//normal graph
				case Ogml::graph:
					break;

				// cluster graph
				case Ogml::clusterGraph:
					// build cluster
					if (!buildCluster(root, G, CG))
						return false;
					break;

				// compound graph
				case Ogml::compoundGraph:
					// build cluster because we got a cluster graph variable
					//  although we have a compound graph in the ogml file
					if (!buildCluster(root, G, CG))
						return false;
					break;

				// corrupt compound graph
				case Ogml::corruptCompoundGraph:
					// build cluster because we got a cluster graph variable
					//  although we have a corrupted compound graph in the ogml file
					if (!buildCluster(root, G, CG))
						return false;
					break;
				}
			} else
				return false;
		} else
			return false;

	} catch(const char *error) {
		cout << error << endl << flush;
		return false;
	}

	return true;
};


bool OgmlParser::read(
	const char* fileName,
	Graph &G,
	ClusterGraph &CG,
	ClusterGraphAttributes &CGA)
{
	try {
		// DinoXmlParser for parsing the ogml file
		DinoXmlParser p(fileName);
		p.createParseTree();

		// get root object of the parse tree
		const XmlTagObject *root = &p.getRootTag();

		// build the required hash tables
		buildHashTables();

		// valide the document
		if ( validate(root, Ogml::t_ogml) == Ogml::vs_valid )
		{
			checkGraphType(root);

			// build graph
			if (buildGraph(G))
			{
				Ogml::GraphType gt = getGraphType();
				// switch GraphType
				switch (gt){
				// normal graph
				case Ogml::graph:
					if (!buildAttributedClusterGraph(G, CGA, root))
						return false;
					break;

				// cluster graph
				case Ogml::clusterGraph:
					if (!buildCluster(root, G, CG))
						return false;
					if (!buildAttributedClusterGraph(G, CGA, root))
						return false;
					break;

				// compound graph
				case Ogml::compoundGraph:
					// build cluster because we got a cluster graph variable
					//  although we have a compound graph in the ogml file
					if (!buildCluster(root, G, CG))
						return false;
					if (!buildAttributedClusterGraph(G, CGA, root))
						return false;
					break;

				// corrupt compound graph
				case Ogml::corruptCompoundGraph:
					// build cluster because we got a cluster graph variable
					//  although we have a corrupted compound graph in the ogml file
					if (!buildCluster(root, G, CG))
						return false;
					if (!buildAttributedClusterGraph(G, CGA, root))
						return false;
				}

			} else
				return false;

		} else
			return false;

	} catch(const char *error) {
		cout << error << endl << flush;
		return false;
	}

	return true;
};


}//namespace ogdf

