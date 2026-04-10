#include <bobgui/bobgui.h>

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

/*******************************************************
 *                      Simple Test                    *
 *******************************************************/
enum {
  SIMPLE_COLUMN_NAME,
  SIMPLE_COLUMN_ICON,
  SIMPLE_COLUMN_DESCRIPTION,
  N_SIMPLE_COLUMNS
};

static BobguiCellRenderer *cell_1 = NULL, *cell_2 = NULL, *cell_3 = NULL;

static BobguiTreeModel *
simple_list_model (void)
{
  BobguiTreeIter   iter;
  BobguiListStore *store = 
    bobgui_list_store_new (N_SIMPLE_COLUMNS,
			G_TYPE_STRING,  /* name text */
			G_TYPE_STRING,  /* icon name */
			G_TYPE_STRING); /* description text */

  bobgui_list_store_append (store, &iter);
  bobgui_list_store_set (store, &iter, 
		      SIMPLE_COLUMN_NAME, "Alice in wonderland",
		      SIMPLE_COLUMN_ICON, "system-run",
		      SIMPLE_COLUMN_DESCRIPTION, 
		      "Twas brillig, and the slithy toves "
		      "did gyre and gimble in the wabe; "
		      "all mimsy were the borogoves, "
		      "and the mome raths outgrabe",
		      -1);

  bobgui_list_store_append (store, &iter);
  bobgui_list_store_set (store, &iter, 
		      SIMPLE_COLUMN_NAME, "Marry Poppins",
		      SIMPLE_COLUMN_ICON, "dialog-information",
		      SIMPLE_COLUMN_DESCRIPTION, "Supercalifragilisticexpialidocious",
		      -1);

  bobgui_list_store_append (store, &iter);
  bobgui_list_store_set (store, &iter, 
		      SIMPLE_COLUMN_NAME, "George Bush",
		      SIMPLE_COLUMN_ICON, "dialog-warning",
		      SIMPLE_COLUMN_DESCRIPTION, "It's a very good question, very direct, "
		      "and I'm not going to answer it",
		      -1);

  bobgui_list_store_append (store, &iter);
  bobgui_list_store_set (store, &iter, 
		      SIMPLE_COLUMN_NAME, "Whinnie the pooh",
		      SIMPLE_COLUMN_ICON, "process-stop",
		      SIMPLE_COLUMN_DESCRIPTION, "The most wonderful thing about tiggers, "
		      "is tiggers are wonderful things",
		      -1);

  bobgui_list_store_append (store, &iter);
  bobgui_list_store_set (store, &iter, 
		      SIMPLE_COLUMN_NAME, "Aleister Crowley",
		      SIMPLE_COLUMN_ICON, "help-about",
		      SIMPLE_COLUMN_DESCRIPTION, 
		      "Thou shalt do what thou wilt shall be the whole of the law",
		      -1);

  bobgui_list_store_append (store, &iter);
  bobgui_list_store_set (store, &iter, 
		      SIMPLE_COLUMN_NAME, "Mark Twain",
		      SIMPLE_COLUMN_ICON, "application-exit",
		      SIMPLE_COLUMN_DESCRIPTION, 
		      "Giving up smoking is the easiest thing in the world. "
		      "I know because I've done it thousands of times.",
		      -1);


  return (BobguiTreeModel *)store;
}

static BobguiWidget *
simple_iconview (void)
{
  BobguiTreeModel *model;
  BobguiWidget *iconview;
  BobguiCellArea *area;
  BobguiCellRenderer *renderer;

  iconview = bobgui_icon_view_new ();

  model = simple_list_model ();

  bobgui_icon_view_set_model (BOBGUI_ICON_VIEW (iconview), model);
  bobgui_icon_view_set_item_orientation (BOBGUI_ICON_VIEW (iconview), BOBGUI_ORIENTATION_HORIZONTAL);

  area = bobgui_cell_layout_get_area (BOBGUI_CELL_LAYOUT (iconview));

  cell_1 = renderer = bobgui_cell_renderer_text_new ();
  bobgui_cell_area_box_pack_start (BOBGUI_CELL_AREA_BOX (area), renderer, FALSE, FALSE, FALSE);
  bobgui_cell_area_attribute_connect (area, renderer, "text", SIMPLE_COLUMN_NAME);

  cell_2 = renderer = bobgui_cell_renderer_pixbuf_new ();
  g_object_set (G_OBJECT (renderer), "xalign", 0.0F, NULL);
  bobgui_cell_area_box_pack_start (BOBGUI_CELL_AREA_BOX (area), renderer, TRUE, FALSE, FALSE);
  bobgui_cell_area_attribute_connect (area, renderer, "icon-name", SIMPLE_COLUMN_ICON);

  cell_3 = renderer = bobgui_cell_renderer_text_new ();
  g_object_set (G_OBJECT (renderer), 
		"wrap-mode", PANGO_WRAP_WORD,
		"wrap-width", 215,
		NULL);
  bobgui_cell_area_box_pack_start (BOBGUI_CELL_AREA_BOX (area), renderer, FALSE, TRUE, FALSE);
  bobgui_cell_area_attribute_connect (area, renderer, "text", SIMPLE_COLUMN_DESCRIPTION);

  return iconview;
}

static void
orientation_changed (BobguiComboBox      *combo,
		     BobguiIconView *iconview)
{
  BobguiOrientation orientation = bobgui_combo_box_get_active (combo);

  bobgui_icon_view_set_item_orientation (iconview, orientation);
}

static void
align_cell_2_toggled (BobguiCheckButton  *toggle,
		      BobguiIconView *iconview)
{
  BobguiCellArea *area = bobgui_cell_layout_get_area (BOBGUI_CELL_LAYOUT (iconview));
  gboolean     align = bobgui_check_button_get_active (toggle);

  bobgui_cell_area_cell_set (area, cell_2, "align", align, NULL);
}

static void
align_cell_3_toggled (BobguiCheckButton  *toggle,
		      BobguiIconView *iconview)
{
  BobguiCellArea *area = bobgui_cell_layout_get_area (BOBGUI_CELL_LAYOUT (iconview));
  gboolean     align = bobgui_check_button_get_active (toggle);

  bobgui_cell_area_cell_set (area, cell_3, "align", align, NULL);
}

static void
expand_cell_1_toggled (BobguiCheckButton  *toggle,
		       BobguiIconView *iconview)
{
  BobguiCellArea *area = bobgui_cell_layout_get_area (BOBGUI_CELL_LAYOUT (iconview));
  gboolean     expand = bobgui_check_button_get_active (toggle);

  bobgui_cell_area_cell_set (area, cell_1, "expand", expand, NULL);
}

static void
expand_cell_2_toggled (BobguiCheckButton  *toggle,
		       BobguiIconView *iconview)
{
  BobguiCellArea *area = bobgui_cell_layout_get_area (BOBGUI_CELL_LAYOUT (iconview));
  gboolean     expand = bobgui_check_button_get_active (toggle);

  bobgui_cell_area_cell_set (area, cell_2, "expand", expand, NULL);
}

static void
expand_cell_3_toggled (BobguiCheckButton  *toggle,
		       BobguiIconView *iconview)
{
  BobguiCellArea *area = bobgui_cell_layout_get_area (BOBGUI_CELL_LAYOUT (iconview));
  gboolean     expand = bobgui_check_button_get_active (toggle);

  bobgui_cell_area_cell_set (area, cell_3, "expand", expand, NULL);
}

static void
simple_cell_area (void)
{
  BobguiWidget *window, *widget;
  BobguiWidget *iconview, *frame, *vbox, *hbox;

  window = bobgui_window_new ();

  bobgui_window_set_title (BOBGUI_WINDOW (window), "CellArea expand and alignments");

  iconview = simple_iconview ();

  hbox  = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 4);
  frame = bobgui_frame_new (NULL);
  bobgui_widget_set_hexpand (frame, TRUE);

  bobgui_widget_set_valign (frame, BOBGUI_ALIGN_CENTER);
  bobgui_widget_set_halign (frame, BOBGUI_ALIGN_FILL);

  bobgui_frame_set_child (BOBGUI_FRAME (frame), iconview);

  /* Now add some controls */
  vbox  = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 4);
  bobgui_box_append (BOBGUI_BOX (hbox), vbox);

  bobgui_box_append (BOBGUI_BOX (hbox), frame);

  widget = bobgui_combo_box_text_new ();
  bobgui_combo_box_text_append_text (BOBGUI_COMBO_BOX_TEXT (widget), "Horizontal");
  bobgui_combo_box_text_append_text (BOBGUI_COMBO_BOX_TEXT (widget), "Vertical");
  bobgui_combo_box_set_active (BOBGUI_COMBO_BOX (widget), 0);
  bobgui_box_append (BOBGUI_BOX (vbox), widget);

  g_signal_connect (G_OBJECT (widget), "changed",
                    G_CALLBACK (orientation_changed), iconview);

  widget = bobgui_check_button_new_with_label ("Align 2nd Cell");
  bobgui_check_button_set_active (BOBGUI_CHECK_BUTTON (widget), FALSE);
  bobgui_box_append (BOBGUI_BOX (vbox), widget);

  g_signal_connect (G_OBJECT (widget), "toggled",
                    G_CALLBACK (align_cell_2_toggled), iconview);

  widget = bobgui_check_button_new_with_label ("Align 3rd Cell");
  bobgui_check_button_set_active (BOBGUI_CHECK_BUTTON (widget), TRUE);
  bobgui_box_append (BOBGUI_BOX (vbox), widget);

  g_signal_connect (G_OBJECT (widget), "toggled",
                    G_CALLBACK (align_cell_3_toggled), iconview);


  widget = bobgui_check_button_new_with_label ("Expand 1st Cell");
  bobgui_check_button_set_active (BOBGUI_CHECK_BUTTON (widget), FALSE);
  bobgui_box_append (BOBGUI_BOX (vbox), widget);

  g_signal_connect (G_OBJECT (widget), "toggled",
                    G_CALLBACK (expand_cell_1_toggled), iconview);

  widget = bobgui_check_button_new_with_label ("Expand 2nd Cell");
  bobgui_check_button_set_active (BOBGUI_CHECK_BUTTON (widget), TRUE);
  bobgui_box_append (BOBGUI_BOX (vbox), widget);

  g_signal_connect (G_OBJECT (widget), "toggled",
                    G_CALLBACK (expand_cell_2_toggled), iconview);

  widget = bobgui_check_button_new_with_label ("Expand 3rd Cell");
  bobgui_check_button_set_active (BOBGUI_CHECK_BUTTON (widget), FALSE);
  bobgui_box_append (BOBGUI_BOX (vbox), widget);

  g_signal_connect (G_OBJECT (widget), "toggled",
                    G_CALLBACK (expand_cell_3_toggled), iconview);

  bobgui_window_set_child (BOBGUI_WINDOW (window), hbox);

  bobgui_window_present (BOBGUI_WINDOW (window));
}

/*******************************************************
 *                      Focus Test                     *
 *******************************************************/
static BobguiCellRenderer *focus_renderer, *sibling_renderer;

enum {
  FOCUS_COLUMN_NAME,
  FOCUS_COLUMN_CHECK,
  FOCUS_COLUMN_STATIC_TEXT,
  N_FOCUS_COLUMNS
};

static BobguiTreeModel *
focus_list_model (void)
{
  BobguiTreeIter   iter;
  BobguiListStore *store = 
    bobgui_list_store_new (N_FOCUS_COLUMNS,
			G_TYPE_STRING,  /* name text */
			G_TYPE_BOOLEAN, /* check */
			G_TYPE_STRING); /* static text */

  bobgui_list_store_append (store, &iter);
  bobgui_list_store_set (store, &iter, 
		      FOCUS_COLUMN_NAME, "Enter a string",
		      FOCUS_COLUMN_CHECK, TRUE,
		      FOCUS_COLUMN_STATIC_TEXT, "Does it fly ?",
		      -1);

  bobgui_list_store_append (store, &iter);
  bobgui_list_store_set (store, &iter, 
		      FOCUS_COLUMN_NAME, "Enter a string",
		      FOCUS_COLUMN_CHECK, FALSE,
		      FOCUS_COLUMN_STATIC_TEXT, "Would you put it in a toaster ?",
		      -1);

  bobgui_list_store_append (store, &iter);
  bobgui_list_store_set (store, &iter, 
		      FOCUS_COLUMN_NAME, "Type something",
		      FOCUS_COLUMN_CHECK, FALSE,
		      FOCUS_COLUMN_STATIC_TEXT, "Does it feed on cute kittens ?",
		      -1);

  return (BobguiTreeModel *)store;
}

static void
cell_toggled (BobguiCellRendererToggle *cell_renderer,
	      const char            *path,
	      BobguiIconView      *iconview)
{
  BobguiTreeModel *model = bobgui_icon_view_get_model (iconview);
  BobguiTreeIter   iter;
  gboolean      active;

  g_print ("Cell toggled !\n");

  if (!bobgui_tree_model_get_iter_from_string (model, &iter, path))
    return;

  bobgui_tree_model_get (model, &iter, FOCUS_COLUMN_CHECK, &active, -1);
  bobgui_list_store_set (BOBGUI_LIST_STORE (model), &iter, FOCUS_COLUMN_CHECK, !active, -1);
}

static void
cell_edited (BobguiCellRendererToggle *cell_renderer,
	     const char            *path,
	     const char            *new_text,
	     BobguiIconView      *iconview)
{
  BobguiTreeModel *model = bobgui_icon_view_get_model (iconview);
  BobguiTreeIter   iter;

  g_print ("Cell edited with new text '%s' !\n", new_text);

  if (!bobgui_tree_model_get_iter_from_string (model, &iter, path))
    return;

  bobgui_list_store_set (BOBGUI_LIST_STORE (model), &iter, FOCUS_COLUMN_NAME, new_text, -1);
}

static BobguiWidget *
focus_iconview (gboolean color_bg, BobguiCellRenderer **focus, BobguiCellRenderer **sibling)
{
  BobguiTreeModel *model;
  BobguiWidget *iconview;
  BobguiCellArea *area;
  BobguiCellRenderer *renderer, *toggle;

  iconview = bobgui_icon_view_new ();

  model = focus_list_model ();

  bobgui_icon_view_set_model (BOBGUI_ICON_VIEW (iconview), model);
  bobgui_icon_view_set_item_orientation (BOBGUI_ICON_VIEW (iconview), BOBGUI_ORIENTATION_HORIZONTAL);

  area = bobgui_cell_layout_get_area (BOBGUI_CELL_LAYOUT (iconview));

  renderer = bobgui_cell_renderer_text_new ();
  g_object_set (G_OBJECT (renderer), "editable", TRUE, NULL);
  bobgui_cell_area_box_pack_start (BOBGUI_CELL_AREA_BOX (area), renderer, TRUE, FALSE, FALSE);
  bobgui_cell_area_attribute_connect (area, renderer, "text", FOCUS_COLUMN_NAME);

  if (color_bg)
    g_object_set (G_OBJECT (renderer), "cell-background", "red", NULL);

  g_signal_connect (G_OBJECT (renderer), "edited",
		    G_CALLBACK (cell_edited), iconview);

  toggle = renderer = bobgui_cell_renderer_toggle_new ();
  g_object_set (G_OBJECT (renderer), "xalign", 0.0F, NULL);
  bobgui_cell_area_box_pack_start (BOBGUI_CELL_AREA_BOX (area), renderer, FALSE, TRUE, FALSE);
  bobgui_cell_area_attribute_connect (area, renderer, "active", FOCUS_COLUMN_CHECK);

  if (color_bg)
    g_object_set (G_OBJECT (renderer), "cell-background", "green", NULL);

  if (focus)
    *focus = renderer;

  g_signal_connect (G_OBJECT (renderer), "toggled",
		    G_CALLBACK (cell_toggled), iconview);

  renderer = bobgui_cell_renderer_text_new ();
  g_object_set (G_OBJECT (renderer), 
		"wrap-mode", PANGO_WRAP_WORD,
		"wrap-width", 150,
		NULL);

  if (color_bg)
    g_object_set (G_OBJECT (renderer), "cell-background", "blue", NULL);

  if (sibling)
    *sibling = renderer;

  bobgui_cell_area_box_pack_start (BOBGUI_CELL_AREA_BOX (area), renderer, FALSE, TRUE, FALSE);
  bobgui_cell_area_attribute_connect (area, renderer, "text", FOCUS_COLUMN_STATIC_TEXT);

  bobgui_cell_area_add_focus_sibling (area, toggle, renderer);

  return iconview;
}

static void
focus_sibling_toggled (BobguiCheckButton  *toggle,
		       BobguiIconView *iconview)
{
  BobguiCellArea *area = bobgui_cell_layout_get_area (BOBGUI_CELL_LAYOUT (iconview));
  gboolean     active = bobgui_check_button_get_active (toggle);

  if (active)
    bobgui_cell_area_add_focus_sibling (area, focus_renderer, sibling_renderer);
  else
    bobgui_cell_area_remove_focus_sibling (area, focus_renderer, sibling_renderer);

  bobgui_widget_queue_draw (BOBGUI_WIDGET (iconview));
}


static void
focus_cell_area (void)
{
  BobguiWidget *window, *widget;
  BobguiWidget *iconview, *frame, *vbox, *hbox;

  window = bobgui_window_new ();
  hbox  = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 4);

  bobgui_window_set_title (BOBGUI_WINDOW (window), "Focus and editable cells");

  iconview = focus_iconview (FALSE, &focus_renderer, &sibling_renderer);

  frame = bobgui_frame_new (NULL);
  bobgui_widget_set_hexpand (frame, TRUE);

  bobgui_widget_set_valign (frame, BOBGUI_ALIGN_CENTER);
  bobgui_widget_set_halign (frame, BOBGUI_ALIGN_FILL);

  bobgui_frame_set_child (BOBGUI_FRAME (frame), iconview);

  /* Now add some controls */
  vbox  = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 4);
  bobgui_box_append (BOBGUI_BOX (hbox), vbox);
  bobgui_box_append (BOBGUI_BOX (hbox), frame);

  widget = bobgui_combo_box_text_new ();
  bobgui_combo_box_text_append_text (BOBGUI_COMBO_BOX_TEXT (widget), "Horizontal");
  bobgui_combo_box_text_append_text (BOBGUI_COMBO_BOX_TEXT (widget), "Vertical");
  bobgui_combo_box_set_active (BOBGUI_COMBO_BOX (widget), 0);
  bobgui_box_append (BOBGUI_BOX (vbox), widget);

  g_signal_connect (G_OBJECT (widget), "changed",
                    G_CALLBACK (orientation_changed), iconview);

  widget = bobgui_check_button_new_with_label ("Focus Sibling");
  bobgui_check_button_set_active (BOBGUI_CHECK_BUTTON (widget), TRUE);
  bobgui_box_append (BOBGUI_BOX (vbox), widget);

  g_signal_connect (G_OBJECT (widget), "toggled",
                    G_CALLBACK (focus_sibling_toggled), iconview);

  bobgui_window_set_child (BOBGUI_WINDOW (window), hbox);

  bobgui_window_present (BOBGUI_WINDOW (window));
}



/*******************************************************
 *                  Background Area                    *
 *******************************************************/
static void
cell_spacing_changed (BobguiSpinButton    *spin_button,
		      BobguiIconView *iconview)
{
  BobguiCellArea *area = bobgui_cell_layout_get_area (BOBGUI_CELL_LAYOUT (iconview));
  int         value;

  value = (int)bobgui_spin_button_get_value (spin_button);

  bobgui_cell_area_box_set_spacing (BOBGUI_CELL_AREA_BOX (area), value);
}

static void
row_spacing_changed (BobguiSpinButton    *spin_button,
		     BobguiIconView *iconview)
{
  int value;

  value = (int)bobgui_spin_button_get_value (spin_button);

  bobgui_icon_view_set_row_spacing (iconview, value);
}

static void
item_padding_changed (BobguiSpinButton    *spin_button,
		     BobguiIconView *iconview)
{
  int value;

  value = (int)bobgui_spin_button_get_value (spin_button);

  bobgui_icon_view_set_item_padding (iconview, value);
}

static void
background_area (void)
{
  BobguiWidget *window, *widget, *label, *main_vbox;
  BobguiWidget *iconview, *frame, *vbox, *hbox;

  window = bobgui_window_new ();
  hbox  = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 4);
  main_vbox  = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 4);
  bobgui_window_set_child (BOBGUI_WINDOW (window), main_vbox);

  bobgui_window_set_title (BOBGUI_WINDOW (window), "Background Area");

  label = bobgui_label_new ("In this example, row spacing gets divided into the background area, "
			 "column spacing is added between each background area, item_padding is "
			 "prepended space distributed to the background area.");
  bobgui_label_set_wrap (BOBGUI_LABEL (label), TRUE);
  bobgui_label_set_width_chars (BOBGUI_LABEL (label), 40);
  bobgui_box_append (BOBGUI_BOX (main_vbox), label);

  iconview = focus_iconview (TRUE, NULL, NULL);

  frame = bobgui_frame_new (NULL);
  bobgui_widget_set_hexpand (frame, TRUE);

  bobgui_widget_set_valign (frame, BOBGUI_ALIGN_CENTER);
  bobgui_widget_set_halign (frame, BOBGUI_ALIGN_FILL);

  bobgui_frame_set_child (BOBGUI_FRAME (frame), iconview);

  /* Now add some controls */
  vbox  = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 4);
  bobgui_box_append (BOBGUI_BOX (hbox), vbox);
  bobgui_box_append (BOBGUI_BOX (hbox), frame);

  bobgui_box_append (BOBGUI_BOX (main_vbox), hbox);

  widget = bobgui_combo_box_text_new ();
  bobgui_combo_box_text_append_text (BOBGUI_COMBO_BOX_TEXT (widget), "Horizontal");
  bobgui_combo_box_text_append_text (BOBGUI_COMBO_BOX_TEXT (widget), "Vertical");
  bobgui_combo_box_set_active (BOBGUI_COMBO_BOX (widget), 0);
  bobgui_box_append (BOBGUI_BOX (vbox), widget);

  g_signal_connect (G_OBJECT (widget), "changed",
                    G_CALLBACK (orientation_changed), iconview);

  widget = bobgui_spin_button_new_with_range (0, 10, 1);
  label = bobgui_label_new ("Cell spacing");
  bobgui_widget_set_hexpand (label, TRUE);
  hbox = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 4);
  bobgui_box_append (BOBGUI_BOX (hbox), label);
  bobgui_box_append (BOBGUI_BOX (hbox), widget);
  bobgui_box_append (BOBGUI_BOX (vbox), hbox);

  g_signal_connect (G_OBJECT (widget), "value-changed",
                    G_CALLBACK (cell_spacing_changed), iconview);


  widget = bobgui_spin_button_new_with_range (0, 10, 1);
  bobgui_spin_button_set_value (BOBGUI_SPIN_BUTTON (widget), bobgui_icon_view_get_row_spacing (BOBGUI_ICON_VIEW (iconview)));
  label = bobgui_label_new ("Row spacing");
  bobgui_widget_set_hexpand (label, TRUE);
  hbox = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 4);
  bobgui_box_append (BOBGUI_BOX (hbox), label);
  bobgui_box_append (BOBGUI_BOX (hbox), widget);
  bobgui_box_append (BOBGUI_BOX (vbox), hbox);

  g_signal_connect (G_OBJECT (widget), "value-changed",
                    G_CALLBACK (row_spacing_changed), iconview);

  widget = bobgui_spin_button_new_with_range (0, 30, 1);
  label = bobgui_label_new ("Item padding");
  bobgui_widget_set_hexpand (label, TRUE);
  bobgui_spin_button_set_value (BOBGUI_SPIN_BUTTON (widget), bobgui_icon_view_get_item_padding (BOBGUI_ICON_VIEW (iconview)));
  hbox = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 4);
  bobgui_box_append (BOBGUI_BOX (hbox), label);
  bobgui_box_append (BOBGUI_BOX (hbox), widget);
  bobgui_box_append (BOBGUI_BOX (vbox), hbox);

  g_signal_connect (G_OBJECT (widget), "value-changed",
                    G_CALLBACK (item_padding_changed), iconview);

  bobgui_window_present (BOBGUI_WINDOW (window));
}






int
main (int argc, char *argv[])
{
  bobgui_init ();

  if (g_getenv ("RTL"))
    bobgui_widget_set_default_direction (BOBGUI_TEXT_DIR_RTL);

  simple_cell_area ();
  focus_cell_area ();
  background_area ();

  while (TRUE)
    g_main_context_iteration (NULL, TRUE);

  return 0;
}
