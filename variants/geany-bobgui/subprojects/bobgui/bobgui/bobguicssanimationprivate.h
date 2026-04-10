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

#pragma once

#include "bobguistyleanimationprivate.h"

#include "bobguicsskeyframesprivate.h"
#include "bobguiprogresstrackerprivate.h"

G_BEGIN_DECLS

typedef struct _BobguiCssAnimation           BobguiCssAnimation;
typedef struct _BobguiCssAnimationClass      BobguiCssAnimationClass;

struct _BobguiCssAnimation
{
  BobguiStyleAnimation parent;

  char            *name;
  BobguiCssKeyframes *keyframes;
  BobguiCssValue     *ease;
  BobguiCssDirection  direction;
  BobguiCssPlayState  play_state;
  BobguiCssFillMode   fill_mode;
  BobguiProgressTracker tracker;
};

struct _BobguiCssAnimationClass
{
  BobguiStyleAnimationClass parent_class;
};

GType                   _bobgui_css_animation_get_type        (void) G_GNUC_CONST;

BobguiStyleAnimation *     _bobgui_css_animation_new             (const char         *name,
                                                            BobguiCssKeyframes    *keyframes,
                                                            gint64              timestamp,
                                                            gint64              delay_us,
                                                            gint64              duration_us,
                                                            BobguiCssValue        *ease,
                                                            BobguiCssDirection     direction,
                                                            BobguiCssPlayState     play_state,
                                                            BobguiCssFillMode      fill_mode,
                                                            double              iteration_count);

BobguiStyleAnimation *     _bobgui_css_animation_advance_with_play_state (BobguiCssAnimation   *animation,
                                                                    gint64             timestamp,
                                                                    BobguiCssPlayState    play_state);

const char *            _bobgui_css_animation_get_name        (BobguiCssAnimation   *animation);
gboolean                _bobgui_css_animation_is_animation    (BobguiStyleAnimation *animation);

G_END_DECLS

