/*
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

#ifdef GDK_WINDOWING_X11
#include <gdk/x11/gdkx.h>
#endif

#ifdef GDK_WINDOWING_WAYLAND
#include <gdk/wayland/gdkwayland.h>
#endif

#ifdef GDK_WINDOWING_MACOS
#include <gdk/macos/gdkmacos.h>
#endif

#ifdef GDK_WINDOWING_ANDROID
#include <gdk/android/gdkandroid.h>
#endif

#ifdef GDK_WINDOWING_WIN32
#include <gdk/win32/gdkwin32display.h>
#endif

G_DEFINE_TYPE (BobguiApplicationImpl, bobgui_application_impl, G_TYPE_OBJECT)

static void
bobgui_application_impl_init (BobguiApplicationImpl *impl)
{
}

static guint do_nothing (void) { return 0; }
static gpointer return_null (void) { return NULL; }

static void
bobgui_application_impl_class_init (BobguiApplicationImplClass *class)
{
  /* NB: can only 'do_nothing' for functions with integer or void return */
  class->startup = (gpointer) do_nothing;
  class->shutdown = (gpointer) do_nothing;
  class->before_emit = (gpointer) do_nothing;
  class->window_added = (gpointer) do_nothing;
  class->window_removed = (gpointer) do_nothing;
  class->window_forget = (gpointer) do_nothing;
  class->active_window_changed = (gpointer) do_nothing;
  class->handle_window_realize = (gpointer) do_nothing;
  class->handle_window_map = (gpointer) do_nothing;
  class->set_app_menu = (gpointer) do_nothing;
  class->set_menubar = (gpointer) do_nothing;
  class->inhibit = (gpointer) do_nothing;
  class->uninhibit = (gpointer) do_nothing;
  class->get_restore_reason = (gpointer) do_nothing;
  class->clear_restore_reason = (gpointer) do_nothing;
  class->collect_global_state = (gpointer) do_nothing;
  class->restore_global_state = (gpointer) do_nothing;
  class->collect_window_state = (gpointer) do_nothing;
  class->store_state = (gpointer) do_nothing;
  class->forget_state = (gpointer) do_nothing;
  class->retrieve_state = (gpointer) return_null;
}

void
bobgui_application_impl_startup (BobguiApplicationImpl *impl,
                              gboolean            support_save)
{
  BOBGUI_APPLICATION_IMPL_GET_CLASS (impl)->startup (impl, support_save);
}

void
bobgui_application_impl_shutdown (BobguiApplicationImpl *impl)
{
  BOBGUI_APPLICATION_IMPL_GET_CLASS (impl)->shutdown (impl);
}

void
bobgui_application_impl_before_emit (BobguiApplicationImpl *impl,
                                  GVariant           *platform_data)
{
  BOBGUI_APPLICATION_IMPL_GET_CLASS (impl)->before_emit (impl, platform_data);
}

void
bobgui_application_impl_window_added (BobguiApplicationImpl *impl,
                                   BobguiWindow          *window,
                                   GVariant           *state)
{
  BOBGUI_APPLICATION_IMPL_GET_CLASS (impl)->window_added (impl, window, state);
}

void
bobgui_application_impl_window_removed (BobguiApplicationImpl *impl,
                                     BobguiWindow          *window)
{
  BOBGUI_APPLICATION_IMPL_GET_CLASS (impl)->window_removed (impl, window);
}

void
bobgui_application_impl_window_forget (BobguiApplicationImpl *impl,
                                    BobguiWindow          *window)
{
  BOBGUI_APPLICATION_IMPL_GET_CLASS (impl)->window_forget (impl, window);
}

void
bobgui_application_impl_active_window_changed (BobguiApplicationImpl *impl,
                                            BobguiWindow          *window)
{
  BOBGUI_APPLICATION_IMPL_GET_CLASS (impl)->active_window_changed (impl, window);
}

void
bobgui_application_impl_handle_window_realize (BobguiApplicationImpl *impl,
                                            BobguiWindow          *window)
{
  BOBGUI_APPLICATION_IMPL_GET_CLASS (impl)->handle_window_realize (impl, window);
}

void
bobgui_application_impl_handle_window_map (BobguiApplicationImpl *impl,
                                        BobguiWindow          *window)
{
  BOBGUI_APPLICATION_IMPL_GET_CLASS (impl)->handle_window_map (impl, window);
}

