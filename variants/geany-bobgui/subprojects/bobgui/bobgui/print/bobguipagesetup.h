/* BOBGUI - The Bobgui Framework
 * bobguipagesetup.h: Page Setup
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

#include <bobgui/print/bobguipapersize.h>


G_BEGIN_DECLS

typedef struct _BobguiPageSetup BobguiPageSetup;

#define BOBGUI_TYPE_PAGE_SETUP    (bobgui_page_setup_get_type ())
#define BOBGUI_PAGE_SETUP(obj)    (G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_PAGE_SETUP, BobguiPageSetup))
#define BOBGUI_IS_PAGE_SETUP(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOBGUI_TYPE_PAGE_SETUP))

GDK_AVAILABLE_IN_ALL
GType              bobgui_page_setup_get_type          (void) G_GNUC_CONST;
GDK_AVAILABLE_IN_ALL
BobguiPageSetup *     bobgui_page_setup_new               (void);
GDK_AVAILABLE_IN_ALL
BobguiPageSetup *     bobgui_page_setup_copy              (BobguiPageSetup       *other);
GDK_AVAILABLE_IN_ALL
BobguiPageOrientation bobgui_page_setup_get_orientation   (BobguiPageSetup       *setup);
GDK_AVAILABLE_IN_ALL
void               bobgui_page_setup_set_orientation   (BobguiPageSetup       *setup,
						     BobguiPageOrientation  orientation);
GDK_AVAILABLE_IN_ALL
BobguiPaperSize *     bobgui_page_setup_get_paper_size    (BobguiPageSetup       *setup);
GDK_AVAILABLE_IN_ALL
void               bobgui_page_setup_set_paper_size    (BobguiPageSetup       *setup,
						     BobguiPaperSize       *size);
GDK_AVAILABLE_IN_ALL
double             bobgui_page_setup_get_top_margin    (BobguiPageSetup       *setup,
						     BobguiUnit             unit);
GDK_AVAILABLE_IN_ALL
void               bobgui_page_setup_set_top_margin    (BobguiPageSetup       *setup,
						     double              margin,
						     BobguiUnit             unit);
GDK_AVAILABLE_IN_ALL
double             bobgui_page_setup_get_bottom_margin (BobguiPageSetup       *setup,
						     BobguiUnit             unit);
GDK_AVAILABLE_IN_ALL
void               bobgui_page_setup_set_bottom_margin (BobguiPageSetup       *setup,
						     double              margin,
						     BobguiUnit             unit);
GDK_AVAILABLE_IN_ALL
double             bobgui_page_setup_get_left_margin   (BobguiPageSetup       *setup,
						     BobguiUnit             unit);
GDK_AVAILABLE_IN_ALL
void               bobgui_page_setup_set_left_margin   (BobguiPageSetup       *setup,
						     double              margin,
						     BobguiUnit             unit);
GDK_AVAILABLE_IN_ALL
double             bobgui_page_setup_get_right_margin  (BobguiPageSetup       *setup,
						     BobguiUnit             unit);
GDK_AVAILABLE_IN_ALL
void               bobgui_page_setup_set_right_margin  (BobguiPageSetup       *setup,
						     double              margin,
						     BobguiUnit             unit);

GDK_AVAILABLE_IN_ALL
void bobgui_page_setup_set_paper_size_and_default_margins (BobguiPageSetup    *setup,
							BobguiPaperSize    *size);

/* These take orientation, but not margins into consideration */
GDK_AVAILABLE_IN_ALL
double             bobgui_page_setup_get_paper_width   (BobguiPageSetup       *setup,
						     BobguiUnit             unit);
GDK_AVAILABLE_IN_ALL
double             bobgui_page_setup_get_paper_height  (BobguiPageSetup       *setup,
						     BobguiUnit             unit);


/* These take orientation, and margins into consideration */
GDK_AVAILABLE_IN_ALL
double             bobgui_page_setup_get_page_width    (BobguiPageSetup       *setup,
						     BobguiUnit             unit);
GDK_AVAILABLE_IN_ALL
double             bobgui_page_setup_get_page_height   (BobguiPageSetup       *setup,
						     BobguiUnit             unit);

/* Saving and restoring page setup */
GDK_AVAILABLE_IN_ALL
BobguiPageSetup	  *bobgui_page_setup_new_from_file	    (const char          *file_name,
						     GError              **error);
GDK_AVAILABLE_IN_ALL
gboolean	   bobgui_page_setup_load_file	    (BobguiPageSetup        *setup,
						     const char          *file_name,
						     GError             **error);
GDK_AVAILABLE_IN_ALL
gboolean	   bobgui_page_setup_to_file	    (BobguiPageSetup        *setup,
						     const char          *file_name,
						     GError             **error);
GDK_AVAILABLE_IN_ALL
BobguiPageSetup	  *bobgui_page_setup_new_from_key_file (GKeyFile            *key_file,
						     const char          *group_name,
						     GError             **error);
GDK_AVAILABLE_IN_ALL
gboolean           bobgui_page_setup_load_key_file     (BobguiPageSetup        *setup,
				                     GKeyFile            *key_file,
				                     const char          *group_name,
				                     GError             **error);
GDK_AVAILABLE_IN_ALL
void		   bobgui_page_setup_to_key_file	    (BobguiPageSetup        *setup,
						     GKeyFile            *key_file,
						     const char          *group_name);

GDK_AVAILABLE_IN_ALL
GVariant          *bobgui_page_setup_to_gvariant       (BobguiPageSetup        *setup);
GDK_AVAILABLE_IN_ALL
BobguiPageSetup      *bobgui_page_setup_new_from_gvariant (GVariant            *variant);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(BobguiPageSetup, g_object_unref)

G_END_DECLS

