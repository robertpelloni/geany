/* 
 * BOBGUI - The GIMP Toolkit
 * Copyright (C) 1999  Red Hat, Inc.
 * Copyright (C) 2002  Anders Carlsson <andersca@gnu.org>
 * Copyright (C) 2003  Matthias Clasen <mclasen@redhat.com>
 * Copyright (C) 2005  Carlos Garnacho Parro <carlosg@gnome.org>
 *
 * All rights reserved.
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

#include <bobgui/bobgui.h>

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

static BobguiWidget*
get_test_page (const char *text)
{
  return bobgui_label_new (text);
}

typedef struct {
  BobguiAssistant *assistant;
  BobguiWidget    *page;
} PageData;

static void
complete_cb (BobguiWidget *check, 
	     gpointer   data)
{
  PageData *pdata = data;
  gboolean complete;

  complete = bobgui_check_button_get_active (BOBGUI_CHECK_BUTTON (check));

  bobgui_assistant_set_page_complete (pdata->assistant,
				   pdata->page,
				   complete);
}
	     
static BobguiWidget *
add_completion_test_page (BobguiWidget   *assistant,
			  const char *text, 
			  gboolean     visible,
			  gboolean     complete)
{
  BobguiWidget *page;
  BobguiWidget *check;
  PageData *pdata;

  page = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 0);
  check = bobgui_check_button_new_with_label ("Complete");

  bobgui_box_append (BOBGUI_BOX (page), bobgui_label_new (text));
  bobgui_box_append (BOBGUI_BOX (page), check);
  
  bobgui_check_button_set_active (BOBGUI_CHECK_BUTTON (check), complete);

  pdata = g_new (PageData, 1);
  pdata->assistant = BOBGUI_ASSISTANT (assistant);
  pdata->page = page;
  g_signal_connect (G_OBJECT (check), "toggled", 
		    G_CALLBACK (complete_cb), pdata);


  bobgui_widget_set_visible (page, visible);

  bobgui_assistant_append_page (BOBGUI_ASSISTANT (assistant), page);
  bobgui_assistant_set_page_title (BOBGUI_ASSISTANT (assistant), page, text);
  bobgui_assistant_set_page_complete (BOBGUI_ASSISTANT (assistant), page, complete);

  return page;
}

static void
cancel_callback (BobguiWidget *widget)
{
  g_print ("cancel\n");

  bobgui_widget_hide (widget);
}

static void
close_callback (BobguiWidget *widget)
{
  g_print ("close\n");

  bobgui_widget_hide (widget);
}

static void
apply_callback (BobguiWidget *widget)
{
  g_print ("apply\n");
}

static gboolean
progress_timeout (BobguiWidget *assistant)
{
  BobguiWidget *progress;
  int current_page;
  double value;

  current_page = bobgui_assistant_get_current_page (BOBGUI_ASSISTANT (assistant));
  progress = bobgui_assistant_get_nth_page (BOBGUI_ASSISTANT (assistant), current_page);

  value  = bobgui_progress_bar_get_fraction (BOBGUI_PROGRESS_BAR (progress));
  value += 0.1;
  bobgui_progress_bar_set_fraction (BOBGUI_PROGRESS_BAR (progress), value);

  if (value >= 1.0)
    {
      bobgui_assistant_set_page_complete (BOBGUI_ASSISTANT (assistant), progress, TRUE);
      return FALSE;
    }

  return TRUE;
}

static void
prepare_callback (BobguiWidget *widget, BobguiWidget *page)
{
  if (BOBGUI_IS_LABEL (page))
    g_print ("prepare: %s\n", bobgui_label_get_text (BOBGUI_LABEL (page)));
  else if (bobgui_assistant_get_page_type (BOBGUI_ASSISTANT (widget), page) == BOBGUI_ASSISTANT_PAGE_PROGRESS)
    {
      bobgui_assistant_set_page_complete (BOBGUI_ASSISTANT (widget), page, FALSE);
      bobgui_progress_bar_set_fraction (BOBGUI_PROGRESS_BAR (page), 0.0);
      g_timeout_add (300, (GSourceFunc) progress_timeout, widget);
    }
  else
    g_print ("prepare: %d\n", bobgui_assistant_get_current_page (BOBGUI_ASSISTANT (widget)));
}

static void
create_simple_assistant (BobguiWidget *widget)
{
  static BobguiWidget *assistant = NULL;

  if (!assistant)
    {
      BobguiWidget *page;

      assistant = bobgui_assistant_new ();
      bobgui_window_set_default_size (BOBGUI_WINDOW (assistant), 400, 300);

      g_signal_connect (G_OBJECT (assistant), "cancel",
			G_CALLBACK (cancel_callback), NULL);
      g_signal_connect (G_OBJECT (assistant), "close",
			G_CALLBACK (close_callback), NULL);
      g_signal_connect (G_OBJECT (assistant), "apply",
			G_CALLBACK (apply_callback), NULL);
      g_signal_connect (G_OBJECT (assistant), "prepare",
			G_CALLBACK (prepare_callback), NULL);

      page = get_test_page ("Page 1");
      bobgui_assistant_append_page (BOBGUI_ASSISTANT (assistant), page);
      bobgui_assistant_set_page_title (BOBGUI_ASSISTANT (assistant), page, "Page 1");
      bobgui_assistant_set_page_complete (BOBGUI_ASSISTANT (assistant), page, TRUE);

      page = get_test_page ("Page 2");
      bobgui_assistant_append_page (BOBGUI_ASSISTANT (assistant), page);
      bobgui_assistant_set_page_title (BOBGUI_ASSISTANT (assistant), page, "Page 2");
      bobgui_assistant_set_page_type  (BOBGUI_ASSISTANT (assistant), page, BOBGUI_ASSISTANT_PAGE_CONFIRM);
      bobgui_assistant_set_page_complete (BOBGUI_ASSISTANT (assistant), page, TRUE);
    }

  if (!bobgui_widget_get_visible (assistant))
    bobgui_window_present (BOBGUI_WINDOW (assistant));
  else
    {
      bobgui_window_destroy (BOBGUI_WINDOW (assistant));
      assistant = NULL;
    }
}

static void
create_anonymous_assistant (BobguiWidget *widget)
{
  static BobguiWidget *assistant = NULL;

  if (!assistant)
    {
      BobguiWidget *page;

      assistant = bobgui_assistant_new ();
      bobgui_window_set_default_size (BOBGUI_WINDOW (assistant), 400, 300);

      g_signal_connect (G_OBJECT (assistant), "cancel",
			G_CALLBACK (cancel_callback), NULL);
      g_signal_connect (G_OBJECT (assistant), "close",
			G_CALLBACK (close_callback), NULL);
      g_signal_connect (G_OBJECT (assistant), "apply",
			G_CALLBACK (apply_callback), NULL);
      g_signal_connect (G_OBJECT (assistant), "prepare",
			G_CALLBACK (prepare_callback), NULL);

      page = get_test_page ("Page 1");
      bobgui_assistant_append_page (BOBGUI_ASSISTANT (assistant), page);
      bobgui_assistant_set_page_complete (BOBGUI_ASSISTANT (assistant), page, TRUE);

      page = get_test_page ("Page 2");
      bobgui_assistant_append_page (BOBGUI_ASSISTANT (assistant), page);
      bobgui_assistant_set_page_type  (BOBGUI_ASSISTANT (assistant), page, BOBGUI_ASSISTANT_PAGE_CONFIRM);
      bobgui_assistant_set_page_complete (BOBGUI_ASSISTANT (assistant), page, TRUE);
    }

  if (!bobgui_widget_get_visible (assistant))
    bobgui_widget_show (assistant);
  else
    {
      bobgui_window_destroy (BOBGUI_WINDOW (assistant));
      assistant = NULL;
    }
}

static void
visible_cb (BobguiWidget *check, 
	    gpointer   data)
{
  BobguiWidget *page = data;
  gboolean visible;

  visible = bobgui_check_button_get_active (BOBGUI_CHECK_BUTTON (check));

  g_object_set (G_OBJECT (page), "visible", visible, NULL);
}

static void
create_generous_assistant (BobguiWidget *widget)
{
  static BobguiWidget *assistant = NULL;

  if (!assistant)
    {
      BobguiWidget *page, *next, *check;
      PageData  *pdata;

      assistant = bobgui_assistant_new ();
      bobgui_window_set_default_size (BOBGUI_WINDOW (assistant), 400, 300);

      g_signal_connect (G_OBJECT (assistant), "cancel",
			G_CALLBACK (cancel_callback), NULL);
      g_signal_connect (G_OBJECT (assistant), "close",
			G_CALLBACK (close_callback), NULL);
      g_signal_connect (G_OBJECT (assistant), "apply",
			G_CALLBACK (apply_callback), NULL);
      g_signal_connect (G_OBJECT (assistant), "prepare",
			G_CALLBACK (prepare_callback), NULL);

      page = get_test_page ("Introduction");
      bobgui_assistant_append_page (BOBGUI_ASSISTANT (assistant), page);
      bobgui_assistant_set_page_title (BOBGUI_ASSISTANT (assistant), page, "Introduction");
      bobgui_assistant_set_page_type  (BOBGUI_ASSISTANT (assistant), page, BOBGUI_ASSISTANT_PAGE_INTRO);
      bobgui_assistant_set_page_complete (BOBGUI_ASSISTANT (assistant), page, TRUE);

      page = add_completion_test_page (assistant, "Content", TRUE, FALSE);
      next = add_completion_test_page (assistant, "More Content", TRUE, TRUE);

      check = bobgui_check_button_new_with_label ("Next page visible");
      bobgui_check_button_set_active (BOBGUI_CHECK_BUTTON (check), TRUE);
      g_signal_connect (G_OBJECT (check), "toggled", 
                        G_CALLBACK (visible_cb), next);
      bobgui_box_append (BOBGUI_BOX (page), check);
      
      add_completion_test_page (assistant, "Even More Content", TRUE, TRUE);

      page = get_test_page ("Confirmation");
      bobgui_assistant_append_page (BOBGUI_ASSISTANT (assistant), page);
      bobgui_assistant_set_page_title (BOBGUI_ASSISTANT (assistant), page, "Confirmation");
      bobgui_assistant_set_page_type  (BOBGUI_ASSISTANT (assistant), page, BOBGUI_ASSISTANT_PAGE_CONFIRM);
      bobgui_assistant_set_page_complete (BOBGUI_ASSISTANT (assistant), page, TRUE);

      page = bobgui_progress_bar_new ();
      bobgui_widget_set_halign (page, BOBGUI_ALIGN_FILL);
      bobgui_widget_set_valign (page, BOBGUI_ALIGN_CENTER);
      bobgui_widget_set_margin_start (page, 20);
      bobgui_widget_set_margin_end (page, 20);
      bobgui_assistant_append_page (BOBGUI_ASSISTANT (assistant), page);
      bobgui_assistant_set_page_title (BOBGUI_ASSISTANT (assistant), page, "Progress");
      bobgui_assistant_set_page_type  (BOBGUI_ASSISTANT (assistant), page, BOBGUI_ASSISTANT_PAGE_PROGRESS);

      page = bobgui_check_button_new_with_label ("Summary complete");
      bobgui_assistant_append_page (BOBGUI_ASSISTANT (assistant), page);
      bobgui_assistant_set_page_title (BOBGUI_ASSISTANT (assistant), page, "Summary");
      bobgui_assistant_set_page_type  (BOBGUI_ASSISTANT (assistant), page, BOBGUI_ASSISTANT_PAGE_SUMMARY);

      bobgui_check_button_set_active (BOBGUI_CHECK_BUTTON (page),
                                   bobgui_assistant_get_page_complete (BOBGUI_ASSISTANT (assistant),
                                                                    page));

      pdata = g_new (PageData, 1);
      pdata->assistant = BOBGUI_ASSISTANT (assistant);
      pdata->page = page;
      g_signal_connect (page, "toggled",
                      G_CALLBACK (complete_cb), pdata);
    }

  if (!bobgui_widget_get_visible (assistant))
    bobgui_widget_show (assistant);
  else
    {
      bobgui_window_destroy (BOBGUI_WINDOW (assistant));
      assistant = NULL;
    }
}

static char selected_branch = 'A';

static void
select_branch (BobguiWidget *widget, char branch)
{
  selected_branch = branch;
}

static int
nonlinear_assistant_forward_page (int current_page, gpointer data)
{
  switch (current_page)
    {
    case 0:
      if (selected_branch == 'A')
	return 1;
      else
	return 2;
    case 1:
    case 2:
      return 3;
    default:
      return -1;
    }
}

static void
create_nonlinear_assistant (BobguiWidget *widget)
{
  static BobguiWidget *assistant = NULL;

  if (!assistant)
    {
      BobguiWidget *page, *button, *group;

      assistant = bobgui_assistant_new ();
      bobgui_window_set_default_size (BOBGUI_WINDOW (assistant), 400, 300);

      g_signal_connect (G_OBJECT (assistant), "cancel",
			G_CALLBACK (cancel_callback), NULL);
      g_signal_connect (G_OBJECT (assistant), "close",
			G_CALLBACK (close_callback), NULL);
      g_signal_connect (G_OBJECT (assistant), "apply",
			G_CALLBACK (apply_callback), NULL);
      g_signal_connect (G_OBJECT (assistant), "prepare",
			G_CALLBACK (prepare_callback), NULL);

      bobgui_assistant_set_forward_page_func (BOBGUI_ASSISTANT (assistant),
					   nonlinear_assistant_forward_page,
					   NULL, NULL);

      page = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 6);

      button = bobgui_check_button_new_with_label ("branch A");
      bobgui_box_append (BOBGUI_BOX (page), button);
      g_signal_connect (G_OBJECT (button), "toggled", G_CALLBACK (select_branch), GINT_TO_POINTER ('A'));
      bobgui_check_button_set_active (BOBGUI_CHECK_BUTTON (button), TRUE);
      group = button;

      button = bobgui_check_button_new_with_label ("branch B");
      bobgui_check_button_set_group (BOBGUI_CHECK_BUTTON (button), BOBGUI_CHECK_BUTTON (group));
      bobgui_box_append (BOBGUI_BOX (page), button);
      g_signal_connect (G_OBJECT (button), "toggled", G_CALLBACK (select_branch), GINT_TO_POINTER ('B'));

      bobgui_assistant_append_page (BOBGUI_ASSISTANT (assistant), page);
      bobgui_assistant_set_page_title (BOBGUI_ASSISTANT (assistant), page, "Page 1");
      bobgui_assistant_set_page_complete (BOBGUI_ASSISTANT (assistant), page, TRUE);

      page = get_test_page ("Page 2A");
      bobgui_assistant_append_page (BOBGUI_ASSISTANT (assistant), page);
      bobgui_assistant_set_page_title (BOBGUI_ASSISTANT (assistant), page, "Page 2");
      bobgui_assistant_set_page_complete (BOBGUI_ASSISTANT (assistant), page, TRUE);

      page = get_test_page ("Page 2B");
      bobgui_assistant_append_page (BOBGUI_ASSISTANT (assistant), page);
      bobgui_assistant_set_page_title (BOBGUI_ASSISTANT (assistant), page, "Page 2");
      bobgui_assistant_set_page_complete (BOBGUI_ASSISTANT (assistant), page, TRUE);

      page = get_test_page ("Confirmation");
      bobgui_assistant_append_page (BOBGUI_ASSISTANT (assistant), page);
      bobgui_assistant_set_page_title (BOBGUI_ASSISTANT (assistant), page, "Confirmation");
      bobgui_assistant_set_page_type  (BOBGUI_ASSISTANT (assistant), page, BOBGUI_ASSISTANT_PAGE_CONFIRM);
      bobgui_assistant_set_page_complete (BOBGUI_ASSISTANT (assistant), page, TRUE);
    }

  if (!bobgui_widget_get_visible (assistant))
    bobgui_widget_show (assistant);
  else
    {
      bobgui_window_destroy (BOBGUI_WINDOW (assistant));
      assistant = NULL;
    }
}

static int
looping_assistant_forward_page (int current_page, gpointer data)
{
  switch (current_page)
    {
    case 0:
      return 1;
    case 1:
      return 2;
    case 2:
      return 3;
    case 3:
      {
	BobguiAssistant *assistant;
	BobguiWidget *page;

	assistant = (BobguiAssistant*) data;
	page = bobgui_assistant_get_nth_page (assistant, current_page);

	if (bobgui_check_button_get_active (BOBGUI_CHECK_BUTTON (page)))
	  return 0;
	else
	  return 4;
      }
    case 4:
    default:
      return -1;
    }
}

static void
create_looping_assistant (BobguiWidget *widget)
{
  static BobguiWidget *assistant = NULL;

  if (!assistant)
    {
      BobguiWidget *page;

      assistant = bobgui_assistant_new ();
      bobgui_window_set_default_size (BOBGUI_WINDOW (assistant), 400, 300);

      g_signal_connect (G_OBJECT (assistant), "cancel",
			G_CALLBACK (cancel_callback), NULL);
      g_signal_connect (G_OBJECT (assistant), "close",
			G_CALLBACK (close_callback), NULL);
      g_signal_connect (G_OBJECT (assistant), "apply",
			G_CALLBACK (apply_callback), NULL);
      g_signal_connect (G_OBJECT (assistant), "prepare",
			G_CALLBACK (prepare_callback), NULL);

      bobgui_assistant_set_forward_page_func (BOBGUI_ASSISTANT (assistant),
					   looping_assistant_forward_page,
					   assistant, NULL);

      page = get_test_page ("Introduction");
      bobgui_assistant_append_page (BOBGUI_ASSISTANT (assistant), page);
      bobgui_assistant_set_page_title (BOBGUI_ASSISTANT (assistant), page, "Introduction");
      bobgui_assistant_set_page_type  (BOBGUI_ASSISTANT (assistant), page, BOBGUI_ASSISTANT_PAGE_INTRO);
      bobgui_assistant_set_page_complete (BOBGUI_ASSISTANT (assistant), page, TRUE);

      page = get_test_page ("Content");
      bobgui_assistant_append_page (BOBGUI_ASSISTANT (assistant), page);
      bobgui_assistant_set_page_title (BOBGUI_ASSISTANT (assistant), page, "Content");
      bobgui_assistant_set_page_complete (BOBGUI_ASSISTANT (assistant), page, TRUE);

      page = get_test_page ("More content");
      bobgui_assistant_append_page (BOBGUI_ASSISTANT (assistant), page);
      bobgui_assistant_set_page_title (BOBGUI_ASSISTANT (assistant), page, "More content");
      bobgui_assistant_set_page_complete (BOBGUI_ASSISTANT (assistant), page, TRUE);

      page = bobgui_check_button_new_with_label ("Loop?");
      bobgui_assistant_append_page (BOBGUI_ASSISTANT (assistant), page);
      bobgui_assistant_set_page_title (BOBGUI_ASSISTANT (assistant), page, "Loop?");
      bobgui_assistant_set_page_complete (BOBGUI_ASSISTANT (assistant), page, TRUE);
      
      page = get_test_page ("Confirmation");
      bobgui_assistant_append_page (BOBGUI_ASSISTANT (assistant), page);
      bobgui_assistant_set_page_title (BOBGUI_ASSISTANT (assistant), page, "Confirmation");
      bobgui_assistant_set_page_type  (BOBGUI_ASSISTANT (assistant), page, BOBGUI_ASSISTANT_PAGE_CONFIRM);
      bobgui_assistant_set_page_complete (BOBGUI_ASSISTANT (assistant), page, TRUE);
    }

  if (!bobgui_widget_get_visible (assistant))
    bobgui_widget_show (assistant);
  else
    {
      bobgui_window_destroy (BOBGUI_WINDOW (assistant));
      assistant = NULL;
    }
}

static void
toggle_invisible (BobguiButton *button, BobguiAssistant *assistant)
{
  BobguiWidget *page;

  page = bobgui_assistant_get_nth_page (assistant, 1);

  bobgui_widget_set_visible (page, !bobgui_widget_get_visible (page));
}

static void
create_full_featured_assistant (BobguiWidget *widget)
{
  static BobguiWidget *assistant = NULL;

  if (!assistant)
    {
      BobguiWidget *page, *button;

      assistant = bobgui_assistant_new ();
      bobgui_window_set_default_size (BOBGUI_WINDOW (assistant), 400, 300);

      button = bobgui_button_new_with_label ("_Stop");
      bobgui_button_set_use_underline (BOBGUI_BUTTON (button), TRUE);
      bobgui_assistant_add_action_widget (BOBGUI_ASSISTANT (assistant), button);
      g_signal_connect (button, "clicked",
                        G_CALLBACK (toggle_invisible), assistant);

      g_signal_connect (G_OBJECT (assistant), "cancel",
			G_CALLBACK (cancel_callback), NULL);
      g_signal_connect (G_OBJECT (assistant), "close",
			G_CALLBACK (close_callback), NULL);
      g_signal_connect (G_OBJECT (assistant), "apply",
			G_CALLBACK (apply_callback), NULL);
      g_signal_connect (G_OBJECT (assistant), "prepare",
			G_CALLBACK (prepare_callback), NULL);

      page = get_test_page ("Page 1");
      bobgui_assistant_append_page (BOBGUI_ASSISTANT (assistant), page);
      bobgui_assistant_set_page_title (BOBGUI_ASSISTANT (assistant), page, "Page 1");
      bobgui_assistant_set_page_complete (BOBGUI_ASSISTANT (assistant), page, TRUE);

      page = get_test_page ("Invisible page");
      bobgui_assistant_append_page (BOBGUI_ASSISTANT (assistant), page);
      bobgui_assistant_set_page_title (BOBGUI_ASSISTANT (assistant), page, "Page 2");
      bobgui_assistant_set_page_complete (BOBGUI_ASSISTANT (assistant), page, TRUE);

      page = bobgui_file_chooser_widget_new (BOBGUI_FILE_CHOOSER_ACTION_OPEN);
      bobgui_assistant_append_page (BOBGUI_ASSISTANT (assistant), page);
      bobgui_assistant_set_page_title (BOBGUI_ASSISTANT (assistant), page, "Filechooser");
      bobgui_assistant_set_page_complete (BOBGUI_ASSISTANT (assistant), page, TRUE);

      page = get_test_page ("Page 3");
      bobgui_assistant_append_page (BOBGUI_ASSISTANT (assistant), page);
      bobgui_assistant_set_page_title (BOBGUI_ASSISTANT (assistant), page, "Page 3");
      bobgui_assistant_set_page_type  (BOBGUI_ASSISTANT (assistant), page, BOBGUI_ASSISTANT_PAGE_CONFIRM);
      bobgui_assistant_set_page_complete (BOBGUI_ASSISTANT (assistant), page, TRUE);
    }

  if (!bobgui_widget_get_visible (assistant))
    bobgui_widget_show (assistant);
  else
    {
      bobgui_window_destroy (BOBGUI_WINDOW (assistant));
      assistant = NULL;
    }
}

static void
flip_pages (BobguiButton *button, BobguiAssistant *assistant)
{
  BobguiWidget *page;
  char *title;

  page = bobgui_assistant_get_nth_page (assistant, 1);

  g_object_ref (page);

  title = g_strdup (bobgui_assistant_get_page_title (assistant, page));

  bobgui_assistant_remove_page (assistant, 1);
  bobgui_assistant_insert_page (assistant, page, 2);

  bobgui_assistant_set_page_title (BOBGUI_ASSISTANT (assistant), page, title);
  bobgui_assistant_set_page_complete (BOBGUI_ASSISTANT (assistant), page, TRUE);

  g_object_unref (page);
  g_free (title);
}


static void
create_page_flipping_assistant (BobguiWidget *widget)
{
  static BobguiWidget *assistant = NULL;

  if (!assistant)
    {
      BobguiWidget *page, *button;

      assistant = bobgui_assistant_new ();
      bobgui_window_set_default_size (BOBGUI_WINDOW (assistant), 400, 300);

      button = bobgui_button_new_with_label ("_Flip");
      bobgui_button_set_use_underline (BOBGUI_BUTTON (button), TRUE);
      bobgui_assistant_add_action_widget (BOBGUI_ASSISTANT (assistant), button);
      g_signal_connect (button, "clicked",
                        G_CALLBACK (flip_pages), assistant);

      g_signal_connect (G_OBJECT (assistant), "cancel",
			G_CALLBACK (cancel_callback), NULL);
      g_signal_connect (G_OBJECT (assistant), "close",
			G_CALLBACK (close_callback), NULL);
      g_signal_connect (G_OBJECT (assistant), "apply",
			G_CALLBACK (apply_callback), NULL);
      g_signal_connect (G_OBJECT (assistant), "prepare",
			G_CALLBACK (prepare_callback), NULL);

      page = get_test_page ("Page 1");
      bobgui_assistant_append_page (BOBGUI_ASSISTANT (assistant), page);
      bobgui_assistant_set_page_title (BOBGUI_ASSISTANT (assistant), page, "Page 1");
      bobgui_assistant_set_page_complete (BOBGUI_ASSISTANT (assistant), page, TRUE);

      page = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 0);
      bobgui_box_append (BOBGUI_BOX (page),
                          get_test_page ("Page 2"));
      bobgui_assistant_append_page (BOBGUI_ASSISTANT (assistant), page);
      bobgui_assistant_set_page_title (BOBGUI_ASSISTANT (assistant), page, "Page 2");
      bobgui_assistant_set_page_complete (BOBGUI_ASSISTANT (assistant), page, TRUE);

      page = get_test_page ("Page 3");
      bobgui_assistant_append_page (BOBGUI_ASSISTANT (assistant), page);
      bobgui_assistant_set_page_title (BOBGUI_ASSISTANT (assistant), page, "Page 3");
      bobgui_assistant_set_page_complete (BOBGUI_ASSISTANT (assistant), page, TRUE);

      page = get_test_page ("Summary");
      bobgui_assistant_append_page (BOBGUI_ASSISTANT (assistant), page);
      bobgui_assistant_set_page_title (BOBGUI_ASSISTANT (assistant), page, "Summary");
      bobgui_assistant_set_page_type  (BOBGUI_ASSISTANT (assistant), page, BOBGUI_ASSISTANT_PAGE_SUMMARY);
      bobgui_assistant_set_page_complete (BOBGUI_ASSISTANT (assistant), page, TRUE);
    }

  if (!bobgui_widget_get_visible (assistant))
    bobgui_widget_show (assistant);
  else
    {
      bobgui_window_destroy (BOBGUI_WINDOW (assistant));
      assistant = NULL;
    }
}

struct {
  const char *text;
  void  (*func) (BobguiWidget *widget);
} buttons[] =
  {
    { "simple assistant",        create_simple_assistant },
    { "anonymous assistant",        create_anonymous_assistant },
    { "generous assistant",      create_generous_assistant },
    { "nonlinear assistant",     create_nonlinear_assistant },
    { "looping assistant",       create_looping_assistant },
    { "full featured assistant", create_full_featured_assistant },
    { "page-flipping assistant", create_page_flipping_assistant },
  };

static void
quit_cb (BobguiWidget *widget,
         gpointer   data)
{
  gboolean *done = data;

  *done = TRUE;

  g_main_context_wakeup (NULL);
}

int
main (int argc, char *argv[])
{
  BobguiWidget *window, *box, *button;
  int i;
  gboolean done = FALSE;

  bobgui_init ();

  if (g_getenv ("RTL"))
    bobgui_widget_set_default_direction (BOBGUI_TEXT_DIR_RTL);

  window = bobgui_window_new ();
  bobgui_window_set_hide_on_close (BOBGUI_WINDOW (window), TRUE);

  g_signal_connect (G_OBJECT (window), "destroy", G_CALLBACK (quit_cb), &done);

  box = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 6);
  bobgui_window_set_child (BOBGUI_WINDOW (window), box);

  for (i = 0; i < G_N_ELEMENTS (buttons); i++)
    {
      button = bobgui_button_new_with_label (buttons[i].text);

      if (buttons[i].func)
	g_signal_connect (G_OBJECT (button), "clicked",
			  G_CALLBACK (buttons[i].func), NULL);

      bobgui_box_append (BOBGUI_BOX (box), button);
    }

  bobgui_window_present (BOBGUI_WINDOW (window));

  while (!done)
    g_main_context_iteration (NULL, TRUE);

  return 0;
}
