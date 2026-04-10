/* BOBGUI - The Bobgui Framework
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * BOBGUI Calendar Widget
 * Copyright (C) 1998 Cesar Miquel, Shawn T. Amundson and Mattias Groenlund
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
 * Modified by the BOBGUI Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the BOBGUI Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * BOBGUI at ftp://ftp.bobgui.org/pub/bobgui/.
 */

/**
 * BobguiCalendar:
 *
 * Displays a Gregorian calendar, one month at a time.
 *
 * <picture>
 *   <source srcset="calendar-dark.png" media="(prefers-color-scheme: dark)">
 *   <img alt="An example BobguiCalendar" src="calendar.png">
 * </picture>
 *
 * A `BobguiCalendar` can be created with [ctor@Bobgui.Calendar.new].
 *
 * The selected date can be retrieved from a `BobguiCalendar` using
 * [method@Bobgui.Calendar.get_date].
 * It can be altered with [method@Bobgui.Calendar.set_date].
 *
 * To place a visual marker on a particular day, use
 * [method@Bobgui.Calendar.mark_day] and to remove the marker,
 * [method@Bobgui.Calendar.unmark_day]. Alternative, all
 * marks can be cleared with [method@Bobgui.Calendar.clear_marks].
 *
 * Users should be aware that, although the Gregorian calendar is the
 * legal calendar in most countries, it was adopted progressively
 * between 1582 and 1929. Display before these dates is likely to be
 * historically incorrect.
 *
 * # Shortcuts and Gestures
 *
 * `BobguiCalendar` supports the following gestures:
 *
 * - Scrolling up or down will switch to the previous or next month.
 * - Date strings can be dropped for setting the current day.
 *
 * # CSS nodes
 *
 * ```
 * calendar.view
 * ├── header
 * │   ├── button
 * │   ├── stack.month
 * │   ├── button
 * │   ├── button
 * │   ├── label.year
 * │   ╰── button
 * ╰── grid
 *     ╰── label[.day-name][.week-number][.day-number][.other-month][.today]
 * ```
 *
 * `BobguiCalendar` has a main node with name calendar. It contains a subnode
 * called header containing the widgets for switching between years and months.
 *
 * The grid subnode contains all day labels, including week numbers on the left
 * (marked with the .week-number css class) and day names on top (marked with the
 * .day-name css class).
 *
 * Day labels that belong to the previous or next month get the .other-month
 * style class. The label of the current day get the .today style class.
 *
 * Marked day labels get the :selected state assigned.
 */

#include "config.h"

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#ifdef HAVE__NL_TIME_FIRST_WEEKDAY
#include <langinfo.h>
#endif
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include <glib.h>

#ifdef G_OS_WIN32
#include <windows.h>
#endif

#include "bobguicalendar.h"
#include "bobguidroptarget.h"
#include <glib/gi18n-lib.h>
#include "bobguimain.h"
#include "bobguimarshalers.h"
#include "bobguitooltip.h"
#include "bobguiprivate.h"
#include "bobguirendericonprivate.h"
#include "bobguisnapshot.h"
#include "bobguiwidgetprivate.h"
#include "bobguigestureclick.h"
#include "bobguigesturedrag.h"
#include "bobguieventcontrollerscroll.h"
#include "bobguieventcontrollerkey.h"
#include "bobguieventcontrollerfocus.h"
#include "bobguidragsource.h"
#include "bobguinative.h"
#include "bobguidragicon.h"
#include "bobguibutton.h"
#include "bobguibox.h"
#include "bobguiboxlayout.h"
#include "bobguiorientable.h"
#include "bobguilabel.h"
#include "bobguistack.h"
#include "bobguigrid.h"

/* GDateTime is used, from 0001-01-01 to 9999-12-31 */
static const int YEAR_MIN = 1;
static const int YEAR_MAX = 9999;

static const guint month_length[2][13] =
{
  { 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 },
  { 0, 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 }
};

static gboolean
leap (guint year)
{
  return ((((year % 4) == 0) && ((year % 100) != 0)) || ((year % 400) == 0));
}

static guint
day_of_week (guint year, guint mm, guint dd)
{
  GDateTime *dt;
  guint days;

  dt = g_date_time_new_local (year, mm, dd, 1, 1, 1);
  if (dt == NULL)
    return 0;

  days = g_date_time_get_day_of_week (dt);
  g_date_time_unref (dt);

  return days;
}

static guint
week_of_year (guint year, guint mm, guint dd)
{
  GDateTime *dt;
  guint week;

  dt = g_date_time_new_local (year, mm, dd, 1, 1, 1);
  if (dt == NULL)
    return 1;

  week = g_date_time_get_week_of_year (dt);
  g_date_time_unref (dt);

  return week;
}

enum {
  MONTH_PREV,
  MONTH_CURRENT,
  MONTH_NEXT
};

enum {
  DAY_SELECTED_SIGNAL,
  PREV_MONTH_SIGNAL,
  NEXT_MONTH_SIGNAL,
  PREV_YEAR_SIGNAL,
  NEXT_YEAR_SIGNAL,
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_DATE,
  PROP_YEAR,
  PROP_MONTH,
  PROP_DAY,
  PROP_SHOW_HEADING,
  PROP_SHOW_DAY_NAMES,
  PROP_SHOW_WEEK_NUMBERS,
};

static guint bobgui_calendar_signals[LAST_SIGNAL] = { 0 };

typedef struct _BobguiCalendarClass   BobguiCalendarClass;
typedef struct _BobguiCalendarPrivate BobguiCalendarPrivate;

struct _BobguiCalendar
{
  BobguiWidget widget;

  guint show_week_numbers : 1;
  guint show_heading      : 1;
  guint show_day_names    : 1;
  guint year_before       : 1;

  BobguiWidget *header_box;
  BobguiWidget *year_label;
  BobguiWidget *month_name_stack;
  BobguiWidget *arrow_widgets[4];

  BobguiWidget *grid;
  BobguiWidget *day_name_labels[7];
  BobguiWidget *week_number_labels[6];
  BobguiWidget *day_number_labels[6][7];

  GDateTime *date;

  int   day_month[6][7];
  int   day[6][7];

  int   num_marked_dates;
  int   marked_date[31];

  int   focus_row;
  int   focus_col;

  int week_start;
};

struct _BobguiCalendarClass
{
  BobguiWidgetClass parent_class;

  void (* day_selected)                 (BobguiCalendar *calendar);
  void (* prev_month)                   (BobguiCalendar *calendar);
  void (* next_month)                   (BobguiCalendar *calendar);
  void (* prev_year)                    (BobguiCalendar *calendar);
  void (* next_year)                    (BobguiCalendar *calendar);
};

static void bobgui_calendar_set_property (GObject      *object,
                                       guint         prop_id,
                                       const GValue *value,
                                       GParamSpec   *pspec);
static void bobgui_calendar_get_property (GObject      *object,
                                       guint         prop_id,
                                       GValue       *value,
                                       GParamSpec   *pspec);

static void     bobgui_calendar_button_press   (BobguiGestureClick *gesture,
                                             int              n_press,
                                             double           x,
                                             double           y,
                                             gpointer         user_data);
static gboolean bobgui_calendar_key_controller_key_pressed (BobguiEventControllerKey *controller,
                                                         guint                  keyval,
                                                         guint                  keycode,
                                                         GdkModifierType        state,
                                                         BobguiWidget             *widget);
static void     bobgui_calendar_focus_controller_focus     (BobguiEventController    *controller,
                                                         BobguiWidget             *widget);

static void calendar_select_day_internal (BobguiCalendar *calendar,
                                          GDateTime   *date,
                                          gboolean     emit_day_signal);

static void calendar_invalidate_day     (BobguiCalendar *widget,
                                         int        row,
                                         int        col);
static void calendar_invalidate_day_num (BobguiCalendar *widget,
                                         int        day);

static gboolean bobgui_calendar_scroll_controller_scroll (BobguiEventControllerScroll *scroll,
                                                       double                    dx,
                                                       double                    dy,
                                                       BobguiWidget                *widget);

