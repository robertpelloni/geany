/* BOBGUI - The Bobgui Framework
 * Copyright (C) 2010 Red Hat, Inc.
 * Author: Matthias Clasen
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


#if !defined (__BOBGUI_H_INSIDE__) && !defined (BOBGUI_COMPILATION)
#error "Only <bobgui/bobgui.h> can be included directly."
#endif

#include <bobgui/bobguiwidget.h>


G_BEGIN_DECLS

#define BOBGUI_TYPE_GRID                   (bobgui_grid_get_type ())
#define BOBGUI_GRID(obj)                   (G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_GRID, BobguiGrid))
#define BOBGUI_GRID_CLASS(klass)           (G_TYPE_CHECK_CLASS_CAST ((klass), BOBGUI_TYPE_GRID, BobguiGridClass))
#define BOBGUI_IS_GRID(obj)                (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOBGUI_TYPE_GRID))
#define BOBGUI_IS_GRID_CLASS(klass)        (G_TYPE_CHECK_CLASS_TYPE ((klass), BOBGUI_TYPE_GRID))
#define BOBGUI_GRID_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), BOBGUI_TYPE_GRID, BobguiGridClass))


typedef struct _BobguiGrid              BobguiGrid;
typedef struct _BobguiGridClass         BobguiGridClass;

struct _BobguiGrid
{
  /*< private >*/
  BobguiWidget parent_instance;
};

/**
 * BobguiGridClass:
 * @parent_class: The parent class.
 */
struct _BobguiGridClass
{
  BobguiWidgetClass parent_class;

  /*< private >*/

  gpointer padding[8];
};

GDK_AVAILABLE_IN_ALL
GType      bobgui_grid_get_type               (void) G_GNUC_CONST;
GDK_AVAILABLE_IN_ALL
BobguiWidget* bobgui_grid_new                    (void);
GDK_AVAILABLE_IN_ALL
void       bobgui_grid_attach                 (BobguiGrid         *grid,
                                            BobguiWidget       *child,
                                            int              column,
                                            int              row,
                                            int              width,
                                            int              height);
GDK_AVAILABLE_IN_ALL
void       bobgui_grid_attach_next_to         (BobguiGrid         *grid,
                                            BobguiWidget       *child,
                                            BobguiWidget       *sibling,
                                            BobguiPositionType  side,
                                            int              width,
                                            int              height);
GDK_AVAILABLE_IN_ALL
BobguiWidget *bobgui_grid_get_child_at           (BobguiGrid         *grid,
                                            int              column,
                                            int              row);
GDK_AVAILABLE_IN_ALL
void       bobgui_grid_remove                 (BobguiGrid         *grid,
                                            BobguiWidget       *child);

GDK_AVAILABLE_IN_ALL
void       bobgui_grid_insert_row             (BobguiGrid         *grid,
                                            int              position);
GDK_AVAILABLE_IN_ALL
void       bobgui_grid_insert_column          (BobguiGrid         *grid,
                                            int              position);
GDK_AVAILABLE_IN_ALL
void       bobgui_grid_remove_row             (BobguiGrid         *grid,
                                            int              position);
GDK_AVAILABLE_IN_ALL
void       bobgui_grid_remove_column          (BobguiGrid         *grid,
                                            int              position);
GDK_AVAILABLE_IN_ALL
void       bobgui_grid_insert_next_to         (BobguiGrid         *grid,
                                            BobguiWidget       *sibling,
                                            BobguiPositionType  side);
GDK_AVAILABLE_IN_ALL
void       bobgui_grid_set_row_homogeneous    (BobguiGrid         *grid,
                                            gboolean         homogeneous);
GDK_AVAILABLE_IN_ALL
gboolean   bobgui_grid_get_row_homogeneous    (BobguiGrid         *grid);
GDK_AVAILABLE_IN_ALL
void       bobgui_grid_set_row_spacing        (BobguiGrid         *grid,
                                            guint            spacing);
GDK_AVAILABLE_IN_ALL
guint      bobgui_grid_get_row_spacing        (BobguiGrid         *grid);
GDK_AVAILABLE_IN_ALL
void       bobgui_grid_set_column_homogeneous (BobguiGrid         *grid,
                                            gboolean         homogeneous);
GDK_AVAILABLE_IN_ALL
gboolean   bobgui_grid_get_column_homogeneous (BobguiGrid         *grid);
GDK_AVAILABLE_IN_ALL
void       bobgui_grid_set_column_spacing     (BobguiGrid         *grid,
                                            guint            spacing);
GDK_AVAILABLE_IN_ALL
guint      bobgui_grid_get_column_spacing     (BobguiGrid         *grid);
GDK_AVAILABLE_IN_ALL
void       bobgui_grid_set_row_baseline_position (BobguiGrid      *grid,
                                               int           row,
                                               BobguiBaselinePosition pos);
GDK_AVAILABLE_IN_ALL
BobguiBaselinePosition bobgui_grid_get_row_baseline_position (BobguiGrid      *grid,
                                                        int           row);
GDK_AVAILABLE_IN_ALL
void       bobgui_grid_set_baseline_row       (BobguiGrid         *grid,
                                            int              row);
GDK_AVAILABLE_IN_ALL
int        bobgui_grid_get_baseline_row       (BobguiGrid         *grid);

GDK_AVAILABLE_IN_ALL
void       bobgui_grid_query_child            (BobguiGrid         *grid,
                                            BobguiWidget       *child,
                                            int             *column,
                                            int             *row,
                                            int             *width,
                                            int             *height);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(BobguiGrid, g_object_unref)

G_END_DECLS

