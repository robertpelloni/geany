/* BOBGUI - The Bobgui Framework
 * bobguilinkbutton.c - a hyperlink-enabled button
 *
 * Copyright (C) 2006 Emmanuele Bassi <ebassi@gmail.com>
 * All rights reserved.
 *
 * Based on gnome-href code by:
 *      James Henstridge <james@daa.com.au>
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

/**
 * BobguiLinkButton:
 *
 * A button with a hyperlink.
 *
 * <picture>
 *   <source srcset="link-button-dark.png" media="(prefers-color-scheme: dark)">
 *   <img alt="An example BobguiLinkButton" src="link-button.png">
 * </picture>
 *
 * It is useful to show quick links to resources.
 *
 * A link button is created by calling either [ctor@Bobgui.LinkButton.new] or
 * [ctor@Bobgui.LinkButton.new_with_label]. If using the former, the URI you
 * pass to the constructor is used as a label for the widget.
 *
 * The URI bound to a `BobguiLinkButton` can be set specifically using
 * [method@Bobgui.LinkButton.set_uri].
 *
 * By default, `BobguiLinkButton` calls [method@Bobgui.FileLauncher.launch] when the button
 * is clicked. This behaviour can be overridden by connecting to the
 * [signal@Bobgui.LinkButton::activate-link] signal and returning %TRUE from
 * the signal handler.
 *
 * # Shortcuts and Gestures
 *
 * `BobguiLinkButton` supports the following keyboard shortcuts:
 *
 * - <kbd>Shift</kbd>+<kbd>F10</kbd> or <kbd>Menu</kbd> opens the context menu.
 *
 * # Actions
 *
 * `BobguiLinkButton` defines a set of built-in actions:
 *
 * - `clipboard.copy` copies the url to the clipboard.
 * - `menu.popup` opens the context menu.
 *
 * # CSS nodes
 *
 * `BobguiLinkButton` has a single CSS node with name button. To differentiate
 * it from a plain `BobguiButton`, it gets the .link style class.
 *
 * # Accessibility
 *
 * `BobguiLinkButton` uses the [enum@Bobgui.AccessibleRole.link] role.
 */

#include "config.h"

#include "bobguilinkbutton.h"

#include "bobguidragsource.h"
#include "bobguifilelauncher.h"
#include "bobguigestureclick.h"
#include "bobguigesturesingle.h"
#include "bobguilabel.h"
#include "bobguimain.h"
#include "bobguimarshalers.h"
#include "bobguipopovermenu.h"
#include "bobguiprivate.h"
#include "bobguisizerequest.h"
#include "bobguitooltip.h"
#include "bobguiurilauncher.h"
#include "bobguiwidgetprivate.h"

#include <string.h>
#include <glib/gi18n-lib.h>

typedef struct _BobguiLinkButtonClass BobguiLinkButtonClass;

struct _BobguiLinkButton
{
  /*< private >*/
  BobguiButton parent_instance;

  char *uri;
  gboolean visited;
  BobguiWidget *popup_menu;
};

struct _BobguiLinkButtonClass
{
  /*< private >*/
  BobguiButtonClass parent_class;

  /*< public >*/
  gboolean (* activate_link) (BobguiLinkButton *button);
};

enum
{
  PROP_0,
  PROP_URI,
  PROP_VISITED
};

enum
{
  ACTIVATE_LINK,

  LAST_SIGNAL
};

static void     bobgui_link_button_finalize     (GObject          *object);
static void     bobgui_link_button_get_property (GObject          *object,
					      guint             prop_id,
					      GValue           *value,
					      GParamSpec       *pspec);
static void     bobgui_link_button_set_property (GObject          *object,
					      guint             prop_id,
					      const GValue     *value,
					      GParamSpec       *pspec);
static void     bobgui_link_button_clicked      (BobguiButton        *button);
static void     bobgui_link_button_popup_menu   (BobguiWidget        *widget,
                                              const char       *action_name,
                                              GVariant         *parameters);
static gboolean bobgui_link_button_query_tooltip_cb (BobguiWidget    *widget,
                                                  int           x,
                                                  int           y,
                                                  gboolean      keyboard_tip,
                                                  BobguiTooltip   *tooltip,
                                                  gpointer      data);
