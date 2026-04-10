/* BOBGUI - The Bobgui Framework
 * Copyright (C) 2011 Red Hat, Inc.
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

#include "bobguicsstransformvalueprivate.h"

#include <math.h>
#include <string.h>

#include "bobguicssnumbervalueprivate.h"
#include "gsktransform.h"

typedef union _BobguiCssTransform BobguiCssTransform;

typedef enum {
  BOBGUI_CSS_TRANSFORM_NONE,
  BOBGUI_CSS_TRANSFORM_MATRIX,
  BOBGUI_CSS_TRANSFORM_TRANSLATE,
  BOBGUI_CSS_TRANSFORM_ROTATE,
  BOBGUI_CSS_TRANSFORM_SCALE,
  BOBGUI_CSS_TRANSFORM_SKEW,
  BOBGUI_CSS_TRANSFORM_SKEW_X,
  BOBGUI_CSS_TRANSFORM_SKEW_Y,
  BOBGUI_CSS_TRANSFORM_PERSPECTIVE
} BobguiCssTransformType;

union _BobguiCssTransform {
  BobguiCssTransformType type;
  struct {
    BobguiCssTransformType type;
    graphene_matrix_t   matrix;
  }                   matrix;
  struct {
    BobguiCssTransformType type;
    BobguiCssValue        *x;
    BobguiCssValue        *y;
    BobguiCssValue        *z;
  }                   translate, scale;
  struct {
    BobguiCssTransformType type;
    BobguiCssValue        *x;
    BobguiCssValue        *y;
  }                   skew;
  struct {
    BobguiCssTransformType type;
    BobguiCssValue        *x;
    BobguiCssValue        *y;
    BobguiCssValue        *z;
    BobguiCssValue        *angle;
  }                   rotate;
  struct {
    BobguiCssTransformType type;
    BobguiCssValue        *skew;
  }                   skew_x, skew_y;
  struct {
    BobguiCssTransformType type;
    BobguiCssValue        *depth;
  }                   perspective;
};

struct _BobguiCssValue {
  BOBGUI_CSS_VALUE_BASE
  guint             n_transforms;
  BobguiCssTransform   transforms[1];
};

static BobguiCssValue *    bobgui_css_transform_value_alloc           (guint                  n_values);
static gboolean         bobgui_css_transform_value_is_none         (const BobguiCssValue     *value);

static void
bobgui_css_transform_clear (BobguiCssTransform *transform)
{
  switch (transform->type)
    {
    case BOBGUI_CSS_TRANSFORM_MATRIX:
      break;
    case BOBGUI_CSS_TRANSFORM_TRANSLATE:
      bobgui_css_value_unref (transform->translate.x);
      bobgui_css_value_unref (transform->translate.y);
      bobgui_css_value_unref (transform->translate.z);
      break;
    case BOBGUI_CSS_TRANSFORM_ROTATE:
      bobgui_css_value_unref (transform->rotate.x);
      bobgui_css_value_unref (transform->rotate.y);
      bobgui_css_value_unref (transform->rotate.z);
      bobgui_css_value_unref (transform->rotate.angle);
      break;
    case BOBGUI_CSS_TRANSFORM_SCALE:
      bobgui_css_value_unref (transform->scale.x);
      bobgui_css_value_unref (transform->scale.y);
      bobgui_css_value_unref (transform->scale.z);
      break;
    case BOBGUI_CSS_TRANSFORM_SKEW:
      bobgui_css_value_unref (transform->skew.x);
      bobgui_css_value_unref (transform->skew.y);
      break;
    case BOBGUI_CSS_TRANSFORM_SKEW_X:
      bobgui_css_value_unref (transform->skew_x.skew);
      break;
    case BOBGUI_CSS_TRANSFORM_SKEW_Y:
      bobgui_css_value_unref (transform->skew_y.skew);
      break;
    case BOBGUI_CSS_TRANSFORM_PERSPECTIVE:
      bobgui_css_value_unref (transform->perspective.depth);
      break;
    case BOBGUI_CSS_TRANSFORM_NONE:
    default:
      g_assert_not_reached ();
      break;
    }
}

static gboolean
bobgui_css_transform_init_identity (BobguiCssTransform     *transform,
                                 BobguiCssTransformType  type)
{
  switch (type)
    {
    case BOBGUI_CSS_TRANSFORM_MATRIX:
      graphene_matrix_init_identity (&transform->matrix.matrix);
      break;
    case BOBGUI_CSS_TRANSFORM_TRANSLATE:
      transform->translate.x = bobgui_css_number_value_new (0, BOBGUI_CSS_PX);
      transform->translate.y = bobgui_css_number_value_new (0, BOBGUI_CSS_PX);
      transform->translate.z = bobgui_css_number_value_new (0, BOBGUI_CSS_PX);
      break;
    case BOBGUI_CSS_TRANSFORM_ROTATE:
      transform->rotate.x = bobgui_css_number_value_new (0, BOBGUI_CSS_NUMBER);
      transform->rotate.y = bobgui_css_number_value_new (0, BOBGUI_CSS_NUMBER);
      transform->rotate.z = bobgui_css_number_value_new (1, BOBGUI_CSS_NUMBER);
      transform->rotate.angle = bobgui_css_number_value_new (0, BOBGUI_CSS_DEG);
      break;
    case BOBGUI_CSS_TRANSFORM_SCALE:
      transform->scale.x = bobgui_css_number_value_new (1, BOBGUI_CSS_NUMBER);
      transform->scale.y = bobgui_css_number_value_new (1, BOBGUI_CSS_NUMBER);
      transform->scale.z = bobgui_css_number_value_new (1, BOBGUI_CSS_NUMBER);
      break;
    case BOBGUI_CSS_TRANSFORM_SKEW:
      transform->skew.x = bobgui_css_number_value_new (0, BOBGUI_CSS_DEG);
      transform->skew.y = bobgui_css_number_value_new (0, BOBGUI_CSS_DEG);
      break;
    case BOBGUI_CSS_TRANSFORM_SKEW_X:
      transform->skew_x.skew = bobgui_css_number_value_new (0, BOBGUI_CSS_DEG);
      break;
    case BOBGUI_CSS_TRANSFORM_SKEW_Y:
      transform->skew_y.skew = bobgui_css_number_value_new (0, BOBGUI_CSS_DEG);
      break;
    case BOBGUI_CSS_TRANSFORM_PERSPECTIVE:
      return FALSE;

    case BOBGUI_CSS_TRANSFORM_NONE:
    default:
      g_assert_not_reached ();
      return FALSE;
    }

  transform->type = type;

  return TRUE;
}

static GskTransform *
bobgui_css_transform_apply (const BobguiCssTransform   *transform,
                         GskTransform            *next)
{
  graphene_matrix_t skew;

  switch (transform->type)
    {
    case BOBGUI_CSS_TRANSFORM_MATRIX:
      return gsk_transform_matrix (next, &transform->matrix.matrix);

    case BOBGUI_CSS_TRANSFORM_TRANSLATE:
      return gsk_transform_translate_3d (next,
                                         &GRAPHENE_POINT3D_INIT (
                                             bobgui_css_number_value_get (transform->translate.x, 100),
                                             bobgui_css_number_value_get (transform->translate.y, 100),
                                             bobgui_css_number_value_get (transform->translate.z, 100)
                                         ));

    case BOBGUI_CSS_TRANSFORM_ROTATE:
      {
        graphene_vec3_t axis;

        graphene_vec3_init (&axis,
                            bobgui_css_number_value_get (transform->rotate.x, 1),
                            bobgui_css_number_value_get (transform->rotate.y, 1),
                            bobgui_css_number_value_get (transform->rotate.z, 1));
        return gsk_transform_rotate_3d (next,
                                        bobgui_css_number_value_get (transform->rotate.angle, 100),
                                        &axis);
      }

    case BOBGUI_CSS_TRANSFORM_SCALE:
      return gsk_transform_scale_3d (next,
                                     bobgui_css_number_value_get (transform->scale.x, 1),
                                     bobgui_css_number_value_get (transform->scale.y, 1),
                                     bobgui_css_number_value_get (transform->scale.z, 1));

    case BOBGUI_CSS_TRANSFORM_SKEW:
      graphene_matrix_init_skew (&skew,
                                 bobgui_css_number_value_get (transform->skew.x, 100) / 180.0f * G_PI,
                                 bobgui_css_number_value_get (transform->skew.y, 100) / 180.0f * G_PI);
      return gsk_transform_matrix (next, &skew);

    case BOBGUI_CSS_TRANSFORM_SKEW_X:
      graphene_matrix_init_skew (&skew,
                                 bobgui_css_number_value_get (transform->skew_x.skew, 100) / 180.0f * G_PI,
                                 0);
      return gsk_transform_matrix (next, &skew);

    case BOBGUI_CSS_TRANSFORM_SKEW_Y:
      graphene_matrix_init_skew (&skew,
                                 0,
                                 bobgui_css_number_value_get (transform->skew_y.skew, 100) / 180.0f * G_PI);
      return gsk_transform_matrix (next, &skew);

    case BOBGUI_CSS_TRANSFORM_PERSPECTIVE:
      return gsk_transform_perspective (next,
                                        bobgui_css_number_value_get (transform->perspective.depth, 100));

    case BOBGUI_CSS_TRANSFORM_NONE:
    default:
      g_assert_not_reached ();
      break;
    }

  return NULL;
}

/* NB: The returned matrix may be invalid */
static GskTransform *
bobgui_css_transform_value_compute_transform (const BobguiCssValue *value)
{
  GskTransform *transform;
  guint i;

  transform = NULL;

  for (i = 0; i < value->n_transforms; i++)
    {
      transform = bobgui_css_transform_apply (&value->transforms[i], transform);
    }

  return transform;
}

static void
bobgui_css_value_transform_free (BobguiCssValue *value)
{
  guint i;

  for (i = 0; i < value->n_transforms; i++)
    bobgui_css_transform_clear (&value->transforms[i]);

  g_free (value);
}

/* returns TRUE if dest == src */
static gboolean
bobgui_css_transform_compute (BobguiCssTransform      *dest,
                           BobguiCssTransform      *src,
                           guint                 property_id,
                           BobguiCssComputeContext *context)
{
  dest->type = src->type;

  switch (src->type)
    {
    case BOBGUI_CSS_TRANSFORM_MATRIX:
      memcpy (dest, src, sizeof (BobguiCssTransform));
      return TRUE;
    case BOBGUI_CSS_TRANSFORM_TRANSLATE:
      dest->translate.x = bobgui_css_value_compute (src->translate.x, property_id, context);
      dest->translate.y = bobgui_css_value_compute (src->translate.y, property_id, context);
      dest->translate.z = bobgui_css_value_compute (src->translate.z, property_id, context);
      return dest->translate.x == src->translate.x
          && dest->translate.y == src->translate.y
          && dest->translate.z == src->translate.z;
    case BOBGUI_CSS_TRANSFORM_ROTATE:
      dest->rotate.x = bobgui_css_value_compute (src->rotate.x, property_id, context);
      dest->rotate.y = bobgui_css_value_compute (src->rotate.y, property_id, context);
      dest->rotate.z = bobgui_css_value_compute (src->rotate.z, property_id, context);
      dest->rotate.angle = bobgui_css_value_compute (src->rotate.angle, property_id, context);
      return dest->rotate.x == src->rotate.x
          && dest->rotate.y == src->rotate.y
          && dest->rotate.z == src->rotate.z
          && dest->rotate.angle == src->rotate.angle;
    case BOBGUI_CSS_TRANSFORM_SCALE:
      dest->scale.x = bobgui_css_value_compute (src->scale.x, property_id, context);
      dest->scale.y = bobgui_css_value_compute (src->scale.y, property_id, context);
      dest->scale.z = bobgui_css_value_compute (src->scale.z, property_id, context);
      return dest->scale.x == src->scale.x
          && dest->scale.y == src->scale.y
          && dest->scale.z == src->scale.z;
    case BOBGUI_CSS_TRANSFORM_SKEW:
      dest->skew.x = bobgui_css_value_compute (src->skew.x, property_id, context);
      dest->skew.y = bobgui_css_value_compute (src->skew.y, property_id, context);
      return dest->skew.x == src->skew.x
          && dest->skew.y == src->skew.y;
    case BOBGUI_CSS_TRANSFORM_SKEW_X:
      dest->skew_x.skew = bobgui_css_value_compute (src->skew_x.skew, property_id, context);
      return dest->skew_x.skew == src->skew_x.skew;
    case BOBGUI_CSS_TRANSFORM_SKEW_Y:
      dest->skew_y.skew = bobgui_css_value_compute (src->skew_y.skew, property_id, context);
      return dest->skew_y.skew == src->skew_y.skew;
    case BOBGUI_CSS_TRANSFORM_PERSPECTIVE:
      dest->perspective.depth = bobgui_css_value_compute (src->perspective.depth, property_id, context);
      return dest->perspective.depth == src->perspective.depth;
    case BOBGUI_CSS_TRANSFORM_NONE:
    default:
      g_assert_not_reached ();
      return FALSE;
    }
}

