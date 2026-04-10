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

#include "config.h"

#include "bobguiborder.h"

/**
 * bobgui_border_new:
 *
 * Allocates a new `BobguiBorder` struct and initializes its elements to zero.
 *
 * Returns: (transfer full): a newly allocated `BobguiBorder` struct.
 *  Free with [method@Bobgui.Border.free]
 */
BobguiBorder *
bobgui_border_new (void)
{
  return g_new0 (BobguiBorder, 1);
}

/**
 * bobgui_border_copy:
 * @border_: a `BobguiBorder` struct
 *
 * Copies a `BobguiBorder`.
 *
 * Returns: (transfer full): a copy of @border_.
 */
BobguiBorder *
bobgui_border_copy (const BobguiBorder *border_)
{
  BobguiBorder *copy;

  g_return_val_if_fail (border_ != NULL, NULL);

  copy = g_new0 (BobguiBorder, 1);
  memcpy (copy, border_, sizeof (BobguiBorder));
  return copy;
}

/**
 * bobgui_border_free:
 * @border_: a `BobguiBorder` struct
 *
 * Frees a `BobguiBorder`.
 */
void
bobgui_border_free (BobguiBorder *border_)
{
  g_free (border_);
}

G_DEFINE_BOXED_TYPE (BobguiBorder, bobgui_border,
                     bobgui_border_copy,
                     bobgui_border_free)
