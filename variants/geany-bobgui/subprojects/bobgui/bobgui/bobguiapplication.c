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

#include "bobguiapplication.h"
#include "gdkprofilerprivate.h"

#include <stdlib.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "bobguiapplicationprivate.h"
#include "bobguiapplicationwindowprivate.h"
#include "bobguimarshalers.h"
#include "bobguimain.h"
#include "bobguiicontheme.h"
#include "bobguibuilder.h"
#include "bobguiprivate.h"
#include "bobguitypebuiltins.h"

/* NB: please do not add backend-specific GDK headers here.  This should
 * be abstracted via BobguiApplicationImpl.
 */

#ifdef GDK_WINDOWING_ANDROID
/* Unfortunatly, we'll have to include this here, as we want to force
 * applications running as Android applications to run as service that
 * never exists.
 *
 * It is not possible to move this into BobguiApplicationImpl, as
 * bobgui_application_startup has yet to be called, which means that
 * bobgui_init hasn't been called yet and it is impossible to determine the
 * the correct BobguiApplicationImpl to use.
 */
#include "android/gdkandroidinit-private.h"
#endif // GDK_WINDOWING_ANDROID

/**
 * BobguiApplication:
 *
 * A high-level API for writing applications.
 *
 * `BobguiApplication` supports many aspects of writing a BOBGUI application
 * in a convenient fashion, without enforcing a one-size-fits-all model.
 *
 * Currently, it handles BOBGUI initialization, application uniqueness, session
 * management, provides some basic scriptability and desktop shell integration
 * by exporting actions and menus and manages a list of toplevel windows whose
 * life-cycle is automatically tied to the life-cycle of your application.
 *
 * While `BobguiApplication` works fine with plain [class@Bobgui.Window]s,
 * it is recommended to use it together with [class@Bobgui.ApplicationWindow].
 *
 * ## Initialization
 *
 * A typical `BobguiApplication` will create a window in its
 * [signal@GIO.Application::activate], [signal@GIO.Application::open]
 * or [signal@GIO.Application::command-line] handlers. Note that all
 * of these signals may be emitted multiple times, so handlers must
 * be careful to take existing windows into account.
 *
 * A typical ::activate handler should look like this:
 *
 * ```
 * static void
 * activate (GApplication *gapp)
 * {
 *   BobguiApplication *app = BOBGUI_APPLICATION (gapp);
 *   BobguiWindow *window;
 *
 *   window = bobgui_application_get_active_window (app);
 *   if (!window)
 *     window = create_window (app);
 *
 *   bobgui_window_present (window);
 * }
 * ```
 *
 * ## Automatic resources
 *
 * `BobguiApplication` will automatically load menus from the `BobguiBuilder`
 * resource located at "bobgui/menus.ui", relative to the application's
 * resource base path (see [method@Gio.Application.set_resource_base_path]).
 * The menu with the ID "menubar" is taken as the application's
 * menubar. Additional menus (most interesting submenus) can be named
 * and accessed via [method@Bobgui.Application.get_menu_by_id] which allows for
 * dynamic population of a part of the menu structure.
 *
 * Note that automatic resource loading uses the resource base path
 * that is set at construction time and will not work if the resource
 * base path is changed at a later time.
 *
 * It is also possible to provide the menubar manually using
 * [method@Bobgui.Application.set_menubar].
 *
 * `BobguiApplication` will also automatically setup an icon search path for
 * the default icon theme by appending "icons" to the resource base
 * path. This allows your application to easily store its icons as
 * resources. See [method@Bobgui.IconTheme.add_resource_path] for more
 * information.
 *
 * If there is a resource located at `bobgui/help-overlay.ui` which
 * defines a [class@Bobgui.ShortcutsWindow] with ID `help_overlay` then
 * `BobguiApplication` associates an instance of this shortcuts window with
 * each [class@Bobgui.ApplicationWindow] and sets up the keyboard accelerator
 * <kbd>Control</kbd>+<kbd>?</kbd> to open it. To create a menu item that
 * displays the shortcuts window, associate the item with the action
 * `win.show-help-overlay`.
 *
 * `BobguiApplication` will also automatically set the application id as the
 * default window icon. Use [func@Bobgui.Window.set_default_icon_name] or
 * [property@Bobgui.Window:icon-name] to override that behavior.
 *
 * ## State saving
 *
 * `BobguiApplication` registers with a session manager if possible and
 * offers various functionality related to the session life-cycle,
 * such as state saving.
 *
 * State-saving functionality can be enabled by setting the
 * [property@Bobgui.Application:support-save] property to true.
 *
 * In order to save and restore per-window state, applications must
 * connect to the [signal@Bobgui.Application::restore-window] signal and
 * handle the [signal@Bobgui.ApplicationWindow::save-state] signal. There
 * are also [signal@Bobgui.Application::restore-state] and
 * [signal@BobguiApplication::save-state] signals, which can be used
 * for global state that is not connected to any window.
 *
 * `BobguiApplication` automatically saves state before app shutdown, and by
 * default periodically auto-saves app state (as configured by the
 * [property@Bobgui.Application:autosave-interval] property). Applications can
 * also call [method@Bobgui.Application.save] themselves at opportune times.
 *
 * # Inhibiting
 *
 * An application can block various ways to end the session with
 * the [method@Bobgui.Application.inhibit] function. Typical use cases for
 * this kind of inhibiting are long-running, uninterruptible operations,
 * such as burning a CD or performing a disk backup. The session
 * manager may not honor the inhibitor, but it can be expected to
 * inform the user about the negative consequences of ending the
 * session while inhibitors are present.
 *
 * ## A simple application
 *
 * [A simple example](https://gitlab.gnome.org/GNOME/bobgui/tree/main/examples/bp/bloatpad.c)
 * is available in the BOBGUI source code repository
 *
 * ## See Also
 *
 * - [Using BobguiApplication](https://developer.gnome.org/documentation/tutorials/application.html)
 * - [Getting Started with BOBGUI: Basics](getting_started.html#basics)
 */

enum {
  WINDOW_ADDED,
  WINDOW_REMOVED,
  QUERY_END,
  SAVE_STATE,
  RESTORE_STATE,
  RESTORE_WINDOW,
  LAST_SIGNAL
};

static guint bobgui_application_signals[LAST_SIGNAL];

enum {
  PROP_ZERO,
  PROP_REGISTER_SESSION,
  PROP_SCREENSAVER_ACTIVE,
  PROP_MENUBAR,
  PROP_ACTIVE_WINDOW,
  PROP_SUPPORT_SAVE,
  PROP_AUTOSAVE_INTERVAL,
  NUM_PROPERTIES
};

static GParamSpec *bobgui_application_props[NUM_PROPERTIES];

typedef struct
{
  BobguiApplicationImpl *impl;
  BobguiApplicationAccels *accels;

  GList *windows;

  GMenuModel      *menubar;
  guint            last_window_id;

  gboolean         register_session;
  gboolean         screensaver_active;
  BobguiActionMuxer  *muxer;
  BobguiBuilder      *menus_builder;
  char            *help_overlay_path;
  gboolean         support_save;
  guint            autosave_interval;
  guint            autosave_id;
  GVariant        *pending_window_state;
  GVariant        *kept_window_state;
  gboolean         restored;
  gboolean         forgotten;
} BobguiApplicationPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (BobguiApplication, bobgui_application, G_TYPE_APPLICATION)

static void
schedule_autosave (BobguiApplication *application);

static void
bobgui_application_window_active_cb (BobguiWindow      *window,
                                  GParamSpec     *pspec,
                                  BobguiApplication *application)
{
  BobguiApplicationPrivate *priv = bobgui_application_get_instance_private (application);
  GList *link;

  schedule_autosave (application);

  if (!bobgui_window_is_active (window))
    return;

  /* Keep the window list sorted by most-recently-focused. */
  link = g_list_find (priv->windows, window);
  if (link != NULL && link != priv->windows)
    {
      priv->windows = g_list_remove_link (priv->windows, link);
      priv->windows = g_list_concat (link, priv->windows);
    }

  if (priv->impl)
    bobgui_application_impl_active_window_changed (priv->impl, window);

  g_object_notify_by_pspec (G_OBJECT (application), bobgui_application_props[PROP_ACTIVE_WINDOW]);
}

static void
bobgui_application_load_resources (BobguiApplication *application)
{
  BobguiApplicationPrivate *priv = bobgui_application_get_instance_private (application);
  const char *base_path;
  const char *optional_slash = "/";

  base_path = g_application_get_resource_base_path (G_APPLICATION (application));

  if (base_path == NULL)
    return;

  if (base_path[strlen (base_path) - 1] == '/')
    optional_slash = "";

  /* Expand the icon search path */
  {
    BobguiIconTheme *default_theme;
    char *iconspath;

    default_theme = bobgui_icon_theme_get_for_display (gdk_display_get_default ());
    iconspath = g_strconcat (base_path, optional_slash, "icons/", NULL);
    bobgui_icon_theme_add_resource_path (default_theme, iconspath);
    g_free (iconspath);
  }

  /* Load the menus */
  {
    char *menuspath;

    menuspath = g_strconcat (base_path, optional_slash, "bobgui/menus.ui", NULL);
    if (g_resources_get_info (menuspath, G_RESOURCE_LOOKUP_FLAGS_NONE, NULL, NULL, NULL))
      priv->menus_builder = bobgui_builder_new_from_resource (menuspath);
    g_free (menuspath);

    if (priv->menus_builder)
      {
        GObject *menu;

        menu = bobgui_builder_get_object (priv->menus_builder, "menubar");
        if (menu != NULL && G_IS_MENU_MODEL (menu))
          bobgui_application_set_menubar (application, G_MENU_MODEL (menu));
      }
  }

  /* Help overlay */
  {
    char *path;

    path = g_strconcat (base_path, optional_slash, "bobgui/help-overlay.ui", NULL);
    if (g_resources_get_info (path, G_RESOURCE_LOOKUP_FLAGS_NONE, NULL, NULL, NULL))
      {
#ifdef __APPLE__
        const char * const accels[] = { "<Meta>question", NULL };
#else
        const char * const accels[] = { "<Control>question", NULL };
#endif

        priv->help_overlay_path = path;
        bobgui_application_set_accels_for_action (application, "win.show-help-overlay", accels);
      }
    else
      {
        g_free (path);
      }
  }
}

static void
bobgui_application_set_window_icon (BobguiApplication *application)
{
  BobguiIconTheme *default_theme;
  const char *appid;

  if (bobgui_window_get_default_icon_name () != NULL)
    return;

  default_theme = bobgui_icon_theme_get_for_display (gdk_display_get_default ());
  appid = g_application_get_application_id (G_APPLICATION (application));

  if (appid == NULL || !bobgui_icon_theme_has_icon (default_theme, appid))
    return;

  bobgui_window_set_default_icon_name (appid);
}

static void
bobgui_application_startup (GApplication *g_application)
{
  BobguiApplication *application = BOBGUI_APPLICATION (g_application);
  BobguiApplicationPrivate *priv = bobgui_application_get_instance_private (application);
  gint64 before G_GNUC_UNUSED;
  gint64 before2 G_GNUC_UNUSED;

  before = GDK_PROFILER_CURRENT_TIME;

  G_APPLICATION_CLASS (bobgui_application_parent_class)->startup (g_application);

  bobgui_action_muxer_insert (priv->muxer, "app", G_ACTION_GROUP (application));

  gdk_set_portals_app_id (g_application_get_application_id (g_application));

  before2 = GDK_PROFILER_CURRENT_TIME;
  bobgui_init ();
  gdk_profiler_end_mark (before2, "bobgui_init", NULL);

  priv->impl = bobgui_application_impl_new (application, gdk_display_get_default ());

  bobgui_application_impl_startup (priv->impl, priv->support_save);

  bobgui_application_load_resources (application);
  bobgui_application_set_window_icon (application);

  gdk_profiler_end_mark (before, "Application startup", NULL);
}

static void
bobgui_application_shutdown (GApplication *g_application)
{
  BobguiApplication *application = BOBGUI_APPLICATION (g_application);
  BobguiApplicationPrivate *priv = bobgui_application_get_instance_private (application);

  if (priv->impl == NULL)
    return;

  if (!priv->forgotten)
    bobgui_application_save (application);

  bobgui_application_impl_shutdown (priv->impl);
  g_clear_object (&priv->impl);

  bobgui_action_muxer_remove (priv->muxer, "app");

  bobgui_main_sync ();

  G_APPLICATION_CLASS (bobgui_application_parent_class)->shutdown (g_application);
}

static gboolean
bobgui_application_local_command_line (GApplication   *application,
                                    char         ***arguments,
                                    int            *exit_status)
{
  /* We need to call setlocale() here so --help output works */
  setlocale_initialization ();

#ifdef GDK_WINDOWING_ANDROID
  if (gdk_android_get_activity ())
    {
      g_application_set_flags (application, g_application_get_flags (application) | G_APPLICATION_IS_SERVICE);
      // This should get the application service to never exit on Android
      g_application_hold (application);
    }
#endif // GDK_WINDOWING_ANDROID

  return G_APPLICATION_CLASS (bobgui_application_parent_class)->local_command_line (application, arguments, exit_status);
}

static void
bobgui_application_add_platform_data (GApplication    *application,
                                   GVariantBuilder *builder)
{
  /* This is slightly evil.
   *
   * We don't have an impl here because we're remote so we can't figure
   * out what to do on a per-display-server basis.
   *
   * So we do all the things... which currently is just one thing.
   */
  const char *startup_id = gdk_get_startup_notification_id ();

  if (startup_id && g_utf8_validate (startup_id, -1, NULL))
    {
      g_variant_builder_add (builder, "{sv}", "activation-token",
                             g_variant_new_string (startup_id));
      g_variant_builder_add (builder, "{sv}", "desktop-startup-id",
                             g_variant_new_string (startup_id));
    }
}

static gboolean
bobgui_application_restore (BobguiApplication   *application);

static void
bobgui_application_before_emit (GApplication *app,
                             GVariant     *platform_data)
{
  BobguiApplication *application = BOBGUI_APPLICATION (app);
  BobguiApplicationPrivate *priv = bobgui_application_get_instance_private (application);

  bobgui_application_impl_before_emit (priv->impl, platform_data);

  if (priv->support_save && !priv->restored)
    {
      bobgui_application_restore (application);
      schedule_autosave (application);
      priv->restored = TRUE;
    }
}

static void
bobgui_application_after_emit (GApplication *application,
                            GVariant     *platform_data)
{
}

static void
bobgui_application_init (BobguiApplication *application)
{
  BobguiApplicationPrivate *priv = bobgui_application_get_instance_private (application);

  priv->muxer = bobgui_action_muxer_new (NULL);

  priv->accels = bobgui_application_accels_new ();

  priv->autosave_interval = 15;
}

