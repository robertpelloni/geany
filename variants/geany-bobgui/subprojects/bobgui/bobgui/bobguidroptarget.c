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

#include "bobguidroptarget.h"

#include "bobguidropprivate.h"
#include "bobguieventcontrollerprivate.h"
#include "bobguimarshalers.h"
#include "gdk/gdkmarshalers.h"
#include "bobguinative.h"
#include "bobguiprivate.h"
#include "bobguitypebuiltins.h"


/**
 * BobguiDropTarget:
 *
 * An event controller to receive Drag-and-Drop operations.
 *
 * The most basic way to use a `BobguiDropTarget` to receive drops on a
 * widget is to create it via [ctor@Bobgui.DropTarget.new], passing in the
 * `GType` of the data you want to receive and connect to the
 * [signal@Bobgui.DropTarget::drop] signal to receive the data:
 *
 * ```c
 * static gboolean
 * on_drop (BobguiDropTarget *target,
 *          const GValue  *value,
 *          double         x,
 *          double         y,
 *          gpointer       data)
 * {
 *   MyWidget *self = data;
 *
 *   // Call the appropriate setter depending on the type of data
 *   // that we received
 *   if (G_VALUE_HOLDS (value, G_TYPE_FILE))
 *     my_widget_set_file (self, g_value_get_object (value));
 *   else if (G_VALUE_HOLDS (value, GDK_TYPE_PIXBUF))
 *     my_widget_set_pixbuf (self, g_value_get_object (value));
 *   else
 *     return FALSE;
 *
 *   return TRUE;
 * }
 *
 * static void
 * my_widget_init (MyWidget *self)
 * {
 *   BobguiDropTarget *target =
 *     bobgui_drop_target_new (G_TYPE_INVALID, GDK_ACTION_COPY);
 *
 *   // This widget accepts two types of drop types: GFile objects
 *   // and GdkPixbuf objects
 *   bobgui_drop_target_set_gtypes (target, (GType [2]) {
 *     G_TYPE_FILE,
 *     GDK_TYPE_PIXBUF,
 *   }, 2);
 *
 *   g_signal_connect (target, "drop", G_CALLBACK (on_drop), self);
 *   bobgui_widget_add_controller (BOBGUI_WIDGET (self), BOBGUI_EVENT_CONTROLLER (target));
 * }
 * ```
 *
 * `BobguiDropTarget` supports more options, such as:
 *
 *  * rejecting potential drops via the [signal@Bobgui.DropTarget::accept] signal
 *    and the [method@Bobgui.DropTarget.reject] function to let other drop
 *    targets handle the drop
 *  * tracking an ongoing drag operation before the drop via the
 *    [signal@Bobgui.DropTarget::enter], [signal@Bobgui.DropTarget::motion] and
 *    [signal@Bobgui.DropTarget::leave] signals
 *  * configuring how to receive data by setting the
 *    [property@Bobgui.DropTarget:preload] property and listening for its
 *    availability via the [property@Bobgui.DropTarget:value] property
 *
 * However, `BobguiDropTarget` is ultimately modeled in a synchronous way
 * and only supports data transferred via `GType`. If you want full control
 * over an ongoing drop, the [class@Bobgui.DropTargetAsync] object gives you
 * this ability.
 *
 * While a pointer is dragged over the drop target's widget and the drop
 * has not been rejected, that widget will receive the
 * %BOBGUI_STATE_FLAG_DROP_ACTIVE state, which can be used to style the widget.
 *
 * If you are not interested in receiving the drop, but just want to update
 * UI state during a Drag-and-Drop operation (e.g. switching tabs), you can
 * use [class@Bobgui.DropControllerMotion].
 */

struct _BobguiDropTarget
{
  BobguiEventController parent_object;

  GdkContentFormats *formats;
  GdkDragAction actions;
  guint preload  : 1;
  guint entered  : 1;
  guint dropping : 1;
  graphene_point_t coords;
  GdkDrop *drop;
  GCancellable *cancellable; /* NULL unless doing a read of value */
  GValue value;
};

struct _BobguiDropTargetClass
{
  BobguiEventControllerClass parent_class;

  gboolean              (* accept)                              (BobguiDropTarget  *self,
                                                                 GdkDrop        *drop);
  GdkDragAction         (* enter)                               (BobguiDropTarget  *self,
                                                                 double          x,
                                                                 double          y);
  GdkDragAction         (* motion)                              (BobguiDropTarget  *self,
                                                                 double          x,
                                                                 double          y);
  void                  (* leave)                               (BobguiDropTarget  *self,
                                                                 GdkDrop        *drop);
  gboolean              (* drop)                                (BobguiDropTarget  *self,
                                                                 const GValue   *value,
                                                                 double          x,
                                                                 double          y);
};

enum {
  PROP_0,
  PROP_ACTIONS,
  PROP_CURRENT_DROP,
  PROP_DROP,
  PROP_FORMATS,
  PROP_PRELOAD,
  PROP_VALUE,
  NUM_PROPERTIES
};

static GParamSpec *properties[NUM_PROPERTIES];

enum {
  ACCEPT,
  ENTER,
  MOTION,
  LEAVE,
  DROP,
  NUM_SIGNALS
};

static guint signals[NUM_SIGNALS];

G_DEFINE_TYPE (BobguiDropTarget, bobgui_drop_target, BOBGUI_TYPE_EVENT_CONTROLLER);

static void
bobgui_drop_target_end_drop (BobguiDropTarget *self)
{
  if (self->drop == NULL)
    return;

  g_object_freeze_notify (G_OBJECT (self));

  if (self->dropping)
    {
      gdk_drop_finish (self->drop, GDK_ACTION_NONE);
      self->dropping = FALSE;
    }

  g_clear_object (&self->drop);
  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_DROP]);
  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_CURRENT_DROP]);

  if (G_IS_VALUE (&self->value))
    {
      g_value_unset (&self->value);
      g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_VALUE]);
    }

  if (self->cancellable)
    {
      g_cancellable_cancel (self->cancellable);
      g_clear_object (&self->cancellable);
    }

  bobgui_widget_unset_state_flags (bobgui_event_controller_get_widget (BOBGUI_EVENT_CONTROLLER (self)),
                                BOBGUI_STATE_FLAG_DROP_ACTIVE);

  g_object_thaw_notify (G_OBJECT (self));
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

static void
bobgui_drop_target_do_drop (BobguiDropTarget *self)
{
  gboolean success;

  g_assert (self->dropping);
  g_assert (G_IS_VALUE (&self->value));

  g_signal_emit (self, signals[DROP], 0, &self->value, self->coords.x, self->coords.y, &success);

  if (self->drop)
    {
      GdkDragAction action;

      if (success)
        action = make_action_unique (self->actions & gdk_drop_get_actions (self->drop));
      else
        action = GDK_ACTION_NONE;

      gdk_drop_finish (self->drop, action);
    }

  self->dropping = FALSE;

  bobgui_drop_target_end_drop (self);
}

static void
bobgui_drop_target_load_done (GObject      *source,
                           GAsyncResult *res,
                           gpointer      data)
{
  BobguiDropTarget *self = data;
  const GValue *value;
  GError *error = NULL;

  value = gdk_drop_read_value_finish (GDK_DROP (source), res, &error);
  if (value == NULL)
    {
      /* If this happens, data/self is invalid */
      if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
        {
          g_clear_error (&error);
          return;
        }

      g_clear_object (&self->cancellable);
      /* XXX: Should this be a warning? */
      g_warning ("Failed to receive drop data: %s", error->message);
      g_clear_error (&error);
      bobgui_drop_target_end_drop (self);
      return;
    }

  g_clear_object (&self->cancellable);
  g_value_init (&self->value, G_VALUE_TYPE (value));
  g_value_copy (value, &self->value);
  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_VALUE]);

  if (self->dropping)
    bobgui_drop_target_do_drop (self);
}

static gboolean
bobgui_drop_target_load_local (BobguiDropTarget *self,
                            GType          type)
{
  GdkDrag *drag;

  drag = gdk_drop_get_drag (self->drop);
  if (drag == NULL)
    return FALSE;

  g_value_init (&self->value, type);
  if (gdk_content_provider_get_value (gdk_drag_get_content (drag),
                                      &self->value,
                                      NULL))
    {
      g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_VALUE]);
      return TRUE;
    }

  g_value_unset (&self->value);
  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_VALUE]);
  return FALSE;
}

static gboolean
bobgui_drop_target_load (BobguiDropTarget *self)
{
  GType type;

  g_assert (self->drop);

  if (G_IS_VALUE (&self->value))
    return TRUE;

  if (self->cancellable)
    return FALSE;

  type = gdk_content_formats_match_gtype (self->formats, gdk_drop_get_formats (self->drop));

  if (bobgui_drop_target_load_local (self, type))
    return TRUE;

  self->cancellable = g_cancellable_new ();

  gdk_drop_read_value_async (self->drop,
                             type,
                             G_PRIORITY_DEFAULT,
                             self->cancellable,
                             bobgui_drop_target_load_done,
                             g_object_ref (self));
  return FALSE;
}

static void
bobgui_drop_target_start_drop (BobguiDropTarget *self,
                            GdkDrop       *drop)
{
  g_object_freeze_notify (G_OBJECT (self));

  bobgui_drop_target_end_drop (self);

  self->drop = g_object_ref (drop);
  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_DROP]);
  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_CURRENT_DROP]);

  if (self->preload)
    bobgui_drop_target_load (self);

  bobgui_widget_set_state_flags (bobgui_event_controller_get_widget (BOBGUI_EVENT_CONTROLLER (self)),
                              BOBGUI_STATE_FLAG_DROP_ACTIVE,
                              FALSE);

  g_object_thaw_notify (G_OBJECT (self));
}

static gboolean
bobgui_drop_target_accept (BobguiDropTarget *self,
                        GdkDrop       *drop)
{
  if ((gdk_drop_get_actions (drop) & bobgui_drop_target_get_actions (self)) == GDK_ACTION_NONE)
    return FALSE;

  if (self->formats == NULL)
    return TRUE;

  return gdk_content_formats_match_gtype (self->formats, gdk_drop_get_formats (drop)) != G_TYPE_INVALID;
}

static GdkDragAction
bobgui_drop_target_enter (BobguiDropTarget  *self,
                       double          x,
                       double          y)
{
  return make_action_unique (self->actions & gdk_drop_get_actions (self->drop));
}

static GdkDragAction
bobgui_drop_target_motion (BobguiDropTarget  *self,
                        double          x,
                        double          y)
{
  return make_action_unique (self->actions & gdk_drop_get_actions (self->drop));
}

static gboolean
bobgui_drop_target_drop (BobguiDropTarget  *self,
                      const GValue   *value,
                      double          x,
                      double          y)
{
  return FALSE;
}

static gboolean
bobgui_drop_target_filter_event (BobguiEventController *controller,
                              GdkEvent           *event)
{
  switch ((int) gdk_event_get_event_type (event))
    {
    case GDK_DRAG_ENTER:
    case GDK_DRAG_LEAVE:
    case GDK_DRAG_MOTION:
    case GDK_DROP_START:
      return BOBGUI_EVENT_CONTROLLER_CLASS (bobgui_drop_target_parent_class)->filter_event (controller, event);

    default:;
    }

  return TRUE;
}

static gboolean
bobgui_drop_target_handle_event (BobguiEventController *controller,
                              GdkEvent           *event,
                              double              x,
                              double              y)
{
  BobguiDropTarget *self = BOBGUI_DROP_TARGET (controller);

  /* All drops have been rejected. New drops only arrive via crossing
   * events, so we can: */
  if (self->drop == NULL)
    return FALSE;

  switch ((int) gdk_event_get_event_type (event))
    {
    case GDK_DRAG_MOTION:
      {
        BobguiWidget *widget = bobgui_event_controller_get_widget (controller);
        GdkDragAction preferred;

        /* sanity check */
        g_return_val_if_fail (self->drop == gdk_dnd_event_get_drop (event), FALSE);

        graphene_point_init (&self->coords, x, y);
        g_signal_emit (self, signals[MOTION], 0, x, y, &preferred);
        if (!gdk_drag_action_is_unique (preferred))
          {
            g_critical ("Handler for BobguiDropTarget::motion on %s %p did not return a unique preferred action",
                        G_OBJECT_TYPE_NAME (widget), widget);
            preferred = make_action_unique (preferred);
          }
        if (preferred && self->drop &&
            bobgui_drop_status (self->drop, self->actions, preferred))
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
        /* sanity check */
        g_return_val_if_fail (self->drop == gdk_dnd_event_get_drop (event), FALSE);

        graphene_point_init (&self->coords, x, y);
        self->dropping = TRUE;
        if (bobgui_drop_target_load (self))
          bobgui_drop_target_do_drop (self);

        return TRUE;
      }

    default:
      return FALSE;
    }
}

