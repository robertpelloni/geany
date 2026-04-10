/* BOBGUI - The Bobgui Framework
 * bobguiprintcontext.h: Print Context
 * Copyright (C) 2006, Red Hat, Inc.
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


#if !defined (__BOBGUI_H_INSIDE__) && !defined (BOBGUI_COMPILATION)
#error "Only <bobgui/bobgui.h> can be included directly."
#endif

#include <pango/pango.h>
#include <bobgui/print/bobguipagesetup.h>


G_BEGIN_DECLS

typedef struct _BobguiPrintContext BobguiPrintContext;

#define BOBGUI_TYPE_PRINT_CONTEXT    (bobgui_print_context_get_type ())
#define BOBGUI_PRINT_CONTEXT(obj)    (G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_PRINT_CONTEXT, BobguiPrintContext))
#define BOBGUI_IS_PRINT_CONTEXT(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOBGUI_TYPE_PRINT_CONTEXT))

GDK_AVAILABLE_IN_ALL
GType          bobgui_print_context_get_type (void) G_GNUC_CONST;


/* Rendering */
GDK_AVAILABLE_IN_ALL
cairo_t      *bobgui_print_context_get_cairo_context    (BobguiPrintContext *context);

GDK_AVAILABLE_IN_ALL
BobguiPageSetup *bobgui_print_context_get_page_setup       (BobguiPrintContext *context);
GDK_AVAILABLE_IN_ALL
double        bobgui_print_context_get_width            (BobguiPrintContext *context);
GDK_AVAILABLE_IN_ALL
double        bobgui_print_context_get_height           (BobguiPrintContext *context);
GDK_AVAILABLE_IN_ALL
double        bobgui_print_context_get_dpi_x            (BobguiPrintContext *context);
GDK_AVAILABLE_IN_ALL
double        bobgui_print_context_get_dpi_y            (BobguiPrintContext *context);
GDK_AVAILABLE_IN_ALL
gboolean      bobgui_print_context_get_hard_margins     (BobguiPrintContext *context,
						      double          *top,
						      double          *bottom,
						      double          *left,
						      double          *right);

/* Fonts */
GDK_AVAILABLE_IN_ALL
PangoFontMap *bobgui_print_context_get_pango_fontmap    (BobguiPrintContext *context);
GDK_AVAILABLE_IN_ALL
PangoContext *bobgui_print_context_create_pango_context (BobguiPrintContext *context);
GDK_AVAILABLE_IN_ALL
PangoLayout  *bobgui_print_context_create_pango_layout  (BobguiPrintContext *context);

/* Needed for preview implementations */
GDK_AVAILABLE_IN_ALL
void         bobgui_print_context_set_cairo_context     (BobguiPrintContext *context,
						      cairo_t         *cr,
						      double           dpi_x,
						      double           dpi_y);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(BobguiPrintContext, g_object_unref)

G_END_DECLS

