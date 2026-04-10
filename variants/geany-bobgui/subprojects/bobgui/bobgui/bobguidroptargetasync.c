/* BOBGUI - The Bobgui Framework
 * Copyright (C) 1995-1999 Peter Mattis, Spencer Kimball and Josh MacDonald
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

/*
 * Modified by the BOBGUI Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the BOBGUI Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * BOBGUI at ftp://ftp.bobgui.org/pub/bobgui/.
 */

#include "config.h"

#include "bobguidroptargetasync.h"

#include "bobguidropprivate.h"
#include "gdk/gdkdropprivate.h"
#include "bobguieventcontrollerprivate.h"
#include "bobguimarshalers.h"
#include "gdk/gdkmarshalers.h"
#include "bobguinative.h"
#include "bobguitypebuiltins.h"
#include "bobguiprivate.h"


/**
 * BobguiDropTargetAsync:
 *
 * An event controller to receive Drag-and-Drop operations, asynchronously.
 *
 * It is the more complete but also more complex method of handling drop
 * operations compared to [class@Bobgui.DropTarget], and you should only use
 * it if `BobguiDropTarget` doesn't provide all the features you need.
 *
 * To use a `BobguiDropTargetAsync` to receive drops on a widget, you create
 * a `BobguiDropTargetAsync` object, configure which data formats and actions
 * you support, connect to its signals, and then attach it to the widget
 * with [method@Bobgui.Widget.add_controller].
 *
 * During a drag operation, the first signal that a `BobguiDropTargetAsync`
 * emits is [signal@Bobgui.DropTargetAsync::accept], which is meant to determine
 * whether the target is a possible drop site for the ongoing drop. The
 * default handler for the ::accept signal accepts the drop if it finds
 * a compatible data format and an action that is supported on both sides.
 *
 * If it is, and the widget becomes a target, you will receive a
 * [signal@Bobgui.DropTargetAsync::drag-enter] signal, followed by
 * [signal@Bobgui.DropTargetAsync::drag-motion] signals as the pointer moves,
 * optionally a [signal@Bobgui.DropTargetAsync::drop] signal when a drop happens,
 * and finally a [signal@Bobgui.DropTargetAsync::drag-leave] signal when the
 * pointer moves off the widget.
 *
 * The ::drag-enter and ::drag-motion handler return a `GdkDragAction`
 * to update the status of the ongoing operation. The ::drop handler
 * should decide if it ultimately accepts the drop and if it does, it
 * should initiate the data transfer and finish the operation by calling
 * [method@Gdk.Drop.finish].
 *
 * Between the ::drag-enter and ::drag-leave signals the widget is a
 * current drop target, and will receive the %BOBGUI_STATE_FLAG_DROP_ACTIVE
 * state, which can be used by themes to style the widget as a drop target.
 */

struct _BobguiDropTargetAsync
{
  BobguiEventController parent_object;

  GdkContentFormats *formats;
  GdkDragAction actions;

  GdkDrop *drop;
  gboolean rejected;
};

struct _BobguiDropTargetAsyncClass
{
  BobguiEventControllerClass parent_class;

  gboolean              (* accept)                              (BobguiDropTargetAsync     *self,
                                                                 GdkDrop                *drop);
  GdkDragAction         (* drag_enter)                          (BobguiDropTargetAsync     *self,
                                                                 GdkDrop                *drop,
                                                                 double                  x,
                                                                 double                  y);
  GdkDragAction         (* drag_motion)                         (BobguiDropTargetAsync     *self,
                                                                 GdkDrop                *drop,
                                                                 double                  x,
                                                                 double                  y);
  void                  (* drag_leave)                          (BobguiDropTargetAsync     *self,
                                                                 GdkDrop                *drop);
  gboolean              (* drop)                                (BobguiDropTargetAsync     *self,
                                                                 GdkDrop                *drop,
                                                                 double                  x,
                                                                 double                  y);
};

enum {
  PROP_0,
  PROP_ACTIONS,
  PROP_FORMATS,
  NUM_PROPERTIES
};

static GParamSpec *properties[NUM_PROPERTIES];

enum {
  ACCEPT,
  DRAG_ENTER,
  DRAG_MOTION,
  DRAG_LEAVE,
  DROP,
  NUM_SIGNALS
};

static guint signals[NUM_SIGNALS];

G_DEFINE_TYPE (BobguiDropTargetAsync, bobgui_drop_target_async, BOBGUI_TYPE_EVENT_CONTROLLER);

static gboolean
bobgui_drop_target_async_accept (BobguiDropTargetAsync *self,
                              GdkDrop            *drop)
{
  if ((gdk_drop_get_actions (drop) & self->actions) == GDK_ACTION_NONE)
    return FALSE;

  if (self->formats == NULL)
    return TRUE;

  return gdk_content_formats_match (self->formats, gdk_drop_get_formats (drop));
}

static GdkDragAction
make_action_unique (GdkDragAction actions)
{
  if (actions & GDK_ACTION_COPY)
    return GDK_ACTION_COPY;

  if (actions & GDK_ACTION_MOVE)
    return GDK_ACTION_MOVE;

  if (actions & GDK_ACTION_LINK)
    return GDK_ACTION_LINK;

  return GDK_ACTION_NONE;
}

static GdkDragAction
bobgui_drop_target_async_drag_enter (BobguiDropTargetAsync *self,
                                  GdkDrop            *drop,
                                  double              x,
                                  double              y)
{
  return make_action_unique (self->actions & gdk_drop_get_actions (drop));
}

static GdkDragAction
bobgui_drop_target_async_drag_motion (BobguiDropTargetAsync *self,
                                   GdkDrop            *drop,
                                   double              x,
                                   double              y)
{
  return make_action_unique (self->actions & gdk_drop_get_actions (drop));
}

static gboolean
bobgui_drop_target_async_drop (BobguiDropTargetAsync  *self,
                            GdkDrop             *drop,
                            double               x,
                            double               y)
{
  return FALSE;
}

static gboolean
bobgui_drop_target_async_filter_event (BobguiEventController *controller,
                                    GdkEvent           *event)
{
  switch ((int)gdk_event_get_event_type (event))
    {
    case GDK_DRAG_ENTER:
    case GDK_DRAG_LEAVE:
    case GDK_DRAG_MOTION:
    case GDK_DROP_START:
      return BOBGUI_EVENT_CONTROLLER_CLASS (bobgui_drop_target_async_parent_class)->filter_event (controller, event);

    default:;
    }

  return TRUE;
}

static gboolean
bobgui_drop_target_async_handle_event (BobguiEventController *controller,
                                    GdkEvent           *event,
                                    double              x,
                                    double              y)
{
  BobguiDropTargetAsync *self = BOBGUI_DROP_TARGET_ASYNC (controller);
  BobguiWidget *widget = bobgui_event_controller_get_widget (controller);
  GdkDrop *drop;

  switch ((int) gdk_event_get_event_type (event))
    {
    case GDK_DRAG_MOTION:
      {
        GdkDragAction preferred_action;

        drop = gdk_dnd_event_get_drop (event);
        /* sanity check */
        g_return_val_if_fail (self->drop == drop, FALSE);
        if (self->rejected)
          return FALSE;

        g_signal_emit (self, signals[DRAG_MOTION], 0, drop, x, y, &preferred_action);
        if (preferred_action && self->drop && !self->rejected &&
            bobgui_drop_status (self->drop, self->actions, preferred_action))
          {
            bobgui_widget_set_state_flags (widget, BOBGUI_STATE_FLAG_DROP_ACTIVE, FALSE);
          }
        else
          {
            bobgui_widget_unset_state_flags (widget, BOBGUI_STATE_FLAG_DROP_ACTIVE);
          }
      }
      return FALSE;

    case GDK_DROP_START:
      {
        gboolean handled;

        drop = gdk_dnd_event_get_drop (event);
        /* sanity check */
        g_return_val_if_fail (self->drop == drop, FALSE);
        if (self->rejected)
          return FALSE;

        g_signal_emit (self, signals[DROP], 0, self->drop, x, y, &handled);

        g_clear_object (&self->drop);
        bobgui_widget_unset_state_flags (widget, BOBGUI_STATE_FLAG_DROP_ACTIVE);

        return handled;
      }

    default:
      return FALSE;
    }
}

