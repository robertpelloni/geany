/*
 * Copyright © 2014 Benjamin Otte <otte@gnome.org>
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
 */

#pragma once

#include "bobguicountingbloomfilterprivate.h"
#include "bobguicsstypesprivate.h"
#include "bobguienums.h"

G_BEGIN_DECLS

BobguiCssNodeDeclaration * bobgui_css_node_declaration_new                    (void);
BobguiCssNodeDeclaration * bobgui_css_node_declaration_ref                    (BobguiCssNodeDeclaration         *decl);
void                    bobgui_css_node_declaration_unref                  (BobguiCssNodeDeclaration         *decl);

gboolean                bobgui_css_node_declaration_set_name               (BobguiCssNodeDeclaration        **decl,
                                                                         GQuark                         name);
GQuark                  bobgui_css_node_declaration_get_name               (const BobguiCssNodeDeclaration   *decl);
gboolean                bobgui_css_node_declaration_set_id                 (BobguiCssNodeDeclaration        **decl,
                                                                         GQuark                         id);
GQuark                  bobgui_css_node_declaration_get_id                 (const BobguiCssNodeDeclaration   *decl);
gboolean                bobgui_css_node_declaration_set_state              (BobguiCssNodeDeclaration        **decl,
                                                                         BobguiStateFlags                  flags);
BobguiStateFlags           bobgui_css_node_declaration_get_state              (const BobguiCssNodeDeclaration   *decl);

gboolean                bobgui_css_node_declaration_add_class              (BobguiCssNodeDeclaration        **decl,
                                                                         GQuark                         class_quark);
gboolean                bobgui_css_node_declaration_remove_class           (BobguiCssNodeDeclaration        **decl,
                                                                         GQuark                         class_quark);
gboolean                bobgui_css_node_declaration_clear_classes          (BobguiCssNodeDeclaration        **decl);
gboolean                bobgui_css_node_declaration_has_class              (const BobguiCssNodeDeclaration   *decl,
                                                                         GQuark                         class_quark);
const GQuark *          bobgui_css_node_declaration_get_classes            (const BobguiCssNodeDeclaration   *decl,
                                                                         guint                         *n_classes);

void                    bobgui_css_node_declaration_add_bloom_hashes       (const BobguiCssNodeDeclaration   *decl,
                                                                         BobguiCountingBloomFilter        *filter);
void                    bobgui_css_node_declaration_remove_bloom_hashes    (const BobguiCssNodeDeclaration   *decl,
                                                                         BobguiCountingBloomFilter        *filter);

guint                   bobgui_css_node_declaration_hash                   (gconstpointer                  elem);
gboolean                bobgui_css_node_declaration_equal                  (gconstpointer                  elem1,
                                                                         gconstpointer                  elem2);

void                    bobgui_css_node_declaration_print                  (const BobguiCssNodeDeclaration   *decl,
                                                                         GString                       *string);

char *                  bobgui_css_node_declaration_to_string              (const BobguiCssNodeDeclaration   *decl);

G_END_DECLS

