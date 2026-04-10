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

#include "config.h"

#include "bobguicsswidgetnodeprivate.h"

#include "bobguicssanimatedstyleprivate.h"
#include "bobguiprivate.h"
#include "bobguisettingsprivate.h"
#include "deprecated/bobguistylecontextprivate.h"
#include "bobguiwidgetprivate.h"
#include "bobguiwindowprivate.h"

G_DEFINE_TYPE (BobguiCssWidgetNode, bobgui_css_widget_node, BOBGUI_TYPE_CSS_NODE)

static void
bobgui_css_widget_node_finalize (GObject *object)
{
  BobguiCssWidgetNode *node = BOBGUI_CSS_WIDGET_NODE (object);

  g_object_unref (node->last_updated_style);

  G_OBJECT_CLASS (bobgui_css_widget_node_parent_class)->finalize (object);
}

static gboolean
bobgui_css_widget_node_queue_callback (BobguiWidget     *widget,
                                    GdkFrameClock *frame_clock,
                                    gpointer       user_data)
{
  BobguiCssNode *node = user_data;

  bobgui_css_node_invalidate_frame_clock (node, TRUE);
  bobgui_root_queue_restyle (BOBGUI_ROOT (widget));

  return G_SOURCE_CONTINUE;
}

static void
bobgui_css_widget_node_queue_validate (BobguiCssNode *node)
{
  BobguiCssWidgetNode *widget_node = BOBGUI_CSS_WIDGET_NODE (node);

  if (BOBGUI_IS_ROOT (widget_node->widget))
    widget_node->validate_cb_id = bobgui_widget_add_tick_callback (widget_node->widget,
                                                                bobgui_css_widget_node_queue_callback,
                                                                node,
                                                                NULL);
}

static void
bobgui_css_widget_node_dequeue_validate (BobguiCssNode *node)
{
  BobguiCssWidgetNode *widget_node = BOBGUI_CSS_WIDGET_NODE (node);

  if (widget_node->widget && BOBGUI_IS_ROOT (widget_node->widget))
    bobgui_widget_remove_tick_callback (widget_node->widget,
                                     widget_node->validate_cb_id);
}

static void
bobgui_css_widget_node_validate (BobguiCssNode *node)
{
  BobguiCssWidgetNode *widget_node = BOBGUI_CSS_WIDGET_NODE (node);
  BobguiCssStyleChange change;
  BobguiCssStyle *style;

  if (widget_node->widget == NULL)
    return;

  if (node->style == widget_node->last_updated_style)
    return;

  style = bobgui_css_node_get_style (node);

  bobgui_css_style_change_init (&change, widget_node->last_updated_style, style);
  if (bobgui_css_style_change_has_change (&change))
    {
      bobgui_widget_css_changed (widget_node->widget, &change);
      g_set_object (&widget_node->last_updated_style, style);
    }
  bobgui_css_style_change_finish (&change);
}

static BobguiStyleProvider *
bobgui_css_widget_node_get_style_provider (BobguiCssNode *node)
{
  BobguiCssWidgetNode *widget_node = BOBGUI_CSS_WIDGET_NODE (node);
  BobguiStyleContext *context;
  BobguiStyleCascade *cascade;

  if (widget_node->widget == NULL)
    return NULL;

  context = _bobgui_widget_peek_style_context (widget_node->widget);
  if (context)
    return bobgui_style_context_get_style_provider (context);

  cascade = _bobgui_settings_get_style_cascade (bobgui_widget_get_settings (widget_node->widget),
                                             bobgui_widget_get_scale_factor (widget_node->widget));
  return BOBGUI_STYLE_PROVIDER (cascade);
}

static GdkFrameClock *
bobgui_css_widget_node_get_frame_clock (BobguiCssNode *node)
{
  BobguiCssWidgetNode *widget_node = BOBGUI_CSS_WIDGET_NODE (node);

  if (widget_node->widget == NULL)
    return NULL;

  if (!bobgui_settings_get_enable_animations (bobgui_widget_get_settings (widget_node->widget)))
    return NULL;

  return bobgui_widget_get_frame_clock (widget_node->widget);
}

static void
bobgui_css_widget_node_class_init (BobguiCssWidgetNodeClass *klass)
{
  BobguiCssNodeClass *node_class = BOBGUI_CSS_NODE_CLASS (klass);
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = bobgui_css_widget_node_finalize;
  node_class->validate = bobgui_css_widget_node_validate;
  node_class->queue_validate = bobgui_css_widget_node_queue_validate;
  node_class->dequeue_validate = bobgui_css_widget_node_dequeue_validate;
  node_class->get_style_provider = bobgui_css_widget_node_get_style_provider;
  node_class->get_frame_clock = bobgui_css_widget_node_get_frame_clock;
}

static void
bobgui_css_widget_node_init (BobguiCssWidgetNode *node)
{
  node->last_updated_style = g_object_ref (bobgui_css_static_style_get_default ());
}

BobguiCssNode *
bobgui_css_widget_node_new (BobguiWidget *widget)
{
  BobguiCssWidgetNode *result;

  bobgui_internal_return_val_if_fail (BOBGUI_IS_WIDGET (widget), NULL);

  result = g_object_new (BOBGUI_TYPE_CSS_WIDGET_NODE, NULL);
  result->widget = widget;
  bobgui_css_node_set_visible (BOBGUI_CSS_NODE (result),
                            _bobgui_widget_get_visible (widget));

  return BOBGUI_CSS_NODE (result);
}

void
bobgui_css_widget_node_widget_destroyed (BobguiCssWidgetNode *node)
{
  bobgui_internal_return_if_fail (BOBGUI_IS_CSS_WIDGET_NODE (node));
  bobgui_internal_return_if_fail (node->widget != NULL);

  node->widget = NULL;
  /* Contents of this node are now undefined.
   * So we don't clear the style or anything.
   */
}

BobguiWidget *
bobgui_css_widget_node_get_widget (BobguiCssWidgetNode *node)
{
  bobgui_internal_return_val_if_fail (BOBGUI_IS_CSS_WIDGET_NODE (node), NULL);

  return node->widget;
}