static void
bobgui_drop_target_async_handle_crossing (BobguiEventController    *controller,
                                       const BobguiCrossingData *crossing,
                                       double                 x,
                                       double                 y)
{
  BobguiDropTargetAsync *self = BOBGUI_DROP_TARGET_ASYNC (controller);
  BobguiWidget *widget = bobgui_event_controller_get_widget (controller);

  if (crossing->type != BOBGUI_CROSSING_DROP)
    return;

  if (self->drop && gdk_drop_is_finished (self->drop))
    g_clear_object (&self->drop);

  /* sanity check */
  g_warn_if_fail (self->drop == NULL || self->drop == crossing->drop);

  if (crossing->direction == BOBGUI_CROSSING_IN)
    {
      gboolean accept = FALSE;
      GdkDragAction preferred_action;

      if (self->drop != NULL)
        return;

      self->drop = g_object_ref (crossing->drop);

      g_signal_emit (self, signals[ACCEPT], 0, self->drop, &accept);
      self->rejected = !accept;
      if (self->rejected || !self->drop)
        return;

      g_signal_emit (self, signals[DRAG_ENTER], 0, self->drop, x, y, &preferred_action);
      if (preferred_action && self->drop && !self->rejected &&
          bobgui_drop_status (self->drop, self->actions, preferred_action))
        {
          bobgui_widget_set_state_flags (widget, BOBGUI_STATE_FLAG_DROP_ACTIVE, FALSE);
        }
    }
  else
    {
      if (crossing->new_descendent != NULL ||
          crossing->new_target == widget ||
          self->drop == NULL)
        return;

      g_signal_emit (self, signals[DRAG_LEAVE], 0, self->drop);
      g_clear_object (&self->drop);
      bobgui_widget_unset_state_flags (widget, BOBGUI_STATE_FLAG_DROP_ACTIVE);
    }
}

static void
bobgui_drop_target_async_reset (BobguiEventController *controller)
{
  BobguiDropTargetAsync *self = BOBGUI_DROP_TARGET_ASYNC (controller);
  BobguiWidget *widget = bobgui_event_controller_get_widget (controller);

  if (self->drop)
    {
      g_signal_emit (self, signals[DRAG_LEAVE], 0, self->drop);
      g_clear_object (&self->drop);
    }
  if (widget)
    bobgui_widget_unset_state_flags (widget, BOBGUI_STATE_FLAG_DROP_ACTIVE);
}

static void
bobgui_drop_target_async_finalize (GObject *object)
{
  BobguiDropTargetAsync *self = BOBGUI_DROP_TARGET_ASYNC (object);

  g_clear_object (&self->drop);
  g_clear_pointer (&self->formats, gdk_content_formats_unref);

  G_OBJECT_CLASS (bobgui_drop_target_async_parent_class)->finalize (object);
}

