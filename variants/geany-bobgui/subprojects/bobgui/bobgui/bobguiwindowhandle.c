/*
 * Copyright (c) 2020 Alexander Mikhaylenko <alexm@gnome.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "config.h"

#include "bobguiwindowhandle.h"

#include "bobguibinlayout.h"
#include "bobguibox.h"
#include "bobguibuildable.h"
#include "bobguibuilderprivate.h"
#include "bobguidragsourceprivate.h"
#include "bobguigestureclick.h"
#include "bobguigesturedrag.h"
#include "bobguigestureprivate.h"
#include <glib/gi18n-lib.h>
#include "bobguimodelbuttonprivate.h"
#include "bobguinative.h"
#include "bobguipopovermenuprivate.h"
#include "bobguiprivate.h"
#include "bobguiseparator.h"
#include "bobguiwidgetprivate.h"


/**
 * BobguiWindowHandle:
 *
 * Implements titlebar functionality for a window.
 *
 * When added into a window, it can be dragged to move the window,
 * and it implements the right click, double click and middle click
 * behaviors that are expected of a titlebar.
 *
 * # CSS nodes
 *
 * `BobguiWindowHandle` has a single CSS node with the name `windowhandle`.
 *
 * # Accessibility
 *
 * Until BOBGUI 4.10, `BobguiWindowHandle` used the [enum@Bobgui.AccessibleRole.group] role.
 *
 * Starting from BOBGUI 4.12, `BobguiWindowHandle` uses the [enum@Bobgui.AccessibleRole.generic]
 * role.
 */

struct _BobguiWindowHandle {
  BobguiWidget parent_instance;

  BobguiGesture *click_gesture;
  BobguiGesture *drag_gesture;

  BobguiWidget *child;
  BobguiWidget *fallback_menu;
};

enum {
  PROP_0,
  PROP_CHILD,
  LAST_PROP
};

static GParamSpec *props[LAST_PROP] = { NULL, };

static void bobgui_window_handle_buildable_iface_init (BobguiBuildableIface *iface);

G_DEFINE_TYPE_WITH_CODE (BobguiWindowHandle, bobgui_window_handle, BOBGUI_TYPE_WIDGET,
                         G_IMPLEMENT_INTERFACE (BOBGUI_TYPE_BUILDABLE, bobgui_window_handle_buildable_iface_init))

static void
lower_window (BobguiWindowHandle *self)
{
  GdkSurface *surface =
    bobgui_native_get_surface (bobgui_widget_get_native (BOBGUI_WIDGET (self)));

  gdk_toplevel_lower (GDK_TOPLEVEL (surface));
}

static BobguiWindow *
get_window (BobguiWindowHandle *self)
{
  BobguiRoot *root = bobgui_widget_get_root (BOBGUI_WIDGET (self));

  if (BOBGUI_IS_WINDOW (root))
    return BOBGUI_WINDOW (root);

  return NULL;
}

static void
restore_window_clicked (BobguiModelButton  *button,
                        BobguiWindowHandle *self)
{
  BobguiWindow *window = get_window (self);

  if (!window)
    return;

  if (bobgui_window_is_maximized (window))
    bobgui_window_unmaximize (window);
}

static void
minimize_window_clicked (BobguiModelButton  *button,
                         BobguiWindowHandle *self)
{
  BobguiWindow *window = get_window (self);

  if (!window)
    return;

  /* Turns out, we can't minimize a maximized window */
  if (bobgui_window_is_maximized (window))
    bobgui_window_unmaximize (window);

  bobgui_window_minimize (window);
}

static void
maximize_window_clicked (BobguiModelButton  *button,
                         BobguiWindowHandle *self)
{
  BobguiWindow *window = get_window (self);

  if (window)
    bobgui_window_maximize (window);
}

static void
close_window_clicked (BobguiModelButton  *button,
                      BobguiWindowHandle *self)
{
  BobguiWindow *window = get_window (self);

  if (window)
    bobgui_window_close (window);
}

static void
popup_menu_closed (BobguiPopover      *popover,
                   BobguiWindowHandle *self)
{
  g_clear_pointer (&self->fallback_menu, bobgui_widget_unparent);
}

