#include <string.h>
#include <bobgui/bobgui.h>

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

static const char *css =
".overlay-green {"
"  background-image: none;"
"  background-color: green;"
"}\n"
".overlay-white {"
"  background-image: none;"
"  background-color: white;"
"}\n"
".transparent-red {"
"  background-image: none;"
"  background-color: rgba(255, 0, 0, 0.8);"
"}\n"
".transparent-green {"
"  background-image: none;"
"  background-color: rgba(0, 255, 0, 0.8);"
"}\n"
".transparent-blue {"
"  background-image: none;"
"  background-color: rgba(0, 0, 255, 0.8);"
"}\n"
".transparent-purple {"
"  background-image: none;"
"  background-color: rgba(255, 0, 255, 0.8);"
"}\n"
;


/* test that margins and non-zero allocation x/y
 * of the main widget are handled correctly
 */
static BobguiWidget *
test_nonzerox (void)
{
  BobguiWidget *win;
  BobguiWidget *grid;
  BobguiWidget *overlay;
  BobguiWidget *text;
  BobguiWidget *child;

  win = bobgui_window_new ();
  bobgui_window_set_title (BOBGUI_WINDOW (win), "Non-zero X");

  grid = bobgui_grid_new ();
  bobgui_widget_set_margin_start (grid, 5);
  bobgui_widget_set_margin_end (grid, 5);
  bobgui_widget_set_margin_top (grid, 5);
  bobgui_widget_set_margin_bottom (grid, 5);

  bobgui_window_set_child (BOBGUI_WINDOW (win), grid);
  bobgui_grid_attach (BOBGUI_GRID (grid), bobgui_label_new ("Above"), 1, 0, 1, 1);
  bobgui_grid_attach (BOBGUI_GRID (grid), bobgui_label_new ("Below"), 1, 2, 1, 1);
  bobgui_grid_attach (BOBGUI_GRID (grid), bobgui_label_new ("Left"), 0, 1, 1, 1);
  bobgui_grid_attach (BOBGUI_GRID (grid), bobgui_label_new ("Right"), 2, 1, 1, 1);

  overlay = bobgui_overlay_new ();
  bobgui_grid_attach (BOBGUI_GRID (grid), overlay, 1, 1, 1, 1);

  text = bobgui_text_view_new ();
  bobgui_widget_set_size_request (text, 200, 200);
  bobgui_widget_set_hexpand (text, TRUE);
  bobgui_widget_set_vexpand (text, TRUE);
  bobgui_overlay_set_child (BOBGUI_OVERLAY (overlay), text);

  child = bobgui_label_new ("I'm the overlay");
  bobgui_widget_set_halign (child, BOBGUI_ALIGN_START);
  bobgui_widget_set_valign (child, BOBGUI_ALIGN_START);
  bobgui_widget_set_margin_start (child, 3);
  bobgui_widget_set_margin_end (child, 3);
  bobgui_widget_set_margin_top (child, 3);
  bobgui_widget_set_margin_bottom (child, 3);

  bobgui_overlay_add_overlay (BOBGUI_OVERLAY (overlay), child);

  child = bobgui_label_new ("No, I'm the overlay");
  bobgui_widget_set_halign (child, BOBGUI_ALIGN_END);
  bobgui_widget_set_valign (child, BOBGUI_ALIGN_END);
  bobgui_overlay_add_overlay (BOBGUI_OVERLAY (overlay), child);
  bobgui_widget_set_margin_start (child, 3);
  bobgui_widget_set_margin_end (child, 3);
  bobgui_widget_set_margin_top (child, 3);
  bobgui_widget_set_margin_bottom (child, 3);

  return win;
}

static gboolean
get_child_position (BobguiOverlay    *overlay,
                    BobguiWidget     *widget,
                    BobguiAllocation *alloc,
                    BobguiWidget     *relative)
{
  BobguiRequisition req;
  BobguiWidget *child;
  BobguiAllocation main_alloc;
  double x, y;

  child = bobgui_overlay_get_child (BOBGUI_OVERLAY (overlay));

  bobgui_widget_translate_coordinates (relative, child, 0, 0, &x, &y);
  main_alloc.x = x;
  main_alloc.y = y;
  main_alloc.width = bobgui_widget_get_allocated_width (relative);
  main_alloc.height = bobgui_widget_get_allocated_height (relative);

  bobgui_widget_get_preferred_size (widget, NULL, &req);

  alloc->x = main_alloc.x;
  alloc->width = MIN (main_alloc.width, req.width);
  if (bobgui_widget_get_halign (widget) == BOBGUI_ALIGN_END)
    alloc->x += main_alloc.width - req.width;

  alloc->y = main_alloc.y;
  alloc->height = MIN (main_alloc.height, req.height);
  if (bobgui_widget_get_valign (widget) == BOBGUI_ALIGN_END)
    alloc->y += main_alloc.height - req.height;

  return TRUE;
}

