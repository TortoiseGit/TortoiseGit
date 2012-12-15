/*
 * $Revision: 2559 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-06 15:04:28 +0200 (Fr, 06. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Definition of utility functions for FME layout.
 *
 * \author Martin Gronemann
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

#ifndef OGDF_FAST_UTILS_H
#define OGDF_FAST_UTILS_H

#include <ogdf/basic/GraphAttributes.h>
#include <iostream>

namespace ogdf {

// use SSE for Multipole computations
//#define OGDF_FME_KERNEL_USE_SSE

// use special thread affinity (works only for unix and scatters the threads)
//#define OGDF_FME_THREAD_AFFINITY

// simple parallel quadtree sort
//#define OGDF_FME_PARALLEL_QUADTREE_SORT

// use SSE for direct interaction (this is slower than the normal direct computation)
//#define OGDF_FME_KERNEL_USE_SSE_DIRECT

inline void OGDF_FME_Print_Config()
{
#ifdef OGDF_FME_KERNEL_USE_SSE
	std::cout << "OGDF_FME_KERNEL_USE_SSE" << std::endl;
#endif
#ifdef OGDF_FME_THREAD_AFFINITY
	std::cout << "OGDF_FME_THREAD_AFFINITY" << std::endl;
#endif
#ifdef OGDF_FME_PARALLEL_QUADTREE_SORT
	std::cout << "OGDF_FME_PARALLEL_QUADTREE_SORT" << std::endl;
#endif
#ifdef OGDF_FME_KERNEL_USE_SSE_DIRECT
	std::cout << "OGDF_FME_KERNEL_USE_SSE_DIRECT" << std::endl;
#endif
};

#ifdef OGDF_FME_KERNEL_USE_SSE
#include <emmintrin.h>
#include <pmmintrin.h>
#endif

typedef __uint64 MortonNR;
typedef __uint32 CoordInt;

template<typename T>
inline bool is_align_16(T* ptr)
{
	return !(((size_t)(void*)ptr) & 0x0F);
}

template<typename T>
inline T* align_16_prev_ptr(T* t)
{
	return (T*)(((size_t)((void*)t))&~ 0x0F);
}

template<typename T>
inline T* align_16_next_ptr(T* t)
{
	return (T*)((((size_t)((void*)t)) + 15)&~ 0x0F);
}

#ifdef OGDF_SYSTEM_UNIX
#include <sys/time.h>
inline timeval GetDiffTime(timeval _then, double& dtime)
{
	timeval then = (timeval) _then;
	timeval now;
	gettimeofday(&now, NULL);
	timeval diff;

	diff.tv_sec = now.tv_sec - then.tv_sec;
	diff.tv_usec = now.tv_usec - then.tv_usec;
	while(diff.tv_usec < 0)
	{
		diff.tv_sec--;
		diff.tv_usec = 1000000 + now.tv_usec - then.tv_usec;
	}

	dtime = diff.tv_sec;
	dtime += (double) diff.tv_usec / 1e6;

	return (timeval) now;
}
#endif

inline void printProfiledTime(double t, const char* text) { std::cout << t <<"s\t" << text << std::endl; };
inline void printProfiledTime(double t, double sum, const char* text) { std::cout << t <<"s\t" << text << "\t" << (t / sum)*100.0 <<"%"<< std::endl; };
//! Profile Macro to measure time with OGDF
#ifdef OGDF_SYSTEM_WINDOWS
#define FME_PROFILE(STATEMENT, TEXT) if(true) { double t=0.0;t=usedTime(t);STATEMENT;t=usedTime(t);std::cout << t <<"s\t" << TEXT << std::endl;};
#else
#define FME_PROFILE(STATEMENT, TEXT) if(true) { double t=0.0;timeval start_time,end_time;gettimeofday(&start_time,0);STATEMENT;end_time=GetDiffTime(start_time,t);if (isMainThread()) std::cout << t <<"s\t" << TEXT << std::endl;};
#endif

//! 16-byte aligned memory allocation macro
#define MALLOC_16(s) System::alignedMemoryAlloc16((s))

//! 16-byte aligned memory deallocation macro
#define FREE_16(ptr) System::alignedMemoryFree((ptr))

//! square root of two
#define SQRT_OF_TWO 1.4142135623730950488016887242097

//! common template for bit-interleaving to compute the morton number  assumes sizeOf(MNR_T) = 2*sizeOf(C_T)
template<typename MNR_T, typename C_T>
inline MNR_T mortonNumber(C_T ix, C_T iy)
{
	MNR_T x = (MNR_T)ix;
	MNR_T y = (MNR_T)iy;
	// bit length of the result
	const unsigned int BIT_LENGTH = sizeof(MNR_T) << 3;
	// set all bits
	MNR_T mask = 0x0;
	mask = ~mask;

	for (unsigned int i = (BIT_LENGTH >> 1);i>0; i = i >> 1)
	{
		// increase frequency
		mask = mask ^ (mask << i);
		x = (x | (x << i)) & mask;
		y = (y | (y << i)) & mask;
	}
	return x | (y << 1);
}


//! common template for extracting the coordinates from a morton number assumes sizeOf(MNR_T) = 2*sizeOf(C_T)
template<typename MNR_T, typename C_T>
inline void mortonNumberInv(MNR_T mnr, C_T& x, C_T& y)
{
	// bit length of the coordinates
	unsigned int BIT_LENGTH = sizeof(C_T) << 3;
	// set least significant bit
	MNR_T mask = 0x1;
	// set coords to zero
	x = y = 0;
	for (unsigned int i=0; i < BIT_LENGTH; i++)
	{
		x = (C_T)(x | (mnr & mask));
		mnr = mnr >> 1;
		y = (C_T)(y | (mnr & mask));
		mask = mask << 1;
	}
}

//! returns the index of the most signficant bit set. 0 = most signif, bitlength-1 = least signif
template<typename T>
inline __uint32 mostSignificantBit(T n)
{
	__uint32 BIT_LENGTH = sizeof(T) << 3;
	T mask = 0x1;
	mask = mask << (BIT_LENGTH - 1);
	for (__uint32 i = 0; i < BIT_LENGTH; i++)
	{
		if (mask & n)
			return i;
		mask = mask >> 1;
	}
	return BIT_LENGTH;
}

//! returns the prev power of two
inline __uint32 prevPowerOfTwo(__uint32 n)
{
	__uint32 msb = 32 - mostSignificantBit(n);
	return 0x1 << (msb - 1);
}

//! utility class to select multiple nodes randomly
class RandomNodeSet
{
public:
	//! init the random node set with the given graph. takes O(n)
	RandomNodeSet(const Graph& G) : m_graph(G) { allocate(); }

	//! destructor
	~RandomNodeSet() { deallocate(); }

	//! chooses a node from the available nodes in O(1)
	node chooseNode() const
	{
		int i = m_numNodesChoosen + ogdf::randomNumber(0,nodesLeft()-1);//(int)((double)nodesLeft()*rand()/(RAND_MAX+1.0));
		return m_array[i];
	}

	//! removes a node from available nodes (assumes v is available) in O(1)
	void removeNode(node v)
	{
		int i = m_nodeIndex[v];
		int j = m_numNodesChoosen;
		node w = m_array[j];
		swap(m_array[i], m_array[j]);
		m_nodeIndex[w] = i;
		m_nodeIndex[v] = j;
		m_numNodesChoosen++;
	}

	bool isAvailable(node v) const { return (m_nodeIndex[v]>=m_numNodesChoosen); }

	//! number of nodes available;
	int nodesLeft() const { return m_numNodes - m_numNodesChoosen; }

private:
	void allocate()
	{
		m_array = new node[m_graph.numberOfNodes()];
		m_nodeIndex.init(m_graph);
		m_numNodes = m_graph.numberOfNodes();
		m_numNodesChoosen = 0;
		node v;
		int i = 0;
		forall_nodes(v, m_graph)
		{
			m_array[i] = v;
			m_nodeIndex[v] = i;
			i++;
		}
	}

	void deallocate()
	{
		delete[] m_array;
	}

	//! the graph
	const Graph& m_graph;

	//! the set of all nodes (at the end the available nodes)
	node* m_array;

	//! the index in the array of the nodes
	NodeArray<int> m_nodeIndex;

	//! total num nodes
	int m_numNodes;

	//! num available nodes
	int m_numNodesChoosen;
};

inline void gridGraph(Graph& G, int n, int m)
{
	G.clear();
	node v;
	node* topRow = new node[m];
	topRow[0] = G.newNode();;
	for (int j=1; j<m; j++)
	{
		topRow[j] = G.newNode();
		G.newEdge(topRow[j-1], topRow[j]);
	}
	for (int i=1; i<n; i++)
	{
		v = G.newNode();
		G.newEdge(topRow[0], v);
		topRow[0] = v;
		for (int j=1; j<m; j++)
		{
			v = G.newNode();
			G.newEdge(topRow[j-1], v);
			G.newEdge(topRow[j], v);
			topRow[j] = v;
		}
	}
	delete[] topRow;
}

inline void randomGridGraph(Graph& G, int n, int m, double missinNodesPercentage = 0.03)
{
	gridGraph(G, n, m);
	int numberOfNodesToDelete = (int)((double)G.numberOfNodes() * missinNodesPercentage);

	RandomNodeSet rndSet(G);
	for(int i=0; i<numberOfNodesToDelete;i++)
	{
		node v = rndSet.chooseNode();
		rndSet.removeNode(v);
		G.delNode(v);
	}
}

//! binomial coeffs from Hachuls FMMM
template<class TYP>
class BinCoeff
{
public:
	BinCoeff(unsigned int n) : m_max_n(n) {	init_array(); }

	~BinCoeff() { free_array(); }

	//! Init BK -matrix for values n, k in 0 to t.
	void init_array()
	{
		typedef TYP*  ptr;
		unsigned int i,j;
		m_binCoeffs = new ptr[m_max_n+1];
		for(i = 0;i<= m_max_n ;i++)
		{
			m_binCoeffs[i]=  new TYP[i+1];
		}

		//Pascalsches Dreieck
		for (i = 0; i <= m_max_n;i++)
		{
			m_binCoeffs[i][0] = m_binCoeffs[i][i] = 1;
		}

		for (i = 2; i <= m_max_n; i ++)
		{
			for (j = 1; j < i; j++)
			{
				m_binCoeffs[i][j] = m_binCoeffs[i-1][j-1]+m_binCoeffs[i-1][j];
			}
		}
	}

	//! Free space for BK.
	void free_array()
	{
		unsigned int i;
		for(i = 0;i<= m_max_n;i++)
		{
			delete[] m_binCoeffs[i];
		}
		delete[] m_binCoeffs;
	}

	//Returns n over k.
	inline TYP value(unsigned int n, unsigned int k) const
	{
		return m_binCoeffs[n][k];
	}

private:
	unsigned int m_max_n;

	//! holds the binominal coefficients
	TYP** m_binCoeffs;
};


// nothing
struct EmptyArgType {};
//
// Function Invoker for 8 args
//
template<typename FunctionType, typename ArgType1 = EmptyArgType, typename ArgType2 = EmptyArgType, typename ArgType3 = EmptyArgType, typename ArgType4 = EmptyArgType, typename ArgType5 = EmptyArgType, typename ArgType6 = EmptyArgType, typename ArgType7 = EmptyArgType, typename ArgType8 = EmptyArgType>
struct FuncInvoker
{
	FuncInvoker(FunctionType f, ArgType1 _arg1, ArgType2 _arg2, ArgType3 _arg3, ArgType4 _arg4, ArgType5 _arg5, ArgType6 _arg6, ArgType7 _arg7, ArgType8 _arg8) :
		function(f), arg1(_arg1), arg2(_arg2), arg3(_arg3), arg4(_arg4), arg5(_arg5), arg6(_arg6), arg7(_arg7), arg8(_arg8) { }

	inline void operator()() { function(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8); }

	FunctionType function;
	ArgType1 arg1;
	ArgType2 arg2;
	ArgType3 arg3;
	ArgType4 arg4;
	ArgType5 arg5;
	ArgType6 arg6;
	ArgType7 arg7;
	ArgType8 arg8;
};


//
// Function Invoker for 7 args
//
template<typename FunctionType, typename ArgType1, typename ArgType2, typename ArgType3, typename ArgType4, typename ArgType5, typename ArgType6, typename ArgType7>
struct FuncInvoker<FunctionType, ArgType1, ArgType2, ArgType3, ArgType4, ArgType5, ArgType6, ArgType7, EmptyArgType>
{
	FuncInvoker(FunctionType f, ArgType1 _arg1, ArgType2 _arg2, ArgType3 _arg3, ArgType4 _arg4, ArgType5 _arg5, ArgType6 _arg6, ArgType7 _arg7) :
		function(f), arg1(_arg1), arg2(_arg2), arg3(_arg3), arg4(_arg4), arg5(_arg5), arg6(_arg6), arg7(_arg7) { }

	inline void operator()() { function(arg1, arg2, arg3, arg4, arg5, arg6, arg7); }

	FunctionType function;
	ArgType1 arg1;
	ArgType2 arg2;
	ArgType3 arg3;
	ArgType4 arg4;
	ArgType5 arg5;
	ArgType6 arg6;
	ArgType7 arg7;
};

//
// Function Invoker for 6 args
//
template<typename FunctionType, typename ArgType1, typename ArgType2, typename ArgType3, typename ArgType4, typename ArgType5, typename ArgType6>
struct FuncInvoker<FunctionType, ArgType1, ArgType2, ArgType3, ArgType4, ArgType5, ArgType6, EmptyArgType, EmptyArgType>
{
	FuncInvoker(FunctionType f, ArgType1 _arg1, ArgType2 _arg2, ArgType3 _arg3, ArgType4 _arg4, ArgType5 _arg5, ArgType6 _arg6) :
				 function(f), arg1(_arg1), arg2(_arg2), arg3(_arg3), arg4(_arg4), arg5(_arg5), arg6(_arg6) { }

	inline void operator()() { function(arg1, arg2, arg3, arg4, arg5, arg6); }

	FunctionType function;
	ArgType1 arg1;
	ArgType2 arg2;
	ArgType3 arg3;
	ArgType4 arg4;
	ArgType5 arg5;
	ArgType6 arg6;
};

//
// Function Invoker for 5 args
//
template<typename FunctionType, typename ArgType1, typename ArgType2, typename ArgType3, typename ArgType4, typename ArgType5>
struct FuncInvoker<FunctionType, ArgType1, ArgType2, ArgType3, ArgType4, ArgType5, EmptyArgType, EmptyArgType, EmptyArgType>
{
	FuncInvoker(FunctionType f, ArgType1 _arg1, ArgType2 _arg2, ArgType3 _arg3, ArgType4 _arg4, ArgType5 _arg5) :
				 function(f), arg1(_arg1), arg2(_arg2), arg3(_arg3), arg4(_arg4), arg5(_arg5) { }

	inline void operator()() { function(arg1, arg2, arg3, arg4, arg5); }

	FunctionType function;
	ArgType1 arg1;
	ArgType2 arg2;
	ArgType3 arg3;
	ArgType4 arg4;
	ArgType5 arg5;
};

//
// Function Invoker for 4 args
//
template<typename FunctionType, typename ArgType1, typename ArgType2, typename ArgType3, typename ArgType4>
struct FuncInvoker<FunctionType, ArgType1, ArgType2, ArgType3, ArgType4, EmptyArgType, EmptyArgType, EmptyArgType, EmptyArgType>
{
	FuncInvoker(FunctionType f, ArgType1 _arg1, ArgType2 _arg2, ArgType3 _arg3, ArgType4 _arg4) :
				 function(f), arg1(_arg1), arg2(_arg2), arg3(_arg3), arg4(_arg4) { }

	inline void operator()() { function(arg1, arg2, arg3, arg4); }

	FunctionType function;
	ArgType1 arg1;
	ArgType2 arg2;
	ArgType3 arg3;
	ArgType4 arg4;
};

//
// Function Invoker for 3 args
//
template<typename FunctionType, typename ArgType1, typename ArgType2, typename ArgType3>
struct FuncInvoker<FunctionType, ArgType1, ArgType2, ArgType3, EmptyArgType, EmptyArgType, EmptyArgType, EmptyArgType, EmptyArgType>
{
	FuncInvoker(FunctionType f, ArgType1 _arg1, ArgType2 _arg2, ArgType3 _arg3) :
				 function(f), arg1(_arg1), arg2(_arg2), arg3(_arg3) { }

	inline void operator()() { function(arg1, arg2, arg3); }

	FunctionType function;
	ArgType1 arg1;
	ArgType2 arg2;
	ArgType3 arg3;
};

//
// Function Invoker for 2 args
//
template<typename FunctionType, typename ArgType1, typename ArgType2>
struct FuncInvoker<FunctionType, ArgType1, ArgType2, EmptyArgType, EmptyArgType, EmptyArgType, EmptyArgType, EmptyArgType, EmptyArgType>
{
	FuncInvoker(FunctionType f, ArgType1 _arg1, ArgType2 _arg2) :
				 function(f), arg1(_arg1), arg2(_arg2) { }

	inline void operator()() { function(arg1, arg2); }

	FunctionType function;
	ArgType1 arg1;
	ArgType2 arg2;
};

//
// Function Invoker for 1 args
//
template<typename FunctionType, typename ArgType1>
struct FuncInvoker<FunctionType, ArgType1, EmptyArgType, EmptyArgType, EmptyArgType, EmptyArgType, EmptyArgType, EmptyArgType, EmptyArgType>
{
	FuncInvoker(FunctionType f, ArgType1 _arg1) :
				 function(f), arg1(_arg1) { }

	inline void operator()() { function(arg1); }

	FunctionType function;
	ArgType1 arg1;
};

//
// Function Invoker for 0 args
//
template<typename FunctionType>
struct FuncInvoker<FunctionType, EmptyArgType, EmptyArgType, EmptyArgType, EmptyArgType, EmptyArgType, EmptyArgType, EmptyArgType, EmptyArgType>
{
	FuncInvoker(FunctionType f) :
				 function(f) { }

	inline void operator()() { function(); }

	FunctionType function;
};


template<typename FunctionType, typename ArgType1, typename ArgType2, typename ArgType3, typename ArgType4, typename ArgType5, typename ArgType6, typename ArgType7, typename ArgType8>
FuncInvoker<FunctionType, ArgType1, ArgType2, ArgType3, ArgType4, ArgType5, ArgType6, ArgType7, ArgType8>
createFuncInvoker(FunctionType func, ArgType1 _arg1, ArgType2 _arg2, ArgType3 _arg3, ArgType4 _arg4, ArgType5 _arg5, ArgType6 _arg6, ArgType7 _arg7, ArgType8 _arg8)
{
	return FuncInvoker<FunctionType, ArgType1, ArgType2, ArgType3, ArgType4, ArgType5, ArgType6, ArgType7, ArgType8>(func, _arg1, _arg2, _arg3, _arg4, _arg5, _arg6, _arg7, _arg8);
}

template<typename FunctionType, typename ArgType1, typename ArgType2, typename ArgType3, typename ArgType4, typename ArgType5, typename ArgType6, typename ArgType7, typename ArgType8>
FuncInvoker<FunctionType, ArgType1, ArgType2, ArgType3, ArgType4, ArgType5, ArgType6, ArgType7, ArgType8>
createFuncInvoker(FunctionType func, ArgType1 _arg1, ArgType2 _arg2, ArgType3 _arg3, ArgType4 _arg4, ArgType5 _arg5, ArgType6 _arg6, ArgType7 _arg7)
{
	return FuncInvoker<FunctionType, ArgType1, ArgType2, ArgType3, ArgType4, ArgType5, ArgType6, ArgType7, EmptyArgType>(func, _arg1, _arg2, _arg3, _arg4, _arg5, _arg6, _arg7);
}

template<typename FunctionType, typename ArgType1, typename ArgType2, typename ArgType3, typename ArgType4, typename ArgType5, typename ArgType6>
FuncInvoker<FunctionType, ArgType1, ArgType2, ArgType3, ArgType4, ArgType5, ArgType6>
createFuncInvoker(FunctionType func, ArgType1 _arg1, ArgType2 _arg2, ArgType3 _arg3, ArgType4 _arg4, ArgType5 _arg5, ArgType6 _arg6)
{
	return FuncInvoker<FunctionType, ArgType1, ArgType2, ArgType3, ArgType4, ArgType5, ArgType6>(func, _arg1, _arg2, _arg3, _arg4, _arg5, _arg6);
}

template<typename FunctionType, typename ArgType1, typename ArgType2, typename ArgType3, typename ArgType4, typename ArgType5>
FuncInvoker<FunctionType, ArgType1, ArgType2, ArgType3, ArgType4, ArgType5>
createFuncInvoker(FunctionType func, ArgType1 _arg1, ArgType2 _arg2, ArgType3 _arg3, ArgType4 _arg4, ArgType5 _arg5)
{
	return FuncInvoker<FunctionType, ArgType1, ArgType2, ArgType3, ArgType4, ArgType5>(func, _arg1, _arg2, _arg3, _arg4, _arg5);
}

template<typename FunctionType, typename ArgType1, typename ArgType2, typename ArgType3, typename ArgType4>
FuncInvoker<FunctionType, ArgType1, ArgType2, ArgType3, ArgType4>
createFuncInvoker(FunctionType func, ArgType1 _arg1, ArgType2 _arg2, ArgType3 _arg3, ArgType4 _arg4)
{
	return FuncInvoker<FunctionType, ArgType1, ArgType2, ArgType3, ArgType4>(func, _arg1, _arg2, _arg3, _arg4);
}

template<typename FunctionType, typename ArgType1, typename ArgType2, typename ArgType3>
FuncInvoker<FunctionType, ArgType1, ArgType2, ArgType3>
createFuncInvoker(FunctionType func, ArgType1 _arg1, ArgType2 _arg2, ArgType3 _arg3)
{
	return FuncInvoker<FunctionType, ArgType1, ArgType2, ArgType3>(func, _arg1, _arg2, _arg3);
}

template<typename FunctionType, typename ArgType1, typename ArgType2>
FuncInvoker<FunctionType, ArgType1, ArgType2>
createFuncInvoker(FunctionType func, ArgType1 _arg1, ArgType2 _arg2)
{
	return FuncInvoker<FunctionType, ArgType1, ArgType2>(func, _arg1, _arg2);
}

template<typename FunctionType, typename ArgType1>
FuncInvoker<FunctionType, ArgType1>
createFuncInvoker(FunctionType func, ArgType1 _arg1)
{
	return FuncInvoker<FunctionType, ArgType1>(func, _arg1);
}

template<typename FunctionType>
FuncInvoker<FunctionType>
createFuncInvoker(FunctionType func)
{
	return FuncInvoker<FunctionType>(func);
}

}

#endif // fast utils h
