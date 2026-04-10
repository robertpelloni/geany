/* BOBGUI - The Bobgui Framework
 * Copyright © 2012 Red Hat, Inc.
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
 *
 * Author: Cosimo Cecchi <cosimoc@gnome.org>
 *
 */

/**
 * BobguiLevelBar:
 *
 * Shows a level indicator.
 *
 * Typical use cases are displaying the strength of a password, or
 * showing the charge level of a battery.
 *
 * <picture>
 *   <source srcset="levelbar-dark.png" media="(prefers-color-scheme: dark)">
 *   <img alt="An example BobguiLevelBar" src="levelbar.png">
 * </picture>
 *
 * Use [method@Bobgui.LevelBar.set_value] to set the current value, and
 * [method@Bobgui.LevelBar.add_offset_value] to set the value offsets at which
 * the bar will be considered in a different state. BOBGUI will add a few
 * offsets by default on the level bar: %BOBGUI_LEVEL_BAR_OFFSET_LOW,
 * %BOBGUI_LEVEL_BAR_OFFSET_HIGH and %BOBGUI_LEVEL_BAR_OFFSET_FULL, with
 * values 0.25, 0.75 and 1.0 respectively.
 *
 * Note that it is your responsibility to update preexisting offsets
 * when changing the minimum or maximum value. BOBGUI will simply clamp
 * them to the new range.
 *
 * ## Adding a custom offset on the bar
 *
 * ```c
 * static BobguiWidget *
 * create_level_bar (void)
 * {
 *   BobguiWidget *widget;
 *   BobguiLevelBar *bar;
 *
 *   widget = bobgui_level_bar_new ();
 *   bar = BOBGUI_LEVEL_BAR (widget);
 *
 *   // This changes the value of the default low offset
 *
 *   bobgui_level_bar_add_offset_value (bar,
 *                                   BOBGUI_LEVEL_BAR_OFFSET_LOW,
 *                                   0.10);
 *
 *   // This adds a new offset to the bar; the application will
 *   // be able to change its color CSS like this:
 *   //
 *   // levelbar block.my-offset {
 *   //   background-color: magenta;
 *   //   border-style: solid;
 *   //   border-color: black;
 *   //   border-width: 1px;
 *   // }
 *
 *   bobgui_level_bar_add_offset_value (bar, "my-offset", 0.60);
 *
 *   return widget;
 * }
 * ```
 *
 * The default interval of values is between zero and one, but it’s possible
 * to modify the interval using [method@Bobgui.LevelBar.set_min_value] and
 * [method@Bobgui.LevelBar.set_max_value]. The value will be always drawn in
 * proportion to the admissible interval, i.e. a value of 15 with a specified
 * interval between 10 and 20 is equivalent to a value of 0.5 with an interval
 * between 0 and 1. When %BOBGUI_LEVEL_BAR_MODE_DISCRETE is used, the bar level
 * is rendered as a finite number of separated blocks instead of a single one.
 * The number of blocks that will be rendered is equal to the number of units
 * specified by the admissible interval.
 *
 * For instance, to build a bar rendered with five blocks, it’s sufficient to
 * set the minimum value to 0 and the maximum value to 5 after changing the
 * indicator mode to discrete.
 *
 * # BobguiLevelBar as BobguiBuildable
 *
 * The `BobguiLevelBar` implementation of the `BobguiBuildable` interface supports a
 * custom `<offsets>` element, which can contain any number of `<offset>` elements,
 * each of which must have "name" and "value" attributes.
 *
 * # CSS nodes
 *
 * ```
 * levelbar[.discrete]
 * ╰── trough
 *     ├── block.filled.level-name
 *     ┊
 *     ├── block.empty
 *     ┊
 * ```
 *
 * `BobguiLevelBar` has a main CSS node with name levelbar and one of the style
 * classes .discrete or .continuous and a subnode with name trough. Below the
 * trough node are a number of nodes with name block and style class .filled
 * or .empty. In continuous mode, there is exactly one node of each, in discrete
 * mode, the number of filled and unfilled nodes corresponds to blocks that are
 * drawn. The block.filled nodes also get a style class .level-name corresponding
 * to the level for the current value.
 *
 * In horizontal orientation, the nodes are always arranged from left to right,
 * regardless of text direction.
 *
 * # Accessibility
 *
 * `BobguiLevelBar` uses the [enum@Bobgui.AccessibleRole.meter] role.
 */
#include "config.h"

#include "bobguiaccessiblerange.h"
#include "bobguibinlayout.h"
#include "bobguibuildable.h"
#include "bobguibuilderprivate.h"
#include "bobguigizmoprivate.h"
#include "bobguilevelbar.h"
#include "bobguimarshalers.h"
#include "bobguiorientable.h"
#include "bobguicssnodeprivate.h"
#include "bobguitypebuiltins.h"
#include "bobguiwidgetprivate.h"
#include "bobguiprivate.h"

#include <math.h>
#include <stdlib.h>

enum {
  PROP_VALUE = 1,
  PROP_MIN_VALUE,
  PROP_MAX_VALUE,
  PROP_MODE,
  PROP_INVERTED,
  LAST_PROPERTY,
  PROP_ORIENTATION /* overridden */
};

enum {
  SIGNAL_OFFSET_CHANGED,
  NUM_SIGNALS
};

static GParamSpec *properties[LAST_PROPERTY] = { NULL, };
static guint signals[NUM_SIGNALS] = { 0, };

typedef struct _BobguiLevelBarClass   BobguiLevelBarClass;

typedef struct {
  char *name;
  double value;
} BobguiLevelBarOffset;

struct _BobguiLevelBar {
  BobguiWidget parent_instance;

  BobguiOrientation orientation;

