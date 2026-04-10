/* BOBGUI - The Bobgui Framework
 * Copyright (C) 2021 Red Hat, Inc.
 *
 * Authors:
 * - Matthias Clasen <mclasen@redhat.com>
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

#include "bobguipanedhandleprivate.h"
#include "bobguiwidgetprivate.h"
#include "bobguicssnodeprivate.h"
#include "bobguicssstyleprivate.h"
#include "bobguirendericonprivate.h"
#include "bobguicssboxesprivate.h"
#include "bobguiprivate.h"

#include "bobguipaned.h"


G_DEFINE_TYPE (BobguiPanedHandle, bobgui_paned_handle, BOBGUI_TYPE_WIDGET);

static void
bobgui_paned_handle_snapshot (BobguiWidget   *widget,
                           BobguiSnapshot *snapshot)
{
  BobguiCssStyle *style = bobgui_css_node_get_style (bobgui_widget_get_css_node (widget));
  int width, height;

  width = bobgui_widget_get_width (widget);
  height = bobgui_widget_get_height (widget);

  if (width > 0 && height > 0)
    bobgui_css_style_snapshot_icon (style, snapshot, width, height);
}

#define HANDLE_EXTRA_SIZE 6

static gboolean
bobgui_paned_handle_contains (BobguiWidget *widget,
                           double     x,
                           double     y)
{
  BobguiWidget *paned;
  BobguiCssBoxes boxes;
  graphene_rect_t area;

  bobgui_css_boxes_init (&boxes, widget);

  graphene_rect_init_from_rect (&area, bobgui_css_boxes_get_border_rect (&boxes));

  paned = bobgui_widget_get_parent (widget);
  if (!bobgui_paned_get_wide_handle (BOBGUI_PANED (paned)))
    graphene_rect_inset (&area, - HANDLE_EXTRA_SIZE, - HANDLE_EXTRA_SIZE);

  return graphene_rect_contains_point (&area, &GRAPHENE_POINT_INIT (x, y));
}

static void
bobgui_paned_handle_finalize (GObject *object)
{
  BobguiPanedHandle *self = BOBGUI_PANED_HANDLE (object);
  BobguiWidget *widget;

  widget = _bobgui_widget_get_first_child (BOBGUI_WIDGET (self));
  while (widget != NULL)
    {
      BobguiWidget *next = _bobgui_widget_get_next_sibling (widget);

      bobgui_widget_unparent (widget);

      widget = next;
    }

  G_OBJECT_CLASS (bobgui_paned_handle_parent_class)->finalize (object);
}

static void
bobgui_paned_handle_class_init (BobguiPanedHandleClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (klass);

  object_class->finalize = bobgui_paned_handle_finalize;

  widget_class->snapshot = bobgui_paned_handle_snapshot;
  widget_class->contains = bobgui_paned_handle_contains;

  bobgui_widget_class_set_css_name (widget_class, I_("separator"));
}

static void
bobgui_paned_handle_init (BobguiPanedHandle *self)
{
}

BobguiWidget *
bobgui_paned_handle_new (void)
{
  return g_object_new (BOBGUI_TYPE_PANED_HANDLE, NULL);
}
