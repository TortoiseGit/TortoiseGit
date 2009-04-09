/*
 * paths.c:   a path manipulation library using svn_stringbuf_t
 *
 * ====================================================================
 * Copyright (c) 2000-2007, 2009 CollabNet.  All rights reserved.
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



#include <string.h>
#include <assert.h>

#include <apr_file_info.h>
#include <apr_lib.h>
#include <apr_uri.h>

#include "svn_string.h"
//#include "svn_dirent_uri.h"
#include "svn_path.h"
//#include "svn_private_config.h"         /* for SVN_PATH_LOCAL_SEPARATOR */
#include "svn_utf.h"
#include "svn_io.h"                     /* for svn_io_stat() */
#include "svn_ctype.h"

//#include "private_uri.h"

/* The canonical empty path.  Can this be changed?  Well, change the empty
   test below and the path library will work, not so sure about the fs/wc
   libraries. */
#define SVN_EMPTY_PATH ""

/* TRUE if s is the canonical empty path, FALSE otherwise */
#define SVN_PATH_IS_EMPTY(s) ((s)[0] == '\0')

/* TRUE if s,n is the platform's empty path ("."), FALSE otherwise. Can
   this be changed?  Well, the path library will work, not so sure about
   the OS! */
#define SVN_PATH_IS_PLATFORM_EMPTY(s,n) ((n) == 1 && (s)[0] == '.')


const char *
svn_path_internal_style(const char *path, apr_pool_t *pool)
{
#if defined(WIN32) || defined(__CYGWIN__)
  if ((path[0] == '/' || path[0] == '\\') && path[1] == path[0])
    return svn_dirent_internal_style(path, pool);
#endif
  return svn_uri_internal_style(path, pool);
}


const char *
svn_path_local_style(const char *path, apr_pool_t *pool)
{
#if defined(WIN32) || defined(__CYGWIN__)
  if (path[0] == '/' && path[1] == '/')
    return svn_dirent_local_style(path, pool);
#endif
  return svn_uri_local_style(path, pool);
}


#ifndef NDEBUG
/* This function is an approximation of svn_path_is_canonical.
 * It is supposed to be used in functions that do not have access
 * to a pool, but still want to assert that a path is canonical.
 *
 * PATH with length LEN is assumed to be canonical if it isn't
 * the platform's empty path (see definition of SVN_PATH_IS_PLATFORM_EMPTY),
 * and does not contain "/./", and any one of the following
 * conditions is also met:
 *
 *  1. PATH has zero length
 *  2. PATH is the root directory (what exactly a root directory is
 *                                depends on the platform)
 *  3. PATH is not a root directory and does not end with '/'
 *
 * If possible, please use svn_path_is_canonical instead.
 */
static svn_boolean_t
is_canonical(const char *path,
             apr_size_t len)
{
  return (! SVN_PATH_IS_PLATFORM_EMPTY(path, len)
          && strstr(path, "/./") == NULL
          && (len == 0
              || svn_dirent_is_root(path, len)
              /* The len > 0 check is redundant, but here to make
               * sure we never ever end up indexing with -1. */
              || (len > 0 && path[len-1] != '/')));
}
#endif


char *svn_path_join(const char *base,
                    const char *component,
                    apr_pool_t *pool)
{
  apr_size_t blen = strlen(base);
  apr_size_t clen = strlen(component);
  char *path;

  assert(svn_path_is_canonical(base, pool));
  assert(svn_path_is_canonical(component, pool));

  /* If the component is absolute, then return it.  */
  if (*component == '/')
    return apr_pmemdup(pool, component, clen + 1);

  /* If either is empty return the other */
  if (SVN_PATH_IS_EMPTY(base))
    return apr_pmemdup(pool, component, clen + 1);
  if (SVN_PATH_IS_EMPTY(component))
    return apr_pmemdup(pool, base, blen + 1);

  if (blen == 1 && base[0] == '/')
    blen = 0; /* Ignore base, just return separator + component */

  /* Construct the new, combined path. */
  path = apr_palloc(pool, blen + 1 + clen + 1);
  memcpy(path, base, blen);
  path[blen] = '/';
  memcpy(path + blen + 1, component, clen + 1);

  return path;
}

char *svn_path_join_many(apr_pool_t *pool, const char *base, ...)
{
#define MAX_SAVED_LENGTHS 10
  apr_size_t saved_lengths[MAX_SAVED_LENGTHS];
  apr_size_t total_len;
  int nargs;
  va_list va;
  const char *s;
  apr_size_t len;
  char *path;
  char *p;
  svn_boolean_t base_is_empty = FALSE, base_is_root = FALSE;
  int base_arg = 0;

  total_len = strlen(base);

  assert(svn_path_is_canonical(base, pool));

  if (total_len == 1 && *base == '/')
    base_is_root = TRUE;
  else if (SVN_PATH_IS_EMPTY(base))
    {
      total_len = sizeof(SVN_EMPTY_PATH) - 1;
      base_is_empty = TRUE;
    }

  saved_lengths[0] = total_len;

  /* Compute the length of the resulting string. */

  nargs = 0;
  va_start(va, base);
  while ((s = va_arg(va, const char *)) != NULL)
    {
      len = strlen(s);

      assert(svn_path_is_canonical(s, pool));

      if (SVN_PATH_IS_EMPTY(s))
        continue;

      if (nargs++ < MAX_SAVED_LENGTHS)
        saved_lengths[nargs] = len;

      if (*s == '/')
        {
          /* an absolute path. skip all components to this point and reset
             the total length. */
          total_len = len;
          base_arg = nargs;
          base_is_root = len == 1;
          base_is_empty = FALSE;
        }
      else if (nargs == base_arg
               || (nargs == base_arg + 1 && base_is_root)
               || base_is_empty)
        {
          /* if we have skipped everything up to this arg, then the base
             and all prior components are empty. just set the length to
             this component; do not add a separator.  If the base is empty
             we can now ignore it. */
          if (base_is_empty)
            {
              base_is_empty = FALSE;
              total_len = 0;
            }
          total_len += len;
        }
      else
        {
          total_len += 1 + len;
        }
    }
  va_end(va);

  /* base == "/" and no further components. just return that. */
  if (base_is_root && total_len == 1)
    return apr_pmemdup(pool, "/", 2);

  /* we got the total size. allocate it, with room for a NULL character. */
  path = p = apr_palloc(pool, total_len + 1);

  /* if we aren't supposed to skip forward to an absolute component, and if
     this is not an empty base that we are skipping, then copy the base
     into the output. */
  if (base_arg == 0 && ! (SVN_PATH_IS_EMPTY(base) && ! base_is_empty))
    {
      if (SVN_PATH_IS_EMPTY(base))
        memcpy(p, SVN_EMPTY_PATH, len = saved_lengths[0]);
      else
        memcpy(p, base, len = saved_lengths[0]);
      p += len;
    }

  nargs = 0;
  va_start(va, base);
  while ((s = va_arg(va, const char *)) != NULL)
    {
      if (SVN_PATH_IS_EMPTY(s))
        continue;

      if (++nargs < base_arg)
        continue;

      if (nargs < MAX_SAVED_LENGTHS)
        len = saved_lengths[nargs];
      else
        len = strlen(s);

      /* insert a separator if we aren't copying in the first component
         (which can happen when base_arg is set). also, don't put in a slash
         if the prior character is a slash (occurs when prior component
         is "/"). */
      if (p != path && p[-1] != '/')
        *p++ = '/';

      /* copy the new component and advance the pointer */
      memcpy(p, s, len);
      p += len;
    }
  va_end(va);

  *p = '\0';
  assert((apr_size_t)(p - path) == total_len);

  return path;
}



apr_size_t
svn_path_component_count(const char *path)
{
  apr_size_t count = 0;

  assert(is_canonical(path, strlen(path)));

  while (*path)
    {
      const char *start;

      while (*path == '/')
        ++path;

      start = path;

      while (*path && *path != '/')
        ++path;

      if (path != start)
        ++count;
    }

  return count;
}


/* Return the length of substring necessary to encompass the entire
 * previous path segment in PATH, which should be a LEN byte string.
 *
 * A trailing slash will not be included in the returned length except
 * in the case in which PATH is absolute and there are no more
 * previous segments.
 */
