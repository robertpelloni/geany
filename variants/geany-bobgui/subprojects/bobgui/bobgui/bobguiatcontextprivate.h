/* bobguiatcontextprivate.h: Private header for BobguiATContext
 *
 * Copyright 2020  GNOME Foundation
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "bobguiatcontext.h"

#include "bobguiaccessibleprivate.h"
#include "bobguiaccessibleattributesetprivate.h"
#include "bobguiaccessibletext.h"

G_BEGIN_DECLS

typedef enum {
  BOBGUI_ACCESSIBLE_PROPERTY_CHANGE_AUTOCOMPLETE     = 1 << BOBGUI_ACCESSIBLE_PROPERTY_AUTOCOMPLETE,
  BOBGUI_ACCESSIBLE_PROPERTY_CHANGE_DESCRIPTION      = 1 << BOBGUI_ACCESSIBLE_PROPERTY_DESCRIPTION,
  BOBGUI_ACCESSIBLE_PROPERTY_CHANGE_HAS_POPUP        = 1 << BOBGUI_ACCESSIBLE_PROPERTY_HAS_POPUP,
  BOBGUI_ACCESSIBLE_PROPERTY_CHANGE_KEY_SHORTCUTS    = 1 << BOBGUI_ACCESSIBLE_PROPERTY_KEY_SHORTCUTS,
  BOBGUI_ACCESSIBLE_PROPERTY_CHANGE_LABEL            = 1 << BOBGUI_ACCESSIBLE_PROPERTY_LABEL,
  BOBGUI_ACCESSIBLE_PROPERTY_CHANGE_LEVEL            = 1 << BOBGUI_ACCESSIBLE_PROPERTY_LEVEL,
  BOBGUI_ACCESSIBLE_PROPERTY_CHANGE_MODAL            = 1 << BOBGUI_ACCESSIBLE_PROPERTY_MODAL,
  BOBGUI_ACCESSIBLE_PROPERTY_CHANGE_MULTI_LINE       = 1 << BOBGUI_ACCESSIBLE_PROPERTY_MULTI_LINE,
  BOBGUI_ACCESSIBLE_PROPERTY_CHANGE_MULTI_SELECTABLE = 1 << BOBGUI_ACCESSIBLE_PROPERTY_MULTI_SELECTABLE,
  BOBGUI_ACCESSIBLE_PROPERTY_CHANGE_ORIENTATION      = 1 << BOBGUI_ACCESSIBLE_PROPERTY_ORIENTATION,
  BOBGUI_ACCESSIBLE_PROPERTY_CHANGE_PLACEHOLDER      = 1 << BOBGUI_ACCESSIBLE_PROPERTY_PLACEHOLDER,
  BOBGUI_ACCESSIBLE_PROPERTY_CHANGE_READ_ONLY        = 1 << BOBGUI_ACCESSIBLE_PROPERTY_READ_ONLY,
  BOBGUI_ACCESSIBLE_PROPERTY_CHANGE_REQUIRED         = 1 << BOBGUI_ACCESSIBLE_PROPERTY_REQUIRED,
  BOBGUI_ACCESSIBLE_PROPERTY_CHANGE_ROLE_DESCRIPTION = 1 << BOBGUI_ACCESSIBLE_PROPERTY_ROLE_DESCRIPTION,
  BOBGUI_ACCESSIBLE_PROPERTY_CHANGE_SORT             = 1 << BOBGUI_ACCESSIBLE_PROPERTY_SORT,
  BOBGUI_ACCESSIBLE_PROPERTY_CHANGE_VALUE_MAX        = 1 << BOBGUI_ACCESSIBLE_PROPERTY_VALUE_MAX,
  BOBGUI_ACCESSIBLE_PROPERTY_CHANGE_VALUE_MIN        = 1 << BOBGUI_ACCESSIBLE_PROPERTY_VALUE_MIN,
  BOBGUI_ACCESSIBLE_PROPERTY_CHANGE_VALUE_NOW        = 1 << BOBGUI_ACCESSIBLE_PROPERTY_VALUE_NOW,
  BOBGUI_ACCESSIBLE_PROPERTY_CHANGE_VALUE_TEXT       = 1 << BOBGUI_ACCESSIBLE_PROPERTY_VALUE_TEXT,
  BOBGUI_ACCESSIBLE_PROPERTY_CHANGE_HELP_TEXT        = 1 << BOBGUI_ACCESSIBLE_PROPERTY_HELP_TEXT,
} BobguiAccessiblePropertyChange;

typedef enum {
  BOBGUI_ACCESSIBLE_RELATION_CHANGE_ACTIVE_DESCENDANT = 1 << BOBGUI_ACCESSIBLE_RELATION_ACTIVE_DESCENDANT,
  BOBGUI_ACCESSIBLE_RELATION_CHANGE_COL_COUNT         = 1 << BOBGUI_ACCESSIBLE_RELATION_COL_COUNT,
  BOBGUI_ACCESSIBLE_RELATION_CHANGE_COL_INDEX         = 1 << BOBGUI_ACCESSIBLE_RELATION_COL_INDEX,
  BOBGUI_ACCESSIBLE_RELATION_CHANGE_COL_INDEX_TEXT    = 1 << BOBGUI_ACCESSIBLE_RELATION_COL_INDEX_TEXT,
  BOBGUI_ACCESSIBLE_RELATION_CHANGE_COL_SPAN          = 1 << BOBGUI_ACCESSIBLE_RELATION_COL_SPAN,
  BOBGUI_ACCESSIBLE_RELATION_CHANGE_CONTROLS          = 1 << BOBGUI_ACCESSIBLE_RELATION_CONTROLS,
  BOBGUI_ACCESSIBLE_RELATION_CHANGE_DESCRIBED_BY      = 1 << BOBGUI_ACCESSIBLE_RELATION_DESCRIBED_BY,
  BOBGUI_ACCESSIBLE_RELATION_CHANGE_DETAILS           = 1 << BOBGUI_ACCESSIBLE_RELATION_DETAILS,
  BOBGUI_ACCESSIBLE_RELATION_CHANGE_ERROR_MESSAGE     = 1 << BOBGUI_ACCESSIBLE_RELATION_ERROR_MESSAGE,
  BOBGUI_ACCESSIBLE_RELATION_CHANGE_FLOW_TO           = 1 << BOBGUI_ACCESSIBLE_RELATION_FLOW_TO,
  BOBGUI_ACCESSIBLE_RELATION_CHANGE_LABELLED_BY       = 1 << BOBGUI_ACCESSIBLE_RELATION_LABELLED_BY,
  BOBGUI_ACCESSIBLE_RELATION_CHANGE_OWNS              = 1 << BOBGUI_ACCESSIBLE_RELATION_OWNS,
  BOBGUI_ACCESSIBLE_RELATION_CHANGE_POS_IN_SET        = 1 << BOBGUI_ACCESSIBLE_RELATION_POS_IN_SET,
  BOBGUI_ACCESSIBLE_RELATION_CHANGE_ROW_COUNT         = 1 << BOBGUI_ACCESSIBLE_RELATION_ROW_COUNT,
  BOBGUI_ACCESSIBLE_RELATION_CHANGE_ROW_INDEX         = 1 << BOBGUI_ACCESSIBLE_RELATION_ROW_INDEX,
  BOBGUI_ACCESSIBLE_RELATION_CHANGE_ROW_INDEX_TEXT    = 1 << BOBGUI_ACCESSIBLE_RELATION_ROW_INDEX_TEXT,
  BOBGUI_ACCESSIBLE_RELATION_CHANGE_ROW_SPAN          = 1 << BOBGUI_ACCESSIBLE_RELATION_ROW_SPAN,
  BOBGUI_ACCESSIBLE_RELATION_CHANGE_SET_SIZE          = 1 << BOBGUI_ACCESSIBLE_RELATION_SET_SIZE,
  BOBGUI_ACCESSIBLE_RELATION_CHANGE_LABEL_FOR         = 1 << BOBGUI_ACCESSIBLE_RELATION_LABEL_FOR,
  BOBGUI_ACCESSIBLE_RELATION_CHANGE_DESCRIPTION_FOR   = 1 << BOBGUI_ACCESSIBLE_RELATION_DESCRIPTION_FOR,
  BOBGUI_ACCESSIBLE_RELATION_CHANGE_CONTROLLED_BY     = 1 << BOBGUI_ACCESSIBLE_RELATION_CONTROLLED_BY,
  BOBGUI_ACCESSIBLE_RELATION_CHANGE_DETAILS_FOR       = 1 << BOBGUI_ACCESSIBLE_RELATION_DETAILS_FOR,
  BOBGUI_ACCESSIBLE_RELATION_CHANGE_ERROR_MESSAGE_FOR = 1 << BOBGUI_ACCESSIBLE_RELATION_ERROR_MESSAGE_FOR,
  BOBGUI_ACCESSIBLE_RELATION_CHANGE_FLOW_FROM = 1 << BOBGUI_ACCESSIBLE_RELATION_FLOW_FROM,
} BobguiAccessibleRelationChange;

typedef enum {
  BOBGUI_ACCESSIBLE_STATE_CHANGE_BUSY     = 1 << BOBGUI_ACCESSIBLE_STATE_BUSY,
  BOBGUI_ACCESSIBLE_STATE_CHANGE_CHECKED  = 1 << BOBGUI_ACCESSIBLE_STATE_CHECKED,
  BOBGUI_ACCESSIBLE_STATE_CHANGE_DISABLED = 1 << BOBGUI_ACCESSIBLE_STATE_DISABLED,
  BOBGUI_ACCESSIBLE_STATE_CHANGE_EXPANDED = 1 << BOBGUI_ACCESSIBLE_STATE_EXPANDED,
  BOBGUI_ACCESSIBLE_STATE_CHANGE_HIDDEN   = 1 << BOBGUI_ACCESSIBLE_STATE_HIDDEN,
  BOBGUI_ACCESSIBLE_STATE_CHANGE_INVALID  = 1 << BOBGUI_ACCESSIBLE_STATE_INVALID,
  BOBGUI_ACCESSIBLE_STATE_CHANGE_PRESSED  = 1 << BOBGUI_ACCESSIBLE_STATE_PRESSED,
  BOBGUI_ACCESSIBLE_STATE_CHANGE_SELECTED = 1 << BOBGUI_ACCESSIBLE_STATE_SELECTED,
  BOBGUI_ACCESSIBLE_STATE_CHANGE_VISITED = 1 << BOBGUI_ACCESSIBLE_STATE_VISITED
} BobguiAccessibleStateChange;

struct _BobguiATContext
{
  GObject parent_instance;

  BobguiAccessibleRole accessible_role;
  BobguiAccessible *accessible;
  BobguiAccessible *accessible_parent;
  BobguiAccessible *next_accessible_sibling;
  GdkDisplay *display;

  BobguiAccessibleAttributeSet *states;
  BobguiAccessibleAttributeSet *properties;
  BobguiAccessibleAttributeSet *relations;

  BobguiAccessibleStateChange updated_states;
  BobguiAccessiblePropertyChange updated_properties;
  BobguiAccessibleRelationChange updated_relations;
  BobguiAccessiblePlatformChange updated_platform;

  guint realized : 1;
};

struct _BobguiATContextClass
{
  GObjectClass parent_class;

  void (* state_change) (BobguiATContext                *self,
                         BobguiAccessibleStateChange     changed_states,
                         BobguiAccessiblePropertyChange  changed_properties,
                         BobguiAccessibleRelationChange  changed_relations,
                         BobguiAccessibleAttributeSet   *states,
                         BobguiAccessibleAttributeSet   *properties,
                         BobguiAccessibleAttributeSet   *relations);

  void (* platform_change) (BobguiATContext                *self,
                            BobguiAccessiblePlatformChange  changed_platform);

  void (* bounds_change) (BobguiATContext                *self);

  void (* child_change) (BobguiATContext             *self,
                         BobguiAccessibleChildChange  changed_child,
                         BobguiAccessible            *child);

  void (* realize)       (BobguiATContext *self);
  void (* unrealize)     (BobguiATContext *self);

  void (* announce)      (BobguiATContext *self,
                          const char   *message,
                          BobguiAccessibleAnnouncementPriority priority);

  /* Text interface */
  void (* update_caret_position) (BobguiATContext *self);
  void (* update_selection_bound) (BobguiATContext *self);
  void (* update_text_contents) (BobguiATContext *self,
                                 BobguiAccessibleTextContentChange change,
                                 unsigned int start,
                                 unsigned int end);
};

BobguiATContext *          bobgui_at_context_clone                    (BobguiATContext          *self,
                                                                 BobguiAccessibleRole      role,
                                                                 BobguiAccessible         *accessible,
                                                                 GdkDisplay            *display);

void                    bobgui_at_context_set_display              (BobguiATContext          *self,
                                                                 GdkDisplay            *display);
GdkDisplay *            bobgui_at_context_get_display              (BobguiATContext          *self);
void                    bobgui_at_context_set_accessible_role      (BobguiATContext          *self,
                                                                 BobguiAccessibleRole      role);

void                    bobgui_at_context_realize                  (BobguiATContext          *self);
void                    bobgui_at_context_unrealize                (BobguiATContext          *self);
gboolean                bobgui_at_context_is_realized              (BobguiATContext          *self);

void                    bobgui_at_context_update                   (BobguiATContext          *self);

void                    bobgui_at_context_set_accessible_state     (BobguiATContext          *self,
                                                                 BobguiAccessibleState     state,
                                                                 BobguiAccessibleValue    *value);
gboolean                bobgui_at_context_has_accessible_state     (BobguiATContext          *self,
                                                                 BobguiAccessibleState     state);
BobguiAccessibleValue *    bobgui_at_context_get_accessible_state     (BobguiATContext          *self,
                                                                 BobguiAccessibleState     state);
