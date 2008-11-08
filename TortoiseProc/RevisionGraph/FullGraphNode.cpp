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
#include "FullGraphNode.h"

// CVisibleGraphNode::CFactory methods

CFullGraphNode::CFactory::CFactory()
    : nodePool (sizeof (CFullGraphNode), 1024)
    , copyTargetFactory()
{
}

CFullGraphNode* CFullGraphNode::CFactory::Create 
    ( const CDictionaryBasedTempPath& path
    , revision_t revision
    , CNodeClassification classification
    , CFullGraphNode* source)
{
    CFullGraphNode * result = static_cast<CFullGraphNode *>(nodePool.malloc());
    new (result) CFullGraphNode (path, revision, classification, source, copyTargetFactory);
    return result;
}

void CFullGraphNode::CFactory::Replace ( CFullGraphNode* toReplace
                                       , CFullGraphNode::CCopyTarget*& toMove
                                       , CNodeClassification newClassification)
{
    // remove the old node

    CFullGraphNode* previousNode = toReplace->GetPrevious();
    Destroy (toReplace);

    // extract the new node

    CFullGraphNode* toInsert = copyTargetFactory.remove (toMove);

    // (re-)insert the new node

    toInsert->classification = newClassification;
    toInsert->InsertAt (previousNode, copyTargetFactory);
}

void CFullGraphNode::CFactory::Destroy (CFullGraphNode* node)
{
    node->DestroySubNodes (*this, copyTargetFactory);

    node->~CFullGraphNode();
    nodePool.free (node);
}

// CVisibleGraphNode construction / destruction 

CFullGraphNode::CFullGraphNode 
    ( const CDictionaryBasedTempPath& path
    , revision_t revision
    , CNodeClassification classification
    , CFullGraphNode* source
    , CCopyTarget::factory& copyTargetFactory)
    : path (path)
    , realPathID (path.GetBasePath().GetIndex())
	, firstCopyTarget(NULL)
    , prev (NULL)
    , next (NULL)
    , copySource (NULL)
    , revision (revision)
    , classification (classification)
{
    InsertAt (source, copyTargetFactory);
}

CFullGraphNode::~CFullGraphNode() 
{
    assert (next == NULL);
    assert (firstCopyTarget == NULL);
};

void CFullGraphNode::InsertAt ( CFullGraphNode* source
                              , CCopyTarget::factory& copyTargetFactory)
{
    if (source != NULL)
        if (classification.Is (CNodeClassification::IS_COPY_TARGET))
        {
            copySource = source;
            prev = NULL;
            copyTargetFactory.insert (this, source->firstCopyTarget);
        }
        else
        {
            copySource = NULL;
            source->next = this;
            prev = source;
        }
}

void CFullGraphNode::DestroySubNodes 
    ( CFactory& factory
    , CCopyTarget::factory& copyTargetFactory)
{
    while (next != NULL)
    {
        CFullGraphNode* toDestroy = next; 
        next = toDestroy->next;
        toDestroy->next = NULL;

        factory.Destroy (toDestroy);
    }

    while (firstCopyTarget)
    {
        factory.Destroy (copyTargetFactory.remove (firstCopyTarget));
    }
}
