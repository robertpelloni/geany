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

#include "bobguilistfactorywidgetprivate.h"

G_BEGIN_DECLS

#define BOBGUI_TYPE_COLUMN_VIEW_ROW_WIDGET         (bobgui_column_view_row_widget_get_type ())
#define BOBGUI_COLUMN_VIEW_ROW_WIDGET(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), BOBGUI_TYPE_COLUMN_VIEW_ROW_WIDGET, BobguiColumnViewRowWidget))
#define BOBGUI_COLUMN_VIEW_ROW_WIDGET_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), BOBGUI_TYPE_COLUMN_VIEW_ROW_WIDGET, BobguiColumnViewRowWidgetClass))
#define BOBGUI_IS_COLUMN_VIEW_ROW_WIDGET(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), BOBGUI_TYPE_COLUMN_VIEW_ROW_WIDGET))
#define BOBGUI_IS_COLUMN_VIEW_ROW_WIDGET_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), BOBGUI_TYPE_COLUMN_VIEW_ROW_WIDGET))
#define BOBGUI_COLUMN_VIEW_ROW_WIDGET_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), BOBGUI_TYPE_COLUMN_VIEW_ROW_WIDGET, BobguiColumnViewRowWidgetClass))

typedef struct _BobguiColumnViewRowWidget BobguiColumnViewRowWidget;
typedef struct _BobguiColumnViewRowWidgetClass BobguiColumnViewRowWidgetClass;

struct _BobguiColumnViewRowWidget
{
  BobguiListFactoryWidget parent_instance;
};

struct _BobguiColumnViewRowWidgetClass
{
  BobguiListFactoryWidgetClass parent_class;
};

GType                   bobgui_column_view_row_widget_get_type             (void) G_GNUC_CONST;

BobguiWidget *             bobgui_column_view_row_widget_new                  (BobguiListItemFactory     *factory,
                                                                         gboolean                is_header);

void                    bobgui_column_view_row_widget_add_child            (BobguiColumnViewRowWidget *self,
                                                                         BobguiWidget              *child);
void                    bobgui_column_view_row_widget_reorder_child        (BobguiColumnViewRowWidget *self,
                                                                         BobguiWidget              *child,
                                                                         guint                   position);
void                    bobgui_column_view_row_widget_remove_child         (BobguiColumnViewRowWidget *self,
                                                                         BobguiWidget              *child);

G_END_DECLS