static apr_size_t
previous_segment(const char *path,
                 apr_size_t len)
{
  if (len == 0)
    return 0;

  while (len > 0 && path[--len] != '/')
    ;

  if (len == 0 && path[0] == '/')
    return 1;
  else
    return len;
}


void
svn_path_add_component(svn_stringbuf_t *path,
                       const char *component)
{
  apr_size_t len = strlen(component);

  assert(is_canonical(path->data, path->len));
  assert(is_canonical(component, strlen(component)));

  /* Append a dir separator, but only if this path is neither empty
     nor consists of a single dir separator already. */
  if ((! SVN_PATH_IS_EMPTY(path->data))
      && (! ((path->len == 1) && (*(path->data) == '/'))))
    {
      char dirsep = '/';
      svn_stringbuf_appendbytes(path, &dirsep, sizeof(dirsep));
    }

  svn_stringbuf_appendbytes(path, component, len);
}


void
svn_path_remove_component(svn_stringbuf_t *path)
{
  assert(is_canonical(path->data, path->len));

  path->len = previous_segment(path->data, path->len);
  path->data[path->len] = '\0';
}


void
svn_path_remove_components(svn_stringbuf_t *path, apr_size_t n)
{
  while (n > 0)
    {
      svn_path_remove_component(path);
      n--;
    }
}


char *
svn_path_dirname(const char *path, apr_pool_t *pool)
{
  apr_size_t len = strlen(path);

  assert(svn_path_is_canonical(path, pool));

  return apr_pstrmemdup(pool, path, previous_segment(path, len));
}


char *
svn_path_basename(const char *path, apr_pool_t *pool)
{
  apr_size_t len = strlen(path);
  apr_size_t start;

  assert(svn_path_is_canonical(path, pool));

  if (len == 1 && path[0] == '/')
    start = 0;
  else
    {
      start = len;
      while (start > 0 && path[start - 1] != '/')
        --start;
    }

  return apr_pstrmemdup(pool, path + start, len - start);
}


void
svn_path_split(const char *path,
               const char **dirpath,
               const char **base_name,
               apr_pool_t *pool)
{
  assert(dirpath != base_name);

  if (dirpath)
    *dirpath = svn_path_dirname(path, pool);

  if (base_name)
    *base_name = svn_path_basename(path, pool);
}


int
svn_path_is_empty(const char *path)
{
  assert(is_canonical(path, strlen(path)));

  if (SVN_PATH_IS_EMPTY(path))
    return 1;

  return 0;
}

int
svn_path_compare_paths(const char *path1,
                       const char *path2)
{
  apr_size_t path1_len = strlen(path1);
  apr_size_t path2_len = strlen(path2);
  apr_size_t min_len = ((path1_len < path2_len) ? path1_len : path2_len);
  apr_size_t i = 0;

  assert(is_canonical(path1, strlen(path1)));
  assert(is_canonical(path2, strlen(path2)));

  /* Skip past common prefix. */
  while (i < min_len && path1[i] == path2[i])
    ++i;

  /* Are the paths exactly the same? */
  if ((path1_len == path2_len) && (i >= min_len))
    return 0;

  /* Children of paths are greater than their parents, but less than
     greater siblings of their parents. */
  if ((path1[i] == '/') && (path2[i] == 0))
    return 1;
  if ((path2[i] == '/') && (path1[i] == 0))
    return -1;
  if (path1[i] == '/')
    return -1;
  if (path2[i] == '/')
    return 1;

  /* Common prefix was skipped above, next character is compared to
     determine order.  We need to use an unsigned comparison, though,
     so a "next character" of NULL (0x00) sorts numerically
     smallest. */
  return (unsigned char)(path1[i]) < (unsigned char)(path2[i]) ? -1 : 1;
}

char *
svn_path_get_longest_ancestor(const char *path1,
                              const char *path2,
                              apr_pool_t *pool)
{
  return svn_uri_get_longest_ancestor(path1, path2, pool);
}

const char *
svn_path_is_child(const char *path1,
                  const char *path2,
                  apr_pool_t *pool)
{
  return svn_uri_is_child(path1, path2, pool);
}


