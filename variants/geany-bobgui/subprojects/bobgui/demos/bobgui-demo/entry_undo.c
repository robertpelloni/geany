/* Entry/Undo and Redo
 *
 * BobguiEntry can provide basic Undo/Redo support using standard keyboard
 * accelerators such as Control+z to undo and Control+Shift+z to redo.
 * Additionally, Control+y can be used to redo.
 *
 * Use bobgui_entry_set_enable_undo() to enable undo/redo support.
 */

#include <glib/gi18n.h>
#include <bobgui/bobgui.h>

BobguiWidget *
do_entry_undo (BobguiWidget *do_widget)
{
  static BobguiWidget *window = NULL;
  BobguiWidget *vbox;
  BobguiWidget *label;
  BobguiWidget *entry;

  if (!window)
    {
      window = bobgui_window_new ();
      bobgui_window_set_display (BOBGUI_WINDOW (window),
                              bobgui_widget_get_display (do_widget));
      bobgui_window_set_title (BOBGUI_WINDOW (window), "Undo and Redo");
      bobgui_window_set_resizable (BOBGUI_WINDOW (window), FALSE);
      g_object_add_weak_pointer (G_OBJECT (window), (gpointer *)&window);

      vbox = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 12);
      bobgui_widget_set_margin_start (vbox, 18);
      bobgui_widget_set_margin_end (vbox, 18);
      bobgui_widget_set_margin_top (vbox, 18);
      bobgui_widget_set_margin_bottom (vbox, 18);
      bobgui_window_set_child (BOBGUI_WINDOW (window), vbox);

      label = bobgui_label_new (NULL);
      bobgui_label_set_markup (BOBGUI_LABEL (label),
                            "Use Control+z or Control+Shift+z to undo or redo changes");
      bobgui_box_append (BOBGUI_BOX (vbox), label);

      /* Create our entry */
      entry = bobgui_entry_new ();
      bobgui_editable_set_enable_undo (BOBGUI_EDITABLE (entry), TRUE);
      bobgui_box_append (BOBGUI_BOX (vbox), entry);

      bobgui_accessible_update_relation (BOBGUI_ACCESSIBLE (entry),
                                      BOBGUI_ACCESSIBLE_RELATION_LABELLED_BY, label, NULL,
                                      -1);
    }

  if (!bobgui_widget_get_visible (window))
    bobgui_widget_set_visible (window, TRUE);
  else
    bobgui_window_destroy (BOBGUI_WINDOW (window));

  return window;
}