static void
do_popup_fallback (BobguiWindowHandle *self,
                   GdkEvent        *event)
{
  GdkRectangle rect = { 0, 0, 1, 1 };
  GdkDevice *device;
  GdkSeat *seat;
  BobguiWidget *box, *menuitem;
  BobguiWindow *window;
  gboolean maximized, resizable, deletable;

  g_clear_pointer (&self->fallback_menu, bobgui_widget_unparent);

  window = get_window (self);

  if (window)
    {
      maximized = bobgui_window_is_maximized (window);
      resizable = bobgui_window_get_resizable (window);
      deletable = bobgui_window_get_deletable (window);
    }
  else
    {
      maximized = FALSE;
      resizable = FALSE;
      deletable = FALSE;
    }

  self->fallback_menu = bobgui_popover_menu_new ();
  bobgui_widget_set_parent (self->fallback_menu, BOBGUI_WIDGET (self));

  bobgui_popover_set_has_arrow (BOBGUI_POPOVER (self->fallback_menu), FALSE);
  bobgui_widget_set_halign (self->fallback_menu, BOBGUI_ALIGN_START);


  device = gdk_event_get_device (event);
  seat = gdk_event_get_seat (event);

  if (device == gdk_seat_get_keyboard (seat))
    device = gdk_seat_get_pointer (seat);

  if (device)
    {
      BobguiNative *native;
      GdkSurface *surface;
      double px, py;
      double nx, ny;
      graphene_point_t p;

      native = bobgui_widget_get_native (BOBGUI_WIDGET (self));
      surface = bobgui_native_get_surface (native);
      gdk_surface_get_device_position (surface, device, &px, &py, NULL);
      bobgui_native_get_surface_transform (native, &nx, &ny);

      if (!bobgui_widget_compute_point (BOBGUI_WIDGET (bobgui_widget_get_native (BOBGUI_WIDGET (self))),
                                     BOBGUI_WIDGET (self),
                                     &GRAPHENE_POINT_INIT (px - nx, py - ny),
                                     &p))
        graphene_point_init (&p, 0, 0);

      rect.x = p.x;
      rect.y = p.y;
    }

  bobgui_popover_set_pointing_to (BOBGUI_POPOVER (self->fallback_menu), &rect);

  box = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 0);
  bobgui_popover_menu_add_submenu (BOBGUI_POPOVER_MENU (self->fallback_menu), box, "main");

  menuitem = bobgui_model_button_new ();
  g_object_set (menuitem, "text", _("Restore"), NULL);
  bobgui_widget_set_sensitive (menuitem, maximized && resizable);
  g_signal_connect (G_OBJECT (menuitem), "clicked",
                    G_CALLBACK (restore_window_clicked), self);
  bobgui_box_append (BOBGUI_BOX (box), menuitem);

  menuitem = bobgui_model_button_new ();
  g_object_set (menuitem, "text", _("Minimize"), NULL);
  g_signal_connect (G_OBJECT (menuitem), "clicked",
                    G_CALLBACK (minimize_window_clicked), self);
  bobgui_box_append (BOBGUI_BOX (box), menuitem);

  menuitem = bobgui_model_button_new ();
  g_object_set (menuitem, "text", _("Maximize"), NULL);
  bobgui_widget_set_sensitive (menuitem, resizable && !maximized);
  g_signal_connect (G_OBJECT (menuitem), "clicked",
                    G_CALLBACK (maximize_window_clicked), self);
  bobgui_box_append (BOBGUI_BOX (box), menuitem);

  menuitem = bobgui_separator_new (BOBGUI_ORIENTATION_HORIZONTAL);
  bobgui_box_append (BOBGUI_BOX (box), menuitem);

  menuitem = bobgui_model_button_new ();
  g_object_set (menuitem, "text", _("Close"), NULL);
  bobgui_widget_set_sensitive (menuitem, deletable);
  g_signal_connect (G_OBJECT (menuitem), "clicked",
                    G_CALLBACK (close_window_clicked), self);
  bobgui_box_append (BOBGUI_BOX (box), menuitem);

  g_signal_connect (self->fallback_menu, "closed",
                    G_CALLBACK (popup_menu_closed), self);
  bobgui_popover_popup (BOBGUI_POPOVER (self->fallback_menu));
}

