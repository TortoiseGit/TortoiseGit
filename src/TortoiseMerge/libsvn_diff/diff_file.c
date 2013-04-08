/*
 * diff_file.c :  routines for doing diffs on files
 *
 * ====================================================================
 * Copyright (c) 2002-2009 CollabNet.  All rights reserved.
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


#include <apr.h>
#include <apr_pools.h>
#include <apr_general.h>
#include <apr_file_io.h>
#include <apr_file_info.h>
#include <apr_time.h>
#include <apr_mmap.h>
#include <apr_getopt.h>

#include <apr_general.h>
#include "svn_version.h"
#include "svn_io.h"
#include "svn_ctype.h"

#include "svn_error.h"
#include "svn_diff.h"
#include "svn_types.h"
#include "svn_string.h"
#include "svn_io.h"
#include "svn_utf.h"
#include "svn_pools.h"
#include "diff.h"

/* A token, i.e. a line read from a file. */
typedef struct svn_diff__file_token_t
{
  /* Next token in free list. */
  struct svn_diff__file_token_t *next;
  svn_diff_datasource_e datasource;
  /* Offset in the datasource. */
  apr_off_t offset;
  /* Offset of the normalized token (may skip leading whitespace) */
  apr_off_t norm_offset;
  /* Total length - before normalization. */
  apr_off_t raw_length;
  /* Total length - after normalization. */
  apr_off_t length;
} svn_diff__file_token_t;


typedef struct svn_diff__file_baton_t
{
  const svn_diff_file_options_t *options;
  const char *path[4];

  apr_file_t *file[4];
  apr_off_t size[4];

  int chunk[4];
  char *buffer[4];
  char *curp[4];
  char *endp[4];

  /* List of free tokens that may be reused. */
  svn_diff__file_token_t *tokens;

  svn_diff__normalize_state_t normalize_state[4];

  apr_pool_t *pool;
} svn_diff__file_baton_t;


/* Look for the start of an end-of-line sequence (i.e. CR or LF)
 * in the array pointed to by BUF, of length LEN.
 * If such a byte is found, return the pointer to it, else return NULL.
 */
static char *
find_eol_start(char *buf, apr_size_t len)
{
  for (; len > 0; ++buf, --len)
    {
      if (*buf == '\n' || *buf == '\r')
        return buf;
    }
  return NULL;
}

static int
datasource_to_index(svn_diff_datasource_e datasource)
{
  switch (datasource)
    {
    case svn_diff_datasource_original:
      return 0;

    case svn_diff_datasource_modified:
      return 1;

    case svn_diff_datasource_latest:
      return 2;

    case svn_diff_datasource_ancestor:
      return 3;
    }

  return -1;
}

/* Files are read in chunks of 128k.  There is no support for this number
 * whatsoever.  If there is a number someone comes up with that has some
 * argumentation, let's use that.
 */
#define CHUNK_SHIFT 17
#define CHUNK_SIZE (1 << CHUNK_SHIFT)

#define chunk_to_offset(chunk) ((chunk) << CHUNK_SHIFT)
#define offset_to_chunk(offset) ((offset) >> CHUNK_SHIFT)
#define offset_in_chunk(offset) ((offset) & (CHUNK_SIZE - 1))


/* Read a chunk from a FILE into BUFFER, starting from OFFSET, going for
 * *LENGTH.  The actual bytes read are stored in *LENGTH on return.
 */
static APR_INLINE svn_error_t *
read_chunk(apr_file_t *file, const char *path,
           char *buffer, apr_off_t length,
           apr_off_t offset, apr_pool_t *pool)
{
  /* XXX: The final offset may not be the one we asked for.
   * XXX: Check.
   */
  SVN_ERR(svn_io_file_seek(file, APR_SET, &offset, pool));
  return svn_io_file_read_full(file, buffer, (apr_size_t) length, NULL, pool);
}


/* Map or read a file at PATH. *BUFFER will point to the file
 * contents; if the file was mapped, *FILE and *MM will contain the
 * mmap context; otherwise they will be NULL.  SIZE will contain the
 * file size.  Allocate from POOL.
 */
#if APR_HAS_MMAP
#define MMAP_T_PARAM(NAME) apr_mmap_t **NAME,
#define MMAP_T_ARG(NAME)   &(NAME),
#else
#define MMAP_T_PARAM(NAME)
#define MMAP_T_ARG(NAME)
#endif

static svn_error_t *
map_or_read_file(apr_file_t **file,
                 MMAP_T_PARAM(mm)
                 char **buffer, apr_off_t *size,
                 const char *path, apr_pool_t *pool)
{
  apr_finfo_t finfo;
  apr_status_t rv;

  *buffer = NULL;

  SVN_ERR(svn_io_file_open(file, path, APR_READ, APR_OS_DEFAULT, pool));
  SVN_ERR(svn_io_file_info_get(&finfo, APR_FINFO_SIZE, *file, pool));

#if APR_HAS_MMAP
  if (finfo.size > APR_MMAP_THRESHOLD)
    {
      rv = apr_mmap_create(mm, *file, 0, (apr_size_t) finfo.size,
                           APR_MMAP_READ, pool);
      if (rv == APR_SUCCESS)
        {
          *buffer = (*mm)->mm;
        }

      /* On failure we just fall through and try reading the file into
       * memory instead.
       */
    }
#endif /* APR_HAS_MMAP */

   if (*buffer == NULL && finfo.size > 0)
    {
      *buffer = apr_palloc(pool, (apr_size_t) finfo.size);

      SVN_ERR(svn_io_file_read_full(*file, *buffer, (apr_size_t) finfo.size,
                                    NULL, pool));

      /* Since we have the entire contents of the file we can
       * close it now.
       */
      SVN_ERR(svn_io_file_close(*file, pool));

      *file = NULL;
    }

  *size = finfo.size;

  return SVN_NO_ERROR;
}


/* Implements svn_diff_fns_t::datasource_open */
static svn_error_t *
datasource_open(void *baton, svn_diff_datasource_e datasource)
{
  svn_diff__file_baton_t *file_baton = baton;
  int idx;
  apr_finfo_t finfo;
  apr_off_t length;
  char *curp;
  char *endp;

  idx = datasource_to_index(datasource);

  SVN_ERR(svn_io_file_open(&file_baton->file[idx], file_baton->path[idx],
                           APR_READ, APR_OS_DEFAULT, file_baton->pool));

  if( strcmp(file_baton->path[idx], "NUL") == 0)
  {
	  memset(&finfo, 0, sizeof(apr_finfo_t));

  }else
  {
	SVN_ERR(svn_io_file_info_get(&finfo, APR_FINFO_SIZE,
                               file_baton->file[idx], file_baton->pool));
  }

  file_baton->size[idx] = finfo.size;
  length = finfo.size > CHUNK_SIZE ? CHUNK_SIZE : finfo.size;

  if (length == 0)
    return SVN_NO_ERROR;

  endp = curp = apr_palloc(file_baton->pool, (apr_size_t) length);
  endp += length;

  file_baton->buffer[idx] = file_baton->curp[idx] = curp;
  file_baton->endp[idx] = endp;

  return read_chunk(file_baton->file[idx], file_baton->path[idx],
                    curp, length, 0, file_baton->pool);
}


