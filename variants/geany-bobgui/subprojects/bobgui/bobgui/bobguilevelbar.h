/* BOBGUI - The Bobgui Framework
 * Copyright © 2012 Red Hat, Inc.
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
 *
 * Author: Cosimo Cecchi <cosimoc@gnome.org>
 *
 */

#pragma once

#if !defined (__BOBGUI_H_INSIDE__) && !defined (BOBGUI_COMPILATION)
#error "Only <bobgui/bobgui.h> can be included directly."
#endif

#include <bobgui/bobguiwidget.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_LEVEL_BAR            (bobgui_level_bar_get_type ())
#define BOBGUI_LEVEL_BAR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_LEVEL_BAR, BobguiLevelBar))
#define BOBGUI_IS_LEVEL_BAR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOBGUI_TYPE_LEVEL_BAR))

/**
 * BOBGUI_LEVEL_BAR_OFFSET_LOW:
 *
 * The name used for the stock low offset included by `BobguiLevelBar`.
 */
#define BOBGUI_LEVEL_BAR_OFFSET_LOW  "low"

/**
 * BOBGUI_LEVEL_BAR_OFFSET_HIGH:
 *
 * The name used for the stock high offset included by `BobguiLevelBar`.
 */
#define BOBGUI_LEVEL_BAR_OFFSET_HIGH "high"

/**
 * BOBGUI_LEVEL_BAR_OFFSET_FULL:
 *
 * The name used for the stock full offset included by `BobguiLevelBar`.
 */
#define BOBGUI_LEVEL_BAR_OFFSET_FULL "full"

typedef struct _BobguiLevelBar        BobguiLevelBar;


GDK_AVAILABLE_IN_ALL
GType      bobgui_level_bar_get_type           (void) G_GNUC_CONST;

GDK_AVAILABLE_IN_ALL
BobguiWidget *bobgui_level_bar_new                (void);

GDK_AVAILABLE_IN_ALL
BobguiWidget *bobgui_level_bar_new_for_interval   (double       min_value,
                                             double       max_value);

GDK_AVAILABLE_IN_ALL
void       bobgui_level_bar_set_mode           (BobguiLevelBar *self,
                                             BobguiLevelBarMode mode);
GDK_AVAILABLE_IN_ALL
BobguiLevelBarMode bobgui_level_bar_get_mode      (BobguiLevelBar *self);

GDK_AVAILABLE_IN_ALL
void       bobgui_level_bar_set_value          (BobguiLevelBar *self,
                                             double       value);
GDK_AVAILABLE_IN_ALL
double     bobgui_level_bar_get_value          (BobguiLevelBar *self);

GDK_AVAILABLE_IN_ALL
void       bobgui_level_bar_set_min_value      (BobguiLevelBar *self,
                                             double       value);
GDK_AVAILABLE_IN_ALL
double     bobgui_level_bar_get_min_value      (BobguiLevelBar *self);

GDK_AVAILABLE_IN_ALL
void       bobgui_level_bar_set_max_value      (BobguiLevelBar *self,
                                             double       value);
GDK_AVAILABLE_IN_ALL
double     bobgui_level_bar_get_max_value      (BobguiLevelBar *self);

GDK_AVAILABLE_IN_ALL
void       bobgui_level_bar_set_inverted       (BobguiLevelBar *self,
                                             gboolean     inverted);

GDK_AVAILABLE_IN_ALL
gboolean   bobgui_level_bar_get_inverted       (BobguiLevelBar *self);

GDK_AVAILABLE_IN_ALL
void       bobgui_level_bar_add_offset_value   (BobguiLevelBar *self,
                                             const char *name,
                                             double       value);
GDK_AVAILABLE_IN_ALL
void       bobgui_level_bar_remove_offset_value (BobguiLevelBar *self,
                                              const char *name);
GDK_AVAILABLE_IN_ALL
gboolean   bobgui_level_bar_get_offset_value   (BobguiLevelBar *self,
                                             const char *name,
                                             double      *value);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(BobguiLevelBar, g_object_unref)

G_END_DECLS

