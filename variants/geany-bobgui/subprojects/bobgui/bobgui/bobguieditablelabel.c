/* BOBGUI - The Bobgui Framework
 * Copyright (C) 2020, Red Hat, Inc
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

#include "bobguidragsource.h"
#include "bobguidroptarget.h"
#include "bobguieditablelabel.h"
#include "bobguieditable.h"
#include "bobguilabel.h"
#include "bobguistack.h"
#include "bobguitext.h"
#include "bobguibinlayout.h"
#include "bobguigestureclick.h"
#include "bobguiprivate.h"
#include "bobguishortcut.h"
#include "bobguishortcuttrigger.h"
#include "bobguipopovermenu.h"
#include "bobguiwidgetprivate.h"
#include "bobguieventcontrollerfocus.h"

#include <glib/gi18n-lib.h>

/**
 * BobguiEditableLabel:
 *
 * Allows users to edit the displayed text by switching to an “edit mode”.
 *
 * <picture>
 *   <source srcset="editable-label-dark.png" media="(prefers-color-scheme: dark)">
 *   <img alt="An example BobguiEditableLabel" src="editable-label.png">
 * </picture>
 *
 * `BobguiEditableLabel` does not have API of its own, but it
 * implements the [iface@Bobgui.Editable] interface.
 *
 * The default bindings for activating the edit mode is
 * to click or press the Enter key. The default bindings
 * for leaving the edit mode are the Enter key (to save
 * the results) or the Escape key (to cancel the editing).
 *
 * # Shortcuts and Gestures
 *
 * `BobguiEditableLabel` supports the following keyboard shortcuts:
 *
 * - <kbd>Enter</kbd> starts editing.
 * - <kbd>Escape</kbd> stops editing.
 *
 * # Actions
 *
 * `BobguiEditableLabel` defines a set of built-in actions:
 *
 * - `editing.starts` switches the widget into editing mode.
 * - `editing.stop` switches the widget out of editing mode.
 *
 * # CSS nodes
 *
 * ```
 * editablelabel[.editing]
 * ╰── stack
 *     ├── label
 *     ╰── text
 * ```
 *
 * `BobguiEditableLabel` has a main node with the name editablelabel.
 * When the entry is in editing mode, it gets the .editing style
 * class.
 *
 * For all the subnodes added to the text node in various situations,
 * see [class@Bobgui.Text].
 */

struct _BobguiEditableLabel
{
  BobguiWidget parent_instance;

  BobguiWidget *stack;
  BobguiWidget *label;
  BobguiWidget *entry;
  BobguiWidget *popup_menu;

  guint stop_editing_soon_id;
};

struct _BobguiEditableLabelClass
{
  BobguiWidgetClass parent_class;
};

enum
{
  PROP_EDITING = 1,
  NUM_PROPERTIES
};

static GParamSpec *properties[NUM_PROPERTIES] = { NULL, };

static BobguiEditable *
bobgui_editable_label_get_delegate (BobguiEditable *editable)
{
  BobguiEditableLabel *self = BOBGUI_EDITABLE_LABEL (editable);

  return BOBGUI_EDITABLE (self->entry);
}

static void
bobgui_editable_label_editable_init (BobguiEditableInterface *iface)
{
  iface->get_delegate = bobgui_editable_label_get_delegate;
}


G_DEFINE_TYPE_WITH_CODE (BobguiEditableLabel, bobgui_editable_label, BOBGUI_TYPE_WIDGET,
                         G_IMPLEMENT_INTERFACE (BOBGUI_TYPE_EDITABLE,
                                                bobgui_editable_label_editable_init))

static void
start_editing (BobguiWidget  *widget,
               const char *action_name,
               GVariant   *parameters)
{
  bobgui_editable_label_start_editing (BOBGUI_EDITABLE_LABEL (widget));
}

static void
stop_editing (BobguiWidget  *widget,
              const char *action_name,
              GVariant   *parameters)
{
  bobgui_editable_label_stop_editing (BOBGUI_EDITABLE_LABEL (widget),
                                   g_variant_get_boolean (parameters));
}

static void
clipboard_copy (BobguiWidget  *widget,
                const char *name,
                GVariant   *parameter)
{
  BobguiEditableLabel *self = BOBGUI_EDITABLE_LABEL (widget);
  GdkClipboard *clipboard;
  const char *text;

  clipboard = bobgui_widget_get_clipboard (widget);
  text = bobgui_label_get_label (BOBGUI_LABEL (self->label));

  gdk_clipboard_set_text (clipboard, text);
}

