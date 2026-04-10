#include <bobgui/bobgui.h>

#include <string.h>

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

typedef struct _Row Row;
typedef struct _RowClass RowClass;

struct _Row
{
  BobguiListBoxRow parent_instance;
  BobguiWidget *label;
  int sort_id;
};

struct _RowClass
{
  BobguiListBoxRowClass parent_class;
};

const char *css =
  "list row {"
  " border-width: 1px;"
  " border-style: solid;"
  " border-color: blue;"
  "}"
  "list row:hover {"
  "background-color: green;"
  "}"
  "list row:active {"
  "background-color: red;"
  "}";

static GType row_get_type (void);
G_DEFINE_TYPE (Row, row, BOBGUI_TYPE_LIST_BOX_ROW)

static void
row_init (Row *row)
{
}

static void
row_class_init (RowClass *class)
{
}

static BobguiWidget *
row_new (const char * text, int sort_id) {
  Row *row;

  row = g_object_new (row_get_type (), NULL);
  if (text != NULL)
    {
      row->label = bobgui_label_new (text);
      bobgui_list_box_row_set_child (BOBGUI_LIST_BOX_ROW (row), row->label);
    }
  row->sort_id = sort_id;

  return BOBGUI_WIDGET (row);
}


static void
update_header_cb (Row *row, Row *before, gpointer data)
{
  BobguiWidget *hbox, *l, *b;

  if (before == NULL ||
      (row->label != NULL &&
       strcmp (bobgui_label_get_text (BOBGUI_LABEL (row->label)), "blah3") == 0))
    {
      /* Create header if needed */
      if (bobgui_list_box_row_get_header (BOBGUI_LIST_BOX_ROW (row)) == NULL)
        {
          hbox = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 0);
          l = bobgui_label_new ("Header");
          bobgui_box_append (BOBGUI_BOX (hbox), l);
          b = bobgui_button_new_with_label ("button");
          bobgui_box_append (BOBGUI_BOX (hbox), b);
          bobgui_list_box_row_set_header (BOBGUI_LIST_BOX_ROW (row), hbox);
      }

      hbox = bobgui_list_box_row_get_header(BOBGUI_LIST_BOX_ROW (row));

      l = bobgui_widget_get_first_child (hbox);
      bobgui_label_set_text (BOBGUI_LABEL (l), g_strdup_printf ("Header %d", row->sort_id));
    }
  else
    {
      bobgui_list_box_row_set_header(BOBGUI_LIST_BOX_ROW (row), NULL);
    }
}

static int
sort_cb (Row *a, Row *b, gpointer data)
{
  return a->sort_id - b->sort_id;
}

static int
reverse_sort_cb (Row *a, Row *b, gpointer data)
{
  return b->sort_id - a->sort_id;
}

static gboolean
filter_cb (Row *row, gpointer data)
{
  const char *text;

  if (row->label != NULL)
    {
      text = bobgui_label_get_text (BOBGUI_LABEL (row->label));
      return strcmp (text, "blah3") != 0;
    }

  return TRUE;
}

static void
row_activated_cb (BobguiListBox *list_box,
                  BobguiListBoxRow *row)
{
  g_print ("activated row %p\n", row);
}

static void
row_selected_cb (BobguiListBox *list_box,
                 BobguiListBoxRow *row)
{
  g_print ("selected row %p\n", row);
}

static void
sort_clicked_cb (BobguiButton *button,
                 gpointer data)
{
  BobguiListBox *list = data;

  bobgui_list_box_set_sort_func (list, (BobguiListBoxSortFunc)sort_cb, NULL, NULL);
}

static void
reverse_sort_clicked_cb (BobguiButton *button,
                         gpointer data)
{
  BobguiListBox *list = data;

  bobgui_list_box_set_sort_func (list, (BobguiListBoxSortFunc)reverse_sort_cb, NULL, NULL);
}

static void
filter_clicked_cb (BobguiButton *button,
                   gpointer data)
{
  BobguiListBox *list = data;

  bobgui_list_box_set_filter_func (list, (BobguiListBoxFilterFunc)filter_cb, NULL, NULL);
}

static void
unfilter_clicked_cb (BobguiButton *button,
                   gpointer data)
{
  BobguiListBox *list = data;

  bobgui_list_box_set_filter_func (list, NULL, NULL, NULL);
}

static void
change_clicked_cb (BobguiButton *button,
                   gpointer data)
{
  Row *row = data;

  if (strcmp (bobgui_label_get_text (BOBGUI_LABEL (row->label)), "blah3") == 0)
    {
      bobgui_label_set_text (BOBGUI_LABEL (row->label), "blah5");
      row->sort_id = 5;
    }
  else
    {
      bobgui_label_set_text (BOBGUI_LABEL (row->label), "blah3");
      row->sort_id = 3;
    }
  bobgui_list_box_row_changed (BOBGUI_LIST_BOX_ROW (row));
}

static void
add_clicked_cb (BobguiButton *button,
                gpointer data)
{
  BobguiListBox *list = data;
  BobguiWidget *new_row;
  static int new_button_nr = 1;

  new_row = row_new( g_strdup_printf ("blah2 new %d", new_button_nr), new_button_nr);
  bobgui_list_box_insert (BOBGUI_LIST_BOX (list), new_row, -1);
  new_button_nr++;
}

static void
separate_clicked_cb (BobguiButton *button,
                     gpointer data)
{
  BobguiListBox *list = data;

  bobgui_list_box_set_header_func (list, (BobguiListBoxUpdateHeaderFunc)update_header_cb, NULL, NULL);
}

static void
unseparate_clicked_cb (BobguiButton *button,
                       gpointer data)
{
  BobguiListBox *list = data;

  bobgui_list_box_set_header_func (list, NULL, NULL, NULL);
}

static void
visibility_clicked_cb (BobguiButton *button,
                       gpointer data)
{
  BobguiWidget *row = data;

  bobgui_widget_set_visible (row, !bobgui_widget_get_visible (row));
}

static void
selection_mode_changed (BobguiComboBox *combo, gpointer data)
{
  BobguiListBox *list = data;

  bobgui_list_box_set_selection_mode (list, bobgui_combo_box_get_active (combo));
}

static void
single_click_clicked (BobguiButton *check, gpointer data)
{
  BobguiListBox *list = data;

  g_print ("single: %d\n", bobgui_check_button_get_active (BOBGUI_CHECK_BUTTON (check)));
  bobgui_list_box_set_activate_on_single_click (list, bobgui_check_button_get_active (BOBGUI_CHECK_BUTTON (check)));
}

int
main (int argc, char *argv[])
{
  BobguiCssProvider *provider;
  BobguiWidget *window, *hbox, *vbox, *list, *row, *row3, *row_vbox, *row_hbox, *l;
  BobguiWidget *check, *button, *combo, *scrolled;

  bobgui_init ();

  window = bobgui_window_new ();
  hbox = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 0);
  bobgui_window_set_child (BOBGUI_WINDOW (window), hbox);

  provider = bobgui_css_provider_new ();
  bobgui_css_provider_load_from_data (provider, css, -1);
  bobgui_style_context_add_provider_for_display (bobgui_widget_get_display (window),
                                              BOBGUI_STYLE_PROVIDER (provider),
                                              BOBGUI_STYLE_PROVIDER_PRIORITY_USER);


  list = bobgui_list_box_new ();

  vbox = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 0);
  bobgui_box_append (BOBGUI_BOX (hbox), vbox);

  combo = bobgui_combo_box_text_new ();
  bobgui_combo_box_text_append_text (BOBGUI_COMBO_BOX_TEXT (combo),
                                  "BOBGUI_SELECTION_NONE");
  bobgui_combo_box_text_append_text (BOBGUI_COMBO_BOX_TEXT (combo),
                                  "BOBGUI_SELECTION_SINGLE");
  bobgui_combo_box_text_append_text (BOBGUI_COMBO_BOX_TEXT (combo),
                                  "BOBGUI_SELECTION_BROWSE");
  g_signal_connect (combo, "changed", G_CALLBACK (selection_mode_changed), list);
  bobgui_box_append (BOBGUI_BOX (vbox), combo);
  bobgui_combo_box_set_active (BOBGUI_COMBO_BOX (combo), bobgui_list_box_get_selection_mode (BOBGUI_LIST_BOX (list)));
  check = bobgui_check_button_new_with_label ("single click mode");
  bobgui_check_button_set_active (BOBGUI_CHECK_BUTTON (check), bobgui_list_box_get_activate_on_single_click (BOBGUI_LIST_BOX (list)));
  g_signal_connect (check, "toggled", G_CALLBACK (single_click_clicked), list);
  bobgui_box_append (BOBGUI_BOX (vbox), check);

  scrolled = bobgui_scrolled_window_new ();
  bobgui_scrolled_window_set_policy (BOBGUI_SCROLLED_WINDOW (scrolled), BOBGUI_POLICY_NEVER, BOBGUI_POLICY_AUTOMATIC);
  bobgui_scrolled_window_set_child (BOBGUI_SCROLLED_WINDOW (scrolled), list);
  bobgui_box_append (BOBGUI_BOX (hbox), scrolled);

  g_signal_connect (list, "row-activated", G_CALLBACK (row_activated_cb), NULL);
  g_signal_connect (list, "row-selected", G_CALLBACK (row_selected_cb), NULL);

  row = row_new ("blah4", 4);
  bobgui_list_box_insert (BOBGUI_LIST_BOX (list), row, -1);
  row3 = row = row_new ("blah3", 3);
  bobgui_list_box_insert (BOBGUI_LIST_BOX (list), row, -1);
  row = row_new ("blah1", 1);
  bobgui_list_box_insert (BOBGUI_LIST_BOX (list), row, -1);
  row = row_new ("blah2", 2);
  bobgui_list_box_insert (BOBGUI_LIST_BOX (list), row, -1);

  row = row_new (NULL, 0);
  row_vbox = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 0);
  row_hbox = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 0);
  l = bobgui_label_new ("da box for da man");
  bobgui_box_append (BOBGUI_BOX (row_hbox), l);
  check = bobgui_check_button_new ();
  bobgui_box_append (BOBGUI_BOX (row_hbox), check);
  button = bobgui_button_new_with_label ("ya!");
  bobgui_box_append (BOBGUI_BOX (row_hbox), button);
  bobgui_box_append (BOBGUI_BOX (row_vbox), row_hbox);
  check = bobgui_check_button_new ();
  bobgui_box_append (BOBGUI_BOX (row_vbox), check);
  bobgui_list_box_row_set_child (BOBGUI_LIST_BOX_ROW (row), row_vbox);
  bobgui_list_box_insert (BOBGUI_LIST_BOX (list), row, -1);

  row = row_new (NULL, 0);
  button = bobgui_button_new_with_label ("focusable row");
  bobgui_widget_set_hexpand (button, FALSE);
  bobgui_widget_set_halign (button, BOBGUI_ALIGN_START);
  bobgui_list_box_row_set_child (BOBGUI_LIST_BOX_ROW (row), button);
  bobgui_list_box_insert (BOBGUI_LIST_BOX (list), row, -1);

  vbox = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 0);
  bobgui_box_append (BOBGUI_BOX (hbox), vbox);

  button = bobgui_button_new_with_label ("sort");
  bobgui_box_append (BOBGUI_BOX (vbox), button);
  g_signal_connect (button, "clicked", G_CALLBACK (sort_clicked_cb), list);

  button = bobgui_button_new_with_label ("reverse");
  bobgui_box_append (BOBGUI_BOX (vbox), button);
  g_signal_connect (button, "clicked", G_CALLBACK (reverse_sort_clicked_cb), list);

  button = bobgui_button_new_with_label ("change");
  bobgui_box_append (BOBGUI_BOX (vbox), button);
  g_signal_connect (button, "clicked", G_CALLBACK (change_clicked_cb), row3);

  button = bobgui_button_new_with_label ("filter");
  bobgui_box_append (BOBGUI_BOX (vbox), button);
  g_signal_connect (button, "clicked", G_CALLBACK (filter_clicked_cb), list);

  button = bobgui_button_new_with_label ("unfilter");
  bobgui_box_append (BOBGUI_BOX (vbox), button);
  g_signal_connect (button, "clicked", G_CALLBACK (unfilter_clicked_cb), list);

  button = bobgui_button_new_with_label ("add");
  bobgui_box_append (BOBGUI_BOX (vbox), button);
  g_signal_connect (button, "clicked", G_CALLBACK (add_clicked_cb), list);

  button = bobgui_button_new_with_label ("separate");
  bobgui_box_append (BOBGUI_BOX (vbox), button);
  g_signal_connect (button, "clicked", G_CALLBACK (separate_clicked_cb), list);

  button = bobgui_button_new_with_label ("unseparate");
  bobgui_box_append (BOBGUI_BOX (vbox), button);
  g_signal_connect (button, "clicked", G_CALLBACK (unseparate_clicked_cb), list);

  button = bobgui_button_new_with_label ("visibility");
  bobgui_box_append (BOBGUI_BOX (vbox), button);
  g_signal_connect (button, "clicked", G_CALLBACK (visibility_clicked_cb), row3);

  bobgui_window_present (BOBGUI_WINDOW (window));

  while (TRUE)
    g_main_context_iteration (NULL, TRUE);

  return 0;
}
