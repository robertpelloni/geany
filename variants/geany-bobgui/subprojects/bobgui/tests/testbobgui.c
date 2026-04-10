  /* BOBGUI - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * Modified by the BOBGUI+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the BOBGUI+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * BOBGUI+ at ftp://ftp.bobgui.org/pub/bobgui/. 
 */


#include "config.h"

#undef	G_LOG_DOMAIN

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <math.h>
#include <time.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <glib/gstdio.h>

#include "bobgui/bobgui.h"
#include "gdk/gdk.h"
#include "gdk/gdkkeysyms.h"

#ifdef G_OS_WIN32
#define sleep(n) _sleep(n)
#endif

#include "test.xpm"

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

static gboolean done = FALSE;

static gboolean
file_exists (const char *filename)
{
  struct stat statbuf;

  return stat (filename, &statbuf) == 0;
}

static BobguiWidget *
build_option_menu (const char           *items[],
		   int              num_items,
		   int              history,
		   void           (*func) (BobguiWidget *widget, gpointer data),
		   gpointer         data);

/* macro, structure and variables used by tree window demos */
#define DEFAULT_NUMBER_OF_ITEM  3
#define DEFAULT_RECURSION_LEVEL 3

struct {
  GSList* selection_mode_group;
  BobguiWidget* single_button;
  BobguiWidget* browse_button;
  BobguiWidget* multiple_button;
  BobguiWidget* draw_line_button;
  BobguiWidget* view_line_button;
  BobguiWidget* no_root_item_button;
  BobguiWidget* nb_item_spinner;
  BobguiWidget* recursion_spinner;
} sTreeSampleSelection;

typedef struct sTreeButtons {
  guint nb_item_add;
  BobguiWidget* add_button;
  BobguiWidget* remove_button;
  BobguiWidget* subtree_button;
} sTreeButtons;
/* end of tree section */

static BobguiWidget *
build_option_menu (const char           *items[],
		   int              num_items,
		   int              history,
		   void           (*func)(BobguiWidget *widget, gpointer data),
		   gpointer         data)
{
  BobguiWidget *omenu;
  int i;

  omenu = bobgui_combo_box_text_new ();
  g_signal_connect (omenu, "changed",
		    G_CALLBACK (func), data);
      
  for (i = 0; i < num_items; i++)
      bobgui_combo_box_text_append_text (BOBGUI_COMBO_BOX_TEXT (omenu), items[i]);

  bobgui_combo_box_set_active (BOBGUI_COMBO_BOX (omenu), history);
  
  return omenu;
}

/*
 * Windows with an alpha channel
 */
static BobguiWidget *
build_alpha_widgets (void)
{
  BobguiWidget *grid;
  BobguiWidget *radio_button;
  BobguiWidget *check_button;
  BobguiWidget *hbox;
  BobguiWidget *label;
  BobguiWidget *entry;
  BobguiWidget *group;

  grid = bobgui_grid_new ();
  bobgui_widget_set_vexpand (grid, TRUE);

  radio_button = bobgui_check_button_new_with_label ("Red");
  bobgui_widget_set_hexpand (radio_button, TRUE);
  bobgui_grid_attach (BOBGUI_GRID (grid), radio_button, 0, 0, 1, 1);
  group = radio_button;

  radio_button = bobgui_check_button_new_with_label ("Green");
  bobgui_widget_set_hexpand (radio_button, TRUE);
  bobgui_grid_attach (BOBGUI_GRID (grid), radio_button, 0, 1, 1, 1);
  bobgui_check_button_set_group (BOBGUI_CHECK_BUTTON (radio_button), BOBGUI_CHECK_BUTTON (group));

  radio_button = bobgui_check_button_new_with_label ("Blue"),
  bobgui_widget_set_hexpand (radio_button, TRUE);
  bobgui_grid_attach (BOBGUI_GRID (grid), radio_button, 0, 2, 1, 1);
  bobgui_check_button_set_group (BOBGUI_CHECK_BUTTON (radio_button), BOBGUI_CHECK_BUTTON (group));
  bobgui_check_button_set_active (BOBGUI_CHECK_BUTTON (group), TRUE);

  check_button = bobgui_check_button_new_with_label ("Sedentary"),
  bobgui_widget_set_hexpand (check_button, TRUE);
  bobgui_grid_attach (BOBGUI_GRID (grid), check_button, 1, 0, 1, 1);

  check_button = bobgui_check_button_new_with_label ("Nocturnal"),
  bobgui_widget_set_hexpand (check_button, TRUE);
  bobgui_grid_attach (BOBGUI_GRID (grid), check_button, 1, 1, 1, 1);

  check_button = bobgui_check_button_new_with_label ("Compulsive"),
  bobgui_widget_set_hexpand (check_button, TRUE);
  bobgui_grid_attach (BOBGUI_GRID (grid), check_button, 1, 2, 1, 1);

  hbox = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 0);
  label = bobgui_label_new (NULL);
  bobgui_label_set_markup (BOBGUI_LABEL (label), "<i>Entry: </i>");
  bobgui_box_append (BOBGUI_BOX (hbox), label);
  entry = bobgui_entry_new ();
  bobgui_widget_set_hexpand (entry, TRUE);
  bobgui_box_append (BOBGUI_BOX (hbox), entry);
  bobgui_widget_set_hexpand (hbox, TRUE);
  bobgui_grid_attach (BOBGUI_GRID (grid), hbox, 0, 3, 2, 1);

  return grid;
}

static void
on_composited_changed (GdkDisplay *display,
                       GParamSpec *pspec,
                       BobguiLabel   *label)
{
  gboolean is_composited = gdk_display_is_composited (display);

  if (is_composited)
    bobgui_label_set_text (label, "Composited");
  else
    bobgui_label_set_text (label, "Not composited");

  /* We draw a different background on the GdkSurface */
  bobgui_widget_queue_draw (BOBGUI_WIDGET (bobgui_widget_get_root (BOBGUI_WIDGET (label))));
}

static void
create_alpha_window (BobguiWidget *widget)
{
  static BobguiWidget *window;

  if (!window)
    {
      static BobguiCssProvider *provider = NULL;
      BobguiWidget *content_area;
      BobguiWidget *vbox;
      BobguiWidget *label;
      GdkDisplay *display;
      
      window = bobgui_dialog_new_with_buttons ("Alpha Window",
					    BOBGUI_WINDOW (bobgui_widget_get_root (widget)), 0,
					    "_Close", 0,
					    NULL);
      bobgui_widget_add_css_class (window, "alpha");
      if (provider == NULL)
        {
          provider = bobgui_css_provider_new ();
          bobgui_css_provider_load_from_data (provider,
                                           "dialog.alpha {\n"
                                           "  background: radial-gradient(ellipse at center, #FFBF00, #FFBF0000);\n"
                                           "}\n",
                                           -1);
          bobgui_style_context_add_provider_for_display (bobgui_widget_get_display (window),
                                                      BOBGUI_STYLE_PROVIDER (provider),
                                                      BOBGUI_STYLE_PROVIDER_PRIORITY_APPLICATION);
          g_object_unref (provider);
        }

      content_area = bobgui_dialog_get_content_area (BOBGUI_DIALOG (window));

      vbox = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 8);
      bobgui_widget_set_margin_top (vbox, 12);
      bobgui_widget_set_margin_bottom (vbox, 12);
      bobgui_widget_set_margin_start (vbox, 12);
      bobgui_widget_set_margin_end (vbox, 12);
      bobgui_widget_set_vexpand (vbox, TRUE);
      label = bobgui_label_new (NULL);
      bobgui_box_append (BOBGUI_BOX (content_area), vbox);
      bobgui_box_append (BOBGUI_BOX (vbox), label);
      display = bobgui_widget_get_display (window);
      on_composited_changed (display, NULL, BOBGUI_LABEL (label));
      g_signal_connect (display, "notify::composited", G_CALLBACK (on_composited_changed), label);

      bobgui_box_append (BOBGUI_BOX (vbox), build_alpha_widgets ());
      g_object_add_weak_pointer (G_OBJECT (window), (gpointer *)&window);

      g_signal_connect (window, "response",
                        G_CALLBACK (bobgui_window_destroy),
                        NULL);
    }

  if (!bobgui_widget_get_visible (window))
    bobgui_window_present (BOBGUI_WINDOW (window));
  else
    bobgui_window_destroy (BOBGUI_WINDOW (window));
}

/*
 * BobguiButton
 */

static void
button_window (BobguiWidget *widget,
              BobguiWidget *button)
{
  bobgui_widget_set_visible (button, !bobgui_widget_get_visible (button));
}

static void
create_buttons (BobguiWidget *widget)
{
  static BobguiWidget *window = NULL;
  BobguiWidget *box1;
  BobguiWidget *box2;
  BobguiWidget *grid;
  BobguiWidget *separator;
  BobguiWidget *button[10];
  int button_x[9] = { 0, 1, 2, 0, 2, 1, 1, 2, 0 };
  int button_y[9] = { 0, 1, 2, 2, 0, 2, 0, 1, 1 };
  guint i;

  if (!window)
    {
      window = bobgui_window_new ();
      bobgui_window_set_display (BOBGUI_WINDOW (window),
                              bobgui_widget_get_display (widget));
      g_object_add_weak_pointer (G_OBJECT (window), (gpointer *)&window);

      bobgui_window_set_title (BOBGUI_WINDOW (window), "BobguiButton");

      box1 = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 0);
      bobgui_window_set_child (BOBGUI_WINDOW (window), box1);

      grid = bobgui_grid_new ();
      bobgui_grid_set_row_spacing (BOBGUI_GRID (grid), 5);
      bobgui_grid_set_column_spacing (BOBGUI_GRID (grid), 5);

      bobgui_widget_set_margin_top (grid, 10);
      bobgui_widget_set_margin_bottom (grid, 10);
      bobgui_widget_set_margin_start (grid, 10);
      bobgui_widget_set_margin_end (grid, 10);
      bobgui_box_append (BOBGUI_BOX (box1), grid);

      button[0] = bobgui_button_new_with_label ("button1");
      button[1] = bobgui_button_new_with_mnemonic ("_button2");
      button[2] = bobgui_button_new_with_mnemonic ("_button3");
      button[3] = bobgui_button_new_with_mnemonic ("_button4");
      button[4] = bobgui_button_new_with_label ("button5");
      button[5] = bobgui_button_new_with_label ("button6");
      button[6] = bobgui_button_new_with_label ("button7");
      button[7] = bobgui_button_new_with_label ("button8");
      button[8] = bobgui_button_new_with_label ("button9");

      for (i = 0; i < 9; i++)
        {
          g_signal_connect (button[i], "clicked",
                            G_CALLBACK (button_window),
                            button[(i + 1) % 9]);
          bobgui_widget_set_hexpand (button[i], TRUE);
          bobgui_widget_set_vexpand (button[i], TRUE);

          bobgui_grid_attach (BOBGUI_GRID (grid), button[i],
                           button_x[i], button_y[i] + 1, 1, 1);
        }

      separator = bobgui_separator_new (BOBGUI_ORIENTATION_HORIZONTAL);
      bobgui_box_append (BOBGUI_BOX (box1), separator);

      box2 = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 10);
      bobgui_box_append (BOBGUI_BOX (box1), box2);

      button[9] = bobgui_button_new_with_label ("close");
      bobgui_widget_set_margin_top (button[9], 10);
      bobgui_widget_set_margin_bottom (button[9], 10);
      bobgui_widget_set_margin_start (button[9], 10);
      bobgui_widget_set_margin_end (button[9], 10);
      g_signal_connect_swapped (button[9], "clicked",
				G_CALLBACK (bobgui_window_destroy),
				window);
      bobgui_box_append (BOBGUI_BOX (box2), button[9]);
      bobgui_window_set_default_widget (BOBGUI_WINDOW (window), button[9]);
    }

  if (!bobgui_widget_get_visible (window))
    bobgui_window_present (BOBGUI_WINDOW (window));
  else
    bobgui_window_destroy (BOBGUI_WINDOW (window));
}

/*
 * BobguiToggleButton
 */

static void
create_toggle_buttons (BobguiWidget *widget)
{
  static BobguiWidget *window = NULL;
  BobguiWidget *box1;
  BobguiWidget *box2;
  BobguiWidget *button;
  BobguiWidget *separator;

  if (!window)
    {
      window = bobgui_window_new ();
      bobgui_window_set_display (BOBGUI_WINDOW (window),
                              bobgui_widget_get_display (widget));
      g_object_add_weak_pointer (G_OBJECT (window), (gpointer *)&window);

      bobgui_window_set_title (BOBGUI_WINDOW (window), "BobguiToggleButton");

      box1 = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 0);
      bobgui_window_set_child (BOBGUI_WINDOW (window), box1);

      box2 = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 10);
      bobgui_box_append (BOBGUI_BOX (box1), box2);

      button = bobgui_toggle_button_new_with_label ("button1");
      bobgui_box_append (BOBGUI_BOX (box2), button);

      button = bobgui_toggle_button_new_with_label ("button2");
      bobgui_box_append (BOBGUI_BOX (box2), button);

      button = bobgui_toggle_button_new_with_label ("button3");
      bobgui_box_append (BOBGUI_BOX (box2), button);

      separator = bobgui_separator_new (BOBGUI_ORIENTATION_HORIZONTAL);
      bobgui_box_append (BOBGUI_BOX (box1), separator);

      box2 = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 10);
      bobgui_box_append (BOBGUI_BOX (box1), box2);

      button = bobgui_button_new_with_label ("close");
      g_signal_connect_swapped (button, "clicked",
			        G_CALLBACK (bobgui_window_destroy),
				window);
      bobgui_box_append (BOBGUI_BOX (box2), button);
      bobgui_window_set_default_widget (BOBGUI_WINDOW (window), button);
    }

  if (!bobgui_widget_get_visible (window))
    bobgui_window_present (BOBGUI_WINDOW (window));
  else
    bobgui_window_destroy (BOBGUI_WINDOW (window));
}

static BobguiWidget *
create_widget_grid (gboolean group)
{
  BobguiWidget *grid;
  BobguiWidget *group_widget = NULL;
  int i, j;
  
  grid = bobgui_grid_new ();
  
  for (i = 0; i < 5; i++)
    {
      for (j = 0; j < 5; j++)
	{
	  BobguiWidget *widget;
	  char *tmp;
	  
	  if (i == 0 && j == 0)
	    {
	      widget = NULL;
	    }
	  else if (i == 0)
	    {
	      tmp = g_strdup_printf ("%d", j);
	      widget = bobgui_label_new (tmp);
	      g_free (tmp);
	    }
	  else if (j == 0)
	    {
	      tmp = g_strdup_printf ("%c", 'A' + i - 1);
	      widget = bobgui_label_new (tmp);
	      g_free (tmp);
	    }
	  else
	    {
	      widget = bobgui_check_button_new ();
              if (group)
		{
		  if (!group_widget)
		    group_widget = widget;
		  else
                    bobgui_check_button_set_group (BOBGUI_CHECK_BUTTON (widget), BOBGUI_CHECK_BUTTON (group_widget));
		}
	    }
	  
	  if (widget)
	    bobgui_grid_attach (BOBGUI_GRID (grid), widget, i, j, 1, 1);
	}
    }

  return grid;
}

/*
 * BobguiCheckButton
 */

static void
create_check_buttons (BobguiWidget *widget)
{
  static BobguiWidget *window = NULL;
  BobguiWidget *box1;
  BobguiWidget *box2;
  BobguiWidget *button;
  BobguiWidget *separator;
  BobguiWidget *table;
  
  if (!window)
    {
      window = bobgui_dialog_new_with_buttons ("Check Buttons",
                                            NULL, 0,
                                            "_Close",
                                            BOBGUI_RESPONSE_NONE,
                                            NULL);

      bobgui_window_set_display (BOBGUI_WINDOW (window), 
                              bobgui_widget_get_display (widget));
      g_object_add_weak_pointer (G_OBJECT (window), (gpointer *)&window);
      g_signal_connect (window, "response",
                        G_CALLBACK (bobgui_window_destroy),
                        NULL);

      box1 = bobgui_dialog_get_content_area (BOBGUI_DIALOG (window));

      box2 = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 10);
      bobgui_widget_set_hexpand (box2, TRUE);
      bobgui_widget_set_margin_start (box2, 10);
      bobgui_widget_set_margin_end (box2, 10);
      bobgui_widget_set_margin_top (box2, 10);
      bobgui_widget_set_margin_bottom (box2, 10);
      bobgui_box_append (BOBGUI_BOX (box1), box2);

      button = bobgui_check_button_new_with_mnemonic ("_button1");
      bobgui_box_append (BOBGUI_BOX (box2), button);

      button = bobgui_check_button_new_with_label ("button2");
      bobgui_box_append (BOBGUI_BOX (box2), button);

      button = bobgui_check_button_new_with_label ("button3");
      bobgui_box_append (BOBGUI_BOX (box2), button);

      button = bobgui_check_button_new_with_label ("inconsistent");
      bobgui_check_button_set_inconsistent (BOBGUI_CHECK_BUTTON (button), TRUE);
      bobgui_box_append (BOBGUI_BOX (box2), button);

      separator = bobgui_separator_new (BOBGUI_ORIENTATION_HORIZONTAL);
      bobgui_box_append (BOBGUI_BOX (box1), separator);

      table = create_widget_grid (FALSE);
      bobgui_widget_set_vexpand (table, TRUE);
      bobgui_box_append (BOBGUI_BOX (box2), table);
    }

  if (!bobgui_widget_get_visible (window))
    bobgui_window_present (BOBGUI_WINDOW (window));
  else
    bobgui_window_destroy (BOBGUI_WINDOW (window));
}

static void
create_radio_buttons (BobguiWidget *widget)
{
  static BobguiWidget *window = NULL;
  BobguiWidget *box1;
  BobguiWidget *box2;
  BobguiWidget *button;
  BobguiWidget *separator;
  BobguiWidget *table;
  BobguiWidget *group;

  if (!window)
    {
      window = bobgui_dialog_new_with_buttons ("Radio Buttons",
                                            NULL, 0,
                                            "_Close",
                                            BOBGUI_RESPONSE_NONE,
                                            NULL);

      bobgui_window_set_display (BOBGUI_WINDOW (window),
                              bobgui_widget_get_display (widget));
      g_object_add_weak_pointer (G_OBJECT (window), (gpointer *)&window);
      g_signal_connect (window, "response",
                        G_CALLBACK (bobgui_window_destroy),
                        NULL);

      box1 = bobgui_dialog_get_content_area (BOBGUI_DIALOG (window));

      box2 = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 10);
      bobgui_box_append (BOBGUI_BOX (box1), box2);

      button = bobgui_check_button_new_with_label ("button1");
      bobgui_box_append (BOBGUI_BOX (box2), button);
      group = button;

      button = bobgui_check_button_new_with_label ("button2");
      bobgui_check_button_set_active (BOBGUI_CHECK_BUTTON (button), TRUE);
      bobgui_check_button_set_group (BOBGUI_CHECK_BUTTON (button), BOBGUI_CHECK_BUTTON (group));
      bobgui_box_append (BOBGUI_BOX (box2), button);

      button = bobgui_check_button_new_with_label ("button3");
      bobgui_check_button_set_group (BOBGUI_CHECK_BUTTON (button), BOBGUI_CHECK_BUTTON (group));
      bobgui_box_append (BOBGUI_BOX (box2), button);

      button = bobgui_check_button_new_with_label ("inconsistent");
      bobgui_check_button_set_group (BOBGUI_CHECK_BUTTON (button), BOBGUI_CHECK_BUTTON (group));
      bobgui_check_button_set_inconsistent (BOBGUI_CHECK_BUTTON (button), TRUE);
      bobgui_box_append (BOBGUI_BOX (box2), button);

      separator = bobgui_separator_new (BOBGUI_ORIENTATION_HORIZONTAL);
      bobgui_box_append (BOBGUI_BOX (box1), separator);

      box2 = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 10);
      bobgui_box_append (BOBGUI_BOX (box1), box2);

      button = bobgui_check_button_new_with_label ("button4");
      bobgui_box_append (BOBGUI_BOX (box2), button);
      group = button;

      button = bobgui_check_button_new_with_label ("button5");
      bobgui_check_button_set_active (BOBGUI_CHECK_BUTTON (button), TRUE);
      bobgui_check_button_set_group (BOBGUI_CHECK_BUTTON (button), BOBGUI_CHECK_BUTTON (group));
      bobgui_box_append (BOBGUI_BOX (box2), button);

      button = bobgui_check_button_new_with_label ("button6");
      bobgui_check_button_set_group (BOBGUI_CHECK_BUTTON (button), BOBGUI_CHECK_BUTTON (group));
      bobgui_box_append (BOBGUI_BOX (box2), button);

      separator = bobgui_separator_new (BOBGUI_ORIENTATION_HORIZONTAL);
      bobgui_box_append (BOBGUI_BOX (box1), separator);

      table = create_widget_grid (TRUE);
      bobgui_box_append (BOBGUI_BOX (box1), table);
    }

  if (!bobgui_widget_get_visible (window))
    bobgui_window_present (BOBGUI_WINDOW (window));
  else
    bobgui_window_destroy (BOBGUI_WINDOW (window));
}

/*
 * BobguiToolBar
 */

static BobguiWidget*
new_pixbuf (const char *filename,
            GdkSurface *window)
{
  BobguiWidget *widget;
  GdkPixbuf *pixbuf;

  if (strcmp (filename, "test.xpm") == 0)
    pixbuf = NULL;
  else
    pixbuf = gdk_pixbuf_new_from_file (filename, NULL);

  if (pixbuf == NULL)
    pixbuf = gdk_pixbuf_new_from_xpm_data ((const char **) openfile);
  
  widget = bobgui_image_new_from_pixbuf (pixbuf);

  g_object_unref (pixbuf);

  return widget;
}

/*
 * BobguiStatusBar
 */

static guint statusbar_counter = 1;

static void
statusbar_push (BobguiWidget *button,
		BobguiStatusbar *statusbar)
{
  char text[1024];

  sprintf (text, "something %d", statusbar_counter++);

  bobgui_statusbar_push (statusbar, 1, text);
}

static void
statusbar_push_long (BobguiWidget *button,
                     BobguiStatusbar *statusbar)
{
  char text[1024];

  sprintf (text, "Just because a system has menu choices written with English words, phrases or sentences, that is no guarantee, that it is comprehensible. Individual words may not be familiar to some users (for example, \"repaginate\"), and two menu items may appear to satisfy the users's needs, whereas only one does (for example, \"put away\" or \"eject\").");

  bobgui_statusbar_push (statusbar, 1, text);
}

static void
statusbar_pop (BobguiWidget *button,
	       BobguiStatusbar *statusbar)
{
  bobgui_statusbar_pop (statusbar, 1);
}

static void
statusbar_steal (BobguiWidget *button,
	         BobguiStatusbar *statusbar)
{
  bobgui_statusbar_remove (statusbar, 1, 4);
}

static void
statusbar_popped (BobguiStatusbar  *statusbar,
		  guint          context_id,
		  const char	*text)
{
  if (!text)
    statusbar_counter = 1;
}

static void
statusbar_contexts (BobguiStatusbar *statusbar)
{
  const char *string;

  string = "any context";
  g_print ("BobguiStatusBar: context=\"%s\", context_id=%d\n",
	   string,
	   bobgui_statusbar_get_context_id (statusbar, string));
  
  string = "idle messages";
  g_print ("BobguiStatusBar: context=\"%s\", context_id=%d\n",
	   string,
	   bobgui_statusbar_get_context_id (statusbar, string));
  
  string = "some text";
  g_print ("BobguiStatusBar: context=\"%s\", context_id=%d\n",
	   string,
	   bobgui_statusbar_get_context_id (statusbar, string));

  string = "hit the mouse";
  g_print ("BobguiStatusBar: context=\"%s\", context_id=%d\n",
	   string,
	   bobgui_statusbar_get_context_id (statusbar, string));

  string = "hit the mouse2";
  g_print ("BobguiStatusBar: context=\"%s\", context_id=%d\n",
	   string,
	   bobgui_statusbar_get_context_id (statusbar, string));
}

static void
create_statusbar (BobguiWidget *widget)
{
  static BobguiWidget *window = NULL;
  BobguiWidget *box1;
  BobguiWidget *box2;
  BobguiWidget *button;
  BobguiWidget *separator;
  BobguiWidget *statusbar;

  if (!window)
    {
      window = bobgui_window_new ();
      bobgui_window_set_display (BOBGUI_WINDOW (window),
                              bobgui_widget_get_display (widget));
      g_object_add_weak_pointer (G_OBJECT (window), (gpointer *)&window);

      bobgui_window_set_title (BOBGUI_WINDOW (window), "statusbar");

      box1 = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 0);
      bobgui_window_set_child (BOBGUI_WINDOW (window), box1);

      box2 = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 10);
      bobgui_box_append (BOBGUI_BOX (box1), box2);

      statusbar = bobgui_statusbar_new ();
      g_signal_connect (statusbar,
			"text_popped",
			G_CALLBACK (statusbar_popped),
			NULL);

      button = g_object_new (bobgui_button_get_type (),
			       "label", "push something",
			       NULL);
      bobgui_box_append (BOBGUI_BOX (box2), button);
      g_object_connect (button,
			"signal::clicked", statusbar_push, statusbar,
			NULL);

      button = g_object_connect (g_object_new (bobgui_button_get_type (),
						 "label", "pop",
						 NULL),
				 "signal_after::clicked", statusbar_pop, statusbar,
				 NULL);
      bobgui_box_append (BOBGUI_BOX (box2), button);

      button = g_object_connect (g_object_new (bobgui_button_get_type (),
						 "label", "steal #4",
						 NULL),
				 "signal_after::clicked", statusbar_steal, statusbar,
				 NULL);
      bobgui_box_append (BOBGUI_BOX (box2), button);

      button = g_object_connect (g_object_new (bobgui_button_get_type (),
						 "label", "test contexts",
						 NULL),
				 "swapped_signal_after::clicked", statusbar_contexts, statusbar,
				 NULL);
      bobgui_box_append (BOBGUI_BOX (box2), button);

      button = g_object_connect (g_object_new (bobgui_button_get_type (),
						 "label", "push something long",
						 NULL),
				 "signal_after::clicked", statusbar_push_long, statusbar,
				 NULL);
      bobgui_box_append (BOBGUI_BOX (box2), button);

      separator = bobgui_separator_new (BOBGUI_ORIENTATION_HORIZONTAL);
      bobgui_box_append (BOBGUI_BOX (box1), separator);

      box2 = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 10);
      bobgui_box_append (BOBGUI_BOX (box1), box2);
      bobgui_box_append (BOBGUI_BOX (box1), statusbar);

      button = bobgui_button_new_with_label ("close");
      g_signal_connect_swapped (button, "clicked",
			        G_CALLBACK (bobgui_window_destroy),
				window);
      bobgui_box_append (BOBGUI_BOX (box2), button);
      bobgui_window_set_default_widget (BOBGUI_WINDOW (window), button);
    }

  if (!bobgui_widget_get_visible (window))
    bobgui_window_present (BOBGUI_WINDOW (window));
  else
    bobgui_window_destroy (BOBGUI_WINDOW (window));
}

/* 
 * Label Demo
 */
static void
sensitivity_toggled (BobguiWidget *toggle,
                     BobguiWidget *widget)
{
  bobgui_widget_set_sensitive (widget,
                            bobgui_toggle_button_get_active (BOBGUI_TOGGLE_BUTTON (toggle)));
}

static BobguiWidget*
create_sensitivity_control (BobguiWidget *widget)
{
  BobguiWidget *button;

  button = bobgui_toggle_button_new_with_label ("Sensitive");  

  bobgui_toggle_button_set_active (BOBGUI_TOGGLE_BUTTON (button),
                                bobgui_widget_is_sensitive (widget));
  
  g_signal_connect (button,
                    "toggled",
                    G_CALLBACK (sensitivity_toggled),
                    widget);

  return button;
}

static void
set_selectable_recursive (BobguiWidget *widget,
                          gboolean   setting)
{
  if (BOBGUI_IS_LABEL (widget))
    bobgui_label_set_selectable (BOBGUI_LABEL (widget), setting);
  else
    {
      BobguiWidget *child;

      for (child = bobgui_widget_get_first_child (widget);
           child != NULL;
           child = bobgui_widget_get_next_sibling (child))
        set_selectable_recursive (child, setting);
    }
}

static void
selectable_toggled (BobguiWidget *toggle,
                    BobguiWidget *widget)
{
  set_selectable_recursive (widget,
                            bobgui_toggle_button_get_active (BOBGUI_TOGGLE_BUTTON (toggle)));
}

static BobguiWidget*
create_selectable_control (BobguiWidget *widget)
{
  BobguiWidget *button;

  button = bobgui_toggle_button_new_with_label ("Selectable");  

  bobgui_toggle_button_set_active (BOBGUI_TOGGLE_BUTTON (button),
                                FALSE);
  
  g_signal_connect (button,
                    "toggled",
                    G_CALLBACK (selectable_toggled),
                    widget);

  return button;
}

static void
dialog_response (BobguiWidget *dialog, int response_id, BobguiLabel *label)
{
  const char *text;

  bobgui_window_destroy (BOBGUI_WINDOW (dialog));

  text = "Some <a href=\"http://en.wikipedia.org/wiki/Text\" title=\"plain text\">text</a> may be marked up\n"
         "as hyperlinks, which can be clicked\n"
         "or activated via <a href=\"keynav\">keynav</a>.\n"
         "The links remain the same.";
  bobgui_label_set_markup (label, text);
}

static gboolean
activate_link (BobguiWidget *label, const char *uri, gpointer data)
{
  if (g_strcmp0 (uri, "keynav") == 0)
    {
      BobguiWidget *dialog;

      dialog = bobgui_message_dialog_new_with_markup (BOBGUI_WINDOW (bobgui_widget_get_root (label)),
                                       BOBGUI_DIALOG_DESTROY_WITH_PARENT,
                                       BOBGUI_MESSAGE_INFO,
                                       BOBGUI_BUTTONS_OK,
                                       "The term <i>keynav</i> is a shorthand for "
                                       "keyboard navigation and refers to the process of using a program "
                                       "(exclusively) via keyboard input.");

      bobgui_window_present (BOBGUI_WINDOW (dialog));

      g_signal_connect (dialog, "response", G_CALLBACK (dialog_response), label);

      return TRUE;
    }

  return FALSE;
}

static void create_labels (BobguiWidget *widget)
{
  static BobguiWidget *window = NULL;
  BobguiWidget *hbox;
  BobguiWidget *vbox;
  BobguiWidget *frame;
  BobguiWidget *label;
  BobguiWidget *button;

  if (!window)
    {
      window = bobgui_window_new ();

      bobgui_window_set_display (BOBGUI_WINDOW (window),
                              bobgui_widget_get_display (widget));
      g_object_add_weak_pointer (G_OBJECT (window), (gpointer *)&window);

      bobgui_window_set_title (BOBGUI_WINDOW (window), "Label");

      vbox = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 5);

      hbox = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 5);
      bobgui_window_set_child (BOBGUI_WINDOW (window), vbox);


      button = create_sensitivity_control (hbox);

      bobgui_box_append (BOBGUI_BOX (vbox), button);

      button = create_selectable_control (hbox);

      bobgui_box_append (BOBGUI_BOX (vbox), button);
      bobgui_box_append (BOBGUI_BOX (vbox), hbox);

      vbox = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 5);

      bobgui_box_append (BOBGUI_BOX (hbox), vbox);

      frame = bobgui_frame_new ("Normal Label");
      label = bobgui_label_new ("This is a Normal label");
      bobgui_label_set_ellipsize (BOBGUI_LABEL (label), PANGO_ELLIPSIZE_START);
      bobgui_frame_set_child (BOBGUI_FRAME (frame), label);
      bobgui_box_append (BOBGUI_BOX (vbox), frame);

      frame = bobgui_frame_new ("Multi-line Label");
      label = bobgui_label_new ("This is a Multi-line label.\nSecond line\nThird line");
      bobgui_label_set_ellipsize (BOBGUI_LABEL (label), PANGO_ELLIPSIZE_END);
      bobgui_frame_set_child (BOBGUI_FRAME (frame), label);
      bobgui_box_append (BOBGUI_BOX (vbox), frame);

      frame = bobgui_frame_new ("Left Justified Label");
      label = bobgui_label_new ("This is a Left-Justified\nMulti-line label.\nThird      line");
      bobgui_label_set_ellipsize (BOBGUI_LABEL (label), PANGO_ELLIPSIZE_MIDDLE);
      bobgui_label_set_justify (BOBGUI_LABEL (label), BOBGUI_JUSTIFY_LEFT);
      bobgui_frame_set_child (BOBGUI_FRAME (frame), label);
      bobgui_box_append (BOBGUI_BOX (vbox), frame);

      frame = bobgui_frame_new ("Right Justified Label");
      bobgui_label_set_ellipsize (BOBGUI_LABEL (label), PANGO_ELLIPSIZE_START);
      label = bobgui_label_new ("This is a Right-Justified\nMulti-line label.\nFourth line, (j/k)");
      bobgui_label_set_justify (BOBGUI_LABEL (label), BOBGUI_JUSTIFY_RIGHT);
      bobgui_frame_set_child (BOBGUI_FRAME (frame), label);
      bobgui_box_append (BOBGUI_BOX (vbox), frame);

      frame = bobgui_frame_new ("Internationalized Label");
      label = bobgui_label_new (NULL);
      bobgui_label_set_markup (BOBGUI_LABEL (label),
			    "French (Fran\303\247ais) Bonjour, Salut\n"
			    "Korean (\355\225\234\352\270\200)   \354\225\210\353\205\225\355\225\230\354\204\270\354\232\224, \354\225\210\353\205\225\355\225\230\354\213\255\353\213\210\352\271\214\n"
			    "Russian (\320\240\321\203\321\201\321\201\320\272\320\270\320\271) \320\227\320\264\321\200\320\260\320\262\321\201\321\202\320\262\321\203\320\271\321\202\320\265!\n"
			    "Chinese (Simplified) <span lang=\"zh-cn\">\345\205\203\346\260\224	\345\274\200\345\217\221</span>\n"
			    "Chinese (Traditional) <span lang=\"zh-tw\">\345\205\203\346\260\243	\351\226\213\347\231\274</span>\n"
			    "Japanese <span lang=\"ja\">\345\205\203\346\260\227	\351\226\213\347\231\272</span>");
      bobgui_label_set_justify (BOBGUI_LABEL (label), BOBGUI_JUSTIFY_LEFT);
      bobgui_frame_set_child (BOBGUI_FRAME (frame), label);
      bobgui_box_append (BOBGUI_BOX (vbox), frame);

      frame = bobgui_frame_new ("Bidirection Label");
      label = bobgui_label_new ("\342\200\217Arabic	\330\247\331\204\330\263\331\204\330\247\331\205 \330\271\331\204\331\212\331\203\331\205\n"
			     "\342\200\217Hebrew	\327\251\327\234\327\225\327\235");
      bobgui_frame_set_child (BOBGUI_FRAME (frame), label);
      bobgui_box_append (BOBGUI_BOX (vbox), frame);

      frame = bobgui_frame_new ("Links in a label");
      label = bobgui_label_new ("Some <a href=\"http://en.wikipedia.org/wiki/Text\" title=\"plain text\">text</a> may be marked up\n"
                             "as hyperlinks, which can be clicked\n"
                             "or activated via <a href=\"keynav\">keynav</a>");
      bobgui_label_set_use_markup (BOBGUI_LABEL (label), TRUE);
      bobgui_frame_set_child (BOBGUI_FRAME (frame), label);
      bobgui_box_append (BOBGUI_BOX (vbox), frame);
      g_signal_connect (label, "activate-link", G_CALLBACK (activate_link), NULL);

      vbox = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 5);
      bobgui_box_append (BOBGUI_BOX (hbox), vbox);
      frame = bobgui_frame_new ("Line wrapped label");
      label = bobgui_label_new ("This is an example of a line-wrapped label.  It should not be taking "\
			     "up the entire             "/* big space to test spacing */\
			     "width allocated to it, but automatically wraps the words to fit.  "\
			     "The time has come, for all good men, to come to the aid of their party.  "\
			     "The sixth sheik's six sheep's sick.\n"\
			     "     It supports multiple paragraphs correctly, and  correctly   adds "\
			     "many          extra  spaces. ");

      bobgui_label_set_wrap (BOBGUI_LABEL (label), TRUE);
      bobgui_frame_set_child (BOBGUI_FRAME (frame), label);
      bobgui_box_append (BOBGUI_BOX (vbox), frame);

      frame = bobgui_frame_new ("Filled, wrapped label");
      label = bobgui_label_new ("This is an example of a line-wrapped, filled label.  It should be taking "\
			     "up the entire              width allocated to it.  Here is a seneance to prove "\
			     "my point.  Here is another sentence. "\
			     "Here comes the sun, do de do de do.\n"\
			     "    This is a new paragraph.\n"\
			     "    This is another newer, longer, better paragraph.  It is coming to an end, "\
			     "unfortunately.");
      bobgui_label_set_justify (BOBGUI_LABEL (label), BOBGUI_JUSTIFY_FILL);
      bobgui_label_set_wrap (BOBGUI_LABEL (label), TRUE);
      bobgui_frame_set_child (BOBGUI_FRAME (frame), label);
      bobgui_box_append (BOBGUI_BOX (vbox), frame);

      frame = bobgui_frame_new ("Underlined label");
      label = bobgui_label_new ("This label is underlined!\n"
			     "This one is underlined (\343\201\223\343\202\223\343\201\253\343\201\241\343\201\257) in quite a funky fashion");
      bobgui_label_set_justify (BOBGUI_LABEL (label), BOBGUI_JUSTIFY_LEFT);
      bobgui_frame_set_child (BOBGUI_FRAME (frame), label);
      bobgui_box_append (BOBGUI_BOX (vbox), frame);

      frame = bobgui_frame_new ("Markup label");
      label = bobgui_label_new (NULL);

      /* There's also a bobgui_label_set_markup() without accel if you
       * don't have an accelerator key
       */
      bobgui_label_set_markup_with_mnemonic (BOBGUI_LABEL (label),
					  "This <span foreground=\"blue\" background=\"orange\">label</span> has "
					  "<b>markup</b> _such as "
					  "<big><i>Big Italics</i></big>\n"
					  "<tt>Monospace font</tt>\n"
					  "<u>Underline!</u>\n"
					  "foo\n"
					  "<span foreground=\"green\" background=\"red\">Ugly colors</span>\n"
					  "and nothing on this line,\n"
					  "or this.\n"
					  "or this either\n"
					  "or even on this one\n"
					  "la <big>la <big>la <big>la <big>la</big></big></big></big>\n"
					  "but this _word is <span foreground=\"purple\"><big>purple</big></span>\n"
					  "<span underline=\"double\">We like <sup>superscript</sup> and <sub>subscript</sub> too</span>");

      g_assert (bobgui_label_get_mnemonic_keyval (BOBGUI_LABEL (label)) == GDK_KEY_s);

      bobgui_frame_set_child (BOBGUI_FRAME (frame), label);
      bobgui_box_append (BOBGUI_BOX (vbox), frame);
    }

  if (!bobgui_widget_get_visible (window))
    bobgui_window_present (BOBGUI_WINDOW (window));
  else
    bobgui_window_destroy (BOBGUI_WINDOW (window));
}

#define DEFAULT_TEXT_RADIUS 200

static void
on_rotated_text_unrealize (BobguiWidget *widget)
{
  g_object_set_data (G_OBJECT (widget), "text-gc", NULL);
}

static void
on_rotated_text_draw (BobguiDrawingArea *drawing_area,
                      cairo_t        *cr,
                      int             width,
                      int             height,
                      gpointer        tile_pixbuf)
{
  static const char *words[] = { "The", "grand", "old", "Duke", "of", "York",
                                  "had", "10,000", "men" };
  int n_words;
  int i;
  double radius;
  PangoLayout *layout;
  PangoContext *context;
  PangoFontDescription *desc;

  cairo_set_source_rgb (cr, 1, 1, 1);
  cairo_paint (cr);

  if (tile_pixbuf)
    {
      gdk_cairo_set_source_pixbuf (cr, tile_pixbuf, 0, 0);
      cairo_pattern_set_extend (cairo_get_source (cr), CAIRO_EXTEND_REPEAT);
    }
  else
    cairo_set_source_rgb (cr, 0, 0, 0);

  radius = MIN (width, height) / 2.;

  cairo_translate (cr,
                   radius + (width - 2 * radius) / 2,
                   radius + (height - 2 * radius) / 2);
  cairo_scale (cr, radius / DEFAULT_TEXT_RADIUS, radius / DEFAULT_TEXT_RADIUS);

  context = bobgui_widget_get_pango_context (BOBGUI_WIDGET (drawing_area));
  layout = pango_layout_new (context);
  desc = pango_font_description_from_string ("Sans Bold 30");
  pango_layout_set_font_description (layout, desc);
  pango_font_description_free (desc);
    
  n_words = G_N_ELEMENTS (words);
  for (i = 0; i < n_words; i++)
    {
      int layout_width, layout_height;

      cairo_save (cr);

      cairo_rotate (cr, 2 * G_PI * i / n_words);
      pango_cairo_update_layout (cr, layout);

      pango_layout_set_text (layout, words[i], -1);
      pango_layout_get_size (layout, &layout_width, &layout_height);

      cairo_move_to (cr, - layout_width / 2 / PANGO_SCALE, - DEFAULT_TEXT_RADIUS);
      pango_cairo_show_layout (cr, layout);

      cairo_restore (cr);
    }
  
  g_object_unref (layout);
}

static void
create_rotated_text (BobguiWidget *widget)
{
  static BobguiWidget *window = NULL;

  if (!window)
    {
      BobguiWidget *content_area;
      BobguiWidget *drawing_area;
      GdkPixbuf *tile_pixbuf;

      window = bobgui_dialog_new_with_buttons ("Rotated Text",
					    BOBGUI_WINDOW (bobgui_widget_get_root (widget)), 0,
					    "_Close", BOBGUI_RESPONSE_CLOSE,
					    NULL);

      bobgui_window_set_resizable (BOBGUI_WINDOW (window), TRUE);

      bobgui_window_set_display (BOBGUI_WINDOW (window),
			      bobgui_widget_get_display (widget));

      g_signal_connect (window, "response",
			G_CALLBACK (bobgui_window_destroy), NULL);
      g_object_add_weak_pointer (G_OBJECT (window), (gpointer *)&window);

      content_area = bobgui_dialog_get_content_area (BOBGUI_DIALOG (window));

      drawing_area = bobgui_drawing_area_new ();
      bobgui_widget_set_hexpand (drawing_area, TRUE);
      bobgui_widget_set_vexpand (drawing_area, TRUE);
      bobgui_box_append (BOBGUI_BOX (content_area), drawing_area);

      tile_pixbuf = gdk_pixbuf_new_from_file ("marble.xpm", NULL);

      bobgui_drawing_area_set_draw_func (BOBGUI_DRAWING_AREA (drawing_area),
			              on_rotated_text_draw,
                                      tile_pixbuf,
                                      g_object_unref);
      g_signal_connect (drawing_area, "unrealize",
			G_CALLBACK (on_rotated_text_unrealize), NULL);

      bobgui_drawing_area_set_content_width (BOBGUI_DRAWING_AREA (drawing_area), DEFAULT_TEXT_RADIUS * 2);
      bobgui_drawing_area_set_content_height (BOBGUI_DRAWING_AREA (drawing_area), DEFAULT_TEXT_RADIUS * 2);
    }

  if (!bobgui_widget_get_visible (window))
    bobgui_window_present (BOBGUI_WINDOW (window));
  else
    bobgui_window_destroy (BOBGUI_WINDOW (window));
}


/*
 * BobguiPixmap
 */

static void
create_pixbuf (BobguiWidget *widget)
{
  static BobguiWidget *window = NULL;
  BobguiWidget *box1;
  BobguiWidget *box2;
  BobguiWidget *box3;
  BobguiWidget *button;
  BobguiWidget *label;
  BobguiWidget *separator;
  BobguiWidget *pixbufwid;
  GdkSurface *gdk_surface;

  if (!window)
    {
      window = bobgui_window_new ();

      bobgui_window_set_display (BOBGUI_WINDOW (window),
                              bobgui_widget_get_display (widget));
      g_object_add_weak_pointer (G_OBJECT (window), (gpointer *)&window);

      bobgui_window_set_title (BOBGUI_WINDOW (window), "BobguiPixmap");
      bobgui_widget_realize(window);

      box1 = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 0);
      bobgui_window_set_child (BOBGUI_WINDOW (window), box1);

      box2 = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 10);
      bobgui_box_append (BOBGUI_BOX (box1), box2);

      button = bobgui_button_new ();
      bobgui_box_append (BOBGUI_BOX (box2), button);

      gdk_surface = bobgui_native_get_surface (BOBGUI_NATIVE (window));

      pixbufwid = new_pixbuf ("test.xpm", gdk_surface);

      label = bobgui_label_new ("Pixbuf\ntest");
      box3 = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 0);
      bobgui_box_append (BOBGUI_BOX (box3), pixbufwid);
      bobgui_box_append (BOBGUI_BOX (box3), label);
      bobgui_button_set_child (BOBGUI_BUTTON (button), box3);

      button = bobgui_button_new ();
      bobgui_box_append (BOBGUI_BOX (box2), button);

      pixbufwid = new_pixbuf ("test.xpm", gdk_surface);

      label = bobgui_label_new ("Pixbuf\ntest");
      box3 = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 0);
      bobgui_box_append (BOBGUI_BOX (box3), pixbufwid);
      bobgui_box_append (BOBGUI_BOX (box3), label);
      bobgui_button_set_child (BOBGUI_BUTTON (button), box3);

      bobgui_widget_set_sensitive (button, FALSE);

      separator = bobgui_separator_new (BOBGUI_ORIENTATION_HORIZONTAL);
      bobgui_box_append (BOBGUI_BOX (box1), separator);

      box2 = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 10);
      bobgui_box_append (BOBGUI_BOX (box1), box2);

      button = bobgui_button_new_with_label ("close");
      g_signal_connect_swapped (button, "clicked",
			        G_CALLBACK (bobgui_window_destroy),
				window);
      bobgui_box_append (BOBGUI_BOX (box2), button);
      bobgui_window_set_default_widget (BOBGUI_WINDOW (window), button);
    }

  if (!bobgui_widget_get_visible (window))
    bobgui_window_present (BOBGUI_WINDOW (window));
  else
    bobgui_window_destroy (BOBGUI_WINDOW (window));
}

static void
create_tooltips (BobguiWidget *widget)
{
  static BobguiWidget *window = NULL;
  BobguiWidget *box1;
  BobguiWidget *box2;
  BobguiWidget *box3;
  BobguiWidget *button;
  BobguiWidget *toggle;
  BobguiWidget *frame;
  BobguiWidget *separator;

  if (!window)
    {
      window =
	g_object_new (bobgui_window_get_type (),
			"BobguiWindow::title", "Tooltips",
			"BobguiWindow::resizable", FALSE,
			NULL);

      bobgui_window_set_display (BOBGUI_WINDOW (window),
			      bobgui_widget_get_display (widget));

      box1 = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 0);
      bobgui_window_set_child (BOBGUI_WINDOW (window), box1);

      box2 = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 10);
      bobgui_box_append (BOBGUI_BOX (box1), box2);

      button = bobgui_toggle_button_new_with_label ("button1");
      bobgui_box_append (BOBGUI_BOX (box2), button);

      bobgui_widget_set_tooltip_text (button, "This is button 1");

      button = bobgui_toggle_button_new_with_label ("button2");
      bobgui_box_append (BOBGUI_BOX (box2), button);

      bobgui_widget_set_tooltip_text (button,
        "This is button 2. This is also a really long tooltip which probably "
        "won't fit on a single line and will therefore need to be wrapped. "
        "Hopefully the wrapping will work correctly.");

      toggle = bobgui_toggle_button_new_with_label ("Override TipsQuery Label");
      bobgui_box_append (BOBGUI_BOX (box2), toggle);

      bobgui_widget_set_tooltip_text (toggle, "Toggle TipsQuery view.");

      box3 =
	g_object_new (BOBGUI_TYPE_BOX,
                      "orientation", BOBGUI_ORIENTATION_VERTICAL,
			"homogeneous", FALSE,
			"spacing", 5,
			NULL);

      button =
	g_object_new (bobgui_button_get_type (),
			"label", "[?]",
			NULL);
      bobgui_box_append (BOBGUI_BOX (box3), button);
      bobgui_widget_set_tooltip_text (button, "Start the Tooltips Inspector");

      frame = g_object_new (bobgui_frame_get_type (),
			      "label", "ToolTips Inspector",
			      "label_xalign", (double) 0.5,
			      NULL);
      bobgui_box_append (BOBGUI_BOX (box2), frame);
      bobgui_frame_set_child (BOBGUI_FRAME (frame), box3);

      separator = bobgui_separator_new (BOBGUI_ORIENTATION_HORIZONTAL);
      bobgui_box_append (BOBGUI_BOX (box1), separator);

      box2 = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 10);
      bobgui_box_append (BOBGUI_BOX (box1), box2);

      button = bobgui_button_new_with_label ("close");
      g_signal_connect_swapped (button, "clicked",
			        G_CALLBACK (bobgui_window_destroy),
				window);
      bobgui_box_append (BOBGUI_BOX (box2), button);
      bobgui_window_set_default_widget (BOBGUI_WINDOW (window), button);

      bobgui_widget_set_tooltip_text (button, "Push this button to close window");
    }

  if (!bobgui_widget_get_visible (window))
    bobgui_window_present (BOBGUI_WINDOW (window));
  else
    bobgui_window_destroy (BOBGUI_WINDOW (window));
}

/*
 * BobguiImage
 */

static void
pack_image (BobguiWidget *box,
            const char *text,
            BobguiWidget *image)
{
  bobgui_box_append (BOBGUI_BOX (box),
                      bobgui_label_new (text));

  bobgui_box_append (BOBGUI_BOX (box),
                      image);
}

static void
create_image (BobguiWidget *widget)
{
  static BobguiWidget *window = NULL;

  if (window == NULL)
    {
      BobguiWidget *vbox;
      GdkPixbuf *pixbuf;
        
      window = bobgui_window_new ();
      
      bobgui_window_set_display (BOBGUI_WINDOW (window),
			      bobgui_widget_get_display (widget));

      /* this is bogus for testing drawing when allocation < request,
       * don't copy into real code
       */
      bobgui_window_set_resizable (BOBGUI_WINDOW (window), TRUE);

      g_object_add_weak_pointer (G_OBJECT (window), (gpointer *)&window);

      vbox = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 5);

      bobgui_window_set_child (BOBGUI_WINDOW (window), vbox);

      pack_image (vbox, "Stock Warning Dialog", bobgui_image_new_from_icon_name ("dialog-warning"));

      pixbuf = gdk_pixbuf_new_from_xpm_data ((const char **) openfile);
      
      pack_image (vbox, "Pixbuf",
                  bobgui_image_new_from_pixbuf (pixbuf));

      g_object_unref (pixbuf);
    }

  if (!bobgui_widget_get_visible (window))
    bobgui_window_present (BOBGUI_WINDOW (window));
  else
    bobgui_window_destroy (BOBGUI_WINDOW (window));
}

/*
 * ListBox demo
 */

static int
list_sort_cb (BobguiListBoxRow *a, BobguiListBoxRow *b, gpointer data)
{
  int aa = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (a), "value"));
  int bb = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (b), "value"));
  return aa - bb;
}

static gboolean
list_filter_all_cb (BobguiListBoxRow *row, gpointer data)
{
  return FALSE;
}

static gboolean
list_filter_odd_cb (BobguiListBoxRow *row, gpointer data)
{
  int value = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (row), "value"));

  return value % 2 == 0;
}

static void
list_sort_clicked_cb (BobguiButton *button,
                      gpointer data)
{
  BobguiListBox *list = data;

  bobgui_list_box_set_sort_func (list, list_sort_cb, NULL, NULL);
}

static void
list_filter_odd_clicked_cb (BobguiButton *button,
                            gpointer data)
{
  BobguiListBox *list = data;

  bobgui_list_box_set_filter_func (list, list_filter_odd_cb, NULL, NULL);
}

static void
list_filter_all_clicked_cb (BobguiButton *button,
                            gpointer data)
{
  BobguiListBox *list = data;

  bobgui_list_box_set_filter_func (list, list_filter_all_cb, NULL, NULL);
}


static void
list_unfilter_clicked_cb (BobguiButton *button,
                          gpointer data)
{
  BobguiListBox *list = data;

  bobgui_list_box_set_filter_func (list, NULL, NULL, NULL);
}

static void
add_placeholder_clicked_cb (BobguiButton *button,
                            gpointer data)
{
  BobguiListBox *list = data;
  BobguiWidget *label;

  label = bobgui_label_new ("You filtered everything!!!");
  bobgui_list_box_set_placeholder (BOBGUI_LIST_BOX (list), label);
}

static void
remove_placeholder_clicked_cb (BobguiButton *button,
                            gpointer data)
{
  BobguiListBox *list = data;

  bobgui_list_box_set_placeholder (BOBGUI_LIST_BOX (list), NULL);
}


static void
create_listbox (BobguiWidget *widget)
{
  static BobguiWidget *window = NULL;

  if (!window)
    {
      BobguiWidget *hbox, *vbox, *scrolled, *scrolled_box, *list, *label, *button;
      GdkDisplay *display = bobgui_widget_get_display (widget);
      int i;

      window = bobgui_window_new ();
      bobgui_window_set_hide_on_close (BOBGUI_WINDOW (window), TRUE);
      bobgui_window_set_display (BOBGUI_WINDOW (window), display);

      g_object_add_weak_pointer (G_OBJECT (window), (gpointer *)&window);

      bobgui_window_set_title (BOBGUI_WINDOW (window), "listbox");

      hbox = bobgui_box_new(BOBGUI_ORIENTATION_HORIZONTAL, 0);
      bobgui_window_set_child (BOBGUI_WINDOW (window), hbox);

      scrolled = bobgui_scrolled_window_new ();
      bobgui_scrolled_window_set_policy (BOBGUI_SCROLLED_WINDOW (scrolled), BOBGUI_POLICY_NEVER, BOBGUI_POLICY_AUTOMATIC);
      bobgui_box_append (BOBGUI_BOX (hbox), scrolled);

      scrolled_box = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 0);
      bobgui_scrolled_window_set_child (BOBGUI_SCROLLED_WINDOW (scrolled), scrolled_box);

      label = bobgui_label_new ("This is \na LABEL\nwith rows");
      bobgui_box_append (BOBGUI_BOX (scrolled_box), label);

      list = bobgui_list_box_new();
      bobgui_list_box_set_adjustment (BOBGUI_LIST_BOX (list), bobgui_scrolled_window_get_vadjustment (BOBGUI_SCROLLED_WINDOW (scrolled)));
      bobgui_box_append (BOBGUI_BOX (scrolled_box), list);

      for (i = 0; i < 1000; i++)
        {
          int value = g_random_int_range (0, 10000);
          label = bobgui_label_new (g_strdup_printf ("Value %u", value));
          bobgui_list_box_insert (BOBGUI_LIST_BOX (list), label, -1);
          g_object_set_data (G_OBJECT (bobgui_widget_get_parent (label)), "value", GINT_TO_POINTER (value));
        }

      vbox = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 0);
      bobgui_box_append (BOBGUI_BOX (hbox), vbox);

      button = bobgui_button_new_with_label ("sort");
      bobgui_box_append (BOBGUI_BOX (vbox), button);
      g_signal_connect (button, "clicked", G_CALLBACK (list_sort_clicked_cb), list);

      button = bobgui_button_new_with_label ("filter odd");
      bobgui_box_append (BOBGUI_BOX (vbox), button);
      g_signal_connect (button, "clicked", G_CALLBACK (list_filter_odd_clicked_cb), list);

      button = bobgui_button_new_with_label ("filter all");
      bobgui_box_append (BOBGUI_BOX (vbox), button);
      g_signal_connect (button, "clicked", G_CALLBACK (list_filter_all_clicked_cb), list);

      button = bobgui_button_new_with_label ("unfilter");
      bobgui_box_append (BOBGUI_BOX (vbox), button);
      g_signal_connect (button, "clicked", G_CALLBACK (list_unfilter_clicked_cb), list);

      button = bobgui_button_new_with_label ("add placeholder");
      bobgui_box_append (BOBGUI_BOX (vbox), button);
      g_signal_connect (button, "clicked", G_CALLBACK (add_placeholder_clicked_cb), list);

      button = bobgui_button_new_with_label ("remove placeholder");
      bobgui_box_append (BOBGUI_BOX (vbox), button);
      g_signal_connect (button, "clicked", G_CALLBACK (remove_placeholder_clicked_cb), list);
    }

  if (!bobgui_widget_get_visible (window))
    bobgui_window_present (BOBGUI_WINDOW (window));
  else
    bobgui_window_destroy (BOBGUI_WINDOW (window));
}

/*
 create_modal_window
 */

static gboolean
cmw_destroy_cb(BobguiWidget *widget)
{
  done = TRUE;
  g_main_context_wakeup (NULL);

  return FALSE;
}

static void
cmw_color (BobguiWidget *widget, BobguiWidget *parent)
{
    BobguiWidget *csd;

    csd = bobgui_color_chooser_dialog_new ("This is a modal color selection dialog", BOBGUI_WINDOW (parent));

    /* Set as modal */
    bobgui_window_set_modal (BOBGUI_WINDOW(csd),TRUE);

    g_signal_connect (csd, "destroy",
		      G_CALLBACK (cmw_destroy_cb), NULL);
    g_signal_connect (csd, "response",
                      G_CALLBACK (bobgui_window_destroy), NULL);
    
    /* wait until destroy calls bobgui_main_quit */
    bobgui_window_present (BOBGUI_WINDOW (csd));
    while (!done)
      g_main_context_iteration (NULL, TRUE);
}

static void
cmw_file (BobguiWidget *widget, BobguiWidget *parent)
{
    BobguiWidget *fs;

    fs = bobgui_file_chooser_dialog_new ("This is a modal file selection dialog",
                                      BOBGUI_WINDOW (parent),
                                      BOBGUI_FILE_CHOOSER_ACTION_OPEN,
                                      "_Open", BOBGUI_RESPONSE_ACCEPT,
                                      "_Cancel", BOBGUI_RESPONSE_CANCEL,
                                      NULL);
    bobgui_window_set_display (BOBGUI_WINDOW (fs), bobgui_widget_get_display (parent));
    bobgui_window_set_modal (BOBGUI_WINDOW (fs), TRUE);

    g_signal_connect (fs, "destroy",
                      G_CALLBACK (cmw_destroy_cb), NULL);
    g_signal_connect_swapped (fs, "response",
                      G_CALLBACK (bobgui_window_destroy), fs);

    /* wait until destroy calls bobgui_main_quit */
    bobgui_widget_show (fs);
    while (!done)
      g_main_context_iteration (NULL, TRUE);
}


static void
create_modal_window (BobguiWidget *widget)
{
  BobguiWidget *window = NULL;
  BobguiWidget *box1,*box2;
  BobguiWidget *frame1;
  BobguiWidget *btnColor,*btnFile,*btnClose;

  /* Create modal window (Here you can use any window descendent )*/
  window = bobgui_window_new ();
  bobgui_window_set_display (BOBGUI_WINDOW (window),
			  bobgui_widget_get_display (widget));

  bobgui_window_set_title (BOBGUI_WINDOW(window),"This window is modal");

  /* Set window as modal */
  bobgui_window_set_modal (BOBGUI_WINDOW(window),TRUE);

  /* Create widgets */
  box1 = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 5);
  frame1 = bobgui_frame_new ("Standard dialogs in modal form");
  box2 = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 5);
  bobgui_box_set_homogeneous (BOBGUI_BOX (box2), TRUE);
  btnColor = bobgui_button_new_with_label ("Color");
  btnFile = bobgui_button_new_with_label ("File Selection");
  btnClose = bobgui_button_new_with_label ("Close");

  /* Pack widgets */
  bobgui_window_set_child (BOBGUI_WINDOW (window), box1);
  bobgui_box_append (BOBGUI_BOX (box1), frame1);
  bobgui_frame_set_child (BOBGUI_FRAME (frame1), box2);
  bobgui_box_append (BOBGUI_BOX (box2), btnColor);
  bobgui_box_append (BOBGUI_BOX (box2), btnFile);
  bobgui_box_append (BOBGUI_BOX (box1), bobgui_separator_new (BOBGUI_ORIENTATION_HORIZONTAL));
  bobgui_box_append (BOBGUI_BOX (box1), btnClose);

  /* connect signals */
  g_signal_connect_swapped (btnClose, "clicked",
			    G_CALLBACK (bobgui_window_destroy), window);

  g_signal_connect (window, "destroy",
                    G_CALLBACK (cmw_destroy_cb), NULL);

  g_signal_connect (btnColor, "clicked",
                    G_CALLBACK (cmw_color), window);
  g_signal_connect (btnFile, "clicked",
                    G_CALLBACK (cmw_file), window);

  /* Show widgets */
  bobgui_window_present (BOBGUI_WINDOW (window));

  /* wait until dialog get destroyed */
  while (!done)
    g_main_context_iteration (NULL, TRUE);
}

/*
 * BobguiMessageDialog
 */

static void
make_message_dialog (GdkDisplay     *display,
		     BobguiWidget     **dialog,
                     BobguiMessageType  type,
                     BobguiButtonsType  buttons,
		     guint           default_response)
{
  if (*dialog)
    {
      bobgui_window_destroy (BOBGUI_WINDOW (*dialog));

      return;
    }

  *dialog = bobgui_message_dialog_new (NULL, 0, type, buttons,
                                    "This is a message dialog; it can wrap long lines. This is a long line. La la la. Look this line is wrapped. Blah blah blah blah blah blah. (Note: testbobgui has a nonstandard bobguirc that changes some of the message dialog icons.)");

  bobgui_window_set_display (BOBGUI_WINDOW (*dialog), display);

  g_signal_connect_swapped (*dialog,
			    "response",
			    G_CALLBACK (bobgui_window_destroy),
			    *dialog);
  
  g_object_add_weak_pointer (G_OBJECT (*dialog), (gpointer)dialog);

  bobgui_dialog_set_default_response (BOBGUI_DIALOG (*dialog), default_response);

  bobgui_widget_show (*dialog);
}

static void
create_message_dialog (BobguiWidget *widget)
{
  static BobguiWidget *info = NULL;
  static BobguiWidget *warning = NULL;
  static BobguiWidget *error = NULL;
  static BobguiWidget *question = NULL;
  GdkDisplay *display = bobgui_widget_get_display (widget);

  make_message_dialog (display, &info, BOBGUI_MESSAGE_INFO, BOBGUI_BUTTONS_OK, BOBGUI_RESPONSE_OK);
  make_message_dialog (display, &warning, BOBGUI_MESSAGE_WARNING, BOBGUI_BUTTONS_CLOSE, BOBGUI_RESPONSE_CLOSE);
  make_message_dialog (display, &error, BOBGUI_MESSAGE_ERROR, BOBGUI_BUTTONS_OK_CANCEL, BOBGUI_RESPONSE_OK);
  make_message_dialog (display, &question, BOBGUI_MESSAGE_QUESTION, BOBGUI_BUTTONS_YES_NO, BOBGUI_RESPONSE_NO);
}

/*
 * BobguiScrolledWindow
 */

static BobguiWidget *sw_parent = NULL;
static BobguiWidget *sw_float_parent;
static gulong sw_destroyed_handler = 0;

static gboolean
scrolled_windows_delete_cb (BobguiWidget *widget,
                            BobguiWidget *scrollwin)
{
  g_object_ref (scrollwin);
  bobgui_box_remove (BOBGUI_BOX (bobgui_widget_get_parent (scrollwin)), scrollwin);
  bobgui_scrolled_window_set_child (BOBGUI_SCROLLED_WINDOW (sw_parent), scrollwin);
  g_object_unref (scrollwin);

  g_signal_handler_disconnect (sw_parent, sw_destroyed_handler);
  sw_float_parent = NULL;
  sw_parent = NULL;
  sw_destroyed_handler = 0;

  return FALSE;
}

static void
scrolled_windows_destroy_cb (BobguiWidget *widget, BobguiWidget *scrollwin)
{
  bobgui_window_destroy (BOBGUI_WINDOW (sw_float_parent));

  sw_float_parent = NULL;
  sw_parent = NULL;
  sw_destroyed_handler = 0;
}

static void
scrolled_windows_remove (BobguiWidget *dialog, int response, BobguiWidget *scrollwin)
{
  if (response != BOBGUI_RESPONSE_APPLY)
    {
      bobgui_window_destroy (BOBGUI_WINDOW (dialog));
      return;
    }

  if (sw_parent)
    {
      g_object_ref (scrollwin);
      bobgui_box_remove (BOBGUI_BOX (bobgui_widget_get_parent (scrollwin)), scrollwin);
      bobgui_window_set_child (BOBGUI_WINDOW (sw_float_parent), scrollwin);
      g_object_unref (scrollwin);


      bobgui_window_destroy (BOBGUI_WINDOW (sw_float_parent));

      g_signal_handler_disconnect (sw_parent, sw_destroyed_handler);
      sw_float_parent = NULL;
      sw_parent = NULL;
      sw_destroyed_handler = 0;
    }
  else
    {
      sw_parent = bobgui_widget_get_parent (scrollwin);
      sw_float_parent = bobgui_window_new ();
      bobgui_window_set_display (BOBGUI_WINDOW (sw_float_parent),
			      bobgui_widget_get_display (dialog));
      
      bobgui_window_set_default_size (BOBGUI_WINDOW (sw_float_parent), 200, 200);
      
      g_object_ref (scrollwin);
      bobgui_box_remove (BOBGUI_BOX (bobgui_widget_get_parent (scrollwin)), scrollwin);
      bobgui_window_set_child (BOBGUI_WINDOW (sw_float_parent), scrollwin);
      g_object_unref (scrollwin);


      bobgui_widget_show (sw_float_parent);

      sw_destroyed_handler =
	g_signal_connect (sw_parent, "destroy",
			  G_CALLBACK (scrolled_windows_destroy_cb), scrollwin);
      g_signal_connect (sw_float_parent, "close-request",
			G_CALLBACK (scrolled_windows_delete_cb), scrollwin);
    }
}

static void
create_scrolled_windows (BobguiWidget *widget)
{
  static BobguiWidget *window;
  BobguiWidget *content_area;
  BobguiWidget *scrolled_window;
  BobguiWidget *button;
  BobguiWidget *grid;
  char buffer[32];
  int i, j;

  if (!window)
    {
      window = bobgui_dialog_new ();

      bobgui_window_set_display (BOBGUI_WINDOW (window),
                              bobgui_widget_get_display (widget));
      g_object_add_weak_pointer (G_OBJECT (window), (gpointer *)&window);

      content_area = bobgui_dialog_get_content_area (BOBGUI_DIALOG (window));

      bobgui_window_set_title (BOBGUI_WINDOW (window), "dialog");

      scrolled_window = bobgui_scrolled_window_new ();
      bobgui_widget_set_hexpand (scrolled_window, TRUE);
      bobgui_widget_set_vexpand (scrolled_window, TRUE);
      bobgui_scrolled_window_set_policy (BOBGUI_SCROLLED_WINDOW (scrolled_window),
				      BOBGUI_POLICY_AUTOMATIC,
				      BOBGUI_POLICY_AUTOMATIC);
      bobgui_box_append (BOBGUI_BOX (content_area), scrolled_window);

      grid = bobgui_grid_new ();
      bobgui_grid_set_row_spacing (BOBGUI_GRID (grid), 10);
      bobgui_grid_set_column_spacing (BOBGUI_GRID (grid), 10);
      bobgui_scrolled_window_set_child (BOBGUI_SCROLLED_WINDOW (scrolled_window), grid);
      bobgui_viewport_set_scroll_to_focus (BOBGUI_VIEWPORT (bobgui_widget_get_parent (grid)), TRUE);

      for (i = 0; i < 20; i++)
	for (j = 0; j < 20; j++)
	  {
	    sprintf (buffer, "button (%d,%d)\n", i, j);
	    button = bobgui_toggle_button_new_with_label (buffer);
	    bobgui_grid_attach (BOBGUI_GRID (grid), button, i, j, 1, 1);
	  }

      bobgui_dialog_add_button (BOBGUI_DIALOG (window),
                             "Close",
                             BOBGUI_RESPONSE_CLOSE);

      bobgui_dialog_add_button (BOBGUI_DIALOG (window),
                             "Reparent Out",
                             BOBGUI_RESPONSE_APPLY);

      g_signal_connect (window, "response",
			G_CALLBACK (scrolled_windows_remove),
			scrolled_window);

      bobgui_window_set_default_size (BOBGUI_WINDOW (window), 300, 300);
    }

  if (!bobgui_widget_get_visible (window))
    bobgui_window_present (BOBGUI_WINDOW (window));
  else
    bobgui_window_destroy (BOBGUI_WINDOW (window));
}

/*
 * BobguiEntry
 */

static void
entry_toggle_frame (BobguiWidget *checkbutton,
                    BobguiWidget *entry)
{
   bobgui_entry_set_has_frame (BOBGUI_ENTRY(entry),
                            bobgui_toggle_button_get_active (BOBGUI_TOGGLE_BUTTON (checkbutton)));
}

static void
entry_toggle_sensitive (BobguiWidget *checkbutton,
			BobguiWidget *entry)
{
   bobgui_widget_set_sensitive (entry,
                             bobgui_toggle_button_get_active (BOBGUI_TOGGLE_BUTTON(checkbutton)));
}

static gboolean
entry_progress_timeout (gpointer data)
{
  if (GPOINTER_TO_INT (g_object_get_data (G_OBJECT (data), "progress-pulse")))
    {
      bobgui_entry_progress_pulse (BOBGUI_ENTRY (data));
    }
  else
    {
      double fraction;

      fraction = bobgui_entry_get_progress_fraction (BOBGUI_ENTRY (data));

      fraction += 0.05;
      if (fraction > 1.0001)
        fraction = 0.0;

      bobgui_entry_set_progress_fraction (BOBGUI_ENTRY (data), fraction);
    }

  return G_SOURCE_CONTINUE;
}

static void
entry_remove_timeout (gpointer data)
{
  g_source_remove (GPOINTER_TO_UINT (data));
}

static void
entry_toggle_progress (BobguiWidget *checkbutton,
                       BobguiWidget *entry)
{
  if (bobgui_toggle_button_get_active (BOBGUI_TOGGLE_BUTTON (checkbutton)))
    {
      guint timeout = g_timeout_add (100, entry_progress_timeout, entry);
      g_object_set_data_full (G_OBJECT (entry), "timeout-id",
                              GUINT_TO_POINTER (timeout),
                              entry_remove_timeout);
    }
  else
    {
      g_object_set_data (G_OBJECT (entry), "timeout-id",
                         GUINT_TO_POINTER (0));

      bobgui_entry_set_progress_fraction (BOBGUI_ENTRY (entry), 0.0);
    }
}

static void
entry_toggle_pulse (BobguiWidget *checkbutton,
                    BobguiWidget *entry)
{
  g_object_set_data (G_OBJECT (entry), "progress-pulse",
                     GUINT_TO_POINTER ((guint) bobgui_toggle_button_get_active (BOBGUI_TOGGLE_BUTTON (checkbutton))));
}

static void
create_entry (BobguiWidget *widget)
{
  static BobguiWidget *window = NULL;
  BobguiWidget *box1;
  BobguiWidget *box2;
  BobguiWidget *hbox;
  BobguiWidget *has_frame_check;
  BobguiWidget *sensitive_check;
  BobguiWidget *progress_check;
  BobguiWidget *entry;
  BobguiComboBoxText *cb;
  BobguiWidget *cb_entry;
  BobguiWidget *button;
  BobguiWidget *separator;

  if (!window)
    {
      window = bobgui_window_new ();
      bobgui_window_set_display (BOBGUI_WINDOW (window),
                              bobgui_widget_get_display (widget));
      g_object_add_weak_pointer (G_OBJECT (window), (gpointer *)&window);

      bobgui_window_set_title (BOBGUI_WINDOW (window), "entry");


      box1 = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 0);
      bobgui_window_set_child (BOBGUI_WINDOW (window), box1);


      box2 = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 10);
      bobgui_widget_set_margin_top (box2, 10);
      bobgui_widget_set_margin_bottom (box2, 10);
      bobgui_widget_set_margin_start (box2, 10);
      bobgui_widget_set_margin_end (box2, 10);
      bobgui_box_append (BOBGUI_BOX (box1), box2);

      hbox = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 5);
      bobgui_box_append (BOBGUI_BOX (box2), hbox);

      entry = bobgui_entry_new ();
      bobgui_editable_set_text (BOBGUI_EDITABLE (entry), "hello world \330\247\331\204\330\263\331\204\330\247\331\205 \330\271\331\204\331\212\331\203\331\205");
      bobgui_editable_select_region (BOBGUI_EDITABLE (entry), 0, 5);
      bobgui_widget_set_hexpand (entry, TRUE);
      bobgui_box_append (BOBGUI_BOX (hbox), entry);

      cb = BOBGUI_COMBO_BOX_TEXT (bobgui_combo_box_text_new_with_entry ());

      bobgui_combo_box_text_append_text (cb, "item0");
      bobgui_combo_box_text_append_text (cb, "item0");
      bobgui_combo_box_text_append_text (cb, "item1 item1");
      bobgui_combo_box_text_append_text (cb, "item2 item2 item2");
      bobgui_combo_box_text_append_text (cb, "item3 item3 item3 item3");
      bobgui_combo_box_text_append_text (cb, "item4 item4 item4 item4 item4");
      bobgui_combo_box_text_append_text (cb, "item5 item5 item5 item5 item5 item5");
      bobgui_combo_box_text_append_text (cb, "item6 item6 item6 item6 item6");
      bobgui_combo_box_text_append_text (cb, "item7 item7 item7 item7");
      bobgui_combo_box_text_append_text (cb, "item8 item8 item8");
      bobgui_combo_box_text_append_text (cb, "item9 item9");

      cb_entry = bobgui_combo_box_get_child (BOBGUI_COMBO_BOX (cb));
      bobgui_editable_set_text (BOBGUI_EDITABLE (cb_entry), "hello world \n\n\n foo");
      bobgui_editable_select_region (BOBGUI_EDITABLE (cb_entry), 0, -1);
      bobgui_box_append (BOBGUI_BOX (box2), BOBGUI_WIDGET (cb));

      sensitive_check = bobgui_check_button_new_with_label ("Sensitive");
      bobgui_box_append (BOBGUI_BOX (box2), sensitive_check);
      g_signal_connect (sensitive_check, "toggled",
			G_CALLBACK (entry_toggle_sensitive), entry);
      bobgui_toggle_button_set_active (BOBGUI_TOGGLE_BUTTON (sensitive_check), TRUE);

      has_frame_check = bobgui_check_button_new_with_label("Has Frame");
      bobgui_box_append (BOBGUI_BOX (box2), has_frame_check);
      g_signal_connect (has_frame_check, "toggled",
			G_CALLBACK (entry_toggle_frame), entry);
      bobgui_toggle_button_set_active (BOBGUI_TOGGLE_BUTTON (has_frame_check), TRUE);

      progress_check = bobgui_check_button_new_with_label("Show Progress");
      bobgui_box_append (BOBGUI_BOX (box2), progress_check);
      g_signal_connect (progress_check, "toggled",
			G_CALLBACK (entry_toggle_progress), entry);

      progress_check = bobgui_check_button_new_with_label("Pulse Progress");
      bobgui_box_append (BOBGUI_BOX (box2), progress_check);
      g_signal_connect (progress_check, "toggled",
			G_CALLBACK (entry_toggle_pulse), entry);

      separator = bobgui_separator_new (BOBGUI_ORIENTATION_HORIZONTAL);
      bobgui_box_append (BOBGUI_BOX (box1), separator);

      box2 = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 10);
      bobgui_widget_set_margin_top (box2, 10);
      bobgui_widget_set_margin_bottom (box2, 10);
      bobgui_widget_set_margin_start (box2, 10);
      bobgui_widget_set_margin_end (box2, 10);
      bobgui_box_append (BOBGUI_BOX (box1), box2);

      button = bobgui_button_new_with_label ("close");
      g_signal_connect_swapped (button, "clicked",
			        G_CALLBACK (bobgui_window_destroy),
				window);
      bobgui_box_append (BOBGUI_BOX (box2), button);
      bobgui_window_set_default_widget (BOBGUI_WINDOW (window), button);
    }

  if (!bobgui_widget_get_visible (window))
    bobgui_window_present (BOBGUI_WINDOW (window));
  else
    bobgui_window_destroy (BOBGUI_WINDOW (window));
}

static void
create_expander (BobguiWidget *widget)
{
  BobguiWidget *box1;
  BobguiWidget *expander;
  BobguiWidget *hidden;
  static BobguiWidget *window = NULL;

  if (!window)
    {
      window = bobgui_window_new ();
      bobgui_window_set_display (BOBGUI_WINDOW (window),
                              bobgui_widget_get_display (widget));
      g_object_add_weak_pointer (G_OBJECT (window), (gpointer *)&window);

      bobgui_window_set_title (BOBGUI_WINDOW (window), "expander");

      box1 = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 0);
      bobgui_window_set_child (BOBGUI_WINDOW (window), box1);

      expander = bobgui_expander_new ("The Hidden");

      bobgui_box_append (BOBGUI_BOX (box1), expander);

      hidden = bobgui_label_new ("Revealed!");

      bobgui_expander_set_child (BOBGUI_EXPANDER (expander), hidden);
    }

  if (!bobgui_widget_get_visible (window))
    bobgui_window_present (BOBGUI_WINDOW (window));
  else
    bobgui_window_destroy (BOBGUI_WINDOW (window));
}


/*
 * BobguiSizeGroup
 */

#define SIZE_GROUP_INITIAL_SIZE 50

static void
size_group_hsize_changed (BobguiSpinButton *spin_button,
			  BobguiWidget     *button)
{
  bobgui_widget_set_size_request (bobgui_button_get_child (BOBGUI_BUTTON (button)),
			       bobgui_spin_button_get_value_as_int (spin_button),
			       -1);
}

static void
size_group_vsize_changed (BobguiSpinButton *spin_button,
			  BobguiWidget     *button)
{
  bobgui_widget_set_size_request (bobgui_button_get_child (BOBGUI_BUTTON (button)),
			       -1,
			       bobgui_spin_button_get_value_as_int (spin_button));
}

static BobguiWidget *
create_size_group_window (GdkDisplay   *display,
			  BobguiSizeGroup *master_size_group)
{
  BobguiWidget *content_area;
  BobguiWidget *window;
  BobguiWidget *vbox;
  BobguiWidget *grid;
  BobguiWidget *main_button;
  BobguiWidget *button;
  BobguiWidget *spin_button;
  BobguiWidget *hbox;
  BobguiSizeGroup *hgroup1;
  BobguiSizeGroup *hgroup2;
  BobguiSizeGroup *vgroup1;
  BobguiSizeGroup *vgroup2;

  window = bobgui_dialog_new_with_buttons ("BobguiSizeGroup",
					NULL, 0,
					"_Close",
					BOBGUI_RESPONSE_NONE,
					NULL);

  bobgui_window_set_display (BOBGUI_WINDOW (window), display);

  bobgui_window_set_resizable (BOBGUI_WINDOW (window), TRUE);

  g_signal_connect (window, "response",
		    G_CALLBACK (bobgui_window_destroy),
		    NULL);

  content_area = bobgui_dialog_get_content_area (BOBGUI_DIALOG (window));

  vbox = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 0);
  bobgui_box_append (BOBGUI_BOX (content_area), vbox);

  grid = bobgui_grid_new ();
  bobgui_box_append (BOBGUI_BOX (content_area), grid);

  bobgui_grid_set_row_spacing (BOBGUI_GRID (grid), 5);
  bobgui_grid_set_column_spacing (BOBGUI_GRID (grid), 5);
  bobgui_widget_set_size_request (grid, 250, 250);

  hgroup1 = bobgui_size_group_new (BOBGUI_SIZE_GROUP_HORIZONTAL);
  hgroup2 = bobgui_size_group_new (BOBGUI_SIZE_GROUP_HORIZONTAL);
  vgroup1 = bobgui_size_group_new (BOBGUI_SIZE_GROUP_VERTICAL);
  vgroup2 = bobgui_size_group_new (BOBGUI_SIZE_GROUP_VERTICAL);

  main_button = bobgui_button_new_with_label ("X");
  bobgui_widget_set_hexpand (main_button, TRUE);
  bobgui_widget_set_vexpand (main_button, TRUE);
  bobgui_widget_set_halign (main_button, BOBGUI_ALIGN_CENTER);
  bobgui_widget_set_valign (main_button, BOBGUI_ALIGN_CENTER);
  bobgui_grid_attach (BOBGUI_GRID (grid), main_button, 0, 0, 1, 1);
  
  bobgui_size_group_add_widget (master_size_group, main_button);
  bobgui_size_group_add_widget (hgroup1, main_button);
  bobgui_size_group_add_widget (vgroup1, main_button);
  bobgui_widget_set_size_request (bobgui_button_get_child (BOBGUI_BUTTON (main_button)),
			       SIZE_GROUP_INITIAL_SIZE,
			       SIZE_GROUP_INITIAL_SIZE);

  button = bobgui_button_new ();
  bobgui_widget_set_hexpand (button, TRUE);
  bobgui_widget_set_vexpand (button, TRUE);
  bobgui_widget_set_halign (button, BOBGUI_ALIGN_CENTER);
  bobgui_widget_set_valign (button, BOBGUI_ALIGN_CENTER);
  bobgui_grid_attach (BOBGUI_GRID (grid), button, 1, 0, 1, 1);

  bobgui_size_group_add_widget (vgroup1, button);
  bobgui_size_group_add_widget (vgroup2, button);

  button = bobgui_button_new ();
  bobgui_widget_set_hexpand (button, TRUE);
  bobgui_widget_set_vexpand (button, TRUE);
  bobgui_widget_set_halign (button, BOBGUI_ALIGN_CENTER);
  bobgui_widget_set_valign (button, BOBGUI_ALIGN_CENTER);
  bobgui_grid_attach (BOBGUI_GRID (grid), button, 0, 1, 1, 1);

  bobgui_size_group_add_widget (hgroup1, button);
  bobgui_size_group_add_widget (hgroup2, button);

  button = bobgui_button_new ();
  bobgui_widget_set_hexpand (button, TRUE);
  bobgui_widget_set_vexpand (button, TRUE);
  bobgui_widget_set_halign (button, BOBGUI_ALIGN_CENTER);
  bobgui_widget_set_valign (button, BOBGUI_ALIGN_CENTER);
  bobgui_grid_attach (BOBGUI_GRID (grid), button, 1, 1, 1, 1);

  bobgui_size_group_add_widget (hgroup2, button);
  bobgui_size_group_add_widget (vgroup2, button);

  g_object_unref (hgroup1);
  g_object_unref (hgroup2);
  g_object_unref (vgroup1);
  g_object_unref (vgroup2);

  hbox = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 5);
  bobgui_box_append (BOBGUI_BOX (content_area), hbox);

  spin_button = bobgui_spin_button_new_with_range (1, 100, 1);
  bobgui_spin_button_set_value (BOBGUI_SPIN_BUTTON (spin_button), SIZE_GROUP_INITIAL_SIZE);
  bobgui_widget_set_hexpand (spin_button, TRUE);
  bobgui_box_append (BOBGUI_BOX (hbox), spin_button);
  g_signal_connect (spin_button, "value_changed",
		    G_CALLBACK (size_group_hsize_changed), main_button);

  spin_button = bobgui_spin_button_new_with_range (1, 100, 1);
  bobgui_spin_button_set_value (BOBGUI_SPIN_BUTTON (spin_button), SIZE_GROUP_INITIAL_SIZE);
  bobgui_widget_set_hexpand (spin_button, TRUE);
  bobgui_box_append (BOBGUI_BOX (hbox), spin_button);
  g_signal_connect (spin_button, "value_changed",
		    G_CALLBACK (size_group_vsize_changed), main_button);

  return window;
}

static void
create_size_groups (BobguiWidget *widget)
{
  static BobguiWidget *window1 = NULL;
  static BobguiWidget *window2 = NULL;
  static BobguiSizeGroup *master_size_group;

  if (!master_size_group)
    master_size_group = bobgui_size_group_new (BOBGUI_SIZE_GROUP_BOTH);

  if (!window1)
    {
      window1 = create_size_group_window (bobgui_widget_get_display (widget),
                                          master_size_group);
      g_object_add_weak_pointer (G_OBJECT (window1), (gpointer *)&window1);
    }

  if (!window2)
    {
      window2 = create_size_group_window (bobgui_widget_get_display (widget),
                                          master_size_group);
      g_object_add_weak_pointer (G_OBJECT (window2), (gpointer *)&window2);
    }

  if (bobgui_widget_get_visible (window1) && bobgui_widget_get_visible (window2))
    {
      bobgui_window_destroy (BOBGUI_WINDOW (window1));
      bobgui_window_destroy (BOBGUI_WINDOW (window2));
    }
  else
    {
      if (!bobgui_widget_get_visible (window1))
	bobgui_widget_show (window1);
      if (!bobgui_widget_get_visible (window2))
	bobgui_widget_show (window2);
    }
}

/*
 * BobguiSpinButton
 */

static BobguiWidget *spinner1;

static void
toggle_snap (BobguiWidget *widget, BobguiSpinButton *spin)
{
  bobgui_spin_button_set_snap_to_ticks (spin,
                                     bobgui_toggle_button_get_active (BOBGUI_TOGGLE_BUTTON (widget)));
}

static void
toggle_numeric (BobguiWidget *widget, BobguiSpinButton *spin)
{
  bobgui_spin_button_set_numeric (spin,
                               bobgui_toggle_button_get_active (BOBGUI_TOGGLE_BUTTON (widget)));
}

static void
change_digits (BobguiWidget *widget, BobguiSpinButton *spin)
{
  bobgui_spin_button_set_digits (BOBGUI_SPIN_BUTTON (spinner1),
			      bobgui_spin_button_get_value_as_int (spin));
}

static void
get_value (BobguiWidget *widget, gpointer data)
{
  char buf[32];
  BobguiLabel *label;
  BobguiSpinButton *spin;

  spin = BOBGUI_SPIN_BUTTON (spinner1);
  label = BOBGUI_LABEL (g_object_get_data (G_OBJECT (widget), "user_data"));
  if (GPOINTER_TO_INT (data) == 1)
    sprintf (buf, "%d", bobgui_spin_button_get_value_as_int (spin));
  else
    sprintf (buf, "%0.*f",
             bobgui_spin_button_get_digits (spin),
             bobgui_spin_button_get_value (spin));

  bobgui_label_set_text (label, buf);
}

static void
get_spin_value (BobguiWidget *widget, gpointer data)
{
  char *buffer;
  BobguiLabel *label;
  BobguiSpinButton *spin;

  spin = BOBGUI_SPIN_BUTTON (widget);
  label = BOBGUI_LABEL (data);

  buffer = g_strdup_printf ("%0.*f",
                            bobgui_spin_button_get_digits (spin),
			    bobgui_spin_button_get_value (spin));
  bobgui_label_set_text (label, buffer);

  g_free (buffer);
}

static int
spin_button_time_output_func (BobguiSpinButton *spin_button)
{
  BobguiAdjustment *adjustment;
  static char buf[6];
  double hours;
  double minutes;

  adjustment = bobgui_spin_button_get_adjustment (spin_button);
  hours = bobgui_adjustment_get_value (adjustment) / 60.0;
  minutes = (fabs(floor (hours) - hours) < 1e-5) ? 0.0 : 30;
  sprintf (buf, "%02.0f:%02.0f", floor (hours), minutes);
  if (strcmp (buf, bobgui_editable_get_text (BOBGUI_EDITABLE (spin_button))))
    bobgui_editable_set_text (BOBGUI_EDITABLE (spin_button), buf);
  return TRUE;
}

static int
spin_button_month_input_func (BobguiSpinButton *spin_button,
			      double        *new_val)
{
  int i;
  static const char *month[12] = { "January", "February", "March", "April",
			      "May", "June", "July", "August",
			      "September", "October", "November", "December" };
  char *tmp1, *tmp2;
  gboolean found = FALSE;

  for (i = 1; i <= 12; i++)
    {
      tmp1 = g_ascii_strup (month[i - 1], -1);
      tmp2 = g_ascii_strup (bobgui_editable_get_text (BOBGUI_EDITABLE (spin_button)), -1);
      if (strstr (tmp1, tmp2) == tmp1)
	found = TRUE;
      g_free (tmp1);
      g_free (tmp2);
      if (found)
	break;
    }
  if (!found)
    {
      *new_val = 0.0;
      return BOBGUI_INPUT_ERROR;
    }
  *new_val = (double) i;
  return TRUE;
}

static int
spin_button_month_output_func (BobguiSpinButton *spin_button)
{
  BobguiAdjustment *adjustment;
  double value;
  int i;
  static const char *month[12] = { "January", "February", "March", "April",
			      "May", "June", "July", "August", "September",
			      "October", "November", "December" };

  adjustment = bobgui_spin_button_get_adjustment (spin_button);
  value = bobgui_adjustment_get_value (adjustment);
  for (i = 1; i <= 12; i++)
    if (fabs (value - (double)i) < 1e-5)
      {
        if (strcmp (month[i-1], bobgui_editable_get_text (BOBGUI_EDITABLE (spin_button))))
          bobgui_editable_set_text (BOBGUI_EDITABLE (spin_button), month[i-1]);
      }
  return TRUE;
}

static int
spin_button_hex_input_func (BobguiSpinButton *spin_button,
			    double        *new_val)
{
  const char *buf;
  char *err;
  double res;

  buf = bobgui_editable_get_text (BOBGUI_EDITABLE (spin_button));
  res = strtol(buf, &err, 16);
  *new_val = res;
  if (*err)
    return BOBGUI_INPUT_ERROR;
  else
    return TRUE;
}

static int
spin_button_hex_output_func (BobguiSpinButton *spin_button)
{
  BobguiAdjustment *adjustment;
  static char buf[7];
  double val;

  adjustment = bobgui_spin_button_get_adjustment (spin_button);
  val = bobgui_adjustment_get_value (adjustment);
  if (fabs (val) < 1e-5)
    sprintf (buf, "0x00");
  else
    sprintf (buf, "0x%.2X", (int) val);
  if (strcmp (buf, bobgui_editable_get_text (BOBGUI_EDITABLE (spin_button))))
    bobgui_editable_set_text (BOBGUI_EDITABLE (spin_button), buf);

  return TRUE;
}

static void
create_spins (BobguiWidget *widget)
{
  static BobguiWidget *window = NULL;
  BobguiWidget *frame;
  BobguiWidget *hbox;
  BobguiWidget *main_vbox;
  BobguiWidget *vbox;
  BobguiWidget *vbox2;
  BobguiWidget *spinner2;
  BobguiWidget *spinner;
  BobguiWidget *button;
  BobguiWidget *label;
  BobguiWidget *val_label;
  BobguiAdjustment *adjustment;

  if (!window)
    {
      window = bobgui_window_new ();
      bobgui_window_set_display (BOBGUI_WINDOW (window),
                              bobgui_widget_get_display (widget));
      g_object_add_weak_pointer (G_OBJECT (window), (gpointer *)&window);

      bobgui_window_set_title (BOBGUI_WINDOW (window), "BobguiSpinButton");

      main_vbox = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 5);
      bobgui_window_set_child (BOBGUI_WINDOW (window), main_vbox);

      frame = bobgui_frame_new ("Not accelerated");
      bobgui_box_append (BOBGUI_BOX (main_vbox), frame);

      vbox = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 0);
      bobgui_frame_set_child (BOBGUI_FRAME (frame), vbox);

      /* Time, month, hex spinners */

      hbox = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 0);
      bobgui_box_append (BOBGUI_BOX (vbox), hbox);

      vbox2 = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 0);
      bobgui_widget_set_hexpand (vbox2, TRUE);
      bobgui_box_append (BOBGUI_BOX (hbox), vbox2);

      label = bobgui_label_new ("Time :");
      bobgui_widget_set_halign (label, BOBGUI_ALIGN_START);
      bobgui_widget_set_valign (label, BOBGUI_ALIGN_CENTER);
      bobgui_box_append (BOBGUI_BOX (vbox2), label);

      adjustment = bobgui_adjustment_new (0, 0, 1410, 30, 60, 0);
      spinner = bobgui_spin_button_new (adjustment, 0, 0);
      bobgui_editable_set_editable (BOBGUI_EDITABLE (spinner), FALSE);
      g_signal_connect (spinner,
			"output",
			G_CALLBACK (spin_button_time_output_func),
			NULL);
      bobgui_spin_button_set_wrap (BOBGUI_SPIN_BUTTON (spinner), TRUE);
      bobgui_editable_set_width_chars (BOBGUI_EDITABLE (spinner), 5);
      bobgui_box_append (BOBGUI_BOX (vbox2), spinner);

      vbox2 = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 0);
      bobgui_widget_set_hexpand (vbox2, TRUE);
      bobgui_box_append (BOBGUI_BOX (hbox), vbox2);

      label = bobgui_label_new ("Month :");
      bobgui_widget_set_halign (label, BOBGUI_ALIGN_START);
      bobgui_widget_set_valign (label, BOBGUI_ALIGN_CENTER);
      bobgui_box_append (BOBGUI_BOX (vbox2), label);

      adjustment = bobgui_adjustment_new (1.0, 1.0, 12.0, 1.0,
						  5.0, 0.0);
      spinner = bobgui_spin_button_new (adjustment, 0, 0);
      bobgui_spin_button_set_update_policy (BOBGUI_SPIN_BUTTON (spinner),
					 BOBGUI_UPDATE_IF_VALID);
      g_signal_connect (spinner,
			"input",
			G_CALLBACK (spin_button_month_input_func),
			NULL);
      g_signal_connect (spinner,
			"output",
			G_CALLBACK (spin_button_month_output_func),
			NULL);
      bobgui_spin_button_set_wrap (BOBGUI_SPIN_BUTTON (spinner), TRUE);
      bobgui_editable_set_width_chars (BOBGUI_EDITABLE (spinner), 9);
      bobgui_box_append (BOBGUI_BOX (vbox2), spinner);

      vbox2 = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 0);
      bobgui_widget_set_hexpand (vbox2, TRUE);
      bobgui_box_append (BOBGUI_BOX (hbox), vbox2);

      label = bobgui_label_new ("Hex :");
      bobgui_widget_set_halign (label, BOBGUI_ALIGN_START);
      bobgui_widget_set_valign (label, BOBGUI_ALIGN_CENTER);
      bobgui_box_append (BOBGUI_BOX (vbox2), label);

      adjustment = bobgui_adjustment_new (0, 0, 255, 1, 16, 0);
      spinner = bobgui_spin_button_new (adjustment, 0, 0);
      bobgui_editable_set_editable (BOBGUI_EDITABLE (spinner), TRUE);
      g_signal_connect (spinner,
			"input",
			G_CALLBACK (spin_button_hex_input_func),
			NULL);
      g_signal_connect (spinner,
			"output",
			G_CALLBACK (spin_button_hex_output_func),
			NULL);
      bobgui_spin_button_set_wrap (BOBGUI_SPIN_BUTTON (spinner), TRUE);
      bobgui_editable_set_width_chars (BOBGUI_EDITABLE (spinner), 4);
      bobgui_box_append (BOBGUI_BOX (vbox2), spinner);

      frame = bobgui_frame_new ("Accelerated");
      bobgui_box_append (BOBGUI_BOX (main_vbox), frame);

      vbox = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 0);
      bobgui_frame_set_child (BOBGUI_FRAME (frame), vbox);

      hbox = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 0);
      bobgui_box_append (BOBGUI_BOX (vbox), hbox);

      vbox2 = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 0);
      bobgui_box_append (BOBGUI_BOX (hbox), vbox2);

      label = bobgui_label_new ("Value :");
      bobgui_widget_set_halign (label, BOBGUI_ALIGN_START);
      bobgui_widget_set_valign (label, BOBGUI_ALIGN_CENTER);
      bobgui_box_append (BOBGUI_BOX (vbox2), label);

      adjustment = bobgui_adjustment_new (0.0, -10000.0, 10000.0,
						  0.5, 100.0, 0.0);
      spinner1 = bobgui_spin_button_new (adjustment, 1.0, 2);
      bobgui_spin_button_set_wrap (BOBGUI_SPIN_BUTTON (spinner1), TRUE);
      bobgui_box_append (BOBGUI_BOX (vbox2), spinner1);

      vbox2 = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 0);
      bobgui_box_append (BOBGUI_BOX (hbox), vbox2);

      label = bobgui_label_new ("Digits :");
      bobgui_widget_set_halign (label, BOBGUI_ALIGN_START);
      bobgui_widget_set_valign (label, BOBGUI_ALIGN_CENTER);
      bobgui_box_append (BOBGUI_BOX (vbox2), label);

      adjustment = bobgui_adjustment_new (2, 1, 15, 1, 1, 0);
      spinner2 = bobgui_spin_button_new (adjustment, 0.0, 0);
      g_signal_connect (adjustment, "value_changed",
			G_CALLBACK (change_digits),
			spinner2);
      bobgui_box_append (BOBGUI_BOX (vbox2), spinner2);

      hbox = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 0);
      bobgui_box_append (BOBGUI_BOX (vbox), hbox);

      button = bobgui_check_button_new_with_label ("Snap to 0.5-ticks");
      g_signal_connect (button, "clicked",
			G_CALLBACK (toggle_snap),
			spinner1);
      bobgui_box_append (BOBGUI_BOX (vbox), button);
      bobgui_toggle_button_set_active (BOBGUI_TOGGLE_BUTTON (button), TRUE);

      button = bobgui_check_button_new_with_label ("Numeric only input mode");
      g_signal_connect (button, "clicked",
			G_CALLBACK (toggle_numeric),
			spinner1);
      bobgui_box_append (BOBGUI_BOX (vbox), button);
      bobgui_toggle_button_set_active (BOBGUI_TOGGLE_BUTTON (button), TRUE);

      val_label = bobgui_label_new ("");

      hbox = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 0);
      bobgui_box_append (BOBGUI_BOX (vbox), hbox);

      button = bobgui_button_new_with_label ("Value as Int");
      bobgui_widget_set_hexpand (button, TRUE);
      g_object_set_data (G_OBJECT (button), "user_data", val_label);
      g_signal_connect (button, "clicked",
			G_CALLBACK (get_value),
			GINT_TO_POINTER (1));
      bobgui_box_append (BOBGUI_BOX (hbox), button);

      button = bobgui_button_new_with_label ("Value as Float");
      bobgui_widget_set_hexpand (button, TRUE);
      g_object_set_data (G_OBJECT (button), "user_data", val_label);
      g_signal_connect (button, "clicked",
			G_CALLBACK (get_value),
			GINT_TO_POINTER (2));
      bobgui_box_append (BOBGUI_BOX (hbox), button);

      bobgui_box_append (BOBGUI_BOX (vbox), val_label);
      bobgui_label_set_text (BOBGUI_LABEL (val_label), "0");

      frame = bobgui_frame_new ("Using Convenience Constructor");
      bobgui_box_append (BOBGUI_BOX (main_vbox), frame);

      hbox = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 0);
      bobgui_frame_set_child (BOBGUI_FRAME (frame), hbox);

      val_label = bobgui_label_new ("0.0");
      bobgui_widget_set_hexpand (val_label, TRUE);

      spinner = bobgui_spin_button_new_with_range (0.0, 10.0, 0.009);
      bobgui_widget_set_hexpand (spinner, TRUE);
      bobgui_spin_button_set_value (BOBGUI_SPIN_BUTTON (spinner), 0.0);
      g_signal_connect (spinner, "value_changed",
			G_CALLBACK (get_spin_value), val_label);
      bobgui_box_append (BOBGUI_BOX (hbox), spinner);
      bobgui_box_append (BOBGUI_BOX (hbox), val_label);

      hbox = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 0);
      bobgui_box_append (BOBGUI_BOX (main_vbox), hbox);

      button = bobgui_button_new_with_label ("Close");
      bobgui_widget_set_hexpand (button, TRUE);
      g_signal_connect_swapped (button, "clicked",
			        G_CALLBACK (bobgui_window_destroy),
				window);
      bobgui_box_append (BOBGUI_BOX (hbox), button);
    }

  if (!bobgui_widget_get_visible (window))
    bobgui_window_present (BOBGUI_WINDOW (window));
  else
    bobgui_window_destroy (BOBGUI_WINDOW (window));
}


/*
 * Cursors
 */

static void
cursor_draw (BobguiDrawingArea *darea,
	     cairo_t        *cr,
             int             width,
             int             height,
	     gpointer        user_data)
{
  cairo_set_fill_rule (cr, CAIRO_FILL_RULE_EVEN_ODD);
  cairo_rectangle (cr, 0, 0, width, height);
  cairo_rectangle (cr, width / 3, height / 3, width / 3, height / 3);
  cairo_clip (cr);

  cairo_set_source_rgb (cr, 1, 1, 1);
  cairo_rectangle (cr, 0, 0, width, height / 2);
  cairo_fill (cr);

  cairo_set_source_rgb (cr, 0, 0, 0);
  cairo_rectangle (cr, 0, height / 2, width, height / 2);
  cairo_fill (cr);
}

static const char *cursor_names[] = {
  "none",
  "default",
  "help",
  "pointer",
  "context-menu",
  "progress",
  "wait",
  "cell",
  "crosshair",
  "text",
  "vertical-text",
  "alias",
  "copy",
  "no-drop",
  "move",
  "not-allowed",
  "grab",
  "grabbing",
  "all-scroll",
  "col-resize",
  "row-resize",
  "n-resize",
  "e-resize",
  "s-resize",
  "w-resize",
  "ne-resize",
  "nw-resize",
  "sw-resize",
  "se-resize",
  "ew-resize",
  "ns-resize",
  "nesw-resize",
  "nwse-resize",
  "zoom-in",
  "zoom-out",
  NULL
};

static BobguiTreeModel *
cursor_model (void)
{
  BobguiListStore *store;
  int i;
  store = bobgui_list_store_new (1, G_TYPE_STRING);

  for (i = 0; i < G_N_ELEMENTS (cursor_names); i++)
    bobgui_list_store_insert_with_values (store, NULL, -1, 0, cursor_names[i], -1);

  return (BobguiTreeModel *)store;
}

static void
cursor_pressed_cb (BobguiGesture *gesture,
                   int         n_pressed,
                   double      x,
                   double      y,
                   BobguiWidget  *entry)
{
  BobguiWidget *widget;
  const char *name;
  int i;
  const int n = G_N_ELEMENTS (cursor_names);
  int button;

  widget = bobgui_event_controller_get_widget (BOBGUI_EVENT_CONTROLLER (gesture));

  name = (const char *)g_object_get_data (G_OBJECT (widget), "name");
  if (name != NULL)
    {
      for (i = 0; i < n; i++)
        if (strcmp (name, cursor_names[i]) == 0)
          break;
    }
  else
    i = 0;

  button = bobgui_gesture_single_get_current_button (BOBGUI_GESTURE_SINGLE (gesture));
  if (button == GDK_BUTTON_PRIMARY)
    i = (i + 1) % n;
  else
    i = (i + n - 1) % n;

  bobgui_editable_set_text (BOBGUI_EDITABLE (entry), cursor_names[i]);
}

static void
set_cursor_from_name (BobguiWidget *entry,
                      BobguiWidget *widget)
{
  const char *name;

  name = bobgui_editable_get_text (BOBGUI_EDITABLE (entry));
  bobgui_widget_set_cursor_from_name (widget, name);

  g_object_set_data_full (G_OBJECT (widget), "name", g_strdup (name), g_free);
}

#ifdef GDK_WINDOWING_X11
#include "x11/gdkx.h"
#endif
#ifdef GDK_WINDOWING_WAYLAND
#include "wayland/gdkwayland.h"
#endif

static void
change_cursor_theme (BobguiWidget *widget,
                     gpointer   data)
{
#if defined(GDK_WINDOWING_X11) || defined (GDK_WINDOWING_WAYLAND)
  const char *theme;
  int size;
  GdkDisplay *display;
  BobguiWidget *entry;
  BobguiWidget *spin;

  entry = bobgui_widget_get_next_sibling (bobgui_widget_get_first_child (BOBGUI_WIDGET (data)));
  spin = bobgui_widget_get_next_sibling (entry);

  theme = bobgui_editable_get_text (BOBGUI_EDITABLE (entry));
  size = (int) bobgui_spin_button_get_value (BOBGUI_SPIN_BUTTON (spin));

  display = bobgui_widget_get_display (widget);
#ifdef GDK_WINDOWING_X11
  if (GDK_IS_X11_DISPLAY (display))
    gdk_x11_display_set_cursor_theme (display, theme, size);
#endif
#ifdef GDK_WINDOWING_WAYLAND
  if (GDK_IS_WAYLAND_DISPLAY (display))
    gdk_wayland_display_set_cursor_theme (display, theme, size);
#endif
#endif
}


static void
create_cursors (BobguiWidget *widget)
{
  static BobguiWidget *window = NULL;
  BobguiWidget *frame;
  BobguiWidget *hbox;
  BobguiWidget *main_vbox;
  BobguiWidget *vbox;
  BobguiWidget *darea;
  BobguiWidget *button;
  BobguiWidget *label;
  BobguiWidget *any;
  BobguiWidget *entry;
  BobguiWidget *size;
  BobguiEntryCompletion *completion;
  BobguiTreeModel *model;
  gboolean cursor_demo = FALSE;
  BobguiGesture *gesture;

  if (!window)
    {
      window = bobgui_window_new ();
      bobgui_window_set_display (BOBGUI_WINDOW (window),
                              bobgui_widget_get_display (widget));
      g_object_add_weak_pointer (G_OBJECT (window), (gpointer *)&window);

      bobgui_window_set_title (BOBGUI_WINDOW (window), "Cursors");

      main_vbox = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 5);
      bobgui_window_set_child (BOBGUI_WINDOW (window), main_vbox);

      vbox = g_object_new (BOBGUI_TYPE_BOX,
                           "orientation", BOBGUI_ORIENTATION_VERTICAL,
                           "homogeneous", FALSE,
                           "spacing", 5,
                           NULL);
      bobgui_box_append (BOBGUI_BOX (main_vbox), vbox);

#ifdef GDK_WINDOWING_X11
      if (GDK_IS_X11_DISPLAY (bobgui_widget_get_display (vbox)))
        cursor_demo = TRUE;
#endif
#ifdef GDK_WINDOWING_WAYLAND
      if (GDK_IS_WAYLAND_DISPLAY (bobgui_widget_get_display (vbox)))
        cursor_demo = TRUE;
#endif

    if (cursor_demo)
        {
          hbox = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 6);
          bobgui_widget_set_margin_top (hbox, 5);
          bobgui_widget_set_margin_bottom (hbox, 5);
          bobgui_widget_set_margin_start (hbox, 5);
          bobgui_widget_set_margin_end (hbox, 5);
          bobgui_box_append (BOBGUI_BOX (vbox), hbox);

          label = bobgui_label_new ("Cursor Theme:");
          bobgui_widget_set_halign (label, BOBGUI_ALIGN_START);
          bobgui_widget_set_valign (label, BOBGUI_ALIGN_CENTER);
          bobgui_box_append (BOBGUI_BOX (hbox), label);

          entry = bobgui_entry_new ();
          bobgui_editable_set_text (BOBGUI_EDITABLE (entry), "default");
          bobgui_box_append (BOBGUI_BOX (hbox), entry);

          size = bobgui_spin_button_new_with_range (1.0, 128.0, 1.0);
          bobgui_spin_button_set_value (BOBGUI_SPIN_BUTTON (size), 24.0);
          bobgui_widget_set_hexpand (size, TRUE);
          bobgui_box_append (BOBGUI_BOX (hbox), size);

          g_signal_connect (entry, "changed",
                            G_CALLBACK (change_cursor_theme), hbox);
          g_signal_connect (size, "value-changed",
                            G_CALLBACK (change_cursor_theme), hbox);
        }

      hbox = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 6);
      bobgui_widget_set_margin_top (hbox, 5);
      bobgui_widget_set_margin_bottom (hbox, 5);
      bobgui_widget_set_margin_start (hbox, 5);
      bobgui_widget_set_margin_end (hbox, 5);
      bobgui_box_append (BOBGUI_BOX (vbox), hbox);

      label = bobgui_label_new ("Cursor Name:");
      bobgui_widget_set_halign (label, BOBGUI_ALIGN_START);
      bobgui_widget_set_valign (label, BOBGUI_ALIGN_CENTER);
      bobgui_box_append (BOBGUI_BOX (hbox), label);

      entry = bobgui_entry_new ();

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

      completion = bobgui_entry_completion_new ();
      model = cursor_model ();
      bobgui_entry_completion_set_model (completion, model);
      bobgui_entry_completion_set_text_column (completion, 0);
      bobgui_entry_set_completion (BOBGUI_ENTRY (entry), completion);

G_GNUC_END_IGNORE_DEPRECATIONS

      g_object_unref (model);
      bobgui_widget_set_hexpand (entry, TRUE);
      bobgui_box_append (BOBGUI_BOX (hbox), entry);

      frame =
	g_object_new (bobgui_frame_get_type (),
			"label_xalign", 0.5,
			"label", "Cursor Area",
			NULL);
      bobgui_box_append (BOBGUI_BOX (vbox), frame);

      darea = bobgui_drawing_area_new ();
      bobgui_drawing_area_set_content_width (BOBGUI_DRAWING_AREA (darea), 80);
      bobgui_drawing_area_set_content_height (BOBGUI_DRAWING_AREA (darea), 80);
      bobgui_drawing_area_set_draw_func (BOBGUI_DRAWING_AREA (darea), cursor_draw, NULL, NULL);
      bobgui_frame_set_child (BOBGUI_FRAME (frame), darea);
      gesture = bobgui_gesture_click_new ();
      bobgui_gesture_single_set_button (BOBGUI_GESTURE_SINGLE (gesture), 0);
      g_signal_connect (gesture, "pressed", G_CALLBACK (cursor_pressed_cb), entry);
      bobgui_widget_add_controller (darea, BOBGUI_EVENT_CONTROLLER (gesture));

      g_signal_connect (entry, "changed",
                        G_CALLBACK (set_cursor_from_name), darea);


      any = bobgui_separator_new (BOBGUI_ORIENTATION_HORIZONTAL);
      bobgui_box_append (BOBGUI_BOX (main_vbox), any);

      hbox = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 0);
      bobgui_widget_set_margin_top (hbox, 10);
      bobgui_widget_set_margin_bottom (hbox, 10);
      bobgui_widget_set_margin_start (hbox, 10);
      bobgui_widget_set_margin_end (hbox, 10);
      bobgui_box_append (BOBGUI_BOX (main_vbox), hbox);

      button = bobgui_button_new_with_label ("Close");
      bobgui_widget_set_hexpand (button, TRUE);
      g_signal_connect_swapped (button, "clicked",
			        G_CALLBACK (bobgui_window_destroy),
				window);
      bobgui_box_append (BOBGUI_BOX (hbox), button);

      bobgui_window_present (BOBGUI_WINDOW (window));

      bobgui_editable_set_text (BOBGUI_EDITABLE (entry), "arrow");
    }
  else
    bobgui_window_destroy (BOBGUI_WINDOW (window));
}

/*
 * BobguiColorSelection
 */

static void
create_color_selection (BobguiWidget *widget)
{
  static BobguiWidget *window = NULL;

  if (!window)
    {
      BobguiWidget *picker;
      BobguiWidget *hbox;
      BobguiWidget *label;

      window = bobgui_window_new ();
      bobgui_window_set_display (BOBGUI_WINDOW (window),
                              bobgui_widget_get_display (widget));
      g_object_add_weak_pointer (G_OBJECT (window), (gpointer *)&window);

      bobgui_window_set_title (BOBGUI_WINDOW (window), "BobguiColorButton");

      hbox = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 8);
      bobgui_widget_set_margin_top (hbox, 5);
      bobgui_widget_set_margin_bottom (hbox, 5);
      bobgui_widget_set_margin_start (hbox, 5);
      bobgui_widget_set_margin_end (hbox, 5);
      bobgui_window_set_child (BOBGUI_WINDOW (window), hbox);
      
      label = bobgui_label_new ("Pick a color");
      bobgui_box_append (BOBGUI_BOX (hbox), label);

      picker = bobgui_color_button_new ();
      bobgui_color_chooser_set_use_alpha (BOBGUI_COLOR_CHOOSER (picker), TRUE);
      bobgui_box_append (BOBGUI_BOX (hbox), picker);
    }

  if (!bobgui_widget_get_visible (window))
    bobgui_window_present (BOBGUI_WINDOW (window));
  else
    bobgui_window_destroy (BOBGUI_WINDOW (window));
}

static void
flipping_toggled_cb (BobguiWidget *widget, gpointer data)
{
  int state = bobgui_toggle_button_get_active (BOBGUI_TOGGLE_BUTTON (widget));
  int new_direction = state ? BOBGUI_TEXT_DIR_RTL : BOBGUI_TEXT_DIR_LTR;

  bobgui_widget_set_default_direction (new_direction);
}

static void
orientable_toggle_orientation (BobguiOrientable *orientable)
{
  BobguiOrientation orientation;
  BobguiWidget *child;

  orientation = bobgui_orientable_get_orientation (orientable);
  bobgui_orientable_set_orientation (orientable,
                                  orientation == BOBGUI_ORIENTATION_HORIZONTAL ?
                                  BOBGUI_ORIENTATION_VERTICAL :
                                  BOBGUI_ORIENTATION_HORIZONTAL);


  for (child = bobgui_widget_get_first_child (BOBGUI_WIDGET (orientable));
       child != NULL;
       child = bobgui_widget_get_next_sibling (child))
    {
      if (BOBGUI_IS_ORIENTABLE (child))
        orientable_toggle_orientation (BOBGUI_ORIENTABLE (child));
    }
}

static void
flipping_orientation_toggled_cb (BobguiWidget *widget, gpointer data)
{
  BobguiWidget *content_area;
  BobguiWidget *toplevel;

  toplevel = BOBGUI_WIDGET (bobgui_widget_get_root (widget));
  content_area = bobgui_dialog_get_content_area (BOBGUI_DIALOG (toplevel));
  orientable_toggle_orientation (BOBGUI_ORIENTABLE (content_area));
}

static void
set_direction_recurse (BobguiWidget *widget,
                       gpointer   data)
{
  BobguiTextDirection *dir = data;
  BobguiWidget *child;

  bobgui_widget_set_direction (widget, *dir);
  for (child = bobgui_widget_get_first_child (widget);
       child != NULL;
       child = bobgui_widget_get_next_sibling (child))
    set_direction_recurse (child, data);
}

static BobguiWidget *
create_forward_back (const char       *title,
		     BobguiTextDirection  text_dir)
{
  BobguiWidget *frame = bobgui_frame_new (title);
  BobguiWidget *bbox = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 0);
  BobguiWidget *back_button = bobgui_button_new_with_label ("Back");
  BobguiWidget *forward_button = bobgui_button_new_with_label ("Forward");

  bobgui_frame_set_child (BOBGUI_FRAME (frame), bbox);
  bobgui_box_append (BOBGUI_BOX (bbox), back_button);
  bobgui_box_append (BOBGUI_BOX (bbox), forward_button);

  set_direction_recurse (frame, &text_dir);

  return frame;
}

static void
create_flipping (BobguiWidget *widget)
{
  static BobguiWidget *window = NULL;
  BobguiWidget *check_button;
  BobguiWidget *content_area;

  if (!window)
    {
      window = bobgui_dialog_new ();

      bobgui_window_set_display (BOBGUI_WINDOW (window),
                              bobgui_widget_get_display (widget));
      g_object_add_weak_pointer (G_OBJECT (window), (gpointer *)&window);

      content_area = bobgui_dialog_get_content_area (BOBGUI_DIALOG (window));

      bobgui_window_set_title (BOBGUI_WINDOW (window), "Bidirectional Flipping");

      check_button = bobgui_check_button_new_with_label ("Right-to-left global direction");
      bobgui_box_append (BOBGUI_BOX (content_area), check_button);

      if (bobgui_widget_get_default_direction () == BOBGUI_TEXT_DIR_RTL)
	bobgui_toggle_button_set_active (BOBGUI_TOGGLE_BUTTON (check_button), TRUE);

      g_signal_connect (check_button, "toggled",
			G_CALLBACK (flipping_toggled_cb), NULL);

      check_button = bobgui_check_button_new_with_label ("Toggle orientation of all boxes");
      bobgui_box_append (BOBGUI_BOX (content_area), check_button);

      g_signal_connect (check_button, "toggled",
			G_CALLBACK (flipping_orientation_toggled_cb), NULL);

      bobgui_box_append (BOBGUI_BOX (content_area),
			  create_forward_back ("Default", BOBGUI_TEXT_DIR_NONE));

      bobgui_box_append (BOBGUI_BOX (content_area),
			  create_forward_back ("Left-to-Right", BOBGUI_TEXT_DIR_LTR));

      bobgui_box_append (BOBGUI_BOX (content_area),
			  create_forward_back ("Right-to-Left", BOBGUI_TEXT_DIR_RTL));

      bobgui_dialog_add_button (BOBGUI_DIALOG (window), "Close", BOBGUI_RESPONSE_CLOSE);
      g_signal_connect (window, "response", G_CALLBACK (bobgui_window_destroy), NULL);
    }

  if (!bobgui_widget_get_visible (window))
    bobgui_window_present (BOBGUI_WINDOW (window));
  else
    bobgui_window_destroy (BOBGUI_WINDOW (window));
}

/*
 * BobguiFontSelection
 */

static void
create_font_selection (BobguiWidget *widget)
{
  static BobguiWidget *window = NULL;

  if (!window)
    {
      BobguiWidget *picker;
      BobguiWidget *hbox;
      BobguiWidget *label;
      
      window = bobgui_window_new ();
      bobgui_window_set_display (BOBGUI_WINDOW (window),
                              bobgui_widget_get_display (widget));
      g_object_add_weak_pointer (G_OBJECT (window), (gpointer *)&window);

      bobgui_window_set_title (BOBGUI_WINDOW (window), "BobguiFontButton");

      hbox = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 8);
      bobgui_widget_set_margin_top (hbox, 8);
      bobgui_widget_set_margin_bottom (hbox, 8);
      bobgui_widget_set_margin_start (hbox, 8);
      bobgui_widget_set_margin_end (hbox, 8);
      bobgui_window_set_child (BOBGUI_WINDOW (window), hbox);
      
      label = bobgui_label_new ("Pick a font");
      bobgui_box_append (BOBGUI_BOX (hbox), label);

      picker = bobgui_font_button_new ();
      bobgui_font_button_set_use_font (BOBGUI_FONT_BUTTON (picker), TRUE);
      bobgui_box_append (BOBGUI_BOX (hbox), picker);
    }
  
  if (!bobgui_widget_get_visible (window))
    bobgui_window_present (BOBGUI_WINDOW (window));
  else
    bobgui_window_destroy (BOBGUI_WINDOW (window));
}

/*
 * BobguiDialog
 */

static BobguiWidget *dialog_window = NULL;

static void
dialog_response_cb (BobguiWidget *widget, int response, gpointer unused)
{
  BobguiWidget *content_area;
  BobguiWidget *child;

  if (response == BOBGUI_RESPONSE_APPLY)
    {
      content_area = bobgui_dialog_get_content_area (BOBGUI_DIALOG (dialog_window));

      for (child = bobgui_widget_get_first_child (content_area);
           child != NULL;
           child = bobgui_widget_get_next_sibling (child))
        {
          if (BOBGUI_IS_LABEL (child))
            {
              bobgui_box_remove (BOBGUI_BOX (content_area), child);
              break;
            }
        }

      /* no label removed, so add one */
      if (child == NULL)
        {
          BobguiWidget *label;

          label = bobgui_label_new ("Dialog Test");
          bobgui_widget_set_margin_start (label, 10);
          bobgui_widget_set_margin_end (label, 10);
          bobgui_widget_set_margin_top (label, 10);
          bobgui_widget_set_margin_bottom (label, 10);
          bobgui_box_append (BOBGUI_BOX (content_area), label);
        }
    }
}

static void
create_dialog (BobguiWidget *widget)
{
  if (!dialog_window)
    {
      /* This is a terrible example; it's much simpler to create
       * dialogs than this. Don't use testbobgui for example code,
       * use bobgui-demo ;-)
       */
      
      dialog_window = bobgui_dialog_new ();
      bobgui_window_set_display (BOBGUI_WINDOW (dialog_window),
                              bobgui_widget_get_display (widget));
      g_object_add_weak_pointer (G_OBJECT (dialog_window), (gpointer *)&dialog_window);

      bobgui_window_set_title (BOBGUI_WINDOW (dialog_window), "BobguiDialog");

      bobgui_dialog_add_button (BOBGUI_DIALOG (dialog_window),
                             "OK",
                             BOBGUI_RESPONSE_OK);

      bobgui_dialog_add_button (BOBGUI_DIALOG (dialog_window),
                             "Toggle",
                             BOBGUI_RESPONSE_APPLY);
      
      g_signal_connect (dialog_window, "response",
			G_CALLBACK (dialog_response_cb),
			NULL);
    }

  if (!bobgui_widget_get_visible (dialog_window))
    bobgui_widget_show (dialog_window);
  else
    bobgui_window_destroy (BOBGUI_WINDOW (dialog_window));
}

/* Display & Screen test 
 */

typedef struct
{
  BobguiWidget *combo;
  BobguiWidget *entry;
  BobguiWidget *toplevel;
  BobguiWidget *dialog_window;
} ScreenDisplaySelection;

static void
screen_display_check (BobguiWidget *widget, ScreenDisplaySelection *data)
{
  const char *display_name;
  GdkDisplay *display;
  BobguiWidget *dialog;
  GdkDisplay *new_display = NULL;
  GdkDisplay *current_display = bobgui_widget_get_display (widget);
  
  display_name = bobgui_editable_get_text (BOBGUI_EDITABLE (data->entry));
  display = gdk_display_open (display_name);
      
  if (!display)
    {
      dialog = bobgui_message_dialog_new (BOBGUI_WINDOW (bobgui_widget_get_root (widget)),
                                       BOBGUI_DIALOG_DESTROY_WITH_PARENT,
                                       BOBGUI_MESSAGE_ERROR,
                                       BOBGUI_BUTTONS_OK,
                                       "The display :\n%s\ncannot be opened",
                                       display_name);
      bobgui_window_set_display (BOBGUI_WINDOW (dialog), current_display);
      bobgui_widget_show (dialog);
      g_signal_connect (dialog, "response",
                        G_CALLBACK (bobgui_window_destroy),
                        NULL);
    }
  else
    {
      BobguiTreeModel *model = bobgui_combo_box_get_model (BOBGUI_COMBO_BOX (data->combo));
      int i = 0;
      BobguiTreeIter iter;
      gboolean found = FALSE;
      while (bobgui_tree_model_iter_nth_child (model, &iter, NULL, i++))
        {
          char *name;
          bobgui_tree_model_get (model, &iter, 0, &name, -1);
          found = !g_ascii_strcasecmp (display_name, name);
          g_free (name);

          if (found)
            break;
        }
      if (!found)
        bobgui_combo_box_text_append_text (BOBGUI_COMBO_BOX_TEXT (data->combo), display_name);
      new_display = display;

      bobgui_window_set_display (BOBGUI_WINDOW (data->toplevel), new_display);
      bobgui_window_destroy (BOBGUI_WINDOW (data->dialog_window));
    }
}

static void
screen_display_destroy_diag (BobguiWidget *widget, BobguiWidget *data)
{
  bobgui_window_destroy (BOBGUI_WINDOW (data));
}

static void
create_display_screen (BobguiWidget *widget)
{
  BobguiWidget *grid, *frame, *window, *combo_dpy, *vbox;
  BobguiWidget *label_dpy, *applyb, *cancelb;
  BobguiWidget *bbox;
  ScreenDisplaySelection *scr_dpy_data;
  GdkDisplay *display = bobgui_widget_get_display (widget);

  window = g_object_new (bobgui_window_get_type (),
			 "display", display,
			 "title", "Screen or Display selection",
                         NULL);
  g_signal_connect (window, "destroy", 
		    G_CALLBACK (bobgui_window_destroy), NULL);

  vbox = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 3);
  bobgui_window_set_child (BOBGUI_WINDOW (window), vbox);
  
  frame = bobgui_frame_new ("Select display");
  bobgui_box_append (BOBGUI_BOX (vbox), frame);
  
  grid = bobgui_grid_new ();
  bobgui_grid_set_row_spacing (BOBGUI_GRID (grid), 3);
  bobgui_grid_set_column_spacing (BOBGUI_GRID (grid), 3);

  bobgui_frame_set_child (BOBGUI_FRAME (frame), grid);

  label_dpy = bobgui_label_new ("move to another X display");
  combo_dpy = bobgui_combo_box_text_new_with_entry ();
  bobgui_widget_set_hexpand (combo_dpy, TRUE);
  bobgui_combo_box_text_append_text (BOBGUI_COMBO_BOX_TEXT (combo_dpy), "diabolo:0.0");
  bobgui_editable_set_text (BOBGUI_EDITABLE (bobgui_combo_box_get_child (BOBGUI_COMBO_BOX (combo_dpy))),
                         "<hostname>:<X Server Num>.<Screen Num>");

  bobgui_grid_attach (BOBGUI_GRID (grid), label_dpy, 0, 0, 1, 1);
  bobgui_grid_attach (BOBGUI_GRID (grid), combo_dpy, 0, 1, 1, 1);

  bbox = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 0);
  bobgui_widget_set_halign (bbox, BOBGUI_ALIGN_END);
  applyb = bobgui_button_new_with_mnemonic ("_Apply");
  cancelb = bobgui_button_new_with_mnemonic ("_Cancel");

  bobgui_box_append (BOBGUI_BOX (vbox), bbox);

  bobgui_box_append (BOBGUI_BOX (bbox), applyb);
  bobgui_box_append (BOBGUI_BOX (bbox), cancelb);

  scr_dpy_data = g_new0 (ScreenDisplaySelection, 1);

  scr_dpy_data->entry = bobgui_combo_box_get_child (BOBGUI_COMBO_BOX (combo_dpy));
  scr_dpy_data->toplevel = BOBGUI_WIDGET (bobgui_widget_get_root (widget));
  scr_dpy_data->dialog_window = window;

  g_signal_connect (cancelb, "clicked", 
		    G_CALLBACK (screen_display_destroy_diag), window);
  g_signal_connect (applyb, "clicked", 
		    G_CALLBACK (screen_display_check), scr_dpy_data);
  bobgui_window_present (BOBGUI_WINDOW (window));
}

/*
 * BobguiRange
 */

static char *
reformat_value (BobguiScale *scale,
                double    value)
{
  return g_strdup_printf ("-->%0.*g<--",
                          bobgui_scale_get_digits (scale), value);
}

static void
create_range_controls (BobguiWidget *widget)
{
  static BobguiWidget *window = NULL;
  BobguiWidget *box1;
  BobguiWidget *box2;
  BobguiWidget *button;
  BobguiWidget *scrollbar;
  BobguiWidget *scale;
  BobguiWidget *separator;
  BobguiAdjustment *adjustment;
  BobguiWidget *hbox;

  if (!window)
    {
      window = bobgui_window_new ();

      bobgui_window_set_display (BOBGUI_WINDOW (window),
                              bobgui_widget_get_display (widget));
      g_object_add_weak_pointer (G_OBJECT (window), (gpointer *)&window);

      bobgui_window_set_title (BOBGUI_WINDOW (window), "range controls");


      box1 = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 0);
      bobgui_window_set_child (BOBGUI_WINDOW (window), box1);


      box2 = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 10);
      bobgui_box_append (BOBGUI_BOX (box1), box2);


      adjustment = bobgui_adjustment_new (0.0, 0.0, 101.0, 0.1, 1.0, 1.0);

      scale = bobgui_scale_new (BOBGUI_ORIENTATION_HORIZONTAL, BOBGUI_ADJUSTMENT (adjustment));
      bobgui_widget_set_size_request (BOBGUI_WIDGET (scale), 150, -1);
      bobgui_scale_set_digits (BOBGUI_SCALE (scale), 1);
      bobgui_scale_set_draw_value (BOBGUI_SCALE (scale), TRUE);
      bobgui_box_append (BOBGUI_BOX (box2), scale);

      scrollbar = bobgui_scrollbar_new (BOBGUI_ORIENTATION_HORIZONTAL, BOBGUI_ADJUSTMENT (adjustment));
      bobgui_box_append (BOBGUI_BOX (box2), scrollbar);

      scale = bobgui_scale_new (BOBGUI_ORIENTATION_HORIZONTAL, BOBGUI_ADJUSTMENT (adjustment));
      bobgui_scale_set_draw_value (BOBGUI_SCALE (scale), TRUE);
      bobgui_scale_set_format_value_func (BOBGUI_SCALE (scale),
                                       (BobguiScaleFormatValueFunc) reformat_value,
                                       NULL, NULL);
      bobgui_box_append (BOBGUI_BOX (box2), scale);

      hbox = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 0);

      scale = bobgui_scale_new (BOBGUI_ORIENTATION_VERTICAL, BOBGUI_ADJUSTMENT (adjustment));
      bobgui_widget_set_size_request (scale, -1, 200);
      bobgui_scale_set_digits (BOBGUI_SCALE (scale), 2);
      bobgui_scale_set_draw_value (BOBGUI_SCALE (scale), TRUE);
      bobgui_box_append (BOBGUI_BOX (hbox), scale);

      scale = bobgui_scale_new (BOBGUI_ORIENTATION_VERTICAL, BOBGUI_ADJUSTMENT (adjustment));
      bobgui_widget_set_size_request (scale, -1, 200);
      bobgui_scale_set_digits (BOBGUI_SCALE (scale), 2);
      bobgui_scale_set_draw_value (BOBGUI_SCALE (scale), TRUE);
      bobgui_range_set_inverted (BOBGUI_RANGE (scale), TRUE);
      bobgui_box_append (BOBGUI_BOX (hbox), scale);

      scale = bobgui_scale_new (BOBGUI_ORIENTATION_VERTICAL, BOBGUI_ADJUSTMENT (adjustment));
      bobgui_scale_set_draw_value (BOBGUI_SCALE (scale), TRUE);
      bobgui_scale_set_format_value_func (BOBGUI_SCALE (scale),
                                       (BobguiScaleFormatValueFunc) reformat_value,
                                       NULL, NULL);
      bobgui_box_append (BOBGUI_BOX (hbox), scale);


      bobgui_box_append (BOBGUI_BOX (box2), hbox);

      separator = bobgui_separator_new (BOBGUI_ORIENTATION_HORIZONTAL);
      bobgui_box_append (BOBGUI_BOX (box1), separator);


      box2 = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 10);
      bobgui_box_append (BOBGUI_BOX (box1), box2);


      button = bobgui_button_new_with_label ("close");
      g_signal_connect_swapped (button, "clicked",
			        G_CALLBACK (bobgui_window_destroy),
				window);
      bobgui_box_append (BOBGUI_BOX (box2), button);
      bobgui_window_set_default_widget (BOBGUI_WINDOW (window), button);
    }

  if (!bobgui_widget_get_visible (window))
    bobgui_window_present (BOBGUI_WINDOW (window));
  else
    bobgui_window_destroy (BOBGUI_WINDOW (window));
}

/*
 * BobguiNotebook
 */

static const char * book_open_xpm[] = {
"16 16 4 1",
"       c None s None",
".      c black",
"X      c #808080",
"o      c white",
"                ",
"  ..            ",
" .Xo.    ...    ",
" .Xoo. ..oo.    ",
" .Xooo.Xooo...  ",
" .Xooo.oooo.X.  ",
" .Xooo.Xooo.X.  ",
" .Xooo.oooo.X.  ",
" .Xooo.Xooo.X.  ",
" .Xooo.oooo.X.  ",
"  .Xoo.Xoo..X.  ",
"   .Xo.o..ooX.  ",
"    .X..XXXXX.  ",
"    ..X.......  ",
"     ..         ",
"                "};

static const char * book_closed_xpm[] = {
"16 16 6 1",
"       c None s None",
".      c black",
"X      c red",
"o      c yellow",
"O      c #808080",
"#      c white",
"                ",
"       ..       ",
"     ..XX.      ",
"   ..XXXXX.     ",
" ..XXXXXXXX.    ",
".ooXXXXXXXXX.   ",
"..ooXXXXXXXXX.  ",
".X.ooXXXXXXXXX. ",
".XX.ooXXXXXX..  ",
" .XX.ooXXX..#O  ",
"  .XX.oo..##OO. ",
"   .XX..##OO..  ",
"    .X.#OO..    ",
"     ..O..      ",
"      ..        ",
"                "};

GdkPixbuf *book_open;
GdkPixbuf *book_closed;
BobguiWidget *sample_notebook;

static void
set_page_image (BobguiNotebook *notebook, int page_num, GdkPixbuf *pixbuf)
{
  BobguiWidget *page_widget;
  BobguiWidget *pixwid;

  page_widget = bobgui_notebook_get_nth_page (notebook, page_num);

  pixwid = g_object_get_data (G_OBJECT (page_widget), "tab_pixmap");
  bobgui_image_set_from_pixbuf (BOBGUI_IMAGE (pixwid), pixbuf);
  
  pixwid = g_object_get_data (G_OBJECT (page_widget), "menu_pixmap");
  bobgui_image_set_from_pixbuf (BOBGUI_IMAGE (pixwid), pixbuf);
}

static void
page_switch (BobguiWidget *widget, gpointer *page, int page_num)
{
  BobguiNotebook *notebook = BOBGUI_NOTEBOOK (widget);
  int old_page_num = bobgui_notebook_get_current_page (notebook);
 
  if (page_num == old_page_num)
    return;

  set_page_image (notebook, page_num, book_open);

  if (old_page_num != -1)
    set_page_image (notebook, old_page_num, book_closed);
}

static void
tab_fill (BobguiToggleButton *button, BobguiWidget *child)
{
  BobguiNotebookPage *page = bobgui_notebook_get_page (BOBGUI_NOTEBOOK (sample_notebook), child);
  g_object_set (page, "tab-fill", bobgui_toggle_button_get_active (button), NULL);
}

static void
tab_expand (BobguiToggleButton *button, BobguiWidget *child)
{
  BobguiNotebookPage *page = bobgui_notebook_get_page (BOBGUI_NOTEBOOK (sample_notebook), child);
  g_object_set (page, "tab-expand", bobgui_toggle_button_get_active (button), NULL);
}

static void
create_pages (BobguiNotebook *notebook, int start, int end)
{
  BobguiWidget *child = NULL;
  BobguiWidget *button;
  BobguiWidget *label;
  BobguiWidget *hbox;
  BobguiWidget *vbox;
  BobguiWidget *label_box;
  BobguiWidget *menu_box;
  BobguiWidget *pixwid;
  int i;
  char buffer[32];
  char accel_buffer[32];

  for (i = start; i <= end; i++)
    {
      sprintf (buffer, "Page %d", i);
      sprintf (accel_buffer, "Page _%d", i);

      child = bobgui_frame_new (buffer);

      vbox = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 0);
      bobgui_box_set_homogeneous (BOBGUI_BOX (vbox), TRUE);
      bobgui_frame_set_child (BOBGUI_FRAME (child), vbox);

      hbox = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 0);
      bobgui_box_set_homogeneous (BOBGUI_BOX (hbox), TRUE);
      bobgui_box_append (BOBGUI_BOX (vbox), hbox);

      button = bobgui_check_button_new_with_label ("Fill Tab");
      bobgui_box_append (BOBGUI_BOX (hbox), button);
      bobgui_toggle_button_set_active (BOBGUI_TOGGLE_BUTTON (button), TRUE);
      g_signal_connect (button, "toggled",
			G_CALLBACK (tab_fill), child);

      button = bobgui_check_button_new_with_label ("Expand Tab");
      bobgui_box_append (BOBGUI_BOX (hbox), button);
      g_signal_connect (button, "toggled",
			G_CALLBACK (tab_expand), child);

      button = bobgui_button_new_with_label ("Hide Page");
      g_signal_connect_swapped (button, "clicked",
				G_CALLBACK (bobgui_widget_hide),
				child);

      label_box = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 0);
      pixwid = bobgui_image_new_from_pixbuf (book_closed);
      g_object_set_data (G_OBJECT (child), "tab_pixmap", pixwid);

      bobgui_box_append (BOBGUI_BOX (label_box), pixwid);
      bobgui_widget_set_margin_start (pixwid, 3);
      bobgui_widget_set_margin_end (pixwid, 3);
      bobgui_widget_set_margin_bottom (pixwid, 1);
      bobgui_widget_set_margin_top (pixwid, 1);
      label = bobgui_label_new_with_mnemonic (accel_buffer);
      bobgui_box_append (BOBGUI_BOX (label_box), label);


      menu_box = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 0);
      pixwid = bobgui_image_new_from_pixbuf (book_closed);
      g_object_set_data (G_OBJECT (child), "menu_pixmap", pixwid);

      bobgui_box_append (BOBGUI_BOX (menu_box), pixwid);
      bobgui_widget_set_margin_start (pixwid, 3);
      bobgui_widget_set_margin_end (pixwid, 3);
      bobgui_widget_set_margin_bottom (pixwid, 1);
      bobgui_widget_set_margin_top (pixwid, 1);
      label = bobgui_label_new (buffer);
      bobgui_box_append (BOBGUI_BOX (menu_box), label);

      bobgui_notebook_append_page_menu (notebook, child, label_box, menu_box);
    }
}

static void
rotate_notebook (BobguiButton   *button,
		 BobguiNotebook *notebook)
{
  bobgui_notebook_set_tab_pos (notebook, (bobgui_notebook_get_tab_pos (notebook) + 1) % 4);
}

static void
show_all_pages (BobguiButton   *button,
		BobguiNotebook *notebook)
{
  int i;

  for (i = 0; i < bobgui_notebook_get_n_pages (notebook); i++)
    bobgui_widget_show (bobgui_notebook_get_nth_page (notebook, i)); 
}

static void
notebook_type_changed (BobguiWidget *optionmenu,
		       gpointer   data)
{
  BobguiNotebook *notebook;
  int i, c;

  enum {
    STANDARD,
    NOTABS,
    BORDERLESS,
    SCROLLABLE
  };

  notebook = BOBGUI_NOTEBOOK (data);

  c = bobgui_combo_box_get_active (BOBGUI_COMBO_BOX (optionmenu));

  switch (c)
    {
    case STANDARD:
      /* standard notebook */
      bobgui_notebook_set_show_tabs (notebook, TRUE);
      bobgui_notebook_set_show_border (notebook, TRUE);
      bobgui_notebook_set_scrollable (notebook, FALSE);
      break;

    case NOTABS:
      /* notabs notebook */
      bobgui_notebook_set_show_tabs (notebook, FALSE);
      bobgui_notebook_set_show_border (notebook, TRUE);
      break;

    case BORDERLESS:
      /* borderless */
      bobgui_notebook_set_show_tabs (notebook, FALSE);
      bobgui_notebook_set_show_border (notebook, FALSE);
      break;

    case SCROLLABLE:  
      /* scrollable */
      bobgui_notebook_set_show_tabs (notebook, TRUE);
      bobgui_notebook_set_show_border (notebook, TRUE);
      bobgui_notebook_set_scrollable (notebook, TRUE);
      if (bobgui_notebook_get_n_pages (notebook) == 5)
	create_pages (notebook, 6, 15);

      return;
      break;
    default:
      g_assert_not_reached ();
    }

  if (bobgui_notebook_get_n_pages (notebook) == 15)
    for (i = 0; i < 10; i++)
      bobgui_notebook_remove_page (notebook, 5);
}

static void
notebook_popup (BobguiToggleButton *button,
		BobguiNotebook     *notebook)
{
  if (bobgui_toggle_button_get_active (button))
    bobgui_notebook_popup_enable (notebook);
  else
    bobgui_notebook_popup_disable (notebook);
}

static void
create_notebook (BobguiWidget *widget)
{
  static BobguiWidget *window = NULL;
  BobguiWidget *box1;
  BobguiWidget *box2;
  BobguiWidget *button;
  BobguiWidget *separator;
  BobguiWidget *omenu;
  BobguiWidget *label;

  static const char *items[] =
  {
    "Standard",
    "No tabs",
    "Borderless",
    "Scrollable"
  };
  
  if (!window)
    {
      window = bobgui_window_new ();
      bobgui_window_set_display (BOBGUI_WINDOW (window),
                              bobgui_widget_get_display (widget));
      g_object_add_weak_pointer (G_OBJECT (window), (gpointer *)&window);

      bobgui_window_set_title (BOBGUI_WINDOW (window), "notebook");

      box1 = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 0);
      bobgui_window_set_child (BOBGUI_WINDOW (window), box1);

      sample_notebook = bobgui_notebook_new ();
      g_signal_connect (sample_notebook, "switch_page",
                        G_CALLBACK (page_switch), NULL);
      bobgui_notebook_set_tab_pos (BOBGUI_NOTEBOOK (sample_notebook), BOBGUI_POS_TOP);
      bobgui_widget_set_vexpand (sample_notebook, TRUE);
      bobgui_box_append (BOBGUI_BOX (box1), sample_notebook);

      bobgui_widget_realize (sample_notebook);

      if (!book_open)
	book_open = gdk_pixbuf_new_from_xpm_data (book_open_xpm);

      if (!book_closed)
	book_closed = gdk_pixbuf_new_from_xpm_data (book_closed_xpm);

      create_pages (BOBGUI_NOTEBOOK (sample_notebook), 1, 5);

      separator = bobgui_separator_new (BOBGUI_ORIENTATION_HORIZONTAL);
      bobgui_box_append (BOBGUI_BOX (box1), separator);

      box2 = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 5);
      bobgui_box_append (BOBGUI_BOX (box1), box2);

      button = bobgui_check_button_new_with_label ("popup menu");
      bobgui_box_append (BOBGUI_BOX (box2), button);
      g_signal_connect (button, "clicked",
			G_CALLBACK (notebook_popup),
			sample_notebook);

      box2 = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 5);
      bobgui_box_append (BOBGUI_BOX (box1), box2);

      label = bobgui_label_new ("Notebook Style :");
      bobgui_box_append (BOBGUI_BOX (box2), label);

      omenu = build_option_menu (items, G_N_ELEMENTS (items), 0,
				 notebook_type_changed,
				 sample_notebook);
      bobgui_box_append (BOBGUI_BOX (box2), omenu);

      button = bobgui_button_new_with_label ("Show all Pages");
      bobgui_box_append (BOBGUI_BOX (box2), button);
      g_signal_connect (button, "clicked",
			G_CALLBACK (show_all_pages), sample_notebook);

      box2 = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 10);
      bobgui_box_set_homogeneous (BOBGUI_BOX (box2), TRUE);
      bobgui_box_append (BOBGUI_BOX (box1), box2);

      button = bobgui_button_new_with_label ("prev");
      g_signal_connect_swapped (button, "clicked",
			        G_CALLBACK (bobgui_notebook_prev_page),
				sample_notebook);
      bobgui_box_append (BOBGUI_BOX (box2), button);

      button = bobgui_button_new_with_label ("next");
      g_signal_connect_swapped (button, "clicked",
			        G_CALLBACK (bobgui_notebook_next_page),
				sample_notebook);
      bobgui_box_append (BOBGUI_BOX (box2), button);

      button = bobgui_button_new_with_label ("rotate");
      g_signal_connect (button, "clicked",
			G_CALLBACK (rotate_notebook), sample_notebook);
      bobgui_box_append (BOBGUI_BOX (box2), button);

      separator = bobgui_separator_new (BOBGUI_ORIENTATION_HORIZONTAL);
      bobgui_box_append (BOBGUI_BOX (box1), separator);

      button = bobgui_button_new_with_label ("close");
      g_signal_connect_swapped (button, "clicked",
			        G_CALLBACK (bobgui_window_destroy),
				window);
      bobgui_box_append (BOBGUI_BOX (box1), button);
      bobgui_window_set_default_widget (BOBGUI_WINDOW (window), button);
    }

  if (!bobgui_widget_get_visible (window))
    bobgui_window_present (BOBGUI_WINDOW (window));
  else
    bobgui_window_destroy (BOBGUI_WINDOW (window));
}

/*
 * BobguiPanes
 */

static void
toggle_resize (BobguiWidget *widget, BobguiWidget *child)
{
  BobguiPaned *paned = BOBGUI_PANED (bobgui_widget_get_parent (child));

  if (child == bobgui_paned_get_start_child (paned))
    bobgui_paned_set_resize_start_child (paned, !bobgui_paned_get_resize_start_child (paned));
  else
    bobgui_paned_set_resize_end_child (paned, !bobgui_paned_get_resize_end_child (paned));
}

static void
toggle_shrink (BobguiWidget *widget, BobguiWidget *child)
{
  BobguiPaned *paned = BOBGUI_PANED (bobgui_widget_get_parent (child));

  if (child == bobgui_paned_get_start_child (paned))
    bobgui_paned_set_shrink_start_child (paned, !bobgui_paned_get_shrink_start_child (paned));
  else
    bobgui_paned_set_shrink_end_child (paned, !bobgui_paned_get_shrink_end_child (paned));
}

static BobguiWidget *
create_pane_options (BobguiPaned    *paned,
		     const char *frame_label,
		     const char *label1,
		     const char *label2)
{
  BobguiWidget *child1, *child2;
  BobguiWidget *frame;
  BobguiWidget *grid;
  BobguiWidget *label;
  BobguiWidget *check_button;

  child1 = bobgui_paned_get_start_child (paned);
  child2 = bobgui_paned_get_end_child (paned);

  frame = bobgui_frame_new (frame_label);
  
  grid = bobgui_grid_new ();
  bobgui_frame_set_child (BOBGUI_FRAME (frame), grid);
  
  label = bobgui_label_new (label1);
  bobgui_grid_attach (BOBGUI_GRID (grid), label, 0, 0, 1, 1);
  
  check_button = bobgui_check_button_new_with_label ("Resize");
  bobgui_grid_attach (BOBGUI_GRID (grid), check_button, 0, 1, 1, 1);
  g_signal_connect (check_button, "toggled",
		    G_CALLBACK (toggle_resize),
                    child1);

  check_button = bobgui_check_button_new_with_label ("Shrink");
  bobgui_grid_attach (BOBGUI_GRID (grid), check_button, 0, 2, 1, 1);
  bobgui_toggle_button_set_active (BOBGUI_TOGGLE_BUTTON (check_button),
			       TRUE);
  g_signal_connect (check_button, "toggled",
		    G_CALLBACK (toggle_shrink),
                    child1);

  label = bobgui_label_new (label2);
  bobgui_grid_attach (BOBGUI_GRID (grid), label, 1, 0, 1, 1);
  
  check_button = bobgui_check_button_new_with_label ("Resize");
  bobgui_grid_attach (BOBGUI_GRID (grid), check_button, 1, 1, 1, 1);
  bobgui_toggle_button_set_active (BOBGUI_TOGGLE_BUTTON (check_button),
			       TRUE);
  g_signal_connect (check_button, "toggled",
		    G_CALLBACK (toggle_resize),
                    child2);

  check_button = bobgui_check_button_new_with_label ("Shrink");
  bobgui_grid_attach (BOBGUI_GRID (grid), check_button, 1, 2, 1, 1);
  bobgui_toggle_button_set_active (BOBGUI_TOGGLE_BUTTON (check_button),
			       TRUE);
  g_signal_connect (check_button, "toggled",
		    G_CALLBACK (toggle_shrink),
                    child2);

  return frame;
}

static void
create_panes (BobguiWidget *widget)
{
  static BobguiWidget *window = NULL;
  BobguiWidget *frame;
  BobguiWidget *hpaned;
  BobguiWidget *vpaned;
  BobguiWidget *button;
  BobguiWidget *vbox;

  if (!window)
    {
      window = bobgui_window_new ();

      bobgui_window_set_display (BOBGUI_WINDOW (window),
                              bobgui_widget_get_display (widget));
      g_object_add_weak_pointer (G_OBJECT (window), (gpointer *)&window);

      bobgui_window_set_title (BOBGUI_WINDOW (window), "Panes");

      vbox = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 0);
      bobgui_window_set_child (BOBGUI_WINDOW (window), vbox);

      vpaned = bobgui_paned_new (BOBGUI_ORIENTATION_VERTICAL);
      bobgui_box_append (BOBGUI_BOX (vbox), vpaned);

      hpaned = bobgui_paned_new (BOBGUI_ORIENTATION_HORIZONTAL);
      bobgui_paned_set_start_child (BOBGUI_PANED (vpaned), hpaned);

      frame = bobgui_frame_new (NULL);
      bobgui_widget_set_size_request (frame, 60, 60);
      bobgui_paned_set_start_child (BOBGUI_PANED (hpaned), frame);
      
      button = bobgui_button_new_with_label ("Hi there");
      bobgui_frame_set_child (BOBGUI_FRAME (frame), button);

      frame = bobgui_frame_new (NULL);
      bobgui_widget_set_size_request (frame, 80, 60);
      bobgui_paned_set_end_child (BOBGUI_PANED (hpaned), frame);

      frame = bobgui_frame_new (NULL);
      bobgui_widget_set_size_request (frame, 60, 80);
      bobgui_paned_set_end_child (BOBGUI_PANED (vpaned), frame);

      /* Now create toggle buttons to control sizing */

      bobgui_box_append (BOBGUI_BOX (vbox),
			  create_pane_options (BOBGUI_PANED (hpaned),
					       "Horizontal",
					       "Left",
					       "Right"));

      bobgui_box_append (BOBGUI_BOX (vbox),
			  create_pane_options (BOBGUI_PANED (vpaned),
					       "Vertical",
					       "Top",
					       "Bottom"));
    }

  if (!bobgui_widget_get_visible (window))
    bobgui_window_present (BOBGUI_WINDOW (window));
  else
    bobgui_window_destroy (BOBGUI_WINDOW (window));
}

/*
 * Paned keyboard navigation
 */

static BobguiWidget*
paned_keyboard_window1 (BobguiWidget *widget)
{
  BobguiWidget *window1;
  BobguiWidget *hpaned1;
  BobguiWidget *frame1;
  BobguiWidget *vbox1;
  BobguiWidget *button7;
  BobguiWidget *button8;
  BobguiWidget *button9;
  BobguiWidget *vpaned1;
  BobguiWidget *frame2;
  BobguiWidget *frame5;
  BobguiWidget *hbox1;
  BobguiWidget *button5;
  BobguiWidget *button6;
  BobguiWidget *frame3;
  BobguiWidget *frame4;
  BobguiWidget *grid1;
  BobguiWidget *button1;
  BobguiWidget *button2;
  BobguiWidget *button3;
  BobguiWidget *button4;

  window1 = bobgui_window_new ();
  bobgui_window_set_title (BOBGUI_WINDOW (window1), "Basic paned navigation");
  bobgui_window_set_display (BOBGUI_WINDOW (window1), 
			  bobgui_widget_get_display (widget));

  hpaned1 = bobgui_paned_new (BOBGUI_ORIENTATION_HORIZONTAL);
  bobgui_window_set_child (BOBGUI_WINDOW (window1), hpaned1);

  frame1 = bobgui_frame_new (NULL);
  bobgui_paned_set_start_child (BOBGUI_PANED (hpaned1), frame1);
  bobgui_paned_set_resize_start_child (BOBGUI_PANED (hpaned1), FALSE);
  bobgui_paned_set_shrink_start_child (BOBGUI_PANED (hpaned1), TRUE);

  vbox1 = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 0);
  bobgui_frame_set_child (BOBGUI_FRAME (frame1), vbox1);

  button7 = bobgui_button_new_with_label ("button7");
  bobgui_box_append (BOBGUI_BOX (vbox1), button7);

  button8 = bobgui_button_new_with_label ("button8");
  bobgui_box_append (BOBGUI_BOX (vbox1), button8);

  button9 = bobgui_button_new_with_label ("button9");
  bobgui_box_append (BOBGUI_BOX (vbox1), button9);

  vpaned1 = bobgui_paned_new (BOBGUI_ORIENTATION_VERTICAL);
  bobgui_paned_set_end_child (BOBGUI_PANED (hpaned1), vpaned1);
  bobgui_paned_set_resize_end_child (BOBGUI_PANED (hpaned1), TRUE);
  bobgui_paned_set_shrink_end_child (BOBGUI_PANED (hpaned1), TRUE);

  frame2 = bobgui_frame_new (NULL);
  bobgui_paned_set_start_child (BOBGUI_PANED (vpaned1), frame2);
  bobgui_paned_set_resize_start_child (BOBGUI_PANED (vpaned1), FALSE);
  bobgui_paned_set_shrink_start_child (BOBGUI_PANED (vpaned1), TRUE);

  frame5 = bobgui_frame_new (NULL);
  bobgui_frame_set_child (BOBGUI_FRAME (frame2), frame5);

  hbox1 = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 0);
  bobgui_frame_set_child (BOBGUI_FRAME (frame5), hbox1);

  button5 = bobgui_button_new_with_label ("button5");
  bobgui_box_append (BOBGUI_BOX (hbox1), button5);

  button6 = bobgui_button_new_with_label ("button6");
  bobgui_box_append (BOBGUI_BOX (hbox1), button6);

  frame3 = bobgui_frame_new (NULL);
  bobgui_paned_set_end_child (BOBGUI_PANED (vpaned1), frame3);
  bobgui_paned_set_resize_end_child (BOBGUI_PANED (vpaned1), TRUE);
  bobgui_paned_set_shrink_end_child (BOBGUI_PANED (vpaned1), TRUE);

  frame4 = bobgui_frame_new ("Buttons");
  bobgui_frame_set_child (BOBGUI_FRAME (frame3), frame4);

  grid1 = bobgui_grid_new ();
  bobgui_frame_set_child (BOBGUI_FRAME (frame4), grid1);

  button1 = bobgui_button_new_with_label ("button1");
  bobgui_grid_attach (BOBGUI_GRID (grid1), button1, 0, 0, 1, 1);

  button2 = bobgui_button_new_with_label ("button2");
  bobgui_grid_attach (BOBGUI_GRID (grid1), button2, 1, 0, 1, 1);

  button3 = bobgui_button_new_with_label ("button3");
  bobgui_grid_attach (BOBGUI_GRID (grid1), button3, 0, 1, 1, 1);

  button4 = bobgui_button_new_with_label ("button4");
  bobgui_grid_attach (BOBGUI_GRID (grid1), button4, 1, 1, 1, 1);

  return window1;
}

static BobguiWidget*
paned_keyboard_window2 (BobguiWidget *widget)
{
  BobguiWidget *window2;
  BobguiWidget *hpaned2;
  BobguiWidget *frame6;
  BobguiWidget *button13;
  BobguiWidget *hbox2;
  BobguiWidget *vpaned2;
  BobguiWidget *frame7;
  BobguiWidget *button12;
  BobguiWidget *frame8;
  BobguiWidget *button11;
  BobguiWidget *button10;

  window2 = bobgui_window_new ();
  bobgui_window_set_title (BOBGUI_WINDOW (window2), "\"button 10\" is not inside the horizontal pane");

  bobgui_window_set_display (BOBGUI_WINDOW (window2), 
			  bobgui_widget_get_display (widget));

  hpaned2 = bobgui_paned_new (BOBGUI_ORIENTATION_HORIZONTAL);
  bobgui_window_set_child (BOBGUI_WINDOW (window2), hpaned2);

  frame6 = bobgui_frame_new (NULL);
  bobgui_paned_set_start_child (BOBGUI_PANED (hpaned2), frame6);
  bobgui_paned_set_resize_start_child (BOBGUI_PANED (hpaned2), FALSE);
  bobgui_paned_set_shrink_start_child (BOBGUI_PANED (hpaned2), TRUE);

  button13 = bobgui_button_new_with_label ("button13");
  bobgui_frame_set_child (BOBGUI_FRAME (frame6), button13);

  hbox2 = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 0);
  bobgui_paned_set_end_child (BOBGUI_PANED (hpaned2), hbox2);
  bobgui_paned_set_resize_end_child (BOBGUI_PANED (hpaned2), TRUE);
  bobgui_paned_set_shrink_end_child (BOBGUI_PANED (hpaned2), TRUE);

  vpaned2 = bobgui_paned_new (BOBGUI_ORIENTATION_VERTICAL);
  bobgui_box_append (BOBGUI_BOX (hbox2), vpaned2);

  frame7 = bobgui_frame_new (NULL);
  bobgui_paned_set_start_child (BOBGUI_PANED (vpaned2), frame7);
  bobgui_paned_set_resize_start_child (BOBGUI_PANED (vpaned2), FALSE);
  bobgui_paned_set_shrink_start_child (BOBGUI_PANED (vpaned2), TRUE);

  button12 = bobgui_button_new_with_label ("button12");
  bobgui_frame_set_child (BOBGUI_FRAME (frame7), button12);

  frame8 = bobgui_frame_new (NULL);
  bobgui_paned_set_end_child (BOBGUI_PANED (vpaned2), frame8);
  bobgui_paned_set_resize_end_child (BOBGUI_PANED (vpaned2), TRUE);
  bobgui_paned_set_shrink_end_child (BOBGUI_PANED (vpaned2), TRUE);

  button11 = bobgui_button_new_with_label ("button11");
  bobgui_frame_set_child (BOBGUI_FRAME (frame8), button11);

  button10 = bobgui_button_new_with_label ("button10");
  bobgui_box_append (BOBGUI_BOX (hbox2), button10);

  return window2;
}

static BobguiWidget*
paned_keyboard_window3 (BobguiWidget *widget)
{
  BobguiWidget *window3;
  BobguiWidget *vbox2;
  BobguiWidget *label1;
  BobguiWidget *hpaned3;
  BobguiWidget *frame9;
  BobguiWidget *button14;
  BobguiWidget *hpaned4;
  BobguiWidget *frame10;
  BobguiWidget *button15;
  BobguiWidget *hpaned5;
  BobguiWidget *frame11;
  BobguiWidget *button16;
  BobguiWidget *frame12;
  BobguiWidget *button17;

  window3 = bobgui_window_new ();
  g_object_set_data (G_OBJECT (window3), "window3", window3);
  bobgui_window_set_title (BOBGUI_WINDOW (window3), "Nested panes");

  bobgui_window_set_display (BOBGUI_WINDOW (window3),
			  bobgui_widget_get_display (widget));


  vbox2 = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 0);
  bobgui_window_set_child (BOBGUI_WINDOW (window3), vbox2);

  label1 = bobgui_label_new ("Three panes nested inside each other");
  bobgui_box_append (BOBGUI_BOX (vbox2), label1);

  hpaned3 = bobgui_paned_new (BOBGUI_ORIENTATION_HORIZONTAL);
  bobgui_box_append (BOBGUI_BOX (vbox2), hpaned3);

  frame9 = bobgui_frame_new (NULL);
  bobgui_paned_set_start_child (BOBGUI_PANED (hpaned3), frame9);
  bobgui_paned_set_resize_start_child (BOBGUI_PANED (hpaned3), FALSE);
  bobgui_paned_set_shrink_start_child (BOBGUI_PANED (hpaned3), TRUE);

  button14 = bobgui_button_new_with_label ("button14");
  bobgui_frame_set_child (BOBGUI_FRAME (frame9), button14);

  hpaned4 = bobgui_paned_new (BOBGUI_ORIENTATION_HORIZONTAL);
  bobgui_paned_set_end_child (BOBGUI_PANED (hpaned3), hpaned4);
  bobgui_paned_set_resize_end_child (BOBGUI_PANED (hpaned3), TRUE);
  bobgui_paned_set_shrink_end_child (BOBGUI_PANED (hpaned3), TRUE);

  frame10 = bobgui_frame_new (NULL);
  bobgui_paned_set_start_child (BOBGUI_PANED (hpaned4), frame10);
  bobgui_paned_set_resize_start_child (BOBGUI_PANED (hpaned4), FALSE);
  bobgui_paned_set_shrink_start_child (BOBGUI_PANED (hpaned4), TRUE);

  button15 = bobgui_button_new_with_label ("button15");
  bobgui_frame_set_child (BOBGUI_FRAME (frame10), button15);

  hpaned5 = bobgui_paned_new (BOBGUI_ORIENTATION_HORIZONTAL);
  bobgui_paned_set_end_child (BOBGUI_PANED (hpaned4), hpaned5);
  bobgui_paned_set_resize_end_child (BOBGUI_PANED (hpaned4), TRUE);
  bobgui_paned_set_shrink_end_child (BOBGUI_PANED (hpaned4), TRUE);

  frame11 = bobgui_frame_new (NULL);
  bobgui_paned_set_start_child (BOBGUI_PANED (hpaned5), frame11);
  bobgui_paned_set_resize_start_child (BOBGUI_PANED (hpaned5), FALSE);
  bobgui_paned_set_shrink_start_child (BOBGUI_PANED (hpaned5), TRUE);

  button16 = bobgui_button_new_with_label ("button16");
  bobgui_frame_set_child (BOBGUI_FRAME (frame11), button16);

  frame12 = bobgui_frame_new (NULL);
  bobgui_paned_set_end_child (BOBGUI_PANED (hpaned5), frame12);
  bobgui_paned_set_resize_end_child (BOBGUI_PANED (hpaned5), TRUE);
  bobgui_paned_set_shrink_end_child (BOBGUI_PANED (hpaned5), TRUE);

  button17 = bobgui_button_new_with_label ("button17");
  bobgui_frame_set_child (BOBGUI_FRAME (frame12), button17);

  return window3;
}

static BobguiWidget*
paned_keyboard_window4 (BobguiWidget *widget)
{
  BobguiWidget *window4;
  BobguiWidget *vbox3;
  BobguiWidget *label2;
  BobguiWidget *hpaned6;
  BobguiWidget *vpaned3;
  BobguiWidget *button19;
  BobguiWidget *button18;
  BobguiWidget *hbox3;
  BobguiWidget *vpaned4;
  BobguiWidget *button21;
  BobguiWidget *button20;
  BobguiWidget *vpaned5;
  BobguiWidget *button23;
  BobguiWidget *button22;
  BobguiWidget *vpaned6;
  BobguiWidget *button25;
  BobguiWidget *button24;

  window4 = bobgui_window_new ();
  g_object_set_data (G_OBJECT (window4), "window4", window4);
  bobgui_window_set_title (BOBGUI_WINDOW (window4), "window4");

  bobgui_window_set_display (BOBGUI_WINDOW (window4),
                          bobgui_widget_get_display (widget));

  vbox3 = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 0);
  bobgui_window_set_child (BOBGUI_WINDOW (window4), vbox3);

  label2 = bobgui_label_new ("Widget tree:\n\nhpaned \n - vpaned\n - hbox\n    - vpaned\n    - vpaned\n    - vpaned\n");
  bobgui_box_append (BOBGUI_BOX (vbox3), label2);
  bobgui_label_set_justify (BOBGUI_LABEL (label2), BOBGUI_JUSTIFY_LEFT);

  hpaned6 = bobgui_paned_new (BOBGUI_ORIENTATION_HORIZONTAL);
  bobgui_box_append (BOBGUI_BOX (vbox3), hpaned6);

  vpaned3 = bobgui_paned_new (BOBGUI_ORIENTATION_VERTICAL);
  bobgui_paned_set_start_child (BOBGUI_PANED (hpaned6), vpaned3);
  bobgui_paned_set_resize_start_child (BOBGUI_PANED (hpaned6), FALSE);
  bobgui_paned_set_shrink_start_child (BOBGUI_PANED (hpaned6), TRUE);

  button19 = bobgui_button_new_with_label ("button19");
  bobgui_paned_set_start_child (BOBGUI_PANED (vpaned3), button19);
  bobgui_paned_set_resize_start_child (BOBGUI_PANED (vpaned3), FALSE);
  bobgui_paned_set_shrink_start_child (BOBGUI_PANED (vpaned3), TRUE);

  button18 = bobgui_button_new_with_label ("button18");
  bobgui_paned_set_end_child (BOBGUI_PANED (vpaned3), button18);
  bobgui_paned_set_resize_end_child (BOBGUI_PANED (vpaned3), TRUE);
  bobgui_paned_set_shrink_end_child (BOBGUI_PANED (vpaned3), TRUE);

  hbox3 = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 0);
  bobgui_paned_set_end_child (BOBGUI_PANED (hpaned6), hbox3);
  bobgui_paned_set_resize_end_child (BOBGUI_PANED (hpaned6), TRUE);
  bobgui_paned_set_shrink_end_child (BOBGUI_PANED (hpaned6), TRUE);

  vpaned4 = bobgui_paned_new (BOBGUI_ORIENTATION_VERTICAL);
  bobgui_box_append (BOBGUI_BOX (hbox3), vpaned4);

  button21 = bobgui_button_new_with_label ("button21");
  bobgui_paned_set_start_child (BOBGUI_PANED (vpaned4), button21);
  bobgui_paned_set_resize_start_child (BOBGUI_PANED (vpaned4), FALSE);
  bobgui_paned_set_shrink_start_child (BOBGUI_PANED (vpaned4), TRUE);

  button20 = bobgui_button_new_with_label ("button20");
  bobgui_paned_set_end_child (BOBGUI_PANED (vpaned4), button20);
  bobgui_paned_set_resize_end_child (BOBGUI_PANED (vpaned4), TRUE);
  bobgui_paned_set_shrink_end_child (BOBGUI_PANED (vpaned4), TRUE);

  vpaned5 = bobgui_paned_new (BOBGUI_ORIENTATION_VERTICAL);
  bobgui_box_append (BOBGUI_BOX (hbox3), vpaned5);

  button23 = bobgui_button_new_with_label ("button23");
  bobgui_paned_set_start_child (BOBGUI_PANED (vpaned5), button23);
  bobgui_paned_set_resize_start_child (BOBGUI_PANED (vpaned5), FALSE);
  bobgui_paned_set_shrink_start_child (BOBGUI_PANED (vpaned5), TRUE);

  button22 = bobgui_button_new_with_label ("button22");
  bobgui_paned_set_end_child (BOBGUI_PANED (vpaned5), button22);
  bobgui_paned_set_resize_end_child (BOBGUI_PANED (vpaned5), TRUE);
  bobgui_paned_set_shrink_end_child (BOBGUI_PANED (vpaned5), TRUE);

  vpaned6 = bobgui_paned_new (BOBGUI_ORIENTATION_VERTICAL);
  bobgui_box_append (BOBGUI_BOX (hbox3), vpaned6);

  button25 = bobgui_button_new_with_label ("button25");
  bobgui_paned_set_start_child (BOBGUI_PANED (vpaned6), button25);
  bobgui_paned_set_resize_start_child (BOBGUI_PANED (vpaned6), FALSE);
  bobgui_paned_set_shrink_start_child (BOBGUI_PANED (vpaned6), TRUE);

  button24 = bobgui_button_new_with_label ("button24");
  bobgui_paned_set_end_child (BOBGUI_PANED (vpaned6), button24);
  bobgui_paned_set_resize_end_child (BOBGUI_PANED (vpaned6), TRUE);
  bobgui_paned_set_shrink_end_child (BOBGUI_PANED (vpaned6), TRUE);

  return window4;
}

static void
create_paned_keyboard_navigation (BobguiWidget *widget)
{
  static BobguiWidget *window1 = NULL;
  static BobguiWidget *window2 = NULL;
  static BobguiWidget *window3 = NULL;
  static BobguiWidget *window4 = NULL;

  if (window1 && 
     (bobgui_widget_get_display (window1) != bobgui_widget_get_display (widget)))
    {
      bobgui_window_destroy (BOBGUI_WINDOW (window1));
      bobgui_window_destroy (BOBGUI_WINDOW (window2));
      bobgui_window_destroy (BOBGUI_WINDOW (window3));
      bobgui_window_destroy (BOBGUI_WINDOW (window4));
    }
  
  if (!window1)
    {
      window1 = paned_keyboard_window1 (widget);
      g_object_add_weak_pointer (G_OBJECT (window1), (gpointer *)&window1);
    }

  if (!window2)
    {
      window2 = paned_keyboard_window2 (widget);
      g_object_add_weak_pointer (G_OBJECT (window2), (gpointer *)&window2);
    }

  if (!window3)
    {
      window3 = paned_keyboard_window3 (widget);
      g_object_add_weak_pointer (G_OBJECT (window3), (gpointer *)&window3);
    }

  if (!window4)
    {
      window4 = paned_keyboard_window4 (widget);
      g_object_add_weak_pointer (G_OBJECT (window4), (gpointer *)&window4);
    }

  if (bobgui_widget_get_visible (window1))
    bobgui_window_destroy (BOBGUI_WINDOW (window1));
  else
    bobgui_widget_show (BOBGUI_WIDGET (window1));

  if (bobgui_widget_get_visible (window2))
    bobgui_window_destroy (BOBGUI_WINDOW (window2));
  else
    bobgui_widget_show (BOBGUI_WIDGET (window2));

  if (bobgui_widget_get_visible (window3))
    bobgui_window_destroy (BOBGUI_WINDOW (window3));
  else
    bobgui_widget_show (BOBGUI_WIDGET (window3));

  if (bobgui_widget_get_visible (window4))
    bobgui_window_destroy (BOBGUI_WINDOW (window4));
  else
    bobgui_widget_show (BOBGUI_WIDGET (window4));
}

/*
 * WM Hints demo
 */

static void
create_wmhints (BobguiWidget *widget)
{
  static BobguiWidget *window = NULL;
  BobguiWidget *label;
  BobguiWidget *separator;
  BobguiWidget *button;
  BobguiWidget *box1;
  BobguiWidget *box2;
  GdkPixbuf *pixbuf;
  GdkTexture *texture;
  GList *list;

  if (!window)
    {
      window = bobgui_window_new ();

      bobgui_window_set_display (BOBGUI_WINDOW (window),
                              bobgui_widget_get_display (widget));
      g_object_add_weak_pointer (G_OBJECT (window), (gpointer *)&window);

      bobgui_window_set_title (BOBGUI_WINDOW (window), "WM Hints");

      bobgui_widget_realize (window);

      pixbuf = gdk_pixbuf_new_from_xpm_data ((const char **) openfile);
      texture = gdk_texture_new_for_pixbuf (pixbuf);

      list = g_list_prepend (NULL, texture);

      g_list_free (list);
      g_object_unref (texture);
      g_object_unref (pixbuf);

      box1 = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 0);
      bobgui_window_set_child (BOBGUI_WINDOW (window), box1);

      label = bobgui_label_new ("Try iconizing me!");
      bobgui_widget_set_size_request (label, 150, 50);

      bobgui_box_append (BOBGUI_BOX (box1), label);

      separator = bobgui_separator_new (BOBGUI_ORIENTATION_HORIZONTAL);
      bobgui_box_append (BOBGUI_BOX (box1), separator);


      box2 = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 10);
      bobgui_box_append (BOBGUI_BOX (box1), box2);


      button = bobgui_button_new_with_label ("close");

      g_signal_connect_swapped (button, "clicked",
				G_CALLBACK (bobgui_window_destroy),
				window);

      bobgui_box_append (BOBGUI_BOX (box2), button);
      bobgui_window_set_default_widget (BOBGUI_WINDOW (window), button);
    }

  if (!bobgui_widget_get_visible (window))
    bobgui_window_present (BOBGUI_WINDOW (window));
  else
    bobgui_window_destroy (BOBGUI_WINDOW (window));
}

/*
 * Window state tracking
 */

static void
surface_state_callback (GdkSurface  *window,
                       GParamSpec *pspec,
                       BobguiWidget  *label)
{
  char *msg;
  GdkToplevelState new_state;

  new_state = gdk_toplevel_get_state (GDK_TOPLEVEL (window));
  msg = g_strconcat ((const char *)g_object_get_data (G_OBJECT (label), "title"), ": ",
                     (new_state & GDK_TOPLEVEL_STATE_MINIMIZED) ?
                     "minimized" : "not minimized", ", ",
                     (new_state & GDK_TOPLEVEL_STATE_SUSPENDED) ?
                     "suspended" : "not suspended", ", ",
                     (new_state & GDK_TOPLEVEL_STATE_STICKY) ?
                     "sticky" : "not sticky", ", ",
                     (new_state & GDK_TOPLEVEL_STATE_MAXIMIZED) ?
                     "maximized" : "not maximized", ", ",
                     (new_state & GDK_TOPLEVEL_STATE_FULLSCREEN) ?
                     "fullscreen" : "not fullscreen", ", ",
                     (new_state & GDK_TOPLEVEL_STATE_ABOVE) ?
                     "above" : "not above", ", ",
                     (new_state & GDK_TOPLEVEL_STATE_BELOW) ?
                     "below" : "not below", ", ",
                     NULL);

  bobgui_label_set_text (BOBGUI_LABEL (label), msg);

  g_free (msg);
}

static BobguiWidget*
tracking_label (BobguiWidget *window)
{
  BobguiWidget *label;
  BobguiWidget *hbox;
  BobguiWidget *button;

  hbox = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 5);

  g_signal_connect_object (hbox,
			   "destroy",
			   G_CALLBACK (bobgui_window_destroy),
			   window,
			   G_CONNECT_SWAPPED);

  label = bobgui_label_new ("<no window state events received>");
  bobgui_label_set_wrap (BOBGUI_LABEL (label), TRUE);
  bobgui_box_append (BOBGUI_BOX (hbox), label);

  g_object_set_data (G_OBJECT (label), "title", (gpointer)bobgui_window_get_title (BOBGUI_WINDOW (window)));
  g_signal_connect_object (bobgui_native_get_surface (BOBGUI_NATIVE (window)), "notify::state",
                           G_CALLBACK (surface_state_callback),
                           label,
                           0);

  button = bobgui_button_new_with_label ("Unminimize");
  g_signal_connect_object (button,
			   "clicked",
			   G_CALLBACK (bobgui_window_unminimize),
                           window,
			   G_CONNECT_SWAPPED);
  bobgui_box_append (BOBGUI_BOX (hbox), button);

  button = bobgui_button_new_with_label ("Minimize");
  g_signal_connect_object (button,
			   "clicked",
			   G_CALLBACK (bobgui_window_minimize),
                           window,
			   G_CONNECT_SWAPPED);
  bobgui_box_append (BOBGUI_BOX (hbox), button);

  button = bobgui_button_new_with_label ("Fullscreen");
  g_signal_connect_object (button,
			   "clicked",
			   G_CALLBACK (bobgui_window_fullscreen),
                           window,
			   G_CONNECT_SWAPPED);
  bobgui_box_append (BOBGUI_BOX (hbox), button);

  button = bobgui_button_new_with_label ("Unfullscreen");
  g_signal_connect_object (button,
			   "clicked",
			   G_CALLBACK (bobgui_window_unfullscreen),
                           window,
			   G_CONNECT_SWAPPED);
  bobgui_box_append (BOBGUI_BOX (hbox), button);

  button = bobgui_button_new_with_label ("Present");
  g_signal_connect_object (button,
			   "clicked",
			   G_CALLBACK (bobgui_window_present),
                           window,
			   G_CONNECT_SWAPPED);
  bobgui_box_append (BOBGUI_BOX (hbox), button);

  button = bobgui_button_new_with_label ("Show");
  g_signal_connect_object (button,
			   "clicked",
			   G_CALLBACK (bobgui_widget_show),
                           window,
			   G_CONNECT_SWAPPED);
  bobgui_box_append (BOBGUI_BOX (hbox), button);

  return hbox;
}

static BobguiWidget*
get_state_controls (BobguiWidget *window)
{
  BobguiWidget *vbox;
  BobguiWidget *button;

  vbox = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 0);

  button = bobgui_button_new_with_label ("Maximize");
  g_signal_connect_object (button,
			   "clicked",
			   G_CALLBACK (bobgui_window_maximize),
			   window,
			   G_CONNECT_SWAPPED);
  bobgui_box_append (BOBGUI_BOX (vbox), button);

  button = bobgui_button_new_with_label ("Unmaximize");
  g_signal_connect_object (button,
			   "clicked",
			   G_CALLBACK (bobgui_window_unmaximize),
			   window,
			   G_CONNECT_SWAPPED);
  bobgui_box_append (BOBGUI_BOX (vbox), button);

  button = bobgui_button_new_with_label ("Minimize");
  g_signal_connect_object (button,
			   "clicked",
			   G_CALLBACK (bobgui_window_minimize),
			   window,
			   G_CONNECT_SWAPPED);
  bobgui_box_append (BOBGUI_BOX (vbox), button);

  button = bobgui_button_new_with_label ("Fullscreen");
  g_signal_connect_object (button,
			   "clicked",
			   G_CALLBACK (bobgui_window_fullscreen),
                           window,
			   G_CONNECT_SWAPPED);
  bobgui_box_append (BOBGUI_BOX (vbox), button);

  button = bobgui_button_new_with_label ("Unfullscreen");
  g_signal_connect_object (button,
			   "clicked",
                           G_CALLBACK (bobgui_window_unfullscreen),
			   window,
			   G_CONNECT_SWAPPED);
  bobgui_box_append (BOBGUI_BOX (vbox), button);

  button = bobgui_button_new_with_label ("Hide (withdraw)");
  g_signal_connect_object (button,
			   "clicked",
			   G_CALLBACK (bobgui_widget_hide),
			   window,
			   G_CONNECT_SWAPPED);
  bobgui_box_append (BOBGUI_BOX (vbox), button);

  return vbox;
}

static void
create_surface_states (BobguiWidget *widget)
{
  static BobguiWidget *window = NULL;
  BobguiWidget *label;
  BobguiWidget *box1;
  BobguiWidget *iconified;
  BobguiWidget *normal;
  BobguiWidget *controls;

  if (!window)
    {
      window = bobgui_window_new ();
      bobgui_window_set_display (BOBGUI_WINDOW (window),
                              bobgui_widget_get_display (widget));
      g_object_add_weak_pointer (G_OBJECT (window), (gpointer *)&window);

      bobgui_window_set_title (BOBGUI_WINDOW (window), "Surface states");
      
      box1 = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 0);
      bobgui_window_set_child (BOBGUI_WINDOW (window), box1);

      iconified = bobgui_window_new ();

      bobgui_window_set_display (BOBGUI_WINDOW (iconified),
			      bobgui_widget_get_display (widget));
      
      g_signal_connect_object (iconified, "destroy",
			       G_CALLBACK (bobgui_window_destroy),
			       window,
			       G_CONNECT_SWAPPED);
      bobgui_window_minimize (BOBGUI_WINDOW (iconified));
      bobgui_window_set_title (BOBGUI_WINDOW (iconified), "Minimized initially");
      controls = get_state_controls (iconified);
      bobgui_window_set_child (BOBGUI_WINDOW (iconified), controls);
      
      normal = bobgui_window_new ();

      bobgui_window_set_display (BOBGUI_WINDOW (normal),
			      bobgui_widget_get_display (widget));
      
      g_signal_connect_object (normal, "destroy",
			       G_CALLBACK (bobgui_window_destroy),
			       window,
			       G_CONNECT_SWAPPED);
      
      bobgui_window_set_title (BOBGUI_WINDOW (normal), "Unminimized initially");
      controls = get_state_controls (normal);
      bobgui_window_set_child (BOBGUI_WINDOW (normal), controls);
      
      bobgui_widget_realize (iconified);
      bobgui_widget_realize (normal);

      label = tracking_label (iconified);
      bobgui_box_append (BOBGUI_BOX (box1), label);

      label = tracking_label (normal);
      bobgui_box_append (BOBGUI_BOX (box1), label);

      bobgui_widget_show (iconified);
      bobgui_widget_show (normal);
    }

  if (!bobgui_widget_get_visible (window))
    bobgui_window_present (BOBGUI_WINDOW (window));
  else
    bobgui_window_destroy (BOBGUI_WINDOW (window));
}

/*
 * Window sizing
 */

static void
get_ints (BobguiWidget *window,
          int       *a,
          int       *b)
{
  BobguiWidget *spin1;
  BobguiWidget *spin2;

  spin1 = g_object_get_data (G_OBJECT (window), "spin1");
  spin2 = g_object_get_data (G_OBJECT (window), "spin2");

  *a = bobgui_spin_button_get_value_as_int (BOBGUI_SPIN_BUTTON (spin1));
  *b = bobgui_spin_button_get_value_as_int (BOBGUI_SPIN_BUTTON (spin2));
}

static void
unset_default_size_callback (BobguiWidget *widget,
                             gpointer   data)
{
  bobgui_window_set_default_size (g_object_get_data (data, "target"),
                               -1, -1);
}

static void
set_default_size_callback (BobguiWidget *widget,
                           gpointer   data)
{
  int w, h;
  
  get_ints (data, &w, &h);

  bobgui_window_set_default_size (g_object_get_data (data, "target"),
                               w, h);
}

static void
unset_size_request_callback (BobguiWidget *widget,
			     gpointer   data)
{
  bobgui_widget_set_size_request (g_object_get_data (data, "target"),
                               -1, -1);
}

static void
set_size_request_callback (BobguiWidget *widget,
			   gpointer   data)
{
  int w, h;
  
  get_ints (data, &w, &h);

  bobgui_widget_set_size_request (g_object_get_data (data, "target"),
                               w, h);
}

static void
resizable_callback (BobguiWidget *widget,
                     gpointer   data)
{
  g_object_set (g_object_get_data (data, "target"),
                "resizable", bobgui_toggle_button_get_active (BOBGUI_TOGGLE_BUTTON (widget)),
                NULL);
}

static BobguiWidget*
window_controls (BobguiWidget *window)
{
  BobguiWidget *control_window;
  BobguiWidget *label;
  BobguiWidget *vbox;
  BobguiWidget *button;
  BobguiWidget *spin;
  BobguiAdjustment *adjustment;
  
  control_window = bobgui_window_new ();

  bobgui_window_set_display (BOBGUI_WINDOW (control_window),
			  bobgui_widget_get_display (window));

  bobgui_window_set_title (BOBGUI_WINDOW (control_window), "Size controls");
  
  g_object_set_data (G_OBJECT (control_window),
                     "target",
                     window);
  
  g_signal_connect_object (control_window,
			   "destroy",
			   G_CALLBACK (bobgui_window_destroy),
                           window,
			   G_CONNECT_SWAPPED);

  vbox = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 5);

  bobgui_window_set_child (BOBGUI_WINDOW (control_window), vbox);

  label = bobgui_label_new ("<no size>");
  bobgui_box_append (BOBGUI_BOX (vbox), label);

  adjustment = bobgui_adjustment_new (10.0, -2000.0, 2000.0, 1.0, 5.0, 0.0);
  spin = bobgui_spin_button_new (adjustment, 0, 0);

  bobgui_box_append (BOBGUI_BOX (vbox), spin);

  g_object_set_data (G_OBJECT (control_window), "spin1", spin);

  adjustment = bobgui_adjustment_new (10.0, -2000.0, 2000.0, 1.0, 5.0, 0.0);
  spin = bobgui_spin_button_new (adjustment, 0, 0);

  bobgui_box_append (BOBGUI_BOX (vbox), spin);

  g_object_set_data (G_OBJECT (control_window), "spin2", spin);

  button = bobgui_button_new_with_label ("Queue resize");
  g_signal_connect_object (button,
			   "clicked",
			   G_CALLBACK (bobgui_widget_queue_resize),
			   window,
			   G_CONNECT_SWAPPED);
  bobgui_box_append (BOBGUI_BOX (vbox), button);

  button = bobgui_button_new_with_label ("Set default size");
  g_signal_connect (button,
		    "clicked",
		    G_CALLBACK (set_default_size_callback),
		    control_window);
  bobgui_box_append (BOBGUI_BOX (vbox), button);

  button = bobgui_button_new_with_label ("Unset default size");
  g_signal_connect (button,
		    "clicked",
		    G_CALLBACK (unset_default_size_callback),
                    control_window);
  bobgui_box_append (BOBGUI_BOX (vbox), button);

  button = bobgui_button_new_with_label ("Set size request");
  g_signal_connect (button,
		    "clicked",
		    G_CALLBACK (set_size_request_callback),
		    control_window);
  bobgui_box_append (BOBGUI_BOX (vbox), button);

  button = bobgui_button_new_with_label ("Unset size request");
  g_signal_connect (button,
		    "clicked",
		    G_CALLBACK (unset_size_request_callback),
                    control_window);
  bobgui_box_append (BOBGUI_BOX (vbox), button);

  button = bobgui_check_button_new_with_label ("Allow resize");
  bobgui_toggle_button_set_active (BOBGUI_TOGGLE_BUTTON (button), TRUE);
  g_signal_connect (button,
		    "toggled",
		    G_CALLBACK (resizable_callback),
                    control_window);
  bobgui_box_append (BOBGUI_BOX (vbox), button);

  button = bobgui_button_new_with_mnemonic ("_Show");
  g_signal_connect_object (button,
			   "clicked",
			   G_CALLBACK (bobgui_widget_show),
			   window,
			   G_CONNECT_SWAPPED);
  bobgui_box_append (BOBGUI_BOX (vbox), button);

  button = bobgui_button_new_with_mnemonic ("_Hide");
  g_signal_connect_object (button,
			   "clicked",
			   G_CALLBACK (bobgui_widget_hide),
                           window,
			   G_CONNECT_SWAPPED);
  bobgui_box_append (BOBGUI_BOX (vbox), button);

  return control_window;
}

static void
create_window_sizing (BobguiWidget *widget)
{
  static BobguiWidget *window = NULL;
  static BobguiWidget *target_window = NULL;

  if (!target_window)
    {
      BobguiWidget *label;
      
      target_window = bobgui_window_new ();
      bobgui_window_set_display (BOBGUI_WINDOW (target_window),
			      bobgui_widget_get_display (widget));
      label = bobgui_label_new (NULL);
      bobgui_label_set_markup (BOBGUI_LABEL (label), "<span foreground=\"purple\"><big>Window being resized</big></span>\nBlah blah blah blah\nblah blah blah\nblah blah blah blah blah");
      bobgui_window_set_child (BOBGUI_WINDOW (target_window), label);
      bobgui_widget_show (target_window);

      g_object_add_weak_pointer (G_OBJECT (target_window), (gpointer *)&target_window);

      window = window_controls (target_window);
      g_object_add_weak_pointer (G_OBJECT (window), (gpointer *)&window);

      bobgui_window_set_title (BOBGUI_WINDOW (target_window), "Window to size");
    }

  /* don't show target window by default, we want to allow testing
   * of behavior on first show.
   */
  
  if (!bobgui_widget_get_visible (window))
    bobgui_window_present (BOBGUI_WINDOW (window));
  else
    bobgui_window_destroy (BOBGUI_WINDOW (window));
}

/*
 * BobguiProgressBar
 */

typedef struct _ProgressData {
  BobguiWidget *window;
  BobguiWidget *pbar;
  BobguiWidget *block_spin;
  BobguiWidget *x_align_spin;
  BobguiWidget *y_align_spin;
  BobguiWidget *step_spin;
  BobguiWidget *act_blocks_spin;
  BobguiWidget *label;
  BobguiWidget *omenu1;
  BobguiWidget *elmenu;
  BobguiWidget *omenu2;
  BobguiWidget *entry;
  int timer;
  gboolean activity;
} ProgressData;

static gboolean
progress_timeout (gpointer data)
{
  ProgressData *pdata = data;
  double new_val;
  char *text;

  if (pdata->activity)
    {
      bobgui_progress_bar_pulse (BOBGUI_PROGRESS_BAR (pdata->pbar));

      text = g_strdup_printf ("%s", "???");
    }
  else
    {
      new_val = bobgui_progress_bar_get_fraction (BOBGUI_PROGRESS_BAR (pdata->pbar)) + 0.01;
      if (new_val > 1.00)
        new_val = 0.00;
      bobgui_progress_bar_set_fraction (BOBGUI_PROGRESS_BAR (pdata->pbar), new_val);

      text = g_strdup_printf ("%.0f%%", 100 * new_val);
    }

  bobgui_label_set_text (BOBGUI_LABEL (pdata->label), text);
  g_free (text);

  return TRUE;
}

static void
destroy_progress (BobguiWidget     *widget,
		  ProgressData **pdata)
{
  if ((*pdata)->timer)
    {
      g_source_remove ((*pdata)->timer);
      (*pdata)->timer = 0;
    }
  (*pdata)->window = NULL;
  g_free (*pdata);
  *pdata = NULL;
}

static void
progressbar_toggle_orientation (BobguiWidget *widget, gpointer data)
{
  ProgressData *pdata;
  int i;

  pdata = (ProgressData *) data;

  if (!bobgui_widget_get_mapped (widget))
    return;

  i = bobgui_combo_box_get_active (BOBGUI_COMBO_BOX (widget));

  if (i == 0 || i == 1)
    bobgui_orientable_set_orientation (BOBGUI_ORIENTABLE (pdata->pbar), BOBGUI_ORIENTATION_HORIZONTAL);
  else
    bobgui_orientable_set_orientation (BOBGUI_ORIENTABLE (pdata->pbar), BOBGUI_ORIENTATION_VERTICAL);
 
  if (i == 1 || i == 2)
    bobgui_progress_bar_set_inverted (BOBGUI_PROGRESS_BAR (pdata->pbar), TRUE);
  else
    bobgui_progress_bar_set_inverted (BOBGUI_PROGRESS_BAR (pdata->pbar), FALSE);
}

static void
toggle_show_text (BobguiWidget *widget, ProgressData *pdata)
{
  gboolean active;

  active = bobgui_toggle_button_get_active (BOBGUI_TOGGLE_BUTTON (widget));
  bobgui_progress_bar_set_show_text (BOBGUI_PROGRESS_BAR (pdata->pbar), active);
}

static void
progressbar_toggle_ellipsize (BobguiWidget *widget,
                              gpointer   data)
{
  ProgressData *pdata = data;
  if (bobgui_widget_is_drawable (widget))
    {
      int i = bobgui_combo_box_get_active (BOBGUI_COMBO_BOX (widget));
      bobgui_progress_bar_set_ellipsize (BOBGUI_PROGRESS_BAR (pdata->pbar), i);
    }
}

static void
toggle_activity_mode (BobguiWidget *widget, ProgressData *pdata)
{
  pdata->activity = bobgui_toggle_button_get_active (BOBGUI_TOGGLE_BUTTON (widget));
}

static void
toggle_running (BobguiWidget *widget, ProgressData *pdata)
{
  if (bobgui_toggle_button_get_active (BOBGUI_TOGGLE_BUTTON (widget)))
    {
      if (pdata->timer == 0)
        pdata->timer = g_timeout_add (100, (GSourceFunc)progress_timeout, pdata);
    }
  else
    {
      if (pdata->timer != 0)
        {
          g_source_remove (pdata->timer);
          pdata->timer = 0;
        }
    }
}

static void
entry_changed (BobguiWidget *widget, ProgressData *pdata)
{
  bobgui_progress_bar_set_text (BOBGUI_PROGRESS_BAR (pdata->pbar),
			  bobgui_editable_get_text (BOBGUI_EDITABLE (pdata->entry)));
}

static void
create_progress_bar (BobguiWidget *widget)
{
  BobguiWidget *content_area;
  BobguiWidget *vbox;
  BobguiWidget *vbox2;
  BobguiWidget *hbox;
  BobguiWidget *check;
  BobguiWidget *frame;
  BobguiWidget *grid;
  BobguiWidget *label;
  static ProgressData *pdata = NULL;

  static const char *items1[] =
  {
    "Left-Right",
    "Right-Left",
    "Bottom-Top",
    "Top-Bottom"
  };

    static const char *ellipsize_items[] = {
    "None",     // PANGO_ELLIPSIZE_NONE,
    "Start",    // PANGO_ELLIPSIZE_START,
    "Middle",   // PANGO_ELLIPSIZE_MIDDLE,
    "End",      // PANGO_ELLIPSIZE_END
  };
  
  if (!pdata)
    pdata = g_new0 (ProgressData, 1);

  if (!pdata->window)
    {
      pdata->window = bobgui_dialog_new ();

      bobgui_window_set_display (BOBGUI_WINDOW (pdata->window),
	 		      bobgui_widget_get_display (widget));

      bobgui_window_set_resizable (BOBGUI_WINDOW (pdata->window), TRUE);

      g_signal_connect (pdata->window, "destroy",
			G_CALLBACK (destroy_progress),
			&pdata);
      pdata->timer = 0;

      content_area = bobgui_dialog_get_content_area (BOBGUI_DIALOG (pdata->window));

      bobgui_window_set_title (BOBGUI_WINDOW (pdata->window), "BobguiProgressBar");

      vbox = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 5);
      bobgui_box_append (BOBGUI_BOX (content_area), vbox);

      frame = bobgui_frame_new ("Progress");
      bobgui_box_append (BOBGUI_BOX (vbox), frame);

      vbox2 = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 5);
      bobgui_frame_set_child (BOBGUI_FRAME (frame), vbox2);

      pdata->pbar = bobgui_progress_bar_new ();
      bobgui_progress_bar_set_ellipsize (BOBGUI_PROGRESS_BAR (pdata->pbar),
                                      PANGO_ELLIPSIZE_MIDDLE);
      bobgui_widget_set_halign (pdata->pbar, BOBGUI_ALIGN_CENTER);
      bobgui_widget_set_valign (pdata->pbar, BOBGUI_ALIGN_CENTER);
      bobgui_box_append (BOBGUI_BOX (vbox2), pdata->pbar);

      hbox = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 5);
      bobgui_widget_set_halign (hbox, BOBGUI_ALIGN_CENTER);
      bobgui_widget_set_valign (hbox, BOBGUI_ALIGN_CENTER);
      bobgui_box_append (BOBGUI_BOX (vbox2), hbox);
      label = bobgui_label_new ("Label updated by user :");
      bobgui_box_append (BOBGUI_BOX (hbox), label);
      pdata->label = bobgui_label_new ("");
      bobgui_box_append (BOBGUI_BOX (hbox), pdata->label);

      frame = bobgui_frame_new ("Options");
      bobgui_box_append (BOBGUI_BOX (vbox), frame);

      vbox2 = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 5);
      bobgui_frame_set_child (BOBGUI_FRAME (frame), vbox2);

      grid = bobgui_grid_new ();
      bobgui_grid_set_row_spacing (BOBGUI_GRID (grid), 10);
      bobgui_grid_set_column_spacing (BOBGUI_GRID (grid), 10);
      bobgui_box_append (BOBGUI_BOX (vbox2), grid);

      label = bobgui_label_new ("Orientation :");
      bobgui_grid_attach (BOBGUI_GRID (grid), label, 0, 0, 1, 1);
      bobgui_widget_set_halign (label, BOBGUI_ALIGN_START);
      bobgui_widget_set_valign (label, BOBGUI_ALIGN_CENTER);

      pdata->omenu1 = build_option_menu (items1, 4, 0,
					 progressbar_toggle_orientation,
					 pdata);
      bobgui_grid_attach (BOBGUI_GRID (grid), pdata->omenu1, 1, 0, 1, 1);
      
      check = bobgui_check_button_new_with_label ("Running");
      g_signal_connect (check, "toggled",
			G_CALLBACK (toggle_running),
			pdata);
      bobgui_grid_attach (BOBGUI_GRID (grid), check, 0, 1, 2, 1);
      bobgui_toggle_button_set_active (BOBGUI_TOGGLE_BUTTON (check), TRUE);

      check = bobgui_check_button_new_with_label ("Show text");
      g_signal_connect (check, "clicked",
			G_CALLBACK (toggle_show_text),
			pdata);
      bobgui_grid_attach (BOBGUI_GRID (grid), check, 0, 2, 1, 1);

      hbox = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 0);
      bobgui_grid_attach (BOBGUI_GRID (grid), hbox, 1, 2, 1, 1);

      label = bobgui_label_new ("Text: ");
      bobgui_box_append (BOBGUI_BOX (hbox), label);

      pdata->entry = bobgui_entry_new ();
      bobgui_widget_set_hexpand (pdata->entry, TRUE);
      g_signal_connect (pdata->entry, "changed",
			G_CALLBACK (entry_changed),
			pdata);
      bobgui_box_append (BOBGUI_BOX (hbox), pdata->entry);
      bobgui_widget_set_size_request (pdata->entry, 100, -1);

      label = bobgui_label_new ("Ellipsize text :");
      bobgui_grid_attach (BOBGUI_GRID (grid), label, 0, 10, 1, 1);

      bobgui_widget_set_halign (label, BOBGUI_ALIGN_START);
      bobgui_widget_set_valign (label, BOBGUI_ALIGN_CENTER);
      pdata->elmenu = build_option_menu (ellipsize_items,
                                         sizeof (ellipsize_items) / sizeof (ellipsize_items[0]),
                                         2, // PANGO_ELLIPSIZE_MIDDLE
					 progressbar_toggle_ellipsize,
					 pdata);
      bobgui_grid_attach (BOBGUI_GRID (grid), pdata->elmenu, 1, 10, 1, 1);

      check = bobgui_check_button_new_with_label ("Activity mode");
      g_signal_connect (check, "clicked",
			G_CALLBACK (toggle_activity_mode), pdata);
      bobgui_grid_attach (BOBGUI_GRID (grid), check, 0, 15, 1, 1);

      bobgui_dialog_add_button (BOBGUI_DIALOG (pdata->window), "Close", BOBGUI_RESPONSE_CLOSE);
      g_signal_connect (pdata->window, "response",
			G_CALLBACK (bobgui_window_destroy),
			NULL);
    }

  if (!bobgui_widget_get_visible (pdata->window))
    bobgui_widget_show (pdata->window);
  else
    bobgui_window_destroy (BOBGUI_WINDOW (pdata->window));
}

/*
 * Timeout Test
 */

static int timer = 0;

static int
timeout_test (BobguiWidget *label)
{
  static int count = 0;
  static char buffer[32];

  sprintf (buffer, "count: %d", ++count);
  bobgui_label_set_text (BOBGUI_LABEL (label), buffer);

  return TRUE;
}

static void
start_timeout_test (BobguiWidget *widget,
		    BobguiWidget *label)
{
  if (!timer)
    {
      timer = g_timeout_add (100, (GSourceFunc)timeout_test, label);
    }
}

static void
stop_timeout_test (BobguiWidget *widget,
		   gpointer   data)
{
  if (timer)
    {
      g_source_remove (timer);
      timer = 0;
    }
}

static void
destroy_timeout_test (BobguiWidget  *widget,
		      BobguiWidget **window)
{
  stop_timeout_test (NULL, NULL);

  *window = NULL;
}

static void
create_timeout_test (BobguiWidget *widget)
{
  static BobguiWidget *window = NULL;
  BobguiWidget *action_area, *content_area;
  BobguiWidget *button;
  BobguiWidget *label;

  if (!window)
    {
      window = bobgui_dialog_new ();

      bobgui_window_set_display (BOBGUI_WINDOW (window),
			      bobgui_widget_get_display (widget));

      g_signal_connect (window, "destroy",
			G_CALLBACK (destroy_timeout_test),
			&window);

      content_area = bobgui_dialog_get_content_area (BOBGUI_DIALOG (window));
      action_area = bobgui_dialog_get_content_area (BOBGUI_DIALOG (window));

      bobgui_window_set_title (BOBGUI_WINDOW (window), "Timeout Test");

      label = bobgui_label_new ("count: 0");
      bobgui_widget_set_margin_start (label, 10);
      bobgui_widget_set_margin_end (label, 10);
      bobgui_widget_set_margin_top (label, 10);
      bobgui_widget_set_margin_bottom (label, 10);
      bobgui_box_append (BOBGUI_BOX (content_area), label);

      button = bobgui_button_new_with_label ("close");
      g_signal_connect_swapped (button, "clicked",
				G_CALLBACK (bobgui_window_destroy),
				window);
      bobgui_box_append (BOBGUI_BOX (action_area), button);
      bobgui_window_set_default_widget (BOBGUI_WINDOW (window), button);

      button = bobgui_button_new_with_label ("start");
      g_signal_connect (button, "clicked",
			G_CALLBACK(start_timeout_test),
			label);
      bobgui_box_append (BOBGUI_BOX (action_area), button);

      button = bobgui_button_new_with_label ("stop");
      g_signal_connect (button, "clicked",
			G_CALLBACK (stop_timeout_test),
			NULL);
      bobgui_box_append (BOBGUI_BOX (action_area), button);
    }

  if (!bobgui_widget_get_visible (window))
    bobgui_window_present (BOBGUI_WINDOW (window));
  else
    bobgui_window_destroy (BOBGUI_WINDOW (window));
}

static void
show_native (BobguiWidget *button,
             BobguiFileChooserNative *native)
{
  bobgui_native_dialog_show (BOBGUI_NATIVE_DIALOG (native));
}

static void
hide_native (BobguiWidget *button,
             BobguiFileChooserNative *native)
{
  bobgui_native_dialog_hide (BOBGUI_NATIVE_DIALOG (native));
}

static void
native_response (BobguiNativeDialog *self,
                 int response_id,
                 BobguiWidget *label)
{
  static int count = 0;
  char *res;
  GString *s;
  char *response;
  BobguiFileFilter *filter;
  GListModel *files;
  guint i, n;

  files = bobgui_file_chooser_get_files (BOBGUI_FILE_CHOOSER (self));
  filter = bobgui_file_chooser_get_filter (BOBGUI_FILE_CHOOSER (self));
  s = g_string_new ("");
  n = g_list_model_get_n_items (files);
  for (i = 0; i < n; i++)
    {
      GFile *file = g_list_model_get_item (files, i);
      char *uri = g_file_get_uri (file);
      g_string_prepend (s, uri);
      g_string_prepend (s, "\n");
      g_free (uri);
      g_object_unref (file);
    }
  g_object_unref (files);

  switch (response_id)
    {
    case BOBGUI_RESPONSE_NONE:
      response = g_strdup ("BOBGUI_RESPONSE_NONE");
      break;
    case BOBGUI_RESPONSE_ACCEPT:
      response = g_strdup ("BOBGUI_RESPONSE_ACCEPT");
      break;
    case BOBGUI_RESPONSE_CANCEL:
      response = g_strdup ("BOBGUI_RESPONSE_CANCEL");
      break;
    case BOBGUI_RESPONSE_DELETE_EVENT:
      response = g_strdup ("BOBGUI_RESPONSE_DELETE_EVENT");
      break;
    default:
      response = g_strdup_printf ("%d", response_id);
      break;
    }

  if (filter)
    res = g_strdup_printf ("Response #%d: %s\n"
                           "Filter: %s\n"
                           "Files:\n"
                           "%s",
                           ++count,
                           response,
                           bobgui_file_filter_get_name (filter),
                           s->str);
  else
    res = g_strdup_printf ("Response #%d: %s\n"
                           "NO Filter\n"
                           "Files:\n"
                           "%s",
                           ++count,
                           response,
                           s->str);
  bobgui_label_set_text (BOBGUI_LABEL (label), res);
  g_free (response);
  g_string_free (s, TRUE);
}

static void
native_modal_toggle (BobguiWidget *checkbutton,
                     BobguiFileChooserNative *native)
{
  bobgui_native_dialog_set_modal (BOBGUI_NATIVE_DIALOG (native),
                               bobgui_toggle_button_get_active (BOBGUI_TOGGLE_BUTTON(checkbutton)));
}

static void
native_multi_select_toggle (BobguiWidget *checkbutton,
                            BobguiFileChooserNative *native)
{
  bobgui_file_chooser_set_select_multiple (BOBGUI_FILE_CHOOSER (native),
                                        bobgui_toggle_button_get_active (BOBGUI_TOGGLE_BUTTON(checkbutton)));
}

static void
native_visible_notify_show (GObject	*object,
                            GParamSpec	*pspec,
                            BobguiWidget   *show_button)
{
  BobguiFileChooserNative *native = BOBGUI_FILE_CHOOSER_NATIVE (object);
  gboolean visible;

  visible = bobgui_native_dialog_get_visible (BOBGUI_NATIVE_DIALOG (native));
  bobgui_widget_set_sensitive (show_button, !visible);
}

static void
native_visible_notify_hide (GObject	*object,
                            GParamSpec	*pspec,
                            BobguiWidget   *hide_button)
{
  BobguiFileChooserNative *native = BOBGUI_FILE_CHOOSER_NATIVE (object);
  gboolean visible;

  visible = bobgui_native_dialog_get_visible (BOBGUI_NATIVE_DIALOG (native));
  bobgui_widget_set_sensitive (hide_button, visible);
}

static GFile *
get_some_file (void)
{
  GFile *dir = g_file_new_for_path (g_get_current_dir ());
  GFileEnumerator *e;
  GFile *res = NULL;

  e = g_file_enumerate_children (dir, "*", 0, NULL, NULL);
  if (e)
    {
      GFileInfo *info;

      while (res == NULL)
        {
          info = g_file_enumerator_next_file (e, NULL, NULL);
          if (info)
            {
              if (g_file_info_get_file_type (info) == G_FILE_TYPE_REGULAR)
                {
                  GFile *child = g_file_enumerator_get_child (e, info);
                  res = g_steal_pointer (&child);
                }
              g_object_unref (info);
            }
          else
            break;
        }
    }

  return res;
}

static void
native_action_changed (BobguiWidget *combo,
                       BobguiFileChooserNative *native)
{
  int i;
  gboolean save_as = FALSE;
  i = bobgui_combo_box_get_active (BOBGUI_COMBO_BOX (combo));

  if (i == 4) /* Save as */
    {
      save_as = TRUE;
      i = BOBGUI_FILE_CHOOSER_ACTION_SAVE;
    }

  bobgui_file_chooser_set_action (BOBGUI_FILE_CHOOSER (native),
                               (BobguiFileChooserAction) i);


  if (i == BOBGUI_FILE_CHOOSER_ACTION_SAVE)
    {
      if (save_as)
        {
          GFile *file = get_some_file ();
          bobgui_file_chooser_set_file (BOBGUI_FILE_CHOOSER (native), file, NULL);
        }
      else
        bobgui_file_chooser_set_current_name (BOBGUI_FILE_CHOOSER (native), "newname.txt");
    }
}

static void
native_filter_changed (BobguiWidget *combo,
                       BobguiFileChooserNative *native)
{
  int i;
  GListModel *filters;
  BobguiFileFilter *filter;

  i = bobgui_combo_box_get_active (BOBGUI_COMBO_BOX (combo));

  filters = bobgui_file_chooser_get_filters (BOBGUI_FILE_CHOOSER (native));
  while (g_list_model_get_n_items (filters) > 0)
    {
      BobguiFileFilter *f = g_list_model_get_item (filters, 0);
      bobgui_file_chooser_remove_filter (BOBGUI_FILE_CHOOSER (native), f);
      g_object_unref (f);
    }
  g_object_unref (filters);

  switch (i)
    {
    case 0:
      break;
    case 1:   /* pattern */
      filter = bobgui_file_filter_new ();
      bobgui_file_filter_set_name (filter, "Text");
      bobgui_file_filter_add_suffix (filter, "doc");
      bobgui_file_filter_add_suffix (filter, "txt");
      bobgui_file_chooser_add_filter (BOBGUI_FILE_CHOOSER (native), filter);
      g_object_unref (filter);

      filter = bobgui_file_filter_new ();
      bobgui_file_filter_set_name (filter, "Images");
      bobgui_file_filter_add_pixbuf_formats (filter);
      bobgui_file_chooser_add_filter (BOBGUI_FILE_CHOOSER (native), filter);
      bobgui_file_chooser_set_filter (BOBGUI_FILE_CHOOSER (native), filter);
      g_object_unref (filter);

      filter = bobgui_file_filter_new ();
      bobgui_file_filter_set_name (filter, "All");
      bobgui_file_filter_add_pattern (filter, "*");
      bobgui_file_chooser_add_filter (BOBGUI_FILE_CHOOSER (native), filter);
      g_object_unref (filter);
      break;

    case 2:   /* mimetype */
      filter = bobgui_file_filter_new ();
      bobgui_file_filter_set_name (filter, "Text");
      bobgui_file_filter_add_mime_type (filter, "text/plain");
      bobgui_file_chooser_add_filter (BOBGUI_FILE_CHOOSER (native), filter);
      g_object_unref (filter);

      filter = bobgui_file_filter_new ();
      bobgui_file_filter_set_name (filter, "All");
      bobgui_file_filter_add_pattern (filter, "*");
      bobgui_file_chooser_add_filter (BOBGUI_FILE_CHOOSER (native), filter);
      bobgui_file_chooser_set_filter (BOBGUI_FILE_CHOOSER (native), filter);
      g_object_unref (filter);
      break;
    default:
      g_assert_not_reached ();
    }
}

static void
destroy_native (BobguiFileChooserNative *native)
{
  bobgui_native_dialog_destroy (BOBGUI_NATIVE_DIALOG (native));
  g_object_unref (native);
}

static void
create_native_dialogs (BobguiWidget *widget)
{
  static BobguiWidget *window = NULL;
  BobguiWidget *box, *label;
  BobguiWidget *show_button, *hide_button, *check_button;
  BobguiFileChooserNative *native;
  BobguiWidget *combo;

  if (!window)
    {
      GFile *path;

      window = bobgui_window_new ();
      bobgui_window_set_display (BOBGUI_WINDOW (window),
                              bobgui_widget_get_display (widget));

      native = bobgui_file_chooser_native_new ("Native title",
                                            BOBGUI_WINDOW (window),
                                            BOBGUI_FILE_CHOOSER_ACTION_OPEN,
                                            "_accept&native",
                                            "_cancel__native");

      g_signal_connect_swapped (G_OBJECT (window), "destroy", G_CALLBACK (destroy_native), native);

      path = g_file_new_for_path (g_get_current_dir ());
      bobgui_file_chooser_add_shortcut_folder (BOBGUI_FILE_CHOOSER (native), path, NULL);
      g_object_unref (path);

      bobgui_window_set_title (BOBGUI_WINDOW(window), "Native dialog parent");

      box = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 5);
      bobgui_window_set_child (BOBGUI_WINDOW (window), box);

      label = bobgui_label_new ("");
      bobgui_box_append (BOBGUI_BOX (box), label);

      combo = bobgui_combo_box_text_new ();

      bobgui_combo_box_text_append_text (BOBGUI_COMBO_BOX_TEXT (combo), "Open");
      bobgui_combo_box_text_append_text (BOBGUI_COMBO_BOX_TEXT (combo), "Save");
      bobgui_combo_box_text_append_text (BOBGUI_COMBO_BOX_TEXT (combo), "Select Folder");
      bobgui_combo_box_text_append_text (BOBGUI_COMBO_BOX_TEXT (combo), "Create Folder");
      bobgui_combo_box_text_append_text (BOBGUI_COMBO_BOX_TEXT (combo), "Save as");

      g_signal_connect (combo, "changed",
                        G_CALLBACK (native_action_changed), native);
      bobgui_combo_box_set_active (BOBGUI_COMBO_BOX (combo), BOBGUI_FILE_CHOOSER_ACTION_OPEN);
      bobgui_box_append (BOBGUI_BOX (box), combo);

      combo = bobgui_combo_box_text_new ();

      bobgui_combo_box_text_append_text (BOBGUI_COMBO_BOX_TEXT (combo), "No filters");
      bobgui_combo_box_text_append_text (BOBGUI_COMBO_BOX_TEXT (combo), "Pattern filter");
      bobgui_combo_box_text_append_text (BOBGUI_COMBO_BOX_TEXT (combo), "Mimetype filter");

      g_signal_connect (combo, "changed",
                        G_CALLBACK (native_filter_changed), native);
      bobgui_combo_box_set_active (BOBGUI_COMBO_BOX (combo), 0);
      bobgui_box_append (BOBGUI_BOX (box), combo);

      check_button = bobgui_check_button_new_with_label ("Modal");
      g_signal_connect (check_button, "toggled",
                        G_CALLBACK (native_modal_toggle), native);
      bobgui_box_append (BOBGUI_BOX (box), check_button);

      check_button = bobgui_check_button_new_with_label ("Multiple select");
      g_signal_connect (check_button, "toggled",
                        G_CALLBACK (native_multi_select_toggle), native);
      bobgui_box_append (BOBGUI_BOX (box), check_button);

      show_button = bobgui_button_new_with_label ("Show");
      hide_button = bobgui_button_new_with_label ("Hide");
      bobgui_widget_set_sensitive (hide_button, FALSE);

      bobgui_box_append (BOBGUI_BOX (box), show_button);
      bobgui_box_append (BOBGUI_BOX (box), hide_button);

      /* connect signals */
      g_signal_connect (native, "response",
                        G_CALLBACK (native_response), label);
      g_signal_connect (show_button, "clicked",
                        G_CALLBACK (show_native), native);
      g_signal_connect (hide_button, "clicked",
                        G_CALLBACK (hide_native), native);

      g_signal_connect (native, "notify::visible",
                        G_CALLBACK (native_visible_notify_show), show_button);
      g_signal_connect (native, "notify::visible",
                        G_CALLBACK (native_visible_notify_hide), hide_button);

      g_object_add_weak_pointer (G_OBJECT (window), (gpointer *)&window);
    }

  if (!bobgui_widget_get_visible (window))
    bobgui_window_present (BOBGUI_WINDOW (window));
  else
    bobgui_window_destroy (BOBGUI_WINDOW (window));
}

/*
 * Main Window and Exit
 */

static void
do_exit (BobguiWidget *widget, BobguiWidget *window)
{
  bobgui_window_destroy (BOBGUI_WINDOW (window));
  done = TRUE;
  g_main_context_wakeup (NULL);
}

struct {
  const char *label;
  void (*func) (BobguiWidget *widget);
  gboolean do_not_benchmark;
} buttons[] =
{
  { "alpha window", create_alpha_window },
  { "buttons", create_buttons },
  { "check buttons", create_check_buttons },
  { "color selection", create_color_selection },
  { "cursors", create_cursors },
  { "dialog", create_dialog },
  { "display", create_display_screen, TRUE },
  { "entry", create_entry },
  { "expander", create_expander },
  { "flipping", create_flipping },
  { "font selection", create_font_selection },
  { "image", create_image },
  { "labels", create_labels },
  { "listbox", create_listbox },
  { "message dialog", create_message_dialog },
  { "modal window", create_modal_window, TRUE },
  { "native dialogs", create_native_dialogs },
  { "notebook", create_notebook },
  { "panes", create_panes },
  { "paned keyboard", create_paned_keyboard_navigation },
  { "pixbuf", create_pixbuf },
  { "progress bar", create_progress_bar },
  { "radio buttons", create_radio_buttons },
  { "range controls", create_range_controls },
  { "rotated text", create_rotated_text },
  { "scrolled windows", create_scrolled_windows },
  { "size groups", create_size_groups },
  { "spinbutton", create_spins },
  { "statusbar", create_statusbar },
  { "surface states", create_surface_states },
  { "test timeout", create_timeout_test },
  { "toggle buttons", create_toggle_buttons },
  { "tooltips", create_tooltips },
  { "WM hints", create_wmhints },
  { "window sizing", create_window_sizing }
};
int nbuttons = sizeof (buttons) / sizeof (buttons[0]);

static void
quit_cb (BobguiWidget *widget,
         gpointer   user_data)
{
  gboolean *is_done = user_data;

  *is_done = TRUE;

  g_main_context_wakeup (NULL);
}

static void
create_main_window (void)
{
  BobguiWidget *window;
  BobguiWidget *box1;
  BobguiWidget *box2;
  BobguiWidget *scrolled_window;
  BobguiWidget *button;
  BobguiWidget *label;
  char buffer[64];
  BobguiWidget *separator;
  int i;

  window = bobgui_window_new ();
  bobgui_widget_set_name (window, "main_window");
  bobgui_window_set_default_size (BOBGUI_WINDOW (window), -1, 400);

  g_signal_connect (window, "destroy", G_CALLBACK (quit_cb), &done);

  box1 = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 0);
  bobgui_window_set_child (BOBGUI_WINDOW (window), box1);

  if (bobgui_get_micro_version () > 0)
    sprintf (buffer,
	     "Bobgui+ v%d.%d.%d",
	     bobgui_get_major_version (),
	     bobgui_get_minor_version (),
	     bobgui_get_micro_version ());
  else
    sprintf (buffer,
	     "Bobgui+ v%d.%d",
	     bobgui_get_major_version (),
	     bobgui_get_minor_version ());

  label = bobgui_label_new (buffer);
  bobgui_box_append (BOBGUI_BOX (box1), label);
  bobgui_widget_set_name (label, "testbobgui-version-label");

  scrolled_window = bobgui_scrolled_window_new ();
  bobgui_widget_set_margin_top (scrolled_window, 10);
  bobgui_widget_set_margin_bottom (scrolled_window, 10);
  bobgui_widget_set_margin_start (scrolled_window, 10);
  bobgui_widget_set_margin_end (scrolled_window, 10);
  bobgui_widget_set_vexpand (scrolled_window, TRUE);
  bobgui_scrolled_window_set_policy (BOBGUI_SCROLLED_WINDOW (scrolled_window),
     		                  BOBGUI_POLICY_NEVER, 
                                  BOBGUI_POLICY_AUTOMATIC);
  bobgui_box_append (BOBGUI_BOX (box1), scrolled_window);

  box2 = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 0);
  bobgui_widget_set_margin_top (box2, 10);
  bobgui_widget_set_margin_bottom (box2, 10);
  bobgui_widget_set_margin_start (box2, 10);
  bobgui_widget_set_margin_end (box2, 10);
  bobgui_scrolled_window_set_child (BOBGUI_SCROLLED_WINDOW (scrolled_window), box2);
  bobgui_viewport_set_scroll_to_focus (BOBGUI_VIEWPORT (bobgui_widget_get_parent (box2)), TRUE);

  for (i = 0; i < nbuttons; i++)
    {
      button = bobgui_button_new_with_label (buttons[i].label);
      if (buttons[i].func)
        g_signal_connect (button,
			  "clicked",
			  G_CALLBACK(buttons[i].func),
			  NULL);
      else
        bobgui_widget_set_sensitive (button, FALSE);
      bobgui_box_append (BOBGUI_BOX (box2), button);
    }

  separator = bobgui_separator_new (BOBGUI_ORIENTATION_HORIZONTAL);
  bobgui_box_append (BOBGUI_BOX (box1), separator);

  box2 = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 10);
  bobgui_box_append (BOBGUI_BOX (box1), box2);

  button = bobgui_button_new_with_mnemonic ("_Close");
  bobgui_widget_set_margin_top (button, 10);
  bobgui_widget_set_margin_bottom (button, 10);
  bobgui_widget_set_margin_start (button, 10);
  bobgui_widget_set_margin_end (button, 10);
  g_signal_connect (button, "clicked",
		    G_CALLBACK (do_exit),
		    window);
  bobgui_box_append (BOBGUI_BOX (box2), button);
  bobgui_window_set_default_widget (BOBGUI_WINDOW (window), button);

  bobgui_window_present (BOBGUI_WINDOW (window));
}

static char *
pad (const char *str, int to)
{
  static char buf[256];
  int len = strlen (str);
  int i;

  for (i = 0; i < to; i++)
    buf[i] = ' ';

  buf[to] = '\0';

  memcpy (buf, str, len);

  return buf;
}

static void
bench_iteration (BobguiWidget *widget, void (* fn) (BobguiWidget *widget))
{
  fn (widget); /* on */
  while (g_main_context_iteration (NULL, FALSE));
  fn (widget); /* off */
  while (g_main_context_iteration (NULL, FALSE));
}

static void
do_real_bench (BobguiWidget *widget, void (* fn) (BobguiWidget *widget), const char *name, int num)
{
  gint64 t0, t1;
  double dt_first;
  double dt;
  int n;
  static gboolean printed_headers = FALSE;

  if (!printed_headers) {
    g_print ("Test                 Iters      First      Other\n");
    g_print ("-------------------- ----- ---------- ----------\n");
    printed_headers = TRUE;
  }

  t0 = g_get_monotonic_time ();
  bench_iteration (widget, fn);
  t1 = g_get_monotonic_time ();

  dt_first = ((double)(t1 - t0)) / 1000.0;

  t0 = g_get_monotonic_time ();
  for (n = 0; n < num - 1; n++)
    bench_iteration (widget, fn);
  t1 = g_get_monotonic_time ();
  dt = ((double)(t1 - t0)) / 1000.0;

  g_print ("%s %5d ", pad (name, 20), num);
  if (num > 1)
    g_print ("%10.1f %10.1f\n", dt_first, dt/(num-1));
  else
    g_print ("%10.1f\n", dt_first);
}

static void
do_bench (char* what, int num)
{
  int i;
  BobguiWidget *widget;
  void (* fn) (BobguiWidget *widget);
  fn = NULL;
  widget = bobgui_window_new ();

  if (g_ascii_strcasecmp (what, "ALL") == 0)
    {
      for (i = 0; i < nbuttons; i++)
	{
	  if (!buttons[i].do_not_benchmark)
	    do_real_bench (widget, buttons[i].func, buttons[i].label, num);
	}

      return;
    }
  else
    {
      for (i = 0; i < nbuttons; i++)
	{
	  if (strcmp (buttons[i].label, what) == 0)
	    {
	      fn = buttons[i].func;
	      break;
	    }
	}
      
      if (!fn)
	g_print ("Can't bench: \"%s\" not found.\n", what);
      else
	do_real_bench (widget, fn, buttons[i].label, num);
    }
}

static void G_GNUC_NORETURN
usage (void)
{
  fprintf (stderr, "Usage: testbobgui [--bench ALL|<bench>[:<count>]]\n");
  exit (1);
}

int
main (int argc, char *argv[])
{
  BobguiCssProvider *provider, *memory_provider;
  GdkDisplay *display;
  int i;
  gboolean done_benchmarks = FALSE;

  srand (time (NULL));

  g_set_application_name ("BOBGUI Test Program");

#ifdef BOBGUI_SRCDIR
  g_chdir (BOBGUI_SRCDIR);
#endif

  bobgui_init ();

  provider = bobgui_css_provider_new ();

  /* Check to see if we are being run from the correct
   * directory.
   */
  if (file_exists ("testbobgui.css"))
    bobgui_css_provider_load_from_path (provider, "testbobgui.css");
  else if (file_exists ("tests/testbobgui.css"))
    bobgui_css_provider_load_from_path (provider, "tests/testbobgui.css");
  else
    g_warning ("Couldn't find file \"testbobgui.css\".");

  display = gdk_display_get_default ();

  bobgui_style_context_add_provider_for_display (display, BOBGUI_STYLE_PROVIDER (provider),
                                              BOBGUI_STYLE_PROVIDER_PRIORITY_APPLICATION);
  g_object_unref (provider);

  /*  benchmarking
   */
  for (i = 1; i < argc; i++)
    {
      if (strncmp (argv[i], "--bench", strlen("--bench")) == 0)
        {
          int num = 1;
	  char *nextarg;
	  char *what;
	  char *count;
	  
	  nextarg = strchr (argv[i], '=');
	  if (nextarg)
	    nextarg++;
	  else
	    {
	      i++;
	      if (i == argc)
		usage ();
	      nextarg = argv[i];
	    }

	  count = strchr (nextarg, ':');
	  if (count)
	    {
	      what = g_strndup (nextarg, count - nextarg);
	      count++;
	      num = atoi (count);
	      if (num <= 0)
		usage ();
	    }
	  else
	    what = g_strdup (nextarg);

          do_bench (what, num ? num : 1);
	  done_benchmarks = TRUE;
        }
      else
	usage ();
    }
  if (done_benchmarks)
    return 0;

  memory_provider = bobgui_css_provider_new ();
  bobgui_css_provider_load_from_data (memory_provider,
                                   "#testbobgui-version-label {\n"
                                   "  color: #f00;\n"
                                   "  font-family: Sans;\n"
                                   "  font-size: 18px;\n"
                                   "}",
                                   -1);
  bobgui_style_context_add_provider_for_display (display, BOBGUI_STYLE_PROVIDER (memory_provider),
                                              BOBGUI_STYLE_PROVIDER_PRIORITY_APPLICATION + 1);

  create_main_window ();

  while (!done)
    g_main_context_iteration (NULL, TRUE);

  if (1)
    {
      while (g_main_context_pending (NULL))
	g_main_context_iteration (NULL, FALSE);
#if 0
      sleep (1);
      while (g_main_context_pending (NULL))
	g_main_context_iteration (NULL, FALSE);
#endif
    }
  return 0;
}
