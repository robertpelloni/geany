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

#include "bobguilistbaseprivate.h"

#include "bobguiadjustment.h"
#include "bobguibitset.h"
#include "bobguicssboxesprivate.h"
#include "bobguicssnodeprivate.h"
#include "bobguicsspositionvalueprivate.h"
#include "bobguidragsourceprivate.h"
#include "bobguidropcontrollermotion.h"
#include "bobguigesturedrag.h"
#include "bobguigizmoprivate.h"
#include "bobguilistitemwidgetprivate.h"
#include "bobguimultiselection.h"
#include "bobguiorientable.h"
#include "bobguiscrollable.h"
#include "bobguiscrollinfoprivate.h"
#include "bobguisingleselection.h"
#include "bobguisnapshot.h"
#include "bobguitypebuiltins.h"
#include "bobguiwidgetprivate.h"

/* Allow shadows to overdraw without immediately culling the widget at the viewport
 * boundary.
 * Choose this so that roughly 1 extra widget gets drawn on each side of the viewport,
 * but not more. Icons are 16px, text height is somewhere there, too.
 */
#define BOBGUI_LIST_BASE_CHILD_MAX_OVERDRAW 10

typedef struct _RubberbandData RubberbandData;

struct _RubberbandData
{
  BobguiWidget *widget;                            /* The rubberband widget */

  BobguiListItemTracker *start_tracker;            /* The item we started dragging on */
  double              start_align_across;       /* alignment in horizontal direction */
  double              start_align_along;        /* alignment in vertical direction */

  double pointer_x, pointer_y;                  /* mouse coordinates in widget space */
};

typedef struct _BobguiListBasePrivate BobguiListBasePrivate;

struct _BobguiListBasePrivate
{
  BobguiListItemManager *item_manager;
  BobguiSelectionModel *model;
  BobguiOrientation orientation;
  BobguiAdjustment *adjustment[2];
  BobguiScrollablePolicy scroll_policy[2];
  BobguiListTabBehavior tab_behavior;

  BobguiListItemTracker *anchor;
  double anchor_align_along;
  double anchor_align_across;
  BobguiPackType anchor_side_along;
  BobguiPackType anchor_side_across;
  guint center_widgets;
  guint above_below_widgets;
  /* the last item that was selected - basically the location to extend selections from */
  BobguiListItemTracker *selected;
  /* the item that has input focus */
  BobguiListItemTracker *focus;

  gboolean enable_rubberband;
  BobguiGesture *drag_gesture;
  RubberbandData *rubberband;

  guint autoscroll_id;
  double autoscroll_delta_x;
  double autoscroll_delta_y;
};

enum
{
  PROP_0,
  PROP_HADJUSTMENT,
  PROP_HSCROLL_POLICY,
  PROP_ORIENTATION,
  PROP_VADJUSTMENT,
  PROP_VSCROLL_POLICY,

  N_PROPS
};

/* HACK: We want the g_class argument in our instance init func and G_DEFINE_TYPE() won't let us */
static void bobgui_list_base_init_real (BobguiListBase *self, BobguiListBaseClass *g_class);
#define g_type_register_static_simple(a,b,c,d,e,evil,f) g_type_register_static_simple(a,b,c,d,e, (GInstanceInitFunc) bobgui_list_base_init_real, f);
G_DEFINE_ABSTRACT_TYPE_WITH_CODE (BobguiListBase, bobgui_list_base, BOBGUI_TYPE_WIDGET,
                                                              G_ADD_PRIVATE (BobguiListBase)
                                                              G_IMPLEMENT_INTERFACE (BOBGUI_TYPE_ORIENTABLE, NULL)
                                                              G_IMPLEMENT_INTERFACE (BOBGUI_TYPE_SCROLLABLE, NULL))
#undef g_type_register_static_simple
G_GNUC_UNUSED static void bobgui_list_base_init (BobguiListBase *self) { }

static GParamSpec *properties[N_PROPS] = { NULL, };

/*
 * bobgui_list_base_get_position_from_allocation:
 * @self: a `BobguiListBase`
 * @across: position in pixels in the direction cross to the list
 * @along:  position in pixels in the direction of the list
 * @pos: (out): set to the looked up position
 * @area: (out caller-allocates) (optional): set to the area occupied
 *   by the returned position
 *
 * Given a coordinate in list coordinates, determine the position of the
 * item that occupies that position.
 *
 * It is possible for @area to not include the point given by (across, along).
 * This will happen for example in the last row of a gridview, where the
 * last item will be returned for the whole width, even if there are empty
 * cells.
 *
 * Returns: %TRUE on success or %FALSE if no position occupies the given offset.
 **/
static guint
bobgui_list_base_get_position_from_allocation (BobguiListBase           *self,
                                            int                    across,
                                            int                    along,
                                            guint                 *pos,
                                            cairo_rectangle_int_t *area)
{
  return BOBGUI_LIST_BASE_GET_CLASS (self)->get_position_from_allocation (self, across, along, pos, area);
}

static gboolean
bobgui_list_base_adjustment_is_flipped (BobguiListBase    *self,
                                     BobguiOrientation  orientation)
{
  if (orientation == BOBGUI_ORIENTATION_VERTICAL)
    return FALSE;

  return bobgui_widget_get_direction (BOBGUI_WIDGET (self)) == BOBGUI_TEXT_DIR_RTL;
}

static void
bobgui_list_base_get_adjustment_values (BobguiListBase    *self,
                                     BobguiOrientation  orientation,
                                     int            *value,
                                     int            *size,
                                     int            *page_size)
{
  BobguiListBasePrivate *priv = bobgui_list_base_get_instance_private (self);
  int val, upper, ps;

  val = bobgui_adjustment_get_value (priv->adjustment[orientation]);
  upper = bobgui_adjustment_get_upper (priv->adjustment[orientation]);
  ps = bobgui_adjustment_get_page_size (priv->adjustment[orientation]);
  if (bobgui_list_base_adjustment_is_flipped (self, orientation))
    val = upper - ps - val;

  if (value)
    *value = val;
  if (size)
    *size = upper;
  if (page_size)
    *page_size = ps;
}

static void
bobgui_list_base_adjustment_value_changed_cb (BobguiAdjustment *adjustment,
                                           BobguiListBase   *self)
{
  BobguiListBasePrivate *priv = bobgui_list_base_get_instance_private (self);
  cairo_rectangle_int_t area, cell_area;
  int along, across, total_size;
  double align_across, align_along;
  BobguiPackType side_across, side_along;
  guint pos;

  bobgui_list_base_get_adjustment_values (self, OPPOSITE_ORIENTATION (priv->orientation), &area.x, &total_size, &area.width);
  if (total_size == area.width)
    align_across = 0.5;
  else if (adjustment != priv->adjustment[priv->orientation])
    align_across = CLAMP (priv->anchor_align_across, 0, 1);
  else
    align_across = (double) area.x / (total_size - area.width);
  across = area.x + round (align_across * area.width);
  across = CLAMP (across, 0, total_size - 1);

  bobgui_list_base_get_adjustment_values (self, priv->orientation, &area.y, &total_size, &area.height);
  if (total_size == area.height)
    align_along = 0.5;
  else if (adjustment != priv->adjustment[OPPOSITE_ORIENTATION(priv->orientation)])
    align_along = CLAMP (priv->anchor_align_along, 0, 1);
  else
    align_along = (double) area.y / (total_size - area.height);
  along = area.y + round (align_along * area.height);
  along = CLAMP (along, 0, total_size - 1);

  if (!bobgui_list_base_get_position_from_allocation (self,
                                                   across, along,
                                                   &pos,
                                                   &cell_area))
    {
      /* If we get here with n-items == 0, then somebody cleared the list but
       * GC hasn't run. So no item to be found. */
      if (bobgui_list_base_get_n_items (self) == 0)
        return;

      g_warning ("%s failed to scroll to given position. Ignoring...", G_OBJECT_TYPE_NAME (self));
      return;
    }

  /* find an anchor that is in the visible area */
  if (cell_area.x < area.x && cell_area.x + cell_area.width <= area.x + area.width)
    side_across = BOBGUI_PACK_END;
  else if (cell_area.x >= area.x && cell_area.x + cell_area.width > area.x + area.width)
    side_across = BOBGUI_PACK_START;
  else if (cell_area.x + cell_area.width / 2 > across)
    side_across = BOBGUI_PACK_END;
  else
    side_across = BOBGUI_PACK_START;

  if (cell_area.y < area.y && cell_area.y + cell_area.height <= area.y + area.height)
    side_along = BOBGUI_PACK_END;
  else if (cell_area.y >= area.y && cell_area.y + cell_area.height > area.y + area.height)
    side_along = BOBGUI_PACK_START;
  else if (cell_area.y + cell_area.height / 2 > along)
    side_along = BOBGUI_PACK_END;
  else
    side_along = BOBGUI_PACK_START;

  /* Compute the align based on side to keep the values identical */
  if (side_across == BOBGUI_PACK_START)
    align_across = (double) (cell_area.x - area.x) / area.width;
  else
    align_across = (double) (cell_area.x + cell_area.width - area.x) / area.width;
  if (side_along == BOBGUI_PACK_START)
    align_along = (double) (cell_area.y - area.y) / area.height;
  else
    align_along = (double) (cell_area.y + cell_area.height - area.y) / area.height;

  bobgui_list_base_set_anchor (self,
                            pos,
                            align_across, side_across,
                            align_along, side_along);

  bobgui_widget_queue_allocate (BOBGUI_WIDGET (self));
}

static void
bobgui_list_base_clear_adjustment (BobguiListBase    *self,
                                BobguiOrientation  orientation)
{
  BobguiListBasePrivate *priv = bobgui_list_base_get_instance_private (self);

  if (priv->adjustment[orientation] == NULL)
    return;

  g_signal_handlers_disconnect_by_func (priv->adjustment[orientation],
                                        bobgui_list_base_adjustment_value_changed_cb,
                                        self);
  g_clear_object (&priv->adjustment[orientation]);
}

/*
 * bobgui_list_base_move_focus_along:
 * @self: a `BobguiListBase`
 * @pos: position from which to move focus
 * @steps: steps to move focus - negative numbers move focus backwards
 *
 * Moves focus @steps in the direction of the list.
 * If focus cannot be moved, @pos is returned.
 * If focus should be moved out of the widget, %BOBGUI_INVALID_LIST_POSITION
 * is returned.
 *
 * Returns: new focus position
 **/
static guint
bobgui_list_base_move_focus_along (BobguiListBase *self,
                                guint        pos,
                                int          steps)
{
  return BOBGUI_LIST_BASE_GET_CLASS (self)->move_focus_along (self, pos, steps);
}

