/* bobguitreerbtree.c
 * Copyright (C) 2000  Red Hat, Inc.,  Jonathan Blandford <jrb@redhat.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"
#include "bobguitreerbtreeprivate.h"
#include "bobguidebug.h"

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

static BobguiTreeRBNode *bobgui_tree_rbnode_new                (BobguiTreeRBTree *tree,
                                                          int            height);
static void        bobgui_tree_rbnode_free               (BobguiTreeRBNode *node);
static void        bobgui_tree_rbnode_rotate_left        (BobguiTreeRBTree *tree,
                                                       BobguiTreeRBNode *node);
static void        bobgui_tree_rbnode_rotate_right       (BobguiTreeRBTree *tree,
                                                       BobguiTreeRBNode *node);
static void        bobgui_tree_rbtree_insert_fixup       (BobguiTreeRBTree *tree,
                                                       BobguiTreeRBNode *node);
static void        bobgui_tree_rbtree_remove_node_fixup  (BobguiTreeRBTree *tree,
                                                       BobguiTreeRBNode *node,
                                                       BobguiTreeRBNode *parent);
static inline void fixup_validation              (BobguiTreeRBTree *tree,
                                                  BobguiTreeRBNode *node);
static inline void fixup_total_count             (BobguiTreeRBTree *tree,
                                                  BobguiTreeRBNode *node);
static void        bobgui_tree_rbtree_test               (const char    *where,
                                                       BobguiTreeRBTree *tree);
static void        bobgui_tree_rbtree_debug_spew         (BobguiTreeRBTree *tree,
                                                       GString       *s);

static const BobguiTreeRBNode nil =
{
  /* .flags = */ BOBGUI_TREE_RBNODE_BLACK,

  /* rest is NULL */
};

gboolean
bobgui_tree_rbtree_is_nil (BobguiTreeRBNode *node)
{
  return node == &nil;
}

static BobguiTreeRBNode *
bobgui_tree_rbnode_new (BobguiTreeRBTree *tree,
                     int            height)
{
  BobguiTreeRBNode *node = g_slice_new (BobguiTreeRBNode);

  node->left = (BobguiTreeRBNode *) &nil;
  node->right = (BobguiTreeRBNode *) &nil;
  node->parent = (BobguiTreeRBNode *) &nil;
  node->flags = BOBGUI_TREE_RBNODE_RED;
  node->total_count = 1;
  node->count = 1;
  node->children = NULL;
  node->offset = height;
  return node;
}

static void
bobgui_tree_rbnode_free (BobguiTreeRBNode *node)
{
  g_slice_free (BobguiTreeRBNode, node);
}

static void
bobgui_tree_rbnode_rotate_left (BobguiTreeRBTree *tree,
                             BobguiTreeRBNode *node)
{
  int node_height, right_height;
  BobguiTreeRBNode *right;

  g_return_if_fail (!bobgui_tree_rbtree_is_nil (node));
  g_return_if_fail (!bobgui_tree_rbtree_is_nil (node->right));

  right = node->right;

  node_height = BOBGUI_TREE_RBNODE_GET_HEIGHT (node);
  right_height = BOBGUI_TREE_RBNODE_GET_HEIGHT (right);
  node->right = right->left;
  if (!bobgui_tree_rbtree_is_nil (right->left))
    right->left->parent = node;

  right->parent = node->parent;
  if (!bobgui_tree_rbtree_is_nil (node->parent))
    {
      if (node == node->parent->left)
        node->parent->left = right;
      else
        node->parent->right = right;
    }
  else
    {
      tree->root = right;
    }

  right->left = node;
  node->parent = right;

  node->count = 1 + node->left->count + node->right->count;
  right->count = 1 + right->left->count + right->right->count;

  node->offset = node_height + node->left->offset + node->right->offset +
                 (node->children ? node->children->root->offset : 0);
  right->offset = right_height + right->left->offset + right->right->offset +
                  (right->children ? right->children->root->offset : 0);

  fixup_validation (tree, node);
  fixup_validation (tree, right);
  fixup_total_count (tree, node);
  fixup_total_count (tree, right);
}

static void
bobgui_tree_rbnode_rotate_right (BobguiTreeRBTree *tree,
                              BobguiTreeRBNode *node)
{
  int node_height, left_height;
  BobguiTreeRBNode *left;

  g_return_if_fail (!bobgui_tree_rbtree_is_nil (node));
  g_return_if_fail (!bobgui_tree_rbtree_is_nil (node->left));

  left = node->left;

  node_height = BOBGUI_TREE_RBNODE_GET_HEIGHT (node);
  left_height = BOBGUI_TREE_RBNODE_GET_HEIGHT (left);

  node->left = left->right;
  if (!bobgui_tree_rbtree_is_nil (left->right))
    left->right->parent = node;

  left->parent = node->parent;
  if (!bobgui_tree_rbtree_is_nil (node->parent))
    {
      if (node == node->parent->right)
        node->parent->right = left;
      else
        node->parent->left = left;
    }
  else
    {
      tree->root = left;
    }

  /* link node and left */
  left->right = node;
  node->parent = left;

  node->count = 1 + node->left->count + node->right->count;
  left->count = 1 + left->left->count + left->right->count;

  node->offset = node_height + node->left->offset + node->right->offset +
                 (node->children ? node->children->root->offset : 0);
  left->offset = left_height + left->left->offset + left->right->offset +
                 (left->children ? left->children->root->offset : 0);

  fixup_validation (tree, node);
  fixup_validation (tree, left);
  fixup_total_count (tree, node);
  fixup_total_count (tree, left);
}

static void
bobgui_tree_rbtree_insert_fixup (BobguiTreeRBTree *tree,
                              BobguiTreeRBNode *node)
{
  /* check Red-Black properties */
  while (node != tree->root && BOBGUI_TREE_RBNODE_GET_COLOR (node->parent) == BOBGUI_TREE_RBNODE_RED)
    {
      /* we have a violation */
      if (node->parent == node->parent->parent->left)
        {
          BobguiTreeRBNode *y = node->parent->parent->right;
          if (BOBGUI_TREE_RBNODE_GET_COLOR (y) == BOBGUI_TREE_RBNODE_RED)
            {
              /* uncle is BOBGUI_TREE_RBNODE_RED */
              BOBGUI_TREE_RBNODE_SET_COLOR (node->parent, BOBGUI_TREE_RBNODE_BLACK);
              BOBGUI_TREE_RBNODE_SET_COLOR (y, BOBGUI_TREE_RBNODE_BLACK);
              BOBGUI_TREE_RBNODE_SET_COLOR (node->parent->parent, BOBGUI_TREE_RBNODE_RED);
              node = node->parent->parent;
            }
          else
            {
              /* uncle is BOBGUI_TREE_RBNODE_BLACK */
              if (node == node->parent->right)
                {
                  /* make node a left child */
                  node = node->parent;
                  bobgui_tree_rbnode_rotate_left (tree, node);
                }

              /* recolor and rotate */
              BOBGUI_TREE_RBNODE_SET_COLOR (node->parent, BOBGUI_TREE_RBNODE_BLACK);
              BOBGUI_TREE_RBNODE_SET_COLOR (node->parent->parent, BOBGUI_TREE_RBNODE_RED);
              bobgui_tree_rbnode_rotate_right (tree, node->parent->parent);
            }
        }
      else
        {
          /* mirror image of above code */
          BobguiTreeRBNode *y = node->parent->parent->left;
          if (BOBGUI_TREE_RBNODE_GET_COLOR (y) == BOBGUI_TREE_RBNODE_RED)
            {
              /* uncle is BOBGUI_TREE_RBNODE_RED */
              BOBGUI_TREE_RBNODE_SET_COLOR (node->parent, BOBGUI_TREE_RBNODE_BLACK);
              BOBGUI_TREE_RBNODE_SET_COLOR (y, BOBGUI_TREE_RBNODE_BLACK);
              BOBGUI_TREE_RBNODE_SET_COLOR (node->parent->parent, BOBGUI_TREE_RBNODE_RED);
              node = node->parent->parent;
            }
          else
            {
              /* uncle is BOBGUI_TREE_RBNODE_BLACK */
              if (node == node->parent->left)
                {
                  node = node->parent;
                  bobgui_tree_rbnode_rotate_right (tree, node);
                }
              BOBGUI_TREE_RBNODE_SET_COLOR (node->parent, BOBGUI_TREE_RBNODE_BLACK);
              BOBGUI_TREE_RBNODE_SET_COLOR (node->parent->parent, BOBGUI_TREE_RBNODE_RED);
              bobgui_tree_rbnode_rotate_left (tree, node->parent->parent);
            }
        }
    }
  BOBGUI_TREE_RBNODE_SET_COLOR (tree->root, BOBGUI_TREE_RBNODE_BLACK);
}

static void
bobgui_tree_rbtree_remove_node_fixup (BobguiTreeRBTree *tree,
                                   BobguiTreeRBNode *node,
                                   BobguiTreeRBNode *parent)
{
  while (node != tree->root && BOBGUI_TREE_RBNODE_GET_COLOR (node) == BOBGUI_TREE_RBNODE_BLACK)
    {
      if (node == parent->left)
        {
          BobguiTreeRBNode *w = parent->right;
          if (BOBGUI_TREE_RBNODE_GET_COLOR (w) == BOBGUI_TREE_RBNODE_RED)
            {
              BOBGUI_TREE_RBNODE_SET_COLOR (w, BOBGUI_TREE_RBNODE_BLACK);
              BOBGUI_TREE_RBNODE_SET_COLOR (parent, BOBGUI_TREE_RBNODE_RED);
              bobgui_tree_rbnode_rotate_left (tree, parent);
              w = parent->right;
            }
          g_assert (w);
          if (BOBGUI_TREE_RBNODE_GET_COLOR (w->left) == BOBGUI_TREE_RBNODE_BLACK && BOBGUI_TREE_RBNODE_GET_COLOR (w->right) == BOBGUI_TREE_RBNODE_BLACK)
            {
              BOBGUI_TREE_RBNODE_SET_COLOR (w, BOBGUI_TREE_RBNODE_RED);
              node = parent;
            }
          else
            {
              if (BOBGUI_TREE_RBNODE_GET_COLOR (w->right) == BOBGUI_TREE_RBNODE_BLACK)
                {
                  BOBGUI_TREE_RBNODE_SET_COLOR (w->left, BOBGUI_TREE_RBNODE_BLACK);
                  BOBGUI_TREE_RBNODE_SET_COLOR (w, BOBGUI_TREE_RBNODE_RED);
                  bobgui_tree_rbnode_rotate_right (tree, w);
                  w = parent->right;
                }
              BOBGUI_TREE_RBNODE_SET_COLOR (w, BOBGUI_TREE_RBNODE_GET_COLOR (parent));
              BOBGUI_TREE_RBNODE_SET_COLOR (parent, BOBGUI_TREE_RBNODE_BLACK);
              BOBGUI_TREE_RBNODE_SET_COLOR (w->right, BOBGUI_TREE_RBNODE_BLACK);
              bobgui_tree_rbnode_rotate_left (tree, parent);
              node = tree->root;
            }
        }
      else
        {
          BobguiTreeRBNode *w = parent->left;
          if (BOBGUI_TREE_RBNODE_GET_COLOR (w) == BOBGUI_TREE_RBNODE_RED)
            {
              BOBGUI_TREE_RBNODE_SET_COLOR (w, BOBGUI_TREE_RBNODE_BLACK);
              BOBGUI_TREE_RBNODE_SET_COLOR (parent, BOBGUI_TREE_RBNODE_RED);
              bobgui_tree_rbnode_rotate_right (tree, parent);
              w = parent->left;
            }
          g_assert (w);
          if (BOBGUI_TREE_RBNODE_GET_COLOR (w->right) == BOBGUI_TREE_RBNODE_BLACK && BOBGUI_TREE_RBNODE_GET_COLOR (w->left) == BOBGUI_TREE_RBNODE_BLACK)
            {
              BOBGUI_TREE_RBNODE_SET_COLOR (w, BOBGUI_TREE_RBNODE_RED);
              node = parent;
            }
          else
            {
              if (BOBGUI_TREE_RBNODE_GET_COLOR (w->left) == BOBGUI_TREE_RBNODE_BLACK)
                {
                  BOBGUI_TREE_RBNODE_SET_COLOR (w->right, BOBGUI_TREE_RBNODE_BLACK);
                  BOBGUI_TREE_RBNODE_SET_COLOR (w, BOBGUI_TREE_RBNODE_RED);
                  bobgui_tree_rbnode_rotate_left (tree, w);
                  w = parent->left;
                }
              BOBGUI_TREE_RBNODE_SET_COLOR (w, BOBGUI_TREE_RBNODE_GET_COLOR (parent));
              BOBGUI_TREE_RBNODE_SET_COLOR (parent, BOBGUI_TREE_RBNODE_BLACK);
              BOBGUI_TREE_RBNODE_SET_COLOR (w->left, BOBGUI_TREE_RBNODE_BLACK);
              bobgui_tree_rbnode_rotate_right (tree, parent);
              node = tree->root;
            }
        }

      parent = node->parent;
    }
  BOBGUI_TREE_RBNODE_SET_COLOR (node, BOBGUI_TREE_RBNODE_BLACK);
}

BobguiTreeRBTree *
bobgui_tree_rbtree_new (void)
{
  BobguiTreeRBTree *retval;

  retval = g_new (BobguiTreeRBTree, 1);
  retval->parent_tree = NULL;
  retval->parent_node = NULL;

  retval->root = (BobguiTreeRBNode *) &nil;

  return retval;
}

static void
bobgui_tree_rbtree_free_helper (BobguiTreeRBTree *tree,
                             BobguiTreeRBNode *node,
                             gpointer       data)
{
  if (node->children)
    bobgui_tree_rbtree_free (node->children);

  bobgui_tree_rbnode_free (node);
}

void
bobgui_tree_rbtree_free (BobguiTreeRBTree *tree)
{
  bobgui_tree_rbtree_traverse (tree,
                            tree->root,
                            G_POST_ORDER,
                            bobgui_tree_rbtree_free_helper,
                            NULL);

  if (tree->parent_node &&
      tree->parent_node->children == tree)
    tree->parent_node->children = NULL;
  g_free (tree);
}

static void
bobgui_rbnode_adjust (BobguiTreeRBTree *tree,
                   BobguiTreeRBNode *node,
                   int            count_diff,
                   int            total_count_diff,
                   int            offset_diff)
{
  while (tree && node && !bobgui_tree_rbtree_is_nil (node))
    {
      fixup_validation (tree, node);
      node->offset += offset_diff;
      node->count += count_diff;
      node->total_count += total_count_diff;

      node = node->parent;
      if (bobgui_tree_rbtree_is_nil (node))
        {
          node = tree->parent_node;
          tree = tree->parent_tree;
          count_diff = 0;
        }
    }
}

void
bobgui_tree_rbtree_remove (BobguiTreeRBTree *tree)
{
  BobguiTreeRBTree *tmp_tree;

  if (BOBGUI_DEBUG_CHECK (TREE))
    bobgui_tree_rbtree_test (G_STRLOC, tree);

  /* ugly hack to make fixup_validation work in the first iteration of the
   * loop below */
  BOBGUI_TREE_RBNODE_UNSET_FLAG (tree->root, BOBGUI_TREE_RBNODE_DESCENDANTS_INVALID);

  bobgui_rbnode_adjust (tree->parent_tree,
                     tree->parent_node,
                     0,
                     -(int) tree->root->total_count,
                     -tree->root->offset);

  tmp_tree = tree->parent_tree;

  bobgui_tree_rbtree_free (tree);

  if (BOBGUI_DEBUG_CHECK (TREE))
    bobgui_tree_rbtree_test (G_STRLOC, tmp_tree);
}


BobguiTreeRBNode *
bobgui_tree_rbtree_insert_after (BobguiTreeRBTree *tree,
                              BobguiTreeRBNode *current,
                              int            height,
                              gboolean       valid)
{
  BobguiTreeRBNode *node;
  gboolean right = TRUE;

  if (BOBGUI_DEBUG_CHECK (TREE))
    {
      GString *s;

      s = g_string_new ("");
      g_string_append_printf (s, "bobgui_tree_rbtree_insert_after: %p\n", current);
      bobgui_tree_rbtree_debug_spew (tree, s);
      g_message ("%s", s->str);
      g_string_free (s, TRUE);
      bobgui_tree_rbtree_test (G_STRLOC, tree);
    }

  if (current != NULL && !bobgui_tree_rbtree_is_nil (current->right))
    {
      current = current->right;
      while (!bobgui_tree_rbtree_is_nil (current->left))
        current = current->left;
      right = FALSE;
    }
  /* setup new node */
  node = bobgui_tree_rbnode_new (tree, height);

  /* insert node in tree */
  if (current)
    {
      node->parent = current;
      if (right)
        current->right = node;
      else
        current->left = node;
      bobgui_rbnode_adjust (tree, node->parent,
                         1, 1, height);
    }
  else
    {
      g_assert (bobgui_tree_rbtree_is_nil (tree->root));
      tree->root = node;
      bobgui_rbnode_adjust (tree->parent_tree, tree->parent_node,
                         0, 1, height);
    }

  if (valid)
    bobgui_tree_rbtree_node_mark_valid (tree, node);
  else
    bobgui_tree_rbtree_node_mark_invalid (tree, node);

  bobgui_tree_rbtree_insert_fixup (tree, node);

  if (BOBGUI_DEBUG_CHECK (TREE))
    {
      GString *s;

      s = g_string_new ("bobgui_tree_rbtree_insert_after finished...\n");
      bobgui_tree_rbtree_debug_spew (tree, s);
      g_message ("%s", s->str);
      g_string_free (s, TRUE);
      bobgui_tree_rbtree_test (G_STRLOC, tree);
    }

  return node;
}

BobguiTreeRBNode *
bobgui_tree_rbtree_insert_before (BobguiTreeRBTree *tree,
                               BobguiTreeRBNode *current,
                               int            height,
                               gboolean       valid)
{
  BobguiTreeRBNode *node;
  gboolean left = TRUE;

  if (BOBGUI_DEBUG_CHECK (TREE))
    {
      GString *s;

      s = g_string_new ("");
      g_string_append_printf (s, "bobgui_tree_rbtree_insert_before: %p\n", current);
      bobgui_tree_rbtree_debug_spew (tree, s);
      g_message ("%s", s->str);
      g_string_free (s, TRUE);
      bobgui_tree_rbtree_test (G_STRLOC, tree);
    }

  if (current != NULL && !bobgui_tree_rbtree_is_nil (current->left))
    {
      current = current->left;
      while (!bobgui_tree_rbtree_is_nil (current->right))
        current = current->right;
      left = FALSE;
    }

  /* setup new node */
  node = bobgui_tree_rbnode_new (tree, height);

  /* insert node in tree */
  if (current)
    {
      node->parent = current;
      if (left)
        current->left = node;
      else
        current->right = node;
      bobgui_rbnode_adjust (tree, node->parent,
                         1, 1, height);
    }
  else
    {
      g_assert (bobgui_tree_rbtree_is_nil (tree->root));
      tree->root = node;
      bobgui_rbnode_adjust (tree->parent_tree, tree->parent_node,
                         0, 1, height);
    }

  if (valid)
    bobgui_tree_rbtree_node_mark_valid (tree, node);
  else
    bobgui_tree_rbtree_node_mark_invalid (tree, node);

  bobgui_tree_rbtree_insert_fixup (tree, node);

  if (BOBGUI_DEBUG_CHECK (TREE))
    {
      GString *s;

      s = g_string_new ("bobgui_tree_rbtree_insert_before finished...\n");
      bobgui_tree_rbtree_debug_spew (tree, s);
      g_message ("%s", s->str);
      g_string_free (s, TRUE);
      bobgui_tree_rbtree_test (G_STRLOC, tree);
    }

  return node;
}

BobguiTreeRBNode *
bobgui_tree_rbtree_find_count (BobguiTreeRBTree *tree,
                            int            count)
{
  BobguiTreeRBNode *node;

  node = tree->root;
  while (!bobgui_tree_rbtree_is_nil (node) && (node->left->count + 1 != count))
    {
      if (node->left->count >= count)
        node = node->left;
      else
        {
          count -= (node->left->count + 1);
          node = node->right;
        }
    }
  if (bobgui_tree_rbtree_is_nil (node))
    return NULL;
  return node;
}

void
bobgui_tree_rbtree_node_set_height (BobguiTreeRBTree *tree,
                                 BobguiTreeRBNode *node,
                                 int            height)
{
  int diff = height - BOBGUI_TREE_RBNODE_GET_HEIGHT (node);

  if (diff == 0)
    return;

  bobgui_rbnode_adjust (tree, node, 0, 0, diff);

  if (BOBGUI_DEBUG_CHECK (TREE))
    bobgui_tree_rbtree_test (G_STRLOC, tree);
}

void
bobgui_tree_rbtree_node_mark_invalid (BobguiTreeRBTree *tree,
                                   BobguiTreeRBNode *node)
{
  if (BOBGUI_TREE_RBNODE_FLAG_SET (node, BOBGUI_TREE_RBNODE_INVALID))
    return;

  BOBGUI_TREE_RBNODE_SET_FLAG (node, BOBGUI_TREE_RBNODE_INVALID);
  do
    {
      if (BOBGUI_TREE_RBNODE_FLAG_SET (node, BOBGUI_TREE_RBNODE_DESCENDANTS_INVALID))
        return;
      BOBGUI_TREE_RBNODE_SET_FLAG (node, BOBGUI_TREE_RBNODE_DESCENDANTS_INVALID);
      node = node->parent;
      if (bobgui_tree_rbtree_is_nil (node))
        {
          node = tree->parent_node;
          tree = tree->parent_tree;
        }
    }
  while (node);
}