static void
bobgui_application_window_added (BobguiApplication *application,
                              BobguiWindow      *window)
{
  BobguiApplicationPrivate *priv = bobgui_application_get_instance_private (application);

  if (BOBGUI_IS_APPLICATION_WINDOW (window))
    {
      bobgui_application_window_set_id (BOBGUI_APPLICATION_WINDOW (window), ++priv->last_window_id);
      if (priv->help_overlay_path)
        {
          BobguiBuilder *builder;
          BobguiWidget *help_overlay;

          builder = bobgui_builder_new_from_resource (priv->help_overlay_path);
          help_overlay = BOBGUI_WIDGET (bobgui_builder_get_object (builder, "help_overlay"));
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
          if (BOBGUI_IS_SHORTCUTS_WINDOW (help_overlay))
            bobgui_application_window_set_help_overlay (BOBGUI_APPLICATION_WINDOW (window),
                                                     BOBGUI_SHORTCUTS_WINDOW (help_overlay));
G_GNUC_END_IGNORE_DEPRECATIONS
          g_object_unref (builder);
        }
    }

  g_clear_pointer (&priv->kept_window_state, g_variant_unref);

  priv->windows = g_list_prepend (priv->windows, window);
  bobgui_window_set_application (window, application);
  g_application_hold (G_APPLICATION (application));

  g_signal_connect (window, "notify::is-active",
                    G_CALLBACK (bobgui_application_window_active_cb),
                    application);

  bobgui_application_impl_window_added (priv->impl, window, priv->pending_window_state);
  priv->pending_window_state = NULL;

  bobgui_application_impl_active_window_changed (priv->impl, window);

  g_object_notify_by_pspec (G_OBJECT (application), bobgui_application_props[PROP_ACTIVE_WINDOW]);
}

static gboolean
should_remove_from_session (BobguiApplication *application,
                            BobguiWindow      *window)
{
  BobguiApplicationPrivate *priv = bobgui_application_get_instance_private (application);

  if (!priv->support_save)
    return TRUE;

  if (bobgui_window_get_transient_for (window))
    {
      BOBGUI_DEBUG (SESSION, "Removing transient toplevel from session state");
      return TRUE;
    }

  for (GList *l = priv->windows; l != NULL; l = l->next)
    {
      BobguiWindow *candidate = BOBGUI_WINDOW (l->data);
      if (candidate != window && !bobgui_window_get_transient_for (candidate))
        {
          /* This isn't the last non-transient toplevel in the session */
          BOBGUI_DEBUG (SESSION, "Removing toplevel from session state");
          return TRUE;
        }
    }

  BOBGUI_DEBUG (SESSION, "Keeping last toplevel in session state");
  return FALSE;
}

static GVariant *
collect_window_state (BobguiApplication *application,
                      BobguiWindow      *window);

static void
bobgui_application_window_removed (BobguiApplication *application,
                                BobguiWindow      *window)
{
  BobguiApplicationPrivate *priv = bobgui_application_get_instance_private (application);
  gpointer old_active;
  gboolean remove_from_session;

  old_active = priv->windows;

  remove_from_session = should_remove_from_session (application, window);

  if (!remove_from_session)
    {
      g_assert (!priv->kept_window_state);
      priv->kept_window_state = collect_window_state (application, window);

      /* If we're keeping around the last window, and the window is now gone,
       * from the user's perspective the app is now gone even if it hasn't
       * technically quit yet. In case the user relaunches the app before
       * it manages to quit, let's re-restore state on next startup. */
      priv->restored = FALSE;
      bobgui_application_impl_clear_restore_reason (priv->impl);
    }

  if (priv->impl)
    {
      bobgui_application_impl_window_removed (priv->impl, window);

      if (remove_from_session)
        bobgui_application_impl_window_forget (priv->impl, window);
    }

  g_signal_handlers_disconnect_by_func (window,
                                        bobgui_application_window_active_cb,
                                        application);

  g_application_release (G_APPLICATION (application));
  priv->windows = g_list_remove (priv->windows, window);
  bobgui_window_set_application (window, NULL);

  if (priv->windows != old_active && priv->impl)
    {
      bobgui_application_impl_active_window_changed (priv->impl, priv->windows ? priv->windows->data : NULL);
      g_object_notify_by_pspec (G_OBJECT (application), bobgui_application_props[PROP_ACTIVE_WINDOW]);
    }
}

