/* testtreesort.c
 * Copyright (C) 2001 Red Hat, Inc
 * Author: Jonathan Blandford
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

typedef struct _ListSort ListSort;
struct _ListSort
{
  const char *word_1;
  const char *word_2;
  const char *word_3;
  const char *word_4;
  int number_1;
};

static ListSort data[] =
{
  { "Apples", "Transmogrify long word to demonstrate weirdness", "Exculpatory", "Gesundheit", 30 },
  { "Oranges", "Wicker", "Adamantine", "Convivial", 10 },
  { "Bovine Spongiform Encephilopathy", "Sleazebucket", "Mountaineer", "Pander", 40 },
  { "Foot and Mouth", "Lampshade", "Skim Milk\nFull Milk", "Viewless", 20 },
  { "Blood,\nsweat,\ntears", "The Man", "Horses", "Muckety-Muck", 435 },
  { "Rare Steak", "Siam", "Watchdog", "Xantippe" , 99999 },
  { "SIGINT", "Rabbit Breath", "Alligator", "Bloodstained", 4123 },
  { "Google", "Chrysanthemums", "Hobnob", "Leapfrog", 1 },
  { "Technology fibre optic", "Turtle", "Academe", "Lonely", 3 },
  { "Freon", "Harpes", "Quidditch", "Reagan", 6},
  { "Transposition", "Fruit Basket", "Monkey Wort", "Glogg", 54 },
  { "Fern", "Glasnost and Perestroika", "Latitude", "Bomberman!!!", 2 },
  {NULL, }
};

static ListSort childdata[] =
{
  { "Heineken", "Nederland", "Wanda de vis", "Electronische post", 2},
  { "Hottentottententententoonstelling", "Rotterdam", "Ionentransport", "Palm", 45},
  { "Fruitvlieg", "Eigenfrequentie", "Supernoodles", "Ramen", 2002},
  { "Gereedschapskist", "Stelsel van lineaire vergelijkingen", "Tulpen", "Badlaken", 1311},
  { "Stereoinstallatie", "Rood tapijt", "Het periodieke systeem der elementen", "Laaste woord", 200},
  {NULL, }
};
  

enum
{
  WORD_COLUMN_1 = 0,
  WORD_COLUMN_2,
  WORD_COLUMN_3,
  WORD_COLUMN_4,
  NUMBER_COLUMN_1,
  NUM_COLUMNS
};

static void
switch_search_method (BobguiWidget *button,
		      gpointer   tree_view)
{
  if (!bobgui_tree_view_get_search_entry (BOBGUI_TREE_VIEW (tree_view)))
    {
      gpointer search_entry = g_object_get_data (tree_view, "my-search-entry");
      bobgui_tree_view_set_search_entry (BOBGUI_TREE_VIEW (tree_view), BOBGUI_EDITABLE (search_entry));
    }
  else
    bobgui_tree_view_set_search_entry (BOBGUI_TREE_VIEW (tree_view), NULL);
}

static void
quit_cb (BobguiWidget *widget,
         gpointer   user_data)
{
  gboolean *done = user_data;

  *done = TRUE;

  g_main_context_wakeup (NULL);
}

int
main (int argc, char *argv[])
{
  BobguiWidget *window;
  BobguiWidget *vbox;
  BobguiWidget *scrolled_window;
  BobguiWidget *tree_view;
  BobguiTreeStore *model;
  BobguiTreeModel *smodel = NULL;
  BobguiTreeModel *ssmodel = NULL;
  BobguiCellRenderer *renderer;
  BobguiTreeViewColumn *column;
  BobguiTreeIter iter;
  int i;

  BobguiWidget *entry, *button;
  BobguiWidget *window2, *vbox2, *scrolled_window2, *tree_view2;
  BobguiWidget *window3, *vbox3, *scrolled_window3, *tree_view3;
  gboolean done = FALSE;

  bobgui_init ();

  /**
   * First window - Just a BobguiTreeStore
   */

  window = bobgui_window_new ();
  bobgui_window_set_title (BOBGUI_WINDOW (window), "Words, words, words - Window 1");
  g_signal_connect (window, "destroy", G_CALLBACK (quit_cb), &done);
  vbox = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 8);
  bobgui_box_append (BOBGUI_BOX (vbox), bobgui_label_new ("Jonathan and Kristian's list of cool words. (And Anders' cool list of numbers) \n\nThis is just a BobguiTreeStore"));
  bobgui_window_set_child (BOBGUI_WINDOW (window), vbox);

  entry = bobgui_entry_new ();
  bobgui_box_append (BOBGUI_BOX (vbox), entry);

  button = bobgui_button_new_with_label ("Switch search method");
  bobgui_box_append (BOBGUI_BOX (vbox), button);

  scrolled_window = bobgui_scrolled_window_new ();
  bobgui_scrolled_window_set_has_frame (BOBGUI_SCROLLED_WINDOW (scrolled_window), TRUE);
  bobgui_scrolled_window_set_policy (BOBGUI_SCROLLED_WINDOW (scrolled_window), BOBGUI_POLICY_AUTOMATIC, BOBGUI_POLICY_AUTOMATIC);
  bobgui_widget_set_vexpand (scrolled_window, TRUE);
  bobgui_box_append (BOBGUI_BOX (vbox), scrolled_window);

  model = bobgui_tree_store_new (NUM_COLUMNS, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INT);

