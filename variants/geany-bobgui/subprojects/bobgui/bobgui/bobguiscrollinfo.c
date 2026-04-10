/* BOBGUI - The Bobgui Framework
 * Copyright (C) 2023 Benjamin Otte
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.          See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * BobguiScrollInfo:
 *
 * Provides detailed information on how a scroll operation should be performed.
 *
 * Scrolling functions usually allow passing a `NULL` scroll info which will
 * cause the default values to be used and just scroll the element into view.
 *
 * Since: 4.12
 */

#include "config.h"

#include "bobguiscrollinfoprivate.h"

#include <math.h>

struct _BobguiScrollInfo
{
  guint ref_count;

  gboolean enabled[2]; /* directions */
};

static BobguiScrollInfo default_scroll_info = {
  1,
  { TRUE, TRUE }
};

G_DEFINE_BOXED_TYPE (BobguiScrollInfo, bobgui_scroll_info,
                     bobgui_scroll_info_ref,
                     bobgui_scroll_info_unref)


/**
 * bobgui_scroll_info_new:
 *
 * Creates a new scroll info for scrolling an element into view.
 *
 * Returns: A new scroll info
 *
 * Since: 4.12
 **/
BobguiScrollInfo *
bobgui_scroll_info_new (void)
{
  BobguiScrollInfo *self;

  self = g_new0 (BobguiScrollInfo, 1);
  self->ref_count = 1;
  self->enabled[BOBGUI_ORIENTATION_HORIZONTAL] = TRUE;
  self->enabled[BOBGUI_ORIENTATION_VERTICAL] = TRUE;

  return self;
}

/**
 * bobgui_scroll_info_ref:
 * @self:  a `BobguiScrollInfo`
 *
 * Increases the reference count of a `BobguiScrollInfo` by one.
 *
 * Returns: the passed in `BobguiScrollInfo`.
 *
 * Since: 4.12
 */
BobguiScrollInfo *
bobgui_scroll_info_ref (BobguiScrollInfo *self)
{
  g_return_val_if_fail (self != NULL, NULL);

  self->ref_count++;

  return self;
}

/**
 * bobgui_scroll_info_unref:
 * @self: a `BobguiScrollInfo`
 *
 * Decreases the reference count of a `BobguiScrollInfo` by one.
 *
 * If the resulting reference count is zero, frees the self.
 *
 * Since: 4.12
 */
void
bobgui_scroll_info_unref (BobguiScrollInfo *self)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (self->ref_count > 0);

  self->ref_count--;
  if (self->ref_count > 0)
    return;

  g_free (self);
}

/**
 * bobgui_scroll_info_set_enable_horizontal:
 * @self: a `BobguiScrollInfo`
 * @horizontal: if scrolling in the horizontal direction
 *     should happen
 *
 * Turns horizontal scrolling on or off.
 *
 * Since: 4.12
 **/
void
bobgui_scroll_info_set_enable_horizontal (BobguiScrollInfo *self,
                                       gboolean       horizontal)
{
  g_return_if_fail (self != NULL);

  self->enabled[BOBGUI_ORIENTATION_HORIZONTAL] = horizontal;
}

/**
 * bobgui_scroll_info_get_enable_horizontal:
 * @self: a `BobguiScrollInfo`
 *
 * Checks if horizontal scrolling is enabled.
 *
 * Returns: %TRUE if horizontal scrolling is enabled.
 *
 * Since: 4.12
 **/
gboolean
bobgui_scroll_info_get_enable_horizontal (BobguiScrollInfo *self)
{
  g_return_val_if_fail (self != NULL, FALSE);

  return self->enabled[BOBGUI_ORIENTATION_HORIZONTAL];
}

/**
 * bobgui_scroll_info_set_enable_vertical:
 * @self: a `BobguiScrollInfo`
 * @vertical: if scrolling in the vertical direction
 *     should happen
 *
 * Turns vertical scrolling on or off.
 *
 * Since: 4.12
 **/
void
bobgui_scroll_info_set_enable_vertical (BobguiScrollInfo *self,
                                     gboolean       vertical)
{
  g_return_if_fail (self != NULL);

  self->enabled[BOBGUI_ORIENTATION_VERTICAL] = vertical;
}

/**
 * bobgui_scroll_info_get_enable_vertical:
 * @self: a `BobguiScrollInfo`
 *
 * Checks if vertical scrolling is enabled.
 *
 * Returns: %TRUE if vertical scrolling is enabled.
 *
 * Since: 4.12
 **/
gboolean
bobgui_scroll_info_get_enable_vertical (BobguiScrollInfo *self)
{
  g_return_val_if_fail (self != NULL, FALSE);

  return self->enabled[BOBGUI_ORIENTATION_VERTICAL];
}

int
bobgui_scroll_info_compute_for_orientation (BobguiScrollInfo  *self,
                                         BobguiOrientation  orientation,
                                         int             area_origin,
                                         int             area_size,
                                         int             viewport_origin,
                                         int             viewport_size)
{
  float origin, size;
  int delta;

  if (self == NULL)
    self = &default_scroll_info;

  if (!self->enabled[orientation])
    return viewport_origin;

  origin = viewport_origin;
  size = viewport_size;

  if (area_origin <= origin)
    delta = area_origin - ceil (origin);
  else if (area_origin + area_size > origin + size)
    delta = area_origin + area_size - floor (origin + size);
  else
    delta = 0;
                                      
  return viewport_origin + delta;
}

/*<private>
 * bobgui_scroll_info_compute_scroll:
 * @self: a `BobguiScrollInfo`
 * @area: area to scroll
 * @viewport: viewport area to scroll into
 * @out_x: (out): x coordinate to scroll viewport to
 * @out_y: (out): y coordinate to scroll viewport to
 *
 * Computes The new x/y coordinate to move the viewport to
 * according to this scroll info.
 **/
void
bobgui_scroll_info_compute_scroll (BobguiScrollInfo               *self,
                                const cairo_rectangle_int_t *area,
                                const cairo_rectangle_int_t *viewport,
                                int                         *out_x,
                                int                         *out_y)
{
  *out_x = bobgui_scroll_info_compute_for_orientation (self,
                                                    BOBGUI_ORIENTATION_HORIZONTAL,
                                                    area->x, area->width,
                                                    viewport->x, viewport->width);
  *out_y = bobgui_scroll_info_compute_for_orientation (self,
                                                    BOBGUI_ORIENTATION_VERTICAL,
                                                    area->y, area->height,
                                                    viewport->y, viewport->height);
}

