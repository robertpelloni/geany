/*
 * BOBGUI - The Bobgui Framework
 * Copyright (C) 2023 Red Hat, Inc.
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

#include "bobguiprintdialog.h"

#include "bobguiwindowprivate.h"
#include "bobguidialogerror.h"
#include "bobguiprivate.h"
#include "deprecated/bobguidialog.h"

#include "print/bobguiprinter.h"

#include "print/bobguiprinterprivate.h"

#ifdef HAVE_GIO_UNIX

#include <fcntl.h>

#include <glib-unix.h>
#include <gio/gunixfdlist.h>
#include <gio/gunixoutputstream.h>
#include <gio/gfiledescriptorbased.h>
#include "print/bobguiprintjob.h"
#include "print/bobguiprintunixdialog.h"

#endif

#include <glib/gi18n-lib.h>

/* {{{ BobguiPrintSetup */

/**
 * BobguiPrintSetup:
 *
 * An auxiliary object for printing that allows decoupling the setup from the printing.
 *
 * A print setup is obtained by calling [method@Bobgui.PrintDialog.setup],
 * and can later be passed to print functions such as [method@Bobgui.PrintDialog.print].
 *
 * Print setups can be reused for multiple print calls.
 *
 * Applications may wish to store the page_setup and print_settings from the print setup
 * and copy them to the PrintDialog if they want to keep using them.
 *
 * Since: 4.14
 */

struct _BobguiPrintSetup
{
  unsigned int ref_count;

  BobguiPrintSettings *print_settings;
  BobguiPageSetup *page_setup;
  BobguiPrinter *printer;
  unsigned int token;
};

G_DEFINE_BOXED_TYPE (BobguiPrintSetup, bobgui_print_setup,
                     bobgui_print_setup_ref,
                     bobgui_print_setup_unref)

#ifdef HAVE_GIO_UNIX
static BobguiPrintSetup *
bobgui_print_setup_new (void)
{
  BobguiPrintSetup *setup;

  setup = g_new0 (BobguiPrintSetup, 1);

  setup->ref_count = 1;

  return setup;
}
#endif

/**
 * bobgui_print_setup_ref:
 * @setup: a `BobguiPrintSetup`
 *
 * Increase the reference count of @setup.
 *
 * Returns: the print setup
 *
 * Since: 4.14
 */
BobguiPrintSetup *
bobgui_print_setup_ref (BobguiPrintSetup *setup)
{
  setup->ref_count++;

  return setup;
}

/**
 * bobgui_print_setup_unref:
 * @setup: a `BobguiPrintSetup`
 *
 * Decrease the reference count of @setup.
 *
 * If the reference count reaches zero,
 * the object is freed.
 *
 * Since: 4.14
 */
void
bobgui_print_setup_unref (BobguiPrintSetup *setup)
{
  setup->ref_count--;

  if (setup->ref_count > 0)
    return;

  g_clear_object (&setup->print_settings);
  g_clear_object (&setup->page_setup);
  g_clear_object (&setup->printer);
  g_free (setup);
}

/**
 * bobgui_print_setup_get_print_settings:
 * @setup: a `BobguiPrintSetup`
 *
 * Returns the print settings of @setup.
 *
 * They may be different from the `BobguiPrintDialog`'s settings
 * if the user changed them during the setup process.
 *
 * Returns: (transfer none): the print settings, or `NULL`
 *
 * Since: 4.14
 */
BobguiPrintSettings *
bobgui_print_setup_get_print_settings (BobguiPrintSetup *setup)
{
  return setup->print_settings;
}

#ifdef HAVE_GIO_UNIX
static void
bobgui_print_setup_set_print_settings (BobguiPrintSetup    *setup,
                                    BobguiPrintSettings *print_settings)
{
  g_set_object (&setup->print_settings, print_settings);
}
#endif

/**
 * bobgui_print_setup_get_page_setup:
 * @setup: a `BobguiPrintSetup`
 *
 * Returns the page setup of @setup.
 *
 * It may be different from the `BobguiPrintDialog`'s page setup
 * if the user changed it during the setup process.
 *
 * Returns: (transfer none): the page setup, or `NULL`
 *
 * Since: 4.14
 */
BobguiPageSetup *
bobgui_print_setup_get_page_setup (BobguiPrintSetup *setup)
{
  return setup->page_setup;
}

#ifdef HAVE_GIO_UNIX
static void
bobgui_print_setup_set_page_setup (BobguiPrintSetup *setup,
                                BobguiPageSetup  *page_setup)
{
  g_set_object (&setup->page_setup, page_setup);
}

static BobguiPrinter *
bobgui_print_setup_get_printer (BobguiPrintSetup *setup)
{
  if (!setup->printer)
    {
      const char *name = NULL;

      if (setup->print_settings)
        name = bobgui_print_settings_get (setup->print_settings, BOBGUI_PRINT_SETTINGS_PRINTER);

      if (name)
        setup->printer = bobgui_printer_find (name);
    }

  return setup->printer;
}

static void
bobgui_print_setup_set_printer (BobguiPrintSetup *setup,
                             BobguiPrinter    *printer)
{
  g_set_object (&setup->printer, printer);
}
#endif

/* }}} */
/* {{{ GObject implementation */

/**
 * BobguiPrintDialog:
 *
 * Asynchronous API to present a print dialog to the user.
 *
 * `BobguiPrintDialog` collects the arguments that are needed to present
 *  the dialog, such as a title for the dialog and whether it should
 *  be modal.
 *
 * The dialog is shown with the [method@Bobgui.PrintDialog.setup] function.
 *
 * The actual printing can be done with [method@Bobgui.PrintDialog.print] or
 * [method@Bobgui.PrintDialog.print_file]. These APIs follows the GIO async pattern,
 * and the results can be obtained by calling the corresponding finish methods.
 *
 * Since: 4.14
 */

struct _BobguiPrintDialog
{
  GObject parent_instance;

  BobguiPrintSettings *print_settings;
  BobguiPageSetup *page_setup;

  GDBusProxy *portal;

  char *accept_label;
  char *title;

  unsigned int modal : 1;
};

enum
{
  PROP_ACCEPT_LABEL = 1,
  PROP_PAGE_SETUP,
  PROP_MODAL,
  PROP_PRINT_SETTINGS,
  PROP_TITLE,

  NUM_PROPERTIES
};

static GParamSpec *properties[NUM_PROPERTIES];

G_DEFINE_TYPE (BobguiPrintDialog, bobgui_print_dialog, G_TYPE_OBJECT)

static void
bobgui_print_dialog_init (BobguiPrintDialog *self)
{
  self->modal = TRUE;
}

static void
bobgui_print_dialog_finalize (GObject *object)
{
  BobguiPrintDialog *self = BOBGUI_PRINT_DIALOG (object);

  g_clear_object (&self->portal);
  g_clear_object (&self->print_settings);
  g_clear_object (&self->page_setup);
  g_free (self->accept_label);
  g_free (self->title);

  G_OBJECT_CLASS (bobgui_print_dialog_parent_class)->finalize (object);
}

