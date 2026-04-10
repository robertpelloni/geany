/* BOBGUI - The Bobgui Framework
 * Copyright (C) 2011 Benjamin Otte <otte@gnome.org>
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

#include <glib-object.h>

#include "bobgui/bobguibitmaskprivate.h"
#include "bobgui/bobguicssstaticstyleprivate.h"

#include "bobgui/css/bobguicsssection.h"
#include "bobgui/css/bobguicsstokenizerprivate.h"
#include "bobgui/css/bobguicssvariablevalueprivate.h"


G_BEGIN_DECLS

typedef struct _BobguiCssLookup BobguiCssLookup;

typedef struct {
  BobguiCssSection     *section;
  BobguiCssValue       *value;
} BobguiCssLookupValue;

struct _BobguiCssLookup {
  BobguiBitmask *set_values;
  BobguiCssLookupValue  values[BOBGUI_CSS_PROPERTY_N_PROPERTIES];
  GHashTable *custom_values;
};

void                    _bobgui_css_lookup_init                    (BobguiCssLookup               *lookup);
void                    _bobgui_css_lookup_destroy                 (BobguiCssLookup               *lookup);
gboolean                _bobgui_css_lookup_is_missing              (const BobguiCssLookup         *lookup,
                                                                 guint                       id);
void                    _bobgui_css_lookup_set                     (BobguiCssLookup               *lookup,
                                                                 guint                       id,
                                                                 BobguiCssSection              *section,
                                                                 BobguiCssValue                *value);
void                    _bobgui_css_lookup_set_custom              (BobguiCssLookup               *lookup,
                                                                 int                         id,
                                                                 BobguiCssVariableValue        *value);

static inline const BobguiBitmask *
_bobgui_css_lookup_get_set_values (const BobguiCssLookup *lookup)
{
  return lookup->set_values;
}

G_END_DECLS

