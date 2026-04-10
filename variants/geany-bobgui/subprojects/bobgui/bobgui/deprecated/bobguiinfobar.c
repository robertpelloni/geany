/*
 * bobguiinfobar.c
 * This file is part of BOBGUI
 *
 * Copyright (C) 2005 - Paolo Maggi
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
 * Modified by the gedit Team, 2005. See the AUTHORS file for a
 * list of people on the bobgui Team.
 * See the gedit ChangeLog files for a list of changes.
 *
 * Modified by the BOBGUI team, 2008-2009.
 */


#include "config.h"

#include <stdlib.h>

#include "bobguiinfobar.h"
#include "bobguibuildable.h"
#include "bobguibuilderprivate.h"
#include "bobguibox.h"
#include "bobguilabel.h"
#include "bobguibutton.h"
#include "bobguiimage.h"
#include "bobguienums.h"
#include "deprecated/bobguidialog.h"
#include "bobguirevealer.h"
#include "bobguiprivate.h"
#include "bobguitypebuiltins.h"
#include "bobguiwidgetprivate.h"
#include "bobguibinlayout.h"
#include "bobguigestureclick.h"

#include <glib/gi18n-lib.h>

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

/**
 * BobguiInfoBar:
 *
 * `BobguiInfoBar` can be used to show messages to the user without a dialog.
 *
 * <picture>
 *   <source srcset="info-bar-dark.png" media="(prefers-color-scheme: dark)">
 *   <img alt="An example BobguiInfoBar" src="info-bar.png">
 * </picture>
 *
 * It is often temporarily shown at the top or bottom of a document.
 * In contrast to [class@Bobgui.Dialog], which has an action area at the
 * bottom, `BobguiInfoBar` has an action area at the side.
 *
 * The API of `BobguiInfoBar` is very similar to `BobguiDialog`, allowing you
 * to add buttons to the action area with [method@Bobgui.InfoBar.add_button]
 * or [ctor@Bobgui.InfoBar.new_with_buttons]. The sensitivity of action widgets
 * can be controlled with [method@Bobgui.InfoBar.set_response_sensitive].
 *
 * To add widgets to the main content area of a `BobguiInfoBar`, use
 * [method@Bobgui.InfoBar.add_child].
 *
 * Similar to [class@Bobgui.MessageDialog], the contents of a `BobguiInfoBar`
 * can by classified as error message, warning, informational message, etc,
 * by using [method@Bobgui.InfoBar.set_message_type]. BOBGUI may use the message
 * type to determine how the message is displayed.
 *
 * A simple example for using a `BobguiInfoBar`:
 * ```c
 * BobguiWidget *message_label;
 * BobguiWidget *widget;
 * BobguiWidget *grid;
 * BobguiInfoBar *bar;
 *
 * // set up info bar
 * widget = bobgui_info_bar_new ();
 * bar = BOBGUI_INFO_BAR (widget);
 * grid = bobgui_grid_new ();
 *
 * message_label = bobgui_label_new ("");
 * bobgui_info_bar_add_child (bar, message_label);
 * bobgui_info_bar_add_button (bar,
 *                          _("_OK"),
 *                          BOBGUI_RESPONSE_OK);
 * g_signal_connect (bar,
 *                   "response",
 *                   G_CALLBACK (bobgui_widget_hide),
 *                   NULL);
 * bobgui_grid_attach (BOBGUI_GRID (grid),
 *                  widget,
 *                  0, 2, 1, 1);
 *
 * // ...
 *
 * // show an error message
 * bobgui_label_set_text (BOBGUI_LABEL (message_label), "An error occurred!");
 * bobgui_info_bar_set_message_type (bar, BOBGUI_MESSAGE_ERROR);
 * bobgui_widget_show (bar);
 * ```
 *
 * # BobguiInfoBar as BobguiBuildable
 *
 * `BobguiInfoBar` supports a custom `<action-widgets>` element, which can contain
 * multiple `<action-widget>` elements. The “response” attribute specifies a
 * numeric response, and the content of the element is the id of widget
 * (which should be a child of the dialogs @action_area).
 *
 * `BobguiInfoBar` supports adding action widgets by specifying “action” as
 * the “type” attribute of a `<child>` element. The widget will be added
 * either to the action area. The response id has to be associated
 * with the action widget using the `<action-widgets>` element.
 *
 * # CSS nodes
 *
 * `BobguiInfoBar` has a single CSS node with name infobar. The node may get
 * one of the style classes .info, .warning, .error or .question, depending
 * on the message type.
 * If the info bar shows a close button, that button will have the .close
 * style class applied.
 *
 * Deprecated: 4.10: There is no replacement in BOBGUI for an "info bar" widget;
 *   you can use [class@Bobgui.Revealer] with a [class@Bobgui.Box] containing a
 *   [class@Bobgui.Label] and an optional [class@Bobgui.Button], according to
 *   your application's design.
 */

enum
{
  PROP_0,
  PROP_MESSAGE_TYPE,
  PROP_SHOW_CLOSE_BUTTON,
  PROP_REVEALED,
  LAST_PROP
};

typedef struct _BobguiInfoBarClass BobguiInfoBarClass;

struct _BobguiInfoBar
{
  BobguiWidget parent_instance;

  BobguiWidget *content_area;
  BobguiWidget *action_area;
  BobguiWidget *close_button;
  BobguiWidget *revealer;

  BobguiMessageType message_type;
  int default_response;
  gboolean default_response_sensitive;
};

struct _BobguiInfoBarClass
{
  BobguiWidgetClass parent_class;

  void (* response) (BobguiInfoBar *info_bar, int response_id);
  void (* close)    (BobguiInfoBar *info_bar);
};

typedef struct _ResponseData ResponseData;

struct _ResponseData
{
  int response_id;
  gulong handler_id;
};

enum
{
  RESPONSE,
  CLOSE,
  LAST_SIGNAL
};

static GParamSpec *props[LAST_PROP] = { NULL, };
static guint signals[LAST_SIGNAL];

static void     bobgui_info_bar_set_property (GObject        *object,
                                           guint           prop_id,
                                           const GValue   *value,
                                           GParamSpec     *pspec);
static void     bobgui_info_bar_get_property (GObject        *object,
                                           guint           prop_id,
                                           GValue         *value,
                                           GParamSpec     *pspec);
static void     bobgui_info_bar_buildable_interface_init   (BobguiBuildableIface  *iface);
static gboolean bobgui_info_bar_buildable_custom_tag_start (BobguiBuildable       *buildable,
                                                         BobguiBuilder         *builder,
                                                         GObject            *child,
                                                         const char         *tagname,
                                                         BobguiBuildableParser *parser,
                                                         gpointer           *data);
static void     bobgui_info_bar_buildable_custom_finished  (BobguiBuildable       *buildable,
                                                         BobguiBuilder         *builder,
                                                         GObject            *child,
                                                         const char         *tagname,
                                                         gpointer            user_data);
static void      bobgui_info_bar_buildable_add_child       (BobguiBuildable *buildable,
                                                         BobguiBuilder   *builder,
                                                         GObject      *child,
                                                         const char   *type);



G_DEFINE_TYPE_WITH_CODE (BobguiInfoBar, bobgui_info_bar, BOBGUI_TYPE_WIDGET,
                         G_IMPLEMENT_INTERFACE (BOBGUI_TYPE_BUILDABLE,
                                                bobgui_info_bar_buildable_interface_init))

static void
bobgui_info_bar_set_property (GObject      *object,
                           guint         prop_id,
                           const GValue *value,
                           GParamSpec   *pspec)
{
  BobguiInfoBar *info_bar = BOBGUI_INFO_BAR (object);

  switch (prop_id)
    {
    case PROP_MESSAGE_TYPE:
      bobgui_info_bar_set_message_type (info_bar, g_value_get_enum (value));
      break;
    case PROP_SHOW_CLOSE_BUTTON:
      bobgui_info_bar_set_show_close_button (info_bar, g_value_get_boolean (value));
      break;
    case PROP_REVEALED:
      bobgui_info_bar_set_revealed (info_bar, g_value_get_boolean (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bobgui_info_bar_get_property (GObject    *object,
                           guint       prop_id,
                           GValue     *value,
                           GParamSpec *pspec)
{
  BobguiInfoBar *info_bar = BOBGUI_INFO_BAR (object);

  switch (prop_id)
    {
    case PROP_MESSAGE_TYPE:
      g_value_set_enum (value, bobgui_info_bar_get_message_type (info_bar));
      break;
    case PROP_SHOW_CLOSE_BUTTON:
      g_value_set_boolean (value, bobgui_info_bar_get_show_close_button (info_bar));
      break;
    case PROP_REVEALED:
      g_value_set_boolean (value, bobgui_info_bar_get_revealed (info_bar));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
response_data_free (gpointer data)
{
  g_slice_free (ResponseData, data);
}

static ResponseData *
get_response_data (BobguiWidget *widget,
                   gboolean   create)
{
  ResponseData *ad = g_object_get_data (G_OBJECT (widget),
                                        "bobgui-info-bar-response-data");

  if (ad == NULL && create)
    {
      ad = g_slice_new (ResponseData);

      g_object_set_data_full (G_OBJECT (widget),
                              I_("bobgui-info-bar-response-data"),
                              ad,
                              response_data_free);
    }

  return ad;
}

static void
clear_response_data (BobguiWidget *widget)
{
  ResponseData *data;

  data = get_response_data (widget, FALSE);
  g_signal_handler_disconnect (widget, data->handler_id);
  g_object_set_data (G_OBJECT (widget), "bobgui-info-bar-response-data", NULL);
}

static BobguiWidget *
find_button (BobguiInfoBar *info_bar,
             int         response_id)
{
  BobguiWidget *child;

  for (child = bobgui_widget_get_first_child (info_bar->action_area);
       child != NULL;
       child = bobgui_widget_get_next_sibling (child))
    {
      ResponseData *rd = get_response_data (child, FALSE);

      if (rd && rd->response_id == response_id)
        return child;
    }

  return NULL;
}

static void
bobgui_info_bar_close (BobguiInfoBar *info_bar)
{
  if (!bobgui_widget_get_visible (info_bar->close_button) &&
      !find_button (info_bar, BOBGUI_RESPONSE_CANCEL))
    return;

  bobgui_info_bar_response (BOBGUI_INFO_BAR (info_bar),
                         BOBGUI_RESPONSE_CANCEL);
}

static void
bobgui_info_bar_dispose (GObject *object)
{
  BobguiInfoBar *self = BOBGUI_INFO_BAR (object);

  g_clear_pointer (&self->revealer, bobgui_widget_unparent);

  G_OBJECT_CLASS (bobgui_info_bar_parent_class)->dispose (object);
}

static void
bobgui_info_bar_class_init (BobguiInfoBarClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (klass);

  object_class->get_property = bobgui_info_bar_get_property;
  object_class->set_property = bobgui_info_bar_set_property;
  object_class->dispose = bobgui_info_bar_dispose;

  klass->close = bobgui_info_bar_close;

  /**
   * BobguiInfoBar:message-type:
   *
   * The type of the message.
   *
   * The type may be used to determine the appearance of the info bar.
   */
  props[PROP_MESSAGE_TYPE] =
    g_param_spec_enum ("message-type", NULL, NULL,
                       BOBGUI_TYPE_MESSAGE_TYPE,
                       BOBGUI_MESSAGE_INFO,
                       BOBGUI_PARAM_READWRITE|G_PARAM_CONSTRUCT|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiInfoBar:show-close-button:
   *
   * Whether to include a standard close button.
   */
  props[PROP_SHOW_CLOSE_BUTTON] =
    g_param_spec_boolean ("show-close-button", NULL, NULL,
                          FALSE,
                          BOBGUI_PARAM_READWRITE|G_PARAM_CONSTRUCT|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiInfoBar:revealed:
   *
   * Whether the info bar shows its contents.
   */
  props[PROP_REVEALED] =
    g_param_spec_boolean ("revealed", NULL, NULL,
                          TRUE,
                          BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, LAST_PROP, props);

  /**
   * BobguiInfoBar::response:
   * @info_bar: the object on which the signal is emitted
   * @response_id: the response ID
   *
   * Emitted when an action widget is clicked.
   *
   * The signal is also emitted when the application programmer
   * calls [method@Bobgui.InfoBar.response]. The @response_id depends
   * on which action widget was clicked.
   */
  signals[RESPONSE] = g_signal_new (I_("response"),
                                    G_OBJECT_CLASS_TYPE (klass),
                                    G_SIGNAL_RUN_LAST,
                                    G_STRUCT_OFFSET (BobguiInfoBarClass, response),
                                    NULL, NULL,
                                    NULL,
                                    G_TYPE_NONE, 1,
                                    G_TYPE_INT);

  /**
   * BobguiInfoBar::close:
   *
   * Gets emitted when the user uses a keybinding to dismiss the info bar.
   *
   * The ::close signal is a [keybinding signal](class.SignalAction.html).
   *
   * The default binding for this signal is the Escape key.
   */
  signals[CLOSE] =  g_signal_new (I_("close"),
                                  G_OBJECT_CLASS_TYPE (klass),
                                  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                                  G_STRUCT_OFFSET (BobguiInfoBarClass, close),
                                  NULL, NULL,
                                  NULL,
                                  G_TYPE_NONE, 0);

  bobgui_widget_class_add_binding_signal (widget_class,
                                       GDK_KEY_Escape, 0,
                                       "close",
                                       NULL);

  bobgui_widget_class_set_css_name (widget_class, I_("infobar"));
  bobgui_widget_class_set_layout_manager_type (widget_class, BOBGUI_TYPE_BIN_LAYOUT);
}

static void
close_button_clicked_cb (BobguiWidget  *button,
                         BobguiInfoBar *info_bar)
{
  bobgui_info_bar_response (BOBGUI_INFO_BAR (info_bar), BOBGUI_RESPONSE_CLOSE);
}

static void
click_released_cb (BobguiGestureClick *gesture,
                   guint            n_press,
                   double           x,
                   double           y,
                   BobguiInfoBar      *info_bar)
{
  if (info_bar->default_response && info_bar->default_response_sensitive)
    bobgui_info_bar_response (info_bar, info_bar->default_response);
}

static void
bobgui_info_bar_init (BobguiInfoBar *info_bar)
{
  BobguiWidget *widget = BOBGUI_WIDGET (info_bar);
  BobguiWidget *main_box;
  BobguiGesture *gesture;
  BobguiWidget *image;

  /* message-type is a CONSTRUCT property, so we init to a value
   * different from its default to trigger its property setter
   * during construction */
  info_bar->message_type = BOBGUI_MESSAGE_OTHER;

  info_bar->revealer = bobgui_revealer_new ();
  bobgui_revealer_set_reveal_child (BOBGUI_REVEALER (info_bar->revealer), TRUE);
  bobgui_widget_set_parent (info_bar->revealer, widget);

  main_box = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 0);
  bobgui_revealer_set_child (BOBGUI_REVEALER (info_bar->revealer), main_box);

  info_bar->content_area = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 0);
  bobgui_widget_set_hexpand (info_bar->content_area, TRUE);
  bobgui_box_append (BOBGUI_BOX (main_box), info_bar->content_area);

  info_bar->action_area = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 0);
  bobgui_widget_set_halign (info_bar->action_area, BOBGUI_ALIGN_END);
  bobgui_widget_set_valign (info_bar->action_area, BOBGUI_ALIGN_CENTER);
  bobgui_box_append (BOBGUI_BOX (main_box), info_bar->action_area);

  info_bar->close_button = bobgui_button_new ();
  /* The icon is not relevant for accessibility purposes */
  image = g_object_new (BOBGUI_TYPE_IMAGE,
                        "accessible-role", BOBGUI_ACCESSIBLE_ROLE_PRESENTATION,
                        "icon-name", "window-close-symbolic",
                        "use-fallback", TRUE,
                        NULL);
  bobgui_button_set_child (BOBGUI_BUTTON (info_bar->close_button), image);
  bobgui_widget_hide (info_bar->close_button);
  bobgui_widget_set_valign (info_bar->close_button, BOBGUI_ALIGN_CENTER);
  bobgui_widget_add_css_class (info_bar->close_button, "close");
  bobgui_box_append (BOBGUI_BOX (main_box), info_bar->close_button);
  bobgui_accessible_update_property (BOBGUI_ACCESSIBLE (info_bar->close_button),
                                  BOBGUI_ACCESSIBLE_PROPERTY_LABEL, _("Close"),
                                  BOBGUI_ACCESSIBLE_PROPERTY_DESCRIPTION, _("Close the infobar"),
                                  -1);

  g_signal_connect (info_bar->close_button, "clicked",
                    G_CALLBACK (close_button_clicked_cb), info_bar);

  gesture = bobgui_gesture_click_new ();
  bobgui_gesture_single_set_button (BOBGUI_GESTURE_SINGLE (gesture), GDK_BUTTON_PRIMARY);
  g_signal_connect (gesture, "released", G_CALLBACK (click_released_cb), widget);
  bobgui_widget_add_controller (widget, BOBGUI_EVENT_CONTROLLER (gesture));
}

static BobguiBuildableIface *parent_buildable_iface;

static void
bobgui_info_bar_buildable_interface_init (BobguiBuildableIface *iface)
{
  parent_buildable_iface = g_type_interface_peek_parent (iface);

  iface->add_child = bobgui_info_bar_buildable_add_child;
  iface->custom_tag_start = bobgui_info_bar_buildable_custom_tag_start;
  iface->custom_finished = bobgui_info_bar_buildable_custom_finished;
}

static int
get_response_for_widget (BobguiInfoBar *info_bar,
                         BobguiWidget  *widget)
{
  ResponseData *rd;

  rd = get_response_data (widget, FALSE);
  if (!rd)
    return BOBGUI_RESPONSE_NONE;
  else
    return rd->response_id;
}

static void
action_widget_activated (BobguiWidget  *widget,
                         BobguiInfoBar *info_bar)
{
  int response_id;

  response_id = get_response_for_widget (info_bar, widget);
  bobgui_info_bar_response (info_bar, response_id);
}

/**
 * bobgui_info_bar_add_action_widget:
 * @info_bar: a `BobguiInfoBar`
 * @child: an activatable widget
 * @response_id: response ID for @child
 *
 * Add an activatable widget to the action area of a `BobguiInfoBar`.
 *
 * This also connects a signal handler that will emit the
 * [signal@Bobgui.InfoBar::response] signal on the message area
 * when the widget is activated. The widget is appended to the
 * end of the message areas action area.
 *
 * Deprecated: 4.10
 */
void
bobgui_info_bar_add_action_widget (BobguiInfoBar *info_bar,
                                BobguiWidget  *child,
                                int         response_id)
{
  ResponseData *ad;
  guint signal_id;

  g_return_if_fail (BOBGUI_IS_INFO_BAR (info_bar));
  g_return_if_fail (BOBGUI_IS_WIDGET (child));

  ad = get_response_data (child, TRUE);

  G_DEBUG_HERE();
  ad->response_id = response_id;

  if (BOBGUI_IS_BUTTON (child))
    signal_id = g_signal_lookup ("clicked", BOBGUI_TYPE_BUTTON);
  else
    signal_id = bobgui_widget_class_get_activate_signal (BOBGUI_WIDGET_GET_CLASS (child));

  if (signal_id)
    {
      GClosure *closure;

      closure = g_cclosure_new_object (G_CALLBACK (action_widget_activated),
                                       G_OBJECT (info_bar));
      ad->handler_id = g_signal_connect_closure_by_id (child, signal_id, 0, closure, FALSE);
    }
  else
    g_warning ("Only 'activatable' widgets can be packed into the action area of a BobguiInfoBar");

  bobgui_box_append (BOBGUI_BOX (info_bar->action_area), child);
}

/**
 * bobgui_info_bar_remove_action_widget:
 * @info_bar: a `BobguiInfoBar`
 * @widget: an action widget to remove
 *
 * Removes a widget from the action area of @info_bar.
 *
 * The widget must have been put there by a call to
 * [method@Bobgui.InfoBar.add_action_widget] or [method@Bobgui.InfoBar.add_button].
 *
 * Deprecated: 4.10
 */
void
bobgui_info_bar_remove_action_widget (BobguiInfoBar *info_bar,
                                   BobguiWidget  *widget)
{
  g_return_if_fail (BOBGUI_IS_INFO_BAR (info_bar));
  g_return_if_fail (BOBGUI_IS_WIDGET (widget));
  g_return_if_fail (bobgui_widget_get_parent (widget) == info_bar->action_area);

  clear_response_data (widget);

  bobgui_box_remove (BOBGUI_BOX (info_bar->action_area), widget);
}

/**
 * bobgui_info_bar_add_button:
 * @info_bar: a `BobguiInfoBar`
 * @button_text: text of button
 * @response_id: response ID for the button
 *
 * Adds a button with the given text.
 *
 * Clicking the button will emit the [signal@Bobgui.InfoBar::response]
 * signal with the given response_id. The button is appended to the
 * end of the info bar's action area. The button widget is returned,
 * but usually you don't need it.
 *
 * Returns: (transfer none) (type Bobgui.Button): the `BobguiButton` widget
 * that was added
 *
 * Deprecated: 4.10
 */
BobguiWidget*
bobgui_info_bar_add_button (BobguiInfoBar  *info_bar,
                         const char *button_text,
                         int          response_id)
{
  BobguiWidget *button;

  g_return_val_if_fail (BOBGUI_IS_INFO_BAR (info_bar), NULL);
  g_return_val_if_fail (button_text != NULL, NULL);

  button = bobgui_button_new_with_label (button_text);
  bobgui_button_set_use_underline (BOBGUI_BUTTON (button), TRUE);

  bobgui_widget_show (button);

  bobgui_info_bar_add_action_widget (info_bar, button, response_id);

  return button;
}

static void
add_buttons_valist (BobguiInfoBar  *info_bar,
                    const char *first_button_text,
                    va_list      args)
{
  const char * text;
  int response_id;

  g_return_if_fail (BOBGUI_IS_INFO_BAR (info_bar));

  if (first_button_text == NULL)
    return;

  text = first_button_text;
  response_id = va_arg (args, int);

  while (text != NULL)
    {
      bobgui_info_bar_add_button (info_bar, text, response_id);

      text = va_arg (args, char *);
      if (text == NULL)
        break;

      response_id = va_arg (args, int);
    }
}

/**
 * bobgui_info_bar_add_buttons:
 * @info_bar: a `BobguiInfoBar`
 * @first_button_text: button text
 * @...: response ID for first button, then more text-response_id pairs,
 *   ending with %NULL
 *
 * Adds multiple buttons.
 *
 * This is the same as calling [method@Bobgui.InfoBar.add_button]
 * repeatedly. The variable argument list should be %NULL-terminated
 * as with [ctor@Bobgui.InfoBar.new_with_buttons]. Each button must have both
 * text and response ID.
 *
 * Deprecated: 4.10
 */
void
bobgui_info_bar_add_buttons (BobguiInfoBar  *info_bar,
                          const char *first_button_text,
                          ...)
{
  va_list args;

  va_start (args, first_button_text);
  add_buttons_valist (info_bar, first_button_text, args);
  va_end (args);
}

/**
 * bobgui_info_bar_new:
 *
 * Creates a new `BobguiInfoBar` object.
 *
 * Returns: a new `BobguiInfoBar` object
 *
 * Deprecated: 4.10
 */
BobguiWidget *
bobgui_info_bar_new (void)
{
   return g_object_new (BOBGUI_TYPE_INFO_BAR, NULL);
}

/**
 * bobgui_info_bar_new_with_buttons:
 * @first_button_text: (nullable): ext to go in first button
 * @...: response ID for first button, then additional buttons, ending
 *    with %NULL
 *
 * Creates a new `BobguiInfoBar` with buttons.
 *
 * Button text/response ID pairs should be listed, with a %NULL pointer
 * ending the list. A response ID can be any positive number,
 * or one of the values in the `BobguiResponseType` enumeration. If the
 * user clicks one of these dialog buttons, BobguiInfoBar will emit
 * the [signal@Bobgui.InfoBar::response] signal with the corresponding
 * response ID.
 *
 * Returns: a new `BobguiInfoBar`
 *
 * Deprecated: 4.10
 */
BobguiWidget*
bobgui_info_bar_new_with_buttons (const char *first_button_text,
                               ...)
{
  BobguiInfoBar *info_bar;
  va_list args;

  info_bar = BOBGUI_INFO_BAR (bobgui_info_bar_new ());

  va_start (args, first_button_text);
  add_buttons_valist (info_bar, first_button_text, args);
  va_end (args);

  return BOBGUI_WIDGET (info_bar);
}

static void
update_default_response (BobguiInfoBar *info_bar,
                         int         response_id,
                         gboolean    sensitive)
{
  info_bar->default_response = response_id;
  info_bar->default_response_sensitive = sensitive;

  if (response_id && sensitive)
    bobgui_widget_add_css_class (BOBGUI_WIDGET (info_bar), "action");
  else
    bobgui_widget_remove_css_class (BOBGUI_WIDGET (info_bar), "action");
}

/**
 * bobgui_info_bar_set_response_sensitive:
 * @info_bar: a `BobguiInfoBar`
 * @response_id: a response ID
 * @setting: TRUE for sensitive
 *
 * Sets the sensitivity of action widgets for @response_id.
 *
 * Calls `bobgui_widget_set_sensitive (widget, setting)` for each
 * widget in the info bars’s action area with the given @response_id.
 * A convenient way to sensitize/desensitize buttons.
 *
 * Deprecated: 4.10
 */
void
bobgui_info_bar_set_response_sensitive (BobguiInfoBar *info_bar,
                                     int         response_id,
                                     gboolean    setting)
{
  BobguiWidget *child;

  g_return_if_fail (BOBGUI_IS_INFO_BAR (info_bar));

  for (child = bobgui_widget_get_first_child (info_bar->action_area);
       child != NULL;
       child = bobgui_widget_get_next_sibling (child))
    {
      ResponseData *rd = get_response_data (child, FALSE);

      if (rd && rd->response_id == response_id)
        bobgui_widget_set_sensitive (child, setting);
    }

  if (response_id == info_bar->default_response)
    update_default_response (info_bar, response_id, setting);
}

/**
 * bobgui_info_bar_set_default_response:
 * @info_bar: a `BobguiInfoBar`
 * @response_id: a response ID
 *
 * Sets the last widget in the info bar’s action area with
 * the given response_id as the default widget for the dialog.
 *
 * Pressing “Enter” normally activates the default widget.
 *
 * Note that this function currently requires @info_bar to
 * be added to a widget hierarchy.
 *
 * Deprecated: 4.10
 */
void
bobgui_info_bar_set_default_response (BobguiInfoBar *info_bar,
                                   int         response_id)
{
  BobguiWidget *child;
  BobguiWidget *window;
  gboolean sensitive = TRUE;

  g_return_if_fail (BOBGUI_IS_INFO_BAR (info_bar));

  window = bobgui_widget_get_ancestor (BOBGUI_WIDGET (info_bar), BOBGUI_TYPE_WINDOW);

  for (child = bobgui_widget_get_first_child (info_bar->action_area);
       child != NULL;
       child = bobgui_widget_get_next_sibling (child))
    {
      ResponseData *rd = get_response_data (child, FALSE);

      if (rd && rd->response_id == response_id)
        {
          bobgui_window_set_default_widget (BOBGUI_WINDOW (window), child);
          sensitive = bobgui_widget_get_sensitive (child);
          break;
        }
    }

  update_default_response (info_bar, response_id, sensitive);
}

/**
 * bobgui_info_bar_response:
 * @info_bar: a `BobguiInfoBar`
 * @response_id: a response ID
 *
 * Emits the “response” signal with the given @response_id.
 *
 * Deprecated: 4.10
 */
void
bobgui_info_bar_response (BobguiInfoBar *info_bar,
                       int         response_id)
{
  g_return_if_fail (BOBGUI_IS_INFO_BAR (info_bar));

  g_signal_emit (info_bar, signals[RESPONSE], 0, response_id);
}

typedef struct
{
  char *name;
  int response_id;
  int line;
  int col;
} ActionWidgetInfo;

typedef struct
{
  BobguiInfoBar *info_bar;
  BobguiBuilder *builder;
  GSList *items;
  int response_id;
  gboolean is_text;
  GString *string;
  int line;
  int col;
} SubParserData;

static void
action_widget_info_free (gpointer data)
{
  ActionWidgetInfo *item = data;

  g_free (item->name);
  g_free (item);
}

static void
parser_start_element (BobguiBuildableParseContext  *context,
                      const char                *element_name,
                      const char               **names,
                      const char               **values,
                      gpointer                   user_data,
                      GError                   **error)
{
  SubParserData *data = (SubParserData*)user_data;

  if (strcmp (element_name, "action-widget") == 0)
    {
      const char *response;
      GValue gvalue = G_VALUE_INIT;

      if (!_bobgui_builder_check_parent (data->builder, context, "action-widgets", error))
        return;

      if (!g_markup_collect_attributes (element_name, names, values, error,
                                        G_MARKUP_COLLECT_STRING, "response", &response,
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
    }
  else
    {
      _bobgui_builder_error_unhandled_tag (data->builder, context,
                                        "BobguiInfoBar", element_name,
                                        error);
    }
}

static void
parser_text_element (BobguiBuildableParseContext  *context,
                     const char                *text,
                     gsize                      text_len,
                     gpointer                   user_data,
                     GError                   **error)
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
      item->name = g_strdup (data->string->str);
      item->response_id = data->response_id;
      item->line = data->line;
      item->col = data->col;

      data->items = g_slist_prepend (data->items, item);
      data->is_text = FALSE;
    }
}

static const BobguiBuildableParser sub_parser =
{
  parser_start_element,
  parser_end_element,
  parser_text_element,
};

gboolean
bobgui_info_bar_buildable_custom_tag_start (BobguiBuildable       *buildable,
                                         BobguiBuilder         *builder,
                                         GObject            *child,
                                         const char         *tagname,
                                         BobguiBuildableParser *parser,
                                         gpointer           *parser_data)
{
  SubParserData *data;

  if (parent_buildable_iface->custom_tag_start (buildable, builder, child,
                                                tagname, parser, parser_data))
    return TRUE;

  if (!child && strcmp (tagname, "action-widgets") == 0)
    {
      data = g_slice_new0 (SubParserData);
      data->info_bar = BOBGUI_INFO_BAR (buildable);
      data->builder = builder;
      data->string = g_string_new ("");
      data->items = NULL;

      *parser = sub_parser;
      *parser_data = data;
      return TRUE;
    }

  return FALSE;
}

static void
bobgui_info_bar_buildable_custom_finished (BobguiBuildable *buildable,
                                        BobguiBuilder   *builder,
                                        GObject      *child,
                                        const char   *tagname,
                                        gpointer      user_data)
{
  BobguiInfoBar *info_bar = BOBGUI_INFO_BAR (buildable);
  GSList *l;
  SubParserData *data;
  GObject *object;
  ResponseData *ad;
  guint signal_id;

  if (strcmp (tagname, "action-widgets"))
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

      object = _bobgui_builder_lookup_object (builder, item->name, item->line, item->col);
      if (!object)
        continue;

      ad = get_response_data (BOBGUI_WIDGET (object), TRUE);
      ad->response_id = item->response_id;

      if (BOBGUI_IS_BUTTON (object))
        signal_id = g_signal_lookup ("clicked", BOBGUI_TYPE_BUTTON);
      else
        signal_id = bobgui_widget_class_get_activate_signal (BOBGUI_WIDGET_GET_CLASS (object));

      if (signal_id)
        {
          GClosure *closure;

          closure = g_cclosure_new_object (G_CALLBACK (action_widget_activated),
                                           G_OBJECT (info_bar));
          g_signal_connect_closure_by_id (object, signal_id, 0, closure, FALSE);
        }
    }

  g_slist_free_full (data->items, action_widget_info_free);
  g_string_free (data->string, TRUE);
  g_slice_free (SubParserData, data);
}

static void
bobgui_info_bar_buildable_add_child (BobguiBuildable *buildable,
                                  BobguiBuilder   *builder,
                                  GObject      *child,
                                  const char   *type)
{
  BobguiInfoBar *info_bar = BOBGUI_INFO_BAR (buildable);

  if (!type && BOBGUI_IS_WIDGET (child))
    bobgui_info_bar_add_child (BOBGUI_INFO_BAR (info_bar), BOBGUI_WIDGET (child));
  else if (g_strcmp0 (type, "action") == 0)
    bobgui_box_append (BOBGUI_BOX (info_bar->action_area), BOBGUI_WIDGET (child));
  else
    parent_buildable_iface->add_child (buildable, builder, child, type);
}

/**
 * bobgui_info_bar_set_message_type:
 * @info_bar: a `BobguiInfoBar`
 * @message_type: a `BobguiMessageType`
 *
 * Sets the message type of the message area.
 *
 * BOBGUI uses this type to determine how the message is displayed.
 *
 * Deprecated: 4.10
 */
void
bobgui_info_bar_set_message_type (BobguiInfoBar     *info_bar,
                               BobguiMessageType  message_type)
{
  g_return_if_fail (BOBGUI_IS_INFO_BAR (info_bar));

  if (info_bar->message_type != message_type)
    {
      const char *type_class[] = {
        "info",
        "warning",
        "question",
        "error",
        NULL
      };

      if (type_class[info_bar->message_type])
        bobgui_widget_remove_css_class (BOBGUI_WIDGET (info_bar), type_class[info_bar->message_type]);

      info_bar->message_type = message_type;

      bobgui_widget_queue_draw (BOBGUI_WIDGET (info_bar));

      if (type_class[info_bar->message_type])
        bobgui_widget_add_css_class (BOBGUI_WIDGET (info_bar), type_class[info_bar->message_type]);

      g_object_notify_by_pspec (G_OBJECT (info_bar), props[PROP_MESSAGE_TYPE]);
    }
}

/**
 * bobgui_info_bar_get_message_type:
 * @info_bar: a `BobguiInfoBar`
 *
 * Returns the message type of the message area.
 *
 * Returns: the message type of the message area.
 *
 * Deprecated: 4.10
 */
BobguiMessageType
bobgui_info_bar_get_message_type (BobguiInfoBar *info_bar)
{
  g_return_val_if_fail (BOBGUI_IS_INFO_BAR (info_bar), BOBGUI_MESSAGE_OTHER);

  return info_bar->message_type;
}


/**
 * bobgui_info_bar_set_show_close_button:
 * @info_bar: a `BobguiInfoBar`
 * @setting: %TRUE to include a close button
 *
 * If true, a standard close button is shown.
 *
 * When clicked it emits the response %BOBGUI_RESPONSE_CLOSE.
 *
 * Deprecated: 4.10
 */
void
bobgui_info_bar_set_show_close_button (BobguiInfoBar *info_bar,
                                    gboolean    setting)
{
  g_return_if_fail (BOBGUI_IS_INFO_BAR (info_bar));

  if (setting == bobgui_info_bar_get_show_close_button (info_bar))
    return;

  bobgui_widget_set_visible (info_bar->close_button, setting);
  g_object_notify_by_pspec (G_OBJECT (info_bar), props[PROP_SHOW_CLOSE_BUTTON]);
}

/**
 * bobgui_info_bar_get_show_close_button:
 * @info_bar: a `BobguiInfoBar`
 *
 * Returns whether the widget will display a standard close button.
 *
 * Returns: %TRUE if the widget displays standard close button
 *
 * Deprecated: 4.10
 */
gboolean
bobgui_info_bar_get_show_close_button (BobguiInfoBar *info_bar)
{
  g_return_val_if_fail (BOBGUI_IS_INFO_BAR (info_bar), FALSE);

  return bobgui_widget_get_visible (info_bar->close_button);
}

/**
 * bobgui_info_bar_set_revealed:
 * @info_bar: a `BobguiInfoBar`
 * @revealed: The new value of the property
 *
 * Sets whether the `BobguiInfoBar` is revealed.
 *
 * Changing this will make @info_bar reveal or conceal
 * itself via a sliding transition.
 *
 * Note: this does not show or hide @info_bar in the
 * [property@Bobgui.Widget:visible] sense, so revealing has no effect
 * if [property@Bobgui.Widget:visible] is %FALSE.
 *
 * Deprecated: 4.10
 */
void
bobgui_info_bar_set_revealed (BobguiInfoBar *info_bar,
                           gboolean    revealed)
{
  g_return_if_fail (BOBGUI_IS_INFO_BAR (info_bar));

  if (revealed == bobgui_revealer_get_reveal_child (BOBGUI_REVEALER (info_bar->revealer)))
    return;

  bobgui_revealer_set_reveal_child (BOBGUI_REVEALER (info_bar->revealer), revealed);
  g_object_notify_by_pspec (G_OBJECT (info_bar), props[PROP_REVEALED]);
}

/**
 * bobgui_info_bar_get_revealed:
 * @info_bar: a `BobguiInfoBar`
 *
 * Returns whether the info bar is currently revealed.
 *
 * Returns: the current value of the [property@Bobgui.InfoBar:revealed] property
 *
 * Deprecated: 4.10
 */
gboolean
bobgui_info_bar_get_revealed (BobguiInfoBar *info_bar)
{
  g_return_val_if_fail (BOBGUI_IS_INFO_BAR (info_bar), FALSE);

  return bobgui_revealer_get_reveal_child (BOBGUI_REVEALER (info_bar->revealer));
}

/**
 * bobgui_info_bar_add_child:
 * @info_bar: a `BobguiInfoBar`
 * @widget: the child to be added
 *
 * Adds a widget to the content area of the info bar.
 *
 * Deprecated: 4.10
 */
void
bobgui_info_bar_add_child (BobguiInfoBar *info_bar,
                        BobguiWidget  *widget)
{
  g_return_if_fail (BOBGUI_IS_INFO_BAR (info_bar));
  g_return_if_fail (BOBGUI_IS_WIDGET (widget));

  bobgui_box_append (BOBGUI_BOX (info_bar->content_area), widget);
}

/**
 * bobgui_info_bar_remove_child:
 * @info_bar: a `BobguiInfoBar`
 * @widget: a child that has been added to the content area
 *
 * Removes a widget from the content area of the info bar.
 *
 * Deprecated: 4.10
 */
void
bobgui_info_bar_remove_child (BobguiInfoBar *info_bar,
                           BobguiWidget  *widget)
{
  g_return_if_fail (BOBGUI_IS_INFO_BAR (info_bar));
  g_return_if_fail (BOBGUI_IS_WIDGET (widget));

  bobgui_box_remove (BOBGUI_BOX (info_bar->content_area), widget);
}
