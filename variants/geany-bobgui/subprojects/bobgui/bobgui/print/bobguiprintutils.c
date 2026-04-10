/* BOBGUI - The Bobgui Framework
 * bobguipapersize.c: Paper Size
 * Copyright (C) 2006, Red Hat, Inc.
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

#include "config.h"
#include "bobguiprintutilsprivate.h"

double
_bobgui_print_convert_to_mm (double len, 
			  BobguiUnit unit)
{
  switch (unit)
    {
    case BOBGUI_UNIT_MM:
      return len;
    case BOBGUI_UNIT_INCH:
      return len * MM_PER_INCH;
    case BOBGUI_UNIT_NONE:
    default:
      g_warning ("Unsupported unit");
      G_GNUC_FALLTHROUGH;
    case BOBGUI_UNIT_POINTS:
      return len * (MM_PER_INCH / POINTS_PER_INCH);
      break;
    }
}

double
_bobgui_print_convert_from_mm (double len, 
			    BobguiUnit unit)
{
  switch (unit)
    {
    case BOBGUI_UNIT_MM:
      return len;
    case BOBGUI_UNIT_INCH:
      return len / MM_PER_INCH;
    case BOBGUI_UNIT_NONE:
    default:
      g_warning ("Unsupported unit");
      G_GNUC_FALLTHROUGH;
    case BOBGUI_UNIT_POINTS:
      return len / (MM_PER_INCH / POINTS_PER_INCH);
      break;
    }
}