static GMenuModel *
create_menu (void)
{
  GMenu *menu;

  menu = g_menu_new ();

  g_menu_append (menu, _("_Edit"), "editing.start");
  g_menu_append (menu, _("_Copy"), "clipboard.copy");

  return G_MENU_MODEL (menu);
}

static void
do_popup (BobguiEditableLabel *self,
          double            x,
          double            y)
{
  if (x != -1 && y != -1)
    {
      GdkRectangle rect = { x, y, 1, 1 };
      bobgui_popover_set_pointing_to (BOBGUI_POPOVER (self->popup_menu), &rect);
    }
  else
    {
      bobgui_popover_set_pointing_to (BOBGUI_POPOVER (self->popup_menu), NULL);
    }

  bobgui_popover_popup (BOBGUI_POPOVER (self->popup_menu));
}

static void
clicked_cb (BobguiGestureClick  *gesture,
            int               n_press,
            double            x,
            double            y,
            BobguiEditableLabel *self)
{
  guint button;
  GdkEventSequence *sequence;
  GdkEvent *event;

  button = bobgui_gesture_single_get_current_button (BOBGUI_GESTURE_SINGLE (gesture));
  sequence = bobgui_gesture_single_get_current_sequence (BOBGUI_GESTURE_SINGLE (gesture));
  event = bobgui_gesture_get_last_event (BOBGUI_GESTURE (gesture), sequence);

  if (button == GDK_BUTTON_PRIMARY)
    {
      bobgui_gesture_set_state (BOBGUI_GESTURE (gesture), BOBGUI_EVENT_SEQUENCE_CLAIMED);
      bobgui_widget_activate_action (BOBGUI_WIDGET (self), "editing.start", NULL);
    }
  else if (gdk_event_triggers_context_menu (event))
    {
      bobgui_gesture_set_state (BOBGUI_GESTURE (gesture), BOBGUI_EVENT_SEQUENCE_CLAIMED);
      do_popup (self, x, y);
    }
}

static void
popup_menu (BobguiWidget  *widget,
            const char *action_name,
            GVariant   *parameters)
{
  BobguiEditableLabel *self = BOBGUI_EDITABLE_LABEL (widget);

  do_popup (self, -1, -1);
}

static void
activate_cb (BobguiWidget *self)
{
  bobgui_widget_activate_action (self, "editing.stop", "b", TRUE);
}

static void
text_changed (BobguiEditableLabel *self)
{
  /* Sync the entry text to the label, unless we are editing.
   *
   * This is necessary to catch apis like bobgui_editable_insert_text(),
   * which don't go through the text property.
   */
  if (!bobgui_editable_label_get_editing (self))
    {
      const char *text = bobgui_editable_get_text (BOBGUI_EDITABLE (self->entry));
      bobgui_label_set_label (BOBGUI_LABEL (self->label), text);
    }
}

static gboolean
bobgui_editable_label_drag_accept (BobguiDropTarget    *dest,
                                GdkDrop          *drop,
                                BobguiEditableLabel *self)
{
  if (!bobgui_editable_get_editable (BOBGUI_EDITABLE (self)))
    return FALSE;

  if ((gdk_drop_get_actions (drop) & bobgui_drop_target_get_actions (dest)) == GDK_ACTION_NONE)
    return FALSE;

  return gdk_content_formats_match (bobgui_drop_target_get_formats (dest), gdk_drop_get_formats (drop));
}

static gboolean
bobgui_editable_label_drag_drop (BobguiDropTarget    *dest,
                              const GValue     *value,
                              double            x,
                              double            y,
                              BobguiEditableLabel *self)
{
  if (!bobgui_editable_get_editable (BOBGUI_EDITABLE (self)))
    return FALSE;

  bobgui_editable_set_text (BOBGUI_EDITABLE (self), g_value_get_string (value));

  return TRUE;
}

static GdkContentProvider *
bobgui_editable_label_prepare_drag (BobguiDragSource    *source,
                                 double            x,
                                 double            y,
                                 BobguiEditableLabel *self)
{
  if (!bobgui_editable_get_editable (BOBGUI_EDITABLE (self)))
    return NULL;

  return gdk_content_provider_new_typed (G_TYPE_STRING,
                                         bobgui_label_get_label (BOBGUI_LABEL (self->label)));
}