static void
do_popup (BobguiWindowHandle *self,
          BobguiGestureClick *gesture)
{
  GdkSurface *surface =
    bobgui_native_get_surface (bobgui_widget_get_native (BOBGUI_WIDGET (self)));
  GdkEventSequence *sequence;
  GdkEvent *event;

  sequence = bobgui_gesture_single_get_current_sequence (BOBGUI_GESTURE_SINGLE (gesture));
  event = bobgui_gesture_get_last_event (BOBGUI_GESTURE (gesture), sequence);
  if (!event)
    return;

  if (!gdk_toplevel_show_window_menu (GDK_TOPLEVEL (surface), event))
    do_popup_fallback (self, event);
}

static gboolean
perform_titlebar_action_fallback (BobguiWindowHandle    *self,
                                  BobguiGestureClick    *click_gesture,
                                  GdkTitlebarGesture  gesture)
{
  BobguiSettings *settings;
  char *action = NULL;
  gboolean retval = TRUE;

  settings = bobgui_widget_get_settings (BOBGUI_WIDGET (self));
  switch (gesture)
    {
    case GDK_TITLEBAR_GESTURE_DOUBLE_CLICK:
      g_object_get (settings, "bobgui-titlebar-double-click", &action, NULL);
      break;
    case GDK_TITLEBAR_GESTURE_MIDDLE_CLICK:
      g_object_get (settings, "bobgui-titlebar-middle-click", &action, NULL);
      break;
    case GDK_TITLEBAR_GESTURE_RIGHT_CLICK:
      g_object_get (settings, "bobgui-titlebar-right-click", &action, NULL);
      break;
    default:
      break;
    }

  if (action == NULL)
    retval = FALSE;
  else if (g_str_equal (action, "none"))
    retval = FALSE;
    /* treat all maximization variants the same */
  else if (g_str_has_prefix (action, "toggle-maximize"))
    bobgui_widget_activate_action (BOBGUI_WIDGET (self),
                                "window.toggle-maximized",
                                NULL);
  else if (g_str_equal (action, "lower"))
    lower_window (self);
  else if (g_str_equal (action, "minimize"))
    bobgui_widget_activate_action (BOBGUI_WIDGET (self),
                                "window.minimize",
                                NULL);
  else if (g_str_equal (action, "menu"))
    do_popup (self, click_gesture);
  else
    {
      g_warning ("Unsupported titlebar action %s", action);
      retval = FALSE;
    }

  g_free (action);

  return retval;
}

static gboolean
perform_titlebar_action (BobguiWindowHandle *self,
			 BobguiGestureClick *click_gesture,
                         guint            button,
                         int              n_press)
{
  GdkSurface *surface =
    bobgui_native_get_surface (bobgui_widget_get_native (BOBGUI_WIDGET (self)));
  GdkTitlebarGesture gesture;

  switch (button)
    {
    case GDK_BUTTON_PRIMARY:
      if (n_press == 2)
        gesture = GDK_TITLEBAR_GESTURE_DOUBLE_CLICK;
      else
        return FALSE;
      break;
    case GDK_BUTTON_MIDDLE:
      gesture = GDK_TITLEBAR_GESTURE_MIDDLE_CLICK;
      break;
    case GDK_BUTTON_SECONDARY:
      gesture = GDK_TITLEBAR_GESTURE_RIGHT_CLICK;
      break;
    default:
      return FALSE;
    }

  if (gdk_toplevel_titlebar_gesture (GDK_TOPLEVEL (surface), gesture))
    return TRUE;

  return perform_titlebar_action_fallback (self, click_gesture, gesture);
}

