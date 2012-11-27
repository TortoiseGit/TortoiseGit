/*
 * $Revision: 2554 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-06 11:39:38 +0200 (Fr, 06. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Offers variety of possible SimDraw creations.
 *
 * \author Michael Schulz and Daniel Lueckerath
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

#include <ogdf/simultaneous/SimDrawCreatorSimple.h>
#include <ogdf/basic/Array2D.h>

namespace ogdf {

//*************************************************************
// creates simultaneous graph given by two trees with n^2+n+1 nodes
// see Geyer,Kaufmann,Vrto (GD'05) for description of graph
void SimDrawCreatorSimple::createTrees_GKV05(int n)
{
	OGDF_ASSERT( n>=1 );

	node v0 = m_G->newNode();
	Array<node> v(n);
	Array2D<node> w(0,n,0,n);
	edge e;

	for(int i=0; i<n; i++)
	{
		v[i] = m_G->newNode();
		for(int j=0; j<n; j++)
			if(i!=j)
				w(i,j) = m_G->newNode();
	}

	for(int i=0; i<n; i++)
	{
		e = m_G->newEdge(v0,v[i]);
		m_GA->addSubGraph(e,0);
		m_GA->addSubGraph(e,1);
		for(int j=0; j<n; j++)
			if(i!=j)
			{
				e = m_G->newEdge(v[i],w(i,j));
				m_GA->addSubGraph(e,0);
				e = m_G->newEdge(v[j],w(i,j));
				m_GA->addSubGraph(e,1);
			}
	}

} // end createGKV


//*************************************************************
// creates simultaneous graph given by a path and a planar graph
// see Erten, Kobourov (GD'04) for description of instance
void SimDrawCreatorSimple::createPathPlanar_EK04()
{
	node v[10];
	edge e;

	for(int i= 1; i< 10; i++)
		v[i] = m_G->newNode();

	e = m_G->newEdge(v[1],v[2]);
	m_GA->addSubGraph(e,0);

	e = m_G->newEdge(v[1],v[3]);
	m_GA->addSubGraph(e,0);
	m_GA->addSubGraph(e,1);

	e = m_G->newEdge(v[1],v[4]);
	m_GA->addSubGraph(e,0);

	e = m_G->newEdge(v[1],v[5]);
	m_GA->addSubGraph(e,0);

	e = m_G->newEdge(v[1],v[6]);
	m_GA->addSubGraph(e,0);

	e = m_G->newEdge(v[2],v[3]);
	m_GA->addSubGraph(e,0);

	e = m_G->newEdge(v[2],v[4]);
	m_GA->addSubGraph(e,1);

	e = m_G->newEdge(v[2],v[5]);
	m_GA->addSubGraph(e,0);

	e = m_G->newEdge(v[2],v[6]);
	m_GA->addSubGraph(e,0);

	e = m_G->newEdge(v[2],v[7]);
	m_GA->addSubGraph(e,0);
	m_GA->addSubGraph(e,1);

	e = m_G->newEdge(v[2],v[8]);
	m_GA->addSubGraph(e,0);

	e = m_G->newEdge(v[2],v[9]);
	m_GA->addSubGraph(e,0);

	e = m_G->newEdge(v[3],v[4]);
	m_GA->addSubGraph(e,0);

	e = m_G->newEdge(v[3],v[5]);
	m_GA->addSubGraph(e,0);
	m_GA->addSubGraph(e,1);

	e = m_G->newEdge(v[4],v[5]);
	m_GA->addSubGraph(e,0);
	m_GA->addSubGraph(e,1);

	e = m_G->newEdge(v[5],v[6]);
	m_GA->addSubGraph(e,0);

	e = m_G->newEdge(v[5],v[9]);
	m_GA->addSubGraph(e,0);

	e = m_G->newEdge(v[6],v[7]);
	m_GA->addSubGraph(e,0);

	e = m_G->newEdge(v[6],v[9]);
	m_GA->addSubGraph(e,0);

	e = m_G->newEdge(v[6],v[8]);
	m_GA->addSubGraph(e,1);

	e = m_G->newEdge(v[7],v[8]);
	m_GA->addSubGraph(e,0);

	e = m_G->newEdge(v[7],v[9]);
	m_GA->addSubGraph(e,0);
	m_GA->addSubGraph(e,1);

	e = m_G->newEdge(v[8],v[9]);
	m_GA->addSubGraph(e,0);
	m_GA->addSubGraph(e,1);

}//end createPathPlanar_EK04


//*************************************************************
// creates simultaneous graph given by a colored K5
// see Erten, Kobourov (GD'04) for description of instance
void SimDrawCreatorSimple::createK5_EK04()
{
	int number = 5;
	Array<node> v(number);
	edge e;

	for(int i = 0; i < number; i++)
		v[i] = m_G->newNode();

	for(int i = 0; i < number-1; i++)
	{
		for(int j = i+1; j < number; j++)
		{
			e = m_G->newEdge(v[i],v[j]);
			if ((j == i+1) || ((j == number-1) && (i == 0)))
				m_GA->addSubGraph(e,0);
			else
				m_GA->addSubGraph(e,1);
		}
	}

}//end createK5_EK04


//*************************************************************
// creates simultaneous graph given by a colored K5
// see Gassner, Juenger, Percan, Schaefer, Schulz (WG'06) for description of instance
void SimDrawCreatorSimple::createK5_GJPSS06()
{
	int number = 5;
	Array<node> v(number);
	edge e;

	for(int i = 0; i < number; i++)
		v[i] = m_G->newNode();

	for(int i = 0; i < 3; i++)
	{
		for(int j = i+1; j <= 2; j++)
		{
			e = m_G->newEdge(v[i],v[j]);
			m_GA->addSubGraph(e,0);
			m_GA->addSubGraph(e,1);
		}
	}

	for(int i = 3; i < number; i++)
	{
		for(int j = 0; j < i; j++)
		{
			e = m_G->newEdge(v[i],v[j]);
			if(j == 3)
				m_GA->addSubGraph(e,0);
			else
				m_GA->addSubGraph(e,1);
		}
	}

}//end createK5_GJPSS06


//*************************************************************
// creates simultaneous graph given by two outerplanar graphs
// see Brass et al. (WADS '03) for description of instance
void SimDrawCreatorSimple::createOuterplanar_BCDEEIKLM03()
{
	int number = 6;
	Array<node> v(number);
	edge e;

	for(int i = 0; i < number; i++)
		v[i] = m_G->newNode();

	for(int i = 0; i < number-1; i++)
	{
		e = m_G->newEdge(v[i],v[i+1]);
		if(!(i == 2))
		{
			m_GA->addSubGraph(e,0);
			m_GA->addSubGraph(e,1);
		}
		else
		{
			m_GA->addSubGraph(e,0);

			e = m_G->newEdge(v[i],v[number-1]);
			m_GA->addSubGraph(e,1);
		}
	}
	e = m_G->newEdge(v[number-1],v[0]);
	m_GA->addSubGraph(e,0);

	e = m_G->newEdge(v[0],v[3]);
	m_GA->addSubGraph(e,1);

	e = m_G->newEdge(v[1],v[4]);
	m_GA->addSubGraph(e,0);
	m_GA->addSubGraph(e,1);

}// end createOuterplanar_BCDEEIKLM03


//*************************************************************
// creates simultaneous graph with crossing number 0 but
// with multicrossings of adjacent edges in mincross drawing
// see Kratochvil (GD '98) for description of instance
void SimDrawCreatorSimple::createKrat98(int N, int nodeNumber)
{
	OGDF_ASSERT( N>=1 && nodeNumber>=1 );

	Array<node> p(nodeNumber);
	Array<node> q(nodeNumber);
	Array<node> r(nodeNumber);
	Array<node> outerNodes(4);
	Array<node> outerOuterNodes(4);
	node a = m_G->newNode();
	node b = m_G->newNode();
	node nodeC = m_G->newNode();
	edge e;

	for(int i = 0; i < nodeNumber; i++)
	{
		p[i] = m_G->newNode();
		q[i] = m_G->newNode();
		r[i] = m_G->newNode();
	}

	for(int i = 0; i < 4; i++)
	{
		outerNodes[i] = m_G->newNode();
		outerOuterNodes[i] = m_G->newNode();
	}

	if(N > 1)
	{
		Array<node> c(N);
		for(int i = 0; i < N; i++)
		{
			c[i] = m_G->newNode();

			e = m_G->newEdge(c[i],nodeC);
			m_GA->addSubGraph(e,1);

			e = m_G->newEdge(a,c[i]);
			m_GA->addSubGraph(e,1);

			//eventuell unnoetig?
			e = m_G->newEdge(outerNodes[1],c[i]);
			m_GA->addSubGraph(e,1);
		}
	}
	else
	{
		e = m_G->newEdge(a,nodeC);
		m_GA->addSubGraph(e,1);
	}

	e = m_G->newEdge(a,b);
	m_GA->addSubGraph(e,0);

	e = m_G->newEdge(b,nodeC);
	m_GA->addSubGraph(e,0);
	m_GA->addSubGraph(e,1);
	m_GA->addSubGraph(e,2);

	for(int i = 0; i < nodeNumber-1; i++)
	{
		e = m_G->newEdge(p[i],p[i+1]);
		m_GA->addSubGraph(e,0);
		m_GA->addSubGraph(e,1);
		m_GA->addSubGraph(e,2);

		e = m_G->newEdge(r[i],r[i+1]);
		m_GA->addSubGraph(e,0);
		m_GA->addSubGraph(e,1);
		m_GA->addSubGraph(e,2);
	}

	for(int i = 0; i < nodeNumber; i++)
	{
		e = m_G->newEdge(p[i],q[i]);
		m_GA->addSubGraph(e,2);
		if(i%2)
			m_GA->addSubGraph(e,0);
		else
			m_GA->addSubGraph(e,1);

		e = m_G->newEdge(r[i],q[i]);
		m_GA->addSubGraph(e,2);
		if(i%2)
			m_GA->addSubGraph(e,1);
		else
			m_GA->addSubGraph(e,0);
	}

	e = m_G->newEdge(a,p[0]);
	m_GA->addSubGraph(e,0);
	m_GA->addSubGraph(e,1);
	m_GA->addSubGraph(e,2);

	e = m_G->newEdge(a,r[0]);
	m_GA->addSubGraph(e,0);
	m_GA->addSubGraph(e,1);
	m_GA->addSubGraph(e,2);

	e = m_G->newEdge(r[nodeNumber-1],b);
	m_GA->addSubGraph(e,0);
	m_GA->addSubGraph(e,1);
	m_GA->addSubGraph(e,2);

	e = m_G->newEdge(p[nodeNumber-1],nodeC);
	m_GA->addSubGraph(e,0);
	m_GA->addSubGraph(e,1);
	m_GA->addSubGraph(e,2);

	for(int i = 0; i < 4; i++)
	{
		e = m_G->newEdge(outerOuterNodes[i],outerNodes[i]);
		m_GA->addSubGraph(e,0);
		m_GA->addSubGraph(e,1);
		m_GA->addSubGraph(e,2);

		if(i < 3)
		{
			e = m_G->newEdge(outerOuterNodes[i],outerOuterNodes[i+1]);
			m_GA->addSubGraph(e,0);
			m_GA->addSubGraph(e,1);
			m_GA->addSubGraph(e,2);
		}
	}

	e = m_G->newEdge(outerOuterNodes[3],outerOuterNodes[0]);
	m_GA->addSubGraph(e,0);
	m_GA->addSubGraph(e,1);
	m_GA->addSubGraph(e,2);

	e = m_G->newEdge(outerNodes[1],outerNodes[2]);
	m_GA->addSubGraph(e,0);
	m_GA->addSubGraph(e,1);
	m_GA->addSubGraph(e,2);

	e = m_G->newEdge(outerNodes[3],outerNodes[0]);
	m_GA->addSubGraph(e,0);
	m_GA->addSubGraph(e,1);
	m_GA->addSubGraph(e,2);

	e = m_G->newEdge(outerNodes[0],a);
	m_GA->addSubGraph(e,0);
	m_GA->addSubGraph(e,1);
	m_GA->addSubGraph(e,2);

	e = m_G->newEdge(outerNodes[3],a);
	m_GA->addSubGraph(e,0);
	m_GA->addSubGraph(e,1);
	m_GA->addSubGraph(e,2);

	e = m_G->newEdge(outerNodes[1],nodeC);
	m_GA->addSubGraph(e,0);
	m_GA->addSubGraph(e,1);
	m_GA->addSubGraph(e,2);

	e = m_G->newEdge(outerNodes[2],b);
	m_GA->addSubGraph(e,0);
	m_GA->addSubGraph(e,1);
	m_GA->addSubGraph(e,2);

}// end createKrat


//*************************************************************
// creates Graph with numberofBasic*2 outer, numberOfParallels*numberOfBasic
// inner Nodes and one Root.
void SimDrawCreatorSimple::createWheel(int numberOfParallels, int numberOfBasic )
{
	OGDF_ASSERT(numberOfBasic > 0 && numberOfParallels >= 0);

	node root = m_G->newNode();
	Array<node> v(numberOfBasic*2);
	edge e;

	for(int i = 0; i < numberOfBasic*2; i++)
	{
		v[i] = m_G->newNode();
		e = m_G->newEdge(root,v[i]);
		for(int j = 0; j < numberOfBasic; j++)
			m_GA->addSubGraph(e,j);
	}

	for(int i = 0; i < numberOfBasic*2; i++)
	{
		if((i >= 0) && (i < (numberOfBasic*2)-1))
		{
			e = m_G->newEdge(v[i],v[i+1]);
			for(int j = 0; j < numberOfBasic; j++)
				m_GA->addSubGraph(e,j);
		}
		if(i == (numberOfBasic*2)-1)
		{
			e = m_G->newEdge(v[i],v[0]);
			for(int j = 0; j < numberOfBasic; j++)
				m_GA->addSubGraph(e,j);
		}

		if((numberOfBasic+i) < (numberOfBasic*2))
		{
			for(int j = 0; j < numberOfParallels; j++)
			{
				node tmpNOP = m_G->newNode();
				e = m_G->newEdge(v[i],tmpNOP);
				m_GA->addSubGraph(e,i);
				e = m_G->newEdge(v[numberOfBasic+i],tmpNOP);
				m_GA->addSubGraph(e,i);
			}
		}
	}

}//end createWheel


//*************************************************************
// creates simultaneously planar simulatenous graph with n+1 basic graphs.
//
void SimDrawCreatorSimple::createExpo(int n)
{

	OGDF_ASSERT(n>0 && n<31);

	Array<node> u(n+1);
	Array<node> v(n+1);
	Array<node> twinNodesU(n+1);
	Array<node> outerNodes(6);
	edge e;

	for(int i = 0; i < n+1; i++)
	{
		u[i] = m_G->newNode();
		v[i] = m_G->newNode();
		twinNodesU[i] = m_G->newNode();
	}

	for(int i = 0; i < 6; i++)
		outerNodes[i] = m_G->newNode();

	for(int i = 1; i < 3 ; i++)
	{
		e = m_G->newEdge(outerNodes[i],outerNodes[i+1]);
		for(int j = 0; j < 4; j++)
			m_GA->addSubGraph(e,j);
	}

	e = m_G->newEdge(outerNodes[4],outerNodes[5]);
	for(int j = 0; j < 4; j++)
		m_GA->addSubGraph(e,j);

	e = m_G->newEdge(outerNodes[5],outerNodes[0]);
	for(int j = 0; j < 4; j++)
		m_GA->addSubGraph(e,j);

	for(int i = 0; i < n+1; i++)
	{
		e = m_G->newEdge(u[i],twinNodesU[i]);
		for(int j = 0; j < 4; j++)
			m_GA->addSubGraph(e,j);
	}

	for(int i = 0; i < n; i++)
	{
		e = m_G->newEdge(twinNodesU[i],twinNodesU[i+1]);
		for(int j = 0; j < 4; j++)
			m_GA->addSubGraph(e,j);

		if(i == 0)
		{
			e = m_G->newEdge(outerNodes[3],twinNodesU[i]);
			for(int j = 0; j < 4; j++)
				m_GA->addSubGraph(e,j);
		}
	}

	e = m_G->newEdge(outerNodes[4],twinNodesU[n]);
	for(int j = 0; j < 4; j++)
		m_GA->addSubGraph(e,j);

	e = m_G->newEdge(v[0],outerNodes[0]);
	for(int j = 0; j < 4; j++)
		m_GA->addSubGraph(e,j);

	e = m_G->newEdge(v[0],outerNodes[1]);
	for(int j = 0; j < 4; j++)
		m_GA->addSubGraph(e,j);

	for(int i = 0; i < n+1; i++)
	{
		e = m_G->newEdge(u[i],v[i]);
		if(i == 0)
			m_GA->addSubGraph(e,0);
		else
		{
			m_GA->addSubGraph(e,1);
			if(i == 1)
				m_GA->addSubGraph(e,2);
			if(i == 2)
				m_GA->addSubGraph(e,3);
		}
	}

	e = m_G->newEdge(outerNodes[5],u[n]);
	m_GA->addSubGraph(e,0);
	m_GA->addSubGraph(e,2);
	m_GA->addSubGraph(e,3);

	e = m_G->newEdge(outerNodes[2],v[1]);
	m_GA->addSubGraph(e,0);

	for(int i = 1; i < n+1; i++)
	{
		e = m_G->newEdge(v[i],u[i-1]);
		m_GA->addSubGraph(e,0);
		if(i == 3)
			m_GA->addSubGraph(e,2);
	}

	for(int i = 0; i < 2; i++)
	{
		e = m_G->newEdge(u[i],v[i+2]);
		m_GA->addSubGraph(e,0);
		m_GA->addSubGraph(e,2);
		if(i == 1)
			m_GA->addSubGraph(e,3);
	}

	e = m_G->newEdge(u[n-1],u[n]);
	for(int j = 0; j < 4; j++)
		if(j != 1)
			m_GA->addSubGraph(e,j);

}//end createExpo

} // end namespace ogdf