/* Implements svn_diff_fns_t::datasource_close */
static svn_error_t *
datasource_close(void *baton, svn_diff_datasource_e datasource)
{
  /* Do nothing.  The compare_token function needs previous datasources
   * to stay available until all datasources are processed.
   */

  return SVN_NO_ERROR;
}

/* Implements svn_diff_fns_t::datasource_get_next_token */
static svn_error_t *
datasource_get_next_token(apr_uint32_t *hash, void **token, void *baton,
                          svn_diff_datasource_e datasource)
{
  svn_diff__file_baton_t *file_baton = baton;
  svn_diff__file_token_t *file_token;
  int idx;
  char *endp;
  char *curp;
  char *eol;
  apr_off_t last_chunk;
  apr_off_t length;
  apr_uint32_t h = 0;
  /* Did the last chunk end in a CR character? */
  svn_boolean_t had_cr = FALSE;

  *token = NULL;

  idx = datasource_to_index(datasource);

  curp = file_baton->curp[idx];
  endp = file_baton->endp[idx];

  last_chunk = offset_to_chunk(file_baton->size[idx]);

  if (curp == endp
      && last_chunk == file_baton->chunk[idx])
    {
      return SVN_NO_ERROR;
    }

  /* Get a new token */
  file_token = file_baton->tokens;
  if (file_token)
    {
      file_baton->tokens = file_token->next;
    }
  else
    {
      file_token = apr_palloc(file_baton->pool, sizeof(*file_token));
    }

  file_token->datasource = datasource;
  file_token->offset = chunk_to_offset(file_baton->chunk[idx])
                       + (curp - file_baton->buffer[idx]);
  file_token->raw_length = 0;
  file_token->length = 0;

  while (1)
    {
      eol = find_eol_start(curp, endp - curp);
      if (eol)
        {
          had_cr = (*eol == '\r');
          eol++;
          /* If we have the whole eol sequence in the chunk... */
          if (!had_cr || eol != endp)
            {
              if (had_cr && *eol == '\n')
                ++eol;
              break;
            }
        }

      if (file_baton->chunk[idx] == last_chunk)
        {
          eol = endp;
          break;
        }

      length = endp - curp;
      file_token->raw_length += length;
      svn_diff__normalize_buffer(&curp, &length,
                                 &file_baton->normalize_state[idx],
                                 curp, file_baton->options);
      file_token->length += length;
      h = svn_diff__adler32(h, curp, length);

      curp = endp = file_baton->buffer[idx];
      file_baton->chunk[idx]++;
      length = file_baton->chunk[idx] == last_chunk ?
        offset_in_chunk(file_baton->size[idx]) : CHUNK_SIZE;
      endp += length;
      file_baton->endp[idx] = endp;

      SVN_ERR(read_chunk(file_baton->file[idx], file_baton->path[idx],
                         curp, length,
                         chunk_to_offset(file_baton->chunk[idx]),
                         file_baton->pool));

      /* If the last chunk ended in a CR, we're done. */
      if (had_cr)
        {
          eol = curp;
          if (*curp == '\n')
            ++eol;
          break;
        }
    }

  length = eol - curp;
  file_token->raw_length += length;
  file_baton->curp[idx] = eol;

  /* If the file length is exactly a multiple of CHUNK_SIZE, we will end up
   * with a spurious empty token.  Avoid returning it.
   * Note that we use the unnormalized length; we don't want a line containing
   * only spaces (and no trailing newline) to appear like a non-existent
   * line. */
  if (file_token->raw_length > 0)
    {
      char *c = curp;
      svn_diff__normalize_buffer(&c, &length,
                                 &file_baton->normalize_state[idx],
                                 curp, file_baton->options);

      file_token->norm_offset = file_token->offset;
      if (file_token->length == 0) 
        /* move past leading ignored characters */
        file_token->norm_offset += (c - curp);

      file_token->length += length;

      *hash = svn_diff__adler32(h, c, length);
      *token = file_token;
    }

  return SVN_NO_ERROR;
}

#define COMPARE_CHUNK_SIZE 4096

/* Implements svn_diff_fns_t::token_compare */
static svn_error_t *
token_compare(void *baton, void *token1, void *token2, int *compare)
{
  svn_diff__file_baton_t *file_baton = baton;
  svn_diff__file_token_t *file_token[2];
  char buffer[2][COMPARE_CHUNK_SIZE];
  char *bufp[2];
  apr_off_t offset[2];
  int idx[2];
  apr_off_t length[2];
  apr_off_t total_length;
  /* How much is left to read of each token from the file. */
  apr_off_t raw_length[2];
  int i;
  int chunk[2];
  svn_diff__normalize_state_t state[2];

  file_token[0] = token1;
  file_token[1] = token2;
  if (file_token[0]->length < file_token[1]->length)
    {
      *compare = -1;
      return SVN_NO_ERROR;
    }

  if (file_token[0]->length > file_token[1]->length)
    {
      *compare = 1;
      return SVN_NO_ERROR;
    }

  total_length = file_token[0]->length;
  if (total_length == 0)
    {
      *compare = 0;
      return SVN_NO_ERROR;
    }

  for (i = 0; i < 2; ++i)
    {
      idx[i] = datasource_to_index(file_token[i]->datasource);
      offset[i] = file_token[i]->norm_offset;
      chunk[i] = file_baton->chunk[idx[i]];
      state[i] = svn_diff__normalize_state_normal;

      if (offset_to_chunk(offset[i]) == chunk[i])
        {
          /* If the start of the token is in memory, the entire token is
           * in memory.
           */
          bufp[i] = file_baton->buffer[idx[i]];
          bufp[i] += offset_in_chunk(offset[i]);

          length[i] = total_length;
          raw_length[i] = 0;
        }
      else
        {
          length[i] = 0;
          raw_length[i] = file_token[i]->raw_length;
        }
    }

  do
    {
      apr_off_t len;
      for (i = 0; i < 2; i++)
        {
          if (length[i] == 0)
            {
              /* Error if raw_length is 0, that's an unexpected change
               * of the file that can happen when ingoring whitespace
               * and that can lead to an infinite loop. */
              if (raw_length[i] == 0)
                return svn_error_createf(SVN_ERR_DIFF_DATASOURCE_MODIFIED,
                                         NULL,
                                         _("The file '%s' changed unexpectedly"
                                           " during diff"),
                                         file_baton->path[idx[i]]);

              /* Read a chunk from disk into a buffer */
              bufp[i] = buffer[i];
              length[i] = raw_length[i] > COMPARE_CHUNK_SIZE ?
                COMPARE_CHUNK_SIZE : raw_length[i];

              SVN_ERR(read_chunk(file_baton->file[idx[i]],
                                 file_baton->path[idx[i]],
                                 bufp[i], length[i], offset[i],
                                 file_baton->pool));
              offset[i] += length[i];
              raw_length[i] -= length[i];
              /* bufp[i] gets reset to buffer[i] before reading each chunk,
                 so, overwriting it isn't a problem */
              svn_diff__normalize_buffer(&bufp[i], &length[i], &state[i],
                                         bufp[i], file_baton->options);
            }
        }

      len = length[0] > length[1] ? length[1] : length[0];

      /* Compare two chunks (that could be entire tokens if they both reside
       * in memory).
       */
      *compare = memcmp(bufp[0], bufp[1], (size_t) len);
      if (*compare != 0)
        return SVN_NO_ERROR;

      total_length -= len;
      length[0] -= len;
      length[1] -= len;
      bufp[0] += len;
      bufp[1] += len;
    }
  while(total_length > 0);

  *compare = 0;
  return SVN_NO_ERROR;
}


/* Implements svn_diff_fns_t::token_discard */
static void
token_discard(void *baton, void *token)
{
  svn_diff__file_baton_t *file_baton = baton;
  svn_diff__file_token_t *file_token = token;

  file_token->next = file_baton->tokens;
  file_baton->tokens = file_token;
}


/* Implements svn_diff_fns_t::token_discard_all */
static void
token_discard_all(void *baton)
{
  svn_diff__file_baton_t *file_baton = baton;

  /* Discard all memory in use by the tokens, and close all open files. */
  svn_pool_clear(file_baton->pool);
}


static const svn_diff_fns_t svn_diff__file_vtable =
{
  datasource_open,
  datasource_close,
  datasource_get_next_token,
  token_compare,
  token_discard,
  token_discard_all
};

/* Id for the --ignore-eol-style option, which doesn't have a short name. */
#define SVN_DIFF__OPT_IGNORE_EOL_STYLE 256

/* Options supported by svn_diff_file_options_parse(). */
static const apr_getopt_option_t diff_options[] =
{
  { "ignore-space-change", 'b', 0, NULL },
  { "ignore-all-space", 'w', 0, NULL },
  { "ignore-eol-style", SVN_DIFF__OPT_IGNORE_EOL_STYLE, 0, NULL },
  { "show-c-function", 'p', 0, NULL },
  /* ### For compatibility; we don't support the argument to -u, because
   * ### we don't have optional argument support. */
  { "unified", 'u', 0, NULL },
  { NULL, 0, 0, NULL }
};

svn_diff_file_options_t *
svn_diff_file_options_create(apr_pool_t *pool)
{
  return apr_pcalloc(pool, sizeof(svn_diff_file_options_t));
}

svn_error_t *
svn_diff_file_options_parse(svn_diff_file_options_t *options,
                            const apr_array_header_t *args,
                            apr_pool_t *pool)
{
  apr_getopt_t *os;
  /* Make room for each option (starting at index 1) plus trailing NULL. */
  const char **argv = apr_palloc(pool, sizeof(char*) * (args->nelts + 2));

  argv[0] = "";
  memcpy((void *) (argv + 1), args->elts, sizeof(char*) * args->nelts);
  argv[args->nelts + 1] = NULL;

  apr_getopt_init(&os, pool, args->nelts + 1, argv);
  /* No printing of error messages, please! */
  os->errfn = NULL;
  while (1)
    {
      const char *opt_arg;
      int opt_id;
      apr_status_t err = apr_getopt_long(os, diff_options, &opt_id, &opt_arg);

      if (APR_STATUS_IS_EOF(err))
        break;
      if (err)
        return svn_error_wrap_apr(err, _("Error parsing diff options"));

      switch (opt_id)
        {
        case 'b':
          /* -w takes precedence over -b. */
          if (! options->ignore_space)
            options->ignore_space = svn_diff_file_ignore_space_change;
          break;
        case 'w':
          options->ignore_space = svn_diff_file_ignore_space_all;
          break;
        case SVN_DIFF__OPT_IGNORE_EOL_STYLE:
          options->ignore_eol_style = TRUE;
          break;
        case 'p':
          options->show_c_function = TRUE;
          break;
        default:
          break;
        }
    }

  /* Check for spurious arguments. */
  if (os->ind < os->argc)
    return svn_error_createf(SVN_ERR_INVALID_DIFF_OPTION, NULL,
                             _("Invalid argument '%s' in diff options"),
                             os->argv[os->ind]);

  return SVN_NO_ERROR;
}

svn_error_t *
svn_diff_file_diff_2(svn_diff_t **diff,
                     const char *original,
                     const char *modified,
                     const svn_diff_file_options_t *options,
                     apr_pool_t *pool)
{
  svn_diff__file_baton_t baton;

  memset(&baton, 0, sizeof(baton));
  baton.options = options;
  baton.path[0] = original;
  baton.path[1] = modified;
  baton.pool = svn_pool_create(pool);

  SVN_ERR(svn_diff_diff(diff, &baton, &svn_diff__file_vtable, pool));

  svn_pool_destroy(baton.pool);
  return SVN_NO_ERROR;
}

svn_error_t *
svn_diff_file_diff(svn_diff_t **diff,
                   const char *original,
                   const char *modified,
                   apr_pool_t *pool)
{
  return svn_diff_file_diff_2(diff, original, modified,
                              svn_diff_file_options_create(pool), pool);
}

svn_error_t *
svn_diff_file_diff3_2(svn_diff_t **diff,
                      const char *original,
                      const char *modified,
                      const char *latest,
                      const svn_diff_file_options_t *options,
                      apr_pool_t *pool)
{
  svn_diff__file_baton_t baton;

  memset(&baton, 0, sizeof(baton));
  baton.options = options;
  baton.path[0] = original;
  baton.path[1] = modified;
  baton.path[2] = latest;
  baton.pool = svn_pool_create(pool);

  SVN_ERR(svn_diff_diff3(diff, &baton, &svn_diff__file_vtable, pool));

  svn_pool_destroy(baton.pool);
  return SVN_NO_ERROR;
}

svn_error_t *
svn_diff_file_diff3(svn_diff_t **diff,
                    const char *original,
                    const char *modified,
                    const char *latest,
                    apr_pool_t *pool)
{
  return svn_diff_file_diff3_2(diff, original, modified, latest,
                               svn_diff_file_options_create(pool), pool);
}

svn_error_t *
svn_diff_file_diff4_2(svn_diff_t **diff,
                      const char *original,
                      const char *modified,
                      const char *latest,
                      const char *ancestor,
                      const svn_diff_file_options_t *options,
                      apr_pool_t *pool)
{
  svn_diff__file_baton_t baton;

  memset(&baton, 0, sizeof(baton));
  baton.options = options;
  baton.path[0] = original;
  baton.path[1] = modified;
  baton.path[2] = latest;
  baton.path[3] = ancestor;
  baton.pool = svn_pool_create(pool);

  SVN_ERR(svn_diff_diff4(diff, &baton, &svn_diff__file_vtable, pool));

  svn_pool_destroy(baton.pool);
  return SVN_NO_ERROR;
}

svn_error_t *
svn_diff_file_diff4(svn_diff_t **diff,
                    const char *original,
                    const char *modified,
                    const char *latest,
                    const char *ancestor,
                    apr_pool_t *pool)
{
  return svn_diff_file_diff4_2(diff, original, modified, latest, ancestor,
                               svn_diff_file_options_create(pool), pool);
}

/** Display unified context diffs **/

/* Maximum length of the extra context to show when show_c_function is set.
 * GNU diff uses 40, let's be brave and use 50 instead. */
#define SVN_DIFF__EXTRA_CONTEXT_LENGTH 50
typedef struct svn_diff__file_output_baton_t
{
  svn_stream_t *output_stream;
  const char *header_encoding;

  /* Cached markers, in header_encoding. */
  const char *context_str;
  const char *delete_str;
  const char *insert_str;

  const char *path[2];
  apr_file_t *file[2];

  apr_off_t   current_line[2];

  char        buffer[2][4096];
  apr_size_t  length[2];
  char       *curp[2];

  apr_off_t   hunk_start[2];
  apr_off_t   hunk_length[2];
  svn_stringbuf_t *hunk;

  /* Should we emit C functions in the unified diff header */
  svn_boolean_t show_c_function;
  /* Extra strings to skip over if we match. */
  apr_array_header_t *extra_skip_match;
  /* "Context" to append to the @@ line when the show_c_function option
   * is set. */
  svn_stringbuf_t *extra_context;
  /* Extra context for the current hunk. */
  char hunk_extra_context[SVN_DIFF__EXTRA_CONTEXT_LENGTH + 1];

  apr_pool_t *pool;
} svn_diff__file_output_baton_t;

typedef enum svn_diff__file_output_unified_type_e
{
  svn_diff__file_output_unified_skip,
  svn_diff__file_output_unified_context,
  svn_diff__file_output_unified_delete,
  svn_diff__file_output_unified_insert
} svn_diff__file_output_unified_type_e;


static svn_error_t *
output_unified_line(svn_diff__file_output_baton_t *baton,
                    svn_diff__file_output_unified_type_e type, int idx)
{
  char *curp;
  char *eol;
  apr_size_t length;
  svn_error_t *err;
  svn_boolean_t bytes_processed = FALSE;
  svn_boolean_t had_cr = FALSE;
  /* Are we collecting extra context? */
  svn_boolean_t collect_extra = FALSE;

  length = baton->length[idx];
  curp = baton->curp[idx];

  /* Lazily update the current line even if we're at EOF.
   * This way we fake output of context at EOF
   */
  baton->current_line[idx]++;

  if (length == 0 && apr_file_eof(baton->file[idx]))
    {
      return SVN_NO_ERROR;
    }

  do
    {
      if (length > 0)
        {
          if (!bytes_processed)
            {
              switch (type)
                {
                case svn_diff__file_output_unified_context:
                  svn_stringbuf_appendcstr(baton->hunk, baton->context_str);
                  baton->hunk_length[0]++;
                  baton->hunk_length[1]++;
                  break;
                case svn_diff__file_output_unified_delete:
                  svn_stringbuf_appendcstr(baton->hunk, baton->delete_str);
                  baton->hunk_length[0]++;
                  break;
                case svn_diff__file_output_unified_insert:
                  svn_stringbuf_appendcstr(baton->hunk, baton->insert_str);
                  baton->hunk_length[1]++;
                  break;
                default:
                  break;
                }

              if (baton->show_c_function
                  && (type == svn_diff__file_output_unified_skip
                      || type == svn_diff__file_output_unified_context)
                  && (svn_ctype_isalpha(*curp) || *curp == '$' || *curp == '_')
                  && !svn_cstring_match_glob_list(curp,
                                                  baton->extra_skip_match))
                {
                  svn_stringbuf_setempty(baton->extra_context);
                  collect_extra = TRUE;
                }
            }

          eol = find_eol_start(curp, length);

          if (eol != NULL)
            {
              apr_size_t len;

              had_cr = (*eol == '\r');
              eol++;
              len = (apr_size_t)(eol - curp);

              if (! had_cr || len < length)
                {
                  if (had_cr && *eol == '\n')
                    {
                      ++eol;
                      ++len;
                    }

                  length -= len;

                  if (type != svn_diff__file_output_unified_skip)
                    {
                      svn_stringbuf_appendbytes(baton->hunk, curp, len);
                    }
                  if (collect_extra)
                    {
                      svn_stringbuf_appendbytes(baton->extra_context,
                                                curp, len);
                    }

                  baton->curp[idx] = eol;
                  baton->length[idx] = length;

                  err = SVN_NO_ERROR;

                  break;
                }
            }

          if (type != svn_diff__file_output_unified_skip)
            {
              svn_stringbuf_appendbytes(baton->hunk, curp, length);
            }

          if (collect_extra)
            {
              svn_stringbuf_appendbytes(baton->extra_context, curp, length);
            }

          bytes_processed = TRUE;
        }

      curp = baton->buffer[idx];
      length = sizeof(baton->buffer[idx]);

      err = svn_io_file_read(baton->file[idx], curp, &length, baton->pool);

      /* If the last chunk ended with a CR, we look for an LF at the start
         of this chunk. */
      if (had_cr)
        {
          if (! err && length > 0 && *curp == '\n')
            {
              if (type != svn_diff__file_output_unified_skip)
                {
                  svn_stringbuf_appendbytes(baton->hunk, curp, 1);
                }
              /* We don't append the LF to extra_context, since it would
               * just be stripped anyway. */
              ++curp;
              --length;
            }

          baton->curp[idx] = curp;
          baton->length[idx] = length;

          break;
        }
    }
  while (! err);

  if (err && ! APR_STATUS_IS_EOF(err->apr_err))
    return err;

  if (err && APR_STATUS_IS_EOF(err->apr_err))
    {
      svn_error_clear(err);
      /* Special case if we reach the end of file AND the last line is in the
         changed range AND the file doesn't end with a newline */
      if (bytes_processed && (type != svn_diff__file_output_unified_skip)
          && ! had_cr)
        {
          const char *out_str;
          SVN_ERR(svn_utf_cstring_from_utf8_ex2
                  (&out_str,
                   /* The string below is intentionally not marked for
                      translation: it's vital to correct operation of
                      the diff(1)/patch(1) program pair. */
                   APR_EOL_STR "\\ No newline at end of file" APR_EOL_STR,
                   baton->header_encoding, baton->pool));
          svn_stringbuf_appendcstr(baton->hunk, out_str);
        }

      baton->length[idx] = 0;
    }

  return SVN_NO_ERROR;
}

static svn_error_t *
output_unified_flush_hunk(svn_diff__file_output_baton_t *baton)
{
  apr_off_t target_line;
  apr_size_t hunk_len;
  int i;

  if (svn_stringbuf_isempty(baton->hunk))
    {
      /* Nothing to flush */
      return SVN_NO_ERROR;
    }

  target_line = baton->hunk_start[0] + baton->hunk_length[0]
                + SVN_DIFF__UNIFIED_CONTEXT_SIZE;

  /* Add trailing context to the hunk */
  while (baton->current_line[0] < target_line)
    {
      SVN_ERR(output_unified_line
              (baton, svn_diff__file_output_unified_context, 0));
    }

  /* If the file is non-empty, convert the line indexes from
     zero based to one based */
  for (i = 0; i < 2; i++)
    {
      if (baton->hunk_length[i] > 0)
        baton->hunk_start[i]++;
    }

  /* Output the hunk header.  If the hunk length is 1, the file is a one line
     file.  In this case, surpress the number of lines in the hunk (it is
     1 implicitly)
   */
  SVN_ERR(svn_stream_printf_from_utf8(baton->output_stream,
                                      baton->header_encoding,
                                      baton->pool,
                                      "@@ -%" APR_OFF_T_FMT,
                                      baton->hunk_start[0]));
  if (baton->hunk_length[0] != 1)
    {
      SVN_ERR(svn_stream_printf_from_utf8(baton->output_stream,
                                          baton->header_encoding,
                                          baton->pool, ",%" APR_OFF_T_FMT,
                                          baton->hunk_length[0]));
    }

  SVN_ERR(svn_stream_printf_from_utf8(baton->output_stream,
                                      baton->header_encoding,
                                      baton->pool, " +%" APR_OFF_T_FMT,
                                      baton->hunk_start[1]));
  if (baton->hunk_length[1] != 1)
    {
      SVN_ERR(svn_stream_printf_from_utf8(baton->output_stream,
                                          baton->header_encoding,
                                          baton->pool, ",%" APR_OFF_T_FMT,
                                          baton->hunk_length[1]));
    }

  SVN_ERR(svn_stream_printf_from_utf8(baton->output_stream,
                                      baton->header_encoding,
                                      baton->pool, " @@%s%s" APR_EOL_STR,
                                      baton->hunk_extra_context[0]
                                      ? " " : "",
                                      baton->hunk_extra_context));

  /* Output the hunk content */
  hunk_len = baton->hunk->len;
  SVN_ERR(svn_stream_write(baton->output_stream, baton->hunk->data,
                           &hunk_len));

  /* Prepare for the next hunk */
  baton->hunk_length[0] = 0;
  baton->hunk_length[1] = 0;
  svn_stringbuf_setempty(baton->hunk);

  return SVN_NO_ERROR;
}

static svn_error_t *
output_unified_diff_modified(void *baton,
  apr_off_t original_start, apr_off_t original_length,
  apr_off_t modified_start, apr_off_t modified_length,
  apr_off_t latest_start, apr_off_t latest_length)
{
  svn_diff__file_output_baton_t *output_baton = baton;
  apr_off_t target_line[2];
  int i;

  target_line[0] = original_start >= SVN_DIFF__UNIFIED_CONTEXT_SIZE
                   ? original_start - SVN_DIFF__UNIFIED_CONTEXT_SIZE : 0;
  target_line[1] = modified_start;

  /* If the changed ranges are far enough apart (no overlapping or connecting
     context), flush the current hunk, initialize the next hunk and skip the
     lines not in context.  Also do this when this is the first hunk.
   */
  if (output_baton->current_line[0] < target_line[0]
      && (output_baton->hunk_start[0] + output_baton->hunk_length[0]
          + SVN_DIFF__UNIFIED_CONTEXT_SIZE < target_line[0]
          || output_baton->hunk_length[0] == 0))
    {
      SVN_ERR(output_unified_flush_hunk(output_baton));

      output_baton->hunk_start[0] = target_line[0];
      output_baton->hunk_start[1] = target_line[1] + target_line[0]
                                    - original_start;

      /* Skip lines until we are at the beginning of the context we want to
         display */
      while (output_baton->current_line[0] < target_line[0])
        {
          SVN_ERR(output_unified_line(output_baton,
                                      svn_diff__file_output_unified_skip, 0));
        }

      if (output_baton->show_c_function)
        {
          int p;
          const char *invalid_character;

          /* Save the extra context for later use.
           * Note that the last byte of the hunk_extra_context array is never
           * touched after it is zero-initialized, so the array is always
           * 0-terminated. */
          strncpy(output_baton->hunk_extra_context,
                  output_baton->extra_context->data,
                  SVN_DIFF__EXTRA_CONTEXT_LENGTH);
          /* Trim whitespace at the end, most notably to get rid of any
           * newline characters. */
          p = strlen(output_baton->hunk_extra_context);
          while (p > 0
                 && svn_ctype_isspace(output_baton->hunk_extra_context[p - 1]))
            {
              output_baton->hunk_extra_context[--p] = '\0';
            }
          invalid_character =
            svn_utf__last_valid(output_baton->hunk_extra_context,
                                SVN_DIFF__EXTRA_CONTEXT_LENGTH);
          for (p = invalid_character - output_baton->hunk_extra_context;
               p < SVN_DIFF__EXTRA_CONTEXT_LENGTH; p++)
            {
              output_baton->hunk_extra_context[p] = '\0';
            }
        }
    }

  /* Skip lines until we are at the start of the changed range */
  while (output_baton->current_line[1] < target_line[1])
    {
      SVN_ERR(output_unified_line(output_baton,
                                  svn_diff__file_output_unified_skip, 1));
    }

  /* Output the context preceding the changed range */
  while (output_baton->current_line[0] < original_start)
    {
      SVN_ERR(output_unified_line(output_baton,
                                  svn_diff__file_output_unified_context, 0));
    }

  target_line[0] = original_start + original_length;
  target_line[1] = modified_start + modified_length;

  /* Output the changed range */
  for (i = 0; i < 2; i++)
    {
      while (output_baton->current_line[i] < target_line[i])
        {
          SVN_ERR(output_unified_line
                          (output_baton,
                           i == 0 ? svn_diff__file_output_unified_delete
                                  : svn_diff__file_output_unified_insert, i));
        }
    }

  return SVN_NO_ERROR;
}

/* Set *HEADER to a new string consisting of PATH, a tab, and PATH's mtime. */
static svn_error_t *
output_unified_default_hdr(const char **header, const char *path,
                           apr_pool_t *pool)
{
  apr_finfo_t file_info;
  apr_time_exp_t exploded_time;
  char time_buffer[64];
  apr_size_t time_len;
  const char *utf8_timestr;

  SVN_ERR(svn_io_stat(&file_info, path, APR_FINFO_MTIME, pool));
  apr_time_exp_lt(&exploded_time, file_info.mtime);

  apr_strftime(time_buffer, &time_len, sizeof(time_buffer) - 1,
  /* Order of date components can be different in different languages */
               _("%a %b %e %H:%M:%S %Y"), &exploded_time);

  SVN_ERR(svn_utf_cstring_to_utf8(&utf8_timestr, time_buffer, pool));

  *header = apr_psprintf(pool, "%s\t%s", path, utf8_timestr);

  return SVN_NO_ERROR;
}

static const svn_diff_output_fns_t svn_diff__file_output_unified_vtable =
{
  NULL, /* output_common */
  output_unified_diff_modified,
  NULL, /* output_diff_latest */
  NULL, /* output_diff_common */
  NULL  /* output_conflict */
};

