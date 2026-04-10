/* BOBGUI - The Bobgui Framework
 * Copyright 2019 Matthias Clasen
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

#include "bobguidragicon.h"

#include "bobguiwidgetprivate.h"
#include "bobguicssstyleprivate.h"
#include "bobguicsstypesprivate.h"
#include "bobguinativeprivate.h"
#include "bobguipicture.h"
#include "bobguicssboxesimplprivate.h"
#include "bobguicssnumbervalueprivate.h"

#include "gdk/gdksurfaceprivate.h"
#include "gdk/gdkdragsurfacesize.h"
#include "gsk/gskrendererprivate.h"

/* for the drag icons */
#include "bobguicolorswatchprivate.h"
#include "bobguiimage.h"
#include "bobguilabel.h"
#include "bobguirendernodepaintableprivate.h"
#include "bobguitextutilprivate.h"


/**
 * BobguiDragIcon:
 *
 * A `BobguiRoot` implementation for drag icons.
 *
 * A drag icon moves with the pointer during a Drag-and-Drop operation
 * and is destroyed when the drag ends.
 *
 * To set up a drag icon and associate it with an ongoing drag operation,
 * use [ctor@Bobgui.DragIcon.get_for_drag] to get the icon for a drag. You can
 * then use it like any other widget and use [method@Bobgui.DragIcon.set_child]
 * to set whatever widget should be used for the drag icon.
 *
 * Keep in mind that drag icons do not allow user input.
 */
struct _BobguiDragIcon
{
  BobguiWidget parent_instance;

  GdkSurface *surface;
  GskRenderer *renderer;
  BobguiWidget *child;
};

struct _BobguiDragIconClass
{
  BobguiWidgetClass parent_class;
};

enum {
  PROP_0,
  PROP_CHILD,

  LAST_ARG
};

static GParamSpec *properties[LAST_ARG] = { NULL, };

static void bobgui_drag_icon_root_init   (BobguiRootInterface *iface);
static void bobgui_drag_icon_native_init (BobguiNativeInterface *iface);

G_DEFINE_TYPE_WITH_CODE (BobguiDragIcon, bobgui_drag_icon, BOBGUI_TYPE_WIDGET,
                         G_IMPLEMENT_INTERFACE (BOBGUI_TYPE_NATIVE,
                                                bobgui_drag_icon_native_init)
                         G_IMPLEMENT_INTERFACE (BOBGUI_TYPE_ROOT,
                                                bobgui_drag_icon_root_init))

static GdkDisplay *
bobgui_drag_icon_root_get_display (BobguiRoot *self)
{
  BobguiDragIcon *icon = BOBGUI_DRAG_ICON (self);

  if (icon->surface)
    return gdk_surface_get_display (icon->surface);

  return gdk_display_get_default ();
}

static void
bobgui_drag_icon_root_init (BobguiRootInterface *iface)
{
  iface->get_display = bobgui_drag_icon_root_get_display;
}

static GdkSurface *
bobgui_drag_icon_native_get_surface (BobguiNative *native)
{
  BobguiDragIcon *icon = BOBGUI_DRAG_ICON (native);

  return icon->surface;
}

static GskRenderer *
bobgui_drag_icon_native_get_renderer (BobguiNative *native)
{
  BobguiDragIcon *icon = BOBGUI_DRAG_ICON (native);

  return icon->renderer;
}

static void
bobgui_drag_icon_native_get_surface_transform (BobguiNative *native,
                                            double    *x,
                                            double    *y)
{
  BobguiCssBoxes css_boxes;
  const graphene_rect_t *margin_rect;

  bobgui_css_boxes_init (&css_boxes, BOBGUI_WIDGET (native));
  margin_rect = bobgui_css_boxes_get_margin_rect (&css_boxes);

  *x = - margin_rect->origin.x;
  *y = - margin_rect->origin.y;
}