/*
 * bobgui_list_base_move_focus_across:
 * @self: a `BobguiListBase`
 * @pos: position from which to move focus
 * @steps: steps to move focus - negative numbers move focus backwards
 *
 * Moves focus @steps in the direction across the list.
 * If focus cannot be moved, @pos is returned.
 * If focus should be moved out of the widget, %BOBGUI_INVALID_LIST_POSITION
 * is returned.
 *
 * Returns: new focus position
 **/
static guint
bobgui_list_base_move_focus_across (BobguiListBase *self,
                                 guint        pos,
                                 int          steps)
{
  return BOBGUI_LIST_BASE_GET_CLASS (self)->move_focus_across (self, pos, steps);
}

static guint
bobgui_list_base_move_focus (BobguiListBase    *self,
                          guint           pos,
                          BobguiOrientation  orientation,
                          int             steps)
{
  BobguiListBasePrivate *priv = bobgui_list_base_get_instance_private (self);

  if (orientation == BOBGUI_ORIENTATION_HORIZONTAL &&
      bobgui_widget_get_direction (BOBGUI_WIDGET (self)) == BOBGUI_TEXT_DIR_RTL)
    steps = -steps;

  if (orientation == priv->orientation)
    return bobgui_list_base_move_focus_along (self, pos, steps);
  else
    return bobgui_list_base_move_focus_across (self, pos, steps);
}

/*
 * bobgui_list_base_get_allocation:
 * @self: a `BobguiListBase`
 * @pos: item to get the area of
 * @area: (out caller-allocates): set to the area
 *   occupied by the item
 *
 * Computes the allocation of the item in the given position
 *
 * Returns: %TRUE if the item exists and has an allocation, %FALSE otherwise
 **/
static gboolean
bobgui_list_base_get_allocation (BobguiListBase  *self,
                              guint         pos,
                              GdkRectangle *area)
{
  return BOBGUI_LIST_BASE_GET_CLASS (self)->get_allocation (self, pos, area);
}

/*
 * bobgui_list_base_select_item:
 * @self: a `BobguiListBase`
 * @pos: item to select
 * @modify: %TRUE if the selection should be modified, %FALSE
 *   if a new selection should be done. This is usually set
 *   to %TRUE if the user keeps the `<Shift>` key pressed.
 * @extend_pos: %TRUE if the selection should be extended.
 *   Selections are usually extended from the last selected
 *   position if the user presses the `<Ctrl>` key.
 *
 * Selects the item at @pos according to how BOBGUI list widgets modify
 * selections, both when clicking rows with the mouse or when using
 * the keyboard.
 **/
static void
bobgui_list_base_select_item (BobguiListBase *self,
                           guint        pos,
                           gboolean     modify,
                           gboolean     extend)
{
  BobguiListBasePrivate *priv = bobgui_list_base_get_instance_private (self);
  BobguiSelectionModel *model;
  gboolean success = FALSE;
  guint n_items;

  model = bobgui_list_item_manager_get_model (priv->item_manager);
  if (model == NULL)
    return;

  n_items = g_list_model_get_n_items (G_LIST_MODEL (model));
  if (pos >= n_items)
    return;

  if (extend)
    {
      guint extend_pos = bobgui_list_item_tracker_get_position (priv->item_manager, priv->selected);

      if (extend_pos < n_items)
        {
          guint max = MAX (extend_pos, pos);
          guint min = MIN (extend_pos, pos);

          if (modify)
            {
              if (bobgui_selection_model_is_selected (model, extend_pos))
                {
                  success = bobgui_selection_model_select_range (model,
                                                              min,
                                                              max - min + 1,
                                                              FALSE);
                }
              else
                {
                  success = bobgui_selection_model_unselect_range (model,
                                                                min,
                                                                max - min + 1);
                }
            }
          else
            {
              success = bobgui_selection_model_select_range (model,
                                                          min,
                                                          max - min + 1,
                                                          TRUE);
            }
        }
      /* If there's no range to select or selecting ranges isn't supported
       * by the model, fall through to normal setting.
       */
    }

  if (success)
    return;

  if (modify)
    {
      if (bobgui_selection_model_is_selected (model, pos))
        bobgui_selection_model_unselect_item (model, pos);
      else
        bobgui_selection_model_select_item (model, pos, FALSE);
    }
  else
    {
      bobgui_selection_model_select_item (model, pos, TRUE);
    }

  bobgui_list_item_tracker_set_position (priv->item_manager,
                                      priv->selected,
                                      pos,
                                      0, 0);
}

static void
activate_listitem_select_action (BobguiListBasePrivate *priv,
                                 guint               pos,
                                 gboolean            modify,
                                 gboolean            extend)
{
  BobguiListTile *tile;

  tile = bobgui_list_item_manager_get_nth (priv->item_manager, pos, NULL);

  /* We do this convoluted calling into the widget because that way
   * BobguiListItem::selectable gets respected, which is what one would expect.
   */
  g_assert (tile->widget);
  bobgui_widget_activate_action (tile->widget, "listitem.select", "(bb)", modify, extend);
}

/*
 * bobgui_list_base_grab_focus_on_item:
 * @self: a `BobguiListBase`
 * @pos: position of the item to focus
 * @select: %TRUE to select the item
 * @modify: if selecting, %TRUE to modify the selected
 *   state, %FALSE to always select
 * @extend: if selecting, %TRUE to extend the selection,
 *   %FALSE to only operate on this item
 *
 * Tries to grab focus on the given item. If there is no item
 * at this position or grabbing focus failed, %FALSE will be
 * returned.
 *
 * Returns: %TRUE if focusing the item succeeded
 **/
static gboolean
bobgui_list_base_grab_focus_on_item (BobguiListBase *self,
                                  guint        pos,
                                  gboolean     select,
                                  gboolean     modify,
                                  gboolean     extend)
{
  BobguiListBasePrivate *priv = bobgui_list_base_get_instance_private (self);
  BobguiListTile *tile;
  gboolean success;

  tile = bobgui_list_item_manager_get_nth (priv->item_manager, pos, NULL);
  if (tile == NULL)
    return FALSE;

  if (!tile->widget)
    {
      BobguiListItemTracker *tracker = bobgui_list_item_tracker_new (priv->item_manager);

      /* We need a tracker here to create the widget.
       * That needs to have happened or we can't grab it.
       * And we can't use a different tracker, because they manage important rows,
       * so we create a temporary one. */
      bobgui_list_item_tracker_set_position (priv->item_manager, tracker, pos, 0, 0);

      tile = bobgui_list_item_manager_get_nth (priv->item_manager, pos, NULL);
      g_assert (tile->widget);

      success = bobgui_widget_grab_focus (tile->widget);

      bobgui_list_item_tracker_free (priv->item_manager, tracker);
    }
  else
    {
      success = bobgui_widget_grab_focus (tile->widget);
    }

  if (!success)
    return FALSE;

  if (select)
    {
      activate_listitem_select_action (priv, pos, modify, extend);
    }

  return TRUE;
}

guint
bobgui_list_base_get_n_items (BobguiListBase *self)
{
  BobguiListBasePrivate *priv = bobgui_list_base_get_instance_private (self);

  if (priv->model == NULL)
    return 0;

  return g_list_model_get_n_items (G_LIST_MODEL (priv->model));
}

static guint
bobgui_list_base_get_focus_position (BobguiListBase *self)
{
  BobguiListBasePrivate *priv = bobgui_list_base_get_instance_private (self);

  return bobgui_list_item_tracker_get_position (priv->item_manager, priv->focus);
}

static gboolean
bobgui_list_base_focus (BobguiWidget        *widget,
                     BobguiDirectionType  direction)
{
  BobguiListBase *self = BOBGUI_LIST_BASE (widget);
  BobguiListBasePrivate *priv = bobgui_list_base_get_instance_private (self);
  guint old, pos, n_items;
  BobguiWidget *focus_child;
  BobguiListTile *tile;

  focus_child = bobgui_widget_get_focus_child (widget);
  /* focus is moving around fine inside the focus child, don't disturb it */
  if (focus_child && bobgui_widget_child_focus (focus_child, direction))
    return TRUE;

  pos = bobgui_list_base_get_focus_position (self);
  n_items = bobgui_list_base_get_n_items (self);
  old = pos;

  if (pos >= n_items)
    {
      if (n_items == 0)
        return FALSE;

      pos = 0;
    }
  else if (focus_child == NULL)
    {
      /* Focus was outside the list, just grab the old focus item
       * while keeping the selection intact.
       */
      old = BOBGUI_INVALID_LIST_POSITION;
      if (priv->tab_behavior == BOBGUI_LIST_TAB_ALL)
        {
          if (direction == BOBGUI_DIR_TAB_FORWARD)
            pos = 0;
          else if (direction == BOBGUI_DIR_TAB_BACKWARD)
            pos = n_items - 1;
        }
    }
  else
    {
      switch (direction)
        {
        case BOBGUI_DIR_TAB_FORWARD:
          if (priv->tab_behavior == BOBGUI_LIST_TAB_ALL)
            {
              pos++;
              if (pos >= n_items)
                return FALSE;
            }
          else
            {
              return FALSE;
            }
          break;

        case BOBGUI_DIR_TAB_BACKWARD:
          if (priv->tab_behavior == BOBGUI_LIST_TAB_ALL)
            {
              if (pos == 0)
                return FALSE;
              pos--;
            }
          else
            {
              return FALSE;
            }
          break;

        case BOBGUI_DIR_UP:
          pos = bobgui_list_base_move_focus (self, pos, BOBGUI_ORIENTATION_VERTICAL, -1);
          break;

        case BOBGUI_DIR_DOWN:
          pos = bobgui_list_base_move_focus (self, pos, BOBGUI_ORIENTATION_VERTICAL, 1);
          break;

        case BOBGUI_DIR_LEFT:
          pos = bobgui_list_base_move_focus (self, pos, BOBGUI_ORIENTATION_HORIZONTAL, -1);
          break;

        case BOBGUI_DIR_RIGHT:
          pos = bobgui_list_base_move_focus (self, pos, BOBGUI_ORIENTATION_HORIZONTAL, 1);
          break;

        default:
          g_assert_not_reached ();
          return TRUE;
        }
    }

  if (old == pos)
    return TRUE;

  tile = bobgui_list_item_manager_get_nth (priv->item_manager, pos, NULL);
  if (tile == NULL)
    return FALSE;

  /* This shouldn't really happen, but if it does, oh well */
  if (tile->widget == NULL)
    return bobgui_list_base_grab_focus_on_item (BOBGUI_LIST_BASE (self), pos, TRUE, FALSE, FALSE);

  return bobgui_widget_child_focus (tile->widget, direction);
}

