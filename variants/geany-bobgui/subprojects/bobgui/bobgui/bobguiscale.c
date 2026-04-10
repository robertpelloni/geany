/* BOBGUI - The Bobgui Framework
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 * Copyright (C) 2001 Red Hat, Inc.
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

/*
 * Modified by the BOBGUI Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the BOBGUI Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * BOBGUI at ftp://ftp.bobgui.org/pub/bobgui/.
 */

#include "config.h"

#include "bobguiscale.h"

#include "bobguiadjustment.h"
#include "bobguibuildable.h"
#include "bobguibuilderprivate.h"
#include "bobguigizmoprivate.h"
#include "bobguilabel.h"
#include "bobguimarshalers.h"
#include "bobguiorientable.h"
#include "bobguiprivate.h"
#include "bobguirangeprivate.h"
#include "bobguitypebuiltins.h"
#include "bobguiwidgetprivate.h"

#include <math.h>
#include <stdlib.h>


/**
 * BobguiScale:
 *
 * Allows to select a numeric value with a slider control.
 *
 * <picture>
 *   <source srcset="scales-dark.png" media="(prefers-color-scheme: dark)">
 *   <img alt="An example BobguiScale" src="scales.png">
 * </picture>
 *
 * To use it, you’ll probably want to investigate the methods on its base
 * class, [class@Bobgui.Range], in addition to the methods for `BobguiScale` itself.
 * To set the value of a scale, you would normally use [method@Bobgui.Range.set_value].
 * To detect changes to the value, you would normally use the
 * [signal@Bobgui.Range::value-changed] signal.
 *
 * Note that using the same upper and lower bounds for the `BobguiScale` (through
 * the `BobguiRange` methods) will hide the slider itself. This is useful for
 * applications that want to show an undeterminate value on the scale, without
 * changing the layout of the application (such as movie or music players).
 *
 * # BobguiScale as BobguiBuildable
 *
 * `BobguiScale` supports a custom `<marks>` element, which can contain multiple
 * `<mark\>` elements. The “value” and “position” attributes have the same
 * meaning as [method@Bobgui.Scale.add_mark] parameters of the same name. If
 * the element is not empty, its content is taken as the markup to show at
 * the mark. It can be translated with the usual ”translatable” and
 * “context” attributes.
 *
 * # Shortcuts and Gestures
 *
 * `BobguiPopoverMenu` supports the following keyboard shortcuts:
 *
 * - Arrow keys, <kbd>+</kbd> and <kbd>-</kbd> will increment or decrement
 *   by step, or by page when combined with <kbd>Ctrl</kbd>.
 * - <kbd>PgUp</kbd> and <kbd>PgDn</kbd> will increment or decrement by page.
 * - <kbd>Home</kbd> and <kbd>End</kbd> will set the minimum or maximum value.
 *
 * # CSS nodes
 *
 * ```
 * scale[.fine-tune][.marks-before][.marks-after]
 * ├── [value][.top][.right][.bottom][.left]
 * ├── marks.top
 * │   ├── mark
 * │   ┊    ├── [label]
 * │   ┊    ╰── indicator
 * ┊   ┊
 * │   ╰── mark
 * ├── marks.bottom
 * │   ├── mark
 * │   ┊    ├── indicator
 * │   ┊    ╰── [label]
 * ┊   ┊
 * │   ╰── mark
 * ╰── trough
 *     ├── [fill]
 *     ├── [highlight]
 *     ╰── slider
 * ```
 *
 * `BobguiScale` has a main CSS node with name scale and a subnode for its contents,
 * with subnodes named trough and slider.
 *
 * The main node gets the style class .fine-tune added when the scale is in
 * 'fine-tuning' mode.
 *
 * If the scale has an origin (see [method@Bobgui.Scale.set_has_origin]), there is
 * a subnode with name highlight below the trough node that is used for rendering
 * the highlighted part of the trough.
 *
 * If the scale is showing a fill level (see [method@Bobgui.Range.set_show_fill_level]),
 * there is a subnode with name fill below the trough node that is used for
 * rendering the filled in part of the trough.
 *
 * If marks are present, there is a marks subnode before or after the trough
 * node, below which each mark gets a node with name mark. The marks nodes get
 * either the .top or .bottom style class.
 *
 * The mark node has a subnode named indicator. If the mark has text, it also
 * has a subnode named label. When the mark is either above or left of the
 * scale, the label subnode is the first when present. Otherwise, the indicator
 * subnode is the first.
 *
 * The main CSS node gets the 'marks-before' and/or 'marks-after' style classes
 * added depending on what marks are present.
 *
 * If the scale is displaying the value (see [property@Bobgui.Scale:draw-value]),
 * there is subnode with name value. This node will get the .top or .bottom style
 * classes similar to the marks node.
 *
 * # Accessibility
 *
 * `BobguiScale` uses the [enum@Bobgui.AccessibleRole.slider] role.
 */


#define	MAX_DIGITS	(64)	/* don't change this,
				 * a) you don't need to and
				 * b) you might cause buffer overflows in
				 *    unrelated code portions otherwise
				 */

typedef struct _BobguiScaleMark BobguiScaleMark;

typedef struct _BobguiScalePrivate       BobguiScalePrivate;
struct _BobguiScalePrivate
{
  GSList       *marks;

  BobguiWidget    *value_widget;
  BobguiWidget    *top_marks_widget;
  BobguiWidget    *bottom_marks_widget;

  int           digits;

  guint         draw_value : 1;
  guint         value_pos  : 2;

  BobguiScaleFormatValueFunc format_value_func;
  gpointer format_value_func_user_data;
  GDestroyNotify format_value_func_destroy_notify;
};

struct _BobguiScaleMark
{
  double           value;
  int              stop_position;
  BobguiPositionType  position; /* always BOBGUI_POS_TOP or BOBGUI_POS_BOTTOM */
  char            *markup;
  BobguiWidget       *label_widget;
  BobguiWidget       *indicator_widget;
  BobguiWidget       *widget;
};

enum {
  PROP_0,
  PROP_DIGITS,
  PROP_DRAW_VALUE,
  PROP_HAS_ORIGIN,
  PROP_VALUE_POS,
  LAST_PROP
};


static GParamSpec *properties[LAST_PROP];

static void     bobgui_scale_set_property            (GObject        *object,
                                                   guint           prop_id,
                                                   const GValue   *value,
                                                   GParamSpec     *pspec);
static void     bobgui_scale_get_property            (GObject        *object,
                                                   guint           prop_id,
                                                   GValue         *value,
                                                   GParamSpec     *pspec);
static void     bobgui_scale_measure (BobguiWidget      *widget,
                                   BobguiOrientation  orientation,
                                   int             for_size,
                                   int            *minimum,
                                   int            *natural,
                                   int            *minimum_baseline,
                                   int            *natural_baseline);
static void     bobgui_scale_get_range_border        (BobguiRange       *range,
                                                   BobguiBorder      *border);
static void     bobgui_scale_finalize                (GObject        *object);
static void     bobgui_scale_real_get_layout_offsets (BobguiScale       *scale,
                                                   int            *x,
                                                   int            *y);
static void     bobgui_scale_buildable_interface_init   (BobguiBuildableIface  *iface);
static gboolean bobgui_scale_buildable_custom_tag_start (BobguiBuildable       *buildable,
                                                      BobguiBuilder         *builder,
                                                      GObject            *child,
                                                      const char         *tagname,
                                                      BobguiBuildableParser *parser,
                                                      gpointer           *data);
static void     bobgui_scale_buildable_custom_finished  (BobguiBuildable       *buildable,
                                                      BobguiBuilder         *builder,
                                                      GObject            *child,
                                                      const char         *tagname,
                                                      gpointer            user_data);
static char   * bobgui_scale_format_value               (BobguiScale           *scale,
                                                      double              value);


G_DEFINE_TYPE_WITH_CODE (BobguiScale, bobgui_scale, BOBGUI_TYPE_RANGE,
                         G_ADD_PRIVATE (BobguiScale)
                         G_IMPLEMENT_INTERFACE (BOBGUI_TYPE_BUILDABLE,
                                                bobgui_scale_buildable_interface_init))

static int
compare_marks (gconstpointer a, gconstpointer b, gpointer data)
{
  gboolean inverted = GPOINTER_TO_INT (data);
  int val;
  const BobguiScaleMark *ma, *mb;

  val = inverted ? -1 : 1;

  ma = a; mb = b;

  return (ma->value > mb->value) ? val : ((ma->value < mb->value) ? -val : 0);
}

static void
update_label_request (BobguiScale *scale)
{
  BobguiScalePrivate *priv = bobgui_scale_get_instance_private (scale);
  BobguiAdjustment *adjustment = bobgui_range_get_adjustment (BOBGUI_RANGE (scale));
  double lowest_value, highest_value;
  char *text;
  int size = 0;
  int min;

  g_assert (priv->value_widget != NULL);

  lowest_value = bobgui_adjustment_get_lower (adjustment);
  highest_value = bobgui_adjustment_get_upper (adjustment);

  bobgui_widget_set_size_request (priv->value_widget, -1, -1);

  text = bobgui_scale_format_value (scale, lowest_value);
  bobgui_label_set_label (BOBGUI_LABEL (priv->value_widget), text);

  bobgui_widget_measure (priv->value_widget, BOBGUI_ORIENTATION_HORIZONTAL, -1,
                      &min, NULL, NULL, NULL);
  size = MAX (size, min);
  g_free (text);

  text = bobgui_scale_format_value (scale, highest_value);
  bobgui_label_set_label (BOBGUI_LABEL (priv->value_widget), text);

  bobgui_widget_measure (priv->value_widget, BOBGUI_ORIENTATION_HORIZONTAL, -1,
                      &min, NULL, NULL, NULL);
  size = MAX (size, min);
  g_free (text);

  text = bobgui_scale_format_value (scale, bobgui_adjustment_get_value (adjustment));
  bobgui_widget_set_size_request (priv->value_widget, size, -1);
  bobgui_label_set_label (BOBGUI_LABEL (priv->value_widget), text);
  g_free (text);
}

static void
bobgui_scale_notify (GObject    *object,
                  GParamSpec *pspec)
{
  BobguiScale *scale = BOBGUI_SCALE (object);
  BobguiScalePrivate *priv = bobgui_scale_get_instance_private (scale);

  if (strcmp (pspec->name, "inverted") == 0)
    {
      BobguiScaleMark *mark;
      GSList *m;
      int i, n;
      double *values;

      priv->marks = g_slist_sort_with_data (priv->marks,
                                            compare_marks,
                                            GINT_TO_POINTER (bobgui_range_get_inverted (BOBGUI_RANGE (scale))));

      n = g_slist_length (priv->marks);
      values = g_new (double, n);
      for (m = priv->marks, i = 0; m; m = m->next, i++)
        {
          mark = m->data;
          values[i] = mark->value;
        }

      _bobgui_range_set_stop_values (BOBGUI_RANGE (scale), values, n);

      if (priv->top_marks_widget)
        bobgui_widget_queue_resize (priv->top_marks_widget);

      if (priv->bottom_marks_widget)
        bobgui_widget_queue_resize (priv->bottom_marks_widget);

      g_free (values);
    }
  else if (strcmp (pspec->name, "adjustment") == 0)
    {
      if (priv->value_widget)
        update_label_request (scale);
    }

  if (G_OBJECT_CLASS (bobgui_scale_parent_class)->notify)
    G_OBJECT_CLASS (bobgui_scale_parent_class)->notify (object, pspec);
}

static void
bobgui_scale_allocate_value (BobguiScale *scale)
{
  BobguiScalePrivate *priv = bobgui_scale_get_instance_private (scale);
  BobguiWidget *widget = BOBGUI_WIDGET (scale);
  BobguiRange *range = BOBGUI_RANGE (widget);
  BobguiWidget *slider_widget;
  BobguiAllocation value_alloc;
  int range_width, range_height;
  graphene_rect_t slider_bounds;
  GdkRectangle trough_rect;
  int slider_center_x, slider_center_y, trough_center_x, trough_center_y;

  range_width = bobgui_widget_get_width (widget);
  range_height = bobgui_widget_get_height (widget);

  slider_widget = bobgui_range_get_slider_widget (range);
  if (!bobgui_widget_compute_bounds (slider_widget, widget, &slider_bounds))
    graphene_rect_init (&slider_bounds, 0, 0, bobgui_widget_get_width (widget), bobgui_widget_get_height (widget));

  slider_center_x = slider_bounds.origin.x + slider_bounds.size.width / 2;
  slider_center_y = slider_bounds.origin.y + slider_bounds.size.height / 2;

  bobgui_range_get_range_rect (range, &trough_rect);

  trough_center_x = trough_rect.x + trough_rect.width / 2;
  trough_center_y = trough_rect.y + trough_rect.height / 2;

  bobgui_widget_measure (priv->value_widget,
                      BOBGUI_ORIENTATION_HORIZONTAL, -1,
                      &value_alloc.width, NULL,
                      NULL, NULL);
  bobgui_widget_measure (priv->value_widget,
                      BOBGUI_ORIENTATION_VERTICAL, -1,
                      &value_alloc.height, NULL,
                      NULL, NULL);

  if (bobgui_orientable_get_orientation (BOBGUI_ORIENTABLE (range)) == BOBGUI_ORIENTATION_HORIZONTAL)
    {
      switch (priv->value_pos)
        {
        case BOBGUI_POS_LEFT:
          value_alloc.x = 0;
          value_alloc.y = trough_center_y - value_alloc.height / 2;
          break;

        case BOBGUI_POS_RIGHT:
          value_alloc.x = range_width - value_alloc.width;
          value_alloc.y = trough_center_y - value_alloc.height / 2;
          break;

        case BOBGUI_POS_TOP:
          value_alloc.x = slider_center_x - value_alloc.width / 2;
          value_alloc.y = 0;
          break;

        case BOBGUI_POS_BOTTOM:
          value_alloc.x = slider_center_x - value_alloc.width / 2;
          value_alloc.y = range_height - value_alloc.height;
          break;

        default:
          g_return_if_reached ();
          break;
        }
    }
  else /* VERTICAL */
    {
      switch (priv->value_pos)
        {
        case BOBGUI_POS_LEFT:
          value_alloc.x = 0;
          value_alloc.y = slider_center_y - value_alloc.height / 2;
          break;

        case BOBGUI_POS_RIGHT:
          value_alloc.x = range_width - value_alloc.width;
          value_alloc.y = slider_center_y - value_alloc.height / 2;
          break;

        case BOBGUI_POS_TOP:
          value_alloc.x = trough_center_x - value_alloc.width / 2;
          value_alloc.y = 0;
          break;

        case BOBGUI_POS_BOTTOM:
          value_alloc.x = trough_center_x - value_alloc.width / 2;
          value_alloc.y = range_height - value_alloc.height;
          break;

        default:
          g_return_if_reached ();
        }
    }

  bobgui_widget_size_allocate (priv->value_widget, &value_alloc, -1);
}

static void
bobgui_scale_allocate_mark (BobguiGizmo *gizmo,
                         int       width,
                         int       height,
                         int       baseline)
{
  BobguiWidget *widget = BOBGUI_WIDGET (gizmo);
  BobguiScale *scale = BOBGUI_SCALE (bobgui_widget_get_parent (bobgui_widget_get_parent (widget)));
  BobguiScaleMark *mark = g_object_get_data (G_OBJECT (gizmo), "mark");
  int indicator_width, indicator_height;
  BobguiAllocation indicator_alloc;
  BobguiOrientation orientation;

  orientation = bobgui_orientable_get_orientation (BOBGUI_ORIENTABLE (scale));
  bobgui_widget_measure (mark->indicator_widget,
                      BOBGUI_ORIENTATION_HORIZONTAL, -1,
                      &indicator_width, NULL,
                      NULL, NULL);
  bobgui_widget_measure (mark->indicator_widget,
                      BOBGUI_ORIENTATION_VERTICAL, -1,
                      &indicator_height, NULL,
                      NULL, NULL);

  if (orientation == BOBGUI_ORIENTATION_HORIZONTAL)
    {
      indicator_alloc.x = (width - indicator_width) / 2;
      if (mark->position == BOBGUI_POS_TOP)
        indicator_alloc.y =  height - indicator_height;
      else
        indicator_alloc.y = 0;

      indicator_alloc.width = indicator_width;
      indicator_alloc.height = indicator_height;
    }
  else
    {
      if (mark->position == BOBGUI_POS_TOP)
        indicator_alloc.x = width - indicator_width;
      else
        indicator_alloc.x = 0;
      indicator_alloc.y = (height - indicator_height) / 2;
      indicator_alloc.width = indicator_width;
      indicator_alloc.height = indicator_height;
    }

  bobgui_widget_size_allocate (mark->indicator_widget, &indicator_alloc, baseline);

  if (mark->label_widget)
    {
      BobguiAllocation label_alloc;

      label_alloc = (BobguiAllocation) {0, 0, width, height};

      if (orientation == BOBGUI_ORIENTATION_HORIZONTAL)
        {
          label_alloc.height = height - indicator_alloc.height;
          if (mark->position == BOBGUI_POS_BOTTOM)
            label_alloc.y = indicator_alloc.y + indicator_alloc.height;
        }
      else
        {
          label_alloc.width = width - indicator_alloc.width;
          if (mark->position == BOBGUI_POS_BOTTOM)
            label_alloc.x = indicator_alloc.x + indicator_alloc.width;
        }

      bobgui_widget_size_allocate (mark->label_widget, &label_alloc, baseline);
    }
}

