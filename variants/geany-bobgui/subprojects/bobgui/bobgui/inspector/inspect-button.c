/*
 * Copyright (c) 2008-2009  Christian Hammond
 * Copyright (c) 2008-2009  David Trowbridge
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#include "window.h"

#include "highlightoverlay.h"
#include "object-tree.h"

#include "bobguistack.h"
#include "bobguimain.h"
#include "bobguiwidgetprivate.h"
#include "bobguieventcontrollermotion.h"
#include "bobguieventcontrollerkey.h"
#include "bobguinative.h"
#include "bobguiwindowprivate.h"

static BobguiWidget *
find_widget_at_pointer (GdkDevice *device)
{
  BobguiWidget *widget = NULL;
  GdkSurface *pointer_surface;

  pointer_surface = gdk_device_get_surface_at_position (device, NULL, NULL);

  if (pointer_surface)
    widget = BOBGUI_WIDGET (bobgui_native_get_for_surface (pointer_surface));

  if (widget)
    {
      double x, y;
      double nx, ny;

      gdk_surface_get_device_position (bobgui_native_get_surface (BOBGUI_NATIVE (widget)),
                                       device, &x, &y, NULL);
      bobgui_native_get_surface_transform (BOBGUI_NATIVE (widget), &nx, &ny);
      x -= nx;
      y -= ny;

      widget = bobgui_widget_pick (widget, x, y, BOBGUI_PICK_INSENSITIVE|BOBGUI_PICK_NON_TARGETABLE);
    }

  return widget;
}

static void
clear_flash (BobguiInspectorWindow *iw)
{
  if (iw->flash_overlay)
    {
      bobgui_inspector_window_remove_overlay (iw, iw->flash_overlay);
      g_clear_object (&iw->flash_overlay);
    }
}

static void
start_flash (BobguiInspectorWindow *iw,
             BobguiWidget          *widget)
{
  clear_flash (iw);

  iw->flash_count = 1;
  iw->flash_overlay = bobgui_highlight_overlay_new (widget);
  bobgui_inspector_window_add_overlay (iw, iw->flash_overlay);
}

static void
select_widget (BobguiInspectorWindow *iw,
               BobguiWidget          *widget)
{
  BobguiInspectorObjectTree *wt = BOBGUI_INSPECTOR_OBJECT_TREE (iw->object_tree);

  bobgui_inspector_object_tree_activate_object (wt, G_OBJECT (widget));
}

static void
on_inspect_widget (BobguiInspectorWindow *iw,
                   GdkEvent           *event)
{
  BobguiWidget *widget;

  bobgui_window_present (BOBGUI_WINDOW (iw));

  clear_flash (iw);

  widget = find_widget_at_pointer (gdk_event_get_device (event));

  if (widget)
    select_widget (iw, widget);
}

static void
reemphasize_window (BobguiWidget *window)
{
  bobgui_window_present (BOBGUI_WINDOW (window));
}

static gboolean handle_event (BobguiInspectorWindow *iw, GdkEvent *event);

static gboolean
handle_event (BobguiInspectorWindow *iw, GdkEvent *event)
{
  switch ((int)gdk_event_get_event_type (event))
    {
    case GDK_KEY_PRESS:
    case GDK_KEY_RELEASE:
      {
        guint keyval = 0;

        keyval = gdk_key_event_get_keyval (event);
        if (keyval == GDK_KEY_Escape)
          {
            g_signal_handlers_disconnect_by_func (iw, handle_event, NULL);
            reemphasize_window (BOBGUI_WIDGET (iw));
            clear_flash (iw);
          }
      }
      break;

    case GDK_MOTION_NOTIFY:
      {
        BobguiWidget *widget = find_widget_at_pointer (gdk_event_get_device (event));

        if (widget == NULL)
          {
            /* This window isn't in-process. Ignore it. */
            break;
          }

        if (bobgui_widget_get_root (widget) == BOBGUI_ROOT (iw))
          {
            /* Don't highlight things in the inspector window */
            break;
          }

        if (iw->flash_overlay &&
            bobgui_highlight_overlay_get_widget (BOBGUI_HIGHLIGHT_OVERLAY (iw->flash_overlay)) == widget)
          {
            /* Already selected */
            break;
          }

        start_flash (iw, widget);
      }
      break;

    case GDK_BUTTON_PRESS:
    case GDK_BUTTON_RELEASE:
      g_signal_handlers_disconnect_by_func (iw, handle_event, NULL);
      reemphasize_window (BOBGUI_WIDGET (iw));
      on_inspect_widget (iw, event);
      break;

    default:;
    }

  return TRUE;
}

void
bobgui_inspector_on_inspect (BobguiWidget          *button,
                          BobguiInspectorWindow *iw)
{
  bobgui_widget_set_visible (BOBGUI_WIDGET (iw), FALSE);

  g_signal_connect (iw, "event", G_CALLBACK (handle_event), NULL);
}

static gboolean
on_flash_timeout (BobguiInspectorWindow *iw)
{
  iw->flash_count++;

  bobgui_highlight_overlay_set_color (BOBGUI_HIGHLIGHT_OVERLAY (iw->flash_overlay),
                                   &(GdkRGBA) { 
                                       0.0, 0.0, 1.0,
                                       (iw && iw->flash_count % 2 == 0) ? 0.0 : 0.2
                                   });

  if (iw->flash_count == 6)
    {
      clear_flash (iw);
      iw->flash_cnx = 0;

      return G_SOURCE_REMOVE;
    }

  return G_SOURCE_CONTINUE;
}

void
bobgui_inspector_flash_widget (BobguiInspectorWindow *iw,
                            BobguiWidget          *widget)
{
  if (!bobgui_widget_get_visible (widget) || !bobgui_widget_get_mapped (widget))
    return;

  if (iw->flash_cnx != 0)
    {
      g_source_remove (iw->flash_cnx);
      iw->flash_cnx = 0;
    }

  start_flash (iw, widget);
  iw->flash_cnx = g_timeout_add (150, (GSourceFunc) on_flash_timeout, iw);
}

void
bobgui_inspector_window_select_widget_under_pointer (BobguiInspectorWindow *iw)
{
  GdkDisplay *display;
  GdkSeat *seat;
  GdkDevice *device;
  BobguiWidget *widget;

  display = bobgui_inspector_window_get_inspected_display (iw);
  seat = gdk_display_get_default_seat (display);
  if (!seat)
    return;

  device = gdk_seat_get_pointer (seat);

  widget = find_widget_at_pointer (device);

  if (widget)
    select_widget (iw, widget);
}

/* vim: set et sw=2 ts=2: */
