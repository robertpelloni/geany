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

#include <bobgui/bobguibutton.h>


G_BEGIN_DECLS

#define BOBGUI_TYPE_TOGGLE_BUTTON                  (bobgui_toggle_button_get_type ())
#define BOBGUI_TOGGLE_BUTTON(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_TOGGLE_BUTTON, BobguiToggleButton))
#define BOBGUI_TOGGLE_BUTTON_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), BOBGUI_TYPE_TOGGLE_BUTTON, BobguiToggleButtonClass))
#define BOBGUI_IS_TOGGLE_BUTTON(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOBGUI_TYPE_TOGGLE_BUTTON))
#define BOBGUI_IS_TOGGLE_BUTTON_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), BOBGUI_TYPE_TOGGLE_BUTTON))
#define BOBGUI_TOGGLE_BUTTON_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), BOBGUI_TYPE_TOGGLE_BUTTON, BobguiToggleButtonClass))

typedef struct _BobguiToggleButton              BobguiToggleButton;
typedef struct _BobguiToggleButtonClass         BobguiToggleButtonClass;

struct _BobguiToggleButton
{
  /*< private >*/
  BobguiButton button;
};

struct _BobguiToggleButtonClass
{
  BobguiButtonClass parent_class;

  void (* toggled) (BobguiToggleButton *toggle_button);

  /*< private >*/

  gpointer padding[8];
};


GDK_AVAILABLE_IN_ALL
GType      bobgui_toggle_button_get_type          (void) G_GNUC_CONST;

GDK_AVAILABLE_IN_ALL
BobguiWidget* bobgui_toggle_button_new               (void);
GDK_AVAILABLE_IN_ALL
BobguiWidget* bobgui_toggle_button_new_with_label    (const char      *label);
GDK_AVAILABLE_IN_ALL
BobguiWidget* bobgui_toggle_button_new_with_mnemonic (const char      *label);
GDK_AVAILABLE_IN_ALL
void       bobgui_toggle_button_set_active        (BobguiToggleButton *toggle_button,
                                                gboolean         is_active);
GDK_AVAILABLE_IN_ALL
gboolean   bobgui_toggle_button_get_active        (BobguiToggleButton *toggle_button);
GDK_DEPRECATED_IN_4_10
void       bobgui_toggle_button_toggled           (BobguiToggleButton *toggle_button);
GDK_AVAILABLE_IN_ALL
void       bobgui_toggle_button_set_group         (BobguiToggleButton *toggle_button,
                                                BobguiToggleButton *group);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(BobguiToggleButton, g_object_unref)

G_END_DECLS

