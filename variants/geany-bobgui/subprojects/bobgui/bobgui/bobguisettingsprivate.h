/* BOBGUI - The Bobgui Framework
 * Copyright (C) 2000 Red Hat, Inc.
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

#include <bobgui/bobguisettings.h>
#include "bobguistylecascadeprivate.h"

G_BEGIN_DECLS

#define DEFAULT_THEME_NAME      "Default"
#define DEFAULT_ICON_THEME      "Adwaita"

const cairo_font_options_t *
                    bobgui_settings_get_font_options            (BobguiSettings            *settings);
GdkDisplay         *_bobgui_settings_get_display                (BobguiSettings            *settings);
BobguiStyleCascade    *_bobgui_settings_get_style_cascade          (BobguiSettings            *settings,
                                                              int                     scale);

typedef enum
{
  BOBGUI_SETTINGS_SOURCE_DEFAULT,
  BOBGUI_SETTINGS_SOURCE_THEME,
  BOBGUI_SETTINGS_SOURCE_XSETTING,
  BOBGUI_SETTINGS_SOURCE_APPLICATION
} BobguiSettingsSource;

BobguiSettingsSource  _bobgui_settings_get_setting_source (BobguiSettings *settings,
                                                     const char *name);

gboolean bobgui_settings_get_enable_animations  (BobguiSettings *settings);
int      bobgui_settings_get_dnd_drag_threshold (BobguiSettings *settings);
const char *bobgui_settings_get_font_family    (BobguiSettings *settings);
int          bobgui_settings_get_font_size      (BobguiSettings *settings);
gboolean     bobgui_settings_get_font_size_is_absolute (BobguiSettings *settings);

G_END_DECLS

