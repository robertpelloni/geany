/*
 * bobguiinfobar.h
 * This file is part of BOBGUI
 *
 * Copyright (C) 2005 - Paolo Maggi
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

/*
 * Modified by the gedit Team, 2005. See the gedit AUTHORS file for a
 * list of people on the gedit Team.
 * See the gedit ChangeLog files for a list of changes.
 *
 * Modified by the BOBGUI Team, 2008-2009.
 */

#pragma once

#if !defined (__BOBGUI_H_INSIDE__) && !defined (BOBGUI_COMPILATION)
#error "Only <bobgui/bobgui.h> can be included directly."
#endif

#include <bobgui/bobguiwidget.h>
#include <bobgui/bobguienums.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_INFO_BAR              (bobgui_info_bar_get_type())
#define BOBGUI_INFO_BAR(obj)              (G_TYPE_CHECK_INSTANCE_CAST((obj), BOBGUI_TYPE_INFO_BAR, BobguiInfoBar))
#define BOBGUI_IS_INFO_BAR(obj)           (G_TYPE_CHECK_INSTANCE_TYPE((obj), BOBGUI_TYPE_INFO_BAR))

typedef struct _BobguiInfoBar BobguiInfoBar;

GDK_AVAILABLE_IN_ALL
GType          bobgui_info_bar_get_type               (void) G_GNUC_CONST;
GDK_DEPRECATED_IN_4_10
BobguiWidget     *bobgui_info_bar_new                    (void);

GDK_DEPRECATED_IN_4_10
BobguiWidget     *bobgui_info_bar_new_with_buttons       (const char     *first_button_text,
                                                    ...);

GDK_DEPRECATED_IN_4_10
void           bobgui_info_bar_add_action_widget      (BobguiInfoBar     *info_bar,
                                                    BobguiWidget      *child,
                                                    int             response_id);
GDK_DEPRECATED_IN_4_10
void           bobgui_info_bar_remove_action_widget   (BobguiInfoBar     *info_bar,
                                                    BobguiWidget      *widget);
GDK_DEPRECATED_IN_4_10
BobguiWidget     *bobgui_info_bar_add_button             (BobguiInfoBar     *info_bar,
                                                    const char     *button_text,
                                                    int             response_id);
GDK_DEPRECATED_IN_4_10
void           bobgui_info_bar_add_buttons            (BobguiInfoBar     *info_bar,
                                                    const char     *first_button_text,
                                                    ...);
GDK_DEPRECATED_IN_4_10
void           bobgui_info_bar_add_child              (BobguiInfoBar     *info_bar,
                                                    BobguiWidget      *widget);
GDK_DEPRECATED_IN_4_10
void           bobgui_info_bar_remove_child           (BobguiInfoBar     *info_bar,
                                                    BobguiWidget      *widget);

GDK_DEPRECATED_IN_4_10
void           bobgui_info_bar_set_response_sensitive (BobguiInfoBar     *info_bar,
                                                    int             response_id,
                                                    gboolean        setting);
GDK_DEPRECATED_IN_4_10
void           bobgui_info_bar_set_default_response   (BobguiInfoBar     *info_bar,
                                                    int             response_id);

GDK_DEPRECATED_IN_4_10
void           bobgui_info_bar_response               (BobguiInfoBar     *info_bar,
                                                    int             response_id);

GDK_DEPRECATED_IN_4_10
void           bobgui_info_bar_set_message_type       (BobguiInfoBar     *info_bar,
                                                    BobguiMessageType  message_type);
GDK_DEPRECATED_IN_4_10
BobguiMessageType bobgui_info_bar_get_message_type       (BobguiInfoBar     *info_bar);

GDK_DEPRECATED_IN_4_10
void           bobgui_info_bar_set_show_close_button  (BobguiInfoBar     *info_bar,
                                                    gboolean        setting);
GDK_DEPRECATED_IN_4_10
gboolean       bobgui_info_bar_get_show_close_button  (BobguiInfoBar     *info_bar);

GDK_DEPRECATED_IN_4_10
void           bobgui_info_bar_set_revealed           (BobguiInfoBar     *info_bar,
                                                    gboolean        revealed);
GDK_DEPRECATED_IN_4_10
gboolean       bobgui_info_bar_get_revealed           (BobguiInfoBar     *info_bar);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(BobguiInfoBar, g_object_unref)

G_END_DECLS

