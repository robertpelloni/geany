/* testcombo.c
 * Copyright (C) 2003  Kristian Rietveld
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
#include <bobgui/bobgui.h>

#include <string.h>
#include <stdio.h>

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

/**
 * oh yes, this test app surely has a lot of ugly code
 */

/* blaat */
static BobguiTreeModel *
create_tree_blaat (void)
{
        BobguiWidget *cellview;
        BobguiTreeIter iter, iter2;
        BobguiTreeStore *store;

        cellview = bobgui_cell_view_new ();

	store = bobgui_tree_store_new (3, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_BOOLEAN);

        bobgui_tree_store_append (store, &iter, NULL);
        bobgui_tree_store_set (store, &iter,
                            0, "dialog-warning",
                            1, "dialog-warning",
			    2, FALSE,
                            -1);

        bobgui_tree_store_append (store, &iter2, &iter);			       
        bobgui_tree_store_set (store, &iter2,
                            0, "process-stop",
                            1, "process-stop",
			    2, FALSE,
                            -1);

        bobgui_tree_store_append (store, &iter2, &iter);			       
        bobgui_tree_store_set (store, &iter2,
                            0, "document-new",
                            1, "document-new",
			    2, FALSE,
                            -1);

        bobgui_tree_store_append (store, &iter, NULL);
        bobgui_tree_store_set (store, &iter,
                            0, "edit-clear",
                            1, "edit-clear",
			    2, FALSE,
                            -1);

#if 0
        bobgui_tree_store_append (store, &iter, NULL);
        bobgui_tree_store_set (store, &iter,
                            0, NULL,
                            1, "separator",
			    2, TRUE,
                            -1);
#endif

        bobgui_tree_store_append (store, &iter, NULL);
        bobgui_tree_store_set (store, &iter,
                            0, "document-open",
                            1, "document-open",
			    2, FALSE,
                            -1);

        g_object_unref (g_object_ref_sink (cellview));

        return BOBGUI_TREE_MODEL (store);
}

static BobguiTreeModel *
create_empty_list_blaat (void)
{
        BobguiWidget *cellview;
        BobguiTreeIter iter;
        BobguiListStore *store;

        cellview = bobgui_cell_view_new ();

        store = bobgui_list_store_new (2, G_TYPE_STRING, G_TYPE_STRING);

        bobgui_list_store_append (store, &iter);
        bobgui_list_store_set (store, &iter,
                            0, "dialog-warning",
                            1, "dialog-warning",
                            -1);

        g_object_unref (g_object_ref_sink (cellview));

        return BOBGUI_TREE_MODEL (store);
}

static void
populate_list_blaat (gpointer data)
{
  BobguiComboBox *combo_box = BOBGUI_COMBO_BOX (data);
  BobguiListStore *store;
  BobguiWidget *cellview;
  BobguiTreeIter iter;
  
  store = BOBGUI_LIST_STORE (bobgui_combo_box_get_model (combo_box));

  bobgui_tree_model_get_iter_first (BOBGUI_TREE_MODEL (store), &iter);

  if (bobgui_tree_model_iter_next (BOBGUI_TREE_MODEL (store), &iter))
    return;

  cellview = bobgui_cell_view_new ();
  
  bobgui_list_store_append (store, &iter);			       
  bobgui_list_store_set (store, &iter,
		      0, "process-stop",
		      1, "process-stop",
		      -1);
  
  bobgui_list_store_append (store, &iter);			       
  bobgui_list_store_set (store, &iter,
		      0, "document-new",
		      1, "document-new",
		      -1);
  
  bobgui_list_store_append (store, &iter);
  bobgui_list_store_set (store, &iter,
		      0, "edit-clear",
		      1, "edit-clear",
		      -1);
  
  bobgui_list_store_append (store, &iter);
  bobgui_list_store_set (store, &iter,
		      0, NULL,
		      1, "separator",
		      -1);
  
  bobgui_list_store_append (store, &iter);
  bobgui_list_store_set (store, &iter,
		      0, "document-open",
		      1, "document-open",
		      -1);
  
  g_object_unref (g_object_ref_sink (cellview));
}

static BobguiTreeModel *
create_list_blaat (void)
{
        BobguiWidget *cellview;
        BobguiTreeIter iter;
        BobguiListStore *store;

        cellview = bobgui_cell_view_new ();

        store = bobgui_list_store_new (2, G_TYPE_STRING, G_TYPE_STRING);

        bobgui_list_store_append (store, &iter);
        bobgui_list_store_set (store, &iter,
                            0, "dialog-warning",
                            1, "dialog-warning",
                            -1);

        bobgui_list_store_append (store, &iter);			       
        bobgui_list_store_set (store, &iter,
                            0, "process-stop",
                            1, "process-stop",
                            -1);

        bobgui_list_store_append (store, &iter);			       
        bobgui_list_store_set (store, &iter,
                            0, "document-new",
                            1, "document-new",
                            -1);

        bobgui_list_store_append (store, &iter);
        bobgui_list_store_set (store, &iter,
                            0, "edit-clear",
                            1, "edit-clear",
                            -1);

        bobgui_list_store_append (store, &iter);
        bobgui_list_store_set (store, &iter,
                            0, NULL,
                            1, "separator",
                            -1);

        bobgui_list_store_append (store, &iter);
        bobgui_list_store_set (store, &iter,
                            0, "document-open",
                            1, "document-open",
                            -1);

        g_object_unref (g_object_ref_sink (cellview));

        return BOBGUI_TREE_MODEL (store);
}


