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

#include "bobguilistheader.h"

#include "bobguilistheaderwidgetprivate.h"

G_BEGIN_DECLS

struct _BobguiListHeader
{
  GObject parent_instance;

  BobguiListHeaderWidget *owner; /* has a reference */

  BobguiWidget *child;
};

struct _BobguiListHeaderClass
{
  GObjectClass parent_class;
};

BobguiListHeader * bobgui_list_header_new                             (void);

void            bobgui_list_header_do_notify                       (BobguiListHeader          *list_header,
                                                                 gboolean                notify_item,
                                                                 gboolean                notify_start,
                                                                 gboolean                notify_end,
                                                                 gboolean                notify_n_items);


G_END_DECLS

