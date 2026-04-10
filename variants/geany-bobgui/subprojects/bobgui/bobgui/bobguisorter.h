/*
 * Copyright © 2019 Matthias Clasen
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

#pragma once

#if !defined (__BOBGUI_H_INSIDE__) && !defined (BOBGUI_COMPILATION)
#error "Only <bobgui/bobgui.h> can be included directly."
#endif

#include <gdk/gdk.h>
#include <bobgui/bobguienums.h>

G_BEGIN_DECLS

/**
 * BobguiSorterOrder:
 * @BOBGUI_SORTER_ORDER_PARTIAL: A partial order. Any `BobguiOrdering` is possible.
 * @BOBGUI_SORTER_ORDER_NONE: No order, all elements are considered equal.
 *   bobgui_sorter_compare() will only return %BOBGUI_ORDERING_EQUAL.
 * @BOBGUI_SORTER_ORDER_TOTAL: A total order. bobgui_sorter_compare() will only
 *   return %BOBGUI_ORDERING_EQUAL if an item is compared with itself. Two
 *   different items will never cause this value to be returned.
 *
 * Describes the type of order that a `BobguiSorter` may produce.
 */
typedef enum {
  BOBGUI_SORTER_ORDER_PARTIAL,
  BOBGUI_SORTER_ORDER_NONE,
  BOBGUI_SORTER_ORDER_TOTAL
} BobguiSorterOrder;

/**
 * BobguiSorterChange:
 * @BOBGUI_SORTER_CHANGE_DIFFERENT: The sorter change cannot be described
 *   by any of the other enumeration values
 * @BOBGUI_SORTER_CHANGE_INVERTED: The sort order was inverted. Comparisons
 *   that returned %BOBGUI_ORDERING_SMALLER now return %BOBGUI_ORDERING_LARGER
 *   and vice versa. Other comparisons return the same values as before.
 * @BOBGUI_SORTER_CHANGE_LESS_STRICT: The sorter is less strict: Comparisons
 *   may now return %BOBGUI_ORDERING_EQUAL that did not do so before.
 * @BOBGUI_SORTER_CHANGE_MORE_STRICT: The sorter is more strict: Comparisons
 *   that did return %BOBGUI_ORDERING_EQUAL may not do so anymore.
 *
 * Describes changes in a sorter in more detail and allows users
 * to optimize resorting.
 */
typedef enum {
  BOBGUI_SORTER_CHANGE_DIFFERENT,
  BOBGUI_SORTER_CHANGE_INVERTED,
  BOBGUI_SORTER_CHANGE_LESS_STRICT,
  BOBGUI_SORTER_CHANGE_MORE_STRICT
} BobguiSorterChange;

#define BOBGUI_TYPE_SORTER             (bobgui_sorter_get_type ())

GDK_AVAILABLE_IN_ALL
G_DECLARE_DERIVABLE_TYPE (BobguiSorter, bobgui_sorter, BOBGUI, SORTER, GObject)

/**
 * BobguiSorterClass
 * @compare: Compare two items. See bobgui_sorter_compare() for details.
 * @get_order: Get the `BobguiSorderOrder` that applies to the current sorter.
 *   If unimplemented, it returns %BOBGUI_SORTER_ORDER_PARTIAL.
 *
 * The virtual table for `BobguiSorter`.
 */
struct _BobguiSorterClass
{
  GObjectClass parent_class;

  BobguiOrdering           (* compare)                             (BobguiSorter              *self,
                                                                 gpointer                item1,
                                                                 gpointer                item2);

  /* optional */
  BobguiSorterOrder        (* get_order)                           (BobguiSorter              *self);

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
BobguiOrdering             bobgui_sorter_compare                      (BobguiSorter              *self,
                                                                 gpointer                item1,
                                                                 gpointer                item2);
GDK_AVAILABLE_IN_ALL
BobguiSorterOrder          bobgui_sorter_get_order                    (BobguiSorter              *self);

/* for sorter implementations */
GDK_AVAILABLE_IN_ALL
void                    bobgui_sorter_changed                      (BobguiSorter              *self,
                                                                 BobguiSorterChange         change);


G_END_DECLS


