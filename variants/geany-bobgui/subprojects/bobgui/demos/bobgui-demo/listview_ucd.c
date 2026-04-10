/* Lists/Characters
 *
 * This demo shows a multi-column representation of some parts
 * of the Unicode Character Database, or UCD. It also demonstrates
 * the use of sections with headings to group items.
 *
 * The dataset used here has 33 796 items.
 */

#include <bobgui/bobgui.h>
#include "script-names.h"
#include "unicode-names.h"

#define UCD_TYPE_ITEM (ucd_item_get_type ())
G_DECLARE_FINAL_TYPE (UcdItem, ucd_item, UCD, ITEM, GObject)

struct _UcdItem
{
  GObject parent_instance;
  gunichar codepoint;
  const char *name;
  GUnicodeScript script;
};

struct _UcdItemClass
{
  GObjectClass parent_class;
};

enum
{
  PROP_CODEPOINT = 1,
  PROP_NAME,
  PROP_SCRIPT,
  NUM_PROPERTIES,
};

G_DEFINE_TYPE (UcdItem, ucd_item, G_TYPE_OBJECT)

static void
ucd_item_init (UcdItem *item)
{
}

static void
ucd_item_get_property (GObject    *object,
                       guint       prop_id,
                       GValue     *value,
                       GParamSpec *pspec)
{
  UcdItem *item = UCD_ITEM (object);

  switch (prop_id)
    {
    case PROP_CODEPOINT:
      g_value_set_uint (value, item->codepoint);
      break;

    case PROP_NAME:
      g_value_set_string (value, item->name);
      break;

    case PROP_SCRIPT:
      g_value_set_uint (value, item->script);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
ucd_item_class_init (UcdItemClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  object_class->get_property = ucd_item_get_property;

  g_object_class_install_property (object_class,
                                   PROP_CODEPOINT,
                                   g_param_spec_uint ("codepoint", NULL, NULL,
                                                      0, G_MAXUINT, 0,
                                                      G_PARAM_READABLE));

  g_object_class_install_property (object_class,
                                   PROP_NAME,
                                   g_param_spec_string ("name", NULL, NULL,
                                                        NULL,
                                                        G_PARAM_READABLE));

  g_object_class_install_property (object_class,
                                   PROP_SCRIPT,
                                   g_param_spec_uint ("script", NULL, NULL,
                                                      0, G_MAXUINT, 0,
                                                      G_PARAM_READABLE));
}

static UcdItem *
ucd_item_new (gunichar    codepoint,
              const char *name)
{
  UcdItem *item;

  item = g_object_new (UCD_TYPE_ITEM, NULL);

  item->codepoint = codepoint;
  item->name = name;
  item->script = g_unichar_get_script (codepoint);

  return item;
}

static gunichar
ucd_item_get_codepoint (UcdItem *item)
{
  return item->codepoint;
}

static const char *
ucd_item_get_name (UcdItem *item)
{
  return item->name;
}

static GUnicodeScript
ucd_item_get_script (UcdItem *item)
{
  return item->script;
}

static GListModel *
ucd_model_new (void)
{
  GBytes *bytes;
  GVariant *v;
  GVariantIter *iter;
  GListStore *store;
  guint u;
  char *name;
  BobguiExpression *expression;
  BobguiNumericSorter *sorter;
  BobguiSortListModel *sort;

  bytes = g_resources_lookup_data ("/listview_ucd_data/ucdnames.data", 0, NULL);
  v = g_variant_ref_sink (g_variant_new_from_bytes (G_VARIANT_TYPE ("a(us)"), bytes, TRUE));

  iter = g_variant_iter_new (v);

  store = g_list_store_new (G_TYPE_OBJECT);
  while (g_variant_iter_next (iter, "(u&s)", &u, &name))
    {
      if (u == 0)
        continue;

      UcdItem *item = ucd_item_new (u, name);
      g_list_store_append (store, item);
      g_object_unref (item);
    }

  g_variant_iter_free (iter);
  g_variant_unref (v);
  g_bytes_unref (bytes);

  expression = bobgui_property_expression_new (ucd_item_get_type (),
                                            NULL,
                                            "codepoint");
  sorter = bobgui_numeric_sorter_new (expression);
  sort = bobgui_sort_list_model_new (G_LIST_MODEL (store), BOBGUI_SORTER (sorter));

  expression = bobgui_property_expression_new (ucd_item_get_type (),
                                            NULL,
                                            "script");
  sorter = bobgui_numeric_sorter_new (expression);
  bobgui_sort_list_model_set_section_sorter (sort, BOBGUI_SORTER (sorter));
  g_object_unref (sorter);

  return G_LIST_MODEL (sort);
}

static void
setup_centered_label (BobguiSignalListItemFactory *factory,
                      GObject                  *listitem)
{
  BobguiWidget *label;
  label = bobgui_inscription_new ("");
  bobgui_list_item_set_child (BOBGUI_LIST_ITEM (listitem), label);
}

static void
setup_label (BobguiSignalListItemFactory *factory,
             GObject                  *listitem)
{
  BobguiWidget *label;
  label = bobgui_inscription_new ("");
  bobgui_inscription_set_xalign (BOBGUI_INSCRIPTION (label), 0);
  bobgui_list_item_set_child (BOBGUI_LIST_ITEM (listitem), label);
}

static void
setup_ellipsizing_label (BobguiSignalListItemFactory *factory,
                         GObject                  *listitem)
{
  BobguiWidget *label;
  label = bobgui_inscription_new ("");
  bobgui_inscription_set_xalign (BOBGUI_INSCRIPTION (label), 0);
  bobgui_inscription_set_text_overflow (BOBGUI_INSCRIPTION (label), BOBGUI_INSCRIPTION_OVERFLOW_ELLIPSIZE_END);
  bobgui_inscription_set_nat_chars (BOBGUI_INSCRIPTION (label), 20);
  bobgui_list_item_set_child (BOBGUI_LIST_ITEM (listitem), label);
}

static void
bind_codepoint (BobguiSignalListItemFactory *factory,
                GObject                  *listitem)
{
  BobguiWidget *label;
  GObject *item;
  gunichar codepoint;
  char buffer[16] = { 0, };

  label = bobgui_list_item_get_child (BOBGUI_LIST_ITEM (listitem));
  item = bobgui_list_item_get_item (BOBGUI_LIST_ITEM (listitem));
  codepoint = ucd_item_get_codepoint (UCD_ITEM (item));

  g_snprintf (buffer, 10, "%#06x", codepoint);
  bobgui_inscription_set_text (BOBGUI_INSCRIPTION (label), buffer);
}

static void
bind_char (BobguiSignalListItemFactory *factory,
           GObject                  *listitem)
{
  BobguiWidget *label;
  GObject *item;
  gunichar codepoint;
  char buffer[16] = { 0, };

  label = bobgui_list_item_get_child (BOBGUI_LIST_ITEM (listitem));
  item = bobgui_list_item_get_item (BOBGUI_LIST_ITEM (listitem));
  codepoint = ucd_item_get_codepoint (UCD_ITEM (item));

  if (g_unichar_isprint (codepoint))
    g_unichar_to_utf8 (codepoint, buffer);

  bobgui_inscription_set_text (BOBGUI_INSCRIPTION (label), buffer);
}

static void
bind_name (BobguiSignalListItemFactory *factory,
           GObject                  *listitem)
{
  BobguiWidget *label;
  GObject *item;
  const char *name;

  label = bobgui_list_item_get_child (BOBGUI_LIST_ITEM (listitem));
  item = bobgui_list_item_get_item (BOBGUI_LIST_ITEM (listitem));
  name = ucd_item_get_name (UCD_ITEM (item));

  bobgui_inscription_set_text (BOBGUI_INSCRIPTION (label), name);
}

static void
bind_type (BobguiSignalListItemFactory *factory,
           GObject                  *listitem)
{
  BobguiWidget *label;
  GObject *item;
  gunichar codepoint;

  label = bobgui_list_item_get_child (BOBGUI_LIST_ITEM (listitem));
  item = bobgui_list_item_get_item (BOBGUI_LIST_ITEM (listitem));
  codepoint = ucd_item_get_codepoint (UCD_ITEM (item));

  bobgui_inscription_set_text (BOBGUI_INSCRIPTION (label), get_unicode_type_name (g_unichar_type (codepoint)));
}

static void
bind_break_type (BobguiSignalListItemFactory *factory,
                 GObject                  *listitem)
{
  BobguiWidget *label;
  GObject *item;
  gunichar codepoint;

  label = bobgui_list_item_get_child (BOBGUI_LIST_ITEM (listitem));
  item = bobgui_list_item_get_item (BOBGUI_LIST_ITEM (listitem));
  codepoint = ucd_item_get_codepoint (UCD_ITEM (item));

  bobgui_inscription_set_text (BOBGUI_INSCRIPTION (label), get_break_type_name (g_unichar_break_type (codepoint)));
}

static void
bind_combining_class (BobguiSignalListItemFactory *factory,
                      GObject                  *listitem)
{
  BobguiWidget *label;
  GObject *item;
  gunichar codepoint;

  label = bobgui_list_item_get_child (BOBGUI_LIST_ITEM (listitem));
  item = bobgui_list_item_get_item (BOBGUI_LIST_ITEM (listitem));
  codepoint = ucd_item_get_codepoint (UCD_ITEM (item));

  bobgui_inscription_set_text (BOBGUI_INSCRIPTION (label), get_combining_class_name (g_unichar_combining_class (codepoint)));
}

static void
setup_header (BobguiSignalListItemFactory *factory,
              GObject                  *listitem)
{
  BobguiWidget *label;

  label = bobgui_inscription_new ("");
  bobgui_widget_add_css_class (label, "heading");
  bobgui_widget_set_margin_start (label, 20);
  bobgui_widget_set_margin_end (label, 20);
  bobgui_widget_set_margin_top (label, 10);
  bobgui_widget_set_margin_bottom (label, 10);
  bobgui_inscription_set_xalign (BOBGUI_INSCRIPTION (label), 0);
  bobgui_list_header_set_child (BOBGUI_LIST_HEADER (listitem), label);
}

static void
bind_header (BobguiSignalListItemFactory *factory,
             GObject                  *listitem)
{
  BobguiWidget *label;
  GObject *item;
  GUnicodeScript script;

  label = bobgui_list_header_get_child (BOBGUI_LIST_HEADER (listitem));
  item = bobgui_list_header_get_item (BOBGUI_LIST_HEADER (listitem));
  script = ucd_item_get_script (UCD_ITEM (item));

  bobgui_inscription_set_text (BOBGUI_INSCRIPTION (label), get_script_name (script));
}

static void
selection_changed (GObject    *object,
                   GParamSpec *pspec,
                   BobguiWidget  *label)
{
  UcdItem *item;
  guint codepoint;
  char buffer[16] = { 0, };

  item = bobgui_single_selection_get_selected_item (BOBGUI_SINGLE_SELECTION (object));
  codepoint = ucd_item_get_codepoint (item);

  if (g_unichar_isprint (codepoint))
    g_unichar_to_utf8 (codepoint, buffer);

  bobgui_label_set_text (BOBGUI_LABEL (label), buffer);
}

BobguiWidget *
create_ucd_view (BobguiWidget *label)
{
  BobguiWidget *cv;
  GListModel *ucd_model;
  BobguiSingleSelection *selection;
  BobguiListItemFactory *factory;
  BobguiColumnViewColumn *column;

  ucd_model = ucd_model_new ();

  selection = bobgui_single_selection_new (ucd_model);
  bobgui_single_selection_set_autoselect (selection, TRUE);
  bobgui_single_selection_set_can_unselect (selection, FALSE);
  if (label)
    g_signal_connect (selection, "notify::selected", G_CALLBACK (selection_changed), label);

  cv = bobgui_column_view_new (BOBGUI_SELECTION_MODEL (selection));
  bobgui_column_view_set_show_column_separators (BOBGUI_COLUMN_VIEW (cv), TRUE);

  factory = bobgui_signal_list_item_factory_new ();
  g_signal_connect (factory, "setup", G_CALLBACK (setup_centered_label), NULL);
  g_signal_connect (factory, "bind", G_CALLBACK (bind_codepoint), NULL);
  column = bobgui_column_view_column_new ("Codepoint", factory);
  bobgui_column_view_append_column (BOBGUI_COLUMN_VIEW (cv), column);
  g_object_unref (column);

  factory = bobgui_signal_list_item_factory_new ();
  g_signal_connect (factory, "setup", G_CALLBACK (setup_centered_label), NULL);
  g_signal_connect (factory, "bind", G_CALLBACK (bind_char), NULL);
  column = bobgui_column_view_column_new ("Char", factory);
  bobgui_column_view_append_column (BOBGUI_COLUMN_VIEW (cv), column);
  g_object_unref (column);

  factory = bobgui_signal_list_item_factory_new ();
  g_signal_connect (factory, "setup", G_CALLBACK (setup_ellipsizing_label), NULL);
  g_signal_connect (factory, "bind", G_CALLBACK (bind_name), NULL);
  column = bobgui_column_view_column_new ("Name", factory);
  bobgui_column_view_column_set_resizable (column, TRUE);
  bobgui_column_view_append_column (BOBGUI_COLUMN_VIEW (cv), column);
  g_object_unref (column);

  factory = bobgui_signal_list_item_factory_new ();
  g_signal_connect (factory, "setup", G_CALLBACK (setup_ellipsizing_label), NULL);
  g_signal_connect (factory, "bind", G_CALLBACK (bind_type), NULL);
  column = bobgui_column_view_column_new ("Type", factory);
  bobgui_column_view_column_set_resizable (column, TRUE);
  bobgui_column_view_append_column (BOBGUI_COLUMN_VIEW (cv), column);
  g_object_unref (column);

  factory = bobgui_signal_list_item_factory_new ();
  g_signal_connect (factory, "setup", G_CALLBACK (setup_ellipsizing_label), NULL);
  g_signal_connect (factory, "bind", G_CALLBACK (bind_break_type), NULL);
  column = bobgui_column_view_column_new ("Break Type", factory);
  bobgui_column_view_column_set_resizable (column, TRUE);
  bobgui_column_view_append_column (BOBGUI_COLUMN_VIEW (cv), column);
  g_object_unref (column);

  factory = bobgui_signal_list_item_factory_new ();
  g_signal_connect (factory, "setup", G_CALLBACK (setup_label), NULL);
  g_signal_connect (factory, "bind", G_CALLBACK (bind_combining_class), NULL);
  column = bobgui_column_view_column_new ("Combining Class", factory);
  bobgui_column_view_column_set_resizable (column, TRUE);
  bobgui_column_view_append_column (BOBGUI_COLUMN_VIEW (cv), column);
  g_object_unref (column);

  factory = bobgui_signal_list_item_factory_new ();
  g_signal_connect (factory, "setup", G_CALLBACK (setup_header), NULL);
  g_signal_connect (factory, "bind", G_CALLBACK (bind_header), NULL);
  bobgui_column_view_set_header_factory (BOBGUI_COLUMN_VIEW (cv), factory);
  g_object_unref (factory);

  return cv;
}

static BobguiWidget *window;

static void
remove_provider (gpointer data)
{
  BobguiStyleProvider *provider = BOBGUI_STYLE_PROVIDER (data);

  bobgui_style_context_remove_provider_for_display (gdk_display_get_default (), provider);
  g_object_unref (provider);
}

BobguiWidget *
do_listview_ucd (BobguiWidget *do_widget)
{
  if (window == NULL)
    {
      BobguiWidget *listview, *sw;
      BobguiWidget *box, *label;
      BobguiCssProvider *provider;

      window = bobgui_window_new ();
      bobgui_window_set_default_size (BOBGUI_WINDOW (window), 800, 400);
      bobgui_window_set_title (BOBGUI_WINDOW (window), "Characters");
      bobgui_window_set_display (BOBGUI_WINDOW (window),
                              bobgui_widget_get_display (do_widget));
      g_object_add_weak_pointer (G_OBJECT (window), (gpointer *) &window);

      box = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 0);
      label = bobgui_label_new ("");
      bobgui_label_set_width_chars (BOBGUI_LABEL (label), 2);
      bobgui_widget_add_css_class (label, "enormous");
      provider = bobgui_css_provider_new ();
      bobgui_css_provider_load_from_string (provider, "label.enormous { font-size: 80px; }");
      bobgui_style_context_add_provider_for_display (gdk_display_get_default (), BOBGUI_STYLE_PROVIDER (provider), 800);
      bobgui_widget_set_hexpand (label, TRUE);
      bobgui_box_append (BOBGUI_BOX (box), label);

      sw = bobgui_scrolled_window_new ();
      bobgui_scrolled_window_set_propagate_natural_width (BOBGUI_SCROLLED_WINDOW (sw), TRUE);
      listview = create_ucd_view (label);
      bobgui_scrolled_window_set_child (BOBGUI_SCROLLED_WINDOW (sw), listview);
      bobgui_box_prepend (BOBGUI_BOX (box), sw);
      bobgui_window_set_child (BOBGUI_WINDOW (window), box);

      g_object_set_data_full (G_OBJECT (window), "provider", provider, remove_provider);
    }

  if (!bobgui_widget_get_visible (window))
    bobgui_widget_set_visible (window, TRUE);
  else
    bobgui_window_destroy (BOBGUI_WINDOW (window));

  return window;
}

