/* BOBGUI - The Bobgui Framework
 * Copyright (C) 2010 Carlos Garnacho <carlosg@gnome.org>
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

#include "bobguirender.h"

#include <math.h>

#include "bobguicsscornervalueprivate.h"
#include "bobguicssimagevalueprivate.h"
#include "bobguicssnumbervalueprivate.h"
#include "bobguicsscolorvalueprivate.h"
#include "bobguicssshadowvalueprivate.h"
#include "bobguicsstransformvalueprivate.h"
#include "bobguirendericonprivate.h"
#include "bobguirenderborderprivate.h"
#include "bobguirenderbackgroundprivate.h"
#include "bobguirenderlayoutprivate.h"
#include "bobguistylecontextprivate.h"
#include "bobguisettings.h"

#include "gsk/gskroundedrectprivate.h"
#include <gdk/gdktextureprivate.h>

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

static void
bobgui_do_render_icon (BobguiStyleContext        *context,
                    cairo_t                *cr,
                    double                  x,
                    double                  y,
                    double                  width,
                    double                  height)
{
  BobguiSnapshot *snapshot;
  GskRenderNode *node;

  snapshot = bobgui_snapshot_new ();
  bobgui_css_style_snapshot_icon (bobgui_style_context_lookup_style (context), snapshot, width, height);
  node = bobgui_snapshot_free_to_node (snapshot);
  if (node == NULL)
    return;

  cairo_save (cr);
  cairo_translate (cr, x, y);
  gsk_render_node_draw (node, cr);
  cairo_restore (cr);

  gsk_render_node_unref (node);
}

/**
 * bobgui_render_check:
 * @context: a `BobguiStyleContext`
 * @cr: a `cairo_t`
 * @x: X origin of the rectangle
 * @y: Y origin of the rectangle
 * @width: rectangle width
 * @height: rectangle height
 *
 * Renders a checkmark (as in a `BobguiCheckButton`).
 *
 * The %BOBGUI_STATE_FLAG_CHECKED state determines whether the check is
 * on or off, and %BOBGUI_STATE_FLAG_INCONSISTENT determines whether it
 * should be marked as undefined.
 *
 * Typical checkmark rendering:
 *
 * ![](checks.png)
 *
 * Deprecated: 4.10
 **/
void
bobgui_render_check (BobguiStyleContext *context,
                  cairo_t         *cr,
                  double           x,
                  double           y,
                  double           width,
                  double           height)
{
  g_return_if_fail (BOBGUI_IS_STYLE_CONTEXT (context));
  g_return_if_fail (cr != NULL);

  if (width <= 0 || height <= 0)
    return;

  bobgui_do_render_icon (context, cr, x, y, width, height);
}

/**
 * bobgui_render_option:
 * @context: a `BobguiStyleContext`
 * @cr: a `cairo_t`
 * @x: X origin of the rectangle
 * @y: Y origin of the rectangle
 * @width: rectangle width
 * @height: rectangle height
 *
 * Renders an option mark (as in a radio button), the %BOBGUI_STATE_FLAG_CHECKED
 * state will determine whether the option is on or off, and
 * %BOBGUI_STATE_FLAG_INCONSISTENT whether it should be marked as undefined.
 *
 * Typical option mark rendering:
 *
 * ![](options.png)
 *
 * Deprecated: 4.10
 **/
void
bobgui_render_option (BobguiStyleContext *context,
                   cairo_t         *cr,
                   double           x,
                   double           y,
                   double           width,
                   double           height)
{
  g_return_if_fail (BOBGUI_IS_STYLE_CONTEXT (context));
  g_return_if_fail (cr != NULL);

  if (width <= 0 || height <= 0)
    return;

  bobgui_do_render_icon (context, cr, x, y, width, height);
}

/**
 * bobgui_render_arrow:
 * @context: a `BobguiStyleContext`
 * @cr: a `cairo_t`
 * @angle: arrow angle from 0 to 2 * %G_PI, being 0 the arrow pointing to the north
 * @x: X origin of the render area
 * @y: Y origin of the render area
 * @size: square side for render area
 *
 * Renders an arrow pointing to @angle.
 *
 * Typical arrow rendering at 0, 1⁄2 π;, π; and 3⁄2 π:
 *
 * ![](arrows.png)
 *
 * Deprecated: 4.10
 **/
void
bobgui_render_arrow (BobguiStyleContext *context,
                  cairo_t         *cr,
                  double           angle,
                  double           x,
                  double           y,
                  double           size)
{
  g_return_if_fail (BOBGUI_IS_STYLE_CONTEXT (context));
  g_return_if_fail (cr != NULL);

  if (size <= 0)
    return;

  bobgui_do_render_icon (context, cr, x, y, size, size);
}

/**
 * bobgui_render_background:
 * @context: a `BobguiStyleContext`
 * @cr: a `cairo_t`
 * @x: X origin of the rectangle
 * @y: Y origin of the rectangle
 * @width: rectangle width
 * @height: rectangle height
 *
 * Renders the background of an element.
 *
 * Typical background rendering, showing the effect of
 * `background-image`, `border-width` and `border-radius`:
 *
 * ![](background.png)
 *
 * Deprecated: 4.10
 **/
void
bobgui_render_background (BobguiStyleContext *context,
                       cairo_t         *cr,
                       double           x,
                       double           y,
                       double           width,
                       double           height)
{
  BobguiSnapshot *snapshot;
  GskRenderNode *node;

  g_return_if_fail (BOBGUI_IS_STYLE_CONTEXT (context));
  g_return_if_fail (cr != NULL);

  if (width <= 0 || height <= 0)
    return;

  snapshot = bobgui_snapshot_new ();
  bobgui_snapshot_render_background (snapshot, context, x, y, width, height);
  node = bobgui_snapshot_free_to_node (snapshot);
  if (node == NULL)
    return;

  cairo_save (cr);
  gsk_render_node_draw (node, cr);
  cairo_restore (cr);

  gsk_render_node_unref (node);
}

/**
 * bobgui_render_frame:
 * @context: a `BobguiStyleContext`
 * @cr: a `cairo_t`
 * @x: X origin of the rectangle
 * @y: Y origin of the rectangle
 * @width: rectangle width
 * @height: rectangle height
 *
 * Renders a frame around the rectangle defined by @x, @y, @width, @height.
 *
 * Examples of frame rendering, showing the effect of `border-image`,
 * `border-color`, `border-width`, `border-radius` and junctions:
 *
 * ![](frames.png)
 *
 * Deprecated: 4.10
 **/
void
bobgui_render_frame (BobguiStyleContext *context,
                  cairo_t         *cr,
                  double           x,
                  double           y,
                  double           width,
                  double           height)
{
  BobguiSnapshot *snapshot;
  GskRenderNode *node;

  g_return_if_fail (BOBGUI_IS_STYLE_CONTEXT (context));
  g_return_if_fail (cr != NULL);

  if (width <= 0 || height <= 0)
    return;

  snapshot = bobgui_snapshot_new ();
  bobgui_snapshot_render_frame (snapshot, context, x, y, width, height);
  node = bobgui_snapshot_free_to_node (snapshot);
  if (node == NULL)
    return;

  cairo_save (cr);
  gsk_render_node_draw (node, cr);
  cairo_restore (cr);

  gsk_render_node_unref (node);
}

/**
 * bobgui_render_expander:
 * @context: a `BobguiStyleContext`
 * @cr: a `cairo_t`
 * @x: X origin of the rectangle
 * @y: Y origin of the rectangle
 * @width: rectangle width
 * @height: rectangle height
 *
 * Renders an expander (as used in `BobguiTreeView` and `BobguiExpander`) in the area
 * defined by @x, @y, @width, @height. The state %BOBGUI_STATE_FLAG_CHECKED
 * determines whether the expander is collapsed or expanded.
 *
 * Typical expander rendering:
 *
 * ![](expanders.png)
 *
 * Deprecated: 4.10
 **/
void
bobgui_render_expander (BobguiStyleContext *context,
                     cairo_t         *cr,
                     double           x,
                     double           y,
                     double           width,
                     double           height)
{
  g_return_if_fail (BOBGUI_IS_STYLE_CONTEXT (context));
  g_return_if_fail (cr != NULL);

  if (width <= 0 || height <= 0)
    return;

  bobgui_do_render_icon (context, cr, x, y, width, height);
}

/**
 * bobgui_render_focus:
 * @context: a `BobguiStyleContext`
 * @cr: a `cairo_t`
 * @x: X origin of the rectangle
 * @y: Y origin of the rectangle
 * @width: rectangle width
 * @height: rectangle height
 *
 * Renders a focus indicator on the rectangle determined by @x, @y, @width, @height.
 *
 * Typical focus rendering:
 *
 * ![](focus.png)
 *
 * Deprecated: 4.10
 **/
void
bobgui_render_focus (BobguiStyleContext *context,
                  cairo_t         *cr,
                  double           x,
                  double           y,
                  double           width,
                  double           height)
{
  BobguiSnapshot *snapshot;
  GskRenderNode *node;

  g_return_if_fail (BOBGUI_IS_STYLE_CONTEXT (context));
  g_return_if_fail (cr != NULL);

  if (width <= 0 || height <= 0)
    return;

  snapshot = bobgui_snapshot_new ();
  bobgui_snapshot_render_frame (snapshot, context, x, y, width, height);
  node = bobgui_snapshot_free_to_node (snapshot);
  if (node == NULL)
    return;

  cairo_save (cr);
  gsk_render_node_draw (node, cr);
  cairo_restore (cr);

  gsk_render_node_unref (node);
}

/**
 * bobgui_render_layout:
 * @context: a `BobguiStyleContext`
 * @cr: a `cairo_t`
 * @x: X origin
 * @y: Y origin
 * @layout: the `PangoLayout` to render
 *
 * Renders @layout on the coordinates @x, @y
 *
 * Deprecated: 4.10
 **/
void
bobgui_render_layout (BobguiStyleContext *context,
                   cairo_t         *cr,
                   double           x,
                   double           y,
                   PangoLayout     *layout)
{
  BobguiSnapshot *snapshot;
  GskRenderNode *node;

  g_return_if_fail (BOBGUI_IS_STYLE_CONTEXT (context));
  g_return_if_fail (PANGO_IS_LAYOUT (layout));
  g_return_if_fail (cr != NULL);

  snapshot = bobgui_snapshot_new ();
  bobgui_snapshot_render_layout (snapshot, context, x, y, layout); 
  node = bobgui_snapshot_free_to_node (snapshot);
  if (node == NULL)
    return;

  cairo_save (cr);
  gsk_render_node_draw (node, cr);
  cairo_restore (cr);

  gsk_render_node_unref (node);
}

/**
 * bobgui_render_line:
 * @context: a `BobguiStyleContext`
 * @cr: a `cairo_t`
 * @x0: X coordinate for the origin of the line
 * @y0: Y coordinate for the origin of the line
 * @x1: X coordinate for the end of the line
 * @y1: Y coordinate for the end of the line
 *
 * Renders a line from (x0, y0) to (x1, y1).
 *
 * Deprecated: 4.10
 **/
void
bobgui_render_line (BobguiStyleContext *context,
                 cairo_t         *cr,
                 double           x0,
                 double           y0,
                 double           x1,
                 double           y1)
{
  const GdkRGBA *color;

  g_return_if_fail (BOBGUI_IS_STYLE_CONTEXT (context));
  g_return_if_fail (cr != NULL);

  cairo_save (cr);

  color = bobgui_css_color_value_get_rgba (_bobgui_style_context_peek_property (context, BOBGUI_CSS_PROPERTY_COLOR));

  cairo_set_line_cap (cr, CAIRO_LINE_CAP_SQUARE);
  cairo_set_line_width (cr, 1);

  cairo_move_to (cr, x0 + 0.5, y0 + 0.5);
  cairo_line_to (cr, x1 + 0.5, y1 + 0.5);

  gdk_cairo_set_source_rgba (cr, color);
  cairo_stroke (cr);

  cairo_restore (cr);
}

/**
 * bobgui_render_handle:
 * @context: a `BobguiStyleContext`
 * @cr: a `cairo_t`
 * @x: X origin of the rectangle
 * @y: Y origin of the rectangle
 * @width: rectangle width
 * @height: rectangle height
 *
 * Renders a handle (as in `BobguiPaned` and `BobguiWindow`’s resize grip),
 * in the rectangle determined by @x, @y, @width, @height.
 *
 * Handles rendered for the paned and grip classes:
 *
 * ![](handles.png)
 *
 * Deprecated: 4.10
 **/
void
bobgui_render_handle (BobguiStyleContext *context,
                   cairo_t         *cr,
                   double           x,
                   double           y,
                   double           width,
                   double           height)
{
  g_return_if_fail (BOBGUI_IS_STYLE_CONTEXT (context));
  g_return_if_fail (cr != NULL);

  if (width <= 0 || height <= 0)
    return;

  bobgui_render_background (context, cr, x, y, width, height);
  bobgui_render_frame (context, cr, x, y, width, height);

  bobgui_do_render_icon (context, cr, x, y, width, height);
}

/**
 * bobgui_render_activity:
 * @context: a `BobguiStyleContext`
 * @cr: a `cairo_t`
 * @x: X origin of the rectangle
 * @y: Y origin of the rectangle
 * @width: rectangle width
 * @height: rectangle height
 *
 * Renders an activity indicator (such as in `BobguiSpinner`).
 * The state %BOBGUI_STATE_FLAG_CHECKED determines whether there is
 * activity going on.
 *
 * Deprecated: 4.10
 **/
void
bobgui_render_activity (BobguiStyleContext *context,
                     cairo_t         *cr,
                     double           x,
                     double           y,
                     double           width,
                     double           height)
{
  g_return_if_fail (BOBGUI_IS_STYLE_CONTEXT (context));
  g_return_if_fail (cr != NULL);

  if (width <= 0 || height <= 0)
    return;

  bobgui_do_render_icon (context, cr, x, y, width, height);
}

/**
 * bobgui_render_icon:
 * @context: a `BobguiStyleContext`
 * @cr: a `cairo_t`
 * @texture: a `GdkTexture` containing the icon to draw
 * @x: X position for the @texture
 * @y: Y position for the @texture
 *
 * Renders the icon in @texture at the specified @x and @y coordinates.
 *
 * This function will render the icon in @texture at exactly its size,
 * regardless of scaling factors, which may not be appropriate when
 * drawing on displays with high pixel densities.
 *
 * Deprecated: 4.10
 **/
void
bobgui_render_icon (BobguiStyleContext *context,
                 cairo_t         *cr,
                 GdkTexture      *texture,
                 double           x,
                 double           y)
{
  BobguiSnapshot *snapshot;
  GskRenderNode *node;

  snapshot = bobgui_snapshot_new ();
  bobgui_css_style_snapshot_icon_paintable (bobgui_style_context_lookup_style (context),
                                         snapshot,
                                         GDK_PAINTABLE (texture),
                                         gdk_texture_get_width (texture),
                                         gdk_texture_get_height (texture));
  node = bobgui_snapshot_free_to_node (snapshot);
  if (node == NULL)
    return;

  cairo_save (cr);
  cairo_translate (cr, x, y);
  gsk_render_node_draw (node, cr);
  cairo_restore (cr);
}


/**
 * bobgui_snapshot_render_background:
 * @snapshot: a `BobguiSnapshot`
 * @context: the style context that defines the background
 * @x: X origin of the rectangle
 * @y: Y origin of the rectangle
 * @width: rectangle width
 * @height: rectangle height
 *
 * Creates a render node for the CSS background according to @context,
 * and appends it to the current node of @snapshot, without changing
 * the current node.
 *
 * Deprecated: 4.10
 */
void
bobgui_snapshot_render_background (BobguiSnapshot     *snapshot,
                                BobguiStyleContext *context,
                                double           x,
                                double           y,
                                double           width,
                                double           height)
{
  BobguiCssBoxes boxes;

  g_return_if_fail (snapshot != NULL);
  g_return_if_fail (BOBGUI_IS_STYLE_CONTEXT (context));

  bobgui_css_boxes_init_border_box (&boxes,
                                 bobgui_style_context_lookup_style (context),
                                 x, y, width, height);
  bobgui_css_style_snapshot_background (&boxes, snapshot);
}

/**
 * bobgui_snapshot_render_frame:
 * @snapshot: a `BobguiSnapshot`
 * @context: the style context that defines the frame
 * @x: X origin of the rectangle
 * @y: Y origin of the rectangle
 * @width: rectangle width
 * @height: rectangle height
 *
 * Creates a render node for the CSS border according to @context,
 * and appends it to the current node of @snapshot, without changing
 * the current node.
 *
 * Deprecated: 4.10
 */