static gboolean
bobgui_list_base_grab_focus (BobguiWidget *widget)
{
  BobguiListBase *self = BOBGUI_LIST_BASE (widget);
  BobguiListBasePrivate *priv = bobgui_list_base_get_instance_private (self);
  guint pos;

  pos = bobgui_list_item_tracker_get_position (priv->item_manager, priv->focus);
  if (bobgui_list_base_grab_focus_on_item (self, pos, FALSE, FALSE, FALSE))
    return TRUE;

  return BOBGUI_WIDGET_CLASS (bobgui_list_base_parent_class)->grab_focus (widget);
}

static void
bobgui_list_base_dispose (GObject *object)
{
  BobguiListBase *self = BOBGUI_LIST_BASE (object);
  BobguiListBasePrivate *priv = bobgui_list_base_get_instance_private (self);

  bobgui_list_base_clear_adjustment (self, BOBGUI_ORIENTATION_HORIZONTAL);
  bobgui_list_base_clear_adjustment (self, BOBGUI_ORIENTATION_VERTICAL);

  if (priv->anchor)
    {
      bobgui_list_item_tracker_free (priv->item_manager, priv->anchor);
      priv->anchor = NULL;
    }
  if (priv->selected)
    {
      bobgui_list_item_tracker_free (priv->item_manager, priv->selected);
      priv->selected = NULL;
    }
  if (priv->focus)
    {
      bobgui_list_item_tracker_free (priv->item_manager, priv->focus);
      priv->focus = NULL;
    }
  g_clear_object (&priv->item_manager);

  g_clear_object (&priv->model);

  G_OBJECT_CLASS (bobgui_list_base_parent_class)->dispose (object);
}

