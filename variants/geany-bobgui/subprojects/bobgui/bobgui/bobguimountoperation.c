/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* BOBGUI - The Bobgui Framework
 * Copyright (C) Christian Kellner <gicmo@gnome.org>
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

/*
 * Modified by the BOBGUI Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the BOBGUI Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * BOBGUI at ftp://ftp.bobgui.org/pub/bobgui/.
 */

#include "config.h"

#include <errno.h>
#include <string.h>

#include "bobguimountoperationprivate.h"
#include "bobguibox.h"
#include "bobguidbusgenerated.h"
#include "bobguientry.h"
#include "bobguibox.h"
#include <glib/gi18n-lib.h>
#include "bobguilabel.h"
#include "bobguialertdialog.h"
#include "bobguimountoperation.h"
#include "bobguiprivate.h"
#include "bobguicheckbutton.h"
#include "bobguigrid.h"
#include "bobguiwindow.h"
#include "bobguiscrolledwindow.h"
#include "bobguiicontheme.h"
#include "bobguimain.h"
#include "bobguisettings.h"
#include "deprecated/bobguidialogprivate.h"
#include "bobguipopover.h"
#include "bobguisnapshot.h"
#include "gdktextureprivate.h"
#include <glib/gprintf.h>
#include "bobguilistview.h"
#include "bobguisignallistitemfactory.h"
#include "bobguilistitem.h"
#include "bobguisingleselection.h"
#include "bobguipicture.h"


/**
 * BobguiMountOperation:
 *
 * Asks the user for passwords and other information required to
 * mount a volume.
 *
 * `BobguiMountOperation` is needed when mounting volumes:
 * It is an implementation of `GMountOperation` that can be used with
 * GIO functions for mounting volumes such as
 * [method@Gio.File.mount_enclosing_volume],
 * [method@Gio.File.mount_mountable],
 * [method@Gio.Volume.mount],
 * [method@Gio.Mount.unmount_with_operation] and others.
 *
 * When necessary, `BobguiMountOperation` shows dialogs to let the user
 * enter passwords, ask questions or show processes blocking unmount.
 */

static void   bobgui_mount_operation_finalize     (GObject          *object);
static void   bobgui_mount_operation_set_property (GObject          *object,
                                                guint             prop_id,
                                                const GValue     *value,
                                                GParamSpec       *pspec);
static void   bobgui_mount_operation_get_property (GObject          *object,
                                                guint             prop_id,
                                                GValue           *value,
                                                GParamSpec       *pspec);

static void   bobgui_mount_operation_ask_password (GMountOperation *op,
                                                const char      *message,
                                                const char      *default_user,
                                                const char      *default_domain,
                                                GAskPasswordFlags flags);

static void   bobgui_mount_operation_ask_question (GMountOperation *op,
                                                const char      *message,
                                                const char      *choices[]);

static void   bobgui_mount_operation_show_processes (GMountOperation *op,
                                                  const char      *message,
                                                  GArray          *processes,
                                                  const char      *choices[]);

static void   bobgui_mount_operation_aborted      (GMountOperation *op);

struct _BobguiMountOperationPrivate {
  BobguiWindow *parent_window;
  BobguiDialog *dialog;
  GdkDisplay *display;

  /* bus proxy */
  _BobguiMountOperationHandler *handler;
  GCancellable *cancellable;
  gboolean handler_showing;

  /* for the ask-password dialog */
  BobguiWidget *grid;
  BobguiWidget *username_entry;
  BobguiWidget *domain_entry;
  BobguiWidget *password_entry;
  BobguiWidget *pim_entry;
  BobguiWidget *anonymous_toggle;
  BobguiWidget *tcrypt_hidden_toggle;
  BobguiWidget *tcrypt_system_toggle;
  GList *user_widgets;

  GAskPasswordFlags ask_flags;
  GPasswordSave     password_save;
  gboolean          anonymous;

  /* for the show-processes dialog */
  BobguiWidget *process_list_view;
  GListStore *process_list_store;
};

enum {
  PROP_0,
  PROP_PARENT,
  PROP_IS_SHOWING,
  PROP_DISPLAY

};

G_DEFINE_TYPE_WITH_PRIVATE (BobguiMountOperation, bobgui_mount_operation, G_TYPE_MOUNT_OPERATION)

static void
bobgui_mount_operation_class_init (BobguiMountOperationClass *klass)
{
  GObjectClass         *object_class = G_OBJECT_CLASS (klass);
  GMountOperationClass *mount_op_class = G_MOUNT_OPERATION_CLASS (klass);

  object_class->finalize     = bobgui_mount_operation_finalize;
  object_class->get_property = bobgui_mount_operation_get_property;
  object_class->set_property = bobgui_mount_operation_set_property;

  mount_op_class->ask_password = bobgui_mount_operation_ask_password;
  mount_op_class->ask_question = bobgui_mount_operation_ask_question;
  mount_op_class->show_processes = bobgui_mount_operation_show_processes;
  mount_op_class->aborted = bobgui_mount_operation_aborted;

  /**
   * BobguiMountOperation:parent:
   *
   * The parent window.
   */
  g_object_class_install_property (object_class,
                                   PROP_PARENT,
                                   g_param_spec_object ("parent", NULL, NULL,
                                                        BOBGUI_TYPE_WINDOW,
                                                        BOBGUI_PARAM_READWRITE));

  /**
   * BobguiMountOperation:is-showing:
   *
   * Whether a dialog is currently shown.
   */
  g_object_class_install_property (object_class,
                                   PROP_IS_SHOWING,
                                   g_param_spec_boolean ("is-showing", NULL, NULL,
                                                         FALSE,
                                                         BOBGUI_PARAM_READABLE));

  /**
   * BobguiMountOperation:display:
   *
   * The display where dialogs will be shown.
   */
  g_object_class_install_property (object_class,
                                   PROP_DISPLAY,
                                   g_param_spec_object ("display", NULL, NULL,
                                                        GDK_TYPE_DISPLAY,
                                                        BOBGUI_PARAM_READWRITE));
}

static void
bobgui_mount_operation_init (BobguiMountOperation *operation)
{
  char *name_owner;

  operation->priv = bobgui_mount_operation_get_instance_private (operation);

  operation->priv->handler =
    _bobgui_mount_operation_handler_proxy_new_for_bus_sync (G_BUS_TYPE_SESSION,
                                                         G_DBUS_PROXY_FLAGS_DO_NOT_AUTO_START,
                                                         "org.bobgui.MountOperationHandler",
                                                         "/org/bobgui/MountOperationHandler",
                                                         NULL, NULL);
  if (!operation->priv->handler)
    return;

  name_owner = g_dbus_proxy_get_name_owner (G_DBUS_PROXY (operation->priv->handler));
  if (!name_owner)
    g_clear_object (&operation->priv->handler);
  g_free (name_owner);

  if (operation->priv->handler)
    g_dbus_proxy_set_default_timeout (G_DBUS_PROXY (operation->priv->handler), G_MAXINT);
}

static void
parent_destroyed (BobguiWidget  *parent,
                  gpointer  **pointer)
{
  *pointer = NULL;
}

static void
bobgui_mount_operation_finalize (GObject *object)
{
  BobguiMountOperation *operation = BOBGUI_MOUNT_OPERATION (object);
  BobguiMountOperationPrivate *priv = operation->priv;

  if (priv->user_widgets)
    g_list_free (priv->user_widgets);

  if (priv->parent_window)
    {
      g_signal_handlers_disconnect_by_func (priv->parent_window,
                                            parent_destroyed,
                                            &priv->parent_window);
      g_object_unref (priv->parent_window);
    }

  if (priv->display)
    g_object_unref (priv->display);

  if (priv->handler)
    g_object_unref (priv->handler);

  G_OBJECT_CLASS (bobgui_mount_operation_parent_class)->finalize (object);
}

static void
bobgui_mount_operation_set_property (GObject      *object,
                                  guint         prop_id,
                                  const GValue *value,
                                  GParamSpec   *pspec)
{
  BobguiMountOperation *operation = BOBGUI_MOUNT_OPERATION (object);

  switch (prop_id)
    {
    case PROP_PARENT:
      bobgui_mount_operation_set_parent (operation, g_value_get_object (value));
      break;

    case PROP_DISPLAY:
      bobgui_mount_operation_set_display (operation, g_value_get_object (value));
      break;

    case PROP_IS_SHOWING:
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bobgui_mount_operation_get_property (GObject    *object,
                                  guint       prop_id,
                                  GValue     *value,
                                  GParamSpec *pspec)
{
  BobguiMountOperation *operation = BOBGUI_MOUNT_OPERATION (object);
  BobguiMountOperationPrivate *priv = operation->priv;

  switch (prop_id)
    {
    case PROP_PARENT:
      g_value_set_object (value, priv->parent_window);
      break;

    case PROP_IS_SHOWING:
      g_value_set_boolean (value, priv->dialog != NULL || priv->handler_showing);
      break;

    case PROP_DISPLAY:
      g_value_set_object (value, priv->display);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bobgui_mount_operation_proxy_finish (BobguiMountOperation     *op,
                                  GMountOperationResult  result)
{
  _bobgui_mount_operation_handler_call_close (op->priv->handler, NULL, NULL, NULL);

  op->priv->handler_showing = FALSE;
  g_object_notify (G_OBJECT (op), "is-showing");

  g_mount_operation_reply (G_MOUNT_OPERATION (op), result);

  /* drop the reference acquired when calling the proxy method */
  g_object_unref (op);
}

static void
remember_button_toggled (BobguiCheckButton    *button,
                         BobguiMountOperation *operation)
{
  BobguiMountOperationPrivate *priv = operation->priv;

  if (bobgui_check_button_get_active (button))
    {
      gpointer data;

      data = g_object_get_data (G_OBJECT (button), "password-save");
      priv->password_save = GPOINTER_TO_INT (data);
    }
}

static void
pw_dialog_got_response (BobguiDialog         *dialog,
                        int                response_id,
                        BobguiMountOperation *mount_op)
{
  BobguiMountOperationPrivate *priv = mount_op->priv;
  GMountOperation *op = G_MOUNT_OPERATION (mount_op);

  if (response_id == BOBGUI_RESPONSE_OK)
    {
      const char *text;

      if (priv->ask_flags & G_ASK_PASSWORD_ANONYMOUS_SUPPORTED)
        g_mount_operation_set_anonymous (op, priv->anonymous);

      if (priv->username_entry)
        {
          text = bobgui_editable_get_text (BOBGUI_EDITABLE (priv->username_entry));
          g_mount_operation_set_username (op, text);
        }

      if (priv->domain_entry)
        {
          text = bobgui_editable_get_text (BOBGUI_EDITABLE (priv->domain_entry));
          g_mount_operation_set_domain (op, text);
        }

      if (priv->password_entry)
        {
          text = bobgui_editable_get_text (BOBGUI_EDITABLE (priv->password_entry));
          g_mount_operation_set_password (op, text);
        }

      if (priv->pim_entry)
        {
          text = bobgui_editable_get_text (BOBGUI_EDITABLE (priv->pim_entry));
          if (text && strlen (text) > 0)
            {
              guint64 pim;
              char *end = NULL;

              errno = 0;
              pim = g_ascii_strtoull (text, &end, 10);
              if (errno == 0 && pim <= G_MAXUINT && end != text)
                g_mount_operation_set_pim (op, (guint) pim);
            }
        }

      if (priv->tcrypt_hidden_toggle && bobgui_check_button_get_active (BOBGUI_CHECK_BUTTON (priv->tcrypt_hidden_toggle)))
        g_mount_operation_set_is_tcrypt_hidden_volume (op, TRUE);

      if (priv->tcrypt_system_toggle && bobgui_check_button_get_active (BOBGUI_CHECK_BUTTON (priv->tcrypt_system_toggle)))
        g_mount_operation_set_is_tcrypt_system_volume (op, TRUE);

      if (priv->ask_flags & G_ASK_PASSWORD_SAVING_SUPPORTED)
        g_mount_operation_set_password_save (op, priv->password_save);

      g_mount_operation_reply (op, G_MOUNT_OPERATION_HANDLED);
    }
  else
    g_mount_operation_reply (op, G_MOUNT_OPERATION_ABORTED);

  if (priv->user_widgets)
    g_list_free (priv->user_widgets);

  priv->user_widgets = NULL;
  priv->dialog = NULL;
  g_object_notify (G_OBJECT (op), "is-showing");
  bobgui_window_destroy (BOBGUI_WINDOW (dialog));
  g_object_unref (op);
}

static gboolean
entry_has_input (BobguiWidget *entry_widget)
{
  const char *text;

  if (entry_widget == NULL)
    return TRUE;

  text = bobgui_editable_get_text (BOBGUI_EDITABLE (entry_widget));

  return text != NULL && text[0] != '\0';
}

static gboolean
pim_entry_is_valid (BobguiWidget *entry_widget)
{
  const char *text;
  char *end = NULL;
  guint64 pim;

  if (entry_widget == NULL)
    return TRUE;

  text = bobgui_editable_get_text (BOBGUI_EDITABLE (entry_widget));
  /* An empty PIM entry is OK */
  if (text == NULL || text[0] == '\0')
    return TRUE;

  errno = 0;
  pim = g_ascii_strtoull (text, &end, 10);
  if (errno || pim > G_MAXUINT || end == text)
    return FALSE;
  else
    return TRUE;
}

static gboolean
pw_dialog_input_is_valid (BobguiMountOperation *operation)
{
  BobguiMountOperationPrivate *priv = operation->priv;
  gboolean is_valid = TRUE;

  /* We don't require password to be non-empty here
   * since there are situations where it is not needed,
   * see bug 578365.
   * We may add a way for the backend to specify that it
   * definitively needs a password.
   */
  is_valid = entry_has_input (priv->username_entry) &&
             entry_has_input (priv->domain_entry) &&
             pim_entry_is_valid (priv->pim_entry);

  return is_valid;
}

static void
pw_dialog_verify_input (BobguiEditable       *editable,
                        BobguiMountOperation *operation)
{
  BobguiMountOperationPrivate *priv = operation->priv;
  gboolean is_valid;

  is_valid = pw_dialog_input_is_valid (operation);
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  bobgui_dialog_set_response_sensitive (BOBGUI_DIALOG (priv->dialog),
                                     BOBGUI_RESPONSE_OK,
                                     is_valid);
G_GNUC_END_IGNORE_DEPRECATIONS
}

static void
pw_dialog_anonymous_toggled (BobguiWidget         *widget,
                             BobguiMountOperation *operation)
{
  BobguiMountOperationPrivate *priv = operation->priv;
  gboolean is_valid;
  GList *l;

  priv->anonymous = widget == priv->anonymous_toggle;

  if (priv->anonymous)
    is_valid = TRUE;
  else
    is_valid = pw_dialog_input_is_valid (operation);

  for (l = priv->user_widgets; l != NULL; l = l->next)
    {
      bobgui_widget_set_sensitive (BOBGUI_WIDGET (l->data), !priv->anonymous);
    }

G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  bobgui_dialog_set_response_sensitive (BOBGUI_DIALOG (priv->dialog),
                                     BOBGUI_RESPONSE_OK,
                                     is_valid);
G_GNUC_END_IGNORE_DEPRECATIONS
}


static void
pw_dialog_cycle_focus (BobguiWidget         *widget,
                       BobguiMountOperation *operation)
{
  BobguiMountOperationPrivate *priv;
  BobguiWidget *next_widget = NULL;

  priv = operation->priv;

  if (widget == priv->username_entry)
    {
      if (priv->domain_entry != NULL)
        next_widget = priv->domain_entry;
      else if (priv->password_entry != NULL)
        next_widget = priv->password_entry;
    }
  else if (widget == priv->domain_entry && priv->password_entry)
    next_widget = priv->password_entry;

  if (next_widget)
    bobgui_widget_grab_focus (next_widget);
  else if (pw_dialog_input_is_valid (operation))
    bobgui_widget_activate_default (widget);
}

static BobguiWidget *
table_add_entry (BobguiMountOperation *operation,
                 int                row,
                 const char        *label_text,
                 const char        *value,
                 gpointer           user_data)
{
  BobguiWidget *entry;
  BobguiWidget *label;

  label = bobgui_label_new_with_mnemonic (label_text);
  bobgui_widget_set_halign (label, BOBGUI_ALIGN_END);
  bobgui_widget_set_valign (label, BOBGUI_ALIGN_CENTER);
  bobgui_widget_set_hexpand (label, FALSE);
  operation->priv->user_widgets = g_list_prepend (operation->priv->user_widgets, label);

  entry = bobgui_entry_new ();
  bobgui_widget_set_hexpand (entry, TRUE);

  if (value)
    bobgui_editable_set_text (BOBGUI_EDITABLE (entry), value);

  bobgui_grid_attach (BOBGUI_GRID (operation->priv->grid), label, 0, row, 1, 1);
  bobgui_grid_attach (BOBGUI_GRID (operation->priv->grid), entry, 1, row, 1, 1);
  bobgui_label_set_mnemonic_widget (BOBGUI_LABEL (label), entry);
  operation->priv->user_widgets = g_list_prepend (operation->priv->user_widgets, entry);

  g_signal_connect (entry, "changed",
                    G_CALLBACK (pw_dialog_verify_input), user_data);

  g_signal_connect (entry, "activate",
                    G_CALLBACK (pw_dialog_cycle_focus), user_data);

  return entry;
}

static void
bobgui_mount_operation_ask_password_do_bobgui (BobguiMountOperation *operation,
                                         const char        *message,
                                         const char        *default_user,
                                         const char        *default_domain)
{
  BobguiMountOperationPrivate *priv;
  BobguiWidget *widget;
  BobguiDialog *dialog;
  BobguiWindow *window;
  BobguiWidget *hbox, *main_vbox, *icon;
  BobguiWidget *grid;
  BobguiWidget *label;
  BobguiWidget *content_area;
  gboolean   can_anonymous;
  guint      rows;
  char *primary;
  const char *secondary;
  gboolean use_header;

  priv = operation->priv;

  g_object_get (bobgui_settings_get_default (),
                "bobgui-dialogs-use-header", &use_header,
                NULL);
  widget = g_object_new (BOBGUI_TYPE_DIALOG,
                         "use-header-bar", use_header,
                         NULL);
  dialog = BOBGUI_DIALOG (widget);
  window = BOBGUI_WINDOW (widget);

  priv->dialog = dialog;

G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  content_area = bobgui_dialog_get_content_area (dialog);

  bobgui_window_set_resizable (window, FALSE);
  bobgui_window_set_title (window, "");
  bobgui_window_set_icon_name (window, "dialog-password");

  bobgui_dialog_add_buttons (dialog,
                          _("_Cancel"), BOBGUI_RESPONSE_CANCEL,
                          _("Co_nnect"), BOBGUI_RESPONSE_OK,
                          NULL);
  bobgui_dialog_set_default_response (dialog, BOBGUI_RESPONSE_OK);
G_GNUC_END_IGNORE_DEPRECATIONS


  /* Build contents */
  hbox = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 12);
  g_object_set (hbox,
                "margin-start", 12,
                "margin-end", 12,
                "margin-top", 12,
                "margin-bottom", 12,
                NULL);
  bobgui_box_append (BOBGUI_BOX (content_area), hbox);

  icon = bobgui_image_new_from_icon_name ("dialog-password");
  bobgui_image_set_icon_size (BOBGUI_IMAGE (icon), BOBGUI_ICON_SIZE_LARGE);

  bobgui_widget_set_halign (icon, BOBGUI_ALIGN_CENTER);
  bobgui_widget_set_valign (icon, BOBGUI_ALIGN_START);
  bobgui_box_append (BOBGUI_BOX (hbox), icon);

  main_vbox = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 18);
  bobgui_box_append (BOBGUI_BOX (hbox), main_vbox);

  secondary = strstr (message, "\n");
  if (secondary)
    {
      primary = g_strndup (message, secondary - message);
      secondary++;
    }
  else
    primary = NULL;

  label = bobgui_label_new (primary != NULL ? primary : message);
  bobgui_widget_set_halign (label, BOBGUI_ALIGN_START);
  bobgui_widget_set_valign (label, BOBGUI_ALIGN_CENTER);
  bobgui_label_set_wrap (BOBGUI_LABEL (label), TRUE);
  bobgui_box_append (BOBGUI_BOX (main_vbox), BOBGUI_WIDGET (label));
  g_free (primary);
  bobgui_widget_add_css_class (label, "title-3");

  if (secondary != NULL)
    {
      label = bobgui_label_new (secondary);
      bobgui_widget_set_halign (label, BOBGUI_ALIGN_START);
      bobgui_widget_set_valign (label, BOBGUI_ALIGN_CENTER);
      bobgui_label_set_wrap (BOBGUI_LABEL (label), TRUE);
      bobgui_box_append (BOBGUI_BOX (main_vbox), BOBGUI_WIDGET (label));
    }

  grid = bobgui_grid_new ();
  operation->priv->grid = grid;
  bobgui_grid_set_row_spacing (BOBGUI_GRID (grid), 12);
  bobgui_grid_set_column_spacing (BOBGUI_GRID (grid), 12);
  bobgui_widget_set_margin_bottom (grid, 12);
  bobgui_box_append (BOBGUI_BOX (main_vbox), grid);

  can_anonymous = priv->ask_flags & G_ASK_PASSWORD_ANONYMOUS_SUPPORTED;

  rows = 0;

  priv->anonymous_toggle = NULL;
  if (can_anonymous)
    {
      BobguiWidget *anon_box;
      BobguiWidget *choice;

      label = bobgui_label_new (_("Connect As"));
      bobgui_widget_set_halign (label, BOBGUI_ALIGN_END);
      bobgui_widget_set_valign (label, BOBGUI_ALIGN_START);
      bobgui_widget_set_hexpand (label, FALSE);
      bobgui_grid_attach (BOBGUI_GRID (grid), label, 0, rows, 1, 1);

      anon_box = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 0);
      bobgui_grid_attach (BOBGUI_GRID (grid), anon_box, 1, rows++, 1, 1);

      choice = bobgui_check_button_new_with_mnemonic (_("_Anonymous"));
      bobgui_check_button_set_active (BOBGUI_CHECK_BUTTON (choice), TRUE);
      bobgui_box_append (BOBGUI_BOX (anon_box), choice);
      g_signal_connect (choice, "toggled",
                        G_CALLBACK (pw_dialog_anonymous_toggled), operation);
      priv->anonymous_toggle = choice;

      choice = bobgui_check_button_new_with_mnemonic (_("Registered U_ser"));
      bobgui_check_button_set_group (BOBGUI_CHECK_BUTTON (choice), BOBGUI_CHECK_BUTTON (priv->anonymous_toggle));
      bobgui_box_append (BOBGUI_BOX (anon_box), choice);
      g_signal_connect (choice, "toggled",
                        G_CALLBACK (pw_dialog_anonymous_toggled), operation);
    }

  priv->username_entry = NULL;

  if (priv->ask_flags & G_ASK_PASSWORD_NEED_USERNAME)
    priv->username_entry = table_add_entry (operation, rows++, _("_Username"),
                                            default_user, operation);

  priv->domain_entry = NULL;
  if (priv->ask_flags & G_ASK_PASSWORD_NEED_DOMAIN)
    priv->domain_entry = table_add_entry (operation, rows++, _("_Domain"),
                                          default_domain, operation);

  priv->pim_entry = NULL;
  if (priv->ask_flags & G_ASK_PASSWORD_TCRYPT)
    {
      BobguiWidget *volume_type_label;
      BobguiWidget *volume_type_box;

      volume_type_label = bobgui_label_new (_("Volume type"));
      bobgui_widget_set_halign (volume_type_label, BOBGUI_ALIGN_END);
      bobgui_widget_set_hexpand (volume_type_label, FALSE);
      bobgui_grid_attach (BOBGUI_GRID (grid), volume_type_label, 0, rows, 1, 1);
      priv->user_widgets = g_list_prepend (priv->user_widgets, volume_type_label);

      volume_type_box =  bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 10);
      bobgui_grid_attach (BOBGUI_GRID (grid), volume_type_box, 1, rows++, 1, 1);
      priv->user_widgets = g_list_prepend (priv->user_widgets, volume_type_box);

      priv->tcrypt_hidden_toggle = bobgui_check_button_new_with_mnemonic (_("_Hidden"));
      bobgui_box_append (BOBGUI_BOX (volume_type_box), priv->tcrypt_hidden_toggle);

      priv->tcrypt_system_toggle = bobgui_check_button_new_with_mnemonic (_("_Windows system"));
      bobgui_box_append (BOBGUI_BOX (volume_type_box), priv->tcrypt_system_toggle);

      priv->pim_entry = table_add_entry (operation, rows++, _("_PIM"), NULL, operation);
    }

  priv->password_entry = NULL;
  if (priv->ask_flags & G_ASK_PASSWORD_NEED_PASSWORD)
    {
      priv->password_entry = table_add_entry (operation, rows++, _("_Password"),
                                              NULL, operation);
      bobgui_entry_set_visibility (BOBGUI_ENTRY (priv->password_entry), FALSE);
      bobgui_entry_set_input_purpose (BOBGUI_ENTRY (priv->password_entry), BOBGUI_INPUT_PURPOSE_PASSWORD);
    }

   if (priv->ask_flags & G_ASK_PASSWORD_SAVING_SUPPORTED)
    {
      BobguiWidget    *remember_box;
      BobguiWidget    *choice;
      BobguiWidget    *group;
      GPasswordSave password_save;

      remember_box = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 0);
      bobgui_grid_attach (BOBGUI_GRID (grid), remember_box, 0, rows++, 2, 1);
      priv->user_widgets = g_list_prepend (priv->user_widgets, remember_box);

      label = bobgui_label_new ("");
      bobgui_box_append (BOBGUI_BOX (remember_box), label);

      password_save = g_mount_operation_get_password_save (G_MOUNT_OPERATION (operation));
      priv->password_save = password_save;

      choice = bobgui_check_button_new_with_mnemonic (_("Forget password _immediately"));
      bobgui_check_button_set_active (BOBGUI_CHECK_BUTTON (choice),
                                   password_save == G_PASSWORD_SAVE_NEVER);
      g_object_set_data (G_OBJECT (choice), "password-save",
                         GINT_TO_POINTER (G_PASSWORD_SAVE_NEVER));
      g_signal_connect (choice, "toggled",
                        G_CALLBACK (remember_button_toggled), operation);
      bobgui_box_append (BOBGUI_BOX (remember_box), choice);
      group = choice;

      choice = bobgui_check_button_new_with_mnemonic (_("Remember password until you _logout"));
      bobgui_check_button_set_group (BOBGUI_CHECK_BUTTON (choice), BOBGUI_CHECK_BUTTON (group));
      bobgui_check_button_set_active (BOBGUI_CHECK_BUTTON (choice),
                                   password_save == G_PASSWORD_SAVE_FOR_SESSION);
      g_object_set_data (G_OBJECT (choice), "password-save",
                         GINT_TO_POINTER (G_PASSWORD_SAVE_FOR_SESSION));
      g_signal_connect (choice, "toggled",
                        G_CALLBACK (remember_button_toggled), operation);
      bobgui_box_append (BOBGUI_BOX (remember_box), choice);
      group = choice;

      choice = bobgui_check_button_new_with_mnemonic (_("Remember _forever"));
      bobgui_check_button_set_group (BOBGUI_CHECK_BUTTON (choice), BOBGUI_CHECK_BUTTON (group));
      bobgui_check_button_set_active (BOBGUI_CHECK_BUTTON (choice),
                                   password_save == G_PASSWORD_SAVE_PERMANENTLY);
      g_object_set_data (G_OBJECT (choice), "password-save",
                         GINT_TO_POINTER (G_PASSWORD_SAVE_PERMANENTLY));
      g_signal_connect (choice, "toggled",
                        G_CALLBACK (remember_button_toggled), operation);
      bobgui_box_append (BOBGUI_BOX (remember_box), choice);
    }

  g_signal_connect (G_OBJECT (dialog), "response",
                    G_CALLBACK (pw_dialog_got_response), operation);

  if (can_anonymous)
    {
      /* The anonymous option will be active by default,
       * ensure the toggled signal is emitted for it.
       */
      g_signal_emit_by_name (priv->anonymous_toggle, "toggled");
    }
  else if (! pw_dialog_input_is_valid (operation))
    {
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
      bobgui_dialog_set_response_sensitive (dialog, BOBGUI_RESPONSE_OK, FALSE);
G_GNUC_END_IGNORE_DEPRECATIONS
    }

  g_object_notify (G_OBJECT (operation), "is-showing");

  if (priv->parent_window)
    {
      bobgui_window_set_transient_for (window, priv->parent_window);
      bobgui_window_set_modal (window, TRUE);
    }
  else if (priv->display)
    bobgui_window_set_display (BOBGUI_WINDOW (dialog), priv->display);

  bobgui_window_present (BOBGUI_WINDOW (dialog));

  g_object_ref (operation);
}

static void
call_password_proxy_cb (GObject      *source,
                        GAsyncResult *res,
                        gpointer      user_data)
{
  _BobguiMountOperationHandler *proxy = _BOBGUI_MOUNT_OPERATION_HANDLER (source);
  GMountOperation *op = user_data;
  guint result;
  GVariant *result_details;
  GVariantIter iter;
  const char *key;
  GVariant *value;
  GError *error = NULL;

  if (!_bobgui_mount_operation_handler_call_ask_password_finish (proxy,
                                                              &result,
                                                              &result_details,
                                                              res,
                                                              &error))
    {
      result = G_MOUNT_OPERATION_ABORTED;
      g_warning ("Shell mount operation error: %s", error->message);
      g_error_free (error);
      goto out;
    }

  g_variant_iter_init (&iter, result_details);
  while (g_variant_iter_loop (&iter, "{&sv}", &key, &value))
    {
      if (strcmp (key, "password") == 0)
        g_mount_operation_set_password (op, g_variant_get_string (value, NULL));
      else if (strcmp (key, "password_save") == 0)
        g_mount_operation_set_password_save (op, g_variant_get_uint32 (value));
      else if (strcmp (key, "hidden_volume") == 0)
        g_mount_operation_set_is_tcrypt_hidden_volume (op, g_variant_get_boolean (value));
      else if (strcmp (key, "system_volume") == 0)
        g_mount_operation_set_is_tcrypt_system_volume (op, g_variant_get_boolean (value));
      else if (strcmp (key, "pim") == 0)
        g_mount_operation_set_pim (op, g_variant_get_uint32 (value));
    }

 out:
  bobgui_mount_operation_proxy_finish (BOBGUI_MOUNT_OPERATION (op), result);
}

