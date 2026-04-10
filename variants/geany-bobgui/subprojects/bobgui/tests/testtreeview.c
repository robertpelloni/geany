/* testtreeview.c
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

#include <string.h>
#include <bobgui/bobgui.h>
#include <stdlib.h>

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

/* Don't copy this bad example; inline RGB data is always a better
 * idea than inline XPMs.
 */
static const char *book_closed_xpm[] = {
"16 16 6 1",
"       c None s None",
".      c black",
"X      c red",
"o      c yellow",
"O      c #808080",
"#      c white",
"                ",
"       ..       ",
"     ..XX.      ",
"   ..XXXXX.     ",
" ..XXXXXXXX.    ",
".ooXXXXXXXXX.   ",
"..ooXXXXXXXXX.  ",
".X.ooXXXXXXXXX. ",
".XX.ooXXXXXX..  ",
" .XX.ooXXX..#O  ",
"  .XX.oo..##OO. ",
"   .XX..##OO..  ",
"    .X.#OO..    ",
"     ..O..      ",
"      ..        ",
"                "
};

static void run_automated_tests (void);

/* This custom model is to test custom model use. */

#define BOBGUI_TYPE_MODEL_TYPES				(bobgui_tree_model_types_get_type ())
#define BOBGUI_TREE_MODEL_TYPES(obj)			(G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_MODEL_TYPES, BobguiTreeModelTypes))
#define BOBGUI_TREE_MODEL_TYPES_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST ((klass), BOBGUI_TYPE_MODEL_TYPES, BobguiTreeModelTypesClass))
#define BOBGUI_IS_TREE_MODEL_TYPES(obj)			(G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOBGUI_TYPE_MODEL_TYPES))
#define BOBGUI_IS_TREE_MODEL_TYPES_GET_CLASS(klass)	(G_TYPE_INSTANCE_GET_CLASS ((obj), BOBGUI_TYPE_MODEL_TYPES))

typedef struct _BobguiTreeModelTypes       BobguiTreeModelTypes;
typedef struct _BobguiTreeModelTypesClass  BobguiTreeModelTypesClass;

struct _BobguiTreeModelTypes
{
  GObject parent;

  int stamp;
};

struct _BobguiTreeModelTypesClass
{
  GObjectClass parent_class;

  guint        (* get_flags)       (BobguiTreeModel *tree_model);   
  int          (* get_n_columns)   (BobguiTreeModel *tree_model);
  GType        (* get_column_type) (BobguiTreeModel *tree_model,
				    int           index);
  gboolean     (* get_iter)        (BobguiTreeModel *tree_model,
				    BobguiTreeIter  *iter,
				    BobguiTreePath  *path);
  BobguiTreePath *(* get_path)        (BobguiTreeModel *tree_model,
				    BobguiTreeIter  *iter);
  void         (* get_value)       (BobguiTreeModel *tree_model,
				    BobguiTreeIter  *iter,
				    int           column,
				    GValue       *value);
  gboolean     (* iter_next)       (BobguiTreeModel *tree_model,
				    BobguiTreeIter  *iter);
  gboolean     (* iter_children)   (BobguiTreeModel *tree_model,
				    BobguiTreeIter  *iter,
				    BobguiTreeIter  *parent);
  gboolean     (* iter_has_child)  (BobguiTreeModel *tree_model,
				    BobguiTreeIter  *iter);
  int          (* iter_n_children) (BobguiTreeModel *tree_model,
				    BobguiTreeIter  *iter);
  gboolean     (* iter_nth_child)  (BobguiTreeModel *tree_model,
				    BobguiTreeIter  *iter,
				    BobguiTreeIter  *parent,
				    int           n);
  gboolean     (* iter_parent)     (BobguiTreeModel *tree_model,
				    BobguiTreeIter  *iter,
				    BobguiTreeIter  *child);
  void         (* ref_iter)        (BobguiTreeModel *tree_model,
				    BobguiTreeIter  *iter);
  void         (* unref_iter)      (BobguiTreeModel *tree_model,
				    BobguiTreeIter  *iter);

  /* These will be moved into the BobguiTreeModelIface eventually */
  void         (* changed)         (BobguiTreeModel *tree_model,
				    BobguiTreePath  *path,
				    BobguiTreeIter  *iter);
  void         (* inserted)        (BobguiTreeModel *tree_model,
				    BobguiTreePath  *path,
				    BobguiTreeIter  *iter);
  void         (* child_toggled)   (BobguiTreeModel *tree_model,
				    BobguiTreePath  *path,
				    BobguiTreeIter  *iter);
  void         (* deleted)         (BobguiTreeModel *tree_model,
				    BobguiTreePath  *path);
};

GType              bobgui_tree_model_types_get_type      (void) G_GNUC_CONST;
BobguiTreeModelTypes *bobgui_tree_model_types_new           (void);

typedef enum
{
  COLUMNS_NONE,
  COLUMNS_ONE,
  COLUMNS_LOTS,
  COLUMNS_LAST
} ColumnsType;

static const char *column_type_names[] = {
  "No columns",
  "One column",
  "Many columns"
};

