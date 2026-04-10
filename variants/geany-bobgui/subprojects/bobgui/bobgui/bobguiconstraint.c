/* bobguiconstraint.c: Constraint between two widgets
 * Copyright 2019  GNOME Foundation
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
 * Author: Emmanuele Bassi
 */

/**
 * BobguiConstraint:
 *
 * Describes a constraint between attributes of two widgets,
 *  expressed as a linear equation.
 *
 * The typical equation for a constraint is:
 *
 * ```
 *   target.target_attr = source.source_attr × multiplier + constant
 * ```
 *
 * Each `BobguiConstraint` is part of a system that will be solved by a
 * [class@Bobgui.ConstraintLayout] in order to allocate and position each
 * child widget or guide.
 *
 * The source and target, as well as their attributes, of a `BobguiConstraint`
 * instance are immutable after creation.
 */

#include "config.h"

#include "bobguiconstraintprivate.h"
#include "bobguiconstraintsolverprivate.h"
#include "bobguitypebuiltins.h"
#include "bobguiwidget.h"

enum {
  PROP_TARGET = 1,
  PROP_TARGET_ATTRIBUTE,
  PROP_RELATION,
  PROP_SOURCE,
  PROP_SOURCE_ATTRIBUTE,
  PROP_MULTIPLIER,
  PROP_CONSTANT,
  PROP_STRENGTH,

  N_PROPERTIES
};

static GParamSpec *obj_props[N_PROPERTIES];

G_DEFINE_TYPE (BobguiConstraint, bobgui_constraint, G_TYPE_OBJECT)

static void
bobgui_constraint_set_property (GObject      *gobject,
                             guint         prop_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
  BobguiConstraint *self = BOBGUI_CONSTRAINT (gobject);

  switch (prop_id)
    {
    case PROP_TARGET:
      self->target = g_value_get_object (value);
      break;

    case PROP_TARGET_ATTRIBUTE:
      self->target_attribute = g_value_get_enum (value);
      break;

    case PROP_RELATION:
      self->relation = g_value_get_enum (value);
      break;

    case PROP_SOURCE:
      self->source = g_value_get_object (value);
      break;

    case PROP_SOURCE_ATTRIBUTE:
      self->source_attribute = g_value_get_enum (value);
      break;

    case PROP_MULTIPLIER:
      self->multiplier = g_value_get_double (value);
      break;

    case PROP_CONSTANT:
      self->constant = g_value_get_double (value);
      break;

    case PROP_STRENGTH:
      self->strength = g_value_get_int (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
    }
}

static void
bobgui_constraint_get_property (GObject    *gobject,
                             guint       prop_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
  BobguiConstraint *self = BOBGUI_CONSTRAINT (gobject);

  switch (prop_id)
    {
    case PROP_TARGET:
      g_value_set_object (value, self->target);
      break;

    case PROP_TARGET_ATTRIBUTE:
      g_value_set_enum (value, self->target_attribute);
      break;

    case PROP_RELATION:
      g_value_set_enum (value, self->relation);
      break;

    case PROP_SOURCE:
      g_value_set_object (value, self->source);
      break;

    case PROP_SOURCE_ATTRIBUTE:
      g_value_set_enum (value, self->source_attribute);
      break;

    case PROP_MULTIPLIER:
      g_value_set_double (value, self->multiplier);
      break;

    case PROP_CONSTANT:
      g_value_set_double (value, self->constant);
      break;

    case PROP_STRENGTH:
      g_value_set_int (value, self->strength);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
    }
}

static void
bobgui_constraint_finalize (GObject *gobject)
{
  BobguiConstraint *self = BOBGUI_CONSTRAINT (gobject);

  bobgui_constraint_detach (self);

  G_OBJECT_CLASS (bobgui_constraint_parent_class)->finalize (gobject);
}

static void
bobgui_constraint_class_init (BobguiConstraintClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->set_property = bobgui_constraint_set_property;
  gobject_class->get_property = bobgui_constraint_get_property;
  gobject_class->finalize = bobgui_constraint_finalize;

  /**
   * BobguiConstraint:target:
   *
   * The target of the constraint.
   *
   * The constraint will set the [property@Bobgui.Constraint:target-attribute]
   * property of the target using the [property@Bobgui.Constraint:source-attribute]
   * property of the source widget.
   *
   *
   */
  obj_props[PROP_TARGET] =
    g_param_spec_object ("target", NULL, NULL,
                         BOBGUI_TYPE_CONSTRAINT_TARGET,
                         G_PARAM_READWRITE |
                         G_PARAM_STATIC_STRINGS |
                         G_PARAM_CONSTRUCT_ONLY);

  /**
   * BobguiConstraint:target-attribute:
   *
   * The attribute of the [property@Bobgui.Constraint:target] set by the constraint.
   */
  obj_props[PROP_TARGET_ATTRIBUTE] =
    g_param_spec_enum ("target-attribute", NULL, NULL,
                       BOBGUI_TYPE_CONSTRAINT_ATTRIBUTE,
                       BOBGUI_CONSTRAINT_ATTRIBUTE_NONE,
                       G_PARAM_READWRITE |
                       G_PARAM_STATIC_STRINGS |
                       G_PARAM_CONSTRUCT_ONLY);

  /**
   * BobguiConstraint:relation:
   *
   * The order relation between the terms of the constraint.
   */
  obj_props[PROP_RELATION] =
    g_param_spec_enum ("relation", NULL, NULL,
                       BOBGUI_TYPE_CONSTRAINT_RELATION,
                       BOBGUI_CONSTRAINT_RELATION_EQ,
                       G_PARAM_READWRITE |
                       G_PARAM_STATIC_STRINGS |
                       G_PARAM_CONSTRUCT_ONLY);

  /**
   * BobguiConstraint:source:
   *
   * The source of the constraint.
   *
   * The constraint will set the [property@Bobgui.Constraint:target-attribute]
   * property of the target using the [property@Bobgui.Constraint:source-attribute]
   * property of the source.
   */
  obj_props[PROP_SOURCE] =
    g_param_spec_object ("source", NULL, NULL,
                         BOBGUI_TYPE_CONSTRAINT_TARGET,
                         G_PARAM_READWRITE |
                         G_PARAM_STATIC_STRINGS |
                         G_PARAM_CONSTRUCT_ONLY);
  /**
   * BobguiConstraint:source-attribute:
   *
   * The attribute of the [property@Bobgui.Constraint:source] read by the
   * constraint.
   */
  obj_props[PROP_SOURCE_ATTRIBUTE] =
    g_param_spec_enum ("source-attribute", NULL, NULL,
                       BOBGUI_TYPE_CONSTRAINT_ATTRIBUTE,
                       BOBGUI_CONSTRAINT_ATTRIBUTE_NONE,
                       G_PARAM_READWRITE |
                       G_PARAM_STATIC_STRINGS |
                       G_PARAM_CONSTRUCT_ONLY);

  /**
   * BobguiConstraint:multiplier:
   *
   * The multiplication factor to be applied to
   * the [property@Bobgui.Constraint:source-attribute].
   */
  obj_props[PROP_MULTIPLIER] =
    g_param_spec_double ("multiplier", NULL, NULL,
                         -G_MAXDOUBLE, G_MAXDOUBLE, 1.0,
                         G_PARAM_READWRITE |
                         G_PARAM_STATIC_STRINGS |
                         G_PARAM_CONSTRUCT_ONLY);

  /**
   * BobguiConstraint:constant:
   *
   * The constant value to be added to the [property@Bobgui.Constraint:source-attribute].
   */
  obj_props[PROP_CONSTANT] =
    g_param_spec_double ("constant", NULL, NULL,
                         -G_MAXDOUBLE, G_MAXDOUBLE, 0.0,
                         G_PARAM_READWRITE |
                         G_PARAM_STATIC_STRINGS |
                         G_PARAM_CONSTRUCT_ONLY);

  /**
   * BobguiConstraint:strength:
   *
   * The strength of the constraint.
   *
   * The strength can be expressed either using one of the symbolic values
   * of the [enum@Bobgui.ConstraintStrength] enumeration, or any positive integer
   * value.
   */
  obj_props[PROP_STRENGTH] =
    g_param_spec_int ("strength", NULL, NULL,
                      0, BOBGUI_CONSTRAINT_STRENGTH_REQUIRED,
                      BOBGUI_CONSTRAINT_STRENGTH_REQUIRED,
                      G_PARAM_READWRITE |
                      G_PARAM_STATIC_STRINGS |
                      G_PARAM_CONSTRUCT_ONLY);

  g_object_class_install_properties (gobject_class, N_PROPERTIES, obj_props);
}

static void
bobgui_constraint_init (BobguiConstraint *self)
{
  self->multiplier = 1.0;
  self->constant = 0.0;

  self->target_attribute = BOBGUI_CONSTRAINT_ATTRIBUTE_NONE;
  self->source_attribute = BOBGUI_CONSTRAINT_ATTRIBUTE_NONE;
  self->relation = BOBGUI_CONSTRAINT_RELATION_EQ;
  self->strength = BOBGUI_CONSTRAINT_STRENGTH_REQUIRED;
}

/**
 * bobgui_constraint_new: (constructor)
 * @target: (nullable) (type BobguiConstraintTarget): the target of the constraint
 * @target_attribute: the attribute of `target` to be set
 * @relation: the relation equivalence between `target_attribute` and `source_attribute`
 * @source: (nullable) (type BobguiConstraintTarget): the source of the constraint
 * @source_attribute: the attribute of `source` to be read
 * @multiplier: a multiplication factor to be applied to `source_attribute`
 * @constant: a constant factor to be added to `source_attribute`
 * @strength: the strength of the constraint
 *
 * Creates a new constraint representing a relation between a layout
 * attribute on a source and a layout attribute on a target.
 *
 * Returns: (transfer full): the newly created constraint
 */
BobguiConstraint *
bobgui_constraint_new (gpointer                target,
                    BobguiConstraintAttribute  target_attribute,
                    BobguiConstraintRelation   relation,
                    gpointer                source,
                    BobguiConstraintAttribute  source_attribute,
                    double                  multiplier,
                    double                  constant,
                    int                     strength)
{
  g_return_val_if_fail (target == NULL || BOBGUI_IS_CONSTRAINT_TARGET (target), NULL);
  g_return_val_if_fail (source == NULL || BOBGUI_IS_CONSTRAINT_TARGET (source), NULL);

  return g_object_new (BOBGUI_TYPE_CONSTRAINT,
                       "target", target,
                       "target-attribute", target_attribute,
                       "relation", relation,
                       "source", source,
                       "source-attribute", source_attribute,
                       "multiplier", multiplier,
                       "constant", constant,
                       "strength", strength,
                       NULL);
}

/**
 * bobgui_constraint_new_constant: (constructor)
 * @target: (nullable) (type BobguiConstraintTarget): a the target of the constraint
 * @target_attribute: the attribute of `target` to be set
 * @relation: the relation equivalence between `target_attribute` and `constant`
 * @constant: a constant factor to be set on `target_attribute`
 * @strength: the strength of the constraint
 *
 * Creates a new constraint representing a relation between a layout
 * attribute on a target and a constant value.
 *
 * Returns: (transfer full): the newly created constraint
 */
BobguiConstraint *
bobgui_constraint_new_constant (gpointer                target,
                             BobguiConstraintAttribute  target_attribute,
                             BobguiConstraintRelation   relation,
                             double                  constant,
                             int                     strength)
{
  g_return_val_if_fail (target == NULL || BOBGUI_IS_CONSTRAINT_TARGET (target), NULL);

  return g_object_new (BOBGUI_TYPE_CONSTRAINT,
                       "target", target,
                       "target-attribute", target_attribute,
                       "relation", relation,
                       "source-attribute", BOBGUI_CONSTRAINT_ATTRIBUTE_NONE,
                       "constant", constant,
                       "strength", strength,
                       NULL);
}

/**
 * bobgui_constraint_get_target:
 * @constraint: a `BobguiConstraint`
 *
 * Retrieves the [iface@Bobgui.ConstraintTarget] used as the target for
 * the constraint.
 *
 * If the targe is set to `NULL` at creation, the constraint will use
 * the widget using the [class@Bobgui.ConstraintLayout] as the target.
 *
 * Returns: (transfer none) (nullable): a `BobguiConstraintTarget`
 */
BobguiConstraintTarget *
bobgui_constraint_get_target (BobguiConstraint *constraint)
{
  g_return_val_if_fail (BOBGUI_IS_CONSTRAINT (constraint), NULL);

  return constraint->target;
}

/**
 * bobgui_constraint_get_target_attribute:
 * @constraint: a `BobguiConstraint`
 *
 * Retrieves the attribute of the target to be set by the constraint.
 *
 * Returns: the target's attribute
 */
BobguiConstraintAttribute
bobgui_constraint_get_target_attribute (BobguiConstraint *constraint)
{
  g_return_val_if_fail (BOBGUI_IS_CONSTRAINT (constraint), BOBGUI_CONSTRAINT_ATTRIBUTE_NONE);

  return constraint->target_attribute;
}

/**
 * bobgui_constraint_get_source:
 * @constraint: a `BobguiConstraint`
 *
 * Retrieves the [iface@Bobgui.ConstraintTarget] used as the source for the
 * constraint.
 *
 * If the source is set to `NULL` at creation, the constraint will use
 * the widget using the [class@Bobgui.ConstraintLayout] as the source.
 *
 * Returns: (transfer none) (nullable): the source of the constraint
 */
BobguiConstraintTarget *
bobgui_constraint_get_source (BobguiConstraint *constraint)
{
  g_return_val_if_fail (BOBGUI_IS_CONSTRAINT (constraint), NULL);

  return constraint->source;
}

/**
 * bobgui_constraint_get_source_attribute:
 * @constraint: a `BobguiConstraint`
 *
 * Retrieves the attribute of the source to be read by the constraint.
 *
 * Returns: the source's attribute
 */
BobguiConstraintAttribute
bobgui_constraint_get_source_attribute (BobguiConstraint *constraint)
{
  g_return_val_if_fail (BOBGUI_IS_CONSTRAINT (constraint), BOBGUI_CONSTRAINT_ATTRIBUTE_NONE);

  return constraint->source_attribute;
}

/**
 * bobgui_constraint_get_relation:
 * @constraint: a `BobguiConstraint`
 *
 * The order relation between the terms of the constraint.
 *
 * Returns: a relation type
 */
BobguiConstraintRelation
bobgui_constraint_get_relation (BobguiConstraint *constraint)
{
  g_return_val_if_fail (BOBGUI_IS_CONSTRAINT (constraint), BOBGUI_CONSTRAINT_RELATION_EQ);

  return constraint->relation;
}

/**
 * bobgui_constraint_get_multiplier:
 * @constraint: a `BobguiConstraint`
 *
 * Retrieves the multiplication factor applied to the source
 * attribute's value.
 *
 * Returns: a multiplication factor
 */
double
bobgui_constraint_get_multiplier (BobguiConstraint *constraint)
{
  g_return_val_if_fail (BOBGUI_IS_CONSTRAINT (constraint), 1.0);

  return constraint->multiplier;
}

/**
 * bobgui_constraint_get_constant:
 * @constraint: a `BobguiConstraint`
 *
 * Retrieves the constant factor added to the source attributes' value.
 *
 * Returns: a constant factor
 */
double
bobgui_constraint_get_constant (BobguiConstraint *constraint)
{
  g_return_val_if_fail (BOBGUI_IS_CONSTRAINT (constraint), 0.0);

  return constraint->constant;
}

/**
 * bobgui_constraint_get_strength:
 * @constraint: a `BobguiConstraint`
 *
 * Retrieves the strength of the constraint.
 *
 * Returns: the strength value
 */
int
bobgui_constraint_get_strength (BobguiConstraint *constraint)
{
  g_return_val_if_fail (BOBGUI_IS_CONSTRAINT (constraint), BOBGUI_CONSTRAINT_STRENGTH_REQUIRED);

  return constraint->strength;
}

/**
 * bobgui_constraint_is_required:
 * @constraint: a `BobguiConstraint`
 *
 * Checks whether the constraint is a required relation for solving the
 * constraint layout.
 *
 * Returns: %TRUE if the constraint is required
 */
gboolean
bobgui_constraint_is_required (BobguiConstraint *constraint)
{
  g_return_val_if_fail (BOBGUI_IS_CONSTRAINT (constraint), FALSE);

  return constraint->strength == BOBGUI_CONSTRAINT_STRENGTH_REQUIRED;
}

/**
 * bobgui_constraint_is_attached:
 * @constraint: a `BobguiConstraint`
 *
 * Checks whether the constraint is attached to a [class@Bobgui.ConstraintLayout],
 * and it is contributing to the layout.
 *
 * Returns: `TRUE` if the constraint is attached
 */
gboolean
bobgui_constraint_is_attached (BobguiConstraint *constraint)
{
  g_return_val_if_fail (BOBGUI_IS_CONSTRAINT (constraint), FALSE);

  return constraint->constraint_ref != NULL;
}

/**
 * bobgui_constraint_is_constant:
 * @constraint: a `BobguiConstraint`
 *
 * Checks whether the constraint describes a relation between an attribute
 * on the [property@Bobgui.Constraint:target] and a constant value.
 *
 * Returns: `TRUE` if the constraint is a constant relation
 */
gboolean
bobgui_constraint_is_constant (BobguiConstraint *constraint)
{
  g_return_val_if_fail (BOBGUI_IS_CONSTRAINT (constraint), FALSE);

  return constraint->source == NULL &&
         constraint->source_attribute == BOBGUI_CONSTRAINT_ATTRIBUTE_NONE;
}

void
bobgui_constraint_attach (BobguiConstraint       *constraint,
                       BobguiConstraintSolver *solver,
                       BobguiConstraintRef    *ref)
{
  g_return_if_fail (BOBGUI_IS_CONSTRAINT (constraint));
  g_return_if_fail (BOBGUI_IS_CONSTRAINT_SOLVER (solver));
  g_return_if_fail (ref != NULL);

  constraint->constraint_ref = ref;
  constraint->solver = solver;
}

void
bobgui_constraint_detach (BobguiConstraint *constraint)
{
  g_return_if_fail (BOBGUI_IS_CONSTRAINT (constraint));

  if (constraint->constraint_ref == NULL)
    return;

  bobgui_constraint_solver_remove_constraint (constraint->solver, constraint->constraint_ref);
  constraint->constraint_ref = NULL;
  constraint->solver = NULL;
}

typedef struct _BobguiConstraintTargetInterface BobguiConstraintTargetInterface;

struct _BobguiConstraintTargetInterface
{
  GTypeInterface g_iface;
};

G_DEFINE_INTERFACE (BobguiConstraintTarget, bobgui_constraint_target, G_TYPE_OBJECT)

static void
bobgui_constraint_target_default_init (BobguiConstraintTargetInterface *iface)
{
}
