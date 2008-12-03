// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2008 - TortoiseSVN

// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software Foundation,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//
#pragma once

#include "CopyFilterOptions.h"

class CFullGraph;
class CFullGraphNode;

/**
 * \ingroup TortoiseProc
 *
 * Create a filtered copy of the given full graph.
 * The \a copyFilter determines what nodes and branches 
 * will be removed.
 */

class CVisibleGraphBuilder
{
public:

    /// construction / destruction (nothing to do here)

	CVisibleGraphBuilder ( const CFullGraph& fullGraph
                         , CVisibleGraph& visibleGraph
                         , const CCopyFilterOptions& copyFilter);
	~CVisibleGraphBuilder (void);

    /// copy

	void Run();

private:

    /// the actual copy loop

    void CopyBranches (const CFullGraphNode* source, CVisibleGraphNode* target);
    void Copy (const CFullGraphNode* source, CVisibleGraphNode* target);

    /// data members

	const CFullGraph& fullGraph;
    CVisibleGraph& visibleGraph;
    CCopyFilterOptions copyFilter;
};
