/* BOBGUI - The Bobgui Framework
 * Copyright (C) 2014 Benjamin Otte <otte@gnome.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include "bobguicssnodeprivate.h"

#include "bobguicssstaticstyleprivate.h"
#include "bobguicssanimatedstyleprivate.h"
#include "bobguicssstylepropertyprivate.h"
#include "bobguimarshalers.h"
#include "bobguisettingsprivate.h"
#include "bobguitypebuiltins.h"
#include "bobguiprivate.h"
#include "gdkprofilerprivate.h"

/*
 * CSS nodes are the backbone of the BobguiStyleContext implementation and
 * replace the role that BobguiWidgetPath played in the past. A CSS node has
 * an element name and a state, and can have an id and style classes, which
 * is what is needed to determine the matching CSS selectors. CSS nodes have
 * a 'visible' property, which makes it possible to temporarily 'hide' them
 * from CSS matching - e.g. an invisible node will not affect :nth-child
 * matching and so forth.
 *
 * The API to manage states, names, ids and classes of CSS nodes is:
 * - bobgui_css_node_get/set_state. States are represented as BobguiStateFlags
 * - bobgui_css_node_get/set_name. Names are represented as interned strings
 * - bobgui_css_node_get/set_id. Ids are represented as interned strings
 * - bobgui_css_node_add/remove/has_class and bobgui_css_node_list_classes. Style
 *   classes are represented as quarks.
 *
 * CSS nodes are organized in a dom-like tree, and there is API to navigate
 * and manipulate this tree:
 * - bobgui_css_node_set_parent
 * - bobgui_css_node_insert_before/after
 * - bobgui_css_node_get_parent
 * - bobgui_css_node_get_first/last_child
 * - bobgui_css_node_get_previous/next_sibling
 * Note that parents keep a reference on their children in this tree.
 *
 * Every widget has one or more CSS nodes - the first one gets created
 * automatically by BobguiStyleContext. To set the name of the main node,
 * call bobgui_widget_class_set_css_name() in class_init(). Widget implementations
 * can and should add subnodes as suitable.
 *
 * Best practice is:
 * - For permanent subnodes, create them in init(), and keep a pointer
 *   to the node (you don't have to keep a reference, cleanup will be
 *   automatic by means of the parent node getting cleaned up by the
 *   style context).
 * - For transient nodes, create/destroy them when the conditions that
 *   warrant their existence change.
 * - Keep the state of all your nodes up-to-date. This probably requires
 *   a ::state-flags-changed (and possibly ::direction-changed) handler,
 *   as well as code to update the state in other places. Note that BOBGUI
 *   does this automatically for the widget's main CSS node.
 * - The sibling ordering in the CSS node tree is supposed to correspond
 *   to the visible order of content: top-to-bottom and left-to-right.
 *   Reorder your nodes to maintain this correlation. In particular for
 *   horizontally laid out widgets, this will require listening to
 *   ::direction-changed.
 * - The draw function should just use bobgui_style_context_save_to_node() to
 *   'switch' to the right node, not make any other changes to the style
 *   context.
 *
 * A noteworthy difference between bobgui_style_context_save() and
 * bobgui_style_context_save_to_node() is that the former inherits all the
 * style classes from the main CSS node, which often leads to unintended
 * inheritance.
 */

/* When these change we do a full restyling. Otherwise we try to figure out
 * if we need to change things. */
#define BOBGUI_CSS_RADICAL_CHANGE (BOBGUI_CSS_CHANGE_ID | BOBGUI_CSS_CHANGE_NAME | BOBGUI_CSS_CHANGE_CLASS | \
                                BOBGUI_CSS_CHANGE_PARENT_ID | BOBGUI_CSS_CHANGE_PARENT_NAME | BOBGUI_CSS_CHANGE_PARENT_CLASS | \
                                BOBGUI_CSS_CHANGE_SOURCE | BOBGUI_CSS_CHANGE_PARENT_STYLE)

/* When these change, we need to recompute the change flags for the new style
 * since they may have changed.
 */
#define BOBGUI_CSS_CHANGE_NEEDS_RECOMPUTE (BOBGUI_CSS_RADICAL_CHANGE & ~BOBGUI_CSS_CHANGE_PARENT_STYLE)

G_DEFINE_TYPE (BobguiCssNode, bobgui_css_node, G_TYPE_OBJECT)

enum {
  STYLE_CHANGED,
  LAST_SIGNAL
};

enum {
  PROP_0,
  PROP_CLASSES,
  PROP_ID,
  PROP_NAME,
  PROP_STATE,
  PROP_VISIBLE,
  NUM_PROPERTIES
};

static guint cssnode_signals[LAST_SIGNAL] = { 0 };
static GParamSpec *cssnode_properties[NUM_PROPERTIES];

static BobguiStyleProvider *
bobgui_css_node_get_style_provider_or_null (BobguiCssNode *cssnode)
{
  return BOBGUI_CSS_NODE_GET_CLASS (cssnode)->get_style_provider (cssnode);
}

static int invalidated_nodes;
static int created_styles;
static guint invalidated_nodes_counter;
static guint created_styles_counter;

static void
bobgui_css_node_set_invalid (BobguiCssNode *node,
                          gboolean    invalid)
{
  if (node->invalid == invalid)
    return;

  node->invalid = invalid;

  if (invalid)
    invalidated_nodes++;

  if (node->visible)
    {
      if (node->parent)
        {
          if (invalid)
            bobgui_css_node_set_invalid (node->parent, TRUE);
        }
      else
        {
          if (invalid)
            BOBGUI_CSS_NODE_GET_CLASS (node)->queue_validate (node);
          else
            BOBGUI_CSS_NODE_GET_CLASS (node)->dequeue_validate (node);
        }
    }
}

