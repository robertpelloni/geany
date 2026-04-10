#include <bobgui/bobgui.h>

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

static BobguiWidget *antialias;
static BobguiWidget *subpixel;
static BobguiWidget *hintstyle;

static void
set_font_options (BobguiWidget *label)
{
  cairo_antialias_t aa;
  cairo_subpixel_order_t sp;
  cairo_hint_style_t hs;
  cairo_font_options_t *options;

  aa = bobgui_combo_box_get_active (BOBGUI_COMBO_BOX (antialias));
  sp = bobgui_combo_box_get_active (BOBGUI_COMBO_BOX (subpixel));
  hs = bobgui_combo_box_get_active (BOBGUI_COMBO_BOX (hintstyle));

  options = cairo_font_options_create ();
  cairo_font_options_set_antialias (options, aa);
  cairo_font_options_set_subpixel_order (options, sp);
  cairo_font_options_set_hint_style (options, hs);

  bobgui_widget_set_font_options (label, options);
  cairo_font_options_destroy (options);

  bobgui_widget_queue_draw (label);
}

int
main (int argc, char *argv[])
{
  BobguiWidget *window, *label, *grid, *demo;

  bobgui_init ();

  window = bobgui_window_new ();
  grid = bobgui_grid_new ();
  bobgui_grid_set_row_spacing (BOBGUI_GRID (grid), 10);
  bobgui_grid_set_column_spacing (BOBGUI_GRID (grid), 10);
  bobgui_window_set_child (BOBGUI_WINDOW (window), grid);
  label = bobgui_label_new ("Default font options");
  bobgui_grid_attach (BOBGUI_GRID (grid), label, 0, 0, 2, 1);
  demo = bobgui_label_new ("Custom font options");
  bobgui_grid_attach (BOBGUI_GRID (grid), demo, 0, 1, 2, 1);

  antialias = bobgui_combo_box_text_new ();
  bobgui_combo_box_text_append_text (BOBGUI_COMBO_BOX_TEXT (antialias), "Default");
  bobgui_combo_box_text_append_text (BOBGUI_COMBO_BOX_TEXT (antialias), "None");
  bobgui_combo_box_text_append_text (BOBGUI_COMBO_BOX_TEXT (antialias), "Gray");
  bobgui_combo_box_text_append_text (BOBGUI_COMBO_BOX_TEXT (antialias), "Subpixel");
  bobgui_combo_box_text_append_text (BOBGUI_COMBO_BOX_TEXT (antialias), "Fast");
  bobgui_combo_box_text_append_text (BOBGUI_COMBO_BOX_TEXT (antialias), "Good");
  bobgui_combo_box_text_append_text (BOBGUI_COMBO_BOX_TEXT (antialias), "Best");
  g_signal_connect_swapped (antialias, "changed", G_CALLBACK (set_font_options), demo);
  label = bobgui_label_new ("Antialias");
  bobgui_grid_attach (BOBGUI_GRID (grid), label, 0, 2, 1, 1);
  bobgui_grid_attach (BOBGUI_GRID (grid), antialias, 1, 2, 1, 1);

  subpixel = bobgui_combo_box_text_new ();
  bobgui_combo_box_text_append_text (BOBGUI_COMBO_BOX_TEXT (subpixel), "Default");
  bobgui_combo_box_text_append_text (BOBGUI_COMBO_BOX_TEXT (subpixel), "RGB");
  bobgui_combo_box_text_append_text (BOBGUI_COMBO_BOX_TEXT (subpixel), "BGR");
  bobgui_combo_box_text_append_text (BOBGUI_COMBO_BOX_TEXT (subpixel), "Vertical RGB");
  bobgui_combo_box_text_append_text (BOBGUI_COMBO_BOX_TEXT (subpixel), "Vertical BGR");
  g_signal_connect_swapped (subpixel, "changed", G_CALLBACK (set_font_options), demo);
  label = bobgui_label_new ("Subpixel");
  bobgui_grid_attach (BOBGUI_GRID (grid), label, 0, 3, 1, 1);
  bobgui_grid_attach (BOBGUI_GRID (grid), subpixel, 1, 3, 1, 1);

  hintstyle = bobgui_combo_box_text_new ();
  bobgui_combo_box_text_append_text (BOBGUI_COMBO_BOX_TEXT (hintstyle), "Default");
  bobgui_combo_box_text_append_text (BOBGUI_COMBO_BOX_TEXT (hintstyle), "None");
  bobgui_combo_box_text_append_text (BOBGUI_COMBO_BOX_TEXT (hintstyle), "Slight");
  bobgui_combo_box_text_append_text (BOBGUI_COMBO_BOX_TEXT (hintstyle), "Medium");
  bobgui_combo_box_text_append_text (BOBGUI_COMBO_BOX_TEXT (hintstyle), "Full");
  g_signal_connect_swapped (hintstyle, "changed", G_CALLBACK (set_font_options), demo);
  label = bobgui_label_new ("Hintstyle");
  bobgui_grid_attach (BOBGUI_GRID (grid), label, 0, 4, 1, 1);
  bobgui_grid_attach (BOBGUI_GRID (grid), hintstyle, 1, 4, 1, 1);

  bobgui_combo_box_set_active (BOBGUI_COMBO_BOX (antialias), 0);
  bobgui_combo_box_set_active (BOBGUI_COMBO_BOX (subpixel), 0);
  bobgui_combo_box_set_active (BOBGUI_COMBO_BOX (hintstyle), 0);

  bobgui_window_present (BOBGUI_WINDOW (window));

  while (TRUE)
    g_main_context_iteration (NULL, TRUE);

  return 0;
}
