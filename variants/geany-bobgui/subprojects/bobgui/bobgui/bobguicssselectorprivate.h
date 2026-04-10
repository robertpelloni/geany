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

#include <bobgui/css/bobguicss.h>
#include "bobgui/css/bobguicsstokenizerprivate.h"
#include "bobgui/css/bobguicssparserprivate.h"
#include "bobgui/bobguicountingbloomfilterprivate.h"
#include "bobgui/bobguicsstypesprivate.h"

#define GDK_ARRAY_ELEMENT_TYPE gpointer
#define GDK_ARRAY_TYPE_NAME BobguiCssSelectorMatches
#define GDK_ARRAY_NAME bobgui_css_selector_matches
#define GDK_ARRAY_PREALLOC 32
#include "gdk/gdkarrayimpl.c"

G_BEGIN_DECLS

typedef union _BobguiCssSelector BobguiCssSelector;
typedef struct _BobguiCssSelectorTree BobguiCssSelectorTree;
typedef struct _BobguiCssSelectorTreeBuilder BobguiCssSelectorTreeBuilder;

BobguiCssSelector *  _bobgui_css_selector_parse           (BobguiCssParser           *parser);
void              _bobgui_css_selector_free            (BobguiCssSelector         *selector);

char *            _bobgui_css_selector_to_string       (const BobguiCssSelector   *selector);
void              _bobgui_css_selector_print           (const BobguiCssSelector   *selector,
                                                     GString                *str);

gboolean          bobgui_css_selector_matches          (const BobguiCssSelector   *selector,
						     BobguiCssNode             *node);
BobguiCssChange      _bobgui_css_selector_get_change      (const BobguiCssSelector   *selector);
int               _bobgui_css_selector_compare         (const BobguiCssSelector   *a,
                                                     const BobguiCssSelector   *b);

void         _bobgui_css_selector_tree_free             (BobguiCssSelectorTree       *tree);
void         _bobgui_css_selector_tree_match_all        (const BobguiCssSelectorTree *tree,
                                                      const BobguiCountingBloomFilter *filter,
                                                      BobguiCssNode               *node,
                                                      BobguiCssSelectorMatches    *out_tree_rules);
BobguiCssChange bobgui_css_selector_tree_get_change_all    (const BobguiCssSelectorTree *tree,
                                                      const BobguiCountingBloomFilter *filter,
						      BobguiCssNode               *node);
void         _bobgui_css_selector_tree_match_print      (const BobguiCssSelectorTree *tree,
						      GString                  *str);
gboolean     _bobgui_css_selector_tree_is_empty         (const BobguiCssSelectorTree *tree) G_GNUC_CONST;



BobguiCssSelectorTreeBuilder *_bobgui_css_selector_tree_builder_new   (void);
void                       _bobgui_css_selector_tree_builder_add   (BobguiCssSelectorTreeBuilder *builder,
								 BobguiCssSelector            *selectors,
								 BobguiCssSelectorTree       **selector_match,
								 gpointer                   match);
BobguiCssSelectorTree *       _bobgui_css_selector_tree_builder_build (BobguiCssSelectorTreeBuilder *builder);
void                       _bobgui_css_selector_tree_builder_free  (BobguiCssSelectorTreeBuilder *builder);

G_END_DECLS