static void
bobgui_application_get_property (GObject    *object,
                              guint       prop_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
  BobguiApplication *application = BOBGUI_APPLICATION (object);
  BobguiApplicationPrivate *priv = bobgui_application_get_instance_private (application);

  switch (prop_id)
    {
    case PROP_REGISTER_SESSION:
      g_value_set_boolean (value, priv->register_session);
      break;

    case PROP_SCREENSAVER_ACTIVE:
      g_value_set_boolean (value, priv->screensaver_active);
      break;

    case PROP_MENUBAR:
      g_value_set_object (value, bobgui_application_get_menubar (application));
      break;

    case PROP_ACTIVE_WINDOW:
      g_value_set_object (value, bobgui_application_get_active_window (application));
      break;

    case PROP_SUPPORT_SAVE:
      g_value_set_boolean (value, priv->support_save);
      break;

    case PROP_AUTOSAVE_INTERVAL:
      g_value_set_uint (value, priv->autosave_interval);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bobgui_application_set_property (GObject      *object,
                              guint         prop_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  BobguiApplication *application = BOBGUI_APPLICATION (object);
  BobguiApplicationPrivate *priv = bobgui_application_get_instance_private (application);

  switch (prop_id)
    {
    case PROP_REGISTER_SESSION:
      priv->register_session = g_value_get_boolean (value);
      break;

    case PROP_MENUBAR:
      bobgui_application_set_menubar (application, g_value_get_object (value));
      break;

    case PROP_SUPPORT_SAVE:
      priv->support_save = g_value_get_boolean (value);
      break;

    case PROP_AUTOSAVE_INTERVAL:
      priv->autosave_interval = g_value_get_uint (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bobgui_application_finalize (GObject *object)
{
  BobguiApplication *application = BOBGUI_APPLICATION (object);
  BobguiApplicationPrivate *priv = bobgui_application_get_instance_private (application);

  g_clear_object (&priv->menus_builder);
  g_clear_object (&priv->menubar);
  g_clear_object (&priv->muxer);
  g_clear_object (&priv->accels);

  g_clear_pointer (&priv->kept_window_state, g_variant_unref);
  g_clear_handle_id (&priv->autosave_id, g_source_remove);

  g_free (priv->help_overlay_path);

  G_OBJECT_CLASS (bobgui_application_parent_class)->finalize (object);
}

static gboolean
bobgui_application_dbus_register (GApplication     *application,
                               GDBusConnection  *connection,
                               const char       *object_path,
                               GError          **error)
{
  return TRUE;
}

static void
bobgui_application_dbus_unregister (GApplication     *application,
                                 GDBusConnection  *connection,
                                 const char       *object_path)
{
}

static void
bobgui_application_class_init (BobguiApplicationClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  GApplicationClass *application_class = G_APPLICATION_CLASS (class);

  object_class->get_property = bobgui_application_get_property;
  object_class->set_property = bobgui_application_set_property;
  object_class->finalize = bobgui_application_finalize;

  application_class->local_command_line = bobgui_application_local_command_line;
  application_class->add_platform_data = bobgui_application_add_platform_data;
  application_class->before_emit = bobgui_application_before_emit;
  application_class->after_emit = bobgui_application_after_emit;
  application_class->startup = bobgui_application_startup;
  application_class->shutdown = bobgui_application_shutdown;
  application_class->dbus_register = bobgui_application_dbus_register;
  application_class->dbus_unregister = bobgui_application_dbus_unregister;

  class->window_added = bobgui_application_window_added;
  class->window_removed = bobgui_application_window_removed;

  /**
   * BobguiApplication::window-added:
   * @application: the application which emitted the signal
   * @window: the newly-added window
   *
   * Emitted when a window is added to an application.
   *
   * See [method@Bobgui.Application.add_window].
   */
  bobgui_application_signals[WINDOW_ADDED] =
    g_signal_new (I_("window-added"), BOBGUI_TYPE_APPLICATION, G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (BobguiApplicationClass, window_added),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 1, BOBGUI_TYPE_WINDOW);

  /**
   * BobguiApplication::window-removed:
   * @application: the application which emitted the signal
   * @window: the window that is being removed
   *
   * Emitted when a window is removed from an application.
   *
   * This can happen as a side-effect of the window being destroyed
   * or explicitly through [method@Bobgui.Application.remove_window].
   */
  bobgui_application_signals[WINDOW_REMOVED] =
    g_signal_new (I_("window-removed"), BOBGUI_TYPE_APPLICATION, G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (BobguiApplicationClass, window_removed),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 1, BOBGUI_TYPE_WINDOW);

  /**
   * BobguiApplication::query-end:
   * @application: the application which emitted the signal
   *
   * Emitted when the session manager is about to end the session.
   *
   * Applications can connect to this signal and call
   * [method@Bobgui.Application.inhibit] with [flags@Bobgui.ApplicationInhibitFlags.logout]
   * to delay the end of the session until state has been saved.
   */
  bobgui_application_signals[QUERY_END] =
    g_signal_new (I_("query-end"), BOBGUI_TYPE_APPLICATION, G_SIGNAL_RUN_FIRST,
                  0,
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 0);

  /**
   * BobguiApplication::restore-window:
   * @application: the `BobguiApplication` which emitted the signal
   * @reason: the reason this window is restored
   * @state: an "a{sv}" `GVariant` with state to restore, as saved by a [signal@Bobgui.ApplicationWindow::save-state] handler
   *
   * Emitted when an application's per-window state is restored.
   *
   * In response to this signal, you should create a new application
   * window, add it to @application, apply the provided @state, and present it.
   * The application can use the @reason to determine how much of the state
   * should be restored.
   *
   * You must be careful to be robust in the face of app upgrades and downgrades:
   * the @state might have been created by a previous or occasionally even a future
   * version of your app. Do not assume that a given key exists in the state.
   * Apps must try to restore state saved by a previous version, but are free to
   * discard state if it was written by a future version.
   *
   * BOBGUI will remember which window the user was using most recently, and will
   * emit this signal for that window first. Thus, if you decide that the provided
   * @reason means that only one window should be restored, you can reliably
   * ignore emissions if a window already exists
   *
   * Note that this signal is not emitted only during the app's initial launch.
   * If all windows are closed but the app keeps running, the signal will be
   * emitted the next time a new window is opened.
   *
   * Since: 4.24
   */
  bobgui_application_signals[RESTORE_WINDOW] =
    g_signal_new (I_("restore-window"), BOBGUI_TYPE_APPLICATION, G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (BobguiApplicationClass, restore_window),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 2,
                  BOBGUI_TYPE_RESTORE_REASON,
                  G_TYPE_VARIANT);

  /**
   * BobguiApplication::save-state:
   * @application: the `BobguiApplication` which emitted the signal
   * @dict: a `GVariantDict`
   *
   * Emitted when the application is saving global state.
   *
   * The handler for this signal should persist any
   * global state of @application into @dict.
   *
   * See [signal@Bobgui.Application::restore-state] for how to
   * restore global state, and [signal@Bobgui.ApplicationWindow::save-state]
   * and [signal@Bobgui.Application::restore-window] for handling
   * per-window state.
   *
   * Returns: true to stop stop further handlers from running
   *
   * Since: 4.24
   */
  bobgui_application_signals[SAVE_STATE] =
    g_signal_new (I_("save-state"),
                  G_TYPE_FROM_CLASS (class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (BobguiApplicationClass, save_state),
                  _bobgui_boolean_handled_accumulator, NULL,
                  NULL,
                  G_TYPE_BOOLEAN, 1,
                  G_TYPE_VARIANT_DICT);

  /**
   * BobguiApplication::restore-state:
   * @application: the `BobguiApplication` which emitted the signal
   * @reason: the reason for restoring state
   * @state: an "a{sv}" `GVariant` with state to restore
   *
   * Emitted when application global state is restored.
   *
   * The handler for this signal should do the opposite of what the
   * corresponding handler for [signal@Bobgui.Application::save-state]
   * does.
   *
   * Returns: true to stop stop further handlers from running
   *
   * Since: 4.24
   */
  bobgui_application_signals[RESTORE_STATE] =
    g_signal_new (I_("restore-state"),
                  G_TYPE_FROM_CLASS (class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (BobguiApplicationClass, restore_state),
                  _bobgui_boolean_handled_accumulator, NULL,
                  NULL,
                  G_TYPE_BOOLEAN, 2,
                  BOBGUI_TYPE_RESTORE_REASON,
                  G_TYPE_VARIANT);

  /**
   * BobguiApplication:register-session:
   *
   * Set this property to true to register with the session manager.
   *
   * This will make BOBGUI track the session state (such as the
   * [property@Bobgui.Application:screensaver-active] property).
   *
   * Deprecated: 4.22: This property is ignored. BOBGUI always registers
   * with the session manager
   */
  bobgui_application_props[PROP_REGISTER_SESSION] =
    g_param_spec_boolean ("register-session", NULL, NULL,
                          FALSE,
                          G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS|G_PARAM_DEPRECATED);

  /**
   * BobguiApplication:screensaver-active:
   *
   * This property is true if BOBGUI believes that the screensaver
   * is currently active.
   *
   * Tracking the screensaver state is currently only supported on
   * Linux.
   */
  bobgui_application_props[PROP_SCREENSAVER_ACTIVE] =
    g_param_spec_boolean ("screensaver-active", NULL, NULL,
                          FALSE,
                          G_PARAM_READABLE|G_PARAM_STATIC_STRINGS);

  /**
   * BobguiApplication:menubar:
   *
   * The menu model to be used for the application's menu bar.
   */
  bobgui_application_props[PROP_MENUBAR] =
    g_param_spec_object ("menubar", NULL, NULL,
                         G_TYPE_MENU_MODEL,
                         G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS);

  /**
   * BobguiApplication:active-window:
   *
   * The currently focused window of the application.
   */
  bobgui_application_props[PROP_ACTIVE_WINDOW] =
    g_param_spec_object ("active-window", NULL, NULL,
                         BOBGUI_TYPE_WINDOW,
                         G_PARAM_READABLE|G_PARAM_STATIC_STRINGS);

  /**
   * BobguiApplication:support-save:
   *
   * Set this property to true if the application supports
   * state saving and restoring.
   *
   * Since: 4.24
   */
  bobgui_application_props[PROP_SUPPORT_SAVE] =
    g_param_spec_boolean ("support-save", NULL, NULL,
                          FALSE,
                          G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS|G_PARAM_DEPRECATED);

  /**
   * BobguiApplication:autosave-interval:
   *
   * The number of seconds between automatic state saves. Defaults to 15.
   * A value of 0 will opt out of automatic state saving.
   *
   * Since: 4.24
   */
  bobgui_application_props[PROP_AUTOSAVE_INTERVAL] =
    g_param_spec_uint ("autosave-interval", NULL, NULL,
                       0, G_MAXUINT, 15,
                       G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, NUM_PROPERTIES, bobgui_application_props);
}

/**
 * bobgui_application_new:
 * @application_id: (nullable): The application ID
 * @flags: the application flags
 *
 * Creates a new application instance.
 *
 * When using `BobguiApplication`, it is not necessary to call [func@Bobgui.init]
 * manually. It is called as soon as the application gets registered as
 * the primary instance.
 *
 * Concretely, [func@Bobgui.init] is called in the default handler for the
 * `GApplication::startup` signal. Therefore, `BobguiApplication` subclasses
 * should always chain up in their [vfunc@GIO.Application.startup] handler
 * before using any BOBGUI API.
 *
 * Note that commandline arguments are not passed to [func@Bobgui.init].
 *
 * If `application_id` is not `NULL`, then it must be valid. See
 * [func@Gio.Application.id_is_valid].
 *
 * If no application ID is given then some features (most notably application
 * uniqueness) will be disabled.
 *
 * Returns: a new `BobguiApplication` instance
 */
BobguiApplication *
bobgui_application_new (const char        *application_id,
                     GApplicationFlags  flags)
{
  g_return_val_if_fail (application_id == NULL || g_application_id_is_valid (application_id), NULL);

  return g_object_new (BOBGUI_TYPE_APPLICATION,
                       "application-id", application_id,
                       "flags", flags,
                       NULL);
}

/**
 * bobgui_application_add_window:
 * @application: an application
 * @window: a window
 *
 * Adds a window to the application.
 *
 * This call can only happen after the application has started;
 * typically, you should add new application windows in response
 * to the emission of the [signal@GIO.Application::activate] signal.
 *
 * This call is equivalent to setting the [property@Bobgui.Window:application]
 * property of the window to @application.
 *
 * Normally, the connection between the application and the window
 * will remain until the window is destroyed, but you can explicitly
 * remove it with [method@Bobgui.Application.remove_window].
 *
 * BOBGUI will keep the application running as long as it has any windows.
 **/
void
bobgui_application_add_window (BobguiApplication *application,
                            BobguiWindow      *window)
{
  BobguiApplicationPrivate *priv = bobgui_application_get_instance_private (application);

  g_return_if_fail (BOBGUI_IS_APPLICATION (application));
  g_return_if_fail (BOBGUI_IS_WINDOW (window));

  if (!g_application_get_is_registered (G_APPLICATION (application)))
    {
      g_critical ("New application windows must be added after the "
                  "GApplication::startup signal has been emitted.");
      return;
    }

  if (!g_list_find (priv->windows, window))
    g_signal_emit (application,
                   bobgui_application_signals[WINDOW_ADDED], 0, window);
}

/**
 * bobgui_application_remove_window:
 * @application: an application
 * @window: a window
 *
 * Remove a window from the application.
 *
 * If the window belongs to the application then this call is
 * equivalent to setting the [property@Bobgui.Window:application]
 * property of the window to `NULL`.
 *
 * The application may stop running as a result of a call to this
 * function, if the window was the last window of the application.
 **/
void
bobgui_application_remove_window (BobguiApplication *application,
                               BobguiWindow      *window)
{
  BobguiApplicationPrivate *priv = bobgui_application_get_instance_private (application);

  g_return_if_fail (BOBGUI_IS_APPLICATION (application));
  g_return_if_fail (BOBGUI_IS_WINDOW (window));

  if (g_list_find (priv->windows, window))
    g_signal_emit (application,
                   bobgui_application_signals[WINDOW_REMOVED], 0, window);
}

/**
 * bobgui_application_get_windows:
 * @application: an application
 *
 * Gets a list of the window associated with the application.
 *
 * The list is sorted by most recently focused window, such that the first
 * element is the currently focused window. (Useful for choosing a parent
 * for a transient window.)
 *
 * The list that is returned should not be modified in any way. It will
 * only remain valid until the next focus change or window creation or
 * deletion.
 *
 * Returns: (element-type BobguiWindow) (transfer none): the list of windows
 **/
GList *
bobgui_application_get_windows (BobguiApplication *application)
{
  BobguiApplicationPrivate *priv = bobgui_application_get_instance_private (application);

  g_return_val_if_fail (BOBGUI_IS_APPLICATION (application), NULL);

  return priv->windows;
}

/**
 * bobgui_application_get_window_by_id:
 * @application: an application`
 * @id: an identifier number
 *
 * Returns the window with the given ID.
 *
 * The ID of a `BobguiApplicationWindow` can be retrieved with
 * [method@Bobgui.ApplicationWindow.get_id].
 *
 * Returns: (nullable) (transfer none): the window for the given ID
 */
BobguiWindow *
bobgui_application_get_window_by_id (BobguiApplication *application,
                                  guint           id)
{
  BobguiApplicationPrivate *priv = bobgui_application_get_instance_private (application);
  GList *l;

  g_return_val_if_fail (BOBGUI_IS_APPLICATION (application), NULL);

  for (l = priv->windows; l != NULL; l = l->next)
    {
      if (BOBGUI_IS_APPLICATION_WINDOW (l->data) &&
          bobgui_application_window_get_id (BOBGUI_APPLICATION_WINDOW (l->data)) == id)
        return l->data;
    }

  return NULL;
}

/**
 * bobgui_application_get_active_window:
 * @application: an application
 *
 * Gets the “active” window for the application.
 *
 * The active window is the one that was most recently focused
 * (within the application). This window may not have the focus
 * at the moment if another application has it — this is just
 * the most recently-focused window within this application.
 *
 * Returns: (transfer none) (nullable): the active window
 **/
BobguiWindow *
bobgui_application_get_active_window (BobguiApplication *application)
{
  BobguiApplicationPrivate *priv = bobgui_application_get_instance_private (application);

  g_return_val_if_fail (BOBGUI_IS_APPLICATION (application), NULL);

  return priv->windows ? priv->windows->data : NULL;
}

static void
bobgui_application_update_accels (BobguiApplication *application)
{
  BobguiApplicationPrivate *priv = bobgui_application_get_instance_private (application);
  GList *l;

  for (l = priv->windows; l != NULL; l = l->next)
    _bobgui_window_notify_keys_changed (l->data);
}

/**
 * bobgui_application_set_menubar:
 * @application: an application
 * @menubar: (nullable): a menu model
 *
 * Sets or unsets the menubar for windows of the application.
 *
 * This is a menubar in the traditional sense.
 *
 * This can only be done in the primary instance of the application,
 * after it has been registered. [vfunc@GIO.Application.startup] is
 * a good place to call this.
 *
 * Depending on the desktop environment, this may appear at the top of
 * each window, or at the top of the screen. In some environments, if
 * both the application menu and the menubar are set, the application
 * menu will be presented as if it were the first item of the menubar.
 * Other environments treat the two as completely separate — for example,
 * the application menu may be rendered by the desktop shell while the
 * menubar (if set) remains in each individual window.
 *
 * Use the base `GActionMap` interface to add actions, to respond to the
 * user selecting these menu items.
 */
void
bobgui_application_set_menubar (BobguiApplication *application,
                             GMenuModel     *menubar)
{
  BobguiApplicationPrivate *priv = bobgui_application_get_instance_private (application);

  g_return_if_fail (BOBGUI_IS_APPLICATION (application));
  g_return_if_fail (g_application_get_is_registered (G_APPLICATION (application)));
  g_return_if_fail (!g_application_get_is_remote (G_APPLICATION (application)));
  g_return_if_fail (menubar == NULL || G_IS_MENU_MODEL (menubar));

  if (g_set_object (&priv->menubar, menubar))
    {
      bobgui_application_impl_set_menubar (priv->impl, menubar);

      g_object_notify_by_pspec (G_OBJECT (application), bobgui_application_props[PROP_MENUBAR]);
    }
}

/**
 * bobgui_application_get_menubar:
 * @application: an application
 *
 * Returns the menu model for the menu bar of the application.
 *
 * Returns: (nullable) (transfer none): the menubar for windows of the application
 */
GMenuModel *
bobgui_application_get_menubar (BobguiApplication *application)
{
  BobguiApplicationPrivate *priv = bobgui_application_get_instance_private (application);

  g_return_val_if_fail (BOBGUI_IS_APPLICATION (application), NULL);

  return priv->menubar;
}

/**
 * BobguiApplicationInhibitFlags:
 * @BOBGUI_APPLICATION_INHIBIT_LOGOUT: Inhibit ending the user session
 *   by logging out or by shutting down the computer
 * @BOBGUI_APPLICATION_INHIBIT_SWITCH: Inhibit user switching
 * @BOBGUI_APPLICATION_INHIBIT_SUSPEND: Inhibit suspending the
 *   session or computer
 * @BOBGUI_APPLICATION_INHIBIT_IDLE: Inhibit the session being
 *   marked as idle (and possibly locked)
 *
 * Types of user actions that may be blocked by `BobguiApplication`.
 *
 * See [method@Bobgui.Application.inhibit].
 */

/**
 * bobgui_application_inhibit:
 * @application: the application
 * @window: (nullable): a window
 * @flags: what types of actions should be inhibited
 * @reason: (nullable): a short, human-readable string that explains
 *   why these operations are inhibited
 *
 * Informs the session manager that certain types of actions should be
 * inhibited.
 *
 * This is not guaranteed to work on all platforms and for all types of
 * actions.
 *
 * Applications should invoke this method when they begin an operation
 * that should not be interrupted, such as creating a CD or DVD. The
 * types of actions that may be blocked are specified by the @flags
 * parameter. When the application completes the operation it should
 * call [method@Bobgui.Application.uninhibit] to remove the inhibitor. Note
 * that an application can have multiple inhibitors, and all of them must
 * be individually removed. Inhibitors are also cleared when the
 * application exits.
 *
 * Applications should not expect that they will always be able to block
 * the action. In most cases, users will be given the option to force
 * the action to take place.
 *
 * The @reason message should be short and to the point.
 *
 * If a window is given, the session manager may point the user to
 * this window to find out more about why the action is inhibited.
 *
 * The cookie that is returned by this function  should be used as an
 * argument to [method@Bobgui.Application.uninhibit] in order to remove
 * the request.
 *
 * Returns: A non-zero cookie that is used to uniquely identify this, or
 *   0 if the platform does not support inhibiting or the request failed
 *   for some reason
 */
guint
bobgui_application_inhibit (BobguiApplication             *application,
                         BobguiWindow                  *window,
                         BobguiApplicationInhibitFlags  flags,
                         const char                 *reason)
{
  BobguiApplicationPrivate *priv = bobgui_application_get_instance_private (application);

  g_return_val_if_fail (BOBGUI_IS_APPLICATION (application), 0);
  g_return_val_if_fail (!g_application_get_is_remote (G_APPLICATION (application)), 0);
  g_return_val_if_fail (window == NULL || BOBGUI_IS_WINDOW (window), 0);

  return bobgui_application_impl_inhibit (priv->impl, window, flags, reason);
}

/**
 * bobgui_application_uninhibit:
 * @application: the application
 * @cookie: a cookie that was returned by [method@Bobgui.Application.inhibit]
 *
 * Removes an inhibitor that has been previously established.
 *
 * See [method@Bobgui.Application.inhibit].
 *
 * Inhibitors are also cleared when the application exits.
 */
void
bobgui_application_uninhibit (BobguiApplication *application,
                           guint           cookie)
{
  BobguiApplicationPrivate *priv = bobgui_application_get_instance_private (application);

  g_return_if_fail (BOBGUI_IS_APPLICATION (application));
  g_return_if_fail (!g_application_get_is_remote (G_APPLICATION (application)));
  g_return_if_fail (cookie > 0);

  bobgui_application_impl_uninhibit (priv->impl, cookie);
}

BobguiActionMuxer *
bobgui_application_get_parent_muxer_for_window (BobguiWindow *window)
{
  BobguiApplication *application = bobgui_window_get_application (window);
  BobguiApplicationPrivate *priv;

  if (!application)
    return NULL;

  priv = bobgui_application_get_instance_private (application);

  return priv->muxer;
}

BobguiApplicationAccels *
bobgui_application_get_application_accels (BobguiApplication *application)
{
  BobguiApplicationPrivate *priv = bobgui_application_get_instance_private (application);

  return priv->accels;
}

/**
 * bobgui_application_list_action_descriptions:
 * @application: an application
 *
 * Lists the detailed action names which have associated accelerators.
 *
 * See [method@Bobgui.Application.set_accels_for_action].
 *
 * Returns: (transfer full) (array zero-terminated=1): the detailed action names
 */
char **
bobgui_application_list_action_descriptions (BobguiApplication *application)
{
  BobguiApplicationPrivate *priv = bobgui_application_get_instance_private (application);

  g_return_val_if_fail (BOBGUI_IS_APPLICATION (application), NULL);

  return bobgui_application_accels_list_action_descriptions (priv->accels);
}

/**
 * bobgui_application_set_accels_for_action:
 * @application: an application
 * @detailed_action_name: a detailed action name, specifying an action
 *   and target to associate accelerators with
 * @accels: (array zero-terminated=1): a list of accelerators in the format
 *   understood by [func@Bobgui.accelerator_parse]
 *
 * Sets zero or more keyboard accelerators that will trigger the
 * given action.
 *
 * The first item in @accels will be the primary accelerator,
 * which may be displayed in the UI.
 *
 * To remove all accelerators for an action, use an empty,
 * zero-terminated array for @accels.
 *
 * For the @detailed_action_name, see [func@Gio.Action.parse_detailed_name]
 * and [Gio.Action.print_detailed_name].
 */
void
bobgui_application_set_accels_for_action (BobguiApplication      *application,
                                       const char          *detailed_action_name,
                                       const char * const *accels)
{
  BobguiApplicationPrivate *priv = bobgui_application_get_instance_private (application);
  char *action_and_target;

  g_return_if_fail (BOBGUI_IS_APPLICATION (application));
  g_return_if_fail (detailed_action_name != NULL);
  g_return_if_fail (accels != NULL);

  bobgui_application_accels_set_accels_for_action (priv->accels,
                                                detailed_action_name,
                                                accels);

  action_and_target = bobgui_normalise_detailed_action_name (detailed_action_name);
  bobgui_action_muxer_set_primary_accel (priv->muxer, action_and_target, accels[0]);
  g_free (action_and_target);

  bobgui_application_update_accels (application);
}

/**
 * bobgui_application_get_accels_for_action:
 * @application: an application
 * @detailed_action_name: a detailed action name, specifying an action
 *   and target to obtain accelerators for
 *
 * Gets the accelerators that are currently associated with
 * the given action.
 *
 * Returns: (transfer full) (array zero-terminated=1) (element-type utf8):
 *   accelerators for @detailed_action_name
 */
char **
bobgui_application_get_accels_for_action (BobguiApplication *application,
                                       const char     *detailed_action_name)
{
  BobguiApplicationPrivate *priv = bobgui_application_get_instance_private (application);

  g_return_val_if_fail (BOBGUI_IS_APPLICATION (application), NULL);
  g_return_val_if_fail (detailed_action_name != NULL, NULL);

  return bobgui_application_accels_get_accels_for_action (priv->accels,
                                                       detailed_action_name);
}

/**
 * bobgui_application_get_actions_for_accel:
 * @application: a application
 * @accel: an accelerator that can be parsed by [func@Bobgui.accelerator_parse]
 *
 * Returns the list of actions (possibly empty) that the accelerator maps to.
 *
 * Each item in the list is a detailed action name in the usual form.
 *
 * This might be useful to discover if an accel already exists in
 * order to prevent installation of a conflicting accelerator (from
 * an accelerator editor or a plugin system, for example). Note that
 * having more than one action per accelerator may not be a bad thing
 * and might make sense in cases where the actions never appear in the
 * same context.
 *
 * In case there are no actions for a given accelerator, an empty array
 * is returned. `NULL` is never returned.
 *
 * It is a programmer error to pass an invalid accelerator string.
 *
 * If you are unsure, check it with [func@Bobgui.accelerator_parse] first.
 *
 * Returns: (transfer full): actions for @accel
 */
char **
bobgui_application_get_actions_for_accel (BobguiApplication *application,
                                       const char     *accel)
{
  BobguiApplicationPrivate *priv = bobgui_application_get_instance_private (application);

  g_return_val_if_fail (BOBGUI_IS_APPLICATION (application), NULL);
  g_return_val_if_fail (accel != NULL, NULL);

  return bobgui_application_accels_get_actions_for_accel (priv->accels, accel);
}

BobguiActionMuxer *
bobgui_application_get_action_muxer (BobguiApplication *application)
{
  BobguiApplicationPrivate *priv = bobgui_application_get_instance_private (application);

  g_assert (priv->muxer);

  return priv->muxer;
}

void
bobgui_application_insert_action_group (BobguiApplication *application,
                                     const char     *name,
                                     GActionGroup   *action_group)
{
  BobguiApplicationPrivate *priv = bobgui_application_get_instance_private (application);

  bobgui_action_muxer_insert (priv->muxer, name, action_group);
}

void
bobgui_application_handle_window_realize (BobguiApplication *application,
                                       BobguiWindow      *window)
{
  BobguiApplicationPrivate *priv = bobgui_application_get_instance_private (application);

  if (priv->impl)
    bobgui_application_impl_handle_window_realize (priv->impl, window);
}

void
bobgui_application_handle_window_map (BobguiApplication *application,
                                   BobguiWindow      *window)
{
  BobguiApplicationPrivate *priv = bobgui_application_get_instance_private (application);

  if (priv->impl)
    bobgui_application_impl_handle_window_map (priv->impl, window);
}

/**
 * bobgui_application_get_menu_by_id:
 * @application: an application
 * @id: the ID of the menu to look up
 *
 * Gets a menu from automatically loaded resources.
 *
 * See [the section on Automatic resources](class.Application.html#automatic-resources)
 * for more information.
 *
 * Returns: (nullable) (transfer none): Gets the menu with the
 *   given ID from the automatically loaded resources
 */
GMenu *
bobgui_application_get_menu_by_id (BobguiApplication *application,
                                const char     *id)
{
  BobguiApplicationPrivate *priv = bobgui_application_get_instance_private (application);
  GObject *object;

  g_return_val_if_fail (BOBGUI_IS_APPLICATION (application), NULL);
  g_return_val_if_fail (id != NULL, NULL);

  if (!priv->menus_builder)
    return NULL;

  object = bobgui_builder_get_object (priv->menus_builder, id);

  if (!object || !G_IS_MENU (object))
    return NULL;

  return G_MENU (object);
}

void
bobgui_application_set_screensaver_active (BobguiApplication *application,
                                        gboolean        active)
{
  BobguiApplicationPrivate *priv = bobgui_application_get_instance_private (application);

  if (priv->screensaver_active != active)
    {
      priv->screensaver_active = active;
      g_object_notify (G_OBJECT (application), "screensaver-active");
    }
}

static GVariant *
collect_window_state (BobguiApplication *application,
                      BobguiWindow      *window)
{
  BobguiApplicationPrivate *priv = bobgui_application_get_instance_private (application);
  GVariantBuilder builder;
  GVariantDict *dict;
  GVariant *state;

  g_variant_builder_init (&builder, G_VARIANT_TYPE_VARDICT);
  bobgui_application_impl_collect_window_state (priv->impl, window, &builder);

  dict = g_variant_dict_new (NULL);
  if (BOBGUI_IS_APPLICATION_WINDOW (window))
    bobgui_application_window_save (BOBGUI_APPLICATION_WINDOW (window), dict);

  state = g_variant_new ("(a{sv}@a{sv})", &builder, g_variant_dict_end (dict));
  g_variant_dict_unref (dict);

  g_variant_ref_sink (state);
  return state;
}

/* State saving.
 *
 * The state is stored in a GVariant of the following form:
 *
 * (a{sv}a{sv}a(a{sv}a{sv}))
 *
 *  - the first a{sv} contains global BOBGUI state
 *  - the second a{sv} contains global application state
 *  - the last array contains per-window state. For each
 *    window there is a tuple of per-window BOBGUI state and
 *    application state
 *
 *  Global BOBGUI state is applied during startup (since it contains
 *  IDs that are needed for session registration). All other state
 *  is applied during activate.
 */
static GVariant *
collect_state (BobguiApplication *application)
{
  BobguiApplicationPrivate *priv = bobgui_application_get_instance_private (application);
  GVariantBuilder win_builder;
  GVariant *state;
  GVariantBuilder global_builder;
  GVariantDict *global_dict;
  gboolean handled;

  g_variant_builder_init (&win_builder, G_VARIANT_TYPE ("a(a{sv}a{sv})"));

  if (priv->kept_window_state)
    {
      BOBGUI_DEBUG (SESSION, "Using state of kept last window");
      g_variant_builder_add_value (&win_builder, priv->kept_window_state);
    }
  else
    {
      BOBGUI_DEBUG (SESSION, "Collecting state for %d windows", g_list_length (priv->windows));

      for (GList *l = priv->windows; l != NULL; l = l->next)
        {
          BobguiWindow *window = BOBGUI_WINDOW (l->data);
          GVariant *win_state;

          win_state = collect_window_state (application, window);
          g_variant_builder_add_value (&win_builder, win_state);

          g_variant_unref (win_state);
        }
    }

  g_variant_builder_init (&global_builder, G_VARIANT_TYPE_VARDICT);
  bobgui_application_impl_collect_global_state (priv->impl, &global_builder);

  global_dict = g_variant_dict_new (NULL);
  g_signal_emit (application, bobgui_application_signals[SAVE_STATE], 0, global_dict, &handled);

  state = g_variant_new ("(a{sv}@a{sv}a(a{sv}a{sv}))",
                         &global_builder,
                         g_variant_dict_end (global_dict),
                         &win_builder);

  g_variant_dict_unref (global_dict);

  g_variant_ref_sink (state);

  return state;
}

/**
 * bobgui_application_save:
 * @application: a `BobguiApplication`
 *
 * Saves the state of application.
 *
 * See [method@Bobgui.Application.forget] for a way to forget the state.
 *
 * If [property@Bobgui.Application:register-session] is set, `BobguiApplication`
 * calls this function automatically when the application is closed or
 * the session ends.
 *
 * Since: 4.24
 */
void
bobgui_application_save (BobguiApplication *application)
{
  BobguiApplicationPrivate *priv = bobgui_application_get_instance_private (application);
  GVariant *state;

  g_return_if_fail (BOBGUI_IS_APPLICATION (application));

  if (!priv->support_save)
    return;

  state = collect_state (application);
  bobgui_application_impl_store_state (priv->impl, state);
  g_variant_unref (state);

  priv->forgotten = FALSE;
  schedule_autosave (application);
}

/**
 * bobgui_application_forget:
 * @application: a `BobguiApplication`
 *
 * Forget state that has been previously saved and prevent
 * further automatic state saving.
 *
 * In order to reenable state saving, call
 * [method@Bobgui.Application.save].
 *
 * Since: 4.24
 */
void
bobgui_application_forget (BobguiApplication *application)
{
  BobguiApplicationPrivate *priv = bobgui_application_get_instance_private (application);

  if (!priv->support_save)
    return;

  if (priv->kept_window_state)
    {
      // TODO: Tell compositor to forget the window state
      //       (currently impossible due to https://gitlab.freedesktop.org/wayland/wayland-protocols/-/merge_requests/18#note_3171587)
      g_clear_pointer (&priv->kept_window_state, g_variant_unref);
    }

  for (GList *l = priv->windows; l != NULL; l = l->next)
    {
      BobguiWindow *window = BOBGUI_WINDOW (l->data);
      bobgui_application_impl_window_forget (priv->impl, window);
    }

  bobgui_application_impl_forget_state (priv->impl);

  if (priv->autosave_id)
    BOBGUI_DEBUG (SESSION, "State forgotten, cancelling autosave");
  g_clear_handle_id (&priv->autosave_id, g_source_remove);

  priv->forgotten = TRUE;
}

static void
restore_window (BobguiApplication   *application,
                BobguiRestoreReason  reason,
                GVariant         *app_state,
                GVariant         *bobgui_state)
{
  BobguiApplicationPrivate *priv = bobgui_application_get_instance_private (application);

  priv->pending_window_state = bobgui_state;
  g_signal_emit (application, bobgui_application_signals[RESTORE_WINDOW], 0, reason, app_state);

  if (priv->pending_window_state)
    {
      BOBGUI_DEBUG (SESSION, "App didn't restore a toplevel, removing it from session");
      // TODO: Tell compositor to forget the window state
      // (currently impossible due to https://gitlab.freedesktop.org/wayland/wayland-protocols/-/merge_requests/18#note_3171587)
      priv->pending_window_state = NULL;
    }
}

static void
restore_file_state (BobguiApplication   *application,
               BobguiRestoreReason  reason,
               GVariant         *state)
{
  BobguiApplicationPrivate *priv = bobgui_application_get_instance_private (application);
  GVariant *bobgui_state;
  GVariant *app_state;
  GVariantIter *iter;
  gboolean handled;

  g_return_if_fail (g_variant_is_of_type (state, G_VARIANT_TYPE ("(a{sv}a{sv}a(a{sv}a{sv}))")));

  BOBGUI_DEBUG (SESSION, "Restoring state, reason %s", g_enum_get_value (g_type_class_get (BOBGUI_TYPE_RESTORE_REASON), reason)->value_nick);

  g_variant_get (state, "(@a{sv}@a{sv}a(a{sv}a{sv}))", &bobgui_state, &app_state, NULL);

  bobgui_application_impl_restore_global_state (priv->impl, bobgui_state);

  g_signal_emit (application, bobgui_application_signals[RESTORE_STATE], 0, reason, app_state, &handled);

  g_variant_unref (bobgui_state);
  g_variant_unref (app_state);

  g_variant_get (state, "(a{sv}a{sv}a(a{sv}a{sv}))", NULL, NULL, &iter);

  while (g_variant_iter_next (iter, "(@a{sv}@a{sv})", &bobgui_state, &app_state))
    {
      BOBGUI_DEBUG (SESSION, "Restoring window");

      restore_window (application, reason, app_state, bobgui_state);

      g_variant_unref (bobgui_state);
      g_variant_unref (app_state);
    }

  g_variant_iter_free (iter);
}

static void
restore_kept_state (BobguiApplication   *application,
                    BobguiRestoreReason  reason)
{
  BobguiApplicationPrivate *priv = bobgui_application_get_instance_private (application);
  GVariant *bobgui_state;
  GVariant *app_state;

  BOBGUI_DEBUG (SESSION, "Restoring kept toplevel, reason %s", g_enum_get_value (g_type_class_get (BOBGUI_TYPE_RESTORE_REASON), reason)->value_nick);

  g_variant_get (priv->kept_window_state, "(@a{sv}@a{sv})", &bobgui_state, &app_state);

  restore_window (application, reason, app_state, bobgui_state);

  g_variant_unref (bobgui_state);
  g_variant_unref (app_state);
}

static gboolean
bobgui_application_restore (BobguiApplication   *application)
{
  BobguiApplicationPrivate *priv = bobgui_application_get_instance_private (application);
  BobguiRestoreReason reason;
  GVariant *state;

  reason = bobgui_application_impl_get_restore_reason (priv->impl);

  if (reason == BOBGUI_RESTORE_REASON_PRISTINE)
    {
      BOBGUI_DEBUG (SESSION, "Pristine start, not restoring state");
      return FALSE;
    }

  if (priv->kept_window_state)
    {
      restore_kept_state (application, reason);
      return TRUE;
    }

  state = bobgui_application_impl_retrieve_state (priv->impl);
  if (state)
    {
      restore_file_state (application, reason, state);
      g_variant_unref (state);
      return TRUE;
    }
  else
    {
      BOBGUI_DEBUG (SESSION, "No saved state, not restoring");
      return FALSE;
    }
}

static gboolean
any_window_active (BobguiApplication *application)
{
  BobguiApplicationPrivate *priv = bobgui_application_get_instance_private (application);

  for (GList *l = priv->windows; l != NULL; l = l->next)
    {
      BobguiWindow *candidate = BOBGUI_WINDOW (l->data);
      if (bobgui_window_is_active (candidate))
        return TRUE;
    }

    return FALSE;
}

static gboolean
autosave_cb (gpointer data)
{
  BobguiApplication *application = BOBGUI_APPLICATION (data);
  BobguiApplicationPrivate *priv = bobgui_application_get_instance_private (application);

  BOBGUI_DEBUG (SESSION, "Autosaving");
  bobgui_application_save (application);

  if (!any_window_active (application))
    {
      BOBGUI_DEBUG (SESSION, "App no longer focused, stopping autosave");
      priv->autosave_id = 0;
      return G_SOURCE_REMOVE;
    }

  return G_SOURCE_CONTINUE;
}

static void
schedule_autosave (BobguiApplication *application)
{
  BobguiApplicationPrivate *priv = bobgui_application_get_instance_private (application);

  if (!priv->support_save)
    return;

  if (priv->forgotten)
    return;

  if (priv->autosave_interval == 0)
    return;

  if (priv->autosave_id != 0)
    return;

  if (!any_window_active (application))
    return;

  BOBGUI_DEBUG (SESSION, "Scheduling autosave");
  priv->autosave_id = g_timeout_add_seconds (priv->autosave_interval, autosave_cb, application);
}
