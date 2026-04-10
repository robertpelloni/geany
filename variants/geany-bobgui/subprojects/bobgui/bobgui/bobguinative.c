/*
 * Copyright © 2019 Red Hat, Inc.
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
 * Authors: Matthias Clasen <mclasen@redhat.com>
 */

#include "config.h"

#include <math.h>


#include "bobguicssboxesimplprivate.h"
#include "bobguicsscolorvalueprivate.h"
#include "bobguicsscornervalueprivate.h"
#include "bobguicssnodeprivate.h"
#include "bobguicssshadowvalueprivate.h"
#include "bobguinativeprivate.h"
#include "bobguiwidgetprivate.h"

#include "gdk/gdksurfaceprivate.h"

typedef struct _BobguiNativePrivate
{
  gulong update_handler_id;
  gulong layout_handler_id;
  gulong scale_changed_handler_id;
  gulong enter_monitor_handler_id;
  gulong leave_monitor_handler_id;
} BobguiNativePrivate;

static GQuark quark_bobgui_native_private;

/**
 * BobguiNative:
 *
 * An interface for widgets that have their own [class@Gdk.Surface].
 *
 * The obvious example of a `BobguiNative` is `BobguiWindow`.
 *
 * Every widget that is not itself a `BobguiNative` is contained in one,
 * and you can get it with [method@Bobgui.Widget.get_native].
 *
 * To get the surface of a `BobguiNative`, use [method@Bobgui.Native.get_surface].
 * It is also possible to find the `BobguiNative` to which a surface
 * belongs, with [func@Bobgui.Native.get_for_surface].
 *
 * In addition to a [class@Gdk.Surface], a `BobguiNative` also provides
 * a [class@Gsk.Renderer] for rendering on that surface. To get the
 * renderer, use [method@Bobgui.Native.get_renderer].
 */

G_DEFINE_INTERFACE (BobguiNative, bobgui_native, BOBGUI_TYPE_WIDGET)

static GskRenderer *
bobgui_native_default_get_renderer (BobguiNative *self)
{
  return NULL;
}

static void
bobgui_native_default_get_surface_transform (BobguiNative *self,
                                          double    *x,
                                          double    *y)
{
  *x = 0;
  *y = 0;
}

static void
bobgui_native_default_layout (BobguiNative *self,
                           int        width,
                           int        height)
{
}

static void
bobgui_native_default_init (BobguiNativeInterface *iface)
{
  iface->get_renderer = bobgui_native_default_get_renderer;
  iface->get_surface_transform = bobgui_native_default_get_surface_transform;
  iface->layout = bobgui_native_default_layout;

  quark_bobgui_native_private = g_quark_from_static_string ("bobgui-native-private");
}

static void
frame_clock_update_cb (GdkFrameClock *clock,
                       BobguiNative     *native)
{
  if (BOBGUI_IS_ROOT (native))
    bobgui_css_node_validate (bobgui_widget_get_css_node (BOBGUI_WIDGET (native)));
}

static void
bobgui_native_layout (BobguiNative *self,
                   int        width,
                   int        height)
{
  BOBGUI_NATIVE_GET_IFACE (self)->layout (self, width, height);
}

static void
surface_layout_cb (GdkSurface *surface,
                   int         width,
                   int         height,
                   BobguiNative  *native)
{
  bobgui_native_layout (native, width, height);

  if (bobgui_widget_needs_allocate (BOBGUI_WIDGET (native)))
    bobgui_native_queue_relayout (native);
}

static void
scale_changed_cb (GdkSurface *surface,
                  GParamSpec *pspec,
                  BobguiNative  *native)
{
  _bobgui_widget_scale_changed (BOBGUI_WIDGET (native));
}

static void
monitor_changed_cb (GdkSurface *surface,
                    GdkMonitor *monitor,
                    BobguiNative  *native)
{
  bobgui_widget_monitor_changed (BOBGUI_WIDGET (native));
}

static void
verify_priv_unrealized (gpointer user_data)
{
  BobguiNativePrivate *priv = user_data;

  g_warn_if_fail (priv->update_handler_id == 0);
  g_warn_if_fail (priv->layout_handler_id == 0);
  g_warn_if_fail (priv->scale_changed_handler_id == 0);
  g_warn_if_fail (priv->enter_monitor_handler_id == 0);
  g_warn_if_fail (priv->leave_monitor_handler_id == 0);

  g_free (priv);
}

/**
 * bobgui_native_realize:
 * @self: a `BobguiNative`
 *
 * Realizes a `BobguiNative`.
 *
 * This should only be used by subclasses.
 */
void
bobgui_native_realize (BobguiNative *self)
{
  GdkSurface *surface;
  GdkFrameClock *clock;
  BobguiNativePrivate *priv;

  g_return_if_fail (g_object_get_qdata (G_OBJECT (self),
                                        quark_bobgui_native_private) == NULL);

  surface = bobgui_native_get_surface (self);
  clock = gdk_surface_get_frame_clock (surface);
  g_return_if_fail (clock != NULL);

  priv = g_new0 (BobguiNativePrivate, 1);
  priv->update_handler_id = g_signal_connect_after (clock, "update",
                                              G_CALLBACK (frame_clock_update_cb),
                                              self);
  priv->layout_handler_id = g_signal_connect (surface, "layout",
                                              G_CALLBACK (surface_layout_cb),
                                              self);

  priv->scale_changed_handler_id = g_signal_connect (surface, "notify::scale-factor",
                                                     G_CALLBACK (scale_changed_cb),
                                                     self);

  priv->enter_monitor_handler_id = g_signal_connect (surface, "enter-monitor",
                                                     G_CALLBACK (monitor_changed_cb),
                                                     self);
  priv->leave_monitor_handler_id = g_signal_connect (surface, "leave-monitor",
                                                     G_CALLBACK (monitor_changed_cb),
                                                     self);

  g_object_set_qdata_full (G_OBJECT (self),
                           quark_bobgui_native_private,
                           priv,
                           verify_priv_unrealized);
}

/**
 * bobgui_native_unrealize:
 * @self: a `BobguiNative`
 *
 * Unrealizes a `BobguiNative`.
 *
 * This should only be used by subclasses.
 */
void
bobgui_native_unrealize (BobguiNative *self)
{
  BobguiNativePrivate *priv;
  GdkSurface *surface;
  GdkFrameClock *clock;

  priv = g_object_get_qdata (G_OBJECT (self), quark_bobgui_native_private);
  g_return_if_fail (priv != NULL);

  surface = bobgui_native_get_surface (self);
  clock = gdk_surface_get_frame_clock (surface);
  g_return_if_fail (clock != NULL);

  g_clear_signal_handler (&priv->update_handler_id, clock);
  g_clear_signal_handler (&priv->layout_handler_id, surface);
  g_clear_signal_handler (&priv->scale_changed_handler_id, surface);
  g_clear_signal_handler (&priv->enter_monitor_handler_id, surface);
  g_clear_signal_handler (&priv->leave_monitor_handler_id, surface);

  g_object_set_qdata (G_OBJECT (self), quark_bobgui_native_private, NULL);
}

/**
 * bobgui_native_get_surface:
 * @self: a `BobguiNative`
 *
 * Returns the surface of this `BobguiNative`.
 *
 * Returns: (transfer none) (nullable): the surface of @self
 */
GdkSurface *
bobgui_native_get_surface (BobguiNative *self)
{
  g_return_val_if_fail (BOBGUI_IS_NATIVE (self), NULL);

  return BOBGUI_NATIVE_GET_IFACE (self)->get_surface (self);
}

/**
 * bobgui_native_get_renderer:
 * @self: a `BobguiNative`
 *
 * Returns the renderer that is used for this `BobguiNative`.
 *
 * Returns: (transfer none) (nullable): the renderer for @self
 */
GskRenderer *
bobgui_native_get_renderer (BobguiNative *self)
{
  g_return_val_if_fail (BOBGUI_IS_NATIVE (self), NULL);

  return BOBGUI_NATIVE_GET_IFACE (self)->get_renderer (self);
}

/**
 * bobgui_native_get_surface_transform:
 * @self: a `BobguiNative`
 * @x: (out): return location for the x coordinate
 * @y: (out): return location for the y coordinate
 *
 * Retrieves the surface transform of @self.
 *
 * This is the translation from @self's surface coordinates into
 * @self's widget coordinates.
 */
void
bobgui_native_get_surface_transform (BobguiNative *self,
                                  double    *x,
                                  double    *y)
{
  g_return_if_fail (BOBGUI_IS_NATIVE (self));
  g_return_if_fail (x != NULL);
  g_return_if_fail (y != NULL);

  BOBGUI_NATIVE_GET_IFACE (self)->get_surface_transform (self, x, y);
}

/**
 * bobgui_native_get_for_surface:
 * @surface: a `GdkSurface`
 *
 * Finds the `BobguiNative` associated with the surface.
 *
 * Returns: (transfer none) (nullable): the `BobguiNative` that is associated with @surface
 */
BobguiNative *
bobgui_native_get_for_surface (GdkSurface *surface)
{
  BobguiWidget *widget;

  widget = (BobguiWidget *)gdk_surface_get_widget (surface);

  if (widget && BOBGUI_IS_NATIVE (widget))
    return BOBGUI_NATIVE (widget);

  return NULL;
}

void
bobgui_native_queue_relayout (BobguiNative *self)
{
  BobguiWidget *widget = BOBGUI_WIDGET (self);
  GdkSurface *surface;
  GdkFrameClock *clock;

  surface = bobgui_widget_get_surface (widget);
  clock = bobgui_widget_get_frame_clock (widget);
  if (clock == NULL)
    return;

  gdk_frame_clock_request_phase (clock, GDK_FRAME_CLOCK_PHASE_UPDATE);
  gdk_surface_request_layout (surface);
}