  BobguiLevelBarMode bar_mode;

  double min_value;
  double max_value;
  double cur_value;

  GList *offsets;

  BobguiWidget *trough_widget;
  BobguiWidget **block_widget;
  guint n_blocks;

  guint inverted : 1;
};

struct _BobguiLevelBarClass {
  BobguiWidgetClass parent_class;

  void (* offset_changed) (BobguiLevelBar *self,
                           const char *name);
};

static void bobgui_level_bar_set_value_internal (BobguiLevelBar *self,
                                              double       value);

static void bobgui_level_bar_buildable_init (BobguiBuildableIface *iface);

G_DEFINE_TYPE_WITH_CODE (BobguiLevelBar, bobgui_level_bar, BOBGUI_TYPE_WIDGET,
                         G_IMPLEMENT_INTERFACE (BOBGUI_TYPE_ORIENTABLE, NULL)
                         G_IMPLEMENT_INTERFACE (BOBGUI_TYPE_ACCESSIBLE_RANGE, NULL)
                         G_IMPLEMENT_INTERFACE (BOBGUI_TYPE_BUILDABLE,
                                                bobgui_level_bar_buildable_init))

static BobguiLevelBarOffset *
bobgui_level_bar_offset_new (const char *name,
                          double       value)
{
  BobguiLevelBarOffset *offset = g_new0 (BobguiLevelBarOffset, 1);

  offset->name = g_strdup (name);
  offset->value = value;

  return offset;
}

static void
bobgui_level_bar_offset_free (BobguiLevelBarOffset *offset)
{
  g_free (offset->name);
  g_free (offset);
}

static int
offset_find_func (gconstpointer data,
                  gconstpointer user_data)
{
  const BobguiLevelBarOffset *offset = data;
  const char *name = user_data;

  return g_strcmp0 (name, offset->name);
}

static int
offset_sort_func (gconstpointer a,
                  gconstpointer b)
{
  const BobguiLevelBarOffset *offset_a = a;
  const BobguiLevelBarOffset *offset_b = b;

  return (offset_a->value > offset_b->value);
}

static gboolean
bobgui_level_bar_ensure_offset (BobguiLevelBar *self,
                             const char *name,
                             double       value)
{
  GList *existing;
  BobguiLevelBarOffset *offset = NULL;
  BobguiLevelBarOffset *new_offset;

  existing = g_list_find_custom (self->offsets, name, offset_find_func);
  if (existing)
    offset = existing->data;

  if (offset && (offset->value == value))
    return FALSE;

  new_offset = bobgui_level_bar_offset_new (name, value);

  if (offset)
    {
      bobgui_level_bar_offset_free (offset);
      self->offsets = g_list_delete_link (self->offsets, existing);
    }

  self->offsets = g_list_insert_sorted (self->offsets, new_offset, offset_sort_func);

  return TRUE;
}

#ifndef G_DISABLE_CHECKS
static gboolean
bobgui_level_bar_value_in_interval (BobguiLevelBar *self,
                                 double       value)
{
  return ((value >= self->min_value) &&
          (value <= self->max_value));
}
#endif

static int
bobgui_level_bar_get_num_blocks (BobguiLevelBar *self)
{
  if (self->bar_mode == BOBGUI_LEVEL_BAR_MODE_CONTINUOUS)
    return 1;
  else if (self->bar_mode == BOBGUI_LEVEL_BAR_MODE_DISCRETE)
    return MAX (1, (int) (round (self->max_value) - round (self->min_value)));

  return 0;
}

static int
bobgui_level_bar_get_num_block_nodes (BobguiLevelBar *self)
{
  if (self->bar_mode == BOBGUI_LEVEL_BAR_MODE_CONTINUOUS)
    return 2;
  else
    return bobgui_level_bar_get_num_blocks (self);
}

static void
bobgui_level_bar_get_min_block_size (BobguiLevelBar *self,
                                  int         *block_width,
                                  int         *block_height)
{
  guint i, n_blocks;
  int width, height;

  *block_width = *block_height = 0;
  n_blocks = bobgui_level_bar_get_num_block_nodes (self);

  for (i = 0; i < n_blocks; i++)
    {
      bobgui_widget_measure (self->block_widget[i],
                          BOBGUI_ORIENTATION_HORIZONTAL,
                          -1,
                          &width, NULL,
                          NULL, NULL);
      bobgui_widget_measure (self->block_widget[i],
                          BOBGUI_ORIENTATION_VERTICAL,
                          -1,
                          &height, NULL,
                          NULL, NULL);

      *block_width = MAX (width, *block_width);
      *block_height = MAX (height, *block_height);
    }
}

static gboolean
bobgui_level_bar_get_real_inverted (BobguiLevelBar *self)
{
  if (bobgui_widget_get_direction (BOBGUI_WIDGET (self)) == BOBGUI_TEXT_DIR_RTL &&
      self->orientation == BOBGUI_ORIENTATION_HORIZONTAL)
    return !self->inverted;

  return self->inverted;
}

static void
bobgui_level_bar_render_trough (BobguiGizmo    *gizmo,
                             BobguiSnapshot *snapshot)
{
  BobguiWidget *widget = BOBGUI_WIDGET (gizmo);
  BobguiLevelBar *self = BOBGUI_LEVEL_BAR (bobgui_widget_get_parent (BOBGUI_WIDGET (gizmo)));

  if (self->bar_mode == BOBGUI_LEVEL_BAR_MODE_CONTINUOUS)
    {
      gboolean inverted;

      inverted = bobgui_level_bar_get_real_inverted (self);

      /* render the empty (unfilled) part */
      bobgui_widget_snapshot_child (widget, self->block_widget[inverted ? 0 : 1], snapshot);

      /* now render the filled part on top of it */
      if (self->cur_value != 0)
        bobgui_widget_snapshot_child (widget, self->block_widget[inverted ? 1 : 0], snapshot);
    }
  else
    {
      int num_blocks, i;

      num_blocks = bobgui_level_bar_get_num_blocks (self);

      for (i = 0; i < num_blocks; i++)
        bobgui_widget_snapshot_child (widget, self->block_widget[i], snapshot);
    }
}

static void
bobgui_level_bar_measure_trough (BobguiGizmo       *gizmo,
                              BobguiOrientation  orientation,
                              int             for_size,
                              int            *minimum,
                              int            *natural,
                              int            *minimum_baseline,
                              int            *natural_baseline)
{
  BobguiWidget *widget = BOBGUI_WIDGET (gizmo);
  BobguiLevelBar *self = BOBGUI_LEVEL_BAR (bobgui_widget_get_parent (widget));
  int num_blocks, size;
  int block_width, block_height;

  num_blocks = bobgui_level_bar_get_num_blocks (self);
  bobgui_level_bar_get_min_block_size (self, &block_width, &block_height);

  if (orientation == BOBGUI_ORIENTATION_HORIZONTAL)
    {
      if (self->orientation == BOBGUI_ORIENTATION_HORIZONTAL)
        size = num_blocks * block_width;
      else
        size = block_width;
    }
  else
    {
      if (self->orientation == BOBGUI_ORIENTATION_VERTICAL)
        size = num_blocks * block_height;
      else
        size = block_height;
    }

  *minimum = size;
  *natural = size;
}

static void
bobgui_level_bar_allocate_trough_continuous (BobguiLevelBar *self,
                                          int          width,
                                          int          height,
                                          int          baseline)
{
  BobguiAllocation block_area;
  double fill_percentage;
  gboolean inverted;
  int block_min;

  inverted = bobgui_level_bar_get_real_inverted (self);

  /* allocate the empty (unfilled) part */
  bobgui_widget_size_allocate (self->block_widget[inverted ? 0 : 1],
                            &(BobguiAllocation) {0, 0, width, height},
                            baseline);

  if (self->cur_value == 0)
    return;

  /* now allocate the filled part */
  block_area = (BobguiAllocation) {0, 0, width, height};
  fill_percentage = (self->cur_value - self->min_value) /
    (self->max_value - self->min_value);

  bobgui_widget_measure (self->block_widget[inverted ? 1 : 0],
                      self->orientation, -1,
                      &block_min, NULL,
                      NULL, NULL);

  if (self->orientation == BOBGUI_ORIENTATION_HORIZONTAL)
    {
      block_area.width = (int) floor (block_area.width * fill_percentage);
      block_area.width = MAX (block_area.width, block_min);

      if (inverted)
        block_area.x += width - block_area.width;
    }
  else
    {
      block_area.height = (int) floor (block_area.height * fill_percentage);
      block_area.height = MAX (block_area.height, block_min);

      if (inverted)
        block_area.y += height - block_area.height;
    }

  bobgui_widget_size_allocate (self->block_widget[inverted ? 1 : 0],
                            &block_area,
                            baseline);
}

static void
bobgui_level_bar_allocate_trough_discrete (BobguiLevelBar *self,
                                        int          width,
                                        int          height,
                                        int          baseline)
{
  BobguiAllocation block_area;
  int num_blocks, i;
  int block_width, block_height;
  int extra_space;

  bobgui_level_bar_get_min_block_size (self, &block_width, &block_height);
  num_blocks = bobgui_level_bar_get_num_blocks (self);

  if (num_blocks == 0)
    return;

  if (self->orientation == BOBGUI_ORIENTATION_HORIZONTAL)
    {
      block_width = MAX (block_width, (int) floor ((double) width / num_blocks));
      block_height = height;
      extra_space = width - block_width * num_blocks;

      if (extra_space > 0)
        block_width++;
    }
  else
    {
      block_width = width;
      block_height = MAX (block_height, (int) floor ((double) height / num_blocks));
      extra_space = height - block_height * num_blocks;

      if (extra_space > 0)
        block_height++;
    }

  block_area.x = 0;
  block_area.y = 0;
  block_area.width = block_width;
  block_area.height = block_height;

  for (i = 0; i < num_blocks; i++)
    {
      if (extra_space > 0 && i == extra_space)
        {
          if (self->orientation == BOBGUI_ORIENTATION_HORIZONTAL)
            block_area.width--;
          else
            block_area.height--;
        }

      bobgui_widget_size_allocate (self->block_widget[i],
                                &block_area,
                                baseline);

      if (self->orientation == BOBGUI_ORIENTATION_HORIZONTAL)
        block_area.x += block_area.width;
      else
        block_area.y += block_area.height;
    }
}

static void
bobgui_level_bar_allocate_trough (BobguiGizmo *gizmo,
                               int       width,
                               int       height,
                               int       baseline)
{
  BobguiWidget *widget = BOBGUI_WIDGET (gizmo);
  BobguiLevelBar *self = BOBGUI_LEVEL_BAR (bobgui_widget_get_parent (widget));

  if (self->bar_mode == BOBGUI_LEVEL_BAR_MODE_CONTINUOUS)
    bobgui_level_bar_allocate_trough_continuous (self, width, height, baseline);
  else
    bobgui_level_bar_allocate_trough_discrete (self, width, height, baseline);
}

static void
update_block_nodes (BobguiLevelBar *self)
{
  guint n_blocks;
  guint i;

  n_blocks = bobgui_level_bar_get_num_block_nodes (self);

  if (self->n_blocks == n_blocks)
    return;
  else if (n_blocks < self->n_blocks)
    {
      for (i = n_blocks; i < self->n_blocks; i++)
        {
          bobgui_widget_unparent (self->block_widget[i]);
        }
      self->block_widget = g_renew (BobguiWidget*, self->block_widget, n_blocks);
      self->n_blocks = n_blocks;
    }
  else
    {
      self->block_widget = g_renew (BobguiWidget*, self->block_widget, n_blocks);
      for (i = self->n_blocks; i < n_blocks; i++)
        {
          self->block_widget[i] = bobgui_gizmo_new_with_role ("block",
                                                           BOBGUI_ACCESSIBLE_ROLE_NONE,
                                                           NULL, NULL, NULL, NULL, NULL, NULL);
          bobgui_widget_insert_before (self->block_widget[i], BOBGUI_WIDGET (self->trough_widget), NULL);
        }
      self->n_blocks = n_blocks;
    }
}

static void
update_mode_style_classes (BobguiLevelBar *self)
{
  BobguiCssNode *widget_node;

  widget_node = bobgui_widget_get_css_node (BOBGUI_WIDGET (self));
  if (self->bar_mode == BOBGUI_LEVEL_BAR_MODE_CONTINUOUS)
    {
      bobgui_css_node_remove_class (widget_node, g_quark_from_static_string ("discrete"));
      bobgui_css_node_add_class (widget_node, g_quark_from_static_string ("continuous"));
    }
  else if (self->bar_mode == BOBGUI_LEVEL_BAR_MODE_DISCRETE)
    {
      bobgui_css_node_add_class (widget_node, g_quark_from_static_string ("discrete"));
      bobgui_css_node_remove_class (widget_node, g_quark_from_static_string ("continuous"));
    }
}

static void
update_level_style_classes (BobguiLevelBar *self)
{
  double value;
  const char *value_class = NULL;
  BobguiLevelBarOffset *offset, *prev_offset;
  GList *l;
  int num_filled, num_blocks, i;
  gboolean inverted;

  value = bobgui_level_bar_get_value (self);

  for (l = self->offsets; l != NULL; l = l->next)
    {
      offset = l->data;

      /* find the right offset for our style class */
      if (value <= offset->value)
        {
          if (l->prev == NULL)
            {
              value_class = offset->name;
            }
          else
            {
              prev_offset = l->prev->data;
              if (prev_offset->value < value)
                value_class = offset->name;
            }
        }

      if (value_class)
        break;
    }

  inverted = bobgui_level_bar_get_real_inverted (self);
  num_blocks = bobgui_level_bar_get_num_block_nodes (self);

  if (self->bar_mode == BOBGUI_LEVEL_BAR_MODE_CONTINUOUS)
    num_filled = 1;
  else
    num_filled = MIN (num_blocks, (int) round (self->cur_value) - (int) round (self->min_value));

  for (i = 0; i < num_filled; i++)
    {
      BobguiCssNode *node = bobgui_widget_get_css_node (self->block_widget[inverted ? num_blocks - 1 - i : i]);

      bobgui_css_node_set_classes (node, NULL);
      bobgui_css_node_add_class (node, g_quark_from_static_string ("filled"));

      if (value_class)
        bobgui_css_node_add_class (node, g_quark_from_string (value_class));
    }

  for (; i < num_blocks; i++)
    {
      BobguiCssNode *node = bobgui_widget_get_css_node (self->block_widget[inverted ? num_blocks - 1 - i : i]);

      bobgui_css_node_set_classes (node, NULL);
      bobgui_css_node_add_class (node, g_quark_from_static_string ("empty"));
    }
}

static void
bobgui_level_bar_direction_changed (BobguiWidget        *widget,
                                 BobguiTextDirection  previous_dir)
{
  BobguiLevelBar *self = BOBGUI_LEVEL_BAR (widget);

  update_level_style_classes (self);

  BOBGUI_WIDGET_CLASS (bobgui_level_bar_parent_class)->direction_changed (widget, previous_dir);
}

static void
bobgui_level_bar_ensure_offsets_in_range (BobguiLevelBar *self)
{
  BobguiLevelBarOffset *offset;
  GList *l = self->offsets;

  while (l != NULL)
    {
      offset = l->data;
      l = l->next;

      if (offset->value < self->min_value)
        bobgui_level_bar_ensure_offset (self, offset->name, self->min_value);
      else if (offset->value > self->max_value)
        bobgui_level_bar_ensure_offset (self, offset->name, self->max_value);
    }
}

typedef struct {
  BobguiLevelBar *self;
  BobguiBuilder *builder;
  GList *offsets;
} OffsetsParserData;

static void
offset_start_element (BobguiBuildableParseContext *context,
                      const char               *element_name,
                      const char              **names,
                      const char              **values,
                      gpointer                  user_data,
                      GError                  **error)
{
  OffsetsParserData *data = user_data;

  if (strcmp (element_name, "offsets") == 0)
    {
      if (!_bobgui_builder_check_parent (data->builder, context, "object", error))
        return;

      if (!g_markup_collect_attributes (element_name, names, values, error,
                                        G_MARKUP_COLLECT_INVALID, NULL, NULL,
                                        G_MARKUP_COLLECT_INVALID))
        _bobgui_builder_prefix_error (data->builder, context, error);
    }
  else if (strcmp (element_name, "offset") == 0)
    {
      const char *name;
      const char *value;
      GValue gvalue = G_VALUE_INIT;
      BobguiLevelBarOffset *offset;

      if (!_bobgui_builder_check_parent (data->builder, context, "offsets", error))
        return;

      if (!g_markup_collect_attributes (element_name, names, values, error,
                                        G_MARKUP_COLLECT_STRING, "name", &name,
                                        G_MARKUP_COLLECT_STRING, "value", &value,
                                        G_MARKUP_COLLECT_INVALID))
        {
          _bobgui_builder_prefix_error (data->builder, context, error);
          return;
        }

      if (!bobgui_builder_value_from_string_type (data->builder, G_TYPE_DOUBLE, value, &gvalue, error))
        {
          _bobgui_builder_prefix_error (data->builder, context, error);
          return;
        }

      offset = bobgui_level_bar_offset_new (name, g_value_get_double (&gvalue));
      data->offsets = g_list_prepend (data->offsets, offset);
    }
  else
    {
      _bobgui_builder_error_unhandled_tag (data->builder, context,
                                        "BobguiLevelBar", element_name,
                                        error);
    }
}

static const BobguiBuildableParser offset_parser =
{
  offset_start_element
};

static BobguiBuildableIface *parent_buildable_iface;

static gboolean
bobgui_level_bar_buildable_custom_tag_start (BobguiBuildable       *buildable,
                                          BobguiBuilder         *builder,
                                          GObject            *child,
                                          const char         *tagname,
                                          BobguiBuildableParser *parser,
                                          gpointer           *parser_data)
{
  OffsetsParserData *data;

  if (parent_buildable_iface->custom_tag_start (buildable, builder, child,
                                                tagname, parser, parser_data))
    return TRUE;

  if (child)
    return FALSE;

  if (strcmp (tagname, "offsets") != 0)
    return FALSE;

  data = g_new0 (OffsetsParserData, 1);
  data->self = BOBGUI_LEVEL_BAR (buildable);
  data->builder = builder;
  data->offsets = NULL;

  *parser = offset_parser;
  *parser_data = data;

  return TRUE;
}

static void
bobgui_level_bar_buildable_custom_finished (BobguiBuildable *buildable,
                                         BobguiBuilder   *builder,
                                         GObject      *child,
                                         const char   *tagname,
                                         gpointer      user_data)
{
  OffsetsParserData *data = user_data;
  BobguiLevelBar *self;
  BobguiLevelBarOffset *offset;
  GList *l;

  self = data->self;

  if (strcmp (tagname, "offsets") != 0)
    {
      parent_buildable_iface->custom_finished (buildable, builder, child,
                                               tagname, user_data);
      return;
    }

  for (l = data->offsets; l != NULL; l = l->next)
    {
      offset = l->data;
      bobgui_level_bar_add_offset_value (self, offset->name, offset->value);
    }

  g_list_free_full (data->offsets, (GDestroyNotify) bobgui_level_bar_offset_free);
  g_free (data);
}

static void
bobgui_level_bar_buildable_init (BobguiBuildableIface *iface)
{
  parent_buildable_iface = g_type_interface_peek_parent (iface);

  iface->custom_tag_start = bobgui_level_bar_buildable_custom_tag_start;
  iface->custom_finished = bobgui_level_bar_buildable_custom_finished;
}

static void
bobgui_level_bar_set_orientation (BobguiLevelBar    *self,
                               BobguiOrientation  orientation)
{
  if (self->orientation != orientation)
    {
      self->orientation = orientation;
      bobgui_widget_update_orientation (BOBGUI_WIDGET (self), self->orientation);
      bobgui_widget_queue_resize (BOBGUI_WIDGET (self));
      g_object_notify (G_OBJECT (self), "orientation");
    }
}

static void
bobgui_level_bar_get_property (GObject    *obj,
                            guint       property_id,
                            GValue     *value,
                            GParamSpec *pspec)
{
  BobguiLevelBar *self = BOBGUI_LEVEL_BAR (obj);

  switch (property_id)
    {
    case PROP_VALUE:
      g_value_set_double (value, bobgui_level_bar_get_value (self));
      break;
    case PROP_MIN_VALUE:
      g_value_set_double (value, bobgui_level_bar_get_min_value (self));
      break;
    case PROP_MAX_VALUE:
      g_value_set_double (value, bobgui_level_bar_get_max_value (self));
      break;
    case PROP_MODE:
      g_value_set_enum (value, bobgui_level_bar_get_mode (self));
      break;
    case PROP_INVERTED:
      g_value_set_boolean (value, bobgui_level_bar_get_inverted (self));
      break;
    case PROP_ORIENTATION:
      g_value_set_enum (value, self->orientation);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, property_id, pspec);
      break;
    }
}

static void
bobgui_level_bar_set_property (GObject      *obj,
                            guint         property_id,
                            const GValue *value,
                            GParamSpec   *pspec)
{
  BobguiLevelBar *self = BOBGUI_LEVEL_BAR (obj);

  switch (property_id)
    {
    case PROP_VALUE:
      bobgui_level_bar_set_value (self, g_value_get_double (value));
      break;
    case PROP_MIN_VALUE:
      bobgui_level_bar_set_min_value (self, g_value_get_double (value));
      break;
    case PROP_MAX_VALUE:
      bobgui_level_bar_set_max_value (self, g_value_get_double (value));
      break;
    case PROP_MODE:
      bobgui_level_bar_set_mode (self, g_value_get_enum (value));
      break;
    case PROP_INVERTED:
      bobgui_level_bar_set_inverted (self, g_value_get_boolean (value));
      break;
    case PROP_ORIENTATION:
      bobgui_level_bar_set_orientation (self, g_value_get_enum (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, property_id, pspec);
      break;
    }
}

static void
bobgui_level_bar_finalize (GObject *obj)
{
  BobguiLevelBar *self = BOBGUI_LEVEL_BAR (obj);
  int i;

  g_list_free_full (self->offsets, (GDestroyNotify) bobgui_level_bar_offset_free);

  for (i = 0; i < self->n_blocks; i++)
    bobgui_widget_unparent (self->block_widget[i]);

  g_free (self->block_widget);

  bobgui_widget_unparent (self->trough_widget);

  G_OBJECT_CLASS (bobgui_level_bar_parent_class)->finalize (obj);
}

static void
bobgui_level_bar_class_init (BobguiLevelBarClass *klass)
{
  GObjectClass *oclass = G_OBJECT_CLASS (klass);
  BobguiWidgetClass *wclass = BOBGUI_WIDGET_CLASS (klass);

  oclass->get_property = bobgui_level_bar_get_property;
  oclass->set_property = bobgui_level_bar_set_property;
  oclass->finalize = bobgui_level_bar_finalize;

  wclass->direction_changed = bobgui_level_bar_direction_changed;

  g_object_class_override_property (oclass, PROP_ORIENTATION, "orientation");

  /**
   * BobguiLevelBar::offset-changed:
   * @self: a `BobguiLevelBar`
   * @name: the name of the offset that changed value
   *
   * Emitted when an offset specified on the bar changes value.
   *
   * This typically is the result of a [method@Bobgui.LevelBar.add_offset_value]
   * call.
   *
   * The signal supports detailed connections; you can connect to the
   * detailed signal "changed::x" in order to only receive callbacks when
   * the value of offset "x" changes.
   */
  signals[SIGNAL_OFFSET_CHANGED] =
    g_signal_new (I_("offset-changed"),
                  BOBGUI_TYPE_LEVEL_BAR,
                  G_SIGNAL_RUN_FIRST | G_SIGNAL_DETAILED,
                  G_STRUCT_OFFSET (BobguiLevelBarClass, offset_changed),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE,
                  1, G_TYPE_STRING);

  /**
   * BobguiLevelBar:value:
   *
   * Determines the currently filled value of the level bar.
   */
  properties[PROP_VALUE] =
    g_param_spec_double ("value", NULL, NULL,
                         0.0, G_MAXDOUBLE, 0.0,
                         G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiLevelBar:min-value:
   *
   * Determines the minimum value of the interval that can be displayed by the bar.
   */
  properties[PROP_MIN_VALUE] =
    g_param_spec_double ("min-value", NULL, NULL,
                         0.0, G_MAXDOUBLE, 0.0,
                         G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiLevelBar:max-value:
   *
   * Determines the maximum value of the interval that can be displayed by the bar.
   */
  properties[PROP_MAX_VALUE] =
    g_param_spec_double ("max-value", NULL, NULL,
                         0.0, G_MAXDOUBLE, 1.0,
                         G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiLevelBar:mode:
   *
   * Determines the way `BobguiLevelBar` interprets the value properties to draw the
   * level fill area.
   *
   * Specifically, when the value is %BOBGUI_LEVEL_BAR_MODE_CONTINUOUS,
   * `BobguiLevelBar` will draw a single block representing the current value in
   * that area; when the value is %BOBGUI_LEVEL_BAR_MODE_DISCRETE,
   * the widget will draw a succession of separate blocks filling the
   * draw area, with the number of blocks being equal to the units separating
   * the integral roundings of [property@Bobgui.LevelBar:min-value] and
   * [property@Bobgui.LevelBar:max-value].
   */
  properties[PROP_MODE] =
    g_param_spec_enum ("mode", NULL, NULL,
                       BOBGUI_TYPE_LEVEL_BAR_MODE,
                       BOBGUI_LEVEL_BAR_MODE_CONTINUOUS,
                       G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiLevelBar:inverted:
   *
   * Whether the `BobguiLeveBar` is inverted.
   *
   * Level bars normally grow from top to bottom or left to right.
   * Inverted level bars grow in the opposite direction.
   */
  properties[PROP_INVERTED] =
    g_param_spec_boolean ("inverted", NULL, NULL,
                          FALSE,
                          G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS|G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (oclass, LAST_PROPERTY, properties);

  bobgui_widget_class_set_layout_manager_type (wclass, BOBGUI_TYPE_BIN_LAYOUT);
  bobgui_widget_class_set_css_name (wclass, I_("levelbar"));
  bobgui_widget_class_set_accessible_role (wclass, BOBGUI_ACCESSIBLE_ROLE_METER);
}

static void
bobgui_level_bar_init (BobguiLevelBar *self)
{
  self->cur_value = 0.0;
  self->min_value = 0.0;
  self->max_value = 1.0;

  /* set initial orientation and style classes */
  self->orientation = BOBGUI_ORIENTATION_HORIZONTAL;
  bobgui_widget_update_orientation (BOBGUI_WIDGET (self), self->orientation);

  self->inverted = FALSE;

  self->trough_widget = bobgui_gizmo_new_with_role ("trough",
                                                 BOBGUI_ACCESSIBLE_ROLE_NONE,
                                                 bobgui_level_bar_measure_trough,
                                                 bobgui_level_bar_allocate_trough,
                                                 bobgui_level_bar_render_trough,
                                                 NULL,
                                                 NULL, NULL);
  bobgui_widget_set_parent (self->trough_widget, BOBGUI_WIDGET (self));

  bobgui_level_bar_ensure_offset (self, BOBGUI_LEVEL_BAR_OFFSET_LOW, 0.25);
  bobgui_level_bar_ensure_offset (self, BOBGUI_LEVEL_BAR_OFFSET_HIGH, 0.75);
  bobgui_level_bar_ensure_offset (self, BOBGUI_LEVEL_BAR_OFFSET_FULL, 1.0);

  self->block_widget = NULL;
  self->n_blocks = 0;

  self->bar_mode = BOBGUI_LEVEL_BAR_MODE_CONTINUOUS;
  update_mode_style_classes (self);
  update_block_nodes (self);
  update_level_style_classes (self);

  bobgui_accessible_update_property (BOBGUI_ACCESSIBLE (self),
                                  BOBGUI_ACCESSIBLE_PROPERTY_VALUE_MAX, 1.0,
                                  BOBGUI_ACCESSIBLE_PROPERTY_VALUE_MIN, 0.0,
                                  BOBGUI_ACCESSIBLE_PROPERTY_VALUE_NOW, 0.0,
                                  -1);
}

/**
 * bobgui_level_bar_new:
 *
 * Creates a new `BobguiLevelBar`.
 *
 * Returns: a `BobguiLevelBar`.
 */
BobguiWidget *
bobgui_level_bar_new (void)
{
  return g_object_new (BOBGUI_TYPE_LEVEL_BAR, NULL);
}

/**
 * bobgui_level_bar_new_for_interval:
 * @min_value: a positive value
 * @max_value: a positive value
 *
 * Creates a new `BobguiLevelBar` for the specified interval.
 *
 * Returns: a `BobguiLevelBar`
 */
BobguiWidget *
bobgui_level_bar_new_for_interval (double min_value,
                                double max_value)
{
  return g_object_new (BOBGUI_TYPE_LEVEL_BAR,
                       "min-value", min_value,
                       "max-value", max_value,
                       NULL);
}

/**
 * bobgui_level_bar_get_min_value:
 * @self: a `BobguiLevelBar`
 *
 * Returns the `min-value` of the `BobguiLevelBar`.
 *
 * Returns: a positive value
 */
double
bobgui_level_bar_get_min_value (BobguiLevelBar *self)
{
  g_return_val_if_fail (BOBGUI_IS_LEVEL_BAR (self), 0.0);

  return self->min_value;
}

/**
 * bobgui_level_bar_get_max_value:
 * @self: a `BobguiLevelBar`
 *
 * Returns the `max-value` of the `BobguiLevelBar`.
 *
 * Returns: a positive value
 */
double
bobgui_level_bar_get_max_value (BobguiLevelBar *self)
{
  g_return_val_if_fail (BOBGUI_IS_LEVEL_BAR (self), 0.0);

  return self->max_value;
}

/**
 * bobgui_level_bar_get_value:
 * @self: a `BobguiLevelBar`
 *
 * Returns the `value` of the `BobguiLevelBar`.
 *
 * Returns: a value in the interval between
 *   [property@Bobgui.LevelBar:min-value] and [property@Bobgui.LevelBar:max-value]
 */
double
bobgui_level_bar_get_value (BobguiLevelBar *self)
{
  g_return_val_if_fail (BOBGUI_IS_LEVEL_BAR (self), 0.0);

  return self->cur_value;
}

static void
bobgui_level_bar_set_value_internal (BobguiLevelBar *self,
                                  double       value)
{
  self->cur_value = value;

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_VALUE]);

  bobgui_widget_queue_allocate (BOBGUI_WIDGET (self->trough_widget));
}

/**
 * bobgui_level_bar_set_min_value:
 * @self: a `BobguiLevelBar`
 * @value: a positive value
 *
 * Sets the `min-value` of the `BobguiLevelBar`.
 *
 * You probably want to update preexisting level offsets after calling
 * this function.
 */
void
bobgui_level_bar_set_min_value (BobguiLevelBar *self,
                             double       value)
{
  g_return_if_fail (BOBGUI_IS_LEVEL_BAR (self));
  g_return_if_fail (value >= 0.0);

  if (value == self->min_value)
    return;

  self->min_value = value;

  if (self->min_value > self->cur_value)
    bobgui_level_bar_set_value_internal (self, self->min_value);
  else
    bobgui_widget_queue_allocate (BOBGUI_WIDGET (self->trough_widget));

  update_block_nodes (self);
  update_level_style_classes (self);

  bobgui_accessible_update_property (BOBGUI_ACCESSIBLE (self),
                                  BOBGUI_ACCESSIBLE_PROPERTY_VALUE_MIN, self->min_value,
                                  BOBGUI_ACCESSIBLE_PROPERTY_VALUE_NOW, self->cur_value,
                                  -1);

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_MIN_VALUE]);
}

/**
 * bobgui_level_bar_set_max_value:
 * @self: a `BobguiLevelBar`
 * @value: a positive value
 *
 * Sets the `max-value` of the `BobguiLevelBar`.
 *
 * You probably want to update preexisting level offsets after calling
 * this function.
 */
void
bobgui_level_bar_set_max_value (BobguiLevelBar *self,
                             double       value)
{
  g_return_if_fail (BOBGUI_IS_LEVEL_BAR (self));
  g_return_if_fail (value >= 0.0);

  if (value == self->max_value)
    return;

  self->max_value = value;

  if (self->max_value < self->cur_value)
    bobgui_level_bar_set_value_internal (self, self->max_value);
  else
    bobgui_widget_queue_allocate (BOBGUI_WIDGET (self->trough_widget));

  bobgui_level_bar_ensure_offsets_in_range (self);
  update_block_nodes (self);
  update_level_style_classes (self);

  bobgui_accessible_update_property (BOBGUI_ACCESSIBLE (self),
                                  BOBGUI_ACCESSIBLE_PROPERTY_VALUE_MAX, self->max_value,
                                  BOBGUI_ACCESSIBLE_PROPERTY_VALUE_NOW, self->cur_value,
                                  -1);

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_MAX_VALUE]);
}

/**
 * bobgui_level_bar_set_value:
 * @self: a `BobguiLevelBar`
 * @value: a value in the interval between
 *   [property@Bobgui.LevelBar:min-value] and [property@Bobgui.LevelBar:max-value]
 *
 * Sets the value of the `BobguiLevelBar`.
 */
void
bobgui_level_bar_set_value (BobguiLevelBar *self,
                         double       value)
{
  g_return_if_fail (BOBGUI_IS_LEVEL_BAR (self));

  if (value == self->cur_value)
    return;

  bobgui_level_bar_set_value_internal (self, value);
  update_level_style_classes (self);

  bobgui_accessible_update_property (BOBGUI_ACCESSIBLE (self),
                                  BOBGUI_ACCESSIBLE_PROPERTY_VALUE_NOW, self->cur_value,
                                  -1);
}

/**
 * bobgui_level_bar_get_mode:
 * @self: a `BobguiLevelBar`
 *
 * Returns the `mode` of the `BobguiLevelBar`.
 *
 * Returns: a `BobguiLevelBarMode`
 */
BobguiLevelBarMode
bobgui_level_bar_get_mode (BobguiLevelBar *self)
{
  g_return_val_if_fail (BOBGUI_IS_LEVEL_BAR (self), 0);

  return self->bar_mode;
}