void
bobgui_application_impl_set_app_menu (BobguiApplicationImpl *impl,
                                   GMenuModel         *app_menu)
{
  BOBGUI_APPLICATION_IMPL_GET_CLASS (impl)->set_app_menu (impl, app_menu);
}

void
bobgui_application_impl_set_menubar (BobguiApplicationImpl *impl,
                                  GMenuModel         *menubar)
{
  BOBGUI_APPLICATION_IMPL_GET_CLASS (impl)->set_menubar (impl, menubar);
}

guint
bobgui_application_impl_inhibit (BobguiApplicationImpl         *impl,
                              BobguiWindow                  *window,
                              BobguiApplicationInhibitFlags  flags,
                              const char                 *reason)
{
  return BOBGUI_APPLICATION_IMPL_GET_CLASS (impl)->inhibit (impl, window, flags, reason);
}

void
bobgui_application_impl_uninhibit (BobguiApplicationImpl *impl,
                                guint               cookie)
{
  BOBGUI_APPLICATION_IMPL_GET_CLASS (impl)->uninhibit (impl, cookie);
}

BobguiRestoreReason
bobgui_application_impl_get_restore_reason (BobguiApplicationImpl *impl)
{
  return BOBGUI_APPLICATION_IMPL_GET_CLASS (impl)->get_restore_reason (impl);
}

void
bobgui_application_impl_clear_restore_reason (BobguiApplicationImpl *impl)
{
  BOBGUI_APPLICATION_IMPL_GET_CLASS (impl)->clear_restore_reason (impl);
}

void
bobgui_application_impl_collect_global_state (BobguiApplicationImpl *impl,
                                           GVariantBuilder    *state)
{
  BOBGUI_APPLICATION_IMPL_GET_CLASS (impl)->collect_global_state (impl, state);
}

void
bobgui_application_impl_restore_global_state (BobguiApplicationImpl *impl,
                                           GVariant           *state)
{
  BOBGUI_APPLICATION_IMPL_GET_CLASS (impl)->restore_global_state (impl, state);
}

void
bobgui_application_impl_collect_window_state (BobguiApplicationImpl *impl,
                                           BobguiWindow          *window,
                                           GVariantBuilder    *state)
{
  BOBGUI_APPLICATION_IMPL_GET_CLASS (impl)->collect_window_state (impl, window, state);
}

void
bobgui_application_impl_store_state (BobguiApplicationImpl *impl,
                                  GVariant           *state)
{
  BOBGUI_APPLICATION_IMPL_GET_CLASS (impl)->store_state (impl, state);
}

void
bobgui_application_impl_forget_state (BobguiApplicationImpl *impl)
{
  BOBGUI_APPLICATION_IMPL_GET_CLASS (impl)->forget_state (impl);
}

GVariant *
bobgui_application_impl_retrieve_state (BobguiApplicationImpl *impl)
{
  return BOBGUI_APPLICATION_IMPL_GET_CLASS (impl)->retrieve_state (impl);
}

BobguiApplicationImpl *
bobgui_application_impl_new (BobguiApplication *application,
                          GdkDisplay     *display)
{
  BobguiApplicationImpl *impl;
  GType impl_type;

  impl_type = bobgui_application_impl_get_type ();

#ifdef GDK_WINDOWING_X11
  if (GDK_IS_X11_DISPLAY (display))
    impl_type = bobgui_application_impl_x11_get_type ();
#endif

#ifdef GDK_WINDOWING_WAYLAND
  if (GDK_IS_WAYLAND_DISPLAY (display))
    impl_type = bobgui_application_impl_wayland_get_type ();
#endif

#ifdef GDK_WINDOWING_MACOS
  if (GDK_IS_MACOS_DISPLAY (display))
    impl_type = bobgui_application_impl_quartz_get_type ();
#endif

#ifdef GDK_WINDOWING_ANDROID
  if (GDK_IS_ANDROID_DISPLAY (display))
    impl_type = bobgui_application_impl_android_get_type ();
#endif

#ifdef GDK_WINDOWING_WIN32
  if (GDK_IS_WIN32_DISPLAY (display))
    impl_type = bobgui_application_impl_win32_get_type ();
#endif

  impl = g_object_new (impl_type, NULL);
  impl->application = application;
  impl->display = display;

  return impl;
}
