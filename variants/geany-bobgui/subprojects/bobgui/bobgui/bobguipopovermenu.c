/* BOBGUI - The Bobgui Framework
 * Copyright © 2014 Red Hat, Inc.
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
#include "bobguipopovermenu.h"
#include "bobguipopovermenuprivate.h"

#include "bobguistack.h"
#include "bobguimenusectionboxprivate.h"
#include "bobguimenubutton.h"
#include "bobguiactionmuxerprivate.h"
#include "bobguimenutrackerprivate.h"
#include "bobguipopoverprivate.h"
#include "bobguiwidgetprivate.h"
#include "bobguieventcontrollerfocus.h"
#include "bobguieventcontrollermotion.h"
#include "bobguimain.h"
#include "bobguitypebuiltins.h"
#include "bobguimodelbuttonprivate.h"
#include "bobguipopovermenubar.h"
#include "bobguishortcutmanager.h"
#include "bobguishortcutcontroller.h"
#include "bobguibuildable.h"
#include "bobguiscrolledwindow.h"
#include "bobguiviewport.h"

/**
 * BobguiPopoverMenu:
 *
 * A subclass of `BobguiPopover` that implements menu behavior.
 *
 * <picture>
 *   <source srcset="menu-dark.png" media="(prefers-color-scheme: dark)">
 *   <img alt="An example BobguiPopoverMenu" src="menu.png">
 * </picture>
 *
 * `BobguiPopoverMenu` treats its children like menus and allows switching
 * between them. It can open submenus as traditional, nested submenus,
 * or in a more touch-friendly sliding fashion.
 * The property [property@Bobgui.PopoverMenu:flags] controls this appearance.
 *
 * `BobguiPopoverMenu` is meant to be used primarily with menu models,
 * using [ctor@Bobgui.PopoverMenu.new_from_model]. If you need to put
 * other widgets such as a `BobguiSpinButton` or a `BobguiSwitch` into a popover,
 * you can use [method@Bobgui.PopoverMenu.add_child].
 *
 * For more dialog-like behavior, use a plain `BobguiPopover`.
 *
 * ## Menu models
 *
 * The XML format understood by `BobguiBuilder` for `GMenuModel` consists
 * of a toplevel `<menu>` element, which contains one or more `<item>`
 * elements. Each `<item>` element contains `<attribute>` and `<link>`
 * elements with a mandatory name attribute. `<link>` elements have the
 * same content model as `<menu>`. Instead of `<link name="submenu">`
 * or `<link name="section">`, you can use `<submenu>` or `<section>`
 * elements.
 *
 * ```xml
 * <menu id='app-menu'>
 *   <section>
 *     <item>
 *       <attribute name='label' translatable='yes'>_New Window</attribute>
 *       <attribute name='action'>app.new</attribute>
 *     </item>
 *     <item>
 *       <attribute name='label' translatable='yes'>_About Sunny</attribute>
 *       <attribute name='action'>app.about</attribute>
 *     </item>
 *     <item>
 *       <attribute name='label' translatable='yes'>_Quit</attribute>
 *       <attribute name='action'>app.quit</attribute>
 *     </item>
 *   </section>
 * </menu>
 * ```
 *
 * Attribute values can be translated using gettext, like other `BobguiBuilder`
 * content. `<attribute>` elements can be marked for translation with a
 * `translatable="yes"` attribute. It is also possible to specify message
 * context and translator comments, using the context and comments attributes.
 * To make use of this, the `BobguiBuilder` must have been given the gettext
 * domain to use.
 *
 * The following attributes are used when constructing menu items:
 *
 * - "label": a user-visible string to display
 * - "use-markup": whether the text in the menu item includes [Pango markup](https://docs.bobgui.org/Pango/pango_markup.html)
 * - "action": the prefixed name of the action to trigger
 * - "target": the parameter to use when activating the action
 * - "icon" and "verb-icon": names of icons that may be displayed
 * - "submenu-action": name of an action that may be used to track
 *      whether a submenu is open
 * - "hidden-when": a string used to determine when the item will be hidden.
 *      Possible values include "action-disabled", "action-missing", "macos-menubar".
 *      This is mainly useful for exported menus, see [method@Bobgui.Application.set_menubar].
 * - "custom": a string used to match against the ID of a custom child added with
 *      [method@Bobgui.PopoverMenu.add_child], [method@Bobgui.PopoverMenuBar.add_child],
 *      or in the ui file with `<child type="ID">`.
 *
 * The following attributes are used when constructing sections:
 *
 * - "label": a user-visible string to use as section heading
 * - "display-hint": a string used to determine special formatting for the section.
 *     Possible values include "horizontal-buttons", "circular-buttons" and
 *     "inline-buttons". They all indicate that section should be
 *     displayed as a horizontal row of buttons.
 * - "text-direction": a string used to determine the `BobguiTextDirection` to use
 *     when "display-hint" is set to "horizontal-buttons". Possible values
 *     include "rtl", "ltr", and "none".
 *
 * The following attributes are used when constructing submenus:
 *
 * - "label": a user-visible string to display
 * - "icon": icon name to display
 * - "bobgui-macos-special": (macOS only, ignored by others) Add special meaning to a menu
 *     in the macOS menu bar. See [Using BOBGUI on Apple macOS](osx.html).
 *
 * Menu items will also show accelerators, which are usually associated
 * with actions via [method@Bobgui.Application.set_accels_for_action],
 * [method@WidgetClass.add_binding_action] or
 * [method@Bobgui.ShortcutController.add_shortcut].
 *
 * # Shortcuts and Gestures
 *
 * `BobguiPopoverMenu` supports the following keyboard shortcuts:
 *
 * - <kbd>Space</kbd> activates the default widget.
 *
 * # CSS Nodes
 *
 * `BobguiPopoverMenu` is just a subclass of `BobguiPopover` that adds custom content
 * to it, therefore it has the same CSS nodes. It is one of the cases that add
 * a `.menu` style class to the main `popover` node.
 *
 * Menu items have nodes with name `button` and class `.model`. If a section
 * display-hint is set, the section gets a node `box` with class `horizontal`
 * plus a class with the same text as the display hint. Note that said box may
 * not be the direct ancestor of the item `button`s. Thus, for example, to style
 * items in an `inline-buttons` section, select `.inline-buttons button.model`.
 * Other things that may be of interest to style in menus include `label` nodes.
 *
 * # Accessibility
 *
 * `BobguiPopoverMenu` uses the [enum@Bobgui.AccessibleRole.menu] role, and its
 * items use the [enum@Bobgui.AccessibleRole.menu_item],
 * [enum@Bobgui.AccessibleRole.checkbox] or [enum@Bobgui.AccessibleRole.menu_item_radio]
 * roles, depending on the action they are connected to.
 */

