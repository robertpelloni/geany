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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
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

#include <bobgui/bobguitexttypesprivate.h>
#include "bobguitextlayoutprivate.h"

G_BEGIN_DECLS

#define BOBGUI_IS_TEXT_MARK_SEGMENT(mark) (((BobguiTextLineSegment*)mark)->type == &bobgui_text_left_mark_type || \
                                ((BobguiTextLineSegment*)mark)->type == &bobgui_text_right_mark_type)

/*
 * The data structure below defines line segments that represent
 * marks.  There is one of these for each mark in the text.
 */

struct _BobguiTextMarkBody {
  BobguiTextMark *obj;
  char *name;
  BobguiTextBTree *tree;
  BobguiTextLine *line;
  guint visible : 1;
  guint not_deleteable : 1;
};

void _bobgui_mark_segment_set_tree (BobguiTextLineSegment *mark,
				 BobguiTextBTree       *tree);

G_END_DECLS




