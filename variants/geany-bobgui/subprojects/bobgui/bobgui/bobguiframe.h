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


#define BOBGUI_TYPE_FRAME                  (bobgui_frame_get_type ())
#define BOBGUI_FRAME(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_FRAME, BobguiFrame))
#define BOBGUI_FRAME_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), BOBGUI_TYPE_FRAME, BobguiFrameClass))
#define BOBGUI_IS_FRAME(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOBGUI_TYPE_FRAME))
#define BOBGUI_IS_FRAME_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), BOBGUI_TYPE_FRAME))
#define BOBGUI_FRAME_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), BOBGUI_TYPE_FRAME, BobguiFrameClass))

typedef struct _BobguiFrame              BobguiFrame;
typedef struct _BobguiFrameClass         BobguiFrameClass;

struct _BobguiFrame
{
  BobguiWidget parent_instance;
};

/**
 * BobguiFrameClass:
 * @parent_class: The parent class.
 * @compute_child_allocation:
 */
struct _BobguiFrameClass
{
  BobguiWidgetClass parent_class;

  /*< public >*/

  void (*compute_child_allocation) (BobguiFrame *frame,
                                    BobguiAllocation *allocation);

  /*< private >*/

  gpointer padding[8];
};


GDK_AVAILABLE_IN_ALL
GType      bobgui_frame_get_type         (void) G_GNUC_CONST;
GDK_AVAILABLE_IN_ALL
BobguiWidget* bobgui_frame_new              (const char    *label);

GDK_AVAILABLE_IN_ALL
void          bobgui_frame_set_label (BobguiFrame    *frame,
                                   const char *label);
GDK_AVAILABLE_IN_ALL
const char * bobgui_frame_get_label (BobguiFrame    *frame);

GDK_AVAILABLE_IN_ALL
void       bobgui_frame_set_label_widget (BobguiFrame      *frame,
                                       BobguiWidget     *label_widget);
GDK_AVAILABLE_IN_ALL
BobguiWidget *bobgui_frame_get_label_widget (BobguiFrame      *frame);
GDK_AVAILABLE_IN_ALL
void       bobgui_frame_set_label_align  (BobguiFrame      *frame,
                                       float          xalign);
GDK_AVAILABLE_IN_ALL
float      bobgui_frame_get_label_align  (BobguiFrame      *frame);

GDK_AVAILABLE_IN_ALL
void       bobgui_frame_set_child        (BobguiFrame      *frame,
                                       BobguiWidget     *child);
GDK_AVAILABLE_IN_ALL
BobguiWidget *bobgui_frame_get_child        (BobguiFrame      *frame);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(BobguiFrame, g_object_unref)

G_END_DECLS


