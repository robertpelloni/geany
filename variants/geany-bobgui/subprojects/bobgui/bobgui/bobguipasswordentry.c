/* BOBGUI - The Bobgui Framework
 * Copyright (C) 2019 Red Hat, Inc.
 *
 * Authors:
 * - Matthias Clasen <mclasen@redhat.com>
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

#include "bobguipasswordentryprivate.h"

#include "bobguiaccessibleprivate.h"
#include "bobguitextprivate.h"
#include "bobguieditable.h"
#include "bobguieventcontrollerkey.h"
#include "bobguigestureclick.h"
#include <glib/gi18n-lib.h>
#include "bobguimarshalers.h"
#include "bobguipasswordentrybuffer.h"
#include "bobguiprivate.h"
#include "bobguiwidgetprivate.h"
#include "bobguicsspositionvalueprivate.h"
#include "bobguicssnodeprivate.h"
#include "bobguijoinedmenuprivate.h"


/**
 * BobguiPasswordEntry:
 *
 * A single-line text entry widget for entering passwords and other secrets.
 *
 * <picture>
 *   <source srcset="password-entry-dark.png" media="(prefers-color-scheme: dark)">
 *   <img alt="An example BobguiPasswordEntry" src="password-entry.png">
 * </picture>
 *
 * It does not show its contents in clear text, does not allow to copy it
 * to the clipboard, and it shows a warning when Caps Lock is engaged. If
 * the underlying platform allows it, `BobguiPasswordEntry` will also place
 * the text in a non-pageable memory area, to avoid it being written out
 * to disk by the operating system.
 *
 * Optionally, it can offer a way to reveal the contents in clear text.
 *
 * `BobguiPasswordEntry` provides only minimal API and should be used with
 * the [iface@Bobgui.Editable] API.
 *
 * # CSS Nodes
 *
 * ```
 * entry.password
 * ╰── text
 *     ├── image.caps-lock-indicator
 *     ┊
 * ```
 *
 * `BobguiPasswordEntry` has a single CSS node with name entry that carries
 * a .passwordstyle class. The text Css node below it has a child with
 * name image and style class .caps-lock-indicator for the Caps Lock
 * icon, and possibly other children.
 *
 * # Accessibility
 *
 * `BobguiPasswordEntry` uses the [enum@Bobgui.AccessibleRole.text_box] role.
 */

struct _BobguiPasswordEntry
{
  BobguiWidget parent_instance;

  BobguiWidget *entry;
  BobguiWidget *icon;
  BobguiWidget *peek_icon;
  GdkDevice *keyboard;
  GMenuModel *extra_menu;
};

struct _BobguiPasswordEntryClass
{
  BobguiWidgetClass parent_class;
};

enum {
  ACTIVATE,
  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0, };

enum {
  PROP_PLACEHOLDER_TEXT = 1,
  PROP_ACTIVATES_DEFAULT,
  PROP_SHOW_PEEK_ICON,
  PROP_EXTRA_MENU,
  NUM_PROPERTIES 
};

static GParamSpec *props[NUM_PROPERTIES] = { NULL, };

static void bobgui_password_entry_editable_init (BobguiEditableInterface *iface);
static void bobgui_password_entry_accessible_init (BobguiAccessibleInterface *iface);

G_DEFINE_TYPE_WITH_CODE (BobguiPasswordEntry, bobgui_password_entry, BOBGUI_TYPE_WIDGET,
                         G_IMPLEMENT_INTERFACE (BOBGUI_TYPE_ACCESSIBLE, bobgui_password_entry_accessible_init)
                         G_IMPLEMENT_INTERFACE (BOBGUI_TYPE_EDITABLE, bobgui_password_entry_editable_init))

static void
caps_lock_state_changed (GdkDevice  *device,
                         GParamSpec *pspec,
                        BobguiWidget   *widget)
{
  BobguiPasswordEntry *entry = BOBGUI_PASSWORD_ENTRY (widget);

  bobgui_widget_set_visible (entry->icon, bobgui_editable_get_editable (BOBGUI_EDITABLE (entry)) &&
                                       bobgui_widget_has_focus (entry->entry) &&
                                       !bobgui_text_get_visibility (BOBGUI_TEXT (entry->entry)) &&
                                       gdk_device_get_caps_lock_state (device));
}

static void
focus_changed (BobguiWidget *widget)
{
  BobguiPasswordEntry *entry = BOBGUI_PASSWORD_ENTRY (widget);

  if (entry->keyboard)
    caps_lock_state_changed (entry->keyboard, NULL, widget);
}

static void
bobgui_password_entry_icon_press (BobguiGesture *gesture)
{
  bobgui_gesture_set_state (gesture, BOBGUI_EVENT_SEQUENCE_CLAIMED);
}

/*< private >
 * bobgui_password_entry_toggle_peek:
 * @entry: a `BobguiPasswordEntry`
 *
 * Toggles the text visibility.
 */
void
bobgui_password_entry_toggle_peek (BobguiPasswordEntry *entry)
{
  gboolean visibility;

  visibility = bobgui_text_get_visibility (BOBGUI_TEXT (entry->entry));
  bobgui_text_set_visibility (BOBGUI_TEXT (entry->entry), !visibility);
}

static void
visibility_toggled (GObject          *object,
                    GParamSpec       *pspec,
                    BobguiPasswordEntry *entry)
{
  if (bobgui_text_get_visibility (BOBGUI_TEXT (entry->entry)))
    {
      bobgui_image_set_from_icon_name (BOBGUI_IMAGE (entry->peek_icon), "view-conceal-symbolic");
      bobgui_widget_set_tooltip_text (entry->peek_icon, _("Hide Text"));
    }
  else
    {
      bobgui_image_set_from_icon_name (BOBGUI_IMAGE (entry->peek_icon), "view-reveal-symbolic");
      bobgui_widget_set_tooltip_text (entry->peek_icon, _("Show Text"));
    }

  if (entry->keyboard)
    caps_lock_state_changed (entry->keyboard, NULL, BOBGUI_WIDGET (entry));
}

