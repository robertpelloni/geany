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

#include "shortcuts.h"
#include "bobguilabel.h"
#include "bobguisizegroup.h"
#include "bobguistack.h"
#include "bobguishortcut.h"
#include "bobguishortcuttrigger.h"
#include "bobguishortcutcontroller.h"
#include "bobguisignallistitemfactory.h"
#include "bobguilistitem.h"
#include "bobguicolumnview.h"
#include "bobguicolumnviewcolumn.h"
#include "bobguiscrolledwindow.h"
#include "bobguinoselection.h"
#include "bobguibinlayout.h"


struct _BobguiInspectorShortcuts
{
  BobguiWidget parent;

  BobguiWidget *view;
};

G_DEFINE_TYPE (BobguiInspectorShortcuts, bobgui_inspector_shortcuts, BOBGUI_TYPE_WIDGET)

static void
setup_row (BobguiSignalListItemFactory *factory,
           BobguiListItem              *list_item,
           gpointer                  data)
{
  BobguiWidget *label;

  label = bobgui_label_new ("");
  bobgui_label_set_xalign (BOBGUI_LABEL (label), 0);
  bobgui_list_item_set_child (list_item, label);
}

static void
bind_trigger (BobguiSignalListItemFactory *factory,
              BobguiListItem              *list_item,
              gpointer                  data)
{
  BobguiWidget *label;
  BobguiShortcut *shortcut;
  BobguiShortcutTrigger *trigger;
  char *str;

  label = bobgui_list_item_get_child (list_item);
  shortcut = bobgui_list_item_get_item (list_item);
  trigger = bobgui_shortcut_get_trigger (shortcut);
  str = bobgui_shortcut_trigger_to_label (trigger, bobgui_widget_get_display (label));
  bobgui_label_set_label (BOBGUI_LABEL (label), str);
  g_free (str);
}

static void
bind_action (BobguiSignalListItemFactory *factory,
             BobguiListItem              *list_item,
             gpointer                  data)
{
  BobguiWidget *label;
  BobguiShortcut *shortcut;
  BobguiShortcutAction *action;
  char *str;

  label = bobgui_list_item_get_child (list_item);
  shortcut = bobgui_list_item_get_item (list_item);
  action = bobgui_shortcut_get_action (shortcut);
  str = bobgui_shortcut_action_to_string (action);
  bobgui_label_set_label (BOBGUI_LABEL (label), str);
  g_free (str);
}

static void
bobgui_inspector_shortcuts_init (BobguiInspectorShortcuts *self)
{
  BobguiWidget *sw;
  BobguiListItemFactory *factory;
  BobguiColumnViewColumn *column;

  sw = bobgui_scrolled_window_new ();

  self->view = bobgui_column_view_new (NULL);

  factory = bobgui_signal_list_item_factory_new ();
  g_signal_connect (factory, "setup", G_CALLBACK (setup_row), NULL);
  g_signal_connect (factory, "bind", G_CALLBACK (bind_trigger), NULL);

  column = bobgui_column_view_column_new ("Trigger", factory);
  bobgui_column_view_append_column (BOBGUI_COLUMN_VIEW (self->view), column);
  g_object_unref (column);

  factory = bobgui_signal_list_item_factory_new ();
  g_signal_connect (factory, "setup", G_CALLBACK (setup_row), NULL);
  g_signal_connect (factory, "bind", G_CALLBACK (bind_action), NULL);

  column = bobgui_column_view_column_new ("Action", factory);
  bobgui_column_view_append_column (BOBGUI_COLUMN_VIEW (self->view), column);
  g_object_unref (column);

  bobgui_scrolled_window_set_child (BOBGUI_SCROLLED_WINDOW (sw), self->view);

  bobgui_widget_set_parent (sw, BOBGUI_WIDGET (self));
}

void
bobgui_inspector_shortcuts_set_object (BobguiInspectorShortcuts *self,
                                    GObject               *object)
{
  BobguiWidget *stack;
  BobguiStackPage *page;
  BobguiNoSelection *no_selection;

  stack = bobgui_widget_get_parent (BOBGUI_WIDGET (self));
  page = bobgui_stack_get_page (BOBGUI_STACK (stack), BOBGUI_WIDGET (self));

  if (!BOBGUI_IS_SHORTCUT_CONTROLLER (object))
    {
      bobgui_column_view_set_model (BOBGUI_COLUMN_VIEW (self->view), NULL);
      g_object_set (page, "visible", FALSE, NULL);
      return;
    }

  g_object_set (page, "visible", TRUE, NULL);

  no_selection = bobgui_no_selection_new (g_object_ref (G_LIST_MODEL (object)));
  bobgui_column_view_set_model (BOBGUI_COLUMN_VIEW (self->view), BOBGUI_SELECTION_MODEL (no_selection));
  g_object_unref (no_selection);
}

static void
dispose (GObject *object)
{
  BobguiInspectorShortcuts *self = BOBGUI_INSPECTOR_SHORTCUTS (object);

  bobgui_widget_unparent (bobgui_widget_get_first_child (BOBGUI_WIDGET (self)));

  G_OBJECT_CLASS (bobgui_inspector_shortcuts_parent_class)->dispose (object);
}

static void
bobgui_inspector_shortcuts_class_init (BobguiInspectorShortcutsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (klass);

  object_class->dispose = dispose;

  bobgui_widget_class_set_layout_manager_type (widget_class, BOBGUI_TYPE_BIN_LAYOUT);
}
