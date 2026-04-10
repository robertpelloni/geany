/* bobguiatspitextbuffer.c - BobguiTextBuffer-related utilities for AT-SPI
 *
 * Copyright (c) 2020 Red Hat, Inc.
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
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.Free
 */

#include "config.h"
#include "bobguiatspitextbufferprivate.h"
#include "bobguiatspipangoprivate.h"
#include "bobguitextbufferprivate.h"
#include "bobguitextviewprivate.h"
#include "bobguipangoprivate.h"

char *
bobgui_text_view_get_text_before (BobguiTextView           *view,
                               int                    offset,
                               AtspiTextBoundaryType  boundary_type,
                               int                   *start_offset,
                               int                   *end_offset)
{
  BobguiTextBuffer *buffer;
  BobguiTextIter pos, start, end;

  buffer = bobgui_text_view_get_buffer (view);
  bobgui_text_buffer_get_iter_at_offset (buffer, &pos, offset);
  start = end = pos;

  switch (boundary_type)
    {
    case ATSPI_TEXT_BOUNDARY_CHAR:
      bobgui_text_iter_backward_char (&start);
      break;

    case ATSPI_TEXT_BOUNDARY_WORD_START:
      if (!bobgui_text_iter_starts_word (&start))
        bobgui_text_iter_backward_word_start (&start);
      end = start;
      bobgui_text_iter_backward_word_start (&start);
      break;

    case ATSPI_TEXT_BOUNDARY_WORD_END:
      if (bobgui_text_iter_inside_word (&start) &&
          !bobgui_text_iter_starts_word (&start))
        bobgui_text_iter_backward_word_start (&start);
      while (!bobgui_text_iter_ends_word (&start))
        {
          if (!bobgui_text_iter_backward_char (&start))
            break;
        }
      end = start;
      bobgui_text_iter_backward_word_start (&start);
      while (!bobgui_text_iter_ends_word (&start))
        {
          if (!bobgui_text_iter_backward_char (&start))
            break;
        }
      break;

    case ATSPI_TEXT_BOUNDARY_SENTENCE_START:
      if (!bobgui_text_iter_starts_sentence (&start))
        bobgui_text_iter_backward_sentence_start (&start);
      end = start;
      bobgui_text_iter_backward_sentence_start (&start);
      break;

    case ATSPI_TEXT_BOUNDARY_SENTENCE_END:
      if (bobgui_text_iter_inside_sentence (&start) &&
          !bobgui_text_iter_starts_sentence (&start))
        bobgui_text_iter_backward_sentence_start (&start);
      while (!bobgui_text_iter_ends_sentence (&start))
        {
          if (!bobgui_text_iter_backward_char (&start))
            break;
        }
      end = start;
      bobgui_text_iter_backward_sentence_start (&start);
      while (!bobgui_text_iter_ends_sentence (&start))
        {
          if (!bobgui_text_iter_backward_char (&start))
            break;
        }
      break;

    case ATSPI_TEXT_BOUNDARY_LINE_START:
      bobgui_text_view_backward_display_line_start (view, &start);
      end = start;
      bobgui_text_view_backward_display_line (view, &start);
      bobgui_text_view_backward_display_line_start (view, &start);
      break;

    case ATSPI_TEXT_BOUNDARY_LINE_END:
      bobgui_text_view_backward_display_line_start (view, &start);
      if (!bobgui_text_iter_is_start (&start))
        {
          bobgui_text_view_backward_display_line (view, &start);
          end = start;
          bobgui_text_view_forward_display_line_end (view, &end);
          if (!bobgui_text_iter_is_start (&start))
            {
              if (bobgui_text_view_backward_display_line (view, &start))
                bobgui_text_view_forward_display_line_end (view, &start);
              else
                bobgui_text_iter_set_offset (&start, 0);
            }
        }
      else
        end = start;
      break;

    default:
      g_assert_not_reached ();
    }

  *start_offset = bobgui_text_iter_get_offset (&start);
  *end_offset = bobgui_text_iter_get_offset (&end);

  return bobgui_text_buffer_get_slice (buffer, &start, &end, FALSE);
}