static void
bobgui_list_base_get_property (GObject    *object,
                            guint       property_id,
                            GValue     *value,
                            GParamSpec *pspec)
{
  BobguiListBase *self = BOBGUI_LIST_BASE (object);
  BobguiListBasePrivate *priv = bobgui_list_base_get_instance_private (self);

  switch (property_id)
    {
    case PROP_HADJUSTMENT:
      g_value_set_object (value, priv->adjustment[BOBGUI_ORIENTATION_HORIZONTAL]);
      break;

    case PROP_HSCROLL_POLICY:
      g_value_set_enum (value, priv->scroll_policy[BOBGUI_ORIENTATION_HORIZONTAL]);
      break;

    case PROP_ORIENTATION:
      g_value_set_enum (value, priv->orientation);
      break;

    case PROP_VADJUSTMENT:
      g_value_set_object (value, priv->adjustment[BOBGUI_ORIENTATION_VERTICAL]);
      break;

    case PROP_VSCROLL_POLICY:
      g_value_set_enum (value, priv->scroll_policy[BOBGUI_ORIENTATION_VERTICAL]);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
bobgui_list_base_set_adjustment (BobguiListBase    *self,
                              BobguiOrientation  orientation,
                              BobguiAdjustment  *adjustment)
{
  BobguiListBasePrivate *priv = bobgui_list_base_get_instance_private (self);

  if (priv->adjustment[orientation] == adjustment)
    return;

  if (adjustment == NULL)
    adjustment = bobgui_adjustment_new (0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
  else
    bobgui_adjustment_configure (adjustment, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
  g_object_ref_sink (adjustment);

  bobgui_list_base_clear_adjustment (self, orientation);

  priv->adjustment[orientation] = adjustment;

  g_signal_connect (adjustment, "value-changed",
		    G_CALLBACK (bobgui_list_base_adjustment_value_changed_cb),
		    self);

  bobgui_widget_queue_allocate (BOBGUI_WIDGET (self));
}

static void
bobgui_list_base_set_scroll_policy (BobguiListBase         *self,
                                 BobguiOrientation       orientation,
                                 BobguiScrollablePolicy  scroll_policy)
{
  BobguiListBasePrivate *priv = bobgui_list_base_get_instance_private (self);

  if (priv->scroll_policy[orientation] == scroll_policy)
    return;

  priv->scroll_policy[orientation] = scroll_policy;

  bobgui_widget_queue_resize (BOBGUI_WIDGET (self));

  g_object_notify_by_pspec (G_OBJECT (self),
                            orientation == BOBGUI_ORIENTATION_HORIZONTAL
                            ? properties[PROP_HSCROLL_POLICY]
                            : properties[PROP_VSCROLL_POLICY]);
}

static void
bobgui_list_base_set_property (GObject      *object,
                            guint         property_id,
                            const GValue *value,
                            GParamSpec   *pspec)
{
  BobguiListBase *self = BOBGUI_LIST_BASE (object);
  BobguiListBasePrivate *priv = bobgui_list_base_get_instance_private (self);

  switch (property_id)
    {
    case PROP_HADJUSTMENT:
      bobgui_list_base_set_adjustment (self, BOBGUI_ORIENTATION_HORIZONTAL, g_value_get_object (value));
      break;

    case PROP_HSCROLL_POLICY:
      bobgui_list_base_set_scroll_policy (self, BOBGUI_ORIENTATION_HORIZONTAL, g_value_get_enum (value));
      break;

    case PROP_ORIENTATION:
      {
        BobguiOrientation orientation = g_value_get_enum (value);
        if (priv->orientation != orientation)
          {
            priv->orientation = orientation;
            bobgui_widget_update_orientation (BOBGUI_WIDGET (self), priv->orientation);
            bobgui_widget_queue_resize (BOBGUI_WIDGET (self));
            g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_ORIENTATION]);
          }
      }
      break;

    case PROP_VADJUSTMENT:
      bobgui_list_base_set_adjustment (self, BOBGUI_ORIENTATION_VERTICAL, g_value_get_object (value));
      break;

    case PROP_VSCROLL_POLICY:
      bobgui_list_base_set_scroll_policy (self, BOBGUI_ORIENTATION_VERTICAL, g_value_get_enum (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
bobgui_list_base_compute_scroll_align (int            cell_start,
                                    int            cell_size,
                                    int            visible_start,
                                    int            visible_size,
                                    double         current_align,
                                    BobguiPackType    current_side,
                                    double        *new_align,
                                    BobguiPackType   *new_side)
{
  int cell_end, visible_end;

  visible_end = visible_start + visible_size;
  cell_end = cell_start + cell_size;

  if (cell_size <= visible_size)
    {
      if (cell_start < visible_start)
        {
          *new_align = 0.0;
          *new_side = BOBGUI_PACK_START;
        }
      else if (cell_end > visible_end)
        {
          *new_align = 1.0;
          *new_side = BOBGUI_PACK_END;
        }
      else
        {
          /* XXX: start or end here? */
          *new_side = BOBGUI_PACK_START;
          *new_align = (double) (cell_start - visible_start) / visible_size;
        }
    }
  else
    {
      /* This is the unlikely case of the cell being higher than the visible area */
      if (cell_start > visible_start)
        {
          *new_align = 0.0;
          *new_side = BOBGUI_PACK_START;
        }
      else if (cell_end < visible_end)
        {
          *new_align = 1.0;
          *new_side = BOBGUI_PACK_END;
        }
      else
        {
          /* the cell already covers the whole screen */
          *new_align = current_align;
          *new_side = current_side;
        }
    }
}

static void
bobgui_list_base_scroll_to_item (BobguiListBase   *self,
                              guint          pos,
                              BobguiScrollInfo *scroll)
{
  BobguiListBasePrivate *priv = bobgui_list_base_get_instance_private (self);
  double align_along, align_across;
  BobguiPackType side_along, side_across;
  GdkRectangle area, viewport;
  int x, y;

  if (!bobgui_list_base_get_allocation (BOBGUI_LIST_BASE (self), pos, &area))
    {
      g_clear_pointer (&scroll, bobgui_scroll_info_unref);
      return;
    }

  bobgui_list_base_get_adjustment_values (BOBGUI_LIST_BASE (self),
                                       bobgui_list_base_get_orientation (BOBGUI_LIST_BASE (self)),
                                       &viewport.y, NULL, &viewport.height);
  bobgui_list_base_get_adjustment_values (BOBGUI_LIST_BASE (self),
                                       bobgui_list_base_get_opposite_orientation (BOBGUI_LIST_BASE (self)),
                                       &viewport.x, NULL, &viewport.width);

  bobgui_scroll_info_compute_scroll (scroll, &area, &viewport, &x, &y);

  bobgui_list_base_compute_scroll_align (area.y, area.height,
                                      y, viewport.height,
                                      priv->anchor_align_along, priv->anchor_side_along,
                                      &align_along, &side_along);

  bobgui_list_base_compute_scroll_align (area.x, area.width,
                                      x, viewport.width,
                                      priv->anchor_align_across, priv->anchor_side_across,
                                      &align_across, &side_across);

  bobgui_list_base_set_anchor (self,
                            pos,
                            align_across, side_across,
                            align_along, side_along);

  g_clear_pointer (&scroll, bobgui_scroll_info_unref);
}

static void
bobgui_list_base_scroll_to_item_action (BobguiWidget  *widget,
                                     const char *action_name,
                                     GVariant   *parameter)
{
  BobguiListBase *self = BOBGUI_LIST_BASE (widget);
  guint pos;

  if (!g_variant_check_format_string (parameter, "u", FALSE))
    return;

  g_variant_get (parameter, "u", &pos);

  bobgui_list_base_scroll_to_item (self, pos, NULL);
}

static void
bobgui_list_base_set_focus_child (BobguiWidget *widget,
                               BobguiWidget *child)
{
  BobguiListBase *self = BOBGUI_LIST_BASE (widget);
  BobguiListBasePrivate *priv = bobgui_list_base_get_instance_private (self);
  guint pos;

  BOBGUI_WIDGET_CLASS (bobgui_list_base_parent_class)->set_focus_child (widget, child);

  if (!BOBGUI_IS_LIST_ITEM_BASE (child))
    return;

  pos = bobgui_list_item_base_get_position (BOBGUI_LIST_ITEM_BASE (child));

  if (pos != bobgui_list_item_tracker_get_position (priv->item_manager, priv->focus))
    {
      bobgui_list_base_scroll_to_item (self, pos, NULL);
      bobgui_list_item_tracker_set_position (priv->item_manager,
                                          priv->focus,
                                          pos,
                                          0,
                                          0);
    }
}

static void
bobgui_list_base_select_item_action (BobguiWidget  *widget,
                                  const char *action_name,
                                  GVariant   *parameter)
{
  BobguiListBase *self = BOBGUI_LIST_BASE (widget);
  guint pos;
  gboolean modify, extend;

  g_variant_get (parameter, "(ubb)", &pos, &modify, &extend);

  bobgui_list_base_select_item (self, pos, modify, extend);
}

static void
bobgui_list_base_select_all (BobguiWidget  *widget,
                          const char *action_name,
                          GVariant   *parameter)
{
  BobguiListBase *self = BOBGUI_LIST_BASE (widget);
  BobguiListBasePrivate *priv = bobgui_list_base_get_instance_private (self);
  BobguiSelectionModel *selection_model;

  selection_model = bobgui_list_item_manager_get_model (priv->item_manager);
  if (selection_model == NULL)
    return;

  bobgui_selection_model_select_all (selection_model);
}

static void
bobgui_list_base_unselect_all (BobguiWidget  *widget,
                            const char *action_name,
                            GVariant   *parameter)
{
  BobguiListBase *self = BOBGUI_LIST_BASE (widget);
  BobguiListBasePrivate *priv = bobgui_list_base_get_instance_private (self);
  BobguiSelectionModel *selection_model;

  selection_model = bobgui_list_item_manager_get_model (priv->item_manager);
  if (selection_model == NULL)
    return;

  bobgui_selection_model_unselect_all (selection_model);
}

static gboolean
bobgui_list_base_move_cursor_to_start (BobguiWidget *widget,
                                    GVariant  *args,
                                    gpointer   unused)
{
  BobguiListBase *self = BOBGUI_LIST_BASE (widget);
  gboolean select, modify, extend;

  if (bobgui_list_base_get_n_items (self) == 0)
    return TRUE;

  g_variant_get (args, "(bbb)", &select, &modify, &extend);

  bobgui_list_base_grab_focus_on_item (BOBGUI_LIST_BASE (self), 0, select, modify, extend);

  return TRUE;
}

static gboolean
bobgui_list_base_move_cursor_page_up (BobguiWidget *widget,
                                   GVariant  *args,
                                   gpointer   unused)
{
  BobguiListBase *self = BOBGUI_LIST_BASE (widget);
  BobguiListBasePrivate *priv = bobgui_list_base_get_instance_private (self);
  gboolean select, modify, extend;
  cairo_rectangle_int_t area, new_area;
  int page_size;
  guint pos, new_pos;

  pos = bobgui_list_base_get_focus_position (self);
  page_size = bobgui_adjustment_get_page_size (priv->adjustment[priv->orientation]);
  if (!bobgui_list_base_get_allocation (self, pos, &area))
    return TRUE;
  if (!bobgui_list_base_get_position_from_allocation (self,
                                                   area.x + area.width / 2,
                                                   MAX (0, area.y + area.height - page_size),
                                                   &new_pos,
                                                   &new_area))
    return TRUE;

  /* We want the whole row to be visible */
  if (new_area.y < MAX (0, area.y + area.height - page_size))
    new_pos = bobgui_list_base_move_focus_along (self, new_pos, 1);
  /* But we definitely want to move if we can */
  if (new_pos >= pos)
    {
      new_pos = bobgui_list_base_move_focus_along (self, new_pos, -1);
      if (new_pos == pos)
        return TRUE;
    }

  g_variant_get (args, "(bbb)", &select, &modify, &extend);

  bobgui_list_base_grab_focus_on_item (BOBGUI_LIST_BASE (self), new_pos, select, modify, extend);

  return TRUE;
}

static gboolean
bobgui_list_base_move_cursor_page_down (BobguiWidget *widget,
                                     GVariant  *args,
                                     gpointer   unused)
{
  BobguiListBase *self = BOBGUI_LIST_BASE (widget);
  BobguiListBasePrivate *priv = bobgui_list_base_get_instance_private (self);
  gboolean select, modify, extend;
  cairo_rectangle_int_t area, new_area;
  int page_size, end;
  guint pos, new_pos;

  pos = bobgui_list_base_get_focus_position (self);
  page_size = bobgui_adjustment_get_page_size (priv->adjustment[priv->orientation]);
  end = bobgui_adjustment_get_upper (priv->adjustment[priv->orientation]);
  if (end == 0)
    return TRUE;

  if (!bobgui_list_base_get_allocation (self, pos, &area))
    return TRUE;

  if (!bobgui_list_base_get_position_from_allocation (self,
                                                   area.x + area.width / 2,
                                                   MIN (end, area.y + page_size) - 1,
                                                   &new_pos,
                                                   &new_area))
    return TRUE;

  /* We want the whole row to be visible */
  if (new_area.y + new_area.height > MIN (end, area.y + page_size))
    new_pos = bobgui_list_base_move_focus_along (self, new_pos, -1);
  /* But we definitely want to move if we can */
  if (new_pos <= pos)
    {
      new_pos = bobgui_list_base_move_focus_along (self, new_pos, 1);
      if (new_pos == pos)
        return TRUE;
    }

  g_variant_get (args, "(bbb)", &select, &modify, &extend);

  bobgui_list_base_grab_focus_on_item (BOBGUI_LIST_BASE (self), new_pos, select, modify, extend);

  return TRUE;
}

static gboolean
bobgui_list_base_move_cursor_to_end (BobguiWidget *widget,
                                  GVariant  *args,
                                  gpointer   unused)
{
  BobguiListBase *self = BOBGUI_LIST_BASE (widget);
  gboolean select, modify, extend;
  guint n_items;

  n_items = bobgui_list_base_get_n_items (self);
  if (n_items == 0)
    return TRUE;

  g_variant_get (args, "(bbb)", &select, &modify, &extend);

  bobgui_list_base_grab_focus_on_item (BOBGUI_LIST_BASE (self), n_items - 1, select, modify, extend);

  return TRUE;
}

static gboolean
handle_selecting_unselected_cursor (BobguiListBase *self,
                                    guint        position,
                                    gboolean     select,
                                    gboolean     modify,
                                    gboolean     extend)
{
  BobguiListBasePrivate *priv = bobgui_list_base_get_instance_private (self);
  BobguiSelectionModel *model;

  /* If Ctrl is pressed, we don't want to reset the selection. */
  if (!select || modify)
    return FALSE;

  model = bobgui_list_item_manager_get_model (priv->item_manager);

  /* Selection of current position is not needed if it's already selected or if
   * there is nothing to select, or the position is invalid.
   */
  if (model == NULL || position == BOBGUI_INVALID_LIST_POSITION || bobgui_selection_model_is_selected (model, position))
    return FALSE;

  /* Reset cursor to current position trying to select it as well. */
  activate_listitem_select_action (priv, position, FALSE, extend);

  /* Report whether the model allowed the selection change. */
  return bobgui_selection_model_is_selected (model, position);
}

static gboolean
bobgui_list_base_move_cursor (BobguiWidget *widget,
                           GVariant  *args,
                           gpointer   unused)
{
  BobguiListBase *self = BOBGUI_LIST_BASE (widget);
  int amount;
  guint orientation;
  guint old_pos, new_pos;
  gboolean select, modify, extend;

  g_variant_get (args, "(ubbbi)", &orientation, &select, &modify, &extend, &amount);

  old_pos = bobgui_list_base_get_focus_position (self);

  /* When the focus is on an unselected item while we're selecting, we want to
   * not move focus but select the focused item instead if we can.
   */
  if (handle_selecting_unselected_cursor (self, old_pos, select, modify, extend))
    return TRUE;

  new_pos = bobgui_list_base_move_focus (self, old_pos, orientation, amount);

  if (old_pos != new_pos)
    bobgui_list_base_grab_focus_on_item (BOBGUI_LIST_BASE (self), new_pos, select, modify, extend);

  return TRUE;
}

static void
bobgui_list_base_add_move_binding (BobguiWidgetClass *widget_class,
                                guint           keyval,
                                BobguiOrientation  orientation,
                                int             amount)
{
  bobgui_widget_class_add_binding (widget_class,
                                keyval,
                                0,
                                bobgui_list_base_move_cursor,
                                "(ubbbi)", orientation, TRUE, FALSE, FALSE, amount);
  bobgui_widget_class_add_binding (widget_class,
                                keyval,
                                GDK_CONTROL_MASK,
                                bobgui_list_base_move_cursor,
                                "(ubbbi)", orientation, FALSE, FALSE, FALSE, amount);
  bobgui_widget_class_add_binding (widget_class,
                                keyval,
                                GDK_SHIFT_MASK,
                                bobgui_list_base_move_cursor,
                                "(ubbbi)", orientation, TRUE, FALSE, TRUE, amount);
  bobgui_widget_class_add_binding (widget_class,
                                keyval,
                                GDK_CONTROL_MASK | GDK_SHIFT_MASK,
                                bobgui_list_base_move_cursor,
                                "(ubbbi)", orientation, TRUE, TRUE, TRUE, amount);
}

static void
bobgui_list_base_add_custom_move_binding (BobguiWidgetClass  *widget_class,
                                       guint            keyval,
                                       BobguiShortcutFunc  callback)
{
  bobgui_widget_class_add_binding (widget_class,
                                keyval,
                                0,
                                callback,
                                "(bbb)", TRUE, FALSE, FALSE);
  bobgui_widget_class_add_binding (widget_class,
                                keyval,
                                GDK_CONTROL_MASK,
                                callback,
                                "(bbb)", FALSE, FALSE, FALSE);
  bobgui_widget_class_add_binding (widget_class,
                                keyval,
                                GDK_SHIFT_MASK,
                                callback,
                                "(bbb)", TRUE, FALSE, TRUE);
  bobgui_widget_class_add_binding (widget_class,
                                keyval,
                                GDK_CONTROL_MASK | GDK_SHIFT_MASK,
                                callback,
                                "(bbb)", TRUE, TRUE, TRUE);
}

static void
bobgui_list_base_class_init (BobguiListBaseClass *klass)
{
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (klass);
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  gpointer iface;

  widget_class->focus = bobgui_list_base_focus;
  widget_class->grab_focus = bobgui_list_base_grab_focus;
  widget_class->set_focus_child = bobgui_list_base_set_focus_child;

  gobject_class->dispose = bobgui_list_base_dispose;
  gobject_class->get_property = bobgui_list_base_get_property;
  gobject_class->set_property = bobgui_list_base_set_property;

  /* BobguiScrollable implementation */
  iface = g_type_default_interface_peek (BOBGUI_TYPE_SCROLLABLE);
  properties[PROP_HADJUSTMENT] =
      g_param_spec_override ("hadjustment",
                             g_object_interface_find_property (iface, "hadjustment"));
  properties[PROP_HSCROLL_POLICY] =
      g_param_spec_override ("hscroll-policy",
                             g_object_interface_find_property (iface, "hscroll-policy"));
  properties[PROP_VADJUSTMENT] =
      g_param_spec_override ("vadjustment",
                             g_object_interface_find_property (iface, "vadjustment"));
  properties[PROP_VSCROLL_POLICY] =
      g_param_spec_override ("vscroll-policy",
                             g_object_interface_find_property (iface, "vscroll-policy"));

  /**
   * BobguiListBase:orientation:
   *
   * The orientation of the list. See BobguiOrientable:orientation
   * for details.
   */
  properties[PROP_ORIENTATION] =
    g_param_spec_enum ("orientation", NULL, NULL,
                       BOBGUI_TYPE_ORIENTATION,
                       BOBGUI_ORIENTATION_VERTICAL,
                       G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (gobject_class, N_PROPS, properties);

  /**
   * BobguiListBase|list.scroll-to-item:
   * @position: position of item to scroll to
   *
   * Moves the visible area to the item given in @position with the minimum amount
   * of scrolling required. If the item is already visible, nothing happens.
   */
  bobgui_widget_class_install_action (widget_class,
                                   "list.scroll-to-item",
                                   "u",
                                   bobgui_list_base_scroll_to_item_action);

  /**
   * BobguiListBase|list.select-item:
   * @position: position of item to select
   * @modify: %TRUE to toggle the existing selection, %FALSE to select
   * @extend: %TRUE to extend the selection
   *
   * Changes selection.
   *
   * If @extend is %TRUE and the model supports selecting ranges, the
   * affected items are all items from the last selected item to the item
   * in @position.
   * If @extend is %FALSE or selecting ranges is not supported, only the
   * item in @position is affected.
   *
   * If @modify is %TRUE, the affected items will be set to the same state.
   * If @modify is %FALSE, the affected items will be selected and
   * all other items will be deselected.
   */
  bobgui_widget_class_install_action (widget_class,
                                   "list.select-item",
                                   "(ubb)",
                                   bobgui_list_base_select_item_action);

  /**
   * BobguiListBase|list.select-all:
   *
   * If the selection model supports it, select all items in the model.
   * If not, do nothing.
   */
  bobgui_widget_class_install_action (widget_class,
                                   "list.select-all",
                                   NULL,
                                   bobgui_list_base_select_all);

  /**
   * BobguiListBase|list.unselect-all:
   *
   * If the selection model supports it, unselect all items in the model.
   * If not, do nothing.
   */
  bobgui_widget_class_install_action (widget_class,
                                   "list.unselect-all",
                                   NULL,
                                   bobgui_list_base_unselect_all);

  bobgui_list_base_add_move_binding (widget_class, GDK_KEY_Up, BOBGUI_ORIENTATION_VERTICAL, -1);
  bobgui_list_base_add_move_binding (widget_class, GDK_KEY_KP_Up, BOBGUI_ORIENTATION_VERTICAL, -1);
  bobgui_list_base_add_move_binding (widget_class, GDK_KEY_Down, BOBGUI_ORIENTATION_VERTICAL, 1);
  bobgui_list_base_add_move_binding (widget_class, GDK_KEY_KP_Down, BOBGUI_ORIENTATION_VERTICAL, 1);
  bobgui_list_base_add_move_binding (widget_class, GDK_KEY_Left, BOBGUI_ORIENTATION_HORIZONTAL, -1);
  bobgui_list_base_add_move_binding (widget_class, GDK_KEY_KP_Left, BOBGUI_ORIENTATION_HORIZONTAL, -1);
  bobgui_list_base_add_move_binding (widget_class, GDK_KEY_Right, BOBGUI_ORIENTATION_HORIZONTAL, 1);
  bobgui_list_base_add_move_binding (widget_class, GDK_KEY_KP_Right, BOBGUI_ORIENTATION_HORIZONTAL, 1);

  bobgui_list_base_add_custom_move_binding (widget_class, GDK_KEY_Home, bobgui_list_base_move_cursor_to_start);
  bobgui_list_base_add_custom_move_binding (widget_class, GDK_KEY_KP_Home, bobgui_list_base_move_cursor_to_start);
  bobgui_list_base_add_custom_move_binding (widget_class, GDK_KEY_End, bobgui_list_base_move_cursor_to_end);
  bobgui_list_base_add_custom_move_binding (widget_class, GDK_KEY_KP_End, bobgui_list_base_move_cursor_to_end);
  bobgui_list_base_add_custom_move_binding (widget_class, GDK_KEY_Page_Up, bobgui_list_base_move_cursor_page_up);
  bobgui_list_base_add_custom_move_binding (widget_class, GDK_KEY_KP_Page_Up, bobgui_list_base_move_cursor_page_up);
  bobgui_list_base_add_custom_move_binding (widget_class, GDK_KEY_Page_Down, bobgui_list_base_move_cursor_page_down);
  bobgui_list_base_add_custom_move_binding (widget_class, GDK_KEY_KP_Page_Down, bobgui_list_base_move_cursor_page_down);

#ifdef __APPLE__
  bobgui_widget_class_add_binding_action (widget_class, GDK_KEY_a, GDK_META_MASK, "list.select-all", NULL);
  bobgui_widget_class_add_binding_action (widget_class, GDK_KEY_A, GDK_META_MASK | GDK_SHIFT_MASK, "list.unselect-all", NULL);
#else
  bobgui_widget_class_add_binding_action (widget_class, GDK_KEY_a, GDK_CONTROL_MASK, "list.select-all", NULL);
  bobgui_widget_class_add_binding_action (widget_class, GDK_KEY_slash, GDK_CONTROL_MASK, "list.select-all", NULL);
  bobgui_widget_class_add_binding_action (widget_class, GDK_KEY_A, GDK_CONTROL_MASK | GDK_SHIFT_MASK, "list.unselect-all", NULL);
  bobgui_widget_class_add_binding_action (widget_class, GDK_KEY_backslash, GDK_CONTROL_MASK, "list.unselect-all", NULL);
#endif
}

static gboolean
autoscroll_cb (BobguiWidget     *widget,
               GdkFrameClock *frame_clock,
               gpointer       data)
{
  BobguiListBase *self = data;
  BobguiListBasePrivate *priv = bobgui_list_base_get_instance_private (self);
  double value;
  double delta_x, delta_y;

  value = bobgui_adjustment_get_value (priv->adjustment[BOBGUI_ORIENTATION_HORIZONTAL]);
  bobgui_adjustment_set_value (priv->adjustment[BOBGUI_ORIENTATION_HORIZONTAL], value + priv->autoscroll_delta_x);

  delta_x = bobgui_adjustment_get_value (priv->adjustment[BOBGUI_ORIENTATION_HORIZONTAL]) - value;

  value = bobgui_adjustment_get_value (priv->adjustment[BOBGUI_ORIENTATION_VERTICAL]);
  bobgui_adjustment_set_value (priv->adjustment[BOBGUI_ORIENTATION_VERTICAL], value + priv->autoscroll_delta_y);

  delta_y = bobgui_adjustment_get_value (priv->adjustment[BOBGUI_ORIENTATION_VERTICAL]) - value;

  if (delta_x != 0 || delta_y != 0)
    {
      return G_SOURCE_CONTINUE;
    }
  else
    {
      priv->autoscroll_id = 0;
      return G_SOURCE_REMOVE;
    }
}

static void
add_autoscroll (BobguiListBase *self,
                double       delta_x,
                double       delta_y)
{
  BobguiListBasePrivate *priv = bobgui_list_base_get_instance_private (self);

  if (bobgui_list_base_adjustment_is_flipped (self, BOBGUI_ORIENTATION_HORIZONTAL))
    priv->autoscroll_delta_x = -delta_x;
  else
    priv->autoscroll_delta_x = delta_x;
  if (bobgui_list_base_adjustment_is_flipped (self, BOBGUI_ORIENTATION_VERTICAL))
    priv->autoscroll_delta_y = -delta_y;
  else
    priv->autoscroll_delta_y = delta_y;

  if (priv->autoscroll_id == 0)
    priv->autoscroll_id = bobgui_widget_add_tick_callback (BOBGUI_WIDGET (self), autoscroll_cb, self, NULL);
}

static void
remove_autoscroll (BobguiListBase *self)
{
  BobguiListBasePrivate *priv = bobgui_list_base_get_instance_private (self);

  if (priv->autoscroll_id != 0)
    {
      bobgui_widget_remove_tick_callback (BOBGUI_WIDGET (self), priv->autoscroll_id);
      priv->autoscroll_id = 0;
    }
}

#define SCROLL_EDGE_SIZE 30

static void
update_autoscroll (BobguiListBase *self,
                   double       x,
                   double       y)
{
  double width, height;
  double delta_x, delta_y;

  width = bobgui_widget_get_width (BOBGUI_WIDGET (self));

  if (x < SCROLL_EDGE_SIZE)
    delta_x = - (SCROLL_EDGE_SIZE - x)/3.0;
  else if (width - x < SCROLL_EDGE_SIZE)
    delta_x = (SCROLL_EDGE_SIZE - (width - x))/3.0;
  else
    delta_x = 0;

  if (bobgui_widget_get_direction (BOBGUI_WIDGET (self)) == BOBGUI_TEXT_DIR_RTL)
    delta_x = - delta_x;

  height = bobgui_widget_get_height (BOBGUI_WIDGET (self));

  if (y < SCROLL_EDGE_SIZE)
    delta_y = - (SCROLL_EDGE_SIZE - y)/3.0;
  else if (height - y < SCROLL_EDGE_SIZE)
    delta_y = (SCROLL_EDGE_SIZE - (height - y))/3.0;
  else
    delta_y = 0;

  if (delta_x != 0 || delta_y != 0)
    add_autoscroll (self, delta_x, delta_y);
  else
    remove_autoscroll (self);
}

/*
 * bobgui_list_base_size_allocate_child:
 * @self: The listbase
 * @boxes: The CSS boxes of @self to allow for proper
 *     clipping
 * @child: The child
 * @x: top left coordinate in the across direction
 * @y: top right coordinate in the along direction
 * @width: size in the across direction
 * @height: size in the along direction
 *
 * Allocates a child widget in the list coordinate system,
 * but with the coordinates already offset by the scroll
 * offset.
 **/
static void
bobgui_list_base_size_allocate_child (BobguiListBase *self,
                                   BobguiCssBoxes *boxes,
                                   BobguiWidget   *child,
                                   int          x,
                                   int          y,
                                   int          width,
                                   int          height)
{
  BobguiAllocation child_allocation;
  int self_width;

  self_width = bobgui_widget_get_width (BOBGUI_WIDGET (self));

  if (bobgui_list_base_get_orientation (BOBGUI_LIST_BASE (self)) == BOBGUI_ORIENTATION_VERTICAL)
    {
      if (_bobgui_widget_get_direction (BOBGUI_WIDGET (self)) == BOBGUI_TEXT_DIR_LTR)
        {
          child_allocation.x = x;
          child_allocation.y = y;
        }
      else
        {
          child_allocation.x = self_width - x - width;
          child_allocation.y = y;
        }
      child_allocation.width = width;
      child_allocation.height = height;
    }
  else
    {
      if (_bobgui_widget_get_direction (BOBGUI_WIDGET (self)) == BOBGUI_TEXT_DIR_LTR)
        {
          child_allocation.x = y;
          child_allocation.y = x;
        }
      else
        {
          child_allocation.x = self_width - y - height;
          child_allocation.y = x;
        }
      child_allocation.width = height;
      child_allocation.height = width;
    }

  if (!graphene_rect_intersection (bobgui_css_boxes_get_padding_rect (boxes),
                                   &GRAPHENE_RECT_INIT(
                                     child_allocation.x - BOBGUI_LIST_BASE_CHILD_MAX_OVERDRAW,
                                     child_allocation.y - BOBGUI_LIST_BASE_CHILD_MAX_OVERDRAW,
                                     child_allocation.width + 2 * BOBGUI_LIST_BASE_CHILD_MAX_OVERDRAW,
                                     child_allocation.height + 2 * BOBGUI_LIST_BASE_CHILD_MAX_OVERDRAW
                                   ),
                                   NULL))
    {
      /* child is fully outside the viewport, hide it and don't allocate it */
      bobgui_widget_set_child_visible (child, FALSE);
      return;
    }

  bobgui_widget_set_child_visible (child, TRUE);

  bobgui_widget_size_allocate (child, &child_allocation, -1);
}

static void
bobgui_list_base_allocate_children (BobguiListBase *self,
                                 BobguiCssBoxes *boxes)
{
  BobguiListBasePrivate *priv = bobgui_list_base_get_instance_private (self);
  BobguiListTile *tile;
  int dx, dy;

  bobgui_list_base_get_adjustment_values (self, OPPOSITE_ORIENTATION (priv->orientation), &dx, NULL, NULL);
  bobgui_list_base_get_adjustment_values (self, priv->orientation, &dy, NULL, NULL);

  for (tile = bobgui_list_item_manager_get_first (priv->item_manager);
       tile != NULL;
       tile = bobgui_rb_tree_node_get_next (tile))
    {
      if (tile->widget)
        {
          bobgui_list_base_size_allocate_child (BOBGUI_LIST_BASE (self),
                                             boxes,
                                             tile->widget,
                                             tile->area.x - dx,
                                             tile->area.y - dy,
                                             tile->area.width,
                                             tile->area.height);
        }
    }
}

static void
bobgui_list_base_widget_to_list (BobguiListBase *self,
                              double       x_widget,
                              double       y_widget,
                              int         *across_out,
                              int         *along_out)
{
  BobguiListBasePrivate *priv = bobgui_list_base_get_instance_private (self);
  BobguiWidget *widget = BOBGUI_WIDGET (self);

  if (bobgui_widget_get_direction (widget) == BOBGUI_TEXT_DIR_RTL)
    x_widget = bobgui_widget_get_width (widget) - x_widget;

  bobgui_list_base_get_adjustment_values (self, OPPOSITE_ORIENTATION (priv->orientation), across_out, NULL, NULL);
  bobgui_list_base_get_adjustment_values (self, priv->orientation, along_out, NULL, NULL);

  if (priv->orientation == BOBGUI_ORIENTATION_VERTICAL)
    {
      *across_out += x_widget;
      *along_out += y_widget;
    }
  else
    {
      *across_out += y_widget;
      *along_out += x_widget;
    }
}

static BobguiBitset *
bobgui_list_base_get_items_in_rect (BobguiListBase        *self,
                                 const GdkRectangle *rect)
{
  BobguiListBasePrivate *priv = bobgui_list_base_get_instance_private (self);
  GdkRectangle bounds;

  bobgui_list_item_manager_get_tile_bounds (priv->item_manager, &bounds);
  if (!gdk_rectangle_intersect (&bounds, rect, &bounds))
    return bobgui_bitset_new_empty ();

  return BOBGUI_LIST_BASE_GET_CLASS (self)->get_items_in_rect (self, &bounds);
}

static gboolean
bobgui_list_base_get_rubberband_coords (BobguiListBase  *self,
                                     GdkRectangle *rect)
{
  BobguiListBasePrivate *priv = bobgui_list_base_get_instance_private (self);
  int x1, x2, y1, y2;

  if (!priv->rubberband)
    return FALSE;

  if (priv->rubberband->start_tracker == NULL)
    {
      x1 = 0;
      y1 = 0;
    }
  else
    {
      GdkRectangle area;
      guint pos = bobgui_list_item_tracker_get_position (priv->item_manager, priv->rubberband->start_tracker);

      if (bobgui_list_base_get_allocation (self, pos, &area))
        {
          x1 = area.x + area.width * priv->rubberband->start_align_across;
          y1 = area.y + area.height * priv->rubberband->start_align_along;
        }
      else
        {
          x1 = 0;
          y1 = 0;
        }
    }

  bobgui_list_base_widget_to_list (self,
                                priv->rubberband->pointer_x, priv->rubberband->pointer_y,
                                &x2, &y2);

  rect->x = MIN (x1, x2);
  rect->y = MIN (y1, y2);
  rect->width = ABS (x1 - x2) + 1;
  rect->height = ABS (y1 - y2) + 1;

  return TRUE;
}

static void
bobgui_list_base_allocate_rubberband (BobguiListBase *self,
                                   BobguiCssBoxes *boxes)
{
  BobguiListBasePrivate *priv = bobgui_list_base_get_instance_private (self);
  BobguiRequisition min_size;
  GdkRectangle rect;
  int offset_x, offset_y;

  if (!bobgui_list_base_get_rubberband_coords (self, &rect))
    return;

  bobgui_widget_get_preferred_size (priv->rubberband->widget, &min_size, NULL);
  rect.width = MAX (min_size.width, rect.width);
  rect.height = MAX (min_size.height, rect.height);

  bobgui_list_base_get_adjustment_values (self, OPPOSITE_ORIENTATION (priv->orientation), &offset_x, NULL, NULL);
  bobgui_list_base_get_adjustment_values (self, priv->orientation, &offset_y, NULL, NULL);
  rect.x -= offset_x;
  rect.y -= offset_y;

  bobgui_list_base_size_allocate_child (self,
                                     boxes,
                                     priv->rubberband->widget,
                                     rect.x, rect.y, rect.width, rect.height);
}

static void
bobgui_list_base_start_rubberband (BobguiListBase *self,
                                double       x,
                                double       y)
{
  BobguiListBasePrivate *priv = bobgui_list_base_get_instance_private (self);
  cairo_rectangle_int_t item_area;
  int list_x, list_y;
  guint pos;

  if (priv->rubberband)
    return;

  bobgui_list_base_widget_to_list (self, x, y, &list_x, &list_y);
  if (!bobgui_list_base_get_position_from_allocation (self, list_x, list_y, &pos, &item_area))
    {
      g_warning ("Could not start rubberbanding: No item\n");
      return;
    }

  priv->rubberband = g_new0 (RubberbandData, 1);

  priv->rubberband->start_tracker = bobgui_list_item_tracker_new (priv->item_manager);
  bobgui_list_item_tracker_set_position (priv->item_manager, priv->rubberband->start_tracker, pos, 0, 0);
  priv->rubberband->start_align_across = (double) (list_x - item_area.x) / item_area.width;
  priv->rubberband->start_align_along = (double) (list_y - item_area.y) / item_area.height;

  priv->rubberband->pointer_x = x;
  priv->rubberband->pointer_y = y;

  priv->rubberband->widget = bobgui_gizmo_new ("rubberband",
                                            NULL, NULL, NULL, NULL, NULL, NULL);
  bobgui_widget_set_parent (priv->rubberband->widget, BOBGUI_WIDGET (self));
}

static void
bobgui_list_base_apply_rubberband_selection (BobguiListBase *self,
                                          gboolean     modify,
                                          gboolean     extend)
{
  BobguiListBasePrivate *priv = bobgui_list_base_get_instance_private (self);
  BobguiSelectionModel *model;

  if (!priv->rubberband)
    return;

  model = bobgui_list_item_manager_get_model (priv->item_manager);
  if (model != NULL)
    {
      BobguiBitset *selected, *mask, *result;
      GdkRectangle rect;
      BobguiBitset *rubberband_selection;

      if (!bobgui_list_base_get_rubberband_coords (self, &rect))
        return;

      rubberband_selection = bobgui_list_base_get_items_in_rect (self, &rect);

      if (modify && extend) /* Ctrl + Shift */
        {
          if (bobgui_bitset_is_empty (rubberband_selection))
            {
              selected = bobgui_bitset_ref (rubberband_selection);
              mask = bobgui_bitset_ref (rubberband_selection);
            }
          else
            {
              BobguiBitset *current;
              guint min = bobgui_bitset_get_minimum (rubberband_selection);
              guint max = bobgui_bitset_get_maximum (rubberband_selection);
              /* toggle the rubberband, keep the rest */
              current = bobgui_selection_model_get_selection_in_range (model, min, max - min + 1);
              selected = bobgui_bitset_copy (current);
              bobgui_bitset_unref (current);
              bobgui_bitset_intersect (selected, rubberband_selection);
              bobgui_bitset_difference (selected, rubberband_selection);

              mask = bobgui_bitset_ref (rubberband_selection);
            }
        }
      else if (modify) /* Ctrl */
        {
          /* select the rubberband, keep the rest */
          selected = bobgui_bitset_ref (rubberband_selection);
          mask = bobgui_bitset_ref (rubberband_selection);
        }
      else if (extend) /* Shift */
        {
          /* unselect the rubberband, keep the rest */
          selected = bobgui_bitset_new_empty ();
          mask = bobgui_bitset_ref (rubberband_selection);
        }
      else /* no modifier */
        {
          /* select the rubberband, clear the rest */
          selected = bobgui_bitset_ref (rubberband_selection);
          mask = bobgui_bitset_new_empty ();
          bobgui_bitset_add_range (mask, 0, g_list_model_get_n_items (G_LIST_MODEL (model)));
        }

      bobgui_selection_model_set_selection (model, selected, mask);

      result = bobgui_selection_model_get_selection (model);

      if (bobgui_bitset_get_size (result) == 1)
        bobgui_list_base_grab_focus_on_item (self, bobgui_bitset_get_minimum (result), TRUE, FALSE, FALSE);

      bobgui_bitset_unref (selected);
      bobgui_bitset_unref (mask);
      bobgui_bitset_unref (result);
      bobgui_bitset_unref (rubberband_selection);
    }
}

static void
bobgui_list_base_stop_rubberband (BobguiListBase *self)
{
  BobguiListBasePrivate *priv = bobgui_list_base_get_instance_private (self);
  BobguiListTile *tile;

  if (!priv->rubberband)
    return;

  for (tile = bobgui_list_item_manager_get_first (priv->item_manager);
       tile != NULL;
       tile = bobgui_rb_tree_node_get_next (tile))
    {
      if (tile->widget)
        bobgui_widget_unset_state_flags (tile->widget, BOBGUI_STATE_FLAG_ACTIVE);
    }

  bobgui_list_item_tracker_free (priv->item_manager, priv->rubberband->start_tracker);
  g_clear_pointer (&priv->rubberband->widget, bobgui_widget_unparent);
  g_free (priv->rubberband);
  priv->rubberband = NULL;

  remove_autoscroll (self);
}

static void
bobgui_list_base_update_rubberband_selection (BobguiListBase *self)
{
  BobguiListBasePrivate *priv = bobgui_list_base_get_instance_private (self);
  BobguiListTile *tile;
  GdkRectangle rect;
  guint pos;
  BobguiBitset *rubberband_selection;

  if (!bobgui_list_base_get_rubberband_coords (self, &rect))
    return;

  rubberband_selection = bobgui_list_base_get_items_in_rect (self, &rect);

  pos = 0;
  for (tile = bobgui_list_item_manager_get_first (priv->item_manager);
       tile != NULL;
       tile = bobgui_rb_tree_node_get_next (tile))
    {
      if (tile->widget)
        {
          if (bobgui_bitset_contains (rubberband_selection, pos))
            bobgui_widget_set_state_flags (tile->widget, BOBGUI_STATE_FLAG_ACTIVE, FALSE);
          else
            bobgui_widget_unset_state_flags (tile->widget, BOBGUI_STATE_FLAG_ACTIVE);
        }

      pos += tile->n_items;
    }

  bobgui_bitset_unref (rubberband_selection);
}

static void
bobgui_list_base_update_rubberband (BobguiListBase *self,
                                 double       x,
                                 double       y)
{
  BobguiListBasePrivate *priv = bobgui_list_base_get_instance_private (self);

  if (!priv->rubberband)
    return;

  priv->rubberband->pointer_x = x;
  priv->rubberband->pointer_y = y;

  bobgui_list_base_update_rubberband_selection (self);

  update_autoscroll (self, x, y);

  bobgui_widget_queue_allocate (BOBGUI_WIDGET (self));
}

static void
get_selection_modifiers (BobguiGesture *gesture,
                         gboolean   *modify,
                         gboolean   *extend)
{
  GdkEventSequence *sequence;
  GdkEvent *event;
  GdkModifierType state;

  *modify = FALSE;
  *extend = FALSE;

  sequence = bobgui_gesture_get_last_updated_sequence (gesture);
  event = bobgui_gesture_get_last_event (gesture, sequence);
  state = gdk_event_get_modifier_state (event);
  if ((state & GDK_CONTROL_MASK) == GDK_CONTROL_MASK)
    *modify = TRUE;
  if ((state & GDK_SHIFT_MASK) == GDK_SHIFT_MASK)
    *extend = TRUE;

#ifdef __APPLE__
  if ((state & GDK_META_MASK) == GDK_META_MASK)
    *modify = TRUE;
#endif
}

static void
bobgui_list_base_drag_update (BobguiGestureDrag *gesture,
                           double          offset_x,
                           double          offset_y,
                           BobguiListBase    *self)
{
  BobguiListBasePrivate *priv = bobgui_list_base_get_instance_private (self);
  double start_x, start_y;

  bobgui_gesture_drag_get_start_point (gesture, &start_x, &start_y);

  if (!priv->rubberband)
    {
      if (!bobgui_drag_check_threshold_double (BOBGUI_WIDGET (self), 0, 0, offset_x, offset_y))
        return;

      bobgui_gesture_set_state (BOBGUI_GESTURE (gesture), BOBGUI_EVENT_SEQUENCE_CLAIMED);
      bobgui_list_base_start_rubberband (self, start_x, start_y);
    }
  bobgui_list_base_update_rubberband (self, start_x + offset_x, start_y + offset_y);
}

static void
bobgui_list_base_drag_end (BobguiGestureDrag *gesture,
                        double          offset_x,
                        double          offset_y,
                        BobguiListBase    *self)
{
  BobguiListBasePrivate *priv = bobgui_list_base_get_instance_private (self);
  GdkEventSequence *sequence;
  gboolean modify, extend;

  if (!priv->rubberband)
    return;

  sequence = bobgui_gesture_get_last_updated_sequence (BOBGUI_GESTURE (gesture));
  if (!bobgui_gesture_handles_sequence (BOBGUI_GESTURE (gesture), sequence))
    {
      bobgui_list_base_stop_rubberband (self);
      return;
    }

  bobgui_list_base_drag_update (gesture, offset_x, offset_y, self);
  get_selection_modifiers (BOBGUI_GESTURE (gesture), &modify, &extend);
  bobgui_list_base_apply_rubberband_selection (self, modify, extend);
  bobgui_list_base_stop_rubberband (self);
}

void
bobgui_list_base_set_enable_rubberband (BobguiListBase *self,
                                     gboolean     enable)
{
  BobguiListBasePrivate *priv = bobgui_list_base_get_instance_private (self);

  if (priv->enable_rubberband == enable)
    return;

  priv->enable_rubberband = enable;

  if (enable)
    {
      priv->drag_gesture = bobgui_gesture_drag_new ();
      g_signal_connect (priv->drag_gesture, "drag-update", G_CALLBACK (bobgui_list_base_drag_update), self);
      g_signal_connect (priv->drag_gesture, "drag-end", G_CALLBACK (bobgui_list_base_drag_end), self);
      bobgui_widget_add_controller (BOBGUI_WIDGET (self), BOBGUI_EVENT_CONTROLLER (priv->drag_gesture));
    }
  else
    {
      bobgui_widget_remove_controller (BOBGUI_WIDGET (self), BOBGUI_EVENT_CONTROLLER (priv->drag_gesture));
      priv->drag_gesture = NULL;
    }
}

gboolean
bobgui_list_base_get_enable_rubberband (BobguiListBase *self)
{
  BobguiListBasePrivate *priv = bobgui_list_base_get_instance_private (self);

  return priv->enable_rubberband;
}

static void
bobgui_list_base_drag_motion (BobguiDropControllerMotion *motion,
                           double                   x,
                           double                   y,
                           gpointer                 unused)
{
  BobguiWidget *widget = bobgui_event_controller_get_widget (BOBGUI_EVENT_CONTROLLER (motion));

  update_autoscroll (BOBGUI_LIST_BASE (widget), x, y);
}

static void
bobgui_list_base_drag_leave (BobguiDropControllerMotion *motion,
                          gpointer                 unused)
{
  BobguiWidget *widget = bobgui_event_controller_get_widget (BOBGUI_EVENT_CONTROLLER (motion));

  remove_autoscroll (BOBGUI_LIST_BASE (widget));
}

static BobguiListTile *
bobgui_list_base_split_func (BobguiWidget   *widget,
                          BobguiListTile *tile,
                          guint        n_items)
{
  return BOBGUI_LIST_BASE_GET_CLASS (widget)->split (BOBGUI_LIST_BASE (widget), tile, n_items);
}

static BobguiListItemBase *
bobgui_list_base_create_list_widget_func (BobguiWidget *widget)
{
  return BOBGUI_LIST_BASE_GET_CLASS (widget)->create_list_widget (BOBGUI_LIST_BASE (widget));
}

static void
bobgui_list_base_prepare_section_func (BobguiWidget   *widget,
                                    BobguiListTile *tile,
                                    guint        pos)
{
  BOBGUI_LIST_BASE_GET_CLASS (widget)->prepare_section (BOBGUI_LIST_BASE (widget), tile, pos);
}

static BobguiListHeaderBase *
bobgui_list_base_create_header_widget_func (BobguiWidget *widget)
{
  return BOBGUI_LIST_BASE_GET_CLASS (widget)->create_header_widget (BOBGUI_LIST_BASE (widget));
}

static void
bobgui_list_base_init_real (BobguiListBase      *self,
                         BobguiListBaseClass *g_class)
{
  BobguiListBasePrivate *priv = bobgui_list_base_get_instance_private (self);
  BobguiEventController *controller;

  priv->item_manager = bobgui_list_item_manager_new (BOBGUI_WIDGET (self),
                                                  bobgui_list_base_split_func,
                                                  bobgui_list_base_create_list_widget_func,
                                                  bobgui_list_base_prepare_section_func,
                                                  bobgui_list_base_create_header_widget_func);
  priv->anchor = bobgui_list_item_tracker_new (priv->item_manager);
  priv->anchor_side_along = BOBGUI_PACK_START;
  priv->anchor_side_across = BOBGUI_PACK_START;
  priv->selected = bobgui_list_item_tracker_new (priv->item_manager);
  priv->focus = bobgui_list_item_tracker_new (priv->item_manager);

  priv->adjustment[BOBGUI_ORIENTATION_HORIZONTAL] = bobgui_adjustment_new (0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
  g_object_ref_sink (priv->adjustment[BOBGUI_ORIENTATION_HORIZONTAL]);
  priv->adjustment[BOBGUI_ORIENTATION_VERTICAL] = bobgui_adjustment_new (0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
  g_object_ref_sink (priv->adjustment[BOBGUI_ORIENTATION_VERTICAL]);

  priv->tab_behavior = BOBGUI_LIST_TAB_ALL;
  priv->orientation = BOBGUI_ORIENTATION_VERTICAL;

  bobgui_widget_set_overflow (BOBGUI_WIDGET (self), BOBGUI_OVERFLOW_HIDDEN);
  bobgui_widget_set_focusable (BOBGUI_WIDGET (self), TRUE);

  controller = bobgui_drop_controller_motion_new ();
  g_signal_connect (controller, "motion", G_CALLBACK (bobgui_list_base_drag_motion), NULL);
  g_signal_connect (controller, "leave", G_CALLBACK (bobgui_list_base_drag_leave), NULL);
  bobgui_widget_add_controller (BOBGUI_WIDGET (self), controller);
}

static void
bobgui_list_base_set_adjustment_values (BobguiListBase    *self,
                                     BobguiOrientation  orientation,
                                     int             value,
                                     int             size,
                                     int             page_size)
{
  BobguiListBasePrivate *priv = bobgui_list_base_get_instance_private (self);

  size = MAX (size, page_size);
  value = MAX (value, 0);
  value = MIN (value, size - page_size);

  g_signal_handlers_block_by_func (priv->adjustment[orientation],
                                   bobgui_list_base_adjustment_value_changed_cb,
                                   self);
  bobgui_adjustment_configure (priv->adjustment[orientation],
                            bobgui_list_base_adjustment_is_flipped (self, orientation)
                              ? size - page_size - value
                              : value,
                            0,
                            size,
                            page_size * 0.1,
                            page_size * 0.9,
                            page_size);
  g_signal_handlers_unblock_by_func (priv->adjustment[orientation],
                                     bobgui_list_base_adjustment_value_changed_cb,
                                     self);
}

static void
bobgui_list_base_update_adjustments (BobguiListBase *self)
{
  BobguiListBasePrivate *priv = bobgui_list_base_get_instance_private (self);
  GdkRectangle bounds;
  int value_along, value_across;
  int page_along, page_across;
  guint pos;

  bobgui_list_item_manager_get_tile_bounds (priv->item_manager, &bounds);
  g_assert (bounds.x == 0);
  g_assert (bounds.y == 0);

  page_across = bobgui_widget_get_size (BOBGUI_WIDGET (self), OPPOSITE_ORIENTATION (priv->orientation));
  page_along = bobgui_widget_get_size (BOBGUI_WIDGET (self), priv->orientation);

  pos = bobgui_list_item_tracker_get_position (priv->item_manager, priv->anchor);
  if (pos == BOBGUI_INVALID_LIST_POSITION)
    {
      value_across = 0;
      value_along = 0;
    }
  else
    {
      GdkRectangle area;

      if (bobgui_list_base_get_allocation (self, pos, &area))
        {
          value_across = area.x;
          value_along = area.y;
          if (priv->anchor_side_across == BOBGUI_PACK_END)
            value_across += area.width;
          if (priv->anchor_side_along == BOBGUI_PACK_END)
            value_along += area.height;
          value_across -= priv->anchor_align_across * page_across;
          value_along -= priv->anchor_align_along * page_along;
        }
      else
        {
          value_across = 0;
          value_along = 0;
        }
    }

  bobgui_list_base_set_adjustment_values (self,
                                       OPPOSITE_ORIENTATION (priv->orientation),
                                       value_across,
                                       bounds.width,
                                       page_across);
  bobgui_list_base_set_adjustment_values (self,
                                       priv->orientation,
                                       value_along,
                                       bounds.height,
                                       page_along);
}

void
bobgui_list_base_allocate (BobguiListBase *self)
{
  BobguiCssBoxes boxes;

  bobgui_css_boxes_init (&boxes, BOBGUI_WIDGET (self));

  bobgui_list_base_update_adjustments (self);

  bobgui_list_base_allocate_children (self, &boxes);
  bobgui_list_base_allocate_rubberband (self, &boxes);
}

BobguiScrollablePolicy
bobgui_list_base_get_scroll_policy (BobguiListBase    *self,
                                 BobguiOrientation  orientation)
{
  BobguiListBasePrivate *priv = bobgui_list_base_get_instance_private (self);

  return priv->scroll_policy[orientation];
}

BobguiOrientation
bobgui_list_base_get_orientation (BobguiListBase *self)
{
  BobguiListBasePrivate *priv = bobgui_list_base_get_instance_private (self);

  return priv->orientation;
}

void
bobgui_list_base_get_border_spacing (BobguiListBase *self,
                                  int         *xspacing,
                                  int         *yspacing)
{
  BobguiCssStyle *style = bobgui_css_node_get_style (bobgui_widget_get_css_node (BOBGUI_WIDGET (self)));
  BobguiCssValue *border_spacing = style->size->border_spacing;

  if (bobgui_list_base_get_orientation (self) == BOBGUI_ORIENTATION_HORIZONTAL)
    {
      if (xspacing)
        *xspacing = _bobgui_css_position_value_get_y (border_spacing, 0);
      if (yspacing)
        *yspacing = _bobgui_css_position_value_get_x (border_spacing, 0);
    }
  else
    {
      if (xspacing)
        *xspacing = _bobgui_css_position_value_get_x (border_spacing, 0);
      if (yspacing)
        *yspacing = _bobgui_css_position_value_get_y (border_spacing, 0);
    }
}

BobguiListItemManager *
bobgui_list_base_get_manager (BobguiListBase *self)
{
  BobguiListBasePrivate *priv = bobgui_list_base_get_instance_private (self);

  return priv->item_manager;
}

guint
bobgui_list_base_get_anchor (BobguiListBase *self)
{
  BobguiListBasePrivate *priv = bobgui_list_base_get_instance_private (self);

  return bobgui_list_item_tracker_get_position (priv->item_manager,
                                             priv->anchor);
}

/*
 * bobgui_list_base_set_anchor:
 * @self: a `BobguiListBase`
 * @anchor_pos: position of the item to anchor
 * @anchor_align_across: how far in the across direction to anchor
 * @anchor_side_across: if the anchor should side to start or end of item
 * @anchor_align_along: how far in the along direction to anchor
 * @anchor_side_along: if the anchor should side to start or end of item
 *
 * Sets the anchor.
 * The anchor is the item that is always kept on screen.
 *
 * In each dimension, anchoring uses 2 variables: The side of the
 * item that gets anchored - either start or end - and where in
 * the widget's allocation it should get anchored - here 0.0 means
 * the start of the widget and 1.0 is the end of the widget.
 * It is allowed to use values outside of this range. In particular,
 * this is necessary when the items are larger than the list's
 * allocation.
 *
 * Using this information, the adjustment's value and in turn widget
 * offsets will then be computed. If the anchor is too far off, it
 * will be clamped so that there are always visible items on screen.
 *
 * Making anchoring this complicated ensures that one item - one
 * corner of one item to be exact - always stays at the same place
 * (usually this item is the focused item). So when the list undergoes
 * heavy changes (like sorting, filtering, removals, additions), this
 * item will stay in place while everything around it will shuffle
 * around.
 *
 * The anchor will also ensure that enough widgets are created according
 * to bobgui_list_base_set_anchor_max_widgets().
 **/
void
bobgui_list_base_set_anchor (BobguiListBase *self,
                          guint        anchor_pos,
                          double       anchor_align_across,
                          BobguiPackType  anchor_side_across,
                          double       anchor_align_along,
                          BobguiPackType  anchor_side_along)
{
  BobguiListBasePrivate *priv = bobgui_list_base_get_instance_private (self);
  guint items_before;

  items_before = round (priv->center_widgets * CLAMP (anchor_align_along, 0, 1));
  bobgui_list_item_tracker_set_position (priv->item_manager,
                                      priv->anchor,
                                      anchor_pos,
                                      items_before + priv->above_below_widgets,
                                      priv->center_widgets - items_before + priv->above_below_widgets);

  priv->anchor_align_across = anchor_align_across;
  priv->anchor_side_across = anchor_side_across;
  priv->anchor_align_along = anchor_align_along;
  priv->anchor_side_along = anchor_side_along;

  bobgui_widget_queue_allocate (BOBGUI_WIDGET (self));
}

/**
 * bobgui_list_base_set_anchor_max_widgets:
 * @self: a `BobguiListBase`
 * @center: the number of widgets in the middle
 * @above_below: extra widgets above and below
 *
 * Sets how many widgets should be kept alive around the anchor.
 * The number of these widgets determines how many items can be
 * displayed and must be chosen to be large enough to cover the
 * allocation but should be kept as small as possible for
 * performance reasons.
 *
 * There will be @center widgets allocated around the anchor
 * evenly distributed according to the anchor's alignment - if
 * the anchor is at the start, all these widgets will be allocated
 * behind it, if it's at the end, all the widgets will be allocated
 * in front of it.
 *
 * Additionally, there will be @above_below widgets allocated both
 * before and after the center widgets, so the total number of
 * widgets kept alive is 2 * above_below + center + 1.
 **/
void
bobgui_list_base_set_anchor_max_widgets (BobguiListBase *self,
                                      guint        n_center,
                                      guint        n_above_below)
{
  BobguiListBasePrivate *priv = bobgui_list_base_get_instance_private (self);

  priv->center_widgets = n_center;
  priv->above_below_widgets = n_above_below;

  bobgui_list_base_set_anchor (self,
                            bobgui_list_item_tracker_get_position (priv->item_manager, priv->anchor),
                            priv->anchor_align_across,
                            priv->anchor_side_across,
                            priv->anchor_align_along,
                            priv->anchor_side_along);
}

BobguiSelectionModel *
bobgui_list_base_get_model (BobguiListBase *self)
{
  BobguiListBasePrivate *priv = bobgui_list_base_get_instance_private (self);

  return priv->model;
}

gboolean
bobgui_list_base_set_model (BobguiListBase       *self,
                         BobguiSelectionModel *model)
{
  BobguiListBasePrivate *priv = bobgui_list_base_get_instance_private (self);

  if (priv->model == model)
    return FALSE;

  g_clear_object (&priv->model);

  if (model)
    {
      priv->model = g_object_ref (model);
      bobgui_list_item_manager_set_model (priv->item_manager, model);
      bobgui_list_base_set_anchor (self, 0, 0.0, BOBGUI_PACK_START, 0.0, BOBGUI_PACK_START);
    }
  else
    {
      bobgui_list_item_manager_set_model (priv->item_manager, NULL);
    }

  return TRUE;
}

void
bobgui_list_base_set_tab_behavior (BobguiListBase        *self,
                                BobguiListTabBehavior  behavior)
{
  BobguiListBasePrivate *priv = bobgui_list_base_get_instance_private (self);

  priv->tab_behavior = behavior;
}

BobguiListTabBehavior
bobgui_list_base_get_tab_behavior (BobguiListBase *self)
{
  BobguiListBasePrivate *priv = bobgui_list_base_get_instance_private (self);

  return priv->tab_behavior;
}

void
bobgui_list_base_scroll_to (BobguiListBase        *self,
                         guint               pos,
                         BobguiListScrollFlags  flags,
                         BobguiScrollInfo      *scroll)
{
  BobguiListBasePrivate *priv = bobgui_list_base_get_instance_private (self);

  if (flags & BOBGUI_LIST_SCROLL_FOCUS)
    {
      BobguiListItemTracker *old_focus;

      /* We need a tracker here to keep the focus widget around,
       * because we need to update the focus tracker before grabbing
       * focus, because otherwise bobgui_list_base_set_focus_child() will
       * scroll to the item, and we want to avoid that.
       */
      old_focus = bobgui_list_item_tracker_new (priv->item_manager);
      bobgui_list_item_tracker_set_position (priv->item_manager, old_focus, bobgui_list_base_get_focus_position (self), 0, 0);

      bobgui_list_item_tracker_set_position (priv->item_manager, priv->focus, pos, 0, 0);

      /* XXX: Is this the proper check? */
      if (bobgui_widget_get_state_flags (BOBGUI_WIDGET (self)) & BOBGUI_STATE_FLAG_FOCUS_WITHIN)
        {
          BobguiListTile *tile = bobgui_list_item_manager_get_nth (priv->item_manager, pos, NULL);

          bobgui_widget_grab_focus (tile->widget);
        }

      bobgui_list_item_tracker_free (priv->item_manager, old_focus);
    }

  if (flags & BOBGUI_LIST_SCROLL_SELECT)
    bobgui_list_base_select_item (self, pos, FALSE, FALSE);

  bobgui_list_base_scroll_to_item (self, pos, scroll);
}
