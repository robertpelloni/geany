/* BOBGUI - The Bobgui Framework
 * Copyright (C) 2014 Red Hat, Inc.
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
#include "bobguigestureclick.h"

struct _BobguiGestureClick
{
  BobguiGestureSingle parent_instance;
};

struct _BobguiGestureClickClass
{
  BobguiGestureSingleClass parent_class;

  void     (* pressed)  (BobguiGestureClick *gesture,
                         int              n_press,
                         double           x,
                         double           y);
  void     (* released) (BobguiGestureClick *gesture,
                         int              n_press,
                         double           x,
                         double           y);
  void     (* stopped)  (BobguiGestureClick *gesture);

  /*<private>*/
  gpointer padding[10];
};