char *
bobgui_text_view_get_text_at (BobguiTextView           *view,
                           int                    offset,
                           AtspiTextBoundaryType  boundary_type,
                           int                   *start_offset,
                           int                   *end_offset)
{
  BobguiTextBuffer *buffer;
  BobguiTextIter pos, start, end;

  buffer = bobgui_text_view_get_buffer (view);
  bobgui_text_buffer_get_iter_at_offset (buffer, &pos, offset);
  start = end = pos;

  switch (boundary_type)
    {
    case ATSPI_TEXT_BOUNDARY_CHAR:
      bobgui_text_iter_forward_char (&end);
      break;

    case ATSPI_TEXT_BOUNDARY_WORD_START:
      if (!bobgui_text_iter_starts_word (&start))
        bobgui_text_iter_backward_word_start (&start);
      if (bobgui_text_iter_inside_word (&end))
        bobgui_text_iter_forward_word_end (&end);
      while (!bobgui_text_iter_starts_word (&end))
        {
          if (!bobgui_text_iter_forward_char (&end))
            break;
        }
      break;

    case ATSPI_TEXT_BOUNDARY_WORD_END:
      if (bobgui_text_iter_inside_word (&start) &&
          !bobgui_text_iter_starts_word (&start))
        bobgui_text_iter_backward_word_start (&start);
      while (!bobgui_text_iter_ends_word (&start))
        {
          if (!bobgui_text_iter_backward_char (&start))
            break;
        }
      bobgui_text_iter_forward_word_end (&end);
      break;

    case ATSPI_TEXT_BOUNDARY_SENTENCE_START:
      if (!bobgui_text_iter_starts_sentence (&start))
        bobgui_text_iter_backward_sentence_start (&start);
      if (bobgui_text_iter_inside_sentence (&end))
        bobgui_text_iter_forward_sentence_end (&end);
      while (!bobgui_text_iter_starts_sentence (&end))
        {
          if (!bobgui_text_iter_forward_char (&end))
            break;
        }
      break;

    case ATSPI_TEXT_BOUNDARY_SENTENCE_END:
      if (bobgui_text_iter_inside_sentence (&start) &&
          !bobgui_text_iter_starts_sentence (&start))
        bobgui_text_iter_backward_sentence_start (&start);
      while (!bobgui_text_iter_ends_sentence (&start))
        {
          if (!bobgui_text_iter_backward_char (&start))
            break;
        }
      bobgui_text_iter_forward_sentence_end (&end);
      break;

    case ATSPI_TEXT_BOUNDARY_LINE_START:
      bobgui_text_view_backward_display_line_start (view, &start);
      bobgui_text_view_forward_display_line (view, &end);
      break;

    case ATSPI_TEXT_BOUNDARY_LINE_END:
      bobgui_text_view_backward_display_line_start (view, &start);
      if (!bobgui_text_iter_is_start (&start))
        {
          bobgui_text_view_backward_display_line (view, &start);
          bobgui_text_view_forward_display_line_end (view, &start);
        }
      bobgui_text_view_forward_display_line_end (view, &end);
      break;

    default:
      g_assert_not_reached ();
    }

  *start_offset = bobgui_text_iter_get_offset (&start);
  *end_offset = bobgui_text_iter_get_offset (&end);

  return bobgui_text_buffer_get_slice (buffer, &start, &end, FALSE);
}

