/*
 * bobguiimmoduleime
 * Copyright (C) 2003 Takuro Ashie
 * Copyright (C) 2003 Kazuki IWAMOTO
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
 *
 * $Id$
 */

#include <bobgui/bobgui.h>

#define BOBGUI_TYPE_IM_CONTEXT_IME            (bobgui_im_context_ime_get_type ())
#define BOBGUI_IM_CONTEXT_IME(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_IM_CONTEXT_IME, BobguiIMContextIME))
#define BOBGUI_IM_CONTEXT_IME_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BOBGUI_TYPE_IM_CONTEXT_IME, BobguiIMContextIMEClass))
#define BOBGUI_IS_IM_CONTEXT_IME(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOBGUI_TYPE_IM_CONTEXT_IME))
#define BOBGUI_IS_IM_CONTEXT_IME_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BOBGUI_TYPE_IM_CONTEXT_IME))
#define BOBGUI_IM_CONTEXT_IME_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BOBGUI_TYPE_IM_CONTEXT_IME, BobguiIMContextIMEClass))

typedef struct _BobguiIMContextIME BobguiIMContextIME;
typedef struct _BobguiIMContextIMEPrivate BobguiIMContextIMEPrivate;
typedef struct _BobguiIMContextIMEClass BobguiIMContextIMEClass;

struct _BobguiIMContextIME
{
  BobguiIMContext object;

  BobguiWidget *client_widget;
  GdkSurface *client_surface;
  guint use_preedit : 1;
  guint preediting : 1;
  guint opened : 1;
  guint focus : 1;
  GdkRectangle cursor_location;
  char *commit_string;

  BobguiIMContextIMEPrivate *priv;
};

struct _BobguiIMContextIMEClass
{
  BobguiIMContextClass parent_class;
};

GDK_AVAILABLE_IN_ALL
GType         bobgui_im_context_ime_get_type      (void) G_GNUC_CONST;

void          bobgui_im_context_ime_register_type (GTypeModule * type_module);
BobguiIMContext *bobgui_im_context_ime_new           (void);
