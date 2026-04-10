#include <bobgui/bobgui.h>

int
main (int argc,
      char ** argv)
{
  BobguiWidget *window, *revealer, *box, *widget, *entry;

  bobgui_init ();

  window = bobgui_window_new ();
  bobgui_widget_set_size_request (window, 300, 300);

  box = bobgui_grid_new ();
  bobgui_window_set_child (BOBGUI_WINDOW (window), box);

  widget = bobgui_label_new ("Some filler text to avoid\nresizing of the window");
  bobgui_widget_set_margin_top (widget, 10);
  bobgui_widget_set_margin_bottom (widget, 10);
  bobgui_widget_set_margin_start (widget, 10);
  bobgui_widget_set_margin_end (widget, 10);
  bobgui_grid_attach (BOBGUI_GRID (box), widget, 1, 1, 1, 1);

  widget = bobgui_label_new ("Some filler text to avoid\nresizing of the window");
  bobgui_widget_set_margin_top (widget, 10);
  bobgui_widget_set_margin_bottom (widget, 10);
  bobgui_widget_set_margin_start (widget, 10);
  bobgui_widget_set_margin_end (widget, 10);
  bobgui_grid_attach (BOBGUI_GRID (box), widget, 4, 4, 1, 1);

  widget = bobgui_toggle_button_new_with_label ("None");
  bobgui_grid_attach (BOBGUI_GRID (box), widget, 0, 0, 1, 1);
  revealer = bobgui_revealer_new ();
  bobgui_widget_set_halign (revealer, BOBGUI_ALIGN_START);
  bobgui_widget_set_valign (revealer, BOBGUI_ALIGN_START);
  entry = bobgui_entry_new ();
  bobgui_editable_set_text (BOBGUI_EDITABLE (entry), "00000");
  bobgui_revealer_set_child (BOBGUI_REVEALER (revealer), entry);
  g_object_bind_property (widget, "active", revealer, "reveal-child", 0);
  bobgui_revealer_set_transition_type (BOBGUI_REVEALER (revealer), BOBGUI_REVEALER_TRANSITION_TYPE_NONE);
  bobgui_revealer_set_transition_duration (BOBGUI_REVEALER (revealer), 2000);
  bobgui_grid_attach (BOBGUI_GRID (box), revealer, 1, 0, 1, 1);

  widget = bobgui_toggle_button_new_with_label ("Fade");
  bobgui_grid_attach (BOBGUI_GRID (box), widget, 5, 5, 1, 1);
  revealer = bobgui_revealer_new ();
  bobgui_widget_set_halign (revealer, BOBGUI_ALIGN_END);
  bobgui_widget_set_valign (revealer, BOBGUI_ALIGN_END);
  entry = bobgui_entry_new ();
  bobgui_editable_set_text (BOBGUI_EDITABLE (entry), "00000");
  bobgui_revealer_set_child (BOBGUI_REVEALER (revealer), entry);
  g_object_bind_property (widget, "active", revealer, "reveal-child", 0);
  bobgui_revealer_set_transition_type (BOBGUI_REVEALER (revealer), BOBGUI_REVEALER_TRANSITION_TYPE_CROSSFADE);
  bobgui_revealer_set_transition_duration (BOBGUI_REVEALER (revealer), 2000);
  bobgui_grid_attach (BOBGUI_GRID (box), revealer, 4, 5, 1, 1);

  widget = bobgui_toggle_button_new_with_label ("Slide");
  bobgui_grid_attach (BOBGUI_GRID (box), widget, 0, 2, 1, 1);
  revealer = bobgui_revealer_new ();
  bobgui_widget_set_hexpand (revealer, TRUE);
  bobgui_widget_set_halign (revealer, BOBGUI_ALIGN_START);
  entry = bobgui_entry_new ();
  bobgui_editable_set_text (BOBGUI_EDITABLE (entry), "12345");
  bobgui_revealer_set_child (BOBGUI_REVEALER (revealer), entry);
  g_object_bind_property (widget, "active", revealer, "reveal-child", 0);
  bobgui_revealer_set_transition_type (BOBGUI_REVEALER (revealer), BOBGUI_REVEALER_TRANSITION_TYPE_SLIDE_RIGHT);
  bobgui_revealer_set_transition_duration (BOBGUI_REVEALER (revealer), 2000);
  bobgui_grid_attach (BOBGUI_GRID (box), revealer, 1, 2, 1, 1);

  widget = bobgui_toggle_button_new_with_label ("Swing");
  bobgui_widget_set_valign (widget, BOBGUI_ALIGN_START);
  bobgui_grid_attach (BOBGUI_GRID (box), widget, 0, 3, 1, 1);
  revealer = bobgui_revealer_new ();
  bobgui_widget_set_hexpand (revealer, TRUE);
  bobgui_widget_set_halign (revealer, BOBGUI_ALIGN_START);
  bobgui_widget_set_valign (revealer, BOBGUI_ALIGN_START);
  entry = bobgui_entry_new ();
  bobgui_editable_set_text (BOBGUI_EDITABLE (entry), "12345");
  bobgui_revealer_set_child (BOBGUI_REVEALER (revealer), entry);
  g_object_bind_property (widget, "active", revealer, "reveal-child", 0);
  bobgui_revealer_set_transition_type (BOBGUI_REVEALER (revealer), BOBGUI_REVEALER_TRANSITION_TYPE_SWING_RIGHT);
  bobgui_revealer_set_transition_duration (BOBGUI_REVEALER (revealer), 2000);
  bobgui_grid_attach (BOBGUI_GRID (box), revealer, 1, 3, 1, 1);

  widget = bobgui_toggle_button_new_with_label ("Slide");
  bobgui_grid_attach (BOBGUI_GRID (box), widget, 2, 0, 1, 1);
  revealer = bobgui_revealer_new ();
  bobgui_widget_set_vexpand (revealer, TRUE);
  bobgui_widget_set_valign (revealer, BOBGUI_ALIGN_START);
  entry = bobgui_entry_new ();
  bobgui_editable_set_text (BOBGUI_EDITABLE (entry), "23456");
  bobgui_revealer_set_child (BOBGUI_REVEALER (revealer), entry);
  g_object_bind_property (widget, "active", revealer, "reveal-child", 0);
  bobgui_revealer_set_transition_type (BOBGUI_REVEALER (revealer), BOBGUI_REVEALER_TRANSITION_TYPE_SLIDE_DOWN);
  bobgui_revealer_set_transition_duration (BOBGUI_REVEALER (revealer), 2000);
  bobgui_grid_attach (BOBGUI_GRID (box), revealer, 2, 1, 1, 1);

  widget = bobgui_toggle_button_new_with_label ("Swing");
  bobgui_grid_attach (BOBGUI_GRID (box), widget, 3, 0, 1, 1);
  revealer = bobgui_revealer_new ();
  bobgui_widget_set_vexpand (revealer, TRUE);
  bobgui_widget_set_valign (revealer, BOBGUI_ALIGN_START);
  entry = bobgui_entry_new ();
  bobgui_editable_set_text (BOBGUI_EDITABLE (entry), "23456");
  bobgui_revealer_set_child (BOBGUI_REVEALER (revealer), entry);
  g_object_bind_property (widget, "active", revealer, "reveal-child", 0);
  bobgui_revealer_set_transition_type (BOBGUI_REVEALER (revealer), BOBGUI_REVEALER_TRANSITION_TYPE_SWING_DOWN);
  bobgui_revealer_set_transition_duration (BOBGUI_REVEALER (revealer), 2000);
  bobgui_grid_attach (BOBGUI_GRID (box), revealer, 3, 1, 1, 1);

  widget = bobgui_toggle_button_new_with_label ("Slide");
  bobgui_grid_attach (BOBGUI_GRID (box), widget, 5, 2, 1, 1);
  revealer = bobgui_revealer_new ();
  bobgui_widget_set_hexpand (revealer, TRUE);
  bobgui_widget_set_halign (revealer, BOBGUI_ALIGN_END);
  entry = bobgui_entry_new ();
  bobgui_editable_set_text (BOBGUI_EDITABLE (entry), "34567");
  bobgui_revealer_set_child (BOBGUI_REVEALER (revealer), entry);
  g_object_bind_property (widget, "active", revealer, "reveal-child", 0);
  bobgui_revealer_set_transition_type (BOBGUI_REVEALER (revealer), BOBGUI_REVEALER_TRANSITION_TYPE_SLIDE_LEFT);
  bobgui_revealer_set_transition_duration (BOBGUI_REVEALER (revealer), 2000);
  bobgui_grid_attach (BOBGUI_GRID (box), revealer, 4, 2, 1, 1);

  widget = bobgui_toggle_button_new_with_label ("Swing");
  bobgui_widget_set_valign (widget, BOBGUI_ALIGN_START);
  bobgui_grid_attach (BOBGUI_GRID (box), widget, 5, 3, 1, 1);
  revealer = bobgui_revealer_new ();
  bobgui_widget_set_hexpand (revealer, TRUE);
  bobgui_widget_set_halign (revealer, BOBGUI_ALIGN_END);
  bobgui_widget_set_valign (revealer, BOBGUI_ALIGN_START);
  entry = bobgui_entry_new ();
  bobgui_editable_set_text (BOBGUI_EDITABLE (entry), "34567");
  bobgui_revealer_set_child (BOBGUI_REVEALER (revealer), entry);
  g_object_bind_property (widget, "active", revealer, "reveal-child", 0);
  bobgui_revealer_set_transition_type (BOBGUI_REVEALER (revealer), BOBGUI_REVEALER_TRANSITION_TYPE_SWING_LEFT);
  bobgui_revealer_set_transition_duration (BOBGUI_REVEALER (revealer), 2000);
  bobgui_grid_attach (BOBGUI_GRID (box), revealer, 4, 3, 1, 1);

  widget = bobgui_toggle_button_new_with_label ("Slide");
  bobgui_grid_attach (BOBGUI_GRID (box), widget, 2, 5, 1, 1);
  revealer = bobgui_revealer_new ();
  bobgui_widget_set_vexpand (revealer, TRUE);
  bobgui_widget_set_valign (revealer, BOBGUI_ALIGN_END);
  entry = bobgui_entry_new ();
  bobgui_editable_set_text (BOBGUI_EDITABLE (entry), "45678");
  bobgui_revealer_set_child (BOBGUI_REVEALER (revealer), entry);
  g_object_bind_property (widget, "active", revealer, "reveal-child", 0);
  bobgui_revealer_set_transition_type (BOBGUI_REVEALER (revealer), BOBGUI_REVEALER_TRANSITION_TYPE_SLIDE_UP);
  bobgui_revealer_set_transition_duration (BOBGUI_REVEALER (revealer), 2000);
  bobgui_grid_attach (BOBGUI_GRID (box), revealer, 2, 4, 1, 1);

  widget = bobgui_toggle_button_new_with_label ("Swing");
  bobgui_grid_attach (BOBGUI_GRID (box), widget, 3, 5, 1, 1);
  revealer = bobgui_revealer_new ();
  bobgui_widget_set_vexpand (revealer, TRUE);
  bobgui_widget_set_valign (revealer, BOBGUI_ALIGN_END);
  entry = bobgui_entry_new ();
  bobgui_editable_set_text (BOBGUI_EDITABLE (entry), "45678");
  bobgui_revealer_set_child (BOBGUI_REVEALER (revealer), entry);
  g_object_bind_property (widget, "active", revealer, "reveal-child", 0);
  bobgui_revealer_set_transition_type (BOBGUI_REVEALER (revealer), BOBGUI_REVEALER_TRANSITION_TYPE_SWING_UP);
  bobgui_revealer_set_transition_duration (BOBGUI_REVEALER (revealer), 2000);
  bobgui_grid_attach (BOBGUI_GRID (box), revealer, 3, 4, 1, 1);

  bobgui_window_present (BOBGUI_WINDOW (window));
  while (TRUE)
    g_main_context_iteration (NULL, TRUE);

  bobgui_window_destroy (BOBGUI_WINDOW (window));

  return 0;
}
