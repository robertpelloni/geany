/*
 * Copyright © 2012 Red Hat Inc.
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

#include "bobguicssimageiconthemeprivate.h"

#include <math.h>

#include "bobgui/css/bobguicssserializerprivate.h"
#include "bobguisettingsprivate.h"
#include "bobguisnapshot.h"
#include "bobguistyleproviderprivate.h"
#include "bobguisymbolicpaintable.h"
#include "bobguiiconthemeprivate.h"
#include "bobguicsscolorvalueprivate.h"
#include "bobguicsspalettevalueprivate.h"

G_DEFINE_TYPE (BobguiCssImageIconTheme, _bobgui_css_image_icon_theme, BOBGUI_TYPE_CSS_IMAGE)

static double
bobgui_css_image_icon_theme_get_aspect_ratio (BobguiCssImage *image)
{
  /* icon theme icons only take a single size when requesting, so we insist on being square */
  return 1.0;
}

static void
bobgui_css_image_icon_theme_snapshot (BobguiCssImage *image,
                                   BobguiSnapshot *snapshot,
                                   double       width,
                                   double       height)
{
  BobguiCssImageIconTheme *icon_theme = BOBGUI_CSS_IMAGE_ICON_THEME (image);
  BobguiIconPaintable *icon;
  double icon_width, icon_height;
  int size;
  double x, y;
  GdkRGBA colors[4];

  size = floor (MIN (width, height));
  if (size <= 0)
    return;

  if (size == icon_theme->cached_size &&
      icon_theme->cached_icon != NULL)
    {
      icon = icon_theme->cached_icon;
    }
  else
    {
      icon = bobgui_icon_theme_lookup_icon (icon_theme->icon_theme,
                                         icon_theme->name,
                                         NULL,
                                         size,
                                         icon_theme->scale,
                                         BOBGUI_TEXT_DIR_NONE,
                                         0);

      g_assert (icon != NULL);

      g_clear_object (&icon_theme->cached_icon);

      icon_theme->cached_size = size;
      icon_theme->cached_icon = icon;
    }

  icon_width = (double) MIN (gdk_paintable_get_intrinsic_width (GDK_PAINTABLE (icon)), width);
  icon_height = (double) MIN (gdk_paintable_get_intrinsic_height (GDK_PAINTABLE (icon)), height);

  x = (width - icon_width) / 2;
  y = (height - icon_height) / 2;

  if (x != 0 || y != 0)
    {
      bobgui_snapshot_save (snapshot);
      bobgui_snapshot_translate (snapshot, &GRAPHENE_POINT_INIT (x, y));
    }

  for (guint i = 0; i < 4; i++)
    colors[i] = *bobgui_css_color_value_get_rgba (icon_theme->colors[i]);

  bobgui_symbolic_paintable_snapshot_symbolic (BOBGUI_SYMBOLIC_PAINTABLE (icon),
                                            snapshot,
                                            icon_width,
                                            icon_height,
                                            colors,
                                            G_N_ELEMENTS (icon_theme->colors));
  if (x != 0 || y != 0)
    bobgui_snapshot_restore (snapshot);
}

static guint
bobgui_css_image_icon_theme_parse_arg (BobguiCssParser *parser,
                                    guint         arg,
                                    gpointer      data)
{
  BobguiCssImageIconTheme *icon_theme = data;

  icon_theme->name = bobgui_css_parser_consume_string (parser);
  if (icon_theme->name == NULL)
    return 0;

  return 1;
}

static gboolean
bobgui_css_image_icon_theme_parse (BobguiCssImage  *image,
                                BobguiCssParser *parser)
{
  if (!bobgui_css_parser_has_function (parser, "-bobgui-icontheme"))
    {
      bobgui_css_parser_error_syntax (parser, "Expected '-bobgui-icontheme('");
      return FALSE;
    }

  return bobgui_css_parser_consume_function (parser, 1, 1, bobgui_css_image_icon_theme_parse_arg, image);
}

static void
bobgui_css_image_icon_theme_print (BobguiCssImage *image,
                                GString     *string)
{
  BobguiCssImageIconTheme *icon_theme = BOBGUI_CSS_IMAGE_ICON_THEME (image);

  g_string_append (string, "-bobgui-icontheme(");
  bobgui_css_print_string (string, icon_theme->name, FALSE);
  g_string_append (string, ")");
}