#define N_COLUMNS 9

static GType*
get_model_types (void)
{
  static GType column_types[N_COLUMNS] = { 0 };
  
  if (column_types[0] == 0)
    {
      column_types[0] = G_TYPE_STRING;
      column_types[1] = G_TYPE_STRING;
      column_types[2] = GDK_TYPE_PIXBUF;
      column_types[3] = G_TYPE_FLOAT;
      column_types[4] = G_TYPE_UINT;
      column_types[5] = G_TYPE_UCHAR;
      column_types[6] = G_TYPE_CHAR;
#define BOOL_COLUMN 7
      column_types[BOOL_COLUMN] = G_TYPE_BOOLEAN;
      column_types[8] = G_TYPE_INT;
    }

  return column_types;
}

static void
toggled_callback (BobguiCellRendererToggle *celltoggle,
                  char                  *path_string,
                  BobguiTreeView           *tree_view)
{
  BobguiTreeModel *model = NULL;
  BobguiTreeModelSort *sort_model = NULL;
  BobguiTreePath *path;
  BobguiTreeIter iter;
  gboolean active = FALSE;
  
  g_return_if_fail (BOBGUI_IS_TREE_VIEW (tree_view));

  model = bobgui_tree_view_get_model (tree_view);
  
  if (BOBGUI_IS_TREE_MODEL_SORT (model))
    {
      sort_model = BOBGUI_TREE_MODEL_SORT (model);
      model = bobgui_tree_model_sort_get_model (sort_model);
    }

  if (model == NULL)
    return;

  if (sort_model)
    {
      g_warning ("FIXME implement conversion from TreeModelSort iter to child model iter");
      return;
    }
      
  path = bobgui_tree_path_new_from_string (path_string);
  if (!bobgui_tree_model_get_iter (model,
                                &iter, path))
    {
      g_warning ("%s: bad path?", G_STRLOC);
      return;
    }
  bobgui_tree_path_free (path);
  
  if (BOBGUI_IS_LIST_STORE (model))
    {
      bobgui_tree_model_get (BOBGUI_TREE_MODEL (model),
                          &iter,
                          BOOL_COLUMN,
                          &active,
                          -1);
      
      bobgui_list_store_set (BOBGUI_LIST_STORE (model),
                          &iter,
                          BOOL_COLUMN,
                          !active,
                          -1);
    }
  else if (BOBGUI_IS_TREE_STORE (model))
    {
      bobgui_tree_model_get (BOBGUI_TREE_MODEL (model),
                          &iter,
                          BOOL_COLUMN,
                          &active,
                          -1);
            
      bobgui_tree_store_set (BOBGUI_TREE_STORE (model),
                          &iter,
                          BOOL_COLUMN,
                          !active,
                          -1);
    }
  else
    g_warning ("don't know how to actually toggle value for model type %s",
               g_type_name (G_TYPE_FROM_INSTANCE (model)));
}

static void
edited_callback (BobguiCellRendererText *renderer,
		 const char    *path_string,
		 const char    *new_text,
		 BobguiTreeView  *tree_view)
{
  BobguiTreeModel *model = NULL;
  BobguiTreeModelSort *sort_model = NULL;
  BobguiTreePath *path;
  BobguiTreeIter iter;
  guint value = atoi (new_text);
  
  g_return_if_fail (BOBGUI_IS_TREE_VIEW (tree_view));

  model = bobgui_tree_view_get_model (tree_view);
  
  if (BOBGUI_IS_TREE_MODEL_SORT (model))
    {
      sort_model = BOBGUI_TREE_MODEL_SORT (model);
      model = bobgui_tree_model_sort_get_model (sort_model);
    }

  if (model == NULL)
    return;

  if (sort_model)
    {
      g_warning ("FIXME implement conversion from TreeModelSort iter to child model iter");
      return;
    }
      
  path = bobgui_tree_path_new_from_string (path_string);
  if (!bobgui_tree_model_get_iter (model,
                                &iter, path))
    {
      g_warning ("%s: bad path?", G_STRLOC);
      return;
    }
  bobgui_tree_path_free (path);

  if (BOBGUI_IS_LIST_STORE (model))
    {
      bobgui_list_store_set (BOBGUI_LIST_STORE (model),
                          &iter,
                          4,
                          value,
                          -1);
    }
  else if (BOBGUI_IS_TREE_STORE (model))
    {
      bobgui_tree_store_set (BOBGUI_TREE_STORE (model),
                          &iter,
                          4,
                          value,
                          -1);
    }
  else
    g_warning ("don't know how to actually toggle value for model type %s",
               g_type_name (G_TYPE_FROM_INSTANCE (model)));
}

static ColumnsType current_column_type = COLUMNS_LOTS;

