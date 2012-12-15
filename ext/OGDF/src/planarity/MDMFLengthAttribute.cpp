/*
 * $Revision: 2559 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-06 15:04:28 +0200 (Fr, 06. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Length attribute used in EmbedderMinDepthMaxFace.
 * It contains two components (d, l) and a linear order is defined by:
 * (d, l) > (d', l') iff d > d' or (d = d' and l > l')
 *
 * \author Thorsten Kerkhof
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

#include <ogdf/internal/planarity/MDMFLengthAttribute.h>

namespace ogdf {

MDMFLengthAttribute MDMFLengthAttribute::operator=(const MDMFLengthAttribute& x)
{
	this->d = x.d;
	this->l = x.l;
	return *this;
}

MDMFLengthAttribute MDMFLengthAttribute::operator=(const int& x)
{
	this->d = x;
	this->l = 0;
	return *this;
}

bool MDMFLengthAttribute::operator==(const MDMFLengthAttribute& x)
{
	return (this->d == x.d && this->l == x.l);
}

bool MDMFLengthAttribute::operator!=(const MDMFLengthAttribute& x)
{
	return !(*this == x);
}

bool MDMFLengthAttribute::operator>(const MDMFLengthAttribute& x)
{
	return (this->d > x.d || (this->d == x.d && this->l > x.l));
}

bool MDMFLengthAttribute::operator<(const MDMFLengthAttribute& x)
{
	return !(*this >= x);
}

bool MDMFLengthAttribute::operator>=(const MDMFLengthAttribute& x)
{
	return (*this == x) || (*this > x);
}

bool MDMFLengthAttribute::operator<=(const MDMFLengthAttribute& x)
{
	return (*this == x) || (*this < x);
}

MDMFLengthAttribute MDMFLengthAttribute::operator+(const MDMFLengthAttribute& x)
{
	return MDMFLengthAttribute(this->d + x.d, this->l + x.l);
}

MDMFLengthAttribute MDMFLengthAttribute::operator-(const MDMFLengthAttribute& x)
{
	return MDMFLengthAttribute(this->d - x.d, this->l - x.l);
}

MDMFLengthAttribute MDMFLengthAttribute::operator+=(const MDMFLengthAttribute& x)
{
	this->d += x.d;
	this->l += x.l;
	return *this;
}

MDMFLengthAttribute MDMFLengthAttribute::operator-=(const MDMFLengthAttribute& x)
{
	this->d -= x.d;
	this->l -= x.l;
	return *this;
}

bool operator==(const MDMFLengthAttribute& x, const MDMFLengthAttribute& y)
{
	return (x.d == y.d && x.l == y.l);
}

bool operator!=(const MDMFLengthAttribute& x, const MDMFLengthAttribute& y)
{
	return !(x == y);
}

bool operator>(const MDMFLengthAttribute& x, const MDMFLengthAttribute& y)
{
	return (x.d > y.d || (x.d == y.d && x.l > y.l));
}

bool operator<(const MDMFLengthAttribute& x, const MDMFLengthAttribute& y)
{
	return y > x;
}

bool operator>=(const MDMFLengthAttribute& x, const MDMFLengthAttribute& y)
{
	return (x == y) || (x > y);
}

bool operator<=(const MDMFLengthAttribute& x, const MDMFLengthAttribute& y)
{
	return (x == y) || (x < y);
}

MDMFLengthAttribute operator+(const MDMFLengthAttribute& x, const MDMFLengthAttribute& y)
{
	return MDMFLengthAttribute(x.d + y.d, x.l + y.l);
}

MDMFLengthAttribute operator-(const MDMFLengthAttribute& x, const MDMFLengthAttribute& y)
{
	return MDMFLengthAttribute(x.d - y.d, x.l - y.l);
}

MDMFLengthAttribute operator+=(const MDMFLengthAttribute& x, const MDMFLengthAttribute& y)
{
	return MDMFLengthAttribute(x.d + y.d, x.l + y.l);
}

MDMFLengthAttribute operator-=(const MDMFLengthAttribute& x, const MDMFLengthAttribute& y)
{
	return MDMFLengthAttribute(x.d - y.d, x.l - y.l);
}

ostream& operator<<(ostream& s, const MDMFLengthAttribute& x)
{
	s << x.d << ", " << x.l;
	return s;
}

} // end namespace ogdf
