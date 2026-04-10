/* BOBGUI - The Bobgui Framework
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
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
 * Modified by the BOBGUI Team and others 1997-2001.  See the AUTHORS
 * file for a list of people on the BOBGUI Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * BOBGUI at ftp://ftp.bobgui.org/pub/bobgui/.
 */

#pragma once

#include "bobguitextbuffer.h"

G_BEGIN_DECLS

/* This is a private uninstalled header shared between
 * BobguiTextView and BobguiEntry
 */

GdkPaintable *    bobgui_text_util_create_drag_icon  (BobguiWidget     *widget,
                                                   char          *text,
                                                   gssize         len);
GdkPaintable *    bobgui_text_util_create_rich_drag_icon (BobguiWidget     *widget,
                                                   BobguiTextBuffer *buffer,
                                                   BobguiTextIter   *start,
                                                   BobguiTextIter   *end);

gboolean _bobgui_text_util_get_block_cursor_location (PangoLayout    *layout,
						   int             index_,
						   PangoRectangle *rectangle,
						   gboolean       *at_line_end);

G_END_DECLS

