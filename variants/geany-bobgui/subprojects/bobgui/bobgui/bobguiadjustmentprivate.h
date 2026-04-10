/* BOBGUI - The Bobgui Framework
 * Copyright (C) 2014 Red Hat, Inc
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

#pragma once


#include <bobgui/bobguiadjustment.h>


G_BEGIN_DECLS

double bobgui_adjustment_get_bounded_upper (BobguiAdjustment *self);

void bobgui_adjustment_enable_animation (BobguiAdjustment *adjustment,
                                      GdkFrameClock *clock,
                                      guint          duration);
guint bobgui_adjustment_get_animation_duration (BobguiAdjustment *adjustment);
void bobgui_adjustment_animate_to_value (BobguiAdjustment *adjustment,
                                      double         value);
double bobgui_adjustment_get_target_value (BobguiAdjustment *adjustment);

gboolean bobgui_adjustment_is_animating (BobguiAdjustment *adjustment);

G_END_DECLS


