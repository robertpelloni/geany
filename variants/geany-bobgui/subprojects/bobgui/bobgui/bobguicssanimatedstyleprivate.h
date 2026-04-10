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

G_BEGIN_DECLS

#define BOBGUI_TYPE_CSS_ANIMATED_STYLE           (bobgui_css_animated_style_get_type ())
#define BOBGUI_CSS_ANIMATED_STYLE(obj)           (G_TYPE_CHECK_INSTANCE_CAST (obj, BOBGUI_TYPE_CSS_ANIMATED_STYLE, BobguiCssAnimatedStyle))
#define BOBGUI_CSS_ANIMATED_STYLE_CLASS(cls)     (G_TYPE_CHECK_CLASS_CAST (cls, BOBGUI_TYPE_CSS_ANIMATED_STYLE, BobguiCssAnimatedStyleClass))
#define BOBGUI_IS_CSS_ANIMATED_STYLE(obj)        (G_TYPE_CHECK_INSTANCE_TYPE (obj, BOBGUI_TYPE_CSS_ANIMATED_STYLE))
#define BOBGUI_IS_CSS_ANIMATED_STYLE_CLASS(obj)  (G_TYPE_CHECK_CLASS_TYPE (obj, BOBGUI_TYPE_CSS_ANIMATED_STYLE))
#define BOBGUI_CSS_ANIMATED_STYLE_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), BOBGUI_TYPE_CSS_ANIMATED_STYLE, BobguiCssAnimatedStyleClass))

typedef struct _BobguiCssAnimatedStyle           BobguiCssAnimatedStyle;
typedef struct _BobguiCssAnimatedStyleClass      BobguiCssAnimatedStyleClass;

struct _BobguiCssAnimatedStyle
{
  BobguiCssStyle parent;

  BobguiCssStyle           *style;                /* the style if we weren't animating */
  BobguiCssStyle           *parent_style;
  BobguiStyleProvider      *provider;

  gint64                 current_time;         /* the current time in our world */
  gpointer              *animations;           /* BobguiStyleAnimation**, least important one first */
  guint                  n_animations;
};

struct _BobguiCssAnimatedStyleClass
{
  BobguiCssStyleClass parent_class;
};

GType                   bobgui_css_animated_style_get_type         (void) G_GNUC_CONST;

BobguiCssStyle *           bobgui_css_animated_style_new              (BobguiCssStyle            *base_style,
                                                                 BobguiCssStyle            *parent_style,
                                                                 gint64                  timestamp,
                                                                 BobguiStyleProvider       *provider,
                                                                 BobguiCssStyle            *previous_style);
BobguiCssStyle *           bobgui_css_animated_style_new_advance      (BobguiCssAnimatedStyle    *source,
                                                                 BobguiCssStyle            *base_style,
                                                                 BobguiCssStyle            *parent_style,
                                                                 gint64                  timestamp,
                                                                 BobguiStyleProvider       *provider);

void                    bobgui_css_animated_style_set_animated_value(BobguiCssAnimatedStyle   *style,
                                                                 guint                   id,
                                                                 BobguiCssValue            *value);
BobguiCssValue *           bobgui_css_animated_style_get_intrinsic_value (BobguiCssAnimatedStyle *style,
                                                                 guint                   id);

gboolean                bobgui_css_animated_style_set_animated_custom_value (BobguiCssAnimatedStyle *animated,
                                                                 int                     id,
                                                                 BobguiCssVariableValue    *value);

void                    bobgui_css_animated_style_recompute        (BobguiCssAnimatedStyle    *style);
BobguiCssVariableValue *   bobgui_css_animated_style_get_intrinsic_custom_value (BobguiCssAnimatedStyle *style,
                                                                 int                     id);

BobguiCssStyle *           bobgui_css_animated_style_get_base_style   (BobguiCssAnimatedStyle    *style);
BobguiCssStyle *           bobgui_css_animated_style_get_parent_style (BobguiCssAnimatedStyle    *style);
BobguiStyleProvider *      bobgui_css_animated_style_get_provider     (BobguiCssAnimatedStyle    *style);

G_END_DECLS

