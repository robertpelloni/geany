#include <bobgui/bobgui.h>

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

static void
horizontal_policy_changed (BobguiComboBox *combo_box,
			   BobguiViewport *viewport)
{
  BobguiScrollablePolicy policy = bobgui_combo_box_get_active (combo_box);

  bobgui_scrollable_set_hscroll_policy (BOBGUI_SCROLLABLE (viewport), policy);
}

static void
vertical_policy_changed (BobguiComboBox *combo_box,
			 BobguiViewport *viewport)
{
  BobguiScrollablePolicy policy = bobgui_combo_box_get_active (combo_box);

  bobgui_scrollable_set_vscroll_policy (BOBGUI_SCROLLABLE (viewport), policy);
}

static void
content_width_changed (BobguiSpinButton *spin_button,
                       gpointer       data)
{
  BobguiScrolledWindow *swindow = data;
  double value;

  value = bobgui_spin_button_get_value (spin_button);
  bobgui_scrolled_window_set_min_content_width (swindow, (int)value);
}

static void
content_height_changed (BobguiSpinButton *spin_button,
                        gpointer       data)
{
  BobguiScrolledWindow *swindow = data;
  double value;

  value = bobgui_spin_button_get_value (spin_button);
  bobgui_scrolled_window_set_min_content_height (swindow, (int)value);
}

static void
kinetic_scrolling_changed (BobguiToggleButton *toggle_button,
                           gpointer         data)
{
  BobguiScrolledWindow *swindow = data;
  gboolean enabled = bobgui_toggle_button_get_active (toggle_button);

  bobgui_scrolled_window_set_kinetic_scrolling (swindow, enabled);
}

static void
add_row (BobguiButton  *button,
         BobguiListBox *listbox)
{
  BobguiWidget *row;

  row = g_object_new (BOBGUI_TYPE_LIST_BOX_ROW, NULL);
  bobgui_list_box_row_set_child (BOBGUI_LIST_BOX_ROW (row), bobgui_label_new ("test"));
  bobgui_list_box_insert (BOBGUI_LIST_BOX (listbox), row, -1);
}

static void
remove_row (BobguiButton  *button,
            BobguiListBox *listbox)
{
  BobguiWidget *last;

  last = bobgui_widget_get_last_child (BOBGUI_WIDGET (listbox));
  if (last)
    bobgui_list_box_remove (BOBGUI_LIST_BOX (listbox), last);
}

