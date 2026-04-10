/* bobguitreemodelfilter.h
 * Copyright (C) 2000,2001  Red Hat, Inc., Jonathan Blandford <jrb@redhat.com>
 * Copyright (C) 2001-2003  Kristian Rietveld <kris@bobgui.org>
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

#if !defined (__BOBGUI_H_INSIDE__) && !defined (BOBGUI_COMPILATION)
#error "Only <bobgui/bobgui.h> can be included directly."
#endif

#include <bobgui/deprecated/bobguitreemodel.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_TREE_MODEL_FILTER              (bobgui_tree_model_filter_get_type ())
#define BOBGUI_TREE_MODEL_FILTER(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_TREE_MODEL_FILTER, BobguiTreeModelFilter))
#define BOBGUI_TREE_MODEL_FILTER_CLASS(vtable)     (G_TYPE_CHECK_CLASS_CAST ((vtable), BOBGUI_TYPE_TREE_MODEL_FILTER, BobguiTreeModelFilterClass))
#define BOBGUI_IS_TREE_MODEL_FILTER(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOBGUI_TYPE_TREE_MODEL_FILTER))
#define BOBGUI_IS_TREE_MODEL_FILTER_CLASS(vtable)  (G_TYPE_CHECK_CLASS_TYPE ((vtable), BOBGUI_TYPE_TREE_MODEL_FILTER))
#define BOBGUI_TREE_MODEL_FILTER_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), BOBGUI_TYPE_TREE_MODEL_FILTER, BobguiTreeModelFilterClass))

/**
 * BobguiTreeModelFilterVisibleFunc:
 * @model: the child model of the `BobguiTreeModelFilter`
 * @iter: a `BobguiTreeIter` pointing to the row in @model whose visibility
 *   is determined
 * @data: (closure): user data given to bobgui_tree_model_filter_set_visible_func()
 *
 * A function which decides whether the row indicated by @iter is visible.
 *
 * Returns: Whether the row indicated by @iter is visible.
 *
 * Deprecated: 4.20: There is no replacement
 */
typedef gboolean (* BobguiTreeModelFilterVisibleFunc) (BobguiTreeModel *model,
                                                    BobguiTreeIter  *iter,
                                                    gpointer      data);

/**
 * BobguiTreeModelFilterModifyFunc:
 * @model: the `BobguiTreeModelFilter`
 * @iter: a `BobguiTreeIter` pointing to the row whose display values are determined
 * @value: (out caller-allocates): A `GValue` which is already initialized for
 *  with the correct type for the column @column.
 * @column: the column whose display value is determined
 * @data: (closure): user data given to bobgui_tree_model_filter_set_modify_func()
 *
 * A function which calculates display values from raw values in the model.
 * It must fill @value with the display value for the column @column in the
 * row indicated by @iter.
 *
 * Since this function is called for each data access, it’s not a
 * particularly efficient operation.
 *
 * Deprecated: 4.20: There is no replacement
 */

typedef void (* BobguiTreeModelFilterModifyFunc) (BobguiTreeModel *model,
                                               BobguiTreeIter  *iter,
                                               GValue       *value,
                                               int           column,
                                               gpointer      data);

typedef struct _BobguiTreeModelFilter          BobguiTreeModelFilter;
typedef struct _BobguiTreeModelFilterClass     BobguiTreeModelFilterClass;
typedef struct _BobguiTreeModelFilterPrivate   BobguiTreeModelFilterPrivate;

struct _BobguiTreeModelFilter
{
  GObject parent;

  /*< private >*/
  BobguiTreeModelFilterPrivate *priv;
};

struct _BobguiTreeModelFilterClass
{
  GObjectClass parent_class;

  gboolean (* visible) (BobguiTreeModelFilter *self,
                        BobguiTreeModel       *child_model,
                        BobguiTreeIter        *iter);
  void (* modify) (BobguiTreeModelFilter *self,
                   BobguiTreeModel       *child_model,
                   BobguiTreeIter        *iter,
                   GValue             *value,
                   int                 column);

  /*< private >*/

  gpointer padding[8];
};

/* base */
GDK_AVAILABLE_IN_ALL
GType         bobgui_tree_model_filter_get_type                   (void) G_GNUC_CONST;
GDK_DEPRECATED_IN_4_10_FOR(BobguiFilterListModel)
BobguiTreeModel *bobgui_tree_model_filter_new                        (BobguiTreeModel                 *child_model,
                                                                BobguiTreePath                  *root);
GDK_DEPRECATED_IN_4_10_FOR(BobguiFilterListModel)
void          bobgui_tree_model_filter_set_visible_func           (BobguiTreeModelFilter           *filter,
                                                                BobguiTreeModelFilterVisibleFunc func,
                                                                gpointer                      data,
                                                                GDestroyNotify                destroy);
GDK_DEPRECATED_IN_4_10_FOR(BobguiFilterListModel)
void          bobgui_tree_model_filter_set_modify_func            (BobguiTreeModelFilter           *filter,
                                                                int                           n_columns,
                                                                GType                        *types,
                                                                BobguiTreeModelFilterModifyFunc  func,
                                                                gpointer                      data,
                                                                GDestroyNotify                destroy);
GDK_DEPRECATED_IN_4_10_FOR(BobguiFilterListModel)
void          bobgui_tree_model_filter_set_visible_column         (BobguiTreeModelFilter           *filter,
                                                                int                           column);

GDK_DEPRECATED_IN_4_10_FOR(BobguiFilterListModel)
BobguiTreeModel *bobgui_tree_model_filter_get_model                  (BobguiTreeModelFilter           *filter);

/* conversion */
GDK_DEPRECATED_IN_4_10_FOR(BobguiFilterListModel)
gboolean      bobgui_tree_model_filter_convert_child_iter_to_iter (BobguiTreeModelFilter           *filter,
                                                                BobguiTreeIter                  *filter_iter,
                                                                BobguiTreeIter                  *child_iter);
GDK_DEPRECATED_IN_4_10_FOR(BobguiFilterListModel)
void          bobgui_tree_model_filter_convert_iter_to_child_iter (BobguiTreeModelFilter           *filter,
                                                                BobguiTreeIter                  *child_iter,
                                                                BobguiTreeIter                  *filter_iter);
GDK_DEPRECATED_IN_4_10_FOR(BobguiFilterListModel)
BobguiTreePath  *bobgui_tree_model_filter_convert_child_path_to_path (BobguiTreeModelFilter           *filter,
                                                                BobguiTreePath                  *child_path);
GDK_DEPRECATED_IN_4_10_FOR(BobguiFilterListModel)
BobguiTreePath  *bobgui_tree_model_filter_convert_path_to_child_path (BobguiTreeModelFilter           *filter,
                                                                BobguiTreePath                  *filter_path);

/* extras */
GDK_DEPRECATED_IN_4_10_FOR(BobguiFilterListModel)
void          bobgui_tree_model_filter_refilter                   (BobguiTreeModelFilter           *filter);
GDK_DEPRECATED_IN_4_10_FOR(BobguiFilterListModel)
void          bobgui_tree_model_filter_clear_cache                (BobguiTreeModelFilter           *filter);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(BobguiTreeModelFilter, g_object_unref)

G_END_DECLS

