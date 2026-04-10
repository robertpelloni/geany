/*
 * Copyright © 2012 Canonical Limited
 *
 * This library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * licence or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors: Ryan Lortie <desrt@desrt.ca>
 */

#include "bobguiactionhelperprivate.h"
#include "bobguiactionobservableprivate.h"

#include "bobguiwidgetprivate.h"
#include "bobguidebug.h"
#include "bobguitypebuiltins.h"
#include "bobguimodelbuttonprivate.h"
#include "bobguiprivate.h"

#include <string.h>

typedef struct
{
  GActionGroup *group;

  GHashTable *watchers;
} BobguiActionHelperGroup;

static void             bobgui_action_helper_action_added                  (BobguiActionHelper    *helper,
                                                                         gboolean            enabled,
                                                                         const GVariantType *parameter_type,
                                                                         GVariant           *state,
                                                                         gboolean            should_emit_signals);

static void             bobgui_action_helper_action_removed                (BobguiActionHelper    *helper,
                                                                         gboolean            should_emit_signals);

static void             bobgui_action_helper_action_enabled_changed        (BobguiActionHelper    *helper,
                                                                         gboolean            enabled);

static void             bobgui_action_helper_action_state_changed          (BobguiActionHelper    *helper,
                                                                         GVariant           *new_state);

typedef GObjectClass BobguiActionHelperClass;

struct _BobguiActionHelper
{
  GObject parent_instance;

  BobguiWidget *widget;

  BobguiActionHelperGroup *group;

  BobguiActionMuxer *action_context;
  char *action_name;

  GVariant *target;

  gboolean can_activate;
  gboolean enabled;
  gboolean active;

  BobguiButtonRole role;

  int reporting;
};

enum
{
  PROP_0,
  PROP_ENABLED,
  PROP_ACTIVE,
  PROP_ROLE,
  N_PROPS
};

static GParamSpec *bobgui_action_helper_pspecs[N_PROPS];

static void bobgui_action_helper_observer_iface_init (BobguiActionObserverInterface *iface);

G_DEFINE_TYPE_WITH_CODE (BobguiActionHelper, bobgui_action_helper, G_TYPE_OBJECT,
  G_IMPLEMENT_INTERFACE (BOBGUI_TYPE_ACTION_OBSERVER, bobgui_action_helper_observer_iface_init))

static void
bobgui_action_helper_report_change (BobguiActionHelper *helper,
                                 guint            prop_id)
{
  helper->reporting++;

  switch (prop_id)
    {
    case PROP_ENABLED:
      bobgui_widget_set_sensitive (BOBGUI_WIDGET (helper->widget), helper->enabled);
      break;

    case PROP_ACTIVE:
      {
        GParamSpec *pspec;

        pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (helper->widget), "active");

        if (pspec && G_PARAM_SPEC_VALUE_TYPE (pspec) == G_TYPE_BOOLEAN)
          g_object_set (G_OBJECT (helper->widget), "active", helper->active, NULL);
      }
      break;

    case PROP_ROLE:
      {
        GParamSpec *pspec;

        pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (helper->widget), "role");

        if (pspec && G_PARAM_SPEC_VALUE_TYPE (pspec) == BOBGUI_TYPE_BUTTON_ROLE)
          g_object_set (G_OBJECT (helper->widget), "role", helper->role, NULL);
      }
      break;

    default:
      g_assert_not_reached ();
    }

  g_object_notify_by_pspec (G_OBJECT (helper), bobgui_action_helper_pspecs[prop_id]);
  helper->reporting--;
}

static void
bobgui_action_helper_action_added (BobguiActionHelper    *helper,
                                gboolean            enabled,
                                const GVariantType *parameter_type,
                                GVariant           *state,
                                gboolean            should_emit_signals)
{
  BOBGUI_DEBUG (ACTIONS, "%s: action %s added", "actionhelper", helper->action_name);

  /* we can only activate if we have the correct type of parameter */
  helper->can_activate = (helper->target == NULL && parameter_type == NULL) ||
                          (helper->target != NULL && parameter_type != NULL &&
                          g_variant_is_of_type (helper->target, parameter_type));

  if (!helper->can_activate)
    {
      g_warning ("%s: action %s can't be activated due to parameter type mismatch "
                 "(parameter type %s, target type %s)",
                 "actionhelper",
                 helper->action_name,
                 parameter_type ? g_variant_type_peek_string (parameter_type) : "NULL",
                 helper->target ? g_variant_get_type_string (helper->target) : "NULL");
      return;
    }

  BOBGUI_DEBUG (ACTIONS, "%s: %s can be activated", "actionhelper", helper->action_name);

  helper->enabled = enabled;

  BOBGUI_DEBUG (ACTIONS, "%s: action %s is %s", "actionhelper", helper->action_name, enabled ? "enabled" : "disabled");

  if (helper->target != NULL && state != NULL)
    {
      helper->active = g_variant_equal (state, helper->target);
      helper->role = BOBGUI_BUTTON_ROLE_RADIO;
    }
  else if (state != NULL && g_variant_is_of_type (state, G_VARIANT_TYPE_BOOLEAN))
    {
      helper->active = g_variant_get_boolean (state);
      helper->role = BOBGUI_BUTTON_ROLE_CHECK;
    }
  else
    {
      helper->role = BOBGUI_BUTTON_ROLE_NORMAL;
    }

  if (should_emit_signals)
    {
      if (helper->enabled)
        bobgui_action_helper_report_change (helper, PROP_ENABLED);

      if (helper->active)
        bobgui_action_helper_report_change (helper, PROP_ACTIVE);

      bobgui_action_helper_report_change (helper, PROP_ROLE);
    }
}