void                    bobgui_at_context_set_accessible_property  (BobguiATContext          *self,
                                                                 BobguiAccessibleProperty  property,
                                                                 BobguiAccessibleValue    *value);
gboolean                bobgui_at_context_has_accessible_property  (BobguiATContext          *self,
                                                                 BobguiAccessibleProperty  property);
BobguiAccessibleValue *    bobgui_at_context_get_accessible_property  (BobguiATContext          *self,
                                                                 BobguiAccessibleProperty  property);
void                    bobgui_at_context_set_accessible_relation  (BobguiATContext          *self,
                                                                 BobguiAccessibleRelation  property,
                                                                 BobguiAccessibleValue    *value);
gboolean                bobgui_at_context_has_accessible_relation  (BobguiATContext          *self,
                                                                 BobguiAccessibleRelation  relation);
BobguiAccessibleValue *    bobgui_at_context_get_accessible_relation  (BobguiATContext          *self,
                                                                 BobguiAccessibleRelation  relation);

char *                  bobgui_at_context_get_name                 (BobguiATContext          *self);
char *                  bobgui_at_context_get_description          (BobguiATContext          *self);

void                    bobgui_at_context_platform_changed         (BobguiATContext                *self,
                                                                 BobguiAccessiblePlatformChange  change);
void                    bobgui_at_context_bounds_changed           (BobguiATContext                *self);
void                    bobgui_at_context_child_changed            (BobguiATContext                *self,
                                                                 BobguiAccessibleChildChange     change,
                                                                 BobguiAccessible               *child);

