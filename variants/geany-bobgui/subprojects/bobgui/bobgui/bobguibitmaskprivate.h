/*
 * Copyright © 2011 Red Hat Inc.
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

#include <glib.h>
#include "bobguiallocatedbitmaskprivate.h"

G_BEGIN_DECLS

static inline BobguiBitmask *      _bobgui_bitmask_new                  (void);
static inline BobguiBitmask *      _bobgui_bitmask_copy                 (const BobguiBitmask  *mask);
static inline void              _bobgui_bitmask_free                 (BobguiBitmask        *mask);

static inline char *            _bobgui_bitmask_to_string            (const BobguiBitmask  *mask);
static inline void              _bobgui_bitmask_print                (const BobguiBitmask  *mask,
                                                                   GString           *string);

static inline BobguiBitmask *      _bobgui_bitmask_intersect            (BobguiBitmask        *mask,
                                                                   const BobguiBitmask  *other) G_GNUC_WARN_UNUSED_RESULT;
static inline BobguiBitmask *      _bobgui_bitmask_union                (BobguiBitmask        *mask,
                                                                   const BobguiBitmask  *other) G_GNUC_WARN_UNUSED_RESULT;
static inline BobguiBitmask *      _bobgui_bitmask_subtract             (BobguiBitmask        *mask,
                                                                   const BobguiBitmask  *other) G_GNUC_WARN_UNUSED_RESULT;

static inline gboolean          _bobgui_bitmask_get                  (const BobguiBitmask  *mask,
                                                                   guint              index_);
static inline BobguiBitmask *      _bobgui_bitmask_set                  (BobguiBitmask        *mask,
                                                                   guint              index_,
                                                                   gboolean           value) G_GNUC_WARN_UNUSED_RESULT;

static inline BobguiBitmask *      _bobgui_bitmask_invert_range         (BobguiBitmask        *mask,
                                                                   guint              start,
                                                                   guint              end) G_GNUC_WARN_UNUSED_RESULT;

static inline gboolean          _bobgui_bitmask_is_empty             (const BobguiBitmask  *mask);
static inline gboolean          _bobgui_bitmask_equals               (const BobguiBitmask  *mask,
                                                                   const BobguiBitmask  *other);
static inline gboolean          _bobgui_bitmask_intersects           (const BobguiBitmask  *mask,
                                                                   const BobguiBitmask  *other);


/* This is the actual implementation of the functions declared above.
 * We put it in a separate file so people don’t get scared from looking at this
 * file when reading source code.
 */
#include "bobguibitmaskprivateimpl.h"


G_END_DECLS

