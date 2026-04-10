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

#include <config.h>

#include "bobguiallocatedbitmaskprivate.h"
#include "bobguiprivate.h"


#define VALUE_TYPE gsize

#define VALUE_SIZE_BITS (sizeof (VALUE_TYPE) * 8)
#define VALUE_BIT(idx) (((VALUE_TYPE) 1) << (idx))
#define ALL_BITS (~((VALUE_TYPE) 0))

struct _BobguiBitmask {
  gsize len;
  VALUE_TYPE data[1];
};

#define ENSURE_ALLOCATED(mask, heap_mask) G_STMT_START { \
  if (!_bobgui_bitmask_is_allocated (mask)) \
    { \
      heap_mask.data[0] = _bobgui_bitmask_to_bits (mask); \
      heap_mask.len = heap_mask.data[0] ? 1 : 0; \
      mask = &heap_mask; \
    } \
} G_STMT_END

static BobguiBitmask *
bobgui_allocated_bitmask_resize (BobguiBitmask *mask,
                              gsize       size) G_GNUC_WARN_UNUSED_RESULT;
static BobguiBitmask *
bobgui_allocated_bitmask_resize (BobguiBitmask *mask,
                              gsize       size)
{
  gsize i;

  if (size == mask->len)
    return mask;

  mask = g_realloc (mask, sizeof (BobguiBitmask) + sizeof(VALUE_TYPE) * (size - 1));

  for (i = mask->len; i < size; i++)
    mask->data[i] = 0;

  mask->len = size;

  return mask;
}

static BobguiBitmask *
bobgui_allocated_bitmask_new_for_bits (gsize bits)
{
  BobguiBitmask *mask;
  
  mask = g_malloc (sizeof (BobguiBitmask));
  mask->len = bits ? 1 : 0;
  mask->data[0] = bits;

  return mask;
}

static BobguiBitmask *
bobgui_bitmask_ensure_allocated (BobguiBitmask *mask)
{
  if (_bobgui_bitmask_is_allocated (mask))
    return mask;
  else
    return bobgui_allocated_bitmask_new_for_bits (_bobgui_bitmask_to_bits (mask));
}

BobguiBitmask *
_bobgui_allocated_bitmask_copy (const BobguiBitmask *mask)
{
  BobguiBitmask *copy;

  bobgui_internal_return_val_if_fail (mask != NULL, NULL);

  copy = bobgui_allocated_bitmask_new_for_bits (0);
  
  return _bobgui_allocated_bitmask_union (copy, mask);
}

void
_bobgui_allocated_bitmask_free (BobguiBitmask *mask)
{
  bobgui_internal_return_if_fail (mask != NULL);

  g_free (mask);
}

char *
_bobgui_allocated_bitmask_to_string (const BobguiBitmask *mask)
{
  GString *str = g_string_new (NULL);

  _bobgui_allocated_bitmask_print (mask, str);

  return g_string_free (str, FALSE);
}

void
_bobgui_allocated_bitmask_print (const BobguiBitmask *mask,
                              GString          *string)
{
  BobguiBitmask mask_allocated;
  int i;

  bobgui_internal_return_if_fail (mask != NULL);
  bobgui_internal_return_if_fail (string != NULL);

  ENSURE_ALLOCATED (mask, mask_allocated);

  for (i = mask->len * VALUE_SIZE_BITS - 1; i >= 0; i--)
    {
      if (_bobgui_allocated_bitmask_get (mask, i))
        break;
    }

  if (i < 0)
    {
      g_string_append_c (string, '0');
      return;
    }

  for (; i >= 0; i--)
    {
      g_string_append_c (string, _bobgui_allocated_bitmask_get (mask, i) ? '1' : '0');
    }
}

/* NB: Call this function whenever the
 * array might have become too large.
 * _bobgui_allocated_bitmask_is_empty() depends on this.
 */
static BobguiBitmask *
bobgui_allocated_bitmask_shrink (BobguiBitmask *mask) G_GNUC_WARN_UNUSED_RESULT;
static BobguiBitmask *
bobgui_allocated_bitmask_shrink (BobguiBitmask *mask)
{
  guint i;

  for (i = mask->len; i; i--)
    {
      if (mask->data[i - 1])
        break;
    }

  if (i == 0 ||
      (i == 1 && mask->data[0] < VALUE_BIT (BOBGUI_BITMASK_N_DIRECT_BITS)))
    {
      BobguiBitmask *result = _bobgui_bitmask_from_bits (i == 0 ? 0 : mask->data[0]);
      _bobgui_allocated_bitmask_free (mask);
      return result;
    }

  return bobgui_allocated_bitmask_resize (mask, i);
}

BobguiBitmask *
_bobgui_allocated_bitmask_intersect (BobguiBitmask       *mask,
                                  const BobguiBitmask *other)
{
  BobguiBitmask other_allocated;
  guint i;

  bobgui_internal_return_val_if_fail (mask != NULL, NULL);
  bobgui_internal_return_val_if_fail (other != NULL, NULL);

  mask = bobgui_bitmask_ensure_allocated (mask);
  ENSURE_ALLOCATED (other, other_allocated);

  for (i = 0; i < MIN (mask->len, other->len); i++)
    {
      mask->data[i] &= other->data[i];
    }
  for (; i < mask->len; i++)
    {
      mask->data[i] = 0;
    }

  return bobgui_allocated_bitmask_shrink (mask);
}

BobguiBitmask *
_bobgui_allocated_bitmask_union (BobguiBitmask       *mask,
                              const BobguiBitmask *other)
{
  BobguiBitmask other_allocated;
  guint i;

  bobgui_internal_return_val_if_fail (mask != NULL, NULL);
  bobgui_internal_return_val_if_fail (other != NULL, NULL);

  mask = bobgui_bitmask_ensure_allocated (mask);
  ENSURE_ALLOCATED (other, other_allocated);

  mask = bobgui_allocated_bitmask_resize (mask, MAX (mask->len, other->len));
  for (i = 0; i < other->len; i++)
    {
      mask->data[i] |= other->data[i];
    }

  return mask;
}

BobguiBitmask *
_bobgui_allocated_bitmask_subtract (BobguiBitmask       *mask,
                                 const BobguiBitmask *other)
{
  BobguiBitmask other_allocated;
  guint i, len;

  bobgui_internal_return_val_if_fail (mask != NULL, NULL);
  bobgui_internal_return_val_if_fail (other != NULL, NULL);

  mask = bobgui_bitmask_ensure_allocated (mask);
  ENSURE_ALLOCATED (other, other_allocated);

  len = MIN (mask->len, other->len);
  for (i = 0; i < len; i++)
    {
      mask->data[i] &= ~other->data[i];
    }

  return bobgui_allocated_bitmask_shrink (mask);
}

static inline void
bobgui_allocated_bitmask_indexes (guint index_,
                               guint *array_index,
                               guint *bit_index)
{
  *array_index = index_ / VALUE_SIZE_BITS;
  *bit_index = index_ % VALUE_SIZE_BITS;
}

gboolean
_bobgui_allocated_bitmask_get (const BobguiBitmask *mask,
                            guint             index_)
{
  guint array_index, bit_index;

  bobgui_internal_return_val_if_fail (mask != NULL, FALSE);

  bobgui_allocated_bitmask_indexes (index_, &array_index, &bit_index);

  if (array_index >= mask->len)
    return FALSE;

  return (mask->data[array_index] & VALUE_BIT (bit_index)) ? TRUE : FALSE;
}

BobguiBitmask *
_bobgui_allocated_bitmask_set (BobguiBitmask *mask,
                            guint       index_,
                            gboolean    value)
{
  guint array_index, bit_index;

  bobgui_internal_return_val_if_fail (mask != NULL, NULL);

  mask = bobgui_bitmask_ensure_allocated (mask);
  bobgui_allocated_bitmask_indexes (index_, &array_index, &bit_index);

  if (value)
    {
      if (array_index >= mask->len)
        mask = bobgui_allocated_bitmask_resize (mask, array_index + 1);
      
      mask->data[array_index] |= VALUE_BIT (bit_index);
    }
  else
    {
      if (array_index < mask->len)
        {
          mask->data[array_index] &= ~ VALUE_BIT (bit_index);
          mask = bobgui_allocated_bitmask_shrink (mask);
        }
    }

  return mask;
}

BobguiBitmask *
_bobgui_allocated_bitmask_invert_range (BobguiBitmask *mask,
                                     guint       start,
                                     guint       end)
{
  guint i;
  guint start_word, start_bit;
  guint end_word, end_bit;

  bobgui_internal_return_val_if_fail (mask != NULL, NULL);
  bobgui_internal_return_val_if_fail (start < end, NULL);

  mask = bobgui_bitmask_ensure_allocated (mask);

  bobgui_allocated_bitmask_indexes (start, &start_word, &start_bit);
  bobgui_allocated_bitmask_indexes (end - 1, &end_word, &end_bit);

  if (end_word >= mask->len)
    mask = bobgui_allocated_bitmask_resize (mask, end_word + 1);

  for (i = start_word; i <= end_word; i++)
    mask->data[i] ^= ALL_BITS;
  mask->data[start_word] ^= (((VALUE_TYPE) 1) << start_bit) - 1;
  if (end_bit != VALUE_SIZE_BITS - 1)
    mask->data[end_word] ^= ALL_BITS << (end_bit + 1);

  return bobgui_allocated_bitmask_shrink (mask);
}

gboolean
_bobgui_allocated_bitmask_equals (const BobguiBitmask  *mask,
                               const BobguiBitmask  *other)
{
  guint i;

  bobgui_internal_return_val_if_fail (mask != NULL, FALSE);
  bobgui_internal_return_val_if_fail (other != NULL, FALSE);

  if (mask->len != other->len)
    return FALSE;

  for (i = 0; i < mask->len; i++)
    {
      if (mask->data[i] != other->data[i])
        return FALSE;
    }

  return TRUE;
}

gboolean
_bobgui_allocated_bitmask_intersects (const BobguiBitmask *mask,
                                   const BobguiBitmask *other)
{
  BobguiBitmask mask_allocated, other_allocated;
  int i;

  bobgui_internal_return_val_if_fail (mask != NULL, FALSE);
  bobgui_internal_return_val_if_fail (other != NULL, FALSE);

  ENSURE_ALLOCATED (mask, mask_allocated);
  ENSURE_ALLOCATED (other, other_allocated);

  for (i = MIN (mask->len, other->len) - 1; i >= 0; i--)
    {
      if (mask->data[i] & other->data[i])
        return TRUE;
    }

  return FALSE;
}

