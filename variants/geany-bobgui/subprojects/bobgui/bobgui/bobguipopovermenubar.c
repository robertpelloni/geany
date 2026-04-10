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

/**
 * BobguiPopoverMenuBar:
 *
 * Presents a horizontal bar of items that pop up menus when clicked.
 *
 * <picture>
 *   <source srcset="menubar-dark.png" media="(prefers-color-scheme: dark)">
 *   <img alt="An example BobguiPopoverMenuBar" src="menubar.png">
 * </picture>
 *
 * The only way to create instances of `BobguiPopoverMenuBar` is
 * from a `GMenuModel`.
 *
 * # CSS nodes
 *
 * ```
 * menubar
 * ├── item[.active]
 * ┊   ╰── popover
 * ╰── item
 *     ╰── popover
 * ```
 *
 * `BobguiPopoverMenuBar` has a single CSS node with name menubar, below which
 * each item has its CSS node, and below that the corresponding popover.
 *
 * The item whose popover is currently open gets the .active
 * style class.
 *
 * # Accessibility
 *
 * `BobguiPopoverMenuBar` uses the [enum@Bobgui.AccessibleRole.menu_bar] role,
 * the menu items use the [enum@Bobgui.AccessibleRole.menu_item] role and
 * the menus use the [enum@Bobgui.AccessibleRole.menu] role.
 */


#include "config.h"

#include "bobguipopovermenubar.h"
#include "bobguipopovermenubarprivate.h"
#include "bobguipopovermenu.h"

#include "bobguibinlayout.h"
#include "bobguiboxlayout.h"
#include "bobguilabel.h"
#include "bobguimenubutton.h"
#include "bobguiprivate.h"
#include "bobguimarshalers.h"
#include "bobguigestureclick.h"
#include "bobguieventcontrollermotion.h"
#include "bobguiactionmuxerprivate.h"
#include "bobguimenutrackerprivate.h"
#include "bobguiwidgetprivate.h"
#include "bobguimain.h"
#include "bobguinative.h"
#include "bobguibuildable.h"

#define BOBGUI_TYPE_POPOVER_MENU_BAR_ITEM    (bobgui_popover_menu_bar_item_get_type ())
#define BOBGUI_POPOVER_MENU_BAR_ITEM(obj)    (G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_POPOVER_MENU_BAR_ITEM, BobguiPopoverMenuBarItem))
#define BOBGUI_IS_POPOVER_MENU_BAR_ITEM(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOBGUI_TYPE_POPOVER_MENU_BAR_ITEM))

GType bobgui_popover_menu_bar_item_get_type (void) G_GNUC_CONST;

typedef struct _BobguiPopoverMenuBarItem BobguiPopoverMenuBarItem;

struct _BobguiPopoverMenuBar
{
  BobguiWidget parent;

  GMenuModel *model;
  BobguiMenuTracker *tracker;

  BobguiPopoverMenuBarItem *active_item;
};

typedef struct _BobguiPopoverMenuBarClass BobguiPopoverMenuBarClass;
struct _BobguiPopoverMenuBarClass
{
  BobguiWidgetClass parent_class;
};

struct _BobguiPopoverMenuBarItem
{
  BobguiWidget parent;

  BobguiWidget *label;
  BobguiPopover *popover;
  BobguiMenuTrackerItem *tracker;
};

typedef struct _BobguiPopoverMenuBarItemClass BobguiPopoverMenuBarItemClass;
struct _BobguiPopoverMenuBarItemClass
{
  BobguiWidgetClass parent_class;

  void (* activate) (BobguiPopoverMenuBarItem *item);
};

G_DEFINE_TYPE (BobguiPopoverMenuBarItem, bobgui_popover_menu_bar_item, BOBGUI_TYPE_WIDGET)

static void
open_submenu (BobguiPopoverMenuBarItem *item)
{
  bobgui_popover_popup (item->popover);
}

static void
close_submenu (BobguiPopoverMenuBarItem *item)
{
  bobgui_popover_popdown (item->popover);
}

static void
set_active_item (BobguiPopoverMenuBar     *bar,
                 BobguiPopoverMenuBarItem *item,
                 gboolean               popup)
{
  gboolean changed;
  gboolean was_popup;

  changed = item != bar->active_item;

  if (bar->active_item)
    was_popup = bobgui_widget_get_mapped (BOBGUI_WIDGET (bar->active_item->popover));
  else
    was_popup = FALSE;

  if (was_popup && changed)
    close_submenu (bar->active_item);

  if (changed)
    {
      if (bar->active_item)
        bobgui_widget_unset_state_flags (BOBGUI_WIDGET (bar->active_item), BOBGUI_STATE_FLAG_SELECTED);

      bar->active_item = item;

      if (bar->active_item)
        bobgui_widget_set_state_flags (BOBGUI_WIDGET (bar->active_item), BOBGUI_STATE_FLAG_SELECTED, FALSE);
    }

  if (bar->active_item)
    {
      BobguiStateFlags state = bobgui_widget_get_state_flags (BOBGUI_WIDGET (bar));

      if (popup || (was_popup && changed))
        open_submenu (bar->active_item);
      else if (changed && (state & BOBGUI_STATE_FLAG_FOCUS_WITHIN))
        bobgui_widget_grab_focus (BOBGUI_WIDGET (bar->active_item));
    }
}

static void
clicked_cb (BobguiGesture *gesture,
            int         n,
            double      x,
            double      y,
            gpointer    data)
{
  BobguiWidget *target;
  BobguiPopoverMenuBar *bar;

  target = bobgui_event_controller_get_widget (BOBGUI_EVENT_CONTROLLER (gesture));
  bar = BOBGUI_POPOVER_MENU_BAR (bobgui_widget_get_ancestor (target, BOBGUI_TYPE_POPOVER_MENU_BAR));

  set_active_item (bar, BOBGUI_POPOVER_MENU_BAR_ITEM (target), TRUE);
}

static void
item_enter_cb (BobguiEventController   *controller,
               double                x,
               double                y,
               gpointer              data)
{
  BobguiWidget *target;
  BobguiPopoverMenuBar *bar;

  target = bobgui_event_controller_get_widget (controller);
  bar = BOBGUI_POPOVER_MENU_BAR (bobgui_widget_get_ancestor (target, BOBGUI_TYPE_POPOVER_MENU_BAR));

  set_active_item (bar, BOBGUI_POPOVER_MENU_BAR_ITEM (target), FALSE);
}

static void
bar_leave_cb (BobguiEventController   *controller,
              gpointer              data)
{
  BobguiWidget *target;
  BobguiPopoverMenuBar *bar;

  target = bobgui_event_controller_get_widget (controller);
  bar = BOBGUI_POPOVER_MENU_BAR (bobgui_widget_get_ancestor (target, BOBGUI_TYPE_POPOVER_MENU_BAR));

  if (bar->active_item &&
      !bobgui_widget_get_mapped (BOBGUI_WIDGET (bar->active_item->popover)))
    set_active_item (bar, NULL, FALSE);
}

static gboolean
bobgui_popover_menu_bar_focus (BobguiWidget        *widget,
                            BobguiDirectionType  direction)
{
  BobguiPopoverMenuBar *bar = BOBGUI_POPOVER_MENU_BAR (widget);
  BobguiWidget *next;

  if (bar->active_item &&
      bobgui_widget_get_mapped (BOBGUI_WIDGET (bar->active_item->popover)))
    {
      if (bobgui_widget_child_focus (BOBGUI_WIDGET (bar->active_item->popover), direction))
        return TRUE;
    }

  if (direction == BOBGUI_DIR_LEFT)
    {
      if (bar->active_item)
        next = bobgui_widget_get_prev_sibling (BOBGUI_WIDGET (bar->active_item));
      else
        next = NULL;

      if (next == NULL)
        next = bobgui_widget_get_last_child (BOBGUI_WIDGET (bar));
    }
  else if (direction == BOBGUI_DIR_RIGHT)
    {
      if (bar->active_item)
        next = bobgui_widget_get_next_sibling (BOBGUI_WIDGET (bar->active_item));
      else
        next = NULL;

      if (next == NULL)
        next = bobgui_widget_get_first_child (BOBGUI_WIDGET (bar));
    }
  else
    return FALSE;

  set_active_item (bar, BOBGUI_POPOVER_MENU_BAR_ITEM (next), FALSE);

  return TRUE;
}

