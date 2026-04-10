/* bobguilayoutmanager.h: Layout manager base class
 * Copyright 2019  The GNOME Foundation
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
 * Author: Emmanuele Bassi
 */
#pragma once

#include <gsk/gsk.h>
#include <bobgui/bobguitypes.h>
#include <bobgui/bobguiwidget.h>
#include <bobgui/bobguilayoutchild.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_LAYOUT_MANAGER (bobgui_layout_manager_get_type ())

GDK_AVAILABLE_IN_ALL
G_DECLARE_DERIVABLE_TYPE (BobguiLayoutManager, bobgui_layout_manager, BOBGUI, LAYOUT_MANAGER, GObject)

/**
 * BobguiLayoutManagerClass:
 * @get_request_mode: a virtual function, used to return the preferred
 *   request mode for the layout manager; for instance, "width for height"
 *   or "height for width"; see `BobguiSizeRequestMode`
 * @measure: a virtual function, used to measure the minimum and preferred
 *   sizes of the widget using the layout manager for a given orientation
 * @allocate: a virtual function, used to allocate the size of the widget
 *   using the layout manager
 * @layout_child_type: the type of `BobguiLayoutChild` used by this layout manager
 * @create_layout_child: a virtual function, used to create a `BobguiLayoutChild`
 *   meta object for the layout properties
 * @root: a virtual function, called when the widget using the layout
 *   manager is attached to a `BobguiRoot`
 * @unroot: a virtual function, called when the widget using the layout
 *   manager is detached from a `BobguiRoot`
 *
 * The `BobguiLayoutManagerClass` structure contains only private data, and
 * should only be accessed through the provided API, or when subclassing
 * `BobguiLayoutManager`.
 */
struct _BobguiLayoutManagerClass
{
  /*< private >*/
  GObjectClass parent_class;

  /*< public >*/
  BobguiSizeRequestMode (* get_request_mode)    (BobguiLayoutManager *manager,
                                              BobguiWidget        *widget);

  void               (* measure)             (BobguiLayoutManager *manager,
                                              BobguiWidget        *widget,
                                              BobguiOrientation    orientation,
                                              int               for_size,
                                              int              *minimum,
                                              int              *natural,
                                              int              *minimum_baseline,
                                              int              *natural_baseline);

  void               (* allocate)            (BobguiLayoutManager *manager,
                                              BobguiWidget        *widget,
                                              int               width,
                                              int               height,
                                              int               baseline);

  GType              layout_child_type;

  /**
   * BobguiLayoutManagerClass::create_layout_child:
   * @manager: the `BobguiLayoutManager`
   * @widget: the widget using the @manager
   * @for_child: the child of @widget
   *
   * Create a `BobguiLayoutChild` instance for the given @for_child widget.
   *
   * Returns: (transfer full): a `BobguiLayoutChild`
   */
  BobguiLayoutChild *   (* create_layout_child) (BobguiLayoutManager *manager,
                                              BobguiWidget        *widget,
                                              BobguiWidget        *for_child);

  void               (* root)                (BobguiLayoutManager *manager);
  void               (* unroot)              (BobguiLayoutManager *manager);

  /*< private >*/
  gpointer _padding[16];
};

GDK_AVAILABLE_IN_ALL
void                    bobgui_layout_manager_measure              (BobguiLayoutManager *manager,
                                                                 BobguiWidget        *widget,
                                                                 BobguiOrientation    orientation,
                                                                 int               for_size,
                                                                 int              *minimum,
                                                                 int              *natural,
                                                                 int              *minimum_baseline,
                                                                 int              *natural_baseline);
GDK_AVAILABLE_IN_ALL
void                    bobgui_layout_manager_allocate             (BobguiLayoutManager *manager,
                                                                 BobguiWidget        *widget,
                                                                 int               width,
                                                                 int               height,
                                                                 int               baseline);
GDK_AVAILABLE_IN_ALL
BobguiSizeRequestMode      bobgui_layout_manager_get_request_mode     (BobguiLayoutManager *manager);

GDK_AVAILABLE_IN_ALL
BobguiWidget *             bobgui_layout_manager_get_widget           (BobguiLayoutManager *manager);

GDK_AVAILABLE_IN_ALL
void                    bobgui_layout_manager_layout_changed       (BobguiLayoutManager *manager);

GDK_AVAILABLE_IN_ALL
BobguiLayoutChild *        bobgui_layout_manager_get_layout_child     (BobguiLayoutManager *manager,
                                                                 BobguiWidget        *child);

G_END_DECLS
