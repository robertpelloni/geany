/* BOBGUI - The Bobgui Framework
 * bobguifilechooserentry.c: Entry with filename completion
 * Copyright (C) 2003, Red Hat, Inc.
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

#include "config.h"

#include "bobguifilechooserentry.h"

#include <string.h>

#include "deprecated/bobguicelllayout.h"
#include "deprecated/bobguicellrenderertext.h"
#include "bobguientryprivate.h"
#include "bobguifilechooserutils.h"
#include "bobguilabel.h"
#include "bobguimain.h"
#include "bobguisizerequest.h"
#include "bobguiwindow.h"
#include "bobguimarshalers.h"
#include "bobguifilefilterprivate.h"
#include "bobguifilter.h"
#include "bobguieventcontrollerfocus.h"
#include "bobguiprivate.h"

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

typedef struct _BobguiFileChooserEntryClass BobguiFileChooserEntryClass;

#define BOBGUI_FILE_CHOOSER_ENTRY_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), BOBGUI_TYPE_FILE_CHOOSER_ENTRY, BobguiFileChooserEntryClass))
#define BOBGUI_IS_FILE_CHOOSER_ENTRY_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), BOBGUI_TYPE_FILE_CHOOSER_ENTRY))
#define BOBGUI_FILE_CHOOSER_ENTRY_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), BOBGUI_TYPE_FILE_CHOOSER_ENTRY, BobguiFileChooserEntryClass))

struct _BobguiFileChooserEntryClass
{
  BobguiEntryClass parent_class;
};

struct _BobguiFileChooserEntry
{
  BobguiEntry parent_instance;

  BobguiFileChooserAction action;

  GFile *base_folder;
  GFile *current_folder_file;
  char *dir_part;
  char *file_part;

  BobguiTreeStore *completion_store;
  BobguiFileSystemModel *model;
  BobguiFileFilter *current_filter;

  guint current_folder_loaded : 1;
  guint complete_on_load : 1;
  guint eat_tabs       : 1;
  guint eat_escape     : 1;
};

enum
{
  FILE_INFO_COLUMN,
  DISPLAY_NAME_COLUMN,
  FULL_PATH_COLUMN,
  N_COLUMNS
};

enum
{
  HIDE_ENTRY,
  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

static void     bobgui_file_chooser_entry_finalize       (GObject          *object);
static void     bobgui_file_chooser_entry_dispose        (GObject          *object);
static gboolean bobgui_file_chooser_entry_grab_focus     (BobguiWidget        *widget);
static gboolean bobgui_file_chooser_entry_tab_handler    (BobguiEventControllerKey *key,
                                                       guint                  keyval,
                                                       guint                  keycode,
                                                       GdkModifierType        state,
                                                       BobguiFileChooserEntry   *chooser_entry);

#ifdef G_OS_WIN32
static int      insert_text_callback      (BobguiFileChooserEntry *widget,
					   const char          *new_text,
					   int                  new_text_length,
					   int                 *position,
					   gpointer             user_data);
static void     delete_text_callback      (BobguiFileChooserEntry *widget,
					   int                  start_pos,
					   int                  end_pos,
					   gpointer             user_data);
#endif

static gboolean match_selected_callback   (BobguiEntryCompletion  *completion,
					   BobguiTreeModel        *model,
					   BobguiTreeIter         *iter,
					   BobguiFileChooserEntry *chooser_entry);

static void set_complete_on_load (BobguiFileChooserEntry *chooser_entry,
                                  gboolean             complete_on_load);
static void refresh_current_folder_and_file_part (BobguiFileChooserEntry *chooser_entry);
static void set_completion_folder (BobguiFileChooserEntry *chooser_entry,
                                   GFile               *folder,
				   char                *dir_part);
static void finished_loading_cb (BobguiFileSystemModel  *model,
                                 GError              *error,
		                 BobguiFileChooserEntry *chooser_entry);

G_DEFINE_TYPE (BobguiFileChooserEntry, _bobgui_file_chooser_entry, BOBGUI_TYPE_ENTRY)

static char *
bobgui_file_chooser_entry_get_completion_text (BobguiFileChooserEntry *chooser_entry)
{
  BobguiEditable *editable = BOBGUI_EDITABLE (chooser_entry);
  int start, end;

  bobgui_editable_get_selection_bounds (editable, &start, &end);
  return bobgui_editable_get_chars (editable, 0, MIN (start, end));
}

static void
bobgui_file_chooser_entry_dispatch_properties_changed (GObject     *object,
                                                    guint        n_pspecs,
                                                    GParamSpec **pspecs)
{
  BobguiFileChooserEntry *chooser_entry = BOBGUI_FILE_CHOOSER_ENTRY (object);
  guint i;

  G_OBJECT_CLASS (_bobgui_file_chooser_entry_parent_class)->dispatch_properties_changed (object, n_pspecs, pspecs);

  /* Don't do this during or after disposal */
  if (bobgui_widget_get_parent (BOBGUI_WIDGET (object)) != NULL)
    {
      /* What we are after: The text in front of the cursor was modified.
       * Unfortunately, there's no other way to catch this.
       */
      for (i = 0; i < n_pspecs; i++)
        {
          if (pspecs[i]->name == I_("cursor-position") ||
              pspecs[i]->name == I_("selection-bound") ||
              pspecs[i]->name == I_("text"))
            {
              set_complete_on_load (chooser_entry, FALSE);
              refresh_current_folder_and_file_part (chooser_entry);
              break;
            }
        }
    }
}

