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

#include "bobguifilelauncher.h"

#include "bobguidialogerror.h"
#include "bobguiopenuriportal.h"
#include "deprecated/bobguishow.h"
#include "bobguiprivate.h"
#include <glib/gi18n-lib.h>

#ifdef G_OS_WIN32
#include "bobguishowwin32.h"
#endif

#ifdef GDK_WINDOWING_ANDROID
#include "bobguinative.h"

#include "android/gdkandroidcontentfile.h"
#include "android/gdkandroidtoplevel.h"

#include "android/gdkandroidinit-private.h"
#include "android/gdkandroidutils-private.h"
#endif // GDK_WINDOWING_ANDROID

/**
 * BobguiFileLauncher:
 *
 * Asynchronous API to open a file with an application.
 *
 * `BobguiFileLauncher` collects the arguments that are needed to open the file.
 *
 * Depending on system configuration, user preferences and available APIs, this
 * may or may not show an app chooser dialog or launch the default application
 * right away.
 *
 * The operation is started with the [method@Bobgui.FileLauncher.launch] function.
 *
 * To launch uris that don't represent files, use [class@Bobgui.UriLauncher].
 *
 * Since: 4.10
 */

/* {{{ GObject implementation */

struct _BobguiFileLauncher
{
  GObject parent_instance;

  GFile *file;
  unsigned int always_ask : 1;
  unsigned int writable   : 1;
};

enum {
  PROP_FILE = 1,
  PROP_ALWAYS_ASK,
  PROP_WRITABLE,

  NUM_PROPERTIES
};

static GParamSpec *properties[NUM_PROPERTIES];

G_DEFINE_TYPE (BobguiFileLauncher, bobgui_file_launcher, G_TYPE_OBJECT)

static void
bobgui_file_launcher_init (BobguiFileLauncher *self)
{
}

static void
bobgui_file_launcher_finalize (GObject *object)
{
  BobguiFileLauncher *self = BOBGUI_FILE_LAUNCHER (object);

  g_clear_object (&self->file);

  G_OBJECT_CLASS (bobgui_file_launcher_parent_class)->finalize (object);
}

