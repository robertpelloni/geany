/* BOBGUI - The Bobgui Framework
 * Copyright (C) 2011 Red Hat, Inc.
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

#include "bobguicssimagevalueprivate.h"

#include "bobguicssimagecrossfadeprivate.h"

struct _BobguiCssValue {
  BOBGUI_CSS_VALUE_BASE
  BobguiCssImage *image;
};

static void
bobgui_css_value_image_free (BobguiCssValue *value)
{
  g_object_unref (value->image);
  g_free (value);
}

static BobguiCssValue *
bobgui_css_value_image_compute (BobguiCssValue          *value,
                             guint                 property_id,
                             BobguiCssComputeContext *context)
{
  BobguiCssImage *image, *computed;
  
  image = _bobgui_css_image_value_get_image (value);

  if (image == NULL)
    return bobgui_css_value_ref (value);

  computed = _bobgui_css_image_compute (image, property_id, context);

  if (computed == image)
    {
      g_object_unref (computed);
      return bobgui_css_value_ref (value);
    }

  return _bobgui_css_image_value_new (computed);
}

static BobguiCssValue *
bobgui_css_value_image_resolve (BobguiCssValue          *value,
                             BobguiCssComputeContext *context,
                             BobguiCssValue          *current_color)
{
  if (!bobgui_css_value_contains_current_color (value))
    return bobgui_css_value_ref (value);

  return _bobgui_css_image_value_new (bobgui_css_image_resolve (_bobgui_css_image_value_get_image (value), context, current_color));
}

static gboolean
bobgui_css_value_image_equal (const BobguiCssValue *value1,
                           const BobguiCssValue *value2)
{
  return _bobgui_css_image_equal (value1->image, value2->image);
}

static BobguiCssValue *
bobgui_css_value_image_transition (BobguiCssValue *start,
                                BobguiCssValue *end,
                                guint        property_id,
                                double       progress)
{
  BobguiCssImage *transition;

  transition = _bobgui_css_image_transition (_bobgui_css_image_value_get_image (start),
                                          _bobgui_css_image_value_get_image (end),
                                          property_id,
                                          progress);
      
  return _bobgui_css_image_value_new (transition);
}

static gboolean
bobgui_css_value_image_is_dynamic (const BobguiCssValue *value)
{
  BobguiCssImage *image = _bobgui_css_image_value_get_image (value);

  if (image == NULL)
    return FALSE;

  return bobgui_css_image_is_dynamic (image);
}

static BobguiCssValue *
bobgui_css_value_image_get_dynamic_value (BobguiCssValue *value,
                                       gint64       monotonic_time)
{
  BobguiCssImage *image, *dynamic;
  
  image = _bobgui_css_image_value_get_image (value);
  if (image == NULL)
    return bobgui_css_value_ref (value);

  dynamic = bobgui_css_image_get_dynamic_image (image, monotonic_time);
  if (dynamic == image)
    {
      g_object_unref (dynamic);
      return bobgui_css_value_ref (value);
    }

  return _bobgui_css_image_value_new (dynamic);
}

static void
bobgui_css_value_image_print (const BobguiCssValue *value,
                           GString           *string)
{
  if (value->image)
    _bobgui_css_image_print (value->image, string);
  else
    g_string_append (string, "none");
}

static const BobguiCssValueClass BOBGUI_CSS_VALUE_IMAGE = {
  "BobguiCssImageValue",
  bobgui_css_value_image_free,
  bobgui_css_value_image_compute,
  bobgui_css_value_image_resolve,
  bobgui_css_value_image_equal,
  bobgui_css_value_image_transition,
  bobgui_css_value_image_is_dynamic,
  bobgui_css_value_image_get_dynamic_value,
  bobgui_css_value_image_print
};

BobguiCssValue *
_bobgui_css_image_value_new (BobguiCssImage *image)
{
  static BobguiCssValue image_none_singleton = { &BOBGUI_CSS_VALUE_IMAGE, 1, 1, 0, 0, NULL };
  BobguiCssValue *value;

  if (image == NULL)
    return bobgui_css_value_ref (&image_none_singleton);

  value = bobgui_css_value_new (BobguiCssValue, &BOBGUI_CSS_VALUE_IMAGE);
  value->image = image;
  value->is_computed = bobgui_css_image_is_computed (image);
  value->contains_current_color = bobgui_css_image_contains_current_color (image);

  return value;
}

BobguiCssImage *
_bobgui_css_image_value_get_image (const BobguiCssValue *value)
{
  g_return_val_if_fail (value->class == &BOBGUI_CSS_VALUE_IMAGE, NULL);

  return value->image;
}
