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

#define BOBGUI_TYPE_COLUMN_VIEW_TITLE         (bobgui_column_view_title_get_type ())
#define BOBGUI_COLUMN_VIEW_TITLE(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), BOBGUI_TYPE_COLUMN_VIEW_TITLE, BobguiColumnViewTitle))
#define BOBGUI_COLUMN_VIEW_TITLE_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), BOBGUI_TYPE_COLUMN_VIEW_TITLE, BobguiColumnViewTitleClass))
#define BOBGUI_IS_COLUMN_VIEW_TITLE(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), BOBGUI_TYPE_COLUMN_VIEW_TITLE))
#define BOBGUI_IS_COLUMN_VIEW_TITLE_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), BOBGUI_TYPE_COLUMN_VIEW_TITLE))
#define BOBGUI_COLUMN_VIEW_TITLE_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), BOBGUI_TYPE_COLUMN_VIEW_TITLE, BobguiColumnViewTitleClass))

typedef struct _BobguiColumnViewTitle BobguiColumnViewTitle;
typedef struct _BobguiColumnViewTitleClass BobguiColumnViewTitleClass;

GType                   bobgui_column_view_title_get_type          (void) G_GNUC_CONST;

BobguiWidget *             bobgui_column_view_title_new               (BobguiColumnViewColumn    *column);

void                    bobgui_column_view_title_set_title         (BobguiColumnViewTitle     *self,
                                                                 const char             *title);
void                    bobgui_column_view_title_set_menu          (BobguiColumnViewTitle     *self,
                                                                 GMenuModel             *model);

void                    bobgui_column_view_title_update_sort       (BobguiColumnViewTitle     *self);

BobguiColumnViewColumn *   bobgui_column_view_title_get_column        (BobguiColumnViewTitle     *self);

G_END_DECLS