static void
_bobgui_file_chooser_entry_class_init (BobguiFileChooserEntryClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (class);

  gobject_class->finalize = bobgui_file_chooser_entry_finalize;
  gobject_class->dispose = bobgui_file_chooser_entry_dispose;
  gobject_class->dispatch_properties_changed = bobgui_file_chooser_entry_dispatch_properties_changed;

  widget_class->grab_focus = bobgui_file_chooser_entry_grab_focus;

  signals[HIDE_ENTRY] =
    g_signal_new (I_("hide-entry"),
                  G_OBJECT_CLASS_TYPE (gobject_class),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                  0,
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 0);
}

static gboolean
match_func (BobguiEntryCompletion *compl,
            const char         *key,
            BobguiTreeIter        *iter,
            gpointer            user_data)
{
  BobguiFileChooserEntry *chooser_entry = user_data;
  GFileInfo *info;

  bobgui_tree_model_get (BOBGUI_TREE_MODEL (chooser_entry->completion_store),
                      iter,
                      FILE_INFO_COLUMN, &info,
                      -1);

  g_assert (info != NULL);
  g_object_unref (info);

  if (g_file_info_get_attribute_boolean (info, "filechooser::filtered-out"))
    return FALSE;

  /* If we arrive here, the BobguiFileSystemModel's BobguiFileFilter already filtered out all
   * files that don't start with the current prefix, so we manually apply the BobguiFileChooser's
   * current file filter (e.g. just jpg files) here.
   */
  if (chooser_entry->current_filter != NULL)
    {
      /* We always allow navigating into subfolders, so don't ever filter directories */
      if (g_file_info_get_file_type (info) != G_FILE_TYPE_REGULAR)
        return TRUE;

      g_assert (g_file_info_has_attribute (info, "standard::file"));
      return bobgui_filter_match (BOBGUI_FILTER (chooser_entry->current_filter), info);
    }

  return TRUE;
}

static void
chooser_entry_focus_out (BobguiEventController   *controller,
                         BobguiFileChooserEntry  *chooser_entry)
{
  set_complete_on_load (chooser_entry, FALSE);
}

static void
_bobgui_file_chooser_entry_init (BobguiFileChooserEntry *chooser_entry)
{
  BobguiEventController *controller;
  BobguiEntryCompletion *comp;
  BobguiCellRenderer *cell;

  g_object_set (chooser_entry, "truncate-multiline", TRUE, NULL);

  comp = bobgui_entry_completion_new ();
  bobgui_entry_completion_set_popup_single_match (comp, FALSE);
  bobgui_entry_completion_set_minimum_key_length (comp, 0);
  /* see docs for bobgui_entry_completion_set_text_column() */
  g_object_set (comp, "text-column", FULL_PATH_COLUMN, NULL);

  /* Need a match func here or entry completion uses a wrong one.
   * We do our own filtering after all. */
  bobgui_entry_completion_set_match_func (comp,
                                       match_func,
                                       chooser_entry,
                                       NULL);

  cell = bobgui_cell_renderer_text_new ();
  bobgui_cell_layout_pack_start (BOBGUI_CELL_LAYOUT (comp),
                              cell, TRUE);
  bobgui_cell_layout_add_attribute (BOBGUI_CELL_LAYOUT (comp),
                                 cell,
                                 "text", DISPLAY_NAME_COLUMN);

  g_signal_connect (comp, "match-selected",
		    G_CALLBACK (match_selected_callback), chooser_entry);

  bobgui_entry_set_completion (BOBGUI_ENTRY (chooser_entry), comp);
  g_object_unref (comp);

  /* NB: This needs to happen after the completion is set, so this controller
   * runs before the one installed by entrycompletion */
  controller = bobgui_event_controller_key_new ();
  g_signal_connect (controller,
                    "key-pressed",
                    G_CALLBACK (bobgui_file_chooser_entry_tab_handler),
                    chooser_entry);
  bobgui_widget_add_controller (BOBGUI_WIDGET (chooser_entry), controller);
  controller = bobgui_event_controller_focus_new ();
  g_signal_connect (controller,
		    "leave", G_CALLBACK (chooser_entry_focus_out),
		    chooser_entry);
  bobgui_widget_add_controller (BOBGUI_WIDGET (chooser_entry), controller);

#ifdef G_OS_WIN32
  g_signal_connect (chooser_entry, "insert-text",
		    G_CALLBACK (insert_text_callback), NULL);
  g_signal_connect (chooser_entry, "delete-text",
		    G_CALLBACK (delete_text_callback), NULL);
#endif
}