static void
bobgui_drop_target_handle_crossing (BobguiEventController    *controller,
                                 const BobguiCrossingData *crossing,
                                 double                 x,
                                 double                 y)
{
  BobguiDropTarget *self = BOBGUI_DROP_TARGET (controller);
  BobguiWidget *widget = bobgui_event_controller_get_widget (controller);

  if (crossing->type != BOBGUI_CROSSING_DROP)
    return;

  /* sanity check */
  g_warn_if_fail (self->drop == NULL || self->drop == crossing->drop);

  if (crossing->direction == BOBGUI_CROSSING_IN)
    {
      gboolean accept = FALSE;
      GdkDragAction preferred;

      if (self->drop != NULL)
        return;

      /* if we were a target already but self->drop == NULL, the drop
       * was rejected already */
      if (crossing->old_descendent != NULL ||
          crossing->old_target == widget)
        return;

      g_signal_emit (self, signals[ACCEPT], 0, crossing->drop, &accept);
      if (!accept)
        return;

      graphene_point_init (&self->coords, x, y);
      bobgui_drop_target_start_drop (self, crossing->drop);

      /* start_drop ends w/ thaw_notify, where handler may reject, so recheck */
      if (self->drop != NULL)
        {
          self->entered = TRUE;
          g_signal_emit (self, signals[ENTER], 0, x, y, &preferred);
        }
      else
        preferred = 0;

      if (!gdk_drag_action_is_unique (preferred))
        {
          g_critical ("Handler for BobguiDropTarget::enter on %s %p did not return a unique preferred action",
                      G_OBJECT_TYPE_NAME (widget), widget);
          preferred = make_action_unique (preferred);
        }

      if (preferred && self->drop &&
          bobgui_drop_status (self->drop, self->actions, preferred))
        {
          bobgui_widget_set_state_flags (widget, BOBGUI_STATE_FLAG_DROP_ACTIVE, FALSE);
        }
      else
        {
          bobgui_widget_unset_state_flags (widget, BOBGUI_STATE_FLAG_DROP_ACTIVE);
        }
    }
  else
    {
      /*
       * @self is attached to the common ancestor of new_target and old_target.
       * I.e. not actually crossing out of the drop target's area, so there is
       * nothing to do.
       */
      if (crossing->new_descendent != NULL ||
          crossing->new_target == widget)
        return;

      if (self->entered)
        {
          self->entered = FALSE;
          g_signal_emit (self, signals[LEAVE], 0);
        }

      if (self->drop == NULL)
        return;

      if (!self->dropping)
        bobgui_drop_target_end_drop (self);

      bobgui_widget_unset_state_flags (widget, BOBGUI_STATE_FLAG_DROP_ACTIVE);
    }
}

static void
bobgui_drop_target_reset (BobguiEventController *controller)
{
  BobguiDropTarget *self = BOBGUI_DROP_TARGET (controller);

  if (self->entered)
    {
      self->entered = FALSE;
      g_signal_emit (self, signals[LEAVE], 0);
    }

  bobgui_drop_target_end_drop (self);
}

static void
bobgui_drop_target_finalize (GObject *object)
{
  BobguiDropTarget *self = BOBGUI_DROP_TARGET (object);

  g_clear_pointer (&self->formats, gdk_content_formats_unref);

  G_OBJECT_CLASS (bobgui_drop_target_parent_class)->finalize (object);
}