/* test custom positioning */
static BobguiWidget *
test_relative (void)
{
  BobguiWidget *win;
  BobguiWidget *grid;
  BobguiWidget *overlay;
  BobguiWidget *text;
  BobguiWidget *child;

  win = bobgui_window_new ();
  bobgui_window_set_title (BOBGUI_WINDOW (win), "Custom positioning");

  overlay = bobgui_overlay_new ();
  bobgui_window_set_child (BOBGUI_WINDOW (win), overlay);

  grid = bobgui_grid_new ();
  bobgui_overlay_set_child (BOBGUI_OVERLAY (overlay), grid);
  bobgui_grid_attach (BOBGUI_GRID (grid), bobgui_label_new ("Above"), 1, 0, 1, 1);
  bobgui_grid_attach (BOBGUI_GRID (grid), bobgui_label_new ("Below"), 1, 2, 1, 1);
  bobgui_grid_attach (BOBGUI_GRID (grid), bobgui_label_new ("Left"), 0, 1, 1, 1);
  bobgui_grid_attach (BOBGUI_GRID (grid), bobgui_label_new ("Right"), 2, 1, 1, 1);

  text = bobgui_text_view_new ();
  bobgui_widget_set_size_request (text, 200, 200);
  bobgui_widget_set_margin_start (text, 5);
  bobgui_widget_set_margin_end (text, 5);
  bobgui_widget_set_margin_top (text, 5);
  bobgui_widget_set_margin_bottom (text, 5);
  bobgui_widget_set_hexpand (text, TRUE);
  bobgui_widget_set_vexpand (text, TRUE);
  bobgui_grid_attach (BOBGUI_GRID (grid), text, 1, 1, 1, 1);
  g_signal_connect (overlay, "get-child-position",
                    G_CALLBACK (get_child_position), text);

  child = bobgui_label_new ("Top left overlay");
  bobgui_widget_set_halign (child, BOBGUI_ALIGN_START);
  bobgui_widget_set_valign (child, BOBGUI_ALIGN_START);
  bobgui_overlay_add_overlay (BOBGUI_OVERLAY (overlay), child);
  bobgui_widget_set_margin_start (child, 1);
  bobgui_widget_set_margin_end (child, 1);
  bobgui_widget_set_margin_top (child, 1);
  bobgui_widget_set_margin_bottom (child, 1);

  child = bobgui_label_new ("Bottom right overlay");
  bobgui_widget_set_halign (child, BOBGUI_ALIGN_END);
  bobgui_widget_set_valign (child, BOBGUI_ALIGN_END);
  bobgui_overlay_add_overlay (BOBGUI_OVERLAY (overlay), child);
  bobgui_widget_set_margin_start (child, 1);
  bobgui_widget_set_margin_end (child, 1);
  bobgui_widget_set_margin_top (child, 1);
  bobgui_widget_set_margin_bottom (child, 1);

  return win;
}

/* test BOBGUI_ALIGN_FILL handling */
static BobguiWidget *
test_fullwidth (void)
{
  BobguiWidget *win;
  BobguiWidget *overlay;
  BobguiWidget *text;
  BobguiWidget *child;

  win = bobgui_window_new ();
  bobgui_window_set_title (BOBGUI_WINDOW (win), "Full-width");

  overlay = bobgui_overlay_new ();
  bobgui_window_set_child (BOBGUI_WINDOW (win), overlay);

  text = bobgui_text_view_new ();
  bobgui_widget_set_size_request (text, 200, 200);
  bobgui_widget_set_hexpand (text, TRUE);
  bobgui_widget_set_vexpand (text, TRUE);
  bobgui_overlay_set_child (BOBGUI_OVERLAY (overlay), text);

  child = bobgui_label_new ("Fullwidth top overlay");
  bobgui_widget_set_halign (child, BOBGUI_ALIGN_FILL);
  bobgui_widget_set_valign (child, BOBGUI_ALIGN_START);
  bobgui_overlay_add_overlay (BOBGUI_OVERLAY (overlay), child);
  bobgui_widget_set_margin_start (child, 4);
  bobgui_widget_set_margin_end (child, 4);
  bobgui_widget_set_margin_top (child, 4);
  bobgui_widget_set_margin_bottom (child, 4);

  return win;
}

