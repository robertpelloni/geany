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

#include "config.h"

#include "bobguipointerfocusprivate.h"
#include "bobguiprivate.h"

static void
target_destroyed (gpointer  data,
                  GObject  *object_location)
{
  BobguiPointerFocus *focus = data;

  focus->target = NULL;
  bobgui_pointer_focus_repick_target (focus);
}

BobguiPointerFocus *
bobgui_pointer_focus_new (BobguiWindow        *toplevel,
                       BobguiWidget        *widget,
                       GdkDevice        *device,
                       GdkEventSequence *sequence,
                       double            x,
                       double            y)
{
  BobguiPointerFocus *focus;

  focus = g_new0 (BobguiPointerFocus, 1);
  focus->ref_count = 1;
  focus->toplevel = toplevel;
  focus->device = device;
  focus->sequence = sequence;
  bobgui_pointer_focus_set_target (focus, widget);
  bobgui_pointer_focus_set_coordinates (focus, x, y);

  return focus;
}

BobguiPointerFocus *
bobgui_pointer_focus_ref (BobguiPointerFocus *focus)
{
  focus->ref_count++;
  return focus;
}

void
bobgui_pointer_focus_unref (BobguiPointerFocus *focus)
{
  focus->ref_count--;

  if (focus->ref_count == 0)
    {
      bobgui_pointer_focus_set_target (focus, NULL);
      bobgui_pointer_focus_set_implicit_grab (focus, NULL);
      g_free (focus);
    }
}

void
bobgui_pointer_focus_set_target (BobguiPointerFocus *focus,
                              BobguiWidget       *target)
{
  if (focus->target == target)
    return;

  if (focus->target)
    g_object_weak_unref (G_OBJECT (focus->target), target_destroyed, focus);

  focus->target = target;

  if (focus->target)
    g_object_weak_ref (G_OBJECT (focus->target), target_destroyed, focus);
}

BobguiWidget *
bobgui_pointer_focus_get_target (BobguiPointerFocus *focus)
{
  return focus->target;
}

void
bobgui_pointer_focus_set_implicit_grab (BobguiPointerFocus *focus,
                                     BobguiWidget       *grab_widget)
{
  focus->grab_widget = grab_widget;
}

BobguiWidget *
bobgui_pointer_focus_get_implicit_grab (BobguiPointerFocus *focus)
{
  return focus->grab_widget;
}

void
bobgui_pointer_focus_set_coordinates (BobguiPointerFocus *focus,
                                   double           x,
                                   double           y)
{
  focus->x = x;
  focus->y = y;
}

BobguiWidget *
bobgui_pointer_focus_get_effective_target (BobguiPointerFocus *focus)
{
  BobguiWidget *target;

  target = focus->target;

  if (focus->grab_widget &&
      focus->grab_widget != target &&
      !bobgui_widget_is_ancestor (target, focus->grab_widget))
    target = focus->grab_widget;

  return target;
}

void
bobgui_pointer_focus_repick_target (BobguiPointerFocus *focus)
{
  BobguiWidget *target;

  target = bobgui_widget_pick (BOBGUI_WIDGET (focus->toplevel), focus->x, focus->y, BOBGUI_PICK_DEFAULT);
  if (target == NULL)
    target = BOBGUI_WIDGET (focus->toplevel);
  bobgui_pointer_focus_set_target (focus, target);
}
