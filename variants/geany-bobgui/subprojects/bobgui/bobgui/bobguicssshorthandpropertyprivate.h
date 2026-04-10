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

#include <glib-object.h>

#include <bobgui/css/bobguicss.h>
#include "bobgui/css/bobguicsstokenizerprivate.h"
#include "bobgui/css/bobguicssparserprivate.h"
#include "bobgui/bobguicssstylepropertyprivate.h"
#include "bobgui/bobguistylepropertyprivate.h"

G_BEGIN_DECLS

#define BOBGUI_TYPE_CSS_SHORTHAND_PROPERTY           (_bobgui_css_shorthand_property_get_type ())
#define BOBGUI_CSS_SHORTHAND_PROPERTY(obj)           (G_TYPE_CHECK_INSTANCE_CAST (obj, BOBGUI_TYPE_CSS_SHORTHAND_PROPERTY, BobguiCssShorthandProperty))
#define BOBGUI_CSS_SHORTHAND_PROPERTY_CLASS(cls)     (G_TYPE_CHECK_CLASS_CAST (cls, BOBGUI_TYPE_CSS_SHORTHAND_PROPERTY, BobguiCssShorthandPropertyClass))
#define BOBGUI_IS_CSS_SHORTHAND_PROPERTY(obj)        (G_TYPE_CHECK_INSTANCE_TYPE (obj, BOBGUI_TYPE_CSS_SHORTHAND_PROPERTY))
#define BOBGUI_IS_CSS_SHORTHAND_PROPERTY_CLASS(obj)  (G_TYPE_CHECK_CLASS_TYPE (obj, BOBGUI_TYPE_CSS_SHORTHAND_PROPERTY))
#define BOBGUI_CSS_SHORTHAND_PROPERTY_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), BOBGUI_TYPE_CSS_SHORTHAND_PROPERTY, BobguiCssShorthandPropertyClass))

typedef struct _BobguiCssShorthandProperty           BobguiCssShorthandProperty;
typedef struct _BobguiCssShorthandPropertyClass      BobguiCssShorthandPropertyClass;

typedef gboolean              (* BobguiCssShorthandPropertyParseFunc)      (BobguiCssShorthandProperty *shorthand,
                                                                         BobguiCssValue            **values,
                                                                         BobguiCssParser            *parser);

struct _BobguiCssShorthandProperty
{
  BobguiStyleProperty parent;

  GPtrArray *subproperties;

  guint id;

  BobguiCssShorthandPropertyParseFunc parse;
};

struct _BobguiCssShorthandPropertyClass
{
  BobguiStylePropertyClass parent_class;
};

void                    _bobgui_css_shorthand_property_init_properties     (void);

GType                   _bobgui_css_shorthand_property_get_type            (void) G_GNUC_CONST;

BobguiCssStyleProperty *   _bobgui_css_shorthand_property_get_subproperty     (BobguiCssShorthandProperty *shorthand,
                                                                         guint                    property);
guint                   _bobgui_css_shorthand_property_get_n_subproperties (BobguiCssShorthandProperty *shorthand) G_GNUC_CONST;

guint                   _bobgui_css_shorthand_property_get_id              (BobguiCssShorthandProperty *shorthand) G_GNUC_CONST;


G_END_DECLS

