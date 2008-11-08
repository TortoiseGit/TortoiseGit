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

#include "PathClassificator.h"

using namespace LogCache;

class CFullHistory;
class CFullGraph;
class CFullGraphNode;

/**
 * \ingroup TortoiseProc
 */
class CFullGraphFinalizer
{
public:

	CFullGraphFinalizer (const CFullHistory& history, CFullGraph& graph);
	~CFullGraphFinalizer (void);

	void Run();

private:

    /// simplify graph

    void FindRenames (CFullGraphNode* node);

    /// mark nodes according to local properties

    void MarkRoot (CFullGraphNode* node);
    void MarkCopySource (CFullGraphNode* node);
    void MarkWCRevision (CFullGraphNode* node);
    void MarkHead (CFullGraphNode* node);
    void ForwardClassification (CFullGraphNode* node);

    /// inherit properties

    DWORD BackwardClassification (CFullGraphNode* node);

    /// data members

    const CFullHistory& history;
    CFullGraph& graph;

    std::auto_ptr<CPathClassificator> pathClassification;
};
