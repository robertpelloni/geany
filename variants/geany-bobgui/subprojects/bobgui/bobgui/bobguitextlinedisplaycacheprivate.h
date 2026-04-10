/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* BOBGUI - The Bobgui Framework
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 * Copyright (C) 2019 Red Hat, Inc.
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

#include "bobguitextlayoutprivate.h"

G_BEGIN_DECLS

typedef struct _BobguiTextLineDisplayCache BobguiTextLineDisplayCache;

BobguiTextLineDisplayCache *bobgui_text_line_display_cache_new                (void);
void                     bobgui_text_line_display_cache_free               (BobguiTextLineDisplayCache *cache);
BobguiTextLineDisplay      *bobgui_text_line_display_cache_get                (BobguiTextLineDisplayCache *cache,
                                                                         BobguiTextLayout           *layout,
                                                                         BobguiTextLine             *line,
                                                                         gboolean                 size_only);
void                     bobgui_text_line_display_cache_delay_eviction     (BobguiTextLineDisplayCache *cache);
void                     bobgui_text_line_display_cache_set_cursor_line    (BobguiTextLineDisplayCache *cache,
                                                                         BobguiTextLine             *line);
void                     bobgui_text_line_display_cache_invalidate         (BobguiTextLineDisplayCache *cache);
void                     bobgui_text_line_display_cache_invalidate_cursors (BobguiTextLineDisplayCache *cache,
                                                                         BobguiTextLine             *line);
void                     bobgui_text_line_display_cache_invalidate_display (BobguiTextLineDisplayCache *cache,
                                                                         BobguiTextLineDisplay      *display,
                                                                         gboolean                 cursors_only);
void                     bobgui_text_line_display_cache_invalidate_line    (BobguiTextLineDisplayCache *cache,
                                                                         BobguiTextLine             *line);
void                     bobgui_text_line_display_cache_invalidate_range   (BobguiTextLineDisplayCache *cache,
                                                                         BobguiTextLayout           *layout,
                                                                         const BobguiTextIter       *begin,
                                                                         const BobguiTextIter       *end,
                                                                         gboolean                 cursors_only);
void                     bobgui_text_line_display_cache_invalidate_y_range (BobguiTextLineDisplayCache *cache,
                                                                         BobguiTextLayout           *layout,
                                                                         int                      y,
                                                                         int                      old_height,
                                                                         int                      new_height,
                                                                         gboolean                 cursors_only);
void                     bobgui_text_line_display_cache_set_mru_size       (BobguiTextLineDisplayCache *cache,
                                                                         guint                    mru_size);

G_END_DECLS

