/* BOBGUI - The Bobgui Framework
 * Copyright © 2012 Carlos Garnacho <carlosg@gnome.org>
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

#include <bobgui/bobguiwidget.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_TEXT_HANDLE (bobgui_text_handle_get_type ())
G_DECLARE_FINAL_TYPE (BobguiTextHandle, bobgui_text_handle,
		      BOBGUI, TEXT_HANDLE, BobguiWidget)

typedef enum
{
  BOBGUI_TEXT_HANDLE_ROLE_CURSOR,
  BOBGUI_TEXT_HANDLE_ROLE_SELECTION_START,
  BOBGUI_TEXT_HANDLE_ROLE_SELECTION_END,
} BobguiTextHandleRole;

BobguiTextHandle *    bobgui_text_handle_new          (BobguiWidget             *parent);

void               bobgui_text_handle_present      (BobguiTextHandle         *handle);

void               bobgui_text_handle_set_role (BobguiTextHandle     *handle,
					     BobguiTextHandleRole  role);
BobguiTextHandleRole  bobgui_text_handle_get_role (BobguiTextHandle     *handle);

void               bobgui_text_handle_set_position (BobguiTextHandle      *handle,
						 const GdkRectangle *rect);

gboolean           bobgui_text_handle_get_is_dragged (BobguiTextHandle *handle);

G_END_DECLS