static void
bobgui_popover_menu_bar_item_init (BobguiPopoverMenuBarItem *item)
{
  BobguiEventController *controller;

  bobgui_widget_set_focusable (BOBGUI_WIDGET (item), TRUE);

  item->label = g_object_new (BOBGUI_TYPE_LABEL,
                              "use-underline", TRUE,
                              NULL);
  bobgui_widget_set_parent (item->label, BOBGUI_WIDGET (item));

  controller = BOBGUI_EVENT_CONTROLLER (bobgui_gesture_click_new ());
  g_signal_connect (controller, "pressed", G_CALLBACK (clicked_cb), NULL);
  bobgui_widget_add_controller (BOBGUI_WIDGET (item), controller);

  controller = bobgui_event_controller_motion_new ();
  bobgui_event_controller_set_propagation_limit (controller, BOBGUI_LIMIT_NONE);
  g_signal_connect (controller, "enter", G_CALLBACK (item_enter_cb), NULL);
  bobgui_widget_add_controller (BOBGUI_WIDGET (item), controller);
}

static void
bobgui_popover_menu_bar_item_dispose (GObject *object)
{
  BobguiPopoverMenuBarItem *item = BOBGUI_POPOVER_MENU_BAR_ITEM (object);

  g_clear_object (&item->tracker);
  g_clear_pointer (&item->label, bobgui_widget_unparent);
  g_clear_pointer ((BobguiWidget **)&item->popover, bobgui_widget_unparent);

  G_OBJECT_CLASS (bobgui_popover_menu_bar_item_parent_class)->dispose (object);
}

static void
bobgui_popover_menu_bar_item_finalize (GObject *object)
{
  G_OBJECT_CLASS (bobgui_popover_menu_bar_item_parent_class)->finalize (object);
}

static void
bobgui_popover_menu_bar_item_activate (BobguiPopoverMenuBarItem *item)
{
  BobguiPopoverMenuBar *bar;

  bar = BOBGUI_POPOVER_MENU_BAR (bobgui_widget_get_ancestor (BOBGUI_WIDGET (item), BOBGUI_TYPE_POPOVER_MENU_BAR));

  set_active_item (bar, item, TRUE);
}

static void
bobgui_popover_menu_bar_item_root (BobguiWidget *widget)
{
  BobguiPopoverMenuBarItem *item = BOBGUI_POPOVER_MENU_BAR_ITEM (widget);

  BOBGUI_WIDGET_CLASS (bobgui_popover_menu_bar_item_parent_class)->root (widget);

  bobgui_accessible_update_relation (BOBGUI_ACCESSIBLE (widget),
                                  BOBGUI_ACCESSIBLE_RELATION_LABELLED_BY, item->label, NULL,
                                  BOBGUI_ACCESSIBLE_RELATION_CONTROLS, item->popover, NULL,
                                  -1);
  bobgui_accessible_update_property (BOBGUI_ACCESSIBLE (widget),
                                  BOBGUI_ACCESSIBLE_PROPERTY_HAS_POPUP, TRUE,
                                  -1);
}

static void
bobgui_popover_menu_bar_item_class_init (BobguiPopoverMenuBarItemClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (klass);
  guint activate_signal;

  object_class->dispose = bobgui_popover_menu_bar_item_dispose;
  object_class->finalize = bobgui_popover_menu_bar_item_finalize;

  widget_class->root = bobgui_popover_menu_bar_item_root;

  klass->activate = bobgui_popover_menu_bar_item_activate;

  activate_signal =
    g_signal_new (I_("activate"),
                  G_OBJECT_CLASS_TYPE (object_class),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (BobguiPopoverMenuBarItemClass, activate),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 0);

  bobgui_widget_class_set_css_name (widget_class, I_("item"));
  bobgui_widget_class_set_accessible_role (widget_class, BOBGUI_ACCESSIBLE_ROLE_MENU_ITEM);
  bobgui_widget_class_set_activate_signal (widget_class, activate_signal);
  bobgui_widget_class_set_layout_manager_type (widget_class, BOBGUI_TYPE_BIN_LAYOUT);
}
enum
{
  PROP_0,
  PROP_MENU_MODEL,
  LAST_PROP
};

