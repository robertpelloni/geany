#include <bobgui/bobgui.h>

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

static void
unset_title (BobguiWidget *window)
{
  BobguiWidget *box;

  g_assert (BOBGUI_IS_WINDOW (window));

  box = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 0);
  bobgui_widget_hide (box);

  bobgui_window_set_titlebar (BOBGUI_WINDOW (window), box);
}

static void
load_css (BobguiWidget  *widget,
          const char *css)
{
  BobguiCssProvider *provider;
  BobguiStyleContext *context;

  context = bobgui_widget_get_style_context (widget);

  provider = bobgui_css_provider_new ();
  bobgui_css_provider_load_from_data (provider, css, -1);
  bobgui_style_context_add_provider (context, BOBGUI_STYLE_PROVIDER (provider), 800);
}

static void
create_regular (BobguiApplication *app)
{
  BobguiWidget *window, *label;

  window = bobgui_application_window_new (app);
  bobgui_window_set_title (BOBGUI_WINDOW (window), "Regular window");

  label = bobgui_label_new ("This window has no titlebar set");
  bobgui_label_set_wrap (BOBGUI_LABEL (label), TRUE);
  bobgui_window_set_child (BOBGUI_WINDOW (window), label);

  bobgui_window_present (BOBGUI_WINDOW (window));
}

static void
create_headerbar_as_titlebar (BobguiApplication *app)
{
  BobguiWidget *window, *header, *label;

  window = bobgui_application_window_new (app);
  bobgui_window_set_title (BOBGUI_WINDOW (window), "Headerbar as titlebar");

  header = bobgui_header_bar_new ();
  bobgui_window_set_titlebar (BOBGUI_WINDOW (window), header);

  label = bobgui_label_new ("This window has a headerbar set as a titlebar");
  bobgui_label_set_wrap (BOBGUI_LABEL (label), TRUE);
  bobgui_window_set_child (BOBGUI_WINDOW (window), label);

  bobgui_window_present (BOBGUI_WINDOW (window));
}

static void
create_headerbar_inside_window (BobguiApplication *app)
{
  BobguiWidget *window, *box, *header, *label;

  window = bobgui_application_window_new (app);
  bobgui_window_set_title (BOBGUI_WINDOW (window), "Headerbar inside window");
  unset_title (window);

  box = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 0);
  bobgui_window_set_child (BOBGUI_WINDOW (window), box);

  header = bobgui_header_bar_new ();
  bobgui_box_append (BOBGUI_BOX (box), header);

  label = bobgui_label_new ("This window has a headerbar inside the window and no titlebar");
  bobgui_label_set_wrap (BOBGUI_LABEL (label), TRUE);
  bobgui_widget_set_vexpand (label, TRUE);
  bobgui_box_append (BOBGUI_BOX (box), label);

  bobgui_window_present (BOBGUI_WINDOW (window));
}

static void
create_headerbar_overlay (BobguiApplication *app)
{
  BobguiWidget *window, *overlay, *sw, *box, *header, *label;

  window = bobgui_application_window_new (app);
  bobgui_window_set_title (BOBGUI_WINDOW (window), "Headerbar overlaying content");
  unset_title (window);

  overlay = bobgui_overlay_new ();
  bobgui_window_set_child (BOBGUI_WINDOW (window), overlay);

  header = bobgui_header_bar_new ();
  bobgui_widget_set_valign (header, BOBGUI_ALIGN_START);
  bobgui_overlay_add_overlay (BOBGUI_OVERLAY (overlay), header);
  load_css (header, "headerbar { background: alpha(shade(@theme_bg_color, .9), .8); }");

  sw = bobgui_scrolled_window_new ();
  bobgui_scrolled_window_set_policy (BOBGUI_SCROLLED_WINDOW (sw), BOBGUI_POLICY_NEVER, BOBGUI_POLICY_AUTOMATIC);
  bobgui_widget_set_size_request (sw, 300, 250);
  bobgui_overlay_set_child (BOBGUI_OVERLAY (overlay), sw);

  box = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 12);
  bobgui_scrolled_window_set_child (BOBGUI_SCROLLED_WINDOW (sw), box);
  bobgui_widget_set_size_request (sw, 300, 250);

  label = bobgui_label_new ("Lorem ipsum dolor sit amet, consectetur adipiscing elit. "
                         "Nulla innn urna ac dui malesuada ornare. Nullam dictum "
                         "tempor mi et tincidunt. Aliquam metus nulla, auctor "
                         "vitae pulvinar nec, egestas at mi. Class aptent taciti "
                         "sociosqu ad litora torquent per conubia nostra, per "
                         "inceptos himenaeos. Aliquam sagittis, tellus congue "
                         "cursus congue, diam massa mollis enim, sit amet gravida "
                         "magna turpis egestas sapien. Aenean vel molestie nunc. "
                         "In hac habitasse platea dictumst. Suspendisse lacinia"
                         "mi eu ipsum vestibulum in venenatis enim commodo. "
                         "Vivamus non malesuada ligula.");
  bobgui_label_set_wrap (BOBGUI_LABEL (label), TRUE);
  bobgui_box_append (BOBGUI_BOX (box), label);

  label = bobgui_label_new ("This window has a headerbar inside an overlay, so the text is visible underneath it");
  bobgui_label_set_wrap (BOBGUI_LABEL (label), TRUE);
  bobgui_widget_set_vexpand (label, TRUE);
  bobgui_box_append (BOBGUI_BOX (box), label);

  bobgui_window_present (BOBGUI_WINDOW (window));
}

