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

#pragma once

#include "bobgui/bobguicssimageprivate.h"
#include "bobgui/bobguiicontheme.h"

G_BEGIN_DECLS

#define BOBGUI_TYPE_CSS_IMAGE_ICON_THEME           (_bobgui_css_image_icon_theme_get_type ())
#define BOBGUI_CSS_IMAGE_ICON_THEME(obj)           (G_TYPE_CHECK_INSTANCE_CAST (obj, BOBGUI_TYPE_CSS_IMAGE_ICON_THEME, BobguiCssImageIconTheme))
#define BOBGUI_CSS_IMAGE_ICON_THEME_CLASS(cls)     (G_TYPE_CHECK_CLASS_CAST (cls, BOBGUI_TYPE_CSS_IMAGE_ICON_THEME, BobguiCssImageIconThemeClass))
#define BOBGUI_IS_CSS_IMAGE_ICON_THEME(obj)        (G_TYPE_CHECK_INSTANCE_TYPE (obj, BOBGUI_TYPE_CSS_IMAGE_ICON_THEME))
#define BOBGUI_IS_CSS_IMAGE_ICON_THEME_CLASS(obj)  (G_TYPE_CHECK_CLASS_TYPE (obj, BOBGUI_TYPE_CSS_IMAGE_ICON_THEME))
#define BOBGUI_CSS_IMAGE_ICON_THEME_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), BOBGUI_TYPE_CSS_IMAGE_ICON_THEME, BobguiCssImageIconThemeClass))

typedef struct _BobguiCssImageIconTheme           BobguiCssImageIconTheme;
typedef struct _BobguiCssImageIconThemeClass      BobguiCssImageIconThemeClass;

struct _BobguiCssImageIconTheme
{
  BobguiCssImage parent;

  BobguiIconTheme *icon_theme;
  BobguiCssValue *colors[4];
  int serial;
  int scale;
  char *name;

  int cached_size;
  gboolean cached_symbolic;
  BobguiIconPaintable *cached_icon;
};

struct _BobguiCssImageIconThemeClass
{
  BobguiCssImageClass parent_class;
};

GType          _bobgui_css_image_icon_theme_get_type             (void) G_GNUC_CONST;

G_END_DECLS

