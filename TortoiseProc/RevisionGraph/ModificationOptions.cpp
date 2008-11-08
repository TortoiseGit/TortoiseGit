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

#include "StdAfx.h"
#include "ModificationOptions.h"
#include "VisibleGraph.h"
#include "VisibleGraphNode.h"

// apply a filter using differnt traversal orders

void CModificationOptions::TraverseFromRootCopiesFirst 
    ( IModificationOption* option
    , CVisibleGraph* graph
    , CVisibleGraphNode* node)
{
    for (CVisibleGraphNode* next = node->GetNext(); node != NULL; node = next)
    {
        next = node->GetNext();

        // copies first

        for ( const CVisibleGraphNode::CCopyTarget* copy = node->GetFirstCopyTarget()
            , *nextCopy = NULL
            ; copy != NULL
            ; copy = nextCopy)
	    {
            nextCopy = copy->next();
            TraverseFromRootCopiesFirst (option, graph, copy->value());
        }

        // node afterwards

        option->Apply (graph, node);
    }
}

void CModificationOptions::TraverseToRootCopiesFirst 
    ( IModificationOption* option
    , CVisibleGraph* graph
    , CVisibleGraphNode* node)
{
    // crawl to branch end

    while (node->GetNext() != NULL)
        node = node->GetNext();

    for (CVisibleGraphNode* prev = node->GetPrevious(); node != NULL; node = prev)
    {
        prev = node->GetPrevious();

        // copies second

        for ( const CVisibleGraphNode::CCopyTarget* copy = node->GetFirstCopyTarget()
            , *nextCopy = NULL
            ; copy != NULL
            ; copy = nextCopy)
	    {
            nextCopy = copy->next();
            TraverseToRootCopiesFirst (option, graph, copy->value());
        }

        // node last

        option->Apply (graph, node);
    }
}

void CModificationOptions::TraverseFromRootCopiesLast 
    ( IModificationOption* option
    , CVisibleGraph* graph
    , CVisibleGraphNode* node)
{
    for (CVisibleGraphNode* next = node->GetNext(); node != NULL; node = next)
    {
        next = node->GetNext();

        // node first

        option->Apply (graph, node);

        // copies last

        for ( const CVisibleGraphNode::CCopyTarget* copy = node->GetFirstCopyTarget()
            , *nextCopy = NULL
            ; copy != NULL
            ; copy = nextCopy)
	    {
            nextCopy = copy->next();
            TraverseFromRootCopiesLast (option, graph, copy->value());
        }
    }
}

void CModificationOptions::TraverseToRootCopiesLast 
    ( IModificationOption* option
    , CVisibleGraph* graph
    , CVisibleGraphNode* node)
{
    // crawl to branch end

    while (node->GetNext() != NULL)
        node = node->GetNext();

    for (CVisibleGraphNode* prev = node->GetPrevious(); node != NULL; node = prev)
    {
        prev = node->GetPrevious();

        // node afterwards

        option->Apply (graph, node);

        // copies last

        for ( const CVisibleGraphNode::CCopyTarget* copy = node->GetFirstCopyTarget()
            , *nextCopy = NULL
            ; copy != NULL
            ; copy = nextCopy)
	    {
            nextCopy = copy->next();
            TraverseToRootCopiesLast (option, graph, copy->value());
        }
    }
}

// construction

CModificationOptions::CModificationOptions 
    ( const std::vector<IModificationOption*>& options)
    : options (options)
{
}

// apply all filters 

void CModificationOptions::Apply (CVisibleGraph* graph)
{
    typedef std::vector<IModificationOption*>::const_iterator IT;

    // apply filters until the graph is stable

    size_t nodeCount = 0;
    while (nodeCount != graph->GetNodeCount())
    {
        nodeCount = graph->GetNodeCount();
        for ( IT iter = options.begin(), end = options.end()
            ; (iter != end)
            ; ++iter)
        {
            for (size_t i = graph->GetRootCount(); i > 0; --i)
            {
                CVisibleGraphNode* root = graph->GetRoot (i-1);
                if ((*iter)->WantsCopiesFirst())
                    if ((*iter)->WantsRootFirst())
                        TraverseFromRootCopiesFirst (*iter, graph, root);
                    else
                        TraverseToRootCopiesFirst (*iter, graph, root);
                else
                    if ((*iter)->WantsRootFirst())
                        TraverseFromRootCopiesLast (*iter, graph, root);
                    else
                        TraverseToRootCopiesLast (*iter, graph, root);
            }
        }
    }
}

