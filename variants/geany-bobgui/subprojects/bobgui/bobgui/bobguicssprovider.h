/* BOBGUI - The Bobgui Framework
 * Copyright (C) 2010 Carlos Garnacho <carlosg@gnome.org>
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

#include <gio/gio.h>
#include <bobgui/css/bobguicss.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_CSS_PROVIDER         (bobgui_css_provider_get_type ())
#define BOBGUI_CSS_PROVIDER(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), BOBGUI_TYPE_CSS_PROVIDER, BobguiCssProvider))
#define BOBGUI_IS_CSS_PROVIDER(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), BOBGUI_TYPE_CSS_PROVIDER))

typedef struct _BobguiCssProvider BobguiCssProvider;
typedef struct _BobguiCssProviderClass BobguiCssProviderClass;
typedef struct _BobguiCssProviderPrivate BobguiCssProviderPrivate;

struct _BobguiCssProvider
{
  GObject parent_instance;
};


GDK_AVAILABLE_IN_ALL
GType bobgui_css_provider_get_type (void) G_GNUC_CONST;

GDK_AVAILABLE_IN_ALL
BobguiCssProvider * bobgui_css_provider_new (void);

GDK_AVAILABLE_IN_ALL
char *           bobgui_css_provider_to_string      (BobguiCssProvider  *provider);

GDK_DEPRECATED_IN_4_12_FOR(bobgui_css_provider_load_from_string)
void             bobgui_css_provider_load_from_data (BobguiCssProvider  *css_provider,
                                                  const char      *data,
                                                  gssize           length);
GDK_AVAILABLE_IN_4_12
void             bobgui_css_provider_load_from_string (BobguiCssProvider *css_provider,
                                                    const char     *string);

GDK_AVAILABLE_IN_4_12
void             bobgui_css_provider_load_from_bytes  (BobguiCssProvider *css_provider,
                                                    GBytes         *data);

GDK_AVAILABLE_IN_ALL
void             bobgui_css_provider_load_from_file (BobguiCssProvider  *css_provider,
                                                  GFile           *file);
GDK_AVAILABLE_IN_ALL
void             bobgui_css_provider_load_from_path (BobguiCssProvider  *css_provider,
                                                  const char      *path);

GDK_AVAILABLE_IN_ALL
void             bobgui_css_provider_load_from_resource (BobguiCssProvider *css_provider,
                                                      const char     *resource_path);

GDK_AVAILABLE_IN_ALL
void             bobgui_css_provider_load_named     (BobguiCssProvider  *provider,
                                                  const char      *name,
                                                  const char      *variant);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(BobguiCssProvider, g_object_unref)

G_END_DECLS

