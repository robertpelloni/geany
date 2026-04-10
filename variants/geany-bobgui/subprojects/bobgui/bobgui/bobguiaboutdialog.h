/* BOBGUI - A modern native application and UI framework

   Copyright (C) 2001 CodeFactory AB
   Copyright (C) 2001 Anders Carlsson <andersca@codefactory.se>
   Copyright (C) 2003, 2004 Matthias Clasen <mclasen@redhat.com>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with this library. If not, see <http://www.gnu.org/licenses/>.

   Author: Anders Carlsson <andersca@codefactory.se>
*/

#pragma once

#if !defined (__BOBGUI_H_INSIDE__) && !defined (BOBGUI_COMPILATION)
#error "Only <bobgui/bobgui.h> can be included directly."
#endif

#include <bobgui/bobguiwindow.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_ABOUT_DIALOG            (bobgui_about_dialog_get_type ())
#define BOBGUI_ABOUT_DIALOG(object)         (G_TYPE_CHECK_INSTANCE_CAST ((object), BOBGUI_TYPE_ABOUT_DIALOG, BobguiAboutDialog))
#define BOBGUI_IS_ABOUT_DIALOG(object)      (G_TYPE_CHECK_INSTANCE_TYPE ((object), BOBGUI_TYPE_ABOUT_DIALOG))

typedef struct _BobguiAboutDialog        BobguiAboutDialog;

/**
 * BobguiLicense:
 * @BOBGUI_LICENSE_UNKNOWN: No license specified
 * @BOBGUI_LICENSE_CUSTOM: A license text is going to be specified by the
 *   developer
 * @BOBGUI_LICENSE_GPL_2_0: The GNU General Public License, version 2.0 or later
 * @BOBGUI_LICENSE_GPL_3_0: The GNU General Public License, version 3.0 or later
 * @BOBGUI_LICENSE_LGPL_2_1: The GNU Lesser General Public License, version 2.1 or later
 * @BOBGUI_LICENSE_LGPL_3_0: The GNU Lesser General Public License, version 3.0 or later
 * @BOBGUI_LICENSE_BSD: The BSD standard license
 * @BOBGUI_LICENSE_MIT_X11: The MIT/X11 standard license
 * @BOBGUI_LICENSE_ARTISTIC: The Artistic License, version 2.0
 * @BOBGUI_LICENSE_GPL_2_0_ONLY: The GNU General Public License, version 2.0 only
 * @BOBGUI_LICENSE_GPL_3_0_ONLY: The GNU General Public License, version 3.0 only
 * @BOBGUI_LICENSE_LGPL_2_1_ONLY: The GNU Lesser General Public License, version 2.1 only
 * @BOBGUI_LICENSE_LGPL_3_0_ONLY: The GNU Lesser General Public License, version 3.0 only
 * @BOBGUI_LICENSE_AGPL_3_0: The GNU Affero General Public License, version 3.0 or later
 * @BOBGUI_LICENSE_AGPL_3_0_ONLY: The GNU Affero General Public License, version 3.0 only
 * @BOBGUI_LICENSE_BSD_3: The 3-clause BSD licence
 * @BOBGUI_LICENSE_APACHE_2_0: The Apache License, version 2.0
 * @BOBGUI_LICENSE_MPL_2_0: The Mozilla Public License, version 2.0
 * @BOBGUI_LICENSE_0BSD: Zero-Clause BSD license
 *
 * The type of license for an application.
 *
 * This enumeration can be expanded at later date.
 */
typedef enum {
  BOBGUI_LICENSE_UNKNOWN,
  BOBGUI_LICENSE_CUSTOM,

  BOBGUI_LICENSE_GPL_2_0,
  BOBGUI_LICENSE_GPL_3_0,

  BOBGUI_LICENSE_LGPL_2_1,
  BOBGUI_LICENSE_LGPL_3_0,

  BOBGUI_LICENSE_BSD,
  BOBGUI_LICENSE_MIT_X11,

  BOBGUI_LICENSE_ARTISTIC,

  BOBGUI_LICENSE_GPL_2_0_ONLY,
  BOBGUI_LICENSE_GPL_3_0_ONLY,
  BOBGUI_LICENSE_LGPL_2_1_ONLY,
  BOBGUI_LICENSE_LGPL_3_0_ONLY,

  BOBGUI_LICENSE_AGPL_3_0,
  BOBGUI_LICENSE_AGPL_3_0_ONLY,

  BOBGUI_LICENSE_BSD_3,
  BOBGUI_LICENSE_APACHE_2_0,
  BOBGUI_LICENSE_MPL_2_0,
  BOBGUI_LICENSE_0BSD
} BobguiLicense;


GDK_AVAILABLE_IN_ALL
GType                  bobgui_about_dialog_get_type               (void) G_GNUC_CONST;
GDK_AVAILABLE_IN_ALL
BobguiWidget             *bobgui_about_dialog_new                    (void);
GDK_AVAILABLE_IN_ALL
void                   bobgui_show_about_dialog                   (BobguiWindow       *parent,
                                                                const char      *first_property_name,
                                                                ...) G_GNUC_NULL_TERMINATED;