static void
bobgui_action_helper_action_removed (BobguiActionHelper *helper,
                                  gboolean         should_emit_signals)
{
  BOBGUI_DEBUG (ACTIONS, "%s: action %s was removed", "actionhelper", helper->action_name);

  if (!helper->can_activate)
    return;

  helper->can_activate = FALSE;

  if (helper->enabled)
    {
      helper->enabled = FALSE;

      if (should_emit_signals)
        bobgui_action_helper_report_change (helper, PROP_ENABLED);
    }

  if (helper->active)
    {
      helper->active = FALSE;

      if (should_emit_signals)
        bobgui_action_helper_report_change (helper, PROP_ACTIVE);
    }
}

static void
bobgui_action_helper_action_enabled_changed (BobguiActionHelper *helper,
                                          gboolean         enabled)
{
  BOBGUI_DEBUG (ACTIONS, "%s: action %s: enabled changed to %d", "actionhelper",  helper->action_name, enabled);

  if (!helper->can_activate)
    return;

  if (helper->enabled == enabled)
    return;

  helper->enabled = enabled;
  bobgui_action_helper_report_change (helper, PROP_ENABLED);
}

static void
bobgui_action_helper_action_state_changed (BobguiActionHelper *helper,
                                        GVariant        *new_state)
{
  gboolean was_active;

  BOBGUI_DEBUG (ACTIONS, "%s: %s state changed", "actionhelper", helper->action_name);

  if (!helper->can_activate)
    return;

  was_active = helper->active;

  if (helper->target)
    helper->active = g_variant_equal (new_state, helper->target);

  else if (g_variant_is_of_type (new_state, G_VARIANT_TYPE_BOOLEAN))
    helper->active = g_variant_get_boolean (new_state);

  else
    helper->active = FALSE;

  if (helper->active != was_active)
    bobgui_action_helper_report_change (helper, PROP_ACTIVE);
}

static void
bobgui_action_helper_get_property (GObject *object, guint prop_id,
                                GValue *value, GParamSpec *pspec)
{
  BobguiActionHelper *helper = BOBGUI_ACTION_HELPER (object);

  switch (prop_id)
    {
    case PROP_ENABLED:
      g_value_set_boolean (value, helper->enabled);
      break;

    case PROP_ACTIVE:
      g_value_set_boolean (value, helper->active);
      break;

    case PROP_ROLE:
      g_value_set_enum (value, helper->role);
      break;

    default:
      g_assert_not_reached ();
    }
}

static void
bobgui_action_helper_finalize (GObject *object)
{
  BobguiActionHelper *helper = BOBGUI_ACTION_HELPER (object);

  g_free (helper->action_name);

  if (helper->target)
    g_variant_unref (helper->target);

  G_OBJECT_CLASS (bobgui_action_helper_parent_class)
    ->finalize (object);
}

static void
bobgui_action_helper_observer_action_added (BobguiActionObserver   *observer,
                                         BobguiActionObservable *observable,
                                         const char          *action_name,
                                         const GVariantType  *parameter_type,
                                         gboolean             enabled,
                                         GVariant            *state)
{
  bobgui_action_helper_action_added (BOBGUI_ACTION_HELPER (observer), enabled, parameter_type, state, TRUE);
}

static void
bobgui_action_helper_observer_action_enabled_changed (BobguiActionObserver   *observer,
                                                   BobguiActionObservable *observable,
                                                   const char          *action_name,
                                                   gboolean             enabled)
{
  bobgui_action_helper_action_enabled_changed (BOBGUI_ACTION_HELPER (observer), enabled);
}

