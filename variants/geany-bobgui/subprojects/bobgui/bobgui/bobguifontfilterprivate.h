/* BOBGUI - The Bobgui Framework
 * bobguifontfilterprivate.h:
 * Copyright (C) 2024 Niels De Graef <nielsdegraef@gmail.com>
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

#include <bobgui/bobguifilter.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_FONT_FILTER (bobgui_font_filter_get_type ())
G_DECLARE_FINAL_TYPE (BobguiFontFilter, bobgui_font_filter,
                      BOBGUI, FONT_FILTER,
                      BobguiFilter)

BobguiFilter *        _bobgui_font_filter_new                    (void);

void               _bobgui_font_filter_set_pango_context      (BobguiFontFilter *self,
                                                            PangoContext  *context);

gboolean           _bobgui_font_filter_get_monospace          (BobguiFontFilter *self);

void               _bobgui_font_filter_set_monospace          (BobguiFontFilter *self,
                                                            gboolean       monospace);

PangoLanguage *    _bobgui_font_filter_get_language           (BobguiFontFilter *self);

void               _bobgui_font_filter_set_language           (BobguiFontFilter *self,
                                                            PangoLanguage *language);

G_END_DECLS

