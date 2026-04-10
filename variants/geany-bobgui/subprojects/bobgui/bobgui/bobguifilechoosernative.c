/* -*- Mode: C; c-file-style: "gnu"; tab-width: 8 -*- */
/* BOBGUI - The Bobgui Framework
 * bobguifilechoosernative.c: Native File selector dialog
 * Copyright (C) 2015, Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include "bobguifilechoosernativeprivate.h"
#include "bobguinativedialogprivate.h"

#include "bobguiprivate.h"
#include "bobguifilechooserprivate.h"
#include "deprecated/bobguifilechooserdialog.h"
#include "bobguifilechooserwidgetprivate.h"
#include "bobguifilechooserutils.h"
#include "bobguisizerequest.h"
#include "bobguitypebuiltins.h"
#include <glib/gi18n-lib.h>
#include "bobguisettings.h"
#include "bobguitogglebutton.h"
#include "bobguiheaderbar.h"
#include "bobguilabel.h"
#include "bobguifilefilterprivate.h"

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

/**
 * BobguiFileChooserNative:
 *
 * `BobguiFileChooserNative` is an abstraction of a dialog suitable
 * for use with “File Open” or “File Save as” commands.
 *
 * By default, this just uses a `BobguiFileChooserDialog` to implement
 * the actual dialog. However, on some platforms, such as Windows and
 * macOS, the native platform file chooser is used instead. When the
 * application is running in a sandboxed environment without direct
 * filesystem access (such as Flatpak), `BobguiFileChooserNative` may call
 * the proper APIs (portals) to let the user choose a file and make it
 * available to the application.
 *
 * While the API of `BobguiFileChooserNative` closely mirrors `BobguiFileChooserDialog`,
 * the main difference is that there is no access to any `BobguiWindow` or `BobguiWidget`
 * for the dialog. This is required, as there may not be one in the case of a
 * platform native dialog.
 *
 * Showing, hiding and running the dialog is handled by the
 * [class@Bobgui.NativeDialog] functions.
 *
 * Note that unlike `BobguiFileChooserDialog`, `BobguiFileChooserNative` objects
 * are not toplevel widgets, and BOBGUI does not keep them alive. It is your
 * responsibility to keep a reference until you are done with the
 * object.

 * ## Typical usage
 *
 * In the simplest of cases, you can the following code to use
 * `BobguiFileChooserNative` to select a file for opening:
 *
 * ```c
 * static void
 * on_response (BobguiNativeDialog *native,
 *              int              response)
 * {
 *   if (response == BOBGUI_RESPONSE_ACCEPT)
 *     {
 *       BobguiFileChooser *chooser = BOBGUI_FILE_CHOOSER (native);
 *       GFile *file = bobgui_file_chooser_get_file (chooser);
 *
 *       open_file (file);
 *
 *       g_object_unref (file);
 *     }
 *
 *   g_object_unref (native);
 * }
 *
 *   // ...
 *   BobguiFileChooserNative *native;
 *   BobguiFileChooserAction action = BOBGUI_FILE_CHOOSER_ACTION_OPEN;
 *
 *   native = bobgui_file_chooser_native_new ("Open File",
 *                                         parent_window,
 *                                         action,
 *                                         "_Open",
 *                                         "_Cancel");
 *
 *   g_signal_connect (native, "response", G_CALLBACK (on_response), NULL);
 *   bobgui_native_dialog_show (BOBGUI_NATIVE_DIALOG (native));
 * ```
 *
 * To use a `BobguiFileChooserNative` for saving, you can use this:
 *
 * ```c
 * static void
 * on_response (BobguiNativeDialog *native,
 *              int              response)
 * {
 *   if (response == BOBGUI_RESPONSE_ACCEPT)
 *     {
 *       BobguiFileChooser *chooser = BOBGUI_FILE_CHOOSER (native);
 *       GFile *file = bobgui_file_chooser_get_file (chooser);
 *
 *       save_to_file (file);
 *
 *       g_object_unref (file);
 *     }
 *
 *   g_object_unref (native);
 * }
 *
 *   // ...
 *   BobguiFileChooserNative *native;
 *   BobguiFileChooser *chooser;
 *   BobguiFileChooserAction action = BOBGUI_FILE_CHOOSER_ACTION_SAVE;
 *
 *   native = bobgui_file_chooser_native_new ("Save File",
 *                                         parent_window,
 *                                         action,
 *                                         "_Save",
 *                                         "_Cancel");
 *   chooser = BOBGUI_FILE_CHOOSER (native);
 *
 *   if (user_edited_a_new_document)
 *     bobgui_file_chooser_set_current_name (chooser, _("Untitled document"));
 *   else
 *     bobgui_file_chooser_set_file (chooser, existing_file, NULL);
 *
 *   g_signal_connect (native, "response", G_CALLBACK (on_response), NULL);
 *   bobgui_native_dialog_show (BOBGUI_NATIVE_DIALOG (native));
 * ```
 *
 * For more information on how to best set up a file dialog,
 * see the [class@Bobgui.FileChooserDialog] documentation.
 *
 * ## Response Codes
 *
 * `BobguiFileChooserNative` inherits from [class@Bobgui.NativeDialog],
 * which means it will return %BOBGUI_RESPONSE_ACCEPT if the user accepted,
 * and %BOBGUI_RESPONSE_CANCEL if he pressed cancel. It can also return
 * %BOBGUI_RESPONSE_DELETE_EVENT if the window was unexpectedly closed.
 *
 * ## Differences from `BobguiFileChooserDialog`
 *
 * There are a few things in the [iface@Bobgui.FileChooser] interface that
 * are not possible to use with `BobguiFileChooserNative`, as such use would
 * prohibit the use of a native dialog.
 *
 * No operations that change the dialog work while the dialog is visible.
 * Set all the properties that are required before showing the dialog.
 *
 * ## Win32 details
 *
 * On windows the `IFileDialog` implementation (added in Windows Vista) is
 * used. It supports many of the features that `BobguiFileChooser` has, but
 * there are some things it does not handle:
 *
 * * Any [class@Bobgui.FileFilter] added using a mimetype
 *
 * If any of these features are used the regular `BobguiFileChooserDialog`
 * will be used in place of the native one.
 *
 * ## Portal details
 *
 * When the `org.freedesktop.portal.FileChooser` portal is available on
 * the session bus, it is used to bring up an out-of-process file chooser.
 * Depending on the kind of session the application is running in, this may
 * or may not be a BOBGUI file chooser.
 *
 * ## macOS details
 *
 * On macOS the `NSSavePanel` and `NSOpenPanel` classes are used to provide
 * native file chooser dialogs. Some features provided by `BobguiFileChooser`
 * are not supported:
 *
 * * Shortcut folders.
 *
 * Deprecated: 4.10: Use [class@Bobgui.FileDialog] instead
 */

enum {
  MODE_FALLBACK,
  MODE_WIN32,
  MODE_QUARTZ,
  MODE_ANDROID,
  MODE_PORTAL,
};

enum {
  PROP_0,
  PROP_ACCEPT_LABEL,
  PROP_CANCEL_LABEL,
  LAST_ARG,
};

static GParamSpec *native_props[LAST_ARG] = { NULL, };

static void    _bobgui_file_chooser_native_iface_init   (BobguiFileChooserIface  *iface);

G_DEFINE_TYPE_WITH_CODE (BobguiFileChooserNative, bobgui_file_chooser_native, BOBGUI_TYPE_NATIVE_DIALOG,
                         G_IMPLEMENT_INTERFACE (BOBGUI_TYPE_FILE_CHOOSER,
                                                _bobgui_file_chooser_native_iface_init))


/**
 * bobgui_file_chooser_native_get_accept_label:
 * @self: a `BobguiFileChooserNative`
 *
 * Retrieves the custom label text for the accept button.
 *
 * Returns: (nullable): The custom label
 *
 * Deprecated: 4.10: Use [class@Bobgui.FileDialog] instead
 */
const char *
bobgui_file_chooser_native_get_accept_label (BobguiFileChooserNative *self)
{
  g_return_val_if_fail (BOBGUI_IS_FILE_CHOOSER_NATIVE (self), NULL);

  return self->accept_label;
}

/**
 * bobgui_file_chooser_native_set_accept_label:
 * @self: a `BobguiFileChooserNative`
 * @accept_label: (nullable): custom label
 *
 * Sets the custom label text for the accept button.
 *
 * If characters in @label are preceded by an underscore, they are
 * underlined. If you need a literal underscore character in a label,
 * use “__” (two underscores). The first underlined character represents
 * a keyboard accelerator called a mnemonic.
 *
 * Pressing Alt and that key should activate the button.
 *
 * Deprecated: 4.10: Use [class@Bobgui.FileDialog] instead
 */
void
bobgui_file_chooser_native_set_accept_label (BobguiFileChooserNative *self,
                                          const char           *accept_label)
{
  g_return_if_fail (BOBGUI_IS_FILE_CHOOSER_NATIVE (self));

  g_free (self->accept_label);
  self->accept_label = g_strdup (accept_label);

  g_object_notify_by_pspec (G_OBJECT (self), native_props[PROP_ACCEPT_LABEL]);
}

/**
 * bobgui_file_chooser_native_get_cancel_label:
 * @self: a `BobguiFileChooserNative`
 *
 * Retrieves the custom label text for the cancel button.
 *
 * Returns: (nullable): The custom label
 *
 * Deprecated: 4.10: Use [class@Bobgui.FileDialog] instead
 */
const char *
bobgui_file_chooser_native_get_cancel_label (BobguiFileChooserNative *self)
{
  g_return_val_if_fail (BOBGUI_IS_FILE_CHOOSER_NATIVE (self), NULL);

  return self->cancel_label;
}

/**
 * bobgui_file_chooser_native_set_cancel_label:
 * @self: a `BobguiFileChooserNative`
 * @cancel_label: (nullable): custom label
 *
 * Sets the custom label text for the cancel button.
 *
 * If characters in @label are preceded by an underscore, they are
 * underlined. If you need a literal underscore character in a label,
 * use “__” (two underscores). The first underlined character represents
 * a keyboard accelerator called a mnemonic.
 *
 * Pressing Alt and that key should activate the button.
 *
 * Deprecated: 4.10: Use [class@Bobgui.FileDialog] instead
 */
void
bobgui_file_chooser_native_set_cancel_label (BobguiFileChooserNative *self,
                                         const char           *cancel_label)
{
  g_return_if_fail (BOBGUI_IS_FILE_CHOOSER_NATIVE (self));

  g_free (self->cancel_label);
  self->cancel_label = g_strdup (cancel_label);

  g_object_notify_by_pspec (G_OBJECT (self), native_props[PROP_CANCEL_LABEL]);
}

static BobguiFileChooserNativeChoice *
find_choice (BobguiFileChooserNative *self,
             const char           *id)
{
  GSList *l;

  for (l = self->choices; l; l = l->next)
    {
      BobguiFileChooserNativeChoice *choice = l->data;

      if (strcmp (choice->id, id) == 0)
        return choice;
    }

  return NULL;
}

static void
bobgui_file_chooser_native_choice_free (BobguiFileChooserNativeChoice *choice)
{
  g_free (choice->id);
  g_free (choice->label);
  g_strfreev (choice->options);
  g_strfreev (choice->option_labels);
  g_free (choice->selected);
  g_free (choice);
}

static void
bobgui_file_chooser_native_add_choice (BobguiFileChooser  *chooser,
                                    const char      *id,
                                    const char      *label,
                                    const char     **options,
                                    const char     **option_labels)
{
  BobguiFileChooserNative *self = BOBGUI_FILE_CHOOSER_NATIVE (chooser);
  BobguiFileChooserNativeChoice *choice = find_choice (self, id);

  if (choice != NULL)
    {
      g_warning ("Choice with id %s already added to %s %p", id, G_OBJECT_TYPE_NAME (self), self);
      return;
    }

  g_assert ((options == NULL && option_labels == NULL) ||
            g_strv_length ((char **)options) == g_strv_length ((char **)option_labels));

  choice = g_new0 (BobguiFileChooserNativeChoice, 1);
  choice->id = g_strdup (id);
  choice->label = g_strdup (label);
  choice->options = g_strdupv ((char **)options);
  choice->option_labels = g_strdupv ((char **)option_labels);

  self->choices = g_slist_append (self->choices, choice);

  bobgui_file_chooser_add_choice (BOBGUI_FILE_CHOOSER (self->dialog),
                               id, label, options, option_labels);
}

