#include "bobguicssprovider.h"
#include <bobgui/bobgui.h>

static BobguiRequisition size;
static gint64 start_time;
static gboolean stop_update_size;

// Animated paintable for testing content-invalidating drag surfaces.
#define BOBGUI_TYPE_ANIMATED_ICON (bobgui_animated_icon_get_type ())
G_DECLARE_FINAL_TYPE (BobguiAnimatedIcon, bobgui_animated_icon, BOBGUI, ANIMATED_ICON, GObject)

struct _BobguiAnimatedIcon
{
  GObject parent_instance;
};

struct _BobguiAnimatedIconClass
{
  GObjectClass parent_class;
};

static void
bobgui_animated_icon_snapshot (GdkPaintable *paintable,
                            GdkSnapshot  *snapshot,
                            double        width,
                            double        height)
{
  gint64 now = g_get_monotonic_time ();
  float t;

  t = fmodf ((now - start_time) / (float) G_TIME_SPAN_SECOND, 1);
  if (t >= 0.5)
    t = 1 - t;

  bobgui_snapshot_append_color (snapshot,
                             &(GdkRGBA) { 0, t + 0.5, 0, 1 },
                             &GRAPHENE_RECT_INIT (0, 0, width, height));
}

static void
bobgui_animated_icon_paintable_init (GdkPaintableInterface *iface)
{
  iface->snapshot = bobgui_animated_icon_snapshot;
}

G_DEFINE_TYPE_WITH_CODE (BobguiAnimatedIcon, bobgui_animated_icon, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (GDK_TYPE_PAINTABLE,
                                                bobgui_animated_icon_paintable_init))

static void
bobgui_animated_icon_class_init (BobguiAnimatedIconClass *klass)
{
}

static void
bobgui_animated_icon_init (BobguiAnimatedIcon *nuclear)
{
}

static gboolean
update_size (BobguiWidget *widget, GdkFrameClock *clock, gpointer data)
{
  GdkDrag *drag = data;
  gint64 now = g_get_monotonic_time ();
  float t;
  int width, height;

  if (stop_update_size)
    return G_SOURCE_REMOVE;

  t = fmodf ((now - start_time) / (float) G_TIME_SPAN_SECOND, 1);
  if (t >= 0.5)
    t = 1 - t;

  width = size.width + t * 300;
  height = size.height + t * 150;

  bobgui_widget_set_size_request (widget, width, height);
  gdk_drag_set_hotspot (drag, width / 2, height / 2);

  return G_SOURCE_CONTINUE;
}

static void
drag_begin (BobguiDragSource *source,
            GdkDrag       *drag)
{
  BobguiWidget *widget;
  BobguiWidget *icon;

  icon = bobgui_drag_icon_get_for_drag (drag);

  widget = bobgui_label_new ("This Should Resize\n\nAnd Stay Centered");
  bobgui_widget_add_css_class (widget, "dnd");
  bobgui_widget_get_preferred_size (widget, NULL, &size);

  bobgui_drag_icon_set_child (BOBGUI_DRAG_ICON (icon), widget);
  bobgui_widget_set_size_request (widget, size.width, size.height);
  gdk_drag_set_hotspot (drag, size.width / 2, size.height / 2);

  start_time = g_get_monotonic_time ();
  stop_update_size = FALSE;
  bobgui_widget_add_tick_callback (widget, update_size, drag, NULL);
}

static void
drag_end (BobguiDragSource *source,
          GdkDrag       *drag,
          gboolean       delete_data,
          gboolean       data)
{
  stop_update_size = TRUE;
}

static gboolean
invalidate_contents (BobguiWidget *widget, GdkFrameClock *clock, gpointer data)
{
  GdkPaintable *paintable = data;
  gdk_paintable_invalidate_contents (paintable);
  return G_SOURCE_CONTINUE;
}

static void
drag_begin_non_resizing (BobguiDragSource *source,
                         GdkDrag       *drag)
{
  GdkPaintable *paintable;
  BobguiWidget *widget;
  int width = 64, height = 32;

  paintable = GDK_PAINTABLE (g_object_new (BOBGUI_TYPE_ANIMATED_ICON, NULL));
  bobgui_drag_icon_set_from_paintable (drag, paintable, width / 2, height / 2);

  widget = bobgui_drag_icon_get_child (BOBGUI_DRAG_ICON (bobgui_drag_icon_get_for_drag (drag)));
  bobgui_widget_set_size_request (widget, width, height);
  bobgui_widget_add_tick_callback (widget, invalidate_contents, paintable, NULL);
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
  BobguiCssProvider *provider;
  BobguiWidget *window;
  BobguiWidget *box;
  BobguiWidget *label;
  BobguiDragSource *source;
  GdkContentProvider *content;
  gboolean done = FALSE;

  bobgui_init ();

  provider = bobgui_css_provider_new ();
  bobgui_css_provider_load_from_string (provider,
                                     ".dnd {"
                                     "background-color: red;"
                                     "border-top: 10px solid rebeccapurple;"
                                     "}");
  bobgui_style_context_add_provider_for_display (gdk_display_get_default (),
                                              BOBGUI_STYLE_PROVIDER(provider),
                                              BOBGUI_STYLE_PROVIDER_PRIORITY_APPLICATION);

  window = bobgui_window_new ();
  g_signal_connect (window, "destroy", G_CALLBACK (quit_cb), &done);

  box = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 0);

  // The resizing icon label.
  label = bobgui_label_new ("Drag Me (Resizing)");
  g_object_set (label,
                "margin-start", 64,
                "margin-end", 64,
                "margin-top", 64,
                "margin-bottom", 64,
                NULL);

  source = bobgui_drag_source_new ();
  content = gdk_content_provider_new_typed (G_TYPE_STRING, "I'm data!");
  bobgui_drag_source_set_content (source, content);
  g_signal_connect (source, "drag-begin", G_CALLBACK (drag_begin), NULL);
  g_signal_connect (source, "drag-end", G_CALLBACK (drag_end), NULL);
  bobgui_widget_add_controller (label, BOBGUI_EVENT_CONTROLLER (source));

  bobgui_box_append (BOBGUI_BOX (box), label);

  // The non-resizing icon label.
  label = bobgui_label_new ("Drag Me (Non-Resizing)");
  g_object_set (label,
                "margin-start", 64,
                "margin-end", 64,
                "margin-top", 64,
                "margin-bottom", 64,
                NULL);

  source = bobgui_drag_source_new ();
  content = gdk_content_provider_new_typed (G_TYPE_STRING, "I'm data!");
  bobgui_drag_source_set_content (source, content);
  g_signal_connect (source, "drag-begin", G_CALLBACK (drag_begin_non_resizing), NULL);
  bobgui_widget_add_controller (label, BOBGUI_EVENT_CONTROLLER (source));

  bobgui_box_append (BOBGUI_BOX (box), label);

  bobgui_window_set_child (BOBGUI_WINDOW (window), box);

  bobgui_window_present (BOBGUI_WINDOW (window));

  while (!done)
    g_main_context_iteration (NULL, TRUE);

  return 0;
}
