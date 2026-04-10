/* bobguilayoutmanager.c: Layout manager base class
 * Copyright 2018  The GNOME Foundation
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Emmanuele Bassi
 */

/**
 * BobguiLayoutManager:
 *
 * Handles the preferred size and allocation for children of a widget.
 *
 * You typically subclass `BobguiLayoutManager` if you want to implement a
 * layout policy for the children of a widget, or if you want to determine
 * the size of a widget depending on its contents.
 *
 * Each `BobguiWidget` can only have a `BobguiLayoutManager` instance associated
 * to it at any given time; it is possible, though, to replace the layout
 * manager instance using [method@Bobgui.Widget.set_layout_manager].
 *
 * ## Layout properties
 *
 * A layout manager can expose properties for controlling the layout of
 * each child, by creating an object type derived from [class@Bobgui.LayoutChild]
 * and installing the properties on it as normal `GObject` properties.
 *
 * Each `BobguiLayoutChild` instance storing the layout properties for a
 * specific child is created through the [method@Bobgui.LayoutManager.get_layout_child]
 * method; a `BobguiLayoutManager` controls the creation of its `BobguiLayoutChild`
 * instances by overriding the BobguiLayoutManagerClass.create_layout_child()
 * virtual function. The typical implementation should look like:
 *
 * ```c
 * static BobguiLayoutChild *
 * create_layout_child (BobguiLayoutManager *manager,
 *                      BobguiWidget        *container,
 *                      BobguiWidget        *child)
 * {
 *   return g_object_new (your_layout_child_get_type (),
 *                        "layout-manager", manager,
 *                        "child-widget", child,
 *                        NULL);
 * }
 * ```
 *
 * The [property@Bobgui.LayoutChild:layout-manager] and
 * [property@Bobgui.LayoutChild:child-widget] properties
 * on the newly created `BobguiLayoutChild` instance are mandatory. The
 * `BobguiLayoutManager` will cache the newly created `BobguiLayoutChild` instance
 * until the widget is removed from its parent, or the parent removes the
 * layout manager.
 *
 * Each `BobguiLayoutManager` instance creating a `BobguiLayoutChild` should use
 * [method@Bobgui.LayoutManager.get_layout_child] every time it needs to query
 * the layout properties; each `BobguiLayoutChild` instance should call
 * [method@Bobgui.LayoutManager.layout_changed] every time a property is
 * updated, in order to queue a new size measuring and allocation.
 */

#include "config.h"

#include "bobguilayoutmanagerprivate.h"
#include "bobguilayoutchild.h"
#include "bobguiwidgetprivate.h"
#include "bobguinative.h"
#include "bobguipopover.h"
#include "bobguitexthandleprivate.h"
#include "bobguitooltipwindowprivate.h"

