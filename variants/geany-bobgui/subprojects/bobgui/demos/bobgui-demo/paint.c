/* Paint
 * #Keywords: GdkDrawingArea, BobguiGesture
 *
 * Demonstrates practical handling of drawing tablets in a real world
 * usecase.
 */
#include <glib/gi18n.h>
#include <bobgui/bobgui.h>

enum {
  COLOR_SET,
  N_SIGNALS
};

static guint area_signals[N_SIGNALS] = { 0, };

typedef struct
{
  BobguiWidget parent_instance;
  cairo_surface_t *surface;
  cairo_t *cr;
  GdkRGBA draw_color;
  BobguiPadController *pad_controller;
  double brush_size;
  BobguiGesture *gesture;
} DrawingArea;

typedef struct
{
  BobguiWidgetClass parent_class;
} DrawingAreaClass;

static BobguiPadActionEntry pad_actions[] = {
  { BOBGUI_PAD_ACTION_BUTTON, 1, -1, N_("Black"), "pad.black" },
  { BOBGUI_PAD_ACTION_BUTTON, 2, -1, N_("Pink"), "pad.pink" },
  { BOBGUI_PAD_ACTION_BUTTON, 3, -1, N_("Green"), "pad.green" },
  { BOBGUI_PAD_ACTION_BUTTON, 4, -1, N_("Red"), "pad.red" },
  { BOBGUI_PAD_ACTION_BUTTON, 5, -1, N_("Purple"), "pad.purple" },
  { BOBGUI_PAD_ACTION_BUTTON, 6, -1, N_("Orange"), "pad.orange" },
  { BOBGUI_PAD_ACTION_STRIP, -1, -1, N_("Brush size"), "pad.brush_size" },
  { BOBGUI_PAD_ACTION_DIAL,  -1, -1, N_("Brush size"), "pad.change_brush_size" },
};

static const char *pad_colors[] = {
  "black",
  "pink",
  "green",
  "red",
  "purple",
  "orange"
};

static GType drawing_area_get_type (void);
G_DEFINE_TYPE (DrawingArea, drawing_area, BOBGUI_TYPE_WIDGET)

static void drawing_area_set_color (DrawingArea   *area,
                                    const GdkRGBA *color);

static void
drawing_area_ensure_surface (DrawingArea *area,
                             int          width,
                             int          height)
{
  if (!area->surface ||
      cairo_image_surface_get_width (area->surface) != width ||
      cairo_image_surface_get_height (area->surface) != height)
    {
      cairo_surface_t *surface;

      surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
                                            width, height);
      if (area->surface)
        {
          cairo_t *cr;

          cr = cairo_create (surface);
          cairo_set_source_surface (cr, area->surface, 0, 0);
          cairo_paint (cr);

          cairo_surface_destroy (area->surface);
          cairo_destroy (area->cr);
          cairo_destroy (cr);
        }

      area->surface = surface;
      area->cr = cairo_create (surface);
    }
}

static void
drawing_area_size_allocate (BobguiWidget *widget,
                            int        width,
                            int        height,
                            int        baseline)
{
  DrawingArea *area = (DrawingArea *) widget;

  drawing_area_ensure_surface (area, width, height);

  BOBGUI_WIDGET_CLASS (drawing_area_parent_class)->size_allocate (widget, width, height, baseline);
}

static void
drawing_area_map (BobguiWidget *widget)
{
  BOBGUI_WIDGET_CLASS (drawing_area_parent_class)->map (widget);

  drawing_area_ensure_surface ((DrawingArea *) widget,
                               bobgui_widget_get_width (widget),
                               bobgui_widget_get_height (widget));
}

static void
drawing_area_unmap (BobguiWidget *widget)
{
  DrawingArea *area = (DrawingArea *) widget;

  g_clear_pointer (&area->cr, cairo_destroy);
  g_clear_pointer (&area->surface, cairo_surface_destroy);

  BOBGUI_WIDGET_CLASS (drawing_area_parent_class)->unmap (widget);
}

static void
drawing_area_snapshot (BobguiWidget   *widget,
                       BobguiSnapshot *snapshot)
{
  DrawingArea *area = (DrawingArea *) widget;
  int width, height;
  cairo_t *cr;

  width = bobgui_widget_get_width (widget);
  height = bobgui_widget_get_height (widget);

  cr = bobgui_snapshot_append_cairo (snapshot, &GRAPHENE_RECT_INIT (0, 0, width, height));

  cairo_set_source_rgb (cr, 1, 1, 1);
  cairo_paint (cr);

  cairo_set_source_surface (cr, area->surface, 0, 0);
  cairo_paint (cr);

  cairo_set_source_rgb (cr, 0.6, 0.6, 0.6);
  cairo_rectangle (cr, 0, 0, width, height);
  cairo_stroke (cr);

  cairo_destroy (cr);
}

static void
on_pad_button_activate (GSimpleAction *action,
                        GVariant      *parameter,
                        DrawingArea   *area)
{
  const char *color = g_object_get_data (G_OBJECT (action), "color");
  GdkRGBA rgba;

  gdk_rgba_parse (&rgba, color);
  drawing_area_set_color (area, &rgba);
}

static void
on_pad_knob_change (GSimpleAction *action,
                    GVariant      *parameter,
                    DrawingArea   *area)
{
  double value = g_variant_get_double (parameter);

  area->brush_size = value;
}

static void
on_pad_dial_change (GSimpleAction *action,
                    GVariant      *parameter,
                    DrawingArea   *area)
{
  double value = g_variant_get_double (parameter);

  area->brush_size += value / 120.0;
}

static void
drawing_area_unroot (BobguiWidget *widget)
{
  DrawingArea *area = (DrawingArea *) widget;
  BobguiWidget *toplevel;

  toplevel = BOBGUI_WIDGET (bobgui_widget_get_root (widget));

  if (area->pad_controller)
    {
      bobgui_widget_remove_controller (toplevel, BOBGUI_EVENT_CONTROLLER (area->pad_controller));
      area->pad_controller = NULL;
    }

  BOBGUI_WIDGET_CLASS (drawing_area_parent_class)->unroot (widget);
}

static void
drawing_area_root (BobguiWidget *widget)
{
  DrawingArea *area = (DrawingArea *) widget;
  GSimpleActionGroup *action_group;
  GSimpleAction *action;
  BobguiWidget *toplevel;
  int i;

  BOBGUI_WIDGET_CLASS (drawing_area_parent_class)->root (widget);

  toplevel = BOBGUI_WIDGET (bobgui_widget_get_root (BOBGUI_WIDGET (area)));

  action_group = g_simple_action_group_new ();
  area->pad_controller = bobgui_pad_controller_new (G_ACTION_GROUP (action_group), NULL);

  for (i = 0; i < G_N_ELEMENTS (pad_actions); i++)
    {
      if (pad_actions[i].type == BOBGUI_PAD_ACTION_BUTTON)
        {
          action = g_simple_action_new (pad_actions[i].action_name, NULL);
          g_object_set_data (G_OBJECT (action), "color",
                             (gpointer) pad_colors[i]);
          g_signal_connect (action, "activate",
                            G_CALLBACK (on_pad_button_activate), area);
        }
      else if (pad_actions[i].type == BOBGUI_PAD_ACTION_DIAL)
        {
          action = g_simple_action_new_stateful (pad_actions[i].action_name,
                                                 G_VARIANT_TYPE_DOUBLE, NULL);
          g_signal_connect (action, "activate",
                            G_CALLBACK (on_pad_dial_change), area);
        }
      else
        {
          action = g_simple_action_new_stateful (pad_actions[i].action_name,
                                                 G_VARIANT_TYPE_DOUBLE, NULL);
          g_signal_connect (action, "activate",
                            G_CALLBACK (on_pad_knob_change), area);
        }

      g_action_map_add_action (G_ACTION_MAP (action_group), G_ACTION (action));
      g_object_unref (action);
    }

  bobgui_pad_controller_set_action_entries (area->pad_controller, pad_actions,
                                         G_N_ELEMENTS (pad_actions));

  bobgui_widget_add_controller (toplevel, BOBGUI_EVENT_CONTROLLER (area->pad_controller));
}

static void
drawing_area_class_init (DrawingAreaClass *klass)
{
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (klass);

  widget_class->size_allocate = drawing_area_size_allocate;
  widget_class->snapshot = drawing_area_snapshot;
  widget_class->map = drawing_area_map;
  widget_class->unmap = drawing_area_unmap;
  widget_class->root = drawing_area_root;
  widget_class->unroot = drawing_area_unroot;

  area_signals[COLOR_SET] =
    g_signal_new ("color-set",
                  G_TYPE_FROM_CLASS (widget_class),
                  G_SIGNAL_RUN_FIRST,
                  0, NULL, NULL, NULL,
                  G_TYPE_NONE, 1, GDK_TYPE_RGBA);
}

static void
drawing_area_apply_stroke (DrawingArea   *area,
                           GdkDeviceTool *tool,
                           double         x,
                           double         y,
                           double         pressure)
{
  if (tool && gdk_device_tool_get_tool_type (tool) == GDK_DEVICE_TOOL_TYPE_ERASER)
    {
      cairo_set_line_width (area->cr, 10 * pressure * area->brush_size);
      cairo_set_operator (area->cr, CAIRO_OPERATOR_DEST_OUT);
    }
  else
    {
      cairo_set_line_width (area->cr, 4 * pressure * area->brush_size);
      cairo_set_operator (area->cr, CAIRO_OPERATOR_SATURATE);
    }

  cairo_set_source_rgba (area->cr, area->draw_color.red,
                         area->draw_color.green, area->draw_color.blue,
                         area->draw_color.alpha * pressure);

  cairo_line_to (area->cr, x, y);
  cairo_stroke (area->cr);
  cairo_move_to (area->cr, x, y);
}

static void
stylus_gesture_down (BobguiGestureStylus *gesture,
                     double            x,
                     double            y,
                     DrawingArea      *area)
{
  cairo_new_path (area->cr);
}

static void
stylus_gesture_motion (BobguiGestureStylus *gesture,
                       double            x,
                       double            y,
                       DrawingArea      *area)
{
  GdkTimeCoord *backlog;
  GdkDeviceTool *tool;
  double pressure;
  guint n_items;

  tool = bobgui_gesture_stylus_get_device_tool (gesture);

  if (bobgui_gesture_stylus_get_backlog (gesture, &backlog, &n_items))
    {
      guint i;

      for (i = 0; i < n_items; i++)
        {
          drawing_area_apply_stroke (area, tool,
                                     backlog[i].axes[GDK_AXIS_X],
                                     backlog[i].axes[GDK_AXIS_Y],
                                     backlog[i].flags & GDK_AXIS_FLAG_PRESSURE
                                        ? backlog[i].axes[GDK_AXIS_PRESSURE]
                                        : 1);
        }

      g_free (backlog);
    }
  else
    {
      if (!bobgui_gesture_stylus_get_axis (gesture, GDK_AXIS_PRESSURE, &pressure))
        pressure = 1;

      drawing_area_apply_stroke (area, tool, x, y, pressure);
    }

  bobgui_widget_queue_draw (BOBGUI_WIDGET (area));
}

static void
drawing_area_init (DrawingArea *area)
{
  BobguiGesture *gesture;

  gesture = bobgui_gesture_stylus_new ();
  g_signal_connect (gesture, "down",
                    G_CALLBACK (stylus_gesture_down), area);
  g_signal_connect (gesture, "motion",
                    G_CALLBACK (stylus_gesture_motion), area);
  bobgui_widget_add_controller (BOBGUI_WIDGET (area), BOBGUI_EVENT_CONTROLLER (gesture));

  area->draw_color = (GdkRGBA) { 0, 0, 0, 1 };
  area->brush_size = 1;

  area->gesture = gesture;
}

