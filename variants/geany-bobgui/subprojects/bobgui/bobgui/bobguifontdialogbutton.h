/*
 * BOBGUI - The Bobgui Framework
 * Copyright (C) 2022 Red Hat, Inc.
 * All rights reserved.
 *
 * This Library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#if !defined (__BOBGUI_H_INSIDE__) && !defined (BOBGUI_COMPILATION)
#error "Only <bobgui/bobgui.h> can be included directly."
#endif

#include <bobgui/bobguibutton.h>
#include <bobgui/bobguifontdialog.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_FONT_DIALOG_BUTTON (bobgui_font_dialog_button_get_type ())

GDK_AVAILABLE_IN_4_10
G_DECLARE_FINAL_TYPE (BobguiFontDialogButton, bobgui_font_dialog_button, BOBGUI, FONT_DIALOG_BUTTON, BobguiWidget)

GDK_AVAILABLE_IN_4_10
BobguiWidget *      bobgui_font_dialog_button_new              (BobguiFontDialog       *dialog);

GDK_AVAILABLE_IN_4_10
BobguiFontDialog *  bobgui_font_dialog_button_get_dialog       (BobguiFontDialogButton *self);

GDK_AVAILABLE_IN_4_10
void             bobgui_font_dialog_button_set_dialog       (BobguiFontDialogButton *self,
                                                          BobguiFontDialog       *dialog);

/**
 * BobguiFontLevel:
 * @BOBGUI_FONT_LEVEL_FAMILY: Select a font family
 * @BOBGUI_FONT_LEVEL_FACE: Select a font face (i.e. a family and a style)
 * @BOBGUI_FONT_LEVEL_FONT: Select a font (i.e. a face with a size, and possibly font variations)
 * @BOBGUI_FONT_LEVEL_FEATURES: Select a font and font features
 *
 * The level of granularity for the font selection.
 *
 * Depending on this value, the `PangoFontDescription` that
 * is returned by [method@Bobgui.FontDialogButton.get_font_desc]
 * will have more or less fields set.
 *
 * Since: 4.10
 */
typedef enum
{
  BOBGUI_FONT_LEVEL_FAMILY,
  BOBGUI_FONT_LEVEL_FACE,
  BOBGUI_FONT_LEVEL_FONT,
  BOBGUI_FONT_LEVEL_FEATURES
} BobguiFontLevel;

GDK_AVAILABLE_IN_4_10
BobguiFontLevel     bobgui_font_dialog_button_get_level        (BobguiFontDialogButton *self);

GDK_AVAILABLE_IN_4_10
void             bobgui_font_dialog_button_set_level        (BobguiFontDialogButton *self,
                                                          BobguiFontLevel         level);

GDK_AVAILABLE_IN_4_10
PangoFontDescription *
                 bobgui_font_dialog_button_get_font_desc    (BobguiFontDialogButton *self);

GDK_AVAILABLE_IN_4_10
void             bobgui_font_dialog_button_set_font_desc    (BobguiFontDialogButton        *self,
                                                          const PangoFontDescription *font_desc);

GDK_AVAILABLE_IN_4_10
const char *     bobgui_font_dialog_button_get_font_features
                                                         (BobguiFontDialogButton *self);

GDK_AVAILABLE_IN_4_10
void             bobgui_font_dialog_button_set_font_features
                                                         (BobguiFontDialogButton  *self,
                                                          const char           *font_features);

GDK_AVAILABLE_IN_4_10
PangoLanguage *  bobgui_font_dialog_button_get_language     (BobguiFontDialogButton *self);

GDK_AVAILABLE_IN_4_10
void             bobgui_font_dialog_button_set_language     (BobguiFontDialogButton  *self,
                                                          PangoLanguage        *language);

GDK_AVAILABLE_IN_4_10
gboolean         bobgui_font_dialog_button_get_use_font     (BobguiFontDialogButton *self);

GDK_AVAILABLE_IN_4_10
void             bobgui_font_dialog_button_set_use_font     (BobguiFontDialogButton  *self,
                                                          gboolean              use_font);

GDK_AVAILABLE_IN_4_10
gboolean         bobgui_font_dialog_button_get_use_size     (BobguiFontDialogButton *self);

GDK_AVAILABLE_IN_4_10
void             bobgui_font_dialog_button_set_use_size     (BobguiFontDialogButton  *self,
                                                          gboolean              use_size);

G_END_DECLS
