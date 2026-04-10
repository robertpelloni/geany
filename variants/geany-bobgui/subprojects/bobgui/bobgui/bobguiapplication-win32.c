/*
 * BobguiApplication implementation for Win32.
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 * SPDX-FileCopyrightText: 2025-2026 GNOME Foundation
 *
 * ## LOGOFF inhibition
 * https://learn.microsoft.com/en-us/windows/win32/shutdown/logging-off
 * It seems that only visible windows are able to block logout requests,
 * so delegate the events handling to the HWND of each application window.
 * Requests counters are managed on GdkWin32 side.
 * Because we need to track added and removed windows, only windows attached
 * to the application are supported.
 * Using SetProcessShutdownParameters() with higher priority will ensure the
 * mainloop catches and responds to session events before all other HWNDs.
 *
 * ## IDLE and SUSPEND inhibition
 * https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-powersetrequest
 * Requests counters are automatically managed by PowerSetRequest().
 * It could be possible to use the old SetThreadExecutionState() API instead,
 * but this one needs to be called periodically.
 * The underlying Win32 APIs need a reason string, if not provided the
 * inhibition will be ignored.
 * Note that power requests will be cancelled if the user manually stops them,
 * e.g. by sleeping from the start menu or by closing the lid.
 */

#define WIN32_LEAN_AND_MEAN

#include "config.h"

#include "bobguiapplicationprivate.h"
#include "bobguinative.h"

#include "win32/gdkprivate-win32.h"

#include <windows.h>


typedef BobguiApplicationImplClass BobguiApplicationImplWin32Class;

typedef struct
{
  BobguiApplicationImpl impl;
  GSList *inhibitors;
  guint next_cookie;
} BobguiApplicationImplWin32;

typedef struct
{
  guint cookie;
  BobguiApplicationInhibitFlags flags;
  HANDLE pwr_handle;
  GdkSurface *surface;
} BobguiApplicationWin32Inhibitor;

G_DEFINE_TYPE (BobguiApplicationImplWin32, bobgui_application_impl_win32, BOBGUI_TYPE_APPLICATION_IMPL)


static void
appwin32_inhibitor_free (BobguiApplicationWin32Inhibitor *inhibitor)
{
  CloseHandle (inhibitor->pwr_handle);
  g_clear_object (&inhibitor->surface);
  g_free (inhibitor);
}

static void
cb_appwin32_session_query_end (void)
{
  GApplication *application = g_application_get_default ();

  if (BOBGUI_IS_APPLICATION (application))
    g_signal_emit_by_name (application, "query-end");
}

static void
cb_appwin32_session_end (void)
{
  GApplication *application = g_application_get_default ();

  if (G_IS_APPLICATION (application))
    g_application_quit (application);
}

static void
bobgui_application_impl_win32_shutdown (BobguiApplicationImpl *impl)
{
  BobguiApplicationImplWin32 *appwin32 = (BobguiApplicationImplWin32 *) impl;
  GList *iter;

  for (iter = bobgui_application_get_windows (impl->application); iter; iter = iter->next)
    {
      if (!bobgui_widget_get_realized (BOBGUI_WIDGET (iter->data)))
        continue;

      gdk_win32_surface_set_session_callbacks (bobgui_native_get_surface (BOBGUI_NATIVE (iter->data)),
                                               NULL,
                                               NULL);
    }

  g_slist_free_full (appwin32->inhibitors, (GDestroyNotify) appwin32_inhibitor_free);
  appwin32->inhibitors = NULL;
}

static void
bobgui_application_impl_win32_handle_window_realize (BobguiApplicationImpl *impl,
                                                  BobguiWindow          *window)
{
  g_return_if_fail (BOBGUI_IS_WINDOW (window));

  gdk_win32_surface_set_session_callbacks (bobgui_native_get_surface (BOBGUI_NATIVE (window)),
                                           cb_appwin32_session_query_end,
                                           cb_appwin32_session_end);
}

static void
bobgui_application_impl_win32_window_added (BobguiApplicationImpl *impl,
                                         BobguiWindow          *window,
                                         GVariant           *state)
{
  g_return_if_fail (BOBGUI_IS_WINDOW (window));

  if (!bobgui_widget_get_realized (BOBGUI_WIDGET (window)))
    return;   /* no surface yet, wait for handle_window_realize */

  gdk_win32_surface_set_session_callbacks (bobgui_native_get_surface (BOBGUI_NATIVE (window)),
                                           cb_appwin32_session_query_end,
                                           cb_appwin32_session_end);
}

static void
bobgui_application_impl_win32_window_removed (BobguiApplicationImpl *impl,
                                           BobguiWindow          *window)
{
  BobguiApplicationImplWin32 *appwin32 = (BobguiApplicationImplWin32 *) impl;
  GdkSurface *surface = bobgui_native_get_surface (BOBGUI_NATIVE (window));
  GSList *iter;

  if (!surface)
    return;

  gdk_win32_surface_set_session_callbacks (surface,
                                           NULL,
                                           NULL);

  /* Ensure we don't keep a ref of the surface in the inhibitors,
   * should the window get removed before uninhibit is called.
   */
  for (iter = appwin32->inhibitors; iter; iter = iter->next)
    {
      BobguiApplicationWin32Inhibitor *inhibitor = iter->data;

      if ((inhibitor->flags & BOBGUI_APPLICATION_INHIBIT_LOGOUT) &&
          (inhibitor->surface == surface))
        {
          gdk_win32_surface_uninhibit_logout (inhibitor->surface);
          inhibitor->flags &= ~BOBGUI_APPLICATION_INHIBIT_LOGOUT;
          g_clear_object (&inhibitor->surface);
        }
    }
}