static void
bobgui_file_chooser_native_remove_choice (BobguiFileChooser *chooser,
                                       const char     *id)
{
  BobguiFileChooserNative *self = BOBGUI_FILE_CHOOSER_NATIVE (chooser);
  BobguiFileChooserNativeChoice *choice = find_choice (self, id);

  if (choice == NULL)
    {
      g_warning ("No choice with id %s found in %s %p", id, G_OBJECT_TYPE_NAME (self), self);
      return;
    }

  self->choices = g_slist_remove (self->choices, choice);

  bobgui_file_chooser_native_choice_free (choice);

  bobgui_file_chooser_remove_choice (BOBGUI_FILE_CHOOSER (self->dialog), id);
}

static void
bobgui_file_chooser_native_set_choice (BobguiFileChooser *chooser,
                                    const char     *id,
                                    const char     *selected)
{
  BobguiFileChooserNative *self = BOBGUI_FILE_CHOOSER_NATIVE (chooser);
  BobguiFileChooserNativeChoice *choice = find_choice (self, id);

  if (choice == NULL)
    {
      g_warning ("No choice with id %s found in %s %p", id, G_OBJECT_TYPE_NAME (self), self);
      return;
    }

  if ((choice->options && !g_strv_contains ((const char *const*)choice->options, selected)) ||
      (!choice->options && !g_str_equal (selected, "true") && !g_str_equal (selected, "false")))
    {
      g_warning ("Not a valid option for %s: %s", id, selected);
      return;
    }

  g_free (choice->selected);
  choice->selected = g_strdup (selected);

  bobgui_file_chooser_set_choice (BOBGUI_FILE_CHOOSER (self->dialog), id, selected);
}

static const char *
bobgui_file_chooser_native_get_choice (BobguiFileChooser *chooser,
                                    const char     *id)
{
  BobguiFileChooserNative *self = BOBGUI_FILE_CHOOSER_NATIVE (chooser);
  BobguiFileChooserNativeChoice *choice = find_choice (self, id);

  if (choice == NULL)
    {
      g_warning ("No choice with id %s found in %s %p", id, G_OBJECT_TYPE_NAME (self), self);
      return NULL;
    }

  if (self->mode == MODE_FALLBACK)
    return bobgui_file_chooser_get_choice (BOBGUI_FILE_CHOOSER (self->dialog), id);

  return choice->selected;
}

static void
bobgui_file_chooser_native_set_property (GObject      *object,
                                      guint         prop_id,
                                      const GValue *value,
                                      GParamSpec   *pspec)

{
  BobguiFileChooserNative *self = BOBGUI_FILE_CHOOSER_NATIVE (object);

  switch (prop_id)
    {
    case PROP_ACCEPT_LABEL:
      bobgui_file_chooser_native_set_accept_label (self, g_value_get_string (value));
      break;

    case PROP_CANCEL_LABEL:
      bobgui_file_chooser_native_set_cancel_label (self, g_value_get_string (value));
      break;

    case BOBGUI_FILE_CHOOSER_PROP_FILTER:
      self->current_filter = g_value_get_object (value);
      bobgui_file_chooser_set_filter (BOBGUI_FILE_CHOOSER (self->dialog), self->current_filter);
      g_object_notify (G_OBJECT (self), "filter");
      break;

    default:
      g_object_set_property (G_OBJECT (self->dialog), pspec->name, value);
      break;
    }
}

static void
bobgui_file_chooser_native_get_property (GObject    *object,
                                      guint       prop_id,
                                      GValue     *value,
                                      GParamSpec *pspec)
{
  BobguiFileChooserNative *self = BOBGUI_FILE_CHOOSER_NATIVE (object);

  switch (prop_id)
    {
    case PROP_ACCEPT_LABEL:
      g_value_set_string (value, self->accept_label);
      break;

    case PROP_CANCEL_LABEL:
      g_value_set_string (value, self->cancel_label);
      break;

    case BOBGUI_FILE_CHOOSER_PROP_FILTER:
      self->current_filter = bobgui_file_chooser_get_filter (BOBGUI_FILE_CHOOSER (self->dialog));
      g_value_set_object (value, self->current_filter);
      break;

    default:
      g_object_get_property (G_OBJECT (self->dialog), pspec->name, value);
      break;
    }
}

static void
bobgui_file_chooser_native_finalize (GObject *object)
{
  BobguiFileChooserNative *self = BOBGUI_FILE_CHOOSER_NATIVE (object);

  g_clear_pointer (&self->current_name, g_free);
  g_clear_object (&self->current_file);
  g_clear_object (&self->current_folder);

  g_clear_pointer (&self->accept_label, g_free);
  g_clear_pointer (&self->cancel_label, g_free);
  bobgui_window_destroy (BOBGUI_WINDOW (self->dialog));

  g_slist_free_full (self->custom_files, g_object_unref);
  g_slist_free_full (self->choices, (GDestroyNotify)bobgui_file_chooser_native_choice_free);

  G_OBJECT_CLASS (bobgui_file_chooser_native_parent_class)->finalize (object);
}

static void
bobgui_file_chooser_native_init (BobguiFileChooserNative *self)
{
  /* We always create a File chooser dialog and delegate all properties to it.
   * This way we can reuse that store, plus we always have a dialog we can use
   * in case something makes the native one not work (like the custom widgets) */
  self->dialog = g_object_new (BOBGUI_TYPE_FILE_CHOOSER_DIALOG, NULL);
  self->cancel_button = bobgui_dialog_add_button (BOBGUI_DIALOG (self->dialog), _("_Cancel"), BOBGUI_RESPONSE_CANCEL);
  self->accept_button = bobgui_dialog_add_button (BOBGUI_DIALOG (self->dialog), _("_Open"), BOBGUI_RESPONSE_ACCEPT);

  bobgui_dialog_set_default_response (BOBGUI_DIALOG (self->dialog), BOBGUI_RESPONSE_ACCEPT);
  bobgui_window_set_hide_on_close (BOBGUI_WINDOW (self->dialog), TRUE);

  /* This is used, instead of the standard delegate, to ensure that signals are not delegated. */
  g_object_set_qdata (G_OBJECT (self), BOBGUI_FILE_CHOOSER_DELEGATE_QUARK, self->dialog);
}

/**
 * bobgui_file_chooser_native_new:
 * @title: (nullable): Title of the native
 * @parent: (nullable): Transient parent of the native
 * @action: Open or save mode for the dialog
 * @accept_label: (nullable): text to go in the accept button, or %NULL for the default
 * @cancel_label: (nullable): text to go in the cancel button, or %NULL for the default
 *
 * Creates a new `BobguiFileChooserNative`.
 *
 * Returns: a new `BobguiFileChooserNative`
 *
 * Deprecated: 4.10: Use [class@Bobgui.FileDialog] instead
 */
BobguiFileChooserNative *
bobgui_file_chooser_native_new (const char           *title,
                             BobguiWindow            *parent,
                             BobguiFileChooserAction  action,
                             const char           *accept_label,
                             const char           *cancel_label)
{
  BobguiFileChooserNative *result;

  result = g_object_new (BOBGUI_TYPE_FILE_CHOOSER_NATIVE,
                         "title", title,
                         "action", action,
                         "transient-for", parent,
                         "accept-label", accept_label,
                         "cancel-label", cancel_label,
                         NULL);

  return result;
}

static void
dialog_response_cb (BobguiDialog *dialog,
                    int response_id,
                    gpointer data)
{
  BobguiFileChooserNative *self = data;

  g_signal_handlers_disconnect_by_func (self->dialog, dialog_response_cb, self);
  bobgui_widget_set_visible (self->dialog, FALSE);

  _bobgui_native_dialog_emit_response (BOBGUI_NATIVE_DIALOG (self), response_id);
}

