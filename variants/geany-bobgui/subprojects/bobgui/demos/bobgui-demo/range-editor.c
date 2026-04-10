/*
 * Copyright © 2025 Red Hat, Inc
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 * Authors: Matthias Clasen <mclasen@redhat.com>
 */

#include "range-editor.h"

/* {{{ Gizmo */

#define GIZMO_TYPE (gizmo_get_type ())
G_DECLARE_FINAL_TYPE (Gizmo, gizmo, GIZ, MO, BobguiWidget)

typedef void    (* GizmoMeasureFunc)   (Gizmo          *gizmo,
                                        BobguiOrientation  orientation,
                                        int             for_size,
                                        int            *minimum,
                                        int            *natural,
                                        int            *minimum_baseline,
                                        int            *natural_baseline);
typedef void    (* GizmoAllocateFunc)  (Gizmo          *gizmo,
                                        int             width,
                                        int             height,
                                        int             baseline);
typedef void    (* GizmoSnapshotFunc)  (Gizmo          *gizmo,
                                        BobguiSnapshot    *snapshot);

struct _Gizmo
{
  BobguiWidget parent_instance;

  GizmoMeasureFunc   measure_func;
  GizmoAllocateFunc  allocate_func;
  GizmoSnapshotFunc  snapshot_func;
};

struct _GizmoClass
{
  BobguiWidgetClass parent_class;
};

G_DEFINE_TYPE (Gizmo, gizmo, BOBGUI_TYPE_WIDGET);

static void
gizmo_measure (BobguiWidget      *widget,
               BobguiOrientation  orientation,
               int             for_size,
               int            *minimum,
               int            *natural,
               int            *minimum_baseline,
               int            *natural_baseline)
{
  Gizmo *self = GIZ_MO (widget);

  if (self->measure_func)
    self->measure_func (self, orientation, for_size,
                        minimum, natural,
                        minimum_baseline, natural_baseline);
}

static void
gizmo_size_allocate (BobguiWidget *widget,
                     int        width,
                     int        height,
                     int        baseline)
{
  Gizmo *self = GIZ_MO (widget);

  if (self->allocate_func)
    self->allocate_func (self, width, height, baseline);
}

static void
gizmo_snapshot (BobguiWidget   *widget,
                BobguiSnapshot *snapshot)
{
  Gizmo *self = GIZ_MO (widget);

  if (self->snapshot_func)
    self->snapshot_func (self, snapshot);
  else
    BOBGUI_WIDGET_CLASS (gizmo_parent_class)->snapshot (widget, snapshot);
}

static void
gizmo_finalize (GObject *object)
{
  Gizmo *self = GIZ_MO (object);
  BobguiWidget *child;

  while ((child = bobgui_widget_get_first_child (BOBGUI_WIDGET (self))) != NULL)
    bobgui_widget_unparent (child);

  G_OBJECT_CLASS (gizmo_parent_class)->finalize (object);
}

static void
gizmo_class_init (GizmoClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (klass);

  object_class->finalize = gizmo_finalize;

  widget_class->measure = gizmo_measure;
  widget_class->size_allocate = gizmo_size_allocate;
  widget_class->snapshot = gizmo_snapshot;
}

static void
gizmo_init (Gizmo *self)
{
}

static BobguiWidget *
gizmo_new (const char        *css_name,
           GizmoMeasureFunc   measure_func,
           GizmoAllocateFunc  allocate_func,
           GizmoSnapshotFunc  snapshot_func)
{
  Gizmo *gizmo;

  gizmo = g_object_new (GIZMO_TYPE,
                        "css-name", css_name,
                        "accessible-role", BOBGUI_ACCESSIBLE_ROLE_NONE,
                        NULL);

  gizmo->measure_func  = measure_func;
  gizmo->allocate_func = allocate_func;
  gizmo->snapshot_func = snapshot_func;

  return BOBGUI_WIDGET (gizmo);
}

/* }}} */

struct _RangeEditor
{
  BobguiWidget parent_instance;

