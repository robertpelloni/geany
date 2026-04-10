#include <bobgui/bobgui.h>

#ifdef SMALL
#define AVERAGE 15
#define VARIANCE 10
#else
#define AVERAGE 300
#define VARIANCE 200
#endif

static void
setup_list_item (BobguiSignalListItemFactory *factory,
                 BobguiListItem              *list_item)
{
  BobguiWidget *label = bobgui_label_new ("");

  bobgui_list_item_set_child (list_item, label);
}

static void
bind_list_item (BobguiSignalListItemFactory *factory,
                BobguiListItem              *list_item)
{
  BobguiWidget *label;
  gpointer item;
  char *s;

  item = bobgui_list_item_get_item (list_item);

  if (item)
    s = g_strdup_printf ("%u: %s",
                         bobgui_list_item_get_position (list_item),
                         (const char *) g_object_get_data (item, "message"));
  else
    s = NULL;

  label = bobgui_list_item_get_child (list_item);
  bobgui_label_set_text (BOBGUI_LABEL (label), s);

  g_free (s);
}

static BobguiWidget *
create_widget_for_listbox (gpointer item,
                           gpointer unused)
{
  const char *message = g_object_get_data (item, "message");
  BobguiWidget *widget;

  widget = bobgui_label_new (message);

  return widget;
}

static guint
get_number (GObject *item)
{
  return GPOINTER_TO_UINT (g_object_get_data (item, "counter")) % 1000;
}

static void
add (GListStore *store)
{
  static guint counter;
  GObject *o;
  char *message;
  guint pos;

  counter++;
  o = g_object_new (G_TYPE_OBJECT, NULL);
  g_object_set_data (o, "counter", GUINT_TO_POINTER (counter));
  message = g_strdup_printf ("Item %u", counter);
  g_object_set_data_full (o, "message", message, g_free);

  pos = g_random_int_range (0, g_list_model_get_n_items (G_LIST_MODEL (store)) + 1);
  g_list_store_insert (store, pos, o);
  g_object_unref (o);
}

static void
delete (GListStore *store)
{
  guint pos;

  pos = g_random_int_range (0, g_list_model_get_n_items (G_LIST_MODEL (store)));
  g_list_store_remove (store, pos);
}

static gboolean
do_stuff (gpointer store)
{
  if (g_random_int_range (AVERAGE - VARIANCE, AVERAGE + VARIANCE) < g_list_model_get_n_items (store))
    delete (store);
  else
    add (store);

  return G_SOURCE_CONTINUE;
}

static gboolean
revert_sort (gpointer sorter)
{
  if (bobgui_numeric_sorter_get_sort_order (sorter) == BOBGUI_SORT_ASCENDING)
    bobgui_numeric_sorter_set_sort_order (sorter, BOBGUI_SORT_DESCENDING);
  else
    bobgui_numeric_sorter_set_sort_order (sorter, BOBGUI_SORT_ASCENDING);

  return G_SOURCE_CONTINUE;
}

int
main (int   argc,
      char *argv[])
{
  BobguiWidget *win, *hbox, *vbox, *sw, *listview, *listbox, *label;
  GListStore *store;
  GListModel *toplevels;
  BobguiSortListModel *sort;
  BobguiSorter *sorter;
  guint i;
  BobguiListItemFactory *factory;
  BobguiSelectionModel *selection;

  bobgui_init ();

  store = g_list_store_new (G_TYPE_OBJECT);
  for (i = 0; i < AVERAGE; i++)
    add (store);
  sorter = BOBGUI_SORTER (bobgui_numeric_sorter_new (bobgui_cclosure_expression_new (G_TYPE_UINT, NULL, 0, NULL, (GCallback)get_number, NULL, NULL)));
  sort = bobgui_sort_list_model_new (G_LIST_MODEL (store), sorter);

  win = bobgui_window_new ();
  bobgui_window_set_default_size (BOBGUI_WINDOW (win), 400, 600);

  hbox = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 4);
  bobgui_window_set_child (BOBGUI_WINDOW (win), hbox);

  vbox = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 4);
  bobgui_box_append (BOBGUI_BOX (hbox), vbox);

  label = bobgui_label_new ("BobguiListView");
  bobgui_box_append (BOBGUI_BOX (vbox), label);

  sw = bobgui_scrolled_window_new ();
  bobgui_widget_set_hexpand (sw, TRUE);
  bobgui_widget_set_vexpand (sw, TRUE);
  bobgui_box_append (BOBGUI_BOX (vbox), sw);

  factory = bobgui_signal_list_item_factory_new ();
  g_signal_connect (factory, "setup", G_CALLBACK (setup_list_item), NULL);
  g_signal_connect (factory, "bind", G_CALLBACK (bind_list_item), NULL);
  listview = bobgui_list_view_new (NULL, factory);

  bobgui_scrolled_window_set_child (BOBGUI_SCROLLED_WINDOW (sw), listview);

  vbox = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 4);
  bobgui_box_append (BOBGUI_BOX (hbox), vbox);

  label = bobgui_label_new ("BobguiListBox");
  bobgui_box_append (BOBGUI_BOX (vbox), label);

  sw = bobgui_scrolled_window_new ();
  bobgui_widget_set_hexpand (sw, TRUE);
  bobgui_widget_set_vexpand (sw, TRUE);
  bobgui_box_append (BOBGUI_BOX (vbox), sw);

  listbox = bobgui_list_box_new ();
  bobgui_scrolled_window_set_child (BOBGUI_SCROLLED_WINDOW (sw), listbox);

  selection = BOBGUI_SELECTION_MODEL (bobgui_single_selection_new (G_LIST_MODEL (sort)));
  bobgui_list_view_set_model (BOBGUI_LIST_VIEW (listview), selection);
  g_object_unref (selection);
  bobgui_list_box_bind_model (BOBGUI_LIST_BOX (listbox),
                           G_LIST_MODEL (sort),
                           create_widget_for_listbox,
                           NULL, NULL);

  g_timeout_add (100, do_stuff, store);
  g_timeout_add_seconds (3, revert_sort, sorter);

  bobgui_window_present (BOBGUI_WINDOW (win));

  toplevels = bobgui_window_get_toplevels ();
  while (g_list_model_get_n_items (toplevels))
    g_main_context_iteration (NULL, TRUE);

  return 0;
}
