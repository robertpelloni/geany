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


#define BOBGUI_TYPE_BOX            (bobgui_box_get_type ())
#define BOBGUI_BOX(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_BOX, BobguiBox))
#define BOBGUI_BOX_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BOBGUI_TYPE_BOX, BobguiBoxClass))
#define BOBGUI_IS_BOX(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOBGUI_TYPE_BOX))
#define BOBGUI_IS_BOX_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BOBGUI_TYPE_BOX))
#define BOBGUI_BOX_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BOBGUI_TYPE_BOX, BobguiBoxClass))


typedef struct _BobguiBox              BobguiBox;
typedef struct _BobguiBoxClass         BobguiBoxClass;

struct _BobguiBox
{
  BobguiWidget parent_instance;
};

/**
 * BobguiBoxClass:
 * @parent_class: The parent class.
 */
struct _BobguiBoxClass
{
  BobguiWidgetClass parent_class;

  /*< private >*/

  gpointer padding[8];
};


GDK_AVAILABLE_IN_ALL
GType       bobgui_box_get_type            (void) G_GNUC_CONST;
GDK_AVAILABLE_IN_ALL
BobguiWidget*  bobgui_box_new                 (BobguiOrientation  orientation,
                                         int             spacing);
GDK_AVAILABLE_IN_ALL
void        bobgui_box_set_homogeneous     (BobguiBox         *box,
                                         gboolean        homogeneous);
GDK_AVAILABLE_IN_ALL
gboolean    bobgui_box_get_homogeneous     (BobguiBox         *box);
GDK_AVAILABLE_IN_ALL
void        bobgui_box_set_spacing         (BobguiBox         *box,
                                         int             spacing);
GDK_AVAILABLE_IN_ALL
int         bobgui_box_get_spacing         (BobguiBox         *box);
GDK_AVAILABLE_IN_ALL
void        bobgui_box_set_baseline_position (BobguiBox             *box,
                                           BobguiBaselinePosition position);
GDK_AVAILABLE_IN_ALL
BobguiBaselinePosition bobgui_box_get_baseline_position (BobguiBox         *box);

GDK_AVAILABLE_IN_4_12
void        bobgui_box_set_baseline_child (BobguiBox         *box,
                                        int             child);
GDK_AVAILABLE_IN_4_12
int         bobgui_box_get_baseline_child (BobguiBox         *box);

GDK_AVAILABLE_IN_ALL
void        bobgui_box_append             (BobguiBox         *box,
                                        BobguiWidget      *child);
GDK_AVAILABLE_IN_ALL
void        bobgui_box_prepend            (BobguiBox         *box,
                                        BobguiWidget      *child);
GDK_AVAILABLE_IN_ALL
void        bobgui_box_remove             (BobguiBox         *box,
                                        BobguiWidget      *child);

GDK_AVAILABLE_IN_ALL
void        bobgui_box_insert_child_after (BobguiBox         *box,
                                        BobguiWidget      *child,
                                        BobguiWidget      *sibling);

GDK_AVAILABLE_IN_ALL
void        bobgui_box_reorder_child_after (BobguiBox         *box,
                                         BobguiWidget      *child,
                                         BobguiWidget      *sibling);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(BobguiBox, g_object_unref)

G_END_DECLS

