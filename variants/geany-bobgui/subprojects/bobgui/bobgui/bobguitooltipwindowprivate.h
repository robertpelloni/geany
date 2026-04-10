/* BOBGUI - The Bobgui Framework
 * Copyright 2015  Emmanuele Bassi 
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
 * Modified by the BOBGUI Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the BOBGUI Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * BOBGUI at ftp://ftp.bobgui.org/pub/bobgui/.
 */

#pragma once

#include <gio/gio.h>
#include <bobgui/bobguiwindow.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_TOOLTIP_WINDOW (bobgui_tooltip_window_get_type ())

G_DECLARE_FINAL_TYPE (BobguiTooltipWindow, bobgui_tooltip_window, BOBGUI, TOOLTIP_WINDOW, BobguiWidget)

BobguiWidget *     bobgui_tooltip_window_new                          (void);

void            bobgui_tooltip_window_present                      (BobguiTooltipWindow *window);

void            bobgui_tooltip_window_set_label_markup             (BobguiTooltipWindow *window,
                                                                 const char       *markup);
void            bobgui_tooltip_window_set_label_text               (BobguiTooltipWindow *window,
                                                                 const char       *text);
void            bobgui_tooltip_window_set_image_icon               (BobguiTooltipWindow *window,
                                                                 GdkPaintable     *paintable);
void            bobgui_tooltip_window_set_image_icon_from_name     (BobguiTooltipWindow *window,
                                                                 const char       *icon_name);
void            bobgui_tooltip_window_set_image_icon_from_gicon    (BobguiTooltipWindow *window,
                                                                 GIcon            *gicon);
void            bobgui_tooltip_window_set_custom_widget            (BobguiTooltipWindow *window,
                                                                 BobguiWidget        *custom_widget);
void            bobgui_tooltip_window_set_relative_to              (BobguiTooltipWindow *window,
                                                                 BobguiWidget        *relative_to);
void            bobgui_tooltip_window_position                     (BobguiTooltipWindow *window,
                                                                 GdkRectangle     *rect,
                                                                 GdkGravity        rect_anchor,
                                                                 GdkGravity        surface_anchor,
                                                                 GdkAnchorHints    anchor_hints,
                                                                 int               dx,
                                                                 int               dy);

G_END_DECLS