typedef struct _BobguiPopoverMenuClass BobguiPopoverMenuClass;

struct _BobguiPopoverMenu
{
  BobguiPopover parent_instance;

  BobguiWidget *active_item;
  BobguiWidget *open_submenu;
  BobguiWidget *parent_menu;
  GMenuModel *model;
  BobguiPopoverMenuFlags flags;
};

struct _BobguiPopoverMenuClass
{
  BobguiPopoverClass parent_class;
};

enum {
  PROP_VISIBLE_SUBMENU = 1,
  PROP_MENU_MODEL,
  PROP_FLAGS
};

static void bobgui_popover_menu_buildable_iface_init (BobguiBuildableIface *iface);

G_DEFINE_TYPE_WITH_CODE (BobguiPopoverMenu, bobgui_popover_menu, BOBGUI_TYPE_POPOVER,
                         G_IMPLEMENT_INTERFACE (BOBGUI_TYPE_BUILDABLE,
                                                bobgui_popover_menu_buildable_iface_init))

BobguiWidget *
bobgui_popover_menu_get_parent_menu (BobguiPopoverMenu *menu)
{
  return menu->parent_menu;
}

void
bobgui_popover_menu_set_parent_menu (BobguiPopoverMenu *menu,
                                  BobguiWidget      *parent)
{
  menu->parent_menu = parent;
}

BobguiWidget *
bobgui_popover_menu_get_open_submenu (BobguiPopoverMenu *menu)
{
  return menu->open_submenu;
}

void
bobgui_popover_menu_set_open_submenu (BobguiPopoverMenu *menu,
                                   BobguiWidget      *submenu)
{
  menu->open_submenu = submenu;
}

void
bobgui_popover_menu_close_submenus (BobguiPopoverMenu *menu)
{
  BobguiWidget *submenu;

  submenu = menu->open_submenu;
  if (submenu)
    {
      bobgui_popover_menu_close_submenus (BOBGUI_POPOVER_MENU (submenu));
      bobgui_widget_set_visible (submenu, FALSE);
      bobgui_popover_menu_set_open_submenu (menu, NULL);
    }
}