#if 0
/* Draconian version. */
void
bobgui_tree_rbtree_node_mark_invalid (BobguiTreeRBTree *tree,
                                   BobguiTreeRBNode *node)
{
  BOBGUI_TREE_RBNODE_SET_FLAG (node, BOBGUI_TREE_RBNODE_INVALID);
  do
    {
      fixup_validation (tree, node);
      node = node->parent;
      if (bobgui_tree_rbtree_is_nil (node))
        {
          node = tree->parent_node;
          tree = tree->parent_tree;
        }
    }
  while (node);
}
#endif

void
bobgui_tree_rbtree_node_mark_valid (BobguiTreeRBTree *tree,
                                 BobguiTreeRBNode *node)
{
  if ((!BOBGUI_TREE_RBNODE_FLAG_SET (node, BOBGUI_TREE_RBNODE_INVALID)) &&
      (!BOBGUI_TREE_RBNODE_FLAG_SET (node, BOBGUI_TREE_RBNODE_COLUMN_INVALID)))
    return;

  BOBGUI_TREE_RBNODE_UNSET_FLAG (node, BOBGUI_TREE_RBNODE_INVALID);
  BOBGUI_TREE_RBNODE_UNSET_FLAG (node, BOBGUI_TREE_RBNODE_COLUMN_INVALID);

  do
    {
      if ((BOBGUI_TREE_RBNODE_FLAG_SET (node, BOBGUI_TREE_RBNODE_INVALID)) ||
          (BOBGUI_TREE_RBNODE_FLAG_SET (node, BOBGUI_TREE_RBNODE_COLUMN_INVALID)) ||
          (node->children && BOBGUI_TREE_RBNODE_FLAG_SET (node->children->root, BOBGUI_TREE_RBNODE_DESCENDANTS_INVALID)) ||
          (BOBGUI_TREE_RBNODE_FLAG_SET (node->left, BOBGUI_TREE_RBNODE_DESCENDANTS_INVALID)) ||
          (BOBGUI_TREE_RBNODE_FLAG_SET (node->right, BOBGUI_TREE_RBNODE_DESCENDANTS_INVALID)))
        return;

      BOBGUI_TREE_RBNODE_UNSET_FLAG (node, BOBGUI_TREE_RBNODE_DESCENDANTS_INVALID);
      node = node->parent;
      if (bobgui_tree_rbtree_is_nil (node))
        {
          node = tree->parent_node;
          tree = tree->parent_tree;
        }
    }
  while (node);
}

#if 0
/* Draconian version */
void
bobgui_tree_rbtree_node_mark_valid (BobguiTreeRBTree *tree,
                                 BobguiTreeRBNode *node)
{
  BOBGUI_TREE_RBNODE_UNSET_FLAG (node, BOBGUI_TREE_RBNODE_INVALID);
  BOBGUI_TREE_RBNODE_UNSET_FLAG (node, BOBGUI_TREE_RBNODE_COLUMN_INVALID);

  do
    {
      fixup_validation (tree, node);
      node = node->parent;
      if (bobgui_tree_rbtree_is_nil (node))
        {
          node = tree->parent_node;
          tree = tree->parent_tree;
        }
    }
  while (node);
}
#endif
/* Assume tree is the root node as it doesn't set DESCENDANTS_INVALID above.
 */
void
bobgui_tree_rbtree_column_invalid (BobguiTreeRBTree *tree)
{
  BobguiTreeRBNode *node;

  if (tree == NULL)
    return;

  for (node = bobgui_tree_rbtree_first (tree);
       node != NULL;
       node = bobgui_tree_rbtree_next (tree, node))
    {
      if (!(BOBGUI_TREE_RBNODE_FLAG_SET (node, BOBGUI_TREE_RBNODE_INVALID)))
        BOBGUI_TREE_RBNODE_SET_FLAG (node, BOBGUI_TREE_RBNODE_COLUMN_INVALID);
      BOBGUI_TREE_RBNODE_SET_FLAG (node, BOBGUI_TREE_RBNODE_DESCENDANTS_INVALID);

      if (node->children)
        bobgui_tree_rbtree_column_invalid (node->children);
    }
}

void
bobgui_tree_rbtree_mark_invalid (BobguiTreeRBTree *tree)
{
  BobguiTreeRBNode *node;

  if (tree == NULL)
    return;

  for (node = bobgui_tree_rbtree_first (tree);
       node != NULL;
       node = bobgui_tree_rbtree_next (tree, node))
    {
      BOBGUI_TREE_RBNODE_SET_FLAG (node, BOBGUI_TREE_RBNODE_INVALID);
      BOBGUI_TREE_RBNODE_SET_FLAG (node, BOBGUI_TREE_RBNODE_DESCENDANTS_INVALID);

      if (node->children)
        bobgui_tree_rbtree_mark_invalid (node->children);
    }
}

void
bobgui_tree_rbtree_set_fixed_height (BobguiTreeRBTree *tree,
                                  int            height,
                                  gboolean       mark_valid)
{
  BobguiTreeRBNode *node;

  if (tree == NULL)
    return;

  for (node = bobgui_tree_rbtree_first (tree);
       node != NULL;
       node = bobgui_tree_rbtree_next (tree, node))
    {
      if (BOBGUI_TREE_RBNODE_FLAG_SET (node, BOBGUI_TREE_RBNODE_INVALID))
        {
          bobgui_tree_rbtree_node_set_height (tree, node, height);
          if (mark_valid)
            bobgui_tree_rbtree_node_mark_valid (tree, node);
        }

      if (node->children)
        bobgui_tree_rbtree_set_fixed_height (node->children, height, mark_valid);
    }
}

static void
reorder_prepare (BobguiTreeRBTree *tree,
                 BobguiTreeRBNode *node,
                 gpointer       data)
{
  node->offset -= node->left->offset + node->right->offset;
  BOBGUI_TREE_RBNODE_UNSET_FLAG (node, BOBGUI_TREE_RBNODE_DESCENDANTS_INVALID);
}

static void
reorder_fixup (BobguiTreeRBTree *tree,
               BobguiTreeRBNode *node,
               gpointer       data)
{
  node->offset += node->left->offset + node->right->offset;
  node->count = 1 + node->left->count + node->right->count;
  fixup_validation (tree, node);
  fixup_total_count (tree, node);
}

static void
reorder_copy_node (BobguiTreeRBTree *tree,
                   BobguiTreeRBNode *to,
                   BobguiTreeRBNode *from)
{
  to->flags = (to->flags & BOBGUI_TREE_RBNODE_NON_COLORS) | BOBGUI_TREE_RBNODE_GET_COLOR (from);

  to->left = from->left;
  if (!bobgui_tree_rbtree_is_nil (to->left))
    to->left->parent = to;

  to->right = from->right;
  if (!bobgui_tree_rbtree_is_nil (to->right))
    to->right->parent = to;

  to->parent = from->parent;
  if (bobgui_tree_rbtree_is_nil (to->parent))
    tree->root = to;
  else if (to->parent->left == from)
    to->parent->left = to;
  else if (to->parent->right == from)
    to->parent->right = to;
}

/* It basically pulls everything out of the tree, rearranges it, and puts it
 * back together.  Our strategy is to keep the old RBTree intact, and just
 * rearrange the contents.  When that is done, we go through and update the
 * heights.  There is probably a more elegant way to write this function.  If
 * anyone wants to spend the time writing it, patches will be accepted.
 */
void
bobgui_tree_rbtree_reorder (BobguiTreeRBTree *tree,
                         int           *new_order,
                         int            length)
{
  BobguiTreeRBNode **nodes;
  BobguiTreeRBNode *node;
  int i, j;

  g_return_if_fail (tree != NULL);
  g_return_if_fail (length > 0);
  g_return_if_fail (tree->root->count == length);

  nodes = g_new (BobguiTreeRBNode *, length);

  bobgui_tree_rbtree_traverse (tree, tree->root, G_PRE_ORDER, reorder_prepare, NULL);

  for (node = bobgui_tree_rbtree_first (tree), i = 0;
       node;
       node = bobgui_tree_rbtree_next (tree, node), i++)
    {
      nodes[i] = node;
    }

  for (i = 0; i < length; i++)
    {
      BobguiTreeRBNode tmp = { 0, };
      GSList *l, *cycle = NULL;

      tmp.offset = -1;

      /* already swapped */
      if (nodes[i] == NULL)
        continue;
      /* no need to swap */
      if (new_order[i] == i)
        continue;

      /* make a list out of the pending nodes */
      for (j = i; new_order[j] != i; j = new_order[j])
        {
          cycle = g_slist_prepend (cycle, nodes[j]);
          nodes[j] = NULL;
        }

      node = nodes[j];
      reorder_copy_node (tree, &tmp, node);
      for (l = cycle; l; l = l->next)
        {
          reorder_copy_node (tree, node, l->data);
          node = l->data;
        }

      reorder_copy_node (tree, node, &tmp);
      nodes[j] = NULL;
      g_slist_free (cycle);
    }

  bobgui_tree_rbtree_traverse (tree, tree->root, G_POST_ORDER, reorder_fixup, NULL);

  g_free (nodes);
}

/**
 * bobgui_tree_rbtree_contains:
 * @tree: a tree
 * @potential_child: a potential child of @tree
 *
 * Checks if @potential_child is a child (direct or via intermediate
 * trees) of @tree.
 *
 * Returns: %TRUE if @potential_child is a child of @tree.
 **/
gboolean
bobgui_tree_rbtree_contains (BobguiTreeRBTree *tree,
                          BobguiTreeRBTree *potential_child)
{
  g_return_val_if_fail (tree != NULL, FALSE);
  g_return_val_if_fail (potential_child != NULL, FALSE);

  do
    {
      potential_child = potential_child->parent_tree;
      if (potential_child == tree)
        return TRUE;
    }
  while (potential_child != NULL);

  return FALSE;
}

int
bobgui_tree_rbtree_node_find_offset (BobguiTreeRBTree *tree,
                                  BobguiTreeRBNode *node)
{
  BobguiTreeRBNode *last;
  int retval;

  g_assert (node);
  g_assert (node->left);

  retval = node->left->offset;

  while (tree && node && !bobgui_tree_rbtree_is_nil (node))
    {
      last = node;
      node = node->parent;

      /* Add left branch, plus children, iff we came from the right */
      if (node->right == last)
        retval += node->offset - node->right->offset;

      if (bobgui_tree_rbtree_is_nil (node))
        {
          node = tree->parent_node;
          tree = tree->parent_tree;

          /* Add the parent node, plus the left branch. */
          if (node)
            retval += node->left->offset + BOBGUI_TREE_RBNODE_GET_HEIGHT (node);
        }
    }
  return retval;
}

guint
bobgui_tree_rbtree_node_get_index (BobguiTreeRBTree *tree,
                                BobguiTreeRBNode *node)
{
  BobguiTreeRBNode *last;
  guint retval;

  g_assert (node);
  g_assert (node->left);

  retval = node->left->total_count;

  while (tree && node && !bobgui_tree_rbtree_is_nil (node))
    {
      last = node;
      node = node->parent;

      /* Add left branch, plus children, iff we came from the right */
      if (node->right == last)
        retval += node->total_count - node->right->total_count;

      if (bobgui_tree_rbtree_is_nil (node))
        {
          node = tree->parent_node;
          tree = tree->parent_tree;

          /* Add the parent node, plus the left branch. */
          if (node)
            retval += node->left->total_count + 1; /* 1 == BOBGUI_TREE_RBNODE_GET_PARITY() */
        }
    }

  return retval;
}

static int
bobgui_rbtree_real_find_offset (BobguiTreeRBTree  *tree,
                             int             height,
                             BobguiTreeRBTree **new_tree,
                             BobguiTreeRBNode **new_node)
{
  BobguiTreeRBNode *tmp_node;

  g_assert (tree);

  if (height < 0)
    {
      *new_tree = NULL;
      *new_node = NULL;

      return 0;
    }


  tmp_node = tree->root;
  while (!bobgui_tree_rbtree_is_nil (tmp_node) &&
         (tmp_node->left->offset > height ||
          (tmp_node->offset - tmp_node->right->offset) < height))
    {
      if (tmp_node->left->offset > height)
        tmp_node = tmp_node->left;
      else
        {
          height -= (tmp_node->offset - tmp_node->right->offset);
          tmp_node = tmp_node->right;
        }
    }
  if (bobgui_tree_rbtree_is_nil (tmp_node))
    {
      *new_tree = NULL;
      *new_node = NULL;
      return 0;
    }
  if (tmp_node->children)
    {
      if ((tmp_node->offset -
           tmp_node->right->offset -
           tmp_node->children->root->offset) > height)
        {
          *new_tree = tree;
          *new_node = tmp_node;
          return (height - tmp_node->left->offset);
        }
      return bobgui_rbtree_real_find_offset (tmp_node->children,
                                          height - tmp_node->left->offset -
                                          (tmp_node->offset -
                                           tmp_node->left->offset -
                                           tmp_node->right->offset -
                                           tmp_node->children->root->offset),
                                          new_tree,
                                          new_node);
    }
  *new_tree = tree;
  *new_node = tmp_node;
  return (height - tmp_node->left->offset);
}

int
bobgui_tree_rbtree_find_offset (BobguiTreeRBTree  *tree,
                             int             height,
                             BobguiTreeRBTree **new_tree,
                             BobguiTreeRBNode **new_node)
{
  g_assert (tree);

  if ((height < 0) ||
      (height >= tree->root->offset))
    {
      *new_tree = NULL;
      *new_node = NULL;

      return 0;
    }
  return bobgui_rbtree_real_find_offset (tree, height, new_tree, new_node);
}

gboolean
bobgui_tree_rbtree_find_index (BobguiTreeRBTree  *tree,
                            guint           index,
                            BobguiTreeRBTree **new_tree,
                            BobguiTreeRBNode **new_node)
{
  BobguiTreeRBNode *tmp_node;

  g_assert (tree);

  tmp_node = tree->root;
  while (!bobgui_tree_rbtree_is_nil (tmp_node))
    {
      if (tmp_node->left->total_count > index)
        {
          tmp_node = tmp_node->left;
        }
      else if (tmp_node->total_count - tmp_node->right->total_count <= index)
        {
          index -= tmp_node->total_count - tmp_node->right->total_count;
          tmp_node = tmp_node->right;
        }
      else
        {
          index -= tmp_node->left->total_count;
          break;
        }
    }
  if (bobgui_tree_rbtree_is_nil (tmp_node))
    {
      *new_tree = NULL;
      *new_node = NULL;
      return FALSE;
    }

  if (index > 0)
    {
      g_assert (tmp_node->children);

      return bobgui_tree_rbtree_find_index (tmp_node->children,
                                         index - 1,
                                         new_tree,
                                         new_node);
    }

  *new_tree = tree;
  *new_node = tmp_node;
  return TRUE;
}

void
bobgui_tree_rbtree_remove_node (BobguiTreeRBTree *tree,
                             BobguiTreeRBNode *node)
{
  BobguiTreeRBNode *x, *y;
  int y_height;
  guint y_total_count;

  g_return_if_fail (tree != NULL);
  g_return_if_fail (node != NULL);


  if (BOBGUI_DEBUG_CHECK (TREE))
    {
      GString *s;

      s = g_string_new ("");
      g_string_append_printf (s, "bobgui_tree_rbtree_remove_node: %p\n", node);
      bobgui_tree_rbtree_debug_spew (tree, s);
      g_message ("%s", s->str);
      g_string_free (s, TRUE);
      bobgui_tree_rbtree_test (G_STRLOC, tree);
    }

  /* make sure we're deleting a node that's actually in the tree */
  for (x = node; !bobgui_tree_rbtree_is_nil (x->parent); x = x->parent)
    ;
  g_return_if_fail (x == tree->root);

  if (BOBGUI_DEBUG_CHECK (TREE))
    bobgui_tree_rbtree_test (G_STRLOC, tree);

  if (bobgui_tree_rbtree_is_nil (node->left) ||
      bobgui_tree_rbtree_is_nil (node->right))
    {
      y = node;
    }
  else
    {
      y = node->right;

      while (!bobgui_tree_rbtree_is_nil (y->left))
        y = y->left;
    }

  y_height = BOBGUI_TREE_RBNODE_GET_HEIGHT (y)
             + (y->children ? y->children->root->offset : 0);
  y_total_count = 1 + (y->children ? y->children->root->total_count : 0);

  /* x is y's only child, or nil */
  if (!bobgui_tree_rbtree_is_nil (y->left))
    x = y->left;
  else
    x = y->right;

  /* remove y from the parent chain */
  if (!bobgui_tree_rbtree_is_nil (x))
    x->parent = y->parent;
  if (!bobgui_tree_rbtree_is_nil (y->parent))
    {
      if (y == y->parent->left)
        y->parent->left = x;
      else
        y->parent->right = x;
    }
  else
    {
      tree->root = x;
    }

  /* We need to clean up the validity of the tree.
   */
  bobgui_rbnode_adjust (tree, y, -1, -(int) y_total_count, -y_height);

  if (BOBGUI_TREE_RBNODE_GET_COLOR (y) == BOBGUI_TREE_RBNODE_BLACK)
    bobgui_tree_rbtree_remove_node_fixup (tree, x, y->parent);

  if (y != node)
    {
      int node_height, node_total_count;

      /* We want to see how much we remove from the aggregate values.
       * This is all the children we remove plus the node's values.
       */
      node_height = BOBGUI_TREE_RBNODE_GET_HEIGHT (node)
                    + (node->children ? node->children->root->offset : 0);
      node_total_count = 1
                         + (node->children ? node->children->root->total_count : 0);

      /* Move the node over */
      if (BOBGUI_TREE_RBNODE_GET_COLOR (node) != BOBGUI_TREE_RBNODE_GET_COLOR (y))
        y->flags ^= (BOBGUI_TREE_RBNODE_BLACK | BOBGUI_TREE_RBNODE_RED);

      y->left = node->left;
      if (!bobgui_tree_rbtree_is_nil (y->left))
        y->left->parent = y;
      y->right = node->right;
      if (!bobgui_tree_rbtree_is_nil (y->right))
        y->right->parent = y;
      y->parent = node->parent;
      if (!bobgui_tree_rbtree_is_nil (y->parent))
        {
          if (y->parent->left == node)
            y->parent->left = y;
          else
            y->parent->right = y;
        }
      else
        {
          tree->root = y;
        }
      y->count = node->count;
      y->total_count = node->total_count;
      y->offset = node->offset;

      bobgui_rbnode_adjust (tree, y,
                         0,
                         y_total_count - node_total_count,
                         y_height - node_height);
    }

  bobgui_tree_rbnode_free (node);

  if (BOBGUI_DEBUG_CHECK (TREE))
    {
      GString *s;

      s = g_string_new ("bobgui_tree_rbtree_remove_node finished...\n");
      bobgui_tree_rbtree_debug_spew (tree, s);
      g_message ("%s", s->str);
      g_string_free (s, TRUE);
      bobgui_tree_rbtree_test (G_STRLOC, tree);
    }
}

BobguiTreeRBNode *
bobgui_tree_rbtree_first (BobguiTreeRBTree *tree)
{
  BobguiTreeRBNode *node;

  node = tree->root;

  if (bobgui_tree_rbtree_is_nil (node))
    return NULL;

  while (!bobgui_tree_rbtree_is_nil (node->left))
    node = node->left;

  return node;
}

BobguiTreeRBNode *
bobgui_tree_rbtree_next (BobguiTreeRBTree *tree,
                      BobguiTreeRBNode *node)
{
  g_return_val_if_fail (tree != NULL, NULL);
  g_return_val_if_fail (node != NULL, NULL);

  /* Case 1: the node's below us. */
  if (!bobgui_tree_rbtree_is_nil (node->right))
    {
      node = node->right;
      while (!bobgui_tree_rbtree_is_nil (node->left))
        node = node->left;
      return node;
    }

  /* Case 2: it's an ancestor */
  while (!bobgui_tree_rbtree_is_nil (node->parent))
    {
      if (node->parent->right == node)
        node = node->parent;
      else
        return (node->parent);
    }

  /* Case 3: There is no next node */
  return NULL;
}

BobguiTreeRBNode *
bobgui_tree_rbtree_prev (BobguiTreeRBTree *tree,
                      BobguiTreeRBNode *node)
{
  g_return_val_if_fail (tree != NULL, NULL);
  g_return_val_if_fail (node != NULL, NULL);

  /* Case 1: the node's below us. */
  if (!bobgui_tree_rbtree_is_nil (node->left))
    {
      node = node->left;
      while (!bobgui_tree_rbtree_is_nil (node->right))
        node = node->right;
      return node;
    }

  /* Case 2: it's an ancestor */
  while (!bobgui_tree_rbtree_is_nil (node->parent))
    {
      if (node->parent->left == node)
        node = node->parent;
      else
        return (node->parent);
    }

  /* Case 3: There is no next node */
  return NULL;
}

void
bobgui_tree_rbtree_next_full (BobguiTreeRBTree  *tree,
                           BobguiTreeRBNode  *node,
                           BobguiTreeRBTree **new_tree,
                           BobguiTreeRBNode **new_node)
{
  g_return_if_fail (tree != NULL);
  g_return_if_fail (node != NULL);
  g_return_if_fail (new_tree != NULL);
  g_return_if_fail (new_node != NULL);

  if (node->children)
    {
      *new_tree = node->children;
      *new_node = (*new_tree)->root;
      while (!bobgui_tree_rbtree_is_nil ((*new_node)->left))
        *new_node = (*new_node)->left;
      return;
    }

  *new_tree = tree;
  *new_node = bobgui_tree_rbtree_next (tree, node);

  while ((*new_node == NULL) &&
         (*new_tree != NULL))
    {
      *new_node = (*new_tree)->parent_node;
      *new_tree = (*new_tree)->parent_tree;
      if (*new_tree)
        *new_node = bobgui_tree_rbtree_next (*new_tree, *new_node);
    }
}

