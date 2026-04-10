/* bobguiconstraintlayout.h: Layout manager using constraints
 * Copyright 2019  GNOME Foundation
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
 * Author: Emmanuele Bassi
 */
#pragma once

#include <bobgui/bobguilayoutmanager.h>
#include <bobgui/bobguiconstraint.h>
#include <bobgui/bobguiconstraintguide.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_CONSTRAINT_LAYOUT (bobgui_constraint_layout_get_type ())
#define BOBGUI_TYPE_CONSTRAINT_LAYOUT_CHILD (bobgui_constraint_layout_child_get_type ())
#define BOBGUI_CONSTRAINT_VFL_PARSER_ERROR (bobgui_constraint_vfl_parser_error_quark ())

GDK_AVAILABLE_IN_ALL
G_DECLARE_FINAL_TYPE (BobguiConstraintLayoutChild, bobgui_constraint_layout_child, BOBGUI, CONSTRAINT_LAYOUT_CHILD, BobguiLayoutChild)

GDK_AVAILABLE_IN_ALL
G_DECLARE_FINAL_TYPE (BobguiConstraintLayout, bobgui_constraint_layout, BOBGUI, CONSTRAINT_LAYOUT, BobguiLayoutManager)

GDK_AVAILABLE_IN_ALL
GQuark                  bobgui_constraint_vfl_parser_error_quark   (void);

GDK_AVAILABLE_IN_ALL
BobguiLayoutManager *      bobgui_constraint_layout_new               (void);

GDK_AVAILABLE_IN_ALL
void                    bobgui_constraint_layout_add_constraint    (BobguiConstraintLayout *layout,
                                                                 BobguiConstraint       *constraint);
GDK_AVAILABLE_IN_ALL
void                    bobgui_constraint_layout_remove_constraint (BobguiConstraintLayout *layout,
                                                                 BobguiConstraint       *constraint);

GDK_AVAILABLE_IN_ALL
void                    bobgui_constraint_layout_add_guide         (BobguiConstraintLayout *layout,
                                                                 BobguiConstraintGuide  *guide);
GDK_AVAILABLE_IN_ALL
void                    bobgui_constraint_layout_remove_guide      (BobguiConstraintLayout *layout,
                                                                 BobguiConstraintGuide  *guide);
GDK_AVAILABLE_IN_ALL
void                    bobgui_constraint_layout_remove_all_constraints            (BobguiConstraintLayout *layout);

GDK_AVAILABLE_IN_ALL
GList *                 bobgui_constraint_layout_add_constraints_from_description  (BobguiConstraintLayout *layout,
                                                                                 const char * const   lines[],
                                                                                 gsize                n_lines,
                                                                                 int                  hspacing,
                                                                                 int                  vspacing,
                                                                                 GError             **error,
                                                                                 const char          *first_view,
                                                                                 ...) G_GNUC_NULL_TERMINATED;
GDK_AVAILABLE_IN_ALL
GList *                 bobgui_constraint_layout_add_constraints_from_descriptionv (BobguiConstraintLayout *layout,
                                                                                 const char * const   lines[],
                                                                                 gsize                n_lines,
                                                                                 int                  hspacing,
                                                                                 int                  vspacing,
                                                                                 GHashTable          *views,
                                                                                 GError             **error);

GDK_AVAILABLE_IN_ALL
GListModel *          bobgui_constraint_layout_observe_constraints (BobguiConstraintLayout *layout);
GDK_AVAILABLE_IN_ALL
GListModel *          bobgui_constraint_layout_observe_guides (BobguiConstraintLayout *layout);

G_END_DECLS