static BobguiCssValue *
bobgui_css_value_transform_compute (BobguiCssValue          *value,
                                 guint                 property_id,
                                 BobguiCssComputeContext *context)
{
  BobguiCssValue *result;
  gboolean changes;
  guint i;

  /* Special case the 99% case of "none" */
  if (bobgui_css_transform_value_is_none (value))
    return bobgui_css_value_ref (value);

  changes = FALSE;
  result = bobgui_css_transform_value_alloc (value->n_transforms);

  for (i = 0; i < value->n_transforms; i++)
    {
      changes |= !bobgui_css_transform_compute (&result->transforms[i],
                                             &value->transforms[i],
                                             property_id,
                                             context);
    }

  if (!changes)
    {
      bobgui_css_value_unref (result);
      result = bobgui_css_value_ref (value);
    }

  return result;
}

static gboolean
bobgui_css_transform_equal (const BobguiCssTransform *transform1,
                         const BobguiCssTransform *transform2)
{
  if (transform1->type != transform2->type)
    return FALSE;

  switch (transform1->type)
    {
    case BOBGUI_CSS_TRANSFORM_MATRIX:
      {
        guint i, j;

        for (i = 0; i < 4; i++)
          for (j = 0; j < 4; j++)
            {
              if (graphene_matrix_get_value (&transform1->matrix.matrix, i, j)
                  != graphene_matrix_get_value (&transform2->matrix.matrix, i, j))
                return FALSE;
            }
        return TRUE;
      }
    case BOBGUI_CSS_TRANSFORM_TRANSLATE:
      return bobgui_css_value_equal (transform1->translate.x, transform2->translate.x)
          && bobgui_css_value_equal (transform1->translate.y, transform2->translate.y)
          && bobgui_css_value_equal (transform1->translate.z, transform2->translate.z);
    case BOBGUI_CSS_TRANSFORM_ROTATE:
      return bobgui_css_value_equal (transform1->rotate.x, transform2->rotate.x)
          && bobgui_css_value_equal (transform1->rotate.y, transform2->rotate.y)
          && bobgui_css_value_equal (transform1->rotate.z, transform2->rotate.z)
          && bobgui_css_value_equal (transform1->rotate.angle, transform2->rotate.angle);
    case BOBGUI_CSS_TRANSFORM_SCALE:
      return bobgui_css_value_equal (transform1->scale.x, transform2->scale.x)
          && bobgui_css_value_equal (transform1->scale.y, transform2->scale.y)
          && bobgui_css_value_equal (transform1->scale.z, transform2->scale.z);
    case BOBGUI_CSS_TRANSFORM_SKEW:
      return bobgui_css_value_equal (transform1->skew.x, transform2->skew.x)
          && bobgui_css_value_equal (transform1->skew.y, transform2->skew.y);
    case BOBGUI_CSS_TRANSFORM_SKEW_X:
      return bobgui_css_value_equal (transform1->skew_x.skew, transform2->skew_x.skew);
    case BOBGUI_CSS_TRANSFORM_SKEW_Y:
      return bobgui_css_value_equal (transform1->skew_y.skew, transform2->skew_y.skew);
    case BOBGUI_CSS_TRANSFORM_PERSPECTIVE:
      return bobgui_css_value_equal (transform1->perspective.depth, transform2->perspective.depth);
    case BOBGUI_CSS_TRANSFORM_NONE:
    default:
      g_assert_not_reached ();
      return FALSE;
    }
}

static gboolean
bobgui_css_value_transform_equal (const BobguiCssValue *value1,
                               const BobguiCssValue *value2)
{
  const BobguiCssValue *larger;
  guint i, n;

  n = MIN (value1->n_transforms, value2->n_transforms);
  for (i = 0; i < n; i++)
    {
      if (!bobgui_css_transform_equal (&value1->transforms[i], &value2->transforms[i]))
        return FALSE;
    }

  larger = value1->n_transforms > value2->n_transforms ? value1 : value2;

  for (; i < larger->n_transforms; i++)
    {
      BobguiCssTransform transform;

      if (!bobgui_css_transform_init_identity (&transform, larger->transforms[i].type))
        return FALSE;

      if (!bobgui_css_transform_equal (&larger->transforms[i], &transform))
        {
          bobgui_css_transform_clear (&transform);
          return FALSE;
        }

      bobgui_css_transform_clear (&transform);
    }

  return TRUE;
}

static void
bobgui_css_transform_transition_default (BobguiCssTransform       *result,
                                      const BobguiCssTransform *start,
                                      const BobguiCssTransform *end,
                                      guint                  property_id,
                                      double                 progress)
{
  graphene_matrix_t start_mat, end_mat;
  GskTransform *trans;

  result->type = BOBGUI_CSS_TRANSFORM_MATRIX;

  if (start)
    trans = bobgui_css_transform_apply (start, NULL);
  else
    trans = NULL;
  gsk_transform_to_matrix (trans, &start_mat);
  gsk_transform_unref (trans);

  if (end)
    trans = bobgui_css_transform_apply (end, NULL);
  else
    trans = NULL;
  gsk_transform_to_matrix (trans, &end_mat);
  gsk_transform_unref (trans);

  graphene_matrix_interpolate (&start_mat,
                               &end_mat,
                               progress,
                               &result->matrix.matrix);
}

static void
bobgui_css_transform_transition (BobguiCssTransform       *result,
                              const BobguiCssTransform *start,
                              const BobguiCssTransform *end,
                              guint                  property_id,
                              double                 progress)
{
  result->type = start->type;

  switch (start->type)
    {
    case BOBGUI_CSS_TRANSFORM_MATRIX:
      graphene_matrix_interpolate (&start->matrix.matrix,
                                   &end->matrix.matrix,
                                   progress,
                                   &result->matrix.matrix);
      break;
    case BOBGUI_CSS_TRANSFORM_TRANSLATE:
      result->translate.x = bobgui_css_value_transition (start->translate.x, end->translate.x, property_id, progress);
      result->translate.y = bobgui_css_value_transition (start->translate.y, end->translate.y, property_id, progress);
      result->translate.z = bobgui_css_value_transition (start->translate.z, end->translate.z, property_id, progress);
      break;
    case BOBGUI_CSS_TRANSFORM_ROTATE:
      result->rotate.x = bobgui_css_value_transition (start->rotate.x, end->rotate.x, property_id, progress);
      result->rotate.y = bobgui_css_value_transition (start->rotate.y, end->rotate.y, property_id, progress);
      result->rotate.z = bobgui_css_value_transition (start->rotate.z, end->rotate.z, property_id, progress);
      result->rotate.angle = bobgui_css_value_transition (start->rotate.angle, end->rotate.angle, property_id, progress);
      break;
    case BOBGUI_CSS_TRANSFORM_SCALE:
      result->scale.x = bobgui_css_value_transition (start->scale.x, end->scale.x, property_id, progress);
      result->scale.y = bobgui_css_value_transition (start->scale.y, end->scale.y, property_id, progress);
      result->scale.z = bobgui_css_value_transition (start->scale.z, end->scale.z, property_id, progress);
      break;
    case BOBGUI_CSS_TRANSFORM_SKEW:
      result->skew.x = bobgui_css_value_transition (start->skew.x, end->skew.x, property_id, progress);
      result->skew.y = bobgui_css_value_transition (start->skew.y, end->skew.y, property_id, progress);
      break;
    case BOBGUI_CSS_TRANSFORM_SKEW_X:
      result->skew_x.skew = bobgui_css_value_transition (start->skew_x.skew, end->skew_x.skew, property_id, progress);
      break;
    case BOBGUI_CSS_TRANSFORM_SKEW_Y:
      result->skew_y.skew = bobgui_css_value_transition (start->skew_y.skew, end->skew_y.skew, property_id, progress);
      break;
    case BOBGUI_CSS_TRANSFORM_PERSPECTIVE:
      bobgui_css_transform_transition_default (result, start, end, property_id, progress);
      break;
    case BOBGUI_CSS_TRANSFORM_NONE:
    default:
      g_assert_not_reached ();
      break;
    }
}

static BobguiCssValue *
bobgui_css_value_transform_transition (BobguiCssValue *start,
                                    BobguiCssValue *end,
                                    guint        property_id,
                                    double       progress)
{
  BobguiCssValue *result;
  guint i, n;

  if (bobgui_css_transform_value_is_none (start))
    {
      if (bobgui_css_transform_value_is_none (end))
        return bobgui_css_value_ref (start);

      n = 0;
    }
  else if (bobgui_css_transform_value_is_none (end))
    {
      n = 0;
    }
  else
    {
      n = MIN (start->n_transforms, end->n_transforms);
    }

  /* Check transforms are compatible. If not, transition between
   * their result matrices.
   */
  for (i = 0; i < n; i++)
    {
      if (start->transforms[i].type != end->transforms[i].type)
        {
          GskTransform *transform;
          graphene_matrix_t start_matrix, end_matrix;

          transform = bobgui_css_transform_value_compute_transform (start);
          gsk_transform_to_matrix (transform, &start_matrix);
          gsk_transform_unref (transform);

          transform = bobgui_css_transform_value_compute_transform (end);
          gsk_transform_to_matrix (transform, &end_matrix);
          gsk_transform_unref (transform);

          result = bobgui_css_transform_value_alloc (1);
          result->transforms[0].type = BOBGUI_CSS_TRANSFORM_MATRIX;
          graphene_matrix_interpolate (&start_matrix, &end_matrix, progress, &result->transforms[0].matrix.matrix);

          return result;
        }
    }

  result = bobgui_css_transform_value_alloc (MAX (start->n_transforms, end->n_transforms));

  for (i = 0; i < n; i++)
    {
      bobgui_css_transform_transition (&result->transforms[i],
                                    &start->transforms[i],
                                    &end->transforms[i],
                                    property_id,
                                    progress);
    }

  for (; i < start->n_transforms; i++)
    {
      BobguiCssTransform transform;

      if (bobgui_css_transform_init_identity (&transform, start->transforms[i].type))
        {
          bobgui_css_transform_transition (&result->transforms[i],
                                        &start->transforms[i],
                                        &transform,
                                        property_id,
                                        progress);
          bobgui_css_transform_clear (&transform);
        }
      else
        {
          bobgui_css_transform_transition_default (&result->transforms[i],
                                                &start->transforms[i],
                                                NULL,
                                                property_id,
                                                progress);
        }
    }
  for (; i < end->n_transforms; i++)
    {
      BobguiCssTransform transform;

      if (bobgui_css_transform_init_identity (&transform, end->transforms[i].type))
        {
          bobgui_css_transform_transition (&result->transforms[i],
                                        &transform,
                                        &end->transforms[i],
                                        property_id,
                                        progress);
          bobgui_css_transform_clear (&transform);
        }
      else
        {
          bobgui_css_transform_transition_default (&result->transforms[i],
                                                NULL,
                                                &end->transforms[i],
                                                property_id,
                                                progress);
        }
    }

  g_assert (i == MAX (start->n_transforms, end->n_transforms));

  return result;
}

static void
bobgui_css_transform_print (const BobguiCssTransform *transform,
                         GString               *string)
{
  char buf[G_ASCII_DTOSTR_BUF_SIZE];

  switch (transform->type)
    {
    case BOBGUI_CSS_TRANSFORM_MATRIX:
      if (graphene_matrix_is_2d (&transform->matrix.matrix))
        {
          g_string_append (string, "matrix(");
          g_ascii_dtostr (buf, sizeof (buf), graphene_matrix_get_value (&transform->matrix.matrix, 0, 0));
          g_string_append (string, buf);
          g_string_append (string, ", ");
          g_ascii_dtostr (buf, sizeof (buf), graphene_matrix_get_value (&transform->matrix.matrix, 0, 1));
          g_string_append (string, buf);
          g_string_append (string, ", ");
          g_ascii_dtostr (buf, sizeof (buf), graphene_matrix_get_value (&transform->matrix.matrix, 0, 2));
          g_string_append (string, buf);
          g_string_append (string, ", ");
          g_ascii_dtostr (buf, sizeof (buf), graphene_matrix_get_value (&transform->matrix.matrix, 1, 0));
          g_string_append (string, buf);
          g_string_append (string, ", ");
          g_ascii_dtostr (buf, sizeof (buf), graphene_matrix_get_value (&transform->matrix.matrix, 1, 1));
          g_string_append (string, buf);
          g_string_append (string, ", ");
          g_ascii_dtostr (buf, sizeof (buf), graphene_matrix_get_value (&transform->matrix.matrix, 1, 2));
          g_string_append (string, buf);
          g_string_append (string, ")");
        }
      else
        {
          guint i;

          g_string_append (string, "matrix3d(");
          for (i = 0; i < 16; i++)
            {
              g_ascii_dtostr (buf, sizeof (buf), graphene_matrix_get_value (&transform->matrix.matrix, i / 4, i % 4));
              g_string_append (string, buf);
              if (i < 15)
                g_string_append (string, ", ");
            }
          g_string_append (string, ")");
        }
      break;
    case BOBGUI_CSS_TRANSFORM_TRANSLATE:
      g_string_append (string, "translate3d(");
      bobgui_css_value_print (transform->translate.x, string);
      g_string_append (string, ", ");
      bobgui_css_value_print (transform->translate.y, string);
      g_string_append (string, ", ");
      bobgui_css_value_print (transform->translate.z, string);
      g_string_append (string, ")");
      break;
    case BOBGUI_CSS_TRANSFORM_ROTATE:
      g_string_append (string, "rotate3d(");
      bobgui_css_value_print (transform->rotate.x, string);
      g_string_append (string, ", ");
      bobgui_css_value_print (transform->rotate.y, string);
      g_string_append (string, ", ");
      bobgui_css_value_print (transform->rotate.z, string);
      g_string_append (string, ", ");
      bobgui_css_value_print (transform->rotate.angle, string);
      g_string_append (string, ")");
      break;
    case BOBGUI_CSS_TRANSFORM_SCALE:
      if (bobgui_css_number_value_get (transform->scale.z, 100) == 1)
        {
          g_string_append (string, "scale(");
          bobgui_css_value_print (transform->scale.x, string);
          if (!bobgui_css_value_equal (transform->scale.x, transform->scale.y))
            {
              g_string_append (string, ", ");
              bobgui_css_value_print (transform->scale.y, string);
            }
          g_string_append (string, ")");
        }
      else
        {
          g_string_append (string, "scale3d(");
          bobgui_css_value_print (transform->scale.x, string);
          g_string_append (string, ", ");
          bobgui_css_value_print (transform->scale.y, string);
          g_string_append (string, ", ");
          bobgui_css_value_print (transform->scale.z, string);
          g_string_append (string, ")");
        }
      break;
    case BOBGUI_CSS_TRANSFORM_SKEW:
      g_string_append (string, "skew(");
      bobgui_css_value_print (transform->skew.x, string);
      g_string_append (string, ", ");
      bobgui_css_value_print (transform->skew.y, string);
      g_string_append (string, ")");
      break;
    case BOBGUI_CSS_TRANSFORM_SKEW_X:
      g_string_append (string, "skewX(");
      bobgui_css_value_print (transform->skew_x.skew, string);
      g_string_append (string, ")");
      break;
    case BOBGUI_CSS_TRANSFORM_SKEW_Y:
      g_string_append (string, "skewY(");
      bobgui_css_value_print (transform->skew_y.skew, string);
      g_string_append (string, ")");
      break;
    case BOBGUI_CSS_TRANSFORM_PERSPECTIVE:
      g_string_append (string, "perspective(");
      bobgui_css_value_print (transform->perspective.depth, string);
      g_string_append (string, ")");
      break;
    case BOBGUI_CSS_TRANSFORM_NONE:
    default:
      g_assert_not_reached ();
      break;
    }
}

static void
bobgui_css_value_transform_print (const BobguiCssValue *value,
                               GString           *string)
{
  guint i;

  if (bobgui_css_transform_value_is_none (value))
    {
      g_string_append (string, "none");
      return;
    }

  for (i = 0; i < value->n_transforms; i++)
    {
      if (i > 0)
        g_string_append_c (string, ' ');

      bobgui_css_transform_print (&value->transforms[i], string);
    }
}

static const BobguiCssValueClass BOBGUI_CSS_VALUE_TRANSFORM = {
  "BobguiCssTransformValue",
  bobgui_css_value_transform_free,
  bobgui_css_value_transform_compute,
  NULL,
  bobgui_css_value_transform_equal,
  bobgui_css_value_transform_transition,
  NULL,
  NULL,
  bobgui_css_value_transform_print
};

static BobguiCssValue transform_none_singleton = { &BOBGUI_CSS_VALUE_TRANSFORM, 1, 1, 0, 0, 0, {  { BOBGUI_CSS_TRANSFORM_NONE } } };

static BobguiCssValue *
bobgui_css_transform_value_alloc (guint n_transforms)
{
  BobguiCssValue *result;
           
  g_return_val_if_fail (n_transforms > 0, NULL);
         
  result = bobgui_css_value_alloc (&BOBGUI_CSS_VALUE_TRANSFORM, sizeof (BobguiCssValue) + sizeof (BobguiCssTransform) * (n_transforms - 1));
  result->n_transforms = n_transforms;
            
  return result;
}

BobguiCssValue *
_bobgui_css_transform_value_new_none (void)
{
  return bobgui_css_value_ref (&transform_none_singleton);
}

static gboolean
bobgui_css_transform_value_is_none (const BobguiCssValue *value)
{
  return value->n_transforms == 0;
}

static guint
bobgui_css_transform_parse_float (BobguiCssParser *parser,
                               guint         n,
                               gpointer      data)
{
  float *f = data;
  double d;

  if (!bobgui_css_parser_consume_number (parser, &d))
    return 0;

  f[n] = d;
  return 1;
}

static guint
bobgui_css_transform_parse_length (BobguiCssParser *parser,
                                guint         n,
                                gpointer      data)
{
  BobguiCssValue **values = data;

  values[n] = bobgui_css_number_value_parse (parser, BOBGUI_CSS_PARSE_LENGTH);
  if (values[n] == NULL)
    return 0;

  return 1;
}

static guint
bobgui_css_transform_parse_angle (BobguiCssParser *parser,
                               guint         n,
                               gpointer      data)
{
  BobguiCssValue **values = data;

  values[n] = bobgui_css_number_value_parse (parser, BOBGUI_CSS_PARSE_ANGLE);
  if (values[n] == NULL)
    return 0;

  return 1;
}

static guint
bobgui_css_transform_parse_number (BobguiCssParser *parser,
                                guint         n,
                                gpointer      data)
{
  BobguiCssValue **values = data;

  values[n] = bobgui_css_number_value_parse (parser, BOBGUI_CSS_PARSE_NUMBER);
  if (values[n] == NULL)
    return 0;

  return 1;
}

static guint
bobgui_css_transform_parse_rotate3d (BobguiCssParser *parser,
                                  guint         n,
                                  gpointer      data)
{
  BobguiCssTransform *transform = data;

  switch (n)
  {
    case 0:
      transform->rotate.x = bobgui_css_number_value_parse (parser, BOBGUI_CSS_PARSE_NUMBER);
      if (transform->rotate.x == NULL)
        return 0;
      break;

    case 1:
      transform->rotate.y = bobgui_css_number_value_parse (parser, BOBGUI_CSS_PARSE_NUMBER);
      if (transform->rotate.y == NULL)
        return 0;
      break;

    case 2:
      transform->rotate.z = bobgui_css_number_value_parse (parser, BOBGUI_CSS_PARSE_NUMBER);
      if (transform->rotate.z == NULL)
        return 0;
      break;

    case 3:
      transform->rotate.angle = bobgui_css_number_value_parse (parser, BOBGUI_CSS_PARSE_ANGLE);
      if (transform->rotate.angle == NULL)
        return 0;
      break;

    default:
      g_assert_not_reached();
      return 0;
  }

  return 1;
}

BobguiCssValue *
_bobgui_css_transform_value_parse (BobguiCssParser *parser)
{
  BobguiCssValue *value;
  GArray *array;
  guint i;
  gboolean computed = TRUE;

  if (bobgui_css_parser_try_ident (parser, "none"))
    return _bobgui_css_transform_value_new_none ();

  array = g_array_new (FALSE, FALSE, sizeof (BobguiCssTransform));

  while (TRUE)
    {
      BobguiCssTransform transform;

      memset (&transform, 0, sizeof (BobguiCssTransform));

      if (bobgui_css_parser_has_function (parser, "matrix"))
        {
          float f[6];

          if (!bobgui_css_parser_consume_function (parser, 6, 6, bobgui_css_transform_parse_float, f))
            goto fail;

          transform.type = BOBGUI_CSS_TRANSFORM_MATRIX;
          graphene_matrix_init_from_2d (&transform.matrix.matrix, f[0], f[1], f[2], f[3], f[4], f[5]);
        }
      else if (bobgui_css_parser_has_function (parser, "matrix3d"))
        {
          float f[16];

          if (!bobgui_css_parser_consume_function (parser, 16, 16, bobgui_css_transform_parse_float, f))
            goto fail;

          transform.type = BOBGUI_CSS_TRANSFORM_MATRIX;
          graphene_matrix_init_from_float (&transform.matrix.matrix, f);
        }
      else if (bobgui_css_parser_has_function (parser, "perspective"))
        {
          if (!bobgui_css_parser_consume_function (parser, 1, 1, bobgui_css_transform_parse_length, &transform.perspective.depth))
            goto fail;

          transform.type = BOBGUI_CSS_TRANSFORM_PERSPECTIVE;
          computed = computed && bobgui_css_value_is_computed (transform.perspective.depth);
        }
      else if (bobgui_css_parser_has_function (parser, "rotate") ||
               bobgui_css_parser_has_function (parser, "rotateZ"))
        {
          if (!bobgui_css_parser_consume_function (parser, 1, 1, bobgui_css_transform_parse_angle, &transform.rotate.angle))
            goto fail;

          transform.type = BOBGUI_CSS_TRANSFORM_ROTATE;
          transform.rotate.x = bobgui_css_number_value_new (0, BOBGUI_CSS_NUMBER);
          transform.rotate.y = bobgui_css_number_value_new (0, BOBGUI_CSS_NUMBER);
          transform.rotate.z = bobgui_css_number_value_new (1, BOBGUI_CSS_NUMBER);
          computed = computed && bobgui_css_value_is_computed (transform.rotate.angle);
        }
      else if (bobgui_css_parser_has_function (parser, "rotate3d"))
        {
          if (!bobgui_css_parser_consume_function (parser, 4, 4, bobgui_css_transform_parse_rotate3d, &transform))
            {
              g_clear_pointer (&transform.rotate.x, bobgui_css_value_unref);
              g_clear_pointer (&transform.rotate.y, bobgui_css_value_unref);
              g_clear_pointer (&transform.rotate.z, bobgui_css_value_unref);
              g_clear_pointer (&transform.rotate.angle, bobgui_css_value_unref);
              goto fail;
            }

          transform.type = BOBGUI_CSS_TRANSFORM_ROTATE;
          computed = computed && bobgui_css_value_is_computed (transform.rotate.angle) &&
                                 bobgui_css_value_is_computed (transform.rotate.x) &&
                                 bobgui_css_value_is_computed (transform.rotate.y) &&
                                 bobgui_css_value_is_computed (transform.rotate.z);
        }
      else if (bobgui_css_parser_has_function (parser, "rotateX"))
        {
          if (!bobgui_css_parser_consume_function (parser, 1, 1, bobgui_css_transform_parse_angle, &transform.rotate.angle))
            goto fail;

          transform.type = BOBGUI_CSS_TRANSFORM_ROTATE;
          transform.rotate.x = bobgui_css_number_value_new (1, BOBGUI_CSS_NUMBER);
          transform.rotate.y = bobgui_css_number_value_new (0, BOBGUI_CSS_NUMBER);
          transform.rotate.z = bobgui_css_number_value_new (0, BOBGUI_CSS_NUMBER);
          computed = computed && bobgui_css_value_is_computed (transform.rotate.angle);
        }
      else if (bobgui_css_parser_has_function (parser, "rotateY"))
        {
          if (!bobgui_css_parser_consume_function (parser, 1, 1, bobgui_css_transform_parse_angle, &transform.rotate.angle))
            goto fail;

          transform.type = BOBGUI_CSS_TRANSFORM_ROTATE;
          transform.rotate.x = bobgui_css_number_value_new (0, BOBGUI_CSS_NUMBER);
          transform.rotate.y = bobgui_css_number_value_new (1, BOBGUI_CSS_NUMBER);
          transform.rotate.z = bobgui_css_number_value_new (0, BOBGUI_CSS_NUMBER);
          computed = computed && bobgui_css_value_is_computed (transform.rotate.angle);
        }
      else if (bobgui_css_parser_has_function (parser, "scale"))
        {
          BobguiCssValue *values[2] = { NULL, NULL };

          if (!bobgui_css_parser_consume_function (parser, 1, 2, bobgui_css_transform_parse_number, values))
            {
              g_clear_pointer (&values[0], bobgui_css_value_unref);
              g_clear_pointer (&values[1], bobgui_css_value_unref);
              goto fail;
            }

          transform.type = BOBGUI_CSS_TRANSFORM_SCALE;
          transform.scale.x = values[0];
          if (values[1])
            transform.scale.y = values[1];
          else
            transform.scale.y = bobgui_css_value_ref (values[0]);
          transform.scale.z = bobgui_css_number_value_new (1, BOBGUI_CSS_NUMBER);
          computed = computed && bobgui_css_value_is_computed (transform.scale.x) &&
                                 bobgui_css_value_is_computed (transform.scale.y);
        }
      else if (bobgui_css_parser_has_function (parser, "scale3d"))
        {
          BobguiCssValue *values[3] = { NULL, NULL };

          if (!bobgui_css_parser_consume_function (parser, 3, 3, bobgui_css_transform_parse_number, values))
            {
              g_clear_pointer (&values[0], bobgui_css_value_unref);
              g_clear_pointer (&values[1], bobgui_css_value_unref);
              g_clear_pointer (&values[2], bobgui_css_value_unref);
              goto fail;
            }

          transform.type = BOBGUI_CSS_TRANSFORM_SCALE;
          transform.scale.x = values[0];
          transform.scale.y = values[1];
          transform.scale.z = values[2];
          computed = computed && bobgui_css_value_is_computed (transform.scale.x) &&
                                 bobgui_css_value_is_computed (transform.scale.y) &&
                                 bobgui_css_value_is_computed (transform.scale.z);
        }
      else if (bobgui_css_parser_has_function (parser, "scaleX"))
        {
          if (!bobgui_css_parser_consume_function (parser, 1, 1, bobgui_css_transform_parse_number, &transform.scale.x))
            goto fail;

          transform.type = BOBGUI_CSS_TRANSFORM_SCALE;
          transform.scale.y = bobgui_css_number_value_new (1, BOBGUI_CSS_NUMBER);
          transform.scale.z = bobgui_css_number_value_new (1, BOBGUI_CSS_NUMBER);
          computed = computed && bobgui_css_value_is_computed (transform.scale.x);
        }
      else if (bobgui_css_parser_has_function (parser, "scaleY"))
        {
          if (!bobgui_css_parser_consume_function (parser, 1, 1, bobgui_css_transform_parse_number, &transform.scale.y))
            goto fail;

          transform.type = BOBGUI_CSS_TRANSFORM_SCALE;
          transform.scale.x = bobgui_css_number_value_new (1, BOBGUI_CSS_NUMBER);
          transform.scale.z = bobgui_css_number_value_new (1, BOBGUI_CSS_NUMBER);
          computed = computed && bobgui_css_value_is_computed (transform.scale.y);
        }
      else if (bobgui_css_parser_has_function (parser, "scaleZ"))
        {
          if (!bobgui_css_parser_consume_function (parser, 1, 1, bobgui_css_transform_parse_number, &transform.scale.z))
            goto fail;

          transform.type = BOBGUI_CSS_TRANSFORM_SCALE;
          transform.scale.x = bobgui_css_number_value_new (1, BOBGUI_CSS_NUMBER);
          transform.scale.y = bobgui_css_number_value_new (1, BOBGUI_CSS_NUMBER);
          computed = computed && bobgui_css_value_is_computed (transform.scale.z);
        }
      else if (bobgui_css_parser_has_function (parser, "skew"))
        {
          BobguiCssValue *values[2] = { NULL, NULL };

          if (!bobgui_css_parser_consume_function (parser, 2, 2, bobgui_css_transform_parse_angle, values))
            {
              g_clear_pointer (&values[0], bobgui_css_value_unref);
              g_clear_pointer (&values[1], bobgui_css_value_unref);
              goto fail;
            }

          transform.type = BOBGUI_CSS_TRANSFORM_SKEW;
          transform.skew.x = values[0];
          transform.skew.y = values[1];
          computed = computed && bobgui_css_value_is_computed (transform.skew.x) &&
                                 bobgui_css_value_is_computed (transform.skew.y);
        }
      else if (bobgui_css_parser_has_function (parser, "skewX"))
        {
          if (!bobgui_css_parser_consume_function (parser, 1, 1, bobgui_css_transform_parse_angle, &transform.skew_x.skew))
            goto fail;

          transform.type = BOBGUI_CSS_TRANSFORM_SKEW_X;
          computed = computed && bobgui_css_value_is_computed (transform.skew_x.skew);
        }
      else if (bobgui_css_parser_has_function (parser, "skewY"))
        {
          if (!bobgui_css_parser_consume_function (parser, 1, 1, bobgui_css_transform_parse_angle, &transform.skew_y.skew))
            goto fail;

          transform.type = BOBGUI_CSS_TRANSFORM_SKEW_Y;
          computed = computed && bobgui_css_value_is_computed (transform.skew_y.skew);
        }
      else if (bobgui_css_parser_has_function (parser, "translate"))
        {
          BobguiCssValue *values[2] = { NULL, NULL };

          if (!bobgui_css_parser_consume_function (parser, 1, 2, bobgui_css_transform_parse_length, values))
            {
              g_clear_pointer (&values[0], bobgui_css_value_unref);
              g_clear_pointer (&values[1], bobgui_css_value_unref);
              goto fail;
            }

          transform.type = BOBGUI_CSS_TRANSFORM_TRANSLATE;
          transform.translate.x = values[0];
          if (values[1])
            transform.translate.y = values[1];
          else
            transform.translate.y = bobgui_css_number_value_new (0, BOBGUI_CSS_PX);
          transform.translate.z = bobgui_css_number_value_new (0, BOBGUI_CSS_PX);
          computed = computed && bobgui_css_value_is_computed (transform.translate.x) &&
                                 bobgui_css_value_is_computed (transform.translate.y);
        }
      else if (bobgui_css_parser_has_function (parser, "translate3d"))
        {
          BobguiCssValue *values[3] = { NULL, NULL };

          if (!bobgui_css_parser_consume_function (parser, 3, 3, bobgui_css_transform_parse_length, values))
            {
              g_clear_pointer (&values[0], bobgui_css_value_unref);
              g_clear_pointer (&values[1], bobgui_css_value_unref);
              g_clear_pointer (&values[2], bobgui_css_value_unref);
              goto fail;
            }

          transform.type = BOBGUI_CSS_TRANSFORM_TRANSLATE;
          transform.translate.x = values[0];
          transform.translate.y = values[1];
          transform.translate.z = values[2];
          computed = computed && bobgui_css_value_is_computed (transform.translate.x) &&
                                 bobgui_css_value_is_computed (transform.translate.y) &&
                                 bobgui_css_value_is_computed (transform.translate.z);
        }
      else if (bobgui_css_parser_has_function (parser, "translateX"))
        {
          if (!bobgui_css_parser_consume_function (parser, 1, 1, bobgui_css_transform_parse_length, &transform.translate.x))
            goto fail;

          transform.type = BOBGUI_CSS_TRANSFORM_TRANSLATE;
          transform.translate.y = bobgui_css_number_value_new (0, BOBGUI_CSS_PX);
          transform.translate.z = bobgui_css_number_value_new (0, BOBGUI_CSS_PX);
          computed = computed && bobgui_css_value_is_computed (transform.translate.x);
        }
      else if (bobgui_css_parser_has_function (parser, "translateY"))
        {
          if (!bobgui_css_parser_consume_function (parser, 1, 1, bobgui_css_transform_parse_length, &transform.translate.y))
            goto fail;

          transform.type = BOBGUI_CSS_TRANSFORM_TRANSLATE;
          transform.translate.x = bobgui_css_number_value_new (0, BOBGUI_CSS_PX);
          transform.translate.z = bobgui_css_number_value_new (0, BOBGUI_CSS_PX);
          computed = computed && bobgui_css_value_is_computed (transform.translate.y);
        }
      else if (bobgui_css_parser_has_function (parser, "translateZ"))
        {
          if (!bobgui_css_parser_consume_function (parser, 1, 1, bobgui_css_transform_parse_length, &transform.translate.z))
            goto fail;

          transform.type = BOBGUI_CSS_TRANSFORM_TRANSLATE;
          transform.translate.x = bobgui_css_number_value_new (0, BOBGUI_CSS_PX);
          transform.translate.y = bobgui_css_number_value_new (0, BOBGUI_CSS_PX);
          computed = computed && bobgui_css_value_is_computed (transform.translate.z);
        }
      else
        {
          break;
        }

      g_array_append_val (array, transform);
    }

  if (array->len == 0)
    {
      bobgui_css_parser_error_syntax (parser, "Expected a transform");
      goto fail;
    }

  value = bobgui_css_transform_value_alloc (array->len);
  value->is_computed = computed;
  memcpy (value->transforms, array->data, sizeof (BobguiCssTransform) * array->len);

  g_array_free (array, TRUE);

  return value;

fail:
  for (i = 0; i < array->len; i++)
    {
      bobgui_css_transform_clear (&g_array_index (array, BobguiCssTransform, i));
    }
  g_array_free (array, TRUE);
  return NULL;
}

GskTransform *
bobgui_css_transform_value_get_transform (const BobguiCssValue *transform)
{
  g_return_val_if_fail (transform->class == &BOBGUI_CSS_VALUE_TRANSFORM, FALSE);

  if (transform->n_transforms == 0)
    return NULL;

  return bobgui_css_transform_value_compute_transform (transform);
}

