/*
 * Copyright © 2020 Benjamin Otte
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

#include "bobguiboolfilter.h"

#include "bobguitypebuiltins.h"

/**
 * BobguiBoolFilter:
 *
 * Evaluates a boolean expression to determine whether to include items.
 */

struct _BobguiBoolFilter
{
  BobguiFilter parent_instance;

  gboolean invert;
  BobguiExpression *expression;
};

enum {
  PROP_0,
  PROP_EXPRESSION,
  PROP_INVERT,
  NUM_PROPERTIES
};

G_DEFINE_TYPE (BobguiBoolFilter, bobgui_bool_filter, BOBGUI_TYPE_FILTER)

static GParamSpec *properties[NUM_PROPERTIES] = { NULL, };

static gboolean
bobgui_bool_filter_match (BobguiFilter *filter,
                       gpointer   item)
{
  BobguiBoolFilter *self = BOBGUI_BOOL_FILTER (filter);
  GValue value = G_VALUE_INIT;
  gboolean result;

  if (self->expression == NULL ||
      !bobgui_expression_evaluate (self->expression, item, &value))
    return FALSE;
  result = g_value_get_boolean (&value);

  g_value_unset (&value);

  if (self->invert)
    result = !result;

  return result;
}

static BobguiFilterMatch
bobgui_bool_filter_get_strictness (BobguiFilter *filter)
{
  BobguiBoolFilter *self = BOBGUI_BOOL_FILTER (filter);

  if (self->expression == NULL)
    return BOBGUI_FILTER_MATCH_NONE;

  return BOBGUI_FILTER_MATCH_SOME;
}

static void
bobgui_bool_filter_set_property (GObject      *object,
                              guint         prop_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  BobguiBoolFilter *self = BOBGUI_BOOL_FILTER (object);

  switch (prop_id)
    {
    case PROP_EXPRESSION:
      bobgui_bool_filter_set_expression (self, bobgui_value_get_expression (value));
      break;

    case PROP_INVERT:
      bobgui_bool_filter_set_invert (self, g_value_get_boolean (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void 
bobgui_bool_filter_get_property (GObject     *object,
                              guint        prop_id,
                              GValue      *value,
                              GParamSpec  *pspec)
{
  BobguiBoolFilter *self = BOBGUI_BOOL_FILTER (object);

  switch (prop_id)
    {
    case PROP_EXPRESSION:
      bobgui_value_set_expression (value, self->expression);
      break;

    case PROP_INVERT:
      g_value_set_boolean (value, self->invert);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bobgui_bool_filter_dispose (GObject *object)
{
  BobguiBoolFilter *self = BOBGUI_BOOL_FILTER (object);

  g_clear_pointer (&self->expression, bobgui_expression_unref);

  G_OBJECT_CLASS (bobgui_bool_filter_parent_class)->dispose (object);
}

static void
bobgui_bool_filter_class_init (BobguiBoolFilterClass *class)
{
  BobguiFilterClass *filter_class = BOBGUI_FILTER_CLASS (class);
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  filter_class->match = bobgui_bool_filter_match;
  filter_class->get_strictness = bobgui_bool_filter_get_strictness;

  object_class->get_property = bobgui_bool_filter_get_property;
  object_class->set_property = bobgui_bool_filter_set_property;
  object_class->dispose = bobgui_bool_filter_dispose;

  /**
   * BobguiBoolFilter:expression: (type BobguiExpression)
   *
   * The boolean expression to evaluate on each item.
   */
  properties[PROP_EXPRESSION] =
    bobgui_param_spec_expression ("expression", NULL, NULL,
                               G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiBoolFilter:invert:
   *
   * If the expression result should be inverted.
   */
  properties[PROP_INVERT] =
      g_param_spec_boolean ("invert", NULL, NULL,
                            FALSE,
                            G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, NUM_PROPERTIES, properties);
}

static void
bobgui_bool_filter_init (BobguiBoolFilter *self)
{
}

/**
 * bobgui_bool_filter_new:
 * @expression: (transfer full) (nullable): the expression to evaluate
 *
 * Creates a new bool filter.
 *
 * Returns: a new `BobguiBoolFilter`
 */
BobguiBoolFilter *
bobgui_bool_filter_new (BobguiExpression *expression)
{
  BobguiBoolFilter *result;

  result = g_object_new (BOBGUI_TYPE_BOOL_FILTER,
                         "expression", expression,
                         NULL);

  g_clear_pointer (&expression, bobgui_expression_unref);

  return result;
}

/**
 * bobgui_bool_filter_get_expression:
 * @self: a bool filter
 *
 * Gets the expression that the filter evaluates for
 * each item.
 *
 * Returns: (transfer none) (nullable): the expression
 */
BobguiExpression *
bobgui_bool_filter_get_expression (BobguiBoolFilter *self)
{
  g_return_val_if_fail (BOBGUI_IS_BOOL_FILTER (self), NULL);

  return self->expression;
}

/**
 * bobgui_bool_filter_set_expression:
 * @self: a bool filter
 * @expression: (nullable): the expression
 *
 * Sets the expression that the filter uses to check if items
 * should be filtered.
 *
 * The expression must have a value type of `G_TYPE_BOOLEAN`.
 */
void
bobgui_bool_filter_set_expression (BobguiBoolFilter *self,
                                BobguiExpression *expression)
{
  g_return_if_fail (BOBGUI_IS_BOOL_FILTER (self));
  g_return_if_fail (expression == NULL || bobgui_expression_get_value_type (expression) == G_TYPE_BOOLEAN);

  if (self->expression == expression)
    return;

  g_clear_pointer (&self->expression, bobgui_expression_unref);
  if (expression)
    self->expression = bobgui_expression_ref (expression);

  bobgui_filter_changed (BOBGUI_FILTER (self), BOBGUI_FILTER_CHANGE_DIFFERENT_REWATCH);

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_EXPRESSION]);
}

/**
 * bobgui_bool_filter_get_invert:
 * @self: a bool filter
 *
 * Returns whether the filter inverts the expression.
 *
 * Returns: true if the filter inverts
 */
gboolean
bobgui_bool_filter_get_invert (BobguiBoolFilter *self)
{
  g_return_val_if_fail (BOBGUI_IS_BOOL_FILTER (self), TRUE);

  return self->invert;
}

/**
 * bobgui_bool_filter_set_invert:
 * @self: a bool filter
 * @invert: true to invert
 *
 * Sets whether the filter should invert the expression.
 */
void
bobgui_bool_filter_set_invert (BobguiBoolFilter *self,
                            gboolean       invert)
{
  g_return_if_fail (BOBGUI_IS_BOOL_FILTER (self));

  if (self->invert == invert)
    return;

  self->invert = invert;

  bobgui_filter_changed (BOBGUI_FILTER (self), BOBGUI_FILTER_CHANGE_DIFFERENT);

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_INVERT]);
}

