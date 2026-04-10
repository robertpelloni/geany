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
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.Free
 */

/*
 * Modified by the BOBGUI+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the BOBGUI+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * BOBGUI+ at ftp://ftp.bobgui.org/pub/bobgui/. 
 */

#include <bobgui/bobgui.h>

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

static void
combo_changed_cb (BobguiWidget *combo,
		  gpointer   data)
{
  BobguiWidget *label = BOBGUI_WIDGET (data);
  int active;

  active = bobgui_combo_box_get_active (BOBGUI_COMBO_BOX (combo));
  bobgui_label_set_ellipsize (BOBGUI_LABEL (label), (PangoEllipsizeMode)active);
}

static void
overlay_draw (BobguiDrawingArea *da,
              cairo_t        *cr,
              int             width,
              int             height,
              gpointer        data)
{
  BobguiWidget *widget = BOBGUI_WIDGET (da);
  PangoLayout *layout;
  const double dashes[] = { 6, 18 };
  BobguiAllocation label_allocation;
  BobguiRequisition minimum_size, natural_size;
  BobguiWidget *label = data;
  double x, y;

  cairo_translate (cr, -0.5, -0.5);
  cairo_set_line_width (cr, 1);

  cairo_set_source_rgb (cr, 1, 1, 1);
  cairo_rectangle (cr, 0, 0, width, height);
  cairo_fill (cr);

  bobgui_widget_translate_coordinates (label, widget, 0, 0, &x, &y);
  layout = bobgui_widget_create_pango_layout (widget, "");

  bobgui_widget_get_preferred_size (label, &minimum_size, &natural_size); 

  pango_layout_set_markup (layout,
    "<span color='#c33'>\342\227\217 requisition</span>\n"
    "<span color='#3c3'>\342\227\217 natural size</span>\n"
    "<span color='#33c'>\342\227\217 allocation</span>", -1);

  pango_cairo_show_layout (cr, layout);
  g_object_unref (layout);

  bobgui_widget_get_allocation (label, &label_allocation);

  cairo_rectangle (cr,
                   x + 0.5 * (label_allocation.width - minimum_size.width),
                   y + 0.5 * (label_allocation.height - minimum_size.height),
                   minimum_size.width, minimum_size.height);
  cairo_set_source_rgb (cr, 0.8, 0.2, 0.2);
  cairo_set_dash (cr, NULL, 0, 0);
  cairo_stroke (cr);

  cairo_rectangle (cr, x, y, label_allocation.width, label_allocation.height);
  cairo_set_source_rgb (cr, 0.2, 0.2, 0.8);
  cairo_set_dash (cr, dashes, 2, 0.5);
  cairo_stroke (cr);

  cairo_rectangle (cr,
                   x + 0.5 * (label_allocation.width - natural_size.width),
                   y + 0.5 * (label_allocation.height - natural_size.height),
                   natural_size.width, natural_size.height);
  cairo_set_source_rgb (cr, 0.2, 0.8, 0.2);
  cairo_set_dash (cr, dashes, 2, 12.5);
  cairo_stroke (cr);
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
main (int argc, char *argv[])
{
  BobguiWidget *window, *vbox, *label;
  BobguiWidget *combo, *scale, *overlay, *da;
  gboolean done = FALSE;

  bobgui_init ();

  window = bobgui_window_new ();
  bobgui_window_set_default_size (BOBGUI_WINDOW (window), 400, 300);
  g_signal_connect (window, "destroy", G_CALLBACK (quit_cb), &done);

  vbox = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 6);
  bobgui_window_set_child (BOBGUI_WINDOW (window), vbox);

  combo = bobgui_combo_box_text_new ();
  scale = bobgui_scale_new_with_range (BOBGUI_ORIENTATION_HORIZONTAL,
                                    0, 360, 1);
  label = bobgui_label_new ("This label may be ellipsized\nto make it fit.");

  bobgui_combo_box_text_append_text (BOBGUI_COMBO_BOX_TEXT (combo), "NONE");
  bobgui_combo_box_text_append_text (BOBGUI_COMBO_BOX_TEXT (combo), "START");
  bobgui_combo_box_text_append_text (BOBGUI_COMBO_BOX_TEXT (combo), "MIDDLE");
  bobgui_combo_box_text_append_text (BOBGUI_COMBO_BOX_TEXT (combo), "END");
  bobgui_combo_box_set_active (BOBGUI_COMBO_BOX (combo), 0);

  bobgui_widget_set_halign (label, BOBGUI_ALIGN_CENTER);
  bobgui_widget_set_valign (label, BOBGUI_ALIGN_CENTER);

  da = bobgui_drawing_area_new ();
  bobgui_drawing_area_set_draw_func (BOBGUI_DRAWING_AREA (da), overlay_draw, label, NULL);

  overlay = bobgui_overlay_new ();
  bobgui_overlay_set_child (BOBGUI_OVERLAY (overlay), da);
  bobgui_widget_set_vexpand (overlay, TRUE);
  bobgui_overlay_add_overlay (BOBGUI_OVERLAY (overlay), label);

  bobgui_box_append (BOBGUI_BOX (vbox), combo);
  bobgui_box_append (BOBGUI_BOX (vbox), scale);
  bobgui_box_append (BOBGUI_BOX (vbox), overlay);

  g_object_set_data (G_OBJECT (label), "combo", combo);

  g_signal_connect (combo, "changed", G_CALLBACK (combo_changed_cb), label);

  bobgui_window_present (BOBGUI_WINDOW (window));

  while (!done)
    g_main_context_iteration (NULL, TRUE);

  return 0;
}
