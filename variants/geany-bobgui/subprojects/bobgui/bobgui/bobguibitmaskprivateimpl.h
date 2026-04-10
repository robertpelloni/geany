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


static inline BobguiBitmask *
_bobgui_bitmask_new (void)
{
  return _bobgui_bitmask_from_bits (0);
}

static inline BobguiBitmask *
_bobgui_bitmask_copy (const BobguiBitmask *mask)
{
  if (_bobgui_bitmask_is_allocated (mask))
    return _bobgui_allocated_bitmask_copy (mask);
  else
    return (BobguiBitmask *) mask;
}

static inline void
_bobgui_bitmask_free (BobguiBitmask *mask)
{
  if (_bobgui_bitmask_is_allocated (mask))
    _bobgui_allocated_bitmask_free (mask);
}

static inline char *
_bobgui_bitmask_to_string (const BobguiBitmask *mask)
{
  GString *string;
  
  string = g_string_new (NULL);
  _bobgui_allocated_bitmask_print (mask, string);
  return g_string_free (string, FALSE);
}

static inline void
_bobgui_bitmask_print (const BobguiBitmask *mask,
                    GString          *string)
{
  _bobgui_allocated_bitmask_print (mask, string);
}

static inline BobguiBitmask *
_bobgui_bitmask_intersect (BobguiBitmask       *mask,
                        const BobguiBitmask *other)
{
  return _bobgui_allocated_bitmask_intersect (mask, other);
}

static inline BobguiBitmask *
_bobgui_bitmask_union (BobguiBitmask       *mask,
                    const BobguiBitmask *other)
{
  if (_bobgui_bitmask_is_allocated (mask) ||
      _bobgui_bitmask_is_allocated (other))
    return _bobgui_allocated_bitmask_union (mask, other);
  else
    return _bobgui_bitmask_from_bits (_bobgui_bitmask_to_bits (mask)
                                   | _bobgui_bitmask_to_bits (other));
}

static inline BobguiBitmask *
_bobgui_bitmask_subtract (BobguiBitmask       *mask,
                       const BobguiBitmask *other)
{
  return _bobgui_allocated_bitmask_subtract (mask, other);
}

static inline gboolean
_bobgui_bitmask_get (const BobguiBitmask *mask,
                  guint             index_)
{
  if (_bobgui_bitmask_is_allocated (mask))
    return _bobgui_allocated_bitmask_get (mask, index_);
  else
    return index_ < BOBGUI_BITMASK_N_DIRECT_BITS
           ? !!(_bobgui_bitmask_to_bits (mask) & (((gsize) 1) << index_))
           : FALSE;
}

static inline BobguiBitmask *
_bobgui_bitmask_set (BobguiBitmask *mask,
                  guint       index_,
                  gboolean    value)
{
  if (_bobgui_bitmask_is_allocated (mask) ||
      (index_ >= BOBGUI_BITMASK_N_DIRECT_BITS && value))
    return _bobgui_allocated_bitmask_set (mask, index_, value);
  else if (index_ < BOBGUI_BITMASK_N_DIRECT_BITS)
    {
      gsize bits = _bobgui_bitmask_to_bits (mask);

      if (value)
        bits |= ((gsize) 1) << index_;
      else
        bits &= ~(((gsize) 1) << index_);

      return _bobgui_bitmask_from_bits (bits);
    }
  else
    return mask;
}

static inline BobguiBitmask *
_bobgui_bitmask_invert_range (BobguiBitmask *mask,
                           guint       start,
                           guint       end)
{
  g_assert (start <= end);

  if (_bobgui_bitmask_is_allocated (mask) ||
      (end > BOBGUI_BITMASK_N_DIRECT_BITS))
    return _bobgui_allocated_bitmask_invert_range (mask, start, end);
  else
    {
      gsize invert = (((gsize) 1) << end) - (((gsize) 1) << start);
      
      return _bobgui_bitmask_from_bits (_bobgui_bitmask_to_bits (mask) ^ invert);
    }
}

static inline gboolean
_bobgui_bitmask_is_empty (const BobguiBitmask *mask)
{
  return mask == _bobgui_bitmask_from_bits (0);
}

static inline gboolean
_bobgui_bitmask_equals (const BobguiBitmask *mask,
                     const BobguiBitmask *other)
{
  if (mask == other)
    return TRUE;

  if (!_bobgui_bitmask_is_allocated (mask) ||
      !_bobgui_bitmask_is_allocated (other))
    return FALSE;

  return _bobgui_allocated_bitmask_equals (mask, other);
}

static inline gboolean
_bobgui_bitmask_intersects (const BobguiBitmask *mask,
                         const BobguiBitmask *other)
{
  if (_bobgui_bitmask_is_allocated (mask) ||
      _bobgui_bitmask_is_allocated (other))
    return _bobgui_allocated_bitmask_intersects (mask, other);
  else
    return _bobgui_bitmask_to_bits (mask) & _bobgui_bitmask_to_bits (other) ? TRUE : FALSE;
}