static void
click_gesture_pressed_cb (BobguiGestureClick *gesture,
                          int              n_press,
                          double           x,
                          double           y,
                          BobguiWindowHandle *self)
{
  BobguiWidget *widget;
  guint button;

  widget = BOBGUI_WIDGET (self);
  button = bobgui_gesture_single_get_current_button (BOBGUI_GESTURE_SINGLE (gesture));

  if (n_press > 1)
    bobgui_gesture_set_state (self->drag_gesture, BOBGUI_EVENT_SEQUENCE_DENIED);

  if (gdk_display_device_is_grabbed (bobgui_widget_get_display (widget),
                                     bobgui_gesture_get_device (BOBGUI_GESTURE (gesture))))
    {
      bobgui_gesture_set_state (self->drag_gesture, BOBGUI_EVENT_SEQUENCE_DENIED);
      return;
    }

  switch (button)
    {
    case GDK_BUTTON_PRIMARY:
      if (n_press == 2)
        {
          perform_titlebar_action (self, gesture, button, n_press);
          bobgui_gesture_set_state (BOBGUI_GESTURE (gesture),
				 BOBGUI_EVENT_SEQUENCE_CLAIMED);
        }
      break;

    case GDK_BUTTON_SECONDARY:
      if (perform_titlebar_action (self, gesture, button, n_press))
        bobgui_gesture_set_state (BOBGUI_GESTURE (gesture),
			       BOBGUI_EVENT_SEQUENCE_CLAIMED);

      bobgui_event_controller_reset (BOBGUI_EVENT_CONTROLLER (gesture));
      bobgui_event_controller_reset (BOBGUI_EVENT_CONTROLLER (self->drag_gesture));
      break;

    case GDK_BUTTON_MIDDLE:
      if (perform_titlebar_action (self, gesture, button, n_press))
        bobgui_gesture_set_state (BOBGUI_GESTURE (gesture),
			       BOBGUI_EVENT_SEQUENCE_CLAIMED);
      break;

    default:
      return;
    }
}

static void
drag_gesture_update_cb (BobguiGestureDrag  *gesture,
                        double           offset_x,
                        double           offset_y,
                        BobguiWindowHandle *self)
{
  if (bobgui_drag_check_threshold_double (BOBGUI_WIDGET (self), 0, 0, offset_x, offset_y))
    {
      double start_x, start_y;
      double native_x, native_y;
      graphene_point_t p;
      BobguiNative *native;
      GdkSurface *surface;

      bobgui_gesture_set_state (BOBGUI_GESTURE (gesture), BOBGUI_EVENT_SEQUENCE_CLAIMED);

      bobgui_gesture_drag_get_start_point (gesture, &start_x, &start_y);

      native = bobgui_widget_get_native (BOBGUI_WIDGET (self));

      if (!bobgui_widget_compute_point (BOBGUI_WIDGET (self),
                                     BOBGUI_WIDGET (native),
                                     &GRAPHENE_POINT_INIT (start_x, start_y),
                                     &p))
        graphene_point_init (&p, start_x, start_y);

      bobgui_native_get_surface_transform (native, &native_x, &native_y);
      p.x += native_x;
      p.y += native_y;

      if (BOBGUI_IS_WINDOW (native))
        bobgui_window_unfullscreen (BOBGUI_WINDOW (native));

      surface = bobgui_native_get_surface (native);
      if (GDK_IS_TOPLEVEL (surface))
        gdk_toplevel_begin_move (GDK_TOPLEVEL (surface),
                                 bobgui_gesture_get_device (BOBGUI_GESTURE (gesture)),
                                 bobgui_gesture_single_get_current_button (BOBGUI_GESTURE_SINGLE (gesture)),
                                 p.x, p.y,
                                 bobgui_event_controller_get_current_event_time (BOBGUI_EVENT_CONTROLLER (gesture)));

      bobgui_event_controller_reset (BOBGUI_EVENT_CONTROLLER (gesture));
      bobgui_event_controller_reset (BOBGUI_EVENT_CONTROLLER (self->click_gesture));
    }
}

static void
bobgui_window_handle_unrealize (BobguiWidget *widget)
{
  BobguiWindowHandle *self = BOBGUI_WINDOW_HANDLE (widget);

  g_clear_pointer (&self->fallback_menu, bobgui_widget_unparent);

  BOBGUI_WIDGET_CLASS (bobgui_window_handle_parent_class)->unrealize (widget);
}

static void
bobgui_window_handle_dispose (GObject *object)
{
  BobguiWindowHandle *self = BOBGUI_WINDOW_HANDLE (object);

  g_clear_pointer (&self->child, bobgui_widget_unparent);

  G_OBJECT_CLASS (bobgui_window_handle_parent_class)->dispose (object);
}

