/* BOBGUI - The Bobgui Framework
 * Copyright (C) 2012 Red Hat, Inc.
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
 * Modified by the BOBGUI Team and others 2012.  See the AUTHORS
 * file for a list of people on the BOBGUI Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * BOBGUI at ftp://ftp.bobgui.org/pub/bobgui/.
 */

#include "config.h"

#include "bobguisearchentryprivate.h"

#include "bobguiaccessibleprivate.h"
#include "bobguieditable.h"
#include "bobguiboxlayout.h"
#include "bobguigestureclick.h"
#include "bobguitextprivate.h"
#include "bobguiimage.h"
#include <glib/gi18n-lib.h>
#include "bobguiprivate.h"
#include "bobguimarshalers.h"
#include "bobguieventcontrollerkey.h"
#include "bobguitypebuiltins.h"
#include "bobguiwidgetprivate.h"
#include "bobguicssnodeprivate.h"
#include "bobguicsspositionvalueprivate.h"

/**
 * BobguiSearchEntry:
 *
 * A single-line text entry widget for use as a search entry.
 *
 * The main API for interacting with a `BobguiSearchEntry` as entry
 * is the `BobguiEditable` interface.
 *
 * <picture>
 *   <source srcset="search-entry-dark.png" media="(prefers-color-scheme: dark)">
 *   <img alt="An example BobguiSearchEntry" src="search-entry.png">
 * </picture>
 *
 * It will show an inactive symbolic “find” icon when the search
 * entry is empty, and a symbolic “clear” icon when there is text.
 * Clicking on the “clear” icon will empty the search entry.
 *
 * To make filtering appear more reactive, it is a good idea to
 * not react to every change in the entry text immediately, but
 * only after a short delay. To support this, `BobguiSearchEntry`
 * emits the [signal@Bobgui.SearchEntry::search-changed] signal which
 * can be used instead of the [signal@Bobgui.Editable::changed] signal.
 *
 * The [signal@Bobgui.SearchEntry::previous-match],
 * [signal@Bobgui.SearchEntry::next-match] and
 * [signal@Bobgui.SearchEntry::stop-search] signals can be used to
 * implement moving between search results and ending the search.
 *
 * Often, `BobguiSearchEntry` will be fed events by means of being
 * placed inside a [class@Bobgui.SearchBar]. If that is not the case,
 * you can use [method@Bobgui.SearchEntry.set_key_capture_widget] to
 * let it capture key input from another widget.
 *
 * `BobguiSearchEntry` provides only minimal API and should be used with
 * the [iface@Bobgui.Editable] API.
 *
 * ## Shortcuts and Gestures
 *
 * The following signals have default keybindings:
 *
 * - [signal@Bobgui.SearchEntry::activate]
 * - [signal@Bobgui.SearchEntry::next-match]
 * - [signal@Bobgui.SearchEntry::previous-match]
 * - [signal@Bobgui.SearchEntry::stop-search]
 *
 * ## CSS Nodes
 *
 * ```
 * entry.search
 * ╰── text
 * ```
 *
 * `BobguiSearchEntry` has a single CSS node with name entry that carries
 * a `.search` style class, and the text node is a child of that.
 *
 * ## Accessibility
 *
 * `BobguiSearchEntry` uses the [enum@Bobgui.AccessibleRole.search_box] role.
 */

enum {
  ACTIVATE,
  SEARCH_CHANGED,
  NEXT_MATCH,
  PREVIOUS_MATCH,
  STOP_SEARCH,
  SEARCH_STARTED,
  LAST_SIGNAL
};

enum {
  PROP_0,
  PROP_PLACEHOLDER_TEXT,
  PROP_INPUT_PURPOSE,
  PROP_INPUT_HINTS,
  PROP_ACTIVATES_DEFAULT,
  PROP_SEARCH_DELAY,
  PROP_KEY_CAPTURE_WIDGET,
  NUM_PROPERTIES,
};

static guint signals[LAST_SIGNAL] = { 0 };

static GParamSpec *props[NUM_PROPERTIES] = { NULL, };

typedef struct _BobguiSearchEntryClass  BobguiSearchEntryClass;

struct _BobguiSearchEntry
{
  BobguiWidget parent;

  BobguiWidget *capture_widget;
  BobguiEventController *capture_widget_controller;

  guint search_delay;

  BobguiWidget *search_icon;
  BobguiWidget *entry;
  BobguiWidget *clear_icon;

  guint delayed_changed_id;
  gboolean content_changed;
  gboolean search_stopped;
};

struct _BobguiSearchEntryClass
{
  BobguiWidgetClass parent_class;

  void (* activate)       (BobguiSearchEntry *entry);
  void (* search_changed) (BobguiSearchEntry *entry);
  void (* next_match)     (BobguiSearchEntry *entry);
  void (* previous_match) (BobguiSearchEntry *entry);
  void (* stop_search)    (BobguiSearchEntry *entry);
};

static void bobgui_search_entry_editable_init (BobguiEditableInterface *iface);
static void bobgui_search_entry_accessible_init (BobguiAccessibleInterface *iface);

G_DEFINE_TYPE_WITH_CODE (BobguiSearchEntry, bobgui_search_entry, BOBGUI_TYPE_WIDGET,
                         G_IMPLEMENT_INTERFACE (BOBGUI_TYPE_ACCESSIBLE,
                                                bobgui_search_entry_accessible_init)
                         G_IMPLEMENT_INTERFACE (BOBGUI_TYPE_EDITABLE,
                                                bobgui_search_entry_editable_init))

static void
text_changed (BobguiSearchEntry *entry)
{
  entry->content_changed = TRUE;
}

static void
bobgui_search_entry_finalize (GObject *object)
{
  BobguiSearchEntry *entry = BOBGUI_SEARCH_ENTRY (object);

  bobgui_editable_finish_delegate (BOBGUI_EDITABLE (entry));

  g_clear_pointer (&entry->search_icon, bobgui_widget_unparent);
  g_clear_pointer (&entry->entry, bobgui_widget_unparent);
  g_clear_pointer (&entry->clear_icon, bobgui_widget_unparent);

  if (entry->delayed_changed_id > 0)
    g_source_remove (entry->delayed_changed_id);

  bobgui_search_entry_set_key_capture_widget (BOBGUI_SEARCH_ENTRY (object), NULL);

  G_OBJECT_CLASS (bobgui_search_entry_parent_class)->finalize (object);
}

static void
bobgui_search_entry_stop_search (BobguiSearchEntry *entry)
{
  entry->search_stopped = TRUE;
}