static void
bobgui_file_launcher_get_property (GObject      *object,
                                unsigned int  property_id,
                                GValue       *value,
                                GParamSpec   *pspec)
{
  BobguiFileLauncher *self = BOBGUI_FILE_LAUNCHER (object);

  switch (property_id)
    {
    case PROP_FILE:
      g_value_set_object (value, self->file);
      break;

    case PROP_ALWAYS_ASK:
      g_value_set_boolean (value, self->always_ask);
      break;

    case PROP_WRITABLE:
      g_value_set_boolean (value, self->writable);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
bobgui_file_launcher_set_property (GObject      *object,
                                unsigned int  property_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  BobguiFileLauncher *self = BOBGUI_FILE_LAUNCHER (object);

  switch (property_id)
    {
    case PROP_FILE:
      bobgui_file_launcher_set_file (self, g_value_get_object (value));
      break;

    case PROP_ALWAYS_ASK:
      bobgui_file_launcher_set_always_ask (self, g_value_get_boolean (value));
      break;

    case PROP_WRITABLE:
      bobgui_file_launcher_set_writable (self, g_value_get_boolean (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
bobgui_file_launcher_class_init (BobguiFileLauncherClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  object_class->finalize = bobgui_file_launcher_finalize;
  object_class->get_property = bobgui_file_launcher_get_property;
  object_class->set_property = bobgui_file_launcher_set_property;

  /**
   * BobguiFileLauncher:file:
   *
   * The file to launch.
   *
   * Since: 4.10
   */
  properties[PROP_FILE] =
      g_param_spec_object ("file", NULL, NULL,
                           G_TYPE_FILE,
                           G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiFileLauncher:always-ask:
   *
   * Whether to ask the user to choose an app for opening the file. If `FALSE`,
   * the file might be opened with a default app or the previous choice.
   *
   * Since: 4.12
   */
  properties[PROP_ALWAYS_ASK] =
      g_param_spec_boolean ("always-ask", NULL, NULL,
                            FALSE,
                            G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiFileLauncher:writable:
   *
   * Whether to make the file writable for the handler.
   *
   * Since: 4.14
   */
  properties[PROP_WRITABLE] =
      g_param_spec_boolean ("writable", NULL, NULL,
                            FALSE,
                            G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS|G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, NUM_PROPERTIES, properties);
}

/* }}} */
/* {{{ API: Constructor */

/**
 * bobgui_file_launcher_new:
 * @file: (nullable): the file to open
 *
 * Creates a new `BobguiFileLauncher` object.
 *
 * Returns: the new `BobguiFileLauncher`
 *
 * Since: 4.10
 */
BobguiFileLauncher *
bobgui_file_launcher_new (GFile *file)
{
  return g_object_new (BOBGUI_TYPE_FILE_LAUNCHER,
                       "file", file,
                       NULL);
}

 /* }}} */
/* {{{ API: Getters and setters */

/**
 * bobgui_file_launcher_get_file:
 * @self: a file launcher
 *
 * Gets the file that will be opened.
 *
 * Returns: (transfer none) (nullable): the file
 *
 * Since: 4.10
 */
GFile *
bobgui_file_launcher_get_file (BobguiFileLauncher *self)
{
  g_return_val_if_fail (BOBGUI_IS_FILE_LAUNCHER (self), NULL);

  return self->file;
}

/**
 * bobgui_file_launcher_set_file:
 * @self: a file launcher
 * @file: (nullable): the file
 *
 * Sets the file that will be opened.
 *
 * Since: 4.10
 */
void
bobgui_file_launcher_set_file (BobguiFileLauncher *self,
                            GFile           *file)
{
  g_return_if_fail (BOBGUI_IS_FILE_LAUNCHER (self));
  g_return_if_fail (file == NULL || G_IS_FILE (file));

  if (!g_set_object (&self->file, file))
    return;

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_FILE]);
}

/**
 * bobgui_file_launcher_get_always_ask:
 * @self: a file launcher
 *
 * Returns whether to ask the user which app to use.
 *
 * Returns: true if always asking the user
 *
 * Since: 4.12
 */
gboolean
bobgui_file_launcher_get_always_ask (BobguiFileLauncher *self)
{
  g_return_val_if_fail (BOBGUI_IS_FILE_LAUNCHER (self), FALSE);

  return self->always_ask;
}

/**
 * bobgui_file_launcher_set_always_ask:
 * @self: a file launcher
 * @always_ask: whether to always ask
 *
 * Sets whether to always ask the user which app to use.
 *
 * If false, the file might be opened with a default app
 * or the previous choice.
 *
 * Since: 4.12
 */
void
bobgui_file_launcher_set_always_ask (BobguiFileLauncher *self,
                                  gboolean         always_ask)
{
  g_return_if_fail (BOBGUI_IS_FILE_LAUNCHER (self));

  if (self->always_ask == always_ask)
    return;

  self->always_ask = always_ask;

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_ALWAYS_ASK]);
}

/**
 * bobgui_file_launcher_get_writable:
 * @self: a file launcher
 *
 * Returns whether to make the file writable for the handler.
 *
 * Returns: true if the file will be made writable
 *
 * Since: 4.14
 */
gboolean
bobgui_file_launcher_get_writable (BobguiFileLauncher *self)
{
  g_return_val_if_fail (BOBGUI_IS_FILE_LAUNCHER (self), FALSE);

  return self->writable;
}

/**
 * bobgui_file_launcher_set_writable:
 * @self: a file launcher
 * @writable: whether to make the file writable
 *
 * Sets whether to make the file writable for the handler.
 *
 * Since: 4.14
 */
void
bobgui_file_launcher_set_writable (BobguiFileLauncher *self,
                                gboolean         writable)
{
  g_return_if_fail (BOBGUI_IS_FILE_LAUNCHER (self));

  if (self->writable == writable)
    return;

  self->writable = writable;

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_WRITABLE]);
}

/* }}} */
/* {{{ Async implementation */

#ifndef GDK_WINDOWING_ANDROID
static void
open_done (GObject      *source,
           GAsyncResult *result,
           gpointer      data)
{
  GTask *task = G_TASK (data);
  GError *error = NULL;

#ifndef G_OS_WIN32
  if (!bobgui_openuri_portal_open_finish (result, &error))
#else
  if (!bobgui_open_containing_folder_win32_finish (result, &error))
#endif
    g_task_return_error (task, error);
  else
    g_task_return_boolean (task, TRUE);

  g_object_unref (task);
}

#ifndef G_OS_WIN32
static void
show_item_done (GObject      *source,
                GAsyncResult *result,
                gpointer      data)
{
  GDBusConnection *bus = G_DBUS_CONNECTION (source);
  GTask *task = G_TASK (data);
  GVariant *res;
  GError *error = NULL;

  res = g_dbus_connection_call_finish (bus, result, &error);
  if (res)
    g_variant_unref (res);

  if (error)
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

#define FILE_MANAGER_DBUS_NAME "org.freedesktop.FileManager1"
#define FILE_MANAGER_DBUS_IFACE "org.freedesktop.FileManager1"
#define FILE_MANAGER_DBUS_PATH "/org/freedesktop/FileManager1"

static void
show_item (BobguiWindow    *parent,
           const char   *uri,
           GCancellable *cancellable,
           GTask        *task)
{
  GDBusConnection *bus;
  GVariantBuilder uris_builder = G_VARIANT_BUILDER_INIT (G_VARIANT_TYPE_STRING_ARRAY);

  bus = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, NULL);

  if (!bus)
    {
      g_task_return_new_error (task,
                               BOBGUI_DIALOG_ERROR, BOBGUI_DIALOG_ERROR_FAILED,
                               "Session bus not available");
      g_object_unref (task);
      return;
    }

  g_variant_builder_add (&uris_builder, "s", uri);

  g_dbus_connection_call (bus,
                          FILE_MANAGER_DBUS_NAME,
                          FILE_MANAGER_DBUS_PATH,
                          FILE_MANAGER_DBUS_IFACE,
                          "ShowItems",
                          g_variant_new ("(ass)", &uris_builder, ""),
                          NULL,   /* ignore returned type */
                          G_DBUS_CALL_FLAGS_NONE,
                          -1,
                          cancellable,
                          show_item_done,
                          task);
}
#endif /* G_OS_WIN32 */

G_GNUC_BEGIN_IGNORE_DEPRECATIONS
static void
show_uri_done (GObject      *source,
               GAsyncResult *result,
               gpointer      data)
{
  BobguiWindow *parent = BOBGUI_WINDOW (source);
  GTask *task = G_TASK (data);
  GError *error = NULL;

#ifndef G_OS_WIN32
  if (!bobgui_show_uri_full_finish (parent, result, &error))
#else
  if (!bobgui_show_uri_win32_finish (parent, result, &error))
#endif
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
G_GNUC_END_IGNORE_DEPRECATIONS
#endif // not GDK_WINDOWING_ANDROID

#ifdef GDK_WINDOWING_ANDROID
static gboolean
bobgui_show_file_android (GFile               *file,
                       GdkAndroidToplevel  *toplevel,
                       gboolean             writable,
                       gboolean             always_ask,
                       GError             **error)
{
  JNIEnv *env = gdk_android_get_env ();

  (*env)->PushLocalFrame (env, 7);

  jobject uri;
  if (GDK_IS_ANDROID_CONTENT_FILE (file))
    uri = gdk_android_content_file_get_uri_object ((GdkAndroidContentFile *)file);
  else
    {
      gchar *curi = g_file_get_uri (file);
      uri = (*env)->CallStaticObjectMethod (env, gdk_android_get_java_cache ()->a_uri.klass,
                                            gdk_android_get_java_cache ()->a_uri.parse,
                                            gdk_android_utf8_to_java (uri));
      g_free (curi);
      if (gdk_android_check_exception (error))
        {
          (*env)->PopLocalFrame (env, NULL);
          return FALSE;
        }
    }

  jobject intent = (*env)->NewObject (env, gdk_android_get_java_cache ()->a_intent.klass,
                                      gdk_android_get_java_cache ()->a_intent.constructor_action,
                                      writable ?
                                        gdk_android_get_java_cache ()->a_intent.action_edit :
                                        gdk_android_get_java_cache ()->a_intent.action_view);
  (*env)->CallObjectMethod (env, intent,
                            gdk_android_get_java_cache ()->a_intent.set_data_norm,
                            uri);

  jint flags = gdk_android_get_java_cache ()->a_intent.flag_grant_read_perm;
  if (writable)
    flags |= gdk_android_get_java_cache ()->a_intent.flag_grant_write_perm;
  (*env)->CallObjectMethod (env, intent,
                            gdk_android_get_java_cache ()->a_intent.add_flags,
                            flags);

  if (always_ask)
    intent = (*env)->CallStaticObjectMethod (env, gdk_android_get_java_cache ()->a_intent.klass,
                                             gdk_android_get_java_cache ()->a_intent.create_chooser,
                                             intent, NULL);

  gboolean launched = gdk_android_toplevel_launch_activity (toplevel, intent, error);
  (*env)->PopLocalFrame (env, NULL);
  return launched;
}
#endif // GDK_WINDOWING_ANDROID
 /* }}} */
/* {{{ Async API */

/**
 * bobgui_file_launcher_launch:
 * @self: a file launcher
 * @parent: (nullable): the parent window
 * @cancellable: (nullable): a cancellable to cancel the operation
 * @callback: (scope async) (closure user_data): a callback to call when the
 *   operation is complete
 * @user_data: data to pass to @callback
 *
 * Launches an application to open the file.
 *
 * This may present an app chooser dialog to the user.
 *
 * Since: 4.10
 */
void
bobgui_file_launcher_launch (BobguiFileLauncher     *self,
                          BobguiWindow           *parent,
                          GCancellable        *cancellable,
                          GAsyncReadyCallback  callback,
                          gpointer             user_data)
{
  GTask *task;
#if !defined (G_OS_WIN32) && !defined (GDK_WINDOWING_ANDROID)
  GdkDisplay *display;
#endif

  g_return_if_fail (BOBGUI_IS_FILE_LAUNCHER (self));

  task = g_task_new (self, cancellable, callback, user_data);
  g_task_set_check_cancellable (task, FALSE);
  g_task_set_source_tag (task, bobgui_file_launcher_launch);

  if (self->file == NULL)
    {
      g_task_return_new_error (task,
                               BOBGUI_DIALOG_ERROR, BOBGUI_DIALOG_ERROR_FAILED,
                               "No file to launch");
      g_object_unref (task);
      return;
    }

#if defined (G_OS_WIN32)
  char *path = g_file_get_path (self->file);
  bobgui_show_uri_win32 (parent, path, self->always_ask, cancellable, show_uri_done, task);
  g_free (path);
#elif defined (GDK_WINDOWING_ANDROID)
  GError *err = NULL;
  GdkAndroidToplevel* toplevel = GDK_ANDROID_TOPLEVEL (bobgui_native_get_surface (BOBGUI_NATIVE (parent)));
  if (bobgui_show_file_android (self->file, toplevel, self->writable, self->always_ask, &err))
    g_task_return_boolean (task, TRUE);
  else
    g_task_return_error (task, err);
  g_object_unref (task);
#else
  if (parent)
    display = bobgui_widget_get_display (BOBGUI_WIDGET (parent));
  else
    display = gdk_display_get_default ();

  if (gdk_display_should_use_portal (display, PORTAL_OPENURI_INTERFACE, 3))
    {
      BobguiOpenuriFlags flags = 0;

      if (self->always_ask)
        flags |= BOBGUI_OPENURI_FLAGS_ASK;

      if (self->writable)
        flags |= BOBGUI_OPENURI_FLAGS_WRITABLE;

      bobgui_openuri_portal_open_async (self->file, FALSE, flags, parent, cancellable, open_done, task);
    }
  else
    {
      char *uri = g_file_get_uri (self->file);

G_GNUC_BEGIN_IGNORE_DEPRECATIONS
      bobgui_show_uri_full (parent, uri, GDK_CURRENT_TIME, cancellable, show_uri_done, task);
G_GNUC_END_IGNORE_DEPRECATIONS

      g_free (uri);
    }
#endif
}

/**
 * bobgui_file_launcher_launch_finish:
 * @self: a file launcher
 * @result: the result
 * @error: return location for a [enum@Bobgui.DialogError] or [enum@Gio.Error] error
 *
 * Finishes the [method@Bobgui.FileLauncher.launch] call and
 * returns the result.
 *
 * Returns: true if an application was launched
 *
 * Since: 4.10
 */
gboolean
bobgui_file_launcher_launch_finish (BobguiFileLauncher  *self,
                                 GAsyncResult     *result,
                                 GError          **error)
{
  g_return_val_if_fail (BOBGUI_IS_FILE_LAUNCHER (self), FALSE);
  g_return_val_if_fail (g_task_is_valid (result, self), FALSE);
  g_return_val_if_fail (g_task_get_source_tag (G_TASK (result)) == bobgui_file_launcher_launch, FALSE);

  return g_task_propagate_boolean (G_TASK (result), error);
}

/**
 * bobgui_file_launcher_open_containing_folder:
 * @self: a file launcher
 * @parent: (nullable): the parent window
 * @cancellable: (nullable): a cancellable to cancel the operation
 * @callback: (scope async) (closure user_data): a callback to call when the
 *   operation is complete
 * @user_data: data to pass to @callback
 *
 * Launches a file manager to show the file in its parent directory.
 *
 * This is only supported for native files. It will fail if @file
 * is e.g. a http:// uri.
 *
 * Since: 4.10
 */
void
bobgui_file_launcher_open_containing_folder (BobguiFileLauncher     *self,
                                          BobguiWindow           *parent,
                                          GCancellable        *cancellable,
                                          GAsyncReadyCallback  callback,
                                          gpointer             user_data)
{
  GTask *task;

  g_return_if_fail (BOBGUI_IS_FILE_LAUNCHER (self));

  task = g_task_new (self, cancellable, callback, user_data);
  g_task_set_check_cancellable (task, FALSE);
  g_task_set_source_tag (task, bobgui_file_launcher_open_containing_folder);

  if (self->file == NULL)
    {
      g_task_return_new_error (task,
                               BOBGUI_DIALOG_ERROR, BOBGUI_DIALOG_ERROR_FAILED,
                               "No file to open");
      g_object_unref (task);
      return;
    }

  if (!g_file_is_native (self->file))
    {
      g_task_return_new_error (task,
                               BOBGUI_DIALOG_ERROR, BOBGUI_DIALOG_ERROR_FAILED,
                               "Operation not supported on non-native files");
      g_object_unref (task);
      return;
    }

#if defined (G_OS_WIN32)
  const char *path = g_file_peek_path (self->file);
  bobgui_open_containing_folder_win32 (path, cancellable, open_done, task);
#elif defined (GDK_WINDOWING_ANDROID)
  g_task_return_new_error (task,
                           BOBGUI_DIALOG_ERROR, BOBGUI_DIALOG_ERROR_FAILED,
                           "Operation not supported");
  g_object_unref (task);
#else
  if (bobgui_openuri_portal_is_available ())
    {
      BobguiOpenuriFlags flags = 0;

      bobgui_openuri_portal_open_async (self->file, TRUE, flags, parent, cancellable, open_done, task);
    }
  else
    {
      char *uri = g_file_get_uri (self->file);

      show_item (parent, uri, cancellable, task);

      g_free (uri);
    }
#endif
}

/**
 * bobgui_file_launcher_open_containing_folder_finish:
 * @self: a file launcher
 * @result: the result
 * @error: return location for a [enum@Bobgui.DialogError] or [enum@Gio.Error] error
 *
 * Finishes the [method@Bobgui.FileLauncher.open_containing_folder]
 * call and returns the result.
 *
 * Returns: true if an application was launched
 *
 * Since: 4.10
 */
gboolean
bobgui_file_launcher_open_containing_folder_finish (BobguiFileLauncher  *self,
                                                 GAsyncResult     *result,
                                                 GError          **error)
{
  g_return_val_if_fail (BOBGUI_IS_FILE_LAUNCHER (self), FALSE);
  g_return_val_if_fail (g_task_is_valid (result, self), FALSE);
  g_return_val_if_fail (g_task_get_source_tag (G_TASK (result)) == bobgui_file_launcher_open_containing_folder, FALSE);

  return g_task_propagate_boolean (G_TASK (result), error);
}

/* }}} */

/* vim:set foldmethod=marker: */
