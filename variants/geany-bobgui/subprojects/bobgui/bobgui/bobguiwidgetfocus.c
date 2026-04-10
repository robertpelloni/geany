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

#include "bobguiwidgetprivate.h"
#include "bobguinative.h"
#include "bobguiboxlayout.h"
#include "bobguiorientable.h"

typedef struct _CompareInfo CompareInfo;

enum Axis {
  HORIZONTAL = 0,
  VERTICAL   = 1
};

struct _CompareInfo
{
  BobguiWidget *widget;
  int x;
  int y;
  guint reverse : 1;
  guint axis : 1;
};

static inline void
get_axis_info (const graphene_rect_t *bounds,
               int                    axis,
               int                   *start,
               int                   *end)
{
  if (axis == HORIZONTAL)
    {
      *start = bounds->origin.x;
      *end = bounds->size.width;
    }
  else if (axis == VERTICAL)
    {
      *start = bounds->origin.y;
      *end = bounds->size.height;
    }
  else
    g_assert(FALSE);
}

/* Utility function, equivalent to g_list_reverse */
static void
reverse_ptr_array (GPtrArray *arr)
{
  int i;

  for (i = 0; i < arr->len / 2; i ++)
    {
      void *a = g_ptr_array_index (arr, i);
      void *b = g_ptr_array_index (arr, arr->len - 1 - i);

      arr->pdata[i] = b;
      arr->pdata[arr->len - 1 - i] = a;
    }
}

static int
tab_sort_func (gconstpointer a,
               gconstpointer b,
               gpointer      user_data)
{
  graphene_rect_t child_bounds1, child_bounds2;
  BobguiWidget *child1 = *((BobguiWidget **)a);
  BobguiWidget *child2 = *((BobguiWidget **)b);
  BobguiTextDirection text_direction = GPOINTER_TO_INT (user_data);
  float y1, y2;

  if (!bobgui_widget_compute_bounds (child1, _bobgui_widget_get_parent (child1), &child_bounds1) ||
      !bobgui_widget_compute_bounds (child2, _bobgui_widget_get_parent (child2), &child_bounds2))
    return 0;

  y1 = child_bounds1.origin.y + (child_bounds1.size.height / 2.0f);
  y2 = child_bounds2.origin.y + (child_bounds2.size.height / 2.0f);

  if (y1 == y2)
    {
      const float x1 = child_bounds1.origin.x + (child_bounds1.size.width / 2.0f);
      const float x2 = child_bounds2.origin.x + (child_bounds2.size.width / 2.0f);

      if (text_direction == BOBGUI_TEXT_DIR_RTL)
        return (x1 < x2) ? 1 : ((x1 == x2) ? 0 : -1);
      else
        return (x1 < x2) ? -1 : ((x1 == x2) ? 0 : 1);
    }
  else
    return (y1 < y2) ? -1 : 1;
}

static void
focus_sort_tab (BobguiWidget        *widget,
                BobguiDirectionType  direction,
                GPtrArray        *focus_order)
{
  BobguiTextDirection text_direction = _bobgui_widget_get_direction (widget);

  g_ptr_array_sort_with_data (focus_order, tab_sort_func, GINT_TO_POINTER (text_direction));

  if (direction == BOBGUI_DIR_TAB_BACKWARD)
    reverse_ptr_array (focus_order);
}

/* Look for a child in @children that is intermediate between
 * the focus widget and container. This widget, if it exists,
 * acts as the starting widget for focus navigation.
 */
static BobguiWidget *
find_old_focus (BobguiWidget *widget,
                GPtrArray *children)
{
  int i;

  for (i = 0; i < children->len; i ++)
    {
      BobguiWidget *child = g_ptr_array_index (children, i);
      BobguiWidget *child_ptr = child;

      while (child_ptr && child_ptr != widget)
        {
          BobguiWidget *parent;

          parent = _bobgui_widget_get_parent (child_ptr);

          if (parent && (_bobgui_widget_get_focus_child (parent) != child_ptr))
            {
              child = NULL;
              break;
            }

          child_ptr = parent;
        }

      if (child)
        return child;
    }

  return NULL;
}

static gboolean
old_focus_coords (BobguiWidget       *widget,
                  graphene_rect_t *old_focus_bounds)
{
  BobguiWidget *old_focus;

  old_focus = bobgui_root_get_focus (bobgui_widget_get_root (widget));
  if (old_focus)
    return bobgui_widget_compute_bounds (old_focus, widget, old_focus_bounds);

  return FALSE;
}

static int
axis_compare (gconstpointer a,
              gconstpointer b,
              gpointer      user_data)
{
  graphene_rect_t bounds1;
  graphene_rect_t bounds2;
  CompareInfo *compare = user_data;
  int start1, end1;
  int start2, end2;

  if (!bobgui_widget_compute_bounds (*((BobguiWidget **)a), compare->widget, &bounds1) ||
      !bobgui_widget_compute_bounds (*((BobguiWidget **)b), compare->widget, &bounds2))
    return 0;

  get_axis_info (&bounds1, compare->axis, &start1, &end1);
  get_axis_info (&bounds2, compare->axis, &start2, &end2);

  start1 = start1 + (end1 / 2);
  start2 = start2 + (end2 / 2);

  if (start1 == start2)
    {
      /* Now use origin/bounds to compare the 2 widgets on the other axis */
      get_axis_info (&bounds1, 1 - compare->axis, &start1, &end1);
      get_axis_info (&bounds2, 1 - compare->axis, &start2, &end2);

      int x1 = abs (start1 + (end1 / 2) - compare->x);
      int x2 = abs (start2 + (end2 / 2) - compare->x);

      if (compare->reverse)
        return (x1 < x2) ? 1 : ((x1 == x2) ? 0 : -1);
      else
        return (x1 < x2) ? -1 : ((x1 == x2) ? 0 : 1);
    }
  else
    return (start1 < start2) ? -1 : 1;
}

static void
focus_sort_left_right (BobguiWidget        *widget,
                       BobguiDirectionType  direction,
                       GPtrArray        *focus_order)
{
  CompareInfo compare_info;
  BobguiWidget *old_focus = _bobgui_widget_get_focus_child (widget);
  graphene_rect_t old_bounds;

  compare_info.widget = widget;
  compare_info.reverse = (direction == BOBGUI_DIR_LEFT);

  if (!old_focus)
    old_focus = find_old_focus (widget, focus_order);

  if (old_focus && bobgui_widget_compute_bounds (old_focus, widget, &old_bounds))
    {
      float compare_y1;
      float compare_y2;
      float compare_x;
      int i;

      /* Delete widgets from list that don't match minimum criteria */

      compare_y1 = old_bounds.origin.y;
      compare_y2 = old_bounds.origin.y + old_bounds.size.height;

      if (direction == BOBGUI_DIR_LEFT)
        compare_x = old_bounds.origin.x;
      else
        compare_x = old_bounds.origin.x + old_bounds.size.width;

      for (i = 0; i < focus_order->len; i ++)
        {
          BobguiWidget *child = g_ptr_array_index (focus_order, i);

          if (child != old_focus)
            {
              graphene_rect_t child_bounds;

              if (bobgui_widget_compute_bounds (child, widget, &child_bounds))
                {
                  const float child_y1 = child_bounds.origin.y;
                  const float child_y2 = child_bounds.origin.y + child_bounds.size.height;

                  if ((child_y2 <= compare_y1 || child_y1 >= compare_y2) /* No vertical overlap */ ||
                      (direction == BOBGUI_DIR_RIGHT && child_bounds.origin.x + child_bounds.size.width < compare_x) || /* Not to left */
                      (direction == BOBGUI_DIR_LEFT && child_bounds.origin.x > compare_x)) /* Not to right */
                    {
                      g_ptr_array_remove_index (focus_order, i);
                      i --;
                    }
                }
              else
                {
                  g_ptr_array_remove_index (focus_order, i);
                  i --;
                }
            }
        }

      compare_info.y = (compare_y1 + compare_y2) / 2;
      compare_info.x = old_bounds.origin.x + (old_bounds.size.width / 2.0f);
    }
  else
    {
      /* No old focus widget, need to figure out starting x,y some other way
       */
      graphene_rect_t bounds;
      BobguiWidget *parent;
      graphene_rect_t old_focus_bounds;

      parent = bobgui_widget_get_parent (widget);
      if (!bobgui_widget_compute_bounds (widget, parent ? parent : widget, &bounds))
        graphene_rect_init (&bounds, 0, 0, 0, 0);

      if (old_focus_coords (widget, &old_focus_bounds))
        {
          compare_info.y = old_focus_bounds.origin.y + (old_focus_bounds.size.height / 2.0f);
        }
      else
        {
          if (!BOBGUI_IS_NATIVE (widget))
            compare_info.y = bounds.origin.y + bounds.size.height;
          else
            compare_info.y = bounds.size.height / 2.0f;
        }

      if (!BOBGUI_IS_NATIVE (widget))
        compare_info.x = (direction == BOBGUI_DIR_RIGHT) ? bounds.origin.x : bounds.origin.x + bounds.size.width;
      else
        compare_info.x = (direction == BOBGUI_DIR_RIGHT) ? 0 : bounds.size.width;
    }


  compare_info.axis = HORIZONTAL;
  g_ptr_array_sort_with_data (focus_order, axis_compare, &compare_info);

  if (compare_info.reverse)
    reverse_ptr_array (focus_order);
}