BobguiWidget *
bobgui_popover_menu_get_active_item (BobguiPopoverMenu *menu)
{
  return menu->active_item;
}

void
bobgui_popover_menu_set_active_item (BobguiPopoverMenu *menu,
                                  BobguiWidget      *item)
{
  if (menu->active_item != item)
    {
      if (menu->active_item)
        {
          bobgui_widget_unset_state_flags (menu->active_item, BOBGUI_STATE_FLAG_SELECTED);
          g_object_remove_weak_pointer (G_OBJECT (menu->active_item), (gpointer *)&menu->active_item);
        }

      menu->active_item = item;

      if (menu->active_item)
        {
          BobguiWidget *popover;

          g_object_add_weak_pointer (G_OBJECT (menu->active_item), (gpointer *)&menu->active_item);

          bobgui_widget_set_state_flags (menu->active_item, BOBGUI_STATE_FLAG_SELECTED, FALSE);
          if (BOBGUI_IS_MODEL_BUTTON (item))
            g_object_get (item, "popover", &popover, NULL);
          else
            popover = NULL;

          if (!popover || popover != menu->open_submenu)
            bobgui_widget_grab_focus (menu->active_item);

          g_clear_object (&popover);
       }
    }
}

static void
visible_submenu_changed (GObject        *object,
                         GParamSpec     *pspec,
                         BobguiPopoverMenu *popover)
{
  g_object_notify (G_OBJECT (popover), "visible-submenu");
}

static void
focus_out (BobguiEventController   *controller,
           BobguiPopoverMenu       *menu)
{
  BobguiRoot *root;
  BobguiWidget *new_focus;

  root = bobgui_widget_get_root (BOBGUI_WIDGET (menu));
  if (!root)
    return;

  new_focus = bobgui_root_get_focus (root);

  if (!bobgui_event_controller_focus_contains_focus (BOBGUI_EVENT_CONTROLLER_FOCUS (controller)) &&
      new_focus != NULL)
    {
      if (menu->parent_menu &&
          BOBGUI_POPOVER_MENU (menu->parent_menu)->open_submenu == (BobguiWidget*) menu)
        BOBGUI_POPOVER_MENU (menu->parent_menu)->open_submenu = NULL;
      bobgui_popover_popdown (BOBGUI_POPOVER (menu));
    }
}

static void
leave_cb (BobguiEventController *controller,
          gpointer            data)
{
  if (!bobgui_event_controller_motion_contains_pointer (BOBGUI_EVENT_CONTROLLER_MOTION (controller)))
    {
      BobguiWidget *target = bobgui_event_controller_get_widget (controller);

      bobgui_popover_menu_set_active_item (BOBGUI_POPOVER_MENU (target), NULL);
    }
}

static void
bobgui_popover_menu_init (BobguiPopoverMenu *popover)
{
  BobguiWidget *sw;
  BobguiWidget *stack;
  BobguiEventController *controller;
  BobguiEventController **controllers;
  guint n_controllers, i;

  sw = bobgui_scrolled_window_new ();
  bobgui_scrolled_window_set_policy (BOBGUI_SCROLLED_WINDOW (sw), BOBGUI_POLICY_AUTOMATIC, BOBGUI_POLICY_AUTOMATIC);
  bobgui_scrolled_window_set_propagate_natural_width (BOBGUI_SCROLLED_WINDOW (sw), TRUE);
  bobgui_scrolled_window_set_propagate_natural_height (BOBGUI_SCROLLED_WINDOW (sw), TRUE);
  bobgui_popover_set_child (BOBGUI_POPOVER (popover), sw);

  stack = bobgui_stack_new ();
  bobgui_stack_set_vhomogeneous (BOBGUI_STACK (stack), FALSE);
  bobgui_stack_set_transition_type (BOBGUI_STACK (stack), BOBGUI_STACK_TRANSITION_TYPE_SLIDE_LEFT_RIGHT);
  bobgui_stack_set_interpolate_size (BOBGUI_STACK (stack), TRUE);
  bobgui_scrolled_window_set_child (BOBGUI_SCROLLED_WINDOW (sw), stack);
  g_signal_connect (stack, "notify::visible-child-name",
                    G_CALLBACK (visible_submenu_changed), popover);

  bobgui_widget_add_css_class (BOBGUI_WIDGET (popover), "menu");

  controller = bobgui_event_controller_focus_new ();
  g_signal_connect (controller, "leave", G_CALLBACK (focus_out), popover);
  bobgui_widget_add_controller (BOBGUI_WIDGET (popover), controller);

  controller = bobgui_event_controller_motion_new ();
  g_signal_connect (controller, "notify::contains-pointer", G_CALLBACK (leave_cb), popover);
  bobgui_widget_add_controller (BOBGUI_WIDGET (popover), controller);

  controllers = bobgui_widget_list_controllers (BOBGUI_WIDGET (popover), BOBGUI_PHASE_CAPTURE, &n_controllers);
  for (i = 0; i < n_controllers; i ++)
    {
      controller = controllers[i];
      if (BOBGUI_IS_SHORTCUT_CONTROLLER (controller) &&
          strcmp (bobgui_event_controller_get_name (controller), "bobgui-shortcut-manager-capture") == 0)
        bobgui_shortcut_controller_set_mnemonics_modifiers (BOBGUI_SHORTCUT_CONTROLLER (controller), 0);
    }
  g_free (controllers);

  bobgui_popover_set_cascade_popdown (BOBGUI_POPOVER (popover), TRUE);
}