static BobguiWidget *
drawing_area_new (void)
{
  return g_object_new (drawing_area_get_type (), NULL);
}

static void
drawing_area_set_color (DrawingArea   *area,
                        const GdkRGBA *color)
{
  if (gdk_rgba_equal (&area->draw_color, color))
    return;

  area->draw_color = *color;
  g_signal_emit (area, area_signals[COLOR_SET], 0, &area->draw_color);
}

static void
color_button_color_set (BobguiColorDialogButton *button,
                        GParamSpec           *pspec,
                        DrawingArea          *draw_area)
{
  const GdkRGBA *color;

  color = bobgui_color_dialog_button_get_rgba (button);
  drawing_area_set_color (draw_area, color);
}

static void
drawing_area_color_set (DrawingArea          *area,
                        GdkRGBA              *color,
                        BobguiColorDialogButton *button)
{
  bobgui_color_dialog_button_set_rgba (button, color);
}

static void
drawing_area_clear (BobguiButton   *button,
                    DrawingArea *area)
{
  int width, height;

  g_clear_pointer (&area->cr, cairo_destroy);
  g_clear_pointer (&area->surface, cairo_surface_destroy);

  width = bobgui_widget_get_width (BOBGUI_WIDGET (area));
  height = bobgui_widget_get_height (BOBGUI_WIDGET (area));

  drawing_area_ensure_surface (area, width, height);
  bobgui_widget_queue_draw (BOBGUI_WIDGET (area));
}

static BobguiGesture *
drawing_area_get_gesture (DrawingArea *area)
{
  return area->gesture;
}

BobguiWidget *
do_paint (BobguiWidget *toplevel)
{
  static BobguiWidget *window = NULL;

  if (!window)
    {
      BobguiWidget *draw_area, *headerbar, *button;

      window = bobgui_window_new ();

      draw_area = drawing_area_new ();
      bobgui_window_set_child (BOBGUI_WINDOW (window), draw_area);

      headerbar = bobgui_header_bar_new ();

      button = bobgui_color_dialog_button_new (bobgui_color_dialog_new ());
      g_signal_connect (button, "notify::rgba",
                        G_CALLBACK (color_button_color_set), draw_area);
      g_signal_connect (draw_area, "color-set",
                        G_CALLBACK (drawing_area_color_set), button);
      bobgui_color_dialog_button_set_rgba (BOBGUI_COLOR_DIALOG_BUTTON (button),
                                        &(GdkRGBA) { 0, 0, 0, 1 });

      bobgui_header_bar_pack_end (BOBGUI_HEADER_BAR (headerbar), button);

      button = bobgui_button_new_from_icon_name ("view-refresh-symbolic");
      g_signal_connect (button, "clicked",
                        G_CALLBACK (drawing_area_clear), draw_area);

      bobgui_header_bar_pack_end (BOBGUI_HEADER_BAR (headerbar), button);

      button = bobgui_check_button_new_with_label ("Stylus only");
      g_object_bind_property (button, "active",
                              drawing_area_get_gesture ((DrawingArea *)draw_area), "stylus-only",
                              G_BINDING_SYNC_CREATE);
      bobgui_header_bar_pack_start (BOBGUI_HEADER_BAR (headerbar), button);

      bobgui_window_set_titlebar (BOBGUI_WINDOW (window), headerbar);
      bobgui_window_set_title (BOBGUI_WINDOW (window), "Paint");
      g_object_add_weak_pointer (G_OBJECT (window), (gpointer *)&window);
    }

  if (!bobgui_widget_get_visible (window))
    bobgui_widget_set_visible (window, TRUE);
  else
    bobgui_window_destroy (BOBGUI_WINDOW (window));

  return window;
}
