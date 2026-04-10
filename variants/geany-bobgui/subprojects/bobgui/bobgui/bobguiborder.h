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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
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

#if !defined (__BOBGUI_H_INSIDE__) && !defined (BOBGUI_COMPILATION)
#error "Only <bobgui/bobgui.h> can be included directly."
#endif

#include <glib-object.h>
#include <gdk/gdk.h>

G_BEGIN_DECLS

typedef struct _BobguiBorder BobguiBorder;

#define BOBGUI_TYPE_BORDER (bobgui_border_get_type ())

/**
 * BobguiBorder:
 * @left: The width of the left border
 * @right: The width of the right border
 * @top: The width of the top border
 * @bottom: The width of the bottom border
 *
 * Specifies a border around a rectangular area.
 *
 * Each side can have a different width.
 */
struct _BobguiBorder
{
  gint16 left;
  gint16 right;
  gint16 top;
  gint16 bottom;
};

GDK_AVAILABLE_IN_ALL
GType      bobgui_border_get_type (void) G_GNUC_CONST;
GDK_AVAILABLE_IN_ALL
BobguiBorder *bobgui_border_new      (void) G_GNUC_MALLOC;
GDK_AVAILABLE_IN_ALL
BobguiBorder *bobgui_border_copy     (const BobguiBorder *border_);
GDK_AVAILABLE_IN_ALL
void       bobgui_border_free     (BobguiBorder       *border_);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(BobguiBorder, bobgui_border_free)

G_END_DECLS

