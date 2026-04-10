/* BOBGUI - The Bobgui Framework
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
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

/*
 * Modified by the BOBGUI Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the BOBGUI Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * BOBGUI at ftp://ftp.bobgui.org/pub/bobgui/.
 */

#pragma once

#include "bobguiwidget.h"

#include "bobguiactionmuxerprivate.h"
#include "bobguiatcontextprivate.h"
#include "bobguiborder.h"
#include "bobguicsstypesprivate.h"
#include "bobguieventcontrollerprivate.h"
#include "bobguilistlistmodelprivate.h"
#include "bobguirootprivate.h"
#include "bobguisizerequestcacheprivate.h"
#include "bobguiwindowprivate.h"
#include "bobguigesture.h"

#include "gsk/gskrendernodeprivate.h"

G_BEGIN_DECLS

typedef gboolean (*BobguiSurfaceTransformChangedCallback) (BobguiWidget               *widget,
                                                        const graphene_matrix_t *surface_transform,
                                                        gpointer                 user_data);

#define BOBGUI_STATE_FLAGS_BITS 15

typedef struct _BobguiWidgetSurfaceTransformData
{
  BobguiWidget *tracked_parent;
  guint parent_surface_transform_changed_id;

  gboolean cached_surface_transform_valid;

  graphene_matrix_t cached_surface_transform;
  GList *callbacks;
} BobguiWidgetSurfaceTransformData;

struct _BobguiWidgetPrivate
{
  /* The state of the widget. Needs to be able to hold all BobguiStateFlags bits
   * (defined in "bobguienums.h").
   */
  guint state_flags : BOBGUI_STATE_FLAGS_BITS;

  guint direction             : 2;

  guint in_destruction        : 1;
  guint realized              : 1;
  guint mapped                : 1;
  guint visible               : 1;
  guint sensitive             : 1;
  guint can_focus             : 1;
  guint focusable             : 1;
  guint has_focus             : 1;
  guint focus_on_click        : 1;
  guint has_default           : 1;
  guint receives_default      : 1;
  guint has_grab              : 1;
  guint child_visible         : 1;
  guint can_target            : 1;
  guint limit_events          : 1;

  /* Queue-resize related flags */
  guint resize_queued         : 1; /* queue_resize() has been called but no get_preferred_size() yet */
  guint alloc_needed          : 1; /* this widget needs a size_allocate() call */
  guint alloc_needed_on_child : 1; /* 0 or more children - or this widget - need a size_allocate() call */

  /* Queue-draw related flags */
  guint draw_needed           : 1;
  /* Expand-related flags */
  guint need_compute_expand   : 1; /* Need to recompute computed_[hv]_expand */
  guint computed_hexpand      : 1; /* computed results (composite of child flags) */
  guint computed_vexpand      : 1;
  guint hexpand               : 1; /* application-forced expand */
  guint vexpand               : 1;
  guint hexpand_set           : 1; /* whether to use application-forced  */
  guint vexpand_set           : 1; /* instead of computing from children */
  guint has_tooltip           : 1;

  /* SizeGroup related flags */
  guint have_size_groups      : 1;

  /* Alignment */
  guint   halign              : 4;
  guint   valign              : 4;

  guint user_alpha            : 8;

  BobguiOverflow overflow;

#ifdef G_ENABLE_CONSISTENCY_CHECKS
  /* Number of bobgui_widget_push_verify_invariants () */
  guint8 verifying_invariants_count;
#endif

  int width_request;
  int height_request;

  /* Animations and other things to update on clock ticks */
  guint clock_tick_id;
  guint8 n_active;
  GList *tick_callbacks;

  void (* resize_func) (BobguiWidget *);
  BobguiBorder margin;

  /* Surface relative transform updates callbacks */
  BobguiWidgetSurfaceTransformData *surface_transform_data;

  /* The widget's name. If the widget does not have a name
   * (the name is NULL), then its name (as returned by
   * "bobgui_widget_get_name") is its class's name.
   * Among other things, the widget name is used to determine
   * the style to use for a widget.
   */
  char *name;

  /* The root this widget belongs to or %NULL if widget is not
   * rooted or is a BobguiRoot itself.
   */
  BobguiRoot *root;

  /* The style for the widget. The style contains the
   * colors the widget should be drawn in for each state
   * along with graphics contexts used to draw with and
   * the font to use for text.
   */
  BobguiCssNode *cssnode;
  BobguiStyleContext *context;

  /* The widget's allocated size */
  GskTransform *allocated_transform;
  int allocated_width;
  int allocated_height;
  int allocated_baseline;

  int width;
  int height;
  int baseline;
  GskTransform *transform;

  /* The widget's requested sizes */
  SizeRequestCache requests;

  /* The render node we draw or %NULL if not yet created.*/
  GskRenderNode *render_node;

  /* The layout manager, or %NULL */
  BobguiLayoutManager *layout_manager;

  GSList *paintables;

  GList *event_controllers;

  /* Widget tree */
  BobguiWidget *parent;
  BobguiWidget *prev_sibling;
  BobguiWidget *next_sibling;
  BobguiWidget *first_child;
  BobguiWidget *last_child;

  /* only created on-demand */
  BobguiListListModel *children_observer;
  BobguiListListModel *controller_observer;
  BobguiActionMuxer *muxer;

  BobguiWidget *focus_child;

  /* Pointer cursor */
  GdkCursor *cursor;

  /* Tooltip */
  char *tooltip_markup;
  char *tooltip_text;

  /* Accessibility */
  BobguiATContext *at_context;
  BobguiAccessibleRole accessible_role;
};

typedef struct
{
  GBytes *data;
  GSList *children;
  BobguiBuilderScope *scope;
} BobguiWidgetTemplate;

struct _BobguiWidgetClassPrivate
{
  BobguiWidgetTemplate *template;
  GListStore *shortcuts;
  GType layout_manager_type;
  BobguiWidgetAction *actions;
  BobguiAccessibleRole accessible_role;
  guint activate_signal;
  GQuark css_name;
};

void          bobgui_widget_root               (BobguiWidget *widget);
void          bobgui_widget_unroot             (BobguiWidget *widget);
BobguiCssNode *  bobgui_widget_get_css_node       (BobguiWidget *widget);
void         _bobgui_widget_set_visible_flag   (BobguiWidget *widget,
                                             gboolean   visible);
gboolean     _bobgui_widget_get_alloc_needed   (BobguiWidget *widget);
gboolean     bobgui_widget_needs_allocate      (BobguiWidget *widget);
void         bobgui_widget_clear_resize_queued (BobguiWidget *widget);
void         bobgui_widget_ensure_allocate     (BobguiWidget *widget);
void          _bobgui_widget_scale_changed     (BobguiWidget *widget);
void         bobgui_widget_monitor_changed     (BobguiWidget *widget);

GdkSurface * bobgui_widget_get_surface         (BobguiWidget *widget);

void         bobgui_widget_render              (BobguiWidget            *widget,
                                             GdkSurface           *surface,
                                             const cairo_region_t *region);

void         _bobgui_widget_add_sizegroup         (BobguiWidget    *widget,
						gpointer      group);
void         _bobgui_widget_remove_sizegroup      (BobguiWidget    *widget,
						gpointer      group);
GSList      *_bobgui_widget_get_sizegroups        (BobguiWidget    *widget);

void              _bobgui_widget_set_has_default              (BobguiWidget *widget,
                                                            gboolean   has_default);
void              _bobgui_widget_set_has_grab                 (BobguiWidget *widget,
                                                            gboolean   has_grab);

gboolean          bobgui_widget_has_grab                      (BobguiWidget *widget);

void              _bobgui_widget_propagate_display_changed    (BobguiWidget  *widget,
                                                            GdkDisplay *previous_display);

void              _bobgui_widget_set_device_surface           (BobguiWidget *widget,
                                                            GdkDevice *device,
                                                            GdkSurface *pointer_window);
void              _bobgui_widget_synthesize_crossing          (BobguiWidget       *from,
                                                            BobguiWidget       *to,
                                                            GdkDevice       *device,
                                                            GdkCrossingMode  mode);

BobguiStyleContext * _bobgui_widget_peek_style_context           (BobguiWidget *widget);

gboolean          _bobgui_widget_captured_event               (BobguiWidget *widget,
                                                            GdkEvent  *event,
                                                            BobguiWidget *target);

void              bobgui_widget_css_changed                   (BobguiWidget           *widget,
                                                            BobguiCssStyleChange   *change);
void              bobgui_widget_system_setting_changed        (BobguiWidget           *widget,
                                                            BobguiSystemSetting     setting);
void              bobgui_system_setting_changed               (GdkDisplay          *display,
                                                            BobguiSystemSetting     setting);

void              _bobgui_widget_update_parent_muxer          (BobguiWidget    *widget);
BobguiActionMuxer *  _bobgui_widget_get_action_muxer             (BobguiWidget    *widget,
                                                            gboolean      create);

gboolean          bobgui_widget_has_tick_callback             (BobguiWidget *widget);

gboolean          bobgui_widget_has_size_request              (BobguiWidget *widget);

void              bobgui_widget_reset_controllers             (BobguiWidget *widget);

BobguiEventController **bobgui_widget_list_controllers           (BobguiWidget           *widget,
                                                            BobguiPropagationPhase  phase,
                                                            guint               *out_n_controllers);

gboolean          bobgui_widget_query_tooltip                 (BobguiWidget  *widget,
                                                            int         x,
                                                            int         y,
                                                            gboolean    keyboard_mode,
                                                            BobguiTooltip *tooltip);

void              bobgui_widget_snapshot                      (BobguiWidget            *widget,
                                                            BobguiSnapshot          *snapshot);
void              bobgui_widget_adjust_size_request           (BobguiWidget      *widget,
                                                            BobguiOrientation  orientation,
                                                            int            *minimum_size,
                                                            int            *natural_size);
void              bobgui_widget_adjust_baseline_request       (BobguiWidget *widget,
                                                            int       *minimum_baseline,
                                                            int       *natural_baseline);

typedef void    (*BobguiCallback)     (BobguiWidget        *widget,
                                    gpointer          data);

void              bobgui_widget_forall                        (BobguiWidget            *widget,
                                                            BobguiCallback           callback,
                                                            gpointer              user_data);

void              bobgui_widget_focus_sort                    (BobguiWidget        *widget,
                                                            BobguiDirectionType  direction,
                                                            GPtrArray        *focus_order);
gboolean          bobgui_widget_focus_move                    (BobguiWidget        *widget,
                                                            BobguiDirectionType  direction);
void              bobgui_widget_set_has_focus                 (BobguiWidget        *widget,
                                                            gboolean          has_focus);

BobguiWidget *       bobgui_widget_common_ancestor               (BobguiWidget *widget_a,
                                                            BobguiWidget *widget_b);

void              bobgui_widget_set_active_state              (BobguiWidget *widget,
                                                            gboolean   active);

void              bobgui_widget_propagate_event_sequence_state (BobguiWidget             *widget,
                                                             BobguiGesture            *gesture,
                                                             GdkEventSequence      *sequence,
                                                             BobguiEventSequenceState  state);
gboolean          bobgui_widget_event                         (BobguiWidget           *widget,
                                                            GdkEvent            *event,
                                                            BobguiWidget           *target);
gboolean          bobgui_widget_run_controllers               (BobguiWidget           *widget,
                                                            GdkEvent            *event,
                                                            BobguiWidget           *target,
                                                            double               x,
                                                            double               y,
                                                            BobguiPropagationPhase  phase);
void              bobgui_widget_handle_crossing               (BobguiWidget             *widget,
                                                            const BobguiCrossingData *crossing,
                                                            double                 x,
                                                            double                 y);


guint             bobgui_widget_add_surface_transform_changed_callback (BobguiWidget                          *widget,
                                                                     BobguiSurfaceTransformChangedCallback  callback,
                                                                     gpointer                            user_data,
                                                                     GDestroyNotify                      notify);

void              bobgui_widget_remove_surface_transform_changed_callback (BobguiWidget *widget,
                                                                        guint      id);

gboolean          bobgui_widget_can_activate       (BobguiWidget *widget);

/* focus vfuncs for non-focusable containers with focusable children */
gboolean bobgui_widget_grab_focus_child (BobguiWidget        *widget);
gboolean bobgui_widget_focus_child      (BobguiWidget        *widget,
                                      BobguiDirectionType  direction);
/* focus vfuncs for focusable widgets with children that don't receive focus */
gboolean bobgui_widget_grab_focus_self  (BobguiWidget        *widget);
gboolean bobgui_widget_focus_self       (BobguiWidget        *widget,
                                      BobguiDirectionType  direction);

void    bobgui_widget_update_orientation   (BobguiWidget      *widget,
                                         BobguiOrientation  orientation);

void    bobgui_widget_realize_at_context   (BobguiWidget *widget);
void    bobgui_widget_unrealize_at_context (BobguiWidget *widget);

gboolean bobgui_widget_update_pango_context (BobguiWidget        *widget,
                                          PangoContext     *context,
                                          BobguiTextDirection  direction);

/* inline getters */

static inline BobguiWidget *
_bobgui_widget_get_parent (BobguiWidget *widget)
{
  return widget->priv->parent;
}

static inline BobguiWidget *
_bobgui_widget_get_focus_child (BobguiWidget *widget)
{
  return widget->priv->focus_child;
}

static inline gboolean
_bobgui_widget_get_visible (BobguiWidget *widget)
{
  return widget->priv->visible;
}

static inline gboolean
_bobgui_widget_get_child_visible (BobguiWidget *widget)
{
  return widget->priv->child_visible;
}

static inline gboolean
_bobgui_widget_get_mapped (BobguiWidget *widget)
{
  return widget->priv->mapped;
}

static inline gboolean
_bobgui_widget_get_realized (BobguiWidget *widget)
{
  return widget->priv->realized;
}

static inline BobguiStateFlags
_bobgui_widget_get_state_flags (BobguiWidget *widget)
{
  return widget->priv->state_flags;
}

extern BobguiTextDirection bobgui_default_direction;

static inline BobguiTextDirection
_bobgui_widget_get_direction (BobguiWidget *widget)
{
  if (widget->priv->direction == BOBGUI_TEXT_DIR_NONE)
    return bobgui_default_direction;
  else
    return widget->priv->direction;
}

static inline BobguiRoot *
_bobgui_widget_get_root (BobguiWidget *widget)
{
  return widget->priv->root;
}

static inline GdkDisplay *
_bobgui_widget_get_display (BobguiWidget *widget)
{
  BobguiRoot *root = _bobgui_widget_get_root (widget);

  if (root == NULL)
    return gdk_display_get_default ();

  return bobgui_root_get_display (root);
}

static inline gpointer
_bobgui_widget_peek_request_cache (BobguiWidget *widget)
{
  return &widget->priv->requests;
}

static inline BobguiWidget *
_bobgui_widget_get_prev_sibling (BobguiWidget *widget)
{
  return widget->priv->prev_sibling;
}

static inline BobguiWidget *
_bobgui_widget_get_next_sibling (BobguiWidget *widget)
{
  return widget->priv->next_sibling;
}

static inline BobguiWidget *
_bobgui_widget_get_first_child (BobguiWidget *widget)
{
  return widget->priv->first_child;
}

static inline BobguiWidget *
_bobgui_widget_get_last_child (BobguiWidget *widget)
{
  return widget->priv->last_child;
}

static inline gboolean
_bobgui_widget_is_sensitive (BobguiWidget *widget)
{
  return !(widget->priv->state_flags & BOBGUI_STATE_FLAG_INSENSITIVE);
}

void bobgui_widget_set_accessible_role (BobguiWidget        *self,
                                     BobguiAccessibleRole role);

G_END_DECLS
