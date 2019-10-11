// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2003-2011 - TortoiseSVN
// Copyright (C) 2012-2019 - TortoiseGit

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
#include "stdafx.h"
#include "TortoiseProc.h"
#include <gdiplus.h>
#include "Revisiongraphdlg.h"
#include "Git.h"
#include "TempFile.h"
#include "UnicodeUtils.h"
#include "TGitPath.h"
#include "DPIAware.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

using namespace Gdiplus;

void CRevisionGraphWnd::InitView()
{
	m_bIsCanvasMove = false;

	SetScrollbars();
}

void CRevisionGraphWnd::BuildPreview()
{
	m_Preview.DeleteObject();
	if (!m_bShowOverview)
		return;

	// is there a point in drawing this at all?

	int nodeCount = this->m_Graph.numberOfNodes();
	if ((nodeCount > REVGRAPH_PREVIEW_MAX_NODES) || (nodeCount == 0))
		return;

	float origZoom = m_fZoomFactor;

	CRect clientRect = GetClientRect();
	CSize preViewSize(max(CDPIAware::Instance().ScaleX(REVGRAPH_PREVIEW_WIDTH), clientRect.Width() / 4),
					  max(CDPIAware::Instance().ScaleY(REVGRAPH_PREVIEW_HEIGHT), clientRect.Height() / 4));

	// zoom the graph so that it is completely visible in the window
	CRect graphRect = GetGraphRect();
	float horzfact = float(graphRect.Width())/float(preViewSize.cx);
	float vertfact = float(graphRect.Height())/float(preViewSize.cy);
	m_previewZoom = min (DEFAULT_ZOOM, 1.0f/(max(horzfact, vertfact)));

	// make sure the preview window has a minimal size

	m_previewWidth = min(static_cast<LONG>(max(graphRect.Width() * m_previewZoom, 30.0f)), preViewSize.cx);
	m_previewHeight = min(static_cast<LONG>(max(graphRect.Height() * m_previewZoom, 30.0f)), preViewSize.cy);

	CClientDC ddc(this);
	CDC dc;
	if (!dc.CreateCompatibleDC(&ddc))
		return;

	m_Preview.CreateCompatibleBitmap(&ddc, m_previewWidth, m_previewHeight);
	HBITMAP oldbm = static_cast<HBITMAP>(dc.SelectObject (m_Preview));

	// paint the whole graph
	DoZoom (m_previewZoom, false);
	CRect rect (0, 0, m_previewWidth, m_previewHeight);
	GraphicsDevice dev;
	dev.pDC = &dc;
	DrawGraph(dev, rect, 0, 0, true);

	// now we have a bitmap the size of the preview window
	dc.SelectObject(oldbm);
	dc.DeleteDC();

	DoZoom (origZoom, false);
}

void CRevisionGraphWnd::SetScrollbar (int bar, int newPos, int clientMax, int graphMax)
{
	SCROLLINFO ScrollInfo = {sizeof(SCROLLINFO), SIF_ALL};
	GetScrollInfo (bar, &ScrollInfo);

	clientMax = max(1, clientMax);
	int oldHeight = ScrollInfo.nMax <= 0 ? clientMax : ScrollInfo.nMax;
	int newHeight = static_cast<int>(graphMax * m_fZoomFactor);
	int maxPos = max (0, newHeight - clientMax);
	int pos = min (maxPos, newPos >= 0
						 ? newPos
						 : ScrollInfo.nPos * newHeight / oldHeight);

	ScrollInfo.nPos = pos;
	ScrollInfo.nMin = 0;
	ScrollInfo.nMax = newHeight;
	ScrollInfo.nPage = clientMax;
	ScrollInfo.nTrackPos = pos;

	SetScrollInfo(bar, &ScrollInfo);
}

void CRevisionGraphWnd::SetScrollbars (int nVert, int nHorz)
{
	CRect clientrect = GetClientRect();
	const CRect& pRect = GetGraphRect();

	SetScrollbar (SB_VERT, nVert, clientrect.Height(), pRect.Height());
	SetScrollbar (SB_HORZ, nHorz, clientrect.Width(), pRect.Width());
}

CRect CRevisionGraphWnd::GetGraphRect()
{
	return m_GraphRect;
}

CRect CRevisionGraphWnd::GetClientRect()
{
	CRect clientRect;
	CWnd::GetClientRect (&clientRect);
	return clientRect;
}

