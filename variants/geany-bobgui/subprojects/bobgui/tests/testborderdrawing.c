
#include<bobgui/bobgui.h>

static const char *css =
".one {"
"  all: unset;"
"  min-width: 100px;"
"  min-height:100px;"
"   border-left:   50px solid #0f0;"
"   border-top:    10px solid red;"
"   border-bottom: 50px solid teal;"
"   border-right:  100px solid pink;"
"   border-radius: 100px;"
"}"
".two {"
"  all: unset;"
"  min-width: 100px;"
"  min-height:100px;"
"   border-left:   50px solid #0f0;"
"   border-top:    10px solid red;"
"   border-bottom: 50px solid teal;"
"   border-right:  100px solid pink;"
"   border-radius: 50%;"
"}"
".three {"
"  all: unset;"
"  min-width: 100px;"
"  min-height:100px;"
"   border-left:   50px solid #0f0;"
"   border-top:    10px solid red;"
"   border-bottom: 50px solid teal;"
"   border-right:  100px solid pink;"
"   border-radius: 0px;"
"}"
".four {"
"  all: unset;"
"  min-width: 100px;"
"  min-height:100px;"
"  border: 10px solid black;"
"  border-radius: 999px;"
"}"
".five {"
"  all: unset;"
"  min-width: 100px;"
"  min-height:100px;"
"  border: 30px solid black;"
"  border-radius: 0px;"
"}"
".b1 {"
"  all: unset;"
"  min-width: 100px;"
"  min-height:100px;"
"  border-top: 30px solid black;"
"  border-radius: 0px;"
"}"
".b2 {"
"  all: unset;"
"  min-width: 100px;"
"  min-height:100px;"
"  border-bottom: 30px solid black;"
"  border-radius: 0px;"
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
  box = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 40);
  top = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 40);
  bottom = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 40);
  bobgui_widget_set_margin_start (box, 40);
  bobgui_widget_set_margin_end (box, 40);
  bobgui_widget_set_margin_top (box, 40);
  bobgui_widget_set_margin_bottom (box, 40);

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

  w = bobgui_button_new ();
  bobgui_widget_set_valign (w, BOBGUI_ALIGN_CENTER);
  bobgui_widget_add_css_class (w, "b2");
  bobgui_box_append (BOBGUI_BOX (bottom), w);

  w = bobgui_button_new ();
  bobgui_widget_set_valign (w, BOBGUI_ALIGN_CENTER);
  bobgui_widget_add_css_class (w, "b3");
  bobgui_box_append (BOBGUI_BOX (bottom), w);

  w = bobgui_button_new ();
  bobgui_widget_set_valign (w, BOBGUI_ALIGN_CENTER);
  bobgui_widget_add_css_class (w, "b4");
  bobgui_box_append (BOBGUI_BOX (bottom), w);

  bobgui_box_append (BOBGUI_BOX (box), top);
  bobgui_box_append (BOBGUI_BOX (box), bottom);
  bobgui_window_set_child (BOBGUI_WINDOW (window), box);
  g_signal_connect (window, "destroy", G_CALLBACK (quit_cb), &done);
  bobgui_window_present (BOBGUI_WINDOW (window));

  while (!done)
    g_main_context_iteration (NULL, TRUE);
}