svn_error_t *
svn_diff_file_output_unified3(svn_stream_t *output_stream,
                              svn_diff_t *diff,
                              const char *original_path,
                              const char *modified_path,
                              const char *original_header,
                              const char *modified_header,
                              const char *header_encoding,
                              const char *relative_to_dir,
                              svn_boolean_t show_c_function,
                              apr_pool_t *pool)
{
  svn_diff__file_output_baton_t baton;
  int i;

  if (svn_diff_contains_diffs(diff))
    {
      const char **c;

      memset(&baton, 0, sizeof(baton));
      baton.output_stream = output_stream;
      baton.pool = pool;
      baton.header_encoding = header_encoding;
      baton.path[0] = original_path;
      baton.path[1] = modified_path;
      baton.hunk = svn_stringbuf_create("", pool);
      baton.show_c_function = show_c_function;
      baton.extra_context = svn_stringbuf_create("", pool);
      baton.extra_skip_match = apr_array_make(pool, 3, sizeof(char **));

      c = apr_array_push(baton.extra_skip_match);
      *c = "public:*";
      c = apr_array_push(baton.extra_skip_match);
      *c = "private:*";
      c = apr_array_push(baton.extra_skip_match);
      *c = "protected:*";

      SVN_ERR(svn_utf_cstring_from_utf8_ex2(&baton.context_str, " ",
                                            header_encoding, pool));
      SVN_ERR(svn_utf_cstring_from_utf8_ex2(&baton.delete_str, "-",
                                            header_encoding, pool));
      SVN_ERR(svn_utf_cstring_from_utf8_ex2(&baton.insert_str, "+",
                                            header_encoding, pool));

  if (relative_to_dir)
    {
      /* Possibly adjust the "original" and "modified" paths shown in
         the output (see issue #2723). */
      const char *child_path;

      if (! original_header)
        {
          child_path = svn_path_is_child(relative_to_dir,
                                         original_path, pool);
          if (child_path)
            original_path = child_path;
          else
            return svn_error_createf(SVN_ERR_BAD_RELATIVE_PATH, NULL,
                                     _("Path '%s' must be an immediate child of "
                                       "the directory '%s'"),
                                     original_path, relative_to_dir);
        }

      if (! modified_header)
        {
          child_path = svn_path_is_child(relative_to_dir, modified_path, pool);
          if (child_path)
            modified_path = child_path;
          else
            return svn_error_createf(SVN_ERR_BAD_RELATIVE_PATH, NULL,
                                     _("Path '%s' must be an immediate child of "
                                       "the directory '%s'"),
                                     modified_path, relative_to_dir);
        }
    }

      for (i = 0; i < 2; i++)
        {
          SVN_ERR(svn_io_file_open(&baton.file[i], baton.path[i],
                                   APR_READ, APR_OS_DEFAULT, pool));
        }

      if (original_header == NULL)
        {
          SVN_ERR(output_unified_default_hdr
                  (&original_header, original_path, pool));
        }

      if (modified_header == NULL)
        {
          SVN_ERR(output_unified_default_hdr
                  (&modified_header, modified_path, pool));
        }

      SVN_ERR(svn_stream_printf_from_utf8(output_stream, header_encoding, pool,
                                          "--- %s" APR_EOL_STR
                                          "+++ %s" APR_EOL_STR,
                                          original_header, modified_header));

      SVN_ERR(svn_diff_output(diff, &baton,
                              &svn_diff__file_output_unified_vtable));
      SVN_ERR(output_unified_flush_hunk(&baton));

      for (i = 0; i < 2; i++)
        {
          SVN_ERR(svn_io_file_close(baton.file[i], pool));
        }
    }

  return SVN_NO_ERROR;
}


/** Display diff3 **/

/* A stream to remember *leading* context.  Note that this stream does
   *not* copy the data that it is remembering; it just saves
   *pointers! */
typedef struct {
  svn_stream_t *stream;
  const char *data[SVN_DIFF__UNIFIED_CONTEXT_SIZE];
  apr_size_t len[SVN_DIFF__UNIFIED_CONTEXT_SIZE];
  apr_size_t next_slot;
  apr_size_t total_written;
} context_saver_t;


static svn_error_t *
context_saver_stream_write(void *baton,
                           const char *data,
                           apr_size_t *len)
{
  context_saver_t *cs = baton;
  cs->data[cs->next_slot] = data;
  cs->len[cs->next_slot] = *len;
  cs->next_slot = (cs->next_slot + 1) % SVN_DIFF__UNIFIED_CONTEXT_SIZE;
  cs->total_written++;
  return SVN_NO_ERROR;
}

typedef struct svn_diff3__file_output_baton_t
{
  svn_stream_t *output_stream;

  const char *path[3];

  apr_off_t   current_line[3];

  char       *buffer[3];
  char       *endp[3];
  char       *curp[3];

  /* The following four members are in the encoding used for the output. */
  const char *conflict_modified;
  const char *conflict_original;
  const char *conflict_separator;
  const char *conflict_latest;

  const char *marker_eol;

  svn_diff_conflict_display_style_t conflict_style;

  /* The rest of the fields are for
     svn_diff_conflict_display_only_conflicts only.  Note that for
     these batons, OUTPUT_STREAM is either CONTEXT_SAVER->STREAM or
     (soon after a conflict) a "trailing context stream", never the
     actual output stream.*/
  /* The actual output stream. */
  svn_stream_t *real_output_stream;
  context_saver_t *context_saver;
  /* Used to allocate context_saver and trailing context streams, and
     for some printfs. */
  apr_pool_t *pool;
} svn_diff3__file_output_baton_t;

static svn_error_t *
flush_context_saver(context_saver_t *cs,
                    svn_stream_t *output_stream)
{
  int i;
  for (i = 0; i < SVN_DIFF__UNIFIED_CONTEXT_SIZE; i++)
    {
      int slot = (i + cs->next_slot) % SVN_DIFF__UNIFIED_CONTEXT_SIZE;
      if (cs->data[slot])
        {
          apr_size_t len = cs->len[slot];
          SVN_ERR(svn_stream_write(output_stream, cs->data[slot], &len));
        }
    }
  return SVN_NO_ERROR;
}

static void
make_context_saver(svn_diff3__file_output_baton_t *fob)
{
  context_saver_t *cs;

  svn_pool_clear(fob->pool);
  cs = apr_pcalloc(fob->pool, sizeof(*cs));
  cs->stream = svn_stream_empty(fob->pool);
  svn_stream_set_baton(cs->stream, cs);
  svn_stream_set_write(cs->stream, context_saver_stream_write);
  fob->context_saver = cs;
  fob->output_stream = cs->stream;
}


/* A stream which prints SVN_DIFF__UNIFIED_CONTEXT_SIZE lines to
   BATON->REAL_OUTPUT_STREAM, and then changes BATON->OUTPUT_STREAM to
   a context_saver; used for *trailing* context. */

struct trailing_context_printer {
  apr_size_t lines_to_print;
  svn_diff3__file_output_baton_t *fob;
};



static svn_error_t *
trailing_context_printer_write(void *baton,
                               const char *data,
                               apr_size_t *len)
{
  struct trailing_context_printer *tcp = baton;
  SVN_ERR_ASSERT(tcp->lines_to_print > 0);
  SVN_ERR(svn_stream_write(tcp->fob->real_output_stream, data, len));
  tcp->lines_to_print--;
  if (tcp->lines_to_print == 0)
    make_context_saver(tcp->fob);
  return SVN_NO_ERROR;
}


static void
make_trailing_context_printer(svn_diff3__file_output_baton_t *btn)
{
  struct trailing_context_printer *tcp;
  svn_stream_t *s;

  svn_pool_clear(btn->pool);

  tcp = apr_pcalloc(btn->pool, sizeof(*tcp));
  tcp->lines_to_print = SVN_DIFF__UNIFIED_CONTEXT_SIZE;
  tcp->fob = btn;
  s = svn_stream_empty(btn->pool);
  svn_stream_set_baton(s, tcp);
  svn_stream_set_write(s, trailing_context_printer_write);
  btn->output_stream = s;
}



