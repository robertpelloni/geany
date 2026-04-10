/* BOBGUI - The Bobgui Framework
 * Copyright (C) 2017-2018, Red Hat, Inc.
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
 *
 * Author(s): Carlos Garnacho <carlosg@gnome.org>
 */
#pragma once

#include "bobguigesturesingleprivate.h"
#include "bobguigesturestylus.h"

struct _BobguiGestureStylus
{
  BobguiGestureSingle parent_instance;
};

struct _BobguiGestureStylusClass
{
  BobguiGestureSingleClass parent_class;

  void (*proximity) (BobguiGestureStylus *gesture,
                     double            x,
                     double            y);
  void (*down)      (BobguiGestureStylus *gesture,
                     double            x,
                     double            y);
  void (*motion)    (BobguiGestureStylus *gesture,
                     double            x,
                     double            y);
  void (*up)        (BobguiGestureStylus *gesture,
                     double            x,
                     double            y);

  /*< private >*/
  gpointer padding[10];
};