static void
bobgui_print_dialog_get_property (GObject      *object,
                               unsigned int  property_id,
                               GValue       *value,
                               GParamSpec   *pspec)
{
  BobguiPrintDialog *self = BOBGUI_PRINT_DIALOG (object);

  switch (property_id)
    {
    case PROP_ACCEPT_LABEL:
      g_value_set_string (value, self->accept_label);
      break;

    case PROP_PAGE_SETUP:
      g_value_set_object (value, self->page_setup);
      break;

    case PROP_MODAL:
      g_value_set_boolean (value, self->modal);
      break;

    case PROP_PRINT_SETTINGS:
      g_value_set_object (value, self->print_settings);
      break;

    case PROP_TITLE:
      g_value_set_string (value, self->title);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
bobgui_print_dialog_set_property (GObject      *object,
                               unsigned int  prop_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  BobguiPrintDialog *self = BOBGUI_PRINT_DIALOG (object);

  switch (prop_id)
    {
    case PROP_ACCEPT_LABEL:
      bobgui_print_dialog_set_accept_label (self, g_value_get_string (value));
      break;

    case PROP_PAGE_SETUP:
      bobgui_print_dialog_set_page_setup (self, g_value_get_object (value));
      break;

    case PROP_MODAL:
      bobgui_print_dialog_set_modal (self, g_value_get_boolean (value));
      break;

    case PROP_PRINT_SETTINGS:
      bobgui_print_dialog_set_print_settings (self, g_value_get_object (value));
      break;

    case PROP_TITLE:
      bobgui_print_dialog_set_title (self, g_value_get_string (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bobgui_print_dialog_class_init (BobguiPrintDialogClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  object_class->finalize = bobgui_print_dialog_finalize;
  object_class->get_property = bobgui_print_dialog_get_property;
  object_class->set_property = bobgui_print_dialog_set_property;

  /**
   * BobguiPrintDialog:accept-label:
   *
   * A label that may be shown on the accept button of a print dialog
   * that is presented by [method@Bobgui.PrintDialog.setup].
   *
   * Since: 4.14
   */
  properties[PROP_ACCEPT_LABEL] =
      g_param_spec_string ("accept-label", NULL, NULL,
                           NULL,
                           G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiPrintDialog:page-setup:
   *
   * The page setup to use.
   *
   * Since: 4.14
   */
  properties[PROP_PAGE_SETUP] =
      g_param_spec_object ("page-setup", NULL, NULL,
                           BOBGUI_TYPE_PAGE_SETUP,
                           G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiPrintDialog:modal:
   *
   * Whether the print dialog is modal.
   *
   * Since: 4.14
   */
  properties[PROP_MODAL] =
      g_param_spec_boolean ("modal", NULL, NULL,
                            TRUE,
                            G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiPrintDialog:print-settings:
   *
   * The print settings to use.
   *
   * Since: 4.14
   */
  properties[PROP_PRINT_SETTINGS] =
      g_param_spec_object ("print-settings", NULL, NULL,
                           BOBGUI_TYPE_PRINT_SETTINGS,
                           G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiPrintDialog:title:
   *
   * A title that may be shown on the print dialog that is
   * presented by [method@Bobgui.PrintDialog.setup].
   *
   * Since: 4.14
   */
  properties[PROP_TITLE] =
      g_param_spec_string ("title", NULL, NULL,
                           NULL,
                           G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS|G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, NUM_PROPERTIES, properties);
}

/* }}} */
/* {{{ API: Constructor */

/**
 * bobgui_print_dialog_new:
 *
 * Creates a new `BobguiPrintDialog` object.
 *
 * Returns: the new `BobguiPrintDialog`
 *
 * Since: 4.14
 */
BobguiPrintDialog *
bobgui_print_dialog_new (void)
{
  return g_object_new (BOBGUI_TYPE_PRINT_DIALOG, NULL);
}

/* }}} */
/* {{{ API: Getters and setters */

/**
 * bobgui_print_dialog_get_title:
 * @self: a `BobguiPrintDialog`
 *
 * Returns the title that will be shown on the
 * print dialog.
 *
 * Returns: the title
 *
 * Since: 4.14
 */
const char *
bobgui_print_dialog_get_title (BobguiPrintDialog *self)
{
  g_return_val_if_fail (BOBGUI_IS_PRINT_DIALOG (self), NULL);

  return self->title;
}

/**
 * bobgui_print_dialog_set_title:
 * @self: a `BobguiPrintDialog`
 * @title: the new title
 *
 * Sets the title that will be shown on the print dialog.
 *
 * Since: 4.14
 */
void
bobgui_print_dialog_set_title (BobguiPrintDialog *self,
                            const char     *title)
{
  g_return_if_fail (BOBGUI_IS_PRINT_DIALOG (self));
  g_return_if_fail (title != NULL);

  if (g_strcmp0 (self->title, title) == 0)
    return;

  g_free (self->title);
  self->title = g_strdup (title);

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_TITLE]);
}

/**
 * bobgui_print_dialog_get_accept_label:
 * @self: a `BobguiPrintDialog`
 *
 * Returns the label that will be shown on the
 * accept button of the print dialog.
 *
 * Returns: the accept label
 *
 * Since: 4.14
 */
const char *
bobgui_print_dialog_get_accept_label (BobguiPrintDialog *self)
{
  g_return_val_if_fail (BOBGUI_IS_PRINT_DIALOG (self), NULL);

  return self->accept_label;
}

/**
 * bobgui_print_dialog_set_accept_label:
 * @self: a `BobguiPrintDialog`
 * @accept_label: the new accept label
 *
 * Sets the label that will be shown on the
 * accept button of the print dialog shown for
 * [method@Bobgui.PrintDialog.setup].
 *
 * Since: 4.14
 */
void
bobgui_print_dialog_set_accept_label (BobguiPrintDialog *self,
                                   const char     *accept_label)
{
  g_return_if_fail (BOBGUI_IS_PRINT_DIALOG (self));
  g_return_if_fail (accept_label != NULL);

  if (g_strcmp0 (self->accept_label, accept_label) == 0)
    return;

  g_free (self->accept_label);
  self->accept_label = g_strdup (accept_label);

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_ACCEPT_LABEL]);
}

/**
 * bobgui_print_dialog_get_modal:
 * @self: a `BobguiPrintDialog`
 *
 * Returns whether the print dialog blocks
 * interaction with the parent window while
 * it is presented.
 *
 * Returns: whether the print dialog is modal
 *
 * Since: 4.14
 */
gboolean
bobgui_print_dialog_get_modal (BobguiPrintDialog *self)
{
  g_return_val_if_fail (BOBGUI_IS_PRINT_DIALOG (self), TRUE);

  return self->modal;
}

/**
 * bobgui_print_dialog_set_modal:
 * @self: a `BobguiPrintDialog`
 * @modal: the new value
 *
 * Sets whether the print dialog blocks
 * interaction with the parent window while
 * it is presented.
 *
 * Since: 4.14
 */
void
bobgui_print_dialog_set_modal (BobguiPrintDialog *self,
                            gboolean        modal)
{
  g_return_if_fail (BOBGUI_IS_PRINT_DIALOG (self));

  if (self->modal == modal)
    return;

  self->modal = modal;

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_MODAL]);
}

/**
 * bobgui_print_dialog_get_page_setup:
 * @self: a `BobguiPrintDialog`
 *
 * Returns the page setup.
 *
 * Returns: (nullable) (transfer none): the page setup
 *
 * Since: 4.14
 */
BobguiPageSetup *
bobgui_print_dialog_get_page_setup (BobguiPrintDialog *self)
{
  g_return_val_if_fail (BOBGUI_IS_PRINT_DIALOG (self), NULL);

  return self->page_setup;
}

/**
 * bobgui_print_dialog_set_page_setup:
 * @self: a `BobguiPrintDialog`
 * @page_setup: the new page setup
 *
 * Set the page setup for the print dialog.
 *
 * Since: 4.14
 */
void
bobgui_print_dialog_set_page_setup (BobguiPrintDialog *self,
                                 BobguiPageSetup   *page_setup)
{
  g_return_if_fail (BOBGUI_IS_PRINT_DIALOG (self));
  g_return_if_fail (BOBGUI_IS_PAGE_SETUP (page_setup));

  if (g_set_object (&self->page_setup, page_setup))
    g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_PAGE_SETUP]);
}

/**
 * bobgui_print_dialog_get_print_settings:
 * @self: a `BobguiPrintDialog`
 *
 * Returns the print settings for the print dialog.
 *
 * Returns: (nullable) (transfer none): the settings
 *
 * Since: 4.14
 */
BobguiPrintSettings *
bobgui_print_dialog_get_print_settings (BobguiPrintDialog *self)
{
  g_return_val_if_fail (BOBGUI_IS_PRINT_DIALOG (self), NULL);

  return self->print_settings;
}

/**
 * bobgui_print_dialog_set_print_settings:
 * @self: a `BobguiPrintDialog`
 * @print_settings: the new print settings
 *
 * Sets the print settings for the print dialog.
 *
 * Since: 4.14
 */
void
bobgui_print_dialog_set_print_settings (BobguiPrintDialog   *self,
                                     BobguiPrintSettings *print_settings)
{
  g_return_if_fail (BOBGUI_IS_PRINT_DIALOG (self));
  g_return_if_fail (BOBGUI_IS_PRINT_SETTINGS (print_settings));

  if (g_set_object (&self->print_settings, print_settings))
    g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_PRINT_SETTINGS]);
}

/* }}} */
/* {{{ Print output stream */

#ifdef HAVE_GIO_UNIX

#define BOBGUI_TYPE_PRINT_OUTPUT_STREAM (bobgui_print_output_stream_get_type ())
G_DECLARE_FINAL_TYPE (BobguiPrintOutputStream, bobgui_print_output_stream, BOBGUI, PRINT_OUTPUT_STREAM, GUnixOutputStream)

struct _BobguiPrintOutputStream
{
  GUnixOutputStream parent_instance;

  gboolean print_done;
  GError *print_error;
};

struct _BobguiPrintOutputStreamClass
{
  GUnixOutputStreamClass parent_class;
};

G_DEFINE_TYPE (BobguiPrintOutputStream, bobgui_print_output_stream, G_TYPE_UNIX_OUTPUT_STREAM);

static void
bobgui_print_output_stream_init (BobguiPrintOutputStream *stream)
{
}

static void
bobgui_print_output_stream_finalize (GObject *object)
{
  BobguiPrintOutputStream *stream = BOBGUI_PRINT_OUTPUT_STREAM (object);

  g_clear_error (&stream->print_error);

  G_OBJECT_CLASS (bobgui_print_output_stream_parent_class)->finalize (object);
}

static gboolean
bobgui_print_output_stream_close (GOutputStream  *ostream,
                               GCancellable   *cancellable,
                               GError        **error)
{
  BobguiPrintOutputStream *stream = BOBGUI_PRINT_OUTPUT_STREAM (ostream);

  G_OUTPUT_STREAM_CLASS (bobgui_print_output_stream_parent_class)->close_fn (ostream, cancellable, NULL);

  while (!stream->print_done)
    g_main_context_iteration (NULL, TRUE);

  if (stream->print_error)
    {
      g_propagate_error (error, stream->print_error);
      stream->print_error = NULL;

      return FALSE;
    }

  return TRUE;
}

static void
bobgui_print_output_stream_class_init (BobguiPrintOutputStreamClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  GOutputStreamClass *stream_class = G_OUTPUT_STREAM_CLASS (class);

  object_class->finalize = bobgui_print_output_stream_finalize;

  stream_class->close_fn = bobgui_print_output_stream_close;
}

static BobguiPrintOutputStream *
bobgui_print_output_stream_new (int fd)
{
  return g_object_new (BOBGUI_TYPE_PRINT_OUTPUT_STREAM, "fd", fd, NULL);
}

static void
bobgui_print_output_stream_set_print_done (BobguiPrintOutputStream *stream,
                                        GError               *error)
{
  g_assert (!stream->print_done);
  stream->print_done = TRUE;
  stream->print_error = error;
}

#endif

/* }}} */
/* {{{ Async implementation */

#ifdef HAVE_GIO_UNIX

typedef struct
{
  BobguiWindow *exported_window;
  char *exported_window_handle;
  char *portal_handle;
  unsigned int response_signal_id;
  unsigned int token;
  int fds[2];
  gboolean has_returned;
  BobguiPrintOutputStream *stream;
} PrintTaskData;

static PrintTaskData *
print_task_data_new (void)
{
  PrintTaskData *ptd = g_new0 (PrintTaskData, 1);

  ptd->fds[0] = ptd->fds[1] = -1;

  return ptd;
}

static void
print_task_data_free (gpointer data)
{
  PrintTaskData *ptd = data;

  g_free (ptd->portal_handle);
  if (ptd->exported_window && ptd->exported_window_handle)
    bobgui_window_unexport_handle (ptd->exported_window, ptd->exported_window_handle);
  g_clear_pointer (&ptd->exported_window_handle, g_free);
  g_clear_object (&ptd->exported_window);
  if (ptd->fds[0] != -1)
    close (ptd->fds[0]);
  if (ptd->fds[1] != -1)
    close (ptd->fds[1]);
  g_free (ptd);
}

/* {{{ Portal helpers */

static void
send_close (GTask *task)
{
  BobguiPrintDialog *self = BOBGUI_PRINT_DIALOG (g_task_get_source_object (task));
  PrintTaskData *ptd = g_task_get_task_data (task);
  GDBusConnection *connection = g_dbus_proxy_get_connection (self->portal);
  GDBusMessage *message;
  GError *error = NULL;

  if (!ptd->portal_handle)
    return;

  message = g_dbus_message_new_method_call (PORTAL_BUS_NAME,
                                            ptd->portal_handle,
                                            PORTAL_REQUEST_INTERFACE,
                                            "Close");

  if (!g_dbus_connection_send_message (connection,
                                       message,
                                       G_DBUS_SEND_MESSAGE_FLAGS_NONE,
                                       NULL, &error))
    {
      g_warning ("unable to send PrintDialog Close message: %s", error->message);
      g_error_free (error);
    }

  g_object_unref (message);
}

static gboolean
ensure_portal_proxy (BobguiPrintDialog  *self,
                     BobguiWindow       *parent,
                     GError         **error)
{
  if (self->portal)
    return TRUE;

  if (!self->portal)
    self->portal = g_dbus_proxy_new_for_bus_sync (G_BUS_TYPE_SESSION,
                                                  G_DBUS_PROXY_FLAGS_NONE,
                                                  NULL,
                                                  PORTAL_BUS_NAME,
                                                  PORTAL_OBJECT_PATH,
                                                  PORTAL_PRINT_INTERFACE,
                                                  NULL,
                                                  error);

  return self->portal != NULL;
}

static void cleanup_portal_call_data (GTask *task);

static void
cancelled_cb (GCancellable *cancellable,
              GTask        *task)
{
  send_close (task);
  cleanup_portal_call_data (task);

  g_task_return_new_error (task,
                           BOBGUI_DIALOG_ERROR,
                           BOBGUI_DIALOG_ERROR_CANCELLED,
                           "Cancelled by application");
  g_object_unref (task);
}

static void
cleanup_portal_call_data (GTask *task)
{
  BobguiPrintDialog *self = BOBGUI_PRINT_DIALOG (g_task_get_source_object (task));
  PrintTaskData *ptd = g_task_get_task_data (task);
  GDBusConnection *connection = g_dbus_proxy_get_connection (self->portal);
  GCancellable *cancellable;

  cancellable = g_task_get_cancellable (task);

  if (cancellable)
    g_signal_handlers_disconnect_by_func (cancellable, cancelled_cb, task);

  if (ptd->response_signal_id != 0)
    {
      g_dbus_connection_signal_unsubscribe (connection, ptd->response_signal_id);
      ptd->response_signal_id = 0;
    }

  if (ptd->exported_window && ptd->exported_window_handle)
    bobgui_window_unexport_handle (ptd->exported_window, ptd->exported_window_handle);

  g_clear_pointer (&ptd->portal_handle, g_free);
  g_clear_object (&ptd->exported_window);
  g_clear_pointer (&ptd->exported_window_handle, g_free);
}

/* }}} */
/* {{{ Portal Setup implementation */

static void
prepare_print_response (GDBusConnection *connection,
                        const char      *sender_name,
                        const char      *object_path,
                        const char      *interface_name,
                        const char      *signal_name,
                        GVariant        *parameters,
                        gpointer         user_data)
{
  GTask *task = user_data;
  guint32 response;
  GVariant *options = NULL;

  cleanup_portal_call_data (task);

  g_variant_get (parameters, "(u@a{sv})", &response, &options);

  switch (response)
    {
    case 0:
      {
        GVariant *v;
        BobguiPrintSettings *settings;
        BobguiPageSetup *page_setup;
        BobguiPrintSetup *setup;
        unsigned int token;

        setup = bobgui_print_setup_new ();

        v = g_variant_lookup_value (options, "settings", G_VARIANT_TYPE_VARDICT);
        settings = bobgui_print_settings_new_from_gvariant (v);
        g_variant_unref (v);

        bobgui_print_setup_set_print_settings (setup, settings);
        g_object_unref (settings);

        v = g_variant_lookup_value (options, "page-setup", G_VARIANT_TYPE_VARDICT);
        page_setup = bobgui_page_setup_new_from_gvariant (v);
        g_variant_unref (v);

        bobgui_print_setup_set_page_setup (setup, page_setup);
        g_object_unref (page_setup);

        g_variant_lookup (options, "token", "u", &token);
        setup->token = token;

        g_task_return_pointer (task, bobgui_print_setup_ref (setup), (GDestroyNotify) bobgui_print_setup_unref);

        bobgui_print_setup_unref (setup);
      }
      break;

    case 1:
      g_task_return_new_error (task,
                               BOBGUI_DIALOG_ERROR,
                               BOBGUI_DIALOG_ERROR_DISMISSED,
                               "Dismissed by user");
      break;

    case 2:
    default:
      g_task_return_new_error (task,
                               BOBGUI_DIALOG_ERROR,
                               BOBGUI_DIALOG_ERROR_FAILED,
                               "Operation failed");
      break;
    }

  g_variant_unref (options);

  g_object_unref (task);
}

static void
prepare_print_called (GObject      *source,
                      GAsyncResult *result,
                      gpointer      user_data)
{
  GTask *task = user_data;
  BobguiPrintDialog *self = BOBGUI_PRINT_DIALOG (g_task_get_source_object (task));
  GDBusConnection *connection = g_dbus_proxy_get_connection (self->portal);
  PrintTaskData *ptd = g_task_get_task_data (task);
  GError *error = NULL;
  GVariant *ret;
  char *path;

  ret = g_dbus_proxy_call_finish (self->portal, result, &error);
  if (ret == NULL)
    {
      cleanup_portal_call_data (task);
      g_task_return_error (task, error);
      g_object_unref (task);
      return;
    }

  g_variant_get (ret, "(o)", &path);
  if (strcmp (path, ptd->portal_handle) != 0)
   {
      g_free (ptd->portal_handle);
      ptd->portal_handle = g_steal_pointer (&path);

      g_dbus_connection_signal_unsubscribe (connection,
                                            ptd->response_signal_id);

      ptd->response_signal_id =
        g_dbus_connection_signal_subscribe (connection,
                                            PORTAL_BUS_NAME,
                                            PORTAL_REQUEST_INTERFACE,
                                            "Response",
                                            ptd->portal_handle,
                                            NULL,
                                            G_DBUS_SIGNAL_FLAGS_NO_MATCH_RULE,
                                            prepare_print_response,
                                            self, NULL);

    }

  g_free (path);
  g_variant_unref (ret);
}

static void
setup_window_handle_exported (BobguiWindow  *window,
                              const char *window_handle,
                              gpointer    user_data)
{
  GTask *task = user_data;
  BobguiPrintDialog *self = BOBGUI_PRINT_DIALOG (g_task_get_source_object (task));
  PrintTaskData *ptd = g_task_get_task_data (task);
  GDBusConnection *connection = g_dbus_proxy_get_connection (self->portal);
  char *handle_token;
  GVariant *settings;
  GVariant *setup;
  GVariant *options;
  GVariantBuilder opt_builder;

  g_assert (ptd->portal_handle == NULL);
  ptd->portal_handle = bobgui_get_portal_request_path (connection, &handle_token);

  if (window)
    {
      ptd->exported_window = g_object_ref (window);
      ptd->exported_window_handle = g_strdup (window_handle);
    }

  g_assert (ptd->response_signal_id == 0);
  ptd->response_signal_id =
    g_dbus_connection_signal_subscribe (connection,
                                        PORTAL_BUS_NAME,
                                        PORTAL_REQUEST_INTERFACE,
                                        "Response",
                                        ptd->portal_handle,
                                        NULL,
                                        G_DBUS_SIGNAL_FLAGS_NO_MATCH_RULE,
                                        prepare_print_response,
                                        task, NULL);

  g_variant_builder_init (&opt_builder, G_VARIANT_TYPE_VARDICT);
  g_variant_builder_add (&opt_builder, "{sv}", "handle_token", g_variant_new_string (handle_token));
  if (self->accept_label)
    g_variant_builder_add (&opt_builder, "{sv}", "accept_label", g_variant_new_string (self->accept_label));
  options = g_variant_builder_end (&opt_builder);

  if (self->print_settings)
    settings = bobgui_print_settings_to_gvariant (self->print_settings);
  else
    {
      GVariantBuilder builder;
      g_variant_builder_init (&builder, G_VARIANT_TYPE_VARDICT);
      settings = g_variant_builder_end (&builder);
    }

  if (self->page_setup)
    setup = bobgui_page_setup_to_gvariant (self->page_setup);
  else
    {
      BobguiPageSetup *page_setup = bobgui_page_setup_new ();
      setup = bobgui_page_setup_to_gvariant (page_setup);
      g_object_unref (page_setup);
    }

  g_dbus_proxy_call (self->portal,
                     "PreparePrint",
                     g_variant_new ("(ss@a{sv}@a{sv}@a{sv})",
                                    window_handle,
                                    self->title ? self->title : "",
                                    settings,
                                    setup,
                                    options),
                     G_DBUS_CALL_FLAGS_NONE,
                     -1,
                     NULL,
                     prepare_print_called,
                     task);

  g_free (handle_token);
}

/* }}} */
/* {{{ Portal Print implementation */

static void
print_response (GDBusConnection *connection,
                const char      *sender_name,
                const char      *object_path,
                const char      *interface_name,
                const char      *signal_name,
                GVariant        *parameters,
                gpointer         user_data)
{
  GTask *task = user_data;
  PrintTaskData *ptd = g_task_get_task_data (task);
  guint32 response;

  cleanup_portal_call_data (task);
  g_variant_get (parameters, "(ua{sv})", &response, NULL);

  if (ptd->has_returned)
    {
      if (ptd->stream)
        {
          switch (response)
            {
            case 0:
              bobgui_print_output_stream_set_print_done (ptd->stream, NULL);
              break;
            case 1:
              bobgui_print_output_stream_set_print_done (ptd->stream,
                                                      g_error_new_literal (BOBGUI_DIALOG_ERROR,
                                                                           BOBGUI_DIALOG_ERROR_DISMISSED,
                                                                           "Dismissed by user"));
              break;
            case 2:
            default:
              bobgui_print_output_stream_set_print_done (ptd->stream,
                                                      g_error_new_literal (BOBGUI_DIALOG_ERROR,
                                                                           BOBGUI_DIALOG_ERROR_FAILED,
                                                                           "Operation failed"));
              break;
            }
        }
    }
  else
    {
      switch (response)
        {
        case 0:
          g_task_return_boolean (task, TRUE);
          break;

        case 1:
          g_task_return_new_error (task,
                                   BOBGUI_DIALOG_ERROR,
                                   BOBGUI_DIALOG_ERROR_DISMISSED,
                                   "Dismissed by user");
          break;

        case 2:
        default:
          g_task_return_new_error (task,
                                   BOBGUI_DIALOG_ERROR,
                                   BOBGUI_DIALOG_ERROR_FAILED,
                                   "Operation failed");
          break;
        }
    }

  g_object_unref (task);
}

static void
print_called (GObject      *source,
              GAsyncResult *result,
              gpointer      user_data)
{
  GTask *task = user_data;
  BobguiPrintDialog *self = BOBGUI_PRINT_DIALOG (g_task_get_source_object (task));
  PrintTaskData *ptd = g_task_get_task_data (task);
  GDBusConnection *connection = g_dbus_proxy_get_connection (self->portal);
  GError *error = NULL;
  GVariant *ret;
  char *path;

  ret = g_dbus_proxy_call_finish (self->portal, result, &error);
  if (ret == NULL)
    {
      cleanup_portal_call_data (task);
      g_task_return_error (task, error);
      g_object_unref (task);
      return;
    }

  g_variant_get (ret, "(o)", &path);
  if (strcmp (path, ptd->portal_handle) != 0)
   {
      g_free (ptd->portal_handle);
      ptd->portal_handle = g_steal_pointer (&path);

      g_dbus_connection_signal_unsubscribe (connection,
                                            ptd->response_signal_id);

      ptd->response_signal_id =
        g_dbus_connection_signal_subscribe (connection,
                                            PORTAL_BUS_NAME,
                                            PORTAL_REQUEST_INTERFACE,
                                            "Response",
                                            ptd->portal_handle,
                                            NULL,
                                            G_DBUS_SIGNAL_FLAGS_NO_MATCH_RULE,
                                            print_response,
                                            task, NULL);

    }

  g_free (path);
  g_variant_unref (ret);

  if (ptd->fds[1] != -1)
    {
      ptd->stream = bobgui_print_output_stream_new (ptd->fds[1]);
      ptd->fds[1] = -1;
      ptd->has_returned = TRUE;
      g_object_add_weak_pointer (G_OBJECT (ptd->stream), (gpointer *)&ptd->stream);
      g_task_return_pointer (task, ptd->stream, g_object_unref);
      g_object_unref (task);
    }
}

static void
print_window_handle_exported (BobguiWindow  *window,
                              const char *window_handle,
                              gpointer    user_data)
{
  GTask *task = user_data;
  BobguiPrintDialog *self = BOBGUI_PRINT_DIALOG (g_task_get_source_object (task));
  PrintTaskData *ptd = g_task_get_task_data (task);
  GDBusConnection *connection = g_dbus_proxy_get_connection (self->portal);
  char *handle_token;
  GVariantBuilder opt_builder;
  GUnixFDList *fd_list;
  int idx;

  if (window)
    {
      ptd->exported_window = g_object_ref (window);
      ptd->exported_window_handle = g_strdup (window_handle);
    }

  g_assert (ptd->fds[0] != -1);

  ptd->portal_handle = bobgui_get_portal_request_path (connection, &handle_token);

  ptd->response_signal_id =
    g_dbus_connection_signal_subscribe (connection,
                                        PORTAL_BUS_NAME,
                                        PORTAL_REQUEST_INTERFACE,
                                        "Response",
                                        ptd->portal_handle,
                                        NULL,
                                        G_DBUS_SIGNAL_FLAGS_NO_MATCH_RULE,
                                        print_response,
                                        task, NULL);

  fd_list = g_unix_fd_list_new ();
  idx = g_unix_fd_list_append (fd_list, ptd->fds[0], NULL);

  g_variant_builder_init (&opt_builder, G_VARIANT_TYPE_VARDICT);
  g_variant_builder_add (&opt_builder, "{sv}", "handle_token", g_variant_new_string (handle_token));
  g_variant_builder_add (&opt_builder, "{sv}", "token", g_variant_new_uint32 (ptd->token));

  g_dbus_proxy_call_with_unix_fd_list (self->portal,
                                       "Print",
                                       g_variant_new ("(ssh@a{sv})",
                                                      window_handle,
                                                      self->title ? self->title : "",
                                                      idx,
                                                      g_variant_builder_end (&opt_builder)),
                                       G_DBUS_CALL_FLAGS_NONE,
                                       -1,
                                       fd_list,
                                       NULL,
                                       print_called,
                                       task);
  g_object_unref (fd_list);
  g_free (handle_token);
}

