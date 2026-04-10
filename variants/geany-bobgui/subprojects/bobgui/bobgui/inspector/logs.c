/*
 * Copyright (c) 2018, Red Hat, Inc.
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

#include "logs.h"
#include "window.h"

#include "bobguitextview.h"
#include "bobguicheckbutton.h"
#include "bobguilabel.h"
#include "bobguitooltip.h"
#include "bobguitextiter.h"
#include "bobguiprivate.h"
#include "bobguiroot.h"
#include "bobguidebug.h"
#include "bobguinative.h"
#include "gskdebugprivate.h"
#include "gskrendererprivate.h"
#include "bobguiboxlayout.h"

#include "gdk/gdkdebugprivate.h"

struct _BobguiInspectorLogs
{
  BobguiWidget parent;

  BobguiWidget *box;
  BobguiWidget *events;
  BobguiWidget *misc;
  BobguiWidget *dnd;
  BobguiWidget *input;
  BobguiWidget *eventloop;
  BobguiWidget *frames;
  BobguiWidget *settings;
  BobguiWidget *opengl;
  BobguiWidget *vulkan;
  BobguiWidget *selection;
  BobguiWidget *clipboard;
  BobguiWidget *dmabuf;
  BobguiWidget *offload;

  BobguiWidget *renderer;
  BobguiWidget *cairo;
  BobguiWidget *shaders;
  BobguiWidget *cache;
  BobguiWidget *fallback;
  BobguiWidget *verbose;

  BobguiWidget *actions;
  BobguiWidget *builder;
  BobguiWidget *sizes;
  BobguiWidget *icons;
  BobguiWidget *keybindings;
  BobguiWidget *modules;
  BobguiWidget *printing;
  BobguiWidget *tree;
  BobguiWidget *text;
  BobguiWidget *constraints;
  BobguiWidget *layout;
  BobguiWidget *a11y;

  GdkDisplay *display;
};

typedef struct _BobguiInspectorLogsClass
{
  BobguiWidgetClass parent;
} BobguiInspectorLogsClass;

G_DEFINE_TYPE (BobguiInspectorLogs, bobgui_inspector_logs, BOBGUI_TYPE_WIDGET)

static void
bobgui_inspector_logs_init (BobguiInspectorLogs *logs)
{
  bobgui_widget_init_template (BOBGUI_WIDGET (logs));
}

static void
dispose (GObject *object)
{
  bobgui_widget_dispose_template (BOBGUI_WIDGET (object), BOBGUI_TYPE_INSPECTOR_LOGS);

  G_OBJECT_CLASS (bobgui_inspector_logs_parent_class)->dispose (object);
}

static void
update_flag (BobguiWidget *widget,
             guint     *flags,
             guint      flag)
{
  if (bobgui_check_button_get_active (BOBGUI_CHECK_BUTTON (widget)))
    *flags = *flags | flag;
  else
    *flags = *flags & ~flag;
}

static void
flag_toggled (BobguiWidget        *button,
              BobguiInspectorLogs *logs)
{
  guint flags;
  GList *toplevels, *l;

  flags = gdk_display_get_debug_flags (logs->display);
  update_flag (logs->events, &flags, GDK_DEBUG_EVENTS);
  update_flag (logs->misc, &flags, GDK_DEBUG_MISC);
  update_flag (logs->dnd, &flags, GDK_DEBUG_DND);
  update_flag (logs->input, &flags, GDK_DEBUG_INPUT);
  update_flag (logs->eventloop, &flags, GDK_DEBUG_EVENTLOOP);
  update_flag (logs->frames, &flags, GDK_DEBUG_FRAMES);
  update_flag (logs->settings, &flags, GDK_DEBUG_SETTINGS);
  update_flag (logs->opengl, &flags, GDK_DEBUG_OPENGL);
  update_flag (logs->vulkan, &flags, GDK_DEBUG_VULKAN);
  update_flag (logs->selection, &flags, GDK_DEBUG_SELECTION);
  update_flag (logs->clipboard, &flags, GDK_DEBUG_CLIPBOARD);
  update_flag (logs->dmabuf, &flags, GDK_DEBUG_DMABUF);
  update_flag (logs->offload, &flags, GDK_DEBUG_OFFLOAD);
  gdk_display_set_debug_flags (logs->display, flags);

  flags = gsk_get_debug_flags ();
  update_flag (logs->renderer, &flags, GSK_DEBUG_RENDERER);
  update_flag (logs->cairo, &flags, GSK_DEBUG_CAIRO);
  update_flag (logs->shaders, &flags, GSK_DEBUG_SHADERS);
  update_flag (logs->cache, &flags, GSK_DEBUG_CACHE);
  update_flag (logs->fallback, &flags, GSK_DEBUG_FALLBACK);
  update_flag (logs->verbose, &flags, GSK_DEBUG_VERBOSE);
  gsk_set_debug_flags (flags);

  toplevels = bobgui_window_list_toplevels ();
  for (l = toplevels; l; l = l->next)
    {
      BobguiWidget *toplevel = l->data;

      if (bobgui_root_get_display (BOBGUI_ROOT (toplevel)) == logs->display)
        {
          GskRenderer *renderer = bobgui_native_get_renderer (BOBGUI_NATIVE (toplevel));
          if (renderer)
            gsk_renderer_set_debug_flags (renderer, flags);
        }
    }
  g_list_free (toplevels);

  flags = bobgui_get_display_debug_flags (logs->display);
  update_flag (logs->actions, &flags, BOBGUI_DEBUG_ACTIONS);
  update_flag (logs->builder, &flags, BOBGUI_DEBUG_BUILDER);
  update_flag (logs->sizes, &flags, BOBGUI_DEBUG_SIZE_REQUEST);
  update_flag (logs->icons, &flags, BOBGUI_DEBUG_ICONTHEME);
  update_flag (logs->keybindings, &flags, BOBGUI_DEBUG_KEYBINDINGS);
  update_flag (logs->modules, &flags, BOBGUI_DEBUG_MODULES);
  update_flag (logs->printing, &flags, BOBGUI_DEBUG_PRINTING);
  update_flag (logs->tree, &flags, BOBGUI_DEBUG_TREE);
  update_flag (logs->text, &flags, BOBGUI_DEBUG_TEXT);
  update_flag (logs->constraints, &flags, BOBGUI_DEBUG_CONSTRAINTS);
  update_flag (logs->layout, &flags, BOBGUI_DEBUG_LAYOUT);
  update_flag (logs->a11y, &flags, BOBGUI_DEBUG_A11Y);
  bobgui_set_display_debug_flags (logs->display, flags);
}

static void
bobgui_inspector_logs_class_init (BobguiInspectorLogsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (klass);

  object_class->dispose = dispose;

  bobgui_widget_class_set_template_from_resource (widget_class, "/org/bobgui/libbobgui/inspector/logs.ui");
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorLogs, box);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorLogs, events);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorLogs, misc);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorLogs, dnd);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorLogs, input);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorLogs, eventloop);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorLogs, frames);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorLogs, settings);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorLogs, opengl);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorLogs, vulkan);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorLogs, selection);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorLogs, clipboard);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorLogs, dmabuf);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorLogs, offload);

  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorLogs, renderer);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorLogs, cairo);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorLogs, shaders);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorLogs, cache);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorLogs, fallback);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorLogs, verbose);

  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorLogs, actions);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorLogs, builder);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorLogs, sizes);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorLogs, icons);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorLogs, keybindings);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorLogs, modules);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorLogs, printing);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorLogs, tree);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorLogs, text);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorLogs, constraints);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorLogs, layout);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorLogs, a11y);
  bobgui_widget_class_bind_template_callback (widget_class, flag_toggled);

  bobgui_widget_class_set_layout_manager_type (widget_class, BOBGUI_TYPE_BOX_LAYOUT);
}

void
bobgui_inspector_logs_set_display (BobguiInspectorLogs *logs,
                                GdkDisplay *display)
{
  logs->display = display;
}

// vim: set et sw=2 ts=2:
