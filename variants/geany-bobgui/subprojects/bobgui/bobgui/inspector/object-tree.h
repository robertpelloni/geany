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

#include <bobgui/bobguibox.h>
#include <bobgui/deprecated/bobguitreemodel.h>

#define BOBGUI_TYPE_INSPECTOR_OBJECT_TREE            (bobgui_inspector_object_tree_get_type())
#define BOBGUI_INSPECTOR_OBJECT_TREE(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), BOBGUI_TYPE_INSPECTOR_OBJECT_TREE, BobguiInspectorObjectTree))
#define BOBGUI_INSPECTOR_OBJECT_TREE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), BOBGUI_TYPE_INSPECTOR_OBJECT_TREE, BobguiInspectorObjectTreeClass))
#define BOBGUI_INSPECTOR_IS_OBJECT_TREE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), BOBGUI_TYPE_INSPECTOR_OBJECT_TREE))
#define BOBGUI_INSPECTOR_IS_OBJECT_TREE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), BOBGUI_TYPE_INSPECTOR_OBJECT_TREE))
#define BOBGUI_INSPECTOR_OBJECT_TREE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), BOBGUI_TYPE_INSPECTOR_OBJECT_TREE, BobguiInspectorObjectTreeClass))


typedef struct _BobguiInspectorObjectTreePrivate BobguiInspectorObjectTreePrivate;

typedef struct _BobguiInspectorObjectTree
{
  BobguiBox parent;
  BobguiInspectorObjectTreePrivate *priv;
} BobguiInspectorObjectTree;

typedef struct _BobguiInspectorObjectTreeClass
{
  BobguiBoxClass parent;

  void (*object_selected)  (BobguiInspectorObjectTree *wt,
                            GObject                *object);
  void (*object_activated) (BobguiInspectorObjectTree *wt,
                            GObject                *object);
} BobguiInspectorObjectTreeClass;


G_BEGIN_DECLS


GType      bobgui_inspector_object_tree_get_type            (void);

char *     bobgui_inspector_get_object_title                (GObject                *object);

void       bobgui_inspector_object_tree_select_object       (BobguiInspectorObjectTree *wt,
                                                          GObject                *object);
void       bobgui_inspector_object_tree_activate_object     (BobguiInspectorObjectTree *wt,
                                                          GObject                *object);

GObject   *bobgui_inspector_object_tree_get_selected        (BobguiInspectorObjectTree *wt);

void       bobgui_inspector_object_tree_set_display         (BobguiInspectorObjectTree *wt,
                                                          GdkDisplay             *display);

G_END_DECLS



// vim: set et sw=2 ts=2:
