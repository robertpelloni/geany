/* Entry/Completion
 *
 * BobguiEntryCompletion provides a mechanism for adding support for
 * completion in BobguiEntry.
 *
 */

#include <glib/gi18n.h>
#include <bobgui/bobgui.h>

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

/* Creates a tree model containing the completions */
static BobguiTreeModel *
create_completion_model (void)
{
  const char *strings[] = {
    "GNOME",
    "gnominious",
    "Gnomonic projection",
    "Gnosophy",
    "total",
    "totally",
    "toto",
    "tottery",
    "totterer",
    "Totten trust",
    "Tottenham hotspurs",
    "totipotent",
    "totipotency",
    "totemism",
    "totem pole",
    "Totara",
    "totalizer",
    "totalizator",
    "totalitarianism",
    "total parenteral nutrition",
    "total eclipse",
    "Totipresence",
    "Totipalmi",
    "zombie",
    "aæx",
    "aæy",
    "aæz",
    NULL
  };
  int i;
  BobguiListStore *store;
  BobguiTreeIter iter;

  store = bobgui_list_store_new (1, G_TYPE_STRING);

  for (i = 0; strings[i]; i++)
    {
      /* Append one word */
      bobgui_list_store_append (store, &iter);
      bobgui_list_store_set (store, &iter, 0, strings[i], -1);
    }

  return BOBGUI_TREE_MODEL (store);
}


BobguiWidget *
do_entry_completion (BobguiWidget *do_widget)
{
  static BobguiWidget *window = NULL;
  BobguiWidget *vbox;
  BobguiWidget *label;
  BobguiWidget *entry;
  BobguiEntryCompletion *completion;
  BobguiTreeModel *completion_model;

  if (!window)
    {
      window = bobgui_window_new ();
      bobgui_window_set_display (BOBGUI_WINDOW (window),
                              bobgui_widget_get_display (do_widget));
      bobgui_window_set_title (BOBGUI_WINDOW (window), "Completion");
      bobgui_window_set_resizable (BOBGUI_WINDOW (window), FALSE);
      g_object_add_weak_pointer (G_OBJECT (window), (gpointer *)&window);

      vbox = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 12);
      bobgui_widget_set_margin_start (vbox, 18);
      bobgui_widget_set_margin_end (vbox, 18);
      bobgui_widget_set_margin_top (vbox, 18);
      bobgui_widget_set_margin_bottom (vbox, 18);
      bobgui_window_set_child (BOBGUI_WINDOW (window), vbox);

      label = bobgui_label_new (NULL);
      bobgui_label_set_markup (BOBGUI_LABEL (label), "Try writing <b>total</b> or <b>gnome</b> for example.");
      bobgui_box_append (BOBGUI_BOX (vbox), label);

      /* Create our entry */
      entry = bobgui_entry_new ();
      bobgui_box_append (BOBGUI_BOX (vbox), entry);

      bobgui_accessible_update_relation (BOBGUI_ACCESSIBLE (entry),
                                      BOBGUI_ACCESSIBLE_RELATION_LABELLED_BY, label, NULL,
                                      -1);
      bobgui_accessible_update_property (BOBGUI_ACCESSIBLE (entry),
                                      BOBGUI_ACCESSIBLE_PROPERTY_AUTOCOMPLETE, BOBGUI_ACCESSIBLE_AUTOCOMPLETE_LIST,
                                      -1);

      /* Create the completion object */
      completion = bobgui_entry_completion_new ();

      /* Assign the completion to the entry */
      bobgui_entry_set_completion (BOBGUI_ENTRY (entry), completion);
      g_object_unref (completion);

      /* Create a tree model and use it as the completion model */
      completion_model = create_completion_model ();
      bobgui_entry_completion_set_model (completion, completion_model);
      g_object_unref (completion_model);

      /* Use model column 0 as the text column */
      bobgui_entry_completion_set_text_column (completion, 0);

      bobgui_entry_completion_set_inline_completion (completion, TRUE);
      bobgui_entry_completion_set_inline_selection (completion, TRUE);
    }

  if (!bobgui_widget_get_visible (window))
    bobgui_widget_set_visible (window, TRUE);
  else
    bobgui_window_destroy (BOBGUI_WINDOW (window));

  return window;
}
