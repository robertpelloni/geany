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

#include <bobgui/bobguiborder.h>
#include <bobgui/bobguiwidget.h>


G_BEGIN_DECLS


#define BOBGUI_TYPE_RANGE            (bobgui_range_get_type ())
#define BOBGUI_RANGE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_RANGE, BobguiRange))
#define BOBGUI_RANGE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BOBGUI_TYPE_RANGE, BobguiRangeClass))
#define BOBGUI_IS_RANGE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOBGUI_TYPE_RANGE))
#define BOBGUI_IS_RANGE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BOBGUI_TYPE_RANGE))
#define BOBGUI_RANGE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BOBGUI_TYPE_RANGE, BobguiRangeClass))

typedef struct _BobguiRange              BobguiRange;
typedef struct _BobguiRangeClass         BobguiRangeClass;

struct _BobguiRange
{
  BobguiWidget parent_instance;
};

struct _BobguiRangeClass
{
  BobguiWidgetClass parent_class;

  void (* value_changed)    (BobguiRange     *range);
  void (* adjust_bounds)    (BobguiRange     *range,
                             double	   new_value);

  /* action signals for keybindings */
  void (* move_slider)      (BobguiRange     *range,
                             BobguiScrollType scroll);

  /* Virtual functions */
  void (* get_range_border) (BobguiRange     *range,
                             BobguiBorder    *border_);

  gboolean (* change_value) (BobguiRange     *range,
                             BobguiScrollType scroll,
                             double        new_value);

  /*< private > */

  gpointer padding[8];
};


GDK_AVAILABLE_IN_ALL
GType              bobgui_range_get_type                      (void) G_GNUC_CONST;

GDK_AVAILABLE_IN_ALL
void               bobgui_range_set_adjustment                (BobguiRange      *range,
                                                            BobguiAdjustment *adjustment);
GDK_AVAILABLE_IN_ALL
BobguiAdjustment*     bobgui_range_get_adjustment                (BobguiRange      *range);

GDK_AVAILABLE_IN_ALL
void               bobgui_range_set_inverted                  (BobguiRange      *range,
                                                            gboolean       setting);
GDK_AVAILABLE_IN_ALL
gboolean           bobgui_range_get_inverted                  (BobguiRange      *range);

GDK_AVAILABLE_IN_ALL
void               bobgui_range_set_flippable                 (BobguiRange      *range,
                                                            gboolean       flippable);
GDK_AVAILABLE_IN_ALL
gboolean           bobgui_range_get_flippable                 (BobguiRange      *range);

GDK_AVAILABLE_IN_ALL
void               bobgui_range_set_slider_size_fixed         (BobguiRange      *range,
                                                            gboolean       size_fixed);
GDK_AVAILABLE_IN_ALL
gboolean           bobgui_range_get_slider_size_fixed         (BobguiRange      *range);

GDK_AVAILABLE_IN_ALL
void               bobgui_range_get_range_rect                (BobguiRange      *range,
                                                            GdkRectangle  *range_rect);
GDK_AVAILABLE_IN_ALL
void               bobgui_range_get_slider_range              (BobguiRange      *range,
                                                            int           *slider_start,
                                                            int           *slider_end);

GDK_AVAILABLE_IN_ALL
void               bobgui_range_set_increments                (BobguiRange      *range,
                                                            double         step,
                                                            double         page);
GDK_AVAILABLE_IN_ALL
void               bobgui_range_set_range                     (BobguiRange      *range,
                                                            double         min,
                                                            double         max);
GDK_AVAILABLE_IN_ALL
void               bobgui_range_set_value                     (BobguiRange      *range,
                                                            double         value);
GDK_AVAILABLE_IN_ALL
double             bobgui_range_get_value                     (BobguiRange      *range);

GDK_AVAILABLE_IN_ALL
void               bobgui_range_set_show_fill_level           (BobguiRange      *range,
                                                            gboolean       show_fill_level);
GDK_AVAILABLE_IN_ALL
gboolean           bobgui_range_get_show_fill_level           (BobguiRange      *range);
GDK_AVAILABLE_IN_ALL
void               bobgui_range_set_restrict_to_fill_level    (BobguiRange      *range,
                                                            gboolean       restrict_to_fill_level);
GDK_AVAILABLE_IN_ALL
gboolean           bobgui_range_get_restrict_to_fill_level    (BobguiRange      *range);
GDK_AVAILABLE_IN_ALL
void               bobgui_range_set_fill_level                (BobguiRange      *range,
                                                            double         fill_level);
GDK_AVAILABLE_IN_ALL
double             bobgui_range_get_fill_level                (BobguiRange      *range);
GDK_AVAILABLE_IN_ALL
void               bobgui_range_set_round_digits              (BobguiRange      *range,
                                                            int            round_digits);
GDK_AVAILABLE_IN_ALL
int                 bobgui_range_get_round_digits              (BobguiRange      *range);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(BobguiRange, g_object_unref)

G_END_DECLS


