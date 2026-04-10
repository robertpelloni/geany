/* Path/Fill and Stroke
 *
 * This demo shows how to use GskPath to draw shapes that are (a bit)
 * more complex than a rounded rectangle.
 *
 * It also demonstrates printing to a stream with BobguiPrintDialog.
 *
 * Finally, it shows how to use BobguiPopoverBin to add a context menu
 * to a widget that does not have one.
 */

#include <glib/gi18n.h>
#include <bobgui/bobgui.h>
#include <cairo-pdf.h>

#include "paintable.h"

#define BOBGUI_TYPE_LOGO_PAINTABLE (bobgui_logo_paintable_get_type ())
G_DECLARE_FINAL_TYPE (BobguiLogoPaintable, bobgui_logo_paintable, BOBGUI, LOGO_PAINTABLE, GObject)

struct _BobguiLogoPaintable
{
  GObject parent_instance;

  int width;
  int height;
  GskPath *path[3];
  GdkRGBA color[3];

  GskPath *stroke_path;
  GskStroke *stroke1;
  GskStroke *stroke2;
  GdkRGBA stroke_color;
};

struct _BobguiLogoPaintableClass
{
  GObjectClass parent_class;
};

static int
bobgui_logo_paintable_get_intrinsic_width (GdkPaintable *paintable)
{
  BobguiLogoPaintable *self = BOBGUI_LOGO_PAINTABLE (paintable);

  return self->width;
}

static int
bobgui_logo_paintable_get_intrinsic_height (GdkPaintable *paintable)
{
  BobguiLogoPaintable *self = BOBGUI_LOGO_PAINTABLE (paintable);

  return self->height;
}

static void
bobgui_logo_paintable_snapshot (GdkPaintable *paintable,
                             GdkSnapshot  *snapshot,
                             double        width,
                             double        height)
{
  BobguiLogoPaintable *self = BOBGUI_LOGO_PAINTABLE (paintable);

  for (unsigned int i = 0; i < 3; i++)
    {
      bobgui_snapshot_push_fill (snapshot, self->path[i], GSK_FILL_RULE_WINDING);
      bobgui_snapshot_append_color (snapshot,
                                 &self->color[i],
                                 &GRAPHENE_RECT_INIT (0, 0, width, height));
      bobgui_snapshot_pop (snapshot);
    }
  for (unsigned int i = 0; i < 3; i++)
    {
      bobgui_snapshot_push_stroke (snapshot, self->stroke_path, self->stroke1);
      bobgui_snapshot_append_color (snapshot,
                                 &self->stroke_color,
                                 &GRAPHENE_RECT_INIT (0, 0, width, height));
      bobgui_snapshot_pop (snapshot);
    }

  bobgui_snapshot_push_stroke (snapshot, self->stroke_path, self->stroke2);
  bobgui_snapshot_append_color (snapshot,
                             &self->stroke_color,
                             &GRAPHENE_RECT_INIT (0, 0, width, height));
  bobgui_snapshot_pop (snapshot);
}

static GdkPaintableFlags
bobgui_logo_paintable_get_flags (GdkPaintable *paintable)
{
  return GDK_PAINTABLE_STATIC_CONTENTS | GDK_PAINTABLE_STATIC_SIZE;
}

static void
bobgui_logo_paintable_paintable_init (GdkPaintableInterface *iface)
{
  iface->get_intrinsic_width = bobgui_logo_paintable_get_intrinsic_width;
  iface->get_intrinsic_height = bobgui_logo_paintable_get_intrinsic_height;
  iface->snapshot = bobgui_logo_paintable_snapshot;
  iface->get_flags = bobgui_logo_paintable_get_flags;
}

/* When defining the GType, we need to implement the GdkPaintable interface */
G_DEFINE_TYPE_WITH_CODE (BobguiLogoPaintable, bobgui_logo_paintable, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (GDK_TYPE_PAINTABLE,
                                                bobgui_logo_paintable_paintable_init))

static void
bobgui_logo_paintable_dispose (GObject *object)
{
  BobguiLogoPaintable *self = BOBGUI_LOGO_PAINTABLE (object);

  for (unsigned int i = 0; i < 3; i++)
    gsk_path_unref (self->path[i]);

  gsk_path_unref (self->stroke_path);

  gsk_stroke_free (self->stroke1);
  gsk_stroke_free (self->stroke2);

  G_OBJECT_CLASS (bobgui_logo_paintable_parent_class)->dispose (object);
}