static void
bobgui_css_node_get_property (GObject    *object,
                           guint       property_id,
                           GValue     *value,
                           GParamSpec *pspec)
{
  BobguiCssNode *cssnode = BOBGUI_CSS_NODE (object);

  switch (property_id)
    {
    case PROP_CLASSES:
      g_value_take_boxed (value, bobgui_css_node_get_classes (cssnode));
      break;

    case PROP_ID:
      g_value_set_string (value, g_quark_to_string (bobgui_css_node_get_id (cssnode)));
      break;

    case PROP_NAME:
      g_value_set_string (value, g_quark_to_string (bobgui_css_node_get_name (cssnode)));
      break;

    case PROP_STATE:
      g_value_set_flags (value, bobgui_css_node_get_state (cssnode));
      break;

    case PROP_VISIBLE:
      g_value_set_boolean (value, bobgui_css_node_get_visible (cssnode));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
bobgui_css_node_set_property (GObject      *object,
                           guint         property_id,
                           const GValue *value,
                           GParamSpec   *pspec)
{
  BobguiCssNode *cssnode = BOBGUI_CSS_NODE (object);

  switch (property_id)
    {
    case PROP_CLASSES:
      bobgui_css_node_set_classes (cssnode, g_value_get_boxed (value));
      break;

    case PROP_ID:
      bobgui_css_node_set_id (cssnode, g_quark_from_string (g_value_get_string (value)));
      break;

    case PROP_NAME:
      bobgui_css_node_set_name (cssnode, g_quark_from_string (g_value_get_string (value)));
      break;

    case PROP_STATE:
      bobgui_css_node_set_state (cssnode, g_value_get_flags (value));
      break;

    case PROP_VISIBLE:
      bobgui_css_node_set_visible (cssnode, g_value_get_boolean (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
bobgui_css_node_dispose (GObject *object)
{
  BobguiCssNode *cssnode = BOBGUI_CSS_NODE (object);

  while (cssnode->first_child)
    {
      bobgui_css_node_set_parent (cssnode->first_child, NULL);
    }

  bobgui_css_node_set_invalid (cssnode, FALSE);

  g_clear_pointer (&cssnode->cache, bobgui_css_node_style_cache_unref);

 if (cssnode->children_observer)
   bobgui_list_list_model_clear (cssnode->children_observer);

  G_OBJECT_CLASS (bobgui_css_node_parent_class)->dispose (object);
}

static void
bobgui_css_node_finalize (GObject *object)
{
  BobguiCssNode *cssnode = BOBGUI_CSS_NODE (object);

  if (cssnode->style)
    g_object_unref (cssnode->style);
  bobgui_css_node_declaration_unref (cssnode->decl);

  G_OBJECT_CLASS (bobgui_css_node_parent_class)->finalize (object);
}

static gboolean
bobgui_css_node_is_first_child (BobguiCssNode *node)
{
  BobguiCssNode *iter;

  for (iter = node->previous_sibling;
       iter != NULL;
       iter = iter->previous_sibling)
    {
      if (iter->visible)
        return FALSE;
    }
  return TRUE;
}

static gboolean
bobgui_css_node_is_last_child (BobguiCssNode *node)
{
  BobguiCssNode *iter;

  for (iter = node->next_sibling;
       iter != NULL;
       iter = iter->next_sibling)
    {
      if (iter->visible)
        return FALSE;
    }
  return TRUE;
}

static gboolean
may_use_global_parent_cache (BobguiCssNode *node)
{
  BobguiStyleProvider *provider;
  BobguiCssNode *parent;

  parent = bobgui_css_node_get_parent (node);
  if (parent == NULL)
    return FALSE;

  provider = bobgui_css_node_get_style_provider_or_null (node);
  if (provider != NULL && provider != bobgui_css_node_get_style_provider (parent))
    return FALSE;

  return TRUE;
}

static BobguiCssStyle *
lookup_in_global_parent_cache (BobguiCssNode                  *node,
                               const BobguiCssNodeDeclaration *decl)
{
  BobguiCssNode *parent;

  parent = node->parent;

  if (parent == NULL ||
      !may_use_global_parent_cache (node))
    return NULL;

  if (parent->cache == NULL)
    return NULL;

  g_assert (node->cache == NULL);
  node->cache = bobgui_css_node_style_cache_lookup (parent->cache,
                                                 decl,
                                                 bobgui_css_node_is_first_child (node),
                                                 bobgui_css_node_is_last_child (node));
  if (node->cache == NULL)
    return NULL;

  return bobgui_css_node_style_cache_get_style (node->cache);
}

static void
store_in_global_parent_cache (BobguiCssNode                  *node,
                              const BobguiCssNodeDeclaration *decl,
                              BobguiCssStyle                 *style)
{
  BobguiCssNode *parent;

  g_assert (BOBGUI_IS_CSS_STATIC_STYLE (style));

  parent = node->parent;

  if (parent == NULL ||
      !may_use_global_parent_cache (node))
    return;

  if (parent->cache == NULL)
    parent->cache = bobgui_css_node_style_cache_new (parent->style);

  node->cache = bobgui_css_node_style_cache_insert (parent->cache,
                                                 (BobguiCssNodeDeclaration *) decl,
                                                 bobgui_css_node_is_first_child (node),
                                                 bobgui_css_node_is_last_child (node),
                                                 style);
}

static BobguiCssStyle *
bobgui_css_node_create_style (BobguiCssNode                   *cssnode,
                           const BobguiCountingBloomFilter *filter,
                           BobguiCssChange                  change)
{
  const BobguiCssNodeDeclaration *decl;
  BobguiCssStyle *style;
  BobguiCssChange style_change;

  decl = bobgui_css_node_get_declaration (cssnode);

  style = lookup_in_global_parent_cache (cssnode, decl);
  if (style)
    return g_object_ref (style);

  created_styles++;

  if (change & BOBGUI_CSS_CHANGE_NEEDS_RECOMPUTE)
    {
      /* Need to recompute the change flags */
      style_change = 0;
    }
  else
    {
      style_change = bobgui_css_static_style_get_change (bobgui_css_style_get_static_style (cssnode->style));
    }

  style = bobgui_css_static_style_new_compute (bobgui_css_node_get_style_provider (cssnode),
                                            filter,
                                            cssnode,
                                            style_change);

  store_in_global_parent_cache (cssnode, decl, style);

  return style;
}

static gboolean
should_create_transitions (BobguiCssChange change)
{
  return (change & BOBGUI_CSS_CHANGE_ANIMATIONS) == 0;
}

static gboolean
bobgui_css_style_needs_recreation (BobguiCssStyle  *style,
                                BobguiCssChange  change)
{
  bobgui_internal_return_val_if_fail (BOBGUI_IS_CSS_STATIC_STYLE (style), TRUE);

  /* Try to avoid invalidating if we can */
  if (change & BOBGUI_CSS_RADICAL_CHANGE)
    return TRUE;

  if (bobgui_css_static_style_get_change (BOBGUI_CSS_STATIC_STYLE (style)) & change)
    return TRUE;
  else
    return FALSE;
}

static BobguiCssStyle *
bobgui_css_node_real_update_style (BobguiCssNode                   *cssnode,
                                const BobguiCountingBloomFilter *filter,
                                BobguiCssChange                  change,
                                gint64                        timestamp,
                                BobguiCssStyle                  *style)
{
  BobguiCssStyle *static_style, *new_static_style, *new_style;

  static_style = BOBGUI_CSS_STYLE (bobgui_css_style_get_static_style (style));

  if (bobgui_css_style_needs_recreation (static_style, change))
    new_static_style = bobgui_css_node_create_style (cssnode, filter, change);
  else
    new_static_style = g_object_ref (static_style);

  if (new_static_style != static_style || (change & BOBGUI_CSS_CHANGE_ANIMATIONS))
    {
      BobguiCssNode *parent = bobgui_css_node_get_parent (cssnode);
      new_style = bobgui_css_animated_style_new (new_static_style,
                                              parent ? bobgui_css_node_get_style (parent) : NULL,
                                              timestamp,
                                              bobgui_css_node_get_style_provider (cssnode),
                                              should_create_transitions (change) ? style : NULL);

      /* Clear the cache again, the static style we looked up above
       * may have populated it. */
      g_clear_pointer (&cssnode->cache, bobgui_css_node_style_cache_unref);
    }
  else if (static_style != style && (change & BOBGUI_CSS_CHANGE_TIMESTAMP))
    {
      BobguiCssNode *parent = bobgui_css_node_get_parent (cssnode);
      new_style = bobgui_css_animated_style_new_advance (BOBGUI_CSS_ANIMATED_STYLE (style),
                                                      static_style,
                                                      parent ? bobgui_css_node_get_style (parent) : NULL,
                                                      timestamp,
                                                      bobgui_css_node_get_style_provider (cssnode));
    }
  else
    {
      new_style = g_object_ref (style);
    }

  if (!bobgui_css_style_is_static (new_style))
    bobgui_css_node_set_invalid (cssnode, TRUE);

  g_object_unref (new_static_style);

  return new_style;
}

static void
bobgui_css_node_real_queue_validate (BobguiCssNode *node)
{
}

static void
bobgui_css_node_real_dequeue_validate (BobguiCssNode *node)
{
}

static void
bobgui_css_node_real_validate (BobguiCssNode *node)
{
}

static BobguiStyleProvider *
bobgui_css_node_real_get_style_provider (BobguiCssNode *cssnode)
{
  return NULL;
}

static GdkFrameClock *
bobgui_css_node_real_get_frame_clock (BobguiCssNode *cssnode)
{
  return NULL;
}

static void
bobgui_css_node_real_node_removed (BobguiCssNode *parent,
                                BobguiCssNode *node,
                                BobguiCssNode *previous)
{
  if (node->previous_sibling)
    node->previous_sibling->next_sibling = node->next_sibling;
  else
    node->parent->first_child = node->next_sibling;

  if (node->next_sibling)
    node->next_sibling->previous_sibling = node->previous_sibling;
  else
    node->parent->last_child = node->previous_sibling;

  node->previous_sibling = NULL;
  node->next_sibling = NULL;
  node->parent = NULL;
}

static void
bobgui_css_node_real_node_added (BobguiCssNode *parent,
                              BobguiCssNode *node,
                              BobguiCssNode *new_previous)
{
  if (new_previous)
    {
      node->previous_sibling = new_previous;
      node->next_sibling = new_previous->next_sibling;
      new_previous->next_sibling = node;
    }
  else
    {
      node->next_sibling = parent->first_child;
      parent->first_child = node;
    }

  if (node->next_sibling)
    node->next_sibling->previous_sibling = node;
  else
    parent->last_child = node;

  node->parent = parent;
}

static void
bobgui_css_node_real_style_changed (BobguiCssNode        *cssnode,
                                 BobguiCssStyleChange *change)
{
  g_set_object (&cssnode->style, bobgui_css_style_change_get_new_style (change));
}

static void
bobgui_css_node_class_init (BobguiCssNodeClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->get_property = bobgui_css_node_get_property;
  object_class->set_property = bobgui_css_node_set_property;
  object_class->dispose = bobgui_css_node_dispose;
  object_class->finalize = bobgui_css_node_finalize;

  klass->update_style = bobgui_css_node_real_update_style;
  klass->validate = bobgui_css_node_real_validate;
  klass->queue_validate = bobgui_css_node_real_queue_validate;
  klass->dequeue_validate = bobgui_css_node_real_dequeue_validate;
  klass->get_style_provider = bobgui_css_node_real_get_style_provider;
  klass->get_frame_clock = bobgui_css_node_real_get_frame_clock;

  klass->node_added = bobgui_css_node_real_node_added;
  klass->node_removed = bobgui_css_node_real_node_removed;
  klass->style_changed = bobgui_css_node_real_style_changed;

  cssnode_signals[STYLE_CHANGED] =
    g_signal_new (I_("style-changed"),
                  G_TYPE_FROM_CLASS (object_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (BobguiCssNodeClass, style_changed),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 1,
                  G_TYPE_POINTER);

  cssnode_properties[PROP_CLASSES] =
    g_param_spec_boxed ("classes", NULL, NULL,
                         G_TYPE_STRV,
                         G_PARAM_READWRITE
                         | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);
  cssnode_properties[PROP_ID] =
    g_param_spec_string ("id", NULL, NULL,
                         NULL,
                         G_PARAM_READWRITE
                         | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);
  cssnode_properties[PROP_NAME] =
    g_param_spec_string ("name", NULL, NULL,
                         NULL,
                         G_PARAM_READWRITE
                         | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);
  cssnode_properties[PROP_STATE] =
    g_param_spec_flags ("state", NULL, NULL,
                        BOBGUI_TYPE_STATE_FLAGS,
                        0,
                        G_PARAM_READWRITE
                        | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);
  cssnode_properties[PROP_VISIBLE] =
    g_param_spec_boolean ("visible", NULL, NULL,
                          TRUE,
                          G_PARAM_READWRITE
                          | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, NUM_PROPERTIES, cssnode_properties);

  if (invalidated_nodes_counter == 0)
    {
      invalidated_nodes_counter = gdk_profiler_define_int_counter ("invalidated-nodes", "CSS Node Invalidations");
      created_styles_counter = gdk_profiler_define_int_counter ("created-styles", "CSS Style Creations");
    }
}

static void
bobgui_css_node_init (BobguiCssNode *cssnode)
{
  cssnode->decl = bobgui_css_node_declaration_new ();

  cssnode->style = g_object_ref (bobgui_css_static_style_get_default ());

  cssnode->visible = TRUE;
}

/**
 * bobgui_css_node_new:
 *
 * Creates a new CSS node.
 *
 * Returns: (transfer full): the new CSS node
 */
BobguiCssNode *
bobgui_css_node_new (void)
{
  return g_object_new (BOBGUI_TYPE_CSS_NODE, NULL);
}

static GdkFrameClock *
bobgui_css_node_get_frame_clock_or_null (BobguiCssNode *cssnode)
{
  while (cssnode->parent)
    cssnode = cssnode->parent;

  return BOBGUI_CSS_NODE_GET_CLASS (cssnode)->get_frame_clock (cssnode);
}

static gint64
bobgui_css_node_get_timestamp (BobguiCssNode *cssnode)
{
  GdkFrameClock *frameclock;

  frameclock = bobgui_css_node_get_frame_clock_or_null (cssnode);
  if (frameclock == NULL)
    return 0;

  return gdk_frame_clock_get_frame_time (frameclock);
}

static void
bobgui_css_node_parent_was_unset (BobguiCssNode *node)
{
  if (node->visible && node->invalid)
    BOBGUI_CSS_NODE_GET_CLASS (node)->queue_validate (node);
}

static void
bobgui_css_node_parent_will_be_set (BobguiCssNode *node)
{
  if (node->visible && node->invalid)
    BOBGUI_CSS_NODE_GET_CLASS (node)->dequeue_validate (node);
}

static void
bobgui_css_node_invalidate_style (BobguiCssNode *cssnode)
{
  if (cssnode->style_is_invalid)
    return;

  cssnode->style_is_invalid = TRUE;
  bobgui_css_node_set_invalid (cssnode, TRUE);

  if (cssnode->first_child)
    bobgui_css_node_invalidate_style (cssnode->first_child);

  if (cssnode->next_sibling)
    bobgui_css_node_invalidate_style (cssnode->next_sibling);
}

static void
bobgui_css_node_reposition (BobguiCssNode *node,
                         BobguiCssNode *new_parent,
                         BobguiCssNode *previous)
{
  BobguiCssNode *old_parent;
  BobguiCssNode *old_previous;

  g_assert (! (new_parent == NULL && previous != NULL));

  old_parent = node->parent;
  old_previous = node->previous_sibling;

  /* Take a reference here so the whole function has a reference */
  g_object_ref (node);

  if (node->visible)
    {
      if (node->next_sibling)
        bobgui_css_node_invalidate (node->next_sibling,
                                 BOBGUI_CSS_CHANGE_ANY_SIBLING
                                 | BOBGUI_CSS_CHANGE_NTH_CHILD
                                 | (node->previous_sibling ? 0 : BOBGUI_CSS_CHANGE_FIRST_CHILD));
      else if (node->previous_sibling)
        bobgui_css_node_invalidate (node->previous_sibling, BOBGUI_CSS_CHANGE_LAST_CHILD);
    }

  if (old_parent != NULL)
    {
      BOBGUI_CSS_NODE_GET_CLASS (old_parent)->node_removed (old_parent, node, node->previous_sibling);
      if (old_parent->children_observer && old_parent != new_parent)
        bobgui_list_list_model_item_removed (old_parent->children_observer, old_previous);
      if (old_parent->first_child && node->visible)
        bobgui_css_node_invalidate (old_parent->first_child, BOBGUI_CSS_CHANGE_NTH_LAST_CHILD);
    }

  if (old_parent != new_parent)
    {
      if (old_parent == NULL)
        {
          bobgui_css_node_parent_will_be_set (node);
        }
      else
        {
          g_object_unref (node);
        }

      if (bobgui_css_node_get_style_provider_or_null (node) == NULL)
        bobgui_css_node_invalidate_style_provider (node);
      bobgui_css_node_invalidate (node, BOBGUI_CSS_CHANGE_TIMESTAMP | BOBGUI_CSS_CHANGE_ANIMATIONS);

      if (new_parent)
        {
          g_object_ref (node);

          if (node->pending_changes)
            new_parent->needs_propagation = TRUE;
          if (node->invalid && node->visible)
            bobgui_css_node_set_invalid (new_parent, TRUE);
        }
      else
        {
          bobgui_css_node_parent_was_unset (node);
        }
    }

  if (new_parent)
    {
      BOBGUI_CSS_NODE_GET_CLASS (new_parent)->node_added (new_parent, node, previous);
      if (node->visible)
        bobgui_css_node_invalidate (new_parent->first_child, BOBGUI_CSS_CHANGE_NTH_LAST_CHILD);
    }

  if (node->visible)
    {
      if (node->next_sibling)
        {
          if (node->previous_sibling == NULL)
            bobgui_css_node_invalidate (node->next_sibling, BOBGUI_CSS_CHANGE_FIRST_CHILD);
          else
            bobgui_css_node_invalidate_style (node->next_sibling);
        }
      else if (node->previous_sibling)
        {
          bobgui_css_node_invalidate (node->previous_sibling, BOBGUI_CSS_CHANGE_LAST_CHILD);
        }
    }
  else
    {
      if (node->next_sibling)
        bobgui_css_node_invalidate_style (node->next_sibling);
    }

  bobgui_css_node_invalidate (node, (old_parent != new_parent ? BOBGUI_CSS_CHANGE_ANY_PARENT : 0)
                                 | BOBGUI_CSS_CHANGE_ANY_SIBLING
                                 | BOBGUI_CSS_CHANGE_NTH_CHILD
                                 | (node->previous_sibling ? 0 : BOBGUI_CSS_CHANGE_FIRST_CHILD)
                                 | (node->next_sibling ? 0 : BOBGUI_CSS_CHANGE_LAST_CHILD));

  if (new_parent && new_parent->children_observer)
    {
      if (old_previous && old_parent == new_parent)
        bobgui_list_list_model_item_moved (new_parent->children_observer, node, old_previous);
      else
        bobgui_list_list_model_item_added (new_parent->children_observer, node);
    }

  g_object_unref (node);
}

void
bobgui_css_node_set_parent (BobguiCssNode *node,
                         BobguiCssNode *parent)
{
  if (node->parent == parent)
    return;

  bobgui_css_node_reposition (node, parent, parent ? parent->last_child : NULL);
}

/* If previous_sibling is NULL, insert at the beginning */
void
bobgui_css_node_insert_after (BobguiCssNode *parent,
                           BobguiCssNode *cssnode,
                           BobguiCssNode *previous_sibling)
{
  g_return_if_fail (previous_sibling == NULL || previous_sibling->parent == parent);
  g_return_if_fail (cssnode != previous_sibling);

  if (cssnode->previous_sibling == previous_sibling &&
      cssnode->parent == parent)
    return;

  bobgui_css_node_reposition (cssnode,
                           parent,
                           previous_sibling);
}

/* If next_sibling is NULL, insert at the end */
void
bobgui_css_node_insert_before (BobguiCssNode *parent,
                            BobguiCssNode *cssnode,
                            BobguiCssNode *next_sibling)
{
  g_return_if_fail (next_sibling == NULL || next_sibling->parent == parent);
  g_return_if_fail (cssnode != next_sibling);

  if (cssnode->next_sibling == next_sibling &&
      cssnode->parent == parent)
    return;

  bobgui_css_node_reposition (cssnode,
                           parent,
                           next_sibling ? next_sibling->previous_sibling : parent->last_child);
}

BobguiCssNode *
bobgui_css_node_get_parent (BobguiCssNode *cssnode)
{
  return cssnode->parent;
}

BobguiCssNode *
bobgui_css_node_get_first_child (BobguiCssNode *cssnode)
{
  return cssnode->first_child;
}

BobguiCssNode *
bobgui_css_node_get_last_child (BobguiCssNode *cssnode)
{
  return cssnode->last_child;
}

BobguiCssNode *
bobgui_css_node_get_previous_sibling (BobguiCssNode *cssnode)
{
  return cssnode->previous_sibling;
}

BobguiCssNode *
bobgui_css_node_get_next_sibling (BobguiCssNode *cssnode)
{
  return cssnode->next_sibling;
}

static gboolean
bobgui_css_node_set_style (BobguiCssNode  *cssnode,
                        BobguiCssStyle *style)
{
  BobguiCssStyleChange change;
  gboolean style_changed;

  if (cssnode->style == style)
    return FALSE;

  bobgui_css_style_change_init (&change, cssnode->style, style);

  style_changed = bobgui_css_style_change_has_change (&change);
  if (style_changed)
    {
      g_signal_emit (cssnode, cssnode_signals[STYLE_CHANGED], 0, &change);
    }
  else if (BOBGUI_IS_CSS_ANIMATED_STYLE (cssnode->style) || BOBGUI_IS_CSS_ANIMATED_STYLE (style))
    {
      /* This is when animations are starting/stopping but they didn't change any CSS this frame */
      g_set_object (&cssnode->style, style);
    }
  else if (bobgui_css_static_style_get_change (bobgui_css_style_get_static_style (cssnode->style)) !=
           bobgui_css_static_style_get_change (bobgui_css_style_get_static_style (style)))
    {
      /* This is when we recomputed the change flags but the style didn't change */
      g_set_object (&cssnode->style, style);
    }

  bobgui_css_style_change_finish (&change);

  return style_changed;
}

static void
bobgui_css_node_propagate_pending_changes (BobguiCssNode *cssnode,
                                        gboolean    style_changed)
{
  BobguiCssChange change, child_change;
  BobguiCssNode *child;

  change = _bobgui_css_change_for_child (cssnode->pending_changes);
  if (style_changed)
    change |= BOBGUI_CSS_CHANGE_PARENT_STYLE;

  if (!cssnode->needs_propagation && change == 0)
    return;

  for (child = bobgui_css_node_get_first_child (cssnode);
       child;
       child = bobgui_css_node_get_next_sibling (child))
    {
      child_change = child->pending_changes;
      bobgui_css_node_invalidate (child, change);
      if (child->visible)
        change |= _bobgui_css_change_for_sibling (child_change);
    }

  cssnode->needs_propagation = FALSE;
}

static gboolean
bobgui_css_node_needs_new_style (BobguiCssNode *cssnode)
{
  return cssnode->style_is_invalid || cssnode->needs_propagation;
}

static void
bobgui_css_node_do_ensure_style (BobguiCssNode                   *cssnode,
                              const BobguiCountingBloomFilter *filter,
                              gint64                        current_time)
{
  gboolean style_changed;

  if (cssnode->style_is_invalid)
    {
      BobguiCssStyle *new_style;

      g_clear_pointer (&cssnode->cache, bobgui_css_node_style_cache_unref);

      new_style = BOBGUI_CSS_NODE_GET_CLASS (cssnode)->update_style (cssnode,
                                                                  filter,
                                                                  cssnode->pending_changes,
                                                                  current_time,
                                                                  cssnode->style);

      style_changed = bobgui_css_node_set_style (cssnode, new_style);
      g_object_unref (new_style);
    }
  else
    {
      style_changed = FALSE;
    }

  bobgui_css_node_propagate_pending_changes (cssnode, style_changed);

  cssnode->pending_changes = 0;
  cssnode->style_is_invalid = FALSE;
}

static void
bobgui_css_node_ensure_style (BobguiCssNode                   *cssnode,
                           const BobguiCountingBloomFilter *filter,
                           gint64                        current_time)
{
  BobguiCssNode *sibling;

  if (!bobgui_css_node_needs_new_style (cssnode))
    return;

  if (cssnode->parent)
    bobgui_css_node_ensure_style (cssnode->parent, filter, current_time);

  /* Ensure all siblings before this have a valid style, in order
   * starting at the first that needs it. */
  sibling = cssnode;
  while (sibling->style_is_invalid &&
         sibling->previous_sibling != NULL &&
         bobgui_css_node_needs_new_style (sibling->previous_sibling))
    sibling = sibling->previous_sibling;

  while (sibling != cssnode)
    {
      bobgui_css_node_do_ensure_style (sibling, filter, current_time);
      sibling = sibling->next_sibling;
    }

  bobgui_css_node_do_ensure_style (cssnode, filter, current_time);
}

BobguiCssStyle *
bobgui_css_node_get_style (BobguiCssNode *cssnode)
{
  if (bobgui_css_node_needs_new_style (cssnode))
    {
      gint64 timestamp = bobgui_css_node_get_timestamp (cssnode);

      bobgui_css_node_ensure_style (cssnode, NULL, timestamp);
    }

  return cssnode->style;
}

void
bobgui_css_node_set_visible (BobguiCssNode *cssnode,
                          gboolean    visible)
{
  BobguiCssNode *iter;

  if (cssnode->visible == visible)
    return;

  cssnode->visible = visible;
  g_object_notify_by_pspec (G_OBJECT (cssnode), cssnode_properties[PROP_VISIBLE]);

  if (cssnode->invalid)
    {
      if (cssnode->visible)
        {
          if (cssnode->parent)
            bobgui_css_node_set_invalid (cssnode->parent, TRUE);
          else
            BOBGUI_CSS_NODE_GET_CLASS (cssnode)->queue_validate (cssnode);
        }
      else
        {
          if (cssnode->parent == NULL)
            BOBGUI_CSS_NODE_GET_CLASS (cssnode)->dequeue_validate (cssnode);
        }
    }

  if (cssnode->next_sibling)
    {
      bobgui_css_node_invalidate (cssnode->next_sibling, BOBGUI_CSS_CHANGE_ANY_SIBLING | BOBGUI_CSS_CHANGE_NTH_CHILD);
      if (bobgui_css_node_is_first_child (cssnode))
        {
          for (iter = cssnode->next_sibling;
               iter != NULL;
               iter = iter->next_sibling)
            {
              bobgui_css_node_invalidate (iter, BOBGUI_CSS_CHANGE_FIRST_CHILD);
              if (iter->visible)
                break;
            }
        }
    }

  if (cssnode->previous_sibling)
    {
      if (bobgui_css_node_is_last_child (cssnode))
        {
          for (iter = cssnode->previous_sibling;
               iter != NULL;
               iter = iter->previous_sibling)
            {
              bobgui_css_node_invalidate (iter, BOBGUI_CSS_CHANGE_LAST_CHILD);
              if (iter->visible)
                break;
            }
        }
      bobgui_css_node_invalidate (cssnode->parent->first_child, BOBGUI_CSS_CHANGE_NTH_LAST_CHILD);
    }
}

gboolean
bobgui_css_node_get_visible (BobguiCssNode *cssnode)
{
  return cssnode->visible;
}

void
bobgui_css_node_set_name (BobguiCssNode *cssnode,
                       GQuark      name)
{
  if (bobgui_css_node_declaration_set_name (&cssnode->decl, name))
    {
      bobgui_css_node_invalidate (cssnode, BOBGUI_CSS_CHANGE_NAME);
      g_object_notify_by_pspec (G_OBJECT (cssnode), cssnode_properties[PROP_NAME]);
    }
}

GQuark
bobgui_css_node_get_name (BobguiCssNode *cssnode)
{
  return bobgui_css_node_declaration_get_name (cssnode->decl);
}

void
bobgui_css_node_set_id (BobguiCssNode *cssnode,
                     GQuark      id)
{
  if (bobgui_css_node_declaration_set_id (&cssnode->decl, id))
    {
      bobgui_css_node_invalidate (cssnode, BOBGUI_CSS_CHANGE_ID);
      g_object_notify_by_pspec (G_OBJECT (cssnode), cssnode_properties[PROP_ID]);
    }
}

GQuark
bobgui_css_node_get_id (BobguiCssNode *cssnode)
{
  return bobgui_css_node_declaration_get_id (cssnode->decl);
}

void
bobgui_css_node_set_state (BobguiCssNode    *cssnode,
                        BobguiStateFlags  state_flags)
{
  BobguiStateFlags old_state;

  old_state = bobgui_css_node_declaration_get_state (cssnode->decl);

  if (bobgui_css_node_declaration_set_state (&cssnode->decl, state_flags))
    {
      BobguiStateFlags states = old_state ^ state_flags;
      BobguiCssChange change = 0;

      if (states & BOBGUI_STATE_FLAG_PRELIGHT)
        change |= BOBGUI_CSS_CHANGE_HOVER;
      if (states & BOBGUI_STATE_FLAG_INSENSITIVE)
        change |= BOBGUI_CSS_CHANGE_DISABLED;
      if (states & BOBGUI_STATE_FLAG_BACKDROP)
        change |= BOBGUI_CSS_CHANGE_BACKDROP;
      if (states & BOBGUI_STATE_FLAG_SELECTED)
        change |= BOBGUI_CSS_CHANGE_SELECTED;
      if (states & ~(BOBGUI_STATE_FLAG_PRELIGHT |
                     BOBGUI_STATE_FLAG_INSENSITIVE |
                     BOBGUI_STATE_FLAG_BACKDROP |
                     BOBGUI_STATE_FLAG_SELECTED))
        change |= BOBGUI_CSS_CHANGE_STATE;

      bobgui_css_node_invalidate (cssnode, change);
      g_object_notify_by_pspec (G_OBJECT (cssnode), cssnode_properties[PROP_STATE]);
    }
}

BobguiStateFlags
bobgui_css_node_get_state (BobguiCssNode *cssnode)
{
  return bobgui_css_node_declaration_get_state (cssnode->decl);
}

static void
bobgui_css_node_clear_classes (BobguiCssNode *cssnode)
{
  if (bobgui_css_node_declaration_clear_classes (&cssnode->decl))
    {
      bobgui_css_node_invalidate (cssnode, BOBGUI_CSS_CHANGE_CLASS);
      g_object_notify_by_pspec (G_OBJECT (cssnode), cssnode_properties[PROP_CLASSES]);
    }
}

void
bobgui_css_node_set_classes (BobguiCssNode  *cssnode,
                          const char **classes)
{
  guint i;

  g_object_freeze_notify (G_OBJECT (cssnode));

  bobgui_css_node_clear_classes (cssnode);

  if (classes)
    {
      for (i = 0; classes[i] != NULL; i++)
        {
          bobgui_css_node_add_class (cssnode, g_quark_from_string (classes[i]));
        }
    }

  g_object_thaw_notify (G_OBJECT (cssnode));
}

char **
bobgui_css_node_get_classes (BobguiCssNode *cssnode)
{
  const GQuark *classes;
  char **result;
  guint n_classes, i, j;

  classes = bobgui_css_node_declaration_get_classes (cssnode->decl, &n_classes);
  result = g_new (char *, n_classes + 1);

  for (i = n_classes, j = 0; i-- > 0; ++j)
    {
      result[j] = g_strdup (g_quark_to_string (classes[i]));
    }

  result[n_classes] = NULL;
  return result;
}

gboolean
bobgui_css_node_add_class (BobguiCssNode *cssnode,
                        GQuark      style_class)
{
  if (bobgui_css_node_declaration_add_class (&cssnode->decl, style_class))
    {
      bobgui_css_node_invalidate (cssnode, BOBGUI_CSS_CHANGE_CLASS);
      g_object_notify_by_pspec (G_OBJECT (cssnode), cssnode_properties[PROP_CLASSES]);
      return TRUE;
    }

  return FALSE;
}

gboolean
bobgui_css_node_remove_class (BobguiCssNode *cssnode,
                           GQuark      style_class)
{
  if (bobgui_css_node_declaration_remove_class (&cssnode->decl, style_class))
    {
      bobgui_css_node_invalidate (cssnode, BOBGUI_CSS_CHANGE_CLASS);
      g_object_notify_by_pspec (G_OBJECT (cssnode), cssnode_properties[PROP_CLASSES]);
      return TRUE;
    }

  return FALSE;
}

gboolean
bobgui_css_node_has_class (BobguiCssNode *cssnode,
                        GQuark      style_class)
{
  return bobgui_css_node_declaration_has_class (cssnode->decl, style_class);
}

const GQuark *
bobgui_css_node_list_classes (BobguiCssNode *cssnode,
                           guint      *n_classes)
{
  return bobgui_css_node_declaration_get_classes (cssnode->decl, n_classes);
}

const BobguiCssNodeDeclaration *
bobgui_css_node_get_declaration (BobguiCssNode *cssnode)
{
  return cssnode->decl;
}

void
bobgui_css_node_invalidate_style_provider (BobguiCssNode *cssnode)
{
  BobguiCssNode *child;

  bobgui_css_node_invalidate (cssnode, BOBGUI_CSS_CHANGE_SOURCE);

  for (child = cssnode->first_child;
       child;
       child = child->next_sibling)
    {
      if (bobgui_css_node_get_style_provider_or_null (child) == NULL)
        bobgui_css_node_invalidate_style_provider (child);
    }
}

static void
bobgui_css_node_invalidate_timestamp (BobguiCssNode *cssnode)
{
  BobguiCssNode *child;

  if (!cssnode->invalid)
    return;

  if (!bobgui_css_style_is_static (cssnode->style))
    bobgui_css_node_invalidate (cssnode, BOBGUI_CSS_CHANGE_TIMESTAMP);

  for (child = cssnode->first_child; child; child = child->next_sibling)
    {
      bobgui_css_node_invalidate_timestamp (child);
    }
}

void
bobgui_css_node_invalidate_frame_clock (BobguiCssNode *cssnode,
                                     gboolean    just_timestamp)
{
  /* frame clock is handled by the top level */
  if (cssnode->parent)
    return;

  bobgui_css_node_invalidate_timestamp (cssnode);

  if (!just_timestamp)
    bobgui_css_node_invalidate (cssnode, BOBGUI_CSS_CHANGE_ANIMATIONS);
}

void
bobgui_css_node_invalidate (BobguiCssNode   *cssnode,
                         BobguiCssChange  change)
{
  if (!cssnode->invalid)
    change &= ~BOBGUI_CSS_CHANGE_TIMESTAMP;

  if (change == 0)
    return;

  cssnode->pending_changes |= change;

  if (cssnode->parent)
    cssnode->parent->needs_propagation = TRUE;
  bobgui_css_node_invalidate_style (cssnode);
}

static void
bobgui_css_node_validate_internal (BobguiCssNode             *cssnode,
                                BobguiCountingBloomFilter *filter,
                                gint64                  timestamp)
{
  BobguiCssNode *child;
  gboolean bloomed = FALSE;

  if (!cssnode->invalid)
    return;

  bobgui_css_node_ensure_style (cssnode, filter, timestamp);

  /* need to set to FALSE then to TRUE here to make it chain up */
  bobgui_css_node_set_invalid (cssnode, FALSE);
  if (!bobgui_css_style_is_static (cssnode->style))
    bobgui_css_node_set_invalid (cssnode, TRUE);

  BOBGUI_CSS_NODE_GET_CLASS (cssnode)->validate (cssnode);

  for (child = bobgui_css_node_get_first_child (cssnode);
       child;
       child = bobgui_css_node_get_next_sibling (child))
    {
      if (!child->visible)
        continue;

      if (!bloomed)
        {
          bobgui_css_node_declaration_add_bloom_hashes (cssnode->decl, filter);
          bloomed = TRUE;
        }

      bobgui_css_node_validate_internal (child, filter, timestamp);
    }

  if (bloomed)
    bobgui_css_node_declaration_remove_bloom_hashes (cssnode->decl, filter);
}

void
bobgui_css_node_validate (BobguiCssNode *cssnode)
{
  BobguiCountingBloomFilter filter = BOBGUI_COUNTING_BLOOM_FILTER_INIT;
  gint64 timestamp;
  gint64 before G_GNUC_UNUSED;

  before = GDK_PROFILER_CURRENT_TIME;

  g_assert (cssnode->parent == NULL);

  timestamp = bobgui_css_node_get_timestamp (cssnode);

  bobgui_css_node_validate_internal (cssnode, &filter, timestamp);

  if (GDK_PROFILER_IS_RUNNING)
    {
      gdk_profiler_end_mark (before,  "Validate CSS", "");
      gdk_profiler_set_int_counter (invalidated_nodes_counter, invalidated_nodes);
      gdk_profiler_set_int_counter (created_styles_counter, created_styles);
      invalidated_nodes = 0;
      created_styles = 0;
    }
}

BobguiStyleProvider *
bobgui_css_node_get_style_provider (BobguiCssNode *cssnode)
{
  BobguiStyleProvider *result;

  result = bobgui_css_node_get_style_provider_or_null (cssnode);
  if (result)
    return result;

  if (cssnode->parent)
    return bobgui_css_node_get_style_provider (cssnode->parent);

  return BOBGUI_STYLE_PROVIDER (_bobgui_settings_get_style_cascade (bobgui_settings_get_default (), 1));
}

void
bobgui_css_node_print (BobguiCssNode           *cssnode,
                    BobguiCssNodePrintFlags  flags,
                    GString              *string,
                    guint                 indent)
{
  gboolean need_newline = FALSE;

  g_string_append_printf (string, "%*s", indent, "");

  if (!cssnode->visible)
    g_string_append_c (string, '[');

  bobgui_css_node_declaration_print (cssnode->decl, string);

  if (!cssnode->visible)
    g_string_append_c (string, ']');

  if (flags & BOBGUI_CSS_NODE_PRINT_SHOW_CHANGE)
    {
      BobguiCssStyle *style = bobgui_css_node_get_style (cssnode);
      BobguiCssChange change;

      change = bobgui_css_static_style_get_change (bobgui_css_style_get_static_style (style));
      g_string_append (string, "    ");
      bobgui_css_change_print (change, string);
    }

  g_string_append_c (string, '\n');

  if (flags & BOBGUI_CSS_NODE_PRINT_SHOW_STYLE)
    need_newline = bobgui_css_style_print (bobgui_css_node_get_style (cssnode), string, indent + 2, TRUE);

  if (flags & BOBGUI_CSS_NODE_PRINT_RECURSE)
    {
      BobguiCssNode *node;

      if (need_newline && bobgui_css_node_get_first_child (cssnode))
        g_string_append_c (string, '\n');

      for (node = bobgui_css_node_get_first_child (cssnode); node; node = bobgui_css_node_get_next_sibling (node))
        bobgui_css_node_print (node, flags, string, indent + 2);
    }
}

static void
bobgui_css_node_child_observer_destroyed (gpointer cssnode)
{
  BOBGUI_CSS_NODE (cssnode)->children_observer = NULL;
}

GListModel *
bobgui_css_node_observe_children (BobguiCssNode *cssnode)
{
  if (cssnode->children_observer)
    return g_object_ref (G_LIST_MODEL (cssnode->children_observer));

  cssnode->children_observer = bobgui_list_list_model_new ((gpointer) bobgui_css_node_get_first_child,
                                                        (gpointer) bobgui_css_node_get_next_sibling,
                                                        (gpointer) bobgui_css_node_get_previous_sibling,
                                                        (gpointer) bobgui_css_node_get_last_child,
                                                        (gpointer) g_object_ref,
                                                        cssnode,
                                                        bobgui_css_node_child_observer_destroyed);

  return G_LIST_MODEL (cssnode->children_observer);
}

