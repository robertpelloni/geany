/*
 * Copyright (C) 2010 Openismus GmbH
 * Copyright (C) 2013 Red Hat, Inc.
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
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.

 *
 * Authors:
 *      Tristan Van Berkom <tristanvb@openismus.com>
 *      Matthias Clasen <mclasen@redhat.com>
 *      William Jon McCann <jmccann@redhat.com>
 */

#pragma once

#if !defined (__BOBGUI_H_INSIDE__) && !defined (BOBGUI_COMPILATION)
#error "Only <bobgui/bobgui.h> can be included directly."
#endif

#include <bobgui/bobguiwidget.h>

G_BEGIN_DECLS


#define BOBGUI_TYPE_FLOW_BOX                  (bobgui_flow_box_get_type ())
#define BOBGUI_FLOW_BOX(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_FLOW_BOX, BobguiFlowBox))
#define BOBGUI_IS_FLOW_BOX(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOBGUI_TYPE_FLOW_BOX))

typedef struct _BobguiFlowBox            BobguiFlowBox;
typedef struct _BobguiFlowBoxChild       BobguiFlowBoxChild;
typedef struct _BobguiFlowBoxChildClass  BobguiFlowBoxChildClass;

#define BOBGUI_TYPE_FLOW_BOX_CHILD            (bobgui_flow_box_child_get_type ())
#define BOBGUI_FLOW_BOX_CHILD(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_FLOW_BOX_CHILD, BobguiFlowBoxChild))
#define BOBGUI_FLOW_BOX_CHILD_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BOBGUI_TYPE_FLOW_BOX_CHILD, BobguiFlowBoxChildClass))
#define BOBGUI_IS_FLOW_BOX_CHILD(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOBGUI_TYPE_FLOW_BOX_CHILD))
#define BOBGUI_IS_FLOW_BOX_CHILD_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BOBGUI_TYPE_FLOW_BOX_CHILD))
#define BOBGUI_FLOW_BOX_CHILD_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), EG_TYPE_FLOW_BOX_CHILD, BobguiFlowBoxChildClass))

struct _BobguiFlowBoxChild
{
  /*< private >*/
  BobguiWidget parent_instance;
};

struct _BobguiFlowBoxChildClass
{
  /*< private >*/
  BobguiWidgetClass parent_class;

  /*< public >*/
  void (* activate) (BobguiFlowBoxChild *child);

  /*< private >*/
  gpointer padding[8];
};

/**
 * BobguiFlowBoxCreateWidgetFunc:
 * @item: (type GObject): the item from the model for which to create a widget for
 * @user_data: (closure): user data from bobgui_flow_box_bind_model()
 *
 * Called for flow boxes that are bound to a `GListModel`.
 *
 * This function is called for each item that gets added to the model.
 *
 * Returns: (transfer full): a `BobguiWidget` that represents @item
 */
typedef BobguiWidget * (*BobguiFlowBoxCreateWidgetFunc) (gpointer item,
                                                   gpointer  user_data);

GDK_AVAILABLE_IN_ALL
GType                 bobgui_flow_box_child_get_type            (void) G_GNUC_CONST;
GDK_AVAILABLE_IN_ALL
BobguiWidget*            bobgui_flow_box_child_new                 (void);

GDK_AVAILABLE_IN_ALL
void                  bobgui_flow_box_child_set_child          (BobguiFlowBoxChild *self,
                                                             BobguiWidget       *child);
GDK_AVAILABLE_IN_ALL
BobguiWidget *           bobgui_flow_box_child_get_child          (BobguiFlowBoxChild *self);

GDK_AVAILABLE_IN_ALL
int                   bobgui_flow_box_child_get_index           (BobguiFlowBoxChild *child);
GDK_AVAILABLE_IN_ALL
gboolean              bobgui_flow_box_child_is_selected         (BobguiFlowBoxChild *child);
GDK_AVAILABLE_IN_ALL
void                  bobgui_flow_box_child_changed             (BobguiFlowBoxChild *child);


GDK_AVAILABLE_IN_ALL
GType                 bobgui_flow_box_get_type                  (void) G_GNUC_CONST;

GDK_AVAILABLE_IN_ALL
BobguiWidget            *bobgui_flow_box_new                       (void);

GDK_AVAILABLE_IN_ALL
void                  bobgui_flow_box_bind_model                (BobguiFlowBox                 *box,
                                                              GListModel                 *model,
                                                              BobguiFlowBoxCreateWidgetFunc  create_widget_func,
                                                              gpointer                    user_data,
                                                              GDestroyNotify              user_data_free_func);

GDK_AVAILABLE_IN_ALL
void                  bobgui_flow_box_set_homogeneous           (BobguiFlowBox           *box,
                                                              gboolean              homogeneous);
GDK_AVAILABLE_IN_ALL
gboolean              bobgui_flow_box_get_homogeneous           (BobguiFlowBox           *box);
GDK_AVAILABLE_IN_ALL
void                  bobgui_flow_box_set_row_spacing           (BobguiFlowBox           *box,
                                                              guint                 spacing);
GDK_AVAILABLE_IN_ALL
guint                 bobgui_flow_box_get_row_spacing           (BobguiFlowBox           *box);

GDK_AVAILABLE_IN_ALL
void                  bobgui_flow_box_set_column_spacing        (BobguiFlowBox           *box,
                                                              guint                 spacing);
GDK_AVAILABLE_IN_ALL
guint                 bobgui_flow_box_get_column_spacing        (BobguiFlowBox           *box);