CRect CRevisionGraphWnd::GetWindowRect()
{
	CRect windowRect;
	CWnd::GetWindowRect (&windowRect);
	return windowRect;
}

CRect CRevisionGraphWnd::GetViewRect()
{
	CRect result;
	result.UnionRect (GetClientRect(), GetGraphRect());
	return result;
}

int CRevisionGraphWnd::GetEncoderClsid(const WCHAR* format, CLSID* pClsid)
{
	UINT num = 0;		// number of image encoders
	UINT size = 0;		 // size of the image encoder array in bytes

	if (GetImageEncodersSize(&num, &size)!=Ok)
		return -1;
	if(size == 0)
		return -1;  // Failure

	auto pImageCodecInfo = reinterpret_cast<ImageCodecInfo*>(malloc(size));
	if (!pImageCodecInfo)
		return -1;  // Failure

	if (GetImageEncoders(num, size, pImageCodecInfo)==Ok)
	{
		for(UINT j = 0; j < num; ++j)
		{
			if( wcscmp(pImageCodecInfo[j].MimeType, format) == 0 )
			{
				*pClsid = pImageCodecInfo[j].Clsid;
				free(pImageCodecInfo);
				return j;  // Success
			}
		}

	}
	free(pImageCodecInfo);
	return -1;  // Failure
}

