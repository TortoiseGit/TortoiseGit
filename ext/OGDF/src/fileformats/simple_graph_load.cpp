/*
 * $Revision: 2565 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-07 17:14:54 +0200 (Sa, 07. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Implementation of simple graph loaders.
 *
 * See header-file simple_graph_load.h for more information.
 *
 * \author Markus Chimani, Carsten Gutwenger, Karsten Klein
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

#include <ogdf/fileformats/simple_graph_load.h>
#include <ogdf/basic/Logger.h>
#include <ogdf/basic/HashArray.h>
#include <ogdf/basic/String.h>
#include <string.h>
#include <sstream>

#define SIMPLE_LOAD_BUFFER_SIZE 2048

namespace ogdf {

bool loadRomeGraph(Graph &G, const char *fileName) {
	ifstream is(fileName);
	if(!is.good()) return false;
	return loadRomeGraph(G, is);
}


bool loadRomeGraph(Graph &G, istream &is) {
	G.clear();

	char buffer[SIMPLE_LOAD_BUFFER_SIZE];
	bool readNodes = true;
	Array<node> indexToNode(1,250,0);

	while(!is.eof())
	{
		is.getline(buffer, SIMPLE_LOAD_BUFFER_SIZE-1);

		if(readNodes) {
			if(buffer[0] == '#') {
				readNodes = false;
				continue;
			}

			int index;
			sscanf(buffer, "%d", &index);
			if(index < 1 || index > 250 || indexToNode[index] != 0) {
				Logger::slout() << "loadRomeGraph: illegal node index!\n";
				return false;
			}

			indexToNode[index] = G.newNode();

		} else {
			int index, dummy, srcIndex, tgtIndex;
			sscanf(buffer, "%d%d%d%d", &index, &dummy, &srcIndex, &tgtIndex);

			if(buffer[0] == 0)
				continue;

			if(srcIndex < 1 || srcIndex > 250 || tgtIndex < 1 || tgtIndex > 250 ||
				indexToNode[srcIndex] == 0 || indexToNode[tgtIndex] == 0)
			{
				Logger::slout() << "loadRomeGraph: illegal node index in edge specification.\n";
				return false;
			}

			G.newEdge(indexToNode[srcIndex], indexToNode[tgtIndex]);
		}
	}
	return true;
}


bool loadChacoGraph(Graph &G, const char *fileName) {
	ifstream is(fileName);
	if(!is.good()) return false;
	return loadChacoGraph(G, is);
}


//Reads the chaco (graph partitioning) file format (usually a .graph file).
bool loadChacoGraph(Graph &G, istream &is)
{
	G.clear();
	char buffer[SIMPLE_LOAD_BUFFER_SIZE];
	int numN = 0, runEdges = 0, lineNum = 0;
	char* pch = NULL;

	//try to read the first line to get the graph size
	if (!is.eof())
	{
		//contains the size numbers
		is.getline(buffer, SIMPLE_LOAD_BUFFER_SIZE-1);
		//char* context = NULL;
		//now read the number of nodes
		pch = strtok(buffer, " ");//strtok_s(buffer, " ", &context);
		if (pch == NULL) return false;
		numN = atoi(pch);
		//now read the number of edges
		pch = strtok(NULL, " ");//strtok_s(NULL, " ", &context);
		if (pch == NULL) return false;
		// extension: check here for weights
	}
	else return false;

	if (numN == 0) return true;

	Array<node> indexToNode(1,numN,0);
	for (int i = 1; i <= numN; i++)
	{
		//we assign new indexes here if they are not consecutive
		//starting from 1 in the file! Thus, if the file is not in the correct
		//format, node indices do not correspond to indices from the file.
		indexToNode[i] = G.newNode();
	}

	while(!is.eof())
	{
		is.getline(buffer, SIMPLE_LOAD_BUFFER_SIZE-1);
		if (strlen(buffer) == 0) continue;
		lineNum++;
		if (lineNum > numN)
		{
			cerr<< "File read error: More lines than expected number of nodes "<< lineNum <<":"<<numN<<"\n";
			return false;
		}
		//char* context = NULL;
		pch = strtok(buffer, " ");//strtok_s(buffer, " ", &context);

		while (pch != NULL)
		{
			int wind = atoi(pch);
			if (wind < 1 || wind > numN)
			{
				cerr<<"File read error: Illegal node index encountered\n";
				return false;
			}

			//create edges
			if (wind >= lineNum)
			{
				G.newEdge( indexToNode[lineNum], indexToNode[wind] );
				runEdges++;
			}

			pch = strtok(NULL, " ");//strtok_s(NULL, " ", &context);
		}//while node entries
	}//while file
	//cout <<"Read #nodes: "<<numN<<", #edges "<<runEdges<<"\n";
	return true;
}


bool loadSimpleGraph(Graph &G, const char *fileName) {
	ifstream is(fileName);
	if(!is.good()) return false;
	return loadSimpleGraph(G, is);
}


bool loadSimpleGraph(Graph &G, istream &is) {
	G.clear();

	char buffer[SIMPLE_LOAD_BUFFER_SIZE];
	int numN = 0;

	//try to read the two first lines to get the graph size
	if (!is.eof())
	{
		char* pch;
		//contains the name
		is.getline(buffer, SIMPLE_LOAD_BUFFER_SIZE-1);
		pch = strtok(buffer, " ");
		if (strcmp(pch, "*BEGIN") != 0) return false;
		if (!is.eof())
		{	//contains the size of the graph
			is.getline(buffer, SIMPLE_LOAD_BUFFER_SIZE-1);
			pch = strtok(buffer, " ");
			if (strcmp(pch, "*GRAPH") != 0) return false;
			//now read the number of nodes
			pch = strtok(NULL, " ");
			if (pch == NULL) return false;
			numN = atoi(pch);
			//now read the number of edges
			pch = strtok(NULL, " ");
			if (pch == NULL) return false;
		}
		else return false;
	}
	else return false;

	if (numN == 0) return true;

	Array<node> indexToNode(1,numN,0);
	for (int i = 1; i <= numN; i++)
	{
		//we assign new indexes here if they are not consecutive
		//starting from 1 in the file!
		indexToNode[i] = G.newNode();
	}

	while(!is.eof())
	{
		is.getline(buffer, SIMPLE_LOAD_BUFFER_SIZE-1);

		if(buffer[0] == 0)
			continue;

		int srcIndex, tgtIndex;
		sscanf(buffer, "%d%d", &srcIndex, &tgtIndex);
		char* pch;
		pch = strtok(buffer, " ");
		if ( (strcmp(pch, "*END") == 0) || (strcmp(pch, "*CHECKSUM") == 0) )
			continue;

		if(srcIndex < 1 || srcIndex > numN || tgtIndex < 1 || tgtIndex > numN)
			{
				Logger::slout() << "loadSimpleGraph: illegal node index in edge specification.\n";
				return false;
			}

			G.newEdge(indexToNode[srcIndex], indexToNode[tgtIndex]);
	}
	return true;
}


#define YG_NEXTBYTE(x) x = fgetc(lineStream);	if(x == EOF || x == '\n') { Logger::slout() << "loadYGraph: line too short!"; return false; } x &= 0x3F;

bool loadYGraph(Graph &G, FILE *lineStream) {
	G.clear();

	char c,s;
	int n,i,j;

	YG_NEXTBYTE(n);
	Array<node> A(n);
	for(i=n; i-->0;)
		A[i] = G.newNode();

	s = 0;
	for(i = 1; i<n; ++i) {
		for(j = 0; j<i; ++j) {
			if(!s) {
				YG_NEXTBYTE(c);
				s = 5;
			} else --s;
			if(c & (1 << s))
				G.newEdge(A[i],A[j]);
		}
	}

	c = fgetc(lineStream);
	if(c != EOF && c != '\n') {
		Logger::slout(Logger::LL_MINOR) << "loadYGraph: Warning: line too long! ignoring...";
	}
	return true;
}


bool loadBenchHypergraph(Graph &G, List<node>& hypernodes, List<edge> *shell, const char *fileName) {
	ifstream is(fileName);
	if(!is.good()) return false;
	return loadBenchHypergraph(G, hypernodes, shell, is);
}


bool loadPlaHypergraph(Graph &G, List<node>& hypernodes, List<edge> *shell, const char *fileName) {
	ifstream is(fileName);
	if(!is.good()) return false;
	return loadPlaHypergraph(G, hypernodes, shell, is);
}


int extractIdentifierLength(char* from, int line) {
	int p = 1;
	while(from[p]!=',' && from[p]!=')' && from[p]!=' ' && from[p]!='(') {
		++p;
		if(from[p]=='\0') {
			cerr << "Loading Hypergraph: Error in line " << line <<
				". Expected comma, bracket or whitespace before EOL; Ignoring.\n";
			break;
		}
	}
	return p;
}


int newStartPos(char* from, int line) {
	int p = 0;
	while(from[p]=='\t' || from[p]==' ' || from[p]==',') {
		++p;
		if(from[p]=='\0') {
			cerr << "Loading Hypergraph: Error in line " << line <<
				". Expected whitespace or delimiter before EOL; Ignoring.\n";
			break;
		}
	}

	return p;
}


int findOpen(char* from, int line) {
	int p = 0;
	while(from[p]!='(') {
		++p;
		if(from[p]=='\0') {
			cerr << "Loading Hypergraph: Error in line " << line <<
				". Expected opening bracket before EOL; Ignoring.\n";
			break;
		}
	}
	return p;
}


String inName(const String& s) {
	size_t n = s.length();
	char *t = new char[n+4];
	ogdf::strcpy(t,s.length()+1,s.cstr());
	t[n] = '%';t[n+1] = '$';t[n+2] = '@';t[n+3] = '\0';
	String u(t);
	delete[] t;
	return u;
}


bool loadBenchHypergraph(Graph &G, List<node>& hypernodes, List<edge> *shell, istream &is) {
	G.clear();
	hypernodes.clear();
	if(shell) shell->clear();
	node si,so;

	char buffer[SIMPLE_LOAD_BUFFER_SIZE];

//	Array<node> indexToNode(1,250,0);

	HashArray<String,node> hm(0);

	if(shell) {
//		hypernodes.pushBack( si=G.newNode() );
//		hypernodes.pushBack( so=G.newNode() );
//		shell.pushBack( G.newEdge( si, so ) );
		shell->pushBack( G.newEdge( si=G.newNode(), so=G.newNode() ) );
	}

	int line = 0;
	while(!is.eof())
	{
		++line;
		is.getline(buffer, SIMPLE_LOAD_BUFFER_SIZE-1);
		size_t l = strlen(buffer);
		if( l > 0 && buffer[l-1]=='\r' ) { // DOS line
			buffer[l-1]='\0';
		}
		if(!strlen(buffer) || buffer[0]==' ' || buffer[0]=='#') continue;
		if(!strncmp("INPUT(",buffer,6)) {
			String s(extractIdentifierLength(buffer+6, line),buffer+6);
			node n = G.newNode();
			hm[s] = n;
			hypernodes.pushBack(n);
			if(shell) shell->pushBack( G.newEdge(si,n) );
//			cout << "input: " << s << " -> " << n->index() << "\n";
		} else if(!strncmp("OUTPUT(",buffer,7)) {
			String s(extractIdentifierLength(buffer+7, line),buffer+7);
			node n = G.newNode();
			hm[s] = n;
			hypernodes.pushBack(n);
			if(shell) shell->pushBack( G.newEdge(n,so) );
//			cout << "output: " << s << " -> " << n->index() << "\n";
		} else {
			int p = extractIdentifierLength(buffer, line);
			String s(p,buffer); // gatename
			node m = hm[s]; // found as outputname -> refOut
			if(!m) {
				m = hm[inName(s)]; // found as innernode input.
				if(!m) { // generate it anew.
					node in = G.newNode();
					node out = G.newNode();
					hm[inName(s)] = in;
					hm[s] = out;
					hypernodes.pushBack(out);
					G.newEdge(in,out);
					m = in;
				}
			}
			p = findOpen(buffer, line);
			do {
				++p;
				p += newStartPos(buffer+p, line);
				int pp = extractIdentifierLength(buffer+p, line);
				String s(pp,buffer+p);
				p += pp;
				node mm = hm[s];
				if(!mm) {
					// new
					node in = G.newNode();
					node out = G.newNode();
					hm[inName(s)] = in;
					hm[s] = out;
					hypernodes.pushBack(out);
					G.newEdge(in,out);
					mm = out;
				}
				G.newEdge(mm,m);
//				cout << "Edge: " << s << "(" << hm[s]->index() << ") TO " << m->index() << "\n";
			} while(buffer[p] == ',');
		}
	}

	return true;
}

bool loadPlaHypergraph(Graph &G, List<node>& hypernodes, List<edge> *shell, istream &is) {
	G.clear();
	hypernodes.clear();
	if(shell) shell->clear();
	node si,so;

	int i;
	int numGates;
	is >> numGates;
//	cout << "numGates=" << numGates << "\n";

	Array<node> outport(1,numGates);
	for(i = 1; i<=numGates; ++i) {
		node out = G.newNode();
		outport[i] = out;
		hypernodes.pushBack(out);
	}

	for(i = 1; i<=numGates; ++i) {
		int id, type, numinput;
		is >> id >> type >> numinput;
//		cout << "Gate=" << i << ", type=" << type << ", numinput=" << numinput << ":";
		if(id != i) cerr << "Error loading PLA hypergraph: ID and linenum does not match\n";
		node in = G.newNode();
		G.newEdge(in,outport[i]);
		for(int j=0; j<numinput; ++j) {
			int from;
			is >> from;
//			cout << " " << from;
			G.newEdge(outport[from],in);
		}
//		cout << "\n";
		is.ignore(500,'\n');
	}

	if(shell) {
		shell->pushBack( G.newEdge( si=G.newNode(), so=G.newNode() ) );
		node n;
		forall_nodes(n,G) {
			if(n->degree()==1) {
				if(n->firstAdj()->theEdge()->source()==n) { //input
					shell->pushBack( G.newEdge( si, n ) );
				} else { // output
					shell->pushBack( G.newEdge( n, so ) );
				}
			}
		}
	}

	return true;
}



bool loadEdgeListSubgraph(Graph &G, List<edge> &delEdges, const char *fileName)
{
	ifstream is(fileName);
	if(!is.good()) return false;
	return loadEdgeListSubgraph(G, delEdges, is);
}


bool loadEdgeListSubgraph(Graph &G, List<edge> &delEdges, istream &is)
{
	G.clear();
	delEdges.clear();

	char buffer[SIMPLE_LOAD_BUFFER_SIZE];

	if(is.eof()) return false;
	is.getline(buffer, SIMPLE_LOAD_BUFFER_SIZE-1);

	int n = 0, m = 0, m_del = 0;
	sscanf(buffer, "%d%d%d", &n, &m, &m_del);

	if(n < 0 || m < 0 || m_del < 0)
		return false;

	Array<node> indexToNode(n);
	for(int i = 0; i < n; ++i)
		indexToNode[i] = G.newNode();

	int m_all = m + m_del;
	for(int i = 0; i < m_all; ++i) {
		if(is.eof()) return false;

		is.getline(buffer, SIMPLE_LOAD_BUFFER_SIZE-1);
		int src, tgt;
		sscanf(buffer, "%d%d", &src, &tgt);
		if(src < 0 || src >= n || tgt < 0 || tgt >= n)
			return false;

		edge e = G.newEdge(indexToNode[src], indexToNode[tgt]);

		if(i >= m)
			delEdges.pushBack(e);
	}

	return true;
}


bool saveEdgeListSubgraph(const Graph &G, const List<edge> &delEdges, const char *fileName)
{
	ofstream os(fileName);
	return saveEdgeListSubgraph(G,delEdges,os);
}


bool saveEdgeListSubgraph(const Graph &G, const List<edge> &delEdges, ostream &os)
{
	if(!os.good()) return false;

	const int m_del = delEdges.size();
	const int n = G.numberOfNodes();
	const int m = G.numberOfEdges() - m_del;

	os << n << " " << m << " " << m_del << "\n";

	EdgeArray<bool> markSub(G,true);
	for(ListConstIterator<edge> it = delEdges.begin(); it.valid(); ++it)
		markSub[*it] = false;

	NodeArray<int> index(G);
	int i = 0;
	node v;
	forall_nodes(v,G)
		index[v] = i++;

	edge e;
	forall_edges(e,G)
		if(markSub[e])
			os << index[e->source()] << " " << index[e->target()] << "\n";

	for(ListConstIterator<edge> it = delEdges.begin(); it.valid(); ++it)
		os << index[(*it)->source()] << " " << index[(*it)->target()] << "\n";


	return true;
}


bool loadChallengeGraph(Graph &G, GridLayout &gl, const char *fileName)
{
	ifstream is(fileName);
	if(!is.good()) return false;
	return loadChallengeGraph(G, gl, is);
}


#define CHALLENGE_LOAD_BUFFER_SIZE 4096

bool loadChallengeGraph(Graph &G, GridLayout &gl, istream &is)
{
	G.clear();
	char buffer[CHALLENGE_LOAD_BUFFER_SIZE];

	int n = -1;
	do {
		if(is.eof()) return false;
		is.getline(buffer,CHALLENGE_LOAD_BUFFER_SIZE);
		if(buffer[0] != '#') {
			sscanf(buffer, "%d", &n);
			if(n < 0) return false;
		}
	} while(n < 0);

	Array<node> indexToNode(n);
	for(int i = 0; i < n; ) {
		if(is.eof()) return false;
		is.getline(buffer,CHALLENGE_LOAD_BUFFER_SIZE);

		if(buffer[0] != '#') {
			node v = G.newNode();
			sscanf(buffer, "%d%d", &gl.x(v), &gl.y(v));
			indexToNode[i++] = v;
		}
	}

	while(!is.eof()) {
		is.getline(buffer,CHALLENGE_LOAD_BUFFER_SIZE);

		if(buffer[0] != '#' && buffer[0] != 0) {
			std::stringstream ss(buffer);
			int srcIndex, tgtIndex;

			if(ss.eof()) return false;
			ss >> srcIndex;
			if(srcIndex < 0 || srcIndex >= n) return false;

			if(ss.eof()) return false;
			ss >> tgtIndex;
			if(tgtIndex < 0 || tgtIndex >= n) return false;

			node src = indexToNode[srcIndex];
			node tgt = indexToNode[tgtIndex];
			edge e = G.newEdge(src,tgt);

			std::string symbol;
			if(ss.eof()) return false;
			ss >> symbol;
			if(symbol != "[") return false;

			IPolyline &ipl = gl.bends(e);;
			for(;;) {
				if(ss.eof()) return false;
				ss >> symbol;
				if(symbol == "]") break;

				IPoint ip;
				ip.m_x = atoi(symbol.c_str());
				if(ss.eof()) return false;
				ss >> ip.m_y;
				ipl.pushBack(ip);
			}
		}
	}

	return true;
}


bool saveChallengeGraph(const Graph &G, const GridLayout &gl, const char *fileName)
{
	ofstream os(fileName);
	return saveChallengeGraph(G, gl, os);
}


bool saveChallengeGraph(const Graph &G, const GridLayout &gl, ostream &os)
{
	if(!os.good()) return false;

	os << "# Number of Nodes\n";
	os << G.numberOfNodes() << "\n";

	os << "# Nodes\n";
	NodeArray<int> index(G);
	int i = 0;
	node v;
	forall_nodes(v,G) {
		os << gl.x(v) << " " << gl.y(v) << "\n";
		index[v] = i++;
	}

	os << "# Edges\n";
	edge e;
	forall_edges(e,G) {
		os << index[e->source()] << " " << index[e->target()] << " [";
		const IPolyline &ipl = gl.bends(e);
		for(ListConstIterator<IPoint> it = ipl.begin(); it.valid(); ++it)
			os << " " << (*it).m_x << " " << (*it).m_y;
		os << " ]\n";
	}

	return true;
}


}
