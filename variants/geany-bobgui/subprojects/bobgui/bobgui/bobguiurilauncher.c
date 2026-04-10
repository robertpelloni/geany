/*
 * BOBGUI - The Bobgui Framework
 * Copyright (C) 2022 Red Hat, Inc.
 * All rights reserved.
 *
 * This Library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include "bobguiurilauncher.h"

#include "bobguidialogerror.h"
#include "bobguiopenuriportal.h"
#include "deprecated/bobguishow.h"
#include "bobguiprivate.h"
#include <glib/gi18n-lib.h>

/**
 * BobguiUriLauncher:
 *
 * Asynchronous API to open a uri with an application.
 *
 * `BobguiUriLauncher` collects the arguments that are needed to open the uri.
 *
 * Depending on system configuration, user preferences and available APIs, this
 * may or may not show an app chooser dialog or launch the default application
 * right away.
 *
 * The operation is started with the [method@Bobgui.UriLauncher.launch] function.
 *
 * To launch a file, use [class@Bobgui.FileLauncher].
 *
 * Since: 4.10
 */

/* {{{ GObject implementation */

struct _BobguiUriLauncher
{
  GObject parent_instance;

  char *uri;
};

enum {
  PROP_URI = 1,

  NUM_PROPERTIES
};

static GParamSpec *properties[NUM_PROPERTIES];

G_DEFINE_TYPE (BobguiUriLauncher, bobgui_uri_launcher, G_TYPE_OBJECT)

static void
bobgui_uri_launcher_init (BobguiUriLauncher *self)
{
}

static void
bobgui_uri_launcher_finalize (GObject *object)
{
  BobguiUriLauncher *self = BOBGUI_URI_LAUNCHER (object);

  g_free (self->uri);

  G_OBJECT_CLASS (bobgui_uri_launcher_parent_class)->finalize (object);
}