static void
bobgui_mount_operation_ask_password_do_proxy (BobguiMountOperation *operation,
                                           const char        *message,
                                           const char        *default_user,
                                           const char        *default_domain)
{
  char id[255];
  g_sprintf(id, "BobguiMountOperation%p", operation);

  operation->priv->handler_showing = TRUE;
  g_object_notify (G_OBJECT (operation), "is-showing");

  /* keep a ref to the operation while the handler is showing */
  g_object_ref (operation);

  _bobgui_mount_operation_handler_call_ask_password (operation->priv->handler, id,
                                                  message, "drive-harddisk",
                                                  default_user, default_domain,
                                                  operation->priv->ask_flags, NULL,
                                                  call_password_proxy_cb, operation);
}

static void
bobgui_mount_operation_ask_password (GMountOperation   *mount_op,
                                  const char        *message,
                                  const char        *default_user,
                                  const char        *default_domain,
                                  GAskPasswordFlags  flags)
{
  BobguiMountOperation *operation;
  BobguiMountOperationPrivate *priv;
  gboolean use_bobgui;

  operation = BOBGUI_MOUNT_OPERATION (mount_op);
  priv = operation->priv;
  priv->ask_flags = flags;

  use_bobgui = (operation->priv->handler == NULL) ||
    (priv->ask_flags & G_ASK_PASSWORD_NEED_DOMAIN) ||
    (priv->ask_flags & G_ASK_PASSWORD_NEED_USERNAME);

  if (use_bobgui)
    bobgui_mount_operation_ask_password_do_bobgui (operation, message, default_user, default_domain);
  else
    bobgui_mount_operation_ask_password_do_proxy (operation, message, default_user, default_domain);
}

static void
question_dialog_button_clicked (GObject      *source,
                                GAsyncResult *result,
                                void         *user_data)
{
  BobguiAlertDialog *dialog = BOBGUI_ALERT_DIALOG (source);
  GMountOperation *op = user_data;
  BobguiMountOperation *operation;
  int button;

  operation = BOBGUI_MOUNT_OPERATION (op);

  button = bobgui_alert_dialog_choose_finish (dialog, result, NULL);
  if (button >= 0)
    {
      g_mount_operation_set_choice (op, button);
      g_mount_operation_reply (op, G_MOUNT_OPERATION_HANDLED);
    }
  else
    g_mount_operation_reply (op, G_MOUNT_OPERATION_ABORTED);

  g_object_notify (G_OBJECT (operation), "is-showing");
  g_object_unref (op);
}

static void
bobgui_mount_operation_ask_question_do_bobgui (BobguiMountOperation *op,
                                         const char        *message,
                                         const char        *choices[])
{
  BobguiMountOperationPrivate *priv;
  BobguiAlertDialog *dialog;
  const char *secondary;
  char       *primary;

  g_return_if_fail (BOBGUI_IS_MOUNT_OPERATION (op));
  g_return_if_fail (message != NULL);
  g_return_if_fail (choices != NULL);

  priv = op->priv;

  secondary = strstr (message, "\n");
  if (secondary)
    {
      primary = g_strndup (message, secondary - message);
      secondary++;
    }
  else
    primary = NULL;

  dialog = bobgui_alert_dialog_new ("%s", primary ? primary : message);
  if (secondary)
    bobgui_alert_dialog_set_detail (dialog, secondary);

  bobgui_alert_dialog_set_buttons (dialog, choices);

  bobgui_alert_dialog_choose (dialog, priv->parent_window,
                           NULL,
                           question_dialog_button_clicked, g_object_ref (op));
  g_object_unref (dialog);
  g_free (primary);

  g_object_notify (G_OBJECT (op), "is-showing");
}

static void
call_question_proxy_cb (GObject      *source,
                        GAsyncResult *res,
                        gpointer      user_data)
{
  _BobguiMountOperationHandler *proxy = _BOBGUI_MOUNT_OPERATION_HANDLER (source);
  GMountOperation *op = user_data;
  guint result;
  GVariant *result_details;
  GVariantIter iter;
  const char *key;
  GVariant *value;
  GError *error = NULL;

  if (!_bobgui_mount_operation_handler_call_ask_question_finish (proxy,
                                                              &result,
                                                              &result_details,
                                                              res,
                                                              &error))
    {
      result = G_MOUNT_OPERATION_ABORTED;
      g_warning ("Shell mount operation error: %s", error->message);
      g_error_free (error);
      goto out;
    }

  g_variant_iter_init (&iter, result_details);
  while (g_variant_iter_loop (&iter, "{&sv}", &key, &value))
    {
      if (strcmp (key, "choice") == 0)
        g_mount_operation_set_choice (op, g_variant_get_int32 (value));
    }
 
 out:
  bobgui_mount_operation_proxy_finish (BOBGUI_MOUNT_OPERATION (op), result);
}

static void
bobgui_mount_operation_ask_question_do_proxy (BobguiMountOperation *operation,
                                           const char        *message,
                                           const char        *choices[])
{
  char id[255];
  g_sprintf(id, "BobguiMountOperation%p", operation);

  operation->priv->handler_showing = TRUE;
  g_object_notify (G_OBJECT (operation), "is-showing");

  /* keep a ref to the operation while the handler is showing */
  g_object_ref (operation);

  _bobgui_mount_operation_handler_call_ask_question (operation->priv->handler, id,
                                                  message, "drive-harddisk",
                                                  choices, NULL,
                                                  call_question_proxy_cb, operation);
}

static void
bobgui_mount_operation_ask_question (GMountOperation *op,
                                  const char      *message,
                                  const char      *choices[])
{
  BobguiMountOperation *operation;
  gboolean use_bobgui;

  operation = BOBGUI_MOUNT_OPERATION (op);
  use_bobgui = (operation->priv->handler == NULL);

  if (use_bobgui)
    bobgui_mount_operation_ask_question_do_bobgui (operation, message, choices);
  else
    bobgui_mount_operation_ask_question_do_proxy (operation, message, choices);
}

static void
show_processes_button_clicked (BobguiWidget       *button,
                               GMountOperation *op)
{
  BobguiMountOperationPrivate *priv;
  BobguiMountOperation *operation;
  int button_number;
  BobguiDialog *dialog;

  button_number = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (button), "choice"));
  dialog = BOBGUI_DIALOG (bobgui_widget_get_ancestor (button, BOBGUI_TYPE_DIALOG));


  operation = BOBGUI_MOUNT_OPERATION (op);
  priv = operation->priv;

  if (button_number >= 0)
    {
      g_mount_operation_set_choice (op, button_number);
      g_mount_operation_reply (op, G_MOUNT_OPERATION_HANDLED);
    }
  else
    g_mount_operation_reply (op, G_MOUNT_OPERATION_ABORTED);

  priv->dialog = NULL;
  g_object_notify (G_OBJECT (operation), "is-showing");
  bobgui_window_destroy (BOBGUI_WINDOW (dialog));
  g_object_unref (op);
}

static int
pid_equal (gconstpointer a,
           gconstpointer b)
{
  GPid pa, pb;

  pa = *((GPid *) a);
  pb = *((GPid *) b);

  return GPOINTER_TO_INT(pb) - GPOINTER_TO_INT(pa);
}

static void
diff_sorted_arrays (GArray         *array1,
                    GArray         *array2,
                    GCompareFunc   compare,
                    GArray         *added_indices,
                    GArray         *removed_indices)
{
  int order;
  guint n1, n2;
  guint elem_size;

  n1 = n2 = 0;

  elem_size = g_array_get_element_size (array1);
  g_assert (elem_size == g_array_get_element_size (array2));

  while (n1 < array1->len && n2 < array2->len)
    {
      order = (*compare) (((const char*) array1->data) + n1 * elem_size,
                          ((const char*) array2->data) + n2 * elem_size);
      if (order < 0)
        {
          g_array_append_val (removed_indices, n1);
          n1++;
        }
      else if (order > 0)
        {
          g_array_append_val (added_indices, n2);
          n2++;
        }
      else
        { /* same item */
          n1++;
          n2++;
        }
    }

  while (n1 < array1->len)
    {
      g_array_append_val (removed_indices, n1);
      n1++;
    }
  while (n2 < array2->len)
    {
      g_array_append_val (added_indices, n2);
      n2++;
    }
}

static GdkTexture *
render_paintable_to_texture (GdkPaintable *paintable)
{
  BobguiSnapshot *snapshot;
  GskRenderNode *node;
  int width, height;
  cairo_surface_t *surface;
  cairo_t *cr;
  GdkTexture *texture;

  width = gdk_paintable_get_intrinsic_width (paintable);
  height = gdk_paintable_get_intrinsic_height (paintable);

  surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, width, height);

  snapshot = bobgui_snapshot_new ();
  gdk_paintable_snapshot (paintable, snapshot, width, height);
  node = bobgui_snapshot_free_to_node (snapshot);

  cr = cairo_create (surface);
  gsk_render_node_draw (node, cr);
  cairo_destroy (cr);

  gsk_render_node_unref (node);

  texture = gdk_texture_new_for_surface (surface);
  cairo_surface_destroy (surface);

  return texture;
}

typedef struct _ProcessData ProcessData;

G_DECLARE_FINAL_TYPE (ProcessData, process_data, PROCESS, DATA, GObject);

struct _ProcessData
{
  GObject parent;

  GdkTexture *texture;
  char *name;
  GPid pid;
};

G_DEFINE_TYPE (ProcessData, process_data, G_TYPE_OBJECT);

static void
process_data_init (ProcessData *self)
{
}

static void
process_data_finalize (GObject *object)
{
  ProcessData *pd = PROCESS_DATA (object);

  g_free (pd->name);
  g_object_unref (pd->texture);

  G_OBJECT_CLASS (process_data_parent_class)->finalize (object);
}

static void
process_data_class_init (ProcessDataClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  object_class->finalize = process_data_finalize;
}

static ProcessData *
process_data_new (const char *name,
                  GPid        pid,
                  GdkTexture *texture)
{
  ProcessData *self;

  self = g_object_new (process_data_get_type (), NULL);
  self->name = g_strdup (name);
  self->pid = pid;
  g_set_object (&self->texture, texture);

  return self;
}

static void
add_pid_to_process_list_store (BobguiMountOperation              *mount_operation,
                               BobguiMountOperationLookupContext *lookup_context,
                               GListStore                     *list_store,
                               GPid                            pid)
{
  char *command_line;
  char *name;
  GdkTexture *texture;
  char *markup;

  name = NULL;
  texture = NULL;
  command_line = NULL;
  _bobgui_mount_operation_lookup_info (lookup_context,
                                    pid,
                                    24,
                                    &name,
                                    &command_line,
                                    &texture);

  if (name == NULL)
    name = g_strdup_printf (_("Unknown Application (PID %d)"), (int) (gssize) pid);

  if (command_line == NULL)
    command_line = g_strdup ("");

  if (texture == NULL)
    {
      BobguiIconTheme *theme;
      BobguiIconPaintable *icon;

      theme = bobgui_icon_theme_get_for_display (bobgui_widget_get_display (BOBGUI_WIDGET (mount_operation->priv->dialog)));
      icon = bobgui_icon_theme_lookup_icon (theme,
                                         "application-x-executable",
                                         NULL,
                                         24, 1,
                                         bobgui_widget_get_direction (BOBGUI_WIDGET (mount_operation->priv->dialog)),
                                         0);
      texture = render_paintable_to_texture (GDK_PAINTABLE (icon));
      g_object_unref (icon);
    }

  markup = g_strdup_printf ("<b>%s</b>\n"
                            "<small>%s</small>",
                            name,
                            command_line);

  g_list_store_append (list_store, process_data_new (markup, pid, texture));

  if (texture != NULL)
    g_object_unref (texture);
  g_free (markup);
  g_free (name);
  g_free (command_line);
}

static void
remove_pid_from_process_list_store (BobguiMountOperation *mount_operation,
                                    GListStore        *list_store,
                                    GPid               pid)
{
  for (guint i = 0; i < g_list_model_get_n_items (G_LIST_MODEL (list_store)); i++)
    {
      ProcessData *data = g_list_model_get_item (G_LIST_MODEL (list_store), i);

      g_object_unref (data);

      if (data->pid == pid)
        {
          g_list_store_remove (list_store, i);
          break;
        }
    }
}


static void
update_process_list_store (BobguiMountOperation *mount_operation,
                           GListStore        *list_store,
                           GArray            *processes)
{
  guint n;
  BobguiMountOperationLookupContext *lookup_context;
  GArray *current_pids;
  GArray *pid_indices_to_add;
  GArray *pid_indices_to_remove;
  GPid pid;

  /* Just removing all items and adding new ones will screw up the
   * focus handling in the treeview - so compute the delta, and add/remove
   * items as appropriate
   */
  current_pids = g_array_new (FALSE, FALSE, sizeof (GPid));
  pid_indices_to_add = g_array_new (FALSE, FALSE, sizeof (int));
  pid_indices_to_remove = g_array_new (FALSE, FALSE, sizeof (int));

  for (guint i = 0; i < g_list_model_get_n_items (G_LIST_MODEL (list_store)); i++)
    {
      ProcessData *data = g_list_model_get_item (G_LIST_MODEL (list_store), i);

      g_array_append_val (current_pids, data->pid);

      g_object_unref (data);
    }

  g_array_sort (current_pids, pid_equal);
  g_array_sort (processes, pid_equal);

  diff_sorted_arrays (current_pids, processes, pid_equal, pid_indices_to_add, pid_indices_to_remove);

  for (n = 0; n < pid_indices_to_remove->len; n++)
    {
      pid = g_array_index (current_pids, GPid, n);
      remove_pid_from_process_list_store (mount_operation, list_store, pid);
    }

  if (pid_indices_to_add->len > 0)
    {
      lookup_context = _bobgui_mount_operation_lookup_context_get (bobgui_widget_get_display (mount_operation->priv->process_list_view));
      for (n = 0; n < pid_indices_to_add->len; n++)
        {
          pid = g_array_index (processes, GPid, n);
          add_pid_to_process_list_store (mount_operation, lookup_context, list_store, pid);
        }
      _bobgui_mount_operation_lookup_context_free (lookup_context);
    }

  g_array_unref (current_pids);
  g_array_unref (pid_indices_to_add);
  g_array_unref (pid_indices_to_remove);
}

static void
on_end_process_activated (BobguiButton         *button,
                          BobguiMountOperation *op)
{
  BobguiSelectionModel *selection;
  ProcessData *data;
  GError *error;

  selection = bobgui_list_view_get_model (BOBGUI_LIST_VIEW (op->priv->process_list_view));
  if (bobgui_single_selection_get_selected (BOBGUI_SINGLE_SELECTION (selection)) == BOBGUI_INVALID_LIST_POSITION)
    goto out;

  data = bobgui_single_selection_get_selected_item (BOBGUI_SINGLE_SELECTION (selection));

  /* TODO: We might want to either
   *
   *       - Be smart about things and send SIGKILL rather than SIGTERM if
   *         this is the second time the user requests killing a process
   *
   *       - Or, easier (but worse user experience), offer both "End Process"
   *         and "Terminate Process" options
   *
   *      But that's not how things work right now....
   */
  error = NULL;
  if (!_bobgui_mount_operation_kill_process (data->pid, &error))
    {
      BobguiAlertDialog *dialog;

      /* Use BOBGUI_DIALOG_DESTROY_WITH_PARENT here since the parent dialog can be
       * indeed be destroyed via the GMountOperation::abort signal... for example,
       * this is triggered if the user yanks the device while we are showing
       * the dialog...
       */
      dialog = bobgui_alert_dialog_new (_("Unable to end process"));
      bobgui_alert_dialog_set_detail (dialog, error->message);
      bobgui_alert_dialog_show (dialog, BOBGUI_WINDOW (op->priv->dialog));
      g_object_unref (dialog);

      g_error_free (error);
    }

 out:
  ;
}

static void
setup_process_row (BobguiListItemFactory *factory,
                   BobguiListItem        *item)
{
  BobguiWidget *box, *picture, *label;

  picture = bobgui_picture_new ();
  label = bobgui_label_new (NULL);
  bobgui_label_set_ellipsize (BOBGUI_LABEL (label), PANGO_ELLIPSIZE_MIDDLE);

  box = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 10);
  bobgui_box_append (BOBGUI_BOX (box), picture);
  bobgui_box_append (BOBGUI_BOX (box), label);

  bobgui_list_item_set_child (item, box);
}

static void
bind_process_row (BobguiListItemFactory *factory,
                  BobguiListItem        *item)
{
  BobguiWidget *box, *picture, *label;
  ProcessData *data;

  data = bobgui_list_item_get_item (item);

  box = bobgui_list_item_get_child (item);
  picture = bobgui_widget_get_first_child (box);
  label = bobgui_widget_get_next_sibling (picture);

  bobgui_picture_set_paintable (BOBGUI_PICTURE (picture), GDK_PAINTABLE (data->texture));
  bobgui_label_set_markup (BOBGUI_LABEL (label), data->name);
}

static BobguiWidget *
create_show_processes_dialog (BobguiMountOperation *op,
                              const char      *message,
                              const char      *choices[])
{
  BobguiMountOperationPrivate *priv;
  BobguiWidget  *dialog;
  const char *secondary;
  char       *primary;
  int        count, len = 0;
  BobguiWidget *label;
  BobguiWidget *list_view;
  BobguiWidget *scrolled_window;
  BobguiWidget *vbox;
  BobguiWidget *hbox;
  BobguiWidget *content_area;
  char *s;
  GListStore *store;
  BobguiListItemFactory *factory;
  BobguiWidget *button;

  priv = op->priv;

  secondary = strstr (message, "\n");
  if (secondary)
    {
      primary = g_strndup (message, secondary - message);
      secondary++;
    }
  else
    primary = g_strdup (message);

G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  dialog = bobgui_dialog_new ();
G_GNUC_END_IGNORE_DEPRECATIONS

  if (priv->parent_window != NULL)
    bobgui_window_set_transient_for (BOBGUI_WINDOW (dialog), priv->parent_window);
  bobgui_window_set_title (BOBGUI_WINDOW (dialog), "");

G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  content_area = bobgui_dialog_get_content_area (BOBGUI_DIALOG (dialog));
G_GNUC_END_IGNORE_DEPRECATIONS
  vbox = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 12);
  bobgui_widget_set_margin_top (vbox, 12);
  bobgui_widget_set_margin_bottom (vbox, 12);
  bobgui_widget_set_margin_start (vbox, 12);
  bobgui_widget_set_margin_end (vbox, 12);
  bobgui_box_append (BOBGUI_BOX (content_area), vbox);

  if (secondary != NULL)
    s = g_strdup_printf ("<big><b>%s</b></big>\n\n%s", primary, secondary);
  else
    s = g_strdup_printf ("%s", primary);

  g_free (primary);
  label = bobgui_label_new (NULL);
  bobgui_label_set_markup (BOBGUI_LABEL (label), s);
  g_free (s);
  bobgui_box_append (BOBGUI_BOX (vbox), label);

  /* First count the items in the list then
   * add the buttons in reverse order
   */

  while (choices[len] != NULL)
    len++;

  hbox = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 12);
  for (count = len - 1; count >= 0; count--)
    {
      button = bobgui_button_new_with_label (choices[count]);
      g_object_set_data (G_OBJECT (button), "choice", GINT_TO_POINTER (count));
      g_signal_connect (button, "clicked", G_CALLBACK (show_processes_button_clicked), op);
      bobgui_box_append (BOBGUI_BOX (hbox), button);
    }
  bobgui_widget_set_halign (hbox, BOBGUI_ALIGN_END);
  bobgui_box_append (BOBGUI_BOX (vbox), hbox);

  priv->dialog = BOBGUI_DIALOG (dialog);
  g_object_notify (G_OBJECT (op), "is-showing");

  if (priv->parent_window == NULL && priv->display)
    bobgui_window_set_display (BOBGUI_WINDOW (dialog), priv->display);

  store = g_list_store_new (process_data_get_type ());
  factory = bobgui_signal_list_item_factory_new ();

  g_signal_connect (factory, "setup", G_CALLBACK (setup_process_row), op);
  g_signal_connect (factory, "bind", G_CALLBACK (bind_process_row), op);

  list_view = bobgui_list_view_new (BOBGUI_SELECTION_MODEL (bobgui_single_selection_new (G_LIST_MODEL (store))), factory);

  bobgui_widget_set_size_request (list_view, 300, 120);

  scrolled_window = bobgui_scrolled_window_new ();
  bobgui_widget_set_vexpand (scrolled_window, TRUE);
  bobgui_scrolled_window_set_policy (BOBGUI_SCROLLED_WINDOW (scrolled_window),
                                  BOBGUI_POLICY_NEVER,
                                  BOBGUI_POLICY_AUTOMATIC);
  bobgui_scrolled_window_set_propagate_natural_height (BOBGUI_SCROLLED_WINDOW (scrolled_window), TRUE);
  bobgui_scrolled_window_set_has_frame (BOBGUI_SCROLLED_WINDOW (scrolled_window), TRUE);

  bobgui_scrolled_window_set_child (BOBGUI_SCROLLED_WINDOW (scrolled_window), list_view);
  bobgui_box_append (BOBGUI_BOX (vbox), scrolled_window);

  button = bobgui_button_new_with_mnemonic (_("_End Process"));
  bobgui_widget_set_halign (button, BOBGUI_ALIGN_END);
  g_signal_connect (button, "clicked", G_CALLBACK (on_end_process_activated), op);
  bobgui_box_append (BOBGUI_BOX (vbox), button);

  priv->process_list_store = store;
  priv->process_list_view = list_view;
  /* set pointers to NULL when dialog goes away */
  g_object_add_weak_pointer (G_OBJECT (priv->process_list_store), (gpointer *) &priv->process_list_store);
  g_object_add_weak_pointer (G_OBJECT (priv->process_list_view), (gpointer *) &priv->process_list_view);

  g_object_ref (op);

  return dialog;
}

static void
call_processes_proxy_cb (GObject     *source,
                        GAsyncResult *res,
                        gpointer      user_data)
{
  _BobguiMountOperationHandler *proxy = _BOBGUI_MOUNT_OPERATION_HANDLER (source);
  GMountOperation *op = user_data;
  guint result;
  GVariant *result_details;
  GVariantIter iter;
  const char *key;
  GVariant *value;
  GError *error = NULL;

  if (!_bobgui_mount_operation_handler_call_show_processes_finish (proxy,
                                                                &result,
                                                                &result_details,
                                                                res,
                                                                &error))
    {
      result = G_MOUNT_OPERATION_ABORTED;
      g_warning ("Shell mount operation error: %s", error->message);
      g_error_free (error);
      goto out;
    }

  /* If the request was unhandled it means we called the method again;
   * in this case, just return and wait for the next response.
   */
  if (result == G_MOUNT_OPERATION_UNHANDLED)
    return;

  g_variant_iter_init (&iter, result_details);
  while (g_variant_iter_loop (&iter, "{&sv}", &key, &value))
    {
      if (strcmp (key, "choice") == 0)
        g_mount_operation_set_choice (op, g_variant_get_int32 (value));
    }

 out:
  bobgui_mount_operation_proxy_finish (BOBGUI_MOUNT_OPERATION (op), result);
}

