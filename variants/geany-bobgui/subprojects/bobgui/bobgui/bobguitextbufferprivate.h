/* BOBGUI - The Bobgui Framework
 * bobguitextbufferprivate.h Copyright (C) 2015 Red Hat, Inc.
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

#include <bobgui/bobguitextbuffer.h>
#include "bobguitexttypesprivate.h"

G_BEGIN_DECLS

void            _bobgui_text_buffer_spew                  (BobguiTextBuffer      *buffer);

BobguiTextBTree*   _bobgui_text_buffer_get_btree             (BobguiTextBuffer      *buffer);

const PangoLogAttr* _bobgui_text_buffer_get_line_log_attrs (BobguiTextBuffer     *buffer,
                                                         const BobguiTextIter *anywhere_in_line,
                                                         int               *char_len);

void _bobgui_text_buffer_notify_will_remove_tag (BobguiTextBuffer *buffer,
                                              BobguiTextTag    *tag);


const char *bobgui_justification_to_string (BobguiJustification just);
const char *bobgui_text_direction_to_string (BobguiTextDirection direction);
const char *bobgui_wrap_mode_to_string (BobguiWrapMode wrap_mode);

void bobgui_text_buffer_add_run_attributes (BobguiTextBuffer   *buffer,
                                         int              offset,
                                         GHashTable      *attributes,
                                         int             *start_offset,
                                         int             *end_offset);

G_END_DECLS