static void     calendar_set_month_prev (BobguiCalendar *calendar);
static void     calendar_set_month_next (BobguiCalendar *calendar);
static void     calendar_set_year_prev  (BobguiCalendar *calendar);
static void     calendar_set_year_next  (BobguiCalendar *calendar);


static char    *default_abbreviated_dayname[7];
static char    *default_monthname[12];

G_DEFINE_TYPE (BobguiCalendar, bobgui_calendar, BOBGUI_TYPE_WIDGET)

static void
bobgui_calendar_drag_notify_value (BobguiDropTarget  *target,
                                GParamSpec    **pspec,
                                BobguiCalendar    *calendar)
{
  GDate *date;
  const GValue *value;

  value = bobgui_drop_target_get_value (target);
  if (value == NULL)
    return;

  date = g_date_new ();
  g_date_set_parse (date, g_value_get_string (value));
  if (!g_date_valid (date))
    bobgui_drop_target_reject (target);
  g_date_free (date);
}

static gboolean
bobgui_calendar_drag_drop (BobguiDropTarget  *dest,
                        const GValue   *value,
                        double          x,
                        double          y,
                        BobguiCalendar    *calendar)
{
  GDate *date;
  GDateTime *datetime;

  date = g_date_new ();
  g_date_set_parse (date, g_value_get_string (value));

  if (!g_date_valid (date))
    {
      g_warning ("Received invalid date data");
      g_date_free (date);
      return FALSE;
    }

  datetime = g_date_time_new_local (g_date_get_year (date),
                                    g_date_get_month (date),
                                    g_date_get_day (date),
                                    0, 0, 0);
  g_date_free (date);

  calendar_select_day_internal (calendar, datetime, TRUE);
  g_date_time_unref (datetime);

  return TRUE;
}

static void
bobgui_calendar_dispose (GObject *object)
{
  BobguiCalendar *calendar = BOBGUI_CALENDAR (object);

  g_clear_pointer (&calendar->date, g_date_time_unref);
  g_clear_pointer (&calendar->header_box, bobgui_widget_unparent);
  g_clear_pointer (&calendar->grid, bobgui_widget_unparent);

  G_OBJECT_CLASS (bobgui_calendar_parent_class)->dispose (object);
}