static gboolean
stop_editing_soon (gpointer data)
{
  BobguiEventController *controller = data;
  BobguiEditableLabel *self = BOBGUI_EDITABLE_LABEL (bobgui_event_controller_get_widget (controller));

  if (!bobgui_event_controller_focus_contains_focus (BOBGUI_EVENT_CONTROLLER_FOCUS (controller)))
    bobgui_editable_label_stop_editing (self, TRUE);

  self->stop_editing_soon_id = 0;

  return FALSE;
}

static void
bobgui_editable_label_focus_out (BobguiEventController *controller,
                              BobguiEditableLabel   *self)
{
  if (!bobgui_editable_label_get_editing (self))
    return;

  if (self->stop_editing_soon_id == 0)
    self->stop_editing_soon_id = g_timeout_add (100, stop_editing_soon, controller);
}

static void
bobgui_editable_label_init (BobguiEditableLabel *self)
{
  BobguiGesture *gesture;
  BobguiDropTarget *target;
  BobguiDragSource *source;
  BobguiEventController *controller;
  GMenuModel *model;

  bobgui_widget_set_focusable (BOBGUI_WIDGET (self), TRUE);

  self->stack = bobgui_stack_new ();
  self->label = bobgui_label_new ("");
  bobgui_label_set_xalign (BOBGUI_LABEL (self->label), 0.0);
  self->entry = bobgui_text_new ();

  model = create_menu ();
  self->popup_menu = bobgui_popover_menu_new_from_model (model);
  bobgui_widget_set_parent (self->popup_menu, BOBGUI_WIDGET (self));
  bobgui_popover_set_position (BOBGUI_POPOVER (self->popup_menu), BOBGUI_POS_BOTTOM);

  bobgui_popover_set_has_arrow (BOBGUI_POPOVER (self->popup_menu), FALSE);
  bobgui_widget_set_halign (self->popup_menu, BOBGUI_ALIGN_START);

  bobgui_accessible_update_property (BOBGUI_ACCESSIBLE (self->popup_menu),
                                  BOBGUI_ACCESSIBLE_PROPERTY_LABEL, _("Context menu"),
                                  -1);

  g_object_unref (model);

  bobgui_stack_add_named (BOBGUI_STACK (self->stack), self->label, "label");
  bobgui_stack_add_named (BOBGUI_STACK (self->stack), self->entry, "entry");

  bobgui_widget_set_parent (self->stack, BOBGUI_WIDGET (self));

  gesture = bobgui_gesture_click_new ();
  bobgui_gesture_single_set_button (BOBGUI_GESTURE_SINGLE (gesture), 0);
  g_signal_connect (gesture, "pressed", G_CALLBACK (clicked_cb), self);
  bobgui_widget_add_controller (self->label, BOBGUI_EVENT_CONTROLLER (gesture));

  g_signal_connect_swapped (self->entry, "activate", G_CALLBACK (activate_cb), self);
  g_signal_connect_swapped (self->entry, "notify::text", G_CALLBACK (text_changed), self);

  target = bobgui_drop_target_new (G_TYPE_STRING, GDK_ACTION_COPY | GDK_ACTION_MOVE);
  g_signal_connect (target, "accept", G_CALLBACK (bobgui_editable_label_drag_accept), self);
  g_signal_connect (target, "drop", G_CALLBACK (bobgui_editable_label_drag_drop), self);
  bobgui_widget_add_controller (self->label, BOBGUI_EVENT_CONTROLLER (target));

  source = bobgui_drag_source_new ();
  g_signal_connect (source, "prepare", G_CALLBACK (bobgui_editable_label_prepare_drag), self);
  bobgui_widget_add_controller (self->label, BOBGUI_EVENT_CONTROLLER (source));

  controller = bobgui_event_controller_focus_new ();
  g_signal_connect (controller, "leave", G_CALLBACK (bobgui_editable_label_focus_out), self);
  bobgui_widget_add_controller (BOBGUI_WIDGET (self), controller);

  bobgui_editable_init_delegate (BOBGUI_EDITABLE (self));
}

static gboolean
bobgui_editable_label_grab_focus (BobguiWidget *widget)
{
  BobguiEditableLabel *self = BOBGUI_EDITABLE_LABEL (widget);

  if (bobgui_editable_label_get_editing (self))
    return bobgui_widget_grab_focus (self->entry);
  else
    return bobgui_widget_grab_focus_self (widget);
}

