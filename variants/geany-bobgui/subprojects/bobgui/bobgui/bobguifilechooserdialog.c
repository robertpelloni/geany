/* -*- Mode: C; c-file-style: "gnu"; tab-width: 8 -*- */
/* BOBGUI - The Bobgui Framework
 * bobguifilechooserdialog.c: File selector dialog
 * Copyright (C) 2003, Red Hat, Inc.
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

#include "deprecated/bobguifilechooserdialog.h"

#include "bobguifilechooserprivate.h"
#include "deprecated/bobguifilechooserwidget.h"
#include "bobguifilechooserwidgetprivate.h"
#include "bobguifilechooserutils.h"
#include "bobguisizerequest.h"
#include "bobguitypebuiltins.h"
#include <glib/gi18n-lib.h>
#include "bobguisettings.h"
#include "bobguitogglebutton.h"
#include "bobguiheaderbar.h"
#include "deprecated/bobguidialogprivate.h"
#include "bobguilabel.h"
#include "bobguifilechooserentry.h"
#include "bobguibox.h"

#include <stdarg.h>


G_GNUC_BEGIN_IGNORE_DEPRECATIONS

/**
 * BobguiFileChooserDialog:
 *
 * `BobguiFileChooserDialog` is a dialog suitable for use with
 * “File Open” or “File Save” commands.
 *
 * ![An example BobguiFileChooserDialog](filechooser.png)
 *
 * This widget works by putting a [class@Bobgui.FileChooserWidget]
 * inside a [class@Bobgui.Dialog]. It exposes the [iface@Bobgui.FileChooser]
 * interface, so you can use all of the [iface@Bobgui.FileChooser] functions
 * on the file chooser dialog as well as those for [class@Bobgui.Dialog].
 *
 * Note that `BobguiFileChooserDialog` does not have any methods of its
 * own. Instead, you should use the functions that work on a
 * [iface@Bobgui.FileChooser].
 *
 * If you want to integrate well with the platform you should use the
 * [class@Bobgui.FileChooserNative] API, which will use a platform-specific
 * dialog if available and fall back to `BobguiFileChooserDialog`
 * otherwise.
 *
 * ## Typical usage
 *
 * In the simplest of cases, you can the following code to use
 * `BobguiFileChooserDialog` to select a file for opening:
 *
 * ```c
 * static void
 * on_open_response (BobguiDialog *dialog,
 *                   int        response)
 * {
 *   if (response == BOBGUI_RESPONSE_ACCEPT)
 *     {
 *       BobguiFileChooser *chooser = BOBGUI_FILE_CHOOSER (dialog);
 *
 *       g_autoptr(GFile) file = bobgui_file_chooser_get_file (chooser);
 *
 *       open_file (file);
 *     }
 *
 *   bobgui_window_destroy (BOBGUI_WINDOW (dialog));
 * }
 *
 *   // ...
 *   BobguiWidget *dialog;
 *   BobguiFileChooserAction action = BOBGUI_FILE_CHOOSER_ACTION_OPEN;
 *
 *   dialog = bobgui_file_chooser_dialog_new ("Open File",
 *                                         parent_window,
 *                                         action,
 *                                         _("_Cancel"),
 *                                         BOBGUI_RESPONSE_CANCEL,
 *                                         _("_Open"),
 *                                         BOBGUI_RESPONSE_ACCEPT,
 *                                         NULL);
 *
 *   bobgui_window_present (BOBGUI_WINDOW (dialog));
 *
 *   g_signal_connect (dialog, "response",
 *                     G_CALLBACK (on_open_response),
 *                     NULL);
 * ```
 *
 * To use a dialog for saving, you can use this:
 *
 * ```c
 * static void
 * on_save_response (BobguiDialog *dialog,
 *                   int        response)
 * {
 *   if (response == BOBGUI_RESPONSE_ACCEPT)
 *     {
 *       BobguiFileChooser *chooser = BOBGUI_FILE_CHOOSER (dialog);
 *
 *       g_autoptr(GFile) file = bobgui_file_chooser_get_file (chooser);
 *
 *       save_to_file (file);
 *     }
 *
 *   bobgui_window_destroy (BOBGUI_WINDOW (dialog));
 * }
 *
 *   // ...
 *   BobguiWidget *dialog;
 *   BobguiFileChooser *chooser;
 *   BobguiFileChooserAction action = BOBGUI_FILE_CHOOSER_ACTION_SAVE;
 *
 *   dialog = bobgui_file_chooser_dialog_new ("Save File",
 *                                         parent_window,
 *                                         action,
 *                                         _("_Cancel"),
 *                                         BOBGUI_RESPONSE_CANCEL,
 *                                         _("_Save"),
 *                                         BOBGUI_RESPONSE_ACCEPT,
 *                                         NULL);
 *   chooser = BOBGUI_FILE_CHOOSER (dialog);
 *
 *   if (user_edited_a_new_document)
 *     bobgui_file_chooser_set_current_name (chooser, _("Untitled document"));
 *   else
 *     bobgui_file_chooser_set_file (chooser, existing_filename);
 *
 *   bobgui_window_present (BOBGUI_WINDOW (dialog));
 *
 *   g_signal_connect (dialog, "response",
 *                     G_CALLBACK (on_save_response),
 *                     NULL);
 * ```
 *
 * ## Setting up a file chooser dialog
 *
 * There are various cases in which you may need to use a `BobguiFileChooserDialog`:
 *
 * - To select a file for opening, use %BOBGUI_FILE_CHOOSER_ACTION_OPEN.
 *
 * - To save a file for the first time, use %BOBGUI_FILE_CHOOSER_ACTION_SAVE,
 *   and suggest a name such as “Untitled” with
 *   [method@Bobgui.FileChooser.set_current_name].
 *
 * - To save a file under a different name, use %BOBGUI_FILE_CHOOSER_ACTION_SAVE,
 *   and set the existing file with [method@Bobgui.FileChooser.set_file].
 *
 * - To choose a folder instead of a file, use %BOBGUI_FILE_CHOOSER_ACTION_SELECT_FOLDER.
 *
 * In general, you should only cause the file chooser to show a specific
 * folder when it is appropriate to use [method@Bobgui.FileChooser.set_file],
 * i.e. when you are doing a “Save As” command and you already have a file
 * saved somewhere.

 * ## Response Codes
 *
 * `BobguiFileChooserDialog` inherits from [class@Bobgui.Dialog], so buttons that
 * go in its action area have response codes such as %BOBGUI_RESPONSE_ACCEPT and
 * %BOBGUI_RESPONSE_CANCEL. For example, you could call
 * [ctor@Bobgui.FileChooserDialog.new] as follows:
 *
 * ```c
 * BobguiWidget *dialog;
 * BobguiFileChooserAction action = BOBGUI_FILE_CHOOSER_ACTION_OPEN;
 *
 * dialog = bobgui_file_chooser_dialog_new ("Open File",
 *                                       parent_window,
 *                                       action,
 *                                       _("_Cancel"),
 *                                       BOBGUI_RESPONSE_CANCEL,
 *                                       _("_Open"),
 *                                       BOBGUI_RESPONSE_ACCEPT,
 *                                       NULL);
 * ```
 *
 * This will create buttons for “Cancel” and “Open” that use predefined
 * response identifiers from [enum@Bobgui.ResponseType].  For most dialog
 * boxes you can use your own custom response codes rather than the
 * ones in [enum@Bobgui.ResponseType], but `BobguiFileChooserDialog` assumes that
 * its “accept”-type action, e.g. an “Open” or “Save” button,
 * will have one of the following response codes:
 *
 * - %BOBGUI_RESPONSE_ACCEPT
 * - %BOBGUI_RESPONSE_OK
 * - %BOBGUI_RESPONSE_YES
 * - %BOBGUI_RESPONSE_APPLY
 *
 * This is because `BobguiFileChooserDialog` must intercept responses and switch
 * to folders if appropriate, rather than letting the dialog terminate — the
 * implementation uses these known response codes to know which responses can
 * be blocked if appropriate.
 *
 * To summarize, make sure you use a predefined response code
 * when you use `BobguiFileChooserDialog` to ensure proper operation.
 *
 * ## CSS nodes
 *
 * `BobguiFileChooserDialog` has a single CSS node with the name `window` and style
 * class `.filechooser`.
 *
 * Deprecated: 4.10: Use [class@Bobgui.FileDialog] instead
 */

typedef struct _BobguiFileChooserDialogPrivate BobguiFileChooserDialogPrivate;
typedef struct _BobguiFileChooserDialogClass   BobguiFileChooserDialogClass;

struct _BobguiFileChooserDialog
{
  BobguiDialog parent_instance;
};

struct _BobguiFileChooserDialogClass
{
  BobguiDialogClass parent_class;
};

struct _BobguiFileChooserDialogPrivate
{
  BobguiWidget *widget;

  BobguiSizeGroup *buttons;

  /* for use with BobguiFileChooserEmbed */
  gboolean response_requested;
  gboolean search_setup;
  gboolean has_entry;
};

static void     bobgui_file_chooser_dialog_dispose      (GObject               *object);
static void     bobgui_file_chooser_dialog_set_property (GObject               *object,
                                                      guint                  prop_id,
                                                      const GValue          *value,
                                                      GParamSpec            *pspec);
static void     bobgui_file_chooser_dialog_get_property (GObject               *object,
                                                      guint                  prop_id,
                                                      GValue                *value,
                                                      GParamSpec            *pspec);
static void     bobgui_file_chooser_dialog_notify       (GObject               *object,
                                                      GParamSpec            *pspec);

static void     bobgui_file_chooser_dialog_realize      (BobguiWidget             *widget);
static void     bobgui_file_chooser_dialog_map          (BobguiWidget             *widget);
static void     bobgui_file_chooser_dialog_unmap        (BobguiWidget             *widget);
static void     bobgui_file_chooser_dialog_size_allocate (BobguiWidget            *widget,
                                                       int                   width,
                                                       int                   height,
                                                       int                    baseline);
static void     bobgui_file_chooser_dialog_activate_response (BobguiWidget        *widget,
                                                           const char       *action_name,
                                                           GVariant         *parameters);

static void response_cb (BobguiDialog *dialog,
                         int        response_id);

static void setup_save_entry (BobguiFileChooserDialog *dialog);

G_DEFINE_TYPE_WITH_CODE (BobguiFileChooserDialog, bobgui_file_chooser_dialog, BOBGUI_TYPE_DIALOG,
                         G_ADD_PRIVATE (BobguiFileChooserDialog)
                         G_IMPLEMENT_INTERFACE (BOBGUI_TYPE_FILE_CHOOSER,
                                                _bobgui_file_chooser_delegate_iface_init))

static void
bobgui_file_chooser_dialog_class_init (BobguiFileChooserDialogClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (class);

  gobject_class->dispose = bobgui_file_chooser_dialog_dispose;
  gobject_class->set_property = bobgui_file_chooser_dialog_set_property;
  gobject_class->get_property = bobgui_file_chooser_dialog_get_property;
  gobject_class->notify = bobgui_file_chooser_dialog_notify;

  widget_class->realize = bobgui_file_chooser_dialog_realize;
  widget_class->map = bobgui_file_chooser_dialog_map;
  widget_class->unmap = bobgui_file_chooser_dialog_unmap;
  widget_class->size_allocate = bobgui_file_chooser_dialog_size_allocate;

  _bobgui_file_chooser_install_properties (gobject_class);

  /* Bind class to template
   */
  bobgui_widget_class_set_template_from_resource (widget_class,
                                               "/org/bobgui/libbobgui/ui/bobguifilechooserdialog.ui");

  bobgui_widget_class_bind_template_child_private (widget_class, BobguiFileChooserDialog, widget);
  bobgui_widget_class_bind_template_child_private (widget_class, BobguiFileChooserDialog, buttons);
  bobgui_widget_class_bind_template_callback (widget_class, response_cb);

  /**
   * BobguiFileChooserDialog|response.activate:
   *
   * Activate the default response of the dialog.
   */
  bobgui_widget_class_install_action (widget_class, "response.activate", NULL, bobgui_file_chooser_dialog_activate_response);
}

static void
bobgui_file_chooser_dialog_init (BobguiFileChooserDialog *dialog)
{
  BobguiFileChooserDialogPrivate *priv = bobgui_file_chooser_dialog_get_instance_private (dialog);

  priv->response_requested = FALSE;

  bobgui_widget_init_template (BOBGUI_WIDGET (dialog));
  bobgui_dialog_set_use_header_bar_from_setting (BOBGUI_DIALOG (dialog));

  _bobgui_file_chooser_set_delegate (BOBGUI_FILE_CHOOSER (dialog),
                                  BOBGUI_FILE_CHOOSER (priv->widget));
}

static BobguiWidget *
get_accept_action_widget (BobguiDialog *dialog,
                          gboolean   sensitive_only)
{
  int response[] = {
    BOBGUI_RESPONSE_ACCEPT,
    BOBGUI_RESPONSE_OK,
    BOBGUI_RESPONSE_YES,
    BOBGUI_RESPONSE_APPLY
  };
  int i;
  BobguiWidget *widget;

  for (i = 0; i < G_N_ELEMENTS (response); i++)
    {
      widget = bobgui_dialog_get_widget_for_response (dialog, response[i]);
      if (widget)
        {
          if (!sensitive_only)
            return widget;

          if (bobgui_widget_is_sensitive (widget))
            return widget;
        }
    }

  return NULL;
}

static gboolean
is_accept_response_id (int response_id)
{
  return (response_id == BOBGUI_RESPONSE_ACCEPT ||
          response_id == BOBGUI_RESPONSE_OK ||
          response_id == BOBGUI_RESPONSE_YES ||
          response_id == BOBGUI_RESPONSE_APPLY);
}

static void
bobgui_file_chooser_dialog_activate_response (BobguiWidget  *widget,
                                           const char *action_name,
                                           GVariant   *parameters)
{
  BobguiFileChooserDialog *dialog = BOBGUI_FILE_CHOOSER_DIALOG (widget);
  BobguiFileChooserDialogPrivate *priv = bobgui_file_chooser_dialog_get_instance_private (dialog);
  BobguiWidget *button;

  priv->response_requested = TRUE;

  button = get_accept_action_widget (BOBGUI_DIALOG (dialog), TRUE);
  if (button)
    {
      bobgui_widget_activate (button);
      return;
    }

  priv->response_requested = FALSE;
}

static void
bobgui_file_chooser_dialog_dispose (GObject *object)
{
  bobgui_widget_dispose_template (BOBGUI_WIDGET (object), BOBGUI_TYPE_FILE_CHOOSER_DIALOG);

  G_OBJECT_CLASS (bobgui_file_chooser_dialog_parent_class)->dispose (object);
}

static void
bobgui_file_chooser_dialog_set_property (GObject      *object,
                                      guint         prop_id,
                                      const GValue *value,
                                      GParamSpec   *pspec)

{
  BobguiFileChooserDialogPrivate *priv = bobgui_file_chooser_dialog_get_instance_private (BOBGUI_FILE_CHOOSER_DIALOG (object));

  g_object_set_property (G_OBJECT (priv->widget), pspec->name, value);
}

static void
bobgui_file_chooser_dialog_get_property (GObject    *object,
                                      guint       prop_id,
                                      GValue     *value,
                                      GParamSpec *pspec)
{
  BobguiFileChooserDialogPrivate *priv = bobgui_file_chooser_dialog_get_instance_private (BOBGUI_FILE_CHOOSER_DIALOG (object));

  g_object_get_property (G_OBJECT (priv->widget), pspec->name, value);
}

static void
bobgui_file_chooser_dialog_notify (GObject    *object,
                                GParamSpec *pspec)
{
  if (strcmp (pspec->name, "action") == 0)
    setup_save_entry (BOBGUI_FILE_CHOOSER_DIALOG (object));

  if (G_OBJECT_CLASS (bobgui_file_chooser_dialog_parent_class)->notify)
    G_OBJECT_CLASS (bobgui_file_chooser_dialog_parent_class)->notify (object, pspec);
}

static void
add_button (BobguiWidget *button, gpointer data)
{
  BobguiFileChooserDialog *dialog = data;
  BobguiFileChooserDialogPrivate *priv = bobgui_file_chooser_dialog_get_instance_private (dialog);

  if (BOBGUI_IS_BUTTON (button))
    bobgui_size_group_add_widget (priv->buttons, button);
}

static gboolean
translate_subtitle_to_visible (GBinding     *binding,
                               const GValue *from_value,
                               GValue       *to_value,
                               gpointer      user_data)
{
  const char *subtitle = g_value_get_string (from_value);

  g_value_set_boolean (to_value, subtitle != NULL);

  return TRUE;
}

static void
setup_search (BobguiFileChooserDialog *dialog)
{
  BobguiFileChooserDialogPrivate *priv = bobgui_file_chooser_dialog_get_instance_private (dialog);
  gboolean use_header;
  BobguiWidget *child;

  if (priv->search_setup)
    return;

  priv->search_setup = TRUE;

  g_object_get (dialog, "use-header-bar", &use_header, NULL);
  if (use_header)
    {
      BobguiWidget *button;
      BobguiWidget *header;
      BobguiWidget *box;
      BobguiWidget *label;

      button = bobgui_toggle_button_new ();
      bobgui_widget_set_focus_on_click (button, FALSE);
      bobgui_widget_set_valign (button, BOBGUI_ALIGN_CENTER);
      bobgui_widget_set_tooltip_text (button, _("Search"));
      bobgui_button_set_icon_name (BOBGUI_BUTTON (button), "edit-find-symbolic");

      bobgui_accessible_update_property (BOBGUI_ACCESSIBLE (button),
                                      BOBGUI_ACCESSIBLE_PROPERTY_KEY_SHORTCUTS, "Alt+S Control+F Find",
                                      -1);

      header = bobgui_dialog_get_header_bar (BOBGUI_DIALOG (dialog));
      bobgui_header_bar_pack_end (BOBGUI_HEADER_BAR (header), button);

      g_object_bind_property (button, "active",
                              priv->widget, "search-mode",
                              G_BINDING_BIDIRECTIONAL);

      if (!priv->has_entry)
        {
          box = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 0);
          bobgui_widget_set_valign (box, BOBGUI_ALIGN_CENTER);

          label = bobgui_label_new (NULL);
          bobgui_widget_set_halign (label, BOBGUI_ALIGN_CENTER);
          bobgui_label_set_single_line_mode (BOBGUI_LABEL (label), TRUE);
          bobgui_label_set_ellipsize (BOBGUI_LABEL (label), PANGO_ELLIPSIZE_END);
          bobgui_label_set_width_chars (BOBGUI_LABEL (label), 5);
          bobgui_widget_add_css_class (label, "title");
          bobgui_widget_set_parent (label, box);

          g_object_bind_property (dialog, "title",
                                  label, "label",
                                  G_BINDING_SYNC_CREATE);

          label = bobgui_label_new (NULL);
          bobgui_widget_set_halign (label, BOBGUI_ALIGN_CENTER);
          bobgui_label_set_single_line_mode (BOBGUI_LABEL (label), TRUE);
          bobgui_label_set_ellipsize (BOBGUI_LABEL (label), PANGO_ELLIPSIZE_END);
          bobgui_widget_add_css_class (label, "subtitle");
          bobgui_widget_set_parent (label, box);

          g_object_bind_property (priv->widget, "subtitle",
                                  label, "label",
                                  G_BINDING_SYNC_CREATE);
          g_object_bind_property_full (priv->widget, "subtitle",
                                       label, "visible",
                                       G_BINDING_SYNC_CREATE,
                                       translate_subtitle_to_visible,
                                       NULL, NULL, NULL);

          bobgui_header_bar_set_title_widget (BOBGUI_HEADER_BAR (header), box);
        }

      for (child = bobgui_widget_get_first_child (header);
           child != NULL;
           child = bobgui_widget_get_next_sibling (child))
        add_button (child, dialog);
    }
}

static void
setup_save_entry (BobguiFileChooserDialog *dialog)
{
  BobguiFileChooserDialogPrivate *priv = bobgui_file_chooser_dialog_get_instance_private (dialog);
  gboolean use_header;
  BobguiFileChooserAction action;
  gboolean need_entry;
  BobguiWidget *header;

  g_object_get (dialog,
                "use-header-bar", &use_header,
                "action", &action,
                NULL);

  if (!use_header)
    return;

  header = bobgui_dialog_get_header_bar (BOBGUI_DIALOG (dialog));

  need_entry = action == BOBGUI_FILE_CHOOSER_ACTION_SAVE;

  if (need_entry && !priv->has_entry)
    {
      BobguiWidget *box;
      BobguiWidget *label;
      BobguiWidget *entry;

      box = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 0);
      label = bobgui_label_new_with_mnemonic (_("_Name"));
      entry = _bobgui_file_chooser_entry_new (FALSE, FALSE);
      g_object_set (label, "margin-start", 6, "margin-end", 6, NULL);
      g_object_set (entry, "margin-start", 6, "margin-end", 6, NULL);
      bobgui_label_set_mnemonic_widget (BOBGUI_LABEL (label), entry);
      bobgui_box_append (BOBGUI_BOX (box), label);
      bobgui_box_append (BOBGUI_BOX (box), entry);

      bobgui_header_bar_set_title_widget (BOBGUI_HEADER_BAR (header), box);
      bobgui_file_chooser_widget_set_save_entry (BOBGUI_FILE_CHOOSER_WIDGET (priv->widget), entry);
    }
  else if (!need_entry && priv->has_entry)
    {
      bobgui_header_bar_set_title_widget (BOBGUI_HEADER_BAR (header), NULL);
      bobgui_file_chooser_widget_set_save_entry (BOBGUI_FILE_CHOOSER_WIDGET (priv->widget), NULL);
    }

  priv->has_entry = need_entry;
}

static void
ensure_default_response (BobguiFileChooserDialog *dialog)
{
  BobguiWidget *widget;

  widget = get_accept_action_widget (BOBGUI_DIALOG (dialog), TRUE);
  if (widget)
    bobgui_window_set_default_widget (BOBGUI_WINDOW (dialog), widget);
}

static void
bobgui_file_chooser_dialog_realize (BobguiWidget *widget)
{
  BobguiFileChooserDialog *dialog = BOBGUI_FILE_CHOOSER_DIALOG (widget);
  GSettings *settings;
  int width, height;

  settings = _bobgui_file_chooser_get_settings_for_widget (widget);
  g_settings_get (settings, SETTINGS_KEY_WINDOW_SIZE, "(ii)", &width, &height);

  if (width != 0 && height != 0)
    bobgui_window_set_default_size (BOBGUI_WINDOW (dialog), width, height);

  BOBGUI_WIDGET_CLASS (bobgui_file_chooser_dialog_parent_class)->realize (widget);
}

static void
bobgui_file_chooser_dialog_map (BobguiWidget *widget)
{
  BobguiFileChooserDialog *dialog = BOBGUI_FILE_CHOOSER_DIALOG (widget);
  BobguiFileChooserDialogPrivate *priv = bobgui_file_chooser_dialog_get_instance_private (dialog);

  setup_search (dialog);
  setup_save_entry (dialog);
  ensure_default_response (dialog);

  bobgui_file_chooser_widget_initial_focus (BOBGUI_FILE_CHOOSER_WIDGET (priv->widget));

  BOBGUI_WIDGET_CLASS (bobgui_file_chooser_dialog_parent_class)->map (widget);
}

static void
save_dialog_geometry (BobguiFileChooserDialog *dialog)
{
  BobguiWindow *window;
  GSettings *settings;
  int old_width, old_height;
  int width, height;

  settings = _bobgui_file_chooser_get_settings_for_widget (BOBGUI_WIDGET (dialog));

  window = BOBGUI_WINDOW (dialog);

  bobgui_window_get_default_size (window, &width, &height);

  g_settings_get (settings, SETTINGS_KEY_WINDOW_SIZE, "(ii)", &old_width, &old_height);
  if (old_width != width || old_height != height)
    g_settings_set (settings, SETTINGS_KEY_WINDOW_SIZE, "(ii)", width, height);

  g_settings_apply (settings);
}

static void
bobgui_file_chooser_dialog_unmap (BobguiWidget *widget)
{
  BobguiFileChooserDialog *dialog = BOBGUI_FILE_CHOOSER_DIALOG (widget);

  save_dialog_geometry (dialog);

  BOBGUI_WIDGET_CLASS (bobgui_file_chooser_dialog_parent_class)->unmap (widget);
}

static void
bobgui_file_chooser_dialog_size_allocate (BobguiWidget *widget,
                                       int        width,
                                       int        height,
                                       int        baseline)
{
  BOBGUI_WIDGET_CLASS (bobgui_file_chooser_dialog_parent_class)->size_allocate (widget,
                                                                          width,
                                                                          height,
                                                                          baseline);
  if (bobgui_widget_is_drawable (widget))
    save_dialog_geometry (BOBGUI_FILE_CHOOSER_DIALOG (widget));
}

/* We do a signal connection here rather than overriding the method in
 * class_init because BobguiDialog::response is a RUN_LAST signal.  We want *our*
 * handler to be run *first*, regardless of whether the user installs response
 * handlers of his own.
 */
static void
response_cb (BobguiDialog *dialog,
             int        response_id)
{
  BobguiFileChooserDialogPrivate *priv = bobgui_file_chooser_dialog_get_instance_private (BOBGUI_FILE_CHOOSER_DIALOG (dialog));

  /* Act only on response IDs we recognize */
  if (is_accept_response_id (response_id) &&
      !priv->response_requested &&
      !bobgui_file_chooser_widget_should_respond (BOBGUI_FILE_CHOOSER_WIDGET (priv->widget)))
    {
      g_signal_stop_emission_by_name (dialog, "response");
    }

  priv->response_requested = FALSE;
}

static BobguiWidget *
bobgui_file_chooser_dialog_new_valist (const char           *title,
                                    BobguiWindow            *parent,
                                    BobguiFileChooserAction  action,
                                    const char           *first_button_text,
                                    va_list               varargs)
{
  BobguiWidget *result;
  const char *button_text = first_button_text;
  int response_id;

  result = g_object_new (BOBGUI_TYPE_FILE_CHOOSER_DIALOG,
                         "title", title,
                         "action", action,
                         NULL);

  if (parent)
    bobgui_window_set_transient_for (BOBGUI_WINDOW (result), parent);

  while (button_text)
    {
      response_id = va_arg (varargs, int);
      bobgui_dialog_add_button (BOBGUI_DIALOG (result), button_text, response_id);
      button_text = va_arg (varargs, const char *);
    }

  return result;
}

/**
 * bobgui_file_chooser_dialog_new:
 * @title: (nullable): Title of the dialog
 * @parent: (nullable): Transient parent of the dialog
 * @action: Open or save mode for the dialog
 * @first_button_text: (nullable): text to go in the first button
 * @...: response ID for the first button, then additional (button, id) pairs, ending with %NULL
 *
 * Creates a new `BobguiFileChooserDialog`.
 *
 * This function is analogous to [ctor@Bobgui.Dialog.new_with_buttons].
 *
 * Returns: a new `BobguiFileChooserDialog`
 *
 * Deprecated: 4.10: Use [class@Bobgui.FileDialog] instead
 */
BobguiWidget *
bobgui_file_chooser_dialog_new (const char           *title,
                             BobguiWindow            *parent,
                             BobguiFileChooserAction  action,
                             const char           *first_button_text,
                             ...)
{
  BobguiWidget *result;
  va_list varargs;

  va_start (varargs, first_button_text);
  result = bobgui_file_chooser_dialog_new_valist (title, parent, action,
                                               first_button_text,
                                               varargs);
  va_end (varargs);

  return result;
}
