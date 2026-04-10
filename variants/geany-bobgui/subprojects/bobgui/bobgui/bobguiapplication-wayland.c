/*
 * Copyright © 2010 Codethink Limited
 * Copyright © 2013 Canonical Limited
 * Copyright © 2020 Emmanuel Gil Peyrot
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

#include <gdk/wayland/gdkwayland.h>
#include <gdk/wayland/gdktoplevel-wayland-private.h>
#include <gdk/wayland/gdkdisplay-wayland.h>

#include "a11y/bobguiatspicontextprivate.h"
#include "bobguiatcontextprivate.h"

typedef struct
{
  BobguiApplicationImplDBusClass parent_class;
} BobguiApplicationImplWaylandClass;

typedef struct
{
  guint cookie;
  guint dbus_cookie;
  BobguiApplicationInhibitFlags flags;
  GdkSurface *surface;

} BobguiApplicationWaylandInhibitor;

static void
bobgui_application_wayland_inhibitor_free (BobguiApplicationWaylandInhibitor *inhibitor)
{
  g_free (inhibitor);
}

typedef struct
{
  BobguiApplicationImplDBus dbus;
  GSList *inhibitors;
  guint next_cookie;
  gboolean session_mgmt_supported;
} BobguiApplicationImplWayland;

G_DEFINE_TYPE (BobguiApplicationImplWayland, bobgui_application_impl_wayland, BOBGUI_TYPE_APPLICATION_IMPL_DBUS)

static void
restore_wayland_fallback_state (BobguiWindow *window,
                                GVariant  *state)
{
  int width, height;
  gboolean is_maximized = FALSE;
  gboolean is_fullscreen = FALSE;

  BOBGUI_DEBUG (SESSION, "No Wayland session management state found! Restoring fallback state");

  if (g_variant_lookup (state, "size", "(ii)", &width, &height))
    bobgui_window_set_default_size (window, width, height);

  g_variant_lookup (state, "is-maximized", "b", &is_maximized);
  g_variant_lookup (state, "is-fullscreen", "b", &is_fullscreen);

  g_object_set (window,
                "maximized", is_maximized,
                "fullscreened", is_fullscreen,
                NULL);
}

static void
set_a11y_properties (BobguiWindow *window)
{
  GdkSurface *gdk_surface;
  BobguiATContext *at_context;

  gdk_surface = bobgui_native_get_surface (BOBGUI_NATIVE (window));

  at_context = bobgui_accessible_get_at_context (BOBGUI_ACCESSIBLE (window));

  if (BOBGUI_IS_AT_SPI_CONTEXT (at_context))
    {
      const char *dbus_name = NULL, *object_path = NULL;
      GVariant *ref_data;

      bobgui_at_context_realize (at_context);

      ref_data = bobgui_at_spi_context_to_ref (BOBGUI_AT_SPI_CONTEXT (at_context));

      g_variant_get (ref_data, "(&s&o)", &dbus_name, &object_path);

      gdk_wayland_toplevel_set_a11y_properties (GDK_TOPLEVEL (gdk_surface),
                                                dbus_name, object_path);
      g_variant_unref (ref_data);
    }

  g_clear_object (&at_context);
}

static void
bobgui_application_impl_wayland_handle_window_realize (BobguiApplicationImpl *impl,
                                                    BobguiWindow          *window)
{
  BobguiApplicationImplClass *impl_class =
    BOBGUI_APPLICATION_IMPL_CLASS (bobgui_application_impl_wayland_parent_class);
  BobguiApplicationImplDBus *dbus = (BobguiApplicationImplDBus *) impl;
  GdkSurface *gdk_surface;
  char *window_path;
  GVariant *state;
  char *id = NULL;
  BobguiATContext *at_context = NULL;

  BOBGUI_DEBUG (SESSION, "Handle window realize");

  gdk_surface = bobgui_native_get_surface (BOBGUI_NATIVE (window));

  if (!GDK_IS_WAYLAND_TOPLEVEL (gdk_surface))
    return;

  window_path = bobgui_application_impl_dbus_get_window_path (dbus, window);

  gdk_wayland_toplevel_set_dbus_properties (GDK_TOPLEVEL (gdk_surface),
                                            dbus->application_id,
                                            dbus->app_menu_path,
                                            dbus->menubar_path,
                                            window_path,
                                            dbus->object_path,
                                            dbus->unique_name);

  g_free (window_path);

  state = bobgui_application_impl_dbus_get_window_state (dbus, window);
  if (state)
    {
      if (g_variant_lookup (state, "session-id", "s", &id))
        BOBGUI_DEBUG (SESSION, "Found saved session ID %s", id);
      else
        restore_wayland_fallback_state (window, state);
    }

  if (!id)
    {
      id = g_uuid_string_random ();
      BOBGUI_DEBUG (SESSION, "No saved session ID, using %s", id);
    }

  BOBGUI_DEBUG (SESSION, "Set Wayland toplevel session ID: %s", id);
  gdk_wayland_toplevel_set_session_id (GDK_TOPLEVEL (gdk_surface), id);
  gdk_wayland_toplevel_restore_from_session (GDK_TOPLEVEL (gdk_surface));
  g_free (id);

  at_context = bobgui_accessible_get_at_context (BOBGUI_ACCESSIBLE (window));
  if (at_context)
    {
      if (bobgui_at_context_is_realized (at_context))
        set_a11y_properties (window);
      else
        g_signal_connect_object (at_context, "notify::realized", (GCallback) set_a11y_properties, window, G_CONNECT_SWAPPED);
      g_clear_object (&at_context);
    }

  impl_class->handle_window_realize (impl, window);
}

static void
bobgui_application_impl_wayland_before_emit (BobguiApplicationImpl *impl,
                                          GVariant           *platform_data)
{
  const char *startup_notification_id = NULL;

  g_variant_lookup (platform_data, "activation-token", "&s", &startup_notification_id);
  if (!startup_notification_id)
    g_variant_lookup (platform_data, "desktop-startup-id", "&s", &startup_notification_id);

G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  gdk_wayland_display_set_startup_notification_id (gdk_display_get_default (), startup_notification_id);
G_GNUC_END_IGNORE_DEPRECATIONS
}

static void
bobgui_application_impl_wayland_window_removed (BobguiApplicationImpl *impl,
                                             BobguiWindow          *window)
{
  BobguiApplicationImplWayland *wayland = (BobguiApplicationImplWayland *) impl;
  GSList *iter = wayland->inhibitors;

  BOBGUI_APPLICATION_IMPL_CLASS (bobgui_application_impl_wayland_parent_class)->window_removed (impl, window);

  while (iter)
    {
      BobguiApplicationWaylandInhibitor *inhibitor = iter->data;
      GSList *next = iter->next;

      if (inhibitor->surface && inhibitor->surface == bobgui_native_get_surface (BOBGUI_NATIVE (window)))
        {
          inhibitor->surface = NULL;

          if (!inhibitor->dbus_cookie)
            {
              bobgui_application_wayland_inhibitor_free (inhibitor);
              wayland->inhibitors = g_slist_delete_link (wayland->inhibitors, iter);
            }
        }

      iter = next;
    }
}

static void
bobgui_application_impl_wayland_window_forget (BobguiApplicationImpl *impl,
                                            BobguiWindow          *window)
{
  GdkSurface *surface;

  BOBGUI_APPLICATION_IMPL_CLASS (bobgui_application_impl_wayland_parent_class)->window_forget (impl, window);

  surface = bobgui_native_get_surface (BOBGUI_NATIVE (window));
  if (GDK_IS_WAYLAND_TOPLEVEL (surface))
    gdk_wayland_toplevel_remove_from_session (GDK_TOPLEVEL (surface));
}

static guint
bobgui_application_impl_wayland_inhibit (BobguiApplicationImpl         *impl,
                                      BobguiWindow                  *window,
                                      BobguiApplicationInhibitFlags  flags,
                                      const char                 *reason)
{
  BobguiApplicationImplWayland *wayland = (BobguiApplicationImplWayland *) impl;
  GdkSurface *surface;
  BobguiApplicationWaylandInhibitor *inhibitor;
  gboolean success;

  if (!flags)
    return 0;

  inhibitor = g_new0 (BobguiApplicationWaylandInhibitor, 1);
  inhibitor->cookie = ++wayland->next_cookie;
  inhibitor->flags = flags;
  wayland->inhibitors = g_slist_prepend (wayland->inhibitors, inhibitor);

  if (flags & BOBGUI_APPLICATION_INHIBIT_IDLE && window && impl->application == bobgui_window_get_application (window))
    {
      surface = bobgui_native_get_surface (BOBGUI_NATIVE (window));
      if (GDK_IS_WAYLAND_TOPLEVEL (surface))
        {
          success = gdk_wayland_toplevel_inhibit_idle (GDK_TOPLEVEL (surface));
          if (success)
            {
              flags &= ~BOBGUI_APPLICATION_INHIBIT_IDLE;
              inhibitor->surface = surface;
            }
        }
    }

  if (flags)
    inhibitor->dbus_cookie = BOBGUI_APPLICATION_IMPL_CLASS (bobgui_application_impl_wayland_parent_class)->inhibit (impl, window, flags, reason);

  return inhibitor->cookie;
}

static void
bobgui_application_impl_wayland_uninhibit (BobguiApplicationImpl *impl,
                                        guint               cookie)
{
  BobguiApplicationImplWayland *wayland = (BobguiApplicationImplWayland *) impl;
  GSList *iter;

  for (iter = wayland->inhibitors; iter; iter = iter->next)
    {
      BobguiApplicationWaylandInhibitor *inhibitor = iter->data;

      if (inhibitor->cookie == cookie)
        {
          if (inhibitor->dbus_cookie)
            BOBGUI_APPLICATION_IMPL_CLASS (bobgui_application_impl_wayland_parent_class)->uninhibit (impl, inhibitor->dbus_cookie);
          if (inhibitor->surface)
            gdk_wayland_toplevel_uninhibit_idle (GDK_TOPLEVEL (inhibitor->surface));
          bobgui_application_wayland_inhibitor_free (inhibitor);
          wayland->inhibitors = g_slist_delete_link (wayland->inhibitors, iter);
          return;
        }
    }

  g_warning ("Invalid inhibitor cookie");
}

static void
bobgui_application_impl_wayland_startup (BobguiApplicationImpl *impl,
                                      gboolean            support_save)
{
  BobguiApplicationImplWayland *wayland = (BobguiApplicationImplWayland *) impl;
  GdkDisplay *display = gdk_display_get_default ();
  enum xx_session_manager_v1_reason wl_reason;
  char *id = NULL;
  GVariant *state;

  BOBGUI_APPLICATION_IMPL_CLASS (bobgui_application_impl_wayland_parent_class)->startup (impl, support_save);

  if (!support_save)
    return;

  state = bobgui_application_impl_retrieve_state (impl);
  if (state)
    {
      GVariant *global = g_variant_get_child_value (state, 0);
      g_variant_lookup (global, "wayland-session", "s", &id);
      g_clear_pointer (&global, g_variant_unref);
      g_clear_pointer (&state, g_variant_unref);
    }

  switch (wayland->dbus.reason)
    {
    case BOBGUI_RESTORE_REASON_LAUNCH:
      wl_reason = XX_SESSION_MANAGER_V1_REASON_LAUNCH;
      break;

    case BOBGUI_RESTORE_REASON_RESTORE:
      wl_reason = XX_SESSION_MANAGER_V1_REASON_SESSION_RESTORE;
      break;

    case BOBGUI_RESTORE_REASON_RECOVER:
      wl_reason = XX_SESSION_MANAGER_V1_REASON_RECOVER;
      break;

    case BOBGUI_RESTORE_REASON_PRISTINE:
      wl_reason = XX_SESSION_MANAGER_V1_REASON_LAUNCH;
      g_clear_pointer (&id, g_free);
      break;
    default:
      g_assert_not_reached ();
    }

  BOBGUI_DEBUG (SESSION, "Wayland register session ID %s", id);
  wayland->session_mgmt_supported = gdk_wayland_display_register_session (display, wl_reason, id);
  g_free (id);
}

static void
collect_wayland_fallback_state (BobguiWindow       *window,
                                GVariantBuilder *state)
{
  int width, height;
  gboolean is_maximized;
  gboolean is_fullscreen;

  BOBGUI_DEBUG (SESSION, "Wayland compositor doesn't appear to support session management! Collecting fallback state");

  bobgui_window_get_default_size (window, &width, &height);
  g_variant_builder_add (state, "{sv}", "size", g_variant_new ("(ii)", width, height));

  is_maximized = bobgui_window_is_maximized (window);
  g_variant_builder_add (state, "{sv}", "is-maximized", g_variant_new_boolean (is_maximized));

  is_fullscreen = bobgui_window_is_fullscreen (window);
  g_variant_builder_add (state, "{sv}", "is-fullscreen", g_variant_new_boolean (is_fullscreen));
}

static void
bobgui_application_impl_wayland_collect_window_state (BobguiApplicationImpl *impl,
                                                   BobguiWindow          *window,
                                                   GVariantBuilder    *state)
{
  BobguiApplicationImplWayland *wayland = (BobguiApplicationImplWayland*) impl;
  GdkSurface *surface;
  const char *session_id;

  if (!wayland->session_mgmt_supported)
    {
      collect_wayland_fallback_state (window, state);
      return;
    }

  surface = bobgui_native_get_surface (BOBGUI_NATIVE (window));

  session_id = gdk_wayland_toplevel_get_session_id (GDK_TOPLEVEL (surface));
  if (session_id)
    g_variant_builder_add (state, "{sv}", "session-id", g_variant_new_string (session_id));
}

static void
bobgui_application_impl_wayland_collect_global_state (BobguiApplicationImpl *impl,
                                                   GVariantBuilder    *state)
{
  GdkDisplay *display = gdk_display_get_default ();
  const char *id;

  BOBGUI_APPLICATION_IMPL_CLASS (bobgui_application_impl_wayland_parent_class)->collect_global_state (impl, state);

  id = gdk_wayland_display_get_session_id (display);

  if (id)
    g_variant_builder_add (state, "{sv}", "wayland-session", g_variant_new_string (id));
}

static void
bobgui_application_impl_wayland_init (BobguiApplicationImplWayland *wayland)
{
}

static void
bobgui_application_impl_wayland_class_init (BobguiApplicationImplWaylandClass *class)
{
  BobguiApplicationImplClass *impl_class = BOBGUI_APPLICATION_IMPL_CLASS (class);

  impl_class->handle_window_realize = bobgui_application_impl_wayland_handle_window_realize;
  impl_class->before_emit = bobgui_application_impl_wayland_before_emit;
  impl_class->window_removed = bobgui_application_impl_wayland_window_removed;
  impl_class->window_forget = bobgui_application_impl_wayland_window_forget;
  impl_class->inhibit = bobgui_application_impl_wayland_inhibit;
  impl_class->uninhibit = bobgui_application_impl_wayland_uninhibit;
  impl_class->startup = bobgui_application_impl_wayland_startup;
  impl_class->collect_window_state = bobgui_application_impl_wayland_collect_window_state;
  impl_class->collect_global_state = bobgui_application_impl_wayland_collect_global_state;
}