typedef enum svn_diff3__file_output_type_e
{
  svn_diff3__file_output_skip,
  svn_diff3__file_output_normal
} svn_diff3__file_output_type_e;


static svn_error_t *
output_line(svn_diff3__file_output_baton_t *baton,
            svn_diff3__file_output_type_e type, int idx)
{
  char *curp;
  char *endp;
  char *eol;
  apr_size_t len;

  curp = baton->curp[idx];
  endp = baton->endp[idx];

  /* Lazily update the current line even if we're at EOF.
   */
  baton->current_line[idx]++;

  if (curp == endp)
    return SVN_NO_ERROR;

  eol = find_eol_start(curp, endp - curp);
  if (!eol)
    eol = endp;
  else
    {
      svn_boolean_t had_cr = (*eol == '\r');
      eol++;
      if (had_cr && eol != endp && *eol == '\n')
        eol++;
    }

  if (type != svn_diff3__file_output_skip)
    {
      len = eol - curp;
      /* Note that the trailing context printer assumes that
         svn_stream_write is called exactly once per line. */
      SVN_ERR(svn_stream_write(baton->output_stream, curp, &len));
    }

  baton->curp[idx] = eol;

  return SVN_NO_ERROR;
}

static svn_error_t *
output_marker_eol(svn_diff3__file_output_baton_t *btn)
{
  apr_size_t len = strlen(btn->marker_eol);
  return svn_stream_write(btn->output_stream, btn->marker_eol, &len);
}

static svn_error_t *
output_hunk(void *baton, int idx, apr_off_t target_line,
            apr_off_t target_length)
{
  svn_diff3__file_output_baton_t *output_baton = baton;

  /* Skip lines until we are at the start of the changed range */
  while (output_baton->current_line[idx] < target_line)
    {
      SVN_ERR(output_line(output_baton, svn_diff3__file_output_skip, idx));
    }

  target_line += target_length;

  while (output_baton->current_line[idx] < target_line)
    {
      SVN_ERR(output_line(output_baton, svn_diff3__file_output_normal, idx));
    }

  return SVN_NO_ERROR;
}

static svn_error_t *
output_common(void *baton, apr_off_t original_start, apr_off_t original_length,
              apr_off_t modified_start, apr_off_t modified_length,
              apr_off_t latest_start, apr_off_t latest_length)
{
  return output_hunk(baton, 1, modified_start, modified_length);
}

static svn_error_t *
output_diff_modified(void *baton,
                     apr_off_t original_start, apr_off_t original_length,
                     apr_off_t modified_start, apr_off_t modified_length,
                     apr_off_t latest_start, apr_off_t latest_length)
{
  return output_hunk(baton, 1, modified_start, modified_length);
}

static svn_error_t *
output_diff_latest(void *baton,
                   apr_off_t original_start, apr_off_t original_length,
                   apr_off_t modified_start, apr_off_t modified_length,
                   apr_off_t latest_start, apr_off_t latest_length)
{
  return output_hunk(baton, 2, latest_start, latest_length);
}

static svn_error_t *
output_conflict(void *baton,
                apr_off_t original_start, apr_off_t original_length,
                apr_off_t modified_start, apr_off_t modified_length,
                apr_off_t latest_start, apr_off_t latest_length,
                svn_diff_t *diff);

static const svn_diff_output_fns_t svn_diff3__file_output_vtable =
{
  output_common,
  output_diff_modified,
  output_diff_latest,
  output_diff_modified, /* output_diff_common */
  output_conflict
};



static svn_error_t *
output_conflict_with_context(svn_diff3__file_output_baton_t *btn,
                             apr_off_t original_start,
                             apr_off_t original_length,
                             apr_off_t modified_start,
                             apr_off_t modified_length,
                             apr_off_t latest_start,
                             apr_off_t latest_length)
{
  /* Are we currently saving starting context (as opposed to printing
     trailing context)?  If so, flush it. */
  if (btn->output_stream == btn->context_saver->stream)
    {
      if (btn->context_saver->total_written > SVN_DIFF__UNIFIED_CONTEXT_SIZE)
        SVN_ERR(svn_stream_printf(btn->real_output_stream, btn->pool, "@@\n"));
      SVN_ERR(flush_context_saver(btn->context_saver, btn->real_output_stream));
    }

  /* Print to the real output stream. */
  btn->output_stream = btn->real_output_stream;

  /* Output the conflict itself. */
  SVN_ERR(svn_stream_printf(btn->output_stream, btn->pool,
                            (modified_length == 1
                             ? "%s (%" APR_OFF_T_FMT ")"
                             : "%s (%" APR_OFF_T_FMT ",%" APR_OFF_T_FMT ")"),
                            btn->conflict_modified,
                            modified_start + 1, modified_length));
  SVN_ERR(output_marker_eol(btn));
  SVN_ERR(output_hunk(btn, 1/*modified*/, modified_start, modified_length));

  SVN_ERR(svn_stream_printf(btn->output_stream, btn->pool,
                            (original_length == 1
                             ? "%s (%" APR_OFF_T_FMT ")"
                             : "%s (%" APR_OFF_T_FMT ",%" APR_OFF_T_FMT ")"),
                            btn->conflict_original,
                            original_start + 1, original_length));
  SVN_ERR(output_marker_eol(btn));
  SVN_ERR(output_hunk(btn, 0/*original*/, original_start, original_length));

  SVN_ERR(svn_stream_printf(btn->output_stream, btn->pool,
                            "%s%s", btn->conflict_separator, btn->marker_eol));
  SVN_ERR(output_hunk(btn, 2/*latest*/, latest_start, latest_length));
  SVN_ERR(svn_stream_printf(btn->output_stream, btn->pool,
                            (latest_length == 1
                             ? "%s (%" APR_OFF_T_FMT ")"
                             : "%s (%" APR_OFF_T_FMT ",%" APR_OFF_T_FMT ")"),
                            btn->conflict_latest,
                            latest_start + 1, latest_length));
  SVN_ERR(output_marker_eol(btn));

  /* Go into print-trailing-context mode instead. */
  make_trailing_context_printer(btn);

  return SVN_NO_ERROR;
}


