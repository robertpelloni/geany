#include "component_filter.h"
#include "minigraph.h"

typedef enum {
  IDENTITY,
  LEVELS,
  LINEAR,
  GAMMA,
  DISCRETE,
  TABLE,
} FilterKind;

struct _ComponentFilter
{
  BobguiWidget parent_instance;

  BobguiDropDown *box;
  BobguiDropDown *kind;
  BobguiStack *stack;
  BobguiSpinButton *levels;
  BobguiSpinButton *linear_m;
  BobguiSpinButton *linear_b;
  BobguiSpinButton *gamma_amp;
  BobguiSpinButton *gamma_exp;
  BobguiSpinButton *gamma_ofs;
  BobguiSpinButton *discrete_size;
  union {
    BobguiSpinButton *discrete_values[6];
    struct {
      BobguiSpinButton *discrete_value0;
      BobguiSpinButton *discrete_value1;
      BobguiSpinButton *discrete_value2;
      BobguiSpinButton *discrete_value3;
      BobguiSpinButton *discrete_value4;
      BobguiSpinButton *discrete_value5;
    };
  };
  MiniGraph *graph;

  FilterKind filter_kind;

  GskComponentTransfer *component_transfer;
};

struct _ComponentFilterClass
{
  BobguiWidgetClass parent_class;
};

enum {
  PROP_TRANSFER = 1,
  NUM_PROPERTIES,
};

G_DEFINE_TYPE (ComponentFilter, component_filter, BOBGUI_TYPE_WIDGET)

static void
component_filter_init (ComponentFilter *self)
{
  bobgui_widget_init_template (BOBGUI_WIDGET (self));

  self->component_transfer = gsk_component_transfer_new_identity ();
}

static void
component_filter_dispose (GObject *object)
{
  ComponentFilter *self = COMPONENT_FILTER (object);

  bobgui_widget_dispose_template (BOBGUI_WIDGET (self), component_filter_get_type ());

  G_OBJECT_CLASS (component_filter_parent_class)->dispose (object);
}

static void
component_filter_finalize (GObject *object)
{
  ComponentFilter *self = COMPONENT_FILTER (object);

  gsk_component_transfer_free (self->component_transfer);

  G_OBJECT_CLASS (component_filter_parent_class)->finalize (object);
}

static void
update_component_transfer (ComponentFilter *self)
{
  const char *page[] = { "identity", "levels", "linear", "gamma", "discrete", "discrete" };

  self->filter_kind = bobgui_drop_down_get_selected (self->kind);
  bobgui_stack_set_visible_child_name (self->stack, page[self->filter_kind]);

  gsk_component_transfer_free (self->component_transfer);

  switch (self->filter_kind)
    {
    case IDENTITY:
      self->component_transfer = gsk_component_transfer_new_identity ();
      mini_graph_set_identity (self->graph);
      break;
    case LEVELS:
      {
        guint n = bobgui_spin_button_get_value_as_int (self->levels);
        self->component_transfer = gsk_component_transfer_new_levels (n);
        mini_graph_set_levels (self->graph, n);
      }
      break;
    case LINEAR:
      {
        float m = bobgui_spin_button_get_value (self->linear_m);
        float b = bobgui_spin_button_get_value (self->linear_b);
        self->component_transfer = gsk_component_transfer_new_linear (m, b);
        mini_graph_set_linear (self->graph, m, b);
      }
      break;
    case GAMMA:
      {
        float amp = bobgui_spin_button_get_value (self->gamma_amp);
        float exp = bobgui_spin_button_get_value (self->gamma_exp);
        float ofs = bobgui_spin_button_get_value (self->gamma_ofs);
        self->component_transfer = gsk_component_transfer_new_gamma (amp, exp, ofs);
        mini_graph_set_gamma (self->graph, amp, exp, ofs);
      }
      break;
    case DISCRETE:
      {
        guint n = bobgui_spin_button_get_value_as_int (self->discrete_size);
        float values[6];
        for (guint i = 0; i < 6; i++)
          {
            bobgui_widget_set_visible (BOBGUI_WIDGET (self->discrete_values[i]), i < n);
            values[i] = bobgui_spin_button_get_value (self->discrete_values[i]);
          }
        self->component_transfer = gsk_component_transfer_new_discrete (n, values);
        mini_graph_set_discrete (self->graph, n, values);
      }
      break;
    case TABLE:
      {
        guint n = bobgui_spin_button_get_value_as_int (self->discrete_size);
        float values[6];
        for (guint i = 0; i < 6; i++)
          {
            bobgui_widget_set_visible (BOBGUI_WIDGET (self->discrete_values[i]), i < n);
            values[i] = bobgui_spin_button_get_value (self->discrete_values[i]);
          }
        self->component_transfer = gsk_component_transfer_new_table (n, values);
        mini_graph_set_table (self->graph, n, values);
      }
      break;
    default:
      g_assert_not_reached ();
    }

  g_object_notify (G_OBJECT (self), "transfer");
}