static void
set_columns_type (BobguiTreeView *tree_view, ColumnsType type)
{
  BobguiTreeViewColumn *col;
  BobguiCellRenderer *rend;
  GdkPixbuf *pixbuf;
  BobguiWidget *image;
  BobguiAdjustment *adjustment;

  current_column_type = type;
  
  col = bobgui_tree_view_get_column (tree_view, 0);
  while (col)
    {
      bobgui_tree_view_remove_column (tree_view, col);

      col = bobgui_tree_view_get_column (tree_view, 0);
    }

  switch (type)
    {
    case COLUMNS_NONE:
      break;

    case COLUMNS_LOTS:
      rend = bobgui_cell_renderer_text_new ();

      col = bobgui_tree_view_column_new_with_attributes ("Column 1",
                                                      rend,
                                                      "text", 1,
                                                      NULL);
      
      bobgui_tree_view_append_column (BOBGUI_TREE_VIEW (tree_view), col);
      
      col = bobgui_tree_view_column_new();
      bobgui_tree_view_column_set_title (col, "Column 2");
      
      rend = bobgui_cell_renderer_pixbuf_new ();
      bobgui_tree_view_column_pack_start (col, rend, FALSE);
      bobgui_tree_view_column_add_attribute (col, rend, "pixbuf", 2);
      rend = bobgui_cell_renderer_text_new ();
      bobgui_tree_view_column_pack_start (col, rend, TRUE);
      bobgui_tree_view_column_add_attribute (col, rend, "text", 0);

      
      bobgui_tree_view_append_column (BOBGUI_TREE_VIEW (tree_view), col);
      bobgui_tree_view_set_expander_column (tree_view, col);
      
      rend = bobgui_cell_renderer_toggle_new ();

      g_signal_connect (rend, "toggled",
			G_CALLBACK (toggled_callback), tree_view);
      
      col = bobgui_tree_view_column_new_with_attributes ("Column 3",
                                                      rend,
                                                      "active", BOOL_COLUMN,
                                                      NULL);

      bobgui_tree_view_append_column (BOBGUI_TREE_VIEW (tree_view), col);

      pixbuf = gdk_pixbuf_new_from_xpm_data ((const char **)book_closed_xpm);

      image = bobgui_image_new_from_pixbuf (pixbuf);

      g_object_unref (pixbuf);

      bobgui_tree_view_column_set_widget (col, image);
      
      rend = bobgui_cell_renderer_toggle_new ();

      /* you could also set this per-row by tying it to a column
       * in the model of course.
       */
      g_object_set (rend, "radio", TRUE, NULL);
      
      g_signal_connect (rend, "toggled",
			G_CALLBACK (toggled_callback), tree_view);
      
      col = bobgui_tree_view_column_new_with_attributes ("Column 4",
                                                      rend,
                                                      "active", BOOL_COLUMN,
                                                      NULL);

      bobgui_tree_view_append_column (BOBGUI_TREE_VIEW (tree_view), col);

      rend = bobgui_cell_renderer_spin_new ();

      adjustment = bobgui_adjustment_new (0, 0, 10000, 100, 100, 100);
      g_object_set (rend, "editable", TRUE, NULL);
      g_object_set (rend, "adjustment", adjustment, NULL);

      g_signal_connect (rend, "edited",
			G_CALLBACK (edited_callback), tree_view);

      col = bobgui_tree_view_column_new_with_attributes ("Column 5",
                                                      rend,
                                                      "text", 4,
                                                      NULL);

      bobgui_tree_view_append_column (BOBGUI_TREE_VIEW (tree_view), col);
#if 0
      
      rend = bobgui_cell_renderer_text_new ();
      
      col = bobgui_tree_view_column_new_with_attributes ("Column 6",
                                                      rend,
                                                      "text", 4,
                                                      NULL);

      bobgui_tree_view_append_column (BOBGUI_TREE_VIEW (tree_view), col);
      
      rend = bobgui_cell_renderer_text_new ();
      
      col = bobgui_tree_view_column_new_with_attributes ("Column 7",
                                                      rend,
                                                      "text", 5,
                                                      NULL);

      bobgui_tree_view_append_column (BOBGUI_TREE_VIEW (tree_view), col);
      
      rend = bobgui_cell_renderer_text_new ();
      
      col = bobgui_tree_view_column_new_with_attributes ("Column 8",
                                                      rend,
                                                      "text", 6,
                                                      NULL);

      bobgui_tree_view_append_column (BOBGUI_TREE_VIEW (tree_view), col);
      
      rend = bobgui_cell_renderer_text_new ();
      
      col = bobgui_tree_view_column_new_with_attributes ("Column 9",
                                                      rend,
                                                      "text", 7,
                                                      NULL);

      bobgui_tree_view_append_column (BOBGUI_TREE_VIEW (tree_view), col);
      
      rend = bobgui_cell_renderer_text_new ();
      
      col = bobgui_tree_view_column_new_with_attributes ("Column 10",
                                                      rend,
                                                      "text", 8,
                                                      NULL);

      bobgui_tree_view_append_column (BOBGUI_TREE_VIEW (tree_view), col);
      
#endif
      
      G_GNUC_FALLTHROUGH;
      
    case COLUMNS_ONE:
      rend = bobgui_cell_renderer_text_new ();
      
      col = bobgui_tree_view_column_new_with_attributes ("Column 0",
                                                      rend,
                                                      "text", 0,
                                                      NULL);

      bobgui_tree_view_insert_column (BOBGUI_TREE_VIEW (tree_view), col, 0);
      break;
    case COLUMNS_LAST:
    default:
      break;
    }
}

