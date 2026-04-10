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

#include <bobgui/bobguirange.h>


G_BEGIN_DECLS

#define BOBGUI_TYPE_SCALE            (bobgui_scale_get_type ())
#define BOBGUI_SCALE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_SCALE, BobguiScale))
#define BOBGUI_SCALE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BOBGUI_TYPE_SCALE, BobguiScaleClass))
#define BOBGUI_IS_SCALE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOBGUI_TYPE_SCALE))
#define BOBGUI_IS_SCALE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BOBGUI_TYPE_SCALE))
#define BOBGUI_SCALE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BOBGUI_TYPE_SCALE, BobguiScaleClass))


typedef struct _BobguiScale              BobguiScale;
typedef struct _BobguiScaleClass         BobguiScaleClass;

struct _BobguiScale
{
  BobguiRange parent_instance;
};

struct _BobguiScaleClass
{
  BobguiRangeClass parent_class;

  void (* get_layout_offsets) (BobguiScale *scale,
                               int      *x,
                               int      *y);

  /*< private >*/

  gpointer padding[8];
};


/**
 * BobguiScaleFormatValueFunc:
 * @scale: The `BobguiScale`
 * @value: The numeric value to format
 * @user_data: (closure): user data
 *
 * Function that formats the value of a scale.
 *
 * See [method@Bobgui.Scale.set_format_value_func].
 *
 * Returns: (not nullable): A newly allocated string describing a textual representation
 *   of the given numerical value.
 */
typedef char * (*BobguiScaleFormatValueFunc) (BobguiScale *scale,
                                           double    value,
                                           gpointer  user_data);


GDK_AVAILABLE_IN_ALL
GType             bobgui_scale_get_type           (void) G_GNUC_CONST;
GDK_AVAILABLE_IN_ALL
BobguiWidget       * bobgui_scale_new                (BobguiOrientation   orientation,
                                                BobguiAdjustment   *adjustment);
GDK_AVAILABLE_IN_ALL
BobguiWidget       * bobgui_scale_new_with_range     (BobguiOrientation   orientation,
                                                double           min,
                                                double           max,
                                                double           step);
GDK_AVAILABLE_IN_ALL
void              bobgui_scale_set_digits         (BobguiScale        *scale,
                                                int              digits);
GDK_AVAILABLE_IN_ALL
int               bobgui_scale_get_digits         (BobguiScale        *scale);
GDK_AVAILABLE_IN_ALL
void              bobgui_scale_set_draw_value     (BobguiScale        *scale,
                                                gboolean         draw_value);
GDK_AVAILABLE_IN_ALL
gboolean          bobgui_scale_get_draw_value     (BobguiScale        *scale);
GDK_AVAILABLE_IN_ALL
void              bobgui_scale_set_has_origin     (BobguiScale        *scale,
                                                gboolean         has_origin);
GDK_AVAILABLE_IN_ALL
gboolean          bobgui_scale_get_has_origin     (BobguiScale        *scale);
GDK_AVAILABLE_IN_ALL
void              bobgui_scale_set_value_pos      (BobguiScale        *scale,
                                                BobguiPositionType  pos);
GDK_AVAILABLE_IN_ALL
BobguiPositionType   bobgui_scale_get_value_pos      (BobguiScale        *scale);

GDK_AVAILABLE_IN_ALL
PangoLayout     * bobgui_scale_get_layout         (BobguiScale        *scale);
GDK_AVAILABLE_IN_ALL
void              bobgui_scale_get_layout_offsets (BobguiScale        *scale,
                                                int             *x,
                                                int             *y);

GDK_AVAILABLE_IN_ALL
void              bobgui_scale_add_mark           (BobguiScale        *scale,
                                                double           value,
                                                BobguiPositionType  position,
                                                const char      *markup);
GDK_AVAILABLE_IN_ALL
void              bobgui_scale_clear_marks        (BobguiScale        *scale);

GDK_AVAILABLE_IN_ALL
void              bobgui_scale_set_format_value_func (BobguiScale                *scale,
                                                   BobguiScaleFormatValueFunc  func,
                                                   gpointer                 user_data,
                                                   GDestroyNotify           destroy_notify);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(BobguiScale, g_object_unref)

G_END_DECLS

