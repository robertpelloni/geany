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

#include "bobguistyleanimationprivate.h"

BobguiStyleAnimation *
_bobgui_style_animation_advance (BobguiStyleAnimation *animation,
                              gint64             timestamp)
{
  g_assert (animation != NULL);

  return animation->class->advance (animation, timestamp);
}

void
_bobgui_style_animation_apply_values (BobguiStyleAnimation    *animation,
                                   BobguiCssAnimatedStyle  *style)
{
  animation->class->apply_values (animation, style);
}

gboolean
_bobgui_style_animation_is_finished (BobguiStyleAnimation *animation)
{
  return animation->class->is_finished (animation);
}


BobguiStyleAnimation *
bobgui_style_animation_ref (BobguiStyleAnimation *animation)
{
  animation->ref_count++;
  return animation;
}

BobguiStyleAnimation *
bobgui_style_animation_unref (BobguiStyleAnimation *animation)
{
  animation->ref_count--;

  if (animation->ref_count == 0)
    {
      animation->class->free (animation);
      return NULL;
    }
  return animation;
}

/**
 * _bobgui_style_animation_is_static:
 * @animation: The animation to query
 * @at_time_us: The timestamp to query for
 *
 * Checks if @animation will not change its values anymore after
 * @at_time_us. This happens for example when the animation has reached its
 * final value or when it has been paused.
 *
 * Returns: %TRUE if @animation will not change anymore after @at_time_us
 **/
gboolean
_bobgui_style_animation_is_static (BobguiStyleAnimation *animation)
{
  return animation->class->is_static (animation);
}
