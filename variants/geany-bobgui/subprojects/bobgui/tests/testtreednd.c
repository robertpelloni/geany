#include <bobgui/bobgui.h>

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

typedef BobguiListStore MyModel;
typedef BobguiListStoreClass MyModelClass;

static void my_model_drag_source_init (BobguiTreeDragSourceIface *iface);

static GType my_model_get_type (void);
G_DEFINE_TYPE_WITH_CODE (MyModel, my_model, BOBGUI_TYPE_LIST_STORE,
                         G_IMPLEMENT_INTERFACE (BOBGUI_TYPE_TREE_DRAG_SOURCE,
                                                my_model_drag_source_init))

static void
my_model_class_init (MyModelClass *class)
{
}

static void
my_model_init (MyModel *object)
{
  GType types[1] = { G_TYPE_STRING };

  bobgui_list_store_set_column_types (BOBGUI_LIST_STORE (object), G_N_ELEMENTS (types), types);
}

static GdkContentProvider *
my_model_drag_data_get (BobguiTreeDragSource *source,
                        BobguiTreePath       *path)
{
  GdkContentProvider *content;
  BobguiTreeIter iter;
  char *text;

  bobgui_tree_model_get_iter (BOBGUI_TREE_MODEL (source), &iter, path);
  bobgui_tree_model_get (BOBGUI_TREE_MODEL (source), &iter, 0, &text, -1);
  content = gdk_content_provider_new_typed (G_TYPE_STRING, text);
  g_free (text);

  return content;
}

static void
my_model_drag_source_init (BobguiTreeDragSourceIface *iface)
{
  static BobguiTreeDragSourceIface *parent;

  parent = g_type_interface_peek_parent (iface);

  iface->row_draggable = parent->row_draggable;
  iface->drag_data_delete = parent->drag_data_delete;
  iface->drag_data_get = my_model_drag_data_get;
}

static BobguiTreeModel *
get_model (void)
{
  MyModel *model;

  model = g_object_new (my_model_get_type (), NULL);
  bobgui_list_store_insert_with_values (BOBGUI_LIST_STORE (model), NULL, -1, 0, "Item 1", -1);
  bobgui_list_store_insert_with_values (BOBGUI_LIST_STORE (model), NULL, -1, 0, "Item 2", -1);
  bobgui_list_store_insert_with_values (BOBGUI_LIST_STORE (model), NULL, -1, 0, "Item 3", -1);

  return BOBGUI_TREE_MODEL (model);
}

static BobguiWidget *
get_dragsource (void)
{
  BobguiTreeView *tv;
  BobguiCellRenderer *renderer;
  BobguiTreeViewColumn *column;
  GdkContentFormats *targets;

  tv = (BobguiTreeView*) bobgui_tree_view_new ();
  renderer = bobgui_cell_renderer_text_new ();
  column = bobgui_tree_view_column_new_with_attributes ("Text", renderer, "text", 0, NULL);
  bobgui_tree_view_append_column (tv, column);

  bobgui_tree_view_set_model (tv, get_model ());
  targets = gdk_content_formats_new_for_gtype (G_TYPE_STRING);
  bobgui_tree_view_enable_model_drag_source (tv, GDK_BUTTON1_MASK, targets, GDK_ACTION_COPY);
  gdk_content_formats_unref (targets);

  return BOBGUI_WIDGET (tv);
}

static void
drag_drop (BobguiDropTarget *dest,
           const GValue  *value,
           int            x,
           int            y,
           gpointer       dada)
{
  BobguiWidget *widget = bobgui_event_controller_get_widget (BOBGUI_EVENT_CONTROLLER (dest));

  bobgui_label_set_label (BOBGUI_LABEL (widget), g_value_get_string (value));
}

static BobguiWidget *
get_droptarget (void)
{
  BobguiWidget *label;
  BobguiDropTarget *dest;

  label = bobgui_label_new ("Drop here");
  dest = bobgui_drop_target_new (G_TYPE_STRING, GDK_ACTION_COPY);
  g_signal_connect (dest, "drop", G_CALLBACK (drag_drop), NULL);
  bobgui_widget_add_controller (label, BOBGUI_EVENT_CONTROLLER (dest));

  return label;
}

int
main (int argc, char *argv[])
{
  BobguiWidget *window;
  BobguiWidget *box;

  bobgui_init ();

  window = bobgui_window_new ();

  box = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 0);
  bobgui_window_set_child (BOBGUI_WINDOW (window), box);
  bobgui_box_append (BOBGUI_BOX (box), get_dragsource ());
  bobgui_box_append (BOBGUI_BOX (box), get_droptarget ());

  bobgui_window_present (BOBGUI_WINDOW (window));

  while (TRUE)
    g_main_context_iteration (NULL, TRUE);

  return 0;
}