  float lower;
  float upper;
  float value1;
  float value2;

  BobguiWidget *trough;
  BobguiWidget *highlight;
  BobguiWidget *slider1;
  BobguiWidget *slider2;

  BobguiGesture *drag_gesture;
  BobguiWidget *mouse_location;
};

enum
{
  PROP_LOWER = 1,
  PROP_UPPER,
  PROP_VALUE1,
  PROP_VALUE2,
  NUM_PROPERTIES,
};

static GParamSpec *properties[NUM_PROPERTIES];

struct _RangeEditorClass
{
  BobguiWidgetClass parent_class;
};

G_DEFINE_TYPE (RangeEditor, range_editor, BOBGUI_TYPE_WIDGET)

/* {{{ Setters */

static void
range_editor_set_values (RangeEditor *self,
                         float        value1,
                         float        value2)
{
  g_return_if_fail (self->lower <= value1);
  g_return_if_fail (value1 <= value2);
  g_return_if_fail (value2 <= self->upper);

  if (self->value1 == value1 && self->value2 == value2)
    return;

  g_object_freeze_notify (G_OBJECT (self));

  if (self->value1 != value1)
    {
      self->value1 = value1;
      g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_VALUE1]);
    }

  if (self->value2 != value2)
    {
      self->value2 = value2;
      g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_VALUE2]);
    }

  g_object_thaw_notify (G_OBJECT (self));
  bobgui_widget_queue_resize (self->highlight);
  bobgui_widget_queue_resize (self->slider1);
  bobgui_widget_queue_resize (self->slider2);
}

static void
range_editor_set_limits (RangeEditor *self,
                         float        lower,
                         float        upper)
{
  g_return_if_fail (lower <= upper);

  if (self->lower == lower && self->upper == upper)
    return;

  g_object_freeze_notify (G_OBJECT (self));

  if (self->lower != lower)
    {
      self->lower = lower;
      g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_LOWER]);
    }

  if (self->upper != upper)
    {
      self->upper = upper;
      g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_UPPER]);
    }

  range_editor_set_values (self,
                           CLAMP (self->value1, self->lower, self->upper),
                           CLAMP (self->value2, self->lower, self->upper));

  g_object_thaw_notify (G_OBJECT (self));
  bobgui_widget_queue_resize (self->highlight);
}

/* }}} */
/* {{{ Utilities */

static void
measure_widget (BobguiWidget      *widget,
                BobguiOrientation  orientation,
                int            *min,
                int            *nat)
{
  bobgui_widget_measure (widget, orientation, -1, min, nat, NULL, NULL);
}

/* }}} */
/* {{{ Trough */

static void
measure_trough (Gizmo          *gizmo,
                BobguiOrientation  orientation,
                int             for_size,
                int            *minimum,
                int            *natural,
                int            *minimum_baseline,
                int            *natural_baseline)
{
  RangeEditor *self = RANGE_EDITOR (bobgui_widget_get_parent (BOBGUI_WIDGET (gizmo)));
  int min, nat;

  measure_widget (self->highlight, orientation, minimum, natural);
  measure_widget (self->slider1, orientation, &min, &nat);

  if (orientation == BOBGUI_ORIENTATION_HORIZONTAL)
    {
      *minimum += min;
      *natural += nat;
    }
  else
    {
      *minimum = MAX (*minimum, min);
      *natural = MAX (*natural, nat);
    }

  measure_widget (self->slider2, orientation, &min, &nat);

  if (orientation == BOBGUI_ORIENTATION_HORIZONTAL)
    {
      *minimum += min;
      *natural += nat;
    }
  else
    {
      *minimum = MAX (*minimum, min);
      *natural = MAX (*natural, nat);
    }
}

static void
allocate_trough (Gizmo *gizmo,
                 int    width,
                 int    height,
                 int    baseline)
{
  RangeEditor *self = RANGE_EDITOR (bobgui_widget_get_parent (BOBGUI_WIDGET (gizmo)));
  BobguiAllocation alloc;
  float x1, x2;
  int slider_width, slider_height;

  x1 = width * (self->value1 - self->lower) / (self->upper - self->lower);
  x2 = width * (self->value2 - self->lower) / (self->upper - self->lower);

  alloc.x = x1;
  alloc.y = 0;
  alloc.width = x2 - x1;
  alloc.height = height;

  bobgui_widget_size_allocate (self->highlight, &alloc, -1);

  measure_widget (self->slider1, BOBGUI_ORIENTATION_HORIZONTAL, &slider_width, NULL);
  measure_widget (self->slider1, BOBGUI_ORIENTATION_VERTICAL, &slider_height, NULL);

  alloc.x = x1 - slider_width;
  alloc.y = (height - slider_height) / 2;
  alloc.width = slider_width;
  alloc.height = slider_height;

  bobgui_widget_size_allocate (self->slider1, &alloc, -1);

  measure_widget (self->slider2, BOBGUI_ORIENTATION_HORIZONTAL, &slider_width, NULL);
  measure_widget (self->slider2, BOBGUI_ORIENTATION_VERTICAL, &slider_height, NULL);

  alloc.x = x2;
  alloc.y = (height - slider_height) / 2;
  alloc.width = slider_width;
  alloc.height = slider_height;

  bobgui_widget_size_allocate (self->slider2, &alloc, -1);
}

static void
render_trough (Gizmo       *gizmo,
               BobguiSnapshot *snapshot)
{
  RangeEditor *self = RANGE_EDITOR (bobgui_widget_get_parent (BOBGUI_WIDGET (gizmo)));

  bobgui_widget_snapshot_child (self->trough, self->highlight, snapshot);
  bobgui_widget_snapshot_child (self->trough, self->slider1, snapshot);
  bobgui_widget_snapshot_child (self->trough, self->slider2, snapshot);
}

/* }}} */
/* {{{ Input */

static void
drag_gesture_begin (BobguiGestureDrag *gesture,
                    double          offset_x,
                    double          offset_y,
                    RangeEditor    *self)
{
  self->mouse_location = bobgui_widget_pick (BOBGUI_WIDGET (self), offset_x, offset_y, 0);

  if (self->mouse_location == self->slider1 ||
      self->mouse_location == self->slider2)
    bobgui_gesture_set_state (self->drag_gesture, BOBGUI_EVENT_SEQUENCE_CLAIMED);
}

static void
drag_gesture_end (BobguiGestureDrag *gesture,
                  double          offset_x,
                  double          offset_y,
                  RangeEditor    *self)
{
  self->mouse_location = NULL;
}

static void
drag_gesture_update (BobguiGestureDrag *gesture,
                     double          offset_x,
                     double          offset_y,
                     RangeEditor    *self)
{
  double start_x, start_y;

  if (self->mouse_location == self->slider1 ||
      self->mouse_location == self->slider2)
    {
      int mouse_x;
      float v;
      graphene_rect_t bounds;

      bobgui_gesture_drag_get_start_point (gesture, &start_x, &start_y);
      mouse_x = start_x + offset_x;

      if (!bobgui_widget_compute_bounds (self->trough, BOBGUI_WIDGET (self), &bounds))
        g_assert_not_reached ();

      v = self->lower + (mouse_x - bounds.origin.x) / bounds.size.width * (self->upper - self->lower);

      if (self->mouse_location == self->slider1)
        {
          v = CLAMP (v, self->lower, self->upper);
          range_editor_set_values (self, MIN (v, self->value2), self->value2);
        }
      else if (self->mouse_location == self->slider2)
        {
          v = CLAMP (v, self->lower, self->upper);
          range_editor_set_values (self, self->value1, MAX (self->value1, v));
        }
    }
}

/* }}} */
/* {{{ BobguiWidget implementation */

static void
range_editor_measure (BobguiWidget      *widget,
                      BobguiOrientation  orientation,
                      int             for_size,
                      int            *minimum,
                      int            *natural,
                      int            *minimum_baseline,
                      int            *natural_baseline)
{
  RangeEditor *self = RANGE_EDITOR (widget);

  measure_widget (self->trough, orientation, minimum, natural);
}

