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
	GtkWidget	*mark_page;
	GtkWidget	*lower_notebook;
	GtkWidget	*activity_view;
	GtkWidget	*results_view;
	GtkWidget	*preview_view;
	GtkListStore	*results_store;
	gint		position[2]; /* x, y */
}
studio_dlg = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, {0, 0}};

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
static gchar *search_studio_document_target(GeanyDocument *doc);
static void search_studio_append_document_result(const gchar *action, GeanyDocument *doc,
	const gchar *query, const gchar *mode, const gchar *summary);
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
static guint search_studio_append_replace_preview_session_rows(const gchar *query,
	GeanyFindFlags flags, const gchar *mode, const gchar *replace_text,
	const gchar *replace_display, guint per_doc_limit);
static guint search_studio_append_replace_impact_rows(const gchar *action, GeanyDocument *doc,
	const gchar *query, GeanyFindFlags flags, const gchar *mode, const gchar *replace_text,
	const gchar *replace_display);
static guint search_studio_append_replace_impact_session_rows(const gchar *action,
	const gchar *query, GeanyFindFlags flags, const gchar *mode, const gchar *replace_text,
	const gchar *replace_display, guint *doc_count);
static void search_studio_fif_find_activate(GtkButton *button, gpointer user_data);
static void search_studio_fif_file_mode_changed(GtkComboBox *combo, gpointer user_data);
static void search_studio_notebook_switch_page(GtkNotebook *notebook, GtkWidget *page,
	guint page_num, gpointer user_data);
static void search_studio_results_row_activated(GtkTreeView *tree_view, GtkTreePath *path,
	GtkTreeViewColumn *column, gpointer user_data);
static void search_studio_results_selection_changed(GtkTreeSelection *selection, gpointer user_data);
static void search_studio_activity_append(const gchar *format, ...) G_GNUC_PRINTF(1, 2);
static void search_studio_set_preview(const gchar *title, const gchar *body);
static void search_studio_result_append_spec(const SearchStudioResultSpec *spec);
static void search_studio_result_append(const gchar *action, const gchar *target,
	const gchar *query, const gchar *mode, const gchar *summary);
static gchar *search_studio_build_line_preview_body(GeanyDocument *doc, gint pos,
	const gchar *query, const gchar *kind_label);
static void search_studio_result_append_match(const gchar *action, GeanyDocument *doc,
	const gchar *query, const gchar *mode, gint pos, const gchar *summary);
static void search_studio_result_append_preview_match(const gchar *action, GeanyDocument *doc,
	const gchar *query, const gchar *mode, gint pos, const gchar *summary,
	const gchar *preview_title, const gchar *preview_body);
static guint search_studio_append_match_rows(const gchar *action, GeanyDocument *doc,
	const gchar *query, GeanyFindFlags flags, const gchar *mode, guint limit);
static guint search_studio_append_session_match_rows(const gchar *action,
	const gchar *query, GeanyFindFlags flags, const gchar *mode, guint per_doc_limit);
static void search_studio_clear_results(GtkButton *button, gpointer user_data);
static void search_studio_collect_document_hits(GtkButton *button, gpointer user_data);
static void search_studio_collect_session_hits(GtkButton *button, gpointer user_data);
static guint search_studio_append_count_impact_row(const gchar *action, GeanyDocument *doc,
	const gchar *query, GeanyFindFlags flags, const gchar *mode);
static void search_studio_count_session_activate(GtkButton *button, gpointer user_data);
static guint search_studio_append_mark_impact_row(const gchar *action, GeanyDocument *doc,
	const gchar *query, GeanyFindFlags flags, const gchar *mode,
	gboolean bookmark_lines, gboolean purge_bookmarks);
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
	GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
	GtkWidget *normal = gtk_radio_button_new_with_mnemonic(NULL, _("_Normal"));
	GtkWidget *extended = gtk_radio_button_new_with_mnemonic_from_widget(GTK_RADIO_BUTTON(normal), _("_Extended"));
	GtkWidget *regex = gtk_radio_button_new_with_mnemonic_from_widget(GTK_RADIO_BUTTON(normal), _("Re_gex"));

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
	GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
	GtkWidget *mode_label = ui_label_new_bold(_("Search mode"));
	GtkWidget *mode_box = search_studio_create_mode_box(owner);
	GtkWidget *options_grid = gtk_grid_new();
	GtkWidget *check_case = gtk_check_button_new_with_mnemonic(_("Match _case"));
	GtkWidget *check_word = gtk_check_button_new_with_mnemonic(_("Whole _word"));
	GtkWidget *check_wordstart = gtk_check_button_new_with_mnemonic(_("Word _start"));
	GtkWidget *check_wrap = gtk_check_button_new_with_mnemonic(_("Wrap ar_ound"));
	GtkWidget *check_multiline = gtk_check_button_new_with_mnemonic(_("Multi-_line regex"));
	GtkWidget *check_dotall = gtk_check_button_new_with_mnemonic(_("._ matches newline"));

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

	gtk_box_pack_start(GTK_BOX(box), mode_label, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(box), mode_box, FALSE, FALSE, 0);
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


static void search_studio_activity_append(const gchar *format, ...)
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


