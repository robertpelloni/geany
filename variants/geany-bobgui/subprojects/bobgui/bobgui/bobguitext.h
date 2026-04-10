/* BOBGUI - The Bobgui Framework
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * Copyright (C) 2004-2006 Christian Hammond
 * Copyright (C) 2008 Cody Russell
 * Copyright (C) 2008 Red Hat, Inc.
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

#include <bobgui/bobguieditable.h>
#include <bobgui/bobguientrybuffer.h>


G_BEGIN_DECLS

#define BOBGUI_TYPE_TEXT                  (bobgui_text_get_type ())
#define BOBGUI_TEXT(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_TEXT, BobguiText))
#define BOBGUI_IS_TEXT(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOBGUI_TYPE_TEXT))

typedef struct _BobguiText              BobguiText;

struct _BobguiText
{
  /*< private >*/
  BobguiWidget  parent_instance;
};

GDK_AVAILABLE_IN_ALL
GType           bobgui_text_get_type                       (void) G_GNUC_CONST;

GDK_AVAILABLE_IN_ALL
BobguiWidget *     bobgui_text_new                            (void);
GDK_AVAILABLE_IN_ALL
BobguiWidget *     bobgui_text_new_with_buffer                (BobguiEntryBuffer  *buffer);

GDK_AVAILABLE_IN_ALL
BobguiEntryBuffer *bobgui_text_get_buffer                     (BobguiText         *self);
GDK_AVAILABLE_IN_ALL
void            bobgui_text_set_buffer                     (BobguiText         *self,
                                                         BobguiEntryBuffer  *buffer);

GDK_AVAILABLE_IN_ALL
void            bobgui_text_set_visibility                 (BobguiText         *self,
                                                         gboolean         visible);
GDK_AVAILABLE_IN_ALL
gboolean        bobgui_text_get_visibility                 (BobguiText         *self);

GDK_AVAILABLE_IN_ALL
void            bobgui_text_set_invisible_char             (BobguiText         *self,
                                                         gunichar         ch);
GDK_AVAILABLE_IN_ALL
gunichar        bobgui_text_get_invisible_char             (BobguiText         *self);
GDK_AVAILABLE_IN_ALL
void            bobgui_text_unset_invisible_char           (BobguiText         *self);

GDK_AVAILABLE_IN_ALL
void            bobgui_text_set_overwrite_mode             (BobguiText         *self,
                                                         gboolean         overwrite);
GDK_AVAILABLE_IN_ALL
gboolean        bobgui_text_get_overwrite_mode             (BobguiText         *self);

GDK_AVAILABLE_IN_ALL
void            bobgui_text_set_max_length                 (BobguiText         *self,
                                                         int              length);
GDK_AVAILABLE_IN_ALL
int             bobgui_text_get_max_length                 (BobguiText         *self);
GDK_AVAILABLE_IN_ALL
guint16         bobgui_text_get_text_length                (BobguiText         *self);

GDK_AVAILABLE_IN_ALL
void            bobgui_text_set_activates_default          (BobguiText         *self,
                                                         gboolean         activates);
GDK_AVAILABLE_IN_ALL
gboolean        bobgui_text_get_activates_default          (BobguiText         *self);

GDK_AVAILABLE_IN_ALL
const char *    bobgui_text_get_placeholder_text           (BobguiText         *self);
GDK_AVAILABLE_IN_ALL
void            bobgui_text_set_placeholder_text           (BobguiText         *self,
                                                         const char      *text);

GDK_AVAILABLE_IN_ALL
void            bobgui_text_set_input_purpose              (BobguiText         *self,
                                                         BobguiInputPurpose  purpose);
GDK_AVAILABLE_IN_ALL
BobguiInputPurpose bobgui_text_get_input_purpose              (BobguiText         *self);

GDK_AVAILABLE_IN_ALL
void            bobgui_text_set_input_hints                (BobguiText         *self,
                                                         BobguiInputHints    hints);
GDK_AVAILABLE_IN_ALL
BobguiInputHints   bobgui_text_get_input_hints                (BobguiText         *self);

GDK_AVAILABLE_IN_ALL
void            bobgui_text_set_attributes                 (BobguiText         *self,
                                                         PangoAttrList   *attrs);
GDK_AVAILABLE_IN_ALL
PangoAttrList * bobgui_text_get_attributes                 (BobguiText         *self);

GDK_AVAILABLE_IN_ALL
void            bobgui_text_set_tabs                       (BobguiText         *self,
                                                         PangoTabArray   *tabs);

GDK_AVAILABLE_IN_ALL
PangoTabArray * bobgui_text_get_tabs                       (BobguiText         *self);

GDK_AVAILABLE_IN_ALL
gboolean        bobgui_text_grab_focus_without_selecting   (BobguiText         *self);

GDK_AVAILABLE_IN_ALL
void            bobgui_text_set_extra_menu                 (BobguiText         *self,
                                                         GMenuModel      *model);
GDK_AVAILABLE_IN_ALL
GMenuModel *    bobgui_text_get_extra_menu                 (BobguiText         *self);

GDK_AVAILABLE_IN_ALL
void            bobgui_text_set_enable_emoji_completion    (BobguiText         *self,
                                                         gboolean         enable_emoji_completion);
GDK_AVAILABLE_IN_ALL
gboolean        bobgui_text_get_enable_emoji_completion    (BobguiText         *self);


GDK_AVAILABLE_IN_ALL
void            bobgui_text_set_propagate_text_width       (BobguiText         *self,
                                                         gboolean         propagate_text_width);
GDK_AVAILABLE_IN_ALL
gboolean        bobgui_text_get_propagate_text_width       (BobguiText         *self);

GDK_AVAILABLE_IN_ALL
void            bobgui_text_set_truncate_multiline         (BobguiText         *self,
                                                         gboolean         truncate_multiline);
GDK_AVAILABLE_IN_ALL
gboolean        bobgui_text_get_truncate_multiline         (BobguiText         *self);

GDK_AVAILABLE_IN_4_4
void            bobgui_text_compute_cursor_extents         (BobguiText         *self,
                                                         gsize            position,
                                                         graphene_rect_t *strong,
                                                         graphene_rect_t *weak);


G_END_DECLS

