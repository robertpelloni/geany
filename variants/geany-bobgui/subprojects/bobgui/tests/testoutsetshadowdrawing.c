
#include<bobgui/bobgui.h>

/*#define COLOR " #0f0;"*/
#define COLOR " red;"

static const char *css =
" window { background-color: white; }\n"
".one {"
"  all: unset;"
"  min-width: 100px;"
"  min-height:100px;"
"  box-shadow: -10px -20px 5px 40px" COLOR
"}"
".two {"
"  all: unset;"
"  min-width: 100px;"
"  min-height:100px;"
"  box-shadow: -10px -20px 0px 40px" COLOR
"}"
".three {"
"  all: unset;"
"  min-width: 100px;"
"  min-height:100px;"
"  border-radius: 0px;"
"  box-shadow: 0px 0px 10px 20px" COLOR
"}"
".four {"
"  all: unset;"
"  min-width: 100px;"
"  min-height: 100px;"
"  box-shadow: 10px 20px 5px 40px" COLOR
"  border-radius: 30px; "
"  margin-right: 50px;"
"}"
".five {"
"  all: unset;"
"  min-width: 100px;"
"  min-height:100px;"
"  border-radius: 30px; "
"  box-shadow: 10px 20px 0px 40px" COLOR
"}"
/* This is the default CSD drop shadow from (current) Adwaita */
".b1 {"
"  all: unset;"
"  min-width: 100px;"
"  min-height: 100px;"
"  border-radius: 7px 7px 0px 0px;"
"  box-shadow: 0px 0px 9px 0px rgba(0, 0, 0, 0.5);"
"}"
#if 0
".b2 {"
"  all: unset;"
"  min-width: 100px;"
"  min-height:100px;"
"  border-radius: 7px 7px 0 0;"
"  box-shadow: 0 0 0 30px green;"
"}"
".b3 {"
"  all: unset;"
"  min-width: 100px;"
"  min-height:100px;"
"  border-right: 30px solid blue;"
"  border-radius: 40px;"
"}"
".b4 {"
"  all: unset;"
"  min-width: 100px;"
"  min-height:100px;"
"  border-bottom: 30px solid blue;"
"  border-radius: 40px;"
"}"
#endif
""
;

static void
quit_cb (BobguiWidget *widget,
         gpointer   data)
{
  gboolean *done = data;

  *done = TRUE;

  g_main_context_wakeup (NULL);
}

int
main (int argc, char **argv)
{
  BobguiWidget *window;
  BobguiWidget *box;
  BobguiWidget *top;
  BobguiWidget *bottom;
  BobguiWidget *w;
  BobguiCssProvider *provider;
  gboolean done = FALSE;

  bobgui_init ();

  provider = bobgui_css_provider_new ();
  bobgui_css_provider_load_from_string (provider, css);
  bobgui_style_context_add_provider_for_display (gdk_display_get_default (),
                                              BOBGUI_STYLE_PROVIDER (provider),
                                              BOBGUI_STYLE_PROVIDER_PRIORITY_APPLICATION);

  window = bobgui_window_new ();
  bobgui_window_set_decorated (BOBGUI_WINDOW (window), FALSE);
  box = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 120);
  top = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 120);
  bottom = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 120);
  bobgui_widget_set_margin_start (box, 120);
  bobgui_widget_set_margin_end (box, 120);
  bobgui_widget_set_margin_top (box, 120);
  bobgui_widget_set_margin_bottom (box, 120);

  w = bobgui_button_new ();
  bobgui_widget_set_valign (w, BOBGUI_ALIGN_CENTER);
  bobgui_widget_add_css_class (w, "one");
  bobgui_box_append (BOBGUI_BOX (top), w);

  w = bobgui_button_new ();
  bobgui_widget_set_valign (w, BOBGUI_ALIGN_CENTER);
  bobgui_widget_add_css_class (w, "two");
  bobgui_box_append (BOBGUI_BOX (top), w);

  w = bobgui_button_new ();
  bobgui_widget_set_valign (w, BOBGUI_ALIGN_CENTER);
  bobgui_widget_add_css_class (w, "three");
  bobgui_widget_set_opacity (w, 0.7);
  bobgui_box_append (BOBGUI_BOX (top), w);

  w = bobgui_button_new ();
  bobgui_widget_set_valign (w, BOBGUI_ALIGN_CENTER);
  bobgui_widget_add_css_class (w, "four");
  bobgui_box_append (BOBGUI_BOX (top), w);

  w = bobgui_button_new ();
  bobgui_widget_set_valign (w, BOBGUI_ALIGN_CENTER);
  bobgui_widget_add_css_class (w, "five");
  bobgui_box_append (BOBGUI_BOX (top), w);

  /* Bottom */
  w = bobgui_button_new ();
  bobgui_widget_set_valign (w, BOBGUI_ALIGN_CENTER);
  bobgui_widget_add_css_class (w, "b1");
  bobgui_box_append (BOBGUI_BOX (bottom), w);

  /*w = bobgui_button_new ();*/
  /*bobgui_widget_set_valign (w, BOBGUI_ALIGN_CENTER);*/
  /*bobgui_widget_add_css_class (w, "b2");*/
  /*bobgui_box_append (BOBGUI_BOX (bottom), w);*/

  /*w = bobgui_button_new ();*/
  /*bobgui_widget_set_valign (w, BOBGUI_ALIGN_CENTER);*/
  /*bobgui_widget_add_css_class (w, "b3");*/
  /*bobgui_box_append (BOBGUI_BOX (bottom), w);*/

  /*w = bobgui_button_new ();*/
  /*bobgui_widget_set_valign (w, BOBGUI_ALIGN_CENTER);*/
  /*bobgui_widget_add_css_class (w, "b4");*/
  /*bobgui_box_append (BOBGUI_BOX (bottom), w);*/

  bobgui_box_append (BOBGUI_BOX (box), top);
  bobgui_box_append (BOBGUI_BOX (box), bottom);
  bobgui_window_set_child (BOBGUI_WINDOW (window), box);
  g_signal_connect (window, "destroy", G_CALLBACK (quit_cb), &done);
  bobgui_window_present (BOBGUI_WINDOW (window));

  while (!done)
    g_main_context_iteration (NULL, TRUE);
}
