/*
 * bobguiappchooserbutton.h: an app-chooser button
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

#include <bobgui/bobguiwidget.h>
#include <gio/gio.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_APP_CHOOSER_BUTTON            (bobgui_app_chooser_button_get_type ())
#define BOBGUI_APP_CHOOSER_BUTTON(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_APP_CHOOSER_BUTTON, BobguiAppChooserButton))
#define BOBGUI_IS_APP_CHOOSER_BUTTON(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOBGUI_TYPE_APP_CHOOSER_BUTTON))

typedef struct _BobguiAppChooserButton        BobguiAppChooserButton;

GDK_AVAILABLE_IN_ALL
GType       bobgui_app_chooser_button_get_type           (void) G_GNUC_CONST;

GDK_DEPRECATED_IN_4_10
BobguiWidget * bobgui_app_chooser_button_new                (const char          *content_type);

GDK_DEPRECATED_IN_4_10
void        bobgui_app_chooser_button_append_separator   (BobguiAppChooserButton *self);
GDK_DEPRECATED_IN_4_10
void        bobgui_app_chooser_button_append_custom_item (BobguiAppChooserButton *self,
                                                       const char          *name,
                                                       const char          *label,
                                                       GIcon               *icon);
GDK_DEPRECATED_IN_4_10
void     bobgui_app_chooser_button_set_active_custom_item (BobguiAppChooserButton *self,
                                                        const char          *name);

GDK_DEPRECATED_IN_4_10
void     bobgui_app_chooser_button_set_show_dialog_item  (BobguiAppChooserButton *self,
                                                       gboolean             setting);
GDK_DEPRECATED_IN_4_10
gboolean bobgui_app_chooser_button_get_show_dialog_item  (BobguiAppChooserButton *self);
GDK_DEPRECATED_IN_4_10
void     bobgui_app_chooser_button_set_heading           (BobguiAppChooserButton *self,
                                                       const char          *heading);
GDK_DEPRECATED_IN_4_10
const char *
         bobgui_app_chooser_button_get_heading           (BobguiAppChooserButton *self);
GDK_DEPRECATED_IN_4_10
void     bobgui_app_chooser_button_set_show_default_item (BobguiAppChooserButton *self,
                                                       gboolean             setting);
GDK_DEPRECATED_IN_4_10
gboolean bobgui_app_chooser_button_get_show_default_item (BobguiAppChooserButton *self);

GDK_DEPRECATED_IN_4_10
gboolean bobgui_app_chooser_button_get_modal             (BobguiAppChooserButton *self);
GDK_DEPRECATED_IN_4_10
void     bobgui_app_chooser_button_set_modal             (BobguiAppChooserButton *self,
                                                       gboolean             modal);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(BobguiAppChooserButton, g_object_unref)

G_END_DECLS

