// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2014 TortoiseGit
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
 * Create an ssh transport with custom git command paths
 *
 * This is a factory function suitable for setting as the transport
 * callback in a remote (or for a clone in the options).
 *
 * The payload argument must be a strarray pointer with the paths for
 * the `git-upload-pack` and `git-receive-pack` at index 0 and 1.
 *
 * @param out the resulting transport
 * @param owner the owning remote
 * @param payload a strarray with the paths
 * @return 0 or an error code
 */
GIT_EXTERN(int) git_transport_ssh_wintunnel_with_paths(git_transport **out, git_remote *owner, void *payload);

/**
 * Create an instance of the ssh subtransport.
 *
 * @param out The newly created subtransport
 * @param owner The smart transport to own this subtransport
 * @return 0 or an error code
 */
GIT_EXTERN(int) git_smart_subtransport_ssh_wintunnel(
	git_smart_subtransport **out,
	git_transport* owner);

/*
 *** End interface for subtransports for the smart transport ***
 */

/** @} */
GIT_END_DECL
#endif
