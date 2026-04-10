/* BOBGUI - The Bobgui Framework
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
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

/*
 * Modified by the BOBGUI Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the BOBGUI Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * BOBGUI at ftp://ftp.bobgui.org/pub/bobgui/.
 */

#pragma once


#include <bobgui/bobguirange.h>


G_BEGIN_DECLS

void               _bobgui_range_set_has_origin               (BobguiRange      *range,
                                                            gboolean       has_origin);
gboolean           _bobgui_range_get_has_origin               (BobguiRange      *range);
void               _bobgui_range_set_stop_values              (BobguiRange      *range,
                                                            double        *values,
                                                            int            n_values);
int                _bobgui_range_get_stop_positions           (BobguiRange      *range,
                                                            int          **values);

BobguiWidget         *bobgui_range_get_slider_widget             (BobguiRange *range);
BobguiWidget         *bobgui_range_get_trough_widget             (BobguiRange *range);


void               bobgui_range_start_autoscroll              (BobguiRange      *range,
                                                            BobguiScrollType  scroll_type);
void               bobgui_range_stop_autoscroll               (BobguiRange      *range);

G_END_DECLS


