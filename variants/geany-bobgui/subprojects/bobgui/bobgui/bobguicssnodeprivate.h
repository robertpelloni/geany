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

#pragma once

#include "bobguicountingbloomfilterprivate.h"
#include "bobguicssnodedeclarationprivate.h"
#include "bobguicssnodestylecacheprivate.h"
#include "bobguicssstylechangeprivate.h"
#include "bobguibitmaskprivate.h"
#include "bobguicsstypesprivate.h"
#include "bobguilistlistmodelprivate.h"

G_BEGIN_DECLS

#define BOBGUI_TYPE_CSS_NODE           (bobgui_css_node_get_type ())
#define BOBGUI_CSS_NODE(obj)           (G_TYPE_CHECK_INSTANCE_CAST (obj, BOBGUI_TYPE_CSS_NODE, BobguiCssNode))
#define BOBGUI_CSS_NODE_CLASS(cls)     (G_TYPE_CHECK_CLASS_CAST (cls, BOBGUI_TYPE_CSS_NODE, BobguiCssNodeClass))
#define BOBGUI_IS_CSS_NODE(obj)        (G_TYPE_CHECK_INSTANCE_TYPE (obj, BOBGUI_TYPE_CSS_NODE))
#define BOBGUI_IS_CSS_NODE_CLASS(obj)  (G_TYPE_CHECK_CLASS_TYPE (obj, BOBGUI_TYPE_CSS_NODE))
#define BOBGUI_CSS_NODE_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), BOBGUI_TYPE_CSS_NODE, BobguiCssNodeClass))

typedef struct _BobguiCssNodeClass         BobguiCssNodeClass;

struct _BobguiCssNode
{
  GObject object;

  BobguiCssNode *parent;
  BobguiCssNode *previous_sibling;
  BobguiCssNode *next_sibling;
  BobguiCssNode *first_child;
  BobguiCssNode *last_child;

  BobguiListListModel *children_observer;

  BobguiCssNodeDeclaration *decl;
  BobguiCssStyle           *style;
  BobguiCssNodeStyleCache  *cache;                 /* cache for children to look up styles */

  BobguiCssChange           pending_changes;       /* changes that accumulated since the style was last computed */

  guint                  visible :1;            /* node will be skipped when validating or computing styles */
  guint                  invalid :1;            /* node or a child needs to be validated (even if just for animation) */
  guint                  needs_propagation :1;  /* children have state changes that need to be propagated to their siblings */
  /* Two invariants hold for this variable:
   * style_is_invalid == TRUE  =>  next_sibling->style_is_invalid == TRUE
   * style_is_invalid == FALSE =>  first_child->style_is_invalid == TRUE
   * So if a valid style is computed, one has to previously ensure that the parent's and the previous sibling's style
   * are valid. This allows both validation and invalidation to run in O(nodes-in-tree) */
  guint                  style_is_invalid :1;   /* the style needs to be recomputed */
};

struct _BobguiCssNodeClass
{
  GObjectClass object_class;

  void                  (* node_added)                  (BobguiCssNode            *cssnode,
                                                         BobguiCssNode            *child,
                                                         BobguiCssNode            *previous);
  void                  (* node_removed)                (BobguiCssNode            *cssnode,
                                                         BobguiCssNode            *child,
                                                         BobguiCssNode            *previous);
  void                  (* style_changed)               (BobguiCssNode            *cssnode,
                                                         BobguiCssStyleChange     *style_change);

  /* get style provider to use or NULL to use parent's */
  BobguiStyleProvider *    (* get_style_provider)          (BobguiCssNode            *cssnode);
  /* get frame clock or NULL (only relevant for root node) */
  GdkFrameClock *       (* get_frame_clock)             (BobguiCssNode            *cssnode);
  BobguiCssStyle *         (* update_style)                (BobguiCssNode            *cssnode,
                                                         const BobguiCountingBloomFilter *filter,
                                                         BobguiCssChange           pending_changes,
                                                         gint64                 timestamp,
                                                         BobguiCssStyle           *old_style);
  void                  (* invalidate)                  (BobguiCssNode            *node);
  void                  (* queue_validate)              (BobguiCssNode            *node);
  void                  (* dequeue_validate)            (BobguiCssNode            *node);
  void                  (* validate)                    (BobguiCssNode            *node);
};

GType                   bobgui_css_node_get_type           (void) G_GNUC_CONST;

BobguiCssNode *            bobgui_css_node_new                (void);

void                    bobgui_css_node_set_parent         (BobguiCssNode            *cssnode,
                                                         BobguiCssNode            *parent);
void                    bobgui_css_node_insert_after       (BobguiCssNode            *parent,
                                                         BobguiCssNode            *cssnode,
                                                         BobguiCssNode            *previous_sibling);
void                    bobgui_css_node_insert_before      (BobguiCssNode            *parent,
                                                         BobguiCssNode            *cssnode,
                                                         BobguiCssNode            *next_sibling);

BobguiCssNode *            bobgui_css_node_get_parent         (BobguiCssNode            *cssnode) G_GNUC_PURE;
BobguiCssNode *            bobgui_css_node_get_first_child    (BobguiCssNode            *cssnode) G_GNUC_PURE;
BobguiCssNode *            bobgui_css_node_get_last_child     (BobguiCssNode            *cssnode) G_GNUC_PURE;
BobguiCssNode *            bobgui_css_node_get_previous_sibling(BobguiCssNode           *cssnode) G_GNUC_PURE;
BobguiCssNode *            bobgui_css_node_get_next_sibling   (BobguiCssNode            *cssnode) G_GNUC_PURE;

void                    bobgui_css_node_set_visible        (BobguiCssNode            *cssnode,
                                                         gboolean               visible);
gboolean                bobgui_css_node_get_visible        (BobguiCssNode            *cssnode) G_GNUC_PURE;

void                    bobgui_css_node_set_name           (BobguiCssNode            *cssnode,
                                                         GQuark                 name);
GQuark                  bobgui_css_node_get_name           (BobguiCssNode            *cssnode) G_GNUC_PURE;
void                    bobgui_css_node_set_id             (BobguiCssNode            *cssnode,
                                                         GQuark                 id);
GQuark                  bobgui_css_node_get_id             (BobguiCssNode            *cssnode) G_GNUC_PURE;
void                    bobgui_css_node_set_state          (BobguiCssNode            *cssnode,
                                                         BobguiStateFlags          state_flags);
BobguiStateFlags           bobgui_css_node_get_state          (BobguiCssNode            *cssnode) G_GNUC_PURE;
void                    bobgui_css_node_set_classes        (BobguiCssNode            *cssnode,
                                                         const char           **classes);
char **                 bobgui_css_node_get_classes        (BobguiCssNode            *cssnode);
gboolean                bobgui_css_node_add_class          (BobguiCssNode            *cssnode,
                                                         GQuark                 style_class);
gboolean                bobgui_css_node_remove_class       (BobguiCssNode            *cssnode,
                                                         GQuark                 style_class);
gboolean                bobgui_css_node_has_class          (BobguiCssNode            *cssnode,
                                                         GQuark                 style_class) G_GNUC_PURE;
const GQuark *          bobgui_css_node_list_classes       (BobguiCssNode            *cssnode,
                                                         guint                 *n_classes);

const BobguiCssNodeDeclaration *
                        bobgui_css_node_get_declaration    (BobguiCssNode            *cssnode) G_GNUC_PURE;
BobguiCssStyle *           bobgui_css_node_get_style          (BobguiCssNode            *cssnode) G_GNUC_PURE;


void                    bobgui_css_node_invalidate_style_provider
                                                        (BobguiCssNode            *cssnode);
void                    bobgui_css_node_invalidate_frame_clock
                                                        (BobguiCssNode            *cssnode,
                                                         gboolean               just_timestamp);
void                    bobgui_css_node_invalidate         (BobguiCssNode            *cssnode,
                                                         BobguiCssChange           change);
void                    bobgui_css_node_validate           (BobguiCssNode            *cssnode);

BobguiStyleProvider *      bobgui_css_node_get_style_provider (BobguiCssNode            *cssnode) G_GNUC_PURE;

typedef enum {
  BOBGUI_CSS_NODE_PRINT_NONE         = 0,
  BOBGUI_CSS_NODE_PRINT_RECURSE      = 1 << 0,
  BOBGUI_CSS_NODE_PRINT_SHOW_STYLE   = 1 << 1,
  BOBGUI_CSS_NODE_PRINT_SHOW_CHANGE  = 1 << 2
} BobguiCssNodePrintFlags;

void                    bobgui_css_node_print              (BobguiCssNode           *cssnode,
                                                         BobguiCssNodePrintFlags  flags,
                                                         GString              *string,
                                                         guint                 indent);

GListModel *            bobgui_css_node_observe_children   (BobguiCssNode                *cssnode);

G_END_DECLS

