/* Combo Boxes
 * #Keywords: BobguiCellRenderer
 *
 * The BobguiComboBox widget allows to select one option out of a list.
 * The BobguiComboBoxEntry additionally allows the user to enter a value
 * that is not in the list of options.
 *
 * How the options are displayed is controlled by cell renderers.
 */

#include <glib/gi18n.h>
#include <bobgui/bobgui.h>

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

enum
{
  ICON_NAME_COL,
  TEXT_COL
};

static BobguiTreeModel *
create_icon_store (void)
{
  const char *icon_names[6] = {
    "dialog-warning",
    "process-stop",
    "document-new",
    "edit-clear",
    NULL,
    "document-open"
  };
  const char *labels[6] = {
    N_("Warning"),
    N_("Stop"),
    N_("New"),
    N_("Clear"),
    NULL,
    N_("Open")
  };

  BobguiTreeIter iter;
  BobguiListStore *store;
  int i;

  store = bobgui_list_store_new (2, G_TYPE_STRING, G_TYPE_STRING);

  for (i = 0; i < G_N_ELEMENTS (icon_names); i++)
    {
      if (icon_names[i])
        {
          bobgui_list_store_append (store, &iter);
          bobgui_list_store_set (store, &iter,
                              ICON_NAME_COL, icon_names[i],
                              TEXT_COL, _(labels[i]),
                              -1);
        }
      else
        {
          bobgui_list_store_append (store, &iter);
          bobgui_list_store_set (store, &iter,
                              ICON_NAME_COL, NULL,
                              TEXT_COL, "separator",
                              -1);
        }
    }

  return BOBGUI_TREE_MODEL (store);
}

/* A BobguiCellLayoutDataFunc that demonstrates how one can control
 * sensitivity of rows. This particular function does nothing
 * useful and just makes the second row insensitive.
 */
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

/* A BobguiTreeViewRowSeparatorFunc that demonstrates how rows can be
 * rendered as separators. This particular function does nothing
 * useful and just turns the fourth row into a separator.
 */
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

static BobguiTreeModel *
create_capital_store (void)
{
  struct {
    const char *group;
    const char *capital;
  } capitals[] = {
    { "A - B", NULL },
    { NULL, "Albany" },
    { NULL, "Annapolis" },
    { NULL, "Atlanta" },
    { NULL, "Augusta" },
    { NULL, "Austin" },
    { NULL, "Baton Rouge" },
    { NULL, "Bismarck" },
    { NULL, "Boise" },
    { NULL, "Boston" },
    { "C - D", NULL },
    { NULL, "Carson City" },
    { NULL, "Charleston" },
    { NULL, "Cheyenne" },
    { NULL, "Columbia" },
    { NULL, "Columbus" },
    { NULL, "Concord" },
    { NULL, "Denver" },
    { NULL, "Des Moines" },
    { NULL, "Dover" },
    { "E - J", NULL },
    { NULL, "Frankfort" },
    { NULL, "Harrisburg" },
    { NULL, "Hartford" },
    { NULL, "Helena" },
    { NULL, "Honolulu" },
    { NULL, "Indianapolis" },
    { NULL, "Jackson" },
    { NULL, "Jefferson City" },
    { NULL, "Juneau" },
    { "K - O", NULL },
    { NULL, "Lansing" },
    { NULL, "Lincoln" },
    { NULL, "Little Rock" },
    { NULL, "Madison" },
    { NULL, "Montgomery" },
    { NULL, "Montpelier" },
    { NULL, "Nashville" },
    { NULL, "Oklahoma City" },
    { NULL, "Olympia" },
    { "P - S", NULL },
    { NULL, "Phoenix" },
    { NULL, "Pierre" },
    { NULL, "Providence" },
    { NULL, "Raleigh" },
    { NULL, "Richmond" },
    { NULL, "Sacramento" },
    { NULL, "Salem" },
    { NULL, "Salt Lake City" },
    { NULL, "Santa Fe" },
    { NULL, "Springfield" },
    { NULL, "St. Paul" },
    { "T - Z", NULL },
    { NULL, "Tallahassee" },
    { NULL, "Topeka" },
    { NULL, "Trenton" },
    { NULL, NULL }
  };

  BobguiTreeIter iter, iter2;
  BobguiTreeStore *store;
  int i;

  store = bobgui_tree_store_new (1, G_TYPE_STRING);

  for (i = 0; capitals[i].group || capitals[i].capital; i++)
    {
      if (capitals[i].group)
        {
          bobgui_tree_store_append (store, &iter, NULL);
          bobgui_tree_store_set (store, &iter, 0, capitals[i].group, -1);
        }
      else if (capitals[i].capital)
        {
          bobgui_tree_store_append (store, &iter2, &iter);
          bobgui_tree_store_set (store, &iter2, 0, capitals[i].capital, -1);
        }
    }

  return BOBGUI_TREE_MODEL (store);
}

static void
is_capital_sensitive (BobguiCellLayout   *cell_layout,
                      BobguiCellRenderer *cell,
                      BobguiTreeModel    *tree_model,
                      BobguiTreeIter     *iter,
                      gpointer         data)
{
  gboolean sensitive;

  sensitive = !bobgui_tree_model_iter_has_child (tree_model, iter);

  g_object_set (cell, "sensitive", sensitive, NULL);
}

static void
fill_combo_entry (BobguiWidget *combo)
{
  bobgui_combo_box_text_append_text (BOBGUI_COMBO_BOX_TEXT (combo), "One");
  bobgui_combo_box_text_append_text (BOBGUI_COMBO_BOX_TEXT (combo), "Two");
  bobgui_combo_box_text_append_text (BOBGUI_COMBO_BOX_TEXT (combo), "2\302\275");
  bobgui_combo_box_text_append_text (BOBGUI_COMBO_BOX_TEXT (combo), "Three");
}


/* A simple validating entry */

#define TYPE_MASK_ENTRY             (mask_entry_get_type ())
#define MASK_ENTRY(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_MASK_ENTRY, MaskEntry))
#define MASK_ENTRY_CLASS(vtable)    (G_TYPE_CHECK_CLASS_CAST ((vtable), TYPE_MASK_ENTRY, MaskEntryClass))
#define IS_MASK_ENTRY(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_MASK_ENTRY))
#define IS_MASK_ENTRY_CLASS(vtable) (G_TYPE_CHECK_CLASS_TYPE ((vtable), TYPE_MASK_ENTRY))
#define MASK_ENTRY_GET_CLASS(inst)  (G_TYPE_INSTANCE_GET_CLASS ((inst), TYPE_MASK_ENTRY, MaskEntryClass))


typedef struct _MaskEntry MaskEntry;
struct _MaskEntry
{
  BobguiEntry entry;
  const char *mask;
};

typedef struct _MaskEntryClass MaskEntryClass;
struct _MaskEntryClass
{
  BobguiEntryClass parent_class;
};


static void mask_entry_editable_init (BobguiEditableInterface *iface);

static GType mask_entry_get_type (void);
G_DEFINE_TYPE_WITH_CODE (MaskEntry, mask_entry, BOBGUI_TYPE_ENTRY,
                         G_IMPLEMENT_INTERFACE (BOBGUI_TYPE_EDITABLE,
                                                mask_entry_editable_init));


static void
mask_entry_set_background (MaskEntry *entry)
{
  if (entry->mask)
    {
      if (!g_regex_match_simple (entry->mask, bobgui_editable_get_text (BOBGUI_EDITABLE (entry)), 0, 0))
        {
          PangoAttrList *attrs;

          attrs = pango_attr_list_new ();
          pango_attr_list_insert (attrs, pango_attr_foreground_new (65535, 32767, 32767));
          bobgui_entry_set_attributes (BOBGUI_ENTRY (entry), attrs);
          pango_attr_list_unref (attrs);
          return;
        }
    }

  bobgui_entry_set_attributes (BOBGUI_ENTRY (entry), NULL);
}


static void
mask_entry_changed (BobguiEditable *editable)
{
  mask_entry_set_background (MASK_ENTRY (editable));
}


static void
mask_entry_init (MaskEntry *entry)
{
  entry->mask = NULL;
}


static void
mask_entry_class_init (MaskEntryClass *klass)
{ }


static void
mask_entry_editable_init (BobguiEditableInterface *iface)
{
  iface->changed = mask_entry_changed;
}


BobguiWidget *
do_combobox (BobguiWidget *do_widget)
{
  static BobguiWidget *window = NULL;
  BobguiWidget *vbox, *frame, *box, *combo, *entry;
  BobguiTreeModel *model;
  BobguiCellRenderer *renderer;
  BobguiTreePath *path;
  BobguiTreeIter iter;

  if (!window)
  {
    window = bobgui_window_new ();
    bobgui_window_set_display (BOBGUI_WINDOW (window),
                            bobgui_widget_get_display (do_widget));
    bobgui_window_set_title (BOBGUI_WINDOW (window), "Combo Boxes");
    g_object_add_weak_pointer (G_OBJECT (window), (gpointer *)&window);

    vbox = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 2);
    bobgui_widget_set_margin_start (vbox, 10);
    bobgui_widget_set_margin_end (vbox, 10);
    bobgui_widget_set_margin_top (vbox, 10);
    bobgui_widget_set_margin_bottom (vbox, 10);
    bobgui_window_set_child (BOBGUI_WINDOW (window), vbox);

    /* A combobox demonstrating cell renderers, separators and
     *  insensitive rows
     */
    frame = bobgui_frame_new ("Items with icons");
    bobgui_box_append (BOBGUI_BOX (vbox), frame);

    box = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 0);
    bobgui_widget_set_margin_start (box, 5);
    bobgui_widget_set_margin_end (box, 5);
    bobgui_widget_set_margin_top (box, 5);
    bobgui_widget_set_margin_bottom (box, 5);
    bobgui_frame_set_child (BOBGUI_FRAME (frame), box);

    model = create_icon_store ();
    combo = bobgui_combo_box_new_with_model (model);
    g_object_unref (model);
    bobgui_box_append (BOBGUI_BOX (box), combo);

    renderer = bobgui_cell_renderer_pixbuf_new ();
    bobgui_cell_layout_pack_start (BOBGUI_CELL_LAYOUT (combo), renderer, FALSE);
    bobgui_cell_layout_set_attributes (BOBGUI_CELL_LAYOUT (combo), renderer,
                                    "icon-name", ICON_NAME_COL,
                                    NULL);

    bobgui_cell_layout_set_cell_data_func (BOBGUI_CELL_LAYOUT (combo),
                                        renderer,
                                        set_sensitive,
                                        NULL, NULL);

    renderer = bobgui_cell_renderer_text_new ();
    bobgui_cell_layout_pack_start (BOBGUI_CELL_LAYOUT (combo), renderer, TRUE);
    bobgui_cell_layout_set_attributes (BOBGUI_CELL_LAYOUT (combo), renderer,
                                    "text", TEXT_COL,
                                    NULL);

    bobgui_cell_layout_set_cell_data_func (BOBGUI_CELL_LAYOUT (combo),
                                        renderer,
                                        set_sensitive,
                                        NULL, NULL);

    bobgui_combo_box_set_row_separator_func (BOBGUI_COMBO_BOX (combo),
                                          is_separator, NULL, NULL);

    bobgui_combo_box_set_active (BOBGUI_COMBO_BOX (combo), 0);

    /* A combobox demonstrating trees.
     */
    frame = bobgui_frame_new ("Where are we ?");
    bobgui_box_append (BOBGUI_BOX (vbox), frame);

    box = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 0);
    bobgui_widget_set_margin_start (box, 5);
    bobgui_widget_set_margin_end (box, 5);
    bobgui_widget_set_margin_top (box, 5);
    bobgui_widget_set_margin_bottom (box, 5);
    bobgui_frame_set_child (BOBGUI_FRAME (frame), box);

    model = create_capital_store ();
    combo = bobgui_combo_box_new_with_model (model);
    g_object_unref (model);
    bobgui_box_append (BOBGUI_BOX (box), combo);

    renderer = bobgui_cell_renderer_text_new ();
    bobgui_cell_layout_pack_start (BOBGUI_CELL_LAYOUT (combo), renderer, TRUE);
    bobgui_cell_layout_set_attributes (BOBGUI_CELL_LAYOUT (combo), renderer,
                                    "text", 0,
                                    NULL);
    bobgui_cell_layout_set_cell_data_func (BOBGUI_CELL_LAYOUT (combo),
                                        renderer,
                                        is_capital_sensitive,
                                        NULL, NULL);

    path = bobgui_tree_path_new_from_indices (0, 8, -1);
    bobgui_tree_model_get_iter (model, &iter, path);
    bobgui_tree_path_free (path);
    bobgui_combo_box_set_active_iter (BOBGUI_COMBO_BOX (combo), &iter);

    /* A BobguiComboBoxEntry with validation */
    frame = bobgui_frame_new ("Editable");
    bobgui_box_append (BOBGUI_BOX (vbox), frame);

    box = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 0);
    bobgui_widget_set_margin_start (box, 5);
    bobgui_widget_set_margin_end (box, 5);
    bobgui_widget_set_margin_top (box, 5);
    bobgui_widget_set_margin_bottom (box, 5);
    bobgui_frame_set_child (BOBGUI_FRAME (frame), box);

    combo = bobgui_combo_box_text_new_with_entry ();
    fill_combo_entry (combo);
    bobgui_box_append (BOBGUI_BOX (box), combo);

    entry = g_object_new (TYPE_MASK_ENTRY, NULL);
    MASK_ENTRY (entry)->mask = "^([0-9]*|One|Two|2\302\275|Three)$";

    bobgui_combo_box_set_child (BOBGUI_COMBO_BOX (combo), entry);

    /* A combobox with string IDs */
    frame = bobgui_frame_new ("String IDs");
    bobgui_box_append (BOBGUI_BOX (vbox), frame);

    box = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 0);
    bobgui_widget_set_margin_start (box, 5);
    bobgui_widget_set_margin_end (box, 5);
    bobgui_widget_set_margin_top (box, 5);
    bobgui_widget_set_margin_bottom (box, 5);
    bobgui_frame_set_child (BOBGUI_FRAME (frame), box);

    combo = bobgui_combo_box_text_new ();
    bobgui_combo_box_text_append (BOBGUI_COMBO_BOX_TEXT (combo), "never", "Not visible");
    bobgui_combo_box_text_append (BOBGUI_COMBO_BOX_TEXT (combo), "when-active", "Visible when active");
    bobgui_combo_box_text_append (BOBGUI_COMBO_BOX_TEXT (combo), "always", "Always visible");
    bobgui_box_append (BOBGUI_BOX (box), combo);

    entry = bobgui_entry_new ();
    g_object_bind_property (combo, "active-id",
                            entry, "text",
                            G_BINDING_BIDIRECTIONAL);
    bobgui_box_append (BOBGUI_BOX (box), entry);
  }

  if (!bobgui_widget_get_visible (window))
    bobgui_widget_set_visible (window, TRUE);
  else
    bobgui_window_destroy (BOBGUI_WINDOW (window));

  return window;
}
