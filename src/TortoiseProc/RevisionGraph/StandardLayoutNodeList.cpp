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
#include "resource.h"
#include "StandardLayoutNodeList.h"
#include "VisibleGraphNode.h"
#include "CachedLogInfo.h"
#include "SVN.h"
#include "UnicodeUtils.h"

/// utilities

index_t CStandardLayoutNodeList::GetStyle 
	(const CVisibleGraphNode* node) const
{
	CNodeClassification classification = node->GetClassification();

	if (classification.Is (CNodeClassification::IS_ADDED))
		return ILayoutNodeList::SNode::STYLE_ADDED;
	else if (classification.Is (CNodeClassification::IS_DELETED))
		return ILayoutNodeList::SNode::STYLE_DELETED;
	else if (classification.Is (CNodeClassification::IS_RENAMED))
		return ILayoutNodeList::SNode::STYLE_RENAMED;
	else if (classification.Is (CNodeClassification::IS_LAST))
		return ILayoutNodeList::SNode::STYLE_LAST;
    else if (classification.Is (CNodeClassification::IS_MODIFIED))
		return ILayoutNodeList::SNode::STYLE_MODIFIED;
	else
		return ILayoutNodeList::SNode::STYLE_DEFAULT;
}

DWORD CStandardLayoutNodeList::GetStyleFlags 
	(const CVisibleGraphNode* /*node*/) const
{
	return 0;
}

// construction

CStandardLayoutNodeList::CStandardLayoutNodeList 
    ( const std::vector<CStandardLayoutNodeInfo>& nodes
    , const CCachedLogInfo* cache)
    : cache (cache)
    , nodes (nodes)
{
}

// implement ILayoutItemList

index_t CStandardLayoutNodeList::GetCount() const
{
    return static_cast<index_t>(nodes.size());
}