void
bobgui_tree_rbtree_prev_full (BobguiTreeRBTree  *tree,
                           BobguiTreeRBNode  *node,
                           BobguiTreeRBTree **new_tree,
                           BobguiTreeRBNode **new_node)
{
  g_return_if_fail (tree != NULL);
  g_return_if_fail (node != NULL);
  g_return_if_fail (new_tree != NULL);
  g_return_if_fail (new_node != NULL);

  *new_tree = tree;
  *new_node = bobgui_tree_rbtree_prev (tree, node);

  if (*new_node == NULL)
    {
      *new_node = (*new_tree)->parent_node;
      *new_tree = (*new_tree)->parent_tree;
    }
  else
    {
      while ((*new_node)->children)
        {
          *new_tree = (*new_node)->children;
          *new_node = (*new_tree)->root;
          while (!bobgui_tree_rbtree_is_nil ((*new_node)->right))
            *new_node = (*new_node)->right;
        }
    }
}

int
bobgui_tree_rbtree_get_depth (BobguiTreeRBTree *tree)
{
  BobguiTreeRBTree *tmp_tree;
  int depth = 0;

  tmp_tree = tree->parent_tree;
  while (tmp_tree)
    {
      ++depth;
      tmp_tree = tmp_tree->parent_tree;
    }

  return depth;
}

static void
bobgui_tree_rbtree_traverse_pre_order (BobguiTreeRBTree            *tree,
                                    BobguiTreeRBNode            *node,
                                    BobguiTreeRBTreeTraverseFunc func,
                                    gpointer                  data)
{
  if (bobgui_tree_rbtree_is_nil (node))
    return;

  (*func)(tree, node, data);
  bobgui_tree_rbtree_traverse_pre_order (tree, node->left, func, data);
  bobgui_tree_rbtree_traverse_pre_order (tree, node->right, func, data);
}

static void
bobgui_tree_rbtree_traverse_post_order (BobguiTreeRBTree            *tree,
                                     BobguiTreeRBNode            *node,
                                     BobguiTreeRBTreeTraverseFunc func,
                                     gpointer                  data)
{
  if (bobgui_tree_rbtree_is_nil (node))
    return;

  bobgui_tree_rbtree_traverse_post_order (tree, node->left, func, data);
  bobgui_tree_rbtree_traverse_post_order (tree, node->right, func, data);
  (*func)(tree, node, data);
}

void
bobgui_tree_rbtree_traverse (BobguiTreeRBTree            *tree,
                          BobguiTreeRBNode            *node,
                          GTraverseType             order,
                          BobguiTreeRBTreeTraverseFunc func,
                          gpointer                  data)
{
  g_return_if_fail (tree != NULL);
  g_return_if_fail (node != NULL);
  g_return_if_fail (func != NULL);
  g_return_if_fail (order <= G_LEVEL_ORDER);

  switch (order)
    {
    case G_PRE_ORDER:
      bobgui_tree_rbtree_traverse_pre_order (tree, node, func, data);
      break;

    case G_POST_ORDER:
      bobgui_tree_rbtree_traverse_post_order (tree, node, func, data);
      break;

    case G_IN_ORDER:
    case G_LEVEL_ORDER:
    default:
      g_warning ("unsupported traversal order.");
      break;
    }
}

static inline
void fixup_validation (BobguiTreeRBTree *tree,
                       BobguiTreeRBNode *node)
{
  if (BOBGUI_TREE_RBNODE_FLAG_SET (node, BOBGUI_TREE_RBNODE_INVALID) ||
      BOBGUI_TREE_RBNODE_FLAG_SET (node, BOBGUI_TREE_RBNODE_COLUMN_INVALID) ||
      BOBGUI_TREE_RBNODE_FLAG_SET (node->left, BOBGUI_TREE_RBNODE_DESCENDANTS_INVALID) ||
      BOBGUI_TREE_RBNODE_FLAG_SET (node->right, BOBGUI_TREE_RBNODE_DESCENDANTS_INVALID) ||
      (node->children != NULL && BOBGUI_TREE_RBNODE_FLAG_SET (node->children->root, BOBGUI_TREE_RBNODE_DESCENDANTS_INVALID)))
    {
      BOBGUI_TREE_RBNODE_SET_FLAG (node, BOBGUI_TREE_RBNODE_DESCENDANTS_INVALID);
    }
  else
    {
      BOBGUI_TREE_RBNODE_UNSET_FLAG (node, BOBGUI_TREE_RBNODE_DESCENDANTS_INVALID);
    }
}

static inline
void fixup_total_count (BobguiTreeRBTree *tree,
                        BobguiTreeRBNode *node)
{
  node->total_count = 1 +
                      (node->children != NULL ? node->children->root->total_count : 0) +
                      node->left->total_count + node->right->total_count;
}

#ifndef G_DISABLE_ASSERT
static guint
get_total_count (BobguiTreeRBNode *node)
{
  guint child_total = 0;

  child_total += (guint) node->left->total_count;
  child_total += (guint) node->right->total_count;

  if (node->children)
    child_total += (guint) node->children->root->total_count;

  return child_total + 1;
}

static guint
count_total (BobguiTreeRBTree *tree,
             BobguiTreeRBNode *node)
{
  guint res;

  if (bobgui_tree_rbtree_is_nil (node))
    return 0;

  res =
    count_total (tree, node->left) +
    count_total (tree, node->right) +
    (guint) 1 +
    (node->children ? count_total (node->children, node->children->root) : 0);

  if (res != node->total_count)
    g_error ("total count incorrect for node");

  if (get_total_count (node) != node->total_count)
    g_error ("Node has incorrect total count %u, should be %u", node->total_count, get_total_count (node));

  return res;
}

static int
_count_nodes (BobguiTreeRBTree *tree,
              BobguiTreeRBNode *node)
{
  int res;
  if (bobgui_tree_rbtree_is_nil (node))
    return 0;

  g_assert (node->left);
  g_assert (node->right);

  res = (_count_nodes (tree, node->left) +
         _count_nodes (tree, node->right) + 1);

  if (res != node->count)
    g_error ("Tree failed");
  return res;
}
#endif /* G_DISABLE_ASSERT */

static void
bobgui_tree_rbtree_test_height (BobguiTreeRBTree *tree,
                             BobguiTreeRBNode *node)
{
  int computed_offset = 0;

  /* This whole test is sort of a useless truism. */

  if (!bobgui_tree_rbtree_is_nil (node->left))
    computed_offset += node->left->offset;

  if (!bobgui_tree_rbtree_is_nil (node->right))
    computed_offset += node->right->offset;

  if (node->children && !bobgui_tree_rbtree_is_nil (node->children->root))
    computed_offset += node->children->root->offset;

  if (BOBGUI_TREE_RBNODE_GET_HEIGHT (node) + computed_offset != node->offset)
    g_error ("node has broken offset");

  if (!bobgui_tree_rbtree_is_nil (node->left))
    bobgui_tree_rbtree_test_height (tree, node->left);

  if (!bobgui_tree_rbtree_is_nil (node->right))
    bobgui_tree_rbtree_test_height (tree, node->right);

  if (node->children && !bobgui_tree_rbtree_is_nil (node->children->root))
    bobgui_tree_rbtree_test_height (node->children, node->children->root);
}