static void
bobgui_uri_launcher_get_property (GObject      *object,
                               unsigned int  property_id,
                               GValue       *value,
                               GParamSpec   *pspec)
{
  BobguiUriLauncher *self = BOBGUI_URI_LAUNCHER (object);

  switch (property_id)
    {
    case PROP_URI:
      g_value_set_string (value, self->uri);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
bobgui_uri_launcher_set_property (GObject      *object,
                                unsigned int  property_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  BobguiUriLauncher *self = BOBGUI_URI_LAUNCHER (object);

  switch (property_id)
    {
    case PROP_URI:
      bobgui_uri_launcher_set_uri (self, g_value_get_string (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
bobgui_uri_launcher_class_init (BobguiUriLauncherClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  object_class->finalize = bobgui_uri_launcher_finalize;
  object_class->get_property = bobgui_uri_launcher_get_property;
  object_class->set_property = bobgui_uri_launcher_set_property;

  /**
   * BobguiUriLauncher:uri:
   *
   * The uri to launch.
   *
   * Since: 4.10
   */
  properties[PROP_URI] =
      g_param_spec_string ("uri", NULL, NULL,
                           NULL,
                           G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS|G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, NUM_PROPERTIES, properties);
}

/* }}} */
/* {{{ API: Constructor */

/**
 * bobgui_uri_launcher_new:
 * @uri: (nullable): the uri to open
 *
 * Creates a new `BobguiUriLauncher` object.
 *
 * Returns: the new `BobguiUriLauncher`
 *
 * Since: 4.10
 */
BobguiUriLauncher *
bobgui_uri_launcher_new (const char *uri)
{
  return g_object_new (BOBGUI_TYPE_URI_LAUNCHER,
                       "uri", uri,
                       NULL);
}

 /* }}} */
/* {{{ API: Getters and setters */

/**
 * bobgui_uri_launcher_get_uri:
 * @self: an uri launcher
 *
 * Gets the uri that will be opened.
 *
 * Returns: (transfer none) (nullable): the uri
 *
 * Since: 4.10
 */
const char *
bobgui_uri_launcher_get_uri (BobguiUriLauncher *self)
{
  g_return_val_if_fail (BOBGUI_IS_URI_LAUNCHER (self), NULL);

  return self->uri;
}

/**
 * bobgui_uri_launcher_set_uri:
 * @self: an uri launcher
 * @uri: (nullable): the uri
 *
 * Sets the uri that will be opened.
 *
 * Since: 4.10
 */
void
bobgui_uri_launcher_set_uri (BobguiUriLauncher *self,
                          const char     *uri)
{
  g_return_if_fail (BOBGUI_IS_URI_LAUNCHER (self));

  if (g_strcmp0 (self->uri, uri) == 0)
    return;

  g_free (self->uri);
  self->uri = g_strdup (uri);

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_URI]);
}

/* }}} */
/* {{{ Async implementation */

#ifndef G_OS_WIN32
static void
open_done (GObject      *source,
           GAsyncResult *result,
           gpointer      data)
{
  GTask *task = G_TASK (data);
  GError *error = NULL;

  if (!bobgui_openuri_portal_open_uri_finish (result, &error))
    g_task_return_error (task, error);
  else
    g_task_return_boolean (task, TRUE);

  g_object_unref (task);
}
#endif

G_GNUC_BEGIN_IGNORE_DEPRECATIONS
static void
show_uri_done (GObject      *source,
               GAsyncResult *result,
               gpointer      data)
{
  BobguiWindow *parent = BOBGUI_WINDOW (source);
  GTask *task = G_TASK (data);
  GError *error = NULL;

  if (!bobgui_show_uri_full_finish (parent, result, &error))
    {
      if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
        g_task_return_new_error (task, BOBGUI_DIALOG_ERROR, BOBGUI_DIALOG_ERROR_CANCELLED, "Cancelled by user");
      else
        g_task_return_new_error (task, BOBGUI_DIALOG_ERROR, BOBGUI_DIALOG_ERROR_FAILED, "%s", error->message);
      g_error_free (error);
    }
  else
    g_task_return_boolean (task, TRUE);

  g_object_unref (task);
}

static gboolean
bobgui_can_show_uri (const char *uri)
{
  const char *scheme;
  GAppInfo *app_info;

  scheme = g_uri_peek_scheme (uri);
  if (!scheme)
    return FALSE;

  app_info = g_app_info_get_default_for_uri_scheme (scheme);

  if (!app_info)
    {
      GFile *file = g_file_new_for_uri (uri);
      app_info = g_file_query_default_handler (file, NULL, NULL);
      g_object_unref (file);
    }

  if (app_info)
    {
      g_object_unref (app_info);
      return TRUE;
    }

  return FALSE;
}

G_GNUC_END_IGNORE_DEPRECATIONS

  /* }}} */
/* {{{ Async API */

/**
 * bobgui_uri_launcher_launch:
 * @self: an uri launcher
 * @parent: (nullable): the parent window
 * @cancellable: (nullable): a cancellable to cancel the operation
 * @callback: (scope async) (closure user_data): a callback to call when the
 *   operation is complete
 * @user_data: data to pass to @callback
 *
 * Launches an application to open the uri.
 *
 * This may present an app chooser dialog to the user.
 *
 * Since: 4.10
 */
void
bobgui_uri_launcher_launch (BobguiUriLauncher      *self,
                         BobguiWindow           *parent,
                         GCancellable        *cancellable,
                         GAsyncReadyCallback  callback,
                         gpointer             user_data)
{
  GTask *task;
#ifndef G_OS_WIN32
  GdkDisplay *display;
#endif
  GError *error = NULL;

  g_return_if_fail (BOBGUI_IS_URI_LAUNCHER (self));

  task = g_task_new (self, cancellable, callback, user_data);
  g_task_set_check_cancellable (task, FALSE);
  g_task_set_source_tag (task, bobgui_uri_launcher_launch);

  if (self->uri == NULL)
    {
      g_task_return_new_error (task,
                               BOBGUI_DIALOG_ERROR, BOBGUI_DIALOG_ERROR_FAILED,
                               "No uri to launch");
      g_object_unref (task);
      return;
    }

  if (!g_uri_is_valid (self->uri, G_URI_FLAGS_NONE, &error))
    {
      g_task_return_new_error (task,
                               BOBGUI_DIALOG_ERROR, BOBGUI_DIALOG_ERROR_FAILED,
                               "%s is not a valid uri: %s", self->uri, error->message);
      g_error_free (error);
      g_object_unref (task);
      return;
    }

#ifndef G_OS_WIN32
  if (parent)
    display = bobgui_widget_get_display (BOBGUI_WIDGET (parent));
  else
    display = gdk_display_get_default ();

  if (gdk_display_should_use_portal (display, PORTAL_OPENURI_INTERFACE, 3))
    {
      bobgui_openuri_portal_open_uri_async (self->uri, parent, cancellable, open_done, task);
    }
  else
#endif
    {
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
      bobgui_show_uri_full (parent, self->uri, GDK_CURRENT_TIME, cancellable, show_uri_done, task);
G_GNUC_END_IGNORE_DEPRECATIONS
    }
}

/**
 * bobgui_uri_launcher_launch_finish:
 * @self: an uri launcher
 * @result: the result
 * @error: return location for a [enum@Bobgui.DialogError] or [enum@Gio.Error] error
 *
 * Finishes the [method@Bobgui.UriLauncher.launch] call and
 * returns the result.
 *
 * Returns: true if an application was launched
 *
 * Since: 4.10
 */
gboolean
bobgui_uri_launcher_launch_finish (BobguiUriLauncher  *self,
                                GAsyncResult    *result,
                                GError         **error)
{
  g_return_val_if_fail (BOBGUI_IS_URI_LAUNCHER (self), FALSE);
  g_return_val_if_fail (g_task_is_valid (result, self), FALSE);
  g_return_val_if_fail (g_task_get_source_tag (G_TASK (result)) == bobgui_uri_launcher_launch, FALSE);

  return g_task_propagate_boolean (G_TASK (result), error);
}

/* }}}  */
/* {{{ Misc API */

/**
 * bobgui_uri_launcher_can_launch:
 * @self: an uri launcher
 * @parent: (nullable): the parent window
 *
 * Returns whether the launcher is likely to succeed
 * in launching an application for its uri.
 *
 * This can be used to disable controls that trigger
 * the launcher when they are known not to work.
 *
 * Returns: false if the launcher is known not to support
 *   the uri, true otherwise
 *
 * Since: 4.20
 */
gboolean
bobgui_uri_launcher_can_launch (BobguiUriLauncher *self,
                             BobguiWindow      *parent)
{
#ifndef G_OS_WIN32
  GdkDisplay *display;
#endif

  if (self->uri == NULL)
    return FALSE;

  if (!g_uri_is_valid (self->uri, G_URI_FLAGS_NONE, NULL))
    return FALSE;

#ifndef G_OS_WIN32
  if (parent)
    display = bobgui_widget_get_display (BOBGUI_WIDGET (parent));
  else
    display = gdk_display_get_default ();

  if (gdk_display_should_use_portal (display, PORTAL_OPENURI_INTERFACE, 3))
    return bobgui_openuri_portal_can_open (self->uri);
#endif

  return bobgui_can_show_uri (self->uri);
}

/* }}} */

/* vim:set foldmethod=marker: */
