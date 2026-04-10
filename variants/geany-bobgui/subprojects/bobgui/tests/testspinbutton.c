/* testspinbutton.c
 * Copyright (C) 2004 Morten Welinder
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

static int num_windows = 0;

static gboolean done = FALSE;

static gboolean
on_delete (BobguiWindow *w)
{
  num_windows--;
  if (num_windows == 0)
    {
      done = TRUE;
      g_main_context_wakeup (NULL);
    }

  return FALSE;
}

static void
prepare_window_for_orientation (BobguiOrientation orientation)
{
  BobguiWidget *window, *mainbox, *wrap_button;
  int max;

  window = bobgui_window_new ();
  g_signal_connect (window, "close-request", G_CALLBACK (on_delete), NULL);

  mainbox = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL ^ orientation, 2);
  bobgui_window_set_child (BOBGUI_WINDOW (window), mainbox);

  wrap_button = bobgui_toggle_button_new_with_label ("Wrap");
  bobgui_box_append (BOBGUI_BOX (mainbox), wrap_button);

  for (max = 9; max <= 999999999; max = max * 10 + 9)
    {
      BobguiAdjustment *adj = bobgui_adjustment_new (max,
                                               1, max,
                                               1,
                                               (max + 1) / 10,
                                               0.0);

      BobguiWidget *spin = bobgui_spin_button_new (adj, 1.0, 0);
      bobgui_orientable_set_orientation (BOBGUI_ORIENTABLE (spin), orientation);
      bobgui_widget_set_halign (BOBGUI_WIDGET (spin), BOBGUI_ALIGN_CENTER);

      g_object_bind_property (wrap_button, "active", spin, "wrap", G_BINDING_SYNC_CREATE);

      BobguiWidget *hbox = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 4);
      bobgui_box_append (BOBGUI_BOX (hbox), spin);
      bobgui_box_append (BOBGUI_BOX (mainbox), hbox);
    }

  bobgui_window_present (BOBGUI_WINDOW (window));
  num_windows++;
}

int
main (int argc, char **argv)
{
  bobgui_init ();

  prepare_window_for_orientation (BOBGUI_ORIENTATION_HORIZONTAL);
  prepare_window_for_orientation (BOBGUI_ORIENTATION_VERTICAL);

  while (!done)
    g_main_context_iteration (NULL, TRUE);

  return 0;
}
