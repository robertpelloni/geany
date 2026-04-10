/* bobguiaccessible.c: Accessible interface
 *
 * Copyright 2020  GNOME Foundation
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
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
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

/**
 * BobguiAccessible:
 *
 * An interface for describing UI elements for Assistive Technologies.
 *
 * Every accessible implementation has:
 *
 *  - a “role”, represented by a value of the [enum@Bobgui.AccessibleRole] enumeration
 *  - “attributes”, represented by a set of [enum@Bobgui.AccessibleState],
 *    [enum@Bobgui.AccessibleProperty] and [enum@Bobgui.AccessibleRelation] values
 *
 * The role cannot be changed after instantiating a `BobguiAccessible`
 * implementation.
 *
 * The attributes are updated every time a UI element's state changes in
 * a way that should be reflected by assistive technologies. For instance,
 * if a `BobguiWidget` visibility changes, the %BOBGUI_ACCESSIBLE_STATE_HIDDEN
 * state will also change to reflect the [property@Bobgui.Widget:visible] property.
 *
 * Every accessible implementation is part of a tree of accessible objects.
 * Normally, this tree corresponds to the widget tree, but can be customized
 * by reimplementing the [vfunc@Bobgui.Accessible.get_accessible_parent],
 * [vfunc@Bobgui.Accessible.get_first_accessible_child] and
 * [vfunc@Bobgui.Accessible.get_next_accessible_sibling] virtual functions.
 *
 * Note that you can not create a top-level accessible object as of now,
 * which means that you must always have a parent accessible object.
 *
 * Also note that when an accessible object does not correspond to a widget,
 * and it has children, whose implementation you don't control,
 * it is necessary to ensure the correct shape of the a11y tree
 * by calling [method@Bobgui.Accessible.set_accessible_parent] and
 * updating the sibling by [method@Bobgui.Accessible.update_next_accessible_sibling].
 */

#include "config.h"

#include "bobguiaccessibleprivate.h"

#include "bobguiatcontextprivate.h"
#include "bobguieditable.h"
#include "bobguienums.h"
#include "bobguitext.h"
#include "bobguitypebuiltins.h"
#include "bobguiwidget.h"

#include <glib/gi18n-lib.h>

#include <stdarg.h>

G_DEFINE_INTERFACE (BobguiAccessible, bobgui_accessible, G_TYPE_OBJECT)

static char *
bobgui_accessible_default_get_accessible_id (BobguiAccessible *self)
{
  return NULL;
}

static void
bobgui_accessible_default_init (BobguiAccessibleInterface *iface)
{
  /**
   * BobguiAccessible:accessible-role:
   *
   * The accessible role of the given `BobguiAccessible` implementation.
   *
   * The accessible role cannot be changed once set.
   */
  GParamSpec *pspec =
    g_param_spec_enum ("accessible-role", NULL, NULL,
                       BOBGUI_TYPE_ACCESSIBLE_ROLE,
                       BOBGUI_ACCESSIBLE_ROLE_NONE,
                       G_PARAM_READWRITE |
                       G_PARAM_STATIC_STRINGS);

  g_object_interface_install_property (iface, pspec);

  iface->get_accessible_id = bobgui_accessible_default_get_accessible_id;
}

/**
 * bobgui_accessible_get_at_context:
 * @self: an accessible object
 *
 * Retrieves the implementation for the given accessible object.
 *
 * Returns: (transfer full): the accessible implementation object
 *
 * Since: 4.10
 */
BobguiATContext *
bobgui_accessible_get_at_context (BobguiAccessible *self)
{
  g_return_val_if_fail (BOBGUI_IS_ACCESSIBLE (self), NULL);

  return BOBGUI_ACCESSIBLE_GET_IFACE (self)->get_at_context (self);
}

/**
 * bobgui_accessible_get_accessible_parent:
 * @self: an accessible object
 *
 * Retrieves the accessible parent for an accessible object.
 *
 * This function returns `NULL` for top level widgets.
 *
 * Returns: (transfer full) (nullable): the accessible parent
 *
 * Since: 4.10
 */
BobguiAccessible *
bobgui_accessible_get_accessible_parent (BobguiAccessible *self)
{
  g_return_val_if_fail (BOBGUI_IS_ACCESSIBLE (self), NULL);

  BobguiATContext *context;
  BobguiAccessible *parent = NULL;

  context = bobgui_accessible_get_at_context (self);
  if (context != NULL)
    {
      parent = bobgui_at_context_get_accessible_parent (context);
      g_object_unref (context);
    }

  if (parent != NULL)
    return g_object_ref (parent);
  else
    return BOBGUI_ACCESSIBLE_GET_IFACE (self)->get_accessible_parent (self);
}

/**
 * bobgui_accessible_set_accessible_parent:
 * @self: an accessible object
 * @parent: (nullable): the parent accessible object
 * @next_sibling: (nullable): the sibling accessible object
 *
 * Sets the parent and sibling of an accessible object.
 *
 * This function is meant to be used by accessible implementations that are
 * not part of the widget hierarchy, and but act as a logical bridge between
 * widgets. For instance, if a widget creates an object that holds metadata
 * for each child, and you want that object to implement the `BobguiAccessible`
 * interface, you will use this function to ensure that the parent of each
 * child widget is the metadata object, and the parent of each metadata
 * object is the container widget.
 *
 * Since: 4.10
 */
void
bobgui_accessible_set_accessible_parent (BobguiAccessible *self,
                                      BobguiAccessible *parent,
                                      BobguiAccessible *next_sibling)
{
  g_return_if_fail (BOBGUI_IS_ACCESSIBLE (self));
  g_return_if_fail (parent == NULL || BOBGUI_IS_ACCESSIBLE (parent));
  g_return_if_fail (next_sibling == NULL || BOBGUI_IS_ACCESSIBLE (next_sibling));

  BobguiATContext *context;

  context = bobgui_accessible_get_at_context (self);
  if (context != NULL)
    {
      bobgui_at_context_set_accessible_parent (context, parent);
      bobgui_at_context_set_next_accessible_sibling (context, next_sibling);
      g_object_unref (context);
    }
}

/**
 * bobgui_accessible_update_next_accessible_sibling:
 * @self: an accessible object
 * @new_sibling: (nullable): the new next accessible sibling to set
 *
 * Updates the next accessible sibling.
 *
 * That might be useful when a new child of a custom accessible
 * is created, and it needs to be linked to a previous child.
 *
 * Since: 4.10
 */
void
bobgui_accessible_update_next_accessible_sibling (BobguiAccessible *self,
                                               BobguiAccessible *new_sibling)
{
  BobguiATContext *context;
  BobguiAccessible *parent;

  g_return_if_fail (BOBGUI_IS_ACCESSIBLE (self));

  context = bobgui_accessible_get_at_context (self);
  if (context == NULL)
    return;

  parent = bobgui_at_context_get_accessible_parent (context);
  if (parent == NULL)
    {
      g_object_unref (context);
      g_critical ("Failed to update next accessible sibling: no parent accessible set for this accessible");
      return;
    }

  bobgui_at_context_set_next_accessible_sibling (context, new_sibling);

  g_object_unref (parent);
  g_object_unref (context);
}

/**
 * bobgui_accessible_get_first_accessible_child:
 * @self: an accessible object
 *
 * Retrieves the first accessible child of an accessible object.
 *
 * Returns: (transfer full) (nullable): the first accessible child
 *
 * since: 4.10
 */
BobguiAccessible *
bobgui_accessible_get_first_accessible_child (BobguiAccessible *self)
{
  g_return_val_if_fail (BOBGUI_IS_ACCESSIBLE (self), NULL);

  return BOBGUI_ACCESSIBLE_GET_IFACE (self)->get_first_accessible_child (self);
}

/**
 * bobgui_accessible_get_next_accessible_sibling:
 * @self: an accessible object
 *
 * Retrieves the next accessible sibling of an accessible object
 *
 * Returns: (transfer full) (nullable): the next accessible sibling
 *
 * since: 4.10
 */
BobguiAccessible *
bobgui_accessible_get_next_accessible_sibling (BobguiAccessible *self)
{
  g_return_val_if_fail (BOBGUI_IS_ACCESSIBLE (self), NULL);

  BobguiATContext *context;
  BobguiAccessible *sibling = NULL;

  context = bobgui_accessible_get_at_context (self);
  if (context != NULL && bobgui_at_context_get_accessible_parent (context) != NULL)
    {
      sibling = bobgui_at_context_get_next_accessible_sibling (context);
      if (sibling != NULL)
        sibling = g_object_ref (sibling);
    }
  else
    sibling = BOBGUI_ACCESSIBLE_GET_IFACE (self)->get_next_accessible_sibling (self);

  g_clear_object (&context);

  return sibling;
}

/**
 * bobgui_accessible_get_accessible_role:
 * @self: an accessible object
 *
 * Retrieves the accessible role of an accessible object.
 *
 * Returns: the accessible role
 */
BobguiAccessibleRole
bobgui_accessible_get_accessible_role (BobguiAccessible *self)
{
  BobguiAccessibleRole role = BOBGUI_ACCESSIBLE_ROLE_NONE;

  g_return_val_if_fail (BOBGUI_IS_ACCESSIBLE (self), BOBGUI_ACCESSIBLE_ROLE_NONE);

  BobguiATContext *context = bobgui_accessible_get_at_context (self);
  if (context != NULL)
    {
      if (bobgui_at_context_is_realized (context))
        role = bobgui_at_context_get_accessible_role (context);

      g_object_unref (context);

      if (role != BOBGUI_ACCESSIBLE_ROLE_NONE)
        return role;
    }

  g_object_get (G_OBJECT (self), "accessible-role", &role, NULL);

  return role;
}

/**
 * bobgui_accessible_update_state:
 * @self: an accessible object
 * @first_state: the first accessible state
 * @...: a list of state and value pairs, terminated by -1
 *
 * Updates a list of accessible states.
 *
 * See the [enum@Bobgui.AccessibleState] documentation for the
 * value types of accessible states.
 *
 * This function should be called by `BobguiWidget` types whenever
 * an accessible state change must be communicated to assistive
 * technologies.
 *
 * Example:
 *
 * ```c
 * value = BOBGUI_ACCESSIBLE_TRISTATE_MIXED;
 * bobgui_accessible_update_state (BOBGUI_ACCESSIBLE (check_button),
 *                              BOBGUI_ACCESSIBLE_STATE_CHECKED, value,
 *                              -1);
 * ```
 */
void
bobgui_accessible_update_state (BobguiAccessible      *self,
                             BobguiAccessibleState  first_state,
                             ...)
{
  int state;
  BobguiATContext *context;
  va_list args;

  g_return_if_fail (BOBGUI_IS_ACCESSIBLE (self));

  context = bobgui_accessible_get_at_context (self);
  if (context == NULL)
    return;

  va_start (args, first_state);

  state = first_state;

  while (state != -1)
    {
      GError *error = NULL;
      BobguiAccessibleValue *value =
        bobgui_accessible_value_collect_for_state ((BobguiAccessibleState) state, &error, &args);

      if (error != NULL)
        {
          g_critical ("Unable to collect value for state “%s”: %s",
                      bobgui_accessible_state_get_attribute_name (state),
                      error->message);
          g_error_free (error);
          goto out;
        }

      bobgui_at_context_set_accessible_state (context, (BobguiAccessibleState) state, value);

      if (value != NULL)
        bobgui_accessible_value_unref (value);

      state = va_arg (args, int);
    }

  bobgui_at_context_update (context);

out:
  va_end (args);

  g_object_unref (context);
}

/**
 * bobgui_accessible_update_state_value: (rename-to bobgui_accessible_update_state)
 * @self: an accessible objedct
 * @n_states: the number of accessible states to set
 * @states: (array length=n_states): an array of accessible states
 * @values: (array length=n_states): an array of `GValues`, one for each state
 *
 * Updates an array of accessible states.
 *
 * This function should be called by `BobguiWidget` types whenever an accessible
 * state change must be communicated to assistive technologies.
 *
 * This function is meant to be used by language bindings.
 */
void
bobgui_accessible_update_state_value (BobguiAccessible      *self,
                                   int                 n_states,
                                   BobguiAccessibleState  states[],
                                   const GValue        values[])
{
  g_return_if_fail (BOBGUI_IS_ACCESSIBLE (self));
  g_return_if_fail (n_states > 0);

  BobguiATContext *context = bobgui_accessible_get_at_context (self);
  if (context == NULL)
    return;

  for (int i = 0; i < n_states; i++)
    {
      BobguiAccessibleState state = states[i];
      const GValue *value = &(values[i]);
      GError *error = NULL;
      BobguiAccessibleValue *real_value =
        bobgui_accessible_value_collect_for_state_value (state, value, &error);

      if (error != NULL)
        {
          g_critical ("Unable to collect the value for state “%s”: %s",
                      bobgui_accessible_state_get_attribute_name (state),
                      error->message);
          g_error_free (error);
          break;
        }

      bobgui_at_context_set_accessible_state (context, state, real_value);

      if (real_value != NULL)
        bobgui_accessible_value_unref (real_value);
    }

  bobgui_at_context_update (context);
  g_object_unref (context);
}

/**
 * bobgui_accessible_reset_state:
 * @self: an accessible object
 * @state: the accessible state
 *
 * Resets the accessible state to its default value.
 */
void
bobgui_accessible_reset_state (BobguiAccessible      *self,
                            BobguiAccessibleState  state)
{
  BobguiATContext *context;

  g_return_if_fail (BOBGUI_IS_ACCESSIBLE (self));

  context = bobgui_accessible_get_at_context (self);
  if (context == NULL)
    return;

  bobgui_at_context_set_accessible_state (context, state, NULL);
  bobgui_at_context_update (context);
  g_object_unref (context);
}

/**
 * bobgui_accessible_update_property:
 * @self: an accessible object
 * @first_property: the first accessible property
 * @...: a list of property and value pairs, terminated by -1
 *
 * Updates a list of accessible properties.
 *
 * See the [enum@Bobgui.AccessibleProperty] documentation for the
 * value types of accessible properties.
 *
 * This function should be called by `BobguiWidget` types whenever
 * an accessible property change must be communicated to assistive
 * technologies.
 *
 * Example:
 * ```c
 * value = bobgui_adjustment_get_value (adjustment);
 * bobgui_accessible_update_property (BOBGUI_ACCESSIBLE (spin_button),
                                   BOBGUI_ACCESSIBLE_PROPERTY_VALUE_NOW, value,
                                   -1);
 * ```
 */
void
bobgui_accessible_update_property (BobguiAccessible         *self,
                                BobguiAccessibleProperty  first_property,
                                ...)
{
  int property;
  BobguiATContext *context;
  va_list args;

  g_return_if_fail (BOBGUI_IS_ACCESSIBLE (self));

  context = bobgui_accessible_get_at_context (self);
  if (context == NULL)
    return;

  va_start (args, first_property);

  property = first_property;

  while (property != -1)
    {
      GError *error = NULL;
      BobguiAccessibleValue *value =
        bobgui_accessible_value_collect_for_property ((BobguiAccessibleProperty) property, &error, &args);

      if (error != NULL)
        {
          g_critical ("Unable to collect the value for property “%s”: %s",
                      bobgui_accessible_property_get_attribute_name (property),
                      error->message);
          g_error_free (error);
          goto out;
        }

      bobgui_at_context_set_accessible_property (context, (BobguiAccessibleProperty) property, value);

      if (value != NULL)
        bobgui_accessible_value_unref (value);

      property = va_arg (args, int);
    }

  bobgui_at_context_update (context);

out:
  va_end (args);

  g_object_unref (context);
}

/**
 * bobgui_accessible_update_property_value: (rename-to bobgui_accessible_update_property)
 * @self: an accessible object
 * @n_properties: the number of accessible properties to set
 * @properties: (array length=n_properties): an array of accessible properties
 * @values: (array length=n_properties): an array of `GValues`, one for each property
 *
 * Updates an array of accessible properties.
 *
 * This function should be called by `BobguiWidget` types whenever an accessible
 * property change must be communicated to assistive technologies.
 *
 * This function is meant to be used by language bindings.
 */
void
bobgui_accessible_update_property_value (BobguiAccessible         *self,
                                      int                    n_properties,
                                      BobguiAccessibleProperty  properties[],
                                      const GValue           values[])
{
  g_return_if_fail (BOBGUI_IS_ACCESSIBLE (self));
  g_return_if_fail (n_properties > 0);

  BobguiATContext *context = bobgui_accessible_get_at_context (self);
  if (context == NULL)
    return;

  for (int i = 0; i < n_properties; i++)
    {
      BobguiAccessibleProperty property = properties[i];
      const GValue *value = &(values[i]);
      GError *error = NULL;
      BobguiAccessibleValue *real_value =
        bobgui_accessible_value_collect_for_property_value (property, value, &error);

      if (error != NULL)
        {
          g_critical ("Unable to collect the value for property “%s”: %s",
                      bobgui_accessible_property_get_attribute_name (property),
                      error->message);
          g_error_free (error);
          break;
        }

      bobgui_at_context_set_accessible_property (context, property, real_value);

      if (real_value != NULL)
        bobgui_accessible_value_unref (real_value);
    }

  bobgui_at_context_update (context);
  g_object_unref (context);
}

/**
 * bobgui_accessible_reset_property:
 * @self: an accessible object
 * @property: the accessible property
 *
 * Resets the accessible property to its default value.
 */
void
bobgui_accessible_reset_property (BobguiAccessible         *self,
                               BobguiAccessibleProperty  property)
{
  BobguiATContext *context;

  g_return_if_fail (BOBGUI_IS_ACCESSIBLE (self));

  context = bobgui_accessible_get_at_context (self);
  if (context == NULL)
    return;

  bobgui_at_context_set_accessible_property (context, property, NULL);
  bobgui_at_context_update (context);
  g_object_unref (context);
}

static inline bool
relation_is_managed (const BobguiAccessibleRelation relation)
{
  static const BobguiAccessibleRelation managed_relations[] = {
    BOBGUI_ACCESSIBLE_RELATION_LABEL_FOR,
    BOBGUI_ACCESSIBLE_RELATION_CONTROLLED_BY,
    BOBGUI_ACCESSIBLE_RELATION_DESCRIPTION_FOR,
    BOBGUI_ACCESSIBLE_RELATION_DETAILS_FOR,
    BOBGUI_ACCESSIBLE_RELATION_ERROR_MESSAGE_FOR,
    BOBGUI_ACCESSIBLE_RELATION_FLOW_FROM,
  };

  for (unsigned i = 0; i < G_N_ELEMENTS (managed_relations); i++)
    {
      if (relation == managed_relations[i])
        return true;
    }

  return false;
}

/**
 * bobgui_accessible_update_relation:
 * @self: an accessible object
 * @first_relation: the first accessible relation
 * @...: a list of relation and value pairs, terminated by -1
 *
 * Updates a list of accessible relations.
 *
 * This function should be called by `BobguiWidget` types whenever an accessible
 * relation change must be communicated to assistive technologies.
 *
 * If the [enum@Bobgui.AccessibleRelation] requires a list of references,
 * you should pass each reference individually, followed by `NULL`, e.g.
 *
 * ```c
 * bobgui_accessible_update_relation (accessible,
 *                                 BOBGUI_ACCESSIBLE_RELATION_CONTROLS,
 *                                   ref1, NULL,
 *                                 BOBGUI_ACCESSIBLE_RELATION_LABELLED_BY,
 *                                   ref1, ref2, ref3, NULL,
 *                                 -1);
 * ```
 */
void
bobgui_accessible_update_relation (BobguiAccessible         *self,
                                BobguiAccessibleRelation  first_relation,
                                ...)
{
  int relation;
  BobguiATContext *context;
  va_list args;

  g_return_if_fail (BOBGUI_IS_ACCESSIBLE (self));

  context = bobgui_accessible_get_at_context (self);
  if (context == NULL)
    return;

  va_start (args, first_relation);

  relation = first_relation;

  while (relation != -1)
    {
      if (relation_is_managed (relation))
        {
          g_warning ("The relation “%s” is managed by BOBGUI and must not be set directly",
                      bobgui_accessible_relation_get_attribute_name (relation));
          continue;
        }
      GError *error = NULL;
      BobguiAccessibleValue *value =
        bobgui_accessible_value_collect_for_relation ((BobguiAccessibleRelation) relation, &error, &args);

      if (error != NULL)
        {
          g_critical ("Unable to collect the value for relation “%s”: %s",
                      bobgui_accessible_relation_get_attribute_name (relation),
                      error->message);
          g_error_free (error);
          goto out;
        }

      bobgui_at_context_set_accessible_relation (context, (BobguiAccessibleRelation) relation, value);

      if (value != NULL)
        bobgui_accessible_value_unref (value);

      relation = va_arg (args, int);
    }

  bobgui_at_context_update (context);

out:
  va_end (args);

  g_object_unref (context);
}

/**
 * bobgui_accessible_update_relation_value: (rename-to bobgui_accessible_update_relation)
 * @self: an accessible object
 * @n_relations: the number of accessible relations to set
 * @relations: (array length=n_relations): an array of accessible relations
 * @values: (array length=n_relations): an array of `GValues`, one for each relation
 *
 * Updates an array of accessible relations.
 *
 * This function should be called by `BobguiWidget` types whenever an accessible
 * relation change must be communicated to assistive technologies.
 *
 * This function is meant to be used by language bindings.
 */
void
bobgui_accessible_update_relation_value (BobguiAccessible         *self,
                                      int                    n_relations,
                                      BobguiAccessibleRelation  relations[],
                                      const GValue           values[])
{
  BobguiATContext *context;

  g_return_if_fail (BOBGUI_IS_ACCESSIBLE (self));
  g_return_if_fail (n_relations > 0);

  context = bobgui_accessible_get_at_context (self);
  if (context == NULL)
    return;

  for (int i = 0; i < n_relations; i++)
    {
      BobguiAccessibleRelation relation = relations[i];
      const GValue *value = &(values[i]);
      GError *error = NULL;
      if (relation_is_managed (relation))
        {
          g_warning ("The relation “%s” is managed by BOBGUI and must not be set directly",
                      bobgui_accessible_relation_get_attribute_name (relation));
          continue;
        }
      BobguiAccessibleValue *real_value =
        bobgui_accessible_value_collect_for_relation_value (relation, value, &error);

      if (error != NULL)
        {
          g_critical ("Unable to collect the value for relation “%s”: %s",
                      bobgui_accessible_relation_get_attribute_name (relation),
                      error->message);
          g_error_free (error);
          break;
        }

      bobgui_at_context_set_accessible_relation (context, relation, real_value);

      if (real_value != NULL)
        bobgui_accessible_value_unref (real_value);
    }

  bobgui_at_context_update (context);
  g_object_unref (context);
}

/**
 * bobgui_accessible_reset_relation:
 * @self: an accessible object
 * @relation: the accessible relation
 *
 * Resets the accessible relation to its default value.
 */
void
bobgui_accessible_reset_relation (BobguiAccessible         *self,
                               BobguiAccessibleRelation  relation)
{
  BobguiATContext *context;

  g_return_if_fail (BOBGUI_IS_ACCESSIBLE (self));

  context = bobgui_accessible_get_at_context (self);
  if (context == NULL)
    return;

  bobgui_at_context_set_accessible_relation (context, relation, NULL);
  bobgui_at_context_update (context);
  g_object_unref (context);
}