static void
range_editor_size_allocate (BobguiWidget *widget,
                            int        width,
                            int        height,
                            int        baseline)
{
  RangeEditor *self = RANGE_EDITOR (widget);
  BobguiAllocation alloc;
  int min_height;
  int slider1_width, slider2_width;

  measure_widget (self->slider1, BOBGUI_ORIENTATION_HORIZONTAL, &slider1_width, NULL);
  measure_widget (self->slider2, BOBGUI_ORIENTATION_HORIZONTAL, &slider2_width, NULL);
  measure_widget (self->trough, BOBGUI_ORIENTATION_VERTICAL, &min_height, NULL);

  alloc.x = slider1_width;
  alloc.y = (height - min_height) / 2;
  alloc.width = width - slider1_width - slider2_width;
  alloc.height = min_height;

  bobgui_widget_size_allocate (self->trough, &alloc, -1);
}

/* }}} */
/* {{{ GObject boilerplate */

static void
range_editor_init (RangeEditor *self)
{
  self->lower = 0;
  self->upper = 0;
  self->value1 = 0;
  self->value2 = 0;

  self->trough = gizmo_new ("trough",
                            measure_trough,
                            allocate_trough,
                            render_trough);
  bobgui_widget_set_parent (self->trough, BOBGUI_WIDGET (self));

  self->highlight = gizmo_new ("highlight", NULL, NULL, NULL);
  bobgui_widget_set_parent (self->highlight, self->trough);

  self->slider1 = gizmo_new ("slider", NULL, NULL, NULL);
  bobgui_widget_set_parent (self->slider1, self->trough);
  bobgui_widget_add_css_class (self->slider1, "left");

  self->slider2 = gizmo_new ("slider", NULL, NULL, NULL);
  bobgui_widget_set_parent (self->slider2, self->trough);
  bobgui_widget_add_css_class (self->slider2, "right");

  self->drag_gesture = bobgui_gesture_drag_new ();
  bobgui_gesture_single_set_button (BOBGUI_GESTURE_SINGLE (self->drag_gesture), 0);
  g_signal_connect (self->drag_gesture, "drag-begin",
                    G_CALLBACK (drag_gesture_begin), self);
  g_signal_connect (self->drag_gesture, "drag-update",
                    G_CALLBACK (drag_gesture_update), self);
  g_signal_connect (self->drag_gesture, "drag-end",
                    G_CALLBACK (drag_gesture_end), self);

  bobgui_widget_add_controller (BOBGUI_WIDGET (self), BOBGUI_EVENT_CONTROLLER (self->drag_gesture));
}

