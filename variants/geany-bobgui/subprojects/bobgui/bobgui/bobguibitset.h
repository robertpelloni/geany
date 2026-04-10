/*
 * Copyright © 2020 Benjamin Otte
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

#if !defined (__BOBGUI_H_INSIDE__) && !defined (BOBGUI_COMPILATION)
#error "Only <bobgui/bobgui.h> can be included directly."
#endif

#include <bobgui/bobguitypes.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_BITSET (bobgui_bitset_get_type ())

GDK_AVAILABLE_IN_ALL
GType                   bobgui_bitset_get_type                     (void) G_GNUC_CONST;

GDK_AVAILABLE_IN_ALL
BobguiBitset *             bobgui_bitset_ref                          (BobguiBitset              *self);
GDK_AVAILABLE_IN_ALL
void                    bobgui_bitset_unref                        (BobguiBitset              *self);
G_DEFINE_AUTOPTR_CLEANUP_FUNC(BobguiBitset, bobgui_bitset_unref)

GDK_AVAILABLE_IN_ALL
gboolean                bobgui_bitset_contains                     (const BobguiBitset        *self,
                                                                 guint                   value);
GDK_AVAILABLE_IN_ALL
gboolean                bobgui_bitset_is_empty                     (const BobguiBitset        *self);
GDK_AVAILABLE_IN_ALL
gboolean                bobgui_bitset_equals                       (const BobguiBitset        *self,
                                                                 const BobguiBitset        *other);
GDK_AVAILABLE_IN_ALL
guint64                 bobgui_bitset_get_size                     (const BobguiBitset        *self);
GDK_AVAILABLE_IN_ALL
guint64                 bobgui_bitset_get_size_in_range            (const BobguiBitset        *self,
                                                                 guint                   first,
                                                                 guint                   last);
GDK_AVAILABLE_IN_ALL
guint                   bobgui_bitset_get_nth                      (const BobguiBitset        *self,
                                                                 guint                   nth);
GDK_AVAILABLE_IN_ALL
guint                   bobgui_bitset_get_minimum                  (const BobguiBitset        *self);
GDK_AVAILABLE_IN_ALL
guint                   bobgui_bitset_get_maximum                  (const BobguiBitset        *self);

GDK_AVAILABLE_IN_ALL
BobguiBitset *             bobgui_bitset_new_empty                    (void);
GDK_AVAILABLE_IN_ALL
BobguiBitset *             bobgui_bitset_copy                         (const BobguiBitset        *self);
GDK_AVAILABLE_IN_ALL
BobguiBitset *             bobgui_bitset_new_range                    (guint                   start,
                                                                 guint                   n_items);

GDK_AVAILABLE_IN_ALL
void                    bobgui_bitset_remove_all                   (BobguiBitset              *self);
GDK_AVAILABLE_IN_ALL
gboolean                bobgui_bitset_add                          (BobguiBitset              *self,
                                                                 guint                   value);
GDK_AVAILABLE_IN_ALL
gboolean                bobgui_bitset_remove                       (BobguiBitset              *self,
                                                                 guint                   value);
GDK_AVAILABLE_IN_ALL
void                    bobgui_bitset_add_range                    (BobguiBitset              *self,
                                                                 guint                   start,
                                                                 guint                   n_items);
GDK_AVAILABLE_IN_ALL
void                    bobgui_bitset_remove_range                 (BobguiBitset              *self,
                                                                 guint                   start,
                                                                 guint                   n_items);
GDK_AVAILABLE_IN_ALL
void                    bobgui_bitset_add_range_closed             (BobguiBitset              *self,
                                                                 guint                   first,
                                                                 guint                   last);
GDK_AVAILABLE_IN_ALL
void                    bobgui_bitset_remove_range_closed          (BobguiBitset              *self,
                                                                 guint                   first,
                                                                 guint                   last);
GDK_AVAILABLE_IN_ALL
void                    bobgui_bitset_add_rectangle                (BobguiBitset              *self,
                                                                 guint                   start,
                                                                 guint                   width,
                                                                 guint                   height,
                                                                 guint                   stride);
GDK_AVAILABLE_IN_ALL
void                    bobgui_bitset_remove_rectangle             (BobguiBitset              *self,
                                                                 guint                   start,
                                                                 guint                   width,
                                                                 guint                   height,
                                                                 guint                   stride);

GDK_AVAILABLE_IN_ALL
void                    bobgui_bitset_union                        (BobguiBitset              *self,
                                                                 const BobguiBitset        *other);
GDK_AVAILABLE_IN_ALL
void                    bobgui_bitset_intersect                    (BobguiBitset              *self,
                                                                 const BobguiBitset        *other);
GDK_AVAILABLE_IN_ALL
void                    bobgui_bitset_subtract                     (BobguiBitset              *self,
                                                                 const BobguiBitset        *other);
GDK_AVAILABLE_IN_ALL
void                    bobgui_bitset_difference                   (BobguiBitset              *self,
                                                                 const BobguiBitset        *other);
GDK_AVAILABLE_IN_ALL
void                    bobgui_bitset_shift_left                   (BobguiBitset              *self,
                                                                 guint                   amount);
GDK_AVAILABLE_IN_ALL
void                    bobgui_bitset_shift_right                  (BobguiBitset              *self,
                                                                 guint                   amount);
GDK_AVAILABLE_IN_ALL
void                    bobgui_bitset_splice                       (BobguiBitset              *self,
                                                                 guint                   position,
                                                                 guint                   removed,
                                                                 guint                   added);

/**
 * BobguiBitsetIter:
 *
 * Iterates over the elements of a [struct@Bobgui.Bitset].
 *
 * `BobguiBitSetIter is an opaque, stack-allocated struct.
 *
 * Before a `BobguiBitsetIter` can be used, it needs to be initialized with
 * [func@Bobgui.BitsetIter.init_first], [func@Bobgui.BitsetIter.init_last]
 * or [func@Bobgui.BitsetIter.init_at].
 */
typedef struct _BobguiBitsetIter BobguiBitsetIter;

struct _BobguiBitsetIter
{
  /*< private >*/
  gpointer private_data[10];
};

GDK_AVAILABLE_IN_4_6
GType                   bobgui_bitset_iter_get_type                (void) G_GNUC_CONST;

GDK_AVAILABLE_IN_ALL
gboolean                bobgui_bitset_iter_init_first              (BobguiBitsetIter          *iter,
                                                                 const BobguiBitset        *set,
                                                                 guint                  *value);
GDK_AVAILABLE_IN_ALL
gboolean                bobgui_bitset_iter_init_last               (BobguiBitsetIter          *iter,
                                                                 const BobguiBitset        *set,
                                                                 guint                  *value);
GDK_AVAILABLE_IN_ALL
gboolean                bobgui_bitset_iter_init_at                 (BobguiBitsetIter          *iter,
                                                                 const BobguiBitset        *set,
                                                                 guint                   target,
                                                                 guint                  *value);
GDK_AVAILABLE_IN_ALL
gboolean                bobgui_bitset_iter_next                    (BobguiBitsetIter          *iter,
                                                                 guint                  *value);
GDK_AVAILABLE_IN_ALL
gboolean                bobgui_bitset_iter_previous                (BobguiBitsetIter          *iter,
                                                                 guint                  *value);
GDK_AVAILABLE_IN_ALL
guint                   bobgui_bitset_iter_get_value               (const BobguiBitsetIter    *iter);
GDK_AVAILABLE_IN_ALL
gboolean                bobgui_bitset_iter_is_valid                (const BobguiBitsetIter    *iter);

G_END_DECLS

