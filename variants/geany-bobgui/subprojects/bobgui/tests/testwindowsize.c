/*  gcc -g -Wall -O2 -o dialog-test dialog-test.c `pkg-config --cflags --libs bobgui4` */
#include <bobgui/bobgui.h>

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

static BobguiWidget *window;
static BobguiWidget *width_chars_spin;
static BobguiWidget *max_width_chars_spin;
static BobguiWidget *default_width_spin;
static BobguiWidget *default_height_spin;
static BobguiWidget *resizable_check;

static gboolean
set_label_idle (gpointer user_data)
{
  BobguiLabel *label = user_data;
  BobguiNative *native = bobgui_widget_get_native (BOBGUI_WIDGET (label));
  GdkSurface *surface = bobgui_native_get_surface (native);
  char *str;

  str = g_strdup_printf ("%d x %d",
                         gdk_surface_get_width (surface),
                         gdk_surface_get_height (surface));
  bobgui_label_set_label (label, str);
  g_free (str);

  return G_SOURCE_REMOVE;
}

static void
layout_cb (GdkSurface *surface, int width, int height, BobguiLabel *label)
{
  g_idle_add (set_label_idle, label);
}

static void
show_dialog (void)
{
  BobguiWidget *dialog;
  BobguiWidget *label;
  int width_chars, max_width_chars, default_width, default_height;
  gboolean resizable;

  width_chars = bobgui_spin_button_get_value_as_int (BOBGUI_SPIN_BUTTON (width_chars_spin));
  max_width_chars = bobgui_spin_button_get_value_as_int (BOBGUI_SPIN_BUTTON (max_width_chars_spin));
  default_width = bobgui_spin_button_get_value_as_int (BOBGUI_SPIN_BUTTON (default_width_spin));
  default_height = bobgui_spin_button_get_value_as_int (BOBGUI_SPIN_BUTTON (default_height_spin));
  resizable = bobgui_check_button_get_active (BOBGUI_CHECK_BUTTON (resizable_check));

  dialog = bobgui_dialog_new_with_buttons ("Test", BOBGUI_WINDOW (window),
                                        BOBGUI_DIALOG_MODAL,
                                        "_Close", BOBGUI_RESPONSE_CANCEL,
                                        NULL);

  label = bobgui_label_new ("Lorem ipsum dolor sit amet, consectetur adipiscing elit. "
                         "Nulla innn urna ac dui malesuada ornare. Nullam dictum "
                         "tempor mi et tincidunt. Aliquam metus nulla, auctor "
                         "vitae pulvinar nec, egestas at mi. Class aptent taciti "
                         "sociosqu ad litora torquent per conubia nostra, per "
                         "inceptos himenaeos. Aliquam sagittis, tellus congue "
                         "cursus congue, diam massa mollis enim, sit amet gravida "
                         "magna turpis egestas sapien. Aenean vel molestie nunc. "
                         "In hac habitasse platea dictumst. Suspendisse lacinia"
                         "mi eu ipsum vestibulum in venenatis enim commodo. "
                         "Vivamus non malesuada ligula.");

  bobgui_label_set_wrap (BOBGUI_LABEL (label), TRUE);
  bobgui_label_set_width_chars (BOBGUI_LABEL (label), width_chars);
  bobgui_label_set_max_width_chars (BOBGUI_LABEL (label), max_width_chars);
  bobgui_window_set_default_size (BOBGUI_WINDOW (dialog), default_width, default_height);
  bobgui_window_set_resizable (BOBGUI_WINDOW (dialog), resizable);


  bobgui_box_append (BOBGUI_BOX (bobgui_dialog_get_content_area (BOBGUI_DIALOG (dialog))),
                      label);

  label = bobgui_label_new ("? x ?");

  bobgui_dialog_add_action_widget (BOBGUI_DIALOG (dialog), label, BOBGUI_RESPONSE_HELP);
  bobgui_widget_realize (dialog);
  g_signal_connect (bobgui_native_get_surface (BOBGUI_NATIVE (dialog)), "layout",
                    G_CALLBACK (layout_cb), label);
  g_signal_connect (dialog, "response",
                    G_CALLBACK (bobgui_window_destroy),
                    NULL);
  bobgui_window_present (BOBGUI_WINDOW (dialog));
}

static void
create_window (void)
{
  BobguiWidget *grid;
  BobguiWidget *label;
  BobguiWidget *button;

  window = bobgui_window_new ();
  bobgui_window_set_title (BOBGUI_WINDOW (window), "Window size");
  bobgui_window_set_resizable (BOBGUI_WINDOW (window), FALSE);

  grid = bobgui_grid_new ();
  bobgui_grid_set_row_spacing (BOBGUI_GRID (grid), 12);
  bobgui_grid_set_column_spacing (BOBGUI_GRID (grid), 12);
  bobgui_window_set_child (BOBGUI_WINDOW (window), grid);

  label = bobgui_label_new ("Width chars");
  bobgui_widget_set_halign (label, BOBGUI_ALIGN_START);
  width_chars_spin = bobgui_spin_button_new_with_range (-1, 1000, 1);
  bobgui_widget_set_halign (width_chars_spin, BOBGUI_ALIGN_START);

  bobgui_grid_attach (BOBGUI_GRID (grid), label, 0, 0, 1, 1);
  bobgui_grid_attach (BOBGUI_GRID (grid), width_chars_spin, 1, 0, 1, 1);

  label = bobgui_label_new ("Max width chars");
  bobgui_widget_set_halign (label, BOBGUI_ALIGN_START);
  max_width_chars_spin = bobgui_spin_button_new_with_range (-1, 1000, 1);
  bobgui_widget_set_halign (width_chars_spin, BOBGUI_ALIGN_START);

  bobgui_grid_attach (BOBGUI_GRID (grid), label, 0, 1, 1, 1);
  bobgui_grid_attach (BOBGUI_GRID (grid), max_width_chars_spin, 1, 1, 1, 1);

  label = bobgui_label_new ("Default size");
  bobgui_widget_set_halign (label, BOBGUI_ALIGN_START);
  default_width_spin = bobgui_spin_button_new_with_range (-1, 1000, 1);
  bobgui_widget_set_halign (default_width_spin, BOBGUI_ALIGN_START);
  default_height_spin = bobgui_spin_button_new_with_range (-1, 1000, 1);
  bobgui_widget_set_halign (default_height_spin, BOBGUI_ALIGN_START);

  bobgui_grid_attach (BOBGUI_GRID (grid), label, 0, 2, 1, 1);
  bobgui_grid_attach (BOBGUI_GRID (grid), default_width_spin, 1, 2, 1, 1);
  bobgui_grid_attach (BOBGUI_GRID (grid), default_height_spin, 2, 2, 1, 1);

  label = bobgui_label_new ("Resizable");
  bobgui_widget_set_halign (label, BOBGUI_ALIGN_START);
  resizable_check = bobgui_check_button_new ();
  bobgui_widget_set_halign (resizable_check, BOBGUI_ALIGN_START);

  bobgui_grid_attach (BOBGUI_GRID (grid), label, 0, 3, 1, 1);
  bobgui_grid_attach (BOBGUI_GRID (grid), resizable_check, 1, 3, 1, 1);

  button = bobgui_button_new_with_label ("Show");
  g_signal_connect (button, "clicked", G_CALLBACK (show_dialog), NULL);
  bobgui_grid_attach (BOBGUI_GRID (grid), button, 2, 4, 1, 1);

  bobgui_window_present (BOBGUI_WINDOW (window));

  GMainLoop *loop = g_main_loop_new (NULL, FALSE);

  g_signal_connect_swapped (window, "destroy",
                            G_CALLBACK (g_main_loop_quit),
                            loop);

  g_main_loop_run (loop);

  g_main_loop_unref (loop);
}

int
main (int argc, char *argv[])
{
  bobgui_init ();

  create_window ();

  return 0;
}
