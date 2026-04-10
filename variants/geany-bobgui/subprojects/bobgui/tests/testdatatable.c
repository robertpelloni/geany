/* -*- mode: C; c-basic-offset: 2; indent-tabs-mode: nil; -*- */

#include <bobgui/bobgui.h>

#include "frame-stats.h"


static gboolean no_auto_scroll = FALSE;
static gint n_columns = 20;
static double scroll_pages = 0;


/* This is our dummy item for the model. */
#define DATA_TABLE_TYPE_ITEM (data_table_item_get_type ())
G_DECLARE_FINAL_TYPE (DataTableItem, data_table_item, DATA_TABLE, ITEM, GObject)

struct _DataTableItem
{
  GObject parent_instance;
  int data;
};

struct _DataTableItemClass
{
  GObjectClass parent_class;
};

G_DEFINE_TYPE (DataTableItem, data_table_item, G_TYPE_OBJECT)

static void data_table_item_init (DataTableItem *item) {}

static void data_table_item_class_init (DataTableItemClass *class) {}

static DataTableItem *
data_table_item_new (int data)
{
  DataTableItem *item = g_object_new (DATA_TABLE_TYPE_ITEM, NULL);
  item->data = data;
  return item;
}


static void
set_adjustment_to_fraction (BobguiAdjustment *adjustment,
                            double         fraction)
{
  double upper = bobgui_adjustment_get_upper (adjustment);
  double lower = bobgui_adjustment_get_lower (adjustment);
  double page_size = bobgui_adjustment_get_page_size (adjustment);

  bobgui_adjustment_set_value (adjustment,
                            (1 - fraction) * lower +
                            fraction * (upper - page_size));
}

static void
move_adjustment_by_pages (BobguiAdjustment *adjustment,
                          double         n_pages)
{
  double page_size = bobgui_adjustment_get_page_size (adjustment);
  double value = bobgui_adjustment_get_value (adjustment);

  value += page_size * n_pages;
  /* the adjustment will clamp properly */
  bobgui_adjustment_set_value (adjustment, value);
}

static gboolean
scroll_column_view (BobguiWidget     *column_view,
                    GdkFrameClock *frame_clock,
                    gpointer       user_data)
{
  BobguiAdjustment *vadjustment;

  vadjustment = bobgui_scrollable_get_vadjustment (BOBGUI_SCROLLABLE (column_view));

  if (scroll_pages == 0.0)
    set_adjustment_to_fraction (vadjustment, g_random_double ());
  else
    move_adjustment_by_pages (vadjustment, (g_random_double () * 2 - 1) * scroll_pages);

  return TRUE;
}

enum WidgetType
{
  WIDGET_TYPE_NONE,
  WIDGET_TYPE_LABEL,
  WIDGET_TYPE_TEXT,
  WIDGET_TYPE_INSCRIPTION,
};

static enum WidgetType widget_type = WIDGET_TYPE_INSCRIPTION;

static void
setup (BobguiSignalListItemFactory *factory,
       GObject                  *listitem)
{
  BobguiWidget *widget;

  switch (widget_type)
    {
    case WIDGET_TYPE_NONE:
      /* It's actually a box, just to request size similar to labels. */
      widget = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 0);
      bobgui_widget_set_size_request (widget, 50, 18);
      break;
    case WIDGET_TYPE_LABEL:
      widget = bobgui_label_new ("");
      break;
    case WIDGET_TYPE_TEXT:
      widget = bobgui_text_new ();
      break;
    case WIDGET_TYPE_INSCRIPTION:
      widget = bobgui_inscription_new ("");
      bobgui_inscription_set_min_chars (BOBGUI_INSCRIPTION (widget), 6);
      break;
    default:
      g_assert_not_reached ();
    }

  bobgui_list_item_set_child (BOBGUI_LIST_ITEM (listitem), widget);
}

static void
bind (BobguiSignalListItemFactory *factory,
      GObject                  *listitem,
      gpointer                  name)
{
  BobguiWidget *widget;
  GObject *item;

  widget = bobgui_list_item_get_child (BOBGUI_LIST_ITEM (listitem));
  item = bobgui_list_item_get_item (BOBGUI_LIST_ITEM (listitem));

  char buffer[16] = { 0, };
  g_snprintf (buffer,
              sizeof (buffer),
              "%c%d",
              GPOINTER_TO_INT (name),
              DATA_TABLE_ITEM (item)->data);

  switch (widget_type)
    {
    case WIDGET_TYPE_NONE:
      break;
    case WIDGET_TYPE_LABEL:
      bobgui_label_set_label (BOBGUI_LABEL (widget), buffer);
      break;
    case WIDGET_TYPE_TEXT:
      bobgui_editable_set_text (BOBGUI_EDITABLE (widget), buffer);
      break;
    case WIDGET_TYPE_INSCRIPTION:
      bobgui_inscription_set_text (BOBGUI_INSCRIPTION (widget), buffer);
      break;
    default:
      g_assert_not_reached ();
    }
}

static gboolean
parse_widget_arg (const gchar* option_name,
                  const gchar* value,
                  gpointer data,
                  GError** error)
{
  if (!g_strcmp0 (value, "none"))
    {
      widget_type = WIDGET_TYPE_NONE;
      return TRUE;
    }
  else if (!g_strcmp0 (value, "label"))
    {
      widget_type = WIDGET_TYPE_LABEL;
      return TRUE;
    }
  else if (!g_strcmp0 (value, "text"))
    {
      widget_type = WIDGET_TYPE_TEXT;
      return TRUE;
    }
  else if (!g_strcmp0 (value, "inscription"))
    {
      widget_type = WIDGET_TYPE_INSCRIPTION;
      return TRUE;
    }
  else
    {
      g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_INVALID_DATA,
                           "Invalid option value");
      return FALSE;
    }
}

static GOptionEntry options[] = {
  {
    "widget",
    'w',
    G_OPTION_FLAG_NONE,
    G_OPTION_ARG_CALLBACK,
    parse_widget_arg,
    "Cell item widget to use, can be one of: none, label, text, inscription",
    "WIDGET"
  },
  {
    "no-auto-scroll",
    'n',
    G_OPTION_FLAG_NONE,
    G_OPTION_ARG_NONE,
    &no_auto_scroll,
    "Disable automatic scrolling",
    NULL
  },
  {
    "columns",
    'c',
    G_OPTION_FLAG_NONE,
    G_OPTION_ARG_INT,
    &n_columns,
    "Column count",
    "COUNT"
  },
  {
    "pages",
    'p',
    G_OPTION_FLAG_NONE,
    G_OPTION_ARG_DOUBLE,
    &scroll_pages,
    "Maximum number of pages to scroll (or 0 for random)",
    "COUNT"
  },
  { NULL }
};

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
  GListStore *store;
  int i;
  BobguiMultiSelection *multi_selection;
  BobguiWidget *column_view;
  GError *error = NULL;
  gboolean done = FALSE;

  GOptionContext *context = g_option_context_new (NULL);
  g_option_context_add_main_entries (context, options, NULL);
  frame_stats_add_options (g_option_context_get_main_group (context));

  if (!g_option_context_parse (context, &argc, &argv, &error))
    {
      g_printerr ("Option parsing failed: %s\n", error->message);
      return 1;
    }

  bobgui_init ();

  window = bobgui_window_new ();
  frame_stats_ensure (BOBGUI_WINDOW (window));
  bobgui_window_set_default_size (BOBGUI_WINDOW (window), 1700, 900);

  scrolled_window = bobgui_scrolled_window_new ();
  bobgui_window_set_child (BOBGUI_WINDOW (window), scrolled_window);

  store = g_list_store_new (DATA_TABLE_TYPE_ITEM);
  for (i = 0; i < 10000; ++i)
    {
      DataTableItem *item = data_table_item_new (i);
      g_list_store_append (store, item);
      g_object_unref (item);
    }

  multi_selection = bobgui_multi_selection_new (G_LIST_MODEL (store));
  column_view = bobgui_column_view_new (BOBGUI_SELECTION_MODEL (multi_selection));

  bobgui_column_view_set_show_column_separators (BOBGUI_COLUMN_VIEW (column_view), TRUE);
  bobgui_column_view_set_show_row_separators (BOBGUI_COLUMN_VIEW (column_view), TRUE);
  bobgui_widget_add_css_class (column_view, "data-table");

  for (i = 0; i < MIN (n_columns, 127 - 65); ++i)
    {
      const char name[] = { 'A' + i, '\0' };

      BobguiListItemFactory *factory = bobgui_signal_list_item_factory_new ();
      g_signal_connect (factory, "setup", G_CALLBACK (setup), NULL);
      g_signal_connect (factory, "bind", G_CALLBACK (bind), GINT_TO_POINTER (name[0]));

      bobgui_column_view_append_column (BOBGUI_COLUMN_VIEW (column_view),
                                     bobgui_column_view_column_new (name, factory));
    }

  bobgui_scrolled_window_set_child (BOBGUI_SCROLLED_WINDOW (scrolled_window),
                                 column_view);

  if (!no_auto_scroll)
    {
      bobgui_widget_add_tick_callback (column_view,
                                    scroll_column_view,
                                    NULL,
                                    NULL);
    }

  bobgui_window_present (BOBGUI_WINDOW (window));
  g_signal_connect (window, "destroy",
                    G_CALLBACK (quit_cb), &done);

  while (!done)
    g_main_context_iteration (NULL, TRUE);

  return 0;
}
