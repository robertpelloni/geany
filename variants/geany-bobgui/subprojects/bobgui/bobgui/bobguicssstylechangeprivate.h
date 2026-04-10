/* BOBGUI - The Bobgui Framework
 * Copyright (C) 2015 Benjamin Otte <otte@gnome.org>
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

#include "bobguicssstyleprivate.h"

G_BEGIN_DECLS

struct _BobguiCssStyleChange {
  BobguiCssStyle   *old_style;
  BobguiCssStyle   *new_style;

  BobguiCssAffects  affects;
  BobguiBitmask    *changes;
};

void            bobgui_css_style_change_init               (BobguiCssStyleChange      *change,
                                                         BobguiCssStyle            *old_style,
                                                         BobguiCssStyle            *new_style);
void            bobgui_css_style_change_finish             (BobguiCssStyleChange      *change);

BobguiCssStyle *   bobgui_css_style_change_get_old_style      (BobguiCssStyleChange      *change);
BobguiCssStyle *   bobgui_css_style_change_get_new_style      (BobguiCssStyleChange      *change);

gboolean        bobgui_css_style_change_has_change         (BobguiCssStyleChange      *change);
gboolean        bobgui_css_style_change_affects            (BobguiCssStyleChange      *change,
                                                         BobguiCssAffects           affects);
gboolean        bobgui_css_style_change_changes_property   (BobguiCssStyleChange      *change,
                                                         guint                   id);
void            bobgui_css_style_change_print              (BobguiCssStyleChange      *change, GString *string);

char *          bobgui_css_style_change_to_string          (BobguiCssStyleChange      *change);
G_END_DECLS

