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
 */

#pragma once

#include <gdk/gdk.h>
#include <bobgui/bobguienums.h>
#include <bobgui/bobguisorter.h>

typedef struct _BobguiSortKeys BobguiSortKeys;
typedef struct _BobguiSortKeysClass BobguiSortKeysClass;

struct _BobguiSortKeys
{
  const BobguiSortKeysClass *klass;
  int ref_count;

  gsize key_size;
  gsize key_align; /* must be power of 2 */
};

struct _BobguiSortKeysClass
{
  void                  (* free)                                (BobguiSortKeys            *self);

  GCompareDataFunc key_compare;

  gboolean              (* is_compatible)                       (BobguiSortKeys            *self,
                                                                 BobguiSortKeys            *other);

  void                  (* init_key)                            (BobguiSortKeys            *self,
                                                                 gpointer                item,
                                                                 gpointer                key_memory);
  void                  (* clear_key)                           (BobguiSortKeys            *self,
                                                                 gpointer                key_memory);
};

BobguiSortKeys *           bobgui_sort_keys_alloc                     (const BobguiSortKeysClass *klass,
                                                                 gsize                   size,
                                                                 gsize                   key_size,
                                                                 gsize                   key_align);
#define bobgui_sort_keys_new(_name, _klass, _key_size, _key_align) \
    ((_name *) bobgui_sort_keys_alloc ((_klass), sizeof (_name), (_key_size), (_key_align)))
BobguiSortKeys *           bobgui_sort_keys_ref                       (BobguiSortKeys            *self);
void                    bobgui_sort_keys_unref                     (BobguiSortKeys            *self);

BobguiSortKeys *           bobgui_sort_keys_new_equal                 (void);

gsize                   bobgui_sort_keys_get_key_size              (BobguiSortKeys            *self);
gsize                   bobgui_sort_keys_get_key_align             (BobguiSortKeys            *self);
GCompareDataFunc        bobgui_sort_keys_get_key_compare_func      (BobguiSortKeys            *self);
gboolean                bobgui_sort_keys_is_compatible             (BobguiSortKeys            *self,
                                                                 BobguiSortKeys            *other);
gboolean                bobgui_sort_keys_needs_clear_key           (BobguiSortKeys            *self);

#define BOBGUI_SORT_KEYS_ALIGN(_size,_align) (((_size) + (_align) - 1) & ~((_align) - 1))
static inline int
bobgui_sort_keys_compare (BobguiSortKeys *self,
                       gconstpointer a,
                       gconstpointer b)
{
  return self->klass->key_compare (a, b, self);
}
                       
static inline void
bobgui_sort_keys_init_key (BobguiSortKeys *self,
                        gpointer       item,
                        gpointer       key_memory)
{
  self->klass->init_key (self, item, key_memory);
}

static inline void
bobgui_sort_keys_clear_key (BobguiSortKeys *self,
                         gpointer       key_memory)
{
  if (self->klass->clear_key)
    self->klass->clear_key (self, key_memory);
}