static void
bobgui_action_helper_observer_action_state_changed (BobguiActionObserver   *observer,
                                                 BobguiActionObservable *observable,
                                                 const char          *action_name,
                                                 GVariant            *state)
{
  bobgui_action_helper_action_state_changed (BOBGUI_ACTION_HELPER (observer), state);
}

static void
bobgui_action_helper_observer_action_removed (BobguiActionObserver   *observer,
                                           BobguiActionObservable *observable,
                                           const char          *action_name)
{
  bobgui_action_helper_action_removed (BOBGUI_ACTION_HELPER (observer), TRUE);
}

static void
bobgui_action_helper_init (BobguiActionHelper *helper)
{
}

static void
bobgui_action_helper_class_init (BobguiActionHelperClass *class)
{
  class->get_property = bobgui_action_helper_get_property;
  class->finalize = bobgui_action_helper_finalize;

  bobgui_action_helper_pspecs[PROP_ENABLED] = g_param_spec_boolean ("enabled", NULL, NULL, FALSE,
                                                                 G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);
  bobgui_action_helper_pspecs[PROP_ACTIVE] = g_param_spec_boolean ("active", NULL, NULL, FALSE,
                                                                G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);
  bobgui_action_helper_pspecs[PROP_ROLE] = g_param_spec_enum ("role", NULL, NULL,
                                                           BOBGUI_TYPE_BUTTON_ROLE,
                                                           BOBGUI_BUTTON_ROLE_NORMAL,
                                                           G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_properties (class, N_PROPS, bobgui_action_helper_pspecs);
}

static void
bobgui_action_helper_observer_iface_init (BobguiActionObserverInterface *iface)
{
  iface->action_added = bobgui_action_helper_observer_action_added;
  iface->action_enabled_changed = bobgui_action_helper_observer_action_enabled_changed;
  iface->action_state_changed = bobgui_action_helper_observer_action_state_changed;
  iface->action_removed = bobgui_action_helper_observer_action_removed;
}

/*< private >
 * bobgui_action_helper_new:
 * @widget: a `BobguiWidget` implementing `BobguiActionable`
 *
 * Creates a helper to track the state of a named action.  This will
 * usually be used by widgets implementing `BobguiActionable`.
 *
 * This helper class is usually used by @widget itself.  In order to
 * avoid reference cycles, the helper does not hold a reference on
 * @widget, but will assume that it continues to exist for the duration
 * of the life of the helper.  If you are using the helper from outside
 * of the widget, you should take a ref on @widget for each ref you hold
 * on the helper.
 *
 * Returns: a new `BobguiActionHelper`
 */
BobguiActionHelper *
bobgui_action_helper_new (BobguiActionable *widget)
{
  BobguiActionHelper *helper;
  GParamSpec *pspec;

  g_return_val_if_fail (BOBGUI_IS_ACTIONABLE (widget), NULL);
  helper = g_object_new (BOBGUI_TYPE_ACTION_HELPER, NULL);

  helper->widget = BOBGUI_WIDGET (widget);
  helper->enabled = bobgui_widget_get_sensitive (BOBGUI_WIDGET (helper->widget));

  pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (helper->widget), "active");
  if (pspec && G_PARAM_SPEC_VALUE_TYPE (pspec) == G_TYPE_BOOLEAN)
    g_object_get (G_OBJECT (helper->widget), "active", &helper->active, NULL);

  helper->action_context = _bobgui_widget_get_action_muxer (BOBGUI_WIDGET (widget), TRUE);

  return helper;
}

void
bobgui_action_helper_set_action_name (BobguiActionHelper *helper,
                                   const char      *action_name)
{
  gboolean was_enabled, was_active;
  const GVariantType *parameter_type;
  gboolean enabled;
  GVariant *state;

  if (g_strcmp0 (action_name, helper->action_name) == 0)
    return;

  if (BOBGUI_DEBUG_CHECK (ACTIONS))
    {
      if (action_name == NULL || !strchr (action_name, '.'))
        gdk_debug_message ("%s: action name %s doesn't look like 'app.' or 'win.'; "
                           "it is unlikely to work",
                           "actionhelper", action_name);
    }

  /* Start by recording the current state of our properties so we know
   * what notify signals we will need to send.
   */
  was_enabled = helper->enabled;
  was_active = helper->active;

  if (helper->action_name)
    {
      bobgui_action_helper_action_removed (helper, FALSE);
      bobgui_action_observable_unregister_observer (BOBGUI_ACTION_OBSERVABLE (helper->action_context),
                                                 helper->action_name,
                                                 BOBGUI_ACTION_OBSERVER (helper));
      g_clear_pointer (&helper->action_name, g_free);
    }

  if (action_name)
    {
      helper->action_name = g_strdup (action_name);

      bobgui_action_observable_register_observer (BOBGUI_ACTION_OBSERVABLE (helper->action_context),
                                               helper->action_name,
                                               BOBGUI_ACTION_OBSERVER (helper));

      if (bobgui_action_muxer_query_action (helper->action_context, helper->action_name,
                                         &enabled, &parameter_type,
                                         NULL, NULL, &state))
        {
          BOBGUI_DEBUG (ACTIONS, "%s: action %s existed from the start", "actionhelper", helper->action_name);

          bobgui_action_helper_action_added (helper, enabled, parameter_type, state, FALSE);

          if (state)
            g_variant_unref (state);
        }
      else
        {
          BOBGUI_DEBUG (ACTIONS, "%s: action %s missing from the start", "actionhelper", helper->action_name);
          helper->enabled = FALSE;
        }
    }

  /* Send the notifies for the properties that changed.
   *
   * When called during construction, widget is NULL.  We don't need to
   * report in that case.
   */
  if (helper->enabled != was_enabled)
    bobgui_action_helper_report_change (helper, PROP_ENABLED);

  if (helper->active != was_active)
    bobgui_action_helper_report_change (helper, PROP_ACTIVE);

  g_object_notify (G_OBJECT (helper->widget), "action-name");
}

