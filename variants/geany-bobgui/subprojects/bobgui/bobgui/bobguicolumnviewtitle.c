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

#include "config.h"

#include "bobguicolumnviewtitleprivate.h"

#include "bobguicolumnviewprivate.h"
#include "bobguicolumnviewcolumnprivate.h"
#include "bobguicolumnviewsorterprivate.h"
#include "bobguicssboxesprivate.h"
#include "bobguicssnodeprivate.h"
#include "bobguiprivate.h"
#include "bobguilabel.h"
#include "bobguiwidgetprivate.h"
#include "bobguibox.h"
#include "bobguibuiltiniconprivate.h"
#include "bobguigestureclick.h"
#include "bobguipopovermenu.h"
#include "bobguinative.h"

struct _BobguiColumnViewTitle
{
  BobguiWidget parent_instance;

  BobguiColumnViewColumn *column;

  BobguiWidget *box;
  BobguiWidget *title;
  BobguiWidget *sort;
  BobguiWidget *popup_menu;
};

struct _BobguiColumnViewTitleClass
{
  BobguiWidgetClass parent_class;
};

G_DEFINE_TYPE (BobguiColumnViewTitle, bobgui_column_view_title, BOBGUI_TYPE_WIDGET)

static int
unadjust_width (BobguiWidget *widget,
                int        width)
{
  BobguiCssBoxes boxes;

  if (width <= -1)
    return -1;

  bobgui_css_boxes_init_border_box (&boxes,
                                 bobgui_css_node_get_style (bobgui_widget_get_css_node (widget)),
                                 0, 0,
                                 width, 100000);
  return MAX (0, floor (bobgui_css_boxes_get_content_rect (&boxes)->size.width));
}

static void
bobgui_column_view_title_measure (BobguiWidget      *widget,
                               BobguiOrientation  orientation,
                               int             for_size,
                               int            *minimum,
                               int            *natural,
                               int            *minimum_baseline,
                               int            *natural_baseline)
{
  BobguiColumnViewTitle *self = BOBGUI_COLUMN_VIEW_TITLE (widget);
  BobguiWidget *child = bobgui_widget_get_first_child (widget);
  int fixed_width, unadj_width;

  fixed_width = bobgui_column_view_column_get_fixed_width (self->column);
  unadj_width = unadjust_width (widget, fixed_width);

  if (orientation == BOBGUI_ORIENTATION_VERTICAL)
    {
      if (fixed_width > -1)
        {
          int min;

          if (for_size == -1)
            for_size = unadj_width;
          else
            for_size = MIN (for_size, unadj_width);

          bobgui_widget_measure (child, BOBGUI_ORIENTATION_HORIZONTAL, -1, &min, NULL, NULL, NULL);
          for_size = MAX (for_size, min);
        }
    }

  if (child)
    bobgui_widget_measure (child, orientation, for_size, minimum, natural, minimum_baseline, natural_baseline);

  if (orientation == BOBGUI_ORIENTATION_HORIZONTAL)
    {
      if (fixed_width > -1)
        {
          *minimum = 0;
          *natural = unadj_width;
        }
    }
}

static void
bobgui_column_view_title_size_allocate (BobguiWidget *widget,
                                     int        width,
                                     int        height,
                                     int        baseline)
{
  BobguiColumnViewTitle *self = BOBGUI_COLUMN_VIEW_TITLE (widget);
  BobguiWidget *child = bobgui_widget_get_first_child (widget);

  if (child)
    {
      int min;

      bobgui_widget_measure (child, BOBGUI_ORIENTATION_HORIZONTAL, height, &min, NULL, NULL, NULL);

      bobgui_widget_allocate (child, MAX (min, width), height, baseline, NULL);
    }

  if (self->popup_menu)
    bobgui_popover_present (BOBGUI_POPOVER (self->popup_menu));
}

static void
bobgui_column_view_title_dispose (GObject *object)
{
  BobguiColumnViewTitle *self = BOBGUI_COLUMN_VIEW_TITLE (object);

  g_clear_pointer (&self->box, bobgui_widget_unparent);
  g_clear_pointer (&self->popup_menu, bobgui_widget_unparent);

  g_clear_object (&self->column);

  G_OBJECT_CLASS (bobgui_column_view_title_parent_class)->dispose (object);
}

static void
bobgui_column_view_title_class_init (BobguiColumnViewTitleClass *klass)
{
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (klass);
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  widget_class->measure = bobgui_column_view_title_measure;
  widget_class->size_allocate = bobgui_column_view_title_size_allocate;

  gobject_class->dispose = bobgui_column_view_title_dispose;

  bobgui_widget_class_set_css_name (widget_class, I_("button"));
  bobgui_widget_class_set_accessible_role (widget_class, BOBGUI_ACCESSIBLE_ROLE_COLUMN_HEADER);
}

static void
bobgui_column_view_title_resize_func (BobguiWidget *widget)
{
  BobguiColumnViewTitle *self = BOBGUI_COLUMN_VIEW_TITLE (widget);

  if (self->column)
    bobgui_column_view_column_queue_resize (self->column);
}

static void
activate_sort (BobguiColumnViewTitle *self)
{
  BobguiSorter *sorter;
  BobguiColumnView *view;
  BobguiColumnViewSorter *view_sorter;

  sorter = bobgui_column_view_column_get_sorter (self->column);
  if (sorter == NULL)
    return;

  view = bobgui_column_view_column_get_column_view (self->column);
  view_sorter = BOBGUI_COLUMN_VIEW_SORTER (bobgui_column_view_get_sorter (view));
  bobgui_column_view_sorter_add_column (view_sorter, self->column);
}

static void
show_menu (BobguiColumnViewTitle *self,
           double              x,
           double              y)
{
  if (!self->popup_menu)
    {
      GMenuModel *model;

      model = bobgui_column_view_column_get_header_menu (self->column);
      if (!model)
        return;

      self->popup_menu = bobgui_popover_menu_new_from_model (model);
      bobgui_widget_set_parent (self->popup_menu, BOBGUI_WIDGET (self));
      bobgui_popover_set_position (BOBGUI_POPOVER (self->popup_menu), BOBGUI_POS_BOTTOM);

      bobgui_popover_set_has_arrow (BOBGUI_POPOVER (self->popup_menu), FALSE);
      bobgui_widget_set_halign (self->popup_menu, BOBGUI_ALIGN_START);
    }

  if (x != -1 && y != -1)
    {
      GdkRectangle rect = { x, y, 1, 1 };
      bobgui_popover_set_pointing_to (BOBGUI_POPOVER (self->popup_menu), &rect);
    }
  else
    bobgui_popover_set_pointing_to (BOBGUI_POPOVER (self->popup_menu), NULL);

  bobgui_popover_popup (BOBGUI_POPOVER (self->popup_menu));
}

static void
click_released_cb (BobguiGestureClick *gesture,
                   guint            n_press,
                   double           x,
                   double           y,
                   BobguiWidget       *widget)
{
  BobguiColumnViewTitle *self = BOBGUI_COLUMN_VIEW_TITLE (widget);
  guint button;

  button = bobgui_gesture_single_get_current_button (BOBGUI_GESTURE_SINGLE (gesture));

  if (button == GDK_BUTTON_PRIMARY)
    activate_sort (self);
  else if (button == GDK_BUTTON_SECONDARY)
    show_menu (self, x, y);
}

static void
click_pressed_cb (BobguiGestureClick *gesture,
                  int              n_press,
                  double           x,
                  double           y,
                  BobguiColumnView   *self)
{
  /* Claim the state here to prevent propagation, the event controllers in
   * BobguiColumView have already been handled in the CAPTURE phase */
  bobgui_gesture_set_state (BOBGUI_GESTURE (gesture), BOBGUI_EVENT_SEQUENCE_CLAIMED);
}

static void
bobgui_column_view_title_init (BobguiColumnViewTitle *self)
{
  BobguiWidget *widget = BOBGUI_WIDGET (self);
  BobguiGesture *gesture;

  widget->priv->resize_func = bobgui_column_view_title_resize_func;

  bobgui_widget_set_overflow (widget, BOBGUI_OVERFLOW_HIDDEN);

  self->box = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 0);
  bobgui_widget_set_parent (self->box, widget);

  self->title = bobgui_label_new (NULL);
  bobgui_box_append (BOBGUI_BOX (self->box), self->title);

  self->sort = bobgui_builtin_icon_new ("sort-indicator");
  bobgui_box_append (BOBGUI_BOX (self->box), self->sort);

  gesture = bobgui_gesture_click_new ();
  bobgui_gesture_single_set_button (BOBGUI_GESTURE_SINGLE (gesture), 0);
  g_signal_connect (gesture, "released", G_CALLBACK (click_released_cb), self);
  g_signal_connect (gesture, "pressed", G_CALLBACK (click_pressed_cb), self);
  bobgui_widget_add_controller (BOBGUI_WIDGET (self), BOBGUI_EVENT_CONTROLLER (gesture));
}

BobguiWidget *
bobgui_column_view_title_new (BobguiColumnViewColumn *column)
{
  BobguiColumnViewTitle *title;

  title = g_object_new (BOBGUI_TYPE_COLUMN_VIEW_TITLE, NULL);

  title->column = g_object_ref (column);
  bobgui_column_view_title_update_sort (title);
  bobgui_column_view_title_set_title (title, bobgui_column_view_column_get_title (column));
  bobgui_column_view_title_set_menu (title, bobgui_column_view_column_get_header_menu (column));

  return BOBGUI_WIDGET (title);
}

void
bobgui_column_view_title_set_title (BobguiColumnViewTitle *self,
                                 const char         *title)
{
  bobgui_label_set_label (BOBGUI_LABEL (self->title), title);
}

void
bobgui_column_view_title_set_menu (BobguiColumnViewTitle *self,
                                GMenuModel         *model)
{
  g_clear_pointer (&self->popup_menu, bobgui_widget_unparent);
}

void
bobgui_column_view_title_update_sort (BobguiColumnViewTitle *self)
{
  if (bobgui_column_view_column_get_sorter (self->column))
    {
      BobguiColumnView *view;
      BobguiColumnViewSorter *view_sorter;
      BobguiColumnViewColumn *primary;
      BobguiSortType sort_order;

      view = bobgui_column_view_column_get_column_view (self->column);
      view_sorter = BOBGUI_COLUMN_VIEW_SORTER (bobgui_column_view_get_sorter (view));
      primary = bobgui_column_view_sorter_get_primary_sort_column (view_sorter);
      sort_order = bobgui_column_view_sorter_get_primary_sort_order (view_sorter);

      bobgui_widget_set_visible (self->sort, TRUE);
      bobgui_widget_remove_css_class (self->sort, "ascending");
      bobgui_widget_remove_css_class (self->sort, "descending");
      bobgui_widget_remove_css_class (self->sort, "unsorted");

      if (self->column != primary)
        bobgui_widget_add_css_class (self->sort, "unsorted");
      else if (sort_order == BOBGUI_SORT_DESCENDING)
        bobgui_widget_add_css_class (self->sort, "descending");
      else
        bobgui_widget_add_css_class (self->sort, "ascending");
    }
  else
    bobgui_widget_set_visible (self->sort, FALSE);
}

BobguiColumnViewColumn *
bobgui_column_view_title_get_column (BobguiColumnViewTitle *self)
{
  return self->column;
}
