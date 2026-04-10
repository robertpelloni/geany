/* BOBGUI - The Bobgui Framework
 *
 * Copyright (C) 2022 Red Hat, Inc.
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

#pragma once

#if !defined (__BOBGUI_H_INSIDE__) && !defined (BOBGUI_COMPILATION)
#error "Only <bobgui/bobgui.h> can be included directly."
#endif

#include <gdk/gdk.h>
#include <bobgui/bobguiwindow.h>
#include <bobgui/bobguifilter.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_FONT_DIALOG (bobgui_font_dialog_get_type ())

GDK_AVAILABLE_IN_4_10
G_DECLARE_FINAL_TYPE (BobguiFontDialog, bobgui_font_dialog, BOBGUI, FONT_DIALOG, GObject)

GDK_AVAILABLE_IN_4_10
BobguiFontDialog *  bobgui_font_dialog_new           (void);

GDK_AVAILABLE_IN_4_10
const char *     bobgui_font_dialog_get_title     (BobguiFontDialog        *self);

GDK_AVAILABLE_IN_4_10
void             bobgui_font_dialog_set_title     (BobguiFontDialog        *self,
                                                const char           *title);

GDK_AVAILABLE_IN_4_10
gboolean         bobgui_font_dialog_get_modal     (BobguiFontDialog        *self);

GDK_AVAILABLE_IN_4_10
void             bobgui_font_dialog_set_modal     (BobguiFontDialog        *self,
                                                gboolean              modal);

GDK_AVAILABLE_IN_4_10
PangoLanguage *  bobgui_font_dialog_get_language  (BobguiFontDialog        *self);

GDK_AVAILABLE_IN_4_10
void             bobgui_font_dialog_set_language  (BobguiFontDialog        *self,
                                                PangoLanguage        *language);

GDK_AVAILABLE_IN_4_10
PangoFontMap *   bobgui_font_dialog_get_font_map  (BobguiFontDialog        *self);

GDK_AVAILABLE_IN_4_10
void             bobgui_font_dialog_set_font_map  (BobguiFontDialog        *self,
                                                PangoFontMap         *fontmap);

GDK_AVAILABLE_IN_4_10
BobguiFilter *      bobgui_font_dialog_get_filter    (BobguiFontDialog        *self);

GDK_AVAILABLE_IN_4_10
void             bobgui_font_dialog_set_filter    (BobguiFontDialog        *self,
                                                BobguiFilter            *filter);

GDK_AVAILABLE_IN_4_10
void             bobgui_font_dialog_choose_family (BobguiFontDialog        *self,
                                                BobguiWindow            *parent,
                                                PangoFontFamily      *initial_value,
                                                GCancellable         *cancellable,
                                                GAsyncReadyCallback   callback,
                                                gpointer              user_data);

GDK_AVAILABLE_IN_4_10
PangoFontFamily *
                 bobgui_font_dialog_choose_family_finish
                                               (BobguiFontDialog         *self,
                                                GAsyncResult          *result,
                                                GError               **error);

GDK_AVAILABLE_IN_4_10
void             bobgui_font_dialog_choose_face   (BobguiFontDialog        *self,
                                                BobguiWindow            *parent,
                                                PangoFontFace        *initial_value,
                                                GCancellable         *cancellable,
                                                GAsyncReadyCallback   callback,
                                                gpointer              user_data);

GDK_AVAILABLE_IN_4_10
PangoFontFace *  bobgui_font_dialog_choose_face_finish
                                               (BobguiFontDialog         *self,
                                                GAsyncResult          *result,
                                                GError               **error);

GDK_AVAILABLE_IN_4_10
void             bobgui_font_dialog_choose_font   (BobguiFontDialog        *self,
                                                BobguiWindow            *parent,
                                                PangoFontDescription *initial_value,
                                                GCancellable         *cancellable,
                                                GAsyncReadyCallback   callback,
                                                gpointer              user_data);

GDK_AVAILABLE_IN_4_10
PangoFontDescription *
                 bobgui_font_dialog_choose_font_finish
                                               (BobguiFontDialog         *self,
                                                GAsyncResult          *result,
                                                GError               **error);

GDK_AVAILABLE_IN_4_10
void             bobgui_font_dialog_choose_font_and_features
                                               (BobguiFontDialog        *self,
                                                BobguiWindow            *parent,
                                                PangoFontDescription *initial_value,
                                                GCancellable         *cancellable,
                                                GAsyncReadyCallback   callback,
                                                gpointer              user_data);

GDK_AVAILABLE_IN_4_10
gboolean        bobgui_font_dialog_choose_font_and_features_finish
                                               (BobguiFontDialog         *self,
                                                GAsyncResult          *result,
                                                PangoFontDescription **font_desc,
                                                char                 **font_features,
                                                PangoLanguage        **language,
                                                GError               **error);

G_END_DECLS
