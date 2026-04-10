/* BOBGUI - The Bobgui Framework
 *
 * Copyright (C) 2023  Red Hat, Inc.
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
 *
 * Author:
 *      Matthias Clasen <mclasen@redhat.com>
 */

#include "config.h"

#include "bobguibinlayout.h"
#include "bobguigraphicsoffload.h"
#include "bobguisnapshotprivate.h"
#include "bobguiwidgetprivate.h"
#include "bobguiprivate.h"
#include "gdk/gdksurfaceprivate.h"
#include "gdk/gdksubsurfaceprivate.h"
#include "gdk/gdkrgbaprivate.h"
#include "bobguitypebuiltins.h"

/**
 * BobguiGraphicsOffload:
 *
 * Bypasses gsk rendering by passing the content of its child directly to the compositor.
 *
 * Graphics offload is an optimization to reduce overhead and battery use that is
 * most useful for video content. It only works on some platforms and in certain
 * situations. BOBGUI will automatically fall back to normal rendering if it doesn't.
 *
 * Graphics offload is most efficient if there are no controls drawn on top of the
 * video content.
 *
 * You should consider using graphics offload for your main widget if it shows
 * frequently changing content (such as a video, or a VM display) and you provide
 * the content in the form of dmabuf textures (see [class@Gdk.DmabufTextureBuilder]),
 * in particular if it may be fullscreen.
 *
 * Numerous factors can prohibit graphics offload:
 *
 * - Unsupported platforms. Currently, graphics offload only works on Linux with Wayland.
 *
 * - Clipping, such as rounded corners that cause the video content to not be rectangular
 *
 * - Unsupported dmabuf formats (see [method@Gdk.Display.get_dmabuf_formats])
 *
 * - Translucent video content (content with an alpha channel, even if it isn't used)
 *
 * - Transforms that are more complex than translations and scales
 *
 * - Filters such as opacity, grayscale or similar
 *
 * To investigate problems related graphics offload, BOBGUI offers debug flags to print
 * out information about graphics offload and dmabuf use:
 *
 *     GDK_DEBUG=offload
 *     GDK_DEBUG=dmabuf
 *
 * The BOBGUI inspector provides a visual debugging tool for graphics offload.
 *
 * Since: 4.14
 */

struct _BobguiGraphicsOffload
{
  BobguiWidget parent_instance;

  BobguiWidget *child;

  GdkSubsurface *subsurface;

  BobguiGraphicsOffloadEnabled enabled;
  gboolean black_background;
};

struct _BobguiGraphicsOffloadClass
{
  BobguiWidgetClass parent_class;
};

enum
{
  PROP_0,
  PROP_CHILD,
  PROP_ENABLED,
  PROP_BLACK_BACKGROUND,
  LAST_PROP,
};

static GParamSpec *properties[LAST_PROP] = { NULL, };

G_DEFINE_TYPE (BobguiGraphicsOffload, bobgui_graphics_offload, BOBGUI_TYPE_WIDGET)

static void
bobgui_graphics_offload_init (BobguiGraphicsOffload *self)
{
  self->enabled = BOBGUI_GRAPHICS_OFFLOAD_ENABLED;
}

static void
bobgui_graphics_offload_dispose (GObject *object)
{
  BobguiGraphicsOffload *self = BOBGUI_GRAPHICS_OFFLOAD (object);

  g_clear_pointer (&self->child, bobgui_widget_unparent);

  G_OBJECT_CLASS (bobgui_graphics_offload_parent_class)->dispose (object);
}

static void
bobgui_graphics_offload_set_property (GObject      *object,
                                   guint         property_id,
                                   const GValue *value,
                                   GParamSpec   *pspec)
{
  BobguiGraphicsOffload *self = BOBGUI_GRAPHICS_OFFLOAD (object);

  switch (property_id)
    {
    case PROP_CHILD:
      bobgui_graphics_offload_set_child (self, g_value_get_object (value));
      break;

    case PROP_ENABLED:
      bobgui_graphics_offload_set_enabled (self, g_value_get_enum (value));
      break;

    case PROP_BLACK_BACKGROUND:
      bobgui_graphics_offload_set_black_background (self, g_value_get_boolean (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
bobgui_graphics_offload_get_property (GObject    *object,
                                   guint       property_id,
                                   GValue     *value,
                                   GParamSpec *pspec)
{
  BobguiGraphicsOffload *self = BOBGUI_GRAPHICS_OFFLOAD (object);

  switch (property_id)
    {
    case PROP_CHILD:
      g_value_set_object (value, bobgui_graphics_offload_get_child (self));
      break;

    case PROP_ENABLED:
      g_value_set_enum (value, bobgui_graphics_offload_get_enabled (self));
      break;

    case PROP_BLACK_BACKGROUND:
      g_value_set_boolean (value, bobgui_graphics_offload_get_black_background (self));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
sync_subsurface (BobguiGraphicsOffload *self)
{
  BobguiWidget *widget = BOBGUI_WIDGET (self);

  if (bobgui_widget_get_realized (widget) && self->enabled != BOBGUI_GRAPHICS_OFFLOAD_DISABLED)
    {
      if (!self->subsurface)
        self->subsurface = gdk_surface_create_subsurface (bobgui_widget_get_surface (widget));
    }
  else
    {
      g_clear_object (&self->subsurface);
    }
}

static void
bobgui_graphics_offload_realize (BobguiWidget *widget)
{
  BobguiGraphicsOffload *self = BOBGUI_GRAPHICS_OFFLOAD (widget);

  BOBGUI_WIDGET_CLASS (bobgui_graphics_offload_parent_class)->realize (widget);

  sync_subsurface (self);
}

static void
bobgui_graphics_offload_unrealize (BobguiWidget *widget)
{
  BobguiGraphicsOffload *self = BOBGUI_GRAPHICS_OFFLOAD (widget);

  BOBGUI_WIDGET_CLASS (bobgui_graphics_offload_parent_class)->unrealize (widget);

  sync_subsurface (self);
}

static void
bobgui_graphics_offload_snapshot (BobguiWidget   *widget,
                               BobguiSnapshot *snapshot)
{
  BobguiGraphicsOffload *self = BOBGUI_GRAPHICS_OFFLOAD (widget);

  if (self->subsurface)
    bobgui_snapshot_push_subsurface (snapshot, self->subsurface);

  if (self->black_background)
    bobgui_snapshot_append_color (snapshot,
                               &GDK_RGBA_BLACK,
                               &GRAPHENE_RECT_INIT (0, 0,
                                                    bobgui_widget_get_width (widget),
                                                    bobgui_widget_get_height (widget)));

  if (self->child)
    bobgui_widget_snapshot_child (widget, self->child, snapshot);

  if (self->subsurface)
    bobgui_snapshot_pop (snapshot);
}

static void
bobgui_graphics_offload_class_init (BobguiGraphicsOffloadClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (class);

  object_class->dispose = bobgui_graphics_offload_dispose;
  object_class->set_property = bobgui_graphics_offload_set_property;
  object_class->get_property = bobgui_graphics_offload_get_property;

  widget_class->realize = bobgui_graphics_offload_realize;
  widget_class->unrealize = bobgui_graphics_offload_unrealize;
  widget_class->snapshot = bobgui_graphics_offload_snapshot;

  /**
   * BobguiGraphicsOffload:child:
   *
   * The child widget.
   *
   * Since: 4.14
   */
  properties[PROP_CHILD] = g_param_spec_object ("child", NULL, NULL,
                                                BOBGUI_TYPE_WIDGET,
                                                BOBGUI_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  /**
   * BobguiGraphicsOffload:enabled:
   *
   * Whether graphics offload is enabled.
   *
   * Since: 4.14
   */
  properties[PROP_ENABLED] = g_param_spec_enum ("enabled", NULL, NULL,
                                                BOBGUI_TYPE_GRAPHICS_OFFLOAD_ENABLED,
                                                BOBGUI_GRAPHICS_OFFLOAD_ENABLED,
                                                BOBGUI_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  /**
   * BobguiGraphicsOffload:black-background:
   *
   * Whether to draw a black background.
   *
   * Since: 4.16
   */
  properties[PROP_BLACK_BACKGROUND] = g_param_spec_boolean ("black-background", NULL, NULL,
                                                            FALSE,
                                                            BOBGUI_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, LAST_PROP, properties);

  bobgui_widget_class_set_layout_manager_type (widget_class, BOBGUI_TYPE_BIN_LAYOUT);
  bobgui_widget_class_set_css_name (widget_class, "graphicsoffload");
}

/**
 * bobgui_graphics_offload_new:
 * @child: (nullable): the child widget
 *
 * Creates a new BobguiGraphicsOffload widget.
 *
 * Returns: the new widget
 *
 * Since: 4.14
 */
BobguiWidget *
bobgui_graphics_offload_new (BobguiWidget *child)
{
  return g_object_new (BOBGUI_TYPE_GRAPHICS_OFFLOAD,
                       "child", child,
                       NULL);
}

/**
 * bobgui_graphics_offload_set_child:
 * @self: a `BobguiGraphicsOffload`
 * @child: (nullable): the child widget
 *
 * Sets the child of @self.
 *
 * Since: 4.14
 */
void
bobgui_graphics_offload_set_child (BobguiGraphicsOffload *self,
                                BobguiWidget           *child)
{
  g_return_if_fail (BOBGUI_IS_GRAPHICS_OFFLOAD (self));
  g_return_if_fail (child == NULL || self->child == child || (BOBGUI_IS_WIDGET (child) &&bobgui_widget_get_parent (child) == NULL));

  if (self->child == child)
    return;

  g_clear_pointer (&self->child, bobgui_widget_unparent);

  if (child)
    {
      self->child = child;
      bobgui_widget_set_parent (child, BOBGUI_WIDGET (self));
    }

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_CHILD]);
}

/**
 * bobgui_graphics_offload_get_child:
 * @self: a `BobguiGraphicsOffload`
 *
 * Gets the child of @self.
 *
 * Returns: (nullable) (transfer none): the child widget
 *
 * Since: 4.14
 */
BobguiWidget *
bobgui_graphics_offload_get_child (BobguiGraphicsOffload *self)
{
  g_return_val_if_fail (BOBGUI_IS_GRAPHICS_OFFLOAD (self), NULL);

  return self->child;
}

/**
 * bobgui_graphics_offload_set_enabled:
 * @self: a `BobguiGraphicsOffload`
 * @enabled: whether to enable offload
 *
 * Sets whether this BobguiGraphicsOffload widget will attempt
 * to offload the content of its child widget.
 *
 * Since: 4.14
 */
void
bobgui_graphics_offload_set_enabled (BobguiGraphicsOffload        *self,
                                  BobguiGraphicsOffloadEnabled  enabled)
{
  g_return_if_fail (BOBGUI_IS_GRAPHICS_OFFLOAD (self));

  if (self->enabled == enabled)
    return;

  self->enabled = enabled;

  sync_subsurface (self);
  bobgui_widget_queue_draw (BOBGUI_WIDGET (self));

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_ENABLED]);
}

/**
 * bobgui_graphics_offload_get_enabled:
 * @self: a `BobguiGraphicsOffload`
 *
 * Returns whether offload is enabled for @self.
 *
 * Returns: whether offload is enabled
 *
 * Since: 4.14
 */
BobguiGraphicsOffloadEnabled
bobgui_graphics_offload_get_enabled (BobguiGraphicsOffload *self)
{
  g_return_val_if_fail (BOBGUI_IS_GRAPHICS_OFFLOAD (self), TRUE);

  return self->enabled;
}

/**
 * bobgui_graphics_offload_set_black_background:
 * @self: a `BobguiGraphicsOffload`
 * @value: whether to draw a black background behind the content
 *
 * Sets whether this BobguiGraphicsOffload widget will draw a black
 * background.
 *
 * A main use case for this is **_letterboxing_** where black bars are
 * visible next to the content if the aspect ratio of the content does
 * not match the dimensions of the monitor.
 *
 * Using this property for letterboxing instead of CSS allows compositors
 * to show content with maximum efficiency, using direct scanout to avoid
 * extra copies in the compositor.
 *
 * On Wayland, this is implemented using the
 * [single-pixel buffer](https://wayland.app/protocols/single-pixel-buffer-v1)
 * protocol.
 *
 * Since: 4.16
 */
void
bobgui_graphics_offload_set_black_background (BobguiGraphicsOffload *self,
                                           gboolean            value)
{
  g_return_if_fail (BOBGUI_IS_GRAPHICS_OFFLOAD (self));

  if (self->black_background == value)
    return;

  self->black_background = value;

  bobgui_widget_queue_draw (BOBGUI_WIDGET (self));

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_BLACK_BACKGROUND]);
}

/**
 * bobgui_graphics_offload_get_black_background:
 * @self: a `BobguiGraphicsOffload`
 *
 * Returns whether the widget draws a black background.
 *
 * See [method@Bobgui.GraphicsOffload.set_black_background].
 *
 * Returns: `TRUE` if black background is drawn
 *
 * Since: 4.16
 */
gboolean
bobgui_graphics_offload_get_black_background (BobguiGraphicsOffload *self)
{
  g_return_val_if_fail (BOBGUI_IS_GRAPHICS_OFFLOAD (self), FALSE);

  return self->black_background;
}