static BobguiTreeModel *
create_list_long (void)
{
        BobguiTreeIter iter;
        BobguiListStore *store;

        store = bobgui_list_store_new (1, G_TYPE_STRING);

        bobgui_list_store_append (store, &iter);
        bobgui_list_store_set (store, &iter,
                            0, "here is some long long text that grows out of the combo's allocation",
                            -1);


        bobgui_list_store_append (store, &iter);
        bobgui_list_store_set (store, &iter,
                            0, "with at least a few of these rows",
                            -1);

        bobgui_list_store_append (store, &iter);
        bobgui_list_store_set (store, &iter,
                            0, "so that we can get some ellipsized text here",
                            -1);

        bobgui_list_store_append (store, &iter);
        bobgui_list_store_set (store, &iter,
                            0, "and see the combo box menu being allocated without any constraints",
                            -1);

        return BOBGUI_TREE_MODEL (store);
}

static BobguiTreeModel *
create_food_list (void)
{
        BobguiTreeIter iter;
        BobguiListStore *store;

        store = bobgui_list_store_new (2, G_TYPE_STRING, G_TYPE_STRING);
        bobgui_list_store_append (store, &iter);
        bobgui_list_store_set (store, &iter,
                            0, "Pepperoni",
                            1, "Pizza",
                            -1);

        bobgui_list_store_append (store, &iter);			       
        bobgui_list_store_set (store, &iter,
                            0, "Cheese",
                            1, "Burger",
                            -1);

        bobgui_list_store_append (store, &iter);			       
        bobgui_list_store_set (store, &iter,
                            0, "Pineapple",
                            1, "Milkshake",
                            -1);

        bobgui_list_store_append (store, &iter);			       
        bobgui_list_store_set (store, &iter,
                            0, "Orange",
                            1, "Soda",
                            -1);

        bobgui_list_store_append (store, &iter);			       
        bobgui_list_store_set (store, &iter,
                            0, "Club",
                            1, "Sandwich",
                            -1);

        return BOBGUI_TREE_MODEL (store);
}


/* blaat */
static BobguiTreeModel *
create_phylogenetic_tree (void)
{
        BobguiTreeIter iter, iter2, iter3;
        BobguiTreeStore *store;

	store = bobgui_tree_store_new (1,G_TYPE_STRING);

        bobgui_tree_store_append (store, &iter, NULL);
        bobgui_tree_store_set (store, &iter,
                            0, "Eubacteria",
                            -1);

        bobgui_tree_store_append (store, &iter2, &iter);			       
        bobgui_tree_store_set (store, &iter2,
                            0, "Aquifecales",
                            -1);

        bobgui_tree_store_append (store, &iter2, &iter);			       
        bobgui_tree_store_set (store, &iter2,
                            0, "Thermotogales",
                            -1);

        bobgui_tree_store_append (store, &iter2, &iter);			       
        bobgui_tree_store_set (store, &iter2,
                            0, "Thermodesulfobacterium",
                            -1);

        bobgui_tree_store_append (store, &iter2, &iter);			       
        bobgui_tree_store_set (store, &iter2,
                            0, "Thermus-Deinococcus group",
                            -1);

        bobgui_tree_store_append (store, &iter2, &iter);			       
        bobgui_tree_store_set (store, &iter2,
                            0, "Chloroflecales",
                            -1);

        bobgui_tree_store_append (store, &iter2, &iter);			       
        bobgui_tree_store_set (store, &iter2,
                            0, "Cyanobacteria",
                            -1);

        bobgui_tree_store_append (store, &iter2, &iter);			       
        bobgui_tree_store_set (store, &iter2,
                            0, "Firmicutes",
                            -1);

        bobgui_tree_store_append (store, &iter2, &iter);			       
        bobgui_tree_store_set (store, &iter2,
                            0, "Leptospirillium Group",
                            -1);

        bobgui_tree_store_append (store, &iter2, &iter);			       
        bobgui_tree_store_set (store, &iter2,
                            0, "Synergistes",
                            -1);
        bobgui_tree_store_append (store, &iter2, &iter);			       
        bobgui_tree_store_set (store, &iter2,
                            0, "Chlorobium-Flavobacteria group",
                            -1);
        bobgui_tree_store_append (store, &iter2, &iter);			       
        bobgui_tree_store_set (store, &iter2,
                            0, "Chlamydia-Verrucomicrobia group",
                            -1);

        bobgui_tree_store_append (store, &iter3, &iter2);			       
        bobgui_tree_store_set (store, &iter3,
                            0, "Verrucomicrobia",
                            -1);
        bobgui_tree_store_append (store, &iter3, &iter2);			       
        bobgui_tree_store_set (store, &iter3,
                            0, "Chlamydia",
                            -1);

        bobgui_tree_store_append (store, &iter2, &iter);			       
        bobgui_tree_store_set (store, &iter2,
                            0, "Flexistipes",
                            -1);


        bobgui_tree_store_append (store, &iter2, &iter);			       
        bobgui_tree_store_set (store, &iter2,
                            0, "Fibrobacter group",
                            -1);


        bobgui_tree_store_append (store, &iter2, &iter);			       
        bobgui_tree_store_set (store, &iter2,
                            0, "spirocheteus",
                            -1);


        bobgui_tree_store_append (store, &iter2, &iter);			       
        bobgui_tree_store_set (store, &iter2,
                            0, "Proteobacteria",
                            -1);


        bobgui_tree_store_append (store, &iter3, &iter2);			       
        bobgui_tree_store_set (store, &iter3,
                            0, "alpha",
                            -1);


        bobgui_tree_store_append (store, &iter3, &iter2);			       
        bobgui_tree_store_set (store, &iter3,
                            0, "beta",
                            -1);


        bobgui_tree_store_append (store, &iter3, &iter2);			       
        bobgui_tree_store_set (store, &iter3,
                            0, "delta ",
                            -1);


        bobgui_tree_store_append (store, &iter3, &iter2);			       
        bobgui_tree_store_set (store, &iter3,
                            0, "epsilon",
                            -1);


        bobgui_tree_store_append (store, &iter3, &iter2);  
        bobgui_tree_store_set (store, &iter3,
                            0, "gamma ",
                            -1);


        bobgui_tree_store_append (store, &iter, NULL);			       
        bobgui_tree_store_set (store, &iter,
                            0, "Eukaryotes",
                            -1);


        bobgui_tree_store_append (store, &iter2, &iter);			       
        bobgui_tree_store_set (store, &iter2,
                            0, "Metazoa",
                            -1);


        bobgui_tree_store_append (store, &iter2, &iter);			       
        bobgui_tree_store_set (store, &iter2,
                            0, "Bilateria",
                            -1);


        bobgui_tree_store_append (store, &iter2, &iter);			       
        bobgui_tree_store_set (store, &iter2,
                            0, "Myxozoa",
                            -1);


        bobgui_tree_store_append (store, &iter2, &iter);			       
        bobgui_tree_store_set (store, &iter2,
                            0, "Cnidaria",
                            -1);


        bobgui_tree_store_append (store, &iter2, &iter);			       
        bobgui_tree_store_set (store, &iter2,
                            0, "Ctenophora",
                            -1);


        bobgui_tree_store_append (store, &iter2, &iter);			       
        bobgui_tree_store_set (store, &iter2,
                            0, "Placozoa",
                            -1);


        bobgui_tree_store_append (store, &iter2, &iter);			       
        bobgui_tree_store_set (store, &iter2,
                            0, "Porifera",
                            -1);


        bobgui_tree_store_append (store, &iter2, &iter);			       
        bobgui_tree_store_set (store, &iter2,
                            0, "choanoflagellates",
                            -1);


        bobgui_tree_store_append (store, &iter2, &iter);			       
        bobgui_tree_store_set (store, &iter2,
                            0, "Fungi",
                            -1);


        bobgui_tree_store_append (store, &iter2, &iter);			       
        bobgui_tree_store_set (store, &iter2,
                            0, "Microsporidia",
                            -1);


        bobgui_tree_store_append (store, &iter2, &iter);			       
        bobgui_tree_store_set (store, &iter2,
                            0, "Aleveolates",
                            -1);


        bobgui_tree_store_append (store, &iter2, &iter);			       
        bobgui_tree_store_set (store, &iter2,
                            0, "Stramenopiles",
                            -1);


        bobgui_tree_store_append (store, &iter2, &iter);			       
        bobgui_tree_store_set (store, &iter2,
                            0, "Rhodophyta",
                            -1);


        bobgui_tree_store_append (store, &iter2, &iter);			       
        bobgui_tree_store_set (store, &iter2,
                            0, "Viridaeplantae",
                            -1);


        bobgui_tree_store_append (store, &iter2, &iter);			       
        bobgui_tree_store_set (store, &iter2,
                            0, "crytomonads et al",
                            -1);


        bobgui_tree_store_append (store, &iter, NULL);			       
        bobgui_tree_store_set (store, &iter,
                            0, "Archaea ",
                            -1);


        bobgui_tree_store_append (store, &iter2, &iter);			       
        bobgui_tree_store_set (store, &iter2,
                            0, "Korarchaeota",
                            -1);


        bobgui_tree_store_append (store, &iter2, &iter);			       
        bobgui_tree_store_set (store, &iter2,
                            0, "Crenarchaeota",
                            -1);


        bobgui_tree_store_append (store, &iter2, &iter);			       
        bobgui_tree_store_set (store, &iter2,
                            0, "Buryarchaeota",
                            -1);

        return BOBGUI_TREE_MODEL (store);
}


/* blaat */
static BobguiTreeModel *
create_capital_tree (void)
{
        BobguiTreeIter iter, iter2;
        BobguiTreeStore *store;

	store = bobgui_tree_store_new (1, G_TYPE_STRING);

        bobgui_tree_store_append (store, &iter, NULL);
        bobgui_tree_store_set (store, &iter, 0, "A - B", -1);

        bobgui_tree_store_append (store, &iter2, &iter);
        bobgui_tree_store_set (store, &iter2, 0, "Albany", -1);

        bobgui_tree_store_append (store, &iter2, &iter);
        bobgui_tree_store_set (store, &iter2, 0, "Annapolis", -1);

        bobgui_tree_store_append (store, &iter2, &iter);
        bobgui_tree_store_set (store, &iter2, 0, "Atlanta", -1);

        bobgui_tree_store_append (store, &iter2, &iter);
        bobgui_tree_store_set (store, &iter2, 0, "Augusta", -1);

        bobgui_tree_store_append (store, &iter2, &iter);
        bobgui_tree_store_set (store, &iter2, 0, "Austin", -1);

        bobgui_tree_store_append (store, &iter2, &iter);
        bobgui_tree_store_set (store, &iter2, 0, "Baton Rouge", -1);

        bobgui_tree_store_append (store, &iter2, &iter);
        bobgui_tree_store_set (store, &iter2, 0, "Bismarck", -1);

        bobgui_tree_store_append (store, &iter2, &iter);
        bobgui_tree_store_set (store, &iter2, 0, "Boise", -1);

        bobgui_tree_store_append (store, &iter2, &iter);
        bobgui_tree_store_set (store, &iter2, 0, "Boston", -1);

        bobgui_tree_store_append (store, &iter, NULL);
        bobgui_tree_store_set (store, &iter, 0, "C - D", -1);

        bobgui_tree_store_append (store, &iter2, &iter);
        bobgui_tree_store_set (store, &iter2, 0, "Carson City", -1);

        bobgui_tree_store_append (store, &iter2, &iter);
        bobgui_tree_store_set (store, &iter2, 0, "Charleston", -1);

        bobgui_tree_store_append (store, &iter2, &iter);
        bobgui_tree_store_set (store, &iter2, 0, "Cheyenne", -1);

        bobgui_tree_store_append (store, &iter2, &iter);
        bobgui_tree_store_set (store, &iter2, 0, "Columbia", -1);

        bobgui_tree_store_append (store, &iter2, &iter);
        bobgui_tree_store_set (store, &iter2, 0, "Columbus", -1);

        bobgui_tree_store_append (store, &iter2, &iter);
        bobgui_tree_store_set (store, &iter2, 0, "Concord", -1);

        bobgui_tree_store_append (store, &iter2, &iter);
        bobgui_tree_store_set (store, &iter2, 0, "Denver", -1);

        bobgui_tree_store_append (store, &iter2, &iter);
        bobgui_tree_store_set (store, &iter2, 0, "Des Moines", -1);

        bobgui_tree_store_append (store, &iter2, &iter);
        bobgui_tree_store_set (store, &iter2, 0, "Dover", -1);


        bobgui_tree_store_append (store, &iter, NULL);
        bobgui_tree_store_set (store, &iter, 0, "E - J", -1);

        bobgui_tree_store_append (store, &iter2, &iter);
        bobgui_tree_store_set (store, &iter2, 0, "Frankfort", -1);

        bobgui_tree_store_append (store, &iter2, &iter);
        bobgui_tree_store_set (store, &iter2, 0, "Harrisburg", -1);

        bobgui_tree_store_append (store, &iter2, &iter);
        bobgui_tree_store_set (store, &iter2, 0, "Hartford", -1);

        bobgui_tree_store_append (store, &iter2, &iter);
        bobgui_tree_store_set (store, &iter2, 0, "Helena", -1);

        bobgui_tree_store_append (store, &iter2, &iter);
        bobgui_tree_store_set (store, &iter2, 0, "Honolulu", -1);

        bobgui_tree_store_append (store, &iter2, &iter);
        bobgui_tree_store_set (store, &iter2, 0, "Indianapolis", -1);

        bobgui_tree_store_append (store, &iter2, &iter);
        bobgui_tree_store_set (store, &iter2, 0, "Jackson", -1);

        bobgui_tree_store_append (store, &iter2, &iter);
        bobgui_tree_store_set (store, &iter2, 0, "Jefferson City", -1);

        bobgui_tree_store_append (store, &iter2, &iter);
        bobgui_tree_store_set (store, &iter2, 0, "Juneau", -1);


        bobgui_tree_store_append (store, &iter, NULL);
        bobgui_tree_store_set (store, &iter, 0, "K - O", -1);

        bobgui_tree_store_append (store, &iter2, &iter);
        bobgui_tree_store_set (store, &iter2, 0, "Lansing", -1);

        bobgui_tree_store_append (store, &iter2, &iter);
        bobgui_tree_store_set (store, &iter2, 0, "Lincoln", -1);

        bobgui_tree_store_append (store, &iter2, &iter);
        bobgui_tree_store_set (store, &iter2, 0, "Little Rock", -1);

        bobgui_tree_store_append (store, &iter2, &iter);
        bobgui_tree_store_set (store, &iter2, 0, "Madison", -1);

        bobgui_tree_store_append (store, &iter2, &iter);
        bobgui_tree_store_set (store, &iter2, 0, "Montgomery", -1);

        bobgui_tree_store_append (store, &iter2, &iter);
        bobgui_tree_store_set (store, &iter2, 0, "Montpelier", -1);

        bobgui_tree_store_append (store, &iter2, &iter);
        bobgui_tree_store_set (store, &iter2, 0, "Nashville", -1);

        bobgui_tree_store_append (store, &iter2, &iter);
        bobgui_tree_store_set (store, &iter2, 0, "Oklahoma City", -1);

        bobgui_tree_store_append (store, &iter2, &iter);
        bobgui_tree_store_set (store, &iter2, 0, "Olympia", -1);


        bobgui_tree_store_append (store, &iter, NULL);
        bobgui_tree_store_set (store, &iter, 0, "P - S", -1);

        bobgui_tree_store_append (store, &iter2, &iter);
        bobgui_tree_store_set (store, &iter2, 0, "Phoenix", -1);

        bobgui_tree_store_append (store, &iter2, &iter);
        bobgui_tree_store_set (store, &iter2, 0, "Pierre", -1);

        bobgui_tree_store_append (store, &iter2, &iter);
        bobgui_tree_store_set (store, &iter2, 0, "Providence", -1);

        bobgui_tree_store_append (store, &iter2, &iter);
        bobgui_tree_store_set (store, &iter2, 0, "Raleigh", -1);

        bobgui_tree_store_append (store, &iter2, &iter);
        bobgui_tree_store_set (store, &iter2, 0, "Richmond", -1);

        bobgui_tree_store_append (store, &iter2, &iter);
        bobgui_tree_store_set (store, &iter2, 0, "Sacramento", -1);

        bobgui_tree_store_append (store, &iter2, &iter);
        bobgui_tree_store_set (store, &iter2, 0, "Salem", -1);

        bobgui_tree_store_append (store, &iter2, &iter);
        bobgui_tree_store_set (store, &iter2, 0, "Salt Lake City", -1);

        bobgui_tree_store_append (store, &iter2, &iter);
        bobgui_tree_store_set (store, &iter2, 0, "Santa Fe", -1);

        bobgui_tree_store_append (store, &iter2, &iter);
        bobgui_tree_store_set (store, &iter2, 0, "Springfield", -1);

        bobgui_tree_store_append (store, &iter2, &iter);
        bobgui_tree_store_set (store, &iter2, 0, "St. Paul", -1);


        bobgui_tree_store_append (store, &iter, NULL);
        bobgui_tree_store_set (store, &iter, 0, "T - Z", -1);

        bobgui_tree_store_append (store, &iter2, &iter);
        bobgui_tree_store_set (store, &iter2, 0, "Tallahassee", -1);

        bobgui_tree_store_append (store, &iter2, &iter);
        bobgui_tree_store_set (store, &iter2, 0, "Topeka", -1);

        bobgui_tree_store_append (store, &iter2, &iter);
        bobgui_tree_store_set (store, &iter2, 0, "Trenton", -1);

        return BOBGUI_TREE_MODEL (store);
}

