/* Size Groups
 *
 * BobguiSizeGroup provides a mechanism for grouping a number of
 * widgets together so they all request the same amount of space.
 * This is typically useful when you want a column of widgets to
 * have the same size, but you can't use a BobguiTable widget.
 *
 * Note that size groups only affect the amount of space requested,
 * not the size that the widgets finally receive. If you want the
 * widgets in a BobguiSizeGroup to actually be the same size, you need
 * to pack them in such a way that they get the size they request
 * and not more. For example, if you are packing your widgets
 * into a table, you would not include the BOBGUI_FILL flag.
 */

#include <glib/gi18n.h>
#include <bobgui/bobgui.h>

static void
add_row (BobguiGrid      *table,
         int           row,
         BobguiSizeGroup *size_group,
         const char   *label_text,
         const char  **options)
{
  BobguiWidget *dropdown;
  BobguiWidget *label;

  label = bobgui_label_new_with_mnemonic (label_text);
  bobgui_widget_set_halign (label, BOBGUI_ALIGN_START);
  bobgui_widget_set_valign (label, BOBGUI_ALIGN_BASELINE_FILL);
  bobgui_widget_set_hexpand (label, TRUE);
  bobgui_grid_attach (table, label, 0, row, 1, 1);

  dropdown = bobgui_drop_down_new_from_strings (options);
  bobgui_label_set_mnemonic_widget (BOBGUI_LABEL (label), dropdown);
  bobgui_widget_set_halign (dropdown, BOBGUI_ALIGN_END);
  bobgui_widget_set_valign (dropdown, BOBGUI_ALIGN_BASELINE_FILL);
  bobgui_size_group_add_widget (size_group, dropdown);
  bobgui_grid_attach (table, dropdown, 1, row, 1, 1);
}

static void
toggle_grouping (BobguiCheckButton *check_button,
                 BobguiSizeGroup   *size_group)
{
  BobguiSizeGroupMode new_mode;

  /* BOBGUI_SIZE_GROUP_NONE is not generally useful, but is useful
   * here to show the effect of BOBGUI_SIZE_GROUP_HORIZONTAL by
   * contrast.
   */
  if (bobgui_check_button_get_active (check_button))
    new_mode = BOBGUI_SIZE_GROUP_HORIZONTAL;
  else
    new_mode = BOBGUI_SIZE_GROUP_NONE;

  bobgui_size_group_set_mode (size_group, new_mode);
}

BobguiWidget *
do_sizegroup (BobguiWidget *do_widget)
{
  static BobguiWidget *window = NULL;
  BobguiWidget *table;
  BobguiWidget *frame;
  BobguiWidget *vbox;
  BobguiWidget *check_button;
  BobguiSizeGroup *size_group;

  static const char *color_options[] = {
    "Red", "Green", "Blue", NULL
  };

  static const char *dash_options[] = {
    "Solid", "Dashed", "Dotted", NULL
  };

  static const char *end_options[] = {
    "Square", "Round", "Double Arrow", NULL
  };

  if (!window)
    {
      window = bobgui_window_new ();
      bobgui_window_set_display (BOBGUI_WINDOW (window),  bobgui_widget_get_display (do_widget));
      bobgui_window_set_title (BOBGUI_WINDOW (window), "Size Groups");
      bobgui_window_set_resizable (BOBGUI_WINDOW (window), FALSE);
      g_object_add_weak_pointer (G_OBJECT (window), (gpointer *)&window);

      vbox = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 5);
      bobgui_widget_set_margin_start (vbox, 5);
      bobgui_widget_set_margin_end (vbox, 5);
      bobgui_widget_set_margin_top (vbox, 5);
      bobgui_widget_set_margin_bottom (vbox, 5);
      bobgui_window_set_child (BOBGUI_WINDOW (window), vbox);

      size_group = bobgui_size_group_new (BOBGUI_SIZE_GROUP_HORIZONTAL);
      g_object_set_data_full (G_OBJECT (window), "size-group", size_group, g_object_unref);

      /* Create one frame holding color options */
      frame = bobgui_frame_new ("Color Options");
      bobgui_box_append (BOBGUI_BOX (vbox), frame);

      table = bobgui_grid_new ();
      bobgui_widget_set_margin_start (table, 5);
      bobgui_widget_set_margin_end (table, 5);
      bobgui_widget_set_margin_top (table, 5);
      bobgui_widget_set_margin_bottom (table, 5);
      bobgui_grid_set_row_spacing (BOBGUI_GRID (table), 5);
      bobgui_grid_set_column_spacing (BOBGUI_GRID (table), 10);
      bobgui_frame_set_child (BOBGUI_FRAME (frame), table);

      add_row (BOBGUI_GRID (table), 0, size_group, "_Foreground", color_options);
      add_row (BOBGUI_GRID (table), 1, size_group, "_Background", color_options);

      /* And another frame holding line style options */
      frame = bobgui_frame_new ("Line Options");
      bobgui_box_append (BOBGUI_BOX (vbox), frame);

      table = bobgui_grid_new ();
      bobgui_widget_set_margin_start (table, 5);
      bobgui_widget_set_margin_end (table, 5);
      bobgui_widget_set_margin_top (table, 5);
      bobgui_widget_set_margin_bottom (table, 5);
      bobgui_grid_set_row_spacing (BOBGUI_GRID (table), 5);
      bobgui_grid_set_column_spacing (BOBGUI_GRID (table), 10);
      bobgui_frame_set_child (BOBGUI_FRAME (frame), table);

      add_row (BOBGUI_GRID (table), 0, size_group, "_Dashing", dash_options);
      add_row (BOBGUI_GRID (table), 1, size_group, "_Line ends", end_options);

      /* And a check button to turn grouping on and off */
      check_button = bobgui_check_button_new_with_mnemonic ("_Enable grouping");
      bobgui_box_append (BOBGUI_BOX (vbox), check_button);

      bobgui_check_button_set_active (BOBGUI_CHECK_BUTTON (check_button), TRUE);
      g_signal_connect (check_button, "toggled",
                        G_CALLBACK (toggle_grouping), size_group);
    }

  if (!bobgui_widget_get_visible (window))
    bobgui_widget_set_visible (window, TRUE);
  else
    bobgui_window_destroy (BOBGUI_WINDOW (window));

  return window;
}
