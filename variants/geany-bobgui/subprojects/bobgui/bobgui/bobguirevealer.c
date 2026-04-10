/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * Copyright 2013, 2015 Red Hat, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Author: Alexander Larsson <alexl@redhat.com>
 *         Carlos Soriano <csoriano@gnome.org>
 */

#include "config.h"

#include "bobguirevealer.h"

#include "bobguiprivate.h"
#include "bobguiprogresstrackerprivate.h"
#include "bobguisettingsprivate.h"
#include "bobguitypebuiltins.h"
#include "bobguiwidgetprivate.h"
#include "bobguibuildable.h"
#include "bobguibuilderprivate.h"
#include "bobguisnapshotprivate.h"

/**
 * BobguiRevealer:
 *
 * Animates the transition of its child from invisible to visible.
 *
 * The style of transition can be controlled with
 * [method@Bobgui.Revealer.set_transition_type].
 *
 * These animations respect the [property@Bobgui.Settings:bobgui-enable-animations]
 * setting.
 *
 * # CSS nodes
 *
 * `BobguiRevealer` has a single CSS node with name revealer.
 * When styling `BobguiRevealer` using CSS, remember that it only hides its contents,
 * not itself. That means applied margin, padding and borders will be visible even
 * when the [property@Bobgui.Revealer:reveal-child] property is set to %FALSE.
 *
 * # Accessibility
 *
 * `BobguiRevealer` uses the [enum@Bobgui.AccessibleRole.group] role.
 *
 * The child of `BobguiRevealer`, if set, is always available in the accessibility
 * tree, regardless of the state of the revealer widget.
 */

/**
 * BobguiRevealerTransitionType:
 * @BOBGUI_REVEALER_TRANSITION_TYPE_NONE: No transition
 * @BOBGUI_REVEALER_TRANSITION_TYPE_CROSSFADE: Fade in
 * @BOBGUI_REVEALER_TRANSITION_TYPE_SLIDE_RIGHT: Slide in from the left
 * @BOBGUI_REVEALER_TRANSITION_TYPE_SLIDE_LEFT: Slide in from the right
 * @BOBGUI_REVEALER_TRANSITION_TYPE_SLIDE_UP: Slide in from the bottom
 * @BOBGUI_REVEALER_TRANSITION_TYPE_SLIDE_DOWN: Slide in from the top
 * @BOBGUI_REVEALER_TRANSITION_TYPE_SWING_RIGHT: Floop in from the left
 * @BOBGUI_REVEALER_TRANSITION_TYPE_SWING_LEFT: Floop in from the right
 * @BOBGUI_REVEALER_TRANSITION_TYPE_SWING_UP: Floop in from the bottom
 * @BOBGUI_REVEALER_TRANSITION_TYPE_SWING_DOWN: Floop in from the top
 *
 * These enumeration values describe the possible transitions
 * when the child of a `BobguiRevealer` widget is shown or hidden.
 */

struct _BobguiRevealer
{
  BobguiWidget parent_instance;

  BobguiWidget *child;

  BobguiRevealerTransitionType transition_type;
  guint transition_duration;

  double current_pos;
  double source_pos;
  double target_pos;

  guint tick_id;
  BobguiProgressTracker tracker;
};

typedef struct
{
  BobguiWidgetClass parent_class;
} BobguiRevealerClass;

enum  {
  PROP_0,
  PROP_TRANSITION_TYPE,
  PROP_TRANSITION_DURATION,
  PROP_REVEAL_CHILD,
  PROP_CHILD_REVEALED,
  PROP_CHILD,
  LAST_PROP
};

static GParamSpec *props[LAST_PROP] = { NULL, };

static void bobgui_revealer_buildable_iface_init (BobguiBuildableIface *iface);

G_DEFINE_TYPE_WITH_CODE (BobguiRevealer, bobgui_revealer, BOBGUI_TYPE_WIDGET,
                         G_IMPLEMENT_INTERFACE (BOBGUI_TYPE_BUILDABLE,
                                                bobgui_revealer_buildable_iface_init))

static BobguiBuildableIface *parent_buildable_iface;

static BobguiRevealerTransitionType
effective_transition (BobguiRevealer *revealer)
{
  if (bobgui_widget_get_direction (BOBGUI_WIDGET (revealer)) == BOBGUI_TEXT_DIR_RTL)
    {
      if (revealer->transition_type == BOBGUI_REVEALER_TRANSITION_TYPE_SLIDE_LEFT)
        return BOBGUI_REVEALER_TRANSITION_TYPE_SLIDE_RIGHT;
      else if (revealer->transition_type == BOBGUI_REVEALER_TRANSITION_TYPE_SLIDE_RIGHT)
        return BOBGUI_REVEALER_TRANSITION_TYPE_SLIDE_LEFT;
      if (revealer->transition_type == BOBGUI_REVEALER_TRANSITION_TYPE_SWING_LEFT)
        return BOBGUI_REVEALER_TRANSITION_TYPE_SWING_RIGHT;
      else if (revealer->transition_type == BOBGUI_REVEALER_TRANSITION_TYPE_SWING_RIGHT)
        return BOBGUI_REVEALER_TRANSITION_TYPE_SWING_LEFT;
      else if (revealer->transition_type == BOBGUI_REVEALER_TRANSITION_TYPE_FADE_SLIDE_LEFT)
        return BOBGUI_REVEALER_TRANSITION_TYPE_FADE_SLIDE_RIGHT;
      else if (revealer->transition_type == BOBGUI_REVEALER_TRANSITION_TYPE_FADE_SLIDE_RIGHT)
        return BOBGUI_REVEALER_TRANSITION_TYPE_FADE_SLIDE_LEFT;
    }

  return revealer->transition_type;
}

static double
get_child_size_scale (BobguiRevealer    *revealer,
                      BobguiOrientation  orientation)
{
  switch (effective_transition (revealer))
    {
    case BOBGUI_REVEALER_TRANSITION_TYPE_SLIDE_RIGHT:
    case BOBGUI_REVEALER_TRANSITION_TYPE_SLIDE_LEFT:
    case BOBGUI_REVEALER_TRANSITION_TYPE_FADE_SLIDE_RIGHT:
    case BOBGUI_REVEALER_TRANSITION_TYPE_FADE_SLIDE_LEFT:
      if (orientation == BOBGUI_ORIENTATION_HORIZONTAL)
        return revealer->current_pos;
      else
        return 1.0;

    case BOBGUI_REVEALER_TRANSITION_TYPE_SLIDE_DOWN:
    case BOBGUI_REVEALER_TRANSITION_TYPE_SLIDE_UP:
    case BOBGUI_REVEALER_TRANSITION_TYPE_FADE_SLIDE_DOWN:
    case BOBGUI_REVEALER_TRANSITION_TYPE_FADE_SLIDE_UP:
      if (orientation == BOBGUI_ORIENTATION_VERTICAL)
        return revealer->current_pos;
      else
        return 1.0;

    case BOBGUI_REVEALER_TRANSITION_TYPE_SWING_RIGHT:
    case BOBGUI_REVEALER_TRANSITION_TYPE_SWING_LEFT:
      if (orientation == BOBGUI_ORIENTATION_HORIZONTAL)
        return sin (G_PI * revealer->current_pos / 2);
      else
        return 1.0;

    case BOBGUI_REVEALER_TRANSITION_TYPE_SWING_DOWN:
    case BOBGUI_REVEALER_TRANSITION_TYPE_SWING_UP:
      if (orientation == BOBGUI_ORIENTATION_VERTICAL)
        return sin (G_PI * revealer->current_pos / 2);
      else
        return 1.0;

    case BOBGUI_REVEALER_TRANSITION_TYPE_NONE:
    case BOBGUI_REVEALER_TRANSITION_TYPE_CROSSFADE:
    default:
      return 1.0;
    }
}

static gboolean
get_is_fading_type (BobguiRevealerTransitionType transition_type)
{
  switch (transition_type)
    {
    case BOBGUI_REVEALER_TRANSITION_TYPE_CROSSFADE:
    case BOBGUI_REVEALER_TRANSITION_TYPE_FADE_SLIDE_RIGHT:
    case BOBGUI_REVEALER_TRANSITION_TYPE_FADE_SLIDE_LEFT:
    case BOBGUI_REVEALER_TRANSITION_TYPE_FADE_SLIDE_UP:
    case BOBGUI_REVEALER_TRANSITION_TYPE_FADE_SLIDE_DOWN:
      return TRUE;

    case BOBGUI_REVEALER_TRANSITION_TYPE_NONE:
    case BOBGUI_REVEALER_TRANSITION_TYPE_SLIDE_RIGHT:
    case BOBGUI_REVEALER_TRANSITION_TYPE_SLIDE_LEFT:
    case BOBGUI_REVEALER_TRANSITION_TYPE_SLIDE_UP:
    case BOBGUI_REVEALER_TRANSITION_TYPE_SLIDE_DOWN:
    case BOBGUI_REVEALER_TRANSITION_TYPE_SWING_RIGHT:
    case BOBGUI_REVEALER_TRANSITION_TYPE_SWING_LEFT:
    case BOBGUI_REVEALER_TRANSITION_TYPE_SWING_UP:
    case BOBGUI_REVEALER_TRANSITION_TYPE_SWING_DOWN:
    default:
      return FALSE;
    }
}

static void
bobgui_revealer_set_position (BobguiRevealer *revealer,
                           double       pos)
{
  gboolean new_visible;
  BobguiRevealerTransitionType transition;

  revealer->current_pos = pos;

  new_visible = revealer->current_pos != 0.0;

  if (revealer->child != NULL &&
      new_visible != bobgui_widget_get_child_visible (revealer->child))
    {
      bobgui_widget_set_child_visible (revealer->child, new_visible);
      bobgui_widget_queue_resize (BOBGUI_WIDGET (revealer));
    }

  transition = effective_transition (revealer);
  if (transition == BOBGUI_REVEALER_TRANSITION_TYPE_NONE ||
      transition == BOBGUI_REVEALER_TRANSITION_TYPE_CROSSFADE)
    bobgui_widget_queue_draw (BOBGUI_WIDGET (revealer));
  else
    bobgui_widget_queue_resize (BOBGUI_WIDGET (revealer));

  if (revealer->current_pos == revealer->target_pos)
    {
      if (transition == BOBGUI_REVEALER_TRANSITION_TYPE_SLIDE_UP ||
          transition == BOBGUI_REVEALER_TRANSITION_TYPE_SLIDE_DOWN ||
          transition == BOBGUI_REVEALER_TRANSITION_TYPE_SLIDE_LEFT ||
          transition == BOBGUI_REVEALER_TRANSITION_TYPE_SLIDE_RIGHT)
        bobgui_widget_set_overflow (BOBGUI_WIDGET (revealer), BOBGUI_OVERFLOW_VISIBLE);

      g_object_notify_by_pspec (G_OBJECT (revealer), props[PROP_CHILD_REVEALED]);
    }
}

static gboolean
bobgui_revealer_animate_cb (BobguiWidget     *widget,
                         GdkFrameClock *frame_clock,
                         gpointer       user_data)
{
  BobguiRevealer *revealer = BOBGUI_REVEALER (widget);
  double ease;

  bobgui_progress_tracker_advance_frame (&revealer->tracker,
                                      gdk_frame_clock_get_frame_time (frame_clock));
  ease = bobgui_progress_tracker_get_ease_out_cubic (&revealer->tracker, FALSE);
  bobgui_revealer_set_position (revealer,
                             revealer->source_pos + (ease * (revealer->target_pos - revealer->source_pos)));

  if (bobgui_progress_tracker_get_state (&revealer->tracker) == BOBGUI_PROGRESS_STATE_AFTER)
    {
      revealer->tick_id = 0;
      return FALSE;
    }

  return TRUE;
}

static void
bobgui_revealer_start_animation (BobguiRevealer *revealer,
                              double       target)
{
  BobguiWidget *widget = BOBGUI_WIDGET (revealer);
  BobguiRevealerTransitionType transition;

  if (revealer->target_pos == target)
    return;

  revealer->target_pos = target;
  g_object_notify_by_pspec (G_OBJECT (revealer), props[PROP_REVEAL_CHILD]);

  transition = effective_transition (revealer);
  if (bobgui_widget_get_mapped (widget) &&
      revealer->transition_duration != 0 &&
      transition != BOBGUI_REVEALER_TRANSITION_TYPE_NONE &&
      bobgui_settings_get_enable_animations (bobgui_widget_get_settings (widget)))
    {
      if (transition == BOBGUI_REVEALER_TRANSITION_TYPE_SLIDE_UP ||
          transition == BOBGUI_REVEALER_TRANSITION_TYPE_SLIDE_DOWN ||
          transition == BOBGUI_REVEALER_TRANSITION_TYPE_SLIDE_LEFT ||
          transition == BOBGUI_REVEALER_TRANSITION_TYPE_SLIDE_RIGHT)
        bobgui_widget_set_overflow (BOBGUI_WIDGET (revealer), BOBGUI_OVERFLOW_HIDDEN);

      revealer->source_pos = revealer->current_pos;
      if (revealer->tick_id == 0)
        revealer->tick_id =
          bobgui_widget_add_tick_callback (widget, bobgui_revealer_animate_cb, revealer, NULL);
      bobgui_progress_tracker_start (&revealer->tracker,
                                  revealer->transition_duration * 1000,
                                  0,
                                  1.0);
    }
  else
    {
      bobgui_revealer_set_position (revealer, target);
    }
}

static void
bobgui_revealer_dispose (GObject *obj)
{
  BobguiRevealer *revealer = BOBGUI_REVEALER (obj);

  g_clear_pointer (&revealer->child, bobgui_widget_unparent);

  G_OBJECT_CLASS (bobgui_revealer_parent_class)->dispose (obj);
}

static void
bobgui_revealer_finalize (GObject *obj)
{
  BobguiRevealer *revealer = BOBGUI_REVEALER (obj);

  if (revealer->tick_id != 0)
    bobgui_widget_remove_tick_callback (BOBGUI_WIDGET (revealer), revealer->tick_id);
  revealer->tick_id = 0;

  G_OBJECT_CLASS (bobgui_revealer_parent_class)->finalize (obj);
}

static void
bobgui_revealer_get_property (GObject    *object,
                           guint       property_id,
                           GValue     *value,
                           GParamSpec *pspec)
{
  BobguiRevealer *revealer = BOBGUI_REVEALER (object);

  switch (property_id)
   {
    case PROP_TRANSITION_TYPE:
      g_value_set_enum (value, bobgui_revealer_get_transition_type (revealer));
      break;
    case PROP_TRANSITION_DURATION:
      g_value_set_uint (value, bobgui_revealer_get_transition_duration (revealer));
      break;
    case PROP_REVEAL_CHILD:
      g_value_set_boolean (value, bobgui_revealer_get_reveal_child (revealer));
      break;
    case PROP_CHILD_REVEALED:
      g_value_set_boolean (value, bobgui_revealer_get_child_revealed (revealer));
      break;
    case PROP_CHILD:
      g_value_set_object (value, bobgui_revealer_get_child (revealer));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
bobgui_revealer_set_property (GObject      *object,
                           guint         property_id,
                           const GValue *value,
                           GParamSpec   *pspec)
{
  BobguiRevealer *revealer = BOBGUI_REVEALER (object);

  switch (property_id)
    {
    case PROP_TRANSITION_TYPE:
      bobgui_revealer_set_transition_type (revealer, g_value_get_enum (value));
      break;
    case PROP_TRANSITION_DURATION:
      bobgui_revealer_set_transition_duration (revealer, g_value_get_uint (value));
      break;
    case PROP_REVEAL_CHILD:
      bobgui_revealer_set_reveal_child (revealer, g_value_get_boolean (value));
      break;
    case PROP_CHILD:
      bobgui_revealer_set_child (revealer, g_value_get_object (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
bobgui_revealer_unmap (BobguiWidget *widget)
{
  BobguiRevealer *revealer = BOBGUI_REVEALER (widget);

  BOBGUI_WIDGET_CLASS (bobgui_revealer_parent_class)->unmap (widget);

  /* Finish & stop the animation */
  if (revealer->current_pos != revealer->target_pos)
    bobgui_revealer_set_position (revealer, revealer->target_pos);

  if (revealer->tick_id != 0)
    {
      bobgui_widget_remove_tick_callback (BOBGUI_WIDGET (revealer), revealer->tick_id);
      revealer->tick_id = 0;
    }
}

static void
bobgui_revealer_compute_expand (BobguiWidget *widget,
                             gboolean  *hexpand,
                             gboolean  *vexpand)
{
  BobguiRevealer *revealer = BOBGUI_REVEALER (widget);

  if (revealer->child)
    {
      *hexpand = bobgui_widget_compute_expand (revealer->child, BOBGUI_ORIENTATION_HORIZONTAL);
      *vexpand = bobgui_widget_compute_expand (revealer->child, BOBGUI_ORIENTATION_VERTICAL);
    }
  else
    {
      *hexpand = FALSE;
      *vexpand = FALSE;
    }
}

static BobguiSizeRequestMode
bobgui_revealer_get_request_mode (BobguiWidget *widget)
{
  BobguiRevealer *revealer = BOBGUI_REVEALER (widget);

  if (revealer->child)
    return bobgui_widget_get_request_mode (revealer->child);
  else
    return BOBGUI_SIZE_REQUEST_CONSTANT_SIZE;
}

static void
bobgui_revealer_size_allocate (BobguiWidget *widget,
                            int        width,
                            int        height,
                            int        baseline)
{
  BobguiRevealer *revealer = BOBGUI_REVEALER (widget);
  GskTransform *transform;
  double hscale, vscale;
  int child_width, child_height;

  if (revealer->child == NULL || !bobgui_widget_get_visible (revealer->child))
    return;

  if (revealer->current_pos >= 1.0)
    {
      bobgui_widget_allocate (revealer->child, width, height, baseline, NULL);
      return;
    }

  hscale = get_child_size_scale (revealer, BOBGUI_ORIENTATION_HORIZONTAL);
  vscale = get_child_size_scale (revealer, BOBGUI_ORIENTATION_VERTICAL);
  if (hscale <= 0 || vscale <= 0)
    {
      /* don't allocate anything, the child is invisible and the numbers
       * don't make sense. */
      return;
    }

  /* We request a different size than the child requested scaled by
   * this scale as it will render smaller from the transition.
   * However, we still want to allocate the child widget with its
   * unscaled size so it renders right instead of e.g. ellipsizing or
   * some other form of clipping. We do this by reverse-applying
   * the scale when size allocating the child.
   *
   * Unfortunately this causes precision issues.
   *
   * So we assume that the fully expanded revealer will likely get
   * an allocation that matches the child's minimum or natural allocation,
   * so we special-case these two values.
   * So when - due to the precision loss - multiple sizes would match
   * the current allocation, we don't pick one at random, we prefer the
   * min and nat size.
   *
   * On top, the scaled size request is always rounded up to an integer.
   * For instance if natural with is 100, and scale is 0.001, we would
   * request a natural size of ceil(0.1) == 1, but reversing this would
   * result in 1 / 0.001 == 1000 (rather than 100).
   * In the swing case we can get the scale arbitrarily near 0 causing
   * arbitrary large problems.
   * These also get avoided by the preference.
   */

  if (hscale < 1.0)
    {
      int min, nat;
      g_assert (vscale == 1.0);
      bobgui_widget_measure (revealer->child, BOBGUI_ORIENTATION_HORIZONTAL, height, &min, &nat, NULL, NULL);
      if (ceil (nat * hscale) == width)
        child_width = nat;
      else if (ceil (min * hscale) == width)
        child_width = min;
      else
        {
          double d = floor (width / hscale);
          child_width = MIN (d, G_MAXINT);
        }
      child_height = height;
    }
  else if (vscale < 1.0)
    {
      int min, nat;
      child_width = width;
      bobgui_widget_measure (revealer->child, BOBGUI_ORIENTATION_VERTICAL, width, &min, &nat, NULL, NULL);
      if (ceil (nat * vscale) == height)
        child_height = nat;
      else if (ceil (min * vscale) == height)
        child_height = min;
      else
        {
          double d = floor (height / vscale);
          child_height = MIN (d, G_MAXINT);
        }
    }
  else
    {
      child_width = width;
      child_height = height;
    }

  transform = NULL;
  switch (effective_transition (revealer))
    {
    case BOBGUI_REVEALER_TRANSITION_TYPE_SLIDE_RIGHT:
    case BOBGUI_REVEALER_TRANSITION_TYPE_FADE_SLIDE_RIGHT:
      transform = gsk_transform_translate (transform, &GRAPHENE_POINT_INIT (width - child_width, 0));
      break;

    case BOBGUI_REVEALER_TRANSITION_TYPE_SLIDE_DOWN:
    case BOBGUI_REVEALER_TRANSITION_TYPE_FADE_SLIDE_DOWN:
      transform = gsk_transform_translate (transform, &GRAPHENE_POINT_INIT (0, height - child_height));
      break;

    case BOBGUI_REVEALER_TRANSITION_TYPE_SWING_LEFT:
      transform = gsk_transform_translate (transform, &GRAPHENE_POINT_INIT (width, height / 2));
      transform = gsk_transform_perspective (transform, 2 * MAX (width, height));
      transform = gsk_transform_rotate_3d (transform, -90 * (1.0 - revealer->current_pos), graphene_vec3_y_axis ());
      transform = gsk_transform_translate (transform, &GRAPHENE_POINT_INIT (- child_width, - child_height / 2));
      break;

    case BOBGUI_REVEALER_TRANSITION_TYPE_SWING_RIGHT:
      transform = gsk_transform_translate (transform, &GRAPHENE_POINT_INIT (0, height / 2));
      transform = gsk_transform_perspective (transform, 2 * MAX (width, height));
      transform = gsk_transform_rotate_3d (transform, 90 * (1.0 - revealer->current_pos), graphene_vec3_y_axis ());
      transform = gsk_transform_translate (transform, &GRAPHENE_POINT_INIT (0, - child_height / 2));
      break;

    case BOBGUI_REVEALER_TRANSITION_TYPE_SWING_DOWN:
      transform = gsk_transform_translate (transform, &GRAPHENE_POINT_INIT (width / 2, 0));
      transform = gsk_transform_perspective (transform, 2 * MAX (width, height));
      transform = gsk_transform_rotate_3d (transform, -90 * (1.0 - revealer->current_pos), graphene_vec3_x_axis ());
      transform = gsk_transform_translate (transform, &GRAPHENE_POINT_INIT (- child_width / 2, 0));
      break;

    case BOBGUI_REVEALER_TRANSITION_TYPE_SWING_UP:
      transform = gsk_transform_translate (transform, &GRAPHENE_POINT_INIT (width / 2, height));
      transform = gsk_transform_perspective (transform, 2 * MAX (width, height));
      transform = gsk_transform_rotate_3d (transform, 90 * (1.0 - revealer->current_pos), graphene_vec3_x_axis ());
      transform = gsk_transform_translate (transform, &GRAPHENE_POINT_INIT (- child_width / 2, - child_height));
      break;

    case BOBGUI_REVEALER_TRANSITION_TYPE_NONE:
    case BOBGUI_REVEALER_TRANSITION_TYPE_CROSSFADE:
    case BOBGUI_REVEALER_TRANSITION_TYPE_SLIDE_LEFT:
    case BOBGUI_REVEALER_TRANSITION_TYPE_SLIDE_UP:
    case BOBGUI_REVEALER_TRANSITION_TYPE_FADE_SLIDE_LEFT:
    case BOBGUI_REVEALER_TRANSITION_TYPE_FADE_SLIDE_UP:
    default:
      break;
    }

  bobgui_widget_allocate (revealer->child, child_width, child_height, -1, transform);
}

static void
bobgui_revealer_measure (BobguiWidget      *widget,
                      BobguiOrientation  orientation,
                      int             for_size,
                      int            *minimum,
                      int            *natural,
                      int            *minimum_baseline,
                      int            *natural_baseline)
{
  BobguiRevealer *revealer = BOBGUI_REVEALER (widget);
  double scale, opposite_scale;

  if (revealer->child == NULL || !_bobgui_widget_get_visible (revealer->child))
    {
      *minimum = *natural = 0;
      return;
    }

  scale = get_child_size_scale (revealer, orientation);
  opposite_scale = get_child_size_scale (revealer, OPPOSITE_ORIENTATION (orientation));
  if (scale == 0)
    {
      *minimum = *natural = 0;
      return;
    }
  else if (opposite_scale == 0)
    {
      for_size = -1;
    }
  else if (for_size >= 0)
    {
      double d = floor (for_size / opposite_scale);
      for_size = MIN (G_MAXINT, d);
    }

  bobgui_widget_measure (revealer->child,
                      orientation,
                      for_size,
                      minimum, natural,
                      NULL, NULL);

  *minimum = ceil (*minimum * scale);
  *natural = ceil (*natural * scale);
}

static void
bobgui_revealer_snapshot (BobguiWidget   *widget,
                       BobguiSnapshot *snapshot)
{
  BobguiRevealer *revealer = BOBGUI_REVEALER (widget);
  gboolean animation_running = (revealer->target_pos != revealer->current_pos);
  BobguiWidget *child;
  gboolean is_fade;

  if (animation_running)
    is_fade = get_is_fading_type (revealer->transition_type);
  else
    is_fade = FALSE;

  if (is_fade)
    bobgui_snapshot_push_opacity (snapshot, revealer->current_pos);

  for (child = _bobgui_widget_get_first_child (widget);
       child != NULL;
       child = _bobgui_widget_get_next_sibling (child))
    bobgui_widget_snapshot_child (widget, child, snapshot);

  if (is_fade)
    bobgui_snapshot_pop (snapshot);
}

static void
bobgui_revealer_buildable_add_child (BobguiBuildable *buildable,
                                  BobguiBuilder   *builder,
                                  GObject      *child,
                                  const char   *type)
{
  if (BOBGUI_IS_WIDGET (child))
    {
      bobgui_buildable_child_deprecation_warning (buildable, builder, NULL, "child");
      bobgui_revealer_set_child (BOBGUI_REVEALER (buildable), BOBGUI_WIDGET (child));
    }
  else
    {
      parent_buildable_iface->add_child (buildable, builder, child, type);
    }
}

static void
bobgui_revealer_class_init (BobguiRevealerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS(klass);

  object_class->dispose = bobgui_revealer_dispose;
  object_class->finalize = bobgui_revealer_finalize;
  object_class->get_property = bobgui_revealer_get_property;
  object_class->set_property = bobgui_revealer_set_property;

  widget_class->unmap = bobgui_revealer_unmap;
  widget_class->size_allocate = bobgui_revealer_size_allocate;
  widget_class->measure = bobgui_revealer_measure;
  widget_class->compute_expand = bobgui_revealer_compute_expand;
  widget_class->get_request_mode = bobgui_revealer_get_request_mode;
  widget_class->snapshot = bobgui_revealer_snapshot;

  /**
   * BobguiRevealer:transition-type:
   *
   * The type of animation used to transition.
   */
  props[PROP_TRANSITION_TYPE] =
    g_param_spec_enum ("transition-type", NULL, NULL,
                       BOBGUI_TYPE_REVEALER_TRANSITION_TYPE,
                       BOBGUI_REVEALER_TRANSITION_TYPE_SLIDE_DOWN,
                       BOBGUI_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiRevealer:transition-duration:
   *
   * The animation duration, in milliseconds.
   */
  props[PROP_TRANSITION_DURATION] =
    g_param_spec_uint ("transition-duration", NULL, NULL,
                       0, G_MAXUINT, 250,
                       BOBGUI_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiRevealer:reveal-child:
   *
   * Whether the revealer should reveal the child.
   */
  props[PROP_REVEAL_CHILD] =
    g_param_spec_boolean ("reveal-child", NULL, NULL,
                          FALSE,
                          BOBGUI_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiRevealer:child-revealed:
   *
   * Whether the child is revealed and the animation target reached.
   */
  props[PROP_CHILD_REVEALED] =
    g_param_spec_boolean ("child-revealed", NULL, NULL,
                          FALSE,
                          BOBGUI_PARAM_READABLE);

  /**
   * BobguiRevealer:child:
   *
   * The child widget.
   */
  props[PROP_CHILD] =
    g_param_spec_object ("child", NULL, NULL,
                         BOBGUI_TYPE_WIDGET,
                         BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);


  g_object_class_install_properties (object_class, LAST_PROP, props);

  bobgui_widget_class_set_css_name (widget_class, I_("revealer"));
  bobgui_widget_class_set_accessible_role (widget_class, BOBGUI_ACCESSIBLE_ROLE_GROUP);
}

static void
bobgui_revealer_buildable_iface_init (BobguiBuildableIface *iface)
{
  parent_buildable_iface = g_type_interface_peek_parent (iface);

  iface->add_child = bobgui_revealer_buildable_add_child;
}

static void
bobgui_revealer_init (BobguiRevealer *revealer)
{
  revealer->transition_type = BOBGUI_REVEALER_TRANSITION_TYPE_SLIDE_DOWN;
  revealer->transition_duration = 250;
  revealer->current_pos = 0.0;
  revealer->target_pos = 0.0;
}

/**
 * bobgui_revealer_new:
 *
 * Creates a new `BobguiRevealer`.
 *
 * Returns: a newly created `BobguiRevealer`
 */
BobguiWidget *
bobgui_revealer_new (void)
{
  return g_object_new (BOBGUI_TYPE_REVEALER, NULL);
}

/**
 * bobgui_revealer_set_reveal_child:
 * @revealer: a `BobguiRevealer`
 * @reveal_child: %TRUE to reveal the child
 *
 * Tells the `BobguiRevealer` to reveal or conceal its child.
 *
 * The transition will be animated with the current
 * transition type of @revealer.
 */
void
bobgui_revealer_set_reveal_child (BobguiRevealer *revealer,
                               gboolean     reveal_child)
{
  g_return_if_fail (BOBGUI_IS_REVEALER (revealer));

  if (reveal_child)
    bobgui_revealer_start_animation (revealer, 1.0);
  else
    bobgui_revealer_start_animation (revealer, 0.0);
}

/**
 * bobgui_revealer_get_reveal_child:
 * @revealer: a `BobguiRevealer`
 *
 * Returns whether the child is currently revealed.
 *
 * This function returns %TRUE as soon as the transition
 * is to the revealed state is started. To learn whether
 * the child is fully revealed (ie the transition is completed),
 * use [method@Bobgui.Revealer.get_child_revealed].
 *
 * Returns: %TRUE if the child is revealed.
 */
gboolean
bobgui_revealer_get_reveal_child (BobguiRevealer *revealer)
{
  g_return_val_if_fail (BOBGUI_IS_REVEALER (revealer), FALSE);

  return revealer->target_pos != 0.0;
}

/**
 * bobgui_revealer_get_child_revealed:
 * @revealer: a `BobguiRevealer`
 *
 * Returns whether the child is fully revealed.
 *
 * In other words, this returns whether the transition
 * to the revealed state is completed.
 *
 * Returns: %TRUE if the child is fully revealed
 */
gboolean
bobgui_revealer_get_child_revealed (BobguiRevealer *revealer)
{
  gboolean animation_finished = (revealer->target_pos == revealer->current_pos);
  gboolean reveal_child = bobgui_revealer_get_reveal_child (revealer);

  if (animation_finished)
    return reveal_child;
  else
    return !reveal_child;
}

/**
 * bobgui_revealer_get_transition_duration:
 * @revealer: a `BobguiRevealer`
 *
 * Returns the amount of time (in milliseconds) that
 * transitions will take.
 *
 * Returns: the transition duration
 */
guint
bobgui_revealer_get_transition_duration (BobguiRevealer *revealer)
{
  g_return_val_if_fail (BOBGUI_IS_REVEALER (revealer), 0);

  return revealer->transition_duration;
}

/**
 * bobgui_revealer_set_transition_duration:
 * @revealer: a `BobguiRevealer`
 * @duration: the new duration, in milliseconds
 *
 * Sets the duration that transitions will take.
 */
void
bobgui_revealer_set_transition_duration (BobguiRevealer *revealer,
                                      guint        value)
{
  g_return_if_fail (BOBGUI_IS_REVEALER (revealer));

  if (revealer->transition_duration == value)
    return;

  revealer->transition_duration = value;
  g_object_notify_by_pspec (G_OBJECT (revealer), props[PROP_TRANSITION_DURATION]);
}

/**
 * bobgui_revealer_get_transition_type:
 * @revealer: a `BobguiRevealer`
 *
 * Gets the type of animation that will be used
 * for transitions in @revealer.
 *
 * Returns: the current transition type of @revealer
 */
BobguiRevealerTransitionType
bobgui_revealer_get_transition_type (BobguiRevealer *revealer)
{
  g_return_val_if_fail (BOBGUI_IS_REVEALER (revealer), BOBGUI_REVEALER_TRANSITION_TYPE_NONE);

  return revealer->transition_type;
}

/**
 * bobgui_revealer_set_transition_type:
 * @revealer: a `BobguiRevealer`
 * @transition: the new transition type
 *
 * Sets the type of animation that will be used for
 * transitions in @revealer.
 *
 * Available types include various kinds of fades and slides.
 */
void
bobgui_revealer_set_transition_type (BobguiRevealer               *revealer,
                                  BobguiRevealerTransitionType  transition)
{
  g_return_if_fail (BOBGUI_IS_REVEALER (revealer));

  if (revealer->transition_type == transition)
    return;

  revealer->transition_type = transition;
  bobgui_widget_queue_resize (BOBGUI_WIDGET (revealer));
  g_object_notify_by_pspec (G_OBJECT (revealer), props[PROP_TRANSITION_TYPE]);
}

/**
 * bobgui_revealer_set_child:
 * @revealer: a `BobguiRevealer`
 * @child: (nullable): the child widget
 *
 * Sets the child widget of @revealer.
 */
void
bobgui_revealer_set_child (BobguiRevealer *revealer,
                        BobguiWidget   *child)
{
  g_return_if_fail (BOBGUI_IS_REVEALER (revealer));
  g_return_if_fail (child == NULL || revealer->child == child || bobgui_widget_get_parent (child) == NULL);

  if (revealer->child == child)
    return;

  g_clear_pointer (&revealer->child, bobgui_widget_unparent);

  if (child)
    {
      bobgui_widget_set_parent (child, BOBGUI_WIDGET (revealer));
      bobgui_widget_set_child_visible (child, revealer->current_pos != 0.0);
      revealer->child = child;
   }

  g_object_notify_by_pspec (G_OBJECT (revealer), props[PROP_CHILD]);
}

/**
 * bobgui_revealer_get_child:
 * @revealer: a `BobguiRevealer`
 *
 * Gets the child widget of @revealer.
 *
 * Returns: (nullable) (transfer none): the child widget of @revealer
 */
BobguiWidget *
bobgui_revealer_get_child (BobguiRevealer *revealer)
{
  g_return_val_if_fail (BOBGUI_IS_REVEALER (revealer), NULL);

  return revealer->child;
}
