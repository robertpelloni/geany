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
 * Modified by the BOBGUI Team and others 1997-2001.  See the AUTHORS
 * file for a list of people on the BOBGUI Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * BOBGUI at ftp://ftp.bobgui.org/pub/bobgui/.
 */

#pragma once


#if !defined (__BOBGUI_H_INSIDE__) && !defined (BOBGUI_COMPILATION)
#error "Only <bobgui/bobgui.h> can be included directly."
#endif

#include <bobgui/bobguiwidget.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_BUTTON                 (bobgui_button_get_type ())
#define BOBGUI_BUTTON(obj)                 (G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_BUTTON, BobguiButton))
#define BOBGUI_BUTTON_CLASS(klass)         (G_TYPE_CHECK_CLASS_CAST ((klass), BOBGUI_TYPE_BUTTON, BobguiButtonClass))
#define BOBGUI_IS_BUTTON(obj)              (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOBGUI_TYPE_BUTTON))
#define BOBGUI_IS_BUTTON_CLASS(klass)      (G_TYPE_CHECK_CLASS_TYPE ((klass), BOBGUI_TYPE_BUTTON))
#define BOBGUI_BUTTON_GET_CLASS(obj)       (G_TYPE_INSTANCE_GET_CLASS ((obj), BOBGUI_TYPE_BUTTON, BobguiButtonClass))

typedef struct _BobguiButton             BobguiButton;
typedef struct _BobguiButtonPrivate      BobguiButtonPrivate;
typedef struct _BobguiButtonClass        BobguiButtonClass;

struct _BobguiButton
{
  /*< private >*/
  BobguiWidget parent_instance;
};

/**
 * BobguiButtonClass:
 * @parent_class: The parent class.
 * @clicked: Signal emitted when the button has been activated (pressed and released).
 * @activate: Signal that causes the button to animate press then
 *    release. Applications should never connect to this signal, but use
 *    the @clicked signal.
 */
struct _BobguiButtonClass
{
  BobguiWidgetClass        parent_class;

  /*< public >*/

  void (* clicked)  (BobguiButton *button);
  void (* activate) (BobguiButton *button);

  /*< private >*/

  gpointer padding[8];
};


GDK_AVAILABLE_IN_ALL
GType          bobgui_button_get_type          (void) G_GNUC_CONST;
GDK_AVAILABLE_IN_ALL
BobguiWidget*     bobgui_button_new               (void);
GDK_AVAILABLE_IN_ALL
BobguiWidget*     bobgui_button_new_with_label    (const char     *label);
GDK_AVAILABLE_IN_ALL
BobguiWidget*     bobgui_button_new_from_icon_name (const char     *icon_name);
GDK_AVAILABLE_IN_ALL
BobguiWidget*     bobgui_button_new_with_mnemonic (const char     *label);

GDK_AVAILABLE_IN_ALL
void                  bobgui_button_set_has_frame      (BobguiButton      *button,
						     gboolean        has_frame);
GDK_AVAILABLE_IN_ALL
gboolean              bobgui_button_get_has_frame      (BobguiButton      *button);
GDK_AVAILABLE_IN_ALL
void                  bobgui_button_set_label          (BobguiButton      *button,
						     const char     *label);
GDK_AVAILABLE_IN_ALL
const char *          bobgui_button_get_label          (BobguiButton      *button);
GDK_AVAILABLE_IN_ALL
void                  bobgui_button_set_use_underline  (BobguiButton      *button,
						     gboolean        use_underline);
GDK_AVAILABLE_IN_ALL
gboolean              bobgui_button_get_use_underline  (BobguiButton      *button);

GDK_AVAILABLE_IN_ALL
void                  bobgui_button_set_icon_name      (BobguiButton      *button,
                                                     const char     *icon_name);
GDK_AVAILABLE_IN_ALL
const char *          bobgui_button_get_icon_name      (BobguiButton      *button);

GDK_AVAILABLE_IN_ALL
void                  bobgui_button_set_child          (BobguiButton      *button,
                                                     BobguiWidget      *child);
GDK_AVAILABLE_IN_ALL
BobguiWidget *           bobgui_button_get_child          (BobguiButton      *button);

GDK_AVAILABLE_IN_4_12
void                  bobgui_button_set_can_shrink     (BobguiButton      *button,
                                                     gboolean        can_shrink);
GDK_AVAILABLE_IN_4_12
gboolean              bobgui_button_get_can_shrink     (BobguiButton      *button);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(BobguiButton, g_object_unref)

G_END_DECLS

