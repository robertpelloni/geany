/* bobguiemojichooser.h: An Emoji chooser widget
 * Copyright 2017, Red Hat, Inc.
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

#include <bobgui/bobguiwidget.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_EMOJI_CHOOSER                 (bobgui_emoji_chooser_get_type ())
#define BOBGUI_EMOJI_CHOOSER(obj)                 (G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_EMOJI_CHOOSER, BobguiEmojiChooser))
#define BOBGUI_EMOJI_CHOOSER_CLASS(klass)         (G_TYPE_CHECK_CLASS_CAST ((klass), BOBGUI_TYPE_EMOJI_CHOOSER, BobguiEmojiChooserClass))
#define BOBGUI_IS_EMOJI_CHOOSER(obj)              (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOBGUI_TYPE_EMOJI_CHOOSER))
#define BOBGUI_IS_EMOJI_CHOOSER_CLASS(klass)      (G_TYPE_CHECK_CLASS_TYPE ((klass), BOBGUI_TYPE_EMOJI_CHOOSER))
#define BOBGUI_EMOJI_CHOOSER_GET_CLASS(obj)       (G_TYPE_INSTANCE_GET_CLASS ((obj), BOBGUI_TYPE_EMOJI_CHOOSER, BobguiEmojiChooserClass))

typedef struct _BobguiEmojiChooser      BobguiEmojiChooser;
typedef struct _BobguiEmojiChooserClass BobguiEmojiChooserClass;

GDK_AVAILABLE_IN_ALL
GType      bobgui_emoji_chooser_get_type (void) G_GNUC_CONST;
GDK_AVAILABLE_IN_ALL
BobguiWidget *bobgui_emoji_chooser_new      (void);

G_END_DECLS
