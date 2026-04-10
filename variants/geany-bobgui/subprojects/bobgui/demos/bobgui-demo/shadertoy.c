/* OpenGL/Shadertoy
 * #Keywords: BobguiGLArea
 *
 * Generate pixels using a custom fragment shader.
 *
 * The names of the uniforms are compatible with the shaders on shadertoy.com, so
 * many of the shaders there work here too.
 */
#include <math.h>
#include <bobgui/bobgui.h>
#include <epoxy/gl.h>
#include "bobguishadertoy.h"

static BobguiWidget *demo_window = NULL;
static BobguiWidget *shadertoy = NULL;
static BobguiTextBuffer *textbuffer = NULL;

static void
run (void)
{
  BobguiTextIter start, end;
  char *text;

  bobgui_text_buffer_get_bounds (textbuffer, &start, &end);
  text = bobgui_text_buffer_get_text (textbuffer, &start, &end, FALSE);

  bobgui_shadertoy_set_image_shader (BOBGUI_SHADERTOY (shadertoy), text);
  g_free (text);
}

static void
run_clicked_cb (BobguiWidget *button,
                gpointer   user_data)
{
  run ();
}

static void
load_clicked_cb (BobguiWidget *button,
                 gpointer   user_data)
{
  const char *path = user_data;
  GBytes *initial_shader;

  initial_shader = g_resources_lookup_data (path, 0, NULL);
  bobgui_text_buffer_set_text (textbuffer, g_bytes_get_data (initial_shader, NULL), -1);
  g_bytes_unref (initial_shader);

  run ();
}

static void
clear_clicked_cb (BobguiWidget *button,
                  gpointer   user_data)
{
  bobgui_text_buffer_set_text (textbuffer, "", 0);
}

static void
close_window (BobguiWidget *widget)
{
  /* Reset the state */
  demo_window = NULL;
  shadertoy = NULL;
  textbuffer = NULL;
}

static BobguiWidget *
new_shadertoy (const char *path)
{
  GBytes *shader;
  BobguiWidget *toy;

  toy = bobgui_shadertoy_new ();
  shader = g_resources_lookup_data (path, 0, NULL);
  bobgui_shadertoy_set_image_shader (BOBGUI_SHADERTOY (toy),
                                  g_bytes_get_data (shader, NULL));
  g_bytes_unref (shader);

  return toy;
}

static BobguiWidget *
new_button (const char *path)
{
  BobguiWidget *button, *toy;

  button = bobgui_button_new ();
  g_signal_connect (button, "clicked", G_CALLBACK (load_clicked_cb), (char *)path);

  toy = new_shadertoy (path);
  bobgui_widget_set_size_request (toy, 64, 36);
  bobgui_button_set_child (BOBGUI_BUTTON (button),  toy);

  return button;
}