static void
bobgui_window_handle_get_property (GObject    *object,
                                guint       prop_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  BobguiWindowHandle *self = BOBGUI_WINDOW_HANDLE (object);

  switch (prop_id)
    {
    case PROP_CHILD:
      g_value_set_object (value, bobgui_window_handle_get_child (self));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bobgui_window_handle_set_property (GObject      *object,
                                guint         prop_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  BobguiWindowHandle *self = BOBGUI_WINDOW_HANDLE (object);

  switch (prop_id)
    {
    case PROP_CHILD:
      bobgui_window_handle_set_child (self, g_value_get_object (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bobgui_window_handle_class_init (BobguiWindowHandleClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (klass);

  object_class->dispose = bobgui_window_handle_dispose;
  object_class->get_property = bobgui_window_handle_get_property;
  object_class->set_property = bobgui_window_handle_set_property;

  widget_class->unrealize = bobgui_window_handle_unrealize;

  /**
   * BobguiWindowHandle:child:
   *
   * The child widget.
   */
  props[PROP_CHILD] =
      g_param_spec_object ("child", NULL, NULL,
                           BOBGUI_TYPE_WIDGET,
                           BOBGUI_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, LAST_PROP, props);

  bobgui_widget_class_set_layout_manager_type (widget_class, BOBGUI_TYPE_BIN_LAYOUT);
  bobgui_widget_class_set_css_name (widget_class, I_("windowhandle"));
  bobgui_widget_class_set_accessible_role (widget_class, BOBGUI_ACCESSIBLE_ROLE_GENERIC);
}

static void
bobgui_window_handle_init (BobguiWindowHandle *self)
{
  self->click_gesture = bobgui_gesture_click_new ();
  bobgui_gesture_single_set_button (BOBGUI_GESTURE_SINGLE (self->click_gesture), 0);
  g_signal_connect (self->click_gesture, "pressed",
                    G_CALLBACK (click_gesture_pressed_cb), self);
  bobgui_widget_add_controller (BOBGUI_WIDGET (self), BOBGUI_EVENT_CONTROLLER (self->click_gesture));

  self->drag_gesture = bobgui_gesture_drag_new ();
  g_signal_connect (self->drag_gesture, "drag-update",
                    G_CALLBACK (drag_gesture_update_cb), self);
  bobgui_widget_add_controller (BOBGUI_WIDGET (self), BOBGUI_EVENT_CONTROLLER (self->drag_gesture));
}

static BobguiBuildableIface *parent_buildable_iface;

static void
bobgui_window_handle_buildable_add_child (BobguiBuildable *buildable,
                                       BobguiBuilder   *builder,
                                       GObject      *child,
                                       const char   *type)
{
  if (BOBGUI_IS_WIDGET (child))
    {
      bobgui_buildable_child_deprecation_warning (buildable, builder, NULL, "child");
      bobgui_window_handle_set_child (BOBGUI_WINDOW_HANDLE (buildable), BOBGUI_WIDGET (child));
    }
  else
    {
      parent_buildable_iface->add_child (buildable, builder, child, type);
    }
}

static void
bobgui_window_handle_buildable_iface_init (BobguiBuildableIface *iface)
{
  parent_buildable_iface = g_type_interface_peek_parent (iface);

  iface->add_child = bobgui_window_handle_buildable_add_child;
}

/**
 * bobgui_window_handle_new:
 *
 * Creates a new `BobguiWindowHandle`.
 *
 * Returns: a new `BobguiWindowHandle`.
 */
BobguiWidget *
bobgui_window_handle_new (void)
{
  return g_object_new (BOBGUI_TYPE_WINDOW_HANDLE, NULL);
}

/**
 * bobgui_window_handle_get_child:
 * @self: a `BobguiWindowHandle`
 *
 * Gets the child widget of @self.
 *
 * Returns: (nullable) (transfer none): the child widget of @self
 */
BobguiWidget *
bobgui_window_handle_get_child (BobguiWindowHandle *self)
{
  g_return_val_if_fail (BOBGUI_IS_WINDOW_HANDLE (self), NULL);

  return self->child;
}

/**
 * bobgui_window_handle_set_child:
 * @self: a `BobguiWindowHandle`
 * @child: (nullable): the child widget
 *
 * Sets the child widget of @self.
 */
void
bobgui_window_handle_set_child (BobguiWindowHandle *self,
                             BobguiWidget       *child)
{
  g_return_if_fail (BOBGUI_IS_WINDOW_HANDLE (self));
  g_return_if_fail (child == NULL || self->child == child || bobgui_widget_get_parent (child) == NULL);

  if (self->child == child)
    return;

  g_clear_pointer (&self->child, bobgui_widget_unparent);

  self->child = child;

  if (child)
    bobgui_widget_set_parent (child, BOBGUI_WIDGET (self));

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_CHILD]);
}