const char *    bobgui_accessible_property_get_attribute_name      (BobguiAccessibleProperty property);
const char *    bobgui_accessible_relation_get_attribute_name      (BobguiAccessibleRelation relation);
const char *    bobgui_accessible_state_get_attribute_name         (BobguiAccessibleState    state);

BobguiAccessible *
bobgui_at_context_get_accessible_parent (BobguiATContext *self);
void
bobgui_at_context_set_accessible_parent (BobguiATContext *self,
                                      BobguiAccessible *parent);
BobguiAccessible *
bobgui_at_context_get_next_accessible_sibling (BobguiATContext *self);
void
bobgui_at_context_set_next_accessible_sibling (BobguiATContext *self,
                                            BobguiAccessible *sibling);

void bobgui_at_context_announce (BobguiATContext                     *self,
                              const char                       *message,
                              BobguiAccessibleAnnouncementPriority priority);
void
bobgui_at_context_update_caret_position (BobguiATContext *self);
void
bobgui_at_context_update_selection_bound (BobguiATContext *self);
void
bobgui_at_context_update_text_contents (BobguiATContext *self,
                                     BobguiAccessibleTextContentChange change,
                                     unsigned int start,
                                     unsigned int end);

gboolean
bobgui_at_context_is_nested_button (BobguiATContext *self);

G_END_DECLS
