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

#include "size-groups.h"

#include "highlightoverlay.h"
#include "window.h"

#include "bobguibinlayout.h"
#include "bobguibox.h"
#include "bobguidropdown.h"
#include "bobguiframe.h"
#include "bobguilabel.h"
#include "bobguilistbox.h"
#include "bobguisizegroup.h"
#include "bobguiswitch.h"
#include "bobguiwidgetprivate.h"
#include "bobguistack.h"
#include "bobguistringlist.h"
#include "bobguiscrolledwindow.h"


typedef struct {
  BobguiListBoxRow parent;
  BobguiInspectorOverlay *highlight;
  BobguiWidget *widget;
} SizeGroupRow;

typedef struct {
  BobguiListBoxRowClass parent;
} SizeGroupRowClass;

enum {
  PROP_WIDGET = 1,
  LAST_PROPERTY
};

GParamSpec *properties[LAST_PROPERTY] = { NULL };

GType size_group_row_get_type (void);

G_DEFINE_TYPE (SizeGroupRow, size_group_row, BOBGUI_TYPE_LIST_BOX_ROW)

static void
size_group_row_init (SizeGroupRow *row)
{
}

static void
size_group_row_get_property (GObject    *object,
                             guint       property_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
  SizeGroupRow *row = (SizeGroupRow*)object;

  switch (property_id)
    {
    case PROP_WIDGET:
      g_value_set_pointer (value, row->widget);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
size_group_row_widget_destroyed (BobguiWidget *widget, SizeGroupRow *row)
{
  BobguiWidget *parent;

  parent = bobgui_widget_get_parent (BOBGUI_WIDGET (row));
  if (parent)
    bobgui_box_remove (BOBGUI_BOX (parent), BOBGUI_WIDGET (row));
}

static void
set_widget (SizeGroupRow *row, BobguiWidget *widget)
{
  if (row->widget)
    g_signal_handlers_disconnect_by_func (row->widget,
                                          size_group_row_widget_destroyed, row);

  row->widget = widget;

  if (row->widget)
    g_signal_connect (row->widget, "destroy",
                      G_CALLBACK (size_group_row_widget_destroyed), row);
}

static void
size_group_row_set_property (GObject      *object,
                             guint         property_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
  SizeGroupRow *row = (SizeGroupRow*)object;

  switch (property_id)
    {
    case PROP_WIDGET:
      set_widget (row, g_value_get_pointer (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}
static void
size_group_row_finalize (GObject *object)
{
  SizeGroupRow *row = (SizeGroupRow *)object;

  set_widget (row, NULL);

  G_OBJECT_CLASS (size_group_row_parent_class)->finalize (object);
}

static void
size_group_state_flags_changed (BobguiWidget     *widget,
                                BobguiStateFlags  old_state)
{
  SizeGroupRow *row = (SizeGroupRow*)widget;
  BobguiStateFlags state;

  if (!row->widget)
    return;

  state = bobgui_widget_get_state_flags (widget);
  if ((state & BOBGUI_STATE_FLAG_PRELIGHT) != (old_state & BOBGUI_STATE_FLAG_PRELIGHT))
    {
      BobguiInspectorWindow *iw = BOBGUI_INSPECTOR_WINDOW (bobgui_widget_get_root (widget));

      if (state & BOBGUI_STATE_FLAG_PRELIGHT)
        {
          row->highlight = bobgui_highlight_overlay_new (row->widget);
          bobgui_inspector_window_add_overlay (iw, row->highlight);
        }
      else
        {
          bobgui_inspector_window_remove_overlay (iw, row->highlight);
          g_clear_object (&row->highlight);
        }
    }
}

static void
size_group_row_class_init (SizeGroupRowClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (class);

  object_class->finalize = size_group_row_finalize;
  object_class->get_property = size_group_row_get_property;
  object_class->set_property = size_group_row_set_property;

  widget_class->state_flags_changed = size_group_state_flags_changed;

  properties[PROP_WIDGET] =
    g_param_spec_pointer ("widget", NULL, NULL, G_PARAM_READWRITE);

  g_object_class_install_properties (object_class, LAST_PROPERTY, properties);

}

G_DEFINE_TYPE (BobguiInspectorSizeGroups, bobgui_inspector_size_groups, BOBGUI_TYPE_WIDGET)

static void
clear_view (BobguiInspectorSizeGroups *sl)
{
  BobguiWidget *child;

  while ((child = bobgui_widget_get_first_child (BOBGUI_WIDGET (sl->groups))))
    bobgui_box_remove (BOBGUI_BOX (sl->groups), child);
}

static void
add_widget (BobguiInspectorSizeGroups *sl,
            BobguiListBox             *listbox,
            BobguiWidget              *widget)
{
  BobguiWidget *row;
  BobguiWidget *label;
  char *text;

  row = g_object_new (size_group_row_get_type (), "widget", widget, NULL);
  text = g_strdup_printf ("%p (%s)", widget, g_type_name_from_instance ((GTypeInstance*)widget));
  label = bobgui_label_new (text);
  g_free (text);
  bobgui_widget_set_margin_start (label, 10);
  bobgui_widget_set_margin_end (label, 10);
  bobgui_widget_set_margin_top (label, 10);
  bobgui_widget_set_margin_bottom (label, 10);
  bobgui_widget_set_halign (label, BOBGUI_ALIGN_START);
  bobgui_widget_set_valign (label, BOBGUI_ALIGN_BASELINE_FILL);
  bobgui_list_box_row_set_child (BOBGUI_LIST_BOX_ROW (row), label);
  bobgui_list_box_insert (listbox, row, -1);
}

static void
add_size_group (BobguiInspectorSizeGroups *sl,
                BobguiSizeGroup           *group)
{
  BobguiWidget *frame, *box, *box2;
  BobguiWidget *label, *dropdown;
  GSList *widgets, *l;
  BobguiWidget *listbox;
  const char *modes[5];

  modes[0] = C_("sizegroup mode", "None");
  modes[1] = C_("sizegroup mode", "Horizontal");
  modes[2] = C_("sizegroup mode", "Vertical");
  modes[3] = C_("sizegroup mode", "Both");
  modes[4] = NULL;

  frame = bobgui_frame_new (NULL);
  bobgui_box_append (BOBGUI_BOX (sl->groups), frame);
  box = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 0);
  bobgui_widget_add_css_class (box, "view");
  bobgui_frame_set_child (BOBGUI_FRAME (frame), box);

  box2 = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 10);
  bobgui_box_append (BOBGUI_BOX (box), box2);

  label = bobgui_label_new (_("Mode"));
  bobgui_widget_set_margin_start (label, 10);
  bobgui_widget_set_margin_end (label, 10);
  bobgui_widget_set_margin_top (label, 10);
  bobgui_widget_set_margin_bottom (label, 10);
  bobgui_widget_set_halign (label, BOBGUI_ALIGN_START);
  bobgui_widget_set_valign (label, BOBGUI_ALIGN_BASELINE_FILL);
  bobgui_box_append (BOBGUI_BOX (box2), label);

  dropdown = bobgui_drop_down_new_from_strings (modes);
  g_object_set (dropdown,
                "margin-start", 10,
                "margin-end", 10,
                "margin-top", 10,
                "margin-bottom", 10,
                NULL);
  bobgui_widget_set_halign (dropdown, BOBGUI_ALIGN_END);
  bobgui_widget_set_valign (dropdown, BOBGUI_ALIGN_BASELINE_FILL);
  g_object_bind_property (group, "mode",
                          dropdown, "selected",
                          G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);
  bobgui_box_append (BOBGUI_BOX (box2), dropdown);

  listbox = bobgui_list_box_new ();
  bobgui_box_append (BOBGUI_BOX (box), listbox);
  bobgui_list_box_set_selection_mode (BOBGUI_LIST_BOX (listbox), BOBGUI_SELECTION_NONE);

  widgets = bobgui_size_group_get_widgets (group);
  for (l = widgets; l; l = l->next)
    add_widget (sl, BOBGUI_LIST_BOX (listbox), BOBGUI_WIDGET (l->data));
}

void
bobgui_inspector_size_groups_set_object (BobguiInspectorSizeGroups *sl,
                                      GObject                *object)
{
  GSList *groups, *l;
  BobguiWidget *stack;
  BobguiStackPage *page;

  stack = bobgui_widget_get_parent (BOBGUI_WIDGET (sl));
  page = bobgui_stack_get_page (BOBGUI_STACK (stack), BOBGUI_WIDGET (sl));

  g_object_set (page, "visible", FALSE, NULL);

  clear_view (sl);

  if (!BOBGUI_IS_WIDGET (object))
    return;

  groups = _bobgui_widget_get_sizegroups (BOBGUI_WIDGET (object));
  if (groups)
    g_object_set (page, "visible", TRUE, NULL);
  for (l = groups; l; l = l->next)
    {
      BobguiSizeGroup *group = l->data;
      add_size_group (sl, group);
    }
}

static void
bobgui_inspector_size_groups_init (BobguiInspectorSizeGroups *sl)
{
  sl->sw = bobgui_scrolled_window_new ();
  bobgui_widget_set_parent (sl->sw, BOBGUI_WIDGET (sl));
  sl->groups = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 10);
  g_object_set (sl->groups,
                "margin-start", 60,
                "margin-end", 60,
                "margin-top", 30,
                "margin-bottom", 30,
                NULL);
  bobgui_scrolled_window_set_child (BOBGUI_SCROLLED_WINDOW (sl->sw), sl->groups);
}

static void
bobgui_inspector_size_groups_dispose (GObject *object)
{
  BobguiInspectorSizeGroups *sl = BOBGUI_INSPECTOR_SIZE_GROUPS (object);

  g_clear_pointer (&sl->sw, bobgui_widget_unparent);

  G_OBJECT_CLASS (bobgui_inspector_size_groups_parent_class)->dispose (object);
}

static void
bobgui_inspector_size_groups_class_init (BobguiInspectorSizeGroupsClass *klass)
{
  G_OBJECT_CLASS (klass)->dispose = bobgui_inspector_size_groups_dispose;

  bobgui_widget_class_set_layout_manager_type (BOBGUI_WIDGET_CLASS (klass), BOBGUI_TYPE_BIN_LAYOUT);
}

// vim: set et sw=2 ts=2:
