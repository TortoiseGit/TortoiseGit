// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2014, 2016 TortoiseGit
// Copyright (C) the libgit2 contributors. All rights reserved.
//               - based on libgit2/include/git2/transport.h

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
#ifndef INCLUDE_git_transport_ssh_wintunnel_h__
#define INCLUDE_git_transport_ssh_wintunnel_h__

//#include "transport.h"

/**
 * @file git2/transport.h
 * @brief Git transport interfaces and functions
 * @defgroup git_transport interfaces and functions
 * @ingroup Git
 * @{
 */
GIT_BEGIN_DECL

/**
 * Create an instance of the ssh subtransport.
 *
 * Must be called by a wrapper, because this method does not match git_smart_subtransport_cb
 *
 * @param out The newly created subtransport
 * @param owner The smart transport to own this subtransport
 * @param sshtoolpath the path to the ssh helper tool (plink or ssh)
 * @param pEnv environment variables to be passed to the ssh helper tool
 * @return 0 or an error code
 */
GIT_EXTERN(int) git_smart_subtransport_ssh_wintunnel(
	git_smart_subtransport **out,
	git_transport* owner,
	LPCWSTR sshtoolpath,
	LPWSTR* pEnv);

/*
 *** End interface for subtransports for the smart transport ***
 */

/** @} */
GIT_END_DECL
#endif
