/*
 * Copyright © 2023 Benjamin Otte
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

#include "bobguiwidget.h"

G_BEGIN_DECLS

#define BOBGUI_TYPE_LIST_HEADER_BASE         (bobgui_list_header_base_get_type ())
#define BOBGUI_LIST_HEADER_BASE(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), BOBGUI_TYPE_LIST_HEADER_BASE, BobguiListHeaderBase))
#define BOBGUI_LIST_HEADER_BASE_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), BOBGUI_TYPE_LIST_HEADER_BASE, BobguiListHeaderBaseClass))
#define BOBGUI_IS_LIST_HEADER_BASE(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), BOBGUI_TYPE_LIST_HEADER_BASE))
#define BOBGUI_IS_LIST_HEADER_BASE_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), BOBGUI_TYPE_LIST_HEADER_BASE))
#define BOBGUI_LIST_HEADER_BASE_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), BOBGUI_TYPE_LIST_HEADER_BASE, BobguiListHeaderBaseClass))

typedef struct _BobguiListHeaderBase BobguiListHeaderBase;
typedef struct _BobguiListHeaderBaseClass BobguiListHeaderBaseClass;

struct _BobguiListHeaderBase
{
  BobguiWidget parent_instance;
};

struct _BobguiListHeaderBaseClass
{
  BobguiWidgetClass parent_class;

  void                  (* update)                              (BobguiListHeaderBase      *self,
                                                                 gpointer                item,
                                                                 guint                   start,
                                                                 guint                   end);
};

GType                   bobgui_list_header_base_get_type           (void) G_GNUC_CONST;

void                    bobgui_list_header_base_update             (BobguiListHeaderBase      *self,
                                                                 gpointer                item,
                                                                 guint                   start,
                                                                 guint                   end);

guint                   bobgui_list_header_base_get_start          (BobguiListHeaderBase      *self);
guint                   bobgui_list_header_base_get_end            (BobguiListHeaderBase      *self);
gpointer                bobgui_list_header_base_get_item           (BobguiListHeaderBase      *self);

G_END_DECLS

