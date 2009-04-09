/*
 * svn_string.h:  routines to manipulate counted-length strings
 *                (svn_stringbuf_t and svn_string_t) and C strings.
 *
 *
 * ====================================================================
 * Copyright (c) 2000-2008 CollabNet.  All rights reserved.
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



#include <string.h>      /* for memcpy(), memcmp(), strlen() */
#include <apr_lib.h>     /* for apr_isspace() */
#include <apr_fnmatch.h>
#include "svn_string.h"  /* loads "svn_types.h" and <apr_pools.h> */
#include "svn_ctype.h"



/* Our own realloc, since APR doesn't have one.  Note: this is a
   generic realloc for memory pools, *not* for strings. */
static void *
my__realloc(char *data, apr_size_t oldsize, apr_size_t request,
            apr_pool_t *pool)
{
  void *new_area;

  /* kff todo: it's a pity APR doesn't give us this -- sometimes it
     could realloc the block merely by extending in place, sparing us
     a memcpy(), but only the pool would know enough to be able to do
     this.  We should add a realloc() to APR if someone hasn't
     already. */

  /* malloc new area */
  new_area = apr_palloc(pool, request);

  /* copy data to new area */
  memcpy(new_area, data, oldsize);

  /* I'm NOT freeing old area here -- cuz we're using pools, ugh. */

  /* return new area */
  return new_area;
}

static APR_INLINE svn_boolean_t
string_compare(const char *str1,
               const char *str2,
               apr_size_t len1,
               apr_size_t len2)
{
  /* easy way out :)  */
  if (len1 != len2)
    return FALSE;

  /* now the strings must have identical lenghths */

  if ((memcmp(str1, str2, len1)) == 0)
    return TRUE;
  else
    return FALSE;
}

static APR_INLINE apr_size_t
string_first_non_whitespace(const char *str, apr_size_t len)
{
  apr_size_t i;

  for (i = 0; i < len; i++)
    {
      if (! apr_isspace(str[i]))
        return i;
    }

  /* if we get here, then the string must be entirely whitespace */
  return len;
}

static APR_INLINE apr_size_t
find_char_backward(const char *str, apr_size_t len, char ch)
{
  apr_size_t i = len;

  while (i != 0)
    {
      if (str[--i] == ch)
        return i;
    }

  /* char was not found, return len */
  return len;
}


/* svn_string functions */

static svn_string_t *
create_string(const char *data, apr_size_t size,
              apr_pool_t *pool)
{
  svn_string_t *new_string;

  new_string = apr_palloc(pool, sizeof(*new_string));

  new_string->data = data;
  new_string->len = size;

  return new_string;
}

svn_string_t *
svn_string_ncreate(const char *bytes, apr_size_t size, apr_pool_t *pool)
{
  char *data;

  data = apr_palloc(pool, size + 1);
  memcpy(data, bytes, size);

  /* Null termination is the convention -- even if we suspect the data
     to be binary, it's not up to us to decide, it's the caller's
     call.  Heck, that's why they call it the caller! */
  data[size] = '\0';

  /* wrap an svn_string_t around the new data */
  return create_string(data, size, pool);
}


svn_string_t *
svn_string_create(const char *cstring, apr_pool_t *pool)
{
  return svn_string_ncreate(cstring, strlen(cstring), pool);
}


svn_string_t *
svn_string_create_from_buf(const svn_stringbuf_t *strbuf, apr_pool_t *pool)
{
  return svn_string_ncreate(strbuf->data, strbuf->len, pool);
}


svn_string_t *
svn_string_createv(apr_pool_t *pool, const char *fmt, va_list ap)
{
  char *data = apr_pvsprintf(pool, fmt, ap);

  /* wrap an svn_string_t around the new data */
  return create_string(data, strlen(data), pool);
}


svn_string_t *
svn_string_createf(apr_pool_t *pool, const char *fmt, ...)
{
  svn_string_t *str;

  va_list ap;
  va_start(ap, fmt);
  str = svn_string_createv(pool, fmt, ap);
  va_end(ap);

  return str;
}


svn_boolean_t
svn_string_isempty(const svn_string_t *str)
{
  return (str->len == 0);
}


svn_string_t *
svn_string_dup(const svn_string_t *original_string, apr_pool_t *pool)
{
  return (svn_string_ncreate(original_string->data,
                             original_string->len, pool));
}



svn_boolean_t
svn_string_compare(const svn_string_t *str1, const svn_string_t *str2)
{
  return
    string_compare(str1->data, str2->data, str1->len, str2->len);
}



apr_size_t
svn_string_first_non_whitespace(const svn_string_t *str)
{
  return
    string_first_non_whitespace(str->data, str->len);
}


apr_size_t
svn_string_find_char_backward(const svn_string_t *str, char ch)
{
  return find_char_backward(str->data, str->len, ch);
}



/* svn_stringbuf functions */

static svn_stringbuf_t *
create_stringbuf(char *data, apr_size_t size, apr_size_t blocksize, apr_pool_t *pool)
{
  svn_stringbuf_t *new_string;

  new_string = apr_palloc(pool, sizeof(*new_string));

  new_string->data = data;
  new_string->len = size;
  new_string->blocksize = blocksize;
  new_string->pool = pool;

  return new_string;
}

svn_stringbuf_t *
svn_stringbuf_create_ensure(apr_size_t blocksize, apr_pool_t *pool)
{
  char *data = apr_palloc(pool, ++blocksize); /* + space for '\0' */

  data[0] = '\0';

  /* wrap an svn_stringbuf_t around the new data buffer. */
  return create_stringbuf(data, 0, blocksize, pool);
}

svn_stringbuf_t *
svn_stringbuf_ncreate(const char *bytes, apr_size_t size, apr_pool_t *pool)
{
  /* Ensure string buffer of size + 1 */
  svn_stringbuf_t *strbuf = svn_stringbuf_create_ensure(size, pool);

  memcpy(strbuf->data, bytes, size);

  /* Null termination is the convention -- even if we suspect the data
     to be binary, it's not up to us to decide, it's the caller's
     call.  Heck, that's why they call it the caller! */
  strbuf->data[size] = '\0';
  strbuf->len = size;

  return strbuf;
}


svn_stringbuf_t *
svn_stringbuf_create(const char *cstring, apr_pool_t *pool)
{
  return svn_stringbuf_ncreate(cstring, strlen(cstring), pool);
}


svn_stringbuf_t *
svn_stringbuf_create_from_string(const svn_string_t *str, apr_pool_t *pool)
{
  return svn_stringbuf_ncreate(str->data, str->len, pool);
}


svn_stringbuf_t *
svn_stringbuf_createv(apr_pool_t *pool, const char *fmt, va_list ap)
{
  char *data = apr_pvsprintf(pool, fmt, ap);
  apr_size_t size = strlen(data);

  /* wrap an svn_stringbuf_t around the new data */
  return create_stringbuf(data, size, size + 1, pool);
}


svn_stringbuf_t *
svn_stringbuf_createf(apr_pool_t *pool, const char *fmt, ...)
{
  svn_stringbuf_t *str;

  va_list ap;
  va_start(ap, fmt);
  str = svn_stringbuf_createv(pool, fmt, ap);
  va_end(ap);

  return str;
}


void
svn_stringbuf_fillchar(svn_stringbuf_t *str, unsigned char c)
{
  memset(str->data, c, str->len);
}


void
svn_stringbuf_set(svn_stringbuf_t *str, const char *value)
{
  apr_size_t amt = strlen(value);

  svn_stringbuf_ensure(str, amt + 1);
  memcpy(str->data, value, amt + 1);
  str->len = amt;
}

void
svn_stringbuf_setempty(svn_stringbuf_t *str)
{
  if (str->len > 0)
    str->data[0] = '\0';

  str->len = 0;
}


void
svn_stringbuf_chop(svn_stringbuf_t *str, apr_size_t nbytes)
{
  if (nbytes > str->len)
    str->len = 0;
  else
    str->len -= nbytes;

  str->data[str->len] = '\0';
}


svn_boolean_t
svn_stringbuf_isempty(const svn_stringbuf_t *str)
{
  return (str->len == 0);
}


void
svn_stringbuf_ensure(svn_stringbuf_t *str, apr_size_t minimum_size)
{
  /* Keep doubling capacity until have enough. */
  if (str->blocksize < minimum_size)
    {
      if (str->blocksize == 0)
        str->blocksize = minimum_size;
      else
        while (str->blocksize < minimum_size)
          {
            apr_size_t prev_size = str->blocksize;
            str->blocksize *= 2;
            /* check for apr_size_t overflow */
            if (prev_size > str->blocksize)
              {
                str->blocksize = minimum_size;
                break;
              }
          }

      str->data = (char *) my__realloc(str->data,
                                       str->len + 1,
                                       /* We need to maintain (and thus copy)
                                          the trailing nul */
                                       str->blocksize,
                                       str->pool);
    }
}


void
svn_stringbuf_appendbytes(svn_stringbuf_t *str, const char *bytes,
                          apr_size_t count)
{
  apr_size_t total_len;
  void *start_address;

  total_len = str->len + count;  /* total size needed */

  /* +1 for null terminator. */
  svn_stringbuf_ensure(str, (total_len + 1));

  /* get address 1 byte beyond end of original bytestring */
  start_address = (str->data + str->len);

  memcpy(start_address, bytes, count);
  str->len = total_len;

  str->data[str->len] = '\0';  /* We don't know if this is binary
                                  data or not, but convention is
                                  to null-terminate. */
}


void
svn_stringbuf_appendstr(svn_stringbuf_t *targetstr,
                        const svn_stringbuf_t *appendstr)
{
  svn_stringbuf_appendbytes(targetstr, appendstr->data, appendstr->len);
}


void
svn_stringbuf_appendcstr(svn_stringbuf_t *targetstr, const char *cstr)
{
  svn_stringbuf_appendbytes(targetstr, cstr, strlen(cstr));
}


svn_stringbuf_t *
svn_stringbuf_dup(const svn_stringbuf_t *original_string, apr_pool_t *pool)
{
  return (svn_stringbuf_ncreate(original_string->data,
                                original_string->len, pool));
}



svn_boolean_t
svn_stringbuf_compare(const svn_stringbuf_t *str1,
                      const svn_stringbuf_t *str2)
{
  return string_compare(str1->data, str2->data, str1->len, str2->len);
}



apr_size_t
svn_stringbuf_first_non_whitespace(const svn_stringbuf_t *str)
{
  return string_first_non_whitespace(str->data, str->len);
}


void
svn_stringbuf_strip_whitespace(svn_stringbuf_t *str)
{
  /* Find first non-whitespace character */
  apr_size_t offset = svn_stringbuf_first_non_whitespace(str);

  /* Go ahead!  Waste some RAM, we've got pools! :)  */
  str->data += offset;
  str->len -= offset;
  str->blocksize -= offset;

  /* Now that we've trimmed the front, trim the end, wasting more RAM. */
  while ((str->len > 0) && apr_isspace(str->data[str->len - 1]))
    str->len--;
  str->data[str->len] = '\0';
}


apr_size_t
svn_stringbuf_find_char_backward(const svn_stringbuf_t *str, char ch)
{
  return find_char_backward(str->data, str->len, ch);
}


svn_boolean_t
svn_string_compare_stringbuf(const svn_string_t *str1,
                             const svn_stringbuf_t *str2)
{
  return string_compare(str1->data, str2->data, str1->len, str2->len);
}



/*** C string stuff. ***/

void
svn_cstring_split_append(apr_array_header_t *array,
                         const char *input,
                         const char *sep_chars,
                         svn_boolean_t chop_whitespace,
                         apr_pool_t *pool)
{
  char *last;
  char *pats;
  char *p;

  pats = apr_pstrdup(pool, input);  /* strtok wants non-const data */
  p = apr_strtok(pats, sep_chars, &last);

  while (p)
    {
      if (chop_whitespace)
        {
          while (apr_isspace(*p))
            p++;

          {
            char *e = p + (strlen(p) - 1);
            while ((e >= p) && (apr_isspace(*e)))
              e--;
            *(++e) = '\0';
          }
        }

      if (p[0] != '\0')
        APR_ARRAY_PUSH(array, const char *) = p;

      p = apr_strtok(NULL, sep_chars, &last);
    }

  return;
}


apr_array_header_t *
svn_cstring_split(const char *input,
                  const char *sep_chars,
                  svn_boolean_t chop_whitespace,
                  apr_pool_t *pool)
{
  apr_array_header_t *a = apr_array_make(pool, 5, sizeof(input));
  svn_cstring_split_append(a, input, sep_chars, chop_whitespace, pool);
  return a;
}


svn_boolean_t svn_cstring_match_glob_list(const char *str,
                                          apr_array_header_t *list)
{
  int i;

  for (i = 0; i < list->nelts; i++)
    {
      const char *this_pattern = APR_ARRAY_IDX(list, i, char *);

      if (apr_fnmatch(this_pattern, str, 0) == APR_SUCCESS)
        return TRUE;
    }

  return FALSE;
}

int svn_cstring_count_newlines(const char *msg)
{
  int count = 0;
  const char *p;

  for (p = msg; *p; p++)
    {
      if (*p == '\n')
        {
          count++;
          if (*(p + 1) == '\r')
            p++;
        }
      else if (*p == '\r')
        {
          count++;
          if (*(p + 1) == '\n')
            p++;
        }
    }

  return count;
}

char *
svn_cstring_join(const apr_array_header_t *strings,
                 const char *separator,
                 apr_pool_t *pool)
{
  svn_stringbuf_t *new_str = svn_stringbuf_create("", pool);
  int sep_len = strlen(separator);
  int i;

  for (i = 0; i < strings->nelts; i++)
    {
      const char *string = APR_ARRAY_IDX(strings, i, const char *);
      svn_stringbuf_appendbytes(new_str, string, strlen(string));
      svn_stringbuf_appendbytes(new_str, separator, sep_len);
    }
  return new_str->data;
}

int
svn_cstring_casecmp(const char *str1, const char *str2)
{
  for (;;)
    {
      const int a = *str1++;
      const int b = *str2++;
      const int cmp = svn_ctype_casecmp(a, b);
      if (cmp || !a || !b)
        return cmp;
    }
}
