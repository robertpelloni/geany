/* testverticalcells.c
 *
 * Copyright (C) 2010 Openismus GmbH
 *
 * Authors:
 *      Tristan Van Berkom <tristanvb@openismus.com>
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

#include "config.h"
#include <bobgui/bobgui.h>

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

typedef struct _TreeEntry TreeEntry;

struct _TreeEntry {
  const char *icon;
  const char *info;
  const char *description;
  const char *fine_print;
  const char *fine_print_color;
  int progress;
  TreeEntry *entries;
};

enum {
  ICON_COLUMN,
  INFO_COLUMN,
  DESCRIPTION_COLUMN,
  FINE_PRINT_COLUMN,
  FINE_PRINT_COLOR_COLUMN,
  PROGRESS_COLUMN,
  NUM_COLUMNS
};


static TreeEntry info_entries[] =
  {
    { 
      "system-run", 
      "Will you\n"
      "run this ?", 
      "Currently executing that thing... you might want to abort",
      "and every day he went fishing for pigs in the sky",
      "green",
      83,
      NULL
    },
    { 
      "dialog-password", 
      "This is the\n"
      "realest of the real", 
      "We are about to authenticate the actual realness, this could take some time",
      "one day he caught a giant ogre who barked and barked and barked",
      "purple",
      4,
      NULL
    },
    { 0, },
  };

static TreeEntry directory_entries[] =
  {
    { 
      "document-open", 
      "We can edit\n"
      "things in here", 
      "Time to edit your directory, almost finished now",
      "she thought the best remedy for daydreams was going to be sleep",
      "dark sea green",
      99,
      NULL
    },
    { 
      "text-x-generic", 
      "You have a\n"
      "file here", 
      "Who would of thought there would be a file in the directory ?",
      "after taking loads of sleeping pills he could still hear the pigs barking",
      "green yellow",
      33,
      NULL
    },
    { 
      "dialog-question", 
      "Any questions ?",
      "This file would like to ask you a question",
      "so he decided that the fine print underneath the progress bar probably made no sense at all",
      "lavender",
      73,
      NULL
    },
    { 0, },
  };

static TreeEntry other_entries[] =
  {
    { 
      "zoom-fit-best", 
      "That's the\n"
      "perfect fit", 
      "Now fitting foo into bar using frobnicator",
      "using his nifty wide angle lense, he was able to catch a 'dark salmon', it was no flying pig "
      "however it was definitely a keeper",
      "dark salmon",
      59,
      NULL
    },
    { 
      "format-text-underline", 
      "Under the\n"
      "line", 
      "Now underlining that this demo would look a lot niftier with some real content",
      "it was indeed strange to catch a red salmon while fishing for pigs in the deep sky blue.",
      "deep sky blue",
      99,
      NULL
    },
    { 0, },
  };

static TreeEntry add_entries[] =
  {
    { 
      "help-about", 
      "its about\n"
      "to start", 
      "This is what it's all about",
      "so he went ahead and added the 'help-about' icon to his story, thinking 'mint cream' would be the "
      "right color for something like that",
      "dark violet",
      1,
      NULL
    },
    { 
      "zoom-in", 
      "Watch it\n"
      "Zoom !", 
      "Now zooming into something",
      "while fishing for pigs in the sky, maybe he would have caught something faster if only he had zoomed in",
      "orchid",
      6,
      NULL
    },
    { 
      "zoom-out", 
      "Zoom Zoom\n"
      "Zoom !", 
      "Now zooming out of something else",
      "the daydream had a much prettier picture over all once he had zoomed out for the wide angle, "
      "jill agreed",
      "dark magenta",
      46,
      other_entries
    },
    { 0, },
  };


static TreeEntry main_entries[] =
  {
    { 
      "dialog-information", 
      "This is all\n"
      "the info", 
      "We are currently informing you",
      "once upon a time in a land far far away there was a guy named buba",
      "red",
      64,
      info_entries
    },
    { 
      "dialog-warning", 
      "This is a\n"
      "warning", 
      "We would like to warn you that your laptop might explode after we're done",
      "so he decided that he must be stark raving mad",
      "orange",
      43,
      NULL
    },
    { 
      "dialog-error", 
      "An error will\n"
      "occur", 
      "Once we're done here, dont worry... an error will surely occur.",
      "and went to a see a yellow physiotherapist who's name was jill",
      "yellow",
      98,
      NULL
    },
    { 
      "folder", 
      "The directory", 
      "Currently scanning your directories.",
      "jill didn't know what to make of the barking pigs either so she fed him sleeping pills",
      "brown",
      20,
      directory_entries
    },
    { 
      "edit-delete", 
      "Now deleting\n"
      "the whole thing",
      "Time to delete the sucker",
      "and he decided to just delete the whole conversation since it didn't make sense to him",
      "dark orange",
      26,
      NULL
    },
    { 
      "list-add", 
      "Anything\n"
      "to add ?",
      "Now adding stuff... please be patient",
      "but on second thought, maybe he had something to add so that things could make a little less sense.",
      "maroon",
      72,
      add_entries
    },
    { 
      "edit-redo", 
      "Time to\n"
      "do it again",
      "For the hell of it, lets add the content to the treeview over and over again !",
      "buba likes telling his story, so maybe he's going to tell it 20 times more.",
      "deep pink",
      100,
      NULL
    },
    { 0, },
  };


static void
populate_model (BobguiTreeStore *model,
		BobguiTreeIter  *parent,
		TreeEntry    *entries)
{
  BobguiTreeIter iter;
  int         i;

  for (i = 0; entries[i].info != NULL; i++)
    {
      bobgui_tree_store_append (model, &iter, parent);
      bobgui_tree_store_set (model, &iter,
			  ICON_COLUMN, entries[i].icon,
			  INFO_COLUMN, entries[i].info,
			  DESCRIPTION_COLUMN, entries[i].description,
			  FINE_PRINT_COLUMN, entries[i].fine_print,
			  FINE_PRINT_COLOR_COLUMN, entries[i].fine_print_color,
			  PROGRESS_COLUMN, entries[i].progress,
			  -1);

      if (entries[i].entries)
	populate_model (model, &iter, entries[i].entries);
    }
}

static BobguiTreeModel *
create_model (void)
{
  BobguiTreeStore *model;
  int           i;

  model = bobgui_tree_store_new (NUM_COLUMNS,
			      G_TYPE_STRING,
			      G_TYPE_STRING,
			      G_TYPE_STRING,
			      G_TYPE_STRING,
			      G_TYPE_STRING,
			      G_TYPE_INT);

  for (i = 0; i < 20; i++)
    populate_model (model, NULL, main_entries);

  return BOBGUI_TREE_MODEL (model);
}

static void
quit_cb (BobguiWidget *widget,
         gpointer   data)
{
  gboolean *done = data;

  *done = TRUE;

  g_main_context_wakeup (NULL);
}

int
main (int argc, char **argv)
{
  BobguiWidget *window;
  BobguiWidget *scrolled_window;
  BobguiWidget *tree_view;
  BobguiTreeModel *tree_model;
  BobguiCellRenderer *renderer;
  BobguiTreeViewColumn *column;
  BobguiCellArea *area;
  gboolean done = FALSE;
  
  bobgui_init ();

  if (g_getenv ("RTL"))
    bobgui_widget_set_default_direction (BOBGUI_TEXT_DIR_RTL);

  window = bobgui_window_new ();
  bobgui_window_set_title (BOBGUI_WINDOW (window), "Vertical cells in BobguiTreeViewColumn example");
  g_signal_connect (window, "destroy", G_CALLBACK (quit_cb), &done);

  scrolled_window = bobgui_scrolled_window_new ();
  bobgui_scrolled_window_set_has_frame (BOBGUI_SCROLLED_WINDOW (scrolled_window), TRUE);
  bobgui_scrolled_window_set_policy (BOBGUI_SCROLLED_WINDOW (scrolled_window), 
				  BOBGUI_POLICY_AUTOMATIC, BOBGUI_POLICY_AUTOMATIC);
  bobgui_window_set_child (BOBGUI_WINDOW (window), scrolled_window);

  tree_model = create_model ();
  tree_view = bobgui_tree_view_new_with_model (tree_model);
  bobgui_tree_view_set_headers_visible (BOBGUI_TREE_VIEW (tree_view), FALSE);

  /* First column */
  column = bobgui_tree_view_column_new ();

  renderer = bobgui_cell_renderer_pixbuf_new ();
  g_object_set (renderer, "icon-size", BOBGUI_ICON_SIZE_LARGE, NULL);
  bobgui_tree_view_column_pack_start (column, renderer, TRUE);
  bobgui_tree_view_column_set_attributes (column, renderer,
				       "icon-name", ICON_COLUMN, NULL);

  renderer = bobgui_cell_renderer_text_new ();
  g_object_set (renderer, "scale", 1.2F, "weight", PANGO_WEIGHT_BOLD, NULL);
  bobgui_tree_view_column_pack_start (column, renderer, TRUE);
  bobgui_tree_view_column_set_attributes (column, renderer,
				       "text", INFO_COLUMN,
				       NULL);
  bobgui_tree_view_append_column (BOBGUI_TREE_VIEW (tree_view), column);

  /* Second (vertical) column */
  column = bobgui_tree_view_column_new ();
  area = bobgui_cell_layout_get_area (BOBGUI_CELL_LAYOUT (column));
  bobgui_orientable_set_orientation (BOBGUI_ORIENTABLE (area), BOBGUI_ORIENTATION_VERTICAL);

  renderer = bobgui_cell_renderer_text_new ();
  g_object_set (renderer, "ellipsize", PANGO_ELLIPSIZE_END, "editable", TRUE, NULL);
  bobgui_tree_view_column_pack_start (column, renderer, TRUE);
  bobgui_tree_view_column_set_attributes (column, renderer,
				       "text", DESCRIPTION_COLUMN,
				       NULL);

  renderer = bobgui_cell_renderer_progress_new ();
  bobgui_tree_view_column_pack_start (column, renderer, TRUE);
  bobgui_tree_view_column_set_attributes (column, renderer,
				       "value", PROGRESS_COLUMN,
				       NULL);

  renderer = bobgui_cell_renderer_text_new ();
  g_object_set (renderer, "scale", 0.6F, "ellipsize", PANGO_ELLIPSIZE_END, NULL);
  bobgui_tree_view_column_pack_start (column, renderer, TRUE);
  bobgui_tree_view_column_set_attributes (column, renderer,
				       "text", FINE_PRINT_COLUMN,
				       "foreground", FINE_PRINT_COLOR_COLUMN,
				       NULL);

  bobgui_tree_view_append_column (BOBGUI_TREE_VIEW (tree_view), column);

  bobgui_tree_view_expand_all (BOBGUI_TREE_VIEW (tree_view));

  bobgui_scrolled_window_set_child (BOBGUI_SCROLLED_WINDOW (scrolled_window), tree_view);

  bobgui_window_set_default_size (BOBGUI_WINDOW (window),
			       800, 400);

  bobgui_window_present (BOBGUI_WINDOW (window));
  while (!done)
    g_main_context_iteration (NULL, TRUE);

  return 0;
}
