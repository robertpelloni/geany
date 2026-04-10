/* BOBGUI - The Bobgui Framework
 *
 * Copyright (C) 2003 Sun Microsystems, Inc.
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
 *
 * Authors:
 *      Mark McLoughlin <mark@skynet.ie>
 */

/**
 * BobguiExpander:
 *
 * Allows the user to reveal or conceal a child widget.
 *
 * <picture>
 *   <source srcset="expander-dark.png" media="(prefers-color-scheme: dark)">
 *   <img alt="An example BobguiExpander" src="expander.png">
 * </picture>
 *
 * This is similar to the triangles used in a `BobguiTreeView`.
 *
 * Normally you use an expander as you would use a frame; you create
 * the child widget and use [method@Bobgui.Expander.set_child] to add it
 * to the expander. When the expander is toggled, it will take care of
 * showing and hiding the child automatically.
 *
 * # Special Usage
 *
 * There are situations in which you may prefer to show and hide the
 * expanded widget yourself, such as when you want to actually create
 * the widget at expansion time. In this case, create a `BobguiExpander`
 * but do not add a child to it. The expander widget has an
 * [property@Bobgui.Expander:expanded] property which can be used to
 * monitor its expansion state. You should watch this property with
 * a signal connection as follows:
 *
 * ```c
 * static void
 * expander_callback (GObject    *object,
 *                    GParamSpec *param_spec,
 *                    gpointer    user_data)
 * {
 *   BobguiExpander *expander;
 *
 *   expander = BOBGUI_EXPANDER (object);
 *
 *   if (bobgui_expander_get_expanded (expander))
 *     {
 *       // Show or create widgets
 *     }
 *   else
 *     {
 *       // Hide or destroy widgets
 *     }
 * }
 *
 * static void
 * create_expander (void)
 * {
 *   BobguiWidget *expander = bobgui_expander_new_with_mnemonic ("_More Options");
 *   g_signal_connect (expander, "notify::expanded",
 *                     G_CALLBACK (expander_callback), NULL);
 *
 *   // ...
 * }
 * ```
 *
 * # BobguiExpander as BobguiBuildable
 *
 * An example of a UI definition fragment with BobguiExpander:
 *
 * ```xml
 * <object class="BobguiExpander">
 *   <property name="label-widget">
 *     <object class="BobguiLabel" id="expander-label"/>
 *   </property>
 *   <property name="child">
 *     <object class="BobguiEntry" id="expander-content"/>
 *   </property>
 * </object>
 * ```
 *
 * # CSS nodes
 *
 * ```
 * expander-widget
 * ╰── box
 *     ├── title
 *     │   ├── expander
 *     │   ╰── <label widget>
 *     ╰── <child>
 * ```
 *
 * `BobguiExpander` has a main node `expander-widget`, and subnode `box` containing
 * the title and child widget. The box subnode `title` contains node `expander`,
 * i.e. the expand/collapse arrow; then the label widget if any. The arrow of an
 * expander that is showing its child gets the `:checked` pseudoclass set on it.
 *
 * # Accessibility
 *
 * `BobguiExpander` uses the [enum@Bobgui.AccessibleRole.button] role.
 */

#include "config.h"

#include "bobguiexpander.h"

#include "bobguibox.h"
#include "bobguibuildable.h"
#include "bobguidropcontrollermotion.h"
#include "bobguibuiltiniconprivate.h"
#include "bobguigestureclick.h"
#include "bobguigesturesingle.h"
#include "bobguilabel.h"
#include "bobguimarshalers.h"
#include "bobguimain.h"
#include "bobguiprivate.h"
#include "bobguiwidgetprivate.h"
#include "bobguibuilderprivate.h"

#include <string.h>

#define TIMEOUT_EXPAND 500

enum
{
  PROP_0,
  PROP_EXPANDED,
  PROP_LABEL,
  PROP_USE_UNDERLINE,
  PROP_USE_MARKUP,
  PROP_LABEL_WIDGET,
  PROP_RESIZE_TOPLEVEL,
  PROP_CHILD
};

typedef struct _BobguiExpanderClass   BobguiExpanderClass;

struct _BobguiExpander
{
  BobguiWidget parent_instance;

  BobguiWidget        *label_widget;

  BobguiWidget        *box;
  BobguiWidget        *title_widget;
  BobguiWidget        *arrow_widget;
  BobguiWidget        *child;

  guint             expand_timer;

  guint             expanded        : 1;
  guint             use_underline   : 1;
  guint             use_markup      : 1;
  guint             resize_toplevel : 1;
};

struct _BobguiExpanderClass
{
  BobguiWidgetClass parent_class;

  void (* activate) (BobguiExpander *expander);
};

static void bobgui_expander_dispose      (GObject          *object);
static void bobgui_expander_set_property (GObject          *object,
                                       guint             prop_id,
                                       const GValue     *value,
                                       GParamSpec       *pspec);
static void bobgui_expander_get_property (GObject          *object,
                                       guint             prop_id,
                                       GValue           *value,
                                       GParamSpec       *pspec);

static void     bobgui_expander_size_allocate  (BobguiWidget        *widget,
                                             int               width,
                                             int               height,
                                             int               baseline);
static gboolean bobgui_expander_focus          (BobguiWidget        *widget,
                                             BobguiDirectionType  direction);

static void bobgui_expander_activate (BobguiExpander *expander);


/* BobguiBuildable */
static void bobgui_expander_buildable_init           (BobguiBuildableIface *iface);
static void bobgui_expander_buildable_add_child      (BobguiBuildable *buildable,
                                                   BobguiBuilder   *builder,
                                                   GObject      *child,
                                                   const char   *type);


/* BobguiWidget      */
static void bobgui_expander_measure (BobguiWidget      *widget,
                                  BobguiOrientation  orientation,
                                  int             for_size,
                                  int            *minimum,
                                  int            *natural,
                                  int            *minimum_baseline,
                                  int            *natural_baseline);

/* Gestures */
static void     gesture_click_released_cb (BobguiGestureClick *gesture,
                                           int              n_press,
                                           double           x,
                                           double           y,
                                           BobguiExpander     *expander);

G_DEFINE_TYPE_WITH_CODE (BobguiExpander, bobgui_expander, BOBGUI_TYPE_WIDGET,
                         G_IMPLEMENT_INTERFACE (BOBGUI_TYPE_BUILDABLE,
                                                bobgui_expander_buildable_init))

static gboolean
expand_timeout (gpointer data)
{
  BobguiExpander *expander = BOBGUI_EXPANDER (data);

  expander->expand_timer = 0;
  bobgui_expander_set_expanded (expander, TRUE);

  return FALSE;
}

static void
bobgui_expander_drag_enter (BobguiDropControllerMotion *motion,
                         double                   x,
                         double                   y,
                         BobguiExpander             *expander)
{
  if (!expander->expanded && !expander->expand_timer)
    {
      expander->expand_timer = g_timeout_add (TIMEOUT_EXPAND, (GSourceFunc) expand_timeout, expander);
      gdk_source_set_static_name_by_id (expander->expand_timer, "[bobgui] expand_timeout");
    }
}

static void
bobgui_expander_drag_leave (BobguiDropControllerMotion *motion,
                         BobguiExpander             *expander)
{
  if (expander->expand_timer)
    {
      g_source_remove (expander->expand_timer);
      expander->expand_timer = 0;
    }
}

static BobguiSizeRequestMode
bobgui_expander_get_request_mode (BobguiWidget *widget)
{
  BobguiExpander *expander = BOBGUI_EXPANDER (widget);

  if (expander->child)
    return bobgui_widget_get_request_mode (expander->child);
  else
    return BOBGUI_SIZE_REQUEST_CONSTANT_SIZE;
}

static void
bobgui_expander_compute_expand (BobguiWidget *widget,
                             gboolean  *hexpand,
                             gboolean  *vexpand)
{
  BobguiExpander *expander = BOBGUI_EXPANDER (widget);

  if (expander->child)
    {
      *hexpand = bobgui_widget_compute_expand (expander->child, BOBGUI_ORIENTATION_HORIZONTAL);
      *vexpand = bobgui_widget_compute_expand (expander->child, BOBGUI_ORIENTATION_VERTICAL);
    }
  else
    {
      *hexpand = FALSE;
      *vexpand = FALSE;
    }
}

