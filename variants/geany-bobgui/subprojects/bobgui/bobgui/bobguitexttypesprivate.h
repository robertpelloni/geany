/* BOBGUI - The Bobgui Framework
 * bobguitexttypes.h Copyright (C) 2000 Red Hat, Inc.
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

#include <bobgui/bobgui.h>
#include <bobgui/bobguitexttagprivate.h>

G_BEGIN_DECLS

/*
 * This is the PUBLIC representation of a text buffer.
 * BobguiTextBTree is the PRIVATE internal representation of it.
 */

typedef struct _BobguiTextBTree BobguiTextBTree;

typedef struct _BobguiTextCounter BobguiTextCounter;
typedef struct _BobguiTextLineSegment BobguiTextLineSegment;
typedef struct _BobguiTextLineSegmentClass BobguiTextLineSegmentClass;
typedef struct _BobguiTextToggleBody BobguiTextToggleBody;
typedef struct _BobguiTextMarkBody BobguiTextMarkBody;

/*
 * Declarations for variables shared among the text-related files:
 */

/* In bobguitextbtree.c */
extern G_GNUC_INTERNAL const BobguiTextLineSegmentClass bobgui_text_char_type;
extern G_GNUC_INTERNAL const BobguiTextLineSegmentClass bobgui_text_toggle_on_type;
extern G_GNUC_INTERNAL const BobguiTextLineSegmentClass bobgui_text_toggle_off_type;

/* In bobguitextmark.c */
extern G_GNUC_INTERNAL const BobguiTextLineSegmentClass bobgui_text_left_mark_type;
extern G_GNUC_INTERNAL const BobguiTextLineSegmentClass bobgui_text_right_mark_type;

/* In bobguitextchild.c */
extern G_GNUC_INTERNAL const BobguiTextLineSegmentClass bobgui_text_paintable_type;
extern G_GNUC_INTERNAL const BobguiTextLineSegmentClass bobgui_text_child_type;

/*
 * UTF 8 Stubs
 */

#define BOBGUI_TEXT_UNKNOWN_CHAR 0xFFFC
#define BOBGUI_TEXT_UNKNOWN_CHAR_UTF8_LEN 3
GDK_AVAILABLE_IN_ALL
const char *bobgui_text_unknown_char_utf8_bobgui_tests_only (void);
extern const char _bobgui_text_unknown_char_utf8[BOBGUI_TEXT_UNKNOWN_CHAR_UTF8_LEN+1];

GDK_AVAILABLE_IN_ALL
gboolean bobgui_text_byte_begins_utf8_char (const char *byte);

G_END_DECLS


