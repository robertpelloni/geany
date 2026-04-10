/* BobguiIconTheme - a loader for icon themes
 * bobgui-icon-loader.h Copyright (C) 2002, 2003 Red Hat, Inc.
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

#pragma once

#if !defined (__BOBGUI_H_INSIDE__) && !defined (BOBGUI_COMPILATION)
#error "Only <bobgui/bobgui.h> can be included directly."
#endif

#include <gdk/gdk.h>
#include <bobgui/bobguienums.h>
#include <bobgui/bobguiiconpaintable.h>

G_BEGIN_DECLS

typedef struct _BobguiIconTheme      BobguiIconTheme;

#define BOBGUI_TYPE_ICON_THEME        (bobgui_icon_theme_get_type ())
#define BOBGUI_ICON_THEME(obj)        (G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_ICON_THEME, BobguiIconTheme))
#define BOBGUI_IS_ICON_THEME(obj)     (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOBGUI_TYPE_ICON_THEME))

/**
 * BobguiIconLookupFlags:
 * @BOBGUI_ICON_LOOKUP_FORCE_REGULAR: Try to always load regular icons, even
 *   when symbolic icon names are given
 * @BOBGUI_ICON_LOOKUP_FORCE_SYMBOLIC: Try to always load symbolic icons, even
 *   when regular icon names are given
 * @BOBGUI_ICON_LOOKUP_PRELOAD: Starts loading the texture in the background
 *   so it is ready when later needed.
 *
 * Used to specify options for bobgui_icon_theme_lookup_icon().
 */
/**
 * BOBGUI_ICON_LOOKUP_NONE:
 *
 * Perform a regular lookup.
 *
 * Since: 4.18
 */
typedef enum
{
  BOBGUI_ICON_LOOKUP_NONE GDK_AVAILABLE_ENUMERATOR_IN_4_18 = 0,
  BOBGUI_ICON_LOOKUP_FORCE_REGULAR  = 1 << 0,
  BOBGUI_ICON_LOOKUP_FORCE_SYMBOLIC = 1 << 1,
  BOBGUI_ICON_LOOKUP_PRELOAD        = 1 << 2,
} BobguiIconLookupFlags;

/**
 * BOBGUI_ICON_THEME_ERROR:
 *
 * The `GQuark` used for `BobguiIconThemeError` errors.
 */
#define BOBGUI_ICON_THEME_ERROR bobgui_icon_theme_error_quark ()

/**
 * BobguiIconThemeError:
 * @BOBGUI_ICON_THEME_NOT_FOUND: The icon specified does not exist in the theme
 * @BOBGUI_ICON_THEME_FAILED: An unspecified error occurred.
 *
 * Error codes for `BobguiIconTheme` operations.
 **/
typedef enum {
  BOBGUI_ICON_THEME_NOT_FOUND,
  BOBGUI_ICON_THEME_FAILED
} BobguiIconThemeError;

GDK_AVAILABLE_IN_ALL
GQuark bobgui_icon_theme_error_quark (void);

GDK_AVAILABLE_IN_ALL
GType            bobgui_icon_theme_get_type             (void) G_GNUC_CONST;

GDK_AVAILABLE_IN_ALL
BobguiIconTheme    *bobgui_icon_theme_new                  (void);
GDK_AVAILABLE_IN_ALL
BobguiIconTheme    *bobgui_icon_theme_get_for_display      (GdkDisplay                  *display);

GDK_AVAILABLE_IN_ALL
GdkDisplay *     bobgui_icon_theme_get_display          (BobguiIconTheme                *self);

GDK_AVAILABLE_IN_ALL
void             bobgui_icon_theme_set_search_path      (BobguiIconTheme                *self,
                                                      const char * const          *path);
GDK_AVAILABLE_IN_ALL
char **          bobgui_icon_theme_get_search_path      (BobguiIconTheme                *self);
GDK_AVAILABLE_IN_ALL
void             bobgui_icon_theme_add_search_path      (BobguiIconTheme                *self,
                                                      const char                  *path);

GDK_AVAILABLE_IN_ALL
void             bobgui_icon_theme_set_resource_path    (BobguiIconTheme                *self,
                                                      const char * const          *path);
GDK_AVAILABLE_IN_ALL
char **          bobgui_icon_theme_get_resource_path    (BobguiIconTheme                *self);
GDK_AVAILABLE_IN_ALL
void             bobgui_icon_theme_add_resource_path    (BobguiIconTheme                *self,
                                                      const char                  *path);

GDK_AVAILABLE_IN_ALL
void             bobgui_icon_theme_set_theme_name       (BobguiIconTheme                *self,
                                                      const char                  *theme_name);
GDK_AVAILABLE_IN_ALL
char *           bobgui_icon_theme_get_theme_name       (BobguiIconTheme                *self);

GDK_AVAILABLE_IN_ALL
gboolean         bobgui_icon_theme_has_icon             (BobguiIconTheme                *self,
                                                      const char                  *icon_name);
GDK_AVAILABLE_IN_4_2
gboolean         bobgui_icon_theme_has_gicon            (BobguiIconTheme                *self,
                                                      GIcon                       *gicon);
GDK_AVAILABLE_IN_ALL
int              *bobgui_icon_theme_get_icon_sizes      (BobguiIconTheme                *self,
                                                      const char                  *icon_name);
GDK_AVAILABLE_IN_ALL
BobguiIconPaintable *bobgui_icon_theme_lookup_icon         (BobguiIconTheme                *self,
                                                      const char                  *icon_name,
                                                      const char                  *fallbacks[],
                                                      int                          size,
                                                      int                          scale,
                                                      BobguiTextDirection             direction,
                                                      BobguiIconLookupFlags           flags);
GDK_AVAILABLE_IN_ALL
BobguiIconPaintable *bobgui_icon_theme_lookup_by_gicon     (BobguiIconTheme                *self,
                                                      GIcon                       *icon,
                                                      int                          size,
                                                      int                          scale,
                                                      BobguiTextDirection             direction,
                                                      BobguiIconLookupFlags           flags);
GDK_AVAILABLE_IN_ALL
char **               bobgui_icon_theme_get_icon_names  (BobguiIconTheme                *self);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(BobguiIconTheme, g_object_unref)

G_END_DECLS