static void
activate_cb (BobguiPasswordEntry *entry)
{
  g_signal_emit (entry, signals[ACTIVATE], 0);
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
bobgui_password_entry_init (BobguiPasswordEntry *entry)
{
  BobguiGesture *catchall;
  BobguiEntryBuffer *buffer = bobgui_password_entry_buffer_new ();

  entry->entry = bobgui_text_new ();
  bobgui_text_set_buffer (BOBGUI_TEXT (entry->entry), buffer);
  bobgui_text_set_visibility (BOBGUI_TEXT (entry->entry), FALSE);
  bobgui_text_set_input_purpose (BOBGUI_TEXT (entry->entry), BOBGUI_INPUT_PURPOSE_PASSWORD);
  bobgui_widget_set_parent (entry->entry, BOBGUI_WIDGET (entry));
  bobgui_editable_init_delegate (BOBGUI_EDITABLE (entry));
  g_signal_connect_swapped (entry->entry, "notify::has-focus", G_CALLBACK (focus_changed), entry);
  g_signal_connect_swapped (entry->entry, "activate", G_CALLBACK (activate_cb), entry);

  entry->icon = g_object_new (BOBGUI_TYPE_IMAGE,
                              "icon-name", "caps-lock-symbolic",
                              "accessible-role", BOBGUI_ACCESSIBLE_ROLE_ALERT,
                              NULL);
  bobgui_widget_set_tooltip_text (entry->icon, _("Caps Lock is on"));
  bobgui_widget_add_css_class (entry->icon, "caps-lock-indicator");
  bobgui_widget_set_cursor (entry->icon, bobgui_widget_get_cursor (entry->entry));
  bobgui_widget_set_parent (entry->icon, BOBGUI_WIDGET (entry));

  catchall = bobgui_gesture_click_new ();
  g_signal_connect (catchall, "pressed",
                    G_CALLBACK (catchall_click_press), entry);
  bobgui_widget_add_controller (BOBGUI_WIDGET (entry),
                             BOBGUI_EVENT_CONTROLLER (catchall));

  bobgui_widget_add_css_class (BOBGUI_WIDGET (entry), I_("password"));

  bobgui_password_entry_set_extra_menu (entry, NULL);

  /* Transfer ownership to the BobguiText widget */
  g_object_unref (buffer);
}

static void
bobgui_password_entry_realize (BobguiWidget *widget)
{
  BobguiPasswordEntry *entry = BOBGUI_PASSWORD_ENTRY (widget);
  GdkSeat *seat;

  BOBGUI_WIDGET_CLASS (bobgui_password_entry_parent_class)->realize (widget);

  seat = gdk_display_get_default_seat (bobgui_widget_get_display (widget));
  if (seat)
    entry->keyboard = gdk_seat_get_keyboard (seat);

  if (entry->keyboard)
    {
      g_signal_connect (entry->keyboard, "notify::caps-lock-state",
                        G_CALLBACK (caps_lock_state_changed), entry);
      caps_lock_state_changed (entry->keyboard, NULL, widget);
    }
}

static void
bobgui_password_entry_unrealize (BobguiWidget *widget)
{
  BobguiPasswordEntry *entry = BOBGUI_PASSWORD_ENTRY (widget);

  if (entry->keyboard)
    {
      g_signal_handlers_disconnect_by_func (entry->keyboard, caps_lock_state_changed, entry);
      entry->keyboard = NULL;
    }

  BOBGUI_WIDGET_CLASS (bobgui_password_entry_parent_class)->unrealize (widget);
}

static void
bobgui_password_entry_dispose (GObject *object)
{
  BobguiPasswordEntry *entry = BOBGUI_PASSWORD_ENTRY (object);

  if (entry->entry)
    bobgui_editable_finish_delegate (BOBGUI_EDITABLE (entry));

  g_clear_pointer (&entry->entry, bobgui_widget_unparent);
  g_clear_pointer (&entry->icon, bobgui_widget_unparent);
  g_clear_pointer (&entry->peek_icon, bobgui_widget_unparent);
  g_clear_object (&entry->extra_menu);

  G_OBJECT_CLASS (bobgui_password_entry_parent_class)->dispose (object);
}

static void
bobgui_password_entry_set_property (GObject      *object,
                                 guint         prop_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  BobguiPasswordEntry *entry = BOBGUI_PASSWORD_ENTRY (object);
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

    case PROP_ACTIVATES_DEFAULT:
      if (bobgui_text_get_activates_default (BOBGUI_TEXT (entry->entry)) != g_value_get_boolean (value))
        {
          bobgui_text_set_activates_default (BOBGUI_TEXT (entry->entry), g_value_get_boolean (value));
          g_object_notify_by_pspec (object, pspec);
        }
      break;

    case PROP_SHOW_PEEK_ICON:
      bobgui_password_entry_set_show_peek_icon (entry, g_value_get_boolean (value));
      break;

    case PROP_EXTRA_MENU:
      bobgui_password_entry_set_extra_menu (entry, g_value_get_object (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bobgui_password_entry_get_property (GObject    *object,
                                 guint       prop_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
  BobguiPasswordEntry *entry = BOBGUI_PASSWORD_ENTRY (object);

  if (bobgui_editable_delegate_get_property (object, prop_id, value, pspec))
    return;

  switch (prop_id)
    {
    case PROP_PLACEHOLDER_TEXT:
      g_value_set_string (value, bobgui_text_get_placeholder_text (BOBGUI_TEXT (entry->entry)));
      break;

    case PROP_ACTIVATES_DEFAULT:
      g_value_set_boolean (value, bobgui_text_get_activates_default (BOBGUI_TEXT (entry->entry)));
      break;

    case PROP_SHOW_PEEK_ICON:
      g_value_set_boolean (value, bobgui_password_entry_get_show_peek_icon (entry));
      break;

    case PROP_EXTRA_MENU:
      g_value_set_object (value, entry->extra_menu);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bobgui_password_entry_measure (BobguiWidget      *widget,
                            BobguiOrientation  orientation,
                            int             for_size,
                            int            *minimum,
                            int            *natural,
                            int            *minimum_baseline,
                            int            *natural_baseline)
{
  BobguiPasswordEntry *entry = BOBGUI_PASSWORD_ENTRY (widget);
  int icon_min = 0, icon_nat = 0;

  bobgui_widget_measure (entry->entry, orientation, for_size,
                      minimum, natural,
                      minimum_baseline, natural_baseline);

  if (entry->icon && bobgui_widget_get_visible (entry->icon))
    bobgui_widget_measure (entry->icon, orientation, for_size,
                        &icon_min, &icon_nat,
                        NULL, NULL);

  if (entry->peek_icon && bobgui_widget_get_visible (entry->peek_icon))
    bobgui_widget_measure (entry->peek_icon, orientation, for_size,
                        &icon_min, &icon_nat,
                        NULL, NULL);
}

static void
bobgui_password_entry_size_allocate (BobguiWidget *widget,
                                  int        width,
                                  int        height,
                                  int        baseline)
{
  BobguiPasswordEntry *entry = BOBGUI_PASSWORD_ENTRY (widget);
  BobguiCssStyle *style = bobgui_css_node_get_style (bobgui_widget_get_css_node (widget));
  int icon_min = 0, icon_nat = 0;
  int peek_min = 0, peek_nat = 0;
  int text_width;
  int spacing;

  spacing = _bobgui_css_position_value_get_x (style->size->border_spacing, 100);

  if (entry->icon && bobgui_widget_get_visible (entry->icon))
    bobgui_widget_measure (entry->icon, BOBGUI_ORIENTATION_HORIZONTAL, -1,
                        &icon_min, &icon_nat,
                        NULL, NULL);

  if (entry->peek_icon && bobgui_widget_get_visible (entry->peek_icon))
    bobgui_widget_measure (entry->peek_icon, BOBGUI_ORIENTATION_HORIZONTAL, -1,
                        &peek_min, &peek_nat,
                        NULL, NULL);

  text_width = width - (icon_nat + (icon_nat > 0 ? spacing : 0))
                     - (peek_nat + (peek_nat > 0 ? spacing : 0));

  bobgui_widget_size_allocate (entry->entry,
                            &(BobguiAllocation) { 0, 0, text_width, height },
                            baseline);

  if (entry->icon && bobgui_widget_get_visible (entry->icon))
    bobgui_widget_size_allocate (entry->icon,
                              &(BobguiAllocation) { text_width + spacing, 0, icon_nat, height },
                              baseline);

  if (entry->peek_icon && bobgui_widget_get_visible (entry->peek_icon))
    bobgui_widget_size_allocate (entry->peek_icon,
                              &(BobguiAllocation) { text_width + spacing + icon_nat + (icon_nat > 0 ? spacing : 0), 0, peek_nat, height },
                              baseline);
}

static gboolean
bobgui_password_entry_mnemonic_activate (BobguiWidget *widget,
                                      gboolean   group_cycling)
{
  BobguiPasswordEntry *entry = BOBGUI_PASSWORD_ENTRY (widget);

  bobgui_widget_grab_focus (entry->entry);

  return TRUE;
}

static void
bobgui_password_entry_class_init (BobguiPasswordEntryClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (klass);

  object_class->dispose = bobgui_password_entry_dispose;
  object_class->get_property = bobgui_password_entry_get_property;
  object_class->set_property = bobgui_password_entry_set_property;

  widget_class->realize = bobgui_password_entry_realize;
  widget_class->unrealize = bobgui_password_entry_unrealize;
  widget_class->measure = bobgui_password_entry_measure;
  widget_class->size_allocate = bobgui_password_entry_size_allocate;
  widget_class->mnemonic_activate = bobgui_password_entry_mnemonic_activate;
  widget_class->grab_focus = bobgui_widget_grab_focus_child;
  widget_class->focus = bobgui_widget_focus_child;

  /**
   * BobguiPasswordEntry:placeholder-text:
   *
   * The text that will be displayed in the `BobguiPasswordEntry`
   * when it is empty and unfocused.
   */
  props[PROP_PLACEHOLDER_TEXT] =
      g_param_spec_string ("placeholder-text", NULL, NULL,
                           NULL,
                           BOBGUI_PARAM_READWRITE);

  /**
   * BobguiPasswordEntry:activates-default:
   *
   * Whether to activate the default widget when Enter is pressed.
   */
  props[PROP_ACTIVATES_DEFAULT] =
      g_param_spec_boolean ("activates-default", NULL, NULL,
                            FALSE,
                            BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiPasswordEntry:show-peek-icon:
   *
   * Whether to show an icon for revealing the content.
   */
  props[PROP_SHOW_PEEK_ICON] =
      g_param_spec_boolean ("show-peek-icon", NULL, NULL,
                            FALSE,
                            BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiPasswordEntry:extra-menu:
   *
   * A menu model whose contents will be appended to
   * the context menu.
   */
  props[PROP_EXTRA_MENU] =
      g_param_spec_object ("extra-menu", NULL, NULL,
                           G_TYPE_MENU_MODEL,
                           BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, NUM_PROPERTIES, props);
  bobgui_editable_install_properties (object_class, NUM_PROPERTIES);

  /**
   * BobguiPasswordEntry::activate:
   * @self: The widget on which the signal is emitted
   *
   * Emitted when the entry is activated.
   *
   * The keybindings for this signal are all forms of the Enter key.
   */
  signals[ACTIVATE] =
    g_signal_new (I_("activate"),
                  G_OBJECT_CLASS_TYPE (object_class),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                  0,
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 0);

  bobgui_widget_class_set_css_name (widget_class, I_("entry"));
  bobgui_widget_class_set_accessible_role (widget_class, BOBGUI_ACCESSIBLE_ROLE_TEXT_BOX);
}

static BobguiEditable *
bobgui_password_entry_get_delegate (BobguiEditable *editable)
{
  BobguiPasswordEntry *entry = BOBGUI_PASSWORD_ENTRY (editable);

  return BOBGUI_EDITABLE (entry->entry);
}

static void
bobgui_password_entry_editable_init (BobguiEditableInterface *iface)
{
  iface->get_delegate = bobgui_password_entry_get_delegate;
}

static gboolean
bobgui_password_entry_accessible_get_platform_state (BobguiAccessible              *self,
                                                  BobguiAccessiblePlatformState  state)
{
  return bobgui_editable_delegate_get_accessible_platform_state (BOBGUI_EDITABLE (self), state);
}

static void
bobgui_password_entry_accessible_init (BobguiAccessibleInterface *iface)
{
  BobguiAccessibleInterface *parent_iface = g_type_interface_peek_parent (iface);
  iface->get_at_context = parent_iface->get_at_context;
  iface->get_platform_state = bobgui_password_entry_accessible_get_platform_state;
}

/*< private >
 * bobgui_password_entry_get_text_widget
 * @entry: a `BobguiPasswordEntry`
 *
 * Retrieves the `BobguiText` delegate of the `BobguiPasswordEntry`.
 *
 * Returns: (transfer none): the `BobguiText` delegate widget
 */
BobguiText *
bobgui_password_entry_get_text_widget (BobguiPasswordEntry *entry)
{
  g_return_val_if_fail (BOBGUI_IS_PASSWORD_ENTRY (entry), NULL);

  return BOBGUI_TEXT (entry->entry);
}

/**
 * bobgui_password_entry_new:
 *
 * Creates a `BobguiPasswordEntry`.
 *
 * Returns: a new `BobguiPasswordEntry`
 */
BobguiWidget *
bobgui_password_entry_new (void)
{
  return BOBGUI_WIDGET (g_object_new (BOBGUI_TYPE_PASSWORD_ENTRY, NULL));
}

/**
 * bobgui_password_entry_set_show_peek_icon:
 * @entry: a `BobguiPasswordEntry`
 * @show_peek_icon: whether to show the peek icon
 *
 * Sets whether the entry should have a clickable icon
 * to reveal the contents.
 *
 * Setting this to %FALSE also hides the text again.
 */
void
bobgui_password_entry_set_show_peek_icon (BobguiPasswordEntry *entry,
                                       gboolean          show_peek_icon)
{
  g_return_if_fail (BOBGUI_IS_PASSWORD_ENTRY (entry));

  show_peek_icon = !!show_peek_icon;

  if (show_peek_icon == (entry->peek_icon != NULL))
    return;

  if (show_peek_icon)
    {
      BobguiGesture *press;

      entry->peek_icon = bobgui_image_new_from_icon_name ("view-reveal-symbolic");
      bobgui_widget_set_tooltip_text (entry->peek_icon, _("Show Text"));
      bobgui_widget_set_parent (entry->peek_icon, BOBGUI_WIDGET (entry));

      press = bobgui_gesture_click_new ();
      g_signal_connect (press, "pressed",
                        G_CALLBACK (bobgui_password_entry_icon_press), entry);
      g_signal_connect_swapped (press, "released",
                                G_CALLBACK (bobgui_password_entry_toggle_peek), entry);
      bobgui_widget_add_controller (entry->peek_icon, BOBGUI_EVENT_CONTROLLER (press));

      g_signal_connect (entry->entry, "notify::visibility",
                        G_CALLBACK (visibility_toggled), entry);
      visibility_toggled (G_OBJECT (entry->entry), NULL, entry);
    }
  else
    {
      g_clear_pointer (&entry->peek_icon, bobgui_widget_unparent);
      bobgui_text_set_visibility (BOBGUI_TEXT (entry->entry), FALSE);
      g_signal_handlers_disconnect_by_func (entry->entry,
                                            visibility_toggled,
                                            entry);
    }

  if (entry->keyboard)
    caps_lock_state_changed (entry->keyboard, NULL, BOBGUI_WIDGET (entry));

  g_object_notify_by_pspec (G_OBJECT (entry), props[PROP_SHOW_PEEK_ICON]);
}

/**
 * bobgui_password_entry_get_show_peek_icon:
 * @entry: a `BobguiPasswordEntry`
 *
 * Returns whether the entry is showing an icon to
 * reveal the contents.
 *
 * Returns: %TRUE if an icon is shown
 */
gboolean
bobgui_password_entry_get_show_peek_icon (BobguiPasswordEntry *entry)
{
  g_return_val_if_fail (BOBGUI_IS_PASSWORD_ENTRY (entry), FALSE);

  return entry->peek_icon != NULL;
}

/**
 * bobgui_password_entry_set_extra_menu:
 * @entry: a `BobguiPasswordEntry`
 * @model: (nullable): a `GMenuModel`
 *
 * Sets a menu model to add when constructing
 * the context menu for @entry.
 */
void
bobgui_password_entry_set_extra_menu (BobguiPasswordEntry *entry,
                                   GMenuModel       *model)
{
  BobguiJoinedMenu *joined;
  GMenu *menu;
  GMenu *section;
  GMenuItem *item;

  g_return_if_fail (BOBGUI_IS_PASSWORD_ENTRY (entry));

  /* bypass this check for the initial call from init */
  if (entry->extra_menu)
    {
      if (!g_set_object (&entry->extra_menu, model))
        return;
    }

  joined = bobgui_joined_menu_new ();
  menu = g_menu_new ();

  section = g_menu_new ();
  item = g_menu_item_new (_("_Show Text"), "misc.toggle-visibility");
  g_menu_item_set_attribute (item, "touch-icon", "s", "view-reveal-symbolic");
  g_menu_append_item (section, item);
  g_object_unref (item);

  g_menu_append_section (menu, NULL, G_MENU_MODEL (section));
  g_object_unref (section);

  bobgui_joined_menu_append_menu (joined, G_MENU_MODEL (menu));
  g_object_unref (menu);

  if (model)
    bobgui_joined_menu_append_menu (joined, model);

  bobgui_text_set_extra_menu (BOBGUI_TEXT (entry->entry), G_MENU_MODEL (joined));

  g_object_unref (joined);

  g_object_notify_by_pspec (G_OBJECT (entry), props[PROP_EXTRA_MENU]);
}

/**
 * bobgui_password_entry_get_extra_menu:
 * @entry: a `BobguiPasswordEntry`
 *
 * Gets the menu model set with bobgui_password_entry_set_extra_menu().
 *
 * Returns: (transfer none) (nullable): the menu model
 */
GMenuModel *
bobgui_password_entry_get_extra_menu (BobguiPasswordEntry *entry)
{
  return entry->extra_menu;
}
