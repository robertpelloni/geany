/*
 * Copyright © 2018 Benjamin Otte
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors: Benjamin Otte <otte@gnome.org>
 */

#pragma once

#if !defined (__BOBGUI_H_INSIDE__) && !defined (BOBGUI_COMPILATION)
#error "Only <bobgui/bobgui.h> can be included directly."
#endif

#include <bobgui/bobguimediastream.h>
#include <bobgui/bobguiwidget.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_MEDIA_CONTROLS         (bobgui_media_controls_get_type ())

GDK_AVAILABLE_IN_ALL
G_DECLARE_FINAL_TYPE (BobguiMediaControls, bobgui_media_controls, BOBGUI, MEDIA_CONTROLS, BobguiWidget)

GDK_AVAILABLE_IN_ALL
BobguiWidget      *bobgui_media_controls_new                  (BobguiMediaStream         *stream);

GDK_AVAILABLE_IN_ALL
BobguiMediaStream *bobgui_media_controls_get_media_stream     (BobguiMediaControls       *controls);
GDK_AVAILABLE_IN_ALL
void            bobgui_media_controls_set_media_stream     (BobguiMediaControls       *controls,
                                                         BobguiMediaStream         *stream);


G_END_DECLS

