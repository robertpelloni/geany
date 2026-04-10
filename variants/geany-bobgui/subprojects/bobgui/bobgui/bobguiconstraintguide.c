/* bobguiconstraintguide.c: Flexible space for constraints
 * Copyright 2019 Red Hat, Inc.
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
 * Author: Matthias Clasen
 */

/**
 * BobguiConstraintGuide:
 *
 * An invisible layout element in a `BobguiConstraintLayout`.
 *
 * The `BobguiConstraintLayout` treats guides like widgets. They
 * can be used as the source or target of a `BobguiConstraint`.
 *
 * Guides have a minimum, maximum and natural size. Depending
 * on the constraints that are applied, they can act like a
 * guideline that widgets can be aligned to, or like *flexible
 * space*.
 *
 * Unlike a `BobguiWidget`, a `BobguiConstraintGuide` will not be drawn.
 */

#include "config.h"

#include "bobguiconstraintguide.h"

#include "bobguiconstraintguideprivate.h"
#include "bobguiconstraintlayoutprivate.h"
#include "bobguiconstraintexpressionprivate.h"
#include "bobguiconstraintsolverprivate.h"

#include "bobguidebug.h"
#include "bobguiprivate.h"


typedef enum {
  MIN_WIDTH,
  MIN_HEIGHT,
  NAT_WIDTH,
  NAT_HEIGHT,
  MAX_WIDTH,
  MAX_HEIGHT,
  LAST_VALUE
} GuideValue;

struct _BobguiConstraintGuide
{ 
  GObject parent_instance;

  char *name;

  int strength;

  int values[LAST_VALUE];

  BobguiConstraintLayout *layout;

  /* HashTable<static string, Variable>; a hash table of variables,
   * one for each attribute; we use these to query and suggest the
   * values for the solver. The string is static and does not need
   * to be freed.
   */
  GHashTable *bound_attributes;

  BobguiConstraintRef *constraints[LAST_VALUE];
};


struct _BobguiConstraintGuideClass {
  GObjectClass parent_class;
};

enum {
  PROP_MIN_WIDTH = 1,
  PROP_MIN_HEIGHT,
  PROP_NAT_WIDTH,
  PROP_NAT_HEIGHT,
  PROP_MAX_WIDTH,
  PROP_MAX_HEIGHT,
  PROP_STRENGTH,
  PROP_NAME,
  LAST_PROP
};

static GParamSpec *guide_props[LAST_PROP];

static void
bobgui_constraint_guide_constraint_target_iface_init (BobguiConstraintTargetInterface *iface)
{
}

G_DEFINE_TYPE_WITH_CODE (BobguiConstraintGuide, bobgui_constraint_guide, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (BOBGUI_TYPE_CONSTRAINT_TARGET,
                                                bobgui_constraint_guide_constraint_target_iface_init))

static void
bobgui_constraint_guide_init (BobguiConstraintGuide *guide)
{
  guide->strength = BOBGUI_CONSTRAINT_STRENGTH_MEDIUM;

  guide->values[MIN_WIDTH] = 0;
  guide->values[MIN_HEIGHT] = 0;
  guide->values[NAT_WIDTH] = 0;
  guide->values[NAT_HEIGHT] = 0;
  guide->values[MAX_WIDTH] = G_MAXINT;
  guide->values[MAX_HEIGHT] = G_MAXINT;

  guide->bound_attributes =
    g_hash_table_new_full (g_str_hash, g_str_equal,
                           NULL,
                           (GDestroyNotify) bobgui_constraint_variable_unref);
}

static void
bobgui_constraint_guide_update_constraint (BobguiConstraintGuide *guide,
                                        GuideValue          index)
{
  BobguiConstraintSolver *solver;
  BobguiConstraintVariable *var;

  if (!guide->layout)
    return;

  solver = bobgui_constraint_layout_get_solver (guide->layout);
  if (!solver)
    return;

  if (guide->constraints[index] != NULL)
    {
      bobgui_constraint_solver_remove_constraint (solver, guide->constraints[index]);
      guide->constraints[index] = NULL;
    }

  if (index == MIN_WIDTH || index == NAT_WIDTH || index == MAX_WIDTH)
    var = bobgui_constraint_layout_get_attribute (guide->layout, BOBGUI_CONSTRAINT_ATTRIBUTE_WIDTH, "guide", NULL, guide->bound_attributes);
  else
    var = bobgui_constraint_layout_get_attribute (guide->layout, BOBGUI_CONSTRAINT_ATTRIBUTE_HEIGHT, "guide", NULL, guide->bound_attributes);

  /* We always install min-size constraints,
   * but we avoid nat-size constraints if min == max
   * and we avoid max-size constraints if max == G_MAXINT
   */
  if (index == MIN_WIDTH || index == MIN_HEIGHT)
    {
      guide->constraints[index] =
        bobgui_constraint_solver_add_constraint (solver,
                                              var,
                                              BOBGUI_CONSTRAINT_RELATION_GE,
                                              bobgui_constraint_expression_new (guide->values[index]),
                                              BOBGUI_CONSTRAINT_STRENGTH_REQUIRED);
    }
  else if ((index == NAT_WIDTH && guide->values[MIN_WIDTH] != guide->values[MAX_WIDTH]) ||
      (index == NAT_HEIGHT && guide->values[MIN_HEIGHT] != guide->values[MAX_HEIGHT]))
    {
      bobgui_constraint_variable_set_value (var, guide->values[index]);
      guide->constraints[index] =
        bobgui_constraint_solver_add_stay_variable (solver,
                                                 var,
                                                 guide->strength);
    }
  else if ((index == MAX_WIDTH || index == MAX_HEIGHT) &&
           guide->values[index] < G_MAXINT)
    {
      guide->constraints[index] =
        bobgui_constraint_solver_add_constraint (solver,
                                              var,
                                              BOBGUI_CONSTRAINT_RELATION_LE,
                                              bobgui_constraint_expression_new (guide->values[index]),
                                              BOBGUI_CONSTRAINT_STRENGTH_REQUIRED);
    }

  bobgui_layout_manager_layout_changed (BOBGUI_LAYOUT_MANAGER (guide->layout));
}

void
bobgui_constraint_guide_update (BobguiConstraintGuide *guide)
{
  int i;

  for (i = 0; i < LAST_VALUE; i++)
    bobgui_constraint_guide_update_constraint (guide, i);
}

void
bobgui_constraint_guide_detach (BobguiConstraintGuide *guide)
{
  BobguiConstraintSolver *solver;
  int i;

  if (!guide->layout)
    return;

  solver = bobgui_constraint_layout_get_solver (guide->layout);
  if (!solver)
    return;

  for (i = 0; i < LAST_VALUE; i++)
    {
      if (guide->constraints[i])
        {
          bobgui_constraint_solver_remove_constraint (solver, guide->constraints[i]);
          guide->constraints[i] = NULL;
        }
    }

  g_hash_table_remove_all (guide->bound_attributes);
}

BobguiConstraintVariable *
bobgui_constraint_guide_get_attribute (BobguiConstraintGuide     *guide,
                                    BobguiConstraintAttribute  attr)
{
  BobguiLayoutManager *manager = BOBGUI_LAYOUT_MANAGER (guide->layout);
  BobguiWidget *widget = bobgui_layout_manager_get_widget (manager);

  return bobgui_constraint_layout_get_attribute (guide->layout, attr, "guide", widget, guide->bound_attributes);
}

BobguiConstraintLayout *
bobgui_constraint_guide_get_layout (BobguiConstraintGuide *guide)
{
  return guide->layout;
}

void
bobgui_constraint_guide_set_layout (BobguiConstraintGuide  *guide,
                                 BobguiConstraintLayout *layout)
{
  guide->layout = layout;
}