static void
bobgui_file_chooser_entry_finalize (GObject *object)
{
  BobguiFileChooserEntry *chooser_entry = BOBGUI_FILE_CHOOSER_ENTRY (object);

  if (chooser_entry->base_folder)
    g_object_unref (chooser_entry->base_folder);

  if (chooser_entry->current_folder_file)
    g_object_unref (chooser_entry->current_folder_file);

  g_free (chooser_entry->dir_part);
  g_free (chooser_entry->file_part);

  G_OBJECT_CLASS (_bobgui_file_chooser_entry_parent_class)->finalize (object);
}

static void
bobgui_file_chooser_entry_dispose (GObject *object)
{
  BobguiFileChooserEntry *chooser_entry = BOBGUI_FILE_CHOOSER_ENTRY (object);

  set_completion_folder (chooser_entry, NULL, NULL);

  G_OBJECT_CLASS (_bobgui_file_chooser_entry_parent_class)->dispose (object);
}

/* Match functions for the BobguiEntryCompletion */
static gboolean
match_selected_callback (BobguiEntryCompletion  *completion,
                         BobguiTreeModel        *model,
                         BobguiTreeIter         *iter,
                         BobguiFileChooserEntry *chooser_entry)
{
  char *path;
  int pos;

  bobgui_tree_model_get (model, iter,
                      FULL_PATH_COLUMN, &path,
                      -1);

  bobgui_editable_delete_text (BOBGUI_EDITABLE (chooser_entry),
                            0,
                            bobgui_editable_get_position (BOBGUI_EDITABLE (chooser_entry)));
  pos = 0;
  bobgui_editable_insert_text (BOBGUI_EDITABLE (chooser_entry),
                            path,
                            -1,
                            &pos);

  bobgui_editable_set_position (BOBGUI_EDITABLE (chooser_entry), pos);

  g_free (path);

  return TRUE;
}

static void
set_complete_on_load (BobguiFileChooserEntry *chooser_entry,
                      gboolean             complete_on_load)
{
  /* a completion was triggered, but we couldn't do it.
   * So no text was inserted when pressing tab, so we beep
   */
  if (chooser_entry->complete_on_load && !complete_on_load)
    bobgui_widget_error_bell (BOBGUI_WIDGET (chooser_entry));

  chooser_entry->complete_on_load = complete_on_load;
}

static gboolean
is_valid_scheme_character (char c)
{
  return g_ascii_isalnum (c) || c == '+' || c == '-' || c == '.';
}

static gboolean
has_uri_scheme (const char *str)
{
  const char *p;

  p = str;

  if (!is_valid_scheme_character (*p))
    return FALSE;

  do
    p++;
  while (is_valid_scheme_character (*p));

  return (strncmp (p, "://", 3) == 0);
}

static GFile *
bobgui_file_chooser_get_file_for_text (BobguiFileChooserEntry *chooser_entry,
                                    const char          *str)
{
  GFile *file;

  if (str[0] == '~' || g_path_is_absolute (str) || has_uri_scheme (str))
    file = g_file_parse_name (str);
  else if (chooser_entry->base_folder != NULL)
    file = g_file_resolve_relative_path (chooser_entry->base_folder, str);
  else
    file = NULL;

  return file;
}

static gboolean
is_directory_shortcut (const char *text)
{
  return strcmp (text, ".") == 0 ||
         strcmp (text, "..") == 0 ||
         strcmp (text, "~" ) == 0;
}

static GFile *
bobgui_file_chooser_entry_get_directory_for_text (BobguiFileChooserEntry *chooser_entry,
                                               const char *         text)
{
  GFile *file, *parent;

  file = bobgui_file_chooser_get_file_for_text (chooser_entry, text);

  if (file == NULL)
    return NULL;

  if (text[0] == 0 || text[strlen (text) - 1] == G_DIR_SEPARATOR ||
      is_directory_shortcut (text))
    return file;

  parent = g_file_get_parent (file);
  g_object_unref (file);

  return parent;
}

/* Finds a common prefix based on the contents of the entry
 * and mandatorily appends it
 */
static void
explicitly_complete (BobguiFileChooserEntry *chooser_entry)
{
  chooser_entry->complete_on_load = FALSE;

  if (chooser_entry->model)
    {
      char *completion, *text;
      gsize completion_len, text_len;

      text = bobgui_file_chooser_entry_get_completion_text (chooser_entry);
      text_len = strlen (text);
      completion = bobgui_entry_completion_compute_prefix (bobgui_entry_get_completion (BOBGUI_ENTRY (chooser_entry)), text);
      completion_len = completion ? strlen (completion) : 0;

      if (completion_len > text_len)
        {
          BobguiEditable *editable = BOBGUI_EDITABLE (chooser_entry);
          int pos = bobgui_editable_get_position (editable);

          bobgui_editable_insert_text (editable,
                                    completion + text_len,
                                    completion_len - text_len,
                                    &pos);
          bobgui_editable_set_position (editable, pos);
          return;
        }
    }

  bobgui_widget_error_bell (BOBGUI_WIDGET (chooser_entry));
}

static gboolean
bobgui_file_chooser_entry_grab_focus (BobguiWidget *widget)
{
  if (!BOBGUI_WIDGET_CLASS (_bobgui_file_chooser_entry_parent_class)->grab_focus (widget))
    return FALSE;

  _bobgui_file_chooser_entry_select_filename (BOBGUI_FILE_CHOOSER_ENTRY (widget));
  return TRUE;
}

static void
start_explicit_completion (BobguiFileChooserEntry *chooser_entry)
{
  if (chooser_entry->current_folder_loaded)
    explicitly_complete (chooser_entry);
  else
    set_complete_on_load (chooser_entry, TRUE);
}

static gboolean
bobgui_file_chooser_entry_tab_handler (BobguiEventControllerKey *key,
                                    guint                  keyval,
                                    guint                  keycode,
                                    GdkModifierType        state,
                                    BobguiFileChooserEntry   *chooser_entry)
{
  BobguiEditable *editable = BOBGUI_EDITABLE (chooser_entry);
  int start, end;

  if (keyval == GDK_KEY_Escape &&
      chooser_entry->eat_escape)
    {
      g_signal_emit (chooser_entry, signals[HIDE_ENTRY], 0);
      return GDK_EVENT_STOP;
    }

  if (!chooser_entry->eat_tabs)
    return GDK_EVENT_PROPAGATE;

  if (keyval != GDK_KEY_Tab)
    return GDK_EVENT_PROPAGATE;

  if ((state & GDK_CONTROL_MASK) == GDK_CONTROL_MASK)
    return GDK_EVENT_PROPAGATE;

  /* This is a bit evil -- it makes Tab never leave the entry. It basically
   * makes it 'safe' for people to hit. */
  bobgui_editable_get_selection_bounds (editable, &start, &end);

  if (start != end)
    bobgui_editable_set_position (editable, MAX (start, end));
  else
    start_explicit_completion (chooser_entry);

  return GDK_EVENT_STOP;
}

static void
update_inline_completion (BobguiFileChooserEntry *chooser_entry)
{
  BobguiEntryCompletion *completion = bobgui_entry_get_completion (BOBGUI_ENTRY (chooser_entry));

  if (!chooser_entry->current_folder_loaded)
    {
      bobgui_entry_completion_set_inline_completion (completion, FALSE);
      return;
    }

  switch (chooser_entry->action)
    {
    case BOBGUI_FILE_CHOOSER_ACTION_OPEN:
    case BOBGUI_FILE_CHOOSER_ACTION_SELECT_FOLDER:
      bobgui_entry_completion_set_inline_completion (completion, TRUE);
      break;
    case BOBGUI_FILE_CHOOSER_ACTION_SAVE:
    default:
      bobgui_entry_completion_set_inline_completion (completion, FALSE);
      break;
    }
}

static void
discard_completion_store (BobguiFileChooserEntry *chooser_entry)
{
  if (!chooser_entry->model)
    return;

  g_assert (chooser_entry->completion_store != NULL);

  bobgui_entry_completion_set_model (bobgui_entry_get_completion (BOBGUI_ENTRY (chooser_entry)), NULL);
  update_inline_completion (chooser_entry);
  g_clear_object (&chooser_entry->completion_store);
  g_clear_object (&chooser_entry->model);
}

static void
model_items_changed_cb (GListModel          *model,
                        guint                position,
                        guint                removed,
                        guint                added,
                        BobguiFileChooserEntry *self)
{
  if (removed > 0)
    {
      BobguiTreeIter iter;

      if (bobgui_tree_model_iter_nth_child (BOBGUI_TREE_MODEL (self->completion_store),
                                         &iter,
                                         NULL,
                                         position))
        {
          while (removed--)
            bobgui_tree_store_remove (self->completion_store, &iter);
        }
    }

  while (added-- > 0)
    {
      BobguiTreeIter iter;
      GFileInfo *info;
      const char *suffix = NULL;
      char *full_path;
      char *display_name;

      info = g_list_model_get_item (model, position);

      if (_bobgui_file_info_consider_as_directory (info))
        suffix = G_DIR_SEPARATOR_S;

      display_name = g_strconcat (g_file_info_get_display_name (info), suffix, NULL);
      full_path = g_strconcat (self->dir_part, display_name, NULL);

      bobgui_tree_store_insert_with_values (self->completion_store,
                                         &iter, NULL,
                                         position,
                                         FILE_INFO_COLUMN, info,
                                         FULL_PATH_COLUMN, full_path,
                                         DISPLAY_NAME_COLUMN, display_name,
                                         -1);

      g_free (display_name);
      g_free (full_path);

      g_clear_object (&info);

      position++;
    }
}

/* Fills the completion store from the contents of the current folder */
static void
populate_completion_store (BobguiFileChooserEntry *chooser_entry)
{
  chooser_entry->completion_store = bobgui_tree_store_new (N_COLUMNS,
                                                        G_TYPE_FILE_INFO,
                                                        G_TYPE_STRING,
                                                        G_TYPE_STRING);

  chooser_entry->model =
      _bobgui_file_system_model_new_for_directory (chooser_entry->current_folder_file,
                                                "standard::name,standard::display-name,standard::type,"
                                                "standard::content-type");
  g_signal_connect (chooser_entry->model, "items-changed",
                    G_CALLBACK (model_items_changed_cb), chooser_entry);
  g_signal_connect (chooser_entry->model, "finished-loading",
		    G_CALLBACK (finished_loading_cb), chooser_entry);

  _bobgui_file_system_model_set_filter_folders (chooser_entry->model, TRUE);
  _bobgui_file_system_model_set_show_files (chooser_entry->model,
                                         chooser_entry->action == BOBGUI_FILE_CHOOSER_ACTION_OPEN ||
                                         chooser_entry->action == BOBGUI_FILE_CHOOSER_ACTION_SAVE);

  bobgui_entry_completion_set_model (bobgui_entry_get_completion (BOBGUI_ENTRY (chooser_entry)),
                                  BOBGUI_TREE_MODEL (chooser_entry->completion_store));
}

/* Callback when the current folder finishes loading */
static void
finished_loading_cb (BobguiFileSystemModel  *model,
                     GError              *error,
		     BobguiFileChooserEntry *chooser_entry)
{
  BobguiEntryCompletion *completion;

  chooser_entry->current_folder_loaded = TRUE;

  if (error)
    {
      discard_completion_store (chooser_entry);
      set_complete_on_load (chooser_entry, FALSE);
      return;
    }

  if (chooser_entry->complete_on_load)
    explicitly_complete (chooser_entry);

  bobgui_widget_set_tooltip_text (BOBGUI_WIDGET (chooser_entry), NULL);

  completion = bobgui_entry_get_completion (BOBGUI_ENTRY (chooser_entry));
  update_inline_completion (chooser_entry);

  if (bobgui_widget_has_focus (BOBGUI_WIDGET (bobgui_entry_get_text_widget (BOBGUI_ENTRY (chooser_entry)))))
    {
      bobgui_entry_completion_complete (completion);
      bobgui_entry_completion_insert_prefix (completion);
    }
}

