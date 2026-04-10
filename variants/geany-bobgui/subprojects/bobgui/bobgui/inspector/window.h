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
#pragma once


#include <bobgui/bobguiwindow.h>

#include "inspectoroverlay.h"

#define BOBGUI_TYPE_INSPECTOR_WINDOW            (bobgui_inspector_window_get_type())
#define BOBGUI_INSPECTOR_WINDOW(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), BOBGUI_TYPE_INSPECTOR_WINDOW, BobguiInspectorWindow))
#define BOBGUI_INSPECTOR_WINDOW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), BOBGUI_TYPE_INSPECTOR_WINDOW, BobguiInspectorWindowClass))
#define BOBGUI_INSPECTOR_IS_WINDOW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), BOBGUI_TYPE_INSPECTOR_WINDOW))
#define BOBGUI_INSPECTOR_IS_WINDOW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), BOBGUI_TYPE_INSPECTOR_WINDOW))
#define BOBGUI_INSPECTOR_WINDOW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), BOBGUI_TYPE_INSPECTOR_WINDOW, BobguiInspectorWindowClass))


#define TREE_TEXT_SCALE 0.8
#define TREE_CHECKBOX_SIZE (int)(0.8 * 13)

typedef struct
{
  BobguiWindow parent;

  BobguiWidget *top_stack;
  BobguiWidget *object_stack;
  BobguiWidget *button_stack;
  BobguiWidget *object_tree;
  BobguiWidget *object_id;
  BobguiWidget *object_details;
  BobguiWidget *object_buttons;
  BobguiWidget *object_details_button;
  BobguiWidget *select_object;
  BobguiWidget *object_start_stack;
  BobguiWidget *object_center_stack;
  BobguiWidget *object_title;
  BobguiWidget *prop_list;
  BobguiWidget *layout_prop_list;
  BobguiWidget *selector;
  BobguiWidget *signals_list;
  BobguiWidget *classes_list;
  BobguiWidget *widget_css_node_tree;
  BobguiWidget *widget_recorder;
  BobguiWidget *object_hierarchy;
  BobguiWidget *size_groups;
  BobguiWidget *tree_data;
  BobguiWidget *list_data;
  BobguiWidget *actions;
  BobguiWidget *shortcuts;
  BobguiWidget *menu;
  BobguiWidget *misc_info;
  BobguiWidget *controllers;
  BobguiWidget *magnifier;
  BobguiWidget *a11y;
  BobguiWidget *svg;
  BobguiWidget *sidebar_revealer;
  BobguiWidget *css_editor;
  BobguiWidget *visual;
  BobguiWidget *clipboard;
  BobguiWidget *general;
  BobguiWidget *logs;

  BobguiWidget *go_up_button;
  BobguiWidget *go_down_button;
  BobguiWidget *go_previous_button;
  BobguiWidget *list_position_label;
  BobguiWidget *go_next_button;

  GList *extra_pages;

  GdkSeat *grab_seat;

  BobguiInspectorOverlay *flash_overlay;
  int flash_count;
  int flash_cnx;

  GArray *objects;

  GList *overlays;

  GdkDisplay *inspected_display;

} BobguiInspectorWindow;

typedef struct
{
  BobguiWindowClass parent;
} BobguiInspectorWindowClass;


G_BEGIN_DECLS

GType      bobgui_inspector_window_get_type    (void);
BobguiWidget *bobgui_inspector_window_get         (GdkDisplay *display);

void       bobgui_inspector_flash_widget       (BobguiInspectorWindow *iw,
                                             BobguiWidget          *widget);

void       bobgui_inspector_on_inspect         (BobguiWidget          *widget,
                                             BobguiInspectorWindow *iw);

void                    bobgui_inspector_window_add_overlay                        (BobguiInspectorWindow     *iw,
                                                                                 BobguiInspectorOverlay    *overlay);
void                    bobgui_inspector_window_remove_overlay                     (BobguiInspectorWindow     *iw,
                                                                                 BobguiInspectorOverlay    *overlay);

void                    bobgui_inspector_window_select_widget_under_pointer        (BobguiInspectorWindow     *iw);
GdkDisplay *            bobgui_inspector_window_get_inspected_display              (BobguiInspectorWindow     *iw);

typedef enum
{
  CHILD_KIND_WIDGET,
  CHILD_KIND_CONTROLLER,
  CHILD_KIND_PROPERTY,
  CHILD_KIND_LISTITEM,
  CHILD_KIND_OTHER
} ChildKind;

void                    bobgui_inspector_window_push_object     (BobguiInspectorWindow *iw,
                                                              GObject            *object,
                                                              ChildKind           kind,
                                                              guint               position);
void                    bobgui_inspector_window_pop_object      (BobguiInspectorWindow *iw);
void                    bobgui_inspector_window_set_object      (BobguiInspectorWindow *iw,
                                                              GObject            *object,
                                                              ChildKind           kind,
                                                              guint               position);
void                    bobgui_inspector_window_replace_object  (BobguiInspectorWindow *iw,
                                                              GObject            *object,
                                                              ChildKind           kind,
                                                              guint               position);

gboolean                bobgui_inspector_is_recording           (BobguiWidget            *widget);
GskRenderNode *         bobgui_inspector_prepare_render         (BobguiWidget            *widget,
                                                              GskRenderer          *renderer,
                                                              GdkSurface           *surface,
                                                              const cairo_region_t *region,
                                                              GskRenderNode        *root,
                                                              GskRenderNode        *widget_node);
gboolean                bobgui_inspector_handle_event           (GdkEvent             *event);
void                    bobgui_inspector_trace_event            (GdkEvent             *event,
                                                              BobguiPropagationPhase   phase,
                                                              BobguiWidget            *widget,
                                                              BobguiEventController   *controller,
                                                              BobguiWidget            *target,
                                                              gboolean              handled);
void                    bobgui_inspector_add_profile_node       (GdkDisplay           *display,
                                                              GskRenderNode        *node,
                                                              GskRenderNode        *profile_node);


G_END_DECLS



// vim: set et sw=2 ts=2:
