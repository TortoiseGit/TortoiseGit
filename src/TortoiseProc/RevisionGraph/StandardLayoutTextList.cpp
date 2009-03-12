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
#include "StandardLayoutTextList.h"
#include "UnicodeUtils.h"
#include "VisibleGraphNode.h"
#include "resource.h"

// construction

CStandardLayoutTextList::CStandardLayoutTextList 
    ( const std::vector<CStandardLayoutNodeInfo>& nodes
    , const std::vector<CStandardLayout::STextInfo>& texts)
    : nodes (nodes)
    , texts (texts)
{
}

// implement ILayoutItemList

index_t CStandardLayoutTextList::GetCount() const
{
    return static_cast<index_t>(texts.size());
}

CString CStandardLayoutTextList::GetToolTip (index_t /* index */) const
{
    return CString();
}

index_t CStandardLayoutTextList::GetFirstVisible 
    (const CRect& viewRect) const
{
    return GetNextVisible (static_cast<index_t>(-1), viewRect);
}

index_t CStandardLayoutTextList::GetNextVisible 
    ( index_t prev
    , const CRect& viewRect) const
{
    for (size_t i = prev+1, count = texts.size(); i < count; ++i)
    {
        const CStandardLayout::STextInfo& text = texts[i];
        const CRect& nodeRect = nodes[text.nodeIndex].rect;

        if (FALSE != CRect().IntersectRect (nodeRect, viewRect))
            return static_cast<index_t>(i);
    }

    return static_cast<index_t>(NO_INDEX);
}

index_t CStandardLayoutTextList::GetAt 
    ( const CPoint& /* point */
    , CSize /* delta */) const
{
    return static_cast<index_t>(NO_INDEX);
}

// implement ILayoutTextList

CStandardLayoutTextList::SText 
CStandardLayoutTextList::GetText (index_t index) const
{
    // determine the text and its bounding rect

    const CStandardLayout::STextInfo& textInfo = texts[index];
    const CStandardLayoutNodeInfo& nodeInfo = nodes[textInfo.nodeIndex];

    bool isModifiedWC = nodeInfo.node->GetClassification().Is 
                            (CNodeClassification::IS_MODIFIED_WC);

    CString text;
    CRect rect = nodeInfo.rect;
    if (textInfo.subPathIndex > 0)
    {
        rect.top = rect.top + 25 + 21 * (textInfo.subPathIndex-1);

        size_t index = textInfo.subPathIndex-1 + nodeInfo.skipStartPathElements;
        const CDictionaryBasedTempPath& path = nodeInfo.node->GetPath();
        size_t visibleElementCount = path.GetDepth() 
                                   - nodeInfo.skipStartPathElements
                                   - nodeInfo.skipTailPathElements;
        text = CUnicodeUtils::StdGetUnicode (path[index]).c_str();
        text.Insert (0, _T('/'));

        // add "...." pre- and post-fixes, if elements have been skipped

        if ((textInfo.subPathIndex == 1) && (nodeInfo.skipStartPathElements > 0))
            text.Insert (0, CString ('.', nodeInfo.skipStartPathElements));

        if (   (visibleElementCount == (size_t)textInfo.subPathIndex) 
            && (nodeInfo.skipTailPathElements != 0))
        {
            text.AppendChar (_T('/'));
            text.Append (CString ('.', nodeInfo.skipTailPathElements));
        }
    }
    else
    {
        rect.top += 4;
        if (isModifiedWC)
        {
            text.LoadString (IDS_SVN_SUMMARIZEMODIFIED);
        }
        else
        {
            TCHAR buffer[20];
            _itot_s (nodeInfo.node->GetRevision(), buffer, 10);
            text = buffer;
        }
    }

    // construct result

    SText result;

    result.style = textInfo.subPathIndex == 0
                 ? isModifiedWC ? SText::STYLE_WARNING
                                : SText::STYLE_HEADING
                 : SText::STYLE_DEFAULT;
    result.rotation = 0;
    result.rect = rect;
    result.text = text;

    return result;
}