GDK_AVAILABLE_IN_ALL
const char *          bobgui_about_dialog_get_program_name       (BobguiAboutDialog  *about);
GDK_AVAILABLE_IN_ALL
void                   bobgui_about_dialog_set_program_name       (BobguiAboutDialog  *about,
                                                                const char      *name);
GDK_AVAILABLE_IN_ALL
const char *          bobgui_about_dialog_get_version            (BobguiAboutDialog  *about);
GDK_AVAILABLE_IN_ALL
void                   bobgui_about_dialog_set_version            (BobguiAboutDialog  *about,
                                                                const char      *version);
GDK_AVAILABLE_IN_ALL
const char *          bobgui_about_dialog_get_copyright          (BobguiAboutDialog  *about);
GDK_AVAILABLE_IN_ALL
void                   bobgui_about_dialog_set_copyright          (BobguiAboutDialog  *about,
                                                                const char      *copyright);
GDK_AVAILABLE_IN_ALL
const char *          bobgui_about_dialog_get_comments           (BobguiAboutDialog  *about);
GDK_AVAILABLE_IN_ALL
void                   bobgui_about_dialog_set_comments           (BobguiAboutDialog  *about,
                                                                const char      *comments);
GDK_AVAILABLE_IN_ALL
const char *          bobgui_about_dialog_get_license            (BobguiAboutDialog  *about);
GDK_AVAILABLE_IN_ALL
void                   bobgui_about_dialog_set_license            (BobguiAboutDialog  *about,
                                                                const char      *license);
GDK_AVAILABLE_IN_ALL
void                   bobgui_about_dialog_set_license_type       (BobguiAboutDialog  *about,
                                                                BobguiLicense       license_type);
GDK_AVAILABLE_IN_ALL
BobguiLicense             bobgui_about_dialog_get_license_type       (BobguiAboutDialog  *about);

GDK_AVAILABLE_IN_ALL
gboolean               bobgui_about_dialog_get_wrap_license       (BobguiAboutDialog  *about);
GDK_AVAILABLE_IN_ALL
void                   bobgui_about_dialog_set_wrap_license       (BobguiAboutDialog  *about,
                                                                gboolean         wrap_license);

GDK_AVAILABLE_IN_ALL
const char *          bobgui_about_dialog_get_system_information (BobguiAboutDialog  *about);
GDK_AVAILABLE_IN_ALL
void                   bobgui_about_dialog_set_system_information (BobguiAboutDialog  *about,
                                                                const char      *system_information);
GDK_AVAILABLE_IN_ALL
const char *          bobgui_about_dialog_get_website            (BobguiAboutDialog  *about);
GDK_AVAILABLE_IN_ALL
void                   bobgui_about_dialog_set_website            (BobguiAboutDialog  *about,
                                                                const char      *website);
GDK_AVAILABLE_IN_ALL
const char *          bobgui_about_dialog_get_website_label      (BobguiAboutDialog  *about);
GDK_AVAILABLE_IN_ALL
void                   bobgui_about_dialog_set_website_label      (BobguiAboutDialog  *about,
                                                                const char      *website_label);
GDK_AVAILABLE_IN_ALL
const char * const *   bobgui_about_dialog_get_authors            (BobguiAboutDialog  *about);
GDK_AVAILABLE_IN_ALL
void                   bobgui_about_dialog_set_authors            (BobguiAboutDialog  *about,
                                                                const char     **authors);
GDK_AVAILABLE_IN_ALL
const char * const *   bobgui_about_dialog_get_documenters        (BobguiAboutDialog  *about);
GDK_AVAILABLE_IN_ALL
void                   bobgui_about_dialog_set_documenters        (BobguiAboutDialog  *about,
                                                                const char     **documenters);
GDK_AVAILABLE_IN_ALL
const char * const *   bobgui_about_dialog_get_artists            (BobguiAboutDialog  *about);
GDK_AVAILABLE_IN_ALL
void                   bobgui_about_dialog_set_artists            (BobguiAboutDialog  *about,
                                                                const char     **artists);
GDK_AVAILABLE_IN_ALL
const char *          bobgui_about_dialog_get_translator_credits (BobguiAboutDialog  *about);
GDK_AVAILABLE_IN_ALL
void                   bobgui_about_dialog_set_translator_credits (BobguiAboutDialog  *about,
                                                                const char      *translator_credits);
GDK_AVAILABLE_IN_ALL
GdkPaintable          *bobgui_about_dialog_get_logo               (BobguiAboutDialog  *about);
GDK_AVAILABLE_IN_ALL
void                   bobgui_about_dialog_set_logo               (BobguiAboutDialog  *about,
                                                                GdkPaintable    *logo);
GDK_AVAILABLE_IN_ALL
const char *          bobgui_about_dialog_get_logo_icon_name     (BobguiAboutDialog  *about);
GDK_AVAILABLE_IN_ALL
void                   bobgui_about_dialog_set_logo_icon_name     (BobguiAboutDialog  *about,
                                                                const char      *icon_name);
GDK_AVAILABLE_IN_ALL
void                  bobgui_about_dialog_add_credit_section      (BobguiAboutDialog  *about,
                                                                const char      *section_name,
                                                                const char     **people);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(BobguiAboutDialog, g_object_unref)

G_END_DECLS



