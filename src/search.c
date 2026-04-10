/*
 *      search.c - this file is part of Geany, a fast and lightweight IDE
 *
 *      Copyright 2006 The Geany contributors
 *
 *      This program is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation; either version 2 of the License, or
 *      (at your option) any later version.
 *
 *      This program is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 *
 *      You should have received a copy of the GNU General Public License along
 *      with this program; if not, write to the Free Software Foundation, Inc.,
 *      51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

/*
 * Find, Replace, Find in Files dialog related functions.
 * Note that the basic text find functions are in document.c.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "search.h"

#include "app.h"
#include "dialogs.h"
#include "document.h"
#include "encodings.h"
#include "encodingsprivate.h"
#include "keyfile.h"
#include "msgwindow.h"
#include "prefs.h"
#include "project.h"
#include "sciwrappers.h"
#include "spawn.h"
#include "stash.h"
#include "support.h"
#include "toolbar.h"
#include "ui_utils.h"
#include "utils.h"
#include "win32.h"

#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>

#include <gdk/gdkkeysyms.h>

#define MIN_DLG_BUTTON_SIZE 130

enum
{
	GEANY_RESPONSE_FIND = 1,
	GEANY_RESPONSE_FIND_PREVIOUS,
	GEANY_RESPONSE_FIND_IN_FILE,
	GEANY_RESPONSE_FIND_IN_SESSION,
	GEANY_RESPONSE_MARK,
	GEANY_RESPONSE_COUNT,
	GEANY_RESPONSE_REPLACE,
	GEANY_RESPONSE_REPLACE_AND_FIND,
	GEANY_RESPONSE_REPLACE_IN_SESSION,
	GEANY_RESPONSE_REPLACE_IN_FILE,
	GEANY_RESPONSE_REPLACE_IN_SEL
};


enum
{
	FILES_MODE_ALL,
	FILES_MODE_PROJECT,
	FILES_MODE_CUSTOM
};


enum
{
	STUDIO_RESULT_ACTION,
	STUDIO_RESULT_TARGET,
	STUDIO_RESULT_QUERY,
	STUDIO_RESULT_MODE,
	STUDIO_RESULT_SUMMARY,
	STUDIO_RESULT_FILE,
	STUDIO_RESULT_LINE,
	STUDIO_RESULT_POS,
	STUDIO_RESULT_CAN_NAVIGATE,
	STUDIO_RESULT_PREVIEW_TITLE,
	STUDIO_RESULT_PREVIEW_BODY,
	STUDIO_RESULT_COLUMNS
};


enum
{
	STUDIO_LOWER_PAGE_ACTIVITY,
	STUDIO_LOWER_PAGE_RESULTS,
	STUDIO_LOWER_PAGE_DIFF_PREVIEW
};


GeanySearchData search_data;
GeanySearchPrefs search_prefs;


static struct
{
	gboolean fif_regexp;
	gboolean fif_case_sensitive;
	gboolean fif_match_whole_word;
	gboolean fif_invert_results;
	gboolean fif_recursive;
	gboolean fif_hidden_folders;
	gboolean fif_use_extra_options;
	gchar *fif_extra_options;
	gint fif_files_mode;
	gchar *fif_files;
	gboolean find_regexp;
	gboolean find_regexp_multiline;
	gboolean find_regexp_dot_matches_newline;
	gboolean find_escape_sequences;
	gboolean find_case_sensitive;
	gboolean find_match_whole_word;
	gboolean find_match_word_start;
	gboolean find_close_dialog;
	gboolean replace_regexp;
	gboolean replace_regexp_multiline;
	gboolean replace_regexp_dot_matches_newline;
	gboolean replace_escape_sequences;
	gboolean replace_case_sensitive;
	gboolean replace_match_whole_word;
	gboolean replace_match_word_start;
	gboolean replace_search_backwards;
	gboolean replace_close_dialog;
	gboolean transparency_enabled;
	gint transparency_mode;
	gint transparency_value;
}
settings;

static StashGroup *fif_prefs = NULL;
static StashGroup *find_prefs = NULL;
static StashGroup *replace_prefs = NULL;


static struct
{
	GtkWidget	*dialog;
	GtkWidget	*entry;
	gboolean	all_expanded;
	gint		position[2]; /* x, y */
}
find_dlg = {NULL, NULL, FALSE, {0, 0}};

static struct
{
	GtkWidget	*dialog;
	GtkWidget	*find_combobox;
	GtkWidget	*find_entry;
	GtkWidget	*replace_combobox;
	GtkWidget	*replace_entry;
	gboolean	all_expanded;
	gint		position[2]; /* x, y */
}
replace_dlg = {NULL, NULL, NULL, NULL, NULL, FALSE, {0, 0}};

static struct
{
	GtkWidget	*dialog;
	GtkWidget	*dir_combo;
	GtkWidget	*files_combo;
	GtkWidget	*search_combo;
	GtkWidget	*encoding_combo;
	GtkWidget	*files_mode_combo;
	gint		position[2]; /* x, y */
}
fif_dlg = {NULL, NULL, NULL, NULL, NULL, NULL, {0, 0}};

static struct
{
	GtkWidget	*dialog;
	GtkWidget	*notebook;
	GtkWidget	*find_page;
	GtkWidget	*replace_page;
	GtkWidget	*fif_page;
	GtkWidget	*fip_page;
	GtkWidget	*mark_page;
	GtkWidget	*lower_notebook;
	GtkWidget	*activity_view;
	GtkWidget	*results_view;
	GtkWidget	*preview_view;
	GtkListStore	*results_store;
	gint		position[2]; /* x, y */
}
studio_dlg = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, {0, 0}};

static struct
{
	gboolean active;
	gchar *query;
	gchar *mode;
	gchar *dir;
	guint results_added;
}
studio_fif_capture = { FALSE, NULL, NULL, NULL, 0 };


typedef struct SearchStudioResultSpec
{
	const gchar *action;
	const gchar *target;
	const gchar *query;
	const gchar *mode;
	const gchar *summary;
	const gchar *filename;
	gint line;
	gint pos;
	gboolean can_navigate;
	const gchar *preview_title;
	const gchar *preview_body;
}
SearchStudioResultSpec;


typedef struct SearchStudioFindSpec
{
	gchar *text;
	gchar *original_text;
	GeanyFindFlags flags;
	gboolean backwards;
	const gchar *mode;
}
SearchStudioFindSpec;


typedef struct SearchStudioReplaceSpec
{
	SearchStudioFindSpec find;
	gchar *replace;
	gchar *original_replace;
}
SearchStudioReplaceSpec;


typedef struct SearchStudioSessionRunResult
{
	guint total_results;
	guint docs_with_results;
}
SearchStudioSessionRunResult;


typedef guint (*SearchStudioOpenDocumentFunc)(GeanyDocument *doc, gpointer user_data);


typedef struct SearchStudioFindSessionActionSpec
{
	const SearchStudioFindSpec *find;
	SearchStudioOpenDocumentFunc open_document_func;
	gpointer open_document_data;
	gboolean add_history;
	gboolean store_find_spec;
}
SearchStudioFindSessionActionSpec;


typedef struct SearchStudioReplaceSessionActionSpec
{
	const SearchStudioReplaceSpec *replace;
	SearchStudioOpenDocumentFunc open_document_func;
	gpointer open_document_data;
	gboolean add_history;
	gboolean store_find_spec;
}
SearchStudioReplaceSessionActionSpec;


typedef enum SearchStudioReplaceSessionRowsKind
{
	SEARCH_STUDIO_REPLACE_SESSION_ROWS_PREVIEW,
	SEARCH_STUDIO_REPLACE_SESSION_ROWS_IMPACT
}
SearchStudioReplaceSessionRowsKind;


typedef struct SearchStudioReplaceSessionRowsActionContext
{
	SearchStudioReplaceSessionRowsKind kind;
	const gchar *action;
	const gchar *query;
	GeanyFindFlags flags;
	const gchar *mode;
	const gchar *replace_text;
	const gchar *replace_display;
	guint per_doc_limit;
}
SearchStudioReplaceSessionRowsActionContext;


typedef struct SearchStudioReplacePlanResult
{
	guint planned_matches;
	guint planned_docs;
}
SearchStudioReplacePlanResult;


typedef struct SearchStudioReplaceExecutionResult
{
	SearchStudioReplacePlanResult plan;
	guint applied_matches;
	guint affected_docs;
}
SearchStudioReplaceExecutionResult;


typedef struct SearchStudioStoredResultRow
{
	gchar *action;
	gchar *target;
	gchar *query;
	gchar *mode;
	gchar *summary;
	gchar *filename;
	gint line;
	gint pos;
	gboolean can_navigate;
	gchar *preview_title;
	gchar *preview_body;
}
SearchStudioStoredResultRow;


static void search_read_io(GString *string, GIOCondition condition, gpointer data);
static void search_read_io_stderr(GString *string, GIOCondition condition, gpointer data);

static void search_finished(GPid child_pid, gint status, gpointer user_data);

static gchar **search_get_argv(const gchar **argv_prefix, const gchar *dir);

static GRegex *compile_regex(const gchar *str, GeanyFindFlags sflags);


static void
on_find_replace_checkbutton_toggled(GtkToggleButton *togglebutton, gpointer user_data);

static void
on_find_dialog_response(GtkDialog *dialog, gint response, gpointer user_data);

static void
on_find_entry_activate(GtkEntry *entry, gpointer user_data);

static void
on_find_entry_activate_backward(GtkEntry *entry, gpointer user_data);

static void
on_replace_dialog_response(GtkDialog *dialog, gint response, gpointer user_data);

static void
on_replace_find_entry_activate(GtkEntry *entry, gpointer user_data);

static void
on_replace_entry_activate(GtkEntry *entry, gpointer user_data);

static void
on_find_in_files_dialog_response(GtkDialog *dialog, gint response, gpointer user_data);

static gboolean
search_find_in_files(const gchar *utf8_search_text, const gchar *dir, const gchar *opts,
	const gchar *enc, gboolean recursive);
static GString *build_grep_options(gboolean invert_results, gboolean case_sensitive,
	gboolean match_whole_word, gboolean recursive, gboolean regexp,
	gboolean use_extra_options, const gchar *extra_options, gint files_mode,
	const gchar *files);
static gboolean execute_find_in_files_request(const gchar *search_text, const gchar *utf8_dir,
	gboolean invert_results, gboolean case_sensitive, gboolean match_whole_word,
	gboolean recursive, gboolean regexp, gboolean use_extra_options,
	const gchar *extra_options, gint files_mode, const gchar *files,
	GeanyEncodingIndex enc_idx, GtkWidget *search_widget, GtkWidget *dir_widget,
	GtkWidget *files_widget, gboolean capture_results, const gchar *capture_mode);
static GtkWidget *search_studio_create_find_page(void);
static GtkWidget *search_studio_create_replace_page(void);
static GtkWidget *search_studio_create_fif_page(void);
static GtkWidget *search_studio_create_fip_page(void);
static GtkWidget *search_studio_create_mark_page(void);
static void search_studio_show_page(gint page_num);
static void search_studio_sync_find_dialog_from_page(GtkWidget *page);
static void search_studio_sync_replace_dialog_from_page(GtkWidget *page);
static void search_studio_mode_toggled(GtkToggleButton *togglebutton, gpointer user_data);
static void search_studio_whole_word_toggled(GtkToggleButton *togglebutton, gpointer user_data);
static gboolean search_studio_prepare_find(GtkWidget *page, const gchar *entry_name,
	gchar **text, gchar **original_text, GeanyFindFlags *flags, gboolean *backwards);
static void search_studio_find_spec_clear(SearchStudioFindSpec *spec);
static gboolean search_studio_build_find_spec(GtkWidget *page, const gchar *entry_name,
	SearchStudioFindSpec *spec);
static void search_studio_store_find_spec(const SearchStudioFindSpec *spec);
static void search_studio_add_combo_history_text(GtkWidget *page, const gchar *combo_name,
	const gchar *text);
static void search_studio_add_find_history(GtkWidget *page, const SearchStudioFindSpec *spec);
static gboolean search_studio_prepare_replace(GtkWidget *page, gchar **find, gchar **replace,
	gchar **original_find, gchar **original_replace, GeanyFindFlags *flags,
	gboolean *backwards);
static void search_studio_replace_spec_clear(SearchStudioReplaceSpec *spec);
static gboolean search_studio_build_replace_spec(GtkWidget *page, SearchStudioReplaceSpec *spec);
static void search_studio_add_replace_history(GtkWidget *page, const SearchStudioReplaceSpec *spec);
static void search_studio_report_count_status(const gchar *query, guint count,
	gboolean session_scope, guint doc_count);
static void search_studio_report_mark_status(const gchar *query, guint count,
	gboolean bookmark_lines, gboolean session_scope, guint doc_count);
static gchar *search_studio_document_target(GeanyDocument *doc);
static void search_studio_append_document_result(const gchar *action, GeanyDocument *doc,
	const gchar *query, const gchar *mode, const gchar *summary);
static void search_studio_append_document_resultf(const gchar *action, GeanyDocument *doc,
	const gchar *query, const gchar *mode, const gchar *summary_format, ...) G_GNUC_PRINTF(5, 6);
static void search_studio_store_search_data(const gchar *text, const gchar *original_text,
	GeanyFindFlags flags, gboolean backwards);
static void search_studio_replace_action_activate(GtkButton *button, gpointer user_data);
static void search_studio_replace_preview_document(GtkButton *button, gpointer user_data);
static void search_studio_replace_preview_session(GtkButton *button, gpointer user_data);
static guint search_studio_append_replace_preview_rows(GeanyDocument *doc, const gchar *query,
	GeanyFindFlags flags, const gchar *mode, const gchar *replace_text,
	const gchar *replace_display, guint limit);
static gchar *search_studio_build_replace_preview_body(GeanyDocument *doc,
	const GeanyMatchInfo *info, const gchar *replace_text, const gchar *replace_display);
static guint search_studio_append_replace_impact_rows(const gchar *action, GeanyDocument *doc,
	const gchar *query, GeanyFindFlags flags, const gchar *mode, const gchar *replace_text,
	const gchar *replace_display);
static guint search_studio_replace_session_rows_action_cb(GeanyDocument *doc,
	gpointer user_data);
static SearchStudioSessionRunResult search_studio_execute_replace_session_rows_action(
	GtkWidget *page, const SearchStudioReplaceSpec *spec,
	SearchStudioReplaceSessionRowsActionContext *ctx, gboolean add_history,
	gboolean store_find_spec);
static SearchStudioReplacePlanResult search_studio_plan_replace_document(
	GeanyDocument *doc, const SearchStudioReplaceSpec *spec);
static SearchStudioReplacePlanResult search_studio_plan_replace_session(GtkWidget *page,
	const SearchStudioReplaceSpec *spec);
static SearchStudioReplaceExecutionResult search_studio_execute_replace_document(
	GeanyDocument *doc, const SearchStudioReplaceSpec *spec);
static SearchStudioReplaceExecutionResult search_studio_execute_replace_session(
	GtkWidget *page, GeanyDocument *doc, const SearchStudioReplaceSpec *spec);
static void search_studio_fif_find_activate(GtkButton *button, gpointer user_data);
static void search_studio_find_in_project_activate(GtkButton *button, gpointer user_data);
static void search_studio_browse_dir_activate(GtkButton *button, gpointer user_data);
static void search_studio_use_current_doc_dir_activate(GtkButton *button, gpointer user_data);
static void search_studio_use_project_dir_activate(GtkButton *button, gpointer user_data);
static void search_studio_fif_file_mode_changed(GtkComboBox *combo, gpointer user_data);
static void search_studio_notebook_switch_page(GtkNotebook *notebook, GtkWidget *page,
	guint page_num, gpointer user_data);
static void search_studio_results_row_activated(GtkTreeView *tree_view, GtkTreePath *path,
	GtkTreeViewColumn *column, gpointer user_data);
static void search_studio_results_selection_changed(GtkTreeSelection *selection, gpointer user_data);
static gboolean search_studio_results_focus_current(void);
static gboolean search_studio_results_select_relative(gint delta, gboolean activate_row);
static void search_studio_focus_results_activate(GtkButton *button, gpointer user_data);
static void search_studio_next_result_activate(GtkButton *button, gpointer user_data);
static void search_studio_prev_result_activate(GtkButton *button, gpointer user_data);
static void search_studio_activity_append(const gchar *format, ...) G_GNUC_PRINTF(1, 2);
static void search_studio_set_preview(const gchar *title, const gchar *body);
static void search_studio_result_append_spec(const SearchStudioResultSpec *spec);
static void search_studio_result_append(const gchar *action, const gchar *target,
	const gchar *query, const gchar *mode, const gchar *summary);
static void search_studio_result_appendf(const gchar *action, const gchar *target,
	const gchar *query, const gchar *mode, const gchar *summary_format, ...) G_GNUC_PRINTF(5, 6);
static gchar *search_studio_build_line_preview_body(GeanyDocument *doc, gint pos,
	const gchar *query, const gchar *kind_label);
static void search_studio_hide_on_delete_cb(GtkWidget *widget, GdkEvent *event, gpointer user_data);
static void search_studio_open_find_dialog_activate(GtkButton *button, gpointer user_data);
static void search_studio_open_replace_dialog_activate(GtkButton *button, gpointer user_data);
static void search_studio_open_fif_dialog_activate(GtkButton *button, gpointer user_data);
static void search_studio_swap_find_replace(GtkButton *button, gpointer user_data);
static void search_studio_clear_results(GtkButton *button, gpointer user_data);
static void search_studio_mark_activate(GtkButton *button, gpointer user_data);
static void search_studio_find_next_activate(GtkButton *button, gpointer user_data);
static void search_studio_find_previous_activate(GtkButton *button, gpointer user_data);
static void search_studio_count_activate(GtkButton *button, gpointer user_data);
static void search_studio_collect_document_hits(GtkButton *button, gpointer user_data);
static void search_studio_collect_session_hits(GtkButton *button, gpointer user_data);
static void search_studio_clear_marks_activate(GtkButton *button, gpointer user_data);
static void search_studio_inverse_marks_activate(GtkButton *button, gpointer user_data);
static void search_studio_copy_marked_lines_activate(GtkButton *button, gpointer user_data);
static void search_studio_delete_marked_lines_activate(GtkButton *button, gpointer user_data);
static gboolean search_studio_hide_on_delete(GtkWidget *widget, GdkEvent *event, gpointer user_data);

static void search_studio_result_append_preview_match(const gchar *action, GeanyDocument *doc,
	const gchar *query, const gchar *mode, gint pos, const gchar *summary,
	const gchar *preview_title, const gchar *preview_body);
static guint search_studio_append_match_rows(const gchar *action, GeanyDocument *doc,
	const gchar *query, GeanyFindFlags flags, const gchar *mode, guint limit);
static guint search_studio_visit_open_documents(SearchStudioOpenDocumentFunc func,
	gpointer user_data, guint *docs_with_results);
static SearchStudioSessionRunResult search_studio_run_session_action(
	SearchStudioOpenDocumentFunc func, gpointer user_data);
static SearchStudioSessionRunResult search_studio_execute_find_session_action(GtkWidget *page,
	const SearchStudioFindSessionActionSpec *action);
static SearchStudioSessionRunResult search_studio_execute_replace_session_action(GtkWidget *page,
	const SearchStudioReplaceSessionActionSpec *action);
static guint search_studio_append_session_match_rows(const gchar *action,
	const gchar *query, GeanyFindFlags flags, const gchar *mode, guint per_doc_limit);
static void search_studio_find_in_results_activate(GtkButton *button, gpointer user_data);
static guint search_studio_append_count_impact_row(const gchar *action, GeanyDocument *doc,
	const gchar *query, GeanyFindFlags flags, const gchar *mode);
static void search_studio_count_session_activate(GtkButton *button, gpointer user_data);
static guint search_studio_append_mark_impact_row(const gchar *action, GeanyDocument *doc,
	const gchar *query, GeanyFindFlags flags, const gchar *mode,
	gboolean bookmark_lines, gboolean purge_bookmarks);
static gboolean search_studio_line_is_marked(GeanyDocument *doc, gint line);
static gboolean *search_studio_collect_marked_line_states(GeanyDocument *doc,
	guint *line_count_out, guint *marked_line_count_out);
static guint search_studio_copy_marked_lines_to_clipboard(GeanyDocument *doc);
static guint search_studio_delete_marked_lines(GeanyDocument *doc, gboolean delete_marked_lines);
static guint search_studio_inverse_marked_lines(GeanyDocument *doc);
static void search_studio_cut_marked_lines_activate(GtkButton *button, gpointer user_data);
static void search_studio_delete_unmarked_lines_activate(GtkButton *button, gpointer user_data);
static void search_studio_mark_session_activate(GtkButton *button, gpointer user_data);
static void search_studio_clear_session_marks_activate(GtkButton *button, gpointer user_data);
static void search_studio_capture_begin(const gchar *query, const gchar *mode, const gchar *dir);
static void search_studio_capture_finish(gint exit_status);
static void search_studio_capture_grep_result(const gchar *utf8_msg);
static void search_studio_activity_show_page_hint(gint page_num);
static const gchar *search_studio_mode_name(GtkWidget *page);
static gint search_mark_all_with_options(GeanyDocument *doc, const gchar *search_text,
	GeanyFindFlags flags, gboolean bookmark_lines, gboolean purge_bookmarks);
static void search_clear_all_marks(GeanyDocument *doc);


static void init_prefs(void)
{
	StashGroup *group;

	group = stash_group_new("search");
	configuration_add_pref_group(group, TRUE);
	stash_group_add_toggle_button(group, &search_prefs.always_wrap,
		"pref_search_hide_find_dialog", FALSE, "check_always_wrap_search");
	stash_group_add_toggle_button(group, &search_prefs.hide_find_dialog,
		"pref_search_always_wrap", FALSE, "check_hide_find_dialog");
	stash_group_add_toggle_button(group, &search_prefs.use_current_file_dir,
		"pref_search_current_file_dir", TRUE, "check_fif_current_dir");

	/* dialog layout & positions */
	group = stash_group_new("search");
	configuration_add_session_group(group, FALSE);
	stash_group_add_boolean(group, &find_dlg.all_expanded, "find_all_expanded", FALSE);
	stash_group_add_boolean(group, &replace_dlg.all_expanded, "replace_all_expanded", FALSE);
	stash_group_add_integer(group, &find_dlg.position[0], "position_find_x", -1);
	stash_group_add_integer(group, &find_dlg.position[1], "position_find_y", -1);
	stash_group_add_integer(group, &replace_dlg.position[0], "position_replace_x", -1);
	stash_group_add_integer(group, &replace_dlg.position[1], "position_replace_y", -1);
	stash_group_add_integer(group, &fif_dlg.position[0], "position_fif_x", -1);
	stash_group_add_integer(group, &fif_dlg.position[1], "position_fif_y", -1);
	stash_group_add_integer(group, &studio_dlg.position[0], "position_search_studio_x", -1);
	stash_group_add_integer(group, &studio_dlg.position[1], "position_search_studio_y", -1);

	memset(&settings, '\0', sizeof(settings));

	group = stash_group_new("search");
	fif_prefs = group;
	configuration_add_pref_group(group, FALSE);
	stash_group_add_toggle_button(group, &settings.fif_regexp,
		"fif_regexp", FALSE, "check_regexp");
	stash_group_add_toggle_button(group, &settings.fif_case_sensitive,
		"fif_case_sensitive", TRUE, "check_case");
	stash_group_add_toggle_button(group, &settings.fif_match_whole_word,
		"fif_match_whole_word", FALSE, "check_wholeword");
	stash_group_add_toggle_button(group, &settings.fif_invert_results,
		"fif_invert_results", FALSE, "check_invert");
	stash_group_add_toggle_button(group, &settings.fif_recursive,
		"fif_recursive", FALSE, "check_recursive");
	stash_group_add_toggle_button(group, &settings.fif_hidden_folders,
		"fif_hidden_folders", FALSE, "check_hidden");
	stash_group_add_entry(group, &settings.fif_extra_options,
		"fif_extra_options", "", "entry_extra");
	stash_group_add_toggle_button(group, &settings.fif_use_extra_options,
		"fif_use_extra_options", FALSE, "check_extra");
	stash_group_add_entry(group, &settings.fif_files,
		"fif_files", "", "entry_files");
	stash_group_add_combo_box(group, &settings.fif_files_mode,
		"fif_files_mode", FILES_MODE_ALL, "combo_files_mode");

	group = stash_group_new("search");
	find_prefs = group;
	configuration_add_pref_group(group, FALSE);
	stash_group_add_toggle_button(group, &settings.find_regexp,
		"find_regexp", FALSE, "check_regexp");
	stash_group_add_toggle_button(group, &settings.find_regexp_multiline,
		"find_regexp_multiline", FALSE, "check_multiline");
	stash_group_add_toggle_button(group, &settings.find_regexp_dot_matches_newline,
		"find_regexp_dot_matches_newline", FALSE, "check_dotmatchnewline");
	stash_group_add_toggle_button(group, &settings.find_case_sensitive,
		"find_case_sensitive", FALSE, "check_case");
	stash_group_add_toggle_button(group, &settings.find_escape_sequences,
		"find_escape_sequences", FALSE, "check_escape");
	stash_group_add_toggle_button(group, &settings.find_match_whole_word,
		"find_match_whole_word", FALSE, "check_word");
	stash_group_add_toggle_button(group, &settings.find_match_word_start,
		"find_match_word_start", FALSE, "check_wordstart");
	stash_group_add_toggle_button(group, &settings.find_close_dialog,
		"find_close_dialog", TRUE, "check_close");

	group = stash_group_new("search");
	replace_prefs = group;
	configuration_add_pref_group(group, FALSE);
	stash_group_add_toggle_button(group, &settings.replace_regexp,
		"replace_regexp", FALSE, "check_regexp");
	stash_group_add_toggle_button(group, &settings.replace_regexp_multiline,
		"replace_regexp_multiline", FALSE, "check_multiline");
	stash_group_add_toggle_button(group, &settings.replace_regexp_dot_matches_newline,
		"replace_regexp_dot_matches_newline", FALSE, "check_dotmatchnewline");
	stash_group_add_toggle_button(group, &settings.replace_case_sensitive,
		"replace_case_sensitive", FALSE, "check_case");
	stash_group_add_toggle_button(group, &settings.replace_escape_sequences,
		"replace_escape_sequences", FALSE, "check_escape");
	stash_group_add_toggle_button(group, &settings.replace_match_whole_word,
		"replace_match_whole_word", FALSE, "check_word");
	stash_group_add_toggle_button(group, &settings.replace_match_word_start,
		"replace_match_word_start", FALSE, "check_wordstart");
	stash_group_add_toggle_button(group, &settings.replace_search_backwards,
		"replace_search_backwards", FALSE, "check_back");
	stash_group_add_toggle_button(group, &settings.replace_close_dialog,
		"replace_close_dialog", TRUE, "check_close");

	group = stash_group_new("search");
	configuration_add_pref_group(group, FALSE);
	stash_group_add_toggle_button(group, &settings.transparency_enabled,
		"transparency_enabled", FALSE, "check_transparency");
	stash_group_add_integer(group, &settings.transparency_mode,
		"transparency_mode", 0, "radio_transparency_mode");
	stash_group_add_integer(group, &settings.transparency_value,
		"transparency_value", 200, "scale_transparency_value");
}


void search_init(void)
{
	search_data.text = NULL;
	search_data.original_text = NULL;
	init_prefs();
}


#define FREE_WIDGET(wid) \
	if (wid && GTK_IS_WIDGET(wid)) gtk_widget_destroy(wid);

void search_finalize(void)
{
	FREE_WIDGET(find_dlg.dialog);
	FREE_WIDGET(replace_dlg.dialog);
	FREE_WIDGET(fif_dlg.dialog);
	FREE_WIDGET(studio_dlg.dialog);
	g_free(search_data.text);
	g_free(search_data.original_text);
	g_free(studio_fif_capture.query);
	g_free(studio_fif_capture.mode);
	g_free(studio_fif_capture.dir);
}


static void on_widget_toggled_set_insensitive(
	GtkToggleButton *togglebutton, gpointer user_data)
{
	gtk_widget_set_sensitive(GTK_WIDGET(user_data),
		!gtk_toggle_button_get_active(togglebutton));
}


static GtkWidget *add_find_checkboxes(GtkDialog *dialog)
{
	GtkWidget *checkbox1, *checkbox2, *check_regexp, *checkbox5,
			  *checkbox7, *check_multiline, *check_dotmatchnewline,
			  *check_wrap, *hbox, *fbox, *mbox;

	check_regexp = gtk_check_button_new_with_mnemonic(_("_Use regular expressions"));
	ui_hookup_widget(dialog, check_regexp, "check_regexp");
	gtk_button_set_focus_on_click(GTK_BUTTON(check_regexp), FALSE);
	gtk_widget_set_tooltip_text(check_regexp, _("Use Perl-like regular expressions. "
		"For detailed information about using regular expressions, please refer to the manual."));
	g_signal_connect(check_regexp, "toggled",
		G_CALLBACK(on_find_replace_checkbutton_toggled), dialog);

	checkbox7 = gtk_check_button_new_with_mnemonic(_("Use _escape sequences"));
	ui_hookup_widget(dialog, checkbox7, "check_escape");
	gtk_button_set_focus_on_click(GTK_BUTTON(checkbox7), FALSE);
	gtk_widget_set_tooltip_text(checkbox7,
		_("Replace \\\\, \\t, \\n, \\r and \\uXXXX (Unicode characters) with the "
		  "corresponding control characters"));

	check_multiline = gtk_check_button_new_with_mnemonic(_("Use multi-line matchin_g"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_multiline), FALSE);
	gtk_widget_set_sensitive(check_multiline, FALSE);
	ui_hookup_widget(dialog, check_multiline, "check_multiline");
	gtk_button_set_focus_on_click(GTK_BUTTON(check_multiline), FALSE);
	gtk_widget_set_tooltip_text(check_multiline, _("Perform regular expression "
		"matching on the whole buffer at once rather than line by line, allowing "
		"matches to span multiple lines. In this mode, newline characters are part "
		"of the input and can be captured as normal characters by the pattern."));

	check_dotmatchnewline = gtk_check_button_new_with_mnemonic(_("._ matches new line"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_dotmatchnewline), FALSE);
	gtk_widget_set_sensitive(check_dotmatchnewline, FALSE);
	ui_hookup_widget(dialog, check_dotmatchnewline, "check_dotmatchnewline");
	gtk_button_set_focus_on_click(GTK_BUTTON(check_dotmatchnewline), FALSE);
	gtk_widget_set_tooltip_text(check_dotmatchnewline,
		_("Make the dot in regular expressions also match newline characters, similar to Notepad++ regex dot-all mode."));

	check_wrap = gtk_check_button_new_with_mnemonic(_("Wrap ar_ound"));
	ui_hookup_widget(dialog, check_wrap, "check_wrap");
	gtk_button_set_focus_on_click(GTK_BUTTON(check_wrap), FALSE);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_wrap), search_prefs.always_wrap);
	gtk_widget_set_tooltip_text(check_wrap,
		_("Wrap around the document automatically instead of prompting when the search reaches the end."));

	/* Search features */
	fbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_box_pack_start(GTK_BOX(fbox), check_regexp, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(fbox), check_multiline, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(fbox), check_dotmatchnewline, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(fbox), checkbox7, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(fbox), check_wrap, FALSE, FALSE, 0);

	if (dialog != GTK_DIALOG(find_dlg.dialog))
	{
		GtkWidget *check_back = gtk_check_button_new_with_mnemonic(_("Search _backwards"));
		ui_hookup_widget(dialog, check_back, "check_back");
		gtk_button_set_focus_on_click(GTK_BUTTON(check_back), FALSE);
		gtk_container_add(GTK_CONTAINER(fbox), check_back);
	}

	checkbox1 = gtk_check_button_new_with_mnemonic(_("C_ase sensitive"));
	ui_hookup_widget(dialog, checkbox1, "check_case");
	gtk_button_set_focus_on_click(GTK_BUTTON(checkbox1), FALSE);

	checkbox2 = gtk_check_button_new_with_mnemonic(_("Match only a _whole word"));
	ui_hookup_widget(dialog, checkbox2, "check_word");
	gtk_button_set_focus_on_click(GTK_BUTTON(checkbox2), FALSE);

	checkbox5 = gtk_check_button_new_with_mnemonic(_("Match from s_tart of word"));
	ui_hookup_widget(dialog, checkbox5, "check_wordstart");
	gtk_button_set_focus_on_click(GTK_BUTTON(checkbox5), FALSE);

	/* disable wordstart when wholeword is checked */
	g_signal_connect(checkbox2, "toggled",
		G_CALLBACK(on_widget_toggled_set_insensitive), checkbox5);

	/* Matching options */
	mbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_box_pack_start(GTK_BOX(mbox), checkbox1, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(mbox), checkbox2, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(mbox), checkbox5, FALSE, FALSE, 0);

	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
	gtk_box_pack_start(GTK_BOX(hbox), fbox, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), mbox, TRUE, TRUE, 0);
	return hbox;
}


static void send_find_dialog_response(GtkButton *button, gpointer user_data)
{
	gtk_dialog_response(GTK_DIALOG(find_dlg.dialog), GPOINTER_TO_INT(user_data));
}


static GtkWidget *search_studio_create_mode_box(GtkWidget *owner)
{
	GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
	GtkWidget *normal = gtk_radio_button_new_with_mnemonic(NULL, _("_Normal"));
	GtkWidget *extended = gtk_radio_button_new_with_mnemonic_from_widget(GTK_RADIO_BUTTON(normal), _("_Extended (\\n, \\r, \\t, \\0, \\x...)"));
	GtkWidget *regex = gtk_radio_button_new_with_mnemonic_from_widget(GTK_RADIO_BUTTON(normal), _("Regular _expression"));

	ui_hookup_widget(owner, normal, "mode_normal");
	ui_hookup_widget(owner, extended, "mode_extended");
	ui_hookup_widget(owner, regex, "mode_regex");

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(normal), TRUE);
	gtk_box_pack_start(GTK_BOX(box), normal, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(box), extended, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(box), regex, FALSE, FALSE, 0);

	g_signal_connect(normal, "toggled", G_CALLBACK(search_studio_mode_toggled), owner);
	g_signal_connect(extended, "toggled", G_CALLBACK(search_studio_mode_toggled), owner);
	g_signal_connect(regex, "toggled", G_CALLBACK(search_studio_mode_toggled), owner);

	return box;
}


static void search_studio_get_mode(GtkWidget *owner, gboolean *regexp, gboolean *escape_sequences)
{
	GtkWidget *mode_regex = ui_lookup_widget(owner, "mode_regex");
	GtkWidget *mode_extended = ui_lookup_widget(owner, "mode_extended");

	*regexp = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(mode_regex));
	*escape_sequences = !*regexp && gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(mode_extended));
}


static void search_studio_mode_toggled(GtkToggleButton *togglebutton, gpointer user_data)
{
	GtkWidget *owner = GTK_WIDGET(user_data);
	gboolean regex_mode;
	gboolean escape_mode;
	GtkWidget *check_multiline = ui_lookup_widget(owner, "check_multiline");
	GtkWidget *check_dotall = ui_lookup_widget(owner, "check_dotall");
	GtkWidget *check_word = ui_lookup_widget(owner, "check_word");
	GtkWidget *check_wordstart = ui_lookup_widget(owner, "check_wordstart");

	if (!gtk_toggle_button_get_active(togglebutton))
		return;

	search_studio_get_mode(owner, &regex_mode, &escape_mode);
	(void) escape_mode;

	if (check_multiline)
		gtk_widget_set_sensitive(check_multiline, regex_mode);
	if (check_dotall)
		gtk_widget_set_sensitive(check_dotall, regex_mode);
	if (check_word)
		gtk_widget_set_sensitive(check_word, !regex_mode);
	if (check_wordstart)
		gtk_widget_set_sensitive(check_wordstart,
			!regex_mode && !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(check_word)));
	}


static void search_studio_whole_word_toggled(GtkToggleButton *togglebutton, gpointer user_data)
{
	GtkWidget *owner = GTK_WIDGET(user_data);
	GtkWidget *check_wordstart = ui_lookup_widget(owner, "check_wordstart");
	GtkWidget *mode_regex = ui_lookup_widget(owner, "mode_regex");
	gboolean regex_mode = mode_regex && gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(mode_regex));

	if (check_wordstart)
		gtk_widget_set_sensitive(check_wordstart,
			!regex_mode && !gtk_toggle_button_get_active(togglebutton));
}


static GtkWidget *search_studio_create_common_options(GtkWidget *owner,
	gboolean include_backwards, gboolean include_bookmarking)
{
	GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
	GtkWidget *options_frame = gtk_frame_new(_("Search Mode"));
	GtkWidget *inner_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
	GtkWidget *mode_box = search_studio_create_mode_box(owner);
	GtkWidget *options_grid = gtk_grid_new();
	GtkWidget *check_case = gtk_check_button_new_with_mnemonic(_("Match _case"));
	GtkWidget *check_word = gtk_check_button_new_with_mnemonic(_("Whole _word only"));
	GtkWidget *check_wordstart = gtk_check_button_new_with_mnemonic(_("Word _start"));
	GtkWidget *check_wrap = gtk_check_button_new_with_mnemonic(_("Wrap ar_ound"));
	GtkWidget *check_multiline = gtk_check_button_new_with_mnemonic(_("Multi-_line regex"));
	GtkWidget *check_dotall = gtk_check_button_new_with_mnemonic(_("._ matches newline"));

	gtk_container_set_border_width(GTK_CONTAINER(inner_vbox), 6);
	gtk_box_pack_start(GTK_BOX(inner_vbox), mode_box, FALSE, FALSE, 0);
	gtk_container_add(GTK_CONTAINER(options_frame), inner_vbox);

	ui_hookup_widget(owner, check_case, "check_case");
	ui_hookup_widget(owner, check_word, "check_word");
	ui_hookup_widget(owner, check_wordstart, "check_wordstart");
	ui_hookup_widget(owner, check_wrap, "check_wrap");
	ui_hookup_widget(owner, check_multiline, "check_multiline");
	ui_hookup_widget(owner, check_dotall, "check_dotall");

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_wrap), search_prefs.always_wrap);
	gtk_widget_set_sensitive(check_multiline, FALSE);
	gtk_widget_set_sensitive(check_dotall, FALSE);

	g_signal_connect(check_word, "toggled", G_CALLBACK(search_studio_whole_word_toggled), owner);

	gtk_grid_set_column_spacing(GTK_GRID(options_grid), 12);
	gtk_grid_set_row_spacing(GTK_GRID(options_grid), 6);
	gtk_grid_attach(GTK_GRID(options_grid), check_case, 0, 0, 1, 1);
	gtk_grid_attach(GTK_GRID(options_grid), check_word, 1, 0, 1, 1);
	gtk_grid_attach(GTK_GRID(options_grid), check_wordstart, 2, 0, 1, 1);
	gtk_grid_attach(GTK_GRID(options_grid), check_wrap, 0, 1, 1, 1);
	gtk_grid_attach(GTK_GRID(options_grid), check_multiline, 1, 1, 1, 1);
	gtk_grid_attach(GTK_GRID(options_grid), check_dotall, 2, 1, 1, 1);

	if (include_backwards)
	{
		GtkWidget *check_back = gtk_check_button_new_with_mnemonic(_("Search _backwards"));
		ui_hookup_widget(owner, check_back, "check_back");
		gtk_grid_attach(GTK_GRID(options_grid), check_back, 0, 2, 1, 1);
	}

	if (include_bookmarking)
	{
		GtkWidget *check_bookmark = gtk_check_button_new_with_mnemonic(_("Bookmark matching _lines"));
		GtkWidget *check_purge = gtk_check_button_new_with_mnemonic(_("_Purge existing bookmarks first"));
		ui_hookup_widget(owner, check_bookmark, "check_bookmark");
		ui_hookup_widget(owner, check_purge, "check_purge");
		gtk_grid_attach(GTK_GRID(options_grid), check_bookmark, 1, 2, 1, 1);
		gtk_grid_attach(GTK_GRID(options_grid), check_purge, 2, 2, 1, 1);
	}

	gtk_box_pack_start(GTK_BOX(box), options_frame, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(box), options_grid, FALSE, FALSE, 0);

	return box;
}


static gboolean search_studio_prepare_find(GtkWidget *page, const gchar *entry_name,
	gchar **text, gchar **original_text, GeanyFindFlags *flags, gboolean *backwards)
{
	GtkWidget *entry = ui_lookup_widget(page, entry_name);
	gboolean regexp;
	gboolean escape_sequences;

	*text = g_strdup(gtk_entry_get_text(GTK_ENTRY(entry)));
	*original_text = g_strdup(*text);
	search_studio_get_mode(page, &regexp, &escape_sequences);
	search_prefs.always_wrap = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(
		ui_lookup_widget(page, "check_wrap")));
	*backwards = FALSE;
	if (ui_lookup_widget(page, "check_back"))
		*backwards = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ui_lookup_widget(page, "check_back")));

	*flags = int_search_flags(
		gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ui_lookup_widget(page, "check_case"))),
		gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ui_lookup_widget(page, "check_word"))),
		regexp,
		gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ui_lookup_widget(page, "check_multiline"))),
		gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ui_lookup_widget(page, "check_dotall"))),
		gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ui_lookup_widget(page, "check_wordstart"))));

	if (EMPTY(*text))
		goto fail;

	if (*flags & GEANY_FIND_REGEXP)
	{
		GRegex *regex = compile_regex(*text, *flags);
		if (!regex)
			goto fail;
		g_regex_unref(regex);
	}
	else if (escape_sequences)
	{
		if (!utils_str_replace_escape(*text, FALSE))
			goto fail;
	}

	return TRUE;

fail:
	utils_beep();
	gtk_widget_grab_focus(entry);
	g_free(*text);
	g_free(*original_text);
	*text = NULL;
	*original_text = NULL;
	return FALSE;
}


static void search_studio_find_spec_clear(SearchStudioFindSpec *spec)
{
	if (spec == NULL)
		return;

	g_free(spec->text);
	g_free(spec->original_text);
	memset(spec, 0, sizeof *spec);
}


static gboolean search_studio_build_find_spec(GtkWidget *page, const gchar *entry_name,
	SearchStudioFindSpec *spec)
{
	g_return_val_if_fail(spec != NULL, FALSE);

	search_studio_find_spec_clear(spec);
	if (!search_studio_prepare_find(page, entry_name, &spec->text, &spec->original_text,
			&spec->flags, &spec->backwards))
		return FALSE;
	spec->mode = search_studio_mode_name(page);
	return TRUE;
}


static void search_studio_store_find_spec(const SearchStudioFindSpec *spec)
{
	if (spec == NULL)
		return;

	search_studio_store_search_data(spec->text, spec->original_text,
		spec->flags, spec->backwards);
}


static void search_studio_add_combo_history_text(GtkWidget *page, const gchar *combo_name,
	const gchar *text)
{
	GtkWidget *combo = ui_lookup_widget(page, combo_name);

	if (combo != NULL)
		ui_combo_box_add_to_history(GTK_COMBO_BOX_TEXT(combo), text, 0);
}


static void search_studio_add_find_history(GtkWidget *page, const SearchStudioFindSpec *spec)
{
	if (spec == NULL)
		return;

	search_studio_add_combo_history_text(page, "combo_search", spec->original_text);
}


static gboolean search_studio_prepare_replace(GtkWidget *page, gchar **find, gchar **replace,
	gchar **original_find, gchar **original_replace, GeanyFindFlags *flags,
	gboolean *backwards)
{
	gboolean regexp;
	gboolean escape_sequences;
	GtkWidget *find_entry = ui_lookup_widget(page, "entry_search");
	GtkWidget *replace_entry = ui_lookup_widget(page, "entry_replace");

	*find = g_strdup(gtk_entry_get_text(GTK_ENTRY(find_entry)));
	*replace = g_strdup(gtk_entry_get_text(GTK_ENTRY(replace_entry)));
	*original_find = g_strdup(*find);
	*original_replace = g_strdup(*replace);
	search_studio_get_mode(page, &regexp, &escape_sequences);
	search_prefs.always_wrap = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(
		ui_lookup_widget(page, "check_wrap")));
	*backwards = ui_lookup_widget(page, "check_back") &&
		gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ui_lookup_widget(page, "check_back")));

	*flags = int_search_flags(
		gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ui_lookup_widget(page, "check_case"))),
		gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ui_lookup_widget(page, "check_word"))),
		regexp,
		gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ui_lookup_widget(page, "check_multiline"))),
		gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ui_lookup_widget(page, "check_dotall"))),
		gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ui_lookup_widget(page, "check_wordstart"))));

	if (EMPTY(*find))
		goto fail;

	if ((*flags & GEANY_FIND_REGEXP) != 0)
	{
		GRegex *regex = compile_regex(*find, *flags);
		if (regex == NULL || !utils_str_replace_escape(*replace, TRUE))
		{
			if (regex)
				g_regex_unref(regex);
			goto fail;
		}
		g_regex_unref(regex);
	}
	else if (escape_sequences)
	{
		if (!utils_str_replace_escape(*find, FALSE) || !utils_str_replace_escape(*replace, FALSE))
			goto fail;
	}

	return TRUE;

fail:
	utils_beep();
	gtk_widget_grab_focus(find_entry);
	g_free(*find);
	g_free(*replace);
	g_free(*original_find);
	g_free(*original_replace);
	*find = NULL;
	*replace = NULL;
	*original_find = NULL;
	*original_replace = NULL;
	return FALSE;
}


static void search_studio_replace_spec_clear(SearchStudioReplaceSpec *spec)
{
	if (spec == NULL)
		return;

	search_studio_find_spec_clear(&spec->find);
	g_free(spec->replace);
	g_free(spec->original_replace);
	memset(spec, 0, sizeof *spec);
}


static gboolean search_studio_build_replace_spec(GtkWidget *page, SearchStudioReplaceSpec *spec)
{
	g_return_val_if_fail(spec != NULL, FALSE);

	search_studio_replace_spec_clear(spec);
	if (!search_studio_prepare_replace(page, &spec->find.text, &spec->replace,
			&spec->find.original_text, &spec->original_replace, &spec->find.flags,
			&spec->find.backwards))
		return FALSE;
	spec->find.mode = search_studio_mode_name(page);
	return TRUE;
}


static void search_studio_add_replace_history(GtkWidget *page, const SearchStudioReplaceSpec *spec)
{
	if (spec == NULL)
		return;

	search_studio_add_find_history(page, &spec->find);
	search_studio_add_combo_history_text(page, "combo_replace", spec->original_replace);
}


static void search_studio_report_count_status(const gchar *query, guint count,
	gboolean session_scope, guint doc_count)
{
	if (session_scope)
	{
		if (count == 0)
			ui_set_statusbar(FALSE, _("No matches found for \"%s\" across open documents."), query);
		else
			ui_set_statusbar(FALSE, _("Counted %u matches across %u open documents for \"%s\"."),
				count, doc_count, query);
		return;
	}

	if (count == 0)
		ui_set_statusbar(FALSE, _("No matches found for \"%s\"."), query);
	else
		ui_set_statusbar(FALSE,
			ngettext("Counted %d match for \"%s\".", "Counted %d matches for \"%s\".", count),
			count, query);
}


static void search_studio_report_mark_status(const gchar *query, guint count,
	gboolean bookmark_lines, gboolean session_scope, guint doc_count)
{
	if (session_scope)
	{
		if (count == 0)
			ui_set_statusbar(FALSE, _("No matches found for \"%s\" across open documents."), query);
		else
			ui_set_statusbar(FALSE, _("Marked %u matches across %u open documents for \"%s\"."),
				count, doc_count, query);
		return;
	}

	if (count == 0)
		ui_set_statusbar(FALSE, _("No matches found for \"%s\"."), query);
	else if (bookmark_lines)
		ui_set_statusbar(FALSE,
			ngettext("Marked %d match for \"%s\" and bookmarked its line.",
				"Marked %d matches for \"%s\" and bookmarked their lines.", count),
			count, query);
	else
		ui_set_statusbar(FALSE,
			ngettext("Marked %d match for \"%s\".", "Marked %d matches for \"%s\".", count),
			count, query);
}


static gchar *search_studio_document_target(GeanyDocument *doc)
{
	g_return_val_if_fail(DOC_VALID(doc), g_strdup(_("(none)")));
	return g_path_get_basename(DOC_FILENAME(doc));
}


static void search_studio_append_document_result(const gchar *action, GeanyDocument *doc,
	const gchar *query, const gchar *mode, const gchar *summary)
{
	gchar *target;

	if (!DOC_VALID(doc))
		return;

	target = search_studio_document_target(doc);
	search_studio_result_append(action, target, query, mode, summary);
	g_free(target);
}


static void search_studio_result_appendf(const gchar *action, const gchar *target,
	const gchar *query, const gchar *mode, const gchar *summary_format, ...)
{
	gchar *summary;
	va_list args;

	va_start(args, summary_format);
	summary = g_strdup_vprintf(summary_format, args);
	va_end(args);
	search_studio_result_append(action, target, query, mode, summary);
	g_free(summary);
}


static void search_studio_append_document_resultf(const gchar *action, GeanyDocument *doc,
	const gchar *query, const gchar *mode, const gchar *summary_format, ...)
{
	gchar *summary;
	va_list args;

	va_start(args, summary_format);
	summary = g_strdup_vprintf(summary_format, args);
	va_end(args);
	search_studio_append_document_result(action, doc, query, mode, summary);
	g_free(summary);
}


static void search_studio_store_search_data(const gchar *text, const gchar *original_text,
	GeanyFindFlags flags, gboolean backwards)
{
	g_free(search_data.text);
	g_free(search_data.original_text);
	search_data.text = g_strdup(text);
	search_data.original_text = g_strdup(original_text);
	search_data.flags = flags;
	search_data.backwards = backwards;
	search_data.search_bar = FALSE;
}


static const gchar *search_studio_mode_name(GtkWidget *page)
{
	GtkWidget *mode_regex = ui_lookup_widget(page, "mode_regex");
	GtkWidget *mode_extended = ui_lookup_widget(page, "mode_extended");

	if (mode_regex && gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(mode_regex)))
		return "Regex";
	if (mode_extended && gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(mode_extended)))
		return "Extended";
	return "Normal";
}


static void search_studio_show_lower_page(gint page)
{
	if (studio_dlg.lower_notebook != NULL)
		gtk_notebook_set_current_page(GTK_NOTEBOOK(studio_dlg.lower_notebook), page);
}


static gint sort_lexico_asc(gconstpointer a, gconstpointer b) { return strcmp(*(gchar **)a, *(gchar **)b); }
static gint sort_lexico_desc(gconstpointer a, gconstpointer b) { return strcmp(*(gchar **)b, *(gchar **)a); }
static gint sort_lexico_case_asc(gconstpointer a, gconstpointer b) { return g_ascii_strcasecmp(*(gchar **)a, *(gchar **)b); }
static gint sort_lexico_case_desc(gconstpointer a, gconstpointer b) { return g_ascii_strcasecmp(*(gchar **)b, *(gchar **)a); }

static gint sort_length_asc(gconstpointer a, gconstpointer b)
{
	gint la = strlen(*(gchar **)a);
	gint lb = strlen(*(gchar **)b);
	if (la == lb) return strcmp(*(gchar **)a, *(gchar **)b);
	return la - lb;
}

static gint sort_length_desc(gconstpointer a, gconstpointer b)
{
	gint la = strlen(*(gchar **)a);
	gint lb = strlen(*(gchar **)b);
	if (la == lb) return strcmp(*(gchar **)b, *(gchar **)a);
	return lb - la;
}

static gint sort_integer_asc(gconstpointer a, gconstpointer b)
{
	gint64 va = g_ascii_strtoll(*(gchar **)a, NULL, 10);
	gint64 vb = g_ascii_strtoll(*(gchar **)b, NULL, 10);
	return (va < vb) ? -1 : (va > vb);
}

static gint sort_random(gconstpointer a, gconstpointer b)
{
	(void)a; (void)b;
	return g_random_int_range(-1, 2);
}

static void search_studio_sort_activate(GtkButton *button, gpointer user_data)
{
	GeanyDocument *doc = document_get_current();
	const gchar *action_id = user_data;
	ScintillaObject *sci;
	gint start, end;
	gchar *text;
	gchar **lines;
	GCompareFunc func = sort_lexico_asc;

	if (!DOC_VALID(doc) || action_id == NULL)
		return;

	sci = doc->editor->sci;
	if (sci_has_selection(sci))
	{
		start = sci_get_selection_start(sci);
		end = sci_get_selection_end(sci);
	}
	else
	{
		start = 0;
		end = sci_get_length(sci);
	}

	text = sci_get_contents_range(sci, start, end);
	lines = g_strsplit_set(text, "\r\n", -1);
	
	/* Filter out empty trailing string from split if EOL was at the end */
	guint n_lines = g_strv_length(lines);
	GPtrArray *arr = g_ptr_array_new();
	for (guint i = 0; i < n_lines; i++)
		if (lines[i] && *lines[i]) g_ptr_array_add(arr, g_strdup(lines[i]));

	if (g_strcmp0(action_id, "sort-lex-asc") == 0) func = sort_lexico_asc;
	else if (g_strcmp0(action_id, "sort-lex-desc") == 0) func = sort_lexico_desc;
	else if (g_strcmp0(action_id, "sort-lex-case-asc") == 0) func = sort_lexico_case_asc;
	else if (g_strcmp0(action_id, "sort-lex-case-desc") == 0) func = sort_lexico_case_desc;
	else if (g_strcmp0(action_id, "sort-len-asc") == 0) func = sort_length_asc;
	else if (g_strcmp0(action_id, "sort-len-desc") == 0) func = sort_length_desc;
	else if (g_strcmp0(action_id, "sort-int-asc") == 0) func = sort_integer_asc;
	else if (g_strcmp0(action_id, "sort-random") == 0) func = sort_random;

	g_ptr_array_sort(arr, func);

	GString *res = g_string_new("");
	const gchar *eol = editor_get_eol_char(doc->editor);
	for (guint i = 0; i < arr->len; i++)
	{
		g_string_append(res, (gchar *)g_ptr_array_index(arr, i));
		g_string_append(res, eol);
	}

	sci_start_undo_action(sci);
	sci_set_target_start(sci, start);
	sci_set_target_end(sci, end);
	sci_replace_target(sci, res->str, FALSE);
	sci_end_undo_action(sci);

	search_studio_activity_append("[Sort] Applied %s to %s.", action_id, sci_has_selection(sci) ? "selection" : "entire document");
	search_studio_append_document_result("Sort", doc, action_id, "N/A", "Sorted lines in document.");

	g_string_free(res, TRUE);
	g_ptr_array_free(arr, TRUE);
	g_strfreev(lines);
	g_free(text);
}


static GtkWidget *search_studio_create_sort_page(void)
{
	GtkWidget *page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
	GtkWidget *grid = gtk_grid_new();
	GtkWidget *button;
	GtkWidget *label = ui_label_new_bold(_("Advanced Line Sorting"));
	GtkWidget *hint = gtk_label_new(_("Native internal sorting mirroring Notepad++ capabilities. Operates on selection or entire document."));

	gtk_grid_set_column_spacing(GTK_GRID(grid), 12);
	gtk_grid_set_row_spacing(GTK_GRID(grid), 8);
	gtk_container_set_border_width(GTK_CONTAINER(page), 12);

	gtk_box_pack_start(GTK_BOX(page), label, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(page), hint, FALSE, FALSE, 0);

	auto add_sort_btn = [&](const gchar *text, const gchar *id, gint left, gint top) {
		GtkWidget *btn = gtk_button_new_with_mnemonic(text);
		g_signal_connect(btn, "clicked", G_CALLBACK(search_studio_sort_activate), (gpointer)id);
		gtk_grid_attach(GTK_GRID(grid), btn, left, top, 1, 1);
	};

	add_sort_btn(_("Lexicographical _Ascending"), "sort-lex-asc", 0, 0);
	add_sort_btn(_("Lexicographical _Descending"), "sort-lex-desc", 1, 0);
	add_sort_btn(_("Lex. Case _Insensitive Asc."), "sort-lex-case-asc", 0, 1);
	add_sort_btn(_("Lex. Case I_nsensitive Desc."), "sort-lex-case-desc", 1, 1);
	add_sort_btn(_("Sort by _Integer Ascending"), "sort-int-asc", 0, 2);
	add_sort_btn(_("Sort by _Length Ascending"), "sort-len-asc", 1, 2);
	add_sort_btn(_("Sort by L_ength Descending"), "sort-len-desc", 0, 3);
	add_sort_btn(_("Sort _Randomly"), "sort-random", 1, 3);

	gtk_box_pack_start(GTK_BOX(page), grid, FALSE, FALSE, 0);
	return page;
}
{
	GtkTextBuffer *buffer;
	GtkTextIter end;
	gchar *message;
	va_list args;

	if (studio_dlg.activity_view == NULL)
		return;

	buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(studio_dlg.activity_view));
	va_start(args, format);
	message = g_strdup_vprintf(format, args);
	va_end(args);

	gtk_text_buffer_get_end_iter(buffer, &end);
	if (gtk_text_buffer_get_char_count(buffer) > 0)
		gtk_text_buffer_insert(buffer, &end, "\n", -1);
	gtk_text_buffer_insert(buffer, &end, message, -1);
	gtk_text_view_scroll_mark_onscreen(GTK_TEXT_VIEW(studio_dlg.activity_view),
		gtk_text_buffer_get_insert(buffer));
	g_free(message);
}


static void search_studio_set_preview(const gchar *title, const gchar *body)
{
	GtkTextBuffer *buffer;
	gchar *text;

	if (studio_dlg.preview_view == NULL)
		return;

	buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(studio_dlg.preview_view));
	text = g_strdup_printf("%s\n\n%s",
		!EMPTY(title) ? title : _("Preview"),
		!EMPTY(body) ? body : _("Select a result row to inspect its preview details."));
	gtk_text_buffer_set_text(buffer, text, -1);
	g_free(text);
}


static void search_studio_result_append_spec(const SearchStudioResultSpec *spec)
{
	GtkTreeIter iter;
	gchar *fallback_preview = NULL;
	const gchar *preview_title;
	const gchar *preview_body;

	if (studio_dlg.results_store == NULL || spec == NULL)
		return;

	fallback_preview = g_strdup_printf("Action: %s\nTarget: %s\nQuery: %s\nMode: %s\n\nSummary:\n%s",
		spec->action != NULL ? spec->action : _("(none)"),
		spec->target != NULL ? spec->target : _("(none)"),
		spec->query != NULL ? spec->query : _("(none)"),
		spec->mode != NULL ? spec->mode : _("(none)"),
		spec->summary != NULL ? spec->summary : _("(none)"));
	preview_title = !EMPTY(spec->preview_title) ? spec->preview_title : spec->target;
	preview_body = !EMPTY(spec->preview_body) ? spec->preview_body : fallback_preview;

	gtk_list_store_prepend(studio_dlg.results_store, &iter);
	gtk_list_store_set(studio_dlg.results_store, &iter,
		STUDIO_RESULT_ACTION, spec->action,
		STUDIO_RESULT_TARGET, spec->target,
		STUDIO_RESULT_QUERY, spec->query,
		STUDIO_RESULT_MODE, spec->mode,
		STUDIO_RESULT_SUMMARY, spec->summary,
		STUDIO_RESULT_FILE, spec->filename,
		STUDIO_RESULT_LINE, spec->line,
		STUDIO_RESULT_POS, spec->pos,
		STUDIO_RESULT_CAN_NAVIGATE, spec->can_navigate,
		STUDIO_RESULT_PREVIEW_TITLE, preview_title,
		STUDIO_RESULT_PREVIEW_BODY, preview_body,
		-1);
	search_studio_show_lower_page(STUDIO_LOWER_PAGE_RESULTS);
	g_free(fallback_preview);
}


static void search_studio_result_append(const gchar *action, const gchar *target,
	const gchar *query, const gchar *mode, const gchar *summary)
{
	SearchStudioResultSpec spec = { 0 };

	spec.action = action;
	spec.target = target;
	spec.query = query;
	spec.mode = mode;
	spec.summary = summary;
	spec.line = -1;
	spec.pos = -1;
	search_studio_result_append_spec(&spec);
}


static gchar *search_studio_build_line_preview_body(GeanyDocument *doc, gint pos,
	const gchar *query, const gchar *kind_label)
{
	gint line_no;
	gint line_start;
	gint rel_start;
	gchar *line_text;
	gchar *before;
	gchar *after;
	gchar *body;

	if (!DOC_VALID(doc) || pos < 0)
		return g_strdup(_("No preview available."));

	line_no = sci_get_line_from_position(doc->editor->sci, pos);
	line_start = sci_get_position_from_line(doc->editor->sci, line_no);
	line_text = sci_get_line(doc->editor->sci, line_no);
	g_strchomp(line_text);
	rel_start = MAX(0, pos - line_start);
	before = g_strndup(line_text, rel_start);
	after = g_strdup(line_text + rel_start);
	body = g_strdup_printf("%s\n\nFile: %s\nLine: %d\nQuery: %s\n\nContext:\n%s\n>>> %s",
		kind_label != NULL ? kind_label : _("Result Preview"),
		DOC_FILENAME(doc),
		line_no + 1,
		query != NULL ? query : _("(none)"),
		before,
		after);
	g_free(after);
	g_free(before);
	g_free(line_text);
	return body;
}


static void search_studio_result_append_match(const gchar *action, GeanyDocument *doc,
	const gchar *query, const gchar *mode, gint pos, const gchar *summary)
{
	SearchStudioResultSpec spec = { 0 };
	gchar *target;
	gchar *preview_body;
	gint line;

	if (studio_dlg.results_store == NULL || !DOC_VALID(doc))
		return;

	line = sci_get_line_from_position(doc->editor->sci, pos) + 1;
	target = g_strdup_printf("%s:%d", DOC_FILENAME(doc), line);
	preview_body = search_studio_build_line_preview_body(doc, pos, query, action);

	spec.action = action;
	spec.target = target;
	spec.query = query;
	spec.mode = mode;
	spec.summary = summary;
	spec.filename = DOC_FILENAME(doc);
	spec.line = line;
	spec.pos = pos;
	spec.can_navigate = TRUE;
	spec.preview_title = target;
	spec.preview_body = preview_body;
	search_studio_result_append_spec(&spec);
	g_free(preview_body);
	g_free(target);
}


static void search_studio_result_append_preview_match(const gchar *action, GeanyDocument *doc,
	const gchar *query, const gchar *mode, gint pos, const gchar *summary,
	const gchar *preview_title, const gchar *preview_body)
{
	SearchStudioResultSpec spec = { 0 };
	gchar *target;
	gint line;

	if (studio_dlg.results_store == NULL || !DOC_VALID(doc))
		return;

	line = sci_get_line_from_position(doc->editor->sci, pos) + 1;
	target = g_strdup_printf("%s:%d", DOC_FILENAME(doc), line);

	spec.action = action;
	spec.target = target;
	spec.query = query;
	spec.mode = mode;
	spec.summary = summary;
	spec.filename = DOC_FILENAME(doc);
	spec.line = line;
	spec.pos = pos;
	spec.can_navigate = TRUE;
	spec.preview_title = preview_title;
	spec.preview_body = preview_body;
	search_studio_result_append_spec(&spec);
	g_free(target);
}


static guint search_studio_append_match_rows(const gchar *action, GeanyDocument *doc,
	const gchar *query, GeanyFindFlags flags, const gchar *mode, guint limit)
{
	struct Sci_TextToFind ttf;
	GSList *match, *matches;
	guint count = 0;

	if (!DOC_VALID(doc) || EMPTY(query))
		return 0;

	ttf.chrg.cpMin = 0;
	ttf.chrg.cpMax = sci_get_length(doc->editor->sci);
	ttf.lpstrText = (gchar *) query;
	matches = find_range(doc->editor->sci, flags, &ttf);
	foreach_slist(match, matches)
	{
		GeanyMatchInfo *info = match->data;

		if (count < limit)
		{
			gchar *line_text = sci_get_line(doc->editor->sci,
				sci_get_line_from_position(doc->editor->sci, info->start));
			gchar *summary;

			g_strstrip(line_text);
			summary = g_strdup_printf("Match at line %d: %s",
				sci_get_line_from_position(doc->editor->sci, info->start) + 1, line_text);
			search_studio_result_append_match(action, doc, query, mode, info->start, summary);
			g_free(summary);
			g_free(line_text);
		}
		count++;
		geany_match_info_free(info);
	}
	g_slist_free(matches);
	return count;
}


static guint search_studio_visit_open_documents(SearchStudioOpenDocumentFunc func,
	gpointer user_data, guint *docs_with_results)
{
	guint page_count;
	guint n;
	guint total = 0;
	guint docs = 0;

	g_return_val_if_fail(func != NULL, 0);

	page_count = gtk_notebook_get_n_pages(GTK_NOTEBOOK(main_widgets.notebook));
	for (n = 0; n < page_count; n++)
	{
		GeanyDocument *doc = document_get_from_page(n);
		guint count;

		if (!DOC_VALID(doc))
			continue;
		count = func(doc, user_data);
		if (count > 0)
		{
			total += count;
			docs++;
		}
	}
	if (docs_with_results != NULL)
		*docs_with_results = docs;
	return total;
}


static SearchStudioSessionRunResult search_studio_run_session_action(
	SearchStudioOpenDocumentFunc func, gpointer user_data)
{
	SearchStudioSessionRunResult result = { 0, 0 };

	result.total_results = search_studio_visit_open_documents(func, user_data,
		&result.docs_with_results);
	return result;
}


static SearchStudioSessionRunResult search_studio_execute_find_session_action(GtkWidget *page,
	const SearchStudioFindSessionActionSpec *action)
{
	SearchStudioSessionRunResult result = { 0, 0 };

	if (action == NULL || action->find == NULL || action->open_document_func == NULL)
		return result;
	if (action->add_history)
		search_studio_add_find_history(page, action->find);
	result = search_studio_run_session_action(action->open_document_func,
		action->open_document_data);
	if (action->store_find_spec)
		search_studio_store_find_spec(action->find);
	return result;
}


static SearchStudioSessionRunResult search_studio_execute_replace_session_action(GtkWidget *page,
	const SearchStudioReplaceSessionActionSpec *action)
{
	SearchStudioSessionRunResult result = { 0, 0 };

	if (action == NULL || action->replace == NULL)
		return result;
	if (action->add_history)
		search_studio_add_replace_history(page, action->replace);
	if (action->open_document_func != NULL)
		result = search_studio_run_session_action(action->open_document_func,
			action->open_document_data);
	if (action->store_find_spec)
		search_studio_store_find_spec(&action->replace->find);
	return result;
}


typedef struct SearchStudioSessionMatchRowsContext
{
	const gchar *action;
	const gchar *query;
	GeanyFindFlags flags;
	const gchar *mode;
	guint per_doc_limit;
}
SearchStudioSessionMatchRowsContext;


static guint search_studio_append_session_match_rows_cb(GeanyDocument *doc, gpointer user_data)
{
	SearchStudioSessionMatchRowsContext *ctx = user_data;

	return search_studio_append_match_rows(ctx->action, doc, ctx->query, ctx->flags,
		ctx->mode, ctx->per_doc_limit);
}


static guint search_studio_append_session_match_rows(const gchar *action,
	const gchar *query, GeanyFindFlags flags, const gchar *mode, guint per_doc_limit)
{
	SearchStudioSessionMatchRowsContext ctx = { action, query, flags, mode, per_doc_limit };
	SearchStudioSessionRunResult result = search_studio_run_session_action(
		search_studio_append_session_match_rows_cb, &ctx);

	return result.total_results;
}


static gchar *search_build_replacement_text(const GeanyMatchInfo *match, const gchar *replace_text)
{
	GString *str;
	gint i = 0;

	g_return_val_if_fail(match != NULL && replace_text != NULL, g_strdup(""));

	if (!(match->flags & GEANY_FIND_REGEXP) || match->match_text == NULL)
		return g_strdup(replace_text);

	str = g_string_new(replace_text);
	while (str->str[i])
	{
		gchar *ptr = &str->str[i];
		gchar *grp;
		gchar c;

		if (ptr[0] != '\\')
		{
			i++;
			continue;
		}
		c = ptr[1];
		if (c == '\\' || !isdigit(c))
		{
			g_string_erase(str, i, 1);
			i++;
			continue;
		}
		g_string_erase(str, i, 2);
		grp = get_regex_match_string(match->match_text - match->matches[0].start, match, c - '0');
		g_string_insert(str, i, grp);
		i += strlen(grp);
		g_free(grp);
	}
	return g_string_free(str, FALSE);
}


static gchar *search_studio_build_replace_preview_body(GeanyDocument *doc,
	const GeanyMatchInfo *info, const gchar *replace_text, const gchar *replace_display)
{
	gint line_no;
	gint line_start;
	gint rel_start;
	gint rel_end;
	gchar *line_text;
	gchar *before;
	gchar *match_text;
	gchar *after_tail;
	gchar *replacement_text;
	gchar *escaped_payload;
	gchar *escaped_replacement;
	gchar *after_line;
	gchar *body;

	if (!DOC_VALID(doc) || info == NULL || replace_text == NULL)
		return g_strdup(_("No preview available."));

	line_no = sci_get_line_from_position(doc->editor->sci, info->start);
	line_start = sci_get_position_from_line(doc->editor->sci, line_no);
	line_text = sci_get_line(doc->editor->sci, line_no);
	g_strchomp(line_text);

	rel_start = MAX(0, info->start - line_start);
	rel_end = MAX(rel_start, info->end - line_start);
	before = g_strndup(line_text, rel_start);
	match_text = g_strndup(line_text + rel_start, rel_end - rel_start);
	after_tail = g_strdup(line_text + rel_end);
	replacement_text = search_build_replacement_text(info, replace_text);
	escaped_payload = g_strescape(replace_display != NULL ? replace_display : replace_text, NULL);
	escaped_replacement = g_strescape(replacement_text, NULL);
	after_line = g_strconcat(before, replacement_text, after_tail, NULL);
	body = g_strdup_printf("Original line:\n- %s\n\nReplacement line:\n+ %s\n\nMatched segment diff:\n- %s\n+ %s\n\nPayload entered:\n%s\n\nActual replacement text:\n%s",
		line_text,
		after_line,
		match_text,
		escaped_replacement != NULL ? escaped_replacement : replacement_text,
		escaped_payload != NULL ? escaped_payload : (replace_display != NULL ? replace_display : replace_text),
		escaped_replacement != NULL ? escaped_replacement : replacement_text);

	g_free(after_line);
	g_free(escaped_replacement);
	g_free(escaped_payload);
	g_free(replacement_text);
	g_free(after_tail);
	g_free(match_text);
	g_free(before);
	g_free(line_text);
	return body;
}


static guint search_studio_append_replace_preview_rows(GeanyDocument *doc, const gchar *query,
	GeanyFindFlags flags, const gchar *mode, const gchar *replace_text,
	const gchar *replace_display, guint limit)
{
	struct Sci_TextToFind ttf;
	GSList *match, *matches;
	guint count = 0;

	if (!DOC_VALID(doc) || EMPTY(query) || replace_text == NULL)
		return 0;

	ttf.chrg.cpMin = 0;
	ttf.chrg.cpMax = sci_get_length(doc->editor->sci);
	ttf.lpstrText = (gchar *) query;
	matches = find_range(doc->editor->sci, flags, &ttf);
	foreach_slist(match, matches)
	{
		GeanyMatchInfo *info = match->data;

		if (count < limit)
		{
			gchar *line_text = sci_get_line(doc->editor->sci,
				sci_get_line_from_position(doc->editor->sci, info->start));
			gchar *replacement_text = search_build_replacement_text(info, replace_text);
			gchar *escaped_replacement = g_strescape(replacement_text, NULL);
			gchar *summary;
			gchar *preview_title;
			gchar *preview_body;

			g_strstrip(line_text);
			summary = g_strdup_printf("Would replace line %d match with: %s | context: %s",
				sci_get_line_from_position(doc->editor->sci, info->start) + 1,
				escaped_replacement != NULL ? escaped_replacement : replacement_text,
				line_text);
			preview_title = g_strdup_printf("Replace Preview — %s:%d",
				DOC_FILENAME(doc), sci_get_line_from_position(doc->editor->sci, info->start) + 1);
			preview_body = search_studio_build_replace_preview_body(doc, info, replace_text, replace_display);
			search_studio_result_append_preview_match("Replace Preview", doc, query, mode,
				info->start, summary, preview_title, preview_body);
			g_free(preview_body);
			g_free(preview_title);
			g_free(summary);
			g_free(escaped_replacement);
			g_free(replacement_text);
			g_free(line_text);
		}
		count++;
		geany_match_info_free(info);
	}
	g_slist_free(matches);
	return count;
}


static guint search_studio_append_replace_impact_rows(const gchar *action, GeanyDocument *doc,
	const gchar *query, GeanyFindFlags flags, const gchar *mode, const gchar *replace_text,
	const gchar *replace_display)
{
	struct Sci_TextToFind ttf;
	GSList *match, *matches;
	guint count = 0;
	gint first_start = -1;
	gint first_end = -1;

	if (!DOC_VALID(doc) || EMPTY(query) || replace_text == NULL)
		return 0;

	ttf.chrg.cpMin = 0;
	ttf.chrg.cpMax = sci_get_length(doc->editor->sci);
	ttf.lpstrText = (gchar *) query;
	matches = find_range(doc->editor->sci, flags, &ttf);
	foreach_slist(match, matches)
	{
		GeanyMatchInfo *info = match->data;

		if (count == 0)
		{
			first_start = info->start;
			first_end = info->end;
		}
		count++;
		geany_match_info_free(info);
	}
	g_slist_free(matches);

	if (count > 0 && first_start >= 0)
	{
		GeanyMatchInfo info = { 0 };
		gint line = sci_get_line_from_position(doc->editor->sci, first_start) + 1;
		gchar *summary;
		gchar *preview_title;
		gchar *payload_preview;
		gchar *preview_body;

		info.start = first_start;
		info.end = first_end;
		summary = g_strdup_printf("Affected %u matches in this document; first affected line %d.", count, line);
		preview_title = g_strdup_printf("%s — %s:%d", action, DOC_FILENAME(doc), line);
		payload_preview = search_studio_build_replace_preview_body(doc, &info, replace_text, replace_display);
		preview_body = g_strdup_printf("Document impact summary\n\nFile: %s\nQuery: %s\nMode: %s\nReplacement payload: %s\nAffected matches: %u\nFirst affected line: %d\n\n%s",
			DOC_FILENAME(doc),
			query,
			mode != NULL ? mode : _("(none)"),
			replace_display != NULL ? replace_display : (replace_text != NULL ? replace_text : _("(none)")),
			count,
			line,
			payload_preview);
		search_studio_result_append_preview_match(action, doc, query, mode, first_start,
			summary, preview_title, preview_body);
		g_free(preview_body);
		g_free(payload_preview);
		g_free(preview_title);
		g_free(summary);
	}

	return count;
}


static guint search_studio_replace_session_rows_action_cb(GeanyDocument *doc,
	gpointer user_data)
{
	SearchStudioReplaceSessionRowsActionContext *ctx = user_data;

	if (ctx->kind == SEARCH_STUDIO_REPLACE_SESSION_ROWS_PREVIEW)
		return search_studio_append_replace_preview_rows(doc, ctx->query, ctx->flags,
			ctx->mode, ctx->replace_text, ctx->replace_display, ctx->per_doc_limit);
	return search_studio_append_replace_impact_rows(ctx->action, doc, ctx->query,
		ctx->flags, ctx->mode, ctx->replace_text, ctx->replace_display);
}


static SearchStudioSessionRunResult search_studio_execute_replace_session_rows_action(
	GtkWidget *page, const SearchStudioReplaceSpec *spec,
	SearchStudioReplaceSessionRowsActionContext *ctx, gboolean add_history,
	gboolean store_find_spec)
{
	SearchStudioReplaceSessionActionSpec action = { 0 };

	action.replace = spec;
	action.open_document_func = search_studio_replace_session_rows_action_cb;
	action.open_document_data = ctx;
	action.add_history = add_history;
	action.store_find_spec = store_find_spec;
	return search_studio_execute_replace_session_action(page, &action);
}


static SearchStudioReplacePlanResult search_studio_plan_replace_document(
	GeanyDocument *doc, const SearchStudioReplaceSpec *spec)
{
	SearchStudioReplacePlanResult result = { 0, 0 };

	if (spec == NULL)
		return result;
	result.planned_matches = search_studio_append_replace_impact_rows("Replace Impact", doc,
		spec->find.text, spec->find.flags, spec->find.mode, spec->replace,
		spec->original_replace);
	result.planned_docs = result.planned_matches > 0 ? 1 : 0;
	return result;
}


static SearchStudioReplacePlanResult search_studio_plan_replace_session(GtkWidget *page,
	const SearchStudioReplaceSpec *spec)
{
	SearchStudioReplaceSessionRowsActionContext ctx;
	SearchStudioSessionRunResult session_result;
	SearchStudioReplacePlanResult result = { 0, 0 };

	if (spec == NULL)
		return result;

	ctx.kind = SEARCH_STUDIO_REPLACE_SESSION_ROWS_IMPACT;
	ctx.action = "Session Replace Impact";
	ctx.query = spec->find.text;
	ctx.flags = spec->find.flags;
	ctx.mode = spec->find.mode;
	ctx.replace_text = spec->replace;
	ctx.replace_display = spec->original_replace;
	ctx.per_doc_limit = 0;
	session_result = search_studio_execute_replace_session_rows_action(page, spec, &ctx,
		FALSE, FALSE);
	result.planned_matches = session_result.total_results;
	result.planned_docs = session_result.docs_with_results;
	return result;
}


static SearchStudioReplaceExecutionResult search_studio_execute_replace_document(
	GeanyDocument *doc, const SearchStudioReplaceSpec *spec)
{
	SearchStudioReplaceExecutionResult result = { { 0, 0 }, 0, 0 };

	if (spec == NULL)
		return result;

	result.plan = search_studio_plan_replace_document(doc, spec);
	result.applied_matches = document_replace_all(doc, spec->find.text, spec->replace,
		spec->find.original_text, spec->original_replace, spec->find.flags);
	result.affected_docs = result.applied_matches > 0 ? 1 : 0;
	return result;
}


static SearchStudioReplaceExecutionResult search_studio_execute_replace_session(
	GtkWidget *page, GeanyDocument *doc, const SearchStudioReplaceSpec *spec)
{
	SearchStudioReplaceExecutionResult result = { { 0, 0 }, 0, 0 };
	guint n, page_count;

	if (spec == NULL)
		return result;

	result.plan = search_studio_plan_replace_session(page, spec);
	page_count = gtk_notebook_get_n_pages(GTK_NOTEBOOK(main_widgets.notebook));
	for (n = 0; n < page_count; n++)
	{
		GeanyDocument *tmp_doc = document_get_from_page(n);
		gint reps = document_replace_all(tmp_doc, spec->find.text, spec->replace,
			spec->find.original_text, spec->original_replace, spec->find.flags);

		result.applied_matches += reps;
		if (reps)
			result.affected_docs++;
	}
	if (result.affected_docs == 0)
	{
		utils_beep();
		ui_set_statusbar(FALSE, _("No matches found for \"%s\"."), spec->find.original_text);
		return result;
	}
	if (result.affected_docs > 1)
		ui_set_statusbar(FALSE, _("Replaced %u matches in %u documents."),
			result.applied_matches, result.affected_docs);
	gtk_notebook_set_current_page(GTK_NOTEBOOK(msgwindow.notebook), MSG_STATUS);
	ui_save_buttons_toggle(doc->changed);
	return result;
}


static void search_studio_capture_begin(const gchar *query, const gchar *mode, const gchar *dir)
{
	studio_fif_capture.active = TRUE;
	studio_fif_capture.results_added = 0;
	SETPTR(studio_fif_capture.query, g_strdup(query));
	SETPTR(studio_fif_capture.mode, g_strdup(mode));
	SETPTR(studio_fif_capture.dir, g_strdup(dir));
}


static void search_studio_capture_finish(gint exit_status)
{
	if (!studio_fif_capture.active)
		return;

	search_studio_activity_append("[Find in Files] Capture finished | query=%s | mode=%s | rows=%u | exit=%d",
		EMPTY(studio_fif_capture.query) ? "(empty)" : studio_fif_capture.query,
		EMPTY(studio_fif_capture.mode) ? "N/A" : studio_fif_capture.mode,
		studio_fif_capture.results_added, exit_status);
	{
		gchar *summary = g_strdup_printf("Captured %u structured Find in Files rows (exit=%d).",
			studio_fif_capture.results_added, exit_status);
		search_studio_result_append("Find in Files Capture",
			EMPTY(studio_fif_capture.dir) ? "Search Studio" : studio_fif_capture.dir,
			EMPTY(studio_fif_capture.query) ? "(empty)" : studio_fif_capture.query,
			EMPTY(studio_fif_capture.mode) ? "N/A" : studio_fif_capture.mode,
			summary);
		g_free(summary);
	}

	studio_fif_capture.active = FALSE;
}


static void search_studio_capture_grep_result(const gchar *utf8_msg)
{
	gchar **parts;
	gchar *filename;
	gchar *full_path;
	gchar *target;
	gchar *summary;
	gint line;

	if (!studio_fif_capture.active || EMPTY(utf8_msg) || studio_dlg.results_store == NULL)
		return;

	parts = g_strsplit(utf8_msg, ":", 3);
	if (parts[0] == NULL || parts[1] == NULL || parts[2] == NULL)
	{
		g_strfreev(parts);
		return;
	}

	line = atoi(parts[1]);
	if (line <= 0)
	{
		g_strfreev(parts);
		return;
	}

	filename = g_strstrip(parts[0]);
	full_path = g_canonicalize_filename(filename,
		EMPTY(studio_fif_capture.dir) ? "." : studio_fif_capture.dir);
	target = g_strdup_printf("%s:%d", full_path, line);
	summary = g_strdup_printf("Find in Files hit at line %d: %s", line, g_strstrip(parts[2]));

	{
		SearchStudioResultSpec spec = { 0 };
		gchar *preview_body = g_strdup_printf("Find in Files Hit\n\nFile: %s\nLine: %d\nQuery: %s\nMode: %s\n\nMatched line:\n%s",
			full_path,
			line,
			studio_fif_capture.query != NULL ? studio_fif_capture.query : _("(none)"),
			studio_fif_capture.mode != NULL ? studio_fif_capture.mode : _("(none)"),
			g_strstrip(parts[2]));
		spec.action = "Find in Files Hit";
		spec.target = target;
		spec.query = studio_fif_capture.query;
		spec.mode = studio_fif_capture.mode;
		spec.summary = summary;
		spec.filename = full_path;
		spec.line = line;
		spec.pos = -1;
		spec.can_navigate = TRUE;
		spec.preview_title = target;
		spec.preview_body = preview_body;
		search_studio_result_append_spec(&spec);
		g_free(preview_body);
	}
	studio_fif_capture.results_added++;
	g_free(summary);
	g_free(target);
	g_free(full_path);
	g_strfreev(parts);
}


static void search_studio_clear_results(GtkButton *button, gpointer user_data)
{
	if (studio_dlg.results_store != NULL)
		gtk_list_store_clear(studio_dlg.results_store);
	search_studio_set_preview(_("Diff Preview"), _("Select a result row to inspect before/after or result details."));
	search_studio_show_lower_page(STUDIO_LOWER_PAGE_RESULTS);
	search_studio_activity_append("[Results] Cleared structured results pane.");
	search_studio_result_append("Results", "Search Studio", "Clear", "N/A",
		"Cleared structured results rows.");
}


static void search_studio_collect_document_hits(GtkButton *button, gpointer user_data)
{
	GtkWidget *page = GTK_WIDGET(user_data);
	GeanyDocument *doc = document_get_current();
	SearchStudioFindSpec spec = { 0 };
	guint count;

	if (!DOC_VALID(doc) || !search_studio_build_find_spec(page, "entry_search", &spec))
		return;

	count = search_studio_append_match_rows("Document Hit", doc, spec.text, spec.flags,
		spec.mode, 250);
	search_studio_activity_append("[Results] Collected %u current-document hits for %s.", count, spec.original_text);
	{
		gchar *target = search_studio_document_target(doc);
		search_studio_result_appendf("Collect Hits", target, spec.original_text, spec.mode,
			"Collected %u navigable hits from the active document.", count);
		g_free(target);
	}
	search_studio_find_spec_clear(&spec);
}


static void search_studio_collect_session_hits(GtkButton *button, gpointer user_data)
{
	GtkWidget *page = GTK_WIDGET(user_data);
	SearchStudioFindSpec spec = { 0 };
	SearchStudioSessionMatchRowsContext ctx;
	SearchStudioFindSessionActionSpec action;
	SearchStudioSessionRunResult result;

	if (!search_studio_build_find_spec(page, "entry_search", &spec))
		return;

	ctx.action = "Session Hit";
	ctx.query = spec.text;
	ctx.flags = spec.flags;
	ctx.mode = spec.mode;
	ctx.per_doc_limit = 100;
	action.find = &spec;
	action.open_document_func = search_studio_append_session_match_rows_cb;
	action.open_document_data = &ctx;
	action.add_history = FALSE;
	action.store_find_spec = FALSE;
	result = search_studio_execute_find_session_action(page, &action);
	search_studio_activity_append("[Results] Collected %u open-document hits for %s.",
		result.total_results, spec.original_text);
	search_studio_result_appendf("Collect Session Hits", "Open Documents", spec.original_text,
		spec.mode, "Collected %u navigable hits across open documents.", result.total_results);
	search_studio_find_spec_clear(&spec);
}


static void search_studio_stored_result_row_free(SearchStudioStoredResultRow *row)
{
	if (row == NULL)
		return;

	g_free(row->action);
	g_free(row->target);
	g_free(row->query);
	g_free(row->mode);
	g_free(row->summary);
	g_free(row->filename);
	g_free(row->preview_title);
	g_free(row->preview_body);
	g_free(row);
}


static GRegex *search_studio_compile_results_regex(const gchar *text, GeanyFindFlags flags)
{
	gchar *pattern;
	GRegex *regex;
	GError *error = NULL;
	gint rflags = 0;

	if (flags & GEANY_FIND_REGEXP)
		return compile_regex(text, flags);

	pattern = g_regex_escape_string(text, -1);
	if (flags & GEANY_FIND_WHOLEWORD)
	{
		gchar *tmp = g_strdup_printf("\\b%s\\b", pattern);
		g_free(pattern);
		pattern = tmp;
	}
	else if (flags & GEANY_FIND_WORDSTART)
	{
		gchar *tmp = g_strdup_printf("\\b%s", pattern);
		g_free(pattern);
		pattern = tmp;
	}

	if (flags & GEANY_FIND_MULTILINE)
		rflags |= G_REGEX_MULTILINE;
	if (flags & GEANY_FIND_DOTALL)
		rflags |= G_REGEX_DOTALL;
	if (~flags & GEANY_FIND_MATCHCASE)
		rflags |= G_REGEX_CASELESS;

	regex = g_regex_new(pattern, rflags, 0, &error);
	if (regex == NULL)
	{
		ui_set_statusbar(FALSE, _("Bad regex: %s"), error->message);
		g_error_free(error);
	}
	g_free(pattern);
	return regex;
}


static void search_studio_find_in_results_activate(GtkButton *button, gpointer user_data)
{
	GtkWidget *page = GTK_WIDGET(user_data);
	SearchStudioFindSpec spec = { 0 };
	GtkTreeModel *model;
	GtkTreeIter iter;
	GPtrArray *hits;
	GRegex *regex;
	guint count = 0;

	if (!search_studio_build_find_spec(page, "entry_search", &spec) || studio_dlg.results_store == NULL)
		return;

	regex = search_studio_compile_results_regex(spec.text, spec.flags);
	if (regex == NULL)
	{
		search_studio_find_spec_clear(&spec);
		return;
	}

	hits = g_ptr_array_new_with_free_func((GDestroyNotify) search_studio_stored_result_row_free);
	model = GTK_TREE_MODEL(studio_dlg.results_store);
	if (gtk_tree_model_get_iter_first(model, &iter))
	{
		do
		{
			SearchStudioStoredResultRow *row;
			gchar *haystack;
			gchar *action = NULL;
			gchar *target = NULL;
			gchar *query = NULL;
			gchar *mode = NULL;
			gchar *summary = NULL;
			gchar *filename = NULL;
			gchar *preview_title = NULL;
			gchar *preview_body = NULL;
			gint line = -1;
			gint pos = -1;
			gboolean can_navigate = FALSE;

			gtk_tree_model_get(model, &iter,
				STUDIO_RESULT_ACTION, &action,
				STUDIO_RESULT_TARGET, &target,
				STUDIO_RESULT_QUERY, &query,
				STUDIO_RESULT_MODE, &mode,
				STUDIO_RESULT_SUMMARY, &summary,
				STUDIO_RESULT_FILE, &filename,
				STUDIO_RESULT_LINE, &line,
				STUDIO_RESULT_POS, &pos,
				STUDIO_RESULT_CAN_NAVIGATE, &can_navigate,
				STUDIO_RESULT_PREVIEW_TITLE, &preview_title,
				STUDIO_RESULT_PREVIEW_BODY, &preview_body,
				-1);

			haystack = g_strconcat(action != NULL ? action : "",
				"\n", target != NULL ? target : "",
				"\n", query != NULL ? query : "",
				"\n", mode != NULL ? mode : "",
				"\n", summary != NULL ? summary : "",
				"\n", preview_body != NULL ? preview_body : "", NULL);
			if (g_regex_match(regex, haystack, 0, NULL))
			{
				row = g_new0(SearchStudioStoredResultRow, 1);
				row->action = action;
				row->target = target;
				row->query = query;
				row->mode = mode;
				row->summary = summary;
				row->filename = filename;
				row->line = line;
				row->pos = pos;
				row->can_navigate = can_navigate;
				row->preview_title = preview_title;
				row->preview_body = preview_body;
				g_ptr_array_add(hits, row);
			}
			else
			{
				g_free(action);
				g_free(target);
				g_free(query);
				g_free(mode);
				g_free(summary);
				g_free(filename);
				g_free(preview_title);
				g_free(preview_body);
			}
			g_free(haystack);
		}
		while (gtk_tree_model_iter_next(model, &iter));
	}

	for (guint i = 0; i < hits->len; i++)
	{
		SearchStudioStoredResultRow *row = g_ptr_array_index(hits, i);
		SearchStudioResultSpec result_spec = { 0 };
		gchar *preview_body = g_strdup_printf("Find in Results Hit\n\nOriginal action: %s\nOriginal target: %s\nOriginal query: %s\nOriginal mode: %s\n\nOriginal summary:\n%s\n\nOriginal preview:\n%s",
			row->action != NULL ? row->action : _("(none)"),
			row->target != NULL ? row->target : _("(none)"),
			row->query != NULL ? row->query : _("(none)"),
			row->mode != NULL ? row->mode : _("(none)"),
			row->summary != NULL ? row->summary : _("(none)"),
			row->preview_body != NULL ? row->preview_body : _("(none)"));
		gchar *summary = g_strdup_printf("Matched existing result row: %s", row->action != NULL ? row->action : _("Result"));
		gchar *preview_title = g_strdup_printf("Find in Results — %s", row->action != NULL ? row->action : _("Result"));

		result_spec.action = "Find in Results Hit";
		result_spec.target = row->target != NULL ? row->target : "Search Studio Results";
		result_spec.query = spec.original_text;
		result_spec.mode = spec.mode;
		result_spec.summary = summary;
		result_spec.filename = row->filename;
		result_spec.line = row->line;
		result_spec.pos = row->pos;
		result_spec.can_navigate = row->can_navigate;
		result_spec.preview_title = preview_title;
		result_spec.preview_body = preview_body;
		search_studio_result_append_spec(&result_spec);
		g_free(preview_body);
		g_free(preview_title);
		g_free(summary);
		count++;
	}

	search_studio_activity_append("[Results] Find in Results | query=%s | mode=%s | hits=%u",
		spec.original_text, spec.mode, count);
	search_studio_result_appendf("Find in Results", "Search Studio Results", spec.original_text,
		spec.mode, "Matched %u existing Search Studio result rows.", count);
	if (count == 0)
		utils_beep();

	g_ptr_array_free(hits, TRUE);
	g_regex_unref(regex);
	search_studio_store_find_spec(&spec);
	search_studio_add_find_history(page, &spec);
	search_studio_find_spec_clear(&spec);
}


static guint search_studio_append_count_impact_row(const gchar *action, GeanyDocument *doc,
	const gchar *query, GeanyFindFlags flags, const gchar *mode)
{
	gint count;
	struct Sci_TextToFind ttf;
	GSList *match, *matches;
	gint first_start = -1;

	if (!DOC_VALID(doc) || EMPTY(query))
		return 0;

	count = search_count_matches(doc, query, flags);
	if (count <= 0)
		return 0;

	ttf.chrg.cpMin = 0;
	ttf.chrg.cpMax = sci_get_length(doc->editor->sci);
	ttf.lpstrText = (gchar *) query;
	matches = find_range(doc->editor->sci, flags, &ttf);
	foreach_slist(match, matches)
	{
		GeanyMatchInfo *info = match->data;

		if (first_start < 0)
			first_start = info->start;
		geany_match_info_free(info);
	}
	g_slist_free(matches);

	if (first_start >= 0)
	{
		gint line = sci_get_line_from_position(doc->editor->sci, first_start) + 1;
		gchar *summary = g_strdup_printf("Counted %d matches in this document; first matching line %d.",
			count, line);
		gchar *preview_title = g_strdup_printf("%s — %s:%d", action, DOC_FILENAME(doc), line);
		gchar *line_preview = search_studio_build_line_preview_body(doc, first_start, query, action);
		gchar *preview_body = g_strdup_printf("Count impact summary\n\nFile: %s\nQuery: %s\nMode: %s\nCounted matches: %d\nFirst matching line: %d\n\n%s",
			DOC_FILENAME(doc),
			query,
			mode != NULL ? mode : _("(none)"),
			count,
			line,
			line_preview);
		search_studio_result_append_preview_match(action, doc, query, mode, first_start,
			summary, preview_title, preview_body);
		g_free(preview_body);
		g_free(line_preview);
		g_free(preview_title);
		g_free(summary);
	}

	return count;
}


static guint search_studio_append_mark_impact_row(const gchar *action, GeanyDocument *doc,
	const gchar *query, GeanyFindFlags flags, const gchar *mode,
	gboolean bookmark_lines, gboolean purge_bookmarks)
{
	struct Sci_TextToFind ttf;
	GSList *match, *matches;
	guint count = 0;
	gint first_start = -1;

	if (!DOC_VALID(doc) || EMPTY(query))
		return 0;

	ttf.chrg.cpMin = 0;
	ttf.chrg.cpMax = sci_get_length(doc->editor->sci);
	ttf.lpstrText = (gchar *) query;
	matches = find_range(doc->editor->sci, flags, &ttf);
	foreach_slist(match, matches)
	{
		GeanyMatchInfo *info = match->data;

		if (count == 0)
			first_start = info->start;
		count++;
		geany_match_info_free(info);
	}
	g_slist_free(matches);

	if (count > 0 && first_start >= 0)
	{
		gint line = sci_get_line_from_position(doc->editor->sci, first_start) + 1;
		gchar *summary = g_strdup_printf("Marked %u matches in this document; bookmark-lines=%s; purge-first=%s.",
			count, bookmark_lines ? "yes" : "no", purge_bookmarks ? "yes" : "no");
		gchar *preview_title = g_strdup_printf("%s — %s:%d", action, DOC_FILENAME(doc), line);
		gchar *line_preview = search_studio_build_line_preview_body(doc, first_start, query, action);
		gchar *preview_body = g_strdup_printf("Mark impact summary\n\nFile: %s\nQuery: %s\nMode: %s\nMarked matches: %u\nBookmark lines: %s\nPurge existing bookmarks first: %s\n\n%s",
			DOC_FILENAME(doc),
			query,
			mode != NULL ? mode : _("(none)"),
			count,
			bookmark_lines ? _("yes") : _("no"),
			purge_bookmarks ? _("yes") : _("no"),
			line_preview);
		search_studio_result_append_preview_match(action, doc, query, mode, first_start,
			summary, preview_title, preview_body);
		g_free(preview_body);
		g_free(line_preview);
		g_free(preview_title);
		g_free(summary);
	}

	return count;
}


static void search_studio_transform_activate(GtkButton *button, gpointer user_data)
{
	GeanyDocument *doc = document_get_current();
	const gchar *action_id = user_data;
	ScintillaObject *sci;
	gint len;

	if (!DOC_VALID(doc) || action_id == NULL)
		return;

	sci = doc->editor->sci;
	len = sci_get_length(sci);

	search_studio_activity_append("[Transform] Executing %s on the active document.", action_id);

	if (g_strcmp0(action_id, "delete-blank-lines") == 0)
	{
		/* NPP IDM_EDIT_REMOVEEMPTYLINES: Remove empty lines (including lines with whitespace) */
		document_replace_all(doc, "^[ \t]*\r?\n", "", NULL, NULL, GEANY_FIND_REGEXP | GEANY_FIND_MULTILINE);
	}
	else if (g_strcmp0(action_id, "delete-surplus-blank-lines") == 0)
	{
		/* NPP IDM_EDIT_REMOVEEMPTYLINESWITHBLANK: consecutive blanks to one blank */
		document_replace_all(doc, "(\r?\n)([ \t]*\r?\n)+", "\\1", NULL, NULL, GEANY_FIND_REGEXP | GEANY_FIND_MULTILINE);
	}
	else if (g_strcmp0(action_id, "zap-non-printable") == 0)
	{
		/* NPP Zap non-printable characters to # */
		document_replace_all(doc, "[^\x20-\x7E\r\n\t]", "#", NULL, NULL, GEANY_FIND_REGEXP);
	}
	else if (g_strcmp0(action_id, "invert-case") == 0)
	{
		/* NPP IDM_EDIT_INVERTCASE */
		sci_start_undo_action(sci);
		for (gint i = 0; i < len; i++)
		{
			gchar c = sci_get_char_at(sci, i);
			if (g_ascii_islower(c))
			{
				gchar buf[2] = { (gchar)g_ascii_toupper(c), 0 };
				sci_set_target_start(sci, i);
				sci_set_target_end(sci, i+1);
				sci_replace_target(sci, buf, FALSE);
			}
			else if (g_ascii_isupper(c))
			{
				gchar buf[2] = { (gchar)g_ascii_tolower(c), 0 };
				sci_set_target_start(sci, i);
				sci_set_target_end(sci, i+1);
				sci_replace_target(sci, buf, FALSE);
			}
		}
		sci_end_undo_action(sci);
	}
	else if (g_strcmp0(action_id, "redact-selection") == 0)
	{
		/* NPP IDM_EDIT_REDACT_SELECTION */
		if (sci_has_selection(sci))
		{
			gint start = sci_get_selection_start(sci);
			gint end = sci_get_selection_end(sci);
			gint sel_len = end - start;
			gchar *redacted = g_malloc(sel_len + 1);
			memset(redacted, 'X', sel_len);
			redacted[sel_len] = '\0';
			sci_replace_sel(sci, redacted);
			g_free(redacted);
		}
	}

	search_studio_append_document_result("Transform", doc, action_id, "N/A", "Applied text transformation.");
}


static GtkWidget *search_studio_create_transform_page(void)
{
	GtkWidget *page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
	GtkWidget *grid = gtk_grid_new();
	GtkWidget *button;
	GtkWidget *label = ui_label_new_bold(_("Advanced Text Transformations"));
	GtkWidget *hint = gtk_label_new(_("Host for Notepad++ and TextFX style systematic transforms: whitespace cleanup, character zapping, and case inversions."));

	gtk_grid_set_column_spacing(GTK_GRID(grid), 12);
	gtk_grid_set_row_spacing(GTK_GRID(grid), 8);
	gtk_container_set_border_width(GTK_CONTAINER(page), 12);

	gtk_box_pack_start(GTK_BOX(page), label, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(page), hint, FALSE, FALSE, 0);

	button = gtk_button_new_with_mnemonic(_("Delete _Blank Lines"));
	g_signal_connect(button, "clicked", G_CALLBACK(search_studio_transform_activate), "delete-blank-lines");
	gtk_grid_attach(GTK_GRID(grid), button, 0, 0, 1, 1);

	button = gtk_button_new_with_mnemonic(_("Delete _Surplus Blank Lines"));
	g_signal_connect(button, "clicked", G_CALLBACK(search_studio_transform_activate), "delete-surplus-blank-lines");
	gtk_grid_attach(GTK_GRID(grid), button, 1, 0, 1, 1);

	button = gtk_button_new_with_mnemonic(_("_Zap Non-Printable"));
	g_signal_connect(button, "clicked", G_CALLBACK(search_studio_transform_activate), "zap-non-printable");
	gtk_grid_attach(GTK_GRID(grid), button, 0, 1, 1, 1);

	button = gtk_button_new_with_mnemonic(_("_Invert Case"));
	g_signal_connect(button, "clicked", G_CALLBACK(search_studio_transform_activate), "invert-case");
	gtk_grid_attach(GTK_GRID(grid), button, 1, 1, 1, 1);

	button = gtk_button_new_with_mnemonic(_("_Redact Selection"));
	g_signal_connect(button, "clicked", G_CALLBACK(search_studio_transform_activate), "redact-selection");
	gtk_grid_attach(GTK_GRID(grid), button, 0, 2, 1, 1);

	gtk_box_pack_start(GTK_BOX(page), grid, FALSE, FALSE, 0);
	return page;
}


static void search_studio_activity_show_page_hint(gint page_num)
{
	static const gchar *hints[] = {
		"Find: use the dense search cockpit for repeated lookup, counting, and bookmark-marking.",
		"Replace: execute targeted or bulk replacements directly, then fall back to the classic dialog if needed.",
		"Find in Files: launch directory searches directly from Search Studio and review detailed results in the message window.",
		"Find in Projects: start project-scoped searches from the current project base path and keep project patterns close at hand.",
		"Transform: systematic text modifications including whitespace cleanup, character zapping, and case inversions.",
		"Mark: highlight all matches, optionally bookmark matching lines, fan marking out across open documents, then clear everything in one click."
	};
	static const gchar *labels[] = {
		"Find",
		"Replace",
		"Find in Files",
		"Find in Projects",
		"Transform",
		"Mark"
	};

	if (page_num >= 0 && page_num < (gint) G_N_ELEMENTS(hints))
		search_studio_activity_append("[%s] %s", labels[page_num], hints[page_num]);
}


static void search_studio_notebook_switch_page(GtkNotebook *notebook, GtkWidget *page,
	guint page_num, gpointer user_data)
{
	search_studio_activity_show_page_hint((gint) page_num);
}


static void search_studio_results_selection_changed(GtkTreeSelection *selection, gpointer user_data)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	gchar *preview_title = NULL;
	gchar *preview_body = NULL;

	if (!gtk_tree_selection_get_selected(selection, &model, &iter))
	{
		search_studio_set_preview(_("Diff Preview"), _("Select a result row to inspect before/after or result details."));
		return;
	}

	gtk_tree_model_get(model, &iter,
		STUDIO_RESULT_PREVIEW_TITLE, &preview_title,
		STUDIO_RESULT_PREVIEW_BODY, &preview_body,
		-1);
	search_studio_set_preview(preview_title, preview_body);
	search_studio_show_lower_page(STUDIO_LOWER_PAGE_DIFF_PREVIEW);
	g_free(preview_title);
	g_free(preview_body);
}


static void search_studio_results_row_activated(GtkTreeView *tree_view, GtkTreePath *path,
	GtkTreeViewColumn *column, gpointer user_data)
{
	GtkTreeIter iter;
	GtkTreeModel *model = GTK_TREE_MODEL(studio_dlg.results_store);
	gboolean can_navigate = FALSE;
	gchar *action = NULL;
	gchar *target = NULL;
	gchar *filename = NULL;
	gint pos = -1;
	GeanyDocument *doc = NULL;

	if (!gtk_tree_model_get_iter(model, &iter, path))
		return;

	gtk_tree_model_get(model, &iter,
		STUDIO_RESULT_ACTION, &action,
		STUDIO_RESULT_TARGET, &target,
		STUDIO_RESULT_FILE, &filename,
		STUDIO_RESULT_POS, &pos,
		STUDIO_RESULT_CAN_NAVIGATE, &can_navigate,
		-1);

	if (!can_navigate)
	{
		search_studio_show_lower_page(STUDIO_LOWER_PAGE_DIFF_PREVIEW);
		search_studio_activity_append("[Results] %s on %s is informational; inspect the Diff Preview pane for details.",
			action != NULL ? action : _("Result"), target != NULL ? target : _("Search Studio"));
		g_free(filename);
		g_free(target);
		g_free(action);
		return;
	}

	if (filename != NULL && g_strcmp0(filename, GEANY_STRING_UNTITLED) != 0)
		doc = document_find_by_filename(filename);
	if (doc == NULL && filename != NULL && g_strcmp0(filename, GEANY_STRING_UNTITLED) != 0)
	{
		gchar *locale_filename = utils_get_locale_from_utf8(filename);
		doc = document_open_file(locale_filename, FALSE, NULL, NULL);
		g_free(locale_filename);
	}
	if (doc == NULL)
		doc = document_get_current();

	if (DOC_VALID(doc))
	{
		gint goto_pos = pos;
		if (goto_pos < 0 && gtk_tree_model_iter_n_children(model, NULL) >= 0)
		{
			gint line = -1;
			gtk_tree_model_get(model, &iter, STUDIO_RESULT_LINE, &line, -1);
			if (line > 0)
				goto_pos = sci_get_position_from_line(doc->editor->sci, line - 1);
		}
		document_show_tab(doc);
		if (goto_pos >= 0)
			editor_goto_pos(doc->editor, goto_pos, TRUE);
		search_studio_activity_append("[Results] Navigated to %s at position %d.", filename ? filename : GEANY_STRING_UNTITLED, goto_pos);
	}
	else
		utils_beep();

	g_free(filename);
	g_free(target);
	g_free(action);
}


static gboolean search_studio_results_focus_current(void)
{
	GtkTreeSelection *selection;
	GtkTreeModel *model;
	GtkTreeIter iter;
	GtkTreePath *path;

	if (studio_dlg.results_view == NULL || studio_dlg.results_store == NULL)
		return FALSE;

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(studio_dlg.results_view));
	if (gtk_tree_selection_get_selected(selection, &model, &iter))
	{
		search_studio_show_lower_page(STUDIO_LOWER_PAGE_RESULTS);
		gtk_widget_grab_focus(studio_dlg.results_view);
		return TRUE;
	}

	model = GTK_TREE_MODEL(studio_dlg.results_store);
	if (!gtk_tree_model_get_iter_first(model, &iter))
		return FALSE;

	path = gtk_tree_model_get_path(model, &iter);
	gtk_tree_view_set_cursor(GTK_TREE_VIEW(studio_dlg.results_view), path, NULL, FALSE);
	gtk_tree_selection_select_path(selection, path);
	gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(studio_dlg.results_view), path, NULL, FALSE, 0.0f, 0.0f);
	gtk_tree_path_free(path);
	search_studio_show_lower_page(STUDIO_LOWER_PAGE_RESULTS);
	gtk_widget_grab_focus(studio_dlg.results_view);
	return TRUE;
}


static gboolean search_studio_results_select_relative(gint delta, gboolean activate_row)
{
	GtkTreeSelection *selection;
	GtkTreeModel *model;
	GtkTreeIter iter;
	GtkTreePath *path;
	gint child_count;
	gint index;

	if (studio_dlg.results_view == NULL || studio_dlg.results_store == NULL)
		return FALSE;

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(studio_dlg.results_view));
	model = GTK_TREE_MODEL(studio_dlg.results_store);
	child_count = gtk_tree_model_iter_n_children(model, NULL);
	if (child_count <= 0)
		return FALSE;

	if (!gtk_tree_selection_get_selected(selection, &model, &iter))
	{
		if (!gtk_tree_model_get_iter_first(model, &iter))
			return FALSE;
		path = gtk_tree_model_get_path(model, &iter);
	}
	else
		path = gtk_tree_model_get_path(model, &iter);

	index = gtk_tree_path_get_indices(path) != NULL ? gtk_tree_path_get_indices(path)[0] : 0;
	index = (index + delta + child_count) % child_count;
	gtk_tree_path_free(path);
	path = gtk_tree_path_new_from_indices(index, -1);
	gtk_tree_view_set_cursor(GTK_TREE_VIEW(studio_dlg.results_view), path, NULL, FALSE);
	gtk_tree_selection_select_path(selection, path);
	gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(studio_dlg.results_view), path, NULL, FALSE, 0.0f, 0.0f);
	search_studio_show_lower_page(STUDIO_LOWER_PAGE_RESULTS);
	gtk_widget_grab_focus(studio_dlg.results_view);
	if (activate_row)
		gtk_tree_view_row_activated(GTK_TREE_VIEW(studio_dlg.results_view), path, NULL);
	gtk_tree_path_free(path);
	return TRUE;
}


static void search_studio_focus_results_activate(GtkButton *button, gpointer user_data)
{
	GtkWidget *page = GTK_WIDGET(user_data);
	const gchar *query = gtk_entry_get_text(GTK_ENTRY(ui_lookup_widget(page, "entry_search")));

	if (!search_studio_results_focus_current())
	{
		utils_beep();
		ui_set_statusbar(FALSE, _("No Search Studio results are available to focus."));
		search_studio_activity_append("[Results] Focus requested, but no Search Studio results were available.");
		search_studio_result_append("Focus Results", "Search Studio Results",
			EMPTY(query) ? "(empty)" : query,
			search_studio_mode_name(page),
			"No Search Studio results were available to focus.");
		return;
	}

	search_studio_activity_append("[Results] Focused the Search Studio results navigator.");
	search_studio_result_append("Focus Results", "Search Studio Results",
		EMPTY(query) ? "(empty)" : query,
		search_studio_mode_name(page),
		"Focused the Search Studio results navigator.");
}


static void search_studio_next_result_activate(GtkButton *button, gpointer user_data)
{
	GtkWidget *page = GTK_WIDGET(user_data);
	const gchar *query = gtk_entry_get_text(GTK_ENTRY(ui_lookup_widget(page, "entry_search")));

	if (!search_studio_results_select_relative(1, TRUE))
	{
		utils_beep();
		ui_set_statusbar(FALSE, _("No Search Studio result rows are available."));
		search_studio_activity_append("[Results] Next result requested, but no Search Studio result rows were available.");
		search_studio_result_append("Next Result", "Search Studio Results",
			EMPTY(query) ? "(empty)" : query,
			search_studio_mode_name(page),
			"No Search Studio result rows were available for next-result navigation.");
		return;
	}

	search_studio_activity_append("[Results] Advanced to the next Search Studio result row.");
}


static void search_studio_prev_result_activate(GtkButton *button, gpointer user_data)
{
	GtkWidget *page = GTK_WIDGET(user_data);
	const gchar *query = gtk_entry_get_text(GTK_ENTRY(ui_lookup_widget(page, "entry_search")));

	if (!search_studio_results_select_relative(-1, TRUE))
	{
		utils_beep();
		ui_set_statusbar(FALSE, _("No Search Studio result rows are available."));
		search_studio_activity_append("[Results] Previous result requested, but no Search Studio result rows were available.");
		search_studio_result_append("Previous Result", "Search Studio Results",
			EMPTY(query) ? "(empty)" : query,
			search_studio_mode_name(page),
			"No Search Studio result rows were available for previous-result navigation.");
		return;
	}

	search_studio_activity_append("[Results] Moved to the previous Search Studio result row.");
}


/* store text, clear search flags so we can use Search->Find Next/Previous */
static void setup_find_next(const gchar *text)
{
	g_free(search_data.text);
	g_free(search_data.original_text);
	search_data.text = g_strdup(text);
	search_data.original_text = g_strdup(text);
	search_data.flags = 0;
	search_data.backwards = FALSE;
	search_data.search_bar = FALSE;
}


/* Search for next match of the current "selection".
 * Optionally for X11 based systems, this will try to use the system-wide
 * x-selection first.
 * If it doesn't find a suitable search string it will try to use
 * the current word instead, or just repeat the last search.
 * Search flags are always zero.
 */
void search_find_selection(GeanyDocument *doc, gboolean search_backwards)
{
	gchar *s = NULL;

	g_return_if_fail(DOC_VALID(doc));

#ifdef G_OS_UNIX
	if (search_prefs.find_selection_type == GEANY_FIND_SEL_X)
	{
		GtkClipboard *clipboard = gtk_clipboard_get(GDK_SELECTION_PRIMARY);

		s = gtk_clipboard_wait_for_text(clipboard);
		if (s && (strchr(s,'\n') || strchr(s, '\r')))
		{
			g_free(s);
			s = NULL;
		};
	}
#endif

	if (!s && sci_has_selection(doc->editor->sci))
		s = sci_get_selection_contents(doc->editor->sci);

	if (!s && search_prefs.find_selection_type != GEANY_FIND_SEL_AGAIN)
	{
		/* get the current word */
		s = editor_get_default_selection(doc->editor, TRUE, NULL);
	}

	if (s)
	{
		setup_find_next(s);	/* allow find next/prev */

		if (document_find_text(doc, s, NULL, 0, search_backwards, NULL, FALSE) > -1)
			editor_display_current_line(doc->editor, 0.3F);
		g_free(s);
	}
	else if (search_prefs.find_selection_type == GEANY_FIND_SEL_AGAIN)
	{
		/* Repeat last search (in case selection was lost) */
		search_find_again(search_backwards);
	}
	else
	{
		utils_beep();
	}
}


static void on_expander_activated(GtkExpander *exp, gpointer data)
{
	gboolean *setting = data;

	*setting = gtk_expander_get_expanded(exp);
}


static void create_find_dialog(void)
{
	GtkWidget *label, *entry, *sbox, *vbox;
	GtkWidget *exp, *bbox, *button, *check_close;

	find_dlg.dialog = gtk_dialog_new();
	gtk_window_set_title(GTK_WINDOW(find_dlg.dialog), _("Find"));
	gtk_window_set_transient_for(GTK_WINDOW(find_dlg.dialog), GTK_WINDOW(main_widgets.window));
	gtk_window_set_destroy_with_parent(GTK_WINDOW(find_dlg.dialog), TRUE);

	vbox = ui_dialog_vbox_new(GTK_DIALOG(find_dlg.dialog));
	geany_search_dialog_set_css_name(find_dlg.dialog);
	gtk_box_set_spacing(GTK_BOX(vbox), 9);

	button = gtk_button_new_from_stock(GTK_STOCK_CLOSE);
	gtk_widget_set_size_request(button, MIN_DLG_BUTTON_SIZE, -1);
	gtk_dialog_add_action_widget(GTK_DIALOG(find_dlg.dialog), button,
		GTK_RESPONSE_CANCEL);

	button = ui_button_new_with_image(GTK_STOCK_GO_BACK, _("_Previous"));
	gtk_widget_set_size_request(button, MIN_DLG_BUTTON_SIZE, -1);
	gtk_dialog_add_action_widget(GTK_DIALOG(find_dlg.dialog), button,
		GEANY_RESPONSE_FIND_PREVIOUS);
	ui_hookup_widget(find_dlg.dialog, button, "btn_previous");

	button = ui_button_new_with_image(GTK_STOCK_GO_FORWARD, _("_Next"));
	gtk_widget_set_size_request(button, MIN_DLG_BUTTON_SIZE, -1);
	gtk_dialog_add_action_widget(GTK_DIALOG(find_dlg.dialog), button,
		GEANY_RESPONSE_FIND);

	label = gtk_label_new_with_mnemonic(_("_Search for:"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);

	entry = gtk_combo_box_text_new_with_entry();
	ui_entry_add_clear_icon(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(entry))));
	gtk_label_set_mnemonic_widget(GTK_LABEL(label), entry);
	gtk_entry_set_width_chars(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(entry))), 50);
	find_dlg.entry = gtk_bin_get_child(GTK_BIN(entry));

	g_signal_connect(gtk_bin_get_child(GTK_BIN(entry)), "activate",
			G_CALLBACK(on_find_entry_activate), entry);
	ui_entry_add_activate_backward_signal(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(entry))));
	g_signal_connect(gtk_bin_get_child(GTK_BIN(entry)), "activate-backward",
			G_CALLBACK(on_find_entry_activate_backward), entry);
	g_signal_connect(find_dlg.dialog, "response",
			G_CALLBACK(on_find_dialog_response), entry);
	g_signal_connect(find_dlg.dialog, "delete-event",
			G_CALLBACK(gtk_widget_hide_on_delete), NULL);

	sbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
	gtk_box_pack_start(GTK_BOX(sbox), label, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(sbox), entry, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), sbox, TRUE, FALSE, 0);

	gtk_container_add(GTK_CONTAINER(vbox),
		add_find_checkboxes(GTK_DIALOG(find_dlg.dialog)));

	/* Now add the multiple match options */
	exp = gtk_expander_new_with_mnemonic(_("_Find All"));
	gtk_expander_set_expanded(GTK_EXPANDER(exp), find_dlg.all_expanded);
	g_signal_connect_after(exp, "activate",
		G_CALLBACK(on_expander_activated), &find_dlg.all_expanded);

	bbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
	gtk_widget_set_margin_top(bbox, 6);

	/* close window checkbox */
	check_close = gtk_check_button_new_with_mnemonic(_("Close _dialog"));
	ui_hookup_widget(find_dlg.dialog, check_close, "check_close");
	gtk_button_set_focus_on_click(GTK_BUTTON(check_close), FALSE);
	gtk_widget_set_tooltip_text(check_close,
			_("Disable this option to keep the dialog open"));
	gtk_box_pack_start(GTK_BOX(bbox), check_close, TRUE, TRUE, 0);

	button = gtk_button_new_with_mnemonic(_("_Mark"));
	gtk_widget_set_size_request(button, MIN_DLG_BUTTON_SIZE, -1);
	gtk_widget_set_tooltip_text(button,
			_("Mark all matches in the current document"));
	gtk_container_add(GTK_CONTAINER(bbox), button);
	g_signal_connect(button, "clicked", G_CALLBACK(send_find_dialog_response),
		GINT_TO_POINTER(GEANY_RESPONSE_MARK));

	button = gtk_button_new_with_mnemonic(_("Co_unt"));
	gtk_widget_set_size_request(button, MIN_DLG_BUTTON_SIZE, -1);
	gtk_widget_set_tooltip_text(button,
			_("Count all matches in the current document without altering markers or the selection."));
	gtk_container_add(GTK_CONTAINER(bbox), button);
	g_signal_connect(button, "clicked", G_CALLBACK(send_find_dialog_response),
		GINT_TO_POINTER(GEANY_RESPONSE_COUNT));

	button = gtk_button_new_with_mnemonic(_("In Sessi_on"));
	gtk_widget_set_size_request(button, MIN_DLG_BUTTON_SIZE, -1);
	gtk_container_add(GTK_CONTAINER(bbox), button);
	g_signal_connect(button, "clicked", G_CALLBACK(send_find_dialog_response),
		GINT_TO_POINTER(GEANY_RESPONSE_FIND_IN_SESSION));

	button = gtk_button_new_with_mnemonic(_("_In Document"));
	gtk_widget_set_size_request(button, MIN_DLG_BUTTON_SIZE, -1);
	gtk_container_add(GTK_CONTAINER(bbox), button);
	g_signal_connect(button, "clicked", G_CALLBACK(send_find_dialog_response),
		GINT_TO_POINTER(GEANY_RESPONSE_FIND_IN_FILE));

	gtk_container_add(GTK_CONTAINER(exp), bbox);
	gtk_container_add(GTK_CONTAINER(vbox), exp);

#ifdef G_OS_WIN32
	win32_update_titlebar_theme(find_dlg.dialog);
#endif
}


static void set_dialog_position(GtkWidget *dialog, gint *position)
{
	if (position[0] >= 0)
		gtk_window_move(GTK_WINDOW(dialog), position[0], position[1]);
}


void search_show_find_dialog(void)
{
	GeanyDocument *doc = document_get_current();
	gchar *sel = NULL;

	g_return_if_fail(doc != NULL);

	sel = editor_get_default_selection(doc->editor, search_prefs.use_current_word, NULL);

	if (find_dlg.dialog == NULL)
	{
		create_find_dialog();
		stash_group_display(find_prefs, find_dlg.dialog);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ui_lookup_widget(find_dlg.dialog, "check_wrap")),
			search_prefs.always_wrap);
		if (sel)
			gtk_entry_set_text(GTK_ENTRY(find_dlg.entry), sel);

		set_dialog_position(find_dlg.dialog, find_dlg.position);
		gtk_widget_show_all(find_dlg.dialog);
	}
	else
	{
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ui_lookup_widget(find_dlg.dialog, "check_wrap")),
			search_prefs.always_wrap);
		if (sel != NULL)
		{
			/* update the search text from current selection */
			gtk_entry_set_text(GTK_ENTRY(find_dlg.entry), sel);
			/* reset the entry widget's background colour */
			ui_set_search_entry_background(find_dlg.entry, TRUE);
		}
		gtk_widget_grab_focus(find_dlg.entry);
		set_dialog_position(find_dlg.dialog, find_dlg.position);
		gtk_widget_show(find_dlg.dialog);
		/* bring the dialog back in the foreground in case it is already open but the focus is away */
		gtk_window_present(GTK_WINDOW(find_dlg.dialog));
	}

	g_free(sel);
}


static void send_replace_dialog_response(GtkButton *button, gpointer user_data)
{
	gtk_dialog_response(GTK_DIALOG(replace_dlg.dialog), GPOINTER_TO_INT(user_data));
}


static gboolean
on_widget_key_pressed_set_focus(GtkWidget *widget, GdkEventKey *event, gpointer user_data)
{
	if (event->keyval == GDK_KEY_Tab)
	{
		gtk_widget_grab_focus(GTK_WIDGET(user_data));
		return TRUE;
	}
	return FALSE;
}


static void create_replace_dialog(void)
{
	GtkWidget *label_find, *label_replace,
		*check_close, *button, *rbox, *fbox, *vbox, *exp, *bbox;
	GtkSizeGroup *label_size;

	replace_dlg.dialog = gtk_dialog_new();
	gtk_window_set_title(GTK_WINDOW(replace_dlg.dialog), _("Replace"));
	gtk_window_set_transient_for(GTK_WINDOW(replace_dlg.dialog), GTK_WINDOW(main_widgets.window));
	gtk_window_set_destroy_with_parent(GTK_WINDOW(replace_dlg.dialog), TRUE);

	vbox = ui_dialog_vbox_new(GTK_DIALOG(replace_dlg.dialog));
	gtk_box_set_spacing(GTK_BOX(vbox), 9);
	geany_search_dialog_set_css_name(replace_dlg.dialog);

	button = gtk_button_new_from_stock(GTK_STOCK_CLOSE);
	gtk_widget_set_size_request(button, MIN_DLG_BUTTON_SIZE, -1);
	gtk_dialog_add_action_widget(GTK_DIALOG(replace_dlg.dialog), button,
		GTK_RESPONSE_CANCEL);

	button = gtk_button_new_from_stock(GTK_STOCK_FIND);
	gtk_widget_set_size_request(button, MIN_DLG_BUTTON_SIZE, -1);
	gtk_dialog_add_action_widget(GTK_DIALOG(replace_dlg.dialog), button,
		GEANY_RESPONSE_FIND);
	button = gtk_button_new_with_mnemonic(_("_Replace"));
	gtk_widget_set_size_request(button, MIN_DLG_BUTTON_SIZE, -1);
	gtk_button_set_image(GTK_BUTTON(button),
		gtk_image_new_from_stock(GTK_STOCK_FIND_AND_REPLACE, GTK_ICON_SIZE_BUTTON));
	gtk_dialog_add_action_widget(GTK_DIALOG(replace_dlg.dialog), button,
		GEANY_RESPONSE_REPLACE);
	button = gtk_button_new_with_mnemonic(_("Replace & Fi_nd"));
	gtk_widget_set_size_request(button, MIN_DLG_BUTTON_SIZE, -1);
	gtk_button_set_image(GTK_BUTTON(button),
		gtk_image_new_from_stock(GTK_STOCK_FIND_AND_REPLACE, GTK_ICON_SIZE_BUTTON));
	gtk_dialog_add_action_widget(GTK_DIALOG(replace_dlg.dialog), button,
		GEANY_RESPONSE_REPLACE_AND_FIND);

	label_find = gtk_label_new_with_mnemonic(_("_Search for:"));
	gtk_misc_set_alignment(GTK_MISC(label_find), 0, 0.5);

	label_replace = gtk_label_new_with_mnemonic(_("Replace wit_h:"));
	gtk_misc_set_alignment(GTK_MISC(label_replace), 0, 0.5);

	replace_dlg.find_combobox = gtk_combo_box_text_new_with_entry();
	replace_dlg.find_entry = gtk_bin_get_child(GTK_BIN(replace_dlg.find_combobox));
	ui_entry_add_clear_icon(GTK_ENTRY(replace_dlg.find_entry));
	gtk_label_set_mnemonic_widget(GTK_LABEL(label_find), replace_dlg.find_combobox);
	gtk_entry_set_width_chars(GTK_ENTRY(replace_dlg.find_entry), 50);
	ui_hookup_widget(replace_dlg.dialog, replace_dlg.find_combobox, "entry_find");

	replace_dlg.replace_combobox = gtk_combo_box_text_new_with_entry();
	replace_dlg.replace_entry = gtk_bin_get_child(GTK_BIN(replace_dlg.replace_combobox));
	ui_entry_add_clear_icon(GTK_ENTRY(replace_dlg.replace_entry));
	gtk_label_set_mnemonic_widget(GTK_LABEL(label_replace), replace_dlg.replace_combobox);
	gtk_entry_set_width_chars(GTK_ENTRY(replace_dlg.replace_entry), 50);
	ui_hookup_widget(replace_dlg.dialog, replace_dlg.replace_combobox, "entry_replace");

	/* tab from find to the replace entry */
	g_signal_connect(replace_dlg.find_entry,
			"key-press-event", G_CALLBACK(on_widget_key_pressed_set_focus),
			replace_dlg.replace_entry);
	g_signal_connect(replace_dlg.find_entry, "activate",
			G_CALLBACK(on_replace_find_entry_activate), NULL);
	g_signal_connect(replace_dlg.replace_entry, "activate",
			G_CALLBACK(on_replace_entry_activate), NULL);
	g_signal_connect(replace_dlg.dialog, "response",
			G_CALLBACK(on_replace_dialog_response), NULL);
	g_signal_connect(replace_dlg.dialog, "delete-event",
			G_CALLBACK(gtk_widget_hide_on_delete), NULL);

	fbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
	gtk_box_pack_start(GTK_BOX(fbox), label_find, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(fbox), replace_dlg.find_combobox, TRUE, TRUE, 0);

	rbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
	gtk_box_pack_start(GTK_BOX(rbox), label_replace, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(rbox), replace_dlg.replace_combobox, TRUE, TRUE, 0);

	label_size = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);
	gtk_size_group_add_widget(label_size, label_find);
	gtk_size_group_add_widget(label_size, label_replace);
	g_object_unref(G_OBJECT(label_size));	/* auto destroy the size group */

	gtk_box_pack_start(GTK_BOX(vbox), fbox, TRUE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), rbox, TRUE, FALSE, 0);
	gtk_container_add(GTK_CONTAINER(vbox),
		add_find_checkboxes(GTK_DIALOG(replace_dlg.dialog)));

	/* Now add the multiple replace options */
	exp = gtk_expander_new_with_mnemonic(_("Re_place All"));
	gtk_expander_set_expanded(GTK_EXPANDER(exp), replace_dlg.all_expanded);
	g_signal_connect_after(exp, "activate",
		G_CALLBACK(on_expander_activated), &replace_dlg.all_expanded);

	bbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
	gtk_widget_set_margin_top(bbox, 6);

	/* close window checkbox */
	check_close = gtk_check_button_new_with_mnemonic(_("Close _dialog"));
	ui_hookup_widget(replace_dlg.dialog, check_close, "check_close");
	gtk_button_set_focus_on_click(GTK_BUTTON(check_close), FALSE);
	gtk_widget_set_tooltip_text(check_close,
			_("Disable this option to keep the dialog open"));
	gtk_box_pack_start(GTK_BOX(bbox), check_close, TRUE, TRUE, 0);

	button = gtk_button_new_with_mnemonic(_("In Sessi_on"));
	gtk_widget_set_size_request(button, MIN_DLG_BUTTON_SIZE, -1);
	gtk_container_add(GTK_CONTAINER(bbox), button);
	g_signal_connect(button, "clicked", G_CALLBACK(send_replace_dialog_response),
		GINT_TO_POINTER(GEANY_RESPONSE_REPLACE_IN_SESSION));

	button = gtk_button_new_with_mnemonic(_("_In Document"));
	gtk_widget_set_size_request(button, MIN_DLG_BUTTON_SIZE, -1);
	gtk_container_add(GTK_CONTAINER(bbox), button);
	g_signal_connect(button, "clicked", G_CALLBACK(send_replace_dialog_response),
		GINT_TO_POINTER(GEANY_RESPONSE_REPLACE_IN_FILE));

	button = gtk_button_new_with_mnemonic(_("In Se_lection"));
	gtk_widget_set_size_request(button, MIN_DLG_BUTTON_SIZE, -1);
	gtk_widget_set_tooltip_text(button,
		_("Replace all matches found in the currently selected text"));
	gtk_container_add(GTK_CONTAINER(bbox), button);
	g_signal_connect(button, "clicked", G_CALLBACK(send_replace_dialog_response),
		GINT_TO_POINTER(GEANY_RESPONSE_REPLACE_IN_SEL));

	gtk_container_add(GTK_CONTAINER(exp), bbox);
	gtk_container_add(GTK_CONTAINER(vbox), exp);

#ifdef G_OS_WIN32
		win32_update_titlebar_theme(replace_dlg.dialog);
#endif
}


void search_show_replace_dialog(void)
{
	GeanyDocument *doc = document_get_current();
	gchar *sel = NULL;

	if (doc == NULL)
		return;

	sel = editor_get_default_selection(doc->editor, search_prefs.use_current_word, NULL);

	if (replace_dlg.dialog == NULL)
	{
		create_replace_dialog();
		stash_group_display(replace_prefs, replace_dlg.dialog);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ui_lookup_widget(replace_dlg.dialog, "check_wrap")),
			search_prefs.always_wrap);
		if (sel)
			gtk_entry_set_text(GTK_ENTRY(replace_dlg.find_entry), sel);

		set_dialog_position(replace_dlg.dialog, replace_dlg.position);
		gtk_widget_show_all(replace_dlg.dialog);
	}
	else
	{
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ui_lookup_widget(replace_dlg.dialog, "check_wrap")),
			search_prefs.always_wrap);
		if (sel != NULL)
		{
			/* update the search text from current selection */
			gtk_entry_set_text(GTK_ENTRY(replace_dlg.find_entry), sel);
			/* reset the entry widget's background colour */
			ui_set_search_entry_background(replace_dlg.find_entry, TRUE);
		}
		gtk_widget_grab_focus(replace_dlg.find_entry);
		set_dialog_position(replace_dlg.dialog, replace_dlg.position);
		gtk_widget_show(replace_dlg.dialog);
		/* bring the dialog back in the foreground in case it is already open but the focus is away */
		gtk_window_present(GTK_WINDOW(replace_dlg.dialog));
	}

	g_free(sel);
}


static void on_widget_toggled_set_sensitive(GtkToggleButton *togglebutton, gpointer user_data)
{
	/* disable extra option entry when checkbutton not checked */
	gtk_widget_set_sensitive(GTK_WIDGET(user_data),
		gtk_toggle_button_get_active(togglebutton));
}


static void update_file_patterns(GtkWidget *mode_combo, GtkWidget *fcombo)
{
	gint selection;
	GtkWidget *entry;

	entry = gtk_bin_get_child(GTK_BIN(fcombo));

	selection = gtk_combo_box_get_active(GTK_COMBO_BOX(mode_combo));

	if (selection == FILES_MODE_ALL)
	{
		gtk_entry_set_text(GTK_ENTRY(entry), "");
		gtk_widget_set_sensitive(fcombo, FALSE);
	}
	else if (selection == FILES_MODE_CUSTOM)
	{
		gtk_widget_set_sensitive(fcombo, TRUE);
	}
	else if (selection == FILES_MODE_PROJECT)
	{
		if (app->project && !EMPTY(app->project->file_patterns))
		{
			gchar *patterns;

			patterns = g_strjoinv(" ", app->project->file_patterns);
			gtk_entry_set_text(GTK_ENTRY(entry), patterns);
			g_free(patterns);
		}
		else
		{
			gtk_entry_set_text(GTK_ENTRY(entry), "");
		}

		gtk_widget_set_sensitive(fcombo, FALSE);
	}
}


/* creates the combo to choose which files include in the search */
static GtkWidget *create_fif_file_mode_combo(void)
{
	GtkWidget *combo;
	GtkCellRenderer *renderer;
	GtkListStore *store;
	GtkTreeIter iter;

	/* text/sensitive */
	store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_BOOLEAN);
	gtk_list_store_append(store, &iter);
	gtk_list_store_set(store, &iter, 0, _("all"), 1, TRUE, -1);
	gtk_list_store_append(store, &iter);
	gtk_list_store_set(store, &iter, 0, _("project"), 1, app->project != NULL, -1);
	gtk_list_store_append(store, &iter);
	gtk_list_store_set(store, &iter, 0, _("custom"), 1, TRUE, -1);

	combo = gtk_combo_box_new_with_model(GTK_TREE_MODEL(store));
	g_object_unref(store);
	gtk_widget_set_tooltip_text(combo, _("All: search all files in the directory\n"
										"Project: use file patterns defined in the project settings\n"
										"Custom: specify file patterns manually"));

	renderer = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(combo), renderer, TRUE);
	gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(combo), renderer, "text", 0, "sensitive", 1, NULL);

	return combo;
}


/* updates the sensitivity of the project combo item */
static void update_fif_file_mode_combo(void)
{
	GtkTreeModel *model = gtk_combo_box_get_model(GTK_COMBO_BOX(fif_dlg.files_mode_combo));
	GtkTreeIter iter;

	/* "1" refers to the second list entry, project  */
	if (gtk_tree_model_get_iter_from_string(model, &iter, "1"))
		gtk_list_store_set(GTK_LIST_STORE(model), &iter, 1, app->project != NULL, -1);
}


static void create_fif_dialog(void)
{
	GtkWidget *dir_combo, *combo, *fcombo, *e_combo, *entry;
	GtkWidget *label, *label1, *label2, *label3, *checkbox1, *checkbox2, *check_wholeword,
		*check_recursive, *check_extra, *entry_extra, *check_regexp, *combo_files_mode;
	GtkWidget *dbox, *sbox, *lbox, *rbox, *hbox, *vbox, *ebox;
	GtkSizeGroup *size_group;

	fif_dlg.dialog = gtk_dialog_new_with_buttons(
		_("Find in Files"), GTK_WINDOW(main_widgets.window), GTK_DIALOG_DESTROY_WITH_PARENT,
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, NULL);
	vbox = ui_dialog_vbox_new(GTK_DIALOG(fif_dlg.dialog));
	gtk_box_set_spacing(GTK_BOX(vbox), 9);
	geany_search_dialog_set_css_name(fif_dlg.dialog);

	gtk_dialog_add_button(GTK_DIALOG(fif_dlg.dialog), GTK_STOCK_FIND, GTK_RESPONSE_ACCEPT);
	gtk_dialog_set_default_response(GTK_DIALOG(fif_dlg.dialog),
		GTK_RESPONSE_ACCEPT);

	label = gtk_label_new_with_mnemonic(_("_Search for:"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);

	combo = gtk_combo_box_text_new_with_entry();
	entry = gtk_bin_get_child(GTK_BIN(combo));
	ui_entry_add_clear_icon(GTK_ENTRY(entry));
	gtk_label_set_mnemonic_widget(GTK_LABEL(label), entry);
	gtk_entry_set_width_chars(GTK_ENTRY(entry), 50);
	gtk_entry_set_activates_default(GTK_ENTRY(entry), TRUE);
	fif_dlg.search_combo = combo;

	sbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
	gtk_box_pack_start(GTK_BOX(sbox), label, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(sbox), combo, TRUE, TRUE, 0);

	/* make labels same width */
	size_group = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);
	gtk_size_group_add_widget(size_group, label);

	label3 = gtk_label_new_with_mnemonic(_("File _patterns:"));
	gtk_misc_set_alignment(GTK_MISC(label3), 0, 0.5);

	combo_files_mode = create_fif_file_mode_combo();
	gtk_label_set_mnemonic_widget(GTK_LABEL(label3), combo_files_mode);
	ui_hookup_widget(fif_dlg.dialog, combo_files_mode, "combo_files_mode");
	fif_dlg.files_mode_combo = combo_files_mode;

	fcombo = gtk_combo_box_text_new_with_entry();
	entry = gtk_bin_get_child(GTK_BIN(fcombo));
	ui_entry_add_clear_icon(GTK_ENTRY(entry));
	gtk_entry_set_activates_default(GTK_ENTRY(entry), TRUE);
	gtk_widget_set_tooltip_text(entry,
		_("Space separated list of file patterns (e.g. *.c *.h)"));
	ui_hookup_widget(fif_dlg.dialog, entry, "entry_files");
	fif_dlg.files_combo = fcombo;

	/* update the entry when selection is changed */
	g_signal_connect(combo_files_mode, "changed", G_CALLBACK(update_file_patterns), fcombo);

	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
	gtk_box_pack_start(GTK_BOX(hbox), label3, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), combo_files_mode, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), fcombo, TRUE, TRUE, 0);

	label1 = gtk_label_new_with_mnemonic(_("_Directory:"));
	gtk_misc_set_alignment(GTK_MISC(label1), 0, 0.5);

	dir_combo = gtk_combo_box_text_new_with_entry();
	entry = gtk_bin_get_child(GTK_BIN(dir_combo));
	ui_entry_add_clear_icon(GTK_ENTRY(entry));
	gtk_entry_set_activates_default(GTK_ENTRY(entry), TRUE);
	gtk_label_set_mnemonic_widget(GTK_LABEL(label1), entry);
	gtk_entry_set_width_chars(GTK_ENTRY(entry), 50);
	fif_dlg.dir_combo = dir_combo;

	/* tab from files to the dir entry */
	g_signal_connect(gtk_bin_get_child(GTK_BIN(fcombo)), "key-press-event",
		G_CALLBACK(on_widget_key_pressed_set_focus), entry);

	dbox = ui_path_box_new(NULL, GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
		GTK_ENTRY(entry));
	gtk_box_pack_start(GTK_BOX(dbox), label1, FALSE, FALSE, 0);

	label2 = gtk_label_new_with_mnemonic(_("E_ncoding:"));
	gtk_misc_set_alignment(GTK_MISC(label2), 0, 0.5);

	e_combo = ui_create_encodings_combo_box(FALSE, GEANY_ENCODING_UTF_8);
	gtk_label_set_mnemonic_widget(GTK_LABEL(label2), e_combo);
	fif_dlg.encoding_combo = e_combo;

	ebox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
	gtk_box_pack_start(GTK_BOX(ebox), label2, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(ebox), e_combo, TRUE, TRUE, 0);

	gtk_size_group_add_widget(size_group, label1);
	gtk_size_group_add_widget(size_group, label2);
	gtk_size_group_add_widget(size_group, label3);
	g_object_unref(G_OBJECT(size_group));	/* auto destroy the size group */

	gtk_box_pack_start(GTK_BOX(vbox), sbox, TRUE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), dbox, TRUE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), ebox, TRUE, FALSE, 0);

	check_regexp = gtk_check_button_new_with_mnemonic(_("_Use regular expressions"));
	ui_hookup_widget(fif_dlg.dialog, check_regexp, "check_regexp");
	gtk_button_set_focus_on_click(GTK_BUTTON(check_regexp), FALSE);
	gtk_widget_set_tooltip_text(check_regexp, _("See grep's manual page for more information"));

	check_recursive = gtk_check_button_new_with_mnemonic(_("_Recurse in subfolders"));
	ui_hookup_widget(fif_dlg.dialog, check_recursive, "check_recursive");
	gtk_button_set_focus_on_click(GTK_BUTTON(check_recursive), FALSE);

	checkbox1 = gtk_check_button_new_with_mnemonic(_("C_ase sensitive"));
	ui_hookup_widget(fif_dlg.dialog, checkbox1, "check_case");
	gtk_button_set_focus_on_click(GTK_BUTTON(checkbox1), FALSE);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(checkbox1), TRUE);

	check_wholeword = gtk_check_button_new_with_mnemonic(_("Match only a _whole word"));
	ui_hookup_widget(fif_dlg.dialog, check_wholeword, "check_wholeword");
	gtk_button_set_focus_on_click(GTK_BUTTON(check_wholeword), FALSE);

	checkbox2 = gtk_check_button_new_with_mnemonic(_("_Invert search results"));
	ui_hookup_widget(fif_dlg.dialog, checkbox2, "check_invert");
	gtk_button_set_focus_on_click(GTK_BUTTON(checkbox2), FALSE);
	gtk_widget_set_tooltip_text(checkbox2,
			_("Invert the sense of matching, to select non-matching lines"));

	lbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_container_add(GTK_CONTAINER(lbox), check_regexp);
	gtk_container_add(GTK_CONTAINER(lbox), checkbox2);
	gtk_container_add(GTK_CONTAINER(lbox), check_recursive);

	rbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_container_add(GTK_CONTAINER(rbox), checkbox1);
	gtk_container_add(GTK_CONTAINER(rbox), check_wholeword);
	gtk_container_add(GTK_CONTAINER(rbox), gtk_label_new(NULL));

	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
	gtk_container_add(GTK_CONTAINER(hbox), lbox);
	gtk_container_add(GTK_CONTAINER(hbox), rbox);
	gtk_container_add(GTK_CONTAINER(vbox), hbox);

	check_extra = gtk_check_button_new_with_mnemonic(_("E_xtra options:"));
	ui_hookup_widget(fif_dlg.dialog, check_extra, "check_extra");
	gtk_button_set_focus_on_click(GTK_BUTTON(check_extra), FALSE);

	entry_extra = gtk_entry_new();
	ui_entry_add_clear_icon(GTK_ENTRY(entry_extra));
	gtk_entry_set_activates_default(GTK_ENTRY(entry_extra), TRUE);
	gtk_widget_set_sensitive(entry_extra, FALSE);
	gtk_widget_set_tooltip_text(entry_extra, _("Other options to pass to Grep"));
	ui_hookup_widget(fif_dlg.dialog, entry_extra, "entry_extra");

	/* enable entry_extra when check_extra is checked */
	g_signal_connect(check_extra, "toggled",
		G_CALLBACK(on_widget_toggled_set_sensitive), entry_extra);

	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
	gtk_box_pack_start(GTK_BOX(hbox), check_extra, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), entry_extra, TRUE, TRUE, 0);
	gtk_container_add(GTK_CONTAINER(vbox), hbox);

	g_signal_connect(fif_dlg.dialog, "response",
			G_CALLBACK(on_find_in_files_dialog_response), NULL);
	g_signal_connect(fif_dlg.dialog, "delete-event",
			G_CALLBACK(gtk_widget_hide_on_delete), NULL);

