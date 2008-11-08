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
#include "VisibleGraphNode.h"
#include "VisibleGraph.h"

// CVisibleGraphNode::CFoldedTag methods

bool CVisibleGraphNode::CFoldedTag::IsAlias() const
{
    const CFullGraphNode* prev 
        = tagNode->GetPrevious() == NULL
            ? tagNode->GetCopySource()
            : tagNode->GetPrevious();

    // skip all non-modifying nodes and make prev point to
    // either the copy node or the last modification

    while (   (prev != NULL) 
           && (!prev->GetClassification().IsAnyOf 
                  (CNodeClassification::IS_OPERATION_MASK)))
    {
        prev = prev->GetPrevious();
    }

    // it's an alias if the previous node is a tag and has
    // not been modified since

    return    (prev != NULL) 
           && prev->GetClassification().Is (CNodeClassification::IS_TAG)
           && prev->GetClassification().IsAnyOf (  CNodeClassification::IS_ADDED
                                                 + CNodeClassification::IS_RENAMED);
}

// CVisibleGraphNode::CFactory methods

CVisibleGraphNode::CFactory::CFactory()
    : nodePool (sizeof (CVisibleGraphNode), 1024)
    , tagPool (sizeof (CFoldedTag), 1024)
    , copyTargetFactory()
    , nodeCount (0)
{
}

// factory interface

CVisibleGraphNode* CVisibleGraphNode::CFactory::Create 
    ( const CFullGraphNode* base
    , CVisibleGraphNode* prev
    , bool preserveNode)
{
    CVisibleGraphNode * result = static_cast<CVisibleGraphNode *>(nodePool.malloc());
    new (result) CVisibleGraphNode (base, prev, copyTargetFactory, preserveNode);

    ++nodeCount;
    return result;
}

void CVisibleGraphNode::CFactory::Destroy (CVisibleGraphNode* node)
{
    node->DestroySubNodes (*this, copyTargetFactory);
    node->DestroyTags (*this);

    node->~CVisibleGraphNode();
    nodePool.free (node);

    --nodeCount;
}

CVisibleGraphNode::CFoldedTag* CVisibleGraphNode::CFactory::Create 
    ( const CFullGraphNode* tagNode
    , size_t depth
    , CFoldedTag* next)
{
    CFoldedTag * result = static_cast<CFoldedTag *>(tagPool.malloc());
    new (result) CFoldedTag (tagNode, depth, next);
    return result;
}

void CVisibleGraphNode::CFactory::Destroy (CFoldedTag* tag)
{
    tag->~CFoldedTag();
    tagPool.free (tag);
}

// CVisibleGraphNode construction / destruction 

CVisibleGraphNode::CVisibleGraphNode 
    ( const CFullGraphNode* base
    , CVisibleGraphNode* prev
    , CCopyTarget::factory& copyTargetFactory
    , bool preserveNode)
	: base (base)
	, firstCopyTarget (NULL), firstTag (NULL)
	, prev (NULL), next (NULL), copySource (NULL)
    , classification (  preserveNode 
                      ? base->GetClassification().GetFlags()
                      : (   base->GetClassification().GetFlags() 
                          | CNodeClassification::MUST_BE_PRESERVED))
	, index ((index_t)NO_INDEX) 
{
    if (prev != NULL)
        if (classification.Is (CNodeClassification::IS_COPY_TARGET))
        {
            copySource = prev;
            copyTargetFactory.insert (this, prev->firstCopyTarget);
        }
        else
        {
            assert (prev->next == NULL);

            prev->next = this;
            this->prev = prev;
        }
}

CVisibleGraphNode::~CVisibleGraphNode() 
{
    assert (next == NULL);
    assert (firstCopyTarget == NULL);
    assert (firstTag == NULL);
};

// destruction utilities

void CVisibleGraphNode::DestroySubNodes 
    ( CFactory& factory
    , CCopyTarget::factory& copyTargetFactory)
{
    while (next != NULL)
    {
        CVisibleGraphNode* toDestroy = next; 
        next = toDestroy->next;
        toDestroy->next = NULL;

        factory.Destroy (toDestroy);
    }

    while (firstCopyTarget)
    {
        CVisibleGraphNode* target = copyTargetFactory.remove (firstCopyTarget);
        if (target != NULL)
            factory.Destroy (target);
    }
}

void CVisibleGraphNode::DestroyTags (CFactory& factory)
{
    while (firstTag != NULL)
    {
        CFoldedTag* toDestroy = firstTag; 
        firstTag = toDestroy->next;
        factory.Destroy (toDestroy);
    }
}

