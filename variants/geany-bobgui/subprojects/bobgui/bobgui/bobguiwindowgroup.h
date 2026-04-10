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
 * Modified by the BOBGUI Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the BOBGUI Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * BOBGUI at ftp://ftp.bobgui.org/pub/bobgui/.
 */

#pragma once


#if !defined (__BOBGUI_H_INSIDE__) && !defined (BOBGUI_COMPILATION)
#error "Only <bobgui/bobgui.h> can be included directly."
#endif

#include "bobguiwindow.h"

G_BEGIN_DECLS

#define BOBGUI_TYPE_WINDOW_GROUP             (bobgui_window_group_get_type ())
#define BOBGUI_WINDOW_GROUP(object)          (G_TYPE_CHECK_INSTANCE_CAST ((object), BOBGUI_TYPE_WINDOW_GROUP, BobguiWindowGroup))
#define BOBGUI_WINDOW_GROUP_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), BOBGUI_TYPE_WINDOW_GROUP, BobguiWindowGroupClass))
#define BOBGUI_IS_WINDOW_GROUP(object)       (G_TYPE_CHECK_INSTANCE_TYPE ((object), BOBGUI_TYPE_WINDOW_GROUP))
#define BOBGUI_IS_WINDOW_GROUP_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), BOBGUI_TYPE_WINDOW_GROUP))
#define BOBGUI_WINDOW_GROUP_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), BOBGUI_TYPE_WINDOW_GROUP, BobguiWindowGroupClass))

struct _BobguiWindowGroup
{
  GObject parent_instance;

  BobguiWindowGroupPrivate *priv;
};

struct _BobguiWindowGroupClass
{
  GObjectClass parent_class;

  /* Padding for future expansion */
  void (*_bobgui_reserved1) (void);
  void (*_bobgui_reserved2) (void);
  void (*_bobgui_reserved3) (void);
  void (*_bobgui_reserved4) (void);
};


/* Window groups
 */
GDK_AVAILABLE_IN_ALL
GType            bobgui_window_group_get_type      (void) G_GNUC_CONST;

GDK_AVAILABLE_IN_ALL
BobguiWindowGroup * bobgui_window_group_new           (void);
GDK_AVAILABLE_IN_ALL
void             bobgui_window_group_add_window    (BobguiWindowGroup     *window_group,
                                                 BobguiWindow          *window);
GDK_AVAILABLE_IN_ALL
void             bobgui_window_group_remove_window (BobguiWindowGroup     *window_group,
                                                 BobguiWindow          *window);
GDK_AVAILABLE_IN_ALL
GList *          bobgui_window_group_list_windows  (BobguiWindowGroup     *window_group);


G_END_DECLS