static guint
bobgui_application_impl_win32_inhibit (BobguiApplicationImpl         *impl,
                                    BobguiWindow                  *window,
                                    BobguiApplicationInhibitFlags  flags,
                                    const char                 *reason)
{
  BobguiApplicationImplWin32 *appwin32 = (BobguiApplicationImplWin32 *) impl;
  BobguiApplicationWin32Inhibitor *inhibitor = g_new0 (BobguiApplicationWin32Inhibitor, 1);
  wchar_t *reason_w = g_utf8_to_utf16 (reason, -1, NULL, NULL, NULL);

  inhibitor->cookie = ++appwin32->next_cookie;

  if (flags & BOBGUI_APPLICATION_INHIBIT_LOGOUT)
    {
      if (BOBGUI_IS_WINDOW (window) &&
          impl->application == bobgui_window_get_application (window))
        {
          GdkSurface *surface = bobgui_native_get_surface (BOBGUI_NATIVE (window));

          if (gdk_win32_surface_inhibit_logout (surface, reason_w))
            {
              inhibitor->surface = g_object_ref (surface);
              inhibitor->flags |= BOBGUI_APPLICATION_INHIBIT_LOGOUT;
            }
        }
      else
        g_warning ("Logout inhibition is only supported on application windows");
    }

  if (flags & (BOBGUI_APPLICATION_INHIBIT_SUSPEND | BOBGUI_APPLICATION_INHIBIT_IDLE))
    {
      REASON_CONTEXT context;

      memset (&context, 0, sizeof (context));
      context.Version = POWER_REQUEST_CONTEXT_VERSION;
      context.Flags = POWER_REQUEST_CONTEXT_SIMPLE_STRING;
      context.Reason.SimpleReasonString = reason_w;

      inhibitor->pwr_handle = PowerCreateRequest (&context);

      if (flags & BOBGUI_APPLICATION_INHIBIT_SUSPEND)
        {
          if (PowerSetRequest (inhibitor->pwr_handle, PowerRequestSystemRequired))
            inhibitor->flags |= BOBGUI_APPLICATION_INHIBIT_SUSPEND;
          else
            g_warning ("Failed to apply suspend inhibition");
        }

      if (flags & BOBGUI_APPLICATION_INHIBIT_IDLE)
        {
          if (PowerSetRequest (inhibitor->pwr_handle, PowerRequestDisplayRequired))
            inhibitor->flags |= BOBGUI_APPLICATION_INHIBIT_IDLE;
          else
            g_warning ("Failed to apply idle inhibition");
        }
    }

  g_free (reason_w);

  if (inhibitor->flags)
    {
      appwin32->inhibitors = g_slist_prepend (appwin32->inhibitors, inhibitor);
      return inhibitor->cookie;
    }

  appwin32_inhibitor_free (inhibitor);
  return 0;
}

static void
bobgui_application_impl_win32_uninhibit (BobguiApplicationImpl *impl, guint cookie)
{
  BobguiApplicationImplWin32 *appwin32 = (BobguiApplicationImplWin32 *) impl;
  GSList *iter;

  for (iter = appwin32->inhibitors; iter; iter = iter->next)
    {
      BobguiApplicationWin32Inhibitor *inhibitor = iter->data;

      if (inhibitor->cookie == cookie)
        {
          if (inhibitor->flags & BOBGUI_APPLICATION_INHIBIT_LOGOUT)
            gdk_win32_surface_uninhibit_logout (inhibitor->surface);

          if (inhibitor->flags & BOBGUI_APPLICATION_INHIBIT_SUSPEND)
            PowerClearRequest (inhibitor->pwr_handle, PowerRequestSystemRequired);

          if (inhibitor->flags & BOBGUI_APPLICATION_INHIBIT_IDLE)
            PowerClearRequest (inhibitor->pwr_handle, PowerRequestDisplayRequired);

          appwin32_inhibitor_free (inhibitor);
          appwin32->inhibitors = g_slist_delete_link (appwin32->inhibitors, iter);
          return;
        }
    }

  g_warning ("Invalid inhibitor cookie: %d", cookie);
}

static gboolean
bobgui_application_impl_win32_is_inhibited (BobguiApplicationImpl         *impl,
                                         BobguiApplicationInhibitFlags  flags)
{
  BobguiApplicationImplWin32 *appwin32 = (BobguiApplicationImplWin32 *) impl;
  BobguiApplicationInhibitFlags active_flags = 0;
  GSList *iter;

  for (iter = appwin32->inhibitors; iter; iter = iter->next)
    {
      BobguiApplicationWin32Inhibitor *inhibitor = iter->data;
      active_flags |= inhibitor->flags;
    }

  return (active_flags & flags) == flags;
}

static void
bobgui_application_impl_win32_class_init (BobguiApplicationImplClass *klass)
{
  klass->shutdown = bobgui_application_impl_win32_shutdown;
  klass->handle_window_realize = bobgui_application_impl_win32_handle_window_realize;
  klass->window_added = bobgui_application_impl_win32_window_added;
  klass->window_removed = bobgui_application_impl_win32_window_removed;
  klass->inhibit = bobgui_application_impl_win32_inhibit;
  klass->uninhibit = bobgui_application_impl_win32_uninhibit;
  klass->is_inhibited = bobgui_application_impl_win32_is_inhibited;
}

static void
bobgui_application_impl_win32_init (BobguiApplicationImplWin32 *appwin32)
{
  DWORD dwLevel, dwFlags;

  appwin32->next_cookie = 0;
  appwin32->inhibitors = NULL;

  if (GetProcessShutdownParameters (&dwLevel, &dwFlags))
    {
      dwLevel = MAX (dwLevel, 0x300);
      SetProcessShutdownParameters (dwLevel, dwFlags);
    }
}
