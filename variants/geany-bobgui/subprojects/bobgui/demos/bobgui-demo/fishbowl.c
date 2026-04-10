/* Benchmark/Fishbowl
 *
 * This demo models the fishbowl demos seen on the web in a BOBGUI way.
 * It's also a neat little tool to see how fast your computer (or
 * your BOBGUI version) is.
 */

#include <bobgui/bobgui.h>

#include "bobguifishbowl.h"
#include "bobguigears.h"

#include "nodewidget.h"
#include "graphwidget.h"

const char *const css =
".blurred-button {"
"  box-shadow: 0px 0px 5px 10px rgba(0, 0, 0, 0.5);"
"}"
"";

char **icon_names = NULL;
gsize n_icon_names = 0;

static void
init_icon_names (BobguiIconTheme *theme)
{
  if (icon_names)
    return;

  icon_names = bobgui_icon_theme_get_icon_names (theme);
  n_icon_names = g_strv_length (icon_names);
}

/* Can't be static because it's also used in iconscroll.c */
BobguiWidget *
create_icon_by_id (gsize icon_index)
{
  BobguiWidget *image;

  image = bobgui_image_new ();

  init_icon_names (bobgui_icon_theme_get_for_display (bobgui_widget_get_display (image)));

  icon_index %= n_icon_names;

  bobgui_image_set_icon_size (BOBGUI_IMAGE (image), BOBGUI_ICON_SIZE_LARGE);
  bobgui_image_set_from_icon_name (BOBGUI_IMAGE (image), icon_names[icon_index]);

  return image;
}

static BobguiWidget *
create_icon (void)
{
  gsize n;

  if (n_icon_names == 0)
    n = 0;
  else
    n = g_random_int_range (0, n_icon_names);

  return create_icon_by_id (n);
}

extern BobguiWidget *create_symbolic (void);
extern BobguiWidget *create_svg (void);

static BobguiWidget *
create_button (void)
{
  return bobgui_button_new_with_label ("Button");
}

static BobguiWidget *
create_blurred_button (void)
{
  BobguiWidget *w = bobgui_button_new ();

  bobgui_widget_add_css_class (w, "blurred-button");

  return w;
}

static BobguiWidget *
create_font_button (void)
{
  return bobgui_font_dialog_button_new (bobgui_font_dialog_new ());
}

static BobguiWidget *
create_level_bar (void)
{
  BobguiWidget *w = bobgui_level_bar_new_for_interval (0, 100);

  bobgui_level_bar_set_value (BOBGUI_LEVEL_BAR (w), 50);

  /* Force them to be a bit larger */
  bobgui_widget_set_size_request (w, 200, -1);

  return w;
}

static BobguiWidget *
create_spinner (void)
{
  BobguiWidget *w = bobgui_spinner_new ();

  bobgui_spinner_start (BOBGUI_SPINNER (w));

  return w;
}

static BobguiWidget *
create_spinbutton (void)
{
  BobguiWidget *w = bobgui_spin_button_new_with_range (0, 10, 1);

  return w;
}

static BobguiWidget *
create_label (void)
{
  BobguiWidget *w = bobgui_label_new ("Lorem ipsum dolor sit amet, consetetur sadipscing elitr, sed diam nonumy eirmod tempor invidunt ut labore et dolore magna aliquyam erat, sed diam voluptua.");

  bobgui_label_set_wrap (BOBGUI_LABEL (w), TRUE);
  bobgui_label_set_max_width_chars (BOBGUI_LABEL (w), 100);

  return w;
}

static BobguiWidget *
create_video (void)
{
  BobguiWidget *w = bobgui_video_new ();

  bobgui_widget_set_size_request (w, 64, 64);
  bobgui_video_set_loop (BOBGUI_VIDEO (w), TRUE);
  bobgui_video_set_autoplay (BOBGUI_VIDEO (w), TRUE);
  bobgui_video_set_resource (BOBGUI_VIDEO (w), "/images/bobgui-logo.webm");

  return w;
}

