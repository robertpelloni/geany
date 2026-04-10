/* bobguiliststore.h
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

#pragma once

#if !defined (__BOBGUI_H_INSIDE__) && !defined (BOBGUI_COMPILATION)
#error "Only <bobgui/bobgui.h> can be included directly."
#endif

#include <gdk/gdk.h>
#include <bobgui/deprecated/bobguitreemodel.h>
#include <bobgui/deprecated/bobguitreesortable.h>


G_BEGIN_DECLS


#define BOBGUI_TYPE_LIST_STORE	       (bobgui_list_store_get_type ())
#define BOBGUI_LIST_STORE(obj)	       (G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_LIST_STORE, BobguiListStore))
#define BOBGUI_LIST_STORE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BOBGUI_TYPE_LIST_STORE, BobguiListStoreClass))
#define BOBGUI_IS_LIST_STORE(obj)	       (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOBGUI_TYPE_LIST_STORE))
#define BOBGUI_IS_LIST_STORE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BOBGUI_TYPE_LIST_STORE))
#define BOBGUI_LIST_STORE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BOBGUI_TYPE_LIST_STORE, BobguiListStoreClass))

typedef struct _BobguiListStore              BobguiListStore;
typedef struct _BobguiListStorePrivate       BobguiListStorePrivate;
typedef struct _BobguiListStoreClass         BobguiListStoreClass;

struct _BobguiListStore
{
  GObject parent;

  /*< private >*/
  BobguiListStorePrivate *priv;
};

struct _BobguiListStoreClass
{
  GObjectClass parent_class;

  /*< private >*/
  gpointer padding[8];
};


GDK_AVAILABLE_IN_ALL
GType         bobgui_list_store_get_type         (void) G_GNUC_CONST;
GDK_DEPRECATED_IN_4_10_FOR(GListStore)
BobguiListStore *bobgui_list_store_new              (int           n_columns,
					       ...);
GDK_DEPRECATED_IN_4_10_FOR(GListStore)
BobguiListStore *bobgui_list_store_newv             (int           n_columns,
					       GType        *types);
GDK_DEPRECATED_IN_4_10_FOR(GListStore)
void          bobgui_list_store_set_column_types (BobguiListStore *list_store,
					       int           n_columns,
					       GType        *types);

/* NOTE: use bobgui_tree_model_get to get values from a BobguiListStore */

GDK_DEPRECATED_IN_4_10_FOR(GListStore)
void          bobgui_list_store_set_value        (BobguiListStore *list_store,
					       BobguiTreeIter  *iter,
					       int           column,
					       GValue       *value);
GDK_DEPRECATED_IN_4_10_FOR(GListStore)
void          bobgui_list_store_set              (BobguiListStore *list_store,
					       BobguiTreeIter  *iter,
					       ...);
GDK_DEPRECATED_IN_4_10_FOR(GListStore)
void          bobgui_list_store_set_valuesv      (BobguiListStore *list_store,
					       BobguiTreeIter  *iter,
					       int          *columns,
					       GValue       *values,
					       int           n_values);
GDK_DEPRECATED_IN_4_10_FOR(GListStore)
void          bobgui_list_store_set_valist       (BobguiListStore *list_store,
					       BobguiTreeIter  *iter,
					       va_list       var_args);
GDK_DEPRECATED_IN_4_10_FOR(GListStore)
gboolean      bobgui_list_store_remove           (BobguiListStore *list_store,
					       BobguiTreeIter  *iter);
GDK_DEPRECATED_IN_4_10_FOR(GListStore)
void          bobgui_list_store_insert           (BobguiListStore *list_store,
					       BobguiTreeIter  *iter,
					       int           position);
GDK_DEPRECATED_IN_4_10_FOR(GListStore)
void          bobgui_list_store_insert_before    (BobguiListStore *list_store,
					       BobguiTreeIter  *iter,
					       BobguiTreeIter  *sibling);
GDK_DEPRECATED_IN_4_10_FOR(GListStore)
void          bobgui_list_store_insert_after     (BobguiListStore *list_store,
					       BobguiTreeIter  *iter,
					       BobguiTreeIter  *sibling);
GDK_DEPRECATED_IN_4_10_FOR(GListStore)
void          bobgui_list_store_insert_with_values  (BobguiListStore *list_store,
						  BobguiTreeIter  *iter,
						  int           position,
						  ...);
GDK_DEPRECATED_IN_4_10_FOR(GListStore)
void          bobgui_list_store_insert_with_valuesv (BobguiListStore *list_store,
						  BobguiTreeIter  *iter,
						  int           position,
						  int          *columns,
						  GValue       *values,
						  int           n_values);
GDK_DEPRECATED_IN_4_10_FOR(GListStore)
void          bobgui_list_store_prepend          (BobguiListStore *list_store,
					       BobguiTreeIter  *iter);
GDK_DEPRECATED_IN_4_10_FOR(GListStore)
void          bobgui_list_store_append           (BobguiListStore *list_store,
					       BobguiTreeIter  *iter);
GDK_DEPRECATED_IN_4_10_FOR(GListStore)
void          bobgui_list_store_clear            (BobguiListStore *list_store);
GDK_DEPRECATED_IN_4_10_FOR(GListStore)
gboolean      bobgui_list_store_iter_is_valid    (BobguiListStore *list_store,
                                               BobguiTreeIter  *iter);
GDK_DEPRECATED_IN_4_10_FOR(GListStore)
void          bobgui_list_store_reorder          (BobguiListStore *store,
                                               int          *new_order);
GDK_DEPRECATED_IN_4_10_FOR(GListStore)
void          bobgui_list_store_swap             (BobguiListStore *store,
                                               BobguiTreeIter  *a,
                                               BobguiTreeIter  *b);
GDK_DEPRECATED_IN_4_10_FOR(GListStore)
void          bobgui_list_store_move_after       (BobguiListStore *store,
                                               BobguiTreeIter  *iter,
                                               BobguiTreeIter  *position);
GDK_DEPRECATED_IN_4_10_FOR(GListStore)
void          bobgui_list_store_move_before      (BobguiListStore *store,
                                               BobguiTreeIter  *iter,
                                               BobguiTreeIter  *position);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(BobguiListStore, g_object_unref)

G_END_DECLS


