/* Copyright (C) 2019 Red Hat, Inc.
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

#include <glib-object.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_TEXT_HISTORY (bobgui_text_history_get_type())

typedef struct _BobguiTextHistoryFuncs BobguiTextHistoryFuncs;

G_DECLARE_FINAL_TYPE (BobguiTextHistory, bobgui_text_history, BOBGUI, TEXT_HISTORY, GObject)

struct _BobguiTextHistoryFuncs
{
  void (*change_state) (gpointer     funcs_data,
                        gboolean     is_modified,
                        gboolean     can_undo,
                        gboolean     can_redo);
  void (*insert)       (gpointer     funcs_data,
                        guint        begin,
                        guint        end,
                        const char  *text,
                        guint        len);
  void (*delete)       (gpointer     funcs_data,
                        guint        begin,
                        guint        end,
                        const char  *expected_text,
                        guint        len);
  void (*select)       (gpointer     funcs_data,
                        int          selection_insert,
                        int          selection_bound);
};

BobguiTextHistory *bobgui_text_history_new                       (const BobguiTextHistoryFuncs *funcs,
                                                            gpointer                   funcs_data);
void            bobgui_text_history_begin_user_action         (BobguiTextHistory            *self);
void            bobgui_text_history_end_user_action           (BobguiTextHistory            *self);
void            bobgui_text_history_begin_irreversible_action (BobguiTextHistory            *self);
void            bobgui_text_history_end_irreversible_action   (BobguiTextHistory            *self);
gboolean        bobgui_text_history_get_can_undo              (BobguiTextHistory            *self);
gboolean        bobgui_text_history_get_can_redo              (BobguiTextHistory            *self);
void            bobgui_text_history_undo                      (BobguiTextHistory            *self);
void            bobgui_text_history_redo                      (BobguiTextHistory            *self);
guint           bobgui_text_history_get_max_undo_levels       (BobguiTextHistory            *self);
void            bobgui_text_history_set_max_undo_levels       (BobguiTextHistory            *self,
                                                            guint                      max_undo_levels);
void            bobgui_text_history_modified_changed          (BobguiTextHistory            *self,
                                                            gboolean                   modified);
void            bobgui_text_history_selection_changed         (BobguiTextHistory            *self,
                                                            int                        selection_insert,
                                                            int                        selection_bound);
void            bobgui_text_history_text_inserted             (BobguiTextHistory            *self,
                                                            guint                      position,
                                                            const char                *text,
                                                            int                        len);
void            bobgui_text_history_text_deleted              (BobguiTextHistory            *self,
                                                            guint                      begin,
                                                            guint                      end,
                                                            const char                *text,
                                                            int                        len);
gboolean        bobgui_text_history_get_enabled               (BobguiTextHistory            *self);
void            bobgui_text_history_set_enabled               (BobguiTextHistory            *self,
                                                            gboolean                   enabled);

G_END_DECLS