/* }}} */
/* {{{ Local fallback */

static BobguiPrintUnixDialog *
create_print_dialog (BobguiPrintDialog   *self,
                     BobguiPrintSettings *print_settings,
                     BobguiPageSetup     *page_setup,
                     BobguiWindow        *parent)
{
  BobguiPrintUnixDialog *dialog;

  dialog = BOBGUI_PRINT_UNIX_DIALOG (bobgui_print_unix_dialog_new (self->title, parent));

  if (print_settings)
    bobgui_print_unix_dialog_set_settings (dialog, print_settings);

  if (page_setup)
    bobgui_print_unix_dialog_set_page_setup (dialog, page_setup);

  bobgui_print_unix_dialog_set_embed_page_setup (dialog, TRUE);

  return dialog;
}

static void
setup_response_cb (BobguiPrintUnixDialog *window,
                   int                 response,
                   GTask              *task)
{
  GCancellable *cancellable = g_task_get_cancellable (task);

  if (cancellable)
    g_signal_handlers_disconnect_by_func (cancellable, cancelled_cb, task);

  if (response == BOBGUI_RESPONSE_OK)
    {
      BobguiPrintSetup *setup = bobgui_print_setup_new ();

      bobgui_print_setup_set_print_settings (setup, bobgui_print_unix_dialog_get_settings (window));
      bobgui_print_setup_set_page_setup (setup, bobgui_print_unix_dialog_get_page_setup (window));
      bobgui_print_setup_set_printer (setup, bobgui_print_unix_dialog_get_selected_printer (window));

      g_task_return_pointer (task, setup, (GDestroyNotify) bobgui_print_setup_unref);
    }
  else if (response == BOBGUI_RESPONSE_CLOSE)
    g_task_return_new_error (task, BOBGUI_DIALOG_ERROR, BOBGUI_DIALOG_ERROR_CANCELLED, "Cancelled by application");
  else if (response == BOBGUI_RESPONSE_CANCEL ||
           response == BOBGUI_RESPONSE_DELETE_EVENT)
    g_task_return_new_error (task, BOBGUI_DIALOG_ERROR, BOBGUI_DIALOG_ERROR_DISMISSED, "Dismissed by user");
  else
    g_task_return_new_error (task, BOBGUI_DIALOG_ERROR, BOBGUI_DIALOG_ERROR_FAILED, "Unknown failure (%d)", response);

  g_object_unref (task);
  bobgui_window_destroy (BOBGUI_WINDOW (window));
}

static void
job_complete (BobguiPrintJob  *job,
              gpointer      user_data,
              const GError *error)
{
  GTask *task = user_data;
  PrintTaskData *ptd = g_task_get_task_data (task);

  if (ptd->has_returned)
    {
      if (ptd->stream)
        bobgui_print_output_stream_set_print_done (ptd->stream, error ? g_error_copy (error) : NULL);
    }
  else if (error)
    g_task_return_error (task, g_error_copy (error));
  else
    g_task_return_boolean (task, TRUE);

  g_object_unref (task);
}

static void
print_content (BobguiPrintSetup *setup,
               GTask         *task)
{
  PrintTaskData *ptd = g_task_get_task_data (task);

  g_assert (ptd->fds[0] != -1);

  if (setup->printer)
    {
      BobguiPrintJob *job;

      g_object_ref (task);

      job = bobgui_print_job_new ("My first printjob",
                               setup->printer,
                               setup->print_settings,
                               setup->page_setup);
      bobgui_print_job_set_source_fd (job, ptd->fds[0], NULL);
      bobgui_print_job_send (job, job_complete, g_object_ref (task), g_object_unref);
      g_object_unref (job);

      if (ptd->fds[1] != -1)
        {
          ptd->stream = bobgui_print_output_stream_new (ptd->fds[1]);
          ptd->fds[1] = -1;
          ptd->has_returned = TRUE;
          g_object_add_weak_pointer (G_OBJECT (ptd->stream), (gpointer *)&ptd->stream);
          g_task_return_pointer (task, ptd->stream, g_object_unref);
        }

      g_object_unref (task);
    }
  else
    {
      g_task_return_new_error (task, BOBGUI_DIALOG_ERROR, BOBGUI_DIALOG_ERROR_FAILED, "No printer selected");
      g_object_unref (task);
    }
}

static void
print_response_cb (BobguiPrintUnixDialog *window,
                   int                 response,
                   GTask              *task)
{
  GCancellable *cancellable = g_task_get_cancellable (task);

  if (cancellable)
    g_signal_handlers_disconnect_by_func (cancellable, cancelled_cb, task);

  if (response == BOBGUI_RESPONSE_OK)
    {
      BobguiPrintSetup *setup = bobgui_print_setup_new ();

      bobgui_print_setup_set_print_settings (setup, bobgui_print_unix_dialog_get_settings (window));
      bobgui_print_setup_set_page_setup (setup, bobgui_print_unix_dialog_get_page_setup (window));
      bobgui_print_setup_set_printer (setup, bobgui_print_unix_dialog_get_selected_printer (window));

      print_content (setup, task);

      bobgui_print_setup_unref (setup);
    }
  else if (response == BOBGUI_RESPONSE_CLOSE)
    {
      g_task_return_new_error (task, BOBGUI_DIALOG_ERROR, BOBGUI_DIALOG_ERROR_CANCELLED, "Cancelled by application");
      g_object_unref (task);
    }
  else if (response == BOBGUI_RESPONSE_CANCEL ||
           response == BOBGUI_RESPONSE_DELETE_EVENT)
    {
      g_task_return_new_error (task, BOBGUI_DIALOG_ERROR, BOBGUI_DIALOG_ERROR_DISMISSED, "Dismissed by user");
      g_object_unref (task);
    }
  else
    {
      g_task_return_new_error (task, BOBGUI_DIALOG_ERROR, BOBGUI_DIALOG_ERROR_FAILED, "Unknown failure (%d)", response);
      g_object_unref (task);
    }

  bobgui_window_destroy (BOBGUI_WINDOW (window));
}

/* }}} */

#endif /* HAVE_GIO_UNIX */

/* }}} */
/* {{{ Async API */

/**
 * bobgui_print_dialog_setup:
 * @self: a `BobguiPrintDialog`
 * @parent: (nullable): the parent `BobguiWindow`
 * @cancellable: (nullable): a `GCancellable` to cancel the operation
 * @callback: (scope async) (closure user_data): a callback to call when the
 *   operation is complete
 * @user_data: data to pass to @callback
 *
 * This function presents a print dialog to let the user select a printer,
 * and set up print settings and page setup.
 *
 * The @callback will be called when the dialog is dismissed.
 * The obtained [struct@Bobgui.PrintSetup] can then be passed
 * to [method@Bobgui.PrintDialog.print] or [method@Bobgui.PrintDialog.print_file].
 *
 * One possible use for this method is to have the user select a printer,
 * then show a page setup UI in the application (e.g. to arrange images
 * on a page), then call [method@Bobgui.PrintDialog.print] on @self
 * to do the printing without further user interaction.
 *
 * Since: 4.14
 */
