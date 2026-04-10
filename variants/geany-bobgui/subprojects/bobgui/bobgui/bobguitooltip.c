/* bobguitooltip.c
 *
 * Copyright (C) 2006-2007 Imendio AB
 * Contact: Kristian Rietveld <kris@imendio.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include "bobguitooltip.h"
#include "bobguitooltipprivate.h"

#include <math.h>
#include <string.h>

#include "bobguiwindow.h"
#include "bobguimain.h"
#include "bobguisettings.h"
#include "bobguisizerequest.h"
#include "bobguitooltipwindowprivate.h"
#include "bobguiwindowprivate.h"
#include "bobguiwidgetprivate.h"
#include "bobguinative.h"
#include "bobguiprivate.h"

/**
 * BobguiTooltip:
 *
 * Represents a widget tooltip.
 *
 * Basic tooltips can be realized simply by using
 * [method@Bobgui.Widget.set_tooltip_text] or
 * [method@Bobgui.Widget.set_tooltip_markup] without
 * any explicit tooltip object.
 *
 * When you need a tooltip with a little more fancy contents,
 * like adding an image, or you want the tooltip to have different
 * contents per `BobguiTreeView` row or cell, you will have to do a
 * little more work:
 *
 * - Set the [property@Bobgui.Widget:has-tooltip] property to %TRUE.
 *   This will make BOBGUI monitor the widget for motion and related events
 *   which are needed to determine when and where to show a tooltip.
 *
 * - Connect to the [signal@Bobgui.Widget::query-tooltip] signal.
 *   This signal will be emitted when a tooltip is supposed to be shown.
 *   One of the arguments passed to the signal handler is a `BobguiTooltip`
 *   object. This is the object that we are about to display as a tooltip,
 *   and can be manipulated in your callback using functions like
 *   [method@Bobgui.Tooltip.set_icon]. There are functions for setting
 *   the tooltip’s markup, setting an image from a named icon, or even
 *   putting in a custom widget.
 *
 * - Return %TRUE from your ::query-tooltip handler. This causes the tooltip
 *   to be show. If you return %FALSE, it will not be shown.
 */


#define HOVER_TIMEOUT          500
#define BROWSE_TIMEOUT         60
#define BROWSE_DISABLE_TIMEOUT 500

#define BOBGUI_TOOLTIP_CLASS(klass)         (G_TYPE_CHECK_CLASS_CAST ((klass), BOBGUI_TYPE_TOOLTIP, BobguiTooltipClass))
#define BOBGUI_IS_TOOLTIP_CLASS(klass)      (G_TYPE_CHECK_CLASS_TYPE ((klass), BOBGUI_TYPE_TOOLTIP))
#define BOBGUI_TOOLTIP_GET_CLASS(obj)       (G_TYPE_INSTANCE_GET_CLASS ((obj), BOBGUI_TYPE_TOOLTIP, BobguiTooltipClass))

/* We keep a single BobguiTooltip object per display. The tooltip object
 * owns a BobguiTooltipWindow widget, which is using a popup surface, similar
 * to what a BobguiPopover does. It gets reparented to the right native widget
 * whenever a tooltip is to be shown. The tooltip object keeps a weak
 * ref on the native in order to remove the tooltip window when the
 * native goes away.
 */
typedef struct _BobguiTooltipClass   BobguiTooltipClass;

struct _BobguiTooltip
{
  GObject parent_instance;

  BobguiWidget *window;

  BobguiWidget *tooltip_widget;

  BobguiWidget *native;

  guint timeout_id;
  guint browse_mode_timeout_id;

  GdkRectangle tip_area;

  guint browse_mode_enabled : 1;
  guint tip_area_set : 1;
  guint custom_was_reset : 1;
};

struct _BobguiTooltipClass
{
  GObjectClass parent_class;
};

#define BOBGUI_TOOLTIP_VISIBLE(tooltip) ((tooltip)->window && bobgui_widget_get_visible (BOBGUI_WIDGET((tooltip)->window)))

static void       bobgui_tooltip_dispose              (GObject         *object);

static void       bobgui_tooltip_window_hide          (BobguiWidget       *widget,
						    gpointer         user_data);
static void       bobgui_tooltip_display_closed       (GdkDisplay      *display,
						    gboolean         was_error,
						    BobguiTooltip      *tooltip);
static void       bobgui_tooltip_set_surface          (BobguiTooltip      *tooltip,
						    GdkSurface       *surface);

static void       bobgui_tooltip_handle_event_internal (GdkEventType   event_type,
                                                     GdkSurface    *surface,
                                                     BobguiWidget     *target_widget,
                                                     double        dx,
                                                     double        dy);

static GQuark quark_current_tooltip;

G_DEFINE_TYPE (BobguiTooltip, bobgui_tooltip, G_TYPE_OBJECT);

static void
bobgui_tooltip_class_init (BobguiTooltipClass *klass)
{
  GObjectClass *object_class;

  quark_current_tooltip = g_quark_from_static_string ("gdk-display-current-tooltip");

  object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = bobgui_tooltip_dispose;
}

static void
bobgui_tooltip_init (BobguiTooltip *tooltip)
{
  tooltip->timeout_id = 0;
  tooltip->browse_mode_timeout_id = 0;

  tooltip->browse_mode_enabled = FALSE;

  tooltip->tooltip_widget = NULL;

  tooltip->native = NULL;

  tooltip->window = bobgui_tooltip_window_new ();
  g_object_ref_sink (tooltip->window);
  g_signal_connect (tooltip->window, "hide",
                    G_CALLBACK (bobgui_tooltip_window_hide),
                    tooltip);
}

static void
bobgui_tooltip_dispose (GObject *object)
{
  BobguiTooltip *tooltip = BOBGUI_TOOLTIP (object);

  if (tooltip->timeout_id)
    {
      g_source_remove (tooltip->timeout_id);
      tooltip->timeout_id = 0;
    }

  if (tooltip->browse_mode_timeout_id)
    {
      g_source_remove (tooltip->browse_mode_timeout_id);
      tooltip->browse_mode_timeout_id = 0;
    }

  bobgui_tooltip_set_custom (tooltip, NULL);
  bobgui_tooltip_set_surface (tooltip, NULL);

  if (tooltip->window)
    {
      GdkDisplay *display;

      display = bobgui_widget_get_display (tooltip->window);
      g_signal_handlers_disconnect_by_func (display,
					    bobgui_tooltip_display_closed,
					    tooltip);
      bobgui_tooltip_window_set_relative_to (BOBGUI_TOOLTIP_WINDOW (tooltip->window), NULL);
      g_clear_object (&tooltip->window);
    }

  G_OBJECT_CLASS (bobgui_tooltip_parent_class)->dispose (object);
}

/* public API */

/**
 * bobgui_tooltip_set_markup:
 * @tooltip: a `BobguiTooltip`
 * @markup: (nullable): a string with Pango markup or %NLL
 *
 * Sets the text of the tooltip to be @markup.
 *
 * The string must be marked up with Pango markup.
 * If @markup is %NULL, the label will be hidden.
 */
void
bobgui_tooltip_set_markup (BobguiTooltip  *tooltip,
			const char *markup)
{
  g_return_if_fail (BOBGUI_IS_TOOLTIP (tooltip));

  bobgui_tooltip_window_set_label_markup (BOBGUI_TOOLTIP_WINDOW (tooltip->window), markup);
}

/**
 * bobgui_tooltip_set_text:
 * @tooltip: a `BobguiTooltip`
 * @text: (nullable): a text string
 *
 * Sets the text of the tooltip to be @text.
 *
 * If @text is %NULL, the label will be hidden.
 * See also [method@Bobgui.Tooltip.set_markup].
 */
void
bobgui_tooltip_set_text (BobguiTooltip  *tooltip,
                      const char *text)
{
  g_return_if_fail (BOBGUI_IS_TOOLTIP (tooltip));

  bobgui_tooltip_window_set_label_text (BOBGUI_TOOLTIP_WINDOW (tooltip->window), text);
}

/**
 * bobgui_tooltip_set_icon:
 * @tooltip: a `BobguiTooltip`
 * @paintable: (nullable): a `GdkPaintable`
 *
 * Sets the icon of the tooltip (which is in front of the text) to be
 * @paintable.  If @paintable is %NULL, the image will be hidden.
 */
void
bobgui_tooltip_set_icon (BobguiTooltip   *tooltip,
		      GdkPaintable *paintable)
{
  g_return_if_fail (BOBGUI_IS_TOOLTIP (tooltip));
  g_return_if_fail (paintable == NULL || GDK_IS_PAINTABLE (paintable));

  bobgui_tooltip_window_set_image_icon (BOBGUI_TOOLTIP_WINDOW (tooltip->window), paintable);
}

/**
 * bobgui_tooltip_set_icon_from_icon_name:
 * @tooltip: a `BobguiTooltip`
 * @icon_name: (nullable): an icon name
 *
 * Sets the icon of the tooltip (which is in front of the text) to be
 * the icon indicated by @icon_name with the size indicated
 * by @size.  If @icon_name is %NULL, the image will be hidden.
 */
void
bobgui_tooltip_set_icon_from_icon_name (BobguiTooltip  *tooltip,
				     const char *icon_name)
{
  g_return_if_fail (BOBGUI_IS_TOOLTIP (tooltip));

  bobgui_tooltip_window_set_image_icon_from_name (BOBGUI_TOOLTIP_WINDOW (tooltip->window),
                                               icon_name);
}

/**
 * bobgui_tooltip_set_icon_from_gicon:
 * @tooltip: a `BobguiTooltip`
 * @gicon: (nullable): a `GIcon` representing the icon
 *
 * Sets the icon of the tooltip (which is in front of the text)
 * to be the icon indicated by @gicon with the size indicated
 * by @size. If @gicon is %NULL, the image will be hidden.
 */
void
bobgui_tooltip_set_icon_from_gicon (BobguiTooltip  *tooltip,
				 GIcon       *gicon)
{
  g_return_if_fail (BOBGUI_IS_TOOLTIP (tooltip));

  bobgui_tooltip_window_set_image_icon_from_gicon (BOBGUI_TOOLTIP_WINDOW (tooltip->window),
                                                gicon);
}

/**
 * bobgui_tooltip_set_custom:
 * @tooltip: a `BobguiTooltip`
 * @custom_widget: (nullable): a `BobguiWidget`, or %NULL to unset the old custom widget.
 *
 * Replaces the widget packed into the tooltip with
 * @custom_widget. @custom_widget does not get destroyed when the tooltip goes
 * away.
 * By default a box with a `BobguiImage` and `BobguiLabel` is embedded in
 * the tooltip, which can be configured using bobgui_tooltip_set_markup()
 * and bobgui_tooltip_set_icon().
 */
void
bobgui_tooltip_set_custom (BobguiTooltip *tooltip,
			BobguiWidget  *custom_widget)
{
  g_return_if_fail (BOBGUI_IS_TOOLTIP (tooltip));
  g_return_if_fail (custom_widget == NULL || BOBGUI_IS_WIDGET (custom_widget));

  /* The custom widget has been updated from the query-tooltip
   * callback, so we do not want to reset the custom widget later on.
   */
  tooltip->custom_was_reset = TRUE;

  bobgui_tooltip_window_set_custom_widget (BOBGUI_TOOLTIP_WINDOW (tooltip->window), custom_widget);
}

/**
 * bobgui_tooltip_set_tip_area:
 * @tooltip: a `BobguiTooltip`
 * @rect: a `GdkRectangle`
 *
 * Sets the area of the widget, where the contents of this tooltip apply,
 * to be @rect (in widget coordinates).  This is especially useful for
 * properly setting tooltips on `BobguiTreeView` rows and cells, `BobguiIconViews`,
 * etc.
 *
 * For setting tooltips on `BobguiTreeView`, please refer to the convenience
 * functions for this: bobgui_tree_view_set_tooltip_row() and
 * bobgui_tree_view_set_tooltip_cell().
 */
void
bobgui_tooltip_set_tip_area (BobguiTooltip         *tooltip,
			  const GdkRectangle *rect)
{
  g_return_if_fail (BOBGUI_IS_TOOLTIP (tooltip));

  if (!rect)
    tooltip->tip_area_set = FALSE;
  else
    {
      tooltip->tip_area_set = TRUE;
      tooltip->tip_area = *rect;
    }
}

/*
 * bobgui_tooltip_trigger_tooltip_query:
 * @display: a `GdkDisplay`
 *
 * Triggers a new tooltip query on @display, in order to update the current
 * visible tooltip, or to show/hide the current tooltip.  This function is
 * useful to call when, for example, the state of the widget changed by a
 * key press.
 */
void
bobgui_tooltip_trigger_tooltip_query (BobguiWidget *widget)
{
  GdkDisplay *display;
  GdkSeat *seat;
  GdkDevice *device;
  GdkSurface *surface;
  double px, py;
  int x, y;
  BobguiWidget *toplevel;
  BobguiWidget *target_widget;

  g_return_if_fail (BOBGUI_IS_WIDGET (widget));

  display = bobgui_widget_get_display (widget);

  /* Trigger logic as if the mouse moved */
  seat = gdk_display_get_default_seat (display);
  if (!seat)
    return;

  device = gdk_seat_get_pointer (seat);
  if (!device)
    return;

  surface = gdk_device_get_surface_at_position (device, &px, &py);
  if (!surface)
    return;

  toplevel = BOBGUI_WIDGET (bobgui_widget_get_root (widget));
  if (toplevel == NULL)
    return;

  if (bobgui_native_get_surface (BOBGUI_NATIVE (toplevel)) != surface)
    return;

  x = round (px);
  y = round (py);

  target_widget = _bobgui_widget_find_at_coords (surface, x, y, &x, &y);

  bobgui_tooltip_handle_event_internal (GDK_MOTION_NOTIFY, surface, target_widget, x, y);
}

static void
bobgui_tooltip_window_hide (BobguiWidget *widget,
			 gpointer   user_data)
{
  BobguiTooltip *tooltip = BOBGUI_TOOLTIP (user_data);

  bobgui_tooltip_set_custom (tooltip, NULL);
}

BobguiWidget *
_bobgui_widget_find_at_coords (GdkSurface *surface,
                            int        surface_x,
                            int        surface_y,
                            int       *widget_x,
                            int       *widget_y)
{
  BobguiWidget *event_widget;
  BobguiWidget *picked_widget;
  double x, y;
  double native_x, native_y;

  g_return_val_if_fail (GDK_IS_SURFACE (surface), NULL);

  event_widget = BOBGUI_WIDGET (bobgui_native_get_for_surface (surface));

  if (!event_widget)
    return NULL;

  bobgui_native_get_surface_transform (BOBGUI_NATIVE (event_widget), &native_x, &native_y);
  x = surface_x - native_x;
  y = surface_y - native_y;

  picked_widget = bobgui_widget_pick (event_widget, x, y, BOBGUI_PICK_INSENSITIVE);

  if (picked_widget != NULL)
    {
      graphene_point_t p;

      if (!bobgui_widget_compute_point (event_widget, picked_widget,
                                     &GRAPHENE_POINT_INIT (x, y), &p))
        graphene_point_init (&p, x, y);
      x = p.x;
      y = p.y;
    }

  *widget_x = x;
  *widget_y = y;

  return picked_widget;
}

static int
tooltip_browse_mode_expired (gpointer data)
{
  BobguiTooltip *tooltip;
  GdkDisplay *display;

  tooltip = BOBGUI_TOOLTIP (data);

  tooltip->browse_mode_enabled = FALSE;
  tooltip->browse_mode_timeout_id = 0;

  if (tooltip->timeout_id)
    {
      g_source_remove (tooltip->timeout_id);
      tooltip->timeout_id = 0;
    }

  /* destroy tooltip */
  display = bobgui_widget_get_display (tooltip->window);
  g_object_set_qdata (G_OBJECT (display), quark_current_tooltip, NULL);

  return FALSE;
}

static void
bobgui_tooltip_display_closed (GdkDisplay *display,
			    gboolean    was_error,
			    BobguiTooltip *tooltip)
{
  if (tooltip->timeout_id)
    {
      g_source_remove (tooltip->timeout_id);
      tooltip->timeout_id = 0;
    }

  g_object_set_qdata (G_OBJECT (display), quark_current_tooltip, NULL);
}

static void
native_weak_notify (gpointer data, GObject *former_object)
{
  BobguiTooltip *tooltip = data;

  bobgui_tooltip_window_set_relative_to (BOBGUI_TOOLTIP_WINDOW (tooltip->window), NULL);
  tooltip->native = NULL;
}

