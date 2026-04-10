/*
 * Copyright (c) 2017 Timm Bäder <mail@baedert.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Author: Timm Bäder <mail@baedert.org>
 *
 */

#pragma once

#if !defined (__BOBGUI_H_INSIDE__) && !defined (BOBGUI_COMPILATION)
#error "Only <bobgui/bobgui.h> can be included directly."
#endif

#include "bobguiwidget.h"

G_BEGIN_DECLS

#define BOBGUI_TYPE_CENTER_BOX                 (bobgui_center_box_get_type ())
#define BOBGUI_CENTER_BOX(obj)                 (G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_CENTER_BOX, BobguiCenterBox))
#define BOBGUI_CENTER_BOX_CLASS(klass)         (G_TYPE_CHECK_CLASS_CAST ((klass), BOBGUI_TYPE_CENTER_BOX, BobguiCenterBoxClass))
#define BOBGUI_IS_CENTER_BOX(obj)              (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOBGUI_TYPE_CENTER_BOX))
#define BOBGUI_IS_CENTER_BOX_CLASS(klass)      (G_TYPE_CHECK_CLASS_TYPE ((klass), BOBGUI_TYPE_CENTER_BOX))
#define BOBGUI_CENTER_BOX_GET_CLASS(obj)       (G_TYPE_INSTANCE_GET_CLASS ((obj), BOBGUI_TYPE_CENTER_BOX, BobguiCenterBoxClass))

typedef struct _BobguiCenterBox             BobguiCenterBox;
typedef struct _BobguiCenterBoxClass        BobguiCenterBoxClass;

GDK_AVAILABLE_IN_ALL
GType      bobgui_center_box_get_type (void) G_GNUC_CONST;

GDK_AVAILABLE_IN_ALL
BobguiWidget *bobgui_center_box_new (void);
GDK_AVAILABLE_IN_ALL
void       bobgui_center_box_set_start_widget   (BobguiCenterBox *self,
                                              BobguiWidget    *child);
GDK_AVAILABLE_IN_ALL
void       bobgui_center_box_set_center_widget  (BobguiCenterBox *self,
                                              BobguiWidget    *child);
GDK_AVAILABLE_IN_ALL
void       bobgui_center_box_set_end_widget     (BobguiCenterBox *self,
                                              BobguiWidget    *child);

GDK_AVAILABLE_IN_ALL
BobguiWidget * bobgui_center_box_get_start_widget  (BobguiCenterBox *self);
GDK_AVAILABLE_IN_ALL
BobguiWidget * bobgui_center_box_get_center_widget (BobguiCenterBox *self);
GDK_AVAILABLE_IN_ALL
BobguiWidget * bobgui_center_box_get_end_widget    (BobguiCenterBox *self);

GDK_AVAILABLE_IN_ALL
void                bobgui_center_box_set_baseline_position (BobguiCenterBox        *self,
                                                          BobguiBaselinePosition  position);
GDK_AVAILABLE_IN_ALL
BobguiBaselinePosition bobgui_center_box_get_baseline_position (BobguiCenterBox        *self);

GDK_AVAILABLE_IN_4_12
void        bobgui_center_box_set_shrink_center_last (BobguiCenterBox *self,
                                                   gboolean      shrink_center_last);
GDK_AVAILABLE_IN_4_12
gboolean    bobgui_center_box_get_shrink_center_last (BobguiCenterBox *self);

G_END_DECLS

