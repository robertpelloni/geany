/* BOBGUI - The Bobgui Framework
 * Copyright © 2013 Carlos Garnacho <carlosg@gnome.org>
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

G_BEGIN_DECLS

#define BOBGUI_TYPE_MAGNIFIER           (bobgui_magnifier_get_type ())
#define BOBGUI_MAGNIFIER(o)             (G_TYPE_CHECK_INSTANCE_CAST ((o), BOBGUI_TYPE_MAGNIFIER, BobguiMagnifier))
#define BOBGUI_IS_MAGNIFIER(o)          (G_TYPE_CHECK_INSTANCE_TYPE ((o), BOBGUI_TYPE_MAGNIFIER))

typedef struct _BobguiMagnifier BobguiMagnifier;

GType       bobgui_magnifier_get_type           (void) G_GNUC_CONST;

BobguiWidget * _bobgui_magnifier_new               (BobguiWidget       *inspected);

BobguiWidget * _bobgui_magnifier_get_inspected     (BobguiMagnifier *magnifier);
void        _bobgui_magnifier_set_inspected     (BobguiMagnifier *magnifier,
                                              BobguiWidget    *inspected);

void        _bobgui_magnifier_set_coords        (BobguiMagnifier *magnifier,
                                              double        x,
                                              double        y);
void        _bobgui_magnifier_get_coords        (BobguiMagnifier *magnifier,
                                              double       *x,
                                              double       *y);

void        _bobgui_magnifier_set_magnification (BobguiMagnifier *magnifier,
                                              double        magnification);
double      _bobgui_magnifier_get_magnification (BobguiMagnifier *magnifier);

void        _bobgui_magnifier_set_resize        (BobguiMagnifier *magnifier,
                                              gboolean      resize);
gboolean    _bobgui_magnifier_get_resize        (BobguiMagnifier *magnifier);

G_END_DECLS