static void
bobgui_logo_paintable_class_init (BobguiLogoPaintableClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = bobgui_logo_paintable_dispose;
}

static void
bobgui_logo_paintable_init (BobguiLogoPaintable *self)
{
}

static GdkPaintable *
bobgui_logo_paintable_new (void)
{
  BobguiLogoPaintable *self;
  graphene_rect_t bounds, bounds2;

  self = g_object_new (BOBGUI_TYPE_LOGO_PAINTABLE, NULL);

  /* Paths and colors extracted from bobgui-logo.svg */
  self->path[0] = gsk_path_parse ("m3.12,66.17 -2.06,-51.46 32.93,24.7 v55.58 l-30.87,-28.82 z");
  self->path[1] = gsk_path_parse ("m34,95 49.4,-20.58 4.12,-51.46 -53.52,16.47 v55.58 z");
  self->path[2] = gsk_path_parse ("m1.06,14.71 32.93,24.7 53.52,-16.47 -36.75,-21.88 -49.7,13.65 z");

  gdk_rgba_parse (&self->color[0], "#e40000");
  gdk_rgba_parse (&self->color[1], "#7fe719");
  gdk_rgba_parse (&self->color[2], "#729fcf");

  self->stroke_path = gsk_path_parse ("m50.6,51.3 -47.3,14 z l33,23 z v-50");
  self->stroke1 = gsk_stroke_new (2.12);
  self->stroke2 = gsk_stroke_new (1.25);
  gdk_rgba_parse (&self->stroke_color, "#ffffff");

  gsk_path_get_stroke_bounds (self->path[0], self->stroke1, &bounds);
  gsk_path_get_stroke_bounds (self->path[1], self->stroke1, &bounds2);
  graphene_rect_union (&bounds, &bounds2, &bounds);
  gsk_path_get_stroke_bounds (self->path[2], self->stroke1, &bounds2);
  graphene_rect_union (&bounds, &bounds2, &bounds);
  gsk_path_get_stroke_bounds (self->stroke_path, self->stroke2, &bounds2);
  graphene_rect_union (&bounds, &bounds2, &bounds);

  self->width = bounds.origin.x + bounds.size.width;
  self->height = bounds.origin.y + bounds.size.height;

  return GDK_PAINTABLE (self);
}

static cairo_status_t
write_cairo (void                *closure,
             const unsigned char *data,
             unsigned int         length)
{
  GOutputStream *stream = closure;
  gsize written;
  GError *error = NULL;

  if (!g_output_stream_write_all (stream, data, length, &written, NULL, &error))
    {
      g_print ("Error writing pdf stream: %s\n", error->message);
      g_error_free (error);
      return CAIRO_STATUS_WRITE_ERROR;
    }

  return CAIRO_STATUS_SUCCESS;
}

static void
print_ready (GObject      *source,
             GAsyncResult *result,
             gpointer      data)
{
  BobguiPrintDialog *dialog = BOBGUI_PRINT_DIALOG (source);
  GError *error = NULL;
  GOutputStream *stream;
  BobguiSnapshot *snapshot;
  GdkPaintable *paintable;
  GskRenderNode *node;
  cairo_surface_t *surface;
  cairo_t *cr;

  stream = bobgui_print_dialog_print_finish (dialog, result, &error);
  if (stream == NULL)
    {
      g_print ("Failed to get output stream: %s\n", error->message);
      g_error_free (error);
      return;
    }

  snapshot = bobgui_snapshot_new ();
  paintable = bobgui_picture_get_paintable (BOBGUI_PICTURE (data));
  gdk_paintable_snapshot (paintable, snapshot, 100, 100);
  node = bobgui_snapshot_free_to_node (snapshot);

  surface = cairo_pdf_surface_create_for_stream (write_cairo, stream, 100, 100);
  cr = cairo_create (surface);

  gsk_render_node_draw (node, cr);

  cairo_destroy (cr);
  cairo_surface_destroy (surface);
  gsk_render_node_unref (node);

  if (!g_output_stream_close (stream, NULL, &error))
    {
      if (!g_error_matches (error, BOBGUI_DIALOG_ERROR, BOBGUI_DIALOG_ERROR_DISMISSED))
        g_print ("Error from close: %s\n", error->message);
      g_error_free (error);
    }

  g_object_unref (stream);
}

static void
print (BobguiWidget *picture)
{
  BobguiPrintDialog *dialog;

  dialog = bobgui_print_dialog_new ();

  bobgui_print_dialog_print (dialog,
                          BOBGUI_WINDOW (bobgui_widget_get_root (picture)),
                          NULL,
                          NULL,
                          print_ready,
                          picture);

  g_object_unref (dialog);
}

static GMenuModel *
get_menu (void)
{
  GMenu *menu;
  GMenuItem *item;

  menu = g_menu_new ();
  item = g_menu_item_new ("Print", "widget.print");
  g_menu_append_item (menu, item);
  item = g_menu_item_new ("About", "app.about");
  g_menu_append_item (menu, item);
  g_object_unref (item);

  return G_MENU_MODEL (menu);
}

static void
activate_print (GSimpleAction *action,
                GVariant      *parameter,
                gpointer       user_data)
{
  print (BOBGUI_WIDGET (user_data));
}

static GActionGroup *
get_actions (BobguiWidget *widget)
{
  GSimpleActionGroup *actions;
  GActionEntry entries[] = {
    { "print", activate_print, NULL, NULL, NULL, },
  };

  actions = g_simple_action_group_new ();
  g_action_map_add_action_entries (G_ACTION_MAP (actions),
                                   entries, G_N_ELEMENTS (entries),
                                   widget);

  return G_ACTION_GROUP (actions);
}

BobguiWidget *
do_path_fill (BobguiWidget *do_widget)
{
  static BobguiWidget *window = NULL;

  if (!window)
    {
      BobguiWidget *header, *button, *label;
      BobguiWidget *picture;
      GdkPaintable *paintable;
      BobguiWidget *bin;
      GMenuModel *menu;
      GActionGroup *actions;

      window = bobgui_window_new ();
      bobgui_window_set_resizable (BOBGUI_WINDOW (window), FALSE);
      bobgui_window_set_default_size (BOBGUI_WINDOW (window), 100, 100);
      bobgui_window_set_title (BOBGUI_WINDOW (window), "Fill and Stroke");

      bobgui_window_set_application (BOBGUI_WINDOW (window), BOBGUI_APPLICATION (g_application_get_default ()));

      header = bobgui_header_bar_new ();
      button = bobgui_button_new_from_icon_name ("printer-symbolic");
      bobgui_header_bar_pack_start (BOBGUI_HEADER_BAR (header), button);
      label = bobgui_label_new ("Fill and Stroke");
      bobgui_widget_add_css_class (label, "title");
      bobgui_header_bar_set_title_widget (BOBGUI_HEADER_BAR (header), label);
      bobgui_window_set_titlebar (BOBGUI_WINDOW (window), header);
      g_object_add_weak_pointer (G_OBJECT (window), (gpointer *)&window);

      paintable = bobgui_logo_paintable_new ();
      picture = bobgui_picture_new_for_paintable (paintable);
      bobgui_picture_set_content_fit (BOBGUI_PICTURE (picture), BOBGUI_CONTENT_FIT_CONTAIN);
      bobgui_picture_set_can_shrink (BOBGUI_PICTURE (picture), FALSE);
      g_object_unref (paintable);

      bin = bobgui_popover_bin_new ();

      actions = get_actions (picture);
      bobgui_widget_insert_action_group (bin, "widget", actions);
      g_object_unref (actions);

      bobgui_popover_bin_set_child (BOBGUI_POPOVER_BIN (bin), picture);

      menu = get_menu ();
      bobgui_popover_bin_set_menu_model (BOBGUI_POPOVER_BIN (bin), menu);
      g_object_unref (menu);

      bobgui_popover_bin_set_handle_input (BOBGUI_POPOVER_BIN (bin), TRUE);

      g_signal_connect_swapped (button, "clicked", G_CALLBACK (print), picture);

      bobgui_window_set_child (BOBGUI_WINDOW (window), bin);
    }

  if (!bobgui_widget_get_visible (window))
    bobgui_window_present (BOBGUI_WINDOW (window));
  else
    bobgui_window_destroy (BOBGUI_WINDOW (window));

  return window;
}
