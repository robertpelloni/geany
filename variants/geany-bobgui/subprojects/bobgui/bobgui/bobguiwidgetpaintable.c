/*
 * Copyright © 2018 Benjamin Otte
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
 * Authors: Benjamin Otte <otte@gnome.org>
 */

#include "config.h"

#include "bobguiwidgetpaintableprivate.h"

#include "bobguisnapshot.h"
#include "bobguirendernodepaintableprivate.h"
#include "bobguiwidgetprivate.h"
#include "bobguiprivate.h"

/**
 * BobguiWidgetPaintable:
 *
 * A `GdkPaintable` that displays the contents of a widget.
 *
 * `BobguiWidgetPaintable` will also take care of the widget not being in a
 * state where it can be drawn (like when it isn't shown) and just draw
 * nothing or where it does not have a size (like when it is hidden) and
 * report no size in that case.
 *
 * Of course, `BobguiWidgetPaintable` allows you to monitor widgets for size
 * changes by emitting the [signal@Gdk.Paintable::invalidate-size] signal
 * whenever the size of the widget changes as well as for visual changes by
 * emitting the [signal@Gdk.Paintable::invalidate-contents] signal whenever
 * the widget changes.
 *
 * You can use a `BobguiWidgetPaintable` everywhere a `GdkPaintable` is allowed,
 * including using it on a `BobguiPicture` (or one of its parents) that it was
 * set on itself via bobgui_picture_set_paintable(). The paintable will take care
 * of recursion when this happens. If you do this however, ensure that the
 * [property@Bobgui.Picture:can-shrink] property is set to %TRUE or you might
 * end up with an infinitely growing widget.
 */
struct _BobguiWidgetPaintable
{
  GObject parent_instance;

  BobguiWidget *widget;
  guint snapshot_count;

  guint         pending_update_cb;      /* the idle source that updates the valid image to be the new current image */

  GdkPaintable *current_image;          /* the image that we are presenting */
  GdkPaintable *pending_image;          /* the image that we should be presenting */
};

struct _BobguiWidgetPaintableClass
{
  GObjectClass parent_class;
};

enum {
  PROP_0,
  PROP_WIDGET,

  N_PROPS,
};

static GParamSpec *properties[N_PROPS] = { NULL, };

static void
bobgui_widget_paintable_paintable_snapshot (GdkPaintable *paintable,
                                         GdkSnapshot  *snapshot,
                                         double        width,
                                         double        height)
{
  BobguiWidgetPaintable *self = BOBGUI_WIDGET_PAINTABLE (paintable);

  if (self->snapshot_count > 4)
    return;
  else if (self->snapshot_count > 0)
    {
      graphene_rect_t bounds;

      bobgui_snapshot_push_clip (snapshot,
                              &GRAPHENE_RECT_INIT(0, 0, width, height));

      if (bobgui_widget_compute_bounds (self->widget, self->widget, &bounds))
        {
          bobgui_snapshot_scale (snapshot, width / bounds.size.width, height / bounds.size.height);
          bobgui_snapshot_translate (snapshot, &bounds.origin);
        }

      bobgui_widget_snapshot (self->widget, snapshot);

      bobgui_snapshot_pop (snapshot);
    }
  else
    {
      gdk_paintable_snapshot (self->current_image, snapshot, width, height);
    }
}

static GdkPaintable *
bobgui_widget_paintable_paintable_get_current_image (GdkPaintable *paintable)
{
  BobguiWidgetPaintable *self = BOBGUI_WIDGET_PAINTABLE (paintable);

  return g_object_ref (self->current_image);
}

static int
bobgui_widget_paintable_paintable_get_intrinsic_width (GdkPaintable *paintable)
{
  BobguiWidgetPaintable *self = BOBGUI_WIDGET_PAINTABLE (paintable);

  return gdk_paintable_get_intrinsic_width (self->current_image);
}

static int
bobgui_widget_paintable_paintable_get_intrinsic_height (GdkPaintable *paintable)
{
  BobguiWidgetPaintable *self = BOBGUI_WIDGET_PAINTABLE (paintable);

  return gdk_paintable_get_intrinsic_height (self->current_image);
}

static void
bobgui_widget_paintable_paintable_init (GdkPaintableInterface *iface)
{
  iface->snapshot = bobgui_widget_paintable_paintable_snapshot;
  iface->get_current_image = bobgui_widget_paintable_paintable_get_current_image;
  iface->get_intrinsic_width = bobgui_widget_paintable_paintable_get_intrinsic_width;
  iface->get_intrinsic_height = bobgui_widget_paintable_paintable_get_intrinsic_height;
}

G_DEFINE_TYPE_EXTENDED (BobguiWidgetPaintable, bobgui_widget_paintable, G_TYPE_OBJECT, 0,
                        G_IMPLEMENT_INTERFACE (GDK_TYPE_PAINTABLE,
                                               bobgui_widget_paintable_paintable_init))

static void
bobgui_widget_paintable_set_property (GObject      *object,
                                   guint         prop_id,
                                   const GValue *value,
                                   GParamSpec   *pspec)

{
  BobguiWidgetPaintable *self = BOBGUI_WIDGET_PAINTABLE (object);

  switch (prop_id)
    {
    case PROP_WIDGET:
      bobgui_widget_paintable_set_widget (self, g_value_get_object (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bobgui_widget_paintable_get_property (GObject    *object,
                                   guint       prop_id,
                                   GValue     *value,
                                   GParamSpec *pspec)
{
  BobguiWidgetPaintable *self = BOBGUI_WIDGET_PAINTABLE (object);

  switch (prop_id)
    {
    case PROP_WIDGET:
      g_value_set_object (value, self->widget);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bobgui_widget_paintable_unset_widget (BobguiWidgetPaintable *self)
{
  if (self->widget == NULL)
    return;

  self->widget->priv->paintables = g_slist_remove (self->widget->priv->paintables,
                                                   self);

  self->widget = NULL;

  g_clear_object (&self->pending_image);
  if (self->pending_update_cb)
    {
      g_source_remove (self->pending_update_cb);
      self->pending_update_cb = 0;
    }
}

static void
bobgui_widget_paintable_dispose (GObject *object)
{
  BobguiWidgetPaintable *self = BOBGUI_WIDGET_PAINTABLE (object);

  bobgui_widget_paintable_unset_widget (self);

  G_OBJECT_CLASS (bobgui_widget_paintable_parent_class)->dispose (object);
}

static void
bobgui_widget_paintable_finalize (GObject *object)
{
  BobguiWidgetPaintable *self = BOBGUI_WIDGET_PAINTABLE (object);

  g_object_unref (self->current_image);

  G_OBJECT_CLASS (bobgui_widget_paintable_parent_class)->finalize (object);
}

static void
bobgui_widget_paintable_class_init (BobguiWidgetPaintableClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->get_property = bobgui_widget_paintable_get_property;
  gobject_class->set_property = bobgui_widget_paintable_set_property;
  gobject_class->dispose = bobgui_widget_paintable_dispose;
  gobject_class->finalize = bobgui_widget_paintable_finalize;

  /**
   * BobguiWidgetPaintable:widget:
   *
   * The observed widget or %NULL if none.
   */
  properties[PROP_WIDGET] =
    g_param_spec_object ("widget", NULL, NULL,
                         BOBGUI_TYPE_WIDGET,
                         G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (gobject_class, N_PROPS, properties);
}

static void
bobgui_widget_paintable_init (BobguiWidgetPaintable *self)
{
  self->current_image = gdk_paintable_new_empty (0, 0);
}

/**
 * bobgui_widget_paintable_new:
 * @widget: (nullable) (transfer none): a `BobguiWidget`
 *
 * Creates a new widget paintable observing the given widget.
 *
 * Returns: (transfer full) (type BobguiWidgetPaintable): a new `BobguiWidgetPaintable`
 */
GdkPaintable *
bobgui_widget_paintable_new (BobguiWidget *widget)
{
  g_return_val_if_fail (widget == NULL || BOBGUI_IS_WIDGET (widget), NULL);

  return g_object_new (BOBGUI_TYPE_WIDGET_PAINTABLE,
                       "widget", widget,
                       NULL);
}

static GdkPaintable *
bobgui_widget_paintable_snapshot_widget (BobguiWidgetPaintable *self)
{
  graphene_rect_t bounds;

  if (self->widget == NULL)
    return gdk_paintable_new_empty (0, 0);

  if (!bobgui_widget_compute_bounds (self->widget, self->widget, &bounds))
    return gdk_paintable_new_empty (0, 0);

  if (self->widget->priv->render_node == NULL)
    return gdk_paintable_new_empty (bounds.size.width, bounds.size.height);
  
  return bobgui_render_node_paintable_new (self->widget->priv->render_node, &bounds);
}

/**
 * bobgui_widget_paintable_get_widget:
 * @self: a `BobguiWidgetPaintable`
 *
 * Returns the widget that is observed or %NULL if none.
 *
 * Returns: (transfer none) (nullable): the observed widget.
 */
BobguiWidget *
bobgui_widget_paintable_get_widget (BobguiWidgetPaintable *self)
{
  g_return_val_if_fail (BOBGUI_IS_WIDGET_PAINTABLE (self), NULL);

  return self->widget;
}

/**
 * bobgui_widget_paintable_set_widget:
 * @self: a `BobguiWidgetPaintable`
 * @widget: (nullable): the widget to observe
 *
 * Sets the widget that should be observed.
 */
void
bobgui_widget_paintable_set_widget (BobguiWidgetPaintable *self,
                                 BobguiWidget          *widget)
{

  g_return_if_fail (BOBGUI_IS_WIDGET_PAINTABLE (self));
  g_return_if_fail (widget == NULL || BOBGUI_IS_WIDGET (widget));

  if (self->widget == widget)
    return;

  bobgui_widget_paintable_unset_widget (self);

  /* We do not ref the widget to not cause ref cycles when a widget
   * is told to observe itself or one of its parent.
   */
  self->widget = widget;

  if (widget)
    widget->priv->paintables = g_slist_prepend (widget->priv->paintables, self);

  g_object_unref (self->current_image);
  self->current_image = bobgui_widget_paintable_snapshot_widget (self);

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_WIDGET]);
  gdk_paintable_invalidate_size (GDK_PAINTABLE (self));
  gdk_paintable_invalidate_contents (GDK_PAINTABLE (self));
}

static gboolean
bobgui_widget_paintable_update_func (gpointer data)
{
  BobguiWidgetPaintable *self = data;
  GdkPaintable *old_image;

  if (self->current_image != self->pending_image)
    {
      old_image = self->current_image;
      self->current_image = self->pending_image;
      self->pending_image = NULL;
      self->pending_update_cb = 0;

      if (gdk_paintable_get_intrinsic_width (self->current_image) != gdk_paintable_get_intrinsic_width (old_image) ||
          gdk_paintable_get_intrinsic_height (self->current_image) != gdk_paintable_get_intrinsic_height (old_image))
        gdk_paintable_invalidate_size (GDK_PAINTABLE (self));

      g_object_unref (old_image);

      gdk_paintable_invalidate_contents (GDK_PAINTABLE (self));
    }
  else
    {
      g_clear_object (&self->pending_image);
      self->pending_update_cb = 0;
    }

  return G_SOURCE_REMOVE;
}

void
bobgui_widget_paintable_update_image (BobguiWidgetPaintable *self)
{
  GdkPaintable *pending_image;

  if (self->pending_update_cb == 0)
    {
      self->pending_update_cb = g_idle_add_full (G_PRIORITY_HIGH,
                                                 bobgui_widget_paintable_update_func,
                                                 self,
                                                 NULL);
      gdk_source_set_static_name_by_id (self->pending_update_cb, "[bobgui] bobgui_widget_paintable_update_func");
    }

  pending_image = bobgui_widget_paintable_snapshot_widget (self);
  g_set_object (&self->pending_image, pending_image);
  g_object_unref (pending_image);
}

void
bobgui_widget_paintable_push_snapshot_count (BobguiWidgetPaintable *self)
{
  self->snapshot_count++;
}

void
bobgui_widget_paintable_pop_snapshot_count (BobguiWidgetPaintable *self)
{
  self->snapshot_count--;
}
