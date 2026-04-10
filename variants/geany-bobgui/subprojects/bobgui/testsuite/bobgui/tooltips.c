/* tooltips.c: Unit test for BobguiWidget tooltip accessors
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 * Copyright 2020  GNOME Foundation
 */

#include <bobgui/bobgui.h>

static void
test_tooltips_widget_accessors (void)
{
  BobguiWidget *w;
  const char *text, *markup;

  g_test_message ("A button using tooltip-markup");
  w = bobgui_check_button_new_with_label ("This one uses the tooltip-markup property");
  g_object_ref_sink (w);
  bobgui_widget_set_tooltip_text (w, "Hello, I am a static tooltip.");

  text = bobgui_widget_get_tooltip_text (w);
  markup = bobgui_widget_get_tooltip_markup (w);
  g_assert_cmpstr (text, ==, "Hello, I am a static tooltip.");
  g_assert_cmpstr (markup, ==, "Hello, I am a static tooltip.");
  g_object_unref (w);

  g_test_message ("A label using tooltip-text");
  w = bobgui_label_new ("I am just a label");
  g_object_ref_sink (w);
  bobgui_widget_set_tooltip_text (w, "Label & and tooltip");

  text = bobgui_widget_get_tooltip_text (w);
  markup = bobgui_widget_get_tooltip_markup (w);
  g_assert_cmpstr (text, ==, "Label & and tooltip");
  g_assert_cmpstr (markup, ==, "Label &amp; and tooltip");
  g_object_unref (w);

  g_test_message ("A label using tooltip-markup");
  w = bobgui_label_new ("I am a selectable label");
  g_object_ref_sink (w);
  bobgui_label_set_selectable (BOBGUI_LABEL (w), TRUE);
  bobgui_widget_set_tooltip_markup (w, "<b>Another</b> Label tooltip");

  text = bobgui_widget_get_tooltip_text (w);
  markup = bobgui_widget_get_tooltip_markup (w);
  g_assert_cmpstr (text, ==, "Another Label tooltip");
  g_assert_cmpstr (markup, ==,"<b>Another</b> Label tooltip");
  g_object_unref (w);
}

int
main (int   argc,
      char *argv[])
{
  bobgui_test_init (&argc, &argv);

  g_test_add_func ("/tooltips/widget-accessors", test_tooltips_widget_accessors);

  return g_test_run ();
}