// set index members within the whole sub-tree

index_t CVisibleGraphNode::InitIndex (index_t startIndex)
{
    for (CVisibleGraphNode* node = this; node != NULL; node = node->next)
    {
        node->index = startIndex;
        ++startIndex;

        for ( CCopyTarget* target = node->firstCopyTarget
            ; target != NULL
            ; target = target->next())
        {
            startIndex = target->value()->InitIndex (startIndex);
        }
    }

    return startIndex;
}

// remove node and move links to pre-decessor

void CVisibleGraphNode::DropNode (CVisibleGraph* graph)
{
    // previous node to receive all our links

    CVisibleGraphNode* target = copySource == NULL
                              ? prev
                              : copySource;

    // special case: remove one of the roots

    if (target == NULL)
    {
        // handle this branch

        if (next)
        {
            next->prev = NULL;
            graph->ReplaceRoot (this, next);
        }
        else
        {
            graph->RemoveRoot (this);
        }

        // add sub-branches as new roots

        for (; firstCopyTarget != NULL; firstCopyTarget = firstCopyTarget->next())
        {
            firstCopyTarget->value()->copySource = NULL;
            graph->AddRoot (firstCopyTarget->value());
        }

        // no tag-folding supported here

        assert (firstTag == NULL);
    }
    else
    {
        // move all branches

        if (firstCopyTarget != NULL)
        {
            // find insertion point

            CCopyTarget** targetFirstCopyTarget = &target->firstCopyTarget;
            while (*targetFirstCopyTarget != NULL)
                targetFirstCopyTarget = &(*targetFirstCopyTarget)->next();

            // concatenate list

            *targetFirstCopyTarget = firstCopyTarget;

            // adjust copy sources and reset firstCopyTarget

            for (; firstCopyTarget != NULL; firstCopyTarget = firstCopyTarget->next())
                firstCopyTarget->value()->copySource = target;
        }

        // move all tags

        if (firstTag != NULL)
        {
            // find insertion point

            CFoldedTag** targetFirstTag = &target->firstTag;
            while (*targetFirstTag != NULL)
                targetFirstTag = &(*targetFirstTag)->next;

            // concatenate list and reset firstTag

            *targetFirstTag = firstTag;
            firstTag = NULL;
        }

        // de-link this node

        if (prev != NULL)
        {
            prev->next = next;
            if (next)
                next->prev = prev;
        }
        else
        {
            // find the copy struct that links to *this

            CCopyTarget** copy = &target->firstCopyTarget;
            for (
                ; (*copy != NULL) && ((*copy)->value() != this)
                ; copy = &(*copy)->next())
            {
            }

            assert (*copy != NULL);

            // make it point to next or remove it

            if (next)
            {
                (*copy)->value() = next;
                next->prev = NULL;
                next->copySource = target;
            }
            else
            {
                // remove from original list and attach it to *this for destruction

                firstCopyTarget = *copy;
                *copy = (*copy)->next();

                firstCopyTarget->next() = NULL;
                firstCopyTarget->value() = NULL;
            }
        }
    }

    // destruct this

    next = NULL;
    graph->GetFactory().Destroy (this);
}

// remove node and add it as folded tag to the parent

void CVisibleGraphNode::FoldTag (CVisibleGraph* graph)
{
    assert ((copySource || classification.Is (CNodeClassification::IS_RENAMED))
            && "This operation is only valid for copy nodes!");

    // fold the whole branch into this node.
    // Handle renames as sub-tags

    CVisibleGraphNode* node = this;
    while (node->next)
        node = node->next;

    while (node != this)
    {
        CVisibleGraphNode* previous = node->prev;
        if (node->classification.Is (CNodeClassification::IS_RENAMED))
            node->FoldTag (graph);
        else
            node->DropNode (graph);

        node = previous;
    }

    // fold all sub-branches as tags into this node

    while (firstCopyTarget != NULL)
        firstCopyTarget->value()->FoldTag (graph);

    // move tags to parent

    CVisibleGraphNode* source = copySource == NULL ? prev : copySource;
    if (firstTag != NULL)
    {
        CFoldedTag* lastTag = NULL;
        for (CFoldedTag* tag = firstTag; tag != NULL; tag = tag->next)
        {
            tag->depth++;
            lastTag = tag;
        }

        lastTag->next = source->firstTag;
        source->firstTag = firstTag;
        firstTag = NULL;
    }

    // create a tag for this node

    CFoldedTag* newTag 
        = graph->GetFactory().Create (base, 0, source->firstTag);
    source->firstTag = newTag;

    // remove this node

    DropNode (graph);
}
