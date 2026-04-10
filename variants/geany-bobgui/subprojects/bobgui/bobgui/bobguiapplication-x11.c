/*
 * Copyright © 2010 Codethink Limited
 * Copyright © 2013 Canonical Limited
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the licence, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Ryan Lortie <desrt@desrt.ca>
 */

#include "config.h"

#include "bobguiapplicationprivate.h"
#include "bobguiapplicationwindowprivate.h"
#include "bobguinative.h"
#include "bobguiprivate.h"

#include "gdk/x11/gdksurface-x11.h"
#include <gdk/x11/gdkx.h>

typedef BobguiApplicationImplDBusClass BobguiApplicationImplX11Class;

typedef struct
{
  BobguiApplicationImplDBus dbus;
} BobguiApplicationImplX11;

G_DEFINE_TYPE (BobguiApplicationImplX11, bobgui_application_impl_x11, BOBGUI_TYPE_APPLICATION_IMPL_DBUS)

static void
bobgui_application_impl_x11_handle_window_realize (BobguiApplicationImpl *impl,
                                                BobguiWindow          *window)
{
  BobguiApplicationImplDBus *dbus = (BobguiApplicationImplDBus *) impl;
  GdkSurface *gdk_surface;
  char *window_path;

  gdk_surface = bobgui_native_get_surface (BOBGUI_NATIVE (window));

  if (!GDK_IS_X11_SURFACE (gdk_surface))
    return;

  window_path = bobgui_application_impl_dbus_get_window_path (dbus, window);

G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  gdk_x11_surface_set_utf8_property (gdk_surface, "_BOBGUI_APPLICATION_ID", dbus->application_id);
  gdk_x11_surface_set_utf8_property (gdk_surface, "_BOBGUI_UNIQUE_BUS_NAME", dbus->unique_name);
  gdk_x11_surface_set_utf8_property (gdk_surface, "_BOBGUI_APPLICATION_OBJECT_PATH", dbus->object_path);
  gdk_x11_surface_set_utf8_property (gdk_surface, "_BOBGUI_WINDOW_OBJECT_PATH", window_path);
  gdk_x11_surface_set_utf8_property (gdk_surface, "_BOBGUI_APP_MENU_OBJECT_PATH", dbus->app_menu_path);
  gdk_x11_surface_set_utf8_property (gdk_surface, "_BOBGUI_MENUBAR_OBJECT_PATH", dbus->menubar_path);
G_GNUC_END_IGNORE_DEPRECATIONS

  g_free (window_path);
}

static void
bobgui_application_impl_x11_handle_window_map (BobguiApplicationImpl *impl,
                                            BobguiWindow          *window)
{
  BobguiApplicationImplDBus *dbus = (BobguiApplicationImplDBus *) impl;
  GdkSurface *gdk_surface;
  GVariant *state;

  if (!BOBGUI_IS_APPLICATION_WINDOW (window))
    return;

  gdk_surface = bobgui_native_get_surface (BOBGUI_NATIVE (window));

  if (!GDK_IS_X11_SURFACE (gdk_surface))
    return;

  state = bobgui_application_impl_dbus_get_window_state (dbus, window);
  if (state)
    {
      gdk_x11_toplevel_restore_state (GDK_TOPLEVEL (gdk_surface), state);
      g_variant_unref (state);
    }
}

static void
bobgui_application_impl_x11_init (BobguiApplicationImplX11 *x11)
{
}

static void
bobgui_application_impl_x11_before_emit (BobguiApplicationImpl *impl,
                                      GVariant           *platform_data)
{
  const char *startup_notification_id = NULL;

  g_variant_lookup (platform_data, "desktop-startup-id", "&s", &startup_notification_id);

G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  gdk_x11_display_set_startup_notification_id (gdk_display_get_default (), startup_notification_id);
G_GNUC_END_IGNORE_DEPRECATIONS
}

static void
bobgui_application_impl_x11_collect_window_state (BobguiApplicationImpl *impl,
                                               BobguiWindow          *window,
                                               GVariantBuilder    *state)
{
  GdkSurface *surface;

  surface = bobgui_native_get_surface (BOBGUI_NATIVE (window));
  gdk_x11_toplevel_save_state (GDK_TOPLEVEL (surface), state);
}

static void
bobgui_application_impl_x11_class_init (BobguiApplicationImplX11Class *class)
{
  BobguiApplicationImplClass *impl_class = BOBGUI_APPLICATION_IMPL_CLASS (class);

  impl_class->handle_window_realize = bobgui_application_impl_x11_handle_window_realize;

  impl_class->handle_window_map = bobgui_application_impl_x11_handle_window_map;
  impl_class->before_emit = bobgui_application_impl_x11_before_emit;
  impl_class->collect_window_state = bobgui_application_impl_x11_collect_window_state;
}