static void
bobgui_drop_target_async_set_property (GObject      *object,
                                    guint         prop_id,
                                    const GValue *value,
                                    GParamSpec   *pspec)
{
  BobguiDropTargetAsync *self = BOBGUI_DROP_TARGET_ASYNC (object);

  switch (prop_id)
    {
    case PROP_ACTIONS:
      bobgui_drop_target_async_set_actions (self, g_value_get_flags (value));
      break;

    case PROP_FORMATS:
      bobgui_drop_target_async_set_formats (self, g_value_get_boxed (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
bobgui_drop_target_async_get_property (GObject    *object,
                                    guint       prop_id,
                                    GValue     *value,
                                    GParamSpec *pspec)
{
  BobguiDropTargetAsync *self = BOBGUI_DROP_TARGET_ASYNC (object);

  switch (prop_id)
    {
    case PROP_ACTIONS:
      g_value_set_flags (value, bobgui_drop_target_async_get_actions (self));
      break;

    case PROP_FORMATS:
      g_value_set_boxed (value, bobgui_drop_target_async_get_formats (self));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
bobgui_drop_target_async_class_init (BobguiDropTargetAsyncClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  BobguiEventControllerClass *controller_class = BOBGUI_EVENT_CONTROLLER_CLASS (class);

  object_class->finalize = bobgui_drop_target_async_finalize;
  object_class->set_property = bobgui_drop_target_async_set_property;
  object_class->get_property = bobgui_drop_target_async_get_property;

  controller_class->handle_event = bobgui_drop_target_async_handle_event;
  controller_class->filter_event = bobgui_drop_target_async_filter_event;
  controller_class->handle_crossing = bobgui_drop_target_async_handle_crossing;
  controller_class->reset = bobgui_drop_target_async_reset;

  class->accept = bobgui_drop_target_async_accept;
  class->drag_enter = bobgui_drop_target_async_drag_enter;
  class->drag_motion = bobgui_drop_target_async_drag_motion;
  class->drop = bobgui_drop_target_async_drop;

  /**
   * BobguiDropTargetAsync:actions:
   *
   * The `GdkDragActions` that this drop target supports.
   */
  properties[PROP_ACTIONS] =
       g_param_spec_flags ("actions", NULL, NULL,
                           GDK_TYPE_DRAG_ACTION, GDK_ACTION_NONE,
                           G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiDropTargetAsync:formats:
   *
   * The `GdkContentFormats` that determines the supported data formats.
   */
  properties[PROP_FORMATS] =
       g_param_spec_boxed ("formats", NULL, NULL,
                           GDK_TYPE_CONTENT_FORMATS,
                           G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, NUM_PROPERTIES, properties);

 /**
   * BobguiDropTargetAsync::accept:
   * @self: the `BobguiDropTargetAsync`
   * @drop: the `GdkDrop`
   *
   * Emitted on the drop site when a drop operation is about to begin.
   *
   * If the drop is not accepted, %FALSE will be returned and the drop target
   * will ignore the drop. If %TRUE is returned, the drop is accepted for now
   * but may be rejected later via a call to [method@Bobgui.DropTargetAsync.reject_drop]
   * or ultimately by returning %FALSE from a [signal@Bobgui.DropTargetAsync::drop]
   * handler.
   *
   * The default handler for this signal decides whether to accept the drop
   * based on the formats provided by the @drop.
   *
   * If the decision whether the drop will be accepted or rejected needs
   * further processing, such as inspecting the data, this function should
   * return %TRUE and proceed as is @drop was accepted and if it decides to
   * reject the drop later, it should call [method@Bobgui.DropTargetAsync.reject_drop].
   *
   * Returns: %TRUE if @drop is accepted
   */
  signals[ACCEPT] =
      g_signal_new (I_("accept"),
                    G_TYPE_FROM_CLASS (class),
                    G_SIGNAL_RUN_LAST,
                    G_STRUCT_OFFSET (BobguiDropTargetAsyncClass, accept),
                    g_signal_accumulator_first_wins, NULL,
                    _gdk_marshal_BOOLEAN__OBJECT,
                    G_TYPE_BOOLEAN, 1,
                    GDK_TYPE_DROP);
   g_signal_set_va_marshaller (signals[ACCEPT],
                               BOBGUI_TYPE_DROP_TARGET_ASYNC,
                               _gdk_marshal_BOOLEAN__OBJECTv);

  /**
   * BobguiDropTargetAsync::drag-enter:
   * @self: the `BobguiDropTargetAsync`
   * @drop: the `GdkDrop`
   * @x: the x coordinate of the current pointer position
   * @y: the y coordinate of the current pointer position
   *
   * Emitted on the drop site when the pointer enters the widget.
   *
   * It can be used to set up custom highlighting.
   *
   * Returns: Preferred action for this drag operation.
   */
  signals[DRAG_ENTER] =
      g_signal_new (I_("drag-enter"),
                    G_TYPE_FROM_CLASS (class),
                    G_SIGNAL_RUN_LAST,
                    G_STRUCT_OFFSET (BobguiDropTargetAsyncClass, drag_enter),
                    g_signal_accumulator_first_wins, NULL,
                    _bobgui_marshal_FLAGS__OBJECT_DOUBLE_DOUBLE,
                    GDK_TYPE_DRAG_ACTION, 3,
                    GDK_TYPE_DROP, G_TYPE_DOUBLE, G_TYPE_DOUBLE);
   g_signal_set_va_marshaller (signals[DRAG_ENTER],
                               BOBGUI_TYPE_DROP_TARGET_ASYNC,
                               _bobgui_marshal_FLAGS__OBJECT_DOUBLE_DOUBLEv);

  /**
   * BobguiDropTargetAsync::drag-motion:
   * @self: the `BobguiDropTargetAsync`
   * @drop: the `GdkDrop`
   * @x: the x coordinate of the current pointer position
   * @y: the y coordinate of the current pointer position
   *
   * Emitted while the pointer is moving over the drop target.
   *
   * Returns: Preferred action for this drag operation.
   */
  signals[DRAG_MOTION] =
      g_signal_new (I_("drag-motion"),
                    G_TYPE_FROM_CLASS (class),
                    G_SIGNAL_RUN_LAST,
                    G_STRUCT_OFFSET (BobguiDropTargetAsyncClass, drag_motion),
                    g_signal_accumulator_first_wins, NULL,
                    _bobgui_marshal_FLAGS__OBJECT_DOUBLE_DOUBLE,
                    GDK_TYPE_DRAG_ACTION, 3,
                    GDK_TYPE_DROP, G_TYPE_DOUBLE, G_TYPE_DOUBLE);
   g_signal_set_va_marshaller (signals[DRAG_MOTION],
                               BOBGUI_TYPE_DROP_TARGET_ASYNC,
                               _bobgui_marshal_FLAGS__OBJECT_DOUBLE_DOUBLEv);

  /**
   * BobguiDropTargetAsync::drag-leave:
   * @self: the `BobguiDropTargetAsync`
   * @drop: the `GdkDrop`
   *
   * Emitted on the drop site when the pointer leaves the widget.
   *
   * Its main purpose it to undo things done in
   * `BobguiDropTargetAsync`::drag-enter.
   */
  signals[DRAG_LEAVE] =
      g_signal_new (I_("drag-leave"),
                    G_TYPE_FROM_CLASS (class),
                    G_SIGNAL_RUN_LAST,
                    G_STRUCT_OFFSET (BobguiDropTargetAsyncClass, drag_leave),
                    NULL, NULL,
                    NULL,
                    G_TYPE_NONE, 1,
                    GDK_TYPE_DROP);

  /**
   * BobguiDropTargetAsync::drop:
   * @self: the `BobguiDropTargetAsync`
   * @drop: the `GdkDrop`
   * @x: the x coordinate of the current pointer position
   * @y: the y coordinate of the current pointer position
   *
   * Emitted on the drop site when the user drops the data onto the widget.
   *
   * The signal handler must determine whether the pointer position is in a
   * drop zone or not. If it is not in a drop zone, it returns %FALSE and no
   * further processing is necessary.
   *
   * Otherwise, the handler returns %TRUE. In this case, this handler will
   * accept the drop. The handler must ensure that [method@Gdk.Drop.finish]
   * is called to let the source know that the drop is done. The call to
   * [method@Gdk.Drop.finish] must only be done when all data has been received.
   *
   * To receive the data, use one of the read functions provided by
   * [class@Gdk.Drop] such as [method@Gdk.Drop.read_async] or
   * [method@Gdk.Drop.read_value_async].
   *
   * Returns: whether the drop is accepted at the given pointer position
   */
  signals[DROP] =
      g_signal_new (I_("drop"),
                    G_TYPE_FROM_CLASS (class),
                    G_SIGNAL_RUN_LAST,
                    0,
                    g_signal_accumulator_first_wins, NULL,
                    _bobgui_marshal_BOOLEAN__OBJECT_DOUBLE_DOUBLE,
                    G_TYPE_BOOLEAN, 3,
                    GDK_TYPE_DROP, G_TYPE_DOUBLE, G_TYPE_DOUBLE);
   g_signal_set_va_marshaller (signals[DROP],
                               BOBGUI_TYPE_DROP_TARGET_ASYNC,
                               _bobgui_marshal_BOOLEAN__OBJECT_DOUBLE_DOUBLEv);
}

static void
bobgui_drop_target_async_init (BobguiDropTargetAsync *self)
{
}

/**
 * bobgui_drop_target_async_new:
 * @formats: (nullable) (transfer full): the supported data formats
 * @actions: the supported actions
 *
 * Creates a new `BobguiDropTargetAsync` object.
 *
 * Returns: the new `BobguiDropTargetAsync`
 */
BobguiDropTargetAsync *
bobgui_drop_target_async_new (GdkContentFormats *formats,
                     GdkDragAction      actions)
{
  BobguiDropTargetAsync *result;

  result = g_object_new (BOBGUI_TYPE_DROP_TARGET_ASYNC,
                         "formats", formats,
                         "actions", actions,
                         NULL);

  g_clear_pointer (&formats, gdk_content_formats_unref);

  return result;
}

/**
 * bobgui_drop_target_async_set_formats:
 * @self: a `BobguiDropTargetAsync`
 * @formats: (nullable): the supported data formats or %NULL for any format
 *
 * Sets the data formats that this drop target will accept.
 */
void
bobgui_drop_target_async_set_formats (BobguiDropTargetAsync *self,
                                   GdkContentFormats  *formats)
{
  g_return_if_fail (BOBGUI_IS_DROP_TARGET_ASYNC (self));

  if (self->formats == formats)
    return;

  if (self->formats)
    gdk_content_formats_unref (self->formats);

  self->formats = formats;

  if (self->formats)
    gdk_content_formats_ref (self->formats);

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_FORMATS]);
}

/**
 * bobgui_drop_target_async_get_formats:
 * @self: a `BobguiDropTargetAsync`
 *
 * Gets the data formats that this drop target accepts.
 *
 * If the result is %NULL, all formats are expected to be supported.
 *
 * Returns: (nullable) (transfer none): the supported data formats
 */
GdkContentFormats *
bobgui_drop_target_async_get_formats (BobguiDropTargetAsync *self)
{
  g_return_val_if_fail (BOBGUI_IS_DROP_TARGET_ASYNC (self), NULL);

  return self->formats;
}

/**
 * bobgui_drop_target_async_set_actions:
 * @self: a `BobguiDropTargetAsync`
 * @actions: the supported actions
 *
 * Sets the actions that this drop target supports.
 */
void
bobgui_drop_target_async_set_actions (BobguiDropTargetAsync *self,
                                   GdkDragAction       actions)
{
  g_return_if_fail (BOBGUI_IS_DROP_TARGET_ASYNC (self));

  if (self->actions == actions)
    return;

  self->actions = actions;

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_ACTIONS]);
}

/**
 * bobgui_drop_target_async_get_actions:
 * @self: a `BobguiDropTargetAsync`
 *
 * Gets the actions that this drop target supports.
 *
 * Returns: the actions that this drop target supports
 */
GdkDragAction
bobgui_drop_target_async_get_actions (BobguiDropTargetAsync *self)
{
  g_return_val_if_fail (BOBGUI_IS_DROP_TARGET_ASYNC (self), GDK_ACTION_NONE);

  return self->actions;
}

/**
 * bobgui_drop_target_async_reject_drop:
 * @self: a `BobguiDropTargetAsync`
 * @drop: the `GdkDrop` of an ongoing drag operation
 *
 * Sets the @drop as not accepted on this drag site.
 *
 * This function should be used when delaying the decision
 * on whether to accept a drag or not until after reading
 * the data.
 */
void
bobgui_drop_target_async_reject_drop (BobguiDropTargetAsync *self,
                                   GdkDrop            *drop)
{
  g_return_if_fail (BOBGUI_IS_DROP_TARGET_ASYNC (self));
  g_return_if_fail (GDK_IS_DROP (drop));

  if (self->rejected)
    return;

  if (self->drop != drop)
    return;

  self->rejected = TRUE;
  bobgui_widget_unset_state_flags (bobgui_event_controller_get_widget (BOBGUI_EVENT_CONTROLLER (self)),
                                BOBGUI_STATE_FLAG_DROP_ACTIVE);
}
