#include <bobgui/bobgui.h>

static gboolean
overlay_get_child_position (BobguiOverlay *overlay,
                            BobguiWidget *child,
                            GdkRectangle *allocation,
                            gpointer user_data)
{
  BobguiWidget *custom_child = user_data;
  BobguiRequisition req;

  if (child != custom_child)
    return FALSE;

  bobgui_widget_get_preferred_size (child, NULL, &req);

  allocation->x = 120;
  allocation->y = 0;
  allocation->width = req.width;
  allocation->height = req.height;

  return TRUE;
}

int
main (int argc, char *argv[])
{
  BobguiWidget *win, *overlay, *grid, *main_child, *child, *label, *sw;
  BobguiCssProvider *provider;
  char *str;

  bobgui_init ();

  provider = bobgui_css_provider_new ();
  bobgui_css_provider_load_from_string (provider,
    "label { border: 3px solid black; border-radius: 5px; padding: 2px; }"
    ".top { border-top-style: none; border-top-right-radius: 0px; border-top-left-radius: 0px; }"
    ".bottom { border-bottom-style: none; border-bottom-right-radius: 0px; border-bottom-left-radius: 0px; }"
    ".left { border-left-style: none; border-top-left-radius: 0px; border-bottom-left-radius: 0px; }"
    ".right { border-right-style: none; border-top-right-radius: 0px; border-bottom-right-radius: 0px; }");
  bobgui_style_context_add_provider_for_display (gdk_display_get_default (),
                                              BOBGUI_STYLE_PROVIDER (provider),
                                              BOBGUI_STYLE_PROVIDER_PRIORITY_APPLICATION);

  win = bobgui_window_new ();
  bobgui_window_set_default_size (BOBGUI_WINDOW (win), 600, 600);

  grid = bobgui_grid_new ();
  label = bobgui_label_new ("Out of overlay");
  bobgui_widget_set_hexpand (label, TRUE);
  bobgui_widget_set_vexpand (label, TRUE);
  bobgui_grid_attach (BOBGUI_GRID (grid), label, 0, 0, 1, 1);

  overlay = bobgui_overlay_new ();
  sw = bobgui_scrolled_window_new ();
  bobgui_scrolled_window_set_policy (BOBGUI_SCROLLED_WINDOW (sw),
                                  BOBGUI_POLICY_ALWAYS,
                                  BOBGUI_POLICY_ALWAYS);
  bobgui_overlay_set_child (BOBGUI_OVERLAY (overlay), sw);

  main_child = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 0);
  bobgui_scrolled_window_set_child (BOBGUI_SCROLLED_WINDOW (sw), main_child);
  bobgui_widget_set_hexpand (main_child, TRUE);
  bobgui_widget_set_vexpand (main_child, TRUE);
  label = bobgui_label_new ("Main child");
  bobgui_widget_set_halign (label, BOBGUI_ALIGN_CENTER);
  bobgui_widget_set_valign (label, BOBGUI_ALIGN_CENTER);
  bobgui_box_append (BOBGUI_BOX (main_child), label);

  child = bobgui_label_new (NULL);
  str = g_strdup_printf ("%p", child);
  bobgui_label_set_text (BOBGUI_LABEL (child), str);
  g_free (str);
  g_print ("Bottom/Right child: %p\n", child);
  bobgui_widget_set_halign (child, BOBGUI_ALIGN_END);
  bobgui_widget_set_valign (child, BOBGUI_ALIGN_END);
  bobgui_overlay_add_overlay (BOBGUI_OVERLAY (overlay), child);

  child = bobgui_label_new (NULL);
  str = g_strdup_printf ("%p", child);
  bobgui_label_set_text (BOBGUI_LABEL (child), str);
  g_free (str);
  g_print ("Left/Top child: %p\n", child);
  bobgui_widget_set_halign (child, BOBGUI_ALIGN_START);
  bobgui_widget_set_valign (child, BOBGUI_ALIGN_START);
  bobgui_overlay_add_overlay (BOBGUI_OVERLAY (overlay), child);

  child = bobgui_label_new (NULL);
  str = g_strdup_printf ("%p", child);
  bobgui_label_set_text (BOBGUI_LABEL (child), str);
  g_free (str);
  g_print ("Right/Center child: %p\n", child);
  bobgui_widget_set_halign (child, BOBGUI_ALIGN_END);
  bobgui_widget_set_valign (child, BOBGUI_ALIGN_CENTER);
  bobgui_overlay_add_overlay (BOBGUI_OVERLAY (overlay), child);

  child = bobgui_label_new (NULL);
  str = g_strdup_printf ("%p", child);
  bobgui_label_set_text (BOBGUI_LABEL (child), str);
  g_free (str);
  bobgui_widget_set_margin_start (child, 55);
  bobgui_widget_set_margin_top (child, 4);
  g_print ("Left/Top margined child: %p\n", child);
  bobgui_widget_set_halign (child, BOBGUI_ALIGN_START);
  bobgui_widget_set_valign (child, BOBGUI_ALIGN_START);
  bobgui_overlay_add_overlay (BOBGUI_OVERLAY (overlay), child);

  child = bobgui_label_new (NULL);
  str = g_strdup_printf ("%p", child);
  bobgui_label_set_text (BOBGUI_LABEL (child), str);
  g_free (str);
  g_print ("Custom get-child-position child: %p\n", child);
  bobgui_widget_set_halign (child, BOBGUI_ALIGN_START);
  bobgui_widget_set_valign (child, BOBGUI_ALIGN_START);
  bobgui_overlay_add_overlay (BOBGUI_OVERLAY (overlay), child);

  g_signal_connect (overlay, "get-child-position",
                    G_CALLBACK (overlay_get_child_position), child);

  bobgui_grid_attach (BOBGUI_GRID (grid), overlay, 1, 0, 1, 3);
  bobgui_window_set_child (BOBGUI_WINDOW (win), grid);

  g_print ("\n");

  bobgui_window_present (BOBGUI_WINDOW (win));

  while (TRUE)
    g_main_context_iteration (NULL, TRUE);

  return 0;
}