/*
  smodel = bobgui_tree_model_sort_new_with_model (BOBGUI_TREE_MODEL (model));
  ssmodel = bobgui_tree_model_sort_new_with_model (BOBGUI_TREE_MODEL (smodel));
*/
  tree_view = bobgui_tree_view_new_with_model (BOBGUI_TREE_MODEL (model));

  bobgui_tree_view_set_search_entry (BOBGUI_TREE_VIEW (tree_view), BOBGUI_EDITABLE (entry));
  g_object_set_data (G_OBJECT (tree_view), "my-search-entry", entry);
  g_signal_connect (button, "clicked",
		    G_CALLBACK (switch_search_method), tree_view);

 /* bobgui_tree_selection_set_select_function (bobgui_tree_view_get_selection (BOBGUI_TREE_VIEW (tree_view)), select_func, NULL, NULL);*/

  /* 12 iters now, 12 later... */
  for (i = 0; data[i].word_1 != NULL; i++)
    {
      int k;
      BobguiTreeIter child_iter;


      bobgui_tree_store_prepend (BOBGUI_TREE_STORE (model), &iter, NULL);
      bobgui_tree_store_set (BOBGUI_TREE_STORE (model), &iter,
			  WORD_COLUMN_1, data[i].word_1,
			  WORD_COLUMN_2, data[i].word_2,
			  WORD_COLUMN_3, data[i].word_3,
			  WORD_COLUMN_4, data[i].word_4,
			  NUMBER_COLUMN_1, data[i].number_1,
			  -1);

      bobgui_tree_store_append (BOBGUI_TREE_STORE (model), &child_iter, &iter);
      bobgui_tree_store_set (BOBGUI_TREE_STORE (model), &child_iter,
			  WORD_COLUMN_1, data[i].word_1,
			  WORD_COLUMN_2, data[i].word_2,
			  WORD_COLUMN_3, data[i].word_3,
			  WORD_COLUMN_4, data[i].word_4,
			  NUMBER_COLUMN_1, data[i].number_1,
			  -1);

      for (k = 0; childdata[k].word_1 != NULL; k++)
	{
	  bobgui_tree_store_append (BOBGUI_TREE_STORE (model), &child_iter, &iter);
	  bobgui_tree_store_set (BOBGUI_TREE_STORE (model), &child_iter,
			      WORD_COLUMN_1, childdata[k].word_1,
			      WORD_COLUMN_2, childdata[k].word_2,
			      WORD_COLUMN_3, childdata[k].word_3,
			      WORD_COLUMN_4, childdata[k].word_4,
			      NUMBER_COLUMN_1, childdata[k].number_1,
			      -1);

	}

    }
  
  smodel = bobgui_tree_model_sort_new_with_model (BOBGUI_TREE_MODEL (model));
  ssmodel = bobgui_tree_model_sort_new_with_model (BOBGUI_TREE_MODEL (smodel));
  g_object_unref (model);

  renderer = bobgui_cell_renderer_text_new ();
  column = bobgui_tree_view_column_new_with_attributes ("First Word", renderer,
						     "text", WORD_COLUMN_1,
						     NULL);
  bobgui_tree_view_append_column (BOBGUI_TREE_VIEW (tree_view), column);
  bobgui_tree_view_column_set_sort_column_id (column, WORD_COLUMN_1);

  renderer = bobgui_cell_renderer_text_new ();
  column = bobgui_tree_view_column_new_with_attributes ("Second Word", renderer,
						     "text", WORD_COLUMN_2,
						     NULL);
  bobgui_tree_view_column_set_sort_column_id (column, WORD_COLUMN_2);
  bobgui_tree_view_append_column (BOBGUI_TREE_VIEW (tree_view), column);

  renderer = bobgui_cell_renderer_text_new ();
  column = bobgui_tree_view_column_new_with_attributes ("Third Word", renderer,
						     "text", WORD_COLUMN_3,
						     NULL);
  bobgui_tree_view_column_set_sort_column_id (column, WORD_COLUMN_3);
  bobgui_tree_view_append_column (BOBGUI_TREE_VIEW (tree_view), column);

  renderer = bobgui_cell_renderer_text_new ();
  column = bobgui_tree_view_column_new_with_attributes ("Fourth Word", renderer,
						     "text", WORD_COLUMN_4,
						     NULL);
  bobgui_tree_view_column_set_sort_column_id (column, WORD_COLUMN_4);
  bobgui_tree_view_append_column (BOBGUI_TREE_VIEW (tree_view), column);
  
  renderer = bobgui_cell_renderer_text_new ();
  column = bobgui_tree_view_column_new_with_attributes ("First Number", renderer,
						     "text", NUMBER_COLUMN_1,
						     NULL);
  bobgui_tree_view_column_set_sort_column_id (column, NUMBER_COLUMN_1);
  bobgui_tree_view_append_column (BOBGUI_TREE_VIEW (tree_view), column);

  /*  bobgui_tree_sortable_set_sort_column_id (BOBGUI_TREE_SORTABLE (smodel),
					WORD_COLUMN_1,
					BOBGUI_SORT_ASCENDING);*/

  bobgui_scrolled_window_set_child (BOBGUI_SCROLLED_WINDOW (scrolled_window), tree_view);
  bobgui_window_set_default_size (BOBGUI_WINDOW (window), 400, 400);
  bobgui_window_present (BOBGUI_WINDOW (window));

  /**
   * Second window - BobguiTreeModelSort wrapping the BobguiTreeStore
   */

  if (smodel)
    {
      window2 = bobgui_window_new ();
      bobgui_window_set_title (BOBGUI_WINDOW (window2), 
			    "Words, words, words - window 2");
      g_signal_connect (window2, "destroy", G_CALLBACK (quit_cb), &done);
      vbox2 = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 8);
      bobgui_box_append (BOBGUI_BOX (vbox2), 
			  bobgui_label_new ("Jonathan and Kristian's list of words.\n\nA BobguiTreeModelSort wrapping the BobguiTreeStore of window 1"));
      bobgui_window_set_child (BOBGUI_WINDOW (window2), vbox2);
      
      scrolled_window2 = bobgui_scrolled_window_new ();
      bobgui_scrolled_window_set_has_frame (BOBGUI_SCROLLED_WINDOW (scrolled_window2), TRUE);
      bobgui_scrolled_window_set_policy (BOBGUI_SCROLLED_WINDOW (scrolled_window2),
				      BOBGUI_POLICY_AUTOMATIC,
				      BOBGUI_POLICY_AUTOMATIC);
      bobgui_widget_set_vexpand (scrolled_window2, TRUE);
      bobgui_box_append (BOBGUI_BOX (vbox2), scrolled_window2);


      tree_view2 = bobgui_tree_view_new_with_model (smodel);
      
      renderer = bobgui_cell_renderer_text_new ();
      column = bobgui_tree_view_column_new_with_attributes ("First Word", renderer,
							 "text", WORD_COLUMN_1,
							 NULL);
      bobgui_tree_view_append_column (BOBGUI_TREE_VIEW (tree_view2), column);
      bobgui_tree_view_column_set_sort_column_id (column, WORD_COLUMN_1);
      
      renderer = bobgui_cell_renderer_text_new ();
      column = bobgui_tree_view_column_new_with_attributes ("Second Word", renderer,
							 "text", WORD_COLUMN_2,
							 NULL);
      bobgui_tree_view_column_set_sort_column_id (column, WORD_COLUMN_2);
      bobgui_tree_view_append_column (BOBGUI_TREE_VIEW (tree_view2), column);
      
      renderer = bobgui_cell_renderer_text_new ();
      column = bobgui_tree_view_column_new_with_attributes ("Third Word", renderer,
							 "text", WORD_COLUMN_3,
							 NULL);
      bobgui_tree_view_column_set_sort_column_id (column, WORD_COLUMN_3);
      bobgui_tree_view_append_column (BOBGUI_TREE_VIEW (tree_view2), column);
      
      renderer = bobgui_cell_renderer_text_new ();
      column = bobgui_tree_view_column_new_with_attributes ("Fourth Word", renderer,
							 "text", WORD_COLUMN_4,
							 NULL);
      bobgui_tree_view_column_set_sort_column_id (column, WORD_COLUMN_4);
      bobgui_tree_view_append_column (BOBGUI_TREE_VIEW (tree_view2), column);
      
      /*      bobgui_tree_sortable_set_default_sort_func (BOBGUI_TREE_SORTABLE (smodel),
					       (BobguiTreeIterCompareFunc)bobgui_tree_data_list_compare_func,
					       NULL, NULL);
      bobgui_tree_sortable_set_sort_column_id (BOBGUI_TREE_SORTABLE (smodel),
					    WORD_COLUMN_1,
					    BOBGUI_SORT_DESCENDING);*/

      bobgui_scrolled_window_set_child (BOBGUI_SCROLLED_WINDOW (scrolled_window2), tree_view2);
      bobgui_window_set_default_size (BOBGUI_WINDOW (window2), 400, 400);
      bobgui_window_present (BOBGUI_WINDOW (window2));
    }
  
  /**
   * Third window - BobguiTreeModelSort wrapping the BobguiTreeModelSort which
   * is wrapping the BobguiTreeStore.
   */
  
  if (ssmodel)
    {
      window3 = bobgui_window_new ();
      bobgui_window_set_title (BOBGUI_WINDOW (window3), 
			    "Words, words, words - Window 3");
      g_signal_connect (window3, "destroy", G_CALLBACK (quit_cb), &done);
      vbox3 = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 8);
      bobgui_box_append (BOBGUI_BOX (vbox3), 
			  bobgui_label_new ("Jonathan and Kristian's list of words.\n\nA BobguiTreeModelSort wrapping the BobguiTreeModelSort of window 2"));
      bobgui_window_set_child (BOBGUI_WINDOW (window3), vbox3);
      
      scrolled_window3 = bobgui_scrolled_window_new ();
      bobgui_scrolled_window_set_has_frame (BOBGUI_SCROLLED_WINDOW (scrolled_window3), TRUE);
      bobgui_scrolled_window_set_policy (BOBGUI_SCROLLED_WINDOW (scrolled_window3),
				      BOBGUI_POLICY_AUTOMATIC,
				      BOBGUI_POLICY_AUTOMATIC);
      bobgui_widget_set_vexpand (scrolled_window3, TRUE);
      bobgui_box_append (BOBGUI_BOX (vbox3), scrolled_window3);


      tree_view3 = bobgui_tree_view_new_with_model (ssmodel);
      
      renderer = bobgui_cell_renderer_text_new ();
      column = bobgui_tree_view_column_new_with_attributes ("First Word", renderer,
							 "text", WORD_COLUMN_1,
							 NULL);
      bobgui_tree_view_append_column (BOBGUI_TREE_VIEW (tree_view3), column);
      bobgui_tree_view_column_set_sort_column_id (column, WORD_COLUMN_1);
      
      renderer = bobgui_cell_renderer_text_new ();
      column = bobgui_tree_view_column_new_with_attributes ("Second Word", renderer,
							 "text", WORD_COLUMN_2,
							 NULL);
      bobgui_tree_view_column_set_sort_column_id (column, WORD_COLUMN_2);
      bobgui_tree_view_append_column (BOBGUI_TREE_VIEW (tree_view3), column);
      
      renderer = bobgui_cell_renderer_text_new ();
      column = bobgui_tree_view_column_new_with_attributes ("Third Word", renderer,
							 "text", WORD_COLUMN_3,
							 NULL);
      bobgui_tree_view_column_set_sort_column_id (column, WORD_COLUMN_3);
      bobgui_tree_view_append_column (BOBGUI_TREE_VIEW (tree_view3), column);
      
      renderer = bobgui_cell_renderer_text_new ();
      column = bobgui_tree_view_column_new_with_attributes ("Fourth Word", renderer,
							 "text", WORD_COLUMN_4,
							 NULL);
      bobgui_tree_view_column_set_sort_column_id (column, WORD_COLUMN_4);
      bobgui_tree_view_append_column (BOBGUI_TREE_VIEW (tree_view3), column);
      
      /*      bobgui_tree_sortable_set_default_sort_func (BOBGUI_TREE_SORTABLE (ssmodel),
					       (BobguiTreeIterCompareFunc)bobgui_tree_data_list_compare_func,
					       NULL, NULL);
      bobgui_tree_sortable_set_sort_column_id (BOBGUI_TREE_SORTABLE (ssmodel),
					    WORD_COLUMN_1,
					    BOBGUI_SORT_ASCENDING);*/

      bobgui_scrolled_window_set_child (BOBGUI_SCROLLED_WINDOW (scrolled_window3), tree_view3);
      bobgui_window_set_default_size (BOBGUI_WINDOW (window3), 400, 400);
      bobgui_window_present (BOBGUI_WINDOW (window3));
    }

  for (i = 0; data[i].word_1 != NULL; i++)
    {
      int k;
      
      bobgui_tree_store_prepend (BOBGUI_TREE_STORE (model), &iter, NULL);
      bobgui_tree_store_set (BOBGUI_TREE_STORE (model), &iter,
			  WORD_COLUMN_1, data[i].word_1,
			  WORD_COLUMN_2, data[i].word_2,
			  WORD_COLUMN_3, data[i].word_3,
			  WORD_COLUMN_4, data[i].word_4,
			  -1);
      for (k = 0; childdata[k].word_1 != NULL; k++)
	{
	  BobguiTreeIter child_iter;
	  
	  bobgui_tree_store_append (BOBGUI_TREE_STORE (model), &child_iter, &iter);
	  bobgui_tree_store_set (BOBGUI_TREE_STORE (model), &child_iter,
			      WORD_COLUMN_1, childdata[k].word_1,
			      WORD_COLUMN_2, childdata[k].word_2,
			      WORD_COLUMN_3, childdata[k].word_3,
			      WORD_COLUMN_4, childdata[k].word_4,
			      -1);
	}
    }

  while (!done)
    g_main_context_iteration (NULL, TRUE);
  
  return 0;
}