/**
 * bobgui_level_bar_set_mode:
 * @self: a `BobguiLevelBar`
 * @mode: a `BobguiLevelBarMode`
 *
 * Sets the `mode` of the `BobguiLevelBar`.
 */
void
bobgui_level_bar_set_mode (BobguiLevelBar     *self,
                        BobguiLevelBarMode  mode)
{
  g_return_if_fail (BOBGUI_IS_LEVEL_BAR (self));

  if (self->bar_mode == mode)
    return;

  self->bar_mode = mode;

  update_mode_style_classes (self);
  update_block_nodes (self);
  update_level_style_classes (self);
  bobgui_widget_queue_resize (BOBGUI_WIDGET (self));
  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_MODE]);

}

/**
 * bobgui_level_bar_get_inverted:
 * @self: a `BobguiLevelBar`
 *
 * Returns whether the levelbar is inverted.
 *
 * Returns: %TRUE if the level bar is inverted
 */
gboolean
bobgui_level_bar_get_inverted (BobguiLevelBar *self)
{
  g_return_val_if_fail (BOBGUI_IS_LEVEL_BAR (self), FALSE);

  return self->inverted;
}

/**
 * bobgui_level_bar_set_inverted:
 * @self: a `BobguiLevelBar`
 * @inverted: %TRUE to invert the level bar
 *
 * Sets whether the `BobguiLevelBar` is inverted.
 */
void
bobgui_level_bar_set_inverted (BobguiLevelBar *self,
                            gboolean     inverted)
{
  g_return_if_fail (BOBGUI_IS_LEVEL_BAR (self));

  if (self->inverted == inverted)
    return;

  self->inverted = inverted;
  bobgui_widget_queue_resize (BOBGUI_WIDGET (self));
  update_level_style_classes (self);
  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_INVERTED]);
}

/**
 * bobgui_level_bar_remove_offset_value:
 * @self: a `BobguiLevelBar`
 * @name: (nullable): the name of an offset in the bar
 *
 * Removes an offset marker from a `BobguiLevelBar`.
 *
 * The marker must have been previously added with
 * [method@Bobgui.LevelBar.add_offset_value].
 */
void
bobgui_level_bar_remove_offset_value (BobguiLevelBar *self,
                                   const char *name)
{
  GList *existing;

  g_return_if_fail (BOBGUI_IS_LEVEL_BAR (self));

  existing = g_list_find_custom (self->offsets, name, offset_find_func);
  if (existing)
    {
      bobgui_level_bar_offset_free (existing->data);
      self->offsets = g_list_delete_link (self->offsets, existing);

      update_level_style_classes (self);
    }
}

/**
 * bobgui_level_bar_add_offset_value:
 * @self: a `BobguiLevelBar`
 * @name: the name of the new offset
 * @value: the value for the new offset
 *
 * Adds a new offset marker on @self at the position specified by @value.
 *
 * When the bar value is in the interval topped by @value (or between @value
 * and [property@Bobgui.LevelBar:max-value] in case the offset is the last one
 * on the bar) a style class named `level-`@name will be applied
 * when rendering the level bar fill.
 *
 * If another offset marker named @name exists, its value will be
 * replaced by @value.
 */
void
bobgui_level_bar_add_offset_value (BobguiLevelBar *self,
                                const char *name,
                                double       value)
{
  GQuark name_quark;

  g_return_if_fail (BOBGUI_IS_LEVEL_BAR (self));
  g_return_if_fail (bobgui_level_bar_value_in_interval (self, value));

  if (!bobgui_level_bar_ensure_offset (self, name, value))
    return;

  update_level_style_classes (self);
  name_quark = g_quark_from_string (name);
  g_signal_emit (self, signals[SIGNAL_OFFSET_CHANGED], name_quark, name);
}

/**
 * bobgui_level_bar_get_offset_value:
 * @self: a `BobguiLevelBar`
 * @name: (nullable): the name of an offset in the bar
 * @value: (out): location where to store the value
 *
 * Fetches the value specified for the offset marker @name in @self.
 *
 * Returns: %TRUE if the specified offset is found
 */
gboolean
bobgui_level_bar_get_offset_value (BobguiLevelBar *self,
                                const char *name,
                                double      *value)
{
  GList *existing;
  BobguiLevelBarOffset *offset = NULL;

  g_return_val_if_fail (BOBGUI_IS_LEVEL_BAR (self), FALSE);

  existing = g_list_find_custom (self->offsets, name, offset_find_func);
  if (existing)
    offset = existing->data;

  if (!offset)
    return FALSE;

  if (value)
    *value = offset->value;

  return TRUE;
}