static void
bobgui_drag_icon_move_resize (BobguiDragIcon *icon)
{
  BobguiRequisition req;

  if (icon->surface)
    {
      bobgui_widget_get_preferred_size (BOBGUI_WIDGET (icon), NULL, &req);
      gdk_drag_surface_present (GDK_DRAG_SURFACE (icon->surface),
                                MAX (1, req.width),
                                MAX (1, req.height));
    }
}

static void
bobgui_drag_icon_present (BobguiDragIcon *icon)
{
  BobguiWidget *widget = BOBGUI_WIDGET (icon);

  if (!_bobgui_widget_get_alloc_needed (widget))
    bobgui_widget_ensure_allocate (widget);
  else if (bobgui_widget_get_visible (widget))
    bobgui_drag_icon_move_resize (icon);
}

static void
bobgui_drag_icon_native_layout (BobguiNative *native,
                             int        width,
                             int        height)
{
  bobgui_widget_allocate (BOBGUI_WIDGET (native), width, height, -1, NULL);
}

static void
bobgui_drag_icon_native_init (BobguiNativeInterface *iface)
{
  iface->get_surface = bobgui_drag_icon_native_get_surface;
  iface->get_renderer = bobgui_drag_icon_native_get_renderer;
  iface->get_surface_transform = bobgui_drag_icon_native_get_surface_transform;
  iface->layout = bobgui_drag_icon_native_layout;
}

static gboolean
surface_render (GdkSurface     *surface,
                cairo_region_t *region,
                BobguiWidget      *widget)
{
  bobgui_widget_render (widget, surface, region);
  return TRUE;
}

static void
surface_compute_size (GdkDragSurface     *surface,
                      GdkDragSurfaceSize *size,
                      BobguiWidget          *widget)
{
  BobguiRequisition nat_size;
  bobgui_widget_get_preferred_size (widget, NULL, &nat_size);
  gdk_drag_surface_size_set_size (size, nat_size.width, nat_size.height);
}

static void
bobgui_drag_icon_realize (BobguiWidget *widget)
{
  BobguiDragIcon *icon = BOBGUI_DRAG_ICON (widget);

  g_warn_if_fail (icon->surface != NULL);

  gdk_surface_set_widget (icon->surface, widget);

  g_signal_connect (icon->surface, "render", G_CALLBACK (surface_render), widget);
  g_signal_connect (icon->surface, "compute-size", G_CALLBACK (surface_compute_size), widget);

  BOBGUI_WIDGET_CLASS (bobgui_drag_icon_parent_class)->realize (widget);

  icon->renderer = gsk_renderer_new_for_surface_full (icon->surface, TRUE);

  bobgui_native_realize (BOBGUI_NATIVE (icon));
}

static void
bobgui_drag_icon_unrealize (BobguiWidget *widget)
{
  BobguiDragIcon *icon = BOBGUI_DRAG_ICON (widget);

  bobgui_native_unrealize (BOBGUI_NATIVE (icon));

  BOBGUI_WIDGET_CLASS (bobgui_drag_icon_parent_class)->unrealize (widget);

  gsk_renderer_unrealize (icon->renderer);
  g_clear_object (&icon->renderer);

  if (icon->surface)
    {
      g_signal_handlers_disconnect_by_func (icon->surface, surface_render, widget);
      g_signal_handlers_disconnect_by_func (icon->surface, surface_compute_size, widget);
      gdk_surface_set_widget (icon->surface, NULL);
    }
}

static void
bobgui_drag_icon_map (BobguiWidget *widget)
{
  BobguiDragIcon *icon = BOBGUI_DRAG_ICON (widget);

  bobgui_drag_icon_move_resize (icon);

  BOBGUI_WIDGET_CLASS (bobgui_drag_icon_parent_class)->map (widget);

  if (icon->child && bobgui_widget_get_visible (icon->child))
    bobgui_widget_map (icon->child);
}