bool CRevisionGraphWnd::FetchRevisionData
	( const CString& /*path*/
	, CProgressDlg* /*progress*/
	, ITaskbarList3 * /*pTaskbarList*/
	, HWND /*hWnd*/)
{
	this->m_LogCache.ClearAllParent();
	this->m_logEntries.ClearAll();
	CString range;
	if (!m_ToRev.IsEmpty() && !m_FromRev.IsEmpty())
		range.Format(L"%s..%s", static_cast<LPCTSTR>(g_Git.FixBranchName(m_FromRev)), static_cast<LPCTSTR>(g_Git.FixBranchName(m_ToRev)));
	else if (!m_ToRev.IsEmpty())
		range = m_ToRev;
	else if (!m_FromRev.IsEmpty())
		range = m_FromRev;
	DWORD infomask = CGit::LOG_INFO_SIMPILFY_BY_DECORATION | (m_bCurrentBranch ? 0 : m_bLocalBranches ? CGit::LOG_INFO_LOCAL_BRANCHES : CGit::LOG_INFO_ALL_BRANCH) | (m_bShowBranchingsMerges ? CGit::LOG_ORDER_TOPOORDER | CGit::LOG_INFO_SPARSE : 0);
	m_logEntries.ParserFromLog(nullptr, 0, infomask, &range);

	ReloadHashMap();
	this->m_Graph.clear();

	m_superProjectHash.Empty();
	if (CRegDWORD(L"Software\\TortoiseGit\\LogShowSuperProjectSubmodulePointer", TRUE) != FALSE)
		m_superProjectHash = g_Git.GetSubmodulePointer();

	// build child graph
	if (!m_bShowAllTags || m_bShowBranchingsMerges)
	{
		std::unordered_map<CGitHash, std::vector<CGitHash>> childMap;
		for (size_t i = 0; i < m_logEntries.size(); ++i)
		{
			const GitRev& rev = m_logEntries.GetGitRevAt(i);
			std::for_each(rev.m_ParentHash.cbegin(), rev.m_ParentHash.cend(), [&](const auto& parent) { childMap[parent].push_back(m_logEntries[i]); });
		}

		// rewrite history
		std::unordered_set<CGitHash> skipList;
		for (size_t i = 0; i < m_logEntries.size(); ++i)
		{
			auto& rev = m_logEntries.GetGitRevAt(i);

			// keep labeled commits
			if (auto foundNames = m_HashMap.find(rev.m_CommitHash); foundNames != m_HashMap.cend() || rev.m_CommitHash == m_superProjectHash)
			{
				// by default any label is enough to keep this commit visible
				if (m_bShowAllTags || rev.m_CommitHash == m_superProjectHash)
					continue;

				// if hiding tags, check if there are any branch names for this commit
				bool haveNonTagNames = false;
				for (auto name : foundNames->second)
				{
					CGit::REF_TYPE refType;
					CGit::GetShortName(name, &refType);
					if (refType != CGit::REF_TYPE::ANNOTATED_TAG && refType != CGit::REF_TYPE::TAG)
					{
						haveNonTagNames = true;
						break;
					}
				}
				if (haveNonTagNames)
					continue;
			}

			if (rev.m_ParentHash.size() != 1)
				continue;

			auto childIt = childMap.find(rev.m_CommitHash);
			if (childIt == childMap.cend() || childIt->second.size() != 1)
				continue;

			auto& childRev = m_logEntries.GetGitRevAt(m_logEntries.m_HashMap.find(childIt->second[0])->second);
			if (childRev.m_ParentHash.size() != 1)
				continue;

			// it's ok to erase this commit
			skipList.insert(rev.m_CommitHash);

			childRev.m_ParentHash[0] = rev.m_ParentHash[0];

			auto& parentChildren = childMap[rev.m_ParentHash[0]];
			if (auto it = std::find(parentChildren.begin(), parentChildren.end(), rev.m_CommitHash); it != parentChildren.end())
				*it = childIt->second[0];

			childMap.erase(childIt);
		}

		// cleanup lists
		m_logEntries.erase(std::remove_if(m_logEntries.begin(), m_logEntries.end(), [&skipList](const auto& hash) { return skipList.find(hash) != skipList.cend(); }), m_logEntries.end());
		m_logEntries.m_HashMap.clear();
		for (size_t i = 0; i < m_logEntries.size(); ++i)
			m_logEntries.m_HashMap[m_logEntries[i]] = i;
	}

	// build revision graph
	CArray<ogdf::node> nodes;
	GraphicsDevice dev;
	dev.pDC = this->GetDC();
	dev.graphics = Graphics::FromHDC(dev.pDC->m_hDC);
	dev.graphics->SetPageUnit (UnitPixel);

	Gdiplus::Font font(CAppUtils::GetLogFontName(), static_cast<REAL>(m_nFontSize), FontStyleRegular);
	Rect commitString;
	MeasureTextLength(dev, font, CString(L'8', g_Git.GetShortHASHLength()), commitString.Width, commitString.Height);

	m_HeadNode = nullptr;

	for (const auto& hash : m_logEntries)
	{
		auto nd = m_Graph.newNode();
		nodes.Add(nd);
		SetNodeRect(dev, font, commitString, &nd, hash);
		if (hash == m_HeadHash)
			m_HeadNode = nd;
	}

	for (size_t i = 0; i < m_logEntries.size(); ++i)
	{
		const GitRev& rev = m_logEntries.GetGitRevAt(i);
		for (size_t j = 0; j < rev.m_ParentHash.size(); ++j)
		{
			auto parentId = m_logEntries.m_HashMap.find(rev.m_ParentHash[j]);
			if (parentId == m_logEntries.m_HashMap.end())
			{
				TRACE(L"Can't found parent node");
				//new parent node as new node
				auto nd = this->m_Graph.newNode();
				m_Graph.newEdge(nodes[i], nd);
				m_logEntries.push_back(rev.m_ParentHash[j]);
				m_logEntries.m_HashMap[rev.m_ParentHash[j]] = m_logEntries.size() - 1;
				nodes.Add(nd);
				SetNodeRect(dev, font, commitString, &nd, rev.m_ParentHash[j]);

			}else
			{
				TRACE(L"edge %d - %d\n",i, m_logEntries.m_HashMap[rev.m_ParentHash[j]]);
				m_Graph.newEdge(nodes[i], nodes[parentId->second]);
			}
		}
	}

	//this->m_OHL.layerDistance(30.0);
	//this->m_OHL.nodeDistance(25.0);
	//this->m_OHL.weightBalancing(0.8);

	m_SugiyamLayout.call(m_GraphAttr);

	ogdf::node v;
	double xmax = 0;
	double ymax = 0;
	forall_nodes(v,m_Graph)
	{
		double x = m_GraphAttr.x(v) + m_GraphAttr.width(v)/2;
		double y = m_GraphAttr.y(v) + m_GraphAttr.height(v)/2;
		if(x>xmax)
			xmax = x;
		if(y>ymax)
			ymax = y;
	}

	this->m_GraphRect.top=m_GraphRect.left=0;
	m_GraphRect.bottom = static_cast<LONG>(ymax);
	m_GraphRect.right = static_cast<LONG>(xmax);

	return true;
}

