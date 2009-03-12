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

// path classification by cache

CRemovePathsBySubString::PathClassification 
CRemovePathsBySubString::Classify (const std::string& path) const
{
    // we have to attempt full string searches as we look for 
    // arbitrary sub-strings

	for ( std::set<std::string>::const_iterator iter = filterPaths.begin()
        , end = filterPaths.end()
        ; iter != end
        ; ++iter)
    {
        if (path.find (*iter) != std::string::npos)
            return REMOVE;
    }

    // done here

    return KEEP;
}

CRemovePathsBySubString::PathClassification 
CRemovePathsBySubString::QuickClassification (const CDictionaryBasedPath& path) const
{
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
        classification = Classify (path.GetPath());

    // done here

    return classification;
}

// construction

CRemovePathsBySubString::CRemovePathsBySubString (CRevisionGraphOptionList& list)
    : inherited (list)
    , removeSubTrees (false)
{
}

// access to the sub-string container

const std::set<std::string>& CRemovePathsBySubString::GetFilterPaths() const
{
    return filterPaths;
}

std::set<std::string>& CRemovePathsBySubString::GetFilterPaths()
{
    return filterPaths;
}

// access to removal behavior

bool CRemovePathsBySubString::GetRemoveSubTrees() const
{
    return removeSubTrees;
}

void CRemovePathsBySubString::SetRemoveSubTrees (bool value)
{
    removeSubTrees = value;
}

// implement IRevisionGraphOption

bool CRemovePathsBySubString::IsActive() const
{
    return !filterPaths.empty();
}

// implement ICopyFilterOption

ICopyFilterOption::EResult 
CRemovePathsBySubString::ShallRemove (const CFullGraphNode* node) const
{
    // short-cut

    if (filterPaths.empty())
        return ICopyFilterOption::KEEP_NODE;

    // path to classify

    const CDictionaryBasedTempPath& path = node->GetPath();

    // most paths can be filtered quickly using the classification cache

    PathClassification classification = QuickClassification (path.GetBasePath());

    // take a closer look if necessary

    if ((classification != REMOVE) && !path.IsFullyCachedPath())
        classification = Classify (path.GetPath());

    // return the result

    return classification == REMOVE
        ? removeSubTrees ? ICopyFilterOption::REMOVE_SUBTREE 
                         : ICopyFilterOption::REMOVE_NODE
        : ICopyFilterOption::KEEP_NODE;
}

void CRemovePathsBySubString::Prepare()
{
    pathClassification.clear();
}
