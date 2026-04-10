#include <bobgui/bobgui.h>



typedef struct _BobguiTextureView      BobguiTextureView;
typedef struct _BobguiTextureViewClass BobguiTextureViewClass;

#define BOBGUI_TYPE_TEXTURE_VIEW           (bobgui_texture_view_get_type ())
#define BOBGUI_TEXTURE_VIEW(obj)           (G_TYPE_CHECK_INSTANCE_CAST(obj, BOBGUI_TYPE_TEXTURE_VIEW, BobguiTextureView))
#define BOBGUI_TEXTURE_VIEW_CLASS(cls)     (G_TYPE_CHECK_CLASS_CAST(cls, BOBGUI_TYPE_TEXTURE_VIEW, BobguiTextureViewClass))
struct _BobguiTextureView
{
  BobguiWidget parent_instance;

  GdkTexture *texture;
};

struct _BobguiTextureViewClass
{
  BobguiWidgetClass parent_class;
};

GType bobgui_texture_view_get_type (void) G_GNUC_CONST;


G_DEFINE_TYPE(BobguiTextureView, bobgui_texture_view, BOBGUI_TYPE_WIDGET)

static void
bobgui_texture_view_measure (BobguiWidget      *widget,
                          BobguiOrientation  orientation,
                          int             for_size,
                          int            *minimum,
                          int            *natural,
                          int            *minimum_baseline,
                          int            *natural_baseline)
{
  BobguiTextureView *self = BOBGUI_TEXTURE_VIEW (widget);

  if (self->texture == NULL)
    return;

  if (orientation == BOBGUI_ORIENTATION_HORIZONTAL)
    {
      *minimum = 0;
      *natural = gdk_texture_get_width (self->texture);
    }
  else /* VERTICAL */
    {
      *minimum = 0;
      *natural = gdk_texture_get_height (self->texture);
    }
}

static void
bobgui_texture_view_snapshot (BobguiWidget   *widget,
                           BobguiSnapshot *snapshot)
{
  BobguiTextureView *self = BOBGUI_TEXTURE_VIEW (widget);
  int width = bobgui_widget_get_width (widget);
  int height = bobgui_widget_get_height (widget);

  if (self->texture != NULL)
    {
      graphene_rect_t bounds;

      bounds.origin.x = MAX (0, width / 2 - gdk_texture_get_width (self->texture));
      bounds.origin.y = MAX (0, height / 2 - gdk_texture_get_height (self->texture));

      bounds.size.width = MIN (width, gdk_texture_get_width (self->texture));
      bounds.size.height = MIN (height, gdk_texture_get_height (self->texture));

      bobgui_snapshot_append_texture (snapshot, self->texture, &bounds);
    }
}

static void
bobgui_texture_view_finalize (GObject *object)
{
  BobguiTextureView *self = BOBGUI_TEXTURE_VIEW (object);

  g_clear_object (&self->texture);

  G_OBJECT_CLASS (bobgui_texture_view_parent_class)->finalize (object);
}

static void
bobgui_texture_view_init (BobguiTextureView *self)
{
}

static void
bobgui_texture_view_class_init (BobguiTextureViewClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (klass);

  object_class->finalize = bobgui_texture_view_finalize;

  widget_class->measure = bobgui_texture_view_measure;
  widget_class->snapshot = bobgui_texture_view_snapshot;
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
  BobguiWidget *view;
  GdkTexture *texture;
  GFile *file;
  GError *error = NULL;
  gboolean done = FALSE;

  bobgui_init ();

  if (argc != 2)
    {
      g_error ("No texture file path given.");
      return -1;
    }

  file = g_file_new_for_path (argv[1]);
  texture = gdk_texture_new_from_file (file, &error);

  if (error != NULL)
    {
      g_error ("Error: %s", error->message);
      return -1;
    }

  window = bobgui_window_new ();
  g_signal_connect (window, "destroy", G_CALLBACK (quit_cb), &done);
  view = g_object_new (BOBGUI_TYPE_TEXTURE_VIEW, NULL);
  ((BobguiTextureView*)view)->texture = g_steal_pointer (&texture);

  bobgui_window_set_child (BOBGUI_WINDOW (window), view);

  bobgui_window_present (BOBGUI_WINDOW (window));

  while (!done)
    g_main_context_iteration (NULL, TRUE);

  g_object_unref (file);

  return 0;
}