#ifdef G_OS_WIN32
		win32_update_titlebar_theme(fif_dlg.dialog);
#endif
}


/**
 * Shows the Find in Files dialog.
 *
 * @param dir The directory to search in (UTF-8 encoding). May be @c NULL
 * to determine it the usual way by using the current document's path.
 *
 * @since 0.14, plugin API 53
 */
GEANY_API_SYMBOL
void search_show_find_in_files_dialog(const gchar *dir)
{
	search_show_find_in_files_dialog_full(NULL, dir);
}


void search_show_find_in_files_dialog_full(const gchar *text, const gchar *dir)
{
	GtkWidget *entry; /* for child GtkEntry of a GtkComboBoxEntry */
	GeanyDocument *doc = document_get_current();
	gchar *sel = NULL;
	gchar *cur_dir = NULL;
	GeanyEncodingIndex enc_idx = GEANY_ENCODING_UTF_8;

	if (fif_dlg.dialog == NULL)
	{
		create_fif_dialog();
		gtk_widget_show_all(fif_dlg.dialog);
		if (doc && !text)
			sel = editor_get_default_selection(doc->editor, search_prefs.use_current_word, NULL);
	}
	stash_group_display(fif_prefs, fif_dlg.dialog);

	if (!text)
	{
		if (doc && ! sel)
			sel = editor_get_default_selection(doc->editor, search_prefs.use_current_word, NULL);
		text = sel;
	}
	entry = gtk_bin_get_child(GTK_BIN(fif_dlg.search_combo));
	if (text)
		gtk_entry_set_text(GTK_ENTRY(entry), text);
	g_free(sel);

	/* add project's base path directory to the dir list, we do this here once
	 * (in create_fif_dialog() it would fail if a project is opened after dialog creation) */
	if (app->project != NULL && !EMPTY(app->project->base_path))
	{
		ui_combo_box_prepend_text_once(GTK_COMBO_BOX_TEXT(fif_dlg.dir_combo),
			app->project->base_path);
	}

	entry = gtk_bin_get_child(GTK_BIN(fif_dlg.dir_combo));
	if (!EMPTY(dir))
		cur_dir = g_strdup(dir);	/* custom directory argument passed */
	else
	{
		if (search_prefs.use_current_file_dir)
		{
			static gchar *last_cur_dir = NULL;
			static GeanyDocument *last_doc = NULL;

			/* Only set the directory entry once for the current document */
			cur_dir = utils_get_current_file_dir_utf8();
			if (doc == last_doc && cur_dir && utils_str_equal(cur_dir, last_cur_dir))
			{
				/* in case the user now wants the current directory, add it to history */
				ui_combo_box_add_to_history(GTK_COMBO_BOX_TEXT(fif_dlg.dir_combo), cur_dir, 0);
				SETPTR(cur_dir, NULL);
			}
			else
				SETPTR(last_cur_dir, g_strdup(cur_dir));

			last_doc = doc;
		}
		if (!cur_dir && EMPTY(gtk_entry_get_text(GTK_ENTRY(entry))))
		{
			/* use default_open_path if no directory could be determined
			 * (e.g. when no files are open) */
			if (!cur_dir)
				cur_dir = g_strdup(utils_get_default_dir_utf8());
			if (!cur_dir)
				cur_dir = g_get_current_dir();
		}
	}
	if (cur_dir)
	{
		gtk_entry_set_text(GTK_ENTRY(entry), cur_dir);
		g_free(cur_dir);
	}

	ui_set_search_entry_background(fif_dlg.search_combo, TRUE);
	ui_set_search_entry_background(fif_dlg.dir_combo, TRUE);
	update_fif_file_mode_combo();
	update_file_patterns(fif_dlg.files_mode_combo, fif_dlg.files_combo);

	/* set the encoding of the current file */
	if (doc != NULL)
		enc_idx = encodings_get_idx_from_charset(doc->encoding);
	ui_encodings_combo_box_set_active_encoding(GTK_COMBO_BOX(fif_dlg.encoding_combo), enc_idx);

	/* put the focus to the directory entry if it is empty */
	if (utils_str_equal(gtk_entry_get_text(GTK_ENTRY(entry)), ""))
		gtk_widget_grab_focus(fif_dlg.dir_combo);
	else
		gtk_widget_grab_focus(fif_dlg.search_combo);

	/* set dialog window position */
	set_dialog_position(fif_dlg.dialog, fif_dlg.position);

	gtk_widget_show(fif_dlg.dialog);
	/* bring the dialog back in the foreground in case it is already open but the focus is away */
	gtk_window_present(GTK_WINDOW(fif_dlg.dialog));
}


/* like dialogs_show_question_full() but makes the non-cancel button default */
gboolean search_show_wrap_dialog(const gchar *search_text)
{
	gboolean ret;
	GtkWidget *dialog;
	GtkWidget *btn;
	GtkWidget *visible_dialog = NULL;
	gchar *question_text;

	if (find_dlg.dialog && gtk_widget_is_visible(find_dlg.dialog))
		visible_dialog = find_dlg.dialog;
	else if (replace_dlg.dialog && gtk_widget_is_visible(replace_dlg.dialog))
		visible_dialog = replace_dlg.dialog;

	if (visible_dialog)
		gtk_widget_hide(visible_dialog);

	question_text = g_strdup_printf(_("\"%s\" was not found."), search_text);

	dialog = gtk_message_dialog_new(GTK_WINDOW(main_widgets.window),
		GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_QUESTION,
		GTK_BUTTONS_NONE, "%s", question_text);
	geany_dialog_set_css_name(dialog);
	gtk_window_set_title(GTK_WINDOW(dialog), _("Question"));
	gtk_window_set_icon_name(GTK_WINDOW(dialog), "geany");

	gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog),
		"%s", _("Wrap search and find again?"));

	gtk_dialog_add_button(GTK_DIALOG(dialog), GTK_STOCK_CANCEL, GTK_RESPONSE_NO);
	btn = gtk_dialog_add_button(GTK_DIALOG(dialog), GTK_STOCK_FIND, GTK_RESPONSE_YES);
	gtk_widget_grab_default(btn);

	ret = gtk_dialog_run(GTK_DIALOG(dialog));

	gtk_widget_destroy(dialog);
	g_free(question_text);

	if (visible_dialog && ret == GTK_RESPONSE_YES)
		gtk_widget_show(visible_dialog);

	return ret == GTK_RESPONSE_YES;
}


static void
on_find_replace_checkbutton_toggled(GtkToggleButton *togglebutton, gpointer user_data)
{
	GtkWidget *dialog = GTK_WIDGET(user_data);
	GtkToggleButton *chk_regexp = GTK_TOGGLE_BUTTON(
		ui_lookup_widget(dialog, "check_regexp"));

	if (togglebutton == chk_regexp)
	{
		gboolean regex_set = gtk_toggle_button_get_active(chk_regexp);
		GtkWidget *check_word = ui_lookup_widget(dialog, "check_word");
		GtkWidget *check_wordstart = ui_lookup_widget(dialog, "check_wordstart");
		GtkWidget *check_escape = ui_lookup_widget(dialog, "check_escape");
		GtkWidget *check_multiline = ui_lookup_widget(dialog, "check_multiline");
		GtkWidget *check_dotmatchnewline = ui_lookup_widget(dialog, "check_dotmatchnewline");
		gboolean replace = (dialog != find_dlg.dialog);
		const char *back_button[2] = { "btn_previous" , "check_back" };

		/* hide options that don't apply to regex searches */
		gtk_widget_set_sensitive(check_escape, ! regex_set);
		gtk_widget_set_sensitive(ui_lookup_widget(dialog, back_button[replace]), ! regex_set);
		gtk_widget_set_sensitive(check_word, ! regex_set);
		gtk_widget_set_sensitive(check_wordstart, ! regex_set);
		gtk_widget_set_sensitive(check_multiline, regex_set);
		gtk_widget_set_sensitive(check_dotmatchnewline, regex_set);
	}
}


static GeanyMatchInfo *match_info_new(GeanyFindFlags flags, gint start, gint end)
{
	GeanyMatchInfo *info = g_slice_alloc(sizeof *info);

	info->flags = flags;
	info->start = start;
	info->end = end;
	info->match_text = NULL;

	return info;
}

void geany_match_info_free(GeanyMatchInfo *info)
{
	g_free(info->match_text);
	g_slice_free1(sizeof *info, info);
}


/* find all in the given range.
 * Returns a list of allocated GeanyMatchInfo, should be freed using:
 *
 * 	foreach_slist(node, matches)
 * 		geany_match_info_free(node->data);
 * 	g_slist_free(matches); */
static GSList *find_range(ScintillaObject *sci, GeanyFindFlags flags, struct Sci_TextToFind *ttf)
{
	GSList *matches = NULL;
	GeanyMatchInfo *info;

	g_return_val_if_fail(sci != NULL && ttf->lpstrText != NULL, NULL);
	if (! *ttf->lpstrText)
		return NULL;

	while (search_find_text(sci, flags, ttf, &info) != -1)
	{
		if (ttf->chrgText.cpMax > ttf->chrg.cpMax)
		{
			/* found text is partially out of range */
			geany_match_info_free(info);
			break;
		}

		matches = g_slist_prepend(matches, info);
		ttf->chrg.cpMin = ttf->chrgText.cpMax;

		/* avoid rematching with empty matches like "(?=[a-z])" or "^$".
		 * note we cannot assume a match will always be empty or not and then break out, since
		 * matches like "a?(?=b)" will sometimes be empty and sometimes not */
		if (ttf->chrgText.cpMax == ttf->chrgText.cpMin)
			ttf->chrg.cpMin ++;
	}

	return g_slist_reverse(matches);
}


static void search_clear_all_marks(GeanyDocument *doc)
{
	g_return_if_fail(DOC_VALID(doc));

	editor_indicator_clear(doc->editor, GEANY_INDICATOR_SEARCH);
	sci_marker_delete_all(doc->editor->sci, 1);
}


/* Clears markers if text is null/empty.
 * @return Number of matches marked. */
static void search_clear_all_marks(GeanyDocument *doc)
{
	g_return_if_fail(DOC_VALID(doc));
	editor_indicator_clear(doc->editor, GEANY_INDICATOR_SEARCH);
	sci_marker_delete_all(doc->editor->sci, 1);
}


static gint search_mark_all_with_options(GeanyDocument *doc, const gchar *search_text,
	GeanyFindFlags flags, gboolean bookmark_lines, gboolean purge_bookmarks)
{
	gint count = 0;
	struct Sci_TextToFind ttf;
	GSList *match, *matches;

	g_return_val_if_fail(DOC_VALID(doc), 0);

	/* clear previous search indicators */
	editor_indicator_clear(doc->editor, GEANY_INDICATOR_SEARCH);
	if (purge_bookmarks)
		sci_marker_delete_all(doc->editor->sci, 1);

	if (G_UNLIKELY(EMPTY(search_text)))
		return 0;

	ttf.chrg.cpMin = 0;
	ttf.chrg.cpMax = sci_get_length(doc->editor->sci);
	ttf.lpstrText = (gchar *)search_text;

	matches = find_range(doc->editor->sci, flags, &ttf);
	foreach_slist (match, matches)
	{
		GeanyMatchInfo *info = match->data;

		if (info->end != info->start)
			editor_indicator_set_on_range(doc->editor, GEANY_INDICATOR_SEARCH, info->start, info->end);
		if (bookmark_lines)
			sci_set_marker_at_line(doc->editor->sci, sci_get_line_from_position(doc->editor->sci, info->start), 1);
		count++;

		geany_match_info_free(info);
	}
	g_slist_free(matches);

	return count;
}


static gboolean search_studio_line_is_marked(GeanyDocument *doc, gint line)
{
	ScintillaObject *sci;
	gint line_start;
	gint line_end;
	gint pos;

	g_return_val_if_fail(DOC_VALID(doc), FALSE);
	g_return_val_if_fail(line >= 0, FALSE);

	sci = doc->editor->sci;
	if (sci_is_marker_set_at_line(sci, line, 1))
		return TRUE;

	line_start = sci_get_position_from_line(sci, line);
	line_end = sci_get_line_end_position(sci, line);
	if (line_end <= line_start)
		return FALSE;

	for (pos = line_start; pos < line_end; pos++)
	{
		if (SSM(sci, SCI_INDICATORVALUEAT, GEANY_INDICATOR_SEARCH, pos))
			return TRUE;
	}

	return FALSE;
}


static gboolean *search_studio_collect_marked_line_states(GeanyDocument *doc,
	guint *line_count_out, guint *marked_line_count_out)
{
	gboolean *states;
	guint line_count;
	guint marked_count = 0;
	guint line;

	g_return_val_if_fail(DOC_VALID(doc), NULL);

	line_count = sci_get_line_count(doc->editor->sci);
	states = g_new0(gboolean, MAX(line_count, 1));
	for (line = 0; line < line_count; line++)
	{
		states[line] = search_studio_line_is_marked(doc, (gint) line);
		if (states[line])
			marked_count++;
	}

	if (line_count_out != NULL)
		*line_count_out = line_count;
	if (marked_line_count_out != NULL)
		*marked_line_count_out = marked_count;
	return states;
}


static guint search_studio_copy_marked_lines_to_clipboard(GeanyDocument *doc)
{
	gboolean *states;
	guint line_count;
	guint marked_count;
	GString *text;
	guint line;

	g_return_val_if_fail(DOC_VALID(doc), 0);

	states = search_studio_collect_marked_line_states(doc, &line_count, &marked_count);
	if (marked_count == 0)
	{
		g_free(states);
		return 0;
	}

	text = g_string_new(NULL);
	for (line = 0; line < line_count; line++)
	{
		gchar *chunk;
		gint start;
		gint end;

		if (!states[line])
			continue;

		start = sci_get_position_from_line(doc->editor->sci, (gint) line);
		end = (line + 1 < line_count) ?
			sci_get_position_from_line(doc->editor->sci, (gint) line + 1) :
			sci_get_length(doc->editor->sci);
		chunk = sci_get_contents_range(doc->editor->sci, start, end);
		g_string_append(text, chunk);
		g_free(chunk);
	}

	gtk_clipboard_set_text(
		gtk_clipboard_get(gdk_atom_intern("CLIPBOARD", FALSE)),
		text->str, -1);
	g_string_free(text, TRUE);
	g_free(states);
	return marked_count;
}


static guint search_studio_delete_marked_lines(GeanyDocument *doc, gboolean delete_marked_lines)
{
	gboolean *states;
	guint line_count;
	guint marked_count;
	guint affected_count;
	gint line;

	g_return_val_if_fail(DOC_VALID(doc), 0);

	states = search_studio_collect_marked_line_states(doc, &line_count, &marked_count);
	affected_count = delete_marked_lines ? marked_count : line_count - marked_count;
	if (affected_count == 0)
	{
		g_free(states);
		return 0;
	}

	sci_start_undo_action(doc->editor->sci);
	for (line = (gint) line_count - 1; line >= 0; line--)
	{
		gboolean delete_line = delete_marked_lines ? states[line] : !states[line];
		gint start;
		gint end;

		if (!delete_line)
			continue;

		start = sci_get_position_from_line(doc->editor->sci, line);
		end = (line + 1 < (gint) line_count) ?
			sci_get_position_from_line(doc->editor->sci, line + 1) :
			sci_get_length(doc->editor->sci);
		sci_set_target_start(doc->editor->sci, start);
		sci_set_target_end(doc->editor->sci, end);
		sci_replace_target(doc->editor->sci, "", FALSE);
	}
	sci_end_undo_action(doc->editor->sci);

	search_clear_all_marks(doc);
	g_free(states);
	return affected_count;
}


static guint search_studio_inverse_marked_lines(GeanyDocument *doc)
{
	gboolean *states;
	guint line_count;
	guint marked_count;
	guint inverted_count = 0;
	guint line;

	g_return_val_if_fail(DOC_VALID(doc), 0);

	states = search_studio_collect_marked_line_states(doc, &line_count, &marked_count);
	search_clear_all_marks(doc);
	for (line = 0; line < line_count; line++)
	{
		if (states[line])
			continue;

		editor_indicator_set_on_line(doc->editor, GEANY_INDICATOR_SEARCH, (gint) line);
		sci_set_marker_at_line(doc->editor->sci, (gint) line, 1);
		inverted_count++;
	}

	g_free(states);
	return inverted_count;
}


gint search_mark_all(GeanyDocument *doc, const gchar *search_text, GeanyFindFlags flags)
{
	return search_mark_all_with_options(doc, search_text, flags, FALSE, FALSE);
}


gint search_count_matches(GeanyDocument *doc, const gchar *search_text, GeanyFindFlags flags)
{
	gint count = 0;
	struct Sci_TextToFind ttf;
	GSList *match, *matches;

	g_return_val_if_fail(DOC_VALID(doc), 0);

	if (G_UNLIKELY(EMPTY(search_text)))
		return 0;

	ttf.chrg.cpMin = 0;
	ttf.chrg.cpMax = sci_get_length(doc->editor->sci);
	ttf.lpstrText = (gchar *)search_text;

	matches = find_range(doc->editor->sci, flags, &ttf);
	foreach_slist (match, matches)
	{
		count++;
		geany_match_info_free(match->data);
	}
	g_slist_free(matches);

	return count;
}


static void
on_find_entry_activate(GtkEntry *entry, gpointer user_data)
{
	on_find_dialog_response(NULL, GEANY_RESPONSE_FIND, user_data);
}


static void
on_find_entry_activate_backward(GtkEntry *entry, gpointer user_data)
{
	/* can't search backwards with a regexp */
	if (search_data.flags & GEANY_FIND_REGEXP)
		utils_beep();
	else
		on_find_dialog_response(NULL, GEANY_RESPONSE_FIND_PREVIOUS, user_data);
}


static GeanyFindFlags int_search_flags(gint match_case, gint whole_word, gint regexp,
	gint multiline, gint dot_matches_newline, gint word_start)
{
	return (match_case ? GEANY_FIND_MATCHCASE : 0) |
		(regexp ? GEANY_FIND_REGEXP : 0) |
		(whole_word ? GEANY_FIND_WHOLEWORD : 0) |
		(multiline && regexp ? GEANY_FIND_MULTILINE : 0) |
		(dot_matches_newline && regexp ? GEANY_FIND_DOTALL : 0) |
		/* SCFIND_WORDSTART overrides SCFIND_WHOLEWORD, but we want the opposite */
		(word_start && !whole_word ? GEANY_FIND_WORDSTART : 0);
}


static void
on_find_dialog_response(GtkDialog *dialog, gint response, gpointer user_data)
{
	gtk_window_get_position(GTK_WINDOW(find_dlg.dialog),
		&find_dlg.position[0], &find_dlg.position[1]);

	stash_group_update(find_prefs, find_dlg.dialog);

	if (response == GTK_RESPONSE_CANCEL || response == GTK_RESPONSE_DELETE_EVENT)
		gtk_widget_hide(find_dlg.dialog);
	else
	{
		GeanyDocument *doc = document_get_current();
		gboolean check_close = settings.find_close_dialog;

		if (doc == NULL)
			return;

		search_data.backwards = FALSE;
		search_data.search_bar = FALSE;

		g_free(search_data.text);
		g_free(search_data.original_text);
		search_data.text = g_strdup(gtk_entry_get_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(user_data)))));
		search_data.original_text = g_strdup(search_data.text);
		search_prefs.always_wrap = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(
			ui_lookup_widget(find_dlg.dialog, "check_wrap")));

		search_data.flags = int_search_flags(settings.find_case_sensitive,
			settings.find_match_whole_word, settings.find_regexp, settings.find_regexp_multiline,
			settings.find_regexp_dot_matches_newline, settings.find_match_word_start);

		if (EMPTY(search_data.text))
		{
			fail:
			utils_beep();
			gtk_widget_grab_focus(find_dlg.entry);
			return;
		}
		if (search_data.flags & GEANY_FIND_REGEXP)
		{
			GRegex *regex = compile_regex(search_data.text, search_data.flags);
			if (!regex)
				goto fail;
			else
				g_regex_unref(regex);
		}
		else if (settings.find_escape_sequences)
		{
			if (! utils_str_replace_escape(search_data.text, FALSE))
				goto fail;
		}
		ui_combo_box_add_to_history(GTK_COMBO_BOX_TEXT(user_data), search_data.original_text, 0);

		switch (response)
		{
			case GEANY_RESPONSE_FIND:
			case GEANY_RESPONSE_FIND_PREVIOUS:
			{
				gint result = document_find_text(doc, search_data.text, search_data.original_text, search_data.flags,
					(response == GEANY_RESPONSE_FIND_PREVIOUS), NULL, TRUE);
				ui_set_search_entry_background(find_dlg.entry, (result > -1));
				check_close = search_prefs.hide_find_dialog;
				break;
			}
			case GEANY_RESPONSE_FIND_IN_FILE:
				search_find_usage(search_data.text, search_data.original_text, search_data.flags, FALSE);
				break;

			case GEANY_RESPONSE_FIND_IN_SESSION:
				search_find_usage(search_data.text, search_data.original_text, search_data.flags, TRUE);
				break;

			case GEANY_RESPONSE_MARK:
			{
				gint count = search_mark_all(doc, search_data.text, search_data.flags);

				if (count == 0)
					ui_set_statusbar(FALSE, _("No matches found for \"%s\"."), search_data.original_text);
				else
					ui_set_statusbar(FALSE,
						ngettext("Found %d match for \"%s\".",
								 "Found %d matches for \"%s\".", count),
						count, search_data.original_text);
			}
			break;

			case GEANY_RESPONSE_COUNT:
			{
				gint count = search_count_matches(doc, search_data.text, search_data.flags);

				if (count == 0)
					ui_set_statusbar(FALSE, _("No matches found for \"%s\"."), search_data.original_text);
				else
					ui_set_statusbar(FALSE,
						ngettext("Counted %d match for \"%s\".",
								 "Counted %d matches for \"%s\".", count),
						count, search_data.original_text);
			}
			break;
		}
		if (check_close)
			gtk_widget_hide(find_dlg.dialog);
	}
}


static void
on_replace_find_entry_activate(GtkEntry *entry, gpointer user_data)
{
	on_replace_dialog_response(NULL, GEANY_RESPONSE_FIND, NULL);
}


static void
on_replace_entry_activate(GtkEntry *entry, gpointer user_data)
{
	on_replace_dialog_response(NULL,
		search_prefs.replace_and_find_by_default ? GEANY_RESPONSE_REPLACE_AND_FIND : GEANY_RESPONSE_REPLACE,
		NULL);
}


static void replace_in_session(GeanyDocument *doc,
		GeanyFindFlags search_flags_re, gboolean search_replace_escape_re,
		const gchar *find, const gchar *replace,
		const gchar *original_find, const gchar *original_replace)
{
	guint n, page_count, rep_count = 0, file_count = 0;

	/* replace in all documents following notebook tab order */
	page_count = gtk_notebook_get_n_pages(GTK_NOTEBOOK(main_widgets.notebook));
	for (n = 0; n < page_count; n++)
	{
		GeanyDocument *tmp_doc = document_get_from_page(n);
		gint reps = 0;

		reps = document_replace_all(tmp_doc, find, replace, original_find, original_replace, search_flags_re);
		rep_count += reps;
		if (reps)
			file_count++;
	}
	if (file_count == 0)
	{
		utils_beep();
		ui_set_statusbar(FALSE, _("No matches found for \"%s\"."), original_find);
		return;
	}
	/* if only one file was changed, don't override that document's status message
	 * so we don't have to translate 4 messages for ngettext */
	if (file_count > 1)
		ui_set_statusbar(FALSE, _("Replaced %u matches in %u documents."),
			rep_count, file_count);

	/* show which docs had replacements: */
	gtk_notebook_set_current_page(GTK_NOTEBOOK(msgwindow.notebook), MSG_STATUS);

	ui_save_buttons_toggle(doc->changed);	/* update save all */
}


static void
on_replace_dialog_response(GtkDialog *dialog, gint response, gpointer user_data)
{
	GeanyDocument *doc = document_get_current();
	GeanyFindFlags search_flags_re;
	gboolean search_backwards_re, search_replace_escape_re;
	gchar *find, *replace, *original_find = NULL, *original_replace = NULL;

	gtk_window_get_position(GTK_WINDOW(replace_dlg.dialog),
		&replace_dlg.position[0], &replace_dlg.position[1]);

	stash_group_update(replace_prefs, replace_dlg.dialog);

	if (response == GTK_RESPONSE_CANCEL || response == GTK_RESPONSE_DELETE_EVENT)
	{
		gtk_widget_hide(replace_dlg.dialog);
		return;
	}

	if (response == GEANY_RESPONSE_REPLACE_IN_SESSION) {
		if (!search_prefs.skip_confirmation_for_replace_in_session &&
			! dialogs_show_question_full(replace_dlg.dialog, NULL, NULL,
			_("This operation will modify all open files which contain the text to replace."),
			_("Are you sure to replace in the whole session?"))) {
			return;
		}
	}

	search_backwards_re = settings.replace_search_backwards;
	search_replace_escape_re = settings.replace_escape_sequences;
	find = g_strdup(gtk_entry_get_text(GTK_ENTRY(replace_dlg.find_entry)));
	replace = g_strdup(gtk_entry_get_text(GTK_ENTRY(replace_dlg.replace_entry)));

	search_prefs.always_wrap = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(
		ui_lookup_widget(replace_dlg.dialog, "check_wrap")));

	search_flags_re = int_search_flags(settings.replace_case_sensitive,
		settings.replace_match_whole_word, settings.replace_regexp,
		settings.replace_regexp_multiline, settings.replace_regexp_dot_matches_newline,
		settings.replace_match_word_start);

	if ((response != GEANY_RESPONSE_FIND) && (search_flags_re & GEANY_FIND_MATCHCASE)
		&& (strcmp(find, replace) == 0))
		goto fail;

	original_find = g_strdup(find);
	original_replace = g_strdup(replace);

	if (search_flags_re & GEANY_FIND_REGEXP)
	{
		GRegex *regex = compile_regex(find, search_flags_re);
		if (regex)
			g_regex_unref(regex);
		/* find escapes will be handled by GRegex */
		if (!regex || !utils_str_replace_escape(replace, TRUE))
			goto fail;
	}
	else if (search_replace_escape_re)
	{
		if (! utils_str_replace_escape(find, FALSE) ||
			! utils_str_replace_escape(replace, FALSE))
			goto fail;
	}

	ui_combo_box_add_to_history(GTK_COMBO_BOX_TEXT(replace_dlg.find_combobox), original_find, 0);
	ui_combo_box_add_to_history(GTK_COMBO_BOX_TEXT(replace_dlg.replace_combobox), original_replace, 0);

	switch (response)
	{
		case GEANY_RESPONSE_REPLACE_AND_FIND:
		{
			gint rep = document_replace_text(doc, find, original_find, replace, search_flags_re,
				search_backwards_re);
			if (rep != -1)
				document_find_text(doc, find, original_find, search_flags_re, search_backwards_re,
					NULL, TRUE);
			break;
		}
		case GEANY_RESPONSE_REPLACE:
			document_replace_text(doc, find, original_find, replace, search_flags_re,
				search_backwards_re);
			break;

		case GEANY_RESPONSE_FIND:
		{
			gint result = document_find_text(doc, find, original_find, search_flags_re,
								search_backwards_re, NULL, TRUE);
			ui_set_search_entry_background(replace_dlg.find_entry, (result > -1));
			break;
		}
		case GEANY_RESPONSE_REPLACE_IN_FILE:
			if (! document_replace_all(doc, find, replace, original_find, original_replace, search_flags_re))
				utils_beep();
			break;

		case GEANY_RESPONSE_REPLACE_IN_SESSION:
			replace_in_session(doc, search_flags_re, search_replace_escape_re, find, replace, original_find, original_replace);
			break;

		case GEANY_RESPONSE_REPLACE_IN_SEL:
			document_replace_sel(doc, find, replace, original_find, original_replace, search_flags_re);
			break;
	}
	switch (response)
	{
		case GEANY_RESPONSE_REPLACE_IN_SEL:
		case GEANY_RESPONSE_REPLACE_IN_FILE:
		case GEANY_RESPONSE_REPLACE_IN_SESSION:
			if (settings.replace_close_dialog)
				gtk_widget_hide(replace_dlg.dialog);
	}
	g_free(find);
	g_free(replace);
	g_free(original_find);
	g_free(original_replace);
	return;

fail:
	utils_beep();
	gtk_widget_grab_focus(replace_dlg.find_entry);
	g_free(find);
	g_free(replace);
	g_free(original_find);
	g_free(original_replace);
}


static void search_studio_hide_on_delete(GtkWidget *widget, gpointer user_data)
{
	search_studio_hide_on_delete_cb(studio_dlg.dialog, NULL, NULL);
}


static gboolean search_studio_hide_on_delete_cb(GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
	gtk_window_get_position(GTK_WINDOW(studio_dlg.dialog),
		&studio_dlg.position[0], &studio_dlg.position[1]);
	gtk_widget_hide(studio_dlg.dialog);
	return TRUE;
}


static void search_studio_open_find_dialog_activate(GtkButton *button, gpointer user_data)
{
	search_studio_sync_find_dialog_from_page(GTK_WIDGET(user_data));
}


static void search_studio_open_replace_dialog_activate(GtkButton *button, gpointer user_data)
{
	search_studio_sync_replace_dialog_from_page(GTK_WIDGET(user_data));
}


static void search_studio_open_fif_dialog_activate(GtkButton *button, gpointer user_data)
{
	search_show_find_in_files_dialog(gtk_entry_get_text(GTK_ENTRY(ui_lookup_widget(GTK_WIDGET(user_data), "entry_dir"))));
}


static void search_studio_swap_find_replace(GtkButton *button, gpointer user_data)
{
	GtkWidget *page = GTK_WIDGET(user_data);
	GtkWidget *entry_find = ui_lookup_widget(page, "entry_search");
	GtkWidget *entry_replace = ui_lookup_widget(page, "entry_replace");
	gchar *find_text = g_strdup(gtk_entry_get_text(GTK_ENTRY(entry_find)));
	gchar *replace_text = g_strdup(gtk_entry_get_text(GTK_ENTRY(entry_replace)));

	gtk_entry_set_text(GTK_ENTRY(entry_find), replace_text);
	gtk_entry_set_text(GTK_ENTRY(entry_replace), find_text);
	g_free(find_text);
	g_free(replace_text);
}


static void search_studio_collect_document_hits(GtkButton *button, gpointer user_data)
{
	GtkWidget *page = GTK_WIDGET(user_data);
	GeanyDocument *doc = document_get_current();
	SearchStudioFindSpec spec = { 0 };
	guint count;

	if (!DOC_VALID(doc) || !search_studio_build_find_spec(page, "entry_search", &spec))
		return;

	search_studio_add_find_history(page, &spec);
	count = search_studio_append_match_rows("Find All (Doc)", doc, spec.text, spec.flags, spec.mode, 1000);
	search_studio_activity_append("[Find All] Document | query=%s | mode=%s | matches=%u",
		spec.original_text, spec.mode, count);
	search_studio_append_document_resultf("Find All", doc, spec.original_text, spec.mode,
		"Found %u matches in the active document.", count);
	search_studio_store_find_spec(&spec);
	search_studio_find_spec_clear(&spec);
}


static void search_studio_collect_session_hits(GtkButton *button, gpointer user_data)
{
	GtkWidget *page = GTK_WIDGET(user_data);
	SearchStudioFindSpec spec = { 0 };
	guint count;

	if (!search_studio_build_find_spec(page, "entry_search", &spec))
		return;

	search_studio_add_find_history(page, &spec);
	count = search_studio_append_session_match_rows("Find All (Session)", spec.text, spec.flags, spec.mode, 250);
	search_studio_activity_append("[Find All] Session | query=%s | mode=%s | matches=%u",
		spec.original_text, spec.mode, count);
	search_studio_result_appendf("Find All", "Open Documents", spec.original_text, spec.mode,
		"Found %u matches in the whole session.", count);
	search_studio_store_find_spec(&spec);
	search_studio_find_spec_clear(&spec);
}


static void search_studio_clear_results(GtkButton *button, gpointer user_data)
{
	gtk_list_store_clear(studio_dlg.results_store);
	ui_set_statusbar(FALSE, _("Cleared Search Studio results."));
	search_studio_activity_append("[Studio] Cleared structured results view.");
}


static void search_studio_find_activate(GtkButton *button, gpointer user_data)
{
	GtkWidget *page = GTK_WIDGET(user_data);
	GeanyDocument *doc = document_get_current();
	SearchStudioFindSpec spec = { 0 };
	gint result;

	if (!DOC_VALID(doc) || !search_studio_build_find_spec(page, "entry_search", &spec))
		return;

	search_studio_store_find_spec(&spec);
	search_studio_add_find_history(page, &spec);
	result = document_find_text(doc, spec.text, spec.original_text, spec.flags, spec.backwards, NULL, TRUE);
	ui_set_search_entry_background(ui_lookup_widget(page, "entry_search"), (result > -1));
	search_studio_activity_append("[Find] %s | mode=%s | wrap=%s | result=%s",
		spec.original_text, spec.mode,
		search_prefs.always_wrap ? "on" : "off", result > -1 ? "match found" : "not found");
	search_studio_append_document_result("Find", doc, spec.original_text, spec.mode,
		result > -1 ? "Found next occurrence in current document." : "No occurrence found from the current position.");
	if (result > -1)
		search_studio_append_match_rows("Find Match", doc, spec.text, spec.flags, spec.mode, 1);
	search_studio_find_spec_clear(&spec);
}


static void search_studio_find_next_activate(GtkButton *button, gpointer user_data)
{
	GtkWidget *page = GTK_WIDGET(user_data);
	if (ui_lookup_widget(page, "check_back"))
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ui_lookup_widget(page, "check_back")), FALSE);
	search_studio_find_activate(button, user_data);
}


static void search_studio_find_previous_activate(GtkButton *button, gpointer user_data)
{
	GtkWidget *page = GTK_WIDGET(user_data);
	if (ui_lookup_widget(page, "check_back"))
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ui_lookup_widget(page, "check_back")), TRUE);
	search_studio_find_activate(button, user_data);
}


static void search_studio_count_activate(GtkButton *button, gpointer user_data)
{
	GtkWidget *page = GTK_WIDGET(user_data);
	GeanyDocument *doc = document_get_current();
	SearchStudioFindSpec spec = { 0 };
	gint count;

	if (!DOC_VALID(doc) || !search_studio_build_find_spec(page, "entry_search", &spec))
		return;

	search_studio_add_find_history(page, &spec);
	count = search_count_matches(doc, spec.text, spec.flags);
	search_studio_report_count_status(spec.original_text, count, FALSE, 0);

	search_studio_activity_append("[Count] %s | mode=%s | matches=%d",
		spec.original_text, spec.mode, count);
	search_studio_append_document_resultf("Count", doc, spec.original_text, spec.mode,
		"Counted %d matches in the active document.", count);
	search_studio_append_count_impact_row("Count Impact", doc, spec.text, spec.flags, spec.mode);
	search_studio_append_match_rows("Count Match", doc, spec.text, spec.flags, spec.mode, 50);
	search_studio_store_find_spec(&spec);
	search_studio_find_spec_clear(&spec);
}


static void search_studio_mark_activate(GtkButton *button, gpointer user_data)
{
	GtkWidget *page = GTK_WIDGET(user_data);
	GeanyDocument *doc = document_get_current();
	SearchStudioFindSpec spec = { 0 };
	gboolean bookmark_lines;
	gboolean purge_bookmarks;
	gint count;

	if (!DOC_VALID(doc) || !search_studio_build_find_spec(page, "entry_search", &spec))
		return;

	bookmark_lines = ui_lookup_widget(page, "check_bookmark") &&
		gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ui_lookup_widget(page, "check_bookmark")));
	purge_bookmarks = ui_lookup_widget(page, "check_purge") &&
		gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ui_lookup_widget(page, "check_purge")));
	search_studio_add_find_history(page, &spec);
	count = search_mark_all_with_options(doc, spec.text, spec.flags, bookmark_lines, purge_bookmarks);
	search_studio_report_mark_status(spec.original_text, count, bookmark_lines, FALSE, 0);

	search_studio_activity_append("[Mark] %s | mode=%s | matches=%d | bookmarks=%s | purge=%s",
		spec.original_text, spec.mode, count,
		bookmark_lines ? "on" : "off", purge_bookmarks ? "on" : "off");
	search_studio_append_document_resultf("Mark", doc, spec.original_text, spec.mode,
		"Marked %d matches; bookmark-lines=%s; purge-first=%s.",
		count, bookmark_lines ? "yes" : "no", purge_bookmarks ? "yes" : "no");
	search_studio_append_mark_impact_row("Mark Impact", doc, spec.text, spec.flags,
		spec.mode, bookmark_lines, purge_bookmarks);
	search_studio_append_match_rows("Mark Match", doc, spec.text, spec.flags, spec.mode, 50);
	search_studio_store_find_spec(&spec);
	search_studio_find_spec_clear(&spec);
}


typedef struct SearchStudioCountSessionContext
{
	const gchar *query;
	GeanyFindFlags flags;
	const gchar *mode;
}
SearchStudioCountSessionContext;


static guint search_studio_count_session_cb(GeanyDocument *doc, gpointer user_data)
{
	SearchStudioCountSessionContext *ctx = user_data;

	return search_studio_append_count_impact_row("Session Count Impact", doc, ctx->query,
		ctx->flags, ctx->mode);
}


static void search_studio_count_session_activate(GtkButton *button, gpointer user_data)
{
	GtkWidget *page = GTK_WIDGET(user_data);
	SearchStudioFindSpec spec = { 0 };
	SearchStudioCountSessionContext ctx;
	SearchStudioFindSessionActionSpec action;
	SearchStudioSessionRunResult result;

	if (!search_studio_build_find_spec(page, "entry_search", &spec))
		return;

	ctx.query = spec.text;
	ctx.flags = spec.flags;
	ctx.mode = spec.mode;
	action.find = &spec;
	action.open_document_func = search_studio_count_session_cb;
	action.open_document_data = &ctx;
	action.add_history = TRUE;
	action.store_find_spec = TRUE;
	result = search_studio_execute_find_session_action(page, &action);
	search_studio_report_count_status(spec.original_text, result.total_results, TRUE,
		result.docs_with_results);

	search_studio_activity_append("[Count] Session | query=%s | mode=%s | matches=%u | docs=%u",
		spec.original_text, spec.mode, result.total_results, result.docs_with_results);
	search_studio_result_appendf("Count in Session", "Open Documents", spec.original_text,
		spec.mode, "Counted %u matches across %u open documents.",
		result.total_results, result.docs_with_results);
	search_studio_find_spec_clear(&spec);
}


static void search_studio_clear_marks_activate(GtkButton *button, gpointer user_data)
{
	GeanyDocument *doc = document_get_current();

	if (!DOC_VALID(doc))
		return;

	search_clear_all_marks(doc);
	ui_set_statusbar(FALSE, _("Cleared search highlights and bookmarks."));
	search_studio_activity_append("[Mark] Cleared search highlights and bookmarks.");
	search_studio_append_document_result("Mark", doc, "Clear", "N/A",
		"Cleared search highlights and bookmarked lines.");
}


static void search_studio_copy_marked_lines_activate(GtkButton *button, gpointer user_data)
{
	GtkWidget *page = GTK_WIDGET(user_data);
	GeanyDocument *doc = document_get_current();
	const gchar *query;
	guint count;

	if (!DOC_VALID(doc))
		return;

	query = gtk_entry_get_text(GTK_ENTRY(ui_lookup_widget(page, "entry_search")));
	count = search_studio_copy_marked_lines_to_clipboard(doc);
	if (count == 0)
	{
		utils_beep();
		ui_set_statusbar(FALSE, _("No marked lines available to copy."));
		search_studio_activity_append("[Mark] Copy marked lines requested, but no marked lines were available.");
		search_studio_append_document_result("Copy Marked Lines", doc,
			EMPTY(query) ? "Current marks" : query,
			search_studio_mode_name(page),
			"No marked lines were available to copy.");
		return;
	}

	ui_set_statusbar(FALSE, _("Copied %u marked lines to the clipboard."), count);
	search_studio_activity_append("[Mark] Copied %u marked lines to the clipboard.", count);
	search_studio_append_document_resultf("Copy Marked Lines", doc,
		EMPTY(query) ? "Current marks" : query,
		search_studio_mode_name(page),
		"Copied %u marked lines to the clipboard.", count);
}


static void search_studio_cut_marked_lines_activate(GtkButton *button, gpointer user_data)
{
	GtkWidget *page = GTK_WIDGET(user_data);
	GeanyDocument *doc = document_get_current();
	const gchar *query;
	guint copied_count;
	guint deleted_count;

	if (!DOC_VALID(doc))
		return;

	query = gtk_entry_get_text(GTK_ENTRY(ui_lookup_widget(page, "entry_search")));
	copied_count = search_studio_copy_marked_lines_to_clipboard(doc);
	if (copied_count == 0)
	{
		utils_beep();
		ui_set_statusbar(FALSE, _("No marked lines available to cut."));
		search_studio_activity_append("[Mark] Cut marked lines requested, but no marked lines were available.");
		search_studio_append_document_result("Cut Marked Lines", doc,
			EMPTY(query) ? "Current marks" : query,
			search_studio_mode_name(page),
			"No marked lines were available to cut.");
		return;
	}

	deleted_count = search_studio_delete_marked_lines(doc, TRUE);
	ui_set_statusbar(FALSE, _("Cut %u marked lines to the clipboard."), deleted_count);
	search_studio_activity_append("[Mark] Cut %u marked lines to the clipboard.", deleted_count);
	search_studio_append_document_resultf("Cut Marked Lines", doc,
		EMPTY(query) ? "Current marks" : query,
		search_studio_mode_name(page),
		"Copied and removed %u marked lines from the active document.", deleted_count);
}


static void search_studio_delete_marked_lines_activate(GtkButton *button, gpointer user_data)
{
	GtkWidget *page = GTK_WIDGET(user_data);
	GeanyDocument *doc = document_get_current();
	const gchar *query;
	guint count;

	if (!DOC_VALID(doc))
		return;

	query = gtk_entry_get_text(GTK_ENTRY(ui_lookup_widget(page, "entry_search")));
	count = search_studio_delete_marked_lines(doc, TRUE);
	if (count == 0)
	{
		utils_beep();
		ui_set_statusbar(FALSE, _("No marked lines available to delete."));
		search_studio_activity_append("[Mark] Delete marked lines requested, but no marked lines were available.");
		search_studio_append_document_result("Delete Marked Lines", doc,
			EMPTY(query) ? "Current marks" : query,
			search_studio_mode_name(page),
			"No marked lines were available to delete.");
		return;
	}

	ui_set_statusbar(FALSE, _("Deleted %u marked lines from the active document."), count);
	search_studio_activity_append("[Mark] Deleted %u marked lines from the active document.", count);
	search_studio_append_document_resultf("Delete Marked Lines", doc,
		EMPTY(query) ? "Current marks" : query,
		search_studio_mode_name(page),
		"Deleted %u marked lines from the active document.", count);
}


static void search_studio_delete_unmarked_lines_activate(GtkButton *button, gpointer user_data)
{
	GtkWidget *page = GTK_WIDGET(user_data);
	GeanyDocument *doc = document_get_current();
	const gchar *query;
	guint count;

	if (!DOC_VALID(doc))
		return;

	query = gtk_entry_get_text(GTK_ENTRY(ui_lookup_widget(page, "entry_search")));
	count = search_studio_delete_marked_lines(doc, FALSE);
	if (count == 0)
	{
		utils_beep();
		ui_set_statusbar(FALSE, _("No unmarked lines were available to delete."));
		search_studio_activity_append("[Mark] Delete unmarked lines requested, but every line was already marked.");
		search_studio_append_document_result("Delete Unmarked Lines", doc,
			EMPTY(query) ? "Current marks" : query,
			search_studio_mode_name(page),
			"No unmarked lines were available to delete.");
		return;
	}

	ui_set_statusbar(FALSE, _("Deleted %u unmarked lines from the active document."), count);
	search_studio_activity_append("[Mark] Deleted %u unmarked lines from the active document.", count);
	search_studio_append_document_resultf("Delete Unmarked Lines", doc,
		EMPTY(query) ? "Current marks" : query,
		search_studio_mode_name(page),
		"Deleted %u unmarked lines from the active document.", count);
}


static void search_studio_inverse_marks_activate(GtkButton *button, gpointer user_data)
{
	GtkWidget *page = GTK_WIDGET(user_data);
	GeanyDocument *doc = document_get_current();
	const gchar *query;
	guint count;

	if (!DOC_VALID(doc))
		return;

	query = gtk_entry_get_text(GTK_ENTRY(ui_lookup_widget(page, "entry_search")));
	count = search_studio_inverse_marked_lines(doc);
	if (count == 0 && sci_get_line_count(doc->editor->sci) == 0)
	{
		utils_beep();
		ui_set_statusbar(FALSE, _("No lines were available to invert."));
		search_studio_activity_append("[Mark] Inverse marks requested, but the active document has no lines.");
		search_studio_append_document_result("Inverse Marks", doc,
			EMPTY(query) ? "Current marks" : query,
			search_studio_mode_name(page),
			"No lines were available to invert.");
		return;
	}

	ui_set_statusbar(FALSE, _("Inverted marked-line coverage; %u lines are now marked."), count);
	search_studio_activity_append("[Mark] Inverted marked-line coverage; %u lines are now marked.", count);
	search_studio_append_document_resultf("Inverse Marks", doc,
		EMPTY(query) ? "Current marks" : query,
		search_studio_mode_name(page),
		"Inverted current marked-line coverage; %u lines are now marked/bookmarked.", count);
}


typedef struct SearchStudioMarkSessionContext
{
	const gchar *query;
	GeanyFindFlags flags;
	const gchar *mode;
	gboolean bookmark_lines;
	gboolean purge_bookmarks;
}
SearchStudioMarkSessionContext;


static guint search_studio_mark_session_cb(GeanyDocument *doc, gpointer user_data)
{
	SearchStudioMarkSessionContext *ctx = user_data;
	guint count;

	count = search_mark_all_with_options(doc, ctx->query, ctx->flags,
		ctx->bookmark_lines, ctx->purge_bookmarks);
	if (count > 0)
		search_studio_append_mark_impact_row("Session Mark Impact", doc, ctx->query, ctx->flags,
			ctx->mode, ctx->bookmark_lines, ctx->purge_bookmarks);
	return count;
}


static void search_studio_mark_session_activate(GtkButton *button, gpointer user_data)
{
	GtkWidget *page = GTK_WIDGET(user_data);
	SearchStudioFindSpec spec = { 0 };
	SearchStudioMarkSessionContext ctx;
	SearchStudioFindSessionActionSpec action;
	SearchStudioSessionRunResult result;

	if (!search_studio_build_find_spec(page, "entry_search", &spec))
		return;

	ctx.bookmark_lines = ui_lookup_widget(page, "check_bookmark") &&
		gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ui_lookup_widget(page, "check_bookmark")));
	ctx.purge_bookmarks = ui_lookup_widget(page, "check_purge") &&
		gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ui_lookup_widget(page, "check_purge")));
	ctx.query = spec.text;
	ctx.flags = spec.flags;
	ctx.mode = spec.mode;
	action.find = &spec;
	action.open_document_func = search_studio_mark_session_cb;
	action.open_document_data = &ctx;
	action.add_history = TRUE;
	action.store_find_spec = TRUE;
	result = search_studio_execute_find_session_action(page, &action);
	search_studio_report_mark_status(spec.original_text, result.total_results, ctx.bookmark_lines,
		TRUE, result.docs_with_results);

	search_studio_activity_append("[Mark] Session | query=%s | mode=%s | matches=%u | docs=%u | bookmarks=%s | purge=%s",
		spec.original_text, spec.mode, result.total_results, result.docs_with_results,
		ctx.bookmark_lines ? "on" : "off", ctx.purge_bookmarks ? "on" : "off");
	search_studio_result_appendf("Mark in Session", "Open Documents", spec.original_text,
		spec.mode, "Marked %u matches across %u open documents; bookmark-lines=%s; purge-first=%s.",
		result.total_results, result.docs_with_results, ctx.bookmark_lines ? "yes" : "no", ctx.purge_bookmarks ? "yes" : "no");
	search_studio_find_spec_clear(&spec);
}


static guint search_studio_clear_session_marks_cb(GeanyDocument *doc, gpointer user_data)
{
	(void) user_data;
	search_clear_all_marks(doc);
	return 1;
}


static void search_studio_clear_session_marks_activate(GtkButton *button, gpointer user_data)
{
	SearchStudioSessionRunResult result = search_studio_run_session_action(
		search_studio_clear_session_marks_cb, NULL);

	ui_set_statusbar(FALSE, _("Cleared search highlights and bookmarks across %u open documents."),
		result.total_results);
	search_studio_activity_append("[Mark] Cleared search highlights and bookmarks across %u open documents.",
		result.total_results);
	search_studio_result_appendf("Mark in Session", "Open Documents", "Clear", "N/A",
		"Cleared search highlights and bookmarks across %u open documents.", result.total_results);
}


static void search_studio_open_find_dialog_activate(GtkButton *button, gpointer user_data)
{
	search_studio_sync_find_dialog_from_page(GTK_WIDGET(user_data));
	search_studio_activity_append("[Find] Opened classic Find dialog with synchronized state.");
	search_studio_result_append("Find", "Classic dialog", gtk_entry_get_text(GTK_ENTRY(ui_lookup_widget(GTK_WIDGET(user_data), "entry_search"))),
		search_studio_mode_name(GTK_WIDGET(user_data)), "Opened classic Find dialog with synchronized state.");
}


static void search_studio_open_replace_dialog_activate(GtkButton *button, gpointer user_data)
{
	search_studio_sync_replace_dialog_from_page(GTK_WIDGET(user_data));
	search_studio_activity_append("[Replace] Opened classic Replace dialog with synchronized state.");
	search_studio_result_append("Replace", "Classic dialog", gtk_entry_get_text(GTK_ENTRY(ui_lookup_widget(GTK_WIDGET(user_data), "entry_search"))),
		search_studio_mode_name(GTK_WIDGET(user_data)), "Opened classic Replace dialog with synchronized state.");
}


static void search_studio_open_fif_dialog_activate(GtkButton *button, gpointer user_data)
{
	GtkWidget *page = GTK_WIDGET(user_data);
	const gchar *text = gtk_entry_get_text(GTK_ENTRY(ui_lookup_widget(page, "entry_search")));
	const gchar *dir = gtk_entry_get_text(GTK_ENTRY(ui_lookup_widget(page, "entry_dir")));
	search_show_find_in_files_dialog_full(EMPTY(text) ? NULL : text, EMPTY(dir) ? NULL : dir);
	search_studio_activity_append("[Find in Files] Opened classic Find in Files dialog with synchronized text/directory.");
	search_studio_result_append("Find in Files", "Classic dialog", EMPTY(text) ? "(empty)" : text,
		search_studio_mode_name(page), "Opened classic Find in Files dialog with synchronized text/directory.");
}


static void search_studio_replace_preview_document(GtkButton *button, gpointer user_data)
{
	GtkWidget *page = GTK_WIDGET(user_data);
	GeanyDocument *doc = document_get_current();
	SearchStudioReplaceSpec spec = { 0 };
	guint count;

	if (!DOC_VALID(doc) || !search_studio_build_replace_spec(page, &spec))
		return;

	count = search_studio_append_replace_preview_rows(doc, spec.find.text, spec.find.flags,
		spec.find.mode, spec.replace, spec.original_replace, 250);
	search_studio_activity_append("[Replace Preview] Document | find=%s | replace=%s | matches=%u | mode=%s",
		spec.find.original_text, spec.original_replace, count, spec.find.mode);
	search_studio_append_document_resultf("Replace Preview", doc, spec.find.original_text,
		spec.find.mode, "Would replace %u matches in the active document.", count);
	search_studio_replace_spec_clear(&spec);
}


static void search_studio_replace_preview_session(GtkButton *button, gpointer user_data)
{
	GtkWidget *page = GTK_WIDGET(user_data);
	SearchStudioReplaceSpec spec = { 0 };
	SearchStudioReplaceSessionRowsActionContext ctx;
	SearchStudioSessionRunResult result;

	if (!search_studio_build_replace_spec(page, &spec))
		return;

	ctx.kind = SEARCH_STUDIO_REPLACE_SESSION_ROWS_PREVIEW;
	ctx.action = "Replace Preview";
	ctx.query = spec.find.text;
	ctx.flags = spec.find.flags;
	ctx.mode = spec.find.mode;
	ctx.replace_text = spec.replace;
	ctx.replace_display = spec.original_replace;
	ctx.per_doc_limit = 100;
	result = search_studio_execute_replace_session_rows_action(page, &spec, &ctx,
		FALSE, FALSE);
	search_studio_activity_append("[Replace Preview] Session | find=%s | replace=%s | matches=%u | mode=%s",
		spec.find.original_text, spec.original_replace, result.total_results, spec.find.mode);
	search_studio_result_appendf("Replace Preview Session", "Open Documents", spec.find.original_text,
		spec.find.mode, "Would replace %u matches across open documents.", result.total_results);
	search_studio_replace_spec_clear(&spec);
}


static void search_studio_swap_find_replace(GtkButton *button, gpointer user_data)
{
	GtkWidget *page = GTK_WIDGET(user_data);
	GtkEntry *entry_find = GTK_ENTRY(ui_lookup_widget(page, "entry_search"));
	GtkEntry *entry_replace = GTK_ENTRY(ui_lookup_widget(page, "entry_replace"));
	const gchar *text_find = gtk_entry_get_text(entry_find);
	const gchar *text_replace = gtk_entry_get_text(entry_replace);
	gchar *tmp = g_strdup(text_find);

	gtk_entry_set_text(entry_find, text_replace);
	gtk_entry_set_text(entry_replace, tmp);
	g_free(tmp);
}


static void search_studio_replace_action_activate(GtkButton *button, gpointer user_data)
{
	GtkWidget *page = GTK_WIDGET(user_data);
	GeanyDocument *doc = document_get_current();
	SearchStudioReplaceSpec spec = { 0 };
	gint action = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(button), "studio-replace-action"));

	if (!DOC_VALID(doc) || !search_studio_build_replace_spec(page, &spec))
		return;

	{
		SearchStudioReplaceSessionActionSpec action_spec = { 0 };
		action_spec.replace = &spec;
		action_spec.add_history = TRUE;
		action_spec.store_find_spec = TRUE;
		search_studio_execute_replace_session_action(page, &action_spec);
	}

	switch (action)
	{
		case GEANY_RESPONSE_FIND:
		{
			gint result = document_find_text(doc, spec.find.text, spec.find.original_text,
				spec.find.flags, spec.find.backwards, NULL, TRUE);
			ui_set_search_entry_background(ui_lookup_widget(page, "entry_search"), (result > -1));
			search_studio_activity_append("[Replace] Find next for %s | mode=%s | result=%s",
				spec.find.original_text, spec.find.mode, result > -1 ? "match found" : "not found");
			search_studio_append_document_result("Replace/Find", doc, spec.find.original_text, spec.find.mode,
				result > -1 ? "Found next match from Replace tab." : "No further match found from Replace tab.");
			if (result > -1)
				search_studio_append_match_rows("Replace Match", doc, spec.find.text, spec.find.flags, spec.find.mode, 1);
			break;
		}
		case GEANY_RESPONSE_REPLACE:
		{
			gint rep = document_replace_text(doc, spec.find.text, spec.find.original_text,
				spec.replace, spec.find.flags, spec.find.backwards);
			search_studio_activity_append("[Replace] Single replace | find=%s | replace=%s | mode=%s | result=%s",
				spec.find.original_text, spec.original_replace, spec.find.mode, rep != -1 ? "changed" : "no change");
			search_studio_append_document_result("Replace", doc, spec.find.original_text, spec.find.mode,
				rep != -1 ? "Replaced current match in the active document." : "No current match to replace.");
			break;
		}
		case GEANY_RESPONSE_REPLACE_AND_FIND:
		{
			gint rep = document_replace_text(doc, spec.find.text, spec.find.original_text,
				spec.replace, spec.find.flags, spec.find.backwards);
			if (rep != -1)
				document_find_text(doc, spec.find.text, spec.find.original_text,
					spec.find.flags, spec.find.backwards, NULL, TRUE);
			search_studio_activity_append("[Replace] Replace & Find | find=%s | replace=%s | mode=%s | result=%s",
				spec.find.original_text, spec.original_replace, spec.find.mode, rep != -1 ? "changed" : "no change");
			search_studio_append_document_result("Replace & Find", doc, spec.find.original_text, spec.find.mode,
				rep != -1 ? "Replaced current match and advanced to the next one." : "No current match to replace before advancing.");
			break;
		}
		case GEANY_RESPONSE_REPLACE_IN_FILE:
		{
			SearchStudioReplaceExecutionResult execution =
				search_studio_execute_replace_document(doc, &spec);
			if (!execution.applied_matches)
				utils_beep();
			search_studio_activity_append("[Replace] Replace in document | find=%s | replace=%s | replacements=%u | planned=%u",
				spec.find.original_text, spec.original_replace, execution.applied_matches,
				execution.plan.planned_matches);
			search_studio_append_document_resultf("Replace in Document", doc, spec.find.original_text,
				spec.find.mode,
				"Replaced %u matches in the active document (planned hits: %u).",
				execution.applied_matches, execution.plan.planned_matches);
			break;
		}
		case GEANY_RESPONSE_REPLACE_IN_SEL:
		{
			document_replace_sel(doc, spec.find.text, spec.replace, spec.find.original_text,
				spec.original_replace, spec.find.flags);
			search_studio_activity_append("[Replace] Replace in selection | find=%s | replace=%s | mode=%s",
				spec.find.original_text, spec.original_replace, spec.find.mode);
			search_studio_append_document_result("Replace in Selection", doc, spec.find.original_text, spec.find.mode,
				"Applied replace-all to the current selection.");
			break;
		}
		case GEANY_RESPONSE_REPLACE_IN_SESSION:
			if (!search_prefs.skip_confirmation_for_replace_in_session &&
				!dialogs_show_question_full(studio_dlg.dialog, NULL, NULL,
				_("This operation will modify all open files which contain the text to replace."),
				_("Are you sure to replace in the whole session?")))
				break;
			{
				SearchStudioReplaceExecutionResult execution =
					search_studio_execute_replace_session(page, doc, &spec);

				search_studio_activity_append("[Replace] Replace in session | find=%s | replace=%s | mode=%s | planned-docs=%u | planned-matches=%u | applied-docs=%u | applied-matches=%u",
					spec.find.original_text, spec.original_replace, spec.find.mode,
					execution.plan.planned_docs, execution.plan.planned_matches,
					execution.affected_docs, execution.applied_matches);
				search_studio_result_appendf("Replace in Session", "Session", spec.find.original_text, spec.find.mode,
					"Applied replacement across open documents (planned docs: %u, planned matches: %u; applied docs: %u, applied matches: %u).",
					execution.plan.planned_docs, execution.plan.planned_matches,
					execution.affected_docs, execution.applied_matches);
			}
			break;
	}

	search_studio_replace_spec_clear(&spec);
}


static void search_studio_fif_file_mode_changed(GtkComboBox *combo, gpointer user_data)
{
	GtkWidget *page = GTK_WIDGET(user_data);
	GtkWidget *entry = ui_lookup_widget(page, "entry_files");
	gint selection = gtk_combo_box_get_active(combo);

	if (selection == FILES_MODE_ALL)
	{
		gtk_entry_set_text(GTK_ENTRY(entry), "");
		gtk_widget_set_sensitive(entry, FALSE);
	}
	else if (selection == FILES_MODE_PROJECT)
	if (app->project && !EMPTY(app->project->file_patterns))
	{
		gchar *patterns = g_strjoinv(" ", app->project->file_patterns);
		gtk_entry_set_text(GTK_ENTRY(entry), patterns);
		g_free(patterns);
	}
	else
		gtk_entry_set_text(GTK_ENTRY(entry), "");
	gtk_widget_set_sensitive(entry, FALSE);
}


static void search_studio_fif_replace_activate(GtkButton *button, gpointer user_data)
{
	GtkWidget *page = GTK_WIDGET(user_data);
	const gchar *find_text = gtk_entry_get_text(GTK_ENTRY(ui_lookup_widget(page, "entry_search")));
	const gchar *replace_text = gtk_entry_get_text(GTK_ENTRY(ui_lookup_widget(page, "entry_replace")));
	const gchar *dir = gtk_entry_get_text(GTK_ENTRY(ui_lookup_widget(page, "entry_dir")));

	if (EMPTY(find_text))
	{
		utils_beep();
		return;
	}

	if (dialogs_show_question_full(NULL, NULL, NULL,
		_("This operation will modify multiple files in the directory \"%s\"."),
		_("Are you sure you want to Replace in Files?"), dir))
	{
		/* Implement actual multi-file replace here or open classic dialog with pre-filled values */
		search_studio_open_fif_dialog_activate(NULL, page);
	}
}


static void search_studio_fif_find_activate(GtkButton *button, gpointer user_data)
{
	GtkWidget *page = GTK_WIDGET(user_data);
	GtkWidget *search_entry = ui_lookup_widget(page, "entry_search");
	GtkWidget *dir_entry = ui_lookup_widget(page, "entry_dir");
	GtkWidget *files_entry = ui_lookup_widget(page, "entry_files");
	GtkWidget *mode_combo = ui_lookup_widget(page, "combo_files_mode");
	GtkWidget *enc_combo = ui_lookup_widget(page, "combo_encoding");
	gboolean regexp;
	gboolean escape_sequences;
	gchar *search_text = g_strdup(gtk_entry_get_text(GTK_ENTRY(search_entry)));
	const gchar *utf8_dir = gtk_entry_get_text(GTK_ENTRY(dir_entry));
	const gchar *files = gtk_entry_get_text(GTK_ENTRY(files_entry));
	gboolean invert = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ui_lookup_widget(page, "check_invert")));
	gboolean case_sensitive = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ui_lookup_widget(page, "check_case")));
	gboolean whole_word = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ui_lookup_widget(page, "check_word")));
	gboolean recursive = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ui_lookup_widget(page, "check_recursive")));
	gboolean hidden_folders = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ui_lookup_widget(page, "check_hidden")));
	gboolean use_extra = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ui_lookup_widget(page, "check_extra")));
	const gchar *extra_options = gtk_entry_get_text(GTK_ENTRY(ui_lookup_widget(page, "entry_extra")));
	gint files_mode = gtk_combo_box_get_active(GTK_COMBO_BOX(mode_combo));
	GeanyEncodingIndex enc_idx = ui_encodings_combo_box_get_active_encoding(GTK_COMBO_BOX(enc_combo));

	search_studio_get_mode(page, &regexp, &escape_sequences);
	if (!regexp && escape_sequences && !utils_str_replace_escape(search_text, FALSE))
	{
		utils_beep();
		gtk_widget_grab_focus(search_entry);
		g_free(search_text);
		return;
	}

	if (execute_find_in_files_request(search_text, utf8_dir, invert, case_sensitive,
			whole_word, recursive, hidden_folders, regexp, use_extra, extra_options, files_mode, files,
			enc_idx, search_entry, dir_entry, files_entry, TRUE, search_studio_mode_name(page)))
	{
		ui_set_search_entry_background(search_entry, TRUE);
		ui_set_search_entry_background(dir_entry, TRUE);
		ui_set_search_entry_background(files_entry, TRUE);
		ui_set_statusbar(FALSE, _("Searching in files started from Search Studio."));
		search_studio_activity_append("[Find in Files] text=%s | dir=%s | mode=%s | recursive=%s | hidden=%s | case=%s | whole-word=%s | invert=%s | files-mode=%d",
			search_text, utf8_dir, search_studio_mode_name(page), recursive ? "on" : "off",
			hidden_folders ? "on" : "off",
			case_sensitive ? "on" : "off", whole_word ? "on" : "off",
			invert ? "on" : "off", files_mode);
		{
			gchar *summary = g_strdup_printf("Launched directory search in %s (recursive=%s, files-mode=%d).",
				utf8_dir, recursive ? "yes" : "no", files_mode);
			search_studio_result_append("Find in Files", utf8_dir, search_text, search_studio_mode_name(page), summary);
			g_free(summary);
		}
	}
	else
		utils_beep();

	g_free(search_text);
}


static void search_studio_find_in_project_activate(GtkButton *button, gpointer user_data)
{
	GtkWidget *page = GTK_WIDGET(user_data);
	GtkWidget *search_entry = ui_lookup_widget(page, "entry_search");
	GtkWidget *files_entry = ui_lookup_widget(page, "entry_files");
	GtkWidget *enc_combo = ui_lookup_widget(page, "combo_encoding");
	GtkWidget *mode_combo = ui_lookup_widget(page, "combo_files_mode");
	gboolean regexp;
	gboolean escape_sequences;
	gchar *search_text;
	gchar *project_dir;
	gchar *project_patterns = NULL;
	gboolean invert;
	gboolean case_sensitive;
	gboolean whole_word;
	gboolean recursive;
	gboolean hidden_folders;
	gboolean use_extra;
	const gchar *extra_options;
	GeanyEncodingIndex enc_idx;

	if (app->project == NULL || EMPTY(app->project->base_path))
	{
		utils_beep();
		ui_set_statusbar(FALSE, _("No project is open for Find in Projects."));
		search_studio_activity_append("[Find in Projects] No project is open; project search was not started.");
		search_studio_result_append("Find in Projects", "Project", "(none)",
			search_studio_mode_name(page), "No project is open for project-wide search.");
		return;
	}

	search_text = g_strdup(gtk_entry_get_text(GTK_ENTRY(search_entry)));
	search_studio_get_mode(page, &regexp, &escape_sequences);
	if (!regexp && escape_sequences && !utils_str_replace_escape(search_text, FALSE))
	{
		utils_beep();
		gtk_widget_grab_focus(search_entry);
		g_free(search_text);
		return;
	}

	project_dir = project_get_base_path();
	if (app->project->file_patterns != NULL && !EMPTY(app->project->file_patterns[0]))
		project_patterns = g_strjoinv(" ", app->project->file_patterns);
	else
		project_patterns = g_strdup(gtk_entry_get_text(GTK_ENTRY(files_entry)));

	invert = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ui_lookup_widget(page, "check_invert")));
	case_sensitive = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ui_lookup_widget(page, "check_case")));
	whole_word = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ui_lookup_widget(page, "check_word")));
	recursive = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ui_lookup_widget(page, "check_recursive")));
	hidden_folders = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ui_lookup_widget(page, "check_hidden")));
	use_extra = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ui_lookup_widget(page, "check_extra")));
	extra_options = gtk_entry_get_text(GTK_ENTRY(ui_lookup_widget(page, "entry_extra")));
	enc_idx = ui_encodings_combo_box_get_active_encoding(GTK_COMBO_BOX(enc_combo));

	if (ui_lookup_widget(page, "entry_dir") != NULL)
		gtk_entry_set_text(GTK_ENTRY(ui_lookup_widget(page, "entry_dir")), project_dir);
	if (ui_lookup_widget(page, "entry_files") != NULL)
		gtk_entry_set_text(GTK_ENTRY(ui_lookup_widget(page, "entry_files")), project_patterns != NULL ? project_patterns : "");
	if (mode_combo != NULL)
		gtk_combo_box_set_active(GTK_COMBO_BOX(mode_combo), FILES_MODE_PROJECT);

	if (execute_find_in_files_request(search_text, project_dir, invert, case_sensitive,
			whole_word, recursive, hidden_folders, regexp, use_extra, extra_options, FILES_MODE_PROJECT,
			project_patterns, enc_idx, search_entry, NULL, files_entry, TRUE,
			search_studio_mode_name(page)))
	{
		ui_set_search_entry_background(search_entry, TRUE);
		ui_set_statusbar(FALSE, _("Searching in project started from Search Studio."));
		search_studio_activity_append("[Find in Projects] text=%s | project=%s | mode=%s | recursive=%s | hidden=%s | case=%s | whole-word=%s",
			search_text, project_dir, search_studio_mode_name(page), recursive ? "on" : "off",
			hidden_folders ? "on" : "off",
			case_sensitive ? "on" : "off", whole_word ? "on" : "off");
		{
			gchar *summary = g_strdup_printf("Launched project search in %s using project file patterns.", project_dir);
			search_studio_result_append("Find in Projects", project_dir, search_text, search_studio_mode_name(page), summary);
			g_free(summary);
		}
	}
	else
		utils_beep();

	g_free(project_patterns);
	g_free(project_dir);
	g_free(search_text);
}


static void search_studio_browse_dir_activate(GtkButton *button, gpointer user_data)
{
	GtkWidget *page = GTK_WIDGET(user_data);
	GtkWidget *entry = ui_lookup_widget(page, "entry_dir");
	GtkWidget *dialog;
	const gchar *current_dir;

	if (entry == NULL)
		return;

	dialog = gtk_file_chooser_dialog_new(_("Select Search Directory"),
		GTK_WINDOW(studio_dlg.dialog), GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
		NULL);
	current_dir = gtk_entry_get_text(GTK_ENTRY(entry));
	if (!EMPTY(current_dir))
		gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), current_dir);

	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
	{
		gchar *dir = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
		gchar *utf8_dir = utils_get_utf8_from_locale(dir);
		gtk_entry_set_text(GTK_ENTRY(entry), utf8_dir);
		ui_set_statusbar(FALSE, _("Search directory selected from the browser."));
		search_studio_activity_append("[Find in Files] Directory selected from browser: %s", utf8_dir);
		search_studio_result_append("Find in Files Setup", utf8_dir, "(directory)",
			search_studio_mode_name(page), "Set search root from the directory browser.");
		g_free(utf8_dir);
		g_free(dir);
	}
	gtk_widget_destroy(dialog);
}


static void search_studio_use_current_doc_dir_activate(GtkButton *button, gpointer user_data)
{
	GtkWidget *page = GTK_WIDGET(user_data);
	gchar *dir = utils_get_current_file_dir_utf8();

	if (EMPTY(dir))
	{
		utils_beep();
		ui_set_statusbar(FALSE, _("No current document directory is available."));
		search_studio_activity_append("[Find in Files] Current document directory shortcut requested, but no current file directory was available.");
		g_free(dir);
		return;
	}

	gtk_entry_set_text(GTK_ENTRY(ui_lookup_widget(page, "entry_dir")), dir);
	ui_set_statusbar(FALSE, _("Find in Files directory set from the current document."));
	search_studio_activity_append("[Find in Files] Directory set from current document: %s", dir);
	search_studio_result_append("Find in Files Setup", dir, "(directory)",
		search_studio_mode_name(page), "Set search root from the current document directory.");
	g_free(dir);
}


static void search_studio_use_project_dir_activate(GtkButton *button, gpointer user_data)
{
	GtkWidget *page = GTK_WIDGET(user_data);
	gchar *dir;

	if (app->project == NULL || EMPTY(app->project->base_path))
	{
		utils_beep();
		ui_set_statusbar(FALSE, _("No project base path is available."));
		search_studio_activity_append("[Find in Files] Project directory shortcut requested, but no project is open.");
		search_studio_result_append("Find in Files Setup", "Project", "(directory)",
			search_studio_mode_name(page), "No project is open for a project-directory shortcut.");
		return;
	}

	dir = project_get_base_path();
	gtk_entry_set_text(GTK_ENTRY(ui_lookup_widget(page, "entry_dir")), dir);
	if (ui_lookup_widget(page, "combo_files_mode") != NULL)
		gtk_combo_box_set_active(GTK_COMBO_BOX(ui_lookup_widget(page, "combo_files_mode")), FILES_MODE_PROJECT);
	ui_set_statusbar(FALSE, _("Find in Files directory set from the current project."));
	search_studio_activity_append("[Find in Files] Directory set from project base path: %s", dir);
	search_studio_result_append("Find in Files Setup", dir, "(directory)",
		search_studio_mode_name(page), "Set search root from the current project base path.");
	g_free(dir);
}


static GtkWidget *search_studio_create_find_page(void)
{
	GtkWidget *page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
	GtkWidget *main_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
	GtkWidget *left_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
	GtkWidget *right_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
	GtkWidget *label = gtk_label_new_with_mnemonic(_("Find _what :"));
	GtkWidget *entry = gtk_combo_box_text_new_with_entry();
	GtkWidget *row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);

	ui_hookup_widget(page, entry, "combo_search");
	ui_hookup_widget(page, gtk_bin_get_child(GTK_BIN(entry)), "entry_search");
	ui_entry_add_clear_icon(GTK_ENTRY(ui_lookup_widget(page, "entry_search")));
	gtk_label_set_mnemonic_widget(GTK_LABEL(label), entry);
	gtk_box_pack_start(GTK_BOX(row), label, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(row), entry, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(left_vbox), row, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(left_vbox), search_studio_create_common_options(page, TRUE, TRUE), FALSE, FALSE, 0);

	button = ui_button_new_with_image(GTK_STOCK_FIND, _("Find _Next"));
	gtk_widget_set_size_request(button, 180, -1);
	gtk_box_pack_start(GTK_BOX(right_vbox), button, FALSE, FALSE, 0);
	g_signal_connect(button, "clicked", G_CALLBACK(search_studio_find_next_activate), page);
	
	button = ui_button_new_with_image(GTK_STOCK_GO_BACK, _("Find _Previous"));
	gtk_widget_set_size_request(button, 180, -1);
	gtk_box_pack_start(GTK_BOX(right_vbox), button, FALSE, FALSE, 0);
	g_signal_connect(button, "clicked", G_CALLBACK(search_studio_find_previous_activate), page);

	button = gtk_button_new_with_mnemonic(_("Fi_nd All in All Opened Docs"));
	gtk_widget_set_size_request(button, 180, -1);
	gtk_box_pack_start(GTK_BOX(right_vbox), button, FALSE, FALSE, 0);
	g_signal_connect(button, "clicked", G_CALLBACK(search_studio_collect_session_hits), page);

	button = gtk_button_new_with_mnemonic(_("Find All in _Current Document"));
	gtk_widget_set_size_request(button, 180, -1);
	gtk_box_pack_start(GTK_BOX(right_vbox), button, FALSE, FALSE, 0);
	g_signal_connect(button, "clicked", G_CALLBACK(search_studio_collect_document_hits), page);

	button = gtk_button_new_with_mnemonic(_("Co_unt"));
	gtk_widget_set_size_request(button, 180, -1);
	gtk_box_pack_start(GTK_BOX(right_vbox), button, FALSE, FALSE, 0);
	g_signal_connect(button, "clicked", G_CALLBACK(search_studio_count_activate), page);

	button = gtk_button_new_with_mnemonic(_("_Mark All"));
	gtk_widget_set_size_request(button, 180, -1);
	gtk_box_pack_start(GTK_BOX(right_vbox), button, FALSE, FALSE, 0);
	g_signal_connect(button, "clicked", G_CALLBACK(search_studio_mark_activate), page);

	button = ui_button_new_with_image(GTK_STOCK_CLOSE, _("_Close"));
	gtk_widget_set_size_request(button, 180, -1);
	gtk_box_pack_start(GTK_BOX(right_vbox), button, FALSE, FALSE, 0);
	g_signal_connect(button, "clicked", G_CALLBACK(search_studio_hide_on_delete), NULL);

	gtk_box_pack_start(GTK_BOX(main_hbox), left_vbox, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(main_hbox), right_vbox, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(page), main_hbox, TRUE, TRUE, 0);

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ui_lookup_widget(page,
		settings.find_regexp ? "mode_regex" : settings.find_escape_sequences ? "mode_extended" : "mode_normal")), TRUE);
	search_studio_mode_toggled(GTK_TOGGLE_BUTTON(ui_lookup_widget(page,
		settings.find_regexp ? "mode_regex" : settings.find_escape_sequences ? "mode_extended" : "mode_normal")), page);
	return page;
}


static GtkWidget *search_studio_create_replace_page(void)
{
	GtkWidget *page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
	GtkWidget *grid = gtk_grid_new();
	GtkWidget *label_find = gtk_label_new_with_mnemonic(_("_Find what:"));
	GtkWidget *label_replace = gtk_label_new_with_mnemonic(_("Replace wit_h:"));
	GtkWidget *find_combo = gtk_combo_box_text_new_with_entry();
	GtkWidget *replace_combo = gtk_combo_box_text_new_with_entry();
	GtkWidget *actions = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
	GtkWidget *button;
	GtkWidget *swap_button;

	GtkWidget *main_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
	GtkWidget *left_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
	GtkWidget *right_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);

	gtk_grid_set_column_spacing(GTK_GRID(grid), 8);
	gtk_grid_set_row_spacing(GTK_GRID(grid), 8);
	ui_hookup_widget(page, find_combo, "combo_search");
	ui_hookup_widget(page, gtk_bin_get_child(GTK_BIN(find_combo)), "entry_search");
	ui_hookup_widget(page, replace_combo, "combo_replace");
	ui_hookup_widget(page, gtk_bin_get_child(GTK_BIN(replace_combo)), "entry_replace");
	ui_entry_add_clear_icon(GTK_ENTRY(ui_lookup_widget(page, "entry_search")));
	ui_entry_add_clear_icon(GTK_ENTRY(ui_lookup_widget(page, "entry_replace")));
	gtk_label_set_mnemonic_widget(GTK_LABEL(label_find), find_combo);
	gtk_label_set_mnemonic_widget(GTK_LABEL(label_replace), replace_combo);

	swap_button = gtk_button_new();
	gtk_button_set_image(GTK_BUTTON(swap_button), gtk_image_new_from_stock(GTK_STOCK_REFRESH, GTK_ICON_SIZE_BUTTON));
	gtk_widget_set_tooltip_text(swap_button, _("Swap Find and Replace text"));
	g_signal_connect(swap_button, "clicked", G_CALLBACK(search_studio_swap_find_replace), page);

	gtk_grid_attach(GTK_GRID(grid), label_find, 0, 0, 1, 1);
	gtk_grid_attach(GTK_GRID(grid), find_combo, 1, 0, 1, 1);
	gtk_grid_attach(GTK_GRID(grid), swap_button, 2, 0, 1, 2);
	gtk_grid_attach(GTK_GRID(grid), label_replace, 0, 1, 1, 1);
	gtk_grid_attach(GTK_GRID(grid), replace_combo, 1, 1, 1, 1);
	gtk_box_pack_start(GTK_BOX(left_vbox), grid, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(left_vbox), search_studio_create_common_options(page, TRUE, FALSE), FALSE, FALSE, 0);

	button = ui_button_new_with_image(GTK_STOCK_FIND, _("Find _Next"));
	gtk_widget_set_size_request(button, 180, -1);
	g_object_set_data(G_OBJECT(button), "studio-replace-action", GINT_TO_POINTER(GEANY_RESPONSE_FIND));
	gtk_box_pack_start(GTK_BOX(right_vbox), button, FALSE, FALSE, 0);
	g_signal_connect(button, "clicked", G_CALLBACK(search_studio_replace_action_activate), page);

	button = gtk_button_new_with_mnemonic(_("_Replace"));
	gtk_widget_set_size_request(button, 180, -1);
	g_object_set_data(G_OBJECT(button), "studio-replace-action", GINT_TO_POINTER(GEANY_RESPONSE_REPLACE));
	gtk_box_pack_start(GTK_BOX(right_vbox), button, FALSE, FALSE, 0);
	g_signal_connect(button, "clicked", G_CALLBACK(search_studio_replace_action_activate), page);

	button = gtk_button_new_with_mnemonic(_("Replace All in _Current Doc"));
	gtk_widget_set_size_request(button, 180, -1);
	g_object_set_data(G_OBJECT(button), "studio-replace-action", GINT_TO_POINTER(GEANY_RESPONSE_REPLACE_IN_FILE));
	gtk_box_pack_start(GTK_BOX(right_vbox), button, FALSE, FALSE, 0);
	g_signal_connect(button, "clicked", G_CALLBACK(search_studio_replace_action_activate), page);

	button = gtk_button_new_with_mnemonic(_("Replace All in All _Opened Docs"));
	gtk_widget_set_size_request(button, 180, -1);
	g_object_set_data(G_OBJECT(button), "studio-replace-action", GINT_TO_POINTER(GEANY_RESPONSE_REPLACE_IN_SESSION));
	gtk_box_pack_start(GTK_BOX(right_vbox), button, FALSE, FALSE, 0);
	g_signal_connect(button, "clicked", G_CALLBACK(search_studio_replace_action_activate), page);

	button = gtk_button_new_with_mnemonic(_("Replace All in _Selection"));
	gtk_widget_set_size_request(button, 180, -1);
	g_object_set_data(G_OBJECT(button), "studio-replace-action", GINT_TO_POINTER(GEANY_RESPONSE_REPLACE_IN_SEL));
	gtk_box_pack_start(GTK_BOX(right_vbox), button, FALSE, FALSE, 0);
	g_signal_connect(button, "clicked", G_CALLBACK(search_studio_replace_action_activate), page);

	button = gtk_button_new_with_mnemonic(_("Find _All in Current Doc"));
	gtk_widget_set_size_request(button, 180, -1);
	gtk_box_pack_start(GTK_BOX(right_vbox), button, FALSE, FALSE, 0);
	g_signal_connect(button, "clicked", G_CALLBACK(search_studio_collect_document_hits), page);

	button = gtk_button_new_with_mnemonic(_("Find All in All _Opened Docs"));
	gtk_widget_set_size_request(button, 180, -1);
	gtk_box_pack_start(GTK_BOX(right_vbox), button, FALSE, FALSE, 0);
	g_signal_connect(button, "clicked", G_CALLBACK(search_studio_collect_session_hits), page);

	button = ui_button_new_with_image(GTK_STOCK_CLOSE, _("_Close"));
	gtk_widget_set_size_request(button, 180, -1);
	gtk_box_pack_start(GTK_BOX(right_vbox), button, FALSE, FALSE, 0);
	g_signal_connect(button, "clicked", G_CALLBACK(search_studio_hide_on_delete), NULL);

	gtk_box_pack_start(GTK_BOX(main_hbox), left_vbox, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(main_hbox), right_vbox, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(page), main_hbox, TRUE, TRUE, 0);

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ui_lookup_widget(page,
		settings.replace_regexp ? "mode_regex" : settings.replace_escape_sequences ? "mode_extended" : "mode_normal")), TRUE);
	search_studio_mode_toggled(GTK_TOGGLE_BUTTON(ui_lookup_widget(page,
		settings.replace_regexp ? "mode_regex" : settings.replace_escape_sequences ? "mode_extended" : "mode_normal")), page);
	return page;
}


static GtkWidget *search_studio_create_fif_page(void)
{
	GtkWidget *page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
	GtkWidget *grid = gtk_grid_new();
	GtkWidget *options_grid = gtk_grid_new();
	GtkWidget *label_find = gtk_label_new_with_mnemonic(_("Find _what :"));
	GtkWidget *label_replace = gtk_label_new_with_mnemonic(_("Replace wit_h :"));
	GtkWidget *label_dir = gtk_label_new_with_mnemonic(_("_Directory :"));
	GtkWidget *label_files = gtk_label_new_with_mnemonic(_("File _patterns :"));
	GtkWidget *label_encoding = gtk_label_new_with_mnemonic(_("E_ncoding :"));
	GtkWidget *entry_find = gtk_entry_new();
	GtkWidget *entry_replace = gtk_entry_new();
	GtkWidget *entry_dir = gtk_entry_new();
	GtkWidget *button_current_dir = gtk_button_new_with_mnemonic(_("Current _Doc"));
	GtkWidget *button_project_dir = gtk_button_new_with_mnemonic(_("Current Pro_ject"));
	GtkWidget *button_browse_dir = gtk_button_new_with_mnemonic(_("_Browse..."));
	GtkWidget *dir_buttons = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
	GtkWidget *entry_files = gtk_entry_new();
	GtkWidget *combo_files_mode = create_fif_file_mode_combo();
	GtkWidget *encoding_combo = ui_create_encodings_combo_box(FALSE, GEANY_ENCODING_UTF_8);
	GtkWidget *check_case = gtk_check_button_new_with_mnemonic(_("Match _case"));
	GtkWidget *check_word = gtk_check_button_new_with_mnemonic(_("Whole _word only"));
	GtkWidget *check_recursive = gtk_check_button_new_with_mnemonic(_("_Recursive"));
	GtkWidget *check_hidden = gtk_check_button_new_with_mnemonic(_("In _hidden folders"));
	GtkWidget *button;

	GtkWidget *main_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
	GtkWidget *left_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
	GtkWidget *right_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);

	gtk_grid_set_column_spacing(GTK_GRID(grid), 8);
	gtk_grid_set_row_spacing(GTK_GRID(grid), 8);
	gtk_grid_set_column_spacing(GTK_GRID(options_grid), 12);
	gtk_grid_set_row_spacing(GTK_GRID(options_grid), 6);
	ui_hookup_widget(page, entry_find, "entry_search");
	ui_hookup_widget(page, entry_replace, "entry_replace");
	ui_hookup_widget(page, entry_dir, "entry_dir");
	ui_hookup_widget(page, entry_files, "entry_files");
	ui_hookup_widget(page, combo_files_mode, "combo_files_mode");
	ui_hookup_widget(page, encoding_combo, "combo_encoding");
	ui_hookup_widget(page, check_case, "check_case");
	ui_hookup_widget(page, check_word, "check_word");
	ui_hookup_widget(page, check_recursive, "check_recursive");
	ui_hookup_widget(page, check_hidden, "check_hidden");

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_recursive), settings.fif_recursive);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_hidden), settings.fif_hidden_folders);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_case), settings.fif_case_sensitive);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_word), settings.fif_match_whole_word);

	g_signal_connect(combo_files_mode, "changed", G_CALLBACK(search_studio_fif_file_mode_changed), page);
	g_signal_connect(button_current_dir, "clicked", G_CALLBACK(search_studio_use_current_doc_dir_activate), page);
	g_signal_connect(button_project_dir, "clicked", G_CALLBACK(search_studio_use_project_dir_activate), page);
	g_signal_connect(button_browse_dir, "clicked", G_CALLBACK(search_studio_browse_dir_activate), page);
	gtk_combo_box_set_active(GTK_COMBO_BOX(combo_files_mode), settings.fif_files_mode);
	gtk_entry_set_text(GTK_ENTRY(entry_files), settings.fif_files ? settings.fif_files : "");

	gtk_label_set_mnemonic_widget(GTK_LABEL(label_find), entry_find);
	gtk_label_set_mnemonic_widget(GTK_LABEL(label_replace), entry_replace);
	gtk_label_set_mnemonic_widget(GTK_LABEL(label_dir), entry_dir);
	gtk_label_set_mnemonic_widget(GTK_LABEL(label_files), combo_files_mode);
	gtk_label_set_mnemonic_widget(GTK_LABEL(label_encoding), encoding_combo);

	gtk_grid_attach(GTK_GRID(grid), label_find, 0, 0, 1, 1);
	gtk_grid_attach(GTK_GRID(grid), entry_find, 1, 0, 2, 1);
	gtk_grid_attach(GTK_GRID(grid), label_replace, 0, 1, 1, 1);
	gtk_grid_attach(GTK_GRID(grid), entry_replace, 1, 1, 2, 1);

	gtk_box_pack_start(GTK_BOX(dir_buttons), button_current_dir, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(dir_buttons), button_project_dir, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(dir_buttons), button_browse_dir, FALSE, FALSE, 0);
	gtk_grid_attach(GTK_GRID(grid), label_dir, 0, 2, 1, 1);
	gtk_grid_attach(GTK_GRID(grid), entry_dir, 1, 2, 1, 1);
	gtk_grid_attach(GTK_GRID(grid), dir_buttons, 2, 2, 1, 1);
	gtk_grid_attach(GTK_GRID(grid), label_files, 0, 3, 1, 1);
	gtk_grid_attach(GTK_GRID(grid), combo_files_mode, 1, 3, 1, 1);
	gtk_grid_attach(GTK_GRID(grid), entry_files, 2, 3, 1, 1);
	gtk_grid_attach(GTK_GRID(grid), label_encoding, 0, 4, 1, 1);
	gtk_grid_attach(GTK_GRID(grid), encoding_combo, 1, 4, 2, 1);

	gtk_grid_attach(GTK_GRID(options_grid), check_case, 0, 0, 1, 1);
	gtk_grid_attach(GTK_GRID(options_grid), check_word, 1, 0, 1, 1);
	gtk_grid_attach(GTK_GRID(options_grid), check_recursive, 0, 1, 1, 1);
	gtk_grid_attach(GTK_GRID(options_grid), check_hidden, 1, 1, 1, 1);

	gtk_box_pack_start(GTK_BOX(left_vbox), grid, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(left_vbox), search_studio_create_mode_box(page), FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(left_vbox), options_grid, FALSE, FALSE, 0);

	button = ui_button_new_with_image(GTK_STOCK_FIND, _("_Find All"));
	gtk_widget_set_size_request(button, 180, -1);
	gtk_box_pack_start(GTK_BOX(right_vbox), button, FALSE, FALSE, 0);
	g_signal_connect(button, "clicked", G_CALLBACK(search_studio_fif_find_activate), page);

	button = gtk_button_new_with_mnemonic(_("_Replace in Files"));
	gtk_widget_set_size_request(button, 180, -1);
	gtk_box_pack_start(GTK_BOX(right_vbox), button, FALSE, FALSE, 0);
	g_signal_connect(button, "clicked", G_CALLBACK(search_studio_fif_replace_activate), page);

	button = ui_button_new_with_image(GTK_STOCK_CLOSE, _("_Close"));
	gtk_widget_set_size_request(button, 180, -1);
	gtk_box_pack_start(GTK_BOX(right_vbox), button, FALSE, FALSE, 0);
	g_signal_connect(button, "clicked", G_CALLBACK(search_studio_hide_on_delete), NULL);

	gtk_box_pack_start(GTK_BOX(main_hbox), left_vbox, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(main_hbox), right_vbox, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(page), main_hbox, TRUE, TRUE, 0);

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ui_lookup_widget(page,
		settings.fif_regexp ? "mode_regex" : "mode_normal")), TRUE);
	search_studio_mode_toggled(GTK_TOGGLE_BUTTON(ui_lookup_widget(page,
		settings.fif_regexp ? "mode_regex" : "mode_normal")), page);
	search_studio_fif_file_mode_changed(GTK_COMBO_BOX(combo_files_mode), page);
	return page;
}


	gtk_label_set_xalign(GTK_LABEL(hint), 0.0f);
	gtk_label_set_line_wrap(GTK_LABEL(hint), TRUE);
	gtk_box_pack_start(GTK_BOX(page), grid, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(page), search_studio_create_mode_box(page), FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(page), options_grid, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(page), hint, FALSE, FALSE, 0);

	button = ui_button_new_with_image(GTK_STOCK_FIND, _("_Find in Files now"));
	gtk_box_pack_start(GTK_BOX(actions), button, FALSE, FALSE, 0);
	g_signal_connect(button, "clicked", G_CALLBACK(search_studio_fif_find_activate), page);
	button = ui_button_new_with_image(GTK_STOCK_FIND, _("Find in Pro_ject"));
	gtk_box_pack_start(GTK_BOX(actions), button, FALSE, FALSE, 0);
	g_signal_connect(button, "clicked", G_CALLBACK(search_studio_find_in_project_activate), page);
	button = gtk_button_new_with_mnemonic(_("Use Pro_ject Scope"));
	gtk_box_pack_start(GTK_BOX(actions), button, FALSE, FALSE, 0);
	g_signal_connect(button, "clicked", G_CALLBACK(search_studio_use_project_dir_activate), page);
	button = ui_button_new_with_image(GTK_STOCK_FIND, _("Open full Find in F_iles"));
	gtk_box_pack_start(GTK_BOX(actions), button, FALSE, FALSE, 0);
	g_signal_connect(button, "clicked", G_CALLBACK(search_studio_open_fif_dialog_activate), page);
	gtk_box_pack_start(GTK_BOX(page), actions, FALSE, FALSE, 0);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ui_lookup_widget(page,
		settings.fif_regexp ? "mode_regex" : "mode_normal")), TRUE);
	search_studio_mode_toggled(GTK_TOGGLE_BUTTON(ui_lookup_widget(page,
		settings.fif_regexp ? "mode_regex" : "mode_normal")), page);
	search_studio_fif_file_mode_changed(GTK_COMBO_BOX(combo_files_mode), page);
	return page;
}


static GtkWidget *search_studio_create_fip_page(void)
{
	GtkWidget *page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
	GtkWidget *grid = gtk_grid_new();
	GtkWidget *options_grid = gtk_grid_new();
	GtkWidget *label_find = gtk_label_new_with_mnemonic(_("Find _what :"));
	GtkWidget *label_project = gtk_label_new_with_mnemonic(_("_Project root :"));
	GtkWidget *label_patterns = gtk_label_new_with_mnemonic(_("Project _patterns :"));
	GtkWidget *label_encoding = gtk_label_new_with_mnemonic(_("E_ncoding :"));
	GtkWidget *entry_find = gtk_entry_new();
	GtkWidget *entry_dir = gtk_entry_new();
	GtkWidget *entry_files = gtk_entry_new();
	GtkWidget *encoding_combo = ui_create_encodings_combo_box(FALSE, GEANY_ENCODING_UTF_8);
	GtkWidget *check_case = gtk_check_button_new_with_mnemonic(_("Match _case"));
	GtkWidget *check_word = gtk_check_button_new_with_mnemonic(_("Whole _word only"));
	GtkWidget *check_recursive = gtk_check_button_new_with_mnemonic(_("_Recursive"));
	GtkWidget *check_hidden = gtk_check_button_new_with_mnemonic(_("In _hidden folders"));
	GtkWidget *dir_buttons = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
	GtkWidget *button;

	GtkWidget *main_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
	GtkWidget *left_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
	GtkWidget *right_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);

	gtk_grid_set_column_spacing(GTK_GRID(grid), 8);
	gtk_grid_set_row_spacing(GTK_GRID(grid), 8);
	gtk_grid_set_column_spacing(GTK_GRID(options_grid), 12);
	gtk_grid_set_row_spacing(GTK_GRID(options_grid), 6);
	ui_hookup_widget(page, entry_find, "entry_search");
	ui_hookup_widget(page, entry_dir, "entry_dir");
	ui_hookup_widget(page, entry_files, "entry_files");
	ui_hookup_widget(page, encoding_combo, "combo_encoding");
	ui_hookup_widget(page, check_case, "check_case");
	ui_hookup_widget(page, check_word, "check_word");
	ui_hookup_widget(page, check_recursive, "check_recursive");
	ui_hookup_widget(page, check_hidden, "check_hidden");

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_recursive), settings.fif_recursive);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_hidden), settings.fif_hidden_folders);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_case), settings.fif_case_sensitive);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_word), settings.fif_match_whole_word);

	gtk_label_set_mnemonic_widget(GTK_LABEL(label_find), entry_find);
	gtk_label_set_mnemonic_widget(GTK_LABEL(label_project), entry_dir);
	gtk_label_set_mnemonic_widget(GTK_LABEL(label_patterns), entry_files);
	gtk_label_set_mnemonic_widget(GTK_LABEL(label_encoding), encoding_combo);
	gtk_grid_attach(GTK_GRID(grid), label_find, 0, 0, 1, 1);
	gtk_grid_attach(GTK_GRID(grid), entry_find, 1, 0, 2, 1);
	gtk_grid_attach(GTK_GRID(grid), label_project, 0, 1, 1, 1);
	gtk_grid_attach(GTK_GRID(grid), entry_dir, 1, 1, 2, 1);
	gtk_grid_attach(GTK_GRID(grid), label_patterns, 0, 2, 1, 1);
	gtk_grid_attach(GTK_GRID(grid), entry_files, 1, 2, 2, 1);
	gtk_grid_attach(GTK_GRID(grid), label_encoding, 0, 3, 1, 1);
	gtk_grid_attach(GTK_GRID(grid), encoding_combo, 1, 3, 2, 1);

	gtk_grid_attach(GTK_GRID(options_grid), check_case, 0, 0, 1, 1);
	gtk_grid_attach(GTK_GRID(options_grid), check_word, 1, 0, 1, 1);
	gtk_grid_attach(GTK_GRID(options_grid), check_recursive, 0, 1, 1, 1);
	gtk_grid_attach(GTK_GRID(options_grid), check_hidden, 1, 1, 1, 1);

	button = gtk_button_new_with_mnemonic(_("Current Pro_ject"));
	gtk_box_pack_start(GTK_BOX(dir_buttons), button, FALSE, FALSE, 0);
	g_signal_connect(button, "clicked", G_CALLBACK(search_studio_use_project_dir_activate), page);

	gtk_box_pack_start(GTK_BOX(left_vbox), grid, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(left_vbox), dir_buttons, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(left_vbox), search_studio_create_mode_box(page), FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(left_vbox), options_grid, FALSE, FALSE, 0);

	button = ui_button_new_with_image(GTK_STOCK_FIND, _("_Find All"));
	gtk_widget_set_size_request(button, 180, -1);
	gtk_box_pack_start(GTK_BOX(right_vbox), button, FALSE, FALSE, 0);
	g_signal_connect(button, "clicked", G_CALLBACK(search_studio_find_in_project_activate), page);

	button = ui_button_new_with_image(GTK_STOCK_CLOSE, _("_Close"));
	gtk_widget_set_size_request(button, 180, -1);
	gtk_box_pack_start(GTK_BOX(right_vbox), button, FALSE, FALSE, 0);
	g_signal_connect(button, "clicked", G_CALLBACK(search_studio_hide_on_delete), NULL);

	gtk_box_pack_start(GTK_BOX(main_hbox), left_vbox, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(main_hbox), right_vbox, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(page), main_hbox, TRUE, TRUE, 0);

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ui_lookup_widget(page,
		settings.fif_regexp ? "mode_regex" : "mode_normal")), TRUE);
	search_studio_mode_toggled(GTK_TOGGLE_BUTTON(ui_lookup_widget(page,
		settings.fif_regexp ? "mode_regex" : "mode_normal")), page);
	search_studio_use_project_dir_activate(NULL, page);
	return page;
}


static GtkWidget *search_studio_create_mark_page(void)
{
	GtkWidget *page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
	GtkWidget *main_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
	GtkWidget *left_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
	GtkWidget *right_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
	GtkWidget *label = gtk_label_new_with_mnemonic(_("Find _what :"));
	GtkWidget *entry = gtk_combo_box_text_new_with_entry();
	GtkWidget *row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
	GtkWidget *check_bookmark = gtk_check_button_new_with_mnemonic(_("Book_mark line"));
	GtkWidget *check_purge = gtk_check_button_new_with_mnemonic(_("_Purge for find"));
	GtkWidget *options_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
	GtkWidget *button;

	ui_hookup_widget(page, entry, "combo_search");
	ui_hookup_widget(page, gtk_bin_get_child(GTK_BIN(entry)), "entry_search");
	ui_hookup_widget(page, check_bookmark, "check_bookmark");
	ui_hookup_widget(page, check_purge, "check_purge");

	ui_entry_add_clear_icon(GTK_ENTRY(ui_lookup_widget(page, "entry_search")));
	gtk_label_set_mnemonic_widget(GTK_LABEL(label), entry);

	gtk_box_pack_start(GTK_BOX(row), label, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(row), entry, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(left_vbox), row, FALSE, FALSE, 0);

	gtk_box_pack_start(GTK_BOX(options_vbox), check_bookmark, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(options_vbox), check_purge, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(left_vbox), options_vbox, FALSE, FALSE, 0);

	gtk_box_pack_start(GTK_BOX(left_vbox), search_studio_create_common_options(page, FALSE, TRUE), FALSE, FALSE, 0);

	button = gtk_button_new_with_mnemonic(_("_Mark All"));
	gtk_widget_set_size_request(button, 180, -1);
	gtk_box_pack_start(GTK_BOX(right_vbox), button, FALSE, FALSE, 0);
	g_signal_connect(button, "clicked", G_CALLBACK(search_studio_mark_activate), page);

	button = gtk_button_new_with_mnemonic(_("Clea_r All Marks"));
	gtk_widget_set_size_request(button, 180, -1);
	gtk_box_pack_start(GTK_BOX(right_vbox), button, FALSE, FALSE, 0);
	g_signal_connect(button, "clicked", G_CALLBACK(search_studio_clear_marks_activate), page);

	button = gtk_button_new_with_mnemonic(_("_Inverse Marks"));
	gtk_widget_set_size_request(button, 180, -1);
	gtk_box_pack_start(GTK_BOX(right_vbox), button, FALSE, FALSE, 0);
	g_signal_connect(button, "clicked", G_CALLBACK(search_studio_inverse_marks_activate), page);

	button = gtk_button_new_with_mnemonic(_("C_opy Marked Lines"));
	gtk_widget_set_size_request(button, 180, -1);
	gtk_box_pack_start(GTK_BOX(right_vbox), button, FALSE, FALSE, 0);
	g_signal_connect(button, "clicked", G_CALLBACK(search_studio_copy_marked_lines_activate), page);

	button = gtk_button_new_with_mnemonic(_("Delete _Marked Lines"));
	gtk_widget_set_size_request(button, 180, -1);
	gtk_box_pack_start(GTK_BOX(right_vbox), button, FALSE, FALSE, 0);
	g_signal_connect(button, "clicked", G_CALLBACK(search_studio_delete_marked_lines_activate), page);

	button = ui_button_new_with_image(GTK_STOCK_CLOSE, _("_Close"));
	gtk_widget_set_size_request(button, 180, -1);
	gtk_box_pack_start(GTK_BOX(right_vbox), button, FALSE, FALSE, 0);
	g_signal_connect(button, "clicked", G_CALLBACK(search_studio_hide_on_delete), NULL);

	gtk_box_pack_start(GTK_BOX(main_hbox), left_vbox, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(main_hbox), right_vbox, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(page), main_hbox, TRUE, TRUE, 0);

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ui_lookup_widget(page,
		settings.find_regexp ? "mode_regex" : settings.find_escape_sequences ? "mode_extended" : "mode_normal")), TRUE);
	search_studio_mode_toggled(GTK_TOGGLE_BUTTON(ui_lookup_widget(page,
		settings.find_regexp ? "mode_regex" : settings.find_escape_sequences ? "mode_extended" : "mode_normal")), page);

	return page;
}


static GtkWidget *add_transparency_frame(GtkWidget *owner)
{
	GtkWidget *frame, *vbox, *hbox, *check, *radio1, *radio2, *scale;

	frame = gtk_frame_new(_("Transparency"));
	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 6);

	check = gtk_check_button_new_with_mnemonic(_("Enable"));
	ui_hookup_widget(owner, check, "check_transparency");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check), settings.transparency_enabled);
	gtk_box_pack_start(GTK_BOX(vbox), check, FALSE, FALSE, 0);

	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
	radio1 = gtk_radio_button_new_with_mnemonic(NULL, _("On losing focus"));
	ui_hookup_widget(owner, radio1, "radio_transparency_focus");
	radio2 = gtk_radio_button_new_with_mnemonic_from_widget(GTK_RADIO_BUTTON(radio1), _("Always"));
	ui_hookup_widget(owner, radio2, "radio_transparency_always");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(settings.transparency_mode == 0 ? radio1 : radio2), TRUE);
	gtk_box_pack_start(GTK_BOX(hbox), radio1, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), radio2, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

	scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0, 255, 1);
	ui_hookup_widget(owner, scale, "scale_transparency_value");
	gtk_scale_set_draw_value(GTK_SCALE(scale), FALSE);
	gtk_range_set_value(GTK_RANGE(scale), settings.transparency_value);
	gtk_box_pack_start(GTK_BOX(vbox), scale, FALSE, FALSE, 0);

	gtk_container_add(GTK_CONTAINER(frame), vbox);
	return frame;
}


static void create_search_studio_dialog(void)
{
	GtkWidget *content;
	GtkWidget *header;
	GtkWidget *title;
	GtkWidget *subtitle;
	GtkWidget *close_button;
	GtkWidget *notebook;
	GtkWidget *paned;
	GtkWidget *preview_notebook;
	GtkWidget *activity_frame;
	GtkWidget *activity_scroll;
	GtkWidget *activity_label;
	GtkWidget *results_frame;
	GtkWidget *results_scroll;
	GtkWidget *results_label;
	GtkWidget *preview_frame;
	GtkWidget *preview_scroll;
	GtkWidget *preview_label;
	GtkTreeSelection *selection;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;

	studio_dlg.dialog = gtk_dialog_new();
	gtk_window_set_title(GTK_WINDOW(studio_dlg.dialog), _("Search Studio"));
	gtk_window_set_transient_for(GTK_WINDOW(studio_dlg.dialog), GTK_WINDOW(main_widgets.window));
	gtk_window_set_destroy_with_parent(GTK_WINDOW(studio_dlg.dialog), TRUE);
	geany_search_dialog_set_css_name(studio_dlg.dialog);
	gtk_window_set_default_size(GTK_WINDOW(studio_dlg.dialog), 920, 480);
	g_signal_connect(studio_dlg.dialog, "delete-event", G_CALLBACK(search_studio_hide_on_delete_cb), NULL);
	g_signal_connect(studio_dlg.dialog, "response", G_CALLBACK(search_studio_hide_on_delete_cb), NULL);

	content = ui_dialog_vbox_new(GTK_DIALOG(studio_dlg.dialog));
	gtk_box_set_spacing(GTK_BOX(content), 10);

	header = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
	title = ui_label_new_bold(_("Search Studio"));
	subtitle = gtk_label_new(_("Unified Search/Replace Cockpit"));
	gtk_label_set_xalign(GTK_LABEL(subtitle), 0.0f);
	gtk_label_set_line_wrap(GTK_LABEL(subtitle), TRUE);
	gtk_box_pack_start(GTK_BOX(header), title, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(header), subtitle, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(content), header, FALSE, FALSE, 0);

	paned = gtk_paned_new(GTK_ORIENTATION_VERTICAL);
	gtk_paned_set_position(GTK_PANED(paned), 450);

	notebook = gtk_notebook_new();
	studio_dlg.notebook = notebook;
	studio_dlg.find_page = search_studio_create_find_page();
	studio_dlg.replace_page = search_studio_create_replace_page();
	studio_dlg.fif_page = search_studio_create_fif_page();
	studio_dlg.fip_page = search_studio_create_fip_page();
	studio_dlg.mark_page = search_studio_create_mark_page();
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), studio_dlg.find_page, gtk_label_new(_("Find")));
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), studio_dlg.replace_page, gtk_label_new(_("Replace")));
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), studio_dlg.fif_page, gtk_label_new(_("Find in Files")));
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), studio_dlg.fip_page, gtk_label_new(_("Find in Projects")));
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), studio_dlg.mark_page, gtk_label_new(_("Mark")));

	g_signal_connect(notebook, "switch-page", G_CALLBACK(search_studio_notebook_switch_page), NULL);
	gtk_paned_pack1(GTK_PANED(paned), notebook, TRUE, FALSE);

	preview_notebook = gtk_notebook_new();
	studio_dlg.lower_notebook = preview_notebook;

	activity_frame = gtk_frame_new(NULL);
	gtk_frame_set_shadow_type(GTK_FRAME(activity_frame), GTK_SHADOW_IN);
	activity_label = ui_label_new_bold(_("Activity / Preview"));
	gtk_frame_set_label_widget(GTK_FRAME(activity_frame), activity_label);
	activity_scroll = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(activity_scroll),
		GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	studio_dlg.activity_view = gtk_text_view_new();
	gtk_text_view_set_editable(GTK_TEXT_VIEW(studio_dlg.activity_view), FALSE);
	gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(studio_dlg.activity_view), FALSE);
	gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(studio_dlg.activity_view), GTK_WRAP_WORD_CHAR);
	gtk_container_add(GTK_CONTAINER(activity_scroll), studio_dlg.activity_view);
	gtk_container_add(GTK_CONTAINER(activity_frame), activity_scroll);
	gtk_notebook_append_page(GTK_NOTEBOOK(preview_notebook), activity_frame, gtk_label_new(_("Activity")));

	results_frame = gtk_frame_new(NULL);
	gtk_frame_set_shadow_type(GTK_FRAME(results_frame), GTK_SHADOW_IN);
	results_label = ui_label_new_bold(_("Structured Results"));
	gtk_frame_set_label_widget(GTK_FRAME(results_frame), results_label);
	results_scroll = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(results_scroll),
		GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	studio_dlg.results_store = gtk_list_store_new(STUDIO_RESULT_COLUMNS,
		G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING,
		G_TYPE_STRING, G_TYPE_INT, G_TYPE_INT, G_TYPE_BOOLEAN, G_TYPE_STRING, G_TYPE_STRING);
	studio_dlg.results_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(studio_dlg.results_store));
	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes(_("Action"), renderer, "text", 0, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(studio_dlg.results_view), column);
	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes(_("Target"), renderer, "text", 1, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(studio_dlg.results_view), column);
	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes(_("Query"), renderer, "text", 2, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(studio_dlg.results_view), column);
	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes(_("Mode"), renderer, "text", 3, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(studio_dlg.results_view), column);
	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes(_("Summary"), renderer, "text", 4, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(studio_dlg.results_view), column);
	g_signal_connect(studio_dlg.results_view, "row-activated",
		G_CALLBACK(search_studio_results_row_activated), NULL);
	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(studio_dlg.results_view));
	gtk_tree_selection_set_mode(selection, GTK_SELECTION_SINGLE);
	g_signal_connect(selection, "changed", G_CALLBACK(search_studio_results_selection_changed), NULL);
	gtk_container_add(GTK_CONTAINER(results_scroll), studio_dlg.results_view);
	gtk_container_add(GTK_CONTAINER(results_frame), results_scroll);
	gtk_notebook_append_page(GTK_NOTEBOOK(preview_notebook), results_frame, gtk_label_new(_("Results")));

	preview_frame = gtk_frame_new(NULL);
	gtk_frame_set_shadow_type(GTK_FRAME(preview_frame), GTK_SHADOW_IN);
	preview_label = ui_label_new_bold(_("Diff Preview"));
	gtk_frame_set_label_widget(GTK_FRAME(preview_frame), preview_label);
	preview_scroll = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(preview_scroll),
		GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	studio_dlg.preview_view = gtk_text_view_new();
	gtk_text_view_set_editable(GTK_TEXT_VIEW(studio_dlg.preview_view), FALSE);
	gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(studio_dlg.preview_view), FALSE);
	gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(studio_dlg.preview_view), GTK_WRAP_WORD_CHAR);
	gtk_container_add(GTK_CONTAINER(preview_scroll), studio_dlg.preview_view);
	gtk_container_add(GTK_CONTAINER(preview_frame), preview_scroll);
	gtk_notebook_append_page(GTK_NOTEBOOK(preview_notebook), preview_frame, gtk_label_new(_("Diff Preview")));

	gtk_paned_pack2(GTK_PANED(paned), preview_notebook, FALSE, FALSE);

	gtk_box_pack_start(GTK_BOX(content), paned, TRUE, TRUE, 0);

	/* Transparency frame in lower right */
	{
		GtkWidget *tframe = add_transparency_frame(studio_dlg.dialog);
		GtkWidget *hbox_trans = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
		gtk_box_pack_end(GTK_BOX(hbox_trans), tframe, FALSE, FALSE, 0);
		gtk_box_pack_start(GTK_BOX(content), hbox_trans, FALSE, FALSE, 0);
	}
	search_studio_activity_append("Search Studio initialized. Classic dialogs remain available, but this unified cockpit now handles Find, Replace, Find in Files, and Mark workflows directly.");
	search_studio_result_append("Studio", "Search Studio", "Initialization", "N/A",
		"Unified notebook, direct execution paths, and lower preview/results panes are ready.");
	search_studio_set_preview(_("Diff Preview"), _("Select a result row to inspect before/after or result details."));

	close_button = gtk_button_new_from_stock(GTK_STOCK_CLOSE);
	gtk_dialog_add_action_widget(GTK_DIALOG(studio_dlg.dialog), close_button, GTK_RESPONSE_CLOSE);

#ifdef G_OS_WIN32
	win32_update_titlebar_theme(studio_dlg.dialog);
#endif
}


static void search_studio_show_page(gint page_num)
{
	GeanyDocument *doc = document_get_current();
	gchar *sel = NULL;
	gchar *cur_dir = NULL;

	if (studio_dlg.dialog == NULL)
		create_search_studio_dialog();

	if (doc)
		sel = editor_get_default_selection(doc->editor, search_prefs.use_current_word, NULL);
	if (sel)
	{
		gtk_entry_set_text(GTK_ENTRY(ui_lookup_widget(studio_dlg.find_page, "entry_search")), sel);
		gtk_entry_set_text(GTK_ENTRY(ui_lookup_widget(studio_dlg.replace_page, "entry_search")), sel);
		gtk_entry_set_text(GTK_ENTRY(ui_lookup_widget(studio_dlg.mark_page, "entry_search")), sel);
		gtk_entry_set_text(GTK_ENTRY(ui_lookup_widget(studio_dlg.fif_page, "entry_search")), sel);
		gtk_entry_set_text(GTK_ENTRY(ui_lookup_widget(studio_dlg.fip_page, "entry_search")), sel);
	}
	g_free(sel);

	cur_dir = utils_get_current_file_dir_utf8();
	if (!EMPTY(cur_dir))
		gtk_entry_set_text(GTK_ENTRY(ui_lookup_widget(studio_dlg.fif_page, "entry_dir")), cur_dir);
	g_free(cur_dir);
	if (studio_dlg.fip_page != NULL)
		search_studio_use_project_dir_activate(NULL, studio_dlg.fip_page);

	gtk_notebook_set_current_page(GTK_NOTEBOOK(studio_dlg.notebook), page_num);
	set_dialog_position(studio_dlg.dialog, studio_dlg.position);
	gtk_widget_show_all(studio_dlg.dialog);
	gtk_window_present(GTK_WINDOW(studio_dlg.dialog));
}


void search_show_search_studio_dialog(void)
{
	search_studio_show_page(0);
}


static void search_studio_sync_find_dialog_from_page(GtkWidget *page)
{
	gboolean regexp;
	gboolean escape_sequences;

	search_studio_get_mode(page, &regexp, &escape_sequences);
	search_show_find_dialog();
	gtk_entry_set_text(GTK_ENTRY(find_dlg.entry), gtk_entry_get_text(GTK_ENTRY(ui_lookup_widget(page, "entry_search"))));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ui_lookup_widget(find_dlg.dialog, "check_case")),
		gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ui_lookup_widget(page, "check_case"))));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ui_lookup_widget(find_dlg.dialog, "check_word")),
		gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ui_lookup_widget(page, "check_word"))));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ui_lookup_widget(find_dlg.dialog, "check_wordstart")),
		gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ui_lookup_widget(page, "check_wordstart"))));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ui_lookup_widget(find_dlg.dialog, "check_multiline")),
		gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ui_lookup_widget(page, "check_multiline"))));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ui_lookup_widget(find_dlg.dialog, "check_dotmatchnewline")),
		gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ui_lookup_widget(page, "check_dotall"))));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ui_lookup_widget(find_dlg.dialog, "check_regexp")), regexp);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ui_lookup_widget(find_dlg.dialog, "check_escape")), !regexp && escape_sequences);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ui_lookup_widget(find_dlg.dialog, "check_wrap")),
		gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ui_lookup_widget(page, "check_wrap"))));
}


static void search_studio_sync_replace_dialog_from_page(GtkWidget *page)
{
	gboolean regexp;
	gboolean escape_sequences;

	search_studio_get_mode(page, &regexp, &escape_sequences);
	search_show_replace_dialog();
	gtk_entry_set_text(GTK_ENTRY(replace_dlg.find_entry), gtk_entry_get_text(GTK_ENTRY(ui_lookup_widget(page, "entry_search"))));
	gtk_entry_set_text(GTK_ENTRY(replace_dlg.replace_entry), gtk_entry_get_text(GTK_ENTRY(ui_lookup_widget(page, "entry_replace"))));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ui_lookup_widget(replace_dlg.dialog, "check_case")),
		gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ui_lookup_widget(page, "check_case"))));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ui_lookup_widget(replace_dlg.dialog, "check_word")),
		gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ui_lookup_widget(page, "check_word"))));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ui_lookup_widget(replace_dlg.dialog, "check_wordstart")),
		gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ui_lookup_widget(page, "check_wordstart"))));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ui_lookup_widget(replace_dlg.dialog, "check_multiline")),
		gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ui_lookup_widget(page, "check_multiline"))));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ui_lookup_widget(replace_dlg.dialog, "check_dotmatchnewline")),
		gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ui_lookup_widget(page, "check_dotall"))));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ui_lookup_widget(replace_dlg.dialog, "check_back")),
		gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ui_lookup_widget(page, "check_back"))));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ui_lookup_widget(replace_dlg.dialog, "check_regexp")), regexp);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ui_lookup_widget(replace_dlg.dialog, "check_escape")), !regexp && escape_sequences);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ui_lookup_widget(replace_dlg.dialog, "check_wrap")),
		gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ui_lookup_widget(page, "check_wrap"))));
}


static void reset_msgwin(void)
{
	gtk_notebook_set_current_page(GTK_NOTEBOOK(msgwindow.notebook), MSG_MESSAGE);
	gtk_list_store_clear(msgwindow.store_msg);
	// reset width after any long messages
	gtk_tree_view_columns_autosize(GTK_TREE_VIEW(msgwindow.tree_msg));
}


static GString *build_grep_options(gboolean invert_results, gboolean case_sensitive,
	gboolean match_whole_word, gboolean recursive, gboolean hidden_folders, gboolean regexp,
	gboolean use_extra_options, const gchar *extra_options, gint files_mode,
	const gchar *files)
{
	GString *gstr = g_string_new("-nHI");	/* line numbers, filenames, ignore binaries */
	gchar *extra_copy = g_strdup(extra_options != NULL ? extra_options : "");
	gchar *files_copy = g_strdup(files != NULL ? files : "");

	if (invert_results)
		g_string_append_c(gstr, 'v');
	if (! case_sensitive)
		g_string_append_c(gstr, 'i');
	if (match_whole_word)
		g_string_append_c(gstr, 'w');
	if (recursive)
		g_string_append_c(gstr, 'r');
	if (!hidden_folders)
		g_string_append(gstr, " --exclude-dir=\".*\"");

	if (!regexp)
		g_string_append_c(gstr, 'F');
	else
		g_string_append_c(gstr, 'E');

	if (use_extra_options)
	{
		g_strstrip(extra_copy);

		if (*extra_copy != 0)
		{
			g_string_append_c(gstr, ' ');
			g_string_append(gstr, extra_copy);
		}
	}
	g_strstrip(files_copy);
	if (files_mode != FILES_MODE_ALL && *files_copy)
	{
		GString *tmp;

		/* put --include= before each pattern */
		tmp = g_string_new(files_copy);
		do {} while (utils_string_replace_all(tmp, "  ", " "));
		g_string_prepend_c(tmp, ' ');
		utils_string_replace_all(tmp, " ", " --include=");
		g_string_append(gstr, tmp->str);
		g_string_free(tmp, TRUE);
	}

	g_free(extra_copy);
	g_free(files_copy);
	return gstr;
}


static GString *get_grep_options(void)
{
	return build_grep_options(settings.fif_invert_results, settings.fif_case_sensitive,
		settings.fif_match_whole_word, settings.fif_recursive, settings.fif_hidden_folders, settings.fif_regexp,
		settings.fif_use_extra_options, settings.fif_extra_options,
		settings.fif_files_mode, settings.fif_files);
}


static gboolean execute_find_in_files_request(const gchar *search_text, const gchar *utf8_dir,
	gboolean invert_results, gboolean case_sensitive, gboolean match_whole_word,
	gboolean recursive, gboolean hidden_folders, gboolean regexp, gboolean use_extra_options,
	const gchar *extra_options, gint files_mode, const gchar *files,
	GeanyEncodingIndex enc_idx, GtkWidget *search_widget, GtkWidget *dir_widget,
	GtkWidget *files_widget, gboolean capture_results, const gchar *capture_mode)
{
	gchar *locale_dir = utils_get_locale_from_utf8(utf8_dir);
	gboolean ok = FALSE;

	if (!g_file_test(locale_dir, G_FILE_TEST_IS_DIR))
	{
		ui_set_statusbar(FALSE, _("Invalid directory for Find in Files."));
		if (dir_widget != NULL)
			ui_set_search_entry_background(dir_widget, FALSE);
	}
	else if (EMPTY(search_text))
	{
		ui_set_statusbar(FALSE, _("No text to find."));
		if (search_widget != NULL)
			ui_set_search_entry_background(search_widget, FALSE);
	}
	else if (files_mode != FILES_MODE_ALL && EMPTY(files))
	{
		ui_set_statusbar(FALSE, _("No file patterns specified for Find in Files."));
		if (files_widget != NULL)
			ui_set_search_entry_background(files_widget, FALSE);
	}
	else
	{
		GString *opts = build_grep_options(invert_results, case_sensitive, match_whole_word,
			recursive, hidden_folders, regexp, use_extra_options, extra_options, files_mode, files);
		const gchar *enc = (enc_idx == GEANY_ENCODING_UTF_8) ? NULL :
			encodings_get_charset_from_index(enc_idx);

		if (capture_results)
			search_studio_capture_begin(search_text, capture_mode, utf8_dir);
		ok = search_find_in_files(search_text, utf8_dir, opts->str, enc, recursive);
		if (!ok && capture_results)
			search_studio_capture_finish(-1);
		g_string_free(opts, TRUE);
	}

	g_free(locale_dir);
	return ok;
}



static void
on_find_in_files_dialog_response(GtkDialog *dialog, gint response,
		G_GNUC_UNUSED gpointer user_data)
{
	gtk_window_get_position(GTK_WINDOW(fif_dlg.dialog), &fif_dlg.position[0], &fif_dlg.position[1]);

	stash_group_update(fif_prefs, fif_dlg.dialog);

	if (response == GTK_RESPONSE_ACCEPT)
	{
		GtkWidget *search_combo = fif_dlg.search_combo;
		const gchar *search_text =
			gtk_entry_get_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(search_combo))));
		GtkWidget *dir_combo = fif_dlg.dir_combo;
		const gchar *utf8_dir =
			gtk_entry_get_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(dir_combo))));
		GeanyEncodingIndex enc_idx =
			ui_encodings_combo_box_get_active_encoding(GTK_COMBO_BOX(fif_dlg.encoding_combo));

		if (execute_find_in_files_request(search_text, utf8_dir,
				settings.fif_invert_results, settings.fif_case_sensitive,
				settings.fif_match_whole_word, settings.fif_recursive,
				settings.fif_hidden_folders,
				settings.fif_regexp, settings.fif_use_extra_options,
				settings.fif_extra_options, settings.fif_files_mode,
				settings.fif_files, enc_idx, search_combo, dir_combo,
				fif_dlg.files_combo, FALSE, NULL))
		{
			ui_combo_box_add_to_history(GTK_COMBO_BOX_TEXT(search_combo), search_text, 0);
			ui_combo_box_add_to_history(GTK_COMBO_BOX_TEXT(fif_dlg.files_combo), NULL, 0);
			ui_combo_box_add_to_history(GTK_COMBO_BOX_TEXT(dir_combo), utf8_dir, 0);
			gtk_widget_hide(fif_dlg.dialog);
		}
	}
	else
		gtk_widget_hide(fif_dlg.dialog);
}


static gboolean
search_find_in_files(const gchar *utf8_search_text, const gchar *utf8_dir, const gchar *opts,
	const gchar *enc, gboolean recursive)
{
	gchar **argv_prefix, **argv;
	gchar *command_grep;
	gchar *command_line, *dir;
	gchar *search_text = NULL;
	GError *error = NULL;
	gboolean ret = FALSE;
	gssize utf8_text_len;

	if (EMPTY(utf8_search_text) || ! utf8_dir) return TRUE;

	command_grep = g_find_program_in_path(tool_prefs.grep_cmd);
	if (command_grep == NULL)
		command_line = g_strdup_printf("%s %s --", tool_prefs.grep_cmd, opts);
	else
	{
		command_line = g_strdup_printf("\"%s\" %s --", command_grep, opts);
		g_free(command_grep);
	}

	/* convert the search text in the preferred encoding (if the text is not valid UTF-8. assume
	 * it is already in the preferred encoding) */
	utf8_text_len = strlen(utf8_search_text);
	if (enc != NULL && g_utf8_validate(utf8_search_text, utf8_text_len, NULL))
	{
		search_text = g_convert(utf8_search_text, utf8_text_len, enc, "UTF-8", NULL, NULL, NULL);
	}
	if (search_text == NULL)
		search_text = g_strdup(utf8_search_text);

	argv_prefix = g_new(gchar*, 3);
	argv_prefix[0] = search_text;
	dir = utils_get_locale_from_utf8(utf8_dir);

	/* finally add the arguments(files to be searched) */
	if (recursive)	/* recursive option set */
	{
		/* Use '.' so we get relative paths in the output */
		argv_prefix[1] = g_strdup(".");
		argv_prefix[2] = NULL;
		argv = argv_prefix;
	}
	else
	{
		argv_prefix[1] = NULL;
		argv = search_get_argv((const gchar**)argv_prefix, dir);
		g_strfreev(argv_prefix);

		if (argv == NULL)	/* no files */
		{
			g_free(command_line);
			return FALSE;
		}
	}
	reset_msgwin();

	/* we can pass 'enc' without strdup'ing it here because it's a global const string and
	 * always exits longer than the lifetime of this function */
	if (spawn_with_callbacks(dir, command_line, argv, NULL, 0, NULL, NULL, search_read_io,
		(gpointer) enc, 0, search_read_io_stderr, (gpointer) enc, 0, search_finished, NULL,
		NULL, &error))
 	{
		gchar *utf8_str;

 		ui_progress_bar_start(_("Searching..."));
 		msgwin_set_messages_dir(dir);
		utf8_str = g_strdup_printf(_("%s %s -- %s (in directory: %s)"),
			tool_prefs.grep_cmd, opts, utf8_search_text, utf8_dir);
 		msgwin_msg_add_string(COLOR_BLUE, -1, NULL, utf8_str);
		g_free(utf8_str);
 		ret = TRUE;
	}
	else
	{
		ui_set_statusbar(TRUE, _("Cannot execute grep tool \"%s\": %s. "
			"Check the path setting in Preferences."), tool_prefs.grep_cmd, error->message);
		g_error_free(error);
	}

	utils_free_pointers(2, dir, command_line, NULL);
	g_strfreev(argv);
	return ret;
}


static gboolean pattern_list_match(GSList *patterns, const gchar *str)
{
	GSList *item;

	foreach_slist(item, patterns)
	{
		if (g_pattern_match_string(item->data, str))
			return TRUE;
	}
	return FALSE;
}


/* Creates an argument vector of strings, copying argv_prefix[] values for
 * the first arguments, then followed by filenames found in dir.
 * Returns NULL if no files were found, otherwise returned vector should be fully freed. */
static gchar **search_get_argv(const gchar **argv_prefix, const gchar *dir)
{
	guint prefix_len, list_len, i, j;
	gchar **argv;
	GSList *list, *item, *patterns = NULL;
	GError *error = NULL;

	g_return_val_if_fail(dir != NULL, NULL);

	prefix_len = g_strv_length((gchar**)argv_prefix);
	list = utils_get_file_list(dir, &list_len, &error);
	if (error)
	{
		ui_set_statusbar(TRUE, _("Could not open directory (%s)"), error->message);
		g_error_free(error);
		return NULL;
	}
	if (list == NULL)
		return NULL;

	argv = g_new(gchar*, prefix_len + list_len + 1);

	for (i = 0, j = 0; i < prefix_len; i++)
	{
		if (g_str_has_prefix(argv_prefix[i], "--include="))
		{
			const gchar *pat = &(argv_prefix[i][10]); /* the pattern part of the argument */

			patterns = g_slist_prepend(patterns, g_pattern_spec_new(pat));
		}
		else
			argv[j++] = g_strdup(argv_prefix[i]);
	}

	if (patterns)
	{
		GSList *pat;

		foreach_slist(item, list)
		{
			if (pattern_list_match(patterns, item->data))
				argv[j++] = item->data;
			else
				g_free(item->data);
		}
		foreach_slist(pat, patterns)
			g_pattern_spec_free(pat->data);
		g_slist_free(patterns);
	}
	else
	{
		foreach_slist(item, list)
			argv[j++] = item->data;
	}

	argv[j] = NULL;
	g_slist_free(list);
	return argv;
}


static void read_fif_io(gchar *msg, GIOCondition condition, gchar *enc, gint msg_color)
{
	if (condition & (G_IO_IN | G_IO_PRI))
	{
		gchar *utf8_msg = NULL;

		g_strstrip(msg);
		/* enc is NULL when encoding is set to UTF-8, so we can skip any conversion */
		if (enc != NULL)
		{
			if (! g_utf8_validate(msg, -1, NULL))
			{
				utf8_msg = g_convert(msg, -1, "UTF-8", enc, NULL, NULL, NULL);
			}
			if (utf8_msg == NULL)
				utf8_msg = msg;
		}
		else
			utf8_msg = msg;

		msgwin_msg_add_string(msg_color, -1, NULL, utf8_msg);
		if (msg_color == COLOR_BLACK)
			search_studio_capture_grep_result(utf8_msg);

		if (utf8_msg != msg)
			g_free(utf8_msg);
	}
}


static void search_read_io(GString *string, GIOCondition condition, gpointer data)
{
	read_fif_io(string->str, condition, data, COLOR_BLACK);
}


static void search_read_io_stderr(GString *string, GIOCondition condition, gpointer data)
{
	read_fif_io(string->str, condition, data, COLOR_DARK_RED);
}


static void search_finished(GPid child_pid, gint status, gpointer user_data)
{
	const gchar *msg = _("Search failed.");
	gint exit_status;

	if (SPAWN_WIFEXITED(status))
	{
		exit_status = SPAWN_WEXITSTATUS(status);
	}
	else if (SPAWN_WIFSIGNALED(status))
	{
		exit_status = -1;
		g_warning("Find in Files: The command failed unexpectedly (signal received).");
	}
	else
	{
		exit_status = 1;
	}

	search_studio_capture_finish(exit_status);

	switch (exit_status)
	{
		case 0:
		{
			gint count = gtk_tree_model_iter_n_children(
				GTK_TREE_MODEL(msgwindow.store_msg), NULL) - 1;
			gchar *text = g_strdup_printf(ngettext(
						"Search completed with %d match.",
						"Search completed with %d matches.", count),
						count);

			msgwin_msg_add_string(COLOR_BLUE, -1, NULL, text);
			ui_set_statusbar(FALSE, "%s", text);
			g_free(text);
			break;
		}
		case 1:
			msg = _("No matches found.");
			/* fall through */
		default:
			msgwin_msg_add_string(COLOR_BLUE, -1, NULL, msg);
			ui_set_statusbar(FALSE, "%s", msg);
			break;
	}
	utils_beep();
	ui_progress_bar_stop();
}


static GRegex *compile_regex(const gchar *str, GeanyFindFlags sflags)
{
	GRegex *regex;
	GError *error = NULL;
	gint rflags = 0;

	if (sflags & GEANY_FIND_MULTILINE)
		rflags |= G_REGEX_MULTILINE;
	if (sflags & GEANY_FIND_DOTALL)
		rflags |= G_REGEX_DOTALL;
	if (~sflags & GEANY_FIND_MATCHCASE)
		rflags |= G_REGEX_CASELESS;
	if (sflags & (GEANY_FIND_WHOLEWORD | GEANY_FIND_WORDSTART))
	{
		geany_debug("%s: Unsupported regex flags found!", G_STRFUNC);
	}

	regex = g_regex_new(str, rflags, 0, &error);
	if (!regex)
	{
		ui_set_statusbar(FALSE, _("Bad regex: %s"), error->message);
		g_error_free(error);
	}
	return regex;
}


/* groups that don't exist are handled OK as len = end - start = (-1) - (-1) = 0 */
static gchar *get_regex_match_string(const gchar *text, const GeanyMatchInfo *match, guint nth)
{
	const gint start = match->matches[nth].start;
	const gint end = match->matches[nth].end;
	return g_strndup(&text[start], end - start);
}


static gint find_regex(ScintillaObject *sci, guint pos, GRegex *regex, gboolean multiline, GeanyMatchInfo *match)
{
	const gchar *text;
	GMatchInfo *minfo;
	guint document_length;
	gint ret = -1;
	gint offset = 0;

	document_length = (guint)sci_get_length(sci);
	if (document_length <= 0)
		return -1; /* skip empty documents */

	g_return_val_if_fail(pos <= document_length, -1);

	if (multiline)
	{
		/* Warning: any SCI calls will invalidate 'text' after calling SCI_GETCHARACTERPOINTER */
		text = (void*)SSM(sci, SCI_GETCHARACTERPOINTER, 0, 0);
		g_regex_match_full(regex, text, -1, pos, 0, &minfo, NULL);
	}
	else /* single-line mode, manually match against each line */
	{
		gint line = sci_get_line_from_position(sci, pos);

		for (;;)
		{
			gint start = sci_get_position_from_line(sci, line);
			gint end = sci_get_line_end_position(sci, line);

			text = (void*)SSM(sci, SCI_GETRANGEPOINTER, start, end - start);
			if (g_regex_match_full(regex, text, end - start, pos - start, 0, &minfo, NULL))
			{
				offset = start;
				break;
			}
			else /* not found, try next line */
			{
				line ++;
				if (line >= sci_get_line_count(sci))
					break;
				pos = sci_get_position_from_line(sci, line);
				/* don't free last info, it's freed below */
				g_match_info_free(minfo);
			}
		}
	}

	/* Warning: minfo will become invalid when 'text' does! */
	if (g_match_info_matches(minfo))
	{
		guint i;

		/* copy whole match text and offsets before they become invalid */
		SETPTR(match->match_text, g_match_info_fetch(minfo, 0));

		foreach_range(i, G_N_ELEMENTS(match->matches))
		{
			gint start = -1, end = -1;

			g_match_info_fetch_pos(minfo, (gint)i, &start, &end);
			match->matches[i].start = offset + start;
			match->matches[i].end = offset + end;
		}
		match->start = match->matches[0].start;
		match->end = match->matches[0].end;
		ret = match->start;
	}
	g_match_info_free(minfo);
	return ret;
}


static gint geany_find_flags_to_sci_flags(GeanyFindFlags flags)
{
	g_warn_if_fail(! (flags & GEANY_FIND_REGEXP) || ! (flags & GEANY_FIND_MULTILINE));

	return ((flags & GEANY_FIND_MATCHCASE) ? SCFIND_MATCHCASE : 0) |
		((flags & GEANY_FIND_WHOLEWORD) ? SCFIND_WHOLEWORD : 0) |
		((flags & GEANY_FIND_REGEXP) ? SCFIND_REGEXP | SCFIND_POSIX : 0) |
		((flags & GEANY_FIND_WORDSTART) ? SCFIND_WORDSTART : 0);
}


gint search_find_prev(ScintillaObject *sci, const gchar *str, GeanyFindFlags flags, GeanyMatchInfo **match_)
{
	gint ret;

	g_return_val_if_fail(! (flags & GEANY_FIND_REGEXP), -1);

	ret = sci_search_prev(sci, geany_find_flags_to_sci_flags(flags), str);
	if (ret != -1 && match_)
		*match_ = match_info_new(flags, ret, ret + strlen(str));
	return ret;
}


gint search_find_next(ScintillaObject *sci, const gchar *str, GeanyFindFlags flags, GeanyMatchInfo **match_)
{
	GeanyMatchInfo *match;
	GRegex *regex;
	gint ret = -1;
	gint pos;

	if (~flags & GEANY_FIND_REGEXP)
	{
		ret = sci_search_next(sci, geany_find_flags_to_sci_flags(flags), str);
		if (ret != -1 && match_)
			*match_ = match_info_new(flags, ret, ret + strlen(str));
		return ret;
	}

	regex = compile_regex(str, flags);
	if (!regex)
		return -1;

	match = match_info_new(flags, 0, 0);

	pos = sci_get_current_position(sci);
	ret = find_regex(sci, pos, regex, flags & GEANY_FIND_MULTILINE, match);
	/* avoid re-matching the same position in case of empty matches */
	if (ret == pos && match->matches[0].start == match->matches[0].end)
		ret = find_regex(sci, pos + 1, regex, flags & GEANY_FIND_MULTILINE, match);
	if (ret >= 0)
		sci_set_selection(sci, match->start, match->end);

	if (ret != -1 && match_)
		*match_ = match;
	else
		geany_match_info_free(match);

	g_regex_unref(regex);
	return ret;
}


gint search_replace_match(ScintillaObject *sci, const GeanyMatchInfo *match, const gchar *replace_text)
{
	gchar *resolved_text;
	gint ret;

	sci_set_target_start(sci, match->start);
	sci_set_target_end(sci, match->end);

	resolved_text = search_build_replacement_text(match, replace_text);
	ret = sci_replace_target(sci, resolved_text, FALSE);
	g_free(resolved_text);
	return ret;
}


gint search_find_text(ScintillaObject *sci, GeanyFindFlags flags, struct Sci_TextToFind *ttf, GeanyMatchInfo **match_)
{
	GeanyMatchInfo *match = NULL;
	GRegex *regex;
	gint ret;

	if (~flags & GEANY_FIND_REGEXP)
	{
		ret = sci_find_text(sci, geany_find_flags_to_sci_flags(flags), ttf);
		if (ret != -1 && match_)
			*match_ = match_info_new(flags, ttf->chrgText.cpMin, ttf->chrgText.cpMax);
		return ret;
	}

	regex = compile_regex(ttf->lpstrText, flags);
	if (!regex)
		return -1;

	match = match_info_new(flags, 0, 0);

	ret = find_regex(sci, ttf->chrg.cpMin, regex, flags & GEANY_FIND_MULTILINE, match);
	if (ret >= ttf->chrg.cpMax)
		ret = -1;
	else if (ret >= 0)
	{
		ttf->chrgText.cpMin = match->start;
		ttf->chrgText.cpMax = match->end;
	}

	if (ret != -1 && match_)
		*match_ = match;
	else
		geany_match_info_free(match);

	g_regex_unref(regex);
	return ret;
}


static gint find_document_usage(GeanyDocument *doc, const gchar *search_text, GeanyFindFlags flags)
{
	gchar *buffer, *short_file_name;
	struct Sci_TextToFind ttf;
	gint count = 0;
	gint prev_line = -1;
	GSList *match, *matches;

	g_return_val_if_fail(DOC_VALID(doc), 0);

	short_file_name = g_path_get_basename(DOC_FILENAME(doc));

	ttf.chrg.cpMin = 0;
	ttf.chrg.cpMax = sci_get_length(doc->editor->sci);
	ttf.lpstrText = (gchar *)search_text;

	matches = find_range(doc->editor->sci, flags, &ttf);
	foreach_slist (match, matches)
	{
		GeanyMatchInfo *info = match->data;
		gint line = sci_get_line_from_position(doc->editor->sci, info->start);

		if (line != prev_line)
		{
			buffer = sci_get_line(doc->editor->sci, line);
			msgwin_msg_add(COLOR_BLACK, line + 1, doc,
				"%s:%d: %s", short_file_name, line + 1, g_strstrip(buffer));
			g_free(buffer);
			prev_line = line;
		}
		count++;

		geany_match_info_free(info);
	}
	g_slist_free(matches);
	g_free(short_file_name);
	return count;
}


void search_find_usage(const gchar *search_text, const gchar *original_search_text,
		GeanyFindFlags flags, gboolean in_session)
{
	GeanyDocument *doc;
	gint count = 0;

	doc = document_get_current();
	g_return_if_fail(doc != NULL);

	if (G_UNLIKELY(EMPTY(search_text)))
	{
		utils_beep();
		return;
	}
	reset_msgwin();

	if (! in_session)
	{	/* use current document */
		count = find_document_usage(doc, search_text, flags);
	}
	else
	{
		guint i;
		for (i = 0; i < documents_array->len; i++)
		{
			if (documents[i]->is_valid)
			{
				count += find_document_usage(documents[i], search_text, flags);
			}
		}
	}

	if (count == 0) /* no matches were found */
	{
		ui_set_statusbar(FALSE, _("No matches found for \"%s\"."), original_search_text);
		msgwin_msg_add(COLOR_BLUE, -1, NULL, _("No matches found for \"%s\"."), original_search_text);
	}
	else
	{
		ui_set_statusbar(FALSE, ngettext(
			"Found %d match for \"%s\".", "Found %d matches for \"%s\".", count),
			count, original_search_text);
		msgwin_msg_add(COLOR_BLUE, -1, NULL, ngettext(
			"Found %d match for \"%s\".", "Found %d matches for \"%s\".", count),
			count, original_search_text);
	}
}


/* ttf is updated to include the last match position (ttf->chrg.cpMin) and
 * the new search range end (ttf->chrg.cpMax).
 * Note: Normally you would call sci_start/end_undo_action() around this call. */
guint search_replace_range(ScintillaObject *sci, struct Sci_TextToFind *ttf,
		GeanyFindFlags flags, const gchar *replace_text)
{
	gint count = 0;
	gint offset = 0; /* difference between search pos and replace pos */
	GSList *match, *matches;

	g_return_val_if_fail(sci != NULL && ttf->lpstrText != NULL && replace_text != NULL, 0);
	if (! *ttf->lpstrText)
		return 0;

	matches = find_range(sci, flags, ttf);
	foreach_slist (match, matches)
	{
		GeanyMatchInfo *info = match->data;
		gint replace_len;

		info->start += offset;
		info->end += offset;

		replace_len = search_replace_match(sci, info, replace_text);
		offset += replace_len - (info->end - info->start);
		count ++;

		/* on last match, update the last match/new range end */
		if (! match->next)
		{
			ttf->chrg.cpMin = info->start;
			ttf->chrg.cpMax += offset;
		}

		geany_match_info_free(info);
	}
	g_slist_free(matches);

	return count;
}


void search_find_again(gboolean change_direction)
{
	GeanyDocument *doc = document_get_current();

	g_return_if_fail(doc != NULL);

	if (search_data.text)
	{
		gboolean forward = ! search_data.backwards;
		gint result = document_find_text(doc, search_data.text, search_data.original_text, search_data.flags,
			change_direction ? forward : !forward, NULL, FALSE);

		if (result > -1)
			editor_display_current_line(doc->editor, 0.3F);

		if (search_data.search_bar)
			ui_set_search_entry_background(
				toolbar_get_widget_child_by_name("SearchEntry"), (result > -1));
	}
}