CString CStandardLayoutNodeList::GetToolTip (index_t index) const
{
    CString strTipText;

    const CRevisionIndex& revisions = cache->GetRevisions();
    const CRevisionInfoContainer& revisionInfo = cache->GetLogInfo();

    const CVisibleGraphNode* node = nodes[index].node;

    // node properties

	CNodeClassification classification = node->GetClassification();
    revision_t revision = node->GetRevision();

    CString path 
        = CUnicodeUtils::StdGetUnicode 
            (node->GetPath().GetPath()).c_str();

    // special case: the "workspace is modified" node

    if (classification.Is (CNodeClassification::IS_MODIFIED_WC))
    {
	    strTipText.Format ( IDS_REVGRAPH_BOXTOOLTIP_WC
                          , revision-1
                          , (LPCTSTR)path);
        return strTipText;
    }

    // find the revision in our cache. 
    // Might not be present if this is the WC / HEAD revision
    // (but should not happen).

    index_t revisionIndex = revisions [revision];
    assert (revisionIndex != NO_INDEX);
    if (revisionIndex == NO_INDEX)
        return strTipText;

    // get standard revprops

	TCHAR date[SVN_DATE_BUFFER];
	apr_time_t timeStamp = revisionInfo.GetTimeStamp (revisionIndex);
	SVN::formatDate(date, timeStamp);

    CString author 
        = CUnicodeUtils::StdGetUnicode 
            (revisionInfo.GetAuthor (revisionIndex)).c_str();
    CString comment 
        = CUnicodeUtils::StdGetUnicode 
            (revisionInfo.GetComment (revisionIndex)).c_str();

    // description of the operation represented by this node

    CString revisionDescription;
	if (classification.Is (CNodeClassification::IS_COPY_TARGET))
    {
        if (classification.Is (CNodeClassification::IS_TAG))
            revisionDescription.LoadString (IDS_REVGRAPH_NODEIS_TAG);
	    else if (classification.Is (CNodeClassification::IS_BRANCH))
            revisionDescription.LoadString (IDS_REVGRAPH_NODEIS_BRANCH);
	    else
            revisionDescription.LoadString (IDS_REVGRAPH_NODEIS_COPY);
    }
	else if (classification.Is (CNodeClassification::IS_ADDED))
        revisionDescription.LoadString (IDS_REVGRAPH_NODEIS_ADDED);
	else if (classification.Is (CNodeClassification::IS_DELETED))
        revisionDescription.LoadString (IDS_REVGRAPH_NODEIS_DELETE);
	else if (classification.Is (CNodeClassification::IS_RENAMED))
        revisionDescription.LoadString (IDS_REVGRAPH_NODEIS_RENAME);
	else if (classification.Is (CNodeClassification::IS_LAST))
        revisionDescription.LoadString (IDS_REVGRAPH_NODEIS_HEAD);
    else if (classification.Is (CNodeClassification::IS_MODIFIED))
        revisionDescription.LoadString (IDS_REVGRAPH_NODEIS_MODIFIED);
	else
        revisionDescription.LoadString (IDS_REVGRAPH_NODEIS_COPYSOURCE);

    // copy-from info, if available

    CString copyFromLine;
    const CFullGraphNode* copySource = node->GetBase()->GetCopySource();
	if (copySource != NULL)
    {
        CString copyFromPath 
            = CUnicodeUtils::StdGetUnicode 
                (copySource->GetPath().GetPath()).c_str();
        revision_t copyFromRevision = copySource->GetRevision();

        copyFromLine.Format ( IDS_REVGRAPH_BOXTOOLTIP_COPYSOURCE
                            , (LPCTSTR)copyFromPath
                            , copyFromRevision);
    }

    // construct the tooltip

    if (node->GetFirstTag() == NULL)
    {
	    strTipText.Format ( IDS_REVGRAPH_BOXTOOLTIP
                          , revision, (LPCTSTR)revisionDescription
                          , (LPCTSTR)path, (LPCTSTR)copyFromLine
                          , (LPCTSTR)author, date, (LPCTSTR)comment);
    }
    else
    {
        CString tags;
        int tagCount = 0;
        for ( const CVisibleGraphNode::CFoldedTag* tag = node->GetFirstTag()
            ; tag != NULL
            ; tag = tag->GetNext())
        {
            ++tagCount;

            CString attributes;
            if (tag->IsModified())
                attributes.LoadString (IDS_REVGRAPH_TAGMODIFIED);

            if (tag->IsDeleted())
            {
                CString attribute;
                attribute.LoadString (IDS_REVGRAPH_TAGDELETED);
                if (attributes.IsEmpty())
                    attributes = attribute;
                else
                    attributes += _T(", ") + attribute;
            }

            CString tagInfo;
            std::string tagPath = tag->GetTag()->GetPath().GetPath();

            if (attributes.IsEmpty())
            {
                tagInfo.Format (   tag->IsAlias() 
                                 ? IDS_REVGRAPH_TAGALIAS 
                                 : IDS_REVGRAPH_TAG
                               , CUnicodeUtils::StdGetUnicode (tagPath).c_str());
            }
            else
            {
                tagInfo.Format (   tag->IsAlias() 
                                 ? IDS_REVGRAPH_TAGALIASATTRIBUTED
                                 : IDS_REVGRAPH_TAGATTRIBUTED
                               , (LPCTSTR)attributes
                               , CUnicodeUtils::StdGetUnicode (tagPath).c_str());
            }

            tags +=   _T("\r\n")
                    + CString (' ', tag->GetDepth() * 6) 
                    + tagInfo;
        }

	    strTipText.Format ( IDS_REVGRAPH_BOXTOOLTIP_TAGGED
                          , revision, (LPCTSTR)revisionDescription
                          , (LPCTSTR)path, (LPCTSTR)copyFromLine
                          , (LPCTSTR)author, date, tagCount, (LPCTSTR)tags
                          , (LPCTSTR)comment);
    }

    // ready

    return strTipText;
}

index_t CStandardLayoutNodeList::GetFirstVisible (const CRect& viewRect) const
{
    return GetNextVisible (static_cast<index_t>(-1), viewRect);
}

index_t CStandardLayoutNodeList::GetNextVisible ( index_t prev
                                                , const CRect& viewRect) const
{
    for (size_t i = prev+1, count = nodes.size(); i < count; ++i)
        if (FALSE != CRect().IntersectRect (nodes[i].rect, viewRect))
            return static_cast<index_t>(i);

    return static_cast<index_t>(NO_INDEX);
}

index_t CStandardLayoutNodeList::GetAt (const CPoint& point, CSize delta) const
{
    for (size_t i = 0, count = nodes.size(); i < count; ++i)
    {
        const CRect& rect = nodes[i].rect;
        if (   (rect.top - point.y <= delta.cy)
            && (rect.left - point.x <= delta.cx)
            && (point.y - rect.bottom <= delta.cy)
            && (point.x - rect.right <= delta.cx))
        {
            return static_cast<index_t>(i);
        }
    }

    return static_cast<index_t>(NO_INDEX);
}

// implement ILayoutNodeList

CStandardLayoutNodeList::SNode 
CStandardLayoutNodeList::GetNode (index_t index) const
{
    SNode result;

	const CVisibleGraphNode* node = nodes[index].node;

    result.rect = nodes[index].rect;
    result.node = node;
    result.style = GetStyle (node);
    result.styleFlags = GetStyleFlags (node);

    return result;
}
