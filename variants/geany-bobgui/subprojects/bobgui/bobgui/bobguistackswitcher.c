/*
 * Copyright (c) 2013 Red Hat, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include "config.h"

#include "bobguistackswitcher.h"

#include "bobguiboxlayout.h"
#include "bobguidropcontrollermotion.h"
#include "bobguiimage.h"
#include "bobguilabel.h"
#include "bobguiorientable.h"
#include "bobguiprivate.h"
#include "bobguiselectionmodel.h"
#include "bobguitogglebutton.h"
#include "bobguitypebuiltins.h"
#include "bobguiwidgetprivate.h"

/**
 * BobguiStackSwitcher:
 *
 * Shows a row of buttons to switch between `BobguiStack` pages.
 *
 * <picture>
 *   <source srcset="stackswitcher-dark.png" media="(prefers-color-scheme: dark)">
 *   <img alt="An example BobguiStackSwitcher" src="stackswitcher.png">
 * </picture>
 *
 * It acts as a controller for the associated `BobguiStack`.
 *
 * All the content for the buttons comes from the properties of the stacks
 * [class@Bobgui.StackPage] objects; the button visibility in a `BobguiStackSwitcher`
 * widget is controlled by the visibility of the child in the `BobguiStack`.
 *
 * It is possible to associate multiple `BobguiStackSwitcher` widgets
 * with the same `BobguiStack` widget.
 *
 * # CSS nodes
 *
 * `BobguiStackSwitcher` has a single CSS node named stackswitcher and
 * style class .stack-switcher.
 *
 * When circumstances require it, `BobguiStackSwitcher` adds the
 * .needs-attention style class to the widgets representing the
 * stack pages.
 *
 * # Accessibility
 *
 * `BobguiStackSwitcher` uses the [enum@Bobgui.AccessibleRole.tab_list] role
 * and uses the [enum@Bobgui.AccessibleRole.tab] role for its buttons.
 *
 * # Orientable
 *
 * Since BOBGUI 4.4, `BobguiStackSwitcher` implements `BobguiOrientable` allowing
 * the stack switcher to be made vertical with
 * `bobgui_orientable_set_orientation()`.
 */

#define TIMEOUT_EXPAND 500

typedef struct _BobguiStackSwitcherClass   BobguiStackSwitcherClass;

struct _BobguiStackSwitcher
{
  BobguiWidget parent_instance;

  BobguiStack *stack;
  BobguiSelectionModel *pages;
  GHashTable *buttons;
};

struct _BobguiStackSwitcherClass
{
  BobguiWidgetClass parent_class;
};

enum {
  PROP_0,
  PROP_STACK,
  PROP_ORIENTATION
};

G_DEFINE_TYPE_WITH_CODE (BobguiStackSwitcher, bobgui_stack_switcher, BOBGUI_TYPE_WIDGET,
                         G_IMPLEMENT_INTERFACE (BOBGUI_TYPE_ORIENTABLE, NULL))

static void
bobgui_stack_switcher_init (BobguiStackSwitcher *switcher)
{
  switcher->buttons = g_hash_table_new_full (g_direct_hash, g_direct_equal, g_object_unref, NULL);

  bobgui_widget_add_css_class (BOBGUI_WIDGET (switcher), "linked");
}

static void
on_button_toggled (BobguiWidget        *button,
                   GParamSpec       *pspec,
                   BobguiStackSwitcher *self)
{
  gboolean active;
  guint index;

  active = bobgui_toggle_button_get_active (BOBGUI_TOGGLE_BUTTON (button));
  index = GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (button), "child-index"));

  if (active)
    {
      bobgui_selection_model_select_item (self->pages, index, TRUE);
    }
  else
    {
      gboolean selected = bobgui_selection_model_is_selected (self->pages, index);
      bobgui_toggle_button_set_active (BOBGUI_TOGGLE_BUTTON (button), selected);
    }
}

static void
rebuild_child (BobguiWidget   *self,
               const char *icon_name,
               const char *title,
               gboolean     use_underline)
{
  BobguiWidget *button_child;

  button_child = NULL;

  if (icon_name != NULL)
    {
      button_child = bobgui_image_new_from_icon_name (icon_name);
      if (title != NULL)
        bobgui_widget_set_tooltip_text (BOBGUI_WIDGET (self), title);

      bobgui_widget_remove_css_class (self, "text-button");
      bobgui_widget_add_css_class (self, "image-button");
    }
  else if (title != NULL)
    {
      button_child = bobgui_label_new (title);
      bobgui_label_set_use_underline (BOBGUI_LABEL (button_child), use_underline);

      bobgui_widget_set_tooltip_text (BOBGUI_WIDGET (self), NULL);

      bobgui_widget_remove_css_class (self, "image-button");
      bobgui_widget_add_css_class (self, "text-button");
    }

  if (button_child)
    {
      bobgui_widget_set_halign (BOBGUI_WIDGET (button_child), BOBGUI_ALIGN_CENTER);
      bobgui_button_set_child (BOBGUI_BUTTON (self), button_child);
    }

  bobgui_accessible_update_property (BOBGUI_ACCESSIBLE (self),
                                  BOBGUI_ACCESSIBLE_PROPERTY_LABEL, title,
                                  -1);
}

static void
update_button (BobguiStackSwitcher *self,
               BobguiStackPage     *page,
               BobguiWidget        *button)
{
  char *title;
  char *icon_name;
  gboolean needs_attention;
  gboolean visible;
  gboolean use_underline;

  g_object_get (page,
                "title", &title,
                "icon-name", &icon_name,
                "needs-attention", &needs_attention,
                "visible", &visible,
                "use-underline", &use_underline,
                NULL);

  rebuild_child (button, icon_name, title, use_underline);

  bobgui_widget_set_visible (button, visible && (title != NULL || icon_name != NULL));

  if (needs_attention)
    bobgui_widget_add_css_class (button, "needs-attention");
  else
    bobgui_widget_remove_css_class (button, "needs-attention");

  g_free (title);
  g_free (icon_name);
}

static void
on_page_updated (BobguiStackPage     *page,
                 GParamSpec       *pspec,
                 BobguiStackSwitcher *self)
{
  BobguiWidget *button;

  button = g_hash_table_lookup (self->buttons, page);
  update_button (self, page, button);
}

static gboolean
bobgui_stack_switcher_switch_timeout (gpointer data)
{
  BobguiWidget *button = data;

  g_object_steal_data (G_OBJECT (button), "-bobgui-switch-timer");

  if (button)
    bobgui_toggle_button_set_active (BOBGUI_TOGGLE_BUTTON (button), TRUE);

  return G_SOURCE_REMOVE;
}

static void
clear_timer (gpointer data)
{
  if (data)
    g_source_remove (GPOINTER_TO_UINT (data));
}

static void
bobgui_stack_switcher_drag_enter (BobguiDropControllerMotion *motion,
                               double                   x,
                               double                   y,
                               gpointer                 unused)
{
  BobguiWidget *button = bobgui_event_controller_get_widget (BOBGUI_EVENT_CONTROLLER (motion));

  if (!bobgui_toggle_button_get_active (BOBGUI_TOGGLE_BUTTON (button)))
    {
      guint switch_timer = g_timeout_add (TIMEOUT_EXPAND,
                                          bobgui_stack_switcher_switch_timeout,
                                          button);
      gdk_source_set_static_name_by_id (switch_timer, "[bobgui] bobgui_stack_switcher_switch_timeout");
      g_object_set_data_full (G_OBJECT (button), "-bobgui-switch-timer", GUINT_TO_POINTER (switch_timer), clear_timer);
    }
}

static void
bobgui_stack_switcher_drag_leave (BobguiDropControllerMotion *motion,
                               gpointer                 unused)
{
  BobguiWidget *button = bobgui_event_controller_get_widget (BOBGUI_EVENT_CONTROLLER (motion));
  guint switch_timer;

  switch_timer = GPOINTER_TO_UINT (g_object_steal_data (G_OBJECT (button), "-bobgui-switch-timer"));
  if (switch_timer)
    g_source_remove (switch_timer);
}

