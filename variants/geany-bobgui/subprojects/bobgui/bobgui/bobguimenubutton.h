/* BOBGUI - The Bobgui Framework
 *
 * Copyright (C) 2003 Ricardo Fernandez Pascual
 * Copyright (C) 2004 Paolo Borelli
 * Copyright (C) 2012 Bastien Nocera
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

#include <bobgui/bobguitogglebutton.h>
#include <bobgui/bobguipopover.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_MENU_BUTTON            (bobgui_menu_button_get_type ())
#define BOBGUI_MENU_BUTTON(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_MENU_BUTTON, BobguiMenuButton))
#define BOBGUI_IS_MENU_BUTTON(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOBGUI_TYPE_MENU_BUTTON))

typedef struct _BobguiMenuButton        BobguiMenuButton;

/**
 * BobguiMenuButtonCreatePopupFunc:
 * @menu_button: the `BobguiMenuButton`
 * @user_data: User data passed to bobgui_menu_button_set_create_popup_func()
 *
 * User-provided callback function to create a popup for a
 * `BobguiMenuButton` on demand.
 *
 * This function is called when the popup of @menu_button is shown,
 * but none has been provided via [method@Bobgui.MenuButton.set_popover]
 * or [method@Bobgui.MenuButton.set_menu_model].
 */
typedef void  (*BobguiMenuButtonCreatePopupFunc) (BobguiMenuButton *menu_button,
                                               gpointer       user_data);

GDK_AVAILABLE_IN_ALL
GType        bobgui_menu_button_get_type       (void) G_GNUC_CONST;
GDK_AVAILABLE_IN_ALL
BobguiWidget   *bobgui_menu_button_new            (void);

GDK_AVAILABLE_IN_ALL
void         bobgui_menu_button_set_popover    (BobguiMenuButton *menu_button,
                                             BobguiWidget     *popover);
GDK_AVAILABLE_IN_ALL
BobguiPopover  *bobgui_menu_button_get_popover    (BobguiMenuButton *menu_button);

GDK_AVAILABLE_IN_ALL
void         bobgui_menu_button_set_direction  (BobguiMenuButton *menu_button,
                                             BobguiArrowType   direction);
GDK_AVAILABLE_IN_ALL
BobguiArrowType bobgui_menu_button_get_direction  (BobguiMenuButton *menu_button);

GDK_AVAILABLE_IN_ALL
void         bobgui_menu_button_set_menu_model (BobguiMenuButton *menu_button,
                                             GMenuModel    *menu_model);
GDK_AVAILABLE_IN_ALL
GMenuModel  *bobgui_menu_button_get_menu_model (BobguiMenuButton *menu_button);

GDK_AVAILABLE_IN_ALL
void         bobgui_menu_button_set_icon_name (BobguiMenuButton *menu_button,
                                            const char    *icon_name);
GDK_AVAILABLE_IN_ALL
const char * bobgui_menu_button_get_icon_name (BobguiMenuButton *menu_button);

GDK_AVAILABLE_IN_4_4
void         bobgui_menu_button_set_always_show_arrow (BobguiMenuButton *menu_button,
                                                    gboolean       always_show_arrow);
GDK_AVAILABLE_IN_4_4
gboolean     bobgui_menu_button_get_always_show_arrow (BobguiMenuButton *menu_button);

GDK_AVAILABLE_IN_ALL
void         bobgui_menu_button_set_label (BobguiMenuButton *menu_button,
                                        const char    *label);
GDK_AVAILABLE_IN_ALL
const char * bobgui_menu_button_get_label (BobguiMenuButton *menu_button);

GDK_AVAILABLE_IN_ALL
void         bobgui_menu_button_set_use_underline (BobguiMenuButton *menu_button,
                                                gboolean       use_underline);
GDK_AVAILABLE_IN_ALL
gboolean     bobgui_menu_button_get_use_underline (BobguiMenuButton *menu_button);

GDK_AVAILABLE_IN_ALL
void           bobgui_menu_button_set_has_frame (BobguiMenuButton  *menu_button,
                                              gboolean        has_frame);
GDK_AVAILABLE_IN_ALL
gboolean       bobgui_menu_button_get_has_frame (BobguiMenuButton  *menu_button);

GDK_AVAILABLE_IN_ALL
void          bobgui_menu_button_popup (BobguiMenuButton *menu_button);
GDK_AVAILABLE_IN_ALL
void          bobgui_menu_button_popdown (BobguiMenuButton *menu_button);

GDK_AVAILABLE_IN_ALL
void          bobgui_menu_button_set_create_popup_func (BobguiMenuButton                *menu_button,
                                                     BobguiMenuButtonCreatePopupFunc  func,
                                                     gpointer                      user_data,
                                                     GDestroyNotify                destroy_notify);

GDK_AVAILABLE_IN_4_4
void          bobgui_menu_button_set_primary (BobguiMenuButton *menu_button,
                                           gboolean       primary);
GDK_AVAILABLE_IN_4_4
gboolean      bobgui_menu_button_get_primary (BobguiMenuButton *menu_button);

GDK_AVAILABLE_IN_4_6
void          bobgui_menu_button_set_child   (BobguiMenuButton *menu_button,
                                           BobguiWidget     *child);
GDK_AVAILABLE_IN_4_6
BobguiWidget *   bobgui_menu_button_get_child   (BobguiMenuButton *menu_button);

GDK_AVAILABLE_IN_4_10
void          bobgui_menu_button_set_active (BobguiMenuButton *menu_button,
                                       gboolean       active);
GDK_AVAILABLE_IN_4_10
gboolean      bobgui_menu_button_get_active (BobguiMenuButton *menu_button);

GDK_AVAILABLE_IN_4_12
void          bobgui_menu_button_set_can_shrink  (BobguiMenuButton *menu_button,
                                               gboolean       can_shrink);
GDK_AVAILABLE_IN_4_12
gboolean      bobgui_menu_button_get_can_shrink  (BobguiMenuButton *menu_button);


G_DEFINE_AUTOPTR_CLEANUP_FUNC(BobguiMenuButton, g_object_unref)

G_END_DECLS