static void
bobgui_tree_rbtree_test_dirty (BobguiTreeRBTree *tree,
                            BobguiTreeRBNode *node,
                            int            expected_dirtyness)
{
  g_assert (node);

  if (expected_dirtyness)
    {
      g_assert (BOBGUI_TREE_RBNODE_FLAG_SET (node, BOBGUI_TREE_RBNODE_COLUMN_INVALID) ||
                BOBGUI_TREE_RBNODE_FLAG_SET (node, BOBGUI_TREE_RBNODE_INVALID) ||
                BOBGUI_TREE_RBNODE_FLAG_SET (node->left, BOBGUI_TREE_RBNODE_DESCENDANTS_INVALID) ||
                BOBGUI_TREE_RBNODE_FLAG_SET (node->right, BOBGUI_TREE_RBNODE_DESCENDANTS_INVALID) ||
                (node->children && BOBGUI_TREE_RBNODE_FLAG_SET (node->children->root, BOBGUI_TREE_RBNODE_DESCENDANTS_INVALID)));
    }
  else
    {
      g_assert (!BOBGUI_TREE_RBNODE_FLAG_SET (node, BOBGUI_TREE_RBNODE_COLUMN_INVALID) &&
                !BOBGUI_TREE_RBNODE_FLAG_SET (node, BOBGUI_TREE_RBNODE_INVALID));
      if (!bobgui_tree_rbtree_is_nil (node->left))
        g_assert (!BOBGUI_TREE_RBNODE_FLAG_SET (node->left, BOBGUI_TREE_RBNODE_DESCENDANTS_INVALID));
      if (!bobgui_tree_rbtree_is_nil (node->right))
        g_assert (!BOBGUI_TREE_RBNODE_FLAG_SET (node->right, BOBGUI_TREE_RBNODE_DESCENDANTS_INVALID));
      if (node->children != NULL)
        g_assert (!BOBGUI_TREE_RBNODE_FLAG_SET (node->children->root, BOBGUI_TREE_RBNODE_DESCENDANTS_INVALID));
    }

  if (!bobgui_tree_rbtree_is_nil (node->left))
    bobgui_tree_rbtree_test_dirty (tree, node->left, BOBGUI_TREE_RBNODE_FLAG_SET (node->left, BOBGUI_TREE_RBNODE_DESCENDANTS_INVALID));
  if (!bobgui_tree_rbtree_is_nil (node->right))
    bobgui_tree_rbtree_test_dirty (tree, node->right, BOBGUI_TREE_RBNODE_FLAG_SET (node->right, BOBGUI_TREE_RBNODE_DESCENDANTS_INVALID));
  if (node->children != NULL && !bobgui_tree_rbtree_is_nil (node->children->root))
    bobgui_tree_rbtree_test_dirty (node->children, node->children->root, BOBGUI_TREE_RBNODE_FLAG_SET (node->children->root, BOBGUI_TREE_RBNODE_DESCENDANTS_INVALID));
}

static void bobgui_tree_rbtree_test_structure (BobguiTreeRBTree *tree);

static void
bobgui_tree_rbtree_test_structure_helper (BobguiTreeRBTree *tree,
                                       BobguiTreeRBNode *node)
{
  g_assert (!bobgui_tree_rbtree_is_nil (node));

  g_assert (node->left != NULL);
  g_assert (node->right != NULL);
  g_assert (node->parent != NULL);

  if (!bobgui_tree_rbtree_is_nil (node->left))
    {
      g_assert (node->left->parent == node);
      bobgui_tree_rbtree_test_structure_helper (tree, node->left);
    }
  if (!bobgui_tree_rbtree_is_nil (node->right))
    {
      g_assert (node->right->parent == node);
      bobgui_tree_rbtree_test_structure_helper (tree, node->right);
    }

  if (node->children != NULL)
    {
      g_assert (node->children->parent_tree == tree);
      g_assert (node->children->parent_node == node);

      bobgui_tree_rbtree_test_structure (node->children);
    }
}
static void
bobgui_tree_rbtree_test_structure (BobguiTreeRBTree *tree)
{
  g_assert (tree->root);
  if (bobgui_tree_rbtree_is_nil (tree->root))
    return;

  g_assert (bobgui_tree_rbtree_is_nil (tree->root->parent));
  bobgui_tree_rbtree_test_structure_helper (tree, tree->root);
}

static void
bobgui_tree_rbtree_test (const char    *where,
                      BobguiTreeRBTree *tree)
{
  BobguiTreeRBTree *tmp_tree;

  if (tree == NULL)
    return;

  /* Test the entire tree */
  tmp_tree = tree;
  while (tmp_tree->parent_tree)
    tmp_tree = tmp_tree->parent_tree;

  if (bobgui_tree_rbtree_is_nil (tmp_tree->root))
    return;

  bobgui_tree_rbtree_test_structure (tmp_tree);

  g_assert ((_count_nodes (tmp_tree, tmp_tree->root->left) +
             _count_nodes (tmp_tree, tmp_tree->root->right) + 1) == tmp_tree->root->count);


  bobgui_tree_rbtree_test_height (tmp_tree, tmp_tree->root);
  bobgui_tree_rbtree_test_dirty (tmp_tree, tmp_tree->root, BOBGUI_TREE_RBNODE_FLAG_SET (tmp_tree->root, BOBGUI_TREE_RBNODE_DESCENDANTS_INVALID));
  g_assert (count_total (tmp_tree, tmp_tree->root) == tmp_tree->root->total_count);
}

static void
bobgui_tree_rbtree_debug_spew_helper (BobguiTreeRBTree *tree,
                                   BobguiTreeRBNode *node,
                                   GString       *s,
                                   int            depth)
{
  int i;
  for (i = 0; i < depth; i++)
    g_string_append (s, "\t");

  g_string_append_printf (s, "(%p - %s) (Offset %d) (Parity %d) (Validity %d%d%d)\n",
                          node,
                          (BOBGUI_TREE_RBNODE_GET_COLOR (node) == BOBGUI_TREE_RBNODE_BLACK) ? "BLACK" : " RED ",
                          node->offset,
                          node->total_count,
                          (BOBGUI_TREE_RBNODE_FLAG_SET (node, BOBGUI_TREE_RBNODE_DESCENDANTS_INVALID)) ? 1 : 0,
                          (BOBGUI_TREE_RBNODE_FLAG_SET (node, BOBGUI_TREE_RBNODE_INVALID)) ? 1 : 0,
                          (BOBGUI_TREE_RBNODE_FLAG_SET (node, BOBGUI_TREE_RBNODE_COLUMN_INVALID)) ? 1 : 0);
  if (node->children != NULL)
    {
      g_string_append (s, "Looking at child.\n");
      bobgui_tree_rbtree_debug_spew (node->children, s);
      g_string_append (s, "Done looking at child.\n");
    }
  if (!bobgui_tree_rbtree_is_nil (node->left))
    {
      bobgui_tree_rbtree_debug_spew_helper (tree, node->left, s, depth + 1);
    }
  if (!bobgui_tree_rbtree_is_nil (node->right))
    {
      bobgui_tree_rbtree_debug_spew_helper (tree, node->right, s, depth + 1);
    }
}

static void
bobgui_tree_rbtree_debug_spew (BobguiTreeRBTree *tree,
                            GString       *s)
{
  g_return_if_fail (tree != NULL);

  if (bobgui_tree_rbtree_is_nil (tree->root))
    g_string_append (s, "Empty tree...");
  else
    bobgui_tree_rbtree_debug_spew_helper (tree, tree->root, s, 0);
}