static void
set_completion_folder (BobguiFileChooserEntry *chooser_entry,
                       GFile               *folder_file,
		       char                *dir_part)
{
  if (((chooser_entry->current_folder_file
	&& folder_file
	&& g_file_equal (folder_file, chooser_entry->current_folder_file))
       || chooser_entry->current_folder_file == folder_file)
      && g_strcmp0 (dir_part, chooser_entry->dir_part) == 0)
    {
      return;
    }

  g_clear_object (&chooser_entry->current_folder_file);

  g_free (chooser_entry->dir_part);
  chooser_entry->dir_part = g_strdup (dir_part);
  
  chooser_entry->current_folder_loaded = FALSE;

  discard_completion_store (chooser_entry);

  if (folder_file)
    {
      chooser_entry->current_folder_file = g_object_ref (folder_file);
      populate_completion_store (chooser_entry);
    }
}

static void
refresh_current_folder_and_file_part (BobguiFileChooserEntry *chooser_entry)
{
  GFile *folder_file;
  char *text, *last_slash, *old_file_part;
  char *dir_part;

  old_file_part = chooser_entry->file_part;

  text = bobgui_file_chooser_entry_get_completion_text (chooser_entry);
  g_return_if_fail (text != NULL);

  last_slash = strrchr (text, G_DIR_SEPARATOR);
  if (last_slash)
    {
      dir_part = g_strndup (text, last_slash - text + 1);
      chooser_entry->file_part = g_strdup (last_slash + 1);
    }
  else
    {
      dir_part = g_strdup ("");
      chooser_entry->file_part = g_strdup (text);
    }

  folder_file = bobgui_file_chooser_entry_get_directory_for_text (chooser_entry, text);

  set_completion_folder (chooser_entry, folder_file, dir_part);

  if (folder_file)
    g_object_unref (folder_file);

  g_free (dir_part);

  if (chooser_entry->model &&
      (g_strcmp0 (old_file_part, chooser_entry->file_part) != 0))
    {
      BobguiFileFilter *filter;
      char *pattern;

      filter = bobgui_file_filter_new ();
      pattern = g_strconcat (chooser_entry->file_part, "*", NULL);
      bobgui_file_filter_add_pattern (filter, pattern);

      _bobgui_file_system_model_set_filter (chooser_entry->model, filter);

      g_free (pattern);
      g_object_unref (filter);
    }

  g_free (text);
  g_free (old_file_part);
}

#ifdef G_OS_WIN32
static int
insert_text_callback (BobguiFileChooserEntry *chooser_entry,
		      const char          *new_text,
		      int        	   new_text_length,
		      int        	  *position,
		      gpointer   	   user_data)
{
  const char *colon = memchr (new_text, ':', new_text_length);
  int i;

  /* Disallow these characters altogether */
  for (i = 0; i < new_text_length; i++)
    {
      if (new_text[i] == '<' ||
	  new_text[i] == '>' ||
	  new_text[i] == '"' ||
	  new_text[i] == '|' ||
	  new_text[i] == '*' ||
	  new_text[i] == '?')
	break;
    }

  if (i < new_text_length ||
      /* Disallow entering text that would cause a colon to be anywhere except
       * after a drive letter.
       */
      (colon != NULL &&
       *position + (colon - new_text) != 1) ||
      (new_text_length > 0 &&
       *position <= 1 &&
       bobgui_entry_get_text_length (BOBGUI_ENTRY (chooser_entry)) >= 2 &&
       bobgui_editable_get_text (BOBGUI_EDITABLE (chooser_entry))[1] == ':'))
    {
      bobgui_widget_error_bell (BOBGUI_WIDGET (chooser_entry));
      g_signal_stop_emission_by_name (chooser_entry, "insert_text");
      return FALSE;
    }

  return TRUE;
}

static void
delete_text_callback (BobguiFileChooserEntry *chooser_entry,
		      int                  start_pos,
		      int                  end_pos,
		      gpointer             user_data)
{
  /* If deleting a drive letter, delete the colon, too */
  if (start_pos == 0 && end_pos == 1 &&
      bobgui_entry_get_text_length (BOBGUI_ENTRY (chooser_entry)) >= 2 &&
      bobgui_editable_get_text (BOBGUI_EDITABLE (chooser_entry))[1] == ':')
    {
      g_signal_handlers_block_by_func (chooser_entry,
				       G_CALLBACK (delete_text_callback),
				       user_data);
      bobgui_editable_delete_text (BOBGUI_EDITABLE (chooser_entry), 0, 1);
      g_signal_handlers_unblock_by_func (chooser_entry,
					 G_CALLBACK (delete_text_callback),
					 user_data);
    }
}
#endif