void
bobgui_snapshot_render_frame (BobguiSnapshot     *snapshot,
                           BobguiStyleContext *context,
                           double           x,
                           double           y,
                           double           width,
                           double           height)
{
  BobguiCssBoxes boxes;

  g_return_if_fail (snapshot != NULL);
  g_return_if_fail (BOBGUI_IS_STYLE_CONTEXT (context));

  bobgui_css_boxes_init_border_box (&boxes,
                                 bobgui_style_context_lookup_style (context),
                                 x, y, width, height);
  bobgui_css_style_snapshot_border (&boxes, snapshot);
}

/**
 * bobgui_snapshot_render_focus:
 * @snapshot: a `BobguiSnapshot`
 * @context: the style context that defines the focus ring
 * @x: X origin of the rectangle
 * @y: Y origin of the rectangle
 * @width: rectangle width
 * @height: rectangle height
 *
 * Creates a render node for the focus outline according to @context,
 * and appends it to the current node of @snapshot, without changing
 * the current node.
 *
 * Deprecated: 4.10
 */
void
bobgui_snapshot_render_focus (BobguiSnapshot     *snapshot,
                           BobguiStyleContext *context,
                           double           x,
                           double           y,
                           double           width,
                           double           height)
{
  BobguiCssBoxes boxes;

  g_return_if_fail (snapshot != NULL);
  g_return_if_fail (BOBGUI_IS_STYLE_CONTEXT (context));

  bobgui_css_boxes_init_border_box (&boxes,
                                 bobgui_style_context_lookup_style (context),
                                 x, y, width, height);
  bobgui_css_style_snapshot_outline (&boxes, snapshot);
}

/**
 * bobgui_snapshot_render_layout:
 * @snapshot: a `BobguiSnapshot`
 * @context: the style context that defines the text
 * @x: X origin of the rectangle
 * @y: Y origin of the rectangle
 * @layout: the `PangoLayout` to render
 *
 * Creates a render node for rendering @layout according to the style
 * information in @context, and appends it to the current node of @snapshot,
 * without changing the current node.
 *
 * Deprecated: 4.10
 */
void
bobgui_snapshot_render_layout (BobguiSnapshot     *snapshot,
                            BobguiStyleContext *context,
                            double           x,
                            double           y,
                            PangoLayout     *layout)
{
  BobguiCssBoxes boxes;

  g_return_if_fail (snapshot != NULL);
  g_return_if_fail (BOBGUI_IS_STYLE_CONTEXT (context));
  g_return_if_fail (PANGO_IS_LAYOUT (layout));

  bobgui_css_boxes_init_border_box (&boxes,
                                 bobgui_style_context_lookup_style (context),
                                 x, y, 0, 0);
  bobgui_css_style_snapshot_layout (&boxes, snapshot, x, y, layout);
}

/**
 * bobgui_snapshot_render_insertion_cursor:
 * @snapshot: snapshot to render to
 * @context: a `BobguiStyleContext`
 * @x: X origin
 * @y: Y origin
 * @layout: the `PangoLayout` of the text
 * @index: the index in the `PangoLayout`
 * @direction: the `PangoDirection` of the text
 *
 * Draws a text caret using @snapshot at the specified index of @layout.
 *
 * Deprecated: 4.10
 */
void
bobgui_snapshot_render_insertion_cursor (BobguiSnapshot     *snapshot,
                                      BobguiStyleContext *context,
                                      double           x,
                                      double           y,
                                      PangoLayout     *layout,
                                      int              index,
                                      PangoDirection   direction)
{
  BobguiCssBoxes boxes;
  GdkDisplay *display;

  g_return_if_fail (snapshot != NULL);
  g_return_if_fail (BOBGUI_IS_STYLE_CONTEXT (context));
  g_return_if_fail (PANGO_IS_LAYOUT (layout));
  g_return_if_fail (index >= 0);

  bobgui_css_boxes_init_border_box (&boxes,
                                 bobgui_style_context_lookup_style (context),
                                 x, y, 0, 0);

  display = bobgui_style_context_get_display (context);
  bobgui_css_style_snapshot_caret (&boxes, display, snapshot, x, y, layout, index, direction);
}
