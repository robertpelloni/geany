/* BOBGUI - The Bobgui Framework
 *
 * Copyright (C) 2003 Ricardo Fernandez Pascual
 * Copyright (C) 2004 Paolo Borelli
 * Copyright (C) 2012 Bastien Nocera
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
 * BobguiMenuButton:
 *
 * Displays a popup when clicked.
 *
 * <picture>
 *   <source srcset="menu-button-dark.png" media="(prefers-color-scheme: dark)">
 *   <img alt="An example BobguiMenuButton" src="menu-button.png">
 * </picture>
 *
 * This popup can be provided either as a `BobguiPopover` or as an abstract
 * `GMenuModel`.
 *
 * The `BobguiMenuButton` widget can show either an icon (set with the
 * [property@Bobgui.MenuButton:icon-name] property) or a label (set with the
 * [property@Bobgui.MenuButton:label] property). If neither is explicitly set,
 * a [class@Bobgui.Image] is automatically created, using an arrow image oriented
 * according to [property@Bobgui.MenuButton:direction] or the generic
 * “open-menu-symbolic” icon if the direction is not set.
 *
 * The positioning of the popup is determined by the
 * [property@Bobgui.MenuButton:direction] property of the menu button.
 *
 * For menus, the [property@Bobgui.Widget:halign] and [property@Bobgui.Widget:valign]
 * properties of the menu are also taken into account. For example, when the
 * direction is %BOBGUI_ARROW_DOWN and the horizontal alignment is %BOBGUI_ALIGN_START,
 * the menu will be positioned below the button, with the starting edge
 * (depending on the text direction) of the menu aligned with the starting
 * edge of the button. If there is not enough space below the button, the
 * menu is popped up above the button instead. If the alignment would move
 * part of the menu offscreen, it is “pushed in”.
 *
 * |           | start                | center                | end                |
 * | -         | ---                  | ---                   | ---                |
 * | **down**  | ![](down-start.png)  | ![](down-center.png)  | ![](down-end.png)  |
 * | **up**    | ![](up-start.png)    | ![](up-center.png)    | ![](up-end.png)    |
 * | **left**  | ![](left-start.png)  | ![](left-center.png)  | ![](left-end.png)  |
 * | **right** | ![](right-start.png) | ![](right-center.png) | ![](right-end.png) |
 *
 * # CSS nodes
 *
 * ```
 * menubutton
 * ╰── button.toggle
 *     ╰── <content>
 *          ╰── [arrow]
 * ```
 *
 * `BobguiMenuButton` has a single CSS node with name `menubutton`
 * which contains a `button` node with a `.toggle` style class.
 *
 * If the button contains an icon, it will have the `.image-button` style class,
 * if it contains text, it will have `.text-button` style class. If an arrow is
 * visible in addition to an icon, text or a custom child, it will also have
 * `.arrow-button` style class.
 *
 * Inside the toggle button content, there is an `arrow` node for
 * the indicator, which will carry one of the `.none`, `.up`, `.down`,
 * `.left` or `.right` style classes to indicate the direction that
 * the menu will appear in. The CSS is expected to provide a suitable
 * image for each of these cases using the `-bobgui-icon-source` property.
 *
 * Optionally, the `menubutton` node can carry the `.circular` style class
 * to request a round appearance.
 *
 * # Accessibility
 *
 * `BobguiMenuButton` uses the [enum@Bobgui.AccessibleRole.button] role.
 */

#include "config.h"

#include "bobguiactionable.h"
#include "bobguibinlayout.h"
#include "bobguibuildable.h"
#include "bobguibuiltiniconprivate.h"
#include "bobguigizmoprivate.h"
#include "bobguiimage.h"
#include "bobguimain.h"
#include "bobguimenubutton.h"
#include "bobguimenubuttonprivate.h"
#include "bobguipopover.h"
#include "bobguipopovermenu.h"
#include "bobguiprivate.h"
#include "bobguitypebuiltins.h"
#include "bobguilabel.h"
#include "bobguibox.h"
#include "bobguiwidgetprivate.h"
#include "bobguibuttonprivate.h"
#include "bobguinative.h"
#include "bobguiwindow.h"
#include "bobguibuilderprivate.h"

typedef struct _BobguiMenuButtonClass   BobguiMenuButtonClass;
typedef struct _BobguiMenuButtonPrivate BobguiMenuButtonPrivate;

struct _BobguiMenuButton
{
  BobguiWidget parent_instance;

  BobguiWidget *button;
  BobguiWidget *popover; /* Only one at a time can be set */
  GMenuModel *model;

  BobguiMenuButtonCreatePopupFunc create_popup_func;
  gpointer create_popup_user_data;
  GDestroyNotify create_popup_destroy_notify;

  BobguiWidget *label_widget;
  BobguiWidget *image_widget;
  BobguiWidget *arrow_widget;
  BobguiWidget *child;
  BobguiArrowType arrow_type;
  gboolean always_show_arrow;

  gboolean primary;
  gboolean can_shrink;
};

struct _BobguiMenuButtonClass
{
  BobguiWidgetClass parent_class;

  void (* activate) (BobguiMenuButton *self);
};

enum
{
  PROP_0,
  PROP_MENU_MODEL,
  PROP_DIRECTION,
  PROP_POPOVER,
  PROP_ICON_NAME,
  PROP_ALWAYS_SHOW_ARROW,
  PROP_LABEL,
  PROP_USE_UNDERLINE,
  PROP_HAS_FRAME,
  PROP_PRIMARY,
  PROP_CHILD,
  PROP_ACTIVE,
  PROP_CAN_SHRINK,
  LAST_PROP
};

enum {
  ACTIVATE,
  LAST_SIGNAL
};

static GParamSpec *menu_button_props[LAST_PROP];
static guint signals[LAST_SIGNAL] = { 0 };

static void bobgui_menu_button_buildable_iface_init (BobguiBuildableIface *iface);

G_DEFINE_TYPE_WITH_CODE (BobguiMenuButton, bobgui_menu_button, BOBGUI_TYPE_WIDGET,
                         G_IMPLEMENT_INTERFACE (BOBGUI_TYPE_BUILDABLE, bobgui_menu_button_buildable_iface_init))

static void bobgui_menu_button_dispose (GObject *object);