static BobguiWidget *
create_shadertoy_window (BobguiWidget *do_widget)
{
  BobguiWidget *window, *box, *hbox, *button, *textview, *sw, *aspect, *centerbox;

  window = bobgui_window_new ();
  bobgui_window_set_display (BOBGUI_WINDOW (window),  bobgui_widget_get_display (do_widget));
  bobgui_window_set_title (BOBGUI_WINDOW (window), "Shadertoy");
  bobgui_window_set_default_size (BOBGUI_WINDOW (window), 690, 740);
  g_signal_connect (window, "destroy", G_CALLBACK (close_window), NULL);

  box = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, FALSE);
  bobgui_widget_set_margin_start (box, 12);
  bobgui_widget_set_margin_end (box, 12);
  bobgui_widget_set_margin_top (box, 12);
  bobgui_widget_set_margin_bottom (box, 12);
  bobgui_box_set_spacing (BOBGUI_BOX (box), 6);
  bobgui_window_set_child (BOBGUI_WINDOW (window), box);

  aspect = bobgui_aspect_frame_new (0.5, 0.5, 1.77777, FALSE);
  bobgui_widget_set_hexpand (aspect, TRUE);
  bobgui_widget_set_vexpand (aspect, TRUE);
  bobgui_box_append (BOBGUI_BOX (box), aspect);

  shadertoy = new_shadertoy ("/shadertoy/alienplanet.glsl");
  bobgui_aspect_frame_set_child (BOBGUI_ASPECT_FRAME (aspect), bobgui_graphics_offload_new (shadertoy));

  sw = bobgui_scrolled_window_new ();
  bobgui_scrolled_window_set_min_content_height (BOBGUI_SCROLLED_WINDOW (sw), 250);
  bobgui_scrolled_window_set_has_frame (BOBGUI_SCROLLED_WINDOW (sw), TRUE);
  bobgui_scrolled_window_set_policy (BOBGUI_SCROLLED_WINDOW (sw),
                                  BOBGUI_POLICY_AUTOMATIC,
                                  BOBGUI_POLICY_AUTOMATIC);
  bobgui_widget_set_hexpand (sw, TRUE);
  bobgui_box_append (BOBGUI_BOX (box), sw);

  textview = bobgui_text_view_new ();
  bobgui_text_view_set_monospace (BOBGUI_TEXT_VIEW (textview), TRUE);
  g_object_set (textview,
                "left-margin", 20,
                "right-margin", 20,
                "top-margin", 20,
                "bottom-margin", 20,
                NULL);
  bobgui_scrolled_window_set_child (BOBGUI_SCROLLED_WINDOW (sw), textview);

  textbuffer = bobgui_text_view_get_buffer (BOBGUI_TEXT_VIEW (textview));
  bobgui_text_buffer_set_text (textbuffer,
                            bobgui_shadertoy_get_image_shader (BOBGUI_SHADERTOY (shadertoy)),
                            -1);

  centerbox = bobgui_center_box_new ();
  bobgui_box_append (BOBGUI_BOX (box), centerbox);

  hbox = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, FALSE);
  bobgui_box_set_spacing (BOBGUI_BOX (hbox), 6);
  bobgui_center_box_set_start_widget (BOBGUI_CENTER_BOX (centerbox), hbox);

  button = bobgui_button_new_from_icon_name ("view-refresh-symbolic");
  bobgui_widget_set_tooltip_text (button, "Restart the demo");
  bobgui_widget_set_valign (button, BOBGUI_ALIGN_CENTER);
  g_signal_connect (button, "clicked", G_CALLBACK (run_clicked_cb), NULL);
  bobgui_box_append (BOBGUI_BOX (hbox), button);

  button = bobgui_button_new_from_icon_name ("edit-clear-all-symbolic");
  bobgui_widget_set_tooltip_text (button, "Clear the text view");
  bobgui_widget_set_valign (button, BOBGUI_ALIGN_CENTER);
  g_signal_connect (button, "clicked", G_CALLBACK (clear_clicked_cb), NULL);
  bobgui_box_append (BOBGUI_BOX (hbox), button);

  hbox = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, FALSE);
  bobgui_box_set_spacing (BOBGUI_BOX (hbox), 6);
  bobgui_center_box_set_end_widget (BOBGUI_CENTER_BOX (centerbox), hbox);

  button = new_button ("/shadertoy/alienplanet.glsl");
  bobgui_box_append (BOBGUI_BOX (hbox), button);

  button = new_button ("/shadertoy/mandelbrot.glsl");
  bobgui_box_append (BOBGUI_BOX (hbox), button);

  button = new_button ("/shadertoy/neon.glsl");
  bobgui_box_append (BOBGUI_BOX (hbox), button);

  button = new_button ("/shadertoy/cogs.glsl");
  bobgui_box_append (BOBGUI_BOX (hbox), button);

  button = new_button ("/shadertoy/glowingstars.glsl");
  bobgui_box_append (BOBGUI_BOX (hbox), button);

  return window;
}

BobguiWidget *
do_shadertoy (BobguiWidget *do_widget)
{
  if (!demo_window)
    demo_window = create_shadertoy_window (do_widget);

  if (!bobgui_widget_get_visible (demo_window))
    bobgui_widget_set_visible (demo_window, TRUE);
  else
    bobgui_window_destroy (BOBGUI_WINDOW (demo_window));

  return demo_window;
}