static void
bobgui_scale_allocate_marks (BobguiGizmo *gizmo,
                          int       width,
                          int       height,
                          int       baseline)
{
  BobguiWidget *widget = BOBGUI_WIDGET (gizmo);
  BobguiScale *scale = BOBGUI_SCALE (bobgui_widget_get_parent (widget));
  BobguiScalePrivate *priv = bobgui_scale_get_instance_private (scale);
  BobguiOrientation orientation;
  int *marks;
  int i;
  GSList *m;

  orientation = bobgui_orientable_get_orientation (BOBGUI_ORIENTABLE (scale));
  _bobgui_range_get_stop_positions (BOBGUI_RANGE (scale), &marks);

  for (m = priv->marks, i = 0; m; m = m->next, i++)
    {
      BobguiScaleMark *mark = m->data;
      BobguiAllocation mark_alloc;
      int mark_size;

      if ((mark->position == BOBGUI_POS_TOP && widget == priv->bottom_marks_widget) ||
          (mark->position == BOBGUI_POS_BOTTOM && widget == priv->top_marks_widget))
        continue;

      bobgui_widget_measure (mark->widget,
                          orientation, -1,
                          &mark_size, NULL,
                          NULL, NULL);
      mark->stop_position = marks[i];

      if (orientation == BOBGUI_ORIENTATION_HORIZONTAL)
        {
          mark_alloc.x = mark->stop_position;
          mark_alloc.y = 0;
          mark_alloc.width = mark_size;
          mark_alloc.height = height;

          mark_alloc.x -= mark_size / 2;
        }
      else
        {
          mark_alloc.x = 0;
          mark_alloc.y = mark->stop_position;
          mark_alloc.width = width;
          mark_alloc.height = mark_size;

          mark_alloc.y -= mark_size / 2;
        }

      bobgui_widget_size_allocate (mark->widget, &mark_alloc, baseline);
    }

  g_free (marks);
}

static void
bobgui_scale_size_allocate (BobguiWidget *widget,
                         int        width,
                         int        height,
                         int        baseline)
{
  BobguiScale *scale = BOBGUI_SCALE (widget);
  BobguiScalePrivate *priv = bobgui_scale_get_instance_private (scale);
  BobguiAllocation range_rect, marks_rect;
  BobguiOrientation orientation;

  BOBGUI_WIDGET_CLASS (bobgui_scale_parent_class)->size_allocate (widget, width, height, baseline);

  orientation = bobgui_orientable_get_orientation (BOBGUI_ORIENTABLE (widget));
  bobgui_range_get_range_rect (BOBGUI_RANGE (scale), &range_rect);

  if (orientation == BOBGUI_ORIENTATION_HORIZONTAL)
    {
      int marks_height = 0;

      if (priv->top_marks_widget)
        {
          bobgui_widget_measure (priv->top_marks_widget,
                              BOBGUI_ORIENTATION_VERTICAL, -1,
                              &marks_height, NULL,
                              NULL, NULL);
          marks_rect.x = 0;
          marks_rect.y = range_rect.y - marks_height;
          marks_rect.width = range_rect.width;
          marks_rect.height = marks_height;
          bobgui_widget_size_allocate (priv->top_marks_widget, &marks_rect, -1);
        }

      if (priv->bottom_marks_widget)
        {
          bobgui_widget_measure (priv->bottom_marks_widget,
                              BOBGUI_ORIENTATION_VERTICAL, -1,
                              &marks_height, NULL,
                              NULL, NULL);
          marks_rect.x = 0;
          marks_rect.y = range_rect.y + range_rect.height;
          marks_rect.width = range_rect.width;
          marks_rect.height = marks_height;
          bobgui_widget_size_allocate (priv->bottom_marks_widget, &marks_rect, -1);
        }
    }
  else
    {
      int marks_width = 0;

      if (priv->top_marks_widget)
        {
          bobgui_widget_measure (priv->top_marks_widget,
                              BOBGUI_ORIENTATION_HORIZONTAL, -1,
                              &marks_width, NULL,
                              NULL, NULL);
          marks_rect.x = range_rect.x - marks_width;
          marks_rect.y = 0;
          marks_rect.width = marks_width;
          marks_rect.height = range_rect.height;
          bobgui_widget_size_allocate (priv->top_marks_widget, &marks_rect, -1);
        }

      if (priv->bottom_marks_widget)
        {
          bobgui_widget_measure (priv->bottom_marks_widget,
                              BOBGUI_ORIENTATION_HORIZONTAL, -1,
                              &marks_width, NULL,
                              NULL, NULL);
          marks_rect = range_rect;
          marks_rect.x = range_rect.x + range_rect.width;
          marks_rect.y = 0;
          marks_rect.width = marks_width;
          marks_rect.height = range_rect.height;
          bobgui_widget_size_allocate (priv->bottom_marks_widget, &marks_rect, -1);
        }
    }

  if (priv->value_widget)
    {
      bobgui_scale_allocate_value (scale);
    }
}

#define add_slider_binding(binding_set, keyval, mask, scroll)        \
  bobgui_widget_class_add_binding_signal (widget_class,                 \
                                       keyval, mask,                 \
                                       I_("move-slider"),            \
                                       "(i)", scroll)

static void
bobgui_scale_value_changed (BobguiRange *range)
{
  BobguiScalePrivate *priv = bobgui_scale_get_instance_private (BOBGUI_SCALE (range));
  BobguiAdjustment *adjustment = bobgui_range_get_adjustment (range);

  if (priv->value_widget)
    {
      char *text = bobgui_scale_format_value (BOBGUI_SCALE (range),
                                           bobgui_adjustment_get_value (adjustment));
      bobgui_label_set_label (BOBGUI_LABEL (priv->value_widget), text);

      g_free (text);
    }
}