/**
 * bobgui_accessible_announce:
 * @self: an accessible object
 * @message: the string to announce
 * @priority: the priority of the announcement
 *
 * Requests the user's screen reader to announce the given message.
 *
 * This kind of notification is useful for messages that
 * either have only a visual representation or that are not
 * exposed visually at all, e.g. a notification about a
 * successful operation.
 *
 * Also, by using this API, you can ensure that the message
 * does not interrupts the user's current screen reader output.
 *
 * Since: 4.14
 */
void
bobgui_accessible_announce (BobguiAccessible                     *self,
                         const char                        *message,
                         BobguiAccessibleAnnouncementPriority  priority)
{
  BobguiATContext *context;

  g_return_if_fail (BOBGUI_IS_ACCESSIBLE (self));

  context = bobgui_accessible_get_at_context (self);
  if (context == NULL)
    return;

  bobgui_at_context_announce (context, message, priority);
  g_object_unref (context);
}

static const char *role_names[] = {
  [BOBGUI_ACCESSIBLE_ROLE_ALERT] = NC_("accessibility", "alert"),
  [BOBGUI_ACCESSIBLE_ROLE_ALERT_DIALOG] = NC_("accessibility", "alert dialog"),
  [BOBGUI_ACCESSIBLE_ROLE_BANNER] = NC_("accessibility", "banner"),
  [BOBGUI_ACCESSIBLE_ROLE_BUTTON] = NC_("accessibility", "button"),
  [BOBGUI_ACCESSIBLE_ROLE_CAPTION] = NC_("accessibility", "caption"),
  [BOBGUI_ACCESSIBLE_ROLE_CELL] = NC_("accessibility", "cell"),
  [BOBGUI_ACCESSIBLE_ROLE_CHECKBOX] = NC_("accessibility", "checkbox"),
  [BOBGUI_ACCESSIBLE_ROLE_COLUMN_HEADER] = NC_("accessibility", "column header"),
  [BOBGUI_ACCESSIBLE_ROLE_COMBO_BOX] = NC_("accessibility", "combo box"),
  [BOBGUI_ACCESSIBLE_ROLE_COMMAND] = NC_("accessibility", "command"),
  [BOBGUI_ACCESSIBLE_ROLE_COMPOSITE] = NC_("accessibility", "composite"),
  [BOBGUI_ACCESSIBLE_ROLE_DIALOG] = NC_("accessibility", "dialog"),
  [BOBGUI_ACCESSIBLE_ROLE_DOCUMENT] = NC_("accessibility", "document"),
  [BOBGUI_ACCESSIBLE_ROLE_FEED] = NC_("accessibility", "feed"),
  [BOBGUI_ACCESSIBLE_ROLE_FORM] = NC_("accessibility", "form"),
  [BOBGUI_ACCESSIBLE_ROLE_GENERIC] = NC_("accessibility", "generic"),
  [BOBGUI_ACCESSIBLE_ROLE_GRID] = NC_("accessibility", "grid"),
  [BOBGUI_ACCESSIBLE_ROLE_GRID_CELL] = NC_("accessibility", "grid cell"),
  [BOBGUI_ACCESSIBLE_ROLE_GROUP] = NC_("accessibility", "group"),
  [BOBGUI_ACCESSIBLE_ROLE_HEADING] = NC_("accessibility", "heading"),
  [BOBGUI_ACCESSIBLE_ROLE_IMG] = NC_("accessibility", "image"),
  [BOBGUI_ACCESSIBLE_ROLE_INPUT] = NC_("accessibility", "input"),
  [BOBGUI_ACCESSIBLE_ROLE_LABEL] = NC_("accessibility", "label"),
  [BOBGUI_ACCESSIBLE_ROLE_LANDMARK] = NC_("accessibility", "landmark"),
  [BOBGUI_ACCESSIBLE_ROLE_LEGEND] = NC_("accessibility", "legend"),
  [BOBGUI_ACCESSIBLE_ROLE_LINK] = NC_("accessibility", "link"),
  [BOBGUI_ACCESSIBLE_ROLE_LIST] = NC_("accessibility", "list"),
  [BOBGUI_ACCESSIBLE_ROLE_LIST_BOX] = NC_("accessibility", "list box"),
  [BOBGUI_ACCESSIBLE_ROLE_LIST_ITEM] = NC_("accessibility", "list item"),
  [BOBGUI_ACCESSIBLE_ROLE_LOG] = NC_("accessibility", "log"),
  [BOBGUI_ACCESSIBLE_ROLE_MAIN] = NC_("accessibility", "main"),
  [BOBGUI_ACCESSIBLE_ROLE_MARQUEE] = NC_("accessibility", "marquee"),
  [BOBGUI_ACCESSIBLE_ROLE_MATH] = NC_("accessibility", "math"),
  [BOBGUI_ACCESSIBLE_ROLE_METER] = NC_("accessibility", "meter"),
  [BOBGUI_ACCESSIBLE_ROLE_MENU] = NC_("accessibility", "menu"),
  [BOBGUI_ACCESSIBLE_ROLE_MENU_BAR] = NC_("accessibility", "menu bar"),
  [BOBGUI_ACCESSIBLE_ROLE_MENU_ITEM] = NC_("accessibility", "menu item"),
  [BOBGUI_ACCESSIBLE_ROLE_MENU_ITEM_CHECKBOX] = NC_("accessibility", "menu item checkbox"),
  [BOBGUI_ACCESSIBLE_ROLE_MENU_ITEM_RADIO] = NC_("accessibility", "menu item radio"),
  [BOBGUI_ACCESSIBLE_ROLE_NAVIGATION] = NC_("accessibility", "navigation"),
  [BOBGUI_ACCESSIBLE_ROLE_NONE] = NC_("accessibility", "none"),
  [BOBGUI_ACCESSIBLE_ROLE_NOTE] = NC_("accessibility", "note"),
  [BOBGUI_ACCESSIBLE_ROLE_OPTION] = NC_("accessibility", "option"),
  [BOBGUI_ACCESSIBLE_ROLE_PRESENTATION] = NC_("accessibility", "presentation"),
  [BOBGUI_ACCESSIBLE_ROLE_PROGRESS_BAR] = NC_("accessibility", "progress bar"),
  [BOBGUI_ACCESSIBLE_ROLE_RADIO] = NC_("accessibility", "radio"),
  [BOBGUI_ACCESSIBLE_ROLE_RADIO_GROUP] = NC_("accessibility", "radio group"),
  [BOBGUI_ACCESSIBLE_ROLE_RANGE] = NC_("accessibility", "range"),
  [BOBGUI_ACCESSIBLE_ROLE_REGION] = NC_("accessibility", "region"),
  [BOBGUI_ACCESSIBLE_ROLE_ROW] = NC_("accessibility", "row"),
  [BOBGUI_ACCESSIBLE_ROLE_ROW_GROUP] = NC_("accessibility", "row group"),
  [BOBGUI_ACCESSIBLE_ROLE_ROW_HEADER] = NC_("accessibility", "row header"),
  [BOBGUI_ACCESSIBLE_ROLE_SCROLLBAR] = NC_("accessibility", "scroll bar"),
  [BOBGUI_ACCESSIBLE_ROLE_SEARCH] = NC_("accessibility", "search"),
  [BOBGUI_ACCESSIBLE_ROLE_SEARCH_BOX] = NC_("accessibility", "search box"),
  [BOBGUI_ACCESSIBLE_ROLE_SECTION] = NC_("accessibility", "section"),
  [BOBGUI_ACCESSIBLE_ROLE_SECTION_HEAD] = NC_("accessibility", "section head"),
  [BOBGUI_ACCESSIBLE_ROLE_SELECT] = NC_("accessibility", "select"),
  [BOBGUI_ACCESSIBLE_ROLE_SEPARATOR] = NC_("accessibility", "separator"),
  [BOBGUI_ACCESSIBLE_ROLE_SLIDER] = NC_("accessibility", "slider"),
  [BOBGUI_ACCESSIBLE_ROLE_SPIN_BUTTON] = NC_("accessibility", "spin button"),
  [BOBGUI_ACCESSIBLE_ROLE_STATUS] = NC_("accessibility", "status"),
  [BOBGUI_ACCESSIBLE_ROLE_STRUCTURE] = NC_("accessibility", "structure"),
  [BOBGUI_ACCESSIBLE_ROLE_SWITCH] = NC_("accessibility", "switch"),
  [BOBGUI_ACCESSIBLE_ROLE_TAB] = NC_("accessibility", "tab"),
  [BOBGUI_ACCESSIBLE_ROLE_TABLE] = NC_("accessibility", "table"),
  [BOBGUI_ACCESSIBLE_ROLE_TAB_LIST] = NC_("accessibility", "tab list"),
  [BOBGUI_ACCESSIBLE_ROLE_TAB_PANEL] = NC_("accessibility", "tab panel"),
  [BOBGUI_ACCESSIBLE_ROLE_TEXT_BOX] = NC_("accessibility", "text box"),
  [BOBGUI_ACCESSIBLE_ROLE_TIME] = NC_("accessibility", "time"),
  [BOBGUI_ACCESSIBLE_ROLE_TIMER] = NC_("accessibility", "timer"),
  [BOBGUI_ACCESSIBLE_ROLE_TOOLBAR] = NC_("accessibility", "tool bar"),
  [BOBGUI_ACCESSIBLE_ROLE_TOOLTIP] = NC_("accessibility", "tool tip"),
  [BOBGUI_ACCESSIBLE_ROLE_TREE] = NC_("accessibility", "tree"),
  [BOBGUI_ACCESSIBLE_ROLE_TREE_GRID] = NC_("accessibility", "tree grid"),
  [BOBGUI_ACCESSIBLE_ROLE_TREE_ITEM] = NC_("accessibility", "tree item"),
  [BOBGUI_ACCESSIBLE_ROLE_WIDGET] = NC_("accessibility", "widget"),
  [BOBGUI_ACCESSIBLE_ROLE_WINDOW] = NC_("accessibility", "window"),
  [BOBGUI_ACCESSIBLE_ROLE_TOGGLE_BUTTON] = NC_("accessibility", "toggle button"),
  [BOBGUI_ACCESSIBLE_ROLE_APPLICATION] = NC_("accessibility", "application"),
  [BOBGUI_ACCESSIBLE_ROLE_PARAGRAPH] = NC_("accessibility", "paragraph"),
  [BOBGUI_ACCESSIBLE_ROLE_BLOCK_QUOTE] = NC_("accessibility", "block quote"),
  [BOBGUI_ACCESSIBLE_ROLE_ARTICLE] = NC_("accessibility", "article"),
  [BOBGUI_ACCESSIBLE_ROLE_COMMENT] = NC_("accessibility", "comment"),
  [BOBGUI_ACCESSIBLE_ROLE_TERMINAL] = NC_("accessibility", "terminal"),
};

