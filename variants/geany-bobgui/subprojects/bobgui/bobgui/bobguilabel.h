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

#define BOBGUI_TYPE_LABEL		  (bobgui_label_get_type ())
#define BOBGUI_LABEL(obj)		  (G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_LABEL, BobguiLabel))
#define BOBGUI_IS_LABEL(obj)	  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOBGUI_TYPE_LABEL))

typedef struct _BobguiLabel BobguiLabel;

GDK_AVAILABLE_IN_ALL
GType                 bobgui_label_get_type          (void) G_GNUC_CONST;
GDK_AVAILABLE_IN_ALL
BobguiWidget*            bobgui_label_new               (const char    *str);
GDK_AVAILABLE_IN_ALL
BobguiWidget*            bobgui_label_new_with_mnemonic (const char    *str);
GDK_AVAILABLE_IN_ALL
void                  bobgui_label_set_text          (BobguiLabel      *self,
						   const char    *str);
GDK_AVAILABLE_IN_ALL
const char *          bobgui_label_get_text          (BobguiLabel      *self);
GDK_AVAILABLE_IN_ALL
void                  bobgui_label_set_attributes    (BobguiLabel      *self,
						   PangoAttrList *attrs);
GDK_AVAILABLE_IN_ALL
PangoAttrList        *bobgui_label_get_attributes    (BobguiLabel      *self);
GDK_AVAILABLE_IN_ALL
void                  bobgui_label_set_label         (BobguiLabel      *self,
						   const char    *str);
GDK_AVAILABLE_IN_ALL
const char *         bobgui_label_get_label         (BobguiLabel      *self);
GDK_AVAILABLE_IN_ALL
void                  bobgui_label_set_markup        (BobguiLabel      *self,
						   const char    *str);
GDK_AVAILABLE_IN_ALL
void                  bobgui_label_set_use_markup    (BobguiLabel      *self,
						   gboolean       setting);
GDK_AVAILABLE_IN_ALL
gboolean              bobgui_label_get_use_markup    (BobguiLabel      *self);
GDK_AVAILABLE_IN_ALL
void                  bobgui_label_set_use_underline (BobguiLabel      *self,
						   gboolean       setting);
GDK_AVAILABLE_IN_ALL
gboolean              bobgui_label_get_use_underline (BobguiLabel      *self);

GDK_AVAILABLE_IN_ALL
void     bobgui_label_set_markup_with_mnemonic       (BobguiLabel         *self,
						   const char       *str);
GDK_AVAILABLE_IN_ALL
guint    bobgui_label_get_mnemonic_keyval            (BobguiLabel         *self);
GDK_AVAILABLE_IN_ALL
void     bobgui_label_set_mnemonic_widget            (BobguiLabel         *self,
						   BobguiWidget        *widget);
GDK_AVAILABLE_IN_ALL
BobguiWidget *bobgui_label_get_mnemonic_widget          (BobguiLabel         *self);
GDK_AVAILABLE_IN_ALL
void     bobgui_label_set_text_with_mnemonic         (BobguiLabel         *self,
						   const char       *str);
GDK_AVAILABLE_IN_ALL
void     bobgui_label_set_justify                    (BobguiLabel         *self,
						   BobguiJustification  jtype);
GDK_AVAILABLE_IN_ALL
BobguiJustification bobgui_label_get_justify            (BobguiLabel         *self);
GDK_AVAILABLE_IN_ALL
void     bobgui_label_set_ellipsize                  (BobguiLabel         *self,
                                                   PangoEllipsizeMode mode);
GDK_AVAILABLE_IN_ALL
PangoEllipsizeMode bobgui_label_get_ellipsize        (BobguiLabel         *self);
GDK_AVAILABLE_IN_ALL
void     bobgui_label_set_width_chars                (BobguiLabel         *self,
                                                   int               n_chars);
GDK_AVAILABLE_IN_ALL
int      bobgui_label_get_width_chars                (BobguiLabel         *self);
GDK_AVAILABLE_IN_ALL
void     bobgui_label_set_max_width_chars            (BobguiLabel         *self,
                                                   int               n_chars);
