/* BOBGUI - The Bobgui Framework
 * Copyright (C) 2014 Benjamin Otte <otte@gnome.org>
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

#include "bobguicssnodeprivate.h"
#include "bobguiwidget.h"

G_BEGIN_DECLS

#define BOBGUI_TYPE_CSS_WIDGET_NODE           (bobgui_css_widget_node_get_type ())
#define BOBGUI_CSS_WIDGET_NODE(obj)           (G_TYPE_CHECK_INSTANCE_CAST (obj, BOBGUI_TYPE_CSS_WIDGET_NODE, BobguiCssWidgetNode))
#define BOBGUI_CSS_WIDGET_NODE_CLASS(cls)     (G_TYPE_CHECK_CLASS_CAST (cls, BOBGUI_TYPE_CSS_WIDGET_NODE, BobguiCssWidgetNodeClass))
#define BOBGUI_IS_CSS_WIDGET_NODE(obj)        (G_TYPE_CHECK_INSTANCE_TYPE (obj, BOBGUI_TYPE_CSS_WIDGET_NODE))
#define BOBGUI_IS_CSS_WIDGET_NODE_CLASS(obj)  (G_TYPE_CHECK_CLASS_TYPE (obj, BOBGUI_TYPE_CSS_WIDGET_NODE))
#define BOBGUI_CSS_WIDGET_NODE_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), BOBGUI_TYPE_CSS_WIDGET_NODE, BobguiCssWidgetNodeClass))

typedef struct _BobguiCssWidgetNode           BobguiCssWidgetNode;
typedef struct _BobguiCssWidgetNodeClass      BobguiCssWidgetNodeClass;

struct _BobguiCssWidgetNode
{
  BobguiCssNode node;

  BobguiWidget *widget;
  guint validate_cb_id;
  BobguiCssStyle *last_updated_style;
};

struct _BobguiCssWidgetNodeClass
{
  BobguiCssNodeClass node_class;
};

GType                   bobgui_css_widget_node_get_type            (void) G_GNUC_CONST;

BobguiCssNode *            bobgui_css_widget_node_new                 (BobguiWidget              *widget);

void                    bobgui_css_widget_node_widget_destroyed    (BobguiCssWidgetNode       *node);

BobguiWidget *             bobgui_css_widget_node_get_widget          (BobguiCssWidgetNode       *node);

G_END_DECLS

