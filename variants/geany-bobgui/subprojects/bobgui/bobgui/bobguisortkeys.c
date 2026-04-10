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

#include "config.h"

#include "bobguisortkeysprivate.h"

#include "bobguicssstyleprivate.h"
#include "bobguistyleproviderprivate.h"

BobguiSortKeys *
bobgui_sort_keys_alloc (const BobguiSortKeysClass *klass,
                     gsize                   size,
                     gsize                   key_size,
                     gsize                   key_align)
{
  BobguiSortKeys *self;

  g_return_val_if_fail (key_align > 0, NULL);

  self = g_malloc0 (size);

  self->klass = klass;
  self->ref_count = 1;

  self->key_size = key_size;
  self->key_align = key_align;

  return self;
}

BobguiSortKeys *
bobgui_sort_keys_ref (BobguiSortKeys *self)
{
  self->ref_count += 1;

  return self;
}

void
bobgui_sort_keys_unref (BobguiSortKeys *self)
{
  self->ref_count -= 1;
  if (self->ref_count > 0)
    return;

  self->klass->free (self);
}

gsize
bobgui_sort_keys_get_key_size (BobguiSortKeys *self)
{
  return self->key_size;
}

gsize
bobgui_sort_keys_get_key_align (BobguiSortKeys *self)
{
  return self->key_align;
}

GCompareDataFunc
bobgui_sort_keys_get_key_compare_func (BobguiSortKeys *self)
{
  return self->klass->key_compare;
}

gboolean
bobgui_sort_keys_is_compatible (BobguiSortKeys *self,
                             BobguiSortKeys *other)
{
  if (self == other)
    return TRUE;

  return self->klass->is_compatible (self, other);
}

gboolean
bobgui_sort_keys_needs_clear_key (BobguiSortKeys *self)
{
  return self->klass->clear_key != NULL;
}

static void
bobgui_equal_sort_keys_free (BobguiSortKeys *keys)
{
  g_free (keys);
}

static int
bobgui_equal_sort_keys_compare (gconstpointer a,
                             gconstpointer b,
                             gpointer      unused)
{
  return BOBGUI_ORDERING_EQUAL;
}

static gboolean
bobgui_equal_sort_keys_is_compatible (BobguiSortKeys *keys,
                                   BobguiSortKeys *other)
{
  return keys->klass == other->klass;
}

static void
bobgui_equal_sort_keys_init_key (BobguiSortKeys *keys,
                               gpointer     item,
                               gpointer     key_memory)
{
}

static const BobguiSortKeysClass BOBGUI_EQUAL_SORT_KEYS_CLASS =
{
  bobgui_equal_sort_keys_free,
  bobgui_equal_sort_keys_compare,
  bobgui_equal_sort_keys_is_compatible,
  bobgui_equal_sort_keys_init_key,
  NULL
};

/*<private>
 * bobgui_sort_keys_new_equal:
 *
 * Creates a new BobguiSortKeys that compares every element as equal.
 * This is useful when sorters are in an invalid configuration.
 *
 * Returns: a new BobguiSortKeys
 **/
BobguiSortKeys *
bobgui_sort_keys_new_equal (void)
{
  return bobgui_sort_keys_new (BobguiSortKeys,
                            &BOBGUI_EQUAL_SORT_KEYS_CLASS,
                            0, 1);
}

