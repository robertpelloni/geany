#include <bobgui/bobgui.h>

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

#define INITIAL_HALIGN          BOBGUI_ALIGN_START
#define INITIAL_VALIGN          BOBGUI_ALIGN_START

static GList *menubuttons = NULL;

static void
horizontal_alignment_changed (BobguiComboBox *box)
{
	BobguiAlign alignment = bobgui_combo_box_get_active (box);
	GList *l;

	for (l = menubuttons; l != NULL; l = l->next) {
		BobguiPopover *popup = bobgui_menu_button_get_popover (BOBGUI_MENU_BUTTON (l->data));
		if (popup != NULL)
			bobgui_widget_set_halign (BOBGUI_WIDGET (popup), alignment);
	}
}

static void
vertical_alignment_changed (BobguiComboBox *box)
{
	BobguiAlign alignment = bobgui_combo_box_get_active (box);
	GList *l;

	for (l = menubuttons; l != NULL; l = l->next) {
		BobguiPopover *popup = bobgui_menu_button_get_popover (BOBGUI_MENU_BUTTON (l->data));
		if (popup != NULL)
			bobgui_widget_set_valign (BOBGUI_WIDGET (popup), alignment);
	}
}

int main (int argc, char **argv)
{
	BobguiWidget *window;
	BobguiWidget *button;
	BobguiWidget *grid;
	BobguiWidget *entry;
	BobguiWidget *label;
	BobguiWidget *combo;
	guint i;
	guint row = 0;
	GMenu *menu;

	bobgui_init ();

	window = bobgui_window_new ();
	bobgui_window_set_default_size (BOBGUI_WINDOW (window), 400, 300);

	grid = bobgui_grid_new ();
	bobgui_grid_set_row_spacing (BOBGUI_GRID (grid), 12);
	bobgui_grid_set_column_spacing (BOBGUI_GRID (grid), 12);
	bobgui_window_set_child (BOBGUI_WINDOW (window), grid);

	/* horizontal alignment */
	label = bobgui_label_new ("Horizontal Alignment:");
	bobgui_grid_attach (BOBGUI_GRID (grid), label, 0, row++, 1, 1);

	combo = bobgui_combo_box_text_new ();
	bobgui_combo_box_text_append_text (BOBGUI_COMBO_BOX_TEXT (combo), "Fill");
	bobgui_combo_box_text_append_text (BOBGUI_COMBO_BOX_TEXT (combo), "Start");
	bobgui_combo_box_text_append_text (BOBGUI_COMBO_BOX_TEXT (combo), "End");
	bobgui_combo_box_text_append_text (BOBGUI_COMBO_BOX_TEXT (combo), "Center");
	bobgui_combo_box_text_append_text (BOBGUI_COMBO_BOX_TEXT (combo), "Baseline");
	bobgui_combo_box_set_active (BOBGUI_COMBO_BOX (combo), INITIAL_HALIGN);
	bobgui_grid_attach_next_to (BOBGUI_GRID (grid), combo, label, BOBGUI_POS_RIGHT, 1, 1);
	g_signal_connect (G_OBJECT (combo), "changed",
			  G_CALLBACK (horizontal_alignment_changed), menubuttons);

	/* vertical alignment */
	label = bobgui_label_new ("Vertical Alignment:");
	bobgui_grid_attach (BOBGUI_GRID (grid), label, 0, row++, 1, 1);

	combo = bobgui_combo_box_text_new ();
	bobgui_combo_box_text_append_text (BOBGUI_COMBO_BOX_TEXT (combo), "Fill");
	bobgui_combo_box_text_append_text (BOBGUI_COMBO_BOX_TEXT (combo), "Start");
	bobgui_combo_box_text_append_text (BOBGUI_COMBO_BOX_TEXT (combo), "End");
	bobgui_combo_box_text_append_text (BOBGUI_COMBO_BOX_TEXT (combo), "Center");
	bobgui_combo_box_text_append_text (BOBGUI_COMBO_BOX_TEXT (combo), "Baseline");
	bobgui_combo_box_set_active (BOBGUI_COMBO_BOX (combo), INITIAL_HALIGN);
	bobgui_grid_attach_next_to (BOBGUI_GRID (grid), combo, label, BOBGUI_POS_RIGHT, 1, 1);
	g_signal_connect (G_OBJECT (combo), "changed",
			  G_CALLBACK (vertical_alignment_changed), menubuttons);

	/* Button next to entry */
	entry = bobgui_entry_new ();
	bobgui_grid_attach (BOBGUI_GRID (grid), entry, 0, row++, 1, 1);
	button = bobgui_menu_button_new ();
	bobgui_widget_set_halign (button, BOBGUI_ALIGN_START);

	bobgui_grid_attach_next_to (BOBGUI_GRID (grid), button, entry, BOBGUI_POS_RIGHT, 1, 1);
	menubuttons = g_list_prepend (menubuttons, button);

	/* Button with GMenuModel */
	menu = g_menu_new ();
	for (i = 5; i > 0; i--) {
		char *item_label;
                GMenuItem *item;
		item_label = g_strdup_printf ("Item _%d", i);
                item = g_menu_item_new (item_label, NULL);
                if (i == 3)
                  g_menu_item_set_attribute (item, "icon", "s", "preferences-desktop-locale-symbolic");
		g_menu_insert_item (menu, 0, item);
                g_object_unref (item);
		g_free (item_label);
	}

	button = bobgui_menu_button_new ();

	bobgui_widget_set_halign (button, BOBGUI_ALIGN_START);
	menubuttons = g_list_prepend (menubuttons, button);
	bobgui_menu_button_set_menu_model (BOBGUI_MENU_BUTTON (button), G_MENU_MODEL (menu));
	bobgui_grid_attach (BOBGUI_GRID (grid), button, 1, row++, 1, 1);

	bobgui_window_present (BOBGUI_WINDOW (window));

        while (TRUE)
                g_main_context_iteration (NULL, TRUE);

	return 0;
}