static void
bobgui_drop_target_set_property (GObject      *object,
                              guint         prop_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  BobguiDropTarget *self = BOBGUI_DROP_TARGET (object);

  switch (prop_id)
    {
    case PROP_ACTIONS:
      bobgui_drop_target_set_actions (self, g_value_get_flags (value));
      break;

    case PROP_FORMATS:
      self->formats = g_value_dup_boxed (value);
      if (self->formats == NULL)
        self->formats = gdk_content_formats_new (NULL, 0);
      break;

    case PROP_PRELOAD:
      bobgui_drop_target_set_preload (self, g_value_get_boolean (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
bobgui_drop_target_get_property (GObject    *object,
                              guint       prop_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
  BobguiDropTarget *self = BOBGUI_DROP_TARGET (object);

  switch (prop_id)
    {
    case PROP_ACTIONS:
      g_value_set_flags (value, self->actions);
      break;

    case PROP_DROP:
    case PROP_CURRENT_DROP:
      g_value_set_object (value, self->drop);
      break;

    case PROP_FORMATS:
      g_value_set_boxed (value, self->formats);
      break;

    case PROP_PRELOAD:
      g_value_set_boolean (value, self->preload);
      break;

    case PROP_VALUE:
      if (G_IS_VALUE (&self->value))
        g_value_set_boxed (value, &self->value);
      else
        g_value_set_boxed (value, NULL);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
bobgui_drop_target_class_init (BobguiDropTargetClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  BobguiEventControllerClass *controller_class = BOBGUI_EVENT_CONTROLLER_CLASS (class);

  object_class->finalize = bobgui_drop_target_finalize;
  object_class->set_property = bobgui_drop_target_set_property;
  object_class->get_property = bobgui_drop_target_get_property;

  controller_class->handle_event = bobgui_drop_target_handle_event;
  controller_class->filter_event = bobgui_drop_target_filter_event;
  controller_class->handle_crossing = bobgui_drop_target_handle_crossing;
  controller_class->reset = bobgui_drop_target_reset;

  class->accept = bobgui_drop_target_accept;
  class->enter = bobgui_drop_target_enter;
  class->motion = bobgui_drop_target_motion;
  class->drop = bobgui_drop_target_drop;

  /**
   * BobguiDropTarget:actions:
   *
   * The `GdkDragActions` that this drop target supports.
   */
  properties[PROP_ACTIONS] =
       g_param_spec_flags ("actions", NULL, NULL,
                           GDK_TYPE_DRAG_ACTION, GDK_ACTION_NONE,
                           G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiDropTarget:drop:
   *
   * The `GdkDrop` that is currently being performed.
   *
   * Deprecated: 4.4: Use [property@Bobgui.DropTarget:current-drop] instead
   */
  properties[PROP_DROP] =
       g_param_spec_object ("drop", NULL, NULL,
                            GDK_TYPE_DROP,
                            BOBGUI_PARAM_READABLE | G_PARAM_DEPRECATED);

  /**
   * BobguiDropTarget:current-drop:
   *
   * The `GdkDrop` that is currently being performed.
   *
   * Since: 4.4
   */
  properties[PROP_CURRENT_DROP] =
       g_param_spec_object ("current-drop", NULL, NULL,
                            GDK_TYPE_DROP,
                            BOBGUI_PARAM_READABLE);

  /**
   * BobguiDropTarget:formats:
   *
   * The `GdkContentFormats` that determine the supported data formats.
   */
  properties[PROP_FORMATS] =
       g_param_spec_boxed ("formats", NULL, NULL,
                           GDK_TYPE_CONTENT_FORMATS,
                           BOBGUI_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

  /**
   * BobguiDropTarget:preload:
   *
   * Whether the drop data should be preloaded when the pointer is only
   * hovering over the widget but has not been released.
   *
   * Setting this property allows finer grained reaction to an ongoing
   * drop at the cost of loading more data.
   *
   * The default value for this property is %FALSE to avoid downloading
   * huge amounts of data by accident.
   *
   * For example, if somebody drags a full document of gigabytes of text
   * from a text editor across a widget with a preloading drop target,
   * this data will be downloaded, even if the data is ultimately dropped
   * elsewhere.
   *
   * For a lot of data formats, the amount of data is very small (like
   * %GDK_TYPE_RGBA), so enabling this property does not hurt at all.
   * And for local-only Drag-and-Drop operations, no data transfer is done,
   * so enabling it there is free.
   */
  properties[PROP_PRELOAD] =
       g_param_spec_boolean ("preload", NULL, NULL,
                             FALSE,
                             G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiDropTarget:value:
   *
   * The value for this drop operation.
   *
   * This is %NULL if the data has not been loaded yet or no drop
   * operation is going on.
   *
   * Data may be available before the [signal@Bobgui.DropTarget::drop]
   * signal gets emitted - for example when the [property@Bobgui.DropTarget:preload]
   * property is set. You can use the ::notify signal to be notified
   * of available data.
   */
  properties[PROP_VALUE] =
       g_param_spec_boxed ("value", NULL, NULL,
                           G_TYPE_VALUE,
                           BOBGUI_PARAM_READABLE);

  g_object_class_install_properties (object_class, NUM_PROPERTIES, properties);

 /**
   * BobguiDropTarget::accept:
   * @self: the `BobguiDropTarget`
   * @drop: the `GdkDrop`
   *
   * Emitted on the drop site when a drop operation is about to begin.
   *
   * If the drop is not accepted, %FALSE will be returned and the drop target
   * will ignore the drop. If %TRUE is returned, the drop is accepted for now
   * but may be rejected later via a call to [method@Bobgui.DropTarget.reject]
   * or ultimately by returning %FALSE from a [signal@Bobgui.DropTarget::drop]
   * handler.
   *
   * The default handler for this signal decides whether to accept the drop
   * based on the formats provided by the @drop.
   *
   * If the decision whether the drop will be accepted or rejected depends
   * on the data, this function should return %TRUE, the
   * [property@Bobgui.DropTarget:preload] property should be set and the value
   * should be inspected via the ::notify:value signal, calling
   * [method@Bobgui.DropTarget.reject] if required.
   *
   * Returns: %TRUE if @drop is accepted
   */
  signals[ACCEPT] =
      g_signal_new (I_("accept"),
                    G_TYPE_FROM_CLASS (class),
                    G_SIGNAL_RUN_LAST,
                    G_STRUCT_OFFSET (BobguiDropTargetClass, accept),
                    g_signal_accumulator_first_wins, NULL,
                    _gdk_marshal_BOOLEAN__OBJECT,
                    G_TYPE_BOOLEAN, 1,
                    GDK_TYPE_DROP);
  g_signal_set_va_marshaller (signals[ACCEPT],
                              BOBGUI_TYPE_DROP_TARGET,
                              _gdk_marshal_BOOLEAN__OBJECTv);

  /**
   * BobguiDropTarget::enter:
   * @self: the `BobguiDropTarget`
   * @x: the x coordinate of the current pointer position
   * @y: the y coordinate of the current pointer position
   *
   * Emitted on the drop site when the pointer enters the widget.
   *
   * It can be used to set up custom highlighting.
   *
   * Returns: Preferred action for this drag operation or `GDK_ACTION_NONE` if
   *   dropping is not supported at the current @x,@y location.
   */
  signals[ENTER] =
      g_signal_new (I_("enter"),
                    G_TYPE_FROM_CLASS (class),
                    G_SIGNAL_RUN_LAST,
                    G_STRUCT_OFFSET (BobguiDropTargetClass, enter),
                    g_signal_accumulator_first_wins, NULL,
                    _bobgui_marshal_FLAGS__DOUBLE_DOUBLE,
                    GDK_TYPE_DRAG_ACTION, 2,
                    G_TYPE_DOUBLE, G_TYPE_DOUBLE);
  g_signal_set_va_marshaller (signals[ENTER],
                              BOBGUI_TYPE_DROP_TARGET,
                              _bobgui_marshal_FLAGS__DOUBLE_DOUBLEv);

  /**
   * BobguiDropTarget::motion:
   * @self: the `BobguiDropTarget`
   * @x: the x coordinate of the current pointer position
   * @y: the y coordinate of the current pointer position
   *
   * Emitted while the pointer is moving over the drop target.
   *
   * Returns: Preferred action for this drag operation or `GDK_ACTION_NONE` if
   *   dropping is not supported at the current @x,@y location.
   */
  signals[MOTION] =
      g_signal_new (I_("motion"),
                    G_TYPE_FROM_CLASS (class),
                    G_SIGNAL_RUN_LAST,
                    G_STRUCT_OFFSET (BobguiDropTargetClass, motion),
                    g_signal_accumulator_first_wins, NULL,
                    _bobgui_marshal_FLAGS__DOUBLE_DOUBLE,
                    GDK_TYPE_DRAG_ACTION, 2,
                    G_TYPE_DOUBLE, G_TYPE_DOUBLE);
  g_signal_set_va_marshaller (signals[MOTION],
                              BOBGUI_TYPE_DROP_TARGET,
                              _bobgui_marshal_FLAGS__DOUBLE_DOUBLEv);

  /**
   * BobguiDropTarget::leave:
   * @self: the `BobguiDropTarget`
   *
   * Emitted on the drop site when the pointer leaves the widget.
   *
   * Its main purpose it to undo things done in
   * [signal@Bobgui.DropTarget::enter].
   */
  signals[LEAVE] =
      g_signal_new (I_("leave"),
                    G_TYPE_FROM_CLASS (class),
                    G_SIGNAL_RUN_LAST,
                    G_STRUCT_OFFSET (BobguiDropTargetClass, leave),
                    NULL, NULL,
                    NULL,
                    G_TYPE_NONE, 0);

  /**
   * BobguiDropTarget::drop:
   * @self: the `BobguiDropTarget`
   * @value: the `GValue` being dropped
   * @x: the x coordinate of the current pointer position
   * @y: the y coordinate of the current pointer position
   *
   * Emitted on the drop site when the user drops the data onto the widget.
   *
   * The signal handler must determine whether the pointer position is in
   * a drop zone or not. If it is not in a drop zone, it returns %FALSE
   * and no further processing is necessary.
   *
   * Otherwise, the handler returns %TRUE. In this case, this handler will
   * accept the drop. The handler is responsible for using the given @value
   * and performing the drop operation.
   *
   * Returns: whether the drop was accepted at the given pointer position
   */
  signals[DROP] =
      g_signal_new (I_("drop"),
                    G_TYPE_FROM_CLASS (class),
                    G_SIGNAL_RUN_LAST,
                    0,
                    g_signal_accumulator_first_wins, NULL,
                    _bobgui_marshal_BOOLEAN__BOXED_DOUBLE_DOUBLE,
                    G_TYPE_BOOLEAN, 3,
                    G_TYPE_VALUE, G_TYPE_DOUBLE, G_TYPE_DOUBLE);
  g_signal_set_va_marshaller (signals[DROP],
                              BOBGUI_TYPE_DROP_TARGET,
                              _bobgui_marshal_BOOLEAN__BOXED_DOUBLE_DOUBLEv);
}

static void
bobgui_drop_target_init (BobguiDropTarget *self)
{
}

/**
 * bobgui_drop_target_new:
 * @type: The supported type or %G_TYPE_INVALID
 * @actions: the supported actions
 *
 * Creates a new `BobguiDropTarget` object.
 *
 * If the drop target should support more than 1 type, pass
 * %G_TYPE_INVALID for @type and then call
 * [method@Bobgui.DropTarget.set_gtypes].
 *
 * Returns: the new `BobguiDropTarget`
 */
BobguiDropTarget *
bobgui_drop_target_new (GType         type,
                     GdkDragAction actions)
{
  BobguiDropTarget *result;
  GdkContentFormats *formats;

  if (type != G_TYPE_INVALID)
    formats = gdk_content_formats_new_for_gtype (type);
  else
    formats = NULL;

  result = g_object_new (BOBGUI_TYPE_DROP_TARGET,
                         "formats", formats,
                         "actions", actions,
                         NULL);

  g_clear_pointer (&formats, gdk_content_formats_unref);

  return result;
}

/**
 * bobgui_drop_target_get_formats:
 * @self: a `BobguiDropTarget`
 *
 * Gets the data formats that this drop target accepts.
 *
 * If the result is %NULL, all formats are expected to be supported.
 *
 * Returns: (nullable) (transfer none): the supported data formats
 */
GdkContentFormats *
bobgui_drop_target_get_formats (BobguiDropTarget *self)
{
  g_return_val_if_fail (BOBGUI_IS_DROP_TARGET (self), NULL);

  return self->formats;
}

/**
 * bobgui_drop_target_set_gtypes:
 * @self: a `BobguiDropTarget`
 * @types: (nullable) (transfer none) (array length=n_types): all supported `GType`s
 *   that can be dropped on the target
 * @n_types: number of @types
 *
 * Sets the supported `GType`s for this drop target.
 */
void
bobgui_drop_target_set_gtypes (BobguiDropTarget *self,
                            GType         *types,
                            gsize          n_types)
{
  GdkContentFormatsBuilder *builder;
  gsize i;

  g_return_if_fail (BOBGUI_IS_DROP_TARGET (self));
  g_return_if_fail (n_types == 0 || types != NULL);

  gdk_content_formats_unref (self->formats);

  builder = gdk_content_formats_builder_new ();
  for (i = 0; i < n_types; i++)
    gdk_content_formats_builder_add_gtype (builder, types[i]);

  self->formats = gdk_content_formats_builder_free_to_formats (builder);

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_FORMATS]);
}

/**
 * bobgui_drop_target_get_gtypes:
 * @self: a `BobguiDropTarget`
 * @n_types: (out) (optional): the number of `GType`s contained in the
 *   return value
 *
 * Gets the list of supported `GType`s that can be dropped on the target.
 *
 * If no types have been set, `NULL` will be returned.
 *
 * Returns: (transfer none) (nullable) (array length=n_types):
 *   the `G_TYPE_INVALID`-terminated array of types included in
 *   formats
 */
const GType *
bobgui_drop_target_get_gtypes (BobguiDropTarget *self,
                            gsize         *n_types)
{
  g_return_val_if_fail (BOBGUI_IS_DROP_TARGET (self), NULL);

  return gdk_content_formats_get_gtypes (self->formats, n_types);
}

/**
 * bobgui_drop_target_set_actions:
 * @self: a `BobguiDropTarget`
 * @actions: the supported actions
 *
 * Sets the actions that this drop target supports.
 */
void
bobgui_drop_target_set_actions (BobguiDropTarget *self,
                             GdkDragAction  actions)
{
  g_return_if_fail (BOBGUI_IS_DROP_TARGET (self));

  if (self->actions == actions)
    return;

  self->actions = actions;

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_ACTIONS]);
}

/**
 * bobgui_drop_target_get_actions:
 * @self: a `BobguiDropTarget`
 *
 * Gets the actions that this drop target supports.
 *
 * Returns: the actions that this drop target supports
 */
GdkDragAction
bobgui_drop_target_get_actions (BobguiDropTarget *self)
{
  g_return_val_if_fail (BOBGUI_IS_DROP_TARGET (self), GDK_ACTION_NONE);

  return self->actions;
}

/**
 * bobgui_drop_target_set_preload:
 * @self: a `BobguiDropTarget`
 * @preload: %TRUE to preload drop data
 *
 * Sets whether data should be preloaded on hover.
 */
void
bobgui_drop_target_set_preload (BobguiDropTarget *self,
                             gboolean       preload)
{
  g_return_if_fail (BOBGUI_IS_DROP_TARGET (self));

  if (self->preload == preload)
    return;

  self->preload = preload;

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_PRELOAD]);
}

/**
 * bobgui_drop_target_get_preload:
 * @self: a `BobguiDropTarget`
 *
 * Gets whether data should be preloaded on hover.
 *
 * Returns: %TRUE if drop data should be preloaded
 */
gboolean
bobgui_drop_target_get_preload (BobguiDropTarget *self)
{
  g_return_val_if_fail (BOBGUI_IS_DROP_TARGET (self), 0);

  return self->preload;
}

/**
 * bobgui_drop_target_get_drop:
 * @self: a `BobguiDropTarget`
 *
 * Gets the currently handled drop operation.
 *
 * If no drop operation is going on, %NULL is returned.
 *
 * Returns: (nullable) (transfer none): The current drop
 *
 * Deprecated: 4.4: Use [method@Bobgui.DropTarget.get_current_drop] instead
 */
GdkDrop *
bobgui_drop_target_get_drop (BobguiDropTarget *self)
{
  g_return_val_if_fail (BOBGUI_IS_DROP_TARGET (self), NULL);

  return self->drop;
}

/**
 * bobgui_drop_target_get_current_drop:
 * @self: a `BobguiDropTarget`
 *
 * Gets the currently handled drop operation.
 *
 * If no drop operation is going on, %NULL is returned.
 *
 * Returns: (nullable) (transfer none): The current drop
 *
 * Since: 4.4
 */
GdkDrop *
bobgui_drop_target_get_current_drop (BobguiDropTarget *self)
{
  g_return_val_if_fail (BOBGUI_IS_DROP_TARGET (self), NULL);

  return self->drop;
}

/**
 * bobgui_drop_target_get_value:
 * @self: a `BobguiDropTarget`
 *
 * Gets the current drop data, as a `GValue`.
 *
 * Returns: (nullable) (transfer none): The current drop data
 */
const GValue *
bobgui_drop_target_get_value (BobguiDropTarget *self)
{
  g_return_val_if_fail (BOBGUI_IS_DROP_TARGET (self), NULL);

  if (!G_IS_VALUE (&self->value))
    return NULL;

  return &self->value;
}

/**
 * bobgui_drop_target_reject:
 * @self: a `BobguiDropTarget`
 *
 * Rejects the ongoing drop operation.
 *
 * If no drop operation is ongoing, i.e when [property@Bobgui.DropTarget:current-drop]
 * is %NULL, this function does nothing.
 *
 * This function should be used when delaying the decision
 * on whether to accept a drag or not until after reading
 * the data.
 */
void
bobgui_drop_target_reject (BobguiDropTarget *self)
{
  g_return_if_fail (BOBGUI_IS_DROP_TARGET (self));

  if (self->drop == NULL)
    return;

  bobgui_drop_target_end_drop (self);
}
