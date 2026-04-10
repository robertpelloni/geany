/* Overlay/Interactive Overlay
 * #Keywords: BobguiOverlay
 *
 * Shows widgets in static positions over a main widget.
 *
 * The overlaid widgets can be interactive controls such
 * as the entry in this example, or just decorative, like
 * the big blue label.
 */

#include <bobgui/bobgui.h>

static void
do_number (BobguiButton *button, BobguiEntry *entry)
{
  bobgui_editable_set_text (BOBGUI_EDITABLE (entry), bobgui_button_get_label (button));
}

BobguiWidget *
do_overlay (BobguiWidget *do_widget)
{
  static BobguiWidget *window = NULL;

  if (!window)
    {
      BobguiWidget *overlay;
      BobguiWidget *grid;
      BobguiWidget *button;
      BobguiWidget *vbox;
      BobguiWidget *label;
      BobguiWidget *entry;
      int i, j;
      char *text;

      window = bobgui_window_new ();
      bobgui_window_set_default_size (BOBGUI_WINDOW (window), 500, 510);
      bobgui_window_set_title (BOBGUI_WINDOW (window), "Interactive Overlay");

      overlay = bobgui_overlay_new ();
      grid = bobgui_grid_new ();
      bobgui_overlay_set_child (BOBGUI_OVERLAY (overlay), grid);

      entry = bobgui_entry_new ();

      for (j = 0; j < 5; j++)
        {
          for (i = 0; i < 5; i++)
            {
              text = g_strdup_printf ("%d", 5*j + i);
              button = bobgui_button_new_with_label (text);
              g_free (text);
              bobgui_widget_set_hexpand (button, TRUE);
              bobgui_widget_set_vexpand (button, TRUE);
              g_signal_connect (button, "clicked", G_CALLBACK (do_number), entry);
              bobgui_grid_attach (BOBGUI_GRID (grid), button, i, j, 1, 1);
            }
        }

      vbox = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 10);
      bobgui_widget_set_can_target (vbox, FALSE);
      bobgui_overlay_add_overlay (BOBGUI_OVERLAY (overlay), vbox);
      bobgui_widget_set_halign (vbox, BOBGUI_ALIGN_CENTER);
      bobgui_widget_set_valign (vbox, BOBGUI_ALIGN_START);

      label = bobgui_label_new ("<span foreground='blue' weight='ultrabold' font='40'>Numbers</span>");
      bobgui_label_set_use_markup (BOBGUI_LABEL (label), TRUE);
      bobgui_widget_set_can_target (label, FALSE);
      bobgui_widget_set_margin_top (label, 8);
      bobgui_widget_set_margin_bottom (label, 8);
      bobgui_box_append (BOBGUI_BOX (vbox), label);

      vbox = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 10);
      bobgui_overlay_add_overlay (BOBGUI_OVERLAY (overlay), vbox);
      bobgui_widget_set_halign (vbox, BOBGUI_ALIGN_CENTER);
      bobgui_widget_set_valign (vbox, BOBGUI_ALIGN_CENTER);

      bobgui_entry_set_placeholder_text (BOBGUI_ENTRY (entry), "Your Lucky Number");
      bobgui_widget_set_margin_top (entry, 8);
      bobgui_widget_set_margin_bottom (entry, 8);
      bobgui_box_append (BOBGUI_BOX (vbox), entry);

      bobgui_window_set_child (BOBGUI_WINDOW (window), overlay);
      g_object_add_weak_pointer (G_OBJECT (window), (gpointer *)&window);
    }

  if (!bobgui_widget_get_visible (window))
    bobgui_widget_set_visible (window, TRUE);
  else
    bobgui_window_destroy (BOBGUI_WINDOW (window));

  return window;
}
