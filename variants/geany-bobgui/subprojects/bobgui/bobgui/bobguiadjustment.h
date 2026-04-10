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

#include <gdk/gdk.h>
#include <bobgui/bobguitypes.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_ADJUSTMENT                  (bobgui_adjustment_get_type ())
#define BOBGUI_ADJUSTMENT(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_ADJUSTMENT, BobguiAdjustment))
#define BOBGUI_ADJUSTMENT_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), BOBGUI_TYPE_ADJUSTMENT, BobguiAdjustmentClass))
#define BOBGUI_IS_ADJUSTMENT(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOBGUI_TYPE_ADJUSTMENT))
#define BOBGUI_IS_ADJUSTMENT_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), BOBGUI_TYPE_ADJUSTMENT))
#define BOBGUI_ADJUSTMENT_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), BOBGUI_TYPE_ADJUSTMENT, BobguiAdjustmentClass))


typedef struct _BobguiAdjustmentClass    BobguiAdjustmentClass;

struct _BobguiAdjustment
{
  GInitiallyUnowned parent_instance;
};

struct _BobguiAdjustmentClass
{
  GInitiallyUnownedClass parent_class;

  void (* changed)       (BobguiAdjustment *adjustment);
  void (* value_changed) (BobguiAdjustment *adjustment);

  /* Padding for future expansion */
  void (*_bobgui_reserved1) (void);
  void (*_bobgui_reserved2) (void);
  void (*_bobgui_reserved3) (void);
  void (*_bobgui_reserved4) (void);
};


GDK_AVAILABLE_IN_ALL
GType      bobgui_adjustment_get_type              (void) G_GNUC_CONST;
GDK_AVAILABLE_IN_ALL
BobguiAdjustment*   bobgui_adjustment_new             (double           value,
                                                 double           lower,
                                                 double           upper,
                                                 double           step_increment,
                                                 double           page_increment,
                                                 double           page_size);

GDK_AVAILABLE_IN_ALL
void       bobgui_adjustment_clamp_page            (BobguiAdjustment   *adjustment,
                                                 double           lower,
                                                 double           upper);

GDK_AVAILABLE_IN_ALL
double     bobgui_adjustment_get_value             (BobguiAdjustment   *adjustment);
GDK_AVAILABLE_IN_ALL
void       bobgui_adjustment_set_value             (BobguiAdjustment   *adjustment,
                                                 double           value);
GDK_AVAILABLE_IN_ALL
double     bobgui_adjustment_get_lower             (BobguiAdjustment   *adjustment);
GDK_AVAILABLE_IN_ALL
void       bobgui_adjustment_set_lower             (BobguiAdjustment   *adjustment,
                                                 double           lower);
GDK_AVAILABLE_IN_ALL
double     bobgui_adjustment_get_upper             (BobguiAdjustment   *adjustment);
GDK_AVAILABLE_IN_ALL
void       bobgui_adjustment_set_upper             (BobguiAdjustment   *adjustment,
                                                 double           upper);
GDK_AVAILABLE_IN_ALL
double     bobgui_adjustment_get_step_increment    (BobguiAdjustment   *adjustment);
GDK_AVAILABLE_IN_ALL
void       bobgui_adjustment_set_step_increment    (BobguiAdjustment   *adjustment,
                                                 double           step_increment);
GDK_AVAILABLE_IN_ALL
double     bobgui_adjustment_get_page_increment    (BobguiAdjustment   *adjustment);
GDK_AVAILABLE_IN_ALL
void       bobgui_adjustment_set_page_increment    (BobguiAdjustment   *adjustment,
                                                 double           page_increment);
GDK_AVAILABLE_IN_ALL
double     bobgui_adjustment_get_page_size         (BobguiAdjustment   *adjustment);
GDK_AVAILABLE_IN_ALL
void       bobgui_adjustment_set_page_size         (BobguiAdjustment   *adjustment,
                                                 double           page_size);

GDK_AVAILABLE_IN_ALL
void       bobgui_adjustment_configure             (BobguiAdjustment   *adjustment,
                                                 double           value,
                                                 double           lower,
                                                 double           upper,
                                                 double           step_increment,
                                                 double           page_increment,
                                                 double           page_size);
GDK_AVAILABLE_IN_ALL
double     bobgui_adjustment_get_minimum_increment (BobguiAdjustment   *adjustment);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(BobguiAdjustment, g_object_unref)

G_END_DECLS