static void
scrollable_policy (void)
{
  BobguiWidget *window, *swindow, *hbox, *vbox, *frame, *cntl, *listbox;
  BobguiWidget *viewport, *label, *expander, *widget, *popover;

  window = bobgui_window_new ();
  hbox   = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 2);
  vbox   = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 6);

  bobgui_window_set_child (BOBGUI_WINDOW (window), hbox);
  bobgui_box_append (BOBGUI_BOX (hbox), vbox);

  frame = bobgui_frame_new ("Scrolled Window");
  bobgui_widget_set_hexpand (frame, TRUE);
  bobgui_box_append (BOBGUI_BOX (hbox), frame);

  swindow = bobgui_scrolled_window_new ();
  bobgui_scrolled_window_set_policy (BOBGUI_SCROLLED_WINDOW (swindow),
                                  BOBGUI_POLICY_AUTOMATIC, BOBGUI_POLICY_AUTOMATIC);

  bobgui_frame_set_child (BOBGUI_FRAME (frame), swindow);

  viewport = bobgui_viewport_new (NULL, NULL);
  label = bobgui_label_new ("Here is a wrapping label with a minimum width-chars of 40 and "
			 "a natural max-width-chars of 100 to demonstrate the usage of "
			 "scrollable widgets \"hscroll-policy\" and \"vscroll-policy\" "
			 "properties. Note also that when playing with the window height, "
			 "one can observe that the vscrollbar disappears as soon as there "
			 "is enough height to fit the content vertically if the window were "
			 "to be allocated a width without a vscrollbar present");

  bobgui_label_set_wrap (BOBGUI_LABEL (label), TRUE);
  bobgui_label_set_width_chars  (BOBGUI_LABEL (label), 40);
  bobgui_label_set_max_width_chars  (BOBGUI_LABEL (label), 100);

  bobgui_viewport_set_child (BOBGUI_VIEWPORT (viewport), label);
  bobgui_scrolled_window_set_child (BOBGUI_SCROLLED_WINDOW (swindow), viewport);

  /* Add controls here */
  expander = bobgui_expander_new ("Controls");
  bobgui_expander_set_expanded (BOBGUI_EXPANDER (expander), TRUE);
  cntl = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 2);
  bobgui_expander_set_child (BOBGUI_EXPANDER (expander), cntl);
  bobgui_box_append (BOBGUI_BOX (vbox), expander);

  /* Add Horizontal policy control here */
  hbox = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 2);

  widget = bobgui_label_new ("hscroll-policy");
  bobgui_widget_set_hexpand (widget, TRUE);
  bobgui_box_append (BOBGUI_BOX (hbox), widget);

  widget = bobgui_combo_box_text_new ();
  bobgui_combo_box_text_append_text (BOBGUI_COMBO_BOX_TEXT (widget), "Minimum");
  bobgui_combo_box_text_append_text (BOBGUI_COMBO_BOX_TEXT (widget), "Natural");
  bobgui_combo_box_set_active (BOBGUI_COMBO_BOX (widget), 0);
  bobgui_widget_set_hexpand (widget, TRUE);

  bobgui_box_append (BOBGUI_BOX (hbox), widget);
  bobgui_box_append (BOBGUI_BOX (cntl), hbox);

  g_signal_connect (G_OBJECT (widget), "changed",
                    G_CALLBACK (horizontal_policy_changed), viewport);

  /* Add Vertical policy control here */
  hbox = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 2);

  widget = bobgui_label_new ("vscroll-policy");
  bobgui_widget_set_hexpand (widget, TRUE);
  bobgui_box_append (BOBGUI_BOX (hbox), widget);

  widget = bobgui_combo_box_text_new ();
  bobgui_combo_box_text_append_text (BOBGUI_COMBO_BOX_TEXT (widget), "Minimum");
  bobgui_combo_box_text_append_text (BOBGUI_COMBO_BOX_TEXT (widget), "Natural");
  bobgui_combo_box_set_active (BOBGUI_COMBO_BOX (widget), 0);
  bobgui_widget_set_hexpand (widget, TRUE);

  bobgui_box_append (BOBGUI_BOX (hbox), widget);
  bobgui_box_append (BOBGUI_BOX (cntl), hbox);

  g_signal_connect (G_OBJECT (widget), "changed",
                    G_CALLBACK (vertical_policy_changed), viewport);

  /* Content size controls */
  hbox = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 2);

  widget = bobgui_label_new ("min-content-width");
  bobgui_widget_set_hexpand (widget, TRUE);
  bobgui_box_append (BOBGUI_BOX (hbox), widget);

  widget = bobgui_spin_button_new_with_range (100.0, 1000.0, 10.0);
  bobgui_widget_set_hexpand (widget, TRUE);
  bobgui_box_append (BOBGUI_BOX (hbox), widget);
  bobgui_box_append (BOBGUI_BOX (cntl), hbox);

  g_signal_connect (G_OBJECT (widget), "value-changed",
                    G_CALLBACK (content_width_changed), swindow);

  hbox = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 2);

  widget = bobgui_label_new ("min-content-height");
  bobgui_widget_set_hexpand (widget, TRUE);
  bobgui_box_append (BOBGUI_BOX (hbox), widget);

  widget = bobgui_spin_button_new_with_range (100.0, 1000.0, 10.0);
  bobgui_widget_set_hexpand (widget, TRUE);
  bobgui_box_append (BOBGUI_BOX (hbox), widget);
  bobgui_box_append (BOBGUI_BOX (cntl), hbox);

  g_signal_connect (G_OBJECT (widget), "value-changed",
                    G_CALLBACK (content_height_changed), swindow);

  /* Add Kinetic scrolling control here */
  widget = bobgui_check_button_new_with_label ("Kinetic scrolling");
  bobgui_widget_set_hexpand (widget, TRUE);
  bobgui_box_append (BOBGUI_BOX (cntl), widget);
  g_signal_connect (G_OBJECT (widget), "toggled",
                    G_CALLBACK (kinetic_scrolling_changed), swindow);

  bobgui_window_present (BOBGUI_WINDOW (window));

  /* Popover */
  popover = bobgui_popover_new ();

  widget = bobgui_menu_button_new ();
  bobgui_menu_button_set_popover (BOBGUI_MENU_BUTTON (widget), popover);
  bobgui_menu_button_set_label (BOBGUI_MENU_BUTTON (widget), "Popover");
  bobgui_box_append (BOBGUI_BOX (cntl), widget);

  vbox = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 6);
  bobgui_popover_set_child (BOBGUI_POPOVER (popover), vbox);

  /* Popover's scrolled window */
  swindow = bobgui_scrolled_window_new ();
  bobgui_scrolled_window_set_policy (BOBGUI_SCROLLED_WINDOW (swindow),
                                  BOBGUI_POLICY_AUTOMATIC, BOBGUI_POLICY_AUTOMATIC);

    bobgui_box_append (BOBGUI_BOX (vbox), swindow);

  /* Listbox */
  listbox = bobgui_list_box_new ();
  bobgui_scrolled_window_set_child (BOBGUI_SCROLLED_WINDOW (swindow), listbox);

  /* Min content */
  hbox = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 2);

  widget = bobgui_label_new ("min-content-width");
  bobgui_widget_set_hexpand (widget, TRUE);
  bobgui_box_append (BOBGUI_BOX (hbox), widget);

  widget = bobgui_spin_button_new_with_range (0.0, 150.0, 10.0);
  bobgui_widget_set_hexpand (widget, TRUE);
  bobgui_box_append (BOBGUI_BOX (hbox), widget);

  g_object_bind_property (bobgui_spin_button_get_adjustment (BOBGUI_SPIN_BUTTON (widget)),
                          "value",
                          swindow,
                          "min-content-width",
                          G_BINDING_DEFAULT | G_BINDING_SYNC_CREATE);

  widget = bobgui_label_new ("min-content-height");
  bobgui_widget_set_hexpand (widget, TRUE);
  bobgui_box_append (BOBGUI_BOX (hbox), widget);


  widget = bobgui_spin_button_new_with_range (0.0, 150.0, 10.0);
  bobgui_widget_set_hexpand (widget, TRUE);
  bobgui_box_append (BOBGUI_BOX (hbox), widget);
  bobgui_box_append (BOBGUI_BOX (vbox), hbox);

  g_object_bind_property (bobgui_spin_button_get_adjustment (BOBGUI_SPIN_BUTTON (widget)),
                          "value",
                          swindow,
                          "min-content-height",
                          G_BINDING_DEFAULT | G_BINDING_SYNC_CREATE);

  /* Max content */
  hbox = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 2);

  widget = bobgui_label_new ("max-content-width");
  bobgui_widget_set_hexpand (widget, TRUE);
  bobgui_box_append (BOBGUI_BOX (hbox), widget);

  widget = bobgui_spin_button_new_with_range (250.0, 1000.0, 10.0);
  bobgui_widget_set_hexpand (widget, TRUE);
  bobgui_box_append (BOBGUI_BOX (hbox), widget);

  g_object_bind_property (bobgui_spin_button_get_adjustment (BOBGUI_SPIN_BUTTON (widget)),
                          "value",
                          swindow,
                          "max-content-width",
                          G_BINDING_DEFAULT | G_BINDING_SYNC_CREATE);

  widget = bobgui_label_new ("max-content-height");
  bobgui_widget_set_hexpand (widget, TRUE);
  bobgui_box_append (BOBGUI_BOX (hbox), widget);

  widget = bobgui_spin_button_new_with_range (250.0, 1000.0, 10.0);
  bobgui_widget_set_hexpand (widget, TRUE);
  bobgui_box_append (BOBGUI_BOX (hbox), widget);
  bobgui_box_append (BOBGUI_BOX (vbox), hbox);

  g_object_bind_property (bobgui_spin_button_get_adjustment (BOBGUI_SPIN_BUTTON (widget)),
                          "value",
                          swindow,
                          "max-content-height",
                          G_BINDING_DEFAULT | G_BINDING_SYNC_CREATE);

  /* Add and Remove buttons */
  hbox = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 2);

  widget = bobgui_button_new_with_label ("Remove");
  bobgui_widget_set_hexpand (widget, TRUE);
  bobgui_box_append (BOBGUI_BOX (hbox), widget);

  g_signal_connect (widget, "clicked",
                    G_CALLBACK (remove_row), listbox);

  widget = bobgui_button_new_with_label ("Add");
  bobgui_widget_set_hexpand (widget, TRUE);
  bobgui_box_append (BOBGUI_BOX (hbox), widget);
  bobgui_box_append (BOBGUI_BOX (vbox), hbox);

  g_signal_connect (widget, "clicked",
                    G_CALLBACK (add_row), listbox);
}


int
main (int argc, char *argv[])
{
  bobgui_init ();

  scrollable_policy ();

  while (TRUE)
    g_main_context_iteration (NULL, TRUE);

  return 0;
}
