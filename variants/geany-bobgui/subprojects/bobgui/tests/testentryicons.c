#include <bobgui/bobgui.h>
#include <stdio.h>

static void
clear_pressed (BobguiEntry *entry, int icon, gpointer data)
{
   if (icon == BOBGUI_ENTRY_ICON_SECONDARY)
     bobgui_editable_set_text (BOBGUI_EDITABLE (entry), "");
}

static void
set_blank (BobguiWidget *button,
           BobguiEntry  *entry)
{
  if (bobgui_check_button_get_active (BOBGUI_CHECK_BUTTON (button)))
    bobgui_entry_set_icon_from_icon_name (entry, BOBGUI_ENTRY_ICON_SECONDARY, NULL);
}

static void
set_icon_name (BobguiWidget *button,
               BobguiEntry  *entry)
{
  if (bobgui_check_button_get_active (BOBGUI_CHECK_BUTTON (button)))
    bobgui_entry_set_icon_from_icon_name (entry, BOBGUI_ENTRY_ICON_SECONDARY, "media-floppy");
}

static void
set_gicon (BobguiWidget *button,
           BobguiEntry  *entry)
{
  GIcon *icon;

  if (bobgui_check_button_get_active (BOBGUI_CHECK_BUTTON (button)))
    {
      icon = g_themed_icon_new ("bobgui-yes");
      bobgui_entry_set_icon_from_gicon (entry, BOBGUI_ENTRY_ICON_SECONDARY, icon);
      g_object_unref (icon);
    }
}

static void
set_texture (BobguiWidget *button,
             BobguiEntry  *entry)
{
  GdkTexture *texture;

  if (bobgui_check_button_get_active (BOBGUI_CHECK_BUTTON (button)))
    {
      texture = gdk_texture_new_from_resource ("/org/bobgui/libbobgui/icons/32x32/places/network-workgroup.png");
      bobgui_entry_set_icon_from_paintable (entry, BOBGUI_ENTRY_ICON_SECONDARY, GDK_PAINTABLE (texture));
      g_object_unref (texture);
    }
}

static const char cssdata[] =
".entry-frame:not(:focus) { "
"  border: 2px solid alpha(gray,0.3);"
"}"
".entry-frame:focus { "
"  border: 2px solid red;"
"}"
".entry-frame entry { "
"  border: none; "
"  box-shadow: none; "
"}";

static void
icon_pressed_cb (BobguiGesture *gesture,
                 int         n_press,
                 double      x,
                 double      y,
                 gpointer    data)
{
  g_print ("You clicked me!\n");
}

static void
quit_cb (BobguiWidget *widget,
         gpointer   data)
{
  gboolean *done = data;

  *done = TRUE;

  g_main_context_wakeup (NULL);
}

