/*
 * Copyright (c) 2014 Benjamin Otte <otte@gnome.org>
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

#include "bobgui/bobguicssnodeprivate.h"

#define BOBGUI_TYPE_INSPECTOR_CSS_NODE_TREE            (bobgui_inspector_css_node_tree_get_type())
#define BOBGUI_INSPECTOR_CSS_NODE_TREE(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), BOBGUI_TYPE_INSPECTOR_CSS_NODE_TREE, BobguiInspectorCssNodeTree))
#define BOBGUI_INSPECTOR_CSS_NODE_TREE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), BOBGUI_TYPE_INSPECTOR_CSS_NODE_TREE, BobguiInspectorCssNodeTreeClass))
#define BOBGUI_INSPECTOR_IS_CSS_NODE_TREE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), BOBGUI_TYPE_INSPECTOR_CSS_NODE_TREE))
#define BOBGUI_INSPECTOR_IS_CSS_NODE_TREE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), BOBGUI_TYPE_INSPECTOR_CSS_NODE_TREE))
#define BOBGUI_INSPECTOR_CSS_NODE_TREE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), BOBGUI_TYPE_INSPECTOR_CSS_NODE_TREE, BobguiInspectorCssNodeTreeClass))


typedef struct _BobguiInspectorCssNodeTreePrivate BobguiInspectorCssNodeTreePrivate;

typedef struct _BobguiInspectorCssNodeTree
{
  BobguiBox parent;
  BobguiInspectorCssNodeTreePrivate *priv;
} BobguiInspectorCssNodeTree;

typedef struct _BobguiInspectorCssNodeTreeClass
{
  BobguiBoxClass parent;
} BobguiInspectorCssNodeTreeClass;

G_BEGIN_DECLS

GType           bobgui_inspector_css_node_tree_get_type            (void);
void            bobgui_inspector_css_node_tree_set_object          (BobguiInspectorCssNodeTree        *cnt,
                                                                 GObject                        *object);

BobguiCssNode *    bobgui_inspector_css_node_tree_get_node            (BobguiInspectorCssNodeTree        *cnt);
void            bobgui_inspector_css_node_tree_set_display         (BobguiInspectorCssNodeTree        *cnt,
                                                                 GdkDisplay                     *display);


G_END_DECLS


// vim: set et sw=2 ts=2:
