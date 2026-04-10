/* BOBGUI - The Bobgui Framework
 * Copyright © 2012 Carlos Garnacho <carlosg@gnome.org>
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

#include "bobguitexthandleprivate.h"

#include "bobguibinlayout.h"
#include "bobguicssboxesimplprivate.h"
#include "bobguicssnodeprivate.h"
#include "bobguigesturedrag.h"
#include "bobguigizmoprivate.h"
#include "bobguimarshalers.h"
#include "gdk/gdkmarshalers.h"
#include "bobguinativeprivate.h"
#include "bobguiprivatetypebuiltins.h"
#include "bobguirendericonprivate.h"
#include "bobguiwidgetprivate.h"
#include "bobguiwindowprivate.h"
#include "bobguiprivate.h"

#include "gdk/gdksurfaceprivate.h"
#include "gsk/gskrendererprivate.h"

enum {
  DRAG_STARTED,
  HANDLE_DRAGGED,
  DRAG_FINISHED,
  LAST_SIGNAL
};

struct _BobguiTextHandle
{
  BobguiWidget parent_instance;

  GdkSurface *surface;
  GskRenderer *renderer;
  BobguiEventController *controller;
  BobguiWidget *controller_widget;
  BobguiWidget *contents;

  GdkRectangle pointing_to;
  BobguiBorder border;
  int dx;
  int dy;
  guint surface_transform_changed_cb;

  guint role : 2;
  guint dragged : 1;
  guint mode_visible : 1;
  guint user_visible : 1;
  guint has_point : 1;
};

static void bobgui_text_handle_native_interface_init (BobguiNativeInterface *iface);

static void handle_drag_begin (BobguiGestureDrag *gesture,
                               double          x,
                               double          y,
                               BobguiTextHandle  *handle);
static void handle_drag_update (BobguiGestureDrag *gesture,
                                double          offset_x,
                                double          offset_y,
                                BobguiWidget      *widget);
static void handle_drag_end (BobguiGestureDrag *gesture,
                             double          offset_x,
                             double          offset_y,
                             BobguiTextHandle  *handle);

G_DEFINE_TYPE_WITH_CODE (BobguiTextHandle, bobgui_text_handle, BOBGUI_TYPE_WIDGET,
                         G_IMPLEMENT_INTERFACE (BOBGUI_TYPE_NATIVE,
                                                bobgui_text_handle_native_interface_init))

static guint signals[LAST_SIGNAL] = { 0 };

static GdkSurface *
bobgui_text_handle_native_get_surface (BobguiNative *native)
{
  return BOBGUI_TEXT_HANDLE (native)->surface;
}

static GskRenderer *
bobgui_text_handle_native_get_renderer (BobguiNative *native)
{
  return BOBGUI_TEXT_HANDLE (native)->renderer;
}

static void
bobgui_text_handle_native_get_surface_transform (BobguiNative *native,
                                              double    *x,
                                              double    *y)
{
  BobguiCssBoxes css_boxes;
  const graphene_rect_t *margin_rect;

  bobgui_css_boxes_init (&css_boxes, BOBGUI_WIDGET (native));
  margin_rect = bobgui_css_boxes_get_margin_rect (&css_boxes);

  *x = - margin_rect->origin.x;
  *y = - margin_rect->origin.y;
}

static void
bobgui_text_handle_get_padding (BobguiTextHandle *handle,
                             BobguiBorder     *padding)
{
  BobguiWidget *widget = BOBGUI_WIDGET (handle);
  BobguiCssStyle *style = bobgui_css_node_get_style (bobgui_widget_get_css_node (widget));

  padding->left = bobgui_css_number_value_get (style->size->padding_left, 100);
  padding->right = bobgui_css_number_value_get (style->size->padding_right, 100);
  padding->top = bobgui_css_number_value_get (style->size->padding_top, 100);
  padding->bottom = bobgui_css_number_value_get (style->size->padding_bottom, 100);
}

static void
bobgui_text_handle_present_surface (BobguiTextHandle *handle)
{
  BobguiWidget *widget = BOBGUI_WIDGET (handle);
  GdkPopupLayout *layout;
  GdkRectangle rect;
  BobguiRequisition req;
  BobguiWidget *parent;
  BobguiNative *native;
  graphene_point_t point = GRAPHENE_POINT_INIT (handle->pointing_to.x, handle->pointing_to.y);
  graphene_point_t transformed;
  double nx, ny;

  bobgui_widget_get_preferred_size (widget, NULL, &req);
  bobgui_text_handle_get_padding (handle, &handle->border);

  parent = bobgui_widget_get_parent (widget);

  native = bobgui_widget_get_native (parent);
  bobgui_native_get_surface_transform (native, &nx, &ny);

  if (!bobgui_widget_compute_point (parent, BOBGUI_WIDGET (native),
                                 &point, &transformed))
    transformed = point;

  rect.x = (int)(transformed.x + nx);
  rect.y = (int)(transformed.y + ny) + handle->pointing_to.height - handle->border.top;

  rect.width = req.width - handle->border.left - handle->border.right;
  rect.height = 1;

  if (handle->role == BOBGUI_TEXT_HANDLE_ROLE_CURSOR)
    rect.x -= rect.width / 2;
  else if ((handle->role == BOBGUI_TEXT_HANDLE_ROLE_SELECTION_END &&
            bobgui_widget_get_direction (widget) == BOBGUI_TEXT_DIR_RTL) ||
           (handle->role == BOBGUI_TEXT_HANDLE_ROLE_SELECTION_START &&
            bobgui_widget_get_direction (widget) != BOBGUI_TEXT_DIR_RTL))
    rect.x -= rect.width;

  layout = gdk_popup_layout_new (&rect,
                                 GDK_GRAVITY_SOUTH,
                                 GDK_GRAVITY_NORTH);

  gdk_popup_present (GDK_POPUP (handle->surface),
                     MAX (req.width, 1),
                     MAX (req.height, 1),
                     layout);
  gdk_popup_layout_unref (layout);
}

void
bobgui_text_handle_present (BobguiTextHandle *handle)
{
  BobguiWidget *widget = BOBGUI_WIDGET (handle);

  if (!_bobgui_widget_get_alloc_needed (widget))
    bobgui_widget_ensure_allocate (widget);
  else if (bobgui_widget_get_visible (widget))
    bobgui_text_handle_present_surface (handle);
}

static void
bobgui_text_handle_native_layout (BobguiNative *native,
                               int        width,
                               int        height)
{
  BobguiWidget *widget = BOBGUI_WIDGET (native);

  if (_bobgui_widget_get_alloc_needed (widget))
    bobgui_widget_allocate (widget, width, height, -1, NULL);
  else
    bobgui_widget_ensure_allocate (widget);
}

static void
bobgui_text_handle_native_interface_init (BobguiNativeInterface *iface)
{
  iface->get_surface = bobgui_text_handle_native_get_surface;
  iface->get_renderer = bobgui_text_handle_native_get_renderer;
  iface->get_surface_transform = bobgui_text_handle_native_get_surface_transform;
  iface->layout = bobgui_text_handle_native_layout;
}

static gboolean
surface_render (GdkSurface     *surface,
                cairo_region_t *region,
                BobguiTextHandle  *handle)
{
  bobgui_widget_render (BOBGUI_WIDGET (handle), surface, region);
  return TRUE;
}

static void
surface_mapped_changed (BobguiWidget *widget)
{
  BobguiTextHandle *handle = BOBGUI_TEXT_HANDLE (widget);

  bobgui_widget_set_visible (widget, gdk_surface_get_mapped (handle->surface));
}

static void
bobgui_text_handle_snapshot (BobguiWidget   *widget,
                          BobguiSnapshot *snapshot)
{
  BobguiCssStyle *style = bobgui_css_node_get_style (bobgui_widget_get_css_node (widget));

  bobgui_css_style_snapshot_icon (style,
                               snapshot,
                               bobgui_widget_get_width (widget),
                               bobgui_widget_get_height (widget));

  BOBGUI_WIDGET_CLASS (bobgui_text_handle_parent_class)->snapshot (widget, snapshot);
}

static void
bobgui_text_handle_realize (BobguiWidget *widget)
{
  BobguiTextHandle *handle = BOBGUI_TEXT_HANDLE (widget);
  GdkSurface *parent_surface;
  BobguiWidget *parent;
  cairo_region_t *region;

  parent = bobgui_widget_get_parent (widget);
  parent_surface = bobgui_native_get_surface (bobgui_widget_get_native (parent));

  handle->surface = gdk_surface_new_popup (parent_surface, FALSE);
  gdk_surface_set_widget (handle->surface, widget);

  region = cairo_region_create ();
  gdk_surface_set_input_region (handle->surface, region);
  cairo_region_destroy (region);

  g_signal_connect_swapped (handle->surface, "notify::mapped",
                            G_CALLBACK (surface_mapped_changed), widget);
  g_signal_connect (handle->surface, "render", G_CALLBACK (surface_render), widget);

  BOBGUI_WIDGET_CLASS (bobgui_text_handle_parent_class)->realize (widget);

  handle->renderer = gsk_renderer_new_for_surface_full (handle->surface, TRUE);

  bobgui_native_realize (BOBGUI_NATIVE (handle));
}

static void
bobgui_text_handle_unrealize (BobguiWidget *widget)
{
  BobguiTextHandle *handle = BOBGUI_TEXT_HANDLE (widget);

  bobgui_native_unrealize (BOBGUI_NATIVE (handle));

  BOBGUI_WIDGET_CLASS (bobgui_text_handle_parent_class)->unrealize (widget);

  gsk_renderer_unrealize (handle->renderer);
  g_clear_object (&handle->renderer);

  g_signal_handlers_disconnect_by_func (handle->surface, surface_render, widget);
  g_signal_handlers_disconnect_by_func (handle->surface, surface_mapped_changed, widget);

  gdk_surface_set_widget (handle->surface, NULL);
  g_clear_pointer (&handle->surface, gdk_surface_destroy);
}

static void
text_handle_set_up_gesture (BobguiTextHandle *handle)
{
  BobguiNative *native;

  /* The drag gesture is hooked on the parent native */
  native = bobgui_widget_get_native (bobgui_widget_get_parent (BOBGUI_WIDGET (handle)));
  handle->controller_widget = BOBGUI_WIDGET (native);

  handle->controller = BOBGUI_EVENT_CONTROLLER (bobgui_gesture_drag_new ());
  bobgui_event_controller_set_propagation_phase (handle->controller,
                                              BOBGUI_PHASE_CAPTURE);
  g_signal_connect (handle->controller, "drag-begin",
                    G_CALLBACK (handle_drag_begin), handle);
  g_signal_connect (handle->controller, "drag-update",
                    G_CALLBACK (handle_drag_update), handle);
  g_signal_connect (handle->controller, "drag-end",
                    G_CALLBACK (handle_drag_end), handle);

  bobgui_widget_add_controller (handle->controller_widget, handle->controller);
}

static void
unset_surface_transform_changed_cb (gpointer user_data)
{
  BobguiTextHandle *handle = BOBGUI_TEXT_HANDLE (user_data);

  handle->surface_transform_changed_cb = 0;
}

static gboolean
surface_transform_changed_cb (BobguiWidget               *widget,
                              const graphene_matrix_t *transform,
                              gpointer                 user_data)
{
  BobguiTextHandle *handle = BOBGUI_TEXT_HANDLE (user_data);

  bobgui_text_handle_present_surface (handle);

  return G_SOURCE_CONTINUE;
}

static void
bobgui_text_handle_map (BobguiWidget *widget)
{
  BobguiTextHandle *handle = BOBGUI_TEXT_HANDLE (widget);

  handle->surface_transform_changed_cb =
    bobgui_widget_add_surface_transform_changed_callback (bobgui_widget_get_parent (widget),
                                                       surface_transform_changed_cb,
                                                       widget,
                                                       unset_surface_transform_changed_cb);

  BOBGUI_WIDGET_CLASS (bobgui_text_handle_parent_class)->map (widget);

  if (handle->has_point)
    {
      bobgui_text_handle_present_surface (handle);
      text_handle_set_up_gesture (handle);
    }
}

static void
bobgui_text_handle_unmap (BobguiWidget *widget)
{
  BobguiTextHandle *handle = BOBGUI_TEXT_HANDLE (widget);

  bobgui_widget_remove_surface_transform_changed_callback (bobgui_widget_get_parent (widget),
                                                        handle->surface_transform_changed_cb);
  handle->surface_transform_changed_cb = 0;

  BOBGUI_WIDGET_CLASS (bobgui_text_handle_parent_class)->unmap (widget);
  gdk_surface_hide (handle->surface);

  if (handle->controller_widget)
    {
      bobgui_widget_remove_controller (handle->controller_widget,
                                    handle->controller);
      handle->controller_widget = NULL;
      handle->controller = NULL;
    }
}

static void
bobgui_text_handle_dispose (GObject *object)
{
  BobguiTextHandle *handle = BOBGUI_TEXT_HANDLE (object);

  g_clear_pointer (&handle->contents, bobgui_widget_unparent);

  G_OBJECT_CLASS (bobgui_text_handle_parent_class)->dispose (object);
}

static void
bobgui_text_handle_class_init (BobguiTextHandleClass *klass)
{
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (klass);
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = bobgui_text_handle_dispose;

  widget_class->snapshot = bobgui_text_handle_snapshot;
  widget_class->realize = bobgui_text_handle_realize;
  widget_class->unrealize = bobgui_text_handle_unrealize;
  widget_class->map = bobgui_text_handle_map;
  widget_class->unmap = bobgui_text_handle_unmap;

  signals[HANDLE_DRAGGED] =
    g_signal_new (I_("handle-dragged"),
                  G_OBJECT_CLASS_TYPE (object_class),
                  G_SIGNAL_RUN_LAST, 0,
                  NULL, NULL,
                  _gdk_marshal_VOID__INT_INT,
                  G_TYPE_NONE, 2,
                  G_TYPE_INT, G_TYPE_INT);
  g_signal_set_va_marshaller (signals[HANDLE_DRAGGED],
                              G_OBJECT_CLASS_TYPE (object_class),
                              _gdk_marshal_VOID__INT_INTv);

  signals[DRAG_STARTED] =
    g_signal_new (I_("drag-started"),
                  G_OBJECT_CLASS_TYPE (object_class),
                  G_SIGNAL_RUN_LAST, 0,
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 0, G_TYPE_NONE);

  signals[DRAG_FINISHED] =
    g_signal_new (I_("drag-finished"),
                  G_OBJECT_CLASS_TYPE (object_class),
                  G_SIGNAL_RUN_LAST, 0,
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 0, G_TYPE_NONE);

  bobgui_widget_class_set_css_name (widget_class, I_("cursor-handle"));
  bobgui_widget_class_set_layout_manager_type (widget_class, BOBGUI_TYPE_BIN_LAYOUT);
}

