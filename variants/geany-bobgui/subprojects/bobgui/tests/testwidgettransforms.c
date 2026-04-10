

#include <bobgui/bobgui.h>

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

static const char *css =
"test>button {"
"  all: unset; "
"  background-color: white;"
"  border: 30px solid teal;"
"  margin: 40px;"
"  padding: 40px;"
"}"
"test>button:hover {"
"  background-color: blue;"
"}"
"test image {"
"  background-color: purple;"
"}"
;

/* Just so we can avoid a signal */
BobguiWidget *transform_tester;
BobguiWidget *test_widget;
BobguiWidget *test_child;
float scale = 1;
gboolean do_picking = TRUE;

static const GdkRGBA RED   = {1, 0, 0, 0.4};
static const GdkRGBA GREEN = {0, 1, 0, 0.7};
static const GdkRGBA BLUE  = {0, 0, 1, 0.4};
static const GdkRGBA BLACK = {0, 0, 0, 1  };



/* ######################################################################### */
/* ############################## MatrixChooser ############################ */
/* ######################################################################### */


#define BOBGUI_TYPE_MATRIX_CHOOSER (bobgui_matrix_chooser_get_type ())
G_DECLARE_FINAL_TYPE (BobguiMatrixChooser, bobgui_matrix_chooser, BOBGUI, MATRIX_CHOOSER, BobguiWidget)

struct _BobguiMatrixChooser
{
  BobguiWidget parent_instance;
};

G_DEFINE_TYPE (BobguiMatrixChooser, bobgui_matrix_chooser, BOBGUI_TYPE_WIDGET)

static void
bobgui_matrix_chooser_init (BobguiMatrixChooser *self)
{
}

static void
bobgui_matrix_chooser_class_init (BobguiMatrixChooserClass *klass)
{

}


/* ######################################################################### */
/* ############################# TransformTester ########################### */
/* ######################################################################### */

#define TEST_WIDGET_MIN_SIZE 100

#define BOBGUI_TYPE_TRANSFORM_TESTER (bobgui_transform_tester_get_type ())
G_DECLARE_FINAL_TYPE (BobguiTransformTester, bobgui_transform_tester, BOBGUI, TRANSFORM_TESTER, BobguiWidget);

struct _BobguiTransformTester
{
  BobguiWidget parent_instance;

  BobguiWidget *test_widget;
  int pick_increase;
};

G_DEFINE_TYPE (BobguiTransformTester, bobgui_transform_tester, BOBGUI_TYPE_WIDGET);

static void
bobgui_transform_tester_measure (BobguiWidget      *widget,
                              BobguiOrientation  orientation,
                              int             for_size,
                              int            *minimum,
                              int            *natural,
                              int            *minimum_baseline,
                              int            *natural_baseline)
{
  BobguiTransformTester *self = (BobguiTransformTester *)widget;

  if (self->test_widget)
    {
      bobgui_widget_measure (self->test_widget, orientation, for_size,
                          minimum, natural, NULL, NULL);
    }
}

static void
bobgui_transform_tester_size_allocate (BobguiWidget  *widget,
                                    int         width,
                                    int         height,
                                    int         baseline)
{
  BobguiTransformTester *self = (BobguiTransformTester *)widget;
  GskTransform *global_transform;
  int w, h;

  if (!self->test_widget)
    return;

  scale += 2.5f;

  bobgui_widget_measure (self->test_widget, BOBGUI_ORIENTATION_HORIZONTAL, -1,
                      &w, NULL, NULL, NULL);
  bobgui_widget_measure (self->test_widget, BOBGUI_ORIENTATION_VERTICAL, w,
                      &h, NULL, NULL, NULL);

  g_message ("%s: %d, %d", __FUNCTION__, w, h);

  global_transform = NULL;

  global_transform = gsk_transform_translate (global_transform, &GRAPHENE_POINT_INIT (width / 2.0f, height / 2.0f));
  global_transform = gsk_transform_rotate (global_transform, scale);
  global_transform = gsk_transform_translate (global_transform, &GRAPHENE_POINT_INIT (-w / 2.0f, -h / 2.0f));

  bobgui_widget_allocate (self->test_widget,
                       w, h,
                       -1,
                       global_transform);
}