static void
bobgui_tooltip_set_surface (BobguiTooltip *tooltip,
                         GdkSurface  *surface)
{
  BobguiWidget *native;

  if (surface)
    native = BOBGUI_WIDGET (bobgui_native_get_for_surface (surface));
  else
    native = NULL;

  if (tooltip->native == native)
    return;

  if (BOBGUI_IS_TOOLTIP_WINDOW (native))
    return;

  if (tooltip->native)
    g_object_weak_unref (G_OBJECT (tooltip->native), native_weak_notify, tooltip);

  tooltip->native = native;

  if (tooltip->native)
    g_object_weak_ref (G_OBJECT (tooltip->native), native_weak_notify, tooltip);

  if (native)
    bobgui_tooltip_window_set_relative_to (BOBGUI_TOOLTIP_WINDOW (tooltip->window), native);
  else
    bobgui_tooltip_window_set_relative_to (BOBGUI_TOOLTIP_WINDOW (tooltip->window), NULL);
}

static gboolean
bobgui_tooltip_run_requery (BobguiWidget  **widget,
			 BobguiTooltip  *tooltip,
			 int         *x,
			 int         *y)
{
  gboolean has_tooltip = FALSE;
  gboolean return_value = FALSE;

  /* Reset tooltip */
  bobgui_tooltip_set_markup (tooltip, NULL);
  bobgui_tooltip_set_icon (tooltip, NULL);
  bobgui_tooltip_set_tip_area (tooltip, NULL);

  /* See if the custom widget is again set from the query-tooltip
   * callback.
   */
  tooltip->custom_was_reset = FALSE;

  do
    {
      has_tooltip = bobgui_widget_get_has_tooltip (*widget);

      if (has_tooltip)
        return_value = bobgui_widget_query_tooltip (*widget, *x, *y, FALSE, tooltip);

      if (!return_value)
        {
          BobguiWidget *parent = bobgui_widget_get_parent (*widget);

          if (BOBGUI_IS_NATIVE (*widget))
            break;

          if (parent)
            {
              graphene_point_t r = GRAPHENE_POINT_INIT (*x, *y);
              graphene_point_t p;

              if (!bobgui_widget_compute_point (*widget, parent, &r, &p))
                break;

              *x = p.x;
              *y = p.y;
            }

          *widget = parent;
        }
      else
        break;
    }
  while (*widget);

  /* If the custom widget was not reset in the query-tooltip
   * callback, we clear it here.
   */
  if (!tooltip->custom_was_reset)
    bobgui_tooltip_set_custom (tooltip, NULL);

  return return_value;
}

static void
bobgui_tooltip_position (BobguiTooltip *tooltip,
		      GdkDisplay *display,
		      BobguiWidget  *new_tooltip_widget,
                      GdkDevice  *device)
{
  BobguiSettings *settings;
  graphene_rect_t anchor_bounds;
  GdkRectangle anchor_rect;
  GdkSurface *effective_toplevel;
  BobguiWidget *toplevel;
  int rect_anchor_dx = 0;
  int cursor_size;
  int anchor_rect_padding;
  double native_x, native_y;

  bobgui_widget_realize (BOBGUI_WIDGET (tooltip->window));

  tooltip->tooltip_widget = new_tooltip_widget;

  toplevel = BOBGUI_WIDGET (bobgui_widget_get_native (new_tooltip_widget));
  bobgui_native_get_surface_transform (BOBGUI_NATIVE (toplevel), &native_x, &native_y);

  if (bobgui_widget_compute_bounds (new_tooltip_widget, toplevel, &anchor_bounds))
    {
      anchor_rect = (GdkRectangle) {
        floorf (anchor_bounds.origin.x + native_x),
        floorf (anchor_bounds.origin.y + native_y),
        ceilf (anchor_bounds.size.width),
        ceilf (anchor_bounds.size.height)
      };
    }
  else
    {
      anchor_rect = (GdkRectangle) { 0, 0, 0, 0 };
    }

  settings = bobgui_settings_get_for_display (display);
  g_object_get (settings,
                "bobgui-cursor-theme-size", &cursor_size,
                NULL);

  if (cursor_size == 0)
    cursor_size = 16;

  if (device)
    anchor_rect_padding = MAX (4, cursor_size - 32);
  else
    anchor_rect_padding = 4;

  anchor_rect.x -= anchor_rect_padding;
  anchor_rect.y -= anchor_rect_padding;
  anchor_rect.width += anchor_rect_padding * 2;
  anchor_rect.height += anchor_rect_padding * 2;

  if (device)
    {
      const int max_x_distance = 32;
      /* Max 48x48 icon + default padding */
      const int max_anchor_rect_height = 48 + 8;
      double px, py;
      int pointer_x, pointer_y;

      /*
       * For pointer position triggered tooltips, implement the following
       * semantics:
       *
       * If the anchor rectangle is too tall (meaning if we'd be constrained
       * and flip, it'd flip too far away), rely only on the pointer position
       * to position the tooltip. The approximate pointer cursor rectangle is
       * used as an anchor rectangle.
       *
       * If the anchor rectangle isn't too tall, make sure the tooltip isn't too
       * far away from the pointer position.
       */
      effective_toplevel = bobgui_native_get_surface (BOBGUI_NATIVE (toplevel));
      gdk_surface_get_device_position (effective_toplevel, device, &px, &py, NULL);
      pointer_x = round (px);
      pointer_y = round (py);

      if (anchor_rect.height > max_anchor_rect_height)
        {
          anchor_rect.x = pointer_x - 4;
          anchor_rect.y = pointer_y - 4;
          anchor_rect.width = cursor_size;
          anchor_rect.height = cursor_size;
        }
      else
        {
          int anchor_point_x;
          int x_distance;

          anchor_point_x = anchor_rect.x + anchor_rect.width / 2;
          x_distance = pointer_x - anchor_point_x;

          if (x_distance > max_x_distance)
            rect_anchor_dx = x_distance - max_x_distance;
          else if (x_distance < -max_x_distance)
            rect_anchor_dx = x_distance + max_x_distance;
        }
    }

  bobgui_tooltip_window_position (BOBGUI_TOOLTIP_WINDOW (tooltip->window),
                               &anchor_rect,
                               GDK_GRAVITY_SOUTH,
                               GDK_GRAVITY_NORTH,
                               GDK_ANCHOR_FLIP_Y | GDK_ANCHOR_SLIDE_X,
                               rect_anchor_dx, 0);
}

static void
bobgui_tooltip_show_tooltip (GdkDisplay *display)
{
  double px, py;
  int x, y;
  GdkSurface *surface;
  BobguiWidget *tooltip_widget;
  GdkSeat *seat;
  GdkDevice *device;
  BobguiTooltip *tooltip;
  gboolean return_value = FALSE;

  tooltip = g_object_get_qdata (G_OBJECT (display), quark_current_tooltip);

  if (!tooltip->native)
    return;

  surface = bobgui_native_get_surface (BOBGUI_NATIVE (tooltip->native));

  seat = gdk_display_get_default_seat (display);
  if (seat)
    device = gdk_seat_get_pointer (seat);
  else
    device = NULL;

  if (device)
    gdk_surface_get_device_position (surface, device, &px, &py, NULL);
  else
    px = py = 0;

  x = round (px);
  y = round (py);

  tooltip_widget = _bobgui_widget_find_at_coords (surface, x, y, &x, &y);

  if (!tooltip_widget)
    return;

  return_value = bobgui_tooltip_run_requery (&tooltip_widget, tooltip, &x, &y);
  if (!return_value)
    return;

  /* FIXME: should use tooltip->window iso tooltip->window */
  if (display != bobgui_widget_get_display (tooltip->window))
    {
      g_signal_handlers_disconnect_by_func (display,
                                            bobgui_tooltip_display_closed,
                                            tooltip);

      bobgui_window_set_display (BOBGUI_WINDOW (tooltip->window), display);

      g_signal_connect (display, "closed",
                        G_CALLBACK (bobgui_tooltip_display_closed), tooltip);
    }

  bobgui_tooltip_position (tooltip, display, tooltip_widget, device);

  bobgui_widget_set_visible (BOBGUI_WIDGET (tooltip->window), TRUE);

  /* Now a tooltip is visible again on the display, make sure browse
   * mode is enabled.
   */
  tooltip->browse_mode_enabled = TRUE;
  if (tooltip->browse_mode_timeout_id)
    {
      g_source_remove (tooltip->browse_mode_timeout_id);
      tooltip->browse_mode_timeout_id = 0;
    }
}

static void
bobgui_tooltip_hide_tooltip (BobguiTooltip *tooltip)
{
  guint timeout = BROWSE_DISABLE_TIMEOUT;

  if (!tooltip)
    return;

  if (tooltip->timeout_id)
    {
      g_source_remove (tooltip->timeout_id);
      tooltip->timeout_id = 0;
    }

  if (!BOBGUI_TOOLTIP_VISIBLE (tooltip))
    return;

  tooltip->tooltip_widget = NULL;

  /* The tooltip is gone, after (by default, should be configurable) 500ms
   * we want to turn off browse mode
   */
  if (!tooltip->browse_mode_timeout_id)
    {
      tooltip->browse_mode_timeout_id =
        g_timeout_add_full (0, timeout,
                            tooltip_browse_mode_expired,
                            g_object_ref (tooltip),
                            g_object_unref);
      gdk_source_set_static_name_by_id (tooltip->browse_mode_timeout_id, "[bobgui] tooltip_browse_mode_expired");
    }

  if (tooltip->window)
    bobgui_widget_set_visible (tooltip->window, FALSE);
}

