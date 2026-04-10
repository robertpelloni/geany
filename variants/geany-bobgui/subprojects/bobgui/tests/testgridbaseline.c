#include <bobgui/bobgui.h>



int
main (int argc, char *argv[])
{
  BobguiWidget *window;
  BobguiWidget *grid;
  BobguiWidget *label1;
  BobguiWidget *label2;
  BobguiWidget *label3;
  BobguiWidget *label4;

  g_setenv ("BOBGUI_DEBUG", "baselines,layout", TRUE);
  bobgui_init ();

  window = bobgui_window_new ();
  grid = bobgui_grid_new ();
  bobgui_grid_set_row_spacing (BOBGUI_GRID (grid), 30);
  bobgui_grid_set_column_spacing (BOBGUI_GRID (grid), 30);
  bobgui_window_set_child (BOBGUI_WINDOW (window), grid);

  label1 = bobgui_label_new ("Some Text");
  label2 = bobgui_label_new ("QQQQQQQQQ");
  label3 = bobgui_label_new ("Some Text");
  label4 = bobgui_label_new ("Some Text");

  g_message ("label1: %p", label1);
  g_message ("label2: %p", label2);
  g_message ("label3: %p", label3);
  g_message ("label4: %p", label4);

  bobgui_widget_set_valign (label1, BOBGUI_ALIGN_BASELINE_FILL);
  bobgui_widget_set_valign (label2, BOBGUI_ALIGN_BASELINE_FILL);
  bobgui_widget_set_valign (label3, BOBGUI_ALIGN_START);
  bobgui_widget_set_valign (label4, BOBGUI_ALIGN_START);

  bobgui_widget_set_margin_top (label1, 12);
  bobgui_widget_set_margin_bottom (label2, 18);
  bobgui_widget_set_margin_top (label3, 30);


  /*
   * Since none of the widgets in the second row request baseline alignment,
   * BobguiGrid should not compute or apply a baseline for those widgets.
   */


  bobgui_grid_attach (BOBGUI_GRID (grid), label1, 0, 0, 1, 1);
  bobgui_grid_attach (BOBGUI_GRID (grid), label2, 1, 0, 1, 1);
  bobgui_grid_attach (BOBGUI_GRID (grid), label3, 0, 1, 1, 1);
  bobgui_grid_attach (BOBGUI_GRID (grid), label4, 1, 1, 1, 1);

  bobgui_window_present (BOBGUI_WINDOW (window));
  while (TRUE)
    g_main_context_iteration (NULL, TRUE);
  return 0;
}