static void
capital_sensitive (BobguiCellLayout   *cell_layout,
		   BobguiCellRenderer *cell,
		   BobguiTreeModel    *tree_model,
		   BobguiTreeIter     *iter,
		   gpointer         data)
{
  gboolean sensitive;

  sensitive = !bobgui_tree_model_iter_has_child (tree_model, iter);

  g_object_set (cell, "sensitive", sensitive, NULL);
}

static gboolean
capital_animation (gpointer data)
{
  static int insert_count = 0;
  BobguiTreeModel *model = BOBGUI_TREE_MODEL (data);
  BobguiTreePath *path;
  BobguiTreeIter iter, parent;

  switch (insert_count % 8)
    {
    case 0:
      bobgui_tree_store_insert (BOBGUI_TREE_STORE (model), &iter, NULL, 0);
      bobgui_tree_store_set (BOBGUI_TREE_STORE (model), 
			  &iter,
			  0, "Europe", -1);
      break;

    case 1:
      path = bobgui_tree_path_new_from_indices (0, -1);
      bobgui_tree_model_get_iter (model, &parent, path);
      bobgui_tree_path_free (path);
      bobgui_tree_store_insert (BOBGUI_TREE_STORE (model), &iter, &parent, 0);
      bobgui_tree_store_set (BOBGUI_TREE_STORE (model), 
			  &iter,
			  0, "Berlin", -1);
      break;

    case 2:
      path = bobgui_tree_path_new_from_indices (0, -1);
      bobgui_tree_model_get_iter (model, &parent, path);
      bobgui_tree_path_free (path);
      bobgui_tree_store_insert (BOBGUI_TREE_STORE (model), &iter, &parent, 1);
      bobgui_tree_store_set (BOBGUI_TREE_STORE (model), 
			  &iter,
			  0, "London", -1);
      break;

    case 3:
      path = bobgui_tree_path_new_from_indices (0, -1);
      bobgui_tree_model_get_iter (model, &parent, path);
      bobgui_tree_path_free (path);
      bobgui_tree_store_insert (BOBGUI_TREE_STORE (model), &iter, &parent, 2);
      bobgui_tree_store_set (BOBGUI_TREE_STORE (model), 
			  &iter,
			  0, "Paris", -1);
      break;

    case 4:
      path = bobgui_tree_path_new_from_indices (0, 2, -1);
      bobgui_tree_model_get_iter (model, &iter, path);
      bobgui_tree_path_free (path);
      bobgui_tree_store_remove (BOBGUI_TREE_STORE (model), &iter);
      break;

    case 5:
      path = bobgui_tree_path_new_from_indices (0, 1, -1);
      bobgui_tree_model_get_iter (model, &iter, path);
      bobgui_tree_path_free (path);
      bobgui_tree_store_remove (BOBGUI_TREE_STORE (model), &iter);
      break;

    case 6:
      path = bobgui_tree_path_new_from_indices (0, 0, -1);
      bobgui_tree_model_get_iter (model, &iter, path);
      bobgui_tree_path_free (path);
      bobgui_tree_store_remove (BOBGUI_TREE_STORE (model), &iter);
      break;

    case 7:
      path = bobgui_tree_path_new_from_indices (0, -1);
      bobgui_tree_model_get_iter (model, &iter, path);
      bobgui_tree_path_free (path);
      bobgui_tree_store_remove (BOBGUI_TREE_STORE (model), &iter);
      break;

    default: ;

    }
  insert_count++;

  return TRUE;
}