svn_boolean_t
svn_path_is_ancestor(const char *path1, const char *path2)
{
  return svn_uri_is_ancestor(path1, path2);
}


apr_array_header_t *
svn_path_decompose(const char *path,
                   apr_pool_t *pool)
{
  apr_size_t i, oldi;

  apr_array_header_t *components =
    apr_array_make(pool, 1, sizeof(const char *));

  assert(svn_path_is_canonical(path, pool));

  if (SVN_PATH_IS_EMPTY(path))
    return components;  /* ### Should we return a "" component? */

  /* If PATH is absolute, store the '/' as the first component. */
  i = oldi = 0;
  if (path[i] == '/')
    {
      char dirsep = '/';

      APR_ARRAY_PUSH(components, const char *)
        = apr_pstrmemdup(pool, &dirsep, sizeof(dirsep));

      i++;
      oldi++;
      if (path[i] == '\0') /* path is a single '/' */
        return components;
    }

  do
    {
      if ((path[i] == '/') || (path[i] == '\0'))
        {
          if (SVN_PATH_IS_PLATFORM_EMPTY(path + oldi, i - oldi))
            APR_ARRAY_PUSH(components, const char *) = SVN_EMPTY_PATH;
          else
            APR_ARRAY_PUSH(components, const char *)
              = apr_pstrmemdup(pool, path + oldi, i - oldi);

          i++;
          oldi = i;  /* skipping past the dirsep */
          continue;
        }
      i++;
    }
  while (path[i-1]);

  return components;
}


const char *
svn_path_compose(const apr_array_header_t *components,
                 apr_pool_t *pool)
{
  apr_size_t *lengths = apr_palloc(pool, components->nelts*sizeof(*lengths));
  apr_size_t max_length = components->nelts;
  char *path;
  char *p;
  int i;

  /* Get the length of each component so a total length can be
     calculated. */
  for (i = 0; i < components->nelts; ++i)
    {
      apr_size_t l = strlen(APR_ARRAY_IDX(components, i, const char *));
      lengths[i] = l;
      max_length += l;
    }

  path = apr_palloc(pool, max_length + 1);
  p = path;

  for (i = 0; i < components->nelts; ++i)
    {
      /* Append a '/' to the path.  Handle the case with an absolute
         path where a '/' appears in the first component.  Only append
         a '/' if the component is the second component that does not
         follow a "/" first component; or it is the third or later
         component. */
      if (i > 1 ||
          (i == 1 && strcmp("/", APR_ARRAY_IDX(components,
                                               0,
                                               const char *)) != 0))
        {
          *p++ = '/';
        }

      memcpy(p, APR_ARRAY_IDX(components, i, const char *), lengths[i]);
      p += lengths[i];
    }

  *p = '\0';

  return path;
}


svn_boolean_t
svn_path_is_single_path_component(const char *name)
{
  assert(is_canonical(name, strlen(name)));

  /* Can't be empty or `..'  */
  if (SVN_PATH_IS_EMPTY(name)
      || (name[0] == '.' && name[1] == '.' && name[2] == '\0'))
    return FALSE;

  /* Slashes are bad, m'kay... */
  if (strchr(name, '/') != NULL)
    return FALSE;

  /* It is valid.  */
  return TRUE;
}


svn_boolean_t
svn_path_is_dotpath_present(const char *path)
{
  int len;

  /* The empty string does not have a dotpath */
  if (path[0] == '\0')
    return FALSE;

  /* Handle "." or a leading "./" */
  if (path[0] == '.' && (path[1] == '\0' || path[1] == '/'))
    return TRUE;

  /* Paths of length 1 (at this point) have no dotpath present. */
  if (path[1] == '\0')
    return FALSE;

  /* If any segment is "/./", then a dotpath is present. */
  if (strstr(path, "/./") != NULL)
    return TRUE;

  /* Does the path end in "/." ? */
  len = strlen(path);
  return path[len - 2] == '/' && path[len - 1] == '.';
}

svn_boolean_t
svn_path_is_backpath_present(const char *path)
{
  int len;

  /* 0 and 1-length paths do not have a backpath */
  if (path[0] == '\0' || path[1] == '\0')
    return FALSE;

  /* Handle ".." or a leading "../" */
  if (path[0] == '.' && path[1] == '.' && (path[2] == '\0' || path[2] == '/'))
    return TRUE;

  /* Paths of length 2 (at this point) have no backpath present. */
  if (path[2] == '\0')
    return FALSE;

  /* If any segment is "..", then a backpath is present. */
  if (strstr(path, "/../") != NULL)
    return TRUE;

  /* Does the path end in "/.." ? */
  len = strlen(path);
  return path[len - 3] == '/' && path[len - 2] == '.' && path[len - 1] == '.';
}


/*** URI Stuff ***/

/* Examine PATH as a potential URI, and return a substring of PATH
   that immediately follows the (scheme):// portion of the URI, or
   NULL if PATH doesn't appear to be a valid URI.  The returned value
   is not alloced -- it shares memory with PATH. */
static const char *
skip_uri_scheme(const char *path)
{
  apr_size_t j;

  /* A scheme is terminated by a : and cannot contain any /'s. */
  for (j = 0; path[j] && path[j] != ':'; ++j)
    if (path[j] == '/')
      return NULL;

  if (j > 0 && path[j] == ':' && path[j+1] == '/' && path[j+2] == '/')
    return path + j + 3;

  return NULL;
}


svn_boolean_t
svn_path_is_url(const char *path)
{
  /* ### This function is reaaaaaaaaaaaaaally stupid right now.
     We're just going to look for:

        (scheme)://(optional_stuff)

     Where (scheme) has no ':' or '/' characters.

     Someday it might be nice to have an actual URI parser here.
  */
  return skip_uri_scheme(path) != NULL;
}



/* Here is the BNF for path components in a URI. "pchar" is a
   character in a path component.

      pchar       = unreserved | escaped |
                    ":" | "@" | "&" | "=" | "+" | "$" | ","
      unreserved  = alphanum | mark
      mark        = "-" | "_" | "." | "!" | "~" | "*" | "'" | "(" | ")"

   Note that "escaped" doesn't really apply to what users can put in
   their paths, so that really means the set of characters is:

      alphanum | mark | ":" | "@" | "&" | "=" | "+" | "$" | ","
*/
static const char uri_char_validity[256] = {
  0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
  0, 1, 0, 0, 1, 0, 1, 1,   1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1,   1, 1, 1, 0, 0, 1, 0, 0,

  /* 64 */
  1, 1, 1, 1, 1, 1, 1, 1,   1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1,   1, 1, 1, 0, 0, 0, 0, 1,
  0, 1, 1, 1, 1, 1, 1, 1,   1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1,   1, 1, 1, 0, 0, 0, 1, 0,

  /* 128 */
  0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,

  /* 192 */
  0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
};


svn_boolean_t
svn_path_is_uri_safe(const char *path)
{
  apr_size_t i;

  /* Skip the URI scheme. */
  path = skip_uri_scheme(path);

  /* No scheme?  Get outta here. */
  if (! path)
    return FALSE;

  /* Skip to the first slash that's after the URI scheme. */
  path = strchr(path, '/');

  /* If there's no first slash, then there's only a host portion;
     therefore there couldn't be any uri-unsafe characters after the
     host... so return true. */
  if (path == NULL)
    return TRUE;

  for (i = 0; path[i]; i++)
    {
      /* Allow '%XX' (where each X is a hex digit) */
      if (path[i] == '%')
        {
          if (apr_isxdigit(path[i + 1]) && apr_isxdigit(path[i + 2]))
            {
              i += 2;
              continue;
            }
          return FALSE;
        }
      else if (! uri_char_validity[((unsigned char)path[i])])
        {
          return FALSE;
        }
    }

  return TRUE;
}


/* URI-encode each character c in PATH for which TABLE[c] is 0.
   If no encoding was needed, return PATH, else return a new string allocated
   in POOL. */
static const char *
uri_escape(const char *path, const char table[], apr_pool_t *pool)
{
  svn_stringbuf_t *retstr;
  apr_size_t i, copied = 0;
  int c;

  retstr = svn_stringbuf_create_ensure(strlen(path), pool);
  for (i = 0; path[i]; i++)
    {
      c = (unsigned char)path[i];
      if (table[c])
        continue;

      /* If we got here, we're looking at a character that isn't
         supported by the (or at least, our) URI encoding scheme.  We
         need to escape this character.  */

      /* First things first, copy all the good stuff that we haven't
         yet copied into our output buffer. */
      if (i - copied)
        svn_stringbuf_appendbytes(retstr, path + copied,
                                  i - copied);

      /* Now, sprintf() in our escaped character, making sure our
         buffer is big enough to hold the '%' and two digits.  We cast
         the C to unsigned char here because the 'X' format character
         will be tempted to treat it as an unsigned int...which causes
         problem when messing with 0x80-0xFF chars.  We also need space
         for a null as sprintf will write one. */
      svn_stringbuf_ensure(retstr, retstr->len + 4);
      sprintf(retstr->data + retstr->len, "%%%02X", (unsigned char)c);
      retstr->len += 3;

      /* Finally, update our copy counter. */
      copied = i + 1;
    }

  /* If we didn't encode anything, we don't need to duplicate the string. */
  if (retstr->len == 0)
    return path;

  /* Anything left to copy? */
  if (i - copied)
    svn_stringbuf_appendbytes(retstr, path + copied, i - copied);

  /* retstr is null-terminated either by sprintf or the svn_stringbuf
     functions. */

  return retstr->data;
}


const char *
svn_path_uri_encode(const char *path, apr_pool_t *pool)
{
  const char *ret;

  ret = uri_escape(path, uri_char_validity, pool);

  /* Our interface guarantees a copy. */
  if (ret == path)
    return apr_pstrdup(pool, path);
  else
    return ret;
}

static const char iri_escape_chars[256] = {
  1, 1, 1, 1, 1, 1, 1, 1,  1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1,  1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1,  1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1,  1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1,  1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1,  1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1,  1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1,  1, 1, 1, 1, 1, 1, 1, 1,

  /* 128 */
  0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0
};

const char *
svn_path_uri_from_iri(const char *iri, apr_pool_t *pool)
{
  return uri_escape(iri, iri_escape_chars, pool);
}

static const char uri_autoescape_chars[256] = {
  1, 1, 1, 1, 1, 1, 1, 1,  1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1,  1, 1, 1, 1, 1, 1, 1, 1,
  0, 1, 0, 1, 1, 1, 1, 1,  1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1,  1, 1, 1, 1, 0, 1, 0, 1,

  /* 64 */
  1, 1, 1, 1, 1, 1, 1, 1,  1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1,  1, 1, 1, 1, 0, 1, 0, 1,
  0, 1, 1, 1, 1, 1, 1, 1,  1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1,  1, 1, 1, 0, 0, 0, 1, 1,

  /* 128 */
  1, 1, 1, 1, 1, 1, 1, 1,  1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1,  1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1,  1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1,  1, 1, 1, 1, 1, 1, 1, 1,

  /* 192 */
  1, 1, 1, 1, 1, 1, 1, 1,  1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1,  1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1,  1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1,  1, 1, 1, 1, 1, 1, 1, 1,
};

const char *
svn_path_uri_autoescape(const char *uri, apr_pool_t *pool)
{
  return uri_escape(uri, uri_autoescape_chars, pool);
}

const char *
svn_path_uri_decode(const char *path, apr_pool_t *pool)
{
  svn_stringbuf_t *retstr;
  apr_size_t i;
  svn_boolean_t query_start = FALSE;

  /* avoid repeated realloc */
  retstr = svn_stringbuf_create_ensure(strlen(path) + 1, pool);

  retstr->len = 0;
  for (i = 0; path[i]; i++)
    {
      char c = path[i];

      if (c == '?')
        {
          /* Mark the start of the query string, if it exists. */
          query_start = TRUE;
        }
      else if (c == '+' && query_start)
        {
          /* Only do this if we are into the query string.
           * RFC 2396, section 3.3  */
          c = ' ';
        }
      else if (c == '%' && apr_isxdigit(path[i + 1])
               && apr_isxdigit(path[i+2]))
        {
          char digitz[3];
          digitz[0] = path[++i];
          digitz[1] = path[++i];
          digitz[2] = '\0';
          c = (char)(strtol(digitz, NULL, 16));
        }

      retstr->data[retstr->len++] = c;
    }

  /* Null-terminate this bad-boy. */
  retstr->data[retstr->len] = 0;

  return retstr->data;
}


