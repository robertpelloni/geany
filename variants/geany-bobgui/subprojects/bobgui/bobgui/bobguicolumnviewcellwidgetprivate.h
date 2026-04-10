/*
 * Copyright © 2019 Benjamin Otte
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

#include "bobguicolumnviewcolumn.h"

G_BEGIN_DECLS

#define BOBGUI_TYPE_COLUMN_VIEW_CELL_WIDGET         (bobgui_column_view_cell_widget_get_type ())
#define BOBGUI_COLUMN_VIEW_CELL_WIDGET(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), BOBGUI_TYPE_COLUMN_VIEW_CELL_WIDGET, BobguiColumnViewCellWidget))
#define BOBGUI_COLUMN_VIEW_CELL_WIDGET_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), BOBGUI_TYPE_COLUMN_VIEW_CELL_WIDGET, BobguiColumnViewCellWidgetClass))
#define BOBGUI_IS_COLUMN_VIEW_CELL_WIDGET(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), BOBGUI_TYPE_COLUMN_VIEW_CELL_WIDGET))
#define BOBGUI_IS_COLUMN_VIEW_CELL_WIDGET_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), BOBGUI_TYPE_COLUMN_VIEW_CELL_WIDGET))
#define BOBGUI_COLUMN_VIEW_CELL_WIDGET_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), BOBGUI_TYPE_COLUMN_VIEW_CELL_WIDGET, BobguiColumnViewCellWidgetClass))

typedef struct _BobguiColumnViewCellWidget BobguiColumnViewCellWidget;
typedef struct _BobguiColumnViewCellWidgetClass BobguiColumnViewCellWidgetClass;

GType                           bobgui_column_view_cell_widget_get_type           (void) G_GNUC_CONST;

BobguiWidget *                     bobgui_column_view_cell_widget_new                (BobguiColumnViewColumn             *column,
                                                                                gboolean                         inert);

void                            bobgui_column_view_cell_widget_set_child          (BobguiColumnViewCellWidget         *self,
                                                                                BobguiWidget                       *child);

void                            bobgui_column_view_cell_widget_remove             (BobguiColumnViewCellWidget         *self);

BobguiColumnViewCellWidget *       bobgui_column_view_cell_widget_get_next           (BobguiColumnViewCellWidget         *self);
BobguiColumnViewCellWidget *       bobgui_column_view_cell_widget_get_prev           (BobguiColumnViewCellWidget         *self);
BobguiColumnViewColumn *           bobgui_column_view_cell_widget_get_column         (BobguiColumnViewCellWidget         *self);
void                            bobgui_column_view_cell_widget_unset_column       (BobguiColumnViewCellWidget         *self);

G_END_DECLS