static BobguiWidget *
create_gears (void)
{
  BobguiWidget *w = bobgui_gears_new ();

  bobgui_widget_set_size_request (w, 100, 100);

  return w;
}

static BobguiWidget *
create_switch (void)
{
  BobguiWidget *w = bobgui_switch_new ();

  bobgui_switch_set_state (BOBGUI_SWITCH (w), TRUE);

  return w;
}

static void
mapped (BobguiWidget *w)
{
  bobgui_menu_button_popup (BOBGUI_MENU_BUTTON (w));
}

static BobguiWidget *
create_menu_button (void)
{
  BobguiWidget *w = bobgui_menu_button_new ();
  BobguiWidget *popover = bobgui_popover_new ();

  bobgui_popover_set_child (BOBGUI_POPOVER (popover), bobgui_button_new_with_label ("Hey!"));
  bobgui_popover_set_autohide (BOBGUI_POPOVER (popover), FALSE);
  bobgui_menu_button_set_popover (BOBGUI_MENU_BUTTON (w), popover);
  g_signal_connect (w, "map", G_CALLBACK (mapped), NULL);

  return w;
}

static BobguiWidget *
create_tiger (void)
{
  return node_widget_new ("/fishbowl/tiger.node");
}

static BobguiWidget *
create_graph (void)
{
  return graph_widget_new ();
}

static const struct {
  const char *name;
  BobguiWidget * (* create_func) (void);
  gboolean    (* check) (BobguiFishbowl *fb);
} widget_types[] = {
  { "Icon",       create_icon,           NULL },
  { "Button",     create_button,         NULL },
  { "Blurbutton", create_blurred_button, NULL },
  { "Fontbutton", create_font_button,    NULL },
  { "Levelbar",   create_level_bar,      NULL },
  { "Label",      create_label,          NULL },
  { "Spinner",    create_spinner,        NULL },
  { "Spinbutton", create_spinbutton,     NULL },
  { "Video",      create_video,          NULL },
  { "Gears",      create_gears,          NULL },
  { "Switch",     create_switch,         NULL },
  { "Menubutton", create_menu_button,    NULL },
  { "Tiger",      create_tiger,          NULL },
  { "Graph",      create_graph,          NULL },
  { "Symbolic",   create_symbolic,       NULL },
  { "SVG",        create_svg,            NULL },
};

static int selected_widget_type = -1;
static const int N_WIDGET_TYPES = G_N_ELEMENTS (widget_types);

static gboolean
set_widget_type (BobguiFishbowl *fishbowl,
                 int          widget_type_index)
{
  BobguiWidget *window;

  if (widget_type_index == selected_widget_type)
    return TRUE;

  if (widget_types[widget_type_index].check != NULL &&
      !widget_types[widget_type_index].check (fishbowl))
    return FALSE;

  selected_widget_type = widget_type_index;

  bobgui_fishbowl_set_creation_func (fishbowl,
                                  widget_types[selected_widget_type].create_func);

  window = BOBGUI_WIDGET (bobgui_widget_get_root (BOBGUI_WIDGET (fishbowl)));
  bobgui_window_set_title (BOBGUI_WINDOW (window),
                        widget_types[selected_widget_type].name);

  return TRUE;
}

G_MODULE_EXPORT void
fishbowl_next_button_clicked_cb (BobguiButton *source,
                                 gpointer   user_data)
{
  BobguiFishbowl *fishbowl = user_data;
  int new_index = selected_widget_type;

  do
    {
      if (new_index + 1 >= N_WIDGET_TYPES)
        new_index = 0;
      else
        new_index = new_index + 1;

    }
  while (!set_widget_type (fishbowl, new_index));
}

G_MODULE_EXPORT void
fishbowl_prev_button_clicked_cb (BobguiButton *source,
                                 gpointer   user_data)
{
  BobguiFishbowl *fishbowl = user_data;
  int new_index = selected_widget_type;

  do
    {
      if (new_index - 1 < 0)
        new_index = N_WIDGET_TYPES - 1;
      else
        new_index = new_index - 1;

    }

  while (!set_widget_type (fishbowl, new_index));
}

G_MODULE_EXPORT void
fishbowl_changes_toggled_cb (BobguiToggleButton *button,
                             gpointer         user_data)
{
  GFile *file;
  GdkPaintable *paintable;
  BobguiWidget *image;

  if (bobgui_toggle_button_get_active (button))
    file = g_file_new_for_uri ("resource:///org/bobgui/libbobgui/icons/changes-prevent-symbolic.svg");
  else
    file = g_file_new_for_uri ("resource:///org/bobgui/libbobgui/icons/changes-allow-symbolic.svg");

  paintable = GDK_PAINTABLE (bobgui_icon_paintable_new_for_file (file, 16, 1));
  image = bobgui_button_get_child (BOBGUI_BUTTON (button));
  bobgui_image_set_from_paintable (BOBGUI_IMAGE (image), paintable);
  g_object_unref (paintable);
  g_object_unref (file);
}

G_MODULE_EXPORT char *
format_header_cb (GObject *object,
                  guint    count,
                  double   fps)
{
  return g_strdup_printf ("%u Icons, %.2f fps", count, fps);
}

BobguiWidget *
do_fishbowl (BobguiWidget *do_widget)
{
  static BobguiWidget *window = NULL;
  static BobguiCssProvider *provider = NULL;

  if (provider == NULL)
    {
      provider = bobgui_css_provider_new ();
      bobgui_css_provider_load_from_string (provider, css);
      bobgui_style_context_add_provider_for_display (gdk_display_get_default (),
                                                  BOBGUI_STYLE_PROVIDER (provider),
                                                  BOBGUI_STYLE_PROVIDER_PRIORITY_APPLICATION);
    }

  if (!window)
    {
      BobguiBuilder *builder;
      BobguiBuilderScope *scope;
      BobguiWidget *bowl;

      g_type_ensure (BOBGUI_TYPE_FISHBOWL);

      scope = bobgui_builder_cscope_new ();
      bobgui_builder_cscope_add_callback (BOBGUI_BUILDER_CSCOPE (scope), fishbowl_prev_button_clicked_cb);
      bobgui_builder_cscope_add_callback (BOBGUI_BUILDER_CSCOPE (scope), fishbowl_next_button_clicked_cb);
      bobgui_builder_cscope_add_callback (BOBGUI_BUILDER_CSCOPE (scope), fishbowl_changes_toggled_cb);
      bobgui_builder_cscope_add_callback (BOBGUI_BUILDER_CSCOPE (scope), format_header_cb);

      builder = bobgui_builder_new ();
      bobgui_builder_set_scope (builder, scope);
      bobgui_builder_add_from_resource (builder, "/fishbowl/fishbowl.ui", NULL);
      window = BOBGUI_WIDGET (bobgui_builder_get_object (builder, "window"));
      g_object_add_weak_pointer (G_OBJECT (window), (gpointer *)&window);

      bowl = BOBGUI_WIDGET (bobgui_builder_get_object (builder, "bowl"));
      selected_widget_type = -1;
      set_widget_type (BOBGUI_FISHBOWL (bowl), 0);
      bobgui_window_set_display (BOBGUI_WINDOW (window),
                              bobgui_widget_get_display (do_widget));

      bobgui_widget_realize (window);
      g_object_unref (builder);
      g_object_unref (scope);
    }

  if (!bobgui_widget_get_visible (window))
    bobgui_widget_set_visible (window, TRUE);
  else
    bobgui_window_destroy (BOBGUI_WINDOW (window));

  return window;
}