static int
tooltip_popup_timeout (gpointer data)
{
  GdkDisplay *display;
  BobguiTooltip *tooltip;

  display = GDK_DISPLAY (data);
  tooltip = g_object_get_qdata (G_OBJECT (display), quark_current_tooltip);

  /* This usually does not happen.  However, it does occur in language
   * bindings were reference counting of objects behaves differently.
   */
  if (!tooltip)
    return FALSE;

  bobgui_tooltip_show_tooltip (display);

  tooltip->timeout_id = 0;

  return FALSE;
}

static void
bobgui_tooltip_start_delay (GdkDisplay *display)
{
  guint timeout;
  BobguiTooltip *tooltip;

  tooltip = g_object_get_qdata (G_OBJECT (display), quark_current_tooltip);

  if (!tooltip || BOBGUI_TOOLTIP_VISIBLE (tooltip))
    return;

  if (tooltip->timeout_id)
    g_source_remove (tooltip->timeout_id);

  if (tooltip->browse_mode_enabled)
    timeout = BROWSE_TIMEOUT;
  else
    timeout = HOVER_TIMEOUT;

  tooltip->timeout_id = g_timeout_add_full (0, timeout,
                                            tooltip_popup_timeout,
                                            g_object_ref (display),
                                            g_object_unref);
  gdk_source_set_static_name_by_id (tooltip->timeout_id, "[bobgui] tooltip_popup_timeout");
}

void
_bobgui_tooltip_hide (BobguiWidget *widget)
{
  GdkDisplay *display;
  BobguiTooltip *tooltip;

  display = bobgui_widget_get_display (widget);
  tooltip = g_object_get_qdata (G_OBJECT (display), quark_current_tooltip);

  if (!tooltip || !tooltip->window || !tooltip->tooltip_widget)
    return;

  if (widget == tooltip->tooltip_widget)
    bobgui_tooltip_hide_tooltip (tooltip);
}

static gboolean
tooltips_enabled (GdkEvent *event)
{
  GdkDevice *source_device;
  GdkInputSource source;
  GdkModifierType event_state = 0;

  switch ((guint)gdk_event_get_event_type (event))
    {
    case GDK_ENTER_NOTIFY:
    case GDK_LEAVE_NOTIFY:
    case GDK_BUTTON_PRESS:
    case GDK_KEY_PRESS:
    case GDK_DRAG_ENTER:
    case GDK_GRAB_BROKEN:
    case GDK_MOTION_NOTIFY:
    case GDK_TOUCH_UPDATE:
    case GDK_SCROLL:
      break; /* OK */

    default:
      return FALSE;
    }

  event_state = gdk_event_get_modifier_state (event);
  if ((event_state &
       (GDK_BUTTON1_MASK |
        GDK_BUTTON2_MASK |
        GDK_BUTTON3_MASK |
        GDK_BUTTON4_MASK |
        GDK_BUTTON5_MASK)) != 0)
    return FALSE;

  source_device = gdk_event_get_device (event);

  if (!source_device)
    return FALSE;

  source = gdk_device_get_source (source_device);

  if (source != GDK_SOURCE_TOUCHSCREEN)
    return TRUE;

  return FALSE;
}

void
_bobgui_tooltip_handle_event (BobguiWidget *target,
                           GdkEvent  *event)
{
  GdkEventType event_type;
  GdkSurface *surface;
  double x, y;
  double nx, ny;
  BobguiNative *native;

  if (!tooltips_enabled (event))
    return;

  native = bobgui_widget_get_native (target);
  if (!native)
    return;

  event_type = gdk_event_get_event_type (event);

  /* ignore synthetic motion events */
  if (event_type == GDK_MOTION_NOTIFY &&
      gdk_event_get_time (event) == GDK_CURRENT_TIME)
    return;

  surface = gdk_event_get_surface (event);
  if (gdk_event_get_position (event, &x, &y))
    {
      graphene_point_t p;
      bobgui_native_get_surface_transform (native, &nx, &ny);
      if (!bobgui_widget_compute_point (BOBGUI_WIDGET (native), target,
                                     &GRAPHENE_POINT_INIT (x - nx, y - ny), &p))
        graphene_point_init (&p, x - nx, y - ny);
      x = p.x;
      y = p.y;
    }
  bobgui_tooltip_handle_event_internal (event_type, surface, target, x, y);
}