/* test that scrolling works as expected */
static BobguiWidget *
test_scrolling (void)
{
  BobguiWidget *win;
  BobguiWidget *overlay;
  BobguiWidget *sw;
  BobguiWidget *text;
  BobguiWidget *child;
  BobguiTextBuffer *buffer;
  char *contents;
  gsize len;

  win = bobgui_window_new ();
  bobgui_window_set_title (BOBGUI_WINDOW (win), "Scrolling");

  overlay = bobgui_overlay_new ();
  bobgui_window_set_child (BOBGUI_WINDOW (win), overlay);

  sw = bobgui_scrolled_window_new ();
  bobgui_scrolled_window_set_min_content_width (BOBGUI_SCROLLED_WINDOW (sw), 200);
  bobgui_scrolled_window_set_min_content_height (BOBGUI_SCROLLED_WINDOW (sw), 200);
  bobgui_overlay_set_child (BOBGUI_OVERLAY (overlay), sw);

  text = bobgui_text_view_new ();
  buffer = bobgui_text_buffer_new (NULL);
  if (!g_file_get_contents ("testoverlay.c", &contents, &len, NULL))
    {
      contents = g_strdup ("Text should go here...");
      len = strlen (contents);
    }
  bobgui_text_buffer_set_text (buffer, contents, len);
  g_free (contents);
  bobgui_text_view_set_buffer (BOBGUI_TEXT_VIEW (text), buffer);

  bobgui_widget_set_hexpand (text, TRUE);
  bobgui_widget_set_vexpand (text, TRUE);
  bobgui_scrolled_window_set_child (BOBGUI_SCROLLED_WINDOW (sw), text);

  child = bobgui_label_new ("This should be visible");
  bobgui_widget_set_halign (child, BOBGUI_ALIGN_CENTER);
  bobgui_widget_set_valign (child, BOBGUI_ALIGN_END);
  bobgui_overlay_add_overlay (BOBGUI_OVERLAY (overlay), child);
  bobgui_widget_set_margin_start (child, 4);
  bobgui_widget_set_margin_end (child, 4);
  bobgui_widget_set_margin_top (child, 4);
  bobgui_widget_set_margin_bottom (child, 4);

  return win;
}

static const char *buffer =
"<interface>"
"  <object class='BobguiWindow' id='window'>"
"    <property name='title'>BobguiBuilder support</property>"
"    <child>"
"      <object class='BobguiOverlay' id='overlay'>"
"        <child type='overlay'>"
"          <object class='BobguiLabel' id='overlay-child'>"
"            <property name='label'>Witty remark goes here</property>"
"            <property name='halign'>end</property>"
"            <property name='valign'>end</property>"
"            <property name='margin-top'>4</property>"
"            <property name='margin-bottom'>4</property>"
"            <property name='margin-start'>4</property>"
"            <property name='margin-end'>4</property>"
"          </object>"
"        </child>"
"        <child>"
"          <object class='BobguiGrid' id='grid'>"
"            <child>"
"              <object class='BobguiLabel' id='left'>"
"                <property name='label'>Left</property>"
"                <layout>"
"                  <property name='column'>0</property>"
"                  <property name='row'>0</property>"
"                </layout>"
"              </object>"
"            </child>"
"            <child>"
"              <object class='BobguiLabel' id='right'>"
"                <property name='label'>Right</property>"
"                <layout>"
"                  <property name='column'>2</property>"
"                  <property name='row'>0</property>"
"                </layout>"
"              </object>"
"            </child>"
"            <child>"
"              <object class='BobguiTextView' id='text'>"
"                 <property name='width-request'>200</property>"
"                 <property name='height-request'>200</property>"
"                 <property name='hexpand'>True</property>"
"                 <property name='vexpand'>True</property>"
"                <layout>"
"                  <property name='column'>1</property>"
"                  <property name='row'>0</property>"
"                </layout>"
"              </object>"
"            </child>"
"          </object>"
"        </child>"
"      </object>"
"    </child>"
"  </object>"
"</interface>";

/* test that overlays can be constructed with BobguiBuilder */
static BobguiWidget *
test_builder (void)
{
  BobguiBuilder *builder;
  BobguiWidget *win;
  GError *error;

  builder = bobgui_builder_new ();

  error = NULL;
  if (!bobgui_builder_add_from_string (builder, buffer, -1, &error))
    {
      g_warning ("%s", error->message);
      g_error_free (error);
      return NULL;
    }

  win = (BobguiWidget *)bobgui_builder_get_object (builder, "window");
  g_object_ref (win);

  g_object_unref (builder);

  return win;
}

static void
on_enter (BobguiEventController *controller,
          double              x,
          double              y,
          BobguiWidget          *overlay)
{
  BobguiWidget *child = bobgui_event_controller_get_widget (controller);

  if (bobgui_widget_get_halign (child) == BOBGUI_ALIGN_START)
    bobgui_widget_set_halign (child, BOBGUI_ALIGN_END);
  else
    bobgui_widget_set_halign (child, BOBGUI_ALIGN_START);

  bobgui_widget_queue_resize (overlay);
}

static BobguiWidget *
test_chase (void)
{
  BobguiWidget *win;
  BobguiWidget *overlay;
  BobguiWidget *sw;
  BobguiWidget *text;
  BobguiWidget *child;
  BobguiTextBuffer *text_buffer;
  char *contents;
  gsize len;
  BobguiEventController *controller;

  win = bobgui_window_new ();
  bobgui_window_set_title (BOBGUI_WINDOW (win), "Chase");

  overlay = bobgui_overlay_new ();
  bobgui_window_set_child (BOBGUI_WINDOW (win), overlay);

  sw = bobgui_scrolled_window_new ();
  bobgui_scrolled_window_set_min_content_width (BOBGUI_SCROLLED_WINDOW (sw), 200);
  bobgui_scrolled_window_set_min_content_height (BOBGUI_SCROLLED_WINDOW (sw), 200);
  bobgui_overlay_set_child (BOBGUI_OVERLAY (overlay), sw);

  text = bobgui_text_view_new ();
  text_buffer = bobgui_text_buffer_new (NULL);
  if (!g_file_get_contents ("testoverlay.c", &contents, &len, NULL))
    {
      contents = g_strdup ("Text should go here...");
      len = strlen (contents);
    }
  bobgui_text_buffer_set_text (text_buffer, contents, len);
  g_free (contents);
  bobgui_text_view_set_buffer (BOBGUI_TEXT_VIEW (text), text_buffer);

  bobgui_widget_set_hexpand (text, TRUE);
  bobgui_widget_set_vexpand (text, TRUE);
  bobgui_scrolled_window_set_child (BOBGUI_SCROLLED_WINDOW (sw), text);

  child = bobgui_label_new ("Try to enter");
  bobgui_widget_set_halign (child, BOBGUI_ALIGN_START);
  bobgui_widget_set_valign (child, BOBGUI_ALIGN_END);
  bobgui_overlay_add_overlay (BOBGUI_OVERLAY (overlay), child);
  bobgui_widget_set_margin_start (child, 4);
  bobgui_widget_set_margin_end (child, 4);
  bobgui_widget_set_margin_top (child, 4);
  bobgui_widget_set_margin_bottom (child, 4);

  controller = bobgui_event_controller_motion_new ();
  g_signal_connect (controller, "enter", G_CALLBACK (on_enter), overlay);
  bobgui_widget_add_controller (child, controller);

  return win;
}

static BobguiWidget *
test_stacking (void)
{
  BobguiWidget *win;
  BobguiWidget *overlay;
  BobguiWidget *main_child;
  BobguiWidget *label;
  BobguiWidget *child;
  BobguiWidget *grid;
  BobguiWidget *check1;
  BobguiWidget *check2;

  win = bobgui_window_new ();
  bobgui_window_set_title (BOBGUI_WINDOW (win), "Stacking");

  grid = bobgui_grid_new ();
  overlay = bobgui_overlay_new ();
  main_child = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 0);
  bobgui_widget_add_css_class (main_child, "overlay-green");
  bobgui_widget_set_hexpand (main_child, TRUE);
  bobgui_widget_set_vexpand (main_child, TRUE);
  label = bobgui_label_new ("Main child");
  child = bobgui_label_new ("Overlay");
  bobgui_widget_set_halign (child, BOBGUI_ALIGN_END);
  bobgui_widget_set_valign (child, BOBGUI_ALIGN_END);

  check1 = bobgui_check_button_new_with_label ("Show main");
  g_object_bind_property (main_child, "visible", check1, "active", G_BINDING_BIDIRECTIONAL);

  check2 = bobgui_check_button_new_with_label ("Show overlay");
  g_object_bind_property (child, "visible", check2, "active", G_BINDING_BIDIRECTIONAL);
  bobgui_box_append (BOBGUI_BOX (main_child), label);
  bobgui_overlay_set_child (BOBGUI_OVERLAY (overlay), main_child);
  bobgui_overlay_add_overlay (BOBGUI_OVERLAY (overlay), child);
  bobgui_grid_attach (BOBGUI_GRID (grid), overlay, 1, 0, 1, 3);
  bobgui_window_set_child (BOBGUI_WINDOW (win), grid);

  bobgui_grid_attach (BOBGUI_GRID (grid), check1, 0, 0, 1, 1);
  bobgui_grid_attach (BOBGUI_GRID (grid), check2, 0, 1, 1, 1);
  child = bobgui_label_new ("");
  bobgui_widget_set_vexpand (child, TRUE);
  bobgui_grid_attach (BOBGUI_GRID (grid), child, 0, 2, 1, 1);

  return win;
}

static BobguiWidget *
test_input_stacking (void)
{
  BobguiWidget *win;
  BobguiWidget *overlay;
  BobguiWidget *label, *entry;
  BobguiWidget *grid;
  BobguiWidget *button;
  BobguiWidget *vbox;
  int i,j;

  win = bobgui_window_new ();
  bobgui_window_set_title (BOBGUI_WINDOW (win), "Input Stacking");

  overlay = bobgui_overlay_new ();
  grid = bobgui_grid_new ();
  bobgui_overlay_set_child (BOBGUI_OVERLAY (overlay), grid);

  for (j = 0; j < 5; j++)
    {
      for (i = 0; i < 5; i++)
	{
	  button = bobgui_button_new_with_label ("     ");
	  bobgui_widget_set_hexpand (button, TRUE);
	  bobgui_widget_set_vexpand (button, TRUE);
	  bobgui_grid_attach (BOBGUI_GRID (grid), button, i, j, 1, 1);
	}
    }

  vbox = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 10);
  bobgui_overlay_add_overlay (BOBGUI_OVERLAY (overlay), vbox);
  bobgui_widget_set_can_target (vbox, FALSE);
  bobgui_widget_set_halign (vbox, BOBGUI_ALIGN_CENTER);
  bobgui_widget_set_valign (vbox, BOBGUI_ALIGN_CENTER);

  label = bobgui_label_new ("This is some overlaid text\n"
			 "It does not get input\n"
			 "But the entry does");
  bobgui_widget_set_margin_top (label, 8);
  bobgui_widget_set_margin_bottom (label, 8);
  bobgui_box_append (BOBGUI_BOX (vbox), label);

  entry = bobgui_entry_new ();
  bobgui_widget_set_margin_top (entry, 8);
  bobgui_widget_set_margin_bottom (entry, 8);
  bobgui_box_append (BOBGUI_BOX (vbox), entry);


  bobgui_window_set_child (BOBGUI_WINDOW (win), overlay);

  return win;
}

int
main (int argc, char *argv[])
{
  BobguiWidget *win1;
  BobguiWidget *win2;
  BobguiWidget *win3;
  BobguiWidget *win4;
  BobguiWidget *win5;
  BobguiWidget *win6;
  BobguiWidget *win7;
  BobguiWidget *win8;
  BobguiCssProvider *css_provider;

  bobgui_init ();

  if (g_getenv ("RTL"))
    bobgui_widget_set_default_direction (BOBGUI_TEXT_DIR_RTL);

  css_provider = bobgui_css_provider_new ();
  bobgui_css_provider_load_from_data (css_provider, css, -1);
  bobgui_style_context_add_provider_for_display (gdk_display_get_default (),
                                              BOBGUI_STYLE_PROVIDER (css_provider),
                                              BOBGUI_STYLE_PROVIDER_PRIORITY_APPLICATION);

  win1 = test_nonzerox ();
  bobgui_window_present (BOBGUI_WINDOW (win1));

  win2 = test_relative ();
  bobgui_window_present (BOBGUI_WINDOW (win2));

  win3 = test_fullwidth ();
  bobgui_window_present (BOBGUI_WINDOW (win3));

  win4 = test_scrolling ();
  bobgui_window_present (BOBGUI_WINDOW (win4));

  win5 = test_builder ();
  bobgui_window_present (BOBGUI_WINDOW (win5));

  win6 = test_chase ();
  bobgui_window_present (BOBGUI_WINDOW (win6));

  win7 = test_stacking ();
  bobgui_window_present (BOBGUI_WINDOW (win7));

  win8 = test_input_stacking ();
  bobgui_window_present (BOBGUI_WINDOW (win8));

  while (TRUE)
    g_main_context_iteration (NULL, TRUE);

  return 0;
}