static void
focus_sort_up_down (BobguiWidget        *widget,
                    BobguiDirectionType  direction,
                    GPtrArray        *focus_order)
{
  CompareInfo compare_info;
  BobguiWidget *old_focus = _bobgui_widget_get_focus_child (widget);
  graphene_rect_t old_bounds;

  compare_info.widget = widget;
  compare_info.reverse = (direction == BOBGUI_DIR_UP);

  if (!old_focus)
    old_focus = find_old_focus (widget, focus_order);

  if (old_focus && bobgui_widget_compute_bounds (old_focus, widget, &old_bounds))
    {
      float compare_x1;
      float compare_x2;
      float compare_y;
      int i;

      /* Delete widgets from list that don't match minimum criteria */

      compare_x1 = old_bounds.origin.x;
      compare_x2 = old_bounds.origin.x + old_bounds.size.width;

      if (direction == BOBGUI_DIR_UP)
        compare_y = old_bounds.origin.y;
      else
        compare_y = old_bounds.origin.y + old_bounds.size.height;

      for (i = 0; i < focus_order->len; i ++)
        {
          BobguiWidget *child = g_ptr_array_index (focus_order, i);

          if (child != old_focus)
            {
              graphene_rect_t child_bounds;

              if (bobgui_widget_compute_bounds (child, widget, &child_bounds))
                {
                  const float child_x1 = child_bounds.origin.x;
                  const float child_x2 = child_bounds.origin.x + child_bounds.size.width;

                  if ((child_x2 <= compare_x1 || child_x1 >= compare_x2) /* No horizontal overlap */ ||
                      (direction == BOBGUI_DIR_DOWN && child_bounds.origin.y + child_bounds.size.height < compare_y) || /* Not below */
                      (direction == BOBGUI_DIR_UP && child_bounds.origin.y > compare_y)) /* Not above */
                    {
                      g_ptr_array_remove_index (focus_order, i);
                      i --;
                    }
                }
              else
                {
                  g_ptr_array_remove_index (focus_order, i);
                  i --;
                }
            }
        }

      compare_info.x = (compare_x1 + compare_x2) / 2;
      compare_info.y = old_bounds.origin.y + (old_bounds.size.height / 2.0f);
    }
  else
    {
      /* No old focus widget, need to figure out starting x,y some other way
       */
      BobguiWidget *parent;
      graphene_rect_t bounds;
      graphene_rect_t old_focus_bounds;

      parent = bobgui_widget_get_parent (widget);
      if (!bobgui_widget_compute_bounds (widget, parent ? parent : widget, &bounds))
        graphene_rect_init (&bounds, 0, 0, 0, 0);

      if (old_focus_coords (widget, &old_focus_bounds))
        {
          compare_info.x = old_focus_bounds.origin.x + (old_focus_bounds.size.width / 2.0f);
        }
      else
        {
          if (!BOBGUI_IS_NATIVE (widget))
            compare_info.x = bounds.origin.x + (bounds.size.width / 2.0f);
          else
            compare_info.x = bounds.size.width / 2.0f;
        }

      if (!BOBGUI_IS_NATIVE (widget))
        compare_info.y = (direction == BOBGUI_DIR_DOWN) ? bounds.origin.y : bounds.origin.y + bounds.size.height;
      else
        compare_info.y = (direction == BOBGUI_DIR_DOWN) ? 0 : + bounds.size.height;
    }

  compare_info.axis = VERTICAL;
  g_ptr_array_sort_with_data (focus_order, axis_compare, &compare_info);

  if (compare_info.reverse)
    reverse_ptr_array (focus_order);
}

void
bobgui_widget_focus_sort (BobguiWidget        *widget,
                       BobguiDirectionType  direction,
                       GPtrArray        *focus_order)
{
  BobguiWidget *child;

  g_assert (focus_order != NULL);

  if (focus_order->len == 0)
    {
      /* Initialize the list with all visible child widgets */
      for (child = _bobgui_widget_get_first_child (widget);
           child != NULL;
           child = _bobgui_widget_get_next_sibling (child))
        {
          if (_bobgui_widget_get_mapped (child) &&
              bobgui_widget_get_sensitive (child))
            g_ptr_array_add (focus_order, child);
        }
    }

  /* Now sort that list depending on @direction */
  switch (direction)
    {
    case BOBGUI_DIR_TAB_FORWARD:
    case BOBGUI_DIR_TAB_BACKWARD:
      {
        BobguiLayoutManager *layout = bobgui_widget_get_layout_manager (widget);
        if (BOBGUI_IS_BOX_LAYOUT (layout))
          {
            BobguiOrientation orientation = bobgui_orientable_get_orientation (BOBGUI_ORIENTABLE (layout));
            if (orientation == BOBGUI_ORIENTATION_HORIZONTAL)
              {
                if (direction == BOBGUI_DIR_TAB_FORWARD)
                  focus_sort_left_right (widget, BOBGUI_DIR_RIGHT, focus_order);
                else
                  focus_sort_left_right (widget, BOBGUI_DIR_LEFT, focus_order);
              }
            else
              {
                if (direction == BOBGUI_DIR_TAB_FORWARD)
                  focus_sort_up_down (widget, BOBGUI_DIR_DOWN, focus_order);
                else
                  focus_sort_up_down (widget, BOBGUI_DIR_UP, focus_order);
              }
          }
        else
          focus_sort_tab (widget, direction, focus_order);
      }
      break;
    case BOBGUI_DIR_UP:
    case BOBGUI_DIR_DOWN:
      focus_sort_up_down (widget, direction, focus_order);
      break;
    case BOBGUI_DIR_LEFT:
    case BOBGUI_DIR_RIGHT:
      focus_sort_left_right (widget, direction, focus_order);
      break;
    default:
      g_assert_not_reached ();
    }
}


gboolean
bobgui_widget_focus_move (BobguiWidget        *widget,
                       BobguiDirectionType  direction)
{
  GPtrArray *focus_order;
  BobguiWidget *focus_child = _bobgui_widget_get_focus_child (widget);
  int i;
  gboolean ret = FALSE;

  focus_order = g_ptr_array_new ();
  bobgui_widget_focus_sort (widget, direction, focus_order);

  for (i = 0; i < focus_order->len && !ret; i++)
    {
      BobguiWidget *child = g_ptr_array_index (focus_order, i);

      if (focus_child)
        {
          if (focus_child == child)
            {
              focus_child = NULL;
              ret = bobgui_widget_child_focus (child, direction);
            }
        }
      else if (_bobgui_widget_get_mapped (child) &&
               bobgui_widget_is_ancestor (child, widget))
        {
          ret = bobgui_widget_child_focus (child, direction);
        }
    }

  g_ptr_array_unref (focus_order);

  return ret;
}