GDK_AVAILABLE_IN_ALL
void                  bobgui_flow_box_set_min_children_per_line (BobguiFlowBox           *box,
                                                              guint                 n_children);
GDK_AVAILABLE_IN_ALL
guint                 bobgui_flow_box_get_min_children_per_line (BobguiFlowBox           *box);

GDK_AVAILABLE_IN_ALL
void                  bobgui_flow_box_set_max_children_per_line (BobguiFlowBox           *box,
                                                              guint                 n_children);
GDK_AVAILABLE_IN_ALL
guint                 bobgui_flow_box_get_max_children_per_line (BobguiFlowBox           *box);
GDK_AVAILABLE_IN_ALL
void                  bobgui_flow_box_set_activate_on_single_click (BobguiFlowBox        *box,
                                                                 gboolean           single);
GDK_AVAILABLE_IN_ALL
gboolean              bobgui_flow_box_get_activate_on_single_click (BobguiFlowBox        *box);

GDK_AVAILABLE_IN_4_6
void                  bobgui_flow_box_prepend                      (BobguiFlowBox        *self,
                                                                 BobguiWidget         *child);
GDK_AVAILABLE_IN_4_6
void                  bobgui_flow_box_append                       (BobguiFlowBox        *self,
                                                                 BobguiWidget         *child);
GDK_AVAILABLE_IN_ALL
void                  bobgui_flow_box_insert                       (BobguiFlowBox        *box,
                                                                 BobguiWidget         *widget,
                                                                 int                position);
GDK_AVAILABLE_IN_ALL
void                  bobgui_flow_box_remove                       (BobguiFlowBox        *box,
                                                                 BobguiWidget         *widget);
GDK_AVAILABLE_IN_4_12
void                  bobgui_flow_box_remove_all                   (BobguiFlowBox        *box);

GDK_AVAILABLE_IN_ALL
BobguiFlowBoxChild      *bobgui_flow_box_get_child_at_index           (BobguiFlowBox        *box,
                                                                 int                idx);

GDK_AVAILABLE_IN_ALL
BobguiFlowBoxChild      *bobgui_flow_box_get_child_at_pos             (BobguiFlowBox        *box,
                                                                 int                x,
                                                                 int                y);

typedef void (* BobguiFlowBoxForeachFunc) (BobguiFlowBox      *box,
                                        BobguiFlowBoxChild *child,
                                        gpointer         user_data);

GDK_AVAILABLE_IN_ALL
void                  bobgui_flow_box_selected_foreach             (BobguiFlowBox        *box,
                                                                 BobguiFlowBoxForeachFunc func,
                                                                 gpointer           data);
GDK_AVAILABLE_IN_ALL
GList                *bobgui_flow_box_get_selected_children        (BobguiFlowBox        *box);
GDK_AVAILABLE_IN_ALL
void                  bobgui_flow_box_select_child                 (BobguiFlowBox        *box,
                                                                 BobguiFlowBoxChild   *child);
GDK_AVAILABLE_IN_ALL
void                  bobgui_flow_box_unselect_child               (BobguiFlowBox        *box,
                                                                 BobguiFlowBoxChild   *child);
GDK_AVAILABLE_IN_ALL
void                  bobgui_flow_box_select_all                   (BobguiFlowBox        *box);
GDK_AVAILABLE_IN_ALL
void                  bobgui_flow_box_unselect_all                 (BobguiFlowBox        *box);
GDK_AVAILABLE_IN_ALL
void                  bobgui_flow_box_set_selection_mode           (BobguiFlowBox        *box,
                                                                 BobguiSelectionMode   mode);
GDK_AVAILABLE_IN_ALL
BobguiSelectionMode      bobgui_flow_box_get_selection_mode           (BobguiFlowBox        *box);
GDK_AVAILABLE_IN_ALL
void                  bobgui_flow_box_set_hadjustment              (BobguiFlowBox        *box,
                                                                 BobguiAdjustment     *adjustment);
GDK_AVAILABLE_IN_ALL
void                  bobgui_flow_box_set_vadjustment              (BobguiFlowBox        *box,
                                                                 BobguiAdjustment     *adjustment);

typedef gboolean (*BobguiFlowBoxFilterFunc) (BobguiFlowBoxChild *child,
                                          gpointer         user_data);

GDK_AVAILABLE_IN_ALL
void                  bobgui_flow_box_set_filter_func              (BobguiFlowBox        *box,
                                                                 BobguiFlowBoxFilterFunc filter_func,
                                                                 gpointer             user_data,
                                                                 GDestroyNotify       destroy);
GDK_AVAILABLE_IN_ALL
void                  bobgui_flow_box_invalidate_filter            (BobguiFlowBox        *box);

typedef int (*BobguiFlowBoxSortFunc) (BobguiFlowBoxChild *child1,
                                   BobguiFlowBoxChild *child2,
                                   gpointer         user_data);

GDK_AVAILABLE_IN_ALL
void                  bobgui_flow_box_set_sort_func                (BobguiFlowBox        *box,
                                                                 BobguiFlowBoxSortFunc  sort_func,
                                                                 gpointer            user_data,
                                                                 GDestroyNotify      destroy);
GDK_AVAILABLE_IN_ALL
void                  bobgui_flow_box_invalidate_sort              (BobguiFlowBox         *box);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(BobguiFlowBox, g_object_unref)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(BobguiFlowBoxChild, g_object_unref)

G_END_DECLS


