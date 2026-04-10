/*
 * Copyright © 2012 Red Hat Inc.
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

#pragma once

#include "bobgui/bobguicssstyleprivate.h"

#include "bobgui/bobguicountingbloomfilterprivate.h"

G_BEGIN_DECLS

#define BOBGUI_TYPE_CSS_STATIC_STYLE           (bobgui_css_static_style_get_type ())
#define BOBGUI_CSS_STATIC_STYLE(obj)           (G_TYPE_CHECK_INSTANCE_CAST (obj, BOBGUI_TYPE_CSS_STATIC_STYLE, BobguiCssStaticStyle))
#define BOBGUI_CSS_STATIC_STYLE_CLASS(cls)     (G_TYPE_CHECK_CLASS_CAST (cls, BOBGUI_TYPE_CSS_STATIC_STYLE, BobguiCssStaticStyleClass))
#define BOBGUI_IS_CSS_STATIC_STYLE(obj)        (G_TYPE_CHECK_INSTANCE_TYPE (obj, BOBGUI_TYPE_CSS_STATIC_STYLE))
#define BOBGUI_IS_CSS_STATIC_STYLE_CLASS(obj)  (G_TYPE_CHECK_CLASS_TYPE (obj, BOBGUI_TYPE_CSS_STATIC_STYLE))
#define BOBGUI_CSS_STATIC_STYLE_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), BOBGUI_TYPE_CSS_STATIC_STYLE, BobguiCssStaticStyleClass))

typedef struct _BobguiCssStaticStyleClass      BobguiCssStaticStyleClass;


struct _BobguiCssStaticStyle
{
  BobguiCssStyle parent;

  GPtrArray             *sections;             /* sections the values are defined in */
  GPtrArray             *original_values;

  BobguiCssChange           change;               /* change as returned by value lookup */
};

struct _BobguiCssStaticStyleClass
{
  BobguiCssStyleClass parent_class;
};

GType                   bobgui_css_static_style_get_type           (void) G_GNUC_CONST;

BobguiCssStyle *           bobgui_css_static_style_get_default        (void);
BobguiCssStyle *           bobgui_css_static_style_new_compute        (BobguiStyleProvider               *provider,
                                                                 const BobguiCountingBloomFilter   *filter,
                                                                 BobguiCssNode                     *node,
                                                                 BobguiCssChange                    change);
BobguiCssChange            bobgui_css_static_style_get_change         (BobguiCssStaticStyle              *style);

G_END_DECLS