/* dx/dy must be in @target_widget's coordinates */
static void
bobgui_tooltip_handle_event_internal (GdkEventType   event_type,
                                   GdkSurface    *surface,
                                   BobguiWidget     *target_widget,
                                   double         dx,
                                   double         dy)
{
  int x = dx, y = dy;
  GdkDisplay *display;
  BobguiTooltip *tooltip;

  display = gdk_surface_get_display (surface);
  tooltip = g_object_get_qdata (G_OBJECT (display), quark_current_tooltip);

  if (tooltip)
    bobgui_tooltip_set_surface (tooltip, surface);

  /* Hide the tooltip when there's no new tooltip widget */
  if (!target_widget)
    {
      if (tooltip)
	bobgui_tooltip_hide_tooltip (tooltip);

      return;
    }

  switch ((guint) event_type)
    {
      case GDK_BUTTON_PRESS:
      case GDK_KEY_PRESS:
      case GDK_DRAG_ENTER:
      case GDK_GRAB_BROKEN:
      case GDK_SCROLL:
	bobgui_tooltip_hide_tooltip (tooltip);
	break;

      case GDK_MOTION_NOTIFY:
      case GDK_ENTER_NOTIFY:
      case GDK_LEAVE_NOTIFY:
	if (tooltip)
	  {
	    gboolean tip_area_set;
            GdkRectangle tip_area;
	    gboolean hide_tooltip;

	    tip_area_set = tooltip->tip_area_set;
	    tip_area = tooltip->tip_area;

	    bobgui_tooltip_run_requery (&target_widget, tooltip, &x, &y);

	    /* Leave notify should override the query function */
	    hide_tooltip = (event_type == GDK_LEAVE_NOTIFY);

	    /* Is the pointer above another widget now? */
	    if (BOBGUI_TOOLTIP_VISIBLE (tooltip))
	      hide_tooltip |= target_widget != tooltip->tooltip_widget;

	    /* Did the pointer move out of the previous "context area"? */
	    if (tip_area_set)
	      hide_tooltip |= !gdk_rectangle_contains_point (&tip_area, x, y);

	    if (hide_tooltip)
	      bobgui_tooltip_hide_tooltip (tooltip);
	    else
	      bobgui_tooltip_start_delay (display);
	  }
	else
	  {
	    /* Need a new tooltip for this display */
	    tooltip = g_object_new (BOBGUI_TYPE_TOOLTIP, NULL);
	    g_object_set_qdata_full (G_OBJECT (display), quark_current_tooltip,
				     tooltip, g_object_unref);
	    g_signal_connect (display, "closed",
			      G_CALLBACK (bobgui_tooltip_display_closed), tooltip);

	    bobgui_tooltip_set_surface (tooltip, surface);

	    bobgui_tooltip_start_delay (display);
	  }
	break;

      default:
	break;
    }
}

void
bobgui_tooltip_maybe_allocate (BobguiNative *native)
{
  GdkDisplay *display = bobgui_widget_get_display (BOBGUI_WIDGET (native));
  BobguiTooltip *tooltip;

  tooltip = g_object_get_qdata (G_OBJECT (display), quark_current_tooltip);
  if (!tooltip || BOBGUI_NATIVE (tooltip->native) != native)
    return;

  bobgui_tooltip_window_present (BOBGUI_TOOLTIP_WINDOW (tooltip->window));
}

void
bobgui_tooltip_unset_surface (BobguiNative *native)
{
  GdkDisplay *display = bobgui_widget_get_display (BOBGUI_WIDGET (native));
  BobguiTooltip *tooltip;

  tooltip = g_object_get_qdata (G_OBJECT (display), quark_current_tooltip);
  if (!tooltip || BOBGUI_NATIVE (tooltip->native) != native)
    return;

  bobgui_tooltip_set_surface (tooltip, NULL);
}
