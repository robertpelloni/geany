#include <bobgui/bobgui.h>

static BobguiWidget *
create_page (const char *title,
             const char *body)
{
  BobguiWidget *box;
  BobguiWidget *heading;
  BobguiWidget *label;

  box = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 12);
  bobgui_widget_set_margin_top (box, 12);
  bobgui_widget_set_margin_bottom (box, 12);
  bobgui_widget_set_margin_start (box, 12);
  bobgui_widget_set_margin_end (box, 12);

  heading = bobgui_label_new (title);
  bobgui_label_set_xalign (BOBGUI_LABEL (heading), 0.0f);
  bobgui_box_append (BOBGUI_BOX (box), heading);

  label = bobgui_label_new (body);
  bobgui_label_set_wrap (BOBGUI_LABEL (label), TRUE);
  bobgui_label_set_xalign (BOBGUI_LABEL (label), 0.0f);
  bobgui_box_append (BOBGUI_BOX (box), label);

  return box;
}

static void
activate (BobguiApplication *app,
          gpointer           user_data)
{
  BobguiWidget *window;
  BobguiWidget *root;
  BobguiWidget *hero;
  BobguiWidget *switcher;
  BobguiWidget *stack;
  BobguiWidget *activity_scroller;
  BobguiWidget *activity_view;
  BobguiTextBuffer *activity_buffer;

  window = bobgui_application_window_new (app);
  bobgui_window_set_title (BOBGUI_WINDOW (window), "Geany Search Studio (BobGUI Experimental)");
  bobgui_window_set_default_size (BOBGUI_WINDOW (window), 1100, 760);

  root = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 12);
  bobgui_widget_set_margin_top (root, 12);
  bobgui_widget_set_margin_bottom (root, 12);
  bobgui_widget_set_margin_start (root, 12);
  bobgui_widget_set_margin_end (root, 12);

  hero = bobgui_label_new (
    "Geany Search Studio\n\n"
    "Experimental BobGUI-native variant shell with toolkit-exclusive pages for Find, Replace, Find in Files, and Mark. "
    "This folder intentionally uses BobGUI APIs only.");
  bobgui_label_set_wrap (BOBGUI_LABEL (hero), TRUE);
  bobgui_label_set_xalign (BOBGUI_LABEL (hero), 0.0f);
  bobgui_box_append (BOBGUI_BOX (root), hero);

  switcher = bobgui_stack_switcher_new ();
  stack = bobgui_stack_new ();
  bobgui_stack_switcher_set_stack (BOBGUI_STACK_SWITCHER (switcher), BOBGUI_STACK (stack));

  bobgui_stack_add_titled (
    BOBGUI_STACK (stack),
    create_page ("Find", "Toolkit-exclusive BobGUI Find surface placeholder for a future search adapter-backed variant."),
    "find",
    "Find");
  bobgui_stack_add_titled (
    BOBGUI_STACK (stack),
    create_page ("Replace", "Toolkit-exclusive BobGUI Replace surface placeholder for future replace-preview and impact routing."),
    "replace",
    "Replace");
  bobgui_stack_add_titled (
    BOBGUI_STACK (stack),
    create_page ("Find in Files", "Toolkit-exclusive BobGUI directory-search placeholder for future structured hit ingestion."),
    "find-in-files",
    "Find in Files");
  bobgui_stack_add_titled (
    BOBGUI_STACK (stack),
    create_page ("Mark", "Toolkit-exclusive BobGUI mark/bookmark placeholder for future session-aware mark workflows."),
    "mark",
    "Mark");

  bobgui_box_append (BOBGUI_BOX (root), switcher);
  bobgui_box_append (BOBGUI_BOX (root), stack);

  activity_scroller = bobgui_scrolled_window_new ();
  activity_view = bobgui_text_view_new ();
  bobgui_text_view_set_editable (BOBGUI_TEXT_VIEW (activity_view), FALSE);
  activity_buffer = bobgui_text_view_get_buffer (BOBGUI_TEXT_VIEW (activity_view));
  bobgui_text_buffer_set_text (
    activity_buffer,
    "[Studio] BobGUI Search Studio shell initialized.\n"
    "[Status] This BobGUI-only variant currently validates toolkit isolation and variant build wiring first.\n"
    "[Next] Future passes can replace these placeholders with adapter-backed search services.\n",
    -1);
  bobgui_scrolled_window_set_child (BOBGUI_SCROLLED_WINDOW (activity_scroller), activity_view);
  bobgui_box_append (BOBGUI_BOX (root), activity_scroller);

  bobgui_window_set_child (BOBGUI_WINDOW (window), root);
  bobgui_window_present (BOBGUI_WINDOW (window));
}

int
main (int argc,
      char **argv)
{
  BobguiApplication *app;
  int status;

  app = bobgui_application_new ("org.geany.bobgui.searchstudio", G_APPLICATION_DEFAULT_FLAGS);
  g_signal_connect (app, "activate", G_CALLBACK (activate), NULL);
  status = g_application_run (G_APPLICATION (app), argc, argv);
  g_object_unref (app);

  return status;
}
