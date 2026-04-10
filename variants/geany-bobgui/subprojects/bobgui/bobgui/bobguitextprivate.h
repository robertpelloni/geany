/* bobguientryprivate.h
 * Copyright (C) 2019 Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "bobguitext.h"

#include "bobguieventcontroller.h"
#include  "bobguiimcontext.h"

G_BEGIN_DECLS

#define BOBGUI_TEXT_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), BOBGUI_TYPE_TEXT, BobguiTextClass))
#define BOBGUI_IS_TEXT_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), BOBGUI_TYPE_TEXT))
#define BOBGUI_TEXT_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), BOBGUI_TYPE_TEXT, BobguiTextClass))

typedef struct _BobguiTextClass         BobguiTextClass;

/*<private>
 * BobguiTextClass:
 * @parent_class: The parent class.
 * @activate: Class handler for the `BobguiText::activate` signal. The default
 *   implementation activates the bobgui.activate-default action.
 * @move_cursor: Class handler for the `BobguiText::move-cursor` signal. The
 *   default implementation specifies the standard `BobguiText` cursor movement
 *   behavior.
 * @insert_at_cursor: Class handler for the `BobguiText::insert-at-cursor` signal.
 *   The default implementation inserts text at the cursor.
 * @delete_from_cursor: Class handler for the `BobguiText::delete-from-cursor`
 *   signal. The default implementation deletes the selection or the specified
 *   number of characters or words.
 * @backspace: Class handler for the `BobguiText::backspace` signal. The default
 *   implementation deletes the selection or a single character or word.
 * @cut_clipboard: Class handler for the `BobguiText::cut-clipboard` signal. The
 *   default implementation cuts the selection, if one exists.
 * @copy_clipboard: Class handler for the `BobguiText::copy-clipboard` signal. The
 *   default implementation copies the selection, if one exists.
 * @paste_clipboard: Class handler for the `BobguiText::paste-clipboard` signal.
 *   The default implementation pastes at the current cursor position or over
 *   the current selection if one exists.
 * @toggle_overwrite: Class handler for the `BobguiText::toggle-overwrite` signal.
 *   The default implementation toggles overwrite mode and blinks the cursor.
 * @insert_emoji: Class handler for the `BobguiText::insert-emoji` signal.
 *
 * Class structure for `BobguiText`. All virtual functions have a default
 * implementation. Derived classes may set the virtual function pointers for the
 * signal handlers to %NULL, but must keep @get_text_area_size and
 * @get_frame_size non-%NULL; either use the default implementation, or provide
 * a custom one.
 */
struct _BobguiTextClass
{
  BobguiWidgetClass parent_class;

  /* Action signals
   */
  void (* activate)           (BobguiText         *self);
  void (* move_cursor)        (BobguiText         *self,
                               BobguiMovementStep  step,
                               int              count,
                               gboolean         extend);
  void (* insert_at_cursor)   (BobguiText         *self,
                               const char      *str);
  void (* delete_from_cursor) (BobguiText         *self,
                               BobguiDeleteType    type,
                               int              count);
  void (* backspace)          (BobguiText         *self);
  void (* cut_clipboard)      (BobguiText         *self);
  void (* copy_clipboard)     (BobguiText         *self);
  void (* paste_clipboard)    (BobguiText         *self);
  void (* toggle_overwrite)   (BobguiText         *self);
  void (* insert_emoji)       (BobguiText         *self);
  void (* undo)               (BobguiText         *self);
  void (* redo)               (BobguiText         *self);
};

char *              bobgui_text_get_display_text   (BobguiText    *entry,
                                                 int         start_pos,
                                                 int         end_pos);
void                bobgui_text_enter_text         (BobguiText    *entry,
                                                 const char *text);
void                bobgui_text_set_positions      (BobguiText    *entry,
                                                 int         current_pos,
                                                 int         selection_bound);
PangoLayout *       bobgui_text_get_layout         (BobguiText    *entry);
void                bobgui_text_get_layout_offsets (BobguiText    *entry,
                                                 int        *x,
                                                 int        *y);
void                bobgui_text_reset_im_context   (BobguiText    *entry);
BobguiEventController *bobgui_text_get_key_controller (BobguiText    *entry);

G_END_DECLS