static void
setup_combo_entry (BobguiComboBoxText *combo)
{
  bobgui_combo_box_text_append_text (combo,
				   "dum de dum");
  bobgui_combo_box_text_append_text (combo,
				   "la la la");
  bobgui_combo_box_text_append_text (combo,
				   "la la la dum de dum la la la la la la boom de da la la");
  bobgui_combo_box_text_append_text (combo,
				   "bloop");
  bobgui_combo_box_text_append_text (combo,
				   "bleep");
  bobgui_combo_box_text_append_text (combo,
				   "klaas");
  bobgui_combo_box_text_append_text (combo,
				   "klaas0");
  bobgui_combo_box_text_append_text (combo,
				   "klaas1");
  bobgui_combo_box_text_append_text (combo,
				   "klaas2");
  bobgui_combo_box_text_append_text (combo,
				   "klaas3");
  bobgui_combo_box_text_append_text (combo,
				   "klaas4");
  bobgui_combo_box_text_append_text (combo,
				   "klaas5");
  bobgui_combo_box_text_append_text (combo,
				   "klaas6");
  bobgui_combo_box_text_append_text (combo,
				   "klaas7");
  bobgui_combo_box_text_append_text (combo,
				   "klaas8");
  bobgui_combo_box_text_append_text (combo,
				   "klaas9");
  bobgui_combo_box_text_append_text (combo,
				   "klaasa");
  bobgui_combo_box_text_append_text (combo,
				   "klaasb");
  bobgui_combo_box_text_append_text (combo,
				   "klaasc");
  bobgui_combo_box_text_append_text (combo,
				   "klaasd");
  bobgui_combo_box_text_append_text (combo,
				   "klaase");
  bobgui_combo_box_text_append_text (combo,
				   "klaasf");
  bobgui_combo_box_text_append_text (combo,
				   "klaas10");
  bobgui_combo_box_text_append_text (combo,
				   "klaas11");
  bobgui_combo_box_text_append_text (combo,
				   "klaas12");
}

static void
set_sensitive (BobguiCellLayout   *cell_layout,
	       BobguiCellRenderer *cell,
	       BobguiTreeModel    *tree_model,
	       BobguiTreeIter     *iter,
	       gpointer         data)
{
  BobguiTreePath *path;
  int *indices;
  gboolean sensitive;

  path = bobgui_tree_model_get_path (tree_model, iter);
  indices = bobgui_tree_path_get_indices (path);
  sensitive = indices[0] != 1;
  bobgui_tree_path_free (path);

  g_object_set (cell, "sensitive", sensitive, NULL);
}

static gboolean
is_separator (BobguiTreeModel *model,
	      BobguiTreeIter  *iter,
	      gpointer      data)
{
  BobguiTreePath *path;
  gboolean result;

  path = bobgui_tree_model_get_path (model, iter);
  result = bobgui_tree_path_get_indices (path)[0] == 4;
  bobgui_tree_path_free (path);

  return result;
  
}

static void
displayed_row_changed (BobguiComboBox *combo,
                       BobguiCellView *cell)
{
  int row;
  BobguiTreePath *path;

  row = bobgui_combo_box_get_active (combo);
  path = bobgui_tree_path_new_from_indices (row, -1);
  bobgui_cell_view_set_displayed_row (cell, path);
  bobgui_tree_path_free (path);
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
        BobguiWidget *window, *cellview, *mainbox;
        BobguiWidget *combobox, *comboboxtext;
        BobguiWidget *tmp, *boom;
        BobguiCellRenderer *renderer;
        BobguiTreeModel *model;
	BobguiTreePath *path;
	BobguiTreeIter iter;
	BobguiCellArea *area;
        char *text;
        int i;
        gboolean done = FALSE;

        bobgui_init ();

	if (g_getenv ("RTL"))
	  bobgui_widget_set_default_direction (BOBGUI_TEXT_DIR_RTL);

        window = bobgui_window_new ();
        g_signal_connect (window, "destroy", G_CALLBACK (quit_cb), &done);

        mainbox = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 2);
        bobgui_window_set_child (BOBGUI_WINDOW (window), mainbox);

        /* BobguiCellView */
        tmp = bobgui_frame_new ("BobguiCellView");
        bobgui_box_append (BOBGUI_BOX (mainbox), tmp);

        boom = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 0);
        bobgui_frame_set_child (BOBGUI_FRAME (tmp), boom);

        cellview = bobgui_cell_view_new ();
        renderer = bobgui_cell_renderer_pixbuf_new ();
        bobgui_cell_layout_pack_start (BOBGUI_CELL_LAYOUT (cellview),
                                    renderer,
                                    FALSE);
        g_object_set (renderer, "icon-name", "dialog-warning", NULL);

        renderer = bobgui_cell_renderer_text_new ();
        bobgui_cell_layout_pack_start (BOBGUI_CELL_LAYOUT (cellview),
                                    renderer,
                                    TRUE);
        g_object_set (renderer, "text", "la la la", NULL);
        bobgui_box_append (BOBGUI_BOX (boom), cellview);

        /* BobguiComboBox list */
        tmp = bobgui_frame_new ("BobguiComboBox (list)");
        bobgui_box_append (BOBGUI_BOX (mainbox), tmp);

        boom = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 0);
        bobgui_frame_set_child (BOBGUI_FRAME (tmp), boom);

        model = create_list_blaat ();
        combobox = bobgui_combo_box_new_with_model (model);
        g_object_unref (model);
        bobgui_box_append (BOBGUI_BOX (boom), combobox);

        renderer = bobgui_cell_renderer_pixbuf_new ();
        bobgui_cell_layout_pack_start (BOBGUI_CELL_LAYOUT (combobox),
                                    renderer,
                                    FALSE);
        bobgui_cell_layout_set_attributes (BOBGUI_CELL_LAYOUT (combobox), renderer,
                                        "icon-name", 0,
                                        NULL);
	bobgui_cell_layout_set_cell_data_func (BOBGUI_CELL_LAYOUT (combobox),
					    renderer,
					    set_sensitive,
					    NULL, NULL);

        renderer = bobgui_cell_renderer_text_new ();
        bobgui_cell_layout_pack_start (BOBGUI_CELL_LAYOUT (combobox),
                                    renderer,
                                    TRUE);
        bobgui_cell_layout_set_attributes (BOBGUI_CELL_LAYOUT (combobox), renderer,
                                        "text", 1,
                                        NULL);
	bobgui_cell_layout_set_cell_data_func (BOBGUI_CELL_LAYOUT (combobox),
					    renderer,
					    set_sensitive,
					    NULL, NULL);
	bobgui_combo_box_set_row_separator_func (BOBGUI_COMBO_BOX (combobox),
					      is_separator, NULL, NULL);

        bobgui_combo_box_set_active (BOBGUI_COMBO_BOX (combobox), 0);

        /* BobguiComboBox dynamic list */
        tmp = bobgui_frame_new ("BobguiComboBox (dynamic list)");
        bobgui_box_append (BOBGUI_BOX (mainbox), tmp);

        boom = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 0);
        bobgui_frame_set_child (BOBGUI_FRAME (tmp), boom);

        model = create_empty_list_blaat ();
        combobox = bobgui_combo_box_new_with_model (model);
	g_signal_connect (combobox, "notify::popup-shown", 
			  G_CALLBACK (populate_list_blaat), combobox);

        g_object_unref (model);
        bobgui_box_append (BOBGUI_BOX (boom), combobox);

        renderer = bobgui_cell_renderer_pixbuf_new ();
        bobgui_cell_layout_pack_start (BOBGUI_CELL_LAYOUT (combobox),
                                    renderer,
                                    FALSE);
        bobgui_cell_layout_set_attributes (BOBGUI_CELL_LAYOUT (combobox), renderer,
                                        "icon-name", 0,
                                        NULL);
	bobgui_cell_layout_set_cell_data_func (BOBGUI_CELL_LAYOUT (combobox),
					    renderer,
					    set_sensitive,
					    NULL, NULL);

        renderer = bobgui_cell_renderer_text_new ();
        bobgui_cell_layout_pack_start (BOBGUI_CELL_LAYOUT (combobox),
                                    renderer,
                                    TRUE);
        bobgui_cell_layout_set_attributes (BOBGUI_CELL_LAYOUT (combobox), renderer,
                                        "text", 1,
                                        NULL);
	bobgui_cell_layout_set_cell_data_func (BOBGUI_CELL_LAYOUT (combobox),
					    renderer,
					    set_sensitive,
					    NULL, NULL);
	bobgui_combo_box_set_row_separator_func (BOBGUI_COMBO_BOX (combobox),
					      is_separator, NULL, NULL);

        bobgui_combo_box_set_active (BOBGUI_COMBO_BOX (combobox), 0);

        /* BobguiComboBox custom entry */
        tmp = bobgui_frame_new ("BobguiComboBox (custom)");
        bobgui_box_append (BOBGUI_BOX (mainbox), tmp);

        boom = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 0);
        bobgui_frame_set_child (BOBGUI_FRAME (tmp), boom);

        model = create_list_blaat ();
        combobox = bobgui_combo_box_new_with_model (model);
        g_object_unref (model);
        bobgui_box_append (BOBGUI_BOX (boom), combobox);

        renderer = bobgui_cell_renderer_pixbuf_new ();
        bobgui_cell_layout_pack_start (BOBGUI_CELL_LAYOUT (combobox),
                                    renderer,
                                    FALSE);
        bobgui_cell_layout_set_attributes (BOBGUI_CELL_LAYOUT (combobox), renderer,
                                        "icon-name", 0,
                                        NULL);
	bobgui_cell_layout_set_cell_data_func (BOBGUI_CELL_LAYOUT (combobox),
					    renderer,
					    set_sensitive,
					    NULL, NULL);

        renderer = bobgui_cell_renderer_text_new ();
        bobgui_cell_layout_pack_start (BOBGUI_CELL_LAYOUT (combobox),
                                    renderer,
                                    TRUE);
        bobgui_cell_layout_set_attributes (BOBGUI_CELL_LAYOUT (combobox), renderer,
                                        "text", 1,
                                        NULL);
	bobgui_cell_layout_set_cell_data_func (BOBGUI_CELL_LAYOUT (combobox),
					    renderer,
					    set_sensitive,
					    NULL, NULL);
	bobgui_combo_box_set_row_separator_func (BOBGUI_COMBO_BOX (combobox), 
					      is_separator, NULL, NULL);
						
        bobgui_combo_box_set_active (BOBGUI_COMBO_BOX (combobox), 0);

        tmp = bobgui_cell_view_new ();
        bobgui_cell_view_set_model (BOBGUI_CELL_VIEW (tmp), model);

        renderer = bobgui_cell_renderer_text_new ();
        bobgui_cell_layout_pack_start (BOBGUI_CELL_LAYOUT (tmp), renderer, TRUE);
        bobgui_cell_layout_set_attributes (BOBGUI_CELL_LAYOUT (tmp), renderer,
                                        "text", 1,
                                        NULL);
        displayed_row_changed (BOBGUI_COMBO_BOX (combobox), BOBGUI_CELL_VIEW (tmp));
        g_signal_connect (combobox, "changed", G_CALLBACK (displayed_row_changed), tmp);

        bobgui_combo_box_set_child (BOBGUI_COMBO_BOX (combobox), tmp);

        /* BobguiComboBox tree */
        tmp = bobgui_frame_new ("BobguiComboBox (tree)");
        bobgui_box_append (BOBGUI_BOX (mainbox), tmp);

        boom = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 0);
        bobgui_frame_set_child (BOBGUI_FRAME (tmp), boom);

        model = create_tree_blaat ();
        combobox = bobgui_combo_box_new_with_model (model);
        g_object_unref (model);
        bobgui_box_append (BOBGUI_BOX (boom), combobox);

        renderer = bobgui_cell_renderer_pixbuf_new ();
        bobgui_cell_layout_pack_start (BOBGUI_CELL_LAYOUT (combobox),
                                    renderer,
                                    FALSE);
        bobgui_cell_layout_set_attributes (BOBGUI_CELL_LAYOUT (combobox), renderer,
                                        "icon-name", 0,
                                        NULL);
	bobgui_cell_layout_set_cell_data_func (BOBGUI_CELL_LAYOUT (combobox),
					    renderer,
					    set_sensitive,
					    NULL, NULL);

        renderer = bobgui_cell_renderer_text_new ();
        bobgui_cell_layout_pack_start (BOBGUI_CELL_LAYOUT (combobox),
                                    renderer,
                                    TRUE);
        bobgui_cell_layout_set_attributes (BOBGUI_CELL_LAYOUT (combobox), renderer,
                                        "text", 1,
                                        NULL);
	bobgui_cell_layout_set_cell_data_func (BOBGUI_CELL_LAYOUT (combobox),
					    renderer,
					    set_sensitive,
					    NULL, NULL);
	bobgui_combo_box_set_row_separator_func (BOBGUI_COMBO_BOX (combobox), 
					      is_separator, NULL, NULL);
						
        bobgui_combo_box_set_active (BOBGUI_COMBO_BOX (combobox), 0);
#if 0
	g_timeout_add (1000, (GSourceFunc) animation_timer, model);
#endif

        /* BobguiComboBox (grid mode) */
        tmp = bobgui_frame_new ("BobguiComboBox (grid mode)");
        bobgui_box_append (BOBGUI_BOX (mainbox), tmp);

        boom = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 0);
        bobgui_frame_set_child (BOBGUI_FRAME (tmp), boom);


        /* BobguiComboBoxEntry */
        tmp = bobgui_frame_new ("BobguiComboBox with entry");
        bobgui_box_append (BOBGUI_BOX (mainbox), tmp);

        boom = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 0);
        bobgui_frame_set_child (BOBGUI_FRAME (tmp), boom);

        comboboxtext = bobgui_combo_box_text_new_with_entry ();
        setup_combo_entry (BOBGUI_COMBO_BOX_TEXT (comboboxtext));
        bobgui_box_append (BOBGUI_BOX (boom), comboboxtext);


        /* Phylogenetic tree */
        tmp = bobgui_frame_new ("What are you ?");
        bobgui_box_append (BOBGUI_BOX (mainbox), tmp);

        boom = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 0);
        bobgui_frame_set_child (BOBGUI_FRAME (tmp), boom);

        model = create_phylogenetic_tree ();
        combobox = bobgui_combo_box_new_with_model (model);
        g_object_unref (model);
        bobgui_box_append (BOBGUI_BOX (boom), combobox);

        renderer = bobgui_cell_renderer_text_new ();
        bobgui_cell_layout_pack_start (BOBGUI_CELL_LAYOUT (combobox),
                                    renderer,
                                    TRUE);
        bobgui_cell_layout_set_attributes (BOBGUI_CELL_LAYOUT (combobox), renderer,
                                        "text", 0,
                                        NULL);

        bobgui_combo_box_set_active (BOBGUI_COMBO_BOX (combobox), 0);

        /* Capitals */
        tmp = bobgui_frame_new ("Where are you ?");
        bobgui_box_append (BOBGUI_BOX (mainbox), tmp);

        boom = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 0);
        bobgui_frame_set_child (BOBGUI_FRAME (tmp), boom);

        model = create_capital_tree ();
	combobox = bobgui_combo_box_new_with_model (model);
        g_object_unref (model);
        bobgui_box_append (BOBGUI_BOX (boom), combobox);
        renderer = bobgui_cell_renderer_text_new ();
        bobgui_cell_layout_pack_start (BOBGUI_CELL_LAYOUT (combobox),
                                    renderer,
                                    TRUE);
        bobgui_cell_layout_set_attributes (BOBGUI_CELL_LAYOUT (combobox), renderer,
                                        "text", 0,
                                        NULL);
	bobgui_cell_layout_set_cell_data_func (BOBGUI_CELL_LAYOUT (combobox),
					    renderer,
					    capital_sensitive,
					    NULL, NULL);
	path = bobgui_tree_path_new_from_indices (0, 8, -1);
	bobgui_tree_model_get_iter (model, &iter, path);
	bobgui_tree_path_free (path);
        bobgui_combo_box_set_active_iter (BOBGUI_COMBO_BOX (combobox), &iter);

