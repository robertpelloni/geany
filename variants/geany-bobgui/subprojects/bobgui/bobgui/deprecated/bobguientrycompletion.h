/* bobguientrycompletion.h
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

#if !defined (__BOBGUI_H_INSIDE__) && !defined (BOBGUI_COMPILATION)
#error "Only <bobgui/bobgui.h> can be included directly."
#endif

#include <gdk/gdk.h>
#include <bobgui/deprecated/bobguitreemodel.h>
#include <bobgui/deprecated/bobguiliststore.h>
#include <bobgui/deprecated/bobguicellarea.h>
#include <bobgui/deprecated/bobguitreeviewcolumn.h>
#include <bobgui/deprecated/bobguitreemodelfilter.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_ENTRY_COMPLETION            (bobgui_entry_completion_get_type ())
#define BOBGUI_ENTRY_COMPLETION(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_ENTRY_COMPLETION, BobguiEntryCompletion))
#define BOBGUI_IS_ENTRY_COMPLETION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOBGUI_TYPE_ENTRY_COMPLETION))

typedef struct _BobguiEntryCompletion            BobguiEntryCompletion;

/**
 * BobguiEntryCompletionMatchFunc:
 * @completion: the `BobguiEntryCompletion`
 * @key: the string to match, normalized and case-folded
 * @iter: a `BobguiTreeIter` indicating the row to match
 * @user_data: user data given to bobgui_entry_completion_set_match_func()
 *
 * A function which decides whether the row indicated by @iter matches
 * a given @key, and should be displayed as a possible completion for @key.
 *
 * Note that @key is normalized and case-folded (see g_utf8_normalize()
 * and g_utf8_casefold()). If this is not appropriate, match functions
 * have access to the unmodified key via
 * `bobgui_editable_get_text (BOBGUI_EDITABLE (bobgui_entry_completion_get_entry ()))`.
 *
 * Returns: %TRUE if @iter should be displayed as a possible completion
 *   for @key
 *
 * Deprecated: 4.20: There is no replacement
 */
typedef gboolean (* BobguiEntryCompletionMatchFunc) (BobguiEntryCompletion *completion,
                                                  const char         *key,
                                                  BobguiTreeIter        *iter,
                                                  gpointer            user_data);


GDK_AVAILABLE_IN_ALL
GType               bobgui_entry_completion_get_type               (void) G_GNUC_CONST;
GDK_DEPRECATED_IN_4_10
BobguiEntryCompletion *bobgui_entry_completion_new                    (void);
GDK_DEPRECATED_IN_4_10
BobguiEntryCompletion *bobgui_entry_completion_new_with_area          (BobguiCellArea                 *area);

GDK_DEPRECATED_IN_4_10
BobguiWidget          *bobgui_entry_completion_get_entry              (BobguiEntryCompletion          *completion);

GDK_DEPRECATED_IN_4_10
void                bobgui_entry_completion_set_model              (BobguiEntryCompletion          *completion,
                                                                 BobguiTreeModel                *model);
GDK_DEPRECATED_IN_4_10
BobguiTreeModel       *bobgui_entry_completion_get_model              (BobguiEntryCompletion          *completion);

GDK_DEPRECATED_IN_4_10
void                bobgui_entry_completion_set_match_func         (BobguiEntryCompletion          *completion,
                                                                 BobguiEntryCompletionMatchFunc  func,
                                                                 gpointer                     func_data,
                                                                 GDestroyNotify               func_notify);
GDK_DEPRECATED_IN_4_10
void                bobgui_entry_completion_set_minimum_key_length (BobguiEntryCompletion          *completion,
                                                                 int                          length);
GDK_DEPRECATED_IN_4_10
int                 bobgui_entry_completion_get_minimum_key_length (BobguiEntryCompletion          *completion);
GDK_DEPRECATED_IN_4_10
char *             bobgui_entry_completion_compute_prefix         (BobguiEntryCompletion          *completion,
                                                                 const char                  *key);
GDK_DEPRECATED_IN_4_10
void                bobgui_entry_completion_complete               (BobguiEntryCompletion          *completion);
GDK_DEPRECATED_IN_4_10
void                bobgui_entry_completion_insert_prefix          (BobguiEntryCompletion          *completion);

GDK_DEPRECATED_IN_4_10
void                bobgui_entry_completion_set_inline_completion  (BobguiEntryCompletion          *completion,
                                                                 gboolean                     inline_completion);
GDK_DEPRECATED_IN_4_10
gboolean            bobgui_entry_completion_get_inline_completion  (BobguiEntryCompletion          *completion);
GDK_DEPRECATED_IN_4_10
void                bobgui_entry_completion_set_inline_selection  (BobguiEntryCompletion          *completion,
                                                                 gboolean                     inline_selection);
GDK_DEPRECATED_IN_4_10
gboolean            bobgui_entry_completion_get_inline_selection  (BobguiEntryCompletion          *completion);
GDK_DEPRECATED_IN_4_10
void                bobgui_entry_completion_set_popup_completion   (BobguiEntryCompletion          *completion,
                                                                 gboolean                     popup_completion);
GDK_DEPRECATED_IN_4_10
gboolean            bobgui_entry_completion_get_popup_completion   (BobguiEntryCompletion          *completion);
GDK_DEPRECATED_IN_4_10
void                bobgui_entry_completion_set_popup_set_width    (BobguiEntryCompletion          *completion,
                                                                 gboolean                     popup_set_width);
GDK_DEPRECATED_IN_4_10
gboolean            bobgui_entry_completion_get_popup_set_width    (BobguiEntryCompletion          *completion);
GDK_DEPRECATED_IN_4_10
void                bobgui_entry_completion_set_popup_single_match (BobguiEntryCompletion          *completion,
                                                                 gboolean                     popup_single_match);
GDK_DEPRECATED_IN_4_10
gboolean            bobgui_entry_completion_get_popup_single_match (BobguiEntryCompletion          *completion);

GDK_DEPRECATED_IN_4_10
const char          *bobgui_entry_completion_get_completion_prefix (BobguiEntryCompletion *completion);
/* convenience */
GDK_DEPRECATED_IN_4_10
void                bobgui_entry_completion_set_text_column        (BobguiEntryCompletion          *completion,
                                                                 int                          column);
GDK_DEPRECATED_IN_4_10
int                 bobgui_entry_completion_get_text_column        (BobguiEntryCompletion          *completion);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(BobguiEntryCompletion, g_object_unref)

G_END_DECLS