static void
show_dialog (BobguiFileChooserNative *self)
{
  BobguiFileChooserAction action;
  const char *accept_label, *cancel_label;

  action = bobgui_file_chooser_get_action (BOBGUI_FILE_CHOOSER (self->dialog));

  accept_label = self->accept_label;
  if (accept_label == NULL)
    accept_label = (action == BOBGUI_FILE_CHOOSER_ACTION_SAVE) ? _("_Save") :  _("_Open");

  bobgui_button_set_label (BOBGUI_BUTTON (self->accept_button), accept_label);

  cancel_label = self->cancel_label;
  if (cancel_label == NULL)
    cancel_label = _("_Cancel");

  bobgui_button_set_label (BOBGUI_BUTTON (self->cancel_button), cancel_label);

  bobgui_window_set_title (BOBGUI_WINDOW (self->dialog),
                        bobgui_native_dialog_get_title (BOBGUI_NATIVE_DIALOG (self)));

  bobgui_window_set_transient_for (BOBGUI_WINDOW (self->dialog),
                                bobgui_native_dialog_get_transient_for (BOBGUI_NATIVE_DIALOG (self)));

  bobgui_window_set_modal (BOBGUI_WINDOW (self->dialog),
                        bobgui_native_dialog_get_modal (BOBGUI_NATIVE_DIALOG (self)));

  g_signal_connect (self->dialog,
                    "response",
                    G_CALLBACK (dialog_response_cb),
                    self);

  bobgui_window_present (BOBGUI_WINDOW (self->dialog));
}

static void
hide_dialog (BobguiFileChooserNative *self)
{
  g_signal_handlers_disconnect_by_func (self->dialog, dialog_response_cb, self);
  bobgui_widget_set_visible (self->dialog, FALSE);
}

static gboolean
bobgui_file_chooser_native_set_current_folder (BobguiFileChooser    *chooser,
                                            GFile             *file,
                                            GError           **error)
{
  BobguiFileChooserNative *self = BOBGUI_FILE_CHOOSER_NATIVE (chooser);
  gboolean res;

  res = bobgui_file_chooser_set_current_folder (BOBGUI_FILE_CHOOSER (self->dialog),
                                             file, error);


  if (res)
    {
      g_set_object (&self->current_folder, file);

      g_clear_object (&self->current_file);
    }

  return res;
}

static gboolean
bobgui_file_chooser_native_select_file (BobguiFileChooser    *chooser,
                                     GFile             *file,
                                     GError           **error)
{
  BobguiFileChooserNative *self = BOBGUI_FILE_CHOOSER_NATIVE (chooser);
  gboolean res;

  res = bobgui_file_chooser_select_file (BOBGUI_FILE_CHOOSER (self->dialog),
                                      file, error);

  if (res)
    {
      g_set_object (&self->current_file, file);

      g_clear_object (&self->current_folder);
      g_clear_pointer (&self->current_name, g_free);
    }

  return res;
}

static void
bobgui_file_chooser_native_set_current_name (BobguiFileChooser    *chooser,
                                          const char        *name)
{
  BobguiFileChooserNative *self = BOBGUI_FILE_CHOOSER_NATIVE (chooser);

  bobgui_file_chooser_set_current_name (BOBGUI_FILE_CHOOSER (self->dialog), name);

  g_clear_pointer (&self->current_name, g_free);
  self->current_name = g_strdup (name);

  g_clear_object (&self->current_file);
}

static GListModel *
bobgui_file_chooser_native_get_files (BobguiFileChooser *chooser)
{
  BobguiFileChooserNative *self = BOBGUI_FILE_CHOOSER_NATIVE (chooser);

  switch (self->mode)
    {
    case MODE_PORTAL:
    case MODE_WIN32:
    case MODE_QUARTZ:
    case MODE_ANDROID:
      {
        GListStore *store;
        GSList *l;

        store = g_list_store_new (G_TYPE_FILE);
        for (l = self->custom_files; l; l = l->next)
          g_list_store_append (store, l->data);

        return G_LIST_MODEL (store);
      }

    case MODE_FALLBACK:
    default:
      return bobgui_file_chooser_get_files (BOBGUI_FILE_CHOOSER (self->dialog));
    }
}