GDK_AVAILABLE_IN_ALL
int      bobgui_label_get_max_width_chars            (BobguiLabel         *self);
GDK_AVAILABLE_IN_ALL
void     bobgui_label_set_lines                      (BobguiLabel         *self,
                                                   int               lines);
GDK_AVAILABLE_IN_ALL
int      bobgui_label_get_lines                      (BobguiLabel         *self);
GDK_AVAILABLE_IN_ALL
void     bobgui_label_set_wrap                       (BobguiLabel         *self,
                                                   gboolean          wrap);
GDK_AVAILABLE_IN_ALL
gboolean bobgui_label_get_wrap                       (BobguiLabel         *self);
GDK_AVAILABLE_IN_ALL
void     bobgui_label_set_wrap_mode                  (BobguiLabel         *self,
                                                   PangoWrapMode     wrap_mode);
GDK_AVAILABLE_IN_ALL
PangoWrapMode bobgui_label_get_wrap_mode             (BobguiLabel         *self);
GDK_AVAILABLE_IN_4_6
void          bobgui_label_set_natural_wrap_mode     (BobguiLabel         *self,
                                                   BobguiNaturalWrapMode wrap_mode);
GDK_AVAILABLE_IN_4_6
BobguiNaturalWrapMode bobgui_label_get_natural_wrap_mode(BobguiLabel         *self);
GDK_AVAILABLE_IN_ALL
void     bobgui_label_set_selectable                 (BobguiLabel         *self,
						   gboolean          setting);
GDK_AVAILABLE_IN_ALL
gboolean bobgui_label_get_selectable                 (BobguiLabel         *self);
GDK_AVAILABLE_IN_ALL
void     bobgui_label_select_region                  (BobguiLabel         *self,
						   int               start_offset,
						   int               end_offset);
GDK_AVAILABLE_IN_ALL
gboolean bobgui_label_get_selection_bounds           (BobguiLabel         *self,
                                                   int              *start,
                                                   int              *end);

GDK_AVAILABLE_IN_ALL
PangoLayout *bobgui_label_get_layout         (BobguiLabel *self);
GDK_AVAILABLE_IN_ALL
void         bobgui_label_get_layout_offsets (BobguiLabel *self,
                                           int      *x,
                                           int      *y);

GDK_AVAILABLE_IN_ALL
void         bobgui_label_set_single_line_mode  (BobguiLabel *self,
                                              gboolean single_line_mode);
GDK_AVAILABLE_IN_ALL
gboolean     bobgui_label_get_single_line_mode  (BobguiLabel *self);

GDK_AVAILABLE_IN_ALL
const char *bobgui_label_get_current_uri (BobguiLabel *self);

GDK_AVAILABLE_IN_ALL
void         bobgui_label_set_xalign (BobguiLabel *self,
                                   float     xalign);

GDK_AVAILABLE_IN_ALL
float        bobgui_label_get_xalign (BobguiLabel *self);

GDK_AVAILABLE_IN_ALL
void         bobgui_label_set_yalign (BobguiLabel *self,
                                   float     yalign);

GDK_AVAILABLE_IN_ALL
float        bobgui_label_get_yalign (BobguiLabel *self);

GDK_AVAILABLE_IN_ALL
void         bobgui_label_set_extra_menu (BobguiLabel   *self,
                                       GMenuModel *model);
GDK_AVAILABLE_IN_ALL
GMenuModel * bobgui_label_get_extra_menu (BobguiLabel   *self);

GDK_AVAILABLE_IN_4_8
void             bobgui_label_set_tabs (BobguiLabel      *self,
                                     PangoTabArray *tabs);

GDK_AVAILABLE_IN_4_8
PangoTabArray * bobgui_label_get_tabs  (BobguiLabel      *self);



G_DEFINE_AUTOPTR_CLEANUP_FUNC(BobguiLabel, g_object_unref)

G_END_DECLS

