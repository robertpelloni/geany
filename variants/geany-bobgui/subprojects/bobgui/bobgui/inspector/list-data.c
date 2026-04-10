/*
 * Copyright (c) 2020 Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#include "list-data.h"

#include "bobguicolumnview.h"
#include "bobguitogglebutton.h"
#include "bobguilabel.h"
#include "bobguistack.h"
#include "bobguiboxlayout.h"
#include "bobguiorientable.h"
#include "bobguinoselection.h"
#include "bobguisignallistitemfactory.h"
#include "bobguilistitem.h"
#include "window.h"


struct _BobguiInspectorListData
{
  BobguiWidget parent_instance;

  BobguiWidget *box;
  BobguiWidget *swin;
  GListModel *object;
  BobguiColumnView *view;
  BobguiWidget *items_label;
};

struct _BobguiInspectorListDataClass
{
  BobguiWidgetClass parent_class;
};

G_DEFINE_TYPE (BobguiInspectorListData, bobgui_inspector_list_data, BOBGUI_TYPE_WIDGET)

static void
bobgui_inspector_list_data_init (BobguiInspectorListData *sl)
{
  bobgui_widget_init_template (BOBGUI_WIDGET (sl));

  bobgui_orientable_set_orientation (BOBGUI_ORIENTABLE (bobgui_widget_get_layout_manager (BOBGUI_WIDGET (sl))),
                                  BOBGUI_ORIENTATION_VERTICAL);
}

void
bobgui_inspector_list_data_set_object (BobguiInspectorListData *sl,
                                    GObject              *object)
{
  BobguiWidget *stack;
  BobguiStackPage *page;
  char *text;
  BobguiNoSelection *selection;

  stack = bobgui_widget_get_parent (BOBGUI_WIDGET (sl));
  page = bobgui_stack_get_page (BOBGUI_STACK (stack), BOBGUI_WIDGET (sl));

  bobgui_column_view_set_model (sl->view, NULL);
  sl->object = NULL;

  if (!G_IS_LIST_MODEL (object))
    {
      g_object_set (page, "visible", FALSE, NULL);
      return;
    }

  text = g_strdup_printf ("%u items", g_list_model_get_n_items (G_LIST_MODEL (object)));
  bobgui_label_set_label (BOBGUI_LABEL (sl->items_label), text);
  g_free (text);

  g_object_set (page, "visible", TRUE, NULL);

  sl->object = G_LIST_MODEL (object);
  selection = bobgui_no_selection_new (g_object_ref (sl->object));
  bobgui_column_view_set_model (sl->view, BOBGUI_SELECTION_MODEL (selection));
  g_object_unref (selection);
}

static void
setup_object (BobguiSignalListItemFactory *factory,
              BobguiListItem              *item)
{
  BobguiWidget *label;

  label = bobgui_label_new ("");
  bobgui_label_set_xalign (BOBGUI_LABEL (label), 0);
  bobgui_widget_add_css_class (label, "cell");
  bobgui_list_item_set_child (item, label);
}

static void
bind_object (BobguiSignalListItemFactory *factory,
             BobguiListItem              *item)
{
  BobguiWidget *label;
  gpointer obj;
  char *text;

  label = bobgui_list_item_get_child (item);
  obj = bobgui_list_item_get_item (item);

  text = g_strdup_printf ("%p", obj);
  bobgui_label_set_label (BOBGUI_LABEL (label), text);
  g_free (text);
}

static void
setup_type (BobguiSignalListItemFactory *factory,
            BobguiListItem              *item)
{
  BobguiWidget *label;

  label = bobgui_label_new ("");
  bobgui_label_set_xalign (BOBGUI_LABEL (label), 0);
  bobgui_widget_add_css_class (label, "cell");
  bobgui_list_item_set_child (item, label);
}

static void
bind_type (BobguiSignalListItemFactory *factory,
           BobguiListItem              *item)
{
  BobguiWidget *label;
  gpointer obj;

  label = bobgui_list_item_get_child (item);
  obj = bobgui_list_item_get_item (item);

  bobgui_label_set_label (BOBGUI_LABEL (label), G_OBJECT_TYPE_NAME (obj));
}

static void
setup_props (BobguiSignalListItemFactory *factory,
             BobguiListItem              *item)
{
  BobguiWidget *button;

  button = bobgui_button_new_with_label ("Properties");
  bobgui_widget_add_css_class (button, "cell");
  bobgui_widget_set_halign (button, BOBGUI_ALIGN_START);
  bobgui_list_item_set_child (item, button);
}

static void
object_properties (BobguiWidget   *button,
                   BobguiListItem *item)
{
  BobguiInspectorWindow *iw;
  GObject *obj;
  guint pos;

  iw = BOBGUI_INSPECTOR_WINDOW (bobgui_widget_get_ancestor (button, BOBGUI_TYPE_INSPECTOR_WINDOW));

  obj = bobgui_list_item_get_item (item);
  pos = bobgui_list_item_get_position (item);
  bobgui_inspector_window_push_object (iw, obj, CHILD_KIND_LISTITEM, pos);
}

static void
bind_props (BobguiSignalListItemFactory *factory,
            BobguiListItem              *item,
            BobguiInspectorListData     *sl)
{
  g_signal_connect (bobgui_list_item_get_child (item), "clicked",
                    G_CALLBACK (object_properties), item);
}

static void
unbind_props (BobguiSignalListItemFactory *factory,
              BobguiListItem              *item)
{
  g_signal_handlers_disconnect_by_func (bobgui_list_item_get_child (item), object_properties, item);
}

static void
dispose (GObject *object)
{
  BobguiInspectorListData *sl = BOBGUI_INSPECTOR_LIST_DATA (object);

  bobgui_widget_dispose_template (BOBGUI_WIDGET (sl), BOBGUI_TYPE_INSPECTOR_LIST_DATA);

  G_OBJECT_CLASS (bobgui_inspector_list_data_parent_class)->dispose (object);
}

static void
bobgui_inspector_list_data_class_init (BobguiInspectorListDataClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (klass);

  object_class->dispose = dispose;

  bobgui_widget_class_set_template_from_resource (widget_class, "/org/bobgui/libbobgui/inspector/list-data.ui");
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorListData, box);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorListData, swin);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorListData, view);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorListData, items_label);

  bobgui_widget_class_bind_template_callback (widget_class, setup_object);
  bobgui_widget_class_bind_template_callback (widget_class, bind_object);
  bobgui_widget_class_bind_template_callback (widget_class, setup_type);
  bobgui_widget_class_bind_template_callback (widget_class, bind_type);
  bobgui_widget_class_bind_template_callback (widget_class, setup_props);
  bobgui_widget_class_bind_template_callback (widget_class, bind_props);
  bobgui_widget_class_bind_template_callback (widget_class, unbind_props);

  bobgui_widget_class_set_layout_manager_type (widget_class, BOBGUI_TYPE_BOX_LAYOUT);
}

// vim: set et sw=2 ts=2:
