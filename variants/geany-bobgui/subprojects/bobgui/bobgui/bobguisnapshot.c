/* BOBGUI - The Bobgui Framework
 * Copyright (C) 2016 Benjamin Otte <otte@gnome.org>
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

#include "bobguisnapshot.h"
#include "bobguisnapshotprivate.h"

#include "bobguicsscolorvalueprivate.h"
#include "bobguicssshadowvalueprivate.h"
#include "bobguidebug.h"
#include "bobguirendernodepaintableprivate.h"
#include "gsktransformprivate.h"

#include "gdk/gdkrgbaprivate.h"
#include "gdk/gdkcolorstateprivate.h"

#include "gsk/gskbordernodeprivate.h"
#include "gsk/gskcolornodeprivate.h"
#include "gsk/gskconicgradientnodeprivate.h"
#include "gsk/gskinsetshadownodeprivate.h"
#include "gsk/gskisolationnodeprivate.h"
#include "gsk/gsklineargradientnodeprivate.h"
#include "gsk/gskoutsetshadownodeprivate.h"
#include "gsk/gskradialgradientnodeprivate.h"
#include "gsk/gskrendernodeprivate.h"
#include "gsk/gskrepeatnodeprivate.h"
#include "gsk/gskroundedrectprivate.h"
#include "gsk/gskstrokeprivate.h"
#include "gsk/gsktextnodeprivate.h"
#include "gsk/gskrectprivate.h"

#include "bobgui/gskpangoprivate.h"

#define GDK_ARRAY_NAME bobgui_snapshot_nodes
#define GDK_ARRAY_TYPE_NAME BobguiSnapshotNodes
#define GDK_ARRAY_ELEMENT_TYPE GskRenderNode *
#define GDK_ARRAY_FREE_FUNC gsk_render_node_unref
#include "gdk/gdkarrayimpl.c"

/**
 * BobguiSnapshot:
 *
 * Assists in creating [class@Gsk.RenderNode]s for widgets.
 *
 * It functions in a similar way to a cairo context, and maintains a stack
 * of render nodes and their associated transformations.
 *
 * The node at the top of the stack is the one that `bobgui_snapshot_append_…()`
 * functions operate on. Use the `bobgui_snapshot_push_…()` functions and
 * [method@Snapshot.pop] to change the current node.
 *
 * The typical way to obtain a `BobguiSnapshot` object is as an argument to
 * the [vfunc@Bobgui.Widget.snapshot] vfunc. If you need to create your own
 * `BobguiSnapshot`, use [ctor@Bobgui.Snapshot.new].
 *
 * Note that `BobguiSnapshot` applies some optimizations, so the node
 * it produces may not match the API calls 1:1. For example, it will
 * omit clip nodes if the child node is entirely contained within the
 * clip rectangle.
 */

typedef struct _BobguiSnapshotState BobguiSnapshotState;

typedef GskRenderNode * (* BobguiSnapshotCollectFunc) (BobguiSnapshot      *snapshot,
                                                    BobguiSnapshotState *state,
                                                    GskRenderNode   **nodes,
                                                    guint             n_nodes);
typedef void            (* BobguiSnapshotClearFunc)   (BobguiSnapshotState *state);

struct _BobguiSnapshotState {
  guint                  start_node_index;
  guint                  n_nodes;

  GskTransform *         transform;

  BobguiSnapshotCollectFunc collect_func;
  BobguiSnapshotClearFunc   clear_func;
  union {
    struct {
      double             opacity;
    } opacity;
    struct {
      double             radius;
    } blur;
    struct {
      graphene_matrix_t matrix;
      graphene_vec4_t offset;
    } color_matrix;
    struct {
      GskComponentTransfer *red;
      GskComponentTransfer *green;
      GskComponentTransfer *blue;
      GskComponentTransfer *alpha;
    } component_transfer;
    struct {
      graphene_rect_t bounds;
      graphene_rect_t child_bounds;
      GskRepeat repeat;
    } repeat;
    struct {
      graphene_rect_t bounds;
    } clip;
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
    struct {
      GskGLShader *shader;
      GBytes *args;
      graphene_rect_t bounds;
      GskRenderNode **nodes;
      GskRenderNode *internal_nodes[4];
    } glshader;
G_GNUC_END_IGNORE_DEPRECATIONS
    struct {
      graphene_rect_t bounds;
      int node_idx;
      int n_children;
    } glshader_texture;
    struct {
      GskRoundedRect bounds;
    } rounded_clip;
    struct {
      GskPath *path;
      GskFillRule fill_rule;
    } fill;
    struct {
      GskPath *path;
      GskStroke stroke;
    } stroke;
    struct {
      gsize n_shadows;
      GskShadowEntry *shadows;
      GskShadowEntry a_shadow; /* Used if n_shadows == 1 */
    } shadow;
    struct {
      GskBlendMode blend_mode;
      GskRenderNode *bottom_node;
    } blend;
    struct {
      double progress;
      GskRenderNode *start_node;
    } cross_fade;
    struct {
      char *message;
    } debug;
    struct {
      GskMaskMode mask_mode;
      GskRenderNode *mask_node;
    } mask;
    struct {
      GdkSubsurface *subsurface;
    } subsurface;
    struct {
      GskPorterDuff op;
      GskRenderNode *mask;
    } composite;
    struct {
      GskIsolation features;
    } isolation;
  } data;
};

static void bobgui_snapshot_state_clear (BobguiSnapshotState *state);

#define GDK_ARRAY_NAME bobgui_snapshot_states
#define GDK_ARRAY_TYPE_NAME BobguiSnapshotStates
#define GDK_ARRAY_ELEMENT_TYPE BobguiSnapshotState
#define GDK_ARRAY_FREE_FUNC bobgui_snapshot_state_clear
#define GDK_ARRAY_BY_VALUE 1
#define GDK_ARRAY_PREALLOC 16
#define GDK_ARRAY_NO_MEMSET 1
#include "gdk/gdkarrayimpl.c"

/* This is a nasty little hack. We typedef BobguiSnapshot to the fake object GdkSnapshot
 * so that we don't need to typecast between them.
 * After all, the GdkSnapshot only exist so poor language bindings don't trip. Hardcore
 * C code can just blatantly ignore such layering violations with a typedef.
 */
struct _GdkSnapshot {
  GObject                parent_instance; /* it's really GdkSnapshot, but don't tell anyone! */

  BobguiSnapshotStates      state_stack;
  BobguiSnapshotNodes       nodes;
};

struct _BobguiSnapshotClass {
  GObjectClass           parent_class; /* it's really GdkSnapshotClass, but don't tell anyone! */
};

G_DEFINE_TYPE (BobguiSnapshot, bobgui_snapshot, GDK_TYPE_SNAPSHOT)

static void
bobgui_snapshot_dispose (GObject *object)
{
  BobguiSnapshot *snapshot = BOBGUI_SNAPSHOT (object);

  if (!bobgui_snapshot_states_is_empty (&snapshot->state_stack))
    {
      GskRenderNode *node = bobgui_snapshot_to_node (snapshot);
      g_clear_pointer (&node, gsk_render_node_unref);
    }

  g_assert (bobgui_snapshot_states_is_empty (&snapshot->state_stack));
  g_assert (bobgui_snapshot_nodes_is_empty (&snapshot->nodes));

  G_OBJECT_CLASS (bobgui_snapshot_parent_class)->dispose (object);
}

static void
bobgui_snapshot_class_init (BobguiSnapshotClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->dispose = bobgui_snapshot_dispose;
}

static GskRenderNode *
bobgui_snapshot_collect_default (BobguiSnapshot       *snapshot,
                              BobguiSnapshotState  *state,
                              GskRenderNode    **nodes,
                              guint              n_nodes)
{
  GskRenderNode *node;

  if (n_nodes == 0)
    {
      node = NULL;
    }
  else if (n_nodes == 1)
    {
      node = gsk_render_node_ref (nodes[0]);
    }
  else
    {
      node = gsk_container_node_new (nodes, n_nodes);
    }

  return node;
}

static BobguiSnapshotState *
bobgui_snapshot_push_state (BobguiSnapshot            *snapshot,
                         GskTransform           *transform,
                         BobguiSnapshotCollectFunc  collect_func,
                         BobguiSnapshotClearFunc    clear_func)
{
  const gsize n_states = bobgui_snapshot_states_get_size (&snapshot->state_stack);
  BobguiSnapshotState *state;

  bobgui_snapshot_states_set_size (&snapshot->state_stack, n_states + 1);
  state = bobgui_snapshot_states_get (&snapshot->state_stack, n_states);

  state->transform = gsk_transform_ref (transform);
  state->collect_func = collect_func;
  state->clear_func = clear_func;
  state->start_node_index = bobgui_snapshot_nodes_get_size (&snapshot->nodes);
  state->n_nodes = 0;

  return state;
}

static BobguiSnapshotState *
bobgui_snapshot_get_current_state (const BobguiSnapshot *snapshot)
{
  gsize size = bobgui_snapshot_states_get_size (&snapshot->state_stack);

  g_assert (size > 0);

  return bobgui_snapshot_states_get (&snapshot->state_stack, size - 1);
}

static BobguiSnapshotState *
bobgui_snapshot_get_previous_state (const BobguiSnapshot *snapshot)
{
  gsize size = bobgui_snapshot_states_get_size (&snapshot->state_stack);

  g_assert (size > 1);

  return bobgui_snapshot_states_get (&snapshot->state_stack, size - 2);
}

/* n == 0 => current, n == 1, previous, etc */
static BobguiSnapshotState *
bobgui_snapshot_get_nth_previous_state (const BobguiSnapshot *snapshot,
                                     int n)
{
  gsize size = bobgui_snapshot_states_get_size (&snapshot->state_stack);

  g_assert (size > n);

  return bobgui_snapshot_states_get (&snapshot->state_stack, size - (1 + n));
}

static void
bobgui_snapshot_state_clear (BobguiSnapshotState *state)
{
  if (state->clear_func)
    state->clear_func (state);

  gsk_transform_unref (state->transform);
}

static void
bobgui_snapshot_init (BobguiSnapshot *self)
{
  bobgui_snapshot_states_init (&self->state_stack);
  bobgui_snapshot_nodes_init (&self->nodes);

  bobgui_snapshot_push_state (self,
                           NULL,
                           bobgui_snapshot_collect_default,
                           NULL);
}

/**
 * bobgui_snapshot_new:
 *
 * Creates a new `BobguiSnapshot`.
 *
 * Returns: a newly-allocated `BobguiSnapshot`
 */
BobguiSnapshot *
bobgui_snapshot_new (void)
{
  return g_object_new (BOBGUI_TYPE_SNAPSHOT, NULL);
}

/**
 * bobgui_snapshot_free_to_node: (skip)
 * @snapshot: (transfer full): a `BobguiSnapshot`
 *
 * Returns the node that was constructed by @snapshot
 * and frees @snapshot.
 *
 * See also [method@Bobgui.Snapshot.to_node].
 *
 * Returns: (transfer full) (nullable): a newly-created [class@Gsk.RenderNode]
 */
GskRenderNode *
bobgui_snapshot_free_to_node (BobguiSnapshot *snapshot)
{
  GskRenderNode *result;

  result = bobgui_snapshot_to_node (snapshot);
  g_object_unref (snapshot);

  return result;
}

/**
 * bobgui_snapshot_free_to_paintable: (skip)
 * @snapshot: (transfer full): a `BobguiSnapshot`
 * @size: (nullable): The size of the resulting paintable
 *   or %NULL to use the bounds of the snapshot
 *
 * Returns a paintable for the node that was
 * constructed by @snapshot and frees @snapshot.
 *
 * Returns: (transfer full) (nullable): a newly-created [iface@Gdk.Paintable]
 */
GdkPaintable *
bobgui_snapshot_free_to_paintable (BobguiSnapshot           *snapshot,
                                const graphene_size_t *size)
{
  GdkPaintable *result;

  result = bobgui_snapshot_to_paintable (snapshot, size);
  g_object_unref (snapshot);

  return result;
}

static GskRenderNode *
bobgui_snapshot_collect_autopush_transform (BobguiSnapshot      *snapshot,
                                         BobguiSnapshotState *state,
                                         GskRenderNode   **nodes,
                                         guint             n_nodes)
{
  GskRenderNode *node, *transform_node;
  BobguiSnapshotState *previous_state;

  previous_state = bobgui_snapshot_get_previous_state (snapshot);

  node = bobgui_snapshot_collect_default (snapshot, state, nodes, n_nodes);
  if (node == NULL)
    return NULL;

  transform_node = gsk_transform_node_new (node, previous_state->transform);

  gsk_render_node_unref (node);

  return transform_node;
}

static void
bobgui_snapshot_autopush_transform (BobguiSnapshot *snapshot)
{
  bobgui_snapshot_push_state (snapshot,
                           NULL,
                           bobgui_snapshot_collect_autopush_transform,
                           NULL);
}

static gboolean
bobgui_snapshot_state_should_autopop (const BobguiSnapshotState *state)
{
  return state->collect_func == bobgui_snapshot_collect_autopush_transform;
}

static GskRenderNode *
bobgui_snapshot_collect_debug (BobguiSnapshot      *snapshot,
                            BobguiSnapshotState *state,
                            GskRenderNode   **nodes,
                            guint             n_nodes)
{
  GskRenderNode *node, *debug_node;

  node = bobgui_snapshot_collect_default (snapshot, state, nodes, n_nodes);
  if (node == NULL)
    return NULL;

  debug_node = gsk_debug_node_new (node, state->data.debug.message);
  state->data.debug.message = NULL;

  gsk_render_node_unref (node);

  return debug_node;
}

static void
bobgui_snapshot_clear_debug (BobguiSnapshotState *state)
{
  g_clear_pointer (&state->data.debug.message, g_free);
}

/**
 * bobgui_snapshot_push_debug:
 * @snapshot: a `BobguiSnapshot`
 * @message: a printf-style format string
 * @...: arguments for @message
 *
 * Inserts a debug node with a message.
 *
 * Debug nodes don't affect the rendering at all, but can be
 * helpful in identifying parts of a render node tree dump,
 * for example in the BOBGUI inspector.
 */
void
bobgui_snapshot_push_debug (BobguiSnapshot *snapshot,
                         const char  *message,
                         ...)
{
  BobguiSnapshotState *current_state = bobgui_snapshot_get_current_state (snapshot);

  if (BOBGUI_DEBUG_CHECK (SNAPSHOT))
    {
      va_list args;
      BobguiSnapshotState *state;

      state = bobgui_snapshot_push_state (snapshot,
                                       current_state->transform,
                                       bobgui_snapshot_collect_debug,
                                       bobgui_snapshot_clear_debug);



      va_start (args, message);
      state->data.debug.message = g_strdup_vprintf (message, args);
      va_end (args);
    }
  else
    {
      bobgui_snapshot_push_state (snapshot,
                               current_state->transform,
                               bobgui_snapshot_collect_default,
                               NULL);
    }
}

static GskRenderNode *
bobgui_snapshot_collect_opacity (BobguiSnapshot      *snapshot,
                              BobguiSnapshotState *state,
                              GskRenderNode   **nodes,
                              guint             n_nodes)
{
  GskRenderNode *node, *opacity_node;

  node = bobgui_snapshot_collect_default (snapshot, state, nodes, n_nodes);
  if (node == NULL)
    return NULL;

  if (state->data.opacity.opacity == 0.0)
    {
      GdkRGBA color = GDK_RGBA ("00000000");
      graphene_rect_t bounds;

      gsk_render_node_get_bounds (node, &bounds);
      opacity_node = gsk_color_node_new (&color, &bounds);
      gsk_render_node_unref (node);
    }
  else
    {
      opacity_node = gsk_opacity_node_new (node, state->data.opacity.opacity);
      gsk_render_node_unref (node);
    }

  return opacity_node;
}

/**
 * bobgui_snapshot_push_opacity:
 * @snapshot: a `BobguiSnapshot`
 * @opacity: the opacity to use
 *
 * Modifies the opacity of an image.
 *
 * The image is recorded until the next call to [method@Bobgui.Snapshot.pop].
 */
void
bobgui_snapshot_push_opacity (BobguiSnapshot *snapshot,
                           double       opacity)
{
  BobguiSnapshotState *current_state = bobgui_snapshot_get_current_state (snapshot);
  BobguiSnapshotState *state;

  state = bobgui_snapshot_push_state (snapshot,
                                   current_state->transform,
                                   bobgui_snapshot_collect_opacity,
                                   NULL);
  state->data.opacity.opacity = CLAMP (opacity, 0.0, 1.0);
}

static GskRenderNode *
bobgui_snapshot_collect_isolation (BobguiSnapshot      *snapshot,
                                BobguiSnapshotState *state,
                                GskRenderNode   **nodes,
                                guint             n_nodes)
{
  GskRenderNode *node, *isolation_node;
  GskIsolation features;

  node = bobgui_snapshot_collect_default (snapshot, state, nodes, n_nodes);
  if (node == NULL)
    return NULL;

  features = gsk_isolation_features_simplify_for_node (state->data.isolation.features, node);
  if (features == 0)
    return node;

  isolation_node = gsk_isolation_node_new (node, features);
  gsk_render_node_unref (node);

  return isolation_node;
}

/**
 * bobgui_snapshot_push_isolation:
 * @snapshot: a `BobguiSnapshot`
 * @features: features that are isolated
 *
 * Isolates the following drawing operations from previous ones.
 *
 * You can express "everything but these flags" in a forward compatible
 * way by using bit math:
 * `GSK_ISOLATION_ALL & ~(GSK_ISOLATION_BACKGROUND | GSK_ISOLATION_COPY_PASTE)`
 * will isolate everything but background and copy/paste.
 *
 * For what isolation features exist, see [flags@Gsk.Isolation].
 *
 * Content is isolated until the next call to [method@Bobgui.Snapshot.pop].
 *
 * Since: 4.22
 */
void
bobgui_snapshot_push_isolation (BobguiSnapshot  *snapshot,
                             GskIsolation  features)
{
  BobguiSnapshotState *current_state = bobgui_snapshot_get_current_state (snapshot);
  BobguiSnapshotState *state;

  state = bobgui_snapshot_push_state (snapshot,
                                   current_state->transform,
                                   bobgui_snapshot_collect_isolation,
                                   NULL);
  state->data.isolation.features = features;
}

static GskRenderNode *
bobgui_snapshot_collect_blur (BobguiSnapshot      *snapshot,
                           BobguiSnapshotState *state,
                           GskRenderNode   **nodes,
                           guint             n_nodes)
{
  GskRenderNode *node, *blur_node;
  double radius;

  node = bobgui_snapshot_collect_default (snapshot, state, nodes, n_nodes);
  if (node == NULL)
    return NULL;

  radius = state->data.blur.radius;

  if (radius == 0.0)
    return node;

  if (radius < 0)
    return node;

  blur_node = gsk_blur_node_new (node, radius);

  gsk_render_node_unref (node);

  return blur_node;
}

/**
 * bobgui_snapshot_push_blur:
 * @snapshot: a `BobguiSnapshot`
 * @radius: the blur radius to use. Must be positive
 *
 * Blurs an image.
 *
 * The image is recorded until the next call to [method@Bobgui.Snapshot.pop].
 */
void
bobgui_snapshot_push_blur (BobguiSnapshot *snapshot,
                        double       radius)
{
  const BobguiSnapshotState *current_state = bobgui_snapshot_get_current_state (snapshot);
  BobguiSnapshotState *state;

  state = bobgui_snapshot_push_state (snapshot,
                                   current_state->transform,
                                   bobgui_snapshot_collect_blur,
                                   NULL);
  state->data.blur.radius = radius;
}

static GskRenderNode *
merge_color_matrix_nodes (const graphene_matrix_t *matrix2,
                          const graphene_vec4_t   *offset2,
                          GskRenderNode           *child)
{
  const graphene_matrix_t *matrix1 = gsk_color_matrix_node_get_color_matrix (child);
  const graphene_vec4_t *offset1 = gsk_color_matrix_node_get_color_offset (child);
  graphene_matrix_t matrix;
  graphene_vec4_t offset;
  GskRenderNode *result;

  g_assert (gsk_render_node_get_node_type (child) == GSK_COLOR_MATRIX_NODE);

  /* color matrix node: color = trans(mat) * p + offset; for a pixel p.
   * color =  trans(mat2) * (trans(mat1) * p + offset1) + offset2
   *       =  trans(mat2) * trans(mat1) * p + trans(mat2) * offset1 + offset2
   *       = trans(mat1 * mat2) * p + (trans(mat2) * offset1 + offset2)
   * Which this code does.
   * mat1 and offset1 come from @child.
   */

  graphene_matrix_transform_vec4 (matrix2, offset1, &offset);
  graphene_vec4_add (&offset, offset2, &offset);

  graphene_matrix_multiply (matrix1, matrix2, &matrix);

  result = gsk_color_matrix_node_new (gsk_color_matrix_node_get_child (child),
                                      &matrix, &offset);

  return result;
}

static GskRenderNode *
bobgui_snapshot_collect_color_matrix (BobguiSnapshot      *snapshot,
                                   BobguiSnapshotState *state,
                                   GskRenderNode   **nodes,
                                   guint             n_nodes)
{
  GskRenderNode *node, *result;

  node = bobgui_snapshot_collect_default (snapshot, state, nodes, n_nodes);
  if (node == NULL)
    return NULL;

  if (gsk_render_node_get_node_type (node) == GSK_COLOR_MATRIX_NODE)
    {
      result = merge_color_matrix_nodes (&state->data.color_matrix.matrix,
                                         &state->data.color_matrix.offset,
                                         node);
      gsk_render_node_unref (node);
    }
  else if (gsk_render_node_get_node_type (node) == GSK_TRANSFORM_NODE)
    {
      GskRenderNode *transform_child = gsk_transform_node_get_child (node);
      GskRenderNode *color_matrix;

      if (gsk_render_node_get_node_type (transform_child) == GSK_COLOR_MATRIX_NODE)
        {
          color_matrix = merge_color_matrix_nodes (&state->data.color_matrix.matrix,
                                                   &state->data.color_matrix.offset,
                                                   transform_child);
        }
      else
        {
          color_matrix = gsk_color_matrix_node_new (transform_child,
                                                    &state->data.color_matrix.matrix,
                                                    &state->data.color_matrix.offset);
        }
      result = gsk_transform_node_new (color_matrix,
                                       gsk_transform_node_get_transform (node));
      gsk_render_node_unref (color_matrix);
      gsk_render_node_unref (node);
    }
  else
    {
      result = gsk_color_matrix_node_new (node,
                                          &state->data.color_matrix.matrix,
                                          &state->data.color_matrix.offset);
      gsk_render_node_unref (node);
    }

  return result;
}

/**
 * bobgui_snapshot_push_color_matrix:
 * @snapshot: a `BobguiSnapshot`
 * @color_matrix: the color matrix to use
 * @color_offset: the color offset to use
 *
 * Modifies the colors of an image by applying an affine transformation
 * in RGB space.
 *
 * In particular, the colors will be transformed by applying
 *
 *     pixel = transpose(color_matrix) * pixel + color_offset
 *
 * for every pixel. The transformation operates on unpremultiplied
 * colors, with color components ordered R, G, B, A.
 *
 * The image is recorded until the next call to [method@Bobgui.Snapshot.pop].
 */
void
bobgui_snapshot_push_color_matrix (BobguiSnapshot             *snapshot,
                                const graphene_matrix_t *color_matrix,
                                const graphene_vec4_t   *color_offset)
{
  const BobguiSnapshotState *current_state = bobgui_snapshot_get_current_state (snapshot);
  BobguiSnapshotState *state;

  state = bobgui_snapshot_push_state (snapshot,
                                   current_state->transform,
                                   bobgui_snapshot_collect_color_matrix,
                                   NULL);

  graphene_matrix_init_from_matrix (&state->data.color_matrix.matrix, color_matrix);
  graphene_vec4_init_from_vec4 (&state->data.color_matrix.offset, color_offset);
}

static GskRenderNode *
bobgui_snapshot_collect_component_transfer (BobguiSnapshot      *snapshot,
                                         BobguiSnapshotState *state,
                                         GskRenderNode   **nodes,
                                         guint             n_nodes)
{
  GskRenderNode *node, *result;

  node = bobgui_snapshot_collect_default (snapshot, state, nodes, n_nodes);
  if (node == NULL)
    return NULL;

  result = gsk_component_transfer_node_new (node,
                                            state->data.component_transfer.red,
                                            state->data.component_transfer.green,
                                            state->data.component_transfer.blue,
                                            state->data.component_transfer.alpha);
  gsk_render_node_unref (node);

  return result;
}

static void
bobgui_snapshot_clear_component_transfer (BobguiSnapshotState *state)
{
  gsk_component_transfer_free (state->data.component_transfer.red);
  gsk_component_transfer_free (state->data.component_transfer.green);
  gsk_component_transfer_free (state->data.component_transfer.blue);
  gsk_component_transfer_free (state->data.component_transfer.alpha);
}

/**
 * bobgui_snapshot_push_component_transfer:
 * @snapshot: a `BobguiSnapshot`
 * @red: the transfer for the red component
 * @green: the transfer for the green component
 * @blue: the transfer for the blue component
 * @alpha: the transfer for the alpha component
 *
 * Modifies the colors of an image by applying a transfer
 * function for each component.
 *
 * The transfer functions operate on unpremultiplied colors.
 *
 * The image is recorded until the next call to [method@Bobgui.Snapshot.pop].
 *
 * Since: 4.20
 */
void
bobgui_snapshot_push_component_transfer (BobguiSnapshot                *snapshot,
                                      const GskComponentTransfer *red,
                                      const GskComponentTransfer *green,
                                      const GskComponentTransfer *blue,
                                      const GskComponentTransfer *alpha)
{
  const BobguiSnapshotState *current_state = bobgui_snapshot_get_current_state (snapshot);
  BobguiSnapshotState *state;

  state = bobgui_snapshot_push_state (snapshot,
                                   current_state->transform,
                                   bobgui_snapshot_collect_component_transfer,
                                   bobgui_snapshot_clear_component_transfer);

  state->data.component_transfer.red = gsk_component_transfer_copy (red);
  state->data.component_transfer.green = gsk_component_transfer_copy (green);
  state->data.component_transfer.blue = gsk_component_transfer_copy (blue);
  state->data.component_transfer.alpha = gsk_component_transfer_copy (alpha);
}

static GskRenderNode *
bobgui_snapshot_collect_repeat (BobguiSnapshot      *snapshot,
                             BobguiSnapshotState *state,
                             GskRenderNode   **nodes,
                             guint             n_nodes)
{
  GskRenderNode *node, *repeat_node;
  const graphene_rect_t *bounds = &state->data.repeat.bounds;
  const graphene_rect_t *child_bounds = &state->data.repeat.child_bounds;
  GskRepeat repeat = state->data.repeat.repeat;

  node = bobgui_snapshot_collect_default (snapshot, state, nodes, n_nodes);
  if (node == NULL)
    return NULL;

  if (gsk_render_node_get_node_type (node) == GSK_COLOR_NODE &&
      gsk_rect_equal (child_bounds, &node->bounds))
    {
      /* Repeating a color node entirely is pretty easy by just increasing
       * the size of the color node.
       */
      GskRenderNode *color_node = gsk_color_node_new2 (gsk_color_node_get_gdk_color (node), bounds);

      gsk_render_node_unref (node);

      return color_node;
    }

  repeat_node = gsk_repeat_node_new2 (bounds,
                                      node,
                                      child_bounds->size.width > 0 ? child_bounds : NULL,
                                      repeat);

  gsk_render_node_unref (node);

  return repeat_node;
}

static GskRenderNode *
bobgui_snapshot_collect_discard_repeat (BobguiSnapshot      *snapshot,
                                     BobguiSnapshotState *state,
                                     GskRenderNode   **nodes,
                                     guint             n_nodes)
{
  /* Drop the node and return nothing.  */
  return NULL;
}

static void
bobgui_graphene_rect_scale_affine (const graphene_rect_t *rect,
                                float                  scale_x,
                                float                  scale_y,
                                float                  dx,
                                float                  dy,
                                graphene_rect_t       *res)
{
  res->origin.x = scale_x * rect->origin.x + dx;
  res->origin.y = scale_y * rect->origin.y + dy;
  res->size.width = scale_x * rect->size.width;
  res->size.height = scale_y * rect->size.height;

  if (scale_x < 0 || scale_y < 0)
    graphene_rect_normalize (res);
}

typedef enum {
  ENSURE_POSITIVE_SCALE = (1 << 0),
  ENSURE_UNIFORM_SCALE = (1 << 1),
} BobguiEnsureFlags;

static void
bobgui_snapshot_ensure_affine_with_flags (BobguiSnapshot    *snapshot,
                                       BobguiEnsureFlags  flags,
                                       float          *scale_x,
                                       float          *scale_y,
                                       float          *dx,
                                       float          *dy)
{
  const BobguiSnapshotState *state = bobgui_snapshot_get_current_state (snapshot);

  if (gsk_transform_get_category (state->transform) < GSK_TRANSFORM_CATEGORY_2D_AFFINE)
    {
      bobgui_snapshot_autopush_transform (snapshot);
      state = bobgui_snapshot_get_current_state (snapshot);
      gsk_transform_to_affine (state->transform, scale_x, scale_y, dx, dy);
    }
  else if (gsk_transform_get_category (state->transform) == GSK_TRANSFORM_CATEGORY_2D_AFFINE)
    {
      gsk_transform_to_affine (state->transform, scale_x, scale_y, dx, dy);
      if (((flags & ENSURE_POSITIVE_SCALE) && (*scale_x < 0.0 || *scale_y < 0.0)) ||
          ((flags & ENSURE_UNIFORM_SCALE) && (*scale_x != *scale_y)))
        {
          bobgui_snapshot_autopush_transform (snapshot);
          state = bobgui_snapshot_get_current_state (snapshot);
          gsk_transform_to_affine (state->transform, scale_x, scale_y, dx, dy);
        }
    }
  else
    {
      gsk_transform_to_affine (state->transform, scale_x, scale_y, dx, dy);
    }
}

static void
bobgui_snapshot_ensure_affine (BobguiSnapshot *snapshot,
                            float       *scale_x,
                            float       *scale_y,
                            float       *dx,
                            float       *dy)
{
  bobgui_snapshot_ensure_affine_with_flags (snapshot,
                                         ENSURE_POSITIVE_SCALE,
                                         scale_x, scale_y,
                                         dx, dy);
}

static void
bobgui_snapshot_ensure_translate (BobguiSnapshot *snapshot,
                               float       *dx,
                               float       *dy)
{
  const BobguiSnapshotState *state = bobgui_snapshot_get_current_state (snapshot);

  if (gsk_transform_get_category (state->transform) < GSK_TRANSFORM_CATEGORY_2D_TRANSLATE)
    {
      bobgui_snapshot_autopush_transform (snapshot);
      state = bobgui_snapshot_get_current_state (snapshot);
    }

  gsk_transform_to_translate (state->transform, dx, dy);
}

static void
bobgui_snapshot_ensure_identity (BobguiSnapshot *snapshot)
{
  const BobguiSnapshotState *state = bobgui_snapshot_get_current_state (snapshot);

  if (gsk_transform_get_category (state->transform) < GSK_TRANSFORM_CATEGORY_IDENTITY)
    bobgui_snapshot_autopush_transform (snapshot);
}

void
bobgui_snapshot_push_repeat2 (BobguiSnapshot           *snapshot,
                           const graphene_rect_t *bounds,
                           const graphene_rect_t *child_bounds,
                           GskRepeat              repeat)
{
  BobguiSnapshotState *state;
  gboolean empty_child_bounds = FALSE;
  graphene_rect_t real_child_bounds = { { 0 } };
  float scale_x, scale_y, dx, dy;

  bobgui_snapshot_ensure_affine (snapshot, &scale_x, &scale_y, &dx, &dy);

  if (child_bounds)
    {
      bobgui_graphene_rect_scale_affine (child_bounds, scale_x, scale_y, dx, dy, &real_child_bounds);
      if (real_child_bounds.size.width <= 0 || real_child_bounds.size.height <= 0)
        empty_child_bounds = TRUE;
    }

  state = bobgui_snapshot_push_state (snapshot,
                                   bobgui_snapshot_get_current_state (snapshot)->transform,
                                   empty_child_bounds
                                   ? bobgui_snapshot_collect_discard_repeat
                                   : bobgui_snapshot_collect_repeat,
                                   NULL);

  bobgui_graphene_rect_scale_affine (bounds, scale_x, scale_y, dx, dy, &state->data.repeat.bounds);
  state->data.repeat.child_bounds = real_child_bounds;
  state->data.repeat.repeat = repeat;
}

/**
 * bobgui_snapshot_push_repeat:
 * @snapshot: a `BobguiSnapshot`
 * @bounds: the bounds within which to repeat
 * @child_bounds: (nullable): the bounds of the child or %NULL
 *   to use the full size of the collected child node
 *
 * Creates a node that repeats the child node.
 *
 * The child is recorded until the next call to [method@Bobgui.Snapshot.pop].
 */
void
bobgui_snapshot_push_repeat (BobguiSnapshot           *snapshot,
                          const graphene_rect_t *bounds,
                          const graphene_rect_t *child_bounds)
{
  bobgui_snapshot_push_repeat2 (snapshot, bounds, child_bounds, GSK_REPEAT_REPEAT);
}

static GskRenderNode *
bobgui_snapshot_collect_clip (BobguiSnapshot      *snapshot,
                           BobguiSnapshotState *state,
                           GskRenderNode   **nodes,
                           guint             n_nodes)
{
  GskRenderNode *node, *clip_node;

  node = bobgui_snapshot_collect_default (snapshot, state, nodes, n_nodes);
  if (node == NULL)
    return NULL;

  /* Check if the child node will even be clipped */
  if (graphene_rect_contains_rect (&state->data.clip.bounds, &node->bounds))
    return node;

  if (state->data.clip.bounds.size.width == 0 ||
      state->data.clip.bounds.size.height == 0)
    {
      gsk_render_node_unref (node);

      return NULL;
    }

  clip_node = gsk_clip_node_new (node, &state->data.clip.bounds);

  gsk_render_node_unref (node);

  return clip_node;
}

/**
 * bobgui_snapshot_push_clip:
 * @snapshot: a `BobguiSnapshot`
 * @bounds: the rectangle to clip to
 *
 * Clips an image to a rectangle.
 *
 * The image is recorded until the next call to [method@Bobgui.Snapshot.pop].
 */
void
bobgui_snapshot_push_clip (BobguiSnapshot           *snapshot,
                        const graphene_rect_t *bounds)
{
  BobguiSnapshotState *state;
  float scale_x, scale_y, dx, dy;

  bobgui_snapshot_ensure_affine (snapshot, &scale_x, &scale_y, &dx, &dy);

  state = bobgui_snapshot_push_state (snapshot,
                                   bobgui_snapshot_get_current_state (snapshot)->transform,
                                   bobgui_snapshot_collect_clip,
                                   NULL);

  bobgui_graphene_rect_scale_affine (bounds, scale_x, scale_y, dx, dy, &state->data.clip.bounds);
}

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

static GskRenderNode *
bobgui_snapshot_collect_gl_shader (BobguiSnapshot      *snapshot,
                                BobguiSnapshotState *state,
                                GskRenderNode   **collected_nodes,
                                guint             n_collected_nodes)
{
  GskRenderNode *shader_node = NULL;
  GskRenderNode **nodes;
  int n_children;

  n_children = gsk_gl_shader_get_n_textures (state->data.glshader.shader);
  shader_node = NULL;

  if (n_collected_nodes != 0)
    g_warning ("Unexpected children when popping gl shader.");

  if (state->data.glshader.nodes)
    nodes = state->data.glshader.nodes;
  else
    nodes = &state->data.glshader.internal_nodes[0];

  if (state->data.glshader.bounds.size.width != 0 &&
      state->data.glshader.bounds.size.height != 0)
    shader_node = gsk_gl_shader_node_new (state->data.glshader.shader,
                                          &state->data.glshader.bounds,
                                          state->data.glshader.args,
                                          nodes, n_children);

  return shader_node;
}

static void
bobgui_snapshot_clear_gl_shader (BobguiSnapshotState *state)
{
  GskRenderNode **nodes;
  guint i, n_children;

  n_children = gsk_gl_shader_get_n_textures (state->data.glshader.shader);

  if (state->data.glshader.nodes)
    nodes = state->data.glshader.nodes;
  else
    nodes = &state->data.glshader.internal_nodes[0];

  g_object_unref (state->data.glshader.shader);
  g_bytes_unref (state->data.glshader.args);

  for (i = 0; i < n_children; i++)
    gsk_render_node_unref (nodes[i]);

  g_free (state->data.glshader.nodes);

}

static GskRenderNode *
bobgui_snapshot_collect_gl_shader_texture (BobguiSnapshot      *snapshot,
                                        BobguiSnapshotState *state,
                                        GskRenderNode   **nodes,
                                        guint             n_nodes)
{
  GskRenderNode *child_node;
  GdkRGBA transparent = { 0, 0, 0, 0 };
  int n_children, node_idx;
  BobguiSnapshotState *glshader_state;
  GskRenderNode **out_nodes;

  child_node = bobgui_snapshot_collect_default (snapshot, state, nodes, n_nodes);

  if (child_node == NULL)
    child_node = gsk_color_node_new (&transparent, &state->data.glshader_texture.bounds);

  n_children = state->data.glshader_texture.n_children;
  node_idx = state->data.glshader_texture.node_idx;

  glshader_state = bobgui_snapshot_get_nth_previous_state (snapshot, n_children - node_idx);
  g_assert (glshader_state->collect_func == bobgui_snapshot_collect_gl_shader);

  if (glshader_state->data.glshader.nodes)
    out_nodes = glshader_state->data.glshader.nodes;
  else
    out_nodes = &glshader_state->data.glshader.internal_nodes[0];

  out_nodes[node_idx] = child_node;

  return NULL;
}

/**
 * bobgui_snapshot_push_gl_shader:
 * @snapshot: a `BobguiSnapshot`
 * @shader: The code to run
 * @bounds: the rectangle to render into
 * @take_args: (transfer full): Data block with arguments for the shader.
 *
 * Push a [class@Gsk.GLShaderNode].
 *
 * The node uses the given [class@Gsk.GLShader] and uniform values
 * Additionally this takes a list of @n_children other nodes
 * which will be passed to the [class@Gsk.GLShaderNode].
 *
 * The @take_args argument is a block of data to use for uniform
 * arguments, as per types and offsets defined by the @shader.
 * Normally this is generated by [method@Gsk.GLShader.format_args]
 * or [struct@Gsk.ShaderArgsBuilder].
 *
 * The snapshotter takes ownership of @take_args, so the caller should
 * not free it after this.
 *
 * If the renderer doesn't support GL shaders, or if there is any
 * problem when compiling the shader, then the node will draw pink.
 * You should use [method@Gsk.GLShader.compile] to ensure the @shader
 * will work for the renderer before using it.
 *
 * If the shader requires textures (see [method@Gsk.GLShader.get_n_textures]),
 * then it is expected that you call [method@Bobgui.Snapshot.gl_shader_pop_texture]
 * the number of times that are required. Each of these calls will generate
 * a node that is added as a child to the `GskGLShaderNode`, which in turn
 * will render these offscreen and pass as a texture to the shader.
 *
 * Once all textures (if any) are pop:ed, you must call the regular
 * [method@Bobgui.Snapshot.pop].
 *
 * If you want to use pre-existing textures as input to the shader rather
 * than rendering new ones, use [method@Bobgui.Snapshot.append_texture] to
 * push a texture node. These will be used directly rather than being
 * re-rendered.
 *
 * For details on how to write shaders, see [class@Gsk.GLShader].
 *
 * Deprecated: 4.16: BOBGUI's new Vulkan-focused rendering
 *   does not support this feature. Use [class@Bobgui.GLArea] for
 *   OpenGL rendering.
 */
void
bobgui_snapshot_push_gl_shader (BobguiSnapshot           *snapshot,
                             GskGLShader           *shader,
                             const graphene_rect_t *bounds,
                             GBytes                *take_args)
{
  BobguiSnapshotState *state;
  float scale_x, scale_y, dx, dy;
  graphene_rect_t transformed_bounds;
  int n_children = gsk_gl_shader_get_n_textures (shader);

  bobgui_snapshot_ensure_affine (snapshot, &scale_x, &scale_y, &dx, &dy);

  state = bobgui_snapshot_push_state (snapshot,
                                   bobgui_snapshot_get_current_state (snapshot)->transform,
                                   bobgui_snapshot_collect_gl_shader,
                                   bobgui_snapshot_clear_gl_shader);
  bobgui_graphene_rect_scale_affine (bounds, scale_x, scale_y, dx, dy, &transformed_bounds);
  state->data.glshader.bounds = transformed_bounds;
  state->data.glshader.shader = g_object_ref (shader);
  state->data.glshader.args = take_args; /* Takes ownership */
  if (n_children <= G_N_ELEMENTS (state->data.glshader.internal_nodes))
    state->data.glshader.nodes = NULL;
  else
    state->data.glshader.nodes = g_new (GskRenderNode *, n_children);

  for (int i = 0; i  < n_children; i++)
    {
      state = bobgui_snapshot_push_state (snapshot,
                                       bobgui_snapshot_get_current_state (snapshot)->transform,
                                       bobgui_snapshot_collect_gl_shader_texture,
                                       NULL);
      state->data.glshader_texture.bounds = transformed_bounds;
      state->data.glshader_texture.node_idx = n_children - 1 - i;/* We pop in reverse order */
      state->data.glshader_texture.n_children = n_children;
    }
}

static GskRenderNode *
bobgui_snapshot_collect_rounded_clip (BobguiSnapshot      *snapshot,
                                   BobguiSnapshotState *state,
                                   GskRenderNode   **nodes,
                                   guint             n_nodes)
{
  GskRenderNode *node, *clip_node;

  node = bobgui_snapshot_collect_default (snapshot, state, nodes, n_nodes);
  if (node == NULL)
    return NULL;

  /* If the given radius is 0 in all corners, we can just create a normal clip node */
  if (gsk_rounded_rect_is_rectilinear (&state->data.rounded_clip.bounds))
    {
      /* ... and do the same optimization */
      if (graphene_rect_contains_rect (&state->data.rounded_clip.bounds.bounds, &node->bounds))
        return node;

      clip_node = gsk_clip_node_new (node, &state->data.rounded_clip.bounds.bounds);
    }
  else
    {
      if (gsk_rounded_rect_contains_rect (&state->data.rounded_clip.bounds, &node->bounds))
        return node;

      clip_node = gsk_rounded_clip_node_new (node, &state->data.rounded_clip.bounds);
    }

  if (clip_node->bounds.size.width == 0 ||
      clip_node->bounds.size.height == 0)
    {
      gsk_render_node_unref (node);
      gsk_render_node_unref (clip_node);
      return NULL;
    }

  gsk_render_node_unref (node);

  return clip_node;
}

G_GNUC_END_IGNORE_DEPRECATIONS

/**
 * bobgui_snapshot_push_rounded_clip:
 * @snapshot: a `BobguiSnapshot`
 * @bounds: the rounded rectangle to clip to
 *
 * Clips an image to a rounded rectangle.
 *
 * The image is recorded until the next call to [method@Bobgui.Snapshot.pop].
 */
void
bobgui_snapshot_push_rounded_clip (BobguiSnapshot          *snapshot,
                                const GskRoundedRect *bounds)
{
  BobguiSnapshotState *state;
  float scale_x, scale_y, dx, dy;

  bobgui_snapshot_ensure_affine (snapshot, &scale_x, &scale_y, &dx, &dy);

  state = bobgui_snapshot_push_state (snapshot,
                                   bobgui_snapshot_get_current_state (snapshot)->transform,
                                   bobgui_snapshot_collect_rounded_clip,
                                   NULL);

  gsk_rounded_rect_scale_affine (&state->data.rounded_clip.bounds, bounds, scale_x, scale_y, dx, dy);
}

static GskRenderNode *
bobgui_snapshot_collect_fill (BobguiSnapshot      *snapshot,
                           BobguiSnapshotState *state,
                           GskRenderNode   **nodes,
                           guint             n_nodes)
{
  GskRenderNode *node, *fill_node;

  node = bobgui_snapshot_collect_default (snapshot, state, nodes, n_nodes);
  if (node == NULL)
    return NULL;

  fill_node = gsk_fill_node_new (node,
                                 state->data.fill.path,
                                 state->data.fill.fill_rule);

  if (fill_node->bounds.size.width == 0 ||
      fill_node->bounds.size.height == 0)
    {
      gsk_render_node_unref (node);
      gsk_render_node_unref (fill_node);
      return NULL;
    }

  gsk_render_node_unref (node);

  return fill_node;
}

static void
bobgui_snapshot_clear_fill (BobguiSnapshotState *state)
{
  gsk_path_unref (state->data.fill.path);
}

/**
 * bobgui_snapshot_push_fill:
 * @snapshot: a `BobguiSnapshot`
 * @path: The path describing the area to fill
 * @fill_rule: The fill rule to use
 *
 * Fills the area given by @path and @fill_rule with an image and discards everything
 * outside of it.
 *
 * The image is recorded until the next call to [method@Bobgui.Snapshot.pop].
 *
 * If you want to fill the path with a color, [method@Bobgui.Snapshot.append_fill]
 * than rendering new ones, use [method@Bobgui.Snapshot.append_fill]
 * may be more convenient.
 *
 * Since: 4.14
 */
void
bobgui_snapshot_push_fill (BobguiSnapshot *snapshot,
                        GskPath     *path,
                        GskFillRule  fill_rule)
{
  BobguiSnapshotState *state;

  bobgui_snapshot_ensure_identity (snapshot);

  state = bobgui_snapshot_push_state (snapshot,
                                   bobgui_snapshot_get_current_state (snapshot)->transform,
                                   bobgui_snapshot_collect_fill,
                                   bobgui_snapshot_clear_fill);

  state->data.fill.path = gsk_path_ref (path);
  state->data.fill.fill_rule = fill_rule;
}

/**
 * bobgui_snapshot_append_fill:
 * @snapshot: a `BobguiSnapshot`
 * @path: The path describing the area to fill
 * @fill_rule: The fill rule to use
 * @color: the color to fill the path with
 *
 * A convenience method to fill a path with a color.
 *
 * See [method@Bobgui.Snapshot.push_fill] if you need
 * to fill a path with more complex content than
 * a color.
 *
 * Since: 4.14
 */
void
bobgui_snapshot_append_fill (BobguiSnapshot   *snapshot,
                          GskPath       *path,
                          GskFillRule    fill_rule,
                          const GdkRGBA *color)
{
  graphene_rect_t bounds;

  if (!gsk_path_get_bounds (path, &bounds))
    return;

  bobgui_snapshot_push_fill (snapshot, path, fill_rule);
  bobgui_snapshot_append_color (snapshot, color, &bounds);
  bobgui_snapshot_pop (snapshot);
}

static GskRenderNode *
bobgui_snapshot_collect_stroke (BobguiSnapshot      *snapshot,
                             BobguiSnapshotState *state,
                             GskRenderNode   **nodes,
                             guint             n_nodes)
{
  GskRenderNode *node, *stroke_node;

  node = bobgui_snapshot_collect_default (snapshot, state, nodes, n_nodes);
  if (node == NULL)
    return NULL;

  stroke_node = gsk_stroke_node_new (node,
                                     state->data.stroke.path,
                                     &state->data.stroke.stroke);

  if (stroke_node->bounds.size.width == 0 ||
      stroke_node->bounds.size.height == 0)
    {
      gsk_render_node_unref (node);
      gsk_render_node_unref (stroke_node);
      return NULL;
    }

  gsk_render_node_unref (node);

  return stroke_node;
}

static void
bobgui_snapshot_clear_stroke (BobguiSnapshotState *state)
{
  gsk_path_unref (state->data.stroke.path);
  gsk_stroke_clear (&state->data.stroke.stroke);
}

/**
 * bobgui_snapshot_push_stroke:
 * @snapshot: a #BobguiSnapshot
 * @path: The path to stroke
 * @stroke: The stroke attributes
 *
 * Strokes the given @path with the attributes given by @stroke and
 * an image.
 *
 * The image is recorded until the next call to [method@Bobgui.Snapshot.pop].
 *
 * Note that the strokes are subject to the same transformation as
 * everything else, so uneven scaling will cause horizontal and vertical
 * strokes to have different widths.
 *
 * If you want to stroke the path with a color, [method@Bobgui.Snapshot.append_stroke]
 * may be more convenient.
 *
 * Since: 4.14
 */
void
bobgui_snapshot_push_stroke (BobguiSnapshot     *snapshot,
                          GskPath         *path,
                          const GskStroke *stroke)
{
  BobguiSnapshotState *state;

  bobgui_snapshot_ensure_identity (snapshot);

  state = bobgui_snapshot_push_state (snapshot,
                                   bobgui_snapshot_get_current_state (snapshot)->transform,
                                   bobgui_snapshot_collect_stroke,
                                   bobgui_snapshot_clear_stroke);

  state->data.stroke.path = gsk_path_ref (path);
  state->data.stroke.stroke = GSK_STROKE_INIT_COPY (stroke);
}

static GskRenderNode *
bobgui_snapshot_collect_shadow (BobguiSnapshot      *snapshot,
                             BobguiSnapshotState *state,
                             GskRenderNode   **nodes,
                             guint             n_nodes)
{
  GskRenderNode *node, *shadow_node;

  node = bobgui_snapshot_collect_default (snapshot, state, nodes, n_nodes);
  if (node == NULL)
    return NULL;

  shadow_node = gsk_shadow_node_new2 (node,
                                      state->data.shadow.shadows != NULL
                                        ? state->data.shadow.shadows
                                        : &state->data.shadow.a_shadow,
                                      state->data.shadow.n_shadows);

  gsk_render_node_unref (node);

  return shadow_node;
}

/**
 * bobgui_snapshot_append_stroke:
 * @snapshot: a `BobguiSnapshot`
 * @path: The path describing the area to fill
 * @stroke: The stroke attributes
 * @color: the color to fill the path with
 *
 * A convenience method to stroke a path with a color.
 *
 * See [method@Bobgui.Snapshot.push_stroke] if you need
 * to stroke a path with more complex content than
 * a color.
 *
 * Since: 4.14
 */
void
bobgui_snapshot_append_stroke (BobguiSnapshot     *snapshot,
                            GskPath         *path,
                            const GskStroke *stroke,
                            const GdkRGBA   *color)
{
  graphene_rect_t bounds;

  gsk_path_get_stroke_bounds (path, stroke, &bounds);
  bobgui_snapshot_push_stroke (snapshot, path, stroke);
  bobgui_snapshot_append_color (snapshot, color, &bounds);
  bobgui_snapshot_pop (snapshot);
}

static void
bobgui_snapshot_clear_shadow (BobguiSnapshotState *state)
{
  if (state->data.shadow.shadows != 0)
    for (gsize i = 0; i < state->data.shadow.n_shadows; i++)
      gdk_color_finish (&state->data.shadow.shadows[i].color);
  else
    gdk_color_finish (&state->data.shadow.a_shadow.color);

  g_free (state->data.shadow.shadows);
}

/**
 * bobgui_snapshot_push_shadow:
 * @snapshot: a `BobguiSnapshot`
 * @shadow: (array length=n_shadows): the first shadow specification
 * @n_shadows: number of shadow specifications
 *
 * Applies a shadow to an image.
 *
 * The image is recorded until the next call to [method@Bobgui.Snapshot.pop].
 */
void
bobgui_snapshot_push_shadow (BobguiSnapshot     *snapshot,
                          const GskShadow *shadow,
                          gsize            n_shadows)
{
  GskShadowEntry *shadow2;

  g_return_if_fail (n_shadows > 0);

  shadow2 = g_new (GskShadowEntry, n_shadows);
  for (gsize i = 0; i < n_shadows; i++)
    {
      gdk_color_init_from_rgba (&shadow2[i].color, &shadow[i].color);
      graphene_point_init (&shadow2[i].offset, shadow[i].dx,shadow[i].dy);
      shadow2[i].radius = shadow[i].radius;
    }

  bobgui_snapshot_push_shadows (snapshot, shadow2, n_shadows);

  for (gsize i = 0; i < n_shadows; i++)
    gdk_color_finish (&shadow2[i].color);

  g_free (shadow2);
}

/*< private >
 * bobgui_snapshot_push_shadows:
 * @snapshot: a `BobguiSnapshot`
 * @shadow: (array length=n_shadows): the first shadow specification
 * @n_shadows: number of shadow specifications
 *
 * Applies a shadow to an image.
 *
 * The image is recorded until the next call to [method@Bobgui.Snapshot.pop].
 */
void
bobgui_snapshot_push_shadows (BobguiSnapshot          *snapshot,
                           const GskShadowEntry *shadow,
                           gsize                 n_shadows)
{
  BobguiSnapshotState *state;
  GskTransform *transform;
  float scale_x, scale_y, dx, dy;
  gsize i;

  bobgui_snapshot_ensure_affine_with_flags (snapshot,
                                         ENSURE_POSITIVE_SCALE | ENSURE_UNIFORM_SCALE,
                                         &scale_x, &scale_y,
                                         &dx, &dy);
  transform = gsk_transform_scale (gsk_transform_translate (NULL, &GRAPHENE_POINT_INIT (dx, dy)), scale_x, scale_y);

  state = bobgui_snapshot_push_state (snapshot,
                                   transform,
                                   bobgui_snapshot_collect_shadow,
                                   bobgui_snapshot_clear_shadow);

  state->data.shadow.n_shadows = n_shadows;
  if (n_shadows == 1)
    {
      state->data.shadow.shadows = NULL;
      gdk_color_init_copy (&state->data.shadow.a_shadow.color, &shadow->color);
      graphene_point_init (&state->data.shadow.a_shadow.offset,
                           shadow->offset.x * scale_x,
                           shadow->offset.y * scale_y);
      state->data.shadow.a_shadow.radius = shadow->radius * scale_x;
    }
  else
    {
      state->data.shadow.shadows = g_malloc (sizeof (GskShadowEntry) * n_shadows);
      memcpy (state->data.shadow.shadows, shadow, sizeof (GskShadowEntry) * n_shadows);
      for (i = 0; i < n_shadows; i++)
        {
          gdk_color_init_copy (&state->data.shadow.shadows[i].color, &shadow[i].color);
          graphene_point_init (&state->data.shadow.shadows[i].offset,
                               shadow[i].offset.x * scale_x,
                               shadow[i].offset.y * scale_y);
          state->data.shadow.shadows[i].radius = shadow[i].radius * scale_x;
        }
    }

  gsk_transform_unref (transform);
}

static GskRenderNode *
bobgui_snapshot_collect_blend_top (BobguiSnapshot      *snapshot,
                                BobguiSnapshotState *state,
                                GskRenderNode   **nodes,
                                guint             n_nodes)
{
  GskRenderNode *bottom_node, *top_node, *blend_node;
  GdkRGBA transparent = { 0, 0, 0, 0 };

  top_node = bobgui_snapshot_collect_default (snapshot, state, nodes, n_nodes);
  bottom_node = state->data.blend.bottom_node != NULL
              ? gsk_render_node_ref (state->data.blend.bottom_node)
              : NULL;

  /* XXX: Is this necessary? Do we need a NULL node? */
  if (top_node == NULL)
    top_node = gsk_color_node_new (&transparent,
                                   bottom_node ? &bottom_node->bounds
                                               : &GRAPHENE_RECT_INIT (0, 0, 0, 0));
  if (bottom_node == NULL)
    bottom_node = gsk_color_node_new (&transparent, &top_node->bounds);

  blend_node = gsk_blend_node_new (bottom_node, top_node, state->data.blend.blend_mode);

  gsk_render_node_unref (top_node);
  gsk_render_node_unref (bottom_node);

  return blend_node;
}

static void
bobgui_snapshot_clear_blend_top (BobguiSnapshotState *state)
{
  g_clear_pointer (&(state->data.blend.bottom_node), gsk_render_node_unref);
}

static GskRenderNode *
bobgui_snapshot_collect_blend_bottom (BobguiSnapshot      *snapshot,
                                   BobguiSnapshotState *state,
                                   GskRenderNode   **nodes,
                                   guint             n_nodes)
{
  BobguiSnapshotState *prev_state = bobgui_snapshot_get_previous_state (snapshot);

  g_assert (prev_state->collect_func == bobgui_snapshot_collect_blend_top);

  prev_state->data.blend.bottom_node = bobgui_snapshot_collect_default (snapshot, state, nodes, n_nodes);

  return NULL;
}

/**
 * bobgui_snapshot_push_blend:
 * @snapshot: a `BobguiSnapshot`
 * @blend_mode: blend mode to use
 *
 * Blends together two images with the given blend mode.
 *
 * Until the first call to [method@Bobgui.Snapshot.pop], the
 * bottom image for the blend operation will be recorded.
 * After that call, the top image to be blended will be
 * recorded until the second call to [method@Bobgui.Snapshot.pop].
 *
 * Calling this function requires two subsequent calls
 * to [method@Bobgui.Snapshot.pop].
 */
void
bobgui_snapshot_push_blend (BobguiSnapshot  *snapshot,
                         GskBlendMode  blend_mode)
{
  BobguiSnapshotState *current_state = bobgui_snapshot_get_current_state (snapshot);
  BobguiSnapshotState *top_state;

  top_state = bobgui_snapshot_push_state (snapshot,
                                       current_state->transform,
                                       bobgui_snapshot_collect_blend_top,
                                       bobgui_snapshot_clear_blend_top);
  top_state->data.blend.blend_mode = blend_mode;

  bobgui_snapshot_push_state (snapshot,
                           top_state->transform,
                           bobgui_snapshot_collect_blend_bottom,
                           NULL);
}

static GskRenderNode *
bobgui_snapshot_collect_mask_source (BobguiSnapshot      *snapshot,
                                  BobguiSnapshotState *state,
                                  GskRenderNode   **nodes,
                                  guint             n_nodes)
{
  GskRenderNode *source_child, *mask_child, *mask_node;

  mask_child = state->data.mask.mask_node;
  source_child = bobgui_snapshot_collect_default (snapshot, state, nodes, n_nodes);
  if (source_child == NULL)
    return NULL;

  if (mask_child)
    mask_node = gsk_mask_node_new (source_child, mask_child, state->data.mask.mask_mode);
  else if (state->data.mask.mask_mode == GSK_MASK_MODE_INVERTED_ALPHA)
    mask_node = gsk_render_node_ref (source_child);
  else
    mask_node = NULL;

  gsk_render_node_unref (source_child);

  return mask_node;
}

static void
bobgui_snapshot_clear_mask_source (BobguiSnapshotState *state)
{
  g_clear_pointer (&(state->data.mask.mask_node), gsk_render_node_unref);
}

static GskRenderNode *
bobgui_snapshot_collect_mask_mask (BobguiSnapshot      *snapshot,
                                BobguiSnapshotState *state,
                                GskRenderNode   **nodes,
                                guint             n_nodes)
{
  BobguiSnapshotState *prev_state = bobgui_snapshot_get_previous_state (snapshot);

  g_assert (prev_state->collect_func == bobgui_snapshot_collect_mask_source);

  prev_state->data.mask.mask_node = bobgui_snapshot_collect_default (snapshot, state, nodes, n_nodes);

  return NULL;
}

/**
 * bobgui_snapshot_push_mask:
 * @snapshot: a #BobguiSnapshot
 * @mask_mode: mask mode to use
 *
 * Until the first call to [method@Bobgui.Snapshot.pop], the
 * mask image for the mask operation will be recorded.
 *
 * After that call, the source image will be recorded until
 * the second call to [method@Bobgui.Snapshot.pop].
 *
 * Calling this function requires 2 subsequent calls to bobgui_snapshot_pop().
 *
 * Since: 4.10
 */
void
bobgui_snapshot_push_mask (BobguiSnapshot *snapshot,
                        GskMaskMode  mask_mode)
{
  BobguiSnapshotState *current_state = bobgui_snapshot_get_current_state (snapshot);
  BobguiSnapshotState *source_state;

  source_state = bobgui_snapshot_push_state (snapshot,
                                          current_state->transform,
                                          bobgui_snapshot_collect_mask_source,
                                          bobgui_snapshot_clear_mask_source);

  source_state->data.mask.mask_mode = mask_mode;

  bobgui_snapshot_push_state (snapshot,
                           source_state->transform,
                           bobgui_snapshot_collect_mask_mask,
                           NULL);
}

static GskRenderNode *
bobgui_snapshot_collect_copy (BobguiSnapshot      *snapshot,
                           BobguiSnapshotState *state,
                           GskRenderNode   **nodes,
                           guint             n_nodes)
{
  GskRenderNode *node, *copy_node;

  node = bobgui_snapshot_collect_default (snapshot, state, nodes, n_nodes);
  if (node == NULL)
    return NULL;

  copy_node = gsk_copy_node_new (node);
  gsk_render_node_unref (node);

  return copy_node;
}

/**
 * bobgui_snapshot_push_copy:
 * @snapshot: a `BobguiSnapshot`
 *
 * Stores the current rendering state for later pasting via
 * [method@Bobgui.Snapshot.append_paste].
 *
 * Pasting is possible until the matching call to [method@Bobgui.Snapshot.pop].
 *
 * Since: 4.22
 */
void
bobgui_snapshot_push_copy (BobguiSnapshot *snapshot)
{
  BobguiSnapshotState *current_state;

  /* need identity here because the coords are used
   * by pastes */
  bobgui_snapshot_ensure_identity (snapshot);

  current_state = bobgui_snapshot_get_current_state (snapshot);

  bobgui_snapshot_push_state (snapshot,
                           current_state->transform,
                           bobgui_snapshot_collect_copy,
                           NULL);
}

static GskRenderNode *
bobgui_snapshot_collect_composite_child (BobguiSnapshot      *snapshot,
                                      BobguiSnapshotState *state,
                                      GskRenderNode   **nodes,
                                      guint             n_nodes)
{
  GskRenderNode *child, *mask, *result;

  mask = state->data.composite.mask;
  child = bobgui_snapshot_collect_default (snapshot, state, nodes, n_nodes);
  if (child == NULL)
    return NULL;

  if (mask == NULL)
    {
      gsk_render_node_unref (child);
      return NULL;
    }

  result = gsk_composite_node_new (child, mask, state->data.composite.op);

  gsk_render_node_unref (child);

  return result;
}

static void
bobgui_snapshot_clear_composite_child (BobguiSnapshotState *state)
{
  g_clear_pointer (&(state->data.composite.mask), gsk_render_node_unref);
}

static GskRenderNode *
bobgui_snapshot_collect_composite_mask (BobguiSnapshot      *snapshot,
                                     BobguiSnapshotState *state,
                                     GskRenderNode   **nodes,
                                     guint             n_nodes)
{
  BobguiSnapshotState *prev_state = bobgui_snapshot_get_previous_state (snapshot);

  g_assert (prev_state->collect_func == bobgui_snapshot_collect_composite_child);

  prev_state->data.composite.mask = bobgui_snapshot_collect_default (snapshot, state, nodes, n_nodes);

  return NULL;
}

/**
 * bobgui_snapshot_push_composite:
 * @snapshot: a #BobguiSnapshot
 * @op: The Porter/Duff compositing operator to use
 *
 * Until the first call to [method@Bobgui.Snapshot.pop], the
 * mask image for the mask operation will be recorded.
 *
 * After that call, the child image will be recorded until
 * the second call to [method@Bobgui.Snapshot.pop].
 *
 * Calling this function requires 2 subsequent calls to bobgui_snapshot_pop().
 *
 * Since: 4.22
 */
void
bobgui_snapshot_push_composite (BobguiSnapshot   *snapshot,
                             GskPorterDuff  op)
{
  BobguiSnapshotState *current_state = bobgui_snapshot_get_current_state (snapshot);
  BobguiSnapshotState *child_state;

  child_state = bobgui_snapshot_push_state (snapshot,
                                         current_state->transform,
                                         bobgui_snapshot_collect_composite_child,
                                         bobgui_snapshot_clear_composite_child);

  child_state->data.composite.op= op;

  bobgui_snapshot_push_state (snapshot,
                           child_state->transform,
                           bobgui_snapshot_collect_composite_mask,
                           NULL);
}

static GskRenderNode *
bobgui_snapshot_collect_cross_fade_end (BobguiSnapshot      *snapshot,
                                     BobguiSnapshotState *state,
                                     GskRenderNode   **nodes,
                                     guint             n_nodes)
{
  GskRenderNode *start_node, *end_node, *node;

  end_node = bobgui_snapshot_collect_default (snapshot, state, nodes, n_nodes);
  start_node = state->data.cross_fade.start_node;
  state->data.cross_fade.start_node = NULL;

  if (state->data.cross_fade.progress <= 0.0)
    {
      node = start_node;

      if (end_node)
        gsk_render_node_unref (end_node);
    }
  else if (state->data.cross_fade.progress >= 1.0)
    {
      node = end_node;

      if (start_node)
        gsk_render_node_unref (start_node);
    }
  else if (start_node && end_node)
    {
      node = gsk_cross_fade_node_new (start_node, end_node, state->data.cross_fade.progress);

      gsk_render_node_unref (start_node);
      gsk_render_node_unref (end_node);
    }
  else if (start_node)
    {
      node = gsk_opacity_node_new (start_node, 1.0 - state->data.cross_fade.progress);

      gsk_render_node_unref (start_node);
    }
  else if (end_node)
    {
      node = gsk_opacity_node_new (end_node, state->data.cross_fade.progress);

      gsk_render_node_unref (end_node);
    }
  else
    {
      node = NULL;
    }

  return node;
}

static void
bobgui_snapshot_clear_cross_fade_end (BobguiSnapshotState *state)
{
  g_clear_pointer (&state->data.cross_fade.start_node, gsk_render_node_unref);
}

static GskRenderNode *
bobgui_snapshot_collect_cross_fade_start (BobguiSnapshot      *snapshot,
                                       BobguiSnapshotState *state,
                                       GskRenderNode   **nodes,
                                       guint             n_nodes)
{
  BobguiSnapshotState *prev_state = bobgui_snapshot_get_previous_state (snapshot);

  g_assert (prev_state->collect_func == bobgui_snapshot_collect_cross_fade_end);

  prev_state->data.cross_fade.start_node = bobgui_snapshot_collect_default (snapshot, state, nodes, n_nodes);

  return NULL;
}

/**
 * bobgui_snapshot_push_cross_fade:
 * @snapshot: a `BobguiSnapshot`
 * @progress: progress between 0.0 and 1.0
 *
 * Snapshots a cross-fade operation between two images with the
 * given @progress.
 *
 * Until the first call to [method@Bobgui.Snapshot.pop], the start image
 * will be snapshot. After that call, the end image will be recorded
 * until the second call to [method@Bobgui.Snapshot.pop].
 *
 * Calling this function requires two subsequent calls
 * to [method@Bobgui.Snapshot.pop].
 */
void
bobgui_snapshot_push_cross_fade (BobguiSnapshot *snapshot,
                              double       progress)
{
  const BobguiSnapshotState *current_state = bobgui_snapshot_get_current_state (snapshot);
  BobguiSnapshotState *end_state;

  end_state = bobgui_snapshot_push_state (snapshot,
                                       current_state->transform,
                                       bobgui_snapshot_collect_cross_fade_end,
                                       bobgui_snapshot_clear_cross_fade_end);
  end_state->data.cross_fade.progress = progress;

  bobgui_snapshot_push_state (snapshot,
                           end_state->transform,
                           bobgui_snapshot_collect_cross_fade_start,
                           NULL);
}

static GskRenderNode *
bobgui_snapshot_pop_one (BobguiSnapshot *snapshot)
{
  BobguiSnapshotState *state;
  guint state_index;
  GskRenderNode *node;

  if (bobgui_snapshot_states_is_empty (&snapshot->state_stack))
    {
      g_warning ("Too many bobgui_snapshot_pop() calls.");
      return NULL;
    }

  state = bobgui_snapshot_get_current_state (snapshot);
  state_index = bobgui_snapshot_states_get_size (&snapshot->state_stack) - 1;

  if (state->collect_func)
    {
      node = state->collect_func (snapshot,
                                  state,
                                  (GskRenderNode **) bobgui_snapshot_nodes_index (&snapshot->nodes, state->start_node_index),
                                  state->n_nodes);

      /* The collect func may not modify the state stack... */
      g_assert (state_index == bobgui_snapshot_states_get_size (&snapshot->state_stack) - 1);

      /* Remove all the state's nodes from the list of nodes */
      g_assert (state->start_node_index + state->n_nodes == bobgui_snapshot_nodes_get_size (&snapshot->nodes));
      bobgui_snapshot_nodes_splice (&snapshot->nodes, state->start_node_index, state->n_nodes, FALSE, NULL, 0);
    }
  else
    {
      BobguiSnapshotState *previous_state;

      node = NULL;

      /* move the nodes to the parent */
      previous_state = bobgui_snapshot_get_previous_state (snapshot);
      previous_state->n_nodes += state->n_nodes;
      g_assert (previous_state->start_node_index + previous_state->n_nodes == bobgui_snapshot_nodes_get_size (&snapshot->nodes));
    }

  bobgui_snapshot_states_splice (&snapshot->state_stack, state_index, 1, FALSE, NULL, 0);

  return node;
}

static void
bobgui_snapshot_append_node_internal (BobguiSnapshot   *snapshot,
                                   GskRenderNode *node)
{
  BobguiSnapshotState *current_state;

  current_state = bobgui_snapshot_get_current_state (snapshot);

  if (current_state)
    {
      if (gsk_render_node_get_node_type (node) == GSK_CONTAINER_NODE)
        {
          GskRenderNode **children;
          gsize i, n_children;

          children = gsk_render_node_get_children (node, &n_children);
          for (i = 0; i < n_children; i++)
            gsk_render_node_ref (children[i]);
          bobgui_snapshot_nodes_splice (&snapshot->nodes,
                                     bobgui_snapshot_nodes_get_size (&snapshot->nodes),
                                     0,
                                     FALSE,
                                     children,
                                     n_children);
          current_state->n_nodes += n_children;
          gsk_render_node_unref (node);
        }
      else
        {
          bobgui_snapshot_nodes_append (&snapshot->nodes, node);
          current_state->n_nodes ++;
        }
    }
  else
    {
      g_critical ("Tried appending a node to an already finished snapshot.");
    }
}

static GskRenderNode *
bobgui_snapshot_pop_internal (BobguiSnapshot *snapshot,
                           gboolean     is_texture_pop)
{
  BobguiSnapshotState *state;
  GskRenderNode *node;
  guint forgotten_restores = 0;

  for (state = bobgui_snapshot_get_current_state (snapshot);
       bobgui_snapshot_state_should_autopop (state) ||
       state->collect_func == NULL;
       state = bobgui_snapshot_get_current_state (snapshot))
    {
      if (state->collect_func == NULL)
        forgotten_restores++;

      node = bobgui_snapshot_pop_one (snapshot);
      if (node)
        bobgui_snapshot_append_node_internal (snapshot, node);
    }

  if (forgotten_restores)
    {
      g_warning ("Too many bobgui_snapshot_save() calls. %u saves remaining.", forgotten_restores);
    }

  if (is_texture_pop && (state->collect_func != bobgui_snapshot_collect_gl_shader_texture))
    {
      g_critical ("Unexpected call to bobgui_snapshot_gl_shader_pop_texture().");
      return NULL;
    }
  else if (!is_texture_pop && (state->collect_func == bobgui_snapshot_collect_gl_shader_texture))
    {
      g_critical ("Expected a call to bobgui_snapshot_gl_shader_pop_texture().");
      return NULL;
    }

  return bobgui_snapshot_pop_one (snapshot);
}

/**
 * bobgui_snapshot_push_collect:
 *
 * Private.
 *
 * Pushes state so a later pop_collect call can collect all nodes
 * appended until that point.
 */
void
bobgui_snapshot_push_collect (BobguiSnapshot *snapshot)
{
  bobgui_snapshot_push_state (snapshot,
                           NULL,
                           bobgui_snapshot_collect_default,
                           NULL);
}

GskRenderNode *
bobgui_snapshot_pop_collect (BobguiSnapshot *snapshot)
{
  GskRenderNode *result = bobgui_snapshot_pop_internal (snapshot, FALSE);

  return result;
}

/**
 * bobgui_snapshot_to_node:
 * @snapshot: a `BobguiSnapshot`
 *
 * Returns the render node that was constructed
 * by @snapshot.
 *
 * Note that this function may return %NULL if nothing has been
 * added to the snapshot or if its content does not produce pixels
 * to be rendered.
 *
 * After calling this function, it is no longer possible to
 * add more nodes to @snapshot. The only function that should
 * be called after this is [method@GObject.Object.unref].
 *
 * Returns: (transfer full) (nullable): the constructed `GskRenderNode` or
 *   %NULL if there are no nodes to render.
 */
GskRenderNode *
bobgui_snapshot_to_node (BobguiSnapshot *snapshot)
{
  GskRenderNode *result;

  result = bobgui_snapshot_pop_internal (snapshot, FALSE);

  /* We should have exactly our initial state */
  if (!bobgui_snapshot_states_is_empty (&snapshot->state_stack))
    {
      g_warning ("Too many bobgui_snapshot_push() calls. %zu states remaining.",
                 bobgui_snapshot_states_get_size (&snapshot->state_stack));
    }

  bobgui_snapshot_states_clear (&snapshot->state_stack);
  bobgui_snapshot_nodes_clear (&snapshot->nodes);

  return result;
}

/**
 * bobgui_snapshot_to_paintable:
 * @snapshot: a `BobguiSnapshot`
 * @size: (nullable): The size of the resulting paintable
 *   or %NULL to use the bounds of the snapshot
 *
 * Returns a paintable encapsulating the render node
 * that was constructed by @snapshot.
 *
 * After calling this function, it is no longer possible to
 * add more nodes to @snapshot. The only function that should
 * be called after this is [method@GObject.Object.unref].
 *
 * Returns: (transfer full) (nullable): a new `GdkPaintable`
 */
GdkPaintable *
bobgui_snapshot_to_paintable (BobguiSnapshot           *snapshot,
                           const graphene_size_t *size)
{
  GskRenderNode *node;
  GdkPaintable *paintable;
  graphene_rect_t bounds;

  node = bobgui_snapshot_to_node (snapshot);
  if (size)
    {
      graphene_size_init_from_size (&bounds.size, size);
    }
  else if (node)
    {
      gsk_render_node_get_bounds (node, &bounds);
      bounds.size.width += bounds.origin.x;
      bounds.size.height += bounds.origin.y;
    }
  else
    {
      bounds.size.width = 0;
      bounds.size.height = 0;
    }
  bounds.origin.x = 0;
  bounds.origin.y = 0;

  paintable = bobgui_render_node_paintable_new (node, &bounds);
  g_clear_pointer (&node, gsk_render_node_unref);

  return paintable;
}

/**
 * bobgui_snapshot_pop:
 * @snapshot: a `BobguiSnapshot`
 *
 * Removes the top element from the stack of render nodes,
 * and appends it to the node underneath it.
 */
void
bobgui_snapshot_pop (BobguiSnapshot *snapshot)
{
  GskRenderNode *node;

  node = bobgui_snapshot_pop_internal (snapshot, FALSE);

  if (node)
    bobgui_snapshot_append_node_internal (snapshot, node);
}

/**
 * bobgui_snapshot_gl_shader_pop_texture:
 * @snapshot: a `BobguiSnapshot`
 *
 * Removes the top element from the stack of render nodes and
 * adds it to the nearest [class@Gsk.GLShaderNode] below it.
 *
 * This must be called the same number of times as the number
 * of textures is needed for the shader in
 * [method@Bobgui.Snapshot.push_gl_shader].
 *
 * Deprecated: 4.16: BOBGUI's new Vulkan-focused rendering
 *   does not support this feature. Use [class@Bobgui.GLArea] for
 *   OpenGL rendering.
 */
void
bobgui_snapshot_gl_shader_pop_texture (BobguiSnapshot *snapshot)
{
  G_GNUC_UNUSED GskRenderNode *node;

  node = bobgui_snapshot_pop_internal (snapshot, TRUE);
  g_assert (node == NULL);
}


/**
 * bobgui_snapshot_save:
 * @snapshot: a `BobguiSnapshot`
 *
 * Makes a copy of the current state of @snapshot and saves it
 * on an internal stack.
 *
 * When [method@Bobgui.Snapshot.restore] is called, @snapshot will
 * be restored to the saved state.
 *
 * Multiple calls to [method@Bobgui.Snapshot.save] and [method@Bobgui.Snapshot.restore]
 * can be nested; each call to `bobgui_snapshot_restore()` restores the state from
 * the matching paired `bobgui_snapshot_save()`.
 *
 * It is necessary to clear all saved states with corresponding
 * calls to `bobgui_snapshot_restore()`.
 */
void
bobgui_snapshot_save (BobguiSnapshot *snapshot)
{
  g_return_if_fail (BOBGUI_IS_SNAPSHOT (snapshot));

  bobgui_snapshot_push_state (snapshot,
                           bobgui_snapshot_get_current_state (snapshot)->transform,
                           NULL,
                           NULL);
}

/**
 * bobgui_snapshot_restore:
 * @snapshot: a `BobguiSnapshot`
 *
 * Restores @snapshot to the state saved by a preceding call to
 * [method@Snapshot.save] and removes that state from the stack of
 * saved states.
 */
void
bobgui_snapshot_restore (BobguiSnapshot *snapshot)
{
  BobguiSnapshotState *state;
  GskRenderNode *node;

  for (state = bobgui_snapshot_get_current_state (snapshot);
       bobgui_snapshot_state_should_autopop (state);
       state = bobgui_snapshot_get_current_state (snapshot))
    {
      node = bobgui_snapshot_pop_one (snapshot);
      if (node)
        bobgui_snapshot_append_node_internal (snapshot, node);
    }

  if (state->collect_func != NULL)
    {
      g_warning ("Too many bobgui_snapshot_restore() calls.");
      return;
    }

  node = bobgui_snapshot_pop_one (snapshot);
  g_assert (node == NULL);
}

/**
 * bobgui_snapshot_transform:
 * @snapshot: a `BobguiSnapshot`
 * @transform: (nullable): the transform to apply
 *
 * Transforms @snapshot's coordinate system with the given @transform.
 */
void
bobgui_snapshot_transform (BobguiSnapshot  *snapshot,
                        GskTransform *transform)
{
  BobguiSnapshotState *state;

  g_return_if_fail (BOBGUI_IS_SNAPSHOT (snapshot));

  state = bobgui_snapshot_get_current_state (snapshot);
  state->transform = gsk_transform_transform (state->transform, transform);
}

/**
 * bobgui_snapshot_transform_matrix:
 * @snapshot: a `BobguiSnapshot`
 * @matrix: the matrix to multiply the transform with
 *
 * Transforms @snapshot's coordinate system with the given @matrix.
 */
void
bobgui_snapshot_transform_matrix (BobguiSnapshot             *snapshot,
                               const graphene_matrix_t *matrix)
{
  BobguiSnapshotState *state;

  g_return_if_fail (BOBGUI_IS_SNAPSHOT (snapshot));
  g_return_if_fail (matrix != NULL);

  state = bobgui_snapshot_get_current_state (snapshot);
  state->transform = gsk_transform_matrix (state->transform, matrix);
}

/**
 * bobgui_snapshot_translate:
 * @snapshot: a `BobguiSnapshot`
 * @point: the point to translate the snapshot by
 *
 * Translates @snapshot's coordinate system by @point in 2-dimensional space.
 */
void
bobgui_snapshot_translate (BobguiSnapshot            *snapshot,
                        const graphene_point_t *point)
{
  BobguiSnapshotState *state;

  g_return_if_fail (BOBGUI_IS_SNAPSHOT (snapshot));
  g_return_if_fail (point != NULL);

  state = bobgui_snapshot_get_current_state (snapshot);
  state->transform = gsk_transform_translate (state->transform, point);
}

/**
 * bobgui_snapshot_translate_3d:
 * @snapshot: a `BobguiSnapshot`
 * @point: the point to translate the snapshot by
 *
 * Translates @snapshot's coordinate system by @point.
 */
void
bobgui_snapshot_translate_3d (BobguiSnapshot              *snapshot,
                           const graphene_point3d_t *point)
{
  BobguiSnapshotState *state;

  g_return_if_fail (BOBGUI_IS_SNAPSHOT (snapshot));
  g_return_if_fail (point != NULL);

  state = bobgui_snapshot_get_current_state (snapshot);
  state->transform = gsk_transform_translate_3d (state->transform, point);
}

/**
 * bobgui_snapshot_rotate:
 * @snapshot: a `BobguiSnapshot`
 * @angle: the rotation angle, in degrees (clockwise)
 *
 * Rotates @@snapshot's coordinate system by @angle degrees in 2D space -
 * or in 3D speak, rotates around the Z axis. The rotation happens around
 * the origin point of (0, 0) in the @snapshot's current coordinate system.
 *
 * To rotate around axes other than the Z axis, use [method@Gsk.Transform.rotate_3d].
 */
void
bobgui_snapshot_rotate (BobguiSnapshot *snapshot,
                     float        angle)
{
  BobguiSnapshotState *state;

  g_return_if_fail (BOBGUI_IS_SNAPSHOT (snapshot));

  state = bobgui_snapshot_get_current_state (snapshot);
  state->transform = gsk_transform_rotate (state->transform, angle);
}

/**
 * bobgui_snapshot_rotate_3d:
 * @snapshot: a `BobguiSnapshot`
 * @angle: the rotation angle, in degrees (clockwise)
 * @axis: The rotation axis
 *
 * Rotates @snapshot's coordinate system by @angle degrees around @axis.
 *
 * For a rotation in 2D space, use [method@Gsk.Transform.rotate].
 */
void
bobgui_snapshot_rotate_3d (BobguiSnapshot           *snapshot,
                        float                  angle,
                        const graphene_vec3_t *axis)
{
  BobguiSnapshotState *state;

  g_return_if_fail (BOBGUI_IS_SNAPSHOT (snapshot));
  g_return_if_fail (axis != NULL);

  state = bobgui_snapshot_get_current_state (snapshot);
  state->transform = gsk_transform_rotate_3d (state->transform, angle, axis);
}

/**
 * bobgui_snapshot_scale:
 * @snapshot: a `BobguiSnapshot`
 * @factor_x: scaling factor on the X axis
 * @factor_y: scaling factor on the Y axis
 *
 * Scales @snapshot's coordinate system in 2-dimensional space by
 * the given factors.
 *
 * Use [method@Bobgui.Snapshot.scale_3d] to scale in all 3 dimensions.
 */
void
bobgui_snapshot_scale (BobguiSnapshot *snapshot,
                    float        factor_x,
                    float        factor_y)
{
  BobguiSnapshotState *state;

  g_return_if_fail (BOBGUI_IS_SNAPSHOT (snapshot));

  state = bobgui_snapshot_get_current_state (snapshot);
  state->transform = gsk_transform_scale (state->transform, factor_x, factor_y);
}

/**
 * bobgui_snapshot_scale_3d:
 * @snapshot: a `BobguiSnapshot`
 * @factor_x: scaling factor on the X axis
 * @factor_y: scaling factor on the Y axis
 * @factor_z: scaling factor on the Z axis
 *
 * Scales @snapshot's coordinate system by the given factors.
 */
void
bobgui_snapshot_scale_3d (BobguiSnapshot *snapshot,
                       float        factor_x,
                       float        factor_y,
                       float        factor_z)
{
  BobguiSnapshotState *state;

  g_return_if_fail (BOBGUI_IS_SNAPSHOT (snapshot));

  state = bobgui_snapshot_get_current_state (snapshot);
  state->transform = gsk_transform_scale_3d (state->transform, factor_x, factor_y, factor_z);
}

/**
 * bobgui_snapshot_perspective:
 * @snapshot: a `BobguiSnapshot`
 * @depth: distance of the z=0 plane
 *
 * Applies a perspective projection transform.
 *
 * See [method@Gsk.Transform.perspective] for a discussion on the details.
 */
void
bobgui_snapshot_perspective (BobguiSnapshot *snapshot,
                          float        depth)
{
  BobguiSnapshotState *state;

  g_return_if_fail (BOBGUI_IS_SNAPSHOT (snapshot));

  state = bobgui_snapshot_get_current_state (snapshot);
  state->transform = gsk_transform_perspective (state->transform, depth);
}

/**
 * bobgui_snapshot_append_node:
 * @snapshot: a `BobguiSnapshot`
 * @node: a `GskRenderNode`
 *
 * Appends @node to the current render node of @snapshot,
 * without changing the current node.
 *
 * If @snapshot does not have a current node yet, @node
 * will become the initial node.
 */
void
bobgui_snapshot_append_node (BobguiSnapshot   *snapshot,
                          GskRenderNode *node)
{
  g_return_if_fail (snapshot != NULL);
  g_return_if_fail (GSK_IS_RENDER_NODE (node));

  bobgui_snapshot_ensure_identity (snapshot);

  bobgui_snapshot_append_node_internal (snapshot, gsk_render_node_ref (node));
}

/*< private>
 * bobgui_snapshot_append_node_scaled:
 * @snapshot: a `BobguiSnapshot`
 * @node: a `GskRenderNode`
 * @from: first rectangle
 * @to: second rectangle
 *
 * Appends @node to the current render node of @snapshot,
 * without changing the current node, with a transform
 * that maps @from to @to.
 *
 * If @snapshot does not have a current node yet, @node
 * will become the initial node.
 */
void
bobgui_snapshot_append_node_scaled (BobguiSnapshot     *snapshot,
                                 GskRenderNode   *node,
                                 graphene_rect_t *from,
                                 graphene_rect_t *to)
{
  if (gsk_render_node_get_node_type (node) == GSK_TEXTURE_NODE &&
      gsk_rect_equal (from, &node->bounds))
    {
      bobgui_snapshot_append_texture (snapshot, gsk_texture_node_get_texture (node), to);
    }
  else if (gsk_rect_equal (from, to))
    {
      bobgui_snapshot_append_node (snapshot, node);
    }
  else
    {
      bobgui_snapshot_save (snapshot);
      bobgui_snapshot_translate (snapshot, &GRAPHENE_POINT_INIT (to->origin.x,
                                                              to->origin.y));
      bobgui_snapshot_scale (snapshot, to->size.width / from->size.width,
                                    to->size.height / from->size.height);
      bobgui_snapshot_translate (snapshot, &GRAPHENE_POINT_INIT (- from->origin.x,
                                                              - from->origin.y));
      bobgui_snapshot_append_node (snapshot, node);
      bobgui_snapshot_restore (snapshot);
    }
}

/**
 * bobgui_snapshot_append_cairo:
 * @snapshot: a `BobguiSnapshot`
 * @bounds: the bounds for the new node
 *
 * Creates a new [class@Gsk.CairoNode] and appends it to the current
 * render node of @snapshot, without changing the current node.
 *
 * Returns: a `cairo_t` suitable for drawing the contents of
 *   the newly created render node
 */
cairo_t *
bobgui_snapshot_append_cairo (BobguiSnapshot           *snapshot,
                           const graphene_rect_t *bounds)
{
  GskRenderNode *node;
  graphene_rect_t real_bounds;
  float scale_x, scale_y, dx, dy;
  cairo_t *cr;

  g_return_val_if_fail (snapshot != NULL, NULL);
  g_return_val_if_fail (bounds != NULL, NULL);

  bobgui_snapshot_ensure_affine (snapshot, &scale_x, &scale_y, &dx, &dy);
  bobgui_graphene_rect_scale_affine (bounds, scale_x, scale_y, dx, dy, &real_bounds);

  node = gsk_cairo_node_new (&real_bounds);

  bobgui_snapshot_append_node_internal (snapshot, node);

  cr = gsk_cairo_node_get_draw_context (node);

  cairo_scale (cr, scale_x, scale_y);
  cairo_translate (cr, dx, dy);

  return cr;
}

/**
 * bobgui_snapshot_append_texture:
 * @snapshot: a `BobguiSnapshot`
 * @texture: the texture to render
 * @bounds: the bounds for the new node
 *
 * Creates a new render node drawing the @texture
 * into the given @bounds and appends it to the
 * current render node of @snapshot.
 *
 * If the texture needs to be scaled to fill @bounds,
 * linear filtering is used. See [method@Bobgui.Snapshot.append_scaled_texture]
 * if you need other filtering, such as nearest-neighbour.
 */
void
bobgui_snapshot_append_texture (BobguiSnapshot           *snapshot,
                             GdkTexture            *texture,
                             const graphene_rect_t *bounds)
{
  GskRenderNode *node;
  graphene_rect_t real_bounds;
  float scale_x, scale_y, dx, dy;

  g_return_if_fail (snapshot != NULL);
  g_return_if_fail (GDK_IS_TEXTURE (texture));
  g_return_if_fail (bounds != NULL);

  bobgui_snapshot_ensure_affine (snapshot, &scale_x, &scale_y, &dx, &dy);
  bobgui_graphene_rect_scale_affine (bounds, scale_x, scale_y, dx, dy, &real_bounds);
  node = gsk_texture_node_new (texture, &real_bounds);

  bobgui_snapshot_append_node_internal (snapshot, node);
}

/**
 * bobgui_snapshot_append_scaled_texture:
 * @snapshot: a `BobguiSnapshot`
 * @texture: the texture to render
 * @filter: the filter to use
 * @bounds: the bounds for the new node
 *
 * Creates a new render node drawing the @texture
 * into the given @bounds and appends it to the
 * current render node of @snapshot.
 *
 * In contrast to [method@Bobgui.Snapshot.append_texture],
 * this function provides control about how the filter
 * that is used when scaling.
 *
 * Since: 4.10
 */
void
bobgui_snapshot_append_scaled_texture (BobguiSnapshot           *snapshot,
                                    GdkTexture            *texture,
                                    GskScalingFilter       filter,
                                    const graphene_rect_t *bounds)
{
  GskRenderNode *node;

  g_return_if_fail (snapshot != NULL);
  g_return_if_fail (GDK_IS_TEXTURE (texture));
  g_return_if_fail (bounds != NULL);

  bobgui_snapshot_ensure_identity (snapshot);
  node = gsk_texture_scale_node_new (texture, bounds, filter);

  bobgui_snapshot_append_node_internal (snapshot, node);
}

/**
 * bobgui_snapshot_append_color:
 * @snapshot: a `BobguiSnapshot`
 * @color: the color to draw
 * @bounds: the bounds for the new node
 *
 * Creates a new render node drawing the @color into the
 * given @bounds and appends it to the current render node
 * of @snapshot.
 *
 * You should try to avoid calling this function if
 * @color is transparent.
 */
void
bobgui_snapshot_append_color (BobguiSnapshot           *snapshot,
                           const GdkRGBA         *color,
                           const graphene_rect_t *bounds)
{
  GdkColor color2;
  gdk_color_init_from_rgba (&color2, color);
  bobgui_snapshot_add_color (snapshot, &color2, bounds);
}

/*< private >
 * bobgui_snapshot_add_color:
 * @snapshot: a `BobguiSnapshot`
 * @color: the color to draw
 * @bounds: the bounds for the new node
 *
 * Creates a new render node drawing the @color into the
 * given @bounds and appends it to the current render node
 * of @snapshot.
 */
void
bobgui_snapshot_add_color (BobguiSnapshot           *snapshot,
                        const GdkColor        *color,
                        const graphene_rect_t *bounds)
{
  GskRenderNode *node;
  graphene_rect_t real_bounds;
  float scale_x, scale_y, dx, dy;

  g_return_if_fail (snapshot != NULL);
  g_return_if_fail (color != NULL);
  g_return_if_fail (bounds != NULL);

  bobgui_snapshot_ensure_affine (snapshot, &scale_x, &scale_y, &dx, &dy);
  bobgui_graphene_rect_scale_affine (bounds, scale_x, scale_y, dx, dy, &real_bounds);

  node = gsk_color_node_new2 (color, &real_bounds);

  bobgui_snapshot_append_node_internal (snapshot, node);
}

void
bobgui_snapshot_append_text (BobguiSnapshot           *snapshot,
                          PangoFont             *font,
                          PangoGlyphString      *glyphs,
                          const GdkRGBA         *color,
                          float                  x,
                          float                  y)
{
  GdkColor color2;

  gdk_color_init_from_rgba (&color2, color);
  bobgui_snapshot_add_text (snapshot, font, glyphs, &color2, x, y);
  gdk_color_finish (&color2);
}

void
bobgui_snapshot_add_text (BobguiSnapshot      *snapshot,
                       PangoFont        *font,
                       PangoGlyphString *glyphs,
                       const GdkColor   *color,
                       float             x,
                       float             y)
{
  GskRenderNode *node;
  float dx, dy;

  g_return_if_fail (snapshot != NULL);
  g_return_if_fail (PANGO_IS_FONT (font));
  g_return_if_fail (glyphs != NULL);
  g_return_if_fail (color != NULL);

  bobgui_snapshot_ensure_translate (snapshot, &dx, &dy);

  node = gsk_text_node_new2 (font,
                             glyphs,
                             color,
                             &GRAPHENE_POINT_INIT (x + dx, y + dy));
  if (node == NULL)
    return;

  bobgui_snapshot_append_node_internal (snapshot, node);
}

/**
 * bobgui_snapshot_append_linear_gradient:
 * @snapshot: a `BobguiSnapshot`
 * @bounds: the rectangle to render the linear gradient into
 * @start_point: the point at which the linear gradient will begin
 * @end_point: the point at which the linear gradient will finish
 * @stops: (array length=n_stops): the color stops defining the gradient
 * @n_stops: the number of elements in @stops
 *
 * Appends a linear gradient node with the given stops to @snapshot.
 */
void
bobgui_snapshot_append_linear_gradient (BobguiSnapshot            *snapshot,
                                     const graphene_rect_t  *bounds,
                                     const graphene_point_t *start_point,
                                     const graphene_point_t *end_point,
                                     const GskColorStop     *stops,
                                     gsize                   n_stops)
{
  GskGradient *gradient;

  gradient = gsk_gradient_new ();
  gsk_gradient_add_color_stops (gradient, stops, n_stops);

  bobgui_snapshot_add_linear_gradient (snapshot, bounds,
                                    start_point, end_point,
                                    gradient);

  gsk_gradient_free (gradient);
}

/*< private >
 * bobgui_snapshot_add_linear_gradient:
 * @snapshot: a `BobguiSnapshot`
 * @bounds: the rectangle to render the linear gradient into
 * @start_point: the point at which the linear gradient will begin
 * @end_point: the point at which the linear gradient will finish
 * @gradient: the gradient specification
 *
 * Appends a linear gradient node with the given stops to @snapshot.
 */
void
bobgui_snapshot_add_linear_gradient (BobguiSnapshot             *snapshot,
                                  const graphene_rect_t   *bounds,
                                  const graphene_point_t  *start_point,
                                  const graphene_point_t  *end_point,
                                  const GskGradient       *gradient)
{
  GskRenderNode *node;
  graphene_rect_t real_bounds;
  float scale_x, scale_y, dx, dy;
  const GdkColor *color;

  g_return_if_fail (snapshot != NULL);
  g_return_if_fail (start_point != NULL);
  g_return_if_fail (end_point != NULL);

  bobgui_snapshot_ensure_affine_with_flags (snapshot,
                                         ENSURE_POSITIVE_SCALE | ENSURE_UNIFORM_SCALE,
                                         &scale_x, &scale_y,
                                         &dx, &dy);
  bobgui_graphene_rect_scale_affine (bounds, scale_x, scale_y, dx, dy, &real_bounds);

  color = gsk_gradient_check_single_color (gradient);
  if (color == NULL)
    {
      graphene_point_t real_start_point, real_end_point;

      real_start_point.x = scale_x * start_point->x + dx;
      real_start_point.y = scale_y * start_point->y + dy;
      real_end_point.x = scale_x * end_point->x + dx;
      real_end_point.y = scale_y * end_point->y + dy;

      node = gsk_linear_gradient_node_new2 (&real_bounds,
                                            &real_start_point,
                                            &real_end_point,
                                            gradient);
    }
  else
    {
      node = gsk_color_node_new2 (color, &real_bounds);
    }

  bobgui_snapshot_append_node_internal (snapshot, node);
}

/**
 * bobgui_snapshot_append_repeating_linear_gradient:
 * @snapshot: a `BobguiSnapshot`
 * @bounds: the rectangle to render the linear gradient into
 * @start_point: the point at which the linear gradient will begin
 * @end_point: the point at which the linear gradient will finish
 * @stops: (array length=n_stops): the color stops defining the gradient
 * @n_stops: the number of elements in @stops
 *
 * Appends a repeating linear gradient node with the given stops to @snapshot.
 */
void
bobgui_snapshot_append_repeating_linear_gradient (BobguiSnapshot            *snapshot,
                                               const graphene_rect_t  *bounds,
                                               const graphene_point_t *start_point,
                                               const graphene_point_t *end_point,
                                               const GskColorStop     *stops,
                                               gsize                   n_stops)
{
  GskGradient *gradient;

  gradient = gsk_gradient_new ();
  gsk_gradient_add_color_stops (gradient, stops, n_stops);
  gsk_gradient_set_repeat (gradient, GSK_REPEAT_REPEAT);

  bobgui_snapshot_add_linear_gradient (snapshot, bounds,
                                    start_point, end_point,
                                    gradient);

  gsk_gradient_free (gradient);
}

/**
 * bobgui_snapshot_append_conic_gradient:
 * @snapshot: a `BobguiSnapshot`
 * @bounds: the rectangle to render the gradient into
 * @center: the center point of the conic gradient
 * @rotation: the clockwise rotation in degrees of the starting angle.
 *   0 means the starting angle is the top.
 * @stops: (array length=n_stops): the color stops defining the gradient
 * @n_stops: the number of elements in @stops
 *
 * Appends a conic gradient node with the given stops to @snapshot.
 */
void
bobgui_snapshot_append_conic_gradient (BobguiSnapshot            *snapshot,
                                    const graphene_rect_t  *bounds,
                                    const graphene_point_t *center,
                                    float                   rotation,
                                    const GskColorStop     *stops,
                                    gsize                   n_stops)
{
  GskGradient *gradient;

  gradient = gsk_gradient_new ();
  gsk_gradient_add_color_stops (gradient, stops, n_stops);

  bobgui_snapshot_add_conic_gradient (snapshot, bounds,
                                   center, rotation,
                                   gradient);

  gsk_gradient_free (gradient);
}

/*< private >
 * bobgui_snapshot_add_conic_gradient:
 * @snapshot: a `BobguiSnapshot`
 * @bounds: the rectangle to render the gradient into
 * @center: the center point of the conic gradient
 * @rotation: the clockwise rotation in degrees of the starting angle.
 *   0 means the starting angle is the top.
 * @gradient: the gradient specification
 *
 * Appends a conic gradient node with the given stops to @snapshot.
 */
void
bobgui_snapshot_add_conic_gradient (BobguiSnapshot            *snapshot,
                                 const graphene_rect_t  *bounds,
                                 const graphene_point_t *center,
                                 float                   rotation,
                                 const GskGradient      *gradient)
{
  GskRenderNode *node;
  graphene_rect_t real_bounds;
  float dx, dy;
  const GdkColor *color;

  g_return_if_fail (snapshot != NULL);
  g_return_if_fail (center != NULL);

  bobgui_snapshot_ensure_translate (snapshot, &dx, &dy);
  graphene_rect_offset_r (bounds, dx, dy, &real_bounds);

  color = gsk_gradient_check_single_color (gradient);
  if (color == NULL)
    node = gsk_conic_gradient_node_new2 (&real_bounds,
                                         &GRAPHENE_POINT_INIT(
                                           center->x + dx,
                                           center->y + dy
                                         ),
                                         rotation,
                                         gradient);
  else
    node = gsk_color_node_new2 (color, &real_bounds);

  bobgui_snapshot_append_node_internal (snapshot, node);
}

/**
 * bobgui_snapshot_append_radial_gradient:
 * @snapshot: a `BobguiSnapshot`
 * @bounds: the rectangle to render the readial gradient into
 * @center: the center point for the radial gradient
 * @hradius: the horizontal radius
 * @vradius: the vertical radius
 * @start: the start position (on the horizontal axis)
 * @end: the end position (on the horizontal axis)
 * @stops: (array length=n_stops): the color stops defining the gradient
 * @n_stops: the number of elements in @stops
 *
 * Appends a radial gradient node with the given stops to @snapshot.
 */
void
bobgui_snapshot_append_radial_gradient (BobguiSnapshot            *snapshot,
                                     const graphene_rect_t  *bounds,
                                     const graphene_point_t *center,
                                     float                   hradius,
                                     float                   vradius,
                                     float                   start,
                                     float                   end,
                                     const GskColorStop     *stops,
                                     gsize                   n_stops)
{
  GskGradient *gradient;

  gradient = gsk_gradient_new ();
  gsk_gradient_add_color_stops (gradient, stops, n_stops);

  bobgui_snapshot_add_radial_gradient (snapshot,
                                    bounds,
                                    center, hradius * start,
                                    center, hradius * end,
                                    hradius / vradius,
                                    gradient);

  gsk_gradient_free (gradient);
}

/*< private>
 * bobgui_snapshot_add_radial_gradient:
 * @snapshot: a `BobguiSnapshot`
 * @bounds: the rectangle to render the readial gradient into
 * @start_center: the center for the start circle
 * @start_radius: the radius for the start circle
 * @end_center: the center for the end circle
 * @end_radius: the radius for the end circle
 * @aspect_ratio: the aspect ratio of the circles
 * @gradient: the gradient specification
 *
 * Appends a radial gradient node with the given stops to @snapshot.
 */
void
bobgui_snapshot_add_radial_gradient (BobguiSnapshot             *snapshot,
                                  const graphene_rect_t   *bounds,
                                  const graphene_point_t  *start_center,
                                  float                    start_radius,
                                  const graphene_point_t  *end_center,
                                  float                    end_radius,
                                  float                    aspect_ratio,
                                  const GskGradient       *gradient)
{
  GskRenderNode *node;
  graphene_rect_t real_bounds;
  float scale_x, scale_y, dx, dy;
  const GdkColor *color;

  g_return_if_fail (snapshot != NULL);
  g_return_if_fail (start_center != NULL);
  g_return_if_fail (start_radius >= 0);
  g_return_if_fail (end_center != NULL);
  g_return_if_fail (end_radius >= 0);
  g_return_if_fail (aspect_ratio > 0);

  bobgui_snapshot_ensure_affine (snapshot, &scale_x, &scale_y, &dx, &dy);
  bobgui_graphene_rect_scale_affine (bounds, scale_x, scale_y, dx, dy, &real_bounds);

  color = gsk_gradient_check_single_color (gradient);
  if (color && gsk_radial_gradient_fills_plane (start_center, start_radius,
                                                end_center, end_radius))
    {
      node = gsk_color_node_new2 (color, &real_bounds);
    }
  else
    {
      graphene_point_t real_start;
      graphene_point_t real_end;

      real_start.x = scale_x * start_center->x + dx;
      real_start.y = scale_y * start_center->y + dy;

      real_end.x = scale_x * end_center->x + dx;
      real_end.y = scale_y * end_center->y + dy;

      node = gsk_radial_gradient_node_new2 (&real_bounds,
                                            &real_start, start_radius * scale_x,
                                            &real_end, end_radius * scale_x,
                                            aspect_ratio * (scale_x / scale_y),
                                            gradient);
    }

  bobgui_snapshot_append_node_internal (snapshot, node);
}

/**
 * bobgui_snapshot_append_repeating_radial_gradient:
 * @snapshot: a `BobguiSnapshot`
 * @bounds: the rectangle to render the readial gradient into
 * @center: the center point for the radial gradient
 * @hradius: the horizontal radius
 * @vradius: the vertical radius
 * @start: the start position (on the horizontal axis)
 * @end: the end position (on the horizontal axis)
 * @stops: (array length=n_stops): the color stops defining the gradient
 * @n_stops: the number of elements in @stops
 *
 * Appends a repeating radial gradient node with the given stops to @snapshot.
 */
void
bobgui_snapshot_append_repeating_radial_gradient (BobguiSnapshot            *snapshot,
                                               const graphene_rect_t  *bounds,
                                               const graphene_point_t *center,
                                               float                   hradius,
                                               float                   vradius,
                                               float                   start,
                                               float                   end,
                                               const GskColorStop     *stops,
                                               gsize                   n_stops)
{
  GskGradient *gradient;

  gradient = gsk_gradient_new ();
  gsk_gradient_add_color_stops (gradient, stops, n_stops);

  bobgui_snapshot_add_radial_gradient (snapshot, bounds,
                                    center, hradius * start,
                                    center, hradius * end,
                                    hradius / vradius,
                                    gradient);

  gsk_gradient_free (gradient);
}

/**
 * bobgui_snapshot_append_border:
 * @snapshot: a `BobguiSnapshot`
 * @outline: the outline of the border
 * @border_width: (array fixed-size=4): the stroke width of the border on
 *   the top, right, bottom and left side respectively.
 * @border_color: (array fixed-size=4): the color used on the top, right,
 *   bottom and left side.
 *
 * Appends a stroked border rectangle inside the given @outline.
 *
 * The four sides of the border can have different widths and colors.
 */
void
bobgui_snapshot_append_border (BobguiSnapshot          *snapshot,
                            const GskRoundedRect *outline,
                            const float           border_width[4],
                            const GdkRGBA         border_color[4])
{
  GdkColor color[4];

  for (int i = 0; i < 4; i++)
    gdk_color_init_from_rgba (&color[i], &border_color[i]);

  bobgui_snapshot_add_border (snapshot, outline, border_width, color);

  for (int i = 0; i < 4; i++)
    gdk_color_finish (&color[i]);
}

/*< private >
 * bobgui_snapshot_add_border:
 * @snapshot: a `BobguiSnapshot`
 * @outline: the outline of the border
 * @border_width: (array fixed-size=4): the stroke width of the border on
 *   the top, right, bottom and left side respectively.
 * @border_color: (array fixed-size=4): the color used on the top, right,
 *   bottom and left side.
 *
 * Appends a stroked border rectangle inside the given @outline.
 *
 * The four sides of the border can have different widths and colors.
 */
void
bobgui_snapshot_add_border (BobguiSnapshot          *snapshot,
                         const GskRoundedRect *outline,
                         const float           border_width[4],
                         const GdkColor        border_color[4])
{
  GskRenderNode *node;
  GskRoundedRect real_outline;
  float scale_x, scale_y, dx, dy;

  g_return_if_fail (snapshot != NULL);
  g_return_if_fail (outline != NULL);
  g_return_if_fail (border_width != NULL);
  g_return_if_fail (border_color != NULL);

  bobgui_snapshot_ensure_affine (snapshot, &scale_x, &scale_y, &dx, &dy);
  gsk_rounded_rect_scale_affine (&real_outline, outline, scale_x, scale_y, dx, dy);

  node = gsk_border_node_new2 (&real_outline,
                               (float[4]) {
                                 border_width[0] * scale_y,
                                 border_width[1] * scale_x,
                                 border_width[2] * scale_y,
                                 border_width[3] * scale_x,
                               },
                               border_color);

  bobgui_snapshot_append_node_internal (snapshot, node);
}

/**
 * bobgui_snapshot_append_inset_shadow:
 * @snapshot: a `BobguiSnapshot`
 * @outline: outline of the region surrounded by shadow
 * @color: color of the shadow
 * @dx: horizontal offset of shadow
 * @dy: vertical offset of shadow
 * @spread: how far the shadow spreads towards the inside
 * @blur_radius: how much blur to apply to the shadow
 *
 * Appends an inset shadow into the box given by @outline.
 */
void
bobgui_snapshot_append_inset_shadow (BobguiSnapshot          *snapshot,
                                  const GskRoundedRect *outline,
                                  const GdkRGBA        *color,
                                  float                 dx,
                                  float                 dy,
                                  float                 spread,
                                  float                 blur_radius)
{
  GdkColor color2;

  gdk_color_init_from_rgba (&color2, color);
  bobgui_snapshot_add_inset_shadow (snapshot,
                                 outline,
                                 &color2,
                                 &GRAPHENE_POINT_INIT (dx, dy),
                                 spread, blur_radius);
  gdk_color_finish (&color2);
}

/*< private >
 * bobgui_snapshot_add_inset_shadow:
 * @snapshot: a `BobguiSnapshot`
 * @outline: outline of the region surrounded by shadow
 * @color: color of the shadow
 * @offset: offset of shadow
 * @spread: how far the shadow spreads towards the inside
 * @blur_radius: how much blur to apply to the shadow
 *
 * Appends an inset shadow into the box given by @outline.
 */
void
bobgui_snapshot_add_inset_shadow (BobguiSnapshot            *snapshot,
                               const GskRoundedRect   *outline,
                               const GdkColor         *color,
                               const graphene_point_t *offset,
                               float                   spread,
                               float                   blur_radius)
{
  GskRenderNode *node;
  GskRoundedRect real_outline;
  float scale_x, scale_y, x, y;

  g_return_if_fail (snapshot != NULL);
  g_return_if_fail (outline != NULL);
  g_return_if_fail (color != NULL);
  g_return_if_fail (offset != NULL);

  bobgui_snapshot_ensure_affine (snapshot, &scale_x, &scale_y, &x, &y);
  gsk_rounded_rect_scale_affine (&real_outline, outline, scale_x, scale_y, x, y);

  node = gsk_inset_shadow_node_new2 (&real_outline,
                                     color,
                                     &GRAPHENE_POINT_INIT (scale_x * offset->x,
                                                           scale_y * offset->y),
                                     spread,
                                     blur_radius);

  bobgui_snapshot_append_node_internal (snapshot, node);
}

/**
 * bobgui_snapshot_append_outset_shadow:
 * @snapshot: a `BobguiSnapshot`
 * @outline: outline of the region surrounded by shadow
 * @color: color of the shadow
 * @dx: horizontal offset of shadow
 * @dy: vertical offset of shadow
 * @spread: how far the shadow spreads towards the outside
 * @blur_radius: how much blur to apply to the shadow
 *
 * Appends an outset shadow node around the box given by @outline.
 */
void
bobgui_snapshot_append_outset_shadow (BobguiSnapshot          *snapshot,
                                   const GskRoundedRect *outline,
                                   const GdkRGBA        *color,
                                   float                 dx,
                                   float                 dy,
                                   float                 spread,
                                   float                 blur_radius)
{
  GdkColor color2;

  gdk_color_init_from_rgba (&color2, color);
  bobgui_snapshot_add_outset_shadow (snapshot,
                                  outline,
                                  &color2,
                                  &GRAPHENE_POINT_INIT (dx, dy),
                                  spread, blur_radius);
  gdk_color_finish (&color2);
}

/*< private >
 * bobgui_snapshot_add_outset_shadow:
 * @snapshot: a `BobguiSnapshot`
 * @outline: outline of the region surrounded by shadow
 * @color: color of the shadow
 * @offset: offset of shadow
 * @spread: how far the shadow spreads towards the outside
 * @blur_radius: how much blur to apply to the shadow
 *
 * Appends an outset shadow node around the box given by @outline.
 */
void
bobgui_snapshot_add_outset_shadow (BobguiSnapshot            *snapshot,
                                const GskRoundedRect   *outline,
                                const GdkColor         *color,
                                const graphene_point_t *offset,
                                float                   spread,
                                float                   blur_radius)
{
  GskRenderNode *node;
  GskRoundedRect real_outline;
  float scale_x, scale_y, x, y;

  g_return_if_fail (snapshot != NULL);
  g_return_if_fail (outline != NULL);
  g_return_if_fail (color != NULL);
  g_return_if_fail (offset != NULL);

  bobgui_snapshot_ensure_affine (snapshot, &scale_x, &scale_y, &x, &y);
  gsk_rounded_rect_scale_affine (&real_outline, outline, scale_x, scale_y, x, y);

  node = gsk_outset_shadow_node_new2 (&real_outline,
                                      color,
                                      &GRAPHENE_POINT_INIT (scale_x * offset->x,
                                                            scale_y * offset->y),
                                      spread,
                                      blur_radius);

  bobgui_snapshot_append_node_internal (snapshot, node);
}

/**
 * bobgui_snapshot_append_paste:
 * @snapshot: a `BobguiSnapshot`
 * @bounds: the bounds for the new node
 * @nth: the index of the copy, with 0 being the latest
 *  copy, 1 being the copy before that, and so on.
 *
 * Creates a new render node that pastes the contents
 * copied by a previous call to [method@Bobgui.Snapshot.push_copy]
 *
 * Since: 4.22
 */
void
bobgui_snapshot_append_paste (BobguiSnapshot           *snapshot,
                           const graphene_rect_t *bounds,
                           gsize                  nth)
{
  GskRenderNode *node;

  g_return_if_fail (snapshot != NULL);
  g_return_if_fail (bounds != NULL);

  /* need identity here because the bounds are used
   * in the copy and the paste coordinate system. */
  bobgui_snapshot_ensure_identity (snapshot);

  node = gsk_paste_node_new (bounds, nth);

  bobgui_snapshot_append_node_internal (snapshot, node);
}

static GskRenderNode *
bobgui_snapshot_collect_subsurface (BobguiSnapshot      *snapshot,
                                 BobguiSnapshotState *state,
                                 GskRenderNode   **nodes,
                                 guint             n_nodes)
{
  GskRenderNode *node, *subsurface_node;

  node = bobgui_snapshot_collect_default (snapshot, state, nodes, n_nodes);
  if (node == NULL)
    return NULL;

  subsurface_node = gsk_subsurface_node_new (node, state->data.subsurface.subsurface);
  gsk_render_node_unref (node);

  return subsurface_node;
}

static void
bobgui_snapshot_clear_subsurface (BobguiSnapshotState *state)
{
  g_object_unref (state->data.subsurface.subsurface);
}

void
bobgui_snapshot_push_subsurface (BobguiSnapshot   *snapshot,
                              GdkSubsurface *subsurface)
{
  const BobguiSnapshotState *current_state = bobgui_snapshot_get_current_state (snapshot);
  BobguiSnapshotState *state;

  state = bobgui_snapshot_push_state (snapshot,
                                   current_state->transform,
                                   bobgui_snapshot_collect_subsurface,
                                   bobgui_snapshot_clear_subsurface);

  state->data.subsurface.subsurface = g_object_ref (subsurface);
}