static void
bobgui_editable_label_set_property (GObject      *object,
                                 guint         prop_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  BobguiEditableLabel *self = BOBGUI_EDITABLE_LABEL (object);

  if (bobgui_editable_delegate_set_property (object, prop_id, value, pspec))
    {
      switch (prop_id)
        {
        case NUM_PROPERTIES + BOBGUI_EDITABLE_PROP_TEXT:
          bobgui_label_set_label (BOBGUI_LABEL (self->label), g_value_get_string (value));
          break;

        case NUM_PROPERTIES + BOBGUI_EDITABLE_PROP_WIDTH_CHARS:
          bobgui_label_set_width_chars (BOBGUI_LABEL (self->label), g_value_get_int (value));
          break;

        case NUM_PROPERTIES + BOBGUI_EDITABLE_PROP_MAX_WIDTH_CHARS:
          bobgui_label_set_max_width_chars (BOBGUI_LABEL (self->label), g_value_get_int (value));
          break;

        case NUM_PROPERTIES + BOBGUI_EDITABLE_PROP_XALIGN:
          bobgui_label_set_xalign (BOBGUI_LABEL (self->label), g_value_get_float (value));
          break;

        case NUM_PROPERTIES + BOBGUI_EDITABLE_PROP_EDITABLE:
          {
            gboolean editable;

            editable = g_value_get_boolean (value);
            if (!editable)
              bobgui_editable_label_stop_editing (self, FALSE);

            bobgui_widget_action_set_enabled (BOBGUI_WIDGET (self), "editing.start", editable);
            bobgui_widget_action_set_enabled (BOBGUI_WIDGET (self), "editing.stop", editable);
          }
          break;

         default: ;
        }
      return;
    }

  switch (prop_id)
    {
    case PROP_EDITING:
      if (g_value_get_boolean (value))
        bobgui_editable_label_start_editing (self);
      else
        bobgui_editable_label_stop_editing (self, FALSE);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bobgui_editable_label_get_property (GObject     *object,
                                 guint        prop_id,
                                 GValue      *value,
                                 GParamSpec  *pspec)
{
  BobguiEditableLabel *self = BOBGUI_EDITABLE_LABEL (object);

  if (bobgui_editable_delegate_get_property (object, prop_id, value, pspec))
    return;

  switch (prop_id)
    {
    case PROP_EDITING:
      g_value_set_boolean (value, bobgui_editable_label_get_editing (self));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bobgui_editable_label_dispose (GObject *object)
{
  BobguiEditableLabel *self = BOBGUI_EDITABLE_LABEL (object);

  bobgui_editable_finish_delegate (BOBGUI_EDITABLE (self));

  g_clear_pointer (&self->popup_menu, bobgui_widget_unparent);
  g_clear_pointer (&self->stack, bobgui_widget_unparent);

  self->entry = NULL;
  self->label = NULL;

  g_clear_handle_id (&self->stop_editing_soon_id, g_source_remove);

  G_OBJECT_CLASS (bobgui_editable_label_parent_class)->dispose (object);
}

static void
bobgui_editable_label_class_init (BobguiEditableLabelClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (class);
  BobguiShortcut *shortcut;
  BobguiShortcutTrigger *trigger;
  BobguiShortcutAction *action;

  object_class->set_property = bobgui_editable_label_set_property;
  object_class->get_property = bobgui_editable_label_get_property;
  object_class->dispose = bobgui_editable_label_dispose;

  widget_class->grab_focus = bobgui_editable_label_grab_focus;

  /**
   * BobguiEditableLabel:editing:
   *
   * This property is %TRUE while the widget is in edit mode.
   */
  properties[PROP_EDITING] =
    g_param_spec_boolean ("editing", NULL, NULL,
                          FALSE,
                          BOBGUI_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, NUM_PROPERTIES, properties);

  bobgui_editable_install_properties (object_class, NUM_PROPERTIES);

  /**
   * BobguiEditableLabel|editing.start:
   *
   * Switch the widget into editing mode, so that the
   * user can make changes to the text.
   *
   * The default bindings for this action are clicking
   * on the widget and the <kbd>Enter</kbd> key.
   *
   * This action is disabled when `BobguiEditableLabel:editing`
   * is %TRUE.
   */
  bobgui_widget_class_install_action (widget_class, "editing.start", NULL, start_editing);

  /**
   * BobguiEditableLabel|editing.stop:
   * @commit: Whether the make changes permanent
   *
   * Switch the widget out of editing mode. If @commit
   * is %TRUE, then the results of the editing are taken
   * as the new value of `BobguiEditable:text`.
   *
   * The default binding for this action is the Escape
   * key.
   *
   * This action is disabled when `BobguiEditableLabel:editing`
   * is %FALSE.
   */
  bobgui_widget_class_install_action (widget_class, "editing.stop", "b", stop_editing);

  trigger = bobgui_alternative_trigger_new (
                bobgui_alternative_trigger_new (
                    bobgui_keyval_trigger_new (GDK_KEY_Return, 0),
                    bobgui_keyval_trigger_new (GDK_KEY_ISO_Enter, 0)),
                    bobgui_keyval_trigger_new (GDK_KEY_KP_Enter, 0));
  action = bobgui_named_action_new ("editing.start");
  shortcut = bobgui_shortcut_new (trigger, action);
  bobgui_widget_class_add_shortcut (widget_class, shortcut);
  g_object_unref (shortcut);

  bobgui_widget_class_add_binding_action (widget_class,
                                       GDK_KEY_Escape, 0,
                                       "editing.stop",
                                       "b", FALSE);

  bobgui_widget_class_install_action (widget_class, "clipboard.copy", NULL, clipboard_copy);
  bobgui_widget_class_install_action (widget_class, "menu.popup", NULL, popup_menu);

  bobgui_widget_class_add_binding_action (widget_class,
                                       GDK_KEY_F10, GDK_SHIFT_MASK,
                                       "menu.popup",
                                       NULL);
  bobgui_widget_class_add_binding_action (widget_class,
                                       GDK_KEY_Menu, 0,
                                       "menu.popup",
                                       NULL);

  bobgui_widget_class_set_layout_manager_type (widget_class, BOBGUI_TYPE_BIN_LAYOUT);
  bobgui_widget_class_set_css_name (widget_class, "editablelabel");
}

/**
 * bobgui_editable_label_new:
 * @str: the text for the label
 *
 * Creates a new `BobguiEditableLabel` widget.
 *
 * Returns: the new `BobguiEditableLabel`
 */
BobguiWidget *
bobgui_editable_label_new (const char *str)
{
  return g_object_new (BOBGUI_TYPE_EDITABLE_LABEL,
                       "text", str,
                       NULL);
}

/**
 * bobgui_editable_label_get_editing:
 * @self: a `BobguiEditableLabel`
 *
 * Returns whether the label is currently in “editing mode”.
 *
 * Returns: %TRUE if @self is currently in editing mode
 */
gboolean
bobgui_editable_label_get_editing (BobguiEditableLabel *self)
{
  g_return_val_if_fail (BOBGUI_IS_EDITABLE_LABEL (self), FALSE);

  return bobgui_stack_get_visible_child (BOBGUI_STACK (self->stack)) == self->entry;
}

/**
 * bobgui_editable_label_start_editing:
 * @self: a `BobguiEditableLabel`
 *
 * Switches the label into “editing mode”.
 */
void
bobgui_editable_label_start_editing (BobguiEditableLabel *self)
{
  g_return_if_fail (BOBGUI_IS_EDITABLE_LABEL (self));

  if (bobgui_editable_label_get_editing (self))
    return;

  bobgui_stack_set_visible_child_name (BOBGUI_STACK (self->stack), "entry");
  bobgui_widget_grab_focus (self->entry);

  bobgui_widget_add_css_class (BOBGUI_WIDGET (self), "editing");

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_EDITING]);
}

/**
 * bobgui_editable_label_stop_editing:
 * @self: a `BobguiEditableLabel`
 * @commit: whether to set the edited text on the label
 *
 * Switches the label out of “editing mode”.
 *
 * If @commit is %TRUE, the resulting text is kept as the
 * [property@Bobgui.Editable:text] property value, otherwise the
 * resulting text is discarded and the label will keep its
 * previous [property@Bobgui.Editable:text] property value.
 */
void
bobgui_editable_label_stop_editing (BobguiEditableLabel *self,
                                 gboolean          commit)
{
  g_return_if_fail (BOBGUI_IS_EDITABLE_LABEL (self));

  if (!bobgui_editable_label_get_editing (self))
    return;

  if (commit)
    {
      bobgui_label_set_label (BOBGUI_LABEL (self->label),
                           bobgui_editable_get_text (BOBGUI_EDITABLE (self->entry)));
      bobgui_stack_set_visible_child_name (BOBGUI_STACK (self->stack), "label");
    }
  else
    {
      bobgui_stack_set_visible_child_name (BOBGUI_STACK (self->stack), "label");
      bobgui_editable_set_text (BOBGUI_EDITABLE (self->entry),
                             bobgui_label_get_label (BOBGUI_LABEL (self->label)));
    }

  bobgui_widget_grab_focus (BOBGUI_WIDGET (self));

  bobgui_widget_remove_css_class (BOBGUI_WIDGET (self), "editing");

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_EDITING]);
}
