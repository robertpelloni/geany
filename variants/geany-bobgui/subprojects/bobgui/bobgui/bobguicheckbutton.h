/* BOBGUI - The Bobgui Framework
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
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

/*
 * Modified by the BOBGUI Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the BOBGUI Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * BOBGUI at ftp://ftp.bobgui.org/pub/bobgui/.
 */

#pragma once


#if !defined (__BOBGUI_H_INSIDE__) && !defined (BOBGUI_COMPILATION)
#error "Only <bobgui/bobgui.h> can be included directly."
#endif

#include <bobgui/bobguitogglebutton.h>


G_BEGIN_DECLS

#define BOBGUI_TYPE_CHECK_BUTTON                  (bobgui_check_button_get_type ())
#define BOBGUI_CHECK_BUTTON(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_CHECK_BUTTON, BobguiCheckButton))
#define BOBGUI_CHECK_BUTTON_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), BOBGUI_TYPE_CHECK_BUTTON, BobguiCheckButtonClass))
#define BOBGUI_IS_CHECK_BUTTON(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOBGUI_TYPE_CHECK_BUTTON))
#define BOBGUI_IS_CHECK_BUTTON_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), BOBGUI_TYPE_CHECK_BUTTON))
#define BOBGUI_CHECK_BUTTON_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), BOBGUI_TYPE_CHECK_BUTTON, BobguiCheckButtonClass))


typedef struct _BobguiCheckButton       BobguiCheckButton;
typedef struct _BobguiCheckButtonClass  BobguiCheckButtonClass;

struct _BobguiCheckButton
{
  BobguiWidget parent_instance;
};

struct _BobguiCheckButtonClass
{
  BobguiWidgetClass parent_class;

  void (* toggled) (BobguiCheckButton *check_button);
  void (* activate) (BobguiCheckButton *check_button);

  /*< private >*/
  gpointer padding[7];
};


GDK_AVAILABLE_IN_ALL
GType           bobgui_check_button_get_type           (void) G_GNUC_CONST;
GDK_AVAILABLE_IN_ALL
BobguiWidget *     bobgui_check_button_new                (void);
GDK_AVAILABLE_IN_ALL
BobguiWidget *     bobgui_check_button_new_with_label     (const char *label);
GDK_AVAILABLE_IN_ALL
BobguiWidget *     bobgui_check_button_new_with_mnemonic  (const char *label);
GDK_AVAILABLE_IN_ALL
void            bobgui_check_button_set_inconsistent   (BobguiCheckButton *check_button,
                                                     gboolean        inconsistent);
GDK_AVAILABLE_IN_ALL
gboolean        bobgui_check_button_get_inconsistent   (BobguiCheckButton *check_button);

GDK_AVAILABLE_IN_ALL
gboolean        bobgui_check_button_get_active         (BobguiCheckButton *self);
GDK_AVAILABLE_IN_ALL
void            bobgui_check_button_set_active         (BobguiCheckButton *self,
                                                     gboolean        setting);
GDK_AVAILABLE_IN_ALL
const char *    bobgui_check_button_get_label          (BobguiCheckButton *self);
GDK_AVAILABLE_IN_ALL
void            bobgui_check_button_set_label          (BobguiCheckButton *self,
                                                     const char     *label);
GDK_AVAILABLE_IN_ALL
void            bobgui_check_button_set_group          (BobguiCheckButton *self,
                                                     BobguiCheckButton *group);
GDK_AVAILABLE_IN_ALL
gboolean        bobgui_check_button_get_use_underline  (BobguiCheckButton *self);
GDK_AVAILABLE_IN_ALL
void            bobgui_check_button_set_use_underline  (BobguiCheckButton *self,
                                                     gboolean        setting);
GDK_AVAILABLE_IN_4_8
BobguiWidget *     bobgui_check_button_get_child          (BobguiCheckButton *button);
GDK_AVAILABLE_IN_4_8
void            bobgui_check_button_set_child          (BobguiCheckButton *button,
                                                     BobguiWidget *child);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(BobguiCheckButton, g_object_unref)

G_END_DECLS

