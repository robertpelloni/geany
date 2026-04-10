/* Demonstrates hooking up an input method context to a custom widget.
 */
#include <bobgui/bobgui.h>

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

G_DECLARE_FINAL_TYPE (DemoWidget, demo_widget, DEMO, WIDGET, BobguiWidget)

struct _DemoWidget
{
  BobguiWidget parent_instance;

  BobguiIMContext *im_context;
  PangoLayout *layout;
};

struct _DemoWidgetClass
{
  BobguiWidgetClass parent_class;
};

G_DEFINE_TYPE (DemoWidget, demo_widget, BOBGUI_TYPE_WIDGET)

static void
commit_cb (BobguiIMContext *context,
           const char   *str,
           DemoWidget   *demo)
{
  pango_layout_set_text (demo->layout, str, -1);
  pango_layout_set_attributes (demo->layout, NULL);
  bobgui_widget_queue_draw (BOBGUI_WIDGET (demo));
}

static void
preedit_changed_cb (BobguiIMContext *context,
                    DemoWidget   *demo)
{
  char *str;
  PangoAttrList *attrs;
  int cursor_pos;

  bobgui_im_context_get_preedit_string (context, &str, &attrs, &cursor_pos);
  pango_layout_set_text (demo->layout, str, -1);
  pango_layout_set_attributes (demo->layout, attrs);
  g_free (str);
  pango_attr_list_unref (attrs);

  bobgui_widget_queue_draw (BOBGUI_WIDGET (demo));
}

static gboolean
key_pressed_cb (BobguiEventControllerKey *controller,
                guint                  keyval,
                guint                  keycode,
                GdkModifierType        state,
                DemoWidget            *demo)
{
  if (keyval == GDK_KEY_BackSpace)
    {
      pango_layout_set_text (demo->layout, "", -1);
      pango_layout_set_attributes (demo->layout, NULL);
      bobgui_widget_queue_draw (BOBGUI_WIDGET (demo));

      return TRUE;
    }

  return FALSE;
}

static void
demo_widget_init (DemoWidget *demo)
{
  BobguiEventController *controller;

  bobgui_widget_set_focusable (BOBGUI_WIDGET (demo), TRUE);

  demo->layout = bobgui_widget_create_pango_layout (BOBGUI_WIDGET (demo), "");

  demo->im_context = bobgui_im_multicontext_new ();

  g_signal_connect (demo->im_context, "commit", G_CALLBACK (commit_cb), demo);
  g_signal_connect (demo->im_context, "preedit-changed", G_CALLBACK (preedit_changed_cb), demo);

  controller = bobgui_event_controller_key_new ();
  bobgui_event_controller_key_set_im_context (BOBGUI_EVENT_CONTROLLER_KEY (controller),
                                           demo->im_context);

  g_signal_connect (controller, "key-pressed", G_CALLBACK (key_pressed_cb), demo);

  bobgui_widget_add_controller (BOBGUI_WIDGET (demo), controller);
}

static void
demo_widget_dispose (GObject *object)
{
  DemoWidget *demo = DEMO_WIDGET (object);

  g_clear_object (&demo->layout);
  g_clear_object (&demo->im_context);

  G_OBJECT_CLASS (demo_widget_parent_class)->dispose (object);
}

static void
demo_widget_snapshot (BobguiWidget   *widget,
                      BobguiSnapshot *snapshot)
{
  DemoWidget *demo = DEMO_WIDGET (widget);

  bobgui_snapshot_render_layout (snapshot,
                              bobgui_widget_get_style_context (widget),
                              0, 0,
                              demo->layout);
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
  return g_object_new (demo_widget_get_type (), NULL);
}

int
main (int argc, char *argv[])
{
  BobguiWindow *window;
  BobguiWidget *demo;

  bobgui_init ();

  window = BOBGUI_WINDOW (bobgui_window_new ());

  demo = demo_widget_new ();

  bobgui_window_set_child (window, demo);

  bobgui_window_present (window);

  bobgui_widget_grab_focus (demo);

  while (g_list_model_get_n_items (bobgui_window_get_toplevels ()) > 0)
    g_main_context_iteration (NULL, TRUE);

  return 0;
}
