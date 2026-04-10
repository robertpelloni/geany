/* BOBGUI - The Bobgui Framework
 * Copyright (C) 2021 Red Hat, Inc.
 *
 * Authors:
 * - Matthias Clasen <mclasen@redhat.com>
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

#include "bobguiwidget.h"
#include "bobguienums.h"

#define BOBGUI_TYPE_POPOVER_CONTENT                 (bobgui_popover_content_get_type ())
#define BOBGUI_POPOVER_CONTENT(obj)                 (G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_POPOVER_CONTENT, BobguiPopoverContent))
#define BOBGUI_POPOVER_CONTENT_CLASS(klass)         (G_TYPE_CHECK_CLASS_CAST ((klass), BOBGUI_TYPE_POPOVER_CONTENT, BobguiPopoverContentClass))
#define BOBGUI_IS_POPOVER_CONTENT(obj)              (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOBGUI_TYPE_POPOVER_CONTENT))
#define BOBGUI_IS_POPOVER_CONTENT_CLASS(klass)      (G_TYPE_CHECK_CLASS_TYPE ((klass), BOBGUI_TYPE_POPOVER_CONTENT))
#define BOBGUI_POPOVER_CONTENT_GET_CLASS(obj)       (G_TYPE_INSTANCE_GET_CLASS ((obj), BOBGUI_TYPE_POPOVER_CONTENT, BobguiPopoverContentClass))

typedef struct _BobguiPopoverContent             BobguiPopoverContent;
typedef struct _BobguiPopoverContentClass        BobguiPopoverContentClass;

struct _BobguiPopoverContent
{
  BobguiWidget parent_instance;
};

struct _BobguiPopoverContentClass
{
  BobguiWidgetClass parent_class;
};

GType      bobgui_popover_content_get_type (void) G_GNUC_CONST;

BobguiWidget *bobgui_popover_content_new (void);

