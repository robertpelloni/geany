/* testfullscreen.c
 * Copyright (C) 2013 Red Hat
 * Author: Olivier Fourdan <ofourdan@redhat.com>
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

static void
set_fullscreen_monitor_cb (BobguiWidget *widget, gpointer user_data)
{
  GdkFullscreenMode mode = GPOINTER_TO_INT (user_data);
  GdkDisplay *display;
  GdkSurface *surface;
  GdkMonitor *monitor;
  GdkToplevelLayout *layout;

  display = bobgui_widget_get_display (widget);
  surface = bobgui_native_get_surface (bobgui_widget_get_native (widget));
  if (mode == GDK_FULLSCREEN_ON_CURRENT_MONITOR)
    monitor = gdk_display_get_monitor_at_surface (display, surface);
  else
    monitor = NULL;
  layout = gdk_toplevel_layout_new ();
  gdk_toplevel_layout_set_resizable (layout, TRUE);
  gdk_toplevel_layout_set_fullscreen (layout, TRUE, monitor);
  gdk_toplevel_present (GDK_TOPLEVEL (surface), layout);
  gdk_toplevel_layout_unref (layout);
}

static void
remove_fullscreen_cb (BobguiWidget *widget, gpointer user_data)
{
  GdkSurface *surface;
  GdkToplevelLayout *layout;

  surface = bobgui_native_get_surface (bobgui_widget_get_native (widget));
  layout = gdk_toplevel_layout_new ();
  gdk_toplevel_layout_set_resizable (layout, TRUE);
  gdk_toplevel_layout_set_fullscreen (layout, FALSE, NULL);
  gdk_toplevel_present (GDK_TOPLEVEL (surface), layout);
  gdk_toplevel_layout_unref (layout);
}

int
main (int argc, char *argv[])
{
  BobguiWidget *window, *vbox, *button;

  bobgui_init ();

  window = bobgui_window_new ();

  vbox = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 5);
  bobgui_widget_set_valign (vbox, BOBGUI_ALIGN_CENTER);
  bobgui_widget_set_halign (vbox, BOBGUI_ALIGN_CENTER);
  bobgui_box_set_homogeneous (BOBGUI_BOX (vbox), TRUE);
  bobgui_window_set_child (BOBGUI_WINDOW (window), vbox);

  button = bobgui_button_new_with_label ("Fullscreen on current monitor");
  g_signal_connect (button, "clicked", G_CALLBACK (set_fullscreen_monitor_cb), GINT_TO_POINTER (GDK_FULLSCREEN_ON_CURRENT_MONITOR));
  bobgui_box_append (BOBGUI_BOX (vbox), button);

  button = bobgui_button_new_with_label ("Fullscreen on all monitors");
  g_signal_connect (button, "clicked", G_CALLBACK (set_fullscreen_monitor_cb), GINT_TO_POINTER (GDK_FULLSCREEN_ON_ALL_MONITORS));
  bobgui_box_append (BOBGUI_BOX (vbox), button);

  button = bobgui_button_new_with_label ("Un-fullscreen");
  g_signal_connect (button, "clicked", G_CALLBACK (remove_fullscreen_cb), NULL);
  bobgui_box_append (BOBGUI_BOX (vbox), button);

  bobgui_window_present (BOBGUI_WINDOW (window));

  while (TRUE)
    g_main_context_iteration (NULL, TRUE);

  return 0;
}
