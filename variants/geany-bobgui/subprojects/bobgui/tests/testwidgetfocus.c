#include <bobgui/bobgui.h>

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

typedef struct _BobguiFocusWidget      BobguiFocusWidget;
typedef struct _BobguiFocusWidgetClass BobguiFocusWidgetClass;

#define BOBGUI_TYPE_FOCUS_WIDGET           (bobgui_focus_widget_get_type ())
#define BOBGUI_FOCUS_WIDGET(obj)           (G_TYPE_CHECK_INSTANCE_CAST(obj, BOBGUI_TYPE_FOCUS_WIDGET, BobguiFocusWidget))
#define BOBGUI_FOCUS_WIDGET_CLASS(cls)     (G_TYPE_CHECK_CLASS_CAST(cls, BOBGUI_TYPE_FOCUS_WIDGET, BobguiFocusWidgetClass))
#define BOBGUI_IS_FOCUS_WIDGET(obj)        (G_TYPE_CHECK_INSTANCE_TYPE(obj, BOBGUI_TYPE_FOCUS_WIDGET))
#define BOBGUI_IS_FOCUS_WIDGET_CLASS(cls)   (G_TYPE_CHECK_CLASS_TYPE(cls, BOBGUI_TYPE_FOCUS_WIDGET))
#define BOBGUI_FOCUS_WIDGET_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS(obj, BOBGUI_TYPE_FOCUS_WIDGET, BobguiFocusWidgetClass))

const char *css =
"* {"
"  transition: none; "
"}"
"focuswidget {"
"  padding: 30px;"
"  font-size: 70%;"
"}"
"focuswidget button:nth-child(1) {"
"  margin-right: 15px;"
"  margin-bottom: 15px;"
"}"
"focuswidget button:nth-child(2) {"
"  margin-left: 15px;"
"  margin-bottom: 15px;"
"}"
"focuswidget button:nth-child(3) {"
"  margin-right: 15px;"
"  margin-top: 15px;"
"}"
"focuswidget button:nth-child(4) {"
"  margin-left: 15px;"
"  margin-top: 15px;"
"}"
"focuswidget button {"
"  min-width: 80px;"
"  min-height: 80px;"
"  margin: 0px;"
"  border: 5px solid green;"
"  border-radius: 0px;"
"  padding: 10px;"
"  background-image: none;"
"  background-color: white;"
"  box-shadow: none;"
"}"
"focuswidget button:focus-visible {"
"  outline-width: 4px;"
"  outline-color: yellow;"
"}"
"focuswidget button:hover {"
"  background-color: black;"
"  color: white;"
"}"
"focuswidget button label:hover {"
"  background-color: green;"
"}"
;

struct _BobguiFocusWidget
{
  BobguiWidget parent_instance;
  double mouse_x;
  double mouse_y;

  union {
    struct {
      BobguiWidget *child1;
      BobguiWidget *child2;
      BobguiWidget *child3;
      BobguiWidget *child4;
    };
    BobguiWidget* children[4];
  };
};

struct _BobguiFocusWidgetClass
{
  BobguiWidgetClass parent_class;
};

GType bobgui_focus_widget_get_type (void) G_GNUC_CONST;


G_DEFINE_TYPE(BobguiFocusWidget, bobgui_focus_widget, BOBGUI_TYPE_WIDGET)

static void
bobgui_focus_widget_size_allocate (BobguiWidget *widget,
                                int        width,
                                int        height,
                                int        baseline)
{
  BobguiFocusWidget *self = BOBGUI_FOCUS_WIDGET (widget);
  int child_width  = width  / 2;
  int child_height = height / 2;
  BobguiAllocation child_alloc;

  child_alloc.x = 0;
  child_alloc.y = 0;
  child_alloc.width = child_width;
  child_alloc.height = child_height;

  bobgui_widget_size_allocate (self->child1, &child_alloc, -1);

  child_alloc.x += child_width;

  bobgui_widget_size_allocate (self->child2, &child_alloc, -1);

  child_alloc.y += child_height;

  bobgui_widget_size_allocate (self->child4, &child_alloc, -1);

  child_alloc.x -= child_width;

  bobgui_widget_size_allocate (self->child3, &child_alloc, -1);
}

static void
bobgui_focus_widget_measure (BobguiWidget      *widget,
                          BobguiOrientation  orientation,
                          int             for_size,
                          int            *minimum,
                          int            *natural,
                          int            *minimum_baseline,
                          int            *natural_baseline)
{
  BobguiFocusWidget *self = BOBGUI_FOCUS_WIDGET (widget);
  int min, nat;
  int i;

  *minimum = 0;
  *natural = 0;

  for (i = 0; i < 4; i ++)
    {
      bobgui_widget_measure (self->children[i], orientation, for_size,
                          &min, &nat, NULL, NULL);

      *minimum = MAX (*minimum, min);
      *natural = MAX (*natural, nat);
    }

  *minimum *= 2;
  *natural *= 2;
}

static void
bobgui_focus_widget_snapshot (BobguiWidget *widget, BobguiSnapshot *snapshot)
{
  BobguiFocusWidget *self = BOBGUI_FOCUS_WIDGET (widget);

  bobgui_widget_snapshot_child (widget, self->child1, snapshot);
  bobgui_widget_snapshot_child (widget, self->child2, snapshot);
  bobgui_widget_snapshot_child (widget, self->child3, snapshot);
  bobgui_widget_snapshot_child (widget, self->child4, snapshot);

  if (self->mouse_x != G_MININT && self->mouse_y != G_MININT)
    {
      PangoLayout *layout;
      char *text;
      BobguiAllocation alloc;
      graphene_rect_t bounds;
      GdkRGBA black = {0, 0, 0, 1};

      bobgui_widget_get_allocation (widget, &alloc);

      /* Since event coordinates and drawing is supposed to happen in the
       * same coordinates space, this should all work out just fine. */
      bounds.origin.x = self->mouse_x;
      bounds.origin.y = -30;
      bounds.size.width = 1;
      bounds.size.height = alloc.height;
      bobgui_snapshot_append_color (snapshot,
                                 &black,
                                 &bounds);

      bounds.origin.x = -30;
      bounds.origin.y = self->mouse_y;
      bounds.size.width = alloc.width;
      bounds.size.height = 1;
      bobgui_snapshot_append_color (snapshot,
                                 &black,
                                 &bounds);

      layout = bobgui_widget_create_pango_layout (widget, NULL);
      text = g_strdup_printf ("%.2f×%.2f", self->mouse_x, self->mouse_y);
      pango_layout_set_text (layout, text, -1);

      bobgui_snapshot_render_layout (snapshot,
                                  bobgui_widget_get_style_context (widget),
                                  self->mouse_x + 2,
                                  self->mouse_y - 15, /* *shrug* */
                                  layout);

      g_free (text);
      g_object_unref (layout);
    }
}

static void
motion_cb (BobguiEventControllerMotion *controller,
           double                    x,
           double                    y,
           BobguiWidget                *widget)
{
  BobguiFocusWidget *self = BOBGUI_FOCUS_WIDGET (widget);

  self->mouse_x = x;
  self->mouse_y = y;

  bobgui_widget_queue_draw (widget);
}

static void
bobgui_focus_widget_finalize (GObject *object)
{
  BobguiFocusWidget *self = BOBGUI_FOCUS_WIDGET (object);

  bobgui_widget_unparent (self->child1);
  bobgui_widget_unparent (self->child2);
  bobgui_widget_unparent (self->child3);
  bobgui_widget_unparent (self->child4);

  G_OBJECT_CLASS (bobgui_focus_widget_parent_class)->finalize (object);
}

static void
bobgui_focus_widget_init (BobguiFocusWidget *self)
{
  BobguiEventController *controller;

  self->child1 = bobgui_button_new_with_label ("1");
  bobgui_widget_set_parent (self->child1, BOBGUI_WIDGET (self));
  self->child2 = bobgui_button_new_with_label ("2");
  bobgui_widget_set_parent (self->child2, BOBGUI_WIDGET (self));
  self->child3 = bobgui_button_new_with_label ("3");
  bobgui_widget_set_parent (self->child3, BOBGUI_WIDGET (self));
  self->child4 = bobgui_button_new_with_label ("4");
  bobgui_widget_set_parent (self->child4, BOBGUI_WIDGET (self));

  self->mouse_x = G_MININT;
  self->mouse_y = G_MININT;

  controller = bobgui_event_controller_motion_new ();
  g_signal_connect (controller, "motion",
		    G_CALLBACK (motion_cb), self);
  bobgui_widget_add_controller (BOBGUI_WIDGET (self), controller);
}

static void
bobgui_focus_widget_class_init (BobguiFocusWidgetClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (klass);

  object_class->finalize = bobgui_focus_widget_finalize;

  widget_class->snapshot = bobgui_focus_widget_snapshot;
  widget_class->measure = bobgui_focus_widget_measure;
  widget_class->size_allocate = bobgui_focus_widget_size_allocate;

  bobgui_widget_class_set_css_name (widget_class, "focuswidget");
}

static void
quit_cb (BobguiWidget *widget,
         gpointer   user_data)
{
  gboolean *is_done = user_data;

  *is_done = TRUE;

  g_main_context_wakeup (NULL);
}

int
main(int argc, char **argv)
{
  BobguiWidget *window;
  BobguiWidget *widget;
  BobguiCssProvider *provider;
  gboolean done = FALSE;

  bobgui_init ();

  provider = bobgui_css_provider_new ();
  bobgui_css_provider_load_from_data (provider, css, -1);
  bobgui_style_context_add_provider_for_display (gdk_display_get_default (),
                                              BOBGUI_STYLE_PROVIDER (provider),
                                              BOBGUI_STYLE_PROVIDER_PRIORITY_APPLICATION);

  window = bobgui_window_new ();
  widget = g_object_new (BOBGUI_TYPE_FOCUS_WIDGET, NULL);

  bobgui_window_set_decorated (BOBGUI_WINDOW (window), FALSE);

  bobgui_window_set_child (BOBGUI_WINDOW (window), widget);
  g_signal_connect (window, "destroy", G_CALLBACK (quit_cb), &done);

  bobgui_window_present (BOBGUI_WINDOW (window));

  while (!done)
    g_main_context_iteration (NULL, TRUE);
}