/*< private >
 * bobgui_accessible_role_to_name:
 * @role: an accessible role
 * @domain: (nullable): the translation domain
 *
 * Converts an accessible role to the equivalent role name.
 *
 * If @domain is not `NULL`, the returned string will be localized.
 *
 * Returns: (transfer none): the name of the role
 */
const char *
bobgui_accessible_role_to_name (BobguiAccessibleRole  role,
                             const char        *domain)
{
  if (domain != NULL)
    return g_dpgettext2 (domain, "accessibility", role_names[role]);

  return role_names[role];
}

static struct {
  BobguiAccessibleRole superclass;
  BobguiAccessibleRole role;
} superclasses[] = {
  { BOBGUI_ACCESSIBLE_ROLE_COMMAND, BOBGUI_ACCESSIBLE_ROLE_BUTTON },
  { BOBGUI_ACCESSIBLE_ROLE_COMMAND, BOBGUI_ACCESSIBLE_ROLE_LINK },
  { BOBGUI_ACCESSIBLE_ROLE_COMMAND, BOBGUI_ACCESSIBLE_ROLE_MENU_ITEM },
  { BOBGUI_ACCESSIBLE_ROLE_COMPOSITE, BOBGUI_ACCESSIBLE_ROLE_GRID },
  { BOBGUI_ACCESSIBLE_ROLE_COMPOSITE, BOBGUI_ACCESSIBLE_ROLE_SELECT },
  { BOBGUI_ACCESSIBLE_ROLE_COMPOSITE, BOBGUI_ACCESSIBLE_ROLE_SPIN_BUTTON },
  { BOBGUI_ACCESSIBLE_ROLE_COMPOSITE, BOBGUI_ACCESSIBLE_ROLE_TAB_LIST },
  { BOBGUI_ACCESSIBLE_ROLE_INPUT, BOBGUI_ACCESSIBLE_ROLE_CHECKBOX },
  { BOBGUI_ACCESSIBLE_ROLE_INPUT, BOBGUI_ACCESSIBLE_ROLE_COMBO_BOX },
  { BOBGUI_ACCESSIBLE_ROLE_INPUT, BOBGUI_ACCESSIBLE_ROLE_OPTION },
  { BOBGUI_ACCESSIBLE_ROLE_INPUT, BOBGUI_ACCESSIBLE_ROLE_RADIO },
  { BOBGUI_ACCESSIBLE_ROLE_INPUT, BOBGUI_ACCESSIBLE_ROLE_SLIDER },
  { BOBGUI_ACCESSIBLE_ROLE_INPUT, BOBGUI_ACCESSIBLE_ROLE_SPIN_BUTTON },
  { BOBGUI_ACCESSIBLE_ROLE_INPUT, BOBGUI_ACCESSIBLE_ROLE_TEXT_BOX },
  { BOBGUI_ACCESSIBLE_ROLE_LANDMARK, BOBGUI_ACCESSIBLE_ROLE_BANNER },
  { BOBGUI_ACCESSIBLE_ROLE_LANDMARK, BOBGUI_ACCESSIBLE_ROLE_FORM },
  { BOBGUI_ACCESSIBLE_ROLE_LANDMARK, BOBGUI_ACCESSIBLE_ROLE_MAIN },
  { BOBGUI_ACCESSIBLE_ROLE_LANDMARK, BOBGUI_ACCESSIBLE_ROLE_NAVIGATION },
  { BOBGUI_ACCESSIBLE_ROLE_LANDMARK, BOBGUI_ACCESSIBLE_ROLE_REGION },
  { BOBGUI_ACCESSIBLE_ROLE_LANDMARK, BOBGUI_ACCESSIBLE_ROLE_SEARCH },
  { BOBGUI_ACCESSIBLE_ROLE_RANGE, BOBGUI_ACCESSIBLE_ROLE_METER },
  { BOBGUI_ACCESSIBLE_ROLE_RANGE, BOBGUI_ACCESSIBLE_ROLE_PROGRESS_BAR },
  { BOBGUI_ACCESSIBLE_ROLE_RANGE, BOBGUI_ACCESSIBLE_ROLE_SCROLLBAR },
  { BOBGUI_ACCESSIBLE_ROLE_RANGE, BOBGUI_ACCESSIBLE_ROLE_SLIDER },
  { BOBGUI_ACCESSIBLE_ROLE_RANGE, BOBGUI_ACCESSIBLE_ROLE_SPIN_BUTTON },
  { BOBGUI_ACCESSIBLE_ROLE_SECTION, BOBGUI_ACCESSIBLE_ROLE_ALERT },
  { BOBGUI_ACCESSIBLE_ROLE_SECTION, BOBGUI_ACCESSIBLE_ROLE_BLOCK_QUOTE },
  { BOBGUI_ACCESSIBLE_ROLE_SECTION, BOBGUI_ACCESSIBLE_ROLE_CAPTION },
  { BOBGUI_ACCESSIBLE_ROLE_SECTION, BOBGUI_ACCESSIBLE_ROLE_CELL },
  { BOBGUI_ACCESSIBLE_ROLE_SECTION, BOBGUI_ACCESSIBLE_ROLE_GROUP },
  { BOBGUI_ACCESSIBLE_ROLE_SECTION, BOBGUI_ACCESSIBLE_ROLE_IMG },
  { BOBGUI_ACCESSIBLE_ROLE_SECTION, BOBGUI_ACCESSIBLE_ROLE_LANDMARK },
  { BOBGUI_ACCESSIBLE_ROLE_SECTION, BOBGUI_ACCESSIBLE_ROLE_LIST },
  { BOBGUI_ACCESSIBLE_ROLE_SECTION, BOBGUI_ACCESSIBLE_ROLE_LIST_ITEM },
  { BOBGUI_ACCESSIBLE_ROLE_SECTION, BOBGUI_ACCESSIBLE_ROLE_LOG },
  { BOBGUI_ACCESSIBLE_ROLE_SECTION, BOBGUI_ACCESSIBLE_ROLE_MARQUEE },
  { BOBGUI_ACCESSIBLE_ROLE_SECTION, BOBGUI_ACCESSIBLE_ROLE_MATH },
  { BOBGUI_ACCESSIBLE_ROLE_SECTION, BOBGUI_ACCESSIBLE_ROLE_NOTE },
  { BOBGUI_ACCESSIBLE_ROLE_SECTION, BOBGUI_ACCESSIBLE_ROLE_PARAGRAPH },
  { BOBGUI_ACCESSIBLE_ROLE_SECTION, BOBGUI_ACCESSIBLE_ROLE_STATUS },
  { BOBGUI_ACCESSIBLE_ROLE_SECTION, BOBGUI_ACCESSIBLE_ROLE_TABLE },
  { BOBGUI_ACCESSIBLE_ROLE_SECTION, BOBGUI_ACCESSIBLE_ROLE_TAB_PANEL },
  { BOBGUI_ACCESSIBLE_ROLE_SECTION, BOBGUI_ACCESSIBLE_ROLE_TIME },
  { BOBGUI_ACCESSIBLE_ROLE_SECTION, BOBGUI_ACCESSIBLE_ROLE_TOOLTIP },
  { BOBGUI_ACCESSIBLE_ROLE_SECTION_HEAD, BOBGUI_ACCESSIBLE_ROLE_COLUMN_HEADER },
  { BOBGUI_ACCESSIBLE_ROLE_SECTION_HEAD, BOBGUI_ACCESSIBLE_ROLE_HEADING },
  { BOBGUI_ACCESSIBLE_ROLE_SECTION_HEAD, BOBGUI_ACCESSIBLE_ROLE_ROW_HEADER },
  { BOBGUI_ACCESSIBLE_ROLE_SECTION_HEAD, BOBGUI_ACCESSIBLE_ROLE_TAB },
  { BOBGUI_ACCESSIBLE_ROLE_SELECT, BOBGUI_ACCESSIBLE_ROLE_LIST_BOX },
  { BOBGUI_ACCESSIBLE_ROLE_SELECT, BOBGUI_ACCESSIBLE_ROLE_MENU },
  { BOBGUI_ACCESSIBLE_ROLE_SELECT, BOBGUI_ACCESSIBLE_ROLE_RADIO_GROUP },
  { BOBGUI_ACCESSIBLE_ROLE_SELECT, BOBGUI_ACCESSIBLE_ROLE_TREE },
  { BOBGUI_ACCESSIBLE_ROLE_STRUCTURE, BOBGUI_ACCESSIBLE_ROLE_APPLICATION },
  { BOBGUI_ACCESSIBLE_ROLE_STRUCTURE, BOBGUI_ACCESSIBLE_ROLE_DOCUMENT },
  { BOBGUI_ACCESSIBLE_ROLE_STRUCTURE, BOBGUI_ACCESSIBLE_ROLE_GENERIC },
  { BOBGUI_ACCESSIBLE_ROLE_STRUCTURE, BOBGUI_ACCESSIBLE_ROLE_PRESENTATION },
  { BOBGUI_ACCESSIBLE_ROLE_STRUCTURE, BOBGUI_ACCESSIBLE_ROLE_RANGE },
  { BOBGUI_ACCESSIBLE_ROLE_STRUCTURE, BOBGUI_ACCESSIBLE_ROLE_ROW_GROUP },
  { BOBGUI_ACCESSIBLE_ROLE_STRUCTURE, BOBGUI_ACCESSIBLE_ROLE_SECTION },
  { BOBGUI_ACCESSIBLE_ROLE_STRUCTURE, BOBGUI_ACCESSIBLE_ROLE_SECTION_HEAD },
  { BOBGUI_ACCESSIBLE_ROLE_STRUCTURE, BOBGUI_ACCESSIBLE_ROLE_SEPARATOR },
  { BOBGUI_ACCESSIBLE_ROLE_WIDGET, BOBGUI_ACCESSIBLE_ROLE_COMMAND },
  { BOBGUI_ACCESSIBLE_ROLE_WIDGET, BOBGUI_ACCESSIBLE_ROLE_COMPOSITE },
  { BOBGUI_ACCESSIBLE_ROLE_WIDGET, BOBGUI_ACCESSIBLE_ROLE_GRID_CELL },
  { BOBGUI_ACCESSIBLE_ROLE_WIDGET, BOBGUI_ACCESSIBLE_ROLE_INPUT },
  { BOBGUI_ACCESSIBLE_ROLE_WIDGET, BOBGUI_ACCESSIBLE_ROLE_PROGRESS_BAR },
  { BOBGUI_ACCESSIBLE_ROLE_WIDGET, BOBGUI_ACCESSIBLE_ROLE_ROW },
  { BOBGUI_ACCESSIBLE_ROLE_WIDGET, BOBGUI_ACCESSIBLE_ROLE_SCROLLBAR },
  { BOBGUI_ACCESSIBLE_ROLE_WIDGET, BOBGUI_ACCESSIBLE_ROLE_SEPARATOR },
  { BOBGUI_ACCESSIBLE_ROLE_WIDGET, BOBGUI_ACCESSIBLE_ROLE_TAB },
  { BOBGUI_ACCESSIBLE_ROLE_WINDOW, BOBGUI_ACCESSIBLE_ROLE_DIALOG },
  { BOBGUI_ACCESSIBLE_ROLE_CHECKBOX, BOBGUI_ACCESSIBLE_ROLE_SWITCH },
  { BOBGUI_ACCESSIBLE_ROLE_GRID_CELL, BOBGUI_ACCESSIBLE_ROLE_COLUMN_HEADER },
  { BOBGUI_ACCESSIBLE_ROLE_GRID_CELL, BOBGUI_ACCESSIBLE_ROLE_ROW_HEADER },
  { BOBGUI_ACCESSIBLE_ROLE_MENU_ITEM, BOBGUI_ACCESSIBLE_ROLE_MENU_ITEM_CHECKBOX },
  { BOBGUI_ACCESSIBLE_ROLE_MENU_ITEM_CHECKBOX, BOBGUI_ACCESSIBLE_ROLE_MENU_ITEM_RADIO },
  { BOBGUI_ACCESSIBLE_ROLE_TREE, BOBGUI_ACCESSIBLE_ROLE_TREE_GRID },
  { BOBGUI_ACCESSIBLE_ROLE_CELL, BOBGUI_ACCESSIBLE_ROLE_COLUMN_HEADER },
  { BOBGUI_ACCESSIBLE_ROLE_CELL, BOBGUI_ACCESSIBLE_ROLE_GRID_CELL },
  { BOBGUI_ACCESSIBLE_ROLE_CELL, BOBGUI_ACCESSIBLE_ROLE_ROW_HEADER },
  { BOBGUI_ACCESSIBLE_ROLE_GROUP, BOBGUI_ACCESSIBLE_ROLE_ROW },
  { BOBGUI_ACCESSIBLE_ROLE_GROUP, BOBGUI_ACCESSIBLE_ROLE_SELECT },
  { BOBGUI_ACCESSIBLE_ROLE_GROUP, BOBGUI_ACCESSIBLE_ROLE_TOOLBAR },
  { BOBGUI_ACCESSIBLE_ROLE_LIST, BOBGUI_ACCESSIBLE_ROLE_FEED },
  { BOBGUI_ACCESSIBLE_ROLE_LIST_ITEM, BOBGUI_ACCESSIBLE_ROLE_TREE_ITEM },
  { BOBGUI_ACCESSIBLE_ROLE_TABLE, BOBGUI_ACCESSIBLE_ROLE_GRID },
  { BOBGUI_ACCESSIBLE_ROLE_ALERT, BOBGUI_ACCESSIBLE_ROLE_ALERT_DIALOG },
  { BOBGUI_ACCESSIBLE_ROLE_STATUS, BOBGUI_ACCESSIBLE_ROLE_TIMER },
  { BOBGUI_ACCESSIBLE_ROLE_DIALOG, BOBGUI_ACCESSIBLE_ROLE_ALERT_DIALOG },
  { BOBGUI_ACCESSIBLE_ROLE_DOCUMENT, BOBGUI_ACCESSIBLE_ROLE_ARTICLE },
  { BOBGUI_ACCESSIBLE_ROLE_ARTICLE, BOBGUI_ACCESSIBLE_ROLE_COMMENT },
  { BOBGUI_ACCESSIBLE_ROLE_TERMINAL, BOBGUI_ACCESSIBLE_ROLE_WIDGET },
};

