/*
 * Copyright © 2025 Red Hat, Inc
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
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
 * Authors: Matthias Clasen <mclasen@redhat.com>
 */

#include "config.h"

#include "bobguisvglocationprivate.h"

/**
 * BobguiSvgLocation:
 * @bytes: the byte index in document. If unknown, this will
 *   be zero (which is also a valid value, but only if all
 *   three values are zero)
 * @lines: the line index in the document, 0-based
 * @line_chars: the char index in the line, 0-based
 *
 * Provides information about a location in an SVG document.
 *
 * The information should be considered approximate; it is
 * meant to provide feedback for errors in an editor.
 *
 * Since: 4.22
 */

void
bobgui_svg_location_init (BobguiSvgLocation      *location,
                       GMarkupParseContext *context)
{
  int lines, chars;

  g_markup_parse_context_get_position (context, &lines, &chars);

  location->lines = lines;
  location->line_chars = chars;

#if GLIB_CHECK_VERSION (2, 88, 0)
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  location->bytes = g_markup_parse_context_get_offset (context);
G_GNUC_END_IGNORE_DEPRECATIONS
#else
  location->bytes = 0;
#endif
}

void
bobgui_svg_location_init_tag_start (BobguiSvgLocation      *location,
                                 GMarkupParseContext *context)
{
#if GLIB_CHECK_VERSION (2, 88, 0)
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  g_markup_parse_context_get_tag_start (context,
                                        &location->lines,
                                        &location->line_chars,
                                        &location->bytes);
G_GNUC_END_IGNORE_DEPRECATIONS
#else
  bobgui_svg_location_init (location, context);
#endif
}

void
bobgui_svg_location_init_tag_end (BobguiSvgLocation      *location,
                               GMarkupParseContext *context)
{
  bobgui_svg_location_init (location, context);
}

void
bobgui_svg_location_init_tag_range (BobguiSvgLocation      *start,
                                 BobguiSvgLocation      *end,
                                 GMarkupParseContext *context)
{
  bobgui_svg_location_init_tag_start (start, context);
  bobgui_svg_location_init_tag_end (end, context);
}


void
bobgui_svg_location_init_attr_range (BobguiSvgLocation      *start,
                                  BobguiSvgLocation      *end,
                                  GMarkupParseContext *context,
                                  unsigned int         attr)
{
#if GLIB_CHECK_VERSION (2, 89, 0)
  /* waiting for https://gitlab.gnome.org/GNOME/glib/-/merge_requests/5106 */
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  g_markup_parse_context_get_attribute_position (context, attr,
                                                 &start->lines, &start->line_chars, &start->bytes,
                                                 &end->lines, &end->line_chars, &end->bytes);
G_GNUC_END_IGNORE_DEPRECATIONS
#else
  bobgui_svg_location_init_tag_range (start, end, context);
#endif
}
