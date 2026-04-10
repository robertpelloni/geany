/*
 * BOBGUI - The Bobgui Framework
 * Copyright (C) 1998, 1999 Red Hat, Inc.
 * All rights reserved.
 *
 * This Library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

/* Color picker button for GNOME
 *
 * Author: Federico Mena <federico@nuclecu.unam.mx>
 *
 * Modified by the BOBGUI Team and others 2003.  See the AUTHORS
 * file for a list of people on the BOBGUI Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * BOBGUI at ftp://ftp.bobgui.org/pub/bobgui/.
 */

#pragma once


#if !defined (__BOBGUI_H_INSIDE__) && !defined (BOBGUI_COMPILATION)
#error "Only <bobgui/bobgui.h> can be included directly."
#endif

#include <bobgui/bobguibutton.h>

G_BEGIN_DECLS


#define BOBGUI_TYPE_COLOR_BUTTON             (bobgui_color_button_get_type ())
#define BOBGUI_COLOR_BUTTON(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_COLOR_BUTTON, BobguiColorButton))
#define BOBGUI_IS_COLOR_BUTTON(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOBGUI_TYPE_COLOR_BUTTON))

typedef struct _BobguiColorButton BobguiColorButton;

GDK_AVAILABLE_IN_ALL
GType        bobgui_color_button_get_type      (void) G_GNUC_CONST;
GDK_DEPRECATED_IN_4_10
BobguiWidget *  bobgui_color_button_new           (void);
GDK_DEPRECATED_IN_4_10
BobguiWidget *  bobgui_color_button_new_with_rgba (const GdkRGBA  *rgba);
GDK_DEPRECATED_IN_4_10
void         bobgui_color_button_set_title     (BobguiColorButton *button,
                                             const char     *title);
GDK_DEPRECATED_IN_4_10
const char *bobgui_color_button_get_title     (BobguiColorButton *button);

GDK_DEPRECATED_IN_4_10
gboolean     bobgui_color_button_get_modal        (BobguiColorButton *button);
GDK_DEPRECATED_IN_4_10
void         bobgui_color_button_set_modal        (BobguiColorButton *button,
                                                gboolean        modal);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(BobguiColorButton, g_object_unref)

G_END_DECLS