static guint search_studio_append_session_match_rows(const gchar *action,
	const gchar *query, GeanyFindFlags flags, const gchar *mode, guint per_doc_limit)
{
	guint page_count;
	guint n;
	guint total = 0;

	page_count = gtk_notebook_get_n_pages(GTK_NOTEBOOK(main_widgets.notebook));
	for (n = 0; n < page_count; n++)
	{
		GeanyDocument *doc = document_get_from_page(n);

		if (!DOC_VALID(doc))
			continue;
		total += search_studio_append_match_rows(action, doc, query, flags, mode, per_doc_limit);
	}
	return total;
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


static guint search_studio_append_replace_preview_session_rows(const gchar *query,
	GeanyFindFlags flags, const gchar *mode, const gchar *replace_text,
	const gchar *replace_display, guint per_doc_limit)
{
	guint page_count;
	guint n;
	guint total = 0;

	page_count = gtk_notebook_get_n_pages(GTK_NOTEBOOK(main_widgets.notebook));
	for (n = 0; n < page_count; n++)
	{
		GeanyDocument *doc = document_get_from_page(n);

		if (!DOC_VALID(doc))
			continue;
		total += search_studio_append_replace_preview_rows(doc, query, flags, mode,
			replace_text, replace_display, per_doc_limit);
	}
	return total;
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


static guint search_studio_append_replace_impact_session_rows(const gchar *action,
	const gchar *query, GeanyFindFlags flags, const gchar *mode, const gchar *replace_text,
	const gchar *replace_display, guint *doc_count)
{
	guint page_count;
	guint n;
	guint total = 0;
	guint docs = 0;

	page_count = gtk_notebook_get_n_pages(GTK_NOTEBOOK(main_widgets.notebook));
	for (n = 0; n < page_count; n++)
	{
		GeanyDocument *doc = document_get_from_page(n);
		guint count;

		if (!DOC_VALID(doc))
			continue;
		count = search_studio_append_replace_impact_rows(action, doc, query, flags, mode,
			replace_text, replace_display);
		if (count > 0)
		{
			total += count;
			docs++;
		}
	}
	if (doc_count != NULL)
		*doc_count = docs;
	return total;
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
		gchar *target = g_path_get_basename(DOC_FILENAME(doc));
		gchar *summary = g_strdup_printf("Collected %u navigable hits from the active document.", count);
		search_studio_result_append("Collect Hits", target, spec.original_text, spec.mode, summary);
		g_free(summary);
		g_free(target);
	}
	search_studio_find_spec_clear(&spec);
}


static void search_studio_collect_session_hits(GtkButton *button, gpointer user_data)
{
	GtkWidget *page = GTK_WIDGET(user_data);
	SearchStudioFindSpec spec = { 0 };
	guint count;

	if (!search_studio_build_find_spec(page, "entry_search", &spec))
		return;

	count = search_studio_append_session_match_rows("Session Hit", spec.text, spec.flags,
		spec.mode, 100);
	search_studio_activity_append("[Results] Collected %u open-document hits for %s.", count, spec.original_text);
	{
		gchar *summary = g_strdup_printf("Collected %u navigable hits across open documents.", count);
		search_studio_result_append("Collect Session Hits", "Open Documents", spec.original_text,
			spec.mode, summary);
		g_free(summary);
	}
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


static void search_studio_activity_show_page_hint(gint page_num)
{
	static const gchar *hints[] = {
		"Find: use the dense search cockpit for repeated lookup, counting, and bookmark-marking.",
		"Replace: execute targeted or bulk replacements directly, then fall back to the classic dialog if needed.",
		"Find in Files: launch directory searches directly from Search Studio and review detailed results in the message window.",
		"Mark: highlight all matches, optionally bookmark matching lines, fan marking out across open documents, then clear everything in one click."
	};

	if (page_num >= 0 && page_num < (gint) G_N_ELEMENTS(hints))
		search_studio_activity_append("[%s] %s", page_num == 0 ? "Find" : page_num == 1 ? "Replace" : page_num == 2 ? "Find in Files" : "Mark", hints[page_num]);
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


static void search_studio_on_response(GtkDialog *dialog, gint response, gpointer user_data)
{
	gtk_window_get_position(GTK_WINDOW(studio_dlg.dialog),
		&studio_dlg.position[0], &studio_dlg.position[1]);
	gtk_widget_hide(studio_dlg.dialog);
}


static gboolean search_studio_hide_on_delete(GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
	gtk_window_get_position(GTK_WINDOW(studio_dlg.dialog),
		&studio_dlg.position[0], &studio_dlg.position[1]);
	gtk_widget_hide(studio_dlg.dialog);
	return TRUE;
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
	if (count == 0)
		ui_set_statusbar(FALSE, _("No matches found for \"%s\"."), spec.original_text);
	else
		ui_set_statusbar(FALSE,
			ngettext("Counted %d match for \"%s\".", "Counted %d matches for \"%s\".", count),
			count, spec.original_text);

	search_studio_activity_append("[Count] %s | mode=%s | matches=%d",
		spec.original_text, spec.mode, count);
	{
		gchar *summary = g_strdup_printf("Counted %d matches in the active document.", count);
		search_studio_append_document_result("Count", doc, spec.original_text, spec.mode, summary);
		search_studio_append_count_impact_row("Count Impact", doc, spec.text, spec.flags, spec.mode);
		search_studio_append_match_rows("Count Match", doc, spec.text, spec.flags, spec.mode, 50);
		g_free(summary);
	}
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

	if (count == 0)
		ui_set_statusbar(FALSE, _("No matches found for \"%s\"."), spec.original_text);
	else if (bookmark_lines)
		ui_set_statusbar(FALSE,
			ngettext("Marked %d match for \"%s\" and bookmarked its line.",
				"Marked %d matches for \"%s\" and bookmarked their lines.", count),
			count, spec.original_text);
	else
		ui_set_statusbar(FALSE,
			ngettext("Marked %d match for \"%s\".", "Marked %d matches for \"%s\".", count),
			count, spec.original_text);

	search_studio_activity_append("[Mark] %s | mode=%s | matches=%d | bookmarks=%s | purge=%s",
		spec.original_text, spec.mode, count,
		bookmark_lines ? "on" : "off", purge_bookmarks ? "on" : "off");
	{
		gchar *summary = g_strdup_printf("Marked %d matches; bookmark-lines=%s; purge-first=%s.",
			count, bookmark_lines ? "yes" : "no", purge_bookmarks ? "yes" : "no");
		search_studio_append_document_result("Mark", doc, spec.original_text, spec.mode, summary);
		search_studio_append_mark_impact_row("Mark Impact", doc, spec.text, spec.flags,
			spec.mode, bookmark_lines, purge_bookmarks);
		search_studio_append_match_rows("Mark Match", doc, spec.text, spec.flags, spec.mode, 50);
		g_free(summary);
	}
	search_studio_store_find_spec(&spec);
	search_studio_find_spec_clear(&spec);
}


static void search_studio_count_session_activate(GtkButton *button, gpointer user_data)
{
	GtkWidget *page = GTK_WIDGET(user_data);
	SearchStudioFindSpec spec = { 0 };
	guint page_count;
	guint n;
	guint docs_counted = 0;
	guint total_matches = 0;

	if (!search_studio_build_find_spec(page, "entry_search", &spec))
		return;

	search_studio_add_find_history(page, &spec);

	page_count = gtk_notebook_get_n_pages(GTK_NOTEBOOK(main_widgets.notebook));
	for (n = 0; n < page_count; n++)
	{
		GeanyDocument *doc = document_get_from_page(n);
		guint count;

		if (!DOC_VALID(doc))
			continue;
		count = search_studio_append_count_impact_row("Session Count Impact", doc, spec.text, spec.flags,
			spec.mode);
		if (count > 0)
		{
			docs_counted++;
			total_matches += count;
		}
	}

	if (total_matches == 0)
		ui_set_statusbar(FALSE, _("No matches found for \"%s\" across open documents."), spec.original_text);
	else
		ui_set_statusbar(FALSE, _("Counted %u matches across %u open documents for \"%s\"."),
			total_matches, docs_counted, spec.original_text);

	search_studio_activity_append("[Count] Session | query=%s | mode=%s | matches=%u | docs=%u",
		spec.original_text, spec.mode, total_matches, docs_counted);
	{
		gchar *summary = g_strdup_printf("Counted %u matches across %u open documents.",
			total_matches, docs_counted);
		search_studio_result_append("Count in Session", "Open Documents", spec.original_text,
			spec.mode, summary);
		g_free(summary);
	}
	search_studio_store_find_spec(&spec);
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
	{
		gchar *target = g_path_get_basename(DOC_FILENAME(doc));
		search_studio_result_append("Mark", target, "Clear", "N/A", "Cleared search highlights and bookmarked lines.");
		g_free(target);
	}
}


static void search_studio_mark_session_activate(GtkButton *button, gpointer user_data)
{
	GtkWidget *page = GTK_WIDGET(user_data);
	SearchStudioFindSpec spec = { 0 };
	gboolean bookmark_lines;
	gboolean purge_bookmarks;
	guint page_count;
	guint n;
	guint docs_marked = 0;
	guint total_matches = 0;

	if (!search_studio_build_find_spec(page, "entry_search", &spec))
		return;

	bookmark_lines = ui_lookup_widget(page, "check_bookmark") &&
		gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ui_lookup_widget(page, "check_bookmark")));
	purge_bookmarks = ui_lookup_widget(page, "check_purge") &&
		gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ui_lookup_widget(page, "check_purge")));
	search_studio_add_find_history(page, &spec);

	page_count = gtk_notebook_get_n_pages(GTK_NOTEBOOK(main_widgets.notebook));
	for (n = 0; n < page_count; n++)
	{
		GeanyDocument *doc = document_get_from_page(n);
		guint count;

		if (!DOC_VALID(doc))
			continue;
		count = search_mark_all_with_options(doc, spec.text, spec.flags, bookmark_lines, purge_bookmarks);
		if (count > 0)
		{
			docs_marked++;
			total_matches += count;
			search_studio_append_mark_impact_row("Session Mark Impact", doc, spec.text, spec.flags,
				spec.mode, bookmark_lines, purge_bookmarks);
		}
	}

	if (total_matches == 0)
		ui_set_statusbar(FALSE, _("No matches found for \"%s\" across open documents."), spec.original_text);
	else
		ui_set_statusbar(FALSE, _("Marked %u matches across %u open documents for \"%s\"."),
			total_matches, docs_marked, spec.original_text);

	search_studio_activity_append("[Mark] Session | query=%s | mode=%s | matches=%u | docs=%u | bookmarks=%s | purge=%s",
		spec.original_text, spec.mode, total_matches, docs_marked,
		bookmark_lines ? "on" : "off", purge_bookmarks ? "on" : "off");
	{
		gchar *summary = g_strdup_printf("Marked %u matches across %u open documents; bookmark-lines=%s; purge-first=%s.",
			total_matches, docs_marked, bookmark_lines ? "yes" : "no", purge_bookmarks ? "yes" : "no");
		search_studio_result_append("Mark in Session", "Open Documents", spec.original_text,
			spec.mode, summary);
		g_free(summary);
	}
	search_studio_store_find_spec(&spec);
	search_studio_find_spec_clear(&spec);
}


