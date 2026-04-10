/* bobguirbtreeprivate.h
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

/* A Red-Black Tree implementation used specifically by BobguiTreeView.
 */
#pragma once

#include <glib.h>


G_BEGIN_DECLS


typedef enum
{
  BOBGUI_TREE_RBNODE_BLACK = 1 << 0,
  BOBGUI_TREE_RBNODE_RED = 1 << 1,
  BOBGUI_TREE_RBNODE_IS_PARENT = 1 << 2,
  BOBGUI_TREE_RBNODE_IS_SELECTED = 1 << 3,
  BOBGUI_TREE_RBNODE_IS_PRELIT = 1 << 4,
  BOBGUI_TREE_RBNODE_INVALID = 1 << 7,
  BOBGUI_TREE_RBNODE_COLUMN_INVALID = 1 << 8,
  BOBGUI_TREE_RBNODE_DESCENDANTS_INVALID = 1 << 9,
  BOBGUI_TREE_RBNODE_NON_COLORS = BOBGUI_TREE_RBNODE_IS_PARENT |
                            BOBGUI_TREE_RBNODE_IS_SELECTED |
                            BOBGUI_TREE_RBNODE_IS_PRELIT |
                          BOBGUI_TREE_RBNODE_INVALID |
                          BOBGUI_TREE_RBNODE_COLUMN_INVALID |
                          BOBGUI_TREE_RBNODE_DESCENDANTS_INVALID
} BobguiTreeRBNodeColor;

typedef struct _BobguiTreeRBTree BobguiTreeRBTree;
typedef struct _BobguiTreeRBNode BobguiTreeRBNode;
typedef struct _BobguiTreeRBTreeView BobguiTreeRBTreeView;

typedef void (*BobguiTreeRBTreeTraverseFunc) (BobguiTreeRBTree  *tree,
                                       BobguiTreeRBNode  *node,
                                       gpointer  data);

struct _BobguiTreeRBTree
{
  BobguiTreeRBNode *root;
  BobguiTreeRBTree *parent_tree;
  BobguiTreeRBNode *parent_node;
};

struct _BobguiTreeRBNode
{
  guint flags : 14;

  /* count is the number of nodes beneath us, plus 1 for ourselves.
   * i.e. node->left->count + node->right->count + 1
   */
  int count;

  BobguiTreeRBNode *left;
  BobguiTreeRBNode *right;
  BobguiTreeRBNode *parent;

  /* count the number of total nodes beneath us, including nodes
   * of children trees.
   * i.e. node->left->count + node->right->count + node->children->root->count + 1
   */
  guint total_count;
  
  /* this is the total of sizes of
   * node->left, node->right, our own height, and the height
   * of all trees in ->children, iff children exists because
   * the thing is expanded.
   */
  int offset;

  /* Child trees */
  BobguiTreeRBTree *children;
};


#define BOBGUI_TREE_RBNODE_GET_COLOR(node)                (node?(((node->flags&BOBGUI_TREE_RBNODE_RED)==BOBGUI_TREE_RBNODE_RED)?BOBGUI_TREE_RBNODE_RED:BOBGUI_TREE_RBNODE_BLACK):BOBGUI_TREE_RBNODE_BLACK)
#define BOBGUI_TREE_RBNODE_SET_COLOR(node,color)         if((node->flags&color)!=color)node->flags=node->flags^(BOBGUI_TREE_RBNODE_RED|BOBGUI_TREE_RBNODE_BLACK)
#define BOBGUI_TREE_RBNODE_GET_HEIGHT(node)                 (node->offset-(node->left->offset+node->right->offset+(node->children?node->children->root->offset:0)))
#define BOBGUI_TREE_RBNODE_SET_FLAG(node, flag)           G_STMT_START{ (node->flags|=flag); }G_STMT_END
#define BOBGUI_TREE_RBNODE_UNSET_FLAG(node, flag)         G_STMT_START{ (node->flags&=~(flag)); }G_STMT_END
#define BOBGUI_TREE_RBNODE_FLAG_SET(node, flag)         (node?(((node->flags&flag)==flag)?TRUE:FALSE):FALSE)


BobguiTreeRBTree * bobgui_tree_rbtree_new                     (void);
void            bobgui_tree_rbtree_free                    (BobguiTreeRBTree                 *tree);
void            bobgui_tree_rbtree_remove                  (BobguiTreeRBTree                 *tree);
void            bobgui_tree_rbtree_destroy                 (BobguiTreeRBTree                 *tree);
BobguiTreeRBNode * bobgui_tree_rbtree_insert_before           (BobguiTreeRBTree                 *tree,
                                                         BobguiTreeRBNode                 *node,
                                                         int                            height,
                                                         gboolean                       valid);
BobguiTreeRBNode * bobgui_tree_rbtree_insert_after            (BobguiTreeRBTree                 *tree,
                                                         BobguiTreeRBNode                 *node,
                                                         int                            height,
                                                         gboolean                       valid);
void            bobgui_tree_rbtree_remove_node             (BobguiTreeRBTree                 *tree,
                                                         BobguiTreeRBNode                 *node);
gboolean        bobgui_tree_rbtree_is_nil                  (BobguiTreeRBNode                 *node);
void            bobgui_tree_rbtree_reorder                 (BobguiTreeRBTree                 *tree,
                                                         int                           *new_order,
                                                         int                            length);
gboolean        bobgui_tree_rbtree_contains                (BobguiTreeRBTree                 *tree,
                                                         BobguiTreeRBTree                 *potential_child);
BobguiTreeRBNode * bobgui_tree_rbtree_find_count              (BobguiTreeRBTree                 *tree,
                                                         int                            count);
void            bobgui_tree_rbtree_node_set_height         (BobguiTreeRBTree                 *tree,
                                                         BobguiTreeRBNode                 *node,
                                                         int                            height);
void            bobgui_tree_rbtree_node_mark_invalid       (BobguiTreeRBTree                 *tree,
                                                         BobguiTreeRBNode                 *node);
void            bobgui_tree_rbtree_node_mark_valid         (BobguiTreeRBTree                 *tree,
                                                         BobguiTreeRBNode                 *node);
void            bobgui_tree_rbtree_column_invalid          (BobguiTreeRBTree                 *tree);
void            bobgui_tree_rbtree_mark_invalid            (BobguiTreeRBTree                 *tree);
void            bobgui_tree_rbtree_set_fixed_height        (BobguiTreeRBTree                 *tree,
                                                         int                            height,
                                                         gboolean                       mark_valid);
int             bobgui_tree_rbtree_node_find_offset        (BobguiTreeRBTree                 *tree,
                                                         BobguiTreeRBNode                 *node);
guint           bobgui_tree_rbtree_node_get_index          (BobguiTreeRBTree                 *tree,
                                                         BobguiTreeRBNode                 *node);
gboolean        bobgui_tree_rbtree_find_index              (BobguiTreeRBTree                 *tree,
                                                         guint                          index,
                                                         BobguiTreeRBTree                **new_tree,
                                                         BobguiTreeRBNode                **new_node);
int             bobgui_tree_rbtree_find_offset             (BobguiTreeRBTree                 *tree,
                                                         int                            offset,
                                                         BobguiTreeRBTree                **new_tree,
                                                         BobguiTreeRBNode                **new_node);
void            bobgui_tree_rbtree_traverse                (BobguiTreeRBTree                 *tree,
                                                         BobguiTreeRBNode                 *node,
                                                         GTraverseType                  order,
                                                         BobguiTreeRBTreeTraverseFunc      func,
                                                         gpointer                       data);
BobguiTreeRBNode * bobgui_tree_rbtree_first                   (BobguiTreeRBTree                 *tree);
BobguiTreeRBNode * bobgui_tree_rbtree_next                    (BobguiTreeRBTree                 *tree,
                                                         BobguiTreeRBNode                 *node);
BobguiTreeRBNode * bobgui_tree_rbtree_prev                    (BobguiTreeRBTree                 *tree,
                                                         BobguiTreeRBNode                 *node);
void            bobgui_tree_rbtree_next_full               (BobguiTreeRBTree                 *tree,
                                                         BobguiTreeRBNode                 *node,
                                                         BobguiTreeRBTree                **new_tree,
                                                         BobguiTreeRBNode                **new_node);
void            bobgui_tree_rbtree_prev_full               (BobguiTreeRBTree                 *tree,
                                                         BobguiTreeRBNode                 *node,
                                                         BobguiTreeRBTree                **new_tree,
                                                         BobguiTreeRBNode                **new_node);

int             bobgui_tree_rbtree_get_depth               (BobguiTreeRBTree                 *tree);


G_END_DECLS


