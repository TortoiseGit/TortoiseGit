/*
 * $Revision: 2554 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-06 11:39:38 +0200 (Fr, 06. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Module for simdraw manipulator classes
 *
 * \author Michael Schulz
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

#include<ogdf/simultaneous/SimDrawManipulatorModule.h>

namespace ogdf {

//*************************************************************
// default constructor
//
SimDrawManipulatorModule::SimDrawManipulatorModule()
{
	m_SD = new SimDraw;
	m_G = &(m_SD->m_G);
	m_GA = &(m_SD->m_GA);
} //end default constructor


//*************************************************************
// initializing base instance
//
void SimDrawManipulatorModule::init(SimDraw &SD)
{
	m_SD = &SD;
	m_G = &(SD.m_G);
	m_GA = &(SD.m_GA);
	OGDF_ASSERT( &(*m_G) == &(m_GA->constGraph()) );
} //end constructor


} // end namespace ogdf