void
bobgui_print_dialog_setup (BobguiPrintDialog       *self,
                        BobguiWindow            *parent,
                        GCancellable         *cancellable,
                        GAsyncReadyCallback   callback,
                        gpointer              user_data)
{
  GTask *task;
#ifdef HAVE_GIO_UNIX
  GdkDisplay *display;
#endif
  G_GNUC_UNUSED GError *error = NULL;

  g_return_if_fail (BOBGUI_IS_PRINT_DIALOG (self));
  g_return_if_fail (parent == NULL || BOBGUI_IS_WINDOW (parent));

  task = g_task_new (self, cancellable, callback, user_data);
  g_task_set_check_cancellable (task, FALSE);
  g_task_set_source_tag (task, bobgui_print_dialog_setup);

#ifdef HAVE_GIO_UNIX
  if (cancellable)
    g_signal_connect (cancellable, "cancelled", G_CALLBACK (cancelled_cb), task);

  if (parent)
    display = bobgui_widget_get_display (BOBGUI_WIDGET (parent));
  else
    display = gdk_display_get_default ();

  if (!gdk_display_should_use_portal (display, PORTAL_PRINT_INTERFACE, 0))
    {
      BobguiPrintUnixDialog *window;

      window = create_print_dialog (self, self->print_settings, self->page_setup, parent);
      g_signal_connect (window, "response", G_CALLBACK (setup_response_cb), task);
      bobgui_window_present (BOBGUI_WINDOW (window));
    }
  else if (!ensure_portal_proxy (self, parent, &error))
    {
      g_task_return_new_error (task,
                               BOBGUI_DIALOG_ERROR, BOBGUI_DIALOG_ERROR_FAILED,
                               "The print portal is not available: %s", error->message);
      g_error_free (error);
      g_object_unref (task);
    }
  else
    {
      g_task_set_task_data (task, print_task_data_new (), (GDestroyNotify) print_task_data_free);

      if (parent &&
          bobgui_widget_is_visible (BOBGUI_WIDGET (parent)) &&
          bobgui_window_export_handle (parent, setup_window_handle_exported, task))
        return;

      setup_window_handle_exported (parent, "", task);
    }
#else
  g_task_return_new_error (task,
                           BOBGUI_DIALOG_ERROR, BOBGUI_DIALOG_ERROR_FAILED,
                           "BobguiPrintDialog is not supported on this platform");
  g_object_unref (task);
#endif
}

/**
 * bobgui_print_dialog_setup_finish:
 * @self: a `BobguiPrintDialog`
 * @result: a `GAsyncResult`
 * @error: return location for a [enum@Bobgui.DialogError] error
 *
 * Finishes the [method@Bobgui.PrintDialog.setup] call.
 *
 * If the call was successful, it returns a [struct@Bobgui.PrintSetup]
 * which contains the print settings and page setup information that
 * will be used to print.
 *
 * Note that this function returns a [error@Bobgui.DialogError.DISMISSED]
 * error if the user cancels the dialog.
 *
 * Returns: (transfer full): the resulting `[struct@Bobgui.PrintSetup]`
 *
 * Since: 4.14
 */
BobguiPrintSetup *
bobgui_print_dialog_setup_finish (BobguiPrintDialog    *self,
                               GAsyncResult      *result,
                               GError           **error)
{
  g_return_val_if_fail (BOBGUI_IS_PRINT_DIALOG (self), FALSE);
  g_return_val_if_fail (g_task_is_valid (result, self), FALSE);
  g_return_val_if_fail (g_task_get_source_tag (G_TASK (result)) == bobgui_print_dialog_setup, FALSE);

  return g_task_propagate_pointer (G_TASK (result), error);
}

/**
 * bobgui_print_dialog_print:
 * @self: a `BobguiPrintDialog`
 * @parent: (nullable): the parent `BobguiWindow`
 * @setup: (nullable): the `BobguiPrintSetup` to use
 * @cancellable: (nullable): a `GCancellable` to cancel the operation
 * @callback: (scope async) (closure user_data): a callback to call when the
 *   operation is complete
 * @user_data: data to pass to @callback
 *
 * This function prints content from a stream.
 *
 * If you pass `NULL` as @setup, then this method will present a print dialog.
 * Otherwise, it will attempt to print directly, without user interaction.
 *
 * The @callback will be called when the printing is done.
 *
 * Since: 4.14
 */
void
bobgui_print_dialog_print (BobguiPrintDialog       *self,
                        BobguiWindow            *parent,
                        BobguiPrintSetup        *setup,
                        GCancellable         *cancellable,
                        GAsyncReadyCallback   callback,
                        gpointer              user_data)
{
  GTask *task;
  G_GNUC_UNUSED GError *error = NULL;
#ifdef HAVE_GIO_UNIX
  GdkDisplay *display;
  PrintTaskData *ptd;
#endif

  g_return_if_fail (BOBGUI_IS_PRINT_DIALOG (self));
  g_return_if_fail (parent == NULL || BOBGUI_IS_WINDOW (parent));

  task = g_task_new (self, cancellable, callback, user_data);
  g_task_set_check_cancellable (task, FALSE);
  g_task_set_source_tag (task, bobgui_print_dialog_print);

#ifdef HAVE_GIO_UNIX
  ptd = print_task_data_new ();
  ptd->token = setup ? setup->token : 0;
  g_task_set_task_data (task, ptd, print_task_data_free);

  if (!g_unix_open_pipe (ptd->fds, O_CLOEXEC, &error))
    {
      g_task_return_error (task, error);
      g_object_unref (task);
      return;
    }

  if (cancellable)
    g_signal_connect (cancellable, "cancelled", G_CALLBACK (cancelled_cb), task);

  if (parent)
    display = bobgui_widget_get_display (BOBGUI_WIDGET (parent));
  else
    display = gdk_display_get_default ();

  if (!gdk_display_should_use_portal (display, PORTAL_PRINT_INTERFACE, 0))
    {
      if (setup == NULL || bobgui_print_setup_get_printer (setup) == NULL)
        {
          BobguiPrintUnixDialog *window;

          window = create_print_dialog (self,
                                        setup ? setup->print_settings : self->print_settings,
                                        setup ? setup->page_setup : self->page_setup,
                                        parent);
          g_signal_connect (window, "response", G_CALLBACK (print_response_cb), task);
          bobgui_window_present (BOBGUI_WINDOW (window));
        }
      else
        {
          print_content (setup, task);
        }
    }
  else if (!ensure_portal_proxy (self, parent, &error))
    {
      g_task_return_new_error (task,
                               BOBGUI_DIALOG_ERROR, BOBGUI_DIALOG_ERROR_FAILED,
                               "The print portal is not available: %s", error->message);
      g_error_free (error);
      g_object_unref (task);
    }
  else
    {
      if (parent &&
          bobgui_widget_is_visible (BOBGUI_WIDGET (parent)) &&
          bobgui_window_export_handle (parent, print_window_handle_exported, task))
        return;

      print_window_handle_exported (parent, "", task);
    }
#else
  g_task_return_new_error (task,
                           BOBGUI_DIALOG_ERROR, BOBGUI_DIALOG_ERROR_FAILED,
                           "BobguiPrintDialog is not supported on this platform");
  g_object_unref (task);
#endif
}

