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

#include "bobguiscalerprivate.h"

#include "bobguisnapshot.h"

struct _BobguiScaler
{
  GObject parent_instance;

  GdkPaintable *paintable;
  double scale;
};

struct _BobguiScalerClass
{
  GObjectClass parent_class;
};

static void
bobgui_scaler_paintable_snapshot (GdkPaintable *paintable,
                               GdkSnapshot  *snapshot,
                               double        width,
                               double        height)
{
  BobguiScaler *self = BOBGUI_SCALER (paintable);

  bobgui_snapshot_save (snapshot);

  bobgui_snapshot_scale (snapshot, 1.0 / self->scale, 1.0 / self->scale);

  gdk_paintable_snapshot (self->paintable,
                          snapshot,
                          width * self->scale,
                          height * self->scale);

  bobgui_snapshot_restore (snapshot);
}

static GdkPaintable *
bobgui_scaler_paintable_get_current_image (GdkPaintable *paintable)
{
  BobguiScaler *self = BOBGUI_SCALER (paintable);
  GdkPaintable *current_paintable, *current_self;

  current_paintable = gdk_paintable_get_current_image (self->paintable);
  current_self = bobgui_scaler_new (current_paintable, self->scale);
  g_object_unref (current_paintable);

  return current_self;
}

static GdkPaintableFlags
bobgui_scaler_paintable_get_flags (GdkPaintable *paintable)
{
  BobguiScaler *self = BOBGUI_SCALER (paintable);

  return gdk_paintable_get_flags (self->paintable);
}

static int
bobgui_scaler_paintable_get_intrinsic_width (GdkPaintable *paintable)
{
  BobguiScaler *self = BOBGUI_SCALER (paintable);

  return gdk_paintable_get_intrinsic_width (self->paintable) / self->scale;
}

static int
bobgui_scaler_paintable_get_intrinsic_height (GdkPaintable *paintable)
{
  BobguiScaler *self = BOBGUI_SCALER (paintable);

  return gdk_paintable_get_intrinsic_height (self->paintable) / self->scale;
}

static double bobgui_scaler_paintable_get_intrinsic_aspect_ratio (GdkPaintable *paintable)
{
  BobguiScaler *self = BOBGUI_SCALER (paintable);

  return gdk_paintable_get_intrinsic_aspect_ratio (self->paintable);
};

static void
bobgui_scaler_paintable_init (GdkPaintableInterface *iface)
{
  iface->snapshot = bobgui_scaler_paintable_snapshot;
  iface->get_current_image = bobgui_scaler_paintable_get_current_image;
  iface->get_flags = bobgui_scaler_paintable_get_flags;
  iface->get_intrinsic_width = bobgui_scaler_paintable_get_intrinsic_width;
  iface->get_intrinsic_height = bobgui_scaler_paintable_get_intrinsic_height;
  iface->get_intrinsic_aspect_ratio = bobgui_scaler_paintable_get_intrinsic_aspect_ratio;
}

G_DEFINE_TYPE_EXTENDED (BobguiScaler, bobgui_scaler, G_TYPE_OBJECT, 0,
                        G_IMPLEMENT_INTERFACE (GDK_TYPE_PAINTABLE,
                                               bobgui_scaler_paintable_init))

static void
bobgui_scaler_dispose (GObject *object)
{
  BobguiScaler *self = BOBGUI_SCALER (object);

  if (self->paintable)
    {
      const guint flags = gdk_paintable_get_flags (self->paintable);

      if ((flags & GDK_PAINTABLE_STATIC_CONTENTS) == 0)
        g_signal_handlers_disconnect_by_func (self->paintable, gdk_paintable_invalidate_contents, self);

      if ((flags & GDK_PAINTABLE_STATIC_SIZE) == 0)
        g_signal_handlers_disconnect_by_func (self->paintable, gdk_paintable_invalidate_size, self);

      g_clear_object (&self->paintable);
    }

  G_OBJECT_CLASS (bobgui_scaler_parent_class)->dispose (object);
}

static void
bobgui_scaler_class_init (BobguiScalerClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->dispose = bobgui_scaler_dispose;
}

static void
bobgui_scaler_init (BobguiScaler *self)
{
  self->scale = 1.0;
}

GdkPaintable *
bobgui_scaler_new (GdkPaintable *paintable,
                double        scale)
{
  BobguiScaler *self;
  guint flags;

  g_return_val_if_fail (GDK_IS_PAINTABLE (paintable), NULL);
  g_return_val_if_fail (scale > 0.0, NULL);

  self = g_object_new (BOBGUI_TYPE_SCALER, NULL);

  self->paintable = g_object_ref (paintable);
  flags = gdk_paintable_get_flags (paintable);

  if ((flags & GDK_PAINTABLE_STATIC_CONTENTS) == 0)
    g_signal_connect_swapped (paintable, "invalidate-contents", G_CALLBACK (gdk_paintable_invalidate_contents), self);

  if ((flags & GDK_PAINTABLE_STATIC_SIZE) == 0)
    g_signal_connect_swapped (paintable, "invalidate-size", G_CALLBACK (gdk_paintable_invalidate_size), self);

  self->scale = scale;

  return GDK_PAINTABLE (self);
}
