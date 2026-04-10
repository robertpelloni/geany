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

#include "bobguicsstransientnodeprivate.h"
#include "bobguiprivate.h"

G_DEFINE_TYPE (BobguiCssTransientNode, bobgui_css_transient_node, BOBGUI_TYPE_CSS_NODE)

static BobguiCssStyle *
bobgui_css_transient_node_update_style (BobguiCssNode                   *cssnode,
                                     const BobguiCountingBloomFilter *filter,
                                     BobguiCssChange                  change,
                                     gint64                        timestamp,
                                     BobguiCssStyle                  *style)
{
  /* This should get rid of animations */
  return BOBGUI_CSS_NODE_CLASS (bobgui_css_transient_node_parent_class)->update_style (cssnode, filter, change, 0, style);
}

static void
bobgui_css_transient_node_class_init (BobguiCssTransientNodeClass *klass)
{
  BobguiCssNodeClass *node_class = BOBGUI_CSS_NODE_CLASS (klass);

  node_class->update_style = bobgui_css_transient_node_update_style;
}

static void
bobgui_css_transient_node_init (BobguiCssTransientNode *cssnode)
{
  bobgui_css_node_set_visible (BOBGUI_CSS_NODE (cssnode), FALSE);
}

BobguiCssNode *
bobgui_css_transient_node_new (BobguiCssNode *parent)
{
  BobguiCssNode *result;

  bobgui_internal_return_val_if_fail (BOBGUI_IS_CSS_NODE (parent), NULL);

  result = g_object_new (BOBGUI_TYPE_CSS_TRANSIENT_NODE, NULL);
  bobgui_css_node_declaration_unref (result->decl);
  result->decl = bobgui_css_node_declaration_ref (parent->decl);

  return result;
}

