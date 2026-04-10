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

#define BOBGUI_TYPE_FIXED                  (bobgui_fixed_get_type ())
#define BOBGUI_FIXED(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_FIXED, BobguiFixed))
#define BOBGUI_FIXED_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), BOBGUI_TYPE_FIXED, BobguiFixedClass))
#define BOBGUI_IS_FIXED(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOBGUI_TYPE_FIXED))
#define BOBGUI_IS_FIXED_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), BOBGUI_TYPE_FIXED))
#define BOBGUI_FIXED_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), BOBGUI_TYPE_FIXED, BobguiFixedClass))

typedef struct _BobguiFixed              BobguiFixed;
typedef struct _BobguiFixedClass         BobguiFixedClass;

struct _BobguiFixed
{
  BobguiWidget parent_instance;
};

struct _BobguiFixedClass
{
  BobguiWidgetClass parent_class;

  /*< private >*/

  gpointer padding[8];
};

GDK_AVAILABLE_IN_ALL
GType bobgui_fixed_get_type (void) G_GNUC_CONST;

GDK_AVAILABLE_IN_ALL
BobguiWidget *     bobgui_fixed_new                (void);
GDK_AVAILABLE_IN_ALL
void            bobgui_fixed_put                   (BobguiFixed     *fixed,
                                                 BobguiWidget    *widget,
                                                 double        x,
                                                 double        y);
GDK_AVAILABLE_IN_ALL
void            bobgui_fixed_remove                (BobguiFixed     *fixed,
                                                 BobguiWidget    *widget);
GDK_AVAILABLE_IN_ALL
void            bobgui_fixed_move                  (BobguiFixed     *fixed,
                                                 BobguiWidget    *widget,
                                                 double        x,
                                                 double        y);
GDK_AVAILABLE_IN_ALL
void            bobgui_fixed_get_child_position    (BobguiFixed     *fixed,
                                                 BobguiWidget    *widget,
                                                 double       *x,
                                                 double       *y);

GDK_AVAILABLE_IN_ALL
void            bobgui_fixed_set_child_transform   (BobguiFixed     *fixed,
                                                 BobguiWidget    *widget,
                                                 GskTransform *transform);
GDK_AVAILABLE_IN_ALL
GskTransform *  bobgui_fixed_get_child_transform   (BobguiFixed     *fixed,
                                                 BobguiWidget    *widget);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(BobguiFixed, g_object_unref)

G_END_DECLS