static ColumnsType
get_columns_type (void)
{
  return current_column_type;
}

static GdkPixbuf *our_pixbuf;
  
typedef enum
{
  /*   MODEL_TYPES, */
  MODEL_TREE,
  MODEL_LIST,
  MODEL_SORTED_TREE,
  MODEL_SORTED_LIST,
  MODEL_EMPTY_LIST,
  MODEL_EMPTY_TREE,
  MODEL_NULL,
  MODEL_LAST
} ModelType;

/* FIXME add a custom model to test */
static BobguiTreeModel *models[MODEL_LAST];
static const char *model_names[MODEL_LAST] = {
  "BobguiTreeStore",
  "BobguiListStore",
  "BobguiTreeModelSort wrapping BobguiTreeStore",
  "BobguiTreeModelSort wrapping BobguiListStore",
  "Empty BobguiListStore",
  "Empty BobguiTreeStore",
  "NULL (no model)"
};

static BobguiTreeModel*
create_list_model (void)
{
  BobguiListStore *store;
  BobguiTreeIter iter;
  int i;
  GType *t;

  t = get_model_types ();
  
  store = bobgui_list_store_new (N_COLUMNS,
			      t[0], t[1], t[2],
			      t[3], t[4], t[5],
			      t[6], t[7], t[8]);

  i = 0;
  while (i < 200)
    {
      char *msg;
      
      bobgui_list_store_append (store, &iter);

      msg = g_strdup_printf ("%d", i);
      
      bobgui_list_store_set (store, &iter, 0, msg, 1, "Foo! Foo! Foo!",
                          2, our_pixbuf,
                          3, 7.0, 4, (guint) 9000,
                          5, 'f', 6, 'g',
                          7, TRUE, 8, 23245454,
                          -1);

      g_free (msg);
      
      ++i;
    }

  return BOBGUI_TREE_MODEL (store);
}

static void
typesystem_recurse (GType        type,
                    BobguiTreeIter *parent_iter,
                    BobguiTreeStore *store)
{
  GType* children;
  guint n_children = 0;
  int i;
  BobguiTreeIter iter;
  char *str;
  
  bobgui_tree_store_append (store, &iter, parent_iter);

  str = g_strdup_printf ("%ld", (glong)type);
  bobgui_tree_store_set (store, &iter, 0, str, 1, g_type_name (type),
                      2, our_pixbuf,
                      3, 7.0, 4, (guint) 9000,
                      5, 'f', 6, 'g',
                      7, TRUE, 8, 23245454,
                      -1);
  g_free (str);
  
  children = g_type_children (type, &n_children);

  i = 0;
  while (i < n_children)
    {
      typesystem_recurse (children[i], &iter, store);

      ++i;
    }
  
  g_free (children);
}

static BobguiTreeModel*
create_tree_model (void)
{
  BobguiTreeStore *store;
  int i;
  GType *t;
  
  /* Make the tree more interesting */
  /* - we need this magic here so we are sure the type ends up being
   * registered and gcc doesn't optimize away the code */
  g_type_class_unref (g_type_class_ref (bobgui_scrolled_window_get_type ()));
  g_type_class_unref (g_type_class_ref (bobgui_label_get_type ()));
  g_type_class_unref (g_type_class_ref (bobgui_scrollbar_get_type ()));
  g_type_class_unref (g_type_class_ref (pango_layout_get_type ()));

  t = get_model_types ();
  
  store = bobgui_tree_store_new (N_COLUMNS,
			      t[0], t[1], t[2],
			      t[3], t[4], t[5],
			      t[6], t[7], t[8]);

  i = 0;
  while (i < G_TYPE_FUNDAMENTAL_MAX)
    {
      typesystem_recurse (i, NULL, store);
      
      ++i;
    }

  return BOBGUI_TREE_MODEL (store);
}

static void
model_selected (BobguiComboBox *combo_box, gpointer data)
{
  BobguiTreeView *tree_view = BOBGUI_TREE_VIEW (data);
  int hist;

  hist = bobgui_combo_box_get_active (combo_box);

  if (models[hist] != bobgui_tree_view_get_model (tree_view))
    {
      bobgui_tree_view_set_model (tree_view, models[hist]);
    }
}

static void
columns_selected (BobguiComboBox *combo_box, gpointer data)
{
  BobguiTreeView *tree_view = BOBGUI_TREE_VIEW (data);
  int hist;

  hist = bobgui_combo_box_get_active (combo_box);

  if (hist != get_columns_type ())
    {
      set_columns_type (tree_view, hist);
    }
}

static void
on_row_activated (BobguiTreeView       *tree_view,
                  BobguiTreePath       *path,
                  BobguiTreeViewColumn *column,
                  gpointer           user_data)
{
  g_print ("Row activated\n");
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
main (int    argc,
      char **argv)
{
  BobguiWidget *window;
  BobguiWidget *sw;
  BobguiWidget *tv;
  BobguiWidget *box;
  BobguiWidget *combo_box;
  BobguiTreeModel *model;
  GdkContentFormats *targets;
  int i;
  gboolean done = FALSE;
  
  bobgui_init ();

  if (g_getenv ("RTL"))
    bobgui_widget_set_default_direction (BOBGUI_TEXT_DIR_RTL);

  our_pixbuf = gdk_pixbuf_new_from_xpm_data ((const char **) book_closed_xpm);  
  
#if 0
  models[MODEL_TYPES] = BOBGUI_TREE_MODEL (bobgui_tree_model_types_new ());
#endif
  models[MODEL_LIST] = create_list_model ();
  models[MODEL_TREE] = create_tree_model ();

  model = create_list_model ();
  models[MODEL_SORTED_LIST] = bobgui_tree_model_sort_new_with_model (model);
  g_object_unref (model);

  model = create_tree_model ();
  models[MODEL_SORTED_TREE] = bobgui_tree_model_sort_new_with_model (model);
  g_object_unref (model);

  models[MODEL_EMPTY_LIST] = BOBGUI_TREE_MODEL (bobgui_list_store_new (1, G_TYPE_INT));
  models[MODEL_EMPTY_TREE] = BOBGUI_TREE_MODEL (bobgui_tree_store_new (1, G_TYPE_INT));
  
  models[MODEL_NULL] = NULL;

  run_automated_tests ();
  
  window = bobgui_window_new ();
  g_signal_connect (window, "destroy", G_CALLBACK (quit_cb), &done);
  bobgui_window_set_default_size (BOBGUI_WINDOW (window), 430, 400);

  box = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 0);

  bobgui_window_set_child (BOBGUI_WINDOW (window), box);

  tv = bobgui_tree_view_new_with_model (models[0]);
  g_signal_connect (tv, "row-activated", G_CALLBACK (on_row_activated), NULL);

  targets = gdk_content_formats_new_for_gtype (BOBGUI_TYPE_TREE_ROW_DATA);
  bobgui_tree_view_enable_model_drag_source (BOBGUI_TREE_VIEW (tv),
					  GDK_BUTTON1_MASK,
                                          targets,
					  GDK_ACTION_MOVE | GDK_ACTION_COPY);

  bobgui_tree_view_enable_model_drag_dest (BOBGUI_TREE_VIEW (tv),
                                        targets,
					GDK_ACTION_MOVE | GDK_ACTION_COPY);
  gdk_content_formats_unref (targets);
  
  /* Model menu */
  combo_box = bobgui_combo_box_text_new ();
  bobgui_widget_set_halign (combo_box, BOBGUI_ALIGN_CENTER);
  for (i = 0; i < MODEL_LAST; i++)
      bobgui_combo_box_text_append_text (BOBGUI_COMBO_BOX_TEXT (combo_box), model_names[i]);

  bobgui_box_append (BOBGUI_BOX (box), combo_box);
  g_signal_connect (combo_box,
                    "changed",
                    G_CALLBACK (model_selected),
		    tv);
  
  /* Columns menu */
  combo_box = bobgui_combo_box_text_new ();
  bobgui_widget_set_halign (combo_box, BOBGUI_ALIGN_CENTER);
  for (i = 0; i < COLUMNS_LAST; i++)
      bobgui_combo_box_text_append_text (BOBGUI_COMBO_BOX_TEXT (combo_box), column_type_names[i]);

  bobgui_box_append (BOBGUI_BOX (box), combo_box);

  set_columns_type (BOBGUI_TREE_VIEW (tv), COLUMNS_LOTS);
  bobgui_combo_box_set_active (BOBGUI_COMBO_BOX (combo_box), COLUMNS_LOTS);

  g_signal_connect (combo_box,
                    "changed",
                    G_CALLBACK (columns_selected),
                    tv);
  
  sw = bobgui_scrolled_window_new ();
  bobgui_widget_set_hexpand (sw, TRUE);
  bobgui_widget_set_vexpand (sw, TRUE);
  bobgui_scrolled_window_set_policy (BOBGUI_SCROLLED_WINDOW (sw),
                                  BOBGUI_POLICY_AUTOMATIC,
                                  BOBGUI_POLICY_AUTOMATIC);
  
  bobgui_box_append (BOBGUI_BOX (box), sw);
  
  bobgui_scrolled_window_set_child (BOBGUI_SCROLLED_WINDOW (sw), tv);
  
  bobgui_window_present (BOBGUI_WINDOW (window));
  
  while (!done)
    g_main_context_iteration (NULL, TRUE);

  return 0;
}

/*
 * BobguiTreeModelTypes
 */

static void         bobgui_tree_model_types_init                 (BobguiTreeModelTypes      *model_types);
static void         bobgui_tree_model_types_tree_model_init      (BobguiTreeModelIface   *iface);
static int          bobgui_real_model_types_get_n_columns   (BobguiTreeModel        *tree_model);
static GType        bobgui_real_model_types_get_column_type (BobguiTreeModel        *tree_model,
							   int                  index);
static BobguiTreePath *bobgui_real_model_types_get_path        (BobguiTreeModel        *tree_model,
							   BobguiTreeIter         *iter);
static void         bobgui_real_model_types_get_value       (BobguiTreeModel        *tree_model,
							   BobguiTreeIter         *iter,
							   int                  column,
							   GValue              *value);
static gboolean     bobgui_real_model_types_iter_next       (BobguiTreeModel        *tree_model,
							   BobguiTreeIter         *iter);
static gboolean     bobgui_real_model_types_iter_children   (BobguiTreeModel        *tree_model,
							   BobguiTreeIter         *iter,
							   BobguiTreeIter         *parent);
static gboolean     bobgui_real_model_types_iter_has_child  (BobguiTreeModel        *tree_model,
							   BobguiTreeIter         *iter);
static int          bobgui_real_model_types_iter_n_children (BobguiTreeModel        *tree_model,
							   BobguiTreeIter         *iter);
static gboolean     bobgui_real_model_types_iter_nth_child  (BobguiTreeModel        *tree_model,
							   BobguiTreeIter         *iter,
							   BobguiTreeIter         *parent,
							   int                  n);
static gboolean     bobgui_real_model_types_iter_parent     (BobguiTreeModel        *tree_model,
							   BobguiTreeIter         *iter,
							   BobguiTreeIter         *child);


GType
bobgui_tree_model_types_get_type (void)
{
  static GType model_types_type = 0;

  if (!model_types_type)
    {
      const GTypeInfo model_types_info =
      {
        sizeof (BobguiTreeModelTypesClass),
	NULL,		/* base_init */
	NULL,		/* base_finalize */
        NULL,           /* class_init */
	NULL,		/* class_finalize */
	NULL,		/* class_data */
        sizeof (BobguiTreeModelTypes),
	0,
        (GInstanceInitFunc) bobgui_tree_model_types_init
      };

      const GInterfaceInfo tree_model_info =
      {
	(GInterfaceInitFunc) bobgui_tree_model_types_tree_model_init,
	NULL,
	NULL
      };

      model_types_type = g_type_register_static (G_TYPE_OBJECT,
						 "BobguiTreeModelTypes",
						 &model_types_info, 0);
      g_type_add_interface_static (model_types_type,
				   BOBGUI_TYPE_TREE_MODEL,
				   &tree_model_info);
    }

  return model_types_type;
}

BobguiTreeModelTypes *
bobgui_tree_model_types_new (void)
{
  BobguiTreeModelTypes *retval;

  retval = g_object_new (BOBGUI_TYPE_MODEL_TYPES, NULL);

  return retval;
}

static void
bobgui_tree_model_types_tree_model_init (BobguiTreeModelIface *iface)
{
  iface->get_n_columns = bobgui_real_model_types_get_n_columns;
  iface->get_column_type = bobgui_real_model_types_get_column_type;
  iface->get_path = bobgui_real_model_types_get_path;
  iface->get_value = bobgui_real_model_types_get_value;
  iface->iter_next = bobgui_real_model_types_iter_next;
  iface->iter_children = bobgui_real_model_types_iter_children;
  iface->iter_has_child = bobgui_real_model_types_iter_has_child;
  iface->iter_n_children = bobgui_real_model_types_iter_n_children;
  iface->iter_nth_child = bobgui_real_model_types_iter_nth_child;
  iface->iter_parent = bobgui_real_model_types_iter_parent;
}

static void
bobgui_tree_model_types_init (BobguiTreeModelTypes *model_types)
{
  model_types->stamp = g_random_int ();
}

static GType column_types[] = {
  G_TYPE_STRING, /* GType */
  G_TYPE_STRING  /* type name */
};
  
static int
bobgui_real_model_types_get_n_columns (BobguiTreeModel *tree_model)
{
  return G_N_ELEMENTS (column_types);
}

static GType
bobgui_real_model_types_get_column_type (BobguiTreeModel *tree_model,
                                      int           index)
{
  g_return_val_if_fail (index < G_N_ELEMENTS (column_types), G_TYPE_INVALID);
  
  return column_types[index];
}

#if 0
/* Use default implementation of this */
static gboolean
bobgui_real_model_types_get_iter (BobguiTreeModel *tree_model,
                               BobguiTreeIter  *iter,
                               BobguiTreePath  *path)
{
  
}
#endif

/* The toplevel nodes of the tree are the reserved types, G_TYPE_NONE through
 * G_TYPE_RESERVED_FUNDAMENTAL.
 */

static BobguiTreePath *
bobgui_real_model_types_get_path (BobguiTreeModel *tree_model,
                               BobguiTreeIter  *iter)
{
  BobguiTreePath *retval;
  GType type;
  GType parent;
  
  g_return_val_if_fail (BOBGUI_IS_TREE_MODEL_TYPES (tree_model), NULL);
  g_return_val_if_fail (iter != NULL, NULL);

  type = GPOINTER_TO_INT (iter->user_data);
  
  retval = bobgui_tree_path_new ();
  
  parent = g_type_parent (type);
  while (parent != G_TYPE_INVALID)
    {
      GType* children = g_type_children (parent, NULL);
      int i = 0;

      if (!children || children[0] == G_TYPE_INVALID)
        {
          g_warning ("bad iterator?");
          return NULL;
        }
      
      while (children[i] != type)
        ++i;

      bobgui_tree_path_prepend_index (retval, i);

      g_free (children);
      
      type = parent;
      parent = g_type_parent (parent);
    }

  /* The fundamental type itself is the index on the toplevel */
  bobgui_tree_path_prepend_index (retval, type);

  return retval;
}

static void
bobgui_real_model_types_get_value (BobguiTreeModel *tree_model,
                                BobguiTreeIter  *iter,
                                int           column,
                                GValue       *value)
{
  GType type;

  type = GPOINTER_TO_INT (iter->user_data);

  switch (column)
    {
    case 0:
      {
        char *str;
        
        g_value_init (value, G_TYPE_STRING);

        str = g_strdup_printf ("%ld", (long int) type);
        g_value_set_string (value, str);
        g_free (str);
      }
      break;

    case 1:
      g_value_init (value, G_TYPE_STRING);
      g_value_set_string (value, g_type_name (type));
      break;

    default:
      g_warning ("Bad column %d requested", column);
    }
}

static gboolean
bobgui_real_model_types_iter_next (BobguiTreeModel  *tree_model,
                                BobguiTreeIter   *iter)
{
  
  GType parent;
  GType type;

  type = GPOINTER_TO_INT (iter->user_data);

  parent = g_type_parent (type);
  
  if (parent == G_TYPE_INVALID)
    {
      /* find next _valid_ fundamental type */
      do
	type++;
      while (!g_type_name (type) && type <= G_TYPE_FUNDAMENTAL_MAX);
      if (type <= G_TYPE_FUNDAMENTAL_MAX)
	{
	  /* found one */
          iter->user_data = GINT_TO_POINTER (type);
          return TRUE;
        }
      else
        return FALSE;
    }
  else
    {
      GType* children = g_type_children (parent, NULL);
      int i = 0;

      g_assert (children != NULL);
      
      while (children[i] != type)
        ++i;
  
      ++i;

      if (children[i] != G_TYPE_INVALID)
        {
          iter->user_data = GINT_TO_POINTER (children[i]);
          g_free (children);
          return TRUE;
        }
      else
        {
          g_free (children);
          return FALSE;
        }
    }
}

static gboolean
bobgui_real_model_types_iter_children (BobguiTreeModel *tree_model,
                                    BobguiTreeIter  *iter,
                                    BobguiTreeIter  *parent)
{
  GType type;
  GType* children;
  
  type = GPOINTER_TO_INT (parent->user_data);

  children = g_type_children (type, NULL);

  if (!children || children[0] == G_TYPE_INVALID)
    {
      g_free (children);
      return FALSE;
    }
  else
    {
      iter->user_data = GINT_TO_POINTER (children[0]);
      g_free (children);
      return TRUE;
    }
}

static gboolean
bobgui_real_model_types_iter_has_child (BobguiTreeModel *tree_model,
                                     BobguiTreeIter  *iter)
{
  GType type;
  GType* children;
  
  type = GPOINTER_TO_INT (iter->user_data);
  
  children = g_type_children (type, NULL);

  if (!children || children[0] == G_TYPE_INVALID)
    {
      g_free (children);
      return FALSE;
    }
  else
    {
      g_free (children);
      return TRUE;
    }
}

static int
bobgui_real_model_types_iter_n_children (BobguiTreeModel *tree_model,
                                      BobguiTreeIter  *iter)
{
  if (iter == NULL)
    {
      return G_TYPE_FUNDAMENTAL_MAX;
    }
  else
    {
      GType type;
      GType* children;
      guint n_children = 0;

      type = GPOINTER_TO_INT (iter->user_data);
      
      children = g_type_children (type, &n_children);
      
      g_free (children);
      
      return n_children;
    }
}

static gboolean
bobgui_real_model_types_iter_nth_child (BobguiTreeModel *tree_model,
                                     BobguiTreeIter  *iter,
                                     BobguiTreeIter  *parent,
                                     int           n)
{  
  if (parent == NULL)
    {
      /* fundamental type */
      if (n < G_TYPE_FUNDAMENTAL_MAX)
        {
          iter->user_data = GINT_TO_POINTER (n);
          return TRUE;
        }
      else
        return FALSE;
    }
  else
    {
      GType type = GPOINTER_TO_INT (parent->user_data);      
      guint n_children = 0;
      GType* children = g_type_children (type, &n_children);

      if (n_children == 0)
        {
          g_free (children);
          return FALSE;
        }
      else if (n >= n_children)
        {
          g_free (children);
          return FALSE;
        }
      else
        {
          iter->user_data = GINT_TO_POINTER (children[n]);
          g_free (children);

          return TRUE;
        }
    }
}

static gboolean
bobgui_real_model_types_iter_parent (BobguiTreeModel *tree_model,
                                  BobguiTreeIter  *iter,
                                  BobguiTreeIter  *child)
{
  GType type;
  GType parent;
  
  type = GPOINTER_TO_INT (child->user_data);
  
  parent = g_type_parent (type);
  
  if (parent == G_TYPE_INVALID)
    {
      if (type > G_TYPE_FUNDAMENTAL_MAX)
        g_warning ("no parent for %ld %s\n",
                   (long int) type,
                   g_type_name (type));
      return FALSE;
    }
  else
    {
      iter->user_data = GINT_TO_POINTER (parent);
      
      return TRUE;
    }
}

/*
 * Automated testing
 */

#if 0

static void
treestore_torture_recurse (BobguiTreeStore *store,
                           BobguiTreeIter  *root,
                           int           depth)
{
  BobguiTreeModel *model;
  int i;
  BobguiTreeIter iter;  
  
  model = BOBGUI_TREE_MODEL (store);    

  if (depth > 2)
    return;

  ++depth;

  bobgui_tree_store_append (store, &iter, root);
  
  bobgui_tree_model_iter_children (model, &iter, root);
  
  i = 0;
  while (i < 100)
    {
      bobgui_tree_store_append (store, &iter, root);
      ++i;
    }

  while (bobgui_tree_model_iter_children (model, &iter, root))
    bobgui_tree_store_remove (store, &iter);

  bobgui_tree_store_append (store, &iter, root);

  /* inserts before last node in tree */
  i = 0;
  while (i < 100)
    {
      bobgui_tree_store_insert_before (store, &iter, root, &iter);
      ++i;
    }

  /* inserts after the node before the last node */
  i = 0;
  while (i < 100)
    {
      bobgui_tree_store_insert_after (store, &iter, root, &iter);
      ++i;
    }

  /* inserts after the last node */
  bobgui_tree_store_append (store, &iter, root);
    
  i = 0;
  while (i < 100)
    {
      bobgui_tree_store_insert_after (store, &iter, root, &iter);
      ++i;
    }

  /* remove everything again */
  while (bobgui_tree_model_iter_children (model, &iter, root))
    bobgui_tree_store_remove (store, &iter);


    /* Prepends */
  bobgui_tree_store_prepend (store, &iter, root);
    
  i = 0;
  while (i < 100)
    {
      bobgui_tree_store_prepend (store, &iter, root);
      ++i;
    }

  /* remove everything again */
  while (bobgui_tree_model_iter_children (model, &iter, root))
    bobgui_tree_store_remove (store, &iter);

  bobgui_tree_store_append (store, &iter, root);
  bobgui_tree_store_append (store, &iter, root);
  bobgui_tree_store_append (store, &iter, root);
  bobgui_tree_store_append (store, &iter, root);

  while (bobgui_tree_model_iter_children (model, &iter, root))
    {
      treestore_torture_recurse (store, &iter, depth);
      bobgui_tree_store_remove (store, &iter);
    }
}

#endif

static void
run_automated_tests (void)
{
  g_print ("Running automated tests...\n");
  
  /* FIXME TreePath basic verification */

  /* FIXME generic consistency checks on the models */

  {
    /* Make sure list store mutations don't crash anything */
    BobguiListStore *store;
    BobguiTreeModel *model;
    int i;
    BobguiTreeIter iter;
    
    store = bobgui_list_store_new (1, G_TYPE_INT);

    model = BOBGUI_TREE_MODEL (store);
    
    i = 0;
    while (i < 100)
      {
        bobgui_list_store_append (store, &iter);
        ++i;
      }

    while (bobgui_tree_model_get_iter_first (model, &iter))
      bobgui_list_store_remove (store, &iter);

    bobgui_list_store_append (store, &iter);

    /* inserts before last node in list */
    i = 0;
    while (i < 100)
      {
        bobgui_list_store_insert_before (store, &iter, &iter);
        ++i;
      }

    /* inserts after the node before the last node */
    i = 0;
    while (i < 100)
      {
        bobgui_list_store_insert_after (store, &iter, &iter);
        ++i;
      }

    /* inserts after the last node */
    bobgui_list_store_append (store, &iter);
    
    i = 0;
    while (i < 100)
      {
        bobgui_list_store_insert_after (store, &iter, &iter);
        ++i;
      }

    /* remove everything again */
    while (bobgui_tree_model_get_iter_first (model, &iter))
      bobgui_list_store_remove (store, &iter);


    /* Prepends */
    bobgui_list_store_prepend (store, &iter);
    
    i = 0;
    while (i < 100)
      {
        bobgui_list_store_prepend (store, &iter);
        ++i;
      }

    /* remove everything again */
    while (bobgui_tree_model_get_iter_first (model, &iter))
      bobgui_list_store_remove (store, &iter);
    
    g_object_unref (store);
  }

  {
    /* Make sure tree store mutations don't crash anything */
    BobguiTreeStore *store;
    BobguiTreeIter root;

    store = bobgui_tree_store_new (1, G_TYPE_INT);
    bobgui_tree_store_append (BOBGUI_TREE_STORE (store), &root, NULL);
    /* Remove test until it is rewritten to work */
    /*    treestore_torture_recurse (store, &root, 0);*/
    
    g_object_unref (store);
  }

  g_print ("Passed.\n");
}
