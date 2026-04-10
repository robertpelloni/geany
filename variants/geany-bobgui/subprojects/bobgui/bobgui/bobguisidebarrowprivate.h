/* bobguisidebarrowprivate.h
 *
 * Copyright (C) 2015 Carlos Soriano <csoriano@gnome.org>
 *
 * This file is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of the
 * License, or (at your option) any later version.
 *
 * This file is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#pragma once

#include <glib.h>
#include "bobguilistbox.h"

G_BEGIN_DECLS

#define BOBGUI_TYPE_SIDEBAR_ROW             (bobgui_sidebar_row_get_type())
#define BOBGUI_SIDEBAR_ROW(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_SIDEBAR_ROW, BobguiSidebarRow))
#define BOBGUI_SIDEBAR_ROW_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), BOBGUI_TYPE_SIDEBAR_ROW, BobguiSidebarRowClass))
#define BOBGUI_IS_SIDEBAR_ROW(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOBGUI_TYPE_SIDEBAR_ROW))
#define BOBGUI_IS_SIDEBAR_ROW_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), BOBGUI_TYPE_SIDEBAR_ROW))
#define BOBGUI_SIDEBAR_ROW_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), BOBGUI_TYPE_SIDEBAR_ROW, BobguiSidebarRowClass))

typedef struct _BobguiSidebarRow BobguiSidebarRow;
typedef struct _BobguiSidebarRowClass BobguiSidebarRowClass;

struct _BobguiSidebarRowClass
{
  BobguiListBoxRowClass parent;
};

GType      bobgui_sidebar_row_get_type   (void) G_GNUC_CONST;

BobguiSidebarRow *bobgui_sidebar_row_new    (void);
BobguiSidebarRow *bobgui_sidebar_row_clone  (BobguiSidebarRow *self);

/* Use these methods instead of bobgui_widget_hide/show to use an animation */
void           bobgui_sidebar_row_hide   (BobguiSidebarRow *self,
                                       gboolean       immediate);
void           bobgui_sidebar_row_reveal (BobguiSidebarRow *self);

BobguiWidget     *bobgui_sidebar_row_get_eject_button (BobguiSidebarRow *self);
void           bobgui_sidebar_row_set_start_icon   (BobguiSidebarRow *self,
                                                 GIcon         *icon);
void           bobgui_sidebar_row_set_end_icon     (BobguiSidebarRow *self,
                                                 GIcon         *icon);
void           bobgui_sidebar_row_set_busy         (BobguiSidebarRow *row,
                                                 gboolean       is_busy);

G_END_DECLS