const char *
svn_path_url_add_component2(const char *url,
                            const char *component,
                            apr_pool_t *pool)
{
  assert(svn_path_is_canonical(url, pool));

  return svn_path_join(url, svn_path_uri_encode(component, pool), pool);
}

svn_error_t *
svn_path_get_absolute(const char **pabsolute,
                      const char *relative,
                      apr_pool_t *pool)
{
  if (svn_path_is_url(relative))
    {
      *pabsolute = apr_pstrdup(pool, relative);
      return SVN_NO_ERROR;
    }

  return svn_dirent_get_absolute(pabsolute, relative, pool);
}


svn_error_t *
svn_path_split_if_file(const char *path,
                       const char **pdirectory,
                       const char **pfile,
                       apr_pool_t *pool)
{
  apr_finfo_t finfo;
  svn_error_t *err;

  SVN_ERR_ASSERT(svn_path_is_canonical(path, pool));

  err = svn_io_stat(&finfo, path, APR_FINFO_TYPE, pool);
  if (err && ! APR_STATUS_IS_ENOENT(err->apr_err))
    return err;

  if (err || finfo.filetype == APR_REG)
    {
      svn_error_clear(err);
      svn_path_split(path, pdirectory, pfile, pool);
    }
  else if (finfo.filetype == APR_DIR)
    {
      *pdirectory = path;
      *pfile = SVN_EMPTY_PATH;
    }
  else
    {
      return svn_error_createf(SVN_ERR_BAD_FILENAME, NULL,
                               _("'%s' is neither a file nor a directory name"),
                               svn_path_local_style(path, pool));
    }

  return SVN_NO_ERROR;
}


const char *
svn_path_canonicalize(const char *path, apr_pool_t *pool)
{
#if defined(WIN32) || defined(__CYGWIN__)
  if (path[0] == '/' && path[1] == '/')
    return svn_dirent_canonicalize(path, pool);
#endif
  return svn_uri_canonicalize(path, pool);
}

svn_boolean_t
svn_path_is_canonical(const char *path, apr_pool_t *pool)
{
  return svn_uri_is_canonical(path, pool)
#if defined(WIN32) || defined(__CYGWIN__)
         || (path[0] == '/' && path[1] == '/' &&
             svn_dirent_is_canonical(path, pool))
#endif
         ;
}


/** Get APR's internal path encoding. */
static svn_error_t *
get_path_encoding(svn_boolean_t *path_is_utf8, apr_pool_t *pool)
{
  apr_status_t apr_err;
  int encoding_style;

  apr_err = apr_filepath_encoding(&encoding_style, pool);
  if (apr_err)
    return svn_error_wrap_apr(apr_err,
                              _("Can't determine the native path encoding"));

  /* ### What to do about APR_FILEPATH_ENCODING_UNKNOWN?
     Well, for now we'll just punt to the svn_utf_ functions;
     those will at least do the ASCII-subset check. */
  *path_is_utf8 = (encoding_style == APR_FILEPATH_ENCODING_UTF8);
  return SVN_NO_ERROR;
}


svn_error_t *
svn_path_cstring_from_utf8(const char **path_apr,
                           const char *path_utf8,
                           apr_pool_t *pool)
{
  svn_boolean_t path_is_utf8;
  SVN_ERR(get_path_encoding(&path_is_utf8, pool));
  if (path_is_utf8)
    {
      *path_apr = apr_pstrdup(pool, path_utf8);
      return SVN_NO_ERROR;
    }
  else
    return svn_utf_cstring_from_utf8(path_apr, path_utf8, pool);
}


