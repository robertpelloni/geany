/* BOBGUI - The Bobgui Framework
 * bobguitextiterprivate.h Copyright (C) 2000 Red Hat, Inc.
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

#include <bobgui/bobguitextiter.h>

G_BEGIN_DECLS

#include <bobgui/bobguitextiter.h>
#include <bobgui/bobguitextbtreeprivate.h>
#include <bobgui/bobguitextattributesprivate.h>

BobguiTextLineSegment *_bobgui_text_iter_get_indexable_segment      (const BobguiTextIter *iter);
BobguiTextLineSegment *_bobgui_text_iter_get_any_segment            (const BobguiTextIter *iter);
BobguiTextLine *       _bobgui_text_iter_get_text_line              (const BobguiTextIter *iter);
BobguiTextBTree *      _bobgui_text_iter_get_btree                  (const BobguiTextIter *iter);
gboolean            _bobgui_text_iter_forward_indexable_segment  (BobguiTextIter       *iter);
gboolean            _bobgui_text_iter_backward_indexable_segment (BobguiTextIter       *iter);
int                 _bobgui_text_iter_get_segment_byte           (const BobguiTextIter *iter);
int                 _bobgui_text_iter_get_segment_char           (const BobguiTextIter *iter);
gboolean            _bobgui_text_iter_same_line                  (const BobguiTextIter *lhs,
                                                               const BobguiTextIter *rhs);

gboolean       bobgui_text_iter_get_attributes (const BobguiTextIter *iter,
                                             BobguiTextAttributes *values);

/* debug */
void _bobgui_text_iter_check (const BobguiTextIter *iter);

G_END_DECLS



