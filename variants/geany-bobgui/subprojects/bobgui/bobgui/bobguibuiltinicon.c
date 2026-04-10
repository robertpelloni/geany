
/*
 * Copyright © 2015 Endless Mobile, Inc.
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
 * Authors: Cosimo Cecchi <cosimoc@gnome.org>
 */

#include "config.h"

#include "bobguicssnodeprivate.h"
#include "bobguicssnumbervalueprivate.h"
#include "bobguibuiltiniconprivate.h"
#include "bobguiwidgetprivate.h"
#include "bobguirendericonprivate.h"
#include "bobguisnapshot.h"

/* BobguiBuiltinIcon was a minimal widget wrapped around a BobguiBuiltinIcon gadget,
 * It should be used whenever builtin-icon functionality is desired
 * but a widget is needed for other reasons.
 */

struct _BobguiBuiltinIcon
{
  BobguiWidget parent;
};

G_DEFINE_TYPE (BobguiBuiltinIcon, bobgui_builtin_icon, BOBGUI_TYPE_WIDGET)

static void
bobgui_builtin_icon_snapshot (BobguiWidget   *widget,
                           BobguiSnapshot *snapshot)
{
  BobguiCssStyle *style = bobgui_css_node_get_style (bobgui_widget_get_css_node (widget));
  int width, height;

  width = bobgui_widget_get_width (widget);
  height = bobgui_widget_get_height (widget);

  if (width > 0 && height > 0)
    bobgui_css_style_snapshot_icon (style, snapshot, width, height);
}

static void
bobgui_builtin_icon_css_changed (BobguiWidget         *widget,
                              BobguiCssStyleChange *change)
{
  BOBGUI_WIDGET_CLASS (bobgui_builtin_icon_parent_class)->css_changed (widget, change);

  if (change == NULL ||
      bobgui_css_style_change_affects (change, BOBGUI_CSS_AFFECTS_ICON_SIZE))
    {
      bobgui_widget_queue_resize (widget);
    }
  else if (bobgui_css_style_change_affects (change, BOBGUI_CSS_AFFECTS_ICON_TEXTURE |
                                                 BOBGUI_CSS_AFFECTS_ICON_REDRAW))
    {
      bobgui_widget_queue_draw (widget);
    }
}

static void
bobgui_builtin_icon_measure (BobguiWidget      *widget,
                          BobguiOrientation  orientation,
                          int             for_size,
                          int            *minimum,
                          int            *natural,
                          int            *minimum_baseline,
                          int            *natural_baseline)
{
  BobguiCssStyle *style;

  style = bobgui_css_node_get_style (bobgui_widget_get_css_node (widget));

  *minimum = *natural = bobgui_css_number_value_get (style->icon->icon_size, 100);
}

static void
bobgui_builtin_icon_class_init (BobguiBuiltinIconClass *klass)
{
  BobguiWidgetClass *wclass = BOBGUI_WIDGET_CLASS (klass);

  wclass->snapshot = bobgui_builtin_icon_snapshot;
  wclass->measure = bobgui_builtin_icon_measure;
  wclass->css_changed = bobgui_builtin_icon_css_changed;
}

static void
bobgui_builtin_icon_init (BobguiBuiltinIcon *self)
{
}

BobguiWidget *
bobgui_builtin_icon_new (const char *css_name)
{
  return g_object_new (BOBGUI_TYPE_BUILTIN_ICON,
                       "css-name", css_name,
                       NULL);
}

void
bobgui_builtin_icon_set_css_name (BobguiBuiltinIcon *self,
                               const char     *css_name)
{
  bobgui_css_node_set_name (bobgui_widget_get_css_node (BOBGUI_WIDGET (self)),
                         g_quark_from_string (css_name));
}