static void
range_editor_set_property (GObject      *object,
                          guint         prop_id,
                          const GValue *value,
                          GParamSpec   *pspec)
{
  RangeEditor *self = RANGE_EDITOR (object);
  float v;

  switch (prop_id)
    {
    case PROP_LOWER:
      v = g_value_get_float (value);
      range_editor_set_limits (self, v, MAX (v, self->upper));
      break;

    case PROP_UPPER:
      v = g_value_get_float (value);
      range_editor_set_limits (self, MIN (v, self->lower), v);
      break;

    case PROP_VALUE1:
      v = g_value_get_float (value);
      range_editor_set_values (self, v, MAX (v, self->value2));
      break;

    case PROP_VALUE2:
      v = g_value_get_float (value);
      range_editor_set_values (self, MIN (v, self->value1), v);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
range_editor_get_property (GObject    *object,
                           guint       prop_id,
                           GValue     *value,
                           GParamSpec *pspec)
{
  RangeEditor *self = RANGE_EDITOR (object);

  switch (prop_id)
    {
    case PROP_LOWER:
      g_value_set_float (value, self->lower);
      break;

    case PROP_UPPER:
      g_value_set_float (value, self->upper);
      break;

    case PROP_VALUE1:
      g_value_set_float (value, self->value1);
      break;

    case PROP_VALUE2:
      g_value_set_float (value, self->value2);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
range_editor_dispose (GObject *object)
{
  RangeEditor *self = RANGE_EDITOR (object);

  g_clear_pointer (&self->highlight, bobgui_widget_unparent);
  g_clear_pointer (&self->slider1, bobgui_widget_unparent);
  g_clear_pointer (&self->slider2, bobgui_widget_unparent);
  g_clear_pointer (&self->trough, bobgui_widget_unparent);

  G_OBJECT_CLASS (range_editor_parent_class)->dispose (object);
}

static void
range_editor_finalize (GObject *object)
{
//  RangeEditor *self = RANGE_EDITOR (object);

  G_OBJECT_CLASS (range_editor_parent_class)->finalize (object);
}

static void
range_editor_class_init (RangeEditorClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (class);

  object_class->set_property = range_editor_set_property;
  object_class->get_property = range_editor_get_property;
  object_class->dispose = range_editor_dispose;
  object_class->finalize = range_editor_finalize;

  widget_class->measure = range_editor_measure;
  widget_class->size_allocate = range_editor_size_allocate;

  properties[PROP_LOWER] =
    g_param_spec_float ("lower", NULL, NULL,
                        -G_MAXFLOAT, G_MAXFLOAT, 0,
                        G_PARAM_READWRITE | G_PARAM_STATIC_NAME | G_PARAM_EXPLICIT_NOTIFY);

  properties[PROP_UPPER] =
    g_param_spec_float ("upper", NULL, NULL,
                        -G_MAXFLOAT, G_MAXFLOAT, 0,
                        G_PARAM_READWRITE | G_PARAM_STATIC_NAME | G_PARAM_EXPLICIT_NOTIFY);

  properties[PROP_VALUE1] =
    g_param_spec_float ("value1", NULL, NULL,
                        -G_MAXFLOAT, G_MAXFLOAT, 0,
                        G_PARAM_READWRITE | G_PARAM_STATIC_NAME | G_PARAM_EXPLICIT_NOTIFY);

  properties[PROP_VALUE2] =
    g_param_spec_float ("value2", NULL, NULL,
                        -G_MAXFLOAT, G_MAXFLOAT, 0,
                        G_PARAM_READWRITE | G_PARAM_STATIC_NAME | G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, NUM_PROPERTIES, properties);

  bobgui_widget_class_set_css_name (widget_class, "rangeeditor");
}

/* }}} */
/* {{{ Public API */

RangeEditor *
range_editor_new (void)
{
  return g_object_new (RANGE_EDITOR_TYPE, NULL);
}

void
range_editor_get_limits (RangeEditor *self,
                         float       *lower,
                         float       *upper)
{
  if (lower)
    *lower = self->lower;

  if (upper)
    *upper = self->upper;
}

void
range_editor_get_values (RangeEditor *self,
                         float       *value1,
                         float       *value2)
{
  if (value1)
    *value1 = self->value1;

  if (value2)
    *value2 = self->value2;
}

void
range_editor_configure (RangeEditor *self,
                        float        lower,
                        float        upper,
                        float        value1,
                        float        value2)
{
  g_return_if_fail (lower <= value1);
  g_return_if_fail (value1 <= value2);
  g_return_if_fail (value2 <= upper);

  g_object_freeze_notify (G_OBJECT (self));

  if (self->lower != lower)
    {
      self->lower = lower;
      g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_LOWER]);
    }

  if (self->upper != upper)
    {
      self->upper = upper;
      g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_UPPER]);
    }

  if (self->value1 != value1)
    {
      self->value1 = value1;
      g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_VALUE1]);
    }

  if (self->value2 != value2)
    {
      self->value2 = value2;
      g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_VALUE2]);
    }

  g_object_thaw_notify (G_OBJECT (self));

  bobgui_widget_queue_resize (self->highlight);
  bobgui_widget_queue_resize (self->slider1);
  bobgui_widget_queue_resize (self->slider2);
}

/* }}} */

/* vim:set foldmethod=marker: */
