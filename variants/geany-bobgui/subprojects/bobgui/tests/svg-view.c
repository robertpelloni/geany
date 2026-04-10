/* svg-view.c
 * Copyright (C) 2025  Red Hat, Inc
 * Author: Matthias Clasen
 *
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


/* Show an SVG animation using the SVG renderer.
 * Left/right click change states.
 */


static void
clicked (BobguiGestureClick *click,
         int              n_press,
         double           x,
         double           y,
         BobguiSvg          *svg)
{
  unsigned int state;

  state = bobgui_svg_get_state (svg);

  if (bobgui_gesture_single_get_current_button (BOBGUI_GESTURE_SINGLE (click)) == 1)
    {
      if (state == 63)
        state = 0;
      else
        state++;
    }
  else
    {
      if (state == 0)
        state = 63;
      else
        state--;
    }

  g_print ("state now %u\n", state);
  bobgui_svg_set_state (svg, state);
}

static void
error_cb (BobguiSvg *svg, GError *error)
{
  if (error->domain == BOBGUI_SVG_ERROR)
    {
      const BobguiSvgLocation *start = bobgui_svg_error_get_start (error);
      const BobguiSvgLocation *end = bobgui_svg_error_get_end (error);
      const char *element = bobgui_svg_error_get_element (error);
      const char *attribute = bobgui_svg_error_get_attribute (error);

      if (start)
        {
          if (end->lines != start->lines || end->line_chars != start->line_chars)
            g_print ("%" G_GSIZE_FORMAT ".%" G_GSIZE_FORMAT " - %" G_GSIZE_FORMAT ".%" G_GSIZE_FORMAT ": ",
                     start->lines, start->line_chars,
                     end->lines, end->line_chars);
          else
            g_print ("%" G_GSIZE_FORMAT ".%" G_GSIZE_FORMAT ": ", start->lines, start->line_chars);
        }

      if (element && attribute)
        g_print ("(%s / %s) ", element, attribute);
      else if (element)
        g_print ("(%s) ", element);
    }

  g_print ("%s\n", error->message);
}

int
main (int argc, char *argv[])
{
  BobguiWidget *window, *picture;
  char *contents;
  size_t length;
  GBytes *bytes;
  GError *error = NULL;
  BobguiSvg *svg;
  BobguiEventController *click;

  if (argc < 2)
    {
      g_print ("No svg file given.\n");
      return 0;
    }

  bobgui_init ();

  window = bobgui_window_new ();

  if (!g_file_get_contents (argv[1], &contents, &length, &error))
    g_error ("%s", error->message);

  bytes = g_bytes_new_take (contents, length);

  svg = bobgui_svg_new ();
  g_signal_connect (svg, "error", G_CALLBACK (error_cb), NULL);
  bobgui_svg_load_from_bytes (svg, bytes);

  bobgui_svg_play (svg);

  picture = bobgui_picture_new_for_paintable (GDK_PAINTABLE (svg));
  bobgui_window_set_child (BOBGUI_WINDOW (window), picture);

  click = BOBGUI_EVENT_CONTROLLER (bobgui_gesture_click_new ());
  bobgui_gesture_single_set_button (BOBGUI_GESTURE_SINGLE (click), 0);
  g_signal_connect (click, "pressed", G_CALLBACK (clicked), svg);
  bobgui_widget_add_controller (picture, click);

  bobgui_window_present (BOBGUI_WINDOW (window));

  while (g_list_model_get_n_items (bobgui_window_get_toplevels ()))
    g_main_context_iteration (NULL, TRUE);

  return 0;
}
