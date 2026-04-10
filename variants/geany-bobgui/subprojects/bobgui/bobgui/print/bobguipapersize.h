/* BOBGUI - The Bobgui Framework
 * bobguipapersize.h: Paper Size
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

#include <gdk/gdk.h>
#include <bobgui/bobguienums.h>


G_BEGIN_DECLS

typedef struct _BobguiPaperSize BobguiPaperSize;

#define BOBGUI_TYPE_PAPER_SIZE    (bobgui_paper_size_get_type ())

/* Common names, from PWG 5101.1-2002 PWG: Standard for Media Standardized Names */
/**
 * BOBGUI_PAPER_NAME_A3:
 *
 * Name for the A3 paper size.
 */
#define BOBGUI_PAPER_NAME_A3 "iso_a3"

/**
 * BOBGUI_PAPER_NAME_A4:
 *
 * Name for the A4 paper size.
 */
#define BOBGUI_PAPER_NAME_A4 "iso_a4"

/**
 * BOBGUI_PAPER_NAME_A5:
 *
 * Name for the A5 paper size.
 */
#define BOBGUI_PAPER_NAME_A5 "iso_a5"

/**
 * BOBGUI_PAPER_NAME_B5:
 *
 * Name for the B5 paper size.
 */
#define BOBGUI_PAPER_NAME_B5 "iso_b5"

/**
 * BOBGUI_PAPER_NAME_LETTER:
 *
 * Name for the Letter paper size.
 */
#define BOBGUI_PAPER_NAME_LETTER "na_letter"

/**
 * BOBGUI_PAPER_NAME_EXECUTIVE:
 *
 * Name for the Executive paper size.
 */
#define BOBGUI_PAPER_NAME_EXECUTIVE "na_executive"

/**
 * BOBGUI_PAPER_NAME_LEGAL:
 *
 * Name for the Legal paper size.
 */
#define BOBGUI_PAPER_NAME_LEGAL "na_legal"

GDK_AVAILABLE_IN_ALL
GType bobgui_paper_size_get_type (void) G_GNUC_CONST;

GDK_AVAILABLE_IN_ALL
BobguiPaperSize *bobgui_paper_size_new          (const char   *name);
GDK_AVAILABLE_IN_ALL
BobguiPaperSize *bobgui_paper_size_new_from_ppd (const char   *ppd_name,
					   const char   *ppd_display_name,
					   double        width,
					   double        height);
GDK_AVAILABLE_IN_ALL
BobguiPaperSize *bobgui_paper_size_new_from_ipp (const char   *ipp_name,
					   double        width,
					   double        height);
GDK_AVAILABLE_IN_ALL
BobguiPaperSize *bobgui_paper_size_new_custom   (const char   *name,
					   const char   *display_name,
					   double        width,
					   double        height,
					   BobguiUnit       unit);
GDK_AVAILABLE_IN_ALL
BobguiPaperSize *bobgui_paper_size_copy         (BobguiPaperSize *other);
GDK_AVAILABLE_IN_ALL
void          bobgui_paper_size_free         (BobguiPaperSize *size);
GDK_AVAILABLE_IN_ALL
gboolean      bobgui_paper_size_is_equal     (BobguiPaperSize *size1,
					   BobguiPaperSize *size2);

GDK_AVAILABLE_IN_ALL
GList        *bobgui_paper_size_get_paper_sizes (gboolean include_custom);

/* The width is always the shortest side, measure in mm */
GDK_AVAILABLE_IN_ALL
const char *bobgui_paper_size_get_name         (BobguiPaperSize *size);
GDK_AVAILABLE_IN_ALL
const char *bobgui_paper_size_get_display_name (BobguiPaperSize *size);
GDK_AVAILABLE_IN_ALL
const char *bobgui_paper_size_get_ppd_name     (BobguiPaperSize *size);

GDK_AVAILABLE_IN_ALL
double   bobgui_paper_size_get_width        (BobguiPaperSize *size, BobguiUnit unit);
GDK_AVAILABLE_IN_ALL
double   bobgui_paper_size_get_height       (BobguiPaperSize *size, BobguiUnit unit);
GDK_AVAILABLE_IN_ALL
gboolean bobgui_paper_size_is_custom        (BobguiPaperSize *size);
GDK_AVAILABLE_IN_ALL
gboolean bobgui_paper_size_is_ipp           (BobguiPaperSize *size);

/* Only for custom sizes: */
GDK_AVAILABLE_IN_ALL
void    bobgui_paper_size_set_size                  (BobguiPaperSize *size, 
                                                  double        width, 
                                                  double        height, 
                                                  BobguiUnit       unit);

GDK_AVAILABLE_IN_ALL
double bobgui_paper_size_get_default_top_margin     (BobguiPaperSize *size,
						  BobguiUnit       unit);
GDK_AVAILABLE_IN_ALL
double bobgui_paper_size_get_default_bottom_margin  (BobguiPaperSize *size,
						  BobguiUnit       unit);
GDK_AVAILABLE_IN_ALL
double bobgui_paper_size_get_default_left_margin    (BobguiPaperSize *size,
						  BobguiUnit       unit);
GDK_AVAILABLE_IN_ALL
double bobgui_paper_size_get_default_right_margin   (BobguiPaperSize *size,
						  BobguiUnit       unit);

GDK_AVAILABLE_IN_ALL
const char *bobgui_paper_size_get_default (void);

GDK_AVAILABLE_IN_ALL
BobguiPaperSize *bobgui_paper_size_new_from_key_file (GKeyFile    *key_file,
					        const char *group_name,
					        GError     **error);
GDK_AVAILABLE_IN_ALL
void     bobgui_paper_size_to_key_file            (BobguiPaperSize *size,
					        GKeyFile     *key_file,
					        const char   *group_name);

GDK_AVAILABLE_IN_ALL
BobguiPaperSize *bobgui_paper_size_new_from_gvariant (GVariant     *variant);
GDK_AVAILABLE_IN_ALL
GVariant     *bobgui_paper_size_to_gvariant       (BobguiPaperSize *paper_size);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(BobguiPaperSize, bobgui_paper_size_free)

G_END_DECLS

