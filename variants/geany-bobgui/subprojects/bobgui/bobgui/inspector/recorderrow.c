/*
 * Copyright (c) 2021 Red Hat, Inc.
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

#include "recorderrow.h"

#include <bobgui/bobguibinlayout.h>

/* This is a minimal widget whose purpose it is to compare the event sequence
 * of its row in the recordings list with the event sequence of the selected
 * row, and highlight itself if they match.
 */

struct _BobguiInspectorRecorderRow
{
  BobguiWidget parent;

  gpointer sequence;
  gpointer match_sequence;
};

enum {
  PROP_SEQUENCE = 1,
  PROP_MATCH_SEQUENCE,
  LAST_PROP
};

static GParamSpec *props[LAST_PROP] = { NULL, };

G_DEFINE_TYPE (BobguiInspectorRecorderRow, bobgui_inspector_recorder_row, BOBGUI_TYPE_WIDGET)

static void
bobgui_inspector_recorder_row_init (BobguiInspectorRecorderRow *self)
{
}

static void
dispose (GObject *object)
{
  BobguiInspectorRecorderRow *self = BOBGUI_INSPECTOR_RECORDER_ROW (object);

  bobgui_widget_unparent (bobgui_widget_get_first_child (BOBGUI_WIDGET (self)));

  G_OBJECT_CLASS (bobgui_inspector_recorder_row_parent_class)->dispose (object);
}

static void
update_style (BobguiInspectorRecorderRow *self)
{
  if (self->sequence == self->match_sequence && self->sequence != NULL)
    bobgui_widget_add_css_class (BOBGUI_WIDGET (self), "highlight");
  else
    bobgui_widget_remove_css_class (BOBGUI_WIDGET (self), "highlight");
}

static void
bobgui_inspector_recorder_row_get_property (GObject    *object,
                                         guint       param_id,
                                         GValue     *value,
                                         GParamSpec *pspec)
{
  BobguiInspectorRecorderRow *self = BOBGUI_INSPECTOR_RECORDER_ROW (object);

  switch (param_id)
    {
    case PROP_SEQUENCE:
      g_value_set_pointer (value, self->sequence);
      break;

    case PROP_MATCH_SEQUENCE:
      g_value_set_pointer (value, self->match_sequence);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
    }
}

static void
bobgui_inspector_recorder_row_set_property (GObject      *object,
                                         guint         param_id,
                                         const GValue *value,
                                         GParamSpec   *pspec)
{
  BobguiInspectorRecorderRow *self = BOBGUI_INSPECTOR_RECORDER_ROW (object);

  switch (param_id)
    {
    case PROP_SEQUENCE:
      self->sequence = g_value_get_pointer (value);
      update_style (self);
      break;

    case PROP_MATCH_SEQUENCE:
      self->match_sequence = g_value_get_pointer (value);
      update_style (self);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
    }
}

static void
bobgui_inspector_recorder_row_class_init (BobguiInspectorRecorderRowClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (klass);

  object_class->dispose = dispose;
  object_class->set_property = bobgui_inspector_recorder_row_set_property;
  object_class->get_property = bobgui_inspector_recorder_row_get_property;

  props[PROP_SEQUENCE] = g_param_spec_pointer ("sequence", NULL, NULL, G_PARAM_READWRITE);
  props[PROP_MATCH_SEQUENCE] = g_param_spec_pointer ("match-sequence", NULL, NULL, G_PARAM_READWRITE);

  g_object_class_install_properties (object_class, LAST_PROP, props);

  bobgui_widget_class_set_layout_manager_type (widget_class, BOBGUI_TYPE_BIN_LAYOUT);
}
