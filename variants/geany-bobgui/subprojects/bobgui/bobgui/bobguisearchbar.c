/* BOBGUI - The Bobgui Framework
 * Copyright (C) 2013 Red Hat, Inc.
 *
 * Authors:
 * - Bastien Nocera <bnocera@redhat.com>
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
 * Modified by the BOBGUI Team and others 2013.  See the AUTHORS
 * file for a list of people on the BOBGUI Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * BOBGUI at ftp://ftp.bobgui.org/pub/bobgui/.
 */

#include "config.h"

#include "bobguisearchbar.h"

#include "bobguibinlayout.h"
#include "bobguibuildable.h"
#include "bobguibuilderprivate.h"
#include "bobguibutton.h"
#include "bobguicenterbox.h"
#include "bobguientryprivate.h"
#include "bobguieventcontrollerkey.h"
#include "bobguiprivate.h"
#include "bobguirevealer.h"
#include "bobguisearchentryprivate.h"
#include "bobguisnapshot.h"
#include "bobguiwidgetprivate.h"

/**
 * BobguiSearchBar:
 *
 * Reveals a search entry when search is started.
 *
 * <picture>
 *   <source srcset="search-bar-dark.png" media="(prefers-color-scheme: dark)">
 *   <img alt="An example BobguiSearchBar" src="search-bar.png">
 * </picture>
 *
 * It can also contain additional widgets, such as drop-down menus,
 * or buttons.  The search bar would appear when a search is started
 * through typing on the keyboard, or the application’s search mode
 * is toggled on.
 *
 * For keyboard presses to start a search, the search bar must be told
 * of a widget to capture key events from through
 * [method@Bobgui.SearchBar.set_key_capture_widget]. This widget will
 * typically be the top-level window, or a parent container of the
 * search bar. Common shortcuts such as Ctrl+F should be handled as an
 * application action, or through the menu items.
 *
 * You will also need to tell the search bar about which entry you
 * are using as your search entry using [method@Bobgui.SearchBar.connect_entry].
 *
 * ## Creating a search bar
 *
 * The following example shows you how to create a more complex search
 * entry.
 *
 * [A simple example](https://gitlab.gnome.org/GNOME/bobgui/tree/main/examples/search-bar.c)
 *
 * # Shortcuts and Gestures
 *
 * `BobguiSearchBar` supports the following keyboard shortcuts:
 *
 * - <kbd>Escape</kbd> hides the search bar.
 *
 * # CSS nodes
 *
 * ```
 * searchbar
 * ╰── revealer
 *     ╰── box
 *          ├── [child]
 *          ╰── [button.close]
 * ```
 *
 * `BobguiSearchBar` has a main CSS node with name searchbar. It has a child
 * node with name revealer that contains a node with name box. The box node
 * contains both the CSS node of the child widget as well as an optional button
 * node which gets the .close style class applied.
 *
 * # Accessibility
 *
 * `BobguiSearchBar` uses the [enum@Bobgui.AccessibleRole.search] role.
 */

typedef struct _BobguiSearchBarClass   BobguiSearchBarClass;

struct _BobguiSearchBar
{
  BobguiWidget parent;

  BobguiWidget   *child;
  BobguiWidget   *revealer;
  BobguiWidget   *box_center;
  BobguiWidget   *close_button;

  BobguiWidget   *entry;
  gboolean     reveal_child;

  BobguiWidget   *capture_widget;
  BobguiEventController *capture_widget_controller;
};

struct _BobguiSearchBarClass
{
  BobguiWidgetClass parent_class;
};

static void bobgui_search_bar_buildable_iface_init (BobguiBuildableIface *iface);

G_DEFINE_TYPE_WITH_CODE (BobguiSearchBar, bobgui_search_bar, BOBGUI_TYPE_WIDGET,
                         G_IMPLEMENT_INTERFACE (BOBGUI_TYPE_BUILDABLE,
                                                bobgui_search_bar_buildable_iface_init))

enum {
  PROP_0,
  PROP_SEARCH_MODE_ENABLED,
  PROP_SHOW_CLOSE_BUTTON,
  PROP_CHILD,
  PROP_KEY_CAPTURE_WIDGET,
  LAST_PROPERTY
};

static GParamSpec *widget_props[LAST_PROPERTY] = { NULL, };

static BobguiBuildableIface *parent_buildable_iface;

static void
bobgui_search_bar_buildable_add_child (BobguiBuildable *buildable,
                                    BobguiBuilder   *builder,
                                    GObject      *child,
                                    const char   *type)
{
  if (BOBGUI_IS_WIDGET (child))
    {
      bobgui_buildable_child_deprecation_warning (buildable, builder, NULL, "child");
      bobgui_search_bar_set_child (BOBGUI_SEARCH_BAR (buildable), BOBGUI_WIDGET (child));
    }
  else
    {
      parent_buildable_iface->add_child (buildable, builder, child, type);
    }
}

static void
bobgui_search_bar_buildable_iface_init (BobguiBuildableIface *iface)
{
  parent_buildable_iface = g_type_interface_peek_parent (iface);

  iface->add_child = bobgui_search_bar_buildable_add_child;
}

static void
stop_search_cb (BobguiWidget    *entry,
                BobguiSearchBar *bar)
{
  bobgui_revealer_set_reveal_child (BOBGUI_REVEALER (bar->revealer), FALSE);
}

static void
reveal_child_changed_cb (GObject      *object,
                         GParamSpec   *pspec,
                         BobguiSearchBar *bar)
{
  gboolean reveal_child;

  g_object_get (object, "reveal-child", &reveal_child, NULL);

  if (reveal_child == bar->reveal_child)
    return;

  bar->reveal_child = reveal_child;

  if (bar->entry)
    {
      if (reveal_child && BOBGUI_IS_ENTRY (bar->entry))
        bobgui_entry_grab_focus_without_selecting (BOBGUI_ENTRY (bar->entry));
      else if (reveal_child && BOBGUI_IS_SEARCH_ENTRY (bar->entry))
        bobgui_widget_grab_focus (bar->entry);
      else
        bobgui_editable_set_text (BOBGUI_EDITABLE (bar->entry), "");
    }

  g_object_notify_by_pspec (G_OBJECT (bar), widget_props[PROP_SEARCH_MODE_ENABLED]);
}

static void
close_button_clicked_cb (BobguiWidget    *button,
                         BobguiSearchBar *bar)
{
  bobgui_revealer_set_reveal_child (BOBGUI_REVEALER (bar->revealer), FALSE);
}

static void
bobgui_search_bar_set_property (GObject      *object,
                             guint         prop_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
  BobguiSearchBar *bar = BOBGUI_SEARCH_BAR (object);

  switch (prop_id)
    {
    case PROP_SEARCH_MODE_ENABLED:
      bobgui_search_bar_set_search_mode (bar, g_value_get_boolean (value));
      break;
    case PROP_SHOW_CLOSE_BUTTON:
      bobgui_search_bar_set_show_close_button (bar, g_value_get_boolean (value));
      break;
    case PROP_CHILD:
      bobgui_search_bar_set_child (bar, g_value_get_object (value));
      break;
    case PROP_KEY_CAPTURE_WIDGET:
      bobgui_search_bar_set_key_capture_widget (bar, g_value_get_object (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bobgui_search_bar_get_property (GObject    *object,
                             guint       prop_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
  BobguiSearchBar *bar = BOBGUI_SEARCH_BAR (object);

  switch (prop_id)
    {
    case PROP_SEARCH_MODE_ENABLED:
      g_value_set_boolean (value, bobgui_search_bar_get_search_mode (bar));
      break;
    case PROP_SHOW_CLOSE_BUTTON:
      g_value_set_boolean (value, bobgui_search_bar_get_show_close_button (bar));
      break;
    case PROP_CHILD:
      g_value_set_object (value, bobgui_search_bar_get_child (bar));
      break;
    case PROP_KEY_CAPTURE_WIDGET:
      g_value_set_object (value, bobgui_search_bar_get_key_capture_widget (bar));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void bobgui_search_bar_set_entry (BobguiSearchBar *bar,
                                      BobguiEditable  *editable);

static void
bobgui_search_bar_dispose (GObject *object)
{
  BobguiSearchBar *bar = BOBGUI_SEARCH_BAR (object);

  bobgui_search_bar_set_key_capture_widget (bar, NULL);
  bobgui_search_bar_set_entry (bar, NULL);

  g_clear_pointer (&bar->revealer, bobgui_widget_unparent);

  bar->child = NULL;
  bar->box_center = NULL;
  bar->close_button = NULL;

  G_OBJECT_CLASS (bobgui_search_bar_parent_class)->dispose (object);
}

static void
bobgui_search_bar_compute_expand (BobguiWidget *widget,
                               gboolean  *hexpand,
                               gboolean  *vexpand)
{
  BobguiSearchBar *bar = BOBGUI_SEARCH_BAR (widget);

  if (bar->child)
    {
      *hexpand = bobgui_widget_compute_expand (bar->child, BOBGUI_ORIENTATION_HORIZONTAL);
      *vexpand = bobgui_widget_compute_expand (bar->child, BOBGUI_ORIENTATION_VERTICAL);
    }
  else
    {
      *hexpand = FALSE;
      *vexpand = FALSE;
    }
}

static void
bobgui_search_bar_class_init (BobguiSearchBarClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (klass);

  object_class->dispose = bobgui_search_bar_dispose;
  object_class->set_property = bobgui_search_bar_set_property;
  object_class->get_property = bobgui_search_bar_get_property;

  widget_class->compute_expand = bobgui_search_bar_compute_expand;
  widget_class->focus = bobgui_widget_focus_child;

  /**
   * BobguiSearchBar:search-mode-enabled: (getter get_search_mode) (setter set_search_mode)
   *
   * Whether the search mode is on and the search bar shown.
   */
  widget_props[PROP_SEARCH_MODE_ENABLED] = g_param_spec_boolean ("search-mode-enabled", NULL, NULL,
                                                                 FALSE,
                                                                 BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiSearchBar:show-close-button:
   *
   * Whether to show the close button in the search bar.
   */
  widget_props[PROP_SHOW_CLOSE_BUTTON] = g_param_spec_boolean ("show-close-button", NULL, NULL,
                                                               FALSE,
                                                               BOBGUI_PARAM_READWRITE|G_PARAM_CONSTRUCT|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiSearchBar:child:
   *
   * The child widget.
   */
  widget_props[PROP_CHILD] = g_param_spec_object ("child", NULL, NULL,
                                                  BOBGUI_TYPE_WIDGET,
                                                  BOBGUI_PARAM_READWRITE|G_PARAM_CONSTRUCT|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiSearchBar:key-capture-widget:
   *
   * The key capture widget.
   */
  widget_props[PROP_KEY_CAPTURE_WIDGET]
      = g_param_spec_object ("key-capture-widget", NULL, NULL,
                             BOBGUI_TYPE_WIDGET,
                             BOBGUI_PARAM_READWRITE|G_PARAM_CONSTRUCT|G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, LAST_PROPERTY, widget_props);

  bobgui_widget_class_set_layout_manager_type (widget_class, BOBGUI_TYPE_BIN_LAYOUT);
  bobgui_widget_class_set_css_name (widget_class, I_("searchbar"));
  bobgui_widget_class_set_accessible_role (widget_class, BOBGUI_ACCESSIBLE_ROLE_SEARCH);
}

static void
bobgui_search_bar_init (BobguiSearchBar *bar)
{
  bar->revealer = bobgui_revealer_new ();
  bobgui_revealer_set_reveal_child (BOBGUI_REVEALER (bar->revealer), FALSE);
  bobgui_widget_set_hexpand (bar->revealer, TRUE);
  bobgui_widget_set_parent (bar->revealer, BOBGUI_WIDGET (bar));

  bar->box_center = bobgui_center_box_new ();
  bobgui_widget_set_hexpand (bar->box_center, TRUE);

  bar->close_button = bobgui_button_new_from_icon_name ("window-close-symbolic");
  bobgui_widget_set_valign (bar->close_button, BOBGUI_ALIGN_CENTER);
  bobgui_widget_add_css_class (bar->close_button, "close");
  bobgui_center_box_set_end_widget (BOBGUI_CENTER_BOX (bar->box_center), bar->close_button);
  bobgui_widget_set_visible (bar->close_button, FALSE);

  bobgui_revealer_set_child (BOBGUI_REVEALER (bar->revealer), bar->box_center);

  g_signal_connect (bar->revealer, "notify::reveal-child",
                    G_CALLBACK (reveal_child_changed_cb), bar);

  g_signal_connect (bar->close_button, "clicked",
                    G_CALLBACK (close_button_clicked_cb), bar);
}

/**
 * bobgui_search_bar_new:
 *
 * Creates a `BobguiSearchBar`.
 *
 * You will need to tell it about which widget is going to be your text
 * entry using [method@Bobgui.SearchBar.connect_entry].
 *
 * Returns: a new `BobguiSearchBar`
 */
BobguiWidget *
bobgui_search_bar_new (void)
{
  return g_object_new (BOBGUI_TYPE_SEARCH_BAR, NULL);
}

static void
bobgui_search_bar_set_entry (BobguiSearchBar *bar,
                          BobguiEditable  *entry)
{
  if (bar->entry != NULL)
    {
      if (BOBGUI_IS_SEARCH_ENTRY (bar->entry))
        {
          bobgui_search_entry_set_key_capture_widget (BOBGUI_SEARCH_ENTRY (bar->entry), NULL);
          g_signal_handlers_disconnect_by_func (bar->entry, stop_search_cb, bar);
        }
      g_object_remove_weak_pointer (G_OBJECT (bar->entry), (gpointer *) &bar->entry);
    }

  bar->entry = BOBGUI_WIDGET (entry);

  if (bar->entry != NULL)
    {
      g_object_add_weak_pointer (G_OBJECT (bar->entry), (gpointer *) &bar->entry);
      if (BOBGUI_IS_SEARCH_ENTRY (bar->entry))
        {
          g_signal_connect (bar->entry, "stop-search",
                            G_CALLBACK (stop_search_cb), bar);
          bobgui_search_entry_set_key_capture_widget (BOBGUI_SEARCH_ENTRY (bar->entry),
                                                   BOBGUI_WIDGET (bar));
        }

    }
}

/**
 * bobgui_search_bar_connect_entry:
 * @bar: a `BobguiSearchBar`
 * @entry: a `BobguiEditable`
 *
 * Connects the `BobguiEditable` widget passed as the one to be used in
 * this search bar.
 *
 * The entry should be a descendant of the search bar. Calling this
 * function manually is only required if the entry isn’t the direct
 * child of the search bar (as in our main example).
 */
void
bobgui_search_bar_connect_entry (BobguiSearchBar *bar,
                              BobguiEditable  *entry)
{
  g_return_if_fail (BOBGUI_IS_SEARCH_BAR (bar));
  g_return_if_fail (entry == NULL || BOBGUI_IS_EDITABLE (entry));

  bobgui_search_bar_set_entry (bar, entry);
}

/**
 * bobgui_search_bar_get_search_mode: (get-property search-mode-enabled)
 * @bar: a `BobguiSearchBar`
 *
 * Returns whether the search mode is on or off.
 *
 * Returns: whether search mode is toggled on
 */
gboolean
bobgui_search_bar_get_search_mode (BobguiSearchBar *bar)
{
  g_return_val_if_fail (BOBGUI_IS_SEARCH_BAR (bar), FALSE);

  return bar->reveal_child;
}

/**
 * bobgui_search_bar_set_search_mode: (set-property search-mode-enabled)
 * @bar: a `BobguiSearchBar`
 * @search_mode: the new state of the search mode
 *
 * Switches the search mode on or off.
 */
void
bobgui_search_bar_set_search_mode (BobguiSearchBar *bar,
                                gboolean      search_mode)
{
  g_return_if_fail (BOBGUI_IS_SEARCH_BAR (bar));

  bobgui_revealer_set_reveal_child (BOBGUI_REVEALER (bar->revealer), search_mode);
}

/**
 * bobgui_search_bar_get_show_close_button:
 * @bar: a `BobguiSearchBar`
 *
 * Returns whether the close button is shown.
 *
 * Returns: whether the close button is shown
 */
gboolean
bobgui_search_bar_get_show_close_button (BobguiSearchBar *bar)
{
  g_return_val_if_fail (BOBGUI_IS_SEARCH_BAR (bar), FALSE);

  return bobgui_widget_get_visible (bar->close_button);
}

/**
 * bobgui_search_bar_set_show_close_button:
 * @bar: a `BobguiSearchBar`
 * @visible: whether the close button will be shown or not
 *
 * Shows or hides the close button.
 *
 * Applications that already have a “search” toggle button should not
 * show a close button in their search bar, as it duplicates the role
 * of the toggle button.
 */
void
bobgui_search_bar_set_show_close_button (BobguiSearchBar *bar,
                                      gboolean      visible)
{
  g_return_if_fail (BOBGUI_IS_SEARCH_BAR (bar));

  visible = visible != FALSE;

  if (bobgui_widget_get_visible (bar->close_button) != visible)
    {
      bobgui_widget_set_visible (bar->close_button, visible);
      g_object_notify_by_pspec (G_OBJECT (bar), widget_props[PROP_SHOW_CLOSE_BUTTON]);
    }
}

static void
changed_cb (gboolean *changed)
{
  *changed = TRUE;
}

static gboolean
capture_widget_key_handled (BobguiEventControllerKey *controller,
                            guint                  keyval,
                            guint                  keycode,
                            GdkModifierType        state,
                            BobguiSearchBar          *bar)
{
  gboolean handled;

  if (!bobgui_widget_get_mapped (BOBGUI_WIDGET (bar)))
    return GDK_EVENT_PROPAGATE;

  if (bar->reveal_child)
    return GDK_EVENT_PROPAGATE;

  if (bar->entry == NULL)
    {
      g_warning ("The search bar does not have an entry connected to it. Call bobgui_search_bar_connect_entry() to connect one.");
      return GDK_EVENT_PROPAGATE;
    }

  if (BOBGUI_IS_SEARCH_ENTRY (bar->entry))
    {
      /* The search entry was told to listen to events from the search bar, so
       * just forward the event to self, so the search entry has an opportunity
       * to intercept those.
       */
      handled = bobgui_event_controller_key_forward (controller, BOBGUI_WIDGET (bar));
    }
  else
    {
      gboolean preedit_changed, buffer_changed;
      guint preedit_change_id, buffer_change_id;
      gboolean res;

      if (bobgui_search_entry_is_keynav (keyval, state) ||
          keyval == GDK_KEY_space ||
          keyval == GDK_KEY_Menu)
        return GDK_EVENT_PROPAGATE;

      if (keyval == GDK_KEY_Escape)
        {
          if (bobgui_revealer_get_reveal_child (BOBGUI_REVEALER (bar->revealer)))
            {
              stop_search_cb (bar->entry, bar);
              return GDK_EVENT_STOP;
            }

          return GDK_EVENT_PROPAGATE;
        }

      handled = GDK_EVENT_PROPAGATE;
      preedit_changed = buffer_changed = FALSE;
      preedit_change_id = g_signal_connect_swapped (bar->entry, "preedit-changed",
                                                    G_CALLBACK (changed_cb), &preedit_changed);
      buffer_change_id = g_signal_connect_swapped (bar->entry, "changed",
                                                   G_CALLBACK (changed_cb), &buffer_changed);

      res = bobgui_event_controller_key_forward (controller, bar->entry);

      g_signal_handler_disconnect (bar->entry, preedit_change_id);
      g_signal_handler_disconnect (bar->entry, buffer_change_id);

      if ((res && buffer_changed) || preedit_changed)
        handled = GDK_EVENT_STOP;
    }

  if (handled == GDK_EVENT_STOP)
    bobgui_revealer_set_reveal_child (BOBGUI_REVEALER (bar->revealer), TRUE);

  return handled;
}

/**
 * bobgui_search_bar_set_key_capture_widget:
 * @bar: a `BobguiSearchBar`
 * @widget: (nullable) (transfer none): a `BobguiWidget`
 *
 * Sets @widget as the widget that @bar will capture key events
 * from.
 *
 * If key events are handled by the search bar, the bar will
 * be shown, and the entry populated with the entered text.
 *
 * Note that despite the name of this function, the events
 * are only 'captured' in the bubble phase, which means that
 * editable child widgets of @widget will receive text input
 * before it gets captured. If that is not desired, you can
 * capture and forward the events yourself with
 * [method@Bobgui.EventControllerKey.forward].
 */
void
bobgui_search_bar_set_key_capture_widget (BobguiSearchBar *bar,
                                       BobguiWidget    *widget)
{
  g_return_if_fail (BOBGUI_IS_SEARCH_BAR (bar));
  g_return_if_fail (!widget || BOBGUI_IS_WIDGET (widget));

  if (bar->capture_widget == widget)
    return;

  if (bar->capture_widget)
    {
      bobgui_widget_remove_controller (bar->capture_widget,
                                    bar->capture_widget_controller);
      g_object_remove_weak_pointer (G_OBJECT (bar->capture_widget),
                                    (gpointer *) &bar->capture_widget);
    }

  bar->capture_widget = widget;

  if (widget)
    {
      g_object_add_weak_pointer (G_OBJECT (bar->capture_widget),
                                 (gpointer *) &bar->capture_widget);

      bar->capture_widget_controller = bobgui_event_controller_key_new ();
      bobgui_event_controller_set_static_name (bar->capture_widget_controller,
                                            "bobgui-search-bar-capture");
      bobgui_event_controller_set_propagation_phase (bar->capture_widget_controller,
                                                  BOBGUI_PHASE_BUBBLE);
      g_signal_connect (bar->capture_widget_controller, "key-pressed",
                        G_CALLBACK (capture_widget_key_handled), bar);
      g_signal_connect (bar->capture_widget_controller, "key-released",
                        G_CALLBACK (capture_widget_key_handled), bar);
      bobgui_widget_add_controller (widget, bar->capture_widget_controller);
    }

  g_object_notify_by_pspec (G_OBJECT (bar), widget_props[PROP_KEY_CAPTURE_WIDGET]);
}

/**
 * bobgui_search_bar_get_key_capture_widget:
 * @bar: a `BobguiSearchBar`
 *
 * Gets the widget that @bar is capturing key events from.
 *
 * Returns: (nullable) (transfer none): The key capture widget.
 **/
BobguiWidget *
bobgui_search_bar_get_key_capture_widget (BobguiSearchBar *bar)
{
  g_return_val_if_fail (BOBGUI_IS_SEARCH_BAR (bar), NULL);

  return bar->capture_widget;
}

/**
 * bobgui_search_bar_set_child:
 * @bar: a `BobguiSearchBar`
 * @child: (nullable): the child widget
 *
 * Sets the child widget of @bar.
 */
void
bobgui_search_bar_set_child (BobguiSearchBar *bar,
                          BobguiWidget    *child)
{
  g_return_if_fail (BOBGUI_IS_SEARCH_BAR (bar));
  g_return_if_fail (child == NULL || bar->child == child || bobgui_widget_get_parent (child) == NULL);

  if (bar->child == child)
    return;

  if (bar->child)
    {
      if (BOBGUI_IS_EDITABLE (bar->child))
        bobgui_search_bar_connect_entry (bar, NULL);

      bobgui_center_box_set_center_widget (BOBGUI_CENTER_BOX (bar->box_center), NULL);
    }

   bar->child = child;

  if (bar->child)
    {
      bobgui_center_box_set_center_widget (BOBGUI_CENTER_BOX (bar->box_center), child);

      if (BOBGUI_IS_EDITABLE (child))
        bobgui_search_bar_connect_entry (bar, BOBGUI_EDITABLE (child));
    }

  g_object_notify_by_pspec (G_OBJECT (bar), widget_props[PROP_CHILD]);
}

/**
 * bobgui_search_bar_get_child:
 * @bar: a `BobguiSearchBar`
 *
 * Gets the child widget of @bar.
 *
 * Returns: (nullable) (transfer none): the child widget of @bar
 */
BobguiWidget *
bobgui_search_bar_get_child (BobguiSearchBar *bar)
{
  return bar->child;
}
