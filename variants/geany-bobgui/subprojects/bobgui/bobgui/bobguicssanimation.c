/*
 * Copyright © 2012 Red Hat Inc.
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

#include "bobguicssanimationprivate.h"

#include "bobguicsseasevalueprivate.h"
#include "bobguiprogresstrackerprivate.h"

#include <math.h>

static gboolean
bobgui_css_animation_is_executing (BobguiCssAnimation *animation)
{
  BobguiProgressState state = bobgui_progress_tracker_get_state (&animation->tracker);

  switch (animation->fill_mode)
    {
    case BOBGUI_CSS_FILL_NONE:
      return state == BOBGUI_PROGRESS_STATE_DURING;
    case BOBGUI_CSS_FILL_FORWARDS:
      return state != BOBGUI_PROGRESS_STATE_BEFORE;
    case BOBGUI_CSS_FILL_BACKWARDS:
      return state != BOBGUI_PROGRESS_STATE_AFTER;
    case BOBGUI_CSS_FILL_BOTH:
      return TRUE;
    default:
      g_return_val_if_reached (FALSE);
    }
}

static double
bobgui_css_animation_get_progress (BobguiCssAnimation *animation)
{
  gboolean reverse, odd_iteration;
  int cycle = bobgui_progress_tracker_get_iteration_cycle (&animation->tracker);
  odd_iteration = cycle % 2 > 0;

  switch (animation->direction)
    {
    case BOBGUI_CSS_DIRECTION_NORMAL:
      reverse = FALSE;
      break;
    case BOBGUI_CSS_DIRECTION_REVERSE:
      reverse = TRUE;
      break;
    case BOBGUI_CSS_DIRECTION_ALTERNATE:
      reverse = odd_iteration;
      break;
    case BOBGUI_CSS_DIRECTION_ALTERNATE_REVERSE:
      reverse = !odd_iteration;
      break;
    default:
      g_return_val_if_reached (0.0);
    }

  return bobgui_progress_tracker_get_progress (&animation->tracker, reverse);
}

static BobguiStyleAnimation *
bobgui_css_animation_advance (BobguiStyleAnimation    *style_animation,
                           gint64                timestamp)
{
  BobguiCssAnimation *animation = (BobguiCssAnimation *)style_animation;

  return _bobgui_css_animation_advance_with_play_state (animation,
                                                     timestamp,
                                                     animation->play_state);
}

static void
bobgui_css_animation_apply_values (BobguiStyleAnimation    *style_animation,
                                BobguiCssAnimatedStyle  *style)
{
  BobguiCssAnimation *animation = (BobguiCssAnimation *)style_animation;
  BobguiCssStyle *base_style, *parent_style;
  BobguiStyleProvider *provider;
  BobguiCssKeyframes *resolved_keyframes;
  double progress;
  guint i;
  gboolean needs_recompute = FALSE;

  if (!bobgui_css_animation_is_executing (animation))
    return;

  progress = bobgui_css_animation_get_progress (animation);
  progress = _bobgui_css_ease_value_transform (animation->ease, progress);

  base_style = bobgui_css_animated_style_get_base_style (style);
  parent_style = bobgui_css_animated_style_get_parent_style (style);
  provider = bobgui_css_animated_style_get_provider (style);
  resolved_keyframes = _bobgui_css_keyframes_compute (animation->keyframes,
                                                   provider,
                                                   base_style,
                                                   parent_style);

  for (i = 0; i < _bobgui_css_keyframes_get_n_variables (resolved_keyframes); i++)
    {
      BobguiCssVariableValue *value;
      int variable_id;

      variable_id = _bobgui_css_keyframes_get_variable_id (resolved_keyframes, i);

      value = _bobgui_css_keyframes_get_variable (resolved_keyframes,
                                               i,
                                               progress,
                                               bobgui_css_animated_style_get_intrinsic_custom_value (style, variable_id));

      if (!value)
        continue;

      if (bobgui_css_animated_style_set_animated_custom_value (style, variable_id, value))
        needs_recompute = TRUE;

      bobgui_css_variable_value_unref (value);
    }

  if (needs_recompute)
    bobgui_css_animated_style_recompute (style);

  for (i = 0; i < _bobgui_css_keyframes_get_n_properties (resolved_keyframes); i++)
    {
      BobguiCssValue *value;
      guint property_id;

      property_id = _bobgui_css_keyframes_get_property_id (resolved_keyframes, i);

      value = _bobgui_css_keyframes_get_value (resolved_keyframes,
                                            i,
                                            progress,
                                            bobgui_css_animated_style_get_intrinsic_value (style, property_id));
      bobgui_css_animated_style_set_animated_value (style, property_id, value);
    }

  _bobgui_css_keyframes_unref (resolved_keyframes);
}

static gboolean
bobgui_css_animation_is_finished (BobguiStyleAnimation *style_animation)
{
  return FALSE;
}

static gboolean
bobgui_css_animation_is_static (BobguiStyleAnimation *style_animation)
{
  BobguiCssAnimation *animation = (BobguiCssAnimation *)style_animation;

  if (animation->play_state == BOBGUI_CSS_PLAY_STATE_PAUSED)
    return TRUE;

  return bobgui_progress_tracker_get_state (&animation->tracker) == BOBGUI_PROGRESS_STATE_AFTER;
}

static void
bobgui_css_animation_free (BobguiStyleAnimation *animation)
{
  BobguiCssAnimation *self = (BobguiCssAnimation *)animation;

  g_free (self->name);
  _bobgui_css_keyframes_unref (self->keyframes);
  bobgui_css_value_unref (self->ease);

  g_free (self);
}

static const BobguiStyleAnimationClass BOBGUI_CSS_ANIMATION_CLASS = {
  "BobguiCssAnimation",
  bobgui_css_animation_free,
  bobgui_css_animation_is_finished,
  bobgui_css_animation_is_static,
  bobgui_css_animation_apply_values,
  bobgui_css_animation_advance,
};


BobguiStyleAnimation *
_bobgui_css_animation_new (const char      *name,
                        BobguiCssKeyframes *keyframes,
                        gint64           timestamp,
                        gint64           delay_us,
                        gint64           duration_us,
                        BobguiCssValue     *ease,
                        BobguiCssDirection  direction,
                        BobguiCssPlayState  play_state,
                        BobguiCssFillMode   fill_mode,
                        double           iteration_count)
{
  BobguiCssAnimation *animation;

  g_return_val_if_fail (name != NULL, NULL);
  g_return_val_if_fail (keyframes != NULL, NULL);
  g_return_val_if_fail (ease != NULL, NULL);
  g_return_val_if_fail (iteration_count >= 0, NULL);

  animation = g_new (BobguiCssAnimation, 1);
  animation->parent.class = &BOBGUI_CSS_ANIMATION_CLASS;
  animation->parent.ref_count = 1;

  animation->name = g_strdup (name);
  animation->keyframes = _bobgui_css_keyframes_ref (keyframes);
  animation->ease = bobgui_css_value_ref (ease);
  animation->direction = direction;
  animation->play_state = play_state;
  animation->fill_mode = fill_mode;

  bobgui_progress_tracker_start (&animation->tracker, duration_us, delay_us, iteration_count);
  if (animation->play_state == BOBGUI_CSS_PLAY_STATE_PAUSED)
    bobgui_progress_tracker_skip_frame (&animation->tracker, timestamp);
  else
    bobgui_progress_tracker_advance_frame (&animation->tracker, timestamp);

  return (BobguiStyleAnimation *)animation;
}

const char *
_bobgui_css_animation_get_name (BobguiCssAnimation *animation)
{
  return animation->name;
}

BobguiStyleAnimation *
_bobgui_css_animation_advance_with_play_state (BobguiCssAnimation *source,
                                            gint64           timestamp,
                                            BobguiCssPlayState  play_state)
{
  BobguiCssAnimation *animation = g_new (BobguiCssAnimation, 1);
  animation->parent.class = &BOBGUI_CSS_ANIMATION_CLASS;
  animation->parent.ref_count = 1;

  animation->name = g_strdup (source->name);
  animation->keyframes = _bobgui_css_keyframes_ref (source->keyframes);
  animation->ease = bobgui_css_value_ref (source->ease);
  animation->direction = source->direction;
  animation->play_state = play_state;
  animation->fill_mode = source->fill_mode;

  bobgui_progress_tracker_init_copy (&source->tracker, &animation->tracker);
  if (animation->play_state == BOBGUI_CSS_PLAY_STATE_PAUSED)
    bobgui_progress_tracker_skip_frame (&animation->tracker, timestamp);
  else
    bobgui_progress_tracker_advance_frame (&animation->tracker, timestamp);

  return (BobguiStyleAnimation *)animation;
}

gboolean
_bobgui_css_animation_is_animation (BobguiStyleAnimation *animation)
{
  return animation->class == &BOBGUI_CSS_ANIMATION_CLASS;
}
