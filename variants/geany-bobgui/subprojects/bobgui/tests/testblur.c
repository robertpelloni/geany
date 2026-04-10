
#include <bobgui/bobgui.h>



struct _BobguiBlurBox
{
  BobguiBox parent_instance;

  double radius;
};
typedef struct _BobguiBlurBox BobguiBlurBox;

struct _BobguiBlurBoxClass
{
  BobguiBoxClass parent_class;
};
typedef struct _BobguiBlurBoxClass BobguiBlurBoxClass;

static GType bobgui_blur_box_get_type (void);
G_DEFINE_TYPE (BobguiBlurBox, bobgui_blur_box, BOBGUI_TYPE_BOX)


static void
snapshot_blur (BobguiWidget   *widget,
               BobguiSnapshot *snapshot)
{
  BobguiBlurBox *box = (BobguiBlurBox *) widget;

  bobgui_snapshot_push_blur (snapshot, box->radius);

  BOBGUI_WIDGET_CLASS (bobgui_blur_box_parent_class)->snapshot (widget, snapshot);

  bobgui_snapshot_pop (snapshot);
}


static void
bobgui_blur_box_init (BobguiBlurBox *box) {
  box->radius = 0;
}

static void
bobgui_blur_box_class_init (BobguiBlurBoxClass *klass)
{
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (klass);

  widget_class->snapshot = snapshot_blur;
}

static void
value_changed_cb (BobguiRange *range,
                  gpointer  user_data)
{
  BobguiBlurBox *box = user_data;
  double value = bobgui_range_get_value (range);

  box->radius = value;
  bobgui_widget_queue_draw (BOBGUI_WIDGET (box));
}

static void
value_changed_cb2 (BobguiRange *range,
                   gpointer  user_data)
{
  BobguiLabel *label = user_data;
  double value = bobgui_range_get_value (range);
  char *text;

  text = g_strdup_printf ("%.2f", value);
  bobgui_label_set_label (label, text);
  g_free (text);
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
  BobguiWidget *blur_box;
  BobguiWidget *scale;
  BobguiWidget *value_label;
  gboolean done = FALSE;

  bobgui_init ();

  window = bobgui_window_new ();
  g_signal_connect (window, "destroy", G_CALLBACK (quit_cb), &done);

  blur_box = g_object_new (bobgui_blur_box_get_type (),
                           "orientation", BOBGUI_ORIENTATION_VERTICAL,
                           "spacing", 32,
                           NULL);

  value_label = bobgui_label_new ("FF");
  bobgui_widget_set_margin_top (value_label, 32);
  {
    PangoAttrList *attrs;

    attrs = pango_attr_list_new ();
    pango_attr_list_insert (attrs, pango_attr_scale_new (6.0));
    bobgui_label_set_attributes (BOBGUI_LABEL (value_label), attrs);
    pango_attr_list_unref (attrs);
  }
  bobgui_box_append (BOBGUI_BOX (blur_box), value_label);


  scale = bobgui_scale_new_with_range (BOBGUI_ORIENTATION_HORIZONTAL, 0, 10, 0.05);
  bobgui_widget_set_size_request (scale, 200, -1);
  bobgui_widget_set_halign (scale, BOBGUI_ALIGN_CENTER);
  bobgui_widget_set_valign (scale, BOBGUI_ALIGN_CENTER);
  bobgui_widget_set_hexpand (scale, TRUE);
  g_signal_connect (scale, "value-changed", G_CALLBACK (value_changed_cb), blur_box);
  g_signal_connect (scale, "value-changed", G_CALLBACK (value_changed_cb2), value_label);

  bobgui_box_append (BOBGUI_BOX (blur_box), scale);
  bobgui_window_set_child (BOBGUI_WINDOW (window), blur_box);

  bobgui_window_present (BOBGUI_WINDOW (window));

  while (!done)
    g_main_context_iteration (NULL, TRUE);

  return 0;
}
