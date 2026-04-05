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

static BobguiWidget *
create_log_surface (const char *text)
{
  BobguiWidget *scroller;
  BobguiWidget *view;
  BobguiTextBuffer *buffer;

  scroller = bobgui_scrolled_window_new ();
  view = bobgui_text_view_new ();
  bobgui_text_view_set_editable (BOBGUI_TEXT_VIEW (view), FALSE);
  bobgui_text_view_set_monospace (BOBGUI_TEXT_VIEW (view), TRUE);

  buffer = bobgui_text_view_get_buffer (BOBGUI_TEXT_VIEW (view));
  bobgui_text_buffer_set_text (buffer, text, -1);
  bobgui_scrolled_window_set_child (BOBGUI_SCROLLED_WINDOW (scroller), view);

  return scroller;
}

static void
activate (BobguiApplication *app,
          gpointer           user_data)
{
  BobguiWidget *window;
  BobguiWidget *root;
  BobguiWidget *hero;
  BobguiWidget *paned;
  BobguiWidget *top_shell;
  BobguiWidget *top_switcher;
  BobguiWidget *top_stack;
  BobguiWidget *bottom_shell;
  BobguiWidget *bottom_switcher;
  BobguiWidget *bottom_stack;

  window = bobgui_application_window_new (app);
  bobgui_window_set_title (BOBGUI_WINDOW (window), "Geany Search Studio (BobGUI Experimental)");
  bobgui_window_set_default_size (BOBGUI_WINDOW (window), 1180, 820);

  root = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 12);
  bobgui_widget_set_margin_top (root, 12);
  bobgui_widget_set_margin_bottom (root, 12);
  bobgui_widget_set_margin_start (root, 12);
  bobgui_widget_set_margin_end (root, 12);

  hero = bobgui_label_new (
    "Geany Search Studio\n\n"
    "Experimental BobGUI-native variant shell with toolkit-exclusive pages for Find, Replace, Find in Files, and Mark. "
    "This folder intentionally uses BobGUI APIs only while growing toward a denser Search Studio workbench.");
  bobgui_label_set_wrap (BOBGUI_LABEL (hero), TRUE);
  bobgui_label_set_xalign (BOBGUI_LABEL (hero), 0.0f);
  bobgui_box_append (BOBGUI_BOX (root), hero);

  paned = bobgui_paned_new (BOBGUI_ORIENTATION_VERTICAL);
  bobgui_paned_set_position (BOBGUI_PANED (paned), 470);
  bobgui_paned_set_resize_start_child (BOBGUI_PANED (paned), TRUE);
  bobgui_paned_set_resize_end_child (BOBGUI_PANED (paned), TRUE);

  top_shell = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 12);
  top_switcher = bobgui_stack_switcher_new ();
  top_stack = bobgui_stack_new ();
  bobgui_stack_switcher_set_stack (BOBGUI_STACK_SWITCHER (top_switcher), BOBGUI_STACK (top_stack));

  bobgui_stack_add_titled (
    BOBGUI_STACK (top_stack),
    create_page ("Find", "Toolkit-exclusive BobGUI Find surface placeholder for a future adapter-backed variant with structured request/result flows."),
    "find",
    "Find");
  bobgui_stack_add_titled (
    BOBGUI_STACK (top_stack),
    create_page ("Replace", "Toolkit-exclusive BobGUI Replace surface placeholder for future replace-preview and impact routing."),
    "replace",
    "Replace");
  bobgui_stack_add_titled (
    BOBGUI_STACK (top_stack),
    create_page ("Find in Files", "Toolkit-exclusive BobGUI directory-search placeholder for future structured hit ingestion."),
    "find-in-files",
    "Find in Files");
  bobgui_stack_add_titled (
    BOBGUI_STACK (top_stack),
    create_page ("Mark", "Toolkit-exclusive BobGUI mark/bookmark placeholder for future session-aware mark workflows."),
    "mark",
    "Mark");

  bobgui_box_append (BOBGUI_BOX (top_shell), top_switcher);
  bobgui_box_append (BOBGUI_BOX (top_shell), top_stack);

  bottom_shell = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 12);
  bottom_switcher = bobgui_stack_switcher_new ();
  bottom_stack = bobgui_stack_new ();
  bobgui_stack_switcher_set_stack (BOBGUI_STACK_SWITCHER (bottom_switcher), BOBGUI_STACK (bottom_stack));

  bobgui_stack_add_titled (
    BOBGUI_STACK (bottom_stack),
    create_log_surface (
      "[Activity] BobGUI Search Studio shell initialized.\n"
      "[Activity] Top pages stay toolkit-exclusive to BobGUI APIs.\n"
      "[Activity] Future passes can swap these placeholders for adapter-backed search actions.\n"),
    "activity",
    "Activity");
  bobgui_stack_add_titled (
    BOBGUI_STACK (bottom_stack),
    create_log_surface (
      "[Results] Structured BobGUI results surface placeholder.\n"
      "[Results] Future passes can map search rows into a richer navigator.\n"),
    "results",
    "Results");
  bobgui_stack_add_titled (
    BOBGUI_STACK (bottom_stack),
    create_log_surface (
      "[Diff Preview] Select a future result row to inspect preview details.\n"
      "[Diff Preview] This lower stack mirrors the broader Search Studio cockpit direction.\n"),
    "preview",
    "Diff Preview");

  bobgui_box_append (BOBGUI_BOX (bottom_shell), bottom_switcher);
  bobgui_box_append (BOBGUI_BOX (bottom_shell), bottom_stack);

  bobgui_paned_set_start_child (BOBGUI_PANED (paned), top_shell);
  bobgui_paned_set_end_child (BOBGUI_PANED (paned), bottom_shell);
  bobgui_box_append (BOBGUI_BOX (root), paned);

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