#if 1
	g_timeout_add (1000, (GSourceFunc) capital_animation, model);
#endif

        /* Aligned Food */
        tmp = bobgui_frame_new ("Hungry ?");
        bobgui_box_append (BOBGUI_BOX (mainbox), tmp);

        boom = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 0);
        bobgui_frame_set_child (BOBGUI_FRAME (tmp), boom);

        model = create_food_list ();
	combobox = bobgui_combo_box_new_with_model (model);
        g_object_unref (model);
        bobgui_box_append (BOBGUI_BOX (boom), combobox);

	area = bobgui_cell_layout_get_area (BOBGUI_CELL_LAYOUT (combobox));

        renderer = bobgui_cell_renderer_text_new ();
	bobgui_cell_area_add_with_properties (area, renderer, 
					   "align", TRUE, 
					   "expand", TRUE, 
					   NULL);
        bobgui_cell_layout_set_attributes (BOBGUI_CELL_LAYOUT (combobox), renderer,
                                        "text", 0,
                                        NULL);

        renderer = bobgui_cell_renderer_text_new ();
	bobgui_cell_area_add_with_properties (area, renderer, 
					   "align", TRUE, 
					   "expand", TRUE, 
					   NULL);
        bobgui_cell_layout_set_attributes (BOBGUI_CELL_LAYOUT (combobox), renderer,
                                        "text", 1,
                                        NULL);

        bobgui_combo_box_set_active (BOBGUI_COMBO_BOX (combobox), 0);

	/* Ellipsizing growing combos */
        tmp = bobgui_frame_new ("Unconstrained Menu");
        bobgui_box_append (BOBGUI_BOX (mainbox), tmp);

        boom = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 0);
        bobgui_frame_set_child (BOBGUI_FRAME (tmp), boom);

	model = create_list_long ();
	combobox = bobgui_combo_box_new_with_model (model);
        g_object_unref (model);
        bobgui_box_append (BOBGUI_BOX (boom), combobox);
        renderer = bobgui_cell_renderer_text_new ();
	g_object_set (G_OBJECT (renderer), "ellipsize", PANGO_ELLIPSIZE_END, NULL);

        bobgui_cell_layout_pack_start (BOBGUI_CELL_LAYOUT (combobox), renderer, TRUE);
        bobgui_cell_layout_set_attributes (BOBGUI_CELL_LAYOUT (combobox), renderer,
                                        "text", 0, NULL);
        bobgui_combo_box_set_active (BOBGUI_COMBO_BOX (combobox), 0);
	bobgui_combo_box_set_popup_fixed_width (BOBGUI_COMBO_BOX (combobox), FALSE);

        tmp = bobgui_frame_new ("Looong");
        bobgui_box_append (BOBGUI_BOX (mainbox), tmp);
        combobox = bobgui_combo_box_text_new ();
        for (i = 0; i < 200; i++)
          {
            text = g_strdup_printf ("Item %d", i);
            bobgui_combo_box_text_append_text (BOBGUI_COMBO_BOX_TEXT (combobox), text);
            g_free (text);
          }
        bobgui_combo_box_set_active (BOBGUI_COMBO_BOX (combobox), 53);
        bobgui_frame_set_child (BOBGUI_FRAME (tmp), combobox);

        bobgui_window_present (BOBGUI_WINDOW (window));

        while (!done)
          g_main_context_iteration (NULL, TRUE);

        return 0;
}
