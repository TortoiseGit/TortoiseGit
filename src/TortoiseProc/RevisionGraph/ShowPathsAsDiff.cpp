// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2009 - TortoiseSVN

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
#include "ShowPathsAsDiff.h"
#include "StandardLayout.h"
#include "VisibleGraphNode.h"

// construction

CShowPathsAsDiff::CShowPathsAsDiff (CRevisionGraphOptionList& list)
    : inherited (list)
{
}

// cast @a layout pointer to the respective modification
// interface and write the data.

void CShowPathsAsDiff::ApplyTo (IRevisionGraphLayout* layout)
{
    // we need access to actual data

    IStandardLayoutNodeAccess* nodeAccess 
        = dynamic_cast<IStandardLayoutNodeAccess*>(layout);
    if (nodeAccess == NULL) 
        return;

    // calculate the path diffs for each node

    for (index_t i = 0, count = nodeAccess->GetNodeCount(); i < count; ++i)
    {
        CStandardLayoutNodeInfo* nodeInfo = nodeAccess->GetNode(i);

        const CVisibleGraphNode* node = nodeInfo->node;
        const CVisibleGraphNode* source = node->GetSource();

        if (source != NULL)
        {
            const CDictionaryBasedTempPath& nodePath = node->GetPath();
            const CDictionaryBasedTempPath& sourcePath = source->GetPath();

            // optimization: in most cases, paths will be equal

            if (nodePath == sourcePath)
            {
                nodeInfo->skipStartPathElements = nodePath.GetDepth();
            }
            else
            {
                // determine the lengths of the unchanged head and tail

                CDictionaryBasedTempPath commonRoot 
                    = nodePath.GetCommonRoot (sourcePath);

                size_t nodeDepth = nodePath.GetDepth();
                size_t sourceDepth = sourcePath.GetDepth();
                size_t commonDepth = commonRoot.GetDepth();

                size_t maxTail = min (nodeDepth, sourceDepth) - commonDepth;
                size_t tail = 0;
                for (; tail < maxTail; ++tail)
                    if (   nodePath[nodeDepth - tail - 1] 
                        != sourcePath[sourceDepth - tail - 1])
                        break;

                // special case: no change but elements have been ommitted
                // (e.g. copy to parent)

                if ((commonDepth + tail == nodeDepth) && (nodeDepth > 0))
                {
                    // show at least one element

                    if (tail > 0)
                    {
                        // show that the tail has been moved up

                        --tail;
                    }
                    else
                    {
                        // show that we cut off some levels

                        --commonDepth;
                    }
                }

                // store results

                nodeInfo->skipStartPathElements = commonDepth;
                nodeInfo->skipTailPathElements = tail;
            }
        }
    }
}