static void
bobgui_file_chooser_native_show (BobguiNativeDialog *native)
{
  BobguiFileChooserNative *self = BOBGUI_FILE_CHOOSER_NATIVE (native);

  self->mode = MODE_FALLBACK;

#ifdef GDK_WINDOWING_WIN32
  if (bobgui_file_chooser_native_win32_show (self))
    self->mode = MODE_WIN32;
#endif

#ifdef GDK_WINDOWING_MACOS
  if (bobgui_file_chooser_native_quartz_show (self))
    self->mode = MODE_QUARTZ;
#endif

#ifdef GDK_WINDOWING_ANDROID
  if (bobgui_file_chooser_native_android_show (self))
    self->mode = MODE_ANDROID;
#endif

  if (self->mode == MODE_FALLBACK &&
      bobgui_file_chooser_native_portal_show (self))
    self->mode = MODE_PORTAL;

  if (self->mode == MODE_FALLBACK)
    show_dialog (self);
}

static void
bobgui_file_chooser_native_hide (BobguiNativeDialog *native)
{
  BobguiFileChooserNative *self = BOBGUI_FILE_CHOOSER_NATIVE (native);

  switch (self->mode)
    {
    case MODE_FALLBACK:
      hide_dialog (self);
      break;
    case MODE_WIN32:
#ifdef GDK_WINDOWING_WIN32
      bobgui_file_chooser_native_win32_hide (self);
#endif
      break;
    case MODE_QUARTZ:
#ifdef GDK_WINDOWING_MACOS
      bobgui_file_chooser_native_quartz_hide (self);
#endif
      break;
    case MODE_ANDROID:
#ifdef GDK_WINDOWING_ANDROID
      bobgui_file_chooser_native_android_hide (self);
#endif
      break;
    case MODE_PORTAL:
      bobgui_file_chooser_native_portal_hide (self);
      break;
    default:
      break;
    }
}

static void
bobgui_file_chooser_native_class_init (BobguiFileChooserNativeClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  BobguiNativeDialogClass *native_dialog_class = BOBGUI_NATIVE_DIALOG_CLASS (class);

  gobject_class->finalize = bobgui_file_chooser_native_finalize;
  gobject_class->set_property = bobgui_file_chooser_native_set_property;
  gobject_class->get_property = bobgui_file_chooser_native_get_property;

  native_dialog_class->show = bobgui_file_chooser_native_show;
  native_dialog_class->hide = bobgui_file_chooser_native_hide;

  _bobgui_file_chooser_install_properties (gobject_class);

  /**
   * BobguiFileChooserNative:accept-label:
   *
   * The text used for the label on the accept button in the dialog, or
   * %NULL to use the default text.
   */
 native_props[PROP_ACCEPT_LABEL] =
      g_param_spec_string ("accept-label", NULL, NULL,
                           NULL,
                           BOBGUI_PARAM_READWRITE);

  /**
   * BobguiFileChooserNative:cancel-label:
   *
   * The text used for the label on the cancel button in the dialog, or
   * %NULL to use the default text.
   */
  native_props[PROP_CANCEL_LABEL] =
      g_param_spec_string ("cancel-label", NULL, NULL,
                           NULL,
                           BOBGUI_PARAM_READWRITE);

  g_object_class_install_properties (gobject_class, LAST_ARG, native_props);
}

static void
_bobgui_file_chooser_native_iface_init (BobguiFileChooserIface *iface)
{
  _bobgui_file_chooser_delegate_iface_init (iface);
  iface->select_file = bobgui_file_chooser_native_select_file;
  iface->set_current_name = bobgui_file_chooser_native_set_current_name;
  iface->set_current_folder = bobgui_file_chooser_native_set_current_folder;
  iface->get_files = bobgui_file_chooser_native_get_files;
  iface->add_choice = bobgui_file_chooser_native_add_choice;
  iface->remove_choice = bobgui_file_chooser_native_remove_choice;
  iface->set_choice = bobgui_file_chooser_native_set_choice;
  iface->get_choice = bobgui_file_chooser_native_get_choice;
}