static void
bobgui_constraint_guide_set_property (GObject      *gobject,
                                   guint         prop_id,
                                   const GValue *value,
                                   GParamSpec   *pspec)
{
  BobguiConstraintGuide *self = BOBGUI_CONSTRAINT_GUIDE (gobject);
  int val;
  GuideValue index;

  switch (prop_id)
    {
    case PROP_MIN_WIDTH:
    case PROP_MIN_HEIGHT:
    case PROP_NAT_WIDTH:
    case PROP_NAT_HEIGHT:
    case PROP_MAX_WIDTH:
    case PROP_MAX_HEIGHT:
      val = g_value_get_int (value);
      index = prop_id - 1;
      if (self->values[index] != val)
        {
          self->values[index] = val;
          g_object_notify_by_pspec (gobject, pspec);

          bobgui_constraint_guide_update_constraint (self, index);
          if (index == MIN_WIDTH || index == MAX_WIDTH)
            bobgui_constraint_guide_update_constraint (self, NAT_WIDTH);
          if (index == MIN_HEIGHT || index == MAX_HEIGHT)
            bobgui_constraint_guide_update_constraint (self, NAT_HEIGHT);
        }
      break;

    case PROP_STRENGTH:
      bobgui_constraint_guide_set_strength (self, g_value_get_enum (value));
      break;

    case PROP_NAME:
      bobgui_constraint_guide_set_name (self, g_value_get_string (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
    }
}

static void
bobgui_constraint_guide_get_property (GObject    *gobject,
                                   guint       prop_id,
                                   GValue     *value,
                                   GParamSpec *pspec)
{
  BobguiConstraintGuide *self = BOBGUI_CONSTRAINT_GUIDE (gobject);

  switch (prop_id)
    {
    case PROP_MIN_WIDTH:
    case PROP_MIN_HEIGHT:
    case PROP_NAT_WIDTH:
    case PROP_NAT_HEIGHT:
    case PROP_MAX_WIDTH:
    case PROP_MAX_HEIGHT:
      g_value_set_int (value, self->values[prop_id - 1]);
      break;

    case PROP_STRENGTH:
      g_value_set_enum (value, self->strength);
      break;

    case PROP_NAME:
      g_value_set_string (value, self->name);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
    }
}

static void
bobgui_constraint_guide_finalize (GObject *object)
{
  BobguiConstraintGuide *self = BOBGUI_CONSTRAINT_GUIDE (object);

  g_free (self->name);

  g_clear_pointer (&self->bound_attributes, g_hash_table_unref);

  G_OBJECT_CLASS (bobgui_constraint_guide_parent_class)->finalize (object);
}

static void
bobgui_constraint_guide_class_init (BobguiConstraintGuideClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  object_class->finalize = bobgui_constraint_guide_finalize;
  object_class->set_property = bobgui_constraint_guide_set_property;
  object_class->get_property = bobgui_constraint_guide_get_property;

  /**
   * BobguiConstraintGuide:min-width:
   *
   * The minimum width of the guide.
   */
  guide_props[PROP_MIN_WIDTH] =
      g_param_spec_int ("min-width", NULL, NULL,
                        0, G_MAXINT, 0,
                        G_PARAM_READWRITE|
                        G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiConstraintGuide:min-height:
   *
   * The minimum height of the guide.
   */
  guide_props[PROP_MIN_HEIGHT] =
      g_param_spec_int ("min-height", NULL, NULL,
                        0, G_MAXINT, 0,
                        G_PARAM_READWRITE|
                        G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiConstraintGuide:nat-width:
   *
   * The preferred, or natural, width of the guide.
   */
  guide_props[PROP_NAT_WIDTH] =
      g_param_spec_int ("nat-width", NULL, NULL,
                        0, G_MAXINT, 0,
                        G_PARAM_READWRITE|
                        G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiConstraintGuide:nat-height:
   *
   * The preferred, or natural, height of the guide.
   */
  guide_props[PROP_NAT_HEIGHT] =
      g_param_spec_int ("nat-height", NULL, NULL,
                        0, G_MAXINT, 0,
                        G_PARAM_READWRITE|
                        G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiConstraintGuide:max-width:
   *
   * The maximum width of the guide.
   */
  guide_props[PROP_MAX_WIDTH] =
      g_param_spec_int ("max-width", NULL, NULL,
                        0, G_MAXINT, G_MAXINT,
                        G_PARAM_READWRITE|
                        G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiConstraintGuide:max-height:
   *
   * The maximum height of the guide.
   */
  guide_props[PROP_MAX_HEIGHT] =
      g_param_spec_int ("max-height", NULL, NULL,
                        0, G_MAXINT, G_MAXINT,
                        G_PARAM_READWRITE|
                        G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiConstraintGuide:strength:
   *
   * The `BobguiConstraintStrength` to be used for the constraint on
   * the natural size of the guide.
   */
  guide_props[PROP_STRENGTH] =
      g_param_spec_enum ("strength", NULL, NULL,
                         BOBGUI_TYPE_CONSTRAINT_STRENGTH,
                         BOBGUI_CONSTRAINT_STRENGTH_MEDIUM,
                         G_PARAM_READWRITE|
                         G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiConstraintGuide:name:
   *
   * A name that identifies the `BobguiConstraintGuide`, for debugging.
   */
  guide_props[PROP_NAME] =
      g_param_spec_string ("name", NULL, NULL,
                           NULL,
                           G_PARAM_READWRITE);

  g_object_class_install_properties (object_class, LAST_PROP, guide_props);
}

/**
 * bobgui_constraint_guide_new:
 *
 * Creates a new `BobguiConstraintGuide` object.
 *
 * Return: a new `BobguiConstraintGuide` object.
 */
BobguiConstraintGuide *
bobgui_constraint_guide_new (void)
{
  return g_object_new (BOBGUI_TYPE_CONSTRAINT_GUIDE, NULL);
}

/**
 * bobgui_constraint_guide_set_min_size:
 * @guide: a `BobguiConstraintGuide` object
 * @width: the new minimum width, or -1 to not change it
 * @height: the new minimum height, or -1 to not change it
 *
 * Sets the minimum size of @guide.
 *
 * If @guide is attached to a `BobguiConstraintLayout`,
 * the constraints will be updated to reflect the new size.
 */
void
bobgui_constraint_guide_set_min_size (BobguiConstraintGuide *guide,
                                   int                 width,
                                   int                 height)
{
  g_return_if_fail (BOBGUI_IS_CONSTRAINT_GUIDE (guide));
  g_return_if_fail (width >= -1);
  g_return_if_fail (height >= -1);

  g_object_freeze_notify (G_OBJECT (guide));

  if (width != -1)
    g_object_set (guide, "min-width", width, NULL);

  if (height != -1)
    g_object_set (guide, "min-height", height, NULL);

  g_object_thaw_notify (G_OBJECT (guide));
}

/**
 * bobgui_constraint_guide_get_min_size:
 * @guide: a `BobguiConstraintGuide` object
 * @width: (out) (optional): return location for the minimum width
 * @height: (out) (optional): return location for the minimum height
 *
 * Gets the minimum size of @guide.
 */
void
bobgui_constraint_guide_get_min_size (BobguiConstraintGuide *guide,
                                   int                *width,
                                   int                *height)
{
  g_return_if_fail (BOBGUI_IS_CONSTRAINT_GUIDE (guide));

  if (width)
    *width = guide->values[MIN_WIDTH];
  if (height)
    *height = guide->values[MIN_HEIGHT];
}

/**
 * bobgui_constraint_guide_set_nat_size:
 * @guide: a `BobguiConstraintGuide` object
 * @width: the new natural width, or -1 to not change it
 * @height: the new natural height, or -1 to not change it
 *
 * Sets the natural size of @guide.
 *
 * If @guide is attached to a `BobguiConstraintLayout`,
 * the constraints will be updated to reflect the new size.
 */
void
bobgui_constraint_guide_set_nat_size (BobguiConstraintGuide *guide,
                                   int                 width,
                                   int                 height)
{
  g_return_if_fail (BOBGUI_IS_CONSTRAINT_GUIDE (guide));
  g_return_if_fail (width >= -1);
  g_return_if_fail (height >= -1);

  g_object_freeze_notify (G_OBJECT (guide));

  if (width != -1)
    g_object_set (guide, "nat-width", width, NULL);

  if (height != -1)
    g_object_set (guide, "nat-height", height, NULL);

  g_object_thaw_notify (G_OBJECT (guide));
}

/**
 * bobgui_constraint_guide_get_nat_size:
 * @guide: a `BobguiConstraintGuide` object
 * @width: (out) (optional): return location for the natural width
 * @height: (out) (optional): return location for the natural height
 *
 * Gets the natural size of @guide.
 */
void
bobgui_constraint_guide_get_nat_size (BobguiConstraintGuide *guide,
                                   int                *width,
                                   int                *height)
{
  g_return_if_fail (BOBGUI_IS_CONSTRAINT_GUIDE (guide));

  if (width)
    *width = guide->values[NAT_WIDTH];
  if (height)
    *height = guide->values[NAT_HEIGHT];
}

/**
 * bobgui_constraint_guide_set_max_size:
 * @guide: a `BobguiConstraintGuide` object
 * @width: the new maximum width, or -1 to not change it
 * @height: the new maximum height, or -1 to not change it
 *
 * Sets the maximum size of @guide.
 *
 * If @guide is attached to a `BobguiConstraintLayout`,
 * the constraints will be updated to reflect the new size.
 */
void
bobgui_constraint_guide_set_max_size (BobguiConstraintGuide *guide,
                                   int                 width,
                                   int                 height)
{
  g_return_if_fail (BOBGUI_IS_CONSTRAINT_GUIDE (guide));
  g_return_if_fail (width >= -1);
  g_return_if_fail (height >= -1);

  g_object_freeze_notify (G_OBJECT (guide));

  if (width != -1)
    g_object_set (guide, "max-width", width, NULL);

  if (height != -1)
    g_object_set (guide, "max-height", height, NULL);

  g_object_thaw_notify (G_OBJECT (guide));
}

/**
 * bobgui_constraint_guide_get_max_size:
 * @guide: a `BobguiConstraintGuide` object
 * @width: (out) (optional): return location for the maximum width
 * @height: (out) (optional): return location for the maximum height
 *
 * Gets the maximum size of @guide.
 */
void
bobgui_constraint_guide_get_max_size (BobguiConstraintGuide *guide,
                                   int                *width,
                                   int                *height)
{
  g_return_if_fail (BOBGUI_IS_CONSTRAINT_GUIDE (guide));

  if (width)
    *width = guide->values[MAX_WIDTH];
  if (height)
    *height = guide->values[MAX_HEIGHT];
}

/**
 * bobgui_constraint_guide_get_name:
 * @guide: a `BobguiConstraintGuide`
 *
 * Retrieves the name set using bobgui_constraint_guide_set_name().
 *
 * Returns: (transfer none) (nullable): the name of the guide
 */
const char *
bobgui_constraint_guide_get_name (BobguiConstraintGuide *guide)
{
  g_return_val_if_fail (BOBGUI_IS_CONSTRAINT_GUIDE (guide), NULL);

  return guide->name;
}

/**
 * bobgui_constraint_guide_set_name:
 * @guide: a `BobguiConstraintGuide`
 * @name: (nullable): a name for the @guide
 *
 * Sets a name for the given `BobguiConstraintGuide`.
 *
 * The name is useful for debugging purposes.
 */
void
bobgui_constraint_guide_set_name (BobguiConstraintGuide *guide,
                               const char         *name)
{
  g_return_if_fail (BOBGUI_IS_CONSTRAINT_GUIDE (guide));

  g_free (guide->name);
  guide->name = g_strdup (name);
  g_object_notify_by_pspec (G_OBJECT (guide), guide_props[PROP_NAME]);
}

/**
 * bobgui_constraint_guide_get_strength:
 * @guide: a `BobguiConstraintGuide`
 *
 * Retrieves the strength set using bobgui_constraint_guide_set_strength().
 *
 * Returns: the strength of the constraint on the natural size
 */
BobguiConstraintStrength
bobgui_constraint_guide_get_strength (BobguiConstraintGuide *guide)
{
  g_return_val_if_fail (BOBGUI_IS_CONSTRAINT_GUIDE (guide),
                        BOBGUI_CONSTRAINT_STRENGTH_MEDIUM);

  return guide->strength;
}

/**
 * bobgui_constraint_guide_set_strength:
 * @guide: a `BobguiConstraintGuide`
 * @strength: the strength of the constraint
 *
 * Sets the strength of the constraint on the natural size of the
 * given `BobguiConstraintGuide`.
 */
void
bobgui_constraint_guide_set_strength (BobguiConstraintGuide    *guide,
                                   BobguiConstraintStrength  strength)
{
  g_return_if_fail (BOBGUI_IS_CONSTRAINT_GUIDE (guide));

  if (guide->strength == strength)
    return;

  guide->strength = strength;
  g_object_notify_by_pspec (G_OBJECT (guide), guide_props[PROP_STRENGTH]);
  bobgui_constraint_guide_update_constraint (guide, NAT_WIDTH);
  bobgui_constraint_guide_update_constraint (guide, NAT_HEIGHT);
}
