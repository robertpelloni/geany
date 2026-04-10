/*
 * bobguioverlay.c
 * This file is part of bobgui
 *
 * Copyright (C) 2011 - Ignacio Casal Quinteiro, Mike Krüger
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

#include "bobguioverlay.h"

#include "bobguioverlaylayout.h"
#include "bobguibuildable.h"
#include "bobguibuilderprivate.h"
#include "bobguimarshalers.h"
#include "bobguiprivate.h"
#include "bobguiscrolledwindow.h"
#include "bobguisnapshot.h"
#include "bobguiwidgetprivate.h"

/**
 * BobguiOverlay
 *
 * Places “overlay” widgets on top of a single main child.
 *
 * <picture>
 *   <source srcset="overlay-dark.png" media="(prefers-color-scheme: dark)">
 *   <img alt="An example BobguiOverlay" src="overlay.png">
 * </picture>
 *
 * The position of each overlay widget is determined by its
 * [property@Bobgui.Widget:halign] and [property@Bobgui.Widget:valign]
 * properties. E.g. a widget with both alignments set to %BOBGUI_ALIGN_START
 * will be placed at the top left corner of the `BobguiOverlay` container,
 * whereas an overlay with halign set to %BOBGUI_ALIGN_CENTER and valign set
 * to %BOBGUI_ALIGN_END will be placed a the bottom edge of the `BobguiOverlay`,
 * horizontally centered. The position can be adjusted by setting the margin
 * properties of the child to non-zero values.
 *
 * More complicated placement of overlays is possible by connecting
 * to the [signal@Bobgui.Overlay::get-child-position] signal.
 *
 * An overlay’s minimum and natural sizes are those of its main child.
 * The sizes of overlay children are not considered when measuring these
 * preferred sizes.
 *
 * # BobguiOverlay as BobguiBuildable
 *
 * The `BobguiOverlay` implementation of the `BobguiBuildable` interface
 * supports placing a child as an overlay by specifying “overlay” as
 * the “type” attribute of a `<child>` element.
 *
 * # CSS nodes
 *
 * `BobguiOverlay` has a single CSS node with the name “overlay”. Overlay children
 * whose alignments cause them to be positioned at an edge get the style classes
 * “.left”, “.right”, “.top”, and/or “.bottom” according to their position.
 */

enum {
  GET_CHILD_POSITION,
  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

enum {
  PROP_CHILD = 1
};

static void bobgui_overlay_buildable_init (BobguiBuildableIface *iface);

typedef struct _BobguiOverlayClass BobguiOverlayClass;

struct _BobguiOverlay
{
  BobguiWidget parent_instance;

  BobguiWidget *child;
};

struct _BobguiOverlayClass
{
  BobguiWidgetClass parent_class;

  gboolean (*get_child_position) (BobguiOverlay    *overlay,
                                  BobguiWidget     *widget,
                                  BobguiAllocation *allocation);
};

G_DEFINE_TYPE_WITH_CODE (BobguiOverlay, bobgui_overlay, BOBGUI_TYPE_WIDGET,
                         G_IMPLEMENT_INTERFACE (BOBGUI_TYPE_BUILDABLE,
                                                bobgui_overlay_buildable_init))

static BobguiAlign
effective_align (BobguiAlign         align,
                 BobguiTextDirection direction)
{
  switch (align)
    {
    case BOBGUI_ALIGN_START:
      return direction == BOBGUI_TEXT_DIR_RTL ? BOBGUI_ALIGN_END : BOBGUI_ALIGN_START;
    case BOBGUI_ALIGN_END:
      return direction == BOBGUI_TEXT_DIR_RTL ? BOBGUI_ALIGN_START : BOBGUI_ALIGN_END;
    case BOBGUI_ALIGN_FILL:
    case BOBGUI_ALIGN_CENTER:
    case BOBGUI_ALIGN_BASELINE_FILL:
    case BOBGUI_ALIGN_BASELINE_CENTER:
    default:
      return align;
    }
}

static gboolean
bobgui_overlay_get_child_position (BobguiOverlay    *overlay,
                                BobguiWidget     *widget,
                                BobguiAllocation *alloc)
{
  BobguiRequisition min, req;
  BobguiAlign halign;
  BobguiTextDirection direction;
  int width, height;

  bobgui_widget_get_preferred_size (widget, &min, &req);
  width = bobgui_widget_get_width (BOBGUI_WIDGET (overlay));
  height = bobgui_widget_get_height (BOBGUI_WIDGET (overlay));

  alloc->x = 0;
  alloc->width = MAX (min.width, MIN (width, req.width));

  direction = _bobgui_widget_get_direction (widget);

  halign = bobgui_widget_get_halign (widget);
  switch (effective_align (halign, direction))
    {
    case BOBGUI_ALIGN_START:
      /* nothing to do */
      break;
    case BOBGUI_ALIGN_FILL:
      alloc->width = MAX (alloc->width, width);
      break;
    case BOBGUI_ALIGN_CENTER:
      alloc->x += width / 2 - alloc->width / 2;
      break;
    case BOBGUI_ALIGN_END:
      alloc->x += width - alloc->width;
      break;
    case BOBGUI_ALIGN_BASELINE_FILL:
    case BOBGUI_ALIGN_BASELINE_CENTER:
    default:
      g_assert_not_reached ();
      break;
    }

  alloc->y = 0;
  alloc->height = MAX  (min.height, MIN (height, req.height));

  switch (bobgui_widget_get_valign (widget))
    {
    case BOBGUI_ALIGN_START:
      /* nothing to do */
      break;
    case BOBGUI_ALIGN_FILL:
      alloc->height = MAX (alloc->height, height);
      break;
    case BOBGUI_ALIGN_CENTER:
      alloc->y += height / 2 - alloc->height / 2;
      break;
    case BOBGUI_ALIGN_END:
      alloc->y += height - alloc->height;
      break;
    case BOBGUI_ALIGN_BASELINE_FILL:
    case BOBGUI_ALIGN_BASELINE_CENTER:
    default:
      g_assert_not_reached ();
      break;
    }

  return TRUE;
}

static void
bobgui_overlay_snapshot_child (BobguiWidget   *overlay,
                            BobguiWidget   *child,
                            BobguiSnapshot *snapshot)
{
  graphene_rect_t bounds;
  gboolean clip_set;

  clip_set = bobgui_overlay_get_clip_overlay (BOBGUI_OVERLAY (overlay), child);

  if (!clip_set)
    {
      bobgui_widget_snapshot_child (overlay, child, snapshot);
      return;
    }

  graphene_rect_init (&bounds, 0, 0,
                      bobgui_widget_get_width (overlay),
                      bobgui_widget_get_height (overlay));

  bobgui_snapshot_push_clip (snapshot, &bounds);
  bobgui_widget_snapshot_child (overlay, child, snapshot);
  bobgui_snapshot_pop (snapshot);
}

static void
bobgui_overlay_snapshot (BobguiWidget   *widget,
                      BobguiSnapshot *snapshot)
{
  BobguiWidget *child;

  for (child = _bobgui_widget_get_first_child (widget);
       child != NULL;
       child = _bobgui_widget_get_next_sibling (child))
    {
      bobgui_overlay_snapshot_child (widget, child, snapshot);
    }
}

static void
bobgui_overlay_get_property (GObject    *object,
                          guint       prop_id,
                          GValue     *value,
                          GParamSpec *pspec)
{
  BobguiOverlay *overlay = BOBGUI_OVERLAY (object);

  switch (prop_id)
    {
    case PROP_CHILD:
      g_value_set_object (value, bobgui_overlay_get_child (overlay));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bobgui_overlay_set_property (GObject      *object,
                          guint         prop_id,
                          const GValue *value,
                          GParamSpec   *pspec)
{
  BobguiOverlay *overlay = BOBGUI_OVERLAY (object);

  switch (prop_id)
    {
    case PROP_CHILD:
      bobgui_overlay_set_child (overlay, g_value_get_object (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bobgui_overlay_dispose (GObject *object)
{
  BobguiOverlay *overlay = BOBGUI_OVERLAY (object);
  BobguiWidget *child;

  g_clear_pointer (&overlay->child, bobgui_widget_unparent);

  while ((child = bobgui_widget_get_first_child (BOBGUI_WIDGET (overlay))))
    bobgui_widget_unparent (child);

  G_OBJECT_CLASS (bobgui_overlay_parent_class)->dispose (object);
}

static void
bobgui_overlay_compute_expand (BobguiWidget *widget,
                            gboolean  *hexpand,
                            gboolean  *vexpand)
{
  BobguiOverlay *overlay = BOBGUI_OVERLAY (widget);

  if (overlay->child)
    {
      *hexpand = bobgui_widget_compute_expand (overlay->child, BOBGUI_ORIENTATION_HORIZONTAL);
      *vexpand = bobgui_widget_compute_expand (overlay->child, BOBGUI_ORIENTATION_VERTICAL);
    }
  else
    {
      *hexpand = FALSE;
      *vexpand = FALSE;
    }
}

static void
bobgui_overlay_class_init (BobguiOverlayClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (klass);

  object_class->dispose = bobgui_overlay_dispose;

  object_class->get_property = bobgui_overlay_get_property;
  object_class->set_property = bobgui_overlay_set_property;

  widget_class->snapshot = bobgui_overlay_snapshot;
  widget_class->compute_expand = bobgui_overlay_compute_expand;

  klass->get_child_position = bobgui_overlay_get_child_position;

  /**
   * BobguiOverlay:child:
   *
   * The main child widget.
   */
  g_object_class_install_property (object_class,
                                   PROP_CHILD,
                                   g_param_spec_object ("child", NULL, NULL,
                                                        BOBGUI_TYPE_WIDGET,
                                                        BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

  /**
   * BobguiOverlay::get-child-position:
   * @overlay: the `BobguiOverlay`
   * @widget: the child widget to position
   * @allocation: (type Gdk.Rectangle) (out caller-allocates): return
   *   location for the allocation
   *
   * Emitted to determine the position and size of any overlay
   * child widgets.
   *
   * A handler for this signal should fill @allocation with
   * the desired position and size for @widget, relative to
   * the 'main' child of @overlay.
   *
   * The default handler for this signal uses the @widget's
   * halign and valign properties to determine the position
   * and gives the widget its natural size (except that an
   * alignment of %BOBGUI_ALIGN_FILL will cause the overlay to
   * be full-width/height). If the main child is a
   * `BobguiScrolledWindow`, the overlays are placed relative
   * to its contents.
   *
   * Returns: %TRUE if the @allocation has been filled
   */
  signals[GET_CHILD_POSITION] =
    g_signal_new (I_("get-child-position"),
                  G_TYPE_FROM_CLASS (object_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (BobguiOverlayClass, get_child_position),
                  _bobgui_boolean_handled_accumulator, NULL,
                  _bobgui_marshal_BOOLEAN__OBJECT_BOXED,
                  G_TYPE_BOOLEAN, 2,
                  BOBGUI_TYPE_WIDGET,
                  GDK_TYPE_RECTANGLE | G_SIGNAL_TYPE_STATIC_SCOPE);
  g_signal_set_va_marshaller (signals[GET_CHILD_POSITION],
                              G_TYPE_FROM_CLASS (object_class),
                              _bobgui_marshal_BOOLEAN__OBJECT_BOXEDv);

  bobgui_widget_class_set_css_name (widget_class, I_("overlay"));

  bobgui_widget_class_set_layout_manager_type (widget_class, BOBGUI_TYPE_OVERLAY_LAYOUT);
}

static void
bobgui_overlay_init (BobguiOverlay *overlay)
{
}

static BobguiBuildableIface *parent_buildable_iface;

static void
bobgui_overlay_buildable_add_child (BobguiBuildable *buildable,
                                 BobguiBuilder   *builder,
                                 GObject      *child,
                                 const char   *type)
{
  if (BOBGUI_IS_WIDGET (child))
    {
      if (type && strcmp (type, "overlay") == 0)
        {
          bobgui_overlay_add_overlay (BOBGUI_OVERLAY (buildable), BOBGUI_WIDGET (child));
        }
      else if (!type)
        {
          bobgui_buildable_child_deprecation_warning (buildable, builder, NULL, "child");
          bobgui_overlay_set_child (BOBGUI_OVERLAY (buildable), BOBGUI_WIDGET (child));
        }
      else
        {
          BOBGUI_BUILDER_WARN_INVALID_CHILD_TYPE (buildable, type);
        }
    }
  else
    {
      parent_buildable_iface->add_child (buildable, builder, child, type);
    }
}

static void
bobgui_overlay_buildable_init (BobguiBuildableIface *iface)
{
  parent_buildable_iface = g_type_interface_peek_parent (iface);

  iface->add_child = bobgui_overlay_buildable_add_child;
}

/**
 * bobgui_overlay_new:
 *
 * Creates a new `BobguiOverlay`.
 *
 * Returns: a new `BobguiOverlay` object.
 */
BobguiWidget *
bobgui_overlay_new (void)
{
  return g_object_new (BOBGUI_TYPE_OVERLAY, NULL);
}

/**
 * bobgui_overlay_add_overlay:
 * @overlay: a `BobguiOverlay`
 * @widget: a `BobguiWidget` to be added to the container
 *
 * Adds @widget to @overlay.
 *
 * The widget will be stacked on top of the main widget
 * added with [method@Bobgui.Overlay.set_child].
 *
 * The position at which @widget is placed is determined
 * from its [property@Bobgui.Widget:halign] and
 * [property@Bobgui.Widget:valign] properties.
 */
void
bobgui_overlay_add_overlay (BobguiOverlay *overlay,
                         BobguiWidget  *widget)
{
  g_return_if_fail (BOBGUI_IS_OVERLAY (overlay));
  g_return_if_fail (BOBGUI_IS_WIDGET (widget));
  g_return_if_fail (widget != overlay->child);

  bobgui_widget_insert_before (widget, BOBGUI_WIDGET (overlay), NULL);
}

/**
 * bobgui_overlay_remove_overlay:
 * @overlay: a `BobguiOverlay`
 * @widget: a `BobguiWidget` to be removed
 *
 * Removes an overlay that was added with bobgui_overlay_add_overlay().
 */
void
bobgui_overlay_remove_overlay (BobguiOverlay *overlay,
                            BobguiWidget  *widget)
{
  g_return_if_fail (BOBGUI_IS_OVERLAY (overlay));
  g_return_if_fail (BOBGUI_IS_WIDGET (widget));
  g_return_if_fail (bobgui_widget_get_parent (widget) == BOBGUI_WIDGET (overlay));
  g_return_if_fail (widget != overlay->child);

  bobgui_widget_unparent (widget);
}

/**
 * bobgui_overlay_set_measure_overlay:
 * @overlay: a `BobguiOverlay`
 * @widget: an overlay child of `BobguiOverlay`
 * @measure: whether the child should be measured
 *
 * Sets whether @widget is included in the measured size of @overlay.
 *
 * The overlay will request the size of the largest child that has
 * this property set to %TRUE. Children who are not included may
 * be drawn outside of @overlay's allocation if they are too large.
 */
void
bobgui_overlay_set_measure_overlay (BobguiOverlay *overlay,
				 BobguiWidget  *widget,
				 gboolean    measure)
{
  BobguiLayoutManager *layout;
  BobguiOverlayLayoutChild *child;

  g_return_if_fail (BOBGUI_IS_OVERLAY (overlay));
  g_return_if_fail (BOBGUI_IS_WIDGET (widget));

  layout = bobgui_widget_get_layout_manager (BOBGUI_WIDGET (overlay));
  child = BOBGUI_OVERLAY_LAYOUT_CHILD (bobgui_layout_manager_get_layout_child (layout, widget));
  bobgui_overlay_layout_child_set_measure (child, measure);
}

/**
 * bobgui_overlay_get_measure_overlay:
 * @overlay: a `BobguiOverlay`
 * @widget: an overlay child of `BobguiOverlay`
 *
 * Gets whether @widget's size is included in the measurement of
 * @overlay.
 *
 * Returns: whether the widget is measured
 */
gboolean
bobgui_overlay_get_measure_overlay (BobguiOverlay *overlay,
				 BobguiWidget  *widget)
{
  BobguiLayoutManager *layout;
  BobguiOverlayLayoutChild *child;

  g_return_val_if_fail (BOBGUI_IS_OVERLAY (overlay), FALSE);
  g_return_val_if_fail (BOBGUI_IS_WIDGET (widget), FALSE);

  layout = bobgui_widget_get_layout_manager (BOBGUI_WIDGET (overlay));
  child = BOBGUI_OVERLAY_LAYOUT_CHILD (bobgui_layout_manager_get_layout_child (layout, widget));
  return bobgui_overlay_layout_child_get_measure (child);
}

/**
 * bobgui_overlay_set_clip_overlay:
 * @overlay: a `BobguiOverlay`
 * @widget: an overlay child of `BobguiOverlay`
 * @clip_overlay: whether the child should be clipped
 *
 * Sets whether @widget should be clipped within the parent.
 */
void
bobgui_overlay_set_clip_overlay (BobguiOverlay *overlay,
                              BobguiWidget  *widget,
                              gboolean    clip_overlay)
{
  BobguiLayoutManager *layout;
  BobguiOverlayLayoutChild *child;

  g_return_if_fail (BOBGUI_IS_OVERLAY (overlay));
  g_return_if_fail (BOBGUI_IS_WIDGET (widget));

  layout = bobgui_widget_get_layout_manager (BOBGUI_WIDGET (overlay));
  child = BOBGUI_OVERLAY_LAYOUT_CHILD (bobgui_layout_manager_get_layout_child (layout, widget));
  bobgui_overlay_layout_child_set_clip_overlay (child, clip_overlay);
}

/**
 * bobgui_overlay_get_clip_overlay:
 * @overlay: a `BobguiOverlay`
 * @widget: an overlay child of `BobguiOverlay`
 *
 * Gets whether @widget should be clipped within the parent.
 *
 * Returns: whether the widget is clipped within the parent.
 */
gboolean
bobgui_overlay_get_clip_overlay (BobguiOverlay *overlay,
                              BobguiWidget  *widget)
{
  BobguiLayoutManager *layout;
  BobguiOverlayLayoutChild *child;

  g_return_val_if_fail (BOBGUI_IS_OVERLAY (overlay), FALSE);
  g_return_val_if_fail (BOBGUI_IS_WIDGET (widget), FALSE);

  layout = bobgui_widget_get_layout_manager (BOBGUI_WIDGET (overlay));
  child = BOBGUI_OVERLAY_LAYOUT_CHILD (bobgui_layout_manager_get_layout_child (layout, widget));

  return bobgui_overlay_layout_child_get_clip_overlay (child);
}

/**
 * bobgui_overlay_set_child:
 * @overlay: a `BobguiOverlay`
 * @child: (nullable): the child widget
 *
 * Sets the child widget of @overlay.
 */
void
bobgui_overlay_set_child (BobguiOverlay *overlay,
                       BobguiWidget  *child)
{
  g_return_if_fail (BOBGUI_IS_OVERLAY (overlay));
  g_return_if_fail (child == NULL || overlay->child == child || bobgui_widget_get_parent (child) == NULL);

  if (overlay->child == child)
    return;

  g_clear_pointer (&overlay->child, bobgui_widget_unparent);

  overlay->child = child;

  if (child)
    {
      /* Make sure the main-child node is the first one */
      bobgui_widget_insert_after (child, BOBGUI_WIDGET (overlay), NULL);
    }

  g_object_notify (G_OBJECT (overlay), "child");
}

/**
 * bobgui_overlay_get_child:
 * @overlay: a `BobguiOverlay`
 *
 * Gets the child widget of @overlay.
 *
 * Returns: (nullable) (transfer none): the child widget of @overlay
 */
BobguiWidget *
bobgui_overlay_get_child (BobguiOverlay *overlay)
{
  g_return_val_if_fail (BOBGUI_IS_OVERLAY (overlay), NULL);

  return overlay->child;
}
