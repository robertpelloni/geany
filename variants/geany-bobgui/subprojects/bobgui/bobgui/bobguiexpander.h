/* BOBGUI - The Bobgui Framework
 *
 * Copyright (C) 2003 Sun Microsystems, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors:
 *      Mark McLoughlin <mark@skynet.ie>
 */

#pragma once

#if !defined (__BOBGUI_H_INSIDE__) && !defined (BOBGUI_COMPILATION)
#error "Only <bobgui/bobgui.h> can be included directly."
#endif

#include <bobgui/bobguiwidget.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_EXPANDER            (bobgui_expander_get_type ())
#define BOBGUI_EXPANDER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_EXPANDER, BobguiExpander))
#define BOBGUI_IS_EXPANDER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOBGUI_TYPE_EXPANDER))

typedef struct _BobguiExpander        BobguiExpander;

GDK_AVAILABLE_IN_ALL
GType                 bobgui_expander_get_type            (void) G_GNUC_CONST;

GDK_AVAILABLE_IN_ALL
BobguiWidget            *bobgui_expander_new                 (const char *label);
GDK_AVAILABLE_IN_ALL
BobguiWidget            *bobgui_expander_new_with_mnemonic   (const char *label);

GDK_AVAILABLE_IN_ALL
void                  bobgui_expander_set_expanded        (BobguiExpander *expander,
                                                        gboolean     expanded);
GDK_AVAILABLE_IN_ALL
gboolean              bobgui_expander_get_expanded        (BobguiExpander *expander);

GDK_AVAILABLE_IN_ALL
void                  bobgui_expander_set_label           (BobguiExpander *expander,
                                                        const char *label);
GDK_AVAILABLE_IN_ALL
const char *         bobgui_expander_get_label           (BobguiExpander *expander);

GDK_AVAILABLE_IN_ALL
void                  bobgui_expander_set_use_underline   (BobguiExpander *expander,
                                                        gboolean     use_underline);
GDK_AVAILABLE_IN_ALL
gboolean              bobgui_expander_get_use_underline   (BobguiExpander *expander);

GDK_AVAILABLE_IN_ALL
void                  bobgui_expander_set_use_markup      (BobguiExpander *expander,
                                                        gboolean    use_markup);
GDK_AVAILABLE_IN_ALL
gboolean              bobgui_expander_get_use_markup      (BobguiExpander *expander);

GDK_AVAILABLE_IN_ALL
void                  bobgui_expander_set_label_widget    (BobguiExpander *expander,
                                                        BobguiWidget   *label_widget);
GDK_AVAILABLE_IN_ALL
BobguiWidget            *bobgui_expander_get_label_widget    (BobguiExpander *expander);
GDK_AVAILABLE_IN_ALL
void                  bobgui_expander_set_resize_toplevel (BobguiExpander *expander,
                                                        gboolean     resize_toplevel);
GDK_AVAILABLE_IN_ALL
gboolean              bobgui_expander_get_resize_toplevel (BobguiExpander *expander);

GDK_AVAILABLE_IN_ALL
void                  bobgui_expander_set_child           (BobguiExpander *expander,
                                                        BobguiWidget      *child);
GDK_AVAILABLE_IN_ALL
BobguiWidget *           bobgui_expander_get_child           (BobguiExpander *expander);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(BobguiExpander, g_object_unref)

G_END_DECLS

