/* BOBGUI - The Bobgui Framework
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
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

/*
 * Modified by the BOBGUI Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the BOBGUI Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * BOBGUI at ftp://ftp.bobgui.org/pub/bobgui/.
 */

#include "config.h"
#include "bobguidrawingarea.h"
#include "bobguimarshalers.h"
#include "gdk/gdkmarshalers.h"
#include "bobguiprivate.h"
#include "bobguisnapshot.h"
#include "bobguiwidgetprivate.h"

typedef struct _BobguiDrawingAreaPrivate BobguiDrawingAreaPrivate;

struct _BobguiDrawingAreaPrivate {
  int content_width;
  int content_height;

  BobguiDrawingAreaDrawFunc draw_func;
  gpointer draw_func_target;
  GDestroyNotify draw_func_target_destroy_notify;
};

enum {
  PROP_0,
  PROP_CONTENT_WIDTH,
  PROP_CONTENT_HEIGHT,
  LAST_PROP
};

static GParamSpec *props[LAST_PROP] = { NULL, };

enum {
  RESIZE,
  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0, };

/**
 * BobguiDrawingArea:
 *
 * Allows drawing with cairo.
 *
 * <picture>
 *   <source srcset="drawingarea-dark.png" media="(prefers-color-scheme: dark)">
 *   <img alt="An example BobguiDrawingArea" src="drawingarea.png">
 * </picture>
 *
 * It’s essentially a blank widget; you can draw on it. After
 * creating a drawing area, the application may want to connect to:
 *
 * - The [signal@Bobgui.Widget::realize] signal to take any necessary actions
 *   when the widget is instantiated on a particular display.
 *   (Create GDK resources in response to this signal.)
 *
 * - The [signal@Bobgui.DrawingArea::resize] signal to take any necessary
 *   actions when the widget changes size.
 *
 * - Call [method@Bobgui.DrawingArea.set_draw_func] to handle redrawing the
 *   contents of the widget.
 *
 * The following code portion demonstrates using a drawing
 * area to display a circle in the normal widget foreground
 * color.
 *
 * ## Simple BobguiDrawingArea usage
 *
 * ```c
 * static void
 * draw_function (BobguiDrawingArea *area,
 *                cairo_t        *cr,
 *                int             width,
 *                int             height,
 *                gpointer        data)
 * {
 *   GdkRGBA color;
 *
 *   cairo_arc (cr,
 *              width / 2.0, height / 2.0,
 *              MIN (width, height) / 2.0,
 *              0, 2 * G_PI);
 *
 *   bobgui_widget_get_color (BOBGUI_WIDGET (area),
 *                         &color);
 *   gdk_cairo_set_source_rgba (cr, &color);
 *
 *   cairo_fill (cr);
 * }
 *
 * int
 * main (int argc, char **argv)
 * {
 *   bobgui_init ();
 *
 *   BobguiWidget *area = bobgui_drawing_area_new ();
 *   bobgui_drawing_area_set_content_width (BOBGUI_DRAWING_AREA (area), 100);
 *   bobgui_drawing_area_set_content_height (BOBGUI_DRAWING_AREA (area), 100);
 *   bobgui_drawing_area_set_draw_func (BOBGUI_DRAWING_AREA (area),
 *                                   draw_function,
 *                                   NULL, NULL);
 *   return 0;
 * }
 * ```
 *
 * The draw function is normally called when a drawing area first comes
 * onscreen, or when it’s covered by another window and then uncovered.
 * You can also force a redraw by adding to the “damage region” of the
 * drawing area’s window using [method@Bobgui.Widget.queue_draw].
 * This will cause the drawing area to call the draw function again.
 *
 * The available routines for drawing are documented in the
 * [Cairo documentation](https://www.cairographics.org/manual/); GDK
 * offers additional API to integrate with Cairo, like [func@Gdk.cairo_set_source_rgba]
 * or [func@Gdk.cairo_set_source_pixbuf].
 *
 * To receive mouse events on a drawing area, you will need to use
 * event controllers. To receive keyboard events, you will need to set
 * the “can-focus” property on the drawing area, and you should probably
 * draw some user-visible indication that the drawing area is focused.
 *
 * If you need more complex control over your widget, you should consider
 * creating your own `BobguiWidget` subclass.
 */

G_DEFINE_TYPE_WITH_PRIVATE (BobguiDrawingArea, bobgui_drawing_area, BOBGUI_TYPE_WIDGET)

static void
bobgui_drawing_area_set_property (GObject      *gobject,
                               guint         prop_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  BobguiDrawingArea *self = BOBGUI_DRAWING_AREA (gobject);

  switch (prop_id)
    {
    case PROP_CONTENT_WIDTH:
      bobgui_drawing_area_set_content_width (self, g_value_get_int (value));
      break;

    case PROP_CONTENT_HEIGHT:
      bobgui_drawing_area_set_content_height (self, g_value_get_int (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
    }
}

static void
bobgui_drawing_area_get_property (GObject    *gobject,
                               guint       prop_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  BobguiDrawingArea *self = BOBGUI_DRAWING_AREA (gobject);
  BobguiDrawingAreaPrivate *priv = bobgui_drawing_area_get_instance_private (self);

  switch (prop_id)
    {
    case PROP_CONTENT_WIDTH:
      g_value_set_int (value, priv->content_width);
      break;

    case PROP_CONTENT_HEIGHT:
      g_value_set_int (value, priv->content_height);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
    }
}

static void
bobgui_drawing_area_dispose (GObject *object)
{
  BobguiDrawingArea *self = BOBGUI_DRAWING_AREA (object);
  BobguiDrawingAreaPrivate *priv = bobgui_drawing_area_get_instance_private (self);

  if (priv->draw_func_target_destroy_notify != NULL)
    priv->draw_func_target_destroy_notify (priv->draw_func_target);

  priv->draw_func = NULL;
  priv->draw_func_target = NULL;
  priv->draw_func_target_destroy_notify = NULL;

  G_OBJECT_CLASS (bobgui_drawing_area_parent_class)->dispose (object);
}

static void
bobgui_drawing_area_measure (BobguiWidget      *widget,
                          BobguiOrientation  orientation,
                          int             for_size,
                          int            *minimum,
                          int            *natural,
                          int            *minimum_baseline,
                          int            *natural_baseline)
{
  BobguiDrawingArea *self = BOBGUI_DRAWING_AREA (widget);
  BobguiDrawingAreaPrivate *priv = bobgui_drawing_area_get_instance_private (self);

  if (orientation == BOBGUI_ORIENTATION_HORIZONTAL)
    {
      *minimum = *natural = priv->content_width;
    }
  else
    {
      *minimum = *natural = priv->content_height;
    }
}

static void
bobgui_drawing_area_size_allocate (BobguiWidget *widget,
                                int        width,
                                int        height,
                                int        baseline)
{
  g_signal_emit (widget, signals[RESIZE], 0, width, height);
}

static void
bobgui_drawing_area_snapshot (BobguiWidget   *widget,
                           BobguiSnapshot *snapshot)
{
  BobguiDrawingArea *self = BOBGUI_DRAWING_AREA (widget);
  BobguiDrawingAreaPrivate *priv = bobgui_drawing_area_get_instance_private (self);
  cairo_t *cr;
  int width, height;

  if (!priv->draw_func)
    return;

  width = bobgui_widget_get_width (widget);
  height = bobgui_widget_get_height (widget);


  cr = bobgui_snapshot_append_cairo (snapshot,
                                  &GRAPHENE_RECT_INIT (
                                      0, 0,
                                      width, height
                                  ));
  priv->draw_func (self,
                   cr,
                   width, height,
                   priv->draw_func_target);
  cairo_destroy (cr);
}

static void
bobgui_drawing_area_class_init (BobguiDrawingAreaClass *class)
{
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (class);
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);

  gobject_class->set_property = bobgui_drawing_area_set_property;
  gobject_class->get_property = bobgui_drawing_area_get_property;
  gobject_class->dispose = bobgui_drawing_area_dispose;

  widget_class->measure = bobgui_drawing_area_measure;
  widget_class->size_allocate = bobgui_drawing_area_size_allocate;
  widget_class->snapshot = bobgui_drawing_area_snapshot;

  /**
   * BobguiDrawingArea:content-width:
   *
   * The content width.
   */
  props[PROP_CONTENT_WIDTH] =
    g_param_spec_int ("content-width", NULL, NULL,
                      0, G_MAXINT, 0,
                      BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiDrawingArea:content-height:
   *
   * The content height.
   */
  props[PROP_CONTENT_HEIGHT] =
    g_param_spec_int ("content-height", NULL, NULL,
                      0, G_MAXINT, 0,
                      BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (gobject_class, LAST_PROP, props);

  /**
   * BobguiDrawingArea::resize:
   * @area: the `BobguiDrawingArea` that emitted the signal
   * @width: the width of the viewport
   * @height: the height of the viewport
   *
   * Emitted once when the widget is realized, and then each time the widget
   * is changed while realized.
   *
   * This is useful in order to keep state up to date with the widget size,
   * like for instance a backing surface.
   */
  signals[RESIZE] =
    g_signal_new (I_("resize"),
                  G_TYPE_FROM_CLASS (class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (BobguiDrawingAreaClass, resize),
                  NULL, NULL,
                  _gdk_marshal_VOID__INT_INT,
                  G_TYPE_NONE, 2, G_TYPE_INT, G_TYPE_INT);
  g_signal_set_va_marshaller (signals[RESIZE],
                              G_TYPE_FROM_CLASS (class),
                              _gdk_marshal_VOID__INT_INTv);
}

static void
bobgui_drawing_area_init (BobguiDrawingArea *darea)
{
  bobgui_widget_set_focusable (BOBGUI_WIDGET (darea), FALSE);
}

/**
 * bobgui_drawing_area_new:
 *
 * Creates a new drawing area.
 *
 * Returns: a new `BobguiDrawingArea`
 */
BobguiWidget*
bobgui_drawing_area_new (void)
{
  return g_object_new (BOBGUI_TYPE_DRAWING_AREA, NULL);
}

/**
 * bobgui_drawing_area_set_content_width:
 * @self: a `BobguiDrawingArea`
 * @width: the width of contents
 *
 * Sets the desired width of the contents of the drawing area.
 *
 * Note that because widgets may be allocated larger sizes than they
 * requested, it is possible that the actual width passed to your draw
 * function is larger than the width set here. You can use
 * [method@Bobgui.Widget.set_halign] to avoid that.
 *
 * If the width is set to 0 (the default), the drawing area may disappear.
 */
void
bobgui_drawing_area_set_content_width (BobguiDrawingArea *self,
                                    int             width)
{
  BobguiDrawingAreaPrivate *priv = bobgui_drawing_area_get_instance_private (self);

  g_return_if_fail (BOBGUI_IS_DRAWING_AREA (self));
  g_return_if_fail (width >= 0);

  if (priv->content_width == width)
    return;

  priv->content_width = width;

  bobgui_widget_queue_resize (BOBGUI_WIDGET (self));
  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_CONTENT_WIDTH]);
}

/**
 * bobgui_drawing_area_get_content_width:
 * @self: a `BobguiDrawingArea`
 *
 * Retrieves the content width of the `BobguiDrawingArea`.
 *
 * Returns: The width requested for content of the drawing area
 */
int
bobgui_drawing_area_get_content_width (BobguiDrawingArea *self)
{
  BobguiDrawingAreaPrivate *priv = bobgui_drawing_area_get_instance_private (self);

  g_return_val_if_fail (BOBGUI_IS_DRAWING_AREA (self), 0);

  return priv->content_width;
}

/**
 * bobgui_drawing_area_set_content_height:
 * @self: a `BobguiDrawingArea`
 * @height: the height of contents
 *
 * Sets the desired height of the contents of the drawing area.
 *
 * Note that because widgets may be allocated larger sizes than they
 * requested, it is possible that the actual height passed to your draw
 * function is larger than the height set here. You can use
 * [method@Bobgui.Widget.set_valign] to avoid that.
 *
 * If the height is set to 0 (the default), the drawing area may disappear.
 */
void
bobgui_drawing_area_set_content_height (BobguiDrawingArea *self,
                                     int             height)
{
  BobguiDrawingAreaPrivate *priv = bobgui_drawing_area_get_instance_private (self);

  g_return_if_fail (BOBGUI_IS_DRAWING_AREA (self));
  g_return_if_fail (height >= 0);

  if (priv->content_height == height)
    return;

  priv->content_height = height;

  bobgui_widget_queue_resize (BOBGUI_WIDGET (self));
  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_CONTENT_HEIGHT]);
}

/**
 * bobgui_drawing_area_get_content_height:
 * @self: a `BobguiDrawingArea`
 *
 * Retrieves the content height of the `BobguiDrawingArea`.
 *
 * Returns: The height requested for content of the drawing area
 */
int
bobgui_drawing_area_get_content_height (BobguiDrawingArea *self)
{
  BobguiDrawingAreaPrivate *priv = bobgui_drawing_area_get_instance_private (self);

  g_return_val_if_fail (BOBGUI_IS_DRAWING_AREA (self), 0);

  return priv->content_height;
}

/**
 * bobgui_drawing_area_set_draw_func:
 * @self: a `BobguiDrawingArea`
 * @draw_func: (nullable) (scope notified) (closure user_data) (destroy destroy): callback
 *   that lets you draw the drawing area's contents
 * @user_data: user data passed to @draw_func
 * @destroy: destroy notifier for @user_data
 *
 * Setting a draw function is the main thing you want to do when using
 * a drawing area.
 *
 * The draw function is called whenever BOBGUI needs to draw the contents
 * of the drawing area to the screen.
 *
 * The draw function will be called during the drawing stage of BOBGUI.
 * In the drawing stage it is not allowed to change properties of any
 * BOBGUI widgets or call any functions that would cause any properties
 * to be changed. You should restrict yourself exclusively to drawing
 * your contents in the draw function.
 *
 * If what you are drawing does change, call [method@Bobgui.Widget.queue_draw]
 * on the drawing area. This will cause a redraw and will call @draw_func again.
 */
void
bobgui_drawing_area_set_draw_func (BobguiDrawingArea         *self,
                                BobguiDrawingAreaDrawFunc  draw_func,
                                gpointer                user_data,
                                GDestroyNotify          destroy)
{
  BobguiDrawingAreaPrivate *priv = bobgui_drawing_area_get_instance_private (self);

  g_return_if_fail (BOBGUI_IS_DRAWING_AREA (self));

  if (priv->draw_func_target_destroy_notify != NULL)
    priv->draw_func_target_destroy_notify (priv->draw_func_target);

  priv->draw_func = draw_func;
  priv->draw_func_target = user_data;
  priv->draw_func_target_destroy_notify = destroy;

  bobgui_widget_queue_draw (BOBGUI_WIDGET (self));
}