static void
bobgui_calendar_class_init (BobguiCalendarClass *class)
{
  GObjectClass   *gobject_class;
  BobguiWidgetClass *widget_class;

  gobject_class = (GObjectClass*)  class;
  widget_class = (BobguiWidgetClass*) class;

  gobject_class->dispose = bobgui_calendar_dispose;
  gobject_class->set_property = bobgui_calendar_set_property;
  gobject_class->get_property = bobgui_calendar_get_property;

  /**
   * BobguiCalendar:date:
   *
   * The selected date.
   *
   * This property gets initially set to the current date.
   */
  g_object_class_install_property (gobject_class,
                                   PROP_DATE,
                                   g_param_spec_boxed ("date", NULL, NULL,
                                                       G_TYPE_DATE_TIME,
                                                       G_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

  /**
   * BobguiCalendar:year:
   *
   * The selected year.
   *
   * This property gets initially set to the current year.
   *
   * Deprecated: 4.20: This property will be removed in BOBGUI 5.
   *   Use [property@Calendar:date] instead.
   */
  g_object_class_install_property (gobject_class,
                                   PROP_YEAR,
                                   g_param_spec_int ("year", NULL, NULL,
                                                     1, 9999, 1,
                                                     G_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

  /**
   * BobguiCalendar:month:
   *
   * The selected month (as a number between 0 and 11).
   *
   * This property gets initially set to the current month.
   *
   * Deprecated: 4.20: This property will be removed in BOBGUI 5.
   *   Use [property@Calendar:date] instead.
   */
  g_object_class_install_property (gobject_class,
                                   PROP_MONTH,
                                   g_param_spec_int ("month", NULL, NULL,
                                                     0, 11, 0,
                                                     G_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

  /**
   * BobguiCalendar:day:
   *
   * The selected day (as a number between 1 and 31).
   *
   * Deprecated: 4.20: This property will be removed in BOBGUI 5.
   *   Use [property@Calendar:date] instead.
   */
  g_object_class_install_property (gobject_class,
                                   PROP_DAY,
                                   g_param_spec_int ("day", NULL, NULL,
                                                     1, 31, 1,
                                                     G_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

  /**
   * BobguiCalendar:show-heading:
   *
   * Determines whether a heading is displayed.
   */
  g_object_class_install_property (gobject_class,
                                   PROP_SHOW_HEADING,
                                   g_param_spec_boolean ("show-heading", NULL, NULL,
                                                         TRUE,
                                                         BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

  /**
   * BobguiCalendar:show-day-names:
   *
   * Determines whether day names are displayed.
   */
  g_object_class_install_property (gobject_class,
                                   PROP_SHOW_DAY_NAMES,
                                   g_param_spec_boolean ("show-day-names", NULL, NULL,
                                                         TRUE,
                                                         BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));
  /**
   * BobguiCalendar:show-week-numbers:
   *
   * Determines whether week numbers are displayed.
   */
  g_object_class_install_property (gobject_class,
                                   PROP_SHOW_WEEK_NUMBERS,
                                   g_param_spec_boolean ("show-week-numbers", NULL, NULL,
                                                         FALSE,
                                                         BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

  /**
   * BobguiCalendar::day-selected:
   * @calendar: the object which received the signal.
   *
   * Emitted when the user selects a day.
   */
  bobgui_calendar_signals[DAY_SELECTED_SIGNAL] =
    g_signal_new (I_("day-selected"),
                  G_OBJECT_CLASS_TYPE (gobject_class),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (BobguiCalendarClass, day_selected),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 0);

  /**
   * BobguiCalendar::prev-month:
   * @calendar: the object which received the signal.
   *
   * Emitted when the user switches to the previous month.
   */
  bobgui_calendar_signals[PREV_MONTH_SIGNAL] =
    g_signal_new (I_("prev-month"),
                  G_OBJECT_CLASS_TYPE (gobject_class),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (BobguiCalendarClass, prev_month),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 0);

  /**
   * BobguiCalendar::next-month:
   * @calendar: the object which received the signal.
   *
   * Emitted when the user switches to the next month.
   */
  bobgui_calendar_signals[NEXT_MONTH_SIGNAL] =
    g_signal_new (I_("next-month"),
                  G_OBJECT_CLASS_TYPE (gobject_class),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (BobguiCalendarClass, next_month),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 0);

  /**
   * BobguiCalendar::prev-year:
   * @calendar: the object which received the signal.
   *
   * Emitted when user switches to the previous year.
   */
  bobgui_calendar_signals[PREV_YEAR_SIGNAL] =
    g_signal_new (I_("prev-year"),
                  G_OBJECT_CLASS_TYPE (gobject_class),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (BobguiCalendarClass, prev_year),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 0);

  /**
   * BobguiCalendar::next-year:
   * @calendar: the object which received the signal.
   *
   * Emitted when user switches to the next year.
   */
  bobgui_calendar_signals[NEXT_YEAR_SIGNAL] =
    g_signal_new (I_("next-year"),
                  G_OBJECT_CLASS_TYPE (gobject_class),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (BobguiCalendarClass, next_year),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 0);

  bobgui_widget_class_set_layout_manager_type (widget_class, BOBGUI_TYPE_BOX_LAYOUT);
  bobgui_widget_class_set_css_name (widget_class, I_("calendar"));
}

static GdkContentProvider *
bobgui_calendar_drag_prepare (BobguiDragSource *source,
                           double         x,
                           double         y,
                           BobguiCalendar   *calendar)
{
  GDate *date;
  char str[128];

  date = g_date_new_dmy (g_date_time_get_day_of_month (calendar->date),
                         g_date_time_get_month (calendar->date),
                         g_date_time_get_year (calendar->date));
  g_date_strftime (str, 127, "%x", date);
  g_free (date);

  return gdk_content_provider_new_typed (G_TYPE_STRING, str);
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"

static void
bobgui_calendar_init (BobguiCalendar *calendar)
{
  BobguiWidget *widget = BOBGUI_WIDGET (calendar);
  BobguiEventController *controller;
  BobguiGesture *gesture;
  BobguiDragSource *source;
  BobguiDropTarget *target;
  int i;
#ifdef G_OS_WIN32
  wchar_t wbuffer[100];
#else
  static const char *month_format = NULL;
  char buffer[255];
  time_t tmp_time;
#endif
  char *year_before;
#ifdef HAVE__NL_TIME_FIRST_WEEKDAY
  union { unsigned int word; char *string; } langinfo;
  int week_1stday = 0;
  int first_weekday = 1;
  guint week_origin;
#else
  char *week_start;
#endif
  int min_year_width;
  GDateTime *now;

  bobgui_widget_set_focusable (widget, TRUE);

  bobgui_widget_add_css_class (BOBGUI_WIDGET (calendar), "view");

  calendar->header_box = g_object_new (BOBGUI_TYPE_BOX,
                                   "css-name", "header",
                                   NULL);
  calendar->year_label = bobgui_label_new ("");
  bobgui_widget_add_css_class (calendar->year_label, "year");
  calendar->month_name_stack = bobgui_stack_new ();
  bobgui_widget_add_css_class (calendar->month_name_stack, "month");
  calendar->arrow_widgets[0] = bobgui_button_new_from_icon_name ("pan-start-symbolic");
  bobgui_widget_set_tooltip_text (calendar->arrow_widgets[0], _("Previous Month"));
  g_signal_connect_swapped (calendar->arrow_widgets[0], "clicked", G_CALLBACK (calendar_set_month_prev), calendar);
  calendar->arrow_widgets[1] = bobgui_button_new_from_icon_name ("pan-end-symbolic");
  bobgui_widget_set_tooltip_text (calendar->arrow_widgets[1], _("Next Month"));
  g_signal_connect_swapped (calendar->arrow_widgets[1], "clicked", G_CALLBACK (calendar_set_month_next), calendar);
  bobgui_widget_set_hexpand (calendar->arrow_widgets[1], TRUE);
  bobgui_widget_set_halign (calendar->arrow_widgets[1], BOBGUI_ALIGN_START);
  calendar->arrow_widgets[2] = bobgui_button_new_from_icon_name ("pan-start-symbolic");
  bobgui_widget_set_tooltip_text (calendar->arrow_widgets[2], _("Previous Year"));
  g_signal_connect_swapped (calendar->arrow_widgets[2], "clicked", G_CALLBACK (calendar_set_year_prev), calendar);
  calendar->arrow_widgets[3] = bobgui_button_new_from_icon_name ("pan-end-symbolic");
  bobgui_widget_set_tooltip_text (calendar->arrow_widgets[3], _("Next Year"));
  g_signal_connect_swapped (calendar->arrow_widgets[3], "clicked", G_CALLBACK (calendar_set_year_next), calendar);

  bobgui_box_append (BOBGUI_BOX (calendar->header_box), calendar->arrow_widgets[0]);
  bobgui_box_append (BOBGUI_BOX (calendar->header_box), calendar->month_name_stack);
  bobgui_box_append (BOBGUI_BOX (calendar->header_box), calendar->arrow_widgets[1]);
  bobgui_box_append (BOBGUI_BOX (calendar->header_box), calendar->arrow_widgets[2]);
  bobgui_box_append (BOBGUI_BOX (calendar->header_box), calendar->year_label);
  bobgui_box_append (BOBGUI_BOX (calendar->header_box), calendar->arrow_widgets[3]);

  bobgui_widget_set_parent (calendar->header_box, BOBGUI_WIDGET (calendar));

  gesture = bobgui_gesture_click_new ();
  g_signal_connect (gesture, "pressed", G_CALLBACK (bobgui_calendar_button_press), calendar);
  bobgui_widget_add_controller (BOBGUI_WIDGET (calendar), BOBGUI_EVENT_CONTROLLER (gesture));

  source = bobgui_drag_source_new ();
  g_signal_connect (source, "prepare", G_CALLBACK (bobgui_calendar_drag_prepare), calendar);
  bobgui_widget_add_controller (BOBGUI_WIDGET (calendar), BOBGUI_EVENT_CONTROLLER (source));

  controller =
    bobgui_event_controller_scroll_new (BOBGUI_EVENT_CONTROLLER_SCROLL_VERTICAL |
                                     BOBGUI_EVENT_CONTROLLER_SCROLL_DISCRETE);
  g_signal_connect (controller, "scroll",
                    G_CALLBACK (bobgui_calendar_scroll_controller_scroll),
                    calendar);
  bobgui_widget_add_controller (BOBGUI_WIDGET (calendar), controller);

  controller = bobgui_event_controller_key_new ();
  g_signal_connect (controller, "key-pressed",
                    G_CALLBACK (bobgui_calendar_key_controller_key_pressed),
                    calendar);
  bobgui_widget_add_controller (BOBGUI_WIDGET (calendar), controller);
  controller = bobgui_event_controller_focus_new ();
  g_signal_connect (controller, "enter",
                    G_CALLBACK (bobgui_calendar_focus_controller_focus),
                    calendar);
  g_signal_connect (controller, "leave",
                    G_CALLBACK (bobgui_calendar_focus_controller_focus),
                    calendar);
  bobgui_widget_add_controller (BOBGUI_WIDGET (calendar), controller);

#ifdef G_OS_WIN32
  calendar->week_start = 0;
  week_start = NULL;

  if (GetLocaleInfoW (GetThreadLocale (), LOCALE_IFIRSTDAYOFWEEK,
                      wbuffer, G_N_ELEMENTS (wbuffer)))
    week_start = g_utf16_to_utf8 (wbuffer, -1, NULL, NULL, NULL);

  if (week_start != NULL)
    {
      calendar->week_start = (week_start[0] - '0' + 1) % 7;
      g_free(week_start);
    }
#else
#ifdef HAVE__NL_TIME_FIRST_WEEKDAY
  langinfo.string = nl_langinfo (_NL_TIME_FIRST_WEEKDAY);
  first_weekday = langinfo.string[0];
  langinfo.string = nl_langinfo (_NL_TIME_WEEK_1STDAY);
  week_origin = langinfo.word;
  if (week_origin == 19971130) /* Sunday */
    week_1stday = 0;
  else if (week_origin == 19971201) /* Monday */
    week_1stday = 1;
  else
    g_warning ("Unknown value of _NL_TIME_WEEK_1STDAY.");

  calendar->week_start = (week_1stday + first_weekday - 1) % 7;
#else
  /* Translate to calendar:week_start:0 if you want Sunday to be the
   * first day of the week to calendar:week_start:1 if you want Monday
   * to be the first day of the week, and so on.
   */
  week_start = _("calendar:week_start:0");

  if (strncmp (week_start, "calendar:week_start:", 20) == 0)
    calendar->week_start = *(week_start + 20) - '0';
  else
    calendar->week_start = -1;

  if (calendar->week_start < 0 || calendar->week_start > 6)
    {
      g_warning ("Whoever translated calendar:week_start:0 did so wrongly.");
      calendar->week_start = 0;
    }
#endif
#endif

  if (!default_abbreviated_dayname[0])
    for (i=0; i<7; i++)
      {
#ifndef G_OS_WIN32
        tmp_time= (i+3)*86400; /* epoch was a Thursday, so add 3 days for Sunday */
        strftime (buffer, sizeof (buffer), "%a", gmtime (&tmp_time));
        default_abbreviated_dayname[i] = g_locale_to_utf8 (buffer, -1, NULL, NULL, NULL);
#else
        if (!GetLocaleInfoW (GetThreadLocale (), LOCALE_SABBREVDAYNAME1 + (i+6)%7,
                             wbuffer, G_N_ELEMENTS (wbuffer)))
          default_abbreviated_dayname[i] = g_strdup_printf ("(%d)", i);
        else
          default_abbreviated_dayname[i] = g_utf16_to_utf8 (wbuffer, -1, NULL, NULL, NULL);
#endif
      }

  if (!default_monthname[0])
    for (i=0; i<12; i++)
      {
#ifndef G_OS_WIN32
        tmp_time=i*2764800;
        if (G_UNLIKELY (month_format == NULL))
          {
            buffer[0] = '\0';
            month_format = "%OB";
            strftime (buffer, sizeof (buffer), month_format, gmtime (&tmp_time));
            /* "%OB" is not supported in Linux with glibc < 2.27  */
            if (!strcmp (buffer, "%OB") || !strcmp (buffer, "OB") || !strcmp (buffer, ""))
              {
                month_format = "%B";
                strftime (buffer, sizeof (buffer), month_format, gmtime (&tmp_time));
              }
          }
        else
          strftime (buffer, sizeof (buffer), month_format, gmtime (&tmp_time));

        default_monthname[i] = g_locale_to_utf8 (buffer, -1, NULL, NULL, NULL);
#else
        if (!GetLocaleInfoW (GetThreadLocale (), LOCALE_SMONTHNAME1 + i,
                             wbuffer, G_N_ELEMENTS (wbuffer)))
          default_monthname[i] = g_strdup_printf ("(%d)", i);
        else
          default_monthname[i] = g_utf16_to_utf8 (wbuffer, -1, NULL, NULL, NULL);
#endif
      }

  for (i = 0; i < 12; i ++)
    {
      BobguiWidget *month_label = bobgui_label_new (default_monthname[i]);

      bobgui_stack_add_named (BOBGUI_STACK (calendar->month_name_stack), month_label, default_monthname[i]);
    }

  calendar->grid = bobgui_grid_new ();
  bobgui_grid_set_row_homogeneous (BOBGUI_GRID (calendar->grid), TRUE);
  bobgui_grid_set_column_homogeneous (BOBGUI_GRID (calendar->grid), TRUE);
  /* Day name labels */
  for (i = 0; i < 7; i ++)
    {
      int day;
      BobguiWidget *label;

      day = (i + calendar->week_start) % 7;
      label = bobgui_label_new (default_abbreviated_dayname[day]);

      bobgui_widget_set_hexpand (label, TRUE);
      bobgui_widget_set_vexpand (label, TRUE);
      bobgui_widget_add_css_class (label, "day-name");
      bobgui_grid_attach (BOBGUI_GRID (calendar->grid), label, 1 + i, 0, 1, 1);

      calendar->day_name_labels[i] = label;
    }

  /* Week number labels */
  for (i = 0; i < 6; i ++)
    {
      BobguiWidget *label = bobgui_label_new ("");

      bobgui_widget_set_hexpand (label, TRUE);
      bobgui_widget_set_vexpand (label, TRUE);
      bobgui_widget_add_css_class (label, "week-number");
      bobgui_grid_attach (BOBGUI_GRID (calendar->grid), label, 0, 1 + i, 1, 1);

      calendar->week_number_labels[i] = label;
      bobgui_widget_set_visible (label, FALSE);
    }

  {
    int x, y;

    for (y = 0; y < 6; y ++)
      for (x = 0; x < 7; x ++)
        {
          BobguiWidget *label = bobgui_label_new ("");

          bobgui_widget_set_hexpand (label, TRUE);
          bobgui_widget_set_vexpand (label, TRUE);
          bobgui_widget_add_css_class (label, "day-number");
          bobgui_grid_attach (BOBGUI_GRID (calendar->grid), label, 1 + x, 1 + y, 1, 1);

          calendar->day_number_labels[y][x] = label;
        }
  }

  bobgui_widget_set_hexpand (calendar->grid, TRUE);
  bobgui_widget_set_vexpand (calendar->grid, TRUE);
  bobgui_widget_set_parent (calendar->grid, BOBGUI_WIDGET (calendar));

  for (i=0;i<31;i++)
    calendar->marked_date[i] = FALSE;
  calendar->num_marked_dates = 0;

  calendar->show_heading = TRUE;
  calendar->show_day_names = TRUE;

  calendar->focus_row = -1;
  calendar->focus_col = -1;

  target = bobgui_drop_target_new (G_TYPE_STRING, GDK_ACTION_COPY);
  bobgui_drop_target_set_preload (target, TRUE);
  g_signal_connect (target, "notify::value", G_CALLBACK (bobgui_calendar_drag_notify_value), calendar);
  g_signal_connect (target, "drop", G_CALLBACK (bobgui_calendar_drag_drop), calendar);
  bobgui_widget_add_controller (widget, BOBGUI_EVENT_CONTROLLER (target));

  calendar->year_before = 0;

  /* Translate to calendar:YM if you want years to be displayed
   * before months; otherwise translate to calendar:MY.
   * Do *not* translate it to anything else, if it
   * it isn't calendar:YM or calendar:MY it will not work.
   *
   * Note that the ordering described here is logical order, which is
   * further influenced by BIDI ordering. Thus, if you have a default
   * text direction of RTL and specify "calendar:YM", then the year
   * will appear to the right of the month.
   */
  year_before = _("calendar:MY");
  if (strcmp (year_before, "calendar:YM") == 0)
    calendar->year_before = 1;
  else if (strcmp (year_before, "calendar:MY") != 0)
    g_warning ("Whoever translated calendar:MY did so wrongly.");

  bobgui_orientable_set_orientation (BOBGUI_ORIENTABLE (bobgui_widget_get_layout_manager (BOBGUI_WIDGET (calendar))),
                                  BOBGUI_ORIENTATION_VERTICAL);

  /* Select current day */
  calendar->date = g_date_time_new_from_unix_local (0);
  now = g_date_time_new_now_local ();
  calendar_select_day_internal (calendar, now, FALSE);
  g_date_time_unref (now);

  /* We just initialized the year label, now add some space to it so
   * changing the year does not increase the calendar width */
  bobgui_widget_measure (calendar->year_label, BOBGUI_ORIENTATION_HORIZONTAL, -1,
                      &min_year_width, NULL, NULL, NULL);
  bobgui_widget_set_size_request (calendar->year_label, min_year_width + 10, -1);
}

#pragma GCC diagnostic pop

static void
calendar_queue_refresh (BobguiCalendar *calendar)
{
  bobgui_widget_queue_resize (BOBGUI_WIDGET (calendar));
}

static void
calendar_set_month_prev (BobguiCalendar *calendar)
{
  GDateTime *new_date;

  new_date = g_date_time_add_months (calendar->date, -1);

  calendar_select_day_internal (calendar, new_date, FALSE);
  g_date_time_unref (new_date);

  g_signal_emit (calendar, bobgui_calendar_signals[PREV_MONTH_SIGNAL], 0);
}

static void
calendar_set_month_next (BobguiCalendar *calendar)
{
  GDateTime *new_date;

  new_date = g_date_time_add_months (calendar->date, 1);

  calendar_select_day_internal (calendar, new_date, FALSE);
  g_date_time_unref (new_date);

  g_signal_emit (calendar, bobgui_calendar_signals[NEXT_MONTH_SIGNAL], 0);
}

static void
calendar_set_year_prev (BobguiCalendar *calendar)
{
  GDateTime *new_date;

  new_date = g_date_time_add_years (calendar->date, -1);

  calendar_select_day_internal (calendar, new_date, FALSE);
  g_date_time_unref (new_date);

  g_signal_emit (calendar, bobgui_calendar_signals[PREV_YEAR_SIGNAL], 0);
}

static void
calendar_set_year_next (BobguiCalendar *calendar)
{
  GDateTime *new_date;

  new_date = g_date_time_add_years (calendar->date, 1);

  calendar_select_day_internal (calendar, new_date, FALSE);
  g_date_time_unref (new_date);

  g_signal_emit (calendar, bobgui_calendar_signals[NEXT_YEAR_SIGNAL], 0);
}

static void
calendar_compute_days (BobguiCalendar *calendar)
{
  const int month = g_date_time_get_month (calendar->date);
  const int year = g_date_time_get_year (calendar->date);
  int ndays_in_month;
  int ndays_in_prev_month;
  int first_day;
  int row;
  int col;
  int day;

  ndays_in_month = month_length[leap (year)][month];

  first_day = day_of_week (year, month, 1);
  first_day = (first_day + 7 - calendar->week_start) % 7;
  if (first_day == 0)
    first_day = 7;

  /* Compute days of previous month */
  if (month > 1)
    ndays_in_prev_month = month_length[leap (year)][month - 1];
  else
    ndays_in_prev_month = month_length[leap (year - 1)][12];
  day = ndays_in_prev_month - first_day+ 1;

  for (col = 0; col < first_day; col++)
    {
      calendar->day[0][col] = day;
      calendar->day_month[0][col] = MONTH_PREV;
      day++;
    }

  /* Compute days of current month */
  row = first_day / 7;
  col = first_day % 7;
  for (day = 1; day <= ndays_in_month; day++)
    {
      calendar->day[row][col] = day;
      calendar->day_month[row][col] = MONTH_CURRENT;

      col++;
      if (col == 7)
        {
          row++;
          col = 0;
        }
    }

  /* Compute days of next month */
  day = 1;
  for (; row <= 5; row++)
    {
      for (; col <= 6; col++)
        {
          calendar->day[row][col] = day;
          calendar->day_month[row][col] = MONTH_NEXT;
          day++;
        }
      col = 0;
    }
}

static void
calendar_update_day_labels (BobguiCalendar *calendar)
{
  char buffer[255];
  GDateTime *today;
  int today_day;
  int selected_year, selected_month, selected_day;
  gboolean lower_limit_reached, upper_limit_reached;

  today_day = -1;
  today = g_date_time_new_now_local ();
  if (g_date_time_get_year (calendar->date) == g_date_time_get_year (today))
    if (g_date_time_get_month (calendar->date) == g_date_time_get_month (today))
      today_day = g_date_time_get_day_of_month (today);
  g_date_time_unref (today);

  g_date_time_get_ymd (calendar->date, &selected_year, &selected_month, &selected_day);

  lower_limit_reached = selected_year == YEAR_MIN && selected_month == G_DATE_JANUARY;
  upper_limit_reached = selected_year == YEAR_MAX && selected_month == G_DATE_DECEMBER;

  for (int row = 0; row < 6; row++)
    for (int col = 0; col < 7; col++)
      {
        const int day = calendar->day[row][col];
        BobguiWidget *label = calendar->day_number_labels[row][col];
        /* Translators: this defines whether the day numbers should use
         * localized digits or the ones used in English (0123...).
         *
         * Translate to "%Id" if you want to use localized digits, or
         * translate to "%d" otherwise.
         *
         * Note that translating this doesn't guarantee that you get localized
         * digits. That needs support from your system and locale definition
         * too.
         */
        g_snprintf (buffer, sizeof (buffer), C_ ("calendar:day:digits", "%d"), day);
        bobgui_label_set_label (BOBGUI_LABEL (label), buffer);
        bobgui_widget_remove_css_class (label, "other-month");
        bobgui_widget_remove_css_class (label, "today");
        bobgui_widget_unset_state_flags (label, BOBGUI_STATE_FLAG_CHECKED);
        bobgui_widget_unset_state_flags (label, BOBGUI_STATE_FLAG_FOCUSED);
        bobgui_widget_unset_state_flags (label, BOBGUI_STATE_FLAG_SELECTED);
        bobgui_widget_set_sensitive (label, TRUE);

        if (calendar->focus_row == row && calendar->focus_col == col)
          bobgui_widget_set_state_flags (label, BOBGUI_STATE_FLAG_FOCUSED, FALSE);

        switch (calendar->day_month[row][col])
          {
          case MONTH_PREV:
            bobgui_widget_add_css_class (label, "other-month");
            if (lower_limit_reached)
              {
                bobgui_label_set_label (BOBGUI_LABEL (label), NULL);
                bobgui_widget_set_sensitive (label, FALSE);
              }
            break;
          case MONTH_CURRENT:
            if (day == selected_day)
              bobgui_widget_set_state_flags (label, BOBGUI_STATE_FLAG_SELECTED, FALSE);
            if (day == today_day)
              bobgui_widget_add_css_class (label, "today");
            if (calendar->marked_date[day - 1])
              bobgui_widget_set_state_flags (label, BOBGUI_STATE_FLAG_CHECKED, FALSE);
            break;
          case MONTH_NEXT:
            bobgui_widget_add_css_class (label, "other-month");
            if (upper_limit_reached)
              {
                bobgui_label_set_label (BOBGUI_LABEL (label), NULL);
                bobgui_widget_set_sensitive (label, FALSE);
              }
            break;
          default:
            g_assert_not_reached ();
            break;
          }
      }
}

static void
calendar_update_navigation_buttons (BobguiCalendar *calendar)
{
  int year, month;
  g_date_time_get_ymd (calendar->date, &year, &month, NULL);
  for (int i = 0; i < 4; i++)
    bobgui_widget_set_sensitive (calendar->arrow_widgets[i], TRUE);
  if (year == YEAR_MIN)
    {
      /* Cannot go back */
      bobgui_widget_set_sensitive (calendar->arrow_widgets[2], FALSE);
      if (month == G_DATE_JANUARY)
        bobgui_widget_set_sensitive (calendar->arrow_widgets[0], FALSE);
    }
  else if (year == YEAR_MAX)
    {
      /* Cannot move forward */
      bobgui_widget_set_sensitive (calendar->arrow_widgets[3], FALSE);
      if (month == G_DATE_DECEMBER)
        bobgui_widget_set_sensitive (calendar->arrow_widgets[1], FALSE);
    }
}

static void
calendar_select_day_internal (BobguiCalendar *calendar,
                              GDateTime   *date,
                              gboolean     emit_day_signal)
{
  int new_month, new_year;
  gboolean day_changed, month_changed, year_changed;
  char buffer[255];
  char *str;
  time_t tmp_time;
  struct tm *tm;
  int i;

  day_changed = g_date_time_get_day_of_month (calendar->date) != g_date_time_get_day_of_month (date);
  month_changed = g_date_time_get_month (calendar->date) != g_date_time_get_month (date);
  year_changed = g_date_time_get_year (calendar->date) != g_date_time_get_year (date);

  if (!day_changed && !month_changed && !year_changed)
    return;

  new_year = g_date_time_get_year (date);
  new_month = g_date_time_get_month (date);

  g_date_time_unref (calendar->date);
  calendar->date = g_date_time_ref (date);

  tmp_time = 1;  /* Jan 1 1970, 00:00:01 UTC */
  tm = gmtime (&tmp_time);
  tm->tm_year = new_year - 1900;

  /* Translators: This dictates how the year is displayed in
   * bobguicalendar widget.  See strftime() manual for the format.
   * Use only ASCII in the translation.
   *
   * "%Y" is appropriate for most locales.
   */
  strftime (buffer, sizeof (buffer), C_("calendar year format", "%Y"), tm);
  str = g_locale_to_utf8 (buffer, -1, NULL, NULL, NULL);
  bobgui_label_set_label (BOBGUI_LABEL (calendar->year_label), str);
  g_free (str);

  /* Update month */

  calendar_compute_days (calendar);
  bobgui_stack_set_visible_child_name (BOBGUI_STACK (calendar->month_name_stack),
                                    default_monthname[new_month - 1]);

  calendar_update_navigation_buttons (calendar);

  calendar_update_day_labels (calendar);

  /* Update week number labels.
   * We simply get the week number of calendar->date and add the others.
   * simple. */
  for (i = 0; i < 6; i ++)
    {
      int year = new_year;
      int month, week;

      month = new_month + calendar->day_month[i][6] - MONTH_CURRENT;

      if (month < 1)
        {
          month += 12;
          year -= 1;
        }
      else if (month > 12)
        {
          month -= 12;
          year += 1;
        }

      week = week_of_year (year, month, calendar->day[i][6]);

      /* Translators: this defines whether the week numbers should use
       * localized digits or the ones used in English (0123...).
       *
       * Translate to "%Id" if you want to use localized digits, or
       * translate to "%d" otherwise.
       * Note that translating this doesn't guarantee that you get localized
       * digits. That needs support from your system and locale definition
       * too. */
      g_snprintf (buffer, sizeof (buffer), C_("calendar:week:digits", "%d"), week);

      bobgui_label_set_label (BOBGUI_LABEL (calendar->week_number_labels[i]), buffer);
    }

  if (day_changed)
    {
      g_object_notify (G_OBJECT (calendar), "day");

      if (emit_day_signal)
        g_signal_emit (calendar, bobgui_calendar_signals[DAY_SELECTED_SIGNAL], 0);
    }

  if (month_changed)
    g_object_notify (G_OBJECT (calendar), "month");

  if (year_changed)
    g_object_notify (G_OBJECT (calendar), "year");

  g_object_notify (G_OBJECT (calendar), "date");
}

static void
calendar_select_and_focus_day (BobguiCalendar *calendar,
                               int          day)
{
  GDateTime *new_date;
  int row;
  int col;

  for (row = 0; row < 6; row ++)
    for (col = 0; col < 7; col++)
      {
        if (calendar->day_month[row][col] == MONTH_CURRENT &&
            calendar->day[row][col] == day)
          {
            calendar->focus_row = row;
            calendar->focus_col = col;
            break;
          }
      }

  new_date = g_date_time_new_local (g_date_time_get_year (calendar->date),
                                    g_date_time_get_month (calendar->date),
                                    day,
                                    0, 0, 0);

  calendar_select_day_internal (calendar, new_date, TRUE);
  g_date_time_unref (new_date);
}

static void
bobgui_calendar_set_property (GObject      *object,
                           guint         prop_id,
                           const GValue *value,
                           GParamSpec   *pspec)
{
  BobguiCalendar *calendar = BOBGUI_CALENDAR (object);

  switch (prop_id)
    {
    case PROP_DATE:
      bobgui_calendar_set_date (calendar, g_value_get_boxed (value));
      break;
    case PROP_YEAR:
      bobgui_calendar_set_year (calendar, g_value_get_int (value));
      break;
    case PROP_MONTH:
      bobgui_calendar_set_month (calendar, g_value_get_int (value));
      break;
    case PROP_DAY:
      bobgui_calendar_set_day (calendar, g_value_get_int (value));
      break;
    case PROP_SHOW_HEADING:
      bobgui_calendar_set_show_heading (calendar, g_value_get_boolean (value));
      break;
    case PROP_SHOW_DAY_NAMES:
      bobgui_calendar_set_show_day_names (calendar, g_value_get_boolean (value));
      break;
    case PROP_SHOW_WEEK_NUMBERS:
      bobgui_calendar_set_show_week_numbers (calendar, g_value_get_boolean (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bobgui_calendar_get_property (GObject      *object,
                           guint         prop_id,
                           GValue       *value,
                           GParamSpec   *pspec)
{
  BobguiCalendar *calendar = BOBGUI_CALENDAR (object);

  switch (prop_id)
    {
    case PROP_DATE:
      g_value_take_boxed (value, bobgui_calendar_get_date (calendar));
      break;
    case PROP_YEAR:
      g_value_set_int (value, bobgui_calendar_get_year (calendar));
      break;
    case PROP_MONTH:
      g_value_set_int (value, bobgui_calendar_get_month (calendar));
      break;
    case PROP_DAY:
      g_value_set_int (value, bobgui_calendar_get_day (calendar));
      break;
    case PROP_SHOW_HEADING:
      g_value_set_boolean (value, bobgui_calendar_get_show_heading (calendar));
      break;
    case PROP_SHOW_DAY_NAMES:
      g_value_set_boolean (value, bobgui_calendar_get_show_day_names (calendar));
      break;
    case PROP_SHOW_WEEK_NUMBERS:
      g_value_set_boolean (value, bobgui_calendar_get_show_week_numbers (calendar));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
calendar_invalidate_day_num (BobguiCalendar *calendar,
                             int          day)
{
  bobgui_widget_queue_draw (BOBGUI_WIDGET (calendar));
}

static void
calendar_invalidate_day (BobguiCalendar *calendar,
                         int          row,
                         int          col)
{
  bobgui_widget_queue_draw (BOBGUI_WIDGET (calendar));
}

static void
bobgui_calendar_button_press (BobguiGestureClick *gesture,
                           int              n_press,
                           double           x,
                           double           y,
                           gpointer         user_data)
{
  BobguiCalendar *calendar = user_data;
  BobguiWidget *widget = BOBGUI_WIDGET (calendar);
  BobguiWidget *label;
  int row = -1, col = -1;
  int ix, iy;
  int day_month;
  int day;

  label = bobgui_widget_pick (widget, x, y, BOBGUI_PICK_DEFAULT);
  for (iy = 0; iy < 6; iy ++)
    for (ix = 0; ix < 7; ix ++)
      {
        if (label == calendar->day_number_labels[iy][ix])
          {
            row = iy;
            col = ix;
          }
      }

  /* If row or column isn't found, just return. */
  if (row == -1 || col == -1)
    return;

  day_month = calendar->day_month[row][col];
  day = calendar->day[row][col];

  if (day_month == MONTH_PREV)
    calendar_set_month_prev (calendar);
  else if (day_month == MONTH_NEXT)
    calendar_set_month_next (calendar);

  if (!bobgui_widget_has_focus (widget))
    bobgui_widget_grab_focus (widget);

  calendar_select_and_focus_day (calendar, day);
}

static gboolean
bobgui_calendar_scroll_controller_scroll (BobguiEventControllerScroll *scroll,
                                       double                    dx,
                                       double                    dy,
                                       BobguiWidget                *widget)
{
  BobguiCalendar *calendar = BOBGUI_CALENDAR (widget);

  if (!bobgui_widget_has_focus (widget))
    bobgui_widget_grab_focus (widget);

  if (dy < 0)
    calendar_set_month_prev (calendar);
  else if (dy > 0)
    calendar_set_month_next (calendar);

  return GDK_EVENT_STOP;
}


/****************************************
 *             Key handling              *
 ****************************************/

static void
move_focus (BobguiCalendar *calendar,
            int          direction,
            int          updown)
{
  BobguiTextDirection text_dir = bobgui_widget_get_direction (BOBGUI_WIDGET (calendar));
  int focus_row, focus_col;
  BobguiWidget *label;

  focus_row = calendar->focus_row;
  focus_col = calendar->focus_col;

  if (updown == 1)
    {
      if (focus_row > 0)
        focus_row--;
      if (focus_row < 0)
        focus_row = 5;
      if (focus_col < 0)
        focus_col = 6;
    }
  else if (updown == -1)
    {
      if (focus_row < 5)
        focus_row++;
      if (focus_col < 0)
        focus_col = 0;
    }
  else if ((text_dir == BOBGUI_TEXT_DIR_LTR && direction == -1) ||
           (text_dir == BOBGUI_TEXT_DIR_RTL && direction == 1))
    {
      if (focus_col > 0)
        focus_col--;
      else if (focus_row > 0)
        {
          focus_col = 6;
          focus_row--;
        }

      if (focus_col < 0)
        focus_col = 6;
      if (focus_row < 0)
        focus_row = 5;
    }
  else
    {
      if (focus_col < 6)
        focus_col++;
      else if (focus_row < 5)
        {
          focus_col = 0;
          focus_row++;
        }

      if (focus_col < 0)
        focus_col = 0;
      if (focus_row < 0)
        focus_row = 0;
    }

  if (!bobgui_widget_is_sensitive (calendar->day_number_labels[focus_row][focus_col]))
    return; /* Not sensitive, not reachable. */

  label = calendar->day_number_labels[calendar->focus_row][calendar->focus_col];
  bobgui_widget_unset_state_flags (label, BOBGUI_STATE_FLAG_FOCUSED);
  calendar_invalidate_day (calendar, calendar->focus_row, calendar->focus_col);

  calendar->focus_row = focus_row;
  calendar->focus_col = focus_col;

  label = calendar->day_number_labels[focus_row][focus_col];
  bobgui_widget_set_state_flags (label, BOBGUI_STATE_FLAG_FOCUSED, FALSE);
  calendar_invalidate_day (calendar, focus_row, focus_col);
}

static gboolean
bobgui_calendar_key_controller_key_pressed (BobguiEventControllerKey *controller,
                                         guint                  keyval,
                                         guint                  keycode,
                                         GdkModifierType        state,
                                         BobguiWidget             *widget)
{
  BobguiCalendar *calendar = BOBGUI_CALENDAR (widget);
  int return_val;
  int row, col, day;
#ifdef __APPLE__
  GdkModifierType modifier = GDK_META_MASK;
#else
  GdkModifierType modifier = GDK_CONTROL_MASK;
#endif

  return_val = FALSE;

  switch (keyval)
    {
    case GDK_KEY_KP_Left:
    case GDK_KEY_Left:
      return_val = TRUE;
      if (state & modifier)
        if (!bobgui_widget_is_sensitive (calendar->arrow_widgets[0]))
          break; /* Not allowed with the mouse, not allowed with the keyboard. */
        else
          calendar_set_month_prev (calendar);
      else
        move_focus (calendar, -1, 0);
      break;
    case GDK_KEY_KP_Right:
    case GDK_KEY_Right:
      return_val = TRUE;
      if (state & modifier)
        if (!bobgui_widget_is_sensitive (calendar->arrow_widgets[1]))
          break; /* Not allowed with the mouse, not allowed with the keyboard. */
        else
          calendar_set_month_next (calendar);
      else
        move_focus (calendar, 1, 0);
      break;
    case GDK_KEY_KP_Up:
    case GDK_KEY_Up:
      return_val = TRUE;
      if (state & modifier)
        if (!bobgui_widget_is_sensitive (calendar->arrow_widgets[2]))
          break; /* Not allowed with the mouse, not allowed with the keyboard. */
        else
          calendar_set_year_prev (calendar);
      else
        move_focus (calendar, 0, 1);
      break;
    case GDK_KEY_KP_Down:
    case GDK_KEY_Down:
      return_val = TRUE;
      if (state & modifier)
        if (!bobgui_widget_is_sensitive (calendar->arrow_widgets[3]))
          break; /* Not allowed with the mouse, not allowed with the keyboard. */
        else
          calendar_set_year_next (calendar);
      else
        move_focus (calendar, 0, -1);
      break;
    case GDK_KEY_KP_Space:
    case GDK_KEY_space:
      row = calendar->focus_row;
      col = calendar->focus_col;

      if (row > -1 && col > -1)
        {
          return_val = TRUE;

          day = calendar->day[row][col];
          if (calendar->day_month[row][col] == MONTH_PREV)
            calendar_set_month_prev (calendar);
          else if (calendar->day_month[row][col] == MONTH_NEXT)
            calendar_set_month_next (calendar);

          calendar_select_and_focus_day (calendar, day);
        }
      break;
    default:
      break;
    }

  return return_val;
}

static void
bobgui_calendar_focus_controller_focus (BobguiEventController     *controller,
                                     BobguiWidget              *widget)
{
  BobguiCalendar *calendar = BOBGUI_CALENDAR (widget);

  calendar_queue_refresh (calendar);
}


/****************************************
 *              Public API              *
 ****************************************/

/**
 * bobgui_calendar_new:
 *
 * Creates a new calendar, with the current date being selected.
 *
 * Returns: a newly `BobguiCalendar` widget
 */
BobguiWidget*
bobgui_calendar_new (void)
{
  return g_object_new (BOBGUI_TYPE_CALENDAR, NULL);
}

/**
 * bobgui_calendar_select_day:
 * @self: a `BobguiCalendar`.
 * @date: (transfer none): a `GDateTime` representing the day to select
 *
 * Switches to @date's year and month and select its day.
 *
 * Deprecated: 4.20: Use [method@Calendar.set_date] instead.
 */
void
bobgui_calendar_select_day (BobguiCalendar *calendar,
                         GDateTime   *date)
{
  g_return_if_fail (BOBGUI_IS_CALENDAR (calendar));
  g_return_if_fail (date != NULL);

  calendar_select_day_internal (calendar, date, TRUE);
}

/**
 * bobgui_calendar_clear_marks:
 * @calendar: a `BobguiCalendar`
 *
 * Remove all visual markers.
 */
void
bobgui_calendar_clear_marks (BobguiCalendar *calendar)
{
  guint day;

  g_return_if_fail (BOBGUI_IS_CALENDAR (calendar));

  for (int y = 0; y < 6; y ++)
    for (int x = 0; x < 7; x ++)
      {
        BobguiWidget *label = calendar->day_number_labels[y][x];

        bobgui_widget_unset_state_flags (label, BOBGUI_STATE_FLAG_CHECKED);
      }

  for (day = 0; day < 31; day++)
    {
      calendar->marked_date[day] = FALSE;
    }

  calendar->num_marked_dates = 0;
  calendar_queue_refresh (calendar);
}

static void
update_mark_state (BobguiCalendar *calendar,
                   guint        day,
                   gboolean     mark)
{
  for (int y = 0; y < 6; y ++)
    for (int x = 0; x < 7; x ++)
      {
        BobguiWidget *label = calendar->day_number_labels[y][x];

        if (day != calendar->day[y][x])
          continue;

        if (mark && calendar->marked_date[day-1] &&
            calendar->day_month[y][x] == MONTH_CURRENT)
          bobgui_widget_set_state_flags (label, BOBGUI_STATE_FLAG_CHECKED, FALSE);
        else
          bobgui_widget_unset_state_flags (label, BOBGUI_STATE_FLAG_CHECKED);
      }
}

/**
 * bobgui_calendar_mark_day:
 * @calendar: a `BobguiCalendar`
 * @day: the day number to mark between 1 and 31.
 *
 * Places a visual marker on a particular day of the current month.
 */
void
bobgui_calendar_mark_day (BobguiCalendar *calendar,
                       guint        day)
{
  g_return_if_fail (BOBGUI_IS_CALENDAR (calendar));

  if (day >= 1 && day <= 31 && !calendar->marked_date[day-1])
    {
      calendar->marked_date[day - 1] = TRUE;
      calendar->num_marked_dates++;
      update_mark_state (calendar, day, TRUE);
      calendar_invalidate_day_num (calendar, day);
    }
}

/**
 * bobgui_calendar_get_day_is_marked:
 * @calendar: a `BobguiCalendar`
 * @day: the day number between 1 and 31.
 *
 * Returns if the @day of the @calendar is already marked.
 *
 * Returns: whether the day is marked.
 */
gboolean
bobgui_calendar_get_day_is_marked (BobguiCalendar *calendar,
                                guint        day)
{
  g_return_val_if_fail (BOBGUI_IS_CALENDAR (calendar), FALSE);

  if (day >= 1 && day <= 31)
    return calendar->marked_date[day - 1];

  return FALSE;
}

/**
 * bobgui_calendar_unmark_day:
 * @calendar: a `BobguiCalendar`.
 * @day: the day number to unmark between 1 and 31.
 *
 * Removes the visual marker from a particular day.
 */
void
bobgui_calendar_unmark_day (BobguiCalendar *calendar,
                         guint        day)
{
  g_return_if_fail (BOBGUI_IS_CALENDAR (calendar));

  if (day >= 1 && day <= 31 && calendar->marked_date[day-1])
    {
      calendar->marked_date[day - 1] = FALSE;
      calendar->num_marked_dates--;
      update_mark_state (calendar, day, FALSE);
      calendar_invalidate_day_num (calendar, day);
    }
}

/**
 * bobgui_calendar_set_date:
 * @self: a `BobguiCalendar`.
 * @date: (transfer none): a `GDateTime` representing the day to select
 *
 * Switches to @date's year and month and selects its day.
 *
 * Since: 4.20
 */
void
bobgui_calendar_set_date (BobguiCalendar *self,
                       GDateTime   *date)
{
  g_return_if_fail (BOBGUI_IS_CALENDAR (self));
  g_return_if_fail (date != NULL);

  calendar_select_day_internal (self, date, TRUE);
}

/**
 * bobgui_calendar_get_date:
 * @self: a `BobguiCalendar`
 *
 * Returns a `GDateTime` representing the shown
 * year, month and the selected day.
 *
 * The returned date is in the local time zone.
 *
 * Returns: (transfer full): the `GDateTime` representing the selected date
 */
GDateTime *
bobgui_calendar_get_date (BobguiCalendar *self)
{
  g_return_val_if_fail (BOBGUI_IS_CALENDAR (self), NULL);

  return g_date_time_ref (self->date);
}

/**
 * bobgui_calendar_set_show_week_numbers:
 * @self: a `BobguiCalendar`
 * @value: whether to show week numbers alongside the days
 *
 * Sets whether week numbers are shown in the calendar.
 */
void
bobgui_calendar_set_show_week_numbers (BobguiCalendar *self,
                                    gboolean     value)
{
  int i;

  g_return_if_fail (BOBGUI_IS_CALENDAR (self));

  if (self->show_week_numbers == value)
    return;

  self->show_week_numbers = value;

  for (i = 0; i < 6; i ++)
    bobgui_widget_set_visible (self->week_number_labels[i], value);

  g_object_notify (G_OBJECT (self), "show-week-numbers");
}

/**
 * bobgui_calendar_get_show_week_numbers:
 * @self: a `BobguiCalendar`
 *
 * Returns whether @self is showing week numbers right
 * now.
 *
 * This is the value of the [property@Bobgui.Calendar:show-week-numbers]
 * property.
 *
 * Return: Whether the calendar is showing week numbers.
 */
gboolean
bobgui_calendar_get_show_week_numbers (BobguiCalendar *self)
{
  g_return_val_if_fail (BOBGUI_IS_CALENDAR (self), FALSE);

  return self->show_week_numbers;
}

/**
 * bobgui_calendar_set_show_heading:
 * @self: a `BobguiCalendar`
 * @value: Whether to show the heading in the calendar
 *
 * Sets whether the calendar should show a heading.
 *
 * The heading contains the current year and month as well as
 * buttons for changing both.
 */
void
bobgui_calendar_set_show_heading (BobguiCalendar *self,
                               gboolean     value)
{
  g_return_if_fail (BOBGUI_IS_CALENDAR (self));

  if (self->show_heading == value)
    return;

  self->show_heading = value;

  bobgui_widget_set_visible (self->header_box, value);

  g_object_notify (G_OBJECT (self), "show-heading");
}

/**
 * bobgui_calendar_get_show_heading:
 * @self: a `BobguiCalendar`
 *
 * Returns whether @self is currently showing the heading.
 *
 * This is the value of the [property@Bobgui.Calendar:show-heading]
 * property.
 *
 * Return: Whether the calendar is showing a heading.
 */
gboolean
bobgui_calendar_get_show_heading (BobguiCalendar *self)
{
  g_return_val_if_fail (BOBGUI_IS_CALENDAR (self), FALSE);

  return self->show_heading;
}

/**
 * bobgui_calendar_set_show_day_names:
 * @self: a `BobguiCalendar`
 * @value: Whether to show day names above the day numbers
 *
 * Sets whether the calendar shows day names.
 */
void
bobgui_calendar_set_show_day_names (BobguiCalendar *self,
                                 gboolean     value)
{
  int i;

  g_return_if_fail (BOBGUI_IS_CALENDAR (self));

  if (self->show_day_names == value)
    return;

  self->show_day_names = value;

  for (i = 0; i < 7; i ++)
    bobgui_widget_set_visible (self->day_name_labels[i], value);

  g_object_notify (G_OBJECT (self), "show-day-names");
}

/**
 * bobgui_calendar_get_show_day_names:
 * @self: a `BobguiCalendar`
 *
 * Returns whether @self is currently showing the names
 * of the week days.
 *
 * This is the value of the [property@Bobgui.Calendar:show-day-names]
 * property.
 *
 * Returns: Whether the calendar shows day names.
 */
gboolean
bobgui_calendar_get_show_day_names (BobguiCalendar *self)
{
  g_return_val_if_fail (BOBGUI_IS_CALENDAR (self), FALSE);

  return self->show_day_names;
}

/**
 * bobgui_calendar_set_day:
 * @self: a `BobguiCalendar`
 * @day: The desired day for the selected date (as a number between 1 and 31).
 *
 * Sets the day for the selected date.
 *
 * The new date must be valid. For example, setting the day to 31 when the
 * month is February will fail.
 *
 * Since: 4.14
 */
void
bobgui_calendar_set_day (BobguiCalendar *self,
                      int          day)
{
  GDateTime *date;

  g_return_if_fail (BOBGUI_IS_CALENDAR (self));
  g_return_if_fail (day >= 1 && day <= 31);

  if (day == g_date_time_get_day_of_month (self->date))
    return;

  date = g_date_time_new_local (g_date_time_get_year (self->date),
                                g_date_time_get_month (self->date),
                                day,
                                0, 0, 0.0);
  g_return_if_fail (date != NULL);

  calendar_select_day_internal (self, date, TRUE);
  g_date_time_unref (date);

  g_object_notify (G_OBJECT (self), "day");
}

/**
 * bobgui_calendar_get_day:
 * @self: a `BobguiCalendar`
 *
 * Gets the day of the selected date.
 *
 * Returns: the day of the selected date.
 *
 * Since: 4.14
 */
int
bobgui_calendar_get_day (BobguiCalendar *self)
{
  g_return_val_if_fail (BOBGUI_IS_CALENDAR (self), -1);

  return g_date_time_get_day_of_month (self->date);
}

/**
 * bobgui_calendar_set_month:
 * @self: a `BobguiCalendar`
 * @month: The desired month for the selected date (as a number between 0 and 11).
 *
 * Sets the month for the selected date.
 *
 * The new date must be valid. For example, setting the month to 1 (February)
 * when the day is 31 will fail.
 *
 * Since: 4.14
 */
void
bobgui_calendar_set_month (BobguiCalendar *self,
                        int          month)
{
  GDateTime *date;

  g_return_if_fail (BOBGUI_IS_CALENDAR (self));
  g_return_if_fail (month >= 0 && month <= 11);

  if (month == g_date_time_get_month (self->date) - 1)
    return;

  date = g_date_time_new_local (g_date_time_get_year (self->date),
                                month + 1,
                                g_date_time_get_day_of_month (self->date),
                                0, 0, 0.0);
  g_return_if_fail (date != NULL);

  calendar_select_day_internal (self, date, TRUE);
  g_date_time_unref (date);

  g_object_notify (G_OBJECT (self), "month");
}

/**
 * bobgui_calendar_get_month:
 * @self: a `BobguiCalendar`
 *
 * Gets the month of the selected date.
 *
 * Returns: The month of the selected date (as a number between 0 and 11).
 *
 * Since: 4.14
 */
int
bobgui_calendar_get_month (BobguiCalendar *self)
{
  g_return_val_if_fail (BOBGUI_IS_CALENDAR (self), -1);

  return g_date_time_get_month (self->date) - 1;
}

/**
 * bobgui_calendar_set_year:
 * @self: a `BobguiCalendar`
 * @year: The desired year for the selected date (within [struct@GLib.DateTime]
 *   limits, i.e. from 0001 to 9999).
 *
 * Sets the year for the selected date.
 *
 * The new date must be valid. For example, setting the year to 2023 when the
 * date is February 29 will fail.
 *
 * Since: 4.14
 */
void
bobgui_calendar_set_year (BobguiCalendar *self,
                       int          year)
{
  GDateTime *date;

  g_return_if_fail (BOBGUI_IS_CALENDAR (self));
  g_return_if_fail (year >= 1 && year <= 9999);

  if (year == g_date_time_get_year (self->date))
    return;

  date = g_date_time_new_local (year,
                                g_date_time_get_month (self->date),
                                g_date_time_get_day_of_month (self->date),
                                0, 0, 0.0);
  g_return_if_fail (date != NULL);

  calendar_select_day_internal (self, date, TRUE);
  g_date_time_unref (date);

  g_object_notify (G_OBJECT (self), "year");
}

/**
 * bobgui_calendar_get_year:
 * @self: a `BobguiCalendar`
 *
 * Gets the year of the selected date.
 *
 * Returns: the year of the selected date.
 *
 * Since: 4.14
 */
int
bobgui_calendar_get_year (BobguiCalendar *self)
{
  g_return_val_if_fail (BOBGUI_IS_CALENDAR (self), -1);

  return g_date_time_get_year (self->date);
}