static void
bobgui_mount_operation_show_processes_do_proxy (BobguiMountOperation *operation,
                                             const char        *message,
                                             GArray            *processes,
                                             const char        *choices[])
{
  char id[255];
  g_sprintf(id, "BobguiMountOperation%p", operation);

  operation->priv->handler_showing = TRUE;
  g_object_notify (G_OBJECT (operation), "is-showing");

  /* keep a ref to the operation while the handler is showing */
  g_object_ref (operation);

  _bobgui_mount_operation_handler_call_show_processes (operation->priv->handler, id,
                                                    message, "drive-harddisk",
                                                    g_variant_new_fixed_array (G_VARIANT_TYPE_INT32,
                                                                               processes->data, processes->len,
                                                                               sizeof (GPid)),
                                                    choices, NULL,
                                                    call_processes_proxy_cb, operation);
}

static void
bobgui_mount_operation_show_processes_do_bobgui (BobguiMountOperation *op,
                                           const char        *message,
                                           GArray            *processes,
                                           const char        *choices[])
{
  BobguiMountOperationPrivate *priv;
  BobguiWidget *dialog = NULL;

  g_return_if_fail (BOBGUI_IS_MOUNT_OPERATION (op));
  g_return_if_fail (message != NULL);
  g_return_if_fail (processes != NULL);
  g_return_if_fail (choices != NULL);

  priv = op->priv;

  if (priv->process_list_store == NULL)
    {
      /* need to create the dialog */
      dialog = create_show_processes_dialog (op, message, choices);
    }

  /* otherwise, we're showing the dialog, assume messages+choices hasn't changed */

  update_process_list_store (op,
                             priv->process_list_store,
                             processes);

  if (dialog != NULL)
    bobgui_window_present (BOBGUI_WINDOW (dialog));
}


static void
bobgui_mount_operation_show_processes (GMountOperation *op,
                                    const char      *message,
                                    GArray          *processes,
                                    const char      *choices[])
{

  BobguiMountOperation *operation;
  gboolean use_bobgui;

  operation = BOBGUI_MOUNT_OPERATION (op);
  use_bobgui = (operation->priv->handler == NULL);

  if (use_bobgui)
    bobgui_mount_operation_show_processes_do_bobgui (operation, message, processes, choices);
  else
    bobgui_mount_operation_show_processes_do_proxy (operation, message, processes, choices);
}

static void
bobgui_mount_operation_aborted (GMountOperation *op)
{
  BobguiMountOperationPrivate *priv;

  priv = BOBGUI_MOUNT_OPERATION (op)->priv;

  if (priv->dialog != NULL)
    {
      bobgui_window_destroy (BOBGUI_WINDOW (priv->dialog));
      priv->dialog = NULL;
      g_object_notify (G_OBJECT (op), "is-showing");
      g_object_unref (op);
    }

  if (priv->handler != NULL)
    {
      _bobgui_mount_operation_handler_call_close (priv->handler, NULL, NULL, NULL);

      priv->handler_showing = FALSE;
      g_object_notify (G_OBJECT (op), "is-showing");
    }
}

/**
 * bobgui_mount_operation_new:
 * @parent: (nullable): transient parent of the window
 *
 * Creates a new `BobguiMountOperation`.
 *
 * Returns: a new `BobguiMountOperation`
 */
GMountOperation *
bobgui_mount_operation_new (BobguiWindow *parent)
{
  GMountOperation *mount_operation;

  mount_operation = g_object_new (BOBGUI_TYPE_MOUNT_OPERATION,
                                  "parent", parent, NULL);

  return mount_operation;
}

/**
 * bobgui_mount_operation_is_showing:
 * @op: a `BobguiMountOperation`
 *
 * Returns whether the `BobguiMountOperation` is currently displaying
 * a window.
 *
 * Returns: %TRUE if @op is currently displaying a window
 */
gboolean
bobgui_mount_operation_is_showing (BobguiMountOperation *op)
{
  g_return_val_if_fail (BOBGUI_IS_MOUNT_OPERATION (op), FALSE);

  return op->priv->dialog != NULL;
}

/**
 * bobgui_mount_operation_set_parent:
 * @op: a `BobguiMountOperation`
 * @parent: (nullable): transient parent of the window
 *
 * Sets the transient parent for windows shown by the
 * `BobguiMountOperation`.
 */
void
bobgui_mount_operation_set_parent (BobguiMountOperation *op,
                                BobguiWindow         *parent)
{
  BobguiMountOperationPrivate *priv;

  g_return_if_fail (BOBGUI_IS_MOUNT_OPERATION (op));
  g_return_if_fail (parent == NULL || BOBGUI_IS_WINDOW (parent));

  priv = op->priv;

  if (priv->parent_window == parent)
    return;

  if (priv->parent_window)
    {
      g_signal_handlers_disconnect_by_func (priv->parent_window,
                                            parent_destroyed,
                                            &priv->parent_window);
      g_object_unref (priv->parent_window);
    }
  priv->parent_window = parent;
  if (priv->parent_window)
    {
      g_object_ref (priv->parent_window);
      g_signal_connect (priv->parent_window, "destroy",
                        G_CALLBACK (parent_destroyed), &priv->parent_window);
    }

  if (priv->dialog)
    bobgui_window_set_transient_for (BOBGUI_WINDOW (priv->dialog), priv->parent_window);

  g_object_notify (G_OBJECT (op), "parent");
}

/**
 * bobgui_mount_operation_get_parent:
 * @op: a `BobguiMountOperation`
 *
 * Gets the transient parent used by the `BobguiMountOperation`.
 *
 * Returns: (transfer none) (nullable): the transient parent for windows shown by @op
 */
BobguiWindow *
bobgui_mount_operation_get_parent (BobguiMountOperation *op)
{
  g_return_val_if_fail (BOBGUI_IS_MOUNT_OPERATION (op), NULL);

  return op->priv->parent_window;
}

/**
 * bobgui_mount_operation_set_display:
 * @op: a `BobguiMountOperation`
 * @display: a `GdkDisplay`
 *
 * Sets the display to show windows of the `BobguiMountOperation` on.
 */
void
bobgui_mount_operation_set_display (BobguiMountOperation *op,
                                 GdkDisplay        *display)
{
  BobguiMountOperationPrivate *priv;

  g_return_if_fail (BOBGUI_IS_MOUNT_OPERATION (op));
  g_return_if_fail (GDK_IS_DISPLAY (display));

  priv = op->priv;

  if (priv->display == display)
    return;

  if (priv->display)
    g_object_unref (priv->display);

  priv->display = g_object_ref (display);

  if (priv->dialog)
    bobgui_window_set_display (BOBGUI_WINDOW (priv->dialog), display);

  g_object_notify (G_OBJECT (op), "display");
}

/**
 * bobgui_mount_operation_get_display:
 * @op: a `BobguiMountOperation`
 *
 * Gets the display on which windows of the `BobguiMountOperation`
 * will be shown.
 *
 * Returns: (transfer none): the display on which windows of @op are shown
 */
GdkDisplay *
bobgui_mount_operation_get_display (BobguiMountOperation *op)
{
  BobguiMountOperationPrivate *priv;

  g_return_val_if_fail (BOBGUI_IS_MOUNT_OPERATION (op), NULL);

  priv = op->priv;

  if (priv->dialog)
    return bobgui_widget_get_display (BOBGUI_WIDGET (priv->dialog));
  else if (priv->parent_window)
    return bobgui_widget_get_display (BOBGUI_WIDGET (priv->parent_window));
  else if (priv->display)
    return priv->display;
  else
    return gdk_display_get_default ();
}
