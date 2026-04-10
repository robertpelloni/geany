
/* This library is free software; you can redistribute it and/or
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

static void
hello (BobguiButton *button)
{
  g_print ("Hello!\n");
}

static void
quit_cb (BobguiWidget *widget,
         gpointer   data)
{
  gboolean *done = data;

  *done = TRUE;

  g_main_context_wakeup (NULL);
}

int
main (int argc, char *argv[])
{
  BobguiWidget *window, *fixed, *button;
  BobguiWidget *fixed2, *frame;
  gboolean done = FALSE;
  GskTransform *transform;

  bobgui_init ();

  window = bobgui_window_new ();
  bobgui_window_set_title (BOBGUI_WINDOW (window), "hello world");
  g_signal_connect (window, "destroy", G_CALLBACK (quit_cb), &done);

  fixed = bobgui_fixed_new ();
  bobgui_widget_set_halign (fixed, BOBGUI_ALIGN_FILL);
  bobgui_widget_set_valign (fixed, BOBGUI_ALIGN_FILL);
  bobgui_widget_set_hexpand (fixed, TRUE);
  bobgui_widget_set_vexpand (fixed, TRUE);

  button = bobgui_button_new ();
  bobgui_button_set_label (BOBGUI_BUTTON (button), "Button");
  g_signal_connect (button, "clicked", G_CALLBACK (hello), NULL);

  bobgui_fixed_put (BOBGUI_FIXED (fixed), button, 0, 0);

  transform = NULL;
  transform = gsk_transform_translate_3d (transform, &GRAPHENE_POINT3D_INIT (0, 0, 50));
  transform = gsk_transform_perspective (transform, 170);
  transform = gsk_transform_translate_3d (transform, &GRAPHENE_POINT3D_INIT (50, 0, 50));
  transform = gsk_transform_rotate (transform, 20);
  transform = gsk_transform_rotate_3d (transform, 20, graphene_vec3_y_axis ());
  bobgui_fixed_set_child_transform (BOBGUI_FIXED (fixed), button, transform);

  frame = bobgui_frame_new ("Frame");
  bobgui_widget_add_css_class (frame, "view");
  bobgui_frame_set_child (BOBGUI_FRAME (frame), fixed);

  fixed2 = bobgui_fixed_new ();

  bobgui_fixed_put (BOBGUI_FIXED (fixed2), frame, 0, 0);

  transform = NULL;
  transform = gsk_transform_translate_3d (transform, &GRAPHENE_POINT3D_INIT (0, 0, 50));
  transform = gsk_transform_perspective (transform, 170);
  transform = gsk_transform_translate_3d (transform, &GRAPHENE_POINT3D_INIT (50, 0, 50));
  transform = gsk_transform_rotate (transform, 20);
  transform = gsk_transform_rotate_3d (transform, 20, graphene_vec3_y_axis ());
  bobgui_fixed_set_child_transform (BOBGUI_FIXED (fixed2), frame, transform);

  bobgui_window_set_child (BOBGUI_WINDOW (window), fixed2);

  bobgui_window_present (BOBGUI_WINDOW (window));

  while (!done)
    g_main_context_iteration (NULL, TRUE);

  return 0;
}
