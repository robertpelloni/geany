/* BOBGUI - The Bobgui Framework
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
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

#include <stdlib.h>
#include <string.h>

#include "bobguibutton.h"
#include "bobguidialog.h"
#include "bobguidialogprivate.h"
#include "bobguiheaderbar.h"
#include "bobguiheaderbarprivate.h"
#include "bobguilabel.h"
#include "bobguimarshalers.h"
#include "bobguibox.h"
#include "bobguimain.h"
#include "bobguiprivate.h"
#include "bobguibuildable.h"
#include "bobguibuilderprivate.h"
#include "bobguisettings.h"
#include "bobguitypebuiltins.h"
#include "bobguisizegroup.h"

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

/**
 * BobguiDialog:
 *
 * Dialogs are a convenient way to prompt the user for a small amount
 * of input.
 *
 * <picture>
 *   <source srcset="dialog-dark.png" media="(prefers-color-scheme: dark)">
 *   <img alt="An example BobguiDialog" src="dialog.png">
 * </picture>
 *
 * Typical uses are to display a message, ask a question, or anything else
 * that does not require extensive effort on the user’s part.
 *
 * The main area of a `BobguiDialog` is called the "content area", and is yours
 * to populate with widgets such a `BobguiLabel` or `BobguiEntry`, to present
 * your information, questions, or tasks to the user.
 *
 * In addition, dialogs allow you to add "action widgets". Most commonly,
 * action widgets are buttons. Depending on the platform, action widgets may
 * be presented in the header bar at the top of the window, or at the bottom
 * of the window. To add action widgets, create your `BobguiDialog` using
 * [ctor@Bobgui.Dialog.new_with_buttons], or use
 * [method@Bobgui.Dialog.add_button], [method@Bobgui.Dialog.add_buttons],
 * or [method@Bobgui.Dialog.add_action_widget].
 *
 * `BobguiDialogs` uses some heuristics to decide whether to add a close
 * button to the window decorations. If any of the action buttons use
 * the response ID %BOBGUI_RESPONSE_CLOSE or %BOBGUI_RESPONSE_CANCEL, the
 * close button is omitted.
 *
 * Clicking a button that was added as an action widget will emit the
 * [signal@Bobgui.Dialog::response] signal with a response ID that you specified.
 * BOBGUI will never assign a meaning to positive response IDs; these are
 * entirely user-defined. But for convenience, you can use the response
 * IDs in the [enum@Bobgui.ResponseType] enumeration (these all have values
 * less than zero). If a dialog receives a delete event, the
 * [signal@Bobgui.Dialog::response] signal will be emitted with the
 * %BOBGUI_RESPONSE_DELETE_EVENT response ID.
 *
 * Dialogs are created with a call to [ctor@Bobgui.Dialog.new] or
 * [ctor@Bobgui.Dialog.new_with_buttons]. The latter is recommended; it allows
 * you to set the dialog title, some convenient flags, and add buttons.
 *
 * A “modal” dialog (that is, one which freezes the rest of the application
 * from user input), can be created by calling [method@Bobgui.Window.set_modal]
 * on the dialog. When using [ctor@Bobgui.Dialog.new_with_buttons], you can also
 * pass the %BOBGUI_DIALOG_MODAL flag to make a dialog modal.
 *
 * For the simple dialog in the following example, a [class@Bobgui.MessageDialog]
 * would save some effort. But you’d need to create the dialog contents manually
 * if you had more than a simple message in the dialog.
 *
 * An example for simple `BobguiDialog` usage:
 *
 * ```c
 * // Function to open a dialog box with a message
 * void
 * quick_message (BobguiWindow *parent, char *message)
 * {
 *  BobguiWidget *dialog, *label, *content_area;
 *  BobguiDialogFlags flags;
 *
 *  // Create the widgets
 *  flags = BOBGUI_DIALOG_DESTROY_WITH_PARENT;
 *  dialog = bobgui_dialog_new_with_buttons ("Message",
 *                                        parent,
 *                                        flags,
 *                                        _("_OK"),
 *                                        BOBGUI_RESPONSE_NONE,
 *                                        NULL);
 *  content_area = bobgui_dialog_get_content_area (BOBGUI_DIALOG (dialog));
 *  label = bobgui_label_new (message);
 *
 *  // Ensure that the dialog box is destroyed when the user responds
 *
 *  g_signal_connect_swapped (dialog,
 *                            "response",
 *                            G_CALLBACK (bobgui_window_destroy),
 *                            dialog);
 *
 *  // Add the label, and show everything we’ve added
 *
 *  bobgui_box_append (BOBGUI_BOX (content_area), label);
 *  bobgui_widget_show (dialog);
 * }
 * ```
 *
 * # BobguiDialog as BobguiBuildable
 *
 * The `BobguiDialog` implementation of the `BobguiBuildable` interface exposes the
 * @content_area as an internal child with the name “content_area”.
 *
 * `BobguiDialog` supports a custom `<action-widgets>` element, which can contain
 * multiple `<action-widget>` elements. The “response” attribute specifies a
 * numeric response, and the content of the element is the id of widget
 * (which should be a child of the dialogs @action_area). To mark a response
 * as default, set the “default” attribute of the `<action-widget>` element
 * to true.
 *
 * `BobguiDialog` supports adding action widgets by specifying “action” as
 * the “type” attribute of a `<child>` element. The widget will be added
 * either to the action area or the headerbar of the dialog, depending
 * on the “use-header-bar” property. The response id has to be associated
 * with the action widget using the `<action-widgets>` element.
 *
 * An example of a `BobguiDialog` UI definition fragment:
 *
 * ```xml
 * <object class="BobguiDialog" id="dialog1">
 *   <child type="action">
 *     <object class="BobguiButton" id="button_cancel"/>
 *   </child>
 *   <child type="action">
 *     <object class="BobguiButton" id="button_ok">
 *     </object>
 *   </child>
 *   <action-widgets>
 *     <action-widget response="cancel">button_cancel</action-widget>
 *     <action-widget response="ok" default="true">button_ok</action-widget>
 *   </action-widgets>
 * </object>
 * ```
 *
 * # Accessibility
 *
 * `BobguiDialog` uses the %BOBGUI_ACCESSIBLE_ROLE_DIALOG role.
 *
 * Deprecated: 4.10: Use [class@Bobgui.Window] instead
 */

typedef struct _ResponseData ResponseData;

typedef struct
{
  BobguiWidget *headerbar;
  BobguiWidget *action_area;
  BobguiWidget *content_area;
  BobguiWidget *action_box;
  BobguiSizeGroup *size_group;

  int use_header_bar;
  gboolean constructed;
  ResponseData *action_widgets;
} BobguiDialogPrivate;

struct _ResponseData
{
  ResponseData *next;
  BobguiDialog *dialog;
  BobguiWidget *widget;
  int response_id;
};

static void      bobgui_dialog_add_buttons_valist   (BobguiDialog    *dialog,
                                                  const char   *first_button_text,
                                                  va_list       args);

static gboolean  bobgui_dialog_close_request        (BobguiWindow    *window);
static void      bobgui_dialog_map                  (BobguiWidget    *widget);

static void      bobgui_dialog_close                (BobguiDialog    *dialog);

static ResponseData * get_response_data          (BobguiDialog    *dialog,
                                                  BobguiWidget    *widget,
                                                  gboolean      create);

static void     bobgui_dialog_buildable_interface_init   (BobguiBuildableIface  *iface);
static gboolean bobgui_dialog_buildable_custom_tag_start (BobguiBuildable       *buildable,
                                                       BobguiBuilder         *builder,
                                                       GObject            *child,
                                                       const char         *tagname,
                                                       BobguiBuildableParser *parser,
                                                       gpointer           *data);
static void     bobgui_dialog_buildable_custom_finished  (BobguiBuildable       *buildable,
                                                       BobguiBuilder         *builder,
                                                       GObject            *child,
                                                       const char         *tagname,
                                                       gpointer            user_data);
static void     bobgui_dialog_buildable_add_child        (BobguiBuildable       *buildable,
                                                       BobguiBuilder         *builder,
                                                       GObject            *child,
                                                       const char         *type);


enum {
  PROP_0,
  PROP_USE_HEADER_BAR
};

enum {
  RESPONSE,
  CLOSE,
  LAST_SIGNAL
};

static guint dialog_signals[LAST_SIGNAL];

G_DEFINE_TYPE_WITH_CODE (BobguiDialog, bobgui_dialog, BOBGUI_TYPE_WINDOW,
                         G_ADD_PRIVATE (BobguiDialog)
			 G_IMPLEMENT_INTERFACE (BOBGUI_TYPE_BUILDABLE,
						bobgui_dialog_buildable_interface_init))

static void
set_use_header_bar (BobguiDialog *dialog,
                    int        use_header_bar)
{
  BobguiDialogPrivate *priv = bobgui_dialog_get_instance_private (dialog);

  if (use_header_bar == -1)
    return;

  priv->use_header_bar = use_header_bar;
}

/* A convenience helper for built-in dialogs */
void
bobgui_dialog_set_use_header_bar_from_setting (BobguiDialog *dialog)
{
  BobguiDialogPrivate *priv = bobgui_dialog_get_instance_private (dialog);

  g_assert (!priv->constructed);

  g_object_get (bobgui_widget_get_settings (BOBGUI_WIDGET (dialog)),
                "bobgui-dialogs-use-header", &priv->use_header_bar,
                NULL);
}

static void
bobgui_dialog_set_property (GObject      *object,
                         guint         prop_id,
                         const GValue *value,
                         GParamSpec   *pspec)
{
  BobguiDialog *dialog = BOBGUI_DIALOG (object);

  switch (prop_id)
    {
    case PROP_USE_HEADER_BAR:
      set_use_header_bar (dialog, g_value_get_int (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bobgui_dialog_get_property (GObject      *object,
                         guint         prop_id,
                         GValue       *value,
                         GParamSpec   *pspec)
{
  BobguiDialog *dialog = BOBGUI_DIALOG (object);
  BobguiDialogPrivate *priv = bobgui_dialog_get_instance_private (dialog);

  switch (prop_id)
    {
    case PROP_USE_HEADER_BAR:
      g_value_set_int (value, priv->use_header_bar);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
action_widget_activated (BobguiWidget *widget, BobguiDialog *dialog)
{
  int response_id;

  response_id = bobgui_dialog_get_response_for_widget (dialog, widget);

  bobgui_dialog_response (dialog, response_id);
}

static void
add_response_data (BobguiDialog *dialog,
                   BobguiWidget *child,
                   int        response_id)
{
  ResponseData *ad;
  guint signal_id;

  ad = get_response_data (dialog, child, TRUE);
  ad->response_id = response_id;

  if (BOBGUI_IS_BUTTON (child))
    signal_id = g_signal_lookup ("clicked", BOBGUI_TYPE_BUTTON);
  else
    signal_id = bobgui_widget_class_get_activate_signal (BOBGUI_WIDGET_GET_CLASS (child));

  if (signal_id)
    {
      GClosure *closure;

      closure = g_cclosure_new_object (G_CALLBACK (action_widget_activated),
                                       G_OBJECT (dialog));
      g_signal_connect_closure_by_id (child, signal_id, 0, closure, FALSE);
    }
  else
    g_warning ("Only 'activatable' widgets can be packed into the action area of a BobguiDialog");
}

static void
add_to_header_bar (BobguiDialog *dialog,
                   BobguiWidget *child,
                   int        response_id)
{
  BobguiDialogPrivate *priv = bobgui_dialog_get_instance_private (dialog);

  bobgui_widget_set_valign (child, BOBGUI_ALIGN_CENTER);

  if (response_id == BOBGUI_RESPONSE_CANCEL || response_id == BOBGUI_RESPONSE_HELP)
    bobgui_header_bar_pack_start (BOBGUI_HEADER_BAR (priv->headerbar), child);
  else
    bobgui_header_bar_pack_end (BOBGUI_HEADER_BAR (priv->headerbar), child);

  bobgui_size_group_add_widget (priv->size_group, child);

  if (response_id == BOBGUI_RESPONSE_CANCEL || response_id == BOBGUI_RESPONSE_CLOSE)
    bobgui_header_bar_set_show_title_buttons (BOBGUI_HEADER_BAR (priv->headerbar), FALSE);
}

static void
apply_response_for_action_area (BobguiDialog *dialog,
                                BobguiWidget *child,
                                int        response_id)
{
  BobguiDialogPrivate *priv G_GNUC_UNUSED = bobgui_dialog_get_instance_private (dialog);

  g_assert (bobgui_widget_get_parent (child) == priv->action_area);
}

static void
add_to_action_area (BobguiDialog *dialog,
                    BobguiWidget *child,
                    int        response_id)
{
  BobguiDialogPrivate *priv = bobgui_dialog_get_instance_private (dialog);

  bobgui_widget_set_valign (child, BOBGUI_ALIGN_BASELINE);
  bobgui_box_append (BOBGUI_BOX (priv->action_area), child);
  apply_response_for_action_area (dialog, child, response_id);
}

static void
update_suggested_action (BobguiDialog *dialog,
                         BobguiWidget *child)
{
  BobguiDialogPrivate *priv = bobgui_dialog_get_instance_private (dialog);

  if (priv->use_header_bar)
    {
      if (bobgui_widget_has_css_class (child, "default"))
        bobgui_widget_add_css_class (child, "suggested-action");
      else
        bobgui_widget_remove_css_class (child, "suggested-action");
    }
}

static void
bobgui_dialog_constructed (GObject *object)
{
  BobguiDialog *dialog = BOBGUI_DIALOG (object);
  BobguiDialogPrivate *priv = bobgui_dialog_get_instance_private (dialog);

  G_OBJECT_CLASS (bobgui_dialog_parent_class)->constructed (object);

  priv->constructed = TRUE;
  if (priv->use_header_bar == -1)
    priv->use_header_bar = FALSE;

  if (priv->use_header_bar)
    {
      GList *children, *l;
      BobguiWidget *child;

      children = NULL;
      for (child = bobgui_widget_get_first_child (priv->action_area);
           child != NULL;
           child = bobgui_widget_get_next_sibling (child))
        children = g_list_append (children, child);

      for (l = children; l != NULL; l = l->next)
        {
          gboolean has_default;
          ResponseData *rd;
          int response_id;

          child = l->data;

          has_default = bobgui_widget_has_default (child);
          rd = get_response_data (dialog, child, FALSE);
          response_id = rd ? rd->response_id : BOBGUI_RESPONSE_NONE;

          g_object_ref (child);
          bobgui_box_remove (BOBGUI_BOX (priv->action_area), child);
          add_to_header_bar (dialog, child, response_id);
          g_object_unref (child);

          if (has_default)
            {
              bobgui_window_set_default_widget (BOBGUI_WINDOW (dialog), child);
              update_suggested_action (dialog, child);
            }
        }
      g_list_free (children);

      if (BOBGUI_IS_HEADER_BAR (priv->headerbar))
        _bobgui_header_bar_track_default_decoration (BOBGUI_HEADER_BAR (priv->headerbar));
    }
  else
    {
      bobgui_window_set_titlebar (BOBGUI_WINDOW (dialog), NULL);
      priv->headerbar = NULL;
    }

  bobgui_widget_set_visible (priv->action_box, !priv->use_header_bar);
}

static void
bobgui_dialog_finalize (GObject *obj)
{
  BobguiDialog *dialog = BOBGUI_DIALOG (obj);
  BobguiDialogPrivate *priv = bobgui_dialog_get_instance_private (dialog);

  while (priv->action_widgets)
    g_object_set_data (G_OBJECT (priv->action_widgets->widget),
                       "bobgui-dialog-response-data", NULL);

  g_object_unref (priv->size_group);

  G_OBJECT_CLASS (bobgui_dialog_parent_class)->finalize (obj);
}

static void
bobgui_dialog_class_init (BobguiDialogClass *class)
{
  GObjectClass *gobject_class;
  BobguiWidgetClass *widget_class;
  BobguiWindowClass *window_class;

  gobject_class = G_OBJECT_CLASS (class);
  widget_class = BOBGUI_WIDGET_CLASS (class);
  window_class = BOBGUI_WINDOW_CLASS (class);

  gobject_class->constructed  = bobgui_dialog_constructed;
  gobject_class->set_property = bobgui_dialog_set_property;
  gobject_class->get_property = bobgui_dialog_get_property;
  gobject_class->finalize = bobgui_dialog_finalize;

  widget_class->map = bobgui_dialog_map;

  window_class->close_request = bobgui_dialog_close_request;

  class->close = bobgui_dialog_close;

  /**
   * BobguiDialog::response:
   * @dialog: the object on which the signal is emitted
   * @response_id: the response ID
   *
   * Emitted when an action widget is clicked.
   *
   * The signal is also emitted when the dialog receives a
   * delete event, and when [method@Bobgui.Dialog.response] is called.
   * On a delete event, the response ID is %BOBGUI_RESPONSE_DELETE_EVENT.
   * Otherwise, it depends on which action widget was clicked.
   *
   * Deprecated: 4.10: Use [class@Bobgui.Window] instead
   */
  dialog_signals[RESPONSE] =
    g_signal_new (I_("response"),
		  G_OBJECT_CLASS_TYPE (class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (BobguiDialogClass, response),
		  NULL, NULL,
		  NULL,
		  G_TYPE_NONE, 1,
		  G_TYPE_INT);

  /**
   * BobguiDialog::close:
   *
   * Emitted when the user uses a keybinding to close the dialog.
   *
   * This is a [keybinding signal](class.SignalAction.html).
   *
   * The default binding for this signal is the Escape key.
   *
   * Deprecated: 4.10: Use [class@Bobgui.Window] instead
   */
  dialog_signals[CLOSE] =
    g_signal_new (I_("close"),
		  G_OBJECT_CLASS_TYPE (class),
		  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
		  G_STRUCT_OFFSET (BobguiDialogClass, close),
		  NULL, NULL,
		  NULL,
		  G_TYPE_NONE, 0);

  /**
   * BobguiDialog:use-header-bar:
   *
   * %TRUE if the dialog uses a headerbar for action buttons
   * instead of the action-area.
   *
   * For technical reasons, this property is declared as an integer
   * property, but you should only set it to %TRUE or %FALSE.
   *
   * ## Creating a dialog with headerbar
   *
   * Builtin `BobguiDialog` subclasses such as [class@Bobgui.ColorChooserDialog]
   * set this property according to platform conventions (using the
   * [property@Bobgui.Settings:bobgui-dialogs-use-header] setting).
   *
   * Here is how you can achieve the same:
   *
   * ```c
   * g_object_get (settings, "bobgui-dialogs-use-header", &header, NULL);
   * dialog = g_object_new (BOBGUI_TYPE_DIALOG, header, TRUE, NULL);
   * ```
   *
   * Deprecated: 4.10: Use [class@Bobgui.Window] instead
   */
  g_object_class_install_property (gobject_class,
                                   PROP_USE_HEADER_BAR,
                                   g_param_spec_int ("use-header-bar", NULL, NULL,
                                                     -1, 1, -1,
                                                     BOBGUI_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY));

  bobgui_widget_class_add_binding_signal (widget_class, GDK_KEY_Escape, 0, "close", NULL);

  /* Bind class to template
   */
  bobgui_widget_class_set_template_from_resource (widget_class, "/org/bobgui/libbobgui/ui/bobguidialog.ui");
  bobgui_widget_class_bind_template_child_internal_private (widget_class, BobguiDialog, headerbar);
  bobgui_widget_class_bind_template_child_internal_private (widget_class, BobguiDialog, action_area);
  bobgui_widget_class_bind_template_child_internal_private (widget_class, BobguiDialog, content_area);
  bobgui_widget_class_bind_template_child_private (widget_class, BobguiDialog, action_box);

  bobgui_widget_class_set_accessible_role (widget_class, BOBGUI_ACCESSIBLE_ROLE_DIALOG);
}

static void
bobgui_dialog_init (BobguiDialog *dialog)
{
  BobguiDialogPrivate *priv = bobgui_dialog_get_instance_private (dialog);

  bobgui_widget_add_css_class (BOBGUI_WIDGET (dialog), "dialog");

  priv->use_header_bar = -1;
  priv->size_group = bobgui_size_group_new (BOBGUI_SIZE_GROUP_HORIZONTAL);

  bobgui_widget_init_template (BOBGUI_WIDGET (dialog));
}

static BobguiBuildableIface *parent_buildable_iface;

static void
bobgui_dialog_buildable_interface_init (BobguiBuildableIface *iface)
{
  parent_buildable_iface = g_type_interface_peek_parent (iface);
  iface->custom_tag_start = bobgui_dialog_buildable_custom_tag_start;
  iface->custom_finished = bobgui_dialog_buildable_custom_finished;
  iface->add_child = bobgui_dialog_buildable_add_child;
}

static gboolean
bobgui_dialog_close_request (BobguiWindow *window)
{
  bobgui_dialog_response (BOBGUI_DIALOG (window), BOBGUI_RESPONSE_DELETE_EVENT);

  return BOBGUI_WINDOW_CLASS (bobgui_dialog_parent_class)->close_request (window);
}

/* A far too tricky heuristic for getting the right initial
 * focus widget if none was set. What we do is we focus the first
 * widget in the tab chain, but if this results in the focus
 * ending up on one of the response widgets _other_ than the
 * default response, we focus the default response instead.
 *
 * Additionally, skip selectable labels when looking for the
 * right initial focus widget.
 */
static void
bobgui_dialog_map (BobguiWidget *widget)
{
  BobguiWidget *default_widget, *focus;
  BobguiWindow *window = BOBGUI_WINDOW (widget);
  BobguiDialog *dialog = BOBGUI_DIALOG (widget);
  BobguiDialogPrivate *priv = bobgui_dialog_get_instance_private (dialog);
  ResponseData *rd;

  if (bobgui_window_get_transient_for (window) == NULL)
    g_message ("BobguiDialog mapped without a transient parent. This is discouraged.");

  BOBGUI_WIDGET_CLASS (bobgui_dialog_parent_class)->map (widget);

  focus = bobgui_window_get_focus (window);
  if (!focus)
    {
      BobguiWidget *first_focus = NULL;

      do
        {
          g_signal_emit_by_name (window, "move_focus", BOBGUI_DIR_TAB_FORWARD);

          focus = bobgui_window_get_focus (window);
          if (BOBGUI_IS_LABEL (focus) &&
              !bobgui_label_get_current_uri (BOBGUI_LABEL (focus)))
            bobgui_label_select_region (BOBGUI_LABEL (focus), 0, 0);

          if (first_focus == NULL)
            first_focus = focus;
          else if (first_focus == focus)
            break;

          if (!BOBGUI_IS_LABEL (focus))
            break;
        }
      while (TRUE);

      default_widget = bobgui_window_get_default_widget (window);
      for (rd = priv->action_widgets; rd != NULL; rd = rd->next)
        {
          if ((focus == NULL || rd->widget == focus) &&
               rd->widget != default_widget &&
               default_widget)
            {
              bobgui_widget_grab_focus (default_widget);
              break;
            }
        }
    }
}

static void
bobgui_dialog_close (BobguiDialog *dialog)
{
  bobgui_window_close (BOBGUI_WINDOW (dialog));
}

/**
 * bobgui_dialog_new:
 *
 * Creates a new dialog box.
 *
 * Widgets should not be packed into the `BobguiWindow`
 * directly, but into the @content_area and @action_area,
 * as described above.
 *
 * Returns: the new dialog as a `BobguiWidget`
 *
 * Deprecated: 4.10: Use [class@Bobgui.Window] instead
 */
BobguiWidget*
bobgui_dialog_new (void)
{
  return g_object_new (BOBGUI_TYPE_DIALOG, NULL);
}

static BobguiWidget*
bobgui_dialog_new_empty (const char      *title,
                      BobguiWindow       *parent,
                      BobguiDialogFlags   flags)
{
  BobguiDialog *dialog;

  dialog = g_object_new (BOBGUI_TYPE_DIALOG,
                         "use-header-bar", (flags & BOBGUI_DIALOG_USE_HEADER_BAR) != 0,
                         NULL);

  if (title)
    bobgui_window_set_title (BOBGUI_WINDOW (dialog), title);

  if (parent)
    bobgui_window_set_transient_for (BOBGUI_WINDOW (dialog), parent);

  if (flags & BOBGUI_DIALOG_MODAL)
    bobgui_window_set_modal (BOBGUI_WINDOW (dialog), TRUE);

  if (flags & BOBGUI_DIALOG_DESTROY_WITH_PARENT)
    bobgui_window_set_destroy_with_parent (BOBGUI_WINDOW (dialog), TRUE);

  return BOBGUI_WIDGET (dialog);
}

/**
 * bobgui_dialog_new_with_buttons:
 * @title: (nullable): Title of the dialog
 * @parent: (nullable): Transient parent of the dialog
 * @flags: from `BobguiDialogFlags`
 * @first_button_text: (nullable): text to go in first button
 * @...: response ID for first button, then additional buttons, ending with %NULL
 *
 * Creates a new `BobguiDialog` with the given title and transient parent.
 *
 * The @flags argument can be used to make the dialog modal, have it
 * destroyed along with its transient parent, or make it use a headerbar.
 *
 * Button text/response ID pairs should be listed in pairs, with a %NULL
 * pointer ending the list. Button text can be arbitrary text. A response
 * ID can be any positive number, or one of the values in the
 * [enum@Bobgui.ResponseType] enumeration. If the user clicks one of these
 * buttons, `BobguiDialog` will emit the [signal@Bobgui.Dialog::response] signal
 * with the corresponding response ID.
 *
 * If a `BobguiDialog` receives a delete event, it will emit ::response with a
 * response ID of %BOBGUI_RESPONSE_DELETE_EVENT.
 *
 * However, destroying a dialog does not emit the ::response signal;
 * so be careful relying on ::response when using the
 * %BOBGUI_DIALOG_DESTROY_WITH_PARENT flag.
 *
 * Here’s a simple example:
 * ```c
 * BobguiWindow *main_app_window; // Window the dialog should show up on
 * BobguiWidget *dialog;
 * BobguiDialogFlags flags = BOBGUI_DIALOG_MODAL | BOBGUI_DIALOG_DESTROY_WITH_PARENT;
 * dialog = bobgui_dialog_new_with_buttons ("My dialog",
 *                                       main_app_window,
 *                                       flags,
 *                                       _("_OK"),
 *                                       BOBGUI_RESPONSE_ACCEPT,
 *                                       _("_Cancel"),
 *                                       BOBGUI_RESPONSE_REJECT,
 *                                       NULL);
 * ```
 *
 * Returns: a new `BobguiDialog`
 *
 * Deprecated: 4.10: Use [class@Bobgui.Window] instead
 */
BobguiWidget*
bobgui_dialog_new_with_buttons (const char     *title,
                             BobguiWindow      *parent,
                             BobguiDialogFlags  flags,
                             const char     *first_button_text,
                             ...)
{
  BobguiDialog *dialog;
  va_list args;

  dialog = BOBGUI_DIALOG (bobgui_dialog_new_empty (title, parent, flags));

  va_start (args, first_button_text);

  bobgui_dialog_add_buttons_valist (dialog,
                                 first_button_text,
                                 args);

  va_end (args);

  return BOBGUI_WIDGET (dialog);
}

static void
response_data_free (gpointer data)
{
  ResponseData *ad = data;
  BobguiDialogPrivate *priv = bobgui_dialog_get_instance_private (ad->dialog);

  if (priv->action_widgets == ad)
    {
      priv->action_widgets = ad->next;
    }
  else
    {
      ResponseData *prev = priv->action_widgets;
      while (prev)
        {
          if (prev->next == ad)
            {
              prev->next = ad->next;
              break;
            }
          prev = prev->next;
        }
    }
  g_slice_free (ResponseData, data);
}

static ResponseData *
get_response_data (BobguiDialog *dialog,
                   BobguiWidget *widget,
                   gboolean   create)
{
  BobguiDialogPrivate *priv = bobgui_dialog_get_instance_private (dialog);

  ResponseData *ad = g_object_get_data (G_OBJECT (widget),
                                        "bobgui-dialog-response-data");

  if (ad == NULL && create)
    {
      ad = g_slice_new (ResponseData);
      ad->dialog = dialog;
      ad->widget = widget;
      g_object_set_data_full (G_OBJECT (widget),
                              I_("bobgui-dialog-response-data"),
                              ad,
                              response_data_free);
      ad->next = priv->action_widgets;
      priv->action_widgets = ad;
    }

  return ad;
}

/**
 * bobgui_dialog_add_action_widget:
 * @dialog: a `BobguiDialog`
 * @child: an activatable widget
 * @response_id: response ID for @child
 *
 * Adds an activatable widget to the action area of a `BobguiDialog`.
 *
 * BOBGUI connects a signal handler that will emit the
 * [signal@Bobgui.Dialog::response] signal on the dialog when the widget
 * is activated. The widget is appended to the end of the dialog’s action
 * area.
 *
 * If you want to add a non-activatable widget, simply pack it into
 * the @action_area field of the `BobguiDialog` struct.
 *
 * Deprecated: 4.10: Use [class@Bobgui.Window] instead
 */
void
bobgui_dialog_add_action_widget (BobguiDialog *dialog,
                              BobguiWidget *child,
                              int        response_id)
{
  BobguiDialogPrivate *priv = bobgui_dialog_get_instance_private (dialog);

  g_return_if_fail (BOBGUI_IS_DIALOG (dialog));
  g_return_if_fail (BOBGUI_IS_WIDGET (child));

  add_response_data (dialog, child, response_id);

  if (priv->constructed && priv->use_header_bar)
    {
      add_to_header_bar (dialog, child, response_id);

      if (bobgui_widget_has_default (child))
        {
          bobgui_window_set_default_widget (BOBGUI_WINDOW (dialog), child);
          update_suggested_action (dialog, child);
        }
    }
  else
    add_to_action_area (dialog, child, response_id);
}

/**
 * bobgui_dialog_add_button:
 * @dialog: a `BobguiDialog`
 * @button_text: text of button
 * @response_id: response ID for the button
 *
 * Adds a button with the given text.
 *
 * BOBGUI arranges things so that clicking the button will emit the
 * [signal@Bobgui.Dialog::response] signal with the given @response_id.
 * The button is appended to the end of the dialog’s action area.
 * The button widget is returned, but usually you don’t need it.
 *
 * Returns: (transfer none): the `BobguiButton` widget that was added
 *
 * Deprecated: 4.10: Use [class@Bobgui.Window] instead
 */
BobguiWidget*
bobgui_dialog_add_button (BobguiDialog   *dialog,
                       const char *button_text,
                       int          response_id)
{
  BobguiWidget *button;

  g_return_val_if_fail (BOBGUI_IS_DIALOG (dialog), NULL);
  g_return_val_if_fail (button_text != NULL, NULL);

  button = bobgui_button_new_with_label (button_text);
  bobgui_button_set_use_underline (BOBGUI_BUTTON (button), TRUE);

  bobgui_dialog_add_action_widget (dialog, button, response_id);

  return button;
}

static void
bobgui_dialog_add_buttons_valist (BobguiDialog      *dialog,
                               const char     *first_button_text,
                               va_list         args)
{
  const char * text;
  int response_id;

  g_return_if_fail (BOBGUI_IS_DIALOG (dialog));

  if (first_button_text == NULL)
    return;

  text = first_button_text;
  response_id = va_arg (args, int);

  while (text != NULL)
    {
      bobgui_dialog_add_button (dialog, text, response_id);

      text = va_arg (args, char *);
      if (text == NULL)
        break;
      response_id = va_arg (args, int);
    }
}

/**
 * bobgui_dialog_add_buttons:
 * @dialog: a `BobguiDialog`
 * @first_button_text: button text
 * @...: response ID for first button, then more text-response_id pairs
 *
 * Adds multiple buttons.
 *
 * This is the same as calling [method@Bobgui.Dialog.add_button]
 * repeatedly. The variable argument list should be %NULL-terminated
 * as with [ctor@Bobgui.Dialog.new_with_buttons]. Each button must have both
 * text and response ID.
 *
 * Deprecated: 4.10: Use [class@Bobgui.Window] instead
 */
void
bobgui_dialog_add_buttons (BobguiDialog   *dialog,
                        const char *first_button_text,
                        ...)
{
  va_list args;

  va_start (args, first_button_text);

  bobgui_dialog_add_buttons_valist (dialog,
                                 first_button_text,
                                 args);

  va_end (args);
}

/**
 * bobgui_dialog_set_response_sensitive:
 * @dialog: a `BobguiDialog`
 * @response_id: a response ID
 * @setting: %TRUE for sensitive
 *
 * A convenient way to sensitize/desensitize dialog buttons.
 *
 * Calls `bobgui_widget_set_sensitive (widget, @setting)`
 * for each widget in the dialog’s action area with the given @response_id.
 *
 * Deprecated: 4.10: Use [class@Bobgui.Window] instead
 */
void
bobgui_dialog_set_response_sensitive (BobguiDialog *dialog,
                                   int        response_id,
                                   gboolean   setting)
{
  BobguiDialogPrivate *priv = bobgui_dialog_get_instance_private (dialog);
  ResponseData *rd;

  g_return_if_fail (BOBGUI_IS_DIALOG (dialog));

  for (rd = priv->action_widgets; rd != NULL; rd = rd->next)
    {
      if (rd->response_id == response_id)
        bobgui_widget_set_sensitive (rd->widget, setting);
    }
}

/**
 * bobgui_dialog_set_default_response:
 * @dialog: a `BobguiDialog`
 * @response_id: a response ID
 *
 * Sets the default widget for the dialog based on the response ID.
 *
 * Pressing “Enter” normally activates the default widget.
 *
 * Deprecated: 4.10: Use [class@Bobgui.Window] instead
 */
void
bobgui_dialog_set_default_response (BobguiDialog *dialog,
                                 int        response_id)
{
  BobguiDialogPrivate *priv = bobgui_dialog_get_instance_private (dialog);
  ResponseData *rd;

  g_return_if_fail (BOBGUI_IS_DIALOG (dialog));

  for (rd = priv->action_widgets; rd != NULL; rd = rd->next)
    {
      if (rd->response_id == response_id)
        {
          bobgui_window_set_default_widget (BOBGUI_WINDOW (dialog), rd->widget);
          update_suggested_action (dialog, rd->widget);
        }
    }
}

/**
 * bobgui_dialog_response: (attributes org.bobgui.Method.signal=response)
 * @dialog: a `BobguiDialog`
 * @response_id: response ID
 *
 * Emits the ::response signal with the given response ID.
 *
 * Used to indicate that the user has responded to the dialog in some way.
 *
 * Deprecated: 4.10: Use [class@Bobgui.Window] instead
 */
void
bobgui_dialog_response (BobguiDialog *dialog,
                     int        response_id)
{
  g_return_if_fail (BOBGUI_IS_DIALOG (dialog));

  g_signal_emit (dialog,
		 dialog_signals[RESPONSE],
		 0,
		 response_id);
}

/**
 * bobgui_dialog_get_widget_for_response:
 * @dialog: a `BobguiDialog`
 * @response_id: the response ID used by the @dialog widget
 *
 * Gets the widget button that uses the given response ID in the action area
 * of a dialog.
 *
 * Returns: (nullable) (transfer none): the @widget button that uses the given
 *   @response_id
 *
 * Deprecated: 4.10: Use [class@Bobgui.Window] instead
 */
BobguiWidget*
bobgui_dialog_get_widget_for_response (BobguiDialog *dialog,
                                    int        response_id)
{
  BobguiDialogPrivate *priv = bobgui_dialog_get_instance_private (dialog);
  ResponseData *rd;

  g_return_val_if_fail (BOBGUI_IS_DIALOG (dialog), NULL);

  for (rd = priv->action_widgets; rd != NULL; rd = rd->next)
    {
      if (rd->response_id == response_id)
        return rd->widget;
    }

  return NULL;
}

/**
 * bobgui_dialog_get_response_for_widget:
 * @dialog: a `BobguiDialog`
 * @widget: a widget in the action area of @dialog
 *
 * Gets the response id of a widget in the action area
 * of a dialog.
 *
 * Returns: the response id of @widget, or %BOBGUI_RESPONSE_NONE
 *  if @widget doesn’t have a response id set.
 *
 * Deprecated: 4.10: Use [class@Bobgui.Window] instead
 */
int
bobgui_dialog_get_response_for_widget (BobguiDialog *dialog,
				    BobguiWidget *widget)
{
  ResponseData *rd;

  rd = get_response_data (dialog, widget, FALSE);
  if (!rd)
    return BOBGUI_RESPONSE_NONE;
  else
    return rd->response_id;
}

typedef struct {
  char *widget_name;
  int response_id;
  gboolean is_default;
  int line;
  int col;
} ActionWidgetInfo;

typedef struct {
  BobguiDialog *dialog;
  BobguiBuilder *builder;
  GSList *items;
  int response_id;
  gboolean is_default;
  gboolean is_text;
  GString *string;
  gboolean in_action_widgets;
  int line;
  int col;
} SubParserData;

static void
free_action_widget_info (gpointer data)
{
  ActionWidgetInfo *item = data;

  g_free (item->widget_name);
  g_free (item);
}

static void
parser_start_element (BobguiBuildableParseContext *context,
                      const char               *element_name,
                      const char              **names,
                      const char              **values,
                      gpointer                  user_data,
                      GError                  **error)
{
  SubParserData *data = (SubParserData*)user_data;

  if (strcmp (element_name, "action-widget") == 0)
    {
      const char *response;
      gboolean is_default = FALSE;
      GValue gvalue = G_VALUE_INIT;

      if (!_bobgui_builder_check_parent (data->builder, context, "action-widgets", error))
        return;

      if (!g_markup_collect_attributes (element_name, names, values, error,
                                        G_MARKUP_COLLECT_STRING, "response", &response,
                                        G_MARKUP_COLLECT_BOOLEAN|G_MARKUP_COLLECT_OPTIONAL, "default", &is_default,
                                        G_MARKUP_COLLECT_INVALID))
        {
          _bobgui_builder_prefix_error (data->builder, context, error);
          return;
        }

      if (!bobgui_builder_value_from_string_type (data->builder, BOBGUI_TYPE_RESPONSE_TYPE, response, &gvalue, error))
        {
          _bobgui_builder_prefix_error (data->builder, context, error);
          return;
        }

      data->response_id = g_value_get_enum (&gvalue);
      data->is_default = is_default;
      data->is_text = TRUE;
      g_string_set_size (data->string, 0);
      bobgui_buildable_parse_context_get_position (context, &data->line, &data->col);
    }
  else if (strcmp (element_name, "action-widgets") == 0)
    {
      if (!_bobgui_builder_check_parent (data->builder, context, "object", error))
        return;

      if (!g_markup_collect_attributes (element_name, names, values, error,
                                        G_MARKUP_COLLECT_INVALID, NULL, NULL,
                                        G_MARKUP_COLLECT_INVALID))
        _bobgui_builder_prefix_error (data->builder, context, error);

      data->in_action_widgets = TRUE;
    }
  else
    {
      _bobgui_builder_error_unhandled_tag (data->builder, context,
                                        "BobguiDialog", element_name,
                                        error);
    }
}

static void
parser_text_element (BobguiBuildableParseContext *context,
                     const char               *text,
                     gsize                     text_len,
                     gpointer                  user_data,
                     GError                  **error)
{
  SubParserData *data = (SubParserData*)user_data;

  if (data->is_text)
    g_string_append_len (data->string, text, text_len);
}

static void
parser_end_element (BobguiBuildableParseContext  *context,
                    const char                *element_name,
                    gpointer                   user_data,
                    GError                   **error)
{
  SubParserData *data = (SubParserData*)user_data;

  if (data->is_text)
    {
      ActionWidgetInfo *item;

      item = g_new (ActionWidgetInfo, 1);
      item->widget_name = g_strdup (data->string->str);
      item->response_id = data->response_id;
      item->is_default = data->is_default;
      item->line = data->line;
      item->col = data->col;

      data->items = g_slist_prepend (data->items, item);
      data->is_default = FALSE;
      data->is_text = FALSE;
    }
}

static const BobguiBuildableParser sub_parser =
  {
    parser_start_element,
    parser_end_element,
    parser_text_element,
  };

static gboolean
bobgui_dialog_buildable_custom_tag_start (BobguiBuildable       *buildable,
                                       BobguiBuilder         *builder,
                                       GObject            *child,
                                       const char         *tagname,
                                       BobguiBuildableParser *parser,
                                       gpointer           *parser_data)
{
  SubParserData *data;

  if (child)
    return FALSE;

  if (strcmp (tagname, "action-widgets") == 0)
    {
      data = g_slice_new0 (SubParserData);
      data->dialog = BOBGUI_DIALOG (buildable);
      data->builder = builder;
      data->string = g_string_new ("");
      data->items = NULL;
      data->in_action_widgets = FALSE;

      *parser = sub_parser;
      *parser_data = data;
      return TRUE;
    }

  return parent_buildable_iface->custom_tag_start (buildable, builder, child,
						   tagname, parser, parser_data);
}

static void
bobgui_dialog_buildable_custom_finished (BobguiBuildable *buildable,
				      BobguiBuilder   *builder,
				      GObject      *child,
				      const char   *tagname,
				      gpointer      user_data)
{
  BobguiDialog *dialog = BOBGUI_DIALOG (buildable);
  BobguiDialogPrivate *priv = bobgui_dialog_get_instance_private (dialog);
  GSList *l;
  SubParserData *data;
  GObject *object;
  ResponseData *ad;
  guint signal_id;

  if (strcmp (tagname, "action-widgets") != 0)
    {
      parent_buildable_iface->custom_finished (buildable, builder, child,
                                               tagname, user_data);
      return;
    }

  data = (SubParserData*)user_data;
  data->items = g_slist_reverse (data->items);

  for (l = data->items; l; l = l->next)
    {
      ActionWidgetInfo *item = l->data;
      gboolean is_action;

      object = _bobgui_builder_lookup_object (builder, item->widget_name, item->line, item->col);
      if (!object)
        continue;

      /* If the widget already has response data at this point, it
       * was either added by bobgui_dialog_add_action_widget(), or via
       * <child type="action"> or by moving an action area child
       * to the header bar. In these cases, apply placement heuristics
       * based on the response id.
       */
      is_action = get_response_data (dialog, BOBGUI_WIDGET (object), FALSE) != NULL;

      ad = get_response_data (dialog, BOBGUI_WIDGET (object), TRUE);
      ad->response_id = item->response_id;

      if (BOBGUI_IS_BUTTON (object))
	signal_id = g_signal_lookup ("clicked", BOBGUI_TYPE_BUTTON);
      else
	signal_id = bobgui_widget_class_get_activate_signal (BOBGUI_WIDGET_GET_CLASS (object));

      if (signal_id && !is_action)
	{
	  GClosure *closure;

	  closure = g_cclosure_new_object (G_CALLBACK (action_widget_activated),
					   G_OBJECT (dialog));
	  g_signal_connect_closure_by_id (object, signal_id, 0, closure, FALSE);
	}

      if (bobgui_widget_get_parent (BOBGUI_WIDGET (object)) == priv->action_area)
        {
          apply_response_for_action_area (dialog, BOBGUI_WIDGET (object), ad->response_id);
        }
      else if (bobgui_widget_get_ancestor (BOBGUI_WIDGET (object), BOBGUI_TYPE_HEADER_BAR) == priv->headerbar)
        {
          if (is_action)
            {
              g_object_ref (object);
              bobgui_header_bar_remove (BOBGUI_HEADER_BAR (priv->headerbar), BOBGUI_WIDGET (object));
              add_to_header_bar (dialog, BOBGUI_WIDGET (object), ad->response_id);
              g_object_unref (object);
            }
        }

      if (item->is_default)
        {
          bobgui_window_set_default_widget (BOBGUI_WINDOW (dialog), BOBGUI_WIDGET (object));
          update_suggested_action (dialog, BOBGUI_WIDGET (object));
        }
    }

  g_slist_free_full (data->items, free_action_widget_info);
  g_string_free (data->string, TRUE);
  g_slice_free (SubParserData, data);
}

static void
bobgui_dialog_buildable_add_child (BobguiBuildable  *buildable,
                                BobguiBuilder    *builder,
                                GObject       *child,
                                const char    *type)
{
  BobguiDialog *dialog = BOBGUI_DIALOG (buildable);
  BobguiDialogPrivate *priv = bobgui_dialog_get_instance_private (dialog);

  if (type == NULL)
    parent_buildable_iface->add_child (buildable, builder, child, type);
  else if (g_str_equal (type, "titlebar"))
    {
      priv->headerbar = BOBGUI_WIDGET (child);
      if (BOBGUI_IS_HEADER_BAR (priv->headerbar))
        _bobgui_header_bar_track_default_decoration (BOBGUI_HEADER_BAR (priv->headerbar));
      bobgui_window_set_titlebar (BOBGUI_WINDOW (buildable), priv->headerbar);
    }
  else if (g_str_equal (type, "action"))
    bobgui_dialog_add_action_widget (BOBGUI_DIALOG (buildable), BOBGUI_WIDGET (child), BOBGUI_RESPONSE_NONE);
  else
    BOBGUI_BUILDER_WARN_INVALID_CHILD_TYPE (buildable, type);
}

BobguiWidget *
bobgui_dialog_get_action_area (BobguiDialog *dialog)
{
  BobguiDialogPrivate *priv = bobgui_dialog_get_instance_private (dialog);

  g_return_val_if_fail (BOBGUI_IS_DIALOG (dialog), NULL);

  return priv->action_area;
}

/**
 * bobgui_dialog_get_header_bar:
 * @dialog: a `BobguiDialog`
 *
 * Returns the header bar of @dialog.
 *
 * Note that the headerbar is only used by the dialog if the
 * [property@Bobgui.Dialog:use-header-bar] property is %TRUE.
 *
 * Returns: (type Bobgui.HeaderBar) (transfer none): the header bar
 *
 * Deprecated: 4.10: Use [class@Bobgui.Window] instead
 */
BobguiWidget *
bobgui_dialog_get_header_bar (BobguiDialog *dialog)
{
  BobguiDialogPrivate *priv = bobgui_dialog_get_instance_private (dialog);

  g_return_val_if_fail (BOBGUI_IS_DIALOG (dialog), NULL);

  return priv->headerbar;
}

/**
 * bobgui_dialog_get_content_area:
 * @dialog: a `BobguiDialog`
 *
 * Returns the content area of @dialog.
 *
 * Returns: (type Bobgui.Box) (transfer none): the content area `BobguiBox`.
 *
 * Deprecated: 4.10: Use [class@Bobgui.Window] instead
 */
BobguiWidget *
bobgui_dialog_get_content_area (BobguiDialog *dialog)
{
  BobguiDialogPrivate *priv = bobgui_dialog_get_instance_private (dialog);

  g_return_val_if_fail (BOBGUI_IS_DIALOG (dialog), NULL);

  return priv->content_area;
}
