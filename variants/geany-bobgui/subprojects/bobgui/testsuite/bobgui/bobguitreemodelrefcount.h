/* bobguitreemodelrefcount.h
 * Copyright (C) 2011  Kristian Rietveld <kris@bobgui.org>
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

#pragma once

#include <bobgui/bobgui.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_TREE_MODEL_REF_COUNT              (bobgui_tree_model_ref_count_get_type ())
#define BOBGUI_TREE_MODEL_REF_COUNT(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_TREE_MODEL_REF_COUNT, BobguiTreeModelRefCount))
#define BOBGUI_TREE_MODEL_REF_COUNT_CLASS(vtable)     (G_TYPE_CHECK_CLASS_CAST ((vtable), BOBGUI_TYPE_TREE_MODEL_REF_COUNT, BobguiTreeModelRefCountClass))
#define BOBGUI_IS_TREE_MODEL_REF_COUNT(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOBGUI_TYPE_TREE_MODEL_REF_COUNT))
#define BOBGUI_IS_TREE_MODEL_REF_COUNT_CLASS(vtable)  (G_TYPE_CHECK_CLASS_TYPE ((vtable), BOBGUI_TYPE_TREE_MODEL_REF_COUNT))
#define BOBGUI_TREE_MODEL_REF_COUNT_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), BOBGUI_TYPE_TREE_MODEL_REF_COUNT, BobguiTreeModelRefCountClass))


typedef struct _BobguiTreeModelRefCount        BobguiTreeModelRefCount;
typedef struct _BobguiTreeModelRefCountClass   BobguiTreeModelRefCountClass;
typedef struct _BobguiTreeModelRefCountPrivate BobguiTreeModelRefCountPrivate;

struct _BobguiTreeModelRefCount
{
  BobguiTreeStore parent;

  /* < private > */
  BobguiTreeModelRefCountPrivate *priv;
};

struct _BobguiTreeModelRefCountClass
{
  BobguiTreeStoreClass parent_class;
};


GType         bobgui_tree_model_ref_count_get_type    (void) G_GNUC_CONST;
BobguiTreeModel *bobgui_tree_model_ref_count_new         (void);

void          bobgui_tree_model_ref_count_dump        (BobguiTreeModelRefCount *ref_model);
gboolean      bobgui_tree_model_ref_count_check_level (BobguiTreeModelRefCount *ref_model,
                                                    BobguiTreeIter          *parent,
                                                    int                   expected_ref_count,
                                                    gboolean              recurse,
                                                    gboolean              may_assert);
gboolean      bobgui_tree_model_ref_count_check_node  (BobguiTreeModelRefCount *ref_model,
                                                    BobguiTreeIter          *iter,
                                                    int                   expected_ref_count,
                                                    gboolean              may_assert);

/* A couple of helpers for the tests.  Since this model will never be used
 * outside of unit tests anyway, it is probably fine to have these here
 * without namespacing.
 */

static inline void
assert_entire_model_unreferenced (BobguiTreeModelRefCount *ref_model)
{
  bobgui_tree_model_ref_count_check_level (ref_model, NULL, 0, TRUE, TRUE);
}

static inline void
assert_root_level_unreferenced (BobguiTreeModelRefCount *ref_model)
{
  bobgui_tree_model_ref_count_check_level (ref_model, NULL, 0, FALSE, TRUE);
}

static inline void
assert_level_unreferenced (BobguiTreeModelRefCount *ref_model,
                           BobguiTreeIter          *iter)
{
  bobgui_tree_model_ref_count_check_level (ref_model, iter, 0, FALSE, TRUE);
}

static inline void
assert_entire_model_referenced (BobguiTreeModelRefCount *ref_model,
                                int                   ref_count)
{
  bobgui_tree_model_ref_count_check_level (ref_model, NULL, ref_count, TRUE, TRUE);
}

static inline void
assert_not_entire_model_referenced (BobguiTreeModelRefCount *ref_model,
                                    int                   ref_count)
{
  g_assert_cmpint (bobgui_tree_model_ref_count_check_level (ref_model, NULL,
                                                         ref_count,
                                                         TRUE, FALSE),
                   ==, FALSE);
}

static inline void
assert_root_level_referenced (BobguiTreeModelRefCount *ref_model,
                              int                   ref_count)
{
  bobgui_tree_model_ref_count_check_level (ref_model, NULL, ref_count,
                                        FALSE, TRUE);
}

static inline void
assert_level_referenced (BobguiTreeModelRefCount *ref_model,
                         int                   ref_count,
                         BobguiTreeIter          *iter)
{
  bobgui_tree_model_ref_count_check_level (ref_model, iter, ref_count,
                                        FALSE, TRUE);
}

static inline void
assert_node_ref_count (BobguiTreeModelRefCount *ref_model,
                       BobguiTreeIter          *iter,
                       int                   ref_count)
{
  bobgui_tree_model_ref_count_check_node (ref_model, iter, ref_count, TRUE);
}