static BobguiCssImage *
bobgui_css_image_icon_theme_compute (BobguiCssImage          *image,
                                  guint                 property_id,
                                  BobguiCssComputeContext *context)
{
  BobguiCssImageIconTheme *icon_theme = BOBGUI_CSS_IMAGE_ICON_THEME (image);
  BobguiCssImageIconTheme *copy;
  BobguiSettings *settings;
  GdkDisplay *display;
  const char *names[4] = { NULL, "success", "warning", "error" };

  copy = g_object_new (BOBGUI_TYPE_CSS_IMAGE_ICON_THEME, NULL);
  copy->name = g_strdup (icon_theme->name);
  settings = bobgui_style_provider_get_settings (context->provider);
  display = _bobgui_settings_get_display (settings);
  copy->icon_theme = bobgui_icon_theme_get_for_display (display);
  copy->serial = bobgui_icon_theme_get_serial (copy->icon_theme);
  copy->scale = bobgui_style_provider_get_scale (context->provider);

  for (guint i = 0; i < 4; i++)
    {
      BobguiCssValue *color = NULL;

      if (names[i])
        color = bobgui_css_palette_value_get_color (context->style->core->icon_palette, names[i]);
      if (color)
        copy->colors[i] = bobgui_css_value_ref (color);
      else
        copy->colors[i] = bobgui_css_value_ref (context->style->core->color);
    }

  return BOBGUI_CSS_IMAGE (copy);
}

static gboolean
bobgui_css_image_icon_theme_equal (BobguiCssImage *image1,
                                BobguiCssImage *image2)
{
  BobguiCssImageIconTheme *icon_theme1 = (BobguiCssImageIconTheme *) image1;
  BobguiCssImageIconTheme *icon_theme2 = (BobguiCssImageIconTheme *) image2;

  return icon_theme1->serial == icon_theme2->serial &&
         icon_theme1->icon_theme == icon_theme2->icon_theme &&
         g_str_equal (icon_theme1->name, icon_theme2->name);
}

static void
bobgui_css_image_icon_theme_dispose (GObject *object)
{
  BobguiCssImageIconTheme *icon_theme = BOBGUI_CSS_IMAGE_ICON_THEME (object);

  g_free (icon_theme->name);
  icon_theme->name = NULL;

  g_clear_object (&icon_theme->cached_icon);
  g_clear_pointer (&icon_theme->colors[0], bobgui_css_value_unref);
  g_clear_pointer (&icon_theme->colors[1], bobgui_css_value_unref);
  g_clear_pointer (&icon_theme->colors[2], bobgui_css_value_unref);
  g_clear_pointer (&icon_theme->colors[3], bobgui_css_value_unref);

  G_OBJECT_CLASS (_bobgui_css_image_icon_theme_parent_class)->dispose (object);
}

static gboolean
bobgui_css_image_icon_theme_contains_current_color (BobguiCssImage *image)
{
  BobguiCssImageIconTheme *icon_theme = BOBGUI_CSS_IMAGE_ICON_THEME (image);

  for (guint i = 0; i < 4; i++)
    {
      if (!icon_theme->colors[i] ||
          bobgui_css_value_contains_current_color (icon_theme->colors[i]))
        return TRUE;
    }

  return FALSE;
}

static BobguiCssImage *
bobgui_css_image_icon_theme_resolve (BobguiCssImage          *image,
                                  BobguiCssComputeContext *context,
                                  BobguiCssValue          *current)
{
  BobguiCssImageIconTheme *icon_theme = BOBGUI_CSS_IMAGE_ICON_THEME (image);
  BobguiCssImageIconTheme *copy;

  if (!bobgui_css_image_icon_theme_contains_current_color (image))
    return g_object_ref (image);

  copy = g_object_new (BOBGUI_TYPE_CSS_IMAGE_ICON_THEME, NULL);
  copy->name = g_strdup (icon_theme->name);
  copy->icon_theme = icon_theme->icon_theme;
  copy->serial = icon_theme->serial;
  copy->scale = icon_theme->scale;

  for (guint i = 0; i < 4; i++)
    copy->colors[i] = bobgui_css_value_resolve (icon_theme->colors[i], context, current);

  return BOBGUI_CSS_IMAGE (copy);
}

static void
_bobgui_css_image_icon_theme_class_init (BobguiCssImageIconThemeClass *klass)
{
  BobguiCssImageClass *image_class = BOBGUI_CSS_IMAGE_CLASS (klass);
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  image_class->get_aspect_ratio = bobgui_css_image_icon_theme_get_aspect_ratio;
  image_class->snapshot = bobgui_css_image_icon_theme_snapshot;
  image_class->parse = bobgui_css_image_icon_theme_parse;
  image_class->print = bobgui_css_image_icon_theme_print;
  image_class->compute = bobgui_css_image_icon_theme_compute;
  image_class->equal = bobgui_css_image_icon_theme_equal;
  image_class->contains_current_color = bobgui_css_image_icon_theme_contains_current_color;
  image_class->resolve = bobgui_css_image_icon_theme_resolve;
  object_class->dispose = bobgui_css_image_icon_theme_dispose;
}

static void
_bobgui_css_image_icon_theme_init (BobguiCssImageIconTheme *icon_theme)
{
  icon_theme->icon_theme = bobgui_icon_theme_get_for_display (gdk_display_get_default ());
  icon_theme->scale = 1;
  icon_theme->cached_size = -1;
  icon_theme->cached_icon = NULL;
}