static void
bobgui_transform_tester_snapshot (BobguiWidget   *widget,
                               BobguiSnapshot *snapshot)
{
  BobguiTransformTester *self = (BobguiTransformTester *)widget;
  const int width = bobgui_widget_get_width (widget);
  const int height = bobgui_widget_get_height (widget);
  const int inc = self->pick_increase;
  graphene_rect_t child_bounds;
  graphene_rect_t self_bounds;
  int x, y;

  BOBGUI_WIDGET_CLASS (bobgui_transform_tester_parent_class)->snapshot (widget, snapshot);

  if (!do_picking ||
      !bobgui_widget_compute_bounds (self->test_widget, widget, &child_bounds) ||
      !bobgui_widget_compute_bounds (self->test_widget, self->test_widget, &self_bounds))
    return;

  {
    const struct {
      graphene_point_t coords;
      GdkRGBA color;
    } points[4] = {
      { self_bounds.origin, {1, 0, 0, 1} },
      { GRAPHENE_POINT_INIT (self_bounds.origin.x + self_bounds.size.width, self_bounds.origin.y), {0, 1, 0, 1} },
      { GRAPHENE_POINT_INIT (self_bounds.origin.x + self_bounds.size.width, self_bounds.origin.y + self_bounds.size.height), {0, 0, 1, 1} },
      { GRAPHENE_POINT_INIT (self_bounds.origin.x, self_bounds.origin.y + self_bounds.size.height), {1, 0, 1, 1} }
    };

    for (x = 0; x < G_N_ELEMENTS (points); x ++)
      {
        double px, py;

        bobgui_widget_translate_coordinates (self->test_widget, widget,
                                          points[x].coords.x, points[x].coords.y,
                                          &px, &py);

        bobgui_snapshot_append_color (snapshot, &points[x].color,
                                   &GRAPHENE_RECT_INIT (px, py,
                                                        4,
                                                        4));
      }
  }

  /* Now add custom drawing */
  for (x = 0; x < width; x += inc)
    {
      for (y = 0; y < height; y += inc)
        {
          const float px = x;
          const float py = y;
          BobguiWidget *picked;
#if 1
          picked = bobgui_widget_pick (widget, px, py, BOBGUI_PICK_DEFAULT);
#else
          {
            int dx, dy;
            bobgui_widget_translate_coordinates (widget, self->test_widget, px, py, &dx, &dy);
            picked = bobgui_widget_pick (self->test_widget, dx, dy, BOBGUI_PICK_DEFAULT);
          }
#endif

          if (picked == self->test_widget)
            bobgui_snapshot_append_color (snapshot, &GREEN,
                                       &GRAPHENE_RECT_INIT (px - (inc / 2), py - (inc / 2), inc, inc));
          else if (picked == test_child)
            bobgui_snapshot_append_color (snapshot, &BLUE,
                                       &GRAPHENE_RECT_INIT (px - (inc / 2), py - (inc / 2), inc, inc));

          else
            bobgui_snapshot_append_color (snapshot, &RED,
                                       &GRAPHENE_RECT_INIT (px - (inc / 2), py - (inc / 2), inc, inc));
        }
    }

  bobgui_snapshot_append_color (snapshot, &BLACK,
                             &GRAPHENE_RECT_INIT (child_bounds.origin.x,
                                                  child_bounds.origin.y,
                                                  child_bounds.size.width,
                                                  1));

  bobgui_snapshot_append_color (snapshot, &BLACK,
                             &GRAPHENE_RECT_INIT (child_bounds.origin.x + child_bounds.size.width,
                                                  child_bounds.origin.y,
                                                  1,
                                                  child_bounds.size.height));

  bobgui_snapshot_append_color (snapshot, &BLACK,
                             &GRAPHENE_RECT_INIT (child_bounds.origin.x,
                                                  child_bounds.origin.y + child_bounds.size.height,
                                                  child_bounds.size.width,
                                                  1));

  bobgui_snapshot_append_color (snapshot, &BLACK,
                             &GRAPHENE_RECT_INIT (child_bounds.origin.x,
                                                  child_bounds.origin.y,
                                                  1,
                                                  child_bounds.size.height));
}

static void
bobgui_transform_tester_init (BobguiTransformTester *self)
{
  self->pick_increase = 4;
}

