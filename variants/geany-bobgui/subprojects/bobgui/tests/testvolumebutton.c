/* testvolumebutton.c
 * Copyright (C) 2007  Red Hat, Inc.
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

#include <bobgui/bobgui.h>

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

static void
value_changed (BobguiWidget *button,
               double volume,
               gpointer user_data)
{
  g_message ("volume changed to %f", volume);
}

static void
response_cb (BobguiDialog *dialog,
             int        arg1,
             gpointer   user_data)
{
  bobgui_window_destroy (BOBGUI_WINDOW (dialog));
}

static gboolean
show_error (gpointer data)
{
  BobguiWindow *window = (BobguiWindow *) data;
  BobguiWidget *dialog;

  g_message ("showing error");

  dialog = bobgui_message_dialog_new (window,
                                   BOBGUI_DIALOG_MODAL,
                                   BOBGUI_MESSAGE_INFO,
                                   BOBGUI_BUTTONS_CLOSE,
                                   "This should have unbroken the grab");
  g_signal_connect_object (G_OBJECT (dialog),
                           "response",
                           G_CALLBACK (response_cb), NULL, 0);
  bobgui_window_present (BOBGUI_WINDOW (dialog));

  return G_SOURCE_REMOVE;
}

int
main (int    argc,
      char **argv)
{
  BobguiWidget *window;
  BobguiWidget *button;
  BobguiWidget *button2;
  BobguiWidget *box;
  BobguiWidget *vbox;

  bobgui_init ();

  window = bobgui_window_new ();
  bobgui_window_set_default_size (BOBGUI_WINDOW (window), 400, 300);
  button = bobgui_volume_button_new ();
  button2 = bobgui_volume_button_new ();
  vbox = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 0);
  box = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 0);

  g_signal_connect (G_OBJECT (button), "value-changed",
                    G_CALLBACK (value_changed),
                    NULL);

  bobgui_window_set_child (BOBGUI_WINDOW (window), vbox);
  bobgui_box_append (BOBGUI_BOX (vbox), box);
  bobgui_box_append (BOBGUI_BOX (box), button);
  bobgui_box_append (BOBGUI_BOX (box), button2);

  bobgui_window_present (BOBGUI_WINDOW (window));
  g_timeout_add (4000, (GSourceFunc) show_error, window);

  while (TRUE)
    g_main_context_iteration (NULL, TRUE);

  return 0;
}