static void
bobgui_scale_class_init (BobguiScaleClass *class)
{
  GObjectClass   *gobject_class;
  BobguiWidgetClass *widget_class;
  BobguiRangeClass  *range_class;

  gobject_class = G_OBJECT_CLASS (class);
  range_class = (BobguiRangeClass*) class;
  widget_class = (BobguiWidgetClass*) class;

  gobject_class->set_property = bobgui_scale_set_property;
  gobject_class->get_property = bobgui_scale_get_property;
  gobject_class->notify = bobgui_scale_notify;
  gobject_class->finalize = bobgui_scale_finalize;

  widget_class->size_allocate = bobgui_scale_size_allocate;
  widget_class->measure = bobgui_scale_measure;
  widget_class->grab_focus = bobgui_widget_grab_focus_self;
  widget_class->focus = bobgui_widget_focus_self;

  range_class->get_range_border = bobgui_scale_get_range_border;
  range_class->value_changed = bobgui_scale_value_changed;

  class->get_layout_offsets = bobgui_scale_real_get_layout_offsets;

  /**
   * BobguiScale:digits:
   *
   * The number of decimal places that are displayed in the value.
   */
  properties[PROP_DIGITS] =
      g_param_spec_int ("digits", NULL, NULL,
                        -1, MAX_DIGITS,
                        1,
                        BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiScale:draw-value:
   *
   * Whether the current value is displayed as a string next to the slider.
   */
  properties[PROP_DRAW_VALUE] =
      g_param_spec_boolean ("draw-value", NULL, NULL,
                            FALSE,
                            BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiScale:has-origin:
   *
   * Whether the scale has an origin.
   */
  properties[PROP_HAS_ORIGIN] =
      g_param_spec_boolean ("has-origin", NULL, NULL,
                            TRUE,
                            BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiScale:value-pos:
   *
   * The position in which the current value is displayed.
   */
  properties[PROP_VALUE_POS] =
      g_param_spec_enum ("value-pos", NULL, NULL,
                         BOBGUI_TYPE_POSITION_TYPE,
                         BOBGUI_POS_TOP,
                         BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (gobject_class, LAST_PROP, properties);

  /* All bindings (even arrow keys) are on both h/v scale, because
   * blind users etc. don't care about scale orientation.
   */

  add_slider_binding (binding_set, GDK_KEY_Left, 0,
                      BOBGUI_SCROLL_STEP_LEFT);

  add_slider_binding (binding_set, GDK_KEY_Left, GDK_CONTROL_MASK,
                      BOBGUI_SCROLL_PAGE_LEFT);

  add_slider_binding (binding_set, GDK_KEY_KP_Left, 0,
                      BOBGUI_SCROLL_STEP_LEFT);

  add_slider_binding (binding_set, GDK_KEY_KP_Left, GDK_CONTROL_MASK,
                      BOBGUI_SCROLL_PAGE_LEFT);

  add_slider_binding (binding_set, GDK_KEY_Right, 0,
                      BOBGUI_SCROLL_STEP_RIGHT);

  add_slider_binding (binding_set, GDK_KEY_Right, GDK_CONTROL_MASK,
                      BOBGUI_SCROLL_PAGE_RIGHT);

  add_slider_binding (binding_set, GDK_KEY_KP_Right, 0,
                      BOBGUI_SCROLL_STEP_RIGHT);

  add_slider_binding (binding_set, GDK_KEY_KP_Right, GDK_CONTROL_MASK,
                      BOBGUI_SCROLL_PAGE_RIGHT);

  add_slider_binding (binding_set, GDK_KEY_Up, 0,
                      BOBGUI_SCROLL_STEP_UP);

  add_slider_binding (binding_set, GDK_KEY_Up, GDK_CONTROL_MASK,
                      BOBGUI_SCROLL_PAGE_UP);

  add_slider_binding (binding_set, GDK_KEY_KP_Up, 0,
                      BOBGUI_SCROLL_STEP_UP);

  add_slider_binding (binding_set, GDK_KEY_KP_Up, GDK_CONTROL_MASK,
                      BOBGUI_SCROLL_PAGE_UP);

  add_slider_binding (binding_set, GDK_KEY_Down, 0,
                      BOBGUI_SCROLL_STEP_DOWN);

  add_slider_binding (binding_set, GDK_KEY_Down, GDK_CONTROL_MASK,
                      BOBGUI_SCROLL_PAGE_DOWN);

  add_slider_binding (binding_set, GDK_KEY_KP_Down, 0,
                      BOBGUI_SCROLL_STEP_DOWN);

  add_slider_binding (binding_set, GDK_KEY_KP_Down, GDK_CONTROL_MASK,
                      BOBGUI_SCROLL_PAGE_DOWN);

  add_slider_binding (binding_set, GDK_KEY_Page_Up, GDK_CONTROL_MASK,
                      BOBGUI_SCROLL_PAGE_LEFT);

  add_slider_binding (binding_set, GDK_KEY_KP_Page_Up, GDK_CONTROL_MASK,
                      BOBGUI_SCROLL_PAGE_LEFT);

  add_slider_binding (binding_set, GDK_KEY_Page_Up, 0,
                      BOBGUI_SCROLL_PAGE_UP);

  add_slider_binding (binding_set, GDK_KEY_KP_Page_Up, 0,
                      BOBGUI_SCROLL_PAGE_UP);

  add_slider_binding (binding_set, GDK_KEY_Page_Down, GDK_CONTROL_MASK,
                      BOBGUI_SCROLL_PAGE_RIGHT);

  add_slider_binding (binding_set, GDK_KEY_KP_Page_Down, GDK_CONTROL_MASK,
                      BOBGUI_SCROLL_PAGE_RIGHT);

  add_slider_binding (binding_set, GDK_KEY_Page_Down, 0,
                      BOBGUI_SCROLL_PAGE_DOWN);

  add_slider_binding (binding_set, GDK_KEY_KP_Page_Down, 0,
                      BOBGUI_SCROLL_PAGE_DOWN);

  /* Logical bindings (vs. visual bindings above) */

  add_slider_binding (binding_set, GDK_KEY_plus, 0,
                      BOBGUI_SCROLL_STEP_FORWARD);

  add_slider_binding (binding_set, GDK_KEY_minus, 0,
                      BOBGUI_SCROLL_STEP_BACKWARD);

  add_slider_binding (binding_set, GDK_KEY_plus, GDK_CONTROL_MASK,
                      BOBGUI_SCROLL_PAGE_FORWARD);

  add_slider_binding (binding_set, GDK_KEY_minus, GDK_CONTROL_MASK,
                      BOBGUI_SCROLL_PAGE_BACKWARD);


  add_slider_binding (binding_set, GDK_KEY_KP_Add, 0,
                      BOBGUI_SCROLL_STEP_FORWARD);

  add_slider_binding (binding_set, GDK_KEY_KP_Subtract, 0,
                      BOBGUI_SCROLL_STEP_BACKWARD);

  add_slider_binding (binding_set, GDK_KEY_KP_Add, GDK_CONTROL_MASK,
                      BOBGUI_SCROLL_PAGE_FORWARD);

  add_slider_binding (binding_set, GDK_KEY_KP_Subtract, GDK_CONTROL_MASK,
                      BOBGUI_SCROLL_PAGE_BACKWARD);

  add_slider_binding (binding_set, GDK_KEY_Home, 0,
                      BOBGUI_SCROLL_START);

  add_slider_binding (binding_set, GDK_KEY_KP_Home, 0,
                      BOBGUI_SCROLL_START);

  add_slider_binding (binding_set, GDK_KEY_End, 0,
                      BOBGUI_SCROLL_END);

  add_slider_binding (binding_set, GDK_KEY_KP_End, 0,
                      BOBGUI_SCROLL_END);

  bobgui_widget_class_set_css_name (widget_class, I_("scale"));

  bobgui_widget_class_set_accessible_role (widget_class, BOBGUI_ACCESSIBLE_ROLE_SLIDER);
}

static void
bobgui_scale_init (BobguiScale *scale)
{
  BobguiScalePrivate *priv = bobgui_scale_get_instance_private (scale);
  BobguiRange *range = BOBGUI_RANGE (scale);

  bobgui_widget_set_focusable (BOBGUI_WIDGET (scale), TRUE);

  priv->value_pos = BOBGUI_POS_TOP;
  priv->digits = 1;

  bobgui_range_set_slider_size_fixed (range, TRUE);

  _bobgui_range_set_has_origin (range, TRUE);

  bobgui_range_set_round_digits (range, -1);

  bobgui_range_set_flippable (range, TRUE);
}

static void
bobgui_scale_set_property (GObject      *object,
			guint         prop_id,
			const GValue *value,
			GParamSpec   *pspec)
{
  BobguiScale *scale;

  scale = BOBGUI_SCALE (object);

  switch (prop_id)
    {
    case PROP_DIGITS:
      bobgui_scale_set_digits (scale, g_value_get_int (value));
      break;
    case PROP_DRAW_VALUE:
      bobgui_scale_set_draw_value (scale, g_value_get_boolean (value));
      break;
    case PROP_HAS_ORIGIN:
      bobgui_scale_set_has_origin (scale, g_value_get_boolean (value));
      break;
    case PROP_VALUE_POS:
      bobgui_scale_set_value_pos (scale, g_value_get_enum (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bobgui_scale_get_property (GObject      *object,
			guint         prop_id,
			GValue       *value,
			GParamSpec   *pspec)
{
  BobguiScale *scale = BOBGUI_SCALE (object);
  BobguiScalePrivate *priv = bobgui_scale_get_instance_private (scale);

  switch (prop_id)
    {
    case PROP_DIGITS:
      g_value_set_int (value, priv->digits);
      break;
    case PROP_DRAW_VALUE:
      g_value_set_boolean (value, priv->draw_value);
      break;
    case PROP_HAS_ORIGIN:
      g_value_set_boolean (value, bobgui_scale_get_has_origin (scale));
      break;
    case PROP_VALUE_POS:
      g_value_set_enum (value, priv->value_pos);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

/**
 * bobgui_scale_new:
 * @orientation: the scale’s orientation.
 * @adjustment: (nullable): the [class@Bobgui.Adjustment] which sets
 *   the range of the scale, or %NULL to create a new adjustment.
 *
 * Creates a new `BobguiScale`.
 *
 * Returns: a new `BobguiScale`
 */
BobguiWidget *
bobgui_scale_new (BobguiOrientation  orientation,
               BobguiAdjustment  *adjustment)
{
  g_return_val_if_fail (adjustment == NULL || BOBGUI_IS_ADJUSTMENT (adjustment),
                        NULL);

  return g_object_new (BOBGUI_TYPE_SCALE,
                       "orientation", orientation,
                       "adjustment",  adjustment,
                       NULL);
}

/**
 * bobgui_scale_new_with_range:
 * @orientation: the scale’s orientation.
 * @min: minimum value
 * @max: maximum value
 * @step: step increment (tick size) used with keyboard shortcuts
 *
 * Creates a new scale widget with a range from @min to @max.
 *
 * The returns scale will have the given orientation and will let the
 * user input a number between @min and @max (including @min and @max)
 * with the increment @step. @step must be nonzero; it’s the distance
 * the slider moves when using the arrow keys to adjust the scale
 * value.
 *
 * Note that the way in which the precision is derived works best if
 * @step is a power of ten. If the resulting precision is not suitable
 * for your needs, use [method@Bobgui.Scale.set_digits] to correct it.
 *
 * Returns: a new `BobguiScale`
 */
BobguiWidget *
bobgui_scale_new_with_range (BobguiOrientation orientation,
                          double         min,
                          double         max,
                          double         step)
{
  BobguiAdjustment *adj;
  int digits;

  g_return_val_if_fail (min < max, NULL);
  g_return_val_if_fail (step != 0.0, NULL);

  adj = bobgui_adjustment_new (min, min, max, step, 10 * step, 0);

  if (fabs (step) >= 1.0 || step == 0.0)
    {
      digits = 0;
    }
  else
    {
      digits = abs ((int) floor (log10 (fabs (step))));
      if (digits > 5)
        digits = 5;
    }

  return g_object_new (BOBGUI_TYPE_SCALE,
                       "orientation", orientation,
                       "adjustment",  adj,
                       "digits",      digits,
                       NULL);
}

/**
 * bobgui_scale_set_digits:
 * @scale: a `BobguiScale`
 * @digits: the number of decimal places to display,
 *   e.g. use 1 to display 1.0, 2 to display 1.00, etc
 *
 * Sets the number of decimal places that are displayed in the value.
 *
 * Also causes the value of the adjustment to be rounded to this number
 * of digits, so the retrieved value matches the displayed one, if
 * [property@Bobgui.Scale:draw-value] is %TRUE when the value changes. If
 * you want to enforce rounding the value when [property@Bobgui.Scale:draw-value]
 * is %FALSE, you can set [property@Bobgui.Range:round-digits] instead.
 *
 * Note that rounding to a small number of digits can interfere with
 * the smooth autoscrolling that is built into `BobguiScale`. As an alternative,
 * you can use [method@Bobgui.Scale.set_format_value_func] to format the displayed
 * value yourself.
 */
void
bobgui_scale_set_digits (BobguiScale *scale,
		      int       digits)
{
  BobguiScalePrivate *priv = bobgui_scale_get_instance_private (scale);
  BobguiRange *range;

  g_return_if_fail (BOBGUI_IS_SCALE (scale));

  range = BOBGUI_RANGE (scale);

  digits = CLAMP (digits, -1, MAX_DIGITS);

  if (priv->digits != digits)
    {
      priv->digits = digits;
      if (priv->draw_value)
        bobgui_range_set_round_digits (range, digits);

      if (priv->value_widget)
        update_label_request (scale);

      bobgui_widget_queue_resize (BOBGUI_WIDGET (scale));

      g_object_notify_by_pspec (G_OBJECT (scale), properties[PROP_DIGITS]);
    }
}

/**
 * bobgui_scale_get_digits:
 * @scale: a `BobguiScale`
 *
 * Gets the number of decimal places that are displayed in the value.
 *
 * Returns: the number of decimal places that are displayed
 */
int
bobgui_scale_get_digits (BobguiScale *scale)
{
  BobguiScalePrivate *priv = bobgui_scale_get_instance_private (scale);

  g_return_val_if_fail (BOBGUI_IS_SCALE (scale), -1);

  return priv->digits;
}

static void
update_value_position (BobguiScale *scale)
{
  BobguiScalePrivate *priv = bobgui_scale_get_instance_private (scale);

  if (!priv->value_widget)
    return;

  bobgui_widget_remove_css_class (priv->value_widget, "top");
  bobgui_widget_remove_css_class (priv->value_widget, "right");
  bobgui_widget_remove_css_class (priv->value_widget, "bottom");
  bobgui_widget_remove_css_class (priv->value_widget, "left");

  switch (priv->value_pos)
    {
    case BOBGUI_POS_TOP:
      bobgui_widget_add_css_class (priv->value_widget, "top");
      break;
    case BOBGUI_POS_RIGHT:
      bobgui_widget_add_css_class (priv->value_widget, "right");
      break;
    case BOBGUI_POS_BOTTOM:
      bobgui_widget_add_css_class (priv->value_widget, "bottom");
      break;
    case BOBGUI_POS_LEFT:
      bobgui_widget_add_css_class (priv->value_widget, "left");
      break;
    default:
      g_assert_not_reached ();
      break;
    }
}

/**
 * bobgui_scale_set_draw_value:
 * @scale: a `BobguiScale`
 * @draw_value: %TRUE to draw the value
 *
 * Specifies whether the current value is displayed as a string next
 * to the slider.
 */
void
bobgui_scale_set_draw_value (BobguiScale *scale,
                          gboolean  draw_value)
{
  BobguiScalePrivate *priv = bobgui_scale_get_instance_private (scale);

  g_return_if_fail (BOBGUI_IS_SCALE (scale));

  draw_value = draw_value != FALSE;

  if (priv->draw_value != draw_value)
    {
      priv->draw_value = draw_value;
      if (draw_value)
        {
          priv->value_widget = g_object_new (BOBGUI_TYPE_LABEL,
                                             "css-name", "value",
                                             NULL);

          bobgui_widget_insert_after (priv->value_widget, BOBGUI_WIDGET (scale), NULL);
          bobgui_range_set_round_digits (BOBGUI_RANGE (scale), priv->digits);
          update_value_position (scale);
          update_label_request (scale);
        }
      else if (priv->value_widget)
        {
          g_clear_pointer (&priv->value_widget, bobgui_widget_unparent);
          bobgui_range_set_round_digits (BOBGUI_RANGE (scale), -1);
        }

      g_object_notify_by_pspec (G_OBJECT (scale), properties[PROP_DRAW_VALUE]);
    }
}

/**
 * bobgui_scale_get_draw_value:
 * @scale: a `BobguiScale`
 *
 * Returns whether the current value is displayed as a string
 * next to the slider.
 *
 * Returns: whether the current value is displayed as a string
 */
gboolean
bobgui_scale_get_draw_value (BobguiScale *scale)
{
  BobguiScalePrivate *priv = bobgui_scale_get_instance_private (scale);

  g_return_val_if_fail (BOBGUI_IS_SCALE (scale), FALSE);

  return priv->draw_value;
}

/**
 * bobgui_scale_set_has_origin:
 * @scale: a `BobguiScale`
 * @has_origin: %TRUE if the scale has an origin
 *
 * Sets whether the scale has an origin.
 *
 * If [property@Bobgui.Scale:has-origin] is set to %TRUE (the default),
 * the scale will highlight the part of the trough between the origin
 * (bottom or left side) and the current value.
 */
void
bobgui_scale_set_has_origin (BobguiScale *scale,
                          gboolean  has_origin)
{
  g_return_if_fail (BOBGUI_IS_SCALE (scale));

  has_origin = has_origin != FALSE;

  if (_bobgui_range_get_has_origin (BOBGUI_RANGE (scale)) != has_origin)
    {
      _bobgui_range_set_has_origin (BOBGUI_RANGE (scale), has_origin);

      bobgui_widget_queue_draw (BOBGUI_WIDGET (scale));

      g_object_notify_by_pspec (G_OBJECT (scale), properties[PROP_HAS_ORIGIN]);
    }
}

/**
 * bobgui_scale_get_has_origin:
 * @scale: a `BobguiScale`
 *
 * Returns whether the scale has an origin.
 *
 * Returns: %TRUE if the scale has an origin.
 */
gboolean
bobgui_scale_get_has_origin (BobguiScale *scale)
{
  g_return_val_if_fail (BOBGUI_IS_SCALE (scale), FALSE);

  return _bobgui_range_get_has_origin (BOBGUI_RANGE (scale));
}

/**
 * bobgui_scale_set_value_pos:
 * @scale: a `BobguiScale`
 * @pos: the position in which the current value is displayed
 *
 * Sets the position in which the current value is displayed.
 */
void
bobgui_scale_set_value_pos (BobguiScale        *scale,
                         BobguiPositionType  pos)
{
  BobguiScalePrivate *priv = bobgui_scale_get_instance_private (scale);

  g_return_if_fail (BOBGUI_IS_SCALE (scale));

  if (priv->value_pos != pos)
    {
      priv->value_pos = pos;

      update_value_position (scale);
      bobgui_widget_queue_resize (BOBGUI_WIDGET (scale));

      g_object_notify_by_pspec (G_OBJECT (scale), properties[PROP_VALUE_POS]);
    }
}

/**
 * bobgui_scale_get_value_pos:
 * @scale: a `BobguiScale`
 *
 * Gets the position in which the current value is displayed.
 *
 * Returns: the position in which the current value is displayed
 */
BobguiPositionType
bobgui_scale_get_value_pos (BobguiScale *scale)
{
  BobguiScalePrivate *priv = bobgui_scale_get_instance_private (scale);

  g_return_val_if_fail (BOBGUI_IS_SCALE (scale), 0);

  return priv->value_pos;
}

static void
bobgui_scale_get_range_border (BobguiRange  *range,
                            BobguiBorder *border)
{
  BobguiScale *scale = BOBGUI_SCALE (range);
  BobguiScalePrivate *priv = bobgui_scale_get_instance_private (scale);

  border->left = 0;
  border->right = 0;
  border->top = 0;
  border->bottom = 0;

  if (priv->value_widget)
    {
      int value_size;
      BobguiOrientation value_orientation;

      if (priv->value_pos == BOBGUI_POS_LEFT || priv->value_pos == BOBGUI_POS_RIGHT)
        value_orientation = BOBGUI_ORIENTATION_HORIZONTAL;
      else
        value_orientation = BOBGUI_ORIENTATION_VERTICAL;

      bobgui_widget_measure (priv->value_widget,
                          value_orientation, -1,
                          &value_size, NULL,
                          NULL, NULL);

      switch (priv->value_pos)
        {
        case BOBGUI_POS_LEFT:
          border->left += value_size;
          break;
        case BOBGUI_POS_RIGHT:
          border->right += value_size;
          break;
        case BOBGUI_POS_TOP:
          border->top += value_size;
          break;
        case BOBGUI_POS_BOTTOM:
          border->bottom += value_size;
          break;
        default:
          g_assert_not_reached ();
          break;
        }
    }

  if (bobgui_orientable_get_orientation (BOBGUI_ORIENTABLE (range)) == BOBGUI_ORIENTATION_HORIZONTAL)
    {
      int height;

      if (priv->top_marks_widget)
        {
          bobgui_widget_measure (priv->top_marks_widget,
                              BOBGUI_ORIENTATION_VERTICAL, -1,
                              &height, NULL,
                              NULL, NULL);
          if (height > 0)
            border->top += height;
        }

      if (priv->bottom_marks_widget)
        {
          bobgui_widget_measure (priv->bottom_marks_widget,
                              BOBGUI_ORIENTATION_VERTICAL, -1,
                              &height, NULL,
                              NULL, NULL);
          if (height > 0)
            border->bottom += height;
        }
    }
  else
    {
      int width;

      if (priv->top_marks_widget)
        {
          bobgui_widget_measure (priv->top_marks_widget,
                              BOBGUI_ORIENTATION_HORIZONTAL, -1,
                              &width, NULL,
                              NULL, NULL);
          if (width > 0)
            border->left += width;
        }

      if (priv->bottom_marks_widget)
        {
          bobgui_widget_measure (priv->bottom_marks_widget,
                              BOBGUI_ORIENTATION_HORIZONTAL, -1,
                              &width, NULL,
                              NULL, NULL);
          if (width > 0)
            border->right += width;
        }
    }
}

static void
bobgui_scale_measure_mark (BobguiGizmo       *gizmo,
                        BobguiOrientation  orientation,
                        int             for_size,
                        int            *minimum,
                        int            *natural,
                        int            *minimum_baseline,
                        int            *natural_baseline)
{
  BobguiScaleMark *mark = g_object_get_data (G_OBJECT (gizmo), "mark");

  bobgui_widget_measure (mark->indicator_widget,
                      orientation, -1,
                      minimum, natural,
                      NULL, NULL);

  if (mark->label_widget)
    {
      int label_min, label_nat;

      bobgui_widget_measure (mark->label_widget,
                          orientation, -1,
                          &label_min, &label_nat,
                          NULL, NULL);
      *minimum += label_min;
      *natural += label_nat;
    }
}

static void
bobgui_scale_measure_marks (BobguiGizmo       *gizmo,
                         BobguiOrientation  orientation,
                         int             for_size,
                         int            *minimum,
                         int            *natural,
                         int            *minimum_baseline,
                         int            *natural_baseline)
{
  BobguiWidget *widget = BOBGUI_WIDGET (gizmo);
  BobguiScale *scale = BOBGUI_SCALE (bobgui_widget_get_parent (widget));;
  BobguiScalePrivate *priv = bobgui_scale_get_instance_private (scale);
  BobguiOrientation scale_orientation = bobgui_orientable_get_orientation (BOBGUI_ORIENTABLE (scale));
  GSList *m;

  *minimum = *natural = 0;

  for (m = priv->marks; m; m = m->next)
    {
      BobguiScaleMark *mark = m->data;
      int mark_size;

      if ((mark->position == BOBGUI_POS_TOP && widget == priv->bottom_marks_widget) ||
          (mark->position == BOBGUI_POS_BOTTOM && widget == priv->top_marks_widget))
        continue;

      bobgui_widget_measure (mark->widget,
                          orientation, -1,
                          &mark_size, NULL,
                          NULL, NULL);

      if (scale_orientation == orientation)
        {
          *minimum += mark_size;
          *natural += mark_size;
        }
      else
        {
          *minimum = MAX (*minimum, mark_size);
          *natural = MAX (*natural, mark_size);
        }
    }
}

static void
bobgui_scale_measure (BobguiWidget      *widget,
                   BobguiOrientation  orientation,
                   int             for_size,
                   int            *minimum,
                   int            *natural,
                   int            *minimum_baseline,
                   int            *natural_baseline)
{
  BobguiScale *scale = BOBGUI_SCALE (widget);
  BobguiScalePrivate *priv = bobgui_scale_get_instance_private (scale);
  BobguiOrientation scale_orientation;
  int range_minimum, range_natural, scale_minimum = 0, scale_natural = 0;

  BOBGUI_WIDGET_CLASS (bobgui_scale_parent_class)->measure (widget,
                                                      orientation,
                                                      for_size,
                                                      &range_minimum, &range_natural,
                                                      minimum_baseline, natural_baseline);

  scale_orientation = bobgui_orientable_get_orientation (BOBGUI_ORIENTABLE (widget));

  if (scale_orientation == orientation)
    {
      int top_marks_size = 0, bottom_marks_size = 0, marks_size;

      if (priv->top_marks_widget)
        bobgui_widget_measure (priv->top_marks_widget,
                            orientation, for_size,
                            &top_marks_size, NULL,
                            NULL, NULL);
      if (priv->bottom_marks_widget)
        bobgui_widget_measure (priv->bottom_marks_widget,
                            orientation, for_size,
                            &bottom_marks_size, NULL,
                            NULL, NULL);

      marks_size = MAX (top_marks_size, bottom_marks_size);

      scale_minimum = MAX (scale_minimum, marks_size);
      scale_natural = MAX (scale_natural, marks_size);
    }

  if (priv->value_widget)
    {
      int min, nat;

      bobgui_widget_measure (priv->value_widget, orientation, -1, &min, &nat, NULL, NULL);

      if (priv->value_pos == BOBGUI_POS_TOP ||
          priv->value_pos == BOBGUI_POS_BOTTOM)
        {
          if (orientation == BOBGUI_ORIENTATION_HORIZONTAL)
            {
              scale_minimum = MAX (scale_minimum, min);
              scale_natural = MAX (scale_natural, nat);
            }
          else
            {
              scale_minimum += min;
              scale_natural += nat;
            }
        }
      else
        {
          if (orientation == BOBGUI_ORIENTATION_HORIZONTAL)
            {
              scale_minimum += min;
              scale_natural += nat;
            }
          else
            {
              scale_minimum = MAX (scale_minimum, min);
              scale_natural = MAX (scale_natural, nat);
            }
        }
    }

  *minimum = MAX (range_minimum, scale_minimum);
  *natural = MAX (range_natural, scale_natural);
}

static void
bobgui_scale_real_get_layout_offsets (BobguiScale *scale,
                                   int      *x,
                                   int      *y)
{
  BobguiScalePrivate *priv = bobgui_scale_get_instance_private (scale);
  graphene_rect_t value_bounds;

  if (!priv->value_widget ||
      !bobgui_widget_compute_bounds (priv->value_widget, BOBGUI_WIDGET (scale), &value_bounds))
    {
      *x = 0;
      *y = 0;

      return;
    }


  *x = value_bounds.origin.x;
  *y = value_bounds.origin.y;
}

static char *
weed_out_neg_zero (char *str,
                   int    digits)
{
  if (str[0] == '-')
    {
      char neg_zero[8];
      g_snprintf (neg_zero, 8, "%0.*f", digits, -0.0);
      if (strcmp (neg_zero, str) == 0)
        memmove (str, str + 1, strlen (str));
    }
  return str;
}

static char *
bobgui_scale_format_value (BobguiScale *scale,
                        double    value)
{
  BobguiScalePrivate *priv = bobgui_scale_get_instance_private (scale);

  if (priv->format_value_func)
    {
      return priv->format_value_func (scale, value, priv->format_value_func_user_data);
    }
  else
    {
      char *fmt = g_strdup_printf ("%0.*f", priv->digits, value);
      return weed_out_neg_zero (fmt, priv->digits);
    }
}

static void
bobgui_scale_finalize (GObject *object)
{
  BobguiScale *scale = BOBGUI_SCALE (object);
  BobguiScalePrivate *priv = bobgui_scale_get_instance_private (scale);

  bobgui_scale_clear_marks (scale);

  g_clear_pointer (&priv->value_widget, bobgui_widget_unparent);

  if (priv->format_value_func_destroy_notify)
    priv->format_value_func_destroy_notify (priv->format_value_func_user_data);

  G_OBJECT_CLASS (bobgui_scale_parent_class)->finalize (object);
}

/**
 * bobgui_scale_get_layout:
 * @scale: A `BobguiScale`
 *
 * Gets the `PangoLayout` used to display the scale.
 *
 * The returned object is owned by the scale so does not need
 * to be freed by the caller.
 *
 * Returns: (transfer none) (nullable): the [class@Pango.Layout]
 *   for this scale, or %NULL if the [property@Bobgui.Scale:draw-value]
 *   property is %FALSE.
 */
PangoLayout *
bobgui_scale_get_layout (BobguiScale *scale)
{
  BobguiScalePrivate *priv = bobgui_scale_get_instance_private (scale);

  g_return_val_if_fail (BOBGUI_IS_SCALE (scale), NULL);

  if (priv->value_widget)
    return bobgui_label_get_layout (BOBGUI_LABEL (priv->value_widget));

  return NULL;
}

/**
 * bobgui_scale_get_layout_offsets:
 * @scale: a `BobguiScale`
 * @x: (out) (optional): location to store X offset of layout
 * @y: (out) (optional): location to store Y offset of layout
 *
 * Obtains the coordinates where the scale will draw the
 * `PangoLayout` representing the text in the scale.
 *
 * Remember when using the `PangoLayout` function you need to
 * convert to and from pixels using `PANGO_PIXELS()` or `PANGO_SCALE`.
 *
 * If the [property@Bobgui.Scale:draw-value] property is %FALSE, the return
 * values are undefined.
 */
void
bobgui_scale_get_layout_offsets (BobguiScale *scale,
                              int      *x,
                              int      *y)
{
  int local_x = 0;
  int local_y = 0;

  g_return_if_fail (BOBGUI_IS_SCALE (scale));

  if (BOBGUI_SCALE_GET_CLASS (scale)->get_layout_offsets)
    (BOBGUI_SCALE_GET_CLASS (scale)->get_layout_offsets) (scale, &local_x, &local_y);

  if (x)
    *x = local_x;

  if (y)
    *y = local_y;
}

static void
bobgui_scale_mark_free (gpointer data)
{
  BobguiScaleMark *mark = data;

  if (mark->label_widget)
    bobgui_widget_unparent (mark->label_widget);

  bobgui_widget_unparent (mark->indicator_widget);
  bobgui_widget_unparent (mark->widget);
  g_free (mark->markup);
  g_free (mark);
}

/**
 * bobgui_scale_clear_marks:
 * @scale: a `BobguiScale`
 *
 * Removes any marks that have been added.
 */
void
bobgui_scale_clear_marks (BobguiScale *scale)
{
  BobguiScalePrivate *priv = bobgui_scale_get_instance_private (scale);

  g_return_if_fail (BOBGUI_IS_SCALE (scale));

  g_slist_free_full (priv->marks, bobgui_scale_mark_free);
  priv->marks = NULL;

  g_clear_pointer (&priv->top_marks_widget, bobgui_widget_unparent);
  g_clear_pointer (&priv->bottom_marks_widget, bobgui_widget_unparent);

  bobgui_widget_remove_css_class (BOBGUI_WIDGET (scale), "marks-before");
  bobgui_widget_remove_css_class (BOBGUI_WIDGET (scale), "marks-after");

  _bobgui_range_set_stop_values (BOBGUI_RANGE (scale), NULL, 0);

  bobgui_widget_queue_resize (BOBGUI_WIDGET (scale));
}

/**
 * bobgui_scale_add_mark:
 * @scale: a `BobguiScale`
 * @value: the value at which the mark is placed, must be between
 *   the lower and upper limits of the scales’ adjustment
 * @position: where to draw the mark. For a horizontal scale, %BOBGUI_POS_TOP
 *   and %BOBGUI_POS_LEFT are drawn above the scale, anything else below.
 *   For a vertical scale, %BOBGUI_POS_LEFT and %BOBGUI_POS_TOP are drawn to
 *   the left of the scale, anything else to the right.
 * @markup: (nullable): Text to be shown at the mark, using Pango markup
 *
 * Adds a mark at @value.
 *
 * A mark is indicated visually by drawing a tick mark next to the scale,
 * and BOBGUI makes it easy for the user to position the scale exactly at the
 * marks value.
 *
 * If @markup is not %NULL, text is shown next to the tick mark.
 *
 * To remove marks from a scale, use [method@Bobgui.Scale.clear_marks].
 */
void
bobgui_scale_add_mark (BobguiScale        *scale,
                    double           value,
                    BobguiPositionType  position,
                    const char      *markup)
{
  BobguiWidget *widget;
  BobguiScalePrivate *priv = bobgui_scale_get_instance_private (scale);
  BobguiScaleMark *mark;
  GSList *m;
  double *values;
  int n, i;
  BobguiWidget *marks_widget;

  g_return_if_fail (BOBGUI_IS_SCALE (scale));

  widget = BOBGUI_WIDGET (scale);

  mark = g_new0 (BobguiScaleMark, 1);
  mark->value = value;
  mark->markup = g_strdup (markup);
  if (position == BOBGUI_POS_LEFT ||
      position == BOBGUI_POS_TOP)
    mark->position = BOBGUI_POS_TOP;
  else
    mark->position = BOBGUI_POS_BOTTOM;

  priv->marks = g_slist_insert_sorted_with_data (priv->marks, mark,
                                                 compare_marks,
                                                 GINT_TO_POINTER (bobgui_range_get_inverted (BOBGUI_RANGE (scale))));

  if (mark->position == BOBGUI_POS_TOP)
    {
      if (!priv->top_marks_widget)
        {
          priv->top_marks_widget = bobgui_gizmo_new_with_role ("marks",
                                                            BOBGUI_ACCESSIBLE_ROLE_NONE,
                                                            bobgui_scale_measure_marks,
                                                            bobgui_scale_allocate_marks,
                                                            NULL,
                                                            NULL,
                                                            NULL, NULL);

          bobgui_widget_insert_after (priv->top_marks_widget,
                                   BOBGUI_WIDGET (scale),
                                   priv->value_widget);
          bobgui_widget_add_css_class (priv->top_marks_widget, "top");
        }
      marks_widget = priv->top_marks_widget;
    }
  else
    {
      if (!priv->bottom_marks_widget)
        {
          priv->bottom_marks_widget = bobgui_gizmo_new_with_role ("marks",
                                                               BOBGUI_ACCESSIBLE_ROLE_NONE,
                                                               bobgui_scale_measure_marks,
                                                               bobgui_scale_allocate_marks,
                                                               NULL,
                                                               NULL,
                                                               NULL, NULL);

          bobgui_widget_insert_before (priv->bottom_marks_widget,
                                    BOBGUI_WIDGET (scale),
                                    bobgui_range_get_trough_widget (BOBGUI_RANGE (scale)));
          bobgui_widget_add_css_class (priv->bottom_marks_widget, "bottom");
        }
      marks_widget = priv->bottom_marks_widget;
    }

  mark->widget = bobgui_gizmo_new ("mark", bobgui_scale_measure_mark, bobgui_scale_allocate_mark, NULL, NULL, NULL, NULL);
  g_object_set_data (G_OBJECT (mark->widget), "mark", mark);

  mark->indicator_widget = bobgui_gizmo_new ("indicator", NULL, NULL, NULL, NULL, NULL, NULL);
  bobgui_widget_set_parent (mark->indicator_widget, mark->widget);
  if (mark->markup && *mark->markup)
    {
      mark->label_widget = g_object_new (BOBGUI_TYPE_LABEL,
                                         "use-markup", TRUE,
                                         "label", mark->markup,
                                         NULL);
      if (marks_widget == priv->top_marks_widget)
        bobgui_widget_insert_after (mark->label_widget, mark->widget, NULL);
      else
        bobgui_widget_insert_before (mark->label_widget, mark->widget, NULL);
    }

  m = g_slist_find (priv->marks, mark);
  m = m->next;
  while (m)
    {
      BobguiScaleMark *next = m->data;
      if (next->position == mark->position)
        break;
      m = m->next;
    }

  if (m)
    {
      BobguiScaleMark *next = m->data;
      bobgui_widget_insert_before (mark->widget, marks_widget, next->widget);
    }
  else
    {
      bobgui_widget_set_parent (mark->widget, marks_widget);
    }

  n = g_slist_length (priv->marks);
  values = g_new (double, n);
  for (m = priv->marks, i = 0; m; m = m->next, i++)
    {
      mark = m->data;
      values[i] = mark->value;
    }

  _bobgui_range_set_stop_values (BOBGUI_RANGE (scale), values, n);

  g_free (values);

  if (priv->top_marks_widget)
    bobgui_widget_add_css_class (BOBGUI_WIDGET (scale), "marks-before");

  if (priv->bottom_marks_widget)
    bobgui_widget_add_css_class (BOBGUI_WIDGET (scale), "marks-after");

  bobgui_widget_queue_resize (widget);
}

static BobguiBuildableIface *parent_buildable_iface;

static void
bobgui_scale_buildable_interface_init (BobguiBuildableIface *iface)
{
  parent_buildable_iface = g_type_interface_peek_parent (iface);
  iface->custom_tag_start = bobgui_scale_buildable_custom_tag_start;
  iface->custom_finished = bobgui_scale_buildable_custom_finished;
}

typedef struct
{
  BobguiScale *scale;
  BobguiBuilder *builder;
  GSList *marks;
} MarksSubparserData;

typedef struct
{
  double value;
  BobguiPositionType position;
  GString *markup;
  char *context;
  gboolean translatable;
} MarkData;

static void
mark_data_free (MarkData *data)
{
  g_string_free (data->markup, TRUE);
  g_free (data->context);
  g_free (data);
}

static void
marks_start_element (BobguiBuildableParseContext *context,
                     const char               *element_name,
                     const char              **names,
                     const char              **values,
                     gpointer                  user_data,
                     GError                  **error)
{
  MarksSubparserData *data = (MarksSubparserData*)user_data;

  if (strcmp (element_name, "marks") == 0)
    {
      if (!_bobgui_builder_check_parent (data->builder, context, "object", error))
        return;

      if (!g_markup_collect_attributes (element_name, names, values, error,
                                        G_MARKUP_COLLECT_INVALID, NULL, NULL,
                                        G_MARKUP_COLLECT_INVALID))
        _bobgui_builder_prefix_error (data->builder, context, error);
    }
  else if (strcmp (element_name, "mark") == 0)
    {
      const char *value_str;
      double value = 0;
      const char *position_str = NULL;
      BobguiPositionType position = BOBGUI_POS_BOTTOM;
      const char *msg_context = NULL;
      gboolean translatable = FALSE;
      MarkData *mark;

      if (!_bobgui_builder_check_parent (data->builder, context, "marks", error))
        return;

      if (!g_markup_collect_attributes (element_name, names, values, error,
                                        G_MARKUP_COLLECT_STRING, "value", &value_str,
                                        G_MARKUP_COLLECT_BOOLEAN|G_MARKUP_COLLECT_OPTIONAL, "translatable", &translatable,
                                        G_MARKUP_COLLECT_STRING|G_MARKUP_COLLECT_OPTIONAL, "comments", NULL,
                                        G_MARKUP_COLLECT_STRING|G_MARKUP_COLLECT_OPTIONAL, "context", &msg_context,
                                        G_MARKUP_COLLECT_STRING|G_MARKUP_COLLECT_OPTIONAL, "position", &position_str,
                                        G_MARKUP_COLLECT_INVALID))
        {
          _bobgui_builder_prefix_error (data->builder, context, error);
          return;
        }

      if (value_str != NULL)
        {
          GValue gvalue = G_VALUE_INIT;

          if (!bobgui_builder_value_from_string_type (data->builder, G_TYPE_DOUBLE, value_str, &gvalue, error))
            {
              _bobgui_builder_prefix_error (data->builder, context, error);
              return;
            }

          value = g_value_get_double (&gvalue);
        }

      if (position_str != NULL)
        {
          GValue gvalue = G_VALUE_INIT;

          if (!bobgui_builder_value_from_string_type (data->builder, BOBGUI_TYPE_POSITION_TYPE, position_str, &gvalue, error))
            {
              _bobgui_builder_prefix_error (data->builder, context, error);
              return;
            }

          position = g_value_get_enum (&gvalue);
        }

      mark = g_new (MarkData, 1);
      mark->value = value;
      if (position == BOBGUI_POS_LEFT || position == BOBGUI_POS_TOP)
        mark->position = BOBGUI_POS_TOP;
      else
        mark->position = BOBGUI_POS_BOTTOM;
      mark->markup = g_string_new ("");
      mark->context = g_strdup (msg_context);
      mark->translatable = translatable;

      data->marks = g_slist_prepend (data->marks, mark);
    }
  else
    {
      _bobgui_builder_error_unhandled_tag (data->builder, context,
                                        "BobguiScale", element_name,
                                        error);
    }
}

static void
marks_text (BobguiBuildableParseContext  *context,
            const char                *text,
            gsize                      text_len,
            gpointer                   user_data,
            GError                   **error)
{
  MarksSubparserData *data = (MarksSubparserData*)user_data;

  if (strcmp (bobgui_buildable_parse_context_get_element (context), "mark") == 0)
    {
      MarkData *mark = data->marks->data;

      g_string_append_len (mark->markup, text, text_len);
    }
}

static const BobguiBuildableParser marks_parser =
  {
    marks_start_element,
    NULL,
    marks_text,
  };


static gboolean
bobgui_scale_buildable_custom_tag_start (BobguiBuildable       *buildable,
                                      BobguiBuilder         *builder,
                                      GObject            *child,
                                      const char         *tagname,
                                      BobguiBuildableParser *parser,
                                      gpointer           *parser_data)
{
  MarksSubparserData *data;

  if (child)
    return FALSE;

  if (strcmp (tagname, "marks") == 0)
    {
      data = g_new0 (MarksSubparserData, 1);
      data->scale = BOBGUI_SCALE (buildable);
      data->builder = builder;
      data->marks = NULL;

      *parser = marks_parser;
      *parser_data = data;

      return TRUE;
    }

  return parent_buildable_iface->custom_tag_start (buildable, builder, child,
                                                   tagname, parser, parser_data);
}

static void
bobgui_scale_buildable_custom_finished (BobguiBuildable *buildable,
                                     BobguiBuilder   *builder,
                                     GObject      *child,
                                     const char   *tagname,
                                     gpointer      user_data)
{
  BobguiScale *scale = BOBGUI_SCALE (buildable);
  MarksSubparserData *marks_data;

  if (strcmp (tagname, "marks") == 0)
    {
      GSList *m;
      const char *markup;

      marks_data = (MarksSubparserData *)user_data;

      for (m = marks_data->marks; m; m = m->next)
        {
          MarkData *mdata = m->data;

          if (mdata->translatable && mdata->markup->len)
            markup = _bobgui_builder_parser_translate (bobgui_builder_get_translation_domain (builder),
                                                    mdata->context,
                                                    mdata->markup->str);
          else
            markup = mdata->markup->str;

          bobgui_scale_add_mark (scale, mdata->value, mdata->position, markup);

          mark_data_free (mdata);
        }

      g_slist_free (marks_data->marks);
      g_free (marks_data);
    }
  else
    {
      parent_buildable_iface->custom_finished (buildable, builder, child,
                                               tagname, user_data);
    }

}

/**
 * bobgui_scale_set_format_value_func:
 * @scale: a `BobguiScale`
 * @func: (nullable) (scope notified) (closure user_data) (destroy destroy_notify): function
 *   that formats the value
 * @user_data: user data to pass to @func
 * @destroy_notify: (nullable): destroy function for @user_data
 *
 * @func allows you to change how the scale value is displayed.
 *
 * The given function will return an allocated string representing
 * @value. That string will then be used to display the scale's value.
 *
 * If #NULL is passed as @func, the value will be displayed on
 * its own, rounded according to the value of the
 * [property@Bobgui.Scale:digits] property.
 */
void
bobgui_scale_set_format_value_func (BobguiScale                *scale,
                                 BobguiScaleFormatValueFunc  func,
                                 gpointer                 user_data,
                                 GDestroyNotify           destroy_notify)
{
  BobguiScalePrivate *priv = bobgui_scale_get_instance_private (scale);

  g_return_if_fail (BOBGUI_IS_SCALE (scale));

  if (priv->format_value_func_destroy_notify)
    priv->format_value_func_destroy_notify (priv->format_value_func_user_data);

  priv->format_value_func = func;
  priv->format_value_func_user_data = user_data;
  priv->format_value_func_destroy_notify = destroy_notify;

  if (priv->value_widget)
    update_label_request (scale);
}
