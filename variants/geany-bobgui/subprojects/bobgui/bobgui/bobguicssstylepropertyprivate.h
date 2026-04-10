/*
 * Copyright © 2011 Red Hat Inc.
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

#include "bobgui/bobguistylepropertyprivate.h"

G_BEGIN_DECLS

#define BOBGUI_TYPE_CSS_STYLE_PROPERTY           (_bobgui_css_style_property_get_type ())
#define BOBGUI_CSS_STYLE_PROPERTY(obj)           (G_TYPE_CHECK_INSTANCE_CAST (obj, BOBGUI_TYPE_CSS_STYLE_PROPERTY, BobguiCssStyleProperty))
#define BOBGUI_CSS_STYLE_PROPERTY_CLASS(cls)     (G_TYPE_CHECK_CLASS_CAST (cls, BOBGUI_TYPE_CSS_STYLE_PROPERTY, BobguiCssStylePropertyClass))
#define BOBGUI_IS_CSS_STYLE_PROPERTY(obj)        (G_TYPE_CHECK_INSTANCE_TYPE (obj, BOBGUI_TYPE_CSS_STYLE_PROPERTY))
#define BOBGUI_IS_CSS_STYLE_PROPERTY_CLASS(obj)  (G_TYPE_CHECK_CLASS_TYPE (obj, BOBGUI_TYPE_CSS_STYLE_PROPERTY))
#define BOBGUI_CSS_STYLE_PROPERTY_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), BOBGUI_TYPE_CSS_STYLE_PROPERTY, BobguiCssStylePropertyClass))

typedef struct _BobguiCssStyleProperty           BobguiCssStyleProperty;
typedef struct _BobguiCssStylePropertyClass      BobguiCssStylePropertyClass;

typedef BobguiCssValue *    (* BobguiCssStylePropertyParseFunc)  (BobguiCssStyleProperty    *property,
                                                            BobguiCssParser           *parser);
typedef void             (* BobguiCssStylePropertyQueryFunc)  (BobguiCssStyleProperty    *property,
                                                            BobguiCssValue            *cssvalue,
                                                            GValue                 *value);
struct _BobguiCssStyleProperty
{
  BobguiStyleProperty parent;

  BobguiCssValue *initial_value;
  guint id;
  BobguiCssAffects affects;
  guint inherit :1;
  guint animated :1;

  BobguiCssStylePropertyParseFunc parse_value;
  BobguiCssStylePropertyQueryFunc query_value;
};

struct _BobguiCssStylePropertyClass
{
  BobguiStylePropertyClass parent_class;

  GPtrArray *style_properties;
};

GType                   _bobgui_css_style_property_get_type        (void) G_GNUC_CONST;

void                    _bobgui_css_style_property_init_properties (void);

guint                   _bobgui_css_style_property_get_n_properties(void) G_GNUC_CONST;
BobguiCssStyleProperty *   _bobgui_css_style_property_lookup_by_id    (guint                   id);

gboolean                _bobgui_css_style_property_is_inherit      (BobguiCssStyleProperty    *property);
gboolean                _bobgui_css_style_property_is_animated     (BobguiCssStyleProperty    *property);
BobguiCssAffects           _bobgui_css_style_property_get_affects     (BobguiCssStyleProperty    *property);
gboolean                _bobgui_css_style_property_affects_size    (BobguiCssStyleProperty    *property);
gboolean                _bobgui_css_style_property_affects_font    (BobguiCssStyleProperty    *property);
guint                   _bobgui_css_style_property_get_id          (BobguiCssStyleProperty    *property);
BobguiCssValue  *          _bobgui_css_style_property_get_initial_value
                                                                (BobguiCssStyleProperty    *property);

void                    _bobgui_css_style_property_print_value     (BobguiCssStyleProperty    *property,
                                                                 BobguiCssValue            *value,
                                                                 GString                *string);

/* XXX - find a better place for these */
BobguiCssValue * bobgui_css_font_family_value_parse (BobguiCssParser *parser);
BobguiCssValue * bobgui_css_font_size_value_parse   (BobguiCssParser *parser);

G_END_DECLS