static svn_error_t *
output_conflict(void *baton,
                apr_off_t original_start, apr_off_t original_length,
                apr_off_t modified_start, apr_off_t modified_length,
                apr_off_t latest_start, apr_off_t latest_length,
                svn_diff_t *diff)
{
  svn_diff3__file_output_baton_t *file_baton = baton;
  apr_size_t len;

  svn_diff_conflict_display_style_t style = file_baton->conflict_style;

  if (style == svn_diff_conflict_display_only_conflicts)
    return output_conflict_with_context(file_baton,
                                        original_start, original_length,
                                        modified_start, modified_length,
                                        latest_start, latest_length);

  if (style == svn_diff_conflict_display_resolved_modified_latest)
    {
      if (diff)
        return svn_diff_output(diff, baton,
                               &svn_diff3__file_output_vtable);
      else
        style = svn_diff_conflict_display_modified_latest;
    }

  if (style == svn_diff_conflict_display_modified_latest ||
      style == svn_diff_conflict_display_modified_original_latest)
    {
      len = strlen(file_baton->conflict_modified);
      SVN_ERR(svn_stream_write(file_baton->output_stream,
                               file_baton->conflict_modified,
                               &len));
      SVN_ERR(output_marker_eol(file_baton));

      SVN_ERR(output_hunk(baton, 1, modified_start, modified_length));

      if (style == svn_diff_conflict_display_modified_original_latest)
        {
          len = strlen(file_baton->conflict_original);
          SVN_ERR(svn_stream_write(file_baton->output_stream,
                                   file_baton->conflict_original, &len));
          SVN_ERR(output_marker_eol(file_baton));
          SVN_ERR(output_hunk(baton, 0, original_start, original_length));
        }

      len = strlen(file_baton->conflict_separator);
      SVN_ERR(svn_stream_write(file_baton->output_stream,
                               file_baton->conflict_separator, &len));
      SVN_ERR(output_marker_eol(file_baton));

      SVN_ERR(output_hunk(baton, 2, latest_start, latest_length));

      len = strlen(file_baton->conflict_latest);
      SVN_ERR(svn_stream_write(file_baton->output_stream,
                               file_baton->conflict_latest, &len));
      SVN_ERR(output_marker_eol(file_baton));
    }
  else if (style == svn_diff_conflict_display_modified)
    SVN_ERR(output_hunk(baton, 1, modified_start, modified_length));
  else if (style == svn_diff_conflict_display_latest)
    SVN_ERR(output_hunk(baton, 2, latest_start, latest_length));
  else /* unknown style */
    SVN_ERR_MALFUNCTION();

  return SVN_NO_ERROR;
}


/* Return the first eol marker found in [BUF, ENDP) as a
 * NUL-terminated string, or NULL if no eol marker is found.
 *
 * If the last valid character of BUF is the first byte of a
 * potentially two-byte eol sequence, just return "\r", that is,
 * assume BUF represents a CR-only file.  This is correct for callers
 * that pass an entire file at once, and is no more likely to be
 * incorrect than correct for any caller that doesn't.
 */
static const char *
detect_eol(char *buf, char *endp)
{
  const char *eol = find_eol_start(buf, endp - buf);
  if (eol)
    {
      if (*eol == '\n')
        return "\n";

      /* We found a CR. */
      ++eol;
      if (eol == endp || *eol != '\n')
        return "\r";
      return "\r\n";
    }

  return NULL;
}

svn_error_t *
svn_diff_file_output_merge2(svn_stream_t *output_stream,
                            svn_diff_t *diff,
                            const char *original_path,
                            const char *modified_path,
                            const char *latest_path,
                            const char *conflict_original,
                            const char *conflict_modified,
                            const char *conflict_latest,
                            const char *conflict_separator,
                            svn_diff_conflict_display_style_t style,
                            apr_pool_t *pool)
{
  svn_diff3__file_output_baton_t baton;
  apr_file_t *file[3];
  apr_off_t size;
  int idx;
#if APR_HAS_MMAP
  apr_mmap_t *mm[3] = { 0 };
#endif /* APR_HAS_MMAP */
  const char *eol;
  svn_boolean_t conflicts_only =
    (style == svn_diff_conflict_display_only_conflicts);

  memset(&baton, 0, sizeof(baton));
  if (conflicts_only)
    {
      baton.pool = svn_pool_create(pool);
      make_context_saver(&baton);
      baton.real_output_stream = output_stream;
    }
  else
    baton.output_stream = output_stream;
  baton.path[0] = original_path;
  baton.path[1] = modified_path;
  baton.path[2] = latest_path;
  SVN_ERR(svn_utf_cstring_from_utf8(&baton.conflict_modified,
                                    conflict_modified ? conflict_modified
                                    : apr_psprintf(pool, "<<<<<<< %s",
                                                   modified_path),
                                    pool));
  SVN_ERR(svn_utf_cstring_from_utf8(&baton.conflict_original,
                                    conflict_original ? conflict_original
                                    : apr_psprintf(pool, "||||||| %s",
                                                   original_path),
                                    pool));
  SVN_ERR(svn_utf_cstring_from_utf8(&baton.conflict_separator,
                                    conflict_separator ? conflict_separator
                                    : "=======", pool));
  SVN_ERR(svn_utf_cstring_from_utf8(&baton.conflict_latest,
                                    conflict_latest ? conflict_latest
                                    : apr_psprintf(pool, ">>>>>>> %s",
                                                   latest_path),
                                    pool));

  baton.conflict_style = style;

  for (idx = 0; idx < 3; idx++)
    {
      SVN_ERR(map_or_read_file(&file[idx],
                               MMAP_T_ARG(mm[idx])
                               &baton.buffer[idx], &size,
                               baton.path[idx], pool));

      baton.curp[idx] = baton.buffer[idx];
      baton.endp[idx] = baton.buffer[idx];

      if (baton.endp[idx])
        baton.endp[idx] += size;
    }

  /* Check what eol marker we should use for conflict markers.
     We use the eol marker of the modified file and fall back on the
     platform's eol marker if that file doesn't contain any newlines. */
  eol = detect_eol(baton.buffer[1], baton.endp[1]);
  if (! eol)
    eol = APR_EOL_STR;
  baton.marker_eol = eol;

  SVN_ERR(svn_diff_output(diff, &baton,
                          &svn_diff3__file_output_vtable));

  for (idx = 0; idx < 3; idx++)
    {
#if APR_HAS_MMAP
      if (mm[idx])
        {
          apr_status_t rv = apr_mmap_delete(mm[idx]);
          if (rv != APR_SUCCESS)
            {
              return svn_error_wrap_apr(rv, _("Failed to delete mmap '%s'"),
                                        baton.path[idx]);
            }
        }
#endif /* APR_HAS_MMAP */

      if (file[idx])
        {
          SVN_ERR(svn_io_file_close(file[idx], pool));
        }
    }

  if (conflicts_only)
    svn_pool_destroy(baton.pool);

  return SVN_NO_ERROR;
}


svn_error_t *
svn_diff_file_output_merge(svn_stream_t *output_stream,
                           svn_diff_t *diff,
                           const char *original_path,
                           const char *modified_path,
                           const char *latest_path,
                           const char *conflict_original,
                           const char *conflict_modified,
                           const char *conflict_latest,
                           const char *conflict_separator,
                           svn_boolean_t display_original_in_conflict,
                           svn_boolean_t display_resolved_conflicts,
                           apr_pool_t *pool)
{
  svn_diff_conflict_display_style_t style =
    svn_diff_conflict_display_modified_latest;

  if (display_resolved_conflicts)
    style = svn_diff_conflict_display_resolved_modified_latest;

  if (display_original_in_conflict)
    style = svn_diff_conflict_display_modified_original_latest;

  return svn_diff_file_output_merge2(output_stream,
                                     diff,
                                     original_path,
                                     modified_path,
                                     latest_path,
                                     conflict_original,
                                     conflict_modified,
                                     conflict_latest,
                                     conflict_separator,
                                     style,
                                     pool);
}