int
main (int argc, char **argv)
{
  BobguiWidget *window;
  BobguiWidget *grid;
  BobguiWidget *label;
  BobguiWidget *entry;
  BobguiWidget *box;
  BobguiWidget *image;
  BobguiWidget *button1;
  BobguiWidget *button2;
  BobguiWidget *button3;
  BobguiWidget *button4;
  GIcon *icon;
  GdkContentProvider *content;
  gboolean done = FALSE;

  bobgui_init ();

  window = bobgui_window_new ();
  bobgui_window_set_title (BOBGUI_WINDOW (window), "Bobgui Entry Icons Test");

  g_signal_connect (G_OBJECT (window), "destroy",
		    G_CALLBACK (quit_cb), &done);

  grid = bobgui_grid_new ();
  bobgui_window_set_child (BOBGUI_WINDOW (window), grid);
  bobgui_grid_set_row_spacing (BOBGUI_GRID (grid), 6);
  bobgui_grid_set_column_spacing (BOBGUI_GRID (grid), 6);
  bobgui_widget_set_margin_start (grid, 10);
  bobgui_widget_set_margin_end (grid, 10);
  bobgui_widget_set_margin_top (grid, 10);
  bobgui_widget_set_margin_bottom (grid, 10);

  /*
   * Open File - Sets the icon using a GIcon
   */
  label = bobgui_label_new ("Open File:");
  bobgui_grid_attach (BOBGUI_GRID (grid), label, 0, 0, 1, 1);
  bobgui_widget_set_halign (label, BOBGUI_ALIGN_START);
  bobgui_widget_set_valign (label, BOBGUI_ALIGN_CENTER);

  entry = bobgui_entry_new ();
  bobgui_widget_set_hexpand (entry, TRUE);
  bobgui_grid_attach (BOBGUI_GRID (grid), entry, 1, 0, 1, 1);

  icon = g_themed_icon_new ("folder-symbolic");
  g_themed_icon_append_name (G_THEMED_ICON (icon), "folder-symbolic");

  bobgui_entry_set_icon_from_gicon (BOBGUI_ENTRY (entry),
                                 BOBGUI_ENTRY_ICON_PRIMARY,
                                 icon);
  g_object_unref (icon);
  bobgui_entry_set_icon_sensitive (BOBGUI_ENTRY (entry),
			        BOBGUI_ENTRY_ICON_PRIMARY,
				FALSE);

  bobgui_entry_set_icon_tooltip_text (BOBGUI_ENTRY (entry),
				   BOBGUI_ENTRY_ICON_PRIMARY,
				   "Open a file");

  /*
   * Save File - sets the icon using an icon name.
   */
  label = bobgui_label_new ("Save File:");
  bobgui_grid_attach (BOBGUI_GRID (grid), label, 0, 1, 1, 1);
  bobgui_widget_set_halign (label, BOBGUI_ALIGN_START);
  bobgui_widget_set_valign (label, BOBGUI_ALIGN_CENTER);

  entry = bobgui_entry_new ();
  bobgui_widget_set_hexpand (entry, TRUE);
  bobgui_grid_attach (BOBGUI_GRID (grid), entry, 1, 1, 1, 1);
  bobgui_editable_set_text (BOBGUI_EDITABLE (entry), "‏Right-to-left");
  bobgui_widget_set_direction (entry, BOBGUI_TEXT_DIR_RTL);

  bobgui_entry_set_icon_from_icon_name (BOBGUI_ENTRY (entry),
                                     BOBGUI_ENTRY_ICON_PRIMARY,
                                     "document-save-symbolic");
  bobgui_entry_set_icon_tooltip_text (BOBGUI_ENTRY (entry),
				   BOBGUI_ENTRY_ICON_PRIMARY,
				   "Save a file");

  content = gdk_content_provider_new_typed (G_TYPE_STRING, "Amazing");
  bobgui_entry_set_icon_drag_source (BOBGUI_ENTRY (entry),
                                  BOBGUI_ENTRY_ICON_PRIMARY,
                                  content, GDK_ACTION_COPY);
  g_object_unref (content);

  /*
   * Search - Uses a helper function
   */
  label = bobgui_label_new ("Search:");
  bobgui_grid_attach (BOBGUI_GRID (grid), label, 0, 2, 1, 1);
  bobgui_widget_set_halign (label, BOBGUI_ALIGN_START);
  bobgui_widget_set_valign (label, BOBGUI_ALIGN_CENTER);

  entry = bobgui_entry_new ();
  bobgui_widget_set_hexpand (entry, TRUE);
  bobgui_grid_attach (BOBGUI_GRID (grid), entry, 1, 2, 1, 1);

  bobgui_entry_set_placeholder_text (BOBGUI_ENTRY (entry),
                                  "Type some text, then click an icon");

  bobgui_entry_set_icon_from_icon_name (BOBGUI_ENTRY (entry),
                                     BOBGUI_ENTRY_ICON_PRIMARY,
                                     "edit-find-symbolic");

  bobgui_entry_set_icon_tooltip_text (BOBGUI_ENTRY (entry),
                                   BOBGUI_ENTRY_ICON_PRIMARY,
                                   "Clicking the other icon is more interesting!");

  bobgui_entry_set_icon_from_icon_name (BOBGUI_ENTRY (entry),
                                     BOBGUI_ENTRY_ICON_SECONDARY,
                                     "edit-clear-symbolic");

  bobgui_entry_set_icon_tooltip_text (BOBGUI_ENTRY (entry),
                                   BOBGUI_ENTRY_ICON_SECONDARY,
                                   "Clear");

  g_signal_connect (entry, "icon-press", G_CALLBACK (clear_pressed), NULL);

  /*
   * Password - Sets the icon using an icon name
   */
  label = bobgui_label_new ("Password:");
  bobgui_grid_attach (BOBGUI_GRID (grid), label, 0, 3, 1, 1);
  bobgui_widget_set_halign (label, BOBGUI_ALIGN_START);
  bobgui_widget_set_valign (label, BOBGUI_ALIGN_CENTER);

  entry = bobgui_password_entry_new ();
  bobgui_password_entry_set_show_peek_icon (BOBGUI_PASSWORD_ENTRY (entry), TRUE);
  bobgui_widget_set_hexpand (entry, TRUE);
  bobgui_grid_attach (BOBGUI_GRID (grid), entry, 1, 3, 1, 1);

  /* Name - Does not set any icons. */
  label = bobgui_label_new ("Name:");
  bobgui_grid_attach (BOBGUI_GRID (grid), label, 0, 4, 1, 1);
  bobgui_widget_set_halign (label, BOBGUI_ALIGN_START);
  bobgui_widget_set_valign (label, BOBGUI_ALIGN_CENTER);

  entry = bobgui_entry_new ();
  bobgui_widget_set_hexpand (entry, TRUE);
  bobgui_entry_set_placeholder_text (BOBGUI_ENTRY (entry),
                                  "Use the RadioButtons to choose an icon");
  bobgui_entry_set_icon_tooltip_text (BOBGUI_ENTRY (entry),
                                   BOBGUI_ENTRY_ICON_SECONDARY,
                                   "Use the RadioButtons to change this icon");
  bobgui_grid_attach (BOBGUI_GRID (grid), entry, 1, 4, 1, 1);

  box = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 6);
  bobgui_widget_set_vexpand (BOBGUI_WIDGET (box), TRUE);
  bobgui_grid_attach (BOBGUI_GRID (grid), box, 0, 5, 3, 1);

  button1 = bobgui_check_button_new_with_label ("Blank");
  bobgui_widget_set_valign (button1, BOBGUI_ALIGN_START);
  g_signal_connect (button1, "toggled", G_CALLBACK (set_blank), entry);
  bobgui_box_append (BOBGUI_BOX (box), button1);
  button2 = bobgui_check_button_new_with_label ("Icon Name");
  bobgui_widget_set_valign (button2, BOBGUI_ALIGN_START);
  bobgui_check_button_set_group (BOBGUI_CHECK_BUTTON (button2), BOBGUI_CHECK_BUTTON (button1));
  g_signal_connect (button2, "toggled", G_CALLBACK (set_icon_name), entry);
  bobgui_box_append (BOBGUI_BOX (box), button2);
  button3 = bobgui_check_button_new_with_label ("GIcon");
  bobgui_widget_set_valign (button3, BOBGUI_ALIGN_START);
  bobgui_check_button_set_group (BOBGUI_CHECK_BUTTON (button3), BOBGUI_CHECK_BUTTON (button1));
  g_signal_connect (button3, "toggled", G_CALLBACK (set_gicon), entry);
  bobgui_box_append (BOBGUI_BOX (box), button3);
  button4 = bobgui_check_button_new_with_label ("Texture");
  bobgui_widget_set_valign (button4, BOBGUI_ALIGN_START);
  bobgui_check_button_set_group (BOBGUI_CHECK_BUTTON (button4), BOBGUI_CHECK_BUTTON (button1));
  g_signal_connect (button4, "toggled", G_CALLBACK (set_texture), entry);
  bobgui_box_append (BOBGUI_BOX (box), button4);

  label = bobgui_label_new ("Emoji:");
  bobgui_grid_attach (BOBGUI_GRID (grid), label, 0, 6, 1, 1);
  bobgui_widget_set_halign (label, BOBGUI_ALIGN_START);
  bobgui_widget_set_valign (label, BOBGUI_ALIGN_CENTER);

  entry = bobgui_entry_new ();
  g_object_set (entry, "show-emoji-icon", TRUE, NULL);
  bobgui_widget_set_hexpand (entry, TRUE);
  bobgui_grid_attach (BOBGUI_GRID (grid), entry, 1, 6, 1, 1);

  box = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 0);
  bobgui_widget_add_css_class (box, "view");
  bobgui_widget_add_css_class (box, "entry-frame");
  bobgui_widget_set_cursor_from_name (box, "text");
  entry = bobgui_entry_new ();
  bobgui_widget_set_hexpand (entry, TRUE);
  bobgui_box_append (BOBGUI_BOX (box), entry);
  image = bobgui_image_new_from_icon_name ("edit-find-symbolic");
  bobgui_widget_set_cursor_from_name (image, "default");
  bobgui_widget_set_margin_start (image, 6);
  bobgui_widget_set_margin_end (image, 6);
  bobgui_widget_set_margin_top (image, 6);
  bobgui_widget_set_margin_bottom (image, 6);
  bobgui_widget_set_tooltip_text (image, "Click me");

  BobguiGesture *gesture;
  gesture = bobgui_gesture_click_new ();
  g_signal_connect (gesture, "pressed", G_CALLBACK (icon_pressed_cb), NULL);
  bobgui_widget_add_controller (image, BOBGUI_EVENT_CONTROLLER (gesture));
  bobgui_box_append (BOBGUI_BOX (box), image);
  image = bobgui_image_new_from_icon_name ("document-save-symbolic");
  bobgui_widget_set_margin_start (image, 6);
  bobgui_widget_set_margin_end (image, 6);
  bobgui_widget_set_margin_top (image, 6);
  bobgui_widget_set_margin_bottom (image, 6);
  bobgui_box_append (BOBGUI_BOX (box), image);
  bobgui_grid_attach (BOBGUI_GRID (grid), box, 1, 7, 1, 1);

  BobguiCssProvider *provider;
  provider = bobgui_css_provider_new ();
  bobgui_css_provider_load_from_string (provider, cssdata);
  bobgui_style_context_add_provider_for_display (gdk_display_get_default (), BOBGUI_STYLE_PROVIDER (provider), 800);
  bobgui_window_present (BOBGUI_WINDOW (window));

  while (!done)
    g_main_context_iteration (NULL, TRUE);

  return 0;
}