static void
create_hiding_headerbar (BobguiApplication *app)
{
  BobguiWidget *window, *box, *revealer, *header, *label, *hbox, *toggle;

  window = bobgui_application_window_new (app);
  bobgui_window_set_title (BOBGUI_WINDOW (window), "Hiding headerbar");
  unset_title (window);

  box = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 0);
  bobgui_window_set_child (BOBGUI_WINDOW (window), box);

  revealer = bobgui_revealer_new ();
  bobgui_box_append (BOBGUI_BOX (box), revealer);

  header = bobgui_header_bar_new ();
  bobgui_revealer_set_child (BOBGUI_REVEALER (revealer), header);

  label = bobgui_label_new ("This window's headerbar can be shown and hidden with animation");
  bobgui_label_set_wrap (BOBGUI_LABEL (label), TRUE);
  bobgui_widget_set_vexpand (label, TRUE);
  bobgui_box_append (BOBGUI_BOX (box), label);

  hbox = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 12);
  bobgui_widget_set_halign (hbox, BOBGUI_ALIGN_CENTER);
  bobgui_widget_set_margin_top (hbox, 12);
  bobgui_widget_set_margin_bottom (hbox, 12);
  bobgui_widget_set_margin_start (hbox, 12);
  bobgui_widget_set_margin_end (hbox, 12);
  bobgui_box_append (BOBGUI_BOX (box), hbox);

  toggle = bobgui_switch_new ();
  bobgui_switch_set_active (BOBGUI_SWITCH (toggle), TRUE);
  bobgui_box_append (BOBGUI_BOX (hbox), toggle);
  g_object_bind_property (toggle, "active",
                          revealer, "reveal-child",
                          G_BINDING_SYNC_CREATE);

  label = bobgui_label_new ("Show headerbar");
  bobgui_box_append (BOBGUI_BOX (hbox), label);

  bobgui_window_present (BOBGUI_WINDOW (window));
}

static void
create_fake_headerbar (BobguiApplication *app)
{
  BobguiWidget *window, *handle, *box, *center_box, *controls, *label;

  window = bobgui_application_window_new (app);
  bobgui_window_set_title (BOBGUI_WINDOW (window), "Fake headerbar");
  unset_title (window);

  handle = bobgui_window_handle_new ();
  bobgui_window_set_child (BOBGUI_WINDOW (window), handle);

  box = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 0);
  bobgui_window_handle_set_child (BOBGUI_WINDOW_HANDLE (handle), box);

  center_box = bobgui_center_box_new ();
  bobgui_box_append (BOBGUI_BOX (box), center_box);

  label = bobgui_label_new ("Fake headerbar");
  bobgui_center_box_set_center_widget (BOBGUI_CENTER_BOX (center_box), label);

  controls = bobgui_window_controls_new (BOBGUI_PACK_START);
  bobgui_center_box_set_start_widget (BOBGUI_CENTER_BOX (center_box), controls);

  controls = bobgui_window_controls_new (BOBGUI_PACK_END);
  bobgui_center_box_set_end_widget (BOBGUI_CENTER_BOX (center_box), controls);

  label = bobgui_label_new ("This window's titlebar is just a centerbox with a label and window controls.\nThe whole window is draggable.");
  bobgui_label_set_wrap (BOBGUI_LABEL (label), TRUE);
  bobgui_widget_set_vexpand (label, TRUE);
  bobgui_box_append (BOBGUI_BOX (box), label);

  bobgui_window_present (BOBGUI_WINDOW (window));
}

/* split headerbar  */

static void
split_decorations (BobguiSettings *settings,
                   GParamSpec  *pspec,
                   BobguiBuilder  *builder)
{
  BobguiWidget *sheader, *mheader;
  char *layout, *p1, *p2;
  char **p;

  sheader = (BobguiWidget *)bobgui_builder_get_object (builder, "sidebar-header");
  mheader = (BobguiWidget *)bobgui_builder_get_object (builder, "main-header");

  g_object_get (settings, "bobgui-decoration-layout", &layout, NULL);

  p = g_strsplit (layout, ":", -1);

  p1 = g_strconcat ("", p[0], ":", NULL);

  if (g_strv_length (p) >= 2)
    p2 = g_strconcat (":", p[1], NULL);
  else
    p2 = g_strdup ("");

  bobgui_header_bar_set_decoration_layout (BOBGUI_HEADER_BAR (sheader), p1);
  bobgui_header_bar_set_decoration_layout (BOBGUI_HEADER_BAR (mheader), p2);
 
  g_free (p1);
  g_free (p2);
  g_strfreev (p);
  g_free (layout);
}


static void
create_split_headerbar (BobguiApplication *app)
{
  BobguiBuilder *builder;
  BobguiSettings *settings;
  BobguiWidget *win;
  BobguiWidget *entry;
  BobguiWidget *check;
  BobguiWidget *header;
  const char *ui = "tests/testsplitheaders.ui";

  if (!g_file_test (ui, G_FILE_TEST_EXISTS))
    {
      g_warning ("Can't find %s", ui);
      return;
    }

  builder = bobgui_builder_new_from_file (ui);

  win = (BobguiWidget *)bobgui_builder_get_object (builder, "window");
  bobgui_window_set_application (BOBGUI_WINDOW (win), app);

  settings = bobgui_widget_get_settings (win);

  g_signal_connect (settings, "notify::bobgui-decoration-layout",
                    G_CALLBACK (split_decorations), builder);
  split_decorations (settings, NULL, builder);

  entry = (BobguiWidget *)bobgui_builder_get_object (builder, "layout-entry");
  g_object_bind_property (settings, "bobgui-decoration-layout",
                          entry, "text",
                          G_BINDING_BIDIRECTIONAL|G_BINDING_SYNC_CREATE);
  check = (BobguiWidget *)bobgui_builder_get_object (builder, "decorations");
  header = (BobguiWidget *)bobgui_builder_get_object (builder, "sidebar-header");
  g_object_bind_property (check, "active",
                          header, "show-title-buttons",
                          G_BINDING_DEFAULT);
  header = (BobguiWidget *)bobgui_builder_get_object (builder, "main-header");
  g_object_bind_property (check, "active",
                          header, "show-title-buttons",
			  G_BINDING_DEFAULT);
  bobgui_window_present (BOBGUI_WINDOW (win));
}

/* stacked headers */

static void
back_to_main (BobguiButton *button,
              BobguiWidget *win)
{
  BobguiWidget *header_stack;
  BobguiWidget *page_stack;

  header_stack = BOBGUI_WIDGET (g_object_get_data (G_OBJECT (win), "header-stack"));
  page_stack = BOBGUI_WIDGET (g_object_get_data (G_OBJECT (win), "page-stack"));

  bobgui_stack_set_visible_child_name (BOBGUI_STACK (header_stack), "main");
  bobgui_stack_set_visible_child_name (BOBGUI_STACK (page_stack), "page1");
}

static void
go_to_secondary (BobguiButton *button,
                 BobguiWidget *win)
{
  BobguiWidget *header_stack;
  BobguiWidget *page_stack;

  header_stack = BOBGUI_WIDGET (g_object_get_data (G_OBJECT (win), "header-stack"));
  page_stack = BOBGUI_WIDGET (g_object_get_data (G_OBJECT (win), "page-stack"));

  bobgui_stack_set_visible_child_name (BOBGUI_STACK (header_stack), "secondary");
  bobgui_stack_set_visible_child_name (BOBGUI_STACK (page_stack), "secondary");
}

