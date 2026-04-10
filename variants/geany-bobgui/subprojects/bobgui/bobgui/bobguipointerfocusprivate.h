/* BOBGUI - The Bobgui Framework
 * Copyright (C) 2017 - Red Hat Inc.
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

#include <bobgui/bobguitypes.h>

typedef struct _BobguiPointerFocus BobguiPointerFocus;

struct _BobguiPointerFocus
{
  int ref_count;
  GdkDevice *device;
  GdkEventSequence *sequence;
  BobguiWindow *toplevel;
  BobguiWidget *target; /* Unaffected by the implicit grab */
  BobguiWidget *grab_widget;
  double x, y; /* In toplevel coordinates */
};

BobguiPointerFocus * bobgui_pointer_focus_new  (BobguiWindow        *toplevel,
                                          BobguiWidget        *widget,
                                          GdkDevice        *device,
                                          GdkEventSequence *sequence,
                                          double            x,
                                          double            y);
BobguiPointerFocus * bobgui_pointer_focus_ref   (BobguiPointerFocus *focus);
void              bobgui_pointer_focus_unref (BobguiPointerFocus *focus);

void              bobgui_pointer_focus_set_coordinates (BobguiPointerFocus *focus,
                                                     double           x,
                                                     double           y);
void              bobgui_pointer_focus_set_target      (BobguiPointerFocus *focus,
                                                     BobguiWidget       *target);
BobguiWidget *       bobgui_pointer_focus_get_target      (BobguiPointerFocus *focus);

void              bobgui_pointer_focus_set_implicit_grab (BobguiPointerFocus *focus,
                                                       BobguiWidget       *grab_widget);
BobguiWidget *       bobgui_pointer_focus_get_implicit_grab (BobguiPointerFocus *focus);

BobguiWidget *       bobgui_pointer_focus_get_effective_target (BobguiPointerFocus *focus);

void              bobgui_pointer_focus_repick_target (BobguiPointerFocus *focus);