/**
 * bobgui_print_dialog_print_finish:
 * @self: a `BobguiPrintDialog`
 * @result: a `GAsyncResult`
 * @error: return location for a [enum@Bobgui.DialogError] error
 *
 * Finishes the [method@Bobgui.PrintDialog.print] call and
 * returns the results.
 *
 * If the call was successful, the content to be printed should be
 * written to the returned output stream. Otherwise, `NULL` is returned.
 *
 * The overall results of the print operation will be returned in the
 * [method@Gio.OutputStream.close] call, so if you are interested in the
 * results, you need to explicitly close the output stream (it will be
 * closed automatically if you just unref it). Be aware that the close
 * call may not be instant as it operation will for the printer to finish
 * printing.
 *
 * Note that this function returns a [error@Bobgui.DialogError.DISMISSED]
 * error if the user cancels the dialog.
 *
 * Returns: (transfer full): a [class@Gio.OutputStream]
 *
 * Since: 4.14
 */
GOutputStream *
bobgui_print_dialog_print_finish (BobguiPrintDialog  *self,
                               GAsyncResult    *result,
                               GError         **error)
{
  g_return_val_if_fail (BOBGUI_IS_PRINT_DIALOG (self), FALSE);
  g_return_val_if_fail (g_task_is_valid (result, self), FALSE);
  g_return_val_if_fail (g_task_get_source_tag (G_TASK (result)) == bobgui_print_dialog_print, FALSE);

  return g_task_propagate_pointer (G_TASK (result), error);
}

/**
 * bobgui_print_dialog_print_file:
 * @self: a `BobguiPrintDialog`
 * @parent: (nullable): the parent `BobguiWindow`
 * @setup: (nullable):  the `BobguiPrintSetup` to use
 * @file: the `GFile` to print
 * @cancellable: (nullable): a `GCancellable` to cancel the operation
 * @callback: (scope async) (closure user_data): a callback to call when the
 *   operation is complete
 * @user_data: data to pass to @callback
 *
 * This function prints a file.
 *
 * If you pass `NULL` as @setup, then this method will present a print dialog.
 * Otherwise, it will attempt to print directly, without user interaction.
 *
 * Since: 4.14
 */
void
bobgui_print_dialog_print_file (BobguiPrintDialog       *self,
                             BobguiWindow            *parent,
                             BobguiPrintSetup        *setup,
                             GFile                *file,
                             GCancellable         *cancellable,
                             GAsyncReadyCallback   callback,
                             gpointer              user_data)
{
  GTask *task;
#ifdef HAVE_GIO_UNIX
  GdkDisplay *display;
  PrintTaskData *ptd;
  GFileInputStream *content;
  GError *error = NULL;
#endif

  g_return_if_fail (BOBGUI_IS_PRINT_DIALOG (self));
  g_return_if_fail (parent == NULL || BOBGUI_IS_WINDOW (parent));
  g_return_if_fail (G_IS_FILE (file));

  task = g_task_new (self, cancellable, callback, user_data);
  g_task_set_check_cancellable (task, FALSE);
  g_task_set_source_tag (task, bobgui_print_dialog_print_file);

#ifdef HAVE_GIO_UNIX
  ptd = print_task_data_new ();
  ptd->token = setup ? setup->token : 0;
  g_task_set_task_data (task, ptd, print_task_data_free);

  content = g_file_read (file, NULL, NULL);
  if (G_IS_FILE_DESCRIPTOR_BASED (content))
    ptd->fds[0] = dup (g_file_descriptor_based_get_fd (G_FILE_DESCRIPTOR_BASED (content)));
  g_clear_object (&content);

  if (ptd->fds[0] == -1)
    {
      g_task_return_new_error (task,
                               BOBGUI_DIALOG_ERROR, BOBGUI_DIALOG_ERROR_FAILED,
                               _("Failed to create the read file descriptor"));
      g_object_unref (task);
      return;
    }

 if (cancellable)
    g_signal_connect (cancellable, "cancelled", G_CALLBACK (cancelled_cb), task);

  if (parent)
    display = bobgui_widget_get_display (BOBGUI_WIDGET (parent));
  else
    display = gdk_display_get_default ();

  if (!gdk_display_should_use_portal (display, PORTAL_PRINT_INTERFACE, 0))
    {
      if (setup == NULL || bobgui_print_setup_get_printer (setup) == NULL)
        {
          BobguiPrintUnixDialog *window;

          window = create_print_dialog (self,
                                        setup ? setup->print_settings : self->print_settings,
                                        setup ? setup->page_setup : self->page_setup,
                                        parent);
          g_signal_connect (window, "response", G_CALLBACK (print_response_cb), task);
          bobgui_window_present (BOBGUI_WINDOW (window));
        }
      else
        {
          print_content (setup, task);
        }
    }
  else if (!ensure_portal_proxy (self, parent, &error))
    {
      g_task_return_new_error (task,
                               BOBGUI_DIALOG_ERROR, BOBGUI_DIALOG_ERROR_FAILED,
                               "The print portal is not available: %s", error->message);
      g_error_free (error);
      g_object_unref (task);
    }
  else
    {
      if (parent &&
          bobgui_widget_is_visible (BOBGUI_WIDGET (parent)) &&
          bobgui_window_export_handle (parent, print_window_handle_exported, task))
        return;

      print_window_handle_exported (parent, "", task);
    }
#else
  g_task_return_new_error (task,
                           BOBGUI_DIALOG_ERROR, BOBGUI_DIALOG_ERROR_FAILED,
                           "BobguiPrintDialog is not supported on this platform");
  g_object_unref (task);
#endif
}

/**
 * bobgui_print_dialog_print_file_finish:
 * @self: a `BobguiPrintDialog`
 * @result: a `GAsyncResult`
 * @error: return location for a [enum@Bobgui.DialogError] error
 *
 * Finishes the [method@Bobgui.PrintDialog.print_file] call and
 * returns the results.
 *
 * Note that this function returns a [error@Bobgui.DialogError.DISMISSED]
 * error if the user cancels the dialog.
 *
 * Returns: Whether the call was successful
 *
 * Since: 4.14
 */
gboolean
bobgui_print_dialog_print_file_finish (BobguiPrintDialog  *self,
                                    GAsyncResult    *result,
                                    GError         **error)
{
  g_return_val_if_fail (BOBGUI_IS_PRINT_DIALOG (self), FALSE);
  g_return_val_if_fail (g_task_is_valid (result, self), FALSE);
  g_return_val_if_fail (g_task_get_source_tag (G_TASK (result)) == bobgui_print_dialog_print_file, FALSE);

  return g_task_propagate_boolean (G_TASK (result), error);
}

/* }}} */

/* vim:set foldmethod=marker: */
