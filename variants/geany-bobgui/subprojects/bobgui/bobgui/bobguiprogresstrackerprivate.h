/*
 * Copyright © 2016 Endless Mobile Inc.
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
 * Authors: Matthew Watson <mattdangerw@gmail.com>
 */

#pragma once

#include <glib-object.h>
#include "bobguicsseasevalueprivate.h"

G_BEGIN_DECLS

typedef enum {
  BOBGUI_PROGRESS_STATE_BEFORE,
  BOBGUI_PROGRESS_STATE_DURING,
  BOBGUI_PROGRESS_STATE_AFTER,
} BobguiProgressState;

typedef struct _BobguiProgressTracker BobguiProgressTracker;

struct _BobguiProgressTracker
{
  guint64 last_frame_time;
  guint64 duration;
  double iteration;
  double iteration_count;
  gboolean is_running;
};

void                 bobgui_progress_tracker_init_copy           (BobguiProgressTracker *source,
                                                               BobguiProgressTracker *dest);

void                 bobgui_progress_tracker_start               (BobguiProgressTracker *tracker,
                                                               guint64 duration,
                                                               gint64 delay,
                                                               double iteration_count);

void                 bobgui_progress_tracker_finish              (BobguiProgressTracker *tracker);

void                 bobgui_progress_tracker_advance_frame       (BobguiProgressTracker *tracker,
                                                               guint64 frame_time);

void                 bobgui_progress_tracker_skip_frame          (BobguiProgressTracker *tracker,
                                                               guint64 frame_time);

BobguiProgressState     bobgui_progress_tracker_get_state           (BobguiProgressTracker *tracker);

double               bobgui_progress_tracker_get_iteration       (BobguiProgressTracker *tracker);

guint64              bobgui_progress_tracker_get_iteration_cycle (BobguiProgressTracker *tracker);

double               bobgui_progress_tracker_get_progress        (BobguiProgressTracker *tracker,
                                                               gboolean reverse);

double               bobgui_progress_tracker_get_ease_out_cubic  (BobguiProgressTracker *tracker,
                                                               gboolean reverse);

G_END_DECLS