static void
add_child (guint             position,
           BobguiStackSwitcher *self)
{
  BobguiWidget *button;
  gboolean selected;
  BobguiStackPage *page;
  BobguiEventController *controller;

  button = g_object_new (BOBGUI_TYPE_TOGGLE_BUTTON,
                         "accessible-role", BOBGUI_ACCESSIBLE_ROLE_TAB,
                         "hexpand", TRUE,
                         "vexpand", TRUE,
                         NULL);
  bobgui_widget_set_focus_on_click (button, FALSE);

  controller = bobgui_drop_controller_motion_new ();
  g_signal_connect (controller, "enter", G_CALLBACK (bobgui_stack_switcher_drag_enter), NULL);
  g_signal_connect (controller, "leave", G_CALLBACK (bobgui_stack_switcher_drag_leave), NULL);
  bobgui_widget_add_controller (button, controller);

  page = g_list_model_get_item (G_LIST_MODEL (self->pages), position);
  update_button (self, page, button);

  bobgui_widget_set_parent (button, BOBGUI_WIDGET (self));

  g_object_set_data (G_OBJECT (button), "child-index", GUINT_TO_POINTER (position));
  selected = bobgui_selection_model_is_selected (self->pages, position);
  bobgui_toggle_button_set_active (BOBGUI_TOGGLE_BUTTON (button), selected);

  bobgui_accessible_update_state (BOBGUI_ACCESSIBLE (button),
                               BOBGUI_ACCESSIBLE_STATE_SELECTED, selected,
                               -1);

  bobgui_accessible_update_relation (BOBGUI_ACCESSIBLE (button),
                                  BOBGUI_ACCESSIBLE_RELATION_CONTROLS, page, NULL,
                                  -1);

  g_signal_connect (button, "notify::active", G_CALLBACK (on_button_toggled), self);
  g_signal_connect (page, "notify", G_CALLBACK (on_page_updated), self);

  g_hash_table_insert (self->buttons, g_object_ref (page), button);

  g_object_unref (page);
}

static void
populate_switcher (BobguiStackSwitcher *self)
{
  guint i;

  for (i = 0; i < g_list_model_get_n_items (G_LIST_MODEL (self->pages)); i++)
    add_child (i, self);
}

static void
clear_switcher (BobguiStackSwitcher *self)
{
  GHashTableIter iter;
  BobguiWidget *page;
  BobguiWidget *button;

  g_hash_table_iter_init (&iter, self->buttons);
  while (g_hash_table_iter_next (&iter, (gpointer *)&page, (gpointer *)&button))
    {
      bobgui_widget_unparent (button);
      g_signal_handlers_disconnect_by_func (page, on_page_updated, self);
      g_hash_table_iter_remove (&iter);
    }
}

static void
items_changed_cb (GListModel       *model,
                  guint             position,
                  guint             removed,
                  guint             added,
                  BobguiStackSwitcher *switcher)
{
  clear_switcher (switcher);
  populate_switcher (switcher);
}

static void
selection_changed_cb (BobguiSelectionModel *model,
                      guint              position,
                      guint              n_items,
                      BobguiStackSwitcher  *switcher)
{
  guint i;

  for (i = position; i < position + n_items; i++)
    {
      BobguiStackPage *page;
      BobguiWidget *button;
      gboolean selected;

      page = g_list_model_get_item (G_LIST_MODEL (switcher->pages), i);
      button = g_hash_table_lookup (switcher->buttons, page);
      if (button)
        {
          selected = bobgui_selection_model_is_selected (switcher->pages, i);
          bobgui_toggle_button_set_active (BOBGUI_TOGGLE_BUTTON (button), selected);

          bobgui_accessible_update_state (BOBGUI_ACCESSIBLE (button),
                                       BOBGUI_ACCESSIBLE_STATE_SELECTED, selected,
                                       -1);
        }
      g_object_unref (page);
    }
}

static void
disconnect_stack_signals (BobguiStackSwitcher *switcher)
{
  g_signal_handlers_disconnect_by_func (switcher->pages, items_changed_cb, switcher);
  g_signal_handlers_disconnect_by_func (switcher->pages, selection_changed_cb, switcher);
}

static void
connect_stack_signals (BobguiStackSwitcher *switcher)
{
  g_signal_connect (switcher->pages, "items-changed", G_CALLBACK (items_changed_cb), switcher);
  g_signal_connect (switcher->pages, "selection-changed", G_CALLBACK (selection_changed_cb), switcher);
}

static void
set_stack (BobguiStackSwitcher *switcher,
           BobguiStack         *stack)
{
  if (stack)
    {
      switcher->stack = g_object_ref (stack);
      switcher->pages = bobgui_stack_get_pages (stack);
      populate_switcher (switcher);
      connect_stack_signals (switcher);
    }
}

static void
unset_stack (BobguiStackSwitcher *switcher)
{
  if (switcher->stack)
    {
      disconnect_stack_signals (switcher);
      clear_switcher (switcher);
      g_clear_object (&switcher->stack);
      g_clear_object (&switcher->pages);
    }
}

/**
 * bobgui_stack_switcher_set_stack:
 * @switcher: a `BobguiStackSwitcher`
 * @stack: (nullable): a `BobguiStack`
 *
 * Sets the stack to control.
 */