BobguiWidget *
bobgui_popover_menu_get_stack (BobguiPopoverMenu *menu)
{
  BobguiWidget *sw = bobgui_popover_get_child (BOBGUI_POPOVER (menu));
  BobguiWidget *vp = bobgui_scrolled_window_get_child (BOBGUI_SCROLLED_WINDOW (sw));
  BobguiWidget *stack = bobgui_viewport_get_child (BOBGUI_VIEWPORT (vp));

  return stack;
}

static void
bobgui_popover_menu_dispose (GObject *object)
{
  BobguiPopoverMenu *popover = BOBGUI_POPOVER_MENU (object);

  if (popover->active_item)
    {
      g_object_remove_weak_pointer (G_OBJECT (popover->active_item), (gpointer *)&popover->active_item);
      popover->active_item = NULL;
    }

  g_clear_object (&popover->model);

  G_OBJECT_CLASS (bobgui_popover_menu_parent_class)->dispose (object);
}

static void
bobgui_popover_menu_map (BobguiWidget *widget)
{
  bobgui_popover_menu_open_submenu (BOBGUI_POPOVER_MENU (widget), "main");
  BOBGUI_WIDGET_CLASS (bobgui_popover_menu_parent_class)->map (widget);
}

static void
bobgui_popover_menu_unmap (BobguiWidget *widget)
{
  BOBGUI_WIDGET_CLASS (bobgui_popover_menu_parent_class)->unmap (widget);
  bobgui_popover_menu_open_submenu (BOBGUI_POPOVER_MENU (widget), "main");
}

