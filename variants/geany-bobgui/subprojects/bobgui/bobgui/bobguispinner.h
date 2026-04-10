/* BOBGUI - The Bobgui Framework
 *
 * Copyright (C) 2007 John Stowers, Neil Jagdish Patel.
 * Copyright (C) 2009 Bastien Nocera, David Zeuthen
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 * Code adapted from egg-spinner
 * by Christian Hergert <christian.hergert@gmail.com>
 */

#pragma once

#if !defined (__BOBGUI_H_INSIDE__) && !defined (BOBGUI_COMPILATION)
#error "Only <bobgui/bobgui.h> can be included directly."
#endif

#include <bobgui/bobguiwidget.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_SPINNER           (bobgui_spinner_get_type ())
#define BOBGUI_SPINNER(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_SPINNER, BobguiSpinner))
#define BOBGUI_IS_SPINNER(obj)        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOBGUI_TYPE_SPINNER))

typedef struct _BobguiSpinner      BobguiSpinner;


GDK_AVAILABLE_IN_ALL
GType      bobgui_spinner_get_type     (void) G_GNUC_CONST;
GDK_AVAILABLE_IN_ALL
BobguiWidget *bobgui_spinner_new          (void);
GDK_AVAILABLE_IN_ALL
void       bobgui_spinner_start        (BobguiSpinner *spinner);
GDK_AVAILABLE_IN_ALL
void       bobgui_spinner_stop         (BobguiSpinner *spinner);
GDK_AVAILABLE_IN_ALL
void       bobgui_spinner_set_spinning (BobguiSpinner *spinner,
                                     gboolean    spinning);
GDK_AVAILABLE_IN_ALL
gboolean   bobgui_spinner_get_spinning (BobguiSpinner *spinner);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(BobguiSpinner, g_object_unref)

G_END_DECLS

