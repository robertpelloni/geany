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

#include <bobgui/bobguiwidget.h>


G_BEGIN_DECLS

#define BOBGUI_TYPE_DRAWING_AREA            (bobgui_drawing_area_get_type ())
#define BOBGUI_DRAWING_AREA(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_DRAWING_AREA, BobguiDrawingArea))
#define BOBGUI_DRAWING_AREA_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BOBGUI_TYPE_DRAWING_AREA, BobguiDrawingAreaClass))
#define BOBGUI_IS_DRAWING_AREA(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOBGUI_TYPE_DRAWING_AREA))
#define BOBGUI_IS_DRAWING_AREA_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BOBGUI_TYPE_DRAWING_AREA))
#define BOBGUI_DRAWING_AREA_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BOBGUI_TYPE_DRAWING_AREA, BobguiDrawingAreaClass))

typedef struct _BobguiDrawingArea       BobguiDrawingArea;
typedef struct _BobguiDrawingAreaClass  BobguiDrawingAreaClass;

/**
 * BobguiDrawingAreaDrawFunc:
 * @drawing_area: the `BobguiDrawingArea` to redraw
 * @cr: the context to draw to
 * @width: the actual width of the contents. This value will be at least
 *   as wide as BobguiDrawingArea:width.
 * @height: the actual height of the contents. This value will be at least
 *   as wide as BobguiDrawingArea:height.
 * @user_data: (closure): user data
 *
 * Whenever @drawing_area needs to redraw, this function will be called.
 *
 * This function should exclusively redraw the contents of the drawing area
 * and must not call any widget functions that cause changes.
 */
typedef void (* BobguiDrawingAreaDrawFunc)  (BobguiDrawingArea *drawing_area,
                                          cairo_t        *cr,
                                          int             width,
                                          int             height,
                                          gpointer        user_data);

struct _BobguiDrawingArea
{
  BobguiWidget widget;
};

struct _BobguiDrawingAreaClass
{
  BobguiWidgetClass parent_class;

  void           (* resize)         (BobguiDrawingArea *area,
                                     int             width,
                                     int             height);

  /*< private >*/

  gpointer padding[8];
};


GDK_AVAILABLE_IN_ALL
GType      bobgui_drawing_area_get_type (void) G_GNUC_CONST;
GDK_AVAILABLE_IN_ALL
BobguiWidget* bobgui_drawing_area_new      (void);

GDK_AVAILABLE_IN_ALL
void            bobgui_drawing_area_set_content_width      (BobguiDrawingArea         *self,
                                                         int                     width);
GDK_AVAILABLE_IN_ALL
int             bobgui_drawing_area_get_content_width      (BobguiDrawingArea         *self);
GDK_AVAILABLE_IN_ALL
void            bobgui_drawing_area_set_content_height     (BobguiDrawingArea         *self,
                                                         int                     height);
GDK_AVAILABLE_IN_ALL
int             bobgui_drawing_area_get_content_height     (BobguiDrawingArea         *self);
GDK_AVAILABLE_IN_ALL
void            bobgui_drawing_area_set_draw_func          (BobguiDrawingArea         *self,
                                                         BobguiDrawingAreaDrawFunc  draw_func,
                                                         gpointer                user_data,
                                                         GDestroyNotify          destroy);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(BobguiDrawingArea, g_object_unref)

G_END_DECLS

