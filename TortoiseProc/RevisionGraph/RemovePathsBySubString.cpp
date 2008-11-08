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
#include "RemovePathsBySubString.h"
#include "FullGraphNode.h"

// construction

CRemovePathsBySubString::CRemovePathsBySubString (CRevisionGraphOptionList& list)
    : inherited (list)
{
}

// get / set limits

/// access to the sub-string container

const std::set<std::string>& CRemovePathsBySubString::GetFilterPaths() const
{
    return filterPaths;
}

std::set<std::string>& CRemovePathsBySubString::GetFilterPaths()
{
    return filterPaths;
}

// implement ICopyFilterOption

ICopyFilterOption::EResult 
CRemovePathsBySubString::ShallRemove (const CFullGraphNode* node) const
{
    // short-cut

    if (filterPaths.empty())
        return ICopyFilterOption::KEEP_NODE;

    // node to classify

    const CDictionaryBasedPath& path = node->GetRealPath();

    // ensure the index is valid within classification cache 

    if (pathClassification.size() <= path.GetIndex())
    {
        size_t newSize = max (8, pathClassification.size()) * 2;
        while (newSize <= path.GetIndex())
            newSize *= 2;

        pathClassification.resize (newSize, UNKNOWN);
    }

    // auto-calculate the entry

    PathClassification& classification = pathClassification[path.GetIndex()];
    if (classification == UNKNOWN)
    {
        std::string fullPath = path.GetPath();

        classification = KEEP;
    	for ( std::set<std::string>::const_iterator iter = filterPaths.begin()
            , end = filterPaths.end()
            ; iter != end
            ; ++iter)
        {
            if (fullPath.find (*iter) != std::string::npos)
            {
                classification = REMOVE;
                break;
            }
        }
    }

    // return the result

    return classification == REMOVE
        ? ICopyFilterOption::REMOVE_NODE
        : ICopyFilterOption::KEEP_NODE;
}

void CRemovePathsBySubString::Prepare()
{
    pathClassification.clear();
}