static void search_studio_clear_session_marks_activate(GtkButton *button, gpointer user_data)
{
	guint page_count;
	guint n;
	guint cleared_docs = 0;

	page_count = gtk_notebook_get_n_pages(GTK_NOTEBOOK(main_widgets.notebook));
	for (n = 0; n < page_count; n++)
	{
		GeanyDocument *doc = document_get_from_page(n);

		if (!DOC_VALID(doc))
			continue;
		search_clear_all_marks(doc);
		cleared_docs++;
	}

	ui_set_statusbar(FALSE, _("Cleared search highlights and bookmarks across %u open documents."),
		cleared_docs);
	search_studio_activity_append("[Mark] Cleared search highlights and bookmarks across %u open documents.",
		cleared_docs);
	{
		gchar *summary = g_strdup_printf("Cleared search highlights and bookmarks across %u open documents.",
			cleared_docs);
		search_studio_result_append("Mark in Session", "Open Documents", "Clear", "N/A", summary);
		g_free(summary);
	}
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
	{
		gchar *summary = g_strdup_printf("Would replace %u matches in the active document.", count);
		search_studio_append_document_result("Replace Preview", doc, spec.find.original_text, spec.find.mode, summary);
		g_free(summary);
	}
	search_studio_replace_spec_clear(&spec);
}


