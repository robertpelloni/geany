/* testadjustsize.c
 * Copyright (C) 2010 Havoc Pennington
 *
 * Author:
 *      Havoc Pennington <hp@pobox.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#include <bobgui/bobgui.h>

static BobguiWidget *test_window;

enum {
  TEST_WIDGET_LABEL,
  TEST_WIDGET_WRAP_LABEL,
  TEST_WIDGET_IMAGE,
  TEST_WIDGET_BUTTON,
  TEST_WIDGET_LAST
};

static gboolean done = FALSE;
static BobguiWidget *test_widgets[TEST_WIDGET_LAST];

static BobguiWidget*
create_image (void)
{
  return bobgui_image_new_from_icon_name ("document-open");
}

static BobguiWidget*
create_label (gboolean wrap)
{
  BobguiWidget *widget;

  widget = bobgui_label_new ("This is a label, label label label");

  if (wrap)
    bobgui_label_set_wrap (BOBGUI_LABEL (widget), TRUE);

  return widget;
}

static BobguiWidget*
create_button (void)
{
  return bobgui_button_new_with_label ("BUTTON!");
}

static void
quit_cb (BobguiWidget *widget,
         gpointer   data)
{
  gboolean *is_done = data;

  *is_done = TRUE;

  g_main_context_wakeup (NULL);
}

static void
open_test_window (void)
{
  BobguiWidget *grid;
  int i;

  test_window = bobgui_window_new ();
  bobgui_window_set_title (BOBGUI_WINDOW (test_window), "Tests");

  g_signal_connect (test_window, "destroy", G_CALLBACK (quit_cb), &done);

  bobgui_window_set_resizable (BOBGUI_WINDOW (test_window), FALSE);

  test_widgets[TEST_WIDGET_IMAGE] = create_image ();
  test_widgets[TEST_WIDGET_LABEL] = create_label (FALSE);
  test_widgets[TEST_WIDGET_WRAP_LABEL] = create_label (TRUE);
  test_widgets[TEST_WIDGET_BUTTON] = create_button ();

  grid = bobgui_grid_new ();

  bobgui_window_set_child (BOBGUI_WINDOW (test_window), grid);

  for (i = 0; i < TEST_WIDGET_LAST; ++i)
    {
      bobgui_grid_attach (BOBGUI_GRID (grid), test_widgets[i], i % 3, i / 3, 1, 1);
    }

  bobgui_window_present (BOBGUI_WINDOW (test_window));
}

static void
on_set_small_size_requests (BobguiToggleButton *button,
                            void            *data)
{
  gboolean has_small_size_requests;
  int i;

  has_small_size_requests = bobgui_toggle_button_get_active (BOBGUI_TOGGLE_BUTTON (button));

  for (i = 0; i < TEST_WIDGET_LAST; ++i)
    {
      bobgui_widget_set_size_request (test_widgets[i],
                                   has_small_size_requests ? 5 : -1,
                                   has_small_size_requests ? 5 : -1);
    }
}

static void
on_set_large_size_requests (BobguiToggleButton *button,
                            void            *data)
{
  gboolean has_large_size_requests;
  int i;

  has_large_size_requests = bobgui_toggle_button_get_active (BOBGUI_TOGGLE_BUTTON (button));

  for (i = 0; i < TEST_WIDGET_LAST; ++i)
    {
      bobgui_widget_set_size_request (test_widgets[i],
                                   has_large_size_requests ? 200 : -1,
                                   has_large_size_requests ? 200 : -1);
    }
}

static void
open_control_window (void)
{
  BobguiWidget *window;
  BobguiWidget *box;
  BobguiWidget *toggle;

  window = bobgui_window_new ();
  bobgui_window_set_title (BOBGUI_WINDOW (window), "Controls");

  g_signal_connect (window, "destroy", G_CALLBACK (quit_cb), &done);

  box = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 0);
  bobgui_window_set_child (BOBGUI_WINDOW (window), box);

  toggle =
    bobgui_toggle_button_new_with_label ("Set small size requests");
  g_signal_connect (G_OBJECT (toggle),
                    "toggled", G_CALLBACK (on_set_small_size_requests),
                    NULL);
  bobgui_box_append (BOBGUI_BOX (box), toggle);

  toggle =
    bobgui_toggle_button_new_with_label ("Set large size requests");
  g_signal_connect (G_OBJECT (toggle),
                    "toggled", G_CALLBACK (on_set_large_size_requests),
                    NULL);
  bobgui_box_append (BOBGUI_BOX (box), toggle);


  bobgui_window_present (BOBGUI_WINDOW (window));
}

#define TEST_WIDGET(outer) (bobgui_overlay_get_child (BOBGUI_OVERLAY (bobgui_overlay_get_child (BOBGUI_OVERLAY (outer)))))

static BobguiWidget*
create_widget_visible_border (const char *text)
{
  BobguiWidget *outer_box;
  BobguiWidget *inner_box;
  BobguiWidget *test_widget;
  BobguiWidget *label;

  outer_box = bobgui_overlay_new ();
  bobgui_widget_add_css_class (outer_box, "black-bg");

  inner_box = bobgui_overlay_new ();
  bobgui_widget_add_css_class (inner_box, "blue-bg");

  bobgui_overlay_set_child (BOBGUI_OVERLAY (outer_box), inner_box);


  test_widget = bobgui_overlay_new ();
  bobgui_widget_add_css_class (test_widget, "red-bg");

  bobgui_overlay_set_child (BOBGUI_OVERLAY (inner_box), test_widget);

  label = bobgui_label_new (text);
  bobgui_overlay_set_child (BOBGUI_OVERLAY (test_widget), label);

  g_assert (TEST_WIDGET (outer_box) == test_widget);

  return outer_box;
}

static const char*
enum_to_string (GType enum_type,
                int   value)
{
  GEnumValue *v;

  v = g_enum_get_value (g_type_class_peek (enum_type), value);

  return v->value_nick;
}

static BobguiWidget*
create_aligned (BobguiAlign halign,
                BobguiAlign valign)
{
  BobguiWidget *widget;
  char *label;

  label = g_strdup_printf ("h=%s v=%s",
                           enum_to_string (BOBGUI_TYPE_ALIGN, halign),
                           enum_to_string (BOBGUI_TYPE_ALIGN, valign));

  widget = create_widget_visible_border (label);

  g_object_set (G_OBJECT (TEST_WIDGET (widget)),
                "halign", halign,
                "valign", valign,
                "hexpand", TRUE,
                "vexpand", TRUE,
                NULL);

  return widget;
}

static void
open_alignment_window (void)
{
  BobguiWidget *grid;
  int i;
  GEnumClass *align_class;

  test_window = bobgui_window_new ();
  bobgui_window_set_title (BOBGUI_WINDOW (test_window), "Alignment");

  g_signal_connect (test_window, "destroy", G_CALLBACK (quit_cb), &done);

  bobgui_window_set_resizable (BOBGUI_WINDOW (test_window), TRUE);
  bobgui_window_set_default_size (BOBGUI_WINDOW (test_window), 500, 500);

  align_class = g_type_class_peek (BOBGUI_TYPE_ALIGN);

  grid = bobgui_grid_new ();
  bobgui_grid_set_row_homogeneous (BOBGUI_GRID (grid), TRUE);
  bobgui_grid_set_column_homogeneous (BOBGUI_GRID (grid), TRUE);

  bobgui_window_set_child (BOBGUI_WINDOW (test_window), grid);

  for (i = 0; i < align_class->n_values; ++i)
    {
      int j;
      for (j = 0; j < align_class->n_values; ++j)
        {
          BobguiWidget *child =
            create_aligned(align_class->values[i].value,
                           align_class->values[j].value);

          bobgui_grid_attach (BOBGUI_GRID (grid), child, i, j, 1, 1);
        }
    }

  bobgui_window_present (BOBGUI_WINDOW (test_window));
}

static BobguiWidget*
create_margined (const char *propname)
{
  BobguiWidget *widget;

  widget = create_widget_visible_border (propname);

  g_object_set (G_OBJECT (TEST_WIDGET (widget)),
                propname, 15,
                "hexpand", TRUE,
                "vexpand", TRUE,
                NULL);

  return widget;
}

static void
open_margin_window (void)
{
  BobguiWidget *box;
  int i;
  const char *margins[] = {
    "margin-start",
    "margin-end",
    "margin-top",
    "margin-bottom"
  };

  test_window = bobgui_window_new ();
  bobgui_window_set_title (BOBGUI_WINDOW (test_window), "Margin");

  g_signal_connect (test_window, "destroy", G_CALLBACK (quit_cb), &done);

  bobgui_window_set_resizable (BOBGUI_WINDOW (test_window), TRUE);

  box = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 0);

  bobgui_window_set_child (BOBGUI_WINDOW (test_window), box);

  for (i = 0; i < (int) G_N_ELEMENTS (margins); ++i)
    {
      BobguiWidget *child =
        create_margined(margins[i]);

      bobgui_box_append (BOBGUI_BOX (box), child);
    }

  bobgui_window_present (BOBGUI_WINDOW (test_window));
}

static void
open_valigned_label_window (void)
{
  BobguiWidget *window, *box, *label, *frame;

  window = bobgui_window_new ();

  g_signal_connect (test_window, "destroy", G_CALLBACK (quit_cb), &done);

  box = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 0);
  bobgui_window_set_child (BOBGUI_WINDOW (window), box);

  label = bobgui_label_new ("Both labels expand");
  bobgui_box_append (BOBGUI_BOX (box), label);

  label = bobgui_label_new ("Some wrapping text with width-chars = 15 and max-width-chars = 35");
  bobgui_label_set_wrap  (BOBGUI_LABEL (label), TRUE);
  bobgui_label_set_width_chars  (BOBGUI_LABEL (label), 15);
  bobgui_label_set_max_width_chars  (BOBGUI_LABEL (label), 35);

  frame  = bobgui_frame_new (NULL);
  bobgui_frame_set_child (BOBGUI_FRAME (frame), label);

  bobgui_widget_set_valign (frame, BOBGUI_ALIGN_CENTER);
  bobgui_widget_set_halign (frame, BOBGUI_ALIGN_CENTER);

  bobgui_box_append (BOBGUI_BOX (box), frame);

  bobgui_window_present (BOBGUI_WINDOW (window));
}

int
main (int argc, char *argv[])
{
  BobguiCssProvider *provider;

  bobgui_init ();

  provider = bobgui_css_provider_new ();
  bobgui_css_provider_load_from_string (provider,
    ".black-bg { background-color: black; }"
    ".red-bg { background-color: red; }"
    ".blue-bg { background-color: blue; }");
  bobgui_style_context_add_provider_for_display (gdk_display_get_default (),
                                              BOBGUI_STYLE_PROVIDER (provider),
                                              BOBGUI_STYLE_PROVIDER_PRIORITY_APPLICATION);
  g_object_unref (provider);

  if (g_getenv ("RTL"))
    bobgui_widget_set_default_direction (BOBGUI_TEXT_DIR_RTL);

  open_test_window ();
  open_control_window ();
  open_alignment_window ();
  open_margin_window ();
  open_valigned_label_window ();

  while (!done)
    g_main_context_iteration (NULL, TRUE);

  return 0;
}
