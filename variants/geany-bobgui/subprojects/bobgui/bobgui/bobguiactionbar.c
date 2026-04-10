/*
 * Copyright (c) 2013 - 2014 Red Hat, Inc.
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

#include "bobguiactionbar.h"
#include "bobguibuildable.h"
#include "bobguitypebuiltins.h"
#include "bobguibox.h"
#include "bobguirevealer.h"
#include "bobguiwidgetprivate.h"
#include "bobguiprivate.h"
#include "bobguicenterbox.h"
#include "bobguibinlayout.h"

#include <string.h>

/**
 * BobguiActionBar:
 *
 * Presents contextual actions.
 *
 * <picture>
 *   <source srcset="action-bar-dark.png" media="(prefers-color-scheme: dark)">
 *   <img alt="An example BobguiActionBar" src="action-bar.png">
 * </picture>
 *
 * `BobguiActionBar` is expected to be displayed below the content and expand
 * horizontally to fill the area.
 *
 * It allows placing children at the start or the end. In addition, it
 * contains an internal centered box which is centered with respect to
 * the full width of the box, even if the children at either side take
 * up different amounts of space.
 *
 * # BobguiActionBar as BobguiBuildable
 *
 * The `BobguiActionBar` implementation of the `BobguiBuildable` interface supports
 * adding children at the start or end sides by specifying “start” or “end” as
 * the “type” attribute of a `<child>` element, or setting the center widget
 * by specifying “center” value.
 *
 * # CSS nodes
 *
 * ```
 * actionbar
 * ╰── revealer
 *     ╰── box
 *         ├── box.start
 *         │   ╰── [start children]
 *         ├── [center widget]
 *         ╰── box.end
 *             ╰── [end children]
 * ```
 *
 * A `BobguiActionBar`'s CSS node is called `actionbar`. It contains a `revealer`
 * subnode, which contains a `box` subnode, which contains two `box` subnodes at
 * the start and end of the action bar, with `start` and `end` style classes
 * respectively, as well as a center node that represents the center child.
 *
 * Each of the boxes contains children packed for that side.
 */

typedef struct _BobguiActionBarClass         BobguiActionBarClass;

struct _BobguiActionBar
{
  BobguiWidget parent;

  BobguiWidget *center_box;
  BobguiWidget *start_box;
  BobguiWidget *end_box;
  BobguiWidget *revealer;
};

struct _BobguiActionBarClass
{
  BobguiWidgetClass parent_class;
};

enum {
  PROP_0,
  PROP_REVEALED,
  LAST_PROP
};
static GParamSpec *props[LAST_PROP] = { NULL, };

static void bobgui_action_bar_buildable_interface_init (BobguiBuildableIface *iface);

G_DEFINE_TYPE_WITH_CODE (BobguiActionBar, bobgui_action_bar, BOBGUI_TYPE_WIDGET,
                         G_IMPLEMENT_INTERFACE (BOBGUI_TYPE_BUILDABLE,
                                                bobgui_action_bar_buildable_interface_init))

static void
bobgui_action_bar_set_property (GObject      *object,
                             guint         prop_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
  BobguiActionBar *self = BOBGUI_ACTION_BAR (object);

  switch (prop_id)
    {
    case PROP_REVEALED:
      bobgui_action_bar_set_revealed (self, g_value_get_boolean (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bobgui_action_bar_get_property (GObject    *object,
                             guint       prop_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
  BobguiActionBar *self = BOBGUI_ACTION_BAR (object);

  switch (prop_id)
    {
    case PROP_REVEALED:
      g_value_set_boolean (value, bobgui_action_bar_get_revealed (self));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bobgui_action_bar_dispose (GObject *object)
{
  BobguiActionBar *self = BOBGUI_ACTION_BAR (object);

  g_clear_pointer (&self->revealer, bobgui_widget_unparent);

  self->center_box = NULL;
  self->start_box = NULL;
  self->end_box = NULL;

  G_OBJECT_CLASS (bobgui_action_bar_parent_class)->dispose (object);
}

static void
bobgui_action_bar_class_init (BobguiActionBarClass *klass)
{
  GObjectClass *object_class;
  BobguiWidgetClass *widget_class;

  object_class = G_OBJECT_CLASS (klass);
  widget_class = BOBGUI_WIDGET_CLASS (klass);

  object_class->set_property = bobgui_action_bar_set_property;
  object_class->get_property = bobgui_action_bar_get_property;
  object_class->dispose = bobgui_action_bar_dispose;

  widget_class->focus = bobgui_widget_focus_child;

  /**
   * BobguiActionBar:revealed:
   *
   * Controls whether the action bar shows its contents.
   */
  props[PROP_REVEALED] =
    g_param_spec_boolean ("revealed", NULL, NULL,
                          TRUE,
                          BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, LAST_PROP, props);

  bobgui_widget_class_set_layout_manager_type (widget_class, BOBGUI_TYPE_BIN_LAYOUT);
  bobgui_widget_class_set_css_name (widget_class, I_("actionbar"));
}

static void
bobgui_action_bar_init (BobguiActionBar *self)
{
  BobguiWidget *widget = BOBGUI_WIDGET (self);

  self->revealer = bobgui_revealer_new ();
  bobgui_widget_set_parent (self->revealer, widget);

  bobgui_revealer_set_reveal_child (BOBGUI_REVEALER (self->revealer), TRUE);
  bobgui_revealer_set_transition_type (BOBGUI_REVEALER (self->revealer), BOBGUI_REVEALER_TRANSITION_TYPE_SLIDE_UP);

  self->start_box = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 0);
  self->end_box = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 0);

  bobgui_widget_add_css_class (self->start_box, "start");
  bobgui_widget_add_css_class (self->end_box, "end");

  self->center_box = bobgui_center_box_new ();
  bobgui_center_box_set_start_widget (BOBGUI_CENTER_BOX (self->center_box), self->start_box);
  bobgui_center_box_set_end_widget (BOBGUI_CENTER_BOX (self->center_box), self->end_box);

  bobgui_revealer_set_child (BOBGUI_REVEALER (self->revealer), self->center_box);
}

static BobguiBuildableIface *parent_buildable_iface;

static void
bobgui_action_bar_buildable_add_child (BobguiBuildable *buildable,
                                    BobguiBuilder   *builder,
                                    GObject      *child,
                                    const char   *type)
{
  BobguiActionBar *self = BOBGUI_ACTION_BAR (buildable);

  if (g_strcmp0 (type, "start") == 0)
    bobgui_action_bar_pack_start (self, BOBGUI_WIDGET (child));
  else if (g_strcmp0 (type, "center") == 0)
    bobgui_action_bar_set_center_widget (self, BOBGUI_WIDGET (child));
  else if (g_strcmp0 (type, "end") == 0)
    bobgui_action_bar_pack_end (self, BOBGUI_WIDGET (child));
  else if (type == NULL && BOBGUI_IS_WIDGET (child))
    bobgui_action_bar_pack_start (self, BOBGUI_WIDGET (child));
  else
    parent_buildable_iface->add_child (buildable, builder, child, type);
}

static void
bobgui_action_bar_buildable_interface_init (BobguiBuildableIface *iface)
{
  parent_buildable_iface = g_type_interface_peek_parent (iface);
  iface->add_child = bobgui_action_bar_buildable_add_child;
}

/**
 * bobgui_action_bar_pack_start:
 * @action_bar: an action bar
 * @child: the widget to be added
 *
 * Adds a child to the action, packed with reference to the
 * start of the action bar.
 */
void
bobgui_action_bar_pack_start (BobguiActionBar *action_bar,
                           BobguiWidget    *child)
{
  bobgui_box_append (BOBGUI_BOX (action_bar->start_box), child);
}

/**
 * bobgui_action_bar_pack_end:
 * @action_bar: an action bar
 * @child: the widget to be added
 *
 * Adds a child to the action bar, packed with reference to the
 * end of the action bar.
 */
void
bobgui_action_bar_pack_end (BobguiActionBar *action_bar,
                         BobguiWidget    *child)
{
  bobgui_box_insert_child_after (BOBGUI_BOX (action_bar->end_box), child, NULL);
}

/**
 * bobgui_action_bar_remove:
 * @action_bar: an action bar
 * @child: the widget to be removed
 *
 * Removes a child from the action bar.
 */
void
bobgui_action_bar_remove (BobguiActionBar *action_bar,
                       BobguiWidget    *child)
{
  if (bobgui_widget_get_parent (child) == action_bar->start_box)
    bobgui_box_remove (BOBGUI_BOX (action_bar->start_box), child);
  else if (bobgui_widget_get_parent (child) == action_bar->end_box)
    bobgui_box_remove (BOBGUI_BOX (action_bar->end_box), child);
  else if (child == bobgui_center_box_get_center_widget (BOBGUI_CENTER_BOX (action_bar->center_box)))
    bobgui_center_box_set_center_widget (BOBGUI_CENTER_BOX (action_bar->center_box), NULL);
  else
    g_warning ("Can't remove non-child %s %p from BobguiActionBar %p",
               G_OBJECT_TYPE_NAME (child), child, action_bar);
}

/**
 * bobgui_action_bar_set_center_widget:
 * @action_bar: an action bar
 * @center_widget: (nullable): a widget to use for the center
 *
 * Sets the center widget for the action bar.
 */
void
bobgui_action_bar_set_center_widget (BobguiActionBar *action_bar,
                                  BobguiWidget    *center_widget)
{
  bobgui_center_box_set_center_widget (BOBGUI_CENTER_BOX (action_bar->center_box), center_widget);
}

/**
 * bobgui_action_bar_get_center_widget:
 * @action_bar: an action bsar
 *
 * Retrieves the center bar widget of the bar.
 *
 * Returns: (transfer none) (nullable): the center widget
 */
BobguiWidget *
bobgui_action_bar_get_center_widget (BobguiActionBar *action_bar)
{
  g_return_val_if_fail (BOBGUI_IS_ACTION_BAR (action_bar), NULL);

  return bobgui_center_box_get_center_widget (BOBGUI_CENTER_BOX (action_bar->center_box));
}

/**
 * bobgui_action_bar_new:
 *
 * Creates a new action bar widget.
 *
 * Returns: a new `BobguiActionBar`
 */
BobguiWidget *
bobgui_action_bar_new (void)
{
  return BOBGUI_WIDGET (g_object_new (BOBGUI_TYPE_ACTION_BAR, NULL));
}

/**
 * bobgui_action_bar_set_revealed:
 * @action_bar: an action bar
 * @revealed: the new value for the property
 *
 * Reveals or conceals the content of the action bar.
 *
 * Note: this does not show or hide the action bar in the
 * [property@Bobgui.Widget:visible] sense, so revealing has
 * no effect if the action bar is hidden.
 */
void
bobgui_action_bar_set_revealed (BobguiActionBar *action_bar,
                             gboolean      revealed)
{
  g_return_if_fail (BOBGUI_IS_ACTION_BAR (action_bar));

  if (revealed == bobgui_revealer_get_reveal_child (BOBGUI_REVEALER (action_bar->revealer)))
    return;

  bobgui_revealer_set_reveal_child (BOBGUI_REVEALER (action_bar->revealer), revealed);
  g_object_notify_by_pspec (G_OBJECT (action_bar), props[PROP_REVEALED]);
}

/**
 * bobgui_action_bar_get_revealed:
 * @action_bar: an action bar
 *
 * Gets whether the contents of the action bar are revealed.
 *
 * Returns: the current value of the [property@Bobgui.ActionBar:revealed]
 *   property
 */
gboolean
bobgui_action_bar_get_revealed (BobguiActionBar *action_bar)
{
  g_return_val_if_fail (BOBGUI_IS_ACTION_BAR (action_bar), FALSE);

  return bobgui_revealer_get_reveal_child (BOBGUI_REVEALER (action_bar->revealer));
}
