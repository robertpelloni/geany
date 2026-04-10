#include <glib-object.h>
#include <glib.h>
#include <bobgui/bobgui.h>

static void
test_calendar_set_get_properties (void)
{
  BobguiWidget *calendar;
  GDateTime *date_in;
  GDateTime *date_out;

  calendar = bobgui_calendar_new ();
  date_in = g_date_time_new_from_iso8601 ("1970-01-01T00:00:00Z", NULL);
  g_object_set (calendar, "date", date_in, NULL);
  g_object_get (calendar, "date", &date_out, NULL);
  g_assert_true (g_date_time_equal (date_in, date_out));
  g_date_time_unref (date_out);
  g_date_time_unref (date_in);
}

static void
test_calendar_set_date (void)
{
  BobguiWidget *calendar;
  GDateTime *date_in;
  GDateTime *date_out;
  calendar = bobgui_calendar_new ();
  date_in = g_date_time_new_from_iso8601 ("2110-11-03T20:20:05Z", NULL);
  bobgui_calendar_set_date (BOBGUI_CALENDAR (calendar), date_in);
  g_object_get (calendar, "date", &date_out, NULL);
  g_assert_true (g_date_time_equal (date_in, date_out));
  g_date_time_unref (date_out);
  g_date_time_unref (date_in);
}

static void
test_calendar_get_date (void)
{
  BobguiWidget *calendar;
  GDateTime *date_in;
  GDateTime *date_out;
  calendar = bobgui_calendar_new ();
  date_in = g_date_time_new_from_iso8601 ("0010-11-25T10:20:30Z", NULL);
  g_object_set (calendar, "date", date_in, NULL);
  date_out = bobgui_calendar_get_date (BOBGUI_CALENDAR (calendar));
  g_assert_true (g_date_time_equal (date_in, date_out));
  g_date_time_unref (date_out);
  g_date_time_unref (date_in);
}

static void
test_calendar_set_get_year (void)
{
  BobguiWidget *calendar;
  int year;

  calendar = bobgui_calendar_new ();
  bobgui_calendar_set_day (BOBGUI_CALENDAR (calendar), 10); /* avoid days that don't exist in all years */

  bobgui_calendar_set_year (BOBGUI_CALENDAR (calendar), 2024);
  year = bobgui_calendar_get_year (BOBGUI_CALENDAR (calendar));
  g_assert_cmpint (year, ==, 2024);
}

static void
test_calendar_set_get_month (void)
{
  BobguiWidget *calendar;
  int month;

  calendar = bobgui_calendar_new ();
  bobgui_calendar_set_day (BOBGUI_CALENDAR (calendar), 10); /* avoid days that don't exist in all months */

  bobgui_calendar_set_month (BOBGUI_CALENDAR (calendar), 1); /* February */
  month = bobgui_calendar_get_month (BOBGUI_CALENDAR (calendar));
  g_assert_cmpint (month, ==, 1);
}

static void
test_calendar_set_get_day (void)
{
  BobguiWidget *calendar;
  int day;

  calendar = bobgui_calendar_new ();
  bobgui_calendar_set_day (BOBGUI_CALENDAR (calendar), 10);

  bobgui_calendar_set_day (BOBGUI_CALENDAR (calendar), 11);
  day = bobgui_calendar_get_day (BOBGUI_CALENDAR (calendar));
  g_assert_cmpint (day, ==, 11);
}

int
main (int argc, char *argv[])
{
  bobgui_init ();
  (g_test_init) (&argc, &argv, NULL);

  g_test_add_func ("/calendar/set_get_properties", test_calendar_set_get_properties);
  g_test_add_func ("/calendar/set_date", test_calendar_set_date);
  g_test_add_func ("/calendar/get_date", test_calendar_get_date);
  g_test_add_func ("/calendar/set_get_day", test_calendar_set_get_day);
  g_test_add_func ("/calendar/set_get_month", test_calendar_set_get_month);
  g_test_add_func ("/calendar/set_get_year", test_calendar_set_get_year);

  return g_test_run ();
}