svn_error_t *
svn_path_cstring_to_utf8(const char **path_utf8,
                         const char *path_apr,
                         apr_pool_t *pool)
{
  svn_boolean_t path_is_utf8;
  SVN_ERR(get_path_encoding(&path_is_utf8, pool));
  if (path_is_utf8)
    {
      *path_utf8 = apr_pstrdup(pool, path_apr);
      return SVN_NO_ERROR;
    }
  else
    return svn_utf_cstring_to_utf8(path_utf8, path_apr, pool);
}


/* Return a copy of PATH, allocated from POOL, for which control
   characters have been escaped using the form \NNN (where NNN is the
   octal representation of the byte's ordinal value).  */
static const char *
illegal_path_escape(const char *path, apr_pool_t *pool)
{
  svn_stringbuf_t *retstr;
  apr_size_t i, copied = 0;
  int c;

  /* At least one control character:
      strlen - 1 (control) + \ + N + N + N + null . */
  retstr = svn_stringbuf_create_ensure(strlen(path) + 4, pool);
  for (i = 0; path[i]; i++)
    {
      c = (unsigned char)path[i];
      if (! svn_ctype_iscntrl(c))
        continue;

      /* If we got here, we're looking at a character that isn't
         supported by the (or at least, our) URI encoding scheme.  We
         need to escape this character.  */

      /* First things first, copy all the good stuff that we haven't
         yet copied into our output buffer. */
      if (i - copied)
        svn_stringbuf_appendbytes(retstr, path + copied,
                                  i - copied);

      /* Make sure buffer is big enough for '\' 'N' 'N' 'N' null */
      svn_stringbuf_ensure(retstr, retstr->len + 5);
      /*### The backslash separator doesn't work too great with Windows,
         but it's what we'll use for consistency with invalid utf8
         formatting (until someone has a better idea) */
      sprintf(retstr->data + retstr->len, "\\%03o", (unsigned char)c);
      retstr->len += 4;

      /* Finally, update our copy counter. */
      copied = i + 1;
    }

  /* If we didn't encode anything, we don't need to duplicate the string. */
  if (retstr->len == 0)
    return path;

  /* Anything left to copy? */
  if (i - copied)
    svn_stringbuf_appendbytes(retstr, path + copied, i - copied);

  /* retstr is null-terminated either by sprintf or the svn_stringbuf
     functions. */

  return retstr->data;
}

svn_error_t *
svn_path_check_valid(const char *path, apr_pool_t *pool)
{
  const char *c;

  for (c = path; *c; c++)
    {
      if (svn_ctype_iscntrl(*c))
        {
          return svn_error_createf
            (SVN_ERR_FS_PATH_SYNTAX, NULL,
             _("Invalid control character '0x%02x' in path '%s'"),
             (unsigned char)*c,
             illegal_path_escape(svn_path_local_style(path, pool), pool));
        }
    }

  return SVN_NO_ERROR;
}

void
svn_path_splitext(const char **path_root,
                  const char **path_ext,
                  const char *path,
                  apr_pool_t *pool)
{
  const char *last_dot, *last_slash;

  /* Easy out -- why do all the work when there's no way to report it? */
  if (! (path_root || path_ext))
    return;

  /* Do we even have a period in this thing?  And if so, is there
     anything after it?  We look for the "rightmost" period in the
     string. */
  last_dot = strrchr(path, '.');
  if (last_dot && (last_dot + 1 != '\0'))
    {
      /* If we have a period, we need to make sure it occurs in the
         final path component -- that there's no path separator
         between the last period and the end of the PATH -- otherwise,
         it doesn't count.  Also, we want to make sure that our period
         isn't the first character of the last component. */
      last_slash = strrchr(path, '/');
      if ((last_slash && (last_dot > (last_slash + 1)))
          || ((! last_slash) && (last_dot > path)))
        {
          if (path_root)
            *path_root = apr_pstrmemdup(pool, path,
                                        (last_dot - path + 1) * sizeof(*path));
          if (path_ext)
            *path_ext = apr_pstrdup(pool, last_dot + 1);
          return;
        }
    }
  /* If we get here, we never found a suitable separator character, so
     there's no split. */
  if (path_root)
    *path_root = apr_pstrdup(pool, path);
  if (path_ext)
    *path_ext = "";
}
