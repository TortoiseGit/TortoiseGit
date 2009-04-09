/*
 * sha1.c: SHA1 checksum routines
 *
 * ====================================================================
 * Copyright (c) 2008 CollabNet.  All rights reserved.
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

#include <apr_sha1.h>

#include "sha1.h"



/* The SHA1 digest for the empty string. */
static const unsigned char svn_sha1__empty_string_digest_array[] = {
  0xda, 0x39, 0xa3, 0xee, 0x5e, 0x6b, 0x4b, 0x0d, 0x32, 0x55,
  0xbf, 0xef, 0x95, 0x60, 0x18, 0x90, 0xaf, 0xd8, 0x07, 0x09
};

const unsigned char *
svn_sha1__empty_string_digest(void)
{
  return svn_sha1__empty_string_digest_array;
}


const char *
svn_sha1__digest_to_cstring_display(const unsigned char digest[],
                                   apr_pool_t *pool)
{
  static const char *hex = "0123456789abcdef";
  char *str = apr_palloc(pool, (APR_SHA1_DIGESTSIZE * 2) + 1);
  int i;

  for (i = 0; i < APR_SHA1_DIGESTSIZE; i++)
    {
      str[i*2]   = hex[digest[i] >> 4];
      str[i*2+1] = hex[digest[i] & 0x0f];
    }
  str[i*2] = '\0';

  return str;
}


const char *
svn_sha1__digest_to_cstring(const unsigned char digest[], apr_pool_t *pool)
{
  static const unsigned char zeros_digest[APR_SHA1_DIGESTSIZE] = { 0 };

  if (memcmp(digest, zeros_digest, APR_SHA1_DIGESTSIZE) != 0)
    return svn_sha1__digest_to_cstring_display(digest, pool);
  else
    return NULL;
}


svn_boolean_t
svn_sha1__digests_match(const unsigned char d1[], const unsigned char d2[])
{
  static const unsigned char zeros[APR_SHA1_DIGESTSIZE] = { 0 };

  return ((memcmp(d1, zeros, APR_SHA1_DIGESTSIZE) == 0)
          || (memcmp(d2, zeros, APR_SHA1_DIGESTSIZE) == 0)
          || (memcmp(d1, d2, APR_SHA1_DIGESTSIZE) == 0));
}