void
bobgui_stack_switcher_set_stack (BobguiStackSwitcher *switcher,
                              BobguiStack         *stack)
{
  g_return_if_fail (BOBGUI_IS_STACK_SWITCHER (switcher));
  g_return_if_fail (BOBGUI_IS_STACK (stack) || stack == NULL);

  if (switcher->stack == stack)
    return;

  unset_stack (switcher);
  set_stack (switcher, stack);

  bobgui_widget_queue_resize (BOBGUI_WIDGET (switcher));

  g_object_notify (G_OBJECT (switcher), "stack");
}

/**
 * bobgui_stack_switcher_get_stack:
 * @switcher: a `BobguiStackSwitcher`
 *
 * Retrieves the stack.
 *
 * Returns: (nullable) (transfer none): the stack
 */
BobguiStack *
bobgui_stack_switcher_get_stack (BobguiStackSwitcher *switcher)
{
  g_return_val_if_fail (BOBGUI_IS_STACK_SWITCHER (switcher), NULL);

  return switcher->stack;
}

static void
bobgui_stack_switcher_get_property (GObject      *object,
                                 guint         prop_id,
                                 GValue       *value,
                                 GParamSpec   *pspec)
{
  BobguiStackSwitcher *switcher = BOBGUI_STACK_SWITCHER (object);
  BobguiLayoutManager *box_layout = bobgui_widget_get_layout_manager (BOBGUI_WIDGET (switcher));

  switch (prop_id)
    {
    case PROP_ORIENTATION:
      g_value_set_enum (value, bobgui_orientable_get_orientation (BOBGUI_ORIENTABLE (box_layout)));
      break;

    case PROP_STACK:
      g_value_set_object (value, switcher->stack);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bobgui_stack_switcher_set_property (GObject      *object,
                                 guint         prop_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  BobguiStackSwitcher *switcher = BOBGUI_STACK_SWITCHER (object);
  BobguiLayoutManager *box_layout = bobgui_widget_get_layout_manager (BOBGUI_WIDGET (switcher));

  switch (prop_id)
    {
    case PROP_ORIENTATION:
      {
        BobguiOrientation orientation = g_value_get_enum (value);
        if (bobgui_orientable_get_orientation (BOBGUI_ORIENTABLE (box_layout)) != orientation)
          {
            bobgui_orientable_set_orientation (BOBGUI_ORIENTABLE (box_layout), orientation);
            bobgui_widget_update_orientation (BOBGUI_WIDGET (switcher), orientation);
            g_object_notify_by_pspec (object, pspec);
          }
      }
      break;

    case PROP_STACK:
      bobgui_stack_switcher_set_stack (switcher, g_value_get_object (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bobgui_stack_switcher_dispose (GObject *object)
{
  BobguiStackSwitcher *switcher = BOBGUI_STACK_SWITCHER (object);

  unset_stack (switcher);

  G_OBJECT_CLASS (bobgui_stack_switcher_parent_class)->dispose (object);
}

static void
bobgui_stack_switcher_finalize (GObject *object)
{
  BobguiStackSwitcher *switcher = BOBGUI_STACK_SWITCHER (object);

  g_hash_table_destroy (switcher->buttons);

  G_OBJECT_CLASS (bobgui_stack_switcher_parent_class)->finalize (object);
}

static void
bobgui_stack_switcher_class_init (BobguiStackSwitcherClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (class);

  object_class->get_property = bobgui_stack_switcher_get_property;
  object_class->set_property = bobgui_stack_switcher_set_property;
  object_class->dispose = bobgui_stack_switcher_dispose;
  object_class->finalize = bobgui_stack_switcher_finalize;

  /**
   * BobguiStackSwitcher:stack:
   *
   * The stack.
   */
  g_object_class_install_property (object_class,
                                   PROP_STACK,
                                   g_param_spec_object ("stack", NULL, NULL,
                                                        BOBGUI_TYPE_STACK,
                                                        BOBGUI_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_override_property (object_class, PROP_ORIENTATION, "orientation");

  bobgui_widget_class_set_layout_manager_type (widget_class, BOBGUI_TYPE_BOX_LAYOUT);
  bobgui_widget_class_set_css_name (widget_class, I_("stackswitcher"));
  bobgui_widget_class_set_accessible_role (widget_class, BOBGUI_ACCESSIBLE_ROLE_TAB_LIST);
}

/**
 * bobgui_stack_switcher_new:
 *
 * Create a new `BobguiStackSwitcher`.
 *
 * Returns: a new `BobguiStackSwitcher`.
 */
BobguiWidget *
bobgui_stack_switcher_new (void)
{
  return g_object_new (BOBGUI_TYPE_STACK_SWITCHER, NULL);
}
