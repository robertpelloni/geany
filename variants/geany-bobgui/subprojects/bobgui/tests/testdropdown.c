/* simple.c
 * Copyright (C) 2017  Red Hat, Inc
 * Author: Benjamin Otte
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


#define STRING_TYPE_HOLDER (string_holder_get_type ())
G_DECLARE_FINAL_TYPE (StringHolder, string_holder, STRING, HOLDER, GObject)

struct _StringHolder {
  GObject parent_instance;
  char *title;
  char *icon;
  char *description;
};

G_DEFINE_TYPE (StringHolder, string_holder, G_TYPE_OBJECT);

static void
string_holder_init (StringHolder *holder)
{
}

static void
string_holder_finalize (GObject *object)
{
  StringHolder *holder = STRING_HOLDER (object);

  g_free (holder->title);
  g_free (holder->icon);
  g_free (holder->description);

  G_OBJECT_CLASS (string_holder_parent_class)->finalize (object);
}

static void
string_holder_class_init (StringHolderClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  object_class->finalize = string_holder_finalize;
}

static StringHolder *
string_holder_new (const char *title, const char *icon, const char *description)
{
  StringHolder *holder = g_object_new (STRING_TYPE_HOLDER, NULL);
  holder->title = g_strdup (title);
  holder->icon = g_strdup (icon);
  holder->description = g_strdup (description);
  return holder;
}

static void
strings_setup_item_single_line (BobguiSignalListItemFactory *factory,
                                BobguiListItem              *item)
{
  BobguiWidget *box, *image, *title;

  box = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 10);

  image = bobgui_image_new ();
  title = bobgui_label_new ("");
  bobgui_label_set_xalign (BOBGUI_LABEL (title), 0.0);

  bobgui_box_append (BOBGUI_BOX (box), image);
  bobgui_box_append (BOBGUI_BOX (box), title);

  g_object_set_data (G_OBJECT (item), "title", title);
  g_object_set_data (G_OBJECT (item), "image", image);

  bobgui_list_item_set_child (item, box);
}

static void
strings_setup_item_full (BobguiSignalListItemFactory *factory,
                         BobguiListItem              *item)
{
  BobguiWidget *box, *box2, *image, *title, *description;

  image = bobgui_image_new ();
  title = bobgui_label_new ("");
  bobgui_label_set_xalign (BOBGUI_LABEL (title), 0.0);
  description = bobgui_label_new ("");
  bobgui_label_set_xalign (BOBGUI_LABEL (description), 0.0);
  bobgui_widget_add_css_class (description, "dim-label");

  box = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 10);
  box2 = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 2);

  bobgui_box_append (BOBGUI_BOX (box), image);
  bobgui_box_append (BOBGUI_BOX (box), box2);
  bobgui_box_append (BOBGUI_BOX (box2), title);
  bobgui_box_append (BOBGUI_BOX (box2), description);

  g_object_set_data (G_OBJECT (item), "title", title);
  g_object_set_data (G_OBJECT (item), "image", image);
  g_object_set_data (G_OBJECT (item), "description", description);

  bobgui_list_item_set_child (item, box);
}

static void
strings_bind_item (BobguiSignalListItemFactory *factory,
                    BobguiListItem              *item)
{
  BobguiWidget *image, *title, *description;
  StringHolder *holder;

  holder = bobgui_list_item_get_item (item);

  title = g_object_get_data (G_OBJECT (item), "title");
  image = g_object_get_data (G_OBJECT (item), "image");
  description = g_object_get_data (G_OBJECT (item), "description");

  bobgui_label_set_label (BOBGUI_LABEL (title), holder->title);
  if (image)
    {
      bobgui_image_set_from_icon_name (BOBGUI_IMAGE (image), holder->icon);
      bobgui_widget_set_visible (image, holder->icon != NULL);
    }
  if (description)
    {
      bobgui_label_set_label (BOBGUI_LABEL (description), holder->description);
      bobgui_widget_set_visible (description , holder->description != NULL);        
    }
}

static BobguiListItemFactory *
strings_factory_new (gboolean full)
{
  BobguiListItemFactory *factory;

  factory = bobgui_signal_list_item_factory_new ();
  if (full)
    g_signal_connect (factory, "setup", G_CALLBACK (strings_setup_item_full), NULL);
  else
    g_signal_connect (factory, "setup", G_CALLBACK (strings_setup_item_single_line), NULL);
  g_signal_connect (factory, "bind", G_CALLBACK (strings_bind_item), NULL);

  return factory;
}

static GListModel *
strings_model_new (const char *const *titles,
                   const char *const *icons,
                   const char *const *descriptions)
{
  GListStore *store;
  int i;

  store = g_list_store_new (STRING_TYPE_HOLDER);
  for (i = 0; titles[i]; i++)
    {
      StringHolder *holder = string_holder_new (titles[i],
                                                icons ? icons[i] : NULL,
                                                descriptions ? descriptions[i] : NULL);
      g_list_store_append (store, holder);
      g_object_unref (holder);
    }

  return G_LIST_MODEL (store);
}

static BobguiWidget *
drop_down_new_from_strings (const char *const *titles,
                            const char *const *icons,
                            const char *const *descriptions)
{
  BobguiWidget *widget;
  GListModel *model;
  BobguiListItemFactory *factory;
  BobguiListItemFactory *list_factory;

  g_return_val_if_fail (titles != NULL, NULL);
  g_return_val_if_fail (icons == NULL || g_strv_length ((char **)icons) == g_strv_length ((char **)titles), NULL);
  g_return_val_if_fail (descriptions == NULL || g_strv_length ((char **)icons) == g_strv_length ((char **)descriptions), NULL);

  model = strings_model_new (titles, icons, descriptions);
  factory = strings_factory_new (FALSE);
  if (icons != NULL || descriptions != NULL)
    list_factory = strings_factory_new (TRUE);
  else
    list_factory = NULL;

  widget = g_object_new (BOBGUI_TYPE_DROP_DOWN,
                         "model", model,
                         "factory", factory,
                         "list-factory", list_factory,
                         NULL);

  g_object_unref (model);
  g_object_unref (factory);
  if (list_factory)
    g_object_unref (list_factory);

  return widget;
}

static char *
get_family_name (gpointer item)
{
  return g_strdup (pango_font_family_get_name (PANGO_FONT_FAMILY (item)));
}

static char *
get_title (gpointer item)
{
  return g_strdup (STRING_HOLDER (item)->title);
}

static gboolean
quit_cb (BobguiWindow *window,
         gpointer   data)
{
  *((gboolean*)data) = TRUE;

  g_main_context_wakeup (NULL);

  return TRUE;
}

#define BOBGUI_TYPE_STRING_PAIR (bobgui_string_pair_get_type ())
G_DECLARE_FINAL_TYPE (BobguiStringPair, bobgui_string_pair, BOBGUI, STRING_PAIR, GObject)

struct _BobguiStringPair {
  GObject parent_instance;
  char *id;
  char *string;
};

enum {
  PROP_ID = 1,
  PROP_STRING,
  PROP_NUM_PROPERTIES
};

G_DEFINE_TYPE (BobguiStringPair, bobgui_string_pair, G_TYPE_OBJECT);

static void
bobgui_string_pair_init (BobguiStringPair *pair)
{
}

static void
bobgui_string_pair_finalize (GObject *object)
{
  BobguiStringPair *pair = BOBGUI_STRING_PAIR (object);

  g_free (pair->id);
  g_free (pair->string);

  G_OBJECT_CLASS (bobgui_string_pair_parent_class)->finalize (object);
}

static void
bobgui_string_pair_set_property (GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  BobguiStringPair *pair = BOBGUI_STRING_PAIR (object);

  switch (property_id)
    {
    case PROP_STRING:
      g_free (pair->string);
      pair->string = g_value_dup_string (value);
      break;

    case PROP_ID:
      g_free (pair->id);
      pair->id = g_value_dup_string (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
bobgui_string_pair_get_property (GObject      *object,
                              guint         property_id,
                              GValue       *value,
                              GParamSpec   *pspec)
{
  BobguiStringPair *pair = BOBGUI_STRING_PAIR (object);

  switch (property_id)
    {
    case PROP_STRING:
      g_value_set_string (value, pair->string);
      break;

    case PROP_ID:
      g_value_set_string (value, pair->id);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
bobgui_string_pair_class_init (BobguiStringPairClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  GParamSpec *pspec;

  object_class->finalize = bobgui_string_pair_finalize;
  object_class->set_property = bobgui_string_pair_set_property;
  object_class->get_property = bobgui_string_pair_get_property;

  pspec = g_param_spec_string ("string", "String", "String",
                               NULL,
                               G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  g_object_class_install_property (object_class, PROP_STRING, pspec);

  pspec = g_param_spec_string ("id", "ID", "ID",
                               NULL,
                               G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  g_object_class_install_property (object_class, PROP_ID, pspec);
}

static BobguiStringPair *
bobgui_string_pair_new (const char *id,
                     const char *string)
{
  return g_object_new (BOBGUI_TYPE_STRING_PAIR,
                       "id", id,
                       "string", string,
                       NULL);
}

static const char *
bobgui_string_pair_get_string (BobguiStringPair *pair)
{
  return pair->string;
}

static const char *
bobgui_string_pair_get_id (BobguiStringPair *pair)
{
  return pair->id;
}

static void
setup_no_item (BobguiSignalListItemFactory *factory,
               BobguiListItem              *item)
{
}

static void
setup_list_item (BobguiSignalListItemFactory *factory,
                 BobguiListItem              *item)
{
  BobguiWidget *label;

  label = bobgui_label_new ("");
  bobgui_widget_set_halign (label, BOBGUI_ALIGN_START);
  bobgui_list_item_set_child (item, label);
}

static void
bind_list_item (BobguiSignalListItemFactory *factory,
                BobguiListItem              *item)
{
  BobguiStringPair *pair;
  BobguiWidget *label;

  pair = bobgui_list_item_get_item (item);
  label = bobgui_list_item_get_child (item);

  bobgui_label_set_text (BOBGUI_LABEL (label), bobgui_string_pair_get_string (pair));
}

static void
selected_changed (BobguiDropDown *dropdown,
                  GParamSpec *pspec,
                  gpointer data)
{
  GListModel *model;
  guint selected;
  BobguiStringPair *pair;

  model = bobgui_drop_down_get_model (dropdown);
  selected = bobgui_drop_down_get_selected (dropdown);

  pair = g_list_model_get_item (model, selected);

  g_print ("selected %s\n", bobgui_string_pair_get_id (pair));

  g_object_unref (pair);
}

static void
selected_changed2 (BobguiDropDown *dropdown,
                   GParamSpec *pspec,
                   gpointer data)
{
  GListModel *model;
  guint selected;
  BobguiStringPair *pair;
  BobguiWidget *entry = data;

  model = bobgui_drop_down_get_model (dropdown);
  selected = bobgui_drop_down_get_selected (dropdown);

  pair = g_list_model_get_item (model, selected);

  bobgui_editable_set_text (BOBGUI_EDITABLE (entry), bobgui_string_pair_get_string (pair));

  g_object_unref (pair);
}

int
main (int argc, char *argv[])
{
  BobguiWidget *window, *button, *box, *spin, *check;
  GListModel *model;
  BobguiExpression *expression;
  const char * const times[] = { "1 minute", "2 minutes", "5 minutes", "20 minutes", NULL };
  const char * const many_times[] = {
    "1 minute", "2 minutes", "5 minutes", "10 minutes", "15 minutes", "20 minutes",
    "25 minutes", "30 minutes", "35 minutes", "40 minutes", "45 minutes", "50 minutes",
    "55 minutes", "1 hour", "2 hours", "3 hours", "5 hours", "6 hours", "7 hours",
    "8 hours", "9 hours", "10 hours", "11 hours", "12 hours", NULL
  };
  const char * const device_titles[] = { "Digital Output", "Headphones", "Digital Output", "Analog Output", NULL };
  const char * const device_icons[] = {  "audio-card-symbolic", "audio-headphones-symbolic", "audio-card-symbolic", "audio-card-symbolic", NULL };
  const char * const device_descriptions[] = {
    "Built-in Audio", "Built-in audio", "Thinkpad Tunderbolt 3 Dock USB Audio", "Thinkpad Tunderbolt 3 Dock USB Audio", NULL
  };
  gboolean quit = FALSE;
  GListStore *store;
  BobguiListItemFactory *factory;
  BobguiWidget *entry;
  BobguiWidget *hbox;

  bobgui_init ();

  window = bobgui_window_new ();
  bobgui_window_set_title (BOBGUI_WINDOW (window), "hello world");
  bobgui_window_set_resizable (BOBGUI_WINDOW (window), TRUE);
  g_signal_connect (window, "close-request", G_CALLBACK (quit_cb), &quit);

  box = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 10);
  bobgui_widget_set_margin_start (box, 10);
  bobgui_widget_set_margin_end (box, 10);
  bobgui_widget_set_margin_top (box, 10);
  bobgui_widget_set_margin_bottom (box, 10);
  bobgui_window_set_child (BOBGUI_WINDOW (window), box);

  button = bobgui_drop_down_new (NULL, NULL);

  model = G_LIST_MODEL (pango_cairo_font_map_get_default ());
  bobgui_drop_down_set_model (BOBGUI_DROP_DOWN (button), model);
  bobgui_drop_down_set_selected (BOBGUI_DROP_DOWN (button), 0);

  expression = bobgui_cclosure_expression_new (G_TYPE_STRING, NULL,
                                            0, NULL,
                                            (GCallback)get_family_name,
                                            NULL, NULL);
  bobgui_drop_down_set_expression (BOBGUI_DROP_DOWN (button), expression);
  bobgui_expression_unref (expression);
  bobgui_box_append (BOBGUI_BOX (box), button);

  spin = bobgui_spin_button_new_with_range (-1, g_list_model_get_n_items (G_LIST_MODEL (model)), 1);
  bobgui_widget_set_halign (spin, BOBGUI_ALIGN_START);
  g_object_bind_property  (button, "selected", spin, "value", G_BINDING_SYNC_CREATE | G_BINDING_BIDIRECTIONAL);
  bobgui_box_append (BOBGUI_BOX (box), spin);

  check = bobgui_check_button_new_with_label ("Enable search");
  g_object_bind_property  (button, "enable-search", check, "active", G_BINDING_SYNC_CREATE | G_BINDING_BIDIRECTIONAL);
  bobgui_box_append (BOBGUI_BOX (box), check);

  g_object_unref (model);

  button = drop_down_new_from_strings (times, NULL, NULL);
  bobgui_box_append (BOBGUI_BOX (box), button);

  button = drop_down_new_from_strings (many_times, NULL, NULL);
  bobgui_box_append (BOBGUI_BOX (box), button);

  button = drop_down_new_from_strings (many_times, NULL, NULL);
  bobgui_drop_down_set_enable_search (BOBGUI_DROP_DOWN (button), TRUE);
  expression = bobgui_cclosure_expression_new (G_TYPE_STRING, NULL,
                                            0, NULL,
                                            (GCallback)get_title,
                                            NULL, NULL);
  bobgui_drop_down_set_expression (BOBGUI_DROP_DOWN (button), expression);
  bobgui_expression_unref (expression);
  bobgui_box_append (BOBGUI_BOX (box), button);

  button = drop_down_new_from_strings (device_titles, device_icons, device_descriptions);
  bobgui_box_append (BOBGUI_BOX (box), button);

  button = bobgui_drop_down_new (NULL, NULL);

  store = g_list_store_new (BOBGUI_TYPE_STRING_PAIR);
  g_list_store_append (store, bobgui_string_pair_new ("1", "One"));
  g_list_store_append (store, bobgui_string_pair_new ("2", "Two"));
  g_list_store_append (store, bobgui_string_pair_new ("2.5", "Two ½"));
  g_list_store_append (store, bobgui_string_pair_new ("3", "Three"));
  bobgui_drop_down_set_model (BOBGUI_DROP_DOWN (button), G_LIST_MODEL (store));

  factory = bobgui_signal_list_item_factory_new ();
  g_signal_connect (factory, "setup", G_CALLBACK (setup_list_item), NULL);
  g_signal_connect (factory, "bind", G_CALLBACK (bind_list_item), NULL);
  bobgui_drop_down_set_factory (BOBGUI_DROP_DOWN (button), factory);
  g_object_unref (factory);

  g_signal_connect (button, "notify::selected", G_CALLBACK (selected_changed), NULL);

  bobgui_box_append (BOBGUI_BOX (box), button);

  hbox = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 0);
  bobgui_widget_add_css_class (hbox, "linked");

  entry = bobgui_entry_new ();
  button = bobgui_drop_down_new (NULL, NULL);

  bobgui_drop_down_set_model (BOBGUI_DROP_DOWN (button), G_LIST_MODEL (store));

  factory = bobgui_signal_list_item_factory_new ();
  g_signal_connect (factory, "setup", G_CALLBACK (setup_no_item), NULL);
  bobgui_drop_down_set_factory (BOBGUI_DROP_DOWN (button), factory);
  g_object_unref (factory);

  factory = bobgui_signal_list_item_factory_new ();
  g_signal_connect (factory, "setup", G_CALLBACK (setup_list_item), NULL);
  g_signal_connect (factory, "bind", G_CALLBACK (bind_list_item), NULL);
  bobgui_drop_down_set_list_factory (BOBGUI_DROP_DOWN (button), factory);
  g_object_unref (factory);

  g_signal_connect (button, "notify::selected", G_CALLBACK (selected_changed2), entry);

  bobgui_box_append (BOBGUI_BOX (hbox), entry);
  bobgui_box_append (BOBGUI_BOX (hbox), button);

  bobgui_box_append (BOBGUI_BOX (box), hbox);

  g_object_unref (store);

  bobgui_window_present (BOBGUI_WINDOW (window));

  while (!quit)
    g_main_context_iteration (NULL, TRUE);

  return 0;
}