/**
 * _bobgui_file_chooser_entry_new:
 * @eat_tabs: If %FALSE, allow focus navigation with the tab key.
 * @eat_escape: If %TRUE, capture Escape key presses and emit ::hide-entry
 *
 * Creates a new `BobguiFileChooserEntry` object. `BobguiFileChooserEntry`
 * is an internal implementation widget for the BOBGUI file chooser
 * which is an entry with completion with respect to a
 * `BobguiFileSystem` object.
 *
 * Returns: the newly created `BobguiFileChooserEntry`
 **/
BobguiWidget *
_bobgui_file_chooser_entry_new (gboolean eat_tabs,
                             gboolean eat_escape)
{
  BobguiFileChooserEntry *chooser_entry;

  chooser_entry = g_object_new (BOBGUI_TYPE_FILE_CHOOSER_ENTRY, NULL);
  chooser_entry->eat_tabs = (eat_tabs != FALSE);
  chooser_entry->eat_escape = (eat_escape != FALSE);

  return BOBGUI_WIDGET (chooser_entry);
}

/**
 * _bobgui_file_chooser_entry_set_base_folder:
 * @chooser_entry: a `BobguiFileChooserEntry`
 * @file: file for a folder in the chooser entries current file system.
 *
 * Sets the folder with respect to which completions occur.
 **/
void
_bobgui_file_chooser_entry_set_base_folder (BobguiFileChooserEntry *chooser_entry,
					 GFile               *file)
{
  g_return_if_fail (BOBGUI_IS_FILE_CHOOSER_ENTRY (chooser_entry));
  g_return_if_fail (file == NULL || G_IS_FILE (file));

  if (chooser_entry->base_folder == file ||
      (file != NULL && chooser_entry->base_folder != NULL 
       && g_file_equal (chooser_entry->base_folder, file)))
    return;

  if (file)
    g_object_ref (file);

  if (chooser_entry->base_folder)
    g_object_unref (chooser_entry->base_folder);

  chooser_entry->base_folder = file;

  refresh_current_folder_and_file_part (chooser_entry);
}

/**
 * _bobgui_file_chooser_entry_get_current_folder:
 * @chooser_entry: a `BobguiFileChooserEntry`
 *
 * Gets the current folder for the `BobguiFileChooserEntry`.
 *
 * If the user has only entered a filename, this will be in the base
 * folder (see _bobgui_file_chooser_entry_set_base_folder()), but if the
 * user has entered a relative or absolute path, then it will be
 * different. If the user has entered unparsable text, or text which
 * the entry cannot handle, this will return %NULL.
 *
 * Returns: (nullable) (transfer full): the file for the current folder
 *   or %NULL if the current folder can not be determined
 */
GFile *
_bobgui_file_chooser_entry_get_current_folder (BobguiFileChooserEntry *chooser_entry)
{
  g_return_val_if_fail (BOBGUI_IS_FILE_CHOOSER_ENTRY (chooser_entry), NULL);

  return bobgui_file_chooser_entry_get_directory_for_text (chooser_entry,
                                                        bobgui_editable_get_text (BOBGUI_EDITABLE (chooser_entry)));
}

/**
 * _bobgui_file_chooser_entry_get_file_part:
 * @chooser_entry: a `BobguiFileChooserEntry`
 *
 * Gets the non-folder portion of whatever the user has entered
 * into the file selector. What is returned is a UTF-8 string,
 * and if a filename path is needed, g_file_get_child_for_display_name()
 * must be used
  *
 * Returns: the entered filename - this value is owned by the
 *  chooser entry and must not be modified or freed.
 **/
const char *
_bobgui_file_chooser_entry_get_file_part (BobguiFileChooserEntry *chooser_entry)
{
  const char *last_slash, *text;

  g_return_val_if_fail (BOBGUI_IS_FILE_CHOOSER_ENTRY (chooser_entry), NULL);

  text = bobgui_editable_get_text (BOBGUI_EDITABLE (chooser_entry));
  last_slash = strrchr (text, G_DIR_SEPARATOR);
  if (last_slash)
    return last_slash + 1;
  else if (is_directory_shortcut (text))
    return "";
  else
    return text;
}