/*< private >
 * bobgui_action_helper_set_action_target_value:
 * @helper: a `BobguiActionHelper`
 * @target_value: an action target, as per `BobguiActionable`
 *
 * This function consumes @action_target if it is floating.
 */
void
bobgui_action_helper_set_action_target_value (BobguiActionHelper *helper,
                                           GVariant        *target_value)
{
  gboolean was_enabled;
  gboolean was_active;

  if (target_value == helper->target)
    return;

  if (target_value && helper->target && g_variant_equal (target_value, helper->target))
    {
      g_variant_unref (g_variant_ref_sink (target_value));
      return;
    }

  if (helper->target)
    {
      g_variant_unref (helper->target);
      helper->target = NULL;
    }

  if (target_value)
    helper->target = g_variant_ref_sink (target_value);

  /* The action_name has not yet been set.  Don't do anything yet. */
  if (helper->action_name == NULL)
    return;

  was_enabled = helper->enabled;
  was_active = helper->active;

  /* If we are attached to an action group then it is possible that this
   * change of the target value could impact our properties (including
   * changes to 'can_activate' and therefore 'enabled', due to resolving
   * a parameter type mismatch).
   *
   * Start over again by pretending the action gets re-added.
   */
  helper->can_activate = FALSE;
  helper->enabled = FALSE;
  helper->active = FALSE;

  if (helper->action_context)
    {
      const GVariantType *parameter_type;
      gboolean enabled;
      GVariant *state;

      if (bobgui_action_muxer_query_action (helper->action_context,
                                         helper->action_name, &enabled, &parameter_type,
                                         NULL, NULL, &state))
        {
          bobgui_action_helper_action_added (helper, enabled, parameter_type, state, FALSE);

          if (state)
            g_variant_unref (state);
        }
    }

  if (helper->enabled != was_enabled)
    bobgui_action_helper_report_change (helper, PROP_ENABLED);

  if (helper->active != was_active)
    bobgui_action_helper_report_change (helper, PROP_ACTIVE);

  g_object_notify (G_OBJECT (helper->widget), "action-target");
}

const char *
bobgui_action_helper_get_action_name (BobguiActionHelper *helper)
{
  if (helper == NULL)
    return NULL;

  return helper->action_name;
}

GVariant *
bobgui_action_helper_get_action_target_value (BobguiActionHelper *helper)
{
  if (helper == NULL)
    return NULL;

  return helper->target;
}

gboolean
bobgui_action_helper_get_enabled (BobguiActionHelper *helper)
{
  g_return_val_if_fail (BOBGUI_IS_ACTION_HELPER (helper), FALSE);

  return helper->enabled;
}

gboolean
bobgui_action_helper_get_active (BobguiActionHelper *helper)
{
  g_return_val_if_fail (BOBGUI_IS_ACTION_HELPER (helper), FALSE);

  return helper->active;
}

void
bobgui_action_helper_activate (BobguiActionHelper *helper)
{
  g_return_if_fail (BOBGUI_IS_ACTION_HELPER (helper));

  if (!helper->can_activate || helper->reporting)
    return;

  bobgui_action_muxer_activate_action (helper->action_context,
                                    helper->action_name,
                                    helper->target);
}

BobguiButtonRole
bobgui_action_helper_get_role (BobguiActionHelper *helper)
{
  g_return_val_if_fail (BOBGUI_IS_ACTION_HELPER (helper), BOBGUI_BUTTON_ROLE_NORMAL);

  return helper->role;
}