static void search_studio_replace_preview_session(GtkButton *button, gpointer user_data)
{
	GtkWidget *page = GTK_WIDGET(user_data);
	SearchStudioReplaceSpec spec = { 0 };
	guint count;

	if (!search_studio_build_replace_spec(page, &spec))
		return;

	count = search_studio_append_replace_preview_session_rows(spec.find.text, spec.find.flags,
		spec.find.mode, spec.replace, spec.original_replace, 100);
	search_studio_activity_append("[Replace Preview] Session | find=%s | replace=%s | matches=%u | mode=%s",
		spec.find.original_text, spec.original_replace, count, spec.find.mode);
	{
		gchar *summary = g_strdup_printf("Would replace %u matches across open documents.", count);
		search_studio_result_append("Replace Preview Session", "Open Documents", spec.find.original_text,
			spec.find.mode, summary);
		g_free(summary);
	}
	search_studio_replace_spec_clear(&spec);
}


static void search_studio_replace_action_activate(GtkButton *button, gpointer user_data)
{
	GtkWidget *page = GTK_WIDGET(user_data);
	GeanyDocument *doc = document_get_current();
	SearchStudioReplaceSpec spec = { 0 };
	gint action = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(button), "studio-replace-action"));

	if (!DOC_VALID(doc) || !search_studio_build_replace_spec(page, &spec))
		return;

	search_studio_store_find_spec(&spec.find);
	search_studio_add_replace_history(page, &spec);

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
			guint planned = search_studio_append_replace_impact_rows("Replace Impact", doc,
				spec.find.text, spec.find.flags, spec.find.mode, spec.replace, spec.original_replace);
			gint reps = document_replace_all(doc, spec.find.text, spec.replace,
				spec.find.original_text, spec.original_replace, spec.find.flags);
			gchar *summary = g_strdup_printf("Replaced %d matches in the active document (planned hits: %u).", reps, planned);
			if (!reps)
				utils_beep();
			search_studio_activity_append("[Replace] Replace in document | find=%s | replace=%s | replacements=%d | planned=%u",
				spec.find.original_text, spec.original_replace, reps, planned);
			search_studio_append_document_result("Replace in Document", doc, spec.find.original_text, spec.find.mode, summary);
			g_free(summary);
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
				guint planned_docs = 0;
				guint planned_matches = search_studio_append_replace_impact_session_rows("Session Replace Impact",
					spec.find.text, spec.find.flags, spec.find.mode, spec.replace, spec.original_replace, &planned_docs);
				replace_in_session(doc, spec.find.flags, FALSE, spec.find.text, spec.replace,
					spec.find.original_text, spec.original_replace);
				search_studio_activity_append("[Replace] Replace in session | find=%s | replace=%s | mode=%s | planned-docs=%u | planned-matches=%u",
					spec.find.original_text, spec.original_replace, spec.find.mode, planned_docs, planned_matches);
				{
					gchar *summary = g_strdup_printf("Applied replacement across open documents (planned docs: %u, planned matches: %u).",
						planned_docs, planned_matches);
					search_studio_result_append("Replace in Session", "Session", spec.find.original_text, spec.find.mode,
						summary);
					g_free(summary);
				}
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
	{
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
	else
		gtk_widget_set_sensitive(entry, TRUE);
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
			whole_word, recursive, regexp, use_extra, extra_options, files_mode, files,
			enc_idx, search_entry, dir_entry, files_entry, TRUE, search_studio_mode_name(page)))
	{
		ui_set_search_entry_background(search_entry, TRUE);
		ui_set_search_entry_background(dir_entry, TRUE);
		ui_set_search_entry_background(files_entry, TRUE);
		ui_set_statusbar(FALSE, _("Searching in files started from Search Studio."));
		search_studio_activity_append("[Find in Files] text=%s | dir=%s | mode=%s | recursive=%s | case=%s | whole-word=%s | invert=%s | files-mode=%d",
			search_text, utf8_dir, search_studio_mode_name(page), recursive ? "on" : "off",
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


static GtkWidget *search_studio_create_find_page(void)
{
	GtkWidget *page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
	GtkWidget *label = gtk_label_new_with_mnemonic(_("_Find what:"));
	GtkWidget *entry = gtk_combo_box_text_new_with_entry();
	GtkWidget *row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
	GtkWidget *actions = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
	GtkWidget *button;

	ui_hookup_widget(page, entry, "combo_search");
	ui_hookup_widget(page, gtk_bin_get_child(GTK_BIN(entry)), "entry_search");
	ui_entry_add_clear_icon(GTK_ENTRY(ui_lookup_widget(page, "entry_search")));
	gtk_label_set_mnemonic_widget(GTK_LABEL(label), entry);
	gtk_box_pack_start(GTK_BOX(row), label, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(row), entry, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(page), row, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(page), search_studio_create_common_options(page, TRUE, TRUE), FALSE, FALSE, 0);

	button = ui_button_new_with_image(GTK_STOCK_FIND, _("Find _Next"));
	gtk_box_pack_start(GTK_BOX(actions), button, FALSE, FALSE, 0);
	g_signal_connect(button, "clicked", G_CALLBACK(search_studio_find_next_activate), page);
	button = ui_button_new_with_image(GTK_STOCK_GO_BACK, _("Find _Previous"));
	gtk_box_pack_start(GTK_BOX(actions), button, FALSE, FALSE, 0);
	g_signal_connect(button, "clicked", G_CALLBACK(search_studio_find_previous_activate), page);
	button = gtk_button_new_with_mnemonic(_("Co_unt"));
	gtk_box_pack_start(GTK_BOX(actions), button, FALSE, FALSE, 0);
	g_signal_connect(button, "clicked", G_CALLBACK(search_studio_count_activate), page);
	button = gtk_button_new_with_mnemonic(_("Count S_ession"));
	gtk_box_pack_start(GTK_BOX(actions), button, FALSE, FALSE, 0);
	g_signal_connect(button, "clicked", G_CALLBACK(search_studio_count_session_activate), page);
	button = gtk_button_new_with_mnemonic(_("_Mark / Bookmark"));
	gtk_box_pack_start(GTK_BOX(actions), button, FALSE, FALSE, 0);
	g_signal_connect(button, "clicked", G_CALLBACK(search_studio_mark_activate), page);
	button = gtk_button_new_with_mnemonic(_("Collect _Document Hits"));
	gtk_box_pack_start(GTK_BOX(actions), button, FALSE, FALSE, 0);
	g_signal_connect(button, "clicked", G_CALLBACK(search_studio_collect_document_hits), page);
	button = gtk_button_new_with_mnemonic(_("Collect Sessi_on Hits"));
	gtk_box_pack_start(GTK_BOX(actions), button, FALSE, FALSE, 0);
	g_signal_connect(button, "clicked", G_CALLBACK(search_studio_collect_session_hits), page);
	button = gtk_button_new_with_mnemonic(_("Clear _Results"));
	gtk_box_pack_start(GTK_BOX(actions), button, FALSE, FALSE, 0);
	g_signal_connect(button, "clicked", G_CALLBACK(search_studio_clear_results), page);
	button = gtk_button_new_with_mnemonic(_("Open classic _Find dialog"));
	gtk_box_pack_start(GTK_BOX(actions), button, FALSE, FALSE, 0);
	g_signal_connect(button, "clicked", G_CALLBACK(search_studio_open_find_dialog_activate), page);

	gtk_box_pack_start(GTK_BOX(page), actions, FALSE, FALSE, 0);
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
	gtk_grid_attach(GTK_GRID(grid), label_find, 0, 0, 1, 1);
	gtk_grid_attach(GTK_GRID(grid), find_combo, 1, 0, 1, 1);
	gtk_grid_attach(GTK_GRID(grid), label_replace, 0, 1, 1, 1);
	gtk_grid_attach(GTK_GRID(grid), replace_combo, 1, 1, 1, 1);
	gtk_box_pack_start(GTK_BOX(page), grid, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(page), search_studio_create_common_options(page, TRUE, FALSE), FALSE, FALSE, 0);

	button = ui_button_new_with_image(GTK_STOCK_FIND, _("Find _Next"));
	g_object_set_data(G_OBJECT(button), "studio-replace-action", GINT_TO_POINTER(GEANY_RESPONSE_FIND));
	gtk_box_pack_start(GTK_BOX(actions), button, FALSE, FALSE, 0);
	g_signal_connect(button, "clicked", G_CALLBACK(search_studio_replace_action_activate), page);
	button = gtk_button_new_with_mnemonic(_("_Replace"));
	g_object_set_data(G_OBJECT(button), "studio-replace-action", GINT_TO_POINTER(GEANY_RESPONSE_REPLACE));
	gtk_box_pack_start(GTK_BOX(actions), button, FALSE, FALSE, 0);
	g_signal_connect(button, "clicked", G_CALLBACK(search_studio_replace_action_activate), page);
	button = gtk_button_new_with_mnemonic(_("Replace & Fi_nd"));
	g_object_set_data(G_OBJECT(button), "studio-replace-action", GINT_TO_POINTER(GEANY_RESPONSE_REPLACE_AND_FIND));
	gtk_box_pack_start(GTK_BOX(actions), button, FALSE, FALSE, 0);
	g_signal_connect(button, "clicked", G_CALLBACK(search_studio_replace_action_activate), page);
	button = gtk_button_new_with_mnemonic(_("Replace in _Document"));
	g_object_set_data(G_OBJECT(button), "studio-replace-action", GINT_TO_POINTER(GEANY_RESPONSE_REPLACE_IN_FILE));
	gtk_box_pack_start(GTK_BOX(actions), button, FALSE, FALSE, 0);
	g_signal_connect(button, "clicked", G_CALLBACK(search_studio_replace_action_activate), page);
	button = gtk_button_new_with_mnemonic(_("Replace in _Selection"));
	g_object_set_data(G_OBJECT(button), "studio-replace-action", GINT_TO_POINTER(GEANY_RESPONSE_REPLACE_IN_SEL));
	gtk_box_pack_start(GTK_BOX(actions), button, FALSE, FALSE, 0);
	g_signal_connect(button, "clicked", G_CALLBACK(search_studio_replace_action_activate), page);
	button = gtk_button_new_with_mnemonic(_("Replace in Sessi_on"));
	g_object_set_data(G_OBJECT(button), "studio-replace-action", GINT_TO_POINTER(GEANY_RESPONSE_REPLACE_IN_SESSION));
	gtk_box_pack_start(GTK_BOX(actions), button, FALSE, FALSE, 0);
	g_signal_connect(button, "clicked", G_CALLBACK(search_studio_replace_action_activate), page);
	button = gtk_button_new_with_mnemonic(_("Preview in _Document"));
	gtk_box_pack_start(GTK_BOX(actions), button, FALSE, FALSE, 0);
	g_signal_connect(button, "clicked", G_CALLBACK(search_studio_replace_preview_document), page);
	button = gtk_button_new_with_mnemonic(_("Preview in S_ession"));
	gtk_box_pack_start(GTK_BOX(actions), button, FALSE, FALSE, 0);
	g_signal_connect(button, "clicked", G_CALLBACK(search_studio_replace_preview_session), page);
	button = gtk_button_new_with_mnemonic(_("Open classic _Replace dialog"));
	gtk_box_pack_start(GTK_BOX(actions), button, FALSE, FALSE, 0);
	g_signal_connect(button, "clicked", G_CALLBACK(search_studio_open_replace_dialog_activate), page);
	gtk_box_pack_start(GTK_BOX(page), actions, FALSE, FALSE, 0);
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
	GtkWidget *label_find = gtk_label_new_with_mnemonic(_("_Find what:"));
	GtkWidget *label_dir = gtk_label_new_with_mnemonic(_("_Directory:"));
	GtkWidget *label_files = gtk_label_new_with_mnemonic(_("File _patterns:"));
	GtkWidget *label_encoding = gtk_label_new_with_mnemonic(_("E_ncoding:"));
	GtkWidget *entry_find = gtk_entry_new();
	GtkWidget *entry_dir = gtk_entry_new();
	GtkWidget *entry_files = gtk_entry_new();
	GtkWidget *combo_files_mode = create_fif_file_mode_combo();
	GtkWidget *encoding_combo = ui_create_encodings_combo_box(FALSE, GEANY_ENCODING_UTF_8);
	GtkWidget *check_case = gtk_check_button_new_with_mnemonic(_("Match _case"));
	GtkWidget *check_word = gtk_check_button_new_with_mnemonic(_("Whole _word"));
	GtkWidget *check_invert = gtk_check_button_new_with_mnemonic(_("_Invert"));
	GtkWidget *check_recursive = gtk_check_button_new_with_mnemonic(_("_Recursive"));
	GtkWidget *check_extra = gtk_check_button_new_with_mnemonic(_("Use e_xtra grep options"));
	GtkWidget *entry_extra = gtk_entry_new();
	GtkWidget *hint = gtk_label_new(_("Search Studio can now launch file searches directly. The classic Find in Files dialog remains available for compatibility and future parity checks."));
	GtkWidget *actions = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
	GtkWidget *button;

	gtk_grid_set_column_spacing(GTK_GRID(grid), 8);
	gtk_grid_set_row_spacing(GTK_GRID(grid), 8);
	gtk_grid_set_column_spacing(GTK_GRID(options_grid), 12);
	gtk_grid_set_row_spacing(GTK_GRID(options_grid), 6);
	ui_hookup_widget(page, entry_find, "entry_search");
	ui_hookup_widget(page, entry_dir, "entry_dir");
	ui_hookup_widget(page, entry_files, "entry_files");
	ui_hookup_widget(page, combo_files_mode, "combo_files_mode");
	ui_hookup_widget(page, encoding_combo, "combo_encoding");
	ui_hookup_widget(page, check_case, "check_case");
	ui_hookup_widget(page, check_word, "check_word");
	ui_hookup_widget(page, check_invert, "check_invert");
	ui_hookup_widget(page, check_recursive, "check_recursive");
	ui_hookup_widget(page, check_extra, "check_extra");
	ui_hookup_widget(page, entry_extra, "entry_extra");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_recursive), settings.fif_recursive);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_case), settings.fif_case_sensitive);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_word), settings.fif_match_whole_word);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_invert), settings.fif_invert_results);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_extra), settings.fif_use_extra_options);
	gtk_entry_set_text(GTK_ENTRY(entry_extra), settings.fif_extra_options ? settings.fif_extra_options : "");
	gtk_widget_set_sensitive(entry_extra, settings.fif_use_extra_options);
	g_signal_connect(check_extra, "toggled", G_CALLBACK(on_widget_toggled_set_sensitive), entry_extra);
	g_signal_connect(combo_files_mode, "changed", G_CALLBACK(search_studio_fif_file_mode_changed), page);
	gtk_combo_box_set_active(GTK_COMBO_BOX(combo_files_mode), settings.fif_files_mode);
	gtk_entry_set_text(GTK_ENTRY(entry_files), settings.fif_files ? settings.fif_files : "");

	gtk_label_set_mnemonic_widget(GTK_LABEL(label_find), entry_find);
	gtk_label_set_mnemonic_widget(GTK_LABEL(label_dir), entry_dir);
	gtk_label_set_mnemonic_widget(GTK_LABEL(label_files), combo_files_mode);
	gtk_label_set_mnemonic_widget(GTK_LABEL(label_encoding), encoding_combo);
	gtk_grid_attach(GTK_GRID(grid), label_find, 0, 0, 1, 1);
	gtk_grid_attach(GTK_GRID(grid), entry_find, 1, 0, 2, 1);
	gtk_grid_attach(GTK_GRID(grid), label_dir, 0, 1, 1, 1);
	gtk_grid_attach(GTK_GRID(grid), entry_dir, 1, 1, 2, 1);
	gtk_grid_attach(GTK_GRID(grid), label_files, 0, 2, 1, 1);
	gtk_grid_attach(GTK_GRID(grid), combo_files_mode, 1, 2, 1, 1);
	gtk_grid_attach(GTK_GRID(grid), entry_files, 2, 2, 1, 1);
	gtk_grid_attach(GTK_GRID(grid), label_encoding, 0, 3, 1, 1);
	gtk_grid_attach(GTK_GRID(grid), encoding_combo, 1, 3, 2, 1);

	gtk_grid_attach(GTK_GRID(options_grid), check_case, 0, 0, 1, 1);
	gtk_grid_attach(GTK_GRID(options_grid), check_word, 1, 0, 1, 1);
	gtk_grid_attach(GTK_GRID(options_grid), check_invert, 2, 0, 1, 1);
	gtk_grid_attach(GTK_GRID(options_grid), check_recursive, 0, 1, 1, 1);
	gtk_grid_attach(GTK_GRID(options_grid), check_extra, 1, 1, 1, 1);
	gtk_grid_attach(GTK_GRID(options_grid), entry_extra, 2, 1, 1, 1);

	gtk_label_set_xalign(GTK_LABEL(hint), 0.0f);
	gtk_label_set_line_wrap(GTK_LABEL(hint), TRUE);
	gtk_box_pack_start(GTK_BOX(page), grid, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(page), search_studio_create_mode_box(page), FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(page), options_grid, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(page), hint, FALSE, FALSE, 0);

	button = ui_button_new_with_image(GTK_STOCK_FIND, _("_Find in Files now"));
	gtk_box_pack_start(GTK_BOX(actions), button, FALSE, FALSE, 0);
	g_signal_connect(button, "clicked", G_CALLBACK(search_studio_fif_find_activate), page);
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