static GParamSpec * bar_props[LAST_PROP];

static void bobgui_popover_menu_bar_buildable_iface_init (BobguiBuildableIface *iface);

G_DEFINE_TYPE_WITH_CODE (BobguiPopoverMenuBar, bobgui_popover_menu_bar, BOBGUI_TYPE_WIDGET,
                         G_IMPLEMENT_INTERFACE (BOBGUI_TYPE_BUILDABLE,
                                                bobgui_popover_menu_bar_buildable_iface_init))

static void
tracker_remove (int      position,
                gpointer user_data)
{
  BobguiWidget *bar = user_data;
  BobguiWidget *child;
  int i;

  for (child = bobgui_widget_get_first_child (bar), i = 0;
       child;
       child = bobgui_widget_get_next_sibling (child), i++)
    {
      if (i == position)
        {
          bobgui_widget_unparent (child);
          break;
        }
    }
}

static void
popover_unmap (BobguiPopover        *popover,
               BobguiPopoverMenuBar *bar)
{
  if (bar->active_item && bar->active_item->popover == popover)
    set_active_item (bar, NULL, FALSE);
}

static void
popover_shown (BobguiPopover            *popover,
               BobguiPopoverMenuBarItem *item)
{
  bobgui_accessible_update_state (BOBGUI_ACCESSIBLE (item),
                               BOBGUI_ACCESSIBLE_STATE_EXPANDED, TRUE,
                               -1);

  if (bobgui_menu_tracker_item_get_should_request_show (item->tracker))
    bobgui_menu_tracker_item_request_submenu_shown (item->tracker, TRUE);
}

static void
popover_hidden (BobguiPopover            *popover,
                BobguiPopoverMenuBarItem *item)
{
  bobgui_accessible_update_state (BOBGUI_ACCESSIBLE (item),
                               BOBGUI_ACCESSIBLE_STATE_EXPANDED, FALSE,
                               -1);

  if (bobgui_menu_tracker_item_get_should_request_show (item->tracker))
    bobgui_menu_tracker_item_request_submenu_shown (item->tracker, FALSE);
}

static void
tracker_insert (BobguiMenuTrackerItem *item,
                int                 position,
                gpointer            user_data)
{
  BobguiPopoverMenuBar *bar = user_data;

  if (bobgui_menu_tracker_item_get_has_link (item, G_MENU_LINK_SUBMENU))
    {
      BobguiPopoverMenuBarItem *widget;
      GMenuModel *model;
      BobguiWidget *sibling;
      BobguiWidget *child;
      BobguiPopover *popover;
      int i;

      widget = g_object_new (BOBGUI_TYPE_POPOVER_MENU_BAR_ITEM, NULL);
      g_object_bind_property (item, "label",
                              widget->label, "label",
                              G_BINDING_SYNC_CREATE);

      model = _bobgui_menu_tracker_item_get_link (item, G_MENU_LINK_SUBMENU);
      popover = BOBGUI_POPOVER (bobgui_popover_menu_new_from_model_full (model, BOBGUI_POPOVER_MENU_NESTED));
      bobgui_widget_set_parent (BOBGUI_WIDGET (popover), BOBGUI_WIDGET (widget));
      bobgui_popover_set_position (popover, BOBGUI_POS_BOTTOM);
      bobgui_popover_set_has_arrow (popover, FALSE);
      bobgui_widget_set_halign (BOBGUI_WIDGET (popover), BOBGUI_ALIGN_START);

      g_signal_connect (popover, "unmap", G_CALLBACK (popover_unmap), bar);
      g_signal_connect (popover, "show", G_CALLBACK (popover_shown), widget);
      g_signal_connect (popover, "hide", G_CALLBACK (popover_hidden), widget);

      widget->popover = popover;
      widget->tracker = g_object_ref (item);

      sibling = NULL;
      for (child = bobgui_widget_get_first_child (BOBGUI_WIDGET (bar)), i = 1;
           child;
           child = bobgui_widget_get_next_sibling (child), i++)
        {
          if (i == position)
            {
              sibling = child;
              break;
            }
        }
      bobgui_widget_insert_after (BOBGUI_WIDGET (widget), BOBGUI_WIDGET (bar), sibling);
    }
  else
    g_warning ("Don't know how to handle this item");
}

