/* bobguientryprivate.h
 * Copyright (C) 2003  Kristian Rietveld  <kris@bobgui.org>
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

#include "bobguientry.h"

#include "deprecated/bobguientrycompletion.h"
#include "bobguieventcontrollermotion.h"
#include "deprecated/bobguiliststore.h"
#include "deprecated/bobguitreemodelfilter.h"
#include "deprecated/bobguitreeviewcolumn.h"
#include "bobguieventcontrollerkey.h"
#include "bobguitextprivate.h"

G_BEGIN_DECLS

typedef struct _BobguiEntryCompletionClass       BobguiEntryCompletionClass;

struct _BobguiEntryCompletion
{
  GObject parent_instance;

  BobguiWidget *entry;

  BobguiWidget *tree_view;
  BobguiTreeViewColumn *column;
  BobguiTreeModelFilter *filter_model;
  BobguiCellArea *cell_area;

  BobguiEntryCompletionMatchFunc match_func;
  gpointer match_data;
  GDestroyNotify match_notify;

  int minimum_key_length;
  int text_column;

  char *case_normalized_key;

  BobguiEventController *entry_key_controller;
  BobguiEventController *entry_focus_controller;

  /* only used by BobguiEntry when attached: */
  BobguiWidget *popup_window;
  BobguiWidget *scrolled_window;

  gulong completion_timeout;
  gulong changed_id;

  GSignalGroup *insert_text_signal_group;

  int current_selected;

  guint first_sel_changed : 1;
  guint has_completion    : 1;
  guint inline_completion : 1;
  guint popup_completion  : 1;
  guint popup_set_width   : 1;
  guint popup_single_match : 1;
  guint inline_selection   : 1;
  guint has_grab           : 1;

  char *completion_prefix;

  GSource *check_completion_idle;
};

struct _BobguiEntryCompletionClass
{
  GObjectClass parent_class;

  gboolean (* match_selected)   (BobguiEntryCompletion *completion,
                                 BobguiTreeModel       *model,
                                 BobguiTreeIter        *iter);
  void     (* action_activated) (BobguiEntryCompletion *completion,
                                 int                 index_);
  gboolean (* insert_prefix)    (BobguiEntryCompletion *completion,
                                 const char         *prefix);
  gboolean (* cursor_on_match)  (BobguiEntryCompletion *completion,
                                 BobguiTreeModel       *model,
                                 BobguiTreeIter        *iter);
  void     (* no_matches)       (BobguiEntryCompletion *completion);
};

void     _bobgui_entry_completion_resize_popup (BobguiEntryCompletion *completion);
void     _bobgui_entry_completion_popdown      (BobguiEntryCompletion *completion);
void     _bobgui_entry_completion_connect      (BobguiEntryCompletion *completion,
                                             BobguiEntry           *entry);
void     _bobgui_entry_completion_disconnect   (BobguiEntryCompletion *completion);

BobguiEventController * bobgui_entry_get_key_controller (BobguiEntry *entry);
BobguiText *bobgui_entry_get_text_widget (BobguiEntry *entry);

gboolean bobgui_entry_activate_icon (BobguiEntry             *entry,
                                  BobguiEntryIconPosition  pos);

G_END_DECLS

