#include <bobgui/bobgui.h>

#define BASELINE_TYPE_WIDGET (baseline_widget_get_type ())
G_DECLARE_FINAL_TYPE (BaselineWidget, baseline_widget, BASELINE, WIDGET, BobguiWidget)

enum
{
  PROP_ABOVE = 1,
  PROP_BELOW,
  PROP_ACROSS
};

struct _BaselineWidget
{
  BobguiWidget parent_instance;

  int above, below, across;
};

struct _BaselineWidgetClass
{
  BobguiWidgetClass parent_class;
};

G_DEFINE_TYPE (BaselineWidget, baseline_widget, BOBGUI_TYPE_WIDGET)

static void
baseline_widget_init (BaselineWidget *self)
{
}

static void
baseline_widget_set_property (GObject      *object,
                              guint         prop_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  BaselineWidget *self = BASELINE_WIDGET (object);

  switch (prop_id)
    {
    case PROP_ABOVE:
      self->above = g_value_get_int (value);
      bobgui_widget_queue_resize (BOBGUI_WIDGET (object));
      break;

    case PROP_BELOW:
      self->below = g_value_get_int (value);
      bobgui_widget_queue_resize (BOBGUI_WIDGET (object));
      break;

    case PROP_ACROSS:
      self->across = g_value_get_int (value);
      bobgui_widget_queue_resize (BOBGUI_WIDGET (object));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
baseline_widget_get_property (GObject     *object,
                              guint        prop_id,
                              GValue      *value,
                              GParamSpec  *pspec)
{
  BaselineWidget *self = BASELINE_WIDGET (object);

  switch (prop_id)
    {
    case PROP_ABOVE:
      g_value_set_int (value, self->above);
      break;

    case PROP_BELOW:
      g_value_set_int (value, self->below);
      break;

    case PROP_ACROSS:
      g_value_set_int (value, self->across);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
baseline_widget_measure (BobguiWidget      *widget,
                         BobguiOrientation  orientation,
                         int             for_size,
                         int            *minimum,
                         int            *natural,
                         int            *minimum_baseline,
                         int            *natural_baseline)
{
  BaselineWidget *self = BASELINE_WIDGET (widget);

  if (orientation == BOBGUI_ORIENTATION_VERTICAL)
    {
      if (self->below >= 0)
        {
          *minimum = *natural = self->above + self->below;
          *minimum_baseline = *natural_baseline = self->above;
        }
      else
        {
          *minimum = *natural = self->above;
          *minimum_baseline = *natural_baseline = -1;
        }
    }
  else
    {
      *minimum = *natural = self->across;
      *minimum_baseline = *natural_baseline = -1;
    }
}

static void
baseline_widget_snapshot (BobguiWidget   *widget,
                          BobguiSnapshot *snapshot)
{
  BaselineWidget *self = BASELINE_WIDGET (widget);
  int height, baseline;
  graphene_rect_t bounds;
  float widths[4];
  GdkRGBA colors[4];
  GskRoundedRect outline;

  height = bobgui_widget_get_height (widget);
  baseline = bobgui_widget_get_baseline (widget);

  bobgui_snapshot_save (snapshot);

  if (baseline > -1)
    {
      int y;
      if (self->below >= 0)
        y = MAX (baseline - self->above, 0);
      else
        y = CLAMP (baseline - self->above / 2, 0, height - self->above);
      bobgui_snapshot_translate (snapshot, &GRAPHENE_POINT_INIT (0, y));
    }

  bounds.origin.x = 0;
  bounds.origin.y = 0;
  bounds.size.width = self->across;
  bounds.size.height = self->above;

  if (self->below >= 0)
    bobgui_snapshot_append_color (snapshot, &(GdkRGBA){1, 1, 0, 0.2}, &bounds);
  else
    bobgui_snapshot_append_color (snapshot, &(GdkRGBA){0, 1, 0, 0.2}, &bounds);

  outline = GSK_ROUNDED_RECT_INIT (0, 0, self->across, self->above);
  for (int i = 0; i < 4; i++)
    {
      widths[i] = 1.;
      gdk_rgba_parse (&colors[i], "black");
    }

  bobgui_snapshot_append_border (snapshot, &outline, widths, colors);

  bobgui_snapshot_restore (snapshot);

  if (self->below >= 0)
    {
      bobgui_snapshot_save (snapshot);

      if (baseline > -1)
        bobgui_snapshot_translate (snapshot, &GRAPHENE_POINT_INIT (0, baseline));
      else
        bobgui_snapshot_translate (snapshot, &GRAPHENE_POINT_INIT (0, self->above));
      bounds.origin.x = 0;
      bounds.origin.y = 0;
      bounds.size.width = self->across;
      bounds.size.height = self->below;
      bobgui_snapshot_append_color (snapshot, &(GdkRGBA){0, 0, 1, 0.2}, &bounds);

      outline = GSK_ROUNDED_RECT_INIT (0, 0, self->across, self->below);
      for (int i = 0; i < 4; i++)
        {
          widths[i] = 1.;
          gdk_rgba_parse (&colors[i], "black");
        }

      bobgui_snapshot_append_border (snapshot, &outline, widths, colors);

      bobgui_snapshot_restore (snapshot);
    }
}


static void
baseline_widget_class_init (BaselineWidgetClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (class);

  object_class->set_property = baseline_widget_set_property;
  object_class->get_property = baseline_widget_get_property;

  widget_class->snapshot = baseline_widget_snapshot;
  widget_class->measure = baseline_widget_measure;

  g_object_class_install_property (object_class, PROP_ABOVE,
    g_param_spec_int ("above", NULL, NULL, 0, G_MAXINT, 0, G_PARAM_READWRITE));
  g_object_class_install_property (object_class, PROP_BELOW,
    g_param_spec_int ("below", NULL, NULL, -1, G_MAXINT, 0, G_PARAM_READWRITE));
  g_object_class_install_property (object_class, PROP_ACROSS,
    g_param_spec_int ("across", NULL, NULL, 0, G_MAXINT, 0, G_PARAM_READWRITE));
}

static BobguiWidget *
baseline_widget_new (int above, int below, int across)
{
  return g_object_new (BASELINE_TYPE_WIDGET,
                       "above", above,
                       "below", below,
                       "across", across,
                       "valign", BOBGUI_ALIGN_BASELINE_CENTER,
                       NULL);
}

int
main (int argc, char *argv[])
{
  BobguiWidget *window, *header;
  BobguiWidget *stack, *switcher;
  BobguiWidget *box, *box1;
  BobguiWidget *grid;
  BobguiWidget *child;

  bobgui_init ();

  window = bobgui_window_new ();

  header = bobgui_header_bar_new ();
  bobgui_window_set_titlebar (BOBGUI_WINDOW (window), header);

  stack = bobgui_stack_new ();
  bobgui_window_set_child (BOBGUI_WINDOW (window), stack);

  switcher = bobgui_stack_switcher_new ();
  bobgui_stack_switcher_set_stack (BOBGUI_STACK_SWITCHER (switcher), BOBGUI_STACK (stack));

  bobgui_header_bar_set_title_widget (BOBGUI_HEADER_BAR (header), switcher);

  box = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 20);
  bobgui_widget_set_margin_top (box, 20);
  bobgui_widget_set_margin_bottom (box, 20);
  bobgui_widget_set_margin_start (box, 20);
  bobgui_widget_set_margin_end (box, 20);
  bobgui_widget_set_valign (box, BOBGUI_ALIGN_BASELINE_CENTER);

  bobgui_stack_add_titled (BOBGUI_STACK (stack), box, "boxes", "Boxes");

  box1 = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 0);
  bobgui_widget_set_halign (box1, BOBGUI_ALIGN_FILL);
  bobgui_widget_set_valign (box1, BOBGUI_ALIGN_BASELINE_CENTER);
  bobgui_widget_set_hexpand (box1, TRUE);

  child = baseline_widget_new (20, 10, 20);
  bobgui_box_append (BOBGUI_BOX (box1), child);

  child = baseline_widget_new (5, 20, 20);
  bobgui_box_append (BOBGUI_BOX (box1), child);

  child = baseline_widget_new (25, -1, 20);
  bobgui_box_append (BOBGUI_BOX (box1), child);

  child = baseline_widget_new (25, 20, 30);
  bobgui_box_append (BOBGUI_BOX (box1), child);

  bobgui_box_append (BOBGUI_BOX (box), box1);

  box1 = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 0);
  bobgui_widget_set_halign (box1, BOBGUI_ALIGN_FILL);
  bobgui_widget_set_valign (box1, BOBGUI_ALIGN_BASELINE_CENTER);
  bobgui_widget_set_hexpand (box1, TRUE);

  child = baseline_widget_new (10, 15, 10);
  bobgui_box_append (BOBGUI_BOX (box1), child);

  child = baseline_widget_new (80, -1, 20);
  bobgui_box_append (BOBGUI_BOX (box1), child);

  child = baseline_widget_new (60, 15, 20);
  bobgui_box_append (BOBGUI_BOX (box1), child);

  child = baseline_widget_new (5, 10, 30);
  bobgui_box_append (BOBGUI_BOX (box1), child);

  bobgui_box_append (BOBGUI_BOX (box), box1);

  box = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 20);
  bobgui_widget_set_margin_top (box, 20);
  bobgui_widget_set_margin_bottom (box, 20);
  bobgui_widget_set_margin_start (box, 20);
  bobgui_widget_set_margin_end (box, 20);
  bobgui_widget_set_valign (box, BOBGUI_ALIGN_BASELINE_CENTER);

  grid = bobgui_grid_new ();
  bobgui_widget_set_valign (grid, BOBGUI_ALIGN_BASELINE_CENTER);
  bobgui_widget_set_hexpand (grid, TRUE);

  child = baseline_widget_new (20, 10, 20);
  bobgui_grid_attach (BOBGUI_GRID (grid), child, 0, 0, 1, 1);

  child = baseline_widget_new (5, 20, 20);
  bobgui_grid_attach (BOBGUI_GRID (grid), child, 1, 0, 1, 1);

  child = baseline_widget_new (25, -1, 20);
  bobgui_grid_attach (BOBGUI_GRID (grid), child, 0, 1, 1, 1);

  child = baseline_widget_new (25, 20, 30);
  bobgui_grid_attach (BOBGUI_GRID (grid), child, 1, 1, 1, 1);

  bobgui_box_append (BOBGUI_BOX (box), grid);

  grid = bobgui_grid_new ();
  bobgui_widget_set_valign (grid, BOBGUI_ALIGN_BASELINE_CENTER);
  bobgui_widget_set_hexpand (grid, TRUE);

  child = baseline_widget_new (10, 15, 10);
  bobgui_grid_attach (BOBGUI_GRID (grid), child, 0, 0, 1, 1);

  child = baseline_widget_new (80, -1, 20);
  bobgui_grid_attach (BOBGUI_GRID (grid), child, 1, 0, 1, 1);

  child = baseline_widget_new (60, 15, 20);
  bobgui_grid_attach (BOBGUI_GRID (grid), child, 0, 1, 1, 1);

  child = baseline_widget_new (5, 10, 30);
  bobgui_grid_attach (BOBGUI_GRID (grid), child, 1, 1, 1, 1);

  bobgui_box_append (BOBGUI_BOX (box), grid);

  bobgui_stack_add_titled (BOBGUI_STACK (stack), box, "grids", "Grids");

  box = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 20);
  bobgui_widget_set_margin_top (box, 20);
  bobgui_widget_set_margin_bottom (box, 20);
  bobgui_widget_set_margin_start (box, 20);
  bobgui_widget_set_margin_end (box, 20);
  bobgui_widget_set_valign (box, BOBGUI_ALIGN_BASELINE_CENTER);

  child = baseline_widget_new (60, 15, 20);
  bobgui_box_append (BOBGUI_BOX (box), child);

  child = bobgui_label_new ("Label");
  bobgui_widget_set_valign (child, BOBGUI_ALIGN_BASELINE_CENTER);
  bobgui_box_append (BOBGUI_BOX (box), child);

  child = bobgui_entry_new ();
  bobgui_editable_set_text (BOBGUI_EDITABLE (child), "Entry");
  bobgui_editable_set_width_chars (BOBGUI_EDITABLE (child), 10);
  bobgui_widget_set_valign (child, BOBGUI_ALIGN_BASELINE_CENTER);
  bobgui_box_append (BOBGUI_BOX (box), child);

  child = bobgui_password_entry_new ();
  bobgui_editable_set_text (BOBGUI_EDITABLE (child), "Password");
  bobgui_editable_set_width_chars (BOBGUI_EDITABLE (child), 10);
  bobgui_widget_set_valign (child, BOBGUI_ALIGN_BASELINE_CENTER);
  bobgui_box_append (BOBGUI_BOX (box), child);

  child = bobgui_spin_button_new_with_range (0, 100, 1);
  bobgui_widget_set_valign (child, BOBGUI_ALIGN_BASELINE_CENTER);
  bobgui_box_append (BOBGUI_BOX (box), child);

  child = bobgui_spin_button_new_with_range (0, 100, 1);
  bobgui_orientable_set_orientation (BOBGUI_ORIENTABLE (child), BOBGUI_ORIENTATION_VERTICAL);
  bobgui_widget_set_valign (child, BOBGUI_ALIGN_BASELINE_CENTER);
  bobgui_box_append (BOBGUI_BOX (box), child);

  child = bobgui_switch_new ();
  bobgui_widget_set_valign (child, BOBGUI_ALIGN_BASELINE_CENTER);
  bobgui_box_append (BOBGUI_BOX (box), child);

  child = bobgui_scale_new_with_range (BOBGUI_ORIENTATION_HORIZONTAL, 0, 100, 1);
  bobgui_widget_set_size_request (child, 100, -1);
  //bobgui_scale_add_mark (BOBGUI_SCALE (child), 50, BOBGUI_POS_BOTTOM, "50");
  bobgui_widget_set_valign (child, BOBGUI_ALIGN_BASELINE_CENTER);
  bobgui_box_append (BOBGUI_BOX (box), child);

  bobgui_stack_add_titled (BOBGUI_STACK (stack), box, "controls", "Controls");

  box = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 20);
  bobgui_widget_set_margin_top (box, 20);
  bobgui_widget_set_margin_bottom (box, 20);
  bobgui_widget_set_margin_start (box, 20);
  bobgui_widget_set_margin_end (box, 20);
  bobgui_widget_set_valign (box, BOBGUI_ALIGN_BASELINE_CENTER);

  child = baseline_widget_new (60, 15, 20);
  bobgui_widget_set_hexpand (child, TRUE);
  bobgui_box_append (BOBGUI_BOX (box), child);

  child = bobgui_label_new ("Label");
  bobgui_widget_set_hexpand (child, TRUE);
  bobgui_widget_set_valign (child, BOBGUI_ALIGN_BASELINE_CENTER);
  bobgui_box_append (BOBGUI_BOX (box), child);

  child = bobgui_label_new ("Two\nlines");
  bobgui_widget_set_hexpand (child, TRUE);
  bobgui_widget_set_valign (child, BOBGUI_ALIGN_BASELINE_CENTER);
  bobgui_box_append (BOBGUI_BOX (box), child);

  child = bobgui_label_new ("<span size='large'>Large</span>");
  bobgui_widget_set_hexpand (child, TRUE);
  bobgui_label_set_use_markup (BOBGUI_LABEL (child), TRUE);
  bobgui_widget_set_valign (child, BOBGUI_ALIGN_BASELINE_CENTER);
  bobgui_box_append (BOBGUI_BOX (box), child);

  child = bobgui_label_new ("<span size='xx-large'>Huge</span>");
  bobgui_widget_set_hexpand (child, TRUE);
  bobgui_label_set_use_markup (BOBGUI_LABEL (child), TRUE);
  bobgui_widget_set_valign (child, BOBGUI_ALIGN_BASELINE_CENTER);
  bobgui_box_append (BOBGUI_BOX (box), child);

  child = bobgui_label_new ("<span underline='double'>Underlined</span>");
  bobgui_widget_set_hexpand (child, TRUE);
  bobgui_label_set_use_markup (BOBGUI_LABEL (child), TRUE);
  bobgui_widget_set_valign (child, BOBGUI_ALIGN_BASELINE_CENTER);
  bobgui_box_append (BOBGUI_BOX (box), child);

  child = bobgui_label_new ("♥️");
  bobgui_widget_set_hexpand (child, TRUE);
  bobgui_widget_set_valign (child, BOBGUI_ALIGN_BASELINE_CENTER);
  bobgui_box_append (BOBGUI_BOX (box), child);

  child = bobgui_image_new_from_icon_name ("edit-copy-symbolic");
  bobgui_widget_set_hexpand (child, TRUE);
  bobgui_widget_set_valign (child, BOBGUI_ALIGN_BASELINE_CENTER);
  bobgui_box_append (BOBGUI_BOX (box), child);

  bobgui_stack_add_titled (BOBGUI_STACK (stack), box, "labels", "Labels");

  bobgui_window_present (BOBGUI_WINDOW (window));

  while (g_list_model_get_n_items (bobgui_window_get_toplevels ()) > 0)
    g_main_context_iteration (NULL, FALSE);

  return 0;
}