bool CRevisionGraphWnd::IsUpdateJobRunning() const
{
	return (updateJob.get() != nullptr) && !updateJob->IsDone();
}

bool CRevisionGraphWnd::GetShowOverview() const
{
	return m_bShowOverview;
}

void CRevisionGraphWnd::SetShowOverview (bool value)
{
	m_bShowOverview = value;
	if (m_bShowOverview)
		BuildPreview();
}
CString	CRevisionGraphWnd::GetFriendRefName(ogdf::node v) const
{
	if (!v)
		return CString();
	const CGitHash& hash = this->m_logEntries[v->index()];
	if (const auto refsIt = m_HashMap.find(hash); refsIt == m_HashMap.end())
		return hash.ToString();
	else
		return refsIt->second[0];
}

STRING_VECTOR CRevisionGraphWnd::GetFriendRefNames(ogdf::node v, const CString* exclude, CGit::REF_TYPE* onlyRefType) const
{
	if (!v)
		return STRING_VECTOR();
	const CGitHash& hash = m_logEntries[v->index()];
	if (const auto refsIt = m_HashMap.find(hash); refsIt == m_HashMap.end())
		return STRING_VECTOR();
	else
	{
		STRING_VECTOR list;
		for (const auto& ref: refsIt->second)
		{
			CGit::REF_TYPE refType;
			CString shortName = CGit::GetShortName(ref, &refType);
			if (exclude && *exclude == shortName)
				continue;
			if (!onlyRefType)
				list.push_back(ref);
			else if (*onlyRefType == refType)
				list.push_back(shortName);
		}
		return list;
	}
}

void CRevisionGraphWnd::CompareRevs(const CString& revTo)
{
	ASSERT(m_SelectedEntry1);
	ASSERT(!revTo.IsEmpty() || m_SelectedEntry2);

	bool alternativeTool = !!(GetAsyncKeyState(VK_SHIFT) & 0x8000);

	CString sCmd;

	sCmd.Format(L"/command:showcompare %s /revision1:%s /revision2:%s",
			this->m_sPath.IsEmpty() ? L"" : static_cast<LPCTSTR>(L"/path:\"" + this->m_sPath + L'"'),
			static_cast<LPCTSTR>(GetFriendRefName(m_SelectedEntry1)),
			!revTo.IsEmpty() ? static_cast<LPCTSTR>(revTo) : static_cast<LPCTSTR>(GetFriendRefName(m_SelectedEntry2)));

	if (alternativeTool)
		sCmd += L" /alternative";

	CAppUtils::RunTortoiseGitProc(sCmd);
}

void CRevisionGraphWnd::UnifiedDiffRevs(bool bHead)
{
	ASSERT(m_SelectedEntry1);
	ASSERT(bHead || m_SelectedEntry2);

	bool alternativeTool = !!(GetAsyncKeyState(VK_SHIFT) & 0x8000);
	CAppUtils::StartShowUnifiedDiff(m_hWnd, CString(), GetFriendRefName(m_SelectedEntry1), CString(),
		bHead ? L"HEAD" : GetFriendRefName(m_SelectedEntry2),
		alternativeTool);
}

void CRevisionGraphWnd::DoZoom (float fZoomFactor, bool updateScrollbars)
{
	float oldzoom = m_fZoomFactor;
	m_fZoomFactor = fZoomFactor;

	m_nFontSize = max(1, int(DEFAULT_ZOOM_FONT * fZoomFactor));
	if (m_nFontSize < SMALL_ZOOM_FONT_THRESHOLD)
		m_nFontSize = min(static_cast<int>(SMALL_ZOOM_FONT_THRESHOLD), int(SMALL_ZOOM_FONT * fZoomFactor));

	if (updateScrollbars)
	{
		SCROLLINFO si1 = {sizeof(SCROLLINFO), SIF_ALL};
		GetScrollInfo(SB_VERT, &si1);
		SCROLLINFO si2 = {sizeof(SCROLLINFO), SIF_ALL};
		GetScrollInfo(SB_HORZ, &si2);

		InitView();

		si1.nPos = int(float(si1.nPos)*m_fZoomFactor/oldzoom);
		si2.nPos = int(float(si2.nPos)*m_fZoomFactor/oldzoom);
		SetScrollPos (SB_VERT, si1.nPos);
		SetScrollPos (SB_HORZ, si2.nPos);
	}

	Invalidate (FALSE);
}
