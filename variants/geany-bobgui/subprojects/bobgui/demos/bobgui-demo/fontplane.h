/* BOBGUI - The GIMP Toolkit
 * Copyright (C) 2012 Red Hat, Inc.
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

#include <bobgui/bobgui.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_FONT_PLANE            (bobgui_font_plane_get_type ())
#define BOBGUI_FONT_PLANE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_FONT_PLANE, BobguiFontPlane))
#define BOBGUI_FONT_PLANE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BOBGUI_TYPE_FONT_PLANE, BobguiFontPlaneClass))
#define BOBGUI_IS_FONT_PLANE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOBGUI_TYPE_COLOR_PLANE))
#define BOBGUI_IS_FONT_PLANE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BOBGUI_TYPE_COLOR_PLANE))
#define BOBGUI_FONT_PLANE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BOBGUI_TYPE_FONT_PLANE, BobguiFontPlaneClass))


typedef struct _BobguiFontPlane         BobguiFontPlane;
typedef struct _BobguiFontPlaneClass    BobguiFontPlaneClass;

struct _BobguiFontPlane
{
  BobguiWidget parent_instance;

  BobguiAdjustment *weight_adj;
  BobguiAdjustment *width_adj;

  BobguiGesture *drag_gesture;
};

struct _BobguiFontPlaneClass
{
  BobguiWidgetClass parent_class;

  /* Padding for future expansion */
  void (*_bobgui_reserved1) (void);
  void (*_bobgui_reserved2) (void);
  void (*_bobgui_reserved3) (void);
  void (*_bobgui_reserved4) (void);
};


GType       bobgui_font_plane_get_type (void) G_GNUC_CONST;
BobguiWidget * bobgui_font_plane_new      (BobguiAdjustment *weight_adj,
                                     BobguiAdjustment *width_adj);

G_END_DECLS