char *
bobgui_text_view_get_text_after (BobguiTextView           *view,
                              int                    offset,
                              AtspiTextBoundaryType  boundary_type,
                              int                   *start_offset,
                              int                   *end_offset)
{
  BobguiTextBuffer *buffer;
  BobguiTextIter pos, start, end;

  buffer = bobgui_text_view_get_buffer (view);
  bobgui_text_buffer_get_iter_at_offset (buffer, &pos, offset);
  start = end = pos;

  switch (boundary_type)
    {
    case ATSPI_TEXT_BOUNDARY_CHAR:
      bobgui_text_iter_forward_char (&start);
      bobgui_text_iter_forward_chars (&end, 2);
      break;

    case ATSPI_TEXT_BOUNDARY_WORD_START:
      if (bobgui_text_iter_inside_word (&end))
        bobgui_text_iter_forward_word_end (&end);
      while (!bobgui_text_iter_starts_word (&end))
        {
          if (!bobgui_text_iter_forward_char (&end))
            break;
        }
      start = end;
      if (!bobgui_text_iter_is_end (&end))
        {
          bobgui_text_iter_forward_word_end (&end);
          while (!bobgui_text_iter_starts_word (&end))
            {
              if (!bobgui_text_iter_forward_char (&end))
                break;
            }
        }
      break;

    case ATSPI_TEXT_BOUNDARY_WORD_END:
      bobgui_text_iter_forward_word_end (&end);
      start = end;
      if (!bobgui_text_iter_is_end (&end))
        bobgui_text_iter_forward_word_end (&end);
      break;

    case ATSPI_TEXT_BOUNDARY_SENTENCE_START:
      if (bobgui_text_iter_inside_sentence (&end))
        bobgui_text_iter_forward_sentence_end (&end);
      while (!bobgui_text_iter_starts_sentence (&end))
        {
          if (!bobgui_text_iter_forward_char (&end))
            break;
        }
      start = end;
      if (!bobgui_text_iter_is_end (&end))
        {
          bobgui_text_iter_forward_sentence_end (&end);
          while (!bobgui_text_iter_starts_sentence (&end))
            {
              if (!bobgui_text_iter_forward_char (&end))
                break;
            }
        }
      break;

    case ATSPI_TEXT_BOUNDARY_SENTENCE_END:
      bobgui_text_iter_forward_sentence_end (&end);
      start = end;
      if (!bobgui_text_iter_is_end (&end))
        bobgui_text_iter_forward_sentence_end (&end);
      break;

    case ATSPI_TEXT_BOUNDARY_LINE_START:
      bobgui_text_view_forward_display_line (view, &end);
      start = end;
      bobgui_text_view_forward_display_line (view, &end);
      break;

    case ATSPI_TEXT_BOUNDARY_LINE_END:
      bobgui_text_view_forward_display_line_end (view, &end);
      start = end;
      bobgui_text_view_forward_display_line (view, &end);
      bobgui_text_view_forward_display_line_end (view, &end);
      break;

    default:
      g_assert_not_reached ();
    }

  *start_offset = bobgui_text_iter_get_offset (&start);
  *end_offset = bobgui_text_iter_get_offset (&end);

  return bobgui_text_buffer_get_slice (buffer, &start, &end, FALSE);
}

char *
bobgui_text_view_get_string_at (BobguiTextView           *view,
                             int                    offset,
                             AtspiTextGranularity   granularity,
                             int                   *start_offset,
                             int                   *end_offset)
{
  BobguiTextBuffer *buffer;
  BobguiTextIter pos, start, end;

  buffer = bobgui_text_view_get_buffer (view);
  bobgui_text_buffer_get_iter_at_offset (buffer, &pos, offset);
  start = end = pos;

  if (granularity == ATSPI_TEXT_GRANULARITY_CHAR)
    {
      bobgui_text_iter_forward_char (&end);
    }
  else if (granularity == ATSPI_TEXT_GRANULARITY_WORD)
    {
      if (!bobgui_text_iter_starts_word (&start))
        bobgui_text_iter_backward_word_start (&start);
      bobgui_text_iter_forward_word_end (&end);
    }
  else if (granularity == ATSPI_TEXT_GRANULARITY_SENTENCE)
    {
      if (!bobgui_text_iter_starts_sentence (&start))
        bobgui_text_iter_backward_sentence_start (&start);
      bobgui_text_iter_forward_sentence_end (&end);
    }
  else if (granularity == ATSPI_TEXT_GRANULARITY_LINE)
    {
      if (!bobgui_text_view_starts_display_line (view, &start))
        bobgui_text_view_backward_display_line (view, &start);
      bobgui_text_view_forward_display_line_end (view, &end);
    }
  else if (granularity == ATSPI_TEXT_GRANULARITY_PARAGRAPH)
    {
      bobgui_text_iter_set_line_offset (&start, 0);
      bobgui_text_iter_forward_to_line_end (&end);
    }

  *start_offset = bobgui_text_iter_get_offset (&start);
  *end_offset = bobgui_text_iter_get_offset (&end);

  return bobgui_text_buffer_get_slice (buffer, &start, &end, FALSE);
}
