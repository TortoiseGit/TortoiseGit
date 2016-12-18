// HighColorTab.hpp
//
// Author:  Yves Tkaczyk (yves@tkaczyk.net)
//
// This software is released into the public domain.  You are free to use it
// in any way you like BUT LEAVE THIS HEADER INTACT.
//
// This software is provided "as is" with no expressed or implied warranty.
// I accept no liability for any damage or loss of business that this software
// may cause.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _HIGHCOLORTAB_HPP_INCLUDED_
#define _HIGHCOLORTAB_HPP_INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <memory>

namespace HighColorTab
{
  /*! \brief Policy class for creating image list.

	Policy for creating a high color (32 bits) image list. The policy
	ensure that there is a Win32 image list associated with the CImageList.
	If this is not the case, a nullptr pointer shall be returned.

	Returned image list is wrapped in an std::unique_ptr.

	\sa UpdateImageListFull  */
	struct CHighColorListCreator
	{
	/*! Create the image list.
		\retval std::unique_ptr<CImageList> Not null if success. */
	static std::unique_ptr<CImageList> CreateImageList(int w, int h)
	{
		auto apILNew = std::make_unique<CImageList>();
		if (!apILNew.get())
		{
			// ASSERT: The CImageList object creation failed.
			ASSERT( FALSE );
			return std::unique_ptr<CImageList>();
		}

		if (0 == apILNew->Create(w, h, ILC_COLOR32 | ILC_MASK, 0, 1))
		{
			// ASSERT: The image list (Win32) creation failed.
			ASSERT( FALSE );
			return std::unique_ptr<CImageList>();
		}

		return apILNew;
	}
	};



  /*! \brief Change the image list of the provided control (property sheet interface)

	This method provides full customization via policy over image list creation. The policy
	must have a method with the signature:
	<code>static std::unique_ptr<CImageList> CreateImageList()</code>

	\author Yves Tkaczyk (yves@tkaczyk.net)
	\date 02/2004 */
	template<typename TSheet,
			typename TListCreator>
	bool UpdateImageListFull(TSheet& rSheet, int w, int h)
	{
		// Get the tab control...
		CTabCtrl* pTab = rSheet.GetTabControl();
		if (!IsWindow(pTab->GetSafeHwnd()))
		{
			// ASSERT: Tab control could not be retrieved or it is not a valid window.
			ASSERT( FALSE );
			return false;
		}

		// Create the replacement image list via policy.
		std::unique_ptr<CImageList> apILNew(TListCreator::CreateImageList(w, h));

		bool bSuccess = (nullptr != apILNew.get());

		// Reload the icons from the property pages.
		int nTotalPageCount = rSheet.GetPageCount();
		for(int nCurrentPage = 0; nCurrentPage < nTotalPageCount && bSuccess; ++nCurrentPage )
		{
			// Get the page.
			CPropertyPage* pPage = rSheet.GetPage( nCurrentPage );
			ASSERT( pPage );
			// Set the icon in the image list from the page properties.
			if( pPage && ( pPage->m_psp.dwFlags & PSP_USEHICON ) )
			{
				bSuccess &= ( -1 != apILNew->Add( pPage->m_psp.hIcon ) );
			}

			if( pPage && ( pPage->m_psp.dwFlags & PSP_USEICONID ) )
			{
				bSuccess &= ( -1 != apILNew->Add( AfxGetApp()->LoadIcon( pPage->m_psp.pszIcon ) ) );
			}
		}

		if( !bSuccess )
		{
			// This ASSERT because either the image list could not be created or icon insertion failed.
			ASSERT( FALSE );
			// Cleanup what we have in the new image list.
			if( apILNew.get() )
			{
				apILNew->DeleteImageList();
			}

			return false;
		}

		// Replace the image list from the tab control.
		CImageList* pilOld = pTab->SetImageList( CImageList::FromHandle( apILNew->Detach() ) );
		// Clean the old image list if there was one.
		if( pilOld )
		{
			pilOld->DeleteImageList();
		}

		return true;
	};

	/*! \brief Change the image list of the provided control (property sheet)

	This method uses 32 bits image list creation default policy. */
	template<typename TSheet>
	bool UpdateImageList(TSheet& rSheet, int w, int h)
	{
		return UpdateImageListFull<TSheet, HighColorTab::CHighColorListCreator>(rSheet, w, h);
	};
};

#endif // _HIGHCOLORTAB_HPP_INCLUDED_