static void
bobgui_drag_icon_unmap (BobguiWidget *widget)
{
  BobguiDragIcon *icon = BOBGUI_DRAG_ICON (widget);

  g_warn_if_fail (icon->surface != NULL);
  BOBGUI_WIDGET_CLASS (bobgui_drag_icon_parent_class)->unmap (widget);
  if (icon->surface)
    gdk_surface_hide (icon->surface);

  if (icon->child)
    bobgui_widget_unmap (icon->child);
}

static void
bobgui_drag_icon_measure (BobguiWidget      *widget,
                       BobguiOrientation  orientation,
                       int             for_size,
                       int            *minimum,
                       int            *natural,
                       int            *minimum_baseline,
                       int            *natural_baseline)
{
  BobguiDragIcon *icon = BOBGUI_DRAG_ICON (widget);

  if (icon->child)
    bobgui_widget_measure (icon->child,
                        orientation, for_size,
                        minimum, natural,
                        minimum_baseline, natural_baseline);
}

static void
bobgui_drag_icon_size_allocate (BobguiWidget *widget,
                             int        width,
                             int        height,
                             int        baseline)
{
  BobguiDragIcon *icon = BOBGUI_DRAG_ICON (widget);

  if (icon->child)
    bobgui_widget_allocate (icon->child, width, height, baseline, NULL);
}

static void
bobgui_drag_icon_show (BobguiWidget *widget)
{
  _bobgui_widget_set_visible_flag (widget, TRUE);
  bobgui_css_node_validate (bobgui_widget_get_css_node (widget));
  bobgui_widget_realize (widget);
  bobgui_drag_icon_present (BOBGUI_DRAG_ICON (widget));
  bobgui_widget_map (widget);
}

static void
bobgui_drag_icon_hide (BobguiWidget *widget)
{
  _bobgui_widget_set_visible_flag (widget, FALSE);
  bobgui_widget_unmap (widget);
}

static void
bobgui_drag_icon_dispose (GObject *object)
{
  BobguiDragIcon *icon = BOBGUI_DRAG_ICON (object);

  g_clear_pointer (&icon->child, bobgui_widget_unparent);

  G_OBJECT_CLASS (bobgui_drag_icon_parent_class)->dispose (object);

  g_clear_object (&icon->surface);
}

static void
bobgui_drag_icon_get_property (GObject     *object,
                            guint        prop_id,
                            GValue      *value,
                            GParamSpec  *pspec)
{
  BobguiDragIcon *self = BOBGUI_DRAG_ICON (object);

  switch (prop_id)
    {
    case PROP_CHILD:
      g_value_set_object (value, self->child);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bobgui_drag_icon_set_property (GObject      *object,
                            guint         prop_id,
                            const GValue *value,
                            GParamSpec    *pspec)
{
  BobguiDragIcon *self = BOBGUI_DRAG_ICON (object);

  switch (prop_id)
    {
    case PROP_CHILD:
      bobgui_drag_icon_set_child (self, g_value_get_object (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bobgui_drag_icon_class_init (BobguiDragIconClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (klass);

  object_class->dispose = bobgui_drag_icon_dispose;
  object_class->get_property = bobgui_drag_icon_get_property;
  object_class->set_property = bobgui_drag_icon_set_property;

  widget_class->realize = bobgui_drag_icon_realize;
  widget_class->unrealize = bobgui_drag_icon_unrealize;
  widget_class->map = bobgui_drag_icon_map;
  widget_class->unmap = bobgui_drag_icon_unmap;
  widget_class->measure = bobgui_drag_icon_measure;
  widget_class->size_allocate = bobgui_drag_icon_size_allocate;
  widget_class->show = bobgui_drag_icon_show;
  widget_class->hide = bobgui_drag_icon_hide;

  /**
   * BobguiDragIcon:child:
   *
   * The widget to display as drag icon.
   */
  properties[PROP_CHILD] =
    g_param_spec_object ("child", NULL, NULL,
                         BOBGUI_TYPE_WIDGET,
                         G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, LAST_ARG, properties);

  bobgui_widget_class_set_css_name (widget_class, "dnd");
}

static void
bobgui_drag_icon_init (BobguiDragIcon *self)
{
  bobgui_widget_set_can_target (BOBGUI_WIDGET (self), FALSE);
}

/**
 * bobgui_drag_icon_get_for_drag: (constructor)
 * @drag: a `GdkDrag`
 *
 * Gets the `BobguiDragIcon` in use with @drag.
 *
 * If no drag icon exists yet, a new one will be created
 * and shown.
 *
 * Returns: (transfer none): the `BobguiDragIcon`
 */
BobguiWidget *
bobgui_drag_icon_get_for_drag (GdkDrag *drag)
{
  static GQuark drag_icon_quark = 0;
  BobguiWidget *self;

  g_return_val_if_fail (GDK_IS_DRAG (drag), NULL);

  if (G_UNLIKELY (drag_icon_quark == 0))
    drag_icon_quark = g_quark_from_static_string ("-bobgui-drag-icon");

  self = g_object_get_qdata (G_OBJECT (drag), drag_icon_quark);
  if (self == NULL)
    {
      self = g_object_new (BOBGUI_TYPE_DRAG_ICON, NULL);

      BOBGUI_DRAG_ICON (self)->surface = g_object_ref (gdk_drag_get_drag_surface (drag));

      g_object_set_qdata_full (G_OBJECT (drag), drag_icon_quark, g_object_ref_sink (self), g_object_unref);

      if (BOBGUI_DRAG_ICON (self)->child != NULL)
        bobgui_widget_set_visible (self, TRUE);
    }

  return self;
}

/**
 * bobgui_drag_icon_set_from_paintable:
 * @drag: a `GdkDrag`
 * @paintable: a `GdkPaintable` to display
 * @hot_x: X coordinate of the hotspot
 * @hot_y: Y coordinate of the hotspot
 *
 * Creates a `BobguiDragIcon` that shows @paintable, and associates
 * it with the drag operation.
 *
 * The hotspot position on the paintable is aligned with the
 * hotspot of the cursor.
 */
void
bobgui_drag_icon_set_from_paintable (GdkDrag      *drag,
                                  GdkPaintable *paintable,
                                  int           hot_x,
                                  int           hot_y)
{
  BobguiWidget *icon;
  BobguiWidget *picture;

  gdk_drag_set_hotspot (drag, hot_x, hot_y);

  icon = bobgui_drag_icon_get_for_drag (drag);

  picture = bobgui_picture_new_for_paintable (paintable);
  bobgui_picture_set_can_shrink (BOBGUI_PICTURE (picture), FALSE);
  bobgui_drag_icon_set_child (BOBGUI_DRAG_ICON (icon), picture);
}

/**
 * bobgui_drag_icon_set_child:
 * @self: a `BobguiDragIcon`
 * @child: (nullable): a `BobguiWidget`
 *
 * Sets the widget to display as the drag icon.
 */
void
bobgui_drag_icon_set_child (BobguiDragIcon *self,
                         BobguiWidget   *child)
{
  g_return_if_fail (BOBGUI_IS_DRAG_ICON (self));
  g_return_if_fail (child == NULL || bobgui_widget_get_parent (child) == NULL);

  if (self->child == child)
    return;

  if (self->child)
    bobgui_widget_unparent (self->child);

  self->child = child;

  if (self->child)
    {
      bobgui_widget_set_parent (self->child, BOBGUI_WIDGET (self));
      bobgui_widget_set_visible (BOBGUI_WIDGET (self), TRUE);
    }

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_CHILD]);
}

/**
 * bobgui_drag_icon_get_child:
 * @self: a `BobguiDragIcon`
 *
 * Gets the widget currently used as drag icon.
 *
 * Returns: (nullable) (transfer none): The drag icon
 **/
BobguiWidget *
bobgui_drag_icon_get_child (BobguiDragIcon *self)
{
  g_return_val_if_fail (BOBGUI_IS_DRAG_ICON (self), NULL);

  return self->child;
}

/**
 * bobgui_drag_icon_create_widget_for_value:
 * @value: a `GValue`
 *
 * Creates a widget that can be used as a drag icon for the given
 * @value.
 *
 * Supported types include strings, `GdkRGBA` and `BobguiTextBuffer`.
 * If BOBGUI does not know how to create a widget for a given value,
 * it will return %NULL.
 *
 * This method is used to set the default drag icon on drag-and-drop
 * operations started by `BobguiDragSource`, so you don't need to set
 * a drag icon using this function there.
 *
 * Returns: (nullable) (transfer full): A new `BobguiWidget`
 *   for displaying @value as a drag icon.
 */
BobguiWidget *
bobgui_drag_icon_create_widget_for_value (const GValue *value)
{
  g_return_val_if_fail (G_IS_VALUE (value), NULL);

  if (G_VALUE_HOLDS (value, G_TYPE_STRING))
    {
      return bobgui_label_new (g_value_get_string (value));
    }
  else if (G_VALUE_HOLDS (value, GDK_TYPE_PAINTABLE))
    {
      BobguiWidget *image;

      image = bobgui_image_new_from_paintable (g_value_get_object (value));
      bobgui_widget_add_css_class (image, "large-icons");

      return image;
    }
  else if (G_VALUE_HOLDS (value, GDK_TYPE_RGBA))
    {
      BobguiWidget *swatch;

      swatch = bobgui_color_swatch_new ();
      bobgui_color_swatch_set_can_drag (BOBGUI_COLOR_SWATCH (swatch), FALSE);
      bobgui_color_swatch_set_can_drop (BOBGUI_COLOR_SWATCH (swatch), FALSE);
      bobgui_color_swatch_set_rgba (BOBGUI_COLOR_SWATCH (swatch), g_value_get_boxed (value));

      return swatch;
    }
  else if (G_VALUE_HOLDS (value, G_TYPE_FILE))
    {
      GFileInfo *info;
      BobguiWidget *image;

      info = g_file_query_info (G_FILE (g_value_get_object (value)), "standard::icon", 0, NULL, NULL);
      if (!info)
        return NULL;

      image = bobgui_image_new_from_gicon (g_file_info_get_icon (info));
      bobgui_widget_add_css_class (image, "large-icons");
      g_object_unref (info);

      return image;
    }
  else if (G_VALUE_HOLDS (value, BOBGUI_TYPE_TEXT_BUFFER))
    {
      BobguiTextBuffer *buffer = g_value_get_object (value);
      BobguiTextIter start, end;
      GdkPaintable *paintable;
      BobguiWidget *picture;

      if (buffer == NULL || !bobgui_text_buffer_get_selection_bounds (buffer, &start, &end))
        return NULL;

      picture = bobgui_picture_new ();
      paintable = bobgui_text_util_create_rich_drag_icon (picture, buffer, &start, &end);
      bobgui_picture_set_paintable (BOBGUI_PICTURE (picture), paintable);
      bobgui_picture_set_can_shrink (BOBGUI_PICTURE (picture), FALSE);
      g_object_unref (paintable);

      return picture;
    }
  else if (G_VALUE_HOLDS (value, GSK_TYPE_RENDER_NODE))
    {
      GskRenderNode *node;
      GdkPaintable *paintable;
      graphene_rect_t bounds;
      BobguiWidget *image;

      node = gsk_value_get_render_node (value);
      if (node == NULL)
        return NULL;

      gsk_render_node_get_bounds (node, &bounds);
      paintable = bobgui_render_node_paintable_new (node, &bounds);
      image = bobgui_image_new_from_paintable (paintable);
      bobgui_image_set_icon_size (BOBGUI_IMAGE (image), BOBGUI_ICON_SIZE_LARGE);
      g_object_unref (paintable);

      return image;
    }
  else
    {
      return NULL;
    }
}