static void
bobgui_search_entry_set_property (GObject      *object,
                               guint         prop_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  BobguiSearchEntry *entry = BOBGUI_SEARCH_ENTRY (object);
  const char *text;

  if (bobgui_editable_delegate_set_property (object, prop_id, value, pspec))
    {
      if (prop_id == NUM_PROPERTIES + BOBGUI_EDITABLE_PROP_EDITABLE)
        {
          bobgui_accessible_update_property (BOBGUI_ACCESSIBLE (entry),
                                          BOBGUI_ACCESSIBLE_PROPERTY_READ_ONLY, !g_value_get_boolean (value),
                                          -1);
        }

      return;
    }

  switch (prop_id)
    {
    case PROP_PLACEHOLDER_TEXT:
      text = g_value_get_string (value);
      bobgui_text_set_placeholder_text (BOBGUI_TEXT (entry->entry), text);
      bobgui_accessible_update_property (BOBGUI_ACCESSIBLE (entry),
                                      BOBGUI_ACCESSIBLE_PROPERTY_PLACEHOLDER, text,
                                      -1);
      break;

    case PROP_INPUT_PURPOSE:
      bobgui_search_entry_set_input_purpose (entry, g_value_get_enum (value));
      break;

    case PROP_INPUT_HINTS:
      bobgui_search_entry_set_input_hints (entry, g_value_get_flags (value));
      break;

    case PROP_ACTIVATES_DEFAULT:
      if (bobgui_text_get_activates_default (BOBGUI_TEXT (entry->entry)) != g_value_get_boolean (value))
        {
          bobgui_text_set_activates_default (BOBGUI_TEXT (entry->entry), g_value_get_boolean (value));
          g_object_notify_by_pspec (object, pspec);
        }
      break;

    case PROP_SEARCH_DELAY:
      bobgui_search_entry_set_search_delay (entry, g_value_get_uint (value));
      break;

    case PROP_KEY_CAPTURE_WIDGET:
      bobgui_search_entry_set_key_capture_widget (entry, g_value_get_object (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
bobgui_search_entry_get_property (GObject    *object,
                               guint       prop_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  BobguiSearchEntry *entry = BOBGUI_SEARCH_ENTRY (object);

  if (bobgui_editable_delegate_get_property (object, prop_id, value, pspec))
    return;

  switch (prop_id)
    {
    case PROP_PLACEHOLDER_TEXT:
      g_value_set_string (value, bobgui_text_get_placeholder_text (BOBGUI_TEXT (entry->entry)));
      break;

    case PROP_INPUT_PURPOSE:
      g_value_set_enum (value, bobgui_text_get_input_purpose (BOBGUI_TEXT (entry->entry)));
      break;

    case PROP_INPUT_HINTS:
      g_value_set_flags (value, bobgui_text_get_input_hints (BOBGUI_TEXT (entry->entry)));
      break;

    case PROP_ACTIVATES_DEFAULT:
      g_value_set_boolean (value, bobgui_text_get_activates_default (BOBGUI_TEXT (entry->entry)));
      break;

    case PROP_SEARCH_DELAY:
      g_value_set_uint (value, entry->search_delay);
      break;

    case PROP_KEY_CAPTURE_WIDGET:
      g_value_set_object (value, entry->capture_widget);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static gboolean
bobgui_search_entry_grab_focus (BobguiWidget *widget)
{
  BobguiSearchEntry *entry = BOBGUI_SEARCH_ENTRY (widget);

  return bobgui_text_grab_focus_without_selecting (BOBGUI_TEXT (entry->entry));
}

static gboolean
bobgui_search_entry_mnemonic_activate (BobguiWidget *widget,
                                    gboolean   group_cycling)
{
  BobguiSearchEntry *entry = BOBGUI_SEARCH_ENTRY (widget);

  bobgui_widget_grab_focus (entry->entry);

  return TRUE;
}

static int
get_spacing (BobguiWidget *widget)
{
  BobguiCssNode *node = bobgui_widget_get_css_node (widget);
  BobguiCssStyle *style = bobgui_css_node_get_style (node);

  return _bobgui_css_position_value_get_x (style->size->border_spacing, 100);
}

static void
bobgui_search_entry_measure (BobguiWidget      *widget,
                          BobguiOrientation  orientation,
                          int             for_size,
                          int             *minimum,
                          int             *natural,
                          int             *minimum_baseline,
                          int             *natural_baseline)
{
  BobguiSearchEntry *entry = BOBGUI_SEARCH_ENTRY (widget);
  int text_min, text_nat;
  int icon_min, icon_nat;
  int spacing;

  spacing = get_spacing (widget);

  bobgui_widget_measure (entry->entry,
                      orientation,
                      for_size,
                      &text_min, &text_nat,
                      minimum_baseline, natural_baseline);

  *minimum = text_min;
  *natural = text_nat;

  bobgui_widget_measure (entry->search_icon,
                      BOBGUI_ORIENTATION_HORIZONTAL,
                      -1,
                      &icon_min, &icon_nat,
                      NULL, NULL);

  if (orientation == BOBGUI_ORIENTATION_HORIZONTAL)
    {
      *minimum += icon_min + spacing;
      *natural += icon_nat + spacing;
    }
  else
    {
      *minimum = MAX (*minimum, icon_min);
      *natural = MAX (*natural, icon_nat);
    }

  bobgui_widget_measure (entry->clear_icon,
                      BOBGUI_ORIENTATION_HORIZONTAL,
                      -1,
                      &icon_min, &icon_nat,
                      NULL, NULL);

  if (orientation == BOBGUI_ORIENTATION_HORIZONTAL)
    {
      *minimum += icon_min + spacing;
      *natural += icon_nat + spacing;
    }
  else
    {
      *minimum = MAX (*minimum, icon_min);
      *natural = MAX (*natural, icon_nat);

      if (G_LIKELY (*minimum_baseline >= 0))
        *minimum_baseline += (*minimum - text_min) / 2;
      if (G_LIKELY (*natural_baseline >= 0))
        *natural_baseline += (*natural - text_nat) / 2;
    }
}

static void
bobgui_search_entry_size_allocate (BobguiWidget *widget,
                                int        width,
                                int        height,
                                int        baseline)
{
  BobguiSearchEntry *entry = BOBGUI_SEARCH_ENTRY (widget);
  gboolean is_rtl;
  BobguiAllocation text_alloc;
  BobguiAllocation icon_alloc;
  int icon_width;
  int spacing;

  spacing = get_spacing (widget);

  is_rtl = bobgui_widget_get_direction (widget) == BOBGUI_TEXT_DIR_RTL;

  text_alloc.x = 0;
  text_alloc.y = 0;
  text_alloc.width = width;
  text_alloc.height = height;

  if (bobgui_widget_get_valign (widget) != BOBGUI_ALIGN_BASELINE_FILL &&
      bobgui_widget_get_valign (widget) != BOBGUI_ALIGN_BASELINE_CENTER)
    baseline = -1;

  bobgui_widget_measure (entry->search_icon,
                      BOBGUI_ORIENTATION_HORIZONTAL,
                      -1,
                      NULL, &icon_width,
                      NULL, NULL);

  icon_alloc.x = is_rtl ? width - icon_width : 0;
  icon_alloc.y = 0;
  icon_alloc.width = icon_width;
  icon_alloc.height = height;

  bobgui_widget_size_allocate (entry->search_icon, &icon_alloc, baseline);

  text_alloc.width -= icon_width + spacing;
  text_alloc.x += is_rtl ? 0 : icon_width + spacing;

  if (bobgui_widget_get_child_visible (entry->clear_icon))
    {
      bobgui_widget_measure (entry->clear_icon,
                          BOBGUI_ORIENTATION_HORIZONTAL,
                          -1,
                          NULL, &icon_width,
                          NULL, NULL);

      icon_alloc.x = is_rtl ? 0 : width - icon_width;
      icon_alloc.y = 0;
      icon_alloc.width = icon_width;
      icon_alloc.height = height;

      bobgui_widget_size_allocate (entry->clear_icon, &icon_alloc, baseline);

      text_alloc.width -= icon_width + spacing;
      text_alloc.x += is_rtl ? icon_width + spacing : 0;
    }

  bobgui_widget_size_allocate (entry->entry, &text_alloc, baseline);
}

static void
bobgui_search_entry_class_init (BobguiSearchEntryClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (klass);

  object_class->finalize = bobgui_search_entry_finalize;
  object_class->get_property = bobgui_search_entry_get_property;
  object_class->set_property = bobgui_search_entry_set_property;

  widget_class->grab_focus = bobgui_search_entry_grab_focus;
  widget_class->focus = bobgui_widget_focus_child;
  widget_class->mnemonic_activate = bobgui_search_entry_mnemonic_activate;
  widget_class->measure = bobgui_search_entry_measure;
  widget_class->size_allocate = bobgui_search_entry_size_allocate;

  klass->stop_search = bobgui_search_entry_stop_search;

  /**
   * BobguiSearchEntry:placeholder-text:
   *
   * The text that will be displayed in the `BobguiSearchEntry`
   * when it is empty and unfocused.
   */
  props[PROP_PLACEHOLDER_TEXT] =
      g_param_spec_string ("placeholder-text", NULL, NULL,
                           NULL,
                           BOBGUI_PARAM_READWRITE);

  /**
   * BobguiSearchEntry:input-purpose:
   *
   * The purpose for the `BobguiSearchEntry` input used to alter the
   * behaviour of input methods.
   *
   * Since: 4.14
   */
  props[PROP_INPUT_PURPOSE] =
      g_param_spec_enum ("input-purpose", NULL, NULL,
                         BOBGUI_TYPE_INPUT_PURPOSE,
                         BOBGUI_INPUT_PURPOSE_FREE_FORM,
                         BOBGUI_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiSearchEntry:input-hints:
   *
   * The hints about input for the `BobguiSearchEntry` used to alter the
   * behaviour of input methods.
   *
   * Since: 4.14
   */
  props[PROP_INPUT_HINTS] =
      g_param_spec_flags ("input-hints", NULL, NULL,
                          BOBGUI_TYPE_INPUT_HINTS,
                          BOBGUI_INPUT_HINT_NONE,
                          BOBGUI_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiSearchEntry:activates-default:
   *
   * Whether to activate the default widget when Enter is pressed.
   */
  props[PROP_ACTIVATES_DEFAULT] =
      g_param_spec_boolean ("activates-default", NULL, NULL,
                            FALSE,
                            BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiSearchEntry:search-delay:
   *
   * The delay in milliseconds from last keypress to the search
   * changed signal.
   *
   * Since: 4.8
   */
  props[PROP_SEARCH_DELAY] =
      g_param_spec_uint ("search-delay", NULL, NULL,
                         0, G_MAXUINT, 150,
                         BOBGUI_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiSearchEntry:key-capture-widget:
   *
   * The widget that the entry will use to capture key events.
   *
   * Key events are consumed by the search entry to start or continue a search.
   *
   * Since: 4.22
   */
  props[PROP_KEY_CAPTURE_WIDGET] =
      g_param_spec_object ("key-capture-widget", NULL, NULL,
                           BOBGUI_TYPE_WIDGET,
                           BOBGUI_PARAM_READWRITE|G_PARAM_CONSTRUCT|G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, NUM_PROPERTIES, props);
  bobgui_editable_install_properties (object_class, NUM_PROPERTIES);

  /**
   * BobguiSearchEntry::activate:
   * @self: The widget on which the signal is emitted
   *
   * Emitted when the entry is activated.
   *
   * The keybindings for this signal are all forms of the <kbd>Enter</kbd> key.
   */
  signals[ACTIVATE] =
    g_signal_new (I_("activate"),
                  G_OBJECT_CLASS_TYPE (object_class),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                  G_STRUCT_OFFSET (BobguiSearchEntryClass, activate),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 0);

  /**
   * BobguiSearchEntry::search-changed:
   * @entry: the entry on which the signal was emitted
   *
   * Emitted with a delay. The length of the delay can be
   * changed with the [property@Bobgui.SearchEntry:search-delay]
   * property.
   */
  signals[SEARCH_CHANGED] =
    g_signal_new (I_("search-changed"),
                  G_OBJECT_CLASS_TYPE (object_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (BobguiSearchEntryClass, search_changed),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 0);

  /**
   * BobguiSearchEntry::next-match:
   * @entry: the entry on which the signal was emitted
   *
   * Emitted when the user initiates a move to the next match
   * for the current search string.
   *
   * This is a [keybinding signal](class.SignalAction.html).
   *
   * Applications should connect to it, to implement moving
   * between matches.
   *
   * The default bindings for this signal is <kbd>Ctrl</kbd>+<kbd>g</kbd>.
   */
  signals[NEXT_MATCH] =
    g_signal_new (I_("next-match"),
                  G_OBJECT_CLASS_TYPE (object_class),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                  G_STRUCT_OFFSET (BobguiSearchEntryClass, next_match),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 0);

  /**
   * BobguiSearchEntry::previous-match:
   * @entry: the entry on which the signal was emitted
   *
   * Emitted when the user initiates a move to the previous match
   * for the current search string.
   *
   * This is a [keybinding signal](class.SignalAction.html).
   *
   * Applications should connect to it, to implement moving
   * between matches.
   *
   * The default bindings for this signal is
   * <kbd>Ctrl</kbd>+<kbd>Shift</kbd>+<kbd>g</kbd>.
   */
  signals[PREVIOUS_MATCH] =
    g_signal_new (I_("previous-match"),
                  G_OBJECT_CLASS_TYPE (object_class),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                  G_STRUCT_OFFSET (BobguiSearchEntryClass, previous_match),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 0);

  /**
   * BobguiSearchEntry::stop-search:
   * @entry: the entry on which the signal was emitted
   *
   * Emitted when the user stops a search via keyboard input.
   *
   * This is a [keybinding signal](class.SignalAction.html).
   *
   * Applications should connect to it, to implement hiding
   * the search entry in this case.
   *
   * The default bindings for this signal is <kbd>Escape</kbd>.
   */
  signals[STOP_SEARCH] =
    g_signal_new (I_("stop-search"),
                  G_OBJECT_CLASS_TYPE (object_class),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                  G_STRUCT_OFFSET (BobguiSearchEntryClass, stop_search),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 0);

  /**
   * BobguiSearchEntry::search-started:
   * @entry: the entry on which the signal was emitted
   *
   * Emitted when the user initiated a search on the entry.
   */
  signals[SEARCH_STARTED] =
    g_signal_new (I_("search-started"),
                  G_OBJECT_CLASS_TYPE (object_class),
                  G_SIGNAL_RUN_LAST, 0,
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 0);

#ifdef __APPLE__
  bobgui_widget_class_add_binding_signal (widget_class,
                                       GDK_KEY_g, GDK_META_MASK,
                                       "next-match",
                                       NULL);
  bobgui_widget_class_add_binding_signal (widget_class,
                                       GDK_KEY_g, GDK_SHIFT_MASK | GDK_META_MASK,
                                       "previous-match",
                                       NULL);
#else
  bobgui_widget_class_add_binding_signal (widget_class,
                                       GDK_KEY_g, GDK_CONTROL_MASK,
                                       "next-match",
                                       NULL);
  bobgui_widget_class_add_binding_signal (widget_class,
                                       GDK_KEY_g, GDK_SHIFT_MASK | GDK_CONTROL_MASK,
                                       "previous-match",
                                       NULL);
#endif

  bobgui_widget_class_add_binding_signal (widget_class,
                                       GDK_KEY_Escape, 0,
                                       "stop-search",
                                       NULL);

  bobgui_widget_class_set_css_name (widget_class, I_("entry"));
  bobgui_widget_class_set_accessible_role (widget_class, BOBGUI_ACCESSIBLE_ROLE_SEARCH_BOX);
}

static BobguiEditable *
bobgui_search_entry_get_delegate (BobguiEditable *editable)
{
  return BOBGUI_EDITABLE (BOBGUI_SEARCH_ENTRY (editable)->entry);
}

static void
bobgui_search_entry_editable_init (BobguiEditableInterface *iface)
{
  iface->get_delegate = bobgui_search_entry_get_delegate;
}

static gboolean
bobgui_search_entry_accessible_get_platform_state (BobguiAccessible              *self,
                                                BobguiAccessiblePlatformState  state)
{
  return bobgui_editable_delegate_get_accessible_platform_state (BOBGUI_EDITABLE (self), state);
}

static void
bobgui_search_entry_accessible_init (BobguiAccessibleInterface *iface)
{
  BobguiAccessibleInterface *parent_iface = g_type_interface_peek_parent (iface);
  iface->get_at_context = parent_iface->get_at_context;
  iface->get_platform_state = bobgui_search_entry_accessible_get_platform_state;
}

static void
bobgui_search_entry_icon_press (BobguiGestureClick *press,
                             int              n_press,
                             double           x,
                             double           y,
                             BobguiSearchEntry  *entry)
{
  bobgui_gesture_set_state (BOBGUI_GESTURE (press), BOBGUI_EVENT_SEQUENCE_CLAIMED);
}

static void
bobgui_search_entry_icon_release (BobguiGestureClick *press,
                               int              n_press,
                               double           x,
                               double           y,
                               BobguiSearchEntry  *entry)
{
  bobgui_editable_set_text (BOBGUI_EDITABLE (entry->entry), "");
}

static gboolean
bobgui_search_entry_changed_timeout_cb (gpointer user_data)
{
  BobguiSearchEntry *entry = user_data;

  g_signal_emit (entry, signals[SEARCH_CHANGED], 0);
  entry->delayed_changed_id = 0;

  return G_SOURCE_REMOVE;
}

static void
reset_timeout (BobguiSearchEntry *entry)
{
  if (entry->delayed_changed_id > 0)
    g_source_remove (entry->delayed_changed_id);
  entry->delayed_changed_id = g_timeout_add (entry->search_delay,
                                            bobgui_search_entry_changed_timeout_cb,
                                            entry);
  gdk_source_set_static_name_by_id (entry->delayed_changed_id, "[bobgui] bobgui_search_entry_changed_timeout_cb");
}

static void
bobgui_search_entry_changed (BobguiEditable    *editable,
                          BobguiSearchEntry *entry)
{
  const char *str;

  /* Update the icons first */
  str = bobgui_editable_get_text (BOBGUI_EDITABLE (entry->entry));

  if (str == NULL || *str == '\0')
    {
      bobgui_widget_set_child_visible (entry->clear_icon, FALSE);
      bobgui_widget_queue_allocate (BOBGUI_WIDGET (entry));

      if (entry->delayed_changed_id > 0)
        {
          g_source_remove (entry->delayed_changed_id);
          entry->delayed_changed_id = 0;
        }
      g_signal_emit (entry, signals[SEARCH_CHANGED], 0);
    }
  else
    {
      bobgui_widget_set_child_visible (entry->clear_icon, TRUE);
      bobgui_widget_queue_allocate (BOBGUI_WIDGET (entry));

      /* Queue up the timeout */
      reset_timeout (entry);
    }
}

static void
notify_cb (GObject    *object,
           GParamSpec *pspec,
           gpointer    data)
{
  /* The editable interface properties are already forwarded by the editable delegate setup */
  if (g_str_equal (pspec->name, "placeholder-text") ||
      g_str_equal (pspec->name, "activates-default"))
    g_object_notify (data, pspec->name);
}

static void
activate_cb (BobguiText  *text,
             gpointer  data)
{
  g_signal_emit (data, signals[ACTIVATE], 0);
}

static void
catchall_click_press (BobguiGestureClick *gesture,
                      int              n_press,
                      double           x,
                      double           y,
                      gpointer         user_data)
{
  bobgui_gesture_set_state (BOBGUI_GESTURE (gesture), BOBGUI_EVENT_SEQUENCE_CLAIMED);
}

static void
bobgui_search_entry_init (BobguiSearchEntry *entry)
{
  BobguiGesture *press, *catchall;

  entry->search_delay = 150;

  /* The search icon is purely presentational */
  entry->search_icon = g_object_new (BOBGUI_TYPE_IMAGE,
                                     "accessible-role", BOBGUI_ACCESSIBLE_ROLE_PRESENTATION,
                                     "icon-name", "system-search-symbolic",
                                     NULL);
  bobgui_widget_set_parent (entry->search_icon, BOBGUI_WIDGET (entry));

  entry->entry = bobgui_text_new ();
  bobgui_widget_set_parent (entry->entry, BOBGUI_WIDGET (entry));
  bobgui_widget_set_hexpand (entry->entry, TRUE);
  bobgui_editable_init_delegate (BOBGUI_EDITABLE (entry));
  g_signal_connect_swapped (entry->entry, "changed", G_CALLBACK (text_changed), entry);
  g_signal_connect_after (entry->entry, "changed", G_CALLBACK (bobgui_search_entry_changed), entry);
  g_signal_connect_swapped (entry->entry, "preedit-changed", G_CALLBACK (text_changed), entry);
  g_signal_connect (entry->entry, "notify", G_CALLBACK (notify_cb), entry);
  g_signal_connect (entry->entry, "activate", G_CALLBACK (activate_cb), entry);

  entry->clear_icon = g_object_new (BOBGUI_TYPE_IMAGE,
                                    "accessible-role", BOBGUI_ACCESSIBLE_ROLE_PRESENTATION,
                                    "icon-name", "edit-clear-symbolic",
                                    NULL);
  bobgui_widget_set_tooltip_text (entry->clear_icon, _("Clear Entry"));
  bobgui_widget_set_parent (entry->clear_icon, BOBGUI_WIDGET (entry));
  bobgui_widget_set_child_visible (entry->clear_icon, FALSE);

  press = bobgui_gesture_click_new ();
  g_signal_connect (press, "pressed", G_CALLBACK (bobgui_search_entry_icon_press), entry);
  g_signal_connect (press, "released", G_CALLBACK (bobgui_search_entry_icon_release), entry);
  bobgui_widget_add_controller (entry->clear_icon, BOBGUI_EVENT_CONTROLLER (press));

  catchall = bobgui_gesture_click_new ();
  g_signal_connect (catchall, "pressed",
                    G_CALLBACK (catchall_click_press), entry);
  bobgui_widget_add_controller (BOBGUI_WIDGET (entry),
                             BOBGUI_EVENT_CONTROLLER (catchall));

  bobgui_widget_add_css_class (BOBGUI_WIDGET (entry), I_("search"));
}

/**
 * bobgui_search_entry_new:
 *
 * Creates a `BobguiSearchEntry`.
 *
 * Returns: a new `BobguiSearchEntry`
 */
BobguiWidget *
bobgui_search_entry_new (void)
{
  return BOBGUI_WIDGET (g_object_new (BOBGUI_TYPE_SEARCH_ENTRY, NULL));
}

gboolean
bobgui_search_entry_is_keynav (guint           keyval,
                            GdkModifierType state)
{
  if (keyval == GDK_KEY_Tab       || keyval == GDK_KEY_KP_Tab ||
      keyval == GDK_KEY_Up        || keyval == GDK_KEY_KP_Up ||
      keyval == GDK_KEY_Down      || keyval == GDK_KEY_KP_Down ||
      keyval == GDK_KEY_Left      || keyval == GDK_KEY_KP_Left ||
      keyval == GDK_KEY_Right     || keyval == GDK_KEY_KP_Right ||
      keyval == GDK_KEY_Home      || keyval == GDK_KEY_KP_Home ||
      keyval == GDK_KEY_End       || keyval == GDK_KEY_KP_End ||
      keyval == GDK_KEY_Page_Up   || keyval == GDK_KEY_KP_Page_Up ||
      keyval == GDK_KEY_Page_Down || keyval == GDK_KEY_KP_Page_Down ||
      ((state & (GDK_CONTROL_MASK | GDK_ALT_MASK)) != 0))
        return TRUE;

  /* Other navigation events should get automatically
   * ignored as they will not change the content of the entry
   */
  return FALSE;
}

static gboolean
capture_widget_key_handled (BobguiEventControllerKey *controller,
                            guint                  keyval,
                            guint                  keycode,
                            GdkModifierType        state,
                            BobguiWidget             *widget)
{
  BobguiSearchEntry *entry = BOBGUI_SEARCH_ENTRY (widget);
  gboolean handled, was_empty;

  if (bobgui_search_entry_is_keynav (keyval, state) ||
      keyval == GDK_KEY_space ||
      keyval == GDK_KEY_Menu ||
      keyval == GDK_KEY_Return ||
      keyval == GDK_KEY_KP_Enter ||
      keyval == GDK_KEY_ISO_Enter)
    return FALSE;

  entry->content_changed = FALSE;
  entry->search_stopped = FALSE;
  was_empty = (bobgui_text_get_text_length (BOBGUI_TEXT (entry->entry)) == 0);

  handled = bobgui_event_controller_key_forward (controller, entry->entry);

  if (handled)
    {
      if (was_empty && entry->content_changed && !entry->search_stopped)
        g_signal_emit (entry, signals[SEARCH_STARTED], 0);

      return GDK_EVENT_STOP;
    }

  return GDK_EVENT_PROPAGATE;
}

/**
 * bobgui_search_entry_set_key_capture_widget:
 * @entry: a `BobguiSearchEntry`
 * @widget: (nullable) (transfer none): a `BobguiWidget`
 *
 * Sets @widget as the widget that @entry will capture key
 * events from.
 *
 * Key events are consumed by the search entry to start or
 * continue a search.
 *
 * If the entry is part of a `BobguiSearchBar`, it is preferable
 * to call [method@Bobgui.SearchBar.set_key_capture_widget] instead,
 * which will reveal the entry in addition to triggering the
 * search entry.
 *
 * Note that despite the name of this function, the events
 * are only 'captured' in the bubble phase, which means that
 * editable child widgets of @widget will receive text input
 * before it gets captured. If that is not desired, you can
 * capture and forward the events yourself with
 * [method@Bobgui.EventControllerKey.forward].
 */
void
bobgui_search_entry_set_key_capture_widget (BobguiSearchEntry *entry,
                                         BobguiWidget      *widget)
{
  g_return_if_fail (BOBGUI_IS_SEARCH_ENTRY (entry));
  g_return_if_fail (!widget || BOBGUI_IS_WIDGET (widget));

  if (entry->capture_widget == widget)
    return;

  if (entry->capture_widget)
    {
      bobgui_widget_remove_controller (entry->capture_widget,
                                    entry->capture_widget_controller);
      g_object_remove_weak_pointer (G_OBJECT (entry->capture_widget),
                                    (gpointer *) &entry->capture_widget);
    }

  entry->capture_widget = widget;

  if (widget)
    {
      g_object_add_weak_pointer (G_OBJECT (entry->capture_widget),
                                 (gpointer *) &entry->capture_widget);

      entry->capture_widget_controller = bobgui_event_controller_key_new ();
      bobgui_event_controller_set_propagation_phase (entry->capture_widget_controller,
                                                  BOBGUI_PHASE_BUBBLE);
      g_signal_connect (entry->capture_widget_controller, "key-pressed",
                        G_CALLBACK (capture_widget_key_handled), entry);
      g_signal_connect (entry->capture_widget_controller, "key-released",
                        G_CALLBACK (capture_widget_key_handled), entry);
      bobgui_widget_add_controller (widget, entry->capture_widget_controller);
    }

  g_object_notify_by_pspec (G_OBJECT (entry), props[PROP_KEY_CAPTURE_WIDGET]);
}

/**
 * bobgui_search_entry_get_key_capture_widget:
 * @entry: a `BobguiSearchEntry`
 *
 * Gets the widget that @entry is capturing key events from.
 *
 * Returns: (nullable) (transfer none): The key capture widget.
 */
BobguiWidget *
bobgui_search_entry_get_key_capture_widget (BobguiSearchEntry *entry)
{
  g_return_val_if_fail (BOBGUI_IS_SEARCH_ENTRY (entry), NULL);

  return entry->capture_widget;
}

/**
 * bobgui_search_entry_set_search_delay:
 * @entry: a `BobguiSearchEntry`
 * @delay: a delay in milliseconds
 *
 * Set the delay to be used between the last keypress and the
 * [signal@Bobgui.SearchEntry::search-changed] signal being emitted.
 *
 * Since: 4.8
 */
void
bobgui_search_entry_set_search_delay (BobguiSearchEntry *entry,
                                   guint delay)
{
  g_return_if_fail (BOBGUI_IS_SEARCH_ENTRY (entry));

  if (entry->search_delay == delay)
    return;

  entry->search_delay = delay;

  /* Apply the updated timeout */
  reset_timeout (entry);

  g_object_notify_by_pspec (G_OBJECT (entry), props[PROP_SEARCH_DELAY]);
}

/**
 * bobgui_search_entry_get_search_delay:
 * @entry: a `BobguiSearchEntry`
 *
 * Get the delay to be used between the last keypress and the
 * [signal@Bobgui.SearchEntry::search-changed] signal being emitted.
 *
 * Returns: a delay in milliseconds.
 *
 * Since: 4.8
 */
guint
bobgui_search_entry_get_search_delay (BobguiSearchEntry *entry)
{
  g_return_val_if_fail (BOBGUI_IS_SEARCH_ENTRY (entry), 0);

  return entry->search_delay;
}

BobguiEventController *
bobgui_search_entry_get_key_controller (BobguiSearchEntry *entry)
{
  return bobgui_text_get_key_controller (BOBGUI_TEXT (entry->entry));
}

/**
 * bobgui_search_entry_get_placeholder_text:
 * @entry: a `BobguiSearchEntry`
 *
 * Gets the placeholder text associated with @entry.
 *
 * Returns: (nullable): The placeholder text.
 *
 * Since: 4.10
 */
const char *
bobgui_search_entry_get_placeholder_text (BobguiSearchEntry *entry)
{
  g_return_val_if_fail (BOBGUI_IS_SEARCH_ENTRY (entry), NULL);

  return bobgui_text_get_placeholder_text (BOBGUI_TEXT (entry->entry));
}

/**
 * bobgui_search_entry_set_placeholder_text:
 * @entry: a `BobguiSearchEntry`
 * @text: (nullable): the text to set as a placeholder
 *
 * Sets the placeholder text associated with @entry.
 *
 * Since: 4.10
 */
void
bobgui_search_entry_set_placeholder_text (BobguiSearchEntry *entry,
                                       const char     *text)
{
  g_return_if_fail (BOBGUI_IS_SEARCH_ENTRY (entry));

  bobgui_text_set_placeholder_text (BOBGUI_TEXT (entry->entry), text);
}

/**
 * bobgui_search_entry_get_input_purpose:
 * @entry: a `BobguiSearchEntry`
 *
 * Gets the input purpose of @entry.
 *
 * Returns: The input hints
 *
 * Since: 4.14
 */
BobguiInputPurpose
bobgui_search_entry_get_input_purpose (BobguiSearchEntry *entry)
{
  g_return_val_if_fail (BOBGUI_IS_SEARCH_ENTRY (entry), BOBGUI_INPUT_PURPOSE_FREE_FORM);

  return bobgui_text_get_input_purpose (BOBGUI_TEXT (entry->entry));
}

/**
 * bobgui_search_entry_set_input_purpose:
 * @entry: a `BobguiSearchEntry`
 * @purpose: the new input purpose
 *
 * Sets the input purpose of @entry.
 *
 * Since: 4.14
 */
void
bobgui_search_entry_set_input_purpose (BobguiSearchEntry  *entry,
                                    BobguiInputPurpose  purpose)
{
  g_return_if_fail (BOBGUI_IS_SEARCH_ENTRY (entry));

  if (purpose != bobgui_search_entry_get_input_purpose (entry))
    {
      bobgui_text_set_input_purpose (BOBGUI_TEXT (entry->entry), purpose);

      g_object_notify_by_pspec (G_OBJECT (entry), props[PROP_INPUT_PURPOSE]);
    }
}

/**
 * bobgui_search_entry_get_input_hints:
 * @entry: a `BobguiSearchEntry`
 *
 * Gets the input purpose for @entry.
 *
 * Returns: The input hints
 *
 * Since: 4.14
 */
BobguiInputHints
bobgui_search_entry_get_input_hints (BobguiSearchEntry *entry)
{
  g_return_val_if_fail (BOBGUI_IS_SEARCH_ENTRY (entry), BOBGUI_INPUT_HINT_NONE);

  return bobgui_text_get_input_hints (BOBGUI_TEXT (entry->entry));
}

/**
 * bobgui_search_entry_set_input_hints:
 * @entry: a `BobguiSearchEntry`
 * @hints: the new input hints
 *
 * Sets the input hints for @entry.
 *
 * Since: 4.14
 */
void
bobgui_search_entry_set_input_hints (BobguiSearchEntry *entry,
                                  BobguiInputHints   hints)
{
  g_return_if_fail (BOBGUI_IS_SEARCH_ENTRY (entry));

  if (hints != bobgui_search_entry_get_input_hints (entry))
    {
      bobgui_text_set_input_hints (BOBGUI_TEXT (entry->entry), hints);

      g_object_notify_by_pspec (G_OBJECT (entry), props[PROP_INPUT_HINTS]);
    }
}

BobguiText *
bobgui_search_entry_get_text_widget (BobguiSearchEntry *entry)
{
  return BOBGUI_TEXT (entry->entry);
}