/**
 * _bobgui_file_chooser_entry_set_action:
 * @chooser_entry: a `BobguiFileChooserEntry`
 * @action: the action which is performed by the file selector using this entry
 *
 * Sets action which is performed by the file selector using this entry.
 * The `BobguiFileChooserEntry` will use different completion strategies for
 * different actions.
 **/
void
_bobgui_file_chooser_entry_set_action (BobguiFileChooserEntry *chooser_entry,
				    BobguiFileChooserAction action)
{
  g_return_if_fail (BOBGUI_IS_FILE_CHOOSER_ENTRY (chooser_entry));
  
  if (chooser_entry->action != action)
    {
      BobguiEntryCompletion *comp;

      chooser_entry->action = action;

      comp = bobgui_entry_get_completion (BOBGUI_ENTRY (chooser_entry));

      /* FIXME: do we need to actually set the following? */

      switch (action)
	{
	case BOBGUI_FILE_CHOOSER_ACTION_OPEN:
	case BOBGUI_FILE_CHOOSER_ACTION_SELECT_FOLDER:
        default:
	  bobgui_entry_completion_set_popup_single_match (comp, FALSE);
	  break;
	case BOBGUI_FILE_CHOOSER_ACTION_SAVE:
	  bobgui_entry_completion_set_popup_single_match (comp, TRUE);
	  break;
	}

      if (chooser_entry->model)
        _bobgui_file_system_model_set_show_files (chooser_entry->model,
                                               action == BOBGUI_FILE_CHOOSER_ACTION_OPEN ||
                                               action == BOBGUI_FILE_CHOOSER_ACTION_SAVE);

      update_inline_completion (chooser_entry);
    }
}


/**
 * _bobgui_file_chooser_entry_get_action:
 * @chooser_entry: a `BobguiFileChooserEntry`
 *
 * Gets the action for this entry. 
 *
 * Returns: the action
 **/
BobguiFileChooserAction
_bobgui_file_chooser_entry_get_action (BobguiFileChooserEntry *chooser_entry)
{
  g_return_val_if_fail (BOBGUI_IS_FILE_CHOOSER_ENTRY (chooser_entry),
			BOBGUI_FILE_CHOOSER_ACTION_OPEN);
  
  return chooser_entry->action;
}

gboolean
_bobgui_file_chooser_entry_get_is_folder (BobguiFileChooserEntry *chooser_entry,
				       GFile               *file)
{
  GFileInfo *info;

  if (chooser_entry->model == NULL)
    return FALSE;

  info = _bobgui_file_system_model_get_info_for_file (chooser_entry->model, file);
  if (!info)
    return FALSE;

  return _bobgui_file_info_consider_as_directory (info);
}


/*
 * _bobgui_file_chooser_entry_select_filename:
 * @chooser_entry: a `BobguiFileChooserEntry`
 *
 * Selects the filename (without the extension) for user edition.
 */
void
_bobgui_file_chooser_entry_select_filename (BobguiFileChooserEntry *chooser_entry)
{
  const char *str, *ext;
  glong len = -1;

  if (chooser_entry->action == BOBGUI_FILE_CHOOSER_ACTION_SAVE)
    {
      str = bobgui_editable_get_text (BOBGUI_EDITABLE (chooser_entry));
      ext = g_strrstr (str, ".");

      if (ext)
       len = g_utf8_pointer_to_offset (str, ext);
    }

  bobgui_editable_select_region (BOBGUI_EDITABLE (chooser_entry), 0, (int) len);
}

void
_bobgui_file_chooser_entry_set_file_filter (BobguiFileChooserEntry *chooser_entry,
                                         BobguiFileFilter       *filter)
{
  chooser_entry->current_filter = filter;
}

void
bobgui_file_chooser_entry_set_text (BobguiFileChooserEntry *entry,
                                 const char          *text)
{
  BobguiEntryCompletion *completion;
  gboolean popup;

  completion = bobgui_entry_get_completion (BOBGUI_ENTRY (entry));
  popup = bobgui_entry_completion_get_popup_completion (completion);

  bobgui_entry_completion_set_popup_completion (completion, FALSE);

  bobgui_editable_set_text (BOBGUI_EDITABLE (entry), text);

  bobgui_entry_completion_set_popup_completion (completion, popup);
}
