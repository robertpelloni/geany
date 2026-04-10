/* Entry/Tagged Entry
 *
 * This example shows how to build a complex composite
 * entry using BobguiText, outside of BOBGUI.
 *
 * This tagged entry can display tags and other widgets
 * inside the entry area.
 */

#include <glib/gi18n.h>
#include <bobgui/bobgui.h>
#include "demotaggedentry.h"

static BobguiWidget *spinner = NULL;

static void
closed_cb (DemoTaggedEntryTag *tag, DemoTaggedEntry *entry)
{
  demo_tagged_entry_remove_tag (entry, BOBGUI_WIDGET (tag));
}

static void
add_tag (BobguiButton *button, DemoTaggedEntry *entry)
{
  DemoTaggedEntryTag *tag;

  tag = demo_tagged_entry_tag_new ("Blue");
  bobgui_widget_add_css_class (BOBGUI_WIDGET (tag), "blue");
  demo_tagged_entry_tag_set_has_close_button (tag, TRUE);
  g_signal_connect (tag, "button-clicked", G_CALLBACK (closed_cb), entry);

  if (spinner == NULL)
    demo_tagged_entry_add_tag (entry, BOBGUI_WIDGET (tag));
  else
    demo_tagged_entry_insert_tag_after (entry, BOBGUI_WIDGET (tag), bobgui_widget_get_prev_sibling (spinner));
}

static void
toggle_spinner (BobguiCheckButton *button, DemoTaggedEntry *entry)
{
  if (spinner)
    {
      demo_tagged_entry_remove_tag (entry, spinner);
      spinner = NULL; 
    }
  else
    {
      spinner = bobgui_spinner_new ();
      bobgui_spinner_start (BOBGUI_SPINNER (spinner));
      demo_tagged_entry_add_tag (entry, spinner);
    }
}

BobguiWidget *
do_tagged_entry (BobguiWidget *do_widget)
{
  static BobguiWidget *window = NULL;
  BobguiWidget *box;
  BobguiWidget *box2;
  BobguiWidget *entry;
  BobguiWidget *button;

  if (!window)
    {
      window = bobgui_window_new ();
      bobgui_window_set_display (BOBGUI_WINDOW (window),
                              bobgui_widget_get_display (do_widget));
      bobgui_window_set_title (BOBGUI_WINDOW (window), "Tagged Entry");
      bobgui_window_set_default_size (BOBGUI_WINDOW (window), 260, -1);
      bobgui_window_set_resizable (BOBGUI_WINDOW (window), FALSE);

      g_object_add_weak_pointer (G_OBJECT (window), (gpointer *)&window);

      box = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 6);
      bobgui_widget_set_margin_start (box, 18);
      bobgui_widget_set_margin_end (box, 18);
      bobgui_widget_set_margin_top (box, 18);
      bobgui_widget_set_margin_bottom (box, 18);
      bobgui_window_set_child (BOBGUI_WINDOW (window), box);

      entry = demo_tagged_entry_new ();
      bobgui_box_append (BOBGUI_BOX (box), entry);

      box2 = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 6);
      bobgui_widget_set_halign (box2, BOBGUI_ALIGN_END);
      bobgui_box_append (BOBGUI_BOX (box), box2);

      button = bobgui_button_new_with_mnemonic ("Add _Tag");
      g_signal_connect (button, "clicked", G_CALLBACK (add_tag), entry);
      bobgui_box_append (BOBGUI_BOX (box2), button);

      button = bobgui_check_button_new_with_mnemonic ("_Spinner");
      g_signal_connect (button, "toggled", G_CALLBACK (toggle_spinner), entry);
      bobgui_box_append (BOBGUI_BOX (box2), button);
    }

  if (!bobgui_widget_get_visible (window))
    bobgui_widget_set_visible (window, TRUE);
  else
    bobgui_window_destroy (BOBGUI_WINDOW (window));

  return window;
}
