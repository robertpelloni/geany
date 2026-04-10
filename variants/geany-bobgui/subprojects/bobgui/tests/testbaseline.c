/*
 * Copyright (C) 2006 Nokia Corporation.
 * Author: Xan Lopez <xan.lopez@nokia.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * version 2.1 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <bobgui/bobgui.h>

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

static const char *baseline_pos_str[] = {
  "BASELINE_POSITION_TOP",
  "BASELINE_POSITION_CENTER",
  "BASELINE_POSITION_BOTTOM"
};

static void
baseline_row_value_changed (BobguiSpinButton *spin_button,
			    BobguiGrid *grid)
{
  int row = bobgui_spin_button_get_value_as_int (spin_button);

  bobgui_grid_set_baseline_row (grid, row);
}

static void
homogeneous_changed (BobguiToggleButton *toggle_button,
		    BobguiGrid *grid)
{
  bobgui_grid_set_row_homogeneous (grid, bobgui_toggle_button_get_active (toggle_button));
}

static void
baseline_position_changed (BobguiComboBox *combo,
			   BobguiBox *hbox)
{
  int i = bobgui_combo_box_get_active (combo);

  bobgui_box_set_baseline_position (hbox, i);
}

static void
image_size_value_changed (BobguiSpinButton *spin_button,
			  BobguiImage *image)
{
  int size = bobgui_spin_button_get_value_as_int (spin_button);

  bobgui_image_set_pixel_size (BOBGUI_IMAGE (image), size);
}

static void
set_font_size (BobguiWidget *widget, int size)
{
  const char *class[3] = { "small-font", "medium-font", "large-font" };

  bobgui_widget_add_css_class (widget, class[size]);
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
main (int    argc,
      char **argv)
{
  BobguiWidget *window, *label, *entry, *button, *grid, *notebook;
  BobguiWidget *vbox, *hbox, *grid_hbox, *spin, *spin2, *toggle, *combo, *image;
  BobguiAdjustment *adjustment;
  int i, j;
  BobguiCssProvider *provider;
  gboolean done = FALSE;
  BobguiWidget *group = NULL;

  bobgui_init ();

  provider = bobgui_css_provider_new ();
  bobgui_css_provider_load_from_data (provider,
    ".small-font { font-size: 5px; }"
    ".medium-font { font-size: 10px; }"
    ".large-font { font-size: 15px; }", -1);
  bobgui_style_context_add_provider_for_display (gdk_display_get_default (),
                                              BOBGUI_STYLE_PROVIDER (provider),
                                              BOBGUI_STYLE_PROVIDER_PRIORITY_APPLICATION);
  g_object_unref (provider);
  window = bobgui_window_new ();
  g_signal_connect (G_OBJECT (window), "destroy", G_CALLBACK (quit_cb), &done);

  notebook = bobgui_notebook_new ();
  bobgui_window_set_child (BOBGUI_WINDOW (window), notebook);

  vbox = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 5);
  bobgui_notebook_append_page (BOBGUI_NOTEBOOK (notebook),
			    vbox, bobgui_label_new ("hboxes"));

  for (j = 0; j < 2; j++)
    {
      hbox = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 10);
      bobgui_box_append (BOBGUI_BOX (vbox), hbox);

      const char *aligns_names[] = { "FILL", "BASELINE" };
      BobguiAlign aligns[] = { BOBGUI_ALIGN_FILL, BOBGUI_ALIGN_BASELINE_FILL};

      label = bobgui_label_new (aligns_names[j]);
      bobgui_box_append (BOBGUI_BOX (hbox), label);

      for (i = 0; i < 3; i++) {
	label = bobgui_label_new ("│XYyj,Ö...");

        set_font_size (label, i);

	bobgui_widget_set_valign (label, aligns[j]);

	bobgui_box_append (BOBGUI_BOX (hbox), label);
      }

      for (i = 0; i < 3; i++) {
	entry = bobgui_entry_new ();
	bobgui_editable_set_text (BOBGUI_EDITABLE (entry), "│XYyj,Ö...");

        set_font_size (entry, i);

	bobgui_widget_set_valign (entry, aligns[j]);

	bobgui_box_append (BOBGUI_BOX (hbox), entry);
      }

      spin = bobgui_spin_button_new (NULL, 0, 1);
      bobgui_orientable_set_orientation (BOBGUI_ORIENTABLE (spin), BOBGUI_ORIENTATION_VERTICAL);
      bobgui_widget_set_valign (spin, aligns[j]);
      bobgui_box_append (BOBGUI_BOX (hbox), spin);

      spin = bobgui_spin_button_new (NULL, 0, 1);
      bobgui_widget_set_valign (spin, aligns[j]);
      bobgui_box_append (BOBGUI_BOX (hbox), spin);
    }

  grid_hbox = hbox = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 10);
  bobgui_box_append (BOBGUI_BOX (vbox), hbox);

  combo = bobgui_combo_box_text_new ();
  bobgui_combo_box_text_append_text (BOBGUI_COMBO_BOX_TEXT (combo), baseline_pos_str[0]);
  bobgui_combo_box_text_append_text (BOBGUI_COMBO_BOX_TEXT (combo), baseline_pos_str[1]);
  bobgui_combo_box_text_append_text (BOBGUI_COMBO_BOX_TEXT (combo), baseline_pos_str[2]);
  bobgui_combo_box_set_active (BOBGUI_COMBO_BOX (combo), 1);
  bobgui_box_append (BOBGUI_BOX (hbox), combo);

  for (j = 0; j < 2; j++)
    {
      hbox = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 10);
      bobgui_box_append (BOBGUI_BOX (vbox), hbox);

      g_signal_connect (G_OBJECT (combo), "changed",
			G_CALLBACK (baseline_position_changed), hbox);

      if (j == 0)
	label = bobgui_label_new ("Baseline:");
      else
	label = bobgui_label_new ("Normal:");
      bobgui_box_append (BOBGUI_BOX (hbox), label);

      for (i = 0; i < 3; i++)
	{
	  button = bobgui_button_new_with_label ("│Xyj,Ö");

          set_font_size (button, i);

	  if (j == 0)
	    bobgui_widget_set_valign (button, BOBGUI_ALIGN_BASELINE_FILL);

	  bobgui_box_append (BOBGUI_BOX (hbox), button);
	}

      for (i = 0; i < 3; i++)
	{
          BobguiWidget *box = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL,  6);
          button = bobgui_button_new ();

          bobgui_box_append (BOBGUI_BOX (box), bobgui_label_new ("│Xyj,Ö"));
          bobgui_box_append (BOBGUI_BOX (box), bobgui_image_new_from_icon_name ("face-sad"));
          bobgui_button_set_child (BOBGUI_BUTTON (button), box);

          set_font_size (button, i);

	  if (j == 0)
	    bobgui_widget_set_valign (button, BOBGUI_ALIGN_BASELINE_FILL);

	  bobgui_box_append (BOBGUI_BOX (hbox), button);
	}

      image = bobgui_image_new_from_icon_name ("face-sad");
      bobgui_image_set_pixel_size (BOBGUI_IMAGE (image), 34);
      if (j == 0)
	bobgui_widget_set_valign (image, BOBGUI_ALIGN_BASELINE_FILL);
      bobgui_box_append (BOBGUI_BOX (hbox), image);

      button = bobgui_toggle_button_new_with_label ("│Xyj,Ö");
      if (j == 0)
	bobgui_widget_set_valign (button, BOBGUI_ALIGN_BASELINE_FILL);
      bobgui_box_append (BOBGUI_BOX (hbox), button);

      button = bobgui_toggle_button_new_with_label ("│Xyj,Ö");
      if (j == 0)
	bobgui_widget_set_valign (button, BOBGUI_ALIGN_BASELINE_FILL);
      bobgui_box_append (BOBGUI_BOX (hbox), button);

      button = bobgui_check_button_new_with_label ("│Xyj,Ö");
      if (j == 0)
	bobgui_widget_set_valign (button, BOBGUI_ALIGN_BASELINE_FILL);
      bobgui_box_append (BOBGUI_BOX (hbox), button);

      button = bobgui_check_button_new_with_label ("│Xyj,Ö");
      if (j == 0)
	bobgui_widget_set_valign (button, BOBGUI_ALIGN_BASELINE_FILL);
      bobgui_box_append (BOBGUI_BOX (hbox), button);
      if (group == NULL)
        {
          group = button;
          bobgui_check_button_set_active (BOBGUI_CHECK_BUTTON (button), TRUE);
        }
      else
        bobgui_check_button_set_group (BOBGUI_CHECK_BUTTON (button),
                                    BOBGUI_CHECK_BUTTON (group));
    }


  vbox = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 0);
  bobgui_notebook_append_page (BOBGUI_NOTEBOOK (notebook),
			    vbox, bobgui_label_new ("grid"));

  grid_hbox = hbox = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 10);
  bobgui_box_append (BOBGUI_BOX (vbox), hbox);

  label = bobgui_label_new ("Align me:");
  bobgui_widget_set_valign (label, BOBGUI_ALIGN_BASELINE_FILL);

  bobgui_box_append (BOBGUI_BOX (hbox), label);

  grid = bobgui_grid_new ();
  bobgui_widget_set_valign (grid, BOBGUI_ALIGN_BASELINE_FILL);
  bobgui_grid_set_column_spacing (BOBGUI_GRID (grid), 8);
  bobgui_grid_set_row_spacing (BOBGUI_GRID (grid), 8);

  for (j = 0; j < 4; j++)
    {
      const char *labels[] = { "Normal:", "Baseline (top):", "Baseline (center):", "Baseline (bottom):"};
      label = bobgui_label_new (labels[j]);

      bobgui_grid_attach (BOBGUI_GRID (grid),
		       label,
		       0, j,
		       1, 1);
      bobgui_widget_set_vexpand (label, TRUE);

      if (j != 0)
	bobgui_grid_set_row_baseline_position (BOBGUI_GRID (grid),
					    j, (BobguiBaselinePosition)(j-1));

      for (i = 0; i < 3; i++)
	{
	  label = bobgui_label_new ("Xyjg,Ö.");

          set_font_size (label, i);

	  if (j != 0)
	    bobgui_widget_set_valign (label, BOBGUI_ALIGN_BASELINE_FILL);

	  bobgui_grid_attach (BOBGUI_GRID (grid),
			   label,
			   i+1, j,
			   1, 1);
	}

      for (i = 0; i < 3; i++)
	{
          BobguiWidget *box = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL,  6);
          button = bobgui_button_new ();

          bobgui_box_append (BOBGUI_BOX (box), bobgui_label_new ("│Xyj,Ö"));
          bobgui_box_append (BOBGUI_BOX (box), bobgui_image_new_from_icon_name ("face-sad"));
          bobgui_button_set_child (BOBGUI_BUTTON (button), box);

          set_font_size (button, i);

	  if (j != 0)
	    bobgui_widget_set_valign (button, BOBGUI_ALIGN_BASELINE_FILL);

	  bobgui_grid_attach (BOBGUI_GRID (grid),
			   button,
			   i+4, j,
			   1, 1);
	}

    }

  bobgui_box_append (BOBGUI_BOX (hbox), grid);

  hbox = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 10);
  bobgui_box_append (BOBGUI_BOX (vbox), hbox);

  adjustment = bobgui_adjustment_new (0.0, -1.0, 5.0, 1.0, 1.0, 0.0);
  spin = bobgui_spin_button_new (adjustment, 1.0, 0);
  g_signal_connect (spin, "value-changed", (GCallback)baseline_row_value_changed, grid);
  bobgui_box_append (BOBGUI_BOX (hbox), spin);

  toggle = bobgui_toggle_button_new_with_label ("Homogeneous");
  g_signal_connect (toggle, "toggled", (GCallback)homogeneous_changed, grid);
  bobgui_box_append (BOBGUI_BOX (hbox), toggle);

  combo = bobgui_combo_box_text_new ();
  bobgui_combo_box_text_append_text (BOBGUI_COMBO_BOX_TEXT (combo), baseline_pos_str[0]);
  bobgui_combo_box_text_append_text (BOBGUI_COMBO_BOX_TEXT (combo), baseline_pos_str[1]);
  bobgui_combo_box_text_append_text (BOBGUI_COMBO_BOX_TEXT (combo), baseline_pos_str[2]);
  bobgui_combo_box_set_active (BOBGUI_COMBO_BOX (combo), 1);
  g_signal_connect (G_OBJECT (combo), "changed",
		    G_CALLBACK (baseline_position_changed), grid_hbox);
  bobgui_box_append (BOBGUI_BOX (hbox), combo);

  vbox = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 0);
  bobgui_notebook_append_page (BOBGUI_NOTEBOOK (notebook),
			    vbox, bobgui_label_new ("button box"));

  hbox = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 10);
  bobgui_box_append (BOBGUI_BOX (vbox), hbox);

  adjustment = bobgui_adjustment_new (34.0, 1.0, 64.0, 1.0, 1.0, 0.0);
  spin = bobgui_spin_button_new (adjustment, 1.0, 0);
  bobgui_box_append (BOBGUI_BOX (hbox), spin);

  adjustment = bobgui_adjustment_new (16.0, 1.0, 64.0, 1.0, 1.0, 0.0);
  spin2 = bobgui_spin_button_new (adjustment, 1.0, 0);
  bobgui_box_append (BOBGUI_BOX (hbox), spin2);

  for (j = 0; j < 3; j++)
    {
      hbox = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 0);
      bobgui_box_append (BOBGUI_BOX (vbox), hbox);

      bobgui_box_set_baseline_position (BOBGUI_BOX (hbox), j);

      label = bobgui_label_new (baseline_pos_str[j]);
      bobgui_box_append (BOBGUI_BOX (hbox), label);
      bobgui_widget_set_vexpand (label, TRUE);

      image = bobgui_image_new_from_icon_name ("face-sad");
      bobgui_image_set_pixel_size (BOBGUI_IMAGE (image), 34);
      bobgui_box_append (BOBGUI_BOX (hbox), image);

      g_signal_connect (spin, "value-changed", (GCallback)image_size_value_changed, image);

      for (i = 0; i < 3; i++)
	{
	  button = bobgui_button_new_with_label ("│Xyj,Ö");

          set_font_size (button, i);

	  if (i != 0)
	    bobgui_widget_set_valign (button, BOBGUI_ALIGN_BASELINE_FILL);

	  bobgui_box_append (BOBGUI_BOX (hbox), button);
	}

      for (i = 0; i < 3; i++)
	{
          BobguiWidget *box = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL,  6);
          button = bobgui_button_new ();

          bobgui_box_append (BOBGUI_BOX (box), bobgui_label_new ("│Xyj,Ö"));
          image = bobgui_image_new_from_icon_name ("face-sad");
          bobgui_image_set_pixel_size (BOBGUI_IMAGE (image), 16);
          bobgui_box_append (BOBGUI_BOX (box), image);
          bobgui_button_set_child (BOBGUI_BUTTON (button), box);

	  if (i == 0)
	    g_signal_connect (spin2, "value-changed", (GCallback)image_size_value_changed, image);

          set_font_size (button, i);

	  bobgui_widget_set_valign (button, BOBGUI_ALIGN_BASELINE_FILL);

	  bobgui_box_append (BOBGUI_BOX (hbox), button);
	}
    }

  bobgui_window_present (BOBGUI_WINDOW (window));

  while (!done)
    g_main_context_iteration (NULL, TRUE);

  return 0;
}
