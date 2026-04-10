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

#include "controllers.h"

#include "bobguibinlayout.h"
#include "bobguidropdown.h"
#include "bobguibox.h"
#include "bobguicustomsorter.h"
#include "bobguigesture.h"
#include "bobguilabel.h"
#include "bobguiscrolledwindow.h"
#include "bobguisortlistmodel.h"
#include "bobguistack.h"
#include "bobguiwidgetprivate.h"
#include "window.h"
#include "bobguisignallistitemfactory.h"
#include "bobguicolumnview.h"
#include "bobguicolumnviewcolumn.h"
#include "bobguilistitem.h"
#include "bobguinoselection.h"

struct _BobguiInspectorControllers
{
  BobguiWidget parent_instance;

  BobguiWidget *view;
};

struct _BobguiInspectorControllersClass
{
  BobguiWidgetClass parent_class;
};

G_DEFINE_TYPE (BobguiInspectorControllers, bobgui_inspector_controllers, BOBGUI_TYPE_WIDGET)

static void
row_activated (BobguiColumnView           *view,
               guint                    position,
               BobguiInspectorControllers *self)
{
  BobguiInspectorWindow *iw;
  GListModel *model;
  GObject *controller;

  iw = BOBGUI_INSPECTOR_WINDOW (bobgui_widget_get_ancestor (BOBGUI_WIDGET (self), BOBGUI_TYPE_INSPECTOR_WINDOW));

  model = G_LIST_MODEL (bobgui_column_view_get_model (view));
  controller = g_list_model_get_item (model, position);
  bobgui_inspector_window_push_object (iw, controller, CHILD_KIND_CONTROLLER, 0);
  g_object_unref (controller);
}

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
bind_type (BobguiSignalListItemFactory *factory,
           BobguiListItem              *list_item,
           gpointer                  data)
{
  BobguiWidget *label;
  BobguiEventController *controller;

  label = bobgui_list_item_get_child (list_item);
  controller = bobgui_list_item_get_item (list_item);

  bobgui_label_set_label (BOBGUI_LABEL (label), G_OBJECT_TYPE_NAME (controller));
}

static void
bind_name (BobguiSignalListItemFactory *factory,
           BobguiListItem              *list_item,
           gpointer                  data)
{
  BobguiWidget *label;
  BobguiEventController *controller;

  label = bobgui_list_item_get_child (list_item);
  controller = bobgui_list_item_get_item (list_item);

  bobgui_label_set_label (BOBGUI_LABEL (label), bobgui_event_controller_get_name (controller));
}

static void
bind_phase (BobguiSignalListItemFactory *factory,
            BobguiListItem              *list_item,
            gpointer                  data)
{
  BobguiWidget *label;
  BobguiEventController *controller;
  const char *name;

  label = bobgui_list_item_get_child (list_item);
  controller = bobgui_list_item_get_item (list_item);

  switch (bobgui_event_controller_get_propagation_phase (controller))
    {
    case BOBGUI_PHASE_NONE:
      name = C_("event phase", "None");
      break;
    case BOBGUI_PHASE_CAPTURE:
      name = C_("event phase", "Capture");
      break;
    case BOBGUI_PHASE_BUBBLE:
      name = C_("event phase", "Bubble");
      break;
    case BOBGUI_PHASE_TARGET:
      name = C_("event phase", "Target");
      break;
    default:
      g_assert_not_reached ();
    }

  bobgui_label_set_label (BOBGUI_LABEL (label), name);
}

static void
bind_limit (BobguiSignalListItemFactory *factory,
            BobguiListItem              *list_item,
            gpointer                  data)
{
  BobguiWidget *label;
  BobguiEventController *controller;

  label = bobgui_list_item_get_child (list_item);
  controller = bobgui_list_item_get_item (list_item);

  if (bobgui_event_controller_get_propagation_limit (controller) == BOBGUI_LIMIT_SAME_NATIVE)
    bobgui_label_set_label (BOBGUI_LABEL (label), C_("propagation limit", "Native"));
  else
    bobgui_label_set_label (BOBGUI_LABEL (label), "");
}

static void
bobgui_inspector_controllers_init (BobguiInspectorControllers *self)
{
  BobguiWidget *sw;
  BobguiListItemFactory *factory;
  BobguiColumnViewColumn *column;

  sw = bobgui_scrolled_window_new ();

  self->view = bobgui_column_view_new (NULL);

  factory = bobgui_signal_list_item_factory_new ();
  g_signal_connect (factory, "setup", G_CALLBACK (setup_row), NULL);
  g_signal_connect (factory, "bind", G_CALLBACK (bind_type), NULL);

  column = bobgui_column_view_column_new ("Type", factory);
  bobgui_column_view_append_column (BOBGUI_COLUMN_VIEW (self->view), column);
  g_object_unref (column);

  factory = bobgui_signal_list_item_factory_new ();
  g_signal_connect (factory, "setup", G_CALLBACK (setup_row), NULL);
  g_signal_connect (factory, "bind", G_CALLBACK (bind_name), NULL);

  column = bobgui_column_view_column_new ("Name", factory);
  bobgui_column_view_column_set_expand (column, TRUE);
  bobgui_column_view_append_column (BOBGUI_COLUMN_VIEW (self->view), column);
  g_object_unref (column);

  factory = bobgui_signal_list_item_factory_new ();
  g_signal_connect (factory, "setup", G_CALLBACK (setup_row), NULL);
  g_signal_connect (factory, "bind", G_CALLBACK (bind_phase), NULL);

  column = bobgui_column_view_column_new ("Phase", factory);
  bobgui_column_view_append_column (BOBGUI_COLUMN_VIEW (self->view), column);
  g_object_unref (column);

  factory = bobgui_signal_list_item_factory_new ();
  g_signal_connect (factory, "setup", G_CALLBACK (setup_row), NULL);
  g_signal_connect (factory, "bind", G_CALLBACK (bind_limit), NULL);

  column = bobgui_column_view_column_new ("Limit", factory);
  bobgui_column_view_append_column (BOBGUI_COLUMN_VIEW (self->view), column);
  g_object_unref (column);

  g_signal_connect (self->view, "activate", G_CALLBACK (row_activated), self);

  bobgui_scrolled_window_set_child (BOBGUI_SCROLLED_WINDOW (sw), self->view);

  bobgui_widget_set_parent (sw, BOBGUI_WIDGET (self));
}

static int
compare_phases (BobguiPropagationPhase first,
                BobguiPropagationPhase second)
{
  int priorities[] = {
    [BOBGUI_PHASE_NONE] = 0,
    [BOBGUI_PHASE_CAPTURE] = 1,
    [BOBGUI_PHASE_BUBBLE] = 3,
    [BOBGUI_PHASE_TARGET] = 2
  };

  return priorities[first] - priorities[second];
}

static int
compare_controllers (gconstpointer _first,
                     gconstpointer _second,
                     gpointer      unused)
{
  BobguiEventController *first = BOBGUI_EVENT_CONTROLLER (_first);
  BobguiEventController *second = BOBGUI_EVENT_CONTROLLER (_second);
  BobguiPropagationPhase first_phase, second_phase;
  BobguiWidget *first_widget, *second_widget;
  int result;

  
  first_phase = bobgui_event_controller_get_propagation_phase (first);
  second_phase = bobgui_event_controller_get_propagation_phase (second);
  result = compare_phases (first_phase, second_phase);
  if (result != 0)
    return result;

  first_widget = bobgui_event_controller_get_widget (first);
  second_widget = bobgui_event_controller_get_widget (second);
  if (first_widget == second_widget)
    return 0;

  if (bobgui_widget_is_ancestor (first_widget, second_widget))
    result = -1;
  else
    result = 1;

  if (first_phase == BOBGUI_PHASE_BUBBLE)
    result = -result;

  return result;
}

void
bobgui_inspector_controllers_set_object (BobguiInspectorControllers *self,
                                      GObject                 *object)
{
  BobguiWidget *stack;
  BobguiStackPage *page;
  GListModel *model;
  BobguiSortListModel *sort_model;
  BobguiSorter *sorter;
  BobguiNoSelection *no_selection;

  stack = bobgui_widget_get_parent (BOBGUI_WIDGET (self));
  page = bobgui_stack_get_page (BOBGUI_STACK (stack), BOBGUI_WIDGET (self));

  if (!BOBGUI_IS_WIDGET (object))
    {
      bobgui_column_view_set_model (BOBGUI_COLUMN_VIEW (self->view), NULL);
      g_object_set (page, "visible", FALSE, NULL);
      return;
    }

  g_object_set (page, "visible", TRUE, NULL);

  model = bobgui_widget_observe_controllers (BOBGUI_WIDGET (object));
  sorter = BOBGUI_SORTER (bobgui_custom_sorter_new (compare_controllers, NULL, NULL));
  sort_model = bobgui_sort_list_model_new (model, sorter);
  no_selection = bobgui_no_selection_new (G_LIST_MODEL (sort_model));

  bobgui_column_view_set_model (BOBGUI_COLUMN_VIEW (self->view), BOBGUI_SELECTION_MODEL (no_selection));

  g_object_unref (no_selection);
}

static void
bobgui_inspector_controllers_dispose (GObject *object)
{
  BobguiInspectorControllers *self = BOBGUI_INSPECTOR_CONTROLLERS (object);

  bobgui_widget_unparent (bobgui_widget_get_first_child (BOBGUI_WIDGET (self)));

  G_OBJECT_CLASS (bobgui_inspector_controllers_parent_class)->dispose (object);
}

static void
bobgui_inspector_controllers_class_init (BobguiInspectorControllersClass *klass)
{
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (klass);
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = bobgui_inspector_controllers_dispose;

  bobgui_widget_class_set_layout_manager_type (widget_class, BOBGUI_TYPE_BIN_LAYOUT);
}

// vim: set et sw=2 ts=2:
