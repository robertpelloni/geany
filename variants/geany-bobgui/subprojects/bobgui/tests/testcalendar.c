/* example-start calendar calendar.c */
/*
 * Copyright (C) 1998 Cesar Miquel, Shawn T. Amundson, Mattias Grönlund
 * Copyright (C) 2000 Tony Gale
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"
#include <stdio.h>
#include <string.h>
#include <bobgui/bobgui.h>

#define DEF_PAD_SMALL 6

#define TM_YEAR_BASE 1900

typedef struct _CalendarData
{
  BobguiWidget *calendar_widget;
  BobguiWidget *prev2_sig;
  BobguiWidget *prev_sig;
  BobguiWidget *last_sig;
  BobguiWidget *month;
} CalendarData;

enum
{
  calendar_show_header,
  calendar_show_days,
  calendar_month_change, 
  calendar_show_week,
  calendar_monday_first
};

/*
 * BobguiCalendar
 */

static char *
calendar_date_to_string (CalendarData *data,
                         const char   *format)
{
  GDateTime *date;
  char *str;

  date = bobgui_calendar_get_date (BOBGUI_CALENDAR (data->calendar_widget));
  str = g_date_time_format (date, format);

  g_date_time_unref (date);

  return str;
}

static void
calendar_set_signal_strings (char         *sig_str,
				  CalendarData *data)
{
  const char *prev_sig;

  prev_sig = bobgui_label_get_text (BOBGUI_LABEL (data->prev_sig));
  bobgui_label_set_text (BOBGUI_LABEL (data->prev2_sig), prev_sig);

  prev_sig = bobgui_label_get_text (BOBGUI_LABEL (data->last_sig));
  bobgui_label_set_text (BOBGUI_LABEL (data->prev_sig), prev_sig);
  bobgui_label_set_text (BOBGUI_LABEL (data->last_sig), sig_str);
}

static void
calendar_day_selected (BobguiWidget    *widget,
                            CalendarData *data)
{
  char *str = calendar_date_to_string (data, "day-selected: %c");
  calendar_set_signal_strings (str, data);
  g_free (str);
}

static void
calendar_prev_month (BobguiWidget    *widget,
                          CalendarData *data)
{
  char *str = calendar_date_to_string (data, "prev-month: %c");
  calendar_set_signal_strings (str, data);
  g_free (str);
}

static void
calendar_next_month (BobguiWidget    *widget,
                     CalendarData *data)
{
  char *str = calendar_date_to_string (data, "next-month: %c");
  calendar_set_signal_strings (str, data);
  g_free (str);

}

static void
calendar_prev_year (BobguiWidget    *widget,
                    CalendarData *data)
{
  char *str = calendar_date_to_string (data, "prev-year: %c");
  calendar_set_signal_strings (str, data);
  g_free (str);
}

static void
calendar_next_year (BobguiWidget    *widget,
                    CalendarData *data)
{
  char *str = calendar_date_to_string (data, "next-year: %c");
  calendar_set_signal_strings (str, data);
  g_free (str);
}

static void
flag_toggled_cb (BobguiCheckButton *button,
                 gpointer        user_data)
{
  struct {
    const char *prop_name;
    const char *label;
    BobguiWidget *calendar;
  } *data = user_data;

  g_object_set (G_OBJECT (data->calendar), data->prop_name,
                bobgui_check_button_get_active (BOBGUI_CHECK_BUTTON (button)),
                NULL);
}

static BobguiWidget*
create_frame (const char *caption,
              BobguiWidget  *child,
              BobguiAlign    halign,
              BobguiAlign    valign)
{
  BobguiWidget *frame = bobgui_frame_new ("");
  BobguiWidget *label = bobgui_frame_get_label_widget (BOBGUI_FRAME (frame));

  g_object_set (child,
                "margin-top", 6,
                "margin-bottom", 0,
                "margin-start", 18,
                "margin-end", 0,
                "halign", halign,
                "valign", valign,
                NULL);
  bobgui_label_set_markup (BOBGUI_LABEL (label), caption);

  bobgui_frame_set_child (BOBGUI_FRAME (frame), child);

  return frame;
}

static void
quit_cb (BobguiWidget *widget,
         gpointer   data)
{
  gboolean *done = data;

  *done = TRUE;

  g_main_context_wakeup (NULL);
}

static void
create_calendar(void)
{
  static CalendarData calendar_data;

  BobguiWidget *window, *hpaned, *vbox, *rpane, *hbox;
  BobguiWidget *calendar = bobgui_calendar_new ();
  BobguiWidget *button;
  BobguiWidget *frame, *label, *bbox;
  int i;
  struct {
    const char *prop_name;
    const char *label;
    BobguiWidget *calendar;
  } flags[] = {
    { "show-heading", "Show Heading", calendar },
    { "show-day-names", "Show Day Names", calendar },
    { "show-week-numbers", "Show Week Numbers", calendar },
  };
  gboolean done = FALSE;

  window = bobgui_window_new ();
  bobgui_window_set_hide_on_close (BOBGUI_WINDOW (window), TRUE);
  bobgui_window_set_title (BOBGUI_WINDOW (window), "BobguiCalendar Example");
  g_signal_connect (window, "destroy", G_CALLBACK (quit_cb), &done);

  hpaned = bobgui_paned_new (BOBGUI_ORIENTATION_HORIZONTAL);
  bobgui_widget_set_vexpand (hpaned, TRUE);

  /* Calendar widget */

  calendar_data.calendar_widget = calendar;
  frame = create_frame ("<b>Calendar</b>", calendar, BOBGUI_ALIGN_CENTER, BOBGUI_ALIGN_CENTER);
  bobgui_paned_set_start_child (BOBGUI_PANED (hpaned), frame);
  bobgui_paned_set_resize_start_child (BOBGUI_PANED (hpaned), TRUE);
  bobgui_paned_set_shrink_start_child (BOBGUI_PANED (hpaned), FALSE);

  bobgui_calendar_mark_day (BOBGUI_CALENDAR (calendar), 19);	

  g_signal_connect (calendar, "day-selected", 
		    G_CALLBACK (calendar_day_selected),
		    &calendar_data);
  g_signal_connect (calendar, "prev-month", 
		    G_CALLBACK (calendar_prev_month),
		    &calendar_data);
  g_signal_connect (calendar, "next-month", 
		    G_CALLBACK (calendar_next_month),
		    &calendar_data);
  g_signal_connect (calendar, "prev-year", 
		    G_CALLBACK (calendar_prev_year),
		    &calendar_data);
  g_signal_connect (calendar, "next-year", 
		    G_CALLBACK (calendar_next_year),
		    &calendar_data);

  rpane = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, DEF_PAD_SMALL);
  bobgui_paned_set_end_child (BOBGUI_PANED (hpaned), rpane);
  bobgui_paned_set_resize_end_child (BOBGUI_PANED (hpaned), FALSE);
  bobgui_paned_set_shrink_end_child (BOBGUI_PANED (hpaned), FALSE);

  vbox = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, DEF_PAD_SMALL);
  frame = create_frame ("<b>Options</b>", vbox, BOBGUI_ALIGN_FILL, BOBGUI_ALIGN_CENTER);
  bobgui_box_append (BOBGUI_BOX (rpane), frame);

  hbox = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, DEF_PAD_SMALL);
  bobgui_widget_set_halign (hbox, BOBGUI_ALIGN_START);
  bobgui_widget_set_valign (hbox, BOBGUI_ALIGN_CENTER);
  bobgui_box_append (BOBGUI_BOX (vbox), hbox);

  /* Build the Right frame with the flags in */

  vbox = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 0);
  bobgui_box_append (BOBGUI_BOX (rpane), vbox);

  for (i = 0; i < G_N_ELEMENTS (flags); i++)
    {
      BobguiWidget *toggle = bobgui_check_button_new_with_mnemonic (flags[i].label);
      gboolean value;

      bobgui_box_append (BOBGUI_BOX (vbox), toggle);

      g_object_get (G_OBJECT (calendar), flags[i].prop_name, &value, NULL);
      bobgui_check_button_set_active (BOBGUI_CHECK_BUTTON (toggle), value);

      g_signal_connect (toggle, "toggled", G_CALLBACK (flag_toggled_cb), &flags[i]);
    }

  /*
   *  Build the Signal-event part.
   */

  vbox = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, DEF_PAD_SMALL);
  bobgui_box_set_homogeneous (BOBGUI_BOX (vbox), TRUE);
  frame = create_frame ("<b>Signal Events</b>", vbox, BOBGUI_ALIGN_FILL, BOBGUI_ALIGN_CENTER);

  hbox = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 3);
  bobgui_box_append (BOBGUI_BOX (vbox), hbox);
  label = bobgui_label_new ("Signal:");
  bobgui_box_append (BOBGUI_BOX (hbox), label);
  calendar_data.last_sig = bobgui_label_new ("");
  bobgui_box_append (BOBGUI_BOX (hbox), calendar_data.last_sig);

  hbox = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 3);
  bobgui_box_append (BOBGUI_BOX (vbox), hbox);
  label = bobgui_label_new ("Previous signal:");
  bobgui_box_append (BOBGUI_BOX (hbox), label);
  calendar_data.prev_sig = bobgui_label_new ("");
  bobgui_box_append (BOBGUI_BOX (hbox), calendar_data.prev_sig);

  hbox = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 3);
  bobgui_box_append (BOBGUI_BOX (vbox), hbox);
  label = bobgui_label_new ("Second previous signal:");
  bobgui_box_append (BOBGUI_BOX (hbox), label);
  calendar_data.prev2_sig = bobgui_label_new ("");
  bobgui_box_append (BOBGUI_BOX (hbox), calendar_data.prev2_sig);

  /*
   *  Glue everything together
   */

  bbox = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 0);
  bobgui_widget_set_halign (bbox, BOBGUI_ALIGN_END);

  button = bobgui_button_new_with_label ("Close");
  g_signal_connect (button, "clicked", G_CALLBACK (quit_cb), &done);
  bobgui_box_append (BOBGUI_BOX (bbox), button);

  vbox = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, DEF_PAD_SMALL);

  bobgui_box_append (BOBGUI_BOX (vbox), hpaned);
  bobgui_box_append (BOBGUI_BOX (vbox), bobgui_separator_new (BOBGUI_ORIENTATION_HORIZONTAL));
  bobgui_box_append (BOBGUI_BOX (vbox), frame);
  bobgui_box_append (BOBGUI_BOX (vbox), bobgui_separator_new (BOBGUI_ORIENTATION_HORIZONTAL));
  bobgui_box_append (BOBGUI_BOX (vbox), bbox);

  bobgui_window_set_child (BOBGUI_WINDOW (window), vbox);

  bobgui_window_set_default_widget (BOBGUI_WINDOW (window), button);

  bobgui_window_set_default_size (BOBGUI_WINDOW (window), 600, 0);
  g_signal_connect (window, "close-request", G_CALLBACK (quit_cb), &done);
  bobgui_window_present (BOBGUI_WINDOW (window));

  while (!done)
    g_main_context_iteration (NULL, TRUE);
}


int main(int   argc,
         char *argv[] )
{
  bobgui_init ();

  if (g_getenv ("BOBGUI_RTL"))
    bobgui_widget_set_default_direction (BOBGUI_TEXT_DIR_RTL);

  create_calendar();

  return(0);
}
/* example-end */