#ifdef G_ENABLE_DEBUG
#define LAYOUT_MANAGER_WARN_NOT_IMPLEMENTED(m,method)   G_STMT_START {  \
        GObject *_obj = G_OBJECT (m);                                   \
        g_warning ("Layout managers of type %s do not implement "       \
                   "the BobguiLayoutManager::%s method",                   \
                   G_OBJECT_TYPE_NAME (_obj),                           \
                   #method);                           } G_STMT_END
#else
#define LAYOUT_MANAGER_WARN_NOT_IMPLEMENTED(m,method)
#endif

typedef struct {
  BobguiWidget *widget;
  BobguiRoot *root;

  /* HashTable<Widget, LayoutChild> */
  GHashTable *layout_children;
} BobguiLayoutManagerPrivate;

G_DEFINE_ABSTRACT_TYPE_WITH_PRIVATE (BobguiLayoutManager, bobgui_layout_manager, G_TYPE_OBJECT)

static void
bobgui_layout_manager_real_root (BobguiLayoutManager *manager)
{
}

static void
bobgui_layout_manager_real_unroot (BobguiLayoutManager *manager)
{
}

static BobguiSizeRequestMode
bobgui_layout_manager_real_get_request_mode (BobguiLayoutManager *manager,
                                          BobguiWidget        *widget)
{
  int hfw = 0, wfh = 0;
  BobguiWidget *child;

  for (child = _bobgui_widget_get_first_child (widget);
       child != NULL;
       child = _bobgui_widget_get_next_sibling (child))
    {
      BobguiSizeRequestMode res;

      if (!bobgui_widget_should_layout (child))
        continue;

      res = bobgui_widget_get_request_mode (child);

      switch (res)
        {
        case BOBGUI_SIZE_REQUEST_HEIGHT_FOR_WIDTH:
          hfw += 1;
          break;

        case BOBGUI_SIZE_REQUEST_WIDTH_FOR_HEIGHT:
          wfh += 1;
          break;

        case BOBGUI_SIZE_REQUEST_CONSTANT_SIZE:
        default:
          break;
        }
    }

 if (hfw == 0 && wfh == 0)
   return BOBGUI_SIZE_REQUEST_CONSTANT_SIZE;

 return hfw >= wfh ? BOBGUI_SIZE_REQUEST_HEIGHT_FOR_WIDTH
                   : BOBGUI_SIZE_REQUEST_WIDTH_FOR_HEIGHT;
}

static void
bobgui_layout_manager_real_measure (BobguiLayoutManager *manager,
                                 BobguiWidget        *widget,
                                 BobguiOrientation    orientation,
                                 int               for_size,
                                 int              *minimum,
                                 int              *natural,
                                 int              *baseline_minimum,
                                 int              *baseline_natural)
{
  LAYOUT_MANAGER_WARN_NOT_IMPLEMENTED (manager, measure);

  if (minimum != NULL)
    *minimum = 0;

  if (natural != NULL)
    *natural = 0;

  if (baseline_minimum != NULL)
    *baseline_minimum = 0;

  if (baseline_natural != NULL)
    *baseline_natural = 0;
}

static void
bobgui_layout_manager_real_allocate (BobguiLayoutManager *manager,
                                  BobguiWidget        *widget,
                                  int               width,
                                  int               height,
                                  int               baseline)
{
  LAYOUT_MANAGER_WARN_NOT_IMPLEMENTED (manager, allocate);
}

static BobguiLayoutChild *
bobgui_layout_manager_real_create_layout_child (BobguiLayoutManager *manager,
                                             BobguiWidget        *widget,
                                             BobguiWidget        *child)
{
  BobguiLayoutManagerClass *manager_class = BOBGUI_LAYOUT_MANAGER_GET_CLASS (manager);

  if (manager_class->layout_child_type == G_TYPE_INVALID)
    {
      LAYOUT_MANAGER_WARN_NOT_IMPLEMENTED (manager, create_layout_child);
      return NULL;
    }

  return g_object_new (manager_class->layout_child_type,
                       "layout-manager", manager,
                       "child-widget", child,
                       NULL);
}

static void
bobgui_layout_manager_finalize (GObject *gobject)
{
  BobguiLayoutManager *self = BOBGUI_LAYOUT_MANAGER (gobject);
  BobguiLayoutManagerPrivate *priv = bobgui_layout_manager_get_instance_private (self);

  g_clear_pointer (&priv->layout_children, g_hash_table_unref);

  G_OBJECT_CLASS (bobgui_layout_manager_parent_class)->finalize (gobject);
}

static void
bobgui_layout_manager_class_init (BobguiLayoutManagerClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->finalize = bobgui_layout_manager_finalize;

  klass->get_request_mode = bobgui_layout_manager_real_get_request_mode;
  klass->measure = bobgui_layout_manager_real_measure;
  klass->allocate = bobgui_layout_manager_real_allocate;
  klass->create_layout_child = bobgui_layout_manager_real_create_layout_child;
  klass->root = bobgui_layout_manager_real_root;
  klass->unroot = bobgui_layout_manager_real_unroot;
}

static void
bobgui_layout_manager_init (BobguiLayoutManager *self)
{
}

/*< private >
 * bobgui_layout_manager_set_widget:
 * @layout_manager: a `BobguiLayoutManager`
 * @widget: (nullable): a `BobguiWidget`
 *
 * Sets a back pointer from @widget to @layout_manager.
 */
void
bobgui_layout_manager_set_widget (BobguiLayoutManager *layout_manager,
                               BobguiWidget        *widget)
{
  BobguiLayoutManagerPrivate *priv = bobgui_layout_manager_get_instance_private (layout_manager);

  if (widget != NULL && priv->widget != NULL)
    {
      g_critical ("The layout manager %p of type %s is already in use "
                  "by widget %p '%s', and cannot be used by widget %p '%s'",
                  layout_manager, G_OBJECT_TYPE_NAME (layout_manager),
                  priv->widget, bobgui_widget_get_name (priv->widget),
                  widget, bobgui_widget_get_name (widget));
      return;
    }

  priv->widget = widget;

  if (widget != NULL)
    bobgui_layout_manager_set_root (layout_manager, bobgui_widget_get_root (widget));
}

/*< private >
 * bobgui_layout_manager_set_root:
 * @layout_manager: a i`BobguiLayoutManager`
 * @root: (nullable): a `BobguiWidget` implementing `BobguiRoot`
 *
 * Sets a back pointer from @root to @layout_manager.
 *
 * This function is called by `BobguiWidget` when getting rooted and unrooted,
 * and will call `BobguiLayoutManagerClass.root()` or `BobguiLayoutManagerClass.unroot()`
 * depending on whether @root is a `BobguiWidget` or %NULL.
 */
void
bobgui_layout_manager_set_root (BobguiLayoutManager *layout_manager,
                             BobguiRoot          *root)
{
  BobguiLayoutManagerPrivate *priv = bobgui_layout_manager_get_instance_private (layout_manager);
  BobguiRoot *old_root = priv->root;

  priv->root = root;

  if (old_root != root)
    {
      if (priv->root != NULL)
        BOBGUI_LAYOUT_MANAGER_GET_CLASS (layout_manager)->root (layout_manager);
      else
        BOBGUI_LAYOUT_MANAGER_GET_CLASS (layout_manager)->unroot (layout_manager);
    }
}

/**
 * bobgui_layout_manager_measure:
 * @manager: a `BobguiLayoutManager`
 * @widget: the `BobguiWidget` using @manager
 * @orientation: the orientation to measure
 * @for_size: Size for the opposite of @orientation; for instance, if
 *   the @orientation is %BOBGUI_ORIENTATION_HORIZONTAL, this is the height
 *   of the widget; if the @orientation is %BOBGUI_ORIENTATION_VERTICAL, this
 *   is the width of the widget. This allows to measure the height for the
 *   given width, and the width for the given height. Use -1 if the size
 *   is not known
 * @minimum: (out) (optional): the minimum size for the given size and
 *   orientation
 * @natural: (out) (optional): the natural, or preferred size for the
 *   given size and orientation
 * @minimum_baseline: (out) (optional): the baseline position for the
 *   minimum size
 * @natural_baseline: (out) (optional): the baseline position for the
 *   natural size
 *
 * Measures the size of the @widget using @manager, for the
 * given @orientation and size.
 *
 * See the [class@Bobgui.Widget] documentation on layout management for
 * more details.
 */
void
bobgui_layout_manager_measure (BobguiLayoutManager *manager,
                            BobguiWidget        *widget,
                            BobguiOrientation    orientation,
                            int               for_size,
                            int              *minimum,
                            int              *natural,
                            int              *minimum_baseline,
                            int              *natural_baseline)
{
  BobguiLayoutManagerClass *klass;
  int min_size = 0;
  int nat_size = 0;
  int min_baseline = -1;
  int nat_baseline = -1;


  g_return_if_fail (BOBGUI_IS_LAYOUT_MANAGER (manager));
  g_return_if_fail (BOBGUI_IS_WIDGET (widget));

  klass = BOBGUI_LAYOUT_MANAGER_GET_CLASS (manager);

  klass->measure (manager, widget, orientation,
                  for_size,
                  &min_size, &nat_size,
                  &min_baseline, &nat_baseline);

  if (minimum)
    *minimum = min_size;

  if (natural)
    *natural = nat_size;

  if (minimum_baseline)
    *minimum_baseline = min_baseline;

  if (natural_baseline)
    *natural_baseline = nat_baseline;
}

static void
allocate_native_children (BobguiWidget *widget)
{
  BobguiWidget *child;

  for (child = _bobgui_widget_get_first_child (widget);
       child != NULL;
       child = _bobgui_widget_get_next_sibling (child))
    {
      if (BOBGUI_IS_POPOVER (child))
        bobgui_popover_present (BOBGUI_POPOVER (child));
      else if (BOBGUI_IS_TEXT_HANDLE (child))
        bobgui_text_handle_present (BOBGUI_TEXT_HANDLE (child));
      else if (BOBGUI_IS_TOOLTIP_WINDOW (child))
        bobgui_tooltip_window_present (BOBGUI_TOOLTIP_WINDOW (child));
      else if (BOBGUI_IS_NATIVE (child))
        g_warning ("Unable to present a to the layout manager unknown auxiliary child surface widget type %s",
                   G_OBJECT_TYPE_NAME (child));
    }
}

/**
 * bobgui_layout_manager_allocate:
 * @manager: a `BobguiLayoutManager`
 * @widget: the `BobguiWidget` using @manager
 * @width: the new width of the @widget
 * @height: the new height of the @widget
 * @baseline: the baseline position of the @widget, or -1
 *
 * Assigns the given @width, @height, and @baseline to
 * a @widget, and computes the position and sizes of the children of
 * the @widget using the layout management policy of @manager.
 */
void
bobgui_layout_manager_allocate (BobguiLayoutManager *manager,
                             BobguiWidget        *widget,
                             int               width,
                             int               height,
                             int               baseline)
{
  BobguiLayoutManagerClass *klass;

  g_return_if_fail (BOBGUI_IS_LAYOUT_MANAGER (manager));
  g_return_if_fail (BOBGUI_IS_WIDGET (widget));
  g_return_if_fail (baseline >= -1);

  allocate_native_children (widget);

  klass = BOBGUI_LAYOUT_MANAGER_GET_CLASS (manager);

  klass->allocate (manager, widget, width, height, baseline);
}

/**
 * bobgui_layout_manager_get_request_mode:
 * @manager: a `BobguiLayoutManager`
 *
 * Retrieves the request mode of @manager.
 *
 * Returns: a `BobguiSizeRequestMode`
 */
BobguiSizeRequestMode
bobgui_layout_manager_get_request_mode (BobguiLayoutManager *manager)
{
  BobguiLayoutManagerPrivate *priv = bobgui_layout_manager_get_instance_private (manager);
  BobguiLayoutManagerClass *klass;

  g_return_val_if_fail (BOBGUI_IS_LAYOUT_MANAGER (manager), BOBGUI_SIZE_REQUEST_CONSTANT_SIZE);
  g_return_val_if_fail (priv->widget != NULL, BOBGUI_SIZE_REQUEST_CONSTANT_SIZE);

  klass = BOBGUI_LAYOUT_MANAGER_GET_CLASS (manager);

  return klass->get_request_mode (manager, priv->widget);
}

/**
 * bobgui_layout_manager_get_widget:
 * @manager: a `BobguiLayoutManager`
 *
 * Retrieves the `BobguiWidget` using the given `BobguiLayoutManager`.
 *
 * Returns: (transfer none) (nullable): a `BobguiWidget`
 */
BobguiWidget *
bobgui_layout_manager_get_widget (BobguiLayoutManager *manager)
{
  BobguiLayoutManagerPrivate *priv = bobgui_layout_manager_get_instance_private (manager);

  g_return_val_if_fail (BOBGUI_IS_LAYOUT_MANAGER (manager), NULL);

  return priv->widget;
}

/**
 * bobgui_layout_manager_layout_changed:
 * @manager: a `BobguiLayoutManager`
 *
 * Queues a resize on the `BobguiWidget` using @manager, if any.
 *
 * This function should be called by subclasses of `BobguiLayoutManager`
 * in response to changes to their layout management policies.
 */
void
bobgui_layout_manager_layout_changed (BobguiLayoutManager *manager)
{
  BobguiLayoutManagerPrivate *priv = bobgui_layout_manager_get_instance_private (manager);

  g_return_if_fail (BOBGUI_IS_LAYOUT_MANAGER (manager));

  if (priv->widget != NULL)
    bobgui_widget_queue_resize (priv->widget);
}

/*< private >
 * bobgui_layout_manager_remove_layout_child:
 * @manager: a `BobguiLayoutManager`
 * @widget: a `BobguiWidget`
 *
 * Removes the `BobguiLayoutChild` associated with @widget from the
 * given `BobguiLayoutManager`, if any is set.
 */
void
bobgui_layout_manager_remove_layout_child (BobguiLayoutManager *manager,
                                        BobguiWidget        *widget)
{
  BobguiLayoutManagerPrivate *priv = bobgui_layout_manager_get_instance_private (manager);

  if (priv->layout_children != NULL)
    {
      g_hash_table_remove (priv->layout_children, widget);
      if (g_hash_table_size (priv->layout_children) == 0)
        g_clear_pointer (&priv->layout_children, g_hash_table_unref);
    }
}

/**
 * bobgui_layout_manager_get_layout_child:
 * @manager: a `BobguiLayoutManager`
 * @child: a `BobguiWidget`
 *
 * Retrieves a `BobguiLayoutChild` instance for the `BobguiLayoutManager`,
 * creating one if necessary.
 *
 * The @child widget must be a child of the widget using @manager.
 *
 * The `BobguiLayoutChild` instance is owned by the `BobguiLayoutManager`,
 * and is guaranteed to exist as long as @child is a child of the
 * `BobguiWidget` using the given `BobguiLayoutManager`.
 *
 * Returns: (transfer none): a `BobguiLayoutChild`
 */
BobguiLayoutChild *
bobgui_layout_manager_get_layout_child (BobguiLayoutManager *manager,
                                     BobguiWidget        *child)
{
  BobguiLayoutManagerPrivate *priv = bobgui_layout_manager_get_instance_private (manager);
  BobguiLayoutChild *res;
  BobguiWidget *parent;

  g_return_val_if_fail (BOBGUI_IS_LAYOUT_MANAGER (manager), NULL);
  g_return_val_if_fail (BOBGUI_IS_WIDGET (child), NULL);

  parent = _bobgui_widget_get_parent (child);
  g_return_val_if_fail (parent != NULL, NULL);

  if (priv->widget != parent)
    {
      g_critical ("The parent %s %p of the widget %s %p does not "
                  "use the given layout manager of type %s %p",
                  bobgui_widget_get_name (parent), parent,
                  bobgui_widget_get_name (child), child,
                  G_OBJECT_TYPE_NAME (manager), manager);
      return NULL;
    }

  if (priv->layout_children == NULL)
    {
      priv->layout_children = g_hash_table_new_full (NULL, NULL,
                                                     NULL,
                                                     (GDestroyNotify) g_object_unref);
    }

  res = g_hash_table_lookup (priv->layout_children, child);
  if (res != NULL)
    {
      /* If the LayoutChild instance is stale, and refers to another
       * layout manager, then we simply ask the LayoutManager to
       * replace it, as it means the layout manager for the parent
       * widget was replaced
       */
      if (bobgui_layout_child_get_layout_manager (res) == manager)
        return res;
    }

  res = BOBGUI_LAYOUT_MANAGER_GET_CLASS (manager)->create_layout_child (manager, parent, child);
  if (res == NULL)
    {
      g_critical ("The layout manager of type %s %p does not create "
                  "BobguiLayoutChild instances",
                  G_OBJECT_TYPE_NAME (manager), manager);
      return NULL;
    }

  g_assert (g_type_is_a (G_OBJECT_TYPE (res), BOBGUI_TYPE_LAYOUT_CHILD));
  g_hash_table_insert (priv->layout_children, child, res);

  return res;
}