/* Relative to pointing_to x/y */
static void
handle_get_input_extents (BobguiTextHandle *handle,
                          BobguiBorder     *border)
{
  BobguiWidget *widget = BOBGUI_WIDGET (handle);

  if (handle->role == BOBGUI_TEXT_HANDLE_ROLE_CURSOR)
    {
      border->left = (-bobgui_widget_get_width (widget) / 2) - handle->border.left;
      border->right = (bobgui_widget_get_width (widget) / 2) + handle->border.right;
    }
  else if ((handle->role == BOBGUI_TEXT_HANDLE_ROLE_SELECTION_END &&
            bobgui_widget_get_direction (widget) == BOBGUI_TEXT_DIR_RTL) ||
           (handle->role == BOBGUI_TEXT_HANDLE_ROLE_SELECTION_START &&
            bobgui_widget_get_direction (widget) != BOBGUI_TEXT_DIR_RTL))
    {
      border->left = -bobgui_widget_get_width (widget) - handle->border.left;
      border->right = handle->border.right;
    }
  else
    {
      border->left = -handle->border.left;
      border->right = bobgui_widget_get_width (widget) + handle->border.right;
    }

  border->top = - handle->border.top;
  border->bottom = bobgui_widget_get_height (widget) + handle->border.bottom;
}

static void
handle_drag_begin (BobguiGestureDrag *gesture,
                   double          x,
                   double          y,
                   BobguiTextHandle  *handle)
{
  BobguiBorder input_extents;
  graphene_point_t p;

  x -= handle->pointing_to.x;
  y -= handle->pointing_to.y;

  /* Figure out if the coordinates fall into the handle input area, coordinates
   * are relative to the parent widget.
   */
  handle_get_input_extents (handle, &input_extents);
  if (!bobgui_widget_compute_point (handle->controller_widget,
                                 bobgui_widget_get_parent (BOBGUI_WIDGET (handle)),
                                 &GRAPHENE_POINT_INIT (x, y),
                                 &p))
    graphene_point_init (&p, x, y);

  if (p.x < input_extents.left || p.x >= input_extents.right ||
      p.y < input_extents.top || p.y >= input_extents.bottom)
    {
      bobgui_gesture_set_state (BOBGUI_GESTURE (gesture), BOBGUI_EVENT_SEQUENCE_DENIED);
      return;
    }

  bobgui_gesture_set_state (BOBGUI_GESTURE (gesture), BOBGUI_EVENT_SEQUENCE_CLAIMED);
  /* Store untranslated coordinates here, so ::update does not need
   * an extra translation
   */
  handle->dx = x;
  handle->dy = y;
  handle->dragged = TRUE;
  g_signal_emit (handle, signals[DRAG_STARTED], 0);
}

