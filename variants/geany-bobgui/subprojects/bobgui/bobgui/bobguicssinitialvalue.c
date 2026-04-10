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

#include "bobguicssinitialvalueprivate.h"

#include "bobguicssarrayvalueprivate.h"
#include "bobguicssnumbervalueprivate.h"
#include "bobguicssstringvalueprivate.h"
#include "bobguicssstylepropertyprivate.h"
#include "bobguisettingsprivate.h"
#include "bobguistyleproviderprivate.h"

struct _BobguiCssValue {
  BOBGUI_CSS_VALUE_BASE
};

static void G_GNUC_NORETURN
bobgui_css_value_initial_free (BobguiCssValue *value)
{
  /* Can only happen if the unique value gets unreffed too often */
  g_assert_not_reached ();
}

static BobguiCssValue *
bobgui_css_value_initial_compute (BobguiCssValue          *value,
                               guint                 property_id,
                               BobguiCssComputeContext *context)
{
  BobguiSettings *settings;

  switch (property_id)
    {
    case BOBGUI_CSS_PROPERTY_DPI:
      settings = bobgui_style_provider_get_settings (context->provider);
      if (settings)
        {
          int dpi_int;

          g_object_get (settings, "bobgui-xft-dpi", &dpi_int, NULL);

          if (dpi_int > 0.0)
            return bobgui_css_number_value_new (dpi_int / 1024., BOBGUI_CSS_NUMBER);
        }
      break;

    case BOBGUI_CSS_PROPERTY_FONT_FAMILY:
      settings = bobgui_style_provider_get_settings (context->provider);
      if (settings && bobgui_settings_get_font_family (settings) != NULL)
        return _bobgui_css_array_value_new (_bobgui_css_string_value_new (bobgui_settings_get_font_family (settings)));
      break;

    default:
      break;
    }

  return bobgui_css_value_compute (_bobgui_css_style_property_get_initial_value (_bobgui_css_style_property_lookup_by_id (property_id)),
                                 property_id,
                                 context);
}

static gboolean
bobgui_css_value_initial_equal (const BobguiCssValue *value1,
                             const BobguiCssValue *value2)
{
  return TRUE;
}

static BobguiCssValue *
bobgui_css_value_initial_transition (BobguiCssValue *start,
                                  BobguiCssValue *end,
                                  guint        property_id,
                                  double       progress)
{
  return NULL;
}

static void
bobgui_css_value_initial_print (const BobguiCssValue *value,
                             GString           *string)
{
  g_string_append (string, "initial");
}

static const BobguiCssValueClass BOBGUI_CSS_VALUE_INITIAL = {
  "BobguiCssInitialValue",
  bobgui_css_value_initial_free,
  bobgui_css_value_initial_compute,
  NULL,
  bobgui_css_value_initial_equal,
  bobgui_css_value_initial_transition,
  NULL,
  NULL,
  bobgui_css_value_initial_print
};

static BobguiCssValue initial = { &BOBGUI_CSS_VALUE_INITIAL, 1 };

BobguiCssValue *
_bobgui_css_initial_value_new (void)
{
  return bobgui_css_value_ref (&initial);
}

BobguiCssValue *
_bobgui_css_initial_value_get (void)
{
  return &initial;
}
BobguiCssValue *
_bobgui_css_initial_value_new_compute (guint                 property_id,
                                    BobguiCssComputeContext *context)
{
  return bobgui_css_value_initial_compute (NULL,
                                        property_id,
                                        context);
}
