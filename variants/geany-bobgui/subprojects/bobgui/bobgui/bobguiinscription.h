/*
 * Copyright © 2022 Benjamin Otte
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors: Benjamin Otte <otte@gnome.org>
 */

#pragma once

#if !defined (__BOBGUI_H_INSIDE__) && !defined (BOBGUI_COMPILATION)
#error "Only <bobgui/bobgui.h> can be included directly."
#endif

#include <bobgui/bobguiwidget.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_INSCRIPTION         (bobgui_inscription_get_type ())

/**
 * BobguiInscriptionOverflow:
 * @BOBGUI_INSCRIPTION_OVERFLOW_CLIP: Clip the remaining text
 * @BOBGUI_INSCRIPTION_OVERFLOW_ELLIPSIZE_START: Omit characters at the start of the text
 * @BOBGUI_INSCRIPTION_OVERFLOW_ELLIPSIZE_MIDDLE: Omit characters at the middle of the text
 * @BOBGUI_INSCRIPTION_OVERFLOW_ELLIPSIZE_END: Omit characters at the end of the text
 *
 * The different methods to handle text in #BobguiInscription when it doesn't
 * fit the available space.
 *
 * Since: 4.8
 */
typedef enum {
  BOBGUI_INSCRIPTION_OVERFLOW_CLIP,
  BOBGUI_INSCRIPTION_OVERFLOW_ELLIPSIZE_START,
  BOBGUI_INSCRIPTION_OVERFLOW_ELLIPSIZE_MIDDLE,
  BOBGUI_INSCRIPTION_OVERFLOW_ELLIPSIZE_END
} BobguiInscriptionOverflow;

GDK_AVAILABLE_IN_4_8
G_DECLARE_FINAL_TYPE (BobguiInscription, bobgui_inscription, BOBGUI, INSCRIPTION, BobguiWidget)

GDK_AVAILABLE_IN_4_8
BobguiWidget *             bobgui_inscription_new                     (const char             *text);

GDK_AVAILABLE_IN_4_8
const char *            bobgui_inscription_get_text                (BobguiInscription         *self);
GDK_AVAILABLE_IN_4_8
void                    bobgui_inscription_set_text                (BobguiInscription         *self,
                                                                 const char             *text);
GDK_AVAILABLE_IN_4_8
PangoAttrList *         bobgui_inscription_get_attributes          (BobguiInscription         *self);
GDK_AVAILABLE_IN_4_8
void                    bobgui_inscription_set_attributes          (BobguiInscription         *self,
                                                                 PangoAttrList          *attrs);
GDK_AVAILABLE_IN_4_8
void                    bobgui_inscription_set_markup              (BobguiInscription         *self,
                                                                 const char             *markup);
GDK_AVAILABLE_IN_4_8
BobguiInscriptionOverflow  bobgui_inscription_get_text_overflow       (BobguiInscription         *self);
GDK_AVAILABLE_IN_4_8
void                    bobgui_inscription_set_text_overflow       (BobguiInscription         *self,
                                                                 BobguiInscriptionOverflow  overflow);
GDK_AVAILABLE_IN_4_8
PangoWrapMode           bobgui_inscription_get_wrap_mode           (BobguiInscription         *self);
GDK_AVAILABLE_IN_4_8
void                    bobgui_inscription_set_wrap_mode           (BobguiInscription         *self,
                                                                 PangoWrapMode           wrap_mode);

GDK_AVAILABLE_IN_4_8
guint                   bobgui_inscription_get_min_chars           (BobguiInscription         *self);
GDK_AVAILABLE_IN_4_8
void                    bobgui_inscription_set_min_chars           (BobguiInscription         *self,
                                                                 guint                   min_chars);
GDK_AVAILABLE_IN_4_8
guint                   bobgui_inscription_get_nat_chars           (BobguiInscription         *self);
GDK_AVAILABLE_IN_4_8
void                    bobgui_inscription_set_nat_chars           (BobguiInscription         *self,
                                                                 guint                   nat_chars);
GDK_AVAILABLE_IN_4_8
guint                   bobgui_inscription_get_min_lines           (BobguiInscription         *self);
GDK_AVAILABLE_IN_4_8
void                    bobgui_inscription_set_min_lines           (BobguiInscription         *self,
                                                                 guint                   min_lines);
GDK_AVAILABLE_IN_4_8
guint                   bobgui_inscription_get_nat_lines           (BobguiInscription         *self);
GDK_AVAILABLE_IN_4_8
void                    bobgui_inscription_set_nat_lines           (BobguiInscription         *self,
                                                                 guint                   nat_lines);

GDK_AVAILABLE_IN_4_8
float                   bobgui_inscription_get_xalign              (BobguiInscription         *self);
GDK_AVAILABLE_IN_4_8
void                    bobgui_inscription_set_xalign              (BobguiInscription         *self,
                                                                 float                   xalign);
GDK_AVAILABLE_IN_4_8
float                   bobgui_inscription_get_yalign              (BobguiInscription         *self);
GDK_AVAILABLE_IN_4_8
void                    bobgui_inscription_set_yalign              (BobguiInscription         *self,
                                                                 float                   yalign);

G_END_DECLS

