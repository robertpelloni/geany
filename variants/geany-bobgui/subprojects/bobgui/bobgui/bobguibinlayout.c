/* bobguibinlayout.c: Layout manager for bin-like widgets
 * Copyright 2019  GNOME Foundation
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
 */

/**
 * BobguiBinLayout:
 *
 * A layout manager for widgets with a single child.
 *
 * `BobguiBinLayout` will stack each child of a widget on top of each other,
 * using the [property@Bobgui.Widget:hexpand], [property@Bobgui.Widget:vexpand],
 * [property@Bobgui.Widget:halign], and [property@Bobgui.Widget:valign] properties
 * of each child to determine where they should be positioned.
 */

#include "config.h"

#include "bobguibinlayout.h"

#include "bobguiwidgetprivate.h"

struct _BobguiBinLayout
{
  BobguiLayoutManager parent_instance;
};

G_DEFINE_TYPE (BobguiBinLayout, bobgui_bin_layout, BOBGUI_TYPE_LAYOUT_MANAGER)

static void
bobgui_bin_layout_measure (BobguiLayoutManager *layout_manager,
                        BobguiWidget        *widget,
                        BobguiOrientation    orientation,
                        int               for_size,
                        int              *minimum,
                        int              *natural,
                        int              *minimum_baseline,
                        int              *natural_baseline)
{
  BobguiWidget *child;

  for (child = _bobgui_widget_get_first_child (widget);
       child != NULL;
       child = _bobgui_widget_get_next_sibling (child))
    {
      if (bobgui_widget_should_layout (child))
        {
          int child_min = 0;
          int child_nat = 0;
          int child_min_baseline = -1;
          int child_nat_baseline = -1;

          bobgui_widget_measure (child, orientation, for_size,
                              &child_min, &child_nat,
                              &child_min_baseline, &child_nat_baseline);

          *minimum = MAX (*minimum, child_min);
          *natural = MAX (*natural, child_nat);

          if (child_min_baseline > -1)
            *minimum_baseline = MAX (*minimum_baseline, child_min_baseline);
          if (child_nat_baseline > -1)
            *natural_baseline = MAX (*natural_baseline, child_nat_baseline);
        }
    }
}

static void
bobgui_bin_layout_allocate (BobguiLayoutManager *layout_manager,
                         BobguiWidget        *widget,
                         int               width,
                         int               height,
                         int               baseline)
{
  BobguiWidget *child;

  for (child = _bobgui_widget_get_first_child (widget);
       child != NULL;
       child = _bobgui_widget_get_next_sibling (child))
    {
      if (child && bobgui_widget_should_layout (child))
        bobgui_widget_allocate (child, width, height, baseline, NULL);
    }
}
static void
bobgui_bin_layout_class_init (BobguiBinLayoutClass *klass)
{
  BobguiLayoutManagerClass *layout_manager_class = BOBGUI_LAYOUT_MANAGER_CLASS (klass);

  layout_manager_class->measure = bobgui_bin_layout_measure;
  layout_manager_class->allocate = bobgui_bin_layout_allocate;
}

static void
bobgui_bin_layout_init (BobguiBinLayout *self)
{
}

/**
 * bobgui_bin_layout_new:
 *
 * Creates a new `BobguiBinLayout` instance.
 *
 * Returns: the newly created `BobguiBinLayout`
 */
BobguiLayoutManager *
bobgui_bin_layout_new (void)
{
  return g_object_new (BOBGUI_TYPE_BIN_LAYOUT, NULL);
}
