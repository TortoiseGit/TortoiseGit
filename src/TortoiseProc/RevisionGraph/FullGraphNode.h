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

#include "DictionaryBasedTempPath.h"
#include "SimpleList.h"
#include "NodeClassification.h"

using namespace LogCache;

/**
 * \ingroup TortoiseProc
 * Helper class, representing a revision with all the required information
 * which we need to draw a revision graph.
 */
class CFullGraphNode
{
public:

    /// copy target list type

    typedef simple_list<CFullGraphNode> CCopyTarget;

    /// factory type

    class CFactory
    {
    private:

        boost::pool<> nodePool;
        CCopyTarget::factory copyTargetFactory;

    public:

        /// factory creation

        CFactory();

        /// factory interface

        CFullGraphNode* Create ( const CDictionaryBasedTempPath& path
                               , revision_t revision
                               , CNodeClassification classification
                               , CFullGraphNode* source);
        void Replace ( CFullGraphNode* toReplace
                     , CFullGraphNode::CCopyTarget*& toMove
                     , CNodeClassification newClassification);
        void Destroy (CFullGraphNode* node);
    };

    friend class CFactory;

private:

	///members

	CDictionaryBasedTempPath path;
	index_t              realPathID;

	CCopyTarget*         firstCopyTarget;

	CFullGraphNode*      prev;
	CFullGraphNode*      next;

	CFullGraphNode*      copySource;

	revision_t		     revision;
    CNodeClassification  classification;

protected:

	/// protect construction / destruction to force usage of pool

	CFullGraphNode ( const CDictionaryBasedTempPath& path
                   , revision_t revision
                   , CNodeClassification classification
                   , CFullGraphNode* source
                   , CCopyTarget::factory& copyTargetFactory);
    ~CFullGraphNode();

    /// destruction utility

    void InsertAt ( CFullGraphNode* source
                  , CCopyTarget::factory& copyTargetFactory);
    void DestroySubNodes ( CFactory& factory
                         , CCopyTarget::factory& copyTargetFactory);

public:

    /// modification

    void AddClassification (DWORD toAdd);

    /// data access

	const CDictionaryBasedTempPath& GetPath() const;
	CDictionaryBasedPath GetRealPath() const;

	const CFullGraphNode* GetCopySource() const;
	const CCopyTarget* GetFirstCopyTarget() const;
	CCopyTarget*& GetFirstCopyTarget();

	const CFullGraphNode* GetPrevious() const;
	CFullGraphNode* GetPrevious();
	const CFullGraphNode* GetNext() const;
	CFullGraphNode* GetNext();

	revision_t GetRevision() const;
	CNodeClassification GetClassification() const;

};

/// CVisibleGraphNode  modification

inline void CFullGraphNode::AddClassification (DWORD toAdd)
{
    classification.Add (toAdd);
}

/// CVisibleGraphNode data access

inline const CDictionaryBasedTempPath& CFullGraphNode::GetPath() const
{
    return path;
}

inline CDictionaryBasedPath CFullGraphNode::GetRealPath() const
{
    return CDictionaryBasedPath (path.GetBasePath().GetDictionary(), realPathID);
}

inline const CFullGraphNode* CFullGraphNode::GetCopySource() const
{
    return copySource;
}

inline const CFullGraphNode::CCopyTarget* 
CFullGraphNode::GetFirstCopyTarget() const
{
    return firstCopyTarget;
}

inline CFullGraphNode::CCopyTarget*& CFullGraphNode::GetFirstCopyTarget()
{
    return firstCopyTarget;
}

inline const CFullGraphNode* CFullGraphNode::GetPrevious() const
{
    return prev;
}

inline CFullGraphNode* CFullGraphNode::GetPrevious()
{
    return prev;
}

inline const CFullGraphNode* CFullGraphNode::GetNext() const
{
    return next;
}

inline CFullGraphNode* CFullGraphNode::GetNext()
{
    return next;
}

inline revision_t CFullGraphNode::GetRevision() const
{
    return revision;
}

inline CNodeClassification CFullGraphNode::GetClassification() const
{
    return classification;
}