static void
bobgui_popover_menu_get_property (GObject    *object,
                               guint       property_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  BobguiPopoverMenu *menu = BOBGUI_POPOVER_MENU (object);

  switch (property_id)
    {
    case PROP_VISIBLE_SUBMENU:
      g_value_set_string (value, bobgui_stack_get_visible_child_name (BOBGUI_STACK (bobgui_popover_menu_get_stack (menu))));
      break;

    case PROP_MENU_MODEL:
      g_value_set_object (value, bobgui_popover_menu_get_menu_model (menu));
      break;

    case PROP_FLAGS:
      g_value_set_flags (value, bobgui_popover_menu_get_flags (menu));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
bobgui_popover_menu_set_property (GObject      *object,
                               guint         property_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  BobguiPopoverMenu *menu = BOBGUI_POPOVER_MENU (object);

  switch (property_id)
    {
    case PROP_VISIBLE_SUBMENU:
      bobgui_stack_set_visible_child_name (BOBGUI_STACK (bobgui_popover_menu_get_stack (menu)), g_value_get_string (value));
      break;

    case PROP_MENU_MODEL:
      bobgui_popover_menu_set_menu_model (menu, g_value_get_object (value));
      break;

    case PROP_FLAGS:
      bobgui_popover_menu_set_flags (menu, g_value_get_flags (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gboolean
bobgui_popover_menu_focus (BobguiWidget        *widget,
                        BobguiDirectionType  direction)
{
  BobguiPopoverMenu *menu = BOBGUI_POPOVER_MENU (widget);

  if (bobgui_widget_get_first_child (widget) == NULL)
    {
      return FALSE;
    }
  else
    {
      if (menu->open_submenu)
        {
          if (bobgui_widget_child_focus (menu->open_submenu, direction))
            return TRUE;
          if (direction == BOBGUI_DIR_LEFT)
            {
              if (menu->open_submenu)
                {
                  bobgui_popover_popdown (BOBGUI_POPOVER (menu->open_submenu));
                  menu->open_submenu = NULL;
                }

              bobgui_widget_grab_focus (menu->active_item);

              return TRUE;
            }
          return FALSE;
        }

      if (bobgui_widget_focus_move (widget, direction))
        return TRUE;

      if (direction == BOBGUI_DIR_LEFT || direction == BOBGUI_DIR_RIGHT)
        {
          /* If we are part of a menubar, we want to let the
           * menubar use left/right arrows for cycling, else
           * we eat them.
           */
          if (bobgui_widget_get_ancestor (widget, BOBGUI_TYPE_POPOVER_MENU_BAR) ||
              (bobgui_popover_menu_get_parent_menu (menu) &&
               direction == BOBGUI_DIR_LEFT))
            return FALSE;
          else
            return TRUE;
        }
      /* Cycle around with up/down arrows and (Shift+)Tab when modal */
      else if (bobgui_popover_get_autohide (BOBGUI_POPOVER (menu)))
        {
          BobguiWidget *p = bobgui_root_get_focus (bobgui_widget_get_root (widget));

          /* In the case where the popover doesn't have any focusable child, if
           * the menu doesn't have any item for example, then the focus will end
           * up out of the popover, hence creating an infinite loop below. To
           * avoid this, just say we had focus and stop here.
           */
          if (!bobgui_widget_is_ancestor (p, widget) && p != widget)
            return TRUE;

          /* cycle around */
          for (;
               p != widget;
               p = bobgui_widget_get_parent (p))
            {
              bobgui_widget_set_focus_child (p, NULL);
            }
          if (bobgui_widget_focus_move (widget, direction))
            return TRUE;
        }
    }

  return FALSE;
}

static void
add_tab_bindings (BobguiWidgetClass   *widget_class,
                  GdkModifierType   modifiers,
                  BobguiDirectionType  direction)
{
  bobgui_widget_class_add_binding_signal (widget_class, GDK_KEY_Tab, modifiers,
                                       "move-focus",
                                       "(i)", direction);
  bobgui_widget_class_add_binding_signal (widget_class, GDK_KEY_KP_Tab, modifiers,
                                       "move-focus",
                                       "(i)", direction);
}

static void
add_arrow_bindings (BobguiWidgetClass   *widget_class,
                    guint             keysym,
                    BobguiDirectionType  direction)
{
  guint keypad_keysym = keysym - GDK_KEY_Left + GDK_KEY_KP_Left;

  bobgui_widget_class_add_binding_signal (widget_class, keysym, 0,
                                       "move-focus",
                                       "(i)", direction);
  bobgui_widget_class_add_binding_signal (widget_class, keysym, GDK_CONTROL_MASK,
                                       "move-focus",
                                       "(i)", direction);
  bobgui_widget_class_add_binding_signal (widget_class, keypad_keysym, 0,
                                       "move-focus",
                                       "(i)", direction);
  bobgui_widget_class_add_binding_signal (widget_class, keypad_keysym, GDK_CONTROL_MASK,
                                       "move-focus",
                                       "(i)", direction);
}

static void
bobgui_popover_menu_show (BobguiWidget *widget)
{
  bobgui_popover_menu_set_open_submenu (BOBGUI_POPOVER_MENU (widget), NULL);

  BOBGUI_WIDGET_CLASS (bobgui_popover_menu_parent_class)->show (widget);
}

static void
bobgui_popover_menu_move_focus (BobguiWidget         *widget,
                             BobguiDirectionType  direction)
{
  bobgui_popover_set_mnemonics_visible (BOBGUI_POPOVER (widget), TRUE);

  BOBGUI_WIDGET_CLASS (bobgui_popover_menu_parent_class)->move_focus (widget, direction);
}

static void
bobgui_popover_menu_root (BobguiWidget *widget)
{
  BOBGUI_WIDGET_CLASS (bobgui_popover_menu_parent_class)->root (widget);

  bobgui_accessible_update_property (BOBGUI_ACCESSIBLE (widget),
                                  BOBGUI_ACCESSIBLE_PROPERTY_ORIENTATION, BOBGUI_ORIENTATION_VERTICAL,
                                  -1);
}

static void
bobgui_popover_menu_class_init (BobguiPopoverMenuClass *klass)
{
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (klass);
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = bobgui_popover_menu_dispose;
  object_class->set_property = bobgui_popover_menu_set_property;
  object_class->get_property = bobgui_popover_menu_get_property;

  widget_class->root = bobgui_popover_menu_root;
  widget_class->map = bobgui_popover_menu_map;
  widget_class->unmap = bobgui_popover_menu_unmap;
  widget_class->focus = bobgui_popover_menu_focus;
  widget_class->show = bobgui_popover_menu_show;
  widget_class->move_focus = bobgui_popover_menu_move_focus;

  /**
   * BobguiPopoverMenu:visible-submenu:
   *
   * The name of the visible submenu.
   */
  g_object_class_install_property (object_class,
                                   PROP_VISIBLE_SUBMENU,
                                   g_param_spec_string ("visible-submenu", NULL, NULL,
                                                        NULL,
                                                        G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  /**
   * BobguiPopoverMenu:menu-model:
   *
   * The model from which the menu is made.
   */
  g_object_class_install_property (object_class,
                                   PROP_MENU_MODEL,
                                   g_param_spec_object ("menu-model", NULL, NULL,
                                                        G_TYPE_MENU_MODEL,
                                                        G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  /**
   * BobguiPopoverMenu:flags:
   *
   * The flags that @popover uses to create/display a menu from its model.
   *
   * If a model is set and the flags change, contents are rebuilt, so if setting
   * properties individually, set flags before model to avoid a redundant rebuild.
   *
   * Since: 4.14
   */
  g_object_class_install_property (object_class,
                                   PROP_FLAGS,
                                   g_param_spec_flags ("flags", NULL, NULL,
                                                       BOBGUI_TYPE_POPOVER_MENU_FLAGS,
                                                       BOBGUI_POPOVER_MENU_SLIDING,
                                                       G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS
                                                         | G_PARAM_EXPLICIT_NOTIFY));

  add_arrow_bindings (widget_class, GDK_KEY_Up, BOBGUI_DIR_UP);
  add_arrow_bindings (widget_class, GDK_KEY_Down, BOBGUI_DIR_DOWN);
  add_arrow_bindings (widget_class, GDK_KEY_Left, BOBGUI_DIR_LEFT);
  add_arrow_bindings (widget_class, GDK_KEY_Right, BOBGUI_DIR_RIGHT);

  add_tab_bindings (widget_class, 0, BOBGUI_DIR_TAB_FORWARD);
  add_tab_bindings (widget_class, GDK_CONTROL_MASK, BOBGUI_DIR_TAB_FORWARD);
  add_tab_bindings (widget_class, GDK_SHIFT_MASK, BOBGUI_DIR_TAB_BACKWARD);
  add_tab_bindings (widget_class, GDK_CONTROL_MASK | GDK_SHIFT_MASK, BOBGUI_DIR_TAB_BACKWARD);

  bobgui_widget_class_add_binding_signal (widget_class, GDK_KEY_Return, 0,
                                       "activate-default", NULL);
  bobgui_widget_class_add_binding_signal (widget_class, GDK_KEY_ISO_Enter, 0,
                                       "activate-default", NULL);
  bobgui_widget_class_add_binding_signal (widget_class, GDK_KEY_KP_Enter, 0,
                                       "activate-default", NULL);
  bobgui_widget_class_add_binding_signal (widget_class, GDK_KEY_space, 0,
                                       "activate-default", NULL);
  bobgui_widget_class_add_binding_signal (widget_class, GDK_KEY_KP_Space, 0,
                                       "activate-default", NULL);

  bobgui_widget_class_set_accessible_role (widget_class, BOBGUI_ACCESSIBLE_ROLE_MENU);
}

static BobguiBuildableIface *parent_buildable_iface;

static void
bobgui_popover_menu_buildable_add_child (BobguiBuildable *buildable,
                                      BobguiBuilder   *builder,
                                      GObject      *child,
                                      const char   *type)
{
  if (BOBGUI_IS_WIDGET (child))
    {
      if (!bobgui_popover_menu_add_child (BOBGUI_POPOVER_MENU (buildable), BOBGUI_WIDGET (child), type))
        g_warning ("No such custom attribute: %s", type);
    }
  else
    parent_buildable_iface->add_child (buildable, builder, child, type);
}

static void
bobgui_popover_menu_buildable_iface_init (BobguiBuildableIface *iface)
{
  parent_buildable_iface = g_type_interface_peek_parent (iface);

  iface->add_child = bobgui_popover_menu_buildable_add_child;
}

static void
bobgui_popover_menu_rebuild_contents (BobguiPopoverMenu *popover)
{
  BobguiWidget *stack;
  BobguiWidget *child;

  stack = bobgui_popover_menu_get_stack (popover);
  while ((child = bobgui_widget_get_first_child (stack)))
    bobgui_stack_remove (BOBGUI_STACK (stack), child);

  if (popover->model)
    bobgui_menu_section_box_new_toplevel (popover, popover->model, popover->flags);
}

/**
 * bobgui_popover_menu_new:
 *
 * Creates a new popover menu.
 *
 * Returns: a new `BobguiPopoverMenu`
 */
BobguiWidget *
bobgui_popover_menu_new (void)
{
  BobguiWidget *popover;

  popover = g_object_new (BOBGUI_TYPE_POPOVER_MENU,
                          "autohide", TRUE,
                          NULL);

  return popover;
}

/*<private>
 * bobgui_popover_menu_open_submenu:
 * @popover: a `BobguiPopoverMenu`
 * @name: the name of the menu to switch to
 *
 * Opens a submenu of the @popover. The @name
 * must be one of the names given to the submenus
 * of @popover with `BobguiPopoverMenu:submenu`, or
 * "main" to switch back to the main menu.
 *
 * `BobguiModelButton` will open submenus automatically
 * when the `BobguiModelButton:menu-name` property is set,
 * so this function is only needed when you are using
 * other kinds of widgets to initiate menu changes.
 */
void
bobgui_popover_menu_open_submenu (BobguiPopoverMenu *popover,
                               const char     *name)
{
  BobguiWidget *stack;

  g_return_if_fail (BOBGUI_IS_POPOVER_MENU (popover));

  stack = bobgui_popover_menu_get_stack (popover);
  bobgui_stack_set_visible_child_name (BOBGUI_STACK (stack), name);
}

void
bobgui_popover_menu_add_submenu (BobguiPopoverMenu *popover,
                              BobguiWidget      *submenu,
                              const char     *name)
{
  BobguiWidget *stack = bobgui_popover_menu_get_stack (popover);
  bobgui_stack_add_named (BOBGUI_STACK (stack), submenu, name);
}

/**
 * bobgui_popover_menu_new_from_model:
 * @model: (nullable): a `GMenuModel`
 *
 * Creates a `BobguiPopoverMenu` and populates it according to @model.
 *
 * The created buttons are connected to actions found in the
 * `BobguiApplicationWindow` to which the popover belongs - typically
 * by means of being attached to a widget that is contained within
 * the `BobguiApplicationWindow`s widget hierarchy.
 *
 * Actions can also be added using [method@Bobgui.Widget.insert_action_group]
 * on the menus attach widget or on any of its parent widgets.
 *
 * This function creates menus with sliding submenus.
 * See [ctor@Bobgui.PopoverMenu.new_from_model_full] for a way
 * to control this.
 *
 * Returns: the new `BobguiPopoverMenu`
 */
BobguiWidget *
bobgui_popover_menu_new_from_model (GMenuModel *model)

{
  return bobgui_popover_menu_new_from_model_full (model, BOBGUI_POPOVER_MENU_SLIDING);
}

/**
 * bobgui_popover_menu_new_from_model_full:
 * @model: a `GMenuModel`
 * @flags: flags that affect how the menu is created
 *
 * Creates a `BobguiPopoverMenu` and populates it according to @model.
 *
 * The created buttons are connected to actions found in the
 * action groups that are accessible from the parent widget.
 * This includes the `BobguiApplicationWindow` to which the popover
 * belongs. Actions can also be added using [method@Bobgui.Widget.insert_action_group]
 * on the parent widget or on any of its parent widgets.
 *
 * Returns: the new `BobguiPopoverMenu`
 */
BobguiWidget *
bobgui_popover_menu_new_from_model_full (GMenuModel          *model,
                                      BobguiPopoverMenuFlags  flags)
{
  BobguiWidget *popover;

  g_return_val_if_fail (model == NULL || G_IS_MENU_MODEL (model), NULL);

  popover = bobgui_popover_menu_new ();
  bobgui_popover_menu_set_flags (BOBGUI_POPOVER_MENU (popover), flags);
  bobgui_popover_menu_set_menu_model (BOBGUI_POPOVER_MENU (popover), model);

  return popover;
}

/**
 * bobgui_popover_menu_set_menu_model:
 * @popover: a `BobguiPopoverMenu`
 * @model: (nullable): a `GMenuModel`
 *
 * Sets a new menu model on @popover.
 *
 * The existing contents of @popover are removed, and
 * the @popover is populated with new contents according
 * to @model.
 */
void
bobgui_popover_menu_set_menu_model (BobguiPopoverMenu *popover,
                                 GMenuModel     *model)
{
  g_return_if_fail (BOBGUI_IS_POPOVER_MENU (popover));
  g_return_if_fail (model == NULL || G_IS_MENU_MODEL (model));

  if (g_set_object (&popover->model, model))
    {
      bobgui_popover_menu_rebuild_contents (popover);
      g_object_notify (G_OBJECT (popover), "menu-model");
    }
}

/**
 * bobgui_popover_menu_set_flags:
 * @popover: a `BobguiPopoverMenu`
 * @flags: a set of `BobguiPopoverMenuFlags`
 *
 * Sets the flags that @popover uses to create/display a menu from its model.
 *
 * If a model is set and the flags change, contents are rebuilt, so if setting
 * properties individually, set flags before model to avoid a redundant rebuild.
 *
 * Since: 4.14
 */
void
bobgui_popover_menu_set_flags (BobguiPopoverMenu      *popover,
                            BobguiPopoverMenuFlags  flags)
{
  g_return_if_fail (BOBGUI_IS_POPOVER_MENU (popover));

  if (popover->flags == flags)
    return;

  popover->flags = flags;

  /* This shouldn’t happen IRL, but notify test unsets :child, so dodge error */
  if (bobgui_popover_get_child (BOBGUI_POPOVER (popover)) != NULL)
    bobgui_popover_menu_rebuild_contents (popover);

  g_object_notify (G_OBJECT (popover), "flags");
}

/**
 * bobgui_popover_menu_get_menu_model:
 * @popover: a `BobguiPopoverMenu`
 *
 * Returns the menu model used to populate the popover.
 *
 * Returns: (transfer none) (nullable): the menu model of @popover
 */
GMenuModel *
bobgui_popover_menu_get_menu_model (BobguiPopoverMenu *popover)
{
  g_return_val_if_fail (BOBGUI_IS_POPOVER_MENU (popover), NULL);

  return popover->model;
}

/**
 * bobgui_popover_menu_get_flags:
 * @popover: a `BobguiPopoverMenu`
 *
 * Returns the flags that @popover uses to create/display a menu from its model.
 *
 * Returns: the `BobguiPopoverMenuFlags`
 *
 * Since: 4.14
 */
BobguiPopoverMenuFlags
bobgui_popover_menu_get_flags (BobguiPopoverMenu *popover)
{
  g_return_val_if_fail (BOBGUI_IS_POPOVER_MENU (popover), 0);

  return popover->flags;
}

/**
 * bobgui_popover_menu_add_child:
 * @popover: a `BobguiPopoverMenu`
 * @child: the `BobguiWidget` to add
 * @id: the ID to insert @child at
 *
 * Adds a custom widget to a generated menu.
 *
 * For this to work, the menu model of @popover must have
 * an item with a `custom` attribute that matches @id.
 *
 * Returns: %TRUE if @id was found and the widget added
 */
gboolean
bobgui_popover_menu_add_child (BobguiPopoverMenu *popover,
                            BobguiWidget      *child,
                            const char     *id)
{

  g_return_val_if_fail (BOBGUI_IS_POPOVER_MENU (popover), FALSE);
  g_return_val_if_fail (BOBGUI_IS_WIDGET (child), FALSE);
  g_return_val_if_fail (id != NULL, FALSE);

  return bobgui_menu_section_box_add_custom (popover, child, id);
}

/**
 * bobgui_popover_menu_remove_child:
 * @popover: a `BobguiPopoverMenu`
 * @child: the `BobguiWidget` to remove
 *
 * Removes a widget that has previously been added with
 * [method@Bobgui.PopoverMenu.add_child]
 *
 * Returns: %TRUE if the widget was removed
 */
gboolean
bobgui_popover_menu_remove_child (BobguiPopoverMenu *popover,
                               BobguiWidget      *child)
{

  g_return_val_if_fail (BOBGUI_IS_POPOVER_MENU (popover), FALSE);
  g_return_val_if_fail (BOBGUI_IS_WIDGET (child), FALSE);

  return bobgui_menu_section_box_remove_custom (popover, child);
}