static GtkWidget *search_studio_create_mark_page(void)
{
	GtkWidget *page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
	GtkWidget *label = gtk_label_new_with_mnemonic(_("_Mark what:"));
	GtkWidget *entry = gtk_combo_box_text_new_with_entry();
	GtkWidget *row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
	GtkWidget *hint = gtk_label_new(_("This page goes beyond the classic Geany mark-all behavior by optionally bookmarking matching lines."));
	GtkWidget *actions = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
	GtkWidget *button;

	ui_hookup_widget(page, entry, "combo_search");
	ui_hookup_widget(page, gtk_bin_get_child(GTK_BIN(entry)), "entry_search");
	ui_entry_add_clear_icon(GTK_ENTRY(ui_lookup_widget(page, "entry_search")));
	gtk_label_set_mnemonic_widget(GTK_LABEL(label), entry);
	gtk_box_pack_start(GTK_BOX(row), label, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(row), entry, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(page), row, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(page), search_studio_create_common_options(page, FALSE, TRUE), FALSE, FALSE, 0);
	gtk_label_set_xalign(GTK_LABEL(hint), 0.0f);
	gtk_label_set_line_wrap(GTK_LABEL(hint), TRUE);
	gtk_box_pack_start(GTK_BOX(page), hint, FALSE, FALSE, 0);

	button = gtk_button_new_with_mnemonic(_("_Mark now"));
	gtk_box_pack_start(GTK_BOX(actions), button, FALSE, FALSE, 0);
	g_signal_connect(button, "clicked", G_CALLBACK(search_studio_mark_activate), page);
	button = gtk_button_new_with_mnemonic(_("Mark S_ession"));
	gtk_box_pack_start(GTK_BOX(actions), button, FALSE, FALSE, 0);
	g_signal_connect(button, "clicked", G_CALLBACK(search_studio_mark_session_activate), page);
	button = gtk_button_new_with_mnemonic(_("_Clear marks"));
	gtk_box_pack_start(GTK_BOX(actions), button, FALSE, FALSE, 0);
	g_signal_connect(button, "clicked", G_CALLBACK(search_studio_clear_marks_activate), page);
	button = gtk_button_new_with_mnemonic(_("Clea_r Session Marks"));
	gtk_box_pack_start(GTK_BOX(actions), button, FALSE, FALSE, 0);
	g_signal_connect(button, "clicked", G_CALLBACK(search_studio_clear_session_marks_activate), page);
	gtk_box_pack_start(GTK_BOX(page), actions, FALSE, FALSE, 0);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ui_lookup_widget(page,
		settings.find_regexp ? "mode_regex" : settings.find_escape_sequences ? "mode_extended" : "mode_normal")), TRUE);
	search_studio_mode_toggled(GTK_TOGGLE_BUTTON(ui_lookup_widget(page,
		settings.find_regexp ? "mode_regex" : settings.find_escape_sequences ? "mode_extended" : "mode_normal")), page);
	return page;
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
	gtk_window_set_default_size(GTK_WINDOW(studio_dlg.dialog), 900, 420);
	g_signal_connect(studio_dlg.dialog, "delete-event", G_CALLBACK(search_studio_hide_on_delete), NULL);
	g_signal_connect(studio_dlg.dialog, "response", G_CALLBACK(search_studio_on_response), NULL);

	content = ui_dialog_vbox_new(GTK_DIALOG(studio_dlg.dialog));
	gtk_box_set_spacing(GTK_BOX(content), 10);

	header = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
	title = ui_label_new_bold(_("Notepad++-style Search Studio"));
	subtitle = gtk_label_new(_("A unified cockpit for Find, Replace, Find in Files, and Mark workflows. Use it as the higher-density search surface while Geany's classic dialogs remain available."));
	gtk_label_set_xalign(GTK_LABEL(subtitle), 0.0f);
	gtk_label_set_line_wrap(GTK_LABEL(subtitle), TRUE);
	gtk_box_pack_start(GTK_BOX(header), title, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(header), subtitle, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(content), header, FALSE, FALSE, 0);

	paned = gtk_paned_new(GTK_ORIENTATION_VERTICAL);
	gtk_paned_set_position(GTK_PANED(paned), 300);

	notebook = gtk_notebook_new();
	studio_dlg.notebook = notebook;
	studio_dlg.find_page = search_studio_create_find_page();
	studio_dlg.replace_page = search_studio_create_replace_page();
	studio_dlg.fif_page = search_studio_create_fif_page();
	studio_dlg.mark_page = search_studio_create_mark_page();
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), studio_dlg.find_page, gtk_label_new(_("Find")));
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), studio_dlg.replace_page, gtk_label_new(_("Replace")));
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), studio_dlg.fif_page, gtk_label_new(_("Find in Files")));
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
	}
	g_free(sel);

	cur_dir = utils_get_current_file_dir_utf8();
	if (!EMPTY(cur_dir))
		gtk_entry_set_text(GTK_ENTRY(ui_lookup_widget(studio_dlg.fif_page, "entry_dir")), cur_dir);
	g_free(cur_dir);

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
	gboolean match_whole_word, gboolean recursive, gboolean regexp,
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
		settings.fif_match_whole_word, settings.fif_recursive, settings.fif_regexp,
		settings.fif_use_extra_options, settings.fif_extra_options,
		settings.fif_files_mode, settings.fif_files);
}


static gboolean execute_find_in_files_request(const gchar *search_text, const gchar *utf8_dir,
	gboolean invert_results, gboolean case_sensitive, gboolean match_whole_word,
	gboolean recursive, gboolean regexp, gboolean use_extra_options,
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
			recursive, regexp, use_extra_options, extra_options, files_mode, files);
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