static void bobgui_link_button_pressed_cb (BobguiGestureClick *gesture,
                                        int                   n_press,
                                        double                x,
                                        double                y,
                                        gpointer              user_data);

static gboolean bobgui_link_button_activate_link (BobguiLinkButton *link_button);

static const char *link_drop_types[] = {
  "text/uri-list",
  "_NETSCAPE_URL"
};

static guint link_signals[LAST_SIGNAL] = { 0, };

G_DEFINE_TYPE (BobguiLinkButton, bobgui_link_button, BOBGUI_TYPE_BUTTON)

static void
bobgui_link_button_activate_clipboard_copy (BobguiWidget  *widget,
                                         const char *name,
                                         GVariant   *parameter)
{
  BobguiLinkButton *link_button = BOBGUI_LINK_BUTTON (widget);

  gdk_clipboard_set_text (bobgui_widget_get_clipboard (widget), link_button->uri);
}

static void
bobgui_link_button_class_init (BobguiLinkButtonClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (klass);
  BobguiButtonClass *button_class = BOBGUI_BUTTON_CLASS (klass);

  gobject_class->set_property = bobgui_link_button_set_property;
  gobject_class->get_property = bobgui_link_button_get_property;
  gobject_class->finalize = bobgui_link_button_finalize;

  button_class->clicked = bobgui_link_button_clicked;

  klass->activate_link = bobgui_link_button_activate_link;

  /**
   * BobguiLinkButton:uri:
   *
   * The URI bound to this button.
   */
  g_object_class_install_property (gobject_class,
                                   PROP_URI,
                                   g_param_spec_string ("uri", NULL, NULL,
                                                        NULL,
                                                        BOBGUI_PARAM_READWRITE));

  /**
   * BobguiLinkButton:visited:
   *
   * The 'visited' state of this button.
   *
   * A visited link is drawn in a different color.
   */
  g_object_class_install_property (gobject_class,
                                   PROP_VISITED,
                                   g_param_spec_boolean ("visited", NULL, NULL,
                                                         FALSE,
                                                         BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

  /**
   * BobguiLinkButton::activate-link:
   * @button: the `BobguiLinkButton` that emitted the signal
   *
   * Emitted each time the `BobguiLinkButton` is clicked.
   *
   * The default handler will call [method@Bobgui.FileLauncher.launch] with the URI
   * stored inside the [property@Bobgui.LinkButton:uri] property.
   *
   * To override the default behavior, you can connect to the
   * ::activate-link signal and stop the propagation of the signal
   * by returning %TRUE from your handler.
   *
   * Returns: %TRUE if the signal has been handled
   */
  link_signals[ACTIVATE_LINK] =
    g_signal_new (I_("activate-link"),
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (BobguiLinkButtonClass, activate_link),
                  _bobgui_boolean_handled_accumulator, NULL,
                  _bobgui_marshal_BOOLEAN__VOID,
                  G_TYPE_BOOLEAN, 0);
  g_signal_set_va_marshaller (link_signals[ACTIVATE_LINK],
                              G_TYPE_FROM_CLASS (klass),
                              _bobgui_marshal_BOOLEAN__VOIDv);


  bobgui_widget_class_set_css_name (widget_class, I_("button"));
  bobgui_widget_class_set_accessible_role (widget_class, BOBGUI_ACCESSIBLE_ROLE_LINK);

  /**
   * BobguiLinkButton|clipboard.copy:
   *
   * Copies the url to the clipboard.
   */
  bobgui_widget_class_install_action (widget_class, "clipboard.copy", NULL,
                                   bobgui_link_button_activate_clipboard_copy);

  /**
   * BobguiLinkButton|menu.popup:
   *
   * Opens the context menu.
   */
  bobgui_widget_class_install_action (widget_class, "menu.popup", NULL, bobgui_link_button_popup_menu);

  bobgui_widget_class_add_binding_action (widget_class,
                                       GDK_KEY_F10, GDK_SHIFT_MASK,
                                       "menu.popup",
                                       NULL);
  bobgui_widget_class_add_binding_action (widget_class,
                                       GDK_KEY_Menu, 0,
                                       "menu.popup",
                                       NULL);
}

static GMenuModel *
bobgui_link_button_get_menu_model (void)
{
  GMenu *menu, *section;

  menu = g_menu_new ();

  section = g_menu_new ();
  g_menu_append (section, _("_Copy URL"), "clipboard.copy");
  g_menu_append_section (menu, NULL, G_MENU_MODEL (section));
  g_object_unref (section);

  return G_MENU_MODEL (menu);
}

#define BOBGUI_TYPE_LINK_CONTENT            (bobgui_link_content_get_type ())
#define BOBGUI_LINK_CONTENT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_LINK_CONTENT, BobguiLinkContent))

typedef struct _BobguiLinkContent BobguiLinkContent;
typedef struct _BobguiLinkContentClass BobguiLinkContentClass;

struct _BobguiLinkContent
{
  GdkContentProvider parent;
  BobguiLinkButton *link;
};

struct _BobguiLinkContentClass
{
  GdkContentProviderClass parent_class;
};

GType bobgui_link_content_get_type (void) G_GNUC_CONST;

G_DEFINE_TYPE (BobguiLinkContent, bobgui_link_content, GDK_TYPE_CONTENT_PROVIDER)

static GdkContentFormats *
bobgui_link_content_ref_formats (GdkContentProvider *provider)
{
  BobguiLinkContent *content = BOBGUI_LINK_CONTENT (provider);

  if (content->link)
    return gdk_content_formats_union (gdk_content_formats_new_for_gtype (G_TYPE_STRING),
                                      gdk_content_formats_new (link_drop_types, G_N_ELEMENTS (link_drop_types)));
  else
    return gdk_content_formats_new (NULL, 0);
}

static gboolean
bobgui_link_content_get_value (GdkContentProvider  *provider,
                             GValue              *value,
                             GError             **error)
{
  BobguiLinkContent *content = BOBGUI_LINK_CONTENT (provider);

  if (G_VALUE_HOLDS (value, G_TYPE_STRING) &&
      content->link != NULL)
    {
      char *uri;

      uri = g_strdup_printf ("%s\r\n", content->link->uri);
      g_value_set_string (value, uri);
      g_free (uri);

      return TRUE;
    }

  return GDK_CONTENT_PROVIDER_CLASS (bobgui_link_content_parent_class)->get_value (provider, value, error);
}

static void
bobgui_link_content_class_init (BobguiLinkContentClass *class)
{
  GdkContentProviderClass *provider_class = GDK_CONTENT_PROVIDER_CLASS (class);

  provider_class->ref_formats = bobgui_link_content_ref_formats;
  provider_class->get_value = bobgui_link_content_get_value;
}

static void
bobgui_link_content_init (BobguiLinkContent *content)
{
}

static void
bobgui_link_button_init (BobguiLinkButton *link_button)
{
  BobguiGesture *gesture;
  GdkContentProvider *content;
  BobguiDragSource *source;

  bobgui_button_set_has_frame (BOBGUI_BUTTON (link_button), FALSE);
  bobgui_widget_set_state_flags (BOBGUI_WIDGET (link_button), BOBGUI_STATE_FLAG_LINK, FALSE);
  bobgui_widget_set_has_tooltip (BOBGUI_WIDGET (link_button), TRUE);

  g_signal_connect (link_button, "query-tooltip",
                    G_CALLBACK (bobgui_link_button_query_tooltip_cb), NULL);

  source = bobgui_drag_source_new ();
  content = g_object_new (BOBGUI_TYPE_LINK_CONTENT, NULL);
  BOBGUI_LINK_CONTENT (content)->link = link_button;
  bobgui_drag_source_set_content (source, content);
  g_object_unref (content);
  bobgui_widget_add_controller (BOBGUI_WIDGET (link_button), BOBGUI_EVENT_CONTROLLER (source));

  gesture = bobgui_gesture_click_new ();
  bobgui_gesture_single_set_touch_only (BOBGUI_GESTURE_SINGLE (gesture), FALSE);
  bobgui_gesture_single_set_button (BOBGUI_GESTURE_SINGLE (gesture), 0);
  bobgui_event_controller_set_propagation_phase (BOBGUI_EVENT_CONTROLLER (gesture), BOBGUI_PHASE_BUBBLE);
  g_signal_connect (gesture, "pressed", G_CALLBACK (bobgui_link_button_pressed_cb),
                    link_button);
  bobgui_widget_add_controller (BOBGUI_WIDGET (link_button), BOBGUI_EVENT_CONTROLLER (gesture));

  bobgui_widget_add_css_class (BOBGUI_WIDGET (link_button), "link");

  bobgui_widget_set_cursor_from_name (BOBGUI_WIDGET (link_button), "pointer");
}

static void
bobgui_link_button_finalize (GObject *object)
{
  BobguiLinkButton *link_button = BOBGUI_LINK_BUTTON (object);

  g_free (link_button->uri);

  g_clear_pointer (&link_button->popup_menu, bobgui_widget_unparent);

  G_OBJECT_CLASS (bobgui_link_button_parent_class)->finalize (object);
}

static void
bobgui_link_button_get_property (GObject    *object,
			      guint       prop_id,
			      GValue     *value,
			      GParamSpec *pspec)
{
  BobguiLinkButton *link_button = BOBGUI_LINK_BUTTON (object);

  switch (prop_id)
    {
    case PROP_URI:
      g_value_set_string (value, link_button->uri);
      break;
    case PROP_VISITED:
      g_value_set_boolean (value, link_button->visited);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bobgui_link_button_set_property (GObject      *object,
			      guint         prop_id,
			      const GValue *value,
			      GParamSpec   *pspec)
{
  BobguiLinkButton *link_button = BOBGUI_LINK_BUTTON (object);

  switch (prop_id)
    {
    case PROP_URI:
      bobgui_link_button_set_uri (link_button, g_value_get_string (value));
      break;
    case PROP_VISITED:
      bobgui_link_button_set_visited (link_button, g_value_get_boolean (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bobgui_link_button_do_popup (BobguiLinkButton *link_button,
                          double         x,
                          double         y)
{
  if (!link_button->popup_menu)
    {
      GMenuModel *model;

      model = bobgui_link_button_get_menu_model ();
      link_button->popup_menu = bobgui_popover_menu_new_from_model (model);
      bobgui_widget_set_parent (link_button->popup_menu, BOBGUI_WIDGET (link_button));
      bobgui_popover_set_position (BOBGUI_POPOVER (link_button->popup_menu), BOBGUI_POS_BOTTOM);

      bobgui_popover_set_has_arrow (BOBGUI_POPOVER (link_button->popup_menu), FALSE);
      bobgui_widget_set_halign (link_button->popup_menu, BOBGUI_ALIGN_START);

      g_object_unref (model);
    }

  if (x != -1 && y != -1)
    {
      GdkRectangle rect = { x, y, 1, 1 };
      bobgui_popover_set_pointing_to (BOBGUI_POPOVER (link_button->popup_menu), &rect);
    }
  else
    bobgui_popover_set_pointing_to (BOBGUI_POPOVER (link_button->popup_menu), NULL);

  bobgui_popover_popup (BOBGUI_POPOVER (link_button->popup_menu));
}

static void
bobgui_link_button_pressed_cb (BobguiGestureClick *gesture,
                            int              n_press,
                            double           x,
                            double           y,
                            gpointer         user_data)
{
  BobguiLinkButton *link_button = user_data;
  GdkEventSequence *sequence = bobgui_gesture_single_get_current_sequence (BOBGUI_GESTURE_SINGLE (gesture));
  GdkEvent *event = bobgui_gesture_get_last_event (BOBGUI_GESTURE (gesture), sequence);

  if (!bobgui_widget_has_focus (BOBGUI_WIDGET (link_button)))
    bobgui_widget_grab_focus (BOBGUI_WIDGET (link_button));

  if (gdk_event_triggers_context_menu (event) &&
      link_button->uri != NULL)
    {
      bobgui_link_button_do_popup (link_button, x, y);
      bobgui_gesture_set_state (BOBGUI_GESTURE (gesture), BOBGUI_EVENT_SEQUENCE_CLAIMED);
    }
  else
    {
      bobgui_gesture_set_state (BOBGUI_GESTURE (gesture), BOBGUI_EVENT_SEQUENCE_DENIED);
    }
}

static void
launch_done (GObject      *source,
             GAsyncResult *result,
             gpointer      data)
{
  GError *error = NULL;
  gboolean success;

  if (BOBGUI_IS_FILE_LAUNCHER (source))
    success = bobgui_file_launcher_launch_finish (BOBGUI_FILE_LAUNCHER (source), result, &error);
  else if (BOBGUI_IS_URI_LAUNCHER (source))
    success = bobgui_uri_launcher_launch_finish (BOBGUI_URI_LAUNCHER (source), result, &error);
  else
    g_assert_not_reached ();

  if (!success)
    {
      g_warning ("Failed to launch handler: %s", error->message);
      g_error_free (error);
    }
}

static gboolean
bobgui_link_button_activate_link (BobguiLinkButton *link_button)
{
  BobguiWidget *toplevel;
  const char *uri_scheme;

  toplevel = BOBGUI_WIDGET (bobgui_widget_get_root (BOBGUI_WIDGET (link_button)));

  uri_scheme = g_uri_peek_scheme (link_button->uri);
  if (g_strcmp0 (uri_scheme, "file") == 0)
    {
      GFile *file = g_file_new_for_uri (link_button->uri);
      BobguiFileLauncher *launcher;

      launcher = bobgui_file_launcher_new (file);

      bobgui_file_launcher_launch (launcher, BOBGUI_WINDOW (toplevel), NULL, launch_done, NULL);

      g_object_unref (launcher);
      g_object_unref (file);
    }
  else
    {
      BobguiUriLauncher *launcher = bobgui_uri_launcher_new (link_button->uri);

      bobgui_uri_launcher_launch (launcher, BOBGUI_WINDOW (toplevel), NULL, launch_done, NULL);

      g_object_unref (launcher);
    }

  bobgui_link_button_set_visited (link_button, TRUE);

  return TRUE;
}

static void
bobgui_link_button_clicked (BobguiButton *button)
{
  gboolean retval = FALSE;

  g_signal_emit (button, link_signals[ACTIVATE_LINK], 0, &retval);
}

static void
bobgui_link_button_popup_menu (BobguiWidget  *widget,
                            const char *action_name,
                            GVariant   *parameters)
{
  bobgui_link_button_do_popup (BOBGUI_LINK_BUTTON (widget), -1, -1);
}

/**
 * bobgui_link_button_new:
 * @uri: a valid URI
 *
 * Creates a new `BobguiLinkButton` with the URI as its text.
 *
 * Returns: a new link button widget.
 */
BobguiWidget *
bobgui_link_button_new (const char *uri)
{
  char *utf8_uri = NULL;
  BobguiWidget *retval;

  g_return_val_if_fail (uri != NULL, NULL);

  if (g_utf8_validate (uri, -1, NULL))
    {
      utf8_uri = g_strdup (uri);
    }
  else
    {
      GError *conv_err = NULL;

      utf8_uri = g_locale_to_utf8 (uri, -1, NULL, NULL, &conv_err);
      if (conv_err)
        {
          g_warning ("Attempting to convert URI '%s' to UTF-8, but failed "
                     "with error: %s",
                     uri,
                     conv_err->message);
          g_error_free (conv_err);

          utf8_uri = g_strdup (_("Invalid URI"));
        }
    }

  retval = g_object_new (BOBGUI_TYPE_LINK_BUTTON,
  			 "label", utf8_uri,
  			 "uri", uri,
  			 NULL);

  g_free (utf8_uri);

  return retval;
}

/**
 * bobgui_link_button_new_with_label:
 * @uri: a valid URI
 * @label: (nullable): the text of the button
 *
 * Creates a new `BobguiLinkButton` containing a label.
 *
 * Returns: (transfer none): a new link button widget.
 */
BobguiWidget *
bobgui_link_button_new_with_label (const char *uri,
				const char *label)
{
  BobguiWidget *retval;

  g_return_val_if_fail (uri != NULL, NULL);

  if (!label)
    return bobgui_link_button_new (uri);

  retval = g_object_new (BOBGUI_TYPE_LINK_BUTTON,
		         "label", label,
			 "uri", uri,
			 NULL);

  return retval;
}

static gboolean
bobgui_link_button_query_tooltip_cb (BobguiWidget    *widget,
                                  int           x,
                                  int           y,
                                  gboolean      keyboard_tip,
                                  BobguiTooltip   *tooltip,
                                  gpointer      data)
{
  BobguiLinkButton *link_button = BOBGUI_LINK_BUTTON (widget);
  const char *label, *uri;
  const char *text, *markup;

  label = bobgui_button_get_label (BOBGUI_BUTTON (link_button));
  uri = link_button->uri;
  text = bobgui_widget_get_tooltip_text (widget);
  markup = bobgui_widget_get_tooltip_markup (widget);

  if (text == NULL &&
      markup == NULL &&
      label && *label != '\0' && uri && strcmp (label, uri) != 0)
    {
      bobgui_tooltip_set_text (tooltip, uri);
      return TRUE;
    }

  return FALSE;
}

/**
 * bobgui_link_button_set_uri:
 * @link_button: a `BobguiLinkButton`
 * @uri: a valid URI
 *
 * Sets @uri as the URI where the `BobguiLinkButton` points.
 *
 * As a side-effect this unsets the “visited” state of the button.
 */
void
bobgui_link_button_set_uri (BobguiLinkButton *link_button,
			 const char    *uri)
{
  g_return_if_fail (BOBGUI_IS_LINK_BUTTON (link_button));
  g_return_if_fail (uri != NULL);

  g_free (link_button->uri);
  link_button->uri = g_strdup (uri);

  g_object_notify (G_OBJECT (link_button), "uri");

  bobgui_link_button_set_visited (link_button, FALSE);
}

/**
 * bobgui_link_button_get_uri:
 * @link_button: a `BobguiLinkButton`
 *
 * Retrieves the URI of the `BobguiLinkButton`.
 *
 * Returns: a valid URI. The returned string is owned by the link button
 *   and should not be modified or freed.
 */
const char *
bobgui_link_button_get_uri (BobguiLinkButton *link_button)
{
  g_return_val_if_fail (BOBGUI_IS_LINK_BUTTON (link_button), NULL);

  return link_button->uri;
}

/**
 * bobgui_link_button_set_visited:
 * @link_button: a `BobguiLinkButton`
 * @visited: the new “visited” state
 *
 * Sets the “visited” state of the `BobguiLinkButton`.
 *
 * See [method@Bobgui.LinkButton.get_visited] for more details.
 */
void
bobgui_link_button_set_visited (BobguiLinkButton *link_button,
                             gboolean       visited)
{
  g_return_if_fail (BOBGUI_IS_LINK_BUTTON (link_button));

  visited = visited != FALSE;

  if (link_button->visited != visited)
    {
      link_button->visited = visited;

      bobgui_accessible_update_state (BOBGUI_ACCESSIBLE (link_button),
                                   BOBGUI_ACCESSIBLE_STATE_VISITED, visited,
                                   -1);

      if (visited)
        {
          bobgui_widget_unset_state_flags (BOBGUI_WIDGET (link_button), BOBGUI_STATE_FLAG_LINK);
          bobgui_widget_set_state_flags (BOBGUI_WIDGET (link_button), BOBGUI_STATE_FLAG_VISITED, FALSE);
        }
      else
        {
          bobgui_widget_unset_state_flags (BOBGUI_WIDGET (link_button), BOBGUI_STATE_FLAG_VISITED);
          bobgui_widget_set_state_flags (BOBGUI_WIDGET (link_button), BOBGUI_STATE_FLAG_LINK, FALSE);
        }

      g_object_notify (G_OBJECT (link_button), "visited");
    }
}

/**
 * bobgui_link_button_get_visited:
 * @link_button: a `BobguiLinkButton`
 *
 * Retrieves the “visited” state of the `BobguiLinkButton`.
 *
 * The button becomes visited when it is clicked. If the URI
 * is changed on the button, the “visited” state is unset again.
 *
 * The state may also be changed using [method@Bobgui.LinkButton.set_visited].
 *
 * Returns: %TRUE if the link has been visited, %FALSE otherwise
 */
gboolean
bobgui_link_button_get_visited (BobguiLinkButton *link_button)
{
  g_return_val_if_fail (BOBGUI_IS_LINK_BUTTON (link_button), FALSE);

  return link_button->visited;
}
