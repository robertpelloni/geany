/* testinhibitshortcuts.c

   Copyright (C) 2017 Red Hat
   Author: Olivier Fourdan <ofourdan@redhat.com>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#include <bobgui/bobgui.h>

static void
on_shortcuts_inhibit_change (GdkSurface *surface, GParamSpec *pspec, gpointer data)
{
  BobguiWidget *button = BOBGUI_WIDGET (data);
  gboolean button_active;
  gboolean shortcuts_inhibited;

  g_object_get (GDK_TOPLEVEL (surface), "shortcuts-inhibited", &shortcuts_inhibited, NULL);

  bobgui_check_button_set_inconsistent (BOBGUI_CHECK_BUTTON (button), FALSE);

  button_active = bobgui_check_button_get_active (BOBGUI_CHECK_BUTTON (button));

  if (button_active != shortcuts_inhibited)
    bobgui_check_button_set_active (BOBGUI_CHECK_BUTTON (button), shortcuts_inhibited);
}

static void
on_button_toggle (BobguiWidget *button, gpointer data)
{
  GdkSurface *surface = GDK_SURFACE (data);

  if (!bobgui_check_button_get_active (BOBGUI_CHECK_BUTTON (button)))
    {
      gdk_toplevel_restore_system_shortcuts (GDK_TOPLEVEL (surface));
      return;
    }

  bobgui_check_button_set_inconsistent (BOBGUI_CHECK_BUTTON (button), TRUE);
  gdk_toplevel_inhibit_system_shortcuts (GDK_TOPLEVEL (surface), NULL);
}

static void
quit_cb (BobguiWidget *widget,
         gpointer   user_data)
{
  gboolean *done = user_data;

  *done = TRUE;

  g_main_context_wakeup (NULL);
}

int
main (int argc, char *argv[])
{
  GdkSurface *surface;
  BobguiWidget *window;
  BobguiWidget *button;
  BobguiWidget *vbox;
  BobguiWidget *text_view;
  gboolean done = FALSE;

  bobgui_init ();

  window = bobgui_window_new ();
  g_signal_connect (window, "destroy", G_CALLBACK (quit_cb), &done);
  bobgui_widget_realize (window);
  surface = bobgui_native_get_surface (bobgui_widget_get_native (window));

  vbox = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 2);
  bobgui_window_set_child (BOBGUI_WINDOW (window), vbox);

  text_view = bobgui_text_view_new ();
  bobgui_widget_set_hexpand (text_view, TRUE);
  bobgui_widget_set_vexpand (text_view, TRUE);
  bobgui_box_append (BOBGUI_BOX (vbox), text_view);

  button = bobgui_check_button_new_with_label ("Inhibit system keyboard shortcuts");

  bobgui_box_append (BOBGUI_BOX (vbox), button);
  g_signal_connect (G_OBJECT (button), "toggled",
                    G_CALLBACK (on_button_toggle), surface);

  g_signal_connect (G_OBJECT (surface), "notify::shortcuts-inhibited",
                    G_CALLBACK (on_shortcuts_inhibit_change), button);

  bobgui_window_present (BOBGUI_WINDOW (window));

  while (!done)
    g_main_context_iteration (NULL, TRUE);

  return 0;
}
