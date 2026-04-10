#include <bobgui/bobgui.h>

#define DEMO_TYPE_WIDGET (demo_widget_get_type ())
G_DECLARE_FINAL_TYPE (DemoWidget, demo_widget, DEMO, WIDGET, BobguiWidget)

struct _DemoWidget
{
  BobguiWidget parent_instance;
};

struct _DemoWidgetClass
{
  BobguiWidgetClass parent_class;
};

G_DEFINE_TYPE (DemoWidget, demo_widget, BOBGUI_TYPE_WIDGET)

static void
demo_widget_init (DemoWidget *self)
{
}

static void
demo_widget_snapshot (BobguiWidget   *widget,
                      BobguiSnapshot *snapshot)
{
  DemoWidget *self = DEMO_WIDGET (widget);
  PangoLayout *layout;
  int width, height;
  int pwidth, pheight;
  PangoFontDescription *desc;
  int size;
  double scale;
  int x, y;
  GdkRGBA color;

  width = bobgui_widget_get_width (widget);
  height = bobgui_widget_get_height (widget);

  bobgui_widget_get_color (widget, &color);

  layout = bobgui_widget_create_pango_layout (BOBGUI_WIDGET (self), "Best Aa");

  pango_layout_get_pixel_size (layout, &pwidth, &pheight);
  desc = pango_font_description_copy_static (pango_context_get_font_description (pango_layout_get_context (layout)));
  size = pango_font_description_get_size (desc);

  scale = MIN (width / (double)pwidth, height / (double)pheight);

  pango_font_description_set_size (desc, size * scale * 0.5);
  pango_layout_set_font_description (layout, desc);
  pango_font_description_free (desc);

  pango_layout_get_pixel_size (layout, &pwidth, &pheight);

  x = floor ((width - pwidth) / 2);
  y = floor ((height - pheight) / 2);

  bobgui_snapshot_save (snapshot);

  bobgui_snapshot_translate (snapshot, &GRAPHENE_POINT_INIT (x, y));

  bobgui_snapshot_append_layout (snapshot, layout, &color);

  bobgui_snapshot_restore (snapshot);

  g_object_unref (layout);
}

static void
demo_widget_dispose (GObject *object)
{
  G_OBJECT_CLASS (demo_widget_parent_class)->dispose (object);
}

static void
demo_widget_class_init (DemoWidgetClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (class);

  object_class->dispose = demo_widget_dispose;

  widget_class->snapshot = demo_widget_snapshot;
}

static BobguiWidget *
demo_widget_new (void)
{
  return g_object_new (DEMO_TYPE_WIDGET, NULL);
}

static const char css[] =
  "* {\n"
  "  font-family: Cantarell;\n"
  "  font-weight: 520;\n"
  "}";

int
main (int argc, char *argv[])
{
  BobguiCssProvider *style;
  BobguiWidget *window;

  bobgui_init ();

  style = bobgui_css_provider_new ();
  bobgui_css_provider_load_from_string (style, css);
  bobgui_style_context_add_provider_for_display (gdk_display_get_default (),
                                              BOBGUI_STYLE_PROVIDER (style),
                                              800);

  window = bobgui_window_new ();

  bobgui_window_set_child (BOBGUI_WINDOW (window), demo_widget_new ());

  bobgui_window_present (BOBGUI_WINDOW (window));

  while (g_list_model_get_n_items (bobgui_window_get_toplevels ()) > 0)
    g_main_context_iteration (NULL, TRUE);

  return 0;
}
