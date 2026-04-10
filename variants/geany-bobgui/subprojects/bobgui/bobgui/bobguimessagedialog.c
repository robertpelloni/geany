/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 2 -*- */
/* BOBGUI - The Bobgui Framework
 * Copyright (C) 2000 Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * Modified by the BOBGUI Team and others 1997-2003.  See the AUTHORS
 * file for a list of people on the BOBGUI Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * BOBGUI at ftp://ftp.bobgui.org/pub/bobgui/.
 */

#include "config.h"

#include "deprecated/bobguimessagedialog.h"

#include "bobguibox.h"
#include "bobguibuildable.h"
#include "deprecated/bobguidialogprivate.h"
#include <glib/gi18n-lib.h>
#include "bobguilabel.h"
#include "bobguiprivate.h"
#include "bobguitypebuiltins.h"

#include <string.h>

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

/**
 * BobguiMessageDialog:
 *
 * `BobguiMessageDialog` presents a dialog with some message text.
 *
 * <picture>
 *   <source srcset="messagedialog-dark.png" media="(prefers-color-scheme: dark)">
 *   <img alt="An example BobguiMessageDialog" src="messagedialog.png">
 * </picture>
 *
 * It’s simply a convenience widget; you could construct the equivalent of
 * `BobguiMessageDialog` from `BobguiDialog` without too much effort, but
 * `BobguiMessageDialog` saves typing.
 *
 * The easiest way to do a modal message dialog is to use the %BOBGUI_DIALOG_MODAL
 * flag, which will call [method@Bobgui.Window.set_modal] internally. The dialog will
 * prevent interaction with the parent window until it's hidden or destroyed.
 * You can use the [signal@Bobgui.Dialog::response] signal to know when the user
 * dismissed the dialog.
 *
 * An example for using a modal dialog:
 * ```c
 * BobguiDialogFlags flags = BOBGUI_DIALOG_DESTROY_WITH_PARENT | BOBGUI_DIALOG_MODAL;
 * dialog = bobgui_message_dialog_new (parent_window,
 *                                  flags,
 *                                  BOBGUI_MESSAGE_ERROR,
 *                                  BOBGUI_BUTTONS_CLOSE,
 *                                  "Error reading “%s”: %s",
 *                                  filename,
 *                                  g_strerror (errno));
 * // Destroy the dialog when the user responds to it
 * // (e.g. clicks a button)
 *
 * g_signal_connect (dialog, "response",
 *                   G_CALLBACK (bobgui_window_destroy),
 *                   NULL);
 * ```
 *
 * You might do a non-modal `BobguiMessageDialog` simply by omitting the
 * %BOBGUI_DIALOG_MODAL flag:
 *
 * ```c
 * BobguiDialogFlags flags = BOBGUI_DIALOG_DESTROY_WITH_PARENT;
 * dialog = bobgui_message_dialog_new (parent_window,
 *                                  flags,
 *                                  BOBGUI_MESSAGE_ERROR,
 *                                  BOBGUI_BUTTONS_CLOSE,
 *                                  "Error reading “%s”: %s",
 *                                  filename,
 *                                  g_strerror (errno));
 *
 * // Destroy the dialog when the user responds to it
 * // (e.g. clicks a button)
 * g_signal_connect (dialog, "response",
 *                   G_CALLBACK (bobgui_window_destroy),
 *                   NULL);
 * ```
 *
 * # BobguiMessageDialog as BobguiBuildable
 *
 * The `BobguiMessageDialog` implementation of the `BobguiBuildable` interface exposes
 * the message area as an internal child with the name “message_area”.
 *
 * Deprecated: 4.10: Use [class@Bobgui.AlertDialog] instead
 */

typedef struct
{
  BobguiWidget     *label;
  BobguiWidget     *message_area; /* vbox for the primary and secondary labels, and any extra content from the caller */
  BobguiWidget     *secondary_label;

  guint          has_primary_markup : 1;
  guint          has_secondary_text : 1;
  guint          message_type       : 3;
} BobguiMessageDialogPrivate;

struct _BobguiMessageDialogClass
{
  BobguiDialogClass parent_class;
};

enum {
  PROP_0,
  PROP_MESSAGE_TYPE,
  PROP_BUTTONS,
  PROP_TEXT,
  PROP_USE_MARKUP,
  PROP_SECONDARY_TEXT,
  PROP_SECONDARY_USE_MARKUP,
  PROP_IMAGE,
  PROP_MESSAGE_AREA
};

G_DEFINE_TYPE_WITH_PRIVATE (BobguiMessageDialog, bobgui_message_dialog, BOBGUI_TYPE_DIALOG)

static void
setup_type (BobguiMessageDialog *dialog,
            BobguiMessageType    type)
{
  BobguiMessageDialogPrivate *priv = bobgui_message_dialog_get_instance_private (dialog);

  if (priv->message_type == type)
    return;

  priv->message_type = type;

  g_object_notify (G_OBJECT (dialog), "message-type");
}

static void
bobgui_message_dialog_add_buttons (BobguiMessageDialog *message_dialog,
                                BobguiButtonsType    buttons)
{
  BobguiDialog* dialog = BOBGUI_DIALOG (message_dialog);

  switch (buttons)
    {
    case BOBGUI_BUTTONS_NONE:
      /* nothing */
      break;

    case BOBGUI_BUTTONS_OK:
      bobgui_dialog_add_button (dialog, _("_OK"), BOBGUI_RESPONSE_OK);
      break;

    case BOBGUI_BUTTONS_CLOSE:
      bobgui_dialog_add_button (dialog, _("_Close"), BOBGUI_RESPONSE_CLOSE);
      break;

    case BOBGUI_BUTTONS_CANCEL:
      bobgui_dialog_add_button (dialog, _("_Cancel"), BOBGUI_RESPONSE_CANCEL);
      break;

    case BOBGUI_BUTTONS_YES_NO:
      bobgui_dialog_add_button (dialog, _("_No"), BOBGUI_RESPONSE_NO);
      bobgui_dialog_add_button (dialog, _("_Yes"), BOBGUI_RESPONSE_YES);
      break;

    case BOBGUI_BUTTONS_OK_CANCEL:
      bobgui_dialog_add_button (dialog, _("_Cancel"), BOBGUI_RESPONSE_CANCEL);
      bobgui_dialog_add_button (dialog, _("_OK"), BOBGUI_RESPONSE_OK);
      break;

    default:
      g_warning ("Unknown BobguiButtonsType");
      break;
    }

  g_object_notify (G_OBJECT (message_dialog), "buttons");
}

static void
bobgui_message_dialog_set_property (GObject      *object,
                                 guint         prop_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  BobguiMessageDialog *dialog = BOBGUI_MESSAGE_DIALOG (object);
  BobguiMessageDialogPrivate *priv = bobgui_message_dialog_get_instance_private (dialog);

  switch (prop_id)
    {
    case PROP_MESSAGE_TYPE:
      setup_type (dialog, g_value_get_enum (value));
      break;
    case PROP_BUTTONS:
      bobgui_message_dialog_add_buttons (dialog, g_value_get_enum (value));
      break;
    case PROP_TEXT:
      if (priv->has_primary_markup)
        bobgui_label_set_markup (BOBGUI_LABEL (priv->label), g_value_get_string (value));
      else
        bobgui_label_set_text (BOBGUI_LABEL (priv->label), g_value_get_string (value));
      break;
    case PROP_USE_MARKUP:
      if (priv->has_primary_markup != g_value_get_boolean (value))
        {
          priv->has_primary_markup = g_value_get_boolean (value);
          bobgui_label_set_use_markup (BOBGUI_LABEL (priv->label), priv->has_primary_markup);
          g_object_notify_by_pspec (object, pspec);
        }
      break;
    case PROP_SECONDARY_TEXT:
      {
        const char *txt = g_value_get_string (value);

        if (bobgui_label_get_use_markup (BOBGUI_LABEL (priv->secondary_label)))
          bobgui_label_set_markup (BOBGUI_LABEL (priv->secondary_label), txt);
        else
          bobgui_label_set_text (BOBGUI_LABEL (priv->secondary_label), txt);

        if (txt)
          {
            priv->has_secondary_text = TRUE;
            bobgui_widget_add_css_class (priv->label, "title");
          }
        else
          {
            priv->has_secondary_text = FALSE;
            bobgui_widget_remove_css_class (priv->label, "title");
          }

        bobgui_widget_set_visible (priv->secondary_label, priv->has_secondary_text);
      }
      break;
    case PROP_SECONDARY_USE_MARKUP:
      if (bobgui_label_get_use_markup (BOBGUI_LABEL (priv->secondary_label)) != g_value_get_boolean (value))
        {
          bobgui_label_set_use_markup (BOBGUI_LABEL (priv->secondary_label), g_value_get_boolean (value));
          g_object_notify_by_pspec (object, pspec);
        }
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bobgui_message_dialog_get_property (GObject     *object,
                                 guint        prop_id,
                                 GValue      *value,
                                 GParamSpec  *pspec)
{
  BobguiMessageDialog *dialog = BOBGUI_MESSAGE_DIALOG (object);
  BobguiMessageDialogPrivate *priv = bobgui_message_dialog_get_instance_private (dialog);

  switch (prop_id)
    {
    case PROP_MESSAGE_TYPE:
      g_value_set_enum (value, (BobguiMessageType) priv->message_type);
      break;
    case PROP_TEXT:
      g_value_set_string (value, bobgui_label_get_label (BOBGUI_LABEL (priv->label)));
      break;
    case PROP_USE_MARKUP:
      g_value_set_boolean (value, priv->has_primary_markup);
      break;
    case PROP_SECONDARY_TEXT:
      if (priv->has_secondary_text)
      g_value_set_string (value,
                          bobgui_label_get_label (BOBGUI_LABEL (priv->secondary_label)));
      else
        g_value_set_string (value, NULL);
      break;
    case PROP_SECONDARY_USE_MARKUP:
      if (priv->has_secondary_text)
        g_value_set_boolean (value,
                             bobgui_label_get_use_markup (BOBGUI_LABEL (priv->secondary_label)));
      else
        g_value_set_boolean (value, FALSE);
      break;
    case PROP_MESSAGE_AREA:
      g_value_set_object (value, priv->message_area);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
update_title (GObject    *dialog,
              GParamSpec *pspec,
              BobguiWidget  *label)
{
  const char *title;

  title = bobgui_window_get_title (BOBGUI_WINDOW (dialog));
  bobgui_label_set_label (BOBGUI_LABEL (label), title);
  bobgui_widget_set_visible (label, title && title[0]);
}

static void
bobgui_message_dialog_constructed (GObject *object)
{
  BobguiMessageDialog *dialog = BOBGUI_MESSAGE_DIALOG (object);
  gboolean use_header;

  G_OBJECT_CLASS (bobgui_message_dialog_parent_class)->constructed (object);

  g_object_get (bobgui_widget_get_settings (BOBGUI_WIDGET (dialog)),
                "bobgui-dialogs-use-header", &use_header,
                NULL);

  if (use_header)
    {
      BobguiWidget *box;
      BobguiWidget *label;

      box = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 0);
      bobgui_widget_set_size_request (box, -1, 16);
      label = bobgui_label_new ("");
      bobgui_widget_set_visible (label, FALSE);
      bobgui_widget_set_margin_top (label, 6);
      bobgui_widget_set_margin_bottom (label, 6);
      bobgui_widget_set_halign (label, BOBGUI_ALIGN_CENTER);
      bobgui_widget_set_hexpand (label, TRUE);
      bobgui_widget_add_css_class (label, "title");
      bobgui_box_append (BOBGUI_BOX (box), label);
      g_signal_connect_object (dialog, "notify::title", G_CALLBACK (update_title), label, 0);

      bobgui_window_set_titlebar (BOBGUI_WINDOW (dialog), box);
    }
}

static void
bobgui_message_dialog_class_init (BobguiMessageDialogClass *class)
{
  BobguiWidgetClass *widget_class;
  GObjectClass *gobject_class;

  widget_class = BOBGUI_WIDGET_CLASS (class);
  gobject_class = G_OBJECT_CLASS (class);

  gobject_class->constructed = bobgui_message_dialog_constructed;
  gobject_class->set_property = bobgui_message_dialog_set_property;
  gobject_class->get_property = bobgui_message_dialog_get_property;

  /**
   * BobguiMessageDialog:message-type:
   *
   * The type of the message.
   */
  g_object_class_install_property (gobject_class,
                                   PROP_MESSAGE_TYPE,
                                   g_param_spec_enum ("message-type", NULL, NULL,
                                                      BOBGUI_TYPE_MESSAGE_TYPE,
                                                      BOBGUI_MESSAGE_INFO,
                                                      BOBGUI_PARAM_READWRITE|G_PARAM_CONSTRUCT|G_PARAM_EXPLICIT_NOTIFY));
  /**
   * BobguiMessageDialog:buttons:
   *
   * Set of buttons to display on the dialog.
   */
  g_object_class_install_property (gobject_class,
                                   PROP_BUTTONS,
                                   g_param_spec_enum ("buttons", NULL, NULL,
                                                      BOBGUI_TYPE_BUTTONS_TYPE,
                                                      BOBGUI_BUTTONS_NONE,
                                                      BOBGUI_PARAM_WRITABLE|G_PARAM_CONSTRUCT_ONLY));
  /**
   * BobguiMessageDialog:text:
   *
   * The primary text of the message dialog.
   *
   * If the dialog has a secondary text, this will appear as the title.
   */
  g_object_class_install_property (gobject_class,
                                   PROP_TEXT,
                                   g_param_spec_string ("text", NULL, NULL,
                                                        "",
                                                        BOBGUI_PARAM_READWRITE));
  /**
   * BobguiMessageDialog:use-markup:
   *
   * %TRUE if the primary text of the dialog includes Pango markup.
   *
   * See [func@Pango.parse_markup].
   */
  g_object_class_install_property (gobject_class,
                                   PROP_USE_MARKUP,
                                   g_param_spec_boolean ("use-markup", NULL, NULL,
                                                         FALSE,
                                                         BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));
  /**
   * BobguiMessageDialog:secondary-text:
   *
   * The secondary text of the message dialog.
   */
  g_object_class_install_property (gobject_class,
                                   PROP_SECONDARY_TEXT,
                                   g_param_spec_string ("secondary-text", NULL, NULL,
                                                        NULL,
                                                        BOBGUI_PARAM_READWRITE));
  /**
   * BobguiMessageDialog:secondary-use-markup:
   *
   * %TRUE if the secondary text of the dialog includes Pango markup.
   *
   * See [func@Pango.parse_markup].
   */
  g_object_class_install_property (gobject_class,
                                   PROP_SECONDARY_USE_MARKUP,
                                   g_param_spec_boolean ("secondary-use-markup", NULL, NULL,
                                                         FALSE,
                                                         BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));
  /**
   * BobguiMessageDialog:message-area:
   *
   * The `BobguiBox` that corresponds to the message area of this dialog.
   *
   * See [method@Bobgui.MessageDialog.get_message_area] for a detailed
   * description of this area.
   */
  g_object_class_install_property (gobject_class,
                                   PROP_MESSAGE_AREA,
                                   g_param_spec_object ("message-area", NULL, NULL,
                                                        BOBGUI_TYPE_WIDGET,
                                                        BOBGUI_PARAM_READABLE));

  bobgui_widget_class_set_template_from_resource (widget_class, "/org/bobgui/libbobgui/ui/bobguimessagedialog.ui");
  bobgui_widget_class_bind_template_child_private (widget_class, BobguiMessageDialog, label);
  bobgui_widget_class_bind_template_child_private (widget_class, BobguiMessageDialog, secondary_label);
  bobgui_widget_class_bind_template_child_internal_private (widget_class, BobguiMessageDialog, message_area);
}

static void
bobgui_message_dialog_init (BobguiMessageDialog *dialog)
{
  BobguiMessageDialogPrivate *priv = bobgui_message_dialog_get_instance_private (dialog);
  BobguiWidget *action_area;
  BobguiSettings *settings;
  gboolean use_caret;

  priv->has_primary_markup = FALSE;
  priv->has_secondary_text = FALSE;
  priv->message_type = BOBGUI_MESSAGE_OTHER;

  bobgui_widget_add_css_class (BOBGUI_WIDGET (dialog), "message");

  bobgui_widget_init_template (BOBGUI_WIDGET (dialog));
  action_area = bobgui_dialog_get_action_area (BOBGUI_DIALOG (dialog));
  bobgui_widget_set_halign (action_area, BOBGUI_ALIGN_FILL);
  bobgui_box_set_homogeneous (BOBGUI_BOX (action_area), TRUE);

  settings = bobgui_widget_get_settings (BOBGUI_WIDGET (dialog));
  g_object_get (settings, "bobgui-keynav-use-caret", &use_caret, NULL);
  bobgui_label_set_selectable (BOBGUI_LABEL (priv->label), use_caret);
  bobgui_label_set_selectable (BOBGUI_LABEL (priv->secondary_label), use_caret);
}

/**
 * bobgui_message_dialog_new:
 * @parent: (nullable): transient parent
 * @flags: flags
 * @type: type of message
 * @buttons: set of buttons to use
 * @message_format: (nullable): printf()-style format string
 * @...: arguments for @message_format
 *
 * Creates a new message dialog.
 *
 * This is a simple dialog with some text the user may want to see.
 * When the user clicks a button a “response” signal is emitted with
 * response IDs from [enum@Bobgui.ResponseType]. See [class@Bobgui.Dialog]
 * for more details.
 *
 * Returns: (transfer none): a new `BobguiMessageDialog`
 *
 * Deprecated: 4.10: Use [class@Bobgui.AlertDialog] instead
 */
BobguiWidget*
bobgui_message_dialog_new (BobguiWindow     *parent,
                        BobguiDialogFlags flags,
                        BobguiMessageType type,
                        BobguiButtonsType buttons,
                        const char    *message_format,
                        ...)
{
  BobguiWidget *widget;
  BobguiDialog *dialog;
  char * msg = NULL;
  va_list args;

  g_return_val_if_fail (parent == NULL || BOBGUI_IS_WINDOW (parent), NULL);

  widget = g_object_new (BOBGUI_TYPE_MESSAGE_DIALOG,
                         "use-header-bar", FALSE,
                         "message-type", type,
                         "buttons", buttons,
                         NULL);
  dialog = BOBGUI_DIALOG (widget);

  if (message_format)
    {
      BobguiMessageDialogPrivate *priv = bobgui_message_dialog_get_instance_private ((BobguiMessageDialog*)dialog);
      va_start (args, message_format);
      msg = g_strdup_vprintf (message_format, args);
      va_end (args);

      bobgui_label_set_text (BOBGUI_LABEL (priv->label), msg);

      g_free (msg);
    }

  if (parent != NULL)
    bobgui_window_set_transient_for (BOBGUI_WINDOW (widget), BOBGUI_WINDOW (parent));

  if (flags & BOBGUI_DIALOG_MODAL)
    bobgui_window_set_modal (BOBGUI_WINDOW (dialog), TRUE);

  if (flags & BOBGUI_DIALOG_DESTROY_WITH_PARENT)
    bobgui_window_set_destroy_with_parent (BOBGUI_WINDOW (dialog), TRUE);

  return widget;
}

/**
 * bobgui_message_dialog_new_with_markup:
 * @parent: (nullable): transient parent
 * @flags: flags
 * @type: type of message
 * @buttons: set of buttons to use
 * @message_format: (nullable): printf()-style format string
 * @...: arguments for @message_format
 *
 * Creates a new message dialog.
 *
 * This is a simple dialog with some text that is marked up with
 * Pango markup. When the user clicks a button a “response” signal
 * is emitted with response IDs from [enum@Bobgui.ResponseType]. See
 * [class@Bobgui.Dialog] for more details.
 *
 * Special XML characters in the printf() arguments passed to this
 * function will automatically be escaped as necessary.
 * (See g_markup_printf_escaped() for how this is implemented.)
 * Usually this is what you want, but if you have an existing
 * Pango markup string that you want to use literally as the
 * label, then you need to use [method@Bobgui.MessageDialog.set_markup]
 * instead, since you can’t pass the markup string either
 * as the format (it might contain “%” characters) or as a string
 * argument.
 *
 * ```c
 * BobguiWidget *dialog;
 * BobguiDialogFlags flags = BOBGUI_DIALOG_DESTROY_WITH_PARENT;
 * dialog = bobgui_message_dialog_new (parent_window,
 *                                  flags,
 *                                  BOBGUI_MESSAGE_ERROR,
 *                                  BOBGUI_BUTTONS_CLOSE,
 *                                  NULL);
 * bobgui_message_dialog_set_markup (BOBGUI_MESSAGE_DIALOG (dialog),
 *                                markup);
 * ```
 *
 * Returns: a new `BobguiMessageDialog`
 *
 * Deprecated: 4.10: Use [class@Bobgui.AlertDialog] instead
 **/
BobguiWidget*
bobgui_message_dialog_new_with_markup (BobguiWindow     *parent,
                                    BobguiDialogFlags flags,
                                    BobguiMessageType type,
                                    BobguiButtonsType buttons,
                                    const char    *message_format,
                                    ...)
{
  BobguiWidget *widget;
  va_list args;
  char *msg = NULL;

  g_return_val_if_fail (parent == NULL || BOBGUI_IS_WINDOW (parent), NULL);

  widget = bobgui_message_dialog_new (parent, flags, type, buttons, NULL);

  if (message_format)
    {
      va_start (args, message_format);
      msg = g_markup_vprintf_escaped (message_format, args);
      va_end (args);

      bobgui_message_dialog_set_markup (BOBGUI_MESSAGE_DIALOG (widget), msg);

      g_free (msg);
    }

  return widget;
}

/**
 * bobgui_message_dialog_set_markup:
 * @message_dialog: a `BobguiMessageDialog`
 * @str: string with Pango markup
 *
 * Sets the text of the message dialog.
 *
 * Deprecated: 4.10: Use [class@Bobgui.AlertDialog] instead
 */
void
bobgui_message_dialog_set_markup (BobguiMessageDialog *message_dialog,
                               const char       *str)
{
  BobguiMessageDialogPrivate *priv = bobgui_message_dialog_get_instance_private (message_dialog);

  g_return_if_fail (BOBGUI_IS_MESSAGE_DIALOG (message_dialog));

  priv->has_primary_markup = TRUE;
  bobgui_label_set_markup (BOBGUI_LABEL (priv->label), str);
}

/**
 * bobgui_message_dialog_format_secondary_text:
 * @message_dialog: a `BobguiMessageDialog`
 * @message_format: (nullable): printf()-style format string
 * @...: arguments for @message_format
 *
 * Sets the secondary text of the message dialog.
 *
 * Deprecated: 4.10: Use [class@Bobgui.AlertDialog] instead
 */
void
bobgui_message_dialog_format_secondary_text (BobguiMessageDialog *message_dialog,
                                          const char       *message_format,
                                          ...)
{
  BobguiMessageDialogPrivate *priv = bobgui_message_dialog_get_instance_private (message_dialog);
  va_list args;
  char *msg = NULL;

  g_return_if_fail (BOBGUI_IS_MESSAGE_DIALOG (message_dialog));

  if (message_format)
    {
      priv->has_secondary_text = TRUE;
      bobgui_widget_add_css_class (priv->label, "title");

      va_start (args, message_format);
      msg = g_strdup_vprintf (message_format, args);
      va_end (args);

      bobgui_label_set_text (BOBGUI_LABEL (priv->secondary_label), msg);

      g_free (msg);
    }
  else
    {
      priv->has_secondary_text = FALSE;
      bobgui_widget_remove_css_class (priv->label, "title");
    }

  bobgui_widget_set_visible (priv->secondary_label, priv->has_secondary_text);
}

/**
 * bobgui_message_dialog_format_secondary_markup:
 * @message_dialog: a `BobguiMessageDialog`
 * @message_format: printf()-style string with Pango markup
 * @...: arguments for @message_format
 *
 * Sets the secondary text of the message dialog.
 *
 * The @message_format is assumed to contain Pango markup.
 *
 * Due to an oversight, this function does not escape special
 * XML characters like [ctor@Bobgui.MessageDialog.new_with_markup]
 * does. Thus, if the arguments may contain special XML characters,
 * you should use g_markup_printf_escaped() to escape it.
 *
 * ```c
 * char *msg;
 *
 * msg = g_markup_printf_escaped (message_format, ...);
 * bobgui_message_dialog_format_secondary_markup (message_dialog,
 *                                             "%s", msg);
 * g_free (msg);
 * ```
 *
 * Deprecated: 4.10: Use [class@Bobgui.AlertDialog] instead
 */
void
bobgui_message_dialog_format_secondary_markup (BobguiMessageDialog *message_dialog,
                                            const char       *message_format,
                                            ...)
{
  BobguiMessageDialogPrivate *priv = bobgui_message_dialog_get_instance_private (message_dialog);
  va_list args;
  char *msg = NULL;

  g_return_if_fail (BOBGUI_IS_MESSAGE_DIALOG (message_dialog));

  if (message_format)
    {
      priv->has_secondary_text = TRUE;
      bobgui_widget_add_css_class (priv->label, "title");

      va_start (args, message_format);
      msg = g_strdup_vprintf (message_format, args);
      va_end (args);

      bobgui_label_set_markup (BOBGUI_LABEL (priv->secondary_label), msg);

      g_free (msg);
    }
  else
    {
      priv->has_secondary_text = FALSE;
      bobgui_widget_remove_css_class (priv->label, "title");
    }

  bobgui_widget_set_visible (priv->secondary_label, priv->has_secondary_text);
}

/**
 * bobgui_message_dialog_get_message_area:
 * @message_dialog: a `BobguiMessageDialog`
 *
 * Returns the message area of the dialog.
 *
 * This is the box where the dialog’s primary and secondary labels
 * are packed. You can add your own extra content to that box and it
 * will appear below those labels. See [method@Bobgui.Dialog.get_content_area]
 * for the corresponding function in the parent [class@Bobgui.Dialog].
 *
 * Returns: (transfer none): A `BobguiBox` corresponding to the
 *   “message area” in the @message_dialog
 *
 * Deprecated: 4.10: Use [class@Bobgui.AlertDialog] instead
 */
BobguiWidget *
bobgui_message_dialog_get_message_area (BobguiMessageDialog *message_dialog)
{
  BobguiMessageDialogPrivate *priv = bobgui_message_dialog_get_instance_private (message_dialog);

  g_return_val_if_fail (BOBGUI_IS_MESSAGE_DIALOG (message_dialog), NULL);

  return priv->message_area;
}