static void
bobgui_transform_tester_class_init (BobguiTransformTesterClass *klass)
{
  BobguiWidgetClass *widget_class = (BobguiWidgetClass *)klass;

  widget_class->measure = bobgui_transform_tester_measure;
  widget_class->size_allocate = bobgui_transform_tester_size_allocate;
  widget_class->snapshot = bobgui_transform_tester_snapshot;

  bobgui_widget_class_set_css_name (widget_class, "test");
}

static gboolean
tick_cb (BobguiWidget     *widget,
         GdkFrameClock *frame_clock,
         gpointer       user_data)
{
  bobgui_widget_queue_allocate (widget);

  return G_SOURCE_CONTINUE;
}

static void
bobgui_transform_tester_set_test_widget (BobguiTransformTester *self,
                                      BobguiWidget          *widget)
{
  g_assert (!self->test_widget);

  self->test_widget = widget;
  bobgui_widget_set_parent (widget, (BobguiWidget *)self);

  bobgui_widget_add_tick_callback (BOBGUI_WIDGET (self), tick_cb, NULL, NULL);
}

static void
toggled_cb (BobguiToggleButton *source,
            gpointer         user_data)
{
  do_picking = bobgui_toggle_button_get_active (source);
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
main (int argc, char **argv)
{
  BobguiWidget *window;
  BobguiWidget *matrix_chooser;
  BobguiWidget *box;
  BobguiWidget *titlebar;
  BobguiWidget *toggle_button;
  BobguiCssProvider *provider;
  gboolean done = FALSE;

  bobgui_init ();

  provider = bobgui_css_provider_new ();
  bobgui_css_provider_load_from_data (provider, css, -1);
  bobgui_style_context_add_provider_for_display (gdk_display_get_default (),
                                              BOBGUI_STYLE_PROVIDER (provider),
                                              BOBGUI_STYLE_PROVIDER_PRIORITY_APPLICATION);

  window = bobgui_window_new ();
  matrix_chooser = g_object_new (BOBGUI_TYPE_MATRIX_CHOOSER, NULL);
  transform_tester = g_object_new (BOBGUI_TYPE_TRANSFORM_TESTER, NULL);
  box = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 12);
  titlebar = bobgui_header_bar_new ();

  bobgui_window_set_titlebar (BOBGUI_WINDOW (window), titlebar);

  toggle_button = bobgui_toggle_button_new ();
  bobgui_button_set_label (BOBGUI_BUTTON (toggle_button), "Picking");
  bobgui_toggle_button_set_active (BOBGUI_TOGGLE_BUTTON (toggle_button), do_picking);
  g_signal_connect (toggle_button, "toggled", G_CALLBACK (toggled_cb), NULL);
  bobgui_header_bar_pack_start (BOBGUI_HEADER_BAR (titlebar), toggle_button);

  test_widget = bobgui_button_new ();
  bobgui_widget_set_size_request (test_widget, TEST_WIDGET_MIN_SIZE, TEST_WIDGET_MIN_SIZE);
  bobgui_widget_set_halign (test_widget, BOBGUI_ALIGN_CENTER);
  bobgui_widget_set_valign (test_widget, BOBGUI_ALIGN_CENTER);


  test_child = bobgui_image_new_from_icon_name ("weather-clear");
  bobgui_widget_set_halign (test_child, BOBGUI_ALIGN_CENTER);
  bobgui_widget_set_valign (test_child, BOBGUI_ALIGN_CENTER);
  bobgui_widget_set_size_request (test_child, TEST_WIDGET_MIN_SIZE / 2, TEST_WIDGET_MIN_SIZE / 2);
  bobgui_button_set_child (BOBGUI_BUTTON (test_widget), test_child);


  bobgui_transform_tester_set_test_widget (BOBGUI_TRANSFORM_TESTER (transform_tester), test_widget);

  bobgui_widget_set_vexpand (transform_tester, TRUE);
  bobgui_box_append (BOBGUI_BOX (box), transform_tester);
  bobgui_box_append (BOBGUI_BOX (box), matrix_chooser);
  bobgui_window_set_child (BOBGUI_WINDOW (window), box);

  bobgui_window_set_default_size ((BobguiWindow *)window, 200, 200);
  g_signal_connect (window, "close-request", G_CALLBACK (quit_cb), &done);
  bobgui_window_present (BOBGUI_WINDOW (window));

  while (!done)
    g_main_context_iteration (NULL, TRUE);

  return 0;
}
