/* BOBGUI - The Bobgui Framework
 * Copyright (C) 2010 Carlos Garnacho <carlosg@gnome.org>
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

#include <bobgui/css/bobguicss.h>
#include "bobgui/css/bobguicsstokenizerprivate.h"
#include "bobgui/css/bobguicssparserprivate.h"
#include "bobguicssvalueprivate.h"

G_BEGIN_DECLS

#define BOBGUI_TYPE_STYLE_PROPERTY           (_bobgui_style_property_get_type ())
#define BOBGUI_STYLE_PROPERTY(obj)           (G_TYPE_CHECK_INSTANCE_CAST (obj, BOBGUI_TYPE_STYLE_PROPERTY, BobguiStyleProperty))
#define BOBGUI_STYLE_PROPERTY_CLASS(cls)     (G_TYPE_CHECK_CLASS_CAST (cls, BOBGUI_TYPE_STYLE_PROPERTY, BobguiStylePropertyClass))
#define BOBGUI_IS_STYLE_PROPERTY(obj)        (G_TYPE_CHECK_INSTANCE_TYPE (obj, BOBGUI_TYPE_STYLE_PROPERTY))
#define BOBGUI_IS_STYLE_PROPERTY_CLASS(obj)  (G_TYPE_CHECK_CLASS_TYPE (obj, BOBGUI_TYPE_STYLE_PROPERTY))
#define BOBGUI_STYLE_PROPERTY_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), BOBGUI_TYPE_STYLE_PROPERTY, BobguiStylePropertyClass))

typedef struct _BobguiStyleProperty           BobguiStyleProperty;
typedef struct _BobguiStylePropertyClass      BobguiStylePropertyClass;

typedef BobguiCssValue *   (* BobguiStyleQueryFunc)        (guint                   id,
						      gpointer                data);

struct _BobguiStyleProperty
{
  GObject parent;

  char *name;
};

struct _BobguiStylePropertyClass
{
  GObjectClass  parent_class;
  
  BobguiCssValue *     (* parse_value)                        (BobguiStyleProperty *      property,
                                                            BobguiCssParser           *parser);

  GHashTable   *properties;
};

GType               _bobgui_style_property_get_type             (void) G_GNUC_CONST;

void                _bobgui_style_property_init_properties      (void);

BobguiStyleProperty *       _bobgui_style_property_lookup        (const char             *name);

const char *             _bobgui_style_property_get_name      (BobguiStyleProperty       *property);

BobguiCssValue *            _bobgui_style_property_parse_value   (BobguiStyleProperty *      property,
                                                            BobguiCssParser           *parser);

G_END_DECLS