static void
component_filter_get_property (GObject      *object,
                               guint         prop_id,
                               GValue       *value,
                               GParamSpec   *pspec)
{
  ComponentFilter *self = COMPONENT_FILTER (object);

  switch (prop_id)
    {
    case PROP_TRANSFER:
      g_value_set_boxed (value, self->component_transfer);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
component_filter_class_init (ComponentFilterClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (class);

  g_type_ensure (mini_graph_get_type ());

  object_class->dispose = component_filter_dispose;
  object_class->finalize = component_filter_finalize;
  object_class->get_property = component_filter_get_property;

  g_object_class_install_property (object_class,
                                   PROP_TRANSFER,
                                   g_param_spec_boxed ("transfer", NULL, NULL,
                                                       GSK_TYPE_COMPONENT_TRANSFER,
                                                       G_PARAM_READABLE));

  bobgui_widget_class_set_template_from_resource (widget_class, "/image_filter/component_filter.ui");

  bobgui_widget_class_bind_template_child (widget_class, ComponentFilter, box);
  bobgui_widget_class_bind_template_child (widget_class, ComponentFilter, kind);
  bobgui_widget_class_bind_template_child (widget_class, ComponentFilter, stack);
  bobgui_widget_class_bind_template_child (widget_class, ComponentFilter, levels);
  bobgui_widget_class_bind_template_child (widget_class, ComponentFilter, linear_m);
  bobgui_widget_class_bind_template_child (widget_class, ComponentFilter, linear_b);
  bobgui_widget_class_bind_template_child (widget_class, ComponentFilter, gamma_amp);
  bobgui_widget_class_bind_template_child (widget_class, ComponentFilter, gamma_exp);
  bobgui_widget_class_bind_template_child (widget_class, ComponentFilter, gamma_ofs);
  bobgui_widget_class_bind_template_child (widget_class, ComponentFilter, discrete_size);
  bobgui_widget_class_bind_template_child (widget_class, ComponentFilter, discrete_value0);
  bobgui_widget_class_bind_template_child (widget_class, ComponentFilter, discrete_value1);
  bobgui_widget_class_bind_template_child (widget_class, ComponentFilter, discrete_value2);
  bobgui_widget_class_bind_template_child (widget_class, ComponentFilter, discrete_value3);
  bobgui_widget_class_bind_template_child (widget_class, ComponentFilter, discrete_value4);
  bobgui_widget_class_bind_template_child (widget_class, ComponentFilter, discrete_value5);
  bobgui_widget_class_bind_template_child (widget_class, ComponentFilter, graph);
  bobgui_widget_class_bind_template_callback (widget_class, update_component_transfer);

  bobgui_widget_class_set_layout_manager_type (widget_class, BOBGUI_TYPE_BIN_LAYOUT);
}

BobguiWidget *
component_filter_new (void)
{
  return g_object_new (component_filter_get_type (), NULL);
}

GskComponentTransfer *
component_filter_get_component_transfer (ComponentFilter *self)
{
  return self->component_transfer;
}
