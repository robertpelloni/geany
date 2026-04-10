/*
 * Copyright © 2020 Benjamin Otte
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

#include "bobguidropprivate.h"

typedef struct _BobguiDrop BobguiDrop;

struct _BobguiDrop
{
  /* TRUE if we are waiting for a gdk_drop_status() call */
  gboolean waiting;
  /* TRUE if begin_event() has been called but end_event() hasn't yet - purely for debugging */
  gboolean active;
};

static void
bobgui_drop_free (gpointer data)
{
  BobguiDrop *self = data;

  g_free (self);
}

static BobguiDrop *
bobgui_drop_lookup (GdkDrop *drop)
{
  static GQuark drop_quark = 0;
  BobguiDrop *result;

  if (G_UNLIKELY (drop_quark == 0))
    drop_quark = g_quark_from_string ("-bobgui-drop-data");

  result = g_object_get_qdata (G_OBJECT (drop), drop_quark);
  if (result == NULL)
    {
      result = g_new0 (BobguiDrop, 1);
      g_object_set_qdata_full (G_OBJECT (drop), drop_quark, result, bobgui_drop_free);
    }

  return result;
}

void
bobgui_drop_begin_event (GdkDrop      *drop,
                      GdkEventType  event_type)
{
  BobguiDrop *self;

  self = bobgui_drop_lookup (drop);

  g_assert (self->waiting == FALSE);
  g_assert (self->active == FALSE);

  self->active = TRUE;
  if (event_type == GDK_DRAG_ENTER ||
      event_type == GDK_DRAG_MOTION)
    self->waiting = TRUE;
}

void
bobgui_drop_end_event (GdkDrop *drop)
{
  BobguiDrop *self;

  self = bobgui_drop_lookup (drop);

  g_assert (self->active == TRUE);

  if (self->waiting)
    {
      gdk_drop_status (drop, 0, 0);
      self->waiting = FALSE;
    }
  self->active = FALSE;
}

gboolean
bobgui_drop_status (GdkDrop       *drop,
                 GdkDragAction  actions,
                 GdkDragAction  preferred_action)
{
  BobguiDrop *self;

  self = bobgui_drop_lookup (drop);

  g_assert (self->active == TRUE);

  if (!self->waiting)
    return FALSE;

  gdk_drop_status (drop, actions, preferred_action);
  self->waiting = FALSE;
  return TRUE;
}
                     
