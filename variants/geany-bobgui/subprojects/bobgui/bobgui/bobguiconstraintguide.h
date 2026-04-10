/* bobguiconstraintguide.h: Flexible space for constraints
 * Copyright 2019 Red Hat, Inc.
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
 * Author: Matthias Clasen
 */

#pragma once

#include <bobgui/bobguitypes.h>
#include <bobgui/bobguienums.h>
#include <bobgui/bobguitypebuiltins.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_CONSTRAINT_GUIDE (bobgui_constraint_guide_get_type ())

GDK_AVAILABLE_IN_ALL
G_DECLARE_FINAL_TYPE (BobguiConstraintGuide, bobgui_constraint_guide, BOBGUI, CONSTRAINT_GUIDE, GObject)

GDK_AVAILABLE_IN_ALL
BobguiConstraintGuide *    bobgui_constraint_guide_new                (void);

GDK_AVAILABLE_IN_ALL
void                    bobgui_constraint_guide_set_min_size       (BobguiConstraintGuide *guide,
                                                                 int                 width,
                                                                 int                 height);
GDK_AVAILABLE_IN_ALL
void                    bobgui_constraint_guide_get_min_size       (BobguiConstraintGuide *guide,
                                                                 int                *width,
                                                                 int                *height);
GDK_AVAILABLE_IN_ALL
void                    bobgui_constraint_guide_set_nat_size       (BobguiConstraintGuide *guide,
                                                                 int                 width,
                                                                 int                 height);
GDK_AVAILABLE_IN_ALL
void                    bobgui_constraint_guide_get_nat_size       (BobguiConstraintGuide *guide,
                                                                 int                *width,
                                                                 int                *height);
GDK_AVAILABLE_IN_ALL
void                    bobgui_constraint_guide_set_max_size       (BobguiConstraintGuide *guide,
                                                                 int                 width,
                                                                 int                 height);
GDK_AVAILABLE_IN_ALL
void                    bobgui_constraint_guide_get_max_size       (BobguiConstraintGuide *guide,
                                                                 int                *width,
                                                                 int                *height);

GDK_AVAILABLE_IN_ALL
BobguiConstraintStrength   bobgui_constraint_guide_get_strength       (BobguiConstraintGuide *guide);
GDK_AVAILABLE_IN_ALL
void                    bobgui_constraint_guide_set_strength       (BobguiConstraintGuide    *guide,
                                                                 BobguiConstraintStrength  strength);

GDK_AVAILABLE_IN_ALL
void                    bobgui_constraint_guide_set_name           (BobguiConstraintGuide *guide,
                                                                 const char         *name);
GDK_AVAILABLE_IN_ALL
const char *            bobgui_constraint_guide_get_name           (BobguiConstraintGuide *guide);

G_END_DECLS
