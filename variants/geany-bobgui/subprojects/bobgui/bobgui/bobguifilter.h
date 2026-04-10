/*
 * Copyright © 2019 Benjamin Otte
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

#pragma once

#if !defined (__BOBGUI_H_INSIDE__) && !defined (BOBGUI_COMPILATION)
#error "Only <bobgui/bobgui.h> can be included directly."
#endif

#include <gdk/gdk.h>

G_BEGIN_DECLS

/**
 * BobguiFilterMatch:
 * @BOBGUI_FILTER_MATCH_SOME: The filter matches some items,
 *   [method@Bobgui.Filter.match] may return true or false
 * @BOBGUI_FILTER_MATCH_NONE: The filter does not match any item,
 *   [method@Bobgui.Filter.match] will always return false
 * @BOBGUI_FILTER_MATCH_ALL: The filter matches all items,
 *   [method@Bobgui.Filter.match] will alays return true
 *
 * Describes the known strictness of a filter.
 *
 * Note that for filters where the strictness is not known,
 * `BOBGUI_FILTER_MATCH_SOME` is always an acceptable value,
 * even if a filter does match all or no items.
 */
typedef enum {
  BOBGUI_FILTER_MATCH_SOME = 0,
  BOBGUI_FILTER_MATCH_NONE,
  BOBGUI_FILTER_MATCH_ALL
} BobguiFilterMatch;

/**
 * BobguiFilterChange:
 * @BOBGUI_FILTER_CHANGE_DIFFERENT: The filter change cannot be
 *   described with any of the other enumeration values
 * @BOBGUI_FILTER_CHANGE_LESS_STRICT: The filter is less strict than
 *   it was before: All items that it used to return true
 *   still return true, others now may, too.
 * @BOBGUI_FILTER_CHANGE_MORE_STRICT: The filter is more strict than
 *   it was before: All items that it used to return false
 *   still return false, others now may, too.
 *
 * Describes changes in a filter in more detail and allows objects
 * using the filter to optimize refiltering items.
 *
 * If you are writing an implementation and are not sure which
 * value to pass, `BOBGUI_FILTER_CHANGE_DIFFERENT` is always a correct
 * choice.
 *
 * New values may be added in the future.
 */

/**
 * BOBGUI_FILTER_CHANGE_DIFFERENT_REWATCH:
 *
 * Similar to [enum@Bobgui.FilterChange.DIFFERENT],
 * but signs that item watches should be recreated. This is used by
 * [class@Bobgui.FilterListModel] to keep the list up-to-date when items
 * change.
 *
 * Since: 4.20
 */

/**
 * BOBGUI_FILTER_CHANGE_LESS_STRICT_REWATCH:
 *
 * Similar to [enum@Bobgui.FilterChange.LESS_STRICT],
 * but signs that item watches should be recreated. This is used by
 * [class@Bobgui.FilterListModel] to keep the list up-to-date when items
 * change.
 *
 * Since: 4.20
 */

/**
 * BOBGUI_FILTER_CHANGE_MORE_STRICT_REWATCH:
 * Similar to [enum@Bobgui.FilterChange.MORE_STRICT],
 * but signs that item watches should be recreated. This is used by
 * [class@Bobgui.FilterListModel] to keep the list up-to-date when items
 * change.
 *
 * Since: 4.20
 */
typedef enum {
  BOBGUI_FILTER_CHANGE_DIFFERENT = 0,
  BOBGUI_FILTER_CHANGE_LESS_STRICT,
  BOBGUI_FILTER_CHANGE_MORE_STRICT,
  BOBGUI_FILTER_CHANGE_DIFFERENT_REWATCH,
  BOBGUI_FILTER_CHANGE_LESS_STRICT_REWATCH,
  BOBGUI_FILTER_CHANGE_MORE_STRICT_REWATCH,
} BobguiFilterChange;

#define BOBGUI_TYPE_FILTER             (bobgui_filter_get_type ())

GDK_AVAILABLE_IN_ALL
G_DECLARE_DERIVABLE_TYPE (BobguiFilter, bobgui_filter, BOBGUI, FILTER, GObject)

struct _BobguiFilterClass
{
  GObjectClass parent_class;

  gboolean              (* match)                               (BobguiFilter              *self,
                                                                 gpointer                item);

  /* optional */
  BobguiFilterMatch        (* get_strictness)                      (BobguiFilter              *self);

  /* Padding for future expansion */
  void (*_bobgui_reserved1) (void);
  void (*_bobgui_reserved2) (void);
  void (*_bobgui_reserved3) (void);
  void (*_bobgui_reserved4) (void);
  void (*_bobgui_reserved5) (void);
  void (*_bobgui_reserved6) (void);
  void (*_bobgui_reserved7) (void);
  void (*_bobgui_reserved8) (void);
};

GDK_AVAILABLE_IN_ALL
gboolean                bobgui_filter_match                        (BobguiFilter              *self,
                                                                 gpointer                item);
GDK_AVAILABLE_IN_ALL
BobguiFilterMatch          bobgui_filter_get_strictness               (BobguiFilter              *self);

/* for filter implementations */
GDK_AVAILABLE_IN_ALL
void                    bobgui_filter_changed                      (BobguiFilter              *self,
                                                                 BobguiFilterChange         change);


G_END_DECLS

