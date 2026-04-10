/* BOBGUI - The Bobgui Framework
 * Copyright (C) 2011 Red Hat, Inc.
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

#if !defined (__BOBGUI_CSS_H_INSIDE__) && !defined (BOBGUI_COMPILATION)
#error "Only <bobgui/css/bobguicss.h> can be included directly."
#endif

#include <gio/gio.h>
#include <gdk/version/gdkversionmacros.h>
#include <bobgui/css/bobguicsslocation.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_CSS_SECTION         (bobgui_css_section_get_type ())

/**
 * BobguiCssSection:
 *
 * Defines a part of a CSS document.
 *
 * Because sections are nested into one another, you can use
 * [method@CssSection.get_parent] to get the containing region.
 */
typedef struct _BobguiCssSection BobguiCssSection;

GDK_AVAILABLE_IN_ALL
GType              bobgui_css_section_get_type            (void) G_GNUC_CONST;

GDK_AVAILABLE_IN_ALL
BobguiCssSection *    bobgui_css_section_new                 (GFile                *file,
                                                        const BobguiCssLocation *start,
                                                        const BobguiCssLocation *end);
GDK_AVAILABLE_IN_4_16
BobguiCssSection *    bobgui_css_section_new_with_bytes      (GFile                *file,
                                                        GBytes               *bytes,
                                                        const BobguiCssLocation *start,
                                                        const BobguiCssLocation *end);
GDK_AVAILABLE_IN_ALL
BobguiCssSection *    bobgui_css_section_ref                 (BobguiCssSection        *section);
GDK_AVAILABLE_IN_ALL
void               bobgui_css_section_unref               (BobguiCssSection        *section);

GDK_AVAILABLE_IN_ALL
void               bobgui_css_section_print               (const BobguiCssSection  *section,
                                                        GString              *string);
GDK_AVAILABLE_IN_ALL
char *             bobgui_css_section_to_string           (const BobguiCssSection  *section);

GDK_AVAILABLE_IN_ALL
BobguiCssSection *    bobgui_css_section_get_parent          (const BobguiCssSection  *section);
GDK_AVAILABLE_IN_ALL
GFile *            bobgui_css_section_get_file            (const BobguiCssSection  *section);
GDK_AVAILABLE_IN_4_16
GBytes *           bobgui_css_section_get_bytes           (const BobguiCssSection  *section);
GDK_AVAILABLE_IN_ALL
const BobguiCssLocation *
                   bobgui_css_section_get_start_location  (const BobguiCssSection  *section);
GDK_AVAILABLE_IN_ALL
const BobguiCssLocation *
                   bobgui_css_section_get_end_location    (const BobguiCssSection  *section);

G_END_DECLS