static void
bobgui_popover_menu_bar_set_property (GObject      *object,
                                   guint         property_id,
                                   const GValue *value,
                                   GParamSpec   *pspec)
{
  BobguiPopoverMenuBar *bar = BOBGUI_POPOVER_MENU_BAR (object);

  switch (property_id)
    {
      case PROP_MENU_MODEL:
        bobgui_popover_menu_bar_set_menu_model (bar, g_value_get_object (value));
        break;

      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
bobgui_popover_menu_bar_get_property (GObject    *object,
                                   guint       property_id,
                                   GValue     *value,
                                   GParamSpec *pspec)
{
  BobguiPopoverMenuBar *bar = BOBGUI_POPOVER_MENU_BAR (object);

  switch (property_id)
    {
      case PROP_MENU_MODEL:
        g_value_set_object (value, bar->model);
        break;

      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
bobgui_popover_menu_bar_dispose (GObject *object)
{
  BobguiPopoverMenuBar *bar = BOBGUI_POPOVER_MENU_BAR (object);
  BobguiWidget *child;

  g_clear_pointer (&bar->tracker, bobgui_menu_tracker_free);
  g_clear_object (&bar->model);

  while ((child = bobgui_widget_get_first_child (BOBGUI_WIDGET (bar))))
    bobgui_widget_unparent (child);

  G_OBJECT_CLASS (bobgui_popover_menu_bar_parent_class)->dispose (object);
}

static GList *
get_menu_bars (BobguiWindow *window)
{
  return g_object_get_data (G_OBJECT (window), "bobgui-menu-bar-list");
}

GList *
bobgui_popover_menu_bar_get_viewable_menu_bars (BobguiWindow *window)
{
  GList *menu_bars;
  GList *viewable_menu_bars = NULL;

  for (menu_bars = get_menu_bars (window);
       menu_bars;
       menu_bars = menu_bars->next)
    {
      BobguiWidget *widget = menu_bars->data;
      gboolean viewable = TRUE;

      while (widget)
        {
          if (!bobgui_widget_get_mapped (widget))
            viewable = FALSE;

          widget = bobgui_widget_get_parent (widget);
        }

      if (viewable)
        viewable_menu_bars = g_list_prepend (viewable_menu_bars, menu_bars->data);
    }

  return g_list_reverse (viewable_menu_bars);
}

static void
set_menu_bars (BobguiWindow *window,
               GList     *menubars)
{
  g_object_set_data (G_OBJECT (window), I_("bobgui-menu-bar-list"), menubars);
}

static void
add_to_window (BobguiWindow         *window,
               BobguiPopoverMenuBar *bar)
{
  GList *menubars = get_menu_bars (window);

  set_menu_bars (window, g_list_prepend (menubars, bar));
}

static void
remove_from_window (BobguiWindow         *window,
                    BobguiPopoverMenuBar *bar)
{
  GList *menubars = get_menu_bars (window);

  menubars = g_list_remove (menubars, bar);
  set_menu_bars (window, menubars);
}

static void
bobgui_popover_menu_bar_root (BobguiWidget *widget)
{
  BobguiPopoverMenuBar *bar = BOBGUI_POPOVER_MENU_BAR (widget);
  BobguiWidget *toplevel;

  BOBGUI_WIDGET_CLASS (bobgui_popover_menu_bar_parent_class)->root (widget);

  toplevel = BOBGUI_WIDGET (bobgui_widget_get_root (widget));
  add_to_window (BOBGUI_WINDOW (toplevel), bar);

  bobgui_accessible_update_property (BOBGUI_ACCESSIBLE (bar),
                                  BOBGUI_ACCESSIBLE_PROPERTY_ORIENTATION, BOBGUI_ORIENTATION_HORIZONTAL,
                                  -1);
}

static void
bobgui_popover_menu_bar_unroot (BobguiWidget *widget)
{
  BobguiPopoverMenuBar *bar = BOBGUI_POPOVER_MENU_BAR (widget);
  BobguiWidget *toplevel;

  toplevel = BOBGUI_WIDGET (bobgui_widget_get_root (widget));
  remove_from_window (BOBGUI_WINDOW (toplevel), bar);

  BOBGUI_WIDGET_CLASS (bobgui_popover_menu_bar_parent_class)->unroot (widget);
}

static void
bobgui_popover_menu_bar_class_init (BobguiPopoverMenuBarClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (klass);

  object_class->dispose = bobgui_popover_menu_bar_dispose;
  object_class->set_property = bobgui_popover_menu_bar_set_property;
  object_class->get_property = bobgui_popover_menu_bar_get_property;

  widget_class->root = bobgui_popover_menu_bar_root;
  widget_class->unroot = bobgui_popover_menu_bar_unroot;
  widget_class->focus = bobgui_popover_menu_bar_focus;

  /**
   * BobguiPopoverMenuBar:menu-model:
   *
   * The `GMenuModel` from which the menu bar is created.
   *
   * The model should only contain submenus as toplevel elements.
   */
  bar_props[PROP_MENU_MODEL] =
      g_param_spec_object ("menu-model", NULL, NULL,
                           G_TYPE_MENU_MODEL,
                           BOBGUI_PARAM_READWRITE);

  g_object_class_install_properties (object_class, LAST_PROP, bar_props);

  bobgui_widget_class_set_layout_manager_type (widget_class, BOBGUI_TYPE_BOX_LAYOUT);
  bobgui_widget_class_set_css_name (widget_class, I_("menubar"));
  bobgui_widget_class_set_accessible_role (widget_class, BOBGUI_ACCESSIBLE_ROLE_MENU_BAR);
}

static void
bobgui_popover_menu_bar_init (BobguiPopoverMenuBar *bar)
{
  BobguiEventController *controller;

  controller = bobgui_event_controller_motion_new ();
  bobgui_event_controller_set_propagation_limit (controller, BOBGUI_LIMIT_NONE);
  g_signal_connect (controller, "leave", G_CALLBACK (bar_leave_cb), NULL);
  bobgui_widget_add_controller (BOBGUI_WIDGET (bar), controller);
}

static BobguiBuildableIface *parent_buildable_iface;

static void
bobgui_popover_menu_bar_buildable_add_child (BobguiBuildable *buildable,
                                          BobguiBuilder   *builder,
                                          GObject      *child,
                                          const char   *type)
{
  if (BOBGUI_IS_WIDGET (child))
    {
      if (!bobgui_popover_menu_bar_add_child (BOBGUI_POPOVER_MENU_BAR (buildable), BOBGUI_WIDGET (child), type))
        g_warning ("No such custom attribute: %s", type);
    }
  else
    parent_buildable_iface->add_child (buildable, builder, child, type);
}

static void
bobgui_popover_menu_bar_buildable_iface_init (BobguiBuildableIface *iface)
{
  parent_buildable_iface = g_type_interface_peek_parent (iface);

  iface->add_child = bobgui_popover_menu_bar_buildable_add_child;
}

/**
 * bobgui_popover_menu_bar_new_from_model:
 * @model: (nullable): a `GMenuModel`
 *
 * Creates a `BobguiPopoverMenuBar` from a `GMenuModel`.
 *
 * Returns: a new `BobguiPopoverMenuBar`
 */
BobguiWidget *
bobgui_popover_menu_bar_new_from_model (GMenuModel *model)
{
  return g_object_new (BOBGUI_TYPE_POPOVER_MENU_BAR,
                       "menu-model", model,
                       NULL);
}

/**
 * bobgui_popover_menu_bar_set_menu_model:
 * @bar: a `BobguiPopoverMenuBar`
 * @model: (nullable): a `GMenuModel`
 *
 * Sets a menu model from which @bar should take
 * its contents.
 */
void
bobgui_popover_menu_bar_set_menu_model (BobguiPopoverMenuBar *bar,
                                     GMenuModel        *model)
{
  g_return_if_fail (BOBGUI_IS_POPOVER_MENU_BAR (bar));
  g_return_if_fail (model == NULL || G_IS_MENU_MODEL (model));

  if (g_set_object (&bar->model, model))
    {
      BobguiWidget *child;
      BobguiActionMuxer *muxer;

      while ((child = bobgui_widget_get_first_child (BOBGUI_WIDGET (bar))))
        bobgui_widget_unparent (child);

      g_clear_pointer (&bar->tracker, bobgui_menu_tracker_free);

      if (model)
        {
          muxer = _bobgui_widget_get_action_muxer (BOBGUI_WIDGET (bar), TRUE);
          bar->tracker = bobgui_menu_tracker_new (BOBGUI_ACTION_OBSERVABLE (muxer),
                                               model,
                                               FALSE,
                                               TRUE,
                                               FALSE,
                                               NULL,
                                               tracker_insert,
                                               tracker_remove,
                                               bar);
        }

      g_object_notify_by_pspec (G_OBJECT (bar), bar_props[PROP_MENU_MODEL]);
    }
}

/**
 * bobgui_popover_menu_bar_get_menu_model:
 * @bar: a `BobguiPopoverMenuBar`
 *
 * Returns the model from which the contents of @bar are taken.
 *
 * Returns: (transfer none) (nullable): a `GMenuModel`
 */
GMenuModel *
bobgui_popover_menu_bar_get_menu_model (BobguiPopoverMenuBar *bar)
{
  g_return_val_if_fail (BOBGUI_IS_POPOVER_MENU_BAR (bar), NULL);

  return bar->model;
}

void
bobgui_popover_menu_bar_select_first (BobguiPopoverMenuBar *bar)
{
  BobguiPopoverMenuBarItem *item;

  item = BOBGUI_POPOVER_MENU_BAR_ITEM (bobgui_widget_get_first_child (BOBGUI_WIDGET (bar)));
  set_active_item (bar, item, TRUE);
}

/**
 * bobgui_popover_menu_bar_add_child:
 * @bar: a `BobguiPopoverMenuBar`
 * @child: the `BobguiWidget` to add
 * @id: the ID to insert @child at
 *
 * Adds a custom widget to a generated menubar.
 *
 * For this to work, the menu model of @bar must have an
 * item with a `custom` attribute that matches @id.
 *
 * Returns: %TRUE if @id was found and the widget added
 */
gboolean
bobgui_popover_menu_bar_add_child (BobguiPopoverMenuBar *bar,
                                BobguiWidget         *child,
                                const char        *id)
{
  BobguiWidget *item;

  g_return_val_if_fail (BOBGUI_IS_POPOVER_MENU_BAR (bar), FALSE);
  g_return_val_if_fail (BOBGUI_IS_WIDGET (child), FALSE);
  g_return_val_if_fail (id != NULL, FALSE);

  for (item = bobgui_widget_get_first_child (BOBGUI_WIDGET (bar));
       item;
       item = bobgui_widget_get_next_sibling (item))
    {
      BobguiPopover *popover = BOBGUI_POPOVER_MENU_BAR_ITEM (item)->popover;

      if (bobgui_popover_menu_add_child (BOBGUI_POPOVER_MENU (popover), child, id))
        return TRUE;
    }

  return FALSE;
}

/**
 * bobgui_popover_menu_bar_remove_child:
 * @bar: a `BobguiPopoverMenuBar`
 * @child: the `BobguiWidget` to remove
 *
 * Removes a widget that has previously been added with
 * bobgui_popover_menu_bar_add_child().
 *
 * Returns: %TRUE if the widget was removed
 */
gboolean
bobgui_popover_menu_bar_remove_child (BobguiPopoverMenuBar *bar,
                                   BobguiWidget         *child)
{
  BobguiWidget *item;

  g_return_val_if_fail (BOBGUI_IS_POPOVER_MENU_BAR (bar), FALSE);
  g_return_val_if_fail (BOBGUI_IS_WIDGET (child), FALSE);

  for (item = bobgui_widget_get_first_child (BOBGUI_WIDGET (bar));
       item;
       item = bobgui_widget_get_next_sibling (item))
    {
      BobguiPopover *popover = BOBGUI_POPOVER_MENU_BAR_ITEM (item)->popover;

      if (bobgui_popover_menu_remove_child (BOBGUI_POPOVER_MENU (popover), child))
        return TRUE;
    }

  return FALSE;
}
