/*
 * Copyright © 2018 Red Hat Inc.
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
 * Authors: Benjamin Otte <otte@gnome.org>
 */

#include "config.h"

#include "bobguicssdynamicprivate.h"
#include "bobguiprogresstrackerprivate.h"

struct _BobguiCssDynamic
{
  BobguiStyleAnimation parent;
  gint64            timestamp;
};

static BobguiStyleAnimation *
bobgui_css_dynamic_advance (BobguiStyleAnimation    *style_animation,
                         gint64                timestamp)
{
  return bobgui_css_dynamic_new (timestamp);
}

static void
bobgui_css_dynamic_apply_values (BobguiStyleAnimation    *style_animation,
                              BobguiCssAnimatedStyle  *style)
{
  BobguiCssDynamic *dynamic = (BobguiCssDynamic *)style_animation;
  guint i;

  for (i = 0; i < BOBGUI_CSS_PROPERTY_N_PROPERTIES; i++)
    {
      BobguiCssValue *value, *dynamic_value;

      value = bobgui_css_style_get_value (BOBGUI_CSS_STYLE (style), i);
      dynamic_value = bobgui_css_value_get_dynamic_value (value, dynamic->timestamp);
      if (value != dynamic_value)
        bobgui_css_animated_style_set_animated_value (style, i, dynamic_value);
      else
        bobgui_css_value_unref (dynamic_value);
    }
}

static gboolean
bobgui_css_dynamic_is_finished (BobguiStyleAnimation *style_animation)
{
  return FALSE;
}

static gboolean
bobgui_css_dynamic_is_static (BobguiStyleAnimation *style_animation)
{
  return FALSE;
}

static void
bobgui_css_dynamic_free (BobguiStyleAnimation *animation)
{
  g_free (animation);
}

static const BobguiStyleAnimationClass BOBGUI_CSS_DYNAMIC_CLASS = {
  "BobguiCssDynamic",
  bobgui_css_dynamic_free,
  bobgui_css_dynamic_is_finished,
  bobgui_css_dynamic_is_static,
  bobgui_css_dynamic_apply_values,
  bobgui_css_dynamic_advance,
};

BobguiStyleAnimation *
bobgui_css_dynamic_new (gint64 timestamp)
{
  BobguiCssDynamic *dynamic = g_new (BobguiCssDynamic, 1);

  dynamic->parent.class = &BOBGUI_CSS_DYNAMIC_CLASS;
  dynamic->parent.ref_count = 1;
  dynamic->timestamp = timestamp;

  return (BobguiStyleAnimation *)dynamic;
}

