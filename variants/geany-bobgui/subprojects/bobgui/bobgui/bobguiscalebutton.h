/* BOBGUI - The Bobgui Framework
 * Copyright (C) 2005 Ronald S. Bultje
 * Copyright (C) 2006, 2007 Christian Persch
 * Copyright (C) 2006 Jan Arne Petersen
 * Copyright (C) 2007 Red Hat, Inc.
 *
 * Authors:
 * - Ronald S. Bultje <rbultje@ronald.bitfreak.net>
 * - Bastien Nocera <bnocera@redhat.com>
 * - Jan Arne Petersen <jpetersen@jpetersen.org>
 * - Christian Persch <chpe@svn.gnome.org>
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
 * Modified by the BOBGUI Team and others 2007.  See the AUTHORS
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

#define BOBGUI_TYPE_SCALE_BUTTON                 (bobgui_scale_button_get_type ())
#define BOBGUI_SCALE_BUTTON(obj)                 (G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_SCALE_BUTTON, BobguiScaleButton))
#define BOBGUI_SCALE_BUTTON_CLASS(klass)         (G_TYPE_CHECK_CLASS_CAST ((klass), BOBGUI_TYPE_SCALE_BUTTON, BobguiScaleButtonClass))
#define BOBGUI_IS_SCALE_BUTTON(obj)              (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOBGUI_TYPE_SCALE_BUTTON))
#define BOBGUI_IS_SCALE_BUTTON_CLASS(klass)      (G_TYPE_CHECK_CLASS_TYPE ((klass), BOBGUI_TYPE_SCALE_BUTTON))
#define BOBGUI_SCALE_BUTTON_GET_CLASS(obj)       (G_TYPE_INSTANCE_GET_CLASS ((obj), BOBGUI_TYPE_SCALE_BUTTON, BobguiScaleButtonClass))

typedef struct _BobguiScaleButton        BobguiScaleButton;
typedef struct _BobguiScaleButtonClass   BobguiScaleButtonClass;

struct _BobguiScaleButton
{
  BobguiWidget parent_instance;
};

struct _BobguiScaleButtonClass
{
  BobguiWidgetClass parent_class;

  /* signals */
  void	(* value_changed) (BobguiScaleButton *button,
                           double          value);

  /*< private >*/

  gpointer padding[8];
};

GDK_AVAILABLE_IN_ALL
GType            bobgui_scale_button_get_type         (void) G_GNUC_CONST;
GDK_AVAILABLE_IN_ALL
BobguiWidget *      bobgui_scale_button_new              (double           min,
                                                    double           max,
                                                    double           step,
                                                    const char     **icons);
GDK_AVAILABLE_IN_ALL
void             bobgui_scale_button_set_icons        (BobguiScaleButton  *button,
                                                    const char     **icons);
GDK_AVAILABLE_IN_ALL
double           bobgui_scale_button_get_value        (BobguiScaleButton  *button);
GDK_AVAILABLE_IN_ALL
void             bobgui_scale_button_set_value        (BobguiScaleButton  *button,
                                                    double           value);
GDK_AVAILABLE_IN_ALL
BobguiAdjustment *  bobgui_scale_button_get_adjustment   (BobguiScaleButton  *button);
GDK_AVAILABLE_IN_ALL
void             bobgui_scale_button_set_adjustment   (BobguiScaleButton  *button,
                                                    BobguiAdjustment   *adjustment);
GDK_AVAILABLE_IN_ALL
BobguiWidget *      bobgui_scale_button_get_plus_button  (BobguiScaleButton  *button);
GDK_AVAILABLE_IN_ALL
BobguiWidget *      bobgui_scale_button_get_minus_button (BobguiScaleButton  *button);
GDK_AVAILABLE_IN_ALL
BobguiWidget *      bobgui_scale_button_get_popup        (BobguiScaleButton  *button);
GDK_AVAILABLE_IN_4_10
gboolean         bobgui_scale_button_get_active       (BobguiScaleButton  *button);
GDK_AVAILABLE_IN_4_14
gboolean         bobgui_scale_button_get_has_frame    (BobguiScaleButton  *button);
GDK_AVAILABLE_IN_4_14
void             bobgui_scale_button_set_has_frame    (BobguiScaleButton  *button,
                                                    gboolean         has_frame);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(BobguiScaleButton, g_object_unref)

G_END_DECLS

