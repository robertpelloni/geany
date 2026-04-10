#include "config.h"

#include "bobguilayoutchild.h"

#include "bobguilayoutmanager.h"
#include "bobguiprivate.h"

/**
 * BobguiLayoutChild:
 *
 * The base class for objects that are meant to hold layout properties.
 *
 * If a `BobguiLayoutManager` has per-child properties, like their packing type,
 * or the horizontal and vertical span, or the icon name, then the layout
 * manager should use a `BobguiLayoutChild` implementation to store those properties.
 *
 * A `BobguiLayoutChild` instance is only ever valid while a widget is part
 * of a layout.
 */

typedef struct {
  BobguiLayoutManager *manager;
  BobguiWidget *widget;
} BobguiLayoutChildPrivate;

G_DEFINE_ABSTRACT_TYPE_WITH_PRIVATE (BobguiLayoutChild, bobgui_layout_child, G_TYPE_OBJECT)

enum {
  PROP_LAYOUT_MANAGER = 1,
  PROP_CHILD_WIDGET,

  N_PROPS
};

static GParamSpec *layout_child_properties[N_PROPS];

static void
bobgui_layout_child_set_property (GObject      *gobject,
                               guint         prop_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  BobguiLayoutChild *layout_child = BOBGUI_LAYOUT_CHILD (gobject);
  BobguiLayoutChildPrivate *priv = bobgui_layout_child_get_instance_private (layout_child);

  switch (prop_id)
    {
    case PROP_LAYOUT_MANAGER:
      priv->manager = g_value_get_object (value);
      break;

    case PROP_CHILD_WIDGET:
      priv->widget = g_value_get_object (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
    }
}

static void
bobgui_layout_child_get_property (GObject      *gobject,
                               guint         prop_id,
                               GValue       *value,
                               GParamSpec   *pspec)
{
  BobguiLayoutChild *layout_child = BOBGUI_LAYOUT_CHILD (gobject);
  BobguiLayoutChildPrivate *priv = bobgui_layout_child_get_instance_private (layout_child);

  switch (prop_id)
    {
    case PROP_LAYOUT_MANAGER:
      g_value_set_object (value, priv->manager);
      break;

    case PROP_CHILD_WIDGET:
      g_value_set_object (value, priv->widget);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
    }
}

static void
bobgui_layout_child_constructed (GObject *gobject)
{
  BobguiLayoutChild *layout_child = BOBGUI_LAYOUT_CHILD (gobject);
  BobguiLayoutChildPrivate *priv = bobgui_layout_child_get_instance_private (layout_child);

  G_OBJECT_CLASS (bobgui_layout_child_parent_class)->constructed (gobject);

  if (priv->manager == NULL)
    {
      g_critical ("The layout child of type %s does not have "
                  "the BobguiLayoutChild:layout-manager property set",
                  G_OBJECT_TYPE_NAME (gobject));
      return;
    }

  if (priv->widget == NULL)
    {
      g_critical ("The layout child of type %s does not have "
                  "the BobguiLayoutChild:child-widget property set",
                  G_OBJECT_TYPE_NAME (gobject));
      return;
    }
}

static void
bobgui_layout_child_class_init (BobguiLayoutChildClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->set_property = bobgui_layout_child_set_property;
  gobject_class->get_property = bobgui_layout_child_get_property;
  gobject_class->constructed = bobgui_layout_child_constructed;

  /**
   * BobguiLayoutChild:layout-manager:
   *
   * The layout manager that created the `BobguiLayoutChild` instance.
   */
  layout_child_properties[PROP_LAYOUT_MANAGER] =
    g_param_spec_object ("layout-manager", NULL, NULL,
                         BOBGUI_TYPE_LAYOUT_MANAGER,
                         BOBGUI_PARAM_READWRITE |
                         G_PARAM_CONSTRUCT_ONLY);

  /**
   * BobguiLayoutChild:child-widget:
   *
   * The widget that is associated to the `BobguiLayoutChild` instance.
   */
  layout_child_properties[PROP_CHILD_WIDGET] =
    g_param_spec_object ("child-widget", NULL, NULL,
                         BOBGUI_TYPE_WIDGET,
                         BOBGUI_PARAM_READWRITE |
                         G_PARAM_CONSTRUCT_ONLY);

  g_object_class_install_properties (gobject_class, N_PROPS, layout_child_properties);
}

static void
bobgui_layout_child_init (BobguiLayoutChild *self)
{
}

/**
 * bobgui_layout_child_get_layout_manager:
 * @layout_child: a `BobguiLayoutChild`
 *
 * Retrieves the `BobguiLayoutManager` instance that created the
 * given @layout_child.
 *
 * Returns: (transfer none): a `BobguiLayoutManager`
 */
BobguiLayoutManager *
bobgui_layout_child_get_layout_manager (BobguiLayoutChild *layout_child)
{
  BobguiLayoutChildPrivate *priv = bobgui_layout_child_get_instance_private (layout_child);

  g_return_val_if_fail (BOBGUI_IS_LAYOUT_CHILD (layout_child), NULL);

  return priv->manager;
}

/**
 * bobgui_layout_child_get_child_widget:
 * @layout_child: a `BobguiLayoutChild`
 *
 * Retrieves the `BobguiWidget` associated to the given @layout_child.
 *
 * Returns: (transfer none): a `BobguiWidget`
 */
BobguiWidget *
bobgui_layout_child_get_child_widget (BobguiLayoutChild *layout_child)
{
  BobguiLayoutChildPrivate *priv = bobgui_layout_child_get_instance_private (layout_child);

  g_return_val_if_fail (BOBGUI_IS_LAYOUT_CHILD (layout_child), NULL);

  return priv->widget;
}