static void
bobgui_expander_class_init (BobguiExpanderClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (klass);
  guint activate_signal;

  gobject_class->dispose = bobgui_expander_dispose;
  gobject_class->set_property = bobgui_expander_set_property;
  gobject_class->get_property = bobgui_expander_get_property;

  widget_class->size_allocate = bobgui_expander_size_allocate;
  widget_class->focus = bobgui_expander_focus;
  widget_class->grab_focus = bobgui_widget_grab_focus_self;
  widget_class->measure = bobgui_expander_measure;
  widget_class->compute_expand = bobgui_expander_compute_expand;
  widget_class->get_request_mode = bobgui_expander_get_request_mode;

  klass->activate = bobgui_expander_activate;

  /**
   * BobguiExpander:expanded:
   *
   * Whether the expander has been opened to reveal the child.
   */
  g_object_class_install_property (gobject_class,
                                   PROP_EXPANDED,
                                   g_param_spec_boolean ("expanded", NULL, NULL,
                                                         FALSE,
                                                         BOBGUI_PARAM_READWRITE|G_PARAM_CONSTRUCT|G_PARAM_EXPLICIT_NOTIFY));

  /**
   * BobguiExpander:label:
   *
   * The text of the expanders label.
   */
  g_object_class_install_property (gobject_class,
                                   PROP_LABEL,
                                   g_param_spec_string ("label", NULL, NULL,
                                                        NULL,
                                                        BOBGUI_PARAM_READWRITE|G_PARAM_CONSTRUCT));

  /**
   * BobguiExpander:use-underline:
   *
   * Whether an underline in the text indicates a mnemonic.
   */
  g_object_class_install_property (gobject_class,
                                   PROP_USE_UNDERLINE,
                                   g_param_spec_boolean ("use-underline", NULL, NULL,
                                                         FALSE,
                                                         BOBGUI_PARAM_READWRITE|G_PARAM_CONSTRUCT|G_PARAM_EXPLICIT_NOTIFY));

  /**
   * BobguiExpander:use-markup:
   *
   * Whether the text in the label is Pango markup.
   */
  g_object_class_install_property (gobject_class,
                                   PROP_USE_MARKUP,
                                   g_param_spec_boolean ("use-markup", NULL, NULL,
                                                         FALSE,
                                                         BOBGUI_PARAM_READWRITE|G_PARAM_CONSTRUCT|G_PARAM_EXPLICIT_NOTIFY));

  /**
   * BobguiExpander:label-widget:
   *
   * A widget to display instead of the usual expander label.
   */
  g_object_class_install_property (gobject_class,
                                   PROP_LABEL_WIDGET,
                                   g_param_spec_object ("label-widget", NULL, NULL,
                                                        BOBGUI_TYPE_WIDGET,
                                                        BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

  /**
   * BobguiExpander:resize-toplevel:
   *
   * When this property is %TRUE, the expander will resize the toplevel
   * widget containing the expander upon expanding and collapsing.
   */
  g_object_class_install_property (gobject_class,
                                   PROP_RESIZE_TOPLEVEL,
                                   g_param_spec_boolean ("resize-toplevel", NULL, NULL,
                                                         FALSE,
                                                         BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

  /**
   * BobguiExpander:child:
   *
   * The child widget.
   */
  g_object_class_install_property (gobject_class,
                                   PROP_CHILD,
                                   g_param_spec_object ("child", NULL, NULL,
                                                        BOBGUI_TYPE_WIDGET,
                                                        BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

  /**
   * BobguiExpander::activate:
   * @expander: the `BobguiExpander` that emitted the signal
   *
   * Activates the `BobguiExpander`.
   */
  activate_signal =
    g_signal_new (I_("activate"),
                  G_TYPE_FROM_CLASS (gobject_class),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                  G_STRUCT_OFFSET (BobguiExpanderClass, activate),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 0);

  bobgui_widget_class_set_activate_signal (widget_class, activate_signal);
  bobgui_widget_class_set_css_name (widget_class, I_("expander-widget"));
  bobgui_widget_class_set_accessible_role (widget_class, BOBGUI_ACCESSIBLE_ROLE_BUTTON);
}

static void
bobgui_expander_init (BobguiExpander *expander)
{
  BobguiGesture *gesture;
  BobguiEventController *controller;

  expander->label_widget = NULL;
  expander->child = NULL;

  expander->expanded = FALSE;
  expander->use_underline = FALSE;
  expander->use_markup = FALSE;
  expander->expand_timer = 0;
  expander->resize_toplevel = 0;

  bobgui_widget_set_focusable (BOBGUI_WIDGET (expander), TRUE);

  expander->box = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 0);
  bobgui_widget_set_parent (expander->box, BOBGUI_WIDGET (expander));

  expander->title_widget = g_object_new (BOBGUI_TYPE_BOX,
                                         "css-name", "title",
                                         NULL);
  bobgui_box_append (BOBGUI_BOX (expander->box), expander->title_widget);

  expander->arrow_widget = bobgui_builtin_icon_new ("expander");
  bobgui_widget_add_css_class (expander->arrow_widget, "horizontal");
  bobgui_box_append (BOBGUI_BOX (expander->title_widget), expander->arrow_widget);

  controller = bobgui_drop_controller_motion_new ();
  g_signal_connect (controller, "enter", G_CALLBACK (bobgui_expander_drag_enter), expander);
  g_signal_connect (controller, "leave", G_CALLBACK (bobgui_expander_drag_leave), expander);
  bobgui_widget_add_controller (BOBGUI_WIDGET (expander), controller);

  gesture = bobgui_gesture_click_new ();
  bobgui_gesture_single_set_button (BOBGUI_GESTURE_SINGLE (gesture),
                                 GDK_BUTTON_PRIMARY);
  bobgui_gesture_single_set_touch_only (BOBGUI_GESTURE_SINGLE (gesture),
                                     FALSE);
  g_signal_connect (gesture, "released",
                    G_CALLBACK (gesture_click_released_cb), expander);
  bobgui_event_controller_set_propagation_phase (BOBGUI_EVENT_CONTROLLER (gesture),
                                              BOBGUI_PHASE_BUBBLE);
  bobgui_widget_add_controller (BOBGUI_WIDGET (expander->title_widget), BOBGUI_EVENT_CONTROLLER (gesture));

  bobgui_accessible_update_state (BOBGUI_ACCESSIBLE (expander),
                               BOBGUI_ACCESSIBLE_STATE_EXPANDED, FALSE,
                               -1);
}

static BobguiBuildableIface *parent_buildable_iface;

static void
bobgui_expander_buildable_add_child (BobguiBuildable  *buildable,
                                  BobguiBuilder    *builder,
                                  GObject       *child,
                                  const char    *type)
{
  if (g_strcmp0 (type, "label") == 0)
    {
      bobgui_buildable_child_deprecation_warning (buildable, builder, "label", "label-widget");
      bobgui_expander_set_label_widget (BOBGUI_EXPANDER (buildable), BOBGUI_WIDGET (child));
    }
  else if (BOBGUI_IS_WIDGET (child))
    {
      bobgui_buildable_child_deprecation_warning (buildable, builder, NULL, "child");
      bobgui_expander_set_child (BOBGUI_EXPANDER (buildable), BOBGUI_WIDGET (child));
    }
  else
    {
      parent_buildable_iface->add_child (buildable, builder, child, type);
    }
}

static void
bobgui_expander_buildable_init (BobguiBuildableIface *iface)
{
  parent_buildable_iface = g_type_interface_peek_parent (iface);

  iface->add_child = bobgui_expander_buildable_add_child;
}

static void
bobgui_expander_set_property (GObject      *object,
                           guint         prop_id,
                           const GValue *value,
                           GParamSpec   *pspec)
{
  BobguiExpander *expander = BOBGUI_EXPANDER (object);

  switch (prop_id)
    {
    case PROP_EXPANDED:
      bobgui_expander_set_expanded (expander, g_value_get_boolean (value));
      break;
    case PROP_LABEL:
      bobgui_expander_set_label (expander, g_value_get_string (value));
      break;
    case PROP_USE_UNDERLINE:
      bobgui_expander_set_use_underline (expander, g_value_get_boolean (value));
      break;
    case PROP_USE_MARKUP:
      bobgui_expander_set_use_markup (expander, g_value_get_boolean (value));
      break;
    case PROP_LABEL_WIDGET:
      bobgui_expander_set_label_widget (expander, g_value_get_object (value));
      break;
    case PROP_RESIZE_TOPLEVEL:
      bobgui_expander_set_resize_toplevel (expander, g_value_get_boolean (value));
      break;
    case PROP_CHILD:
      bobgui_expander_set_child (expander, g_value_get_object (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bobgui_expander_get_property (GObject    *object,
                           guint       prop_id,
                           GValue     *value,
                           GParamSpec *pspec)
{
  BobguiExpander *expander = BOBGUI_EXPANDER (object);

  switch (prop_id)
    {
    case PROP_EXPANDED:
      g_value_set_boolean (value, expander->expanded);
      break;
    case PROP_LABEL:
      g_value_set_string (value, bobgui_expander_get_label (expander));
      break;
    case PROP_USE_UNDERLINE:
      g_value_set_boolean (value, expander->use_underline);
      break;
    case PROP_USE_MARKUP:
      g_value_set_boolean (value, expander->use_markup);
      break;
    case PROP_LABEL_WIDGET:
      g_value_set_object (value,
                          expander->label_widget ?
                          G_OBJECT (expander->label_widget) : NULL);
      break;
    case PROP_RESIZE_TOPLEVEL:
      g_value_set_boolean (value, bobgui_expander_get_resize_toplevel (expander));
      break;
    case PROP_CHILD:
      g_value_set_object (value, bobgui_expander_get_child (expander));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bobgui_expander_dispose (GObject *object)
{
  BobguiExpander *expander = BOBGUI_EXPANDER (object);

  if (expander->expand_timer)
    {
      g_source_remove (expander->expand_timer);
      expander->expand_timer = 0;
    }

  /* If the expander is not expanded, we own the child */
  if (!expander->expanded)
    g_clear_object (&expander->child);

  if (expander->box)
    {
      bobgui_widget_unparent (expander->box);
      expander->box = NULL;
      expander->child = NULL;
      expander->label_widget = NULL;
      expander->arrow_widget = NULL;
    }

  G_OBJECT_CLASS (bobgui_expander_parent_class)->dispose (object);
}

static void
bobgui_expander_size_allocate (BobguiWidget *widget,
                            int        width,
                            int        height,
                            int        baseline)
{
  BobguiExpander *expander = BOBGUI_EXPANDER (widget);

  bobgui_widget_size_allocate (expander->box,
                            &(BobguiAllocation) {
                              0, 0,
                              width, height
                            }, baseline);
}

static void
gesture_click_released_cb (BobguiGestureClick *gesture,
                           int              n_press,
                           double           x,
                           double           y,
                           BobguiExpander     *expander)
{
  bobgui_widget_activate (BOBGUI_WIDGET (expander));
}

typedef enum
{
  FOCUS_NONE,
  FOCUS_WIDGET,
  FOCUS_LABEL,
  FOCUS_CHILD
} FocusSite;

static gboolean
focus_current_site (BobguiExpander      *expander,
                    BobguiDirectionType  direction)
{
  BobguiWidget *current_focus;

  current_focus = bobgui_widget_get_focus_child (BOBGUI_WIDGET (expander));

  if (!current_focus)
    return FALSE;

  return bobgui_widget_child_focus (current_focus, direction);
}

static gboolean
focus_in_site (BobguiExpander      *expander,
               FocusSite         site,
               BobguiDirectionType  direction)
{
  switch (site)
    {
    case FOCUS_WIDGET:
      bobgui_widget_grab_focus (BOBGUI_WIDGET (expander));
      return TRUE;
    case FOCUS_LABEL:
      if (expander->label_widget)
        return bobgui_widget_child_focus (expander->label_widget, direction);
      else
        return FALSE;
    case FOCUS_CHILD:
      {
        BobguiWidget *child = expander->child;

        if (child && bobgui_widget_get_child_visible (child))
          return bobgui_widget_child_focus (child, direction);
        else
          return FALSE;
      }
    case FOCUS_NONE:
    default:
      break;
    }

  g_assert_not_reached ();
  return FALSE;
}

static FocusSite
get_next_site (BobguiExpander      *expander,
               FocusSite         site,
               BobguiDirectionType  direction)
{
  gboolean ltr;

  ltr = bobgui_widget_get_direction (BOBGUI_WIDGET (expander)) != BOBGUI_TEXT_DIR_RTL;

  switch (site)
    {
    case FOCUS_NONE:
      switch (direction)
        {
        case BOBGUI_DIR_TAB_BACKWARD:
        case BOBGUI_DIR_LEFT:
        case BOBGUI_DIR_UP:
          return FOCUS_CHILD;
        case BOBGUI_DIR_TAB_FORWARD:
        case BOBGUI_DIR_DOWN:
        case BOBGUI_DIR_RIGHT:
        default:
          return FOCUS_WIDGET;
        }
      break;
    case FOCUS_WIDGET:
      switch (direction)
        {
        case BOBGUI_DIR_TAB_BACKWARD:
        case BOBGUI_DIR_UP:
          return FOCUS_NONE;
        case BOBGUI_DIR_LEFT:
          return ltr ? FOCUS_NONE : FOCUS_LABEL;
        case BOBGUI_DIR_TAB_FORWARD:
        case BOBGUI_DIR_DOWN:
        default:
          return FOCUS_LABEL;
        case BOBGUI_DIR_RIGHT:
          return ltr ? FOCUS_LABEL : FOCUS_NONE;
        }
      break;
    case FOCUS_LABEL:
      switch (direction)
        {
        case BOBGUI_DIR_TAB_BACKWARD:
        case BOBGUI_DIR_UP:
          return FOCUS_WIDGET;
        case BOBGUI_DIR_LEFT:
          return ltr ? FOCUS_WIDGET : FOCUS_CHILD;
        case BOBGUI_DIR_TAB_FORWARD:
        case BOBGUI_DIR_DOWN:
        default:
          return FOCUS_CHILD;
        case BOBGUI_DIR_RIGHT:
          return ltr ? FOCUS_CHILD : FOCUS_WIDGET;
        }
      break;
    case FOCUS_CHILD:
      switch (direction)
        {
        case BOBGUI_DIR_TAB_BACKWARD:
        case BOBGUI_DIR_LEFT:
        case BOBGUI_DIR_UP:
          return FOCUS_LABEL;
        case BOBGUI_DIR_TAB_FORWARD:
        case BOBGUI_DIR_DOWN:
        case BOBGUI_DIR_RIGHT:
        default:
          return FOCUS_NONE;
        }
      break;
    default:
      g_assert_not_reached ();
      break;
    }
  
  return FOCUS_NONE;
}

static void
bobgui_expander_resize_toplevel (BobguiExpander *expander)
{
  BobguiWidget *child = expander->child;

  if (child && expander->resize_toplevel &&
      bobgui_widget_get_realized (BOBGUI_WIDGET (expander)))
    {
      BobguiWidget *toplevel = BOBGUI_WIDGET (bobgui_widget_get_root (BOBGUI_WIDGET (expander)));

      if (BOBGUI_IS_WINDOW (toplevel) &&
          bobgui_widget_get_realized (toplevel))
        bobgui_widget_queue_resize (BOBGUI_WIDGET (expander));
    }
}

static gboolean
bobgui_expander_focus (BobguiWidget        *widget,
                    BobguiDirectionType  direction)
{
  BobguiExpander *expander = BOBGUI_EXPANDER (widget);

  if (!focus_current_site (expander, direction))
    {
      BobguiWidget *old_focus_child;
      gboolean widget_is_focus;
      FocusSite site = FOCUS_NONE;

      widget_is_focus = bobgui_widget_is_focus (widget);
      old_focus_child = bobgui_widget_get_focus_child (BOBGUI_WIDGET (widget));

      if (old_focus_child && old_focus_child == expander->label_widget)
        site = FOCUS_LABEL;
      else if (old_focus_child)
        site = FOCUS_CHILD;
      else if (widget_is_focus)
        site = FOCUS_WIDGET;

      while ((site = get_next_site (expander, site, direction)) != FOCUS_NONE)
        {
          if (focus_in_site (expander, site, direction))
            return TRUE;
        }

      return FALSE;
    }

  return TRUE;
}

static void
bobgui_expander_activate (BobguiExpander *expander)
{
  bobgui_expander_set_expanded (expander, !expander->expanded);
}

static void
bobgui_expander_measure (BobguiWidget      *widget,
                      BobguiOrientation  orientation,
                      int             for_size,
                      int            *minimum,
                      int            *natural,
                      int            *minimum_baseline,
                      int            *natural_baseline)
{
  BobguiExpander *expander = BOBGUI_EXPANDER (widget);

  bobgui_widget_measure (expander->box,
                       orientation,
                       for_size,
                       minimum, natural,
                       minimum_baseline, natural_baseline);
}

/**
 * bobgui_expander_new:
 * @label: (nullable): the text of the label
 *
 * Creates a new expander using @label as the text of the label.
 *
 * Returns: a new `BobguiExpander` widget.
 */
BobguiWidget *
bobgui_expander_new (const char *label)
{
  return g_object_new (BOBGUI_TYPE_EXPANDER, "label", label, NULL);
}

/**
 * bobgui_expander_new_with_mnemonic:
 * @label: (nullable): the text of the label with an underscore
 *   in front of the mnemonic character
 *
 * Creates a new expander using @label as the text of the label.
 *
 * If characters in @label are preceded by an underscore, they are
 * underlined. If you need a literal underscore character in a label,
 * use “__” (two underscores). The first underlined character represents
 * a keyboard accelerator called a mnemonic.
 *
 * Pressing Alt and that key activates the button.
 *
 * Returns: a new `BobguiExpander` widget.
 */
BobguiWidget *
bobgui_expander_new_with_mnemonic (const char *label)
{
  return g_object_new (BOBGUI_TYPE_EXPANDER,
                       "label", label,
                       "use-underline", TRUE,
                       NULL);
}

/**
 * bobgui_expander_set_expanded:
 * @expander: a `BobguiExpander`
 * @expanded: whether the child widget is revealed
 *
 * Sets the state of the expander.
 *
 * Set to %TRUE, if you want the child widget to be revealed,
 * and %FALSE if you want the child widget to be hidden.
 */
void
bobgui_expander_set_expanded (BobguiExpander *expander,
                           gboolean     expanded)
{
  BobguiWidget *child;

  g_return_if_fail (BOBGUI_IS_EXPANDER (expander));

  expanded = expanded != FALSE;

  if (expander->expanded == expanded)
    return;

  expander->expanded = expanded;

  if (expander->expanded)
    bobgui_widget_set_state_flags (expander->arrow_widget, BOBGUI_STATE_FLAG_CHECKED, FALSE);
  else
    bobgui_widget_unset_state_flags (expander->arrow_widget, BOBGUI_STATE_FLAG_CHECKED);

  child = expander->child;

  if (child)
    {
      /* Transfer the ownership of the child to the box when
       * expanded is set, and then back to us when it is unset
       */
      if (expander->expanded)
        {
          bobgui_box_append (BOBGUI_BOX (expander->box), child);
          g_object_unref (expander->child);
          bobgui_accessible_update_relation (BOBGUI_ACCESSIBLE (expander),
                                          BOBGUI_ACCESSIBLE_RELATION_CONTROLS, expander->child, NULL,
                                          -1);
        }
      else
        {
          bobgui_accessible_reset_relation (BOBGUI_ACCESSIBLE (expander),
                                         BOBGUI_ACCESSIBLE_RELATION_CONTROLS);
          g_object_ref (expander->child);
          bobgui_box_remove (BOBGUI_BOX (expander->box), child);
        }
      bobgui_expander_resize_toplevel (expander);
    }

  bobgui_accessible_update_state (BOBGUI_ACCESSIBLE (expander),
                               BOBGUI_ACCESSIBLE_STATE_EXPANDED, expanded,
                               -1);

  g_object_notify (G_OBJECT (expander), "expanded");
}

/**
 * bobgui_expander_get_expanded:
 * @expander:a `BobguiExpander`
 *
 * Queries a `BobguiExpander` and returns its current state.
 *
 * Returns %TRUE if the child widget is revealed.
 *
 * Returns: the current state of the expander
 */
gboolean
bobgui_expander_get_expanded (BobguiExpander *expander)
{
  g_return_val_if_fail (BOBGUI_IS_EXPANDER (expander), FALSE);

  return expander->expanded;
}

/**
 * bobgui_expander_set_label:
 * @expander: a `BobguiExpander`
 * @label: (nullable): a string
 *
 * Sets the text of the label of the expander to @label.
 *
 * This will also clear any previously set labels.
 */
void
bobgui_expander_set_label (BobguiExpander *expander,
                        const char *label)
{
  g_return_if_fail (BOBGUI_IS_EXPANDER (expander));

  if (!label)
    {
      bobgui_expander_set_label_widget (expander, NULL);
    }
  else
    {
      BobguiWidget *child;

      child = bobgui_label_new (label);
      bobgui_label_set_use_underline (BOBGUI_LABEL (child), expander->use_underline);
      bobgui_label_set_use_markup (BOBGUI_LABEL (child), expander->use_markup);

      bobgui_expander_set_label_widget (expander, child);
    }

  g_object_notify (G_OBJECT (expander), "label");
}

/**
 * bobgui_expander_get_label:
 * @expander: a `BobguiExpander`
 *
 * Fetches the text from a label widget.
 *
 * This is including any embedded underlines indicating mnemonics and
 * Pango markup, as set by [method@Bobgui.Expander.set_label]. If the label
 * text has not been set the return value will be %NULL. This will be the
 * case if you create an empty button with bobgui_button_new() to use as a
 * container.
 *
 * Returns: (nullable): The text of the label widget. This string is owned
 *   by the widget and must not be modified or freed.
 */
const char *
bobgui_expander_get_label (BobguiExpander *expander)
{
  g_return_val_if_fail (BOBGUI_IS_EXPANDER (expander), NULL);

  if (BOBGUI_IS_LABEL (expander->label_widget))
    return bobgui_label_get_label (BOBGUI_LABEL (expander->label_widget));
  else
    return NULL;
}

/**
 * bobgui_expander_set_use_underline:
 * @expander: a `BobguiExpander`
 * @use_underline: %TRUE if underlines in the text indicate mnemonics
 *
 * If true, an underline in the text indicates a mnemonic.
 */
void
bobgui_expander_set_use_underline (BobguiExpander *expander,
                                gboolean     use_underline)
{
  g_return_if_fail (BOBGUI_IS_EXPANDER (expander));

  use_underline = use_underline != FALSE;

  if (expander->use_underline != use_underline)
    {
      expander->use_underline = use_underline;

      if (BOBGUI_IS_LABEL (expander->label_widget))
        bobgui_label_set_use_underline (BOBGUI_LABEL (expander->label_widget), use_underline);

      g_object_notify (G_OBJECT (expander), "use-underline");
    }
}

/**
 * bobgui_expander_get_use_underline:
 * @expander: a `BobguiExpander`
 *
 * Returns whether an underline in the text indicates a mnemonic.
 *
 * Returns: %TRUE if an embedded underline in the expander
 *   label indicates the mnemonic accelerator keys
 */
gboolean
bobgui_expander_get_use_underline (BobguiExpander *expander)
{
  g_return_val_if_fail (BOBGUI_IS_EXPANDER (expander), FALSE);

  return expander->use_underline;
}

/**
 * bobgui_expander_set_use_markup:
 * @expander: a `BobguiExpander`
 * @use_markup: %TRUE if the label’s text should be parsed for markup
 *
 * Sets whether the text of the label contains Pango markup.
 */
void
bobgui_expander_set_use_markup (BobguiExpander *expander,
                             gboolean     use_markup)
{
  g_return_if_fail (BOBGUI_IS_EXPANDER (expander));

  use_markup = use_markup != FALSE;

  if (expander->use_markup != use_markup)
    {
      expander->use_markup = use_markup;

      if (BOBGUI_IS_LABEL (expander->label_widget))
        bobgui_label_set_use_markup (BOBGUI_LABEL (expander->label_widget), use_markup);

      g_object_notify (G_OBJECT (expander), "use-markup");
    }
}

/**
 * bobgui_expander_get_use_markup:
 * @expander: a `BobguiExpander`
 *
 * Returns whether the label’s text is interpreted as Pango markup.
 *
 * Returns: %TRUE if the label’s text will be parsed for markup
 */
gboolean
bobgui_expander_get_use_markup (BobguiExpander *expander)
{
  g_return_val_if_fail (BOBGUI_IS_EXPANDER (expander), FALSE);

  return expander->use_markup;
}

/**
 * bobgui_expander_set_label_widget:
 * @expander: a `BobguiExpander`
 * @label_widget: (nullable): the new label widget
 *
 * Set the label widget for the expander.
 *
 * This is the widget that will appear embedded alongside
 * the expander arrow.
 */
void
bobgui_expander_set_label_widget (BobguiExpander *expander,
                               BobguiWidget   *label_widget)
{
  BobguiWidget *widget;

  g_return_if_fail (BOBGUI_IS_EXPANDER (expander));
  g_return_if_fail (label_widget == NULL || expander->label_widget == label_widget || bobgui_widget_get_parent (label_widget) == NULL);

  if (expander->label_widget == label_widget)
    return;

  if (expander->label_widget)
    bobgui_box_remove (BOBGUI_BOX (expander->title_widget), expander->label_widget);

  expander->label_widget = label_widget;
  widget = BOBGUI_WIDGET (expander);

  if (label_widget)
    {
      expander->label_widget = label_widget;

      bobgui_box_append (BOBGUI_BOX (expander->title_widget), label_widget);
    }

  if (bobgui_widget_get_visible (widget))
    bobgui_widget_queue_resize (widget);

  g_object_freeze_notify (G_OBJECT (expander));
  g_object_notify (G_OBJECT (expander), "label-widget");
  g_object_notify (G_OBJECT (expander), "label");
  g_object_thaw_notify (G_OBJECT (expander));
}

/**
 * bobgui_expander_get_label_widget:
 * @expander: a `BobguiExpander`
 *
 * Retrieves the label widget for the frame.
 *
 * Returns: (nullable) (transfer none): the label widget
 */
BobguiWidget *
bobgui_expander_get_label_widget (BobguiExpander *expander)
{
  g_return_val_if_fail (BOBGUI_IS_EXPANDER (expander), NULL);

  return expander->label_widget;
}

/**
 * bobgui_expander_set_resize_toplevel:
 * @expander: a `BobguiExpander`
 * @resize_toplevel: whether to resize the toplevel
 *
 * Sets whether the expander will resize the toplevel widget
 * containing the expander upon resizing and collapsing.
 */
void
bobgui_expander_set_resize_toplevel (BobguiExpander *expander,
                                  gboolean     resize_toplevel)
{
  g_return_if_fail (BOBGUI_IS_EXPANDER (expander));

  if (expander->resize_toplevel != resize_toplevel)
    {
      expander->resize_toplevel = resize_toplevel ? TRUE : FALSE;
      g_object_notify (G_OBJECT (expander), "resize-toplevel");
    }
}

/**
 * bobgui_expander_get_resize_toplevel:
 * @expander: a `BobguiExpander`
 *
 * Returns whether the expander will resize the toplevel widget
 * containing the expander upon resizing and collapsing.
 *
 * Returns: the “resize toplevel” setting.
 */
gboolean
bobgui_expander_get_resize_toplevel (BobguiExpander *expander)
{
  g_return_val_if_fail (BOBGUI_IS_EXPANDER (expander), FALSE);

  return expander->resize_toplevel;
}

/**
 * bobgui_expander_set_child:
 * @expander: a `BobguiExpander`
 * @child: (nullable): the child widget
 *
 * Sets the child widget of @expander.
 */
void
bobgui_expander_set_child (BobguiExpander *expander,
                        BobguiWidget   *child)
{
  g_return_if_fail (BOBGUI_IS_EXPANDER (expander));
  g_return_if_fail (child == NULL || expander->child == child || bobgui_widget_get_parent (child) == NULL);

  if (expander->child == child)
    return;

  if (expander->child)
    {
      if (!expander->expanded)
        g_object_unref (expander->child);
      else
        bobgui_box_remove (BOBGUI_BOX (expander->box), expander->child);
    }

  expander->child = child;

  if (expander->child)
    {
      /* We only add the child to the box if the expander is
       * expanded; otherwise we just claim ownership of the
       * child by sinking its floating reference, or acquiring
       * an additional reference to it. The reference will be
       * dropped once the expander is expanded
       */
      if (expander->expanded)
        {
          bobgui_box_append (BOBGUI_BOX (expander->box), expander->child);
          bobgui_accessible_update_relation (BOBGUI_ACCESSIBLE (expander),
                                          BOBGUI_ACCESSIBLE_RELATION_CONTROLS, expander->child, NULL,
                                          -1);
        }
      else
        {
          bobgui_accessible_reset_relation (BOBGUI_ACCESSIBLE (expander),
                                         BOBGUI_ACCESSIBLE_RELATION_CONTROLS);
          g_object_ref_sink (expander->child);
        }
    }
  else
    {
      bobgui_accessible_reset_relation (BOBGUI_ACCESSIBLE (expander),
                                     BOBGUI_ACCESSIBLE_RELATION_CONTROLS);
    }

  g_object_notify (G_OBJECT (expander), "child");
}

/**
 * bobgui_expander_get_child:
 * @expander: a `BobguiExpander`
 *
 * Gets the child widget of @expander.
 *
 * Returns: (nullable) (transfer none): the child widget of @expander
 */
BobguiWidget *
bobgui_expander_get_child (BobguiExpander *expander)
{
  g_return_val_if_fail (BOBGUI_IS_EXPANDER (expander), NULL);

  return expander->child;
}