static void
handle_drag_update (BobguiGestureDrag *gesture,
                    double          offset_x,
                    double          offset_y,
                    BobguiWidget      *widget)
{
  BobguiTextHandle *handle = BOBGUI_TEXT_HANDLE (widget);
  double start_x, start_y;
  int x, y;

  bobgui_gesture_drag_get_start_point (gesture, &start_x, &start_y);

  x = start_x + offset_x - handle->dx;
  y = start_y + offset_y - handle->dy + (handle->pointing_to.height / 2);
  g_signal_emit (widget, signals[HANDLE_DRAGGED], 0, x, y);
}

static void
handle_drag_end (BobguiGestureDrag *gesture,
                 double          offset_x,
                 double          offset_y,
                 BobguiTextHandle  *handle)
{
  GdkEventSequence *sequence;
  BobguiEventSequenceState state;

  sequence = bobgui_gesture_single_get_current_sequence (BOBGUI_GESTURE_SINGLE (gesture));
  state = bobgui_gesture_get_sequence_state (BOBGUI_GESTURE (gesture), sequence);

  if (state == BOBGUI_EVENT_SEQUENCE_CLAIMED)
    g_signal_emit (handle, signals[DRAG_FINISHED], 0);

  handle->dragged = FALSE;
}

static void
bobgui_text_handle_update_for_role (BobguiTextHandle *handle)
{
  BobguiWidget *widget = BOBGUI_WIDGET (handle);

  if (handle->role == BOBGUI_TEXT_HANDLE_ROLE_CURSOR)
    {
      bobgui_widget_remove_css_class (widget, "top");
      bobgui_widget_add_css_class (widget, "bottom");
      bobgui_widget_add_css_class (widget, "insertion-cursor");
    }
  else if (handle->role == BOBGUI_TEXT_HANDLE_ROLE_SELECTION_END)
    {
      bobgui_widget_remove_css_class (widget, "top");
      bobgui_widget_add_css_class (widget, "bottom");
      bobgui_widget_remove_css_class (widget, "insertion-cursor");
    }
  else if (handle->role == BOBGUI_TEXT_HANDLE_ROLE_SELECTION_START)
    {
      bobgui_widget_add_css_class (widget, "top");
      bobgui_widget_remove_css_class (widget, "bottom");
      bobgui_widget_remove_css_class (widget, "insertion-cursor");
    }

  bobgui_widget_queue_draw (widget);
}

static void
bobgui_text_handle_init (BobguiTextHandle *handle)
{
  handle->contents = bobgui_gizmo_new ("contents", NULL, NULL, NULL, NULL, NULL, NULL);
  bobgui_widget_set_can_target (handle->contents, FALSE);
  bobgui_widget_set_parent (handle->contents, BOBGUI_WIDGET (handle));

  bobgui_text_handle_update_for_role (handle);
}

BobguiTextHandle *
bobgui_text_handle_new (BobguiWidget *parent)
{
  BobguiTextHandle *handle;

  handle = g_object_new (BOBGUI_TYPE_TEXT_HANDLE, NULL);
  bobgui_widget_set_parent (BOBGUI_WIDGET (handle), parent);

  return handle;
}

void
bobgui_text_handle_set_role (BobguiTextHandle     *handle,
                          BobguiTextHandleRole  role)
{
  g_return_if_fail (BOBGUI_IS_TEXT_HANDLE (handle));

  if (handle->role == role)
    return;

  handle->role = role;
  bobgui_text_handle_update_for_role (handle);

  if (bobgui_widget_get_visible (BOBGUI_WIDGET (handle)))
    {
      if (handle->has_point)
        bobgui_text_handle_present_surface (handle);
    }
}

BobguiTextHandleRole
bobgui_text_handle_get_role (BobguiTextHandle *handle)
{
  g_return_val_if_fail (BOBGUI_IS_TEXT_HANDLE (handle), BOBGUI_TEXT_HANDLE_ROLE_CURSOR);

  return handle->role;
}

void
bobgui_text_handle_set_position (BobguiTextHandle      *handle,
                              const GdkRectangle *rect)
{
  g_return_if_fail (BOBGUI_IS_TEXT_HANDLE (handle));

  if (handle->pointing_to.x == rect->x &&
      handle->pointing_to.y == rect->y &&
      handle->pointing_to.width == rect->width &&
      handle->pointing_to.height == rect->height)
    return;

  handle->pointing_to = *rect;
  handle->has_point = TRUE;

  if (bobgui_widget_is_visible (BOBGUI_WIDGET (handle)))
    bobgui_text_handle_present_surface (handle);
}

gboolean
bobgui_text_handle_get_is_dragged (BobguiTextHandle *handle)
{
  g_return_val_if_fail (BOBGUI_IS_TEXT_HANDLE (handle), FALSE);

  return handle->dragged;
}
