/* bobguicellareacontext.h
 *
 * Copyright (C) 2010 Openismus GmbH
 *
 * Authors:
 *      Tristan Van Berkom <tristanvb@openismus.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#if !defined (__BOBGUI_H_INSIDE__) && !defined (BOBGUI_COMPILATION)
#error "Only <bobgui/bobgui.h> can be included directly."
#endif

#include <bobgui/deprecated/bobguicellarea.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_CELL_AREA_CONTEXT            (bobgui_cell_area_context_get_type ())
#define BOBGUI_CELL_AREA_CONTEXT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_CELL_AREA_CONTEXT, BobguiCellAreaContext))
#define BOBGUI_CELL_AREA_CONTEXT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BOBGUI_TYPE_CELL_AREA_CONTEXT, BobguiCellAreaContextClass))
#define BOBGUI_IS_CELL_AREA_CONTEXT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOBGUI_TYPE_CELL_AREA_CONTEXT))
#define BOBGUI_IS_CELL_AREA_CONTEXT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BOBGUI_TYPE_CELL_AREA_CONTEXT))
#define BOBGUI_CELL_AREA_CONTEXT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BOBGUI_TYPE_CELL_AREA_CONTEXT, BobguiCellAreaContextClass))

typedef struct _BobguiCellAreaContextPrivate       BobguiCellAreaContextPrivate;
typedef struct _BobguiCellAreaContextClass         BobguiCellAreaContextClass;

struct _BobguiCellAreaContext
{
  /*< private >*/
  GObject parent_instance;
};

/**
 * BobguiCellAreaContextClass:
 * @allocate: This tells the context that an allocation width or height
 *   (or both) have been decided for a group of rows. The context should
 *   store any allocations for internally aligned cells at this point so
 *   that they dont need to be recalculated at bobgui_cell_area_render() time.
 * @reset: Clear any previously stored information about requested and
 *   allocated sizes for the context.
 * @get_preferred_height_for_width: Returns the aligned height for the given
 *   width that context must store while collecting sizes for it’s rows.
 * @get_preferred_width_for_height: Returns the aligned width for the given
 *   height that context must store while collecting sizes for it’s rows.
 */
struct _BobguiCellAreaContextClass
{
  /*< private >*/
  GObjectClass parent_class;

  /*< public >*/
  void    (* allocate)                       (BobguiCellAreaContext *context,
                                              int                 width,
                                              int                 height);
  void    (* reset)                          (BobguiCellAreaContext *context);
  void    (* get_preferred_height_for_width) (BobguiCellAreaContext *context,
                                              int                 width,
                                              int                *minimum_height,
                                              int                *natural_height);
  void    (* get_preferred_width_for_height) (BobguiCellAreaContext *context,
                                              int                 height,
                                              int                *minimum_width,
                                              int                *natural_width);

  /*< private >*/

  gpointer padding[8];
};

GDK_AVAILABLE_IN_ALL
GType        bobgui_cell_area_context_get_type              (void) G_GNUC_CONST;

/* Main apis */
GDK_DEPRECATED_IN_4_10
BobguiCellArea *bobgui_cell_area_context_get_area                        (BobguiCellAreaContext *context);
GDK_DEPRECATED_IN_4_10
void         bobgui_cell_area_context_allocate                        (BobguiCellAreaContext *context,
                                                                    int                 width,
                                                                    int                 height);
GDK_DEPRECATED_IN_4_10
void         bobgui_cell_area_context_reset                           (BobguiCellAreaContext *context);

/* Apis for BobguiCellArea clients to consult cached values
 * for a series of BobguiTreeModel rows
 */
GDK_DEPRECATED_IN_4_10
void         bobgui_cell_area_context_get_preferred_width            (BobguiCellAreaContext *context,
                                                                   int                *minimum_width,
                                                                   int                *natural_width);
GDK_DEPRECATED_IN_4_10
void         bobgui_cell_area_context_get_preferred_height           (BobguiCellAreaContext *context,
                                                                   int                *minimum_height,
                                                                   int                *natural_height);
GDK_DEPRECATED_IN_4_10
void         bobgui_cell_area_context_get_preferred_height_for_width (BobguiCellAreaContext *context,
                                                                   int                 width,
                                                                   int                *minimum_height,
                                                                   int                *natural_height);
GDK_DEPRECATED_IN_4_10
void         bobgui_cell_area_context_get_preferred_width_for_height (BobguiCellAreaContext *context,
                                                                   int                 height,
                                                                   int                *minimum_width,
                                                                   int                *natural_width);
GDK_DEPRECATED_IN_4_10
void         bobgui_cell_area_context_get_allocation                 (BobguiCellAreaContext *context,
                                                                   int                *width,
                                                                   int                *height);

/* Apis for BobguiCellArea implementations to update cached values
 * for multiple BobguiTreeModel rows
 */
GDK_DEPRECATED_IN_4_10
void         bobgui_cell_area_context_push_preferred_width  (BobguiCellAreaContext *context,
                                                          int                 minimum_width,
                                                          int                 natural_width);
GDK_DEPRECATED_IN_4_10
void         bobgui_cell_area_context_push_preferred_height (BobguiCellAreaContext *context,
                                                          int                 minimum_height,
                                                          int                 natural_height);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(BobguiCellAreaContext, g_object_unref)

G_END_DECLS

