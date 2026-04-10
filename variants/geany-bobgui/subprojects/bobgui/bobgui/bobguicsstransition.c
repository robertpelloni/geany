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

#include "bobguicsstransitionprivate.h"

#include "bobguicsseasevalueprivate.h"
#include "bobguiprogresstrackerprivate.h"

struct _BobguiCssTransition
{
  BobguiStyleAnimation parent;

  guint               property;
  guint               finished;
  BobguiCssValue        *start;
  BobguiCssValue        *ease;
  BobguiProgressTracker  tracker;
};


static BobguiStyleAnimation *   bobgui_css_transition_advance  (BobguiStyleAnimation    *style_animation,
                                                          gint64                timestamp);



static void
bobgui_css_transition_apply_values (BobguiStyleAnimation   *style_animation,
                                 BobguiCssAnimatedStyle *style)
{
  BobguiCssTransition *transition = (BobguiCssTransition *)style_animation;
  BobguiCssValue *value, *end;
  double progress;
  BobguiProgressState state;

  if (transition->finished)
    return;

  end = bobgui_css_animated_style_get_intrinsic_value (style, transition->property);
  state = bobgui_progress_tracker_get_state (&transition->tracker);

  if (state == BOBGUI_PROGRESS_STATE_BEFORE)
    value = bobgui_css_value_ref (transition->start);
  else if (state == BOBGUI_PROGRESS_STATE_DURING)
    {
      progress = bobgui_progress_tracker_get_progress (&transition->tracker, FALSE);
      progress = _bobgui_css_ease_value_transform (transition->ease, progress);

      value = bobgui_css_value_transition (transition->start,
                                        end,
                                        transition->property,
                                        progress);
    }
  else
    return;

  if (value == NULL)
    value = bobgui_css_value_ref (end);

  bobgui_css_animated_style_set_animated_value (style, transition->property, value);
}

static gboolean
bobgui_css_transition_is_finished (BobguiStyleAnimation *animation)
{
  BobguiCssTransition *transition = (BobguiCssTransition *)animation;

  return transition->finished;
}

static gboolean
bobgui_css_transition_is_static (BobguiStyleAnimation *animation)
{
  BobguiCssTransition *transition = (BobguiCssTransition *)animation;

  return transition->finished;
}

static void
bobgui_css_transition_free (BobguiStyleAnimation *animation)
{
  BobguiCssTransition *self = (BobguiCssTransition *)animation;

  bobgui_css_value_unref (self->start);
  bobgui_css_value_unref (self->ease);
  g_free (self);
}

static const BobguiStyleAnimationClass BOBGUI_CSS_TRANSITION_CLASS = {
  "BobguiCssTransition",
  bobgui_css_transition_free,
  bobgui_css_transition_is_finished,
  bobgui_css_transition_is_static,
  bobgui_css_transition_apply_values,
  bobgui_css_transition_advance,
};

static BobguiStyleAnimation *
bobgui_css_transition_advance (BobguiStyleAnimation    *style_animation,
                            gint64                timestamp)
{
  BobguiCssTransition *source = (BobguiCssTransition *)style_animation;
  BobguiCssTransition *transition;

  transition = g_new (BobguiCssTransition, 1);
  transition->parent.class = &BOBGUI_CSS_TRANSITION_CLASS;
  transition->parent.ref_count = 1;

  transition->property = source->property;
  transition->start = bobgui_css_value_ref (source->start);
  transition->ease = bobgui_css_value_ref (source->ease);

  bobgui_progress_tracker_init_copy (&source->tracker, &transition->tracker);
  bobgui_progress_tracker_advance_frame (&transition->tracker, timestamp);
  transition->finished = bobgui_progress_tracker_get_state (&transition->tracker) == BOBGUI_PROGRESS_STATE_AFTER;

  return (BobguiStyleAnimation *)transition;
}
BobguiStyleAnimation *
_bobgui_css_transition_new (guint        property,
                         BobguiCssValue *start,
                         BobguiCssValue *ease,
                         gint64       timestamp,
                         gint64       duration_us,
                         gint64       delay_us)
{
  BobguiCssTransition *transition;

  g_return_val_if_fail (start != NULL, NULL);
  g_return_val_if_fail (ease != NULL, NULL);

  transition = g_new (BobguiCssTransition, 1);
  transition->parent.class = &BOBGUI_CSS_TRANSITION_CLASS;
  transition->parent.ref_count = 1;

  transition->property = property;
  transition->start = bobgui_css_value_ref (start);
  transition->ease = bobgui_css_value_ref (ease);
  bobgui_progress_tracker_start (&transition->tracker, duration_us, delay_us, 1.0);
  bobgui_progress_tracker_advance_frame (&transition->tracker, timestamp);
  transition->finished = bobgui_progress_tracker_get_state (&transition->tracker) == BOBGUI_PROGRESS_STATE_AFTER;

  return (BobguiStyleAnimation*)transition;
}

guint
_bobgui_css_transition_get_property (BobguiCssTransition *transition)
{
  return transition->property;
}

gboolean
_bobgui_css_transition_is_transition (BobguiStyleAnimation  *animation)
{
  return animation->class == &BOBGUI_CSS_TRANSITION_CLASS;
}
