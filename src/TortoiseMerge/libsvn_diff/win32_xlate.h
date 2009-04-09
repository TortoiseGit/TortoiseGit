/*
 * win32_xlate.h : Windows xlate stuff.
 *
 * ====================================================================
 * Copyright (c) 2007 CollabNet.  All rights reserved.
 *
 * This software is licensed as described in the file COPYING, which
 * you should have received as part of this distribution.  The terms
 * are also available at http://subversion.tigris.org/license-1.html.
 * If newer versions of this license are posted there, you may use a
 * newer version instead, at your option.
 *
 * This software consists of voluntary contributions made by many
 * individuals.  For exact contribution history, see the revision
 * history and logs, available at http://subversion.tigris.org/.
 * ====================================================================
 */

#ifndef SVN_LIBSVN_SUBR_WIN32_XLATE_H
#define SVN_LIBSVN_SUBR_WIN32_XLATE_H

#ifdef WIN32

/* Opaque translation buffer. */
typedef struct win32_xlate_t win32_xlate_t;

/* Set *XLATE_P to a handle node for converting from FROMPAGE to TOPAGE.
   Returns APR_EINVAL or APR_ENOTIMPL, if a conversion isn't supported.
   If fail for any other reason, return the error.

   Allocate *RET in POOL. */
apr_status_t svn_subr__win32_xlate_open(win32_xlate_t **xlate_p,
                                        const char *topage,
                                        const char *frompage,
                                        apr_pool_t *pool);

/* Convert SRC_LENGTH bytes of SRC_DATA in NODE->handle, store the result
   in *DEST, which is allocated in POOL. */
apr_status_t svn_subr__win32_xlate_to_stringbuf(win32_xlate_t *handle,
                                                const char *src_data,
                                                apr_size_t src_length,
                                                svn_stringbuf_t **dest,
                                                apr_pool_t *pool);

#endif /* WIN32 */

#endif /* SVN_LIBSVN_SUBR_WIN32_XLATE_H */
