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
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Authors: Benjamin Otte <otte@gnome.org>
 */

#pragma once

#include <glib.h>

G_BEGIN_DECLS

typedef struct _BobguiBitmask BobguiBitmask;

#define _bobgui_bitmask_to_bits(mask) \
  (GPOINTER_TO_SIZE (mask) >> ((gsize) 1))

#define _bobgui_bitmask_from_bits(bits) \
  GSIZE_TO_POINTER ((((gsize) (bits)) << 1) | 1)

#define _bobgui_bitmask_is_allocated(mask) \
  (!(GPOINTER_TO_SIZE (mask) & 1))

#define BOBGUI_BITMASK_N_DIRECT_BITS (sizeof (gsize) * 8 - 1)


BobguiBitmask *   _bobgui_allocated_bitmask_copy              (const BobguiBitmask  *mask);
void           _bobgui_allocated_bitmask_free              (BobguiBitmask        *mask);

char *         _bobgui_allocated_bitmask_to_string         (const BobguiBitmask  *mask);
void           _bobgui_allocated_bitmask_print             (const BobguiBitmask  *mask,
                                                         GString           *string);

BobguiBitmask *   _bobgui_allocated_bitmask_intersect         (BobguiBitmask        *mask,
                                                         const BobguiBitmask  *other) G_GNUC_WARN_UNUSED_RESULT;
BobguiBitmask *   _bobgui_allocated_bitmask_union             (BobguiBitmask        *mask,
                                                         const BobguiBitmask  *other) G_GNUC_WARN_UNUSED_RESULT;
BobguiBitmask *   _bobgui_allocated_bitmask_subtract          (BobguiBitmask        *mask,
                                                         const BobguiBitmask  *other) G_GNUC_WARN_UNUSED_RESULT;

gboolean       _bobgui_allocated_bitmask_get               (const BobguiBitmask  *mask,
                                                         guint              index_);
BobguiBitmask *   _bobgui_allocated_bitmask_set               (BobguiBitmask        *mask,
                                                         guint              index_,
                                                         gboolean           value) G_GNUC_WARN_UNUSED_RESULT;

BobguiBitmask *   _bobgui_allocated_bitmask_invert_range      (BobguiBitmask        *mask,
                                                         guint              start,
                                                         guint              end) G_GNUC_WARN_UNUSED_RESULT;

gboolean       _bobgui_allocated_bitmask_equals            (const BobguiBitmask  *mask,
                                                         const BobguiBitmask  *other);
gboolean       _bobgui_allocated_bitmask_intersects        (const BobguiBitmask  *mask,
                                                         const BobguiBitmask  *other);

G_END_DECLS

