/*
 * bobguiappchooser.h: app-chooser interface
 *
 * Copyright (C) 2010 Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors: Cosimo Cecchi <ccecchi@redhat.com>
 */

#pragma once

#if !defined (__BOBGUI_H_INSIDE__) && !defined (BOBGUI_COMPILATION)
#error "Only <bobgui/bobgui.h> can be included directly."
#endif

#include <glib.h>
#include <gio/gio.h>
#include <gdk/gdk.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_APP_CHOOSER    (bobgui_app_chooser_get_type ())
#define BOBGUI_APP_CHOOSER(obj)    (G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_APP_CHOOSER, BobguiAppChooser))
#define BOBGUI_IS_APP_CHOOSER(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOBGUI_TYPE_APP_CHOOSER))

typedef struct _BobguiAppChooser BobguiAppChooser;

GDK_AVAILABLE_IN_ALL
GType      bobgui_app_chooser_get_type         (void) G_GNUC_CONST;

GDK_DEPRECATED_IN_4_10
GAppInfo * bobgui_app_chooser_get_app_info     (BobguiAppChooser *self);
GDK_DEPRECATED_IN_4_10
char *    bobgui_app_chooser_get_content_type (BobguiAppChooser *self);
GDK_DEPRECATED_IN_4_10
void       bobgui_app_chooser_refresh          (BobguiAppChooser *self);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(BobguiAppChooser, g_object_unref)

G_END_DECLS


