/* Fixed Layout / Cube
 * #Keywords: BobguiLayoutManager
 *
 * BobguiFixed is a container that allows placing and transforming
 * widgets manually.
 *
 * This demo uses a BobguiFixed to create a cube out of child widgets.
 */

#include <bobgui/bobgui.h>

/* This enumeration determines the paint order */
enum {
  FACE_BACK,
  FACE_LEFT,
  FACE_BOTTOM,
  FACE_RIGHT,
  FACE_TOP,
  FACE_FRONT,

  N_FACES
};

/* Map face widgets to CSS classes */
static struct {
  BobguiWidget *face;
  const char *css_class;
} faces[N_FACES] = {
  [FACE_BACK] = { NULL, "back", },
  [FACE_LEFT] = { NULL, "left", },
  [FACE_RIGHT] = { NULL, "right", },
  [FACE_TOP] = { NULL, "top", },
  [FACE_BOTTOM] = { NULL, "bottom", },
  [FACE_FRONT] = { NULL, "front", },
};

static BobguiWidget *
create_faces (void)
{
  BobguiWidget *fixed = bobgui_fixed_new ();
  int face_size = 200;
  float w, h, d, p;

  bobgui_widget_set_overflow (fixed, BOBGUI_OVERFLOW_VISIBLE);

  w = (float) face_size / 2.f;
  h = (float) face_size / 2.f;
  d = (float) face_size / 2.f;
  p = face_size * 3.f;

  for (int i = 0; i < N_FACES; i++)
    {
      GskTransform *transform = NULL;

      /* Add a face */
      faces[i].face = bobgui_frame_new (NULL);
      bobgui_widget_set_size_request (faces[i].face, face_size, face_size);
      bobgui_widget_add_css_class (faces[i].face, faces[i].css_class);
      bobgui_fixed_put (BOBGUI_FIXED (fixed), faces[i].face, 0, 0);

      /* Set up the transformation for each face */
      transform = gsk_transform_translate (transform, &GRAPHENE_POINT_INIT (w, h));
      transform = gsk_transform_perspective (transform, p);
      transform = gsk_transform_rotate_3d (transform, -30.f, graphene_vec3_x_axis ());
      transform = gsk_transform_rotate_3d (transform, 135.f, graphene_vec3_y_axis ());
      transform = gsk_transform_translate_3d (transform, &GRAPHENE_POINT3D_INIT (0, 0, -face_size / 6.f));

      switch (i)
        {
        case FACE_FRONT:
          transform = gsk_transform_rotate_3d (transform, 0.f, graphene_vec3_y_axis ());
          break;

        case FACE_BACK:
          transform = gsk_transform_rotate_3d (transform, -180.f, graphene_vec3_y_axis ());
          break;

        case FACE_RIGHT:
          transform = gsk_transform_rotate_3d (transform, 90.f, graphene_vec3_y_axis ());
          break;

        case FACE_LEFT:
          transform = gsk_transform_rotate_3d (transform, -90.f, graphene_vec3_y_axis ());
          break;

        case FACE_TOP:
          transform = gsk_transform_rotate_3d (transform, 90.f, graphene_vec3_x_axis ());
          break;

        case FACE_BOTTOM:
          transform = gsk_transform_rotate_3d (transform, -90.f, graphene_vec3_x_axis ());
          break;

        default:
          break;
        }

      transform = gsk_transform_translate_3d (transform, &GRAPHENE_POINT3D_INIT (0, 0, d));
      transform = gsk_transform_translate_3d (transform, &GRAPHENE_POINT3D_INIT (-w, -h, 0));

      bobgui_fixed_set_child_transform (BOBGUI_FIXED (fixed), faces[i].face, transform);
      gsk_transform_unref (transform);
    }

  return fixed;
}

static BobguiWidget *demo_window = NULL;
static BobguiCssProvider *provider = NULL;

static void
close_window (BobguiWidget *widget)
{
  /* Reset the state */
  for (int i = 0; i < N_FACES; i++)
    faces[i].face = NULL;

  bobgui_style_context_remove_provider_for_display (gdk_display_get_default (),
                                                 BOBGUI_STYLE_PROVIDER (provider));
  provider = NULL;

  demo_window = NULL;
}

static BobguiWidget *
create_demo_window (BobguiWidget *do_widget)
{
  BobguiWidget *window, *sw, *fixed, *cube;

  window = bobgui_window_new ();
  bobgui_window_set_display (BOBGUI_WINDOW (window),  bobgui_widget_get_display (do_widget));
  bobgui_window_set_title (BOBGUI_WINDOW (window), "Fixed Layout ‐ Cube");
  bobgui_window_set_default_size (BOBGUI_WINDOW (window), 600, 400);
  g_signal_connect (window, "destroy", G_CALLBACK (close_window), NULL);

  sw = bobgui_scrolled_window_new ();
  bobgui_window_set_child (BOBGUI_WINDOW (window), sw);

  fixed = bobgui_fixed_new ();
  bobgui_scrolled_window_set_child (BOBGUI_SCROLLED_WINDOW (sw), fixed);
  bobgui_widget_set_halign (BOBGUI_WIDGET (fixed), BOBGUI_ALIGN_CENTER);
  bobgui_widget_set_valign (BOBGUI_WIDGET (fixed), BOBGUI_ALIGN_CENTER);

  cube = create_faces ();
  bobgui_fixed_put (BOBGUI_FIXED (fixed), cube, 0, 0);
  bobgui_widget_set_overflow (fixed, BOBGUI_OVERFLOW_VISIBLE);

  provider = bobgui_css_provider_new ();
  bobgui_css_provider_load_from_resource (provider, "/fixed/fixed.css");
  bobgui_style_context_add_provider_for_display (gdk_display_get_default (),
                                              BOBGUI_STYLE_PROVIDER (provider),
                                              800);
  g_object_unref (provider);

  return window;
}

BobguiWidget*
do_fixed (BobguiWidget *do_widget)
{
  if (demo_window == NULL)
    demo_window = create_demo_window (do_widget);

  if (!bobgui_widget_get_visible (demo_window))
    bobgui_widget_set_visible (demo_window, TRUE);
  else
    bobgui_window_destroy (BOBGUI_WINDOW (demo_window));

  return demo_window;
}
