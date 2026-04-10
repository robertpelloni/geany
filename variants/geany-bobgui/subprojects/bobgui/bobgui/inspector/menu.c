/*
 * Copyright (c) 2014 Red Hat, Inc.
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

#include "menu.h"

#include "bobguiwidgetprivate.h"
#include "bobguilabel.h"
#include "bobguistack.h"
#include "bobguicolumnview.h"
#include "bobguicolumnviewcolumn.h"
#include "bobguitreelistmodel.h"
#include "bobguinoselection.h"
#include "bobguisignallistitemfactory.h"
#include "bobguitreeexpander.h"
#include "bobguilistitem.h"


typedef struct _MenuItem MenuItem;

G_DECLARE_FINAL_TYPE (MenuItem, menu_item, MENU, ITEM, GObject);

struct _MenuItem
{
  GObject parent;

  char *label;
  char *action;
  char *target;
  char *icon;
  GMenuModel *model;
};

G_DEFINE_TYPE (MenuItem, menu_item, G_TYPE_OBJECT);

static void
menu_item_init (MenuItem *self)
{
}

static void
menu_item_finalize (GObject *object)
{
  MenuItem *self = MENU_ITEM (object);

  g_free (self->label);
  g_free (self->action);
  g_free (self->target);
  g_free (self->icon);
  g_clear_object (&self->model);

  G_OBJECT_CLASS (menu_item_parent_class)->finalize (object);
}

static void
menu_item_class_init (MenuItemClass *class)
{
  G_OBJECT_CLASS (class)->finalize = menu_item_finalize;
}

static MenuItem *
menu_item_new (const char *label,
               const char *action,
               const char *target,
               const char *icon,
               GMenuModel *model)
{
  MenuItem *self;

  self = g_object_new (menu_item_get_type (), NULL);
  self->label = g_strdup (label);
  self->action = g_strdup (action);
  self->target = g_strdup (target);
  self->icon = g_strdup (icon);
  g_set_object (&self->model, model);

  return self;
}

struct _BobguiInspectorMenuPrivate
{
  BobguiColumnView *view;
  BobguiTreeListModel *tree_model;
};

G_DEFINE_TYPE_WITH_PRIVATE (BobguiInspectorMenu, bobgui_inspector_menu, BOBGUI_TYPE_BOX)

static GListModel * create_model (gpointer item, gpointer user_data);

static void
setup_label (BobguiListItemFactory *factory,
             BobguiListItem        *item)
{
  BobguiWidget *expander;
  BobguiWidget *label;

  expander = bobgui_tree_expander_new ();
  label = bobgui_label_new (NULL);
  bobgui_label_set_xalign (BOBGUI_LABEL (label), 0.);
  bobgui_tree_expander_set_child (BOBGUI_TREE_EXPANDER (expander), label);

  bobgui_list_item_set_child (item, expander);
}

static void
bind_label (BobguiListItemFactory *factory,
            BobguiListItem        *item)
{
  BobguiWidget *expander;
  BobguiWidget *label;
  BobguiTreeListRow *row;
  MenuItem *menu_item;

  row = bobgui_list_item_get_item (item);
  menu_item = bobgui_tree_list_row_get_item (row);

  expander = bobgui_list_item_get_child (item);
  bobgui_tree_expander_set_list_row (BOBGUI_TREE_EXPANDER (expander), row);

  label = bobgui_tree_expander_get_child (BOBGUI_TREE_EXPANDER (expander));
  bobgui_label_set_label (BOBGUI_LABEL (label), menu_item->label);
}

static void
setup_action (BobguiListItemFactory *factory,
              BobguiListItem        *item)
{
  BobguiWidget *label;

  label = bobgui_label_new (NULL);
  bobgui_label_set_xalign (BOBGUI_LABEL (label), 0.);

  bobgui_list_item_set_child (item, label);
}

static void
bind_action (BobguiListItemFactory *factory,
             BobguiListItem        *item)
{
  BobguiWidget *label;
  BobguiTreeListRow *row;
  MenuItem *menu_item;

  row = bobgui_list_item_get_item (item);
  menu_item = bobgui_tree_list_row_get_item (row);

  label = bobgui_list_item_get_child (item);
  bobgui_label_set_label (BOBGUI_LABEL (label), menu_item->action);
}

static void
setup_target (BobguiListItemFactory *factory,
              BobguiListItem        *item)
{
  BobguiWidget *label;

  label = bobgui_label_new (NULL);
  bobgui_label_set_xalign (BOBGUI_LABEL (label), 0.);

  bobgui_list_item_set_child (item, label);
}

static void
bind_target (BobguiListItemFactory *factory,
             BobguiListItem        *item)
{
  BobguiWidget *label;
  BobguiTreeListRow *row;
  MenuItem *menu_item;

  row = bobgui_list_item_get_item (item);
  menu_item = bobgui_tree_list_row_get_item (row);

  label = bobgui_list_item_get_child (item);
  bobgui_label_set_label (BOBGUI_LABEL (label), menu_item->target);
}

static void
bobgui_inspector_menu_init (BobguiInspectorMenu *sl)
{
  BobguiTreeListModel *store;
  BobguiColumnViewColumn *column;
  BobguiListItemFactory *factory;

  sl->priv = bobgui_inspector_menu_get_instance_private (sl);
  bobgui_widget_init_template (BOBGUI_WIDGET (sl));

  store = bobgui_tree_list_model_new (G_LIST_MODEL (g_list_store_new (menu_item_get_type ())),
                                   FALSE, FALSE,
                                   create_model,
                                   sl, NULL);

  bobgui_column_view_set_model (sl->priv->view, BOBGUI_SELECTION_MODEL (bobgui_no_selection_new (G_LIST_MODEL (store))));

  column = g_list_model_get_item (bobgui_column_view_get_columns (sl->priv->view), 0);
  factory = bobgui_signal_list_item_factory_new ();
  g_signal_connect (factory, "setup", G_CALLBACK (setup_label), NULL);
  g_signal_connect (factory, "bind", G_CALLBACK (bind_label), NULL);
  bobgui_column_view_column_set_factory (column, factory);
  g_object_unref (factory);
  g_object_unref (column);

  column = g_list_model_get_item (bobgui_column_view_get_columns (sl->priv->view), 1);
  factory = bobgui_signal_list_item_factory_new ();
  g_signal_connect (factory, "setup", G_CALLBACK (setup_action), NULL);
  g_signal_connect (factory, "bind", G_CALLBACK (bind_action), NULL);
  bobgui_column_view_column_set_factory (column, factory);
  g_object_unref (factory);
  g_object_unref (column);

  column = g_list_model_get_item (bobgui_column_view_get_columns (sl->priv->view), 2);
  factory = bobgui_signal_list_item_factory_new ();
  g_signal_connect (factory, "setup", G_CALLBACK (setup_target), NULL);
  g_signal_connect (factory, "bind", G_CALLBACK (bind_target), NULL);
  bobgui_column_view_column_set_factory (column, factory);
  g_object_unref (factory);
  g_object_unref (column);

  sl->priv->tree_model = store;
}

static void
add_item (BobguiInspectorMenu *sl,
          GMenuModel       *menu,
          int               idx,
          GListStore       *store)
{
  GVariant *value;
  char *label = NULL;
  char *action = NULL;
  char *target = NULL;
  char *icon = NULL;
  GMenuModel *model;

  g_menu_model_get_item_attribute (menu, idx, G_MENU_ATTRIBUTE_LABEL, "s", &label);
  g_menu_model_get_item_attribute (menu, idx, G_MENU_ATTRIBUTE_ACTION, "s", &action);
  value = g_menu_model_get_item_attribute_value (menu, idx, G_MENU_ATTRIBUTE_TARGET, NULL);
  if (value)
    {
      target = g_variant_print (value, FALSE);
      g_variant_unref (value);
    }

  model = g_menu_model_get_item_link (menu, idx, G_MENU_LINK_SECTION);
  if (model)
    {
      if (label == NULL)
        label = g_strdup (_("Unnamed section"));
    }
  else
    model = g_menu_model_get_item_link (menu, idx, G_MENU_LINK_SUBMENU);

  g_list_store_append (store, menu_item_new (label, action, target, icon, model));

  g_clear_object (&model);
  g_free (label);
  g_free (action);
  g_free (target);
  g_free (icon);
}

static void
add_menu (BobguiInspectorMenu *sl,
          GMenuModel       *menu,
          GListStore       *store)
{
  int n_items;
  int i;

  n_items = g_menu_model_get_n_items (menu);
  for (i = 0; i < n_items; i++)
    add_item (sl, menu, i, store);
}

static GListModel *
create_model (gpointer item,
              gpointer user_data)
{
  MenuItem *self = item;
  BobguiInspectorMenu *sl = user_data;
  GListStore *store;

  if (self->model == NULL)
    return NULL;

  store = g_list_store_new (menu_item_get_type ());
  add_menu (sl, self->model, store);

  return G_LIST_MODEL (store);
}

void
bobgui_inspector_menu_set_object (BobguiInspectorMenu *sl,
                               GObject          *object)
{
  BobguiWidget *stack;
  BobguiStackPage *page;
  GListStore *store;

  stack = bobgui_widget_get_parent (BOBGUI_WIDGET (sl));
  page = bobgui_stack_get_page (BOBGUI_STACK (stack), BOBGUI_WIDGET (sl));

  g_object_set (page, "visible", FALSE, NULL);

  store = G_LIST_STORE (bobgui_tree_list_model_get_model (sl->priv->tree_model));
  g_list_store_remove_all (store);

  if (G_IS_MENU_MODEL (object))
    {
      g_object_set (page, "visible", TRUE, NULL);
      add_menu (sl, G_MENU_MODEL (object), store);
    }
}

static void
bobgui_inspector_menu_class_init (BobguiInspectorMenuClass *klass)
{
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (klass);

  bobgui_widget_class_set_template_from_resource (widget_class, "/org/bobgui/libbobgui/inspector/menu.ui");
  bobgui_widget_class_bind_template_child_private (widget_class, BobguiInspectorMenu, view);
}

// vim: set et sw=2 ts=2:
