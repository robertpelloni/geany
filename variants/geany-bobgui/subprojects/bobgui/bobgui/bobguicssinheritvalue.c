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

#include "bobguicssinheritvalueprivate.h"

#include "bobguicssinitialvalueprivate.h"
#include "bobguicssstyleprivate.h"

struct _BobguiCssValue {
  BOBGUI_CSS_VALUE_BASE
};

static void G_GNUC_NORETURN
bobgui_css_value_inherit_free (BobguiCssValue *value)
{
  /* Can only happen if the unique value gets unreffed too often */
  g_assert_not_reached ();
}

static BobguiCssValue *
bobgui_css_value_inherit_compute (BobguiCssValue          *value,
                               guint                 property_id,
                               BobguiCssComputeContext *context)
{
  if (context->parent_style)
    {
      return bobgui_css_value_ref (bobgui_css_style_get_value (context->parent_style, property_id));
    }
  else
    {
      return bobgui_css_value_compute (_bobgui_css_initial_value_get (),
                                    property_id,
                                    context);
    }
}

static gboolean
bobgui_css_value_inherit_equal (const BobguiCssValue *value1,
                             const BobguiCssValue *value2)
{
  return TRUE;
}

static BobguiCssValue *
bobgui_css_value_inherit_transition (BobguiCssValue *start,
                                  BobguiCssValue *end,
                                  guint        property_id,
                                  double       progress)
{
  return NULL;
}

static void
bobgui_css_value_inherit_print (const BobguiCssValue *value,
                             GString           *string)
{
  g_string_append (string, "inherit");
}

static const BobguiCssValueClass BOBGUI_CSS_VALUE_INHERIT = {
  "BobguiCssInheritValue",
  bobgui_css_value_inherit_free,
  bobgui_css_value_inherit_compute,
  NULL,
  bobgui_css_value_inherit_equal,
  bobgui_css_value_inherit_transition,
  NULL,
  NULL,
  bobgui_css_value_inherit_print
};

static BobguiCssValue inherit = { &BOBGUI_CSS_VALUE_INHERIT, 1 };

BobguiCssValue *
_bobgui_css_inherit_value_new (void)
{
  return bobgui_css_value_ref (&inherit);
}

BobguiCssValue *
_bobgui_css_inherit_value_get (void)
{
  return &inherit;
}
