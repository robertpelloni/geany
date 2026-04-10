/*
 * Copyright (C) 2007-2010 Openismus GmbH
 * Copyright (C) 2013 Red Hat, Inc.
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
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 *
 * Authors:
 *      Tristan Van Berkom <tristanvb@openismus.com>
 *      Matthias Clasen <mclasen@redhat.com>
 *      William Jon McCann <jmccann@redhat.com>
 */

/* Preamble {{{1 */

/**
 * BobguiFlowBox:
 *
 * Puts child widgets in a reflowing grid.
 *
 * <picture>
 *   <source srcset="flow-box-dark.png" media="(prefers-color-scheme: dark)">
 *   <img alt="An example BobguiFlowBox" src="flow-box.png">
 * </picture>
 *
 * For instance, with the horizontal orientation, the widgets will be
 * arranged from left to right, starting a new row under the previous
 * row when necessary. Reducing the width in this case will require more
 * rows, so a larger height will be requested.
 *
 * Likewise, with the vertical orientation, the widgets will be arranged
 * from top to bottom, starting a new column to the right when necessary.
 * Reducing the height will require more columns, so a larger width will
 * be requested.
 *
 * The size request of a `BobguiFlowBox` alone may not be what you expect;
 * if you need to be able to shrink it along both axes and dynamically
 * reflow its children, you may have to wrap it in a `BobguiScrolledWindow`
 * to enable that.
 *
 * The children of a `BobguiFlowBox` can be dynamically sorted and filtered.
 *
 * Although a `BobguiFlowBox` must have only `BobguiFlowBoxChild` children, you
 * can add any kind of widget to it via [method@Bobgui.FlowBox.insert], and a
 * `BobguiFlowBoxChild` widget will automatically be inserted between the box
 * and the widget.
 *
 * Also see [class@Bobgui.ListBox].
 *
 * # Shortcuts and Gestures
 *
 * The following signals have default keybindings:
 *
 * - [signal@Bobgui.FlowBox::move-cursor]
 * - [signal@Bobgui.FlowBox::select-all]
 * - [signal@Bobgui.FlowBox::toggle-cursor-child]
 * - [signal@Bobgui.FlowBox::unselect-all]
 *
 * # CSS nodes
 *
 * ```
 * flowbox
 * ├── flowboxchild
 * │   ╰── <child>
 * ├── flowboxchild
 * │   ╰── <child>
 * ┊
 * ╰── [rubberband]
 * ```
 *
 * `BobguiFlowBox` uses a single CSS node with name flowbox. `BobguiFlowBoxChild`
 * uses a single CSS node with name flowboxchild. For rubberband selection,
 * a subnode with name rubberband is used.
 *
 * # Accessibility
 *
 * `BobguiFlowBox` uses the [enum@Bobgui.AccessibleRole.grid] role, and `BobguiFlowBoxChild`
 * uses the [enum@Bobgui.AccessibleRole.grid_cell] role.
 */

/**
 * BobguiFlowBoxChild:
 *
 * The kind of widget that can be added to a `BobguiFlowBox`.
 *
 * [class@Bobgui.FlowBox] will automatically wrap its children in a `BobguiFlowBoxChild`
 * when necessary.
 */

#include <config.h>

#include "bobguiflowboxprivate.h"

#include "bobguiaccessible.h"
#include "bobguiadjustment.h"
#include "bobguibinlayout.h"
#include "bobguibuildable.h"
#include "bobguibuilderprivate.h"
#include "bobguicsscolorvalueprivate.h"
#include "bobguieventcontrollerkey.h"
#include "bobguigestureclick.h"
#include "bobguigesturedrag.h"
#include "bobguimain.h"
#include "bobguimarshalers.h"
#include "bobguiorientable.h"
#include "bobguiprivate.h"
#include "bobguisizerequest.h"
#include "bobguisnapshot.h"
#include "bobguirenderbackgroundprivate.h"
#include "bobguitypebuiltins.h"
#include "bobguiviewport.h"
#include "bobguiwidgetprivate.h"

/* Forward declarations and utilities {{{1 */

static void bobgui_flow_box_update_cursor       (BobguiFlowBox      *box,
                                              BobguiFlowBoxChild *child);
static void bobgui_flow_box_select_and_activate (BobguiFlowBox      *box,
                                              BobguiFlowBoxChild *child);
static void bobgui_flow_box_update_selection    (BobguiFlowBox      *box,
                                              BobguiFlowBoxChild *child,
                                              gboolean         modify,
                                              gboolean         extend);
static void bobgui_flow_box_apply_filter        (BobguiFlowBox      *box,
                                              BobguiFlowBoxChild *child);
static void bobgui_flow_box_apply_sort          (BobguiFlowBox      *box,
                                              BobguiFlowBoxChild *child);
static int bobgui_flow_box_sort                 (BobguiFlowBoxChild *a,
                                              BobguiFlowBoxChild *b,
                                              BobguiFlowBox      *box);

static void bobgui_flow_box_bound_model_changed (GListModel *list,
                                              guint       position,
                                              guint       removed,
                                              guint       added,
                                              gpointer    user_data);

static void bobgui_flow_box_set_accept_unpaired_release (BobguiFlowBox *box,
                                                      gboolean    accept);

static void bobgui_flow_box_check_model_compat  (BobguiFlowBox *box);

static void
path_from_horizontal_line_rects (cairo_t      *cr,
                                 GdkRectangle *lines,
                                 int           n_lines)
{
  int start_line, end_line;
  GdkRectangle *r;
  int i;

  /* Join rows vertically by extending to the middle */
  for (i = 0; i < n_lines - 1; i++)
    {
      GdkRectangle *r1 = &lines[i];
      GdkRectangle *r2 = &lines[i+1];
      int gap, old;

      gap = r2->y - (r1->y + r1->height);
      r1->height += gap / 2;
      old = r2->y;
      r2->y = r1->y + r1->height;
      r2->height += old - r2->y;
    }

  cairo_new_path (cr);
  start_line = 0;

  do
    {
      for (i = start_line; i < n_lines; i++)
        {
          r = &lines[i];
          if (i == start_line)
            cairo_move_to (cr, r->x + r->width, r->y);
          else
            cairo_line_to (cr, r->x + r->width, r->y);
          cairo_line_to (cr, r->x + r->width, r->y + r->height);

          if (i < n_lines - 1 &&
              (r->x + r->width < lines[i+1].x ||
              r->x > lines[i+1].x + lines[i+1].width))
            {
              i++;
              break;
            }
        }
      end_line = i;
      for (i = end_line - 1; i >= start_line; i--)
        {
          r = &lines[i];
          cairo_line_to (cr, r->x, r->y + r->height);
          cairo_line_to (cr, r->x, r->y);
        }
      cairo_close_path (cr);
      start_line = end_line;
    }
  while (end_line < n_lines);
}

static void
path_from_vertical_line_rects (cairo_t      *cr,
                               GdkRectangle *lines,
                               int           n_lines)
{
  int start_line, end_line;
  GdkRectangle *r;
  int i;

  /* Join rows horizontally by extending to the middle */
  for (i = 0; i < n_lines - 1; i++)
    {
      GdkRectangle *r1 = &lines[i];
      GdkRectangle *r2 = &lines[i+1];
      int gap, old;

      gap = r2->x - (r1->x + r1->width);
      r1->width += gap / 2;
      old = r2->x;
      r2->x = r1->x + r1->width;
      r2->width += old - r2->x;
    }

  cairo_new_path (cr);
  start_line = 0;

  do
    {
      for (i = start_line; i < n_lines; i++)
        {
          r = &lines[i];
          if (i == start_line)
            cairo_move_to (cr, r->x, r->y + r->height);
          else
            cairo_line_to (cr, r->x, r->y + r->height);
          cairo_line_to (cr, r->x + r->width, r->y + r->height);

          if (i < n_lines - 1 &&
              (r->y + r->height < lines[i+1].y ||
              r->y > lines[i+1].y + lines[i+1].height))
            {
              i++;
              break;
            }
        }
      end_line = i;
      for (i = end_line - 1; i >= start_line; i--)
        {
          r = &lines[i];
          cairo_line_to (cr, r->x + r->width, r->y);
          cairo_line_to (cr, r->x, r->y);
        }
      cairo_close_path (cr);
      start_line = end_line;
    }
  while (end_line < n_lines);
}

/* BobguiFlowBoxChild {{{1 */

/* GObject boilerplate {{{2 */

enum {
  CHILD_ACTIVATE,
  CHILD_LAST_SIGNAL
};

static guint child_signals[CHILD_LAST_SIGNAL] = { 0 };

enum {
  PROP_CHILD = 1
};

typedef struct _BobguiFlowBoxChildPrivate BobguiFlowBoxChildPrivate;
struct _BobguiFlowBoxChildPrivate
{
  BobguiWidget     *child;
  GSequenceIter *iter;
  gboolean       selected;
};

#define CHILD_PRIV(child) ((BobguiFlowBoxChildPrivate*)bobgui_flow_box_child_get_instance_private ((BobguiFlowBoxChild*)(child)))

static void bobgui_flow_box_child_buildable_iface_init (BobguiBuildableIface *iface);

G_DEFINE_TYPE_WITH_CODE (BobguiFlowBoxChild, bobgui_flow_box_child, BOBGUI_TYPE_WIDGET,
                         G_ADD_PRIVATE (BobguiFlowBoxChild)
                         G_IMPLEMENT_INTERFACE (BOBGUI_TYPE_BUILDABLE,
                                                bobgui_flow_box_child_buildable_iface_init))

/* Internal API {{{2 */

static BobguiFlowBox *
bobgui_flow_box_child_get_box (BobguiFlowBoxChild *child)
{
  BobguiWidget *parent;

  parent = bobgui_widget_get_parent (BOBGUI_WIDGET (child));
  if (parent && BOBGUI_IS_FLOW_BOX (parent))
    return BOBGUI_FLOW_BOX (parent);

  return NULL;
}

static void
bobgui_flow_box_child_set_focus (BobguiFlowBoxChild *child)
{
  BobguiFlowBox *box = bobgui_flow_box_child_get_box (child);

  bobgui_flow_box_update_selection (box, child, FALSE, FALSE);
}

/* BobguiWidget implementation {{{2 */

static BobguiBuildableIface *parent_child_buildable_iface;

static void
bobgui_flow_box_child_buildable_add_child (BobguiBuildable *buildable,
                                        BobguiBuilder   *builder,
                                        GObject      *child,
                                        const char   *type)
{
  if (BOBGUI_IS_WIDGET (child))
    {
      bobgui_buildable_child_deprecation_warning (buildable, builder, NULL, "child");
      bobgui_flow_box_child_set_child (BOBGUI_FLOW_BOX_CHILD (buildable), BOBGUI_WIDGET (child));
    }
  else
    {
      parent_child_buildable_iface->add_child (buildable, builder, child, type);
    }
}

static void
bobgui_flow_box_child_buildable_iface_init (BobguiBuildableIface *iface)
{
  parent_child_buildable_iface = g_type_interface_peek_parent (iface);

  iface->add_child = bobgui_flow_box_child_buildable_add_child;
}

static gboolean
bobgui_flow_box_child_focus (BobguiWidget        *widget,
                          BobguiDirectionType  direction)
{
  BobguiFlowBoxChild *self = BOBGUI_FLOW_BOX_CHILD (widget);
  BobguiFlowBoxChildPrivate *priv = CHILD_PRIV (self);
  BobguiWidget *child = priv->child;
  gboolean had_focus = FALSE;

  /* Without "focusable" flag try to pass the focus to the child immediately */
  if (!bobgui_widget_get_focusable (widget))
    {
      if (child)
        {
          if (bobgui_widget_child_focus (child, direction))
            {
              BobguiFlowBox *box;
              box = bobgui_flow_box_child_get_box (BOBGUI_FLOW_BOX_CHILD (widget));
              if (box)
                bobgui_flow_box_update_cursor (box, BOBGUI_FLOW_BOX_CHILD (widget));
              return TRUE;
            }
        }
      return FALSE;
    }

  g_object_get (widget, "has-focus", &had_focus, NULL);
  if (had_focus)
    {
      /* If on row, going right, enter into possible container */
      if (child &&
          (direction == BOBGUI_DIR_RIGHT || direction == BOBGUI_DIR_TAB_FORWARD))
        {
          if (bobgui_widget_child_focus (BOBGUI_WIDGET (child), direction))
            return TRUE;
        }

      return FALSE;
    }
  else if (bobgui_widget_get_focus_child (widget) != NULL)
    {
      /* Child has focus, always navigate inside it first */
      if (bobgui_widget_child_focus (child, direction))
        return TRUE;

      /* If exiting child container to the left, select child  */
      if (direction == BOBGUI_DIR_LEFT || direction == BOBGUI_DIR_TAB_BACKWARD)
        {
          bobgui_flow_box_child_set_focus (BOBGUI_FLOW_BOX_CHILD (widget));
          return TRUE;
        }

      return FALSE;
    }
  else
    {
      /* If coming from the left, enter into possible container */
      if (child &&
          (direction == BOBGUI_DIR_LEFT || direction == BOBGUI_DIR_TAB_BACKWARD))
        {
          if (bobgui_widget_child_focus (child, direction))
            return TRUE;
        }

      bobgui_flow_box_child_set_focus (BOBGUI_FLOW_BOX_CHILD (widget));
      return TRUE;
    }
}

static void
bobgui_flow_box_child_activate (BobguiFlowBoxChild *child)
{
  BobguiFlowBox *box;

  box = bobgui_flow_box_child_get_box (child);
  if (box)
    bobgui_flow_box_select_and_activate (box, child);
}

/* Size allocation {{{3 */

static void
bobgui_flow_box_child_dispose (GObject *object)
{
  BobguiFlowBoxChild *self = BOBGUI_FLOW_BOX_CHILD (object);
  BobguiFlowBoxChildPrivate *priv = CHILD_PRIV (self);

  g_clear_pointer (&priv->child, bobgui_widget_unparent);

  G_OBJECT_CLASS (bobgui_flow_box_child_parent_class)->dispose (object);
}

static void
bobgui_flow_box_child_get_property (GObject    *object,
                                 guint       prop_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
  BobguiFlowBoxChild *self = BOBGUI_FLOW_BOX_CHILD (object);

  switch (prop_id)
    {
    case PROP_CHILD:
      g_value_set_object (value, bobgui_flow_box_child_get_child (self));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bobgui_flow_box_child_set_property (GObject      *object,
                                 guint         prop_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  BobguiFlowBoxChild *self = BOBGUI_FLOW_BOX_CHILD (object);

  switch (prop_id)
    {
    case PROP_CHILD:
      bobgui_flow_box_child_set_child (self, g_value_get_object (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bobgui_flow_box_child_compute_expand (BobguiWidget *widget,
                                   gboolean  *hexpand,
                                   gboolean  *vexpand)
{
  BobguiFlowBoxChild *self = BOBGUI_FLOW_BOX_CHILD (widget);
  BobguiFlowBoxChildPrivate *priv = CHILD_PRIV (self);

  if (priv->child)
    {
      *hexpand = bobgui_widget_compute_expand (priv->child, BOBGUI_ORIENTATION_HORIZONTAL);
      *vexpand = bobgui_widget_compute_expand (priv->child, BOBGUI_ORIENTATION_VERTICAL);
    }
  else
    {
      *hexpand = FALSE;
      *vexpand = FALSE;
    }
}

static void
bobgui_flow_box_child_root (BobguiWidget *widget)
{
  BobguiFlowBoxChild *child = BOBGUI_FLOW_BOX_CHILD (widget);

  BOBGUI_WIDGET_CLASS (bobgui_flow_box_child_parent_class)->root (widget);

  bobgui_accessible_update_state (BOBGUI_ACCESSIBLE (child),
                               BOBGUI_ACCESSIBLE_STATE_SELECTED, CHILD_PRIV (child)->selected,
                               -1);
}

/* GObject implementation {{{2 */

static void
bobgui_flow_box_child_class_init (BobguiFlowBoxChildClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (class);

  object_class->dispose = bobgui_flow_box_child_dispose;
  object_class->get_property = bobgui_flow_box_child_get_property;
  object_class->set_property = bobgui_flow_box_child_set_property;

  widget_class->root = bobgui_flow_box_child_root;
  widget_class->compute_expand = bobgui_flow_box_child_compute_expand;
  widget_class->focus = bobgui_flow_box_child_focus;

  class->activate = bobgui_flow_box_child_activate;

  /**
   * BobguiFlowBoxChild:child:
   *
   * The child widget.
   */
  g_object_class_install_property (object_class,
                                   PROP_CHILD,
                                   g_param_spec_object ("child", NULL, NULL,
                                                        BOBGUI_TYPE_WIDGET,
                                                        BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

  /**
   * BobguiFlowBoxChild::activate:
   * @child: The child on which the signal is emitted
   *
   * Emitted when the user activates a child widget in a `BobguiFlowBox`.
   *
   * This can happen either by clicking or double-clicking,
   * or via a keybinding.
   *
   * This is a [keybinding signal](class.SignalAction.html),
   * but it can be used by applications for their own purposes.
   *
   * The default bindings are <kbd>Space</kbd> and <kbd>Enter</kbd>.
   */
  child_signals[CHILD_ACTIVATE] =
    g_signal_new (I_("activate"),
                  G_OBJECT_CLASS_TYPE (object_class),
                  G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
                  G_STRUCT_OFFSET (BobguiFlowBoxChildClass, activate),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 0);

  bobgui_widget_class_set_layout_manager_type (widget_class, BOBGUI_TYPE_BIN_LAYOUT);
  bobgui_widget_class_set_css_name (widget_class, I_("flowboxchild"));
  bobgui_widget_class_set_accessible_role (widget_class, BOBGUI_ACCESSIBLE_ROLE_GRID_CELL);
  bobgui_widget_class_set_activate_signal (widget_class, child_signals[CHILD_ACTIVATE]);
}

static void
bobgui_flow_box_child_init (BobguiFlowBoxChild *child)
{
  bobgui_widget_set_focusable (BOBGUI_WIDGET (child), TRUE);
}

/* Public API {{{2 */

/**
 * bobgui_flow_box_child_new:
 *
 * Creates a new `BobguiFlowBoxChild`.
 *
 * This should only be used as a child of a `BobguiFlowBox`.
 *
 * Returns: a new `BobguiFlowBoxChild`
 */
BobguiWidget *
bobgui_flow_box_child_new (void)
{
  return g_object_new (BOBGUI_TYPE_FLOW_BOX_CHILD, NULL);
}

/**
 * bobgui_flow_box_child_set_child:
 * @self: a `BobguiFlowBoxChild`
 * @child: (nullable): the child widget
 *
 * Sets the child widget of @self.
 */
void
bobgui_flow_box_child_set_child (BobguiFlowBoxChild *self,
                              BobguiWidget       *child)
{
  BobguiFlowBoxChildPrivate *priv = CHILD_PRIV (self);

  g_return_if_fail (BOBGUI_IS_FLOW_BOX_CHILD (self));
  g_return_if_fail (child == NULL || priv->child == child || bobgui_widget_get_parent (child) == NULL);

  if (priv->child == child)
    return;

  g_clear_pointer (&priv->child, bobgui_widget_unparent);

  priv->child = child;
  if (child)
    bobgui_widget_set_parent (child, BOBGUI_WIDGET (self));
  g_object_notify (G_OBJECT (self), "child");
}

/**
 * bobgui_flow_box_child_get_child:
 * @self: a `BobguiFlowBoxChild`
 *
 * Gets the child widget of @self.
 *
 * Returns: (nullable) (transfer none): the child widget of @self
 */
BobguiWidget *
bobgui_flow_box_child_get_child (BobguiFlowBoxChild *self)
{
  BobguiFlowBoxChildPrivate *priv = CHILD_PRIV (self);

  return priv->child;
}

/**
 * bobgui_flow_box_child_get_index:
 * @child: a `BobguiFlowBoxChild`
 *
 * Gets the current index of the @child in its `BobguiFlowBox` container.
 *
 * Returns: the index of the @child, or -1 if the @child is not
 *   in a flow box
 */
int
bobgui_flow_box_child_get_index (BobguiFlowBoxChild *child)
{
  BobguiFlowBoxChildPrivate *priv;

  g_return_val_if_fail (BOBGUI_IS_FLOW_BOX_CHILD (child), -1);

  priv = CHILD_PRIV (child);

  if (priv->iter != NULL)
    return g_sequence_iter_get_position (priv->iter);

  return -1;
}

/**
 * bobgui_flow_box_child_is_selected:
 * @child: a `BobguiFlowBoxChild`
 *
 * Returns whether the @child is currently selected in its
 * `BobguiFlowBox` container.
 *
 * Returns: %TRUE if @child is selected
 */
gboolean
bobgui_flow_box_child_is_selected (BobguiFlowBoxChild *child)
{
  g_return_val_if_fail (BOBGUI_IS_FLOW_BOX_CHILD (child), FALSE);

  return CHILD_PRIV (child)->selected;
}

/**
 * bobgui_flow_box_child_changed:
 * @child: a `BobguiFlowBoxChild`
 *
 * Marks @child as changed, causing any state that depends on this
 * to be updated.
 *
 * This affects sorting and filtering.
 *
 * Note that calls to this method must be in sync with the data
 * used for the sorting and filtering functions. For instance, if
 * the list is mirroring some external data set, and *two* children
 * changed in the external data set when you call
 * bobgui_flow_box_child_changed() on the first child, the sort function
 * must only read the new data for the first of the two changed
 * children, otherwise the resorting of the children will be wrong.
 *
 * This generally means that if you don’t fully control the data
 * model, you have to duplicate the data that affects the sorting
 * and filtering functions into the widgets themselves.
 *
 * Another alternative is to call [method@Bobgui.FlowBox.invalidate_sort]
 * on any model change, but that is more expensive.
 */
void
bobgui_flow_box_child_changed (BobguiFlowBoxChild *child)
{
  BobguiFlowBox *box;

  g_return_if_fail (BOBGUI_IS_FLOW_BOX_CHILD (child));

  box = bobgui_flow_box_child_get_box (child);

  if (box == NULL)
    return;

  bobgui_flow_box_apply_sort (box, child);
  bobgui_flow_box_apply_filter (box, child);
}

/* BobguiFlowBox  {{{1 */

 /* Constants {{{2 */

#define DEFAULT_MAX_CHILDREN_PER_LINE 7
#define RUBBERBAND_START_DISTANCE 32
#define AUTOSCROLL_FAST_DISTANCE 32
#define AUTOSCROLL_FACTOR 20
#define AUTOSCROLL_FACTOR_FAST 10

/* GObject boilerplate {{{2 */

enum {
  CHILD_ACTIVATED,
  SELECTED_CHILDREN_CHANGED,
  ACTIVATE_CURSOR_CHILD,
  TOGGLE_CURSOR_CHILD,
  MOVE_CURSOR,
  SELECT_ALL,
  UNSELECT_ALL,
  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

enum {
  PROP_0,
  PROP_HOMOGENEOUS,
  PROP_COLUMN_SPACING,
  PROP_ROW_SPACING,
  PROP_MIN_CHILDREN_PER_LINE,
  PROP_MAX_CHILDREN_PER_LINE,
  PROP_SELECTION_MODE,
  PROP_ACTIVATE_ON_SINGLE_CLICK,
  PROP_ACCEPT_UNPAIRED_RELEASE,

  /* orientable */
  PROP_ORIENTATION,
  LAST_PROP = PROP_ORIENTATION
};

static GParamSpec *props[LAST_PROP] = { NULL, };

typedef struct _BobguiFlowBoxClass       BobguiFlowBoxClass;

struct _BobguiFlowBox
{
  BobguiWidget container;
};

struct _BobguiFlowBoxClass
{
  BobguiWidgetClass parent_class;

  void (*child_activated)            (BobguiFlowBox        *box,
                                      BobguiFlowBoxChild   *child);
  void (*selected_children_changed)  (BobguiFlowBox        *box);
  void (*activate_cursor_child)      (BobguiFlowBox        *box);
  void (*toggle_cursor_child)        (BobguiFlowBox        *box);
  gboolean (*move_cursor)            (BobguiFlowBox        *box,
                                      BobguiMovementStep    step,
                                      int                count,
                                      gboolean           extend,
                                      gboolean           modify);
  void (*select_all)                 (BobguiFlowBox        *box);
  void (*unselect_all)               (BobguiFlowBox        *box);
};

typedef struct _BobguiFlowBoxPrivate BobguiFlowBoxPrivate;
struct _BobguiFlowBoxPrivate {
  BobguiOrientation    orientation;
  gboolean          homogeneous;

  guint             row_spacing;
  guint             column_spacing;

  BobguiFlowBoxChild  *cursor_child;
  BobguiFlowBoxChild  *selected_child;

  BobguiFlowBoxChild  *active_child;

  BobguiSelectionMode  selection_mode;

  BobguiAdjustment    *hadjustment;
  BobguiAdjustment    *vadjustment;
  gboolean          activate_on_single_click;
  gboolean          accept_unpaired_release;

  guint16           min_children_per_line;
  guint16           max_children_per_line;
  guint16           cur_children_per_line;

  GSequence        *children;

  BobguiFlowBoxFilterFunc filter_func;
  gpointer             filter_data;
  GDestroyNotify       filter_destroy;

  BobguiFlowBoxSortFunc sort_func;
  gpointer           sort_data;
  GDestroyNotify     sort_destroy;

  BobguiGesture        *drag_gesture;

  BobguiFlowBoxChild   *rubberband_first;
  BobguiFlowBoxChild   *rubberband_last;
  BobguiCssNode        *rubberband_node;
  gboolean           rubberband_select;
  gboolean           rubberband_modify;
  gboolean           rubberband_extend;

  BobguiScrollType      autoscroll_mode;
  guint              autoscroll_id;

  GListModel                 *bound_model;
  BobguiFlowBoxCreateWidgetFunc  create_widget_func;
  gpointer                    create_widget_func_data;
  GDestroyNotify              create_widget_func_data_destroy;

  gboolean           disable_move_cursor;
};

#define BOX_PRIV(box) ((BobguiFlowBoxPrivate*)bobgui_flow_box_get_instance_private ((BobguiFlowBox*)(box)))

static void bobgui_flow_box_buildable_iface_init (BobguiBuildableIface *iface);

G_DEFINE_TYPE_WITH_CODE (BobguiFlowBox, bobgui_flow_box, BOBGUI_TYPE_WIDGET,
                         G_ADD_PRIVATE (BobguiFlowBox)
                         G_IMPLEMENT_INTERFACE (BOBGUI_TYPE_ORIENTABLE, NULL)
                         G_IMPLEMENT_INTERFACE (BOBGUI_TYPE_BUILDABLE,
                                                bobgui_flow_box_buildable_iface_init))

/*  Internal API, utilities {{{2 */

#define ORIENTATION_ALIGN(box)                              \
  (BOX_PRIV(box)->orientation == BOBGUI_ORIENTATION_HORIZONTAL \
   ? bobgui_widget_get_halign (BOBGUI_WIDGET (box))               \
   : bobgui_widget_get_valign (BOBGUI_WIDGET (box)))

#define OPPOSING_ORIENTATION_ALIGN(box)                     \
  (BOX_PRIV(box)->orientation == BOBGUI_ORIENTATION_HORIZONTAL \
   ? bobgui_widget_get_valign (BOBGUI_WIDGET (box))               \
   : bobgui_widget_get_halign (BOBGUI_WIDGET (box)))

/* Children are visible if they are shown by the app (visible)
 * and not filtered out (child_visible) by the box
 */
static inline gboolean
child_is_visible (BobguiWidget *child)
{
  return bobgui_widget_get_visible (child) &&
         bobgui_widget_get_child_visible (child);
}

static int
get_visible_children (BobguiFlowBox *box)
{
  GSequenceIter *iter;
  int i = 0;

  for (iter = g_sequence_get_begin_iter (BOX_PRIV (box)->children);
       !g_sequence_iter_is_end (iter);
       iter = g_sequence_iter_next (iter))
    {
      BobguiWidget *child;

      child = g_sequence_get (iter);
      if (child_is_visible (child))
        i++;
    }

  return i;
}

static void
bobgui_flow_box_apply_filter (BobguiFlowBox      *box,
                           BobguiFlowBoxChild *child)
{
  BobguiFlowBoxPrivate *priv = BOX_PRIV (box);
  gboolean do_show;

  do_show = TRUE;
  if (priv->filter_func != NULL)
    do_show = priv->filter_func (child, priv->filter_data);

  bobgui_widget_set_child_visible (BOBGUI_WIDGET (child), do_show);
}

static void
bobgui_flow_box_apply_filter_all (BobguiFlowBox *box)
{
  GSequenceIter *iter;

  for (iter = g_sequence_get_begin_iter (BOX_PRIV (box)->children);
       !g_sequence_iter_is_end (iter);
       iter = g_sequence_iter_next (iter))
    {
      BobguiFlowBoxChild *child;

      child = g_sequence_get (iter);
      bobgui_flow_box_apply_filter (box, child);
    }
  bobgui_widget_queue_resize (BOBGUI_WIDGET (box));
}

static void
bobgui_flow_box_apply_sort (BobguiFlowBox      *box,
                         BobguiFlowBoxChild *child)
{
  if (BOX_PRIV (box)->sort_func != NULL)
    {
      g_sequence_sort_changed (CHILD_PRIV (child)->iter,
                               (GCompareDataFunc)bobgui_flow_box_sort, box);
      bobgui_widget_queue_resize (BOBGUI_WIDGET (box));
    }
}

/* Sel ection utilities {{{3 */

static gboolean
bobgui_flow_box_child_set_selected (BobguiFlowBoxChild *child,
                                 gboolean         selected)
{
  if (CHILD_PRIV (child)->selected != selected)
    {
      CHILD_PRIV (child)->selected = selected;
      if (selected)
        bobgui_widget_set_state_flags (BOBGUI_WIDGET (child),
                                    BOBGUI_STATE_FLAG_SELECTED, FALSE);
      else
        bobgui_widget_unset_state_flags (BOBGUI_WIDGET (child),
                                      BOBGUI_STATE_FLAG_SELECTED);

      bobgui_accessible_update_state (BOBGUI_ACCESSIBLE (child),
                                   BOBGUI_ACCESSIBLE_STATE_SELECTED, selected,
                                   -1);
      return TRUE;
    }

  return FALSE;
}

static gboolean
bobgui_flow_box_unselect_all_internal (BobguiFlowBox *box)
{
  BobguiFlowBoxChild *child;
  GSequenceIter *iter;
  gboolean dirty = FALSE;

  if (BOX_PRIV (box)->selection_mode == BOBGUI_SELECTION_NONE)
    return FALSE;

  for (iter = g_sequence_get_begin_iter (BOX_PRIV (box)->children);
       !g_sequence_iter_is_end (iter);
       iter = g_sequence_iter_next (iter))
    {
      child = g_sequence_get (iter);
      dirty |= bobgui_flow_box_child_set_selected (child, FALSE);
    }

  return dirty;
}

static void
bobgui_flow_box_unselect_child_internal (BobguiFlowBox      *box,
                                      BobguiFlowBoxChild *child)
{
  if (!CHILD_PRIV (child)->selected)
    return;

  if (BOX_PRIV (box)->selection_mode == BOBGUI_SELECTION_NONE)
    return;
  else if (BOX_PRIV (box)->selection_mode != BOBGUI_SELECTION_MULTIPLE)
    bobgui_flow_box_unselect_all_internal (box);
  else
    bobgui_flow_box_child_set_selected (child, FALSE);

  g_signal_emit (box, signals[SELECTED_CHILDREN_CHANGED], 0);
}

static void
bobgui_flow_box_update_cursor (BobguiFlowBox      *box,
                            BobguiFlowBoxChild *child)
{
  BOX_PRIV (box)->cursor_child = child;
  bobgui_widget_grab_focus (BOBGUI_WIDGET (child));
}

static void
bobgui_flow_box_select_child_internal (BobguiFlowBox      *box,
                                    BobguiFlowBoxChild *child)
{
  if (CHILD_PRIV (child)->selected)
    return;

  if (BOX_PRIV (box)->selection_mode == BOBGUI_SELECTION_NONE)
    return;
  if (BOX_PRIV (box)->selection_mode != BOBGUI_SELECTION_MULTIPLE)
    bobgui_flow_box_unselect_all_internal (box);

  bobgui_flow_box_child_set_selected (child, TRUE);
  BOX_PRIV (box)->selected_child = child;

  g_signal_emit (box, signals[SELECTED_CHILDREN_CHANGED], 0);
}

static void
bobgui_flow_box_select_all_between (BobguiFlowBox      *box,
                                 BobguiFlowBoxChild *child1,
                                 BobguiFlowBoxChild *child2,
				 gboolean         modify)
{
  GSequenceIter *iter, *iter1, *iter2;

  if (child1)
    iter1 = CHILD_PRIV (child1)->iter;
  else
    iter1 = g_sequence_get_begin_iter (BOX_PRIV (box)->children);

  if (child2)
    iter2 = CHILD_PRIV (child2)->iter;
  else
    iter2 = g_sequence_get_end_iter (BOX_PRIV (box)->children);

  if (g_sequence_iter_compare (iter2, iter1) < 0)
    {
      iter = iter1;
      iter1 = iter2;
      iter2 = iter;
    }

  for (iter = iter1;
       !g_sequence_iter_is_end (iter);
       iter = g_sequence_iter_next (iter))
    {
      BobguiWidget *child;

      child = g_sequence_get (iter);
      if (child_is_visible (child))
        {
          if (modify)
            bobgui_flow_box_child_set_selected (BOBGUI_FLOW_BOX_CHILD (child), !CHILD_PRIV (child)->selected);
          else
            bobgui_flow_box_child_set_selected (BOBGUI_FLOW_BOX_CHILD (child), TRUE);
	}

      if (g_sequence_iter_compare (iter, iter2) == 0)
        break;
    }
}

static void
bobgui_flow_box_update_selection (BobguiFlowBox      *box,
                               BobguiFlowBoxChild *child,
                               gboolean         modify,
                               gboolean         extend)
{
  BobguiFlowBoxPrivate *priv = BOX_PRIV (box);

  bobgui_flow_box_update_cursor (box, child);

  if (priv->selection_mode == BOBGUI_SELECTION_NONE)
    return;

  if (priv->selection_mode == BOBGUI_SELECTION_BROWSE)
    {
      bobgui_flow_box_unselect_all_internal (box);
      bobgui_flow_box_child_set_selected (child, TRUE);
      priv->selected_child = child;
    }
  else if (priv->selection_mode == BOBGUI_SELECTION_SINGLE)
    {
      gboolean was_selected;

      was_selected = CHILD_PRIV (child)->selected;
      bobgui_flow_box_unselect_all_internal (box);
      bobgui_flow_box_child_set_selected (child, modify ? !was_selected : TRUE);
      priv->selected_child = CHILD_PRIV (child)->selected ? child : NULL;
    }
  else /* BOBGUI_SELECTION_MULTIPLE */
    {
      if (extend)
        {
          bobgui_flow_box_unselect_all_internal (box);
          if (priv->selected_child == NULL)
            {
              bobgui_flow_box_child_set_selected (child, TRUE);
              priv->selected_child = child;
            }
          else
            bobgui_flow_box_select_all_between (box, priv->selected_child, child, FALSE);
        }
      else
        {
          if (modify)
            {
              bobgui_flow_box_child_set_selected (child, !CHILD_PRIV (child)->selected);
            }
          else
            {
              bobgui_flow_box_unselect_all_internal (box);
              bobgui_flow_box_child_set_selected (child, !CHILD_PRIV (child)->selected);
              priv->selected_child = child;
            }
        }
    }

  g_signal_emit (box, signals[SELECTED_CHILDREN_CHANGED], 0);
}

static void
bobgui_flow_box_select_and_activate (BobguiFlowBox      *box,
                                  BobguiFlowBoxChild *child)
{
  if (child != NULL)
    {
      bobgui_flow_box_select_child_internal (box, child);
      bobgui_flow_box_update_cursor (box, child);
      g_signal_emit (box, signals[CHILD_ACTIVATED], 0, child);
    }
}

/* Focus utilities {{{3 */

static GSequenceIter *
bobgui_flow_box_get_previous_focusable (BobguiFlowBox    *box,
                                     GSequenceIter *iter)
{
  BobguiFlowBoxChild *child;

  while (!g_sequence_iter_is_begin (iter))
    {
      iter = g_sequence_iter_prev (iter);
      child = g_sequence_get (iter);
      if (child_is_visible (BOBGUI_WIDGET (child)) &&
          bobgui_widget_is_sensitive (BOBGUI_WIDGET (child)))
        return iter;
    }

  return NULL;
}

static GSequenceIter *
bobgui_flow_box_get_next_focusable (BobguiFlowBox    *box,
                                 GSequenceIter *iter)
{
  BobguiFlowBoxChild *child;

  while (TRUE)
    {
      iter = g_sequence_iter_next (iter);
      if (g_sequence_iter_is_end (iter))
        return NULL;
      child = g_sequence_get (iter);
      if (child_is_visible (BOBGUI_WIDGET (child)) &&
          bobgui_widget_is_sensitive (BOBGUI_WIDGET (child)))
        return iter;
    }

  return NULL;
}

static GSequenceIter *
bobgui_flow_box_get_first_focusable (BobguiFlowBox *box)
{
  GSequenceIter *iter;
  BobguiFlowBoxChild *child;

  iter = g_sequence_get_begin_iter (BOX_PRIV (box)->children);
  if (g_sequence_iter_is_end (iter))
    return NULL;

  child = g_sequence_get (iter);
  if (child_is_visible (BOBGUI_WIDGET (child)) &&
      bobgui_widget_is_sensitive (BOBGUI_WIDGET (child)))
    return iter;

  return bobgui_flow_box_get_next_focusable (box, iter);
}

static GSequenceIter *
bobgui_flow_box_get_last_focusable (BobguiFlowBox *box)
{
  GSequenceIter *iter;

  iter = g_sequence_get_end_iter (BOX_PRIV (box)->children);
  return bobgui_flow_box_get_previous_focusable (box, iter);
}


static GSequenceIter *
bobgui_flow_box_get_above_focusable (BobguiFlowBox    *box,
                                  GSequenceIter *iter)
{
  BobguiFlowBoxChild *child = NULL;
  int i;

  while (TRUE)
    {
      i = 0;
      while (i < BOX_PRIV (box)->cur_children_per_line)
        {
          if (g_sequence_iter_is_begin (iter))
            return NULL;
          iter = g_sequence_iter_prev (iter);
          child = g_sequence_get (iter);
          if (child_is_visible (BOBGUI_WIDGET (child)))
            i++;
        }
      if (child && bobgui_widget_get_sensitive (BOBGUI_WIDGET (child)))
        return iter;
    }

  return NULL;
}

static GSequenceIter *
bobgui_flow_box_get_below_focusable (BobguiFlowBox    *box,
                                  GSequenceIter *iter)
{
  BobguiFlowBoxChild *child = NULL;
  int i;

  while (TRUE)
    {
      i = 0;
      while (i < BOX_PRIV (box)->cur_children_per_line)
        {
          iter = g_sequence_iter_next (iter);
          if (g_sequence_iter_is_end (iter))
            return NULL;
          child = g_sequence_get (iter);
          if (child_is_visible (BOBGUI_WIDGET (child)))
            i++;
        }
      if (child && bobgui_widget_get_sensitive (BOBGUI_WIDGET (child)))
        return iter;
    }

  return NULL;
}

/* BobguiWidget implementation {{{2 */

/* Size allocation {{{3 */

/* Used in columned modes where all items share at least their
 * equal widths or heights
 */
static void
get_max_item_size (BobguiFlowBox     *box,
                   BobguiOrientation  orientation,
                   int            *min_size,
                   int            *nat_size)
{
  GSequenceIter *iter;
  int max_min_size = 0;
  int max_nat_size = 0;

  for (iter = g_sequence_get_begin_iter (BOX_PRIV (box)->children);
       !g_sequence_iter_is_end (iter);
       iter = g_sequence_iter_next (iter))
    {
      BobguiWidget *child;
      int child_min, child_nat;

      child = g_sequence_get (iter);

      if (!child_is_visible (child))
        continue;

      bobgui_widget_measure (child, orientation, -1,
                          &child_min, &child_nat,
                          NULL, NULL);

      max_min_size = MAX (max_min_size, child_min);
      max_nat_size = MAX (max_nat_size, child_nat);
    }

  if (min_size)
    *min_size = max_min_size;

  if (nat_size)
    *nat_size = max_nat_size;
}


/* Gets the largest minimum/natural size for a given size (used to get
 * the largest item heights for a fixed item width and the opposite)
 */
static void
get_largest_size_for_opposing_orientation (BobguiFlowBox     *box,
                                           BobguiOrientation  orientation,
                                           int             item_size,
                                           int            *min_item_size,
                                           int            *nat_item_size)
{
  GSequenceIter *iter;
  int max_min_size = 0;
  int max_nat_size = 0;

  for (iter = g_sequence_get_begin_iter (BOX_PRIV (box)->children);
       !g_sequence_iter_is_end (iter);
       iter = g_sequence_iter_next (iter))
    {
      BobguiWidget *child;
      int        child_min, child_nat;

      child = g_sequence_get (iter);

      if (!child_is_visible (child))
        continue;

      bobgui_widget_measure (child, 1 - orientation, item_size,
                          &child_min, &child_nat,
                          NULL, NULL);

      max_min_size = MAX (max_min_size, child_min);
      max_nat_size = MAX (max_nat_size, child_nat);
    }

  if (min_item_size)
    *min_item_size = max_min_size;

  if (nat_item_size)
    *nat_item_size = max_nat_size;
}

/* Gets the largest minimum/natural size on a single line for a given size
 * (used to get the largest line heights for a fixed item width and the opposite
 * while iterating over a list of children, note the new index is returned)
 */
static GSequenceIter *
get_largest_size_for_line_in_opposing_orientation (BobguiFlowBox       *box,
                                                   BobguiOrientation    orientation,
                                                   GSequenceIter    *cursor,
                                                   int               line_length,
                                                   BobguiRequestedSize *item_sizes,
                                                   int               extra_pixels,
                                                   int              *min_item_size,
                                                   int              *nat_item_size)
{
  GSequenceIter *iter;
  int max_min_size = 0;
  int max_nat_size = 0;
  int i;

  i = 0;
  for (iter = cursor;
       !g_sequence_iter_is_end (iter) && i < line_length;
       iter = g_sequence_iter_next (iter))
    {
      BobguiWidget *child;
      int child_min, child_nat, this_item_size;

      child = g_sequence_get (iter);

      if (!child_is_visible (child))
        continue;

      /* Distribute the extra pixels to the first children in the line
       * (could be fancier and spread them out more evenly) */
      this_item_size = item_sizes[i].minimum_size;
      if (extra_pixels > 0 && ORIENTATION_ALIGN (box) == BOBGUI_ALIGN_FILL)
        {
          this_item_size++;
          extra_pixels--;
        }

      bobgui_widget_measure (child, 1 - orientation, this_item_size,
                          &child_min, &child_nat,
                          NULL, NULL);

      max_min_size = MAX (max_min_size, child_min);
      max_nat_size = MAX (max_nat_size, child_nat);

      i++;
    }

  if (min_item_size)
    *min_item_size = max_min_size;

  if (nat_item_size)
    *nat_item_size = max_nat_size;

  /* Return next item in the list */
  return iter;
}

/* fit_aligned_item_requests() helper */
static int
gather_aligned_item_requests (BobguiFlowBox       *box,
                              BobguiOrientation    orientation,
                              int               line_length,
                              int               item_spacing,
                              int               n_children,
                              BobguiRequestedSize *item_sizes)
{
  GSequenceIter *iter;
  int i;
  int extra_items, natural_line_size = 0;

  extra_items = n_children % line_length;

  i = 0;
  for (iter = g_sequence_get_begin_iter (BOX_PRIV (box)->children);
       !g_sequence_iter_is_end (iter);
       iter = g_sequence_iter_next (iter))
    {
      BobguiWidget *child;
      BobguiAlign item_align;
      int child_min, child_nat;
      int position;

      child = g_sequence_get (iter);

      if (!child_is_visible (child))
        continue;

      bobgui_widget_measure (child, orientation, -1,
                          &child_min, &child_nat,
                          NULL, NULL);

      /* Get the index and push it over for the last line when spreading to the end */
      position = i % line_length;

      item_align = ORIENTATION_ALIGN (box);
      if (item_align == BOBGUI_ALIGN_END && i >= n_children - extra_items)
        position += line_length - extra_items;

      /* Round up the size of every column/row */
      item_sizes[position].minimum_size = MAX (item_sizes[position].minimum_size, child_min);
      item_sizes[position].natural_size = MAX (item_sizes[position].natural_size, child_nat);

      i++;
    }

  for (i = 0; i < line_length; i++)
    natural_line_size += item_sizes[i].natural_size;

  natural_line_size += (line_length - 1) * item_spacing;

  return natural_line_size;
}

static BobguiRequestedSize *
fit_aligned_item_requests (BobguiFlowBox     *box,
                           BobguiOrientation  orientation,
                           int             avail_size,
                           int             item_spacing,
                           int            *line_length, /* in-out */
                           int             items_per_line,
                           int             n_children)
{
  BobguiRequestedSize *sizes, *try_sizes;
  int try_line_size, try_length;

  sizes = g_new0 (BobguiRequestedSize, *line_length);

  /* get the sizes for the initial guess */
  try_line_size = gather_aligned_item_requests (box,
                                                orientation,
                                                *line_length,
                                                item_spacing,
                                                n_children,
                                                sizes);

  /* Try columnizing the whole thing and adding an item to the end of
   * the line; try to fit as many columns into the available size as
   * possible
   */
  for (try_length = *line_length + 1; try_line_size < avail_size; try_length++)
    {
      try_sizes = g_new0 (BobguiRequestedSize, try_length);
      try_line_size = gather_aligned_item_requests (box,
                                                    orientation,
                                                    try_length,
                                                    item_spacing,
                                                    n_children,
                                                    try_sizes);

      if (try_line_size <= avail_size &&
          items_per_line >= try_length)
        {
          *line_length = try_length;

          g_free (sizes);
          sizes = try_sizes;
        }
      else
        {
          /* oops, this one failed; stick to the last size that fit and then return */
          g_free (try_sizes);
          break;
        }
    }

  return sizes;
}

typedef struct {
  GArray *requested;
  int     extra_pixels;
} AllocatedLine;

static int
get_offset_pixels (BobguiAlign align,
                   int      pixels)
{
  int offset;

  switch (align) {
  case BOBGUI_ALIGN_START:
  case BOBGUI_ALIGN_FILL:
    offset = 0;
    break;
  case BOBGUI_ALIGN_CENTER:
    offset = pixels / 2;
    break;
  case BOBGUI_ALIGN_END:
    offset = pixels;
    break;
  case BOBGUI_ALIGN_BASELINE_FILL:
  case BOBGUI_ALIGN_BASELINE_CENTER:
  default:
    g_assert_not_reached ();
    break;
  }

  return offset;
}

static void
bobgui_flow_box_size_allocate (BobguiWidget *widget,
                            int        width,
                            int        height,
                            int        baseline)
{
  BobguiFlowBox *box = BOBGUI_FLOW_BOX (widget);
  BobguiFlowBoxPrivate  *priv = BOX_PRIV (box);
  BobguiAllocation child_allocation;
  int avail_size, avail_other_size, min_items, item_spacing, line_spacing;
  BobguiAlign item_align;
  BobguiAlign line_align;
  BobguiRequestedSize *line_sizes = NULL;
  BobguiRequestedSize *item_sizes = NULL;
  int min_item_size, nat_item_size;
  int line_length;
  int item_size = 0;
  int line_size = 0, min_fixed_line_size = 0, nat_fixed_line_size = 0;
  int line_offset, item_offset, n_children, n_lines, line_count;
  int extra_pixels = 0, extra_per_item = 0, extra_extra = 0;
  int extra_line_pixels = 0, extra_per_line = 0, extra_line_extra = 0;
  int i, this_line_size;
  GSequenceIter *iter;

  min_items = MAX (1, priv->min_children_per_line);

  if (priv->orientation == BOBGUI_ORIENTATION_HORIZONTAL)
    {
      avail_size = width;
      avail_other_size = height;
      item_spacing = priv->column_spacing; line_spacing = priv->row_spacing;
    }
  else /* BOBGUI_ORIENTATION_VERTICAL */
    {
      avail_size = height;
      avail_other_size = width;
      item_spacing = priv->row_spacing;
      line_spacing = priv->column_spacing;
    }

  item_align = ORIENTATION_ALIGN (box);
  line_align = OPPOSING_ORIENTATION_ALIGN (box);

  /* Get how many lines we'll be needing to flow */
  n_children = get_visible_children (box);
  if (n_children <= 0)
    return;

  /* Deal with ALIGNED/HOMOGENEOUS modes first, start with
   * initial guesses at item/line sizes
   */
  get_max_item_size (box, priv->orientation, &min_item_size, &nat_item_size);
  if (nat_item_size <= 0)
    {
      child_allocation.x = 0;
      child_allocation.y = 0;
      child_allocation.width = 0;
      child_allocation.height = 0;

      for (iter = g_sequence_get_begin_iter (priv->children);
           !g_sequence_iter_is_end (iter);
           iter = g_sequence_iter_next (iter))
        {
          BobguiWidget *child;

          child = g_sequence_get (iter);

          if (!child_is_visible (child))
            continue;

          bobgui_widget_size_allocate (child, &child_allocation, -1);
        }

      return;
    }

  /* By default flow at the natural item width */
  line_length = avail_size / (nat_item_size + item_spacing);

  /* After the above approximation, check if we can't fit one more on the line */
  if (line_length * item_spacing + (line_length + 1) * nat_item_size <= avail_size)
    line_length++;

  /* Its possible we were allocated just less than the natural width of the
   * minimum item flow length
   */
  line_length = MAX (min_items, line_length);
  line_length = MIN (line_length, priv->max_children_per_line);

  /* Here we just use the largest height-for-width and use that for the height
   * of all lines
   */
  if (priv->homogeneous)
    {
      n_lines = n_children / line_length;
      if ((n_children % line_length) > 0)
        n_lines++;

      n_lines = MAX (n_lines, 1);

      /* Now we need the real item allocation size */
      item_size = (avail_size - (line_length - 1) * item_spacing) / line_length;

      /* Cut out the expand space if we're not distributing any */
      if (item_align != BOBGUI_ALIGN_FILL)
        item_size = MIN (item_size, nat_item_size);

      get_largest_size_for_opposing_orientation (box,
                                                 priv->orientation,
                                                 item_size,
                                                 &min_fixed_line_size,
                                                 &nat_fixed_line_size);

      /* resolve a fixed 'line_size' */
      line_size = (avail_other_size - (n_lines - 1) * line_spacing) / n_lines;

      if (line_align != BOBGUI_ALIGN_FILL)
        line_size = MIN (line_size, nat_fixed_line_size);

      /* Get the real extra pixels in case of BOBGUI_ALIGN_START lines */
      extra_pixels = avail_size - (line_length - 1) * item_spacing - item_size * line_length;
      extra_line_pixels = avail_other_size - (n_lines - 1) * line_spacing - line_size * n_lines;
    }
  else
    {
      gboolean first_line = TRUE;

      /* Find the amount of columns that can fit aligned into the available space
       * and collect their requests.
       */
      item_sizes = fit_aligned_item_requests (box,
                                              priv->orientation,
                                              avail_size,
                                              item_spacing,
                                              &line_length,
                                              priv->max_children_per_line,
                                              n_children);

      /* Calculate the number of lines after determining the final line_length */
      n_lines = n_children / line_length;
      if ((n_children % line_length) > 0)
        n_lines++;

      n_lines = MAX (n_lines, 1);
      line_sizes = g_new0 (BobguiRequestedSize, n_lines);

      /* Get the available remaining size */
      avail_size -= (line_length - 1) * item_spacing;
      for (i = 0; i < line_length; i++)
        avail_size -= item_sizes[i].minimum_size;

      /* Perform a natural allocation on the columnized items and get the remaining pixels */
      if (avail_size > 0)
        extra_pixels = bobgui_distribute_natural_allocation (avail_size, line_length, item_sizes);

      /* Now that we have the size of each column of items find the size of each individual
       * line based on the aligned item sizes.
       */

      for (i = 0, iter = g_sequence_get_begin_iter (priv->children);
           !g_sequence_iter_is_end (iter) && i < n_lines;
           i++)
        {
          iter = get_largest_size_for_line_in_opposing_orientation (box,
                                                                    priv->orientation,
                                                                    iter,
                                                                    line_length,
                                                                    item_sizes,
                                                                    extra_pixels,
                                                                    &line_sizes[i].minimum_size,
                                                                    &line_sizes[i].natural_size);


          /* Its possible a line is made of completely invisible children */
          if (line_sizes[i].natural_size > 0)
            {
              if (first_line)
                first_line = FALSE;
              else
                avail_other_size -= line_spacing;

              avail_other_size -= line_sizes[i].minimum_size;

              line_sizes[i].data = GINT_TO_POINTER (i);
            }
        }

      /* Distribute space among lines naturally */
      if (avail_other_size > 0)
        extra_line_pixels = bobgui_distribute_natural_allocation (avail_other_size, n_lines, line_sizes);
    }

  /*
   * Initial sizes of items/lines guessed at this point,
   * go on to distribute expand space if needed.
   */

  priv->cur_children_per_line = line_length;

  /* FIXME: This portion needs to consider which columns
   * and rows asked for expand space and distribute those
   * accordingly for the case of ALIGNED allocation.
   *
   * If at least one child in a column/row asked for expand;
   * we should make that row/column expand entirely.
   */

  /* Calculate expand space per item */
  if (item_align == BOBGUI_ALIGN_FILL)
    {
      extra_per_item = extra_pixels / line_length;
      extra_extra    = extra_pixels % line_length;
    }

  /* Calculate expand space per line */
  if (line_align == BOBGUI_ALIGN_FILL)
    {
      extra_per_line   = extra_line_pixels / n_lines;
      extra_line_extra = extra_line_pixels % n_lines;
    }

  /* prepend extra space to item_offset/line_offset for SPREAD_END */
  item_offset = get_offset_pixels (item_align, extra_pixels);
  line_offset = get_offset_pixels (line_align, extra_line_pixels);

  /* Get the allocation size for the first line */
  if (priv->homogeneous)
    this_line_size = line_size;
  else
    {
      this_line_size = line_sizes[0].minimum_size;

      if (line_align == BOBGUI_ALIGN_FILL)
        {
          this_line_size += extra_per_line;

          if (extra_line_extra > 0)
            this_line_size++;
        }
    }

  i = 0;
  line_count = 0;
  for (iter = g_sequence_get_begin_iter (priv->children);
       !g_sequence_iter_is_end (iter);
       iter = g_sequence_iter_next (iter))
    {
      BobguiWidget *child;
      int position;
      int this_item_size;
      int last_line_n_items = n_children % line_length;

      if (last_line_n_items == 0)
        last_line_n_items = line_length;

      int last_line_extra_items = line_length - last_line_n_items;

      child = g_sequence_get (iter);

      if (!child_is_visible (child))
        continue;

      /* Get item position */
      position = i % line_length;

      /* adjust the line_offset/count at the beginning of each new line */
      if (i > 0 && position == 0)
        {
          /* Push the line_offset */
          line_offset += this_line_size + line_spacing;

          line_count++;

          /* Get the new line size */
          if (priv->homogeneous)
            this_line_size = line_size;
          else
            {
              this_line_size = line_sizes[line_count].minimum_size;

              if (line_align == BOBGUI_ALIGN_FILL)
                {
                  this_line_size += extra_per_line;

                  if (line_count < extra_line_extra)
                    this_line_size++;
                }
            }

          item_offset = 0;

          if (item_align == BOBGUI_ALIGN_CENTER)
            {
              item_offset += get_offset_pixels (item_align, extra_pixels);
            }
          else if (item_align == BOBGUI_ALIGN_END)
            {
              item_offset += get_offset_pixels (item_align, extra_pixels);

              /* If we're on the last line, prepend the space for
               * any leading items */
              if (line_count == n_lines -1)
                {
                  if (priv->homogeneous)
                    {
                      item_offset += item_size * last_line_extra_items;
                      item_offset += item_spacing * last_line_extra_items;
                    }
                  else
                    {
                      int j;

                      for (j = 0; j < last_line_extra_items; j++)
                        {
                          item_offset += item_sizes[j].minimum_size;
                          item_offset += item_spacing;
                        }
                    }
                }
            }
        }

      /* Push the index along for the last line when spreading to the end */
      if (item_align == BOBGUI_ALIGN_END && line_count == n_lines -1)
        position += last_line_extra_items;

      if (priv->homogeneous)
        this_item_size = item_size;
      else
        this_item_size = item_sizes[position].minimum_size;

      if (item_align == BOBGUI_ALIGN_FILL)
        {
          this_item_size += extra_per_item;

          if (position < extra_extra)
            this_item_size++;
        }

      /* Do the actual allocation */
      if (priv->orientation == BOBGUI_ORIENTATION_HORIZONTAL)
        {
          child_allocation.x = item_offset;
          child_allocation.y = line_offset;
          child_allocation.width = this_item_size;
          child_allocation.height = this_line_size;
        }
      else /* BOBGUI_ORIENTATION_VERTICAL */
        {
          child_allocation.x = line_offset;
          child_allocation.y = item_offset;
          child_allocation.width = this_line_size;
          child_allocation.height = this_item_size;
        }

      if (bobgui_widget_get_direction (widget) == BOBGUI_TEXT_DIR_RTL)
        child_allocation.x = width - child_allocation.x - child_allocation.width;

      bobgui_widget_size_allocate (child, &child_allocation, -1);

      item_offset += this_item_size;
      item_offset += item_spacing;

      i++;
    }

  g_free (item_sizes);
  g_free (line_sizes);
}

static BobguiSizeRequestMode
bobgui_flow_box_get_request_mode (BobguiWidget *widget)
{
  BobguiFlowBox *box = BOBGUI_FLOW_BOX (widget);
  BobguiWidget *visible_child = NULL;
  GSequenceIter *iter;

  for (iter = g_sequence_get_begin_iter (BOX_PRIV (box)->children);
       !g_sequence_iter_is_end (iter);
       iter = g_sequence_iter_next (iter))
    {
      BobguiWidget *child;

      child = g_sequence_get (iter);
      if (!child_is_visible (child))
        continue;

      if (!visible_child)
        visible_child = child;
      else
        /* Multiple visible children */
        return (BOX_PRIV (box)->orientation == BOBGUI_ORIENTATION_HORIZONTAL) ?
                BOBGUI_SIZE_REQUEST_HEIGHT_FOR_WIDTH : BOBGUI_SIZE_REQUEST_WIDTH_FOR_HEIGHT;
    }

  if (visible_child)
    return bobgui_widget_get_request_mode (visible_child);

  return BOBGUI_SIZE_REQUEST_CONSTANT_SIZE;
}

/* Gets the largest minimum and natural length of
 * 'line_length' consecutive items when aligned into rows/columns */
static void
get_largest_aligned_line_length (BobguiFlowBox     *box,
                                 BobguiOrientation  orientation,
                                 int             line_length,
                                 int            *min_size,
                                 int            *nat_size)
{
  GSequenceIter *iter;
  int max_min_size = 0;
  int max_nat_size = 0;
  int spacing, i;
  BobguiRequestedSize *aligned_item_sizes;

  if (orientation == BOBGUI_ORIENTATION_HORIZONTAL)
    spacing = BOX_PRIV (box)->column_spacing;
  else
    spacing = BOX_PRIV (box)->row_spacing;

  aligned_item_sizes = g_new0 (BobguiRequestedSize, line_length);

  /* Get the largest sizes of each index in the line.
   */
  i = 0;
  for (iter = g_sequence_get_begin_iter (BOX_PRIV (box)->children);
       !g_sequence_iter_is_end (iter);
       iter = g_sequence_iter_next (iter))
    {
      BobguiWidget *child;
      int child_min, child_nat;

      child = g_sequence_get (iter);
      if (!child_is_visible (child))
        continue;

      bobgui_widget_measure (child, orientation, -1,
                          &child_min, &child_nat,
                          NULL, NULL);

      aligned_item_sizes[i % line_length].minimum_size =
        MAX (aligned_item_sizes[i % line_length].minimum_size, child_min);

      aligned_item_sizes[i % line_length].natural_size =
        MAX (aligned_item_sizes[i % line_length].natural_size, child_nat);

      i++;
    }

  /* Add up the largest indexes */
  for (i = 0; i < line_length; i++)
    {
      max_min_size += aligned_item_sizes[i].minimum_size;
      max_nat_size += aligned_item_sizes[i].natural_size;
    }

  g_free (aligned_item_sizes);

  max_min_size += (line_length - 1) * spacing;
  max_nat_size += (line_length - 1) * spacing;

  if (min_size)
    *min_size = max_min_size;

  if (nat_size)
    *nat_size = max_nat_size;
}

static void
bobgui_flow_box_measure (BobguiWidget      *widget,
                      BobguiOrientation  orientation,
                      int             for_size,
                      int            *minimum,
                      int            *natural,
                      int            *minimum_baseline,
                      int            *natural_baseline)
{
  BobguiFlowBox *box = BOBGUI_FLOW_BOX (widget);
  BobguiFlowBoxPrivate *priv = BOX_PRIV (box);

  if (orientation == BOBGUI_ORIENTATION_HORIZONTAL)
    {
      if (for_size < 0)
        {
          int min_item_width, nat_item_width;
          int min_items, nat_items;
          int min_width, nat_width;

          min_items = MAX (1, priv->min_children_per_line);
          nat_items = MAX (min_items, priv->max_children_per_line);

          if (priv->orientation == BOBGUI_ORIENTATION_HORIZONTAL)
            {
              min_width = nat_width = 0;

              if (!priv->homogeneous)
                {
                  /* When not homogeneous; horizontally oriented boxes
                   * need enough width for the widest row
                   */
                  int min_line_length, nat_line_length;

                  get_largest_aligned_line_length (box,
                                                   BOBGUI_ORIENTATION_HORIZONTAL,
                                                   min_items,
                                                   &min_line_length,
                                                   &nat_line_length);

                  if (nat_items > min_items)
                    get_largest_aligned_line_length (box,
                                                     BOBGUI_ORIENTATION_HORIZONTAL,
                                                     nat_items,
                                                     NULL,
                                                     &nat_line_length);

                  min_width += min_line_length;
                  nat_width += nat_line_length;
                }
              else /* In homogeneous mode; horizontally oriented boxes
                    * give the same width to all children */
                {
                  get_max_item_size (box, BOBGUI_ORIENTATION_HORIZONTAL,
                                     &min_item_width, &nat_item_width);

                  min_width += min_item_width * min_items;
                  min_width += (min_items -1) * priv->column_spacing;

                  nat_width += nat_item_width * nat_items;
                  nat_width += (nat_items -1) * priv->column_spacing;
                }
            }
          else /* BOBGUI_ORIENTATION_VERTICAL */
            {
              /* Return the width for the minimum height */
              int min_height;
              int dummy;

              bobgui_flow_box_measure (widget,
                                    BOBGUI_ORIENTATION_VERTICAL,
                                    -1,
                                    &min_height, &dummy,
                                    NULL, NULL);
              bobgui_flow_box_measure (widget,
                                    BOBGUI_ORIENTATION_HORIZONTAL,
                                    min_height,
                                    &min_width, &nat_width,
                                    NULL, NULL);
            }

          *minimum = min_width;
          *natural = nat_width;
        }
      else
        {
          int min_item_height, nat_item_height;
          int min_items;
          int min_width, nat_width;
          int avail_size, n_children;

          min_items = MAX (1, priv->min_children_per_line);

          min_width = 0;
          nat_width = 0;

          if (priv->orientation == BOBGUI_ORIENTATION_HORIZONTAL)
            {
              /* Binary search :( */
              int min, max;
              int min_height, nat_height;

              bobgui_flow_box_measure (widget,
                                    BOBGUI_ORIENTATION_HORIZONTAL,
                                    -1,
                                    &min_width, &nat_width,
                                    NULL, NULL);
              min = min_width;
              max = G_MAXINT;

              while (min < max)
                {
                  int test;

                  if (max != G_MAXINT)
                    test = (min + max) / 2;
                  else if (min == min_width)
                    test = min;
                  else
                    test = min * 2;

                  bobgui_flow_box_measure (widget, BOBGUI_ORIENTATION_VERTICAL,
                                        test, &min_height, &nat_height,
                                        NULL, NULL);
                  if (min_height > for_size)
                    min = test + 1;
                  else
                    max = test;
                }
              /* TODO: calculate natural size properly */
              min_width = min;
              nat_width = MAX (min, nat_width);
            }
          else /* BOBGUI_ORIENTATION_VERTICAL */
            {
              int min_height;
              int line_length;
              int item_size, extra_pixels;
              int dummy;

              n_children = get_visible_children (box);
              if (n_children <= 0)
                goto out_width;

              /* Make sure its no smaller than the minimum */
              bobgui_flow_box_measure (widget,
                                    BOBGUI_ORIENTATION_VERTICAL,
                                    -1,
                                    &min_height, &dummy,
                                    NULL, NULL);

              avail_size = MAX (for_size, min_height);
              if (avail_size <= 0)
                goto out_width;

              get_max_item_size (box, BOBGUI_ORIENTATION_VERTICAL, &min_item_height, &nat_item_height);
              if (nat_item_height <= 0)
                goto out_width;

              /* By default flow at the natural item width */
              line_length = avail_size / (nat_item_height + priv->row_spacing);

              /* After the above approximation, check if we can't fit one more on the line */
              if (line_length * priv->row_spacing + (line_length + 1) * nat_item_height <= avail_size)
                line_length++;

              /* Its possible we were allocated just less than the natural width of the
               * minimum item flow length
               */
              line_length = MAX (min_items, line_length);
              line_length = MIN (line_length, priv->max_children_per_line);

              /* Now we need the real item allocation size */
              item_size = (avail_size - (line_length - 1) * priv->row_spacing) / line_length;

              /* Cut out the expand space if we're not distributing any */
              if (bobgui_widget_get_valign (widget) != BOBGUI_ALIGN_FILL)
                {
                  item_size = MIN (item_size, nat_item_height);
                  extra_pixels = 0;
                }
              else
                /* Collect the extra pixels for expand children */
                extra_pixels = (avail_size - (line_length - 1) * priv->row_spacing) % line_length;

              if (priv->homogeneous)
                {
                  int min_item_width, nat_item_width;
                  int lines;

                  /* Here we just use the largest height-for-width and
                   * add up the size accordingly
                   */
                  get_largest_size_for_opposing_orientation (box,
                                                             BOBGUI_ORIENTATION_VERTICAL,
                                                             item_size,
                                                             &min_item_width,
                                                             &nat_item_width);

                  /* Round up how many lines we need to allocate for */
                  n_children = get_visible_children (box);
                  lines = n_children / line_length;
                  if ((n_children % line_length) > 0)
                    lines++;

                  min_width = min_item_width * lines;
                  nat_width = nat_item_width * lines;

                  min_width += (lines - 1) * priv->column_spacing;
                  nat_width += (lines - 1) * priv->column_spacing;
                }
              else
                {
                  int min_line_width, nat_line_width, i;
                  gboolean first_line = TRUE;
                  BobguiRequestedSize *item_sizes;
                  GSequenceIter *iter;

                  /* First get the size each set of items take to span the line
                   * when aligning the items above and below after flowping.
                   */
                  item_sizes = fit_aligned_item_requests (box,
                                                          priv->orientation,
                                                          avail_size,
                                                          priv->row_spacing,
                                                          &line_length,
                                                          priv->max_children_per_line,
                                                          n_children);

                  /* Get the available remaining size */
                  avail_size -= (line_length - 1) * priv->column_spacing;
                  for (i = 0; i < line_length; i++)
                    avail_size -= item_sizes[i].minimum_size;

                  if (avail_size > 0)
                    extra_pixels = bobgui_distribute_natural_allocation (avail_size, line_length, item_sizes);

                  for (iter = g_sequence_get_begin_iter (priv->children);
                       !g_sequence_iter_is_end (iter);)
                    {
                      iter = get_largest_size_for_line_in_opposing_orientation (box,
                                                                                BOBGUI_ORIENTATION_VERTICAL,
                                                                                iter,
                                                                                line_length,
                                                                                item_sizes,
                                                                                extra_pixels,
                                                                                &min_line_width,
                                                                                &nat_line_width);

                      /* Its possible the last line only had invisible widgets */
                      if (nat_line_width > 0)
                        {
                          if (first_line)
                            first_line = FALSE;
                          else
                            {
                              min_width += priv->column_spacing;
                              nat_width += priv->column_spacing;
                            }

                          min_width += min_line_width;
                          nat_width += nat_line_width;
                        }
                    }
                  g_free (item_sizes);
                }
            }

         out_width:
          *minimum = min_width;
          *natural = nat_width;
        }
    }
  else
    {
      if (for_size < 0)
        {
          int min_item_height, nat_item_height;
          int min_items, nat_items;
          int min_height, nat_height;

          min_items = MAX (1, priv->min_children_per_line);
          nat_items = MAX (min_items, priv->max_children_per_line);

          if (priv->orientation == BOBGUI_ORIENTATION_HORIZONTAL)
            {
              /* Return the height for the natural width */
              int nat_width, dummy;

              bobgui_flow_box_measure (widget,
                                    BOBGUI_ORIENTATION_HORIZONTAL,
                                    -1,
                                    &dummy, &nat_width,
                                    NULL, NULL);
              bobgui_flow_box_measure (widget,
                                    BOBGUI_ORIENTATION_VERTICAL,
                                    nat_width,
                                    &min_height, &nat_height,
                                    NULL, NULL);
            }
          else /* BOBGUI_ORIENTATION_VERTICAL */
            {
              min_height = nat_height = 0;

              if (! priv->homogeneous)
                {
                  /* When not homogeneous; vertically oriented boxes
                   * need enough height for the tallest column
                   */
                  if (min_items == 1)
                    {
                      get_max_item_size (box, BOBGUI_ORIENTATION_VERTICAL,
                                         &min_item_height, &nat_item_height);

                      min_height += min_item_height;
                      nat_height += nat_item_height;
                    }
                  else
                    {
                      int min_line_length, nat_line_length;

                      get_largest_aligned_line_length (box,
                                                       BOBGUI_ORIENTATION_VERTICAL,
                                                       min_items,
                                                       &min_line_length,
                                                       &nat_line_length);

                      if (nat_items > min_items)
                        get_largest_aligned_line_length (box,
                                                         BOBGUI_ORIENTATION_VERTICAL,
                                                         nat_items,
                                                         NULL,
                                                         &nat_line_length);

                      min_height += min_line_length;
                      nat_height += nat_line_length;
                    }

                }
              else
                {
                  /* In homogeneous mode; vertically oriented boxes
                   * give the same height to all children
                   */
                  get_max_item_size (box,
                                     BOBGUI_ORIENTATION_VERTICAL,
                                     &min_item_height,
                                     &nat_item_height);

                  min_height += min_item_height * min_items;
                  min_height += (min_items -1) * priv->row_spacing;

                  nat_height += nat_item_height * nat_items;
                  nat_height += (nat_items -1) * priv->row_spacing;
                }
            }

          *minimum = min_height;
          *natural = nat_height;
        }
      else
        {
          int min_item_width, nat_item_width;
          int min_items;
          int min_height, nat_height;
          int avail_size, n_children;

          min_items = MAX (1, priv->min_children_per_line);

          min_height = 0;
          nat_height = 0;

          if (priv->orientation == BOBGUI_ORIENTATION_HORIZONTAL)
            {
              int min_width;
              int line_length;
              int item_size, extra_pixels;
              int dummy;

              n_children = get_visible_children (box);
              if (n_children <= 0)
                goto out_height;

              /* Make sure its no smaller than the minimum */
              bobgui_flow_box_measure (widget,
                                    BOBGUI_ORIENTATION_HORIZONTAL,
                                    -1,
                                    &min_width, &dummy,
                                    NULL, NULL);

              avail_size = MAX (for_size, min_width);
              if (avail_size <= 0)
                goto out_height;

              get_max_item_size (box, BOBGUI_ORIENTATION_HORIZONTAL, &min_item_width, &nat_item_width);
              if (nat_item_width <= 0)
                goto out_height;

              /* By default flow at the natural item width */
              line_length = avail_size / (nat_item_width + priv->column_spacing);

              /* After the above approximation, check if we can't fit one more on the line */
              if (line_length * priv->column_spacing + (line_length + 1) * nat_item_width <= avail_size)
                line_length++;

              /* Its possible we were allocated just less than the natural width of the
               * minimum item flow length
               */
              line_length = MAX (min_items, line_length);
              line_length = MIN (line_length, priv->max_children_per_line);

              /* Now we need the real item allocation size */
              item_size = (avail_size - (line_length - 1) * priv->column_spacing) / line_length;

              /* Cut out the expand space if we're not distributing any */
              if (bobgui_widget_get_halign (widget) != BOBGUI_ALIGN_FILL)
                {
                  item_size    = MIN (item_size, nat_item_width);
                  extra_pixels = 0;
                }
              else
                /* Collect the extra pixels for expand children */
                extra_pixels = (avail_size - (line_length - 1) * priv->column_spacing) % line_length;

              if (priv->homogeneous)
                {
                  int min_item_height, nat_item_height;
                  int lines;

                  /* Here we just use the largest height-for-width and
                   * add up the size accordingly
                   */
                  get_largest_size_for_opposing_orientation (box,
                                                             BOBGUI_ORIENTATION_HORIZONTAL,
                                                             item_size,
                                                             &min_item_height,
                                                             &nat_item_height);

                  /* Round up how many lines we need to allocate for */
                  lines = n_children / line_length;
                  if ((n_children % line_length) > 0)
                    lines++;

                  min_height = min_item_height * lines;
                  nat_height = nat_item_height * lines;

                  min_height += (lines - 1) * priv->row_spacing;
                  nat_height += (lines - 1) * priv->row_spacing;
                }
              else
                {
                  int min_line_height, nat_line_height, i;
                  gboolean first_line = TRUE;
                  BobguiRequestedSize *item_sizes;
                  GSequenceIter *iter;

                  /* First get the size each set of items take to span the line
                   * when aligning the items above and below after flowping.
                   */
                  item_sizes = fit_aligned_item_requests (box,
                                                          priv->orientation,
                                                          avail_size,
                                                          priv->column_spacing,
                                                          &line_length,
                                                          priv->max_children_per_line,
                                                          n_children);

                  /* Get the available remaining size */
                  avail_size -= (line_length - 1) * priv->column_spacing;
                  for (i = 0; i < line_length; i++)
                    avail_size -= item_sizes[i].minimum_size;

                  if (avail_size > 0)
                    extra_pixels = bobgui_distribute_natural_allocation (avail_size, line_length, item_sizes);

                  for (iter = g_sequence_get_begin_iter (priv->children);
                       !g_sequence_iter_is_end (iter);)
                    {
                      iter = get_largest_size_for_line_in_opposing_orientation (box,
                                                                                BOBGUI_ORIENTATION_HORIZONTAL,
                                                                                iter,
                                                                                line_length,
                                                                                item_sizes,
                                                                                extra_pixels,
                                                                                &min_line_height,
                                                                                &nat_line_height);
                      /* Its possible the line only had invisible widgets */
                      if (nat_line_height > 0)
                        {
                          if (first_line)
                            first_line = FALSE;
                          else
                            {
                              min_height += priv->row_spacing;
                              nat_height += priv->row_spacing;
                            }

                          min_height += min_line_height;
                          nat_height += nat_line_height;
                        }
                    }

                  g_free (item_sizes);
                }
            }
          else /* BOBGUI_ORIENTATION_VERTICAL */
            {
              /* Return the minimum height */
              bobgui_flow_box_measure (widget,
                                    BOBGUI_ORIENTATION_VERTICAL,
                                    -1,
                                    &min_height, &nat_height,
                                    NULL, NULL);
            }

         out_height:
          *minimum = min_height;
          *natural = nat_height;
        }
    }
}

/* Drawing {{{3 */

static void
bobgui_flow_box_snapshot (BobguiWidget   *widget,
                       BobguiSnapshot *snapshot)
{
  BobguiFlowBox *box = BOBGUI_FLOW_BOX (widget);
  BobguiFlowBoxPrivate *priv = BOX_PRIV (box);
  int x, y, width, height;
  BobguiCssBoxes boxes;

  BOBGUI_WIDGET_CLASS (bobgui_flow_box_parent_class)->snapshot (widget, snapshot);

  x = 0;
  y = 0;
  width = bobgui_widget_get_width (widget);
  height = bobgui_widget_get_height (widget);

  if (priv->rubberband_first && priv->rubberband_last)
    {
      BobguiCssStyle *style;
      GSequenceIter *iter, *iter1, *iter2;
      GdkRectangle line_rect = { 0, }, rect;
      GArray *lines;
      gboolean vertical;
      cairo_t *cr;

      vertical = priv->orientation == BOBGUI_ORIENTATION_VERTICAL;

      cr = bobgui_snapshot_append_cairo (snapshot,
                                      &GRAPHENE_RECT_INIT (x, y, width, height));

      style = bobgui_css_node_get_style (priv->rubberband_node);

      iter1 = CHILD_PRIV (priv->rubberband_first)->iter;
      iter2 = CHILD_PRIV (priv->rubberband_last)->iter;

      if (g_sequence_iter_compare (iter2, iter1) < 0)
        {
          iter = iter1;
          iter1 = iter2;
          iter2 = iter;
        }

      lines = g_array_new (FALSE, FALSE, sizeof (GdkRectangle));

      for (iter = iter1;
           !g_sequence_iter_is_end (iter);
           iter = g_sequence_iter_next (iter))
        {
          BobguiWidget *child;

          child = g_sequence_get (iter);
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
          bobgui_widget_get_allocation (BOBGUI_WIDGET (child), &rect);
G_GNUC_END_IGNORE_DEPRECATIONS
          if (line_rect.width == 0)
            line_rect = rect;
          else
            {
              if ((vertical && rect.x == line_rect.x) ||
                  (!vertical && rect.y == line_rect.y))
                gdk_rectangle_union (&rect, &line_rect, &line_rect);
              else
                {
                  g_array_append_val (lines, line_rect);
                  line_rect = rect;
                }
            }

          if (g_sequence_iter_compare (iter, iter2) == 0)
            break;
        }

      if (line_rect.width != 0)
        g_array_append_val (lines, line_rect);

      if (lines->len > 0)
        {
          cairo_path_t *path;
          const GdkRGBA *border_color;
          int border_width;
          BobguiSnapshot *bg_snapshot;
          GskRenderNode *node;

          if (vertical)
            path_from_vertical_line_rects (cr, (GdkRectangle *)lines->data, lines->len);
          else
            path_from_horizontal_line_rects (cr, (GdkRectangle *)lines->data, lines->len);

          /* For some reason we need to copy and reapply the path,
           * or it gets eaten by bobgui_render_background()
           */
          path = cairo_copy_path (cr);

          cairo_save (cr);
          cairo_clip (cr);

          bg_snapshot = bobgui_snapshot_new ();
          bobgui_css_boxes_init_border_box (&boxes, style, x, y, width, height);
          bobgui_css_style_snapshot_background (&boxes, bg_snapshot);
          node = bobgui_snapshot_free_to_node (bg_snapshot);
          if (node)
            {
              gsk_render_node_draw (node, cr);
              gsk_render_node_unref (node);
            }

          cairo_restore (cr);

          cairo_append_path (cr, path);
          cairo_path_destroy (path);

          border_color = bobgui_css_color_value_get_rgba (style->used->border_top_color);
          border_width = round (bobgui_css_number_value_get (style->border->border_left_width, 100));

          cairo_set_line_width (cr, border_width);
          gdk_cairo_set_source_rgba (cr, border_color);
          cairo_stroke (cr);
        }
      g_array_free (lines, TRUE);

      cairo_destroy (cr);
    }
}

/* Autoscrolling {{{3 */

static void
remove_autoscroll (BobguiFlowBox *box)
{
  BobguiFlowBoxPrivate *priv = BOX_PRIV (box);

  if (priv->autoscroll_id)
    {
      bobgui_widget_remove_tick_callback (BOBGUI_WIDGET (box), priv->autoscroll_id);
      priv->autoscroll_id = 0;
    }

  priv->autoscroll_mode = BOBGUI_SCROLL_NONE;
}

static gboolean
autoscroll_cb (BobguiWidget     *widget,
               GdkFrameClock *frame_clock,
               gpointer       data)
{
  BobguiFlowBox *box = BOBGUI_FLOW_BOX (data);
  BobguiFlowBoxPrivate *priv = BOX_PRIV (box);
  BobguiAdjustment *adjustment;
  double factor;
  double increment;
  double value;

  if (priv->orientation == BOBGUI_ORIENTATION_HORIZONTAL)
    adjustment = priv->vadjustment;
  else
    adjustment = priv->hadjustment;

  switch (priv->autoscroll_mode)
    {
    case BOBGUI_SCROLL_STEP_FORWARD:
      factor = AUTOSCROLL_FACTOR;
      break;
    case BOBGUI_SCROLL_STEP_BACKWARD:
      factor = - AUTOSCROLL_FACTOR;
      break;
    case BOBGUI_SCROLL_PAGE_FORWARD:
      factor = AUTOSCROLL_FACTOR_FAST;
      break;
    case BOBGUI_SCROLL_PAGE_BACKWARD:
      factor = - AUTOSCROLL_FACTOR_FAST;
      break;
    case BOBGUI_SCROLL_NONE:
    case BOBGUI_SCROLL_JUMP:
    case BOBGUI_SCROLL_STEP_UP:
    case BOBGUI_SCROLL_STEP_DOWN:
    case BOBGUI_SCROLL_STEP_LEFT:
    case BOBGUI_SCROLL_STEP_RIGHT:
    case BOBGUI_SCROLL_PAGE_UP:
    case BOBGUI_SCROLL_PAGE_DOWN:
    case BOBGUI_SCROLL_PAGE_LEFT:
    case BOBGUI_SCROLL_PAGE_RIGHT:
    case BOBGUI_SCROLL_START:
    case BOBGUI_SCROLL_END:
    default:
      g_assert_not_reached ();
      break;
    }

  increment = bobgui_adjustment_get_step_increment (adjustment) / factor;

  value = bobgui_adjustment_get_value (adjustment);
  value += increment;
  bobgui_adjustment_set_value (adjustment, value);

  if (priv->rubberband_select)
    {
      GdkEventSequence *sequence;
      double x, y;
      BobguiFlowBoxChild *child;

      sequence = bobgui_gesture_single_get_current_sequence (BOBGUI_GESTURE_SINGLE (priv->drag_gesture));
      bobgui_gesture_get_point (priv->drag_gesture, sequence, &x, &y);

      child = bobgui_flow_box_get_child_at_pos (box, x, y);

      if (child != NULL)
        priv->rubberband_last = child;
    }

  return G_SOURCE_CONTINUE;
}

static void
add_autoscroll (BobguiFlowBox *box)
{
  BobguiFlowBoxPrivate *priv = BOX_PRIV (box);

  if (priv->autoscroll_id != 0 ||
      priv->autoscroll_mode == BOBGUI_SCROLL_NONE)
    return;

  priv->autoscroll_id = bobgui_widget_add_tick_callback (BOBGUI_WIDGET (box),
                                                      autoscroll_cb,
                                                      box,
                                                      NULL);
}

static gboolean
get_view_rect (BobguiFlowBox   *box,
               GdkRectangle *rect)
{
  BobguiFlowBoxPrivate *priv = BOX_PRIV (box);
  BobguiWidget *parent;

  parent = bobgui_widget_get_parent (BOBGUI_WIDGET (box));
  if (BOBGUI_IS_VIEWPORT (parent))
    {
      rect->x = bobgui_adjustment_get_value (priv->hadjustment);
      rect->y = bobgui_adjustment_get_value (priv->vadjustment);
      rect->width = bobgui_widget_get_width (parent);
      rect->height = bobgui_widget_get_height (parent);
      return TRUE;
    }

  return FALSE;
}

static void
update_autoscroll_mode (BobguiFlowBox *box,
                        int         x,
                        int         y)
{
  BobguiFlowBoxPrivate *priv = BOX_PRIV (box);
  BobguiScrollType mode = BOBGUI_SCROLL_NONE;
  GdkRectangle rect;
  int size, pos;

  if (priv->rubberband_select && get_view_rect (box, &rect))
    {
      if (priv->orientation == BOBGUI_ORIENTATION_VERTICAL)
        {
          size = rect.width;
          pos = x - rect.x;
        }
      else
        {
          size = rect.height;
          pos = y - rect.y;
        }

      if (pos < 0 - AUTOSCROLL_FAST_DISTANCE)
        mode = BOBGUI_SCROLL_PAGE_BACKWARD;
      else if (pos > size + AUTOSCROLL_FAST_DISTANCE)
        mode = BOBGUI_SCROLL_PAGE_FORWARD;
      else if (pos < 0)
        mode = BOBGUI_SCROLL_STEP_BACKWARD;
      else if (pos > size)
        mode = BOBGUI_SCROLL_STEP_FORWARD;
    }

  if (mode != priv->autoscroll_mode)
    {
      remove_autoscroll (box);
      priv->autoscroll_mode = mode;
      add_autoscroll (box);
    }
}

/* Event handling {{{3 */

static void
bobgui_flow_box_drag_gesture_update (BobguiGestureDrag *gesture,
                                  double          offset_x,
                                  double          offset_y,
                                  BobguiFlowBox     *box)
{
  BobguiFlowBoxPrivate *priv = BOX_PRIV (box);
  double start_x, start_y;
  BobguiFlowBoxChild *child;
  BobguiCssNode *widget_node;

  bobgui_gesture_drag_get_start_point (gesture, &start_x, &start_y);

  if (!priv->rubberband_select &&
      (offset_x * offset_x) + (offset_y * offset_y) > RUBBERBAND_START_DISTANCE * RUBBERBAND_START_DISTANCE)
    {
      priv->rubberband_select = TRUE;
      priv->rubberband_first = bobgui_flow_box_get_child_at_pos (box, start_x, start_y);

      widget_node = bobgui_widget_get_css_node (BOBGUI_WIDGET (box));
      priv->rubberband_node = bobgui_css_node_new ();
      bobgui_css_node_set_name (priv->rubberband_node, g_quark_from_static_string ("rubberband"));
      bobgui_css_node_set_parent (priv->rubberband_node, widget_node);
      bobgui_css_node_set_state (priv->rubberband_node, bobgui_css_node_get_state (widget_node));
      g_object_unref (priv->rubberband_node);

      /* Grab focus here, so Escape-to-stop-rubberband  works */
      if (priv->rubberband_first)
        bobgui_flow_box_update_cursor (box, priv->rubberband_first);
      bobgui_gesture_set_state (BOBGUI_GESTURE (gesture), BOBGUI_EVENT_SEQUENCE_CLAIMED);
    }

  if (priv->rubberband_select)
    {
      child = bobgui_flow_box_get_child_at_pos (box, start_x + offset_x,
                                              start_y + offset_y);

      if (priv->rubberband_first == NULL)
        {
          priv->rubberband_first = child;
          if (priv->rubberband_first)
            bobgui_flow_box_update_cursor (box, priv->rubberband_first);
        }
      if (child != NULL)
        priv->rubberband_last = child;

      update_autoscroll_mode (box, start_x + offset_x, start_y + offset_y);
      bobgui_widget_queue_draw (BOBGUI_WIDGET (box));
    }
}

static void
bobgui_flow_box_click_gesture_pressed (BobguiGestureClick *gesture,
                                    guint            n_press,
                                    double           x,
                                    double           y,
                                    BobguiFlowBox      *box)
{
  BobguiFlowBoxPrivate *priv = BOX_PRIV (box);
  BobguiFlowBoxChild *child;

  child = bobgui_flow_box_get_child_at_pos (box, x, y);

  if (child == NULL)
    return;

  /* The drag gesture is only triggered by first press */
  if (n_press != 1)
    bobgui_gesture_set_state (priv->drag_gesture, BOBGUI_EVENT_SEQUENCE_DENIED);

  priv->active_child = child;
  bobgui_widget_queue_draw (BOBGUI_WIDGET (box));

  if (n_press == 2 && !priv->activate_on_single_click)
    {
      bobgui_gesture_set_state (BOBGUI_GESTURE (gesture),
                             BOBGUI_EVENT_SEQUENCE_CLAIMED);
      g_signal_emit (box, signals[CHILD_ACTIVATED], 0, child);
    }
}

static void
bobgui_flow_box_click_unpaired_release (BobguiGestureClick  *gesture,
                                     double            x,
                                     double            y,
                                     guint             button,
                                     GdkEventSequence *sequence,
                                     BobguiFlowBox       *box)
{
  BobguiFlowBoxPrivate *priv = BOX_PRIV (box);
  BobguiFlowBoxChild *child;

  if (!priv->activate_on_single_click || !priv->accept_unpaired_release)
    return;

  child = bobgui_flow_box_get_child_at_pos (box, x, y);

  if (child)
    bobgui_flow_box_select_and_activate (box, child);
}

static void
bobgui_flow_box_click_gesture_released (BobguiGestureClick *gesture,
                                     guint            n_press,
                                     double           x,
                                     double           y,
                                     BobguiFlowBox      *box)
{
  BobguiFlowBoxPrivate *priv = BOX_PRIV (box);

  if (priv->active_child != NULL &&
      priv->active_child == bobgui_flow_box_get_child_at_pos (box, x, y))
    {
      bobgui_gesture_set_state (BOBGUI_GESTURE (gesture),
                             BOBGUI_EVENT_SEQUENCE_CLAIMED);

      if (priv->activate_on_single_click)
        bobgui_flow_box_select_and_activate (box, priv->active_child);
      else
        {
          GdkEventSequence *sequence;
          GdkInputSource source;
          GdkEvent *event;
          GdkModifierType state;
          gboolean modify;
          gboolean extend;

          state = bobgui_event_controller_get_current_event_state (BOBGUI_EVENT_CONTROLLER (gesture));
          modify = (state & GDK_CONTROL_MASK) != 0;
          extend = (state & GDK_SHIFT_MASK) != 0;

          /* With touch, we default to modifying the selection.
           * You can still clear the selection and start over
           * by holding Ctrl.
           */

          sequence = bobgui_gesture_single_get_current_sequence (BOBGUI_GESTURE_SINGLE (gesture));
          event = bobgui_gesture_get_last_event (BOBGUI_GESTURE (gesture), sequence);
          source = gdk_device_get_source (gdk_event_get_device (event));

          if (source == GDK_SOURCE_TOUCHSCREEN)
            modify = !modify;

          bobgui_flow_box_update_selection (box, priv->active_child, modify, extend);
        }
    }
}

static void
bobgui_flow_box_click_gesture_stopped (BobguiGestureClick *gesture,
                                    BobguiFlowBox      *box)
{
  BobguiFlowBoxPrivate *priv = BOX_PRIV (box);

  priv->active_child = NULL;
  bobgui_widget_queue_draw (BOBGUI_WIDGET (box));
}

static void
bobgui_flow_box_drag_gesture_begin (BobguiGestureDrag *gesture,
                                 double          start_x,
                                 double          start_y,
                                 BobguiWidget      *widget)
{
  BobguiFlowBoxPrivate *priv = BOX_PRIV (widget);
  GdkModifierType state;

  if (priv->selection_mode != BOBGUI_SELECTION_MULTIPLE)
    {
      bobgui_gesture_set_state (BOBGUI_GESTURE (gesture), BOBGUI_EVENT_SEQUENCE_DENIED);
      return;
    }

  priv->rubberband_select = FALSE;
  priv->rubberband_first = NULL;
  priv->rubberband_last = NULL;

  state = bobgui_event_controller_get_current_event_state (BOBGUI_EVENT_CONTROLLER (gesture));
  priv->rubberband_modify = (state & GDK_CONTROL_MASK) != 0;
  priv->rubberband_extend = (state & GDK_SHIFT_MASK) != 0;
}

static void
bobgui_flow_box_stop_rubberband (BobguiFlowBox *box)
{
  BobguiFlowBoxPrivate *priv = BOX_PRIV (box);

  priv->rubberband_select = FALSE;
  priv->rubberband_first = NULL;
  priv->rubberband_last = NULL;

  bobgui_css_node_set_parent (priv->rubberband_node, NULL);
  priv->rubberband_node = NULL;

  remove_autoscroll (box);

  bobgui_widget_queue_draw (BOBGUI_WIDGET (box));
}

static void
bobgui_flow_box_drag_gesture_end (BobguiGestureDrag *gesture,
                               double          offset_x,
                               double          offset_y,
                               BobguiFlowBox     *box)
{
  BobguiFlowBoxPrivate *priv = BOX_PRIV (box);
  GdkEventSequence *sequence;

  if (!priv->rubberband_select)
    return;

  sequence = bobgui_gesture_single_get_current_sequence (BOBGUI_GESTURE_SINGLE (gesture));

  if (bobgui_gesture_handles_sequence (BOBGUI_GESTURE (gesture), sequence))
    {
      if (!priv->rubberband_extend && !priv->rubberband_modify)
        bobgui_flow_box_unselect_all_internal (box);

      if (priv->rubberband_first && priv->rubberband_last)
        bobgui_flow_box_select_all_between (box, priv->rubberband_first, priv->rubberband_last, priv->rubberband_modify);

      bobgui_flow_box_stop_rubberband (box);

      g_signal_emit (box, signals[SELECTED_CHILDREN_CHANGED], 0);
    }
  else
    bobgui_flow_box_stop_rubberband (box);

  bobgui_widget_queue_draw (BOBGUI_WIDGET (box));
}

static gboolean
bobgui_flow_box_key_controller_key_pressed (BobguiEventControllerKey *controller,
                                         guint                  keyval,
                                         guint                  keycode,
                                         GdkModifierType        state,
                                         BobguiWidget             *widget)
{
  BobguiFlowBox *box = BOBGUI_FLOW_BOX (widget);
  BobguiFlowBoxPrivate *priv = BOX_PRIV (box);

  if (priv->rubberband_select && keyval == GDK_KEY_Escape)
    {
      bobgui_flow_box_stop_rubberband (box);
      return TRUE;
    }

  return FALSE;
}

/* Realize and map {{{3 */

static void
bobgui_flow_box_unmap (BobguiWidget *widget)
{
  BobguiFlowBox *box = BOBGUI_FLOW_BOX (widget);

  remove_autoscroll (box);

  BOBGUI_WIDGET_CLASS (bobgui_flow_box_parent_class)->unmap (widget);
}

/**
 * bobgui_flow_box_remove:
 * @box: a `BobguiFlowBox`
 * @widget: the child widget to remove
 *
 * Removes a child from @box.
 */
void
bobgui_flow_box_remove (BobguiFlowBox *box,
                     BobguiWidget  *widget)
{
  BobguiFlowBoxPrivate *priv = BOX_PRIV (box);
  gboolean was_visible;
  gboolean was_selected;
  BobguiFlowBoxChild *child;

  g_return_if_fail (BOBGUI_IS_FLOW_BOX (box));
  g_return_if_fail (BOBGUI_IS_WIDGET (widget));
  g_return_if_fail (bobgui_widget_get_parent (widget) == BOBGUI_WIDGET (box) ||
                    bobgui_widget_get_parent (bobgui_widget_get_parent (widget)) == BOBGUI_WIDGET (box));

  if (BOBGUI_IS_FLOW_BOX_CHILD (widget))
    child = BOBGUI_FLOW_BOX_CHILD (widget);
  else
    {
      child = (BobguiFlowBoxChild*)bobgui_widget_get_parent (widget);
      if (!BOBGUI_IS_FLOW_BOX_CHILD (child))
        {
          g_warning ("Tried to remove non-child %p", widget);
          return;
        }
    }

  was_visible = child_is_visible (BOBGUI_WIDGET (child));
  was_selected = CHILD_PRIV (child)->selected;

  if (child == priv->active_child)
    priv->active_child = NULL;
  if (child == priv->selected_child)
    priv->selected_child = NULL;

  g_sequence_remove (CHILD_PRIV (child)->iter);
  bobgui_widget_unparent (BOBGUI_WIDGET (child));

  if (was_visible && bobgui_widget_get_visible (BOBGUI_WIDGET (box)))
    bobgui_widget_queue_resize (BOBGUI_WIDGET (box));

  if (was_selected && !bobgui_widget_in_destruction (BOBGUI_WIDGET (box)))
    g_signal_emit (box, signals[SELECTED_CHILDREN_CHANGED], 0);
}

/**
 * bobgui_flow_box_remove_all:
 * @box: a `BobguiFlowBox`
 *
 * Removes all children from @box.
 *
 * This function does nothing if @box is backed by a model.
 *
 * Since: 4.12
 */
void
bobgui_flow_box_remove_all (BobguiFlowBox *box)
{
  BobguiFlowBoxPrivate *priv = BOX_PRIV (box);
  BobguiWidget *widget = BOBGUI_WIDGET (box);
  BobguiWidget *child;

  g_return_if_fail (BOBGUI_IS_FLOW_BOX (box));

  if (priv->bound_model)
    return;

  while ((child = bobgui_widget_get_first_child (widget)) != NULL)
    bobgui_flow_box_remove (box, child);
}

/* Keynav {{{2 */

static gboolean
bobgui_flow_box_focus (BobguiWidget        *widget,
                    BobguiDirectionType  direction)
{
  BobguiFlowBox *box = BOBGUI_FLOW_BOX (widget);
  BobguiWidget *focus_child;
  GSequenceIter *iter;
  BobguiFlowBoxChild *next_focus_child;

  focus_child = bobgui_widget_get_focus_child (widget);
  next_focus_child = NULL;

  if (focus_child != NULL)
    {
      if (bobgui_widget_child_focus (focus_child, direction))
        return TRUE;

      iter = CHILD_PRIV (focus_child)->iter;

      if (direction == BOBGUI_DIR_LEFT || direction == BOBGUI_DIR_TAB_BACKWARD)
        iter = bobgui_flow_box_get_previous_focusable (box, iter);
      else if (direction == BOBGUI_DIR_RIGHT || direction == BOBGUI_DIR_TAB_FORWARD)
        iter = bobgui_flow_box_get_next_focusable (box, iter);
      else if (direction == BOBGUI_DIR_UP)
        iter = bobgui_flow_box_get_above_focusable (box, iter);
      else if (direction == BOBGUI_DIR_DOWN)
        iter = bobgui_flow_box_get_below_focusable (box, iter);

      if (iter != NULL)
        next_focus_child = g_sequence_get (iter);
    }
  else
    {
      if (BOX_PRIV (box)->selected_child)
        next_focus_child = BOX_PRIV (box)->selected_child;
      else
        {
          if (direction == BOBGUI_DIR_UP || direction == BOBGUI_DIR_TAB_BACKWARD)
            iter = bobgui_flow_box_get_last_focusable (box);
          else
            iter = bobgui_flow_box_get_first_focusable (box);

          if (iter != NULL)
            next_focus_child = g_sequence_get (iter);
        }
    }

  if (next_focus_child == NULL)
    {
      if (direction == BOBGUI_DIR_UP || direction == BOBGUI_DIR_DOWN ||
          direction == BOBGUI_DIR_LEFT || direction == BOBGUI_DIR_RIGHT)
        {
          if (bobgui_widget_keynav_failed (BOBGUI_WIDGET (box), direction))
            return TRUE;
        }

      return FALSE;
    }

  if (bobgui_widget_child_focus (BOBGUI_WIDGET (next_focus_child), direction))
    return TRUE;

  return TRUE;
}

static void
bobgui_flow_box_add_move_binding (BobguiWidgetClass  *widget_class,
                               guint            keyval,
                               GdkModifierType  modmask,
                               BobguiMovementStep  step,
                               int              count)
{
  bobgui_widget_class_add_binding_signal (widget_class,
                                       keyval, modmask,
                                       "move-cursor",
                                       "(iibb)", step, count, FALSE, FALSE);
  bobgui_widget_class_add_binding_signal (widget_class,
                                       keyval, modmask | GDK_SHIFT_MASK,
                                       "move-cursor",
                                       "(iibb)", step, count, TRUE, FALSE);
  bobgui_widget_class_add_binding_signal (widget_class,
                                       keyval, modmask | GDK_CONTROL_MASK,
                                       "move-cursor",
                                       "(iibb)", step, count, FALSE, TRUE);
  bobgui_widget_class_add_binding_signal (widget_class,
                                       keyval, modmask | GDK_SHIFT_MASK | GDK_CONTROL_MASK,
                                       "move-cursor",
                                       "(iibb)", step, count, TRUE, TRUE);
}

static void
bobgui_flow_box_activate_cursor_child (BobguiFlowBox *box)
{
  bobgui_flow_box_select_and_activate (box, BOX_PRIV (box)->cursor_child);
}

static void
bobgui_flow_box_toggle_cursor_child (BobguiFlowBox *box)
{
  BobguiFlowBoxPrivate *priv = BOX_PRIV (box);

  if (priv->cursor_child == NULL)
    return;

  if ((priv->selection_mode == BOBGUI_SELECTION_SINGLE ||
       priv->selection_mode == BOBGUI_SELECTION_MULTIPLE) &&
      CHILD_PRIV (priv->cursor_child)->selected)
    bobgui_flow_box_unselect_child_internal (box, priv->cursor_child);
  else
    bobgui_flow_box_select_and_activate (box, priv->cursor_child);
}

void
bobgui_flow_box_disable_move_cursor (BobguiFlowBox *box)
{
  BobguiFlowBoxPrivate *priv = BOX_PRIV (box);

  priv->disable_move_cursor = TRUE;
}

static gboolean
bobgui_flow_box_move_cursor (BobguiFlowBox      *box,
                          BobguiMovementStep  step,
                          int              count,
                          gboolean         extend,
                          gboolean         modify)
{
  BobguiFlowBoxPrivate *priv = BOX_PRIV (box);
  BobguiFlowBoxChild *child;
  BobguiFlowBoxChild *prev;
  BobguiFlowBoxChild *next;
  BobguiAllocation allocation;
  int page_size;
  GSequenceIter *iter;
  int start;
  BobguiAdjustment *adjustment;
  gboolean vertical;

  if (priv->disable_move_cursor)
    return FALSE;

  vertical = priv->orientation == BOBGUI_ORIENTATION_VERTICAL;

  if (vertical)
    {
       switch ((guint) step)
         {
         case BOBGUI_MOVEMENT_VISUAL_POSITIONS:
           step = BOBGUI_MOVEMENT_DISPLAY_LINES;
           break;
         case BOBGUI_MOVEMENT_DISPLAY_LINES:
           step = BOBGUI_MOVEMENT_VISUAL_POSITIONS;
           break;
         default:
           break;
         }
    }

  child = NULL;
  switch ((guint) step)
    {
    case BOBGUI_MOVEMENT_VISUAL_POSITIONS:
      if (priv->cursor_child != NULL)
        {
          iter = CHILD_PRIV (priv->cursor_child)->iter;
          if (bobgui_widget_get_direction (BOBGUI_WIDGET (box)) == BOBGUI_TEXT_DIR_RTL)
            count = - count;

          while (count < 0 && iter != NULL)
            {
              iter = bobgui_flow_box_get_previous_focusable (box, iter);
              count = count + 1;
            }
          while (count > 0 && iter != NULL)
            {
              iter = bobgui_flow_box_get_next_focusable (box, iter);
              count = count - 1;
            }

          if (iter != NULL && !g_sequence_iter_is_end (iter))
            child = g_sequence_get (iter);
        }
      break;

    case BOBGUI_MOVEMENT_BUFFER_ENDS:
      if (count < 0)
        iter = bobgui_flow_box_get_first_focusable (box);
      else
        iter = bobgui_flow_box_get_last_focusable (box);
      if (iter != NULL)
        child = g_sequence_get (iter);
      break;

    case BOBGUI_MOVEMENT_DISPLAY_LINES:
      if (priv->cursor_child != NULL)
        {
          iter = CHILD_PRIV (priv->cursor_child)->iter;

          while (count < 0 && iter != NULL)
            {
              iter = bobgui_flow_box_get_above_focusable (box, iter);
              count = count + 1;
            }
          while (count > 0 && iter != NULL)
            {
              iter = bobgui_flow_box_get_below_focusable (box, iter);
              count = count - 1;
            }

          if (iter != NULL)
            child = g_sequence_get (iter);
        }
      break;

    case BOBGUI_MOVEMENT_PAGES:
      page_size = 100;
      adjustment = vertical ? priv->hadjustment : priv->vadjustment;
      if (adjustment)
        page_size = bobgui_adjustment_get_page_increment (adjustment);

      if (priv->cursor_child != NULL)
        {
          child = priv->cursor_child;
          iter = CHILD_PRIV (child)->iter;
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
          bobgui_widget_get_allocation (BOBGUI_WIDGET (child), &allocation);
G_GNUC_END_IGNORE_DEPRECATIONS
          start = vertical ? allocation.x : allocation.y;

          if (count < 0)
            {
              int i = 0;

              /* Up */
              while (iter != NULL)
                {
                  iter = bobgui_flow_box_get_previous_focusable (box, iter);
                  if (iter == NULL)
                    break;

                  prev = g_sequence_get (iter);

                  /* go up an even number of rows */
                  if (i % priv->cur_children_per_line == 0)
                    {
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
                      bobgui_widget_get_allocation (BOBGUI_WIDGET (prev), &allocation);
G_GNUC_END_IGNORE_DEPRECATIONS
                      if ((vertical ? allocation.x : allocation.y) < start - page_size)
                        break;
                    }

                  child = prev;
                  i++;
                }
            }
          else
            {
              int i = 0;

              /* Down */
              while (!g_sequence_iter_is_end (iter))
                {
                  iter = bobgui_flow_box_get_next_focusable (box, iter);
                  if (iter == NULL || g_sequence_iter_is_end (iter))
                    break;

                  next = g_sequence_get (iter);

                  if (i % priv->cur_children_per_line == 0)
                    {
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
                      bobgui_widget_get_allocation (BOBGUI_WIDGET (next), &allocation);
G_GNUC_END_IGNORE_DEPRECATIONS
                      if ((vertical ? allocation.x : allocation.y) > start + page_size)
                        break;
                    }

                  child = next;
                  i++;
                }
            }
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
          bobgui_widget_get_allocation (BOBGUI_WIDGET (child), &allocation);
G_GNUC_END_IGNORE_DEPRECATIONS
        }
      break;

    default:
      g_assert_not_reached ();
    }

  if (child == NULL || child == priv->cursor_child)
    {
      BobguiDirectionType direction = count < 0 ? BOBGUI_DIR_UP : BOBGUI_DIR_DOWN;

      if (!bobgui_widget_keynav_failed (BOBGUI_WIDGET (box), direction))
        {
          return FALSE;
        }

      return TRUE;
    }

  /* If the child has its "focusable" property set to FALSE then it will
   * not grab the focus. We must pass the focus to its child directly.
   */
  if (!bobgui_widget_get_focusable (BOBGUI_WIDGET (child)))
    {
      BobguiWidget *subchild;

      subchild = bobgui_flow_box_child_get_child (BOBGUI_FLOW_BOX_CHILD (child));
      if (subchild)
        {
          BobguiDirectionType direction = count < 0 ? BOBGUI_DIR_TAB_BACKWARD : BOBGUI_DIR_TAB_FORWARD;
          bobgui_widget_child_focus (subchild, direction);
        }
    }

  bobgui_flow_box_update_cursor (box, child);
  if (!modify)
    bobgui_flow_box_update_selection (box, child, FALSE, extend);
  return TRUE;
}

/* Selection {{{2 */

static void
bobgui_flow_box_selected_children_changed (BobguiFlowBox *box)
{
}

/* GObject implementation {{{2 */

static void
bobgui_flow_box_get_property (GObject    *object,
                           guint       prop_id,
                           GValue     *value,
                           GParamSpec *pspec)
{
  BobguiFlowBox *box = BOBGUI_FLOW_BOX (object);
  BobguiFlowBoxPrivate *priv = BOX_PRIV (box);

  switch (prop_id)
    {
    case PROP_ORIENTATION:
      g_value_set_enum (value, priv->orientation);
      break;
    case PROP_HOMOGENEOUS:
      g_value_set_boolean (value, priv->homogeneous);
      break;
    case PROP_COLUMN_SPACING:
      g_value_set_uint (value, priv->column_spacing);
      break;
    case PROP_ROW_SPACING:
      g_value_set_uint (value, priv->row_spacing);
      break;
    case PROP_MIN_CHILDREN_PER_LINE:
      g_value_set_uint (value, priv->min_children_per_line);
      break;
    case PROP_MAX_CHILDREN_PER_LINE:
      g_value_set_uint (value, priv->max_children_per_line);
      break;
    case PROP_SELECTION_MODE:
      g_value_set_enum (value, priv->selection_mode);
      break;
    case PROP_ACTIVATE_ON_SINGLE_CLICK:
      g_value_set_boolean (value, priv->activate_on_single_click);
      break;
    case PROP_ACCEPT_UNPAIRED_RELEASE:
      g_value_set_boolean (value, priv->accept_unpaired_release);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bobgui_flow_box_set_property (GObject      *object,
                           guint         prop_id,
                           const GValue *value,
                           GParamSpec   *pspec)
{
  BobguiFlowBox *box = BOBGUI_FLOW_BOX (object);
  BobguiFlowBoxPrivate *priv = BOX_PRIV (box);

  switch (prop_id)
    {
    case PROP_ORIENTATION:
      {
        BobguiOrientation orientation = g_value_get_enum (value);

        if (priv->orientation != orientation)
          {
            priv->orientation = orientation;

            bobgui_widget_update_orientation (BOBGUI_WIDGET (box), priv->orientation);

            /* Re-box the children in the new orientation */
            bobgui_widget_queue_resize (BOBGUI_WIDGET (box));
            g_object_notify_by_pspec (object, pspec);
          }
      }
      break;
    case PROP_HOMOGENEOUS:
      bobgui_flow_box_set_homogeneous (box, g_value_get_boolean (value));
      break;
    case PROP_COLUMN_SPACING:
      bobgui_flow_box_set_column_spacing (box, g_value_get_uint (value));
      break;
    case PROP_ROW_SPACING:
      bobgui_flow_box_set_row_spacing (box, g_value_get_uint (value));
      break;
    case PROP_MIN_CHILDREN_PER_LINE:
      bobgui_flow_box_set_min_children_per_line (box, g_value_get_uint (value));
      break;
    case PROP_MAX_CHILDREN_PER_LINE:
      bobgui_flow_box_set_max_children_per_line (box, g_value_get_uint (value));
      break;
    case PROP_SELECTION_MODE:
      bobgui_flow_box_set_selection_mode (box, g_value_get_enum (value));
      break;
    case PROP_ACTIVATE_ON_SINGLE_CLICK:
      bobgui_flow_box_set_activate_on_single_click (box, g_value_get_boolean (value));
      break;
    case PROP_ACCEPT_UNPAIRED_RELEASE:
      bobgui_flow_box_set_accept_unpaired_release (box, g_value_get_boolean (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bobgui_flow_box_dispose (GObject *obj)
{
  BobguiFlowBoxPrivate *priv = BOX_PRIV (obj);

  if (priv->filter_destroy != NULL)
    priv->filter_destroy (priv->filter_data);
  if (priv->sort_destroy != NULL)
    priv->sort_destroy (priv->sort_data);

  if (priv->children)
    {
      GSequenceIter *iter;
      BobguiWidget *child;

      for (iter = g_sequence_get_begin_iter (priv->children);
           !g_sequence_iter_is_end (iter);
           iter = g_sequence_iter_next (iter))
        {
          child = g_sequence_get (iter);
          bobgui_widget_unparent (child);
        }
      g_clear_pointer (&priv->children, g_sequence_free);
    }

  g_clear_object (&priv->hadjustment);
  g_clear_object (&priv->vadjustment);

  if (priv->bound_model)
    {
      if (priv->create_widget_func_data_destroy)
        priv->create_widget_func_data_destroy (priv->create_widget_func_data);

      g_signal_handlers_disconnect_by_func (priv->bound_model, bobgui_flow_box_bound_model_changed, obj);
      g_clear_object (&priv->bound_model);
    }

  G_OBJECT_CLASS (bobgui_flow_box_parent_class)->dispose (obj);
}

static void
bobgui_flow_box_compute_expand (BobguiWidget *widget,
                             gboolean  *hexpand_p,
                             gboolean  *vexpand_p)
{
  BobguiWidget *w;
  gboolean hexpand = FALSE;
  gboolean vexpand = FALSE;

  for (w = bobgui_widget_get_first_child (widget);
       w != NULL;
       w = bobgui_widget_get_next_sibling (w))
    {
      hexpand = hexpand || bobgui_widget_compute_expand (w, BOBGUI_ORIENTATION_HORIZONTAL);
      vexpand = vexpand || bobgui_widget_compute_expand (w, BOBGUI_ORIENTATION_VERTICAL);
    }

  *hexpand_p = hexpand;
  *vexpand_p = vexpand;
}

static void
bobgui_flow_box_class_init (BobguiFlowBoxClass *class)
{
  GObjectClass      *object_class = G_OBJECT_CLASS (class);
  BobguiWidgetClass    *widget_class = BOBGUI_WIDGET_CLASS (class);

  object_class->dispose = bobgui_flow_box_dispose;
  object_class->get_property = bobgui_flow_box_get_property;
  object_class->set_property = bobgui_flow_box_set_property;

  widget_class->size_allocate = bobgui_flow_box_size_allocate;
  widget_class->unmap = bobgui_flow_box_unmap;
  widget_class->focus = bobgui_flow_box_focus;
  widget_class->snapshot = bobgui_flow_box_snapshot;
  widget_class->get_request_mode = bobgui_flow_box_get_request_mode;
  widget_class->compute_expand = bobgui_flow_box_compute_expand;
  widget_class->measure = bobgui_flow_box_measure;

  class->activate_cursor_child = bobgui_flow_box_activate_cursor_child;
  class->toggle_cursor_child = bobgui_flow_box_toggle_cursor_child;
  class->move_cursor = bobgui_flow_box_move_cursor;
  class->select_all = bobgui_flow_box_select_all;
  class->unselect_all = bobgui_flow_box_unselect_all;
  class->selected_children_changed = bobgui_flow_box_selected_children_changed;

  g_object_class_override_property (object_class, PROP_ORIENTATION, "orientation");

  /**
   * BobguiFlowBox:selection-mode:
   *
   * The selection mode used by the flow box.
   */
  props[PROP_SELECTION_MODE] =
    g_param_spec_enum ("selection-mode", NULL, NULL,
                       BOBGUI_TYPE_SELECTION_MODE,
                       BOBGUI_SELECTION_SINGLE,
                       BOBGUI_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiFlowBox:activate-on-single-click:
   *
   * Determines whether children can be activated with a single
   * click, or require a double-click.
   */
  props[PROP_ACTIVATE_ON_SINGLE_CLICK] =
    g_param_spec_boolean ("activate-on-single-click", NULL, NULL,
                          TRUE,
                          BOBGUI_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiFlowBox:accept-unpaired-release:
   *
   * Whether to accept unpaired release events.
   */
  props[PROP_ACCEPT_UNPAIRED_RELEASE] =
    g_param_spec_boolean ("accept-unpaired-release", NULL, NULL,
                          FALSE,
                          BOBGUI_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiFlowBox:homogeneous:
   *
   * Determines whether all children should be allocated the
   * same size.
   */
  props[PROP_HOMOGENEOUS] =
    g_param_spec_boolean ("homogeneous", NULL, NULL,
                          FALSE,
                          BOBGUI_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiFlowBox:min-children-per-line:
   *
   * The minimum number of children to allocate consecutively
   * in the given orientation.
   *
   * Setting the minimum children per line ensures
   * that a reasonably small height will be requested
   * for the overall minimum width of the box.
   */
  props[PROP_MIN_CHILDREN_PER_LINE] =
    g_param_spec_uint ("min-children-per-line", NULL, NULL,
                       0, G_MAXUINT, 0,
                       BOBGUI_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiFlowBox:max-children-per-line:
   *
   * The maximum amount of children to request space for consecutively
   * in the given orientation.
   */
  props[PROP_MAX_CHILDREN_PER_LINE] =
    g_param_spec_uint ("max-children-per-line", NULL, NULL,
                       1, G_MAXUINT, DEFAULT_MAX_CHILDREN_PER_LINE,
                       BOBGUI_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiFlowBox:row-spacing:
   *
   * The amount of vertical space between two children.
   */
  props[PROP_ROW_SPACING] =
    g_param_spec_uint ("row-spacing", NULL, NULL,
                       0, G_MAXUINT, 0,
                       BOBGUI_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiFlowBox:column-spacing:
   *
   * The amount of horizontal space between two children.
   */
  props[PROP_COLUMN_SPACING] =
    g_param_spec_uint ("column-spacing", NULL, NULL,
                       0, G_MAXUINT, 0,
                       BOBGUI_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, LAST_PROP, props);

  /**
   * BobguiFlowBox::child-activated:
   * @box: the `BobguiFlowBox` on which the signal is emitted
   * @child: the child that is activated
   *
   * Emitted when a child has been activated by the user.
   */
  signals[CHILD_ACTIVATED] = g_signal_new (I_("child-activated"),
                                           BOBGUI_TYPE_FLOW_BOX,
                                           G_SIGNAL_RUN_LAST,
                                           G_STRUCT_OFFSET (BobguiFlowBoxClass, child_activated),
                                           NULL, NULL,
                                           NULL,
                                           G_TYPE_NONE, 1,
                                           BOBGUI_TYPE_FLOW_BOX_CHILD);

  /**
   * BobguiFlowBox::selected-children-changed:
   * @box: the `BobguiFlowBox` on which the signal is emitted
   *
   * Emitted when the set of selected children changes.
   *
   * Use [method@Bobgui.FlowBox.selected_foreach] or
   * [method@Bobgui.FlowBox.get_selected_children] to obtain the
   * selected children.
   */
  signals[SELECTED_CHILDREN_CHANGED] = g_signal_new (I_("selected-children-changed"),
                                                     BOBGUI_TYPE_FLOW_BOX,
                                                     G_SIGNAL_RUN_FIRST,
                                                     G_STRUCT_OFFSET (BobguiFlowBoxClass, selected_children_changed),
                                                     NULL, NULL,
                                                     NULL,
                                                     G_TYPE_NONE, 0);

  /**
   * BobguiFlowBox::activate-cursor-child:
   * @box: the `BobguiFlowBox` on which the signal is emitted
   *
   * Emitted when the user activates the @box.
   *
   * This is a [keybinding signal](class.SignalAction.html).
   */
  signals[ACTIVATE_CURSOR_CHILD] = g_signal_new (I_("activate-cursor-child"),
                                                 BOBGUI_TYPE_FLOW_BOX,
                                                 G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                                                 G_STRUCT_OFFSET (BobguiFlowBoxClass, activate_cursor_child),
                                                 NULL, NULL,
                                                 NULL,
                                                 G_TYPE_NONE, 0);

  /**
   * BobguiFlowBox::toggle-cursor-child:
   * @box: the `BobguiFlowBox` on which the signal is emitted
   *
   * Emitted to toggle the selection of the child that has the focus.
   *
   * This is a [keybinding signal](class.SignalAction.html).
   *
   * The default binding for this signal is <kbd>Ctrl</kbd>-<kbd>Space</kbd>.
   */
  signals[TOGGLE_CURSOR_CHILD] = g_signal_new (I_("toggle-cursor-child"),
                                               BOBGUI_TYPE_FLOW_BOX,
                                               G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                                               G_STRUCT_OFFSET (BobguiFlowBoxClass, toggle_cursor_child),
                                               NULL, NULL,
                                               NULL,
                                               G_TYPE_NONE, 0);

  /**
   * BobguiFlowBox::move-cursor:
   * @box: the `BobguiFlowBox` on which the signal is emitted
   * @step: the granularity of the move, as a `BobguiMovementStep`
   * @count: the number of @step units to move
   * @extend: whether to extend the selection
   * @modify: whether to modify the selection
   *
   * Emitted when the user initiates a cursor movement.
   *
   * This is a [keybinding signal](class.SignalAction.html).
   * Applications should not connect to it, but may emit it with
   * g_signal_emit_by_name() if they need to control the cursor
   * programmatically.
   *
   * The default bindings for this signal come in two variants,
   * the variant with the Shift modifier extends the selection,
   * the variant without the Shift modifier does not.
   * There are too many key combinations to list them all here.
   *
   * - <kbd>←</kbd>, <kbd>→</kbd>, <kbd>↑</kbd>, <kbd>↓</kbd>
   *   move by individual children
   * - <kbd>Home</kbd>, <kbd>End</kbd> move to the ends of the box
   * - <kbd>PgUp</kbd>, <kbd>PgDn</kbd> move vertically by pages
   *
   * Returns: %TRUE to stop other handlers from being invoked for the event.
   *   %FALSE to propagate the event further.
   */
  signals[MOVE_CURSOR] = g_signal_new (I_("move-cursor"),
                                       BOBGUI_TYPE_FLOW_BOX,
                                       G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                                       G_STRUCT_OFFSET (BobguiFlowBoxClass, move_cursor),
                                       NULL, NULL,
                                       _bobgui_marshal_BOOLEAN__ENUM_INT_BOOLEAN_BOOLEAN,
                                       G_TYPE_BOOLEAN, 4,
                                       BOBGUI_TYPE_MOVEMENT_STEP, G_TYPE_INT, G_TYPE_BOOLEAN, G_TYPE_BOOLEAN);
  g_signal_set_va_marshaller (signals[MOVE_CURSOR],
                              G_TYPE_FROM_CLASS (class),
                              _bobgui_marshal_BOOLEAN__ENUM_INT_BOOLEAN_BOOLEANv);
  /**
   * BobguiFlowBox::select-all:
   * @box: the `BobguiFlowBox` on which the signal is emitted
   *
   * Emitted to select all children of the box,
   * if the selection mode permits it.
   *
   * This is a [keybinding signal](class.SignalAction.html).
   *
   * The default bindings for this signal is <kbd>Ctrl</kbd>-<kbd>a</kbd>.
   */
  signals[SELECT_ALL] = g_signal_new (I_("select-all"),
                                      BOBGUI_TYPE_FLOW_BOX,
                                      G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                                      G_STRUCT_OFFSET (BobguiFlowBoxClass, select_all),
                                      NULL, NULL,
                                      NULL,
                                      G_TYPE_NONE, 0);

  /**
   * BobguiFlowBox::unselect-all:
   * @box: the `BobguiFlowBox` on which the signal is emitted
   *
   * Emitted to unselect all children of the box,
   * if the selection mode permits it.
   *
   * This is a [keybinding signal](class.SignalAction.html).
   *
   * The default bindings for this signal is <kbd>Ctrl</kbd>-<kbd>Shift</kbd>-<kbd>a</kbd>.
   */
  signals[UNSELECT_ALL] = g_signal_new (I_("unselect-all"),
                                        BOBGUI_TYPE_FLOW_BOX,
                                        G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                                        G_STRUCT_OFFSET (BobguiFlowBoxClass, unselect_all),
                                        NULL, NULL,
                                        NULL,
                                        G_TYPE_NONE, 0);

  bobgui_widget_class_set_activate_signal (widget_class, signals[ACTIVATE_CURSOR_CHILD]);

  bobgui_flow_box_add_move_binding (widget_class, GDK_KEY_Home, 0,
                                 BOBGUI_MOVEMENT_BUFFER_ENDS, -1);
  bobgui_flow_box_add_move_binding (widget_class, GDK_KEY_KP_Home, 0,
                                 BOBGUI_MOVEMENT_BUFFER_ENDS, -1);
  bobgui_flow_box_add_move_binding (widget_class, GDK_KEY_End, 0,
                                 BOBGUI_MOVEMENT_BUFFER_ENDS, 1);
  bobgui_flow_box_add_move_binding (widget_class, GDK_KEY_KP_End, 0,
                                 BOBGUI_MOVEMENT_BUFFER_ENDS, 1);
  bobgui_flow_box_add_move_binding (widget_class, GDK_KEY_Up, 0,
                                 BOBGUI_MOVEMENT_DISPLAY_LINES, -1);
  bobgui_flow_box_add_move_binding (widget_class, GDK_KEY_KP_Up, 0,
                                 BOBGUI_MOVEMENT_DISPLAY_LINES, -1);
  bobgui_flow_box_add_move_binding (widget_class, GDK_KEY_Down, 0,
                                 BOBGUI_MOVEMENT_DISPLAY_LINES, 1);
  bobgui_flow_box_add_move_binding (widget_class, GDK_KEY_KP_Down, 0,
                                 BOBGUI_MOVEMENT_DISPLAY_LINES, 1);
  bobgui_flow_box_add_move_binding (widget_class, GDK_KEY_Page_Up, 0,
                                 BOBGUI_MOVEMENT_PAGES, -1);
  bobgui_flow_box_add_move_binding (widget_class, GDK_KEY_KP_Page_Up, 0,
                                 BOBGUI_MOVEMENT_PAGES, -1);
  bobgui_flow_box_add_move_binding (widget_class, GDK_KEY_Page_Down, 0,
                                 BOBGUI_MOVEMENT_PAGES, 1);
  bobgui_flow_box_add_move_binding (widget_class, GDK_KEY_KP_Page_Down, 0,
                                 BOBGUI_MOVEMENT_PAGES, 1);

  bobgui_flow_box_add_move_binding (widget_class, GDK_KEY_Right, 0,
                                 BOBGUI_MOVEMENT_VISUAL_POSITIONS, 1);
  bobgui_flow_box_add_move_binding (widget_class, GDK_KEY_KP_Right, 0,
                                 BOBGUI_MOVEMENT_VISUAL_POSITIONS, 1);
  bobgui_flow_box_add_move_binding (widget_class, GDK_KEY_Left, 0,
                                 BOBGUI_MOVEMENT_VISUAL_POSITIONS, -1);
  bobgui_flow_box_add_move_binding (widget_class, GDK_KEY_KP_Left, 0,
                                 BOBGUI_MOVEMENT_VISUAL_POSITIONS, -1);

  bobgui_widget_class_add_binding_signal (widget_class,
                                       GDK_KEY_space, GDK_CONTROL_MASK,
                                       "toggle-cursor-child",
                                       NULL);
  bobgui_widget_class_add_binding_signal (widget_class,
                                       GDK_KEY_KP_Space, GDK_CONTROL_MASK,
                                       "toggle-cursor-child",
                                       NULL);

  bobgui_widget_class_add_binding_signal (widget_class,
                                       GDK_KEY_a, GDK_CONTROL_MASK,
                                       "select-all",
                                       NULL);
  bobgui_widget_class_add_binding_signal (widget_class,
                                       GDK_KEY_a, GDK_CONTROL_MASK | GDK_SHIFT_MASK,
                                       "unselect-all",
                                       NULL);

  bobgui_widget_class_set_css_name (widget_class, I_("flowbox"));
  bobgui_widget_class_set_accessible_role (widget_class, BOBGUI_ACCESSIBLE_ROLE_GRID);
}

static void
bobgui_flow_box_init (BobguiFlowBox *box)
{
  BobguiFlowBoxPrivate *priv = BOX_PRIV (box);
  BobguiEventController *controller;
  BobguiGesture *gesture;

  priv->orientation = BOBGUI_ORIENTATION_HORIZONTAL;
  priv->selection_mode = BOBGUI_SELECTION_SINGLE;
  priv->max_children_per_line = DEFAULT_MAX_CHILDREN_PER_LINE;
  priv->column_spacing = 0;
  priv->row_spacing = 0;
  priv->activate_on_single_click = TRUE;

  bobgui_widget_update_orientation (BOBGUI_WIDGET (box), priv->orientation);

  priv->children = g_sequence_new (NULL);

  gesture = bobgui_gesture_click_new ();
  bobgui_gesture_single_set_touch_only (BOBGUI_GESTURE_SINGLE (gesture),
                                     FALSE);
  bobgui_gesture_single_set_button (BOBGUI_GESTURE_SINGLE (gesture),
                                 GDK_BUTTON_PRIMARY);
  bobgui_event_controller_set_propagation_phase (BOBGUI_EVENT_CONTROLLER (gesture),
                                              BOBGUI_PHASE_BUBBLE);
  g_signal_connect (gesture, "pressed",
                    G_CALLBACK (bobgui_flow_box_click_gesture_pressed), box);
  g_signal_connect (gesture, "released",
                    G_CALLBACK (bobgui_flow_box_click_gesture_released), box);
  g_signal_connect (gesture, "stopped",
                    G_CALLBACK (bobgui_flow_box_click_gesture_stopped), box);
  g_signal_connect (gesture, "unpaired-release",
                    G_CALLBACK (bobgui_flow_box_click_unpaired_release), box);
  bobgui_widget_add_controller (BOBGUI_WIDGET (box), BOBGUI_EVENT_CONTROLLER (gesture));

  priv->drag_gesture = bobgui_gesture_drag_new ();
  bobgui_gesture_single_set_touch_only (BOBGUI_GESTURE_SINGLE (priv->drag_gesture),
                                     FALSE);
  bobgui_gesture_single_set_button (BOBGUI_GESTURE_SINGLE (priv->drag_gesture),
                                 GDK_BUTTON_PRIMARY);
  bobgui_event_controller_set_propagation_phase (BOBGUI_EVENT_CONTROLLER (priv->drag_gesture),
                                              BOBGUI_PHASE_CAPTURE);
  g_signal_connect (priv->drag_gesture, "drag-begin",
                    G_CALLBACK (bobgui_flow_box_drag_gesture_begin), box);
  g_signal_connect (priv->drag_gesture, "drag-update",
                    G_CALLBACK (bobgui_flow_box_drag_gesture_update), box);
  g_signal_connect (priv->drag_gesture, "drag-end",
                    G_CALLBACK (bobgui_flow_box_drag_gesture_end), box);
  bobgui_widget_add_controller (BOBGUI_WIDGET (box), BOBGUI_EVENT_CONTROLLER (priv->drag_gesture));

  controller = bobgui_event_controller_key_new ();
  g_signal_connect (controller, "key-pressed",
                    G_CALLBACK (bobgui_flow_box_key_controller_key_pressed), box);
  bobgui_widget_add_controller (BOBGUI_WIDGET (box), controller);
}

static void
bobgui_flow_box_bound_model_changed (GListModel *list,
                                  guint       position,
                                  guint       removed,
                                  guint       added,
                                  gpointer    user_data)
{
  BobguiFlowBox *box = user_data;
  BobguiFlowBoxPrivate *priv = BOX_PRIV (box);
  int i;

  while (removed--)
    {
      BobguiFlowBoxChild *child;

      child = bobgui_flow_box_get_child_at_index (box, position);
      bobgui_flow_box_remove (box, BOBGUI_WIDGET (child));
    }

  for (i = 0; i < added; i++)
    {
      GObject *item;
      BobguiWidget *widget;

      item = g_list_model_get_item (list, position + i);
      widget = priv->create_widget_func (item, priv->create_widget_func_data);

      /* We need to sink the floating reference here, so that we can accept
       * both instances created with a floating reference (e.g. C functions
       * that just return the result of g_object_new()) and without (e.g.
       * from language bindings which will automatically sink the floating
       * reference).
       *
       * See the similar code in bobguilistbox.c:bobgui_list_box_bound_model_changed.
       */
      if (g_object_is_floating (widget))
        g_object_ref_sink (widget);

      bobgui_widget_set_visible (widget, TRUE);
      bobgui_flow_box_insert (box, widget, position + i);

      g_object_unref (widget);
      g_object_unref (item);
    }
}

/* Buildable implementation {{{3 */

static BobguiBuildableIface *parent_buildable_iface;

static void
bobgui_flow_box_buildable_add_child (BobguiBuildable *buildable,
                                  BobguiBuilder   *builder,
                                  GObject      *child,
                                  const char   *type)
{
  if (BOBGUI_IS_WIDGET (child))
    bobgui_flow_box_insert (BOBGUI_FLOW_BOX (buildable), BOBGUI_WIDGET (child), -1);
  else
    parent_buildable_iface->add_child (buildable, builder, child, type);
}

static void
bobgui_flow_box_buildable_iface_init (BobguiBuildableIface *iface)
{
  parent_buildable_iface = g_type_interface_peek_parent (iface);

  iface->add_child = bobgui_flow_box_buildable_add_child;
}
   /* Public API {{{2 */

/**
 * bobgui_flow_box_new:
 *
 * Creates a `BobguiFlowBox`.
 *
 * Returns: a new `BobguiFlowBox`
 */
BobguiWidget *
bobgui_flow_box_new (void)
{
  return (BobguiWidget *)g_object_new (BOBGUI_TYPE_FLOW_BOX, NULL);
}

static void
bobgui_flow_box_insert_widget (BobguiFlowBox    *box,
                            BobguiWidget     *child,
                            GSequenceIter *iter)
{
  GSequenceIter *prev_iter;
  BobguiWidget *sibling;

  prev_iter = g_sequence_iter_prev (iter);

  if (prev_iter != iter)
    sibling = g_sequence_get (prev_iter);
  else
    sibling = NULL;

  bobgui_widget_insert_after (child, BOBGUI_WIDGET (box), sibling);
}

/**
 * bobgui_flow_box_prepend:
 * @self: a `BobguiFlowBox
 * @child: the `BobguiWidget` to add
 *
 * Adds @child to the start of @self.
 *
 * If a sort function is set, the widget will
 * actually be inserted at the calculated position.
 *
 * See also: [method@Bobgui.FlowBox.insert].
 *
 * Since: 4.6
 */
void
bobgui_flow_box_prepend (BobguiFlowBox *self,
                      BobguiWidget  *child)
{
  g_return_if_fail (BOBGUI_IS_FLOW_BOX (self));
  g_return_if_fail (BOBGUI_IS_WIDGET (child));

  bobgui_flow_box_insert (self, child, 0);
}

/**
 * bobgui_flow_box_append:
 * @self: a `BobguiFlowBox
 * @child: the `BobguiWidget` to add
 *
 * Adds @child to the end of @self.
 *
 * If a sort function is set, the widget will
 * actually be inserted at the calculated position.
 *
 * See also: [method@Bobgui.FlowBox.insert].
 *
 * Since: 4.6
 */
void
bobgui_flow_box_append (BobguiFlowBox *self,
                     BobguiWidget  *child)
{
  g_return_if_fail (BOBGUI_IS_FLOW_BOX (self));
  g_return_if_fail (BOBGUI_IS_WIDGET (child));

  bobgui_flow_box_insert (self, child, -1);
}

/**
 * bobgui_flow_box_insert:
 * @box: a `BobguiFlowBox`
 * @widget: the `BobguiWidget` to add
 * @position: the position to insert @child in
 *
 * Inserts the @widget into @box at @position.
 *
 * If a sort function is set, the widget will actually be inserted
 * at the calculated position.
 *
 * If @position is -1, or larger than the total number of children
 * in the @box, then the @widget will be appended to the end.
 */
void
bobgui_flow_box_insert (BobguiFlowBox *box,
                     BobguiWidget  *widget,
                     int         position)
{
  BobguiFlowBoxPrivate *priv;
  BobguiFlowBoxChild *child;
  GSequenceIter *iter;

  g_return_if_fail (BOBGUI_IS_FLOW_BOX (box));
  g_return_if_fail (BOBGUI_IS_WIDGET (widget));

  priv = BOX_PRIV (box);

  if (BOBGUI_IS_FLOW_BOX_CHILD (widget))
    child = BOBGUI_FLOW_BOX_CHILD (widget);
  else
    {
      child = BOBGUI_FLOW_BOX_CHILD (bobgui_flow_box_child_new ());
      bobgui_flow_box_child_set_child (child, widget);
    }

  if (priv->sort_func != NULL)
    iter = g_sequence_insert_sorted (priv->children, child,
                                     (GCompareDataFunc)bobgui_flow_box_sort, box);
  else if (position == 0)
    iter = g_sequence_prepend (priv->children, child);
  else if (position == -1)
    iter = g_sequence_append (priv->children, child);
  else
    {
      GSequenceIter *pos;
      pos = g_sequence_get_iter_at_pos (priv->children, position);
      iter = g_sequence_insert_before (pos, child);
    }

  CHILD_PRIV (child)->iter = iter;
  bobgui_flow_box_insert_widget (box, BOBGUI_WIDGET (child), iter);
  bobgui_flow_box_apply_filter (box, child);
}

/**
 * bobgui_flow_box_get_child_at_index:
 * @box: a `BobguiFlowBox`
 * @idx: the position of the child
 *
 * Gets the nth child in the @box.
 *
 * Returns: (transfer none) (nullable): the child widget, which will
 *   always be a `BobguiFlowBoxChild` or %NULL in case no child widget
 *   with the given index exists.
 */
BobguiFlowBoxChild *
bobgui_flow_box_get_child_at_index (BobguiFlowBox *box,
                                 int         idx)
{
  GSequenceIter *iter;

  g_return_val_if_fail (BOBGUI_IS_FLOW_BOX (box), NULL);

  iter = g_sequence_get_iter_at_pos (BOX_PRIV (box)->children, idx);
  if (!g_sequence_iter_is_end (iter))
    return g_sequence_get (iter);

  return NULL;
}

/**
 * bobgui_flow_box_get_child_at_pos:
 * @box: a `BobguiFlowBox`
 * @x: the x coordinate of the child
 * @y: the y coordinate of the child
 *
 * Gets the child in the (@x, @y) position.
 *
 * Both @x and @y are assumed to be relative to the origin of @box.
 *
 * Returns: (transfer none) (nullable): the child widget, which will
 *   always be a `BobguiFlowBoxChild` or %NULL in case no child widget
 *   exists for the given x and y coordinates.
 */
BobguiFlowBoxChild *
bobgui_flow_box_get_child_at_pos (BobguiFlowBox *box,
                               int         x,
                               int         y)
{
  BobguiWidget *child = bobgui_widget_pick (BOBGUI_WIDGET (box), x, y, BOBGUI_PICK_DEFAULT);

  if (!child)
    return NULL;

  return (BobguiFlowBoxChild *)bobgui_widget_get_ancestor (child, BOBGUI_TYPE_FLOW_BOX_CHILD);
}

/**
 * bobgui_flow_box_set_hadjustment:
 * @box: a `BobguiFlowBox`
 * @adjustment: an adjustment which should be adjusted
 *    when the focus is moved among the descendents of @container
 *
 * Hooks up an adjustment to focus handling in @box.
 *
 * The adjustment is also used for autoscrolling during
 * rubberband selection. See [method@Bobgui.ScrolledWindow.get_hadjustment]
 * for a typical way of obtaining the adjustment, and
 * [method@Bobgui.FlowBox.set_vadjustment] for setting the vertical
 * adjustment.
 *
 * The adjustments have to be in pixel units and in the same
 * coordinate system as the allocation for immediate children
 * of the box.
 */
void
bobgui_flow_box_set_hadjustment (BobguiFlowBox    *box,
                              BobguiAdjustment *adjustment)
{
  BobguiFlowBoxPrivate *priv;

  g_return_if_fail (BOBGUI_IS_FLOW_BOX (box));
  g_return_if_fail (BOBGUI_IS_ADJUSTMENT (adjustment));

  priv = BOX_PRIV (box);

  g_object_ref (adjustment);
  if (priv->hadjustment)
    g_object_unref (priv->hadjustment);
  priv->hadjustment = adjustment;
}

/**
 * bobgui_flow_box_set_vadjustment:
 * @box: a `BobguiFlowBox`
 * @adjustment: an adjustment which should be adjusted
 *    when the focus is moved among the descendents of @container
 *
 * Hooks up an adjustment to focus handling in @box.
 *
 * The adjustment is also used for autoscrolling during
 * rubberband selection. See [method@Bobgui.ScrolledWindow.get_vadjustment]
 * for a typical way of obtaining the adjustment, and
 * [method@Bobgui.FlowBox.set_hadjustment] for setting the horizontal
 * adjustment.
 *
 * The adjustments have to be in pixel units and in the same
 * coordinate system as the allocation for immediate children
 * of the box.
 */
void
bobgui_flow_box_set_vadjustment (BobguiFlowBox    *box,
                              BobguiAdjustment *adjustment)
{
  BobguiFlowBoxPrivate *priv;

  g_return_if_fail (BOBGUI_IS_FLOW_BOX (box));
  g_return_if_fail (BOBGUI_IS_ADJUSTMENT (adjustment));

  priv = BOX_PRIV (box);

  g_object_ref (adjustment);
  if (priv->vadjustment)
    g_object_unref (priv->vadjustment);
  priv->vadjustment = adjustment;
}

static void
bobgui_flow_box_check_model_compat (BobguiFlowBox *box)
{
  BobguiFlowBoxPrivate *priv = BOX_PRIV (box);

  if (priv->bound_model &&
      (priv->sort_func || priv->filter_func))
    g_warning ("BobguiFlowBox with a model will ignore sort and filter functions");
}

/**
 * bobgui_flow_box_bind_model:
 * @box: a `BobguiFlowBox`
 * @model: (nullable): the `GListModel` to be bound to @box
 * @create_widget_func: (scope notified) (closure user_data) (destroy user_data_free_func): a function
 *   that creates widgets for items
 * @user_data: user data passed to @create_widget_func
 * @user_data_free_func: function for freeing @user_data
 *
 * Binds @model to @box.
 *
 * If @box was already bound to a model, that previous binding is
 * destroyed.
 *
 * The contents of @box are cleared and then filled with widgets that
 * represent items from @model. @box is updated whenever @model changes.
 * If @model is %NULL, @box is left empty.
 *
 * It is undefined to add or remove widgets directly (for example, with
 * [method@Bobgui.FlowBox.insert]) while @box is bound to a model.
 *
 * Note that using a model is incompatible with the filtering and sorting
 * functionality in `BobguiFlowBox`. When using a model, filtering and sorting
 * should be implemented by the model.
 */
void
bobgui_flow_box_bind_model (BobguiFlowBox                 *box,
                         GListModel                 *model,
                         BobguiFlowBoxCreateWidgetFunc  create_widget_func,
                         gpointer                    user_data,
                         GDestroyNotify              user_data_free_func)
{
  BobguiFlowBoxPrivate *priv = BOX_PRIV (box);
  BobguiWidget *child;

  g_return_if_fail (BOBGUI_IS_FLOW_BOX (box));
  g_return_if_fail (model == NULL || G_IS_LIST_MODEL (model));
  g_return_if_fail (model == NULL || create_widget_func != NULL);

  if (priv->bound_model)
    {
      if (priv->create_widget_func_data_destroy)
        priv->create_widget_func_data_destroy (priv->create_widget_func_data);

      g_signal_handlers_disconnect_by_func (priv->bound_model, bobgui_flow_box_bound_model_changed, box);
      g_clear_object (&priv->bound_model);
    }

  while ((child = bobgui_widget_get_first_child (BOBGUI_WIDGET (box))))
    bobgui_flow_box_remove (box, child);

  if (model == NULL)
    return;

  priv->bound_model = g_object_ref (model);
  priv->create_widget_func = create_widget_func;
  priv->create_widget_func_data = user_data;
  priv->create_widget_func_data_destroy = user_data_free_func;

  bobgui_flow_box_check_model_compat (box);

  g_signal_connect (priv->bound_model, "items-changed", G_CALLBACK (bobgui_flow_box_bound_model_changed), box);
  bobgui_flow_box_bound_model_changed (model, 0, 0, g_list_model_get_n_items (model), box);
}

/*  Setters and getters {{{2 */

/**
 * bobgui_flow_box_get_homogeneous:
 * @box: a `BobguiFlowBox`
 *
 * Returns whether the box is homogeneous.
 *
 * Returns: %TRUE if the box is homogeneous.
 */
gboolean
bobgui_flow_box_get_homogeneous (BobguiFlowBox *box)
{
  g_return_val_if_fail (BOBGUI_IS_FLOW_BOX (box), FALSE);

  return BOX_PRIV (box)->homogeneous;
}

/**
 * bobgui_flow_box_set_homogeneous:
 * @box: a `BobguiFlowBox`
 * @homogeneous: %TRUE to create equal allotments,
 *   %FALSE for variable allotments
 *
 * Sets whether or not all children of @box are given
 * equal space in the box.
 */
void
bobgui_flow_box_set_homogeneous (BobguiFlowBox *box,
                              gboolean    homogeneous)
{
  g_return_if_fail (BOBGUI_IS_FLOW_BOX (box));

  homogeneous = homogeneous != FALSE;

  if (BOX_PRIV (box)->homogeneous != homogeneous)
    {
      BOX_PRIV (box)->homogeneous = homogeneous;

      g_object_notify_by_pspec (G_OBJECT (box), props[PROP_HOMOGENEOUS]);
      bobgui_widget_queue_resize (BOBGUI_WIDGET (box));
    }
}

/**
 * bobgui_flow_box_set_row_spacing:
 * @box: a `BobguiFlowBox`
 * @spacing: the spacing to use
 *
 * Sets the vertical space to add between children.
 */
void
bobgui_flow_box_set_row_spacing (BobguiFlowBox *box,
                              guint       spacing)
{
  g_return_if_fail (BOBGUI_IS_FLOW_BOX (box));

  if (BOX_PRIV (box)->row_spacing != spacing)
    {
      BOX_PRIV (box)->row_spacing = spacing;

      bobgui_widget_queue_resize (BOBGUI_WIDGET (box));
      g_object_notify_by_pspec (G_OBJECT (box), props[PROP_ROW_SPACING]);
    }
}

/**
 * bobgui_flow_box_get_row_spacing:
 * @box: a `BobguiFlowBox`
 *
 * Gets the vertical spacing.
 *
 * Returns: the vertical spacing
 */
guint
bobgui_flow_box_get_row_spacing (BobguiFlowBox *box)
{
  g_return_val_if_fail (BOBGUI_IS_FLOW_BOX (box), FALSE);

  return BOX_PRIV (box)->row_spacing;
}

/**
 * bobgui_flow_box_set_column_spacing:
 * @box: a `BobguiFlowBox`
 * @spacing: the spacing to use
 *
 * Sets the horizontal space to add between children.
 */
void
bobgui_flow_box_set_column_spacing (BobguiFlowBox *box,
                                 guint       spacing)
{
  g_return_if_fail (BOBGUI_IS_FLOW_BOX (box));

  if (BOX_PRIV (box)->column_spacing != spacing)
    {
      BOX_PRIV (box)->column_spacing = spacing;

      bobgui_widget_queue_resize (BOBGUI_WIDGET (box));
      g_object_notify_by_pspec (G_OBJECT (box), props[PROP_COLUMN_SPACING]);
    }
}

/**
 * bobgui_flow_box_get_column_spacing:
 * @box: a `BobguiFlowBox`
 *
 * Gets the horizontal spacing.
 *
 * Returns: the horizontal spacing
 */
guint
bobgui_flow_box_get_column_spacing (BobguiFlowBox *box)
{
  g_return_val_if_fail (BOBGUI_IS_FLOW_BOX (box), FALSE);

  return BOX_PRIV (box)->column_spacing;
}

/**
 * bobgui_flow_box_set_min_children_per_line:
 * @box: a `BobguiFlowBox`
 * @n_children: the minimum number of children per line
 *
 * Sets the minimum number of children to line up
 * in @box’s orientation before flowing.
 */
void
bobgui_flow_box_set_min_children_per_line (BobguiFlowBox *box,
                                        guint       n_children)
{
  g_return_if_fail (BOBGUI_IS_FLOW_BOX (box));

  if (BOX_PRIV (box)->min_children_per_line != n_children)
    {
      BOX_PRIV (box)->min_children_per_line = n_children;

      bobgui_widget_queue_resize (BOBGUI_WIDGET (box));
      g_object_notify_by_pspec (G_OBJECT (box), props[PROP_MIN_CHILDREN_PER_LINE]);
    }
}

/**
 * bobgui_flow_box_get_min_children_per_line:
 * @box: a `BobguiFlowBox`
 *
 * Gets the minimum number of children per line.
 *
 * Returns: the minimum number of children per line
 */
guint
bobgui_flow_box_get_min_children_per_line (BobguiFlowBox *box)
{
  g_return_val_if_fail (BOBGUI_IS_FLOW_BOX (box), FALSE);

  return BOX_PRIV (box)->min_children_per_line;
}

/**
 * bobgui_flow_box_set_max_children_per_line:
 * @box: a `BobguiFlowBox`
 * @n_children: the maximum number of children per line
 *
 * Sets the maximum number of children to request and
 * allocate space for in @box’s orientation.
 *
 * Setting the maximum number of children per line
 * limits the overall natural size request to be no more
 * than @n_children children long in the given orientation.
 */
void
bobgui_flow_box_set_max_children_per_line (BobguiFlowBox *box,
                                        guint       n_children)
{
  g_return_if_fail (BOBGUI_IS_FLOW_BOX (box));
  g_return_if_fail (n_children > 0);

  if (BOX_PRIV (box)->max_children_per_line != n_children)
    {
      BOX_PRIV (box)->max_children_per_line = n_children;

      bobgui_widget_queue_resize (BOBGUI_WIDGET (box));
      g_object_notify_by_pspec (G_OBJECT (box), props[PROP_MAX_CHILDREN_PER_LINE]);
    }
}

/**
 * bobgui_flow_box_get_max_children_per_line:
 * @box: a `BobguiFlowBox`
 *
 * Gets the maximum number of children per line.
 *
 * Returns: the maximum number of children per line
 */
guint
bobgui_flow_box_get_max_children_per_line (BobguiFlowBox *box)
{
  g_return_val_if_fail (BOBGUI_IS_FLOW_BOX (box), FALSE);

  return BOX_PRIV (box)->max_children_per_line;
}

/**
 * bobgui_flow_box_set_activate_on_single_click:
 * @box: a `BobguiFlowBox`
 * @single: %TRUE to emit child-activated on a single click
 *
 * If @single is %TRUE, children will be activated when you click
 * on them, otherwise you need to double-click.
 */
void
bobgui_flow_box_set_activate_on_single_click (BobguiFlowBox *box,
                                           gboolean    single)
{
  g_return_if_fail (BOBGUI_IS_FLOW_BOX (box));

  single = single != FALSE;

  if (BOX_PRIV (box)->activate_on_single_click != single)
    {
      BOX_PRIV (box)->activate_on_single_click = single;
      g_object_notify_by_pspec (G_OBJECT (box), props[PROP_ACTIVATE_ON_SINGLE_CLICK]);
    }
}

/**
 * bobgui_flow_box_get_activate_on_single_click:
 * @box: a `BobguiFlowBox`
 *
 * Returns whether children activate on single clicks.
 *
 * Returns: %TRUE if children are activated on single click,
 *   %FALSE otherwise
 */
gboolean
bobgui_flow_box_get_activate_on_single_click (BobguiFlowBox *box)
{
  g_return_val_if_fail (BOBGUI_IS_FLOW_BOX (box), FALSE);

  return BOX_PRIV (box)->activate_on_single_click;
}

static void
bobgui_flow_box_set_accept_unpaired_release (BobguiFlowBox *box,
                                          gboolean    accept)
{
  if (BOX_PRIV (box)->accept_unpaired_release == accept)
    return;

  BOX_PRIV (box)->accept_unpaired_release = accept;
  g_object_notify_by_pspec (G_OBJECT (box), props[PROP_ACCEPT_UNPAIRED_RELEASE]);
}

 /* Selection handling {{{2 */

/**
 * bobgui_flow_box_get_selected_children:
 * @box: a `BobguiFlowBox`
 *
 * Creates a list of all selected children.
 *
 * Returns: (element-type BobguiFlowBoxChild) (transfer container):
 *   A `GList` containing the `BobguiWidget` for each selected child.
 *   Free with g_list_free() when done.
 */
GList *
bobgui_flow_box_get_selected_children (BobguiFlowBox *box)
{
  BobguiFlowBoxChild *child;
  GSequenceIter *iter;
  GList *selected = NULL;

  g_return_val_if_fail (BOBGUI_IS_FLOW_BOX (box), NULL);

  for (iter = g_sequence_get_begin_iter (BOX_PRIV (box)->children);
       !g_sequence_iter_is_end (iter);
       iter = g_sequence_iter_next (iter))
    {
      child = g_sequence_get (iter);
      if (CHILD_PRIV (child)->selected)
        selected = g_list_prepend (selected, child);
    }

  return g_list_reverse (selected);
}

/**
 * bobgui_flow_box_select_child:
 * @box: a `BobguiFlowBox`
 * @child: a child of @box
 *
 * Selects a single child of @box, if the selection
 * mode allows it.
 */
void
bobgui_flow_box_select_child (BobguiFlowBox      *box,
                           BobguiFlowBoxChild *child)
{
  g_return_if_fail (BOBGUI_IS_FLOW_BOX (box));
  g_return_if_fail (BOBGUI_IS_FLOW_BOX_CHILD (child));

  bobgui_flow_box_select_child_internal (box, child);
}

/**
 * bobgui_flow_box_unselect_child:
 * @box: a `BobguiFlowBox`
 * @child: a child of @box
 *
 * Unselects a single child of @box, if the selection
 * mode allows it.
 */
void
bobgui_flow_box_unselect_child (BobguiFlowBox      *box,
                             BobguiFlowBoxChild *child)
{
  g_return_if_fail (BOBGUI_IS_FLOW_BOX (box));
  g_return_if_fail (BOBGUI_IS_FLOW_BOX_CHILD (child));

  bobgui_flow_box_unselect_child_internal (box, child);
}

/**
 * bobgui_flow_box_select_all:
 * @box: a `BobguiFlowBox`
 *
 * Select all children of @box, if the selection
 * mode allows it.
 */
void
bobgui_flow_box_select_all (BobguiFlowBox *box)
{
  g_return_if_fail (BOBGUI_IS_FLOW_BOX (box));

  if (BOX_PRIV (box)->selection_mode != BOBGUI_SELECTION_MULTIPLE)
    return;

  if (g_sequence_get_length (BOX_PRIV (box)->children) > 0)
    {
      bobgui_flow_box_select_all_between (box, NULL, NULL, FALSE);
      g_signal_emit (box, signals[SELECTED_CHILDREN_CHANGED], 0);
    }
}

/**
 * bobgui_flow_box_unselect_all:
 * @box: a `BobguiFlowBox`
 *
 * Unselect all children of @box, if the selection
 * mode allows it.
 */
void
bobgui_flow_box_unselect_all (BobguiFlowBox *box)
{
  gboolean dirty = FALSE;

  g_return_if_fail (BOBGUI_IS_FLOW_BOX (box));

  if (BOX_PRIV (box)->selection_mode == BOBGUI_SELECTION_BROWSE)
    return;

  dirty = bobgui_flow_box_unselect_all_internal (box);

  if (dirty)
    g_signal_emit (box, signals[SELECTED_CHILDREN_CHANGED], 0);
}

/**
 * BobguiFlowBoxForeachFunc:
 * @box: a `BobguiFlowBox`
 * @child: a `BobguiFlowBoxChild`
 * @user_data: (closure): user data
 *
 * A function used by bobgui_flow_box_selected_foreach().
 *
 * It will be called on every selected child of the @box.
 */

/**
 * bobgui_flow_box_selected_foreach:
 * @box: a `BobguiFlowBox`
 * @func: (scope call): the function to call for each selected child
 * @data: user data to pass to the function
 *
 * Calls a function for each selected child.
 *
 * Note that the selection cannot be modified from within
 * this function.
 */
void
bobgui_flow_box_selected_foreach (BobguiFlowBox            *box,
                               BobguiFlowBoxForeachFunc  func,
                               gpointer               data)
{
  BobguiFlowBoxChild *child;
  GSequenceIter *iter;

  g_return_if_fail (BOBGUI_IS_FLOW_BOX (box));

  for (iter = g_sequence_get_begin_iter (BOX_PRIV (box)->children);
       !g_sequence_iter_is_end (iter);
       iter = g_sequence_iter_next (iter))
    {
      child = g_sequence_get (iter);
      if (CHILD_PRIV (child)->selected)
        (*func) (box, child, data);
    }
}

/**
 * bobgui_flow_box_set_selection_mode:
 * @box: a `BobguiFlowBox`
 * @mode: the new selection mode
 *
 * Sets how selection works in @box.
 */
void
bobgui_flow_box_set_selection_mode (BobguiFlowBox       *box,
                                 BobguiSelectionMode  mode)
{
  gboolean dirty = FALSE;

  g_return_if_fail (BOBGUI_IS_FLOW_BOX (box));

  if (mode == BOX_PRIV (box)->selection_mode)
    return;

  if (mode == BOBGUI_SELECTION_NONE ||
      BOX_PRIV (box)->selection_mode == BOBGUI_SELECTION_MULTIPLE)
    {
      dirty = bobgui_flow_box_unselect_all_internal (box);
      BOX_PRIV (box)->selected_child = NULL;
    }

  BOX_PRIV (box)->selection_mode = mode;

  bobgui_accessible_update_property (BOBGUI_ACCESSIBLE (box),
                                  BOBGUI_ACCESSIBLE_PROPERTY_MULTI_SELECTABLE, mode == BOBGUI_SELECTION_MULTIPLE,
                                  -1);

  g_object_notify_by_pspec (G_OBJECT (box), props[PROP_SELECTION_MODE]);

  if (dirty)
    g_signal_emit (box, signals[SELECTED_CHILDREN_CHANGED], 0);
}

/**
 * bobgui_flow_box_get_selection_mode:
 * @box: a `BobguiFlowBox`
 *
 * Gets the selection mode of @box.
 *
 * Returns: the `BobguiSelectionMode`
 */
BobguiSelectionMode
bobgui_flow_box_get_selection_mode (BobguiFlowBox *box)
{
  g_return_val_if_fail (BOBGUI_IS_FLOW_BOX (box), BOBGUI_SELECTION_SINGLE);

  return BOX_PRIV (box)->selection_mode;
}

/* Filtering {{{2 */

/**
 * BobguiFlowBoxFilterFunc:
 * @child: a `BobguiFlowBoxChild` that may be filtered
 * @user_data: (closure): user data
 *
 * A function that will be called whenever a child changes
 * or is added.
 *
 * It lets you control if the child should be visible or not.
 *
 * Returns: %TRUE if the row should be visible, %FALSE otherwise
 */

/**
 * bobgui_flow_box_set_filter_func:
 * @box: a `BobguiFlowBox`
 * @filter_func: (nullable) (scope notified) (closure user_data) (destroy destroy): callback
 *   that lets you filter which children to show
 * @user_data: user data passed to @filter_func
 * @destroy: destroy notifier for @user_data
 *
 * By setting a filter function on the @box one can decide dynamically
 * which of the children to show.
 *
 * For instance, to implement a search function that only shows the
 * children matching the search terms.
 *
 * The @filter_func will be called for each child after the call, and
 * it will continue to be called each time a child changes (via
 * [method@Bobgui.FlowBoxChild.changed]) or when
 * [method@Bobgui.FlowBox.invalidate_filter] is called.
 *
 * Note that using a filter function is incompatible with using a model
 * (see [method@Bobgui.FlowBox.bind_model]).
 */
void
bobgui_flow_box_set_filter_func (BobguiFlowBox           *box,
                              BobguiFlowBoxFilterFunc  filter_func,
                              gpointer              user_data,
                              GDestroyNotify        destroy)
{
  BobguiFlowBoxPrivate *priv;

  g_return_if_fail (BOBGUI_IS_FLOW_BOX (box));

  priv = BOX_PRIV (box);

  if (priv->filter_destroy != NULL)
    priv->filter_destroy (priv->filter_data);

  priv->filter_func = filter_func;
  priv->filter_data = user_data;
  priv->filter_destroy = destroy;

  bobgui_flow_box_check_model_compat (box);

  bobgui_flow_box_apply_filter_all (box);
}

/**
 * bobgui_flow_box_invalidate_filter:
 * @box: a `BobguiFlowBox`
 *
 * Updates the filtering for all children.
 *
 * Call this function when the result of the filter
 * function on the @box is changed due to an external
 * factor. For instance, this would be used if the
 * filter function just looked for a specific search
 * term, and the entry with the string has changed.
 */
void
bobgui_flow_box_invalidate_filter (BobguiFlowBox *box)
{
  g_return_if_fail (BOBGUI_IS_FLOW_BOX (box));

  if (BOX_PRIV (box)->filter_func != NULL)
    bobgui_flow_box_apply_filter_all (box);
}

/* Sorting {{{2 */

/**
 * BobguiFlowBoxSortFunc:
 * @child1: the first child
 * @child2: the second child
 * @user_data: (closure): user data
 *
 * A function to compare two children to determine which
 * should come first.
 *
 * Returns: < 0 if @child1 should be before @child2, 0 if
 *   they are equal, and > 0 otherwise
 */

/**
 * bobgui_flow_box_set_sort_func:
 * @box: a `BobguiFlowBox`
 * @sort_func: (nullable) (scope notified) (closure user_data) (destroy destroy): the sort function
 * @user_data: user data passed to @sort_func
 * @destroy: destroy notifier for @user_data
 *
 * By setting a sort function on the @box, one can dynamically
 * reorder the children of the box, based on the contents of
 * the children.
 *
 * The @sort_func will be called for each child after the call,
 * and will continue to be called each time a child changes (via
 * [method@Bobgui.FlowBoxChild.changed]) and when
 * [method@Bobgui.FlowBox.invalidate_sort] is called.
 *
 * Note that using a sort function is incompatible with using a model
 * (see [method@Bobgui.FlowBox.bind_model]).
 */
void
bobgui_flow_box_set_sort_func (BobguiFlowBox         *box,
                            BobguiFlowBoxSortFunc  sort_func,
                            gpointer            user_data,
                            GDestroyNotify      destroy)
{
  BobguiFlowBoxPrivate *priv;

  g_return_if_fail (BOBGUI_IS_FLOW_BOX (box));

  priv = BOX_PRIV (box);

  if (priv->sort_destroy != NULL)
    priv->sort_destroy (priv->sort_data);

  priv->sort_func = sort_func;
  priv->sort_data = user_data;
  priv->sort_destroy = destroy;

  bobgui_flow_box_check_model_compat (box);

  bobgui_flow_box_invalidate_sort (box);
}

static int
bobgui_flow_box_sort (BobguiFlowBoxChild *a,
                   BobguiFlowBoxChild *b,
                   BobguiFlowBox      *box)
{
  BobguiFlowBoxPrivate *priv = BOX_PRIV (box);

  return priv->sort_func (a, b, priv->sort_data);
}

static void
bobgui_flow_box_reorder_foreach (gpointer data,
                              gpointer user_data)
{
  BobguiWidget **previous = user_data;
  BobguiWidget *row = data;

  if (*previous)
    bobgui_widget_insert_after (row, _bobgui_widget_get_parent (row), *previous);

  *previous = row;
}

/**
 * bobgui_flow_box_invalidate_sort:
 * @box: a `BobguiFlowBox`
 *
 * Updates the sorting for all children.
 *
 * Call this when the result of the sort function on
 * @box is changed due to an external factor.
 */
void
bobgui_flow_box_invalidate_sort (BobguiFlowBox *box)
{
  BobguiFlowBoxPrivate *priv;
  BobguiWidget *previous = NULL;

  g_return_if_fail (BOBGUI_IS_FLOW_BOX (box));

  priv = BOX_PRIV (box);

  if (priv->sort_func != NULL)
    {
      g_sequence_sort (priv->children, (GCompareDataFunc)bobgui_flow_box_sort, box);
      g_sequence_foreach (priv->children, bobgui_flow_box_reorder_foreach, &previous);
      bobgui_widget_queue_resize (BOBGUI_WIDGET (box));
    }
}

/* vim:set foldmethod=marker: */