static void
create_stacked_headerbar (BobguiApplication *app)
{
  BobguiBuilder *builder;
  BobguiWidget *win;
  BobguiWidget *new_btn;
  BobguiWidget *back_btn;
  BobguiWidget *header_stack;
  BobguiWidget *page_stack;
  const char *ui = "tests/teststackedheaders.ui";

  if (!g_file_test (ui, G_FILE_TEST_EXISTS))
    {
      g_warning ("Can't find %s", ui);
      return;
    }

  builder = bobgui_builder_new ();
  bobgui_builder_add_from_file (builder, ui, NULL);

  win = (BobguiWidget *)bobgui_builder_get_object (builder, "window");
  bobgui_window_set_application (BOBGUI_WINDOW (win), app);

  header_stack = (BobguiWidget *)bobgui_builder_get_object (builder, "header_stack");
  page_stack = (BobguiWidget *)bobgui_builder_get_object (builder, "page_stack");

  g_object_set_data (G_OBJECT (win), "header-stack", header_stack);
  g_object_set_data (G_OBJECT (win), "page-stack", page_stack);

  new_btn = (BobguiWidget *)bobgui_builder_get_object (builder, "new_btn");
  back_btn = (BobguiWidget *)bobgui_builder_get_object (builder, "back_btn");

  g_signal_connect (new_btn, "clicked", G_CALLBACK (go_to_secondary), win);
  g_signal_connect (back_btn, "clicked", G_CALLBACK (back_to_main), win);

  bobgui_window_present (BOBGUI_WINDOW (win));
}

/* controls */
static void
create_controls (BobguiApplication *app)
{
  BobguiBuilder *builder;
  BobguiWidget *win;
  const char *ui = "tests/testheadercontrols.ui";

  if (!g_file_test (ui, G_FILE_TEST_EXISTS))
    {
      g_warning ("Can't find %s", ui);
      return;
    }

  builder = bobgui_builder_new_from_file (ui);

  win = (BobguiWidget *)bobgui_builder_get_object (builder, "window");
  bobgui_window_set_application (BOBGUI_WINDOW (win), app);

  bobgui_window_present (BOBGUI_WINDOW (win));
}

/* technorama */

static const char css[] =
 ".main.background { "
 " background-image: linear-gradient(to bottom, red, blue);"
 " border-width: 0px; "
 "}"
 ".titlebar.backdrop { "
 " background-image: none; "
 " background-color: @bg_color; "
 " border-radius: 10px 10px 0px 0px; "
 "}"
 ".titlebar { "
 " background-image: linear-gradient(to bottom, white, @bg_color);"
 " border-radius: 10px 10px 0px 0px; "
 "}";

static void
on_bookmark_clicked (BobguiButton *button, gpointer data)
{
  BobguiWindow *window = BOBGUI_WINDOW (data);
  BobguiWidget *chooser;

  chooser = bobgui_file_chooser_dialog_new ("File Chooser Test",
                                         window,
                                         BOBGUI_FILE_CHOOSER_ACTION_OPEN,
                                         "_Close",
                                         BOBGUI_RESPONSE_CLOSE,
                                         NULL);

  g_signal_connect (chooser, "response",
                    G_CALLBACK (bobgui_window_destroy), NULL);

  bobgui_window_present (BOBGUI_WINDOW (chooser));
}

static void
toggle_fullscreen (BobguiButton *button, gpointer data)
{
  BobguiWidget *window = BOBGUI_WIDGET (data);
  static gboolean fullscreen = FALSE;

  if (fullscreen)
    {
      bobgui_window_unfullscreen (BOBGUI_WINDOW (window));
      fullscreen = FALSE;
    }
  else
    {
      bobgui_window_fullscreen (BOBGUI_WINDOW (window));
      fullscreen = TRUE;
    }
}

static void
change_header (BobguiButton *button, gpointer data)
{
  BobguiWidget *window = BOBGUI_WIDGET (data);
  BobguiWidget *label;
  BobguiWidget *widget;
  BobguiWidget *header;

  if (button && bobgui_toggle_button_get_active (BOBGUI_TOGGLE_BUTTON (button)))
    {
      header = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 10);
      bobgui_widget_add_css_class (header, "titlebar");
      bobgui_widget_add_css_class (header, "header-bar");
      bobgui_widget_set_margin_start (header, 10);
      bobgui_widget_set_margin_end (header, 10);
      bobgui_widget_set_margin_top (header, 10);
      bobgui_widget_set_margin_bottom (header, 10);
      label = bobgui_label_new ("Label");
      bobgui_box_append (BOBGUI_BOX (header), label);
      widget = bobgui_level_bar_new ();
      bobgui_level_bar_set_value (BOBGUI_LEVEL_BAR (widget), 0.4);
      bobgui_widget_set_hexpand (widget, TRUE);
      bobgui_box_append (BOBGUI_BOX (header), widget);
    }
  else
    {
      header = bobgui_header_bar_new ();
      bobgui_widget_add_css_class (header, "titlebar");

      widget = bobgui_button_new_with_label ("_Close");
      bobgui_button_set_use_underline (BOBGUI_BUTTON (widget), TRUE);
      bobgui_widget_add_css_class (widget, "suggested-action");
      g_signal_connect_swapped (widget, "clicked", G_CALLBACK (bobgui_window_destroy), window);

      bobgui_header_bar_pack_end (BOBGUI_HEADER_BAR (header), widget);

      widget= bobgui_button_new_from_icon_name ("bookmark-new-symbolic");
      g_signal_connect (widget, "clicked", G_CALLBACK (on_bookmark_clicked), window);

      bobgui_header_bar_pack_start (BOBGUI_HEADER_BAR (header), widget);
    }

  bobgui_window_set_titlebar (BOBGUI_WINDOW (window), header);
}

static void
create_technorama (BobguiApplication *app)
{
  BobguiWidget *window;
  BobguiWidget *box;
  BobguiWidget *footer;
  BobguiWidget *button;
  BobguiWidget *content;
  BobguiCssProvider *provider;

  window = bobgui_window_new ();
  bobgui_window_set_application (BOBGUI_WINDOW (window), app);

  bobgui_widget_add_css_class (window, "main");

  provider = bobgui_css_provider_new ();
  bobgui_css_provider_load_from_data (provider, css, -1);
  bobgui_style_context_add_provider_for_display (bobgui_widget_get_display (window),
                                              BOBGUI_STYLE_PROVIDER (provider),
                                              BOBGUI_STYLE_PROVIDER_PRIORITY_USER);


  change_header (NULL, window);

  box = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 0);
  bobgui_window_set_child (BOBGUI_WINDOW (window), box);

  content = bobgui_image_new_from_icon_name ("start-here-symbolic");
  bobgui_image_set_pixel_size (BOBGUI_IMAGE (content), 512);
  bobgui_widget_set_vexpand (content, TRUE);

  bobgui_box_append (BOBGUI_BOX (box), content);

  footer = bobgui_action_bar_new ();
  bobgui_action_bar_set_center_widget (BOBGUI_ACTION_BAR (footer), bobgui_check_button_new_with_label ("Middle"));
  button = bobgui_toggle_button_new_with_label ("Custom");
  g_signal_connect (button, "clicked", G_CALLBACK (change_header), window);
  bobgui_action_bar_pack_start (BOBGUI_ACTION_BAR (footer), button);
  button = bobgui_button_new_with_label ("Fullscreen");
  bobgui_action_bar_pack_end (BOBGUI_ACTION_BAR (footer), button);
  g_signal_connect (button, "clicked", G_CALLBACK (toggle_fullscreen), window);
  bobgui_box_append (BOBGUI_BOX (box), footer);
  bobgui_window_present (BOBGUI_WINDOW (window));
}

struct {
  const char *name;
  void (*cb) (BobguiApplication *app);
} buttons[] =
{
    { "Regular window", create_regular },
    { "Headerbar as titlebar", create_headerbar_as_titlebar },
    { "Headerbar inside window", create_headerbar_inside_window },
    { "Headerbar overlaying content", create_headerbar_overlay },
    { "Hiding headerbar", create_hiding_headerbar },
    { "Fake headerbar", create_fake_headerbar },
    { "Split headerbar", create_split_headerbar },
    { "Stacked headerbar", create_stacked_headerbar },
    { "Headerbar with controls", create_controls },
    { "Technorama", create_technorama },
};
int n_buttons = sizeof (buttons) / sizeof (buttons[0]);

static void
app_activate_cb (BobguiApplication *app)
{
  BobguiWidget *window, *box;
  int i;

  window = bobgui_application_window_new (app);
  bobgui_window_set_title (BOBGUI_WINDOW (window), "Headerbar test");

  box = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 0);
  bobgui_widget_set_halign (box, BOBGUI_ALIGN_CENTER);
  bobgui_widget_set_valign (box, BOBGUI_ALIGN_CENTER);
  bobgui_widget_add_css_class (box, "linked");
  bobgui_window_set_child (BOBGUI_WINDOW (window), box);

  for (i = 0; i < n_buttons; i++)
    {
      BobguiWidget *btn;

      btn = bobgui_button_new_with_label (buttons[i].name);
      g_signal_connect_object (btn,
                               "clicked",
                               G_CALLBACK (buttons[i].cb),
                               app,
                               G_CONNECT_SWAPPED);
      bobgui_box_append (BOBGUI_BOX (box), btn);
    }

  bobgui_window_present (BOBGUI_WINDOW (window));
}

int
main (int    argc,
      char **argv)
{
  BobguiApplication *app;

  app = bobgui_application_new ("org.bobgui.Test.headerbar2", 0);

  g_signal_connect (app,
                    "activate",
                    G_CALLBACK (app_activate_cb),
                    NULL);

  g_application_run (G_APPLICATION (app), argc, argv);

  return 0;
}
