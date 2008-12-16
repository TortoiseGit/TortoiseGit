/*
 * token.c :  routines for doing diffs
 *
 * ====================================================================
 * Copyright (c) 2000-2004 CollabNet.  All rights reserved.
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

#include "svn_error.h"
#include "svn_diff.h"
#include "svn_types.h"

#include "diff.h"


/*
 * Prime number to use as the size of the hash table.  This number was
 * not selected by testing of any kind and may need tweaking.
 */
#define SVN_DIFF__HASH_SIZE 127

struct svn_diff__node_t
{
  svn_diff__node_t     *parent;
  svn_diff__node_t     *left;
  svn_diff__node_t     *right;

  apr_uint32_t          hash;
  void                 *token;
};

struct svn_diff__tree_t
{
  svn_diff__node_t     *root[SVN_DIFF__HASH_SIZE];
  apr_pool_t           *pool;
};


/*
 * Support functions to build a tree of token positions
 */

void
svn_diff__tree_create(svn_diff__tree_t **tree, apr_pool_t *pool)
{
  *tree = apr_pcalloc(pool, sizeof(**tree));
  (*tree)->pool = pool;
}


static svn_error_t *
svn_diff__tree_insert_token(svn_diff__node_t **node, svn_diff__tree_t *tree,
                            void *diff_baton,
                            const svn_diff_fns_t *vtable,
                            apr_uint32_t hash, void *token)
{
  svn_diff__node_t *new_node;
  svn_diff__node_t **node_ref;
  svn_diff__node_t *parent;
  int rv;

  SVN_ERR_ASSERT(token);

  parent = NULL;
  node_ref = &tree->root[hash % SVN_DIFF__HASH_SIZE];

  while (*node_ref != NULL)
    {
      parent = *node_ref;

      rv = hash - parent->hash;
      if (!rv)
        SVN_ERR(vtable->token_compare(diff_baton, parent->token, token, &rv));

      if (rv == 0)
        {
          /* Discard the previous token.  This helps in cases where
           * only recently read tokens are still in memory.
           */
          if (vtable->token_discard != NULL)
            vtable->token_discard(diff_baton, parent->token);

          parent->token = token;
          *node = parent;

          return SVN_NO_ERROR;
        }
      else if (rv > 0)
        {
          node_ref = &parent->left;
        }
      else
        {
          node_ref = &parent->right;
        }
    }

  /* Create a new node */
  new_node = apr_palloc(tree->pool, sizeof(*new_node));
  new_node->parent = parent;
  new_node->left = NULL;
  new_node->right = NULL;
  new_node->hash = hash;
  new_node->token = token;

  *node = *node_ref = new_node;

  return SVN_NO_ERROR;
}


/*
 * Get all tokens from a datasource.  Return the
 * last item in the (circular) list.
 */
svn_error_t *
svn_diff__get_tokens(svn_diff__position_t **position_list,
                     svn_diff__tree_t *tree,
                     void *diff_baton,
                     const svn_diff_fns_t *vtable,
                     svn_diff_datasource_e datasource,
                     apr_pool_t *pool)
{
  svn_diff__position_t *start_position;
  svn_diff__position_t *position = NULL;
  svn_diff__position_t **position_ref;
  svn_diff__node_t *node;
  void *token;
  apr_off_t offset;
  apr_uint32_t hash;

  *position_list = NULL;


  SVN_ERR(vtable->datasource_open(diff_baton, datasource));

  position_ref = &start_position;
  offset = 0;
  hash = 0; /* The callback fn doesn't need to touch it per se */
  while (1)
    {
      SVN_ERR(vtable->datasource_get_next_token(&hash, &token,
                                                diff_baton, datasource));
      if (token == NULL)
        break;

      offset++;
      SVN_ERR(svn_diff__tree_insert_token(&node, tree,
                                          diff_baton, vtable,
                                          hash, token));

      /* Create a new position */
      position = apr_palloc(pool, sizeof(*position));
      position->next = NULL;
      position->node = node;
      position->offset = offset;

      *position_ref = position;
      position_ref = &position->next;
    }

  *position_ref = start_position;

  SVN_ERR(vtable->datasource_close(diff_baton, datasource));

  *position_list = position;

  return SVN_NO_ERROR;
}