gboolean
bobgui_accessible_role_is_subclass (BobguiAccessibleRole role,
                                 BobguiAccessibleRole superclass)
{
  for (unsigned int i = 0; i < G_N_ELEMENTS (superclasses); i++)
    {
      if (superclasses[i].role == role &&
          superclasses[i].superclass == superclass)
        return TRUE;
    }

  return FALSE;
}

/*< private >
 * bobgui_accessible_role_is_range_subclass:
 * @role: an accessible role
 *
 * Checks if the role is considered to be a subclass of
 * [enum@Bobgui.AccessibleRole.range] according to the WAI-ARIA
 * specification.
 *
 * Returns: whether the role is range-like
 */
gboolean
bobgui_accessible_role_is_range_subclass (BobguiAccessibleRole role)
{
  return bobgui_accessible_role_is_subclass (role, BOBGUI_ACCESSIBLE_ROLE_RANGE);
}

/* < private >
 * bobgui_accessible_role_is_abstract:
 * @role: an accessible role
 *
 * Checks if the role is considered abstract and should not be used
 * for concrete widgets.
 *
 * Returns: whether the role is abstract
 */
gboolean
bobgui_accessible_role_is_abstract (BobguiAccessibleRole role)
{
  switch ((int) role)
    {
    case BOBGUI_ACCESSIBLE_ROLE_COMMAND:
    case BOBGUI_ACCESSIBLE_ROLE_COMPOSITE:
    case BOBGUI_ACCESSIBLE_ROLE_INPUT:
    case BOBGUI_ACCESSIBLE_ROLE_LANDMARK:
    case BOBGUI_ACCESSIBLE_ROLE_RANGE:
    case BOBGUI_ACCESSIBLE_ROLE_SECTION:
    case BOBGUI_ACCESSIBLE_ROLE_SECTION_HEAD:
    case BOBGUI_ACCESSIBLE_ROLE_SELECT:
    case BOBGUI_ACCESSIBLE_ROLE_STRUCTURE:
    case BOBGUI_ACCESSIBLE_ROLE_WIDGET:
      return TRUE;
    default:
      return FALSE;
    }
}

/*< private >
 * bobgui_accessible_platform_changed:
 * @self: an accessible object
 * @change: the platform state change to report
 *
 * Notifies accessible technologies that a platform value has changed.
 *
 * ARIA discriminates between author-controlled states and 'platform'
 * states, which are not. This function can be used by widgets to
 * inform ATs that a platform state, such as focus, has changed.
 *
 * Note that the state itself is not included in this API.
 * AT backends should use [method@Bobgui.Accessible.get_platform_state]
 * to obtain the actual state.
 */
static void
bobgui_accessible_platform_changed (BobguiAccessible               *self,
                                 BobguiAccessiblePlatformChange  change)
{
  BobguiATContext *context;

  if (change == 0)
    return;

  if (BOBGUI_IS_WIDGET (self) &&
      !bobgui_widget_get_realized (BOBGUI_WIDGET (self)))
    return;

  context = bobgui_accessible_get_at_context (self);

  /* propagate changes up from ignored widgets */
  if (bobgui_accessible_get_accessible_role (self) == BOBGUI_ACCESSIBLE_ROLE_NONE)
    {
      BobguiAccessible *parent = bobgui_accessible_get_accessible_parent (self);

      if (parent != NULL)
        {
          g_clear_object (&context);
          context = bobgui_accessible_get_at_context (parent);
          g_object_unref (parent);
        }
    }

  if (context == NULL)
    return;

  bobgui_at_context_platform_changed (context, change);
  bobgui_at_context_update (context);
  g_object_unref (context);
}

/**
 * bobgui_accessible_get_platform_state:
 * @self: an accessible object
 * @state: platform state to query
 *
 * Queries a platform state, such as focus.
 *
 * This functionality can be overridden by `BobguiAccessible`
 * implementations, e.g. to get platform state from an ignored
 * child widget, as is the case for `BobguiText` wrappers.
 *
 * Returns: the value of state for the accessible
 *
 * Since: 4.10
 */
gboolean
bobgui_accessible_get_platform_state (BobguiAccessible              *self,
                                   BobguiAccessiblePlatformState  state)
{
  g_return_val_if_fail (BOBGUI_IS_ACCESSIBLE (self), FALSE);

  return BOBGUI_ACCESSIBLE_GET_IFACE (self)->get_platform_state (self, state);
}

/**
 * bobgui_accessible_update_platform_state:
 * @self: an accessible object
 * @state: the platform state to update
 *
 * Informs ATs that the platform state has changed.
 *
 * This function should be used by `BobguiAccessible` implementations that
 * have a platform state but are not widgets. Widgets handle platform
 * states automatically.
 *
 * Since: 4.18
 */
void
bobgui_accessible_update_platform_state (BobguiAccessible              *self,
                                      BobguiAccessiblePlatformState  state)
{
  BobguiAccessiblePlatformChange change = 0;

  g_return_if_fail (BOBGUI_IS_ACCESSIBLE (self));

  switch (state)
    {
    case BOBGUI_ACCESSIBLE_PLATFORM_STATE_FOCUSABLE:
      change |= BOBGUI_ACCESSIBLE_PLATFORM_CHANGE_FOCUSABLE;
      break;

    case BOBGUI_ACCESSIBLE_PLATFORM_STATE_FOCUSED:
      change |= BOBGUI_ACCESSIBLE_PLATFORM_CHANGE_FOCUSED;
      break;

    case BOBGUI_ACCESSIBLE_PLATFORM_STATE_ACTIVE:
      change |= BOBGUI_ACCESSIBLE_PLATFORM_CHANGE_ACTIVE;
      break;

    default:
      g_assert_not_reached ();
    }

  bobgui_accessible_platform_changed (self, change);
}

/*< private >
 * bobgui_accessible_bounds_changed:
 * @self: an accessible object
 *
 * This function can be used to inform ATs that an
 * accessibles bounds (ie its screen extents) have
 * changed.
 *
 * Note that the bounds are not included in this API.
 * AT backends should use [method@Bobgui.Accessible.get_bounds]
 * to get them.
 */
void
bobgui_accessible_bounds_changed (BobguiAccessible *self)
{
  BobguiATContext *context;

  if (BOBGUI_IS_WIDGET (self) &&
      bobgui_widget_get_root (BOBGUI_WIDGET (self)) == NULL)
    return;

  context = bobgui_accessible_get_at_context (self);
  if (context == NULL)
    return;

  bobgui_at_context_bounds_changed (context);
  g_object_unref (context);
}

/**
 * bobgui_accessible_get_bounds:
 * @self: an accessible object
 * @x: (out): the x coordinate of the top left corner of the accessible
 * @y: (out): the y coordinate of the top left corner of the widget
 * @width: (out): the width of the accessible object
 * @height: (out): the height of the accessible object
 *
 * Queries the coordinates and dimensions of this accessible
 *
 * This functionality can be overridden by `BobguiAccessible`
 * implementations, e.g. to get the bounds from an ignored
 * child widget.
 *
 * Returns: true if the bounds are valid, and false otherwise
 *
 * Since: 4.10
 */
gboolean
bobgui_accessible_get_bounds (BobguiAccessible *self,
                           int           *x,
                           int           *y,
                           int           *width,
                           int           *height)
{
  g_return_val_if_fail (BOBGUI_IS_ACCESSIBLE (self), FALSE);
  g_return_val_if_fail (x != NULL && y != NULL, FALSE);
  g_return_val_if_fail (width != NULL && height != NULL, FALSE);

  return BOBGUI_ACCESSIBLE_GET_IFACE (self)->get_bounds (self, x, y, width, height);
}

/**
 * bobgui_accessible_get_accessible_id:
 * @self: an accessible object
 *
 * Retrieves the accessible identifier for the accessible object.
 *
 * This functionality can be overridden by `BobguiAccessible`
 * implementations.
 *
 * It is left to the accessible implementation to define the scope
 * and uniqueness of the identifier.
 *
 * Returns: (transfer full) (nullable): the accessible identifier
 *
 * Since: 4.22
 */
char *
bobgui_accessible_get_accessible_id (BobguiAccessible *self)
{
  g_return_val_if_fail (BOBGUI_IS_ACCESSIBLE (self), NULL);

  return BOBGUI_ACCESSIBLE_GET_IFACE (self)->get_accessible_id (self);
}

struct _BobguiAccessibleList
{
  GList *objects;
};

/**
 * bobgui_accessible_list_new_from_list:
 * @list: (element-type BobguiAccessible): a list
 *   of accessible objects
 *
 * Allocates a new `BobguiAccessibleList`, doing a shallow copy
 * of the passed list of accessible objects
 *
 * Returns: (transfer full): the list of accessible objects
 *
 * Since: 4.14
 */
BobguiAccessibleList *
bobgui_accessible_list_new_from_list (GList *list)
{
  BobguiAccessibleList *accessible_list = g_new (BobguiAccessibleList, 1);

  accessible_list->objects = g_list_copy (list);

  return accessible_list;
}

/**
 * bobgui_accessible_list_new_from_array:
 * @accessibles: (array length=n_accessibles): array of accessible objects
 * @n_accessibles: length of the @accessibles array
 *
 * Allocates a new list of accessible objects.
 *
 * Returns: (transfer full): the newly created list of accessible objects
 *
 * Since: 4.14
 */
BobguiAccessibleList *
bobgui_accessible_list_new_from_array (BobguiAccessible **accessibles,
                                    gsize           n_accessibles)
{
  BobguiAccessibleList *accessible_list;
  GList *list = NULL;

  g_return_val_if_fail (accessibles == NULL || n_accessibles == 0, NULL);

  accessible_list = g_new (BobguiAccessibleList, 1);

  for (gsize i = 0; i < n_accessibles; i++)
    {
      list = g_list_prepend (list, accessibles[i]);
    }

  accessible_list->objects = g_list_reverse (list);

  return accessible_list;
}

static void
bobgui_accessible_list_free (BobguiAccessibleList *accessible_list)
{
  g_free (accessible_list->objects);
  g_free (accessible_list);
}

static BobguiAccessibleList *
bobgui_accessible_list_copy (BobguiAccessibleList *accessible_list)
{
  return bobgui_accessible_list_new_from_list (accessible_list->objects);
}

/**
 * bobgui_accessible_list_get_objects:
 *
 * Gets the list of objects this boxed type holds.
 *
 * Returns: (transfer container) (element-type BobguiAccessible): a shallow copy
 *   of the objects
 *
 * Since: 4.14
 */
GList *
bobgui_accessible_list_get_objects (BobguiAccessibleList *accessible_list)
{
  return g_list_copy (accessible_list->objects);
}

G_DEFINE_BOXED_TYPE (BobguiAccessibleList, bobgui_accessible_list,
                     bobgui_accessible_list_copy,
                     bobgui_accessible_list_free)

/*< private >
 * bobgui_accessible_should_present:
 * @self: an accessible object
 *
 * Returns whether this accessible object should be represented to ATs.
 *
 * By default, hidden widgets are are among these, but there can
 * be other reasons to return false, e.g. for widgets that are
 * purely presentations, or for widgets whose functionality is
 * represented elsewhere, as is the case for `BobguiText` widgets.
 *
 * Returns: true if the widget should be represented
 */
gboolean
bobgui_accessible_should_present (BobguiAccessible *self)
{
  BobguiAccessibleRole role;
  BobguiATContext *context;
  gboolean res = TRUE;

  if (BOBGUI_IS_WIDGET (self) &&
      !bobgui_widget_get_visible (BOBGUI_WIDGET (self)))
    return FALSE;

  role = bobgui_accessible_get_accessible_role (self);
  if (role == BOBGUI_ACCESSIBLE_ROLE_NONE ||
      role == BOBGUI_ACCESSIBLE_ROLE_PRESENTATION)
    return FALSE;

  context = bobgui_accessible_get_at_context (self);
  if (context == NULL)
    return FALSE;

  if (bobgui_at_context_has_accessible_state (context, BOBGUI_ACCESSIBLE_STATE_HIDDEN))
    {
      BobguiAccessibleValue *value;

      value = bobgui_at_context_get_accessible_state (context, BOBGUI_ACCESSIBLE_STATE_HIDDEN);
      if (bobgui_boolean_accessible_value_get (value))
        res = FALSE;
    }

  g_object_unref (context);

  return res;
}

void
bobgui_accessible_update_children (BobguiAccessible           *self,
                                BobguiAccessible           *child,
                                BobguiAccessibleChildState  state)
{
  BobguiATContext *context;

  if (BOBGUI_IS_WIDGET (self) &&
      bobgui_widget_get_root (BOBGUI_WIDGET (self)) == NULL)
    return;

  /* propagate changes up from ignored widgets */
  if (bobgui_accessible_get_accessible_role (self) == BOBGUI_ACCESSIBLE_ROLE_NONE)
    {
      BobguiAccessible *parent = bobgui_accessible_get_accessible_parent (self);

      context = bobgui_accessible_get_at_context (parent);

      g_object_unref (parent);
    }
  else
    {
      context = bobgui_accessible_get_at_context (self);
    }

  if (context == NULL)
    return;

  bobgui_at_context_child_changed (context, 1 << state, child);
  bobgui_at_context_update (context);
  g_object_unref (context);
}

/*< private >
 * bobgui_accessible_is_password_text:
 * @accessible: an accessible object
 *
 * Returns whether this accessible represents a password text field.
 *
 * Returns: true if the accessible is a password text field
 */
gboolean
bobgui_accessible_is_password_text (BobguiAccessible *accessible)
{
  BobguiInputPurpose purpose = BOBGUI_INPUT_PURPOSE_FREE_FORM;
  gboolean found_purpose = FALSE;

  if (BOBGUI_IS_TEXT (accessible))
    {
      purpose = bobgui_text_get_input_purpose (BOBGUI_TEXT (accessible));
      found_purpose = TRUE;
    }
  else if (BOBGUI_IS_EDITABLE (accessible))
    {
      BobguiEditable *delegate = bobgui_editable_get_delegate (BOBGUI_EDITABLE (accessible));
      if (delegate && BOBGUI_IS_TEXT (delegate))
        {
          purpose = bobgui_text_get_input_purpose (BOBGUI_TEXT (delegate));
          found_purpose = TRUE;
        }
    }

  if (found_purpose && 
      (purpose == BOBGUI_INPUT_PURPOSE_PASSWORD || purpose == BOBGUI_INPUT_PURPOSE_PIN))
    return TRUE;
  return FALSE;
}