static void
bobgui_menu_button_set_property (GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  BobguiMenuButton *self = BOBGUI_MENU_BUTTON (object);

  switch (property_id)
    {
      case PROP_MENU_MODEL:
        bobgui_menu_button_set_menu_model (self, g_value_get_object (value));
        break;
      case PROP_DIRECTION:
        bobgui_menu_button_set_direction (self, g_value_get_enum (value));
        break;
      case PROP_POPOVER:
        bobgui_menu_button_set_popover (self, g_value_get_object (value));
        break;
      case PROP_ICON_NAME:
        bobgui_menu_button_set_icon_name (self, g_value_get_string (value));
        break;
      case PROP_ALWAYS_SHOW_ARROW:
        bobgui_menu_button_set_always_show_arrow (self, g_value_get_boolean (value));
        break;
      case PROP_LABEL:
        bobgui_menu_button_set_label (self, g_value_get_string (value));
        break;
      case PROP_USE_UNDERLINE:
        bobgui_menu_button_set_use_underline (self, g_value_get_boolean (value));
        break;
      case PROP_HAS_FRAME:
        bobgui_menu_button_set_has_frame (self, g_value_get_boolean (value));
        break;
      case PROP_PRIMARY:
        bobgui_menu_button_set_primary (self, g_value_get_boolean (value));
        break;
      case PROP_CHILD:
        bobgui_menu_button_set_child (self, g_value_get_object (value));
        break;
      case PROP_ACTIVE:
        bobgui_menu_button_set_active (self, g_value_get_boolean (value));
        break;
      case PROP_CAN_SHRINK:
        bobgui_menu_button_set_can_shrink (self, g_value_get_boolean (value));
        break;
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
bobgui_menu_button_get_property (GObject    *object,
                              guint       property_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
  BobguiMenuButton *self = BOBGUI_MENU_BUTTON (object);

  switch (property_id)
    {
      case PROP_MENU_MODEL:
        g_value_set_object (value, self->model);
        break;
      case PROP_DIRECTION:
        g_value_set_enum (value, self->arrow_type);
        break;
      case PROP_POPOVER:
        g_value_set_object (value, self->popover);
        break;
      case PROP_ICON_NAME:
        g_value_set_string (value, bobgui_menu_button_get_icon_name (BOBGUI_MENU_BUTTON (object)));
        break;
      case PROP_ALWAYS_SHOW_ARROW:
        g_value_set_boolean (value, bobgui_menu_button_get_always_show_arrow (self));
        break;
      case PROP_LABEL:
        g_value_set_string (value, bobgui_menu_button_get_label (BOBGUI_MENU_BUTTON (object)));
        break;
      case PROP_USE_UNDERLINE:
        g_value_set_boolean (value, bobgui_menu_button_get_use_underline (BOBGUI_MENU_BUTTON (object)));
        break;
      case PROP_HAS_FRAME:
        g_value_set_boolean (value, bobgui_menu_button_get_has_frame (BOBGUI_MENU_BUTTON (object)));
        break;
      case PROP_PRIMARY:
        g_value_set_boolean (value, bobgui_menu_button_get_primary (BOBGUI_MENU_BUTTON (object)));
        break;
      case PROP_CHILD:
        g_value_set_object (value, bobgui_menu_button_get_child (self));
        break;
      case PROP_ACTIVE:
        g_value_set_boolean (value, bobgui_menu_button_get_active (self));
        break;
      case PROP_CAN_SHRINK:
        g_value_set_boolean (value, bobgui_menu_button_get_can_shrink (self));
        break;
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
bobgui_menu_button_notify (GObject    *object,
                        GParamSpec *pspec)
{
  if (strcmp (pspec->name, "focus-on-click") == 0)
    {
      BobguiMenuButton *self = BOBGUI_MENU_BUTTON (object);

      bobgui_widget_set_focus_on_click (self->button,
                                     bobgui_widget_get_focus_on_click (BOBGUI_WIDGET (self)));
    }

  if (G_OBJECT_CLASS (bobgui_menu_button_parent_class)->notify)
    G_OBJECT_CLASS (bobgui_menu_button_parent_class)->notify (object, pspec);
}

static void
bobgui_menu_button_state_flags_changed (BobguiWidget    *widget,
                                     BobguiStateFlags previous_state_flags)
{
  BobguiMenuButton *self = BOBGUI_MENU_BUTTON (widget);

  if (!bobgui_widget_is_sensitive (widget))
    {
      if (self->popover)
        bobgui_widget_set_visible (self->popover, FALSE);
    }
}

static void
bobgui_menu_button_toggled (BobguiMenuButton *self)
{
  const gboolean active = bobgui_toggle_button_get_active (BOBGUI_TOGGLE_BUTTON (self->button));

  /* Might set a new menu/popover */
  if (active && self->create_popup_func)
    {
      self->create_popup_func (self, self->create_popup_user_data);
    }

  if (self->popover)
    {
      if (active)
        {
          bobgui_popover_popup (BOBGUI_POPOVER (self->popover));
          bobgui_accessible_update_state (BOBGUI_ACCESSIBLE (self),
                                       BOBGUI_ACCESSIBLE_STATE_EXPANDED, TRUE,
                                       -1);
        }
      else
        {
          bobgui_popover_popdown (BOBGUI_POPOVER (self->popover));
          bobgui_accessible_reset_state (BOBGUI_ACCESSIBLE (self),
                                      BOBGUI_ACCESSIBLE_STATE_EXPANDED);
        }
    }

  g_object_notify_by_pspec (G_OBJECT (self), menu_button_props[PROP_ACTIVE]);
}

static void
bobgui_menu_button_measure (BobguiWidget      *widget,
                         BobguiOrientation  orientation,
                         int             for_size,
                         int            *minimum,
                         int            *natural,
                         int            *minimum_baseline,
                         int            *natural_baseline)
{
  BobguiMenuButton *self = BOBGUI_MENU_BUTTON (widget);

  bobgui_widget_measure (self->button,
                      orientation,
                      for_size,
                      minimum, natural,
                      minimum_baseline, natural_baseline);

}

static void
bobgui_menu_button_size_allocate (BobguiWidget *widget,
                               int        width,
                               int        height,
                               int        baseline)
{
  BobguiMenuButton *self= BOBGUI_MENU_BUTTON (widget);

  bobgui_widget_size_allocate (self->button,
                            &(BobguiAllocation) { 0, 0, width, height },
                            baseline);
  if (self->popover)
    bobgui_popover_present (BOBGUI_POPOVER (self->popover));
}

static gboolean
bobgui_menu_button_focus (BobguiWidget        *widget,
                       BobguiDirectionType  direction)
{
  BobguiMenuButton *self = BOBGUI_MENU_BUTTON (widget);

  if (self->popover && bobgui_widget_get_visible (self->popover))
    return bobgui_widget_child_focus (self->popover, direction);
  else
    return bobgui_widget_child_focus (self->button, direction);
}

static gboolean
bobgui_menu_button_grab_focus (BobguiWidget *widget)
{
  BobguiMenuButton *self = BOBGUI_MENU_BUTTON (widget);

  return bobgui_widget_grab_focus (self->button);
}

static void
bobgui_menu_button_activate (BobguiMenuButton *self)
{
  bobgui_widget_activate (self->button);
}

static void bobgui_menu_button_root (BobguiWidget *widget);
static void bobgui_menu_button_unroot (BobguiWidget *widget);

static void
bobgui_menu_button_class_init (BobguiMenuButtonClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (klass);

  gobject_class->set_property = bobgui_menu_button_set_property;
  gobject_class->get_property = bobgui_menu_button_get_property;
  gobject_class->notify = bobgui_menu_button_notify;
  gobject_class->dispose = bobgui_menu_button_dispose;

  widget_class->root = bobgui_menu_button_root;
  widget_class->unroot = bobgui_menu_button_unroot;
  widget_class->measure = bobgui_menu_button_measure;
  widget_class->size_allocate = bobgui_menu_button_size_allocate;
  widget_class->state_flags_changed = bobgui_menu_button_state_flags_changed;
  widget_class->focus = bobgui_menu_button_focus;
  widget_class->grab_focus = bobgui_menu_button_grab_focus;

  klass->activate = bobgui_menu_button_activate;

  /**
   * BobguiMenuButton:menu-model:
   *
   * The `GMenuModel` from which the popup will be created.
   *
   * See [method@Bobgui.MenuButton.set_menu_model] for the interaction
   * with the [property@Bobgui.MenuButton:popover] property.
   */
  menu_button_props[PROP_MENU_MODEL] =
      g_param_spec_object ("menu-model", NULL, NULL,
                           G_TYPE_MENU_MODEL,
                           BOBGUI_PARAM_READWRITE);

  /**
   * BobguiMenuButton:direction:
   *
   * The `BobguiArrowType` representing the direction in which the
   * menu or popover will be popped out.
   */
  menu_button_props[PROP_DIRECTION] =
      g_param_spec_enum ("direction", NULL, NULL,
                         BOBGUI_TYPE_ARROW_TYPE,
                         BOBGUI_ARROW_DOWN,
                         BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiMenuButton:popover:
   *
   * The `BobguiPopover` that will be popped up when the button is clicked.
   */
  menu_button_props[PROP_POPOVER] =
      g_param_spec_object ("popover", NULL, NULL,
                           BOBGUI_TYPE_POPOVER,
                           G_PARAM_READWRITE);

  /**
   * BobguiMenuButton:icon-name:
   *
   * The name of the icon used to automatically populate the button.
   */
  menu_button_props[PROP_ICON_NAME] =
      g_param_spec_string ("icon-name", NULL, NULL,
                           NULL,
                           BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiMenuButton:always-show-arrow:
   *
   * Whether to show a dropdown arrow even when using an icon or a custom child.
   *
   * Since: 4.4
   */
  menu_button_props[PROP_ALWAYS_SHOW_ARROW] =
      g_param_spec_boolean ("always-show-arrow", NULL, NULL,
                            FALSE,
                            BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiMenuButton:label:
   *
   * The label for the button.
   */
  menu_button_props[PROP_LABEL] =
      g_param_spec_string ("label", NULL, NULL,
                           NULL,
                           BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiMenuButton:use-underline:
   *
   * If set an underscore in the text indicates a mnemonic.
   */
  menu_button_props[PROP_USE_UNDERLINE] =
      g_param_spec_boolean ("use-underline", NULL, NULL,
                           FALSE,
                           BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiMenuButton:has-frame:
   *
   * Whether the button has a frame.
   */
  menu_button_props[PROP_HAS_FRAME] =
    g_param_spec_boolean ("has-frame", NULL, NULL,
                          TRUE,
                          BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiMenuButton:primary:
   *
   * Whether the menu button acts as a primary menu.
   *
   * Primary menus can be opened using the <kbd>F10</kbd> key
   *
   * Since: 4.4
   */
  menu_button_props[PROP_PRIMARY] =
    g_param_spec_boolean ("primary", NULL, NULL,
                          FALSE,
                          BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiMenuButton:child:
   *
   * The child widget.
   *
   * Since: 4.6
   */
  menu_button_props[PROP_CHILD] =
    g_param_spec_object ("child", NULL, NULL,
                         BOBGUI_TYPE_WIDGET,
                         BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiMenuButton:active:
   *
   * Whether the menu button is active.
   *
   * Since: 4.10
   */
  menu_button_props[PROP_ACTIVE] =
    g_param_spec_boolean ("active", NULL, NULL,
                          FALSE,
                          BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiMenuButton:can-shrink:
   *
   * Whether the size of the button can be made smaller than the natural
   * size of its contents.
   *
   * Since: 4.12
   */
  menu_button_props[PROP_CAN_SHRINK] =
    g_param_spec_boolean ("can-shrink", NULL, NULL,
                          FALSE,
                          BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (gobject_class, LAST_PROP, menu_button_props);

  /**
   * BobguiMenuButton::activate:
   * @widget: the object which received the signal.
   *
   * Emitted to when the menu button is activated.
   *
   * The `::activate` signal on `BobguiMenuButton` is an action signal and
   * emitting it causes the button to pop up its menu.
   *
   * Since: 4.4
   */
  signals[ACTIVATE] =
      g_signal_new (I_ ("activate"),
                    G_OBJECT_CLASS_TYPE (gobject_class),
                    G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
                    G_STRUCT_OFFSET (BobguiMenuButtonClass, activate),
                    NULL, NULL,
                    NULL,
                    G_TYPE_NONE, 0);

  bobgui_widget_class_set_activate_signal (widget_class, signals[ACTIVATE]);

  bobgui_widget_class_set_css_name (widget_class, I_("menubutton"));
  bobgui_widget_class_set_accessible_role (widget_class, BOBGUI_ACCESSIBLE_ROLE_BUTTON);
}

static void
set_arrow_type (BobguiWidget    *arrow,
                BobguiArrowType  arrow_type,
                gboolean      visible)
{
  bobgui_widget_remove_css_class (arrow, "none");
  bobgui_widget_remove_css_class (arrow, "down");
  bobgui_widget_remove_css_class (arrow, "up");
  bobgui_widget_remove_css_class (arrow, "left");
  bobgui_widget_remove_css_class (arrow, "right");
  switch (arrow_type)
    {
    case BOBGUI_ARROW_NONE:
      bobgui_widget_add_css_class (arrow, "none");
      break;
    case BOBGUI_ARROW_DOWN:
      bobgui_widget_add_css_class (arrow, "down");
      break;
    case BOBGUI_ARROW_UP:
      bobgui_widget_add_css_class (arrow, "up");
      break;
    case BOBGUI_ARROW_LEFT:
      bobgui_widget_add_css_class (arrow, "left");
      break;
    case BOBGUI_ARROW_RIGHT:
      bobgui_widget_add_css_class (arrow, "right");
      break;
    default:
      break;
    }

  bobgui_widget_set_visible (arrow, visible);
}

static void
update_style_classes (BobguiMenuButton *menu_button)
{
  gboolean has_icon = menu_button->image_widget != NULL;
  gboolean has_label = menu_button->label_widget != NULL;
  gboolean has_only_arrow = menu_button->arrow_widget == bobgui_button_get_child (BOBGUI_BUTTON (menu_button->button));
  gboolean has_arrow = bobgui_widget_get_visible (menu_button->arrow_widget);

  if (has_only_arrow || has_icon)
    bobgui_widget_add_css_class (menu_button->button, "image-button");
  else
    bobgui_widget_remove_css_class (menu_button->button, "image-button");

  if (has_label)
    bobgui_widget_add_css_class (menu_button->button, "text-button");
  else
    bobgui_widget_remove_css_class (menu_button->button, "text-button");

  if (has_arrow && !has_only_arrow)
    bobgui_widget_add_css_class (menu_button->button, "arrow-button");
  else
    bobgui_widget_remove_css_class (menu_button->button, "arrow-button");
}

static void
update_arrow (BobguiMenuButton *menu_button)
{
  gboolean has_only_arrow, is_text_button;

  if (menu_button->arrow_widget == NULL)
    return;

  has_only_arrow = menu_button->arrow_widget == bobgui_button_get_child (BOBGUI_BUTTON (menu_button->button));
  is_text_button = menu_button->label_widget != NULL;

  set_arrow_type (menu_button->arrow_widget,
                  menu_button->arrow_type,
                  has_only_arrow ||
                  ((is_text_button || menu_button->always_show_arrow) &&
                   (menu_button->arrow_type != BOBGUI_ARROW_NONE)));

  update_style_classes (menu_button);
}

static void
add_arrow (BobguiMenuButton *self)
{
  BobguiWidget *arrow;

  arrow = bobgui_builtin_icon_new ("arrow");
  bobgui_widget_set_halign (arrow, BOBGUI_ALIGN_CENTER);
  set_arrow_type (arrow, self->arrow_type, TRUE);
  bobgui_button_set_child (BOBGUI_BUTTON (self->button), arrow);
  self->arrow_widget = arrow;
}

static void
bobgui_menu_button_init (BobguiMenuButton *self)
{
  self->arrow_type = BOBGUI_ARROW_DOWN;

  self->button = bobgui_toggle_button_new ();
  bobgui_widget_set_parent (self->button, BOBGUI_WIDGET (self));
  g_signal_connect_swapped (self->button, "toggled", G_CALLBACK (bobgui_menu_button_toggled), self);
  add_arrow (self);
  update_style_classes (self);

  bobgui_widget_set_sensitive (self->button, FALSE);

  bobgui_widget_add_css_class (BOBGUI_WIDGET (self), "popup");
}

static BobguiBuildableIface *parent_buildable_iface;

static void
bobgui_menu_button_buildable_add_child (BobguiBuildable *buildable,
                                     BobguiBuilder   *builder,
                                     GObject      *child,
                                     const char   *type)
{
  if (BOBGUI_IS_WIDGET (child))
    {
      bobgui_buildable_child_deprecation_warning (buildable, builder, NULL, "child");
      bobgui_menu_button_set_child (BOBGUI_MENU_BUTTON (buildable), BOBGUI_WIDGET (child));
    }
  else
    {
      parent_buildable_iface->add_child (buildable, builder, child, type);
    }
}

static void
bobgui_menu_button_buildable_iface_init (BobguiBuildableIface *iface)
{
  parent_buildable_iface = g_type_interface_peek_parent (iface);

  iface->add_child = bobgui_menu_button_buildable_add_child;
}

/**
 * bobgui_menu_button_new:
 *
 * Creates a new `BobguiMenuButton` widget with downwards-pointing
 * arrow as the only child.
 *
 * You can replace the child widget with another `BobguiWidget`
 * should you wish to.
 *
 * Returns: The newly created `BobguiMenuButton`
 */
BobguiWidget *
bobgui_menu_button_new (void)
{
  return g_object_new (BOBGUI_TYPE_MENU_BUTTON, NULL);
}

static void
update_sensitivity (BobguiMenuButton *self)
{
  gboolean has_popup;

  has_popup = self->popover != NULL || self->create_popup_func != NULL;

  bobgui_widget_set_sensitive (self->button, has_popup);

  bobgui_accessible_update_property (BOBGUI_ACCESSIBLE (self->button),
                                  BOBGUI_ACCESSIBLE_PROPERTY_HAS_POPUP, has_popup,
                                  -1);
  if (self->popover != NULL)
    bobgui_accessible_update_relation (BOBGUI_ACCESSIBLE (self->button),
                                    BOBGUI_ACCESSIBLE_RELATION_CONTROLS, self->popover, NULL,
                                    -1);
  else
    bobgui_accessible_reset_relation (BOBGUI_ACCESSIBLE (self->button),
                                   BOBGUI_ACCESSIBLE_RELATION_CONTROLS);
}

static gboolean
menu_deactivate_cb (BobguiMenuButton *self)
{
  bobgui_toggle_button_set_active (BOBGUI_TOGGLE_BUTTON (self->button), FALSE);

  return TRUE;
}

/**
 * bobgui_menu_button_set_menu_model:
 * @menu_button: a `BobguiMenuButton`
 * @menu_model: (nullable): a `GMenuModel`, or %NULL to unset and disable the
 *   button
 *
 * Sets the `GMenuModel` from which the popup will be constructed.
 *
 * If @menu_model is %NULL, the button is disabled.
 *
 * A [class@Bobgui.Popover] will be created from the menu model with
 * [ctor@Bobgui.PopoverMenu.new_from_model]. Actions will be connected
 * as documented for this function.
 *
 * If [property@Bobgui.MenuButton:popover] is already set, it will be
 * dissociated from the @menu_button, and the property is set to %NULL.
 */
void
bobgui_menu_button_set_menu_model (BobguiMenuButton *menu_button,
                                GMenuModel    *menu_model)
{
  g_return_if_fail (BOBGUI_IS_MENU_BUTTON (menu_button));
  g_return_if_fail (G_IS_MENU_MODEL (menu_model) || menu_model == NULL);

  g_object_freeze_notify (G_OBJECT (menu_button));

  if (menu_model)
    g_object_ref (menu_model);

  if (menu_model)
    {
      BobguiWidget *popover;

      popover = bobgui_popover_menu_new_from_model (menu_model);

      bobgui_accessible_update_relation (BOBGUI_ACCESSIBLE (popover),
                                      BOBGUI_ACCESSIBLE_RELATION_LABELLED_BY, menu_button, NULL,
                                      -1);

      bobgui_menu_button_set_popover (menu_button, popover);
    }
  else
    {
      bobgui_menu_button_set_popover (menu_button, NULL);
    }

  menu_button->model = menu_model;
  g_object_notify_by_pspec (G_OBJECT (menu_button), menu_button_props[PROP_MENU_MODEL]);

  g_object_thaw_notify (G_OBJECT (menu_button));
}

/**
 * bobgui_menu_button_get_menu_model:
 * @menu_button: a `BobguiMenuButton`
 *
 * Returns the `GMenuModel` used to generate the popup.
 *
 * Returns: (nullable) (transfer none): a `GMenuModel`
 */
GMenuModel *
bobgui_menu_button_get_menu_model (BobguiMenuButton *menu_button)
{
  g_return_val_if_fail (BOBGUI_IS_MENU_BUTTON (menu_button), NULL);

  return menu_button->model;
}

static void
update_popover_direction (BobguiMenuButton *self)
{
  if (!self->popover)
    return;

  switch (self->arrow_type)
    {
    case BOBGUI_ARROW_UP:
      bobgui_popover_set_position (BOBGUI_POPOVER (self->popover), BOBGUI_POS_TOP);
      break;
    case BOBGUI_ARROW_DOWN:
    case BOBGUI_ARROW_NONE:
      bobgui_popover_set_position (BOBGUI_POPOVER (self->popover), BOBGUI_POS_BOTTOM);
      break;
    case BOBGUI_ARROW_LEFT:
      bobgui_popover_set_position (BOBGUI_POPOVER (self->popover), BOBGUI_POS_LEFT);
      break;
    case BOBGUI_ARROW_RIGHT:
      bobgui_popover_set_position (BOBGUI_POPOVER (self->popover), BOBGUI_POS_RIGHT);
      break;
    default:
      break;
    }
}

static void
popover_destroy_cb (BobguiMenuButton *menu_button)
{
  bobgui_menu_button_set_popover (menu_button, NULL);
}

/**
 * bobgui_menu_button_set_direction:
 * @menu_button: a `BobguiMenuButton`
 * @direction: a `BobguiArrowType`
 *
 * Sets the direction in which the popup will be popped up.
 *
 * If the button is automatically populated with an arrow icon,
 * its direction will be changed to match.
 *
 * If the does not fit in the available space in the given direction,
 * BOBGUI will its best to keep it inside the screen and fully visible.
 *
 * If you pass %BOBGUI_ARROW_NONE for a @direction, the popup will behave
 * as if you passed %BOBGUI_ARROW_DOWN (although you won’t see any arrows).
 */
void
bobgui_menu_button_set_direction (BobguiMenuButton *menu_button,
                               BobguiArrowType   direction)
{
  g_return_if_fail (BOBGUI_IS_MENU_BUTTON (menu_button));

  if (menu_button->arrow_type == direction)
    return;

  menu_button->arrow_type = direction;
  g_object_notify_by_pspec (G_OBJECT (menu_button), menu_button_props[PROP_DIRECTION]);

  update_arrow (menu_button);
  update_popover_direction (menu_button);
}

/**
 * bobgui_menu_button_get_direction:
 * @menu_button: a `BobguiMenuButton`
 *
 * Returns the direction the popup will be pointing at when popped up.
 *
 * Returns: a `BobguiArrowType` value
 */
BobguiArrowType
bobgui_menu_button_get_direction (BobguiMenuButton *menu_button)
{
  g_return_val_if_fail (BOBGUI_IS_MENU_BUTTON (menu_button), BOBGUI_ARROW_DOWN);

  return menu_button->arrow_type;
}

static void
bobgui_menu_button_dispose (GObject *object)
{
  BobguiMenuButton *self = BOBGUI_MENU_BUTTON (object);

  if (self->popover)
    {
      g_signal_handlers_disconnect_by_func (self->popover,
                                            menu_deactivate_cb,
                                            object);
      g_signal_handlers_disconnect_by_func (self->popover,
                                            popover_destroy_cb,
                                            object);
      bobgui_widget_unparent (self->popover);
      self->popover = NULL;
    }

  g_clear_object (&self->model);
  g_clear_pointer (&self->button, bobgui_widget_unparent);

  if (self->create_popup_destroy_notify)
    self->create_popup_destroy_notify (self->create_popup_user_data);

  G_OBJECT_CLASS (bobgui_menu_button_parent_class)->dispose (object);
}

/**
 * bobgui_menu_button_set_popover:
 * @menu_button: a `BobguiMenuButton`
 * @popover: (nullable) (type BobguiPopover): a `BobguiPopover`, or %NULL to unset and
 *   disable the button
 *
 * Sets the `BobguiPopover` that will be popped up when the @menu_button is clicked.
 *
 * If @popover is %NULL, the button is disabled.
 *
 * If [property@Bobgui.MenuButton:menu-model] is set, the menu model is dissociated
 * from the @menu_button, and the property is set to %NULL.
 */
void
bobgui_menu_button_set_popover (BobguiMenuButton *menu_button,
                             BobguiWidget     *popover)
{
  g_return_if_fail (BOBGUI_IS_MENU_BUTTON (menu_button));
  g_return_if_fail (BOBGUI_IS_POPOVER (popover) || popover == NULL);

  g_object_freeze_notify (G_OBJECT (menu_button));

  g_clear_object (&menu_button->model);

  if (menu_button->popover)
    {
      bobgui_widget_set_visible (menu_button->popover, FALSE);

      g_signal_handlers_disconnect_by_func (menu_button->popover,
                                            menu_deactivate_cb,
                                            menu_button);
      g_signal_handlers_disconnect_by_func (menu_button->popover,
                                            popover_destroy_cb,
                                            menu_button);

      bobgui_widget_unparent (menu_button->popover);
    }

  menu_button->popover = popover;

  if (popover)
    {
      bobgui_widget_set_parent (menu_button->popover, BOBGUI_WIDGET (menu_button));
      g_signal_connect_swapped (menu_button->popover, "closed",
                                G_CALLBACK (menu_deactivate_cb), menu_button);
      g_signal_connect_swapped (menu_button->popover, "destroy",
                                G_CALLBACK (popover_destroy_cb), menu_button);
      update_popover_direction (menu_button);
    }

  update_sensitivity (menu_button);

  g_object_notify_by_pspec (G_OBJECT (menu_button), menu_button_props[PROP_POPOVER]);
  g_object_notify_by_pspec (G_OBJECT (menu_button), menu_button_props[PROP_MENU_MODEL]);
  g_object_thaw_notify (G_OBJECT (menu_button));
}

/**
 * bobgui_menu_button_get_popover:
 * @menu_button: a `BobguiMenuButton`
 *
 * Returns the `BobguiPopover` that pops out of the button.
 *
 * If the button is not using a `BobguiPopover`, this function
 * returns %NULL.
 *
 * Returns: (nullable) (transfer none): a `BobguiPopover` or %NULL
 */
BobguiPopover *
bobgui_menu_button_get_popover (BobguiMenuButton *menu_button)
{
  g_return_val_if_fail (BOBGUI_IS_MENU_BUTTON (menu_button), NULL);

  return BOBGUI_POPOVER (menu_button->popover);
}

/**
 * bobgui_menu_button_set_icon_name:
 * @menu_button: a `BobguiMenuButton`
 * @icon_name: the icon name
 *
 * Sets the name of an icon to show inside the menu button.
 *
 * Setting icon name resets [property@Bobgui.MenuButton:label] and
 * [property@Bobgui.MenuButton:child].
 *
 * If [property@Bobgui.MenuButton:always-show-arrow] is set to `TRUE` and
 * [property@Bobgui.MenuButton:direction] is not `BOBGUI_ARROW_NONE`, a dropdown arrow
 * will be shown next to the icon.
 */
void
bobgui_menu_button_set_icon_name (BobguiMenuButton *menu_button,
                               const char    *icon_name)
{
  BobguiWidget *box, *image_widget, *arrow;

  g_return_if_fail (BOBGUI_IS_MENU_BUTTON (menu_button));

  g_object_freeze_notify (G_OBJECT (menu_button));

  if (bobgui_menu_button_get_label (menu_button))
    g_object_notify_by_pspec (G_OBJECT (menu_button), menu_button_props[PROP_LABEL]);

  if (bobgui_menu_button_get_child (menu_button))
    g_object_notify_by_pspec (G_OBJECT (menu_button), menu_button_props[PROP_CHILD]);

  box = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 0);
  bobgui_widget_set_halign (box, BOBGUI_ALIGN_CENTER);

  image_widget = g_object_new (BOBGUI_TYPE_IMAGE,
                               "accessible-role", BOBGUI_ACCESSIBLE_ROLE_PRESENTATION,
                               "icon-name", icon_name,
                               NULL);
  menu_button->image_widget = image_widget;

  arrow = bobgui_builtin_icon_new ("arrow");
  menu_button->arrow_widget = arrow;

  bobgui_box_append (BOBGUI_BOX (box), image_widget);
  bobgui_box_append (BOBGUI_BOX (box), arrow);
  bobgui_button_set_child (BOBGUI_BUTTON (menu_button->button), box);

  menu_button->label_widget = NULL;
  menu_button->child = NULL;

  update_arrow (menu_button);

  g_object_notify_by_pspec (G_OBJECT (menu_button), menu_button_props[PROP_ICON_NAME]);

  g_object_thaw_notify (G_OBJECT (menu_button));
}

/**
 * bobgui_menu_button_get_icon_name:
 * @menu_button: a `BobguiMenuButton`
 *
 * Gets the name of the icon shown in the button.
 *
 * Returns: (nullable): the name of the icon shown in the button
 */
const char *
bobgui_menu_button_get_icon_name (BobguiMenuButton *menu_button)
{
  g_return_val_if_fail (BOBGUI_IS_MENU_BUTTON (menu_button), NULL);

  if (menu_button->image_widget)
    return bobgui_image_get_icon_name (BOBGUI_IMAGE (menu_button->image_widget));

  return NULL;
}

/**
 * bobgui_menu_button_set_always_show_arrow:
 * @menu_button: a `BobguiMenuButton`
 * @always_show_arrow: whether to show a dropdown arrow even when using an icon
 * or a custom child
 *
 * Sets whether to show a dropdown arrow even when using an icon or a custom
 * child.
 *
 * Since: 4.4
 */
void
bobgui_menu_button_set_always_show_arrow (BobguiMenuButton *menu_button,
                                       gboolean       always_show_arrow)
{
  g_return_if_fail (BOBGUI_IS_MENU_BUTTON (menu_button));

  always_show_arrow = !!always_show_arrow;

  if (always_show_arrow == menu_button->always_show_arrow)
    return;

  menu_button->always_show_arrow = always_show_arrow;

  update_arrow (menu_button);

  g_object_notify_by_pspec (G_OBJECT (menu_button), menu_button_props[PROP_ALWAYS_SHOW_ARROW]);
}

/**
 * bobgui_menu_button_get_always_show_arrow:
 * @menu_button: a `BobguiMenuButton`
 *
 * Gets whether to show a dropdown arrow even when using an icon or a custom
 * child.
 *
 * Returns: whether to show a dropdown arrow even when using an icon or a custom
 * child.
 *
 * Since: 4.4
 */
gboolean
bobgui_menu_button_get_always_show_arrow (BobguiMenuButton *menu_button)
{
  g_return_val_if_fail (BOBGUI_IS_MENU_BUTTON (menu_button), FALSE);

  return menu_button->always_show_arrow;
}

/**
 * bobgui_menu_button_set_label:
 * @menu_button: a `BobguiMenuButton`
 * @label: the label
 *
 * Sets the label to show inside the menu button.
 *
 * Setting a label resets [property@Bobgui.MenuButton:icon-name] and
 * [property@Bobgui.MenuButton:child].
 *
 * If [property@Bobgui.MenuButton:direction] is not `BOBGUI_ARROW_NONE`, a dropdown
 * arrow will be shown next to the label.
 */
void
bobgui_menu_button_set_label (BobguiMenuButton *menu_button,
                           const char    *label)
{
  BobguiWidget *box;
  BobguiWidget *label_widget;
  BobguiWidget *arrow;

  g_return_if_fail (BOBGUI_IS_MENU_BUTTON (menu_button));

  g_object_freeze_notify (G_OBJECT (menu_button));

  if (bobgui_menu_button_get_icon_name (menu_button))
    g_object_notify_by_pspec (G_OBJECT (menu_button), menu_button_props[PROP_ICON_NAME]);
  if (bobgui_menu_button_get_child (menu_button))
    g_object_notify_by_pspec (G_OBJECT (menu_button), menu_button_props[PROP_CHILD]);

  box = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 0);
  bobgui_widget_set_hexpand (box, FALSE);
  label_widget = bobgui_label_new (label);
  bobgui_label_set_use_underline (BOBGUI_LABEL (label_widget),
                               bobgui_button_get_use_underline (BOBGUI_BUTTON (menu_button->button)));
  bobgui_label_set_ellipsize (BOBGUI_LABEL (label_widget),
                           menu_button->can_shrink ? PANGO_ELLIPSIZE_END
                                                   : PANGO_ELLIPSIZE_NONE);
  bobgui_widget_set_hexpand (label_widget, TRUE);
  arrow = bobgui_builtin_icon_new ("arrow");
  menu_button->arrow_widget = arrow;
  bobgui_box_append (BOBGUI_BOX (box), label_widget);
  bobgui_box_append (BOBGUI_BOX (box), arrow);
  bobgui_button_set_child (BOBGUI_BUTTON (menu_button->button), box);
  menu_button->label_widget = label_widget;

  menu_button->image_widget = NULL;
  menu_button->child = NULL;

  update_arrow (menu_button);

  g_object_notify_by_pspec (G_OBJECT (menu_button), menu_button_props[PROP_LABEL]);

  g_object_thaw_notify (G_OBJECT (menu_button));
}

/**
 * bobgui_menu_button_get_label:
 * @menu_button: a `BobguiMenuButton`
 *
 * Gets the label shown in the button
 *
 * Returns: (nullable): the label shown in the button
 */
const char *
bobgui_menu_button_get_label (BobguiMenuButton *menu_button)
{
  g_return_val_if_fail (BOBGUI_IS_MENU_BUTTON (menu_button), NULL);

  if (menu_button->label_widget)
    return bobgui_label_get_label (BOBGUI_LABEL (menu_button->label_widget));

  return NULL;
}

/**
 * bobgui_menu_button_set_has_frame:
 * @menu_button: a `BobguiMenuButton`
 * @has_frame: whether the button should have a visible frame
 *
 * Sets the style of the button.
 */
void
bobgui_menu_button_set_has_frame (BobguiMenuButton *menu_button,
                               gboolean       has_frame)
{
  g_return_if_fail (BOBGUI_IS_MENU_BUTTON (menu_button));

  if (bobgui_button_get_has_frame (BOBGUI_BUTTON (menu_button->button)) == has_frame)
    return;

  bobgui_button_set_has_frame (BOBGUI_BUTTON (menu_button->button), has_frame);
  g_object_notify_by_pspec (G_OBJECT (menu_button), menu_button_props[PROP_HAS_FRAME]);
}

/**
 * bobgui_menu_button_get_has_frame:
 * @menu_button: a `BobguiMenuButton`
 *
 * Returns whether the button has a frame.
 *
 * Returns: %TRUE if the button has a frame
 */
gboolean
bobgui_menu_button_get_has_frame (BobguiMenuButton *menu_button)
{
  g_return_val_if_fail (BOBGUI_IS_MENU_BUTTON (menu_button), TRUE);

  return bobgui_button_get_has_frame (BOBGUI_BUTTON (menu_button->button));
}

/**
 * bobgui_menu_button_popup:
 * @menu_button: a `BobguiMenuButton`
 *
 * Pop up the menu.
 */
void
bobgui_menu_button_popup (BobguiMenuButton *menu_button)
{
  g_return_if_fail (BOBGUI_IS_MENU_BUTTON (menu_button));

  bobgui_toggle_button_set_active (BOBGUI_TOGGLE_BUTTON (menu_button->button), TRUE);
}

/**
 * bobgui_menu_button_popdown:
 * @menu_button: a `BobguiMenuButton`
 *
 * Dismiss the menu.
 */
void
bobgui_menu_button_popdown (BobguiMenuButton *menu_button)
{
  g_return_if_fail (BOBGUI_IS_MENU_BUTTON (menu_button));

  bobgui_toggle_button_set_active (BOBGUI_TOGGLE_BUTTON (menu_button->button), FALSE);
}

/**
 * bobgui_menu_button_set_create_popup_func:
 * @menu_button: a `BobguiMenuButton`
 * @func: (nullable) (scope notified) (closure user_data) (destroy destroy_notify): function
 *   to call when a popup is about to be shown, but none has been provided via other means,
 *   or %NULL to reset to default behavior
 * @user_data: user data to pass to @func
 * @destroy_notify: (nullable): destroy notify for @user_data
 *
 * Sets @func to be called when a popup is about to be shown.
 *
 * @func should use one of
 *
 *  - [method@Bobgui.MenuButton.set_popover]
 *  - [method@Bobgui.MenuButton.set_menu_model]
 *
 * to set a popup for @menu_button.
 * If @func is non-%NULL, @menu_button will always be sensitive.
 *
 * Using this function will not reset the menu widget attached to
 * @menu_button. Instead, this can be done manually in @func.
 */
void
bobgui_menu_button_set_create_popup_func (BobguiMenuButton                *menu_button,
                                       BobguiMenuButtonCreatePopupFunc  func,
                                       gpointer                      user_data,
                                       GDestroyNotify                destroy_notify)
{
  g_return_if_fail (BOBGUI_IS_MENU_BUTTON (menu_button));

  if (menu_button->create_popup_destroy_notify)
    menu_button->create_popup_destroy_notify (menu_button->create_popup_user_data);

  menu_button->create_popup_func = func;
  menu_button->create_popup_user_data = user_data;
  menu_button->create_popup_destroy_notify = destroy_notify;

  update_sensitivity (menu_button);
}

/**
 * bobgui_menu_button_set_use_underline:
 * @menu_button: a `BobguiMenuButton`
 * @use_underline: %TRUE if underlines in the text indicate mnemonics
 *
 * If true, an underline in the text indicates a mnemonic.
 */
void
bobgui_menu_button_set_use_underline (BobguiMenuButton *menu_button,
                                   gboolean       use_underline)
{
  g_return_if_fail (BOBGUI_IS_MENU_BUTTON (menu_button));

  if (bobgui_button_get_use_underline (BOBGUI_BUTTON (menu_button->button)) == use_underline)
    return;

  bobgui_button_set_use_underline (BOBGUI_BUTTON (menu_button->button), use_underline);
  if (menu_button->label_widget)
    bobgui_label_set_use_underline (BOBGUI_LABEL (menu_button->label_widget), use_underline);

  g_object_notify_by_pspec (G_OBJECT (menu_button), menu_button_props[PROP_USE_UNDERLINE]);
}

/**
 * bobgui_menu_button_get_use_underline:
 * @menu_button: a `BobguiMenuButton`
 *
 * Returns whether an embedded underline in the text indicates a
 * mnemonic.
 *
 * Returns: %TRUE whether an embedded underline in the text indicates
 *   the mnemonic accelerator keys.
 */
gboolean
bobgui_menu_button_get_use_underline (BobguiMenuButton *menu_button)
{
  g_return_val_if_fail (BOBGUI_IS_MENU_BUTTON (menu_button), FALSE);

  return bobgui_button_get_use_underline (BOBGUI_BUTTON (menu_button->button));
}

static GList *
get_menu_bars (BobguiWidget *toplevel)
{
  return g_object_get_data (G_OBJECT (toplevel), "bobgui-menu-bar-list");
}

static void
set_menu_bars (BobguiWidget *toplevel,
               GList     *menubars)
{
  g_object_set_data (G_OBJECT (toplevel), I_("bobgui-menu-bar-list"), menubars);
}

static void
add_to_toplevel (BobguiWidget     *toplevel,
                 BobguiMenuButton *button)
{
  GList *menubars = get_menu_bars (toplevel);

  set_menu_bars (toplevel, g_list_prepend (menubars, button));
}

static void
remove_from_toplevel (BobguiWidget     *toplevel,
                      BobguiMenuButton *button)
{
  GList *menubars = get_menu_bars (toplevel);

  menubars = g_list_remove (menubars, button);
  set_menu_bars (toplevel, menubars);
}

static void
bobgui_menu_button_root (BobguiWidget *widget)
{
  BobguiMenuButton *button = BOBGUI_MENU_BUTTON (widget);

  BOBGUI_WIDGET_CLASS (bobgui_menu_button_parent_class)->root (widget);

  if (button->primary)
    {
      BobguiWidget *toplevel = BOBGUI_WIDGET (bobgui_widget_get_root (widget));
      add_to_toplevel (toplevel, button);
    }
}

static void
bobgui_menu_button_unroot (BobguiWidget *widget)
{
  BobguiWidget *toplevel;

  toplevel = BOBGUI_WIDGET (bobgui_widget_get_root (widget));
  remove_from_toplevel (toplevel, BOBGUI_MENU_BUTTON (widget));

  BOBGUI_WIDGET_CLASS (bobgui_menu_button_parent_class)->unroot (widget);
}

/**
 * bobgui_menu_button_set_primary:
 * @menu_button: a `BobguiMenuButton`
 * @primary: whether the menubutton should act as a primary menu
 *
 * Sets whether menu button acts as a primary menu.
 *
 * Primary menus can be opened with the <kbd>F10</kbd> key.
 *
 * Since: 4.4
 */
void
bobgui_menu_button_set_primary (BobguiMenuButton *menu_button,
                             gboolean       primary)
{
  BobguiRoot *toplevel;

  g_return_if_fail (BOBGUI_IS_MENU_BUTTON (menu_button));

  if (menu_button->primary == primary)
    return;

  menu_button->primary = primary;
  toplevel = bobgui_widget_get_root (BOBGUI_WIDGET (menu_button));

  if (toplevel)
    {
      if (menu_button->primary)
        add_to_toplevel (BOBGUI_WIDGET (toplevel), menu_button);
      else
        remove_from_toplevel (BOBGUI_WIDGET (toplevel), menu_button);
    }

  g_object_notify_by_pspec (G_OBJECT (menu_button), menu_button_props[PROP_PRIMARY]);
}

/**
 * bobgui_menu_button_get_primary:
 * @menu_button: a `BobguiMenuButton`
 *
 * Returns whether the menu button acts as a primary menu.
 *
 * Returns: %TRUE if the button is a primary menu
 *
 * Since: 4.4
 */
gboolean
bobgui_menu_button_get_primary (BobguiMenuButton *menu_button)
{
  g_return_val_if_fail (BOBGUI_IS_MENU_BUTTON (menu_button), FALSE);

  return menu_button->primary;
}

/**
 * bobgui_menu_button_set_child:
 * @menu_button: a `BobguiMenuButton`
 * @child: (nullable): the child widget
 *
 * Sets the child widget of @menu_button.
 *
 * Setting a child resets [property@Bobgui.MenuButton:label] and
 * [property@Bobgui.MenuButton:icon-name].
 *
 * If [property@Bobgui.MenuButton:always-show-arrow] is set to `TRUE` and
 * [property@Bobgui.MenuButton:direction] is not `BOBGUI_ARROW_NONE`, a dropdown arrow
 * will be shown next to the child.
 *
 * Since: 4.6
 */
void
bobgui_menu_button_set_child (BobguiMenuButton *menu_button,
                           BobguiWidget     *child)
{
  BobguiWidget *box, *arrow, *inner_widget;

  g_return_if_fail (BOBGUI_IS_MENU_BUTTON (menu_button));
  g_return_if_fail (child == NULL || menu_button->child == child || bobgui_widget_get_parent (child) == NULL);

  if (menu_button->child == child)
    return;

  g_object_freeze_notify (G_OBJECT (menu_button));

  if (bobgui_menu_button_get_label (menu_button))
    g_object_notify_by_pspec (G_OBJECT (menu_button), menu_button_props[PROP_LABEL]);
  if (bobgui_menu_button_get_icon_name (menu_button))
    g_object_notify_by_pspec (G_OBJECT (menu_button), menu_button_props[PROP_ICON_NAME]);

  box = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 0);
  bobgui_widget_set_hexpand (box, FALSE);

  arrow = bobgui_builtin_icon_new ("arrow");
  menu_button->arrow_widget = arrow;

  inner_widget = bobgui_gizmo_new_with_role ("contents",
                                          BOBGUI_ACCESSIBLE_ROLE_GROUP,
                                          NULL,
                                          NULL,
                                          NULL,
                                          NULL,
                                          (BobguiGizmoFocusFunc)bobgui_widget_focus_child,
                                          NULL);

  bobgui_widget_set_layout_manager (inner_widget, bobgui_bin_layout_new ());
  bobgui_widget_set_hexpand (inner_widget, TRUE);
  if (child)
    bobgui_widget_set_parent (child, inner_widget);

  bobgui_box_append (BOBGUI_BOX (box), inner_widget);
  bobgui_box_append (BOBGUI_BOX (box), arrow);
  bobgui_button_set_child (BOBGUI_BUTTON (menu_button->button), box);

  menu_button->child = child;

  menu_button->image_widget = NULL;
  menu_button->label_widget = NULL;

  update_arrow (menu_button);

  g_object_notify_by_pspec (G_OBJECT (menu_button), menu_button_props[PROP_CHILD]);

  g_object_thaw_notify (G_OBJECT (menu_button));
}

/**
 * bobgui_menu_button_get_child:
 * @menu_button: a `BobguiMenuButton`
 *
 * Gets the child widget of @menu_button.
 *
 * Returns: (nullable) (transfer none): the child widget of @menu_button
 *
 * Since: 4.6
 */
BobguiWidget *
bobgui_menu_button_get_child (BobguiMenuButton *menu_button)
{
  g_return_val_if_fail (BOBGUI_IS_MENU_BUTTON (menu_button), NULL);

  return menu_button->child;
}

/**
 * bobgui_menu_button_set_active:
 * @menu_button: a `BobguiMenuButton`
 * @active: whether the menu button is active
 *
 * Sets whether the menu button is active.
 *
 * Since: 4.10
 */
void
bobgui_menu_button_set_active (BobguiMenuButton *menu_button,
                            gboolean       active)
{
  g_return_if_fail (BOBGUI_IS_MENU_BUTTON (menu_button));

  if (active == bobgui_menu_button_get_active (menu_button))
    return;

  bobgui_toggle_button_set_active (BOBGUI_TOGGLE_BUTTON (menu_button->button),
                                active);

  g_object_notify_by_pspec (G_OBJECT (menu_button), menu_button_props[PROP_ACTIVE]);
}

/**
 * bobgui_menu_button_get_active:
 * @menu_button: a `BobguiMenuButton`
 *
 * Returns whether the menu button is active.
 *
 * Returns: TRUE if the button is active
 *
 * Since: 4.10
 */
gboolean
bobgui_menu_button_get_active (BobguiMenuButton *menu_button)
{
  g_return_val_if_fail (BOBGUI_IS_MENU_BUTTON (menu_button), FALSE);

  return bobgui_toggle_button_get_active (BOBGUI_TOGGLE_BUTTON (menu_button->button));
}

/**
 * bobgui_menu_button_set_can_shrink:
 * @menu_button: a menu button
 * @can_shrink: whether the button can shrink
 *
 * Sets whether the button size can be smaller than the natural size of
 * its contents.
 *
 * For text buttons, setting @can_shrink to true will ellipsize the label.
 *
 * For icon buttons, this function has no effect.
 *
 * Since: 4.12
 */
void
bobgui_menu_button_set_can_shrink (BobguiMenuButton *menu_button,
                                gboolean       can_shrink)
{
  g_return_if_fail (BOBGUI_IS_MENU_BUTTON (menu_button));

  can_shrink = !!can_shrink;

  if (menu_button->can_shrink == can_shrink)
    return;

  menu_button->can_shrink = can_shrink;

  if (menu_button->label_widget != NULL)
    {
      bobgui_label_set_ellipsize (BOBGUI_LABEL (menu_button->label_widget),
                               can_shrink ? PANGO_ELLIPSIZE_END
                                          : PANGO_ELLIPSIZE_NONE);
    }

  g_object_notify_by_pspec (G_OBJECT (menu_button), menu_button_props[PROP_CAN_SHRINK]);
}

/**
 * bobgui_menu_button_get_can_shrink:
 * @menu_button: a button
 *
 * Retrieves whether the button can be smaller than the natural
 * size of its contents.
 *
 * Returns: true if the button can shrink, and false otherwise
 *
 * Since: 4.12
 */
gboolean
bobgui_menu_button_get_can_shrink (BobguiMenuButton *menu_button)
{
  g_return_val_if_fail (BOBGUI_IS_MENU_BUTTON (menu_button), FALSE);

  return menu_button->can_shrink;
}
