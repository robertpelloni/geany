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

#if !defined (__BOBGUI_H_INSIDE__) && !defined (BOBGUI_COMPILATION)
#error "Only <bobgui/bobgui.h> can be included directly."
#endif

#include <gdk/gdk.h>
#include <bobgui/bobguienums.h>
#include <bobgui/bobguitypes.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_STYLE_PROVIDER          (bobgui_style_provider_get_type ())
#define BOBGUI_STYLE_PROVIDER(o)            (G_TYPE_CHECK_INSTANCE_CAST ((o), BOBGUI_TYPE_STYLE_PROVIDER, BobguiStyleProvider))
#define BOBGUI_IS_STYLE_PROVIDER(o)         (G_TYPE_CHECK_INSTANCE_TYPE ((o), BOBGUI_TYPE_STYLE_PROVIDER))

/**
 * BOBGUI_STYLE_PROVIDER_PRIORITY_FALLBACK:
 *
 * The priority used for default style information
 * that is used in the absence of themes.
 *
 * Note that this is not very useful for providing default
 * styling for custom style classes - themes are likely to
 * override styling provided at this priority with
 * catch-all `* {...}` rules.
 */
#define BOBGUI_STYLE_PROVIDER_PRIORITY_FALLBACK      1

/**
 * BOBGUI_STYLE_PROVIDER_PRIORITY_THEME:
 *
 * The priority used for style information provided
 * by themes.
 */
#define BOBGUI_STYLE_PROVIDER_PRIORITY_THEME     200

/**
 * BOBGUI_STYLE_PROVIDER_PRIORITY_SETTINGS:
 *
 * The priority used for style information provided
 * via `BobguiSettings`.
 *
 * This priority is higher than %BOBGUI_STYLE_PROVIDER_PRIORITY_THEME
 * to let settings override themes.
 */
#define BOBGUI_STYLE_PROVIDER_PRIORITY_SETTINGS    400

/**
 * BOBGUI_STYLE_PROVIDER_PRIORITY_APPLICATION:
 *
 * A priority that can be used when adding a `BobguiStyleProvider`
 * for application-specific style information.
 */
#define BOBGUI_STYLE_PROVIDER_PRIORITY_APPLICATION 600

/**
 * BOBGUI_STYLE_PROVIDER_PRIORITY_USER:
 *
 * The priority used for the style information from
 * `$XDG_CONFIG_HOME/bobgui-4.0/bobgui.css`.
 *
 * You should not use priorities higher than this, to
 * give the user the last word.
 */
#define BOBGUI_STYLE_PROVIDER_PRIORITY_USER        800

typedef struct _BobguiStyleProvider BobguiStyleProvider; /* dummy typedef */

GDK_AVAILABLE_IN_ALL
GType bobgui_style_provider_get_type (void) G_GNUC_CONST;

G_DEFINE_AUTOPTR_CLEANUP_FUNC(BobguiStyleProvider, g_object_unref)

GDK_AVAILABLE_IN_ALL
void bobgui_style_context_add_provider_for_display    (GdkDisplay       *display,
                                                    BobguiStyleProvider *provider,
                                                    guint             priority);
GDK_AVAILABLE_IN_ALL
void bobgui_style_context_remove_provider_for_display (GdkDisplay       *display,
                                                    BobguiStyleProvider *provider);


G_END_DECLS

