
/*
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <bobgui/bobgui.h>


G_MODULE_EXPORT void
apply_tags_blue (BobguiTextView *text_view)
{
  BobguiTextBuffer *buffer;
  BobguiTextIter start, end;
  BobguiTextIter four, eight;

  buffer = bobgui_text_view_get_buffer (text_view);
  bobgui_text_buffer_get_bounds (buffer, &start, &end);
  bobgui_text_buffer_apply_tag_by_name (buffer, "blue", &start, &end);
  four = start;
  eight = start;
  bobgui_text_iter_forward_chars (&four, 4);
  bobgui_text_iter_forward_chars (&eight, 8);
  bobgui_text_buffer_apply_tag_by_name (buffer, "black", &four, &end);
  bobgui_text_buffer_apply_tag_by_name (buffer, "white", &eight, &end);
}

G_MODULE_EXPORT void
apply_tags_red_blue (BobguiTextView *text_view)
{
  BobguiTextBuffer *buffer;
  BobguiTextIter start, end;
  BobguiTextIter four, eight;

  buffer = bobgui_text_view_get_buffer (text_view);
  bobgui_text_buffer_get_bounds (buffer, &start, &end);
  bobgui_text_buffer_apply_tag_by_name (buffer, "red", &start, &end);
  bobgui_text_buffer_apply_tag_by_name (buffer, "blue", &start, &end);
  four = start;
  eight = start;
  bobgui_text_iter_forward_chars (&four, 4);
  bobgui_text_iter_forward_chars (&eight, 8);
  bobgui_text_buffer_apply_tag_by_name (buffer, "black", &four, &eight);
  bobgui_text_buffer_apply_tag_by_name (buffer, "white", &eight, &end);
}
